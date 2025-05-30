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
#include <unordered_map>
#include <memory>
#include <array>


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
	uint32_t from, to;
	uint64_t sort_key;
};

struct ANIMFRAME
{
	Vector3f scale = Vector3f(0.f, 0.f, 0.f);
	Vector3f pos = Vector3f(0.f, 0.f, 0.f);
	Rotation rot;
};

struct iIMDPoly
{
	std::array<Vector2f, 3> texCoord;
	Vector2f texAnim = Vector2f(0.f, 0.f);
	uint32_t flags = 0;
	int32_t zcentre = 0;
	Vector3f normal = Vector3f(0.f, 0.f, 0.f);
	uint32_t pindex[3] = { 0 };
};

enum VBO_TYPE
{
	VBO_VERTEX,
	VBO_TEXCOORD,
	VBO_MINIMAL,
	VBO_NORMAL = VBO_MINIMAL,
	VBO_INDEX,
	VBO_TANGENT,
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

#define WZ_CURRENT_GRAPHICS_OVERRIDES_PREFIX "graphics_overrides/curr"

// A game model will have (potentially) two sets of information:
// 1. Information used for game state calculations (this is always from the "base" / core model file)
//		- This is accessible in the `iIMDBaseShape`
// 2. Information used for display (this may be from the "base" / core model, or a graphics mod overlay display-only model replacement)
//		- This is accessible in the `iIMDShape` that can be obtained via `iIMDBaseShape::displayModel()`

struct iIMDShape;

struct iIMDBaseShape
{
	iIMDBaseShape(std::unique_ptr<iIMDShape> baseModel, const WzString &path, const WzString &filename);

	~iIMDBaseShape();

	// SAFE FOR USE IN GAME STATE CALCULATIONS

	Vector3i min = Vector3i(0, 0, 0); // used by: establishTargetHeight (game state calculation), alignStructure (game state calculation), etc
	Vector3i max = Vector3i(0, 0, 0);
	int sradius = 0;
	int radius = 0;

	Vector3f ocen = Vector3f(0.f, 0.f, 0.f);

	std::vector<Vector3i> connectors; // used by: muzzle base location, fire line, actionVisibleTarget, etc)

	const WzString path;
	const WzString filename;

	// the display shape used for rendering (*NOT* for any game state calculations!)
	inline const iIMDShape* displayModel() const { return m_displayModel.get(); }
protected:
	friend bool tryLoad(const WzString &path, const WzString &filename);
	friend void modelUpdateTilesetIdx(size_t tilesetIdx);
	friend void modelReloadAllModelTextures();
	friend bool debugReloadDisplayModelsForBaseModel(iIMDBaseShape& baseModel);
	friend void debugReloadAllDisplayModels();

	void replaceDisplayModel(std::unique_ptr<iIMDShape> newDisplayModel);
	bool debugReloadDisplayModel(); // not to be called normally at runtime - intended for the script / graphics debugger panel
	inline iIMDShape* mutableDisplayModel() { return m_displayModel.get(); }
private:
	bool debugReloadDisplayModelInternal(bool recurseAnimPie);
private:
	std::unique_ptr<iIMDShape> m_displayModel = nullptr;  // the display shape used for rendering (*NOT* for any game state calculations!)
};

struct iIMDShapeTextures
{
	bool initialized = false;
	size_t texpage = iV_TEX_INVALID;
	size_t tcmaskpage = iV_TEX_INVALID;
	size_t normalpage = iV_TEX_INVALID;
	size_t specularpage = iV_TEX_INVALID;
};

struct TilesetTextureFiles
{
	std::string texfile;
	std::string tcmaskfile;
	std::string normalfile;
	std::string specfile;
};

// DISPLAY-ONLY
// NOTE: Do *NOT* use any data from iIMDShape in game state calculations - instead, use the data in an iIMDBaseShape
struct iIMDShape
{
	iIMDShape& operator=(iIMDShape&& other) noexcept;
	~iIMDShape();

	Vector3i min = Vector3i(0, 0, 0);
	Vector3i max = Vector3i(0, 0, 0);
	int sradius = 0;
	int radius = 0;

	Vector3f ocen = Vector3f(0.f, 0.f, 0.f);

	std::vector<Vector3i> connectors;

	unsigned int flags = 0;

	const iIMDShapeTextures& getTextures() const;

	unsigned short numFrames = 0;
	unsigned short animInterval = 0;

	EDGE *shadowEdgeList = nullptr;
	size_t nShadowEdges = 0;

	// The old rendering data
	std::vector<Vector3f> points; // NOTE: This is used to calculate some of the values above on imd load (in _imd_calc_bounds)
	std::vector<iIMDPoly> polys;

	// Data used for stencil shadows
	std::vector<Vector3f> altShadowPoints;
	std::vector<iIMDPoly> altShadowPolys;
	std::vector<Vector3f> *pShadowPoints = nullptr;
	std::vector<iIMDPoly> *pShadowPolys = nullptr;

	// The new rendering data
	std::array<gfx_api::buffer*, VBO_COUNT> buffers = { };
	uint16_t vertexCount = 0;

	// object animation (animating a level, rather than its texture)
	std::vector<ANIMFRAME> objanimdata;
	int objanimframes = 0;

	// more object animation, but these are only set for the first level
	int objanimtime = 0; ///< total time to render all animation frames
	int objanimcycles = 0; ///< Number of cycles to render, zero means infinitely many
	std::array<iIMDBaseShape*, ANIM_EVENT_COUNT> objanimpie = { }; // non-owned pointer to loaded base shape

	int interpolate = 1; // if the model wants to be interpolated

	WzString modelName;
	uint32_t modelLevel = 0;

	std::array<TilesetTextureFiles, 3> tilesetTextureFiles;

	std::unique_ptr<iIMDShape> next = nullptr;  // next pie in multilevel pies (NULL for non multilevel !)

public:
	PIELIGHT getTeamColourForModel(int team) const;

protected:
	friend void modelUpdateTilesetIdx(size_t tilesetIdx);
	void reloadTexturesIfLoaded();

private:
	void freeInternalResources();

private:
	std::unique_ptr<iIMDShapeTextures> m_textures = std::make_unique<iIMDShapeTextures>();
};

inline const iIMDShape *safeGetDisplayModelFromBase(iIMDBaseShape* pBaseIMD)
{
	return ((pBaseIMD) ? pBaseIMD->displayModel() : nullptr);
}


//*************************************************************************
//
// immitmap image structures
//
//*************************************************************************

struct AtlasImageDef
{
	size_t TPageID;   /**< Which associated file to read our info from */
	unsigned int Tu;        /**< First vertex coordinate */
	unsigned int Tv;        /**< Second vertex coordinate */
	unsigned int Width;     /**< Width of image */
	unsigned int Height;    /**< Height of image */
	int XOffset;            /**< X offset into source position */
	int YOffset;            /**< Y offset into source position */

	size_t textureId;		///< duplicate of below, fix later
	gfx_api::gfxFloat invTextureSize;
};

struct AtlasImage;

struct IMAGEFILE
{
	struct Page
	{
		size_t id;    /// OpenGL texture ID.
		int size;  /// Size of texture in pixels. (Should be square.)
	};

	~IMAGEFILE(); // Defined in bitimage.cpp.
	AtlasImageDef* find(WzString const &name);  // Defined in bitimage.cpp.

	inline size_t numImages() const { return imageDefs.size(); }

	std::vector<Page> pages;          /// Texture pages.
	std::vector<AtlasImageDef> imageDefs;  /// Stored images.
	std::unordered_map<WzString, AtlasImageDef *> imageNamesMap; // Names of images -> AtlasImageDef
};

struct AtlasImage
{
	AtlasImage(IMAGEFILE const *images = nullptr, unsigned id = 0) : images(const_cast<IMAGEFILE *>(images)), id(id) {}

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
