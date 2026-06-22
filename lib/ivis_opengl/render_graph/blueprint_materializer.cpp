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
/** @file blueprint_materializer.cpp
 * Implementation of blueprint materialization (surfaces, viewports, record callbacks).
 */

#include "blueprint_materializer.h"

#include "gfx_api.h"
#include "pipeline_surfaces.h"

#include "lib/framework/wzapp.h"

#include <unordered_map>

namespace gfx_api
{

BlueprintMaterializer::BlueprintMaterializer(const RenderTopologySnapshot& snapshot)
	: _snapshot(snapshot)
{
}

AttachmentDesc BlueprintMaterializer::resolveAttachment(const BlueprintAttachment& attachment) const
{
	switch (attachment.target)
	{
	case BlueprintAttachment::Target::PipelineSurface:
	{
		auto& ctx = gfx_api::context::get();
		abstract_texture* texture = ctx.getPipelineSurface(attachment.surfaceId);
		const PipelineSurfaceMeta meta = ctx.pipelineSurfaceMeta(attachment.surfaceId);
		const bool isDepth = meta.usage == PipelineSurfaceUsage::DepthStencil
			|| meta.usage == PipelineSurfaceUsage::DepthOnly;
		AttachmentDesc desc = isDepth
			? AttachmentDesc::depth(texture, attachment.loadOp, attachment.clearValue)
			: AttachmentDesc::color(texture, attachment.loadOp, attachment.clearValue);
		desc.storeOp = attachment.storeOp;
		if (attachment.arrayLayer.has_value())
		{
			desc.arrayLayer = attachment.arrayLayer.value();
		}
		return desc;
	}
	case BlueprintAttachment::Target::TransientColor:
	{
		AttachmentDesc desc = AttachmentDesc::transientColor(attachment.loadOp, attachment.clearValue);
		desc.storeOp = attachment.storeOp;
		return desc;
	}
	case BlueprintAttachment::Target::TransientDepth:
	{
		AttachmentDesc desc = AttachmentDesc::transientDepth(attachment.loadOp, attachment.clearValue);
		desc.storeOp = attachment.storeOp;
		return desc;
	}
	}
	return {};
}

std::pair<uint32_t, uint32_t> BlueprintMaterializer::resolveViewport(const BlueprintPass& pass) const
{
	switch (pass.viewportRule)
	{
	case ViewportRule::Drawable:
		return {_snapshot.drawableW, _snapshot.drawableH};
	case ViewportRule::SceneColorTarget:
		return {_snapshot.sceneW, _snapshot.sceneH};
	case ViewportRule::DepthCascade:
	{
		const size_t dim = gfx_api::context::get().getDepthPassDimensions(pass.viewportCascadeIndex);
		return {static_cast<uint32_t>(dim), static_cast<uint32_t>(dim)};
	}
	}
	return {0, 0};
}

std::vector<RenderPassDesc> BlueprintMaterializer::materialize(const PassGraphTopologyBlueprint& blueprint,
	const RecordFuncTable& recordFuncs) const
{
	std::vector<RenderPassDesc> passes;
	std::unordered_map<PassId, PassHandle> idToHandle;

	passes.reserve(blueprint.passes().size());
	idToHandle.reserve(blueprint.passes().size());

	for (const BlueprintPass& bpPass : blueprint.passes())
	{
		RenderPassDesc desc;
		desc.debugName = bpPass.debugName;
		desc.passId = bpPass.id;

		const size_t funcIndex = static_cast<size_t>(bpPass.id);
		if (funcIndex >= static_cast<size_t>(PassId::Count))
		{
			debug(LOG_ERROR, "BlueprintMaterializer: invalid PassId=%zu", funcIndex);
			return {};
		}
		desc.recordFunc = recordFuncs.funcs[funcIndex];

		desc.colorAttachments.reserve(bpPass.colorAttachments.size());
		for (const BlueprintAttachment& colorAttachment : bpPass.colorAttachments)
		{
			desc.colorAttachments.emplace_back(resolveAttachment(colorAttachment));
		}
		if (bpPass.depthAttachment.has_value())
		{
			desc.depthAttachment = resolveAttachment(bpPass.depthAttachment.value());
		}
		if (bpPass.resolveAttachment.has_value())
		{
			desc.resolveAttachment = resolveAttachment(bpPass.resolveAttachment.value());
		}

		const auto viewport = resolveViewport(bpPass);
		if (viewport.first > 0 && viewport.second > 0)
		{
			desc.viewportSize = viewport;
		}

		const PassHandle myHandle = passes.size();
		if (!idToHandle.emplace(bpPass.id, myHandle).second)
		{
			debug(LOG_ERROR, "BlueprintMaterializer: duplicate PassId=%u", static_cast<unsigned>(bpPass.id));
			return {};
		}

		desc.reads.reserve(bpPass.reads.size());
		for (const BlueprintReadEdge& edge : bpPass.reads)
		{
			ReadDesc read;
			read.source = ReadSource::PassOutput;
			const auto producerIt = idToHandle.find(edge.producerPass);
			if (producerIt == idToHandle.end())
			{
				debug(LOG_ERROR, "BlueprintMaterializer: unresolved producer PassId=%u for consumer PassId=%u",
					static_cast<unsigned>(edge.producerPass), static_cast<unsigned>(bpPass.id));
				return {};
			}
			read.producerPass = producerIt->second;
			read.producerRole = edge.producerRole;
			read.attachmentIndex = edge.attachmentIndex;
			desc.reads.emplace_back(std::move(read));
		}

		passes.emplace_back(std::move(desc));
	}

	ASSERT(passes.size() == expectedPassCount(_snapshot),
	       "BlueprintMaterializer: materialized %zu passes, expected %zu",
	       passes.size(), expectedPassCount(_snapshot));

	return passes;
}

} // namespace gfx_api
