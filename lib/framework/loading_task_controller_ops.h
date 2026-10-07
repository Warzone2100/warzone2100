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
/** @file loading_task_controller_ops.h
 * Non-template bridge so `loading_task.h` can drive `ResourceLoadingController`
 * without including the controller implementation.
 */

#pragma once

#include <coroutine>
#include <string>

struct LoadingTaskPromiseBase;

namespace loading_task_detail
{

/// <summary>
/// Invoked from `LoadingTaskPromise<T>::FinalAwaiter` when a stack frame's coroutine
/// completes. Notifies `ResourceLoadingController::onFrameFinished` when the promise is
/// bound to a controller (completing handle must be the execution-stack top).
/// </summary>
void notifyFrameFinished(LoadingTaskPromiseBase* promise,
                         std::coroutine_handle<> handle,
                         bool succeeded) noexcept;

/// <summary>
/// Invoked from `LoadingTask<T>::ChildTaskAwaiter` after the child handle is detached.
/// Pauses the parent frame, binds the child to the same controller if needed, applies the
/// child's `framePolicy` when set, and pushes the child onto the execution stack.
/// </summary>
void suspendAwaitChild(std::coroutine_handle<> parent,
                       std::coroutine_handle<> child,
                       LoadingTaskPromiseBase* childPromise);

/// <summary>
/// Pushes `name` onto the controller domain stack for `promise`.
/// Called from `setLoadingDomain`.
/// </summary>
void activateLoadingDomain(LoadingTaskPromiseBase* promise, std::string name);

/// <summary>
/// Pops `promise`'s domain segment, if it activated one, and drops it from the live-frame set.
/// Called from `~LoadingTaskPromiseBase`.
/// </summary>
void releaseLoadingDomain(LoadingTaskPromiseBase* promise) noexcept;

/// <summary>
/// Destroys a completed child frame that is no longer on the execution stack.
/// A frame the controller already destroyed, or one still running, is left alone.
/// </summary>
void destroyDetachedChildFrame(std::coroutine_handle<> handle,
                               LoadingTaskPromiseBase* promise) noexcept;

} // namespace loading_task_detail
