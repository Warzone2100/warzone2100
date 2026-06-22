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
/** @file frame_layout_tracker.h
 * Runtime per-subresource layout map and swapchain present transition for the current frame.
 */

#pragma once

#if defined(WZ_VULKAN_ENABLED)

#include "vk/vulkan_hpp_include.h"

#include "render_graph/compile.h"
#include "render_graph/layout_subresource.h"
#include "vk/legacy_layout_final.h"

#include <unordered_map>
#include <utility>
#include <vector>

struct VkRoot;

namespace gfx_api::vk
{

/// <summary>
/// Runtime per-subresource layout map for the current frame.
///
/// Separate from compile-time `LayoutStateMap` in `layout_timeline`. Consumed by
/// `PrePassBarrierEmitter` (External read old layouts), post-pass updates from
/// `CompiledPass`, `LegacyPassLayoutCommit`, and the single swapchain present transition.
/// </summary>
class FrameLayoutTracker
{
public:
	/// Clears all tracked layouts and the swapchain-touched flag.
	void reset();
	/// Resets per-frame swapchain write tracking at frame start.
	void beginFrame();
	/// Marks that the swapchain color surface was written this frame.
	void noteSwapchainWrite();
	void set(gfx_api::abstract_texture* texture, ::vk::ImageLayout layout);
	void set(const gfx_api::LayoutSubresourceKey& subresource, ::vk::ImageLayout layout);
	void erase(gfx_api::abstract_texture* texture);
	void erase(const gfx_api::LayoutSubresourceKey& subresource);
	/// Returns tracked layout for the subresource, or eUndefined when not in the map.
	::vk::ImageLayout get(gfx_api::abstract_texture* texture) const;
	::vk::ImageLayout get(const gfx_api::LayoutSubresourceKey& subresource) const;

	/// Applies `CompiledPass::postPassLayoutUpdates` after a graph pass ends.
	void applyPostPassUpdates(const gfx_api::CompiledPass& pass);
	/// Commits out-of-graph final layouts captured by `LegacyPassLayoutCommit`.
	void commitLegacyFinalLayouts(const std::vector<LegacyLayoutFinal>& finals);

	/// Single frame-level swapchain -> Present transition. No-op if swapchain not touched or texture is null.
	void transitionSwapchainToPresent(VkRoot& root, ::vk::CommandBuffer cmd,
		gfx_api::abstract_texture* swapchainColor);

private:
	std::unordered_map<gfx_api::LayoutSubresourceKey, ::vk::ImageLayout, gfx_api::LayoutSubresourceKeyHash> _layouts;
	bool _swapchainTouchedThisFrame = false;
};

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
