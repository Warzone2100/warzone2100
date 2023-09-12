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
	struct texture; // forward-declare
}

void pie_StartMeshes();
void pie_UpdateLightmap(gfx_api::texture* lightmapTexture, const glm::mat4& modelUVLightmapMatrix);
void pie_FinalizeMeshes(uint64_t currentGameFrame);
void pie_DrawAllMeshes(uint64_t currentGameFrame, const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix, const ShadowCascadesInfo& shadowMVPMatrix, bool depthPass);
