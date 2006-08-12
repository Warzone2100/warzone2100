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
void	pie_BuildTransTable(UDWORD tableNo);

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
//*** line plot 2D line - clipped
//*
//* params	x0,y0  = line point 1
//*	 		x1,y1  = line point 2
//*	 		colour = line colour value
//*****

void line(int x0, int y0, int x1, int y1, uint32 colour)

{
	int code1, code2, code;
	int x = 0, y = 0;


	code1 = code2 = 0;


	if (y0>psRendSurface->clip.bottom)   	code1 = 1;
	else if (y0<psRendSurface->clip.top) 	code1 = 2;
	if (x0>psRendSurface->clip.right)	code1 |= 4;
	else if (x0<psRendSurface->clip.left)	code1 |= 8;
	if (y1>psRendSurface->clip.bottom)	code2 = 1;
	else if (y1<psRendSurface->clip.top) 	code2 = 2;
	if (x1>psRendSurface->clip.right)	code2 |= 4;
	else if (x1<psRendSurface->clip.left)	code2 |= 8;

	// line totally outside screen: reject

	if ((code1 & code2) != 0)
		return;

	// line totally inside screen: accept

	if ((code1 | code2) == 0) {
		iV_pLine(x0,y0,x1,y1,colour);
		return;
	}

	// at least one point needs clipping

	code = (code1 != 0) ? code1 : code2;

	if (code & 1) {
		x = x0 + (x1-x0) * (psRendSurface->clip.bottom-y0)/(y1-y0);
		y = psRendSurface->clip.bottom;
	} else if (code & 2) {
		x = x0 + (x1-x0) * (psRendSurface->clip.top-y0)/(y1-y0);
		y = psRendSurface->clip.top;
	} else if (code & 4) {
		y = y0 + (y1-y0) * (psRendSurface->clip.right-x0)/(x1-x0);
		x = psRendSurface->clip.right;
	} else if (code & 8) {
		y = y0 + (y1-y0) * (psRendSurface->clip.left-x0)/(x1-x0);
		x = psRendSurface->clip.left;
	}


	if (code == code1) {
		x0 = x;
		y0 = y;
	} else {
		x1 = x;
		y1 = y;
	}

	code1 = code2 = 0;


	if (y0>psRendSurface->clip.bottom)		code1 = 1;
	else if (y0<psRendSurface->clip.top)	code1 = 2;
	if (x0>psRendSurface->clip.right)  	code1 |= 4;
	else if (x0<psRendSurface->clip.left)	code1 |= 8;
	if (y1>psRendSurface->clip.bottom)		code2 = 1;
	else if (y1<psRendSurface->clip.top)	code2 = 2;
	if (x1>psRendSurface->clip.right)		code2 |= 4;
	else if (x1<psRendSurface->clip.left)	code2 |= 8;

	if ((code1 & code2) != 0)
		return;

	if ((code1 | code2) == 0) {
		iV_pLine(x0,y0,x1,y1,colour);
		return;
	}

	code = (code1 != 0) ? code1 : code2;

	if (code & 1) {
		x = x0 + (x1-x0) * (psRendSurface->clip.bottom-y0)/(y1-y0);
		y = psRendSurface->clip.bottom;
	} else if (code & 2) {
		x = x0 + (x1-x0) * (psRendSurface->clip.top-y0)/(y1-y0);
		y = psRendSurface->clip.top;
	} else if (code & 4) {
		y = y0 + (y1-y0) * (psRendSurface->clip.right-x0)/(x1-x0);
		x = psRendSurface->clip.right;
	} else if (code & 8) {
		y = y0 + (y1-y0) * (psRendSurface->clip.left-x0)/(x1-x0);
		x = psRendSurface->clip.left;
	}

	if (code == code1)
		iV_pLine(x,y,x1,y1,colour);
	else
		iV_pLine(x0,y0,x,y,colour);
}

//*************************************************************************

void box(int x0, int y0, int x1, int y1, uint32 colour)
{

	if (x0>psRendSurface->clip.right || x1<psRendSurface->clip.left ||
			y0>psRendSurface->clip.bottom || y1<psRendSurface->clip.top)
		return;

	if (x0<psRendSurface->clip.left)
		x0 = psRendSurface->clip.left;
	if (x1>psRendSurface->clip.right)
		x1 = psRendSurface->clip.right;
	if (y0<psRendSurface->clip.top)
		y0 = psRendSurface->clip.top;
	if (y1>psRendSurface->clip.bottom)
		y1 = psRendSurface->clip.bottom;

	iV_pBox(x0,y0,x1,y1,colour);

}

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

void DrawImageDef(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y)
{
	Bmp += ((UDWORD)Image->Tu) + ((UDWORD)Image->Tv) * Modulus;

	iV_ppBitmap(Bmp,
			   	x+Image->XOffset,y+Image->YOffset,
			   	Image->Width,Image->Height,Modulus);
}


void DrawSemiTransImageDef(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y,int TransRate)
{
	Bmp += ((UDWORD)Image->Tu) + ((UDWORD)Image->Tv) * Modulus;

	iV_ppBitmapTrans(Bmp,
			   	x+Image->XOffset,y+Image->YOffset,
			   	Image->Width,Image->Height,Modulus);
}


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

void DrawTransColourImage(IMAGEFILE *ImageFile, UWORD ID, int x, int y, SWORD ColourIndex)
{
	IMAGEDEF *Image;
	iBitmap *bmp;
	UDWORD modulus;
	SDWORD width;
	SDWORD height;
	SDWORD delta;
	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

	width = Image->Width;
	height = Image->Height;
	modulus = ImageFile->TexturePages[Image->TPageID].width;
	bmp = ImageFile->TexturePages[Image->TPageID].bmp;

	bmp += ((UDWORD)Image->Tu) + ((UDWORD)Image->Tv) * modulus;

	x +=Image->XOffset;
	y +=Image->YOffset;
	//clip
		//clip
		if (x < psRendSurface->clip.left)//off left hand edge
		{
			delta = psRendSurface->clip.left - x;
			if (delta < width)
			{
				bmp += delta;
				x += delta;
				width -= delta;
			}
			else
			{
				return;
			}
		}
		if ((x + width) > (psRendSurface->clip.right + 1))
		{
			delta = (x + width) - (psRendSurface->clip.right + 1);
			if (delta < width)
			{
				width -= delta;
			}
			else
			{
				return;
			}
		}
		if (y < psRendSurface->clip.top)
		{
			delta = psRendSurface->clip.top - y;
			if (delta < height)
			{
				bmp+= delta * modulus;
				y += delta;
				height -= delta;
			}
			else
			{
				return;
			}
		}
		if ((y + height) > (psRendSurface->clip.bottom + 1))
		{
			delta = (y + height) - (psRendSurface->clip.bottom + 1);
			if (delta < height)
			{
				height -= delta;
			}
			else
			{
				return;
			}
		}

	if (ColourIndex == PIE_TEXT_WHITE) {
		iV_ppBitmapColourTrans(bmp, x, y, width, height, modulus, 255);
	} else if (ColourIndex == PIE_TEXT_LIGHTBLUE) {
		pie_RenderBlueTintedBitmap(bmp,	x, y, width, height, modulus);
	} else if (ColourIndex == PIE_TEXT_DARKBLUE) {
		pie_RenderDeepBlueTintedBitmap(bmp, x, y, width, height, modulus);
	} else {
		iV_ppBitmapColourTrans(bmp, x, y, width, height, modulus, ColourIndex);
	}
}



//*************************************************************************

void iV_SetMousePointer(IMAGEFILE *ImageFile,UWORD ImageID)
{
	ASSERT((ImageID < ImageFile->Header.NumImages,"iV_SetMousePointer : Invalid image id"));

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

// Software version does nothing.
void DownLoadRadar(unsigned char *buffer)
{
	return;
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

// Download buffer in system memory to the display back buffer.
//
/*
void DownloadDisplayBuffer(char *DisplayBuffer)
{
#ifndef PIEPSX		// was #ifndef PSX
	UDWORD *Source = (UDWORD*)DisplayBuffer;
	UDWORD *Dest = (UDWORD*) rendSurface.buffer;
	UDWORD Size = rendSurface.size / 4;
	UDWORD i;

	for(i=0; i<Size; i++) {
		*Dest = *Source;
		Source++;
		Dest++;
	}
#endif
}
 */



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
// Scale a bitmaps colours.
//
void ScaleBitmapRGB(char *DisplayBuffer,int Width,int Height,int ScaleR,int ScaleG,int ScaleB)
{
	char *Ptr = DisplayBuffer;
	UDWORD Size = Width*Height;
	UDWORD i;

	for(i=0; i<Size; i++) {
		*Ptr = palShades[(*Ptr * PALETTE_SHADE_LEVEL) + 4];	//iV_PALETTE_SHADE_LEVEL-4];
		Ptr++;
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

void	pie_BuildTransTable(UDWORD tableNo)
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
			ASSERT((FALSE,"Invalid transparency filter selection"));
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

