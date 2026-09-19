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
/** @file scene_prepass.cpp
 * Scene prepass for SSAO, deferred fog, and forward transparents:
 * depth + view-space normals when needed (ex: SSAO), depth alone otherwise.
 */

#include "scene_prepass.h"

#include "display3d_render_graph.h"
#include "terrain.h"

#include "lib/ivis_opengl/piedraw.h"

void recordScenePrepass(const gfx_api::RenderPassContext& passCtx)
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}

	const auto& fc = pie_GetInGame3DFrameContext();
	const Vector3f cameraPos{fc.cameraPos.x, fc.cameraPos.y, fc.cameraPos.z};

	// The blueprint attaches the normals color target only when an enabled post-effect needs it (PrepassNeed::Normals, i.e. SSAO).
	// Follow the pass's actual attachments rather than re-deriving the config, so record and blueprint can never disagree:
	// with no color attachment bound, rasterize depth alone through the depth-only PSOs (empty fragment stage, no normals
	// bandwidth - many GPUs rasterize depth-only at increased rate).
	const bool writesNormals = passCtx.writeCount() > 0;

	if (writesNormals)
	{
		drawTerrainDepthNormalPrepass(fc.perspectiveViewMatrix, fc.viewMatrix);
		drawWaterDepthNormalPrepass(fc.perspectiveMatrix, fc.viewMatrix);
	}
	else
	{
		drawTerrainDepthOnlyPrepass(fc.perspectiveViewMatrix, fc.viewMatrix);
		drawWaterDepthOnlyPrepass(fc.perspectiveMatrix, fc.viewMatrix);
	}
	pie_DrawAllMeshes(fc.currentGameFrame, fc.perspectiveMatrix, fc.viewMatrix,
		cameraPos, fc.shadowCascadesInfo, nullptr, fc.pointLights,
		writesNormals ? MeshDepthPassMode::ScenePrepass : MeshDepthPassMode::ScenePrepassDepthOnly);
}
