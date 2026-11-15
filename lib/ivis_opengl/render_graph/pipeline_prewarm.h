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
/** @file pipeline_prewarm.h
 * Building a render graph's pipelines ahead of its first frame (behind a loading screen), rather than as its
 * passes first bind them (a hitch on the first frames: on Vulkan, a pipeline is built for each render pass it is
 * bound in; through MoltenVK, each is translated to Metal and compiled).
 */

#pragma once

#include "render_pass.h"
#include "render_pass_id.h"
#include "compile.h"

#include <array>
#include <chrono>
#include <functional>
#include <vector>

namespace gfx_api
{

/// Builds one pipeline for the pass being prewarmed: `pipeline_state_helper<...>::prewarm`
using PipelinePrewarmFn = void (*)();

/// What a pass's declaration can go by, as its record callback would: the pass as built for this graph (its
/// attachments: a record callback may pick its pipelines by them), and which other passes the graph has
struct PassPipelineContext
{
	const RenderPassDesc& pass;
	const std::array<bool, static_cast<size_t>(PassId::Count)>& present;

	bool graphContains(PassId id) const { return id != PassId::Count && present[static_cast<size_t>(id)]; }
};

/// What a pass may bind: every pipeline its record callback can use with the current options,
/// including the variants used only now and then (a translucent part, the first explosion)
using PassPipelinesFunc = std::function<void(const PassPipelineContext&, std::vector<PipelinePrewarmFn>&)>;

/// Per `PassId`: the pipelines its record callback may bind (empty: none declared)
struct PassPipelineTable
{
	std::array<PassPipelinesFunc, static_cast<size_t>(PassId::Count)> funcs{};

	void set(PassId id, PassPipelinesFunc func)
	{
		funcs[static_cast<size_t>(id)] = std::move(func);
	}
};

/// A graph's pipelines, built a little at a time (a loading screen's frames between). From
/// `CachedRenderGraph::prewarmJob()`: a graph compiled as the frames that will use it compile theirs, so each pass's
/// render pass layout is the one they will find
class PipelinePrewarmJob
{
public:
	PipelinePrewarmJob() = default;
	PipelinePrewarmJob(std::vector<RenderPassDesc> passes, PassGraphCompileResult compileResult, const PassPipelineTable& table);

	/// Build pipelines for about `budget` (at least one), true once all are built
	bool step(std::chrono::microseconds budget);
	bool finished() const { return _next >= _items.size(); }
	size_t built() const { return _next; }
	size_t total() const { return _items.size(); }

private:
	struct Item
	{
		size_t compiledIndex = 0;
		PipelinePrewarmFn fn = nullptr;
	};
	std::array<bool, static_cast<size_t>(PassId::Count)> _present{};
	std::vector<RenderPassDesc> _passes;
	PassGraphCompileResult _compileResult;
	std::vector<Item> _items;
	size_t _next = 0;
};

} // namespace gfx_api
