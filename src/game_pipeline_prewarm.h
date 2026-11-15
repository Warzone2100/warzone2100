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
/** @file game_pipeline_prewarm.h
 * Building the game's pipelines ahead of their first use (lib/ivis_opengl/render_graph/pipeline_prewarm.h): behind the
 * loading screen before a game's first frame (rather than in its first frames, and the first time each effect shows),
 * and again in the frame an option changes what the game binds (shadows, terrain quality, anti-aliasing, the post
 * effects), rather than as each pipeline is first bound over the frames after it.
 */

#pragma once

#include <chrono>

/// Start building the pipelines of the game as its first frame will show it (at the end of loading it)
void gamePipelinePrewarm_Start();
/// Build them for about `budget` (at least one), true once all are built
bool gamePipelinePrewarm_Step(std::chrono::microseconds budget);
/// Checks each game frame for a change in what it binds, and builds the new pipelines then (pie_InitRenderGraphs)
void gamePipelinePrewarm_Init();
