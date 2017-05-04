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
/***************************************************************************/
/*
 * pieBlitFunc.c
 *
 */
/***************************************************************************/

#include "lib/framework/frame.h"
#include "lib/framework/opengl.h"
#include "lib/gamelib/gtime.h"
#include <time.h>

#include "lib/ivis_opengl/bitimage.h"
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
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

static GFX *radarGfx = nullptr;

struct PIERECT  ///< Screen rectangle.
{
	float x, y, w, h;
};

/***************************************************************************/
/*
 *	Static function forward declarations
 */
/***************************************************************************/

static bool assertValidImage(IMAGEFILE *imageFile, unsigned id);
static Vector2i makePieImage(IMAGEFILE *imageFile, unsigned id, PIERECT *dest = nullptr, int x = 0, int y = 0);

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
	if (width == -1)
	{
		width = mWidth;
	}
	if (height == -1)
	{
		height = mHeight;
	}
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

#define VERTEX_POS_ATTRIB_INDEX 0
#define VERTEX_COORDS_ATTRIB_INDEX 1
#define VERTEX_COLOR_ATTRIB_INDEX 2

void GFX::draw(const glm::mat4 &modelViewProjectionMatrix)
{
	if (mType == GFX_TEXTURE)
	{
		pie_SetTexturePage(TEXPAGE_EXTERN);
		glBindTexture(GL_TEXTURE_2D, mTexture);
		pie_ActivateShader(SHADER_GFX_TEXT, modelViewProjectionMatrix, glm::vec4(1), 0);
		glEnableVertexAttribArray(VERTEX_COORDS_ATTRIB_INDEX);
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_TEXCOORD]);
		glVertexAttribPointer(VERTEX_COORDS_ATTRIB_INDEX, 2, GL_FLOAT, false, 0, nullptr);
	}
	else if (mType == GFX_COLOUR)
	{
		pie_SetTexturePage(TEXPAGE_NONE);
		pie_ActivateShader(SHADER_GFX_COLOUR, modelViewProjectionMatrix);
		glEnableVertexAttribArray(VERTEX_COLOR_ATTRIB_INDEX);
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_TEXCOORD]);
		glVertexAttribPointer(VERTEX_COLOR_ATTRIB_INDEX, 4, GL_UNSIGNED_BYTE, true, 0, nullptr);
	}
	glEnableVertexAttribArray(VERTEX_POS_ATTRIB_INDEX);
	glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_VERTEX]);
	glVertexAttribPointer(VERTEX_POS_ATTRIB_INDEX, mCoordsPerVertex, GL_FLOAT, false, 0, nullptr);
	glDrawArrays(mdrawType, 0, mSize);
	glDisableVertexAttribArray(VERTEX_POS_ATTRIB_INDEX);
	if (mType == GFX_TEXTURE)
	{
		glDisableVertexAttribArray(VERTEX_COORDS_ATTRIB_INDEX);
	}
	else if (mType == GFX_COLOUR)
	{
		glDisableVertexAttribArray(VERTEX_COLOR_ATTRIB_INDEX);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	pie_DeactivateShader();
}

GFX::~GFX()
{
	glDeleteBuffers(VBO_MINIMAL, mBuffers);
	if (mType == GFX_TEXTURE)
	{
		glDeleteTextures(1, &mTexture);
	}
}

static void enableRect()
{
	glEnableVertexAttribArray(VERTEX_POS_ATTRIB_INDEX);
	glBindBuffer(GL_ARRAY_BUFFER, pie_internal::rectBuffer);
	glVertexAttribPointer(VERTEX_POS_ATTRIB_INDEX, 4, GL_BYTE, false, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void disableRect()
{
	glDisableVertexAttribArray(VERTEX_POS_ATTRIB_INDEX);
}

void iV_Line(int x0, int y0, int x1, int y1, PIELIGHT colour)
{
	pie_SetTexturePage(TEXPAGE_NONE);
	const glm::vec4 color(
		colour.vector[0] / 255.f,
		colour.vector[1] / 255.f,
		colour.vector[2] / 255.f,
		colour.vector[3] / 255.f
	);
	const auto &mat = glm::ortho(0.f, static_cast<float>(pie_GetVideoBufferWidth()), static_cast<float>(pie_GetVideoBufferHeight()), 0.f);
	pie_ActivateShader(SHADER_LINE, glm::vec2(x0, y0), glm::vec2(x1, y1), color, mat);
	enableRect();
	glDrawArrays(GL_LINES, 0, 2);
	disableRect();
	pie_DeactivateShader();
}

void iV_Lines(const std::vector<glm::ivec4> &lines, PIELIGHT colour)
{
	pie_SetTexturePage(TEXPAGE_NONE);
	const glm::vec4 color(
		colour.vector[0] / 255.f,
		colour.vector[1] / 255.f,
		colour.vector[2] / 255.f,
		colour.vector[3] / 255.f
	);
	const auto &mat = glm::ortho(0.f, static_cast<float>(pie_GetVideoBufferWidth()), static_cast<float>(pie_GetVideoBufferHeight()), 0.f);
	enableRect();
	for (const auto &line : lines)
	{
		pie_ActivateShader(SHADER_LINE, glm::vec2(line.x, line.y), glm::vec2(line.z, line.w), color, mat);
		glDrawArrays(GL_LINES, 0, 2);
	}
	pie_DeactivateShader();
	disableRect();
}

/**
 *	Assumes render mode set up externally, draws filled rectangle.
 */
static void pie_DrawRect(float x0, float y0, float x1, float y1, PIELIGHT colour)
{
	if (x0 > x1)
	{
		std::swap(x0, x1);
	}
	if (y0 > y1)
	{
		std::swap(y0, y1);
	}
	const auto& center = Vector2f(x0, y0);
	const auto& mvp = defaultProjectionMatrix() * glm::translate(Vector3f(center, 0.f)) * glm::scale(x1 - x0, y1 - y0, 1.f);

	pie_ActivateShader(SHADER_RECT, mvp,
		glm::vec4(colour.vector[0] / 255.f, colour.vector[1] / 255.f, colour.vector[2] / 255.f, colour.vector[3] / 255.f));
	enableRect();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	disableRect();
	pie_DeactivateShader();
}

void iV_ShadowBox(int x0, int y0, int x1, int y1, int pad, PIELIGHT first, PIELIGHT second, PIELIGHT fill)
{
	pie_SetRendMode(REND_OPAQUE);
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_DrawRect(x0 + pad, y0 + pad, x1 - pad, y1 - pad, fill);
	iV_Box2(x0, y0, x1, y1, first, second);
}

/***************************************************************************/

void iV_Box2(int x0, int y0, int x1, int y1, PIELIGHT first, PIELIGHT second)
{
	pie_SetTexturePage(TEXPAGE_NONE);
	const glm::mat4 mat = glm::ortho(0.f, static_cast<float>(pie_GetVideoBufferWidth()), static_cast<float>(pie_GetVideoBufferHeight()), 0.f);
	const glm::vec4 firstColor(
		first.vector[0] / 255.f,
		first.vector[1] / 255.f,
		first.vector[2] / 255.f,
		first.vector[3] / 255.f
	);
	enableRect();
	pie_ActivateShader(SHADER_LINE, glm::vec2(x0, y1), glm::vec2(x0, y0), firstColor, mat);
	glDrawArrays(GL_LINES, 0, 2);
	pie_ActivateShader(SHADER_LINE, glm::vec2(x0, y0), glm::vec2(x1, y0), firstColor, mat);
	glDrawArrays(GL_LINES, 0, 2);

	const glm::vec4 secondColor(
		second.vector[0] / 255.f,
		second.vector[1] / 255.f,
		second.vector[2] / 255.f,
		second.vector[3] / 255.f
	);
	pie_ActivateShader(SHADER_LINE, glm::vec2(x1, y0), glm::vec2(x1, y1), secondColor, mat);
	glDrawArrays(GL_LINES, 0, 2);
	pie_ActivateShader(SHADER_LINE, glm::vec2(x0, y1), glm::vec2(x1, y1), secondColor, mat);
	glDrawArrays(GL_LINES, 0, 2);
	pie_DeactivateShader();
	disableRect();
}

/***************************************************************************/

void pie_BoxFill(int x0, int y0, int x1, int y1, PIELIGHT colour, REND_MODE rendermode)
{
	pie_SetRendMode(rendermode);
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_DrawRect(x0, y0, x1, y1, colour);
}

/***************************************************************************/

void iV_TransBoxFill(float x0, float y0, float x1, float y1)
{
	pie_UniTransBoxFill(x0, y0, x1, y1, WZCOL_TRANSPARENT_BOX);
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

static void iv_DrawImageImpl(Vector2i offset, Vector2i size, Vector2f TextureUV, Vector2f TextureSize, PIELIGHT colour, const glm::mat4 &modelViewProjection, SHADER_MODE mode = SHADER_TEXRECT)
{
	glm::mat4 transformMat = modelViewProjection * glm::translate(offset.x, offset.y, 0) * glm::scale(size.x, size.y, 1);

	pie_ActivateShader(mode,
		transformMat,
		Vector2f(TextureUV.x, TextureUV.y),
		Vector2f(TextureSize.x, TextureSize.y),
		glm::vec4(colour.vector[0] / 255.f, colour.vector[1] / 255.f, colour.vector[2] / 255.f, colour.vector[3] / 255.f), 0);
	enableRect();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	disableRect();
	pie_DeactivateShader();
}

void iV_DrawImage(GLuint TextureID, Vector2i Position, Vector2i offset, Vector2i size, float angle, REND_MODE mode, PIELIGHT colour)
{
	pie_SetRendMode(mode);
	pie_SetTexturePage(TEXPAGE_EXTERN);
	glBindTexture(GL_TEXTURE_2D, TextureID);

	glm::mat4 mvp = defaultProjectionMatrix() * glm::translate(Position.x, Position.y, 0) * glm::rotate(angle, glm::vec3(0.f, 0.f, 1.f));

	iv_DrawImageImpl(offset, size, Vector2f(0.f, 0.f), Vector2f(1.f, 1.f), colour, mvp);
}

void iV_DrawImageText(GLuint TextureID, Vector2i Position, Vector2i offset, Vector2i size, float angle, REND_MODE mode, PIELIGHT colour)
{
	pie_SetRendMode(mode);
	pie_SetTexturePage(TEXPAGE_EXTERN);
	glBindTexture(GL_TEXTURE_2D, TextureID);

	glm::mat4 mvp = defaultProjectionMatrix() * glm::translate(Position.x, Position.y, 0) * glm::rotate(angle, glm::vec3(0.f, 0.f, 1.f));

	iv_DrawImageImpl(offset, size, Vector2f(0.f, 0.f), Vector2f(1.f, 1.f), colour, mvp, SHADER_TEXT);
}

static void pie_DrawImage(IMAGEFILE *imageFile, int id, Vector2i size, const PIERECT *dest, PIELIGHT colour, const glm::mat4 &modelViewProjection)
{
	ImageDef const &image2 = imageFile->imageDefs[id];
	GLuint texPage = imageFile->pages[image2.TPageID].id;
	GLfloat invTextureSize = 1.f / imageFile->pages[image2.TPageID].size;
	float tu = image2.Tu * invTextureSize;
	float tv = image2.Tv * invTextureSize;
	float su = size.x * invTextureSize;
	float sv = size.y * invTextureSize;

	glm::mat4 mvp = modelViewProjection * glm::translate(dest->x, dest->y, 0);

	pie_SetTexturePage(texPage);
	iv_DrawImageImpl(Vector2i(0, 0), Vector2i(dest->w, dest->h), Vector2f(tu, tv), Vector2f(su, sv), colour, mvp);
}

static Vector2i makePieImage(IMAGEFILE *imageFile, unsigned id, PIERECT *dest, int x, int y)
{
	ImageDef const &image = imageFile->imageDefs[id];
	Vector2i pieImage;
	pieImage.x = image.Width;
	pieImage.y = image.Height;
	if (dest != nullptr)
	{
		dest->x = x + image.XOffset;
		dest->y = y + image.YOffset;
		dest->w = image.Width;
		dest->h = image.Height;
	}
	return pieImage;
}

void iV_DrawImage2(const QString &filename, float x, float y, float width, float height)
{
	ImageDef *image = iV_GetImage(filename);
	const GLfloat invTextureSize = image->invTextureSize;
	const int tu = image->Tu;
	const int tv = image->Tv;
	const int w = width > 0 ? width : image->Width;
	const int h = height > 0 ? height : image->Height;
	x += image->XOffset;
	y += image->YOffset;
	pie_SetTexturePage(image->textureId);
	pie_SetRendMode(REND_ALPHA);

	glm::mat4 mvp = defaultProjectionMatrix() * glm::translate(x, y, 0);
	iv_DrawImageImpl(Vector2i(0, 0), Vector2i(w, h),
		Vector2f(tu * invTextureSize, tv * invTextureSize),
		Vector2f(image->Width * invTextureSize, image->Height * invTextureSize),
		WZCOL_WHITE, mvp);
}

void iV_DrawImage(IMAGEFILE *ImageFile, UWORD ID, int x, int y, const glm::mat4 &modelViewProjection)
{
	if (!assertValidImage(ImageFile, ID))
	{
		return;
	}

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	pie_SetRendMode(REND_ALPHA);

	pie_DrawImage(ImageFile, ID, pieImage, &dest, WZCOL_WHITE, modelViewProjection);
}

void iV_DrawImageTc(Image image, Image imageTc, int x, int y, PIELIGHT colour, const glm::mat4 &modelViewProjection)
{
	if (!assertValidImage(image.images, image.id) || !assertValidImage(imageTc.images, imageTc.id))
	{
		return;
	}

	PIERECT dest;
	Vector2i pieImage   = makePieImage(image.images, image.id, &dest, x, y);
	Vector2i pieImageTc = makePieImage(imageTc.images, imageTc.id);

	pie_SetRendMode(REND_ALPHA);

	pie_DrawImage(image.images, image.id, pieImage, &dest, WZCOL_WHITE, modelViewProjection);
	pie_DrawImage(imageTc.images, imageTc.id, pieImageTc, &dest, colour, modelViewProjection);
}

// Repeat a texture
void iV_DrawImageRepeatX(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Width, const glm::mat4 &modelViewProjection)
{
	assertValidImage(ImageFile, ID);
	const ImageDef *Image = &ImageFile->imageDefs[ID];

	pie_SetRendMode(REND_OPAQUE);

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	unsigned hRemainder = Width % Image->Width;

	for (unsigned hRep = 0; hRep < Width / Image->Width; hRep++)
	{
		pie_DrawImage(ImageFile, ID, pieImage, &dest, WZCOL_WHITE, modelViewProjection);
		dest.x += Image->Width;
	}

	// draw remainder
	if (hRemainder > 0)
	{
		pieImage.x = hRemainder;
		dest.w = hRemainder;
		pie_DrawImage(ImageFile, ID, pieImage, &dest, WZCOL_WHITE, modelViewProjection);
	}
}

void iV_DrawImageRepeatY(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Height, const glm::mat4 &modelViewProjection)
{
	assertValidImage(ImageFile, ID);
	const ImageDef *Image = &ImageFile->imageDefs[ID];

	pie_SetRendMode(REND_OPAQUE);

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	unsigned vRemainder = Height % Image->Height;

	for (unsigned vRep = 0; vRep < Height / Image->Height; vRep++)
	{
		pie_DrawImage(ImageFile, ID, pieImage, &dest, WZCOL_WHITE, modelViewProjection);
		dest.y += Image->Height;
	}

	// draw remainder
	if (vRemainder > 0)
	{
		pieImage.y = vRemainder;
		dest.h = vRemainder;
		pie_DrawImage(ImageFile, ID, pieImage, &dest, WZCOL_WHITE, modelViewProjection);
	}
}

bool pie_InitRadar(void)
{
	radarGfx = new GFX(GFX_TEXTURE, GL_TRIANGLE_STRIP, 2);
	return true;
}

bool pie_ShutdownRadar(void)
{
	delete radarGfx;
	radarGfx = nullptr;
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
void pie_RenderRadar(const glm::mat4 &modelViewProjectionMatrix)
{
	pie_SetRendMode(REND_ALPHA);
	radarGfx->draw(modelViewProjectionMatrix);
}

/// Load and display a random backdrop picture.
void pie_LoadBackDrop(SCREENTYPE screenType)
{
	switch (screenType)
	{
	case SCREEN_RANDOMBDROP:
		screen_SetRandomBackdrop("texpages/bdrops/", "backdrop");
		break;
	case SCREEN_MISSIONEND:
		screen_SetRandomBackdrop("texpages/bdrops/", "missionend");
		break;
	case SCREEN_CREDITS:
		screen_SetRandomBackdrop("texpages/bdrops/", "credits");
		break;
	}
}

glm::mat4 defaultProjectionMatrix()
{
        float w = pie_GetVideoBufferWidth();
        float h = pie_GetVideoBufferHeight();

        return glm::ortho(0.f, static_cast<float>(w), static_cast<float>(h), 0.f);
}
