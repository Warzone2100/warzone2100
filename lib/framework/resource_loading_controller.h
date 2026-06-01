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
/** @file resource_loading_controller.h
 * Cooperative loading scheduler: runs `LoadingTask` coroutines across main-loop frames
 * instead of one blocking call.
 */

#pragma once

#include "load_result.h"
#include "loading_task.h"
#include "resource_loading_frame_policy.h"
#include "loading_task_controller_ops.h"

#include "lib/framework/wzapp.h"

#include <coroutine>
#include <memory>
#include <queue>
#include <stack>
#include <utility>
#include <vector>

/// <summary>
/// Global cooperative loading scheduler (singleton). Uses coroutine-based tasks
/// to run load pipelines as sequential `LoadingTask` coroutines while returning to
/// `mainLoop` each frame (responsive input, audio, loading UI).
///
/// Schedules work in frame quanta instead of one long blocking call on the main thread.
/// Queues multiple load requests fairly: one active submission, pending FIFO.
///
/// How it works:
/// * `request()` starts work immediately or enqueues it when a submission is already active.
/// * Inside a task, `co_await controller.yieldFrame()` suspends until the next quantum.
/// * `step()` (main loop) calls `stepOneQuantum()` in a loop until ~1/60 s elapses or the
///   session finishes; each quantum resumes the top execution-stack frame from `Paused`.
/// * `runTaskToCompletion()` drains quanta on the calling thread when the controller is idle.
/// * `FramePolicy` selects `ConsumeFrame` vs `ContinueMainLoop` and whether the loading
///   screen is shown (`presentResourceLoadingScreenIfNeeded()`).
/// * Nested work uses `co_await child_task` (see `loading_task_controller_ops`).
///
/// Usage constraints:
/// * Call `yieldFrame()` only from inside a `LoadingTask` that the controller is driving
///   (`request()` / `runTaskToCompletion()`). Split long work with `co_await yieldFrame()`;
///   do not spin on the main thread waiting for the controller.
/// * Use `co_await child_task` for nested loads; do not call `runTaskToCompletion()` on a child
///   while a cooperative session is already active.
/// * Call `runTaskToCompletion()` only when `!active()` and not from inside a loading coroutine;
///   otherwise use `co_await` / `yieldFrame()`.
/// * `isExecutingLoadingCoroutine()` is true only during an in-flight `resume()`, not when the
///   task is merely paused after `yieldFrame`. Use `active()` for “a submission is in progress”.
/// * Query `currentFrameProcessingMode()` and `loadingScreenHandledByController()` only while
///   `active()` (during `step()` / inside a running load).
/// </summary>
class ResourceLoadingController
{
public:

	using FrameProcessingMode = ResourceLoadingFramePolicy::FrameProcessingMode;
	using FramePolicy = ResourceLoadingFramePolicy;

	/// Optional hook invoked after each `stepOneQuantum()` during blocking drains.
	using BetweenQuantumCallback = void (*)(const FramePolicy& policy);

	struct FrameYield;

	static ResourceLoadingController& instance();

	ResourceLoadingController(const ResourceLoadingController&) = delete;
	ResourceLoadingController& operator=(const ResourceLoadingController&) = delete;

	ResourceLoadingController(ResourceLoadingController&&) = delete;
	ResourceLoadingController& operator=(ResourceLoadingController&&) = delete;

	// Submit loading work. If there is an active submission, queue the follow-up;
	// otherwise begin immediately.
	template <typename T = void>
	void request(LoadingTask<T> task, FramePolicy policy);

	// Returns true if there is an active submission.
	bool active() const;

	// Advance the active load by one frame quantum.
	void step();

	// Run a loading task to completion on the calling thread.
	// Preconditions (debug ASSERT): !active(), !isExecutingLoadingCoroutine().
	template <typename T = void>
	LoadResult<T> runTaskToCompletion(LoadingTask<T> task,
	                                  FramePolicy policy = {FrameProcessingMode::ConsumeFrame, false},
	                                  BetweenQuantumCallback betweenQuantum = nullptr);

	// Reads `FramePolicy` from the execution-stack top.
	FrameProcessingMode currentFrameProcessingMode() const;

	// When true, mainLoop presents the loading screen via `presentResourceLoadingScreenIfNeeded()`.
	bool loadingScreenHandledByController() const;

	// True while a loading coroutine frame is actively running (not merely paused/yielded).
	bool isExecutingLoadingCoroutine() const noexcept;

	// Returns an awaitable that suspends the current coroutine until the next `stepOneQuantum()`.
	FrameYield yieldFrame() noexcept;

private:

	friend struct FrameYield;
	template <typename T>
	friend class LoadingTask;
	template <typename T>
	friend struct LoadingTaskPromise;

	friend void loading_task_detail::notifyFrameFinished(
	    LoadingTaskPromiseBase*, std::coroutine_handle<>, bool) noexcept;
	friend void loading_task_detail::suspendAwaitChild(
	    std::coroutine_handle<>, std::coroutine_handle<>, LoadingTaskPromiseBase*);

	struct ExecutionFrame
	{
		std::coroutine_handle<> handle{};
		ExecutionFrameState state = ExecutionFrameState::Paused;
		FramePolicy policy{};
		LoadingTaskPromiseBase* promiseBase = nullptr;
	};

	struct ResourceLoadingSubmission;

	explicit ResourceLoadingController() = default;
	~ResourceLoadingController();

	void begin(std::unique_ptr<ResourceLoadingSubmission> submission);
	void startNextPendingSubmission();

	void requestImpl(LoadingTaskHandle task, FramePolicy policy);
	void start(LoadingTaskHandle task, FramePolicy policy);
	LoadStepStatus stepOneQuantum();
	void completeActiveSubmission(LoadStepStatus result);
	void resetTaskState() noexcept;

	ExecutionFrame& topFrame();
	const ExecutionFrame& topFrame() const;
	void pushFrame(std::coroutine_handle<> handle, FramePolicy policy, LoadingTaskPromiseBase* promiseBase);
	void popAndDestroyTop() noexcept;
	void onFrameFinished(bool succeeded) noexcept;
	bool hasActiveExecution() const noexcept { return !executionStack.empty(); }

	std::unique_ptr<ResourceLoadingSubmission> activeSubmission;
	std::queue<std::unique_ptr<ResourceLoadingSubmission>> pendingSubmissions; // FIFO while active

	std::stack<ExecutionFrame, std::vector<ExecutionFrame>> executionStack; // root + nested children
	std::coroutine_handle<> sessionRootHandle{}; // root handle kept until submission completes
	bool terminalSucceeded = true;
	bool sessionFinished = false;
};

/// <summary>
/// Awaitable from `yieldFrame()`: suspends the execution-stack top until the next
/// `stepOneQuantum()` / `step()` resume.
/// </summary>
struct ResourceLoadingController::FrameYield
{
	ResourceLoadingController* controller = nullptr;

	bool await_ready() const noexcept { return false; }

	void await_suspend(std::coroutine_handle<> h) const noexcept;

	void await_resume() const noexcept {}
};

template <typename T>
LoadResult<T> ResourceLoadingController::runTaskToCompletion(LoadingTask<T> task, FramePolicy policy,
                                                             BetweenQuantumCallback betweenQuantum)
{
	ASSERT(!task.empty(), "runTaskToCompletion given empty LoadingTask");
	ASSERT(!isExecutingLoadingCoroutine(),
	       "runTaskToCompletion called from inside a LoadingTask coroutine; use co_await instead");
	ASSERT(!active(), "runTaskToCompletion requires idle controller");

	request(std::move(task), policy);

	LoadResult<T> result = load_fail();
	while (active())
	{
		const LoadStepStatus status = stepOneQuantum();
		if (betweenQuantum)
		{
			betweenQuantum(policy);
		}
		if (status != LoadStepStatus::InProgress)
		{
			if (sessionRootHandle)
			{
				result = std::coroutine_handle<LoadingTaskPromise<T>>::from_address(sessionRootHandle.address())
				             .promise()
				             .result.take();
			}
			completeActiveSubmission(status);
		}
	}
	return result;
}

template <typename T>
void ResourceLoadingController::request(LoadingTask<T> task, FramePolicy policy)
{
	requestImpl(LoadingTaskHandle(std::move(task)), policy);
}
