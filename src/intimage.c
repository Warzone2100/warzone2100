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
/*
 * Image.c
 *
 * Image definitions and related functions.
 *
 */

#include <stdio.h>

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/widget/widget.h"

#include "objects.h"
#include "loop.h"
#include "map.h"
/* Includes direct access to render library */
#include "lib/ivis_common/ivisdef.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/piemode.h"

// FIXME Direct iVis implementation include!
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/bitimage.h"

#include "display3d.h"
#include "edit3d.h"
#include "structure.h"
#include "research.h"
#include "function.h"
#include "lib/gamelib/gtime.h"
#include "hci.h"
#include "stats.h"
#include "game.h"
#include "power.h"
#include "lib/sound/audio.h"
#include "lib/widget/widgint.h"
#include "lib/widget/bar.h"
#include "lib/widget/form.h"
#include "lib/widget/label.h"
#include "lib/widget/button.h"
#include "lib/widget/editbox.h"
#include "lib/widget/slider.h"
#include "lib/framework/fractions.h"
#include "order.h"
#include "main.h"

#include "intimage.h"

#define TRANSRECT

IMAGEFILE *IntImages;	// All the 2d graphics for the user interface.

// Form frame definitions.
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
	{{FR_FRAME,	0,1, 0,-1 ,190},
	{FR_IGNORE, 0,0, 0,0 ,0},
	{FR_IGNORE, 0,0, 0,0 ,0},
	{FR_IGNORE, 0,0, 0,0 ,0},
	{FR_IGNORE, 0,0, 0,0 ,0}},
};

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
TABDEF SystemTab = {
	IMAGE_DES_WEAPONS,
	IMAGE_DES_WEAPONSDOWN,
	IMAGE_DES_EXTRAHI,
	IMAGE_DES_WEAPONSDOWN,

	/*IMAGE_TAB1,
	IMAGE_TAB1DOWN,
	IMAGE_TABHILIGHT,
	IMAGE_TABSELECTED,*/
	IMAGE_SIDETAB,
	IMAGE_SIDETABDOWN,
	IMAGE_SIDETABHI,
	IMAGE_SIDETABSEL,
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
BOOL imageInitBitmaps(void)
{
	IntImages = (IMAGEFILE*)resGetData("IMG", "intfac.img");

	return TRUE;
}

void RenderWindowFrame(IMAGEFRAME *Frame,UDWORD x,UDWORD y,UDWORD Width,UDWORD Height)
{
	RenderWindow(Frame,x,y,Width,Height,FALSE);
}

void RenderOpaqueWindow(IMAGEFRAME *Frame,UDWORD x,UDWORD y,UDWORD Width,UDWORD Height)
{
	RenderWindow(Frame,x,y,Width,Height,TRUE);
}


#define INCEND	(0)


// Render a window frame.
//
void RenderWindow(IMAGEFRAME *Frame,UDWORD x,UDWORD y,UDWORD Width,UDWORD Height,BOOL Opaque)
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
	BOOL Masked = FALSE;

	x += Frame->OffsetX0;
	y += Frame->OffsetY0;
	Width -= Frame->OffsetX1+Frame->OffsetX0;
	Height -= Frame->OffsetY1+Frame->OffsetY0;

	for(RectI=0; RectI<5; RectI++) {
		Rect = &Frame->FRect[RectI];

		switch(Rect->Type) {
			case FR_FRAME:
				if(Opaque==FALSE)
				{
					if(Masked == FALSE) {
						Width &= 0xfffc;	// Software transboxfill needs to be a multiple of 4 pixels.
						Masked = TRUE;
					}


					iV_TransBoxFill( x+Rect->TLXOffset,
									y+Rect->TLYOffset,
									x+Width-INCEND+Rect->BRXOffset,
									y+Height-INCEND+Rect->BRYOffset);
				}
				else
				{

					pie_BoxFillIndex( x+Rect->TLXOffset,
								y+Rect->TLYOffset,
								x+Width-INCEND+Rect->BRXOffset,
								y+Height-INCEND+Rect->BRYOffset,Rect->ColourIndex);

				}
				break;

			case FR_LEFT:
				if(Opaque==FALSE) {
					if(Masked == FALSE) {
						Width &= 0xfffc;	// Software transboxfill needs to be a multiple of 4 pixels.
						Masked = TRUE;
					}
					iV_TransBoxFill( x+Rect->TLXOffset,
									y+Rect->TLYOffset,
									x+Rect->BRXOffset,
									y+Height-INCEND+Rect->BRYOffset);
				} else {
					pie_BoxFillIndex(x + Rect->TLXOffset, y + Rect->TLYOffset, x + Rect->BRXOffset,
								y+Height-INCEND+Rect->BRYOffset,Rect->ColourIndex);
				}
				break;

			case FR_RIGHT:
				if(Opaque==FALSE) {
					if(Masked == FALSE) {
						Width &= 0xfffc;	// Software transboxfill needs to be a multiple of 4 pixels.
						Masked = TRUE;
					}
					iV_TransBoxFill( x+Width-INCEND+Rect->TLXOffset,
									y+Rect->TLYOffset,
									x+Width-INCEND+Rect->BRXOffset,
									y+Height-INCEND+Rect->BRYOffset);
				} else {
					pie_BoxFillIndex(x + Width-INCEND + Rect->TLXOffset, y + Rect->TLYOffset,
								x+Width-INCEND+Rect->BRXOffset,
								y+Height-INCEND+Rect->BRYOffset,Rect->ColourIndex);
				}
				break;

			case FR_TOP:
				if(Opaque==FALSE) {
					if(Masked == FALSE) {
						Width &= 0xfffc;	// Software transboxfill needs to be a multiple of 4 pixels.
						Masked = TRUE;
					}
					iV_TransBoxFill( x+Rect->TLXOffset,
									y+Rect->TLYOffset,
									x+Width-INCEND+Rect->BRXOffset,
									y+Rect->BRYOffset);
				} else {
					pie_BoxFillIndex(x + Rect->TLXOffset, y + Rect->TLYOffset, x + Width-INCEND+Rect->BRXOffset,
								y+Rect->BRYOffset,Rect->ColourIndex);
				}
				break;

			case FR_BOTTOM:
				if(Opaque==FALSE)
				{
					if(Masked == FALSE)
					{
						Width &= 0xfffc;	// Software transboxfill needs to be a multiple of 4 pixels.
						Masked = TRUE;
					}
					iV_TransBoxFill( x+Rect->TLXOffset,
									y+Height-INCEND+Rect->TLYOffset,
									x+Width-INCEND+Rect->BRXOffset,
									y+Height-INCEND+Rect->BRYOffset);
				} else {
					pie_BoxFillIndex(x + Rect->TLXOffset, y + Height-INCEND+Rect->TLYOffset,
						   		x+Width-INCEND+Rect->BRXOffset,
						   		y+Height-INCEND+Rect->BRYOffset,Rect->ColourIndex);
				}
				break;
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
