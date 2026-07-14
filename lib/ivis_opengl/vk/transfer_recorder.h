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
/** @file transfer_recorder.h
 * Typed wrapper for per-frame GPU upload / copy command recording.
 * Automatically marks transfer work on the current ring slot.
 */

#pragma once

#include "vk/transfer_recording_context.h"

#include "vk/vulkan_hpp_include.h"

// Fix #define MemoryBarrier coming from winnt.h
#undef MemoryBarrier

struct perFrameResources_t;

namespace gfx_api::vk
{

/// <summary>
/// Typed wrapper around the per-frame transfer (copy) command buffer.
///
/// Used by buffer/texture upload paths during the screen frame. `cmd()` begins recording and
/// sets `transferWorkRecorded` on the current ring slot; work is sealed and submitted in
/// `finishScreenFrame()` via `VkRoot::sealAndSubmitTransferGraphics`.
/// </summary>
class TransferRecorder
{
public:
	TransferRecorder(perFrameResources_t& frame, TransferRecordingContext context);

	bool hasWork() const;

	::vk::CommandBuffer cmd();

	void pipelineBarrier(
		::vk::PipelineStageFlags srcStageMask,
		::vk::PipelineStageFlags dstStageMask,
		::vk::DependencyFlags dependencyFlags,
		::vk::ArrayProxy<const ::vk::MemoryBarrier> memoryBarriers,
		::vk::ArrayProxy<const ::vk::BufferMemoryBarrier> bufferMemoryBarriers,
		::vk::ArrayProxy<const ::vk::ImageMemoryBarrier> imageMemoryBarriers);

	void copyBuffer(
		::vk::Buffer srcBuffer,
		::vk::Buffer dstBuffer,
		::vk::ArrayProxy<const ::vk::BufferCopy> regions);

	void copyBufferToImage(
		::vk::Buffer srcBuffer,
		::vk::Image dstImage,
		::vk::ImageLayout dstImageLayout,
		::vk::ArrayProxy<const ::vk::BufferImageCopy> regions);

private:
	void markWork();

	perFrameResources_t& _frame;
	TransferRecordingContext _context;
};

} // namespace gfx_api::vk
