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
/** @file pipeline_prewarm.cpp
 * Implementation of `PipelinePrewarmJob`: what it builds (its building: pipeline_prewarm_step.cpp - apart, as it
 * needs the graphics context, and this part is tested without one: tests/render_graph_pass_sequence_test.cpp).
 */

#include "pipeline_prewarm.h"

#include <algorithm>

namespace gfx_api
{

PipelinePrewarmJob::PipelinePrewarmJob(std::vector<RenderPassDesc> passes, PassGraphCompileResult compileResult, const PassPipelineTable& table)
	: _passes(std::move(passes))
	, _compileResult(std::move(compileResult))
{
	for (const CompiledPass& compiled : _compileResult.passes)
	{
		if (!compiled.skipped && compiled.graphIndex < _passes.size() && _passes[compiled.graphIndex].passId != PassId::Count)
		{
			_present[static_cast<size_t>(_passes[compiled.graphIndex].passId)] = true;
		}
	}
	std::vector<PipelinePrewarmFn> fns;
	for (size_t i = 0; i < _compileResult.passes.size(); ++i)
	{
		const CompiledPass& compiled = _compileResult.passes[i];
		if (compiled.skipped || compiled.graphIndex >= _passes.size())
		{
			continue;
		}
		const PassId id = _passes[compiled.graphIndex].passId;
		if (id == PassId::Count || !table.funcs[static_cast<size_t>(id)])
		{
			continue;
		}
		fns.clear();
		table.funcs[static_cast<size_t>(id)](PassPipelineContext{_passes[compiled.graphIndex], _present}, fns);
		// (a pass may list one twice: once is enough)
		std::sort(fns.begin(), fns.end(), [](PipelinePrewarmFn a, PipelinePrewarmFn b) { return reinterpret_cast<uintptr_t>(a) < reinterpret_cast<uintptr_t>(b); });
		fns.erase(std::unique(fns.begin(), fns.end()), fns.end());
		for (PipelinePrewarmFn fn : fns)
		{
			if (fn != nullptr)
			{
				_items.push_back(Item{i, fn});
			}
		}
	}
}

} // namespace gfx_api
