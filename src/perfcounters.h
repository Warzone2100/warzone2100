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
 *  Per-tick counters and timers, compiled in by -DWZ_PERF_COUNTERS=ON and enabled at
 *  runtime by --perfcounters=<file>. Unlike WZ_PROFILE_SCOPE they work headless and write
 *  a CSV. They never touch game state.
 */
#pragma once

#include "lib/framework/wzglobal.h" // required for config.h

#include <cstdint>

#if defined(WZ_PERF_COUNTERS)

#include <chrono>

namespace perf
{
enum Counter : unsigned
{
	// nanoseconds accumulated per tick
	T_gameStateUpdate, T_processVisibility, T_visibleObject, T_checkFireLine, T_gridQuery,
	T_droidUpdate, T_moveUpdateDroid, T_moveCalcDroidSlide, T_moveCheckSquished, T_steeringScan,
	T_aiBestNearestTarget, T_aiChooseTargetStruct, T_aiChooseTargetDroid, T_structureUpdate, T_projUpdateAll, T_projInFlight,
	T_checkForDamagedStruct,
	T_fpathRoute, T_fpathRouteWait, T_fpathSetBlockingMap, T_fpathAStarRoute, T_corridorGateUpdate,
	T_congestionOverlay, T_objmemUpdate, T_checkReferences,
	// nanoseconds accumulated on the render side, over whatever frames fall between two CSV lines
	T_displayDynamicObjects, T_display3DProjectiles, T_processEffects, T_bucketSort,
	T_renderEffects,
	// counts
	C_gridQueries, C_gridResultsReturned, C_rayCasts, C_fireLineTraces, C_pathJobsQueued, C_pathPollsNotReady,
	C_pathContextsAllocated, C_blockingMapsBuilt, C_effectsAlive, C_projectilesAlive, C_droidsAlive, C_structuresAlive,
	C_framesDrawn,
	NUM_COUNTERS
};

extern bool g_enabled;

void add(Counter c, uint64_t v);      ///< thread-safe
void endOfTick(uint32_t atGameTime);  ///< write one CSV line and reset every accumulator
void endOfFrame();
bool open(const char *path);
void close();

/// Function-level scoped timer. Not for a per-neighbour loop body - the clock read costs about 20 ns.
struct Scope
{
	explicit Scope(Counter c) : c_(c), on_(g_enabled)
	{
		if (on_)
		{
			t0_ = std::chrono::steady_clock::now();
		}
	}
	~Scope()
	{
		if (on_)
		{
			add(c_, (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0_).count());
		}
	}
	Scope(const Scope &) = delete;
	Scope &operator=(const Scope &) = delete;

private:
	Counter c_;
	bool on_;
	std::chrono::steady_clock::time_point t0_;
};
}

#define WZ_PERF_SCOPE(c) perf::Scope perfScope_##c(perf::c)
#define WZ_PERF_COUNT(c, v) do { if (perf::g_enabled) { perf::add(perf::c, (uint64_t)(v)); } } while (0)

#else // !defined(WZ_PERF_COUNTERS)

namespace perf
{
constexpr bool g_enabled = false;

bool open(const char *path);  ///< reports that this build has no counters and fails
inline void endOfTick(uint32_t) {}
inline void endOfFrame() {}
inline void close() {}
}

#define WZ_PERF_SCOPE(c)
#define WZ_PERF_COUNT(c, v)

#endif // defined(WZ_PERF_COUNTERS)
