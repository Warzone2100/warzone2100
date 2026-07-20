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
/** @file swapchain_presentation_state.h
 * Drawable/swapchain sync state: cached surface extents and last-known drawable size.
 */

#pragma once

#include "vk/vulkan_hpp_include.h"

namespace gfx_api::vk
{

/// <summary>
/// Drawable vs swapchain sync state for resize and WSI handling.
///
/// Tracks drawable dirty flag, last-known drawable size, and cached surface min/max extents.
/// Owned by `ScreenFrameCoordinator`; supports O(1) recreate checks and extent clamping at
/// swapchain create/finish without querying WSI every frame.
/// </summary>
class SwapchainPresentationState
{
public:
	bool drawableSizeDirty() const { return _drawableSizeDirty; }
	int lastKnownDrawableWidth() const { return _lastKnownDrawableWidth; }
	int lastKnownDrawableHeight() const { return _lastKnownDrawableHeight; }

	void markDrawableSizeDirty() { _drawableSizeDirty = true; }
	void syncDrawableSize(int drawableWidth, int drawableHeight);
	void updateCachedExtentLimits(const ::vk::SurfaceCapabilitiesKHR& capabilities);
	void invalidateCachedExtentLimits();

	::vk::Extent2D clampDrawableToSwapchainExtent(uint32_t drawableWidth, uint32_t drawableHeight) const;
	bool drawableRequiresSwapchainRecreate(int drawableWidth, int drawableHeight,
		::vk::Extent2D swapchainSize, ::vk::SwapchainKHR swapchain) const;

private:
	bool _drawableSizeDirty = true;
	int _lastKnownDrawableWidth = 0;
	int _lastKnownDrawableHeight = 0;
	::vk::Extent2D _cachedMinImageExtent{};
	::vk::Extent2D _cachedMaxImageExtent{};
	bool _swapchainExtentLimitsCached = false;
};

} // namespace gfx_api::vk
