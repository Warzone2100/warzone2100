/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include "lib/framework/fixedpoint.h"
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
#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

static GFX *radarGfx = nullptr;

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

GFX::GFX(GFXTYPE type, int coordsPerVertex) : mType(type), mCoordsPerVertex(coordsPerVertex), mSize(0)
{
}

void GFX::loadTexture(const char *filename, int maxWidth /*= -1*/, int maxHeight /*= -1*/)
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
		scaleImageMaxSize(&image, maxWidth, maxHeight);
		makeTexture(image.width, image.height, iV_getPixelFormat(&image), image.bmp);
		iV_unloadImage(&image);
	}
}

void GFX::makeTexture(int width, int height, const gfx_api::pixel_format& format, const void *image)
{
	ASSERT(mType == GFX_TEXTURE, "Wrong GFX type");
	if (mTexture)
		delete mTexture;
	if (width > 0 && height > 0)
	{
		mTexture = gfx_api::context::get().create_texture(1, width, height, format);
		if (image != nullptr)
			mTexture->upload(0u, 0u, 0u, width, height, format, image);
	}
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
	mTexture->upload(0u, 0u, 0u, width, height, mFormat, image);
}

void GFX::buffers(int vertices, const void *vertBuf, const void *auxBuf)
{
	if (!mBuffers[VBO_VERTEX])
		mBuffers[VBO_VERTEX] = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer);
	mBuffers[VBO_VERTEX]->upload(vertices * mCoordsPerVertex * sizeof(gfx_api::gfxFloat), vertBuf);

	if (mType == GFX_TEXTURE)
	{
		if (!mBuffers[VBO_TEXCOORD])
			mBuffers[VBO_TEXCOORD] = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer);
		mBuffers[VBO_TEXCOORD]->upload(vertices * 2 * sizeof(gfx_api::gfxFloat), auxBuf);
	}
	else if (mType == GFX_COLOUR)
	{
		// reusing texture buffer for colours for now
		if (!mBuffers[VBO_TEXCOORD])
			mBuffers[VBO_TEXCOORD] = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer);
		mBuffers[VBO_TEXCOORD]->upload(vertices * 4 * sizeof(gfx_api::gfxByte), auxBuf);
	}
	mSize = vertices;
}

GFX::~GFX()
{
	for (auto* buffer : mBuffers)
	{
		delete buffer;
	}
	if (mTexture)
		delete mTexture;
}

void iV_Line(int x0, int y0, int x1, int y1, PIELIGHT colour)
{
	const glm::vec4 color(
		colour.vector[0] / 255.f,
		colour.vector[1] / 255.f,
		colour.vector[2] / 255.f,
		colour.vector[3] / 255.f
	);
	const auto &mat = glm::ortho(0.f, static_cast<float>(pie_GetVideoBufferWidth()), static_cast<float>(pie_GetVideoBufferHeight()), 0.f);
	gfx_api::LinePSO::get().bind();
	gfx_api::LinePSO::get().bind_constants({ mat, glm::vec2(x0, y0), glm::vec2(x1, y1), color });
	gfx_api::LinePSO::get().bind_vertex_buffers(pie_internal::rectBuffer);
	gfx_api::LinePSO::get().draw(2, 0);
	gfx_api::LinePSO::get().unbind_vertex_buffers(pie_internal::rectBuffer);
}

void iV_Lines(const std::vector<glm::ivec4> &lines, PIELIGHT colour)
{
	const glm::vec4 color(
		colour.vector[0] / 255.f,
		colour.vector[1] / 255.f,
		colour.vector[2] / 255.f,
		colour.vector[3] / 255.f
	);
	const auto &mat = glm::ortho(0.f, static_cast<float>(pie_GetVideoBufferWidth()), static_cast<float>(pie_GetVideoBufferHeight()), 0.f);
	gfx_api::LinePSO::get().bind();
	gfx_api::LinePSO::get().bind_vertex_buffers(pie_internal::rectBuffer);
	for (const auto &line : lines)
	{
		gfx_api::LinePSO::get().bind_constants({ mat, glm::vec2(line.x, line.y), glm::vec2(line.z, line.w), color });
		gfx_api::LinePSO::get().draw(2, 0);
	}
	gfx_api::LinePSO::get().unbind_vertex_buffers(pie_internal::rectBuffer);
}

/**
 *	Assumes render mode set up externally, draws filled rectangle.
 */
template<typename PSO>
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
	const auto& mvp = defaultProjectionMatrix() * glm::translate(Vector3f(center, 0.f)) * glm::scale(glm::vec3(x1 - x0, y1 - y0, 1.f));

	PSO::get().bind();
	PSO::get().bind_constants({ mvp, glm::vec2{}, glm::vec2{},
		glm::vec4(colour.vector[0] / 255.f, colour.vector[1] / 255.f, colour.vector[2] / 255.f, colour.vector[3] / 255.f) });
	PSO::get().bind_vertex_buffers(pie_internal::rectBuffer);
	PSO::get().draw(4, 0);
	PSO::get().unbind_vertex_buffers(pie_internal::rectBuffer);
}

void pie_DrawMultiRect(std::vector<PIERECT_DrawRequest> rects)
{
	if (rects.empty()) { return; }

	const auto projectionMatrix = defaultProjectionMatrix();
	bool didEnableRect = false;
	gfx_api::BoxFillPSO::get().bind();

	for (auto it = rects.begin(); it != rects.end(); ++it)
	{
		if (it->x0 > it->x1)
		{
			std::swap(it->x0, it->x1);
		}
		if (it->y0 > it->y1)
		{
			std::swap(it->y0, it->y1);
		}
		const auto& colour = it->color;
		const auto& center = Vector2f(it->x0, it->y0);
		const auto& mvp = projectionMatrix * glm::translate(Vector3f(center, 0.f)) * glm::scale(glm::vec3(it->x1 - it->x0, it->y1 - it->y0, 1.f));
		gfx_api::BoxFillPSO::get().bind_constants({ mvp, glm::vec2(0.f), glm::vec2(0.f),
			glm::vec4(colour.vector[0] / 255.f, colour.vector[1] / 255.f, colour.vector[2] / 255.f, colour.vector[3] / 255.f) });
		if (!didEnableRect)
		{
			gfx_api::BoxFillPSO::get().bind_vertex_buffers(pie_internal::rectBuffer);
			didEnableRect = true;
		}

		gfx_api::BoxFillPSO::get().draw(4, 0);
	}
	gfx_api::BoxFillPSO::get().unbind_vertex_buffers(pie_internal::rectBuffer);
}

void iV_ShadowBox(int x0, int y0, int x1, int y1, int pad, PIELIGHT first, PIELIGHT second, PIELIGHT fill)
{
	pie_DrawRect<gfx_api::ShadowBox2DPSO>(x0 + pad, y0 + pad, x1 - pad, y1 - pad, fill);
	iV_Box2(x0, y0, x1, y1, first, second);
}

/***************************************************************************/

void iV_Box2(int x0, int y0, int x1, int y1, PIELIGHT first, PIELIGHT second)
{
	const glm::mat4 mat = glm::ortho(0.f, static_cast<float>(pie_GetVideoBufferWidth()), static_cast<float>(pie_GetVideoBufferHeight()), 0.f);
	const glm::vec4 firstColor(
		first.vector[0] / 255.f,
		first.vector[1] / 255.f,
		first.vector[2] / 255.f,
		first.vector[3] / 255.f
	);
	gfx_api::LinePSO::get().bind();
	gfx_api::LinePSO::get().bind_vertex_buffers(pie_internal::rectBuffer);
	gfx_api::LinePSO::get().bind_constants({ mat, glm::vec2(x0, y1), glm::vec2(x0, y0), firstColor });
	gfx_api::LinePSO::get().draw(2, 0);
	gfx_api::LinePSO::get().bind_constants({ mat, glm::vec2(x0, y0), glm::vec2(x1, y0), firstColor });
	gfx_api::LinePSO::get().draw(2, 0);

	const glm::vec4 secondColor(
		second.vector[0] / 255.f,
		second.vector[1] / 255.f,
		second.vector[2] / 255.f,
		second.vector[3] / 255.f
	);
	gfx_api::LinePSO::get().bind_constants({ mat, glm::vec2(x1, y0), glm::vec2(x1, y1), secondColor });
	gfx_api::LinePSO::get().draw(2, 0);
	gfx_api::LinePSO::get().bind_constants({ mat, glm::vec2(x0, y1), glm::vec2(x1, y1), secondColor });
	gfx_api::LinePSO::get().draw(2, 0);
	gfx_api::LinePSO::get().unbind_vertex_buffers(pie_internal::rectBuffer);
}

/***************************************************************************/

void pie_BoxFill(int x0, int y0, int x1, int y1, PIELIGHT colour)
{
	pie_DrawRect<gfx_api::BoxFillPSO>(x0, y0, x1, y1, colour);
}

void pie_BoxFill_alpha(int x0, int y0, int x1, int y1, PIELIGHT colour)
{
	pie_DrawRect<gfx_api::BoxFillAlphaPSO>(x0, y0, x1, y1, colour);
}

/***************************************************************************/

void iV_TransBoxFill(float x0, float y0, float x1, float y1)
{
	pie_UniTransBoxFill(x0, y0, x1, y1, WZCOL_TRANSPARENT_BOX);
}

/***************************************************************************/

void pie_UniTransBoxFill(float x0, float y0, float x1, float y1, PIELIGHT light)
{
	pie_DrawRect<gfx_api::UniTransBoxPSO>(x0, y0, x1, y1, light);
}

/***************************************************************************/

static bool assertValidImage(IMAGEFILE *imageFile, unsigned id)
{
	ASSERT_OR_RETURN(false, id < imageFile->imageDefs.size(), "Out of range 1: %u/%d", id, (int)imageFile->imageDefs.size());
	ASSERT_OR_RETURN(false, imageFile->imageDefs[id].TPageID < imageFile->pages.size(), "Out of range 2: %u", imageFile->imageDefs[id].TPageID);
	return true;
}

template<typename PSO>
static void iv_DrawImageImpl(gfx_api::texture& TextureID, Vector2f offset, Vector2f size, Vector2f TextureUV, Vector2f TextureSize, PIELIGHT colour, const glm::mat4 &modelViewProjection, SHADER_MODE mode = SHADER_TEXRECT)
{
	glm::mat4 transformMat = modelViewProjection * glm::translate(glm::vec3(offset.x, offset.y, 0.f)) * glm::scale(glm::vec3(size.x, size.y, 1.f));

	PSO::get().bind();
	PSO::get().bind_constants({ transformMat,
		TextureUV,
		TextureSize,
		glm::vec4(colour.vector[0] / 255.f, colour.vector[1] / 255.f, colour.vector[2] / 255.f, colour.vector[3] / 255.f), 0});
	PSO::get().bind_textures(&TextureID);
	PSO::get().bind_vertex_buffers(pie_internal::rectBuffer);
	PSO::get().draw(4, 0);
	PSO::get().unbind_vertex_buffers(pie_internal::rectBuffer);
}

void iV_DrawImageAnisotropic(gfx_api::texture& TextureID, Vector2i Position, Vector2f offset, Vector2f size, float angle, PIELIGHT colour)
{
	glm::mat4 mvp = defaultProjectionMatrix() * glm::translate(glm::vec3(Position.x, Position.y, 0)) * glm::rotate(angle, glm::vec3(0.f, 0.f, 1.f));

	iv_DrawImageImpl<gfx_api::DrawImageAnisotropicPSO>(TextureID, offset, size, Vector2f(0.f, 0.f), Vector2f(1.f, 1.f), colour, mvp);
}

void iV_DrawImageText(gfx_api::texture& TextureID, Vector2i Position, Vector2f offset, Vector2f size, float angle, PIELIGHT colour)
{
	glm::mat4 mvp = defaultProjectionMatrix() * glm::translate(glm::vec3(Position.x, Position.y, 0)) * glm::rotate(RADIANS(angle), glm::vec3(0.f, 0.f, 1.f));

	iv_DrawImageImpl<gfx_api::DrawImageTextPSO>(TextureID, offset, size, Vector2f(0.f, 0.f), Vector2f(1.f, 1.f), colour, mvp, SHADER_TEXT);
}

static void pie_DrawImage(IMAGEFILE *imageFile, int id, Vector2i size, const PIERECT *dest, PIELIGHT colour, const glm::mat4 &modelViewProjection, Vector2i textureInset = Vector2i(0, 0))
{
	ImageDef const &image2 = imageFile->imageDefs[id];
	uint32_t texPage = imageFile->pages[image2.TPageID].id;
	gfx_api::gfxFloat invTextureSize = 1.f / (float)imageFile->pages[image2.TPageID].size;
	float tu = (float)(image2.Tu + textureInset.x) * invTextureSize;
	float tv = (float)(image2.Tv + textureInset.y) * invTextureSize;
	float su = (float)(size.x - (textureInset.x * 2)) * invTextureSize;
	float sv = (float)(size.y - (textureInset.y * 2)) * invTextureSize;

	glm::mat4 mvp = modelViewProjection * glm::translate(glm::vec3((float)dest->x, (float)dest->y, 0.f));

	iv_DrawImageImpl<gfx_api::DrawImagePSO>(pie_Texture(texPage), Vector2i(0, 0), Vector2i(dest->w, dest->h), Vector2f(tu, tv), Vector2f(su, sv), colour, mvp);
}

static void pie_DrawMultipleImages(const std::list<PieDrawImageRequest>& requests)
{
	if (requests.empty()) { return; }

	bool didEnableRect = false;
	gfx_api::DrawImagePSO::get().bind();
	
	for (auto& request : requests)
	{
		// The following is the equivalent of:
		// pie_DrawImage(request.imageFile, request.ID, request.size, &request.dest, request.colour, request.modelViewProjection, request.textureInset)
		// but is tweaked to use custom implementation of iv_DrawImageImpl that does not disable the shader after every glDrawArrays call.

		ImageDef const &image2 = request.imageFile->imageDefs[request.ID];
		uint32_t texPage = request.imageFile->pages[image2.TPageID].id;
		gfx_api::gfxFloat invTextureSize = 1.f / (float)request.imageFile->pages[image2.TPageID].size;
		float tu = (float)(image2.Tu + request.textureInset.x) * invTextureSize;
		float tv = (float)(image2.Tv + request.textureInset.y) * invTextureSize;
		float su = (float)(request.size.x - (request.textureInset.x * 2)) * invTextureSize;
		float sv = (float)(request.size.y - (request.textureInset.y * 2)) * invTextureSize;

		glm::mat4 mvp = request.modelViewProjection * glm::translate(glm::vec3((float)request.dest.x, (float)request.dest.y, 0.f));

		gfx_api::texture& TextureID = pie_Texture(texPage);
		Vector2f offset = Vector2f(0.f, 0.f);
		Vector2f size = Vector2f(request.dest.w, request.dest.h);
		Vector2f TextureUV = Vector2f(tu, tv);
		Vector2f TextureSize = Vector2f(su, sv);
		glm::mat4 transformMat = mvp * glm::translate(glm::vec3(offset.x, offset.y, 0.f)) * glm::scale(glm::vec3(size.x, size.y, 1.f));

		gfx_api::DrawImagePSO::get().bind_constants({ transformMat,
			TextureUV,
			TextureSize,
			glm::vec4(request.colour.vector[0] / 255.f, request.colour.vector[1] / 255.f, request.colour.vector[2] / 255.f, request.colour.vector[3] / 255.f), 0});

		gfx_api::DrawImagePSO::get().bind_textures(&TextureID);

		if (!didEnableRect)
		{
			gfx_api::DrawImagePSO::get().bind_vertex_buffers(pie_internal::rectBuffer);
			didEnableRect = true;
		}

		gfx_api::DrawImagePSO::get().draw(4, 0);
	}
	gfx_api::DrawImagePSO::get().unbind_vertex_buffers(pie_internal::rectBuffer);
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

void iV_DrawImage2(const WzString &filename, float x, float y, float width, float height)
{
	ImageDef *image = iV_GetImage(filename);
	const gfx_api::gfxFloat invTextureSize = image->invTextureSize;
	const int tu = image->Tu;
	const int tv = image->Tv;
	const int w = width > 0 ? width : image->Width;
	const int h = height > 0 ? height : image->Height;
	x += image->XOffset;
	y += image->YOffset;

	glm::mat4 mvp = defaultProjectionMatrix() * glm::translate(glm::vec3(x, y, 0));
	iv_DrawImageImpl<gfx_api::DrawImagePSO>(pie_Texture(image->textureId), Vector2i(0, 0), Vector2i(w, h),
		Vector2f(tu * invTextureSize, tv * invTextureSize),
		Vector2f(image->Width * invTextureSize, image->Height * invTextureSize),
		WZCOL_WHITE, mvp);
}

void iV_DrawImage(IMAGEFILE *ImageFile, UWORD ID, int x, int y, const glm::mat4 &modelViewProjection, BatchedImageDrawRequests* pBatchedRequests)
{
	if (!assertValidImage(ImageFile, ID))
	{
		return;
	}

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	if (pBatchedRequests == nullptr)
	{
		gfx_api::DrawImagePSO::get().bind();
		pie_DrawImage(ImageFile, ID, pieImage, &dest, WZCOL_WHITE, modelViewProjection);
	}
	else
	{
		pBatchedRequests->queuePieImageDraw(REND_ALPHA, ImageFile, ID, pieImage, dest, WZCOL_WHITE, modelViewProjection);
		pBatchedRequests->draw(); // draw only if not deferred
	}
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

	gfx_api::DrawImagePSO::get().bind();

	pie_DrawImage(image.images, image.id, pieImage, &dest, WZCOL_WHITE, modelViewProjection);
	pie_DrawImage(imageTc.images, imageTc.id, pieImageTc, &dest, colour, modelViewProjection);
}

// Repeat a texture
void iV_DrawImageRepeatX(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Width, const glm::mat4 &modelViewProjection, bool enableHorizontalTilingSeamWorkaround, BatchedImageDrawRequests *pBatchedRequests)
{
	static BatchedImageDrawRequests localBatch;
	if (pBatchedRequests == nullptr)
	{
		localBatch.clear();
		pBatchedRequests = &localBatch;
	}

	assertValidImage(ImageFile, ID);
	const ImageDef *Image = &ImageFile->imageDefs[ID];

	REND_MODE mode = REND_OPAQUE;

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	unsigned int usableImageWidth = Image->Width;
	unsigned int imageXInset = 0;
	if (enableHorizontalTilingSeamWorkaround)
	{
		// Inset the portion of the image that is used by 1 on both the left + right sides
		usableImageWidth -= 2;
		imageXInset = 1;
		dest.w -= 2;
	}
	assert(usableImageWidth > 0);

	unsigned hRemainder = (Width % usableImageWidth);

	for (unsigned hRep = 0; hRep < Width / usableImageWidth; hRep++)
	{
		pBatchedRequests->queuePieImageDraw(mode, ImageFile, ID, pieImage, dest, WZCOL_WHITE, modelViewProjection, Vector2i(imageXInset, 0));
		dest.x += usableImageWidth;
	}

	// draw remainder
	if (hRemainder > 0)
	{
		pieImage.x = hRemainder;
		dest.w = hRemainder;
		pBatchedRequests->queuePieImageDraw(mode, ImageFile, ID, pieImage, dest, WZCOL_WHITE, modelViewProjection, Vector2i(imageXInset, 0));
	}

	// draw batched requests (unless batch is deferred)
	pBatchedRequests->draw();
}

void iV_DrawImageRepeatY(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Height, const glm::mat4 &modelViewProjection, BatchedImageDrawRequests* pBatchedRequests)
{
	static BatchedImageDrawRequests localBatch;
	if (pBatchedRequests == nullptr)
	{
		localBatch.clear();
		pBatchedRequests = &localBatch;
	}

	assertValidImage(ImageFile, ID);
	const ImageDef *Image = &ImageFile->imageDefs[ID];

	REND_MODE mode = REND_OPAQUE;

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	unsigned vRemainder = Height % Image->Height;

	for (unsigned vRep = 0; vRep < Height / Image->Height; vRep++)
	{
		pBatchedRequests->queuePieImageDraw(mode, ImageFile, ID, pieImage, dest, WZCOL_WHITE, modelViewProjection);
		dest.y += Image->Height;
	}

	// draw remainder
	if (vRemainder > 0)
	{
		pieImage.y = vRemainder;
		dest.h = vRemainder;
		pBatchedRequests->queuePieImageDraw(mode, ImageFile, ID, pieImage, dest, WZCOL_WHITE, modelViewProjection);
	}

	// draw batched requests (unless batch is deferred)
	pBatchedRequests->draw();
}

bool pie_InitRadar()
{
	radarGfx = new GFX(GFX_TEXTURE, 2);
	return true;
}

bool pie_ShutdownRadar()
{
	delete radarGfx;
	radarGfx = nullptr;
	pie_ViewingWindow_Shutdown();
	return true;
}

void pie_SetRadar(gfx_api::gfxFloat x, gfx_api::gfxFloat y, gfx_api::gfxFloat width, gfx_api::gfxFloat height, int twidth, int theight)
{
	radarGfx->makeTexture(twidth, theight);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // Want GL_LINEAR (or GL_LINEAR_MIPMAP_NEAREST) for min filter, but GL_NEAREST for mag filter. // TODO: Add a gfx_api::sampler_type to handle this case? bilinear, but nearest for mag?
	gfx_api::gfxFloat texcoords[] = { 0.0f, 0.0f,  1.0f, 0.0f,  0.0f, 1.0f,  1.0f, 1.0f };
	gfx_api::gfxFloat vertices[] = { x, y,  x + width, y,  x, y + height,  x + width, y + height };
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
	gfx_api::RadarPSO::get().bind();
	gfx_api::RadarPSO::get().bind_constants({ modelViewProjectionMatrix, glm::vec2(0), glm::vec2(0), glm::vec4(1), 0 });
	radarGfx->draw<gfx_api::RadarPSO>(modelViewProjectionMatrix);
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
	}
}

glm::mat4 defaultProjectionMatrix()
{
        float w = pie_GetVideoBufferWidth();
        float h = pie_GetVideoBufferHeight();

        return glm::ortho(0.f, static_cast<float>(w), static_cast<float>(h), 0.f);
}


void BatchedImageDrawRequests::clear()
{
	ASSERT(_imageDrawRequests.empty(), "Clearing a BatchedImageDrawRequests that isn't empty. Images have not been drawn!");
	_imageDrawRequests.clear();
}

bool BatchedImageDrawRequests::draw(bool force /*= false*/)
{
	if (deferRender && !force) { return false; }
	pie_DrawMultipleImages(_imageDrawRequests);
	_imageDrawRequests.clear();
	return true;
}
