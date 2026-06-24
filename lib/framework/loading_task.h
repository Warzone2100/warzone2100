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
/** @file loading_task.h
 * C++20 coroutine primitives for cooperative load pipelines
 * (driven by `ResourceLoadingController::step()`).
 */

#pragma once

#include "load_result.h"
#include "loading_task_controller_ops.h"
#include "loading_task_fwd.h"
#include "resource_loading_frame_policy.h"

#include "lib/framework/wzapp.h"

#include <coroutine>
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

class ResourceLoadingController;

/// <summary>
/// Shared promise state; embedded as the first base of every `LoadingTaskPromise<T>`.
/// Holds the bound `ResourceLoadingController`, optional per-task `framePolicy`, and
/// any stored exception from `unhandled_exception()`.
/// </summary>
struct LoadingTaskPromiseBase
{
	ResourceLoadingController* controller = nullptr;
	std::exception_ptr exception;
	std::optional<ResourceLoadingFramePolicy> framePolicy;
};

template <typename T = void>
struct LoadingTaskPromise : LoadingTaskPromiseBase
{
	LoadResultStorage<T> result;

	LoadingTask<T> get_return_object() noexcept;

	std::suspend_always initial_suspend() noexcept { return {}; }

	struct FinalAwaiter
	{
		bool await_ready() const noexcept { return false; }

		void await_suspend(std::coroutine_handle<LoadingTaskPromise<T>> h) const noexcept
		{
			auto& promise = h.promise();
			loading_task_detail::notifyFrameFinished(
			    &promise, h, promise.result.succeeded());
		}

		void await_resume() const noexcept {}
	};

	FinalAwaiter final_suspend() noexcept { return {}; }

	void return_value(LoadResult<T> value) noexcept { result.set(std::move(value)); }

	void return_value(tl::unexpected<LoadError> error) noexcept
	{
		result.set(std::move(error));
	}

	template <typename U = T>
	std::enable_if_t<!std::is_void_v<U>, void> return_value(U value)
	{
		result.set(std::move(value));
	}

	void unhandled_exception() noexcept
	{
		exception = std::current_exception();
		result.set(load_fail());
	}
};

/// <summary>
/// Coroutine return type for cooperative load work (`co_return load_ok()` / `co_return load_fail()`).
/// Compose pipelines with `co_await` on another `LoadingTask` (rvalue). Override nested frame
/// behavior with `setFramePolicy()` before the child is awaited.
///
/// With `ResourceLoadingController`:
/// * Submit a root task via `request()` (wraps `LoadingTaskHandle` internally).
/// * Split work with `co_await controller.yieldFrame()`; the main loop
///   or `runTaskToCompletion()` advances the coroutine -  do not busy-wait on the main thread.
/// * Nest with `co_await child_task`; the child shares the parent's controller binding.
/// * Until `request()`, this RAII type owns the coroutine; after `release()` / submit, the
///   controller owns destruction. See `ResourceLoadingController` for scheduling and constraints.
/// * Success payload `T` must be movable (not necessarily copyable). Use `take_result()` to move
///   the outcome out once; it is not readable from a copying accessor.
/// </summary>
template <typename T>
class LoadingTask
{
public:
	using promise_type = LoadingTaskPromise<T>;
	using result_type = LoadResult<T>;

	LoadingTask() = default;

	LoadingTask(const LoadingTask&) = delete;
	LoadingTask& operator=(const LoadingTask&) = delete;

	LoadingTask(LoadingTask&& other) noexcept
		: coro(std::exchange(other.coro, {}))
	{}

	LoadingTask& operator=(LoadingTask&& other) noexcept
	{
		if (this != &other)
		{
			destroy();
			coro = std::exchange(other.coro, {});
		}
		return *this;
	}

	~LoadingTask() { destroy(); }

	bool empty() const noexcept { return coro == nullptr; }

	bool done() const noexcept { return coro == nullptr || coro.done(); }

	result_type take_result() noexcept
	{
		return coro ? coro.promise().result.take() : load_fail();
	}

	void setFramePolicy(ResourceLoadingFramePolicy policy) noexcept
	{
		if (coro)
		{
			coro.promise().framePolicy = policy;
		}
	}

	/// Detach handle for `ResourceLoadingController::start` (controller owns destruction).
	std::coroutine_handle<promise_type> release() noexcept
	{
		return std::exchange(coro, {});
	}

	struct ChildTaskAwaiter
	{
		LoadingTask* child = nullptr;
		mutable std::coroutine_handle<promise_type> child_handle{};

		bool await_ready() const noexcept
		{
			return child == nullptr || child->done();
		}

		LoadResult<T> await_resume() noexcept
		{
			if (child_handle)
			{
				// Child handle destroyed here after controller stack pop (see controller ops).
				ASSERT(child_handle.done(), "nested child must be done before destroy");
				LoadResult<T> outcome = child_handle.promise().result.take();
				child_handle.destroy();
				return outcome;
			}
			return child ? child->take_result() : load_fail();
		}

		void await_suspend(std::coroutine_handle<> parent)
		{
			ASSERT(child != nullptr, "co_await null LoadingTask");
			auto child_coro = child->coro;
			ASSERT(child_coro, "co_await empty LoadingTask");
			ASSERT(!child_coro.done(), "co_await already-completed LoadingTask");
			child->coro = {};
			child_handle = child_coro;

			loading_task_detail::suspendAwaitChild(
			    parent,
			    child_coro,
			    static_cast<LoadingTaskPromiseBase*>(&child_coro.promise()));
		}
	};

	ChildTaskAwaiter operator co_await() &&;

private:
	friend class ResourceLoadingController;
	friend struct LoadingTaskPromise<T>;

	explicit LoadingTask(std::coroutine_handle<promise_type> h) noexcept
		: coro(h)
	{}

	void destroy() noexcept
	{
		if (coro)
		{
			coro.destroy();
			coro = {};
		}
	}

	std::coroutine_handle<promise_type> coro{};
};

template <typename T>
LoadingTask<T> LoadingTaskPromise<T>::get_return_object() noexcept
{
	return LoadingTask<T>{std::coroutine_handle<LoadingTaskPromise<T>>::from_promise(*this)};
}

template <typename T>
typename LoadingTask<T>::ChildTaskAwaiter LoadingTask<T>::operator co_await() &&
{
	return ChildTaskAwaiter{this};
}

/// <summary>
/// Type-erased handle for a root loading coroutine passed to `ResourceLoadingController::request()`.
/// </summary>
class LoadingTaskHandle
{
public:
	LoadingTaskHandle() = default;

	template <typename T>
	explicit LoadingTaskHandle(LoadingTask<T>&& task)
	{
		auto typedHandle = task.release();
		if (!typedHandle)
		{
			return;
		}
		handle_ = typedHandle;
		promiseBase_ = static_cast<LoadingTaskPromiseBase*>(&typedHandle.promise());
	}

	LoadingTaskHandle(const LoadingTaskHandle&) = delete;
	LoadingTaskHandle& operator=(const LoadingTaskHandle&) = delete;

	LoadingTaskHandle(LoadingTaskHandle&& other) noexcept
		: handle_(std::exchange(other.handle_, {}))
		, promiseBase_(std::exchange(other.promiseBase_, nullptr))
	{}

	LoadingTaskHandle& operator=(LoadingTaskHandle&& other) noexcept
	{
		if (this != &other)
		{
			destroy();
			handle_ = std::exchange(other.handle_, {});
			promiseBase_ = std::exchange(other.promiseBase_, nullptr);
		}
		return *this;
	}

	~LoadingTaskHandle() { destroy(); }

	bool empty() const noexcept { return handle_ == nullptr; }

	struct ReleasedSession
	{
		std::coroutine_handle<> handle{};
		LoadingTaskPromiseBase* promiseBase = nullptr;
	};

	ReleasedSession release() noexcept
	{
		return {std::exchange(handle_, {}), std::exchange(promiseBase_, nullptr)};
	}

	LoadingTaskPromiseBase* promiseBase() const noexcept
	{
		return promiseBase_;
	}

private:
	friend class ResourceLoadingController;

	void destroy() noexcept
	{
		if (handle_)
		{
			handle_.destroy();
			handle_ = {};
			promiseBase_ = nullptr;
		}
	}

	std::coroutine_handle<> handle_{};
	LoadingTaskPromiseBase* promiseBase_ = nullptr;
};
