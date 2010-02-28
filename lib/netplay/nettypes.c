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

static PACKETDIR NetDir;

static void NETsetPacketDir(PACKETDIR dir)
{
	NetDir = dir;
}

PACKETDIR NETgetPacketDir()
{
	return NetDir;
} 

/*
 * Begin & End functions
 */

void NETbeginEncode(uint8_t type, uint8_t player)
{
	NETsetPacketDir(PACKET_ENCODE);
	NetMsg.type = type;
	NetMsg.size = 0;
	NetMsg.status = true;
	NetMsg.destination = player;
	memset(&NetMsg.body, '\0', sizeof(NetMsg.body));
}

void NETbeginDecode(uint8_t type)
{
	NETsetPacketDir(PACKET_DECODE);
	assert(type == NetMsg.type);
	NetMsg.size = 0;
	NetMsg.status = true;
}

BOOL NETend(void)
{
	assert(NETgetPacketDir() != PACKET_INVALID);

	// If we are decoding just return true
	if (NETgetPacketDir() == PACKET_DECODE)
	{
		return true;
	}

	// If the packet is invalid or failed to compile
	if (NETgetPacketDir() != PACKET_ENCODE || !NetMsg.status)
	{
		return false;
	}

	// We have sent the packet, so make it invalid (to prevent re-sending)
	NETsetPacketDir(PACKET_INVALID);

	// Send the packet, updating the send functions is on my todo list!
	if (NetMsg.destination == NET_ALL_PLAYERS)
	{
		return NETbcast(&NetMsg);
	}
	else
	{
		return NETsend(&NetMsg, NetMsg.destination);
	}
}

/*
 * Primitive functions (ints and strings). Due to the lack of C++ and the fact
 * that I hate macros this is a lot longer than it should be.
 */

BOOL NETint8_t(int8_t *ip)
{
	int8_t *store = (int8_t *) &NetMsg.body[NetMsg.size];

	// Make sure there is enough data/space left in the packet
	if (sizeof(int8_t) + NetMsg.size > sizeof(NetMsg.body) || !NetMsg.status)
	{
		return NetMsg.status = false;
	}

	// 8-bit (1 byte) integers need no endian-swapping!
	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		*store = *ip;
	}
	else if (NETgetPacketDir() == PACKET_DECODE)
	{
		*ip = *store;
	}

	// Increment the size of the message
	NetMsg.size += sizeof(int8_t);

	return true;
}

BOOL NETuint8_t(uint8_t *ip)
{
	uint8_t *store = (uint8_t *) &NetMsg.body[NetMsg.size];

	// Make sure there is enough data/space left in the packet
	if (sizeof(uint8_t) + NetMsg.size > sizeof(NetMsg.body) || !NetMsg.status)
	{
		return NetMsg.status = false;
	}

	// 8-bit (1 byte) integers need no endian-swapping!
	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		*store = *ip;
	}
	else if (NETgetPacketDir() == PACKET_DECODE)
	{
		*ip = *store;
	}

	// Increment the size of the message
	NetMsg.size += sizeof(uint8_t);

	return true;
}

BOOL NETint16_t(int16_t *ip)
{
	int16_t *store = (int16_t *) &NetMsg.body[NetMsg.size];

	// Make sure there is enough data/space left in the packet
	if (sizeof(int16_t) + NetMsg.size > sizeof(NetMsg.body) || !NetMsg.status)
	{
		return NetMsg.status = false;
	}

	// Convert the integer into the network byte order (big endian)
	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		*store = SDL_SwapBE16(*ip);
	}
	else if (NETgetPacketDir() == PACKET_DECODE)
	{
		*ip = SDL_SwapBE16(*store);
	}

	// Increment the size of the message
	NetMsg.size += sizeof(int16_t);

	return true;
}

BOOL NETuint16_t(uint16_t *ip)
{
	uint16_t *store = (uint16_t *) &NetMsg.body[NetMsg.size];

	// Make sure there is enough data/space left in the packet
	if (sizeof(uint16_t) + NetMsg.size > sizeof(NetMsg.body) || !NetMsg.status)
	{
		return NetMsg.status = false;
	}

	// Convert the integer into the network byte order (big endian)
	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		*store = SDL_SwapBE16(*ip);
	}
	else if (NETgetPacketDir() == PACKET_DECODE)
	{
		*ip = SDL_SwapBE16(*store);
	}

	// Increment the size of the message
	NetMsg.size += sizeof(uint16_t);

	return true;
}

BOOL NETint32_t(int32_t *ip)
{
	int32_t *store = (int32_t *) &NetMsg.body[NetMsg.size];

	// Make sure there is enough data/space left in the packet
	if (sizeof(int32_t) + NetMsg.size > sizeof(NetMsg.body) || !NetMsg.status)
	{
		return NetMsg.status = false;
	}

	// Convert the integer into the network byte order (big endian)
	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		*store = SDL_SwapBE32(*ip);
	}
	else if (NETgetPacketDir() == PACKET_DECODE)
	{
		*ip = SDL_SwapBE32(*store);
	}

	// Increment the size of the message
	NetMsg.size += sizeof(int32_t);

	return true;
}

BOOL NETuint32_t(uint32_t *ip)
{
	uint32_t *store = (uint32_t *) &NetMsg.body[NetMsg.size];

	// Make sure there is enough data/space left in the packet
	if (sizeof(uint32_t) + NetMsg.size > sizeof(NetMsg.body) || !NetMsg.status)
	{
		return NetMsg.status = false;
	}

	// Convert the integer into the network byte order (big endian)
	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		*store = SDL_SwapBE32(*ip);
	}
	else if (NETgetPacketDir() == PACKET_DECODE)
	{
		*ip = SDL_SwapBE32(*store);
	}

	// Increment the size of the message
	NetMsg.size += sizeof(uint32_t);

	return true;
}

BOOL NETfloat(float *fp)
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
	STATIC_ASSERT(sizeof(float) == sizeof(int32_t));

	return NETint32_t((int32_t *) fp);
}

BOOL NETbool(BOOL *bp)
{
	// Bools are converted to uint8_ts
	uint8_t tmp = (uint8_t) *bp;
	NETuint8_t(&tmp);

	// If we are decoding and managed to extract the value set it
	if (NETgetPacketDir() == PACKET_DECODE && NetMsg.status)
	{
		*bp = (BOOL) tmp;
	}

	return NetMsg.status;
}

/*
 * NETnull should be used to either add 4 bytes of padding to a message, or to
 * discard 4 bytes of data from a message.
 */
BOOL NETnull()
{
	uint32_t zero = 0;
	return NETuint32_t(&zero);
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
BOOL NETstring(char *str, uint16_t maxlen)
{
	/*
	 * Strings sent over the network are prefixed with their length, sent as an
	 * unsigned 16-bit integer.
	 */

	// Work out the length of the string if we are encoding
	uint16_t len = (NETgetPacketDir() == PACKET_ENCODE) ? strnlen1(str, maxlen) : 0;
	char *store;

	// Add/fetch the length from the packet
	NETuint16_t(&len);

	// Map store to the message buffer
	store = (char *) &NetMsg.body[NetMsg.size];

	// Make sure there is enough data/space left in the packet
	if (len + NetMsg.size > sizeof(NetMsg.body) || !NetMsg.status)
	{
		return NetMsg.status = false;
	}

	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		memcpy(store, str, len);
	}
	else if (NETgetPacketDir() == PACKET_DECODE)
	{
		// Truncate length if necessary
		if (len > maxlen)
		{
			debug(LOG_ERROR, "NETstring: Decoding packet type %d from %d, buffer size %u truncated at %u", 
			      NetMsg.type, NetMsg.source, len, maxlen);
			len = maxlen;
		}
		memcpy(str, store, len);
		// Guarantee NUL-termination
		str[len - 1] = '\0';
	}

	// Increment the size of the message
	NetMsg.size += sizeof(len) + len;

	return true;
}

BOOL NETbin(char *str, uint16_t maxlen)
{
	/*
	 * Strings sent over the network are prefixed with their length, sent as an
	 * unsigned 16-bit integer.
	 */

	// Work out the length of the string if we are encoding
	uint16_t len = (NETgetPacketDir() == PACKET_ENCODE) ? maxlen : 0;
	char *store;

	// Add/fetch the length from the packet
	NETuint16_t(&len);

	// Map store to the message buffer
	store = (char *) &NetMsg.body[NetMsg.size];

	// Make sure there is enough data/space left in the packet
	if (len + NetMsg.size > sizeof(NetMsg.body) || !NetMsg.status)
	{
		return NetMsg.status = false;
	}

	if (NETgetPacketDir() == PACKET_ENCODE)
	{
		memcpy(store, str, len);
	}
	else if (NETgetPacketDir() == PACKET_DECODE)
	{
		// Truncate length if necessary
		if (len > maxlen)
		{
			debug(LOG_ERROR, "NETbin: Decoding packet type %d from %d, buffer size %u truncated at %u", 
			      NetMsg.type, NetMsg.source, len, maxlen);
			len = maxlen;
		}
		memcpy(str, store, len);
	}

	// Increment the size of the message
	NetMsg.size += sizeof(len) + len;

	return true;
}

BOOL NETRotation(Rotation *vp)
{
	return (NETuint16_t(&vp->direction)
	     && NETuint16_t(&vp->pitch)
	     && NETuint16_t(&vp->roll));
}

BOOL NETPosition(Position *vp)
{
	return (NETint32_t(&vp->x)
	     && NETint32_t(&vp->y)
	     && NETint32_t(&vp->z));
}

typedef enum
{
	test_a,
	test_b,
} test_enum;

static void NETcoder(PACKETDIR dir)
{
	static const char original[] = "THIS IS A TEST STRING";
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
		NETbeginDecode(0);
	NETbool(&b);			assert(b == true);
	NETuint32_t(&u32);  assert(u32 == 32);
	NETuint16_t(&u16);  assert(u16 == 16);
	NETuint8_t(&u8);    assert(u8 == 8);
	NETint32_t(&i32);   assert(i32 == -32);
	NETint16_t(&i16);   assert(i16 == -16);
	NETint8_t(&i8);     assert(i8 == -8);
	NETstring(str, sizeof(str)); assert(strncmp(str, original, sizeof(str) - 1) == 0);
	NETenum(&te);       assert(te == test_b);
}

void NETtest()
{
	NETMSG cmp;

	memset(&cmp, 0, sizeof(cmp));
	memset(&NetMsg, 0, sizeof(NetMsg));
	NETcoder(PACKET_ENCODE);
	memcpy(&cmp, &NetMsg, sizeof(cmp));
	NETcoder(PACKET_DECODE);
	ASSERT(memcmp(&cmp, &NetMsg, sizeof(cmp)) == 0, "nettypes unit test failed");
	fprintf(stdout, "\tNETtypes self-test: PASSED\n");
}

int NETgetSource()
{
	return NetMsg.source;
}
