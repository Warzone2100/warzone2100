/*
 * Image.c
 *
 * Image definitions and related functions.
 *
 */

#include <stdio.h>
#include <math.h>

#include "Frame.h"
#include "Widget.h"

#include "Objects.h"
#include "Loop.h"
#include "Edit2D.h"
#include "Map.h"
/* Includes direct access to render library */
#include "ivisdef.h"
#include "pieState.h"

#ifdef WIN32
#include "pieMode.h"
#endif

#include "vid.h"
#include "BitImage.h"

#ifdef PSX
#include "primatives.h"
#endif

#include "Display3d.h"
#include "Edit3D.h"
#include "Disp2D.h"
#include "Structure.h"
#include "Research.h"
#include "Function.h"
#include "GTime.h"
#include "HCI.h"
#include "Stats.h"
#include "game.h"
#include "power.h"
#include "audio.h"
#include "audio_id.h"
#include "WidgInt.h"
#include "bar.h"
#include "form.h"
#include "label.h"
#include "button.h"
#include "editbox.h"
#include "slider.h"
#include "fractions.h"
#include "Order.h"
#include "WinMain.h"

#include "IntImage.h"


#define TRANSRECT

static BOOL	EnableLocks = TRUE;
static SDWORD LockRefs = 0;

IMAGEFILE *IntImages;	// All the 2d graphics for the user interface.
#ifdef PSX
//IMAGEFILE *EffectImages;
#endif


#ifdef WIN32
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
#else
// Form frame definitions.
IMAGEFRAME FrameNormal = {
	0,0, 0,0,
	0,
	0,
	0,
	0,
	0, FR_SOLID,
	0, FR_SOLID,
	0, FR_SOLID,
	0, FR_SOLID,
	{{FR_FRAME,	0,1, 0,-1 ,190},
	{FR_IGNORE, 0,0, 0,0 ,0},
	{FR_IGNORE, 0,0, 0,0 ,0},
	{FR_IGNORE, 0,0, 0,0 ,0},
	{FR_IGNORE, 0,0, 0,0 ,0}},
};
#endif

//IMAGEFRAME FrameObject = {
//	0,0, 0,0,
//	-1,
//	-1,
//	-1,
//	-1,
//	-1, FR_SOLID,
//	-1, FR_SOLID,
//	-1, FR_SOLID,
//	-1, FR_SOLID,
//	{{FR_IGNORE, 0,0, 0,0 ,0},
//	{FR_IGNORE, 0,0, 0,0 ,0},
//	{FR_IGNORE, 0,0, 0,0 ,0},
//	{FR_IGNORE, 0,0, 0,0 ,0},
//	{FR_IGNORE, 0,0, 0,0 ,0}},
//};
//
//IMAGEFRAME FrameStats = {
//	0,0, 0,0,
//	-1,
//	-1,
//	-1,
//	-1,
//	IMAGE_FRAME_HT, FR_SOLID,
//	IMAGE_FRAME_VR, FR_SOLID,
//	IMAGE_FRAME_HB, FR_SOLID,
//	IMAGE_FRAME_VL, FR_SOLID,
//	{{FR_FRAME, 8,3, -6,-5 ,190},
//	{FR_IGNORE, 0,0, 0,0 ,0},
//	{FR_IGNORE, 0,0, 0,0 ,0},
//	{FR_IGNORE, 0,0, 0,0 ,0},
//	{FR_IGNORE, 0,0, 0,0 ,0}},
//};
//
//IMAGEFRAME FrameDesignView = {
//	0,0, 0,0,
//	IMAGE_FRAME_VC0,
//	IMAGE_FRAME_VC1,
//	IMAGE_FRAME_VC2,
//	IMAGE_FRAME_VC3,
//	IMAGE_FRAME_HT2, FR_SOLID,
//	IMAGE_FRAME_VR2, FR_SOLID,
//	IMAGE_FRAME_HB2, FR_SOLID,
//	IMAGE_FRAME_VL2, FR_SOLID,
//	{{FR_FRAME, 0,0, 0,0, 1},
//	{FR_FRAME, 0,0, 0,0 ,1},
//	{FR_FRAME, 0,0, 0,0 ,1},
//	{FR_FRAME, 0,0, 0,0 ,1},
//	{FR_FRAME, 0,0, 0,0 ,1}},
//};
//
//IMAGEFRAME FrameDesignHilight = {
//	0,0, 0,0,
//	IMAGE_FRAME_HC0,
//	IMAGE_FRAME_HC1,
//	IMAGE_FRAME_HC2,
//	IMAGE_FRAME_HC3,
//	IMAGE_FRAME_HTH, FR_SOLID,
//	IMAGE_FRAME_VRH, FR_SOLID,
//	IMAGE_FRAME_HBH, FR_SOLID,
//	IMAGE_FRAME_VLH, FR_SOLID,
//	{{FR_FRAME, 0,0, 0,0, 1},
//	{FR_FRAME, 0,0, 0,0 ,1},
//	{FR_FRAME, 0,0, 0,0 ,1},
//	{FR_FRAME, 0,0, 0,0 ,1},
//	{FR_FRAME, 0,0, 0,0 ,1}},
//};
//
//IMAGEFRAME FrameText = {
//	0,0, 0,0,
//	-1,
//	-1,
//	IMAGE_FRAME_C3,
//	IMAGE_FRAME_C2,
//	-1, FR_SOLID,
//	IMAGE_FRAME_VR, FR_SOLID,
//	IMAGE_FRAME_HB, FR_SOLID,
//	IMAGE_FRAME_VL, FR_SOLID,
//	{{FR_FRAME,	0,1, 0,-1 ,224},
//	{FR_IGNORE, 0,0, 0,0 ,0},
//	{FR_IGNORE, 0,0, 0,0 ,0},
//	{FR_IGNORE, 0,0, 0,0 ,0},
//	{FR_IGNORE, 0,0, 0,0 ,0}},
//};

#ifdef PSX
// Tab definitions, defines graphics to use for major and minor tabs.
TABDEF	StandardTab = {
	IMAGE_TAB1,		  	// Major tab normal.
	IMAGE_TABSELECTED,	// Major tab clicked.
	IMAGE_TABHILIGHT,	// Major tab hilighted by mouse.
	IMAGE_TABSELECTED,	// Major tab currently selected.

	IMAGE_TAB1,			// Minor tab tab Normal.
	IMAGE_TABSELECTED,	// Minor tab clicked.
	IMAGE_TABHILIGHT,	// Minor tab hilighted by mouse.
	IMAGE_TABSELECTED,	// Minor tab currently selected.
};
TABDEF SystemTab = {
	IMAGE_DES_WEAPONS,
	IMAGE_DES_WEAPONS,
	IMAGE_DES_EXTRAHI,
	IMAGE_DES_WEAPONS,

	/*IMAGE_TAB1,
	IMAGE_TAB1DOWN,
	IMAGE_TABHILIGHT,
	IMAGE_TABSELECTED,*/
	IMAGE_SIDETAB,
	IMAGE_SIDETABDOWN,
	IMAGE_SIDETABHI,
	IMAGE_SIDETABSEL,
};
#else
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
#endif



// Read bitmaps used by the interface.
//
BOOL imageInitBitmaps(void)
{
  	IntImages = (IMAGEFILE*)resGetData("IMG","intfac.img");
#ifdef PSX
//  	EffectImages = (IMAGEFILE*)resGetData("IMG","gamefx.img");
	if(GetGameMode() == GS_NORMAL) {
		InitRadar_PSX(RADWIDTH/2,RADHEIGHT/2);
	}
#endif
//	IntImages = iV_LoadImageFile("intpc.img");

	return TRUE;
}

void imageDeleteBitmaps(void)
{
//	iV_FreeImageFile(IntImages);
}


void DrawEnableLocks(BOOL Enable)
{
	EnableLocks = Enable;
}


void DrawBegin(void)
{
#ifdef WIN32
	if(EnableLocks) {
		if(LockRefs == 0) {
			pie_LocalRenderBegin();
		}

		LockRefs++;
	}
#endif
}


void DrawEnd(void)
{
#ifdef WIN32
	if(EnableLocks) {
		LockRefs--;

		ASSERT((LockRefs >= 0,"Inbalanced DrawEnd()"));

		if(LockRefs == 0) {
			pie_LocalRenderEnd();
		}
	}
#endif
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

#ifdef PSX

// Much smaller PSX version without the legacy stuff.

void RenderBorder(UDWORD x,UDWORD y,UDWORD Width,UDWORD Height)
{
	iV_Line(x,y,x+Width,y,COL_BLACK);
	iV_Line(x+Width,y,x+Width,y+Height,COL_BLACK);
	iV_Line(x+Width,y+Height,x,y+Height,COL_BLACK);
	iV_Line(x,y+Height,x,y,COL_BLACK);
}

void RenderWindow(IMAGEFRAME *Frame,UDWORD x,UDWORD y,UDWORD Width,UDWORD Height,BOOL Opaque)
{
	RenderBorder(x,y,Width,Height);

	if(Opaque)
	{
		iV_BoxFill( x,y,x+Width,y+Height,190);
	}
	else
	{
		iV_TransBoxFill( x,y,x+Width,y+Height );
	}
}
#else

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

//		if(Opaque==FALSE) {
//			screenSetFillCacheColour(Rect->ColourIndex);
//		}

		switch(Rect->Type) {
			case FR_FRAME:
				if(Opaque==FALSE)
				{
					if(Masked == FALSE) {
						Width &= 0xfffc;	// Software transboxfill needs to be a multiple of 4 pixels.
						Masked = TRUE;
					}

#ifdef WIN32
					if (pie_GetRenderEngine() == ENGINE_GLIDE)
					{
						iV_UniTransBoxFill( x+Rect->TLXOffset,
										y+Rect->TLYOffset,
										x+Width-INCEND+Rect->BRXOffset,
										y+Height-INCEND+Rect->BRYOffset,
										(FILLRED<<16) | (FILLGREEN<<8) | FILLBLUE, FILLTRANS);
					}
					else
#endif					
					{
						iV_TransBoxFill( x+Rect->TLXOffset,
										y+Rect->TLYOffset,
										x+Width-INCEND+Rect->BRXOffset,
										y+Height-INCEND+Rect->BRYOffset);
					}
				}
				else
				{
#ifdef PSX
					iV_BoxFill( x+Rect->TLXOffset,
								y+Rect->TLYOffset,
								x+Width-INCEND+Rect->BRXOffset,
								y+Height-INCEND+Rect->BRYOffset,Rect->ColourIndex);
#else
					pie_BoxFillIndex( x+Rect->TLXOffset,
								y+Rect->TLYOffset,
								x+Width-INCEND+Rect->BRXOffset,
								y+Height-INCEND+Rect->BRYOffset,Rect->ColourIndex);
#endif
				}
				break;

			case FR_LEFT:
				if(Opaque==FALSE) {
					if(Masked == FALSE) {
						Width &= 0xfffc;	// Software transboxfill needs to be a multiple of 4 pixels.
						Masked = TRUE;
					}
#ifdef WIN32

					if (pie_GetRenderEngine() == ENGINE_GLIDE)
					{
						iV_UniTransBoxFill(x+Rect->TLXOffset,
										y+Rect->TLYOffset,
										x+Rect->BRXOffset,
										y+Height-INCEND+Rect->BRYOffset,
										(FILLRED<<16) | (FILLGREEN<<8) | FILLBLUE, FILLTRANS);
					} else 
#endif
					{
						iV_TransBoxFill( x+Rect->TLXOffset,
										y+Rect->TLYOffset,
										x+Rect->BRXOffset,
										y+Height-INCEND+Rect->BRYOffset);
					}
				} else {
					iV_BoxFill( x+Rect->TLXOffset,
								y+Rect->TLYOffset,
								x+Rect->BRXOffset,
								y+Height-INCEND+Rect->BRYOffset,Rect->ColourIndex);
				}
				break;

			case FR_RIGHT:
				if(Opaque==FALSE) {
					if(Masked == FALSE) {
						Width &= 0xfffc;	// Software transboxfill needs to be a multiple of 4 pixels.
						Masked = TRUE;
					}
#ifdef WIN32
					if (pie_GetRenderEngine() == ENGINE_GLIDE)
					{
						iV_UniTransBoxFill( x+Width-INCEND+Rect->TLXOffset,
										y+Rect->TLYOffset,
										x+Width-INCEND+Rect->BRXOffset,
										y+Height-INCEND+Rect->BRYOffset,
										(FILLRED<<16) | (FILLGREEN<<8) | FILLBLUE, FILLTRANS);
					} else 
#endif
					{
						iV_TransBoxFill( x+Width-INCEND+Rect->TLXOffset,
										y+Rect->TLYOffset,
										x+Width-INCEND+Rect->BRXOffset,
										y+Height-INCEND+Rect->BRYOffset);
					}
				} else {
					iV_BoxFill( x+Width-INCEND+Rect->TLXOffset,
								y+Rect->TLYOffset,
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
#ifdef WIN32
					if (pie_GetRenderEngine() == ENGINE_GLIDE)
					{
						iV_UniTransBoxFill( x+Rect->TLXOffset,
										y+Rect->TLYOffset,
										x+Width-INCEND+Rect->BRXOffset,
										y+Rect->BRYOffset,
										(FILLRED<<16) | (FILLGREEN<<8) | FILLBLUE, FILLTRANS);
					}else
#endif
					  {
						iV_TransBoxFill( x+Rect->TLXOffset,
										y+Rect->TLYOffset,
										x+Width-INCEND+Rect->BRXOffset,
										y+Rect->BRYOffset);
					}
				} else {
					iV_BoxFill( x+Rect->TLXOffset,
								y+Rect->TLYOffset,
								x+Width-INCEND+Rect->BRXOffset,
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
#ifdef WIN32
					if (pie_GetRenderEngine() == ENGINE_GLIDE)
					{
						iV_UniTransBoxFill( x+Rect->TLXOffset,
										y+Height-INCEND+Rect->TLYOffset,
										x+Width-INCEND+Rect->BRXOffset,
										y+Height-INCEND+Rect->BRYOffset,
										(FILLRED<<16) | (FILLGREEN<<8) | FILLBLUE, FILLTRANS);
					} 
					else
#endif
					{
						iV_TransBoxFill( x+Rect->TLXOffset,
										y+Height-INCEND+Rect->TLYOffset,
										x+Width-INCEND+Rect->BRXOffset,
										y+Height-INCEND+Rect->BRYOffset);
					}

				} else {
					iV_BoxFill( x+Rect->TLXOffset,
						   		y+Height-INCEND+Rect->TLYOffset,
						   		x+Width-INCEND+Rect->BRXOffset,
						   		y+Height-INCEND+Rect->BRYOffset,Rect->ColourIndex);
				}
				break;
		}
	}


	DrawBegin();

#ifdef PSX
	iV_SetOTIndex_PSX(iV_GetOTIndex_PSX()-1);
#endif

	if(Frame->TopLeft >= 0) {
		WTopLeft = (SWORD)iV_GetImageWidth(IntImages,Frame->TopLeft);
		HTopLeft = (SWORD)iV_GetImageHeight(IntImages,Frame->TopLeft);
		iV_DrawTransImage(IntImages,Frame->TopLeft,x,y);
	}

	if(Frame->TopRight >= 0) {
		WTopRight = (SWORD)iV_GetImageWidth(IntImages,Frame->TopRight);
		HTopRight = (SWORD)iV_GetImageHeight(IntImages,Frame->TopRight);
		iV_DrawTransImage(IntImages,Frame->TopRight,x+Width-WTopRight, y);
	}
	
	if(Frame->BottomRight >= 0) {
		WBottomRight = (SWORD)iV_GetImageWidth(IntImages,Frame->BottomRight);
		HBottomRight = (SWORD)iV_GetImageHeight(IntImages,Frame->BottomRight);
		iV_DrawTransImage(IntImages,Frame->BottomRight,x+Width-WBottomRight,y+Height-HBottomRight);
	}

	if(Frame->BottomLeft >= 0) {
		WBottomLeft = (SWORD)iV_GetImageWidth(IntImages,Frame->BottomLeft);
		HBottomLeft = (SWORD)iV_GetImageHeight(IntImages,Frame->BottomLeft);
		iV_DrawTransImage(IntImages,Frame->BottomLeft,x,y+Height-HBottomLeft);
	}

	if(Frame->TopEdge >= 0) {
		if(Frame->TopType == FR_SOLID) {
			iV_DrawImageRect(IntImages,Frame->TopEdge,
								x+iV_GetImageWidth(IntImages,Frame->TopLeft),
								y,
								0,0,
								Width-WTopLeft-WTopRight,
								iV_GetImageHeight(IntImages,Frame->TopEdge));
		} else {
			iV_DrawTransImageRect(IntImages,Frame->TopEdge,
								x+iV_GetImageWidth(IntImages,Frame->TopLeft),
								y,
								0,0,
								Width-WTopLeft-WTopRight,
								iV_GetImageHeight(IntImages,Frame->TopEdge));
		}
	}

	if(Frame->BottomEdge >= 0) {
		if(Frame->BottomType == FR_SOLID) {
			iV_DrawImageRect(IntImages,Frame->BottomEdge,
								x+WBottomLeft,
								y+Height-iV_GetImageHeight(IntImages,Frame->BottomEdge),
								0,0,
								Width-WBottomLeft-WBottomRight,
								iV_GetImageHeight(IntImages,Frame->BottomEdge));
		} else {
			iV_DrawTransImageRect(IntImages,Frame->BottomEdge,
								x+WBottomLeft,
								y+Height-iV_GetImageHeight(IntImages,Frame->BottomEdge),
								0,0,
								Width-WBottomLeft-WBottomRight,
								iV_GetImageHeight(IntImages,Frame->BottomEdge));
		}
	}

	if(Frame->LeftEdge >= 0) {
		if(Frame->LeftType == FR_SOLID) {
			iV_DrawImageRect(IntImages,Frame->LeftEdge,
								x,
								y+HTopLeft,
								0,0,
								iV_GetImageWidth(IntImages,Frame->LeftEdge),
								Height-HTopLeft-HBottomLeft);
		} else {
			iV_DrawTransImageRect(IntImages,Frame->LeftEdge,
								x,
								y+HTopLeft,
								0,0,
								iV_GetImageWidth(IntImages,Frame->LeftEdge),
								Height-HTopLeft-HBottomLeft);
		}
	}

	if(Frame->RightEdge >= 0) {
		if(Frame->RightType == FR_SOLID) {
			iV_DrawImageRect(IntImages,Frame->RightEdge,
								x+Width-iV_GetImageWidth(IntImages,Frame->RightEdge),
								y+HTopRight,
								0,0,
								iV_GetImageWidth(IntImages,Frame->RightEdge),
								Height-HTopRight-HBottomRight);
		} else {
			iV_DrawTransImageRect(IntImages,Frame->RightEdge,
								x+Width-iV_GetImageWidth(IntImages,Frame->RightEdge),
								y+HTopRight,
								0,0,
								iV_GetImageWidth(IntImages,Frame->RightEdge),
								Height-HTopRight-HBottomRight);
		}
	}

	DrawEnd();
}

#endif
