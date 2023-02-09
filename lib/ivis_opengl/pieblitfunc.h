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
 * pieBlitFunc.h
 *
 * patch for existing ivis rectangle draw functions.
 *
 */
/***************************************************************************/

#ifndef _pieBlitFunc_h
#define _pieBlitFunc_h

/***************************************************************************/

#include "gfx_api.h"
#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/vector.h"
#include "lib/framework/wzstring.h"
#include "lib/framework/geometry.h"
#include <glm/mat4x4.hpp>
#include "piedef.h"
#include "ivisdef.h"
#include "pietypes.h"
#include "piepalette.h"
#include "pieclip.h"
#include <list>
#include <map>

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/
#define NUM_BACKDROPS 7

/* These are Qamly's hacks used for map previews. They need to be power of
 * two sized until we clean up this mess properly. */
#define BACKDROP_HACK_WIDTH 512
#define BACKDROP_HACK_HEIGHT 512

/***************************************************************************/
/*
 *	Global Classes
 */
/***************************************************************************/

enum GFXTYPE
{
	GFX_TEXTURE,
	GFX_COLOUR,
	GFX_COUNT
};

/// Generic graphics using VBOs drawing class
class GFX
{
public:
	/// Initialize class and allocate GPU resources
	GFX(GFXTYPE type, int coordsPerVertex = 3);

	/// Destroy GPU held resources
	~GFX();

	GFX( const GFX& other ) = delete; // non construction-copyable
	GFX& operator=( const GFX& ) = delete; // non copyable

	/// Load texture data from file, allocate space for it, and put it on the GPU
	void loadTexture(const char *filename, gfx_api::texture_type textureType, int maxWidth = -1, int maxHeight = -1);

	void loadTexture(iV_Image&& bitmap, gfx_api::texture_type textureType, const std::string& filename, int maxWidth = -1, int maxHeight = -1);

	/// Allocate space on the GPU for texture of given parameters. If image is non-NULL,
	/// then that memory buffer is uploaded to the GPU.
	void makeTexture(size_t width, size_t height, const gfx_api::pixel_format& format = gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8, const std::string& debugName = "");
	void makeCompatibleTexture(const iV_Image* image /*= nullptr*/, const std::string& filename);

	/// Upload given memory buffer to already allocated texture space on the GPU
	void updateTexture(const iV_Image& image /*= nullptr*/);

	/// Upload vertex and texture buffer data to the GPU
	void buffers(int vertices, const void *vertBuf, const void *texBuf);

	/// Draw everything
	template<typename PSO>
	typename std::enable_if<(std::tuple_size<typename PSO::texture_tuple>::value != 0), void>::type draw(const glm::mat4 &modelViewProjectionMatrix)
	{
		if(!mTexture) return;
		PSO::get().bind_textures(mTexture);
		PSO::get().bind_vertex_buffers(mBuffers[VBO_VERTEX], mBuffers[VBO_TEXCOORD]);
		PSO::get().draw(mSize, 0);
		PSO::get().unbind_vertex_buffers(mBuffers[VBO_VERTEX], mBuffers[VBO_TEXCOORD]);
	}

	template<typename PSO>
	typename std::enable_if<(std::tuple_size<typename PSO::texture_tuple>::value == 0), void>::type draw(const glm::mat4 &modelViewProjectionMatrix)
	{
		PSO::get().bind_constants({ modelViewProjectionMatrix });
		PSO::get().bind_vertex_buffers(mBuffers[VBO_VERTEX], mBuffers[VBO_TEXCOORD]);
		PSO::get().draw(mSize, 0);
		PSO::get().unbind_vertex_buffers(mBuffers[VBO_VERTEX], mBuffers[VBO_TEXCOORD]);
	}

private:
	GFXTYPE mType;
	int mCoordsPerVertex;
	gfx_api::buffer* mBuffers[VBO_COUNT] = { nullptr };
	gfx_api::texture* mTexture = nullptr;
	int mSize;
};

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
glm::mat4 defaultProjectionMatrix();
void iV_ShadowBox(int x0, int y0, int x1, int y1, int pad, PIELIGHT first, PIELIGHT second, PIELIGHT fill);
void iV_Line(int x0, int y0, int x1, int y1, PIELIGHT colour);
void iV_Lines(const std::vector<glm::ivec4> &lines, PIELIGHT colour);
void iV_Box2(int x0, int y0, int x1, int y1, PIELIGHT first, PIELIGHT second);
static inline void iV_Box(int x0, int y0, int x1, int y1, PIELIGHT first)
{
	iV_Box2(x0, y0, x1, y1, first, first);
}
void pie_BoxFill(int x0, int y0, int x1, int y1, PIELIGHT colour);
void pie_BoxFillf(float x0, float y0, float x1, float y1, PIELIGHT colour);
void pie_BoxFill_alpha(int x0, int y0, int x1, int y1, PIELIGHT colour);
struct PIERECT_DrawRequest
{
	PIERECT_DrawRequest(int x0, int y0, int x1, int y1, PIELIGHT color)
	: x0(x0)
	, y0(y0)
	, x1(x1)
	, y1(y1)
	, color(color)
	{ }

	int x0;
	int y0;
	int x1;
	int y1;
	PIELIGHT color;
};
struct PIERECT_DrawRequest_f
{
	PIERECT_DrawRequest_f(float x0, float y0, float x1, float y1, PIELIGHT color)
	: x0(x0)
	, y0(y0)
	, x1(x1)
	, y1(y1)
	, color(color)
	{ }

	float x0;
	float y0;
	float x1;
	float y1;
	PIELIGHT color;
};
void pie_DrawMultiRect(std::vector<PIERECT_DrawRequest> rects);
class BatchedMultiRectRenderer
{
public:
	void resizeRectGroups(size_t count);
	void addRect(PIERECT_DrawRequest rectRequest, size_t rectGroup = 0);
	void addRectF(PIERECT_DrawRequest_f rectRequest, size_t rectGroup = 0);
	void drawAllRects(glm::mat4 projectionMatrix = defaultProjectionMatrix());
	void drawRects(optional<size_t> rectGroup, glm::mat4 projectionMatrix = defaultProjectionMatrix());
public:
	bool initialize();
	void clear();
	void reset();
private:
	struct UploadedRectsInstanceBufferInfo
	{
		gfx_api::buffer* buffer;
		size_t totalInstances = 0;
		struct GroupBufferInfo
		{
			size_t bufferOffset = 0;
			size_t instancesCount = 0;

			GroupBufferInfo(size_t bufferOffset, size_t instancesCount)
			: bufferOffset(bufferOffset), instancesCount(instancesCount)
			{ }
		};
		std::vector<GroupBufferInfo> groupInfo;
	};
	UploadedRectsInstanceBufferInfo uploadAllRectInstances();
private:
	bool useInstancedRendering = false;
	std::vector<std::vector<gfx_api::MultiRectPerInstanceInterleavedData>> groupsData;
	size_t totalAddedRects = 0;

	std::vector<gfx_api::MultiRectPerInstanceInterleavedData> instancesData;
	std::vector<gfx_api::buffer*> instanceDataBuffers;
	size_t currInstanceBufferIdx = 0;
	optional<UploadedRectsInstanceBufferInfo> currentUploadedRectInfo;
};
struct PIERECT  ///< Screen rectangle.
{
	float x, y, w, h;
};
struct PieDrawImageRequest {
public:
	PieDrawImageRequest(REND_MODE rendMode, IMAGEFILE *imageFile, int ID, Vector2i size, const PIERECT& dest, PIELIGHT colour, const glm::mat4& modelViewProjection, Vector2i textureInset = Vector2i(0, 0))
	: rendMode(rendMode),
	imageFile(imageFile),
	ID(ID),
	size(size),
	dest(dest),
	colour(colour),
	modelViewProjection(modelViewProjection),
	textureInset(textureInset)
	{ }

public:
	REND_MODE rendMode;

	IMAGEFILE *imageFile;
	int ID;
	Vector2i size;
	const PIERECT dest;
	PIELIGHT colour;
	glm::mat4 modelViewProjection;
	Vector2i textureInset;
};
struct BatchedImageDrawRequests {
public:
	BatchedImageDrawRequests(bool deferRender = false)
	: deferRender(deferRender)
	{ }
public:
	void queuePieImageDraw(REND_MODE rendMode, IMAGEFILE *imageFile, int id, Vector2i size, const PIERECT& dest, PIELIGHT colour, const glm::mat4& modelViewProjection, Vector2i textureInset = Vector2i(0, 0))
	{
		queuePieImageDraw(PieDrawImageRequest(rendMode, imageFile, id, size, dest, colour, modelViewProjection, textureInset));
	}
	void queuePieImageDraw(const PieDrawImageRequest& request)
	{
		_imageDrawRequests.push_back(request);
	}
	void queuePieImageDraw(const PieDrawImageRequest&& request)
	{
		_imageDrawRequests.push_back(std::move(request));
	}

	bool draw(bool force = false);
	void clear();
public:
	bool deferRender = false;
private:
	std::list<PieDrawImageRequest> _imageDrawRequests;
};

void iV_DrawImageAnisotropic(gfx_api::texture& TextureID, Vector2i Position, Vector2f offset, Vector2f size, float angle, PIELIGHT colour);
void iV_DrawImageText(gfx_api::texture& TextureID, Vector2f Position, Vector2f offset, Vector2f size, float angle, PIELIGHT colour);
void iV_DrawImageTextClipped(gfx_api::texture& TextureID, Vector2i textureSize, Vector2f Position, Vector2f offset, Vector2f size, float angle, PIELIGHT colour, WzRect clippingRect);
void iV_DrawImage(IMAGEFILE *ImageFile, UWORD ID, int x, int y, const glm::mat4 &modelViewProjection = defaultProjectionMatrix(), BatchedImageDrawRequests* pBatchedRequests = nullptr, uint8_t alpha = 255);
void iV_DrawImageFileAnisotropic(IMAGEFILE *ImageFile, UWORD ID, int x, int y, Vector2f size, const glm::mat4 &modelViewProjection = defaultProjectionMatrix(), uint8_t alpha = 255);
void iV_DrawImage2(const WzString &filename, float x, float y, float width = -0.0f, float height = -0.0f);
void iV_DrawImage2(const AtlasImageDef *image, float x, float y, float width = -0.0f, float height = -0.0f);
void iV_DrawImageTc(AtlasImage image, AtlasImage imageTc, int x, int y, PIELIGHT colour, const glm::mat4 &modelViewProjection = defaultProjectionMatrix());
void iV_DrawImageRepeatX(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Width, const glm::mat4 &modelViewProjection = defaultProjectionMatrix(), bool enableHorizontalTilingSeamWorkaround = false, BatchedImageDrawRequests* pBatchedRequests = nullptr);
void iV_DrawImageRepeatY(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Height, const glm::mat4 &modelViewProjection = defaultProjectionMatrix(), BatchedImageDrawRequests* pBatchedRequests = nullptr);

static inline void iV_DrawImageImage(AtlasImage image, int x, int y, uint8_t alpha = 255)
{
	iV_DrawImage(image.images, image.id, x, y, defaultProjectionMatrix(), nullptr, alpha);
}
static inline void iV_DrawImageTc(IMAGEFILE *imageFile, unsigned id, unsigned idTc, int x, int y, PIELIGHT colour)
{
	iV_DrawImageTc(AtlasImage(imageFile, id), AtlasImage(imageFile, idTc), x, y, colour);
}

void iV_TransBoxFill(float x0, float y0, float x1, float y1);
void pie_UniTransBoxFill(float x0, float y0, float x1, float y1, PIELIGHT colour);

bool pie_InitRadar();
bool pie_ShutdownRadar();
void pie_DownLoadRadar(const iV_Image& bitmap);
void pie_RenderRadar(const glm::mat4 &modelViewProjectionMatrix);
void pie_SetRadar(gfx_api::gfxFloat x, gfx_api::gfxFloat y, gfx_api::gfxFloat width, gfx_api::gfxFloat height, size_t twidth, size_t theight);

enum SCREENTYPE
{
	SCREEN_RANDOMBDROP,
	SCREEN_MISSIONEND,
};

void pie_LoadBackDrop(SCREENTYPE screenType);

#endif //
