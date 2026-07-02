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
/** @file blueprint.cpp
 * `BlueprintBuilder` implementation and swapchain/scene pass blueprint helpers.
 */

#include "blueprint.h"

#include "lib/framework/hash_combine.h"
#include "lib/framework/wzapp.h"

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <sstream>
#include <string>

namespace gfx_api
{

namespace
{

static BlueprintAttachment makePipelineColorAttachment(PipelineSurfaceId surface, AttachmentLoadOp load,
	AttachmentStoreOp store, ClearValue clear = ClearValue::colorClear())
{
	BlueprintAttachment attachment;
	attachment.target = BlueprintAttachment::Target::PipelineSurface;
	attachment.surfaceId = surface;
	attachment.loadOp = load;
	attachment.storeOp = store;
	attachment.clearValue = clear;
	return attachment;
}

static BlueprintAttachment makePipelineDepthAttachment(PipelineSurfaceId surface, AttachmentLoadOp load,
	AttachmentStoreOp store, optional<uint32_t> arrayLayer = nullopt,
	ClearValue clear = ClearValue::depthStencilClear())
{
	BlueprintAttachment attachment;
	attachment.target = BlueprintAttachment::Target::PipelineSurface;
	attachment.surfaceId = surface;
	attachment.loadOp = load;
	attachment.storeOp = store;
	attachment.arrayLayer = arrayLayer;
	attachment.clearValue = clear;
	return attachment;
}

void hashBlueprintAttachment(std::size_t& h, const BlueprintAttachment& attachment)
{
	hash_combine(h,
		static_cast<std::size_t>(attachment.target),
		static_cast<std::size_t>(attachment.surfaceId),
		static_cast<std::size_t>(attachment.loadOp),
		static_cast<std::size_t>(attachment.storeOp),
		attachment.arrayLayer.has_value() ? 1u : 0u);
	if (attachment.arrayLayer.has_value())
	{
		hash_combine(h, attachment.arrayLayer.value());
	}
}

void hashBlueprintReadEdge(std::size_t& h, const BlueprintReadEdge& edge)
{
	hash_combine(h,
		static_cast<std::size_t>(edge.producerPass),
		static_cast<std::size_t>(edge.producerRole),
		edge.attachmentIndex);
}

} // anonymous namespace

BlueprintBuilder& BlueprintBuilder::beginPass(PassId id, std::string debugName)
{
	_passes.push_back({});
	_current = &_passes.back();
	_current->id = id;
	_current->debugName = std::move(debugName);
	return *this;
}

BlueprintBuilder& BlueprintBuilder::color(PipelineSurfaceId surface, AttachmentLoadOp load, AttachmentStoreOp store,
	ClearValue clear)
{
	ASSERT(_current != nullptr, "BlueprintBuilder::color without beginPass");
	_current->colorAttachments.push_back(makePipelineColorAttachment(surface, load, store, clear));
	return *this;
}

BlueprintBuilder& BlueprintBuilder::depth(PipelineSurfaceId surface, AttachmentLoadOp load, AttachmentStoreOp store,
	optional<uint32_t> arrayLayer, ClearValue clear)
{
	ASSERT(_current != nullptr, "BlueprintBuilder::depth without beginPass");
	_current->depthAttachment = makePipelineDepthAttachment(surface, load, store, arrayLayer, clear);
	return *this;
}

BlueprintBuilder& BlueprintBuilder::resolve(PipelineSurfaceId surface, AttachmentLoadOp load, AttachmentStoreOp store,
	ClearValue clear)
{
	ASSERT(_current != nullptr, "BlueprintBuilder::resolve without beginPass");
	_current->resolveAttachment = makePipelineColorAttachment(surface, load, store, clear);
	return *this;
}

BlueprintBuilder& BlueprintBuilder::viewport(ViewportRule rule, uint32_t cascadeIndex)
{
	ASSERT(_current != nullptr, "BlueprintBuilder::viewport without beginPass");
	_current->viewportRule = rule;
	_current->viewportCascadeIndex = cascadeIndex;
	return *this;
}

BlueprintBuilder& BlueprintBuilder::readFrom(PassId producer, AttachmentRole role, uint32_t attachmentIndex)
{
	ASSERT(_current != nullptr, "BlueprintBuilder::readFrom without beginPass");
	BlueprintReadEdge edge;
	edge.producerPass = producer;
	edge.producerRole = role;
	edge.attachmentIndex = attachmentIndex;
	_current->reads.push_back(edge);
	return *this;
}

PassGraphTopologyBlueprint BlueprintBuilder::build()
{
	PassGraphTopologyBlueprint blueprint;
	blueprint._passes = std::move(_passes);
	_current = nullptr;
	return blueprint;
}

uint64_t PassGraphTopologyBlueprint::topologyHash() const
{
	std::size_t h = 0;
	hash_combine(h, _passes.size());
	for (const BlueprintPass& pass : _passes)
	{
		hash_combine(h,
			static_cast<std::size_t>(pass.id),
			static_cast<std::size_t>(pass.viewportRule),
			pass.viewportCascadeIndex);

		for (const BlueprintAttachment& colorAttachment : pass.colorAttachments)
		{
			hashBlueprintAttachment(h, colorAttachment);
		}
		if (pass.depthAttachment.has_value())
		{
			hashBlueprintAttachment(h, pass.depthAttachment.value());
		}
		if (pass.resolveAttachment.has_value())
		{
			hashBlueprintAttachment(h, pass.resolveAttachment.value());
		}
		for (const BlueprintReadEdge& read : pass.reads)
		{
			hashBlueprintReadEdge(h, read);
		}
	}
	return static_cast<uint64_t>(h);
}

void addScenePassToBuilder(BlueprintBuilder& builder, PassId id, bool sceneMsaa)
{
	builder.beginPass(id, "ScenePass");
	if (sceneMsaa)
	{
		builder.color(PipelineSurfaceId::SceneMSAAColor, AttachmentLoadOp::Clear, AttachmentStoreOp::DontCare)
			.resolve(PipelineSurfaceId::SceneColor, AttachmentLoadOp::DontCare, AttachmentStoreOp::Store);
	}
	else
	{
		builder.color(PipelineSurfaceId::SceneColor, AttachmentLoadOp::Clear, AttachmentStoreOp::Store);
	}
	builder.depth(PipelineSurfaceId::SceneDepth, AttachmentLoadOp::Clear, AttachmentStoreOp::Invalidate)
		.viewport(ViewportRule::SceneColorTarget);
}

void addSwapchainPassToBuilder(BlueprintBuilder& builder, PassId id, std::string debugName,
	bool swapchainMsaa, AttachmentLoadOp colorLoad, optional<AttachmentLoadOp> depthLoadOverride)
{
	builder.beginPass(id, std::move(debugName));
	if (swapchainMsaa)
	{
		builder.color(PipelineSurfaceId::SwapchainMSAAColor, colorLoad, AttachmentStoreOp::DontCare)
			.resolve(PipelineSurfaceId::SwapchainColor, AttachmentLoadOp::DontCare, AttachmentStoreOp::Store);
	}
	else
	{
		builder.color(PipelineSurfaceId::SwapchainColor, colorLoad, AttachmentStoreOp::Store);
	}

	const AttachmentLoadOp depthLoad = depthLoadOverride.has_value()
		? depthLoadOverride.value()
		: ((colorLoad == AttachmentLoadOp::Clear) ? AttachmentLoadOp::Clear : AttachmentLoadOp::Load);
	builder.depth(PipelineSurfaceId::SwapchainDepth, depthLoad, AttachmentStoreOp::DontCare)
		.viewport(ViewportRule::Drawable);
}

std::string dumpBlueprint(const PassGraphTopologyBlueprint& blueprint)
{
	std::ostringstream out;
	fmt::print(out, "Blueprint passes={} hash=0x{:x}\n",
		blueprint.passes().size(), blueprint.topologyHash());
	for (size_t i = 0; i < blueprint.passes().size(); ++i)
	{
		const BlueprintPass& pass = blueprint.passes()[i];
		fmt::print(out, "  [{}] {} id={} reads={}\n",
			i, pass.debugName, static_cast<unsigned>(pass.id), pass.reads.size());
	}
	return out.str();
}

} // namespace gfx_api
