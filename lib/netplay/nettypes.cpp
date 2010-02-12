/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2010  Warzone Resurrection Project

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

#include <SDL_endian.h>

#include "../framework/frame.h"
#include "netplay.h"
#include "nettypes.h"
#include "netqueue.h"
#include <cstring>

/// There is a game queue representing each player. The game queues are synchronised among all players, so that all players process the same game queue
/// messages at the same game time. The game queues should be used, even in single-player. Players should write to their own queue, not to other player's
/// queues, and should read messages from all queues including their own.
static NetQueue *gameQueues[MAX_PLAYERS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/// There is a bidirectional net queue for communicating with each client or host. Each queue corresponds either to a real socket, or a virtual socket
/// which routes via the host.
static NetQueuePair *netQueues[MAX_CONNECTED_PLAYERS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/// These queues are for clients which just connected, but haven't yet been assigned a player number.
static NetQueuePair *tmpQueues[MAX_TMP_SOCKETS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/// Sending a message to the broadcast queue is equivalent to sending the message to the net queues of all other players.
static NetQueue *broadcastQueue = NULL;

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
	uint8_t b[2] = {v>>8, v};
	queue(q, b[0]);
	queue(q, b[1]);
	v = b[0]<<8 | b[1];
}

template<class Q>
static void queue(const Q &q, uint32_t &v)
{
	uint16_t b[2] = {v>>16, v};
	queue(q, b[0]);
	queue(q, b[1]);
	v = b[0]<<16 | b[1];
}

template<class Q>
static void queue(const Q &q, uint64_t &v)
{
	uint32_t b[2] = {v>>32, v};
	queue(q, b[0]);
	queue(q, b[1]);
	v = uint64_t(b[0])<<32 | b[1];
}

template<class Q>
static void queue(const Q &q, char &v)
{
	uint8_t b = v;
	queue(q, b);
	v = b;

	STATIC_ASSERT(sizeof(b) == sizeof(v));
}

template<class Q>
static void queue(const Q &q, int8_t &v)
{
	uint8_t b = v;
	queue(q, b);
	v = b;

	STATIC_ASSERT(sizeof(b) == sizeof(v));
}

template<class Q>
static void queue(const Q &q, int16_t &v)
{
	uint16_t b = v;
	queue(q, b);
	v = b;

	STATIC_ASSERT(sizeof(b) == sizeof(v));
}

template<class Q>
static void queue(const Q &q, int32_t &v)
{
	uint32_t b = v;
	queue(q, b);
	v = b;

	STATIC_ASSERT(sizeof(b) == sizeof(v));
}

template<class Q>
static void queue(const Q &q, int64_t &v)
{
	uint64_t b = v;
	queue(q, b);
	v = b;

	STATIC_ASSERT(sizeof(b) == sizeof(v));
}

template<class Q>
void queue(const Q &q, float &v)
{
	/*
	 * NB: Not portable.
	 * This routine only works on machines with IEEE754 floating point numbers.
	 */

	#if !defined(__STDC_IEC_559__) \
	 && !defined(__m68k__) && !defined(__sparc__) && !defined(__i386__) \
	 && !defined(__mips__) && !defined(__ns32k__) && !defined(__alpha__) \
	 && !defined(__arm__) && !defined(__ppc__) && !defined(__ia64__) \
	 && !defined(__arm26__) && !defined(__sparc64__) && !defined(__amd64__) \
	 && !defined(WZ_CC_MSVC) // Assume that all platforms supported by
	                         // MSVC provide IEEE754 floating point numbers
	# error "this platform hasn't been confirmed to support IEEE754 floating point numbers"
	#endif

	// IEEE754 floating point numbers can be treated the same as 32-bit integers
	// with regards to endian conversion
	uint32_t b;
	std::memcpy(&b, &v, sizeof(b));
	queue(q, b);
	std::memcpy(&v, &b, sizeof(b));

	STATIC_ASSERT(sizeof(b) == sizeof(v));
}

template<class Q>
static void queue(const Q &q, Vector3uw &v)
{
	queue(q, v.x);
	queue(q, v.y);
	queue(q, v.z);
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
	return ret;
}

NETQUEUE NETnetQueue(unsigned player)
{
	NETQUEUE ret;

	if (player == NET_ALL_PLAYERS)
	{
		return NETbroadcastQueue();
	}

	ASSERT(player < MAX_CONNECTED_PLAYERS, "Huh?");
	NetQueuePair **queue = &netQueues[player];
	ret.queue = queue;
	ret.isPair = true;
	ret.index = player;
	ret.queueType = QUEUE_NET;
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
	return ret;
}

NETQUEUE NETbroadcastQueue()
{
	NETQUEUE ret;
	NetQueue *queue = broadcastQueue;
	ret.queue = queue;
	ret.isPair = false;
	ret.index = NET_ALL_PLAYERS;
	ret.queueType = QUEUE_BROADCAST;
	return ret;
}

void NETinsertRawData(NETQUEUE queue, uint8_t *data, size_t dataLen)
{
	receiveQueue(queue)->writeRawData(data, dataLen);
}

BOOL NETisMessageReady(NETQUEUE queue)
{
	return receiveQueue(queue)->haveMessage();
}

uint8_t NETmessageType(NETQUEUE queue)
{
	return receiveQueue(queue)->getMessage().type;
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

void NETsetNoSendOverNetwork(NETQUEUE queue)
{
	sendQueue(queue)->setWillNeverReadRawData();  // Will not be sending over net.
}

void NETmoveQueue(NETQUEUE src, NETQUEUE dst)
{
	ASSERT(src.isPair, "Huh?");
	ASSERT(dst.isPair, "Huh?");
	delete pairQueue(dst);
	pairQueue(dst) = NULL;
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

BOOL NETend()
{
	// If we are encoding just return true
	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		// Push the message onto the list.
		NetQueue *queue = sendQueue(queueInfo);
		queue->pushMessage(message);

		if (queueInfo.queueType == QUEUE_NET || queueInfo.queueType == QUEUE_BROADCAST)
		{
			const uint8_t *data;
			size_t dataLen;

			queue->readRawData(&data, &dataLen);
			NETsend(queueInfo.index, data, dataLen);
			queue->popRawData(dataLen);
		}
		else if (queueInfo.queueType == QUEUE_GAME && queue->checkCanReadRawData())
		{
			uint8_t player = queueInfo.index;  // queueInfo.index is changed by NETbeginEncode.

			const uint8_t *data;
			size_t dataLen;
			queue->readRawData(&data, &dataLen);

			// Not sure exactly where this belongs, but easy to put here in NETend()...
			// Decoded in NETprocessSystemMessage in netplay.c.
			NETbeginEncode(NETbroadcastQueue(), NET_SHARE_GAME_QUEUE);
				NETuint8_t(&player);
				uint32_t size = dataLen;
				NETuint32_t(&size);
				NETbin(const_cast<uint8_t *>(data), dataLen);  // const_cast is safe since we are encoding, not decoding.
			NETend();  // Recursive call, but queueInfo.queueType = QUEUE_BROADCAST now.

			queue->popRawData(dataLen);
		}

		// We have ended the serialisation, so mark the direction invalid
		NETsetPacketDir(PACKET_INVALID);

		return true;  // Serialising never fails.
	}

	if (NETgetPacketDir() == PACKET_DECODE)
	{
		bool ret = reader.valid();

		//receiveQueue(queueInfo)->popMessage();  // Moved to NETpop(), since some messages call NETbeginEncode but not NETend, and others call neither.

		// We have ended the deserialisation, so mark the direction invalid
		NETsetPacketDir(PACKET_INVALID);

		return ret;
	}

	assert(false && false && false);
	return false;
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

void NETfloat(float *fp)
{
	queueAuto(*fp);
}

void NETbool(BOOL *bp)
{
	uint8_t i = !!*bp;
	queueAuto(i);
	*bp = !!i;
}

/*
 * NETnull should be used to either add 4 bytes of padding to a message, or to
 * discard 4 bytes of data from a message.
 */
void NETnull()
{
	uint32_t zero = 0;
	NETuint32_t(&zero);
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

	uint16_t len = NETgetPacketDir() == PACKET_ENCODE ? strnlen1(str, maxlen) - 1 : 0;
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

void NETbin(uint8_t *str, uint16_t maxlen)
{
	/*
	 * Bins sent over the network are prefixed with their length, sent as an
	 * unsigned 16-bit integer.
	 */

	// Work out the length of the bin if we are encoding
	uint16_t len = NETgetPacketDir() == PACKET_ENCODE ? maxlen : 0;
	queueAuto(len);

	// Truncate length if necessary
	if (len > maxlen)
	{
		debug(LOG_ERROR, "NETbin: Decoding packet, buffer size %u truncated at %u", len, maxlen);
		len = maxlen;
	}

	for (unsigned i = 0; i < len; ++i)
	{
		queueAuto(str[i]);
	}

	// Throw away length...
	//maxlen = len;
}

void NETVector3uw(Vector3uw *vp)
{
	queueAuto(*vp);
}

typedef enum
{
	test_a,
	test_b,
} test_enum;

static void NETcoder(PACKETDIR dir)
{
/*	static const char original[] = "THIS IS A TEST STRING";
	char str[sizeof(original)];
	BOOL b = true;
	uint32_t u32 = 32;
	uint16_t u16 = 16;
	uint8_t u8 = 8;
	int32_t i32 = -32;
	int16_t i16 = -16;
	int8_t i8 = -8;
	test_enum te = test_b;

	sstrcpy(str, original);

	if (dir == PACKET_ENCODE)
		NETbeginEncode(0, 0);
	else
		NETbeginDecode(0, 0);
	NETbool(&b);			assert(b == true);
	NETuint32_t(&u32);  assert(u32 == 32);
	NETuint16_t(&u16);  assert(u16 == 16);
	NETuint8_t(&u8);    assert(u8 == 8);
	NETint32_t(&i32);   assert(i32 == -32);
	NETint16_t(&i16);   assert(i16 == -16);
	NETint8_t(&i8);     assert(i8 == -8);
	NETstring(str, sizeof(str)); assert(strncmp(str, original, sizeof(str) - 1) == 0);
	NETenum(&te);       assert(te == test_b);*/
}

void NETtest()
{
	/*NETMSG cmp;

	memset(&cmp, 0, sizeof(cmp));
	memset(&NetMsg, 0, sizeof(NetMsg));
	NETcoder(PACKET_ENCODE);
	memcpy(&cmp, &NetMsg, sizeof(cmp));
	NETcoder(PACKET_DECODE);
	ASSERT(memcmp(&cmp, &NetMsg, sizeof(cmp)) == 0, "nettypes unit test failed");
	fprintf(stdout, "\tNETtypes self-test: PASSED\n");*/
	ASSERT(false, "nettypes test disabled, since it doesn't compile anymore.");
}
