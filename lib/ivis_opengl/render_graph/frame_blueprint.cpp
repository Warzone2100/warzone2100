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
/** @file frame_blueprint.cpp
 * Screen-kind blueprint templates (in-game, title, loading, video) built from topology snapshots.
 */

#include "blueprint.h"
#include "topology.h"

#include "lib/framework/wzapp.h"

#include <string>

namespace gfx_api
{

namespace
{

PassGraphTopologyBlueprint buildInGameBlueprint(const RenderTopologySnapshot& snapshot)
{
	ASSERT(snapshot.screenKind == RenderScreenKind::InGame, "buildInGameBlueprint: wrong screen kind");

	if (snapshot.features & RenderFeatures::FrozenWorldOverlay)
	{
		BlueprintBuilder builder;

		if (snapshot.features & RenderFeatures::Backdrop)
		{
			addSwapchainPassToBuilder(builder, PassId::Backdrop, "Backdrop", snapshot.swapchainMsaa,
				AttachmentLoadOp::Clear, AttachmentLoadOp::Clear);
		}

		addSwapchainPassToBuilder(builder, PassId::InGameUI, "InGameUI", snapshot.swapchainMsaa,
			AttachmentLoadOp::Load, AttachmentLoadOp::Clear);

		return builder.build();
	}

	BlueprintBuilder builder;

	if (snapshot.features & RenderFeatures::Backdrop)
	{
		addSwapchainPassToBuilder(builder, PassId::Backdrop, "Backdrop", snapshot.swapchainMsaa,
			AttachmentLoadOp::Clear, AttachmentLoadOp::Clear);
	}

	for (uint32_t i = 0; i < snapshot.numShadowCascades; ++i)
	{
		builder.beginPass(shadowCascadePassId(i), "ShadowCascade" + std::to_string(i))
			.depth(PipelineSurfaceId::ShadowMap, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, i)
			.viewport(ViewportRule::DepthCascade, i);
	}

	const bool ssaoActive = (snapshot.features & RenderFeatures::SSAO) != 0;
	const bool fogActive = (snapshot.features & RenderFeatures::FogApply) != 0;
	if (ssaoActive || fogActive)
	{
		builder.beginPass(PassId::ScenePrepass, "ScenePrepass")
			.color(PipelineSurfaceId::ScenePrepassNormals, AttachmentLoadOp::Clear, AttachmentStoreOp::Store)
			.depth(PipelineSurfaceId::ScenePrepassDepth, AttachmentLoadOp::Clear, AttachmentStoreOp::Store)
			.viewport(ViewportRule::SceneColorTarget);
	}

	addScenePassToBuilder(builder, PassId::ScenePass, snapshot.sceneMsaa, snapshot.numShadowCascades);

	if (ssaoActive)
	{
		const bool ssaoDownsample = (snapshot.features & RenderFeatures::SSAODownsample) != 0;

		builder.beginPass(PassId::SSAOGenerate, "SSAOGenerate")
			.color(PipelineSurfaceId::SSAORaw, AttachmentLoadOp::Clear, AttachmentStoreOp::Store)
			.viewport(ViewportRule::ColorTarget)
			.readFrom(PassId::ScenePrepass, AttachmentRole::Depth)
			.readFrom(PassId::ScenePrepass, AttachmentRole::Color, /*attachmentIndex=*/0);

		if (ssaoDownsample)
		{
			builder.beginPass(PassId::SSAODownsample, "SSAODownsample")
				.color(PipelineSurfaceId::SSAOBlurred, AttachmentLoadOp::Clear, AttachmentStoreOp::Store)
				.viewport(ViewportRule::ColorTarget)
				.readFrom(PassId::SSAOGenerate, AttachmentRole::PrimaryColor);

			builder.beginPass(PassId::SSAOBlurH, "SSAOBlurH")
				.color(PipelineSurfaceId::SSAOBlurH, AttachmentLoadOp::Clear, AttachmentStoreOp::Store)
				.viewport(ViewportRule::ColorTarget)
				.readFrom(PassId::SSAODownsample, AttachmentRole::PrimaryColor)
				.readFrom(PassId::ScenePrepass, AttachmentRole::Depth);

			builder.beginPass(PassId::SSAOBlurV, "SSAOBlurV")
				.color(PipelineSurfaceId::SSAOBlurred, AttachmentLoadOp::Clear, AttachmentStoreOp::Store)
				.viewport(ViewportRule::ColorTarget)
				.readFrom(PassId::SSAOBlurH, AttachmentRole::PrimaryColor)
				.readFrom(PassId::ScenePrepass, AttachmentRole::Depth);
		}
		else
		{
			builder.beginPass(PassId::SSAOBlurH, "SSAOBlurH")
				.color(PipelineSurfaceId::SSAOBlurH, AttachmentLoadOp::Clear, AttachmentStoreOp::Store)
				.viewport(ViewportRule::ColorTarget)
				.readFrom(PassId::SSAOGenerate, AttachmentRole::PrimaryColor)
				.readFrom(PassId::ScenePrepass, AttachmentRole::Depth);

			builder.beginPass(PassId::SSAOBlurV, "SSAOBlurV")
				.color(PipelineSurfaceId::SSAORaw, AttachmentLoadOp::Clear, AttachmentStoreOp::Store)
				.viewport(ViewportRule::ColorTarget)
				.readFrom(PassId::SSAOBlurH, AttachmentRole::PrimaryColor)
				.readFrom(PassId::ScenePrepass, AttachmentRole::Depth);
		}

		builder.beginPass(PassId::SSAOCompose, "SSAOCompose")
			.color(PipelineSurfaceId::SSAOComposedColor, AttachmentLoadOp::DontCare, AttachmentStoreOp::Store)
			.viewport(ViewportRule::SceneColorTarget)
			.readFrom(PassId::ScenePass, AttachmentRole::PrimaryColor)
			.readFrom(PassId::SSAOBlurV, AttachmentRole::PrimaryColor)
			.readFrom(PassId::ScenePrepass, AttachmentRole::Color, /*attachmentIndex=*/0);
	}

	if (fogActive)
	{
		const PassId fogColorSrc = ssaoActive ? PassId::SSAOCompose : PassId::ScenePass;
		builder.beginPass(PassId::FogApply, "FogApply")
			.color(PipelineSurfaceId::FogColor, AttachmentLoadOp::DontCare, AttachmentStoreOp::Store)
			.viewport(ViewportRule::SceneColorTarget)
			.readFrom(fogColorSrc, AttachmentRole::PrimaryColor)
			.readFrom(PassId::ScenePrepass, AttachmentRole::Depth);
	}

	const PassId lightingColor = ssaoActive ? PassId::SSAOCompose : PassId::ScenePass;
	const PassId aaColorSource = fogActive ? PassId::FogApply : lightingColor;

	const bool smaaActive = (snapshot.features & RenderFeatures::Smaa) != 0;
	const bool smaaIntermediate = (snapshot.features & RenderFeatures::SmaaIntermediate) != 0;
	if (smaaActive)
	{
		// SMAA: edge detection and blending weights at scene resolution, then
		// the neighborhood blend either into a scene-sized intermediate for a
		// following scaling pass or straight into the swapchain
		builder.beginPass(PassId::SmaaEdges, std::string("SmaaEdges"));
		builder.color(PipelineSurfaceId::SmaaEdges, AttachmentLoadOp::DontCare, AttachmentStoreOp::Store)
			.viewport(ViewportRule::SceneColorTarget)
			.readFrom(aaColorSource, AttachmentRole::PrimaryColor);

		builder.beginPass(PassId::SmaaWeights, std::string("SmaaWeights"));
		builder.color(PipelineSurfaceId::SmaaWeights, AttachmentLoadOp::DontCare, AttachmentStoreOp::Store)
			.viewport(ViewportRule::SceneColorTarget)
			.readFrom(PassId::SmaaEdges, AttachmentRole::PrimaryColor);

		builder.beginPass(PassId::SmaaBlend, std::string("SmaaBlend"));
		if (smaaIntermediate)
		{
			builder.color(PipelineSurfaceId::SmaaColor, AttachmentLoadOp::DontCare, AttachmentStoreOp::Store)
				.viewport(ViewportRule::SceneColorTarget);
		}
		else if (snapshot.swapchainMsaa)
		{
			builder.color(PipelineSurfaceId::SwapchainMSAAColor, snapshot.sceneBlitColorLoad, AttachmentStoreOp::DontCare)
				.resolve(PipelineSurfaceId::SwapchainColor, AttachmentLoadOp::DontCare, AttachmentStoreOp::Store)
				.depth(PipelineSurfaceId::SwapchainDepth, AttachmentLoadOp::Load, AttachmentStoreOp::DontCare)
				.viewport(ViewportRule::Drawable);
		}
		else
		{
			builder.color(PipelineSurfaceId::SwapchainColor, snapshot.sceneBlitColorLoad, AttachmentStoreOp::Store)
				.depth(PipelineSurfaceId::SwapchainDepth, AttachmentLoadOp::Load, AttachmentStoreOp::DontCare)
				.viewport(ViewportRule::Drawable);
		}
		builder.readFrom(aaColorSource, AttachmentRole::PrimaryColor)
			.readFrom(PassId::SmaaWeights, AttachmentRole::PrimaryColor);
	}

	// the blit or upscale chain consumes the anti-aliased intermediate when present,
	// otherwise the SSAO-composed (or raw) scene color
	const PassId sceneOutputSource = smaaIntermediate ? PassId::SmaaBlend : aaColorSource;

	if (snapshot.features & RenderFeatures::SceneUpscale)
	{
		// FSR1: edge adaptive upscale into a drawable-sized intermediate,
		// then sharpen from the intermediate into the swapchain
		builder.beginPass(PassId::SceneUpscaleEASU, std::string("SceneUpscaleEASU"));
		builder.color(PipelineSurfaceId::UpscaledColor, AttachmentLoadOp::DontCare, AttachmentStoreOp::Store)
			.viewport(ViewportRule::Drawable)
			.readFrom(sceneOutputSource, AttachmentRole::PrimaryColor);

		builder.beginPass(PassId::SceneUpscaleRCAS, std::string("SceneUpscaleRCAS"));
		if (snapshot.swapchainMsaa)
		{
			builder.color(PipelineSurfaceId::SwapchainMSAAColor, snapshot.sceneBlitColorLoad, AttachmentStoreOp::DontCare)
				.resolve(PipelineSurfaceId::SwapchainColor, AttachmentLoadOp::DontCare, AttachmentStoreOp::Store);
		}
		else
		{
			builder.color(PipelineSurfaceId::SwapchainColor, snapshot.sceneBlitColorLoad, AttachmentStoreOp::Store);
		}
		builder.depth(PipelineSurfaceId::SwapchainDepth, AttachmentLoadOp::Load, AttachmentStoreOp::DontCare)
			.viewport(ViewportRule::Drawable)
			.readFrom(PassId::SceneUpscaleEASU, AttachmentRole::PrimaryColor);
	}
	else if (!smaaActive || smaaIntermediate)
	{
		builder.beginPass(PassId::SceneBlit, std::string("SceneBlit"));
		if (snapshot.swapchainMsaa)
		{
			builder.color(PipelineSurfaceId::SwapchainMSAAColor, snapshot.sceneBlitColorLoad, AttachmentStoreOp::DontCare)
				.resolve(PipelineSurfaceId::SwapchainColor, AttachmentLoadOp::DontCare, AttachmentStoreOp::Store);
		}
		else
		{
			builder.color(PipelineSurfaceId::SwapchainColor, snapshot.sceneBlitColorLoad, AttachmentStoreOp::Store);
		}
		builder.depth(PipelineSurfaceId::SwapchainDepth, AttachmentLoadOp::Load, AttachmentStoreOp::DontCare)
			.viewport(ViewportRule::Drawable)
			.readFrom(sceneOutputSource, AttachmentRole::PrimaryColor);
	}

	addSwapchainPassToBuilder(builder, PassId::TargettingEffects, "TargettingEffects", snapshot.swapchainMsaa,
		AttachmentLoadOp::Load, AttachmentLoadOp::Clear);
	addSwapchainPassToBuilder(builder, PassId::SceneOverlays, "3DSceneOverlays", snapshot.swapchainMsaa,
		AttachmentLoadOp::Load);
	if (snapshot.features & RenderFeatures::DebugOverlays)
	{
		addSwapchainPassToBuilder(builder, PassId::SceneDebugOverlays, "3DSceneDebugOverlays", snapshot.swapchainMsaa,
			AttachmentLoadOp::Load);
	}

	if (snapshot.features & RenderFeatures::GameStartFadeSlot)
	{
		addSwapchainPassToBuilder(builder, PassId::GameStartFade, "GameStartFade", snapshot.swapchainMsaa,
			AttachmentLoadOp::Load);
	}

	addSwapchainPassToBuilder(builder, PassId::InGameUI, "InGameUI", snapshot.swapchainMsaa,
		AttachmentLoadOp::Load, AttachmentLoadOp::Clear);

	return builder.build();
}

PassGraphTopologyBlueprint buildTitleBlueprint(const RenderTopologySnapshot& snapshot)
{
	ASSERT(snapshot.screenKind == RenderScreenKind::Title, "buildTitleBlueprint: wrong screen kind");

	BlueprintBuilder builder;

	if (snapshot.features & RenderFeatures::Backdrop)
	{
		addSwapchainPassToBuilder(builder, PassId::Backdrop, "Backdrop", snapshot.swapchainMsaa,
			AttachmentLoadOp::Clear, AttachmentLoadOp::Clear);
	}

	const AttachmentLoadOp titleColorLoad = (snapshot.features & RenderFeatures::Backdrop)
		? AttachmentLoadOp::Load
		: AttachmentLoadOp::Clear;
	addSwapchainPassToBuilder(builder, PassId::TitleUI, "TitleUI", snapshot.swapchainMsaa, titleColorLoad);

	return builder.build();
}

PassGraphTopologyBlueprint buildLoadingBlueprint(const RenderTopologySnapshot& snapshot)
{
	ASSERT(snapshot.screenKind == RenderScreenKind::Loading, "buildLoadingBlueprint: wrong screen kind");

	BlueprintBuilder builder;

	if (snapshot.features & RenderFeatures::Backdrop)
	{
		addSwapchainPassToBuilder(builder, PassId::LoadingBackdrop, "LoadingBackdrop", snapshot.swapchainMsaa,
			AttachmentLoadOp::Clear, AttachmentLoadOp::Clear);
	}

	const AttachmentLoadOp loadingColorLoad = (snapshot.features & RenderFeatures::Backdrop)
		? AttachmentLoadOp::Load
		: AttachmentLoadOp::Clear;
	addSwapchainPassToBuilder(builder, PassId::LoadingScreen, "LoadingScreen", snapshot.swapchainMsaa,
		loadingColorLoad);

	return builder.build();
}

PassGraphTopologyBlueprint buildVideoBlueprint(const RenderTopologySnapshot& snapshot)
{
	ASSERT(snapshot.screenKind == RenderScreenKind::Video, "buildVideoBlueprint: wrong screen kind");

	BlueprintBuilder builder;
	// Video mode stops the backdrop pass (see loop_SetVideoPlaybackMode); always clear swapchain.
	addSwapchainPassToBuilder(builder, PassId::VideoPlayback, "VideoPlayback", snapshot.swapchainMsaa,
		AttachmentLoadOp::Clear);

	return builder.build();
}

} // anonymous namespace

PassGraphTopologyBlueprint PassGraphTopologyBlueprint::fromSnapshot(const RenderTopologySnapshot& snapshot)
{
	switch (snapshot.screenKind)
	{
	case RenderScreenKind::InGame:
		return buildInGameBlueprint(snapshot);
	case RenderScreenKind::Title:
		return buildTitleBlueprint(snapshot);
	case RenderScreenKind::Loading:
		return buildLoadingBlueprint(snapshot);
	case RenderScreenKind::Video:
		return buildVideoBlueprint(snapshot);
	}
	return {};
}

} // namespace gfx_api
