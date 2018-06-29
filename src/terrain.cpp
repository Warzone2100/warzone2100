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
#include "lib/framework/opengl.h"
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
	int *textureOffset;      ///< An array containing the offsets into the texture VBO for each terrain layer
	int *textureSize;        ///< The size of the geometry for this layer for each layer
	int *textureIndexOffset; ///< The offset into the index VBO for the texture for each layer
	int *textureIndexSize;   ///< The size of the indices for each layer
	int decalOffset;         ///< Index into the decal VBO
	int decalSize;           ///< Size of the part of the decal VBO we are going to use
	bool draw;               ///< Do we draw this sector this frame?
	bool dirty;              ///< Do we need to update the geometry for this sector?
};

using RenderVertex = Vector3f;

/// A vertex with a position and texture coordinates
struct DecalVertex
{
	Vector3f pos = Vector3f(0.f, 0.f, 0.f);
	Vector2f uv = Vector2f(0.f, 0.f);
};

/// The lightmap texture
static gfx_api::texture* lightmap_tex_num = nullptr;
/// When are we going to update the lightmap next?
static unsigned int lightmapLastUpdate;
/// How big is the lightmap?
static int lightmapWidth;
static int lightmapHeight;
/// Lightmap image
static GLubyte *lightmapPixmap;
/// Ticks per lightmap refresh
static const unsigned int LIGHTMAP_REFRESH = 80;

/// VBOs
static GLuint geometryVBO, geometryIndexVBO, textureVBO, textureIndexVBO, decalVBO;
/// VBOs
static GLuint waterVBO, waterIndexVBO;
/// The amount we shift the water textures so the waves appear to be moving
static float waterOffset;

/// These are properties of your videocard and hardware
static GLint GLmaxElementsVertices, GLmaxElementsIndices;

/// The sectors are stored here
static Sector *sectors;
/// The default sector size (a sector is sectorSize x sectorSize)
static int sectorSize = 15;
/// What is the distance we can see
static int terrainDistance;
/// How many sectors have we actually got?
static int xSectors, ySectors;

/// Did we initialise the terrain renderer yet?
static bool terrainInitialised = false;

/// Helper to specify the offset in a VBO
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

/// Helper variables for the DrawRangeElements functions
GLuint dreStart, dreEnd, dreOffset;
GLsizei dreCount;
/// Are we actually drawing something using the DrawRangeElements functions?
bool drawRangeElementsStarted = false;

/// Pass all remaining triangles to OpenGL
static void finishDrawRangeElements()
{
	if (drawRangeElementsStarted && dreCount > 0)
	{
		ASSERT(dreEnd - dreStart + 1 <= GLmaxElementsVertices, "too many vertices (%i)", (int)(dreEnd - dreStart + 1));
		ASSERT(dreCount <= GLmaxElementsIndices, "too many indices (%i)", (int)dreCount);
		glDrawRangeElements(GL_TRIANGLES,
		                    dreStart,
		                    dreEnd,
		                    dreCount,
		                    GL_UNSIGNED_INT,
		                    BUFFER_OFFSET(sizeof(GLuint)*dreOffset));
	}
	drawRangeElementsStarted = false;
}

/**
 * Either draw the elements or batch them to be sent to OpenGL later
 * This improves performance by reducing the amount of OpenGL calls.
 */
static void addDrawRangeElements(GLenum mode,
                                 GLuint start,
                                 GLuint end,
                                 GLsizei count,
                                 GLenum type,
                                 GLuint offset)
{
	ASSERT(mode == GL_TRIANGLES, "not supported");
	ASSERT(type == GL_UNSIGNED_INT, "not supported");

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
		finishDrawRangeElements();
		// start anew
		addDrawRangeElements(mode, start, end, count, type, offset);
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

// NOTE:  The current (max) texture size of a tile is 128x128.  We allow up to a user defined texture size
// of 2048.  This will cause ugly seams for the decals, if user picks a texture size bigger than the tile!
#define MAX_TILE_TEXTURE_SIZE 128.0f
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
	if (texsize > MAX_TILE_TEXTURE_SIZE)
	{
		texsize = MAX_TILE_TEXTURE_SIZE;
	}
	const float centertile = 0.5f / texsize;	// compute center of tile
	const float shiftamount = (texsize - 1.0) / texsize;	// 1 pixel border
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
	const Vector2f offset { tileTexInfo[tile].uOffset, tileTexInfo[tile].vOffset };

	uv[0 + 0] = offset + sP1;
	uv[0 + 2] = offset + sP2;
	uv[1 + 2] = offset + sP3;
	uv[1 + 0] = offset + sP4;

	/// Calculate the average texture coordinates of 4 points
	return Vector2f { (uv[0].x + uv[1].x + uv[2].x + uv[3].x) / 4, (uv[0].y + uv[1].y + uv[2].y + uv[3].y) / 4 };
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
		result->y = map_TileHeight(x, y);
		if (water)
		{
			result->y = map_WaterHeight(x, y);
		}
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
static void setSectorGeometry(int x, int y,
                              RenderVertex *geometry, RenderVertex *water,
                              int *geometrySize, int *waterSize)
{
	Vector3i pos;
	int i, j;
	for (i = 0; i < sectorSize + 1; i++)
	{
		for (j = 0; j < sectorSize + 1; j++)
		{
			// set up geometry
			getGridPos(&pos, i + x * sectorSize, j + y * sectorSize, false, false);
			geometry[*geometrySize].x = pos.x;
			geometry[*geometrySize].y = pos.y;
			geometry[*geometrySize].z = pos.z;
			(*geometrySize)++;

			getGridPos(&pos, i + x * sectorSize, j + y * sectorSize, true, false);
			geometry[*geometrySize].x = pos.x;
			geometry[*geometrySize].y = pos.y;
			geometry[*geometrySize].z = pos.z;
			(*geometrySize)++;

			getGridPos(&pos, i + x * sectorSize, j + y * sectorSize, false, true);
			water[*waterSize].x = pos.x;
			water[*waterSize].y = pos.y;
			water[*waterSize].z = pos.z;
			(*waterSize)++;

			getGridPos(&pos, i + x * sectorSize, j + y * sectorSize, true, true);
			water[*waterSize].x = pos.x;
			water[*waterSize].y = pos.y;
			water[*waterSize].z = pos.z;
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

/**
 * Update the sector for when the terrain is changed.
 */
static void updateSectorGeometry(int x, int y)
{
	RenderVertex *geometry;
	RenderVertex *water;
	DecalVertex *decaldata;
	int geometrySize = 0;
	int waterSize = 0;
	int decalSize = 0;

	geometry  = (RenderVertex *)malloc(sizeof(RenderVertex) * sectors[x * ySectors + y].geometrySize);
	water     = (RenderVertex *)malloc(sizeof(RenderVertex) * sectors[x * ySectors + y].waterSize);

	setSectorGeometry(x, y, geometry, water, &geometrySize, &waterSize);
	ASSERT(geometrySize == sectors[x * ySectors + y].geometrySize, "something went seriously wrong updating the terrain");
	ASSERT(waterSize    == sectors[x * ySectors + y].waterSize   , "something went seriously wrong updating the terrain");

	glBindBuffer(GL_ARRAY_BUFFER, geometryVBO);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(RenderVertex)*sectors[x * ySectors + y].geometryOffset,
	                sizeof(RenderVertex)*sectors[x * ySectors + y].geometrySize, geometry);
	glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(RenderVertex)*sectors[x * ySectors + y].waterOffset,
	                sizeof(RenderVertex)*sectors[x * ySectors + y].waterSize, water);

	free(geometry);
	free(water);

	if (sectors[x * ySectors + y].decalSize <= 0)
	{
		// Nothing to do here, and glBufferSubData(GL_ARRAY_BUFFER, 0, 0, *) crashes in my graphics driver. Probably shouldn't crash...
		return;
	}

	decaldata = (DecalVertex *)malloc(sizeof(DecalVertex) * sectors[x * ySectors + y].decalSize);
	setSectorDecals(x, y, decaldata, &decalSize);
	ASSERT(decalSize == sectors[x * ySectors + y].decalSize   , "the amount of decals has changed");

	glBindBuffer(GL_ARRAY_BUFFER, decalVBO);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(DecalVertex)*sectors[x * ySectors + y].decalOffset,
	                sizeof(DecalVertex)*sectors[x * ySectors + y].decalSize, decaldata);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	free(decaldata);

	glBindBuffer(GL_ARRAY_BUFFER, 0);  // HACK Must unbind GL_ARRAY_BUFFER (don't know if it has to be unbound everywhere), otherwise text rendering may mysteriously crash.
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

/**
 * Check what the videocard + drivers support and divide the loaded map into sectors that can be drawn.
 * It also determines the lightmap size.
 */
bool initTerrain()
{
	int i, j, x, y, a, b, absX, absY;
	PIELIGHT colour[2][2], centerColour;
	int layer = 0;

	RenderVertex *geometry;
	RenderVertex *water;
	DecalVertex *decaldata;
	int geometrySize, geometryIndexSize;
	int waterSize, waterIndexSize;
	int textureSize, textureIndexSize;
	GLuint *geometryIndex;
	GLuint *waterIndex;
	GLuint *textureIndex;
	PIELIGHT *texture;
	int decalSize;
	int maxSectorSizeIndices, maxSectorSizeVertices;
	bool decreasedSize = false;

	// this information is useful to prevent crashes with buggy opengl implementations
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &GLmaxElementsVertices);
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES,  &GLmaxElementsIndices);

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
	sectors = (Sector *)malloc(sizeof(Sector) * xSectors * ySectors);

	////////////////////
	// fill the geometry part of the sectors
	geometry = (RenderVertex *)malloc(sizeof(RenderVertex) * xSectors * ySectors * (sectorSize + 1) * (sectorSize + 1) * 2);
	geometryIndex = (GLuint *)malloc(sizeof(GLuint) * xSectors * ySectors * sectorSize * sectorSize * 12);
	geometrySize = 0;
	geometryIndexSize = 0;

	water = (RenderVertex *)malloc(sizeof(RenderVertex) * xSectors * ySectors * (sectorSize + 1) * (sectorSize + 1) * 2);
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
	glGenBuffers(1, &geometryVBO);
	glBindBuffer(GL_ARRAY_BUFFER, geometryVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(RenderVertex)*geometrySize, geometry, GL_DYNAMIC_DRAW);
	free(geometry);

	glGenBuffers(1, &geometryIndexVBO);
	glBindBuffer(GL_ARRAY_BUFFER, geometryIndexVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint)*geometryIndexSize, geometryIndex, GL_STATIC_DRAW);
	free(geometryIndex);

	glGenBuffers(1, &waterVBO);
	glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(RenderVertex)*waterSize, water, GL_DYNAMIC_DRAW);
	free(water);

	glGenBuffers(1, &waterIndexVBO);
	glBindBuffer(GL_ARRAY_BUFFER, waterIndexVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint)*waterIndexSize, waterIndex, GL_STATIC_DRAW);
	free(waterIndex);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	////////////////////
	// fill the texture part of the sectors
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
					sectors[x * ySectors + y].textureOffset = (int *)malloc(sizeof(int) * numGroundTypes);
					sectors[x * ySectors + y].textureSize = (int *)malloc(sizeof(int) * numGroundTypes);
					sectors[x * ySectors + y].textureIndexOffset = (int *)malloc(sizeof(int) * numGroundTypes);
					sectors[x * ySectors + y].textureIndexSize = (int *)malloc(sizeof(int) * numGroundTypes);
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
	glGenBuffers(1, &textureVBO);
	glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(PIELIGHT)*xSectors * ySectors * (sectorSize + 1) * (sectorSize + 1) * 2 * numGroundTypes, texture, GL_STATIC_DRAW);
	free(texture);

	glGenBuffers(1, &textureIndexVBO);
	glBindBuffer(GL_ARRAY_BUFFER, textureIndexVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint)*textureIndexSize, textureIndex, GL_STATIC_DRAW);
	free(textureIndex);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// and finally the decals
	decaldata = (DecalVertex *)malloc(sizeof(DecalVertex) * mapWidth * mapHeight * 12);
	decalSize = 0;
	for (x = 0; x < xSectors; x++)
	{
		for (y = 0; y < ySectors; y++)
		{
			sectors[x * ySectors + y].decalOffset = decalSize;
			sectors[x * ySectors + y].decalSize = 0;
			setSectorDecals(x, y, decaldata, &decalSize);
			sectors[x * ySectors + y].decalSize = decalSize - sectors[x * ySectors + y].decalOffset;
		}
	}
	debug(LOG_TERRAIN, "%i decals found", decalSize / 12);
	glGenBuffers(1, &decalVBO);
	glBindBuffer(GL_ARRAY_BUFFER, decalVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(DecalVertex)*decalSize, decaldata, GL_DYNAMIC_DRAW);
	free(decaldata);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	lightmap_tex_num = 0;
	lightmapLastUpdate = 0;
	lightmapWidth = 1;
	lightmapHeight = 1;
	// determine the smallest power-of-two size we can use for the lightmap
	while (mapWidth > (lightmapWidth <<= 1)) {}
	while (mapHeight > (lightmapHeight <<= 1)) {}
	debug(LOG_TERRAIN, "the size of the map is %ix%i", mapWidth, mapHeight);
	debug(LOG_TERRAIN, "lightmap texture size is %ix%i", lightmapWidth, lightmapHeight);

	// Prepare the lightmap pixmap and texture
	lightmapPixmap = (GLubyte *)calloc(lightmapWidth * lightmapHeight, 3 * sizeof(GLubyte));
	if (lightmapPixmap == nullptr)
	{
		debug(LOG_FATAL, "Out of memory!");
		abort();
		return false;
	}
	if (lightmap_tex_num)
		delete lightmap_tex_num;
	lightmap_tex_num = gfx_api::context::get().create_texture(lightmapWidth, lightmapHeight, gfx_api::pixel_format::rgb);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	lightmap_tex_num->upload(0, 0, 0, lightmapWidth, lightmapHeight, gfx_api::pixel_format::rgb, lightmapPixmap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, lightmapWidth, lightmapHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, lightmapPixmap);

	terrainInitialised = true;

	glBindBuffer(GL_ARRAY_BUFFER, 0);  // HACK Must unbind GL_ARRAY_BUFFER (in this function, at least), otherwise text rendering may mysteriously crash.

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
	glDeleteBuffers(1, &geometryVBO);
	glDeleteBuffers(1, &geometryIndexVBO);
	glDeleteBuffers(1, &waterVBO);
	glDeleteBuffers(1, &waterIndexVBO);
	glDeleteBuffers(1, &textureVBO);
	glDeleteBuffers(1, &textureIndexVBO);
	glDeleteBuffers(1, &decalVBO);

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			free(sectors[x * ySectors + y].textureOffset);
			free(sectors[x * ySectors + y].textureSize);
			free(sectors[x * ySectors + y].textureIndexOffset);
			free(sectors[x * ySectors + y].textureIndexSize);
		}
	}
	free(sectors);
	sectors = nullptr;
	delete lightmap_tex_num;
	lightmap_tex_num = nullptr;
	free(lightmapPixmap);
	lightmapPixmap = nullptr;

	terrainInitialised = false;
}

static void updateLightMap()
{
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

			lightmapPixmap[(i + j * lightmapWidth) * 3 + 0] = colour.byte.r;
			lightmapPixmap[(i + j * lightmapWidth) * 3 + 1] = colour.byte.g;
			lightmapPixmap[(i + j * lightmapWidth) * 3 + 2] = colour.byte.b;

			if (!pie_GetFogStatus())
			{
				// fade to black at the edges of the visible terrain area
				const float playerX = map_coordf(player.p.x);
				const float playerY = map_coordf(player.p.z);

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
					lightmapPixmap[(i + j * lightmapWidth) * 3 + 0] = 0;
					lightmapPixmap[(i + j * lightmapWidth) * 3 + 1] = 0;
					lightmapPixmap[(i + j * lightmapWidth) * 3 + 2] = 0;
				}
				else if (darken < 1)
				{
					lightmapPixmap[(i + j * lightmapWidth) * 3 + 0] *= darken;
					lightmapPixmap[(i + j * lightmapWidth) * 3 + 1] *= darken;
					lightmapPixmap[(i + j * lightmapWidth) * 3 + 2] *= darken;
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
			float distance = pow(player.p.x - xPos, 2) + pow(player.p.z - yPos, 2);

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
	const auto &program = pie_ActivateShader(SHADER_TERRAIN_DEPTH, ModelViewProjection, paramsXLight, paramsYLight, 1, glm::mat4(1.f), glm::mat4(1.f));
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetRendMode(REND_OPAQUE);

	// we only draw in the depth buffer of using fog of war, as the clear color is black then
	if (!pie_GetFogStatus())
	{
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	}

	// draw slightly higher distance than it actually is so it will not
	// by accident obscure the actual terrain
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.1f, 1.0f);

	// bind the vertex buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometryIndexVBO);
	glBindBuffer(GL_ARRAY_BUFFER, geometryVBO);

	glVertexAttribPointer(program.locVertex, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), BUFFER_OFFSET(0));
	glEnableVertexAttribArray(program.locVertex);

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			if (sectors[x * ySectors + y].draw)
			{
				addDrawRangeElements(GL_TRIANGLES,
					sectors[x * ySectors + y].geometryOffset,
					sectors[x * ySectors + y].geometryOffset + sectors[x * ySectors + y].geometrySize,
					sectors[x * ySectors + y].geometryIndexSize,
					GL_UNSIGNED_INT,
					sectors[x * ySectors + y].geometryIndexOffset);
			}
		}
	}
	finishDrawRangeElements();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	if (!pie_GetFogStatus())
	{
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

	// disable the depth offset
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisableVertexAttribArray(program.locVertex);
	pie_DeactivateShader();
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
	const auto &program = pie_ActivateShader(SHADER_TERRAIN, ModelViewProjection, glm::vec4(0.f), glm::vec4(0.f), paramsXLight, paramsYLight, 0, 1,
		glm::mat4(1.f), textureMatrix, renderState.fogEnabled, renderState.fogBegin, renderState.fogEnd, fogColor);

	// additive blending
	pie_SetRendMode(REND_ADDITIVE);

	// only draw colors
	glDepthMask(GL_FALSE);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textureIndexVBO);

	// load the vertex (geometry) buffer
	glBindBuffer(GL_ARRAY_BUFFER, geometryVBO);
	glVertexAttribPointer(program.locVertex, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), BUFFER_OFFSET(0));
	glEnableVertexAttribArray(program.locVertex);

	glBindBuffer(GL_ARRAY_BUFFER, textureVBO);

	ASSERT_OR_RETURN(, psGroundTypes, "Ground type was not set, no textures will be seen.");

	// draw each layer separately
	for (int layer = 0; layer < numGroundTypes; layer++)
	{
		const glm::vec4 paramsX(0, 0, -1.0f / world_coord(psGroundTypes[layer].textureSize), 0 );
		const glm::vec4 paramsY(1.0f / world_coord(psGroundTypes[layer].textureSize), 0, 0, 0 );
		pie_ActivateShader(SHADER_TERRAIN, ModelViewProjection, paramsX, paramsY, paramsXLight, paramsYLight, 0, 1, glm::mat4(1.f), textureMatrix,
			renderState.fogEnabled, renderState.fogBegin, renderState.fogEnd, fogColor);

		// load the texture
		int texPage = iV_GetTexture(psGroundTypes[layer].textureName);
		pie_SetTexturePage(texPage);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// load the color buffer
		glVertexAttribPointer(program.locColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(PIELIGHT), BUFFER_OFFSET(sizeof(PIELIGHT)*xSectors * ySectors * (sectorSize + 1) * (sectorSize + 1) * 2 * layer));
		glEnableVertexAttribArray(program.locColor);

		for (int x = 0; x < xSectors; x++)
		{
			for (int y = 0; y < ySectors; y++)
			{
				if (sectors[x * ySectors + y].draw)
				{
					addDrawRangeElements(GL_TRIANGLES,
						sectors[x * ySectors + y].geometryOffset,
						sectors[x * ySectors + y].geometryOffset + sectors[x * ySectors + y].geometrySize,
						sectors[x * ySectors + y].textureIndexSize[layer],
						GL_UNSIGNED_INT,
						sectors[x * ySectors + y].textureIndexOffset[layer]);
				}
			}
		}
		finishDrawRangeElements();
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	// we don't need this one anymore
	glDisableVertexAttribArray(program.locVertex);
	glDisableVertexAttribArray(program.locColor);
	pie_DeactivateShader();
}

static void drawDecals(const glm::mat4 &ModelViewProjection, const glm::vec4 &paramsXLight, const glm::vec4 &paramsYLight, const glm::mat4 &textureMatrix)
{
	const auto &renderState = getCurrentRenderState();
	const glm::vec4 fogColor(
		renderState.fogColour.vector[0] / 255.f,
		renderState.fogColour.vector[1] / 255.f,
		renderState.fogColour.vector[2] / 255.f,
		renderState.fogColour.vector[3] / 255.f
	);
	const auto &program = pie_ActivateShader(SHADER_DECALS, ModelViewProjection, paramsXLight, paramsYLight, 0, 1, textureMatrix,
		renderState.fogEnabled, renderState.fogBegin, renderState.fogEnd, fogColor);
	// select the terrain texture page
	pie_SetTexturePage(terrainPage);

	// use the alpha to blend
	pie_SetRendMode(REND_ALPHA);

	// and the texture coordinates buffer
	glBindBuffer(GL_ARRAY_BUFFER, decalVBO);
	glVertexAttribPointer(program.locVertex, 3, GL_FLOAT, GL_FALSE, sizeof(DecalVertex), BUFFER_OFFSET(0));
	glEnableVertexAttribArray(program.locVertex);

	glVertexAttribPointer(program.locTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(DecalVertex), BUFFER_OFFSET(12));
	glEnableVertexAttribArray(program.locTexCoord);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

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
				glDrawArrays(GL_TRIANGLES, offset, size);
			}
			size = 0;
			if (y < ySectors && sectors[x * ySectors + y].draw)
			{
				offset = sectors[x * ySectors + y].decalOffset;
				size = sectors[x * ySectors + y].decalSize;
			}
		}
	}

	glDepthMask(GL_TRUE);
	glDisableVertexAttribArray(program.locTexCoord);
	glDisableVertexAttribArray(program.locVertex);
	pie_DeactivateShader();
}


/**
 * Update the lightmap and draw the terrain and decals.
 * This function first draws the terrain in black, and then uses additive blending to put the terrain layers
 * on it one by one. Finally the decals are drawn.
 */
void drawTerrain(const glm::mat4 &mvp)
{
	const glm::vec4 paramsXLight(1.0f / world_coord(mapWidth) *((float)mapWidth / lightmapWidth), 0, 0, 0);
	const glm::vec4 paramsYLight(0, 0, -1.0f / world_coord(mapHeight) *((float)mapHeight / lightmapHeight), 0);

	///////////////////////////////////
	// set up the lightmap texture
	glActiveTexture(GL_TEXTURE1);
	// bind the texture
	lightmap_tex_num->bind();

	// we limit the framerate of the lightmap, because updating a texture is an expensive operation
	if (realTime - lightmapLastUpdate >= LIGHTMAP_REFRESH)
	{
		lightmapLastUpdate = realTime;
		updateLightMap();

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		lightmap_tex_num->upload(0, 0, 0, lightmapWidth, lightmapHeight, gfx_api::pixel_format::rgb, lightmapPixmap);
	}

	///////////////////////////////////
	// terrain culling
	cullTerrain();

	glActiveTexture(GL_TEXTURE0);

	// shift the lightmap half a tile as lights are supposed to be placed at the center of a tile
	const glm::mat4 lightMatrix = glm::translate(glm::vec3(1.f / lightmapWidth / 2, 1.f / lightmapHeight / 2, 0.f));

	//////////////////////////////////////
	// canvas to draw on
	drawDepthOnly(mvp, paramsXLight, paramsYLight);

	///////////////////////////////////
	// terrain
	drawTerrainLayers(mvp, paramsXLight, paramsYLight, lightMatrix);

	//////////////////////////////////
	// decals
	drawDecals(mvp, paramsXLight, paramsYLight, lightMatrix);

	////////////////////////////////
	// disable the lightmap texture
	glActiveTexture(GL_TEXTURE1);
	glActiveTexture(GL_TEXTURE0);

	// leave everything in a sane state so it won't mess up somewhere else

	//glBindBuffer(GL_ARRAY_BUFFER, 0);  // HACK Must unbind GL_ARRAY_BUFFER (don't know if it has to be unbound everywhere), otherwise text rendering may mysteriously crash.
}

/**
 * Draw the water.
 */
void drawWater(const glm::mat4 &viewMatrix)
{
	int x, y;
	const glm::vec4 paramsX(0, 0, -1.0f / world_coord(4), 0);
	const glm::vec4 paramsY(1.0f / world_coord(4), 0, 0, 0);
	const glm::vec4 paramsX2(0, 0, -1.0f / world_coord(5), 0);
	const glm::vec4 paramsY2(1.0f / world_coord(5), 0, 0, 0);
	const auto &renderState = getCurrentRenderState();

	const auto &program = pie_ActivateShader(SHADER_WATER, viewMatrix, paramsX, paramsY, paramsX2, paramsY2, 0, 1,
		glm::translate(glm::vec3(waterOffset, 0.f, 0.f)), glm::mat4(1.f), renderState.fogEnabled, renderState.fogBegin, renderState.fogEnd);

	glDepthMask(GL_FALSE);

	// first texture unit
	pie_SetTexturePage(iV_GetTexture("page-80-water-1.png"));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// multiplicative blending
	pie_SetRendMode(REND_MULTIPLICATIVE);

	// second texture unit
	glActiveTexture(GL_TEXTURE1);
	pie_Texture(iV_GetTexture("page-81-water-2.png")).bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// bind the vertex buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterIndexVBO);
	glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
	glVertexAttribPointer(program.locVertex, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), BUFFER_OFFSET(0));
	glEnableVertexAttribArray(program.locVertex);

	for (x = 0; x < xSectors; x++)
	{
		for (y = 0; y < ySectors; y++)
		{
			if (sectors[x * ySectors + y].draw)
			{
				addDrawRangeElements(GL_TRIANGLES,
				                     sectors[x * ySectors + y].geometryOffset,
				                     sectors[x * ySectors + y].geometryOffset + sectors[x * ySectors + y].geometrySize,
				                     sectors[x * ySectors + y].waterIndexSize,
				                     GL_UNSIGNED_INT,
				                     sectors[x * ySectors + y].waterIndexOffset);
			}
		}
	}
	finishDrawRangeElements();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(program.locVertex);

	// move the water
	if (!gamePaused())
	{
		waterOffset += graphicsTimeAdjustedIncrement(0.1f);
	}

	// disable second texture
	glActiveTexture(GL_TEXTURE0);

	// clean up
	glDepthMask(GL_TRUE);
	pie_DeactivateShader();

	//glBindBuffer(GL_ARRAY_BUFFER, 0);  // HACK Must unbind GL_ARRAY_BUFFER (don't know if it has to be unbound everywhere), otherwise text rendering may mysteriously crash.
}
