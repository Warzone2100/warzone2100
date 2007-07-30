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
/***************************************************************************/
/*
 * pieBlitFunc.c
 *
 * patch for exisitng ivis rectangle draw functions.
 *
 */
/***************************************************************************/

#include "lib/framework/frame.h"

#include <time.h>
#include <SDL/SDL_opengl.h>

#include "lib/ivis_common/pieblitfunc.h"
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piemode.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/pieclip.h"
#include "lib/ivis_common/piefunc.h"
#include "piematrix.h"
#include "screen.h"

extern BOOL dtm_LoadRadarSurface(UBYTE* radarBuffer);
extern SDWORD dtm_GetRadarTexImageSize(void);

/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/
SDWORD gSurfaceOffsetX;
SDWORD gSurfaceOffsetY;
UWORD* pgSrcData = NULL;
SDWORD gSrcWidth;
SDWORD gSrcHeight;
SDWORD gSrcStride;

#define COLOURINTENSITY 0xffffffff
/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

PIESTYLE rendStyle;
Vector2i rectVerts[4];

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
void pie_Line(int x0, int y0, int x1, int y1, Uint32 colour)
{
//	PIELIGHT light;

	pie_SetRendMode(REND_FLAT);
	pie_SetColour(colour);
	pie_SetTexturePage(-1);
	pie_SetColourKeyedBlack(FALSE);

	glBegin(GL_LINE_STRIP);
	glVertex2f(x0, y0);
	glVertex2f(x1, y1);
	glEnd();
}
/***************************************************************************/

void pie_Box(int x0,int y0, int x1, int y1, Uint32 colour)
{
//	PIELIGHT light;
//	iColour* psPalette;

	pie_SetRendMode(REND_FLAT);
	pie_SetColour(colour);
	pie_SetTexturePage(-1);
	pie_SetColourKeyedBlack(FALSE);

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

	glBegin(GL_LINE_STRIP);
	glVertex2f(x0, y0);
	glVertex2f(x1, y0);
	glVertex2f(x1, y1);
	glVertex2f(x0, y1);
	glVertex2f(x0, y0);
	glEnd();
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

	psPalette = pie_GetGamePal();
	light.byte.r = psPalette[colour].r;
	light.byte.g = psPalette[colour].g;
	light.byte.b = psPalette[colour].b;
	light.byte.a = MAX_UB_LIGHT;
	pie_DrawRect( x0, y0, x1, y1, light.argb );
}

void pie_BoxFill(int x0,int y0, int x1, int y1, Uint32 colour)
{
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

	pie_DrawRect( x0, y0, x1, y1, colour );

}
/***************************************************************************/

void pie_TransBoxFill(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1)
{
	UDWORD rgb;
	UDWORD transparency;
	rgb = (pie_FILLRED<<16) | (pie_FILLGREEN<<8) | pie_FILLBLUE;//blue
	transparency = pie_FILLTRANS;
	pie_UniTransBoxFill(x0, y0, x1, y1, rgb, transparency);
}

/***************************************************************************/
void pie_UniTransBoxFill(SDWORD x0,SDWORD y0, SDWORD x1, SDWORD y1, UDWORD rgb, UDWORD transparency)
{
	UDWORD light;

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

	if (transparency == 0 ) {
		transparency = 127;
	}
	pie_SetTexturePage(-1);
	pie_SetRendMode(REND_ALPHA_FLAT);
	light = (rgb & 0x00ffffff) + (transparency << 24);
	pie_DrawRect( x0, y0, x1, y1, light );
}

/***************************************************************************/

void pie_DrawImageFileID(IMAGEFILE *ImageFile, UWORD ID, int x, int y)
{
	IMAGEDEF *Image;
	PIEIMAGE pieImage;
	PIERECT dest;

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

	pieImage.texPage = ImageFile->TPageIDs[Image->TPageID];
	pieImage.tu = Image->Tu;
	pieImage.tv = Image->Tv;
	pieImage.tw = Image->Width;
	pieImage.th = Image->Height;
	dest.x = x + Image->XOffset;
	dest.y = y + Image->YOffset;
	dest.w = Image->Width;
	dest.h = Image->Height;
	pie_DrawImage(&pieImage, &dest, &rendStyle);
}

BOOL	bAddSprites = FALSE;
UDWORD	addSpriteLevel;

void	pie_SetAdditiveSprites(BOOL val) {
	bAddSprites = val;
}

void	pie_SetAdditiveSpriteLevel(UDWORD val) {
	addSpriteLevel = val;
}

static BOOL pie_GetAdditiveSprites( void ) {
	return bAddSprites;
}

void pie_ImageFileID(IMAGEFILE *ImageFile, UWORD ID, int x, int y)
{
	IMAGEDEF *Image;
	PIEIMAGE pieImage;
	PIERECT dest;

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

   	if(pie_GetAdditiveSprites()) {
		pie_SetBilinear(TRUE);
		pie_SetRendMode(REND_ALPHA_TEX);
		pie_SetColour(addSpriteLevel);

	} else {
		pie_SetBilinear(FALSE);
		pie_SetRendMode(REND_GOURAUD_TEX);
		pie_SetColour(COLOURINTENSITY);
	}
	pie_SetColourKeyedBlack(TRUE);

	pieImage.texPage = ImageFile->TPageIDs[Image->TPageID];
	pieImage.tu = Image->Tu;
	pieImage.tv = Image->Tv;
	pieImage.tw = Image->Width;
	pieImage.th = Image->Height;
	dest.x = x + Image->XOffset;
	dest.y = y + Image->YOffset;
	dest.w = Image->Width;
	dest.h = Image->Height;
	pie_DrawImage(&pieImage, &dest, &rendStyle);
}


/***************************************************************************/

void pie_ImageFileIDTile(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Width, int Height)
{
	IMAGEDEF *Image;
	SDWORD hRep, hRemainder, vRep, vRemainder;
	PIEIMAGE pieImage;
	PIERECT dest;

	assert(ID < ImageFile->Header.NumImages);

	Image = &ImageFile->ImageDefs[ID];

	pie_SetBilinear(FALSE);
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);
	pie_SetColourKeyedBlack(TRUE);

	pieImage.texPage = ImageFile->TPageIDs[Image->TPageID];
	pieImage.tu = Image->Tu;
	pieImage.tv = Image->Tv;
	pieImage.tw = Image->Width;
	pieImage.th = Image->Height;

	dest.x = x + Image->XOffset;
	dest.y = y + Image->YOffset;
	dest.w = Image->Width;
	dest.h = Image->Height;

	vRemainder = Height % Image->Height;
	hRemainder = Width % Image->Width;

	for (vRep = 0; vRep < Height/Image->Height; vRep++)
	{
		pieImage.tw = Image->Width;
		dest.x = x + Image->XOffset;
		dest.w = Image->Width;

		for (hRep = 0; hRep < Width/Image->Width; hRep++)
		{
			pie_DrawImage(&pieImage, &dest, &rendStyle);
			dest.x += Image->Width;
		}

		//draw remainder
		if (hRemainder > 0)
		{
			pieImage.tw = hRemainder;
			dest.w = hRemainder;
			pie_DrawImage(&pieImage, &dest, &rendStyle);
		}

		dest.y += Image->Height;
	}

	//draw remainder
	if (vRemainder > 0)
	{
		//as above
		pieImage.tw = Image->Width;
		dest.x = x + Image->XOffset;
		dest.w = Image->Width;

		pieImage.th = vRemainder;
		dest.h = vRemainder;

		for (hRep = 0; hRep < Width/Image->Width; hRep++)
		{
			pie_DrawImage(&pieImage, &dest, &rendStyle);
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

void pie_ImageFileIDStretch(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Width, int Height)
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

	pieImage.texPage = ImageFile->TPageIDs[Image->TPageID];
	pieImage.tu = Image->Tu;
	pieImage.tv = Image->Tv;
	pieImage.tw = Image->Width;
	pieImage.th = Image->Height;

	dest.x = x + Image->XOffset;
	dest.y = y + Image->YOffset;
	dest.w = Width;
	dest.h = Height;
	pie_DrawImage(&pieImage, &dest, &rendStyle);
}

/* FIXME: WTF is this supposed to do? Looks like some other functionality
 * was retrofitted onto something else. - Per */
void pie_UploadDisplayBuffer()
{
	screen_Upload(NULL);
}

UDWORD radarTexture;
unsigned char radarBitmap[128*128*4];

BOOL pie_InitRadar(void)
{
	glGenTextures(1, &radarTexture);
	return TRUE;
}

BOOL pie_ShutdownRadar(void)
{
	glDeleteTextures(1, &radarTexture);
	return TRUE;
}


void pie_DownLoadRadar( unsigned char *buffer )
{
	unsigned int i, j;
	iColour* psPalette = pie_GetGamePal();

	for (i = 0, j = 0; i < 128*128; ++i) {
		radarBitmap[j++] = psPalette[buffer[i]].r;
		radarBitmap[j++] = psPalette[buffer[i]].g;
		radarBitmap[j++] = psPalette[buffer[i]].b;
		if (buffer[i] == 0) {
			radarBitmap[j++] = 0;
		} else {
			radarBitmap[j++] = 255;
		}
	}
	pie_SetTexturePage(radarTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, wz_texture_compression, 128, 128, 0,
		     GL_RGBA, GL_UNSIGNED_BYTE, radarBitmap);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

void pie_RenderRadar( int x, int y )
{
	PIEIMAGE pieImage;
	PIERECT dest;

	pie_SetBilinear(TRUE);
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);
	pie_SetColourKeyedBlack(TRUE);
	//special case function because texture is held outside of texture list
	pieImage.texPage = radarTexture;
	pieImage.tu = 0;
	pieImage.tv = 0;
	pieImage.tw = 256;
	pieImage.th = 256;
	dest.x = x;
	dest.y = y;
	dest.w = 128;
	dest.h = 128;
	pie_DrawImage(&pieImage, &dest, &rendStyle);
}

void pie_LoadBackDrop(SCREENTYPE screenType)
{
	char	backd[128];

	//randomly load in a backdrop piccy.
	srand( (unsigned)time(NULL) + 17 ); // Use offset since time alone doesn't work very well

	switch (screenType)
	{
	case SCREEN_RANDOMBDROP:
		if ( rand()%2 == 0 )
			sprintf(backd,"texpages/bdrops/0%i-bdrop.png", rand()%7); // Range: 0-6
		else
			sprintf(backd,"texpages/bdrops/wzlogo_%i.png", rand()%5); // Range: 0-4
		break;
	case SCREEN_MISSIONEND:
		sprintf(backd,"texpages/bdrops/missionend.png");
		break;

	case SCREEN_CREDITS:
	default:
		sprintf(backd,"texpages/bdrops/credits.png");
		break;
	}

	screen_SetBackDropFromFile(backd);
}
