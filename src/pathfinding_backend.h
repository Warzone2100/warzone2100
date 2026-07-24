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
 *  Route planner backend interface.
 *
 *  fpath.h carries two surfaces with little in common, and this splits them.
 *  The blocking-tile queries stay free functions there: move.cpp calls them
 *  thousands of times per tick, so a virtual dispatch on that path is not worth
 *  paying for, and every planner reads the same static terrain and structure
 *  state for hard blocking anyway. The asynchronous route planner, called once
 *  per droid per repath, goes behind this interface instead, so an alternative
 *  planner can be selected without its callers changing.
 *
 *  A backend owns its worker threads, its per-tick snapshots and its job queue.
 *  Whatever it does with them, it has to keep the guarantees its callers
 *  already depend on:
 *
 *  - One outstanding job per droid. route() queues on the first call and polls
 *    on the ones after, so a caller can call it every tick while a droid waits.
 *  - A queued job whose droid has since been deleted is still processed.
 *    Discarding it would change the order the remaining jobs are handled in,
 *    and that changes the shape of other droids' paths.
 *  - Anything a worker thread reads is built on the main thread beforehand
 *    and is immutable once the worker can see it. Workers never read live
 *    droid or map state.
 *  - The sync stream stays compatible, since replays are validated against it.
 */

#pragma once

#include "fpath.h"

#include <cstdint>

struct WorldMapState;

/// Which planner a game runs. This is a synced game setting: every client, and
/// every save and replay, records it, because different backends are different
/// simulations and a game must agree on one. Values are serialized, so they are
/// fixed - only append. Legacy is the integer-grid A* that has always shipped.
enum class PathfindingBackendId : uint8_t
{
	Legacy = 0,
	Congestion = 1,
};

class IPathfindingBackend
{
public:
	virtual ~IPathfindingBackend() = default;

	virtual bool initialise() = 0;
	virtual void shutdown() = 0;

	/// Discard cached state that a load or a mission boundary has made stale.
	virtual void hardReset() = 0;

	/// Main-thread hook, called once per tick from the game loop after the map
	/// update. A backend holding per-tick snapshots rebuilds them here.
	virtual void updateTick(const WorldMapState& mapState) = 0;

	/// Queues a route request, or polls one already queued. Returns FPR_WAIT
	/// while a result is outstanding, and on FPR_OK has filled psDroid->sMove
	/// with the path and its destination.
	virtual FPATH_RETVAL route(DROID *psDroid, const WorldMapState& mapState,
	                           SDWORD targetX, SDWORD targetY, FPATH_MOVETYPE moveType) = 0;

	/// As route(), but does not return until the route is resolved. Loading an
	/// old-format save uses this, because a droid saved partway through a route
	/// needs a path before play resumes. Drives the droid's movement status as
	/// needed to force the resolve. On return sMove holds the path and the
	/// caller decides the droid's final status from the returned value.
	virtual FPATH_RETVAL routeSynchronous(DROID *psDroid, const WorldMapState& mapState,
	                                      SDWORD targetX, SDWORD targetY, FPATH_MOVETYPE moveType) = 0;

	/// Drops any queued job and pending result for one droid.
	virtual void removeDroidData(int droidID) = 0;
};

/// The backend this game is running. All clients must be running the same one.
IPathfindingBackend& fpathActiveBackend();
