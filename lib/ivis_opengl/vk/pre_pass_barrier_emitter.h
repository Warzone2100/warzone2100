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
/** @file pre_pass_barrier_emitter.h
 * Coalesced emission of compiled pre-pass image barriers before `beginPass`.
 */

#pragma once

#if defined(WZ_VULKAN_ENABLED)

#include "vk/vulkan_hpp_include.h"

#include "render_graph/compile.h"

#include <vector>

struct VkRoot;

namespace gfx_api::vk
{

class FrameLayoutTracker;

/// <summary>
/// Batches `CompiledPass::prePassBarriers` into coalesced `vkCmdPipelineBarrier` calls.
///
/// Honors `ReadProducerScope::External` (runtime old layout, always emit) vs
/// `ReadProducerScope::InGraph` (compile-time old layout, skip when old == new).
/// Invoked from `VkRoot::emitPrePassBarriers` before `beginPass`.
/// </summary>
class PrePassBarrierEmitter
{
public:
	PrePassBarrierEmitter(VkRoot& root, FrameLayoutTracker& tracker);

	/// Coalesced batch emit before beginPass. Ensures draw cmd buffer begun.
	void emitBatch(const gfx_api::ExecutionBatch& batch,
		const std::vector<gfx_api::CompiledPass>& compiledPasses);

private:
	void appendImageBarrier(std::vector<::vk::ImageMemoryBarrier>& outBarriers,
		::vk::PipelineStageFlags& srcStageAccum, ::vk::PipelineStageFlags& dstStageAccum,
		gfx_api::abstract_texture* texture, uint32_t arrayLayer, uint32_t mipLevel,
		::vk::ImageLayout oldLayout, ::vk::ImageLayout newLayout,
		::vk::PipelineStageFlags srcStage, ::vk::PipelineStageFlags dstStage,
		::vk::AccessFlags srcAccess, ::vk::AccessFlags dstAccess);

	VkRoot& _root;
	FrameLayoutTracker& _tracker;
	std::vector<::vk::ImageMemoryBarrier> _scratch;
};

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
