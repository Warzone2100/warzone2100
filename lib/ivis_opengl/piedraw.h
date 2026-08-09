/*
	This file is part of Warzone 2100.
	Copyright (C) 2023  Warzone 2100 Project

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

#pragma once

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include <glm/mat4x4.hpp>
#include "pietypes.h"
#include "shadows.h"

namespace gfx_api
{
	struct abstract_texture; // forward-declare
	struct texture; // forward-declare
}

/// How pie_DrawAllMeshes interprets the opaque bucket / depth-only PSO selection.
enum class MeshDepthPassMode
{
	None,         ///< Normal lit scene draw
	ShadowMap,    ///< Shadow-casting shapes only, shadow depth PSO
	ScenePrepass, ///< All opaque shapes, depth+normal prepass PSO
};

void pie_StartMeshes();
void pie_UpdateLightmap(gfx_api::texture* lightmapTexture, const glm::mat4& modelUVLightmapMatrix);
void pie_FinalizeMeshes(uint64_t currentGameFrame);
void pie_DrawAllMeshes(uint64_t currentGameFrame, const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix, const Vector3f &cameraPos, const ShadowCascadesInfo& shadowMVPMatrix, gfx_api::abstract_texture* shadowMap, MeshDepthPassMode depthPassMode = MeshDepthPassMode::None);
