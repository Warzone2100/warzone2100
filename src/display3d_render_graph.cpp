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
#include "scene_prepass.h"
#include "ssao.h"
#include "ssr.h"
#include "fog_pass.h"
#include "tonemap_pass.h"
#include "bloom_pass.h"
#include "range_rings.h"
#include "terrain.h"

#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piedraw.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/render_graph/cached_render_graph.h"
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

/// Scene draw pass reads are exclusively ordered ShadowCascade Depth outputs; all share one array texture.
static gfx_api::abstract_texture* shadowMapFromCascadeReads(const gfx_api::RenderPassContext& ctx)
{
	if (ctx.readCount() == 0)
	{
		return nullptr;
	}
	gfx_api::abstract_texture* shadowMap = ctx.getRead(0);
	for (size_t i = 0; i < ctx.readCount(); ++i)
	{
		const auto& read = ctx.resolvedRead(i);
		ASSERT(read.isDepth, "Scene draw read %zu must be Depth (cascade)", i);
		ASSERT(read.texture == shadowMap, "Scene draw cascade reads must share ShadowMap texture");
	}
	return shadowMap;
}

static void recordScenePass(const gfx_api::RenderPassContext& passCtx)
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}

	const auto& fc = pie_GetInGame3DFrameContext();
	const Vector3f cameraPos = toVector3f(fc.cameraPos);
	const Vector3f sunPos = toVector3f(-getTheSun());
	gfx_api::abstract_texture* shadowMap = shadowMapFromCascadeReads(passCtx);

	wzPerfBegin(PERF_TERRAIN, "3D scene - terrain");
	pie_SetFogStatus(true);
	drawTerrain(fc.perspectiveViewMatrix, fc.viewMatrix, cameraPos, sunPos, fc.shadowCascadesInfo, shadowMap, fc.pointLights);
	wzPerfEnd(PERF_TERRAIN);

	wzPerfBegin(PERF_SKYBOX, "3D scene - skybox");
	display3d_renderSurroundings(pie_SkyboxPerspectiveGet(), fc.baseViewMatrix);
	wzPerfEnd(PERF_SKYBOX);

	wzPerfBegin(PERF_WATER, "3D scene - water");
	pie_SetFogStatus(true);
	drawWater(fc.perspectiveViewMatrix, fc.viewMatrix, cameraPos, sunPos, fc.shadowCascadesInfo, shadowMap, fc.pointLights);
	wzPerfEnd(PERF_WATER);

	// When no post-effect is enabled, the blueprint emits no SceneTransparent pass and the transparents fuse back into this pass
	// (pre-separation behavior: no prepass cost, one pass roundtrip, and scene MSAA covers the transparents).
	// Key off the executing blueprint, so record and blueprint cannot disagree.
	const bool fusedTransparents =
		!pie_GetCachedFrameRenderGraph().blueprintContainsPass(gfx_api::PassId::SceneTransparent);

	wzPerfBegin(PERF_MODELS, "3D scene - models");
	{
		WZ_PROFILE_SCOPE(pie_DrawAllMeshes);
		// Opaque meshes keep per-mesh fog disabled either way (fogOutput is Disabled outside the transparent buckets),
		// so ForwardDistance in the fused case only fogs the transparent buckets, from fragment distance.
		pie_DrawAllMeshes(fc.currentGameFrame, fc.perspectiveMatrix, fc.viewMatrix, cameraPos,
			fc.shadowCascadesInfo, shadowMap, fc.pointLights, MeshDepthPassMode::None,
			fusedTransparents ? MeshDrawParts::All : MeshDrawParts::Opaque,
			fusedTransparents ? MeshFogMode::ForwardDistance : MeshFogMode::Disabled);
	}
	wzPerfEnd(PERF_MODELS);

	if (fusedTransparents && !gamePaused())
	{
		display3d_doConstructionLines(fc.viewMatrix);
	}

	display3d_locateMouse();
}

static void recordSceneTransparent(const gfx_api::RenderPassContext& passCtx)
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}

	const auto& fc = pie_GetInGame3DFrameContext();
	const Vector3f cameraPos = toVector3f(fc.cameraPos);
	gfx_api::abstract_texture* shadowMap = shadowMapFromCascadeReads(passCtx);

	{
		WZ_PROFILE_SCOPE(pie_DrawTransparentMeshes);
		// ScenePrepassDepth describes only the opaque background. Fogging the
		// composited result with it would assign that unrelated depth to every
		// transparent layer, so forward transparents fog from fragment distance.
		pie_DrawAllMeshes(fc.currentGameFrame, fc.perspectiveMatrix, fc.viewMatrix, cameraPos,
			fc.shadowCascadesInfo, shadowMap, fc.pointLights, MeshDepthPassMode::None,
			MeshDrawParts::Translucent | MeshDrawParts::Additive, MeshFogMode::ForwardDistance);
	}

	if (!gamePaused())
	{
		display3d_doConstructionLines(fc.viewMatrix);
	}
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
		drawTerrainDepthOnly(fc.cascadeProj[cascadeIndex] * fc.cascadeView[cascadeIndex], fc.perspectiveViewMatrix);
	}
	pie_DrawAllMeshes(fc.currentGameFrame, fc.cascadeProj[cascadeIndex], fc.cascadeView[cascadeIndex],
		cameraPos, fc.shadowCascadesInfo, nullptr, fc.pointLights, MeshDepthPassMode::ShadowMap);
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

static void recordSceneUpscaleEASU(const gfx_api::RenderPassContext& passCtx)
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}
	WZ_PROFILE_SCOPE(sceneUpscaleEASU);
	display3d_drawFsr1Easu(passCtx.getRead(0));
}

static void recordSceneUpscaleRCAS(const gfx_api::RenderPassContext& passCtx)
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}
	WZ_PROFILE_SCOPE(sceneUpscaleRCAS);
	display3d_drawFsr1Rcas(passCtx.getRead(0));
}

static void recordSmaaEdges(const gfx_api::RenderPassContext& passCtx)
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}
	WZ_PROFILE_SCOPE(smaaEdges);
	display3d_drawSmaaEdges(passCtx.getRead(0));
}

static void recordSmaaWeights(const gfx_api::RenderPassContext& passCtx)
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}
	WZ_PROFILE_SCOPE(smaaWeights);
	display3d_drawSmaaWeights(passCtx.getRead(0));
}

static void recordSmaaBlend(const gfx_api::RenderPassContext& passCtx)
{
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}
	WZ_PROFILE_SCOPE(smaaBlend);
	display3d_drawSmaaBlend(passCtx.getRead(0), passCtx.getRead(1));
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

void registerInGame3DPassPipelines(gfx_api::PassPipelineTable& table)
{
	using Out = std::vector<gfx_api::PipelinePrewarmFn>;
	using Context = gfx_api::PassPipelineContext;
	// One pipeline, whatever the options
	auto one = [&table](gfx_api::PassId id, gfx_api::PipelinePrewarmFn fn) {
		table.set(id, [fn](const Context&, Out& out) { out.push_back(fn); });
	};

	// The world (as the record callbacks above, and scene_prepass.cpp, draw it)
	for (uint32_t i = 0; i < WZ_MAX_SHADOW_CASCADES; ++i)
	{
		table.set(gfx_api::shadowCascadePassId(i), [](const Context&, Out& out) {
			if (getDrawTerrainShadows())
			{
				terrain_DeclareShadowPipelines(out);
			}
			pie_DeclareMeshPipelines(MeshDepthPassMode::ShadowMap, MeshDrawParts::All, out);
		});
	}
	table.set(gfx_api::PassId::ScenePrepass, [](const Context& ctx, Out& out) {
		// (with a normals target, or depth only: as the pass is built)
		const bool normals = !ctx.pass.colorAttachments.empty();
		terrain_DeclarePrepassPipelines(normals, gfx_api::context::get().storedSceneEffectSurfaces().ssr, out);
		pie_DeclareMeshPipelines(normals ? MeshDepthPassMode::ScenePrepass : MeshDepthPassMode::ScenePrepassDepthOnly, MeshDrawParts::All, out);
	});
	table.set(gfx_api::PassId::ScenePass, [](const Context& ctx, Out& out) {
		terrain_DeclareScenePipelines(out);
		out.push_back(&gfx_api::SkyboxPSO::prewarm);
		// (without a SceneTransparent pass, the transparent parts and the construction beams are drawn here too)
		const bool fusedTransparents = !ctx.graphContains(gfx_api::PassId::SceneTransparent);
		pie_DeclareMeshPipelines(MeshDepthPassMode::None, fusedTransparents ? MeshDrawParts::All : MeshDrawParts::Opaque, out);
		if (fusedTransparents)
		{
			out.push_back(&gfx_api::TransColouredTrianglePSO::prewarm);
		}
	});
	table.set(gfx_api::PassId::SceneTransparent, [](const Context&, Out& out) {
		pie_DeclareMeshPipelines(MeshDepthPassMode::None, MeshDrawParts::Translucent | MeshDrawParts::Additive, out);
		out.push_back(&gfx_api::TransColouredTrianglePSO::prewarm);
	});

	// The post effects
	one(gfx_api::PassId::SSAOGenerate, &gfx_api::SSAOGeneratePSO::prewarm);
	one(gfx_api::PassId::SSAODownsample, &gfx_api::BilinearResamplePSO::prewarm);
	one(gfx_api::PassId::SSAOBlurH, &gfx_api::SSAOBlurPSO::prewarm);
	one(gfx_api::PassId::SSAOBlurV, &gfx_api::SSAOBlurPSO::prewarm);
	one(gfx_api::PassId::SSAOCompose, &gfx_api::SceneComposeSSAOPSO::prewarm);
	one(gfx_api::PassId::SSRGenerate, &gfx_api::SSRGeneratePSO::prewarm);
	one(gfx_api::PassId::SSRDownsample, &gfx_api::BilinearResamplePSO::prewarm);
	one(gfx_api::PassId::SSRBlurH, &gfx_api::SSRBlurPSO::prewarm);
	one(gfx_api::PassId::SSRBlurV, &gfx_api::SSRBlurPSO::prewarm);
	one(gfx_api::PassId::SSRCompose, &gfx_api::SceneComposeSSRPSO::prewarm);
	one(gfx_api::PassId::FogApply, &gfx_api::SceneFogPSO::prewarm);
	one(gfx_api::PassId::RangeRingSdfSensor, &gfx_api::RangeRingSdfSensorPSO::prewarm);
	one(gfx_api::PassId::RangeRingSdfWeapon, &gfx_api::RangeRingSdfWeaponPSO::prewarm);
	one(gfx_api::PassId::RangeRingSdfMin, &gfx_api::RangeRingSdfMinPSO::prewarm);
	one(gfx_api::PassId::RangeRingComposite, &gfx_api::RangeRingCompositePSO::prewarm);
	for (gfx_api::PassId id : {gfx_api::PassId::BloomDownsample0, gfx_api::PassId::BloomDownsample1, gfx_api::PassId::BloomDownsample2,
		gfx_api::PassId::BloomDownsample3, gfx_api::PassId::BloomDownsample4, gfx_api::PassId::BloomDownsample5})
	{
		one(id, &gfx_api::BloomDownsamplePSO::prewarm);
	}
	for (gfx_api::PassId id : {gfx_api::PassId::BloomUpsample0, gfx_api::PassId::BloomUpsample1, gfx_api::PassId::BloomUpsample2,
		gfx_api::PassId::BloomUpsample3, gfx_api::PassId::BloomUpsample4})
	{
		one(id, &gfx_api::BloomUpsamplePSO::prewarm);
	}
	one(gfx_api::PassId::SceneTonemap, &gfx_api::SceneTonemapPSO::prewarm);
	one(gfx_api::PassId::SceneBlit, &gfx_api::WorldToScreenPSO::prewarm);
	one(gfx_api::PassId::SceneUpscaleEASU, &gfx_api::Fsr1EasuPSO::prewarm);
	one(gfx_api::PassId::SceneUpscaleRCAS, &gfx_api::Fsr1RcasPSO::prewarm);
	one(gfx_api::PassId::SmaaEdges, &gfx_api::SmaaEdgesPSO::prewarm);
	one(gfx_api::PassId::SmaaWeights, &gfx_api::SmaaWeightsPSO::prewarm);
	one(gfx_api::PassId::SmaaBlend, &gfx_api::SmaaBlendPSO::prewarm);

	// The overlays on the world: target markers, selections, the drag box, labels (interface drawing)
	table.set(gfx_api::PassId::TargettingEffects, [](const Context&, Out& out) {
		pie_DeclareInterfacePipelines(out);
		out.push_back(&gfx_api::TransColouredTrianglePSO::prewarm);
	});
	table.set(gfx_api::PassId::SceneOverlays, [](const Context&, Out& out) { pie_DeclareInterfacePipelines(out); });
	// (SceneDebugOverlays: debug drawing, left to build as it is turned on)
}

void registerInGame3DRecordFuncs(gfx_api::RecordFuncTable& table)
{
	table.set(gfx_api::PassId::ScenePrepass, recordScenePrepass);
	table.set(gfx_api::PassId::ScenePass, recordScenePass);
	table.set(gfx_api::PassId::SceneTransparent, recordSceneTransparent);
	table.set(gfx_api::PassId::SSAOGenerate, ssao::recordGenerate);
	table.set(gfx_api::PassId::SSAODownsample, ssao::recordDownsample);
	table.set(gfx_api::PassId::SSAOBlurH, ssao::recordBlurH);
	table.set(gfx_api::PassId::SSAOBlurV, ssao::recordBlurV);
	table.set(gfx_api::PassId::SSAOCompose, ssao::recordCompose);
	table.set(gfx_api::PassId::SSRGenerate, ssr::recordGenerate);
	table.set(gfx_api::PassId::SSRDownsample, ssr::recordDownsample);
	table.set(gfx_api::PassId::SSRBlurH, ssr::recordBlurH);
	table.set(gfx_api::PassId::SSRBlurV, ssr::recordBlurV);
	table.set(gfx_api::PassId::SSRCompose, ssr::recordCompose);
	table.set(gfx_api::PassId::FogApply, fog_pass::recordApply);
	for (gfx_api::PassId id : {gfx_api::PassId::BloomDownsample0, gfx_api::PassId::BloomDownsample1, gfx_api::PassId::BloomDownsample2,
		gfx_api::PassId::BloomDownsample3, gfx_api::PassId::BloomDownsample4, gfx_api::PassId::BloomDownsample5})
	{
		table.set(id, bloom_pass::recordDownsample);
	}
	for (gfx_api::PassId id : {gfx_api::PassId::BloomUpsample0, gfx_api::PassId::BloomUpsample1, gfx_api::PassId::BloomUpsample2,
		gfx_api::PassId::BloomUpsample3, gfx_api::PassId::BloomUpsample4})
	{
		table.set(id, bloom_pass::recordUpsample);
	}
	table.set(gfx_api::PassId::SceneTonemap, tonemap_pass::recordApply);
	table.set(gfx_api::PassId::RangeRingSdfSensor, range_rings::recordSdfSensor);
	table.set(gfx_api::PassId::RangeRingSdfWeapon, range_rings::recordSdfWeapon);
	table.set(gfx_api::PassId::RangeRingSdfMin, range_rings::recordSdfMin);
	table.set(gfx_api::PassId::RangeRingComposite, range_rings::recordComposite);
	table.set(gfx_api::PassId::SceneBlit, recordSceneBlit);
	table.set(gfx_api::PassId::SceneUpscaleEASU, recordSceneUpscaleEASU);
	table.set(gfx_api::PassId::SceneUpscaleRCAS, recordSceneUpscaleRCAS);
	table.set(gfx_api::PassId::SmaaEdges, recordSmaaEdges);
	table.set(gfx_api::PassId::SmaaWeights, recordSmaaWeights);
	table.set(gfx_api::PassId::SmaaBlend, recordSmaaBlend);
	table.set(gfx_api::PassId::TargettingEffects, recordTargettingEffects);
	table.set(gfx_api::PassId::SceneOverlays, display3d_recordSceneOverlays);
	table.set(gfx_api::PassId::SceneDebugOverlays, display3d_recordSceneDebugOverlays);

	for (uint32_t i = 0; i < WZ_MAX_SHADOW_CASCADES; ++i)
	{
		table.set(gfx_api::shadowCascadePassId(i), recordShadowCascade);
	}
}
