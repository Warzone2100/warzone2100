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
#include "ivisdef.h"
#include "ivispatch.h"

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

#define FP12_SHIFT			12
#define FP12_MULTIPLIER			(1<<12)
#define STRETCHED_Z_SHIFT		10		//stretchs z range for (1000 to 4000) to (8000 to 32000)
#define	MAX_Z				(32000.0f)	//raised to 32000 from 6000 when stretched
#define	INV_MAX_Z			(0.00003125f)	//1/32000
#define MIN_STRETCHED_Z			256
#define	LONG_WAY			(1<<15)
#define	LONG_TEST			(1<<14)
#define INTERFACE_DEPTH			(MAX_Z - 1.0f)
#define INTERFACE_DEPTH_3DFX		(65535)
#define INV_INTERFACE_DEPTH_3DFX	(1.0f/65535.0f)
#define BUTTON_DEPTH			(2000)		 //will be stretched to 16000

#define TEXTURE_SIZE			(256.0f)
#define INV_TEX_SIZE			(0.00390625f)



#define MAX_FILE_PATH		256
#define pie_MAX_POLY_SIZE	16

//Effects
#define pie_MAX_BRIGHT_LEVEL 255
#define pie_BRIGHT_LEVEL_200 200
#define pie_BRIGHT_LEVEL_180 180
#define pie_DROID_BRIGHT_LEVEL 192

//Render style flags for all pie draw functions
#define pie_FLAG_MASK           0xffff
#define pie_FLAT                0x1
#define pie_TRANSLUCENT         0x2
#define pie_ADDITIVE            0x4
#define pie_NO_BILINEAR         0x8
#define pie_HEIGHT_SCALED       0x10
#define pie_RAISE               0x20
#define pie_BUTTON              0x40
#define pie_SHADOW              0x80
#define pie_STATIC_SHADOW       0x100

#define pie_RAISE_SCALE			256

#define pie_MAX_POINTS			512
#define pie_MAX_POLYS			512
#define pie_MAX_POLY_VERTS		10

#define pie_FILLRED			16
#define pie_FILLGREEN			16
#define pie_FILLBLUE			128
#define pie_FILLTRANS			128

#define MAX_UB_LIGHT			((UBYTE)255)
#define MIN_UB_LIGHT			((UBYTE)0)
#define MAX_LIGHT			0xffffffff

/***************************************************************************/
/*
 *	Global Definitions (MACROS)
 */
/***************************************************************************/
#define pie_MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define pie_MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define pie_ABS(a)		(((a) < 0) ? (-(a)) : (a))

#define pie_ADDLIGHT(l,x)						\
(((l)->byte.r > (MAX_UB_LIGHT - (x))) ? ((l)->byte.r = MAX_UB_LIGHT) : ((l)->byte.r +=(x)));		\
(((l)->byte.g > (MAX_UB_LIGHT - (x))) ? ((l)->byte.g = MAX_UB_LIGHT) : ((l)->byte.g +=(x)));		\
(((l)->byte.b > (MAX_UB_LIGHT - (x))) ? ((l)->byte.b = MAX_UB_LIGHT) : ((l)->byte.b +=(x)));

#define pie_SUBTRACTLIGHT(l,x)						\
(((l->byte.r) < (x)) ? ((l->byte.r) = MIN_UB_LIGHT) : ((l->byte.r) -=(x)));		\
(((l->byte.g) < (x)) ? ((l->byte.g) = MIN_UB_LIGHT) : ((l->byte.g) -=(x)));		\
(((l->byte.b) < (x)) ? ((l->byte.b) = MIN_UB_LIGHT) : ((l->byte.b) -=(x)));


/***************************************************************************/
/*
 *	Global Definitions (STRUCTURES)
 */
/***************************************************************************/

#ifdef __BIG_ENDIAN__
typedef struct {UBYTE a, r, g, b;} PIELIGHTBYTES; //for byte fields in a DWORD
#else
typedef struct {UBYTE b, g, r, a;} PIELIGHTBYTES; //for byte fields in a DWORD
#endif
typedef union  {PIELIGHTBYTES byte; UDWORD argb;} PIELIGHT;
typedef struct {UBYTE r, g, b, a;} PIEVERTLIGHT;
typedef struct {SDWORD sx, sy, sz; UWORD tu, tv; PIELIGHT light, specular;} PIEVERTEX;

typedef struct {SWORD x, y, w, h;} PIERECT; //screen rectangle
typedef struct {SDWORD texPage; SWORD tu, tv, tw, th;} PIEIMAGE; //an area of texture
typedef struct {UDWORD pieFlag; PIELIGHT colour, specular; UBYTE light, trans, scale, height;} PIESTYLE; //render style for pie draw functions

typedef struct {long n; char msge[240];} iError;
typedef Sint32 fixed;

// This is the new resource loaded structure (TEXPAGE)
typedef struct
{
	iTexture *Texture;
	iPalette *Palette;
} TEXTUREPAGE;

typedef struct {
	UDWORD flags;
	SDWORD nVrts;
	PIEVERTEX *pVrts;
	iTexAnim *pTexAnim;
} PIEPOLY;


/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
extern void pie_Draw3DShape(iIMDShape *shape, int frame, int team, UDWORD colour, UDWORD specular, int pieFlag, int pieData);
extern void pie_DrawImage(PIEIMAGE *image, PIERECT *dest, PIESTYLE *style);
extern void pie_DrawImage270( PIEIMAGE *image, PIERECT *dest );

extern void pie_DrawTexTriangle(PIEVERTEX *aVrts, void* psEffects);

extern void pie_GetResetCounts(SDWORD* pPieCount, SDWORD* pTileCount, SDWORD* pPolyCount, SDWORD* pStateCount);

/*!
 * Load a PNG from file into texture
 *
 * \param fileName input file to load from
 * \param sprite Sprite to read into
 * \return TRUE on success, FALSE otherwise
 */
BOOL pie_PNGLoadFile(const char *fileName, iTexture *s);

/*!
 * Save a PNG from texture into file
 *
 * \param fileName output file to save to
 * \param sprite Texture to read from
 * \return TRUE on success, FALSE otherwise
 */
void pie_PNGSaveFile(const char *fileName, iTexture *s);

void pie_BeginLighting(float x, float y, float z);
void pie_EndLighting(void);
void pie_RemainingPasses(void);

void pie_CleanUp( void );

#endif // _piedef_h
