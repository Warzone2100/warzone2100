/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

//*************************************************************************
//
// screen surface structure
//
//*************************************************************************
typedef struct { int32_t left, top, right, bottom; } iClip;

typedef struct _iSurface {
	int xcentre;
	int ycentre;
	iClip clip;

	int width;
	int height;
} iSurface;

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

typedef struct {
	uint32_t flags;
	int32_t zcentre;
	unsigned int npnts;
	Vector3f normal;
	VERTEXID *pindex;
	Vector2f *texCoord;
	Vector2f texAnim;
} iIMDPoly;

typedef struct _iIMDShape {
	unsigned int flags;	
	int texpage;
	int tcmaskpage;
	int sradius, radius;
	Vector3f min, max;

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


//*************************************************************************
//
// immitmap image structures
//
//*************************************************************************

typedef struct {
	unsigned int TPageID;   /**< Which associated file to read our info from */
	unsigned int Tu;        /**< First vertex coordinate */
	unsigned int Tv;        /**< Second vertex coordinate */
	unsigned int Width;     /**< Width of image */
	unsigned int Height;    /**< Height of image */
	int XOffset;            /**< X offset into source position */
	int YOffset;            /**< Y offset into source position */
} IMAGEDEF;

#define MAX_NUM_TPAGEIDS 16
typedef struct {
	int NumImages;          /**< Number of images contained here */
	int TPageIDs[MAX_NUM_TPAGEIDS];	/**< OpenGL Texture IDs */
	IMAGEDEF *ImageDefs;    /**< Stored images */
} IMAGEFILE;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _ivisdef_h
