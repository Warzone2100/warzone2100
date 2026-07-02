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
/** @file cached_render_graph.h
 * Cached frame render graph: blueprint rebuild, materialize, compile, and execute.
 */

#pragma once

#include "render_pass.h"
#include "blueprint.h"
#include "compile.h"
#include "topology.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace gfx_api
{

/// <summary>
/// Frame render-graph cache: blueprint, materialized passes, and compile result.
///
/// `ensureBuilt` compares `RenderTopologySnapshot::topologyHash` and `materializeHash`
/// to avoid rebuilding blueprint vs rematerializing passes. `execute` runs the graph
/// via `executeCompiledRenderGraph`.
/// </summary>
class CachedRenderGraph
{
public:
	/// Builds `PassGraphTopologyBlueprint` when topology changes (typically `fromSnapshot`).
	using BlueprintFactory = std::function<PassGraphTopologyBlueprint(const RenderTopologySnapshot&)>;

	void setBlueprintFactory(BlueprintFactory factory);
	/// Draw callbacks keyed by `PassId`; reused across rematerializations.
	void setRecordFuncs(RecordFuncTable funcs);

	/// Rebuild or rematerialize if snapshot hashes differ; compile and warm backend layouts.
	void ensureBuilt(const RenderTopologySnapshot& snapshot);
	/// Record and submit the cached pass list for the current frame.
	void execute();

private:
	PassGraphTopologyBlueprint _blueprint;
	uint64_t _cachedTopologyHash = 0;
	uint64_t _cachedMaterializeHash = 0;
#if defined(DEBUG)
	uint64_t _rebuildCount = 0;
#endif
	std::vector<RenderPassDesc> _passes;
	PassGraphCompileResult _compileResult;
	BlueprintFactory _blueprintFactory;
	RecordFuncTable _recordFuncs;

	void invalidateCache(bool clearTopologyHash = true);
};

} // namespace gfx_api
