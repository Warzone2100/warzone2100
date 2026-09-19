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

#include "lib/framework/frame.h"

#include "perfcounters.h"

#if defined(WZ_PERF_COUNTERS)

#include <atomic>
#include <cstdio>

namespace perf
{
bool g_enabled = false;

/// Atomic because fpathAStarRoute and fpathSetBlockingMap run on the pathfinding worker.
static std::atomic<uint64_t> acc[NUM_COUNTERS];
static FILE *outFile = nullptr;

static const char *counterNames[] =
{
	"T_gameStateUpdate", "T_processVisibility", "T_visibleObject", "T_checkFireLine", "T_gridQuery",
	"T_droidUpdate", "T_moveUpdateDroid", "T_moveCalcDroidSlide", "T_moveCheckSquished", "T_steeringScan",
	"T_aiBestNearestTarget", "T_aiChooseTargetStruct", "T_aiChooseTargetDroid", "T_structureUpdate", "T_projUpdateAll", "T_projInFlight",
	"T_checkForDamagedStruct",
	"T_fpathRoute", "T_fpathRouteWait", "T_fpathSetBlockingMap", "T_fpathAStarRoute", "T_corridorGateUpdate",
	"T_congestionOverlay", "T_objmemUpdate", "T_checkReferences",
	"T_displayDynamicObjects", "T_display3DProjectiles", "T_processEffects", "T_bucketSort",
	"T_renderEffects",
	"C_gridQueries", "C_gridResultsReturned", "C_rayCasts", "C_fireLineTraces", "C_pathJobsQueued", "C_pathPollsNotReady",
	"C_pathContextsAllocated", "C_blockingMapsBuilt", "C_effectsAlive", "C_projectilesAlive", "C_droidsAlive", "C_structuresAlive",
	"C_framesDrawn"
};
static_assert(sizeof(counterNames) / sizeof(counterNames[0]) == NUM_COUNTERS, "counterNames is out of step with the Counter enum");

void add(Counter c, uint64_t v)
{
	if (!g_enabled)
	{
		return;
	}
	acc[c].fetch_add(v, std::memory_order_relaxed);
}

bool open(const char *path)
{
	outFile = fopen(path, "w");
	if (outFile == nullptr)
	{
		debug(LOG_ERROR, "Failed to open performance counter file for writing: %s", path);
		return false;
	}
	fprintf(outFile, "gameTime");
	for (unsigned c = 0; c < NUM_COUNTERS; ++c)
	{
		fprintf(outFile, ",%s", counterNames[c]);
		acc[c].store(0, std::memory_order_relaxed);
	}
	fprintf(outFile, "\n");
	g_enabled = true;
	debug(LOG_INFO, "Writing per-tick performance counters to: %s", path);
	return true;
}

void endOfTick(uint32_t atGameTime)
{
	if (!g_enabled || outFile == nullptr)
	{
		return;
	}
	fprintf(outFile, "%u", atGameTime);
	for (unsigned c = 0; c < NUM_COUNTERS; ++c)
	{
		fprintf(outFile, ",%" PRIu64, acc[c].exchange(0, std::memory_order_relaxed));
	}
	fprintf(outFile, "\n");
}

void endOfFrame()
{
	add(C_framesDrawn, 1);
}

void close()
{
	g_enabled = false;
	if (outFile != nullptr)
	{
		fclose(outFile);
		outFile = nullptr;
	}
}
}

#else // !defined(WZ_PERF_COUNTERS)

namespace perf
{
bool open(const char *)
{
	debug(LOG_ERROR, "This build has no performance counters. Configure with -DWZ_PERF_COUNTERS=ON.");
	return false;
}
}

#endif // defined(WZ_PERF_COUNTERS)
