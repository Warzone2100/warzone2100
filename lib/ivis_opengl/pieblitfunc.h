/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
 * pieBlitFunc.h
 *
 * patch for exisitng ivis rectangle draw functions.
 *
 */
/***************************************************************************/

#ifndef _pieBlitFunc_h
#define _pieBlitFunc_h

/***************************************************************************/

#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"
#include "piedef.h"

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/
#define NUM_BACKDROPS 7

#define BACKDROP_WIDTH	640
#define BACKDROP_HEIGHT	480

/* These are Qamly's hacks used for map previews. They need to be power of
 * two sized until we clean up this mess properly. */
#define BACKDROP_HACK_WIDTH 512
#define BACKDROP_HACK_HEIGHT 512

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
extern void iV_Line(int x0, int y0, int x1, int y1, PIELIGHT colour);
extern void iV_Box(int x0,int y0, int x1, int y1, PIELIGHT colour);
extern void pie_BoxFill(int x0,int y0, int x1, int y1, PIELIGHT colour);
extern void iV_DrawImage(IMAGEFILE *ImageFile, UWORD ID, int x, int y);
void iV_DrawImageTc(IMAGEFILE *imageFile, unsigned id, unsigned idTc, int x, int y, PIELIGHT colour);
extern void iV_DrawImageRect(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Width, int Height);
void iV_DrawImageScaled(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int w, int h);

extern void iV_TransBoxFill(float x0, float y0, float x1, float y1);
extern void pie_UniTransBoxFill(float x0, float y0, float x1, float y1, PIELIGHT colour);

extern bool pie_InitRadar(void);
extern bool pie_ShutdownRadar(void);
extern void pie_DownLoadRadar(UDWORD *buffer, int width, int height, bool filter);
extern void pie_RenderRadar(int x, int y, int width, int height);

extern void pie_UploadDisplayBuffer(void);

enum SCREENTYPE
{
	SCREEN_RANDOMBDROP,
	SCREEN_CREDITS,
	SCREEN_MISSIONEND,
};

extern void pie_LoadBackDrop(SCREENTYPE screenType);

#endif //
