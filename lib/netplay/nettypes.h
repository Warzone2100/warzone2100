/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2009  Warzone Resurrection Project

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
#include "netplay.h"
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

NETQUEUE NETnetTmpQueue(unsigned tmpPlayer);  ///< One of the temp queues from before a client has joined the game. (See comments on tmpQueues in nettypes.cpp.)
NETQUEUE NETnetQueue(unsigned player);        ///< The queue pair used for sending and receiving data directly from another client. (See comments on netQueues in nettypes.cpp.)
NETQUEUE NETgameQueue(unsigned player);       ///< The game action queue. (See comments on gameQueues in nettypes.cpp.)
NETQUEUE NETbroadcastQueue(void);             ///< The queue for sending data directly to the netQueues of all clients, not just a specific one. (See comments on broadcastQueue in nettypes.cpp.)

void NETinsertRawData(NETQUEUE queue, uint8_t *data, size_t dataLen);  /// Dump raw data from sockets and raw data sent via host here.
BOOL NETisMessageReady(NETQUEUE queue);       ///< Returns true if there is a complete message ready to deserialise in this queue.
uint8_t NETmessageType(NETQUEUE queue);       ///< Returns the type of the message in this queue which is ready to be deserialised.
uint32_t NETmessageSize(NETQUEUE queue);      ///< Returns the size of the message data in this queue which is ready to be deserialised.

void NETinitQueue(NETQUEUE queue);             ///< Allocates the queue. Deletes the old queue, if there was one. Avoids a crash on NULL pointer deference when trying to use the queue.
void NETsetNoSendOverNetwork(NETQUEUE queue);  ///< Used to mark that a game queue should not be sent over the network (for example, if it is being sent to us, instead).
void NETmoveQueue(NETQUEUE src, NETQUEUE dst); ///< Used for moving the tmpQueue to a netQueue, once a newly-connected client is assigned a player number.

void NETbeginEncode(NETQUEUE queue, uint8_t type);
void NETbeginDecode(NETQUEUE queue, uint8_t type);
BOOL NETend(void);
void NETpop(NETQUEUE queue);

void NETint8_t(int8_t *ip);
void NETuint8_t(uint8_t *ip);
void NETint16_t(int16_t *ip);
void NETuint16_t(uint16_t *ip);
void NETint32_t(int32_t *ip);
void NETuint32_t(uint32_t *ip);
void NETfloat(float* fp);
void NETbool(BOOL *bp);
void NETnull(void);
void NETstring(char *str, uint16_t maxlen);
void NETbin(uint8_t *str, uint16_t maxlen);

PACKETDIR NETgetPacketDir(void);

#if defined(__cplusplus)
}

template <typename EnumT>
static BOOL NETenum(EnumT* enumPtr)
{
	int32_t val;
	
	if (NETgetPacketDir() == PACKET_ENCODE)
		val = *enumPtr;

	const BOOL retVal = NETint32_t(&val);

	if (NETgetPacketDir() == PACKET_DECODE)
		*enumPtr = static_cast<EnumT>(val);

	return retVal;
}

extern "C"
{
#else
// FIXME: Causes tons of warnings: <enumPtr> is used unitialised in this function
#define NETenum(enumPtr) \
do \
{ \
	int32_t _val = (NETgetPacketDir() == PACKET_ENCODE) ? *(enumPtr) : 0; \
\
	NETint32_t(&_val); \
\
	*(enumPtr) = _val; \
} while(0)
#endif

void NETVector3uw(Vector3uw* vp);

void NETtest(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif
