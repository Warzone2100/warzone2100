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
/** \file resource_loading_dispatch.h
 * \brief Game-level loading submission and loading-screen presentation.
 */

#pragma once

#include "lib/framework/resource_loading_controller.h"
#include "wrappers.h"

#include <functional>

using ResourceLoadingTaskFactory = std::function<LoadingTask<>(ResourceLoadingController &)>;

void submitResourceLoadingTask(ResourceLoadingTaskFactory taskFactory,
                              bool showLoadingScreen = true,
                              bool drawBackdrop = true,
                              ResourceLoadingController::FrameProcessingMode frameMode =
                                  ResourceLoadingController::FrameProcessingMode::ConsumeFrame);

void presentResourceLoadingScreenIfNeeded();

/// Per-quantum housekeeping during blocking loads (events, audio, optional loading UI).
void pumpResourceLoadingHousekeeping(const ResourceLoadingController::FramePolicy& policy);

/// Advance async loading for one mainLoop iteration. Returns true if the frame was consumed.
bool tickResourceLoadingFrame();

enum class CloseLoadingScreenOnComplete
{
	Yes,
	No,
};

template <typename T>
LoadResult<T> runBlockingResourceLoad(
    LoadingTask<T> task,
    ResourceLoadingController::FramePolicy policy,
    CloseLoadingScreenOnComplete closeOnComplete = CloseLoadingScreenOnComplete::Yes)
{
	bool openedScreen = false;
	if (policy.showLoadingScreen && !isLoadingScreenActive())
	{
		initLoadingScreen(true);
		openedScreen = true;
	}

	ResourceLoadingController& controller = ResourceLoadingController::instance();
	LoadResult<T> result = controller.runTaskToCompletion(
	    std::move(task), policy, pumpResourceLoadingHousekeeping);

	if (openedScreen && closeOnComplete == CloseLoadingScreenOnComplete::Yes)
	{
		closeLoadingScreen();
	}
	return result;
}
