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
/** @file warm_entry.h
 * Per-graph-index warm cache entry for prebuilt render pass layout IDs.
 */

#pragma once

#if defined(WZ_VULKAN_ENABLED)

#include <cstdint>

namespace gfx_api::vk
{

/// <summary>
/// Warm-cache entry for one compiled graph pass.
///
/// Stored on `VkRoot::_warmEntries` keyed by `CompiledPass::graphIndex`. Populated by
/// `warmCompiledRenderGraph`; invalidated when the render-graph epoch bumps.
/// </summary>
struct VulkanWarmEntry
{
	static constexpr size_t INVALID_LAYOUT_ID = ~size_t(0);
	/// Cached `RenderPassLayoutCache` layout id, or `INVALID_LAYOUT_ID` when cold.
	size_t renderPassLayoutId = INVALID_LAYOUT_ID;
	/// Epoch stamp; must match `VkRoot::getRenderGraphEpoch()` for the entry to be valid.
	uint64_t warmEpoch = 0;

	static VulkanWarmEntry& invalid()
	{
		static VulkanWarmEntry kInvalid {};
		return kInvalid;
	}
};

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
