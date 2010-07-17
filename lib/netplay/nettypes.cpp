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
#include "netlog.h"
#include "src/order.h"
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
	if (Q::Direction == Q::Read)
	{
		v = b[0]<<8 | b[1];
	}
}

template<class Q>
static void queue(const Q &q, uint32_t &v)
{
	uint16_t b[2] = {v>>16, v};
	queue(q, b[0]);
	queue(q, b[1]);
	if (Q::Direction == Q::Read)
	{
		v = b[0]<<16 | b[1];
	}
}

template<class Q>
static void queue(const Q &q, uint64_t &v)
{
	uint32_t b[2] = {v>>32, v};
	queue(q, b[0]);
	queue(q, b[1]);
	if (Q::Direction == Q::Read)
	{
		v = uint64_t(b[0])<<32 | b[1];
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
	uint32_t b = v;
	queue(q, b);
	if (Q::Direction == Q::Read)
	{
		v = b;
	}

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
	if (Q::Direction == Q::Read)
	{
		std::memcpy(&v, &b, sizeof(b));
	}

	STATIC_ASSERT(sizeof(b) == sizeof(v));
}

template<class Q>
static void queueSmall(const Q &q, int32_t &v)
{
	// Non-negative values: value*2
	// Negative values:     -value*2 - 1
	// Example: int32_t -5 -4 -3 -2 -1  0  1  2  3  4  5
	// becomes uint32_t  9  7  5  3  1  0  2  4  6  8 10

	uint32_t b = v<<1 ^ (-((uint32_t)v>>31));
	queueSmall(q, b);

	if (Q::Direction == Q::Read)
	{
		v = b>>1 ^ -(b&1);
	}
}

template<class Q>
static void queueSmall(const Q &q, uint32_t &vOrig)
{
	// Byte n is the final byte, iff it is less than 256-a[n].

	// Some possible values of a[n], such that the maximum encodable value is 0xFFFFFFFF, satisfying the assertion below:
	//const unsigned a[5] = {14, 127, 74, 127, 0};  // <242: 1 byte, <2048: 2 bytes, <325644: 3 bytes, <17298432: 4 bytes, <4294967296: 5 bytes
	const unsigned a[5] = {78, 95, 32, 70, 0};  // <178: 1 byte, <12736: 2 bytes, <1672576: 3 bytes, <45776896: 4 bytes, <4294967296: 5 bytes
	//const unsigned a[5] = {78, 95, 71, 31, 0};  // <178: 1 byte, <12736: 2 bytes, <1383586: 3 bytes, <119758336: 4 bytes, <4294967296: 5 bytes
	//const unsigned a[5] = {104, 71, 19, 119, 0};  // <152: 1 byte, <19392: 2 bytes, <1769400: 3 bytes, <20989952: 4 bytes, <4294967296: 5 bytes
	//const unsigned a[5] = {104, 71, 20, 113, 0};  // <152: 1 byte, <19392: 2 bytes, <1762016: 3 bytes, <22880256: 4 bytes, <4294967296: 5 bytes
	//const unsigned a[5] = {104, 71, 24, 94, 0};  // <152: 1 byte, <19392: 2 bytes, <1732480: 3 bytes, <30441472: 4 bytes, <4294967296: 5 bytes
	//const unsigned a[5] = {104, 71, 30, 75, 0};  // <152: 1 byte, <19392: 2 bytes, <1688176: 3 bytes, <41783296: 4 bytes, <4294967296: 5 bytes
	//const unsigned a[5] = {104, 71, 38, 59, 0};  // <152: 1 byte, <19392: 2 bytes, <1629104: 3 bytes, <56905728: 4 bytes, <4294967296: 5 bytes
	//const unsigned a[5] = {104, 71, 40, 56, 0};  // <152: 1 byte, <19392: 2 bytes, <1614336: 3 bytes, <60686336: 4 bytes, <4294967296: 5 bytes
	//const unsigned a[5] = {104, 71, 57, 39, 0};  // <152: 1 byte, <19392: 2 bytes, <1488808: 3 bytes, <92821504: 4 bytes, <4294967296: 5 bytes
	//const unsigned a[5] = {104, 71, 60, 37, 0};  // <152: 1 byte, <19392: 2 bytes, <1466656: 3 bytes, <98492416: 4 bytes, <4294967296: 5 bytes
	//const unsigned a[5] = {104, 71, 76, 29, 0};  // <152: 1 byte, <19392: 2 bytes, <1348512: 3 bytes, <128737280: 4 bytes, <4294967296: 5 bytes
	//const unsigned a[5] = {104, 71, 95, 23, 0};  // <152: 1 byte, <19392: 2 bytes, <1208216: 3 bytes, <164653056: 4 bytes, <4294967296: 5 bytes
	//const unsigned a[5] = {104, 71, 114, 19, 0};  // <152: 1 byte, <19392: 2 bytes, <1067920: 3 bytes, <200568832: 4 bytes, <4294967296: 5 bytes
	//const unsigned a[5] = {104, 71, 120, 18, 0};  // <152: 1 byte, <19392: 2 bytes, <1023616: 3 bytes, <211910656: 4 bytes, <4294967296: 5 bytes

	ASSERT(0xFFFFFFFF == 255*(1 + a[0]*(1 + a[1]*(1 + a[2]*(1 + a[3]*(1 + a[4]))))), "Maximum encodable value not 0xFFFFFFFF.");

	if (Q::Direction == Q::Write)
	{
		uint32_t v = vOrig;
		for (int n = 0;; ++n)
		{
			if (v < 256 - a[n])
			{
				uint8_t b = v;
				queue(q, b);
				break;  // Last encoded byte.
			}
			else
			{
				v -= 256 - a[n];
				uint8_t b = 255 - v%a[n];
				queue(q, b);
				v /= a[n];
			}
		}
	}
	else if (Q::Direction == Q::Read)
	{
		uint32_t v = 0, m = 1;
		for (int n = 0;; ++n)
		{
			uint8_t b;
			queue(q, b);

			if (b < 256 - a[n])
			{
				v += b*m;
				break;  // Last encoded byte.
			}
			else
			{
				v += (256 - a[n] + 255 - b)*m;
				m *= a[n];
			}
		}

		vOrig = v;
	}
}

template<class Q>
static void queue(const Q &q, Position &v)
{
	queueSmall(q, v.x);
	queueSmall(q, v.y);
	queueSmall(q, v.z);
}

template<class Q>
static void queue(const Q &q, Rotation &v)
{
	queue(q, v.direction);
	queue(q, v.pitch);
	queue(q, v.roll);
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

template<class T>
static void queueAutoSmall(T &v)
{
	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		queueSmall(writer, v);
	}
	else if (NETgetPacketDir() == PACKET_DECODE)
	{
		queueSmall(reader, v);
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

void NETinsertMessageFromNet(NETQUEUE queue, NETMESSAGE message)
{
	receiveQueue(queue)->pushMessage(*message);
}

BOOL NETisMessageReady(NETQUEUE queue)
{
	return receiveQueue(queue)->haveMessage();
}

NETMESSAGE NETgetMessage(NETQUEUE queue)
{
	return &receiveQueue(queue)->getMessage();
}

uint8_t NETmessageType(NETMESSAGE message)
{
	return message->type;
}

uint32_t NETmessageSize(NETMESSAGE message)
{
	return message->data.size();
}

uint8_t *NETmessageRawData(NETMESSAGE message)
{
	return message->rawDataDup();
}

void NETmessageDestroyRawData(uint8_t *data)
{
	delete[] data;
}

size_t NETmessageRawSize(NETMESSAGE message)
{
	return message->rawLen();
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
	sendQueue(queue)->setWillNeverGetMessagesForNet();  // Will not be sending over net.
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
		NETlogPacket(message.type, message.data.size(), false);

		if (queueInfo.queueType == QUEUE_GAME)
		{
			ASSERT(message.type > GAME_MIN_TYPE && message.type < GAME_MAX_TYPE, "Inserting %s into game queue.", messageTypeToString(message.type));
		}
		else
		{
			ASSERT(message.type > NET_MIN_TYPE && message.type < NET_MAX_TYPE, "Inserting %s into net queue.", messageTypeToString(message.type));
		}

		if (queueInfo.queueType == QUEUE_NET || queueInfo.queueType == QUEUE_BROADCAST)
		{
			NETsend(queueInfo.index, &queue->getMessageForNet());
			queue->popMessageForNet();
		}

		// We have ended the serialisation, so mark the direction invalid
		NETsetPacketDir(PACKET_INVALID);

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

		if (queue == NULL)
		{
			continue;  // Can't send for this player.
		}

		uint32_t num = queue->numMessagesForNet();

		if (num <= 0)
		{
			continue;  // Nothing to send for this player.
		}

		// Decoded in NETprocessSystemMessage in netplay.c.
		NETbeginEncode(NETbroadcastQueue(), NET_SHARE_GAME_QUEUE);
			NETuint8_t(&player);
			NETuint32_tSmall(&num);
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

void NETint32_tSmall(int32_t *ip)
{
	queueAutoSmall(*ip);
}

void NETuint32_t(uint32_t *ip)
{
	queueAuto(*ip);
}

void NETuint32_tSmall(uint32_t *ip)
{
	queueAutoSmall(*ip);
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

void NETbin(uint8_t *str, uint32_t maxlen)
{
	/*
	 * Bins sent over the network are prefixed with their length, sent as an
	 * unsigned 32-bit integer.
	 */

	// Work out the length of the bin if we are encoding
	uint32_t len = NETgetPacketDir() == PACKET_ENCODE ? maxlen : 0;
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

void NETPosition(Position *vp)
{
	queueAuto(*vp);
}

void NETRotation(Rotation *vp)
{
	queueAuto(*vp);
}

void NETPACKAGED_CHECK(PACKAGED_CHECK *v)
{
	queueAuto(v->player);
	queueAuto(v->droidID);
	queueAuto(v->order);
	queueAuto(v->secondaryOrder);
	queueAuto(v->body);
	queueAuto(v->experience);
	queueAuto(v->pos);
	queueAuto(v->rot);
	queueAuto(v->sMoveX);
	queueAuto(v->sMoveY);
	if (v->order == DORDER_ATTACK)
	{
		queueAuto(v->targetID);
	}
	else if (v->order == DORDER_MOVE)
	{
		queueAuto(v->orderX);
		queueAuto(v->orderY);
	}
}

void NETNETMESSAGE(NETMESSAGE *message)
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

void NETdestroyNETMESSAGE(NETMESSAGE message)
{
	delete message;
}

typedef enum
{
	test_a,
	test_b,
} test_enum;

static void NETcoder(PACKETDIR dir)
{
	(void)dir;
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
	*/
	NETcoder(PACKET_ENCODE);
	/*
	memcpy(&cmp, &NetMsg, sizeof(cmp));
	NETcoder(PACKET_DECODE);
	ASSERT(memcmp(&cmp, &NetMsg, sizeof(cmp)) == 0, "nettypes unit test failed");
	fprintf(stdout, "\tNETtypes self-test: PASSED\n");*/
	ASSERT(false, "nettypes test disabled, since it doesn't compile anymore.");
}
