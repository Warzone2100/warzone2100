/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2020  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/**
 * @file nettypes.c
 *
 * Contains the 'new' Network API functions for sending and receiving both
 * low-level primitives and higher-level types.
 */

#include "lib/framework/wzglobal.h"
#include "lib/framework/string_ext.h"
#include "lib/gamelib/gtime.h"
#include <string.h>

#ifndef WZ_OS_WIN
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

#include "lib/framework/frame.h"
#include "lib/netplay/byteorder_funcs_wrapper.h"
#include "netplay.h"
#include "netreplay.h"
#include "nettypes.h"
#include "netqueue.h"
#include "netlog.h"
#include "src/order.h"
#include <cstring>
#include <limits>
#include <array>

/// There is a game queue representing each player. The game queues are synchronised among all players, so that all players process the same game queue
/// messages at the same game time. The game queues should be used, even in single-player. Players should write to their own queue, not to other player's
/// queues, and should read messages from all queues including their own.
/// (Note: Extra +1 for the added replay spectator)
static NetQueue *gameQueues[MAX_GAMEQUEUE_SLOTS] = {nullptr};

/// There is a bidirectional net queue for communicating with each client or host. Each queue corresponds either to a real socket, or a virtual socket
/// which routes via the host.
static NetQueuePair *netQueues[MAX_CONNECTED_PLAYERS] = {nullptr};

/// These queues are for clients which just connected, but haven't yet been assigned a player number.
static NetQueuePair *tmpQueues[MAX_TMP_SOCKETS] = {nullptr};

/// Sending a message to the broadcast queue is equivalent to sending the message to the net queues of all other players.
static NetQueue *broadcastQueue = nullptr;

static std::array<std::unique_ptr<SessionKeys>, MAX_CONNECTED_PLAYERS> netSessionKeys;

static bool bIsReplay = false;

static size_t numInvalidMessageReads = 0;

// Queue selection functions

/// Gets the &NetQueuePair::send or NetQueue *, corresponding to queue.
static NetQueue *sendQueue(NETQUEUE queue)
{
	return queue.isPair ? &(*static_cast<NetQueuePair **>(queue.queue))->send : static_cast<NetQueue *>(queue.queue);
}

/// Gets the &NetQueuePair::receive or NetQueue *, corresponding to queue.
static NetQueue *receiveQueue(NETQUEUE queue)
{
	return queue.isPair ? &(*static_cast<NetQueuePair **>(queue.queue))->receive : static_cast<NetQueue *>(queue.queue);
}

/// Gets the &NetQueuePair, corresponding to queue.
static NetQueuePair *&pairQueue(NETQUEUE queue)
{
	ASSERT(queue.isPair, "Huh?");
	return *static_cast<NetQueuePair **>(queue.queue);
}

NETQUEUE NETnetTmpQueue(unsigned tmpPlayer)
{
	NETQUEUE ret;
	ASSERT(tmpPlayer < MAX_TMP_SOCKETS, "Huh?");
	NetQueuePair **queue = &tmpQueues[tmpPlayer];
	ret.queue = queue;
	ret.isPair = true;
	ret.index = tmpPlayer;
	ret.queueType = QUEUE_TMP;
	ret.exclude = NET_NO_EXCLUDE;
	return ret;
}

NETQUEUE NETnetQueue(unsigned player, unsigned excludePlayer)
{
	NETQUEUE ret;

	if (player == NET_ALL_PLAYERS)
	{
		return NETbroadcastQueue(excludePlayer);
	}

	ASSERT(player < MAX_CONNECTED_PLAYERS, "Huh?");
	NetQueuePair **queue = &netQueues[player];
	ret.queue = queue;
	ret.isPair = true;
	ret.index = player;
	ret.queueType = QUEUE_NET;
	ret.exclude = excludePlayer;
	return ret;
}

NETQUEUE NETgameQueue(unsigned player)
{
	NETQUEUE ret;
	if (player >= MAX_GAMEQUEUE_SLOTS)
	{
		// found one
		debug(LOG_ERROR, "Found the call");
	}
	ASSERT(player < MAX_GAMEQUEUE_SLOTS, "Huh?");
	NetQueue *queue = gameQueues[player];
	ret.queue = queue;
	ret.isPair = false;
	ret.index = player;
	ret.queueType = QUEUE_GAME;
	ret.exclude = NET_NO_EXCLUDE;
	return ret;
}

bool NETgameIsBehindPlayersByAtLeast(size_t numGameTimeUpdates /*= 2*/)
{
	// if we should be waited on, then there's no reason we should be behind other players
	if (gtimeShouldWaitForPlayer(realSelectedPlayer))
	{
		return false;
	}

	unsigned begin = 0, end = MAX_CONNECTED_PLAYERS;

	for (unsigned player = begin; player < end; ++player)
	{
		auto pPlayerGameQueue = gameQueues[player];
		if (!pPlayerGameQueue)
		{
			continue;
		}

		if (gtimeShouldWaitForPlayer(player) && pPlayerGameQueue->numPendingGameTimeUpdateMessages() < numGameTimeUpdates)
		{
			return false;  // Do not have enough pending game time updates for this player
		}
	}

	return true;  // Have enough pending game time updates from all players that should be waited on
}

NETQUEUE NETgameQueueForced(unsigned player)
{
	NETQUEUE ret;
	ASSERT(player < MAX_CONNECTED_PLAYERS, "Huh?");
	NetQueue *queue = gameQueues[player];
	ret.queue = queue;
	ret.isPair = false;
	ret.index = player;
	ret.queueType = QUEUE_GAME_FORCED;
	ret.exclude = NET_NO_EXCLUDE;
	return ret;
}

NETQUEUE NETbroadcastQueue(unsigned excludePlayer)
{
	NETQUEUE ret;
	NetQueue *queue = broadcastQueue;
	ret.queue = queue;
	ret.isPair = false;
	ret.index = NET_ALL_PLAYERS;
	ret.queueType = QUEUE_BROADCAST;
	ret.exclude = excludePlayer;
	return ret;
}

void NETinsertRawData(NETQUEUE queue, uint8_t *data, size_t dataLen)
{
	receiveQueue(queue)->writeRawData(data, dataLen);
}

void NETinsertMessageFromNet(NETQUEUE queue, NetMessage&& newMessage)
{
	receiveQueue(queue)->pushMessage(std::move(newMessage));
}

bool NETisMessageReady(NETQUEUE queue)
{
	return receiveQueue(queue)->haveMessage();
}

size_t NETincompleteMessageDataBuffered(NETQUEUE queue)
{
	return receiveQueue(queue)->currentIncompleteDataBuffered();
}

NetMessage const *NETgetMessage(NETQUEUE queue)
{
	return &receiveQueue(queue)->getMessage();
}

/*
 * Begin & End functions
 */

void NETinitQueue(NETQUEUE queue)
{
	if (queue.queueType == QUEUE_BROADCAST)
	{
		delete broadcastQueue;
		broadcastQueue = new NetQueue;
		broadcastQueue->setWillNeverGetMessages();
		return;
	}
	else if (queue.queueType == QUEUE_GAME)
	{
		delete gameQueues[queue.index];
		gameQueues[queue.index] = new NetQueue;
		return;
	}
	else
	{
		delete pairQueue(queue);
		pairQueue(queue) = new NetQueuePair;
	}
}

void NETdeleteQueue(void)
{
	for (int i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		delete pairQueue(NETnetQueue(i));
		pairQueue(NETnetQueue(i)) = nullptr;
		delete gameQueues[i];
		gameQueues[i] = nullptr;
	}

	// extra replay spectator gamequeue
	delete gameQueues[MAX_CONNECTED_PLAYERS];
	gameQueues[MAX_CONNECTED_PLAYERS] = nullptr;

	delete broadcastQueue;
	broadcastQueue = nullptr;

	bIsReplay = false;

	if (numInvalidMessageReads != 0)
	{
		debug(LOG_NET, "Experienced %zu invalid message reads", numInvalidMessageReads);
	}
	numInvalidMessageReads = 0;
}

void NETsetNoSendOverNetwork(NETQUEUE queue)
{
	sendQueue(queue)->setWillNeverGetMessagesForNet();  // Will not be sending over net.
}

void NETmoveQueue(NETQUEUE src, NETQUEUE dst)
{
	ASSERT(src.isPair, "Huh?");
	ASSERT(dst.isPair, "Huh?");
	delete pairQueue(dst);
	pairQueue(dst) = nullptr;
	std::swap(pairQueue(src), pairQueue(dst));
}

void NETswapQueues(NETQUEUE src, NETQUEUE dst)
{
	ASSERT(src.isPair, "Huh?");
	ASSERT(dst.isPair, "Huh?");
	std::swap(pairQueue(src), pairQueue(dst));
}

MessageWriter NETbeginEncode(NETQUEUE queue, uint8_t type)
{
	return MessageWriter(queue, type);
}

MessageReader NETbeginDecode(NETQUEUE queue, uint8_t type)
{
	auto res = MessageReader(receiveQueue(queue)->getMessage());
	assert(type == res.msgType);
	return res;
}

void NETsetSessionKeys(uint8_t player, SessionKeys&& keys)
{
	ASSERT_OR_RETURN(, player < netSessionKeys.size(), "Invalid player: %u", static_cast<unsigned>(player));
	netSessionKeys[player] = std::make_unique<SessionKeys>(std::move(keys));
}

void NETclearSessionKeys()
{
	for (auto& key : netSessionKeys)
	{
		key.reset();
	}
}

void NETclearSessionKeys(uint8_t player)
{
	ASSERT_OR_RETURN(, player < netSessionKeys.size(), "Invalid player: %u", static_cast<unsigned>(player));
	netSessionKeys[player].reset();
}

bool NETisExpectedSecuredMessageType(uint8_t type)
{
	switch (type)
	{
		case NET_TEAM_STRATEGY:
			return true;
		case NET_QUICK_CHAT_MSG:
			return true;
		default:
			return false;
	}
}

// For encoding a secured net message, for a *specific player*
// Returns `false` on failure
// Notes:
//	- Only for NET_* messages
optional<MessageWriter> NETbeginEncodeSecured(NETQUEUE queue, uint8_t type)
{
	ASSERT_OR_RETURN(nullopt, type < NET_MAX_TYPE, "Message type %u is >= NET_MAX_TYPE", static_cast<unsigned>(type));
	ASSERT_OR_RETURN(nullopt, queue.index != realSelectedPlayer, "Secured messages are for other players, not ourselves.");
	ASSERT_OR_RETURN(nullopt, queue.index < MAX_PLAYERS || queue.index == NetPlay.hostPlayer, "Invalid recipient (queue.index == %u)", static_cast<unsigned>(queue.index));
	ASSERT_OR_RETURN(nullopt, netSessionKeys[queue.index] != nullptr, "Lacking session key for recipient: %u", static_cast<unsigned>(queue.index));
	ASSERT(NETisExpectedSecuredMessageType(type), "Message type is not expected to be secured, and will be ignored on receipt");

	auto w = NETbeginEncode(queue, type);
	w.bSecretMessageWrap = true;

	return w;
}

// For decoding what is expected to have been a secured message
// Returns `false` on failure
// Notes:
//	- *DO NOT CALL NETend() if this returns false!!*
//	- Only for NET_* messages
optional<MessageReader> NETbeginDecodeSecured(NETQUEUE queue, uint8_t type)
{
	ASSERT_OR_RETURN(nullopt, type < NET_MAX_TYPE, "Message type %u is >= NET_MAX_TYPE", static_cast<unsigned>(type));
	ASSERT_OR_RETURN(nullopt, queue.index != realSelectedPlayer, "Secured messages are for other players, not ourselves.");
	ASSERT_OR_RETURN(nullopt, queue.index < MAX_PLAYERS || queue.index == NetPlay.hostPlayer, "Invalid sender (queue.index == %u)", static_cast<unsigned>(queue.index));
	ASSERT_OR_RETURN(nullopt, receiveQueue(queue)->currentMessageWasDecrypted(), "Message was not sent secured (type: %s)", messageTypeToString(type));

	return NETbeginDecode(queue, type);
}

// Decrypts a secured net message in a queue *and replaces it with the decrypted message*
// If message is successfully decrypted:
//	- Returns true, updates the current message in queue to be the decrypted message, updates `type`
bool NETdecryptSecuredNetMessage(NETQUEUE queue, uint8_t& type)
{
	ASSERT_OR_RETURN(false, queue.index < MAX_PLAYERS || queue.index == NetPlay.hostPlayer, "Invalid sender (queue.index == %u", static_cast<unsigned>(queue.index));
	ASSERT_OR_RETURN(false, netSessionKeys[queue.index] != nullptr, "Lacking session key for player: %u", static_cast<unsigned>(queue.index));
	ASSERT_OR_RETURN(false, type == NET_SECURED_NET_MESSAGE, "Not a secured message?");

	auto pReceiveQueue = receiveQueue(queue);

	// Get & decrypt message raw data
	const auto& encryptedMessage = pReceiveQueue->getMessage();
	ASSERT_OR_RETURN(false, encryptedMessage.type() == NET_SECURED_NET_MESSAGE, "Not a secured message?");
	std::vector<uint8_t> decryptedMessageRawData;
	if (!netSessionKeys[queue.index]->decryptMessageFromOther(encryptedMessage.payload(), encryptedMessage.payloadSize(), decryptedMessageRawData))
	{
		debug(LOG_INFO, "Invalid encrypted message from player: %u", static_cast<unsigned>(queue.index));
		return false;
	}

	auto decryptedMessage = NetMessage::tryFromRawData(decryptedMessageRawData.data(), decryptedMessageRawData.size());
	if (!decryptedMessage)
	{
		debug(LOG_INFO, "Failed to parse decrypted data from player: %u", static_cast<unsigned>(queue.index));
		return false;
	}

	if (!(decryptedMessage->type() > NET_MIN_TYPE && decryptedMessage->type() < NET_MAX_TYPE))
	{
		debug(LOG_NET, "Not a secured NET_* message? (type: %s) - ignoring", messageTypeToString(decryptedMessage->type()));
		return false;
	}

	if (!NETisExpectedSecuredMessageType(decryptedMessage->type()))
	{
		// Ignore message types that aren't expected to be secured
		debug(LOG_NET, "Not a message type that's expected to be secured: (type: %s) - ignoring", messageTypeToString(decryptedMessage->type()));
		return false;
	}

	NETlogPacket(NET_SECURED_NET_MESSAGE, static_cast<uint32_t>(encryptedMessage.rawData().size()), true);

	type = decryptedMessage->type(); // must update type!
	pReceiveQueue->replaceCurrentWithDecrypted(std::move(*decryptedMessage));
	return true;
}

bool NETend(MessageReader& r)
{
	bool result = r.valid();
	if (!result)
	{
		++numInvalidMessageReads;
	}
	return result;
}

static std::vector<uint8_t> tmpMessageRawDataBuffer;

bool NETend(MessageWriter& w)
{
	bool shouldWrapSecretMessage = w.bSecretMessageWrap;
	w.bSecretMessageWrap = false; // reset, always

	if (bIsReplay && w.queueInfo.index != realSelectedPlayer)
	{
		// don't bother adding to the send queue if we're playing a replay
		return true;
	}

	// Push the message onto the list.
	NetQueue* queue = sendQueue(w.queueInfo);
	if (queue == nullptr)
	{
		debug(LOG_WARNING, "Sending %s to null queue, type %d.", messageTypeToString(w.msgBuilder.type()), w.queueInfo.queueType);
		return true;
	}

	auto msg = w.msgBuilder.build();
	if (shouldWrapSecretMessage)
	{
		// Need to actually encrypt and wrap the current net message in a NET_SECURED_NET_MESSAGE message
		ASSERT(msg.type() < NET_MAX_TYPE, "Message type %u is >= NET_MAX_TYPE", static_cast<unsigned>(msg.type()));
		ASSERT(w.queueInfo.index < MAX_PLAYERS || w.queueInfo.index == NetPlay.hostPlayer, "Invalid recipient (queue.index == %u)", static_cast<unsigned>(w.queueInfo.index));
		ASSERT(netSessionKeys[w.queueInfo.index] != nullptr, "Lacking session key for recipient: %u", static_cast<unsigned>(w.queueInfo.index));

		// Decoded in NETprocessSystemMessage in netplay.cpp.
		// Encrypt the serialized message (including type and size)
		tmpMessageRawDataBuffer.clear();
		msg.rawDataAppendToVector(tmpMessageRawDataBuffer);

		auto encryptedData = netSessionKeys[w.queueInfo.index]->encryptMessageForOther(&tmpMessageRawDataBuffer[0], tmpMessageRawDataBuffer.size());
		NetMessageBuilder encryptedNetMessage(NET_SECURED_NET_MESSAGE, encryptedData.size());
		encryptedNetMessage.append(encryptedData.data(), encryptedData.size());
		msg = encryptedNetMessage.build();
	}

	optional<NetMessage> shareGameQueueMsg;
	if (w.queueInfo.queueType == QUEUE_GAME_FORCED)
	{
		shareGameQueueMsg = msg;
	}

	auto msgType = msg.type();
	auto msgRawDataSize = msg.rawData().size();

	NETlogPacket(msgType, static_cast<uint32_t>(msgRawDataSize), false);
	queue->pushMessage(std::move(msg));

	if (w.queueInfo.queueType == QUEUE_GAME || w.queueInfo.queueType == QUEUE_GAME_FORCED)
	{
		ASSERT(msgType > GAME_MIN_TYPE && msgType < GAME_MAX_TYPE, "Inserting %s into game queue.", messageTypeToString(msgType));
	}
	else
	{
		ASSERT(msgType > NET_MIN_TYPE && msgType < NET_MAX_TYPE, "Inserting %s into net queue.", messageTypeToString(msgType));
	}

	if (w.queueInfo.queueType == QUEUE_NET || w.queueInfo.queueType == QUEUE_BROADCAST || w.queueInfo.queueType == QUEUE_TMP)
	{
		NETsend(w.queueInfo, queue->getMessageForNet());
		queue->popMessageForNet();
		ASSERT(queue->numMessagesForNet() == 0, "Queue not empty (%u messages remaining). (message = type: %" PRIu8 ", size: %zu), (queue = index: %" PRIu8 "; queueType: %" PRIu8 "; exclude: %" PRIu8 "; isPair: %d)", queue->numMessagesForNet(), msgType, msgRawDataSize, w.queueInfo.index, w.queueInfo.queueType, w.queueInfo.exclude, (int)w.queueInfo.isPair);
	}

	// Process any delayed actions from the NETsend call
	NETsendProcessDelayedActions();

	if (w.queueInfo.queueType == QUEUE_GAME_FORCED)  // If true, we must be the host, inserting a GAME_PLAYER_LEFT into the other player's game queue. Since they left, they're not around to complain about us messing with their queue, which would normally cause a desynch.
	{
		// Almost duplicate code from NETflushGameQueues() in here.
		// Message must be sent as message in a NET_SHARE_GAME_QUEUE in a NET_SEND_TO_PLAYER.
		// If not sent inside a NET_SEND_TO_PLAYER, and the message arrives at the same time as a real message from that player, then the messages may be processed in an unexpected order.
		// See NETrecvNet, in the for (current = 0; current < MAX_CONNECTED_PLAYERS; ++current) loop.
		// Assume dropped client sends a NET_SENT_TO_PLAYERS(broadcast, NET_SHARE_GAME_QUEUE(GAME_REALMESSAGE)), and then drops.
		// The host then sends the NET_SEND_TO_PLAYER(broadcast, NET_SHARE_GAME_QUEUE(GAME_REALMESSAGE)) to everyone. If the host then spoofs a NET_SHARE_GAME_QUEUE(GAME_PLAYER_LEFT) without
		// wrapping it in a NET_SEND_TO_PLAYER from that player, then the client may sometimes unwrap the real NET_SEND_TO_PLAYER message, then process the host's spoofed NET_SHARE_GAME_QUEUE
		// message, and then after that process the previously unwrapped NET_SHARE_GAME_QUEUE(GAME_REALMESSAGE) message, such that the GAME_PLAYER_LEFT appears on the queue before the
		// GAME_REALMESSAGE.

		// Decoded in NETprocessSystemMessage in netplay.cpp.
		uint8_t player = w.queueInfo.index;
		auto shareQueueWriter = NETbeginEncode(NETbroadcastQueue(), NET_SHARE_GAME_QUEUE);
		NETuint8_t(shareQueueWriter, player);
		NETuint32_t(shareQueueWriter, 1);
		NETnetMessage(shareQueueWriter, *shareGameQueueMsg);

		uint8_t allPlayers = NET_ALL_PLAYERS;
		auto sendToPlayerWriter = NETbeginEncode(NETbroadcastQueue(), NET_SEND_TO_PLAYER);
		NETuint8_t(sendToPlayerWriter, player);
		NETuint8_t(sendToPlayerWriter, allPlayers);
		NETnetMessage(sendToPlayerWriter, shareQueueWriter.msgBuilder.build());
		NETend(sendToPlayerWriter);  // This time we actually send it.
	}

	return true;  // Serialising never fails.
}

void NETflushGameQueues()
{
	for (uint8_t player = 0; player < MAX_GAMEQUEUE_SLOTS; ++player)
	{
		NetQueue *queue = gameQueues[player];

		if (queue == nullptr)
		{
			continue;  // Can't send for this player.
		}

		uint32_t num = queue->numMessagesForNet();

		if (num <= 0)
		{
			continue;  // Nothing to send for this player.
		}

		ASSERT(!bIsReplay, "Where are we sending this if it's a replay?");

		// Decoded in NETprocessSystemMessage in netplay.cpp.
		auto w = NETbeginEncode(NETbroadcastQueue(), NET_SHARE_GAME_QUEUE);
		NETuint8_t(w, player);
		NETuint32_t(w, num);
		for (uint32_t n = 0; n < num; ++n)
		{
			NETnetMessage(w, queue->getMessageForNet());
			queue->popMessageForNet();
		}
		NETend(w);
	}
}

void NETpop(NETQUEUE queue)
{
	receiveQueue(queue)->popMessage();
}

void NETbytesOutputToVector(const std::vector<uint8_t> &data, std::vector<uint8_t>& output)
{
	// same logic as using NETbytes() for a write, except written to the `output` vector

	// same as queueAuto(uint32_t) - write the length
	uint32_t dataSizeU32 = static_cast<uint32_t>(data.size());
	uint32_t v = dataSizeU32;
	bool moreBytes = true;
	for (int n = 0; moreBytes; ++n)
	{
		uint8_t b;
		moreBytes = encode_uint32_t(b, v, n);
		output.push_back(b);
	}

	// write all the data bytes
	output.insert(output.end(), data.begin(), data.end());
}

ReplayOptionsHandler::~ReplayOptionsHandler() { }

// TODO Call this function somewhere.
bool NETloadReplay(std::string const &filename, ReplayOptionsHandler& optionsHandler)
{
	uint32_t replayFormatVer = 0;
	if (!NETreplayLoadStart(filename, optionsHandler, replayFormatVer))
	{
		return false;
	}
	std::unique_ptr<NetMessage> newMessage;
	uint8_t player;
	bool gotReplayEnded = false;
	while (NETreplayLoadNetMessage(newMessage, player))
	{
		if ((player >= MAX_PLAYERS && player != NetPlay.hostPlayer) || gameQueues[player] == nullptr)
		{
			debug((newMessage->type() != GAME_GAME_TIME) ? LOG_ERROR : LOG_INFO, "Skipping message to player %d in replay.", player);
			continue;
		}
		if (newMessage->type() == REPLAY_ENDED)
		{
			gotReplayEnded = true;
			break;
		}
		gameQueues[player]->pushMessage(std::move(*newMessage));
	}
	if (!gotReplayEnded && replayFormatVer >= 2)
	{
		debug(LOG_POPUP, _("Unable to load replay: The replay file is incomplete or corrupted."));
		bIsReplay = true;
		NETshutdownReplay();
		return false;
	}
	// Add special REPLAY_ENDED message to the end of the host's gameQueue
	gameQueues[NetPlay.hostPlayer]->pushMessage(NetMessageBuilder(REPLAY_ENDED, 0).build());
	NETreplayLoadStop();
	bIsReplay = true;
	return true;
}

bool NETisReplay()
{
	return bIsReplay;
}

void NETshutdownReplay()
{
	if (bIsReplay)
	{
		// extra replay spectator gamequeue
		delete gameQueues[MAX_CONNECTED_PLAYERS];
		gameQueues[MAX_CONNECTED_PLAYERS] = nullptr;
	}

	bIsReplay = false;
}

// New overloads implementation
void NETuint8_t(MessageReader& r, uint8_t& val)
{
	r.byte(val);
}

void NETint8_t(MessageReader &r, int8_t& val)
{
	NETuint8_t(r, reinterpret_cast<uint8_t&>(val));
}

void NETuint16_t(MessageReader& r, uint16_t& val)
{
	uint8_t b[2];
	r.byte(b[0]);
	r.byte(b[1]);
	wz_ntohs_load_unaligned(val, b);
}

void NETint16_t(MessageReader &r, int16_t& val)
{
	NETuint16_t(r, reinterpret_cast<uint16_t&>(val));
}

void NETuint32_t(MessageReader& r, uint32_t& val)
{
	uint32_t v = 0;
	bool moreBytes = true;
	for (size_t n = 0; moreBytes; ++n)
	{
		uint8_t b = 0;
		r.byte(b);
		moreBytes = decode_uint32_t(b, v, n);
	}
	val = v;
}

void NETint32_t(MessageReader &r, int32_t& val)
{
	// Non-negative values: value*2
	// Negative values:     -value*2 - 1
	// Example: int32_t -5 -4 -3 -2 -1  0  1  2  3  4  5
	// becomes uint32_t  9  7  5  3  1  0  2  4  6  8 10

#if defined( _MSC_VER )
	#pragma warning( push )
	#pragma warning( disable : 4146 ) // warning C4146: unary minus operator applied to unsigned type, result still unsigned
#endif

	uint32_t b = 0;
	NETuint32_t(r, b);
	val = b >> 1 ^ -(b & 1);

#if defined( _MSC_VER )
	#pragma warning( pop )
#endif
}

void NETuint64_t(MessageReader& r, uint64_t& val)
{
	uint32_t b[2];
	NETuint32_t(r, b[0]);
	NETuint32_t(r, b[1]);
	val = (uint64_t)b[0] << 32 | b[1];
}

void NETint64_t(MessageReader &r, int64_t& val)
{
	NETuint64_t(r, reinterpret_cast<uint64_t&>(val));
}

void NETbool(MessageReader &r, bool& val)
{
    uint8_t b;
	NETuint8_t(r, b);
    val = b != 0;
}

void NETwzstring(MessageReader &r, WzString &str)
{
    std::string s;
    NETstring(r, s);
    str = WzString::fromUtf8(s);
}

/** Receives a string from the current network package.
 *  \param str    The buffer to decode the string from the network package
 *                into. This string is guaranteed to be NUL-terminated
 *                provided that this buffer is at least 1 byte large.
 *  \param maxlen The buffer size of \c str. For static buffers this means
 *                sizeof(\c str), for dynamically allocated buffers this is
 *                whatever number you passed to malloc().
 *  \note If while decoding \c maxlen is smaller than the actual length of the
 *        string being decoded, the resulting string (in \c str) will be
 *        truncated.
 */
void NETstring(MessageReader &r, char *str, uint16_t maxlen)
{
    uint16_t len;
    NETuint16_t(r, len);
    len = std::min(len, maxlen);

    r.bytes((uint8_t *)str, len);
    str[len] = '\0';
}

void NETstring(MessageReader& r, std::string& s, uint32_t maxLen /* = 65536 */)
{
	uint32_t len = 0;
	NETuint32_t(r, len);
	len = std::min(len, maxLen);
	s.resize(len);
	if (r.valid())
	{
		NETbin(r, reinterpret_cast<uint8_t*>(&s[0]), len);
	}
}

void NETbin(MessageReader &r, uint8_t *str, uint32_t len)
{
    r.bytes(str, len);
}

void NETPosition(MessageReader& r, Position& pos)
{
	NETint32_t(r, pos.x);
	NETint32_t(r, pos.y);
	NETint32_t(r, pos.z);
}

void NETRotation(MessageReader& r, Rotation& rot)
{
	NETuint16_t(r, rot.direction);
	NETuint16_t(r, rot.pitch);
	NETuint16_t(r, rot.roll);
}

void NETVector2i(MessageReader& r, Vector2i& vec)
{
	NETint32_t(r, vec.x);
	NETint32_t(r, vec.y);
}

void NETnetMessage(MessageReader& r, NetMessage** msg)
{
	NetMsgDataVector rawData{MsgDataAllocator(defaultMemoryPool())};
	NETbytes(r, rawData, std::numeric_limits<uint32_t>::max());
	*msg = new NetMessage(NetMessageBuilder(std::move(rawData)).build());
}

// MessageWriter overloads for encoding
void NETuint8_t(MessageWriter& w, uint8_t val)
{
	w.byte(val);
}

void NETint8_t(MessageWriter& w, int8_t val)
{
	NETuint8_t(w, static_cast<uint8_t>(val));
}

void NETuint16_t(MessageWriter& w, uint16_t val)
{
	uint8_t b[2];
	wz_htons_store_unaligned(b, val);

	NETuint8_t(w, b[0]);
	NETuint8_t(w, b[1]);
}

void NETint16_t(MessageWriter& w, int16_t val)
{
	NETuint16_t(w, static_cast<uint16_t>(val));
}

void NETuint32_t(MessageWriter& w, uint32_t val)
{
	uint32_t v = val;
	bool moreBytes = true;
	for (int n = 0; moreBytes; ++n)
	{
		uint8_t b = 0;
		moreBytes = encode_uint32_t(b, v, n);
		NETuint8_t(w, b);
	}
}

void NETint32_t(MessageWriter& w, int32_t val)
{
	// Non-negative values: value*2
	// Negative values:     -value*2 - 1
	// Example: int32_t -5 -4 -3 -2 -1  0  1  2  3  4  5
	// becomes uint32_t  9  7  5  3  1  0  2  4  6  8 10

#if defined( _MSC_VER )
	#pragma warning( push )
	#pragma warning( disable : 4146 ) // warning C4146: unary minus operator applied to unsigned type, result still unsigned
#endif

	uint32_t b = (uint32_t)val << 1 ^ (-((uint32_t)val >> 31));
	NETuint32_t(w, b);

#if defined( _MSC_VER )
	#pragma warning( pop )
#endif
}

void NETuint64_t(MessageWriter& w, uint64_t val)
{
	uint32_t b[2] = { uint32_t(val >> 32), uint32_t(val) };
	NETuint32_t(w, b[0]);
	NETuint32_t(w, b[1]);
}

void NETint64_t(MessageWriter& w, int64_t val)
{
	NETuint64_t(w, static_cast<uint64_t>(val));
}

void NETbool(MessageWriter& w, bool val)
{
	uint8_t i = !!val;
	NETuint8_t(w, i);
}

void NETwzstring(MessageWriter& w, const WzString& str)
{
	const std::string& utf8_string = str.toUtf8();
	ASSERT(utf8_string.size() <= static_cast<size_t>(std::numeric_limits<uint16_t>::max()), "utf8_string.size() exceeds uint16_t max");
	NETstring(w, utf8_string);
}

void NETstring(MessageWriter& w, const char* str, uint16_t maxlen)
{
	/*
	 * Strings sent over the network are prefixed with their length, sent as an
	 * unsigned 16-bit integer, not including \0 termination.
	 */

	uint16_t len = 0;
	size_t cappedStrLen = strnlen1(str, maxlen);
	len = static_cast<uint16_t>((cappedStrLen > 0) ? (cappedStrLen - 1) : 0);
	NETuint16_t(w, len);

	// Truncate length if necessary
	uint16_t maxReadLen = (maxlen > 0) ? static_cast<uint16_t>(maxlen - 1) : 0;
	if (len > maxReadLen)
	{
		debug(LOG_ERROR, "NETstring: Encoding packet, length %u truncated at %u", len, maxlen);
		len = maxReadLen;
	}
	w.bytes(reinterpret_cast<const uint8_t*>(str), len);
}

void NETstring(MessageWriter& w, const std::string& s, uint32_t maxLen /* = 65536 */)
{
	uint32_t len = static_cast<uint32_t>(std::min<size_t>(s.size(), maxLen));
	NETuint32_t(w, len);
	NETbin(w, reinterpret_cast<const uint8_t*>(s.c_str()), len);
}

void NETbin(MessageWriter& w, const uint8_t* str, uint32_t len)
{
	w.bytes(str, len);
}

void NETPosition(MessageWriter& w, const Position& pos)
{
	NETint32_t(w, pos.x);
	NETint32_t(w, pos.y);
	NETint32_t(w, pos.z);
}

void NETRotation(MessageWriter& w, const Rotation& rot)
{
	NETuint16_t(w, rot.direction);
	NETuint16_t(w, rot.pitch);
	NETuint16_t(w, rot.roll);
}

void NETVector2i(MessageWriter& w, const Vector2i& vec)
{
	NETint32_t(w, vec.x);
	NETint32_t(w, vec.y);
}

void NETnetMessage(MessageWriter& w, const NetMessage& msg)
{
	NETbytes(w, msg.rawData(), std::numeric_limits<uint32_t>::max());
}
