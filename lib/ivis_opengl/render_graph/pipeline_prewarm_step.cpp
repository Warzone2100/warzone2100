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
/** @file pipeline_prewarm_step.cpp
 * `PipelinePrewarmJob::step`: building a job's pipelines, through the graphics context.
 */

#include "pipeline_prewarm.h"

#include "gfx_api.h"

namespace gfx_api
{

bool PipelinePrewarmJob::step(std::chrono::microseconds budget)
{
	if (finished())
	{
		return true;
	}
	auto& ctx = context::get();
	const auto start = std::chrono::steady_clock::now();
	// (no more than an hour: a duration's max would overflow, compared with the clock's)
	budget = std::min<std::chrono::microseconds>(budget, std::chrono::hours(1));
	// (what is built here is built ahead: not a miss to report)
	const bool reporting = ctx.pipelineMissReporting();
	ctx.setPipelineMissReporting(false);
	while (!finished())
	{
		// The pipelines of one pass (as many as the budget allows), in its render pass
		const size_t compiledIndex = _items[_next].compiledIndex;
		const CompiledPass& compiled = _compileResult.passes[compiledIndex];
		if (!ctx.beginPipelinePrewarm(_passes[compiled.graphIndex], compiled))
		{
			// (its layout can't be made: its frames will build what they bind)
			while (!finished() && _items[_next].compiledIndex == compiledIndex)
			{
				++_next;
			}
			continue;
		}
		do
		{
			_items[_next].fn();
			++_next;
		} while (!finished() && _items[_next].compiledIndex == compiledIndex
			&& std::chrono::steady_clock::now() - start < budget);
		ctx.endPipelinePrewarm();
		if (std::chrono::steady_clock::now() - start >= budget)
		{
			break;
		}
	}
	ctx.setPipelineMissReporting(reporting);
	return finished();
}

} // namespace gfx_api
