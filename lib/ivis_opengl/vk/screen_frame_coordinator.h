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
 * Vulkan screen-frame coordinator: Begin reconcile, End acquire, finish submit/present.
 */

#pragma once

#include "swapchain_presentation_state.h"

struct ScreenFramePipelineState;
struct VkRoot;

namespace gfx_api::vk
{

/// <summary>
/// Vulkan screen-frame coordinator owned by `VkRoot`.
///
/// Lifecycle:
/// * `reconcileSwapchainAtFrameOpen()` at Begin (recreate / surface-lost only; no acquire)
/// * `acquireSwapchainForFrameDraw()` at End before graph (acquire; recreate only via
///   WsiPlatformPolicy macOS suboptimal)
/// * `finishFrame()`: seal/submit/present/advance; WSI recovery deferred to next Begin
///   (macOS present suboptimal via WsiPlatformPolicy is the sole mid-finish recreate exception)
/// </summary>
class ScreenFrameCoordinator
{
public:
	explicit ScreenFrameCoordinator(VkRoot& root);

	SwapchainPresentationState& presentation() { return _presentation; }
	const SwapchainPresentationState& presentation() const { return _presentation; }

	void markDrawableSizeDirty();
	void requestSwapchainRecreate();
	void requestSurfaceLostRecovery();
	void reconcileSwapchainAtFrameOpen();
	bool reconcileSucceededAtFrameOpen() const { return _frameGate.reconcileOk; }
	void acquireSwapchainForFrameDraw();
	bool canRecordSwapchainDraws() const;
	ScreenFramePipelineState buildCommitInputs();
	void finishFrame();

private:
	struct FrameSwapchainGate
	{
		bool reconcileOk = false;
	};

	static bool shouldAdvanceRingAfterSubmit(const ScreenFramePipelineState& state);

	void logScreenFrameDrawSubmitSkip(const ScreenFramePipelineState& state) const;
	void handleSwapchainPostSubmit(ScreenFramePipelineState& state);
	void presentAndAdvanceRing(ScreenFramePipelineState& state);
	void deferSwapchainRecreate(ScreenFramePipelineState& state);
	void advanceRingIfSubmittedDraw(ScreenFramePipelineState& state);
	void throttleSkippedDrawingFrame();
	void advanceRingBufferAfterSubmit(ScreenFramePipelineState& state);
	void completeScreenFrameFinishTail();

	VkRoot& _root;
	SwapchainPresentationState _presentation;
	FrameSwapchainGate _frameGate;
};

} // namespace gfx_api::vk
