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
/** @file display3d_render_internal.h
 * Internal in-game 3D helpers for post-process draws and UV scale/clamp.
 */

#pragma once

#include "lib/ivis_opengl/gfx_pipelines.h"

#include <glm/fwd.hpp>
#include <glm/vec4.hpp>

void display3d_renderSurroundings(const glm::mat4& projectionMatrix, const glm::mat4& skyboxViewMatrix);
void display3d_doConstructionLines(const glm::mat4& viewMatrix);
void display3d_locateMouse();
void display3d_drawWorldToScreenBlit(gfx_api::abstract_texture* sourceTexture);
void display3d_drawFsr1Easu(gfx_api::abstract_texture* sourceTexture);
void display3d_drawFsr1Rcas(gfx_api::abstract_texture* sourceTexture);
void display3d_drawSmaaEdges(gfx_api::abstract_texture* sourceTexture);
void display3d_drawSmaaWeights(gfx_api::abstract_texture* edgesTexture);
void display3d_drawSmaaBlend(gfx_api::abstract_texture* colorTexture, gfx_api::abstract_texture* weightsTexture);
void display3d_processSensorTarget();
void display3d_processDestinationTarget();
/// Fullscreen triangle VBO used by `display3d_drawFullscreenTriangle` (null before init3DView).
gfx_api::buffer* display3d_getScreenTriangleVBO();
/// Bind PSO + constants + textures and draw the shared fullscreen triangle.
template<typename PsoHelper, typename Constants, typename... Textures>
void display3d_drawFullscreenTriangle(const Constants& constants, Textures*... textures)
{
	gfx_api::buffer* vbo = display3d_getScreenTriangleVBO();
	if (vbo == nullptr)
	{
		return;
	}
	auto& pso = PsoHelper::get();
	pso.bind();
	pso.bind_constants(constants);
	pso.bind_vertex_buffers(vbo);
	pso.bind_textures(textures...);
	pso.draw(3, 0);
	pso.unbind_vertex_buffers(vbo);
}
/// Scale/clamp UVs from graph read `readIndex`'s producer pipeline surface.
void display3d_fillPassReadUvScaleClamp(const gfx_api::RenderPassContext& passCtx, size_t readIndex, glm::vec4& uvScaleClamp);
