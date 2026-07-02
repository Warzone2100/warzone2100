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
 * Topology snapshot construction, pass-count validation, and topology/materialize hashing.
 */

#include "topology.h"

#include "gfx_api.h"
#include "pipeline_surfaces.h"
#include "shadows.h"

#include "lib/framework/hash_combine.h"
#include "lib/framework/wzapp.h"

#include <algorithm>

namespace gfx_api
{

namespace
{

size_t expectedInGamePassCount(const RenderTopologySnapshot& snapshot)
{
	ASSERT(snapshot.screenKind == RenderScreenKind::InGame, "expectedInGamePassCount: wrong screen kind");
	size_t count = 6; // Scene, SceneBlit, Targetting, overlays, fade slot, UI
	if (snapshot.features & RenderFeatures::DebugOverlays)
	{
		++count;
	}
	if (snapshot.features & RenderFeatures::Backdrop)
	{
		++count;
	}
	count += snapshot.numShadowCascades;
	return count;
}

} // anonymous namespace

uint64_t RenderTopologySnapshot::topologyHash() const
{
	std::size_t h = 0;
	hash_combine(h,
		static_cast<std::size_t>(screenKind),
		features,
		numShadowCascades,
		sceneMsaa ? 1u : 0u,
		swapchainMsaa ? 1u : 0u,
		static_cast<std::size_t>(sceneBlitColorLoad));
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
		shadowMapSize);
	return static_cast<uint64_t>(h);
}

size_t expectedPassCount(const RenderTopologySnapshot& snapshot)
{
	switch (snapshot.screenKind)
	{
	case RenderScreenKind::InGame:
		return expectedInGamePassCount(snapshot);
	case RenderScreenKind::Title:
		return (snapshot.features & RenderFeatures::Backdrop) ? 2u : 1u;
	case RenderScreenKind::Loading:
		return (snapshot.features & RenderFeatures::Backdrop) ? 2u : 1u;
	case RenderScreenKind::Video:
		return 1u;
	}
	return 0;
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
		snapshot.features |= RenderFeatures::GameStartFadeSlot;
		if (query.debugOverlaysEnabled())
		{
			snapshot.features |= RenderFeatures::DebugOverlays;
		}
	}

	const auto drawable = query.drawableDimensions();
	snapshot.drawableW = drawable.first;
	snapshot.drawableH = drawable.second;

	const auto sceneColor = query.sceneColorDimensions();
	snapshot.sceneW = sceneColor.first;
	snapshot.sceneH = sceneColor.second;

	return snapshot;
}

} // namespace render_topology

} // namespace gfx_api
