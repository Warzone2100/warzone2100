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
/** @file mainthreadexec.h
 * Work handed to the main thread by `wzAsyncExecOnMainThread` (see wzapp.h).
 */

#ifndef __INCLUDED_LIB_FRAMEWORK_MAINTHREADEXEC_H__
#define __INCLUDED_LIB_FRAMEWORK_MAINTHREADEXEC_H__

#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

class WZ_MAINTHREADEXEC
{
public:
	WZ_MAINTHREADEXEC() = default;
	virtual ~WZ_MAINTHREADEXEC() = default;

	virtual void doExecOnMainThread() = 0;
};

class WZ_MAINTHREADEXECFUNC: public WZ_MAINTHREADEXEC
{
public:
	typedef std::function<void ()> execFuncType;
public:
	WZ_MAINTHREADEXECFUNC(const execFuncType &execFunc)
	: execFunc(execFunc)
	{ }
	virtual ~WZ_MAINTHREADEXECFUNC() { };

	void doExecOnMainThread() override
	{
		execFunc();
	};
private:
	execFuncType execFunc;
};

namespace wz
{

/// <summary>
/// FIFO queue of work to run on the main thread.
///
/// Any thread may `post`. Only the main thread may call `process` or `shutdown`.
///
/// Two vectors take turns: posts append to one while the main thread runs the other, and `process` swaps them.
/// Both keep their capacity, so once they have grown to the usual burst size a post is a mutex lock and a pointer write, with no allocation.
/// </summary>
class MainThreadExecQueue
{
public:
	using ExecPtr = std::unique_ptr<WZ_MAINTHREADEXEC>;

	/// A spare buffer larger than this is freed after a burst instead of being kept.
	static constexpr size_t kMaxRetainedCapacity = 4096;

	/// Queues `exec`, taking ownership, and returns the number of queued execs afterwards.
	/// After `shutdown`, returns 0 and leaves `exec` alone (the caller still owns it).
	size_t post(ExecPtr& exec)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!m_accepting)
		{
			return 0;
		}
		m_queue.push_back(std::move(exec));
		return m_queue.size();
	}

	/// **Main thread only.**
	/// Runs every exec queued before the call, in FIFO order, deleting each one right after it runs.
	/// Execs posted during (including from inside an exec) wait for the next call.
	/// Returns the number of execs run.
	///
	/// If an exec throws, it is deleted, the execs after it go back to the front of the queue (keeping FIFO order), and the exception propagates.
	size_t process()
	{
		// Take the spare by move, so a nested call can't clobber this batch.
		std::vector<ExecPtr> batch = std::move(m_spare);
		batch.clear();
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			batch.swap(m_queue);
		}
		for (size_t i = 0; i < batch.size(); ++i)
		{
			try
			{
				batch[i]->doExecOnMainThread();
			}
			catch (...)
			{
				requeueAtFront(batch, i + 1);
				throw;
			}
			batch[i].reset();
		}
		const size_t ran = batch.size();
		batch.clear();
		if (batch.capacity() > kMaxRetainedCapacity)
		{
			batch.shrink_to_fit();
		}
		m_spare = std::move(batch);
		return ran;
	}

	/// **Main thread only.**
	/// Stops accepting posts, and returns the execs that were queued but never run.
	std::vector<ExecPtr> shutdown()
	{
		std::vector<ExecPtr> pending;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_accepting = false;
			pending.swap(m_queue);
		}
		m_spare = std::vector<ExecPtr>();
		return pending;
	}

	size_t pendingCount() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.size();
	}

private:
	void requeueAtFront(std::vector<ExecPtr>& batch, size_t firstNotRun)
	{
		if (firstNotRun >= batch.size())
		{
			return;
		}
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.insert(m_queue.begin(),
		               std::make_move_iterator(batch.begin() + static_cast<std::ptrdiff_t>(firstNotRun)),
		               std::make_move_iterator(batch.end()));
	}

private:
	mutable std::mutex m_mutex;
	std::vector<ExecPtr> m_queue;	// guarded by m_mutex
	bool m_accepting = true;		// guarded by m_mutex
	std::vector<ExecPtr> m_spare;	// main thread only
};

} // namespace wz

#endif // __INCLUDED_LIB_FRAMEWORK_MAINTHREADEXEC_H__
