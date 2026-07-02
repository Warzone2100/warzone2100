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
/** @file pre_pass_barrier_emitter.cpp
 * Implementation of batched pre-pass barrier recording and tracker updates.
 */

#if defined(WZ_VULKAN_ENABLED)

#include "vk/pre_pass_barrier_emitter.h"

#include "gfx_api_vk.h"
#include "render_graph/layout_subresource.h"
#include "vk/frame_layout_tracker.h"
#include "vk/layout_sync.h"
#include "vk/layout_translation.h"

namespace gfx_api::vk
{

PrePassBarrierEmitter::PrePassBarrierEmitter(VkRoot& root, FrameLayoutTracker& tracker)
	: _root(root)
	, _tracker(tracker)
{
}

void PrePassBarrierEmitter::appendImageBarrier(std::vector<::vk::ImageMemoryBarrier>& outBarriers,
	::vk::PipelineStageFlags& srcStageAccum, ::vk::PipelineStageFlags& dstStageAccum,
	gfx_api::abstract_texture* texture, uint32_t arrayLayer, uint32_t mipLevel,
	::vk::ImageLayout oldLayout, ::vk::ImageLayout newLayout,
	::vk::PipelineStageFlags srcStage, ::vk::PipelineStageFlags dstStage,
	::vk::AccessFlags srcAccess, ::vk::AccessFlags dstAccess)
{
	if (texture == nullptr)
	{
		return;
	}

	srcStageAccum |= srcStage;
	dstStageAccum |= dstStage;
	outBarriers.push_back(
		::vk::ImageMemoryBarrier()
			.setImage(_root.getVkImageHandle(texture))
			.setOldLayout(oldLayout)
			.setNewLayout(newLayout)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setSubresourceRange(::vk::ImageSubresourceRange(_root.getVkImageAspect(texture), mipLevel, 1, arrayLayer, 1))
			.setSrcAccessMask(srcAccess)
			.setDstAccessMask(dstAccess));
	_tracker.set(gfx_api::layoutSubresourceKey(texture, arrayLayer, mipLevel), newLayout);
}

void PrePassBarrierEmitter::emitBatch(const gfx_api::ExecutionBatch& batch,
	const std::vector<gfx_api::CompiledPass>& compiledPasses)
{
	if (batch.count == 0)
	{
		return;
	}

	ASSERT_OR_RETURN(, batch.startIndex <= compiledPasses.size()
		&& batch.count <= compiledPasses.size() - batch.startIndex,
		"emitBatch: batch range out of bounds (start=%zu count=%zu compiled=%zu)",
		batch.startIndex, batch.count, compiledPasses.size());

	_scratch.clear();
	::vk::PipelineStageFlags srcStage {};
	::vk::PipelineStageFlags dstStage {};
	// A given subresource transitions at most once per batch (batch members share one render pass
	// and identical attachments), so dedupe cross-pass reads of the same External texture slice.
	LayoutSubresourceSet seen;

	const size_t batchEnd = batch.startIndex + batch.count;
	for (size_t b = batch.startIndex; b < batchEnd; ++b)
	{
		for (const gfx_api::ImageBarrierOp& barrierOp : compiledPasses[b].prePassBarriers)
		{
			if (barrierOp.texture == nullptr)
			{
				continue;
			}
			const gfx_api::LayoutSubresourceKey subresource = gfx_api::layoutSubresourceKey(
				barrierOp.texture, barrierOp.arrayLayer, barrierOp.mipLevel);
			if (!seen.insert(subresource).second)
			{
				continue;
			}

			const ::vk::ImageLayout vkNewLayout = toVkImageLayout(barrierOp.newLayout);
			// External reads: real previous layout lives in the runtime tracker.
			// InGraph ops carry their old layout from the compiled timeline.
			const ::vk::ImageLayout vkOldLayout = barrierOp.producerScope == gfx_api::ReadProducerScope::External
				? _tracker.get(subresource)
				: toVkImageLayout(barrierOp.oldLayout);

			// InGraph transitions already satisfied are no-ops; External reads still need a
			// pure execution/memory barrier to establish the producer -> consumer dependency.
			if (vkOldLayout == vkNewLayout && barrierOp.producerScope == gfx_api::ReadProducerScope::InGraph)
			{
				continue;
			}

			const gfx_api::CompileImageLayout recipeOldLayout = barrierOp.producerScope == gfx_api::ReadProducerScope::External
				? fromVkImageLayout(vkOldLayout)
				: barrierOp.oldLayout;
			const BarrierRecipe recipe = getBarrierRecipe(recipeOldLayout, barrierOp.newLayout);
			appendImageBarrier(_scratch, srcStage, dstStage,
				barrierOp.texture, barrierOp.arrayLayer, barrierOp.mipLevel,
				vkOldLayout, vkNewLayout,
				recipe.srcStage, recipe.dstStage, recipe.srcAccess, recipe.dstAccess);
		}
	}

	if (_scratch.empty())
	{
		return;
	}

	buffering_mechanism::get_current_resources().ensureDrawCmdBufferBegun();
	buffering_mechanism::get_current_resources().drawCmdBuffer().pipelineBarrier(
		srcStage ? srcStage : ::vk::PipelineStageFlagBits::eTopOfPipe,
		dstStage ? dstStage : ::vk::PipelineStageFlagBits::eFragmentShader,
		::vk::DependencyFlags(), nullptr, nullptr, _scratch, _root.vkDynLoader);
}

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
