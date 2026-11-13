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
/** @file swapchain_presentation_state.h
 * Drawable/swapchain sync state: cached surface extents, dirty flags, and deferred WSI recovery.
 */

#pragma once

#include "vk/vulkan_hpp_include.h"

namespace gfx_api::vk
{

/// <summary>
/// Drawable vs swapchain sync state for resize and WSI handling.
///
/// Separates:
/// * drawable dirty - cached drawable size / UI geometry may be stale (does not imply recreate)
/// * swapchain recreate pending - WSI reported stale swapchain or present-mode change
/// * surface lost pending - surface must be recreated at frame Begin
///
/// Owned by `ScreenFrameCoordinator`.
/// </summary>
class SwapchainPresentationState
{
public:
	bool drawableSizeDirty() const { return _drawableSizeDirty; }
	bool swapchainRecreatePending() const { return _swapchainRecreatePending; }
	bool surfaceLostPending() const { return _surfaceLostPending; }
	int lastKnownDrawableWidth() const { return _lastKnownDrawableWidth; }
	int lastKnownDrawableHeight() const { return _lastKnownDrawableHeight; }

	void markDrawableSizeDirty() { _drawableSizeDirty = true; }

	/// Request swapchain recreate at next frame Begin (even if drawable extents still match).
	void requestSwapchainRecreate()
	{
		_swapchainRecreatePending = true;
		_drawableSizeDirty = true;
	}

	/// Request surface + swapchain recovery at next frame Begin.
	void requestSurfaceLostRecovery()
	{
		_surfaceLostPending = true;
		_swapchainRecreatePending = true;
		_drawableSizeDirty = true;
	}

	void clearSwapchainRecreatePending() { _swapchainRecreatePending = false; }
	void clearSurfaceLostPending() { _surfaceLostPending = false; }

	void syncDrawableSize(int drawableWidth, int drawableHeight);
	void updateCachedExtentLimits(const ::vk::SurfaceCapabilitiesKHR& capabilities);
	void invalidateCachedExtentLimits();

	::vk::Extent2D clampDrawableToSwapchainExtent(uint32_t drawableWidth, uint32_t drawableHeight) const;
	bool drawableRequiresSwapchainRecreate(int drawableWidth, int drawableHeight,
		::vk::Extent2D swapchainSize, ::vk::SwapchainKHR swapchain) const;

private:
	bool _drawableSizeDirty = true;
	bool _swapchainRecreatePending = false;
	bool _surfaceLostPending = false;
	int _lastKnownDrawableWidth = 0;
	int _lastKnownDrawableHeight = 0;
	::vk::Extent2D _cachedMinImageExtent{};
	::vk::Extent2D _cachedMaxImageExtent{};
	bool _swapchainExtentLimitsCached = false;
};

} // namespace gfx_api::vk
