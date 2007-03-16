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
 * rendfunc.h
 *
 * render functions for base render library.
 *
 */
/***************************************************************************/

#ifndef _rendFunc_h
#define _rendFunc_h


/***************************************************************************/

#include "lib/framework/frame.h"
#include "piedef.h"

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/


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
//*************************************************************************
// functions accessed dirtectly from rendmode
//*************************************************************************
extern void	SetTransFilter(UDWORD rgb,UDWORD tablenumber);
extern void	TransBoxFill(UDWORD x0, UDWORD y0, UDWORD x1, UDWORD y1);
extern void DrawImageDef(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y);
extern void DrawSemiTransImageDef(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y,int TransRate);
extern void DrawImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
extern void DrawImageRect(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height);
extern void DrawTransImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
extern void DrawTransImageRect(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height);
extern void iV_SetMousePointer(IMAGEFILE *ImageFile,UWORD ImageID);
extern void iV_DrawMousePointer(int x,int y);
extern void ScaleBitmapRGB(char *DisplayBuffer,int Width,int Height,int ScaleR,int ScaleG,int ScaleB);


extern UDWORD iV_GetMouseFrame(void);

//*************************************************************************
// functions accessed indirectly from rendmode
//*************************************************************************
extern void (*iV_pBox)(int x0, int y0, int x1, int y1, Uint32 colour);
extern void (*iV_pBoxFill)(int x0, int y0, int x1, int y1, Uint32 colour);


//*************************************************************************
#endif // _rendFunc_h
