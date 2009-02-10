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

	$Revision$
	$Id$
	$HeadURL$
*/

#ifndef __INCLUDE_DEBUGPRINT_HPP__
#define __INCLUDE_DEBUGPRINT_HPP__

//#define DEBUG0
//#define DEBUG1
//#define DEBUG2
//#define DEBUG3
//#define DEBUG4
#define DEBUG5
	
#ifndef _TCHAR_DEFINED
typedef char TCHAR, *PTCHAR;
typedef unsigned char TBYTE , *PTBYTE ;
#define _TCHAR_DEFINED
#endif /* !_TCHAR_DEFINED */


#ifdef _DEBUG
void DebugOpen(const char* LogName);
void DebugClose();
void DebugPrint(const TCHAR *format, ...);
#else
#define DebugOpen
static inline void DebugClose() {};
#define DebugPrint
#endif

#ifdef __cplusplus
# ifndef ASSERT
#  define ASSERT
# endif
#else
# ifndef ASSERT
#  define DBG_ASSERT(a,b) if(!(a)) { DebugPrint("Assertion Failure in %s, line : %d\n",__FILE__, __LINE__); \
          DebugPrint(b); \
          DebugPrint("\n"); }
#  define ASSERT(a) DBG_ASSERT a

# endif
#endif

#define DBERROR(a) DebugPrint a;

#define DBPRINTF(a) DebugPrint a;

#ifdef DEBUG0
#define DBP0(a) DebugPrint a;
#else
#define DBP0
#endif

#ifdef DEBUG1
#define DBP1(a) DebugPrint a;
#else
#define DBP1
#endif

#ifdef DEBUG2
#define DBP2(a) DebugPrint a;
#else
#define DBP2
#endif

#ifdef DEBUG3
#define DBP3(a) DebugPrint a;
#else
#define DBP3
#endif

#ifdef DEBUG4
#define DBP4(a) DebugPrint a;
#else
#define DBP4
#endif

#ifdef DEBUG5
#define DBP5(a) DebugPrint a;
#else
#define DBP5
#endif

#endif // __INCLUDE_DEBUGPRINT_HPP__
