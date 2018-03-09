/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/piestate.h"

#include "intimage.h"

#define INCEND	(0)

enum
{
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
	SWORD OffsetX0, OffsetY0;	//< Offset top left of frame.
	SWORD OffsetX1, OffsetY1;	//< Offset bottom right of frame.
	SWORD TopLeft;			//< Image indecies for the corners ( -1 = don't draw).
	SWORD TopRight;
	SWORD BottomLeft;
	SWORD BottomRight;
	SWORD TopEdge, TopType;		//< Image indecies for the edges ( -1 = don't draw). Type ie FR_SOLID or FR_KEYED.
	SWORD RightEdge, RightType;
	SWORD BottomEdge, BottomType;
	SWORD LeftEdge, LeftType;
	FRAMERECT FRect[5];		//< Fill rectangles.
};

IMAGEFILE *IntImages;	// All the 2d graphics for the user interface.

/** Form frame definition for normal frames. */
IMAGEFRAME FrameNormal =
{
	0, 0, 0, 0,
	IMAGE_FRAME_C0,
	IMAGE_FRAME_C1,
	IMAGE_FRAME_C3,
	IMAGE_FRAME_C2,
	IMAGE_FRAME_HT, FR_SOLID,
	IMAGE_FRAME_VR, FR_SOLID,
	IMAGE_FRAME_HB, FR_SOLID,
	IMAGE_FRAME_VL, FR_SOLID,
	{	{FR_FRAME, 0, 1, 0, -1, 33},
		{FR_IGNORE, 0, 0, 0, 0 , 0},
		{FR_IGNORE, 0, 0, 0, 0 , 0},
		{FR_IGNORE, 0, 0, 0, 0 , 0},
		{FR_IGNORE, 0, 0, 0, 0 , 0}
	},
};

/** Form frame definition for radar frames. */
IMAGEFRAME FrameRadar =
{
	0, 0, 0, 0,
	IMAGE_FRAME_C0,
	IMAGE_FRAME_C1,
	IMAGE_FRAME_C3,
	IMAGE_FRAME_C2,
	IMAGE_FRAME_HT, FR_SOLID,
	IMAGE_FRAME_VR, FR_SOLID,
	IMAGE_FRAME_HB, FR_SOLID,
	IMAGE_FRAME_VL, FR_SOLID,
	{	{FR_IGNORE, 0, 0, 0, 0 , 0},
		{FR_IGNORE, 0, 0, 0, 0 , 0},
		{FR_IGNORE, 0, 0, 0, 0 , 0},
		{FR_IGNORE, 0, 0, 0, 0 , 0},
		{FR_IGNORE, 0, 0, 0, 0 , 0}
	},
};

// Read bitmaps used by the interface.
//
bool imageInitBitmaps()
{
	IntImages = (IMAGEFILE *)resGetData("IMG", "intfac.img");

	return true;
}

// Render a window frame.
//
void RenderWindowFrame(FRAMETYPE frame, UDWORD x, UDWORD y, UDWORD Width, UDWORD Height, const glm::mat4 &modelViewProjectionMatrix)
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
	const FRAMERECT *Rect;
	const IMAGEFRAME *Frame = (frame == FRAME_NORMAL) ? &FrameNormal : &FrameRadar;

	x += Frame->OffsetX0;
	y += Frame->OffsetY0;
	Width -= Frame->OffsetX1 + Frame->OffsetX0;
	Height -= Frame->OffsetY1 + Frame->OffsetY0;

	for (RectI = 0; RectI < 5; RectI++)
	{
		Rect = &Frame->FRect[RectI];

		switch (Rect->Type)
		{
		case FR_FRAME:
			iV_TransBoxFill(x + Rect->TLXOffset, y + Rect->TLYOffset,
			                x + Width - INCEND + Rect->BRXOffset, y + Height - INCEND + Rect->BRYOffset);
			break;
		case FR_LEFT:
			iV_TransBoxFill(x + Rect->TLXOffset, y + Rect->TLYOffset,
			                x + Rect->BRXOffset, y + Height - INCEND + Rect->BRYOffset);
			break;
		case FR_RIGHT:
			iV_TransBoxFill(x + Width - INCEND + Rect->TLXOffset, y + Rect->TLYOffset,
			                x + Width - INCEND + Rect->BRXOffset, y + Height - INCEND + Rect->BRYOffset);
			break;
		case FR_TOP:
			iV_TransBoxFill(x + Rect->TLXOffset, y + Rect->TLYOffset,
			                x + Width - INCEND + Rect->BRXOffset, y + Rect->BRYOffset);
			break;
		case FR_BOTTOM:
			iV_TransBoxFill(x + Rect->TLXOffset, y + Height - INCEND + Rect->TLYOffset,
			                x + Width - INCEND + Rect->BRXOffset, y + Height - INCEND + Rect->BRYOffset);
			break;
		case FR_IGNORE:
			break; // ignored
		}
	}

	if (Frame->TopLeft >= 0)
	{
		WTopLeft = (SWORD)iV_GetImageWidth(IntImages, Frame->TopLeft);
		HTopLeft = (SWORD)iV_GetImageHeight(IntImages, Frame->TopLeft);
		iV_DrawImage(IntImages, Frame->TopLeft, x, y, modelViewProjectionMatrix);
	}

	if (Frame->TopRight >= 0)
	{
		WTopRight = (SWORD)iV_GetImageWidth(IntImages, Frame->TopRight);
		HTopRight = (SWORD)iV_GetImageHeight(IntImages, Frame->TopRight);
		iV_DrawImage(IntImages, Frame->TopRight, x + Width - WTopRight, y, modelViewProjectionMatrix);
	}

	if (Frame->BottomRight >= 0)
	{
		WBottomRight = (SWORD)iV_GetImageWidth(IntImages, Frame->BottomRight);
		HBottomRight = (SWORD)iV_GetImageHeight(IntImages, Frame->BottomRight);
		iV_DrawImage(IntImages, Frame->BottomRight, x + Width - WBottomRight, y + Height - HBottomRight, modelViewProjectionMatrix);
	}

	if (Frame->BottomLeft >= 0)
	{
		WBottomLeft = (SWORD)iV_GetImageWidth(IntImages, Frame->BottomLeft);
		HBottomLeft = (SWORD)iV_GetImageHeight(IntImages, Frame->BottomLeft);
		iV_DrawImage(IntImages, Frame->BottomLeft, x, y + Height - HBottomLeft, modelViewProjectionMatrix);
	}

	if (Frame->TopEdge >= 0)
	{
		iV_DrawImageRepeatX(IntImages, Frame->TopEdge, x + iV_GetImageWidth(IntImages, Frame->TopLeft), y,
		                    Width - WTopLeft - WTopRight, modelViewProjectionMatrix);
	}

	if (Frame->BottomEdge >= 0)
	{
		iV_DrawImageRepeatX(IntImages, Frame->BottomEdge, x + WBottomLeft, y + Height - iV_GetImageHeight(IntImages, Frame->BottomEdge),
		                    Width - WBottomLeft - WBottomRight, modelViewProjectionMatrix);
	}

	if (Frame->LeftEdge >= 0)
	{
		iV_DrawImageRepeatY(IntImages, Frame->LeftEdge, x, y + HTopLeft, Height - HTopLeft - HBottomLeft, modelViewProjectionMatrix);
	}

	if (Frame->RightEdge >= 0)
	{
		iV_DrawImageRepeatY(IntImages, Frame->RightEdge, x + Width - iV_GetImageWidth(IntImages, Frame->RightEdge), y + HTopRight,
		                    Height - HTopRight - HBottomRight, modelViewProjectionMatrix);
	}
}

IntListTabWidget::IntListTabWidget(WIDGET *parent)
	: ListTabWidget(parent)
{
	tabWidget()->setHeight(15);
	tabWidget()->addStyle(TabSelectionStyle(Image(IntImages, IMAGE_TAB1),    Image(IntImages, IMAGE_TAB1DOWN),    Image(IntImages, IMAGE_TABHILIGHT),    Image(), Image(), Image(), Image(), Image(), Image(), 2));
	tabWidget()->addStyle(TabSelectionStyle(Image(IntImages, IMAGE_TAB1_SM), Image(IntImages, IMAGE_TAB1DOWN_SM), Image(IntImages, IMAGE_TABHILIGHT_SM), Image(), Image(), Image(), Image(), Image(), Image(), 2));
	tabWidget()->addStyle(TabSelectionStyle(Image(IntImages, IMAGE_TAB1_SM), Image(IntImages, IMAGE_TAB1DOWN_SM), Image(IntImages, IMAGE_TABHILIGHT_SM), Image(IntImages, IMAGE_LFTTAB), Image(IntImages, IMAGE_LFTTABD), Image(IntImages, IMAGE_LFTTABD), Image(IntImages, IMAGE_RGTTAB), Image(IntImages, IMAGE_RGTTABD), Image(IntImages, IMAGE_RGTTABD), 2));
}
