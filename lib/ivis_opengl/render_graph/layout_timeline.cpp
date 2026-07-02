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
/** @file layout_timeline.cpp
 * Implementation of layout timeline planning across compiled passes.
 */

#include "layout_timeline.h"

#include "gfx_api.h"
#include "layout_subresource.h"
#include "pass_resolve.h"
#include "pipeline_surfaces.h"
#include "read_scope.h"

#include <unordered_map>

namespace gfx_api
{

namespace
{

CompileImageLayout getReadTargetLayout(const ResolvedRead& read)
{
	return read.isDepth ? CompileImageLayout::DepthReadOnly : CompileImageLayout::ShaderReadOnly;
}

enum class PostPassAttachmentKind : uint8_t
{
	Color,
	Resolve,
	Depth,
};

LayoutStateUpdate makeLayoutStateUpdate(const LayoutSubresourceKey& subresource, CompileImageLayout layout)
{
	LayoutStateUpdate update;
	update.texture = subresource.texture;
	update.arrayLayer = subresource.arrayLayer;
	update.mipLevel = subresource.mipLevel;
	update.layout = layout;
	return update;
}

// Final (post-pass) layout for a stored attachment, derived purely from whether the
// produced texture is sampled by a later pass before being overwritten.
//
// The decision is intentionally surface-agnostic: the swapchain is treated like any
// other color target (left in ColorAttachment), and the single PresentSrcKHR transition
// is handled once per frame by the backend rather than baked into a render pass.
std::optional<CompileImageLayout> getAttachmentPostPassLayout(
	const RenderPassDesc& pass,
	const AttachmentDesc& attachment,
	PostPassAttachmentKind kind,
	const LayoutSubresourceSet& sampledAfter,
	size_t colorIndex = 0)
{
	if (attachment.texture == nullptr)
	{
		return std::nullopt;
	}

	const AttachmentStoreOp storeOp = attachmentStoreOpOr(attachment);
	if (storeOp == AttachmentStoreOp::DontCare || storeOp == AttachmentStoreOp::Invalidate
		|| storeOp == AttachmentStoreOp::Resolve)
	{
		return std::nullopt;
	}

	const LayoutSubresourceKey subresource = layoutSubresourceKey(attachment);
	const bool sampledLater = sampledAfter.count(subresource) > 0;

	switch (kind)
	{
	case PostPassAttachmentKind::Color:
		// The multisampled color buffer of an MSAA-resolve pass is never the stored
		// output (it is resolved, not stored); the resolve attachment carries it.
		if (passNeedsMsaaResolve(pass) && colorIndex == 0)
		{
			return std::nullopt;
		}
		return sampledLater ? CompileImageLayout::ShaderReadOnly : CompileImageLayout::ColorAttachment;
	case PostPassAttachmentKind::Resolve:
		if (!passNeedsMsaaResolve(pass))
		{
			return std::nullopt;
		}
		return sampledLater ? CompileImageLayout::ShaderReadOnly : CompileImageLayout::ColorAttachment;
	case PostPassAttachmentKind::Depth:
		return sampledLater ? CompileImageLayout::DepthReadOnly : CompileImageLayout::DepthAttachment;
	}

	return std::nullopt;
}

// Layout an attachment is in as the pass begins (RP initialLayout).
// Clear/DontCare discard previous contents, so the cheapest correct layout is Undefined.
// Load preserves contents, so we must name the layout the producer left the texture in.
CompileImageLayout incomingInitialLayout(const AttachmentDesc& attachment, const LayoutStateMap& layoutState)
{
	if (attachment.texture == nullptr || attachment.loadOp != AttachmentLoadOp::Load)
	{
		return CompileImageLayout::Undefined;
	}
	const auto it = layoutState.find(layoutSubresourceKey(attachment));
	return (it != layoutState.end()) ? it->second : CompileImageLayout::Undefined;
}

void populateCompiledPassInitialLayouts(CompiledPass& compiledPass, const LayoutStateMap& layoutState)
{
	const RenderPassDesc& pass = compiledPass.desc;
	CompiledPassLayoutMetadata& metadata = compiledPass.renderPassLayouts;
	metadata.colorInitialLayouts.clear();
	metadata.resolveInitialLayout.reset();
	metadata.depthInitialLayout = CompileImageLayout::Undefined;

	metadata.colorInitialLayouts.reserve(pass.colorAttachments.size());
	for (const AttachmentDesc& colorAttachment : pass.colorAttachments)
	{
		metadata.colorInitialLayouts.emplace_back(incomingInitialLayout(colorAttachment, layoutState));
	}

	if (pass.resolveAttachment.has_value())
	{
		metadata.resolveInitialLayout = incomingInitialLayout(pass.resolveAttachment.value(), layoutState);
	}

	if (pass.depthAttachment.has_value())
	{
		metadata.depthInitialLayout = incomingInitialLayout(pass.depthAttachment.value(), layoutState);
	}
}

void populateCompiledPassLayoutMetadata(CompiledPass& compiledPass,
	const LayoutSubresourceSet& sampledAfter)
{
	const RenderPassDesc& pass = compiledPass.desc;
	CompiledPassLayoutMetadata& metadata = compiledPass.renderPassLayouts;
	metadata.colorFinalLayouts.clear();
	metadata.resolveFinalLayout.reset();
	metadata.depthFinalLayout = CompileImageLayout::DepthAttachment;

	metadata.colorFinalLayouts.reserve(pass.colorAttachments.size());
	for (size_t i = 0; i < pass.colorAttachments.size(); ++i)
	{
		const AttachmentDesc& colorAttachment = pass.colorAttachments[i];
		const auto postPassLayout = getAttachmentPostPassLayout(pass, colorAttachment,
			PostPassAttachmentKind::Color, sampledAfter, i);
		// nullopt: attachment-optimal RP final layout; compile tracker left unchanged.
		metadata.colorFinalLayouts.emplace_back(postPassLayout.value_or(CompileImageLayout::ColorAttachment));
	}

	if (pass.resolveAttachment.has_value())
	{
		const auto postPassLayout = getAttachmentPostPassLayout(pass, pass.resolveAttachment.value(),
			PostPassAttachmentKind::Resolve, sampledAfter);
		metadata.resolveFinalLayout = postPassLayout.value_or(CompileImageLayout::ColorAttachment);
	}

	if (pass.depthAttachment.has_value())
	{
		const auto postPassLayout = getAttachmentPostPassLayout(pass, pass.depthAttachment.value(),
			PostPassAttachmentKind::Depth, sampledAfter);
		metadata.depthFinalLayout = postPassLayout.value_or(CompileImageLayout::DepthAttachment);
	}
}

void applyPostPassLayoutUpdates(CompiledPass& compiledPass, LayoutStateMap& layoutState,
	const LayoutSubresourceSet& sampledAfter)
{
	const RenderPassDesc& pass = compiledPass.desc;

	if (passNeedsMsaaResolve(pass) && pass.resolveAttachment.has_value())
	{
		const AttachmentDesc& resolveAttachment = pass.resolveAttachment.value();
		const LayoutSubresourceKey subresource = layoutSubresourceKey(resolveAttachment);
		const auto layout = getAttachmentPostPassLayout(pass, resolveAttachment,
			PostPassAttachmentKind::Resolve, sampledAfter);
		if (layout.has_value())
		{
			compiledPass.postPassLayoutUpdates.push_back(makeLayoutStateUpdate(subresource, layout.value()));
			layoutState[subresource] = layout.value();
		}
	}
	else
	{
		for (size_t i = 0; i < pass.colorAttachments.size(); ++i)
		{
			const AttachmentDesc& colorAttachment = pass.colorAttachments[i];
			const LayoutSubresourceKey subresource = layoutSubresourceKey(colorAttachment);
			const auto layout = getAttachmentPostPassLayout(pass, colorAttachment,
				PostPassAttachmentKind::Color, sampledAfter, i);
			if (layout.has_value())
			{
				compiledPass.postPassLayoutUpdates.push_back(makeLayoutStateUpdate(subresource, layout.value()));
				layoutState[subresource] = layout.value();
			}
		}
	}

	if (pass.depthAttachment.has_value())
	{
		const AttachmentDesc& depthAttachment = pass.depthAttachment.value();
		const LayoutSubresourceKey subresource = layoutSubresourceKey(depthAttachment);
		const auto layout = getAttachmentPostPassLayout(pass, depthAttachment,
			PostPassAttachmentKind::Depth, sampledAfter);
		if (layout.has_value())
		{
			compiledPass.postPassLayoutUpdates.push_back(makeLayoutStateUpdate(subresource, layout.value()));
			layoutState[subresource] = layout.value();
		}
	}
}

// For each pass, the set of subresources it is the (most recent) producer of that some later
// pass samples before they are overwritten. This is the single source of truth for whether
// a produced attachment must end the pass in a read-only layout.
std::vector<LayoutSubresourceSet> computeSampledAfter(
	const std::vector<CompiledPass>& passes)
{
	std::vector<LayoutSubresourceSet> sampledAfter(passes.size());
	std::unordered_map<LayoutSubresourceKey, size_t, LayoutSubresourceKeyHash> lastProducer;

	for (size_t i = 0; i < passes.size(); ++i)
	{
		const CompiledPass& compiledPass = passes[i];
		if (compiledPass.skipped)
		{
			continue;
		}
		const RenderPassDesc& pass = compiledPass.desc;

		// Attribute each sample at this pass to the most recent earlier producer of that subresource.
		LayoutSubresourceSet seenReads;
		for (const ResolvedRead& read : compiledPass.resolvedReads)
		{
			const LayoutSubresourceKey readSubresource = layoutSubresourceKey(read);
			if (read.texture == nullptr || !seenReads.insert(readSubresource).second)
			{
				continue;
			}
			if (isPassAttachmentSubresource(pass, readSubresource))
			{
				continue;
			}
			const auto producerIt = lastProducer.find(readSubresource);
			if (producerIt != lastProducer.end())
			{
				sampledAfter[producerIt->second].insert(readSubresource);
			}
		}

		// Register this pass as the producer of each stored attachment output.
		const auto registerStored = [&](const AttachmentDesc& attachment)
		{
			if (attachment.texture != nullptr && attachmentStoreOpOr(attachment) == AttachmentStoreOp::Store)
			{
				lastProducer[layoutSubresourceKey(attachment)] = i;
			}
		};
		for (const AttachmentDesc& colorAttachment : pass.colorAttachments)
		{
			registerStored(colorAttachment);
		}
		if (pass.resolveAttachment.has_value())
		{
			registerStored(pass.resolveAttachment.value());
		}
		if (pass.depthAttachment.has_value())
		{
			registerStored(pass.depthAttachment.value());
		}
	}

	return sampledAfter;
}

} // anonymous namespace

bool planLayoutTimeline(std::vector<CompiledPass>& passes)
{
	LayoutStateMap layoutState;

	std::vector<RenderPassDesc> passDescs;
	passDescs.reserve(passes.size());
	for (const CompiledPass& compiledPass : passes)
	{
		passDescs.push_back(compiledPass.desc);
	}

	auto& ctx = gfx_api::context::get();
	if (gfx_api::abstract_texture* swapchainColor = ctx.getPipelineSurface(PipelineSurfaceId::SwapchainColor))
	{
		// Across frames the swapchain image starts in PresentSrcKHR (left there by the
		// previous frame's single present transition). This only matters for a Load.
		layoutState[layoutSubresourceKey(swapchainColor)] = CompileImageLayout::Present;
	}

	const auto sampledAfter = computeSampledAfter(passes);

	for (size_t passIndex = 0; passIndex < passes.size(); ++passIndex)
	{
		CompiledPass& compiledPass = passes[passIndex];
		compiledPass.prePassBarriers.clear();
		compiledPass.postPassLayoutUpdates.clear();
		compiledPass.renderPassLayouts = CompiledPassLayoutMetadata {};
		if (compiledPass.skipped)
		{
			continue;
		}

		const RenderPassDesc& pass = compiledPass.desc;

		LayoutSubresourceSet seenReadSubresources;
		for (size_t readIndex = 0; readIndex < compiledPass.resolvedReads.size(); ++readIndex)
		{
			const ResolvedRead& read = compiledPass.resolvedReads[readIndex];
			const ReadDesc& readDesc = compiledPass.desc.reads[readIndex];
			const LayoutSubresourceKey readSubresource = layoutSubresourceKey(read);
			if (read.texture == nullptr || !seenReadSubresources.insert(readSubresource).second)
			{
				continue;
			}
			if (isPassAttachmentSubresource(pass, readSubresource))
			{
				continue;
			}

			const ReadProducerScope scope = classifyReadProducerScope(readDesc, passDescs, passIndex, layoutState);
			const CompileImageLayout targetLayout = getReadTargetLayout(read);

			if (scope == ReadProducerScope::External)
			{
				ImageBarrierOp barrier;
				barrier.texture = read.texture;
				barrier.arrayLayer = read.arrayLayer;
				barrier.mipLevel = read.mipLevel;
				barrier.isDepth = read.isDepth;
				barrier.producerScope = ReadProducerScope::External;
				barrier.oldLayout = CompileImageLayout::Undefined; // resolved at runtime
				barrier.newLayout = targetLayout;
				compiledPass.prePassBarriers.emplace_back(std::move(barrier));
				layoutState[readSubresource] = targetLayout;
				continue;
			}

			const auto layoutIt = layoutState.find(readSubresource);
			ASSERT_OR_RETURN(false, layoutIt != layoutState.end(),
				"Pass \"%s\": InGraph read missing layout state (texture=%p layer=%u mip=%u)",
				pass.debugName.c_str(),
				static_cast<void*>(readSubresource.texture),
				readSubresource.arrayLayer,
				readSubresource.mipLevel);

			const CompileImageLayout currentLayout = layoutIt->second;
			if (currentLayout != targetLayout)
			{
				ImageBarrierOp barrier;
				barrier.texture = read.texture;
				barrier.arrayLayer = read.arrayLayer;
				barrier.mipLevel = read.mipLevel;
				barrier.isDepth = read.isDepth;
				barrier.producerScope = ReadProducerScope::InGraph;
				barrier.oldLayout = currentLayout;
				barrier.newLayout = targetLayout;
				compiledPass.prePassBarriers.emplace_back(std::move(barrier));
				layoutState[readSubresource] = targetLayout;
			}
		}

		// Record the layout each attachment is in as the pass begins (before this pass
		// mutates the tracker), then advance the tracker / record final layouts.
		populateCompiledPassInitialLayouts(compiledPass, layoutState);
		applyPostPassLayoutUpdates(compiledPass, layoutState, sampledAfter[passIndex]);
		populateCompiledPassLayoutMetadata(compiledPass, sampledAfter[passIndex]);
	}

	return true;
}

} // namespace gfx_api
