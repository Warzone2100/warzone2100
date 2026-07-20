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
/** @file display3d_render_graph.cpp
 * In-game 3D render-graph record callbacks and per-frame draw context binding.
 */

#include "lib/ivis_opengl/pietypes.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

#include "display3d_render_graph.h"

#include "display.h"
#include "display3d_render_internal.h"
#include "lighting.h"
#include "loop.h"
#include "profiling.h"
#include "terrain.h"

#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/piedraw.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/render_graph/render_pass_id.h"
#include "lib/ivis_opengl/shadows.h"

#include "lib/framework/wzapp.h"

static InGame3DFrameContext* g_boundCtx = nullptr;
static InGame3DFrameContext g_defaultCtx;
static bool g_frameContextReady = false;

InGame3DFrameContext& pie_GetInGame3DFrameContext()
{
	return g_boundCtx ? *g_boundCtx : g_defaultCtx;
}

void pie_BindInGame3DFrameContext(InGame3DFrameContext* ctx)
{
	g_boundCtx = ctx;
	g_frameContextReady = (ctx != nullptr);
}

void pie_ResetInGame3DFrameContextForFrame()
{
	g_boundCtx = nullptr;
	g_frameContextReady = false;
}

bool pie_IsInGame3DFrameContextReady()
{
	return g_frameContextReady;
}

static Vector3f toVector3f(const glm::vec3& v)
{
	return Vector3f(v.x, v.y, v.z);
}

static void recordScenePass(const gfx_api::RenderPassContext&)
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}
	const auto& fc = pie_GetInGame3DFrameContext();
	const Vector3f cameraPos = toVector3f(fc.cameraPos);
	const Vector3f sunPos = toVector3f(-getTheSun());

	wzPerfBegin(PERF_TERRAIN, "3D scene - terrain");
	pie_SetFogStatus(true);
	drawTerrain(fc.perspectiveViewMatrix, fc.viewMatrix, cameraPos, sunPos, fc.shadowCascadesInfo);
	wzPerfEnd(PERF_TERRAIN);

	wzPerfBegin(PERF_SKYBOX, "3D scene - skybox");
	display3d_renderSurroundings(pie_SkyboxPerspectiveGet(), fc.baseViewMatrix);
	wzPerfEnd(PERF_SKYBOX);

	wzPerfBegin(PERF_WATER, "3D scene - water");
	pie_SetFogStatus(true);
	drawWater(fc.perspectiveViewMatrix, fc.viewMatrix, cameraPos, sunPos, fc.shadowCascadesInfo);
	wzPerfEnd(PERF_WATER);

	wzPerfBegin(PERF_MODELS, "3D scene - models");
	{
		WZ_PROFILE_SCOPE(pie_DrawAllMeshes);
		pie_DrawAllMeshes(fc.currentGameFrame, fc.perspectiveMatrix, fc.viewMatrix, cameraPos, fc.shadowCascadesInfo, false);
	}
	wzPerfEnd(PERF_MODELS);

	if (!gamePaused())
	{
		display3d_doConstructionLines(fc.viewMatrix);
	}
	display3d_locateMouse();
}

static void recordShadowCascade(const gfx_api::RenderPassContext& passCtx)
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}
	const auto& fc = pie_GetInGame3DFrameContext();
	const uint32_t cascadeIndex = gfx_api::passIdToCascadeIndex(passCtx.passId());
	if (cascadeIndex >= WZ_MAX_SHADOW_CASCADES)
	{
		return;
	}

	const Vector3f cameraPos = toVector3f(fc.cameraPos);
	if (getDrawTerrainShadows())
	{
		drawTerrainDepthOnly(fc.cascadeProj[cascadeIndex] * fc.cascadeView[cascadeIndex]);
	}
	pie_DrawAllMeshes(fc.currentGameFrame, fc.cascadeProj[cascadeIndex], fc.cascadeView[cascadeIndex],
		cameraPos, fc.shadowCascadesInfo, true);
}

static void recordSceneBlit(const gfx_api::RenderPassContext& passCtx)
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}
	WZ_PROFILE_SCOPE(copyToFBO);
	display3d_drawWorldToScreenBlit(passCtx.getRead(0));
}

static void recordTargettingEffects(const gfx_api::RenderPassContext&)
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}
	display3d_processSensorTarget();
	display3d_processDestinationTarget();
}

void registerInGame3DRecordFuncs(gfx_api::RecordFuncTable& table)
{
	table.set(gfx_api::PassId::ScenePass, recordScenePass);
	table.set(gfx_api::PassId::SceneBlit, recordSceneBlit);
	table.set(gfx_api::PassId::TargettingEffects, recordTargettingEffects);
	table.set(gfx_api::PassId::SceneOverlays, display3d_recordSceneOverlays);
	table.set(gfx_api::PassId::SceneDebugOverlays, display3d_recordSceneDebugOverlays);

	for (uint32_t i = 0; i < WZ_MAX_SHADOW_CASCADES; ++i)
	{
		table.set(gfx_api::shadowCascadePassId(i), recordShadowCascade);
	}
}
