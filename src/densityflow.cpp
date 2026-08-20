/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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
/** @file
 *  Per-player density/flow grid: build + A* cost integration (patch #1) and
 *  the reroute-trigger scan (patch #2). See densityflow.h for the design
 *  summary; see density-flow-pathfinding-spec.md (design doc) for full
 *  rationale.
 */

#include "lib/framework/frame.h"
#include "lib/netplay/sync_debug.h"

#include "densityflow.h"
#include "fpath.h"
#include "astar.h"
#include "map.h"
#include "game_world.h"
#include "droid.h"
#include "move.h"

#include <algorithm>
#include <cstdlib>

// ---------------------------------------------------------------------------
// Determinism: everything below uses only integer add/multiply/shift on
// values already present in game state (no exp/sqrt/trig, no floating point).
// The kernel LUT is a baked table of literal integer constants (generated
// offline), same pattern as gradYLookup in astar.cpp. The dot product used to
// gate cost/reroute by flow alignment is left unnormalized (no magnitude
// division). sign + relative magnitude is enough, and normalizing would
// require a sqrt.
// ---------------------------------------------------------------------------

/// Bucketed squared-distance -> weight (0..256) kernel, baked offline.
/// Index = min(bucket, KERNEL_LUT_SIZE-1), bucket = squaredDist >> KERNEL_BUCKET_SHIFT.
/// A smooth-ish falloff from full weight at the center tile to ~0 two tiles out;
/// shape is not build order dependent and involves no runtime transcendental calls.
static constexpr int KERNEL_LUT_SIZE = 24;
static constexpr int KERNEL_BUCKET_SHIFT = 10; // squared world-units -> bucket
static const int32_t kernelLUT[KERNEL_LUT_SIZE] =
{
	256, 240, 220, 198, 176, 154, 133, 114,
	 97,  81,  68,  56,  45,  36,  29,  22,
	 17,  13,   9,   6,   4,   3,   1,   0
};

static inline int32_t kernelWeight(int32_t dxWorld, int32_t dyWorld)
{
	int64_t sq = (int64_t)dxWorld * dxWorld + (int64_t)dyWorld * dyWorld;
	int64_t bucket = sq >> KERNEL_BUCKET_SHIFT;
	if (bucket >= KERNEL_LUT_SIZE)
	{
		return 0;
	}
	return kernelLUT[(int)bucket];
}

/// Per-propulsion-class weight (--> density) table.
/// Lower = more nimble = costs less to route (shuffle) through.
/// Mutable and exposed via densityflow.h so the debug menu's
/// "Pathfinding" tab can edit these live
int32_t densityFlowClassWeight[DFLOW_WEIGHT_CLASS_COUNT] =
{
	/* DFLOW_WHEELED     */ 10,
	/* DFLOW_HALF_TRACKED*/ 12,
	/* DFLOW_TRACKED     */ 14,
	/* DFLOW_HOVER       */  6,
	/* DFLOW_LEGGED      */  8,
	/* DFLOW_LANDED_VTOL */ 10,
	/* DFLOW_PROPELLOR   */ 10, // unused?
};

const char *densityFlowClassWeightName(DFLOW_WEIGHT_CLASS wclass)
{
	switch (wclass)
	{
	case DFLOW_WHEELED:      return "Wheeled";
	case DFLOW_TRACKED:      return "Tracked";
	case DFLOW_LEGGED:       return "Legged";
	case DFLOW_HALF_TRACKED: return "Half-tracked";
	case DFLOW_HOVER:        return "Hover";
	case DFLOW_PROPELLOR:    return "Naval";
	case DFLOW_LANDED_VTOL:  return "Landed VTOL";
	default:                 return "?";
	}
}

static bool classifyPropulsion(const DROID *psDroid, DFLOW_WEIGHT_CLASS &outClass)
{
	if (psDroid->isVtol())
	{
		// Only a *landed* VTOL is a ground obstacle. Flying VTOLs are out of
		// scope entirely (no density influence, no participation).
		if (psDroid->sMove.Status == MOVEINACTIVE)
		{
			outClass = DFLOW_LANDED_VTOL;
			return true;
		}
		return false;
	}

	const PROPULSION_STATS *psStats = psDroid->getPropulsionStats();
	if (psStats == nullptr)
	{
		return false;
	}

	switch (psStats->propulsionType)
	{
	case PROPULSION_TYPE_WHEELED:      outClass = DFLOW_WHEELED;      return true;
	case PROPULSION_TYPE_TRACKED:      outClass = DFLOW_TRACKED;      return true;
	case PROPULSION_TYPE_LEGGED:       outClass = DFLOW_LEGGED;       return true;
	case PROPULSION_TYPE_HALF_TRACKED: outClass = DFLOW_HALF_TRACKED; return true;
	case PROPULSION_TYPE_HOVER:        outClass = DFLOW_HOVER;        return true;
	case PROPULSION_TYPE_PROPELLOR:    outClass = DFLOW_PROPELLOR;    return true;
	case PROPULSION_TYPE_LIFT:
	default:
		return false; // VTOL flight itself: out of scope, no density influence.
	}
}

/// Splat one droid's mass + flow contribution onto its tile and up to 8 neighbors. 
/// Geometric spread is driven by moveObjRadius() + sub-tile offset (pos & TILE_MASK, free via TILE_SHIFT).
/// Class weight is a separate multiplier on total mass;
/// the same per-cell kernel weight is reused for the flow contribution.
static void splatDroid(std::vector<DensityFlowCell> &cells, int width, int height, const DROID *psDroid, DFLOW_WEIGHT_CLASS wclass)
{
	const int32_t radius = moveObjRadius((const BASE_OBJECT *)psDroid);
	const int32_t classWeight = densityFlowClassWeight[wclass];

	const int tileX = map_coord(psDroid->pos.x);
	const int tileY = map_coord(psDroid->pos.y);
	const int32_t subX = psDroid->pos.x & TILE_MASK;
	const int32_t subY = psDroid->pos.y & TILE_MASK;

	// Waypoint delta (not instantaneous velocity/moveDir) is stable across
	// ticks, avoids steering jitter. (0,0) for idle/orderless droids.
	int32_t flowDx = 0, flowDy = 0;
	if (psDroid->sMove.Status != MOVEINACTIVE
	    && psDroid->sMove.pathIndex >= 0
	    && (size_t)psDroid->sMove.pathIndex < psDroid->sMove.asPath.size())
	{
		Vector2i wp = psDroid->sMove.asPath[psDroid->sMove.pathIndex];
		flowDx = wp.x - psDroid->pos.x;
		flowDy = wp.y - psDroid->pos.y;
	}

	for (int oy = -1; oy <= 1; ++oy)
	{
		for (int ox = -1; ox <= 1; ++ox)
		{
			int tx = tileX + ox;
			int ty = tileY + oy;
			if (tx < 0 || ty < 0 || tx >= width || ty >= height)
			{
				continue;
			}

			// World-space offset from the droid's actual position to the
			// center of the candidate tile, using the sub-tile offset so a
			// droid near a tile edge biases weight toward that neighbor.
			int32_t dxWorld = ox * TILE_UNITS - subX + TILE_UNITS / 2;
			int32_t dyWorld = oy * TILE_UNITS - subY + TILE_UNITS / 2;

			int32_t w = kernelWeight(dxWorld, dyWorld);
			if (w == 0)
			{
				continue;
			}

			DensityFlowCell &cell = cells[tx + ty * width];
			// radius scales contribution mildly: bigger hitboxes splat more mass
			cell.mass += (w * classWeight * std::max<int32_t>(1, radius)) >> 8;
			cell.flowX += (w * flowDx) >> 8;
			cell.flowY += (w * flowDy) >> 8;
		}
	}
}

// ---------------------------------------------------------------------------
// Per tick cache of built maps, plus owner specific previous tick history for the
// rerouting trigger logic below. The build trigger mirrors fpathSetBlockingMap:
// synchronous, on the main thread, lazily built (once per gameTime per owner)
// the first time something asks for that owner's map this tick.
// ---------------------------------------------------------------------------

static std::vector<std::shared_ptr<DensityFlowMap>> densityFlowMapsThisTick;
static uint32_t densityFlowCurrentGameTime = 0;

// History is intentionally small (indexed by owner, MAX_PLAYERS entries) and
// keyed by owner only - never merged across owners. See file header privacy note.
static std::shared_ptr<const DensityFlowMap> densityFlowPrevMap[MAX_PLAYERS];

std::shared_ptr<const DensityFlowMap> densityFlowGetOrBuildMap(int owner)
{
	if ((unsigned)owner >= (unsigned)MAX_PLAYERS)
	{
		return nullptr;
	}

	if (densityFlowCurrentGameTime != gameTime)
	{
		// New tick: drop the live per-tick cache and rebuild lazily as requested.
		// densityFlowPrevMap[] (the reroute-trigger gate's tick-to-tick history)
		// is advanced separately, in fpathDensityFlowUpdate(), once per owner
		// per tick - so an owner who goes several ticks without a build still
		// compares against its true previous map rather than an empty one.
		densityFlowMapsThisTick.clear();
		densityFlowCurrentGameTime = gameTime;
	}

	auto found = std::find_if(densityFlowMapsThisTick.begin(), densityFlowMapsThisTick.end(),
	                           [&](const std::shared_ptr<DensityFlowMap> &m) { return m->owner == owner; });
	if (found != densityFlowMapsThisTick.end())
	{
		return *found;
	}

	auto map = std::make_shared<DensityFlowMap>();
	map->gameTime = gameTime;
	map->owner = owner;
	map->width = gameWorld.map.width;
	map->height = gameWorld.map.height;
	map->cells.assign((size_t)map->width * (size_t)map->height, DensityFlowCell());

	// Build strictly from this owner's own droids.
	for (const DROID *psDroid : gameWorld.objects.droids[owner])
	{
		DFLOW_WEIGHT_CLASS wclass;
		if (!classifyPropulsion(psDroid, wclass))
		{
			continue; // flying VTOL or unrecognized propulsion: no participation
		}
		splatDroid(map->cells, map->width, map->height, psDroid, wclass);
	}

	uint32_t checksum = 0, factor = 0;
	for (size_t i = 0; i < map->cells.size(); ++i)
	{
		const DensityFlowCell &c = map->cells[i];
		checksum ^= (uint32_t)(c.mass * 3 + c.flowX * 5 + c.flowY * 7) * (factor = 3 * factor + 1);
	}
	map->checksum = checksum;
	syncDebug("densityFlowMap(%d,%d) = %08X", gameTime, owner, checksum);

	densityFlowMapsThisTick.push_back(map);
	return map;
}

void fpathSetDensityFlowMap(PATHJOB *psJob)
{
	psJob->densityFlowMap = densityFlowGetOrBuildMap(psJob->owner);
}

void densityFlowHardReset()
{
	densityFlowMapsThisTick.clear();
	densityFlowCurrentGameTime = 0;
	for (int p = 0; p < MAX_PLAYERS; ++p)
	{
		densityFlowPrevMap[p].reset();
	}
}

// ---------------------------------------------------------------------------
// Reroute-trigger logic.
// Two-level gate (adapted idea from AD*/LPA* "patch vs redo"): a global
// count of significantly-changed tiles (per owner) must clear its own
// threshold before any per-droid check runs at all. Only then: walk that
// owner's droids' remaining asPath[pathIndex..] tail against the changed-tile
// set; first hit -> reroute that droid.
// ---------------------------------------------------------------------------

// Mutable and exposed via densityflow.h so the debug menu's "Pathfinding" tab can edit these live.
int32_t densityFlowPctThreshold = 40;        ///< percent change required to count a tile as "significantly changed"
int32_t densityFlowNoiseFloor = 8;           ///< below this (both old and new), a swing is meaningless and skipped entirely
int32_t densityFlowGlobalGateThreshold = 6;  ///< changed-tile count required before any per-droid scan runs, per owner

/// Percentage-change + flow-dot-product reroute-trigger test for a single tile.
/// A tile going from empty to occupied by same-direction convoy traffic does
/// NOT count as trigger-worthy for a droid moving with that flow - this is
/// what prevents trailing convoy units from false-triggering repaths.
static bool tileChangedSignificantly(const DensityFlowCell &oldCell, const DensityFlowCell &newCell, int32_t droidDirX, int32_t droidDirY)
{
	if (oldCell.mass < densityFlowNoiseFloor && newCell.mass < densityFlowNoiseFloor)
	{
		return false; // near-zero % swings signify meaningless meandering of dawdling droids
	}

	int32_t diff = std::abs(newCell.mass - oldCell.mass);
	int32_t denom = std::max<int32_t>(oldCell.mass, 1);
	if ((diff * 100) / denom < densityFlowPctThreshold)
	{
		return false;
	}

	// Same flow-alignment gate used by the A* cost integration: unnormalized
	// dot product of the droid's own intended direction vs. the tile's flow
	// vector. Aligned (same-direction convoy) traffic does not count as a
	// trigger, even though the raw mass swing cleared the threshold above.
	int64_t dot = (int64_t)droidDirX * newCell.flowX + (int64_t)droidDirY * newCell.flowY;
	if (dot > 0)
	{
		return false; // moving with the flow: not a trigger-worthy change
	}

	return true;
}

/// Flat bitmap of changed tiles. Rebuilt/cleared each tick.
static std::vector<bool> changedTileBitmap;

static void rerouteTriggerScanForOwner(int owner, const DensityFlowMap &oldMap, const DensityFlowMap &newMap)
{
	const int width = newMap.width;
	const int height = newMap.height;
	changedTileBitmap.assign((size_t)width * (size_t)height, false);

	// First gate: does this owner's traffic pattern contain enough changed tiles to be worth scanning droids?
	// We don't yet know any droid's direction here, so use a neutral (0,0) direction
	// i.e. only apply the pct/noise test, without the flow-alignment discount
	size_t changedCount = 0;
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const DensityFlowCell &oldCell = oldMap.at(x, y);
			const DensityFlowCell &newCell = newMap.at(x, y);
			if (tileChangedSignificantly(oldCell, newCell, 0, 0))
			{
				changedTileBitmap[x + y * width] = true;
				++changedCount;
			}
		}
	}

	if (changedCount < (size_t)std::max(densityFlowGlobalGateThreshold, 0))
	{
		return; // skip the per-droid scan entirely
	}

	// Second level: walk this owner's droids' remaining path tail against the changed-tile set.
	// First hit -> reroute that droid.
	for (DROID *psDroid : gameWorld.objects.droids[owner])
	{
		MOVE_CONTROL &sMove = psDroid->sMove;
		if (sMove.Status == MOVEINACTIVE || sMove.asPath.empty())
		{
			continue;
		}
		if (sMove.pathIndex < 0 || (size_t)sMove.pathIndex >= sMove.asPath.size())
		{
			continue;
		}

		Vector2i nextWp = sMove.asPath[sMove.pathIndex];
		int32_t dirX = nextWp.x - psDroid->pos.x;
		int32_t dirY = nextWp.y - psDroid->pos.y;

		bool hit = false;
		for (int idx = sMove.pathIndex; idx < (int)sMove.asPath.size() && !hit; ++idx)
		{
			Vector2i wp = sMove.asPath[idx];
			int tx = map_coord(wp.x);
			int ty = map_coord(wp.y);
			if (tx < 0 || ty < 0 || tx >= width || ty >= height)
			{
				continue;
			}
			if (!changedTileBitmap[tx + ty * width])
			{
				continue;
			}

			// Re-apply the flow alignment gate against this droid's own direction before a reroute,
			// so trailing droids of a convoy don't trigger one.
			// This doesn't seem to work well if density weight is high relative to flow weight.
			const DensityFlowCell &oldCell = oldMap.at(tx, ty);
			const DensityFlowCell &newCell = newMap.at(tx, ty);
			if (tileChangedSignificantly(oldCell, newCell, dirX, dirY))
			{
				hit = true;
			}
		}

		if (hit)
		{
			objTrace(psDroid->id, "Density/flow reroute trigger to (%d,%d)", sMove.destination.x, sMove.destination.y);
			moveDroidTo(psDroid, sMove.destination.x, sMove.destination.y);
		}
	}
}

void fpathDensityFlowUpdate()
{
	// For every owner with at least one moving droid, ensure this tick's map is built,
	// then compare it against map from previous tick (if exists) to find (significantly)
	// changed tiles and reroute droids whose remaining path tail crosses one.
	for (int owner = 0; owner < MAX_PLAYERS; ++owner)
	{
		if (gameWorld.objects.droids[owner].empty())
		{
			continue;
		}

		auto prev = densityFlowPrevMap[owner];
		auto current = densityFlowGetOrBuildMap(owner);
		if (!current)
		{
			continue;
		}

		if (prev && prev->gameTime != current->gameTime)
		{
			rerouteTriggerScanForOwner(owner, *prev, *current);
		}

		// next tick's "previous" is this tick's map.
		densityFlowPrevMap[owner] = current;
	}
}
