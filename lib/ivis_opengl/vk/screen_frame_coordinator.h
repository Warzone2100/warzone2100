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
/** @file screen_frame_coordinator.h
 * Screen-frame finish path: commit inputs, present/acquire, ring advance, and frame epilogue.
 */

#pragma once

#include "swapchain_presentation_state.h"

struct ScreenFramePipelineState;
struct VkRoot;

namespace gfx_api::vk
{

/// <summary>
/// Vulkan screen-frame finish coordinator owned by `VkRoot`.
///
/// Owns `SwapchainPresentationState` and implements `finishScreenFrame()`:
/// * build commit inputs
/// * seal/submit via `VkRoot::sealAndSubmitTransferGraphics`
/// * present/acquire
/// * ring advance
/// * frame epilogue (`frameNum++`, close screen frame, purge FBO pool)
/// `prepareSwapchainForDrawing()` performs late swapchain acquire before render-graph
/// record (called from piemode).
/// </summary>
class ScreenFrameCoordinator
{
public:
	explicit ScreenFrameCoordinator(VkRoot& root);

	SwapchainPresentationState& presentation() { return _presentation; }
	const SwapchainPresentationState& presentation() const { return _presentation; }

	void markDrawableSizeDirty();
	void prepareSwapchainForDrawing();
	ScreenFramePipelineState buildCommitInputs();
	void finishFrame();

private:
	void tryAcquireSwapchainImageForDrawing();
	void logScreenFrameDrawSubmitSkip(const ScreenFramePipelineState& state) const;
	void handleSwapchainPostSubmit(ScreenFramePipelineState& state);
	void presentAndAcquireScreenFrame(ScreenFramePipelineState& state);
	void throttleSkippedDrawingFrame();
	void advanceRingBufferAfterSubmit(ScreenFramePipelineState& state);
	void completeScreenFrameFinishTail();

	VkRoot& _root;
	SwapchainPresentationState _presentation;
};

} // namespace gfx_api::vk
