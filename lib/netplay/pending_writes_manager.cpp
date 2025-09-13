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

#include "pending_writes_manager.h"

#include "lib/framework/frame.h" // for ASSERT
#include "lib/netplay/descriptor_set.h"
#include "lib/netplay/client_connection.h"
#include "lib/netplay/wz_connection_provider.h"
#include "lib/netplay/error_categories.h"

#include <system_error>

PendingWritesManager::~PendingWritesManager()
{
	deinitialize();
}

void PendingWritesManager::deinitialize()
{
	if (thread_ == nullptr)
	{
		// No-op in case of a repeated `deinitialize()` call
		return;
	}
	wzMutexLock(mtx_);
	stopRequested_ = true;
	pendingWrites_.clear();
	writableSet_.reset();
	wzMutexUnlock(mtx_);
	wzSemaphorePost(sema_);  // Wake up the thread, so it can quit.
	wzThreadJoin(thread_);
	wzMutexDestroy(mtx_);
	wzSemaphoreDestroy(sema_);
	thread_ = nullptr;
	mtx_ = nullptr;
	sema_ = nullptr;
}

void PendingWritesManager::initialize(WzConnectionProvider& connProvider)
{
	if (thread_ != nullptr)
	{
		// No-op in case of a repeated `initialize()` call
		return;
	}
	writableSet_ = connProvider.newDescriptorSet(PollEventType::WRITABLE);
	stopRequested_ = false;
	mtx_ = wzMutexCreate();
	sema_ = wzSemaphoreCreate(0);
	thread_ = wzThreadCreate(pendingWritesThreadFunction, reinterpret_cast<void*>(this), "WzPendingWrites");
	wzThreadStart(thread_);
}

net::result<int> PendingWritesManager::checkConnectionsWritable(IDescriptorSet& writableSet, std::chrono::milliseconds timeout)
{
	if (writableSet.empty())
	{
		return 0;
	}

	wzMutexUnlock(mtx_);
	const auto pollRes = writableSet.poll(timeout);
	wzMutexLock(mtx_);

	if (!pollRes.has_value())
	{
		const auto msg = pollRes.error().message();
		debug(LOG_ERROR, "poll failed: %s", msg.c_str());
		return pollRes;
	}

	if (pollRes.value() == 0)
	{
		debug(LOG_WARNING, "poll timed out after waiting for %u milliseconds", static_cast<unsigned int>(timeout.count()));
		return 0;
	}

	return pollRes.value();
}

void PendingWritesManager::populateWritableSet(IDescriptorSet& writableSet)
{
	for (auto it = pendingWrites_.begin(); it != pendingWrites_.end();)
	{
		const auto& pendingConnWrite = *it;
		if (!pendingConnWrite.second.empty())
		{
			writableSet.add(pendingConnWrite.first);
			++it;
		}
		else
		{
			ASSERT(false, "Empty buffer for pending socket writes"); // This shouldn't happen!
			IClientConnection* conn = pendingConnWrite.first;
			it = pendingWrites_.erase(it);
			if (conn->deleteLaterRequested())
			{
				delete conn;
			}
		}
	}
}

void PendingWritesManager::threadImplFunction()
{
	wzMutexLock(mtx_);
	while (!stopRequested_)
	{
		static constexpr std::chrono::milliseconds WRITABLE_CHECK_TIMEOUT{ 50 };
		// Check if we can write to some connections.
		writableSet_->clear();
		populateWritableSet(*writableSet_);
		ASSERT(!writableSet_->empty() || pendingWrites_.empty(), "writableSet must not be empty if there are pending writes.");

		const auto checkWritableRes = checkConnectionsWritable(*writableSet_, WRITABLE_CHECK_TIMEOUT);

		if (checkWritableRes.has_value() && checkWritableRes.value() != 0)
		{
			for (auto connIt = pendingWrites_.begin(); connIt != pendingWrites_.end();)
			{
				auto currentIt = connIt;
				++connIt;

				IClientConnection* conn = currentIt->first;
				ConnectionWriteQueue& writeQueue = currentIt->second;
				ASSERT(!writeQueue.empty(), "writeQueue[sock] must not be empty.");

				if (!writableSet_->isSet(conn) || writeQueue.empty())
				{
					continue;  // This connection is not ready for writing, or we don't have anything to write.
				}

				// Write data.
				const auto retSent = conn->sendImpl(writeQueue);
				if (retSent.has_value())
				{
					// Erase as much data as written.
					writeQueue.erase(writeQueue.begin(), writeQueue.begin() + retSent.value());
					if (writeQueue.empty())
					{
						pendingWrites_.erase(currentIt);  // Nothing left to write, delete from pending list.
						if (conn->deleteLaterRequested())
						{
							delete conn;
						}
					}
				}
				else
				{
					if (retSent.error() == std::errc::interrupted)
					{
						// Not an actual error, just try to send again later
						continue;
					}
					const auto connStatus = conn->connectionStatus();
					if (!conn->isValid() || !connStatus.has_value()) // Check if the connection is still open
					{
						if (!connStatus.has_value())
						{
							const auto errMsg = connStatus.error().message();
							debug(LOG_NET, "Socket error: %s", errMsg.c_str());
							conn->setWriteErrorCode(connStatus.error());
						}
						else
						{
							debug(LOG_NET, "Socket error: connection is not valid");
							conn->setWriteErrorCode(make_network_error_code(ECONNRESET));
						}
						pendingWrites_.erase(currentIt);  // Connection broken, don't try writing to it again.
						if (conn->deleteLaterRequested())
						{
							delete conn;
						}
					}
				}
			}
		}
		if (pendingWrites_.empty())
		{
			// Nothing to do, expect to wait.
			wzMutexUnlock(mtx_);
			wzSemaphoreWait(sema_);
			wzMutexLock(mtx_);
		}
	}
	wzMutexUnlock(mtx_);
}

void PendingWritesManager::safeDispose(IClientConnection* conn)
{
	executeUnderLock([this, conn]
	{
		// Need a `const_cast` to make the `unordered_map` work with `const` key type.
		if (pendingWrites_.count(const_cast<IClientConnection*>(conn)) != 0)
		{
			// Wait until the data is written, then delete the socket.
			conn->requestDeleteLater();
		}
		else
		{
			// Notify the owning connection provider to properly dispose of the connection object.
			if (auto connProvider = conn->connProvider_.lock())
			{
				connProvider->disposeConnection(conn);
			}
			else
			{
				ASSERT(false, "IClientConnection::connProvider_ has gone away before safeDispose!");
			}
			// Delete the socket and destroy the connection right away.
			delete conn;
		}
	});
}

int pendingWritesThreadFunction(void* data)
{
	PendingWritesManager* inst = reinterpret_cast<PendingWritesManager*>(data);
	inst->threadImplFunction();
	// Return value ignored
	return 0;
}
