/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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
 * ivisdef.h
 *
 * type defines for all ivis library functions.
 *
 */
/***************************************************************************/

#ifndef _ivisdef_h
#define _ivisdef_h

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "pietypes.h"
#include "tex.h"

#include <vector>
#include <string>


//*************************************************************************
//
// screen surface structure
//
//*************************************************************************
struct iClip
{
	int32_t left, top, right, bottom;
};

struct iSurface
{
	int xcentre;
	int ycentre;
	iClip clip;

	int width;
	int height;
};

//*************************************************************************
//
// imd structures
//
//*************************************************************************

/// Stores the from and to verticles from an edge
struct EDGE
{
	int from, to;
};

struct ANIMFRAME
{
	Vector3f scale = Vector3f(0.f, 0.f, 0.f);
	Position pos = Position(0, 0, 0);
	Rotation rot;
};

struct iIMDPoly
{
	std::vector<Vector2f> texCoord;
	Vector2f texAnim = Vector2f(0.f, 0.f);
	uint32_t flags = 0;
	int32_t zcentre = 0;
	Vector3f normal = Vector3f(0.f, 0.f, 0.f);
	int pindex[3] = { 0 };
};

enum VBO_TYPE
{
	VBO_VERTEX,
	VBO_TEXCOORD,
	VBO_MINIMAL,
	VBO_NORMAL = VBO_MINIMAL,
	VBO_INDEX,
	VBO_COUNT
};

enum ANIMATION_EVENTS
{
	ANIM_EVENT_NONE,
	ANIM_EVENT_ACTIVE,
	ANIM_EVENT_FIRING, // should not be combined with fire-on-move, as this will look weird
	ANIM_EVENT_DYING,
	ANIM_EVENT_COUNT
};

struct iIMDShape
{
	~iIMDShape();

	Vector3i min = Vector3i(0, 0, 0);
	Vector3i max = Vector3i(0, 0, 0);
	unsigned int flags = 0;
	int texpage = iV_TEX_INVALID;
	int tcmaskpage = iV_TEX_INVALID;
	int normalpage = iV_TEX_INVALID;
	int specularpage = iV_TEX_INVALID;
	int sradius = 0;
	int radius = 0;

	Vector3f ocen = Vector3f(0.f, 0.f, 0.f);
	unsigned short numFrames = 0;
	unsigned short animInterval = 0;

	unsigned int nconnectors = 0;
	Vector3i *connectors = 0;

	EDGE *shadowEdgeList = nullptr;
	unsigned int nShadowEdges = 0;

	// The old rendering data
	std::vector<Vector3f> points;
	std::vector<iIMDPoly> polys;

	// The new rendering data
	gfx_api::buffer* buffers[VBO_COUNT] = { nullptr };
	SHADER_MODE shaderProgram = SHADER_NONE; // if using specialized shader for this model

	// object animation (animating a level, rather than its texture)
	std::vector<ANIMFRAME> objanimdata;
	int objanimframes = 0;

	// more object animation, but these are only set for the first level
	int objanimtime = 0; ///< total time to render all animation frames
	int objanimcycles = 0; ///< Number of cycles to render, zero means infinitely many
	iIMDShape *objanimpie[ANIM_EVENT_COUNT] = { nullptr };

	iIMDShape *next = nullptr;  // next pie in multilevel pies (NULL for non multilevel !)
};


//*************************************************************************
//
// immitmap image structures
//
//*************************************************************************

struct ImageDef
{
	unsigned int TPageID;   /**< Which associated file to read our info from */
	unsigned int Tu;        /**< First vertex coordinate */
	unsigned int Tv;        /**< Second vertex coordinate */
	unsigned int Width;     /**< Width of image */
	unsigned int Height;    /**< Height of image */
	int XOffset;            /**< X offset into source position */
	int YOffset;            /**< Y offset into source position */

	int textureId;		///< duplicate of below, fix later
	gfx_api::gfxFloat invTextureSize;
};

struct Image;

struct IMAGEFILE
{
	struct Page
	{
		int id;    /// OpenGL texture ID.
		int size;  /// Size of texture in pixels. (Should be square.)
	};

	Image find(std::string const &name);  // Defined in bitimage.cpp.

	std::vector<Page> pages;          /// Texture pages.
	std::vector<ImageDef> imageDefs;  /// Stored images.
	std::vector<std::pair<std::string, int> > imageNames;  ///< Names of images, sorted by name. Can lookup indices from name.
};

struct Image
{
	Image(IMAGEFILE const *images = nullptr, unsigned id = 0) : images(const_cast<IMAGEFILE *>(images)), id(id) {}

	bool isNull() const
	{
		return images == nullptr;
	}
	int width() const
	{
		return images->imageDefs[id].Width;
	}
	int height() const
	{
		return images->imageDefs[id].Height;
	}
	int xOffset() const
	{
		return images->imageDefs[id].XOffset;
	}
	int yOffset() const
	{
		return images->imageDefs[id].YOffset;
	}

	IMAGEFILE *images;
	unsigned id;
};

#endif // _ivisdef_h
