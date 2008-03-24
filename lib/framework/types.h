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
 *  \brief Simple type definitions.
 *  Includes wzglobal.h
 */
#ifndef _types_h
#define _types_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */

#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

#include "wzglobal.h"

// Defines C99 types for C99 incompatible compilers (e.g. MSVC)
#include <SDL_types.h>

#include <limits.h>

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
#endif /* !WZ_OS_WIN */

// If we are C99 compatible, the "bool" macro will be defined in <stdbool.h> (as _Bool)
#if defined(WZ_C99)
# include <stdbool.h>
#else
// Pretend we are C99 compatible (well, for the bool type then)
# ifndef bool
#  define bool BOOL
# endif
# ifndef true
#  define true (1)
# endif
# ifndef false
#  define false (0)
# endif
# ifndef __bool_true_false_are_defined
#  define __bool_true_false_are_defined (1)
# endif
#endif /* WZ_C99 */

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
#  define NULL	((void*)(0))
#endif

#ifndef TRUE
#define TRUE	(1)
#endif

#ifndef FALSE
#define FALSE	(0)
#endif


#endif
