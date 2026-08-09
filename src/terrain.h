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

#ifndef __INCLUDED_SRC_TERRAIN_H__
#define __INCLUDED_SRC_TERRAIN_H__

#include <glm/fwd.hpp>
#include "lib/ivis_opengl/pietypes.h"
#include <wzmaplib/terrain_type.h>
#include "terrain_defs.h"
#include "lib/framework/loading_task_fwd.h"

struct ShadowCascadesInfo;
struct LightMap;
struct WorldMapState;

class ResourceLoadingController;

LoadingTask<> loadTerrainTextures(ResourceLoadingController& controller, MAP_TILESET mapTileset);
void loadTerrainTexturesBlocking(MAP_TILESET mapTileset);

bool initTerrain(WorldMapState& mapState);
void shutdownTerrain();

namespace gfx_api
{
	struct abstract_texture; // forward-declare
	struct texture; // forward-declare
}

void perFrameTerrainUpdates(WorldMapState& mapState, const LightMap& lightData);
// tessCameraMVP: the MAIN camera's MVP for this frame
// under the hardware tessellation strategy, every pass must derive tessellation factors from the
// same camera or the passes rasterize different terrain geometry
void drawTerrainDepthOnly(const glm::mat4 &mvp, const glm::mat4 &tessCameraMVP);
/// Opaque terrain depth + view-space normals for the SSAO scene prepass.
void drawTerrainDepthNormalPrepass(const glm::mat4& projection, const glm::mat4& view);
void drawTerrain(const glm::mat4 &mvp, const glm::mat4& viewMatrix, const Vector3f &cameraPos, const Vector3f &sunPos,
	const ShadowCascadesInfo& shadowMVPMatrix, gfx_api::abstract_texture* shadowMap);
void drawWater(const glm::mat4 &ModelViewProjection, const glm::mat4& viewMatrix, const Vector3f &cameraPos, const Vector3f &sunPos,
	const ShadowCascadesInfo& shadowCascades, gfx_api::abstract_texture* shadowMap);

gfx_api::texture* getTerrainLightmapTexture();
const glm::mat4& getModelUVLightmapMatrix();

void markTileDirty(int i, int j);
void dirtyAllSectors();

enum TerrainShaderType
{
	FALLBACK, // old multi-pass method, which only supports "classic" rendering
	SINGLE_PASS // new terrain rendering method, supports all TerrainShaderQuality modes
};

extern TerrainShaderType terrainShaderType;

TerrainShaderQuality getTerrainShaderQuality();
bool setTerrainShaderQuality(TerrainShaderQuality newValue);
std::vector<TerrainShaderQuality> getAllTerrainShaderQualityOptions();
bool isSupportedTerrainShaderQualityOption(TerrainShaderQuality value);
std::string to_display_string(TerrainShaderQuality value);

bool setTerrainMappingTexturesMaxSize(int texSize);
int getTerrainMappingTexturesMaxSize();

// Terrain mesh subdivision factor / "Terrain Detail" (1 = legacy geometry, up
// to MAX_TERRAIN_MESH_SUBDIVISION). Values > 1 render a dense mesh sampled
// from the smooth terrain surface (terrain_surface.h) - N x N sub-quads per
// tile on the CPU strategy, GPU-generated triangles under hardware
// tessellation.
// Rendering only - no simulation impact. Takes effect immediately when
// in-game (full terrain re-init), otherwise at the next initTerrain().
// Classic terrain quality always renders with the legacy geometry regardless
// of this setting. getTerrainMeshSubdivision() returns 0 until a default has
// been picked (during initTerrainShaderType) or a value has been set.
#define MAX_TERRAIN_MESH_SUBDIVISION 4
bool setTerrainMeshSubdivision(int factor);
int getTerrainMeshSubdivision();

// How the dense terrain mesh is produced when Terrain Detail is above Off:
// CPU-subdivided vertex buffers, or hardware tessellation (per-tile patches evaluating baked surface fields on the GPU).
// Internal strategy behind the same Terrain Detail setting.
// Auto currently resolves to the CPU mesh.
enum class TerrainTessellationPreference
{
	Auto = 0,
	ForceCPU = 1,
	ForceHardware = 2,
};
bool setTerrainTessellationPreference(TerrainTessellationPreference pref);
TerrainTessellationPreference getTerrainTessellationPreference();

// Visual-only height correction for settling a rendered object onto the drawn
// terrain surface, which deviates from the gameplay surface (map_Height) when
// terrain mesh subdivision is active. (x, y) is the object's map position in
// world units, z its gameplay height. Zero when subdivision is off, when the
// object is airborne, or when the surfaces agree. NEVER feed the result back
// into simulation state.
float getTerrainVisualObjectHeightDelta(WorldMapState& mapState, int x, int y, int z);

void initTerrainShaderType(); // must be called after the graphics context is initialized

#endif
