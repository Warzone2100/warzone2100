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

// Golden PassId sequences from PassGraphTopologyBlueprint::fromSnapshot (no GPU).
// Build and run via CMake with -DWZ_BUILD_RENDER_GRAPH_TEST=ON
// (target: render_graph_pass_sequence_test). Exits nonzero on failure.

#include "lib/framework/wzapp.h"
#include "render_graph/blueprint.h"
#include "render_graph/topology.h"

#include <cstdio>
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

// framework/debug.cpp references these SDL-backend symbols. Stubs keep the
// test free of GPU/window setup (same idea as tests/framework_linktest.cpp).
bool wzIsFullscreen()
{
	return false;
}

bool wzChangeWindowMode(WINDOW_MODE, bool)
{
	return true;
}

void wzDisplayDialog(DialogType, const char*, const char*)
{
}

using gfx_api::PassId;
using gfx_api::RenderFeatures;
using gfx_api::RenderScreenKind;
using gfx_api::RenderTopologySnapshot;

static int failures = 0;
static int checks = 0;

static const char* passIdName(PassId id)
{
	switch (id)
	{
	case PassId::Backdrop: return "Backdrop";
	case PassId::ShadowCascade0: return "ShadowCascade0";
	case PassId::ShadowCascade1: return "ShadowCascade1";
	case PassId::ShadowCascade2: return "ShadowCascade2";
	case PassId::ShadowCascade3: return "ShadowCascade3";
	case PassId::ScenePrepass: return "ScenePrepass";
	case PassId::ScenePass: return "ScenePass";
	case PassId::SceneTransparent: return "SceneTransparent";
	case PassId::SceneTonemap: return "SceneTonemap";
	case PassId::BloomDownsample0: return "BloomDownsample0";
	case PassId::BloomDownsample1: return "BloomDownsample1";
	case PassId::BloomDownsample2: return "BloomDownsample2";
	case PassId::BloomDownsample3: return "BloomDownsample3";
	case PassId::BloomDownsample4: return "BloomDownsample4";
	case PassId::BloomDownsample5: return "BloomDownsample5";
	case PassId::BloomUpsample0: return "BloomUpsample0";
	case PassId::BloomUpsample1: return "BloomUpsample1";
	case PassId::BloomUpsample2: return "BloomUpsample2";
	case PassId::BloomUpsample3: return "BloomUpsample3";
	case PassId::BloomUpsample4: return "BloomUpsample4";
	case PassId::BloomComposite: return "BloomComposite";
	case PassId::SSAOGenerate: return "SSAOGenerate";
	case PassId::SSAODownsample: return "SSAODownsample";
	case PassId::SSAOBlurH: return "SSAOBlurH";
	case PassId::SSAOBlurV: return "SSAOBlurV";
	case PassId::SSAOCompose: return "SSAOCompose";
	case PassId::SSRGenerate: return "SSRGenerate";
	case PassId::SSRDownsample: return "SSRDownsample";
	case PassId::SSRBlurH: return "SSRBlurH";
	case PassId::SSRBlurV: return "SSRBlurV";
	case PassId::SSRCompose: return "SSRCompose";
	case PassId::FogApply: return "FogApply";
	case PassId::RangeRingSdfSensor: return "RangeRingSdfSensor";
	case PassId::RangeRingSdfWeapon: return "RangeRingSdfWeapon";
	case PassId::RangeRingSdfMin: return "RangeRingSdfMin";
	case PassId::RangeRingComposite: return "RangeRingComposite";
	case PassId::SceneBlit: return "SceneBlit";
	case PassId::SceneUpscaleEASU: return "SceneUpscaleEASU";
	case PassId::SceneUpscaleRCAS: return "SceneUpscaleRCAS";
	case PassId::SmaaEdges: return "SmaaEdges";
	case PassId::SmaaWeights: return "SmaaWeights";
	case PassId::SmaaBlend: return "SmaaBlend";
	case PassId::TargettingEffects: return "TargettingEffects";
	case PassId::SceneOverlays: return "SceneOverlays";
	case PassId::SceneDebugOverlays: return "SceneDebugOverlays";
	case PassId::GameStartFade: return "GameStartFade";
	case PassId::InGameUI: return "InGameUI";
	case PassId::TitleUI: return "TitleUI";
	case PassId::LoadingBackdrop: return "LoadingBackdrop";
	case PassId::LoadingScreen: return "LoadingScreen";
	case PassId::VideoPlayback: return "VideoPlayback";
	case PassId::Count: return "Count";
	}
	return "?";
}

static std::string formatPassIds(const std::vector<PassId>& ids)
{
	std::stringstream strm;
	for (size_t i = 0; i < ids.size(); ++i)
	{
		if (i != 0)
		{
			strm << ", ";
		}
		strm << passIdName(ids[i]);
	}
	return strm.str();
}

static RenderTopologySnapshot inGameBase()
{
	RenderTopologySnapshot snapshot;
	snapshot.screenKind = RenderScreenKind::InGame;
	// Matches render_topology::snapshot() for a live in-game query:
	// GameStartFadeSlot is always set; GameRenderTopologyQuery always enables debug overlays.
	snapshot.features = RenderFeatures::GameStartFadeSlot | RenderFeatures::DebugOverlays;
	snapshot.drawableW = 1920;
	snapshot.drawableH = 1080;
	snapshot.sceneW = 1920;
	snapshot.sceneH = 1080;
	return snapshot;
}

static void expectPassIds(const char* name, const RenderTopologySnapshot& snapshot, const std::vector<PassId>& expected)
{
	++checks;
	const gfx_api::PassGraphTopologyBlueprint blueprint = gfx_api::PassGraphTopologyBlueprint::fromSnapshot(snapshot);
	const std::vector<PassId> actual = gfx_api::blueprintPassIds(blueprint);
	if (actual == expected)
	{
		return;
	}
	++failures;
	std::printf("FAIL %s\n  expected: [%s]\n  actual:   [%s]\n",
		name, formatPassIds(expected).c_str(), formatPassIds(actual).c_str());
}

static void expectPassIds(const char* name, const RenderTopologySnapshot& snapshot, std::initializer_list<PassId> expected)
{
	expectPassIds(name, snapshot, std::vector<PassId>(expected));
}

static std::vector<PassId> withInGameTail(std::initializer_list<PassId> head)
{
	std::vector<PassId> ids(head);
	ids.insert(ids.end(), {
		PassId::TargettingEffects,
		PassId::SceneOverlays,
		PassId::SceneDebugOverlays,
		PassId::GameStartFade,
		PassId::InGameUI,
	});
	return ids;
}

static void expectInGame(const char* name, const RenderTopologySnapshot& snapshot, std::initializer_list<PassId> head)
{
	expectPassIds(name, snapshot, withInGameTail(head));
}

static const gfx_api::BlueprintPass* findPass(const gfx_api::PassGraphTopologyBlueprint& blueprint, PassId id)
{
	for (const auto& pass : blueprint.passes())
	{
		if (pass.id == id)
		{
			return &pass;
		}
	}
	return nullptr;
}

/// The pass exists, writes `surface` with `load` as its first color attachment, and reads exactly `producer`
static void expectPassWiring(const char* name, const gfx_api::PassGraphTopologyBlueprint& blueprint, PassId id,
	gfx_api::PipelineSurfaceId surface, gfx_api::AttachmentLoadOp load, PassId producer)
{
	++checks;
	const gfx_api::BlueprintPass* pass = findPass(blueprint, id);
	const bool ok = pass != nullptr
		&& !pass->colorAttachments.empty()
		&& pass->colorAttachments[0].surfaceId == surface
		&& pass->colorAttachments[0].loadOp == load
		&& pass->reads.size() == 1
		&& pass->reads[0].producerPass == producer;
	if (!ok)
	{
		++failures;
		std::printf("FAIL %s: %s wiring\n", name, passIdName(id));
	}
}

static const std::vector<PassId>& bloomPasses()
{
	static const std::vector<PassId> passes = {
		PassId::BloomDownsample0, PassId::BloomDownsample1, PassId::BloomDownsample2,
		PassId::BloomDownsample3, PassId::BloomDownsample4, PassId::BloomDownsample5,
		PassId::BloomUpsample4, PassId::BloomUpsample3, PassId::BloomUpsample2,
		PassId::BloomUpsample1, PassId::BloomUpsample0, PassId::BloomComposite,
	};
	return passes;
}

static std::vector<PassId> withBloom(std::vector<PassId> before, std::vector<PassId> after)
{
	before.insert(before.end(), bloomPasses().begin(), bloomPasses().end());
	before.insert(before.end(), after.begin(), after.end());
	return before;
}

static void expectInGame(const char* name, const RenderTopologySnapshot& snapshot, std::vector<PassId> head)
{
	head.insert(head.end(), {
		PassId::TargettingEffects,
		PassId::SceneOverlays,
		PassId::SceneDebugOverlays,
		PassId::GameStartFade,
		PassId::InGameUI,
	});
	expectPassIds(name, snapshot, head);
}

int main()
{
	expectInGame("bare", inGameBase(), {PassId::ScenePass, PassId::SceneBlit});

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.fog = true;
		expectInGame("fog-only", snapshot, {
			PassId::ScenePrepass, PassId::ScenePass, PassId::FogApply,
			PassId::SceneTransparent, PassId::SceneBlit});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.features |= RenderFeatures::HdrScene;
		expectInGame("hdr-no-effects", snapshot, {
			PassId::ScenePass, PassId::SceneTonemap, PassId::SceneBlit});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.fog = true;
		snapshot.features |= RenderFeatures::HdrScene;
		expectInGame("hdr-fog", snapshot, {
			PassId::ScenePrepass, PassId::ScenePass, PassId::FogApply,
			PassId::SceneTransparent, PassId::SceneTonemap, PassId::SceneBlit});
	}

	{
		// Without an HDR scene, bloom changes nothing
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.bloom = true;
		expectInGame("bloom-without-hdr", snapshot, {PassId::ScenePass, PassId::SceneBlit});
	}

	{
		// Bloom alone needs neither the prepass nor the transparent split
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.bloom = true;
		snapshot.features |= RenderFeatures::HdrScene;
		expectInGame("hdr-bloom", snapshot, withBloom({PassId::ScenePass}, {PassId::SceneTonemap, PassId::SceneBlit}));

		const auto blueprint = gfx_api::PassGraphTopologyBlueprint::fromSnapshot(snapshot);
		using gfx_api::AttachmentLoadOp;
		using gfx_api::PipelineSurfaceId;
		expectPassWiring("hdr-bloom", blueprint, PassId::BloomDownsample0, PipelineSurfaceId::BloomMip0, AttachmentLoadOp::DontCare, PassId::ScenePass);
		expectPassWiring("hdr-bloom", blueprint, PassId::BloomDownsample5, PipelineSurfaceId::BloomMip5, AttachmentLoadOp::DontCare, PassId::BloomDownsample4);
		expectPassWiring("hdr-bloom", blueprint, PassId::BloomUpsample4, PipelineSurfaceId::BloomMip4, AttachmentLoadOp::Load, PassId::BloomDownsample5);
		expectPassWiring("hdr-bloom", blueprint, PassId::BloomUpsample0, PipelineSurfaceId::BloomMip0, AttachmentLoadOp::Load, PassId::BloomUpsample1);
		expectPassWiring("hdr-bloom", blueprint, PassId::BloomComposite, PipelineSurfaceId::SceneColor, AttachmentLoadOp::Load, PassId::BloomUpsample0);
		expectPassWiring("hdr-bloom", blueprint, PassId::SceneTonemap, PipelineSurfaceId::TonemapColor, AttachmentLoadOp::DontCare, PassId::BloomComposite);
	}

	{
		// After the transparents, onto the last post effect's output
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.fog = true;
		snapshot.sceneEffects.bloom = true;
		snapshot.features |= RenderFeatures::HdrScene;
		expectInGame("hdr-fog-bloom", snapshot, withBloom({PassId::ScenePrepass, PassId::ScenePass, PassId::FogApply,
			PassId::SceneTransparent}, {PassId::SceneTonemap, PassId::SceneBlit}));

		const auto blueprint = gfx_api::PassGraphTopologyBlueprint::fromSnapshot(snapshot);
		expectPassWiring("hdr-fog-bloom", blueprint, PassId::BloomDownsample0, gfx_api::PipelineSurfaceId::BloomMip0,
			gfx_api::AttachmentLoadOp::DontCare, PassId::SceneTransparent);
		expectPassWiring("hdr-fog-bloom", blueprint, PassId::BloomComposite, gfx_api::PipelineSurfaceId::FogColor,
			gfx_api::AttachmentLoadOp::Load, PassId::BloomUpsample0);
	}

	{
		// Scene MSAA: bloom reads and writes the resolved scene
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneMsaa = true;
		snapshot.sceneEffects.bloom = true;
		snapshot.features |= RenderFeatures::HdrScene;
		expectInGame("hdr-bloom-msaa", snapshot, withBloom({PassId::ScenePass}, {PassId::SceneTonemap, PassId::SceneBlit}));
	}

	{
		// The presentation tail is unchanged (SMAA and upscaling work on the tonemapped scene)
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.bloom = true;
		snapshot.features |= RenderFeatures::HdrScene | RenderFeatures::Smaa;
		expectInGame("hdr-bloom-smaa", snapshot, withBloom({PassId::ScenePass},
			{PassId::SceneTonemap, PassId::SmaaEdges, PassId::SmaaWeights, PassId::SmaaBlend}));
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.rangeRings = true;
		expectInGame("rings-only", snapshot, {
			PassId::ScenePrepass, PassId::ScenePass,
			PassId::RangeRingSdfSensor, PassId::RangeRingSdfWeapon, PassId::RangeRingSdfMin,
			PassId::RangeRingComposite, PassId::SceneTransparent, PassId::SceneBlit});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.ssao = true;
		expectInGame("ssao", snapshot, {
			PassId::ScenePrepass, PassId::ScenePass,
			PassId::SSAOGenerate, PassId::SSAOBlurH, PassId::SSAOBlurV, PassId::SSAOCompose,
			PassId::SceneTransparent, PassId::SceneBlit});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.ssao = true;
		snapshot.features |= RenderFeatures::SSAODownsample;
		expectInGame("ssao-downsample", snapshot, {
			PassId::ScenePrepass, PassId::ScenePass,
			PassId::SSAOGenerate, PassId::SSAODownsample, PassId::SSAOBlurH, PassId::SSAOBlurV,
			PassId::SSAOCompose, PassId::SceneTransparent, PassId::SceneBlit});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.ssr = true;
		expectInGame("ssr", snapshot, {
			PassId::ScenePrepass, PassId::ScenePass,
			PassId::SSRGenerate, PassId::SSRBlurH, PassId::SSRBlurV, PassId::SSRCompose,
			PassId::SceneTransparent, PassId::SceneBlit});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.ssr = true;
		snapshot.features |= RenderFeatures::SSRDownsample;
		expectInGame("ssr-downsample", snapshot, {
			PassId::ScenePrepass, PassId::ScenePass,
			PassId::SSRGenerate, PassId::SSRDownsample, PassId::SSRBlurH, PassId::SSRBlurV,
			PassId::SSRCompose, PassId::SceneTransparent, PassId::SceneBlit});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.ssao = true;
		snapshot.sceneEffects.ssr = true;
		expectInGame("ssao-ssr", snapshot, {
			PassId::ScenePrepass, PassId::ScenePass,
			PassId::SSAOGenerate, PassId::SSAOBlurH, PassId::SSAOBlurV, PassId::SSAOCompose,
			PassId::SSRGenerate, PassId::SSRBlurH, PassId::SSRBlurV, PassId::SSRCompose,
			PassId::SceneTransparent, PassId::SceneBlit});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.ssao = true;
		snapshot.sceneEffects.fog = true;
		snapshot.sceneEffects.rangeRings = true;
		expectInGame("ssao-fog-rings", snapshot, {
			PassId::ScenePrepass, PassId::ScenePass,
			PassId::SSAOGenerate, PassId::SSAOBlurH, PassId::SSAOBlurV, PassId::SSAOCompose,
			PassId::FogApply,
			PassId::RangeRingSdfSensor, PassId::RangeRingSdfWeapon, PassId::RangeRingSdfMin,
			PassId::RangeRingComposite, PassId::SceneTransparent, PassId::SceneBlit});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.ssao = true;
		snapshot.sceneEffects.ssr = true;
		snapshot.sceneEffects.fog = true;
		snapshot.sceneEffects.rangeRings = true;
		snapshot.features |= RenderFeatures::SSAODownsample | RenderFeatures::SSRDownsample;
		expectInGame("all-scene-effects-downsampled", snapshot, {
			PassId::ScenePrepass, PassId::ScenePass,
			PassId::SSAOGenerate, PassId::SSAODownsample, PassId::SSAOBlurH, PassId::SSAOBlurV,
			PassId::SSAOCompose,
			PassId::SSRGenerate, PassId::SSRDownsample, PassId::SSRBlurH, PassId::SSRBlurV,
			PassId::SSRCompose, PassId::FogApply,
			PassId::RangeRingSdfSensor, PassId::RangeRingSdfWeapon, PassId::RangeRingSdfMin,
			PassId::RangeRingComposite, PassId::SceneTransparent, PassId::SceneBlit});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.features |= RenderFeatures::Smaa;
		expectInGame("smaa", snapshot, {
			PassId::ScenePass, PassId::SmaaEdges, PassId::SmaaWeights, PassId::SmaaBlend});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.features |= RenderFeatures::Smaa | RenderFeatures::SmaaIntermediate;
		expectInGame("smaa-intermediate", snapshot, {
			PassId::ScenePass, PassId::SmaaEdges, PassId::SmaaWeights, PassId::SmaaBlend,
			PassId::SceneBlit});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.features |= RenderFeatures::FrozenWorldOverlay;
		expectPassIds("frozen", snapshot, {PassId::InGameUI});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.features |= RenderFeatures::FrozenWorldOverlay | RenderFeatures::Backdrop;
		expectPassIds("frozen-backdrop", snapshot, {PassId::Backdrop, PassId::InGameUI});
	}

	{
		RenderTopologySnapshot snapshot;
		snapshot.screenKind = RenderScreenKind::Title;
		expectPassIds("title", snapshot, {PassId::TitleUI});
	}

	{
		RenderTopologySnapshot snapshot;
		snapshot.screenKind = RenderScreenKind::Title;
		snapshot.features = RenderFeatures::Backdrop;
		expectPassIds("title-backdrop", snapshot, {PassId::Backdrop, PassId::TitleUI});
	}

	{
		RenderTopologySnapshot snapshot;
		snapshot.screenKind = RenderScreenKind::Loading;
		expectPassIds("loading", snapshot, {PassId::LoadingScreen});
	}

	{
		RenderTopologySnapshot snapshot;
		snapshot.screenKind = RenderScreenKind::Loading;
		snapshot.features = RenderFeatures::Backdrop;
		expectPassIds("loading-backdrop", snapshot, {PassId::LoadingBackdrop, PassId::LoadingScreen});
	}

	{
		RenderTopologySnapshot snapshot;
		snapshot.screenKind = RenderScreenKind::Video;
		expectPassIds("video", snapshot, {PassId::VideoPlayback});
	}

	if (failures != 0)
	{
		std::printf("%d/%d checks failed\n", failures, checks);
		return 1;
	}
	std::printf("%d checks passed\n", checks);
	return 0;
}
