/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2022  Warzone 2100 Project

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
/*! \file
 *  \brief Simple type definitions.
 */

#ifndef __INCLUDED_LIB_FRAMEWORK_TYPES_H__
#define __INCLUDED_LIB_FRAMEWORK_TYPES_H__

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

// Always include generated config.h
#undef _XOPEN_SOURCE
#include "wz2100-generated-config.h"

#include <stddef.h> // for size_t

#ifdef HAVE_INTTYPES_H
/* Compilers that have support for C99 have all values below defined in stdint.h */
# include <inttypes.h>
#else
// Defines C99 types for C99 incompatible compilers (e.g. MSVC)
//BEGIN Hope this is right.
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef signed   char      int8_t;
typedef signed   short     int16_t;
typedef signed   int       int32_t;
typedef signed   long long int64_t;
//END   Hope this is right.

#if !defined(__MINGW32__) && !defined(__MINGW64__)
#if defined(_MSC_VER) && (_MSC_VER <= 1500)
#ifndef INT8_MIN
# define INT8_MIN               (-128)
#endif
#ifndef INT16_MIN
# define INT16_MIN              (-32767-1)
#endif
#ifndef INT32_MIN
# define INT32_MIN              (-2147483647-1)
#endif
#ifndef INT8_MAX
# define INT8_MAX               (127)
#endif
#ifndef INT16_MAX
# define INT16_MAX              (32767)
#endif
#ifndef INT32_MAX
# define INT32_MAX              (2147483647)
#endif
#ifndef UINT8_MAX
# define UINT8_MAX              (255)
#endif
#ifndef UINT16_MAX
# define UINT16_MAX             (65535)
#endif
#ifndef UINT32_MAX
# define UINT32_MAX             (4294967295U)
#endif
#else
#define __USE_MINGW_ANSI_STDIO 1
#include <stdint.h>		// MSVC 2010 does have those defined
#endif
#endif

#ifdef _MSC_VER
# define PRIu32					"u"
# define PRIu64					"I64u"
# define PRId64					"I64d"
#endif
#endif // HAVE_INTTYPES_H

#ifdef _MSC_VER
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

#ifndef INT8_MAX
#error inttypes.h and stdint.h defines missing! Make sure that __STDC_FORMAT_MACROS and __STDC_LIMIT_MACROS are defined when compiling C++ files.
#endif

#include <limits.h>
#include <ctype.h>

/* Basic numeric types */
typedef uint8_t  UBYTE;
typedef int8_t   SBYTE;
typedef uint16_t UWORD;
typedef int16_t  SWORD;
typedef uint32_t UDWORD;
typedef int32_t  SDWORD;

/* Numeric size defines */
#define UBYTE_MAX	UINT8_MAX
#define SBYTE_MIN	INT8_MIN
#define SBYTE_MAX	INT8_MAX
#define UWORD_MAX	UINT16_MAX
#define SWORD_MIN	INT16_MIN
#define SWORD_MAX	INT16_MAX
#define UDWORD_MAX	UINT32_MAX
#define SDWORD_MIN	INT32_MIN
#define SDWORD_MAX	INT32_MAX

#endif // __INCLUDED_LIB_FRAMEWORK_TYPES_H__
