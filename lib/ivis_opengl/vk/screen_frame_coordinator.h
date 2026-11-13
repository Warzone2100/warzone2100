// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
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
