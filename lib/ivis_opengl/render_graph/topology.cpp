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
/** @file topology.cpp
 * Topology snapshot construction and topology/materialize hashing.
 */

#include "topology.h"

#include "pipeline_surfaces.h"
#include "shadows.h"

#include "lib/framework/hash_combine.h"

#include <algorithm>

namespace gfx_api
{

uint64_t RenderTopologySnapshot::topologyHash() const
{
	std::size_t h = 0;
	hash_combine(h,
		static_cast<std::size_t>(screenKind),
		features,
		numShadowCascades,
		sceneMsaa ? 1u : 0u,
		swapchainMsaa ? 1u : 0u,
		static_cast<std::size_t>(sceneBlitColorLoad),
		sceneEffects.ssao ? 1u : 0u,
		sceneEffects.fog ? 1u : 0u);
	return static_cast<uint64_t>(h);
}

uint64_t RenderTopologySnapshot::materializeHash() const
{
	std::size_t h = static_cast<std::size_t>(topologyHash());
	hash_combine(h,
		backendEpoch,
		drawableW,
		drawableH,
		sceneW,
		sceneH,
		shadowMapSize,
		sceneEffects.ssaoGenerateDivisor,
		sceneEffects.ssaoBlurDivisor);
	return static_cast<uint64_t>(h);
}

namespace render_topology
{

RenderTopologySnapshot snapshot(const IRenderTopologyQuery& query)
{
	RenderTopologySnapshot snapshot;

	if (query.isLoadingScreenActive())
	{
		snapshot.screenKind = RenderScreenKind::Loading;
	}
	else if (query.isVideoPlaybackActive())
	{
		snapshot.screenKind = RenderScreenKind::Video;
	}
	else if (query.isTitleScreenActive())
	{
		snapshot.screenKind = RenderScreenKind::Title;
	}
	else
	{
		snapshot.screenKind = RenderScreenKind::InGame;
	}

	snapshot.backendEpoch = query.backendEpoch();
	snapshot.sceneMsaa = query.sceneMsaa();
	snapshot.swapchainMsaa = query.swapchainMsaa();

	if (query.shadowMode() == ShadowMode::Shadow_Mapping)
	{
		snapshot.numShadowCascades = std::min<uint32_t>(query.numDepthPasses(), WZ_MAX_SHADOW_CASCADES);
		snapshot.shadowMapSize = query.shadowMapSize();
	}

	if (query.hasBackdrop())
	{
		snapshot.features |= RenderFeatures::Backdrop;
		snapshot.sceneBlitColorLoad = AttachmentLoadOp::Load;
	}

	if (snapshot.screenKind == RenderScreenKind::InGame)
	{
		if (query.inGameWorldFrozen())
		{
			snapshot.features |= RenderFeatures::FrozenWorldOverlay;
		}
		snapshot.features |= RenderFeatures::GameStartFadeSlot;
		if (query.debugOverlaysEnabled())
		{
			snapshot.features |= RenderFeatures::DebugOverlays;
		}
		if (query.sceneUpscaleActive())
		{
			snapshot.features |= RenderFeatures::SceneUpscale;
		}
		if (query.smaaActive())
		{
			snapshot.features |= RenderFeatures::Smaa;
			if (query.smaaIntermediateActive())
			{
				snapshot.features |= RenderFeatures::SmaaIntermediate;
			}
		}
	}

	const auto drawable = query.drawableDimensions();
	snapshot.drawableW = drawable.first;
	snapshot.drawableH = drawable.second;

	const auto sceneColor = query.sceneColorDimensions();
	snapshot.sceneW = sceneColor.first;
	snapshot.sceneH = sceneColor.second;

	snapshot.sceneEffects = query.sceneEffectSurfaces();
	if (snapshot.screenKind == RenderScreenKind::InGame
		&& snapshot.sceneEffects.ssao
		&& ssaoBlurIsCoarser(snapshot.sceneW, snapshot.sceneH,
			snapshot.sceneEffects.ssaoGenerateDivisor, snapshot.sceneEffects.ssaoBlurDivisor))
	{
		snapshot.features |= RenderFeatures::SSAODownsample;
	}

	return snapshot;
}

} // namespace render_topology

} // namespace gfx_api
