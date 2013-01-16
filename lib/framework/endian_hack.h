/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
 * @file
 *  Endianness functions
 */

#ifndef ENDIAN_HACK_H
#define ENDIAN_HACK_H

#include <string.h>

static inline void endian_uword(UWORD* p)
{
	STATIC_ASSERT(sizeof(*p) == 2);
	uint8_t bytes[sizeof(*p)];
	memcpy(bytes, p, sizeof(*p));
	*p = bytes[1]<<8 | bytes[0];
}

static inline void endian_sword(SWORD* p)
{
	STATIC_ASSERT(sizeof(*p) == 2);
	uint8_t bytes[sizeof(*p)];
	memcpy(bytes, p, sizeof(*p));
	*p = bytes[1]<<8 | bytes[0];
}

static inline void endian_udword(UDWORD* p)
{
	STATIC_ASSERT(sizeof(*p) == 4);
	uint8_t bytes[sizeof(*p)];
	memcpy(bytes, p, sizeof(*p));
	*p = bytes[3]<<24 | bytes[2]<<16 | bytes[1]<<8 | bytes[0];
}

static inline void endian_sdword(SDWORD* p)
{
	STATIC_ASSERT(sizeof(*p) == 4);
	uint8_t bytes[sizeof(*p)];
	memcpy(bytes, p, sizeof(*p));
	*p = bytes[3]<<24 | bytes[2]<<16 | bytes[1]<<8 | bytes[0];
}

template <typename ENUM>
static inline void endian_enum(ENUM *p)
{
	STATIC_ASSERT(sizeof(*p) == 4);
	uint8_t bytes[sizeof(*p)];
	memcpy(bytes, p, sizeof(*p));
	*p = ENUM(bytes[3]<<24 | bytes[2]<<16 | bytes[1]<<8 | bytes[0]);
}

#endif // ENDIAN_HACK_H
