// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/
/** @file frame_render_init.cpp
 * Registers frame render-graph record callbacks and blueprint factory.
 */

#include "lib/ivis_opengl/pietypes.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

#include "frame_render_init.h"

#include "display3d_render_graph.h"
#include "display.h"
#include "hci.h"
#include "loop.h"
#include "profiling.h"
#include "titleui/titleui.h"
#include "wrappers.h"

#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/render_graph/blueprint.h"
#include "lib/ivis_opengl/render_graph/cached_render_graph.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/widget/widget.h"

static void recordBackdrop(const gfx_api::RenderPassContext&)
{
	screen_Display();
}

static void recordTitleUI(const gfx_api::RenderPassContext&)
{
	if (wzTitleUICurrent)
	{
		wzTitleUICurrent->render();
	}
}

static void recordLoadingBackdrop(const gfx_api::RenderPassContext&)
{
	screen_Display();
}

static void recordVideoPlayback(const gfx_api::RenderPassContext&)
{
	videoLoop();
}

static void recordInGameUI(const gfx_api::RenderPassContext&)
{
	wzPerfBegin(PERF_GUI, "User interface");
	WZ_PROFILE_SCOPE(DrawUI);
	pie_SetFogStatus(false);
	pie_BeginInterface();

	if (getWidgetsStatus())
	{
		intDisplayWidgets();
	}
	pie_SetFogStatus(true);
	wzPerfEnd(PERF_GUI);
}

static void registerAllFrameRecordFuncs(gfx_api::RecordFuncTable& table)
{
	registerInGame3DRecordFuncs(table);

	table.set(gfx_api::PassId::Backdrop, recordBackdrop);
	table.set(gfx_api::PassId::TitleUI, recordTitleUI);
	table.set(gfx_api::PassId::LoadingBackdrop, recordLoadingBackdrop);
	table.set(gfx_api::PassId::LoadingScreen, wrappers_recordLoadingScreen);
	table.set(gfx_api::PassId::VideoPlayback, recordVideoPlayback);
	table.set(gfx_api::PassId::GameStartFade, display_recordGameStartFade);
	table.set(gfx_api::PassId::InGameUI, recordInGameUI);
}

void pie_InitRenderGraphs()
{
	gfx_api::RecordFuncTable funcs;
	registerAllFrameRecordFuncs(funcs);

	gfx_api::CachedRenderGraph& graph = pie_GetCachedFrameRenderGraph();
	graph.setRecordFuncs(std::move(funcs));
	graph.setBlueprintFactory([](const gfx_api::RenderTopologySnapshot& snapshot) {
		return gfx_api::PassGraphTopologyBlueprint::fromSnapshot(snapshot);
	});
}
