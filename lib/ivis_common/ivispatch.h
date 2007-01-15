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
 * ivispatch.h
 *
 * patches old ivis defines to new pie defines allows both definitions to run concurrently .
 *
 */
/***************************************************************************/

#ifndef _ivispatch_h
#define _ivispatch_h

#include "lib/framework/frame.h"
#include "pietypes.h"

/***************************************************************************/
/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/
#define iV_POLY_MAX_POINTS pie_MAX_POLY_SIZE

//palette definitions
#define iV_PALETTE_SIZE				PALETTE_SIZE
#define iV_PALETTE_SHADE_LEVEL		PALETTE_SHADE_LEVEL

#define iV_COL_TRANS				COL_TRANS
#define iV_COL_BLACK				COL_BLACK
#define iV_COL_BLUE					COL_BLUE
#define iV_COL_GREEN				COL_GREEN
#define iV_COL_CYAN					COL_CYAN
#define iV_COL_RED					COL_RED
#define iV_COL_MAGENTA  			COL_MAGENTA
#define iV_COL_BROWN				COL_BROWN
#define iV_COL_GREY					COL_GREY
#define iV_COL_DARKGREY				COL_DARKGREY
#define iV_COL_LIGHTBLUE			COL_LIGHTBLUE
#define iV_COL_LIGHTGREEN			COL_LIGHTGREEN
#define iV_COL_LIGHTCYAN			COL_LIGHTCYAN
#define iV_COL_LIGHTRED				COL_LIGHTRED
#define iV_COL_LIGHTMAGENTA			COL_LIGHTMAGENTA
#define iV_COL_YELLOW				COL_YELLOW
#define iV_COL_WHITE				COL_WHITE

#define _iVPALETTE					psCurrentPalette
#define iV_SHADE_TABLE				palShades

#define iV_PaletteShadeTableCreate	pal_BuildAdjustedShadeTable
#define iV_PaletteNearestColour		pal_GetNearestColour
#define	iV_PaletteAdd				pal_AddNewPalette
#include "piepalette.h"

//matrixstuff
#define iV_setGeometricOffset		pie_setGeometricOffset
#define iV_MatrixBegin				pie_MatBegin
#define iV_MatrixEnd				pie_MatEnd
#define iV_MatrixRotateX			pie_MatRotX
#define iV_MatrixRotateY			pie_MatRotY
#define iV_MatrixRotateZ			pie_MatRotZ
#define iV_TRANSLATE				pie_TRANSLATE
#define iV_SIN						SIN
#define iV_COS						COS

//#define iV_Clipping2D				pie_Set2DClip

//#define iV_PolyClipTex2D			pie_PolyClipTex2D
//#define iV_PolyClip2D				pie_PolyClip2D

//heap crash
#define iV_HeapAlloc(size) MALLOC(size)
#define iV_HeapFree(pointer,size) FREE(pointer)

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
//*************************************************************************
//
// Basic types (now defined in pieTypes.h)
//
//*************************************************************************

//*************************************************************************
//
// Simple derived types (now defined in pieTypes.h)
//
//*************************************************************************

//*************************************************************************
//
// screen surface structure
//
//*************************************************************************


/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

#endif // _ivispatch_h
