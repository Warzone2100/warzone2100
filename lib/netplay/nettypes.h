/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2015  Warzone 2100 Project

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

class QString;

enum PACKETDIR
{
	PACKET_ENCODE,
	PACKET_DECODE,
	PACKET_INVALID
};

enum QueueType
{
	QUEUE_TMP,
	QUEUE_NET,
	QUEUE_GAME,
	QUEUE_GAME_FORCED,
	QUEUE_BROADCAST,
};

struct NETQUEUE
{
	void *queue;  ///< Is either a (NetQueuePair **) or a (NetQueue *). (Note different numbers of *.)
	bool isPair;
	uint8_t index;
	uint8_t queueType;
	uint8_t exclude;
};

#define NET_NO_EXCLUDE 255

NETQUEUE NETnetTmpQueue(unsigned tmpPlayer);  ///< One of the temp queues from before a client has joined the game. (See comments on tmpQueues in nettypes.cpp.)
NETQUEUE NETnetQueue(unsigned player, unsigned excludePlayer = NET_NO_EXCLUDE);  ///< The queue pair used for sending and receiving data directly from another client. (See comments on netQueues in nettypes.cpp.)
NETQUEUE NETgameQueue(unsigned player);       ///< The game action queue. (See comments on gameQueues in nettypes.cpp.)
NETQUEUE NETgameQueueForced(unsigned player); ///< Only used by the host, to force-feed a GAME_PLAYER_LEFT message into someone's game queue.
NETQUEUE NETbroadcastQueue(unsigned excludePlayer = NET_NO_EXCLUDE);  ///< The queue for sending data directly to the netQueues of all clients, not just a specific one. (See comments on broadcastQueue in nettypes.cpp.)

void NETinsertRawData(NETQUEUE queue, uint8_t *data, size_t dataLen);  ///< Dump raw data from sockets and raw data sent via host here.
void NETinsertMessageFromNet(NETQUEUE queue, NetMessage const *message);     ///< Dump whole NetMessages into the queue.
bool NETisMessageReady(NETQUEUE queue);       ///< Returns true if there is a complete message ready to deserialise in this queue.
NetMessage const *NETgetMessage(NETQUEUE queue);///< Returns the current message in the queue which is ready to be deserialised. Do not delete the message.

void NETinitQueue(NETQUEUE queue);             ///< Allocates the queue. Deletes the old queue, if there was one. Avoids a crash on NULL pointer deference when trying to use the queue.
void NETsetNoSendOverNetwork(NETQUEUE queue);  ///< Used to mark that a game queue should not be sent over the network (for example, if it is being sent to us, instead).
void NETmoveQueue(NETQUEUE src, NETQUEUE dst); ///< Used for moving the tmpQueue to a netQueue, once a newly-connected client is assigned a player number.
void NETdeleteQueue(void);					///< Delete queues for cleanup
void NETbeginEncode(NETQUEUE queue, uint8_t type);
void NETbeginDecode(NETQUEUE queue, uint8_t type);
bool NETend(void);
void NETflushGameQueues(void);
void NETpop(NETQUEUE queue);

void NETint8_t(int8_t *ip);
void NETuint8_t(uint8_t *ip);
void NETint16_t(int16_t *ip);
void NETuint16_t(uint16_t *ip);
void NETint32_t(int32_t *ip);         ///< Encodes small values (< 836 288) in at most 3 bytes, large values (≥ 22 888 448) in 5 bytes.
void NETuint32_t(uint32_t *ip);       ///< Encodes small values (< 1 672 576) in at most 3 bytes, large values (≥ 45 776 896) in 5 bytes.
void NETint64_t(int64_t *ip);
void NETuint64_t(uint64_t *ip);
void NETbool(bool *bp);
void NETbool(bool *bp);
void NETqstring(QString &str);
void NETstring(char *str, uint16_t maxlen);
void NETstring(char const *str, uint16_t maxlen);  ///< Encode-only version of NETstring.
void NETbin(uint8_t *str, uint32_t len);
void NETbytes(std::vector<uint8_t> *vec, unsigned maxLen = 10000);

PACKETDIR NETgetPacketDir(void);

template <typename EnumT>
static void NETenum(EnumT *enumPtr)
{
	uint32_t val;

	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		val = *enumPtr;
	}

	NETuint32_t(&val);

	if (NETgetPacketDir() == PACKET_DECODE)
	{
		*enumPtr = static_cast<EnumT>(val);
	}
}

void NETPosition(Position *vp);
void NETRotation(Rotation *vp);
void NETVector2i(Vector2i *vp);

static inline void NETauto(int8_t *ip)
{
	NETint8_t(ip);
}
static inline void NETauto(uint8_t *ip)
{
	NETuint8_t(ip);
}
static inline void NETauto(int16_t *ip)
{
	NETint16_t(ip);
}
static inline void NETauto(uint16_t *ip)
{
	NETuint16_t(ip);
}
static inline void NETauto(int32_t *ip)
{
	NETint32_t(ip);
}
static inline void NETauto(uint32_t *ip)
{
	NETuint32_t(ip);
}
static inline void NETauto(int64_t *ip)
{
	NETint64_t(ip);
}
static inline void NETauto(uint64_t *ip)
{
	NETuint64_t(ip);
}
static inline void NETauto(bool *bp)
{
	NETbool(bp);
}
static inline void NETauto(Position *vp)
{
	NETPosition(vp);
}
static inline void NETauto(Rotation *vp)
{
	NETRotation(vp);
}
static inline void NETauto(Vector2i *vp)
{
	NETVector2i(vp);
}


void NETnetMessage(NetMessage const **message);  ///< If decoding, must delete the NETMESSAGE.

#endif
