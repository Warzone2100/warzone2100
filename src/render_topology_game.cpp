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
/** @file render_topology_game.cpp
 * Game-side implementation of gfx_api::IRenderTopologyQuery for frame render-graph snapshots.
 */

#include "loop.h"
#include "main.h"
#include "wrappers.h"

#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/render_graph/topology.h"
#include "lib/ivis_opengl/screen.h"

class GameRenderTopologyQuery final : public gfx_api::IRenderTopologyQuery
{
public:
	bool isTitleScreenActive() const override
	{
		return GetGameMode() == GS_TITLE_SCREEN;
	}

	ShadowMode shadowMode() const override
	{
		return pie_getShadowMode();
	}

	bool hasBackdrop() const override
	{
		return screen_GetBackDrop();
	}

	bool headlessOrSkipDrawing() const override
	{
		return headlessGameMode() || !gfx_api::context::get().shouldDraw();
	}

	bool isLoadingScreenActive() const override
	{
		return ::isLoadingScreenActive();
	}

	bool isVideoPlaybackActive() const override
	{
		return loop_GetVideoStatus();
	}

	uint32_t numDepthPasses() const override
	{
		return static_cast<uint32_t>(gfx_api::context::get().numDepthPasses());
	}

	bool sceneMsaa() const override
	{
		return gfx_api::context::get().isSceneMSAAEnabled();
	}

	bool swapchainMsaa() const override
	{
		return gfx_api::context::get().isSwapchainMSAAEnabled();
	}

	uint64_t backendEpoch() const override
	{
		return gfx_api::context::get().getRenderGraphEpoch();
	}

	std::pair<uint32_t, uint32_t> drawableDimensions() const override
	{
		return gfx_api::context::get().getDrawableDimensions();
	}

	std::pair<uint32_t, uint32_t> sceneColorDimensions() const override
	{
		gfx_api::abstract_texture* sceneColor = gfx_api::context::get().getPipelineSurface(
			gfx_api::PipelineSurfaceId::SceneColor);
		const auto dims = gfx_api::context::get().getRenderTargetDimensions(sceneColor);
		if (dims.has_value())
		{
			return dims.value();
		}
		return {0, 0};
	}

	uint32_t shadowMapSize() const override
	{
		return static_cast<uint32_t>(gfx_api::context::get().getDepthPassDimensions(0));
	}

	bool debugOverlaysEnabled() const override
	{
		// SceneDebugOverlays pass is always present; record func is a no-op when toggles are off.
		return true;
	}
};

gfx_api::IRenderTopologyQuery& gfx_api::getGameRenderTopologyQuery()
{
	static GameRenderTopologyQuery instance;
	return instance;
}
