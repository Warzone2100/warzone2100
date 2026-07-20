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
/** @file screen_frame_coordinator.cpp
 * Screen-frame finish path: commit inputs, present/acquire, ring advance, and frame epilogue.
 *
 * Screen-frame swapchain invariants (Vulkan):
 *   Phase              | recreate          | acquire | present
 *   Begin reconcile    | yes               | no      | no
 *   End acquire        | no*               | yes     | no
 *   finish present     | no*               | no      | yes
 *   * macOS suboptimal acquire/present only, via WsiPlatformPolicy.
 */

#include "screen_frame_coordinator.h"

#include "wsi_platform_policy.h"

#include "lib/framework/wzapp.h"
#include "lib/ivis_opengl/gfx_api_vk.h"

#include <chrono>
#include <thread>

namespace gfx_api::vk
{

ScreenFrameCoordinator::ScreenFrameCoordinator(VkRoot& root)
	: _root(root)
{
}

void ScreenFrameCoordinator::markDrawableSizeDirty()
{
	_presentation.markDrawableSizeDirty();
	_root.markScreenGeometryDirty();
}

void ScreenFrameCoordinator::requestSwapchainRecreate()
{
	_presentation.requestSwapchainRecreate();
	_root.markScreenGeometryDirty();
}

void ScreenFrameCoordinator::requestSurfaceLostRecovery()
{
	_presentation.requestSurfaceLostRecovery();
	_root.markScreenGeometryDirty();
}

void ScreenFrameCoordinator::reconcileSwapchainAtFrameOpen()
{
	_frameGate.reconcileOk = false;

	if (!buffering_mechanism::isInitialized() || !_root.backend_impl)
	{
		return;
	}

	ASSERT(!_root._screenFrameOpen, "reconcileSwapchainAtFrameOpen while screen frame is open");

	if (_presentation.surfaceLostPending())
	{
		try {
			_root.handleSurfaceLost();
			_presentation.clearSurfaceLostPending();
			_presentation.clearSwapchainRecreatePending();
		}
		catch (const ::vk::SystemError& e)
		{
			auto resultErr = static_cast<::vk::Result>(e.code().value());
			debug(LOG_ERROR, "reconcileSwapchainAtFrameOpen: handleSurfaceLost failed: %s: %s",
				::vk::to_string(resultErr).c_str(), e.what());
			return;
		}
	}

	int drawableWidth = 0;
	int drawableHeight = 0;
	_root.backend_impl->getDrawableSize(&drawableWidth, &drawableHeight);

	const bool drawableMismatch = _presentation.drawableRequiresSwapchainRecreate(drawableWidth, drawableHeight,
		_root.swapchainSize, _root.swapchain);
	const bool forceRecreate = _presentation.swapchainRecreatePending();

	if (!drawableMismatch && !forceRecreate)
	{
		if (_presentation.drawableSizeDirty())
		{
			_presentation.syncDrawableSize(drawableWidth, drawableHeight);
		}
		_frameGate.reconcileOk = _root.shouldDraw();
		return;
	}

	if (forceRecreate && !drawableMismatch)
	{
		debug(LOG_3D, "reconcileSwapchainAtFrameOpen: recreating swapchain (pending, drawable %d x %d, swapchain %d x %d)",
			drawableWidth, drawableHeight, (int)_root.swapchainSize.width, (int)_root.swapchainSize.height);
	}
	else
	{
		debug(LOG_3D, "reconcileSwapchainAtFrameOpen: recreating swapchain (drawable %d x %d, swapchain %d x %d)",
			drawableWidth, drawableHeight, (int)_root.swapchainSize.width, (int)_root.swapchainSize.height);
	}

	if (!_root.recreateSwapchain(::vk::Result::eErrorOutOfDateKHR))
	{
		return;
	}

	_presentation.syncDrawableSize(drawableWidth, drawableHeight);
	_presentation.clearSwapchainRecreatePending();
	// createSwapchain never acquires: End acquireSwapchainForFrameDraw() owns the image.
	_frameGate.reconcileOk = _root.shouldDraw();
}

void ScreenFrameCoordinator::acquireSwapchainForFrameDraw()
{
	if (!buffering_mechanism::isInitialized() || !_root.backend_impl)
	{
		return;
	}

	if (!_frameGate.reconcileOk || !_root.shouldDraw())
	{
		return;
	}

	if (_presentation.drawableSizeDirty())
	{
		debug(LOG_3D, "acquireSwapchainForFrameDraw: drawable dirty mid-frame, skipping draw");
		return;
	}

	auto& frameResources = buffering_mechanism::get_current_resources();
	if (frameResources.swapchainImageAcquired)
	{
		return;
	}

	const auto deferWithPlaceholderExtents = [this](bool surfaceLost) {
		if (surfaceLost)
		{
			requestSurfaceLostRecovery();
		}
		else
		{
			requestSwapchainRecreate();
		}
		_root.swapchainSize.width = 1;
		_root.swapchainSize.height = 1;
	};

	try {
		auto status = _root.tryAcquireSwapchainImage();
		switch (status)
		{
		case VkRoot::SwapchainAcquireStatus::Success:
			return;

		case VkRoot::SwapchainAcquireStatus::OutOfDate:
			deferWithPlaceholderExtents(false);
			return;

		case VkRoot::SwapchainAcquireStatus::SurfaceLost:
			deferWithPlaceholderExtents(true);
			return;

		case VkRoot::SwapchainAcquireStatus::Suboptimal:
			requestSwapchainRecreate();
			switch (WsiPlatformPolicy::suboptimalAcquireAction())
			{
			case SuboptimalAcquireAction::DeferToNextBegin:
				return;

			case SuboptimalAcquireAction::RecreateAndRetryOnce:
				debug(LOG_INFO, "acquireSwapchainForFrameDraw: eSuboptimalKHR - immediately recreate");
				if (!_root.recreateSwapchain(::vk::Result::eSuboptimalKHR))
				{
					return;
				}
				_root.rebindOpenScreenFrameResources();
				status = _root.tryAcquireSwapchainImage();
				if (status == VkRoot::SwapchainAcquireStatus::Success)
				{
					return;
				}
				if (status == VkRoot::SwapchainAcquireStatus::SurfaceLost)
				{
					requestSurfaceLostRecovery();
				}
				else
				{
					requestSwapchainRecreate();
				}
				if (status == VkRoot::SwapchainAcquireStatus::OutOfDate
					|| status == VkRoot::SwapchainAcquireStatus::SurfaceLost)
				{
					_root.swapchainSize.width = 1;
					_root.swapchainSize.height = 1;
				}
				return;
			}
			return;
		}
	}
	catch (const ::vk::SystemError& e)
	{
		auto resultErr = static_cast<::vk::Result>(e.code().value());
		debug(LOG_ERROR, "acquireSwapchainForFrameDraw failed: %s", ::vk::to_string(resultErr).c_str());
		handleUnrecoverableError(resultErr);
	}
}

bool ScreenFrameCoordinator::canRecordSwapchainDraws() const
{
	return _frameGate.reconcileOk
		&& buffering_mechanism::isInitialized()
		&& buffering_mechanism::get_current_resources().swapchainImageAcquired;
}

ScreenFramePipelineState ScreenFrameCoordinator::buildCommitInputs()
{
	ScreenFramePipelineState state;
	state.swapchainRecreatePending = false;
	state.acquiredSwapchainImage = buffering_mechanism::isInitialized()
		&& buffering_mechanism::get_current_resources().swapchainImageAcquired;
	state.mustSkipDrawing = !_root.shouldDraw() || !state.acquiredSwapchainImage;

	if (!buffering_mechanism::isInitialized() || !_root.backend_impl)
	{
		state.mustSkipDrawing = true;
		return state;
	}

	_root.backend_impl->getDrawableSize(&state.drawableWidth, &state.drawableHeight);
	if (_presentation.drawableRequiresSwapchainRecreate(state.drawableWidth, state.drawableHeight,
		_root.swapchainSize, _root.swapchain))
	{
		state.mustSkipDrawing = true;
		debug(LOG_3D, "[1] Drawable size (%d x %d) does not match swapchainSize (%d x %d) - defer recreate to next Begin",
			state.drawableWidth, state.drawableHeight, (int)_root.swapchainSize.width, (int)_root.swapchainSize.height);
		state.swapchainRecreatePending = true;
		requestSwapchainRecreate();
	}
	else if (_presentation.drawableSizeDirty())
	{
		_presentation.syncDrawableSize(state.drawableWidth, state.drawableHeight);
	}

	return state;
}

bool ScreenFrameCoordinator::shouldAdvanceRingAfterSubmit(const ScreenFramePipelineState& state)
{
	if (!state.submittedQueueWork || state.ringSwapped)
	{
		return false;
	}

	// Draw was recorded but not submitted - hold the ring slot (loading/resync path).
	if (state.hadDrawCmdBufferRecording && !state.submitDrawBuffer)
	{
		return false;
	}

	// Successful present path advances the ring in presentAndAdvanceRing().
	if (state.submitDrawBuffer)
	{
		return false;
	}

	// Copy-only upload with no draw recording may advance to chain on the slot fence.
	if (state.hasCopyWork && !state.hadDrawCmdBufferRecording)
	{
		return !state.mustSkipDrawing && !state.swapchainRecreatePending;
	}

	return false;
}

void ScreenFrameCoordinator::logScreenFrameDrawSubmitSkip(const ScreenFramePipelineState& state) const
{
	// submitCommandBuffers clears swapchainImageAcquired after a successful draw submit -
	// only diagnose actual skips; use the pre-submit snapshot for acquire state.
	if (state.submitDrawBuffer || state.mustSkipDrawing)
	{
		return;
	}

	if (state.hadDrawCmdBufferRecording && !state.acquiredSwapchainImage)
	{
		debug(LOG_ERROR, "finishScreenFrame: skipping draw command buffer submit (swapchain image not acquired for current frame slot)");
	}
	else if (!state.hadDrawCmdBufferRecording)
	{
		debug(LOG_ERROR, "finishScreenFrame: skipping draw command buffer submit (draw command buffer was not recording)");
	}
}

void ScreenFrameCoordinator::handleSwapchainPostSubmit(ScreenFramePipelineState& state)
{
	if (_root.queuedSwapModeChange.has_value())
	{
		_root.swapMode = _root.queuedSwapModeChange.value().newMode;
		requestSwapchainRecreate();
		state.swapchainRecreatePending = true;
		// Keep queuedSwapModeChange until Begin recreateSwapchain runs the completion handler.
		// Present this frame if draw was already submitted - do not destroy the swapchain here.
	}

	if (state.shouldPresent)
	{
		presentAndAdvanceRing(state);
		return;
	}

	if (state.swapchainRecreatePending)
	{
		// Draw was skipped; recovery is next Begin. Hold ring if copy submitted.
		if (state.submittedQueueWork)
		{
			state.ringSwapped = true;
		}
		return;
	}

	if (shouldAdvanceRingAfterSubmit(state))
	{
		advanceRingBufferAfterSubmit(state);
	}
	else if (state.submittedQueueWork)
	{
		// Copy-only while skipping draw (resize): keep the ring slot, chain on its fence.
		state.ringSwapped = true;
	}
}

void ScreenFrameCoordinator::finishFrame()
{
	if (!_root._screenFrameOpen)
	{
		return;
	}

	_root.currentPSO = nullptr;

	ScreenFramePipelineState state = buildCommitInputs();
	_root.sealAndSubmitTransferGraphics(state);
	logScreenFrameDrawSubmitSkip(state);

	const bool needsQueueSubmit = state.submittedQueueWork;
	if (needsQueueSubmit)
	{
		handleSwapchainPostSubmit(state);
	}
	else if (state.swapchainRecreatePending)
	{
		// Already requested via buildCommitInputs / presentation flags - next Begin reconciles.
		state.ringSwapped = true;
	}

	if (!state.submitDrawBuffer && state.mustSkipDrawing)
	{
		throttleSkippedDrawingFrame();
	}

	if (shouldAdvanceRingAfterSubmit(state))
	{
#if defined(DEBUG)
		ASSERT(!(state.submitDrawBuffer && state.ringSwapped),
			"double ring buffer advance (present path already advanced)");
#endif
		advanceRingBufferAfterSubmit(state);
	}

#if defined(DEBUG)
	// Acquire implies present this frame unless WSI recovery was requested or drawing is disabled.
	ASSERT(!state.acquiredSwapchainImage
		|| state.submitDrawBuffer
		|| state.swapchainRecreatePending
		|| !_root.shouldDraw(),
		"swapchain acquired for draw but not submitted for present");
#endif

	completeScreenFrameFinishTail();
}

void ScreenFrameCoordinator::deferSwapchainRecreate(ScreenFramePipelineState& state)
{
	requestSwapchainRecreate();
	state.swapchainRecreatePending = true;
}

void ScreenFrameCoordinator::advanceRingIfSubmittedDraw(ScreenFramePipelineState& state)
{
	if (state.submittedQueueWork && state.submitDrawBuffer)
	{
		advanceRingBufferAfterSubmit(state);
	}
}

void ScreenFrameCoordinator::presentAndAdvanceRing(ScreenFramePipelineState& state)
{
	auto presentInfo = ::vk::PresentInfoKHR()
		.setPSwapchains(&_root.swapchain)
		.setSwapchainCount(1)
		.setPImageIndices(&_root.currentSwapchainIndex)
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&buffering_mechanism::get_swapchain_resources(_root.currentSwapchainIndex).renderFinishedSemaphore);

	::vk::Result presentResult = ::vk::Result::eSuccess;
	try {
		presentResult = _root.presentQueue.presentKHR(presentInfo, _root.vkDynLoader);
	}
	catch (const ::vk::OutOfDateKHRError&)
	{
		debug(LOG_3D, "::vk::Queue::presentKHR: ErrorOutOfDateKHR - defer recreate to next frame Begin");
		presentResult = ::vk::Result::eErrorOutOfDateKHR;
	}
	catch (const ::vk::SurfaceLostKHRError&)
	{
		debug(LOG_3D, "::vk::Queue::presentKHR: ErrorSurfaceLostKHR - defer surface recovery to next frame Begin");
		requestSurfaceLostRecovery();
		state.swapchainRecreatePending = true;
		state.shouldPresent = false;
		// Submit already signaled renderFinished; present attempted the wait - advance ring.
		advanceRingIfSubmittedDraw(state);
		return;
	}
	catch (const ::vk::SystemError& e)
	{
		debug(LOG_FATAL, "::vk::Queue::presentKHR: unhandled error: %s", e.what());
		handleUnrecoverableError(static_cast<::vk::Result>(e.code().value()));
	}

	if (presentResult == ::vk::Result::eErrorOutOfDateKHR)
	{
		deferSwapchainRecreate(state);
		advanceRingIfSubmittedDraw(state);
		return;
	}

	if (presentResult == ::vk::Result::eSuboptimalKHR)
	{
		requestSwapchainRecreate();
		switch (WsiPlatformPolicy::suboptimalPresentAction())
		{
		case SuboptimalPresentAction::RecreateInline:
			debug(LOG_3D, "presentKHR returned eSuboptimalKHR (%d) - recreate swapchain", (int)presentResult);
			_root.recreateSwapchain(presentResult);
			state.shouldPresent = false;
			state.submittedQueueWork = false;
			state.ringSwapped = true;
			return;
		case SuboptimalPresentAction::DeferToNextBegin:
			debug(LOG_3D, "presentKHR returned eSuboptimalKHR (%d) - defer swapchain recreate to next frame Begin", (int)presentResult);
			state.swapchainRecreatePending = true;
			advanceRingIfSubmittedDraw(state);
			return;
		}
	}

	// Success
	advanceRingIfSubmittedDraw(state);
}

void ScreenFrameCoordinator::throttleSkippedDrawingFrame()
{
	// Skipped drawing without present/acquire - cap CPU spin to ~120 FPS.
	constexpr uint32_t minFrameInterval = 1000 / 120;

	uint32_t renderPassEndTime = wzGetTicks();
	const uint32_t frameTime = renderPassEndTime - _root.lastRenderPassEndTime;
	if (frameTime < minFrameInterval)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(minFrameInterval - frameTime));
		renderPassEndTime = wzGetTicks();
	}
	_root.lastRenderPassEndTime = renderPassEndTime;
}

void ScreenFrameCoordinator::advanceRingBufferAfterSubmit(ScreenFramePipelineState& state)
{
	if (!state.submittedQueueWork || state.ringSwapped)
	{
		return;
	}

	try {
		buffering_mechanism::swap(_root.dev, _root.vkDynLoader); // must be called *before* End tryAcquireSwapchainImage()
		state.ringSwapped = true;
	}
	catch (const ::vk::OutOfHostMemoryError& e)
	{
		debug(LOG_ERROR, "buffering swap: OutOfHostMemoryError: %s", e.what());
		handleUnrecoverableError(::vk::Result::eErrorOutOfHostMemory);
	}
	catch (const ::vk::OutOfDeviceMemoryError& e)
	{
		debug(LOG_ERROR, "buffering swap: OutOfDeviceMemoryError: %s", e.what());
		handleUnrecoverableError(::vk::Result::eErrorOutOfDeviceMemory);
	}
	catch (const ::vk::DeviceLostError& e)
	{
		debug(LOG_ERROR, "buffering swap: DeviceLostError: %s", e.what());
		handleUnrecoverableError(::vk::Result::eErrorDeviceLost);
	}
	catch (const ::vk::SystemError& e)
	{
		debug(LOG_FATAL, "buffering swap: unhandled error: %s", e.what());
		auto resultErr = static_cast<::vk::Result>(e.code().value());
		handleUnrecoverableError(resultErr);
	}
}

void ScreenFrameCoordinator::completeScreenFrameFinishTail()
{
	_root.frameNum = std::max<size_t>(_root.frameNum + 1, 1);
	_root.purgeFrameResources();
	_root._screenFrameOpen = false;
}

} // namespace gfx_api::vk
