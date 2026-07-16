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
 *  Pathfinding benchmark. See pathbench.h.
 */

#include "pathbench.h"

#include "lib/framework/frame.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/wzapp.h"

#include "astar.h"
#include "droid.h"
#include "fpath.h"
#include "game_world.h"
#include "map.h"
#include "structure.h"

#include <algorithm>
#include <chrono>
#include <vector>

#include <nlohmann/json.hpp>

namespace
{

/// Tiles laid out by data/mp/multiplay/maps/PathBench/game.js. Kept in step with
/// that file by hand, so a change to the terrain means a change here.
const int BAND_LAST_Y = 178;      // top row of the final band
const int CHAMBER_INSIDE = 220;   // a tile sealed inside the ring

/// How many requests a batch case issues. Large enough that per-call overhead
/// averages out, small enough that the run stays short.
const uint32_t BATCH_SIZE = 32;

/// Ticks to let the game settle before measuring, so map load and the first
/// visibility passes are not caught in the timings.
const uint32_t WARMUP_TICKS = 20;

/// What a case asks the planner to do. Cases differ in which part of the search
/// they stress, not merely in size.
enum class Shape
{
	Single,     ///< one request, timed on a context that has never seen it
	SharedDest, ///< a batch to one destination, so the context is reused
	SplitDest,  ///< a batch to distinct destinations, so a context is built each time
};

struct BenchCase
{
	const char *name;
	Shape shape;
	int origTileX, origTileY;
	int destTileX, destTileY;
	const char *what;
};

const BenchCase benchCases[] =
{
	// Corner to corner through every band of the serpentine. The gaps alternate
	// sides, so this crosses the map once per band rather than running straight.
	{ "long", Shape::Single, 4, 4, 128, BAND_LAST_Y + 12,
	  "long route across the serpentine" },
	// Both ends in open ground. Resolves in few expansions, so what it measures
	// is the cost of making a request at all.
	{ "short", Shape::Single, 100, 200, 112, 208,
	  "short route in the open field" },
	// The destination is real ground, but it is sealed inside the chamber, so
	// the search has to exhaust everything reachable before it can answer. This
	// is the worst case and the one a search cap would change.
	{ "unreachable", Shape::Single, 4, 4, CHAMBER_INSIDE, CHAMBER_INSIDE,
	  "goal sealed inside the chamber" },
	// A cohort heading to one place, which is the case the context cache
	// exists to serve. The first request pays for the search and the rest
	// should be close to free.
	{ "reuse", Shape::SharedDest, 8, 4, 128, BAND_LAST_Y + 12,
	  "batch sharing one destination" },
	// The same batch with the destinations spread out, so every request
	// builds its own context. This is the shape a cost function keyed per
	// droid would force on every search, and the gap between it and reuse is
	// what that would cost.
	{ "distinct", Shape::SplitDest, 8, 4, 8, BAND_LAST_Y + 12,
	  "batch with a destination each" },
};

struct CaseResult
{
	std::string name;
	uint32_t requests = 0;
	uint64_t nodesExpanded = 0;   ///< exact, from one untimed pass
	uint64_t medianMicros = 0;
	uint64_t minMicros = 0;
	uint64_t maxMicros = 0;
	uint32_t routesFound = 0;     ///< ASR_OK
	uint32_t routesNearest = 0;   ///< ASR_NEAREST
	uint32_t routesFailed = 0;    ///< ASR_FAILED
};

bool selected = false;
std::string activeTestConfig;
uint32_t repeats = 5;
uint32_t tickCount = 0;
bool finished = false;

std::vector<CaseResult> results;
uint64_t queuedDrainMicros = 0;
uint32_t queuedDrainTicks = 0;
uint32_t queuedRequests = 0;
std::chrono::steady_clock::time_point queuedStart;
std::vector<DROID *> queuedDroids;
bool queuedIssued = false;

int64_t worldOf(int tile)
{
	return world_coord(tile) + TILE_UNITS / 2;
}

/// Builds one request. Mirrors what fpathDroidRoute settles on for a plain move
/// order, so the search sees the same kind of job the game gives it.
PATHJOB makeJob(int origTileX, int origTileY, int destTileX, int destTileY, uint32_t id)
{
	PATHJOB job;
	job.origX = static_cast<int>(worldOf(origTileX));
	job.origY = static_cast<int>(worldOf(origTileY));
	job.destX = static_cast<int>(worldOf(destTileX));
	job.destY = static_cast<int>(worldOf(destTileY));
	job.dstStructure = getStructureBounds((BASE_OBJECT *)nullptr);
	job.droidID = id;
	job.droidType = DROID_WEAPON;
	job.propulsion = PROPULSION_TYPE_WHEELED;
	job.moveType = FMT_MOVE;
	job.owner = 0;
	job.acceptNearest = true;
	job.deleted = false;
	fpathSetBlockingMap(&job);   // main thread, as the planner requires
	return job;
}

/// Destinations for one case. A batch sharing a destination reuses the context,
/// a batch splitting them does not.
std::vector<Vector2i> destinationsFor(const BenchCase &c, uint32_t count)
{
	std::vector<Vector2i> dests;
	for (uint32_t i = 0; i < count; ++i)
	{
		if (c.shape == Shape::SplitDest)
		{
			// Spread along the row below the serpentine, far enough apart that
			// no two share a destination tile.
			dests.push_back(Vector2i(c.destTileX + static_cast<int>(i) * 7, c.destTileY));
		}
		else
		{
			dests.push_back(Vector2i(c.destTileX, c.destTileY));
		}
	}
	return dests;
}

/// Origins for one case, clustered so a batch looks like a group of units
/// starting together.
std::vector<Vector2i> originsFor(const BenchCase &c, uint32_t count)
{
	std::vector<Vector2i> origins;
	for (uint32_t i = 0; i < count; ++i)
	{
		origins.push_back(Vector2i(c.origTileX + static_cast<int>(i % 4),
		                           c.origTileY + static_cast<int>(i / 4)));
	}
	return origins;
}

/// Runs one case once against a context of its own, returning microseconds.
/// A fresh context per pass is what makes a Single case measure a cold search
/// every time rather than a cache hit after the first.
uint64_t runPass(const BenchCase &c, uint32_t count, CaseResult *tally)
{
	const std::vector<Vector2i> origins = originsFor(c, count);
	const std::vector<Vector2i> dests = destinationsFor(c, count);
	auto ctx = makeFPathExecuteContext();

	const auto started = std::chrono::steady_clock::now();
	for (uint32_t i = 0; i < count; ++i)
	{
		PATHJOB job = makeJob(origins[i].x, origins[i].y, dests[i].x, dests[i].y, i + 1);
		MOVE_CONTROL move;
		const ASR_RETVAL r = fpathAStarRoute(ctx, &move, &job);
		if (tally != nullptr)
		{
			switch (r)
			{
			case ASR_OK:      ++tally->routesFound;   break;
			case ASR_NEAREST: ++tally->routesNearest; break;
			case ASR_FAILED:  ++tally->routesFailed;  break;
			}
		}
	}
	const auto ended = std::chrono::steady_clock::now();
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(ended - started).count());
}

/// Times every case on the main thread. This is the measurement to compare.
void runDirect()
{
	for (const auto &c : benchCases)
	{
		const uint32_t count = (c.shape == Shape::Single) ? 1 : BATCH_SIZE;
		CaseResult result;
		result.name = c.name;
		result.requests = count;

		// Count nodes on a pass of its own. Counting and timing the same pass
		// would put the counter's cost inside the number it is there to explain.
		uint64_t nodes = 0;
#if WZ_PATHFINDING_INSTRUMENTATION
		g_pathNodesExpanded = &nodes;
		runPass(c, count, nullptr);
		g_pathNodesExpanded = nullptr;
#endif
		result.nodesExpanded = nodes;

		std::vector<uint64_t> timings;
		for (uint32_t r = 0; r < repeats; ++r)
		{
			// Only the first timed pass tallies outcomes, since the rest repeat it.
			timings.push_back(runPass(c, count, (r == 0) ? &result : nullptr));
		}
		std::sort(timings.begin(), timings.end());
		result.medianMicros = timings[timings.size() / 2];
		result.minMicros = timings.front();
		result.maxMicros = timings.back();
		results.push_back(result);
	}
}

/// Pushes the long route through the worker pool for every droid the scenario
/// spawned, so the drain covers dispatch and cross-tick polling. The node
/// counter stays off here: several threads search at once and would race on it.
void issueQueued()
{
	const BenchCase &c = benchCases[0];
	queuedDroids.clear();
	for (DROID *psDroid : gameWorld.objects.droids[0])
	{
		queuedDroids.push_back(psDroid);
	}
	queuedStart = std::chrono::steady_clock::now();
	for (DROID *psDroid : queuedDroids)
	{
		fpathDroidRoute(psDroid, gameWorld.map, static_cast<SDWORD>(worldOf(c.destTileX)),
		                static_cast<SDWORD>(worldOf(c.destTileY)), FMT_MOVE);
		// fpathRoute only polls a droid it believes is waiting, and setting that
		// is normally the mover's job rather than the planner's.
		psDroid->sMove.Status = MOVEWAITROUTE;
	}
	queuedRequests = static_cast<uint32_t>(queuedDroids.size());
	queuedIssued = true;
}

/// Polls the outstanding requests. Returns true once none are left.
bool pollQueued()
{
	const BenchCase &c = benchCases[0];
	uint32_t outstanding = 0;
	for (DROID *psDroid : queuedDroids)
	{
		if (psDroid->sMove.Status != MOVEWAITROUTE)
		{
			continue;
		}
		const FPATH_RETVAL r = fpathDroidRoute(psDroid, gameWorld.map,
		                                       static_cast<SDWORD>(worldOf(c.destTileX)),
		                                       static_cast<SDWORD>(worldOf(c.destTileY)), FMT_MOVE);
		if (r == FPR_WAIT)
		{
			++outstanding;
		}
	}
	++queuedDrainTicks;
	if (outstanding == 0)
	{
		const auto ended = std::chrono::steady_clock::now();
		queuedDrainMicros = static_cast<uint64_t>(
			std::chrono::duration_cast<std::chrono::microseconds>(ended - queuedStart).count());
		return true;
	}
	return false;
}

void writeScorecard()
{
	nlohmann::json card = nlohmann::json::object();
	card["repeats"] = repeats;
	card["batchSize"] = BATCH_SIZE;

	nlohmann::json cases = nlohmann::json::object();
	for (const auto &r : results)
	{
		nlohmann::json entry = nlohmann::json::object();
		entry["requests"] = r.requests;
		entry["nodesExpanded"] = r.nodesExpanded;
		entry["micros_median"] = r.medianMicros;
		entry["micros_min"] = r.minMicros;
		entry["micros_max"] = r.maxMicros;
		entry["routesFound"] = r.routesFound;
		entry["routesNearest"] = r.routesNearest;
		entry["routesFailed"] = r.routesFailed;
		cases[r.name] = entry;
	}
	card["direct"] = cases;

	nlohmann::json queued = nlohmann::json::object();
	queued["requests"] = queuedRequests;
	queued["drainTicks"] = queuedDrainTicks;
	queued["drainMicros"] = queuedDrainMicros;
	card["queued"] = queued;

	const std::string dumped = card.dump(4);
	fprintf(stdout, "%s\n", dumped.c_str());
	fflush(stdout);

	PHYSFS_file *fileHandle = PHYSFS_openWrite("pathbench.json");
	if (fileHandle == nullptr)
	{
		debug(LOG_ERROR, "Failed to open pathbench.json for writing: %s", WZ_PHYSFS_getLastError());
		return;
	}
	WZ_PHYSFS_writeBytes(fileHandle, dumped.c_str(), static_cast<PHYSFS_uint32>(dumped.size()));
	PHYSFS_close(fileHandle);
	debug(LOG_INFO, "pathbench: wrote scorecard to pathbench.json");
}

} // anonymous namespace

bool pathBenchSelect(const std::string &name)
{
	if (name != "default")
	{
		return false;
	}
	selected = true;
	activeTestConfig = "pathbench.json";
	return true;
}

bool pathBenchActive()
{
	return selected;
}

const std::string &pathBenchTestConfig()
{
	return activeTestConfig;
}

uint32_t pathBenchSeed()
{
	return 0x9A7B3C1D;
}

void pathBenchSetRepeats(uint32_t r)
{
	repeats = std::max<uint32_t>(1, r);
}

void pathBenchUpdate()
{
	if (!selected || finished)
	{
		return;
	}
	++tickCount;
	if (tickCount < WARMUP_TICKS)
	{
		return;
	}

	if (!queuedIssued)
	{
		runDirect();
		issueQueued();
		return;
	}
	if (!pollQueued())
	{
		return;
	}

	writeScorecard();
	finished = true;
	wzQuit(0);
}
