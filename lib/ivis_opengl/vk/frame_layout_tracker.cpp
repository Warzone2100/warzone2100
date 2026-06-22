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
/** @file frame_layout_tracker.cpp
 * Implementation of `FrameLayoutTracker` layout get/set and present transition.
 */

#if defined(WZ_VULKAN_ENABLED)

#include "vk/frame_layout_tracker.h"

#include "gfx_api_vk.h"
#include "render_graph/layout_subresource.h"
#include "vk/layout_translation.h"

namespace gfx_api::vk
{

void FrameLayoutTracker::reset()
{
	_layouts.clear();
	_swapchainTouchedThisFrame = false;
}

void FrameLayoutTracker::beginFrame()
{
	_swapchainTouchedThisFrame = false;
}

void FrameLayoutTracker::noteSwapchainWrite()
{
	_swapchainTouchedThisFrame = true;
}

void FrameLayoutTracker::set(gfx_api::abstract_texture* texture, ::vk::ImageLayout layout)
{
	set(gfx_api::layoutSubresourceKey(texture), layout);
}

void FrameLayoutTracker::set(const gfx_api::LayoutSubresourceKey& subresource, ::vk::ImageLayout layout)
{
	if (subresource.texture != nullptr)
	{
		_layouts[subresource] = layout;
	}
}

void FrameLayoutTracker::erase(gfx_api::abstract_texture* texture)
{
	erase(gfx_api::layoutSubresourceKey(texture));
}

void FrameLayoutTracker::erase(const gfx_api::LayoutSubresourceKey& subresource)
{
	if (subresource.texture != nullptr)
	{
		_layouts.erase(subresource);
	}
}

::vk::ImageLayout FrameLayoutTracker::get(gfx_api::abstract_texture* texture) const
{
	return get(gfx_api::layoutSubresourceKey(texture));
}

::vk::ImageLayout FrameLayoutTracker::get(const gfx_api::LayoutSubresourceKey& subresource) const
{
	if (subresource.texture == nullptr)
	{
		return ::vk::ImageLayout::eUndefined;
	}
	const auto it = _layouts.find(subresource);
	if (it != _layouts.end())
	{
		return it->second;
	}
	return ::vk::ImageLayout::eUndefined;
}

void FrameLayoutTracker::applyPostPassUpdates(const gfx_api::CompiledPass& pass)
{
	for (const gfx_api::LayoutStateUpdate& update : pass.postPassLayoutUpdates)
	{
		if (update.texture == nullptr)
		{
			continue;
		}
		set(gfx_api::layoutSubresourceKey(update.texture, update.arrayLayer, update.mipLevel),
			toVkImageLayout(update.layout));
	}
}

void FrameLayoutTracker::commitLegacyFinalLayouts(const std::vector<LegacyLayoutFinal>& finals)
{
	for (const LegacyLayoutFinal& entry : finals)
	{
		set(entry.subresource, entry.layout);
	}
}

void FrameLayoutTracker::transitionSwapchainToPresent(VkRoot& root, ::vk::CommandBuffer cmd,
	gfx_api::abstract_texture* swapchainColor)
{
	if (!_swapchainTouchedThisFrame || swapchainColor == nullptr)
	{
		return;
	}

	::vk::ImageLayout oldLayout = get(swapchainColor);
	if (oldLayout == ::vk::ImageLayout::ePresentSrcKHR)
	{
		// Swapchain was written this frame but tracker was not reset to ColorAttachment after the write.
		oldLayout = ::vk::ImageLayout::eColorAttachmentOptimal;
	}
	root.transitionImageLayout(cmd, swapchainColor, oldLayout,
		::vk::ImageLayout::ePresentSrcKHR,
		::vk::PipelineStageFlagBits::eColorAttachmentOutput, ::vk::PipelineStageFlagBits::eBottomOfPipe,
		::vk::AccessFlagBits::eColorAttachmentWrite, ::vk::AccessFlags());
}

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
