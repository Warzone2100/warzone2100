/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_ASTART_H__
#define __INCLUDED_SRC_ASTART_H__

#include "fpath.h"
#include <cstdint>
#include <memory>

/** Counts nodes expanded by the search, for the pathfinding benchmark.
 *
 *  Node counts are exact and reproducible where wall-clock is not, so a
 *  benchmark can require them to change by zero rather than a timing staying
 *  within noise.
 *
 *  Two guards, because the cost lands in the search loop. The compile-time one
 *  takes the counting out of a build entirely, and the pointer is null unless a
 *  benchmark is running, leaving one predictable not-taken branch per expanded
 *  node. It counts once per expansion rather than once per neighbour, so it
 *  fires at a fraction of the rate of the blocking checks beside it.
 *
 *  The pointer is written by the benchmark and read by whichever thread is
 *  searching, so it must only be set while the search is driven from one
 *  thread. The benchmark leaves it null for its worker-pool measurements, where
 *  concurrent searches would race on it.
 */
#ifndef WZ_PATHFINDING_INSTRUMENTATION
# define WZ_PATHFINDING_INSTRUMENTATION 1
#endif

#if WZ_PATHFINDING_INSTRUMENTATION
extern uint64_t *g_pathNodesExpanded;
#endif

/** return codes for astar
 *
 *  @ingroup pathfinding
 */
enum ASR_RETVAL
{
	ASR_OK,         ///< found a route
	ASR_FAILED,     ///< no route could be found
	ASR_NEAREST,    ///< found a partial route to a nearby position
};

class FPathExecuteContext
{
protected:
	FPathExecuteContext() { }
public:
	virtual ~FPathExecuteContext();
};
std::shared_ptr<FPathExecuteContext> makeFPathExecuteContext();

/** Use the A* algorithm to find a path
 *
 *  @ingroup pathfinding
 */
ASR_RETVAL fpathAStarRoute(const std::shared_ptr<FPathExecuteContext>& ctx, MOVE_CONTROL *psMove, PATHJOB *psJob);

/// Call from main thread.
/// Sets psJob->blockingMap for later use by pathfinding thread, generating the required map if not already generated.
void fpathSetBlockingMap(PATHJOB *psJob);

/** Clean up the path finding node table.
 *
 *  @note Call this on shutdown to prevent memory from leaking, or if loading/saving, to prevent stale data from being reused.
 *
 *  @ingroup pathfinding
 */
void fpathHardTableReset();

#endif // __INCLUDED_SRC_ASTART_H__
