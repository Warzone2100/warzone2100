// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/
/** @file scene_prepass.cpp
 * Combined scene depth + view-space normals prepass for SSAO.
 */

#include "scene_prepass.h"

#include "display3d_render_graph.h"
#include "terrain.h"

#include "lib/ivis_opengl/piedraw.h"

void recordScenePrepass(const gfx_api::RenderPassContext& /*passCtx*/)
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}

	const auto& fc = pie_GetInGame3DFrameContext();
	const Vector3f cameraPos{fc.cameraPos.x, fc.cameraPos.y, fc.cameraPos.z};

	drawTerrainDepthNormalPrepass(fc.perspectiveViewMatrix, fc.viewMatrix);
	pie_DrawAllMeshes(fc.currentGameFrame, fc.perspectiveMatrix, fc.viewMatrix,
		cameraPos, fc.shadowCascadesInfo, nullptr, MeshDepthPassMode::ScenePrepass);
}
