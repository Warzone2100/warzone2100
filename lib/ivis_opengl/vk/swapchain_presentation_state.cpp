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
/** @file swapchain_presentation_state.cpp
 * Drawable/swapchain sync state: cached surface extents and last-known drawable size.
 */

#include "swapchain_presentation_state.h"

#include "lib/framework/wzapp.h"

#include <algorithm>

namespace gfx_api::vk
{

void SwapchainPresentationState::syncDrawableSize(int drawableWidth, int drawableHeight)
{
	_lastKnownDrawableWidth = drawableWidth;
	_lastKnownDrawableHeight = drawableHeight;
	_drawableSizeDirty = false;
}

void SwapchainPresentationState::updateCachedExtentLimits(const ::vk::SurfaceCapabilitiesKHR& capabilities)
{
	_cachedMinImageExtent = capabilities.minImageExtent;
	_cachedMaxImageExtent = capabilities.maxImageExtent;
	_swapchainExtentLimitsCached = true;
}

void SwapchainPresentationState::invalidateCachedExtentLimits()
{
	_cachedMinImageExtent = ::vk::Extent2D{};
	_cachedMaxImageExtent = ::vk::Extent2D{};
	_swapchainExtentLimitsCached = false;
}

::vk::Extent2D SwapchainPresentationState::clampDrawableToSwapchainExtent(uint32_t drawableWidth,
	uint32_t drawableHeight) const
{
	::vk::Extent2D clamped{};
	if (!_swapchainExtentLimitsCached)
	{
		clamped.width = std::max(drawableWidth, 1u);
		clamped.height = std::max(drawableHeight, 1u);
		return clamped;
	}

	clamped.width = std::clamp(drawableWidth, _cachedMinImageExtent.width, _cachedMaxImageExtent.width);
	clamped.height = std::clamp(drawableHeight, _cachedMinImageExtent.height, _cachedMaxImageExtent.height);

	// Some drivers may return 0 for min/maxImageExtent in certain circumstances
	// (ex. some Nvidia drivers on Windows when minimizing the window).
	if (clamped.width == 0)
	{
		clamped.width = std::max(drawableWidth, 1u);
	}
	if (clamped.height == 0)
	{
		clamped.height = std::max(drawableHeight, 1u);
	}
	return clamped;
}

bool SwapchainPresentationState::drawableRequiresSwapchainRecreate(int drawableWidth, int drawableHeight,
	::vk::Extent2D swapchainSize, ::vk::SwapchainKHR swapchain) const
{
#if defined(DEBUG)
	ASSERT(!swapchain || _swapchainExtentLimitsCached,
		"drawableRequiresSwapchainRecreate with live swapchain but no cached extent limits");
#endif

	if (drawableWidth <= 0 || drawableHeight <= 0)
	{
		return swapchainSize.width > 1 || swapchainSize.height > 1;
	}

	if (!swapchain)
	{
		return true;
	}

	if (!_swapchainExtentLimitsCached)
	{
		return static_cast<uint32_t>(drawableWidth) != swapchainSize.width
			|| static_cast<uint32_t>(drawableHeight) != swapchainSize.height;
	}

	const ::vk::Extent2D clamped = clampDrawableToSwapchainExtent(
		static_cast<uint32_t>(drawableWidth),
		static_cast<uint32_t>(drawableHeight));

	return clamped.width != swapchainSize.width
		|| clamped.height != swapchainSize.height;
}

} // namespace gfx_api::vk
