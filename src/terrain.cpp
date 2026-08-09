// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#include "mikktspace.h"

#include "lib/framework/frame.h"
#include "lib/framework/loading_task.h"
#include "lib/framework/opengl.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/resource_loading_controller.h"
#include "resource_loading_dispatch.h"
#include "lib/framework/wzapp.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/imd.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/pietypes.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/pielighting.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piedraw.h"
#include "lib/ivis_opengl/pielight_convert.h"
#include "world_map_state.h"
#include <glm/mat4x4.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

#include "lib/framework/math_ext.h"

#include "terrain.h"
#include "terrain_surface.h"
#include "terrain_bake.h"
#include "map.h"
#include "texture.h"
#include "display3d.h"
#include "hci.h"
#include "loop.h"
#include "wzcrashhandlingproviders.h"
#include "lighting.h"

#include "profiling.h"

#include "game_world.h"

#include <algorithm>
#include <cstdint>

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

	int decalOffset = 0;         ///< Index into the decal VBO
	int decalSize = 0;           ///< Size of the part of the decal VBO we are going to use
	int terrainAndDecalOffset = 0;
	int terrainAndDecalSize = 0;
	int terrainAndDecalIndexOffset = 0; ///< The point in the terrain+decal index VBO where our triangles start
	int terrainAndDecalIndexSize = 0;   ///< The size of our terrain+decal indices
	int patchIndexOffset = 0; ///< The point in the patch index VBO where our patches start (HardwareTess strategy)
	int patchIndexSize = 0;   ///< The size of our patch indices (HardwareTess strategy)
	bool draw;               ///< Do we draw this sector this frame?
	bool dirty;              ///< Do we need to update the geometry for this sector?
};

// VBO for gfx_api::TerrainDepth / TerrainDepthPrepass (must match TerrainGeometryVertex layout).
struct TerrainVertex
{
	Vector3f pos = Vector3f(0.f, 0.f, 0.f);
	Vector3f normal = Vector3f(0.f, 1.f, 0.f);
};
static_assert(sizeof(TerrainVertex) == sizeof(gfx_api::TerrainGeometryVertex), "TerrainVertex layout must match gfx_api::TerrainGeometryVertex");
static_assert(offsetof(TerrainVertex, pos) == offsetof(gfx_api::TerrainGeometryVertex, pos), "TerrainVertex.pos offset mismatch");
static_assert(offsetof(TerrainVertex, normal) == offsetof(gfx_api::TerrainGeometryVertex, normal), "TerrainVertex.normal offset mismatch");

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
/// Lightmap values
struct LightmapCalculatedValues
{
	glm::vec4 paramsXLight = glm::vec4();
	glm::vec4 paramsYLight = glm::vec4();
	glm::mat4 lightMatrix = glm::mat4(); // shift the lightmap half a tile as lights are supposed to be placed at the center of a tile
	glm::mat4 ModelUVLightmap = glm::mat4();
};
static LightmapCalculatedValues lightmapValues;
/// Ticks per lightmap refresh
static const unsigned int LIGHTMAP_REFRESH = 80;

/// VBOs
static gfx_api::buffer *geometryVBO = nullptr, *geometryIndexVBO = nullptr;
/// VBOs
static gfx_api::buffer *waterVBO = nullptr, *waterIndexVBO = nullptr;
static gfx_api::buffer *terrainDecalVBO = nullptr, *terrainDecalIndexVBO = nullptr;
/// The amount we shift the water textures so the waves appear to be moving
static float waterOffset;

/// The sectors are stored here
static std::unique_ptr<Sector[]> sectors;
/// The sector size (a sector is sectorSize x sectorSize tiles)
static constexpr int sectorSize = 15;
/// Requested mesh subdivision factor (0 = uninitialized - pick default on first run)
static int terrainSubdivisionSetting = 0;
/// The mesh subdivision factor the current buffers were built with (1 = legacy geometry)
static int terrainSubdivision = 1;
/// How the dense terrain mesh is produced: CPU-subdivided VBOs, or hardware
/// tessellation (per-tile patches evaluating baked surface fields on the GPU)
enum class TerrainMeshStrategy { CPU, HardwareTess };
static TerrainMeshStrategy terrainMeshStrategy = TerrainMeshStrategy::CPU;
/// Config override for the mesh strategy ("terrainTessellation" config key)
static TerrainTessellationPreference terrainTessellationPreference = TerrainTessellationPreference::Auto;
/// Subdivision the combined terrain+decal mesh is built with. Equal to
/// terrainSubdivision on the CPU strategy. 1 under HardwareTess, where the
/// tessellator generates the dense mesh from the tile-corner patches instead.
static int combinedMeshSubdivision = 1;
/// Per-tile 4-corner patch indices into terrainDecalVBO (HardwareTess strategy)
static gfx_api::buffer* terrainPatchIndexVBO = nullptr;
/// What is the distance we can see
static int terrainDistance;
/// How many sectors have we actually got?
static int xSectors, ySectors;

/// Did we initialise the terrain renderer yet?
static bool terrainInitialised = false;

static std::vector<gfx_api::TerrainDecalVertex> terrainDecalVertexUpdateBuffer;

/// Helper to specify the offset in a VBO
#define BUFFER_OFFSET(i) (reinterpret_cast<char *>(i))

/// Helper variables for the draw-elements batching functions
static GLuint batchedIndexOffset;
static GLsizei batchedIndexCount;
/// Are we accumulating a draw-elements batch?
static bool drawElementsBatchActive = false;

TerrainShaderQuality terrainShaderQuality = TerrainShaderQuality::UNINITIALIZED_PICK_DEFAULT;
bool initializedTerrainShaderType = false;
int32_t maxTerrainMappingTextureSize = 0; // uninitialized - pick default on first run

void drawWaterClassic(const glm::mat4 &ModelViewProjection, const glm::mat4 &ModelUVLightmap, const Vector3f &cameraPos, const Vector3f &sunPos);

#define MIN_TERRAIN_TEXTURE_SIZE 512

/// Pass the accumulated batch to the graphics backend
template<typename PSO>
static void flushDrawElementsBatch()
{
	if (drawElementsBatchActive && batchedIndexCount > 0)
	{
		PSO::get().draw_elements(batchedIndexCount, sizeof(GLuint)*batchedIndexOffset);
	}
	drawElementsBatchActive = false;
}

/**
 * Merge contiguous index ranges into a single draw call.
 * This improves performance by reducing the number of draw calls.
 */
template<typename PSO>
static void batchDrawElements(GLsizei count, GLuint offset)
{
	if (drawElementsBatchActive && batchedIndexOffset + batchedIndexCount == offset)
	{
		// contiguous with the current batch - append
		batchedIndexCount += count;
		return;
	}
	flushDrawElementsBatch<PSO>();
	batchedIndexOffset = offset;
	batchedIndexCount = count;
	drawElementsBatchActive = true;
}


static void flipRotateTexCoords(unsigned short texture, Vector2f &sP1, Vector2f &sP2, Vector2f &sP3, Vector2f &sP4);

// NOTE:  The current (max) texture size of a tile is 128x128.  We allow up to a user defined texture size
// of 2048.  This will cause ugly seams for the decals, if user picks a texture size bigger than the tile!

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
static bool isWater(WorldMapState& mapState, int x, int y)
{
	bool result = false;
	result = result || (tileOnMap(mapState, x  , y) && terrainType(mapTile(mapState, x  , y)) == TER_WATER);
	result = result || (tileOnMap(mapState, x - 1, y) && terrainType(mapTile(mapState, x - 1, y)) == TER_WATER);
	result = result || (tileOnMap(mapState, x  , y - 1) && terrainType(mapTile(mapState, x  , y - 1)) == TER_WATER);
	result = result || (tileOnMap(mapState, x - 1, y - 1) && terrainType(mapTile(mapState, x - 1, y - 1)) == TER_WATER);
	return result;
}

/// Is this position an *actual* water-only tile?
static bool isOnlyWater(WorldMapState& mapState, int x, int y)
{
	bool result = true;
	result = result && (tileOnMap(mapState, x  , y) && terrainType(mapTile(mapState, x  , y)) == TER_WATER);
	result = result && (tileOnMap(mapState, x - 1, y) && terrainType(mapTile(mapState, x - 1, y)) == TER_WATER);
	result = result && (tileOnMap(mapState, x  , y - 1) && terrainType(mapTile(mapState, x  , y - 1)) == TER_WATER);
	result = result && (tileOnMap(mapState, x - 1, y - 1) && terrainType(mapTile(mapState, x - 1, y - 1)) == TER_WATER);
	return result;
}

/// Get the position of a grid point
static void getGridPos(const WorldMapState& mapState, Vector3i *result, int x, int y, bool center, bool water)
{
	if (center)
	{
		Vector3i a, b, c, d;
		getGridPos(mapState, &a, x  , y  , false, water);
		getGridPos(mapState, &b, x + 1, y  , false, water);
		getGridPos(mapState, &c, x  , y + 1, false, water);
		getGridPos(mapState, &d, x + 1, y + 1, false, water);
		averagePos(result, &a, &b, &c, &d);
		return;
	}
	// NOTE: z is NEGATED - map space and world space differ by a z flip. Anything
	// comparing a direction or a winding across the two has to account for it:
	// theSun_ForTileIllumination (lighting.cpp) is the shaders' light direction with
	// z flipped for exactly this reason, and a tile's triangles are wound
	// consistently with their +Y normals only once the negation is applied, which
	// MikkTSpace relies on in calcDecalTangents below.
	result->x = world_coord(x);
	result->z = world_coord(-y);

	if (x <= 0 || y <= 0 || x >= mapState.width || y >= mapState.height)
	{
		result->y = 0;
	}
	else
	{
		if (terrainShaderQuality != TerrainShaderQuality::CLASSIC)
		{
			result->y = map_TileHeight(mapState, x, y);
			if (water)
			{
				result->y = map_WaterHeight(mapState, x, y);
			}
		}
		else
		{
			result->y = map_TileHeightSurface(mapState, x, y);
		}
	}
}

static Vector3f getGridPosf(const WorldMapState& mapState, int x, int y, bool center = false, bool water = false)
{
	Vector3i r;
	getGridPos(mapState, &r, x, y, center, water);
	return Vector3f(r);
}

/// Get normal vector of grid point
static Vector3f getGridNormal(const WorldMapState& mapState, int x, int y, bool center = false)
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
		// avg nearest normals provide better results
		return (getGridNormal(mapState, x, y) + getGridNormal(mapState, x+1, y) + getGridNormal(mapState, x+1, y+1) + getGridNormal(mapState, x, y+1)) / 4.f;
	} else {
		return calcNormal(getGridPosf(mapState, x, y), {
			getGridPosf(mapState, x+1, y), getGridPosf(mapState, x, y, true),     getGridPosf(mapState, x, y+1), getGridPosf(mapState, x-1, y, true),
			getGridPosf(mapState, x-1, y), getGridPosf(mapState, x-1, y-1, true), getGridPosf(mapState, x, y-1), getGridPosf(mapState, x, y-1, true)
		});
	}
}

/// Emit the 2 triangles of one subdivided sub-quad. qA is the vertex at the
/// sub-quad's (0,0) corner. gridStride is the index distance of one step in the
/// grid's x direction (+1 in y is always adjacent).
/// NOTE: must stay in sync with the tangent triangulation in setTileDecalVertex_Subdivided
static inline void emitSubQuadIndices(GLuint *indices, int &indexSize, GLuint qA, GLuint gridStride)
{
	const GLuint qB = qA + gridStride;     // (+1, 0)
	const GLuint qC = qA + 1;              // (0, +1)
	const GLuint qD = qA + gridStride + 1; // (+1, +1)
	indices[indexSize + 0] = qA;
	indices[indexSize + 1] = qB;
	indices[indexSize + 2] = qD;
	indices[indexSize + 3] = qA;
	indices[indexSize + 4] = qD;
	indices[indexSize + 5] = qC;
	indexSize += 6;
}

/// The height mode matching the ground heights getGridPos() produces for the current quality
static terrainSurface::HeightMode groundHeightMode()
{
	return (terrainShaderQuality != TerrainShaderQuality::CLASSIC)
		? terrainSurface::HeightMode::Ground
		: terrainSurface::HeightMode::Surface;
}

/**
 * Set the terrain and water geometry for the specified sector
 */
static void setSectorGeometry(const WorldMapState& mapState, int sx, int sy,
							  TerrainVertex *geometry, WaterVertex *water,
							  int *geometrySize, int *waterSize)
{
	if (terrainSubdivision > 1)
	{
		// subdivided: a regular (sectorSize*N+1)^2 grid sampled from the smooth surface,
		// no per-tile center vertices
		const int N = terrainSubdivision;
		const float step = static_cast<float>(TILE_UNITS) / N;
		const auto hMode = groundHeightMode();
		const int gridSize = sectorSize * N;
		for (int gx = 0; gx <= gridSize; gx++)
		{
			for (int gy = 0; gy <= gridSize; gy++)
			{
				// NOTE: these expressions must stay bit-identical with the combined
				// terrain+decal builder so the depth prepass matches the color pass
				const float wx = (sx * sectorSize * N + gx) * step;
				const float wy = (sy * sectorSize * N + gy) * step;
				const float groundY = terrainSurface::heightAt(mapState, wx, wy, hMode);
				const float waterY = terrainSurface::heightAt(mapState, wx, wy, terrainSurface::HeightMode::Water);
				// heights are sampled at the undisplaced position. The horizontal
				// offset warps the drawn field, rounding cliff and shoreline outlines
				const Vector2f off = terrainSurface::outlineOffsetAt(mapState, wx, wy);
				geometry[(*geometrySize)++].pos = Vector3f(wx + off.x, groundY, -(wy + off.y));
				geometry[*geometrySize - 1].normal = terrainSurface::worldNormalAt(mapState, wx, wy, hMode);
				water[(*waterSize)++] = glm::vec4(wx + off.x, (terrainShaderQuality != TerrainShaderQuality::CLASSIC) ? waterY : groundY, -(wy + off.y), waterY - groundY);
			}
		}
		return;
	}

	for (int x = sx*sectorSize; x < (sx+1)*sectorSize + 1; x++)
	{
		for (int y = sy * sectorSize; y < (sy+1) * sectorSize + 1; y++)
		{
			// set up geometry
			auto pos = getGridPosf(mapState, x, y, false, false);
			geometry[*geometrySize].pos = pos;
			geometry[*geometrySize].normal = getGridNormal(mapState, x, y, false);
			(*geometrySize)++;

			float waterHeight = map_WaterHeight(mapState, x, y);
			water[*waterSize] = glm::vec4(pos.x, (terrainShaderQuality != TerrainShaderQuality::CLASSIC) ? waterHeight : pos.y, pos.z, waterHeight - pos.y);
			(*waterSize)++;

			pos = getGridPosf(mapState, x, y, true, false);
			geometry[*geometrySize].pos = pos;
			geometry[*geometrySize].normal = getGridNormal(mapState, x, y, true);
			(*geometrySize)++;

			water[*waterSize] = glm::vec4(pos.x, (terrainShaderQuality != TerrainShaderQuality::CLASSIC) ? waterHeight : pos.y, pos.z, waterHeight - pos.y);
			(*waterSize)++;
		}
	}
}

/// Decal/ground info for one tile, shared by the legacy and subdivided builders
struct TileDecalInfo
{
	Vector2f uv[2][2];
	Vector2f centerUv;
	int decalNo;
	uint32_t grounds;
};

static TileDecalInfo getTileDecalInfo(WorldMapState& mapState, int i, int j)
{
	TileDecalInfo info;
	MAPTILE *tile = mapTile(mapState, i, j);
	info.centerUv = getTileTexArrCoords(*info.uv, tile->texture);
	info.decalNo = static_cast<int>(TileNumber_tile(tile->texture));
	bool skipDecalDraw = !TILE_HAS_DECAL(tile);
	if (terrainShaderQuality == TerrainShaderQuality::CLASSIC)
	{
		// in Classic mode, skip drawing any that are the "water only" decal (water is handled separately as a prior pass)
		skipDecalDraw = skipDecalDraw || (isOnlyWater(mapState, i, j) && info.decalNo == 17); // Magic number hack, but decal # 17 is always the *water only* tile in the legacy terrain tilesets we ship // TODO: Figure out a better way of determining this from the tileset data?
	}
	if (skipDecalDraw)
	{
		info.decalNo = -1;
	}

	uint8_t groundsBytes[4];
	static const int dxdy[4][2] = {{0,0}, {0,1}, {1,1}, {1,0}};
	for (int k = 0; k < 4; k++)
	{
		groundsBytes[k] = mapTile(mapState, i + dxdy[k][0], j + dxdy[k][1])->ground;
	}
	PIELIGHT grounds;
	grounds.fromRGBA(groundsBytes[0], groundsBytes[1], groundsBytes[2], groundsBytes[3]);
	info.grounds = grounds.rgba();
	return info;
}

// MikkTSpace tangent generation for terrain decals
//
// The same reference implementation the models use (lib/ivis_opengl/imdload.cpp),
// and the same one every tool that bakes normal maps uses (Blender, Substance,
// xNormal, Marmoset, etc) so a decal normal map is rendered against the tangent
// frame it was baked against.
//
// V handling matches imdload.cpp: WZ texture coordinates have v running DOWN
// the image, while MikkTSpace assumes the standard convention, so it is handed
// V-flipped coordinates. T is unaffected and B = dP/dv changes sign, which
// reproduces the -dP/dv bitangent that "green up" normal maps need - i.e. exactly
// the meaning vertexTangent.w already has here.

namespace {

struct MikkDecalMesh
{
	gfx_api::TerrainDecalVertex *vs;
	const int (*tris)[3];
	int numTris;
};

inline int mikkDecalIndex(const MikkDecalMesh *m, int iFace, int iVert)
{
	return m->tris[iFace][iVert];
}

int mikkDecalGetNumFaces(const SMikkTSpaceContext *pContext)
{
	return static_cast<const MikkDecalMesh *>(pContext->m_pUserData)->numTris;
}

int mikkDecalGetNumVerticesOfFace(const SMikkTSpaceContext *, const int)
{
	return 3;
}

void mikkDecalGetPosition(const SMikkTSpaceContext *pContext, float fvPosOut[], const int iFace, const int iVert)
{
	const auto *m = static_cast<const MikkDecalMesh *>(pContext->m_pUserData);
	const Vector3f &p = m->vs[mikkDecalIndex(m, iFace, iVert)].pos;
	fvPosOut[0] = p.x; fvPosOut[1] = p.y; fvPosOut[2] = p.z;
}

void mikkDecalGetNormal(const SMikkTSpaceContext *pContext, float fvNormOut[], const int iFace, const int iVert)
{
	const auto *m = static_cast<const MikkDecalMesh *>(pContext->m_pUserData);
	const Vector3f &n = m->vs[mikkDecalIndex(m, iFace, iVert)].normal;
	fvNormOut[0] = n.x; fvNormOut[1] = n.y; fvNormOut[2] = n.z;
}

void mikkDecalGetTexCoord(const SMikkTSpaceContext *pContext, float fvTexcOut[], const int iFace, const int iVert)
{
	const auto *m = static_cast<const MikkDecalMesh *>(pContext->m_pUserData);
	const Vector2f &uv = m->vs[mikkDecalIndex(m, iFace, iVert)].decalUv;
	fvTexcOut[0] = uv.x;
	fvTexcOut[1] = 1.f - uv.y; // V flip - see the note above
}

void mikkDecalSetTSpaceBasic(const SMikkTSpaceContext *pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
{
	auto *m = static_cast<MikkDecalMesh *>(pContext->m_pUserData);
	m->vs[mikkDecalIndex(m, iFace, iVert)].decalTangent = glm::vec4(fvTangent[0], fvTangent[1], fvTangent[2], fSign);
}

} // anonymous namespace

/// Calc decal tangents for one tile's vertices, over the same triangulation as the index buffer.
///
/// The handedness w is stored so that
///     bitangent = cross(normal, tangent) * w
/// reproduces -dP/dv, the bitangent "green up" normal maps need where v runs down the image.
/// That is the meaning w has for models too, so the one expression above is correct everywhere in the engine.
///
/// NOTE: MikkTSpace returns its results unindexed and warns against writing them
/// through an index list that merges vertices. That is safe here because these
/// indices only ever refer back to the *same* vertex - a tile's shared corner and
/// centre vertices carry one position, one decal UV and one normal each - which is
/// the criterion MikkTSpace welds on internally, so every corner sharing an index
/// receives the same tangent anyway.
static void calcDecalTangents(gfx_api::TerrainDecalVertex *vs, int numVerts, const int (*tris)[3], int numTris)
{
	constexpr int maxVerts = (MAX_TERRAIN_MESH_SUBDIVISION + 1) * (MAX_TERRAIN_MESH_SUBDIVISION + 1);
	ASSERT_OR_RETURN(, numVerts <= maxVerts, "too many vertices (%d)", numVerts);

	MikkDecalMesh mesh { vs, tris, numTris };

	SMikkTSpaceInterface mikkInterface = {};
	mikkInterface.m_getNumFaces = mikkDecalGetNumFaces;
	mikkInterface.m_getNumVerticesOfFace = mikkDecalGetNumVerticesOfFace;
	mikkInterface.m_getPosition = mikkDecalGetPosition;
	mikkInterface.m_getNormal = mikkDecalGetNormal;
	mikkInterface.m_getTexCoord = mikkDecalGetTexCoord;
	mikkInterface.m_setTSpaceBasic = mikkDecalSetTSpaceBasic;

	SMikkTSpaceContext mikkContext = {};
	mikkContext.m_pInterface = &mikkInterface;
	mikkContext.m_pUserData = &mesh;

	if (!genTangSpaceDefault(&mikkContext))
	{
		// leave the defaults in place rather than half-written tangents
		debug(LOG_ERROR, "MikkTSpace tangent generation failed for a terrain decal tile");
	}
}

/// Subdivided (N > 1) terrain+decal vertices for one tile: (N+1)^2 vertices on a
/// regular grid sampled from the smooth surface. Triangles come from the index buffer.
static void setTileDecalVertex_Subdivided(WorldMapState& mapState, int i, int j, gfx_api::TerrainDecalVertex *terrainDecalData, int *terrainDecalSize)
{
	const int N = combinedMeshSubdivision;
	const float step = static_cast<float>(TILE_UNITS) / N;
	const auto hMode = groundHeightMode();
	const TileDecalInfo info = getTileDecalInfo(mapState, i, j);

	gfx_api::TerrainDecalVertex vs[(MAX_TERRAIN_MESH_SUBDIVISION + 1) * (MAX_TERRAIN_MESH_SUBDIVISION + 1)];
	for (int a = 0; a <= N; a++)
	{
		for (int b = 0; b <= N; b++)
		{
			auto &v = vs[a * (N + 1) + b];
			const float tx = static_cast<float>(a) / N;
			const float ty = static_cast<float>(b) / N;
			// heights come from the undisplaced grid (this expression must stay
			// bit-identical with setSectorGeometry so the depth prepass matches
			// the color pass). The horizontal offset rounds cliff and shoreline outlines.
			const float wx = (i * N + a) * step;
			const float wy = (j * N + b) * step;
			const Vector2f off = terrainSurface::outlineOffsetAt(mapState, wx, wy);
			v.pos = Vector3f(wx + off.x, terrainSurface::heightAt(mapState, wx, wy, hMode), -(wy + off.y));
			// normals are sampled over a fixed world-space radius (independent of
			// the mesh density) so lighting stays consistent across detail levels
			v.normal = terrainSurface::worldNormalAt(mapState, wx, wy, hMode);
			// bilinear decal UV between the tile's (flip/rotated) corner UVs
			const Vector2f uvB = info.uv[0][0] + (info.uv[1][0] - info.uv[0][0]) * tx;
			const Vector2f uvT = info.uv[0][1] + (info.uv[1][1] - info.uv[0][1]) * tx;
			v.decalUv = uvB + (uvT - uvB) * ty;
			v.decalNo = info.decalNo;
			v.grounds = info.grounds;
			// bilinear ground splat weights between the one-hot corner weights
			// (corner order k: {0,0}, {0,1}, {1,1}, {1,0} - see getTileDecalInfo)
			const float wk[4] = {(1.f - tx) * (1.f - ty), (1.f - tx) * ty, tx * ty, tx * (1.f - ty)};
			int wb[4];
			int wSum = 0, wMax = 0;
			for (int k = 0; k < 4; k++)
			{
				wb[k] = static_cast<int>(wk[k] * 255.f + 0.5f);
				wSum += wb[k];
				if (wb[k] > wb[wMax]) { wMax = k; }
			}
			wb[wMax] += 255 - wSum; // keep the weights summing to exactly 1
			v.groundWeights = {static_cast<uint8_t>(wb[0]), static_cast<uint8_t>(wb[1]), static_cast<uint8_t>(wb[2]), static_cast<uint8_t>(wb[3])};
		}
	}

	// calc tangents over the same triangulation as the index buffer
	// (2 triangles per sub-quad, A-D diagonal - see emitSubQuadIndices)
	int tris[2 * MAX_TERRAIN_MESH_SUBDIVISION * MAX_TERRAIN_MESH_SUBDIVISION][3];
	int numTris = 0;
	for (int a = 0; a < N; a++)
	{
		for (int b = 0; b < N; b++)
		{
			const int qA = a * (N + 1) + b;
			const int qB = qA + (N + 1);
			const int qC = qA + 1;
			const int qD = qA + (N + 1) + 1;
			tris[numTris][0] = qA; tris[numTris][1] = qB; tris[numTris][2] = qD;
			numTris++;
			tris[numTris][0] = qA; tris[numTris][1] = qD; tris[numTris][2] = qC;
			numTris++;
		}
	}
	calcDecalTangents(vs, (N + 1) * (N + 1), tris, numTris);

	for (int k = 0; k < (N + 1) * (N + 1); k++)
	{
		terrainDecalData[(*terrainDecalSize)++] = vs[k];
	}
}

static void setSectorDecalVertex_SinglePass(WorldMapState& mapState, int x, int y, gfx_api::TerrainDecalVertex *terrainDecalData, int *terrainDecalSize)
{
	Vector3i pos;
	int i, j;

	for (i = x * sectorSize; i < x * sectorSize + sectorSize; i++)
	{
		for (j = y * sectorSize; j < y * sectorSize + sectorSize; j++)
		{
			if (i < 0 || j < 0 || i >= mapState.width || j >= mapState.height)
			{
				continue;
			}

			if (combinedMeshSubdivision > 1)
			{
				setTileDecalVertex_Subdivided(mapState, i, j, terrainDecalData, terrainDecalSize);
				continue;
			}

			const TileDecalInfo info = getTileDecalInfo(mapState, i, j);

			static const int dxdy[4][2] = {{0,0}, {0,1}, {1,1}, {1,0}};
			gfx_api::TerrainDecalVertex vs[5];
			for (int k = 0; k<4; k++) {
				int dx = dxdy[k][0], dy = dxdy[k][1];
				getGridPos(mapState, &pos, i + dx, j + dy, false, false);
				vs[k].pos = pos;
				vs[k].decalUv = info.uv[dx][dy];
				vs[k].normal = getGridNormal(mapState, i + dx, j + dy);
				vs[k].decalNo = info.decalNo;
				vs[k].groundWeights.clear();
				vs[k].groundWeights.setByte(k, 255);
			}
			// 4 = center;
			getGridPos(mapState, &pos, i, j, true, false);
			vs[4].pos = pos;
			vs[4].decalUv = info.centerUv;
			vs[4].normal = getGridNormal(mapState, i, j, true);
			vs[4].decalNo = info.decalNo;
			vs[4].groundWeights = {0, 0, 0, 0}; // special value for shader.
			for (int k = 0; k < 5; k++) vs[k].grounds = info.grounds;

			// vertices are shared between the tile's 4 triangles (fanned from the center vertex vs[4])
			static const int tileTris[4][3] = {{4, 1, 0}, {4, 2, 1}, {4, 3, 2}, {4, 0, 3}};
			calcDecalTangents(vs, 5, tileTris, 4);

			// emit the 5 unique vertices (4 corners + center). Triangles are formed by the index buffer.
			for (int k = 0; k < 5; k++) {
				terrainDecalData[(*terrainDecalSize)++] = vs[k];
			}
		}
	}
}

/**
 * Update the sector for when the terrain is changed.
 */
static void updateSectorGeometry(WorldMapState& mapState, int x, int y)
{
	TerrainVertex *geometry;
	WaterVertex *water;
	int geometrySize = 0;
	int waterSize = 0;

	if (terrainSubdivision > 1)
	{
		// refresh the per-corner surface caches before re-evaluating the surface:
		// a corner's cached values depend on its +-1 neighbors, so expand the sector's corner
		// rect by 1 (markTileDirty's widening guarantees every affected sector gets here)
		terrainSurface::rebuildSurfaceCachesRegion(mapState, x * sectorSize - 1, y * sectorSize - 1,
													(x + 1) * sectorSize + 1, (y + 1) * sectorSize + 1);
	}

	geometry = new TerrainVertex[sectors[x * ySectors + y].geometrySize];
	water = (WaterVertex *)malloc(sizeof(WaterVertex) * sectors[x * ySectors + y].waterSize);

	setSectorGeometry(mapState, x, y, geometry, water, &geometrySize, &waterSize);
	ASSERT(geometrySize == sectors[x * ySectors + y].geometrySize, "something went seriously wrong updating the terrain");
	ASSERT(waterSize    == sectors[x * ySectors + y].waterSize   , "something went seriously wrong updating the terrain");

	if (geometryVBO) // absent under HardwareTess (the shadow pass draws tessellated patches and the color pass writes depth)
	{
		geometryVBO->update(sizeof(TerrainVertex)*sectors[x * ySectors + y].geometryOffset,
								sizeof(TerrainVertex)*sectors[x * ySectors + y].geometrySize, geometry,
								gfx_api::buffer::update_flag::non_overlapping_updates_promise);
	}
	waterVBO->update(sizeof(WaterVertex)*sectors[x * ySectors + y].waterOffset,
					 sizeof(WaterVertex)*sectors[x * ySectors + y].waterSize, water,
					 gfx_api::buffer::update_flag::non_overlapping_updates_promise);

	delete[] geometry;
	free(water);

	if (terrainMeshStrategy == TerrainMeshStrategy::HardwareTess)
	{
		// the tessellated surface comes from the baked field textures
		terrainBake::rebakeTileRegion(mapState, x * sectorSize, y * sectorSize,
									  x * sectorSize + sectorSize - 1, y * sectorSize + sectorSize - 1);
	}

	terrainDecalVertexUpdateBuffer.resize(sectors[x * ySectors + y].terrainAndDecalSize); // reuse a buffer to avoid repeated allocations if possible
	int terrainDecalSize = 0;
	setSectorDecalVertex_SinglePass(mapState, x, y, terrainDecalVertexUpdateBuffer.data(), &terrainDecalSize);
	ASSERT(terrainDecalSize == sectors[x * ySectors + y].terrainAndDecalSize, "Sizes don't match!");
	terrainDecalVBO->update(sizeof(gfx_api::TerrainDecalVertex)*sectors[x * ySectors + y].terrainAndDecalOffset,
						 sizeof(gfx_api::TerrainDecalVertex)*sectors[x * ySectors + y].terrainAndDecalSize, terrainDecalVertexUpdateBuffer.data(),
						 gfx_api::buffer::update_flag::non_overlapping_updates_promise);
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

	if (terrainSubdivision > 1)
	{
		// the smooth surface's bicubic stencil (plus the sharpness factor) extends a
		// height change's influence to tiles [i-2, i+1] x [j-2, j+1]. Dirty their sectors.
		for (int tj = j - 2; tj <= j + 1; tj++)
		{
			for (int ti = i - 2; ti <= i + 1; ti++)
			{
				if (ti < 0 || tj < 0)
				{
					continue;
				}
				const int sx = ti / sectorSize;
				const int sy = tj / sectorSize;
				if (sx < xSectors && sy < ySectors)
				{
					sectors[sx * ySectors + sy].dirty = true;
				}
			}
		}
		return;
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

// Mark all tiles dirty
// (when switching between classic and other modes, recalculating tile heights is required due to water)
void dirtyAllSectors()
{
	if (sectors)
	{
		size_t len = static_cast<size_t>(xSectors) * static_cast<size_t>(ySectors);
		for (size_t i = 0; i < len; ++i)
		{
			sectors[i].dirty = true;
		}
	}
}

static TerrainShaderQuality determineDefaultTerrainQuality();

/// The subdivision factor the next terrain (re)build will actually use
static int effectiveTerrainMeshSubdivision()
{
	int factor = (terrainSubdivisionSetting > 0) ? terrainSubdivisionSetting : 1;
	if (terrainShaderQuality == TerrainShaderQuality::CLASSIC)
	{
		factor = 1; // preserve the classic low-poly appearance
	}
	return factor;
}

/// The mesh strategy the next terrain (re)build will actually use
static TerrainMeshStrategy resolveTerrainMeshStrategy(int subdivision)
{
	if (terrainTessellationPreference != TerrainTessellationPreference::ForceHardware)
	{
		// Auto currently resolves to the CPU mesh
		return TerrainMeshStrategy::CPU;
	}
	if (gfx_api::context::get().supportsTessellationShaders()
		&& terrainShaderQuality != TerrainShaderQuality::CLASSIC
		&& subdivision > 1)
	{
		return TerrainMeshStrategy::HardwareTess;
	}
	debug(LOG_INFO, "terrain: terrainTessellation=hw requested but unavailable here - using the CPU mesh");
	return TerrainMeshStrategy::CPU;
}

bool setTerrainTessellationPreference(TerrainTessellationPreference pref)
{
	terrainTessellationPreference = pref;
	// apply live when in-game: the strategies use different buffers/pipelines, so a change needs a full terrain re-init
	if (terrainInitialised && resolveTerrainMeshStrategy(effectiveTerrainMeshSubdivision()) != terrainMeshStrategy)
	{
		if (!initTerrain(gameWorld.map))
		{
			debug(LOG_ERROR, "Failed to re-initialize terrain for tessellation preference change");
			return false;
		}
	}
	return true;
}

TerrainTessellationPreference getTerrainTessellationPreference()
{
	return terrainTessellationPreference;
}

bool setTerrainMeshSubdivision(int factor)
{
	if (factor < 1 || factor > MAX_TERRAIN_MESH_SUBDIVISION)
	{
		debug(LOG_ERROR, "Attempted to set bad terrain mesh subdivision factor %d! Ignored.", factor);
		return false;
	}
	terrainSubdivisionSetting = factor;
	// apply live when in-game: the built mesh depends on the factor (buffer
	// sizes on the CPU strategy, the max tessellation level under hardware
	// tessellation), so this needs a full terrain re-init, not just dirty
	// sectors
	if (terrainInitialised && effectiveTerrainMeshSubdivision() != terrainSubdivision)
	{
		if (!initTerrain(gameWorld.map))
		{
			debug(LOG_ERROR, "Failed to re-initialize terrain for mesh subdivision change");
			return false;
		}
	}
	return true;
}

int getTerrainMeshSubdivision()
{
	return terrainSubdivisionSetting;
}

static int determineDefaultTerrainMeshSubdivision()
{
	// Subdivision multiplies terrain vertex data and map load time, so be
	// conservative on the systems the terrain quality heuristic already flags
	// as lower-spec (RAM / VRAM / renderer / 32-bit checks).
#if defined(__EMSCRIPTEN__)
	return 2;
#else
	if (determineDefaultTerrainQuality() != TerrainShaderQuality::NORMAL_MAPPING)
	{
		return 1;
	}
	return 3;
#endif
}

float getTerrainVisualObjectHeightDelta(WorldMapState& mapState, int x, int y, int z)
{
	if (!terrainInitialised || terrainSubdivision <= 1)
	{
		return 0.f;
	}
	// off-map render positions (e.g. transporters flying in from off-map) have
	// no drawn terrain to settle onto (and map_Height asserts on them)
	if (x < 0 || y < 0 || x >= world_coord(mapState.width) || y >= world_coord(mapState.height))
	{
		return 0.f;
	}
	// the gameplay standing surface (map_Height) is max(ground, water)
	const int groundZ = map_Height(mapState, x, y);
	const float aboveGround = static_cast<float>(z - groundZ);
	if (aboveGround >= 16.f)
	{
		return 0.f; // airborne - the ground correction doesn't apply
	}
	// Settle onto the smooth analogue of the sim's standing surface: map_Height()
	// fans the per-corner max(ground, water) lattice (map_TileHeightSurface), so
	// smoothing that same lattice gives a drawn standing surface that is
	// continuous everywhere - no water-adjacency gate whose boundary can pop the
	// unit. Over open water every water cell's corners sit at the water level and
	// the monotone bicubic reproduces locally constant lattices exactly, so hover
	// units still sit precisely on the drawn water plane. Across a shoreline cell
	// the surface slopes continuously, so units pitch through the crossing
	// instead of stepping between ground and water levels.
	const float drawn = terrainSurface::drawnHeightAt(mapState, static_cast<float>(x), static_cast<float>(y), terrainSurface::HeightMode::Surface);
	float delta = drawn - static_cast<float>(groundZ);
	if (aboveGround > 0.f)
	{
		delta *= 1.f - aboveGround / 16.f; // fade out as the object leaves the ground
	}
	return delta;
}

bool setTerrainMappingTexturesMaxSize(int texSize)
{
	bool isPowerOfTwo = !(texSize == 0) && !(texSize & (texSize - 1));
	if (!isPowerOfTwo || texSize < MIN_TERRAIN_TEXTURE_SIZE || texSize > 1024)
	{
		if (texSize != 0)
		{
			debug(LOG_ERROR, "Attempted to set bad terrain mapping texture max size %d! Ignored.", texSize);
		}
		return false;
	}
	maxTerrainMappingTextureSize = texSize;
	debug(LOG_TEXTURE, "terrain mapping texture max size set to %d", texSize);
	return true;
}

int getTerrainMappingTexturesMaxSize()
{
	return maxTerrainMappingTextureSize;
}

static gfx_api::texture_array* groundTexArr = nullptr;
static gfx_api::texture_array* groundNormalArr = nullptr;
static gfx_api::texture_array* groundSpecularArr = nullptr;
static gfx_api::texture_array* groundHeightArr = nullptr;

struct WaterTextures_Normal
{
	gfx_api::texture* tex1 = nullptr;
	gfx_api::texture* tex2 = nullptr;

public:
	void clear()
	{
		delete tex1; tex1 = nullptr;
		delete tex2; tex2 = nullptr;
	}
};

struct WaterTextures_High
{
	gfx_api::texture_array* tex = nullptr;
	gfx_api::texture_array* tex_nm = nullptr;
	gfx_api::texture_array* tex_sm = nullptr;

public:
	void clear()
	{
		delete tex; tex = nullptr;
		delete tex_nm; tex_nm = nullptr;
		delete tex_sm; tex_sm = nullptr;
	}
};

static WaterTextures_Normal waterTexturesNormal;
static WaterTextures_High waterTexturesHigh;
static gfx_api::texture* waterClassicTexture = nullptr; // only used for classic mode

gfx_api::texture* getWaterClassicTexture()
{
	if (waterClassicTexture)
	{
		return waterClassicTexture;
	}

	int32_t maxGfxTextureSize = gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_TEXTURE_SIZE);
	int maxTerrainTextureSize = std::max(std::min({getTextureSize(), maxGfxTextureSize}), MIN_TERRAIN_TEXTURE_SIZE);

	std::string legacyWaterDecalPath = std::string(tilesetDir) + "-" + std::to_string(getCurrentTileTextureSize()) + "/tile-17.png"; // TODO: This is currently hard-coded for legacy tileset textures...
	waterClassicTexture = gfx_api::context::get().loadTextureFromFile(legacyWaterDecalPath.c_str(), gfx_api::texture_type::game_texture, maxTerrainTextureSize, maxTerrainTextureSize);
	return waterClassicTexture;
}

static LoadingTask<> loadWaterTexturesTask(ResourceLoadingController& controller, int maxTerrainTextureSize, optional<int> maxTerrainAuxTextureSizeOpt)
{
	waterTexturesNormal.clear();
	waterTexturesHigh.clear();

	if (terrainShaderQuality == TerrainShaderQuality::MEDIUM)
	{
		auto checkTex = [maxTerrainTextureSize](const WzString &fileName, gfx_api::texture_type type) -> gfx_api::texture* {
			WzString fullName = "texpages/" + fileName;
			auto imageLoadFilename = gfx_api::imageLoadFilenameFromInputFilename(fullName);
			if (!PHYSFS_exists(imageLoadFilename))
			{
				return nullptr;
			}
			return gfx_api::context::get().loadTextureFromFile(imageLoadFilename.toUtf8().c_str(), type, maxTerrainTextureSize, maxTerrainTextureSize);
		};

		waterTexturesNormal.tex1 = checkTex("page-80-water-1.png", gfx_api::texture_type::game_texture);
		waterTexturesNormal.tex2 = checkTex("page-81-water-2.png", gfx_api::texture_type::specular_map);
	}
	else if (terrainShaderQuality == TerrainShaderQuality::NORMAL_MAPPING)
	{
		std::vector<WzString> waterTextureFilenames;
		std::vector<WzString> waterTextureFilenames_nm;
		std::vector<WzString> waterTextureFilenames_sm;

		auto optTexturenameToPath = [](const WzString& textureFilename) -> WzString {
			if (textureFilename.isEmpty())
			{
				return WzString();
			}
			WzString fullName = "texpages/" + textureFilename;
			auto imageLoadFilename = gfx_api::imageLoadFilenameFromInputFilename(fullName);
			if (!PHYSFS_exists(imageLoadFilename))
			{
				return WzString();
			}
			return fullName;
		};

		// check water optional textures
		waterTextureFilenames.push_back(optTexturenameToPath("page-80-water-1.png"));
		waterTextureFilenames.push_back(optTexturenameToPath("page-81-water-2.png"));
		waterTextureFilenames_nm.push_back(optTexturenameToPath("page-80-water-1_nm.png"));
		waterTextureFilenames_nm.push_back(optTexturenameToPath("page-81-water-2_nm.png"));
		waterTextureFilenames_sm.push_back(optTexturenameToPath("page-80-water-1_sm.png"));
		waterTextureFilenames_sm.push_back(optTexturenameToPath("page-81-water-2_sm.png"));

		if (!std::all_of(waterTextureFilenames.begin(), waterTextureFilenames.end(), [](const WzString& texturePath) -> bool { return !texturePath.isEmpty(); }))
		{
			debug(LOG_FATAL, "Missing one or more base water textures?");
			co_return load_fail();
		}

		waterTexturesHigh.tex = (co_await gfx_api::context::get().loadTextureArrayFromFiles(controller, waterTextureFilenames, gfx_api::texture_type::game_texture, maxTerrainTextureSize, maxTerrainTextureSize)).value_or(nullptr);
		waterTexturesHigh.tex_nm = nullptr;
		waterTexturesHigh.tex_sm = nullptr;

		int maxTerrainAuxTextureSize = maxTerrainAuxTextureSizeOpt.value_or(maxTerrainTextureSize);

		if (std::any_of(waterTextureFilenames_nm.begin(), waterTextureFilenames_nm.end(), [](const WzString& filename) {
			return !filename.isEmpty();
		}))
		{
			waterTexturesHigh.tex_nm = (co_await gfx_api::context::get().loadTextureArrayFromFiles(controller, waterTextureFilenames_nm, gfx_api::texture_type::normal_map, maxTerrainAuxTextureSize, maxTerrainAuxTextureSize, [](int width, int height, int channels) -> std::unique_ptr<iV_Image> {
				std::unique_ptr<iV_Image> pDefaultNormalMap = std::make_unique<iV_Image>();
				pDefaultNormalMap->allocate(width, height, channels, true);
				// default normal map: (0,0,1)
				unsigned char* pBmpWrite = pDefaultNormalMap->bmp_w();
				memset(pBmpWrite, 0x7f, pDefaultNormalMap->data_size());
				if (channels >= 3)
				{
					size_t pixelIncrement = static_cast<size_t>(channels);
					for (size_t b = 0; b < pDefaultNormalMap->data_size(); b += pixelIncrement)
					{
						pBmpWrite[b+2] = 0xff; // blue=z
					}
				}
				return pDefaultNormalMap;
			})).value_or(nullptr);
			ASSERT(waterTexturesHigh.tex_nm != nullptr, "Failed to load water normals");
		}
		if (std::any_of(waterTextureFilenames_sm.begin(), waterTextureFilenames_sm.end(), [](const WzString& filename) {
			return !filename.isEmpty();
		}))
		{
			waterTexturesHigh.tex_sm = (co_await gfx_api::context::get().loadTextureArrayFromFiles(controller, waterTextureFilenames_sm, gfx_api::texture_type::specular_map, maxTerrainAuxTextureSize, maxTerrainAuxTextureSize, [](int width, int height, int channels) -> std::unique_ptr<iV_Image> {
				std::unique_ptr<iV_Image> pDefaultSpecularMap = std::make_unique<iV_Image>();
				// default specular map: 0
				pDefaultSpecularMap->allocate(width, height, channels, true);
				return pDefaultSpecularMap;
			})).value_or(nullptr);
			ASSERT(waterTexturesHigh.tex_sm != nullptr, "Failed to load water specular maps");
		}
	}
	else if (terrainShaderQuality == TerrainShaderQuality::CLASSIC)
	{
		// preload classic water texture
		auto pWaterTexClassic = getWaterClassicTexture();
		CORO_ASSERT_OR_RETURN(load_fail(), pWaterTexClassic != nullptr, "Failed to load classic water texture?");
	}
	else
	{
		CORO_ASSERT_OR_RETURN(load_fail(), false, "Unexpected terrainShaderQuality: %u", static_cast<unsigned>(terrainShaderQuality));
	}

	co_return load_ok();
}

static LoadingTask<> loadTerrainTexturesImpl(ResourceLoadingController& controller, MAP_TILESET mapTileset)
{
	(void)mapTileset;
	CORO_ASSERT_OR_RETURN(load_fail(), getNumGroundTypes(), "Ground type was not set, no textures will be seen.");

	int32_t maxGfxTextureSize = gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_TEXTURE_SIZE);
	int maxTerrainTextureSize = std::max(std::min({getTextureSize(), maxGfxTextureSize}), MIN_TERRAIN_TEXTURE_SIZE);

	delete groundTexArr; groundTexArr = nullptr;
	delete groundNormalArr; groundNormalArr = nullptr;
	delete groundSpecularArr; groundSpecularArr = nullptr;
	delete groundHeightArr; groundHeightArr = nullptr;
	delete waterClassicTexture; waterClassicTexture = nullptr;

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
	groundTexArr = (co_await gfx_api::context::get().loadTextureArrayFromFiles(controller, groundTextureFilenames, gfx_api::texture_type::game_texture, maxTerrainTextureSize, maxTerrainTextureSize)).value_or(nullptr);
	ASSERT(groundTexArr != nullptr, "Failed to load terrain textures");

	int maxTerrainAuxTextureSize = std::max(std::min({getTextureSize(), getTerrainMappingTexturesMaxSize(), maxGfxTextureSize}), MIN_TERRAIN_TEXTURE_SIZE);

	if (terrainShaderQuality == TerrainShaderQuality::NORMAL_MAPPING)
	{
		if (std::any_of(groundTextureFilenames_nm.begin(), groundTextureFilenames_nm.end(), [](const WzString& filename) {
			return !filename.isEmpty();
		}))
		{
			groundNormalArr = (co_await gfx_api::context::get().loadTextureArrayFromFiles(controller, groundTextureFilenames_nm, gfx_api::texture_type::normal_map, maxTerrainAuxTextureSize, maxTerrainAuxTextureSize, [](int width, int height, int channels) -> std::unique_ptr<iV_Image> {
				std::unique_ptr<iV_Image> pDefaultNormalMap = std::make_unique<iV_Image>();
				pDefaultNormalMap->allocate(width, height, channels, true);
				// default normal map: (0,0,1)
				unsigned char* pBmpWrite = pDefaultNormalMap->bmp_w();
				memset(pBmpWrite, 0x7f, pDefaultNormalMap->data_size());
				if (channels >= 3)
				{
					size_t pixelIncrement = static_cast<size_t>(channels);
					for (size_t b = 0; b < pDefaultNormalMap->data_size(); b += pixelIncrement)
					{
						pBmpWrite[b+2] = 0xff; // blue=z
					}
				}
				return pDefaultNormalMap;
			})).value_or(nullptr);
			ASSERT(groundNormalArr != nullptr, "Failed to load terrain normals");
		}
		if (std::any_of(groundTextureFilenames_spec.begin(), groundTextureFilenames_spec.end(), [](const WzString& filename) {
			return !filename.isEmpty();
		}))
		{
			groundSpecularArr = (co_await gfx_api::context::get().loadTextureArrayFromFiles(controller, groundTextureFilenames_spec, gfx_api::texture_type::specular_map, maxTerrainAuxTextureSize, maxTerrainAuxTextureSize, [](int width, int height, int channels) -> std::unique_ptr<iV_Image> {
				std::unique_ptr<iV_Image> pDefaultSpecularMap = std::make_unique<iV_Image>();
				// default specular map: 0
				pDefaultSpecularMap->allocate(width, height, channels, true);
				return pDefaultSpecularMap;
			})).value_or(nullptr);
			ASSERT(groundSpecularArr != nullptr, "Failed to load terrain specular maps");
		}
		if (std::any_of(groundTextureFilenames_height.begin(), groundTextureFilenames_height.end(), [](const WzString& filename) {
			return !filename.isEmpty();
		}))
		{
			groundHeightArr = (co_await gfx_api::context::get().loadTextureArrayFromFiles(controller, groundTextureFilenames_height, gfx_api::texture_type::height_map, maxTerrainAuxTextureSize, maxTerrainAuxTextureSize, [](int width, int height, int channels) -> std::unique_ptr<iV_Image> {
				std::unique_ptr<iV_Image> pDefaultHeightMap = std::make_unique<iV_Image>();
				// default height map: 0
				pDefaultHeightMap->allocate(width, height, channels, true);
				return pDefaultHeightMap;
			})).value_or(nullptr);
			ASSERT(groundHeightArr != nullptr, "Failed to load terrain height maps");
		}
	}

	// load water textures
	co_return co_await loadWaterTexturesTask(controller, maxTerrainTextureSize, maxTerrainAuxTextureSize);
}

LoadingTask<> loadTerrainTextures(ResourceLoadingController& controller, MAP_TILESET mapTileset)
{
	if (getNumGroundTypes() == 0)
	{
		co_return load_fail();
	}
	co_return co_await loadTerrainTexturesImpl(controller, mapTileset);
}

void loadTerrainTexturesBlocking(MAP_TILESET mapTileset)
{
	if (getNumGroundTypes() == 0)
	{
		return;
	}
	auto& loadingController = ResourceLoadingController::instance();
	ResourceLoadingController::FramePolicy policy;
	policy.showLoadingScreen = false;
	(void)runBlockingResourceLoad(loadTerrainTextures(loadingController, mapTileset), policy);
}

void reloadTerrainTextures()
{
	if (getNumGroundTypes() == 0)
	{
		return; // nothing loaded yet
	}

	loadTerrainTexturesBlocking(currentMapTileset);
}

/**
 * Check what the videocard + drivers support and divide the loaded map into sectors that can be drawn.
 * It also determines the lightmap size.
 */
bool initTerrain(WorldMapState& mapState)
{
	const uint32_t initStartTime = wzGetTicks();
	int i, j, x, y;

	TerrainVertex *geometry = nullptr;
	WaterVertex *water = nullptr;
	int geometrySize, geometryIndexSize;
	int waterSize, waterIndexSize;
	GLuint *geometryIndex = nullptr;
	GLuint *waterIndex = nullptr;

	// apply the effective mesh subdivision factor (setting + Classic policy)
	terrainSubdivision = effectiveTerrainMeshSubdivision();
	debug(LOG_TERRAIN, "terrain mesh subdivision factor: %d", terrainSubdivision);
	debug(LOG_TERRAIN, "sector size: %i", sectorSize);
	const int N = terrainSubdivision;

	// resolve the mesh strategy ("terrainTessellation" config key)
	terrainMeshStrategy = resolveTerrainMeshStrategy(terrainSubdivision);

	// the smooth surface reads the per-corner sharpness and sanitized water
	// lattices constantly - cache them before the dense builds/bakes below
	// (and the per-frame unit settle) evaluate
	if (terrainSubdivision > 1)
	{
		terrainSurface::rebuildSurfaceCaches(mapState);
	}
	else
	{
		terrainSurface::clearSurfaceCaches();
	}

	// bake the surface fields before any strategy-dependent buffer choices:
	// a bake failure falls back to the CPU mesh, which needs the buffers the
	// hardware strategy skips
	if (terrainMeshStrategy == TerrainMeshStrategy::HardwareTess && !terrainBake::bakeFields(mapState))
	{
		debug(LOG_ERROR, "terrain: field bake failed - falling back to the CPU mesh strategy");
		terrainMeshStrategy = TerrainMeshStrategy::CPU;
	}
	if (terrainMeshStrategy == TerrainMeshStrategy::HardwareTess)
	{
		debug(LOG_INFO, "terrain: hardware tessellation strategy active");
	}
	else
	{
		terrainBake::shutdown();
	}
	// under HardwareTess the combined mesh stays at the tile corners (the
	// tessellator generates the dense mesh). Geometry/water keep the full
	// subdivision (water rendering stays on the CPU path).
	combinedMeshSubdivision = (terrainMeshStrategy == TerrainMeshStrategy::HardwareTess) ? 1 : terrainSubdivision;

	// +4 = +1 for iHypot rounding, +1 for sector size rounding, +2 for edge of visibility
	terrainDistance = iHypot(visibleTiles.x / 2, visibleTiles.y / 2) + 4 + sectorSize / 2;
	debug(LOG_TERRAIN, "visible tiles x:%i y: %i", visibleTiles.x, visibleTiles.y);
	debug(LOG_TERRAIN, "terrain view distance: %i", terrainDistance);

	/////////////////////
	// Create the sectors
	xSectors = (mapState.width + sectorSize - 1) / sectorSize;
	ySectors = (mapState.height + sectorSize - 1) / sectorSize;
	sectors = std::unique_ptr<Sector[]> (new Sector[xSectors * ySectors]());

	////////////////////
	// fill the geometry part of the sectors
	const int vertSize = (N == 1)
		? xSectors * ySectors * (sectorSize + 1) * (sectorSize + 1) * 2
		: xSectors * ySectors * (sectorSize * N + 1) * (sectorSize * N + 1);
	const int indexAllocPerSector = sectorSize * sectorSize * ((N == 1) ? 12 : 6 * N * N);
	geometry = new TerrainVertex[vertSize];
	geometryIndex = (GLuint *)malloc(sizeof(GLuint) * xSectors * ySectors * indexAllocPerSector);
	geometrySize = 0;
	geometryIndexSize = 0;

	water = (WaterVertex *)malloc(sizeof(WaterVertex) * vertSize);
	waterIndex = (GLuint *)malloc(sizeof(GLuint) * xSectors * ySectors * indexAllocPerSector);
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

			setSectorGeometry(mapState, x, y, geometry, water, &geometrySize, &waterSize);

			sectors[x * ySectors + y].geometrySize = geometrySize - sectors[x * ySectors + y].geometryOffset;
			sectors[x * ySectors + y].waterSize = waterSize - sectors[x * ySectors + y].waterOffset;
			// and do the index buffers
			sectors[x * ySectors + y].geometryIndexOffset = geometryIndexSize;
			sectors[x * ySectors + y].geometryIndexSize = 0;
			sectors[x * ySectors + y].waterIndexOffset = waterIndexSize;
			sectors[x * ySectors + y].waterIndexSize = 0;

			if (N > 1)
			{
				// subdivided: 2 triangles per sub-quad on the sector's (sectorSize*N+1)^2 grid
				const int gridStride = sectorSize * N + 1;
				const GLuint sectorBase = (x * ySectors + y) * gridStride * gridStride;
				for (i = 0; i < sectorSize; i++)
				{
					for (j = 0; j < sectorSize; j++)
					{
						if (x * sectorSize + i >= mapState.width || y * sectorSize + j >= mapState.height)
						{
							continue; // off map, so skip
						}
						const bool tileIsWater = isWater(mapState, i + x * sectorSize, j + y * sectorSize);
						for (int a = 0; a < N; a++)
						{
							for (int b = 0; b < N; b++)
							{
								const GLuint qA = sectorBase + (i * N + a) * gridStride + (j * N + b);
								emitSubQuadIndices(geometryIndex, geometryIndexSize, qA, gridStride);
								if (tileIsWater)
								{
									emitSubQuadIndices(waterIndex, waterIndexSize, qA, gridStride);
								}
							}
						}
					}
				}
				sectors[x * ySectors + y].geometryIndexSize = geometryIndexSize - sectors[x * ySectors + y].geometryIndexOffset;
				sectors[x * ySectors + y].waterIndexSize = waterIndexSize - sectors[x * ySectors + y].waterIndexOffset;
				continue;
			}

			for (i = 0; i < sectorSize; i++)
			{
				for (j = 0; j < sectorSize; j++)
				{
					if (x * sectorSize + i >= mapState.width || y * sectorSize + j >= mapState.height)
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
					if (isWater(mapState, i + x * sectorSize, j + y * sectorSize))
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
	geometryVBO = nullptr;
	if (geometryIndexVBO)
		delete geometryIndexVBO;
	geometryIndexVBO = nullptr;
	if (terrainMeshStrategy != TerrainMeshStrategy::HardwareTess)
	{
		// under HardwareTess the shadow pass draws tessellated patches and the
		// color pass writes depth, so the geometry buffers would be dead weight
		// (the vertex grid is still computed above - the water build shares it)
		geometryVBO = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::dynamic_draw, "terrain::geometryVBO");
		geometryVBO->upload(sizeof(TerrainVertex)*geometrySize, geometry);
		geometryIndexVBO = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::index_buffer, gfx_api::context::buffer_storage_hint::static_draw, "terrain::geometryIndexVBO");
		geometryIndexVBO->upload(sizeof(GLuint)*geometryIndexSize, geometryIndex);
	}
	delete[] geometry;
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


	// and finally the decals
	const int decalN = combinedMeshSubdivision;
	const bool buildPatches = (terrainMeshStrategy == TerrainMeshStrategy::HardwareTess);
	const int decalVertsPerTile = (decalN == 1) ? 5 : (decalN + 1) * (decalN + 1);
	const int decalIndicesPerTile = (decalN == 1) ? 12 : 6 * decalN * decalN;
	gfx_api::TerrainDecalVertex *terrainDecalData = (gfx_api::TerrainDecalVertex *)malloc(sizeof(gfx_api::TerrainDecalVertex) * mapState.width * mapState.height * decalVertsPerTile);
	GLuint *terrainDecalIndex = (GLuint *)malloc(sizeof(GLuint) * mapState.width * mapState.height * decalIndicesPerTile);
	GLuint *patchIndex = buildPatches ? (GLuint *)malloc(sizeof(GLuint) * mapState.width * mapState.height * 4) : nullptr;
	int terrainDecalSize = 0;
	int terrainDecalIndexSize = 0;
	int patchIndexSize = 0;

	for (x = 0; x < xSectors; x++)
	{
		for (y = 0; y < ySectors; y++)
		{
				sectors[x * ySectors + y].terrainAndDecalOffset = terrainDecalSize;
				sectors[x * ySectors + y].terrainAndDecalSize = 0;
				setSectorDecalVertex_SinglePass(mapState, x, y, terrainDecalData, &terrainDecalSize);
				sectors[x * ySectors + y].terrainAndDecalSize = terrainDecalSize - sectors[x * ySectors + y].terrainAndDecalOffset;

				sectors[x * ySectors + y].terrainAndDecalIndexOffset = terrainDecalIndexSize;
				sectors[x * ySectors + y].patchIndexOffset = patchIndexSize;
				for (int base = sectors[x * ySectors + y].terrainAndDecalOffset; base < terrainDecalSize; base += decalVertsPerTile)
				{
					if (buildPatches)
					{
						// one 4-control-point patch per tile, corner order
						// [c00, c10, c11, c01] (the 5-vert tile block is
						// [c00, c01, c11, c10, center] - see setSectorDecalVertex_SinglePass)
						patchIndex[patchIndexSize++] = base + 0;
						patchIndex[patchIndexSize++] = base + 3;
						patchIndex[patchIndexSize++] = base + 2;
						patchIndex[patchIndexSize++] = base + 1;
					}
					if (decalN == 1)
					{
						// each tile emitted 5 vertices (4 corners + center).
						// Form its 4 triangles fanned from the center vertex (base + 4).
						terrainDecalIndex[terrainDecalIndexSize + 0]  = base + 4;
						terrainDecalIndex[terrainDecalIndexSize + 1]  = base + 1;
						terrainDecalIndex[terrainDecalIndexSize + 2]  = base + 0;

						terrainDecalIndex[terrainDecalIndexSize + 3]  = base + 4;
						terrainDecalIndex[terrainDecalIndexSize + 4]  = base + 2;
						terrainDecalIndex[terrainDecalIndexSize + 5]  = base + 1;

						terrainDecalIndex[terrainDecalIndexSize + 6]  = base + 4;
						terrainDecalIndex[terrainDecalIndexSize + 7]  = base + 3;
						terrainDecalIndex[terrainDecalIndexSize + 8]  = base + 2;

						terrainDecalIndex[terrainDecalIndexSize + 9]  = base + 4;
						terrainDecalIndex[terrainDecalIndexSize + 10] = base + 0;
						terrainDecalIndex[terrainDecalIndexSize + 11] = base + 3;
						terrainDecalIndexSize += 12;
					}
					else
					{
						// each tile emitted an (N+1)^2 vertex grid, 2 triangles per sub-quad
						for (int a = 0; a < decalN; a++)
						{
							for (int b = 0; b < decalN; b++)
							{
								emitSubQuadIndices(terrainDecalIndex, terrainDecalIndexSize, base + a * (decalN + 1) + b, decalN + 1);
							}
						}
					}
				}
				sectors[x * ySectors + y].terrainAndDecalIndexSize = terrainDecalIndexSize - sectors[x * ySectors + y].terrainAndDecalIndexOffset;
				sectors[x * ySectors + y].patchIndexSize = patchIndexSize - sectors[x * ySectors + y].patchIndexOffset;
		}
	}
	debug(LOG_TERRAIN, "%i decals found", terrainDecalSize / decalVertsPerTile);

	if (terrainDecalVBO)
		delete terrainDecalVBO;
	if (terrainDecalIndexVBO)
		delete terrainDecalIndexVBO;
	if (terrainDecalSize > 0)
	{
		terrainDecalVBO = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::dynamic_draw, "terrain::terrainDecalVBO");
		terrainDecalVBO->upload(sizeof(gfx_api::TerrainDecalVertex)*terrainDecalSize, terrainDecalData);
		terrainDecalIndexVBO = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::index_buffer, gfx_api::context::buffer_storage_hint::static_draw, "terrain::terrainDecalIndexVBO");
		terrainDecalIndexVBO->upload(sizeof(GLuint)*terrainDecalIndexSize, terrainDecalIndex);
	}
	else
	{
		terrainDecalVBO = nullptr;
		terrainDecalIndexVBO = nullptr;
	}
	if (terrainDecalData)
	{
		free(terrainDecalData);
		terrainDecalData = nullptr;
	}
	free(terrainDecalIndex);

	// hardware tessellation: patch index buffer + baked surface field textures
	delete terrainPatchIndexVBO;
	terrainPatchIndexVBO = nullptr;
	if (buildPatches && patchIndexSize > 0)
	{
		terrainPatchIndexVBO = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::index_buffer, gfx_api::context::buffer_storage_hint::static_draw, "terrain::terrainPatchIndexVBO");
		terrainPatchIndexVBO->upload(sizeof(GLuint) * patchIndexSize, patchIndex);
	}
	free(patchIndex);

	lightmapLastUpdate = 0;
	lightmapWidth = 1;
	lightmapHeight = 1;
	// determine the smallest power-of-two size we can use for the lightmap
	while (mapState.width > (lightmapWidth <<= 1)) {}
	while (mapState.height > (lightmapHeight <<= 1)) {}
	debug(LOG_TERRAIN, "the size of the map is %ix%i", mapState.width, mapState.height);
	debug(LOG_TERRAIN, "lightmap texture size is %zu x %zu", lightmapWidth, lightmapHeight);

	lightmapValues.paramsXLight = glm::vec4(1.0f / world_coord(mapState.width) *((float)mapState.width / (float)lightmapWidth), 0, 0, 0);
	lightmapValues.paramsYLight = glm::vec4(0, 0, -1.0f / world_coord(mapState.height) *((float)mapState.height / (float)lightmapHeight), 0);

	// shift the lightmap half a tile as lights are supposed to be placed at the center of a tile
	lightmapValues.lightMatrix = glm::translate(glm::vec3(1.f / (float)lightmapWidth / 2, 1.f / (float)lightmapHeight / 2, 0.f));
	lightmapValues.ModelUVLightmap = lightmapValues.lightMatrix * glm::transpose(glm::mat4(lightmapValues.paramsXLight, lightmapValues.paramsYLight, glm::vec4(0,0,1,0), glm::vec4(0,0,0,1)));

	// Prepare the lightmap pixmap and texture
	lightmapPixmap = std::make_unique<iV_Image>();

	unsigned int lightmapChannels = 4; // always use 4-channel (RGBA)
	if (lightmapPixmap == nullptr || !lightmapPixmap->allocate(lightmapWidth, lightmapHeight, lightmapChannels, true))
	{
		debug(LOG_FATAL, "Out of memory!");
		abort();
		return false;
	}
	if (lightmap_texture)
		delete lightmap_texture;
	lightmap_texture = gfx_api::context::get().create_texture(1, lightmapPixmap->width(), lightmapPixmap->height(), lightmapPixmap->pixel_format(), "mem::lightmap");

	lightmap_texture->upload(0, *(lightmapPixmap.get()));
	terrainInitialised = true;
	debug(LOG_TERRAIN, "initTerrain took %u ms", wzGetTicks() - initStartTime);

	if (enabled_debug[LOG_TERRAIN])
	{
		terrainSurface::debugLogSurfaceStats(mapState);
	}

	return true;
}

/// free all memory and opengl buffers used by the terrain renderer
void shutdownTerrain()
{
	delete geometryVBO;
	geometryVBO = nullptr;
	delete geometryIndexVBO;
	geometryIndexVBO = nullptr;
	delete waterVBO;
	waterVBO = nullptr;
	delete waterIndexVBO;
	waterIndexVBO = nullptr;
	delete terrainDecalVBO;
	terrainDecalVBO = nullptr;
	delete terrainDecalIndexVBO;
	terrainDecalIndexVBO = nullptr;
	delete terrainPatchIndexVBO;
	terrainPatchIndexVBO = nullptr;
	terrainBake::shutdown();
	terrainSurface::clearSurfaceCaches();

	sectors.reset();

	delete lightmap_texture;
	lightmap_texture = nullptr;
	lightmapPixmap = nullptr;

	delete groundTexArr; groundTexArr = nullptr;
	delete groundNormalArr; groundNormalArr = nullptr;
	delete groundSpecularArr; groundSpecularArr = nullptr;
	delete groundHeightArr; groundHeightArr = nullptr;

	waterTexturesNormal.clear();
	waterTexturesHigh.clear();
	delete waterClassicTexture; waterClassicTexture = nullptr;

	delete decalTexArr; decalTexArr = nullptr;
	delete decalNormalArr; decalNormalArr = nullptr;
	delete decalSpecularArr; decalSpecularArr = nullptr;
	delete decalHeightArr; decalHeightArr = nullptr;

	terrainInitialised = false;
}

static void updateLightMap(WorldMapState& mapState, const LightMap& lightmap)
{
	size_t lightmapChannels = lightmapPixmap->channels(); // should always be 4 now...
	unsigned char* lightMapWritePtr = lightmapPixmap->bmp_w();
	for (int j = 0; j < mapState.height; ++j)
	{
		for (int i = 0; i < mapState.width; ++i)
		{
			MAPTILE *psTile = mapTile(mapState, i, j);
			PIELIGHT colour = lightmap(i, j);
			UBYTE level = static_cast<UBYTE>(psTile->level);

			if (psTile->tileInfoBits & BITS_GATEWAY && showGateways)
			{
				colour.byte.g = 255;
			}
			if (psTile->tileInfoBits & BITS_MARKED)
			{
				int m = getModularScaledGraphicsTime(2048, 255);
				colour.byte.r = MAX(m, 255 - m);
				level = std::max<UBYTE>(level, colour.byte.r / 2);
			}

			lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 0] = colour.byte.r;
			lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 1] = colour.byte.g;
			lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 2] = colour.byte.b;
			// store the "brightness" level in byte.a
			// NOTE: This differs depending on whether using the single-pass terrain shader or the fallback terrain shaders
			// (For more, see avUpdateTiles() and getTileIllumination())
			lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 3] = level;

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
					lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 3] = 0;
				}
				else if (darken < 1)
				{
					lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 0] *= darken;
					lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 1] *= darken;
					lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 2] *= darken;
					lightMapWritePtr[(i + j * lightmapWidth) * lightmapChannels + 3] *= darken;
				}
			}
		}
	}
}

static void cullTerrain(WorldMapState& mapState)
{
	const float maxDistance = static_cast<float>(world_coord(terrainDistance));
	const float maxDistanceSquared = maxDistance * maxDistance;

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			float xPos = world_coord(x * sectorSize + sectorSize / 2);
			float yPos = world_coord(y * sectorSize + sectorSize / 2);
			float xDelta = playerPos.p.x - xPos;
			float yDelta = playerPos.p.z - yPos;
			float distance = xDelta * xDelta + yDelta * yDelta;

			if (distance > maxDistanceSquared)
			{
				sectors[x * ySectors + y].draw = false;
			}
			else
			{
				sectors[x * ySectors + y].draw = true;
				if (sectors[x * ySectors + y].dirty)
				{
					updateSectorGeometry(mapState, x, y);
					sectors[x * ySectors + y].dirty = false;
				}
			}
		}
	}
}

/// Near-camera tessellation level for the Terrain Detail setting
/// (Medium/High/Ultra -> 2/4/8)
static float terrainTessMaxLevel()
{
	switch (terrainSubdivision)
	{
		case 2: return 2.f;
		case 3: return 4.f;
		default: return 8.f;
	}
}

static glm::vec4 terrainTessParams()
{
	auto dimension = gfx_api::context::get().getSceneRenderTargetDimensions();
	return glm::vec4(terrainTessMaxLevel(), static_cast<float>(dimension.second), 0.f, 0.f);
}

static void drawDepthOnly(const glm::mat4 &ModelViewProjection, const glm::vec4 &paramsXLight, const glm::vec4 &paramsYLight, bool withOffset)
{
	const auto &renderState = getCurrentRenderState();

	// bind the vertex buffer
	gfx_api::TerrainDepth::get().bind();
	gfx_api::TerrainDepth::get().bind_textures(lightmap_texture);
	gfx_api::TerrainDepth::get().bind_vertex_buffers(geometryVBO);
	gfx_api::TerrainDepth::get().bind_constants({ ModelViewProjection, paramsXLight, paramsYLight, glm::vec4(0.f), glm::vec4(0.f), glm::mat4(1.f), glm::mat4(1.f),
	glm::vec4(0.f), pie_GetShaderDistanceFogEnabled(), renderState.fogBegin, renderState.fogEnd, 0, 0 });
	gfx_api::context::get().bind_index_buffer(*geometryIndexVBO, gfx_api::index_type::u32);

	if (withOffset)
	{
		// draw slightly higher distance than it actually is so it will not
		// by accident obscure the actual terrain
		gfx_api::context::get().set_polygon_offset(0.1f, 1.f);
	}

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			if (sectors[x * ySectors + y].draw)
			{
				batchDrawElements<gfx_api::TerrainDepth>(
					sectors[x * ySectors + y].geometryIndexSize,
					sectors[x * ySectors + y].geometryIndexOffset);
			}
		}
	}
	flushDrawElementsBatch<gfx_api::TerrainDepth>();
	if (withOffset)
	{
		gfx_api::context::get().set_polygon_offset(0.f, 0.f);
	}
	gfx_api::TerrainDepth::get().unbind_vertex_buffers(geometryVBO);
	gfx_api::context::get().unbind_index_buffer(*geometryIndexVBO);
}

static void drawDepthOnlyForDepthMapTess(const glm::mat4 &ModelViewProjection, const glm::mat4 &tessCameraMVP)
{
	if (!terrainDecalVBO || !terrainPatchIndexVBO)
	{
		return;
	}
	const auto &renderState = getCurrentRenderState();

	gfx_api::TerrainDepthOnlyForDepthMapTess::get().bind();
	gfx_api::TerrainDepthOnlyForDepthMapTess::get().bind_textures(lightmap_texture, terrainBake::heightTexture(), terrainBake::offsetTexture(), terrainBake::normalTexture());
	gfx_api::TerrainDepthOnlyForDepthMapTess::get().bind_vertex_buffers(terrainDecalVBO);
	// this pass renders from the light, but the tessellation factors must come from the main camera so the shadow geometry matches the color pass
	gfx_api::TerrainDepthOnlyForDepthMapTess::get().bind_constants({ ModelViewProjection, tessCameraMVP, glm::vec4(0.f), glm::vec4(0.f), glm::mat4(1.f),
	terrainTessParams(), renderState.fogEnabled, renderState.fogBegin, renderState.fogEnd, 0.f });
	gfx_api::context::get().bind_index_buffer(*terrainPatchIndexVBO, gfx_api::index_type::u32);

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			if (sectors[x * ySectors + y].draw)
			{
				batchDrawElements<gfx_api::TerrainDepthOnlyForDepthMapTess>(
					sectors[x * ySectors + y].patchIndexSize,
					sectors[x * ySectors + y].patchIndexOffset);
			}
		}
	}
	flushDrawElementsBatch<gfx_api::TerrainDepthOnlyForDepthMapTess>();
	gfx_api::TerrainDepthOnlyForDepthMapTess::get().unbind_vertex_buffers(terrainDecalVBO);
	gfx_api::context::get().unbind_index_buffer(*terrainPatchIndexVBO);
}

static void drawDepthOnlyForDepthMap(const glm::mat4 &ModelViewProjection, const glm::mat4 &tessCameraMVP, const glm::vec4 &paramsXLight, const glm::vec4 &paramsYLight, bool withOffset)
{
	if (terrainMeshStrategy == TerrainMeshStrategy::HardwareTess)
	{
		drawDepthOnlyForDepthMapTess(ModelViewProjection, tessCameraMVP);
		return;
	}
	const auto &renderState = getCurrentRenderState();

	// bind the vertex buffer
	gfx_api::TerrainDepthOnlyForDepthMap::get().bind();
	gfx_api::TerrainDepthOnlyForDepthMap::get().bind_textures(lightmap_texture);
	gfx_api::TerrainDepthOnlyForDepthMap::get().bind_vertex_buffers(geometryVBO);
	gfx_api::TerrainDepthOnlyForDepthMap::get().bind_constants({ ModelViewProjection, /*paramsXLight, paramsYLight, glm::vec4(0.f), glm::vec4(0.f), glm::mat4(1.f), glm::mat4(1.f),
	glm::vec4(0.f), */ pie_GetShaderDistanceFogEnabled(), renderState.fogBegin, renderState.fogEnd, /*0, 0*/ });
	gfx_api::context::get().bind_index_buffer(*geometryIndexVBO, gfx_api::index_type::u32);

//	if (withOffset)
//	{
//		// draw slightly higher distance than it actually is so it will not
//		// by accident obscure the actual terrain
//		gfx_api::context::get().set_polygon_offset(0.1f, 1.f);
//	}

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			if (sectors[x * ySectors + y].draw)
			{
				batchDrawElements<gfx_api::TerrainDepthOnlyForDepthMap>(
					sectors[x * ySectors + y].geometryIndexSize,
					sectors[x * ySectors + y].geometryIndexOffset);
			}
		}
	}
	flushDrawElementsBatch<gfx_api::TerrainDepthOnlyForDepthMap>();
//	if (withOffset)
//	{
//		gfx_api::context::get().set_polygon_offset(0.f, 0.f);
//	}
	gfx_api::TerrainDepthOnlyForDepthMap::get().unbind_vertex_buffers(geometryVBO);
	gfx_api::context::get().unbind_index_buffer(*geometryIndexVBO);
}

glm::vec4 getFogColorVec4()
{
	const auto &renderState = getCurrentRenderState();
	return pielightToRGBAVec4(renderState.fogColour);
}

template<typename PSO>
static void drawTerrainCombinedmpl(const glm::mat4 &ModelViewProjection, const glm::mat4& ViewMatrix, const glm::mat4 &ModelUVLightmap, const Vector3f &cameraPos, const Vector3f &sunPos, const ShadowCascadesInfo& shadowCascades, gfx_api::abstract_texture* shadowMap)
{
	if (!terrainDecalVBO || !terrainDecalIndexVBO)
	{
		return; // no terrain+decal geometry (empty map)
	}
	const auto &renderState = getCurrentRenderState();
	PSO::get().bind();
	PSO::get().bind_textures(
		lightmap_texture,
		groundTexArr, groundNormalArr, groundSpecularArr, groundHeightArr,
		decalTexArr, decalNormalArr, decalSpecularArr, decalHeightArr,
		shadowMap);
	PSO::get().bind_vertex_buffers(terrainDecalVBO);
	gfx_api::context::get().bind_index_buffer(*terrainDecalIndexVBO, gfx_api::index_type::u32);
	glm::mat4 groundScale = glm::mat4(0);
	for (int i = 0; i < getNumGroundTypes(); i++) {
		groundScale[i/4][i%4] = 1.0f / (getGroundType(i).textureSize * world_coord(1));
	}

	auto bucketLight = getCurrentLightingManager().getPointLightBuckets();
	auto dimension = gfx_api::context::get().getSceneRenderTargetDimensions();
	gfx_api::TerrainCombinedUniforms uniforms = {
		ModelViewProjection, ViewMatrix, ModelUVLightmap, {shadowCascades.shadowMVPMatrix[0], shadowCascades.shadowMVPMatrix[1], shadowCascades.shadowMVPMatrix[2]}, groundScale,
		glm::vec4(cameraPos, 0), glm::vec4(glm::normalize(sunPos), 0),
		pie_GetLighting0(LIGHT_EMISSIVE), pie_GetLighting0(LIGHT_AMBIENT), pie_GetLighting0(LIGHT_DIFFUSE), pie_GetLighting0(LIGHT_SPECULAR),
		getFogColorVec4(), {shadowCascades.shadowCascadeSplit[0], shadowCascades.shadowCascadeSplit[1], shadowCascades.shadowCascadeSplit[2], pie_getPerspectiveZFar()}, shadowCascades.shadowMapSize,
		pie_GetShaderDistanceFogEnabled(), renderState.fogBegin, renderState.fogEnd, terrainShaderQuality, static_cast<int>(dimension.first), static_cast<int>(dimension.second), 0.f, bucketLight.positions, bucketLight.colorAndEnergy, bucketLight.bucketOffsetAndSize, bucketLight.light_index, static_cast<int>(bucketLight.bucketDimensionUsed), gfx_api::context::get().getSceneMipLodBias()
	};
	PSO::get().set_uniforms(uniforms);

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			if (sectors[x * ySectors + y].draw)
			{
				batchDrawElements<PSO>(
					sectors[x * ySectors + y].terrainAndDecalIndexSize,
					sectors[x * ySectors + y].terrainAndDecalIndexOffset);
			}
		}
	}
	flushDrawElementsBatch<PSO>();
	PSO::get().unbind_vertex_buffers(terrainDecalVBO);
	gfx_api::context::get().unbind_index_buffer(*terrainDecalIndexVBO);
}

template<typename PSO>
static void drawTerrainCombinedTessImpl(const glm::mat4 &ModelViewProjection, const glm::mat4& ViewMatrix, const glm::mat4 &ModelUVLightmap, const Vector3f &cameraPos, const Vector3f &sunPos, const ShadowCascadesInfo& shadowCascades, gfx_api::abstract_texture* shadowMap)
{
	if (!terrainDecalVBO || !terrainPatchIndexVBO)
	{
		return; // no terrain+decal geometry (empty map)
	}
	const auto &renderState = getCurrentRenderState();
	PSO::get().bind();
	PSO::get().bind_textures(
		lightmap_texture,
		groundTexArr, groundNormalArr, groundSpecularArr, groundHeightArr,
		decalTexArr, decalNormalArr, decalSpecularArr, decalHeightArr,
		shadowMap,
		terrainBake::heightTexture(), terrainBake::offsetTexture(), terrainBake::normalTexture());
	PSO::get().bind_vertex_buffers(terrainDecalVBO);
	gfx_api::context::get().bind_index_buffer(*terrainPatchIndexVBO, gfx_api::index_type::u32);
	glm::mat4 groundScale = glm::mat4(0);
	for (int i = 0; i < getNumGroundTypes(); i++) {
		groundScale[i/4][i%4] = 1.0f / (getGroundType(i).textureSize * world_coord(1));
	}

	auto bucketLight = getCurrentLightingManager().getPointLightBuckets();
	auto dimension = gfx_api::context::get().getSceneRenderTargetDimensions();
	gfx_api::TerrainCombinedUniforms uniforms = {
		ModelViewProjection, ViewMatrix, ModelUVLightmap, {shadowCascades.shadowMVPMatrix[0], shadowCascades.shadowMVPMatrix[1], shadowCascades.shadowMVPMatrix[2]}, groundScale,
		glm::vec4(cameraPos, 0), glm::vec4(glm::normalize(sunPos), 0),
		pie_GetLighting0(LIGHT_EMISSIVE), pie_GetLighting0(LIGHT_AMBIENT), pie_GetLighting0(LIGHT_DIFFUSE), pie_GetLighting0(LIGHT_SPECULAR),
		getFogColorVec4(), {shadowCascades.shadowCascadeSplit[0], shadowCascades.shadowCascadeSplit[1], shadowCascades.shadowCascadeSplit[2], pie_getPerspectiveZFar()}, shadowCascades.shadowMapSize,
		pie_GetShaderDistanceFogEnabled(), renderState.fogBegin, renderState.fogEnd, terrainShaderQuality, static_cast<int>(dimension.first), static_cast<int>(dimension.second), terrainTessMaxLevel(), bucketLight.positions, bucketLight.colorAndEnergy, bucketLight.bucketOffsetAndSize, bucketLight.light_index, static_cast<int>(bucketLight.bucketDimensionUsed), gfx_api::context::get().getSceneMipLodBias()
	};
	PSO::get().set_uniforms(uniforms);

	// This pass also writes terrain depth (there is no separate depth prepass under
	// hardware tessellation, avoiding a second tessellated pass). Keep the prepass's
	// polygon offset: structure baseplates rely on terrain depth being biased
	// slightly farther to avoid z-fighting, and the offset biases depth only, not
	// the rasterized position, so the drawn terrain is unaffected.
	gfx_api::context::get().set_polygon_offset(0.1f, 1.f);

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			if (sectors[x * ySectors + y].draw)
			{
				batchDrawElements<PSO>(
					sectors[x * ySectors + y].patchIndexSize,
					sectors[x * ySectors + y].patchIndexOffset);
			}
		}
	}
	flushDrawElementsBatch<PSO>();
	gfx_api::context::get().set_polygon_offset(0.f, 0.f);
	PSO::get().unbind_vertex_buffers(terrainDecalVBO);
	gfx_api::context::get().unbind_index_buffer(*terrainPatchIndexVBO);
}

static void drawTerrainCombined(const glm::mat4 &ModelViewProjection, const glm::mat4& ViewMatrix, const glm::mat4 &ModelUVLightmap, const Vector3f &cameraPos, const Vector3f &sunPos, const ShadowCascadesInfo& shadowCascades, gfx_api::abstract_texture* shadowMap)
{
	if (terrainMeshStrategy == TerrainMeshStrategy::HardwareTess)
	{
		switch (terrainShaderQuality)
		{
			case TerrainShaderQuality::MEDIUM:
				drawTerrainCombinedTessImpl<gfx_api::TerrainCombinedTess_Medium>(ModelViewProjection, ViewMatrix, ModelUVLightmap, cameraPos, sunPos, shadowCascades, shadowMap);
				return;
			case TerrainShaderQuality::NORMAL_MAPPING:
				drawTerrainCombinedTessImpl<gfx_api::TerrainCombinedTess_High>(ModelViewProjection, ViewMatrix, ModelUVLightmap, cameraPos, sunPos, shadowCascades, shadowMap);
				return;
			default:
				break; // Classic never uses HardwareTess - fall through to the CPU path
		}
	}
	switch (terrainShaderQuality)
	{
		case TerrainShaderQuality::CLASSIC:
			drawTerrainCombinedmpl<gfx_api::TerrainCombined_Classic>(ModelViewProjection, ViewMatrix, ModelUVLightmap, cameraPos, sunPos, shadowCascades, shadowMap);
			break;
		case TerrainShaderQuality::MEDIUM:
			drawTerrainCombinedmpl<gfx_api::TerrainCombined_Medium>(ModelViewProjection, ViewMatrix, ModelUVLightmap, cameraPos, sunPos, shadowCascades, shadowMap);
			break;
		case TerrainShaderQuality::NORMAL_MAPPING:
			drawTerrainCombinedmpl<gfx_api::TerrainCombined_High>(ModelViewProjection, ViewMatrix, ModelUVLightmap, cameraPos, sunPos, shadowCascades, shadowMap);
			break;
		case TerrainShaderQuality::UNINITIALIZED_PICK_DEFAULT:
			// should not happen
			break;
	}

}

void perFrameTerrainUpdates(WorldMapState& mapState, const LightMap& lightMap)
{
	WZ_PROFILE_SCOPE(perFrameTerrainUpdates);
	///////////////////////////////////
	// set up the lightmap texture

	// we limit the framerate of the lightmap, because updating a texture is an expensive operation
	if (realTime - lightmapLastUpdate >= LIGHTMAP_REFRESH)
	{
		lightmapLastUpdate = realTime;
		updateLightMap(mapState, lightMap);

		lightmap_texture->upload(0, *(lightmapPixmap.get()));
	}

	///////////////////////////////////
	// terrain culling
	cullTerrain(mapState);
}

gfx_api::texture* getTerrainLightmapTexture()
{
	return lightmap_texture;
}

const glm::mat4& getModelUVLightmapMatrix()
{
	return lightmapValues.ModelUVLightmap;
}

void drawTerrainDepthOnly(const glm::mat4 &mvp, const glm::mat4 &tessCameraMVP)
{
	drawDepthOnlyForDepthMap(mvp, tessCameraMVP, lightmapValues.paramsXLight, lightmapValues.paramsYLight, false);
}

void drawTerrainDepthNormalPrepass(const glm::mat4& projection, const glm::mat4& view)
{
	const glm::mat4 mvp = projection * view;
	if (terrainMeshStrategy == TerrainMeshStrategy::HardwareTess)
	{
		if (!terrainDecalVBO || !terrainPatchIndexVBO)
		{
			return;
		}

		gfx_api::TerrainDepthPrepassTess::get().bind();
		gfx_api::TerrainDepthPrepassTess::get().bind_textures(
			terrainBake::heightTexture(), terrainBake::offsetTexture(), terrainBake::normalTexture());
		gfx_api::TerrainDepthPrepassTess::get().bind_vertex_buffers(terrainDecalVBO);
		gfx_api::TerrainDepthPrepassTess::get().bind_constants({ mvp, view, mvp, terrainTessParams() });
		gfx_api::context::get().bind_index_buffer(*terrainPatchIndexVBO, gfx_api::index_type::u32);

		for (int x = 0; x < xSectors; x++)
		{
			for (int y = 0; y < ySectors; y++)
			{
				if (sectors[x * ySectors + y].draw)
				{
					batchDrawElements<gfx_api::TerrainDepthPrepassTess>(
						sectors[x * ySectors + y].patchIndexSize,
						sectors[x * ySectors + y].patchIndexOffset);
				}
			}
		}
		flushDrawElementsBatch<gfx_api::TerrainDepthPrepassTess>();
		gfx_api::TerrainDepthPrepassTess::get().unbind_vertex_buffers(terrainDecalVBO);
		gfx_api::context::get().unbind_index_buffer(*terrainPatchIndexVBO);
		return;
	}

	if (!geometryVBO || !geometryIndexVBO)
	{
		return;
	}

	gfx_api::TerrainDepthPrepass::get().bind();
	gfx_api::TerrainDepthPrepass::get().bind_vertex_buffers(geometryVBO);
	gfx_api::TerrainDepthPrepass::get().bind_constants({ projection, view });
	gfx_api::context::get().bind_index_buffer(*geometryIndexVBO, gfx_api::index_type::u32);

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			if (sectors[x * ySectors + y].draw)
			{
				batchDrawElements<gfx_api::TerrainDepthPrepass>(
					sectors[x * ySectors + y].geometryIndexSize,
					sectors[x * ySectors + y].geometryIndexOffset);
			}
		}
	}
	flushDrawElementsBatch<gfx_api::TerrainDepthPrepass>();
	gfx_api::TerrainDepthPrepass::get().unbind_vertex_buffers(geometryVBO);
	gfx_api::context::get().unbind_index_buffer(*geometryIndexVBO);
}

/**
 * Update the lightmap and draw the terrain and decals.
 * This function first draws the terrain in black, and then uses additive blending to put the terrain layers
 * on it one by one. Finally the decals are drawn.
 */
void drawTerrain(const glm::mat4 &mvp, const glm::mat4& viewMatrix, const Vector3f &cameraPos, const Vector3f &sunPos, const ShadowCascadesInfo& shadowCascades, gfx_api::abstract_texture* shadowMap)
{
	WZ_PROFILE_SCOPE(drawTerrain);
	const glm::vec4& paramsXLight = lightmapValues.paramsXLight;
	const glm::vec4& paramsYLight = lightmapValues.paramsYLight;
	const glm::mat4& ModelUVLightmap = lightmapValues.ModelUVLightmap;

	if (terrainMeshStrategy != TerrainMeshStrategy::HardwareTess)
	{
		//////////////////////////////////////
		// canvas to draw on
		// (under hardware tessellation the combined pass writes depth itself,
		// with the prepass's polygon offset, so no separate depth prepass is needed)
		drawDepthOnly(mvp, paramsXLight, paramsYLight, true);
	}

	if (terrainShaderQuality == TerrainShaderQuality::CLASSIC)
	{
		// draw water *first*
		drawWaterClassic(mvp, ModelUVLightmap, cameraPos, sunPos);
	}

	///////////////////////////////////
	// terrain + decals
	drawTerrainCombined(mvp, viewMatrix, ModelUVLightmap, cameraPos, sunPos, shadowCascades, shadowMap);
}

/**
 * Draw the water.
 * sunPos and cameraPos in Model=WorldSpace
 */
template<typename PSO>
void drawWaterNormalImpl(const glm::mat4 &ModelViewProjection, const Vector3f &cameraPos, const Vector3f &sunPos)
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

	ASSERT_OR_RETURN(, waterTexturesNormal.tex1 && waterTexturesNormal.tex2, "Failed to load water texture");
	PSO::get().bind();

	PSO::get().bind_textures(
		waterTexturesNormal.tex1, waterTexturesNormal.tex2,
		lightmap_texture);
	PSO::get().bind_vertex_buffers(waterVBO);
	PSO::get().bind_constants({
		ModelViewProjection, lightmapValues.ModelUVLightmap, ModelUV1, ModelUV2,
		glm::vec4(cameraPos, 0), glm::vec4(glm::normalize(sunPos), 0),
		pie_GetLighting0(LIGHT_EMISSIVE), pie_GetLighting0(LIGHT_AMBIENT), pie_GetLighting0(LIGHT_DIFFUSE), pie_GetLighting0(LIGHT_SPECULAR),
		getFogColorVec4(), pie_GetShaderDistanceFogEnabled(), renderState.fogBegin, renderState.fogEnd,
		waterOffset*10, gfx_api::context::get().getSceneMipLodBias()
	});

	gfx_api::context::get().bind_index_buffer(*waterIndexVBO, gfx_api::index_type::u32);

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			if (sectors[x * ySectors + y].draw)
			{
				batchDrawElements<PSO>(
				                     sectors[x * ySectors + y].waterIndexSize,
				                     sectors[x * ySectors + y].waterIndexOffset);
			}
		}
	}
	flushDrawElementsBatch<PSO>();
	PSO::get().unbind_vertex_buffers(waterVBO);
	gfx_api::context::get().unbind_index_buffer(*waterIndexVBO);

	// move the water
	if (!gamePaused())
	{
		waterOffset += graphicsTimeAdjustedIncrement(0.1f);
	}
}

template<typename PSO>
void drawWaterHighImpl(const glm::mat4 &ModelViewProjection, const glm::mat4& viewMatrix, const Vector3f &cameraPos, const Vector3f &sunPos, const ShadowCascadesInfo& shadowCascades, gfx_api::abstract_texture* shadowMap)
{
	if (!waterIndexVBO)
	{
		return; // no water
	}

	const glm::vec4 paramsX(0, 0, -1.0f / world_coord(4), 0);
	const glm::vec4 paramsY(1.0f / world_coord(4), 0, 0, 0);
	const glm::vec4 paramsX2(0, 0, -1.0f / world_coord(5), 0);
	const glm::vec4 paramsY2(1.0f / world_coord(5), 0, 0, 0);
	const auto &renderState = getCurrentRenderState();

	ASSERT_OR_RETURN(, waterTexturesHigh.tex, "Failed to load water textures");
	PSO::get().bind();

	PSO::get().bind_textures(
		waterTexturesHigh.tex,
		waterTexturesHigh.tex_nm,
		waterTexturesHigh.tex_sm,
		lightmap_texture,
		shadowMap);
	PSO::get().bind_vertex_buffers(waterVBO);
	PSO::get().bind_constants({
		ModelViewProjection, viewMatrix, lightmapValues.ModelUVLightmap, {shadowCascades.shadowMVPMatrix[0], shadowCascades.shadowMVPMatrix[1], shadowCascades.shadowMVPMatrix[2]},
		glm::vec4(cameraPos, 0), glm::vec4(glm::normalize(sunPos), 0),
		pie_GetLighting0(LIGHT_EMISSIVE), pie_GetLighting0(LIGHT_AMBIENT), pie_GetLighting0(LIGHT_DIFFUSE), pie_GetLighting0(LIGHT_SPECULAR),
		getFogColorVec4(), {shadowCascades.shadowCascadeSplit[0], shadowCascades.shadowCascadeSplit[1], shadowCascades.shadowCascadeSplit[2], pie_getPerspectiveZFar()}, shadowCascades.shadowMapSize, pie_GetShaderDistanceFogEnabled(), renderState.fogBegin, renderState.fogEnd,
		waterOffset*10, gfx_api::context::get().getSceneMipLodBias()
	});

	gfx_api::context::get().bind_index_buffer(*waterIndexVBO, gfx_api::index_type::u32);

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			if (sectors[x * ySectors + y].draw)
			{
				batchDrawElements<PSO>(
									 sectors[x * ySectors + y].waterIndexSize,
									 sectors[x * ySectors + y].waterIndexOffset);
			}
		}
	}
	flushDrawElementsBatch<PSO>();
	PSO::get().unbind_vertex_buffers(waterVBO);
	gfx_api::context::get().unbind_index_buffer(*waterIndexVBO);

	// move the water
	if (!gamePaused())
	{
		waterOffset += graphicsTimeAdjustedIncrement(0.1f);
	}
}

void drawWaterClassic(const glm::mat4 &ModelViewProjection, const glm::mat4 &ModelUVLightmap, const Vector3f &cameraPos, const Vector3f &sunPos)
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

	auto pWaterTexture = getWaterClassicTexture();
	ASSERT(pWaterTexture != nullptr, "Failed to load water legacy texture?");

	gfx_api::WaterClassicPSO::get().bind();

	gfx_api::WaterClassicPSO::get().bind_textures(lightmap_texture, pWaterTexture);
	gfx_api::WaterClassicPSO::get().bind_vertex_buffers(waterVBO);
	gfx_api::WaterClassicPSO::get().set_uniforms(gfx_api::constant_buffer_type<SHADER_WATER_CLASSIC>{
		ModelViewProjection, ModelUVLightmap, glm::mat4(1.f) /* TODO */, ModelUV1, ModelUV2,
		glm::vec4(cameraPos, 0), glm::vec4(glm::normalize(sunPos), 0),
		getFogColorVec4(), pie_GetShaderDistanceFogEnabled(), renderState.fogBegin, renderState.fogEnd,
		waterOffset*10, gfx_api::context::get().getSceneMipLodBias()
	});

	gfx_api::context::get().bind_index_buffer(*waterIndexVBO, gfx_api::index_type::u32);

	for (int x = 0; x < xSectors; x++)
	{
		for (int y = 0; y < ySectors; y++)
		{
			if (sectors[x * ySectors + y].draw)
			{
				batchDrawElements<gfx_api::WaterClassicPSO>(
									 sectors[x * ySectors + y].waterIndexSize,
									 sectors[x * ySectors + y].waterIndexOffset);
			}
		}
	}
	flushDrawElementsBatch<gfx_api::WaterClassicPSO>();
	gfx_api::WaterClassicPSO::get().unbind_vertex_buffers(waterVBO);
	gfx_api::context::get().unbind_index_buffer(*waterIndexVBO);

	// move the water
	if (!gamePaused())
	{
		waterOffset += graphicsTimeAdjustedIncrement(0.1f);
	}
}

#include <lib/ivis_opengl/pieblitfunc.h>

void drawWater(const glm::mat4 &ModelViewProjection, const glm::mat4& viewMatrix, const Vector3f &cameraPos, const Vector3f &sunPos, const ShadowCascadesInfo& shadowCascades, gfx_api::abstract_texture* shadowMap)
{
	switch (terrainShaderQuality)
	{
		case TerrainShaderQuality::CLASSIC:
			// already drawn
			return;
		case TerrainShaderQuality::MEDIUM:
			drawWaterNormalImpl<gfx_api::WaterPSO>(ModelViewProjection, cameraPos, sunPos);
			return;
		case TerrainShaderQuality::NORMAL_MAPPING:
			drawWaterHighImpl<gfx_api::WaterHighPSO>(ModelViewProjection, viewMatrix, cameraPos, sunPos, shadowCascades, shadowMap);
			return;
		case TerrainShaderQuality::UNINITIALIZED_PICK_DEFAULT:
			// should not happen
			break;
	}
}

// MARK: Terrain shader quality / appearance

bool determineTerrainShaderRequirementsMet()
{
	if (!gfx_api::context::get().supports2DTextureArrays())
	{
		debug(LOG_ERROR, "Graphics context: supports2DTextureArrays = false");
		return false;
	}
	if (!gfx_api::context::get().supportsIntVertexAttributes())
	{
		debug(LOG_ERROR, "Graphics context: supportsIntVertexAttributes = false");
		return false;
	}
	auto maxArrayTextureLayers = gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_ARRAY_TEXTURE_LAYERS);
	if (maxArrayTextureLayers < 256) // "big enough" value
	{
		debug(LOG_ERROR, "Graphics context: MAX_ARRAY_TEXTURE_LAYERS is insufficient: %d", maxArrayTextureLayers);
		return false;
	}

	return true;
}

TerrainShaderQuality getTerrainShaderQuality()
{
	return terrainShaderQuality;
}

bool setTerrainShaderQuality(TerrainShaderQuality newValue, bool force, bool forceReloadTextures = false)
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
	auto priorValue = terrainShaderQuality;

	// Check whether the new shader can be used
	// - all quality modes are supported by shader
	// - but check if texture pack is available
	if (isSupportedTerrainShaderQualityOption(newValue))
	{
		terrainShaderQuality = newValue;
		success = true;
	}
	else
	{
		// texture pack is not available
		terrainShaderQuality = priorValue;
		if (newValue != priorValue)
		{
			debug(LOG_INFO, "Missing texture pack for %s terrain quality - reverting to %s terrain quality", to_display_string(newValue).c_str(), to_display_string(priorValue).c_str());
			success = false;
		}
		else
		{
			// need to pick another supported value
			auto supportedModes = getAvailableTerrainShaderQualityTextures();
			if (!supportedModes.empty())
			{
				debug(LOG_INFO, "Missing texture pack for %s terrain quality - reverting to %s terrain quality", to_display_string(newValue).c_str(), to_display_string(supportedModes.back()).c_str());
				terrainShaderQuality = supportedModes.back();
				success = true;
			}
		}
		newValue = terrainShaderQuality;
	}

	if (success)
	{
		uint32_t debugQualityUint = static_cast<uint32_t>(terrainShaderQuality);
		std::string terrainQualityUintStr = std::to_string(debugQualityUint);
		crashHandlingProviderSetTag("wz.terrain_quality", terrainQualityUintStr);

		dirtyAllSectors();

		// re-load terrain textures
		if (!rebuildExistingSearchPathWithGraphicsOptionChange())
		{
			debug(LOG_INFO, "Failed to swap terrain texture overrides");
		}
		// always re-load the tile textures (these change between all modes)
		auto priorNumGroundTypes = getNumGroundTypes();
		reloadTileTextures();
		mapReloadGroundTypes();
		if (priorValue != terrainShaderQuality
			|| priorNumGroundTypes != getNumGroundTypes()
			|| forceReloadTextures)
		{
			reloadTerrainTextures();
		}

		// switching to/from Classic can change the effective mesh subdivision -
		// buffer sizes change with it, so this needs a full terrain re-init
		if (terrainInitialised && effectiveTerrainMeshSubdivision() != terrainSubdivision)
		{
			if (!initTerrain(gameWorld.map))
			{
				debug(LOG_ERROR, "Failed to re-initialize terrain for mesh subdivision change");
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
	auto supportedModes = getAvailableTerrainShaderQualityTextures();
	// all quality modes are supported by shader - check if textures are available
	return std::any_of(supportedModes.begin(), supportedModes.end(), [value](TerrainShaderQuality supportedQuality) { return supportedQuality == value; });
}

static TerrainShaderQuality determineDefaultTerrainQuality()
{
	// Based on system properties, determine a reasonable default (for performance reasons)
	// (Uses a heuristic based on system RAM, graphics renderer, and estimated VRAM)

	// If <= 8 GiB system RAM, default to medium ("normal")
	auto systemRAMinMiB = wzGetCurrentSystemRAM();
	if (systemRAMinMiB <= 8192)
	{
		debug(LOG_INFO, "Due to system RAM (%" PRIu64 " MiB), defaulting to terrain quality: Normal", systemRAMinMiB);
		return TerrainShaderQuality::MEDIUM;
	}

	// For specific older integrated cards on OpenGL, default to medium ("normal")
	auto backendInfo = gfx_api::context::get().getBackendGameInfo();
	auto it = backendInfo.find("openGL_renderer");
	if (it != backendInfo.end())
	{
		// If opengl_renderer starts with "Intel(R) HD Graphics"
		if (it->second.rfind("Intel(R) HD Graphics", 0) == 0)
		{
			// default to medium ("normal")
			return TerrainShaderQuality::MEDIUM;
		}
		// If opengl_renderer starts with "Intel(R) Graphics Media Accelerator"
		if (it->second.rfind("Intel(R) Graphics Media Accelerator", 0) == 0)
		{
			// default to medium ("normal")
			return TerrainShaderQuality::MEDIUM;
		}
	}

	// If it's the null backend (headless mode)
	it = backendInfo.find("null_gfx_backend");
	if (it != backendInfo.end())
	{
		// default to medium ("normal")
		debug(LOG_INFO, "Due to null gfx backend, defaulting to terrain quality: Normal");
		return TerrainShaderQuality::MEDIUM;
	}

	// Try to get the estimated available VRAM
	auto estimatedVRAMinMiB = gfx_api::context::get().get_estimated_vram_mb(false);
	if (estimatedVRAMinMiB > 0)
	{
		// If estimated VRAM < 2 GiB
		if (estimatedVRAMinMiB < 2048)
		{
			// default to medium ("normal")
			debug(LOG_INFO, "Due to estimated VRAM (%" PRIu64 " MiB), defaulting to terrain quality: Normal", estimatedVRAMinMiB);
			return TerrainShaderQuality::MEDIUM;
		}
	}

#if INTPTR_MAX <= INT32_MAX
	// On 32-bit builds, default to MEDIUM
	debug(LOG_INFO, "32-bit build defaulting to terrain quality: Normal");
	return TerrainShaderQuality::MEDIUM;
#else
	// If all of the above check out, default to NORMAL_MAPPING
	return TerrainShaderQuality::NORMAL_MAPPING;
#endif
}

static int32_t determineDefaultTerrainMappingTextureSize()
{
	if (determineDefaultTerrainQuality() == TerrainShaderQuality::MEDIUM)
	{
		// system checks yield medium terrain quality as the default
		// so default the terrain mapping texture size to 512 (in case the user switches to NORMAL_MAPPING)
		return 512;
	}

	// If < 8 GiB system RAM, default to 512
	auto systemRAMinMiB = wzGetCurrentSystemRAM();
	if (systemRAMinMiB < 8192)
	{
		debug(LOG_INFO, "Due to system RAM (%" PRIu64 " MiB), defaulting to terrain mapping texture size: 512", systemRAMinMiB);
		return 512;
	}

	// For specific older integrated cards on OpenGL, default to 512
	auto backendInfo = gfx_api::context::get().getBackendGameInfo();
	auto it = backendInfo.find("openGL_renderer");
	if (it != backendInfo.end())
	{
		// If opengl_renderer starts with "Intel(R) HD Graphics"
		if (it->second.rfind("Intel(R) HD Graphics", 0) == 0)
		{
			return 512;
		}
		// If opengl_renderer starts with "Intel(R) Graphics Media Accelerator"
		if (it->second.rfind("Intel(R) Graphics Media Accelerator", 0) == 0)
		{
			return 512;
		}
	}

	// Get the estimated *dedicated* VRAM
	auto estimatedVRAMinMiB = gfx_api::context::get().get_estimated_vram_mb(true);
	if (estimatedVRAMinMiB <= 4096)
	{
		// If estimated dedicated VRAM value is <= 4 GiB (or cannot be determined), default to 512
		debug(LOG_INFO, "Due to estimated dedicated VRAM (%" PRIu64 " MiB), defaulting to terrain mapping texture size: 512", estimatedVRAMinMiB);
		return 512;
	}

#if INTPTR_MAX <= INT32_MAX
	// On 32-bit builds, default to 512
	debug(LOG_INFO, "32-bit build defaulting to terrain mapping texture size: 512");
	return 512;
#else
	// If all of the above check out, default to 1024
	return 1024;
#endif
}

void initTerrainShaderType()
{
	if (!determineTerrainShaderRequirementsMet())
	{
		crashHandlingProviderCaptureException(__FUNCTION__, "NewTerrainRendererUnsupported", "", false, true);
		debug(LOG_FATAL, "Your system does not support the terrain renderer. Please check your graphics drivers.");
		abort();
	}

	initializedTerrainShaderType = true;
	if (terrainShaderQuality == TerrainShaderQuality::UNINITIALIZED_PICK_DEFAULT)
	{
		terrainShaderQuality = determineDefaultTerrainQuality();
		debug(LOG_INFO, "Defaulting terrain quality to: %s", to_display_string(terrainShaderQuality).c_str());
	}
	if (maxTerrainMappingTextureSize == 0)
	{
		maxTerrainMappingTextureSize = determineDefaultTerrainMappingTextureSize();
	}
	if (getTerrainMeshSubdivision() == 0)
	{
		setTerrainMeshSubdivision(determineDefaultTerrainMeshSubdivision());
		debug(LOG_INFO, "Defaulting terrain detail to: %d", getTerrainMeshSubdivision());
	}
	setTerrainShaderQuality(terrainShaderQuality, true); // checks and resets unsupported values
}

std::string to_display_string(TerrainShaderQuality value)
{
	switch (value)
	{
		case TerrainShaderQuality::UNINITIALIZED_PICK_DEFAULT:
			return "";
		case TerrainShaderQuality::CLASSIC:
			return _("Classic");
		case TerrainShaderQuality::MEDIUM:
			return _("Normal");
		case TerrainShaderQuality::NORMAL_MAPPING:
			return _("Remastered (HQ)");
	}
	return ""; // silence warning
}
