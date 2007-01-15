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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lib/ivis_common/rendfunc.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/bug.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/ivispatch.h"
#include "lib/framework/fractions.h"
#include "lib/ivis_common/pieclip.h"

#ifndef PIETOOL

/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

UBYTE		aTransTable[256];
UBYTE		aTransTable2[256];		// 2 trans tabels so we can have 2 transparancy colours without slowdown.
UBYTE		aTransTable3[256];		// 3 trans tabels so we can have 3 transparancy colours without slowdown.
UBYTE		aTransTable4[256];		// 4 trans tabels so we can have 4 transparancy colours without slowdown.
/* Set default transparency filter to green pass */
UDWORD		transFilter = TRANS_GREY;
//static int	g_mode = REND_UNDEFINED;
static IMAGEFILE *MouseImageFile;
static UWORD MouseImageID;

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/


void	iVBlitPixelTransRect(UDWORD x0, UDWORD y0, UDWORD x1, UDWORD y1);

/* Build a transparency look up table for the interface */
static void pie_BuildTransTable(UDWORD tableNo);

// dummy prototypes for pointer build functions
void (*iV_pBox)(int x0, int y0, int x1, int y1, uint32 colour);
void (*iV_pBoxFill)(int x0, int y0, int x1, int y1, uint32 colour);

//*************************************************************************
//*************************************************************************
//*************************************************************************
//*************************************************************************
/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

//*************************************************************************

void	SetTransFilter(UDWORD rgb,UDWORD tablenumber)
{
	transFilter = rgb;
	pie_BuildTransTable(tablenumber);			/* Need to recalculate the transparency table */
}

//*************************************************************************

void	TransBoxFill(UDWORD x0, UDWORD y0, UDWORD x1, UDWORD y1)
{
	UDWORD	*screen;
	UDWORD	fourPixels;
	UDWORD	output;
	UDWORD	width;
	UDWORD	i,j;

	/* Note x1 must be greater than x0 */
	width = x1-x0;

	/* Not worth it, so use pixel version */
	if( (width) < 8)
	{
		iVBlitPixelTransRect(x0,y0,x1,y1);
	}
	else
	{
	 	/* Get a handle on the current back buffer */

		for(i=y0; i<y1; i++)
		{
			screen = (UDWORD *)(psRendSurface->buffer+psRendSurface->scantable[i] + x0);

			/* We need to go through width/4 times */
			for (j=x0; j<x1; j+=4)
			{
				fourPixels = *screen;
				output = (UDWORD)aTransTable[(UBYTE)((fourPixels & 0xff000000)>>24)];
				output = output<<8;
				output = output ^ (UDWORD)aTransTable[(UBYTE)((fourPixels & 0x00ff0000)>>16)];
				output = output<<8;
				output = output ^ (UDWORD)aTransTable[(UBYTE)((fourPixels & 0x0000ff00)>>8)];
				output = output<<8;
				output = output ^ (UDWORD)aTransTable[(UBYTE)(fourPixels & 0x000000ff)];
				*screen++ = output;
			}
		}
	}
}

//*************************************************************************

void DrawImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y)
{
	IMAGEDEF *Image;
	iBitmap *Bmp;
	UDWORD Modulus;

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

	Modulus = ImageFile->TexturePages[Image->TPageID].width;
	Bmp = ImageFile->TexturePages[Image->TPageID].bmp;

	Bmp += ((UDWORD)Image->Tu) + ((UDWORD)Image->Tv) * Modulus;

	iV_ppBitmap(Bmp,
			   	x+Image->XOffset,y+Image->YOffset,
			   	Image->Width,Image->Height,Modulus);
}

//*************************************************************************

void DrawImageRect(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height)
{
	UDWORD RepWidth = 1;
	UDWORD RepWidthRemain = 0;
	UDWORD RepHeight = 1;
	UDWORD RepHeightRemain = 0;
	BOOL Tiled = FALSE;
	UDWORD w,h;
	UDWORD tx,ty;

	IMAGEDEF *Image;
	iBitmap *Bmp;
	UDWORD Modulus;

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

	Modulus = ImageFile->TexturePages[Image->TPageID].width;
	Bmp = ImageFile->TexturePages[Image->TPageID].bmp;

	Bmp += ((UDWORD)Image->Tu) + ((UDWORD)Image->Tv) * Modulus;

	if(Width + x0 > Image->Width) {
		RepWidth = Width / Image->Width;
		RepWidthRemain = Width - RepWidth * Image->Width;
		Width = Image->Width;
		Tiled = TRUE;
	}

	if(Height+ y0 > Image->Height) {
		RepHeight = Height / Image->Height;
		RepHeightRemain = Height - RepHeight * Image->Height;
		Height = Image->Height;
		Tiled = TRUE;
	}


	if(Tiled) {
		ty = y;
		for(h=0; h<RepHeight; h++) {
			tx = x;
			for(w=0; w<RepWidth; w++) {
				iV_ppBitmap(Bmp,
							tx+Image->XOffset,ty+Image->YOffset,
							Width,Height,Modulus);
				tx += Width;
			}
			if(RepWidthRemain) {
				iV_ppBitmap(Bmp,
							tx+Image->XOffset,ty+Image->YOffset,
							RepWidthRemain,Height,Modulus);
			}
			ty += Height;
		}

		if(RepHeightRemain) {
			tx = x;
			for(w=0; w<RepWidth; w++) {
				iV_ppBitmap(Bmp,
							tx+Image->XOffset,ty+Image->YOffset,
							Width,RepHeightRemain,Modulus);
				tx += Width;
			}
			if(RepWidthRemain) {
				iV_ppBitmap(Bmp,
							tx+Image->XOffset,ty+Image->YOffset,
							RepWidthRemain,RepHeightRemain,Modulus);
			}
		}
	} else {
		iV_ppBitmap(Bmp,
					x+Image->XOffset,y+Image->YOffset,
					Width,Height,Modulus);
	}
}

//*************************************************************************

void DrawTransImage(IMAGEFILE *ImageFile,UWORD ID,int x,int y)
{
	IMAGEDEF *Image;
	iBitmap *Bmp;
	UDWORD Modulus;

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

	Modulus = ImageFile->TexturePages[Image->TPageID].width;
	Bmp = ImageFile->TexturePages[Image->TPageID].bmp;

	Bmp += ((UDWORD)Image->Tu) + ((UDWORD)Image->Tv) * Modulus;

	iV_ppBitmapTrans(Bmp,x+Image->XOffset,y+Image->YOffset,
			   	Image->Width,Image->Height,Modulus);
}

//*************************************************************************

void DrawTransImageRect(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height)
{
	UDWORD RepWidth = 1;
	UDWORD RepWidthRemain = 0;
	UDWORD RepHeight = 1;
	UDWORD RepHeightRemain = 0;
	BOOL Tiled = FALSE;
	UDWORD w,h;
	UDWORD tx,ty;

	IMAGEDEF *Image;
	iBitmap *Bmp;
	UDWORD Modulus;

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

	Modulus = ImageFile->TexturePages[Image->TPageID].width;
	Bmp = ImageFile->TexturePages[Image->TPageID].bmp;

	Bmp += ((UDWORD)Image->Tu) + ((UDWORD)Image->Tv) * Modulus;

	if(Width + x0 > Image->Width) {
		RepWidth = Width / Image->Width;
		RepWidthRemain = Width - RepWidth * Image->Width;
		Width = Image->Width;
		Tiled = TRUE;
	}

	if(Height+ y0 > Image->Height) {
		RepHeight = Height / Image->Height;
		RepHeightRemain = Height - RepHeight * Image->Height;
		Height = Image->Height;
		Tiled = TRUE;
	}

	if(Tiled) {
		ty = y;
		for(h=0; h<RepHeight; h++) {
			tx = x;
			for(w=0; w<RepWidth; w++) {
				iV_ppBitmapTrans(Bmp,

							tx+Image->XOffset,ty+Image->YOffset,
							Width,Height,Modulus);
				tx += Width;
			}
			if(RepWidthRemain) {
				iV_ppBitmapTrans(Bmp,
							tx+Image->XOffset,ty+Image->YOffset,
							RepWidthRemain,Height,Modulus);
			}
			ty += Height;
		}

		if(RepHeightRemain) {
			tx = x;
			for(w=0; w<RepWidth; w++) {
				iV_ppBitmapTrans(Bmp,
							tx+Image->XOffset,ty+Image->YOffset,
							Width,RepHeightRemain,Modulus);
				tx += Width;
			}
			if(RepWidthRemain) {
				iV_ppBitmapTrans(Bmp,
							tx+Image->XOffset,ty+Image->YOffset,
							RepWidthRemain,RepHeightRemain,Modulus);
			}
		}
	} else {
		iV_ppBitmapTrans(Bmp,
						x+Image->XOffset,y+Image->YOffset,
						Width,Height,Modulus);
	}
}


//*************************************************************************

void iV_SetMousePointer(IMAGEFILE *ImageFile,UWORD ImageID)
{
	ASSERT( ImageID < ImageFile->Header.NumImages,"iV_SetMousePointer : Invalid image id" );

	MouseImageFile = ImageFile;
	MouseImageID = ImageID;
}


UDWORD iV_GetMouseFrame(void)
{
	return MouseImageID;
}



void iV_DrawMousePointer(int x,int y)
{
	iV_DrawImage(MouseImageFile,MouseImageID,x,y);
}


//*************************************************************************

// Upload the current display back buffer into system memory.
//
void UploadDisplayBuffer(char *DisplayBuffer)
{

	UDWORD *Source = (UDWORD*) rendSurface.buffer;
	UDWORD *Dest = (UDWORD*)DisplayBuffer;
	UDWORD Size = rendSurface.size / 4;
	UDWORD i;

	for(i=0; i<Size; i++) {
		*Dest = *Source;
		Source++;
		Dest++;
	}

}


//*************************************************************************

void	DownloadDisplayBuffer(char *DisplayBuffer)
{
	UDWORD *Source = (UDWORD*)DisplayBuffer;
	UDWORD *Dest = (UDWORD*) rendSurface.buffer;
	UDWORD depth,width,modulo,drop;
	UDWORD	srcWidth,srcDepth;
	UDWORD	i,j;

	depth = pie_GetVideoBufferHeight();
	width = pie_GetVideoBufferWidth();

//always full screen
	modulo = 0;//width - BACKDROP_WIDTH;

	drop = 0;//(depth - BACKDROP_HEIGHT)/2;

	Dest += ((drop*width) + (modulo))/4;		// dealing with dwords

//always full screen
	srcDepth = depth;//BACKDROP_HEIGHT;
	srcWidth = width;//BACKDROP_WIDTH;
	for(i=0; i<srcDepth; i++)
	{
		for(j=0; j<srcWidth/4; j++)
		{
			*Dest = *Source;
			Source++;
			Dest++;
		}
		Dest+=(modulo/4);		// dest is in dwords ... !
	}
}


//*************************************************************************
//
// local functions
//
//*************************************************************************

void	iVBlitPixelTransRect(UDWORD x0, UDWORD y0, UDWORD x1, UDWORD y1)
{

UBYTE	*screen;
UBYTE	present;
UDWORD	i,j;
 	/* Do a horizontal strip at a time. We're going x inside y to take advantage of the scan table */
	for (i=y0; i<y1; i++)
	{
		/* Get this outside loop cos it's invariant to each scan line */
		screen = (UBYTE *)psRendSurface->buffer + psRendSurface->scantable[i] + x0;

		for(j=x0; j<x1; j++)
		{
   	   		/* What colour is there at the moment? */
			present =  *screen;
  			/* Write in the new improved greener version */
			*screen++ = aTransTable[present];
		}
	}

}

//*************************************************************************

static void pie_BuildTransTable(UDWORD tableNo)
{
UDWORD	i;
UBYTE	red = 0, green = 0, blue = 0;
iColour* psPalette = pie_GetGamePal();

	// Step through all the palette entries for the currently selected iVPALETTE
	for(i=0; i<256; i++)
	{
	 	switch (transFilter)
		{
		case TINT_BLUE:
			red = (psPalette[i].r * 5) / 8;
			blue = (psPalette[i].b * 7) / 8;
			green = (psPalette[i].g * 5) / 8;
			break;

		case TINT_DEEPBLUE:
			red = (psPalette[i].r * 3) / 8;
			blue = (psPalette[i].b * 5) / 8;
			green = (psPalette[i].g * 3) / 8;
			break;

		case TRANS_GREY:
			red = psPalette[i].r/2;
			blue = psPalette[i].b/2;
			green = psPalette[i].g/2;
			break;

		case TRANS_BLUE:
			red = psPalette[i].r/2;
			blue = psPalette[i].b;
			green = psPalette[i].g/2;
			break;

		case TRANS_BRITE:
			if( ((UDWORD)psPalette[i].r) + 50 >255)
			{
				red = 255;
			}
			else
			{
				red = (psPalette[i].r+50);
			}

			if( ((UDWORD)psPalette[i].b) + 50 >255)
			{
				blue = 255;
			}
			else
			{
				blue = (psPalette[i].b+50);
			}

			if( ((UDWORD)psPalette[i].g)+50 >255)
			{
				green = 255;
			}
			else
			{
				green = (psPalette[i].g+50);
			}

			break;

		default:
			ASSERT( FALSE,"Invalid transparency filter selection" );
			break;
		}

		if(tableNo == 0)
		{
			aTransTable[i] = pal_GetNearestColour(red,green,blue);
		}
		else if(tableNo == 1)
		{
			aTransTable2[i] = pal_GetNearestColour(red,green,blue);
		}
		else if(tableNo == 2)
		{
			aTransTable3[i] = pal_GetNearestColour(red,green,blue);
		}
		else
		{
			aTransTable4[i] = pal_GetNearestColour(red,green,blue);
		}
	}

}

#endif
