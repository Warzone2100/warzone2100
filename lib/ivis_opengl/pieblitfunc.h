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
#include "glm/core/type.hpp"
#include "piedef.h"
#include "ivisdef.h"
#include "pietypes.h"
#include "piepalette.h"
#include "pieclip.h"

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
	GFX(GFXTYPE type, GLenum drawType = GL_TRIANGLES, int coordsPerVertex = 3);

	/// Destroy GPU held resources
	~GFX();

	/// Load texture data from file, allocate space for it, and put it on the GPU
	void loadTexture(const char *filename, GLenum filter = GL_LINEAR);

	/// Allocate space on the GPU for texture of given parameters. If image is non-NULL,
	/// then that memory buffer is uploaded to the GPU.
	void makeTexture(int width, int height, GLenum filter = GL_LINEAR, const gfx_api::pixel_format& format = gfx_api::pixel_format::rgba, const GLvoid *image = nullptr);

	/// Upload given memory buffer to already allocated texture space on the GPU
	void updateTexture(const GLvoid *image, int width = -1, int height = -1);

	/// Upload vertex and texture buffer data to the GPU
	void buffers(int vertices, const GLvoid *vertBuf, const GLvoid *texBuf);

	/// Draw everything
	void draw(const glm::mat4 &modelViewProjectionMatrix);

private:
	GFXTYPE mType;
	gfx_api::pixel_format mFormat;
	int mWidth;
	int mHeight;
	GLenum mdrawType;
	int mCoordsPerVertex;
	GLuint mBuffers[VBO_COUNT];
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
void pie_BoxFill(int x0, int y0, int x1, int y1, PIELIGHT colour, REND_MODE rendermode = REND_OPAQUE);
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
void pie_DrawMultiRect(std::vector<PIERECT_DrawRequest> rects, REND_MODE rendermode = REND_OPAQUE);
void iV_DrawImage(GLuint TextureID, Vector2i position, Vector2f offset, Vector2i size, float angle, REND_MODE mode, PIELIGHT colour);
void iV_DrawImageText(gfx_api::texture& TextureID, Vector2i Position, Vector2f offset, Vector2f size, float angle, REND_MODE mode, PIELIGHT colour);
void iV_DrawImage(IMAGEFILE *ImageFile, UWORD ID, int x, int y, const glm::mat4 &modelViewProjection = defaultProjectionMatrix());
void iV_DrawImage2(const QString &filename, float x, float y, float width = -0.0f, float height = -0.0f);
void iV_DrawImageTc(Image image, Image imageTc, int x, int y, PIELIGHT colour, const glm::mat4 &modelViewProjection = defaultProjectionMatrix());
void iV_DrawImageRepeatX(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Width, const glm::mat4 &modelViewProjection = defaultProjectionMatrix(), bool enableHorizontalTilingSeamWorkaround = false);
void iV_DrawImageRepeatY(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Height, const glm::mat4 &modelViewProjection = defaultProjectionMatrix());

static inline void iV_DrawImage(Image image, int x, int y)
{
	iV_DrawImage(image.images, image.id, x, y);
}
static inline void iV_DrawImageTc(IMAGEFILE *imageFile, unsigned id, unsigned idTc, int x, int y, PIELIGHT colour)
{
	iV_DrawImageTc(Image(imageFile, id), Image(imageFile, idTc), x, y, colour);
}

void iV_TransBoxFill(float x0, float y0, float x1, float y1);
void pie_UniTransBoxFill(float x0, float y0, float x1, float y1, PIELIGHT colour);

bool pie_InitRadar();
bool pie_ShutdownRadar();
void pie_DownLoadRadar(UDWORD *buffer);
void pie_RenderRadar(const glm::mat4 &modelViewProjectionMatrix);
void pie_SetRadar(GLfloat x, GLfloat y, GLfloat width, GLfloat height, int twidth, int theight);

enum SCREENTYPE
{
	SCREEN_RANDOMBDROP,
	SCREEN_CREDITS,
	SCREEN_MISSIONEND,
};

void pie_LoadBackDrop(SCREENTYPE screenType);

#endif //
