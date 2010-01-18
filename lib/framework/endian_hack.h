/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
#ifndef ENDIAN_HACK_H
#define ENDIAN_HACK_H

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

/* Endianness hacks */
// TODO Use SDL_SwapXXXX instead

static inline void endian_uword(UWORD* uword)
{
#ifdef __BIG_ENDIAN__
  UBYTE tmp, *ptr;

  ptr = (UBYTE *) uword;
  tmp = ptr[0];
  ptr[0] = ptr[1];
  ptr[1] = tmp;
#else
  // Prevent warnings
  (void)uword;
#endif
}

static inline void endian_sword(SWORD* sword)
{
#ifdef __BIG_ENDIAN__
  UBYTE tmp, *ptr;

  ptr = (UBYTE *) sword;
  tmp = ptr[0];
  ptr[0] = ptr[1];
  ptr[1] = tmp;
#else
  // Prevent warnings
  (void)sword;
#endif
}

static inline void endian_udword(UDWORD* udword)
{
#ifdef __BIG_ENDIAN__
  UBYTE tmp, *ptr;

  ptr = (UBYTE *) udword;
  tmp = ptr[0];
  ptr[0] = ptr[3];
  ptr[3] = tmp;
  tmp = ptr[1];
  ptr[1] = ptr[2];
  ptr[2] = tmp;
#else
  // Prevent warnings
  (void)udword;
#endif
}

static inline void endian_sdword(SDWORD* sdword)
{
#ifdef __BIG_ENDIAN__
  UBYTE tmp, *ptr;

  ptr = (UBYTE *) sdword;
  tmp = ptr[0];
  ptr[0] = ptr[3];
  ptr[3] = tmp;
  tmp = ptr[1];
  ptr[1] = ptr[2];
  ptr[2] = tmp;
#else
  // Prevent warnings
  (void)sdword;
#endif
}

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // ENDIAN_HACK_H
