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

#include <cstdarg>
#include <cstdio>
#include <initializer_list>
#include <source_location>
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
	case PassId::ContactShadowGenerate: return "ContactShadowGenerate";
	case PassId::ContactShadowDownsample: return "ContactShadowDownsample";
	case PassId::ContactShadowBlurH: return "ContactShadowBlurH";
	case PassId::ContactShadowBlurV: return "ContactShadowBlurV";
	case PassId::Count: return "Count";
	}
	return "?";
}

static const char* surfaceName(gfx_api::PipelineSurfaceId id)
{
	switch (id)
	{
	case gfx_api::PipelineSurfaceId::SceneColor: return "SceneColor";
	case gfx_api::PipelineSurfaceId::SceneMSAAColor: return "SceneMSAAColor";
	case gfx_api::PipelineSurfaceId::SceneDepth: return "SceneDepth";
	case gfx_api::PipelineSurfaceId::UpscaledColor: return "UpscaledColor";
	case gfx_api::PipelineSurfaceId::SmaaEdges: return "SmaaEdges";
	case gfx_api::PipelineSurfaceId::SmaaWeights: return "SmaaWeights";
	case gfx_api::PipelineSurfaceId::SmaaColor: return "SmaaColor";
	case gfx_api::PipelineSurfaceId::ScenePrepassDepth: return "ScenePrepassDepth";
	case gfx_api::PipelineSurfaceId::ScenePrepassNormals: return "ScenePrepassNormals";
	case gfx_api::PipelineSurfaceId::ContactShadowRaw: return "ContactShadowRaw";
	case gfx_api::PipelineSurfaceId::ContactShadowBlurH: return "ContactShadowBlurH";
	case gfx_api::PipelineSurfaceId::ContactShadowBlurred: return "ContactShadowBlurred";
	case gfx_api::PipelineSurfaceId::SSAORaw: return "SSAORaw";
	case gfx_api::PipelineSurfaceId::SSAOBlurH: return "SSAOBlurH";
	case gfx_api::PipelineSurfaceId::SSAOBlurred: return "SSAOBlurred";
	case gfx_api::PipelineSurfaceId::SSAOComposedColor: return "SSAOComposedColor";
	case gfx_api::PipelineSurfaceId::SsrRaw: return "SsrRaw";
	case gfx_api::PipelineSurfaceId::SsrBlurH: return "SsrBlurH";
	case gfx_api::PipelineSurfaceId::SsrBlurred: return "SsrBlurred";
	case gfx_api::PipelineSurfaceId::SsrComposedColor: return "SsrComposedColor";
	case gfx_api::PipelineSurfaceId::FogColor: return "FogColor";
	case gfx_api::PipelineSurfaceId::RangeRingSdf: return "RangeRingSdf";
	case gfx_api::PipelineSurfaceId::RangeRingSdfDepth: return "RangeRingSdfDepth";
	case gfx_api::PipelineSurfaceId::RangeRingColor: return "RangeRingColor";
	case gfx_api::PipelineSurfaceId::BloomMip0: return "BloomMip0";
	case gfx_api::PipelineSurfaceId::BloomMip1: return "BloomMip1";
	case gfx_api::PipelineSurfaceId::BloomMip2: return "BloomMip2";
	case gfx_api::PipelineSurfaceId::BloomMip3: return "BloomMip3";
	case gfx_api::PipelineSurfaceId::BloomMip4: return "BloomMip4";
	case gfx_api::PipelineSurfaceId::BloomMip5: return "BloomMip5";
	case gfx_api::PipelineSurfaceId::TonemapColor: return "TonemapColor";
	case gfx_api::PipelineSurfaceId::ShadowMap: return "ShadowMap";
	case gfx_api::PipelineSurfaceId::SwapchainColor: return "SwapchainColor";
	case gfx_api::PipelineSurfaceId::SwapchainMSAAColor: return "SwapchainMSAAColor";
	case gfx_api::PipelineSurfaceId::SwapchainDepth: return "SwapchainDepth";
	case gfx_api::PipelineSurfaceId::Count: return "Count";
	}
	return "?";
}

static const char* loadOpName(gfx_api::AttachmentLoadOp op)
{
	switch (op)
	{
	case gfx_api::AttachmentLoadOp::Clear: return "Clear";
	case gfx_api::AttachmentLoadOp::Load: return "Load";
	case gfx_api::AttachmentLoadOp::DontCare: return "DontCare";
	}
	return "?";
}

static const char* roleName(gfx_api::AttachmentRole role)
{
	switch (role)
	{
	case gfx_api::AttachmentRole::PrimaryColor: return "PrimaryColor";
	case gfx_api::AttachmentRole::Color: return "Color";
	case gfx_api::AttachmentRole::Depth: return "Depth";
	case gfx_api::AttachmentRole::Resolve: return "Resolve";
	}
	return "?";
}

static std::string describe(PassId id) { return passIdName(id); }
static std::string describe(gfx_api::PipelineSurfaceId id) { return surfaceName(id); }
static std::string describe(gfx_api::AttachmentLoadOp op) { return loadOpName(op); }
static std::string describe(gfx_api::AttachmentRole role) { return roleName(role); }


static void noteCheck()
{
	++checks;
}

static void failAt(std::source_location loc, const char* fmt, ...)
{
	++failures;
	std::printf("FAIL %s:%u: ", loc.file_name(), static_cast<unsigned>(loc.line()));
	va_list args;
	va_start(args, fmt);
	std::vprintf(fmt, args);
	va_end(args);
	std::printf("\n");
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

static void expectPassIds(const char* name, const RenderTopologySnapshot& snapshot,
	const std::vector<PassId>& expected, std::source_location loc)
{
	noteCheck();
	const gfx_api::PassGraphTopologyBlueprint blueprint = gfx_api::PassGraphTopologyBlueprint::fromSnapshot(snapshot);
	const std::vector<PassId> actual = gfx_api::blueprintPassIds(blueprint);
	if (actual == expected)
	{
		return;
	}
	failAt(loc, "%s\n  expected: [%s]\n  actual:   [%s]",
		name, formatPassIds(expected).c_str(), formatPassIds(actual).c_str());
}

static void expectPassIds(const char* name, const RenderTopologySnapshot& snapshot,
	std::initializer_list<PassId> expected,
	std::source_location loc = std::source_location::current())
{
	expectPassIds(name, snapshot, std::vector<PassId>(expected), loc);
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

static void expectInGame(const char* name, const RenderTopologySnapshot& snapshot,
	std::initializer_list<PassId> head,
	std::source_location loc = std::source_location::current())
{
	expectPassIds(name, snapshot, withInGameTail(head), loc);
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

static const std::vector<PassId>& bloomPasses()
{
	static const std::vector<PassId> passes = {
		PassId::BloomDownsample0, PassId::BloomDownsample1, PassId::BloomDownsample2,
		PassId::BloomDownsample3, PassId::BloomDownsample4, PassId::BloomDownsample5,
		PassId::BloomUpsample4, PassId::BloomUpsample3, PassId::BloomUpsample2,
		PassId::BloomUpsample1, PassId::BloomUpsample0,
	};
	return passes;
}

static std::vector<PassId> withBloom(std::vector<PassId> before, std::vector<PassId> after)
{
	before.insert(before.end(), bloomPasses().begin(), bloomPasses().end());
	before.insert(before.end(), after.begin(), after.end());
	return before;
}

static void expectInGame(const char* name, const RenderTopologySnapshot& snapshot,
	std::vector<PassId> head,
	std::source_location loc = std::source_location::current())
{
	head.insert(head.end(), {
		PassId::TargettingEffects,
		PassId::SceneOverlays,
		PassId::SceneDebugOverlays,
		PassId::GameStartFade,
		PassId::InGameUI,
	});
	expectPassIds(name, snapshot, head, loc);
}


static bool isUnoccludedWhite(const gfx_api::ClearValue& clear)
{
	const auto& color = clear.color;
	return color[0] == 1.f && color[1] == 1.f && color[2] == 1.f && color[3] == 1.f;
}

struct ExpectedRead
{
	PassId producer;
	gfx_api::AttachmentRole role = gfx_api::AttachmentRole::PrimaryColor;
	uint32_t attachmentIndex = 0;
};

static bool sameRead(const ExpectedRead& expected, const gfx_api::BlueprintReadEdge& actual)
{
	return expected.producer == actual.producerPass
		&& expected.role == actual.producerRole
		&& expected.attachmentIndex == actual.attachmentIndex;
}

static std::string formatExpectedReads(const std::vector<ExpectedRead>& reads)
{
	std::stringstream strm;
	for (size_t i = 0; i < reads.size(); ++i)
	{
		if (i != 0)
		{
			strm << ", ";
		}
		strm << describe(reads[i].producer) << "/" << describe(reads[i].role)
			<< "/" << reads[i].attachmentIndex;
	}
	return strm.str();
}

static std::string formatActualReads(const std::vector<gfx_api::BlueprintReadEdge>& reads)
{
	std::stringstream strm;
	for (size_t i = 0; i < reads.size(); ++i)
	{
		if (i != 0)
		{
			strm << ", ";
		}
		strm << describe(reads[i].producerPass) << "/" << describe(reads[i].producerRole)
			<< "/" << reads[i].attachmentIndex;
	}
	return strm.str();
}

/// The pass exists and its read edges match `expected` in order.
static void expectReads(const char* name, const gfx_api::PassGraphTopologyBlueprint& blueprint, PassId id,
	std::initializer_list<ExpectedRead> expected,
	std::source_location loc = std::source_location::current())
{
	noteCheck();
	const gfx_api::BlueprintPass* pass = findPass(blueprint, id);
	const std::vector<ExpectedRead> expectedReads(expected);
	bool ok = pass != nullptr && pass->reads.size() == expectedReads.size();
	for (size_t i = 0; ok && i < expectedReads.size(); ++i)
	{
		ok = sameRead(expectedReads[i], pass->reads[i]);
	}
	if (ok)
	{
		return;
	}
	failAt(loc, "%s: %s reads\n  expected: [%s]\n  actual:   [%s]",
		name, passIdName(id), formatExpectedReads(expectedReads).c_str(),
		pass != nullptr ? formatActualReads(pass->reads).c_str() : "missing");
}

static void expectColorChecked(const char* name, const gfx_api::PassGraphTopologyBlueprint& blueprint, PassId id,
	gfx_api::PipelineSurfaceId surface, const gfx_api::AttachmentLoadOp* load, std::source_location loc)
{
	noteCheck();
	const gfx_api::BlueprintPass* pass = findPass(blueprint, id);
	const bool surfaceOk = pass != nullptr
		&& pass->colorAttachments.size() == 1
		&& pass->colorAttachments[0].surfaceId == surface;
	const bool loadOk = load == nullptr
		|| (surfaceOk && pass->colorAttachments[0].loadOp == *load);
	if (surfaceOk && loadOk)
	{
		return;
	}
	const char* actualSurface = (pass != nullptr && !pass->colorAttachments.empty())
		? surfaceName(pass->colorAttachments[0].surfaceId) : "missing";
	if (load == nullptr)
	{
		failAt(loc, "%s: %s color: expected %s, got %s", name, passIdName(id), describe(surface).c_str(), actualSurface);
		return;
	}
	failAt(loc, "%s: %s color: expected %s %s, got %s",
		name, passIdName(id), describe(surface).c_str(), describe(*load).c_str(), actualSurface);
}

/// The pass exists and its single color attachment is `surface`.
static void expectColor(const char* name, const gfx_api::PassGraphTopologyBlueprint& blueprint, PassId id,
	gfx_api::PipelineSurfaceId surface,
	std::source_location loc = std::source_location::current())
{
	expectColorChecked(name, blueprint, id, surface, nullptr, loc);
}

/// The pass exists and its single color attachment is `surface` with `load`.
static void expectColor(const char* name, const gfx_api::PassGraphTopologyBlueprint& blueprint, PassId id,
	gfx_api::PipelineSurfaceId surface, gfx_api::AttachmentLoadOp load,
	std::source_location loc = std::source_location::current())
{
	expectColorChecked(name, blueprint, id, surface, &load, loc);
}

/// The pass exists and its single color attachment is `surface`, cleared to unoccluded white.
static void expectUnoccludedColor(const char* name, const gfx_api::PassGraphTopologyBlueprint& blueprint, PassId id,
	gfx_api::PipelineSurfaceId surface,
	std::source_location loc = std::source_location::current())
{
	noteCheck();
	const gfx_api::BlueprintPass* pass = findPass(blueprint, id);
	const bool ok = pass != nullptr
		&& pass->colorAttachments.size() == 1
		&& pass->colorAttachments[0].surfaceId == surface
		&& pass->colorAttachments[0].loadOp == gfx_api::AttachmentLoadOp::Clear
		&& isUnoccludedWhite(pass->colorAttachments[0].clearValue);
	if (!ok)
	{
		failAt(loc, "%s: %s clear: expected %s cleared to white", name, passIdName(id), describe(surface).c_str());
	}
}

/// The pass exists and its depth attachment is `surface`.
static void expectDepth(const char* name, const gfx_api::PassGraphTopologyBlueprint& blueprint, PassId id,
	gfx_api::PipelineSurfaceId surface,
	std::source_location loc = std::source_location::current())
{
	noteCheck();
	const gfx_api::BlueprintPass* pass = findPass(blueprint, id);
	const bool ok = pass != nullptr
		&& pass->depthAttachment.has_value()
		&& pass->depthAttachment.value().surfaceId == surface;
	if (!ok)
	{
		failAt(loc, "%s: %s depth: expected %s", name, passIdName(id), describe(surface).c_str());
	}
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
		expectColor("hdr-bloom", blueprint, PassId::BloomDownsample0, PipelineSurfaceId::BloomMip0, AttachmentLoadOp::DontCare);
		expectReads("hdr-bloom", blueprint, PassId::BloomDownsample0, {{PassId::ScenePass}});
		expectColor("hdr-bloom", blueprint, PassId::BloomDownsample5, PipelineSurfaceId::BloomMip5, AttachmentLoadOp::DontCare);
		expectReads("hdr-bloom", blueprint, PassId::BloomDownsample5, {{PassId::BloomDownsample4}});
		expectColor("hdr-bloom", blueprint, PassId::BloomUpsample4, PipelineSurfaceId::BloomMip4, AttachmentLoadOp::Load);
		expectReads("hdr-bloom", blueprint, PassId::BloomUpsample4, {{PassId::BloomDownsample5}});
		expectColor("hdr-bloom", blueprint, PassId::BloomUpsample0, PipelineSurfaceId::BloomMip0, AttachmentLoadOp::Load);
		expectReads("hdr-bloom", blueprint, PassId::BloomUpsample0, {{PassId::BloomUpsample1}});
		expectReads("hdr-bloom", blueprint, PassId::SceneTonemap, {{PassId::ScenePass}, {PassId::BloomUpsample0}});
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
		expectColor("hdr-fog-bloom", blueprint, PassId::BloomDownsample0, gfx_api::PipelineSurfaceId::BloomMip0,
			gfx_api::AttachmentLoadOp::DontCare);
		expectReads("hdr-fog-bloom", blueprint, PassId::BloomDownsample0, {{PassId::SceneTransparent}});
		expectReads("hdr-fog-bloom", blueprint, PassId::SceneTonemap, {{PassId::SceneTransparent}, {PassId::BloomUpsample0}});
	}

	{
		// Scene MSAA: bloom reads the resolved scene
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
		// Contact shadows are lighting-prepare: prepass, mask, blur, then the scene.
		// Transparents stay fused in ScenePass. There is no compose pass.
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.contactShadows = true;
		expectInGame("contact", snapshot, {
			PassId::ScenePrepass,
			PassId::ContactShadowGenerate, PassId::ContactShadowBlurH, PassId::ContactShadowBlurV,
			PassId::ScenePass, PassId::SceneBlit});

		const auto blueprint = gfx_api::PassGraphTopologyBlueprint::fromSnapshot(snapshot);
		using gfx_api::AttachmentRole;
		using gfx_api::PipelineSurfaceId;

		expectColor("contact", blueprint, PassId::ScenePrepass, PipelineSurfaceId::ScenePrepassNormals);
		expectDepth("contact", blueprint, PassId::ScenePrepass, PipelineSurfaceId::ScenePrepassDepth);
		expectUnoccludedColor("contact", blueprint, PassId::ContactShadowGenerate, PipelineSurfaceId::ContactShadowRaw);
		expectReads("contact", blueprint, PassId::ContactShadowGenerate, {
			{PassId::ScenePrepass, AttachmentRole::Depth},
			{PassId::ScenePrepass, AttachmentRole::Color, 0},
		});
		expectUnoccludedColor("contact", blueprint, PassId::ContactShadowBlurH, PipelineSurfaceId::ContactShadowBlurH);
		expectReads("contact", blueprint, PassId::ContactShadowBlurH, {
			{PassId::ContactShadowGenerate},
			{PassId::ScenePrepass, AttachmentRole::Depth},
		});
		expectUnoccludedColor("contact", blueprint, PassId::ContactShadowBlurV, PipelineSurfaceId::ContactShadowRaw);
		expectReads("contact", blueprint, PassId::ContactShadowBlurV, {
			{PassId::ContactShadowBlurH},
			{PassId::ScenePrepass, AttachmentRole::Depth},
		});
		expectReads("contact", blueprint, PassId::ScenePass, {{PassId::ContactShadowBlurV}});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.contactShadows = true;
		snapshot.features |= RenderFeatures::ContactShadowsDownsample;
		expectInGame("contact-downsample", snapshot, {
			PassId::ScenePrepass,
			PassId::ContactShadowGenerate, PassId::ContactShadowDownsample,
			PassId::ContactShadowBlurH, PassId::ContactShadowBlurV,
			PassId::ScenePass, PassId::SceneBlit});

		const auto blueprint = gfx_api::PassGraphTopologyBlueprint::fromSnapshot(snapshot);
		expectUnoccludedColor("contact-downsample", blueprint, PassId::ContactShadowDownsample, gfx_api::PipelineSurfaceId::ContactShadowBlurred);
		expectReads("contact-downsample", blueprint, PassId::ContactShadowDownsample, {{PassId::ContactShadowGenerate}});
		expectReads("contact-downsample", blueprint, PassId::ContactShadowBlurH, {
			{PassId::ContactShadowDownsample},
			{PassId::ScenePrepass, gfx_api::AttachmentRole::Depth},
		});
		expectColor("contact-downsample", blueprint, PassId::ContactShadowBlurV, gfx_api::PipelineSurfaceId::ContactShadowBlurred);
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.contactShadows = true;
		snapshot.sceneEffects.fog = true;
		expectInGame("contact-fog", snapshot, {
			PassId::ScenePrepass,
			PassId::ContactShadowGenerate, PassId::ContactShadowBlurH, PassId::ContactShadowBlurV,
			PassId::ScenePass, PassId::FogApply, PassId::SceneTransparent, PassId::SceneBlit});

		const auto blueprint = gfx_api::PassGraphTopologyBlueprint::fromSnapshot(snapshot);
		expectReads("contact-fog", blueprint, PassId::ScenePass, {{PassId::ContactShadowBlurV}});
		expectReads("contact-fog", blueprint, PassId::SceneTransparent, {});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.contactShadows = true;
		snapshot.sceneEffects.ssao = true;
		expectInGame("contact-ssao", snapshot, {
			PassId::ScenePrepass,
			PassId::ContactShadowGenerate, PassId::ContactShadowBlurH, PassId::ContactShadowBlurV,
			PassId::ScenePass,
			PassId::SSAOGenerate, PassId::SSAOBlurH, PassId::SSAOBlurV, PassId::SSAOCompose,
			PassId::SceneTransparent, PassId::SceneBlit});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.contactShadows = true;
		snapshot.numShadowCascades = 1;
		expectInGame("contact-cascade", snapshot, {
			PassId::ShadowCascade0,
			PassId::ScenePrepass,
			PassId::ContactShadowGenerate, PassId::ContactShadowBlurH, PassId::ContactShadowBlurV,
			PassId::ScenePass, PassId::SceneBlit});

		const auto blueprint = gfx_api::PassGraphTopologyBlueprint::fromSnapshot(snapshot);
		expectReads("contact-cascade", blueprint, PassId::ScenePass, {
			{PassId::ShadowCascade0, gfx_api::AttachmentRole::Depth},
			{PassId::ContactShadowBlurV},
		});
	}

	{
		// Contact does not split transparents, so bloom still reads ScenePass.
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.sceneEffects.contactShadows = true;
		snapshot.sceneEffects.bloom = true;
		snapshot.features |= RenderFeatures::HdrScene;
		expectInGame("contact-hdr-bloom", snapshot, withBloom({
			PassId::ScenePrepass,
			PassId::ContactShadowGenerate, PassId::ContactShadowBlurH, PassId::ContactShadowBlurV,
			PassId::ScenePass}, {PassId::SceneTonemap, PassId::SceneBlit}));

		const auto blueprint = gfx_api::PassGraphTopologyBlueprint::fromSnapshot(snapshot);
		expectColor("contact-hdr-bloom", blueprint, PassId::BloomDownsample0, gfx_api::PipelineSurfaceId::BloomMip0,
			gfx_api::AttachmentLoadOp::DontCare);
		expectReads("contact-hdr-bloom", blueprint, PassId::BloomDownsample0, {{PassId::ScenePass}});
		expectReads("contact-hdr-bloom", blueprint, PassId::SceneTonemap, {{PassId::ScenePass}, {PassId::BloomUpsample0}});
	}

	{
		RenderTopologySnapshot snapshot;
		snapshot.screenKind = RenderScreenKind::Title;
		snapshot.sceneEffects.contactShadows = true;
		expectPassIds("title-ignores-contact", snapshot, {PassId::TitleUI});
	}

	{
		RenderTopologySnapshot snapshot = inGameBase();
		snapshot.features |= RenderFeatures::FrozenWorldOverlay;
		snapshot.sceneEffects.contactShadows = true;
		expectPassIds("frozen-ignores-contact", snapshot, {PassId::InGameUI});
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
