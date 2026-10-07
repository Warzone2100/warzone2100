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
/** @file loading_task_controller_ops.cpp
 * Implementation of the `LoadingTask` <-> `ResourceLoadingController` bridge.
 */

#include "loading_task_controller_ops.h"

#include "loading_task.h"
#include "resource_loading_controller.h"

#include "lib/framework/wzapp.h"

namespace loading_task_detail
{

void notifyFrameFinished(LoadingTaskPromiseBase* promise,
                         std::coroutine_handle<> handle,
                         bool succeeded) noexcept
{
	if (promise->controller != nullptr)
	{
		ASSERT(promise->controller->topFrame().handle.address() == handle.address(),
		       "completing frame must be execution stack top");
		promise->controller->onFrameFinished(succeeded);
	}
}

void suspendAwaitChild(std::coroutine_handle<> parent,
                       std::coroutine_handle<> child,
                       LoadingTaskPromiseBase* childPromise)
{
	ASSERT(childPromise != nullptr, "suspendAwaitChild given null childPromise");

	LoadingTaskPromiseBase* parent_promise = ResourceLoadingController::instance().topFrame().promiseBase;
	ASSERT(parent_promise != nullptr, "co_await child from a coroutine with no promise on the execution stack");
	ResourceLoadingController* controller = parent_promise->controller;
	ASSERT(controller != nullptr, "co_await LoadingTask from a coroutine that is not bound to a ResourceLoadingController");
	ResourceLoadingController::ExecutionFrame& parent_frame = controller->topFrame();
	ASSERT(parent_frame.promiseBase == parent_promise,
	       "co_await child must suspend the execution stack top");
	ASSERT(parent_frame.handle.address() == parent.address(),
	       "co_await child must suspend the execution stack top");
	parent_frame.state = ExecutionFrameState::Paused;

	if (childPromise->controller == nullptr)
	{
		childPromise->controller = controller;
	}

	ResourceLoadingController::FramePolicy child_policy = parent_frame.policy;
	if (childPromise->framePolicy.has_value())
	{
		child_policy = childPromise->framePolicy.value();
	}

	controller->pushFrame(child, child_policy, childPromise);
}

void activateLoadingDomain(LoadingTaskPromiseBase* promise, std::string name)
{
	ASSERT(promise != nullptr, "setLoadingDomain with null promise");
	ASSERT(promise->controller != nullptr,
	       "setLoadingDomain outside a ResourceLoadingController task");
	ASSERT(!promise->loadingDomainActive, "setLoadingDomain called twice in one LoadingTask");
	ASSERT(!name.empty(), "setLoadingDomain with empty name");

	promise->loadingDomainName = name;
	promise->controller->pushLoadingDomain(promise, std::move(name));
	promise->loadingDomainActive = true;
}

void releaseLoadingDomain(LoadingTaskPromiseBase* promise) noexcept
{
	if (promise == nullptr || promise->controller == nullptr)
	{
		return;
	}
	ResourceLoadingController* controller = promise->controller;
	if (promise->loadingDomainActive)
	{
		controller->popLoadingDomain(promise);
		promise->loadingDomainActive = false;
	}
	controller->noteFrameDead(promise);
}

void destroyDetachedChildFrame(std::coroutine_handle<> handle,
                               LoadingTaskPromiseBase* promise) noexcept
{
	if (!handle || promise == nullptr)
	{
		return;
	}
	ResourceLoadingController& controller = ResourceLoadingController::instance();
	// Live-set lookup only. `handle.done()` would be dangling if the frame is already destroyed.
	if (!controller.isLiveFrame(promise))
	{
		return;
	}
	// Still on the execution stack, so `popAndDestroyTop` owns this frame.
	if (controller.isOnExecutionStack(promise))
	{
		return;
	}
	handle.destroy();
}

} // namespace loading_task_detail

LoadingTaskPromiseBase::~LoadingTaskPromiseBase()
{
	loading_task_detail::releaseLoadingDomain(this);
}

void LoadingTaskPromiseBase::activateLoadingDomain(std::string name)
{
	loading_task_detail::activateLoadingDomain(this, std::move(name));
}
