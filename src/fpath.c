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

/* global pointer for droid being routed - GJ hack -
 * currently only used in fpathLiftBlockingTile */
static DROID* g_psDroidRoute = NULL;

// function pointer for the blocking tile check
BOOL (*fpathBlockingTile)(SDWORD x, SDWORD y);

// if a route is spread over a number of frames this stores the droid
// the route is being done for
static DROID* psPartialRouteDroid = NULL;

// coords of the partial route
static SDWORD partialSX,partialSY, partialTX,partialTY;

// the last frame on which the partial route was calculatated
static SDWORD	lastPartialFrame;

static BOOL fpathFindRoute(DROID *psDroid, SDWORD sX,SDWORD sY, SDWORD tX,SDWORD tY);
static BOOL fpathLiftBlockingTile(SDWORD x, SDWORD y);

// initialise the findpath module
BOOL fpathInitialise(void)
{
	fpathBlockingTile = fpathGroundBlockingTile;
	psPartialRouteDroid = NULL;

	return true;
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
BOOL fpathGroundBlockingTile(SDWORD x, SDWORD y)
{
	MAPTILE	*psTile;

	/* check VTOL limits if not routing */
	if (x < scrollMinX + 1
	 || y < scrollMinY + 1
	 || x >= scrollMaxX - 1
	 || y >= scrollMaxY - 1)
	{
		// coords off map - auto blocking tile
		return true;
	}

	ASSERT(x >= 0 && y >= 0 && x < mapWidth && y < mapHeight, "fpathGroundBlockingTile: off map")

	psTile = mapTile(x, y);

	if (psTile->tileInfoBits & BITS_FPATHBLOCK
	 || (TileIsOccupied(psTile)
	  && !TILE_IS_NOTBLOCKING(psTile))
	 || terrainType(psTile) == TER_CLIFFFACE
	 || terrainType(psTile) == TER_WATER)
	{
		return true;
	}

	return false;
}

// Check if the map tile at a location blocks a droid
BOOL fpathHoverBlockingTile(SDWORD x, SDWORD y)
{
	MAPTILE	*psTile;

	if (x < scrollMinX + 1
	 || y < scrollMinY + 1
	 || x >= scrollMaxX - 1
	 || y >= scrollMaxY - 1)
	{
		// coords off map - auto blocking tile
		return true;
	}

	ASSERT(x >= 0 && y >= 0 && x < mapWidth && y < mapHeight, "fpathHoverBlockingTile: off map")

	psTile = mapTile(x, y);

	if (psTile->tileInfoBits & BITS_FPATHBLOCK
	 || (TileIsOccupied(psTile)
	  && !TILE_IS_NOTBLOCKING(psTile))
	 || terrainType(psTile) == TER_CLIFFFACE)
	{
		return true;
	}

	return false;
}

/** Check if the map tile at a location blocks a VTOL droid
 *
 *  @ingroup pathfinding
 */
static BOOL fpathLiftBlockingTile(SDWORD x, SDWORD y)
{
	MAPTILE		*psTile;
	SDWORD		iLiftHeight, iBlockingHeight;

	ASSERT(g_psDroidRoute != NULL, "fpathLiftBlockingTile: invalid DROID pointer");

	// All tiles outside of the map are blocking
	if (x < 1 || y < 1 || x >= mapWidth - 1 || y >= mapHeight - 1)
	{
		return true;
	}

	psTile = mapTile(x, y);
	if (g_psDroidRoute->droidType == DROID_TRANSPORTER)
	{
		// Only tall structures are blocking now
		return TileHasTallStructure(psTile);
	}

	ASSERT(x >= 0 && y >= 0 && x < mapWidth && y < mapHeight, "fpathLiftBlockingTile: off map");

	/* no tiles are blocking if returning to rearm */
	// FIXME: Do we really want to allow VTOL that needs to reload to fly at
	//        places where other VTOLs can't go??
	if (g_psDroidRoute->action == DACTION_MOVETOREARM)
	{
		return false;
	}

	/* consider cliff faces */
	if (terrainType(psTile) == TER_CLIFFFACE)
	{
		switch ( (asBodyStats + g_psDroidRoute->asBits[COMP_BODY].nStat)->size )
		{
			case SIZE_LIGHT:
				iBlockingHeight = LIFT_BLOCK_HEIGHT_LIGHTBODY;
				break;
			case SIZE_MEDIUM:
				iBlockingHeight = LIFT_BLOCK_HEIGHT_MEDIUMBODY;
				break;
			case SIZE_HEAVY:
				iBlockingHeight = LIFT_BLOCK_HEIGHT_HEAVYBODY;
				break;
			default:
				iBlockingHeight = LIFT_BLOCK_HEIGHT_LIGHTBODY;
		}

		/* approaching cliff face; block if below it */
		iLiftHeight = (SDWORD) map_Height(world_coord(x), world_coord(y)) -
					  (SDWORD) map_Height( g_psDroidRoute->pos.x, g_psDroidRoute->pos.y );
		if (iLiftHeight > iBlockingHeight)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		// Only tall structures are blocking now
		return TileHasTallStructure(psTile);
	}
}

// Check if an edge map tile blocks a vtol (for sliding at map edge)
BOOL fpathLiftSlideBlockingTile(SDWORD x, SDWORD y)
{
	if ( x < 1 || y < 1 ||
		 x >= (SDWORD)GetWidthOfMap()-1  ||
		 y >= (SDWORD)GetHeightOfMap()-1    )
	{
		return true;
	}
	else
	{
		return false;
	}
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

// Variables for the callback
static SDWORD	finalX,finalY, vectorX,vectorY;
static SDWORD	clearX,clearY;
static BOOL		obstruction;

/** Callback to find the first clear tile before an obstructed target
 *
 *  @ingroup pathfinding
 */
static BOOL fpathEndPointCallback(SDWORD x, SDWORD y, SDWORD dist)
{
	SDWORD	vx,vy;

	dist = dist;

	// See if this point is past the final point (dot product)
	vx = x - finalX;
	vy = y - finalY;
	if (vx*vectorX + vy*vectorY <=0)
	{
		return false;
	}

	// note the last clear tile
	if (!fpathBlockingTile(map_coord(x), map_coord(y)))
	{
		clearX = (x & ~TILE_MASK) + TILE_UNITS/2;
		clearY = (y & ~TILE_MASK) + TILE_UNITS/2;
	}
	else
	{
		obstruction = true;
	}

	return true;
}

void fpathSetDirectRoute(DROID* psDroid, SDWORD targetX, SDWORD targetY)
{
	MOVE_CONTROL *psMoveCntl;

	ASSERT(psDroid != NULL, "fpathSetDirectRoute: invalid droid pointer");
	ASSERT(psDroid->type == OBJ_DROID, "We got passed a DROID that isn't a DROID!");

	psMoveCntl = &psDroid->sMove;

	/* set global pointer for DROID being routed - GJ hack */
	fpathSetCurrentDroid(psDroid);

	psMoveCntl->DestinationX = targetX;
	psMoveCntl->DestinationY = targetY;
	psMoveCntl->numPoints = 1;
	psMoveCntl->asPath[0].x = map_coord(targetX);
	psMoveCntl->asPath[0].y = map_coord(targetY);
}

/** Append an astar route onto a move-control route
 *
 *  @ingroup pathfinding
 */
static void fpathAppendRoute( MOVE_CONTROL *psMoveCntl, ASTAR_ROUTE *psAStarRoute )
{
	SDWORD		mi, ai;

	mi = psMoveCntl->numPoints;
	ai = 0;
	while ((mi < TRAVELSIZE) && (ai < psAStarRoute->numPoints))
	{
		psMoveCntl->asPath[mi].x = (UBYTE)(psAStarRoute->asPos[ai].x);
		psMoveCntl->asPath[mi].y = (UBYTE)(psAStarRoute->asPos[ai].y);

		ai += 1;
		mi += 1;
	}

	psMoveCntl->numPoints = (UBYTE)(psMoveCntl->numPoints + ai);
	psMoveCntl->DestinationX = world_coord(psAStarRoute->finalX) + TILE_UNITS/2;
	psMoveCntl->DestinationY = world_coord(psAStarRoute->finalY) + TILE_UNITS/2;
}

/** Check if a new route is closer to the target than the one stored in the
 *  droid
 *
 *  @ingroup pathfinding
 */
static BOOL fpathRouteCloser(MOVE_CONTROL *psMoveCntl, ASTAR_ROUTE *psAStarRoute, SDWORD tx,SDWORD ty)
{
	SDWORD	xdiff,ydiff, prevDist, nextDist;

	if (psAStarRoute->numPoints == 0)
	{
		// no route to copy do nothing
		return false;
	}

	if (psMoveCntl->numPoints == 0)
	{
		// no previous route - this has to be better
		return true;
	}

	// see which route is closest to the final destination
	xdiff = world_coord(psMoveCntl->asPath[psMoveCntl->numPoints - 1].x) + TILE_UNITS/2 - tx;
	ydiff = world_coord(psMoveCntl->asPath[psMoveCntl->numPoints - 1].y) + TILE_UNITS/2 - ty;
	prevDist = xdiff*xdiff + ydiff*ydiff;

	xdiff = world_coord(psAStarRoute->finalX) + TILE_UNITS/2 - tx;
	ydiff = world_coord(psAStarRoute->finalY) + TILE_UNITS/2 - ty;
	nextDist = xdiff*xdiff + ydiff*ydiff;

	if (nextDist < prevDist)
	{
		return true;
	}

	return false;
}

/** Create a final route from a gateway route
 *
 *  @ingroup pathfinding
 */
static FPATH_RETVAL fpathGatewayRoute(DROID* psDroid, SDWORD routeMode, SDWORD sx, SDWORD sy, 
                                      SDWORD fx, SDWORD fy, MOVE_CONTROL *psMoveCntl)
{
	static ASTAR_ROUTE	sAStarRoute;
	int			asret;

	if (routeMode == ASR_NEWROUTE)
	{
		// initialise the move control structures
		psMoveCntl->numPoints = 0;
		sAStarRoute.numPoints = 0;
	}

	objTrace(LOG_MOVEMENT, psDroid->id, "fpathGatewayRoute: astar route : (%d,%d) -> (%d,%d)",
		map_coord(sx), map_coord(sy), map_coord(fx), map_coord(fy));
	asret = fpathAStarRoute(routeMode, &sAStarRoute, sx, sy, fx,fy);
	if (asret == ASR_PARTIAL)
	{
		// routing hasn't finished yet
		objTrace(LOG_MOVEMENT, psDroid->id, "fpathGatewayRoute: Reschedule");
		return FPR_WAIT;
	}
	routeMode = ASR_NEWROUTE;

	if (asret == ASR_NEAREST && actionRouteBlockingPos(psDroid, sAStarRoute.finalX,sAStarRoute.finalY))
	{
		// found a blocking wall - route to that
		objTrace(LOG_MOVEMENT, psDroid->id, "fpathGatewayRoute: Got blocking wall");
		return FPR_OK;
	}
	else if (asret == ASR_NEAREST)
	{
		// all routing was in one zone - this is as good as it's going to be
		objTrace(LOG_MOVEMENT, psDroid->id, "fpathGatewayRoute: Nearest route in same zone");
		if (fpathRouteCloser(psMoveCntl, &sAStarRoute, fx,fy))
		{
			psMoveCntl->numPoints = 0;
			fpathAppendRoute(psMoveCntl, &sAStarRoute);
		}
		return FPR_OK;
	}
	else if (asret == ASR_FAILED)
	{
		// all routing was in one zone - can't retry
		objTrace(LOG_MOVEMENT, psDroid->id, "fpathGatewayRoute: Failed route in same zone");
		return FPR_FAILED;
	}
	else
	{
		if (fpathRouteCloser(psMoveCntl, &sAStarRoute, fx,fy))
		{
			psMoveCntl->numPoints = 0;
			fpathAppendRoute(psMoveCntl, &sAStarRoute);
		}
		return FPR_OK;
	}
}

void fpathSetCurrentDroid(DROID* psDroid)
{
	g_psDroidRoute = psDroid;
}

// set the correct blocking tile function
void fpathSetBlockingTile( UBYTE ubPropulsionType )
{
	switch ( ubPropulsionType )
	{
	case HOVER:
		fpathBlockingTile = fpathHoverBlockingTile;
		break;
	case LIFT:
		fpathBlockingTile = fpathLiftBlockingTile;
		break;
	default:
		fpathBlockingTile = fpathGroundBlockingTile;
	}
}

// Find a route for an DROID to a location
FPATH_RETVAL fpathRoute(DROID* psDroid, SDWORD tX, SDWORD tY)
{
	MOVE_CONTROL		*psMoveCntl = &psDroid->sMove;
	SDWORD				startX,startY, targetX,targetY;
	SDWORD				dir, nearestDir, minDist, tileDist;
	FPATH_RETVAL		retVal = FPR_OK;
	PROPULSION_STATS	*psPropStats;

	ASSERT(psDroid->type == OBJ_DROID, "We got passed a DROID that isn't a DROID!");

	/* set global pointer for DROID being routed - GJ hack */
	fpathSetCurrentDroid(psDroid);

	if (psPartialRouteDroid == NULL || psPartialRouteDroid != psDroid)
	{
		targetX = tX;
		targetY = tY;
		startX = psDroid->pos.x;
		startY = psDroid->pos.y;
	}
	else if (psDroid->sMove.Status == MOVEWAITROUTE
	      && psDroid->sMove.DestinationX != tX)
	{
		// we have a partial route, but changed destination, so need to recalculate
		psPartialRouteDroid = NULL;
		targetX = tX;
		targetY = tY;
		startX = psDroid->pos.x;
		startY = psDroid->pos.y;
	}
	else
	{
		// continuing routing for the DROID
		startX = partialSX;
		startY = partialSY;
		targetX = partialTX;
		targetY = partialTY;
	}

	// don't have to do anything if already there
	if (startX == targetX && startY == targetY)
	{
		// return failed to stop them moving anywhere
		return FPR_FAILED;
	}

	// set the correct blocking tile function
	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT(psPropStats != NULL, "invalid propulsion stats pointer");

	fpathSetBlockingTile( psPropStats->propulsionType );

	if (psPartialRouteDroid == NULL || psPartialRouteDroid != psDroid)
	{
		// check whether the start point of the route
		// is a blocking tile and find an alternative if it is
		if (fpathBlockingTile(map_coord(startX), map_coord(startY)))
		{
			// find the nearest non blocking tile to the DROID
			minDist = SDWORD_MAX;
			nearestDir = NUM_DIR;
			for(dir=0; dir<NUM_DIR; dir++)
			{
				int x = map_coord(startX) + aDirOffset[dir].x;
				int y = map_coord(startY) + aDirOffset[dir].y;
				if (!fpathBlockingTile(x,y))
				{
					tileDist = fpathDistToTile(x,y, startX,startY);
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
				retVal = FPR_FAILED;
 				objTrace(LOG_MOVEMENT, psDroid->id, "droid %u: route failed (surrouned by blocking)", (unsigned int)psDroid->id);
				goto exit;
			}
			else
			{
				startX = world_coord(map_coord(startX) + aDirOffset[nearestDir].x)
							+ TILE_SHIFT / 2;
				startY = world_coord(map_coord(startY) + aDirOffset[nearestDir].y)
							+ TILE_SHIFT / 2;
			}
		}

		// initialise the raycast - if there is los to the target, no routing necessary
		finalX = targetX & ~TILE_MASK;
		finalX += TILE_UNITS/2;
		finalY = targetY & ~TILE_MASK;
		finalY += TILE_UNITS/2;

		clearX = finalX; clearY = finalY;
		vectorX = startX - finalX;
		vectorY = startY - finalY;
		obstruction = false;

		// cast the ray to find the last clear tile before the obstruction
		rayCast(startX,startY, rayPointsToAngle(startX,startY, finalX, finalY),
				RAY_MAXLEN, fpathEndPointCallback);

		if (!obstruction)
		{
			// no obstructions - trivial route
			fpathSetDirectRoute(psDroid, targetX, targetY);
			retVal = FPR_OK;
 			objTrace(LOG_MOVEMENT, psDroid->id, "droid %u: trivial route", (unsigned int)psDroid->id);
			if (psPartialRouteDroid != NULL)
			{
				objTrace(LOG_MOVEMENT, psDroid->id, "Unit %u: trivial route during multi-frame route", (unsigned int)psDroid->id);
			}
			goto exit;
		}

		// check whether the end point of the route
		// is a blocking tile and find an alternative if it is
		if (fpathBlockingTile(map_coord(targetX), map_coord(targetY)))
		{
			// route to the last clear tile found by the raycast
			// Does this code work? - Per
			targetX = clearX;
			targetY = clearY;
			objTrace(LOG_MOVEMENT, psDroid->id, "Unit %u: end point is blocked, going to (%d, %d) instead",
			         (unsigned int)psDroid->id, (int)clearX, (int)clearY);
		}

		// see if there is another unit with a usable route
		if (fpathFindRoute(psDroid, startX,startY, targetX,targetY))
		{
			if (psPartialRouteDroid != NULL)
			{
				objTrace(LOG_MOVEMENT, psDroid->id, "droid %u: found existing route during multi-frame path",
				         (unsigned int)psDroid->id);
			}
			else
			{
 				objTrace(LOG_MOVEMENT, psDroid->id, "droid %u: found existing route", (unsigned int)psDroid->id);
			}
			goto exit;
		}
	}

	ASSERT( startX >= 0 && startX < (SDWORD)mapWidth*TILE_UNITS &&
			startY >= 0 && startY < (SDWORD)mapHeight*TILE_UNITS,
			"start coords off map" );
	ASSERT( targetX >= 0 && targetX < (SDWORD)mapWidth*TILE_UNITS &&
			targetY >= 0 && targetY < (SDWORD)mapHeight*TILE_UNITS,
			"target coords off map" );
	ASSERT( fpathBlockingTile == fpathGroundBlockingTile ||
			fpathBlockingTile == fpathHoverBlockingTile ||
			fpathBlockingTile == fpathLiftBlockingTile,
			"invalid blocking function" );

	ASSERT(astarInner >= 0, "astarInner overflowed!");

	if (astarInner > FPATH_LOOP_LIMIT)
	{
		// Time out
		if (psPartialRouteDroid == psDroid)
		{
			retVal = FPR_WAIT;
			goto exit;
		}
		else
		{
			objTrace(LOG_MOVEMENT, psDroid->id, "droid %u: reschedule", (unsigned int)psDroid->id);
			retVal = FPR_RESCHEDULE;
			goto exit;
		}
	}
	else if ((psPartialRouteDroid != NULL
	       && psPartialRouteDroid != psDroid)
	      || (psPartialRouteDroid != psDroid
	       && psNextRouteDroid != NULL
	       && psNextRouteDroid != psDroid))
	{
		// Not our turn
		retVal = FPR_RESCHEDULE;
		goto exit;
	}

	// Now actually create a route
	if (psPartialRouteDroid == NULL)
	{
		retVal = fpathGatewayRoute(psDroid, ASR_NEWROUTE, startX,startY, targetX,targetY, psMoveCntl);
	}
	else
	{
		objTrace(LOG_MOVEMENT, psDroid->id, "Partial Route");
		psPartialRouteDroid = NULL;
		retVal = fpathGatewayRoute(psDroid, ASR_CONTINUE, startX,startY, targetX,targetY, psMoveCntl);
	}
	if (retVal == FPR_WAIT)
	{
		psPartialRouteDroid = psDroid;
		lastPartialFrame = frameGetFrameNumber();
		partialSX = startX;
		partialSY = startY;
		partialTX = targetX;
		partialTY = targetY;
	}
	else if (retVal == FPR_FAILED && vtolDroid(psDroid))
	{
		fpathSetDirectRoute(psDroid, targetX, targetY);
		retVal = FPR_OK;
	}

exit:

	// reset the blocking tile function
	fpathBlockingTile = fpathGroundBlockingTile;

	/* reset global pointer for DROID being routed */
	fpathSetCurrentDroid(NULL);

#ifdef DEBUG_MAP
	{
		MAPTILE				*psTile;

		psTile = psMapTiles;
		for(x=0; x<(SDWORD)(mapWidth*mapHeight); x+= 1)
		{
			if (psTile->tileInfoBits & BITS_FPATHBLOCK)
			{
				ASSERT( false,"blocking flags still in the map" );
			}
			psTile += 1;
		}
	}
#endif

	return retVal;
}

/** Find the first point on the route which has both droids on the same side of it
 *
 *  @ingroup pathfinding
 */
static BOOL fpathFindFirstRoutePoint(MOVE_CONTROL *psMove, SDWORD *pIndex, SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2)
{
	SDWORD	vx1,vy1, vx2,vy2;

	for(*pIndex = 0; *pIndex < psMove->numPoints; (*pIndex) ++)
	{
		vx1 = x1 - psMove->asPath[ *pIndex ].x;
		vy1 = y1 - psMove->asPath[ *pIndex ].y;
		vx2 = x2 - psMove->asPath[ *pIndex ].x;
		vy2 = y2 - psMove->asPath[ *pIndex ].y;

		// found it if the dot products have the same sign
		if ( (vx1 * vx2 + vy1 * vy2) < 0 )
		{
			return true;
		}
	}

	return false;
}

/** See if there is another unit on your side that has a route this unit can use
 *
 *  @ingroup pathfinding
 */
static BOOL fpathFindRoute(DROID *psDroid, SDWORD sX,SDWORD sY, SDWORD tX,SDWORD tY)
{
	DROID		*psCurr;
	SDWORD		i, startX,startY, index;
	FORMATION* psFormation = formationFind(tX, tY);

	if (!psFormation)
	{
		return false;
	}

	// now look for a unit in this formation with a route that can be used
	for (psCurr = apsDroidLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr != psDroid
		 && psCurr != psPartialRouteDroid
		 && psCurr->sMove.psFormation == psFormation
		 && psCurr->sMove.numPoints > 0)
		{
			// find the first route point
			if (!fpathFindFirstRoutePoint(&psCurr->sMove, &index, sX,sY, (SDWORD)psCurr->pos.x, (SDWORD)psCurr->pos.y))
			{
				continue;
			}

			// initialise the raycast - if there is los to the start of the route
			startX = (sX & ~TILE_MASK) + TILE_UNITS/2;
			startY = (sY & ~TILE_MASK) + TILE_UNITS/2;
			finalX = (psCurr->sMove.asPath[index].x * TILE_UNITS) + TILE_UNITS/2;
			finalY = (psCurr->sMove.asPath[index].y * TILE_UNITS) + TILE_UNITS/2;

			clearX = finalX; clearY = finalY;
			vectorX = startX - finalX;
			vectorY = startY - finalY;
			obstruction = false;

			// cast the ray to find the last clear tile before the obstruction
			rayCast(startX,startY, rayPointsToAngle(startX,startY, finalX, finalY),
					RAY_MAXLEN, fpathEndPointCallback);

			if (!obstruction)
			{
				// This route is OK, copy it over
				for(i=index; i<psCurr->sMove.numPoints; i++)
				{
					psDroid->sMove.asPath[i] = psCurr->sMove.asPath[i];
				}
				psDroid->sMove.numPoints = psCurr->sMove.numPoints;

				// now see if the route

				return true;
			}
		}
	}

	return false;
}
