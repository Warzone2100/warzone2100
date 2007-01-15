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
/***************************************************************************/
/*
 * pieTypes.h
 *
 * type defines for simple pies.
 *
 */
/***************************************************************************/

#ifndef _pieTypes_h
#define _pieTypes_h

#include "lib/framework/frame.h"

/***************************************************************************/
/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

#define PI 	  					3.141592654

/***************************************************************************/
/*
 *	Global Macros
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Global Type Definitions
 */
/***************************************************************************/
typedef signed char int8;
typedef signed short int16;
typedef int int32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

//*************************************************************************
//
// Simple derived types
//
//*************************************************************************
typedef struct {int left, top, right, bottom;} iClip;
typedef uint8 iBitmap;
typedef struct {uint8 r, g, b;} iColour;
typedef int iBool;
typedef struct {int32 x, y;} iPoint;
typedef struct {unsigned int width, height; iBitmap *bmp;} iSprite;
typedef iColour iPalette[256];
typedef struct {uint8 r, g, b, p;} iRGB8;
typedef struct {uint16 r, g, b, p;} iRGB16;
typedef struct {uint32 r, g, b, p;} iRGB32;
typedef struct {int8 x, y;} iPoint8;
typedef struct {int16 x, y;} iPoint16;
typedef struct {int32 x, y;} iPoint32;


	typedef struct {int32 x, y, z;} iVector;
	typedef struct {double x, y, z;} iVectorf;
	typedef struct {int xshift, width, height; iBitmap *bmp;
			iColour *pPal; iBool bColourKeyed; } iTexture;
	typedef struct {int32 x, y, z, u, v; uint8 g;} iVertex;

typedef struct {FRACT x,y,z;} PIEVECTORF;
typedef struct {iVector p, r;} iView;

#endif // _pieTypes_h
