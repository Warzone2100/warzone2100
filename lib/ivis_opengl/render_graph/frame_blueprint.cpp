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

	addScenePassToBuilder(builder, PassId::ScenePass, snapshot.sceneMsaa);

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
		.readFrom(PassId::ScenePass, AttachmentRole::PrimaryColor);

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
