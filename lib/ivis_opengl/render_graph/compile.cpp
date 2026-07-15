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
/** @file compile.cpp
 * Pass graph compilation: attachment resolution, layout planning, and batch assignment.
 */

#include "compile.h"

#include "gfx_api.h"
#include "layout_timeline.h"
#include "pass_resolve.h"
#include "pipeline_surfaces.h"

#include "lib/framework/wzapp.h"

#include <algorithm>

namespace gfx_api
{

namespace
{

std::optional<ResolvedRead> resolveSingleRead(const ReadDesc& read,
	const std::vector<RenderPassDesc>& descs, size_t consumerIndex,
	const std::vector<CompiledPass>& compiledPasses)
{
	auto& ctx = gfx_api::context::get();
	ResolvedRead resolved;

	switch (read.source)
	{
	case ReadSource::ExplicitTexture:
		ASSERT_OR_RETURN(std::nullopt, read.texture != nullptr,
			"Pass \"%s\" read[%zu]: explicit texture is null",
			descs[consumerIndex].debugName.c_str(), consumerIndex);
		resolved.texture = read.texture;
		break;
	case ReadSource::PipelineSurface:
		resolved.texture = ctx.getPipelineSurface(read.pipelineSurface);
		ASSERT_OR_RETURN(std::nullopt, resolved.texture != nullptr,
			"Pass \"%s\": pipeline surface %u is not registered",
			descs[consumerIndex].debugName.c_str(), static_cast<unsigned>(read.pipelineSurface));
		{
			const PipelineSurfaceUsage usage = ctx.pipelineSurfaceMeta(read.pipelineSurface).usage;
			resolved.isDepth = usage == PipelineSurfaceUsage::DepthOnly
				|| usage == PipelineSurfaceUsage::DepthStencil;
		}
		break;
	case ReadSource::PassOutput:
		ASSERT_OR_RETURN(std::nullopt, read.producerPass != INVALID_PASS_HANDLE,
			"Pass \"%s\": PassOutput read has invalid producer handle",
			descs[consumerIndex].debugName.c_str());
		ASSERT_OR_RETURN(std::nullopt, read.producerPass < consumerIndex,
			"Pass \"%s\": cannot read output from pass %zu (not prior to consumer %zu)",
			descs[consumerIndex].debugName.c_str(), read.producerPass, consumerIndex);
		ASSERT_OR_RETURN(std::nullopt, read.producerPass < descs.size(),
			"Pass \"%s\": producer pass handle %zu out of range",
			descs[consumerIndex].debugName.c_str(), read.producerPass);
		if (compiledPasses[read.producerPass].skipped)
		{
			debug(LOG_ERROR,
				"Pass \"%s\": PassOutput read from skipped producer pass %zu",
				descs[consumerIndex].debugName.c_str(), read.producerPass);
			return std::nullopt;
		}
		ASSERT_OR_RETURN(std::nullopt, !passTargetsSwapchainColor(descs[read.producerPass]),
			"Pass \"%s\": cannot read PassOutput from swapchain pass %zu",
			descs[consumerIndex].debugName.c_str(), read.producerPass);
		{
			const auto output = getPassOutputAttachment(descs[read.producerPass],
				read.producerRole, read.attachmentIndex);
			ASSERT_OR_RETURN(std::nullopt, output.has_value(),
				"Pass \"%s\": failed to resolve PassOutput from pass %zu role %u",
				descs[consumerIndex].debugName.c_str(), read.producerPass,
				static_cast<unsigned>(read.producerRole));
			if (read.producerRole == AttachmentRole::Color && output->isMultisampled)
			{
				ASSERT(false,
					"Pass \"%s\": cannot read multisampled Color attachment from pass %zu; use PrimaryColor or Resolve",
					descs[consumerIndex].debugName.c_str(), read.producerPass);
				return std::nullopt;
			}
			resolved.texture = output->texture;
			resolved.arrayLayer = read.arrayLayer.has_value() ? read.arrayLayer.value() : output->arrayLayer;
			resolved.mipLevel = read.mipLevel.has_value() ? read.mipLevel.value() : output->mipLevel;
			resolved.isDepth = output->isDepth;
		}
		break;
	}

	if (read.arrayLayer.has_value() && read.source != ReadSource::PassOutput)
	{
		resolved.arrayLayer = read.arrayLayer.value();
	}
	if (read.mipLevel.has_value() && read.source != ReadSource::PassOutput)
	{
		resolved.mipLevel = read.mipLevel.value();
	}

	ASSERT_OR_RETURN(std::nullopt, resolved.texture != nullptr,
		"Pass \"%s\": unresolved read texture", descs[consumerIndex].debugName.c_str());
	return resolved;
}

bool resolvePassReads(const std::vector<RenderPassDesc>& descs, size_t passIndex,
	std::vector<ResolvedRead>& outReads,
	const std::vector<CompiledPass>& compiledPasses)
{
	outReads.clear();
	if (passIndex >= descs.size())
	{
		return false;
	}
	for (const ReadDesc& read : descs[passIndex].reads)
	{
		const auto resolved = resolveSingleRead(read, descs, passIndex, compiledPasses);
		if (!resolved.has_value())
		{
			return false;
		}
		outReads.emplace_back(resolved.value());
	}
	return true;
}

} // anonymous namespace

RenderPassContext buildRenderPassContext(const CompiledPass& compiledPass)
{
	return RenderPassContext(compiledPass.desc, compiledPass.resolvedReads);
}

bool compilePassGraph(std::vector<RenderPassDesc>& descs, PassGraphCompileResult& out)
{
	out.passes.clear();
	out.executionBatches.clear();
	out.passes.resize(descs.size());

	for (size_t i = 0; i < descs.size(); ++i)
	{
		out.passes[i].graphIndex = i;
		out.passes[i].skipped = !resolvePassDescription(descs[i]);
		if (!out.passes[i].skipped)
		{
			out.passes[i].desc = descs[i];
		}
	}

	for (size_t i = 0; i < descs.size(); ++i)
	{
		if (out.passes[i].skipped)
		{
			continue;
		}
		if (!resolvePassReads(descs, i, out.passes[i].resolvedReads, out.passes))
		{
			return false;
		}
	}

#if defined(DEBUG)
	for (const CompiledPass& p : out.passes)
	{
		if (p.skipped)
		{
			debug(LOG_WZ, "compilePassGraph: skipped \"%s\" (index=%zu)",
				descs[p.graphIndex].debugName.c_str(), p.graphIndex);
		}
	}
#endif

	if (!planLayoutTimeline(out.passes))
	{
		return false;
	}

	assignExecutionBatches(out.passes, out.executionBatches);
	return true;
}

RenderPassContext::RenderPassContext(const RenderPassDesc& desc, std::vector<ResolvedRead> resolvedReads)
	: _desc(desc)
	, _resolvedReads(std::move(resolvedReads))
{
}

abstract_texture* RenderPassContext::getRead(size_t index) const
{
	if (index >= _resolvedReads.size())
	{
		return nullptr;
	}
	return _resolvedReads[index].texture;
}

} // namespace gfx_api
