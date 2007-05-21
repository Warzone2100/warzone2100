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
 * ivisdef.h
 *
 * type defines for all ivis library functions.
 *
 */
/***************************************************************************/

#ifndef _ivisdef_h
#define _ivisdef_h

#include "lib/framework/frame.h"
#include "pietypes.h"



/***************************************************************************/
/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/
#define iV_SCANTABLE_MAX	1024

// texture animation defines
#define iV_IMD_ANIM_LOOP	0
#define iV_IMD_ANIM_RUN		1
#define iV_IMD_ANIM_FRAMES	8


//*************************************************************************
//
// screen surface structure
//
//*************************************************************************
typedef struct { Sint32 left, top, right, bottom; } iClip;

typedef struct iSurface {
	Uint32 flags;
	int xcentre;
	int ycentre;
	int xpshift;
	int ypshift;
	iClip clip;

	UBYTE *buffer;
	Sint32 scantable[iV_SCANTABLE_MAX];	// currently uses 4k per structure (!)

	int width;
	int height;
	Sint32 size;
} iSurface;

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

typedef int VERTEXID;	// Size of the entry for vertex id in the imd polygon structure

typedef struct {
	Uint32 flags;
	Sint32 zcentre;
	int npnts;
	Vector3f normal;
	VERTEXID *pindex;
	fVertex *vrt;
	iTexAnim *pTexAnim;
} iIMDPoly;

typedef struct iIMDShape {
	Sint32 texpage;
	Sint32 oradius, sradius, radius, visRadius, xmin, xmax, ymin, ymax, zmin, zmax;

	Vector3f ocen;
	UWORD numFrames;
	UWORD animInterval;
	int npoints;
	int npolys; // After BSP this number is not updated - it stays the number of pre-bsp polys
	int nconnectors; // After BSP this number is not updated - it stays the number of pre-bsp polys

	Vector3f *points;
	iIMDPoly *polys; // After BSP this is not changed - it stays the original chunk of polys - not all are now used,and others not in this array are, see BSPNode for a tree of all the post BSP polys
	Vector3f *connectors; // After BSP this is not changed - it stays the original chunk of polys - not all are now used,and others not in this array are, see BSPNode for a tree of all the post BSP polys

	int ntexanims;
	iTexAnim **texanims;

	struct iIMDShape *next; // next pie in multilevel pies (NULL for non multilevel !)

	void *shadowEdgeList;
	unsigned int nShadowEdges;
} iIMDShape;


//*************************************************************************
//
// immitmap image structures
//
//*************************************************************************

typedef struct {
	UBYTE Type[4];
	UWORD Version;
	UWORD NumImages;
	UWORD BitDepth;
	UWORD NumTPages;
	UBYTE TPageFiles[16][16];
	UBYTE PalFile[16];
} IMAGEHEADER;


typedef struct {
//	UDWORD HashValue
	UWORD TPageID;
	UWORD PalID;
	UWORD Tu,Tv;
	UWORD Width;
	UWORD Height;
	SWORD XOffset;
	SWORD YOffset;
} IMAGEDEF;


typedef struct {
	IMAGEHEADER Header;
	iTexture *TexturePages;
	UWORD NumCluts;
	UWORD TPageIDs[16];
	UWORD ClutIDs[48];
	IMAGEDEF *ImageDefs;
} IMAGEFILE;

/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

#endif // _ivisdef_h
