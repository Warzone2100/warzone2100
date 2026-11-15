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
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/
/** @file game_pipeline_prewarm.cpp
 * Building the game's pipelines ahead (see game_pipeline_prewarm.h).
 */

#include "game_pipeline_prewarm.h"

#include "render_topology_game.h"
#include "terrain.h"

#include "lib/framework/frame.h"
#include "lib/framework/hash_combine.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/piedraw.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/render_graph/cached_render_graph.h"
#include "lib/ivis_opengl/render_graph/pipeline_prewarm.h"

#include <optional>
#include <unordered_set>

namespace
{

/// What the game's pipelines go by: its graph (the passes, their attachments) and the options the passes' declarations
/// read (the terrain's, the meshes')
uint64_t pipelineSignature(const gfx_api::RenderTopologySnapshot& snapshot)
{
	std::size_t h = static_cast<std::size_t>(snapshot.materializeHash());
	hash_combine(h, terrain_PipelineVariant(), pie_MeshPipelineVariant());
	return static_cast<uint64_t>(h);
}

/// A game's loading began building them (before that - the frames at start-up - there is no game to build them for)
bool started = false;
std::optional<gfx_api::PipelinePrewarmJob> job;
uint64_t jobSignature = 0;
std::chrono::steady_clock::time_point jobStart;
/// The signatures whose pipelines are built (a graph that comes and goes - the range rings' passes with a selection -
/// is checked once)
std::unordered_set<uint64_t> built;

void finishJob(const char* where)
{
	if (job.has_value())
	{
		const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - jobStart).count();
		debug(LOG_INFO, "Built the game's pipelines ahead (%s): %zu in %.1f ms", where, job->total(), ms);
	}
	built.insert(jobSignature);
	job.reset();
}

/// Each frame, before its graph: in the game, if what it binds changed (an option), build the new pipelines now
void beforeFrameGraph(const gfx_api::RenderTopologySnapshot& snapshot)
{
	auto& ctx = gfx_api::context::get();
	if (!started || snapshot.screenKind != gfx_api::RenderScreenKind::InGame)
	{
		// (the title, a loading screen, a video: not declared - their first frames build what they bind)
		ctx.setPipelineMissReporting(false);
		return;
	}
	const uint64_t signature = pipelineSignature(snapshot);
	if (built.count(signature) == 0)
	{
		// (the loading's job, if it is this graph's and not done yet: finished now)
		if (!job.has_value() || jobSignature != signature)
		{
			job = pie_GetCachedFrameRenderGraph().prewarmJob(snapshot);
			jobSignature = signature;
			jobStart = std::chrono::steady_clock::now();
		}
		while (!job->step(std::chrono::hours(1))) {}
		finishJob("before a frame");
	}
	ctx.setPipelineMissReporting(true);
}

} // anonymous namespace

void gamePipelinePrewarm_Start()
{
	started = true;
	const gfx_api::RenderTopologySnapshot snapshot = gameTopologySnapshotAsItWillShow();
	job = pie_GetCachedFrameRenderGraph().prewarmJob(snapshot);
	jobSignature = pipelineSignature(snapshot);
	jobStart = std::chrono::steady_clock::now();
}

bool gamePipelinePrewarm_Step(std::chrono::microseconds budget)
{
	if (!job.has_value())
	{
		return true;
	}
	if (job->step(budget))
	{
		finishJob("behind the loading screen");
		return true;
	}
	return false;
}

void gamePipelinePrewarm_Init()
{
	pie_SetBeforeFrameGraphHook(beforeFrameGraph);
}
