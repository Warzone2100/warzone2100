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

/**
 * @file terrain.c
 * Draws the terrain.
 * It uses the ground property of every MAPTILE to divide the terrain into different layers.
 * For every layer the GROUND_TYPE (from psGroundTypes) determines the texture and size.
 * The technique used it called "texture splatting".
 * Every layer only draws the spots where that terrain is, and additive blending is used to make transitions smooth.
 * Decals are a kind of hack now, as for some tiles (where decal == true) the old tile is just drawn.
 * The water is drawn using the hardcoded page-80 and page-81 textures.
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/opengl.h"
#include "lib/framework/physfs_ext.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/imd.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/pietypes.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/piematrix.h"
#include <glm/mat4x4.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

#include "terrain.h"
#include "map.h"
#include "texture.h"
#include "display3d.h"
#include "hci.h"
#include "loop.h"

// TODO: Fix and remove after merging terrain rendering changes
#if defined(__clang__)
	#pragma clang diagnostic ignored "-Wfloat-conversion"
#elif defined(__GNUC__)
	#pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

/**
 * A sector contains all information to draw a square piece of the map.
 * The actual geometry and texture data is not stored in here but in large VBO's.
 * The sector only stores the index and length of the pieces it's going to use.
 */
struct Sector
{
	int geometryOffset;      ///< The point in the geometry VBO where our geometry starts
	int geometrySize;        ///< The size of our geometry
	int geometryIndexOffset; ///< The point in the index VBO where our triangles start
	int geometryIndexSize;   ///< The size of our indices
	int waterOffset;         ///< The point in the water VBO where the water geometry starts
	int waterSize;           ///< The size of the water geometry
	int waterIndexOffset;    ///< The point in the water index VBO where the water triangles start
	int waterIndexSize;      ///< The size of our water triangles
	std::unique_ptr<int[]> textureOffset;      ///< An array containing the offsets into the texture VBO for each terrain layer
	std::unique_ptr<int[]> textureSize;        ///< The size of the geometry for this layer for each layer
	std::unique_ptr<int[]> textureIndexOffset; ///< The offset into the index VBO for the texture for each layer
	std::unique_ptr<int[]> textureIndexSize;   ///< The size of the indices for each layer
	int decalOffset = 0;         ///< Index into the decal VBO
	int decalSize = 0;           ///< Size of the part of the decal VBO we are going to use
	int terrainAndDecalOffset = 0;
	int terrainAndDecalSize = 0;
	bool draw;               ///< Do we draw this sector this frame?
	bool dirty;              ///< Do we need to update the geometry for this sector?
};

// VBO for gfx_api::TerrainLayer and TerrainDepth
struct TerrainVertex
{
	Vector3f pos = Vector3f(0.f, 0.f, 0.f);
};

/// A vertex with a position and texture coordinates
struct DecalVertex
{
	Vector3f pos = Vector3f(0.f, 0.f, 0.f);
	Vector2f uv = Vector2f(0.f, 0.f);
};

using WaterVertex = glm::vec4; // w is depth

/// The lightmap texture
static gfx_api::texture* lightmap_texture = nullptr;
/// When are we going to update the lightmap next?
static unsigned int lightmapLastUpdate;
/// How big is the lightmap?
static size_t lightmapWidth;
static size_t lightmapHeight;
/// Lightmap image
static std::unique_ptr<iV_Image> lightmapPixmap;
/// Ticks per lightmap refresh
static const unsigned int LIGHTMAP_REFRESH = 80;

// water optional texture names. empty if none
static std::string waterTexture1_nm;
static std::string waterTexture2_nm;
static std::string waterTexture1_sm;
static std::string waterTexture2_sm;
static std::string waterTexture1_hm;
static std::string waterTexture2_hm;

/// VBOs
static gfx_api::buffer *geometryVBO = nullptr, *geometryIndexVBO = nullptr, *textureVBO = nullptr, *textureIndexVBO = nullptr, *decalVBO = nullptr;
/// VBOs
static gfx_api::buffer *waterVBO = nullptr, *waterIndexVBO = nullptr;
static gfx_api::buffer *terrainDecalVBO = nullptr;
/// The amount we shift the water textures so the waves appear to be moving
static float waterOffset;

/// These are properties of your videocard and hardware
static int32_t GLmaxElementsVertices, GLmaxElementsIndices;

/// The sectors are stored here
static std::unique_ptr<Sector[]> sectors;
/// The default sector size (a sector is sectorSize x sectorSize)
static int sectorSize = 15;
/// What is the distance we can see
static int terrainDistance;
/// How many sectors have we actually got?
static int xSectors, ySectors;

/// Did we initialise the terrain renderer yet?
static bool terrainInitialised = false;

/// Helper to specify the offset in a VBO
#define BUFFER_OFFSET(i) (reinterpret_cast<char *>(i))

/// Helper variables for the DrawRangeElements functions
GLuint dreStart, dreEnd, dreOffset;
GLsizei dreCount;
/// Are we actually drawing something using the DrawRangeElements functions?
bool drawRangeElementsStarted = false;

TerrainShaderQuality terrainShaderQuality = TerrainShaderQuality::NORMAL_MAPPING;
TerrainShaderType terrainShaderType = TerrainShaderType::SINGLE_PASS;
bool initializedTerrainShaderType = false;

#define MIN_TERRAIN_TEXTURE_SIZE 512

/// Pass all remaining triangles to OpenGL
template<typename PSO>
static void finishDrawRangeElements()
{
	if (drawRangeElementsStarted && dreCount > 0)
	{
		ASSERT(dreEnd - dreStart + 1 <= GLmaxElementsVertices, "too many vertices (%i)", (int)(dreEnd - dreStart + 1));
		ASSERT(dreCount <= GLmaxElementsIndices, "too many indices (%i)", (int)dreCount);
		PSO::get().draw_elements(dreCount, sizeof(GLuint)*dreOffset);
	}
	drawRangeElementsStarted = false;
}

/**
 * Either draw the elements or batch them to be sent to OpenGL later
 * This improves performance by reducing the amount of OpenGL calls.
 */
template<typename PSO>
static void addDrawRangeElements(GLuint start,
                                 GLuint end,
                                 GLsizei count,
                                 GLuint offset)
{

	if (end - start + 1 > GLmaxElementsVertices)
	{
		debug(LOG_WARNING, "A single call provided too much vertices, will operate at reduced performance or crash. Decrease the sector size to fix this.");
	}
	if (count > GLmaxElementsIndices)
	{
		debug(LOG_WARNING, "A single call provided too much indices, will operate at reduced performance or crash. Decrease the sector size to fix this.");
	}

	if (!drawRangeElementsStarted)
	{
		dreStart  = start;
		dreEnd    = end;
		dreCount  = count;
		dreOffset = offset;
		drawRangeElementsStarted = true;
		return;
	}

	// check if we can append theoretically and
	// check if this will not go over the bounds advised by the opengl implementation
	if (dreOffset + dreCount != offset ||
	    dreCount + count > GLmaxElementsIndices ||
	    end - dreStart + 1 > GLmaxElementsVertices)
	{
		finishDrawRangeElements<PSO>();
		// start anew
		addDrawRangeElements<PSO>(start, end, count, offset);
	}
	else
	{
		// OK to append
		dreCount += count;
		dreEnd = end;
	}
	// make sure we did everything right
	ASSERT(dreEnd - dreStart + 1 <= GLmaxElementsVertices, "too many vertices (%i)", (int)(dreEnd - dreStart + 1));
	ASSERT(dreCount <= GLmaxElementsIndices, "too many indices (%i)", (int)(dreCount));
}

/// Get the colour of the terrain tile at the specified position
PIELIGHT getTileColour(int x, int y)
{
	return mapTile(x, y)->colour;
}

/// Set the colour of the tile at the specified position
void setTileColour(int x, int y, PIELIGHT colour)
{
	MAPTILE *psTile = mapTile(x, y);

	psTile->colour = colour;
}

static void flipRotateTexCoords(unsigned short texture, Vector2f &sP1, Vector2f &sP2, Vector2f &sP3, Vector2f &sP4);

// NOTE:  The current (max) texture size of a tile is 128x128.  We allow up to a user defined texture size
// of 2048.  This will cause ugly seams for the decals, if user picks a texture size bigger than the tile!

/// Set up the texture coordinates for a tile
static Vector2f getTileTexCoords(Vector2f *uv, unsigned int tileNumber)
{
	/* unmask proper values from compressed data */
	const unsigned short texture = TileNumber_texture(tileNumber);
	const unsigned short tile = TileNumber_tile(tileNumber);

	/* Used to calculate texture coordinates */
	const float xMult = 1.0f / TILES_IN_PAGE_COLUMN;
	const float yMult = 1.0f / TILES_IN_PAGE_ROW;
	float texsize = (float)getTextureSize();

	// the decals are 128x128 (at this time), we should not go above this value.  See note above
	if (texsize > getCurrentTileTextureSize())
	{
		texsize = getCurrentTileTextureSize();
	}
	const float centertile = 0.5f / texsize;	// compute center of tile
	const float shiftamount = (texsize - 1.0f) / texsize;	// 1 pixel border
	// bump the texture coords, for 1 pixel border, so our range is [.5,(texsize - .5)]
	const float one = 1.0f / (TILES_IN_PAGE_COLUMN * texsize) + centertile * shiftamount;

	/*
	 * Points for flipping the texture around if the tile is flipped or rotated
	 * Store the source rect as four points
	 */
	Vector2f sP1 { one, one };
	Vector2f sP2 { xMult - one, one };
	Vector2f sP3 { xMult - one, yMult - one };
	Vector2f sP4 { one, yMult - one };
	flipRotateTexCoords(texture, sP1, sP2, sP3, sP4);

	const Vector2f offset { tileTexInfo[tile].uOffset, tileTexInfo[tile].vOffset };

	uv[0 + 0] = offset + sP1;
	uv[0 + 2] = offset + sP2;
	uv[1 + 2] = offset + sP3;
	uv[1 + 0] = offset + sP4;

	/// Calculate the average texture coordinates of 4 points
	return Vector2f { (uv[0].x + uv[1].x + uv[2].x + uv[3].x) / 4, (uv[0].y + uv[1].y + uv[2].y + uv[3].y) / 4 };
}

static void flipRotateTexCoords(unsigned short texture, Vector2f &sP1, Vector2f &sP2, Vector2f &sP3, Vector2f &sP4)
{
	if (texture & TILE_XFLIP)
	{
		std::swap(sP1, sP2);
		std::swap(sP3, sP4);
	}
	if (texture & TILE_YFLIP)
	{
		std::swap(sP1, sP4);
		std::swap(sP2, sP3);
	}

	Vector2f sPTemp;
	switch ((texture & TILE_ROTMASK) >> TILE_ROTSHIFT)
	{
	case 1:
		sPTemp = sP1;
		sP1 = sP4;
		sP4 = sP3;
		sP3 = sP2;
		sP2 = sPTemp;
		break;
	case 2:
		sPTemp = sP1;
		sP1 = sP3;
		sP3 = sPTemp;
		sPTemp = sP4;
		sP4 = sP2;
		sP2 = sPTemp;
		break;
	case 3:
		sPTemp = sP1;
		sP1 = sP2;
		sP2 = sP3;
		sP3 = sP4;
		sP4 = sPTemp;
		break;
	}
}

/// set up tile/decal coords for texture arrays. just flip/rotate, without borders and tex atlas stuff.
static Vector2f getTileTexArrCoords(Vector2f *uv, unsigned int tileNumber)
{
	/* unmask proper values from compressed data */
	const unsigned short texture = TileNumber_texture(tileNumber);
	Vector2f sP[] = {{ 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }};
	flipRotateTexCoords(texture, sP[0], sP[1], sP[2], sP[3]);
	uv[0 + 0] = sP[0];
	uv[0 + 2] = sP[1];
	uv[1 + 2] = sP[2];
	uv[1 + 0] = sP[3];
	return Vector2f { 0.5, 0.5 };
}

/// Average the four positions to get the center
static void averagePos(Vector3i *center, Vector3i *a, Vector3i *b, Vector3i *c, Vector3i *d)
{
	center->x = (a->x + b->x + c->x + d->x) / 4;
	center->y = (a->y + b->y + c->y + d->y) / 4;
	center->z = (a->z + b->z + c->z + d->z) / 4;
}

/// Is this position next to a water tile?
static bool isWater(int x, int y)
{
	bool result = false;
	result = result || (tileOnMap(x  , y) && terrainType(mapTile(x  , y)) == TER_WATER);
	result = result || (tileOnMap(x - 1, y) && terrainType(mapTile(x - 1, y)) == TER_WATER);
	result = result || (tileOnMap(x  , y - 1) && terrainType(mapTile(x  , y - 1)) == TER_WATER);
	result = result || (tileOnMap(x - 1, y - 1) && terrainType(mapTile(x - 1, y - 1)) == TER_WATER);
	return result;
}

/// Is this position an *actual* water-only tile?
static bool isOnlyWater(int x, int y)
{
	bool result = true;
	result = result && (tileOnMap(x  , y) && terrainType(mapTile(x  , y)) == TER_WATER);
	result = result && (tileOnMap(x - 1, y) && terrainType(mapTile(x - 1, y)) == TER_WATER);
	result = result && (tileOnMap(x  , y - 1) && terrainType(mapTile(x  , y - 1)) == TER_WATER);
	result = result && (tileOnMap(x - 1, y - 1) && terrainType(mapTile(x - 1, y - 1)) == TER_WATER);
	return result;
}

/// Get the position of a grid point
static void getGridPos(Vector3i *result, int x, int y, bool center, bool water)
{
	if (center)
	{
		Vector3i a, b, c, d;
		getGridPos(&a, x  , y  , false, water);
		getGridPos(&b, x + 1, y  , false, water);
		getGridPos(&c, x  , y + 1, false, water);
		getGridPos(&d, x + 1, y + 1, false, water);
		averagePos(result, &a, &b, &c, &d);
		return;
	}
	result->x = world_coord(x);
	result->z = world_coord(-y);

	if (x <= 0 || y <= 0 || x >= mapWidth || y >= mapHeight)
	{
		result->y = 0;
	}
	else
	{
		if (terrainShaderQuality != TerrainShaderQuality::CLASSIC)
		{
			result->y = map_TileHeight(x, y);
			if (water)
			{
				result->y = map_WaterHeight(x, y);
			}
		}
		else
		{
			result->y = map_TileHeightSurface(x, y);
		}
	}
}

static Vector3f getGridPosf(int x, int y, bool center = false, bool water = false)
{
	Vector3i r;
	getGridPos(&r, x, y, center, water);
	return Vector3f(r);
}

/// Get normal vector of grid point
static Vector3f getGridNormal(int x, int y, bool center = false)
{
	auto calcNormal = [](const Vector3f &pc, const std::vector<Vector3f> &p) {
		auto res = Vector3f(0.0);
		for (int i = 0; i < p.size(); i++) {
			auto e1 = p[(i+1)%p.size()] - pc, e2 = p[i] - pc;
			float ang = acos(glm::dot(e1, e2) / glm::length(e1) / glm::length(e2));
			res += glm::cross(e1, e2) * ang; // += normal * (2*area) * angle
		}
		return -glm::normalize(res);
	};
	if (center) {
		return calcNormal(getGridPosf(x, y, true), {
			getGridPosf(x,y), getGridPosf(x+1, y), getGridPosf(x+1, y+1), getGridPosf(x, y+1)
		});
	} else {
		return calcNormal(getGridPosf(x, y), {
			getGridPosf(x+1, y), getGridPosf(x, y, true),     getGridPosf(x, y+1), getGridPosf(x-1, y, true),
			getGridPosf(x-1, y), getGridPosf(x-1, y-1, true), getGridPosf(x, y-1), getGridPosf(x, y-1, true)
		});
	}
}

/// Calculate the average colour of 4 points
static inline void averageColour(PIELIGHT *average, PIELIGHT a, PIELIGHT b,
                                 PIELIGHT c, PIELIGHT d)
{
	average->byte.a = (a.byte.a + b.byte.a + c.byte.a + d.byte.a) / 4;
	average->byte.r = (a.byte.r + b.byte.r + c.byte.r + d.byte.r) / 4;
	average->byte.g = (a.byte.g + b.byte.g + c.byte.g + d.byte.g) / 4;
	average->byte.b = (a.byte.b + b.byte.b + c.byte.b + d.byte.b) / 4;
}

/**
 * Set the terrain and water geometry for the specified sector
 */
static void setSectorGeometry(int sx, int sy,
							  TerrainVertex *geometry, WaterVertex *water,
							  int *geometrySize, int *waterSize)
{
	for (int x = sx*sectorSize; x < (sx+1)*sectorSize + 1; x++)
	{
		for (int y = sy * sectorSize; y < (sy+1) * sectorSize + 1; y++)
		{
			// set up geometry
			auto pos = getGridPosf(x, y, false, false);
			geometry[*geometrySize].pos = pos;
			(*geometrySize)++;

			float waterHeight = map_WaterHeight(x, y);
			water[*waterSize] = glm::vec4(pos.x, (terrainShaderQuality != TerrainShaderQuality::CLASSIC) ? waterHeight : pos.y, pos.z, waterHeight - pos.y);
			(*waterSize)++;

			pos = getGridPosf(x, y, true, false);
			geometry[*geometrySize].pos = pos;
			(*geometrySize)++;

			water[*waterSize] = glm::vec4(pos.x, (terrainShaderQuality != TerrainShaderQuality::CLASSIC) ? waterHeight : pos.y, pos.z, waterHeight - pos.y);
			(*waterSize)++;
		}
	}
}

/**
 * Set the decals for a sector. This takes care of both the geometry and the texture part.
 */
static void setSectorDecals(int x, int y, DecalVertex *decaldata, int *decalSize)
{
	Vector3i pos;
	Vector2f uv[2][2], center;
	int a, b;
	int i, j;

	for (i = x * sectorSize; i < x * sectorSize + sectorSize; i++)
	{
		for (j = y * sectorSize; j < y * sectorSize + sectorSize; j++)
		{
			if (i < 0 || j < 0 || i >= mapWidth || j >= mapHeight)
			{
				continue;
			}
			if (TILE_HAS_DECAL(mapTile(i, j)))
			{
				center = getTileTexCoords(*uv, mapTile(i, j)->texture);

				getGridPos(&pos, i, j, true, false);
				decaldata[*decalSize].pos = pos;
				decaldata[*decalSize].uv = center;
				(*decalSize)++;
				a = 0; b = 1;
				getGridPos(&pos, i + a, j + b, false, false);
				decaldata[*decalSize].pos = pos;
				decaldata[*decalSize].uv = uv[a][b];
				(*decalSize)++;
				a = 0; b = 0;
				getGridPos(&pos, i + a, j + b, false, false);
				decaldata[*decalSize].pos = pos;
				decaldata[*decalSize].uv = uv[a][b];
				(*decalSize)++;

				getGridPos(&pos, i, j, true, false);
				decaldata[*decalSize].pos = pos;
				decaldata[*decalSize].uv = center;
				(*decalSize)++;
				a = 1; b = 1;
				getGridPos(&pos, i + a, j + b, false, false);
				decaldata[*decalSize].pos = pos;
				decaldata[*decalSize].uv = uv[a][b];
				(*decalSize)++;
				a = 0; b = 1;
				getGridPos(&pos, i + a, j + b, false, false);
				decaldata[*decalSize].pos = pos;
				decaldata[*decalSize].uv = uv[a][b];
				(*decalSize)++;

				getGridPos(&pos, i, j, true, false);
				decaldata[*decalSize].pos = pos;
				decaldata[*decalSize].uv = center;
				(*decalSize)++;
				a = 1; b = 0;
				getGridPos(&pos, i + a, j + b, false, false);
				decaldata[*decalSize].pos = pos;
				decaldata[*decalSize].uv = uv[a][b];
				(*decalSize)++;
				a = 1; b = 1;
				getGridPos(&pos, i + a, j + b, false, false);
				decaldata[*decalSize].pos = pos;
				decaldata[*decalSize].uv = uv[a][b];
				(*decalSize)++;

				getGridPos(&pos, i, j, true, false);
				decaldata[*decalSize].pos = pos;
				decaldata[*decalSize].uv = center;
				(*decalSize)++;
				a = 0; b = 0;
				getGridPos(&pos, i + a, j + b, false, false);
				decaldata[*decalSize].pos = pos;
				decaldata[*decalSize].uv = uv[a][b];
				(*decalSize)++;
				a = 1; b = 0;
				getGridPos(&pos, i + a, j + b, false, false);
				decaldata[*decalSize].pos = pos;
				decaldata[*decalSize].uv = uv[a][b];
				(*decalSize)++;
			}
		}
	}
}

static void setSectorDecalVertex_SinglePass(int x, int y, gfx_api::TerrainDecalVertex *terrainDecalData, int *terrainDecalSize)
{
	Vector3i pos;
	Vector2f uv[2][2], center;
	int i, j;

	for (i = x * sectorSize; i < x * sectorSize + sectorSize; i++)
	{
		for (j = y * sectorSize; j < y * sectorSize + sectorSize; j++)
		{
			if (i < 0 || j < 0 || i >= mapWidth || j >= mapHeight)
			{
				continue;
			}

			MAPTILE *tile = mapTile(i, j);
			center = getTileTexArrCoords(*uv, tile->texture);
			int decalNo = static_cast<int>(TileNumber_tile(tile->texture));
			bool skipDecalDraw = !TILE_HAS_DECAL(tile);
			if (terrainShaderQuality == TerrainShaderQuality::CLASSIC)
			{
				// in Classic mode, all tiles are decals, but skip drawing any that are the "water only" decal (water is handled separately as a prior pass)
				skipDecalDraw = (isOnlyWater(i, j) && decalNo == 17); // Magic number hack, but decal # 17 is always the *water only* tile in the legacy terrain tilesets we ship // TODO: Figure out a better way of determining this from the tileset data?
			}
			if (skipDecalDraw)
			{
				decalNo = -1;
			}

			int dxdy[4][2] = {{0,0}, {0,1}, {1,1}, {1,0}};
			PIELIGHT grounds;
			gfx_api::TerrainDecalVertex vs[5];
			for (int k = 0; k<4; k++) {
				int dx = dxdy[k][0], dy = dxdy[k][1];
				getGridPos(&pos, i + dx, j + dy, false, false);
				vs[k].pos = pos;
				vs[k].decalUv = uv[dx][dy];
				vs[k].normal = getGridNormal(i + dx, j + dy);
				vs[k].decalNo = decalNo;
				grounds.vector[k] = mapTile(i + dx, j + dy)->ground;
				vs[k].groundWeights.rgba = 0;
				vs[k].groundWeights.vector[k] = 255;
			}
			// 4 = center;
			getGridPos(&pos, i, j, true, false);
			vs[4].pos = pos;
			vs[4].decalUv = center;
			vs[4].normal = getGridNormal(i, j, true);
			vs[4].decalNo = decalNo;
			vs[4].groundWeights = {0, 0, 0, 0}; // special value for shader.
			for (int k = 0; k < 5; k++) vs[k].grounds = grounds.rgba;

			terrainDecalData[(*terrainDecalSize)++] = vs[4];
			terrainDecalData[(*terrainDecalSize)++] = vs[1];
			terrainDecalData[(*terrainDecalSize)++] = vs[0];

			terrainDecalData[(*terrainDecalSize)++] = vs[4];
			terrainDecalData[(*terrainDecalSize)++] = vs[2];
			terrainDecalData[(*terrainDecalSize)++] = vs[1];

			terrainDecalData[(*terrainDecalSize)++] = vs[4];
			terrainDecalData[(*terrainDecalSize)++] = vs[3];
			terrainDecalData[(*terrainDecalSize)++] = vs[2];

			terrainDecalData[(*terrainDecalSize)++] = vs[4];
			terrainDecalData[(*terrainDecalSize)++] = vs[0];
			terrainDecalData[(*terrainDecalSize)++] = vs[3];

			// calc tangents
			for (int idx = *terrainDecalSize - 3*4; idx < *terrainDecalSize; idx+=3) {
				auto p = terrainDecalData + idx;
				auto e1 = p[1].pos - p[0].pos;
				auto e2 = p[2].pos - p[0].pos;
				auto uv1 = p[1].decalUv - p[0].decalUv;
				auto uv2 = p[2].decalUv - p[0].decalUv;
				float r = 1.0f / (uv1.x * uv2.y - uv2.x * uv1.y);
				Vector3f tangent = glm::normalize(r * (uv2.y * e1 - uv1.y * e2));
				Vector3f bitangent = glm::normalize(r * (-uv2.x * e1 + uv1.x * e2));
				for (int k=0; k<3; k++) {
					auto &n = p[k].normal;
					const auto t = glm::normalize(tangent - (n * glm::dot(tangent, n)));
					float w = 1.0f; // not mirrored
					if (glm::dot(glm::cross(n, t), bitangent) < 0.0f) {
						w = -1.0f; // we're mirrored
					}
					p[k].decalTangent = glm::vec4(t, w);
				}
			}
		}
	}
}

/**
 * Update the sector for when the terrain is changed.
 */
static void updateSectorGeometry(int x, int y)
{
	TerrainVertex *geometry;
	WaterVertex *water;
	DecalVertex *decaldata;
	int geometrySize = 0;
	int waterSize = 0;
	int decalSize = 0;

	geometry = new TerrainVertex[sectors[x * ySectors + y].geometrySize];
	water = (WaterVertex *)malloc(sizeof(WaterVertex) * sectors[x * ySectors + y].waterSize);

	setSectorGeometry(x, y, geometry, water, &geometrySize, &waterSize);
	ASSERT(geometrySize == sectors[x * ySectors + y].geometrySize, "something went seriously wrong updating the terrain");
	ASSERT(waterSize    == sectors[x * ySectors + y].waterSize   , "something went seriously wrong updating the terrain");

	geometryVBO->update(sizeof(TerrainVertex)*sectors[x * ySectors + y].geometryOffset,
							sizeof(TerrainVertex)*sectors[x * ySectors + y].geometrySize, geometry,
							gfx_api::buffer::update_flag::non_overlapping_updates_promise);
	waterVBO->update(sizeof(WaterVertex)*sectors[x * ySectors + y].waterOffset,
					 sizeof(WaterVertex)*sectors[x * ySectors + y].waterSize, water,
					 gfx_api::buffer::update_flag::non_overlapping_updates_promise);

	delete[] geometry;
	free(water);

	if (terrainShaderType == TerrainShaderType::FALLBACK)
	{
		if (sectors[x * ySectors + y].decalSize <= 0)
		{
			// Nothing to do here, and glBufferSubData(GL_ARRAY_BUFFER, 0, 0, *) crashes in my graphics driver. Probably shouldn't crash...
			return;
		}

		decaldata = (DecalVertex *)malloc(sizeof(DecalVertex) * sectors[x * ySectors + y].decalSize);
		setSectorDecals(x, y, decaldata, &decalSize);
		ASSERT(decalSize == sectors[x * ySectors + y].decalSize   , "the amount of decals has changed");

		if (decalSize > 0)
		{
			if (decalVBO)
			{
				decalVBO->update(sizeof(DecalVertex)*sectors[x * ySectors + y].decalOffset,
								 sizeof(DecalVertex)*sectors[x * ySectors + y].decalSize, decaldata,
								 gfx_api::buffer::update_flag::non_overlapping_updates_promise);
			}
			else
			{
				// didn't have decals, but now we do??
				// code needs a refactoring if this is the case
				ASSERT(false, "Didn't have decals, but now we do. Unsupported.");
			}
		}

		free(decaldata);
	}
	else
	{
		gfx_api::TerrainDecalVertex *terrainDecalData = (gfx_api::TerrainDecalVertex *)malloc(sizeof(gfx_api::TerrainDecalVertex) * mapWidth * mapHeight * 12);
		int terrainDecalSize = 0;
		setSectorDecalVertex_SinglePass(x, y, terrainDecalData, &terrainDecalSize);
		terrainDecalVBO->update(sizeof(gfx_api::TerrainDecalVertex)*sectors[x * ySectors + y].terrainAndDecalOffset,
							 sizeof(gfx_api::TerrainDecalVertex)*sectors[x * ySectors + y].terrainAndDecalSize, terrainDecalData,
							 gfx_api::buffer::update_flag::non_overlapping_updates_promise);
		free(terrainDecalData);
	}
}

/**
 * Mark all tiles that are influenced by this grid point as dirty.
 * Dirty sectors will later get updated by updateSectorGeometry.
 */
void markTileDirty(int i, int j)
{
	int x, y;

	if (!terrainInitialised)
	{
		return; // will be updated anyway
	}

	x = i / sectorSize;
	y = j / sectorSize;
	if (x < xSectors && y < ySectors) // could be on the lower or left edge of the map
	{
		sectors[x * ySectors + y].dirty = true;
	}

	// it could be on an edge, so update for all sectors it is in
	if (x * sectorSize == i && x > 0)
	{
		if (x - 1 < xSectors && y < ySectors)
		{
			sectors[(x - 1)*ySectors + y].dirty = true;
		}
	}
	if (y * sectorSize == j && y > 0)
	{
		if (x < xSectors && y - 1 < ySectors)
		{
			sectors[x * ySectors + (y - 1)].dirty = true;
		}
	}
	if (x * sectorSize == i && x > 0 && y * sectorSize == j && y > 0)
	{
		if (x - 1 < xSectors && y - 1 < ySectors)
		{
			sectors[(x - 1)*ySectors + (y - 1)].dirty = true;
		}
	}
}

void loadTerrainTextures_Fallback(MAP_TILESET mapTileset)
{
	ASSERT_OR_RETURN(, getNumGroundTypes(), "Ground type was not set, no textures will be seen.");

	int32_t maxGfxTextureSize = gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_TEXTURE_SIZE);
	int maxTerrainTextureSize = std::max(std::min({getTextureSize(), maxGfxTextureSize}), MIN_TERRAIN_TEXTURE_SIZE);

	// for each terrain layer
	for (int layer = 0; layer < getNumGroundTypes(); layer++)
	{
		// pre-load the texture
		const auto groundType = getGroundType(layer);
		optional<size_t> texPage = iV_GetTexture(groundType.textureName.c_str(), gfx_api::texture_type::game_texture, maxTerrainTextureSize, maxTerrainTextureSize);
		ASSERT(texPage.has_value(), "Failed to pre-load terrain texture: %s", groundType.textureName.c_str());
	}
}

static gfx_api::texture_array* groundTexArr = nullptr;
static gfx_api::texture_array* groundNormalArr = nullptr;
static gfx_api::texture_array* groundSpecularArr = nullptr;
static gfx_api::texture_array* groundHeightArr = nullptr;

void loadTerrainTextures_SinglePass(MAP_TILESET mapTileset)
{
	ASSERT_OR_RETURN(, getNumGroundTypes(), "Ground type was not set, no textures will be seen.");

	int32_t maxGfxTextureSize = gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_TEXTURE_SIZE);
	int maxTerrainTextureSize = std::max(std::min({getTextureSize(), maxGfxTextureSize}), MIN_TERRAIN_TEXTURE_SIZE);

	delete groundTexArr; groundTexArr = nullptr;
	delete groundNormalArr; groundNormalArr = nullptr;
	delete groundSpecularArr; groundSpecularArr = nullptr;
	delete groundHeightArr; groundHeightArr = nullptr;

	std::vector<WzString> groundTextureFilenames;
	std::vector<WzString> groundTextureFilenames_nm;
	std::vector<WzString> groundTextureFilenames_spec;
	std::vector<WzString> groundTextureFilenames_height;

	auto optTexturenameToPath = [](const std::string& textureFilename) -> WzString {
		if (textureFilename.empty())
		{
			return WzString();
		}
		return "texpages/" + WzString::fromUtf8(textureFilename);
	};

	// for each terrain layer
	for (int layer = 0; layer < getNumGroundTypes(); layer++)
	{
		const auto groundType = getGroundType(layer);
		groundTextureFilenames.push_back(optTexturenameToPath(groundType.textureName));
		groundTextureFilenames_nm.push_back(optTexturenameToPath(groundType.normalMapTextureName));
		groundTextureFilenames_spec.push_back(optTexturenameToPath(groundType.specularMapTextureName));
		groundTextureFilenames_height.push_back(optTexturenameToPath(groundType.heightMapTextureName));
	}

	// load the textures into the texture arrays
	groundTexArr = gfx_api::context::get().loadTextureArrayFromFiles(groundTextureFilenames, gfx_api::texture_type::game_texture, maxTerrainTextureSize, maxTerrainTextureSize, nullptr, []() { resDoResLoadCallback(); });
	ASSERT(groundTexArr != nullptr, "Failed to load terrain textures");
	if (terrainShaderQuality == TerrainShaderQuality::NORMAL_MAPPING)
	{
		if (std::any_of(groundTextureFilenames_nm.begin(), groundTextureFilenames_nm.end(), [](const WzString& filename) {
			return !filename.isEmpty();
		}))
		{
			groundNormalArr = gfx_api::context::get().loadTextureArrayFromFiles(groundTextureFilenames_nm, gfx_api::texture_type::normal_map, maxTerrainTextureSize, maxTerrainTextureSize, [](int width, int height, int channels) -> std::unique_ptr<iV_Image> {
				std::unique_ptr<iV_Image> pDefaultNormalMap = std::unique_ptr<iV_Image>(new iV_Image);
				pDefaultNormalMap->allocate(width, height, channels, true);
				// default normal map: (0,0,1)
				unsigned char* pBmpWrite = pDefaultNormalMap->bmp_w();
				memset(pBmpWrite, 0x7f, pDefaultNormalMap->data_size());
				if (channels >= 3)
				{
					for (size_t b = 0; b < pDefaultNormalMap->data_size(); b += 4)
					{
						pBmpWrite[b+2] = 0xff; // blue=z
					}
				}
				return pDefaultNormalMap;
			}, []() { resDoResLoadCallback(); });
			ASSERT(groundNormalArr != nullptr, "Failed to load terrain normals");
		}
		if (std::any_of(groundTextureFilenames_spec.begin(), groundTextureFilenames_spec.end(), [](const WzString& filename) {
			return !filename.isEmpty();
		}))
		{
			groundSpecularArr = gfx_api::context::get().loadTextureArrayFromFiles(groundTextureFilenames_spec, gfx_api::texture_type::specular_map, maxTerrainTextureSize, maxTerrainTextureSize, [](int width, int height, int channels) -> std::unique_ptr<iV_Image> {
				std::unique_ptr<iV_Image> pDefaultNormalMap = std::unique_ptr<iV_Image>(new iV_Image);
				// default specular map: 0
				pDefaultNormalMap->allocate(width, height, channels, true);
				return pDefaultNormalMap;
			}, []() { resDoResLoadCallback(); });
			ASSERT(groundSpecularArr != nullptr, "Failed to load terrain specular maps");
		}
		if (std::any_of(groundTextureFilenames_height.begin(), groundTextureFilenames_height.end(), [](const WzString& filename) {
			return !filename.isEmpty();
		}))
		{
			groundHeightArr = gfx_api::context::get().loadTextureArrayFromFiles(groundTextureFilenames_height, gfx_api::texture_type::height_map, maxTerrainTextureSize, maxTerrainTextureSize, [](int width, int height, int channels) -> std::unique_ptr<iV_Image> {
				std::unique_ptr<iV_Image> pDefaultNormalMap = std::unique_ptr<iV_Image>(new iV_Image);
				// default height map: 0
				pDefaultNormalMap->allocate(width, height, channels, true);
				return pDefaultNormalMap;
			}, []() { resDoResLoadCallback(); });
			ASSERT(groundHeightArr != nullptr, "Failed to load terrain height maps");
		}
	}

	// check water optional textures
	auto checkTex = [](const std::string &fileName) {
		std::string fullName = "texpages/"+fileName;
		return PHYSFS_exists(fullName.c_str()) ? fileName : "";
	};
	waterTexture1_nm = checkTex("page-80-water-1_nm.png");
	waterTexture2_nm = checkTex("page-81-water-2_nm.png");
	waterTexture1_sm = checkTex("page-80-water-1_sm.png");
	waterTexture2_sm = checkTex("page-81-water-2_sm.png");
	waterTexture1_hm = checkTex("page-80-water-1_hm.png");
	waterTexture2_hm = checkTex("page-81-water-2_hm.png");
}

void loadTerrainTextures(MAP_TILESET mapTileset)
{
	ASSERT_OR_RETURN(, getNumGroundTypes(), "Ground type was not set, no textures will be seen.");

	switch (terrainShaderType)
	{
	case TerrainShaderType::FALLBACK:
		loadTerrainTextures_Fallback(mapTileset);
		break;
	case TerrainShaderType::SINGLE_PASS:
		loadTerrainTextures_SinglePass(mapTileset);
		break;
	}
}

/**
 * Check what the videocard + drivers support and divide the loaded map into sectors that can be drawn.
 * It also determines the lightmap size.
 */
bool initTerrain()
{
	int i, j, x, y, a, b, absX, absY;
	PIELIGHT colour[2][2], centerColour;
	int layer = 0;

	TerrainVertex *geometry = nullptr;
	WaterVertex *water = nullptr;
	DecalVertex *decaldata = nullptr;
	int geometrySize, geometryIndexSize;
	int waterSize, waterIndexSize;
	int textureSize, textureIndexSize;
	GLuint *geometryIndex = nullptr;
	GLuint *waterIndex = nullptr;
	GLuint *textureIndex = nullptr;
	PIELIGHT *texture = nullptr;
	int decalSize = 0;
	int maxSectorSizeIndices, maxSectorSizeVertices;
	bool decreasedSize = false;

	// this information is useful to prevent crashes with buggy opengl implementations
	GLmaxElementsVertices = gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_ELEMENTS_VERTICES);
	GLmaxElementsIndices = gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_ELEMENTS_INDICES);

	// testing for crappy cards
	debug(LOG_TERRAIN, "GL_MAX_ELEMENTS_VERTICES: %i", (int)GLmaxElementsVertices);
	debug(LOG_TERRAIN, "GL_MAX_ELEMENTS_INDICES:  %i", (int)GLmaxElementsIndices);

	// now we know these values, determine the maximum sector size achievable
	maxSectorSizeVertices = iSqrt(GLmaxElementsVertices / 2) - 1;
	maxSectorSizeIndices = iSqrt(GLmaxElementsIndices / 12);

	debug(LOG_TERRAIN, "preferred sector size: %i", sectorSize);
	debug(LOG_TERRAIN, "maximum sector size due to vertices: %i", maxSectorSizeVertices);
	debug(LOG_TERRAIN, "maximum sector size due to indices: %i", maxSectorSizeIndices);

	if (sectorSize > maxSectorSizeVertices)
	{
		sectorSize = maxSectorSizeVertices;
		decreasedSize = true;
	}
	if (sectorSize > maxSectorSizeIndices)
	{
		sectorSize = maxSectorSizeIndices;
		decreasedSize = true;
	}
	if (decreasedSize)
	{
		if (sectorSize < 1)
		{
			debug(LOG_WARNING, "GL_MAX_ELEMENTS_VERTICES: %i", (int)GLmaxElementsVertices);
			debug(LOG_WARNING, "GL_MAX_ELEMENTS_INDICES:  %i", (int)GLmaxElementsIndices);
			debug(LOG_WARNING, "maximum sector size due to vertices: %i", maxSectorSizeVertices);
			debug(LOG_WARNING, "maximum sector size due to indices: %i", maxSectorSizeIndices);
			debug(LOG_ERROR, "Your graphics card and/or drivers do not seem to support glDrawRangeElements, needed for the terrain renderer.");
			debug(LOG_ERROR, "- Do other 3D games work?");
			debug(LOG_ERROR, "- Did you install the latest drivers correctly?");
			debug(LOG_ERROR, "- Do you have a 3D window manager (Aero/Compiz) running?");
			return false;
		}
		debug(LOG_WARNING, "decreasing sector size to %i to fit graphics card constraints", sectorSize);
	}

	// +4 = +1 for iHypot rounding, +1 for sector size rounding, +2 for edge of visibility
	terrainDistance = iHypot(visibleTiles.x / 2, visibleTiles.y / 2) + 4 + sectorSize / 2;
	debug(LOG_TERRAIN, "visible tiles x:%i y: %i", visibleTiles.x, visibleTiles.y);
	debug(LOG_TERRAIN, "terrain view distance: %i", terrainDistance);

	/////////////////////
	// Create the sectors
	xSectors = (mapWidth + sectorSize - 1) / sectorSize;
	ySectors = (mapHeight + sectorSize - 1) / sectorSize;
	sectors = std::unique_ptr<Sector[]> (new Sector[xSectors * ySectors]());

	////////////////////
	// fill the geometry part of the sectors
	const int vertSize = xSectors * ySectors * (sectorSize + 1) * (sectorSize + 1) * 2;
	geometry = new TerrainVertex[vertSize];
	geometryIndex = (GLuint *)malloc(sizeof(GLuint) * xSectors * ySectors * sectorSize * sectorSize * 12);
	geometrySize = 0;
	geometryIndexSize = 0;

	water = (WaterVertex *)malloc(sizeof(WaterVertex) * vertSize);
	waterIndex = (GLuint *)malloc(sizeof(GLuint) * xSectors * ySectors * sectorSize * sectorSize * 12);
	waterSize = 0;
	waterIndexSize = 0;
	for (x = 0; x < xSectors; x++)
	{
		for (y = 0; y < ySectors; y++)
		{
			sectors[x * ySectors + y].dirty = false;
			sectors[x * ySectors + y].geometryOffset = geometrySize;
			sectors[x * ySectors + y].geometrySize = 0;
			sectors[x * ySectors + y].waterOffset = waterSize;
			sectors[x * ySectors + y].waterSize = 0;

			setSectorGeometry(x, y, geometry, water, &geometrySize, &waterSize);

			sectors[x * ySectors + y].geometrySize = geometrySize - sectors[x * ySectors + y].geometryOffset;
			sectors[x * ySectors + y].waterSize = waterSize - sectors[x * ySectors + y].waterOffset;
			// and do the index buffers
			sectors[x * ySectors + y].geometryIndexOffset = geometryIndexSize;
			sectors[x * ySectors + y].geometryIndexSize = 0;
			sectors[x * ySectors + y].waterIndexOffset = waterIndexSize;
			sectors[x * ySectors + y].waterIndexSize = 0;

			for (i = 0; i < sectorSize; i++)
			{
				for (j = 0; j < sectorSize; j++)
				{
					if (x * sectorSize + i >= mapWidth || y * sectorSize + j >= mapHeight)
					{
						continue; // off map, so skip
					}

					/* One tile is composed of 4 triangles,
					 * we need _2_ vertices per tile (1)
					 * 		e.g. center and bottom left
					 * 	the other 3 vertices are from the adjacent tiles
					 * 	on their top and right.
					 * (1) The top row and right column of tiles need 4 vertices per tile
					 * 	because they do not have adjacent tiles on their top and right,
					 * 	that is why we add _1_ row and _1_ column to provide the geometry
					 * 	for these tiles.
					 * This is the source of the '*2' and '+1' in the index math below.
					 */
#define q(i,j,center) ((x*ySectors+y)*(sectorSize+1)*(sectorSize+1)*2 + ((i)*(sectorSize+1)+(j))*2+(center))
					// First triangle
					geometryIndex[geometryIndexSize + 0]  = q(i  , j  , 1);	// Center vertex
					geometryIndex[geometryIndexSize + 1]  = q(i  , j  , 0);	// Bottom left
					geometryIndex[geometryIndexSize + 2]  = q(i + 1, j  , 0);	// Bottom right
					// Second triangle
					geometryIndex[geometryIndexSize + 3]  = q(i  , j  , 1);	// Center vertex
					geometryIndex[geometryIndexSize + 4]  = q(i  , j + 1, 0);	// Top left
					geometryIndex[geometryIndexSize + 5]  = q(i  , j  , 0);	// Bottom left
					// Third triangle
					geometryIndex[geometryIndexSize + 6]  = q(i  , j  , 1);	// Center vertex
					geometryIndex[geometryIndexSize + 7]  = q(i + 1, j + 1, 0);	// Top right
					geometryIndex[geometryIndexSize + 8]  = q(i  , j + 1, 0);	// Top left
					// Fourth triangle
					geometryIndex[geometryIndexSize + 9]  = q(i  , j  , 1);	// Center vertex
					geometryIndex[geometryIndexSize + 10] = q(i + 1, j  , 0);	// Bottom right
					geometryIndex[geometryIndexSize + 11] = q(i + 1, j + 1, 0);	// Top right
					geometryIndexSize += 12;
					if (isWater(i + x * sectorSize, j + y * sectorSize))
					{
						waterIndex[waterIndexSize + 0]  = q(i  , j  , 1);
						waterIndex[waterIndexSize + 1]  = q(i  , j  , 0);
						waterIndex[waterIndexSize + 2]  = q(i + 1, j  , 0);

						waterIndex[waterIndexSize + 3]  = q(i  , j  , 1);
						waterIndex[waterIndexSize + 4]  = q(i  , j + 1, 0);
						waterIndex[waterIndexSize + 5]  = q(i  , j  , 0);

						waterIndex[waterIndexSize + 6]  = q(i  , j  , 1);
						waterIndex[waterIndexSize + 7]  = q(i + 1, j + 1, 0);
						waterIndex[waterIndexSize + 8]  = q(i  , j + 1, 0);

						waterIndex[waterIndexSize + 9]  = q(i  , j  , 1);
						waterIndex[waterIndexSize + 10] = q(i + 1, j  , 0);
						waterIndex[waterIndexSize + 11] = q(i + 1, j + 1, 0);
						waterIndexSize += 12;
					}
				}
			}
			sectors[x * ySectors + y].geometryIndexSize = geometryIndexSize - sectors[x * ySectors + y].geometryIndexOffset;
			sectors[x * ySectors + y].waterIndexSize = waterIndexSize - sectors[x * ySectors + y].waterIndexOffset;
		}
	}
	if (geometryVBO)
		delete geometryVBO;
	geometryVBO = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::dynamic_draw, "terrain::geometryVBO");
	geometryVBO->upload(sizeof(TerrainVertex)*geometrySize, geometry);
	delete[] geometry;

	if (geometryIndexVBO)
		delete geometryIndexVBO;
	geometryIndexVBO = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::index_buffer, gfx_api::context::buffer_storage_hint::static_draw, "terrain::geometryIndexVBO");
	geometryIndexVBO->upload(sizeof(GLuint)*geometryIndexSize, geometryIndex);
	free(geometryIndex);

	if (waterVBO)
		delete waterVBO;
	waterVBO = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::dynamic_draw, "terrain::waterVBO");
	waterVBO->upload(sizeof(WaterVertex)*waterSize, water);
	free(water);

	if (waterIndexVBO)
		delete waterIndexVBO;
	if (waterIndexSize > 0)
	{
		waterIndexVBO = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::index_buffer, gfx_api::context::buffer_storage_hint::static_draw, "terrain::waterIndexVBO");
		waterIndexVBO->upload(sizeof(GLuint)*waterIndexSize, waterIndex);
	}
	else
	{
		waterIndexVBO = nullptr;
	}
	free(waterIndex);


	////////////////////
	// fill the texture part of the sectors
	const size_t numGroundTypes = getNumGroundTypes();
	texture = (PIELIGHT *)malloc(sizeof(PIELIGHT) * xSectors * ySectors * (sectorSize + 1) * (sectorSize + 1) * 2 * numGroundTypes);
	textureIndex = (GLuint *)malloc(sizeof(GLuint) * xSectors * ySectors * sectorSize * sectorSize * 12 * numGroundTypes);
	textureSize = 0;
	textureIndexSize = 0;
	for (layer = 0; layer < numGroundTypes; layer++)
	{
		for (x = 0; x < xSectors; x++)
		{
			for (y = 0; y < ySectors; y++)
			{
				if (layer == 0)
				{
					sectors[x * ySectors + y].textureOffset = std::unique_ptr<int[]> (new int[numGroundTypes]());
					sectors[x * ySectors + y].textureSize = std::unique_ptr<int[]> (new int[numGroundTypes]());
					sectors[x * ySectors + y].textureIndexOffset = std::unique_ptr<int[]> (new int[numGroundTypes]());
					sectors[x * ySectors + y].textureIndexSize = std::unique_ptr<int[]> (new int[numGroundTypes]());
				}

				sectors[x * ySectors + y].textureOffset[layer] = textureSize;
				sectors[x * ySectors + y].textureSize[layer] = 0;
				sectors[x * ySectors + y].textureIndexOffset[layer] = textureIndexSize;
				sectors[x * ySectors + y].textureIndexSize[layer] = 0;
				//debug(LOG_WARNING, "offset when filling %i: %i", layer, xSectors*ySectors*(sectorSize+1)*(sectorSize+1)*2*layer);
				for (i = 0; i < sectorSize + 1; i++)
				{
					for (j = 0; j < sectorSize + 1; j++)
					{
						bool draw = false;
						bool off_map;

						// set transparency
						for (a = 0; a < 2; a++)
						{
							for (b = 0; b < 2; b++)
							{
								absX = x * sectorSize + i + a;
								absY = y * sectorSize + j + b;
								colour[a][b].rgba = 0x00FFFFFF; // transparent

								// extend the terrain type for the bottom and left edges of the map
								off_map = false;
								if (absX == mapWidth)
								{
									off_map = true;
									absX--;
								}
								if (absY == mapHeight)
								{
									off_map = true;
									absY--;
								}

								if (absX < 0 || absY < 0 || absX >= mapWidth || absY >= mapHeight)
								{
									// not on the map, so don't draw
									continue;
								}
								if (mapTile(absX, absY)->ground == layer)
								{
									colour[a][b].rgba = 0xFFFFFFFF;
									if (!off_map)
									{
										// if this point lies on the edge is may not force this tile to be drawn
										// otherwise this will give a bright line when fog is enabled
										draw = true;
									}
								}
							}
						}
						texture[xSectors * ySectors * (sectorSize + 1) * (sectorSize + 1) * 2 * layer + ((x * ySectors + y) * (sectorSize + 1) * (sectorSize + 1) * 2 + (i * (sectorSize + 1) + j) * 2)].rgba = colour[0][0].rgba;
						averageColour(&centerColour, colour[0][0], colour[0][1], colour[1][0], colour[1][1]);
						texture[xSectors * ySectors * (sectorSize + 1) * (sectorSize + 1) * 2 * layer + ((x * ySectors + y) * (sectorSize + 1) * (sectorSize + 1) * 2 + (i * (sectorSize + 1) + j) * 2 + 1)].rgba = centerColour.rgba;
						textureSize += 2;
						if ((draw) && i < sectorSize && j < sectorSize)
						{
							textureIndex[textureIndexSize + 0]  = q(i  , j  , 1);
							textureIndex[textureIndexSize + 1]  = q(i  , j  , 0);
							textureIndex[textureIndexSize + 2]  = q(i + 1, j  , 0);

							textureIndex[textureIndexSize + 3]  = q(i  , j  , 1);
							textureIndex[textureIndexSize + 4]  = q(i  , j + 1, 0);
							textureIndex[textureIndexSize + 5]  = q(i  , j  , 0);

							textureIndex[textureIndexSize + 6]  = q(i  , j  , 1);
							textureIndex[textureIndexSize + 7]  = q(i + 1, j + 1, 0);
							textureIndex[textureIndexSize + 8]  = q(i  , j + 1, 0);

							textureIndex[textureIndexSize + 9]  = q(i  , j  , 1);
							textureIndex[textureIndexSize + 10] = q(i + 1, j  , 0);
							textureIndex[textureIndexSize + 11] = q(i + 1, j + 1, 0);
							textureIndexSize += 12;
						}

					}
				}
				sectors[x * ySectors + y].textureSize[layer] = textureSize - sectors[x * ySectors + y].textureOffset[layer];
				sectors[x * ySectors + y].textureIndexSize[layer] = textureIndexSize - sectors[x * ySectors + y].textureIndexOffset[layer];
			}
		}
	}
	if (textureVBO)
		delete textureVBO;
	textureVBO = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::static_draw, "terrain::textureVBO");
	textureVBO->upload(sizeof(PIELIGHT)*xSectors * ySectors * (sectorSize + 1) * (sectorSize + 1) * 2 * numGroundTypes, texture);
	free(texture);

	if (textureIndexVBO)
		delete textureIndexVBO;
	textureIndexVBO = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::index_buffer, gfx_api::context::buffer_storage_hint::static_draw, "terrain::textureIndexVBO");
	textureIndexVBO->upload(sizeof(GLuint)*textureIndexSize, textureIndex);
	free(textureIndex);

	// and finally the decals
	gfx_api::TerrainDecalVertex *terrainDecalData = nullptr;
	int terrainDecalSize = 0;

	switch (terrainShaderType)
	{
	case TerrainShaderType::FALLBACK:
		decaldata = (DecalVertex *)malloc(sizeof(DecalVertex) * mapWidth * mapHeight * 12);
		decalSize = 0;
		break;
	case TerrainShaderType::SINGLE_PASS:
		terrainDecalData = (gfx_api::TerrainDecalVertex *)malloc(sizeof(gfx_api::TerrainDecalVertex) * mapWidth * mapHeight * 12);
		terrainDecalSize = 0;
		break;
	}

	for (x = 0; x < xSectors; x++)
	{
		for (y = 0; y < ySectors; y++)
		{
			switch (terrainShaderType)
			{
			case TerrainShaderType::FALLBACK:
				sectors[x * ySectors + y].decalOffset = decalSize;
				sectors[x * ySectors + y].decalSize = 0;
				setSectorDecals(x, y, decaldata, &decalSize);
				sectors[x * ySectors + y].decalSize = decalSize - sectors[x * ySectors + y].decalOffset;
				break;
			case TerrainShaderType::SINGLE_PASS:
				sectors[x * ySectors + y].terrainAndDecalOffset = terrainDecalSize;
				sectors[x * ySectors + y].terrainAndDecalSize = 0;
				setSectorDecalVertex_SinglePass(x, y, terrainDecalData, &terrainDecalSize);
				sectors[x * ySectors + y].terrainAndDecalSize = terrainDecalSize - sectors[x * ySectors + y].terrainAndDecalOffset;
				break;
			}
		}
	}
	debug(LOG_TERRAIN, "%i decals found", decalSize / 12);
	if (decalVBO)
		delete decalVBO;
	if (decalSize > 0)
	{
		decalVBO = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::dynamic_draw, "terrain::decalVBO");
		decalVBO->upload(sizeof(DecalVertex)*decalSize, decaldata);
	}
	else
	{
		decalVBO = nullptr;
	}
	if (decaldata)
	{
		free(decaldata);
		decaldata = nullptr;
	}

	if (terrainDecalVBO)
		delete terrainDecalVBO;
	if (terrainDecalSize > 0)
	{
		terrainDecalVBO = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::dynamic_draw, "terrain::terrainDecalVBO");
		terrainDecalVBO->upload(sizeof(gfx_api::TerrainDecalVertex)*terrainDecalSize, terrainDecalData);
	}
	else
	{
		terrainDecalVBO = nullptr;
	}
	if (terrainDecalData)
	{
		free(terrainDecalData);
		terrainDecalData = nullptr;
	}

	lightmapLastUpdate = 0;
	lightmapWidth = 1;
	lightmapHeight = 1;
	// determine the smallest power-of-two size we can use for the lightmap
	while (mapWidth > (lightmapWidth <<= 1)) {}
	while (mapHeight > (lightmapHeight <<= 1)) {}
	debug(LOG_TERRAIN, "the size of the map is %ix%i", mapWidth, mapHeight);
	debug(LOG_TERRAIN, "lightmap texture size is %zu x %zu", lightmapWidth, lightmapHeight);

	// Prepare the lightmap pixmap and texture
	lightmapPixmap = std::unique_ptr<iV_Image>(new iV_Image());

	// not every system may have RGB texture support, so may need to expand to 4-channel (RGBA)
	auto lightmapChannels = gfx_api::context::get().getClosestSupportedUncompressedImageFormatChannels(gfx_api::pixel_format_target::texture_2d, 3); // ideal is RGB, but check if supported
	ASSERT_OR_RETURN(false, lightmapChannels.has_value(), "Exhausted all possible uncompressed formats for lightmap texture??");

	if (lightmapPixmap == nullptr || !lightmapPixmap->allocate(lightmapWidth, lightmapHeight, lightmapChannels.value(), true))
	{
		debug(LOG_FATAL, "Out of memory!");
		abort();
		return false;
	}
	if (lightmapPixmap->channels() == 4)
	{
		// must set alpha channel to opaque
		auto lightmapWritePtr = lightmapPixmap->bmp_w();
		size_t lightmapNumPixels = static_cast<size_t>(lightmapPixmap->width()) * static_cast<size_t>(lightmapPixmap->height());
		for (size_t pixelIndex = 0; pixelIndex < lightmapNumPixels; ++pixelIndex)
		{
			lightmapWritePtr[pixelIndex * 4 + 3] = 255;
		}
	}
	if (lightmap_texture)
		delete lightmap_texture;
	lightmap_texture = gfx_api::context::get().create_texture(1, lightmapPixmap->width(), lightmapPixmap->height(), lightmapPixmap->pixel_format(), "mem::lightmap");

	lightmap_texture->upload(0, *(lightmapPixmap.get()));
	terrainInitialised = true;

	return true;
}

/// free all memory and opengl buffers used by the terrain renderer
void shutdownTerrain()
{
	if (!sectors)
	{
		// This happens in some cases when loading a savegame from level init
		debug(LOG_ERROR, "Trying to shutdown terrain when we did not need to!");
		return;
	}
	delete geometryVBO;
	geometryVBO = nullptr;
	delete geometryIndexVBO;
	geometryIndexVBO = nullptr;
	delete waterVBO;
	waterVBO = nullptr;
	delete waterIndexVBO;
	waterIndexVBO = nullptr;
	delete textureVBO;
	textureVBO = nullptr;
	delete textureIndexVBO;
	textureIndexVBO = nullptr;
	delete decalVBO;
	decalVBO = nullptr;
	delete terrainDecalVBO;
	terrainDecalVBO = nullptr;

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			sectors[x * ySectors + y].textureOffset = nullptr;
			sectors[x * ySectors + y].textureSize = nullptr;
			sectors[x * ySectors + y].textureIndexOffset = nullptr;
			sectors[x * ySectors + y].textureIndexSize = nullptr;
		}
	}
	sectors = nullptr;
	delete lightmap_texture;
	lightmap_texture = nullptr;
	lightmapPixmap = nullptr;

	delete groundTexArr; groundTexArr = nullptr;
	delete groundNormalArr; groundNormalArr = nullptr;
	delete groundSpecularArr; groundSpecularArr = nullptr;
	delete groundHeightArr; groundHeightArr = nullptr;

	delete decalTexArr; decalTexArr = nullptr;
	delete decalNormalArr; decalNormalArr = nullptr;
	delete decalSpecularArr; decalSpecularArr = nullptr;
	delete decalHeightArr; decalHeightArr = nullptr;

	terrainInitialised = false;
}

static void updateLightMap()
{
	size_t lightmapChannels = lightmapPixmap->channels();
	unsigned char* lightMapWritePtr = lightmapPixmap->bmp_w();
	for (int j = 0; j < mapHeight; ++j)
	{
		for (int i = 0; i < mapWidth; ++i)
		{
			MAPTILE *psTile = mapTile(i, j);
			PIELIGHT colour = psTile->colour;

			if (psTile->tileInfoBits & BITS_GATEWAY && showGateways)
			{
				colour.byte.g = 255;
			}
			if (psTile->tileInfoBits & BITS_MARKED)
			{
				int m = getModularScaledGraphicsTime(2048, 255);
				colour.byte.r = MAX(m, 255 - m);
			}

			lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 0] = colour.byte.r;
			lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 1] = colour.byte.g;
			lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 2] = colour.byte.b;

			if (!pie_GetFogStatus())
			{
				// fade to black at the edges of the visible terrain area
				const float playerX = map_coordf(playerPos.p.x);
				const float playerY = map_coordf(playerPos.p.z);

				const float distA = i - (playerX - visibleTiles.x / 2);
				const float distB = (playerX + visibleTiles.x / 2) - i;
				const float distC = j - (playerY - visibleTiles.y / 2);
				const float distD = (playerY + visibleTiles.y / 2) - j;
				float darken, distToEdge;

				// calculate the distance to the closest edge of the visible map
				// determine the smallest distance
				distToEdge = distA;
				if (distB < distToEdge)
				{
					distToEdge = distB;
				}
				if (distC < distToEdge)
				{
					distToEdge = distC;
				}
				if (distD < distToEdge)
				{
					distToEdge = distD;
				}

				darken = (distToEdge) / 2.0f;
				if (darken <= 0)
				{
					lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 0] = 0;
					lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 1] = 0;
					lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 2] = 0;
				}
				else if (darken < 1)
				{
					lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 0] *= darken;
					lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 1] *= darken;
					lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 2] *= darken;
				}
			}
		}
	}
}

static void cullTerrain()
{
	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			float xPos = world_coord(x * sectorSize + sectorSize / 2);
			float yPos = world_coord(y * sectorSize + sectorSize / 2);
			float distance = pow(playerPos.p.x - xPos, 2) + pow(playerPos.p.z - yPos, 2);

			if (distance > pow((double)world_coord(terrainDistance), 2))
			{
				sectors[x * ySectors + y].draw = false;
			}
			else
			{
				sectors[x * ySectors + y].draw = true;
				if (sectors[x * ySectors + y].dirty)
				{
					updateSectorGeometry(x, y);
					sectors[x * ySectors + y].dirty = false;
				}
			}
		}
	}
}

static void drawDepthOnly(const glm::mat4 &ModelViewProjection, const glm::vec4 &paramsXLight, const glm::vec4 &paramsYLight)
{
	const auto &renderState = getCurrentRenderState();
	
	// bind the vertex buffer
	gfx_api::TerrainDepth::get().bind();
	gfx_api::TerrainDepth::get().bind_textures(lightmap_texture);
	gfx_api::TerrainDepth::get().bind_vertex_buffers(geometryVBO);
	gfx_api::TerrainDepth::get().bind_constants({ ModelViewProjection, paramsXLight, paramsYLight, glm::vec4(0.f), glm::vec4(0.f), glm::mat4(1.f), glm::mat4(1.f), 
	glm::vec4(0.f), renderState.fogEnabled, renderState.fogBegin, renderState.fogEnd, 0, 0 });
	gfx_api::context::get().bind_index_buffer(*geometryIndexVBO, gfx_api::index_type::u32);

	// draw slightly higher distance than it actually is so it will not
	// by accident obscure the actual terrain
	gfx_api::context::get().set_polygon_offset(0.1f, 1.f);

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			if (sectors[x * ySectors + y].draw)
			{
				addDrawRangeElements<gfx_api::TerrainDepth>(
					sectors[x * ySectors + y].geometryOffset,
					sectors[x * ySectors + y].geometryOffset + sectors[x * ySectors + y].geometrySize,
					sectors[x * ySectors + y].geometryIndexSize,
					sectors[x * ySectors + y].geometryIndexOffset);
			}
		}
	}
	finishDrawRangeElements<gfx_api::TerrainDepth>();
	gfx_api::context::get().set_polygon_offset(0.f, 0.f);
	gfx_api::TerrainDepth::get().unbind_vertex_buffers(geometryVBO);
	gfx_api::context::get().unbind_index_buffer(*geometryIndexVBO);
}

glm::vec4 getFogColorVec4()
{
	const auto &renderState = getCurrentRenderState();
	return glm::vec4(
		renderState.fogColour.vector[0] / 255.f,
		renderState.fogColour.vector[1] / 255.f,
		renderState.fogColour.vector[2] / 255.f,
		renderState.fogColour.vector[3] / 255.f
	);
}

static void drawTerrainLayers(const glm::mat4 &ModelViewProjection, const glm::vec4 &paramsXLight, const glm::vec4 &paramsYLight, const glm::mat4 &textureMatrix)
{
	const auto &renderState = getCurrentRenderState();
	const glm::vec4 fogColor(
		renderState.fogColour.vector[0] / 255.f,
		renderState.fogColour.vector[1] / 255.f,
		renderState.fogColour.vector[2] / 255.f,
		renderState.fogColour.vector[3] / 255.f
	);

	// load the vertex (geometry) buffer
	gfx_api::TerrainLayer::get().bind();
	gfx_api::TerrainLayer::get().bind_vertex_buffers(geometryVBO, textureVBO);
	gfx_api::context::get().bind_index_buffer(*textureIndexVBO, gfx_api::index_type::u32);
	const size_t numGroundTypes = getNumGroundTypes();
	ASSERT_OR_RETURN(, numGroundTypes, "Ground type was not set, no textures will be seen.");

	int32_t maxGfxTextureSize = gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_TEXTURE_SIZE);
	int maxTerrainTextureSize = std::max(std::min({getTextureSize(), maxGfxTextureSize}), MIN_TERRAIN_TEXTURE_SIZE);

	// draw each layer separately
	for (int layer = 0; layer < numGroundTypes; layer++)
	{
		const auto& groundType = getGroundType(layer);
		const glm::vec4 paramsX(0, 0, -1.0f / world_coord(groundType.textureSize), 0 );
		const glm::vec4 paramsY(1.0f / world_coord(groundType.textureSize), 0, 0, 0 );
		gfx_api::TerrainLayer::get().bind_constants({ ModelViewProjection, paramsX, paramsY, paramsXLight, paramsYLight, glm::mat4(1.f), textureMatrix,
			fogColor, renderState.fogEnabled, renderState.fogBegin, renderState.fogEnd, 0, 1 });

		// load the texture
		optional<size_t> texPage = iV_GetTexture(groundType.textureName.c_str(), gfx_api::texture_type::game_texture, maxTerrainTextureSize, maxTerrainTextureSize);
		ASSERT_OR_RETURN(, texPage.has_value(), "Failed to retrieve terrain texture: %s", groundType.textureName.c_str());
		gfx_api::TerrainLayer::get().bind_textures(&pie_Texture(texPage.value()), lightmap_texture);

		// load the color buffer
		gfx_api::context::get().bind_vertex_buffers(1, { std::make_tuple(textureVBO, static_cast<size_t>(sizeof(PIELIGHT)*xSectors * ySectors * (sectorSize + 1) * (sectorSize + 1) * 2 * layer)) });

		for (int x = 0; x < xSectors; x++)
		{
			for (int y = 0; y < ySectors; y++)
			{
				if (sectors[x * ySectors + y].draw)
				{
					addDrawRangeElements<gfx_api::TerrainLayer>(
						sectors[x * ySectors + y].geometryOffset,
						sectors[x * ySectors + y].geometryOffset + sectors[x * ySectors + y].geometrySize,
						sectors[x * ySectors + y].textureIndexSize[layer],
						sectors[x * ySectors + y].textureIndexOffset[layer]);
				}
			}
		}
		finishDrawRangeElements<gfx_api::TerrainLayer>();
	}
	gfx_api::TerrainLayer::get().unbind_vertex_buffers(geometryVBO, textureVBO);
	gfx_api::context::get().unbind_index_buffer(*textureIndexVBO);
}

static void drawDecals(const glm::mat4 &ModelViewProjection, const glm::vec4 &paramsXLight, const glm::vec4 &paramsYLight, const glm::mat4 &textureMatrix)
{
	if (!decalVBO)
	{
		return; // no decals
	}

	const auto &renderState = getCurrentRenderState();
	const glm::vec4 fogColor(
		renderState.fogColour.vector[0] / 255.f,
		renderState.fogColour.vector[1] / 255.f,
		renderState.fogColour.vector[2] / 255.f,
		renderState.fogColour.vector[3] / 255.f
	);
	gfx_api::TerrainDecals::get().bind();
	gfx_api::TerrainDecals::get().bind_textures(&pie_Texture(terrainPage), lightmap_texture);
	gfx_api::TerrainDecals::get().bind_vertex_buffers(decalVBO);
	gfx_api::TerrainDecals::get().bind_constants({ ModelViewProjection, textureMatrix, paramsXLight, paramsYLight,
		fogColor, renderState.fogEnabled, renderState.fogBegin, renderState.fogEnd, 0, 1 });

	int size = 0;
	int offset = 0;
	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors + 1; y++)
		{
			if (y < ySectors && offset + size == sectors[x * ySectors + y].decalOffset && sectors[x * ySectors + y].draw)
			{
				// append
				size += sectors[x * ySectors + y].decalSize;
				continue;
			}
			// can't append, so draw what we have and start anew
			if (size > 0)
			{
				gfx_api::TerrainDecals::get().draw(size, offset);
			}
			size = 0;
			if (y < ySectors && sectors[x * ySectors + y].draw)
			{
				offset = sectors[x * ySectors + y].decalOffset;
				size = sectors[x * ySectors + y].decalSize;
			}
		}
	}
	gfx_api::TerrainDecals::get().unbind_vertex_buffers(decalVBO);
}

template<typename PSO>
static void drawTerrainCombinedmpl(const glm::mat4 &ModelViewProjection, const glm::mat4 &ModelUVLightmap, const Vector3f &cameraPos, const Vector3f &sunPos)
{
	const auto &renderState = getCurrentRenderState();
	PSO::get().bind();
	PSO::get().bind_textures(
		lightmap_texture,
		groundTexArr, groundNormalArr, groundSpecularArr, groundHeightArr,
		decalTexArr, decalNormalArr, decalSpecularArr, decalHeightArr);
	PSO::get().bind_vertex_buffers(terrainDecalVBO);
	glm::mat4 groundScale = glm::mat4(0);
	for (int i = 0; i < getNumGroundTypes(); i++) {
		groundScale[i/4][i%4] = 1.0f / (getGroundType(i).textureSize * world_coord(1));
	}
	gfx_api::TerrainCombinedUniforms uniforms = {
		ModelViewProjection, ModelUVLightmap, groundScale,
		glm::vec4(cameraPos, 0), glm::vec4(glm::normalize(sunPos), 0),
		pie_GetLighting0(LIGHT_EMISSIVE), pie_GetLighting0(LIGHT_AMBIENT), pie_GetLighting0(LIGHT_DIFFUSE), pie_GetLighting0(LIGHT_SPECULAR),
		getFogColorVec4(), renderState.fogEnabled, renderState.fogBegin, renderState.fogEnd, terrainShaderQuality
	};
	PSO::get().set_uniforms(uniforms);

	int size = 0;
	int offset = 0;
	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors + 1; y++)
		{
			bool drawSector = y < ySectors && sectors[x * ySectors + y].draw;
			if (drawSector && offset + size == sectors[x * ySectors + y].terrainAndDecalOffset)
			{
				// append
				size += sectors[x * ySectors + y].terrainAndDecalSize;
				continue;
			}
			// can't append, so draw what we have and start anew
			if (size > 0)
			{
				PSO::get().draw(size, offset);
			}
			size = 0;
			if (drawSector)
			{
				offset = sectors[x * ySectors + y].terrainAndDecalOffset;
				size = sectors[x * ySectors + y].terrainAndDecalSize;
			}
		}
	}
	PSO::get().unbind_vertex_buffers(terrainDecalVBO);
}

static void drawTerrainCombined(const glm::mat4 &ModelViewProjection, const glm::mat4 &ModelUVLightmap, const Vector3f &cameraPos, const Vector3f &sunPos)
{
	switch (terrainShaderQuality)
	{
		case TerrainShaderQuality::CLASSIC:
			drawTerrainCombinedmpl<gfx_api::TerrainCombined_Classic>(ModelViewProjection, ModelUVLightmap, cameraPos, sunPos);
			break;
		case TerrainShaderQuality::MEDIUM:
			drawTerrainCombinedmpl<gfx_api::TerrainCombined_Medium>(ModelViewProjection, ModelUVLightmap, cameraPos, sunPos);
			break;
		case TerrainShaderQuality::NORMAL_MAPPING:
			drawTerrainCombinedmpl<gfx_api::TerrainCombined_High>(ModelViewProjection, ModelUVLightmap, cameraPos, sunPos);
			break;
	}

}

/**
 * Update the lightmap and draw the terrain and decals.
 * This function first draws the terrain in black, and then uses additive blending to put the terrain layers
 * on it one by one. Finally the decals are drawn.
 */
void drawTerrain(const glm::mat4 &mvp, const Vector3f &cameraPos, const Vector3f &sunPos)
{
	const glm::vec4 paramsXLight(1.0f / world_coord(mapWidth) *((float)mapWidth / (float)lightmapWidth), 0, 0, 0);
	const glm::vec4 paramsYLight(0, 0, -1.0f / world_coord(mapHeight) *((float)mapHeight / (float)lightmapHeight), 0);

	///////////////////////////////////
	// set up the lightmap texture

	// we limit the framerate of the lightmap, because updating a texture is an expensive operation
	if (realTime - lightmapLastUpdate >= LIGHTMAP_REFRESH)
	{
		lightmapLastUpdate = realTime;
		updateLightMap();

		lightmap_texture->upload(0, *(lightmapPixmap.get()));
	}

	///////////////////////////////////
	// terrain culling
	cullTerrain();

	// shift the lightmap half a tile as lights are supposed to be placed at the center of a tile
	const glm::mat4 lightMatrix = glm::translate(glm::vec3(1.f / (float)lightmapWidth / 2, 1.f / (float)lightmapHeight / 2, 0.f));
	const auto ModelUVLightmap = lightMatrix * glm::transpose(glm::mat4(paramsXLight, paramsYLight, glm::vec4(0,0,1,0), glm::vec4(0,0,0,1)));

	//////////////////////////////////////
	// canvas to draw on
	drawDepthOnly(mvp, paramsXLight, paramsYLight);

	switch (terrainShaderType)
	{
	case TerrainShaderType::FALLBACK:
		///////////////////////////////////
		// terrain
		drawTerrainLayers(mvp, paramsXLight, paramsYLight, lightMatrix);

		//////////////////////////////////
		// decals
		drawDecals(mvp, paramsXLight, paramsYLight, lightMatrix);
		break;
	case TerrainShaderType::SINGLE_PASS:
		///////////////////////////////////
		// terrain + decals
		drawTerrainCombined(mvp, ModelUVLightmap, cameraPos, sunPos);
		break;
	}
}

/**
 * Draw the water.
 * sunPos and cameraPos in Model=WorldSpace
 */
void drawWaterImpl(const glm::mat4 &ModelViewProjection, const Vector3f &cameraPos, const Vector3f &sunPos)
{
	if (!waterIndexVBO)
	{
		return; // no water
	}

	const glm::vec4 paramsX(0, 0, -1.0f / world_coord(4), 0);
	const glm::vec4 paramsY(1.0f / world_coord(4), 0, 0, 0);
	const glm::vec4 paramsX2(0, 0, -1.0f / world_coord(5), 0);
	const glm::vec4 paramsY2(1.0f / world_coord(5), 0, 0, 0);
	const auto ModelUV1 = glm::translate(glm::vec3(waterOffset, 0.f, 0.f)) * glm::transpose(glm::mat4(paramsX, paramsY, glm::vec4(0,0,1,0), glm::vec4(0,0,0,1)));
	const auto ModelUV2 = glm::transpose(glm::mat4(paramsX2, paramsY2, glm::vec4(0,0,1,0), glm::vec4(0,0,0,1)));
	const auto &renderState = getCurrentRenderState();

	int32_t maxGfxTextureSize = gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_TEXTURE_SIZE);
	int maxTerrainTextureSize = std::max(std::min({getTextureSize(), maxGfxTextureSize}), MIN_TERRAIN_TEXTURE_SIZE);

	optional<size_t> water1_texPage = iV_GetTexture("page-80-water-1.png", gfx_api::texture_type::game_texture, maxTerrainTextureSize, maxTerrainTextureSize);
	optional<size_t> water2_texPage = iV_GetTexture("page-81-water-2.png", (terrainShaderQuality != TerrainShaderQuality::NORMAL_MAPPING) ? gfx_api::texture_type::specular_map : gfx_api::texture_type::game_texture, maxTerrainTextureSize, maxTerrainTextureSize);
	ASSERT_OR_RETURN(, water1_texPage.has_value() && water2_texPage.has_value(), "Failed to load water texture");
	gfx_api::WaterPSO::get().bind();

	auto getOptTex = [&maxTerrainTextureSize](const std::string &fileName, gfx_api::texture_type textureType) -> gfx_api::texture* {
		if (fileName.empty() || terrainShaderQuality != TerrainShaderQuality::NORMAL_MAPPING) return nullptr;
		auto texPage = iV_GetTexture(fileName.c_str(), textureType, maxTerrainTextureSize, maxTerrainTextureSize);
		if (!texPage.has_value())
		{
			return nullptr;
		}
		return &pie_Texture(texPage.value());
	};
	gfx_api::WaterPSO::get().bind_textures(
		&pie_Texture(water1_texPage.value()), &pie_Texture(water2_texPage.value()),
		getOptTex(waterTexture1_nm, gfx_api::texture_type::normal_map), getOptTex(waterTexture2_nm, gfx_api::texture_type::normal_map),
		getOptTex(waterTexture1_sm, gfx_api::texture_type::specular_map), getOptTex(waterTexture2_sm, gfx_api::texture_type::specular_map),
		getOptTex(waterTexture1_hm, gfx_api::texture_type::height_map), getOptTex(waterTexture2_hm, gfx_api::texture_type::height_map));
	gfx_api::WaterPSO::get().bind_vertex_buffers(waterVBO);
	gfx_api::WaterPSO::get().bind_constants({
		ModelViewProjection, ModelUV1, ModelUV2,
		glm::vec4(cameraPos, 0), glm::vec4(glm::normalize(sunPos), 0),
		pie_GetLighting0(LIGHT_EMISSIVE), pie_GetLighting0(LIGHT_AMBIENT), pie_GetLighting0(LIGHT_DIFFUSE), pie_GetLighting0(LIGHT_SPECULAR),
		glm::vec4(0.f), renderState.fogEnabled, renderState.fogBegin, renderState.fogEnd,
		waterOffset*10, terrainShaderQuality
	});

	gfx_api::context::get().bind_index_buffer(*waterIndexVBO, gfx_api::index_type::u32);

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			if (sectors[x * ySectors + y].draw)
			{
				addDrawRangeElements<gfx_api::WaterPSO>(
				                     sectors[x * ySectors + y].geometryOffset,
				                     sectors[x * ySectors + y].geometryOffset + sectors[x * ySectors + y].geometrySize,
				                     sectors[x * ySectors + y].waterIndexSize,
				                     sectors[x * ySectors + y].waterIndexOffset);
			}
		}
	}
	finishDrawRangeElements<gfx_api::WaterPSO>();
	gfx_api::WaterPSO::get().unbind_vertex_buffers(waterVBO);
	gfx_api::context::get().unbind_index_buffer(*waterIndexVBO);

	// move the water
	if (!gamePaused())
	{
		waterOffset += graphicsTimeAdjustedIncrement(0.1f);
	}
}

void drawWater(const glm::mat4 &ModelViewProjection, const Vector3f &cameraPos, const Vector3f &sunPos)
{
	drawWaterImpl(ModelViewProjection, cameraPos, sunPos);
}

// MARK: Terrain shader type / quality

TerrainShaderType getTerrainShaderType()
{
	return terrainShaderType;
}

TerrainShaderType determineSupportedTerrainShader()
{
	if (!gfx_api::context::get().supports2DTextureArrays())
	{
		return TerrainShaderType::FALLBACK;
	}
	if (!gfx_api::context::get().supportsIntVertexAttributes())
	{
		return TerrainShaderType::FALLBACK;
	}
	if (gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_ARRAY_TEXTURE_LAYERS) < 256) // "big enough" value
	{
		return TerrainShaderType::FALLBACK;
	}

	return TerrainShaderType::SINGLE_PASS;
}

TerrainShaderQuality getTerrainShaderQuality()
{
	return terrainShaderQuality;
}

bool setTerrainShaderQuality(TerrainShaderQuality newValue, bool force)
{
	if (!initializedTerrainShaderType)
	{
		// cache the desired value for when terrain shader type is initialized (post graphics context initialization)
		terrainShaderQuality = newValue;
		return true;
	}

	if (!force && newValue == terrainShaderQuality)
	{
		return true;
	}

	bool success = false;

	// Check whether the new shader can be used
	switch (terrainShaderType)
	{
	case TerrainShaderType::FALLBACK:
		// only "MEDIUM" is supported
		terrainShaderQuality = TerrainShaderQuality::MEDIUM;
		success = (newValue == TerrainShaderQuality::MEDIUM);
		break;
	case TerrainShaderType::SINGLE_PASS:
		// all quality modes are supported
		terrainShaderQuality = newValue;
		success = true;
		break;
	}

	if (success)
	{
		if (terrainShaderType == TerrainShaderType::SINGLE_PASS)
		{
			// mark all tiles dirty
			// (when switching between classic and other modes, recalculating tile heights is required due to water)
			for (size_t i = 0; i < xSectors * ySectors; ++i)
			{
				sectors[i].dirty = true;
			}
		}
	}

	return success;
}

bool setTerrainShaderQuality(TerrainShaderQuality newValue)
{
	return setTerrainShaderQuality(newValue, false);
}

std::vector<TerrainShaderQuality> getAllTerrainShaderQualityOptions()
{
	return { TerrainShaderQuality::CLASSIC, TerrainShaderQuality::MEDIUM, TerrainShaderQuality::NORMAL_MAPPING };
}

bool isSupportedTerrainShaderQualityOption(TerrainShaderQuality value)
{
	ASSERT_OR_RETURN(false, initializedTerrainShaderType, "Should only be called after graphics context initialization");
	switch (terrainShaderType)
	{
	case TerrainShaderType::FALLBACK:
		// only "CLASSIC" is supported with the fallback shaders
		return value == TerrainShaderQuality::MEDIUM;
	case TerrainShaderType::SINGLE_PASS:
		// all quality modes are supported
		return true;
	}
	return false; // silence compiler warning
}

void initTerrainShaderType()
{
	terrainShaderType = determineSupportedTerrainShader();
	initializedTerrainShaderType = true;
	setTerrainShaderQuality(terrainShaderQuality, true); // checks and resets unsupported values
}

std::string to_display_string(TerrainShaderQuality value)
{
	switch (value)
	{
		case TerrainShaderQuality::CLASSIC:
			return _("Classic");
		case TerrainShaderQuality::MEDIUM:
			return _("Normal");
		case TerrainShaderQuality::NORMAL_MAPPING:
			return _("High");
	}
	return ""; // silence warning
}
