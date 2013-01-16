/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
/*
 * Image.c
 *
 * Image definitions and related functions.
 *
 */

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/piestate.h"

#include "intimage.h"

#define INCEND	(0)

enum {
	FR_SOLID,	//< Bitmap drawn solid.
	FR_KEYED,	//< Bitmap drawn with colour 0 transparent.
};

enum FRAMERECTTYPE
{
	FR_IGNORE,	//< Fill rect is ignored.
	FR_FRAME,	//< Fill rect drawn relative to frame.
	FR_LEFT,	//< Fill rect drawn relative to left of frame.
	FR_RIGHT,	//< Fill rect drawn relative to right of frame.
	FR_TOP,		//< Fill rect drawn relative to top of frame.
	FR_BOTTOM,	//< Fill rect drawn relative to bottom of frame.
};

struct FRAMERECT
{
	FRAMERECTTYPE Type;		//< One of the FR_... values.
	int TLXOffset, TLYOffset;	//< Offsets for the rect fill.
	int BRXOffset, BRYOffset;
	int ColourIndex;		//< Hackish index into the WZCOLOR palette
};

// Frame definition structure.
struct IMAGEFRAME
{
	SWORD OffsetX0,OffsetY0;	//< Offset top left of frame.
	SWORD OffsetX1,OffsetY1;	//< Offset bottom right of frame.
	SWORD TopLeft;			//< Image indecies for the corners ( -1 = don't draw).
	SWORD TopRight;
	SWORD BottomLeft;
	SWORD BottomRight;
	SWORD TopEdge,TopType;		//< Image indecies for the edges ( -1 = don't draw). Type ie FR_SOLID or FR_KEYED.
	SWORD RightEdge,RightType;
	SWORD BottomEdge,BottomType;
	SWORD LeftEdge,LeftType;
	FRAMERECT FRect[5];		//< Fill rectangles.
};

IMAGEFILE *IntImages;	// All the 2d graphics for the user interface.

static const uint16_t MousePointerImageIDs[CURSOR_MAX] =
{
	IMAGE_CURSOR_DEFAULT,	// CURSOR_ARROW
	IMAGE_CURSOR_DEST,	// CURSOR_DEST
	IMAGE_CURSOR_DEFAULT,	// CURSOR_SIGHT
	IMAGE_CURSOR_DEFAULT,	// CURSOR_TARGET
	IMAGE_CURSOR_DEFAULT,	// CURSOR_LARROW
	IMAGE_CURSOR_DEFAULT,	// CURSOR_RARROW
	IMAGE_CURSOR_DEFAULT,	// CURSOR_DARROW
	IMAGE_CURSOR_DEFAULT,	// CURSOR_UARROW
	IMAGE_CURSOR_DEFAULT,	// CURSOR_DEFAULT
	IMAGE_CURSOR_DEFAULT,	// CURSOR_EDGEOFMAP
	IMAGE_CURSOR_ATTACH,	// CURSOR_ATTACH
	IMAGE_CURSOR_ATTACK,	// CURSOR_ATTACK
	IMAGE_CURSOR_BOMB,	// CURSOR_BOMB
	IMAGE_CURSOR_BRIDGE,	// CURSOR_BRIDGE
	IMAGE_CURSOR_BUILD,	// CURSOR_BUILD
	IMAGE_CURSOR_EMBARK,	// CURSOR_EMBARK
	IMAGE_CURSOR_DISEMBARK,	// CURSOR_DISEMBARK
	IMAGE_CURSOR_FIX,	// CURSOR_FIX
	IMAGE_CURSOR_GUARD,	// CURSOR_GUARD
	IMAGE_CURSOR_ECM,	// CURSOR_JAM
	IMAGE_CURSOR_LOCKON,	// CURSOR_LOCKON
	IMAGE_CURSOR_SCOUT,	// CURSOR_SCOUT
	IMAGE_CURSOR_DEFAULT,	// CURSOR_MENU
	IMAGE_CURSOR_MOVE,	// CURSOR_MOVE
	IMAGE_CURSOR_NOTPOS,	// CURSOR_NOTPOSSIBLE
	IMAGE_CURSOR_PICKUP,	// CURSOR_PICKUP
	IMAGE_CURSOR_REPAIR,	// CURSOR_SEEKREPAIR
	IMAGE_CURSOR_SELECT,	// CURSOR_SELECT
};

/** Form frame definition for normal frames. */
IMAGEFRAME FrameNormal = {
	0,0, 0,0,
	IMAGE_FRAME_C0,
	IMAGE_FRAME_C1,
	IMAGE_FRAME_C3,
	IMAGE_FRAME_C2,
	IMAGE_FRAME_HT, FR_SOLID,
	IMAGE_FRAME_VR, FR_SOLID,
	IMAGE_FRAME_HB, FR_SOLID,
	IMAGE_FRAME_VL, FR_SOLID,
	{{FR_FRAME, 0,1, 0,-1, 33},
	{FR_IGNORE, 0,0, 0,0 ,0},
	{FR_IGNORE, 0,0, 0,0 ,0},
	{FR_IGNORE, 0,0, 0,0 ,0},
	{FR_IGNORE, 0,0, 0,0 ,0}},
};

/** Form frame definition for radar frames. */
IMAGEFRAME FrameRadar = {
	0,0, 0,0,
	IMAGE_FRAME_C0,
	IMAGE_FRAME_C1,
	IMAGE_FRAME_C3,
	IMAGE_FRAME_C2,
	IMAGE_FRAME_HT, FR_SOLID,
	IMAGE_FRAME_VR, FR_SOLID,
	IMAGE_FRAME_HB, FR_SOLID,
	IMAGE_FRAME_VL, FR_SOLID,
	{{FR_IGNORE, 0,0, 0,0 ,0},
	{FR_IGNORE, 0,0, 0,0 ,0},
	{FR_IGNORE, 0,0, 0,0 ,0},
	{FR_IGNORE, 0,0, 0,0 ,0},
	{FR_IGNORE, 0,0, 0,0 ,0}},
};

// Tab definitions, defines graphics to use for major and minor tabs.
TABDEF	StandardTab = {
	IMAGE_TAB1,			// Major tab normal.
	IMAGE_TAB1DOWN,		// Major tab clicked.
	IMAGE_TABHILIGHT,	// Major tab hilighted by mouse.
	IMAGE_TABSELECTED,	// Major tab currently selected.

	IMAGE_TAB1,			// Minor tab tab Normal.
	IMAGE_TAB1DOWN,		// Minor tab clicked.
	IMAGE_TABHILIGHT,	// Minor tab hilighted by mouse.
	IMAGE_TABSELECTED,	// Minor tab currently selected.
};

TABDEF	SmallTab = {
	IMAGE_TAB1_SM,			// Major tab normal.
	IMAGE_TAB1DOWN_SM,		// Major tab clicked.
	IMAGE_TABHILIGHT_SM,	// Major tab hilighted by mouse.
	IMAGE_TAB1SELECTED_SM,	// Major tab currently selected.

	IMAGE_TAB1_SM,			// Minor tab tab Normal.
	IMAGE_TAB1DOWN_SM,		// Minor tab clicked.
	IMAGE_TABHILIGHT_SM,	// Minor tab hilighted by mouse.
	IMAGE_TAB1SELECTED_SM,	// Minor tab currently selected.
};

// Read bitmaps used by the interface.
//
bool imageInitBitmaps(void)
{
	IntImages = (IMAGEFILE*)resGetData("IMG", "intfac.img");

	return true;
}

// Render a window frame.
//
static void RenderWindow(FRAMETYPE frame, UDWORD x, UDWORD y, UDWORD Width, UDWORD Height, bool Opaque)
{
	SWORD WTopRight = 0;
	SWORD WTopLeft = 0;
	SWORD WBottomRight = 0;
	SWORD WBottomLeft = 0;
	SWORD HTopRight = 0;
	SWORD HTopLeft = 0;
	SWORD HBottomRight = 0;
	SWORD HBottomLeft = 0;
	UWORD RectI;
	FRAMERECT *Rect;
	bool Masked = false;
	IMAGEFRAME *Frame;

	if (frame == 0)
	{
		Frame = &FrameNormal;
	}
	else
	{
		Frame = &FrameRadar;
	}

	x += Frame->OffsetX0;
	y += Frame->OffsetY0;
	Width -= Frame->OffsetX1+Frame->OffsetX0;
	Height -= Frame->OffsetY1+Frame->OffsetY0;

	for(RectI=0; RectI<5; RectI++) {
		Rect = &Frame->FRect[RectI];

		switch(Rect->Type) {
			case FR_FRAME:
				if(Opaque==false)
				{
					if(Masked == false) {
						Width &= 0xfffc;	// Software transboxfill needs to be a multiple of 4 pixels.
						Masked = true;
					}


					iV_TransBoxFill( x+Rect->TLXOffset,
									y+Rect->TLYOffset,
									x+Width-INCEND+Rect->BRXOffset,
									y+Height-INCEND+Rect->BRYOffset);
				}
				else
				{

					pie_BoxFill(x + Rect->TLXOffset, y + Rect->TLYOffset, x + Width - INCEND + Rect->BRXOffset,
					            y + Height - INCEND + Rect->BRYOffset, psPalette[Rect->ColourIndex]);
				}
				break;

			case FR_LEFT:
				if(Opaque==false) {
					if(Masked == false) {
						Width &= 0xfffc;	// Software transboxfill needs to be a multiple of 4 pixels.
						Masked = true;
					}
					iV_TransBoxFill( x+Rect->TLXOffset,
									y+Rect->TLYOffset,
									x+Rect->BRXOffset,
									y+Height-INCEND+Rect->BRYOffset);
				} else {
					pie_BoxFill(x + Rect->TLXOffset, y + Rect->TLYOffset, x + Rect->BRXOffset,
					            y + Height - INCEND + Rect->BRYOffset, psPalette[Rect->ColourIndex]);
				}
				break;

			case FR_RIGHT:
				if(Opaque==false) {
					if(Masked == false) {
						Width &= 0xfffc;	// Software transboxfill needs to be a multiple of 4 pixels.
						Masked = true;
					}
					iV_TransBoxFill( x+Width-INCEND+Rect->TLXOffset,
									y+Rect->TLYOffset,
									x+Width-INCEND+Rect->BRXOffset,
									y+Height-INCEND+Rect->BRYOffset);
				} else {
					pie_BoxFill(x + Width - INCEND + Rect->TLXOffset, y + Rect->TLYOffset,
					            x + Width - INCEND + Rect->BRXOffset, y + Height - INCEND + Rect->BRYOffset,
					            psPalette[Rect->ColourIndex]);
				}
				break;

			case FR_TOP:
				if(Opaque==false) {
					if(Masked == false) {
						Width &= 0xfffc;	// Software transboxfill needs to be a multiple of 4 pixels.
						Masked = true;
					}
					iV_TransBoxFill( x+Rect->TLXOffset,
									y+Rect->TLYOffset,
									x+Width-INCEND+Rect->BRXOffset,
									y+Rect->BRYOffset);
				} else {
					pie_BoxFill(x + Rect->TLXOffset, y + Rect->TLYOffset, x + Width - INCEND + Rect->BRXOffset,
					            y + Rect->BRYOffset, psPalette[Rect->ColourIndex]);
				}
				break;

			case FR_BOTTOM:
				if(Opaque==false)
				{
					if(Masked == false)
					{
						Width &= 0xfffc;	// Software transboxfill needs to be a multiple of 4 pixels.
						Masked = true;
					}
					iV_TransBoxFill( x+Rect->TLXOffset,
									y+Height-INCEND+Rect->TLYOffset,
									x+Width-INCEND+Rect->BRXOffset,
									y+Height-INCEND+Rect->BRYOffset);
				} else {
					pie_BoxFill(x + Rect->TLXOffset, y + Height - INCEND + Rect->TLYOffset,
					            x + Width - INCEND + Rect->BRXOffset, y + Height - INCEND + Rect->BRYOffset,
					            psPalette[Rect->ColourIndex]);
				}
				break;
			case FR_IGNORE:
				break; // ignored
		}
	}

	if(Frame->TopLeft >= 0) {
		WTopLeft = (SWORD)iV_GetImageWidth(IntImages,Frame->TopLeft);
		HTopLeft = (SWORD)iV_GetImageHeight(IntImages,Frame->TopLeft);
		iV_DrawImage(IntImages,Frame->TopLeft,x,y);
	}

	if(Frame->TopRight >= 0) {
		WTopRight = (SWORD)iV_GetImageWidth(IntImages,Frame->TopRight);
		HTopRight = (SWORD)iV_GetImageHeight(IntImages,Frame->TopRight);
		iV_DrawImage(IntImages,Frame->TopRight,x+Width-WTopRight, y);
	}

	if(Frame->BottomRight >= 0) {
		WBottomRight = (SWORD)iV_GetImageWidth(IntImages,Frame->BottomRight);
		HBottomRight = (SWORD)iV_GetImageHeight(IntImages,Frame->BottomRight);
		iV_DrawImage(IntImages,Frame->BottomRight,x+Width-WBottomRight,y+Height-HBottomRight);
	}

	if(Frame->BottomLeft >= 0) {
		WBottomLeft = (SWORD)iV_GetImageWidth(IntImages,Frame->BottomLeft);
		HBottomLeft = (SWORD)iV_GetImageHeight(IntImages,Frame->BottomLeft);
		iV_DrawImage(IntImages,Frame->BottomLeft,x,y+Height-HBottomLeft);
	}

	if(Frame->TopEdge >= 0) {
		iV_DrawImageRect( IntImages, Frame->TopEdge,
							x + iV_GetImageWidth(IntImages, Frame->TopLeft), y,
							Width - WTopLeft - WTopRight, iV_GetImageHeight(IntImages, Frame->TopEdge) );
	}

	if(Frame->BottomEdge >= 0) {
		iV_DrawImageRect( IntImages, Frame->BottomEdge,
							x + WBottomLeft, y + Height - iV_GetImageHeight(IntImages, Frame->BottomEdge),
							Width - WBottomLeft - WBottomRight, iV_GetImageHeight(IntImages, Frame->BottomEdge) );
	}

	if(Frame->LeftEdge >= 0) {
		iV_DrawImageRect( IntImages, Frame->LeftEdge,
							x, y + HTopLeft,
							iV_GetImageWidth(IntImages, Frame->LeftEdge), Height - HTopLeft - HBottomLeft );
	}

	if(Frame->RightEdge >= 0) {
		iV_DrawImageRect( IntImages, Frame->RightEdge,
							x + Width - iV_GetImageWidth(IntImages, Frame->RightEdge), y + HTopRight,
							iV_GetImageWidth(IntImages, Frame->RightEdge), Height - HTopRight - HBottomRight );
	}
}

void RenderWindowFrame(FRAMETYPE frame, UDWORD x, UDWORD y, UDWORD Width, UDWORD Height)
{
	RenderWindow(frame, x, y, Width, Height, false);
}

