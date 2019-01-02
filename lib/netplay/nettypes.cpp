/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2019  Warzone 2100 Project

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
#include <string.h>
#include <QtCore/QStringList>

#ifndef WZ_OS_WIN
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

#include "../framework/frame.h"
#include "netplay.h"
#include "nettypes.h"
#include "netqueue.h"
#include "netlog.h"
#include "src/order.h"
#include <cstring>

/// There is a game queue representing each player. The game queues are synchronised among all players, so that all players process the same game queue
/// messages at the same game time. The game queues should be used, even in single-player. Players should write to their own queue, not to other player's
/// queues, and should read messages from all queues including their own.
static NetQueue *gameQueues[MAX_PLAYERS] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

/// There is a bidirectional net queue for communicating with each client or host. Each queue corresponds either to a real socket, or a virtual socket
/// which routes via the host.
static NetQueuePair *netQueues[MAX_CONNECTED_PLAYERS] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

/// These queues are for clients which just connected, but haven't yet been assigned a player number.
static NetQueuePair *tmpQueues[MAX_TMP_SOCKETS] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

/// Sending a message to the broadcast queue is equivalent to sending the message to the net queues of all other players.
static NetQueue *broadcastQueue = nullptr;

// Only used between NETbegin{Encode,Decode} and NETend calls.
static MessageWriter writer;  ///< Used when serialising a message.
static MessageReader reader;  ///< Used when deserialising a message.
static NetMessage message;    ///< A message which is being serialised or deserialised.
static NETQUEUE queueInfo;    ///< Indicates which queue is currently being (de)serialised.
static PACKETDIR NetDir;      ///< Indicates whether a message is being serialised (PACKET_ENCODE) or deserialised (PACKET_DECODE), or not doing anything (PACKET_INVALID).

static void NETsetPacketDir(PACKETDIR dir)
{
	NetDir = dir;

	// Can't put STATIC_ASSERT in global scope, arbitrarily putting it here.
	STATIC_ASSERT(MAX_PLAYERS == MAX_CONNECTED_PLAYERS);  // Things might break if each connected player doesn't correspond to a player of the same index.
}

PACKETDIR NETgetPacketDir()
{
	return NetDir;
}

// The queue(q, v) functions (de)serialise the object v to/from q, depending on whether q is a MessageWriter or MessageReader.

template<class Q>
static void queue(const Q &q, uint8_t &v)
{
	q.byte(v);
}

template<class Q>
static void queue(const Q &q, uint16_t &v)
{
	uint8_t b[2] = {uint8_t(v >> 8), uint8_t(v)};
	queue(q, b[0]);
	queue(q, b[1]);
	if (Q::Direction == Q::Read)
	{
		v = b[0] << 8 | b[1];
	}
}

template<class Q>
static void queue(const Q &q, uint32_t &vOrig)
{
	if (Q::Direction == Q::Write)
	{
		uint32_t v = vOrig;
		bool moreBytes = true;
		for (int n = 0; moreBytes; ++n)
		{
			uint8_t b;
			moreBytes = encode_uint32_t(b, v, n);
			queue(q, b);
		}
	}
	else if (Q::Direction == Q::Read)
	{
		uint32_t v = 0;
		bool moreBytes = true;
		for (int n = 0; moreBytes; ++n)
		{
			uint8_t b = 0;
			queue(q, b);
			moreBytes = decode_uint32_t(b, v, n);
		}

		vOrig = v;
	}
}

template<class Q>
static void queue(const Q &q, uint64_t &v)
{
	uint32_t b[2] = {uint32_t(v >> 32), uint32_t(v)};
	queue(q, b[0]);
	queue(q, b[1]);
	if (Q::Direction == Q::Read)
	{
		v = uint64_t(b[0]) << 32 | b[1];
	}
}

template<class Q>
static void queue(const Q &q, char &v)
{
	uint8_t b = v;
	queue(q, b);
	if (Q::Direction == Q::Read)
	{
		v = b;
	}

	STATIC_ASSERT(sizeof(b) == sizeof(v));
}

template<class Q>
static void queue(const Q &q, int8_t &v)
{
	uint8_t b = v;
	queue(q, b);
	if (Q::Direction == Q::Read)
	{
		v = b;
	}

	STATIC_ASSERT(sizeof(b) == sizeof(v));
}

template<class Q>
static void queue(const Q &q, int16_t &v)
{
	uint16_t b = v;
	queue(q, b);
	if (Q::Direction == Q::Read)
	{
		v = b;
	}

	STATIC_ASSERT(sizeof(b) == sizeof(v));
}

template<class Q>
static void queue(const Q &q, int32_t &v)
{
	// Non-negative values: value*2
	// Negative values:     -value*2 - 1
	// Example: int32_t -5 -4 -3 -2 -1  0  1  2  3  4  5
	// becomes uint32_t  9  7  5  3  1  0  2  4  6  8 10

#if defined( _MSC_VER )
	#pragma warning( push )
	#pragma warning( disable : 4146 ) // warning C4146: unary minus operator applied to unsigned type, result still unsigned
#endif

	uint32_t b = (uint32_t)v << 1 ^ (-((uint32_t)v >> 31));
	queue(q, b);
	if (Q::Direction == Q::Read)
	{
		v = b >> 1 ^ -(b & 1);
	}

#if defined( _MSC_VER )
	#pragma warning( pop )
#endif

	STATIC_ASSERT(sizeof(b) == sizeof(v));
}

template<class Q>
static void queue(const Q &q, int64_t &v)
{
	uint64_t b = v;
	queue(q, b);
	if (Q::Direction == Q::Read)
	{
		v = b;
	}

	STATIC_ASSERT(sizeof(b) == sizeof(v));
}

template<class Q>
static void queue(const Q &q, Position &v)
{
	queue(q, v.x);
	queue(q, v.y);
	queue(q, v.z);
}

template<class Q>
static void queue(const Q &q, Rotation &v)
{
	queue(q, v.direction);
	queue(q, v.pitch);
	queue(q, v.roll);
}

template<class Q>
static void queue(const Q &q, Vector2i &v)
{
	queue(q, v.x);
	queue(q, v.y);
}

template<class Q, class T>
static void queue(const Q &q, std::vector<T> &v)
{
	uint32_t len = v.size();
	queue(q, len);
	switch (Q::Direction)
	{
	case Q::Write:
		for (unsigned i = 0; i != len; ++i)
		{
			queue(q, v[i]);
		}
		break;
	case Q::Read:
		v.clear();
		for (unsigned i = 0; i != len && q.valid(); ++i)
		{
			T tmp;
			queue(q, tmp);
			v.push_back(tmp);
		}
		break;
	}
}

template<class Q>
static void queue(const Q &q, NetMessage &v)
{
	queue(q, v.type);
	queue(q, v.data);
}

template<class T>
static void queueAuto(T &v)
{
	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		queue(writer, v);
	}
	else if (NETgetPacketDir() == PACKET_DECODE)
	{
		queue(reader, v);
	}
}

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
	ASSERT(player < MAX_PLAYERS, "Huh?");
	NetQueue *queue = gameQueues[player];
	ret.queue = queue;
	ret.isPair = false;
	ret.index = player;
	ret.queueType = QUEUE_GAME;
	ret.exclude = NET_NO_EXCLUDE;
	return ret;
}

NETQUEUE NETgameQueueForced(unsigned player)
{
	NETQUEUE ret;
	ASSERT(player < MAX_PLAYERS, "Huh?");
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

void NETinsertMessageFromNet(NETQUEUE queue, NetMessage const *message)
{
	receiveQueue(queue)->pushMessage(*message);
}

bool NETisMessageReady(NETQUEUE queue)
{
	return receiveQueue(queue)->haveMessage();
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
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		delete pairQueue(NETnetQueue(i));
		pairQueue(NETnetQueue(i)) = nullptr;
		delete gameQueues[i];
		gameQueues[i] = nullptr;
	}

	delete broadcastQueue;
	broadcastQueue = nullptr;
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

void NETbeginEncode(NETQUEUE queue, uint8_t type)
{
	NETsetPacketDir(PACKET_ENCODE);

	queueInfo = queue;
	message = type;
	writer = MessageWriter(message);
}

void NETbeginDecode(NETQUEUE queue, uint8_t type)
{
	NETsetPacketDir(PACKET_DECODE);

	queueInfo = queue;
	message = receiveQueue(queueInfo)->getMessage();
	reader = MessageReader(message);

	assert(type == message.type);
}

bool NETend()
{
	// If we are encoding just return true
	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		// Push the message onto the list.
		NetQueue *queue = sendQueue(queueInfo);
		if (queue == nullptr) {
			debug(LOG_WARNING, "Sending %s to null queue, type %d.", messageTypeToString(message.type), queueInfo.queueType);
			return true;
		}
		queue->pushMessage(message);
		NETlogPacket(message.type, message.data.size(), false);

		if (queueInfo.queueType == QUEUE_GAME || queueInfo.queueType == QUEUE_GAME_FORCED)
		{
			ASSERT(message.type > GAME_MIN_TYPE && message.type < GAME_MAX_TYPE, "Inserting %s into game queue.", messageTypeToString(message.type));
		}
		else
		{
			ASSERT(message.type > NET_MIN_TYPE && message.type < NET_MAX_TYPE, "Inserting %s into net queue.", messageTypeToString(message.type));
		}

		if (queueInfo.queueType == QUEUE_NET || queueInfo.queueType == QUEUE_BROADCAST || queueInfo.queueType == QUEUE_TMP)
		{
			NETsend(queueInfo, &queue->getMessageForNet());
			queue->popMessageForNet();
			ASSERT(queue->numMessagesForNet() == 0, "Queue not empty.");
		}

		// We have ended the serialisation, so mark the direction invalid
		NETsetPacketDir(PACKET_INVALID);

		if (queueInfo.queueType == QUEUE_GAME_FORCED)  // If true, we must be the host, inserting a GAME_PLAYER_LEFT into the other player's game queue. Since they left, they're not around to complain about us messing with their queue, which would normally cause a desynch.
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
			uint8_t player = queueInfo.index;
			uint32_t num = 1;
			NetMessage backupMessage = message;  // 'message' will be overwritten, so we need a copy (to avoid trying to insert a message into itself).
			NETbeginEncode(NETbroadcastQueue(), NET_SHARE_GAME_QUEUE);
			NETuint8_t(&player);
			NETuint32_t(&num);
			for (uint32_t n = 0; n < num; ++n)
			{
				queueAuto(backupMessage);
			}
			NETsetPacketDir(PACKET_INVALID);  // Instead of NETend();
			backupMessage = message;  // 'message' will be overwritten again, so we need a copy of this NET_SHARE_GAME_QUEUE message.
			uint8_t allPlayers = NET_ALL_PLAYERS;
			NETbeginEncode(NETbroadcastQueue(), NET_SEND_TO_PLAYER);
			NETuint8_t(&player);
			NETuint8_t(&allPlayers);
			queueAuto(backupMessage);
			NETend();  // This time we actually send it.
		}

		return true;  // Serialising never fails.
	}

	if (NETgetPacketDir() == PACKET_DECODE)
	{
		bool ret = reader.valid();

		// We have ended the deserialisation, so mark the direction invalid
		NETsetPacketDir(PACKET_INVALID);

		return ret;
	}

	assert(false && false && false);
	return false;
}

void NETflushGameQueues()
{
	for (uint8_t player = 0; player < MAX_PLAYERS; ++player)
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

		// Decoded in NETprocessSystemMessage in netplay.cpp.
		NETbeginEncode(NETbroadcastQueue(), NET_SHARE_GAME_QUEUE);
		NETuint8_t(&player);
		NETuint32_t(&num);
		for (uint32_t n = 0; n < num; ++n)
		{
			queueAuto(const_cast<NetMessage &>(queue->getMessageForNet()));  // const_cast is safe since we are encoding, not decoding.
			queue->popMessageForNet();
		}
		NETend();
	}
}

void NETpop(NETQUEUE queue)
{
	receiveQueue(queue)->popMessage();
}

void NETint8_t(int8_t *ip)
{
	queueAuto(*ip);
}

void NETuint8_t(uint8_t *ip)
{
	queueAuto(*ip);
}

void NETint16_t(int16_t *ip)
{
	queueAuto(*ip);
}

void NETuint16_t(uint16_t *ip)
{
	queueAuto(*ip);
}

void NETint32_t(int32_t *ip)
{
	queueAuto(*ip);
}

void NETuint32_t(uint32_t *ip)
{
	queueAuto(*ip);
}

void NETint64_t(int64_t *ip)
{
	queueAuto(*ip);
}

void NETuint64_t(uint64_t *ip)
{
	queueAuto(*ip);
}

void NETbool(bool *bp)
{
	uint8_t i = !!*bp;
	queueAuto(i);
	*bp = !!i;
}

/** Sends or receives a string to or from the current network package.
 *  \param str    When encoding a packet this is the (NUL-terminated string to
 *                be sent in the current network package. When decoding this
 *                is the buffer to decode the string from the network package
 *                into. When decoding this string is guaranteed to be
 *                NUL-terminated provided that this buffer is at least 1 byte
 *                large.
 *  \param maxlen The buffer size of \c str. For static buffers this means
 *                sizeof(\c str), for dynamically allocated buffers this is
 *                whatever number you passed to malloc().
 *  \note If while decoding \c maxlen is smaller than the actual length of the
 *        string being decoded, the resulting string (in \c str) will be
 *        truncated.
 */
void NETstring(char *str, uint16_t maxlen)
{
	/*
	 * Strings sent over the network are prefixed with their length, sent as an
	 * unsigned 16-bit integer, not including \0 termination.
	 */

	uint16_t len = (NETgetPacketDir() == PACKET_ENCODE) ? ((uint16_t)strnlen1(str, maxlen)) - 1 : 0;
	queueAuto(len);

	// Truncate length if necessary
	if (len > maxlen - 1)
	{
		debug(LOG_ERROR, "NETstring: %s packet, length %u truncated at %u", NETgetPacketDir() == PACKET_ENCODE ? "Encoding" : "Decoding", len, maxlen);
		len = maxlen - 1;
	}

	for (unsigned i = 0; i < len; ++i)
	{
		queueAuto(str[i]);
	}

	if (NETgetPacketDir() == PACKET_DECODE)
	{
		// NUL-terminate
		str[len] = '\0';
	}
}

void NETwzstring(WzString &str)
{
	// NOTE: To be backwards-compatible with the old NETqstring (QString-based) function,
	// this uses UTF-16 encoding.

	std::vector<uint16_t> u16_characters;
	uint32_t len = 0;
	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		u16_characters = str.toUtf16();
		len = u16_characters.size();
	}

	queueAuto(len);

	if (NETgetPacketDir() == PACKET_DECODE)
	{
		u16_characters.resize(len);
	}
	for (unsigned i = 0; i < len; ++i)
	{
		uint16_t c = 0;
		if (NETgetPacketDir() == PACKET_ENCODE)
		{
			c = u16_characters[i];
		}
		queueAuto(c);
		if (NETgetPacketDir() == PACKET_DECODE)
		{
			u16_characters[i] = c;
		}
	}

	if (NETgetPacketDir() == PACKET_DECODE)
	{
		str = WzString::fromUtf16(u16_characters);
	}
}

void NETstring(char const *str, uint16_t maxlen)
{
	ASSERT(NETgetPacketDir() == PACKET_ENCODE, "Writing to const!");
	NETstring(const_cast<char *>(str), maxlen);
}

void NETbytes(std::vector<uint8_t> *vec, unsigned maxLen)
{
	/*
	 * Strings sent over the network are prefixed with their length, sent as an
	 * unsigned 16-bit integer, not including \0 termination.
	 */

	uint32_t len = NETgetPacketDir() == PACKET_ENCODE ? vec->size() : 0;
	queueAuto(len);

	if (len > maxLen)
	{
		debug(LOG_ERROR, "NETstring: %s packet, length %u truncated at %u", NETgetPacketDir() == PACKET_ENCODE ? "Encoding" : "Decoding", len, maxLen);
	}

	len = std::min<unsigned>(len, maxLen);  // Truncate length if necessary.
	if (NETgetPacketDir() == PACKET_DECODE)
	{
		vec->clear();
		vec->resize(len);  // vec->assign(len, 0) would call the wrong version of assign, here.
	}

	for (unsigned i = 0; i < len; ++i)
	{
		queueAuto((*vec)[i]);
	}
}

void NETbin(uint8_t *str, uint32_t len)
{
	for (unsigned i = 0; i < len; ++i)
	{
		queueAuto(str[i]);
	}
}

void NETPosition(Position *vp)
{
	queueAuto(*vp);
}

void NETRotation(Rotation *vp)
{
	queueAuto(*vp);
}

void NETVector2i(Vector2i *vp)
{
	queueAuto(*vp);
}

void NETnetMessage(NetMessage const **message)
{
	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		queueAuto(*const_cast<NetMessage *>(*message));  // Const cast safe when encoding.
	}

	if (NETgetPacketDir() == PACKET_DECODE)
	{
		NetMessage *m = new NetMessage;
		queueAuto(*m);
		*message = m;
		return;
	}
}
