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
/** @file layout_sync.h
 * Pipeline stage and access flags for layout transitions (`LayoutSync`, barrier recipes).
 */

#pragma once

#if defined(WZ_VULKAN_ENABLED)

#include "vk/vulkan_hpp_include.h"

#include "render_graph/compile.h"

namespace gfx_api::vk
{

/// Pipeline stage and access flags associated with a single `vk::ImageLayout`.
struct LayoutSync
{
	::vk::PipelineStageFlags stage {};
	::vk::AccessFlags access {};
};

/// Producer-side stage/access for an image in the given layout (writes / attachment output).
LayoutSync layoutProducerSync(::vk::ImageLayout layout);
/// Consumer-side stage/access for an image in the given layout (reads / shader sampling).
LayoutSync layoutConsumerSync(::vk::ImageLayout layout);

/// <summary>
/// Source and destination sync for an image layout transition barrier.
///
/// Union of producer sync for `oldLayout` and consumer sync for `newLayout`.
/// Consumed by `PrePassBarrierEmitter` when emitting `vkCmdPipelineBarrier`.
/// </summary>
struct BarrierRecipe
{
	::vk::PipelineStageFlags srcStage = ::vk::PipelineStageFlagBits::eTopOfPipe;
	::vk::PipelineStageFlags dstStage = ::vk::PipelineStageFlagBits::eFragmentShader;
	::vk::AccessFlags srcAccess {};
	::vk::AccessFlags dstAccess = ::vk::AccessFlagBits::eShaderRead;
};

/// Builds a `BarrierRecipe` from compile-time `CompileImageLayout` old/new pair.
BarrierRecipe getBarrierRecipe(CompileImageLayout oldLayout, CompileImageLayout newLayout);

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
