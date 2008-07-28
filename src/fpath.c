/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
/**
 * @file fpath.c
 *
 * Interface to the routing functions.
 *
 */

#include "lib/framework/frame.h"

#include "objects.h"
#include "map.h"
#include "raycast.h"
#include "geometry.h"
#include "hci.h"
#include "order.h"

#include "astar.h"
#include "action.h"
#include "formation.h"

#include "fpath.h"


/* Beware: Enabling this will cause significant slow-down. */
#undef DEBUG_MAP

/* minimum height difference for VTOL blocking tile */
#define	LIFT_BLOCK_HEIGHT_LIGHTBODY		  30
#define	LIFT_BLOCK_HEIGHT_MEDIUMBODY	 350
#define	LIFT_BLOCK_HEIGHT_HEAVYBODY		 350

#define NUM_DIR		8


// Convert a direction into an offset
// dir 0 => x = 0, y = -1
static const Vector2i aDirOffset[NUM_DIR] =
{
	{ 0, 1},
	{-1, 1},
	{-1, 0},
	{-1,-1},
	{ 0,-1},
	{ 1,-1},
	{ 1, 0},
	{ 1, 1},
};

// if a route is spread over a number of frames this stores the droid
// the route is being done for
static DROID* psPartialRouteDroid = NULL;

// coords of the partial route
static SDWORD partialSX,partialSY, partialTX,partialTY;

// the last frame on which the partial route was calculatated
static SDWORD	lastPartialFrame;


// initialise the findpath module
BOOL fpathInitialise(void)
{
	psPartialRouteDroid = NULL;

	return true;
}


void fpathShutdown()
{
	fpathHardTableReset();
}


/** Updates the pathfinding system.
 *  @post Pathfinding jobs for DROID's that died, aren't waiting for a route
 *        anymore, or the currently calculated route is outdated for, are
 *        removed from the job queue.
 *
 *  @ingroup pathfinding
 */
void fpathUpdate(void)
{
	if (psPartialRouteDroid != NULL
	 && (psPartialRouteDroid->died
	  || psPartialRouteDroid->sMove.Status != MOVEWAITROUTE
	  || (lastPartialFrame + 5) < frameGetFrameNumber()))
	{
		psPartialRouteDroid = NULL;
	}
}


// Check if the map tile at a location blocks a droid
BOOL fpathBlockingTile(SDWORD x, SDWORD y, PROPULSION_TYPE propulsion)
{
	MAPTILE	*psTile;

	/* All tiles outside of the map and on map border are blocking. */
	if (x < 1 || y < 1 || x >= mapWidth - 1 || y >= mapHeight - 1)
	{
		return true;
	}

	/* Check scroll limits (used in campaign to partition the map. */
	if (propulsion != PROPULSION_TYPE_LIFT && (x < scrollMinX + 1 || y < scrollMinY + 1 || x >= scrollMaxX - 1 || y >= scrollMaxY - 1))
	{
		// coords off map - auto blocking tile
		return true;
	}

	psTile = mapTile(x, y);

	// Only tall structures are blocking VTOL now
	if (propulsion == PROPULSION_TYPE_LIFT && !TileHasTallStructure(psTile))
	{
		return false;
	}
	else if (propulsion == PROPULSION_TYPE_LIFT)
	{
		return true;
	}
	else if (propulsion == PROPULSION_TYPE_PROPELLOR && terrainType(psTile) != TER_WATER)
	{
		return true;
	}

	if (psTile->tileInfoBits & BITS_FPATHBLOCK || (TileIsOccupied(psTile) && !TILE_IS_NOTBLOCKING(psTile))
	    || terrainType(psTile) == TER_CLIFFFACE 
	    || (terrainType(psTile) == TER_WATER && propulsion != PROPULSION_TYPE_HOVER && propulsion != PROPULSION_TYPE_PROPELLOR))
	{
		return true;
	}

	return false;
}


/** Calculate the distance to a tile from a point
 *
 *  @ingroup pathfinding
 */
static inline int fpathDistToTile(int tileX, int tileY, int pointX, int pointY)
{
	// get the difference in world coords
	int xdiff = world_coord(tileX) - pointX;
	int ydiff = world_coord(tileY) - pointY;

	ASSERT(xdiff != 0 || ydiff != 0, "fpathDistToTile: points are on same position");

	return trigIntSqrt(xdiff * xdiff + ydiff * ydiff);
}


void fpathSetDirectRoute(DROID* psDroid, SDWORD targetX, SDWORD targetY)
{
	MOVE_CONTROL *psMoveCntl;

	ASSERT(psDroid != NULL, "fpathSetDirectRoute: invalid droid pointer");
	ASSERT(psDroid->type == OBJ_DROID, "We got passed a DROID that isn't a DROID!");

	psMoveCntl = &psDroid->sMove;

	psMoveCntl->asPath = realloc(psMoveCntl->asPath, sizeof(*psMoveCntl->asPath));
	psMoveCntl->DestinationX = targetX;
	psMoveCntl->DestinationY = targetY;
	psMoveCntl->numPoints = 1;
	psMoveCntl->asPath[0].x = map_coord(targetX);
	psMoveCntl->asPath[0].y = map_coord(targetY);
}


// Variables for the callback
static SDWORD	finalX,finalY, vectorX,vectorY;
static SDWORD	clearX,clearY;
static BOOL		obstruction;


/** Callback to find the first clear tile before an obstructed target
 *
 *  @ingroup pathfinding
 */
static bool fpathEndPointCallback(Vector3i pos, int dist, void* data)
{
	// See if this point is past the final point (dot product)
	int vx = pos.x - finalX;
	int vy = pos.y - finalY;

	if (vx*vectorX + vy*vectorY <= 0)
	{
		return false;
	}

	// note the last clear tile
	if (!fpathBlockingTile(map_coord(pos.x), map_coord(pos.y), *(PROPULSION_TYPE*)data))
	{
		clearX = (pos.x & ~TILE_MASK) + TILE_UNITS/2;
		clearY = (pos.y & ~TILE_MASK) + TILE_UNITS/2;
	}
	else
	{
		obstruction = true;
	}

	return true;
}


/** Create a final route from a gateway route
 *
 *  @ingroup pathfinding
 */
static FPATH_RETVAL fpathGatewayRoute(DROID* psDroid, SDWORD routeMode, SDWORD sx, SDWORD sy,
                                      SDWORD fx, SDWORD fy, MOVE_CONTROL *psMoveCntl, PROPULSION_TYPE propulsion)
{
	int			asret;

	if (routeMode == ASR_NEWROUTE)
	{
		// initialise the move control structures
		psMoveCntl->numPoints = 0;
	}

	objTrace(psDroid->id, "fpathGatewayRoute: astar route : (%d,%d) -> (%d,%d)",
			 map_coord(sx), map_coord(sy), map_coord(fx), map_coord(fy));
	asret = fpathAStarRoute(routeMode, &psDroid->sMove, sx, sy, fx,fy, propulsion);
	if (asret == ASR_PARTIAL)
	{
		// routing hasn't finished yet
		objTrace(psDroid->id, "fpathGatewayRoute: Reschedule");
		return FPR_WAIT;
	}
	else if (asret == ASR_NEAREST)
	{
		// all routing was in one zone - this is as good as it's going to be
		objTrace(psDroid->id, "fpathGatewayRoute: Nearest route in same zone");
		return FPR_OK;
	}
	else if (asret == ASR_FAILED)
	{
		// all routing was in one zone - can't retry
		objTrace(psDroid->id, "fpathGatewayRoute: Failed route in same zone");
		return FPR_FAILED;
	}
	return FPR_OK;
}


// Find a route for an DROID to a location
FPATH_RETVAL fpathRoute(DROID* psDroid, SDWORD tX, SDWORD tY)
{
	FPATH_RETVAL retVal = FPR_OK;
	MOVE_CONTROL *psMoveCntl = &psDroid->sMove;
	Vector3i start = { psDroid->pos.x, psDroid->pos.y, 0 }, target = { tX, tY, 0 };
	PROPULSION_STATS *psPropStats;

	ASSERT(psDroid->type == OBJ_DROID, "We got passed a DROID that isn't a DROID!");

	if (psPartialRouteDroid == NULL || psPartialRouteDroid != psDroid)
	{
	}
	else if (psDroid->sMove.Status == MOVEWAITROUTE
	      && psDroid->sMove.DestinationX != tX)
	{
		// we have a partial route, but changed destination, so need to recalculate
		psPartialRouteDroid = NULL;
	}
	else
	{
		// continuing routing for the DROID
		start = Vector3i_New(partialSX, partialSY, 0);
		target = Vector3i_New(partialTX, partialTY, 0);
	}

	// don't have to do anything if already there
	if (Vector3i_Compare(start, target))
	{
		// return failed to stop them moving anywhere
		return FPR_FAILED;
	}

	// set the correct blocking tile function
	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT(psPropStats != NULL, "invalid propulsion stats pointer");

	if (psPartialRouteDroid == NULL || psPartialRouteDroid != psDroid)
	{
		Vector3i final = { (target.x & ~TILE_MASK) + TILE_UNITS/2, (target.y & ~TILE_MASK) + TILE_UNITS/2, 0 };

		// check whether the start point of the route
		// is a blocking tile and find an alternative if it is
		if (fpathBlockingTile(map_coord(start.x), map_coord(start.y), psPropStats->propulsionType))
		{

			// find the nearest non blocking tile to the DROID
			int minDist = SDWORD_MAX;
			int nearestDir = NUM_DIR;
			int dir;

			for (dir = 0; dir < NUM_DIR; dir++)
			{
				Vector3i test = { map_coord(start.x) + aDirOffset[dir].x, map_coord(start.y) + aDirOffset[dir].y, 0};
				if (!fpathBlockingTile(test.x, test.y, psPropStats->propulsionType))
				{
					int tileDist = fpathDistToTile(test.x, test.y, start.x, start.y);
					if (tileDist < minDist)
					{
						minDist = tileDist;
						nearestDir = dir;
					}
				}
			}

			if (nearestDir == NUM_DIR)
			{
				// surrounded by blocking tiles, give up
 				objTrace(psDroid->id, "droid %u: route failed (surrouned by blocking)", (unsigned int)psDroid->id);
				return FPR_FAILED;
			}

			start.x = world_coord(map_coord(start.x) + aDirOffset[nearestDir].x)
				+ TILE_SHIFT / 2;
			start.y = world_coord(map_coord(start.y) + aDirOffset[nearestDir].y)
				+ TILE_SHIFT / 2;
		}

		// initialise the raycast - if there is los to the target, no routing necessary
		finalX = final.x;
		finalY = final.y;
		clearX = finalX; clearY = finalY;

		{
			Vector3i dir = Vector3i_Sub(final, start);

			vectorX = -dir.x;
			vectorY = -dir.y;
			obstruction = false;

			// cast the ray to find the last clear tile before the obstruction
			rayCast(start, dir, RAY_MAXLEN, fpathEndPointCallback, &psPropStats->propulsionType);
		}

		if (!obstruction)
		{
			// no obstructions - trivial route
			fpathSetDirectRoute(psDroid, target.x, target.y);
 			objTrace(psDroid->id, "droid %u: trivial route", (unsigned int)psDroid->id);
			if (psPartialRouteDroid != NULL)
			{
				objTrace(psDroid->id, "Unit %u: trivial route during multi-frame route", (unsigned int)psDroid->id);
			}
			return FPR_OK;
		}

		// check whether the end point of the route
		// is a blocking tile and find an alternative if it is
		if (fpathBlockingTile(map_coord(target.x), map_coord(target.y), psPropStats->propulsionType))
		{
			// route to the last clear tile found by the raycast
			// Does this code work? - Per
			target.x = clearX;
			target.y = clearY;
			objTrace(psDroid->id, "Unit %u: end point is blocked, going to (%d, %d) instead",
			         (unsigned int)psDroid->id, (int)clearX, (int)clearY);
		}
	}

	ASSERT(start.x >= 0 && start.x < mapWidth * TILE_UNITS && start.y >= 0 && start.y < mapHeight * TILE_UNITS,
	       "start coords (%d, %d) off map (%u, %u)", start.x, start.y, mapWidth * TILE_UNITS, mapHeight * TILE_UNITS);
	ASSERT(target.x >= 0 && target.x < mapWidth * TILE_UNITS && target.y >= 0 && target.y < mapHeight * TILE_UNITS,
	       "target coords (%d, %d) off map (%u, %u)", target.x, target.y, mapWidth * TILE_UNITS, mapHeight * TILE_UNITS);
	ASSERT(astarInner >= 0, "astarInner overflowed!");
	if (start.x < 0 || start.y < 0 || target.x < 0 || target.y < 0
	    || start.x >= mapWidth * TILE_UNITS || start.y >= mapHeight * TILE_UNITS
	    || target.x >= mapWidth * TILE_UNITS || target.y >= mapHeight * TILE_UNITS)
	{
		return FPR_FAILED;	// fallback if we play with asserts off
	}

	if (astarInner > FPATH_LOOP_LIMIT)
	{
		// Time out
		if (psPartialRouteDroid == psDroid)
		{
			return FPR_WAIT;
		}
		else
		{
			objTrace(psDroid->id, "droid %u: reschedule", (unsigned int)psDroid->id);
			return FPR_RESCHEDULE;
		}
	}
	else if ((psPartialRouteDroid != NULL
	       && psPartialRouteDroid != psDroid)
	      || (psPartialRouteDroid != psDroid
	       && psNextRouteDroid != NULL
	       && psNextRouteDroid != psDroid))
	{
		// Not our turn
		return FPR_RESCHEDULE;
	}

	// Now actually create a route
	if (psPartialRouteDroid == NULL)
	{
		retVal = fpathGatewayRoute(psDroid, ASR_NEWROUTE, start.x, start.y, target.x, target.y, psMoveCntl, psPropStats->propulsionType);
	}
	else
	{
		objTrace(psDroid->id, "Partial Route");
		psPartialRouteDroid = NULL;
		retVal = fpathGatewayRoute(psDroid, ASR_CONTINUE, start.x, start.y, target.x, target.y, psMoveCntl, psPropStats->propulsionType);
	}
	if (retVal == FPR_WAIT)
	{
		psPartialRouteDroid = psDroid;
		lastPartialFrame = frameGetFrameNumber();
		partialSX = start.x;
		partialSY = start.y;
		partialTX = target.x;
		partialTY = target.y;
	}
	else if (retVal == FPR_FAILED && isVtolDroid(psDroid))
	{
		fpathSetDirectRoute(psDroid, target.x, target.y);
		retVal = FPR_OK;
	}

#ifdef DEBUG_MAP
	{
		MAPTILE *psTile;

		psTile = psMapTiles;
		for (x = 0; x < (SDWORD)(mapWidth*mapHeight); x++)
		{
			if (psTile->tileInfoBits & BITS_FPATHBLOCK)
			{
				ASSERT( false,"blocking flags still in the map" );
			}
			psTile++;
		}
	}
#endif

	return retVal;
}
