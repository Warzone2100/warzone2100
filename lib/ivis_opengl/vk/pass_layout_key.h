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
/** @file pass_layout_key.h
 * `PassLayoutKey` identity for Vulkan render pass creation and cache lookup.
 */

#pragma once

#if defined(WZ_VULKAN_ENABLED)

#include "vk/vulkan_hpp_include.h"

#include "render_graph/attachment.h"

#include <optional>
#include <vector>

namespace gfx_api::vk
{

/// <summary>
/// Identity key for `RenderPassLayoutCache` and `vk::RenderPass` creation.
///
/// Mirrors attachment formats, load/store ops, samples, and initial/final layouts.
/// Built by `layout_key_builder` from `RenderPassDesc` and optional
/// `CompiledPass::renderPassLayouts`. Dimensions are keyed separately via
/// `FramebufferResourceKey`.
/// </summary>
struct PassLayoutKey
{
	std::vector<::vk::Format> colorFormats;
	std::vector<AttachmentLoadOp> colorLoadOps;
	std::vector<AttachmentStoreOp> colorStoreOps;
	std::vector<::vk::SampleCountFlagBits> colorSamples;
	std::optional<::vk::Format> resolveFormat;
	AttachmentLoadOp resolveLoadOp = AttachmentLoadOp::DontCare;
	AttachmentStoreOp resolveStoreOp = AttachmentStoreOp::Store;
	std::optional<::vk::Format> depthFormat;
	AttachmentLoadOp depthLoadOp = AttachmentLoadOp::DontCare;
	AttachmentStoreOp depthStoreOp = AttachmentStoreOp::DontCare;
	/// Layout each color attachment is in as the pass begins (RP initialLayout).
	/// Empty => fall back to a load-op derived default.
	std::vector<::vk::ImageLayout> colorInitialLayouts;
	/// Layout the resolve attachment is in as the pass begins (RP initialLayout).
	std::optional<::vk::ImageLayout> resolveInitialLayout;
	/// Layout the depth attachment is in as the pass begins (RP initialLayout).
	std::optional<::vk::ImageLayout> depthInitialLayout;
	/// Layout the depth attachment is left in once the pass ends (RP finalLayout).
	::vk::ImageLayout depthFinalLayout = ::vk::ImageLayout::eDepthStencilAttachmentOptimal;
	/// Layout each color attachment is left in once the pass ends (RP finalLayout).
	std::vector<::vk::ImageLayout> colorFinalLayouts;
	/// Layout the resolve attachment is left in once the pass ends (RP finalLayout).
	std::optional<::vk::ImageLayout> resolveFinalLayout;

	bool operator==(const PassLayoutKey& other) const;

	static const PassLayoutKey& invalid()
	{
		static const PassLayoutKey kInvalid {};
		return kInvalid;
	}
};

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
