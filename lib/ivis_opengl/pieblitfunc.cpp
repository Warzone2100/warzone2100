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
/***************************************************************************/
/*
 * pieBlitFunc.c
 *
 * patch for exisitng ivis rectangle draw functions.
 *
 */
/***************************************************************************/

#include "lib/framework/frame.h"
#include "lib/framework/opengl.h"
#include <time.h>

#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/tex.h"
#include "piematrix.h"
#include "screen.h"

/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

#define pie_FILLRED	 16
#define pie_FILLGREEN	 16
#define pie_FILLBLUE	128
#define pie_FILLTRANS	128

static GLuint radarTexture;
static GLuint radarSizeX, radarSizeY;
static GLfloat radarTexX, radarTexY;

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

void iV_Line(int x0, int y0, int x1, int y1, PIELIGHT colour)
{
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetAlphaTest(false);

	glColor4ubv(colour.vector);
	glBegin(GL_LINE_STRIP);
	glVertex2i(x0, y0);
	glVertex2i(x1, y1);
	glEnd();
}

/**
 *	Assumes render mode set up externally, draws filled rectangle.
 */
static void pie_DrawRect(float x0, float y0, float x1, float y1, PIELIGHT colour)
{
	pie_SetAlphaTest(false);

	glColor4ubv(colour.vector);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex2f(x0, y0);
	glVertex2f(x1, y0);
	glVertex2f(x0, y1);
	glVertex2f(x1, y1);
	glEnd();
}


/***************************************************************************/

void iV_Box(int x0, int y0, int x1, int y1, PIELIGHT colour)
{
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetAlphaTest(false);

	if (x0 > rendSurface.clip.right || x1 < rendSurface.clip.left ||
	    y0 > rendSurface.clip.bottom || y1 < rendSurface.clip.top)
	{
		return;
	}

	if (x0 < rendSurface.clip.left)
	{
		x0 = rendSurface.clip.left;
	}
	if (x1 > rendSurface.clip.right)
	{
		x1 = rendSurface.clip.right;
	}
	if (y0 < rendSurface.clip.top)
	{
		y0 = rendSurface.clip.top;
	}
	if (y1 > rendSurface.clip.bottom)
	{
		y1 = rendSurface.clip.bottom;
	}

	glColor4ubv(colour.vector);
	glBegin(GL_LINE_STRIP);
	glVertex2f(x0, y0);
	glVertex2f(x1, y0);
	glVertex2f(x1, y1);
	glVertex2f(x0, y1);
	glVertex2f(x0, y0);
	glEnd();
}

/***************************************************************************/

void pie_BoxFill(int x0, int y0, int x1, int y1, PIELIGHT colour)
{
	pie_SetRendMode(REND_OPAQUE);
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_DrawRect(x0, y0, x1, y1, colour);
}

/***************************************************************************/

void iV_TransBoxFill(float x0, float y0, float x1, float y1)
{
	PIELIGHT light;

	light.byte.r = pie_FILLRED;
	light.byte.g = pie_FILLGREEN;
	light.byte.b = pie_FILLBLUE;
	light.byte.a = pie_FILLTRANS;
	pie_UniTransBoxFill(x0, y0, x1, y1, light);
}

/***************************************************************************/

void pie_UniTransBoxFill(float x0, float y0, float x1, float y1, PIELIGHT light)
{
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetRendMode(REND_ALPHA);
	pie_DrawRect(x0, y0, x1, y1, light);
}

/***************************************************************************/

bool assertValidImage(IMAGEFILE *imageFile, unsigned id)
{
	ASSERT_OR_RETURN(false, id < imageFile->imageDefs.size(), "Out of range 1: %u/%d", id, (int)imageFile->imageDefs.size());
	ASSERT_OR_RETURN(false, imageFile->imageDefs[id].TPageID < imageFile->pages.size(), "Out of range 2: %u", imageFile->imageDefs[id].TPageID);
	return true;
}

static PIEIMAGE makePieImage(IMAGEFILE *imageFile, unsigned id, PIERECT *dest = NULL, int x = 0, int y = 0)
{
	ImageDef const &image = imageFile->imageDefs[id];
	PIEIMAGE pieImage;
	pieImage.texPage = imageFile->pages[image.TPageID].id;
	pieImage.invTextureSize = 1.f / imageFile->pages[image.TPageID].size;
	pieImage.tu = image.Tu;
	pieImage.tv = image.Tv;
	pieImage.tw = image.Width;
	pieImage.th = image.Height;
	if (dest != NULL)
	{
		dest->x = x + image.XOffset;
		dest->y = y + image.YOffset;
		dest->w = image.Width;
		dest->h = image.Height;
	}
	return pieImage;
}

void iV_DrawImage(IMAGEFILE *ImageFile, UWORD ID, int x, int y)
{
	if (!assertValidImage(ImageFile, ID))
	{
		return;
	}

	PIERECT dest;
	PIEIMAGE pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	pie_SetRendMode(REND_ALPHA);
	pie_SetAlphaTest(true);

	pie_DrawImage(&pieImage, &dest);
}

void iV_DrawImageTc(IMAGEFILE *imageFile, unsigned id, unsigned idTc, int x, int y, PIELIGHT colour)
{
	if (!assertValidImage(imageFile, id) || !assertValidImage(imageFile, idTc))
	{
		return;
	}

	PIERECT dest;
	PIEIMAGE pieImage   = makePieImage(imageFile, id, &dest, x, y);
	PIEIMAGE pieImageTc = makePieImage(imageFile, idTc);

	pie_SetRendMode(REND_ALPHA);
	pie_SetAlphaTest(true);

	pie_DrawImage(&pieImage, &dest);
	pie_DrawImage(&pieImageTc, &dest, colour);
}

void iV_DrawImageRect(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Width, int Height)
{
	SDWORD hRep, hRemainder, vRep, vRemainder;

	assertValidImage(ImageFile, ID);
	ImageDef *Image = &ImageFile->imageDefs[ID];

	pie_SetRendMode(REND_OPAQUE);
	pie_SetAlphaTest(true);

	PIERECT dest;
	PIEIMAGE pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	vRemainder = Height % Image->Height;
	hRemainder = Width % Image->Width;

	for (vRep = 0; vRep < Height / Image->Height; vRep++)
	{
		pieImage.tw = Image->Width;
		dest.x = x + Image->XOffset;
		dest.w = Image->Width;

		for (hRep = 0; hRep < Width / Image->Width; hRep++)
		{
			pie_DrawImage(&pieImage, &dest);
			dest.x += Image->Width;
		}

		//draw remainder
		if (hRemainder > 0)
		{
			pieImage.tw = hRemainder;
			dest.w = hRemainder;
			pie_DrawImage(&pieImage, &dest);
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

		for (hRep = 0; hRep < Width / Image->Width; hRep++)
		{
			pie_DrawImage(&pieImage, &dest);
			dest.x += Image->Width;
		}

		//draw remainder
		if (hRemainder > 0)
		{
			pieImage.tw = hRemainder;
			dest.w = hRemainder;
			pie_DrawImage(&pieImage, &dest);
		}
	}
}

void iV_DrawImageScaled(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int w, int h)
{
	if (!assertValidImage(ImageFile, ID))
	{
		return;
	}

	PIERECT dest;
	PIEIMAGE pieImage = makePieImage(ImageFile, ID, &dest, x, y);
	dest.w = w;
	dest.h = h;

	pie_SetRendMode(REND_ALPHA);
	pie_SetAlphaTest(true);

	pie_DrawImage(&pieImage, &dest);
}

/* FIXME: WTF is this supposed to do? Looks like some other functionality
 * was retrofitted onto something else. - Per */
void pie_UploadDisplayBuffer()
{
	screen_Upload(NULL, false);
}

bool pie_InitRadar(void)
{
	radarTexture = _TEX_INDEX;
	glGenTextures(1, &_TEX_PAGE[_TEX_INDEX].id);
	_TEX_INDEX++;
	return true;
}

bool pie_ShutdownRadar(void)
{
	glDeleteTextures(1, &_TEX_PAGE[radarTexture].id);
	return true;
}

/** Store radar texture with given width and height. */
void pie_DownLoadRadar(UDWORD *buffer, int width, int height, bool filter)
{
	int w = 1, h = 1;
	char *black;

	/* Find power of two size */
	while (width > (w *= 2)) {}
	while (height > (h *= 2)) {}

	pie_SetTexturePage(radarTexture);
	black = (char *)calloc(1, w * h * 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
	free(black);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (filter)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	radarSizeX = width;
	radarSizeY = height;
	radarTexX = ((GLfloat)width / (GLfloat)w);
	radarTexY = ((GLfloat)height / (GLfloat)h);
}

/** Display radar texture using the given height and width, depending on zoom level. */
void pie_RenderRadar(int x, int y, int width, int height)
{
	pie_SetTexturePage(radarTexture);
	pie_SetRendMode(REND_ALPHA);

	glColor4ubv(WZCOL_WHITE.vector);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0, 0);			glVertex2f(x, y);
	glTexCoord2f(radarTexX, 0);		glVertex2f(x + width, y);
	glTexCoord2f(0, radarTexY);		glVertex2f(x, y + height);
	glTexCoord2f(radarTexX, radarTexY);	glVertex2f(x + width, y + height);
	glEnd();
}

void pie_LoadBackDrop(SCREENTYPE screenType)
{
	char backd[128];

	//randomly load in a backdrop piccy.
	srand((unsigned)time(NULL) + 17);   // Use offset since time alone doesn't work very well

	switch (screenType)
	{
	case SCREEN_RANDOMBDROP:
		snprintf(backd, sizeof(backd), "texpages/bdrops/backdrop%i.png", rand() % NUM_BACKDROPS); // Range: 0 to (NUM_BACKDROPS-1)
		break;
	case SCREEN_MISSIONEND:
		sstrcpy(backd, "texpages/bdrops/missionend.png");
		break;

	case SCREEN_CREDITS:
	default:
		sstrcpy(backd, "texpages/bdrops/credits.png");
		break;
	}

	screen_SetBackDropFromFile(backd);
}
