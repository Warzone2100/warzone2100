// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** @file display3d_render_graph.h
 * In-game 3D render-graph record callbacks and per-frame draw context binding.
 */

#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif

#include "lib/ivis_opengl/render_graph/blueprint.h"
#include "lib/ivis_opengl/shadows.h"

#include <array>
#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

struct InGame3DFrameContext
{
	glm::mat4 perspectiveViewMatrix{};
	glm::mat4 viewMatrix{};
	glm::mat4 baseViewMatrix{};
	glm::mat4 perspectiveMatrix{};
	glm::vec3 cameraPos{};
	ShadowCascadesInfo shadowCascadesInfo{};
	std::array<glm::mat4, WZ_MAX_SHADOW_CASCADES> cascadeProj{};
	std::array<glm::mat4, WZ_MAX_SHADOW_CASCADES> cascadeView{};
	uint32_t currentGameFrame = 0;
};

InGame3DFrameContext& pie_GetInGame3DFrameContext();
void pie_BindInGame3DFrameContext(InGame3DFrameContext* ctx);
void pie_ResetInGame3DFrameContextForFrame();
bool pie_IsInGame3DFrameContextReady();

void display3d_recordSceneOverlays(const gfx_api::RenderPassContext& passCtx);
void display3d_recordSceneDebugOverlays(const gfx_api::RenderPassContext& passCtx);

void registerInGame3DRecordFuncs(gfx_api::RecordFuncTable& table);
