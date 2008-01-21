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
#ifndef __INCLUDED_INTIMAGE__
#define __INCLUDED_INTIMAGE__

#include "intfac.h" // Interface image id's.

#define FILLRED 16
#define FILLGREEN 16
#define FILLBLUE 128
#define FILLTRANS 128

// Sprite image structure.
typedef struct {
	UDWORD BMPNum;	//< Source bitmap index.
	UDWORD Offset;	//< byte offset within source bitmap.
	UDWORD Width;	//< Width of image.
	UDWORD Height;	//< Height of image.
	UDWORD Modulus;	//< Width of source bitmap.
	SDWORD XOffset;	//< X offset for drawing.
	SDWORD YOffset;	//< Y offset for drawing.
} IMAGE;

/** Frame type */
typedef enum {
	FRAME_NORMAL, FRAME_RADAR
} FRAMETYPE;

typedef struct {
	SWORD MajorUp;			//< Index of image to use for tab not pressed.
	SWORD MajorDown;		//< Index of image to use for tab pressed.
	SWORD MajorHilight;		//< Index of image to use for tab hilighted by mouse.
	SWORD MajorSelected;		//< Index of image to use for tab selected (same as pressed).

	SWORD MinorUp;			//< As above but for minor tabs.
	SWORD MinorDown;
	SWORD MinorHilight;
	SWORD MinorSelected;
} TABDEF;

extern IMAGEFILE *IntImages;	//< All the 2d graphics for the user interface.

// A few useful defined tabs.
extern TABDEF StandardTab;
extern TABDEF SystemTab;
extern TABDEF SmallTab;

extern BOOL imageInitBitmaps(void);
extern void imageDeleteBitmaps(void);

/** Draws a transparent window. */
extern void RenderWindowFrame(FRAMETYPE frame, UDWORD x, UDWORD y, UDWORD Width, UDWORD Height);

/** Called by RenderWindowFrame and RenderOpaqueWindow but you can call it yourself if you want. */
extern void RenderWindow(FRAMETYPE frame, UDWORD x, UDWORD y, UDWORD Width, UDWORD Height, BOOL Opaque);

#endif
