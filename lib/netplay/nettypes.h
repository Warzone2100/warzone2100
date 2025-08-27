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
/*
 * Nettypes.h
 *
 */

#ifndef __INCLUDE_LIB_NETPLAY_NETTYPES_H__
#define __INCLUDE_LIB_NETPLAY_NETTYPES_H__

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include "lib/netplay/netqueue.h"
#include "lib/framework/wzstring.h"

#include <type_traits>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

enum QueueType
{
	QUEUE_TMP,
	QUEUE_NET,
	QUEUE_GAME,
	QUEUE_GAME_FORCED,
	QUEUE_BROADCAST,
	QUEUE_TRANSIENT_JOIN,
};

#define NET_NO_EXCLUDE 255

#define MAX_SPECTATOR_SLOTS		10
#define MAX_CONNECTED_PLAYERS   (MAX_PLAYER_SLOTS + MAX_SPECTATOR_SLOTS)
#define MAX_GAMEQUEUE_SLOTS		(MAX_CONNECTED_PLAYERS + 1) /// < +1 for the replay spectator slot
#define MAX_TMP_SOCKETS         16

NETQUEUE NETnetTmpQueue(unsigned tmpPlayer);  ///< One of the temp queues from before a client has joined the game. (See comments on tmpQueues in nettypes.cpp.)
NETQUEUE NETnetQueue(unsigned player, unsigned excludePlayer = NET_NO_EXCLUDE);  ///< The queue pair used for sending and receiving data directly from another client. (See comments on netQueues in nettypes.cpp.)
NETQUEUE NETgameQueue(unsigned player);       ///< The game action queue. (See comments on gameQueues in nettypes.cpp.)
NETQUEUE NETgameQueueForced(unsigned player); ///< Only used by the host, to force-feed a GAME_PLAYER_LEFT message into someone's game queue.
NETQUEUE NETbroadcastQueue(unsigned excludePlayer = NET_NO_EXCLUDE);  ///< The queue for sending data directly to the netQueues of all clients, not just a specific one. (See comments on broadcastQueue in nettypes.cpp.)

void NETinsertRawData(NETQUEUE queue, uint8_t *data, size_t dataLen);  ///< Dump raw data from sockets and raw data sent via host here.
void NETinsertMessageFromNet(NETQUEUE queue, NetMessage&& message);     ///< Dump whole NetMessages into the queue.
bool NETisMessageReady(NETQUEUE queue);       ///< Returns true if there is a complete message ready to deserialise in this queue.
size_t NETincompleteMessageDataBuffered(NETQUEUE queue);
NetMessage const *NETgetMessage(NETQUEUE queue);///< Returns the current message in the queue which is ready to be deserialised. Do not delete the message.

void NETinitQueue(NETQUEUE queue);             ///< Allocates the queue. Deletes the old queue, if there was one. Avoids a crash on NULL pointer deference when trying to use the queue.
void NETsetNoSendOverNetwork(NETQUEUE queue);  ///< Used to mark that a game queue should not be sent over the network (for example, if it is being sent to us, instead).
void NETmoveQueue(NETQUEUE src, NETQUEUE dst); ///< Used for moving the tmpQueue to a netQueue, once a newly-connected client is assigned a player number.
void NETswapQueues(NETQUEUE src, NETQUEUE dst); ///< Used for swapping two netQueues, when swapping a player index (i.e. player <-> spectator-only slot). Dangerous and rare.
void NETdeleteQueue();					///< Delete queues for cleanup
MessageWriter NETbeginEncode(NETQUEUE queue, uint8_t type);
MessageReader NETbeginDecode(NETQUEUE queue, uint8_t type);
void NETflushGameQueues();
void NETpop(NETQUEUE queue);

class SessionKeys;
void NETsetSessionKeys(uint8_t player, SessionKeys&& keys);
void NETclearSessionKeys();
void NETclearSessionKeys(uint8_t player);
optional<MessageWriter> NETbeginEncodeSecured(NETQUEUE queue, uint8_t type); ///< For encoding a secured net message, for a *specific player* - see .cpp file for more details
optional<MessageReader> NETbeginDecodeSecured(NETQUEUE queue, uint8_t type);
bool NETdecryptSecuredNetMessage(NETQUEUE queue, uint8_t& type);

// New overloads that accept MessageReader:
void NETuint8_t(MessageReader& r, uint8_t& val);
void NETint8_t(MessageReader &r, int8_t& val);
void NETuint16_t(MessageReader& r, uint16_t& val);
void NETint16_t(MessageReader &r, int16_t& val);
void NETuint32_t(MessageReader& r, uint32_t& val);
void NETint32_t(MessageReader &r, int32_t& val);
void NETuint64_t(MessageReader& r, uint64_t& val);
void NETint64_t(MessageReader &r, int64_t& val);
void NETbool(MessageReader &r, bool& val);
void NETwzstring(MessageReader &r, WzString &str);
void NETstring(MessageReader &r, char *str, uint16_t maxlen);
void NETstring(MessageReader &r, std::string& s, uint32_t maxLen = 65536);
void NETbin(MessageReader &r, uint8_t *str, uint32_t len);
template <typename VecT>
void NETbytes(MessageReader& r, VecT& vec, unsigned maxLen = 10000)
{
	/*
	 * Strings sent over the network are prefixed with their length, sent as an
	 * unsigned 16-bit integer, not including \0 termination.
	 */
	static_assert(std::is_same<typename VecT::value_type, uint8_t>::value, "Expected vector<uint8_t> as the argument");

	uint32_t len = 0;
	NETuint32_t(r, len);
	if (len > maxLen)
	{
		debug(LOG_ERROR, "NETbytes: Decoding packet, length %u truncated at %u", len, maxLen);
	}
	len = std::min<unsigned>(len, maxLen);  // Truncate length if necessary.
	vec.clear();
	if (r.valid())
	{
		r.bytesVector(vec, len);
	}
}
void NETPosition(MessageReader& r, Position& pos);
void NETRotation(MessageReader& r, Rotation& rot);
void NETVector2i(MessageReader& r, Vector2i& vec);
void NETnetMessage(MessageReader& r, NetMessage** msg);  ///< Must delete the NETMESSAGE.

template <typename EnumT>
void NETenum(MessageReader& r, EnumT& enumRef)
{
	static_assert(std::is_enum<EnumT>::value, "Expected enumeration type as the argument");

	uint32_t val = 0;
	NETuint32_t(r, val);
	enumRef = static_cast<EnumT>(val);
}

bool NETend(MessageReader& r);

// New overloads that accept MessageWriter:
void NETuint8_t(MessageWriter& w, uint8_t val);
void NETint8_t(MessageWriter& w, int8_t val);
void NETuint16_t(MessageWriter& w, uint16_t val);
void NETint16_t(MessageWriter& w, int16_t val);
void NETuint32_t(MessageWriter& w, uint32_t val);       ///< Encodes small values (< 1 672 576) in at most 3 bytes, large values (≥ 45 776 896) in 5 bytes.
void NETint32_t(MessageWriter& w, int32_t val);         ///< Encodes small values (< 836 288) in at most 3 bytes, large values (≥ 22 888 448) in 5 bytes.
void NETuint64_t(MessageWriter& w, uint64_t val);
void NETint64_t(MessageWriter& w, int64_t val);
void NETbool(MessageWriter& w, bool val);
void NETwzstring(MessageWriter& w, const WzString& str);
void NETstring(MessageWriter& w, const char* str, uint16_t maxlen);
void NETstring(MessageWriter& w, const std::string& s, uint32_t maxLen = 65536);
void NETbin(MessageWriter& w, const uint8_t* str, uint32_t len);
template <typename VecT>
void NETbytes(MessageWriter& w, const VecT& vec, unsigned maxLen = 10000)
{
	/*
	 * Strings sent over the network are prefixed with their length, sent as an
	 * unsigned 16-bit integer, not including \0 termination.
	 */

	static_assert(std::is_same<typename VecT::value_type, uint8_t>::value, "Expected vector<uint8_t> as the argument");

	ASSERT(vec.size() <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "vec.size() exceeds uint32_t max");
	uint32_t len = static_cast<uint32_t>(std::min(vec.size(), static_cast<size_t>(std::numeric_limits<uint32_t>::max())));
	if (len > maxLen)
	{
		debug(LOG_ERROR, "NETbytes: Encoding packet, length %u truncated at %u", len, maxLen);
	}
	len = std::min<unsigned>(len, maxLen);  // Truncate length if necessary.
	NETuint32_t(w, len);
	w.bytes(vec.data(), len);
}
void NETPosition(MessageWriter& w, const Position& pos);
void NETRotation(MessageWriter& w, const Rotation& rot);
void NETVector2i(MessageWriter& w, const Vector2i& vec);
void NETnetMessage(MessageWriter& w, const NetMessage& msg);

template <typename EnumT>
static void NETenum(MessageWriter& w, EnumT val)
{
	static_assert(std::is_enum<EnumT>::value, "Expected enumeration type as the argument");

	NETuint32_t(w, static_cast<uint32_t>(val));
}

bool NETend(MessageWriter& w);

void NETbytesOutputToVector(const std::vector<uint8_t> &data, std::vector<uint8_t>& output);

#include <nlohmann/json_fwd.hpp>

class ReplayOptionsHandler
{
public:
	struct EmbeddedMapData
	{
		uint32_t dataVersion = 0;
		std::vector<uint8_t> mapBinaryData;
	};
public:
	virtual ~ReplayOptionsHandler();
public:
	virtual bool saveOptions(nlohmann::json& object) const = 0;
	virtual bool saveMap(EmbeddedMapData& mapData) const = 0;
	virtual bool optionsUpdatePlayerInfo(nlohmann::json& object) const = 0;
	virtual bool restoreOptions(const nlohmann::json& object, EmbeddedMapData&& embeddedMapData, uint32_t replay_netcodeMajor, uint32_t replay_netcodeMinor) = 0;
	virtual size_t desiredBufferSize() const = 0;
	virtual size_t maximumEmbeddedMapBufferSize() const = 0;
};

bool NETloadReplay(std::string const &filename, ReplayOptionsHandler& optionsHandler);
bool NETisReplay();
void NETshutdownReplay();

bool NETgameIsBehindPlayersByAtLeast(size_t numGameTimeUpdates = 2);

#endif
