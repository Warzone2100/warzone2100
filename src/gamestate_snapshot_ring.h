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
/** \file
 *  The snapshot ring: in-process snapshots of the running game, taken every so often of game time
 *  and kept in memory, for replay seek-back (gamestate_replay_seek.h).
 *
 *  Only building the document reads the game,
 *  so only that is done on the main thread, at a capture point (captureGameStateSnapshot).
 *  A worker thread then checks it, writes it out, compresses it (zlib) and stores it.
 *  The worker takes one snapshot at a time.
 *  A snapshot that comes due while it is still busy is taken at the next capture point instead.
 *  Above the memory budget, snapshots are thinned out evenly (the first one is kept).
 *
 *  Snapshots are only taken past the newest one kept:
 *  after a seek back, the game time that is played again is covered already.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <nonstd/optional.hpp>

struct GameStateSnapshot;

namespace gamestate
{

struct SnapshotRingOptions
{
	uint32_t interval = 30000;               ///< game time between two snapshots (ms)
	size_t memoryBudget = 64 * 1024 * 1024;  ///< compressed bytes to keep at most, before snapshots are thinned out
	int compressionLevel = 3;                ///< zlib level
	/// Test-only: when a snapshot comes due while the worker is still on the previous one,
	/// wait for it instead of taking the snapshot later
	bool waitForWorker = false;
};

/// Start taking snapshots: the first at the next capture point. Stops a ring already running first.
void snapshotRingStart(const SnapshotRingOptions &options);

/// Stop: waits for the worker, ends it, and frees every snapshot. Nothing if it isn't running.
/// Must be called before the stats are freed (the worker's check reads them): stageThreeShutDown does.
void snapshotRingStop();

bool snapshotRingRunning();

/// At the capture point of every game-loop iteration (main thread):
/// take a snapshot if one is due and the worker is free,
/// and report a snapshot the worker found broken (an assertion: a serializer bug).
void snapshotRingAtCapturePoint();

/// The newest snapshot at or before the given game time, written out
/// (main thread, waits for the worker if a snapshot is in flight).
/// nullopt if there is none, or if it can't be decompressed (error says why).
nonstd::optional<GameStateSnapshot> snapshotRingFindAtOrBefore(uint32_t gameTime, std::string *error = nullptr);

/// The game times of the snapshots kept, oldest first (main thread, waits for the worker).
std::vector<uint32_t> snapshotRingTimes();

/// The game times of the snapshots stored so far, oldest first, without waiting for the worker
/// (a snapshot it is still working on isn't in yet). For display.
std::vector<uint32_t> snapshotRingStoredTimes();

/// Drop the snapshot taken at that game time (one that couldn't be restored), so it isn't tried again.
/// False if there is none (main thread, waits for the worker).
bool snapshotRingDrop(uint32_t gameTime);

struct SnapshotRingStats
{
	size_t count = 0;              ///< snapshots kept
	size_t compressedBytes = 0;    ///< their compressed documents
	size_t documentBytes = 0;      ///< their documents, uncompressed
	size_t taken = 0;              ///< snapshots taken since the start
	size_t thinned = 0;            ///< snapshots dropped to stay in the budget
	size_t deferred = 0;           ///< snapshots taken later than due, waiting for the worker
	double captureMs = 0;          ///< main-thread time taking them (total)
	double maxCaptureMs = 0;       ///< the longest one
	double workerMs = 0;           ///< worker time checking, writing out and compressing them (total)
};
SnapshotRingStats snapshotRingStats();

} // namespace gamestate
