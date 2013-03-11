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

static GFX *radarGfx = NULL;

struct PIERECT  ///< Screen rectangle.
{
	SWORD x, y, w, h;
};

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

GFX::GFX(GFXTYPE type, GLenum drawType, int coordsPerVertex) : mType(type), mdrawType(drawType), mCoordsPerVertex(coordsPerVertex), mSize(0)
{
	glGenBuffers(VBO_MINIMAL, mBuffers);
	if (type == GFX_TEXTURE)
	{
		glGenTextures(1, &mTexture);
	}
}

void GFX::loadTexture(const char *filename, GLenum filter)
{
	ASSERT(mType == GFX_TEXTURE, "Wrong GFX type");
	const char *extension = strrchr(filename, '.'); // determine the filetype
	iV_Image image;
	if (!extension || strcmp(extension, ".png") != 0)
	{
		debug(LOG_ERROR, "Bad image filename: %s", filename);
		return;
	}
	if (iV_loadImage_PNG(filename, &image))
	{
		makeTexture(image.width, image.height, filter, iV_getPixelFormat(&image), image.bmp);
		iV_unloadImage(&image);
	}
}

void GFX::makeTexture(int width, int height, GLenum filter, GLenum format, const GLvoid *image)
{
	ASSERT(mType == GFX_TEXTURE, "Wrong GFX type");
	pie_SetTexturePage(TEXPAGE_EXTERN);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	mWidth = width;
	mHeight = height;
	mFormat = format;
}

void GFX::updateTexture(const void *image, int width, int height)
{
	ASSERT(mType == GFX_TEXTURE, "Wrong GFX type");
	if (width == -1) width = mWidth;
	if (height == -1) height = mHeight;
	pie_SetTexturePage(TEXPAGE_EXTERN);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, mFormat, GL_UNSIGNED_BYTE, image);
}

void GFX::buffers(int vertices, const GLvoid *vertBuf, const GLvoid *auxBuf)
{
	glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_VERTEX]);
	glBufferData(GL_ARRAY_BUFFER, vertices * mCoordsPerVertex * sizeof(GLfloat), vertBuf, GL_STATIC_DRAW);
	if (mType == GFX_TEXTURE)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_TEXCOORD]);
		glBufferData(GL_ARRAY_BUFFER, vertices * 2 * sizeof(GLfloat), auxBuf, GL_STATIC_DRAW);
	}
	else if (mType == GFX_COLOUR)
	{
		// reusing texture buffer for colours for now
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_TEXCOORD]);
		glBufferData(GL_ARRAY_BUFFER, vertices * 4 * sizeof(GLbyte), auxBuf, GL_STATIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	mSize = vertices;
}

void GFX::draw()
{
	if (mType == GFX_TEXTURE)
	{
		pie_SetTexturePage(TEXPAGE_EXTERN);
		glBindTexture(GL_TEXTURE_2D, mTexture);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_TEXCOORD]); glTexCoordPointer(2, GL_FLOAT, 0, NULL);
	}
	else if (mType == GFX_COLOUR)
	{
		pie_SetAlphaTest(false);
		pie_SetTexturePage(TEXPAGE_NONE);
		glEnableClientState(GL_COLOR_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_TEXCOORD]); glColorPointer(4, GL_UNSIGNED_BYTE, 0, NULL);
	}
	glEnableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_VERTEX]); glVertexPointer(mCoordsPerVertex, GL_FLOAT, 0, NULL);
	glDrawArrays(mdrawType, 0, mSize);
	glDisableClientState(GL_VERTEX_ARRAY);
	if (mType == GFX_TEXTURE)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	else if (mType == GFX_COLOUR)
	{
		glDisableClientState(GL_COLOR_ARRAY);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GFX::~GFX()
{
	glDeleteBuffers(VBO_MINIMAL, mBuffers);
	if (mType == GFX_TEXTURE)
	{
		glDeleteTextures(1, &mTexture);
	}
}

void iV_Line(int x0, int y0, int x1, int y1, PIELIGHT colour)
{
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetAlphaTest(false);

	glColor4ubv(colour.vector);
	glBegin(GL_LINES);
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

void iV_ShadowBox(int x0, int y0, int x1, int y1, int pad, PIELIGHT first, PIELIGHT second, PIELIGHT fill)
{
	pie_SetRendMode(REND_OPAQUE);
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_DrawRect(x0 + pad, y0 + pad, x1 - pad, y1 - pad, fill); // necessary side-effect: sets alpha test off
	glColor4ubv(first.vector);
	glBegin(GL_LINES);
	glVertex2i(x0, y1);
	glVertex2i(x0, y0);
	glVertex2i(x0, y0);
	glVertex2i(x1, y0);
	glEnd();
	glColor4ubv(second.vector);
	glBegin(GL_LINES);
	glVertex2i(x1, y0);
	glVertex2i(x1, y1);
	glVertex2i(x0, y1);
	glVertex2i(x1, y1);
	glEnd();
}

/***************************************************************************/

void iV_Box(int x0,int y0, int x1, int y1, PIELIGHT colour)
{
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetAlphaTest(false);

	if (x0>rendSurface.clip.right || x1<rendSurface.clip.left ||
		y0>rendSurface.clip.bottom || y1<rendSurface.clip.top)
	{
		return;
	}

	if (x0<rendSurface.clip.left)
		x0 = rendSurface.clip.left;
	if (x1>rendSurface.clip.right)
		x1 = rendSurface.clip.right;
	if (y0<rendSurface.clip.top)
		y0 = rendSurface.clip.top;
	if (y1>rendSurface.clip.bottom)
		y1 = rendSurface.clip.bottom;

	glColor4ubv(colour.vector);
	glBegin(GL_LINES);
	glVertex2i(x0, y1);
	glVertex2i(x0, y0);
	glVertex2i(x0, y0);
	glVertex2i(x1, y0);
	glVertex2i(x1, y0);
	glVertex2i(x1, y1);
	glVertex2i(x0, y1);
	glVertex2i(x1, y1);
	glEnd();
}

/***************************************************************************/

void pie_BoxFill(int x0,int y0, int x1, int y1, PIELIGHT colour)
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

static bool assertValidImage(IMAGEFILE *imageFile, unsigned id)
{
	ASSERT_OR_RETURN(false, id < imageFile->imageDefs.size(), "Out of range 1: %u/%d", id, (int)imageFile->imageDefs.size());
	ASSERT_OR_RETURN(false, imageFile->imageDefs[id].TPageID < imageFile->pages.size(), "Out of range 2: %u", imageFile->imageDefs[id].TPageID);
	return true;
}

static void pie_DrawImage(IMAGEFILE *imageFile, int id, Vector2i size, const PIERECT *dest, PIELIGHT colour = WZCOL_WHITE)
{
	ImageDef const &image2 = imageFile->imageDefs[id];
	GLuint texPage = imageFile->pages[image2.TPageID].id;
	GLfloat invTextureSize = 1.f / imageFile->pages[image2.TPageID].size;
	int tu = image2.Tu;
	int tv = image2.Tv;

	pie_SetTexturePage(texPage);
	glColor4ubv(colour.vector);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(tu * invTextureSize, tv * invTextureSize);
		glVertex2f(dest->x, dest->y);

		glTexCoord2f((tu + size.x) * invTextureSize, tv * invTextureSize);
		glVertex2f(dest->x + dest->w, dest->y);

		glTexCoord2f(tu * invTextureSize, (tv + size.y) * invTextureSize);
		glVertex2f(dest->x, dest->y + dest->h);

		glTexCoord2f((tu + size.x) * invTextureSize, (tv + size.y) * invTextureSize);
		glVertex2f(dest->x + dest->w, dest->y + dest->h);
	glEnd();
}

static Vector2i makePieImage(IMAGEFILE *imageFile, unsigned id, PIERECT *dest = NULL, int x = 0, int y = 0)
{
	ImageDef const &image = imageFile->imageDefs[id];
	Vector2i pieImage;
	pieImage.x = image.Width;
	pieImage.y = image.Height;
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
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	pie_SetRendMode(REND_ALPHA);
	pie_SetAlphaTest(true);

	pie_DrawImage(ImageFile, ID, pieImage, &dest);
}

void iV_DrawImageTc(Image image, Image imageTc, int x, int y, PIELIGHT colour)
{
	if (!assertValidImage(image.images, image.id) || !assertValidImage(image.images, image.id))
	{
		return;
	}

	PIERECT dest;
	Vector2i pieImage   = makePieImage(image.images, image.id, &dest, x, y);
	Vector2i pieImageTc = makePieImage(imageTc.images, imageTc.id);

	pie_SetRendMode(REND_ALPHA);
	pie_SetAlphaTest(true);

	pie_DrawImage(image.images, image.id, pieImage, &dest);
	pie_DrawImage(imageTc.images, imageTc.id, pieImageTc, &dest, colour);
}

// Repeat a texture
void iV_DrawImageRepeatX(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Width)
{
	int hRep, hRemainder;

	assertValidImage(ImageFile, ID);
	const ImageDef *Image = &ImageFile->imageDefs[ID];

	pie_SetRendMode(REND_OPAQUE);
	pie_SetAlphaTest(true);

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	hRemainder = Width % Image->Width;

	for (hRep = 0; hRep < Width / Image->Width; hRep++)
	{
		pie_DrawImage(ImageFile, ID, pieImage, &dest);
		dest.x += Image->Width;
	}

	// draw remainder
	if (hRemainder > 0)
	{
		pieImage.x = hRemainder;
		dest.w = hRemainder;
		pie_DrawImage(ImageFile, ID, pieImage, &dest);
	}
}

void iV_DrawImageRepeatY(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Height)
{
	int vRep, vRemainder;

	assertValidImage(ImageFile, ID);
	const ImageDef *Image = &ImageFile->imageDefs[ID];

	pie_SetRendMode(REND_OPAQUE);
	pie_SetAlphaTest(true);

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	vRemainder = Height % Image->Height;

	for (vRep = 0; vRep < Height / Image->Height; vRep++)
	{
		pie_DrawImage(ImageFile, ID, pieImage, &dest);
		dest.y += Image->Height;
	}

	// draw remainder
	if (vRemainder > 0)
	{
		pieImage.y = vRemainder;
		dest.h = vRemainder;
		pie_DrawImage(ImageFile, ID, pieImage, &dest);
	}
}

void iV_DrawImageScaled(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int w, int h)
{
	if (!assertValidImage(ImageFile, ID))
	{
		return;
	}

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);
	dest.w = w;
	dest.h = h;

	pie_SetRendMode(REND_ALPHA);
	pie_SetAlphaTest(true);

	pie_DrawImage(ImageFile, ID, pieImage, &dest);
}

bool pie_InitRadar(void)
{
	radarGfx = new GFX(GFX_TEXTURE, GL_TRIANGLE_STRIP, 2);
	return true;
}

bool pie_ShutdownRadar(void)
{
	delete radarGfx;
	radarGfx = NULL;
	return true;
}

void pie_SetRadar(GLfloat x, GLfloat y, GLfloat width, GLfloat height, int twidth, int theight, bool filter)
{
	radarGfx->makeTexture(twidth, theight, filter ? GL_LINEAR : GL_NEAREST);
	GLfloat texcoords[] = { 0.0f, 0.0f,  1.0f, 0.0f,  0.0f, 1.0f,  1.0f, 1.0f };
	GLfloat vertices[] = { x, y,  x + width, y,  x, y + height,  x + width, y + height };
	radarGfx->buffers(4, vertices, texcoords);
}

/** Store radar texture with given width and height. */
void pie_DownLoadRadar(UDWORD *buffer)
{
	radarGfx->updateTexture(buffer);
}

/** Display radar texture using the given height and width, depending on zoom level. */
void pie_RenderRadar()
{
	pie_SetRendMode(REND_ALPHA);
	glColor4ubv(WZCOL_WHITE.vector); // hack
	radarGfx->draw();
}

void pie_LoadBackDrop(SCREENTYPE screenType)
{
	char backd[128];

	//randomly load in a backdrop piccy.
	srand( (unsigned)time(NULL) + 17 ); // Use offset since time alone doesn't work very well

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
