/***************************************************************************/
/*
 * pieBlitFunc.c
 *
 * patch for exisitng ivis rectangle draw functions.
 *
 */
/***************************************************************************/

#include "frame.h"
#include <time.h>
#include "pieBlitFunc.h"
#include "dx6TexMan.h"
#include "bug.h"
#include "piedef.h"
#include "pieMode.h"
#include "pieState.h"
#include "3dfxFunc.h"
#include "rendFunc.h"
#include "rendMode.h"
#include "texd3d.h"
#include "pcx.h"
#include "pieClip.h"
#include "pieFunc.h"
#include "pieMatrix.h"
#ifndef PIEPSX
/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/
#ifdef WIN32
UWORD	backDropBmp[BACKDROP_WIDTH * BACKDROP_HEIGHT * 2];
SDWORD gSurfaceOffsetX;
SDWORD gSurfaceOffsetY;
UWORD* pgSrcData = NULL;
SDWORD gSrcWidth;
SDWORD gSrcHeight;
SDWORD gSrcStride;
#endif

#define COLOURINTENSITY 0xffffffff
/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

PIESTYLE	rendStyle;
POINT	rectVerts[4];

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/
void pie_Line(int x0, int y0, int x1, int y1, uint32 colour)
{
	PIELIGHT light;
	iColour* psPalette;

	pie_SetRendMode(REND_FLAT);
	pie_SetColour(colour);
	pie_SetTexturePage(-1);

	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		line(x0, y0, x1, y1, colour);
		break;
	case ENGINE_GLIDE:
		gl_Line(x0, y0, x1, y1, colour);
		break;
	case ENGINE_D3D:
		/* Get our colour values from the ivis palette */
		psPalette = pie_GetGamePal();
		light.byte.r = psPalette[colour].r;
		light.byte.g = psPalette[colour].g;
		light.byte.b = psPalette[colour].b;
		light.byte.a = MAX_UB_LIGHT;
		pie_DrawLine(x0, y0, x1, y1, light.argb, TRUE);
	default:
		break;
	}
}
/***************************************************************************/

void pie_Box(int x0,int y0, int x1, int y1, uint32 colour)
{
	PIELIGHT light;
	iColour* psPalette;

	pie_SetRendMode(REND_FLAT);
	pie_SetColour(colour);
	pie_SetTexturePage(-1);

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

	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		box(x0,y0,x1,y1,colour);
		break;
	case ENGINE_GLIDE:
		gl_Line(x0, y0, x1, y0, colour);
		gl_Line(x1, y0, x1, y1, colour);
		gl_Line(x1, y1, x0, y1, colour);
		gl_Line(x0, y1, x0, y0, colour);
		break;
	case ENGINE_D3D:
		psPalette = pie_GetGamePal();
		/* Get our colour values from the ivis palette */
		light.byte.r = psPalette[colour].r;
		light.byte.g = psPalette[colour].g;
		light.byte.b = psPalette[colour].b;
		light.byte.a = MAX_UB_LIGHT;
		pie_DrawLine(x0, y0, x1, y0, light.argb, FALSE);
		pie_DrawLine(x1, y0, x1, y1, light.argb, FALSE);
		pie_DrawLine(x1, y1, x0, y1, light.argb, FALSE);
		pie_DrawLine(x0, y1, x0, y0, light.argb, FALSE);
	default:
		break;
	}

}

/***************************************************************************/

void pie_BoxFillIndex(int x0,int y0, int x1, int y1, UBYTE colour)
{
	PIELIGHT light;
	iColour* psPalette;

	pie_SetRendMode(REND_FLAT);
	pie_SetTexturePage(-1);

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

	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		iV_pBoxFill(x0,y0,x1,y1,colour);
		break;
	case ENGINE_GLIDE:
		gl_BoxFill(x0, y0, x1, y1, colour);
		break;
	case ENGINE_D3D:
		/* Get our colour values from the ivis palette */
		psPalette = pie_GetGamePal();
		light.byte.r = psPalette[colour].r;
		light.byte.g = psPalette[colour].g;
		light.byte.b = psPalette[colour].b;
		light.byte.a = MAX_UB_LIGHT;
		pie_DrawRect(x0, y0, x1, y1, light.argb, FALSE);
	default:
		break;
	}

}

void pie_BoxFill(int x0,int y0, int x1, int y1, uint32 colour)
{
	PIELIGHT light;

	pie_SetRendMode(REND_FLAT);
	pie_SetTexturePage(-1);

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

	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		light.argb = colour;
		iV_pBoxFill(x0,y0,x1,y1,pal_GetNearestColour(light.byte.r,  light.byte.g, light.byte.b));
		break;
	case ENGINE_GLIDE:
		gl_IntelTransBoxFill(x0,y0,x1,y1,colour, MAX_UB_LIGHT);
		break;
	case ENGINE_D3D:
		/* Get our colour values from the ivis palette */
		light.argb = colour;
		light.byte.a = MAX_UB_LIGHT;
		pie_DrawRect(x0, y0, x1, y1, light.argb, FALSE);
	default:
		break;
	}

}
/***************************************************************************/

void pie_TransBoxFill(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1)
{
	UDWORD rgb;
	UDWORD transparency;
	rgb = (pie_FILLRED<<16) | (pie_FILLGREEN<<8) | pie_FILLBLUE;//blue
	transparency = pie_FILLTRANS;
	pie_UniTransBoxFill(x0, y0, x1, y1, rgb, transparency);
//	pie_doWeirdBoxFX(x0,y0,x1,y1);

}

/***************************************************************************/
void pie_UniTransBoxFill(SDWORD x0,SDWORD y0, SDWORD x1, SDWORD y1, UDWORD rgb, UDWORD transparency)
{
	UDWORD light;
//  	pie_doWeirdBoxFX(x0,y0,x1,y1);
//	return;

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

	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		TransBoxFill(x0,y0,x1,y1);
		break;
	case ENGINE_GLIDE:
		if(pie_SwirlyBoxes())
		{
			pie_doWeirdBoxFX(x0,y0,x1,y1,rgb);
		}
		else
		{
			gl_IntelTransBoxFill(x0, y0, x1, y1, rgb, transparency);
		}
		break;
	case ENGINE_D3D:
		if (transparency == 0 )
		{
			transparency = 127;
		}
		pie_SetTexturePage(-1);
		pie_SetRendMode(REND_ALPHA_FLAT);
		light = (rgb & 0x00ffffff) + (transparency << 24);
		pie_DrawRect(x0, y0, x1, y1, light, FALSE);
	default:
		break;
	}
}

/***************************************************************************/

void pie_DrawImageFileID(IMAGEFILE *ImageFile,UWORD ID,int x,int y)
{
	IMAGEDEF *Image;
	iBitmap *bmp;
	UDWORD modulus;
	PIEIMAGE pieImage;
	PIERECT dest;
	SDWORD width;
	SDWORD height;
	SDWORD delta;

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		width = Image->Width;
		height = Image->Height;
		modulus = ImageFile->TexturePages[Image->TPageID].width;
		bmp = ImageFile->TexturePages[Image->TPageID].bmp;

		bmp += ((UDWORD)Image->Tu) + ((UDWORD)Image->Tv) * modulus;
		x +=Image->XOffset;
		y +=Image->YOffset;
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


		iV_ppBitmapTrans(bmp,x,y,width,height,modulus);
		break;
	case ENGINE_GLIDE:
			gl_IntelBitmap(ImageFile->TPageIDs[Image->TPageID], Image->Tu, Image->Tv, Image->Width, Image->Height, 
					x+Image->XOffset, y+Image->YOffset, Image->Width, Image->Height);
		break;
	case ENGINE_D3D:
		pieImage.texPage = ImageFile->TPageIDs[Image->TPageID];
		pieImage.tu = Image->Tu;
		pieImage.tv = Image->Tv;
		pieImage.tw = Image->Width;
		pieImage.th = Image->Height;
		dest.x = x+Image->XOffset;
		dest.y = y+Image->YOffset;
		dest.w = Image->Width;
		dest.h = Image->Height;
		pie_DrawImage(&pieImage, &dest, &rendStyle);
	default:
		break;
	}
}

BOOL	bAddSprites = FALSE;
UDWORD	addSpriteLevel;



void	pie_SetAdditiveSprites(BOOL	val)
{
	bAddSprites = val;
}

void	pie_SetAdditiveSpriteLevel(UDWORD	val)
{
	addSpriteLevel = val;
}

BOOL	pie_GetAdditiveSprites( void )
{
	return(bAddSprites);
}

void pie_ImageFileID(IMAGEFILE *ImageFile,UWORD ID,int x,int y)
{
	IMAGEDEF *Image;
	iBitmap *bmp;
	UDWORD modulus;
	PIEIMAGE pieImage;
	PIERECT dest;
	SDWORD width;
	SDWORD height;
	SDWORD delta;
	SDWORD	div,wave;

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

   	if(pie_GetAdditiveSprites())
	{
		pie_SetBilinear(TRUE);
		pie_SetRendMode(REND_ALPHA_TEX);
		pie_SetColour(addSpriteLevel);
		pie_SetColourKeyedBlack(TRUE);

	}
	else
	{
		pie_SetBilinear(FALSE);
		pie_SetRendMode(REND_GOURAUD_TEX);
		pie_SetColour(COLOURINTENSITY);
		pie_SetColourKeyedBlack(TRUE);
	}
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
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

		iV_ppBitmapTrans(bmp,x,y,width,height,modulus);
		break;
	case ENGINE_GLIDE:
	   	if(pie_WaveBlit())
	  	{
			wave = getTimeValueRange(2880,360);
			div =	96*SIN(DEG(wave));
			div = div/4096;
			div =	96+div;
		 //	gl_IntelBitmap(ImageFile->TPageIDs[Image->TPageID], Image->Tu, Image->Tv, Image->Width, Image->Height, 
		 //			x+Image->XOffset-wave/2, y+Image->YOffset-wave/2, Image->Width+wave, Image->Height+wave);
			
			gl_DrawStretchImage(ImageFile, ID, x+Image->XOffset-div/2,y+Image->YOffset , 
				Image->Width+div, Image->Height);
	  	}
	 	else
	  	{
		 	gl_IntelBitmap(ImageFile->TPageIDs[Image->TPageID], Image->Tu, Image->Tv, Image->Width, Image->Height, 
					x+Image->XOffset, y+Image->YOffset, Image->Width, Image->Height);
			//gl_DrawStretchImage(ImageFile, ID, x+Image->XOffset,y+Image->YOffset , 
			//	Image->Width, Image->Height);
	  	}

		break;
	case ENGINE_D3D:
		pieImage.texPage = ImageFile->TPageIDs[Image->TPageID];
		pieImage.tu = Image->Tu;
		pieImage.tv = Image->Tv;
		pieImage.tw = Image->Width;
		pieImage.th = Image->Height;
		dest.x = x+Image->XOffset;
		dest.y = y+Image->YOffset;
		dest.w = Image->Width;
		dest.h = Image->Height;
		pie_DrawImage(&pieImage, &dest, &rendStyle);
	default:
		break;
	}
}


/***************************************************************************/

void pie_ImageFileIDTile(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height)
{
	IMAGEDEF *Image;
	SDWORD hRep, hRemainder, vRep, vRemainder;
	PIEIMAGE pieImage;
	PIERECT dest;
	assert(ID < ImageFile->Header.NumImages);

	assert(x0 == 0);
	assert(y0 == 0);

	Image = &ImageFile->ImageDefs[ID];

	pie_SetBilinear(FALSE);
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);
	pie_SetColourKeyedBlack(TRUE);


	

	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		DrawImageRect(ImageFile, ID, x, y, x0, y0, Width, Height);
		break;
	case ENGINE_GLIDE:
		gl_DrawImageRect(ImageFile, ID, x, y, x0, y0, Width, Height);
		break;
	case ENGINE_D3D:
		pieImage.texPage = ImageFile->TPageIDs[Image->TPageID];
		pieImage.tu = Image->Tu;
		pieImage.tv = Image->Tv;
		pieImage.tw = Image->Width;
		pieImage.th = Image->Height;

		dest.x = x+Image->XOffset;
		dest.y = y+Image->YOffset;
		dest.w = Image->Width;
		dest.h = Image->Height;

		vRep = Height/Image->Height;
		vRemainder = Height - (vRep * Image->Height);

		while(vRep > 0)
		{
			hRep = Width/Image->Width;
			hRemainder = Width - (hRep * Image->Width);
			pieImage.tw = Image->Width;
			dest.x = x+Image->XOffset;
			dest.w = Image->Width;
			while(hRep > 0)
			{
				pie_DrawImage(&pieImage, &dest, &rendStyle);
				hRep --;
				dest.x += Image->Width;
			}
			//draw remainder
			if (hRemainder > 0)
			{
				pieImage.tw = hRemainder;
				dest.w = hRemainder;
				pie_DrawImage(&pieImage, &dest, &rendStyle);
			}
			vRep --;
			dest.y += Image->Height;
		}
		//draw remainder
		if (vRemainder > 0)
		{
			hRep = Width/Image->Width;
			hRemainder = Width - (hRep * Image->Width);
			pieImage.th = vRemainder;
			dest.h = vRemainder;
			//as above
			{
				pieImage.tw = Image->Width;
				dest.x = x+Image->XOffset;
				dest.w = Image->Width;
				while(hRep > 0)
				{
					pie_DrawImage(&pieImage, &dest, &rendStyle);
					hRep --;
					dest.x += Image->Width;
				}
				//draw remainder
				if (hRemainder > 0)
				{
					pieImage.tw = hRemainder;
					dest.w = hRemainder;
					pie_DrawImage(&pieImage, &dest, &rendStyle);
				}
			}
		}
	default:
		break;
	}
}

void pie_ImageFileIDStretch(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int Width,int Height)
{
	IMAGEDEF *Image;
	PIEIMAGE pieImage;
	PIERECT dest;
	assert(ID < ImageFile->Header.NumImages);

	Image = &ImageFile->ImageDefs[ID];

	pie_SetBilinear(FALSE);
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);
	pie_SetColourKeyedBlack(TRUE);

	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		break;
	case ENGINE_GLIDE:
		gl_DrawStretchImage(ImageFile, ID, x, y, Width, Height);
		break;
	case ENGINE_D3D:
		pieImage.texPage = ImageFile->TPageIDs[Image->TPageID];
		pieImage.tu = Image->Tu;
		pieImage.tv = Image->Tv;
		pieImage.tw = Image->Width;
		pieImage.th = Image->Height;

		dest.x = x+Image->XOffset;
		dest.y = y+Image->YOffset;
		dest.w = Width;
		dest.h = Height;
		pie_DrawImage(&pieImage, &dest, &rendStyle);
	default:
		break;
	}
}

void pie_ImageDef(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y,BOOL bBilinear)
{
	PIEIMAGE pieImage;
	PIERECT dest;

	pie_SetBilinear(bBilinear);	//changed by alex 19 oct 98
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);
	pie_SetColourKeyedBlack(TRUE);

	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
//		Modulus = ImageFile->TexturePages[Image->TPageID].width;
//		Bmp = ImageFile->TexturePages[Image->TPageID].bmp;
		Bmp += ((UDWORD)Image->Tu) + ((UDWORD)Image->Tv) * Modulus;
		iV_ppBitmapTrans(Bmp,x+Image->XOffset,y+Image->YOffset,
			   		Image->Width,Image->Height,Modulus);
		break;
	case ENGINE_GLIDE:
		gl_IntelBitmap(Image->TPageID, Image->Tu, Image->Tv, Image->Width, Image->Height, 
					x+Image->XOffset, y+Image->YOffset, Image->Width, Image->Height);
		break;
	case ENGINE_D3D:
		pieImage.texPage = Image->TPageID;
		pieImage.tu = Image->Tu;
		pieImage.tv = Image->Tv;
		pieImage.tw = Image->Width;
		pieImage.th = Image->Height;
		dest.x = x+Image->XOffset;
		dest.y = y+Image->YOffset;
		dest.w = Image->Width;
		dest.h = Image->Height;
		pie_DrawImage(&pieImage, &dest, &rendStyle);
	default:
		break;
	}
	pie_SetBilinear(FALSE);	//changed by alex 19 oct 98

}

void pie_ImageDefTrans(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y,int TransRate)
{
	pie_ImageDef(Image,Bmp,Modulus,x,y,FALSE);
}

void pie_UploadDisplayBuffer(UBYTE *DisplayBuffer)
{
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		UploadDisplayBuffer(DisplayBuffer);
		break;
	case ENGINE_GLIDE:
		gl_UploadDisplayBuffer(DisplayBuffer);
		screen_SetBackDrop(DisplayBuffer, pie_GetVideoBufferWidth(), pie_GetVideoBufferHeight());
		break;
	case ENGINE_D3D:
		//only call inside D3D render
		pie_GlobalRenderEnd(FALSE);
		screen_Upload(DisplayBuffer);
		screen_SetBackDrop(DisplayBuffer, pie_GetVideoBufferWidth(), pie_GetVideoBufferHeight());
		pie_GlobalRenderBegin();
	default:
		break;
	}
}

void pie_DownloadDisplayBuffer(UBYTE *DisplayBuffer)
{
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		DownloadDisplayBuffer(DisplayBuffer);
		break;
	case ENGINE_GLIDE:
		gl_DownloadDisplayBuffer((UWORD*)DisplayBuffer);
		break;
	case ENGINE_D3D:
		//screen_SetBackDropFullWidth();//set when background sets
	default:
		break;
	}
}

void pie_ScaleBitmapRGB(UBYTE *DisplayBuffer,int Width,int Height,int ScaleR,int ScaleG,int ScaleB)
{
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		ScaleBitmapRGB(DisplayBuffer, Width, Height, ScaleR, ScaleG, ScaleB);
		break;
	case ENGINE_GLIDE:
		gl_ScaleBitmapRGB(DisplayBuffer, Width, Height, ScaleR, ScaleG, ScaleB);
		break;
	case ENGINE_D3D:
	default:
		break;
	}
}

BOOL pie_InitRadar(void)
{
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		break;
	case ENGINE_GLIDE:
		break;
	case ENGINE_D3D:
	default:
		break;
	}
	return TRUE;
}

BOOL pie_ShutdownRadar(void)
{
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		break;
	case ENGINE_GLIDE:
		break;
	case ENGINE_D3D:
	default:
		break;
	}
	return TRUE;
}


void pie_DownLoadRadar(unsigned char *buffer, UDWORD texPageID)
{
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		DownLoadRadar(buffer);
		break;
	case ENGINE_GLIDE:
		gl_DownLoadRadar(buffer,texPageID);
		break;
	case ENGINE_D3D:
		dtm_LoadRadarSurface(buffer);
	default:
		break;
	}
}

void pie_RenderRadar(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y)
{
	PIEIMAGE pieImage;
	PIERECT dest;
	//special case of pie_ImageDef
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		pie_ImageDef(Image,Bmp,Modulus,x,y,FALSE);
		break;
	case ENGINE_GLIDE:
		pie_ImageDef(Image,Bmp,Modulus,x,y,TRUE);
		break;
	case ENGINE_D3D:
		pie_SetBilinear(TRUE);
		pie_SetRendMode(REND_GOURAUD_TEX);
		pie_SetColour(COLOURINTENSITY);
		pie_SetColourKeyedBlack(TRUE);
		//special case function because texture is held outside of texture list
		pieImage.texPage = RADAR_TEXPAGE_D3D;
		pieImage.tu = 0;
		pieImage.tv = 0;
		pieImage.tw = dtm_GetRadarTexImageSize();
		pieImage.th = dtm_GetRadarTexImageSize();
		dest.x = x;
		dest.y = y;
		dest.w = 128;
		dest.h = 128;
		pie_DrawImage(&pieImage, &dest, &rendStyle);
	default:
		break;
	}
}


void pie_RenderRadarRotated(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y,int angle)
{
	PIEIMAGE pieImage;
	PIERECT dest;
	//special case of pie_ImageDef
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		pie_ImageDef(Image,Bmp,Modulus,x,y,FALSE);
		break;
	case ENGINE_GLIDE:
		pie_ImageDef(Image,Bmp,Modulus,x,y,TRUE);
		break;
	case ENGINE_D3D:
		pie_SetBilinear(TRUE);
		pie_SetRendMode(REND_GOURAUD_TEX);
		pie_SetColour(COLOURINTENSITY);
		pie_SetColourKeyedBlack(TRUE);
		//special case function because texture is held outside of texture list
		pieImage.texPage = RADAR_TEXPAGE_D3D;
		pieImage.tu = 0;
		pieImage.tv = 0;
		pieImage.tw = dtm_GetRadarTexImageSize();
		pieImage.th = dtm_GetRadarTexImageSize();
		dest.x = x;
		dest.y = y;
		dest.w = 128;
		dest.h = 128;
		pie_DrawImage(&pieImage, &dest, &rendStyle);
	default:
		break;
	}
}


/*	Converts an 8 bit raw (palettised) source image to
	a 16 bit argb destination image 
*/
void	bufferTo16Bit(UBYTE *origBuffer,UWORD *newBuffer, BOOL b3DFX)
{
UBYTE	paletteIndex;
UWORD	newColour;
UWORD	gun;
UDWORD	i;
DDPIXELFORMAT	*DDPixelFormat;
ULONG			mask;
BYTE			ap = 0,	ac = 0, rp = 0,	rc = 0, gp = 0,	gc = 0, bp = 0, bc = 0;
iColour*		psPalette;
UDWORD			size;

	if (b3DFX)
	{
		// 565 RGB
		ap = 16;
		ac = 0;
		rp = 11;
		rc = 5;
		gp = 5;
		gc = 6;
		bp = 0;
		bc = 5;
	}
	else
	{
		DDPixelFormat = screenGetBackBufferPixelFormat();
		/*
		// Cannot playback if not 16bit mode 
		*/
		if( DDPixelFormat->dwRGBBitCount == 16 )
		{
			/*
			// Find out the RGB type of the surface and tell the codec...
			*/
			mask = DDPixelFormat->dwRGBAlphaBitMask;
			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					ap++;
				}
			}
			while((mask & 1))
			{
				mask>>=1;
				ac++;
			}

			mask = DDPixelFormat->dwRBitMask;
			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					rp++;
				}
			}
			while((mask & 1))
			{
				mask>>=1;
				rc++;
			}

			mask = DDPixelFormat->dwGBitMask;
			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					gp++;
				}
			}
			while((mask & 1))
			{
				mask>>=1;
				gc++;
			}

			mask = DDPixelFormat->dwBBitMask;
			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					bp++;
				}
			}
			while((mask & 1))
			{
				mask>>=1;
				bc++;
			}
		}
	}

	/*
		640*480, 8 bit colour source image 
		640*480, 16 bit colour destination image
	*/
	size = BACKDROP_WIDTH*BACKDROP_HEIGHT;//pie_GetVideoBufferWidth()*pie_GetVideoBufferHeight();
	for(i=0; i<size; i++)
	{
		psPalette = pie_GetGamePal();
		/* Get the next colour */
		paletteIndex = *origBuffer++;
		/* Flush out destination word (and alpha bits) */
		newColour = 0;
		/* Get red bits - 5 */
		gun = (UWORD)(psPalette[paletteIndex].r>>(8-rc));
		gun = gun << rp;
		newColour += gun;
		/* Get green bits - 6 */
		gun = (UWORD)(psPalette[paletteIndex].g>>(8-gc));
		gun = gun << gp;
		newColour += gun;
		/* Get blue bits - 5 */
		gun = (UWORD)(psPalette[paletteIndex].b>>(8-bc));
		gun = gun << bp;
		newColour += gun;
		/* Copy over */
		*newBuffer++ = newColour;
	}
}


void pie_ResetBackDrop(void)
{
	screen_SetBackDrop(backDropBmp, BACKDROP_WIDTH, BACKDROP_HEIGHT);
	return;
}
	
	
void pie_LoadBackDrop(SCREENTYPE screenType, BOOL b3DFX)
{
iSprite backDropSprite;
iBitmap	tempBmp[BACKDROP_WIDTH*BACKDROP_HEIGHT];
DDPIXELFORMAT	*pDDPixelFormat;
UDWORD	chooser0,chooser1;
CHAR	backd[128];
SDWORD	bitDepth;

	if (b3DFX)
	{
		bitDepth = 16;
	}
	else
	{
		pDDPixelFormat = screenGetBackBufferPixelFormat();
		if( pDDPixelFormat->dwRGBBitCount == 16 )
		{
			bitDepth = 16;
		}
		else
		{
			bitDepth = 8;
		}
	}

	//randomly load in a backdrop piccy.
	srand((unsigned)time( NULL ) );

	chooser0 = 0;
	chooser1 = rand()%7;

	backDropSprite.width = BACKDROP_WIDTH;
	backDropSprite.height = BACKDROP_HEIGHT;
	if (bitDepth == 8)
	{
		backDropSprite.bmp = (UBYTE*)backDropBmp;
	}
	else
	{
		backDropSprite.bmp = tempBmp;
	}

	switch (screenType)
	{
	case SCREEN_RANDOMBDROP:
		sprintf(backd,"texpages\\bdrops\\%d%d-bdrop.pcx",chooser0,chooser1);
		break;
	case SCREEN_COVERMOUNT:
		sprintf(backd,"texpages\\bdrops\\demo-bdrop.pcx");
		break;
	case SCREEN_MISSIONEND:
		sprintf(backd,"texpages\\bdrops\\missionend.pcx");
		break;
	case SCREEN_SLIDE1:
		sprintf(backd,"texpages\\slides\\slide1.pcx");
		break;
	case SCREEN_SLIDE2:
		sprintf(backd,"texpages\\slides\\slide2.pcx");
		break;
	case SCREEN_SLIDE3:
		sprintf(backd,"texpages\\slides\\slide3.pcx");
		break;
	case SCREEN_SLIDE4:
		sprintf(backd,"texpages\\slides\\slide4.pcx");
		break;
	case SCREEN_SLIDE5:
		sprintf(backd,"texpages\\slides\\slide5.pcx");
		break;

	case SCREEN_CREDITS:
		sprintf(backd,"texpages\\bdrops\\credits.pcx");
		break;

	default:
		sprintf(backd,"texpages\\bdrops\\credits.pcx");
		break;
	}
	if (!pie_PCXLoadToBuffer(backd, &backDropSprite, NULL))
	{
		return;
	}

	if (bitDepth != 8)
	{
		bufferTo16Bit(tempBmp, backDropBmp, b3DFX);		// convert
	}

	screen_SetBackDrop(backDropBmp, BACKDROP_WIDTH, BACKDROP_HEIGHT);
}

void pie_D3DSetupRenderForFlip(SDWORD surfaceOffsetX, SDWORD surfaceOffsetY, UWORD* pSrcData, SDWORD srcWidth, SDWORD srcHeight, SDWORD srcStride)
{

	gSurfaceOffsetX = surfaceOffsetX;
	gSurfaceOffsetY = surfaceOffsetY;
	pgSrcData		=	pSrcData;
	gSrcWidth		=	srcWidth;
	gSrcHeight		= srcHeight;
	gSrcStride		= srcStride;
	return;
}

void pie_D3DRenderForFlip(void)
{
	if (pgSrcData != NULL)
	{
		pie_RenderImageToSurface(screenGetSurface(), gSurfaceOffsetX, gSurfaceOffsetY, pgSrcData, gSrcWidth, gSrcHeight, gSrcStride);
		pgSrcData = NULL;
	}
}



#endif