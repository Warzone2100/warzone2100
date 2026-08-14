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
#include "pipeline_surfaces.h"

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

static bool tracksUsedSceneExtent(SurfaceExtentPolicy policy)
{
	return policy == SurfaceExtentPolicy::MatchScene
		|| policy == SurfaceExtentPolicy::MatchSceneDivided;
}

static optional<PipelineSurfaceId> passExtentSurface(const RenderPassDesc& pass)
{
	if (!pass.colorAttachments.empty() && pass.colorAttachments[0].pipelineSurfaceId.has_value())
	{
		return pass.colorAttachments[0].pipelineSurfaceId;
	}
	if (pass.depthAttachment.has_value() && pass.depthAttachment->pipelineSurfaceId.has_value())
	{
		return pass.depthAttachment->pipelineSurfaceId;
	}
	return nullopt;
}

// Track the per-frame used scene size in every MatchScene / MatchSceneDivided writer so
// dyn-res fraction changes never rematerialize the graph. Execution reads the compiled
// pass description copies, not the materialized descriptions, so the rewrite must reach both.
//
// Viewport = resolveExtent(catalog[color-or-depth], used sceneW/H) - the same policy that
// allocated the image. MatchDrawable / ShadowMapSquare / DepthCascade stay materialized.
// (Any omitted MatchScene writer would cover its whole target while the rest of the chain
// reads only the sub-rect, so it is sampled as - in effect - a magnified crop.)
static void applyUsedCatalogViewports(std::vector<RenderPassDesc>& passes, PassGraphCompileResult& compileResult)
{
	auto& ctx = gfx_api::context::get();
	auto applyToPass = [&](RenderPassDesc& pass) {
		const optional<PipelineSurfaceId> surfaceId = passExtentSurface(pass);
		if (!surfaceId.has_value())
		{
			return;
		}
		const PipelineSurfaceCatalogEntry& cat = pipelineSurfaceCatalogEntry(surfaceId.value());
		if (!tracksUsedSceneExtent(cat.extentPolicy))
		{
			return;
		}
		pass.viewportSize = ctx.usedPipelineSurfaceExtent(surfaceId.value());
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
	applyUsedCatalogViewports(_passes, _compileResult);
	gfx_api::context::get().executeCompiledRenderGraph(_passes, _compileResult);
}

} // namespace gfx_api
