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
/*
 * Generate a 'meta' route through the gateways to guide the normal routing
 *
 */

#include "lib/framework/frame.h"

#include "map.h"
#include "gateway.h"
#include "gatewayroute.h"
#include "fpath.h"

// the open list
static GATEWAY	*psOpenList;

// estimate the distance to the target from a gateway
static SDWORD gwrEstimate(GATEWAY *psGate, SDWORD x, SDWORD y)
{
	SDWORD	gwx,gwy, dx,dy;

	gwx = (psGate->x1 + psGate->x2)/2;
	gwy = (psGate->y1 + psGate->y2)/2;

	dx = gwx - x;
	dy = gwy - y;

	dx = abs( dx );
	dy = abs( dy );

	return dx > dy ? dx + dy/2 : dx/2 + dy;
}


// add a gateway to the open list
static void gwrOpenAdd(GATEWAY *psGate)
{
	psGate->flags &= ~GWR_CLOSED;
	psGate->flags |= GWR_OPEN;
	psGate->psOpen = psOpenList;
	psOpenList = psGate;
}


// get the next gateway from the open list
static GATEWAY *gwrOpenGet(void)
{
	SDWORD		minDist, dist;
	GATEWAY		*psCurr, *psPrev, *psParent = NULL, *psFound = NULL;

	if (psOpenList == NULL)
	{
		return NULL;
	}

	psPrev = NULL;
	minDist = SDWORD_MAX;
	for (psCurr = psOpenList; psCurr; psCurr=psCurr->psOpen)
	{
		dist = psCurr->dist + psCurr->est;
		if (dist < minDist)
		{
			minDist = dist;
			psParent = psPrev;
			psFound = psCurr;
		}
		psPrev = psCurr;
	}

	// remove the found gateway from the list
	if (psParent == NULL)
	{
		psOpenList = psOpenList->psOpen;
	}
	else
	{
		psParent->psOpen = psFound->psOpen;
	}
	psFound->psOpen = NULL;
	psFound->flags &= ~(GWR_OPEN|GWR_CLOSED);

	return psFound;
}


// check whether a gateway should be considered for routing
// (i.e. whether it is inside the current scroll limits)
static BOOL gwrConsiderGateway(GATEWAY *psGate)
{
	return (psGate->x1 >= scrollMinX) && (psGate->x1 <= scrollMaxX) &&
		   (psGate->x2 >= scrollMinX) && (psGate->x2 <= scrollMaxX) &&
		   (psGate->y1 >= scrollMinY) && (psGate->y1 <= scrollMaxY) &&
		   (psGate->y2 >= scrollMinY) && (psGate->y2 <= scrollMaxY) &&
		   !(psGate->flags & GWR_BLOCKED);
}


// check whether all the tiles on a gateway are blocked
static BOOL gwrBlockedGateway(GATEWAY *psGate, SDWORD player, UDWORD terrain)
{
//	SDWORD	pos;
	BOOL	blocked;
	MAPTILE	*psTile;

	blocked = false;

	psTile = mapTile( (psGate->x1+psGate->x2)/2,
					  (psGate->y1+psGate->y2)/2);
	if ( (terrainType(psTile) == TER_WATER) &&
		 !(terrain & GWR_TER_WATER))
	{
		blocked = true;
	}
	if ( (terrainType(psTile) != TER_WATER) &&
		 !(terrain & GWR_TER_LAND))
	{
		blocked = true;
	}
	if (psGate->flags & GWR_IGNORE)
	{
		blocked = true;
	}

/*	blocked = true;
	if (psGate->x1 == psGate->x2)
	{
		for(pos = psGate->y1; pos <= psGate->y2; pos += 1)
		{
			psTile = mapTile(psGate->x1, pos);
			if (!fpathBlockingTile(psGate->x1, pos) &&
				TEST_TILE_VISIBLE(player, psTile))
			{
				blocked = false;
			}
		}
	}
	else
	{
		for(pos = psGate->x1; pos <= psGate->x2; pos += 1)
		{
			psTile = mapTile(pos, psGate->y1);
			if (!fpathBlockingTile(pos, psGate->y1) &&
				TEST_TILE_VISIBLE(player, psTile))
			{
				blocked = false;
			}
		}
	}*/

	return blocked;
}


// A* findpath for the gateway system
SDWORD gwrAStarRoute(SDWORD player, UDWORD terrain,
					 SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy,
					 GATEWAY **ppsRoute)
{
	SDWORD		tileSX,tileSY,tileFX,tileFY;
	SDWORD		currDist, retval;
	GATEWAY		*psCurr, *psNew, *psRoute, *psParent, *psNext, *psNearest, *psPrev;
	SDWORD		zone, finalZone, link, finalLink;
	BOOL		add;

	*ppsRoute = NULL;
	tileSX = map_coord(sx);
	tileSY = map_coord(sy);

	tileFX = map_coord(fx);
	tileFY = map_coord(fy);

	// reset the flags on the gateways
	for(psCurr = psGateways; psCurr; psCurr=psCurr->psNext)
	{
		psCurr->flags &= ~GWR_RESET_MASK;
		psCurr->dist = SWORD_MAX;
		for(link = 0; link < (psCurr->zone1Links + psCurr->zone2Links); link++)
		{
			psCurr->psLinks[link].flags &= ~GWRL_RESET_MASK;
		}
	}

	// add all the gateways next to the start point
	zone = gwGetZone(tileSX, tileSY);
	finalZone = gwGetZone(tileFX, tileFY);
	if (zone == finalZone)
	{
		debug( LOG_GATEWAY, "Route same zone\n");
		return GWR_SAMEZONE;
	}
	if (zone == 0 || finalZone == 0)
	{
		debug( LOG_GATEWAY, "Route no zone info\n");
		return GWR_NOZONE;
	}
	psOpenList = NULL;
	for(psNew = psGateways; psNew; psNew=psNew->psNext)
	{
		add = false;
		if (psNew->zone1 == zone)
		{
			psNew->flags |= GWR_ZONE1;
			add = true;
		}
		else if (psNew->zone2 == zone)
		{
			psNew->flags |= GWR_ZONE2;
			add = true;
		}
		else if ((psNew->flags & GWR_WATERLINK) &&
				 gwZoneInEquiv(psNew->zone1, zone))
		{
			psNew->flags |= GWR_ZONE2;
			add = true;
		}

		if (gwrBlockedGateway(psNew, player, terrain))
		{
			psNew->flags |= GWR_BLOCKED;
			add = false;
		}

		if (add && gwrConsiderGateway(psNew))
		{
			psNew->dist = (SWORD)gwrEstimate(psNew, tileSX,tileSY);
			psNew->est = (SWORD)gwrEstimate(psNew, tileFX,tileFY);
			psNew->psRoute = NULL;
			gwrOpenAdd(psNew);
		}
	}

	// search for a route
	psRoute = NULL;
	psNearest = NULL;
	while (psOpenList != NULL)
	{
		psCurr = gwrOpenGet();
// 		debug( LOG_NEVER, "processing gateway (%d,%d)->(%d,%d)\n", psCurr->x1, psCurr->y1, psCurr->x2, psCurr->y2 );

		if (psCurr->zone1 == finalZone || psCurr->zone2 == finalZone ||
			((psCurr->flags & GWR_WATERLINK) && gwZoneInEquiv(psCurr->zone1, finalZone)))
		{
			// reached the target
			psRoute = psCurr;
// 			debug( LOG_NEVER, "Found route\n" );
			break;
		}

		if (psNearest == NULL || psCurr->est < psNearest->est)
		{
			psNearest = psCurr;
		}

		if (psCurr->flags & GWR_WATERLINK)
		{
			// water link gatway - route can continue to any other
			// gateway apart from the one it came from
			zone = psCurr->zone1;
			link = 0;
			finalLink = psCurr->zone1Links + psCurr->zone2Links;
		}
		else if (psCurr->flags & GWR_ZONE1)
		{
			// route came from zone1 continue with zone2
			zone = psCurr->zone2;
			link = psCurr->zone1Links;
			finalLink = psCurr->zone1Links + psCurr->zone2Links;
		}
		else
		{
			zone = psCurr->zone1;
			link = 0;
			finalLink = psCurr->zone1Links;
		}

		for (; link < finalLink; link += 1)
		{
			// skip any link that is known to be blocked
			if (psCurr->psLinks[link].flags & GWRL_BLOCKED)
			{
				continue;
			}

			psNew = psCurr->psLinks[link].psGateway;
			currDist = psCurr->dist + psCurr->psLinks[link].dist;

				// don't loop back on the route
			if ((psNew == psCurr->psRoute) ||
				// skip any gates outside the current scroll limits
				!gwrConsiderGateway(psNew) ||
				// skip any gate already visited node by a shorter route
				(psNew->dist <= currDist))
			{
				continue;
			}

			// Now insert the gateway into the appropriate list
			if ((psNew->flags & GWR_RESET_MASK) == 0)
			{
				// Not in open or closed lists - add to the open list
				psNew->dist = (SWORD)currDist;
				psNew->est = (SWORD)gwrEstimate(psNew, tileFX,tileFY);
				psNew->psRoute = psCurr;
				psNew->flags |= (psNew->zone1 == zone) ? GWR_ZONE1 : GWR_ZONE2;
				gwrOpenAdd(psNew);
// 				debug( LOG_NEVER, "new gateway (%d,%d)->(%d,%d) dist %d est %d\n", psNew->x1, psNew->y1, psNew->x2, psNew->y2, psNew->dist, psNew->est );
			}
			else if (psNew->flags & GWR_OPEN)
			{
				// already in the open list but this is shorter
// 				debug( LOG_NEVER, "new route to open gateway (%d,%d)->(%d,%d) dist %d->%d est %d\n", psNew->x1, psNew->y1, psNew->x2, psNew->y2, currDist, psNew->dist, psNew->est );
				psNew->dist = (SWORD)currDist;
				psNew->psRoute = psCurr;
			}
			else if (psNew->flags & GWR_CLOSED)
			{
				// already in the closed list but this is shorter
// 				debug( LOG_NEVER, "new route to closed gateway (%d,%d)->(%d,%d) dist %d->%d est %d\n", psNew->x1, psNew->y1, psNew->x2, psNew->y2, currDist, psNew->dist, psNew->est );
				psNew->dist = (SWORD)currDist;
				psNew->psRoute = psCurr;
				gwrOpenAdd(psNew);
			}

			psNew->flags &= ~(GWR_ZONE1 | GWR_ZONE2);
			psNew->flags |= (psNew->zone1 == zone) ? GWR_ZONE1 : GWR_ZONE2;
		}

		psCurr->flags &= ~(GWR_CLOSED|GWR_OPEN);
		psCurr->flags |= GWR_CLOSED;
	}

	retval = GWR_OK;
	if (psRoute == NULL)
	{
// 		debug( LOG_NEVER, "Partial route\n" );
		psRoute = psNearest;
		retval = GWR_NEAREST;
	}

	// get the route in the correct order if one was found
	// (it is currently in reverse order).
	if (psRoute)
	{
		psParent = NULL;
		psPrev = NULL;
		for(psCurr=psRoute; psCurr; psCurr=psNext)
		{
			psNext = psCurr->psRoute;
			if (!(psCurr->flags & GWR_WATERLINK))
			{
				psCurr->flags |= GWR_INROUTE;
				for(link = 0; link < (psCurr->zone1Links + psCurr->zone2Links); link++)
				{
					// NB link flags are reversed because the route order is being reversed
					if (psCurr->psLinks[link].psGateway == psPrev)
					{
						psCurr->psLinks[link].flags |= GWRL_CHILD;
					}
					else if	(psCurr->psLinks[link].psGateway == psNext)
					{
						psCurr->psLinks[link].flags |= GWRL_PARENT;
					}
				}
				psCurr->psRoute = psParent;
				psParent = psCurr;
			}

			// NB psPrev is not quite the same as psParent as it includes water link gateways
			psPrev = psCurr;
		}
		psRoute = psParent;

		debug( LOG_GATEWAY, "Final route:\n");
		for(psCurr=psRoute; psCurr; psCurr=psCurr->psRoute)
		{
			debug( LOG_GATEWAY, "   (%d,%d)->(%d,%d) dist %d est %d\n",
				psCurr->x1, psCurr->y1, psCurr->x2, psCurr->y2, psCurr->dist, psCurr->est);
		}

		*ppsRoute = psRoute;
	}

	if (psRoute == NULL)
	{
		retval = GWR_FAILED;
	}

	return retval;
}
