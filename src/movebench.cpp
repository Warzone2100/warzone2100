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
 *  Deterministic movement benchmark runner. See movebench.h.
 */

#include "movebench.h"

#include "lib/framework/frame.h"
#include "lib/framework/crc.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/wzapp.h"
#include "lib/gamelib/gtime.h"

#include "droid.h"
#include "game_world.h"
#include "map.h"
#include "order.h"

#include <algorithm>
#include <map>
#include <nonstd/optional.hpp>
using nonstd::optional;
#include <vector>

#include <nlohmann/json.hpp>

MovementMetrics *g_moveMetrics = nullptr;

namespace
{

/// A scenario is a tests/ config plus the fixed budget and seed it runs under.
struct BenchScenario
{
	const char *name;
	const char *testConfig;   ///< file under tests/, launched as if by --skirmish
	uint32_t    tickBudget;   ///< hard cap, in game state updates
	uint32_t    seed;         ///< fixed seed for the synchronised RNG
	/// Whether to stop as soon as everything ordered has arrived. Scenarios that
	/// re-order their units mid-run must set this false, since they would
	/// otherwise end at the first moment a leg happens to be complete.
	bool        endOnCompletion = true;
};

/// Tracks one droid from the tick it was first seen under a move order.
struct TrackedDroid
{
	uint32_t id = 0;
	Vector2i goal = Vector2i(0, 0);
	Vector2i lastPos = Vector2i(0, 0);
	uint32_t startTick = 0;
	uint32_t startDistTiles = 0;   ///< straight-line distance to the goal when this leg began, in tiles
	uint32_t arrivalTick = 0;
	bool     arrived = false;
};

const BenchScenario scenarios[] =
{
	// Tracked units through a sloped cliff corridor.
	{ "counterflow_tracked", "movebench_counterflow_tracked.json", 5000, 0x5EEDBEEF },
	// Same map and block, one direction only. Control for the above.
	{ "oneway_tracked",      "movebench_oneway_tracked.json",      1200, 0x5EEDBEEF },
	// Same conflict on the person movement model, at higher density.
	{ "counterflow_cyborg",  "movebench_counterflow_cyborg.json",  5000, 0x5EEDBEEF },
	// Congestion. One destination for everyone, and a column past parked allies.
	{ "blob",                "movebench_blob.json",                3000, 0x5EEDBEEF },
	{ "parking",             "movebench_parking.json",              900, 0x5EEDBEEF },
	// The same conflict at other gap widths. Tracked units do not start
	// resolving it until 4 tiles, so w3 sits just below the threshold and w4
	// has room to move in both directions.
	{ "counterflow_w1",      "movebench_counterflow_w1.json",      5000, 0x5EEDBEEF },
	// Real-map acceptance pair. Opposing tank and cyborg flows with mid-transit
	// re-orders, through Sk-Mountain's chained winding passes and Sk-Rush's
	// centre corridor with its mid-corridor departure turn. Between them they
	// cover chains, pockets, pinch threading, S-bends and lane handedness, and
	// they test complementary halves: what fixes one has repeatedly broken the
	// other, so neither alone is evidence.
	{ "mountain_chain",      "movebench_mountain_chain.json",      4000, 0x5EEDBEEF, false },
	// As mountain_chain with the cyborg destination on the near side of the
	// tank intake, crossing the exit stream against the entering queue.
	{ "mountain_chain_cross", "movebench_mountain_chain_cross.json", 4000, 0x5EEDBEEF, false },
	{ "rush_turn",           "movebench_rush_turn.json",           4000, 0x5EEDBEEF, false },
	// Mass corner head-on: two full clusters swap across Sk-Rush's north-west,
	// both flows cutting the same open corner from opposite directions at a
	// scale where the corner scrum dominates the whole run.
	{ "rush_corner",         "movebench_rush_corner.json",         4000, 0x5EEDBEEF, false },
	// The same head-on with the corridors stripped: the Sk-Rush corner mass
	// alone on open ground, so the corner scrum is isolated from every piece
	// of corridor machinery.
	{ "open_corner",         "movebench_open_corner.json",         4000, 0x5EEDBEEF, false },
	{ "counterflow_w3",      "movebench_counterflow_w3.json",      5000, 0x5EEDBEEF },
	{ "counterflow_w4",      "movebench_counterflow_w4.json",      5000, 0x5EEDBEEF },
	{ "shift0",             "movebench_shift0.json",             1200, 0x5EEDBEEF },
	{ "shift1",             "movebench_shift1.json",             1200, 0x5EEDBEEF },
	{ "shift2",             "movebench_shift2.json",             1200, 0x5EEDBEEF },
	// Two equal routes through the wall, so congestion has somewhere to go.
	{ "tworoute",            "movebench_tworoute.json",            5000, 0x5EEDBEEF },
	{ "counterflow_w6",      "movebench_counterflow_w6.json",      1200, 0x5EEDBEEF },
	{ "counterflow_w8",      "movebench_counterflow_w8.json",      1200, 0x5EEDBEEF },
	// Opposing flows meeting the pass on different diagonals.
	{ "crossing",            "movebench_crossing.json",            5000, 0x5EEDBEEF },
	{ "separating",          "movebench_separating.json",          5000, 0x5EEDBEEF },
	// Same-direction cluster rounding a corner, uniform and mixed speed.
	{ "corner",              "movebench_corner.json",              3000, 0x5EEDBEEF },
	{ "corner_mixed",        "movebench_corner_mixed.json",        3000, 0x5EEDBEEF },
	// Guards against over-yielding and against eroding enemy blocking.
	{ "openfield",           "movebench_openfield.json",            900, 0x5EEDBEEF },
	// Re-orders its units repeatedly, so it must run its whole budget.
	{ "strafe",              "movebench_strafe.json",              1200, 0x5EEDBEEF, false },
	{ "enemyblock",          "movebench_enemyblock.json",           900, 0x5EEDBEEF },
	{ "enemyblock_press",    "movebench_enemyblock_press.json",      900, 0x5EEDBEEF },
};

optional<uint32_t> seedOverride;
uint32_t arrangementIndex = 0;
bool watching = false;

const BenchScenario *activeScenario = nullptr;
MovementMetrics      metrics;
std::string          activeTestConfig;

uint32_t tickCount = 0;
std::map<uint32_t, TrackedDroid> tracked;   ///< keyed by droid id, ordered for determinism
std::vector<uint32_t> arrivalTicks;         ///< every arrival, including repeat legs
std::vector<uint32_t> arrivalDistTiles;     ///< leg distance for each arrival, parallel to arrivalTicks
std::vector<uint32_t> tickPeakDensity;      ///< per tick, the size of the largest cluster of tracked droids
bool     runFinished = false;

/// Two tracked droids within this of each other count as packed together. At a
/// tile and a half it catches touching neighbours without counting a whole open
/// field, so a tight blob reads high and two clean lanes read low.
const int32_t DENSITY_RADIUS = 3 * TILE_UNITS / 2;

/// A droid counts as arrived once it is within this of its ordered destination.
/// Units settle next to a goal tile rather than on it when it is taken, so this
/// allows a tile and a half - the same slack the movement code itself uses when
/// deciding a droid is near enough to its destination.
const int32_t ARRIVE_TOLERANCE = 3 * TILE_UNITS / 2;

const BenchScenario *findScenario(const std::string &name)
{
	for (const auto &s : scenarios)
	{
		if (name == s.name)
		{
			return &s;
		}
	}
	return nullptr;
}

/// Picks up any droid that has been given a move order, and notes when it arrives.
void sampleDroids()
{
	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		for (const DROID *psDroid : gameWorld.objects.droids[player])
		{
			auto it = tracked.find(psDroid->id);
			if (it == tracked.end())
			{
				if (psDroid->order.type != DORDER_MOVE)
				{
					continue;
				}
				TrackedDroid t;
				t.id = psDroid->id;
				t.goal = psDroid->order.pos;
				t.lastPos = psDroid->pos.xy();
				t.startTick = tickCount;
				t.startDistTiles = static_cast<uint32_t>(iHypot(psDroid->order.pos - psDroid->pos.xy()) / TILE_UNITS);
				tracked[psDroid->id] = t;
				continue;
			}

			TrackedDroid &t = it->second;
			t.lastPos = psDroid->pos.xy();

			// A fresh move order to somewhere else starts a new leg, so the
			// droid is timed again from here. Scenarios that strafe back and
			// forth get a per-leg time this way rather than only a first-leg one.
			if (psDroid->order.type == DORDER_MOVE && psDroid->order.pos != t.goal)
			{
				t.goal = psDroid->order.pos;
				t.startTick = tickCount;
				t.startDistTiles = static_cast<uint32_t>(iHypot(psDroid->order.pos - psDroid->pos.xy()) / TILE_UNITS);
				t.arrived = false;
			}

			if (t.arrived)
			{
				continue;
			}
			// Verify by position rather than by movement state: arrival and the
			// blocked give-up both funnel through MOVETURN into MOVEINACTIVE, so
			// the movement state alone cannot tell them apart.
			const Vector2i delta = psDroid->pos.xy() - t.goal;
			if (dot(delta, delta) <= ARRIVE_TOLERANCE * ARRIVE_TOLERANCE)
			{
				t.arrived = true;
				t.arrivalTick = tickCount - t.startTick;
				arrivalTicks.push_back(t.arrivalTick);
				arrivalDistTiles.push_back(t.startDistTiles);
			}
		}
	}
}

// Records how tightly the tracked droids are packed this tick, as the size of
// the largest cluster within DENSITY_RADIUS of any one of them. A slow untangle
// keeps this high for a long time, so the sustained value tells apart a scenario
// that clears quickly from one that jams into a blob and only slowly resolves.
void sampleDensity()
{
	uint32_t peak = 0;
	for (const auto &a : tracked)
	{
		uint32_t neighbours = 0;
		for (const auto &b : tracked)
		{
			if (a.first == b.first)
			{
				continue;
			}
			const Vector2i d = a.second.lastPos - b.second.lastPos;
			if (dot(d, d) <= DENSITY_RADIUS * DENSITY_RADIUS)
			{
				++neighbours;
			}
		}
		peak = std::max(peak, neighbours);
	}
	tickPeakDensity.push_back(peak);
}

/// CRC of every live droid's final position, taken in id order.
uint32_t finalPositionsCrc()
{
	std::vector<const DROID *> droids;
	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		for (const DROID *psDroid : gameWorld.objects.droids[player])
		{
			droids.push_back(psDroid);
		}
	}
	std::sort(droids.begin(), droids.end(), [](const DROID *a, const DROID *b) {
		return a->id < b->id;
	});

	uint32_t crc = 0;
	for (const DROID *psDroid : droids)
	{
		const Vector2i p = psDroid->pos.xy();
		crc = crcSumVector2i(crc, &p, 1);
	}
	return crc;
}

/// Mean distance from the tracked units' centroid, in tiles. Says whether a
/// block stayed coherent or got smeared.
uint32_t formationSpreadTiles()
{
	if (tracked.empty())
	{
		return 0;
	}
	int64_t sumX = 0;
	int64_t sumY = 0;
	for (const auto &entry : tracked)
	{
		sumX += entry.second.lastPos.x;
		sumY += entry.second.lastPos.y;
	}
	const Vector2i centroid(static_cast<int32_t>(sumX / static_cast<int64_t>(tracked.size())),
	                        static_cast<int32_t>(sumY / static_cast<int64_t>(tracked.size())));

	int64_t sumDist = 0;
	for (const auto &entry : tracked)
	{
		sumDist += iHypot(entry.second.lastPos - centroid);
	}
	return static_cast<uint32_t>(sumDist / static_cast<int64_t>(tracked.size()) / TILE_UNITS);
}

uint32_t percentile(std::vector<uint32_t> &sorted, unsigned pct)
{
	if (sorted.empty())
	{
		return 0;
	}
	const size_t idx = std::min(sorted.size() - 1, (sorted.size() * pct) / 100);
	return sorted[idx];
}

void writeScorecard(bool completed)
{
	std::vector<uint32_t> arrivals = arrivalTicks;
	std::vector<uint32_t> remaining;   // how far the stragglers still are, in tiles
	uint32_t arrivedNow = 0;
	for (const auto &entry : tracked)
	{
		if (entry.second.arrived)
		{
			++arrivedNow;
		}
		else
		{
			const Vector2i delta = entry.second.lastPos - entry.second.goal;
			remaining.push_back(static_cast<uint32_t>(iHypot(delta) / TILE_UNITS));
		}
	}
	std::sort(arrivals.begin(), arrivals.end());
	std::sort(remaining.begin(), remaining.end());

	nlohmann::json card = nlohmann::json::object();
	card["scenario"] = activeScenario->name;
	card["seed"] = movementBenchSeed();
	card["arrangement"] = arrangementIndex;
	card["ticks"] = tickCount;
	card["tickBudget"] = activeScenario->tickBudget;
	card["completed"] = completed;
	card["timedOut"] = !completed;
	card["unitsOrdered"] = static_cast<uint32_t>(tracked.size());
	card["unitsArrived"] = arrivedNow;
	card["formationSpreadTiles"] = formationSpreadTiles();
	card["arrival_p50"] = percentile(arrivals, 50);
	card["arrival_p95"] = percentile(arrivals, 95);
	// The same arrivals in seconds at normal play speed, since a game update is
	// a tenth of a second. This is the number that reads as how long a player
	// waits, where the update count on its own hides a multi-minute crawl inside
	// a budget measured in thousands of updates.
	card["arrival_p50_s"] = percentile(arrivals, 50) / static_cast<double>(GAME_UPDATES_PER_SEC);
	card["arrival_p95_s"] = percentile(arrivals, 95) / static_cast<double>(GAME_UPDATES_PER_SEC);
	// Arrival normalized by the straight-line distance of each leg, in seconds
	// per tile, so scenarios of different lengths compare on one scale. Free
	// travel sits near the open-field value and contention shows up as a multiple
	// of it, which the raw arrival time cannot separate from a longer route.
	std::vector<uint32_t> perTileCs;   // centiseconds of arrival per tile of leg distance
	for (size_t i = 0; i < arrivalTicks.size(); ++i)
	{
		if (arrivalDistTiles[i] > 0)
		{
			perTileCs.push_back(arrivalTicks[i] * 10 / arrivalDistTiles[i]);
		}
	}
	std::sort(perTileCs.begin(), perTileCs.end());
	card["secPerTile_p50"] = percentile(perTileCs, 50) / 100.0;
	card["secPerTile_p95"] = percentile(perTileCs, 95) / 100.0;
	std::vector<uint32_t> density = tickPeakDensity;
	std::sort(density.begin(), density.end());
	card["peakDensity"] = density.empty() ? 0 : density.back();
	card["density_p95"] = percentile(density, 95);
	// Distance still to go for whatever did not arrive, in tiles. Separates
	// "jammed at the pass" from "never left the start".
	card["stuckRemainingTiles_p50"] = percentile(remaining, 50);
	card["stuckRemainingTiles_p95"] = percentile(remaining, 95);
	card["bumps"] = metrics.bumps;
	card["bumpsRepeat"] = metrics.bumpsRepeat;
	card["hardStops"] = metrics.hardStops;
	card["giveUps"] = metrics.giveUps;
	card["repaths"] = metrics.repaths;
	card["finalPositionsCrc"] = finalPositionsCrc();

	const std::string dumped = card.dump(4);

	// stdout is what CI reads. The file is for diffing against a baseline.
	fprintf(stdout, "%s\n", dumped.c_str());
	fflush(stdout);

	const std::string outPath = std::string("movebench_") + activeScenario->name + ".json";
	PHYSFS_file *fileHandle = PHYSFS_openWrite(outPath.c_str());
	if (fileHandle == nullptr)
	{
		debug(LOG_ERROR, "Failed to open %s for writing: %s", outPath.c_str(), WZ_PHYSFS_getLastError());
		return;
	}
	WZ_PHYSFS_writeBytes(fileHandle, dumped.c_str(), static_cast<PHYSFS_uint32>(dumped.size()));
	PHYSFS_close(fileHandle);
	debug(LOG_INFO, "movebench: wrote scorecard to %s", outPath.c_str());
}

} // anonymous namespace

void movementBenchSetWatching()
{
	watching = true;
}

bool movementBenchSelectScenario(const std::string &name)
{
	const BenchScenario *scenario = findScenario(name);
	if (scenario == nullptr)
	{
		return false;
	}
	activeScenario = scenario;
	activeTestConfig = scenario->testConfig;
	g_moveMetrics = &metrics;
	// Watched and headless runs of a scenario must be the same simulation, so
	// the order-queue latency negotiation goes wall-clock-free for the bench.
	gameTimeSetDeterministicLatency(true);
	return true;
}

bool movementBenchActive()
{
	return activeScenario != nullptr;
}

bool movementBenchWatching()
{
	return watching;
}

const std::string &movementBenchTestConfig()
{
	return activeTestConfig;
}

uint32_t movementBenchSeed()
{
	ASSERT_OR_RETURN(0, activeScenario != nullptr, "No active movement bench scenario");
	return seedOverride.value_or(activeScenario->seed);
}

void movementBenchSetSeed(uint32_t seed)
{
	seedOverride = seed;
}

void movementBenchSetArrangement(uint32_t index)
{
	arrangementIndex = index;
}

uint32_t movementBenchArrangement()
{
	return arrangementIndex;
}

void movementBenchUpdate()
{
	if (activeScenario == nullptr || runFinished)
	{
		return;
	}

	++tickCount;
	sampleDroids();
	sampleDensity();

	// Finish early once everything ordered has arrived, so easy scenarios stay
	// quick, but always stop at the budget so a jammed one still terminates.
	const bool completed = activeScenario->endOnCompletion
	                       && !tracked.empty()
	                       && std::all_of(tracked.begin(), tracked.end(),
	                                      [](const std::pair<const uint32_t, TrackedDroid> &e) {
	                                          return e.second.arrived;
	                                      });
	if (!completed && tickCount < activeScenario->tickBudget)
	{
		return;
	}

	runFinished = true;
	writeScorecard(completed);
	if (!watching)
	{
		wzQuit(0);
	}
	// When watching, the scorecard is written at the budget and the game is
	// left running so the scenario can be followed past it.
}
