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
 *  Per-player, per-tile ground/water traffic density + flow grid, used to make
 *  A* congestion-aware and to let moving droids reroute when local congestion
 *  changes.
 *
 *  Privacy: this grid is built strictly per-owner, from that owner's
 *  own droids only (see densityFlowBuildMap). A player's A* query only ever reads
 *  its own grid. Nothing here may be extended to aggregate or expose another
 *  player's droid positions/flow through this system - doing so would leak
 *  information about enemy units through fog of war.
 *
 *  Ground only. VTOL flight pathing is untouched by this feature; only a landed
 *  VTOL (a real, potentially long-lived obstacle) participates, via its own
 *  weight-class row. Land and water share a single grid, matching the engine's
 *  existing domain model (see fpathPropulsionDomain in fpath.cpp) - ground and
 *  water tiles never spatially overlap, so one array indexed by tile coordinate
 *  is safe by construction, which is what HOVER (which crosses both)
 *  needs to get one consistent signal.
 */

#ifndef __INCLUDED_SRC_DENSITYFLOW_H__
#define __INCLUDED_SRC_DENSITYFLOW_H__

#include "lib/framework/vector.h"
#include "statsdef.h"

#include <memory>
#include <vector>
#include <cstdint>

struct PATHJOB;

/// One propulsion "weight class" row for the weight table, 
/// In order of campaign unlock.
enum DFLOW_WEIGHT_CLASS
{
	DFLOW_WHEELED,
	DFLOW_HALF_TRACKED,
	DFLOW_TRACKED,
	DFLOW_HOVER,
	DFLOW_LEGGED,
	DFLOW_LANDED_VTOL,   ///< isVtol() && sMove.Status == MOVEINACTIVE. Real hitbox splat like any droid; no auto-takeoff-to-yield mechanic exists.
	DFLOW_PROPELLOR,     ///< Naval. No shipped content uses it, but the engine's water handling is functional, so it's handled correctly for free.
	DFLOW_WEIGHT_CLASS_COUNT,
};

/// Per-tile accumulator: total splatted mass, plus the sum of contributing droids'
/// waypoint-delta flow vectors (asPath[pathIndex] - pos, NOT instantaneous velocity -
/// stable across ticks, avoids steering jitter). (0,0) contribution for idle/orderless
/// droids. Both mass and flow accumulate via plain integer add, so the result is
/// associative/commutative regardless of droid iteration order.
struct DensityFlowCell
{
	int32_t mass = 0;
	int32_t flowX = 0;
	int32_t flowY = 0;
};

/// One player's frozen-for-the-tick density/flow grid. Workers only ever consume an
/// already-frozen shared_ptr<const DensityFlowMap> snapshot; the grid is built
/// synchronously on the main thread, same place/pattern as fpathSetBlockingMap builds
/// dangerMap.
struct DensityFlowMap
{
	uint32_t gameTime = 0;
	int owner = -1;
	int width = 0;
	int height = 0;
	std::vector<DensityFlowCell> cells;   ///< Indexed by x + y * width.
	uint32_t checksum = 0;                ///< syncDebug companion, mirrors checksumDangerMap.

	const DensityFlowCell &at(int x, int y) const
	{
		static const DensityFlowCell empty;
		if ((unsigned)x >= (unsigned)width || (unsigned)y >= (unsigned)height)
		{
			return empty;
		}
		return cells[x + y * width];
	}
};

/// per-propulsion-class weight table (see densityflow.cpp for the
/// defaults). Mutable: the debug menu's "Pathfinding" tab (see
/// wzscriptdebug.cpp) edits these live for tuning. Only ever read on the main thread,
/// while building a tick's map (densityFlowGetOrBuildMap) - worker threads only ever
/// see an already-frozen DensityFlowMap snapshot, so live edits are thread-safe.
extern int32_t densityFlowClassWeight[DFLOW_WEIGHT_CLASS_COUNT];
const char *densityFlowClassWeightName(DFLOW_WEIGHT_CLASS wclass);

/// Reroute-trigger tunables (see densityflow.cpp for defaults and meaning). Same
/// main-thread-only, debug-menu-editable contract as densityFlowClassWeight above.
extern int32_t densityFlowPctThreshold;
extern int32_t densityFlowNoiseFloor;
extern int32_t densityFlowGlobalGateThreshold;

/// Call from main thread. Sets psJob->densityFlowMap for later use by the pathfinding
/// thread, generating (and caching for this gameTime) the requested owner's map if not
/// already generated this tick. Mirrors fpathSetBlockingMap's cadence and caching.
void fpathSetDensityFlowMap(PATHJOB *psJob);

/// Returns (building if necessary) this tick's density/flow map for the given owner.
/// Used both by fpathSetDensityFlowMap and by the per-tick reroute-trigger scan
/// (fpathDensityFlowUpdate, see fpath.cpp / move.cpp).
std::shared_ptr<const DensityFlowMap> densityFlowGetOrBuildMap(int owner);

/// Per-tick maintenance: for every player with at least one moving ground/water droid,
/// ensures this tick's map is built, compares it against last tick's map for that
/// owner using the percentage-change + flow-dot-product gate, and reroutes at most one
/// droid per significantly-changed tile it is still travelling towards. Call once per
/// tick from fpathUpdate().
void fpathDensityFlowUpdate();

/// Clears all cached density/flow maps and per-owner history. Call on shutdown / load,
/// alongside fpathHardTableReset.
void densityFlowHardReset();

#endif // __INCLUDED_SRC_DENSITYFLOW_H__
