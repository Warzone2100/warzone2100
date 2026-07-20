// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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

#pragma once

#include <chrono>
#include <memory>
#include <unordered_map>
#include <vector>
#include <stdint.h>

#include "lib/framework/wzapp.h"
#include "lib/netplay/net_result.h"

struct WZ_THREAD;
struct WZ_MUTEX;
struct WZ_SEMAPHORE;

class IClientConnection;
class IDescriptorSet;
class WzConnectionProvider;

/// This is a wrapper function that acts as a proxy to `PendingWritesManager::threadImplFunction`.
/// Argument is `PendingWritesManager*` cast to `void*`.
int pendingWritesThreadFunction(void*);

/// <summary>
/// Manager class, that handles all network write operations.
/// Specific to a particular `WzConnectionProvider`.
///
/// The main reason for its existence is the fact that we would like
/// to keep all socket write operations asynchronous, which provide the
/// following advantages over blocking approach:
///
/// 1. Improves responsiveness of the application.
/// 2. Provides means to batch several outstanding write requests
///    into a single larger one to save on network bandwidth.
/// 3. Provides the ability to efficiently compress the write
///    requests, absolutely transparently for the caller.
///
/// This class implements the submission queue running on
/// a separate thread, which intercepts and batches all write
/// requests, automatically handles data compression and provides
/// safe disposal of closed connections (i.e. we want to wait
/// for the data to be sent before actually terminating a client
/// connection).
/// </summary>
class PendingWritesManager
{
public:

	using ConnectionWriteQueue = std::vector<uint8_t>;

	~PendingWritesManager();

	explicit PendingWritesManager() = default;
	PendingWritesManager(const PendingWritesManager&) = delete;
	PendingWritesManager(PendingWritesManager&&) = delete;

	void initialize(WzConnectionProvider& connProvider);
	void deinitialize();

	template <typename Fn>
	void executeUnderLock(Fn&& fn)
	{
		wzMutexLock(mtx_);
		fn();
		wzMutexUnlock(mtx_);
	}

	template <typename Fn>
	void executeUnderLock(Fn&& fn) const
	{
		wzMutexLock(mtx_);
		fn();
		wzMutexUnlock(mtx_);
	}

	/// <summary>
	/// Get the submission queue for a given connection and execute a function, which
	/// may modify the write queue, in a thread-safe manner (while holding the pending writes lock).
	/// </summary>
	/// <typeparam name="AppendToWriteQueueFn">Should accept `ConnectionWriteQueue` by reference.</typeparam>
	/// <param name="conn">Client connection object to search for in the pending writes map.</param>
	/// <param name="appendFn">The function to execute against the connection's write queue.</param>
	template <typename AppendToWriteQueueFn>
	void append(IClientConnection* conn, AppendToWriteQueueFn&& appendFn)
	{
		executeUnderLock([this, conn, &appendFn]
		{
			if (pendingWrites_.empty())
			{
				wzSemaphorePost(sema_);
			}
			ConnectionWriteQueue& writeQueue = pendingWrites_[conn];
			appendFn(writeQueue);
		});
	}

	void clearPendingWrites(IClientConnection* conn)
	{
		executeUnderLock([this, conn]
		{
			pendingWrites_.erase(conn);
		});
	}

	/// <summary>
	/// Safely (in a thread-safe manner) dispose of a connection, which may have a registered pending write attached:
	///
	/// If the connection has a pending write, this method marks it as "needs to be disposed of",
	/// and it will be cleaned up at the next available opportunity.
	/// Otherwise, the connection object will be destroyed right away.
	///
	/// WARNING: don't use `conn` object after calling this function. It will eventually be destroyed!
	/// </summary>
	/// <param name="conn"></param>
	void safeDispose(IClientConnection* conn);

private:

	using ConnectionThreadWriteMap = std::unordered_map<IClientConnection*, ConnectionWriteQueue>;

	friend int pendingWritesThreadFunction(void*);

	void threadImplFunction();
	net::result<int> checkConnectionsWritable(IDescriptorSet& writableSet, std::chrono::milliseconds timeout);
	void populateWritableSet(IDescriptorSet& writableSet);

	ConnectionThreadWriteMap pendingWrites_;
	mutable WZ_MUTEX* mtx_ = nullptr;
	WZ_SEMAPHORE* sema_ = nullptr;
	WZ_THREAD* thread_ = nullptr;
	bool stopRequested_ = false;
	std::unique_ptr<IDescriptorSet> writableSet_;
};
