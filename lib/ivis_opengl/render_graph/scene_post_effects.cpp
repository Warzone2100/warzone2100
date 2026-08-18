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

void emitSsaoPreparePasses(BlueprintBuilder& builder, const RenderTopologySnapshot& snapshot)
{
	const bool ssaoDownsample = (snapshot.features & RenderFeatures::SSAODownsample) != 0;
	const ClearValue ssaoUnoccludedClear = ClearValue::colorClear(1.f, 1.f, 1.f, 1.f);

	builder.beginPass(PassId::SSAOGenerate, "SSAOGenerate")
		.color(PipelineSurfaceId::SSAORaw, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, ssaoUnoccludedClear)
		.viewport(ViewportRule::ColorTarget)
		.readFrom(PassId::ScenePrepass, AttachmentRole::Depth) // 0: prepass depth
		.readFrom(PassId::ScenePrepass, AttachmentRole::Color, /*attachmentIndex=*/0); // 1: prepass normals

	if (ssaoDownsample)
	{
		builder.beginPass(PassId::SSAODownsample, "SSAODownsample")
			.color(PipelineSurfaceId::SSAOBlurred, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, ssaoUnoccludedClear)
			.viewport(ViewportRule::ColorTarget)
			.readFrom(PassId::SSAOGenerate, AttachmentRole::PrimaryColor); // 0: generate AO

		builder.beginPass(PassId::SSAOBlurH, "SSAOBlurH")
			.color(PipelineSurfaceId::SSAOBlurH, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, ssaoUnoccludedClear)
			.viewport(ViewportRule::ColorTarget)
			.readFrom(PassId::SSAODownsample, AttachmentRole::PrimaryColor) // 0: occlusion
			.readFrom(PassId::ScenePrepass, AttachmentRole::Depth); // 1: prepass depth

		builder.beginPass(PassId::SSAOBlurV, "SSAOBlurV")
			.color(PipelineSurfaceId::SSAOBlurred, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, ssaoUnoccludedClear)
			.viewport(ViewportRule::ColorTarget)
			.readFrom(PassId::SSAOBlurH, AttachmentRole::PrimaryColor) // 0: occlusion
			.readFrom(PassId::ScenePrepass, AttachmentRole::Depth); // 1: prepass depth
	}
	else
	{
		builder.beginPass(PassId::SSAOBlurH, "SSAOBlurH")
			.color(PipelineSurfaceId::SSAOBlurH, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, ssaoUnoccludedClear)
			.viewport(ViewportRule::ColorTarget)
			.readFrom(PassId::SSAOGenerate, AttachmentRole::PrimaryColor) // 0: occlusion
			.readFrom(PassId::ScenePrepass, AttachmentRole::Depth); // 1: prepass depth

		builder.beginPass(PassId::SSAOBlurV, "SSAOBlurV")
			.color(PipelineSurfaceId::SSAORaw, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, ssaoUnoccludedClear)
			.viewport(ViewportRule::ColorTarget)
			.readFrom(PassId::SSAOBlurH, AttachmentRole::PrimaryColor) // 0: occlusion
			.readFrom(PassId::ScenePrepass, AttachmentRole::Depth); // 1: prepass depth
	}
}

} // anonymous namespace

bool effectEnabled(const RenderTopologySnapshot& snapshot, ScenePostEffectId id)
{
	switch (id)
	{
	case ScenePostEffectId::Ssao:
		return (snapshot.features & RenderFeatures::SSAO) != 0;
	case ScenePostEffectId::Fog:
		return (snapshot.features & RenderFeatures::FogApply) != 0;
	case ScenePostEffectId::Count:
		break;
	}
	return false;
}

PrepassNeed prepassNeeds(const RenderTopologySnapshot& snapshot)
{
	PrepassNeed needs = PrepassNeed::None;
	for (const ScenePostEffectDesc& effect : kScenePostEffects)
	{
		if (effectEnabled(snapshot, effect.id))
		{
			needs = needs | effect.prepassNeed;
		}
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

	for (uint8_t i = 0; i < effect.applyInputCount; ++i)
	{
		switch (effect.applyInputs[i])
		{
		case ApplyInput::IncomingColor:
			builder.readFrom(incomingColor, AttachmentRole::PrimaryColor);
			break;
		case ApplyInput::PrepassDepth:
			builder.readFrom(PassId::ScenePrepass, AttachmentRole::Depth);
			break;
		case ApplyInput::PrepassNormals:
			builder.readFrom(PassId::ScenePrepass, AttachmentRole::Color, /*attachmentIndex=*/0);
			break;
		case ApplyInput::PreparedOutput:
			ASSERT(effect.preparedColorPass != PassId::Count,
				"emitApplyPass: PreparedOutput sample without preparedColorPass (%s)", debugName);
			builder.readFrom(effect.preparedColorPass, AttachmentRole::PrimaryColor);
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
		.applyInputs = { ApplyInput::IncomingColor, ApplyInput::PreparedOutput, ApplyInput::PrepassNormals },
		.applyInputCount = 3,
		.preparedColorPass = PassId::SSAOBlurV,
	},
	{
		.id = ScenePostEffectId::Fog,
		.prepassNeed = PrepassNeed::Depth,
		.applyPass = PassId::FogApply,
		.applyDebugName = "FogApply",
		.applyOutput = PipelineSurfaceId::FogColor,
		.applyInputs = { ApplyInput::IncomingColor, ApplyInput::PrepassDepth },
		.applyInputCount = 2,
	},
}};

} // namespace gfx_api
