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
/** @file mainthreadexec.cpp
 * The process-wide queue behind `wzAsyncExecOnMainThread`.
 */

#include "frame.h"
#include "wzapp.h"
#include "mainthreadexec.h"

#include <atomic>

namespace
{

// Should never approach this - something is posting far faster than the main loop is draining it.
constexpr size_t kLargeQueueWarningThreshold = 10000;
std::atomic<bool> warnedLargeQueue(false);

wz::MainThreadExecQueue& mainThreadExecQueue()
{
	// Never destroyed:
	// - a detached thread may still post while the process exits, and must not find the queue partially / fully torn down
	static wz::MainThreadExecQueue* queue = new wz::MainThreadExecQueue();
	return *queue;
}

} // anonymous namespace

void wzAsyncExecOnMainThread(WZ_MAINTHREADEXEC *exec)
{
	wz::MainThreadExecQueue::ExecPtr execPtr(exec);
	const size_t queuedCount = mainThreadExecQueue().post(execPtr);
	if (execPtr)
	{
		// The main loop has shut down, so this will never run.
		// Leak it on purpose:
		// - deleting it here would run its destructor on this thread, while the main thread tears (or has torn) things down
		(void)execPtr.release();
		return;
	}
	if (queuedCount >= kLargeQueueWarningThreshold && !warnedLargeQueue.exchange(true))
	{
		debug(LOG_INFO, "Main-thread exec queue has %zu pending entries - ??", queuedCount);
	}
}

void wzProcessMainThreadExecs()
{
	mainThreadExecQueue().process();
}

void wzShutdownMainThreadExecs()
{
	std::vector<wz::MainThreadExecQueue::ExecPtr> abandoned = mainThreadExecQueue().shutdown();
	if (!abandoned.empty())
	{
		debug(LOG_WZ, "Abandoning %zu main-thread exec(s) queued at shutdown", abandoned.size());
	}
	// Leak these on purpose: their destructors could reach into systems that have already shut down
	for (auto& exec : abandoned)
	{
		(void)exec.release();
	}
}
