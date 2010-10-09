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
 * piedef.h
 *
 * type defines for all pumpkin image library functions.
 *
 */
/***************************************************************************/

#ifndef _piedef_h
#define _piedef_h

/***************************************************************************/

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include "ivisdef.h"
#include "pietypes.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

/***************************************************************************/
/*
 *	Global Definitions (CONSTANTS)
 */
/***************************************************************************/
#define STRETCHED_Z_SHIFT		10 // stretchs z range for (1000 to 4000) to (8000 to 32000)
#define MAX_Z				(32000) // raised to 32000 from 6000 when stretched
#define MIN_STRETCHED_Z			256
#define LONG_WAY			(1<<15)
#define INTERFACE_DEPTH		(MAX_Z - 1)
#define BUTTON_DEPTH		2000 // will be stretched to 16000

#define OLD_TEXTURE_SIZE_FIX 256.0f

//Render style flags for all pie draw functions
#define pie_TRANSLUCENT         0x2
#define pie_ADDITIVE            0x4
#define pie_FORCE_FOG			0x8
#define pie_HEIGHT_SCALED       0x10
#define pie_RAISE               0x20
#define pie_BUTTON              0x40
#define pie_SHADOW              0x80
#define pie_STATIC_SHADOW       0x100

#define pie_RAISE_SCALE			256

#define pie_MAX_VERTICES		768
#define pie_MAX_POLYGONS		512
#define pie_MAX_VERTICES_PER_POLYGON	6

/***************************************************************************/
/*
 *	Global Definitions (STRUCTURES)
 */
/***************************************************************************/

typedef struct { UBYTE r, g, b, a; } PIELIGHTBYTES;

/** Our basic colour type. Use whenever you want to define a colour.
 *  Set bytes separetely, and do not assume a byte order between the components. */
typedef union  { PIELIGHTBYTES byte; UDWORD rgba; UBYTE vector[4]; } PIELIGHT;

typedef struct
{
	Vector3i pos;
	float u, v;
	PIELIGHT light;
	Vector3i screen; //! Screenspace tile coordinates
} TERRAIN_VERTEX;

typedef struct {SWORD x, y, w, h;} PIERECT;				/**< Screen rectangle. */
typedef struct {SDWORD texPage; SWORD tu, tv, tw, th;} PIEIMAGE;	/**< An area of texture. */

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
extern void pie_Draw3DShape(iIMDShape *shape, int frame, int team, PIELIGHT colour, PIELIGHT specular, int pieFlag, int pieFlagData);
extern void pie_DrawImage(const PIEIMAGE *image, const PIERECT *dest);

extern void pie_GetResetCounts(unsigned int* pPieCount, unsigned int* pTileCount, unsigned int* pPolyCount, unsigned int* pStateCount);

/** Setup stencil shadows and OpenGL lighting. */
void pie_BeginLighting(const Vector3f * light, bool drawshadows);

/* Stop using stencil shadows and OpenGL lighting (if enabled). */
void pie_EndLighting(void);

void pie_RemainingPasses(void);

void pie_SetUp(void);
void pie_CleanUp(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _piedef_h
