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
/** @file scene_post_effects.cpp
 * Post-effect registry, prepare-pass emitters, and generic apply-pass emission.
 */

#include "scene_post_effects.h"

#include "lib/framework/wzapp.h"

namespace gfx_api
{

namespace
{

struct SeparableBlurPrepareChainDesc
{
	PassId generatePass;
	PassId downsamplePass;
	PassId horizontalPass;
	PassId verticalPass;
	PipelineSurfaceId rawSurface;
	PipelineSurfaceId horizontalSurface;
	PipelineSurfaceId coarseSurface;
	const char* downsampleName;
	const char* horizontalName;
	const char* verticalName;
	ClearValue clearValue;
	uint32_t downsampleFeature;
};

void emitSeparableBlurPasses(BlueprintBuilder& builder, const RenderTopologySnapshot& snapshot,
	const SeparableBlurPrepareChainDesc& desc)
{
	// The routing mechanics are common to SSAO, SSR, and future screen-space effects.
	// Formats, clear values, validity, accumulation, and compose contracts remain effect-owned.
	const bool downsample = (snapshot.features & desc.downsampleFeature) != 0;
	PassId horizontalInput = desc.generatePass;
	PipelineSurfaceId verticalOutput = desc.rawSurface;
	if (downsample)
	{
		builder.beginPass(desc.downsamplePass, desc.downsampleName)
			.color(desc.coarseSurface, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, desc.clearValue)
			.viewport(ViewportRule::ColorTarget)
			.readFrom(desc.generatePass, AttachmentRole::PrimaryColor);
		horizontalInput = desc.downsamplePass;
		verticalOutput = desc.coarseSurface;
	}

	builder.beginPass(desc.horizontalPass, desc.horizontalName)
		.color(desc.horizontalSurface, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, desc.clearValue)
		.viewport(ViewportRule::ColorTarget)
		.readFrom(horizontalInput, AttachmentRole::PrimaryColor)
		.readFrom(PassId::ScenePrepass, AttachmentRole::Depth);

	builder.beginPass(desc.verticalPass, desc.verticalName)
		.color(verticalOutput, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, desc.clearValue)
		.viewport(ViewportRule::ColorTarget)
		.readFrom(desc.horizontalPass, AttachmentRole::PrimaryColor)
		.readFrom(PassId::ScenePrepass, AttachmentRole::Depth);
}

void emitSsaoPreparePasses(BlueprintBuilder& builder, const RenderTopologySnapshot& snapshot, PassId)
{
	const ClearValue ssaoUnoccludedClear = ClearValue::colorClear(1.f, 1.f, 1.f, 1.f);

	builder.beginPass(PassId::SSAOGenerate, "SSAOGenerate")
		.color(PipelineSurfaceId::SSAORaw, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, ssaoUnoccludedClear)
		.viewport(ViewportRule::ColorTarget)
		.readFrom(PassId::ScenePrepass, AttachmentRole::Depth) // 0: prepass depth
		.readFrom(PassId::ScenePrepass, AttachmentRole::Color, /*attachmentIndex=*/0); // 1: prepass normals

	emitSeparableBlurPasses(builder, snapshot, {
		PassId::SSAOGenerate, PassId::SSAODownsample, PassId::SSAOBlurH, PassId::SSAOBlurV,
		PipelineSurfaceId::SSAORaw, PipelineSurfaceId::SSAOBlurH, PipelineSurfaceId::SSAOBlurred,
		"SSAODownsample", "SSAOBlurH", "SSAOBlurV", ssaoUnoccludedClear,
		RenderFeatures::SSAODownsample,
	});
}

void emitSsrPreparePasses(BlueprintBuilder& builder, const RenderTopologySnapshot& snapshot, PassId incomingColor)
{
	const ClearValue noReflection = ClearValue::colorClear(0.f, 0.f, 0.f, 0.f);

	builder.beginPass(PassId::SSRGenerate, "SSRGenerate")
		.color(PipelineSurfaceId::SsrRaw, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, noReflection)
		.viewport(ViewportRule::ColorTarget)
		.readFrom(PassId::ScenePrepass, AttachmentRole::Depth) // 0: prepass depth
		.readFrom(PassId::ScenePrepass, AttachmentRole::Color, /*attachmentIndex=*/0) // 1: prepass normals
		.readFrom(incomingColor, AttachmentRole::PrimaryColor); // 2: current opaque scene

	emitSeparableBlurPasses(builder, snapshot, {
		PassId::SSRGenerate, PassId::SSRDownsample, PassId::SSRBlurH, PassId::SSRBlurV,
		PipelineSurfaceId::SsrRaw, PipelineSurfaceId::SsrBlurH, PipelineSurfaceId::SsrBlurred,
		"SSRDownsample", "SSRBlurH", "SSRBlurV", noReflection,
		RenderFeatures::SSRDownsample,
	});
}

void emitRangeRingPreparePasses(BlueprintBuilder& builder, const RenderTopologySnapshot&, PassId)
{
	static constexpr ClearValue SDF_UNCOVERED = ClearValue::colorClear(1.f, 1.f, 1.f, 1.f);

	// Scratch depth is cleared per pass so sensor/weapon/min do not occlude each other.
	builder.beginPass(PassId::RangeRingSdfSensor, "RangeRingSdfSensor")
		.color(PipelineSurfaceId::RangeRingSdf, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, SDF_UNCOVERED)
		.depth(PipelineSurfaceId::RangeRingSdfDepth, AttachmentLoadOp::Clear, AttachmentStoreOp::DontCare)
		.viewport(ViewportRule::SceneColorTarget);

	builder.beginPass(PassId::RangeRingSdfWeapon, "RangeRingSdfWeapon")
		.color(PipelineSurfaceId::RangeRingSdf, AttachmentLoadOp::Load, AttachmentStoreOp::Store)
		.depth(PipelineSurfaceId::RangeRingSdfDepth, AttachmentLoadOp::Clear, AttachmentStoreOp::DontCare)
		.viewport(ViewportRule::SceneColorTarget);

	builder.beginPass(PassId::RangeRingSdfMin, "RangeRingSdfMin")
		.color(PipelineSurfaceId::RangeRingSdf, AttachmentLoadOp::Load, AttachmentStoreOp::Store)
		.depth(PipelineSurfaceId::RangeRingSdfDepth, AttachmentLoadOp::Clear, AttachmentStoreOp::DontCare)
		.viewport(ViewportRule::SceneColorTarget);
}

} // anonymous namespace

bool effectEnabled(const RenderTopologySnapshot& snapshot, ScenePostEffectId id)
{
	return snapshot.sceneEffects.enabled(id);
}

namespace
{

template <typename Enabled>
PrepassNeed unionPrepassNeeds(Enabled&& enabled)
{
	PrepassNeed needs = PrepassNeed::None;
	for (const ScenePostEffectDesc& effect : kScenePostEffects)
	{
		if (enabled(effect.id))
		{
			needs = needs | effect.prepassNeed;
		}
	}
	return needs;
}

template <typename Enabled>
bool anyEffectEnabled(Enabled&& enabled)
{
	for (const ScenePostEffectDesc& effect : kScenePostEffects)
	{
		if (enabled(effect.id))
		{
			return true;
		}
	}
	return false;
}

// These arrays own the storage viewed by ScenePostEffectDesc::applyReads.
// Array order is consumed unchanged as the apply shader's texture-binding order.
static constexpr auto kSsaoApplyReads = std::array{
	ScenePostEffectRead::incomingColor(),
	ScenePostEffectRead::passOutput(PassId::SSAOBlurV),
	ScenePostEffectRead::passOutput(PassId::ScenePrepass, AttachmentRole::Color, 0),
};

static constexpr auto kSsrApplyReads = std::array{
	ScenePostEffectRead::incomingColor(),
	ScenePostEffectRead::passOutput(PassId::SSRBlurV),
	ScenePostEffectRead::passOutput(PassId::ScenePrepass, AttachmentRole::Color, 0),
	ScenePostEffectRead::passOutput(PassId::ScenePrepass, AttachmentRole::Depth),
};

static constexpr auto kFogApplyReads = std::array{
	ScenePostEffectRead::incomingColor(),
	ScenePostEffectRead::passOutput(PassId::ScenePrepass, AttachmentRole::Depth),
};

static constexpr auto kRangeRingApplyReads = std::array{
	ScenePostEffectRead::incomingColor(),
	ScenePostEffectRead::passOutput(PassId::ScenePrepass, AttachmentRole::Depth),
	ScenePostEffectRead::passOutput(PassId::RangeRingSdfMin),
};

} // anonymous namespace

bool anyScenePostEffectEnabled(const RenderTopologySnapshot& snapshot)
{
	return anyEffectEnabled([&](ScenePostEffectId id) { return effectEnabled(snapshot, id); });
}

bool anyScenePostEffectEnabled(const SceneEffectSurfaces& cfg)
{
	return anyEffectEnabled([&](ScenePostEffectId id) { return cfg.enabled(id); });
}

PrepassNeed prepassNeeds(const RenderTopologySnapshot& snapshot)
{
	PrepassNeed needs = unionPrepassNeeds([&](ScenePostEffectId id) { return effectEnabled(snapshot, id); });
	if (anyScenePostEffectEnabled(snapshot))
	{
		// A post-effect sits between opaque ScenePass and the transparents, so the blueprint separates them into SceneTransparent,
		// which depth-tests forward transparents against the single-sample prepass depth.
		// (With no effect enabled, the passes fuse - transparents draw in ScenePass - and no prepass is required at all.)
		needs = needs | PrepassNeed::Depth;
	}
	return needs;
}

PrepassNeed prepassNeeds(const SceneEffectSurfaces& cfg)
{
	// Keep surface allocation in lockstep with the in-game blueprint requirement above.
	PrepassNeed needs = unionPrepassNeeds([&](ScenePostEffectId id) { return cfg.enabled(id); });
	if (anyScenePostEffectEnabled(cfg))
	{
		needs = needs | PrepassNeed::Depth;
	}
	return needs;
}

void emitApplyPass(BlueprintBuilder& builder, const ScenePostEffectDesc& effect, PassId incomingColor)
{
	const char* debugName = effect.applyDebugName != nullptr ? effect.applyDebugName : "ScenePostEffect";
	ASSERT(effect.applyPass != PassId::Count, "emitApplyPass: missing applyPass for %s", debugName);
	ASSERT(effect.applyOutput != PipelineSurfaceId::Count, "emitApplyPass: missing applyOutput for %s", debugName);

	builder.beginPass(effect.applyPass, debugName)
		.color(effect.applyOutput, AttachmentLoadOp::DontCare, AttachmentStoreOp::Store)
		.viewport(ViewportRule::SceneColorTarget);

	for (const ScenePostEffectRead& read : effect.applyReads)
	{
		switch (read.source)
		{
		case ScenePostEffectRead::Source::IncomingColor:
			builder.readFrom(incomingColor, AttachmentRole::PrimaryColor, 0);
			break;
		case ScenePostEffectRead::Source::PassOutput:
			ASSERT(read.edge.producerPass != PassId::Count,
				"emitApplyPass: fixed read has no producer (%s)", debugName);
			ASSERT(read.edge.attachmentIndex == 0 || read.edge.producerRole == AttachmentRole::Color,
				"emitApplyPass: attachment index is only valid for Color reads (%s)", debugName);
			builder.readFrom(read.edge.producerPass, read.edge.producerRole, read.edge.attachmentIndex);
			break;
		}
	}
}

const std::array<ScenePostEffectDesc, static_cast<size_t>(ScenePostEffectId::Count)> kScenePostEffects = {{
	{
		.id = ScenePostEffectId::Ssao,
		.prepassNeed = PrepassNeed::Depth | PrepassNeed::Normals,
		.emitPreparePasses = emitSsaoPreparePasses,
		.applyPass = PassId::SSAOCompose,
		.applyDebugName = "SSAOCompose",
		.applyOutput = PipelineSurfaceId::SSAOComposedColor,
		.applyReads = kSsaoApplyReads,
	},
	{
		.id = ScenePostEffectId::Ssr,
		.prepassNeed = PrepassNeed::Depth | PrepassNeed::Normals,
		.emitPreparePasses = emitSsrPreparePasses,
		.applyPass = PassId::SSRCompose,
		.applyDebugName = "SSRCompose",
		.applyOutput = PipelineSurfaceId::SsrComposedColor,
		.applyReads = kSsrApplyReads,
	},
	{
		.id = ScenePostEffectId::Fog,
		.prepassNeed = PrepassNeed::Depth,
		.applyPass = PassId::FogApply,
		.applyDebugName = "FogApply",
		.applyOutput = PipelineSurfaceId::FogColor,
		.applyReads = kFogApplyReads,
	},
	{
		.id = ScenePostEffectId::RangeRings,
		.prepassNeed = PrepassNeed::Depth,
		.emitPreparePasses = emitRangeRingPreparePasses,
		.applyPass = PassId::RangeRingComposite,
		.applyDebugName = "RangeRingComposite",
		.applyOutput = PipelineSurfaceId::RangeRingColor,
		.applyReads = kRangeRingApplyReads,
	},
}};

} // namespace gfx_api
