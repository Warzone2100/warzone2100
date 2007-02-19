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
//*************************************************************************
//***	ivi.h iVi engine definitions. [Sam Kerbeck]
//*	24-04-96.30-07-96 PC
//*	18-11-96.04-12-96	WIN'95
//*

#ifndef _ivi_
#define _ivi_

#include "piedef.h"

#define iV_SWAP(a,b)			{ (a) ^= (b); (b) ^= (a); (a) ^= (b); }

#ifdef iV_DDX
	#define iV_DDX_ERROR		0x1000000
#endif

#define iV_DIVSHIFT			15
#define iV_DIVMULTP			(1<<iV_DIVSHIFT)
#define iV_DIVMULTP_2			(1<<(iV_DIVSHIFT-1))

// Simple derived types
typedef union {uint32 *dp; uint8 *bp; uint16 *wp;} iPointer;

extern void iV_Error(long n, const char *msge, ...);

// If its a final build we need to undefine iv_error so that it doesn't generate any code !
#ifdef FINALBUILD

#undef iV_Error
#define iV_Error(fmt...) ;

#endif

//*************************************************************************

/***************************************************************************/
/*
 *	Global Macros
 */
/***************************************************************************/
//* Macros:
//*

#define iV_ASHIFT	  			4
#define iV_SSHIFT	  			7
#define iV_RSHIFT	  	 		12
#define iV_RMULTP				(1L<<iV_RSHIFT)
#define iV_MSHIFT		 		10
#define iV_MMULTP				(1L<<iV_MSHIFT);

#define iV_DIVTABLE_MAX		1024

#define  iV_POLY_MAX_POINTS	 pie_MAX_POLY_SIZE
#define  iV_POLY_FLAT (1)		// added by TJC
#define  iV_POLY_TEXT (2)
#define  iV_POLY_TEXT0 (3)
#define  iV_POLY_GOUR (4)



#ifdef iV_DDX
	#define iV_DDX_ERROR		0x1000000
#endif

/***************************************************************************/
/*
 *	Global Type Definitions
 */
/***************************************************************************/
// Basic type (replace with framework definitions)
typedef unsigned char uchar;
typedef uint32 ufixed;
typedef struct {int32 w, x, y, z;} iQuat;
typedef struct {int x0, y0, x1, y1;} iBox;


//*************************************************************************


extern iError	_iVERROR;


//*************************************************************************

extern void iV_Reset( void );
extern void iV_ShutDown(void);
extern void iV_Stop(char *string, ...);
extern void iV_Abort(char *string, ...);
extern void iV_DisplayLogFile(void);



//*************************************************************************

/* Unused debug functions of dubious design and usefulness */

#define iV_DEBUG0(S)
#define iV_DEBUG1(S,A)
#define iV_DEBUG2(S,A,B)
#define iV_DEBUG3(S,A,B,C)
#define iV_DEBUG4(S,A,B,C,D)
#define iV_DEBUG5(S,A,B,C,D,E)

#endif
