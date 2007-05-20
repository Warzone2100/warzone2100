/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
/*! \file types.h
 *  * \brief Simple type definitions.
 */
#ifndef _types_h
#define _types_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */

#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

#include "platform.h"

// Defines C99 types for C99 incompatible compilers (e.g. MSVC)
#include <SDL/SDL_types.h>

#ifdef WZ_CC_MSVC
# define INT8_MIN               (-128)
# define INT16_MIN              (-32767-1)
# define INT32_MIN              (-2147483647-1)
# define INT8_MAX               (127)
# define INT16_MAX              (32767)
# define INT32_MAX              (2147483647)
# define UINT8_MAX              (255)
# define UINT16_MAX             (65535)
# define UINT32_MAX             (4294967295U)
#else
/* Compilers that have support for C99 have all of the above defined in stdint.h */
# include <stdint.h>
#endif // WZ_CC_MSVC

/* Basic numeric types */
typedef uint8_t  UBYTE;
typedef int8_t   SBYTE;
typedef uint16_t UWORD;
typedef int16_t  SWORD;
typedef uint32_t UDWORD;
typedef int32_t  SDWORD;

#ifndef WZ_OS_WIN

typedef int BOOL;

typedef struct
{
  int  x;
  int  y;
} POINT;

#endif /* !WZ_OS_WIN */


/* Numeric size defines */
#define UBYTE_MAX	0xff
#define SBYTE_MIN	(-128) //(0x80)
#define SBYTE_MAX	0x7f
#define UWORD_MAX	0xffff
#define SWORD_MIN	(-32768) //(0x8000)
#define SWORD_MAX	0x7fff
#define UDWORD_MAX	0xffffffff
#define SDWORD_MIN	(0x80000000)
#define SDWORD_MAX	0x7fffffff

/* Standard Defines */
#ifndef NULL
#define NULL	(0)
#endif

#ifndef TRUE
#define TRUE	(1)
#endif

#ifndef FALSE
#define FALSE	(0)
#endif

/* defines for ONEINX - use

   if (ONEINX)
	{
		code.....
	}

*/

#define	ONEINTWO				rand()%2==0
#define ONEINTHREE				rand()%3==0
#define ONEINFOUR				rand()%4==0
#define ONEINFIVE				rand()%5==0
#define ONEINSIX				rand()%6==0
#define ONEINSEVEN				rand()%7==0
#define ONEINEIGHT				rand()%8==0
#define ONEINNINE				rand()%9==0
#define ONEINTEN				rand()%10==0

#define	ABSDIF(a,b) ((a)>(b) ? (a)-(b) : (b)-(a))
#define CAT(a,b) a##b

#endif
