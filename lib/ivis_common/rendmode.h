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
// vid.c 0.1 10-01-96.22-11-96
#ifndef _rendmode_h_
#define _rendmode_h_
#include "ivisdef.h"
#include "pieblitfunc.h"
#include "bitimage.h"
#include "textdraw.h"

//*************************************************************************
//patch

#define	iV_Line				pie_Line
#define	iV_Box				pie_Box
#define	iV_BoxFill			pie_BoxFillIndex
#define	iV_TransBoxFill			pie_TransBoxFill
#define	iV_DrawImage			pie_ImageFileID
#define	iV_DrawImageRect		pie_ImageFileIDTile
#define	iV_DrawTransImage		pie_ImageFileID
#define	iV_DrawTransImageRect		pie_ImageFileIDTile

//*************************************************************************
// polygon flags	b0..b7: col, b24..b31: anim index

#define PIE_COLOURKEYED			0x00000800
#define PIE_ALPHA			0x00040000
#define PIE_NO_CULL			0x00002000

//*************************************************************************

#define REND_SURFACE_UNDEFINED		0
#define REND_SURFACE_SCREEN		1
#define REND_SURFACE_USR		2

#define iV_SCREEN_WIDTH			(rendSurface.width)

//*************************************************************************

extern iSurface	rendSurface;
extern iSurface	*psRendSurface;

//*************************************************************************

extern void rend_AssignScreen(void);
extern void rend_Assign(iSurface *s);
extern void iV_RenderAssign(iSurface *s);
extern void iV_SurfaceDestroy(iSurface *s);
extern iSurface *iV_SurfaceCreate(Uint32 flags, int width, int height, int xp, int yp, Uint8 *buffer);

//*************************************************************************

extern int iV_GetDisplayWidth(void);
extern int iV_GetDisplayHeight(void);

//*************************************************************************
// vid stuff still to be cut down
//*************************************************************************

extern char* (*iV_ScreenDumpToDisk)(void);
extern void (*iV_ppBitmap)(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void (*iV_ppBitmapTrans)(iBitmap *bmp, int x, int y, int w, int h, int ow);
extern void (*iV_SetTransFilter)(UDWORD rgb,UDWORD tablenumber);
extern void (*iV_UniBitmapDepth)(int texPage, int u, int v, int srcWidth, int srcHeight,
							int x, int y, int destWidth, int destHeight, unsigned char brightness, int depth);

extern void (*iV_SetTransImds)(BOOL trans);

//mapdisplay

/* Blit a transparent rectangle to the back buffer */
extern void	iVBlitTransRect(UDWORD x0, UDWORD x1, UDWORD y0, UDWORD y1);

/* Optimised DWORD read/write to memory */
extern void	iVFBlitTransRect(UDWORD x0, UDWORD y0, UDWORD x1, UDWORD y1);

/* Possible filter colours for the transparency rectangle blit */
#define TINT_BLUE	0
#define TRANS_GREY	1
#define TRANS_BLUE	2
#define TRANS_BRITE	3
#define TINT_DEEPBLUE	4

extern void iV_DrawMousePointer(int x,int y);
extern void iV_SetMousePointer(IMAGEFILE *ImageFile,UWORD ImageID);
extern void (*iV_ppBitmapColourTrans)(iBitmap *bmp, int x, int y, int w, int h, int ow,int ColourIndex);

#endif
