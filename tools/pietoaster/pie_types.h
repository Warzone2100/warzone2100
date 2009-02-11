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
#ifndef _pie_types_h
#define _pie_types_h

#include "wzglobal.h"
#include "base_types.h"

typedef struct Vector3f { float x, y, z; } Vector3f;
typedef struct Vector2f { float x, y; } Vector2f;
typedef struct Vector3i { int x, y, z; } Vector3i;
typedef struct Vector2i { int x, y; } Vector2i;

typedef struct { Vector3i p, r; } iView;

typedef struct { unsigned char r, g, b; } iColour;

typedef unsigned char iBitmap;
typedef struct { unsigned int width, height, depth; unsigned char *bmp; } iV_Image;
typedef iV_Image iTexture;

/***************************************************************************/
/*
 *	Global Definitions (CONSTANTS)
 */
/***************************************************************************/
#define DEG_360	65536
#define DEG_1	(DEG_360/360)
#define DEG_2	(DEG_360/180)
#define DEG_60	(DEG_360/6)
#define DEG(X)	(DEG_1 * (X))

#define FP12_SHIFT 12
#define FP12_MULTIPLIER (1<<12)
#define STRETCHED_Z_SHIFT		10 // stretchs z range for (1000 to 4000) to (8000 to 32000)
#define MAX_Z				(32000.0f) // raised to 32000 from 6000 when stretched
#define MIN_STRETCHED_Z			256
#define LONG_WAY			(1<<15)
#define LONG_TEST			(1<<14)
#define INTERFACE_DEPTH		(MAX_Z - 1.0f)
#define BUTTON_DEPTH		2000 // will be stretched to 16000

#define OLD_TEXTURE_SIZE_FIX 256.0f


#define pie_MAX_POLY_SIZE	16

//Effects
#define pie_MAX_BRIGHT_LEVEL 255
#define pie_BRIGHT_LEVEL_200 200
#define pie_BRIGHT_LEVEL_180 180
#define pie_DROID_BRIGHT_LEVEL 192

//Render style flags for all pie draw functions
#define pie_TRANSLUCENT         0x2
#define pie_ADDITIVE            0x4
#define pie_NO_BILINEAR         0x8
#define pie_HEIGHT_SCALED       0x10
#define pie_RAISE               0x20
#define pie_BUTTON              0x40
#define pie_SHADOW              0x80
#define pie_STATIC_SHADOW       0x100

#define pie_RAISE_SCALE			256

#define pie_MAX_VERTICES		768
#define pie_MAX_POLYGONS		512
#define pie_MAX_VERTICES_PER_POLYGON	6

#define pie_FILLRED			16
#define pie_FILLGREEN			16
#define pie_FILLBLUE			128
#define pie_FILLTRANS			128

#define MAX_UB_LIGHT			((Uint8)255)
#define MIN_UB_LIGHT			((Uint8)0)
#define MAX_LIGHT			0xffffffff

#define iV_SCANTABLE_MAX	1024

// texture animation defines
#define iV_IMD_ANIM_LOOP	0
#define iV_IMD_ANIM_RUN		1
#define iV_IMD_ANIM_FRAMES	8

/***************************************************************************/
/*
 *	Global Definitions (MACROS)
 */
/***************************************************************************/

#define pie_ADDLIGHT(l,x)						\
(((l)->byte.r > (MAX_UB_LIGHT - (x))) ? ((l)->byte.r = MAX_UB_LIGHT) : ((l)->byte.r +=(x)));		\
(((l)->byte.g > (MAX_UB_LIGHT - (x))) ? ((l)->byte.g = MAX_UB_LIGHT) : ((l)->byte.g +=(x)));		\
(((l)->byte.b > (MAX_UB_LIGHT - (x))) ? ((l)->byte.b = MAX_UB_LIGHT) : ((l)->byte.b +=(x)));

#define pie_SUBTRACTLIGHT(l,x)						\
(((l->byte.r) < (x)) ? ((l->byte.r) = MIN_UB_LIGHT) : ((l->byte.r) -=(x)));		\
(((l->byte.g) < (x)) ? ((l->byte.g) = MIN_UB_LIGHT) : ((l->byte.g) -=(x)));		\
(((l->byte.b) < (x)) ? ((l->byte.b) = MIN_UB_LIGHT) : ((l->byte.b) -=(x)));

//*************************************************************************
//
// texture animation structures
//
//*************************************************************************
typedef struct {
	int npoints;
	Vector2i frame[iV_IMD_ANIM_FRAMES];
} iTexAnimFrame;

typedef struct {
	int nFrames;
	int playbackRate;
	int textureWidth;
	int textureHeight;
} iTexAnim;

//*************************************************************************
//
// imd structures
//
//*************************************************************************

/// Stores the from and to verticles from an edge
typedef struct edge_
{
	int from, to;
} EDGE;

typedef int VERTEXID;	// Size of the entry for vertex id in the imd polygon structure

typedef struct iIMDPoly {
	Uint32 flags;
	Sint32 zcentre;
	int npnts;
	Vector3f normal;
	VERTEXID *pindex;
	Vector2f *texCoord;
	iTexAnim *pTexAnim;
} iIMDPoly;

typedef struct _iIMDShape {
	int texpage;
	int sradius, radius, xmin, xmax, ymin, ymax, zmin, zmax;

	Vector3f ocen;
	unsigned short numFrames;
	unsigned short animInterval;

	unsigned int npoints;
	Vector3f *points;

	unsigned int npolys;
	iIMDPoly *polys;

	unsigned int nconnectors;
	Vector3f *connectors;

	unsigned int nShadowEdges;
	EDGE *shadowEdgeList;

	struct _iIMDShape *next; // next pie in multilevel pies (NULL for non multilevel !)
} iIMDShape;

/***************************************************************************/
/*
 *	Global Definitions (STRUCTURES)
 */
/***************************************************************************/

#ifdef __BIG_ENDIAN__
typedef struct {Uint8 a, r, g, b;} PIELIGHTBYTES; //for byte fields in a DWORD
#else
typedef struct {Uint8 b, g, r, a;} PIELIGHTBYTES; //for byte fields in a DWORD
#endif
typedef union  {PIELIGHTBYTES byte; Uint32 argb;} PIELIGHT;
typedef struct {Uint8 r, g, b, a;} PIEVERTLIGHT;
typedef struct {int x, y, z; unsigned int u, v; PIELIGHT light, specular;} PIEVERTEX;
typedef struct _pievertexf {float x, y, z, u, v; PIELIGHT light, specular;} PIEVERTEXF;

typedef struct {Sint16 x, y, w, h;} PIERECT; //screen rectangle
typedef struct {Sint32 texPage; Sint16 tu, tv, tw, th;} PIEIMAGE; //an area of texture
typedef struct {Uint32 pieFlag; PIELIGHT colour, specular; Uint8 light, trans, scale, height;} PIESTYLE; //render style for pie draw functions

typedef struct _piepoly {
	Uint32 flags;
	Sint32 nVrts;
	PIEVERTEXF *pVrts;
	iTexAnim *pTexAnim;
} PIEPOLY;

#endif
