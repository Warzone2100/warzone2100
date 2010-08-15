/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2010  Warzone 2100 Project

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

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

typedef enum packetDirectionEnum
{
	PACKET_ENCODE,
	PACKET_DECODE,
	PACKET_INVALID
} PACKETDIR;

typedef enum QueueType
{
	QUEUE_TMP,
	QUEUE_NET,
	QUEUE_GAME,
	QUEUE_BROADCAST,
} QUEUETYPE;

typedef struct _netqueue
{
	void *queue;  ///< Is either a (NetQueuePair **) or a (NetQueue *). (Note different numbers of *.)
	BOOL isPair;
	uint8_t index;
	uint8_t queueType;
} NETQUEUE;

typedef const struct NetMessage *NETMESSAGE;

NETQUEUE NETnetTmpQueue(unsigned tmpPlayer);  ///< One of the temp queues from before a client has joined the game. (See comments on tmpQueues in nettypes.cpp.)
NETQUEUE NETnetQueue(unsigned player);        ///< The queue pair used for sending and receiving data directly from another client. (See comments on netQueues in nettypes.cpp.)
NETQUEUE NETgameQueue(unsigned player);       ///< The game action queue. (See comments on gameQueues in nettypes.cpp.)
NETQUEUE NETbroadcastQueue(void);             ///< The queue for sending data directly to the netQueues of all clients, not just a specific one. (See comments on broadcastQueue in nettypes.cpp.)

void NETinsertRawData(NETQUEUE queue, uint8_t *data, size_t dataLen);  ///< Dump raw data from sockets and raw data sent via host here.
void NETinsertMessageFromNet(NETQUEUE queue, NETMESSAGE message);      ///< Dump whole NetMessages into the queue.
BOOL NETisMessageReady(NETQUEUE queue);       ///< Returns true if there is a complete message ready to deserialise in this queue.
NETMESSAGE NETgetMessage(NETQUEUE queue);     ///< Returns the current message in the queue which is ready to be deserialised. Do not delete the message.
uint8_t NETmessageType(NETMESSAGE message);   ///< Returns the type of the message.
uint32_t NETmessageSize(NETMESSAGE message);  ///< Returns the size of the message data.
uint8_t *NETmessageRawData(NETMESSAGE message);///<Returns the raw data, must be deleted again with NETmessageDestroyRawData().
void NETmessageDestroyRawData(uint8_t *data); ///< Destroys the data returned by NETmessageRawData().
size_t NETmessageRawSize(NETMESSAGE message); ///< Returns the size of the message, including headers.

void NETinitQueue(NETQUEUE queue);             ///< Allocates the queue. Deletes the old queue, if there was one. Avoids a crash on NULL pointer deference when trying to use the queue.
void NETsetNoSendOverNetwork(NETQUEUE queue);  ///< Used to mark that a game queue should not be sent over the network (for example, if it is being sent to us, instead).
void NETmoveQueue(NETQUEUE src, NETQUEUE dst); ///< Used for moving the tmpQueue to a netQueue, once a newly-connected client is assigned a player number.

void NETbeginEncode(NETQUEUE queue, uint8_t type);
void NETbeginDecode(NETQUEUE queue, uint8_t type);
BOOL NETend(void);
void NETflushGameQueues(void);
void NETpop(NETQUEUE queue);

void NETint8_t(int8_t *ip);
void NETuint8_t(uint8_t *ip);
void NETint16_t(int16_t *ip);
void NETuint16_t(uint16_t *ip);
void NETint32_t(int32_t *ip);         ///< Encodes small values (< 836 288) in at most 3 bytes, large values (≥ 22 888 448) in 5 bytes.
void NETuint32_t(uint32_t *ip);       ///< Encodes small values (< 1 672 576) in at most 3 bytes, large values (≥ 45 776 896) in 5 bytes.
void NETuint32_tLarge(uint32_t *ip);  ///< Encodes all values in exactly 4 bytes.
void NETint64_t(int64_t *ip);
void NETuint64_t(uint64_t *ip);
void NETfloat(float* fp);
void NETbool(BOOL *bp);
void NETnull(void);
void NETstring(char *str, uint16_t maxlen);
void NETbin(uint8_t *str, uint32_t maxlen);

PACKETDIR NETgetPacketDir(void);

#if defined(__cplusplus)
}

template <typename EnumT>
static void NETenum(EnumT* enumPtr)
{
	uint32_t val;
	
	if (NETgetPacketDir() == PACKET_ENCODE)
		val = *enumPtr;

	NETuint32_t(&val);

	if (NETgetPacketDir() == PACKET_DECODE)
		*enumPtr = static_cast<EnumT>(val);
}

extern "C"
{
#else
// FIXME: Somehow still causes tons of warnings: <enumPtr> is used unitialised in this function
static inline void squelchUninitialisedUseWarning(void *ptr) { (void)ptr; }
#define NETenum(enumPtr) \
do \
{ \
	uint32_t _val; \
	squelchUninitialisedUseWarning(enumPtr); \
	_val = (NETgetPacketDir() == PACKET_ENCODE) ? *(enumPtr) : 0; \
\
	NETuint32_t(&_val); \
\
	*(enumPtr) = _val; \
} while(0)
#endif

void NETPosition(Position *vp);
void NETRotation(Rotation *vp);

typedef struct PackagedCheck
{
	uint32_t gameTime;  ///< Game time that this synch check was made. Not touched by NETPACKAGED_CHECK().
	uint8_t player;
	uint32_t droidID;
	int32_t order;
	uint32_t secondaryOrder;
	uint32_t body;
	float experience;
	Position pos;
	Rotation rot;
	float sMoveX;
	float sMoveY;
	uint32_t targetID;  ///< Defined iff order == DORDER_ATTACK.
	uint16_t orderX;    ///< Defined iff order == DORDER_MOVE.
	uint16_t orderY;    ///< Defined iff order == DORDER_MOVE.
} PACKAGED_CHECK;
void NETPACKAGED_CHECK(PACKAGED_CHECK *v);
void NETNETMESSAGE(NETMESSAGE *message);  ///< If decoding, must destroy the NETMESSAGE.
void NETdestroyNETMESSAGE(NETMESSAGE message);

void NETtest(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif
