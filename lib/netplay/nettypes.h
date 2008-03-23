/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Warzone Resurrection Project

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
#include "lib/ivis_common/pievector.h"

typedef enum packetDirectionEnum
{
	PACKET_ENCODE,
	PACKET_DECODE,
	PACKET_INVALID
} PACKETDIR;

void NETbeginEncode(uint8_t type, uint8_t player);
void NETbeginDecode(uint8_t type);
BOOL NETend(void);
BOOL NETint8_t(int8_t *ip);
BOOL NETuint8_t(uint8_t *ip);
BOOL NETint16_t(int16_t *ip);
BOOL NETuint16_t(uint16_t *ip);
BOOL NETint32_t(int32_t *ip);
BOOL NETuint32_t(uint32_t *ip);
BOOL NETfloat(float* fp);
BOOL NETbool(BOOL *bp);
BOOL NETnull(void);
BOOL NETstring(char *str, uint16_t maxlen);
BOOL NETbin(char *str, uint16_t maxlen);

PACKETDIR NETgetPacketDir();

#if defined(__cplusplus)
template <typename EnumT>
BOOL NETenum(EnumT* enumPtr)
{
	int32_t val = (NETgetPacketDir() == PACKET_ENCODE) ? *enumPtr : 0;

	NETint32_t(&val);

	*enumPtr = static_cast<EnumT>(val);
}
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

BOOL NETVector3uw(Vector3uw* vp);

/**
 *	Get player who is the source of the current packet.
 *	@see selectedPlayer
 */
int NETgetSource(void);

void NETtest(void);

#endif
