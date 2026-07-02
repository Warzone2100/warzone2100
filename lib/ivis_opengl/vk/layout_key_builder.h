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
/** @file layout_key_builder.h
 * Builds `PassLayoutKey` from `RenderPassDesc` and optional compiled pass metadata.
 */

#pragma once

#if defined(WZ_VULKAN_ENABLED)

#include "render_graph/compile.h"
#include "render_graph/render_pass.h"
#include "vk/pass_layout_key.h"

struct VkRoot;

namespace gfx_api::vk
{

/// <summary>
/// Inputs for building a `PassLayoutKey`.
///
/// When `compiledPass` is non-null, initial/final layouts come from compile metadata.
/// When null (out-of-graph / legacy pass), layouts are resolved from the runtime
/// `FrameLayoutTracker` via `runtimeLayoutSource`.
/// </summary>
struct LayoutKeyBuildContext
{
	/// When non-null, initial/final layouts come from compiled metadata.
	const gfx_api::CompiledPass* compiledPass = nullptr;
	/// Supplies runtime layout lookups for legacy passes when `compiledPass` is null.
	const VkRoot* runtimeLayoutSource = nullptr;
};

/// Fills formats, load/store ops, and sample counts from pass attachments.
bool populatePassLayoutKeyFormats(PassLayoutKey& key, const gfx_api::RenderPassDesc& pass, const VkRoot& root);
/// Fills initial/final layouts from compile metadata or the runtime tracker.
bool populatePassLayoutKeyLayouts(PassLayoutKey& key, const gfx_api::RenderPassDesc& pass,
	const LayoutKeyBuildContext& ctx);
/// Builds a complete `PassLayoutKey` (formats then layouts).
bool buildPassLayoutKey(PassLayoutKey& out, const gfx_api::RenderPassDesc& pass,
	const LayoutKeyBuildContext& ctx, const VkRoot& root);

::vk::AttachmentLoadOp toVkAttachmentLoadOp(gfx_api::AttachmentLoadOp loadOp);
::vk::AttachmentStoreOp toVkAttachmentStoreOp(gfx_api::AttachmentStoreOp storeOp);
/// Derives RP finalLayout for a color attachment from its store op.
::vk::ImageLayout colorFinalLayoutFromStoreOp(gfx_api::AttachmentStoreOp storeOp);
/// Resolves effective color attachment initialLayout (load-op default when unset).
::vk::ImageLayout resolveColorInitialLayout(const PassLayoutKey& key, size_t colorIndex);
/// Resolves effective resolve attachment initialLayout (load-op default when unset).
::vk::ImageLayout resolveResolveInitialLayout(const PassLayoutKey& key);
/// Resolves effective depth attachment initialLayout (load-op default when unset).
::vk::ImageLayout resolveDepthInitialLayout(const PassLayoutKey& key);

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
