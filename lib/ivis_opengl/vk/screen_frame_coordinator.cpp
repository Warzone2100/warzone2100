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
 */

#include "screen_frame_coordinator.h"

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

void ScreenFrameCoordinator::tryAcquireSwapchainImageForDrawing()
{
	if (!_root.shouldDraw())
	{
		return;
	}

	if (_presentation.drawableSizeDirty())
	{
		// Drawable size not synced with swapchain; finishFrame() handles recreate.
		return;
	}

	auto& frameResources = buffering_mechanism::get_current_resources();
	if (frameResources.swapchainImageAcquired)
	{
		return;
	}

	try {
		_root.acquireNextSwapchainImage(true);
	}
	catch (const ::vk::SystemError& e)
	{
		auto resultErr = static_cast<::vk::Result>(e.code().value());
		const bool isRecoverableError =
			resultErr == ::vk::Result::eSuboptimalKHR ||
			resultErr == ::vk::Result::eErrorOutOfDateKHR ||
			resultErr == ::vk::Result::eErrorSurfaceLostKHR;
		debug(isRecoverableError ? LOG_3D : LOG_ERROR,
			"tryAcquireSwapchainImageForDrawing failed: %s", ::vk::to_string(resultErr).c_str());
		if (isRecoverableError)
		{
			_root.swapchainSize.width = 1;
			_root.swapchainSize.height = 1;
			markDrawableSizeDirty();
		}
		else
		{
			handleUnrecoverableError(resultErr);
		}
	}
}

void ScreenFrameCoordinator::prepareSwapchainForDrawing()
{
	tryAcquireSwapchainImageForDrawing();
}

ScreenFramePipelineState ScreenFrameCoordinator::buildCommitInputs()
{
	ScreenFramePipelineState state;
	state.mustSkipDrawing = !_root.shouldDraw();
	state.mustRecreateSwapchain = false;

	auto& frameResources = buffering_mechanism::get_current_resources();

	// Steady state: drawable unchanged and swapchain image already acquired for this slot.
	if (!_presentation.drawableSizeDirty() && frameResources.swapchainImageAcquired)
	{
		state.drawableWidth = _presentation.lastKnownDrawableWidth();
		state.drawableHeight = _presentation.lastKnownDrawableHeight();
		return state;
	}

	_root.backend_impl->getDrawableSize(&state.drawableWidth, &state.drawableHeight);
	if (_presentation.drawableRequiresSwapchainRecreate(state.drawableWidth, state.drawableHeight,
		_root.swapchainSize, _root.swapchain))
	{
		state.mustSkipDrawing = true;
		debug(LOG_3D, "[1] Drawable size (%d x %d) does not match swapchainSize (%d x %d) - must re-create swapchain",
			state.drawableWidth, state.drawableHeight, (int)_root.swapchainSize.width, (int)_root.swapchainSize.height);
		state.mustRecreateSwapchain = true;
	}
	else
	{
		_presentation.syncDrawableSize(state.drawableWidth, state.drawableHeight);
	}

	return state;
}

void ScreenFrameCoordinator::logScreenFrameDrawSubmitSkip(const ScreenFramePipelineState& state) const
{
	const auto& frameResources = buffering_mechanism::get_current_resources();
	if (!state.mustSkipDrawing && state.hadDrawCmdBufferRecording && !frameResources.swapchainImageAcquired)
	{
		debug(LOG_3D, "finishScreenFrame: skipping draw command buffer submit (swapchain image not acquired for current frame slot)");
	}
	else if (!state.mustSkipDrawing && !state.hadDrawCmdBufferRecording)
	{
		debug(LOG_ERROR, "finishScreenFrame: skipping draw command buffer submit (draw command buffer was not recording)");
	}
}

void ScreenFrameCoordinator::handleSwapchainPostSubmit(ScreenFramePipelineState& state)
{
	if (_root.queuedSwapModeChange.has_value())
	{
		_root.swapMode = _root.queuedSwapModeChange.value().newMode;
		state.mustRecreateSwapchain = true;
	}

	if (state.mustRecreateSwapchain)
	{
		_root.recreateSwapchainAfterPresentError(::vk::Result::eErrorOutOfDateKHR);
		state.submittedQueueWork = false;
		state.ringSwapped = true;
	}
	else if (state.shouldPresent)
	{
		presentAndAcquireScreenFrame(state);
	}
	else if (state.submittedQueueWork)
	{
		if (!state.mustSkipDrawing && !state.mustRecreateSwapchain)
		{
			advanceRingBufferAfterSubmit(state);
		}
		else
		{
			// Copy-only while skipping draw (resize): keep the ring slot, chain on its fence.
			state.ringSwapped = true;
		}
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
	else if (state.mustRecreateSwapchain)
	{
		_root.recreateSwapchainAfterPresentError(::vk::Result::eErrorOutOfDateKHR);
	}

	if (!state.submitDrawBuffer && state.mustSkipDrawing)
	{
		throttleSkippedDrawingFrame();
	}

	advanceRingBufferAfterSubmit(state);
	completeScreenFrameFinishTail();
}

void ScreenFrameCoordinator::presentAndAcquireScreenFrame(ScreenFramePipelineState& state)
{
	auto presentInfo = ::vk::PresentInfoKHR()
		.setPSwapchains(&_root.swapchain)
		.setSwapchainCount(1)
		.setPImageIndices(&_root.currentSwapchainIndex)
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&buffering_mechanism::get_swapchain_resources(_root.currentSwapchainIndex).renderFinishedSemaphore);

	::vk::Result presentResult;
	try {
		presentResult = _root.presentQueue.presentKHR(presentInfo, _root.vkDynLoader);
	}
	catch (const ::vk::OutOfDateKHRError&)
	{
		debug(LOG_3D, "::vk::Queue::presentKHR: ErrorOutOfDateKHR - must recreate swapchain");
		presentResult = ::vk::Result::eErrorOutOfDateKHR;
		state.mustRecreateSwapchain = true;
		markDrawableSizeDirty();
	}
	catch (const ::vk::SurfaceLostKHRError&)
	{
		debug(LOG_3D, "::vk::Queue::presentKHR: ErrorSurfaceLostKHR - must recreate surface + swapchain");
		try {
			_root.handleSurfaceLost();
		}
		catch (const ::vk::SystemError& e) {
			auto resultErr = static_cast<::vk::Result>(e.code().value());
			debug(LOG_ERROR, "handleSurfaceLost failed: %s: %s", ::vk::to_string(resultErr).c_str(), e.what());
			handleUnrecoverableError(resultErr);
		}
		state.shouldPresent = false;
		return;
	}
	catch (const ::vk::SystemError& e)
	{
		debug(LOG_FATAL, "::vk::Queue::presentKHR: unhandled error: %s", e.what());
		handleUnrecoverableError(static_cast<::vk::Result>(e.code().value()));
	}

	if (presentResult == ::vk::Result::eSuboptimalKHR)
	{
		debug(LOG_3D, "presentKHR returned eSuboptimalKHR (%d) - recreate swapchain", (int)presentResult);
		state.mustRecreateSwapchain = true;
		markDrawableSizeDirty();
	}

	if (state.mustRecreateSwapchain)
	{
		_root.recreateSwapchainAfterPresentError(presentResult);
		state.shouldPresent = false;
		return;
	}

	if (state.submittedQueueWork)
	{
		advanceRingBufferAfterSubmit(state);
	}

	try {
		if (_root.acquireNextSwapchainImage(true) != VkRoot::AcquireNextSwapchainImageResult::eSuccess)
		{
			return;
		}
	}
	catch (const ::vk::SystemError& e) {
		auto resultErr = static_cast<::vk::Result>(e.code().value());
		debug((resultErr == ::vk::Result::eSuboptimalKHR) ? LOG_3D : LOG_ERROR, "acquireNextSwapchainImage failed: %s", ::vk::to_string(resultErr).c_str());
		if (resultErr == ::vk::Result::eSuboptimalKHR)
		{
			_root.swapchainSize.width = 1;
			_root.swapchainSize.height = 1;
			markDrawableSizeDirty();
		}
		else
		{
			handleUnrecoverableError(resultErr);
		}
		return;
	}

	if (!_presentation.drawableSizeDirty())
	{
		return;
	}

	int w, h;
	_root.backend_impl->getDrawableSize(&w, &h);
	if (_presentation.drawableRequiresSwapchainRecreate(w, h, _root.swapchainSize, _root.swapchain))
	{
		debug(LOG_3D, "[3] Drawable size (%d x %d) does not match swapchainSize (%d x %d) - re-create swapchain", w, h, (int)_root.swapchainSize.width, (int)_root.swapchainSize.height);
		markDrawableSizeDirty();
		_root.recreateSwapchainAfterPresentError(::vk::Result::eErrorOutOfDateKHR);
	}
	else
	{
		_presentation.syncDrawableSize(w, h);
	}
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
		buffering_mechanism::swap(_root.dev, _root.vkDynLoader); // must be called *before* acquireNextSwapchainImage()
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
	buffering_mechanism::get_current_resources().transferWorkRecorded = false;
	_root.purgeFrameResources();
	_root._screenFrameOpen = false;
}

} // namespace gfx_api::vk
