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
/** @file pass_resolve.cpp
 * Implementation of pass description resolution, read binding, and batch merging rules.
 */

#include "pass_resolve.h"

#include "gfx_api.h"
#include "pipeline_surfaces.h"
#include "render_pass.h"

#include "lib/framework/wzapp.h"

#include <algorithm>

namespace gfx_api
{

namespace
{

bool attachmentIsResolved(const AttachmentDesc& attachment)
{
	return attachment.texture != nullptr;
}

bool passHasResolvedAttachments(const RenderPassDesc& pass)
{
	for (const auto& colorAttachment : pass.colorAttachments)
	{
		if (attachmentIsResolved(colorAttachment))
		{
			return true;
		}
	}
	return pass.depthAttachment.has_value() && attachmentIsResolved(pass.depthAttachment.value());
}

void tryInferPassDimensions(RenderPassDesc& pass, gfx_api::context& ctx, uint32_t& width, uint32_t& height)
{
	if (pass.viewportSize.has_value())
	{
		width = pass.viewportSize->first;
		height = pass.viewportSize->second;
		if (width > 0 && height > 0)
		{
			return;
		}
	}

	auto tryFromTexture = [&](gfx_api::abstract_texture* texture) {
		if ((width > 0 && height > 0) || texture == nullptr)
		{
			return;
		}
		const auto dims = ctx.getRenderTargetDimensions(texture);
		if (dims.has_value())
		{
			width = dims->first;
			height = dims->second;
		}
	};

	for (const auto& colorAttachment : pass.colorAttachments)
	{
		tryFromTexture(colorAttachment.texture);
	}
	if (pass.depthAttachment.has_value())
	{
		tryFromTexture(pass.depthAttachment->texture);
	}
	if (pass.resolveAttachment.has_value())
	{
		tryFromTexture(pass.resolveAttachment->texture);
	}

	if (width == 0 || height == 0)
	{
		const auto drawable = ctx.getDrawableDimensions();
		if (drawable.first > 0 && drawable.second > 0)
		{
			width = drawable.first;
			height = drawable.second;
		}
	}

	if (width > 0 && height > 0)
	{
		pass.viewportSize = std::make_pair(width, height);
	}
}

void setDefaultStoreOpIfUnset(AttachmentDesc& attachment, AttachmentStoreOp storeOp)
{
	if (!attachment.storeOp.has_value())
	{
		attachment.storeOp = storeOp;
	}
}

std::optional<PipelineSurfaceUsage> getAttachmentSurfaceUsage(gfx_api::context& ctx,
	const AttachmentDesc& attachment)
{
	if (attachment.texture == nullptr)
	{
		return std::nullopt;
	}
	const auto surfaceId = ctx.findPipelineSurfaceId(attachment.texture);
	if (!surfaceId.has_value())
	{
		return std::nullopt;
	}
	return ctx.pipelineSurfaceMeta(surfaceId.value()).usage;
}

AttachmentStoreOp defaultDepthStoreOp(gfx_api::context& ctx, const RenderPassDesc& pass,
	const AttachmentDesc& depthAttachment)
{
	const auto usage = getAttachmentSurfaceUsage(ctx, depthAttachment);
	if (usage.has_value())
	{
		if (usage.value() == PipelineSurfaceUsage::DepthOnly)
		{
			return AttachmentStoreOp::Store;
		}
		if (usage.value() == PipelineSurfaceUsage::DepthStencil)
		{
			return AttachmentStoreOp::Invalidate;
		}
	}
	if (passIsDepthOnly(pass))
	{
		return AttachmentStoreOp::Store;
	}
	if (attachmentDepthHasStencil(depthAttachment))
	{
		return AttachmentStoreOp::Invalidate;
	}
	return AttachmentStoreOp::Store;
}

void applyDefaultAttachmentStoreOps(RenderPassDesc& pass)
{
	auto& ctx = gfx_api::context::get();
	const bool needsMsaaResolve = passNeedsMsaaResolve(pass);

	for (size_t i = 0; i < pass.colorAttachments.size(); ++i)
	{
		auto& colorAttachment = pass.colorAttachments[i];
		if (needsMsaaResolve && i == 0
			&& colorAttachment.texture != nullptr
			&& ctx.isMultisampledColorAttachment(colorAttachment.texture))
		{
			setDefaultStoreOpIfUnset(colorAttachment, AttachmentStoreOp::DontCare);
		}
		else
		{
			setDefaultStoreOpIfUnset(colorAttachment, AttachmentStoreOp::Store);
		}
	}

	if (pass.resolveAttachment.has_value())
	{
		setDefaultStoreOpIfUnset(pass.resolveAttachment.value(), AttachmentStoreOp::Store);
	}

	if (pass.depthAttachment.has_value() && pass.depthAttachment->texture != nullptr)
	{
		setDefaultStoreOpIfUnset(pass.depthAttachment.value(),
			defaultDepthStoreOp(ctx, pass, pass.depthAttachment.value()));
	}
}

} // anonymous namespace

AttachmentStoreOp attachmentStoreOpOr(const AttachmentDesc& attachment)
{
	return attachment.storeOp.value_or(AttachmentStoreOp::Store);
}

bool resolvePassDescription(RenderPassDesc& pass)
{
	auto& ctx = gfx_api::context::get();

	uint32_t width = 0;
	uint32_t height = 0;
	tryInferPassDimensions(pass, ctx, width, height);

	ASSERT_OR_RETURN(false, passHasResolvedAttachments(pass),
		"Pass \"%s\" requires at least one resolved color or depth attachment", pass.debugName.c_str());
	ASSERT_OR_RETURN(false, width > 0 && height > 0,
		"Pass \"%s\" requires viewportSize or inferrable attachment dimensions", pass.debugName.c_str());

	applyDefaultAttachmentStoreOps(pass);

	return true;
}

namespace
{

bool passViewportSizesMatch(const RenderPassDesc& a, const RenderPassDesc& b)
{
	return a.viewportSize.has_value() && b.viewportSize.has_value()
		&& a.viewportSize.value() == b.viewportSize.value();
}

bool passAttachmentBindingsMatch(const RenderPassDesc& a, const RenderPassDesc& b)
{
	if (!passViewportSizesMatch(a, b))
	{
		return false;
	}
	if (a.colorAttachments.size() != b.colorAttachments.size())
	{
		return false;
	}
	for (size_t i = 0; i < a.colorAttachments.size(); ++i)
	{
		const AttachmentDesc& aAttachment = a.colorAttachments[i];
		const AttachmentDesc& bAttachment = b.colorAttachments[i];
		if (aAttachment.texture != bAttachment.texture
			|| aAttachment.arrayLayer != bAttachment.arrayLayer
			|| aAttachment.mipLevel != bAttachment.mipLevel)
		{
			return false;
		}
	}
	const bool aHasDepth = a.depthAttachment.has_value() && a.depthAttachment->texture != nullptr;
	const bool bHasDepth = b.depthAttachment.has_value() && b.depthAttachment->texture != nullptr;
	if (aHasDepth != bHasDepth)
	{
		return false;
	}
	if (aHasDepth)
	{
		const AttachmentDesc& aDepth = a.depthAttachment.value();
		const AttachmentDesc& bDepth = b.depthAttachment.value();
		if (aDepth.texture != bDepth.texture
			|| aDepth.arrayLayer != bDepth.arrayLayer
			|| aDepth.mipLevel != bDepth.mipLevel)
		{
			return false;
		}
	}
	const bool aHasResolve = a.resolveAttachment.has_value() && a.resolveAttachment->texture != nullptr;
	const bool bHasResolve = b.resolveAttachment.has_value() && b.resolveAttachment->texture != nullptr;
	if (aHasResolve != bHasResolve)
	{
		return false;
	}
	if (aHasResolve)
	{
		const AttachmentDesc& aResolve = a.resolveAttachment.value();
		const AttachmentDesc& bResolve = b.resolveAttachment.value();
		if (aResolve.texture != bResolve.texture
			|| aResolve.arrayLayer != bResolve.arrayLayer
			|| aResolve.mipLevel != bResolve.mipLevel)
		{
			return false;
		}
	}
	return true;
}

bool passAttachmentLoadOpsMatch(const RenderPassDesc& a, const RenderPassDesc& b)
{
	if (a.colorAttachments.size() != b.colorAttachments.size())
	{
		return false;
	}
	for (size_t i = 0; i < a.colorAttachments.size(); ++i)
	{
		if (a.colorAttachments[i].loadOp != b.colorAttachments[i].loadOp)
		{
			return false;
		}
	}
	const bool aHasDepth = a.depthAttachment.has_value() && a.depthAttachment->texture != nullptr;
	const bool bHasDepth = b.depthAttachment.has_value() && b.depthAttachment->texture != nullptr;
	if (aHasDepth != bHasDepth)
	{
		return false;
	}
	if (aHasDepth && a.depthAttachment->loadOp != b.depthAttachment->loadOp)
	{
		return false;
	}
	const bool aHasResolve = a.resolveAttachment.has_value() && a.resolveAttachment->texture != nullptr;
	const bool bHasResolve = b.resolveAttachment.has_value() && b.resolveAttachment->texture != nullptr;
	if (aHasResolve != bHasResolve)
	{
		return false;
	}
	if (aHasResolve && a.resolveAttachment->loadOp != b.resolveAttachment->loadOp)
	{
		return false;
	}
	return true;
}

bool passAttachmentStoreOpsMatch(const RenderPassDesc& a, const RenderPassDesc& b)
{
	if (a.colorAttachments.size() != b.colorAttachments.size())
	{
		return false;
	}
	for (size_t i = 0; i < a.colorAttachments.size(); ++i)
	{
		if (attachmentStoreOpOr(a.colorAttachments[i]) != attachmentStoreOpOr(b.colorAttachments[i]))
		{
			return false;
		}
	}
	const bool aHasDepth = a.depthAttachment.has_value() && a.depthAttachment->texture != nullptr;
	const bool bHasDepth = b.depthAttachment.has_value() && b.depthAttachment->texture != nullptr;
	if (aHasDepth != bHasDepth)
	{
		return false;
	}
	if (aHasDepth
		&& attachmentStoreOpOr(a.depthAttachment.value()) != attachmentStoreOpOr(b.depthAttachment.value()))
	{
		return false;
	}
	const bool aHasResolve = a.resolveAttachment.has_value() && a.resolveAttachment->texture != nullptr;
	const bool bHasResolve = b.resolveAttachment.has_value() && b.resolveAttachment->texture != nullptr;
	if (aHasResolve != bHasResolve)
	{
		return false;
	}
	if (aHasResolve
		&& attachmentStoreOpOr(a.resolveAttachment.value()) != attachmentStoreOpOr(b.resolveAttachment.value()))
	{
		return false;
	}
	return true;
}

bool compiledPassLayoutMetadataMatch(
	const CompiledPassLayoutMetadata& a,
	const CompiledPassLayoutMetadata& b)
{
	return a.depthInitialLayout == b.depthInitialLayout
		&& a.depthFinalLayout == b.depthFinalLayout
		&& a.colorInitialLayouts == b.colorInitialLayouts
		&& a.colorFinalLayouts == b.colorFinalLayouts
		&& a.resolveInitialLayout == b.resolveInitialLayout
		&& a.resolveFinalLayout == b.resolveFinalLayout;
}

bool compiledPassPostPassLayoutUpdatesMatch(const CompiledPass& a, const CompiledPass& b)
{
	if (a.postPassLayoutUpdates.size() != b.postPassLayoutUpdates.size())
	{
		return false;
	}
	for (size_t i = 0; i < a.postPassLayoutUpdates.size(); ++i)
	{
		const LayoutStateUpdate& lhs = a.postPassLayoutUpdates[i];
		const LayoutStateUpdate& rhs = b.postPassLayoutUpdates[i];
		if (lhs.texture != rhs.texture
			|| lhs.arrayLayer != rhs.arrayLayer
			|| lhs.mipLevel != rhs.mipLevel
			|| lhs.layout != rhs.layout)
		{
			return false;
		}
	}
	return true;
}

bool compiledPassBatchBarrierSafe(const CompiledPass& candidate)
{
	for (const ImageBarrierOp& op : candidate.prePassBarriers)
	{
		if (op.producerScope == ReadProducerScope::InGraph)
		{
			return false;
		}
	}
	return true;
}

} // anonymous namespace

bool canExtendPassBatch(const CompiledPass& batchHead, const CompiledPass& candidate)
{
	const RenderPassDesc& headDesc = batchHead.desc;
	const RenderPassDesc& candidateDesc = candidate.desc;

	if (!passAttachmentBindingsMatch(headDesc, candidateDesc))
	{
		return false;
	}
	// Batching shares one backend render pass; only the batch head's load/clear ops run.
	// Matching load ops across all attachments is sufficient — no per-candidate Clear guard needed.
	if (!passAttachmentLoadOpsMatch(headDesc, candidateDesc))
	{
		return false;
	}
	if (!passAttachmentStoreOpsMatch(headDesc, candidateDesc))
	{
		return false;
	}
	if (!compiledPassLayoutMetadataMatch(batchHead.renderPassLayouts, candidate.renderPassLayouts))
	{
		return false;
	}
	if (!compiledPassPostPassLayoutUpdatesMatch(batchHead, candidate))
	{
		return false;
	}
	if (!compiledPassBatchBarrierSafe(candidate))
	{
		return false;
	}
	return true;
}

void assignExecutionBatches(const std::vector<CompiledPass>& passes,
	std::vector<ExecutionBatch>& outBatches)
{
	outBatches.clear();

	for (size_t i = 0; i < passes.size(); )
	{
		if (passes[i].skipped)
		{
			++i;
			continue;
		}

		ExecutionBatch batch {i, 1};
		while (i + batch.count < passes.size())
		{
			const CompiledPass& next = passes[i + batch.count];
			if (next.skipped)
			{
				break;
			}
			if (!canExtendPassBatch(passes[i], next))
			{
				break;
			}
			++batch.count;
		}

#if defined(DEBUG)
		if (batch.count > 1)
		{
			const CompiledPass& head = passes[batch.startIndex];
			debug(LOG_WZ, "ExecutionBatch [%zu..%zu): head=%s count=%zu",
				batch.startIndex, batch.startIndex + batch.count,
				head.desc.debugName.c_str(), batch.count);
			for (size_t j = batch.startIndex + 1; j < batch.startIndex + batch.count; ++j)
			{
				ASSERT(compiledPassLayoutMetadataMatch(
					head.renderPassLayouts, passes[j].renderPassLayouts),
					"assignExecutionBatches: layout metadata mismatch %s vs %s",
					head.desc.debugName.c_str(), passes[j].desc.debugName.c_str());
				ASSERT(compiledPassPostPassLayoutUpdatesMatch(head, passes[j]),
					"assignExecutionBatches: postPassLayoutUpdates mismatch %s vs %s",
					head.desc.debugName.c_str(), passes[j].desc.debugName.c_str());
			}
		}
#endif

		outBatches.push_back(batch);
		i += batch.count;
	}

#if defined(DEBUG)
	size_t nonSkippedExecuted = 0;
	for (const ExecutionBatch& batch : outBatches)
	{
		nonSkippedExecuted += batch.count;
	}
	size_t nonSkippedTotal = 0;
	for (const CompiledPass& pass : passes)
	{
		if (!pass.skipped)
		{
			++nonSkippedTotal;
		}
	}
	ASSERT(nonSkippedExecuted == nonSkippedTotal,
		"assignExecutionBatches: batch partition does not cover all non-skipped passes");
#endif
}

bool passIsDepthOnly(const RenderPassDesc& pass)
{
	return pass.colorAttachments.empty()
		&& pass.depthAttachment.has_value()
		&& pass.depthAttachment->texture != nullptr;
}

bool passNeedsMsaaResolve(const RenderPassDesc& pass)
{
	if (!pass.resolveAttachment.has_value() || pass.resolveAttachment->texture == nullptr)
	{
		return false;
	}
	if (pass.colorAttachments.empty() || pass.colorAttachments[0].texture == nullptr)
	{
		return false;
	}
	return gfx_api::context::get().isMultisampledColorAttachment(pass.colorAttachments[0].texture);
}

bool attachmentDepthHasStencil(const AttachmentDesc& attachment)
{
	if (attachment.texture == nullptr)
	{
		return false;
	}
	auto& ctx = gfx_api::context::get();
	const auto surfaceId = ctx.findPipelineSurfaceId(attachment.texture);
	if (surfaceId.has_value())
	{
		return ctx.pipelineSurfaceMeta(surfaceId.value()).usage == PipelineSurfaceUsage::DepthStencil;
	}
	return false;
}

std::optional<PassOutputView> getPassOutputAttachment(
	const RenderPassDesc& producer,
	AttachmentRole role,
	uint32_t colorIndex)
{
	auto& ctx = gfx_api::context::get();
	PassOutputView view;

	switch (role)
	{
	case AttachmentRole::PrimaryColor:
		if (passNeedsMsaaResolve(producer))
		{
			if (!producer.resolveAttachment.has_value() || producer.resolveAttachment->texture == nullptr)
			{
				return std::nullopt;
			}
			view.texture = producer.resolveAttachment->texture;
			view.mipLevel = producer.resolveAttachment->mipLevel;
			view.arrayLayer = producer.resolveAttachment->arrayLayer;
		}
		else if (!producer.colorAttachments.empty() && producer.colorAttachments[0].texture != nullptr)
		{
			view.texture = producer.colorAttachments[0].texture;
			view.mipLevel = producer.colorAttachments[0].mipLevel;
			view.arrayLayer = producer.colorAttachments[0].arrayLayer;
			view.isMultisampled = ctx.isMultisampledColorAttachment(view.texture);
		}
		else
		{
			return std::nullopt;
		}
		break;
	case AttachmentRole::Resolve:
		if (!producer.resolveAttachment.has_value() || producer.resolveAttachment->texture == nullptr)
		{
			return std::nullopt;
		}
		view.texture = producer.resolveAttachment->texture;
		view.mipLevel = producer.resolveAttachment->mipLevel;
		view.arrayLayer = producer.resolveAttachment->arrayLayer;
		break;
	case AttachmentRole::Color:
		if (colorIndex >= producer.colorAttachments.size()
			|| producer.colorAttachments[colorIndex].texture == nullptr)
		{
			return std::nullopt;
		}
		view.texture = producer.colorAttachments[colorIndex].texture;
		view.mipLevel = producer.colorAttachments[colorIndex].mipLevel;
		view.arrayLayer = producer.colorAttachments[colorIndex].arrayLayer;
		view.isMultisampled = ctx.isMultisampledColorAttachment(view.texture);
		break;
	case AttachmentRole::Depth:
		if (!producer.depthAttachment.has_value() || producer.depthAttachment->texture == nullptr)
		{
			return std::nullopt;
		}
		view.texture = producer.depthAttachment->texture;
		view.mipLevel = producer.depthAttachment->mipLevel;
		view.arrayLayer = producer.depthAttachment->arrayLayer;
		view.isDepth = true;
		break;
	}

	if (view.texture == nullptr)
	{
		return std::nullopt;
	}
	return view;
}

bool isDepthShaderSampledSurface(abstract_texture* texture)
{
	if (texture == nullptr)
	{
		return false;
	}
	auto& ctx = gfx_api::context::get();
	const auto surfaceId = ctx.findPipelineSurfaceId(texture);
	return surfaceId.has_value()
		&& ctx.pipelineSurfaceMeta(surfaceId.value()).usage == PipelineSurfaceUsage::DepthOnly;
}

bool isSwapchainPresentableColorSurface(abstract_texture* texture)
{
	if (texture == nullptr)
	{
		return false;
	}
	auto& ctx = gfx_api::context::get();
	const auto surfaceId = ctx.findPipelineSurfaceId(texture);
	return surfaceId.has_value() && surfaceId.value() == PipelineSurfaceId::SwapchainColor;
}

bool passTargetsSwapchainColor(const RenderPassDesc& pass)
{
	auto& ctx = gfx_api::context::get();
	for (const auto& attachment : pass.colorAttachments)
	{
		if (attachment.texture != nullptr)
		{
			const auto surfaceId = ctx.findPipelineSurfaceId(attachment.texture);
			if (surfaceId.has_value()
				&& (surfaceId.value() == PipelineSurfaceId::SwapchainColor
					|| surfaceId.value() == PipelineSurfaceId::SwapchainMSAAColor))
			{
				return true;
			}
		}
	}
	if (pass.resolveAttachment.has_value() && pass.resolveAttachment->texture != nullptr)
	{
		const auto surfaceId = ctx.findPipelineSurfaceId(pass.resolveAttachment->texture);
		if (surfaceId.has_value() && surfaceId.value() == PipelineSurfaceId::SwapchainColor)
		{
			return true;
		}
	}
	return false;
}

std::unordered_set<abstract_texture*> getPassAttachmentTextures(const RenderPassDesc& pass)
{
	std::unordered_set<abstract_texture*> textures;
	for (const auto& colorAttachment : pass.colorAttachments)
	{
		if (colorAttachment.texture != nullptr)
		{
			textures.insert(colorAttachment.texture);
		}
	}
	if (pass.depthAttachment.has_value() && pass.depthAttachment->texture != nullptr)
	{
		textures.insert(pass.depthAttachment->texture);
	}
	if (pass.resolveAttachment.has_value() && pass.resolveAttachment->texture != nullptr)
	{
		textures.insert(pass.resolveAttachment->texture);
	}
	return textures;
}

} // namespace gfx_api
