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

#include "gamestate_snapshot_ring.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/gamelib/gtime.h"

#include "gamestate_capture.h"
#include "gamestate_snapshot.h"

#include <zlib.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>

namespace gamestate
{

namespace
{

double msSince(std::chrono::steady_clock::time_point start)
{
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

// A stored snapshot: the document compressed, and the rest of the snapshot as taken
struct Entry
{
	uint32_t gameTime = 0;
	std::vector<uint8_t> compressedDocument;
	size_t documentSize = 0;
	GameStateSnapshot rest;  ///< document empty
};

struct Ring
{
	SnapshotRingOptions options;

	// Main thread only
	bool outstanding = false;        ///< a job was handed to the worker and its completion not yet seen
	uint32_t newestTaken = 0;        ///< game time of the newest snapshot taken (stored or in flight)
	bool anyTaken = false;
	bool deferredDue = false;        ///< the snapshot due now was already counted as deferred
	size_t deferred = 0;
	size_t taken = 0;
	double captureMs = 0;
	double maxCaptureMs = 0;

	// Shared with the worker, under mutex
	WZ_MUTEX *mutex = nullptr;
	WZ_SEMAPHORE *jobAvailable = nullptr;  ///< posted for each job, and to quit
	WZ_SEMAPHORE *jobDone = nullptr;       ///< posted by the worker when a job is done
	WZ_THREAD *thread = nullptr;
	bool quit = false;
	std::unique_ptr<GameStateSnapshotCapture> job;
	std::vector<std::shared_ptr<const Entry>> entries;  ///< oldest first
	size_t compressedBytes = 0;
	size_t documentBytes = 0;
	size_t thinned = 0;
	double workerMs = 0;
	std::vector<std::string> failures;  ///< snapshots the worker couldn't finish, for the main thread to report
};

Ring *ring = nullptr;

class MutexLock
{
public:
	explicit MutexLock(WZ_MUTEX *m) : m(m) { wzMutexLock(m); }
	~MutexLock() { wzMutexUnlock(m); }
	MutexLock(const MutexLock &) = delete;
	MutexLock &operator=(const MutexLock &) = delete;
private:
	WZ_MUTEX *m;
};

// Under the mutex: drop interior snapshots until the kept ones fit the budget,
// each time the one whose neighbors are closest together.
// The first (the start of playback) and the newest are kept.
void thinToBudget(Ring &r)
{
	while (r.compressedBytes > r.options.memoryBudget && r.entries.size() > 2)
	{
		size_t best = 1;
		uint32_t bestSpan = std::numeric_limits<uint32_t>::max();
		for (size_t i = 1; i + 1 < r.entries.size(); ++i)
		{
			const uint32_t span = r.entries[i + 1]->gameTime - r.entries[i - 1]->gameTime;
			if (span < bestSpan)
			{
				bestSpan = span;
				best = i;
			}
		}
		r.compressedBytes -= r.entries[best]->compressedDocument.size();
		r.documentBytes -= r.entries[best]->documentSize;
		r.entries.erase(r.entries.begin() + best);
		++r.thinned;
	}
}

// Worker: check, write out, compress and store one captured snapshot
void processJob(Ring &r, std::unique_ptr<GameStateSnapshotCapture> capture)
{
	const auto start = std::chrono::steady_clock::now();
	auto entry = std::make_shared<Entry>();
	entry->gameTime = capture->snapshot.gameTime;
	std::string failure;
	try
	{
		GameStateSnapshot snapshot = finishGameStateSnapshot(std::move(*capture));
		capture.reset();  // (frees the document's memory form here, off the main thread)
		uLongf size = compressBound(static_cast<uLong>(snapshot.document.size()));
		entry->compressedDocument.resize(size);
		const int result = compress2(entry->compressedDocument.data(), &size, reinterpret_cast<const Bytef *>(snapshot.document.data()),
		                             static_cast<uLong>(snapshot.document.size()), r.options.compressionLevel);
		if (result != Z_OK)
		{
			failure = "compressing the document failed (zlib error " + std::to_string(result) + ")";
		}
		else
		{
			entry->compressedDocument.resize(size);
			entry->compressedDocument.shrink_to_fit();
			entry->documentSize = snapshot.document.size();
			snapshot.document.clear();
			snapshot.document.shrink_to_fit();
			entry->rest = std::move(snapshot);
		}
	}
	catch (const std::exception &e)
	{
		failure = e.what();
	}
	const double ms = msSince(start);

	MutexLock lock(r.mutex);
	r.workerMs += ms;
	if (!failure.empty())
	{
		r.failures.push_back("snapshot at gameTime " + std::to_string(entry->gameTime) + ": " + failure);
		return;
	}
	r.compressedBytes += entry->compressedDocument.size();
	r.documentBytes += entry->documentSize;
	// Taken in game-time order, past the newest one (see the header)
	ASSERT(r.entries.empty() || r.entries.back()->gameTime < entry->gameTime, "Snapshot ring: snapshot out of order");
	r.entries.push_back(std::move(entry));
	thinToBudget(r);
}

int workerMain(void *data)
{
	Ring &r = *static_cast<Ring *>(data);
	while (true)
	{
		wzSemaphoreWait(r.jobAvailable);
		std::unique_ptr<GameStateSnapshotCapture> job;
		{
			MutexLock lock(r.mutex);
			if (r.quit)
			{
				break;
			}
			job = std::move(r.job);
		}
		if (job)
		{
			processJob(r, std::move(job));
		}
		wzSemaphorePost(r.jobDone);
	}
	return 0;
}

// Main thread: see whether the job handed out has been done, and wait for it if asked
void collectJob(Ring &r, bool wait)
{
	if (!r.outstanding)
	{
		return;
	}
	if (wait)
	{
		wzSemaphoreWait(r.jobDone);
		r.outstanding = false;
	}
	else if (wzSemaphoreWaitTimeout(r.jobDone, 0))
	{
		r.outstanding = false;
	}
}

void reportFailures(Ring &r)
{
	std::vector<std::string> failures;
	{
		MutexLock lock(r.mutex);
		failures.swap(r.failures);
	}
	for (const auto &failure : failures)
	{
		ASSERT(false, "Snapshot ring: a snapshot of this game fails its own check, or can't be stored: %s", failure.c_str());
	}
}

} // anonymous namespace

void snapshotRingStart(const SnapshotRingOptions &options)
{
	snapshotRingStop();
	ring = new Ring();
	ring->options = options;
	ring->options.interval = std::max<uint32_t>(ring->options.interval, GAME_TICKS_PER_UPDATE);
	ring->mutex = wzMutexCreate();
	ring->jobAvailable = wzSemaphoreCreate(0);
	ring->jobDone = wzSemaphoreCreate(0);
	ring->thread = wzThreadCreate(workerMain, ring, "wzSnapshotRing");
	wzThreadStart(ring->thread);
	debug(LOG_INFO, "Snapshot ring started: every %u ms of game time, up to %zu KB compressed (zlib level %d)",
	      ring->options.interval, ring->options.memoryBudget / 1024, ring->options.compressionLevel);
}

void snapshotRingStop()
{
	if (ring == nullptr)
	{
		return;
	}
	collectJob(*ring, true);
	reportFailures(*ring);
	const SnapshotRingStats stats = snapshotRingStats();
	{
		MutexLock lock(ring->mutex);
		ring->quit = true;
	}
	wzSemaphorePost(ring->jobAvailable);
	wzThreadJoin(ring->thread);
	wzSemaphoreDestroy(ring->jobAvailable);
	wzSemaphoreDestroy(ring->jobDone);
	wzMutexDestroy(ring->mutex);
	debug(LOG_INFO, "Snapshot ring stopped: %zu snapshot(s) taken, %zu kept (%zu KB compressed, %zu KB written out), %zu thinned out, "
	      "%zu deferred; main thread %.1f ms per snapshot on average (at most %.1f), worker %.1f ms",
	      stats.taken, stats.count, stats.compressedBytes / 1024, stats.documentBytes / 1024, stats.thinned, stats.deferred,
	      stats.taken ? stats.captureMs / stats.taken : 0.0, stats.maxCaptureMs, stats.taken ? stats.workerMs / stats.taken : 0.0);
	delete ring;
	ring = nullptr;
}

bool snapshotRingRunning()
{
	return ring != nullptr;
}

void snapshotRingAtCapturePoint()
{
	if (ring == nullptr)
	{
		return;
	}
	Ring &r = *ring;
	collectJob(r, false);
	reportFailures(r);
	const bool due = !r.anyTaken || gameTime >= r.newestTaken + r.options.interval;
	if (!due || !gameStateCaptureSafe())
	{
		return;
	}
	if (r.outstanding && r.options.waitForWorker)
	{
		collectJob(r, true);
		reportFailures(r);
	}
	if (r.outstanding)
	{
		// The worker is still on the previous one (playback can outrun it when fast-forwarding):
		// take this one at a later capture point
		if (!r.deferredDue)
		{
			++r.deferred;
			r.deferredDue = true;
		}
		return;
	}
	const auto start = std::chrono::steady_clock::now();
	std::unique_ptr<GameStateSnapshotCapture> capture;
	try
	{
		capture = std::make_unique<GameStateSnapshotCapture>(captureGameStateSnapshot());
	}
	catch (const std::exception &e)
	{
		ASSERT(false, "Snapshot ring: can't take a snapshot at gameTime %u: %s", gameTime, e.what());
		r.anyTaken = true;  // (don't retry at every capture point)
		r.newestTaken = gameTime;
		return;
	}
	const double ms = msSince(start);
	debug(LOG_INFO, "Snapshot ring took a snapshot at gameTime %u (%.1f ms on the main thread)", gameTime, ms);
	r.captureMs += ms;
	r.maxCaptureMs = std::max(r.maxCaptureMs, ms);
	++r.taken;
	r.deferredDue = false;
	r.anyTaken = true;
	r.newestTaken = gameTime;
	{
		MutexLock lock(r.mutex);
		r.job = std::move(capture);
	}
	r.outstanding = true;
	wzSemaphorePost(r.jobAvailable);
}

nonstd::optional<GameStateSnapshot> snapshotRingFindAtOrBefore(uint32_t time, std::string *error)
{
	auto fail = [error](const std::string &why) -> nonstd::optional<GameStateSnapshot> {
		if (error != nullptr)
		{
			*error = why;
		}
		return nonstd::nullopt;
	};
	if (ring == nullptr)
	{
		return fail("the snapshot ring isn't running");
	}
	collectJob(*ring, true);
	reportFailures(*ring);
	std::shared_ptr<const Entry> entry;
	{
		MutexLock lock(ring->mutex);
		for (auto it = ring->entries.rbegin(); it != ring->entries.rend(); ++it)
		{
			if ((*it)->gameTime <= time)
			{
				entry = *it;
				break;
			}
		}
	}
	if (!entry)
	{
		return fail("no snapshot at or before gameTime " + std::to_string(time));
	}
	GameStateSnapshot snapshot = entry->rest;
	snapshot.document.resize(entry->documentSize);
	uLongf size = static_cast<uLongf>(entry->documentSize);
	const int result = uncompress(reinterpret_cast<Bytef *>(&snapshot.document[0]), &size, entry->compressedDocument.data(),
	                              static_cast<uLong>(entry->compressedDocument.size()));
	if (result != Z_OK || size != entry->documentSize)
	{
		return fail("the snapshot at gameTime " + std::to_string(entry->gameTime) + " can't be decompressed (zlib error " + std::to_string(result) + ")");
	}
	return snapshot;
}

std::vector<uint32_t> snapshotRingTimes()
{
	std::vector<uint32_t> times;
	if (ring == nullptr)
	{
		return times;
	}
	collectJob(*ring, true);
	MutexLock lock(ring->mutex);
	for (const auto &entry : ring->entries)
	{
		times.push_back(entry->gameTime);
	}
	return times;
}

std::vector<uint32_t> snapshotRingStoredTimes()
{
	std::vector<uint32_t> times;
	if (ring == nullptr)
	{
		return times;
	}
	MutexLock lock(ring->mutex);
	times.reserve(ring->entries.size());
	for (const auto &entry : ring->entries)
	{
		times.push_back(entry->gameTime);
	}
	return times;
}

bool snapshotRingDrop(uint32_t time)
{
	if (ring == nullptr)
	{
		return false;
	}
	collectJob(*ring, true);
	MutexLock lock(ring->mutex);
	for (auto it = ring->entries.begin(); it != ring->entries.end(); ++it)
	{
		if ((*it)->gameTime == time)
		{
			ring->compressedBytes -= (*it)->compressedDocument.size();
			ring->documentBytes -= (*it)->documentSize;
			ring->entries.erase(it);
			return true;
		}
	}
	return false;
}

SnapshotRingStats snapshotRingStats()
{
	SnapshotRingStats stats;
	if (ring == nullptr)
	{
		return stats;
	}
	stats.taken = ring->taken;
	stats.deferred = ring->deferred;
	stats.captureMs = ring->captureMs;
	stats.maxCaptureMs = ring->maxCaptureMs;
	MutexLock lock(ring->mutex);
	stats.count = ring->entries.size();
	stats.compressedBytes = ring->compressedBytes;
	stats.documentBytes = ring->documentBytes;
	stats.thinned = ring->thinned;
	stats.workerMs = ring->workerMs;
	return stats;
}

} // namespace gamestate
