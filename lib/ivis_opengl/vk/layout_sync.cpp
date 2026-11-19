// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
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
	case ::vk::ImageLayout::eGeneral:
		// Compute storage images stay in GENERAL. The use enum is not kept
		// across graphs; the layout is.
		return { ::vk::PipelineStageFlagBits::eComputeShader, ::vk::AccessFlagBits::eShaderWrite };
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
	case ::vk::ImageLayout::eGeneral:
		return { ::vk::PipelineStageFlagBits::eComputeShader,
			::vk::AccessFlagBits::eShaderRead | ::vk::AccessFlagBits::eShaderWrite };
	case ::vk::ImageLayout::eUndefined:
	default:
		return { ::vk::PipelineStageFlagBits::eTopOfPipe, ::vk::AccessFlags() };
	}
}

BarrierRecipe getBarrierRecipe(CompileImageLayout oldLayout, CompileImageLayout newLayout,
	gfx_api::ImageUse srcUse, gfx_api::ImageUse dstUse)
{
	BarrierRecipe recipe;

	if (srcUse == gfx_api::ImageUse::ComputeStorage)
	{
		recipe.srcStage = ::vk::PipelineStageFlagBits::eComputeShader;
		recipe.srcAccess = ::vk::AccessFlagBits::eShaderWrite;
	}
	else if (srcUse == gfx_api::ImageUse::ComputeSampled)
	{
		recipe.srcStage = ::vk::PipelineStageFlagBits::eComputeShader;
		recipe.srcAccess = ::vk::AccessFlags();
	}
	else
	{
		const LayoutSync src = layoutProducerSync(toVkImageLayout(oldLayout));
		recipe.srcStage = src.stage;
		recipe.srcAccess = src.access;
	}

	if (dstUse == gfx_api::ImageUse::ComputeStorage)
	{
		recipe.dstStage = ::vk::PipelineStageFlagBits::eComputeShader;
		recipe.dstAccess = ::vk::AccessFlagBits::eShaderRead | ::vk::AccessFlagBits::eShaderWrite;
	}
	else if (dstUse == gfx_api::ImageUse::ComputeSampled)
	{
		recipe.dstStage = ::vk::PipelineStageFlagBits::eComputeShader;
		recipe.dstAccess = ::vk::AccessFlagBits::eShaderRead;
	}
	else
	{
		const LayoutSync dst = layoutConsumerSync(toVkImageLayout(newLayout));
		recipe.dstStage = dst.stage;
		recipe.dstAccess = dst.access;
	}

	return recipe;
}

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
