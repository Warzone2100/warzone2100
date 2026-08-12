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
/** @file cached_render_graph.cpp
 * Implementation of `CachedRenderGraph::ensureBuilt` and `execute`.
 */

#include "cached_render_graph.h"

#include "gfx_api.h"
#include "blueprint_materializer.h"
#include "topology.h"

#include "lib/framework/wzapp.h"

namespace gfx_api
{

void CachedRenderGraph::invalidateCache(bool clearTopologyHash)
{
	_passes.clear();
	_compileResult = {};
	_cachedMaterializeHash = 0;
	if (clearTopologyHash)
	{
		_cachedTopologyHash = 0;
	}
}

void CachedRenderGraph::setBlueprintFactory(BlueprintFactory factory)
{
	_blueprintFactory = std::move(factory);
	invalidateCache();
}

void CachedRenderGraph::setRecordFuncs(RecordFuncTable funcs)
{
	_recordFuncs = std::move(funcs);
	invalidateCache(false);
}

void CachedRenderGraph::ensureBuilt(const RenderTopologySnapshot& snapshot)
{
	const uint64_t materializeHash = snapshot.materializeHash();
	if (materializeHash == _cachedMaterializeHash && !_passes.empty())
	{
		return;
	}

	const uint64_t topologyHash = snapshot.topologyHash();
	if (topologyHash != _cachedTopologyHash)
	{
		if (!_blueprintFactory)
		{
			debug(LOG_WZ, "CachedRenderGraph: blueprint factory not set");
			invalidateCache();
			return;
		}
		_blueprint = _blueprintFactory(snapshot);
	}

	_passes = BlueprintMaterializer(snapshot).materialize(_blueprint, _recordFuncs);
	if (_passes.empty())
	{
		debug(LOG_ERROR, "CachedRenderGraph: materialize failed");
		invalidateCache();
		return;
	}

	if (!compilePassGraph(_passes, _compileResult))
	{
		debug(LOG_ERROR, "CachedRenderGraph: compile failed");
		invalidateCache();
		return;
	}

	gfx_api::context::get().warmCompiledRenderGraph(_passes, _compileResult);

#if defined(DEBUG)
	{
		const bool topologyChanged = topologyHash != _cachedTopologyHash;
		++_rebuildCount;
		debug(LOG_WZ, "CachedRenderGraph: %s (passes=%zu topologyHash=0x%llx materializeHash=0x%llx rebuild#=%llu)",
		      topologyChanged ? "topology rebuild" : "rematerialize",
		      _passes.size(),
		      static_cast<unsigned long long>(topologyHash),
		      static_cast<unsigned long long>(materializeHash),
		      static_cast<unsigned long long>(_rebuildCount));
		if (topologyChanged)
		{
			debug(LOG_WZ, "%s", dumpBlueprint(_blueprint).c_str());
		}
	}
#endif

	_cachedTopologyHash = topologyHash;
	_cachedMaterializeHash = materializeHash;
}

// Track the per frame render fraction in the viewport of every scene-sized pass, so
// fraction changes never rematerialize the graph. Execution reads the compiled pass
// description copies, not the materialized descriptions, so the rewrite must reach both.
//
// Every pass writing a MatchScene surface belongs here.
// (Any omitted pass will cover its whole target while the rest of the chain reads only the
// sub-rect, so it is sampled as - in effect - a magnified crop.)
static void applySceneFractionViewports(std::vector<RenderPassDesc>& passes, PassGraphCompileResult& compileResult)
{
	auto& ctx = gfx_api::context::get();
	const auto sceneDims = ctx.getSceneRenderTargetDimensions();
	const bool smaaIntermediate = ctx.getPipelineSurface(PipelineSurfaceId::SmaaColor) != nullptr;
	auto applyToPass = [&](RenderPassDesc& pass) {
		switch (pass.passId)
		{
		case PassId::ScenePass:
		case PassId::ScenePrepass:
		case PassId::SSAOGenerate:
		case PassId::SSAOBlurH:
		case PassId::SSAOBlurV:
		case PassId::SSAOCompose:
		case PassId::SmaaEdges:
		case PassId::SmaaWeights:
			pass.viewportSize = sceneDims;
			break;
		case PassId::SmaaBlend:
			// only when the blend writes the scene-sized intermediate, the
			// direct swapchain variant must keep covering the full drawable
			if (smaaIntermediate)
			{
				pass.viewportSize = sceneDims;
			}
			break;
		default:
			break;
		}
	};
	for (auto& pass : passes)
	{
		applyToPass(pass);
	}
	for (auto& compiled : compileResult.passes)
	{
		applyToPass(compiled.desc);
	}
}

void CachedRenderGraph::execute()
{
	if (_passes.empty())
	{
		return;
	}
	applySceneFractionViewports(_passes, _compileResult);
	gfx_api::context::get().executeCompiledRenderGraph(_passes, _compileResult);
}

} // namespace gfx_api
