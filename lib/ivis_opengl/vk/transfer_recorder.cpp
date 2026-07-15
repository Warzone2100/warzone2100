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
/** @file transfer_recorder.cpp
 * Typed wrapper for per-frame GPU upload / copy command recording.
 */

#include "transfer_recorder.h"

#include "lib/framework/wzapp.h"
#include "lib/ivis_opengl/gfx_api_vk.h"

namespace gfx_api::vk
{

TransferRecorder::TransferRecorder(perFrameResources_t& frame, TransferRecordingContext context)
	: _frame(frame)
	, _context(context)
{
}

bool TransferRecorder::hasWork() const
{
	return _frame.transferWorkRecorded;
}

void TransferRecorder::markWork()
{
#if defined(DEBUG)
	ASSERT(_context.screenFrameOpen, "Transfer recording outside screen frame");
#endif
	_frame.ensureTransferRecordingBegun(_context.vkDynLoader);
	_frame.transferWorkRecorded = true;
}

::vk::CommandBuffer TransferRecorder::cmd()
{
	markWork();
	return _frame.copyCmdBuffer();
}

void TransferRecorder::pipelineBarrier(
	::vk::PipelineStageFlags srcStageMask,
	::vk::PipelineStageFlags dstStageMask,
	::vk::DependencyFlags dependencyFlags,
	::vk::ArrayProxy<const ::vk::MemoryBarrier> memoryBarriers,
	::vk::ArrayProxy<const ::vk::BufferMemoryBarrier> bufferMemoryBarriers,
	::vk::ArrayProxy<const ::vk::ImageMemoryBarrier> imageMemoryBarriers)
{
	cmd().pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags,
		memoryBarriers, bufferMemoryBarriers, imageMemoryBarriers, _context.vkDynLoader);
}

void TransferRecorder::copyBuffer(
	::vk::Buffer srcBuffer,
	::vk::Buffer dstBuffer,
	::vk::ArrayProxy<const ::vk::BufferCopy> regions)
{
	cmd().copyBuffer(srcBuffer, dstBuffer, regions, _context.vkDynLoader);
}

void TransferRecorder::copyBufferToImage(
	::vk::Buffer srcBuffer,
	::vk::Image dstImage,
	::vk::ImageLayout dstImageLayout,
	::vk::ArrayProxy<const ::vk::BufferImageCopy> regions)
{
	cmd().copyBufferToImage(srcBuffer, dstImage, dstImageLayout, regions, _context.vkDynLoader);
}

} // namespace gfx_api::vk
