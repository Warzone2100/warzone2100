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
/** @file render_pass_layout_cache.h
 * On-demand `vk::RenderPass` cache keyed by `PassLayoutKey`.
 */

#pragma once

#if defined(WZ_VULKAN_ENABLED)

#include "vk/pass_layout_key.h"

#include <vector>

struct VkRoot;

namespace gfx_api::vk
{

/// <summary>
/// On-demand `vk::RenderPass` cache keyed by `PassLayoutKey`.
///
/// Owns stored keys and creates render pass objects on cache miss. Cleared on render-graph
/// epoch bump via `VkRoot::destroyDynamicRenderPasses`. Used by `beginPass`,
/// `warmCompiledRenderGraph`, and framebuffer/PSO lookup paths.
/// </summary>
class RenderPassLayoutCache
{
public:
	explicit RenderPassLayoutCache(VkRoot& root);

	/// Returns cached render-pass layout id; creates `vk::RenderPass` on miss.
	size_t getOrCreate(const PassLayoutKey& key);
	/// Drops all cached keys and render passes (epoch invalidation).
	void clear();
	/// Returns the key for a previously issued layout id; used by framebuffer and PSO paths.
	const PassLayoutKey& keyAt(size_t layoutId) const;

private:
	VkRoot& _root;
	std::vector<PassLayoutKey> _keys;
};

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
