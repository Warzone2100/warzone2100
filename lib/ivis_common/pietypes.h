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
 *	Global Type Definitions
 */
/***************************************************************************/
typedef Sint32 int32;
typedef Uint8 uint8;
typedef Uint16 uint16;
typedef Uint32 uint32;

//*************************************************************************
//
// Simple derived types
//
//*************************************************************************
typedef struct { Sint32 left, top, right, bottom; } iClip;
typedef char iBitmap;
typedef struct { Uint8 r, g, b; } iColour;
typedef iColour iPalette[256];
typedef struct { Sint32 x, y; } iPoint;
typedef struct { Sint32 x, y, z; } iVector;
typedef struct { double x, y, z; } iVectorf;
typedef struct { Sint32 width, height; iBitmap *bmp; } iTexture;
typedef iTexture iSprite;
typedef struct { Sint32 x, y, z, u, v; uint8 g; } iVertex;
typedef struct { float x, y, z; } PIEVECTORF;
typedef struct { iVector p, r; } iView;

#endif // _pieTypes_h
