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
/** @file layout_sync.cpp
 * Implementation of barrier recipes between compile-time image layouts.
 */

#if defined(WZ_VULKAN_ENABLED)

#include "vk/layout_sync.h"

#include "vk/layout_translation.h"

namespace gfx_api::vk
{

LayoutSync layoutProducerSync(::vk::ImageLayout layout)
{
	switch (layout)
	{
	case ::vk::ImageLayout::eColorAttachmentOptimal:
		return { ::vk::PipelineStageFlagBits::eColorAttachmentOutput, ::vk::AccessFlagBits::eColorAttachmentWrite };
	case ::vk::ImageLayout::eDepthStencilAttachmentOptimal:
		return {
			::vk::PipelineStageFlagBits::eEarlyFragmentTests | ::vk::PipelineStageFlagBits::eLateFragmentTests,
			::vk::AccessFlagBits::eDepthStencilAttachmentWrite
		};
	case ::vk::ImageLayout::eShaderReadOnlyOptimal:
	case ::vk::ImageLayout::eDepthStencilReadOnlyOptimal:
		return { ::vk::PipelineStageFlagBits::eFragmentShader, ::vk::AccessFlagBits::eShaderRead };
	case ::vk::ImageLayout::ePresentSrcKHR:
		return { ::vk::PipelineStageFlagBits::eBottomOfPipe, ::vk::AccessFlags() };
	case ::vk::ImageLayout::eUndefined:
	default:
		return { ::vk::PipelineStageFlagBits::eTopOfPipe, ::vk::AccessFlags() };
	}
}

LayoutSync layoutConsumerSync(::vk::ImageLayout layout)
{
	switch (layout)
	{
	case ::vk::ImageLayout::eColorAttachmentOptimal:
		// Covers blend / Load reads of the color attachment as well as writes.
		return { ::vk::PipelineStageFlagBits::eColorAttachmentOutput,
			::vk::AccessFlagBits::eColorAttachmentWrite | ::vk::AccessFlagBits::eColorAttachmentRead };
	case ::vk::ImageLayout::eDepthStencilAttachmentOptimal:
		return {
			::vk::PipelineStageFlagBits::eEarlyFragmentTests | ::vk::PipelineStageFlagBits::eLateFragmentTests,
			::vk::AccessFlagBits::eDepthStencilAttachmentWrite | ::vk::AccessFlagBits::eDepthStencilAttachmentRead
		};
	case ::vk::ImageLayout::eShaderReadOnlyOptimal:
	case ::vk::ImageLayout::eDepthStencilReadOnlyOptimal:
		return { ::vk::PipelineStageFlagBits::eFragmentShader, ::vk::AccessFlagBits::eShaderRead };
	case ::vk::ImageLayout::ePresentSrcKHR:
		return { ::vk::PipelineStageFlagBits::eBottomOfPipe, ::vk::AccessFlags() };
	case ::vk::ImageLayout::eUndefined:
	default:
		return { ::vk::PipelineStageFlagBits::eTopOfPipe, ::vk::AccessFlags() };
	}
}

BarrierRecipe getBarrierRecipe(CompileImageLayout oldLayout, CompileImageLayout newLayout)
{
	const LayoutSync src = layoutProducerSync(toVkImageLayout(oldLayout));
	const LayoutSync dst = layoutConsumerSync(toVkImageLayout(newLayout));

	BarrierRecipe recipe;
	recipe.srcStage = src.stage;
	recipe.srcAccess = src.access;
	recipe.dstStage = dst.stage;
	recipe.dstAccess = dst.access;
	return recipe;
}

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
