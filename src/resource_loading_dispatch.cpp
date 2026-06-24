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
/** \file resource_loading_dispatch.cpp
 * \brief Game-level loading submission implementation.
 */

#include "resource_loading_dispatch.h"

#include "lib/framework/loading_task.h"
#include "lib/framework/wzapp.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/sound/audio.h"
#include "wrappers.h"

#include <chrono>

namespace
{
constexpr auto kMinPresentInterval = std::chrono::milliseconds(50);
std::chrono::steady_clock::time_point lastPresentTime{};
} // namespace

void submitResourceLoadingTask(ResourceLoadingTaskFactory taskFactory,
                              bool showLoadingScreen,
                              bool drawBackdrop,
                              ResourceLoadingController::FrameProcessingMode frameMode)
{
	ASSERT(taskFactory, "submitResourceLoadingJob given null task factory");
	ResourceLoadingController& controller = ResourceLoadingController::instance();
	if (!controller.active() && showLoadingScreen && !isLoadingScreenActive())
	{
		initLoadingScreen(drawBackdrop);
	}
	ResourceLoadingController::FramePolicy policy;
	policy.showLoadingScreen = showLoadingScreen;
	policy.frameMode = frameMode;
	controller.request(taskFactory(controller), policy);
}

void presentResourceLoadingScreenIfNeeded()
{
	if (ResourceLoadingController::instance().loadingScreenHandledByController())
	{
		presentLoadingScreenForCurrentFrame();
	}
}

void pumpResourceLoadingHousekeeping(const ResourceLoadingController::FramePolicy& policy)
{
	wzPumpEventsWhileLoading();
	audio_Update();

	if (!policy.showLoadingScreen && !isLoadingScreenActive())
	{
		return;
	}

	auto const now = std::chrono::steady_clock::now();
	if (now - lastPresentTime < kMinPresentInterval)
	{
		return;
	}
	lastPresentTime = now;

	presentLoadingScreenForCurrentFrame();
	pie_ScreenFrameRenderEnd();
	pie_ScreenFrameRenderBegin();
}

bool tickResourceLoadingFrame()
{
	ResourceLoadingController& loadingController = ResourceLoadingController::instance();
	if (!loadingController.active())
	{
		return false;
	}
	const auto loadingFrameMode = loadingController.currentFrameProcessingMode();
	loadingController.step();
	presentResourceLoadingScreenIfNeeded();
	return loadingFrameMode == ResourceLoadingController::FrameProcessingMode::ConsumeFrame;
}
