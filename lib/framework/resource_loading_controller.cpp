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
/** @file resource_loading_controller.cpp
 * Implementation of `ResourceLoadingController`.
 */

#include "resource_loading_controller.h"

#include <chrono>
#include <ratio>
#include <utility>

struct ResourceLoadingController::ResourceLoadingSubmission
{
	LoadingTaskHandle task;
	FramePolicy policy{};
};

ResourceLoadingController::~ResourceLoadingController()
{
	resetTaskState();
}

ResourceLoadingController& ResourceLoadingController::instance()
{
	static ResourceLoadingController instance;
	return instance;
}

ResourceLoadingController::FrameYield ResourceLoadingController::yieldFrame() noexcept
{
	return FrameYield{this};
}

void ResourceLoadingController::FrameYield::await_suspend(std::coroutine_handle<> h) const noexcept
{
	ASSERT(controller != nullptr, "yieldFrame without controller");
	auto& top = controller->topFrame();
	ASSERT(top.handle.address() == h.address(),
	       "yieldFrame must suspend the execution stack top");
	top.state = ExecutionFrameState::Paused;
}

ResourceLoadingController::ExecutionFrame& ResourceLoadingController::topFrame()
{
	ASSERT(hasActiveExecution(), "topFrame without active execution");
	return executionStack.top();
}

const ResourceLoadingController::ExecutionFrame& ResourceLoadingController::topFrame() const
{
	ASSERT(hasActiveExecution(), "topFrame without active execution");
	return executionStack.top();
}

void ResourceLoadingController::pushFrame(std::coroutine_handle<> handle, FramePolicy policy,
                                          LoadingTaskPromiseBase* promiseBase)
{
	ASSERT(handle, "pushFrame with null handle");
	ASSERT(!handle.done(), "pushFrame with already-completed coroutine");
	ASSERT(!sessionFinished, "pushFrame after load session finished");
	ASSERT(promiseBase != nullptr, "pushFrame with null promiseBase");
	executionStack.push(ExecutionFrame{handle, ExecutionFrameState::Paused, policy, promiseBase});
}

void ResourceLoadingController::popAndDestroyTop() noexcept
{
	if (executionStack.empty())
	{
		return;
	}
	const std::coroutine_handle<> handle = executionStack.top().handle;
	executionStack.pop();
	if (handle)
	{
		handle.destroy();
	}
}

void ResourceLoadingController::onFrameFinished(bool succeeded) noexcept
{
	ASSERT(!sessionFinished, "onFrameFinished after load session finished");
	ASSERT(hasActiveExecution(), "onFrameFinished without active execution");
	const std::coroutine_handle<> finished = executionStack.top().handle;
	ASSERT(finished.done(), "onFrameFinished for a coroutine that is not done");
	executionStack.pop();
	if (executionStack.empty())
	{
		sessionRootHandle = finished;
		terminalSucceeded = succeeded;
		sessionFinished = true;
		ASSERT(!hasActiveExecution(), "sessionFinished requires empty execution stack");
	}
	// Nested completion: keep `finished` alive until the parent's ChildTaskAwaiter::await_resume.
}

void ResourceLoadingController::start(LoadingTaskHandle task, FramePolicy policy)
{
	ASSERT(!hasActiveExecution(), "ResourceLoadingController.start called while execution is active");
	auto session = task.release();
	ASSERT(session.handle, "ResourceLoadingController.start given an empty task");
	ASSERT(session.promiseBase != nullptr, "ResourceLoadingController.start given null promiseBase");
	session.promiseBase->controller = this;
	sessionFinished = false;
	sessionRootHandle = {};
	terminalSucceeded = true;
	pushFrame(session.handle, policy, session.promiseBase);
	ASSERT(executionStack.size() == 1, "start must leave a single root execution frame");
}

LoadStepStatus ResourceLoadingController::stepOneQuantum()
{
	if (sessionFinished)
	{
		ASSERT(!hasActiveExecution(), "sessionFinished requires empty execution stack");
		return terminalSucceeded ? LoadStepStatus::Completed : LoadStepStatus::Failed;
	}

	ASSERT(hasActiveExecution(), "ResourceLoadingController.stepOneQuantum without active execution");

	ExecutionFrame& top = topFrame();
	ASSERT(top.state == ExecutionFrameState::Paused,
	       "stepOneQuantum must resume a paused execution frame");
	top.state = ExecutionFrameState::Running;
	top.handle.resume();

	if (sessionFinished)
	{
		ASSERT(!hasActiveExecution(), "sessionFinished requires empty execution stack");
		return terminalSucceeded ? LoadStepStatus::Completed : LoadStepStatus::Failed;
	}

	ASSERT(hasActiveExecution() && !sessionFinished,
	       "InProgress requires active execution and an unfinished session");
	return LoadStepStatus::InProgress;
}

void ResourceLoadingController::completeActiveSubmission(LoadStepStatus result)
{
	if (result == LoadStepStatus::InProgress)
	{
		return;
	}

	activeSubmission.reset();
	resetTaskState();
	startNextPendingSubmission();
}

void ResourceLoadingController::resetTaskState() noexcept
{
	while (!executionStack.empty())
	{
		popAndDestroyTop();
	}
	if (sessionRootHandle)
	{
		sessionRootHandle.destroy();
		sessionRootHandle = {};
	}
	sessionFinished = false;
	terminalSucceeded = true;
	ASSERT(!hasActiveExecution() && !sessionFinished, "resetTaskState must clear execution state");
}

void ResourceLoadingController::requestImpl(LoadingTaskHandle task, FramePolicy policy)
{
	ASSERT(!task.empty(), "request given empty LoadingTaskHandle");
	auto submission = std::make_unique<ResourceLoadingSubmission>();
	submission->task = std::move(task);
	submission->policy = policy;

	if (activeSubmission)
	{
		pendingSubmissions.push(std::move(submission));
		return;
	}

	begin(std::move(submission));
}

void ResourceLoadingController::begin(std::unique_ptr<ResourceLoadingSubmission> submission)
{
	ASSERT(!activeSubmission, "begin called while another submission is active");
	ASSERT(submission && !submission->task.empty(), "begin given empty LoadingTask");
	const FramePolicy policy = submission->policy;
	activeSubmission = std::move(submission);
	resetTaskState();
	start(std::move(activeSubmission->task), policy);
}

void ResourceLoadingController::startNextPendingSubmission()
{
	if (pendingSubmissions.empty())
	{
		return;
	}
	std::unique_ptr<ResourceLoadingSubmission> nextSubmission = std::move(pendingSubmissions.front());
	pendingSubmissions.pop();
	begin(std::move(nextSubmission));
}

bool ResourceLoadingController::active() const
{
	return activeSubmission != nullptr;
}

void ResourceLoadingController::step()
{
	using MinimumStepDuration = std::chrono::duration<int, std::ratio<1, 60>>;

	ASSERT(activeSubmission, "step called without an active submission");
	const auto minDeadline = std::chrono::steady_clock::now() + MinimumStepDuration(1);
	do
	{
		completeActiveSubmission(stepOneQuantum());
	} while (std::chrono::steady_clock::now() < minDeadline && hasActiveExecution());
}

ResourceLoadingController::FrameProcessingMode ResourceLoadingController::currentFrameProcessingMode() const
{
	ASSERT(activeSubmission, "currentFrameProcessingMode without active submission");
	ASSERT(hasActiveExecution(), "currentFrameProcessingMode without active execution");
	return topFrame().policy.frameMode;
}

bool ResourceLoadingController::loadingScreenHandledByController() const
{
	if (!activeSubmission)
	{
		return false;
	}
	if (!hasActiveExecution())
	{
		return activeSubmission->policy.showLoadingScreen;
	}
	return topFrame().policy.showLoadingScreen;
}

bool ResourceLoadingController::isExecutingLoadingCoroutine() const noexcept
{
	return hasActiveExecution()
	    && topFrame().state == ExecutionFrameState::Running;
}
