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
 * Gateway.c
 *
 * Routing gateway code.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lib/framework/frame.h"
#include "map.h"
#include "astar.h"
#include "fpath.h"
#include "wrappers.h"

#include "gateway.h"
#include "lib/framework/listmacs.h"

// the list of gateways on the current map
GATEWAY		*psGateways;

// the RLE map zones for each tile
UBYTE		**apRLEZones;

// the number of map zones
SDWORD		gwNumZones;

// The zone equivalence tables - shows which land zones
// border on a water zone
UBYTE		*aNumEquiv;
UBYTE		**apEquivZones;

// note which zones have a gateway to them and can therefore be reached
UBYTE		*aZoneReachable;


// Initialise the gateway system
BOOL gwInitialise(void)
{
	ASSERT( psGateways == NULL, "gwInitialise: gateway list has not been reset" );

	psGateways = NULL;

	return TRUE;
}


// Shutdown the gateway system
void gwShutDown(void)
{
	GATEWAY *psNext;

	while (psGateways != NULL)
	{
		psNext = psGateways->psNext;
		gwFreeGateway(psGateways);
		psGateways = psNext;
	}

	gwFreeZoneMap();
	gwFreeEquivTable();

	if (aZoneReachable != NULL)
	{
		free(aZoneReachable);
		aZoneReachable = NULL;
	}
}


// Add a gateway to the system
BOOL gwNewGateway(SDWORD x1, SDWORD y1, SDWORD x2, SDWORD y2)
{
	GATEWAY		*psNew;
	SDWORD		pos, temp;

	if ((x1 < 0) || (x1 >= gwMapWidth())  ||
		(y1 < 0) || (y1 >= gwMapHeight()) ||
		(x2 < 0) || (x2 >= gwMapWidth())  ||
		(y2 < 0) || (y2 >= gwMapHeight()) ||
		((x1 != x2) && (y1 != y2)))
	{
		ASSERT( FALSE,"gwNewGateway: invalid coordinates" );
		return FALSE;
	}

	psNew = (GATEWAY*)malloc(sizeof(GATEWAY));
	if (!psNew)
	{
		debug( LOG_ERROR, "gwNewGateway: out of memory" );
		abort();
		return FALSE;
	}

	// make sure the first coordinate is always the smallest
	if (x2 < x1)
	{
		// y is the same, swap x
		temp = x2;
		x2 = x1;
		x1 = temp;
	}
	else if (y2 < y1)
	{
		// x is the same, swap y
		temp = y2;
		y2 = y1;
		y1 = temp;
	}

	// initialise the gateway
	psNew->x1 = (UBYTE)x1;
	psNew->y1 = (UBYTE)y1;
	psNew->x2 = (UBYTE)x2;
	psNew->y2 = (UBYTE)y2;
	psNew->zone1 = 0;
	psNew->zone2 = 0;
	psNew->psLinks = NULL;
	psNew->flags = 0;

	// add the gateway to the list
	psNew->psNext = psGateways;
	psGateways = psNew;

	// set the map flags
	if (x1 == x2)
	{
		// vertical gateway
		for(pos = y1; pos <= y2; pos++)
		{
			gwSetGatewayFlag(x1, pos);
		}
	}
	else
	{
		// horizontal gateway
		for(pos = x1; pos <= x2; pos++)
		{
			gwSetGatewayFlag(pos, y1);
		}
	}

	return TRUE;
}


// Add a land/water link gateway to the system
BOOL gwNewLinkGateway(SDWORD x, SDWORD y)
{
	GATEWAY		*psNew;

	if ((x < 0) || (x >= gwMapWidth())  ||
		(y < 0) || (y >= gwMapHeight()))
	{
		ASSERT( FALSE,"gwNewLinkGateway: invalid coordinates" );
		return FALSE;
	}

	psNew = (GATEWAY*)malloc(sizeof(GATEWAY));
	if (!psNew)
	{
		debug( LOG_ERROR, "gwNewGateway: out of memory" );
		abort();
		return FALSE;
	}

	// initialise the gateway
	psNew->x1 = (UBYTE)x;
	psNew->y1 = (UBYTE)y;
	psNew->x2 = (UBYTE)x;
	psNew->y2 = (UBYTE)y;
	psNew->zone1 = 0;
	psNew->zone2 = 0;
	psNew->psLinks = NULL;
	psNew->flags = GWR_WATERLINK;

	// add the gateway to the list
	psNew->psNext = psGateways;
	psGateways = psNew;

	return TRUE;
}


static BOOL gwBlockingTile(SDWORD x,SDWORD y)
{
	MAPTILE	*psTile;

	if (x <1 || y < 1 || x >= (SDWORD)mapWidth-1 || y >= (SDWORD)mapHeight-1)
	{
		// coords off map - auto blocking tile
		return TRUE;
	}

	psTile = mapTile((UDWORD)x, (UDWORD)y);
	if (terrainType(psTile) == TER_CLIFFFACE)
	{
		return TRUE;
	}

	return FALSE;
}


// scan for a particular zone on the map
// given a start point
static BOOL gwFindZone(SDWORD zone, SDWORD cx, SDWORD cy,
					   SDWORD *px, SDWORD *py)
{
	SDWORD	x,y, dist, maxDist;

	maxDist = gwMapWidth() > gwMapHeight() ? gwMapWidth() : gwMapHeight();

	for(dist = 0; dist < maxDist; dist += 1)
	{
		// scan accross the top
		y = cy - dist;
		for(x = cx - dist; x <= cx + dist; x += 1)
		{
			if (x >= 0 && x < gwMapWidth() &&
				y >= 0 && y < gwMapHeight() &&
				gwGetZone(x,y) == zone)
			{
				*px = x;
				*py = y;
				return TRUE;
			}
		}

		// scan down the left
		x = cx - dist;
		for(y = cy - dist; y <= cy + dist; y += 1)
		{
			if (x >= 0 && x < gwMapWidth() &&
				y >= 0 && y < gwMapHeight() &&
				gwGetZone(x,y) == zone)
			{
				*px = x;
				*py = y;
				return TRUE;
			}
		}

		// scan down the right
		x = cx + dist;
		for(y = cy - dist; y <= cy + dist; y += 1)
		{
			if (x >= 0 && x < gwMapWidth() &&
				y >= 0 && y < gwMapHeight() &&
				gwGetZone(x,y) == zone)
			{
				*px = x;
				*py = y;
				return TRUE;
			}
		}

		// scan accross the bottom
		y = cy + dist;
		for(x = cx - dist; x <= cx + dist; x += 1)
		{
			if (x >= 0 && x < gwMapWidth() &&
				y >= 0 && y < gwMapHeight() &&
				gwGetZone(x,y) == zone)
			{
				*px = x;
				*py = y;
				return TRUE;
			}
		}
	}

	return FALSE;
}


// find a rough center position for a zone
static void gwCalcZoneCenter(SDWORD zone, SDWORD *px, SDWORD *py)
{
	SDWORD		xsum,ysum, numtiles;
	SDWORD		x,y;

	xsum = ysum = numtiles = 0;
	for(y=0; y<gwMapHeight(); y+= 1)
	{
		for(x=0; x<gwMapWidth(); x+= 1)
		{
			if (gwGetZone(x,y) == zone)
			{
				xsum += x;
				ysum += y;
				numtiles += 1;
			}
		}
	}

	ASSERT( numtiles != 0,
		"gwCalcZoneCenter: zone not found on map" );

	x = xsum / numtiles;
	y = ysum / numtiles;

	if (!gwFindZone(zone, x,y, px,py))
	{
		*px = x;
		*py = y;
	}
}


// check all the zones are of reasonable sizes
void gwCheckZoneSizes(void)
{
	SDWORD		zone, xsum,ysum, numtiles, inzone;
	SDWORD		x,y, cx,cy;

	for(zone=1; zone < UBYTE_MAX; zone += 1)
	{
		xsum = ysum = numtiles = inzone = 0;
		for(y=0; y<gwMapHeight(); y+= 1)
		{
			for(x=0; x<gwMapWidth(); x+= 1)
			{
				if (gwGetZone(x,y) == zone)
				{
					xsum += x;
					ysum += y;
					numtiles += 1;
					if (!gwBlockingTile(x,y))
					{
						inzone += 1;
					}
				}
			}
		}

		if (numtiles > 0)
		{
			x = xsum / numtiles;
			y = ysum / numtiles;

			if (!gwFindZone(zone, x,y, &cx,&cy))
			{
				cx = x;
				cy = y;
			}

			if (inzone > FPATH_LOOP_LIMIT)
			{
				debug(LOG_ERROR, "gwCheckZoneSizes: warning zone %d at (%d,%d) is too large %d tiles (max %d)", zone, cx, cy, inzone, FPATH_LOOP_LIMIT);
			}
		}
	}
}


// add the land/water link gateways
BOOL gwGenerateLinkGates(void)
{
	SDWORD		zone, cx,cy;

	ASSERT( apEquivZones != NULL,
		"gwGenerateLinkGates: no zone equivalence table" );

	debug( LOG_NEVER, "Generating water link Gateways...." );

	for(zone=1; zone<gwNumZones; zone += 1)
	{

		if (aNumEquiv[zone] > 0)
		{
			LOADBARCALLBACK();	//			loadingScreenCallback();

			// got a water zone that borders on land
			// find it's center
			gwCalcZoneCenter(zone, &cx,&cy);
			if (!gwNewLinkGateway(cx,cy))
			{
				return FALSE;
			}
			debug(LOG_GATEWAY, "new water link gateway at (%d,%d) for zone %d", cx, cy, zone);
		}
	}

	debug( LOG_NEVER, "Done\n" );

	return TRUE;
}


// Return the number of gateways.
UDWORD gwNumGateways(void)
{
	GATEWAY		*psCurr;
	UDWORD NumGateways = 0;

	for(psCurr = psGateways; psCurr; psCurr = psCurr->psNext)
	{
		NumGateways++;
	}

	return NumGateways;
}


GATEWAY *gwGetGateways(void)
{
	return psGateways;
}


// Release a gateway
void gwFreeGateway(GATEWAY *psDel)
{
	SDWORD	pos;

	LIST_REMOVE(psGateways, psDel, GATEWAY);

	if (psMapTiles) // this lines fixes the bug where we were closing the gateways after freeing the map
	{
		// clear the map flags
		if (psDel->x1 == psDel->x2)
		{
			// vertical gateway
			for(pos = psDel->y1; pos <= psDel->y2; pos++)
			{
				gwClearGatewayFlag(psDel->x1, pos);
			}
		}
		else
		{
			// horizontal gateway
			for(pos = psDel->x1; pos <= psDel->x2; pos++)
			{
				gwClearGatewayFlag(pos, psDel->y1);
			}
		}

	}

	if (psDel->psLinks != NULL)
	{
		free(psDel->psLinks);
	}
	free(psDel);
}


// load a gateway list
BOOL gwLoadGateways(char *pFileBuffer, UDWORD fileSize)
{
	SDWORD	numGW, x1,y1, x2,y2;
	char	*pPos;

	// get the number of gateways
	pPos = pFileBuffer;
	sscanf((char *)pPos, "%d", &numGW);
	for (; *pPos != '\n' && pPos < (pFileBuffer + fileSize); pPos += 1)
		;
	pPos += 1;

	while ((pPos < (pFileBuffer + fileSize)) && (numGW > 0))
	{
		sscanf((char *)pPos, "%d %d %d %d", &x1,&y1, &x2, &y2);

		if (!gwNewGateway(x1,y1, x2,y2))
		{
			return FALSE;
		}

		for (; *pPos != '\n' && pPos < (pFileBuffer + fileSize); pPos += 1)
			;
		pPos += 1;
		numGW -= 1;
	}

	return TRUE;
}


// check if a zone is in the equivalence table for a water zone
BOOL gwZoneInEquiv(SDWORD mainZone, SDWORD checkZone)
{
	SDWORD i;

	if (apEquivZones == NULL)
	{
		return FALSE;
	}

	for(i=0; i<aNumEquiv[mainZone]; i+= 1)
	{
		if (apEquivZones[mainZone][i] == checkZone)
		{
			return TRUE;
		}
	}

	return FALSE;
}

// find a route between two gateways and return
// its length
static SDWORD gwRouteLength(GATEWAY *psStart, GATEWAY *psEnd)
{
	SDWORD			ret, sx,sy, ex,ey, xdiff,ydiff, i;
	ASTAR_ROUTE		sRoute;
	SDWORD			routeMode, dist;
#ifdef DEBUG
	SDWORD			zone;
#endif

	fpathBlockingTile = gwBlockingTile;

	sx = (psStart->x1 + psStart->x2)/2;
	sy = (psStart->y1 + psStart->y2)/2;
	ex = (psEnd->x1 + psEnd->x2)/2;
	ey = (psEnd->y1 + psEnd->y2)/2;

	// force the router to finish a route
	routeMode = ASR_NEWROUTE;
	sRoute.asPos[0].x = -1;
	sRoute.asPos[0].y = -1;
	do
	{
		astarResetCounters();
		sRoute.numPoints = 0;
		ret = fpathAStarRoute(routeMode, &sRoute,
					world_coord(sx), world_coord(sy),
					world_coord(ex), world_coord(ey));
		if (ret == ASR_PARTIAL)
		{
			routeMode = ASR_CONTINUE;
		}
	} while (ret == ASR_PARTIAL);

	ASSERT( ret != ASR_FAILED,
		"gwRouteLength: no route between gateways at (%d,%d) and (%d,%d)",
		sx,sy, ex,ey );

#ifdef DEBUG
	if (ret == ASR_NEAREST)
	{
		zone = (psStart->zone1 == psEnd->zone1) || (psStart->zone1 == psEnd->zone2) ? psStart->zone1 : psStart->zone2;
		debug( LOG_ERROR, "gwRouteLength: warning only partial route between gateways at %s(%d,%d) and %s(%d,%d) zone %d\n", psStart->flags & GWR_WATERLINK ? "W" : "", sx,sy, psStart->flags & GWR_WATERLINK ? "W" : "", ex, ey, zone );
	}
#endif

	// calculate the length of the route
	dist = 0;
	for(i=0; i < sRoute.numPoints; i+= 1)
	{
		xdiff = sx - sRoute.asPos[i].x;
		ydiff = sy - sRoute.asPos[i].y;
		dist += (SDWORD)sqrtf(xdiff*xdiff + ydiff*ydiff);
		sx = sRoute.asPos[i].x;
		sy = sRoute.asPos[i].y;
	}
	xdiff = sx - ex;
	ydiff = sy - ey;
	dist += (SDWORD)sqrtf(xdiff*xdiff + ydiff*ydiff);

	fpathBlockingTile = fpathGroundBlockingTile;

	return dist;
}


// check that the initial flood fill tiles are not on a blocking tile
static BOOL gwCheckFloodTiles(GATEWAY *psGate)
{
	SDWORD	floodX,floodY;

	// first zone is left/above
	if (psGate->x1 == psGate->x2)
	{
		// vertical - go left
		floodX = psGate->x1 - 1;
		floodY = (psGate->y2 - psGate->y1)/2 + psGate->y1;
	}
	else
	{
		// horizontal - go above
		floodX = (psGate->x2 - psGate->x1)/2 + psGate->x1;
		floodY = psGate->y1 - 1;
	}

	if (gwBlockingTile(floodX,floodY))
	{
		return FALSE;
	}

	// second zone is right/below
	if (psGate->x1 == psGate->x2)
	{
		// vertical - go right
		floodX = psGate->x1 + 1;
		floodY = (psGate->y2 - psGate->y1)/2 + psGate->y1;
	}
	else
	{
		// horizontal - go below
		floodX = (psGate->x2 - psGate->x1)/2 + psGate->x1;
		floodY = psGate->y1 + 1;
	}

	if (gwBlockingTile(floodX,floodY))
	{
		return FALSE;
	}

	return TRUE;
}


// link all the gateways together
BOOL gwLinkGateways(void)
{
	GATEWAY		*psCurr, *psLink;
	SDWORD		x,y, gwX,gwY, zone1Links,zone2Links, link, zone, otherZone;
	SDWORD		zoneLinks;
	BOOL		bZone1, bAddLink;


	// note which zones have a gateway
	aZoneReachable = (UBYTE*)malloc( sizeof(UBYTE) * gwNumZones );
	if (aZoneReachable == NULL)
	{
		debug( LOG_ERROR, "gwLinkGateways: out of memory" );
		abort();
		return FALSE;
	}
	memset(aZoneReachable, 0, sizeof(UBYTE) * gwNumZones);

	// initialise the zones for the gateways
	for(psCurr = psGateways; psCurr; psCurr = psCurr->psNext)
	{
		// a gateway is always in it's own zone1
		psCurr->zone1 = (UBYTE)gwGetZone(psCurr->x1,psCurr->y1);

		if (psCurr->flags & GWR_WATERLINK)
		{
			// a water link gateway is only in one zone
			x = psCurr->x1;
			y = psCurr->y1;
		}
		else if (psCurr->x1 == psCurr->x2)
		{
			// vertical - go right
			x = psCurr->x1 + 1;
			y = (psCurr->y2 - psCurr->y1)/2 + psCurr->y1;
		}
		else
		{
			// horizontal - go below
			x = (psCurr->x2 - psCurr->x1)/2 + psCurr->x1;
			y = psCurr->y1 + 1;
		}
		psCurr->zone2 = (UBYTE)gwGetZone(x,y);

		ASSERT( (psCurr->flags & GWR_WATERLINK) || gwCheckFloodTiles(psCurr),
			"gwLinkGateways: Gateway at (%d,%d)->(%d,%d) is too close to a blocking tile. Zones %d, %d",
			psCurr->x1,psCurr->y1, psCurr->x2,psCurr->y2,
			psCurr->zone1, psCurr->zone2 );

		aZoneReachable[psCurr->zone1] = TRUE;
		aZoneReachable[psCurr->zone2] = TRUE;
	}

	// now link all the gateways together
	for(psCurr = psGateways; psCurr; psCurr = psCurr->psNext)
	{
		LOADBARCALLBACK();	//		loadingScreenCallback();

		gwX = (psCurr->x1 + psCurr->x2)/2;
		gwY = (psCurr->y1 + psCurr->y2)/2;

		// count the number of links
		zone1Links = 0;
		zone2Links = 0;
		for(psLink=psGateways; psLink; psLink=psLink->psNext)
		{
			if (psLink == psCurr)
			{
				// don't link a gateway to itself
				continue;
			}
			if ((psLink->zone1 == psCurr->zone1) || (psLink->zone2 == psCurr->zone1) ||
				((psLink->flags & GWR_WATERLINK) &&
				 gwZoneInEquiv(psLink->zone1, psCurr->zone1) &&
				 !gwZoneInEquiv(psLink->zone1, psCurr->zone2) ))
			{
				zone1Links += 1;
			}
			if (psCurr->flags & GWR_WATERLINK)
			{
				// calculating links for a water link gateway
				if (gwZoneInEquiv(psCurr->zone1, psLink->zone1) ||
					gwZoneInEquiv(psCurr->zone1, psLink->zone2))
				{
					zone2Links += 1;
				}
			}
			else if ((psLink->zone1 == psCurr->zone2) || (psLink->zone2 == psCurr->zone2) ||
				((psLink->flags & GWR_WATERLINK) &&
				 gwZoneInEquiv(psLink->zone1, psCurr->zone2) &&
				 !gwZoneInEquiv(psLink->zone1, psCurr->zone1) ))
			{
				zone2Links += 1;
			}
		}
		if (zone1Links+zone2Links > 0)
		{
			psCurr->psLinks = (GATEWAY_LINK*)malloc(sizeof(GATEWAY_LINK) * (zone1Links+zone2Links));
			if (psCurr->psLinks == NULL)
			{
				debug( LOG_ERROR, "gwLinkGateways: out of memory" );
				abort();
				return FALSE;
			}
		}
		else
		{
			psCurr->psLinks = NULL;
		}
		psCurr->zone1Links = (UBYTE)zone1Links;
		psCurr->zone2Links = (UBYTE)zone2Links;

		// generate the links starting with all those through zone1
		link = 0;
		zone = psCurr->zone1;
		otherZone = psCurr->zone2;
		zoneLinks = zone1Links;
		bZone1 = TRUE;
		while (link < (zone1Links + zone2Links))
		{
			for(psLink=psGateways; psLink && (link < zoneLinks); psLink=psLink->psNext)
			{
				if (psLink == psCurr)
				{
					// don't link a gateway to itself
					continue;
				}
				bAddLink = FALSE;
				if (!bZone1 && (psCurr->flags & GWR_WATERLINK))
				{
					// calculating links for a water link gateway
					if (gwZoneInEquiv(psCurr->zone1, psLink->zone1) ||
						gwZoneInEquiv(psCurr->zone1, psLink->zone2))
					{
						bAddLink = TRUE;
					}
				}
				else if ((psLink->zone1 == zone) || (psLink->zone2 == zone) ||
						 ((psLink->flags & GWR_WATERLINK) &&
						  gwZoneInEquiv(psLink->zone1, zone) &&
						  !gwZoneInEquiv(psLink->zone1, otherZone) ))
				{
					bAddLink = TRUE;
				}

				if (bAddLink)
				{
// 					debug( LOG_NEVER, "Linking %sgateway (%d,%d)->(%d,%d) through %s to gateway (%d,%d)->(%d,%d)\n", (psCurr->flags & GWR_WATERLINK) ? "water " : "", psCurr->x1,psCurr->y1, psCurr->x2, psCurr->y2, bZone1 ? "zone1" : "zone2", psLink->x1, psLink->y1, psLink->x2, psLink->y2 );
					psCurr->psLinks[link].psGateway = psLink;
					psCurr->psLinks[link].flags = 0;

					psCurr->psLinks[link].dist = (SWORD)gwRouteLength(psCurr, psLink);

					link += 1;
				}
			}

			// found all the links to zone1, now do it for zone2
			zone = psCurr->zone2;
			otherZone = psCurr->zone1;
			zoneLinks = zone1Links + zone2Links;
			bZone1 = FALSE;
		}
	}

	return TRUE;
}


/******************************************************************************************************/
/*                            RLE Zone data access functions                                          */


// Get number of zone lines.
UDWORD gwNumZoneLines(void)
{
	return gwMapHeight();
}


// Get the size of a zone line.
UDWORD gwZoneLineSize(UDWORD Line)
{
	UBYTE *pCode;
	UDWORD pos = 0;
	UDWORD x = 0;

	ASSERT( Line < (UDWORD)gwMapHeight(),"gwNewZoneLine : Invalid line requested" );
	ASSERT( apRLEZones != NULL,"gwNewZoneLine : NULL Zone map" );

	pCode = apRLEZones[Line];

	while (x < (UDWORD)gwMapWidth()) {
		x += pCode[pos];
		pos += 2;
	}

	return pos;
}


// Create a new empty zone map but don't allocate the actual zones yet.
//
BOOL gwNewZoneMap(void)
{
	UWORD i;

	if (apRLEZones != NULL)
	{
		gwFreeZoneMap();
	}

	apRLEZones = (UBYTE**)malloc(sizeof(UBYTE *) * gwMapHeight());
	if (apRLEZones == NULL)
	{
		debug( LOG_ERROR, "gwNewZoneMap: Out of memory" );
		abort();
		return FALSE;
	}

	for(i=0; i< gwMapHeight(); i++)
	{
		apRLEZones[i] = NULL;
	}

	return TRUE;
}

// Create a new empty zone map line in the zone map.
//
UBYTE * gwNewZoneLine(UDWORD Line,UDWORD Size)
{
	ASSERT( Line < (UDWORD)gwMapHeight(),"gwNewZoneLine : Invalid line requested" );
	ASSERT( apRLEZones != NULL,"gwNewZoneLine : NULL Zone map" );

	if(apRLEZones[Line] != NULL) {
		free(apRLEZones[Line]);
	}

	apRLEZones[Line] = (UBYTE*)malloc(Size);
	if (apRLEZones[Line] == NULL)
	{
		debug( LOG_ERROR, "gwNewZoneLine: Out of memory" );
		abort();
		return NULL;
	}

	return apRLEZones[Line];
}


// Create a NULL zone map for when there is no zone info loaded
BOOL gwCreateNULLZoneMap(void)
{
	SDWORD	y;
	UBYTE	*pBuf;

	if (!gwNewZoneMap())
	{
		return FALSE;
	}

	for(y=0; y<gwMapHeight(); y++)
	{
		pBuf = gwNewZoneLine(y, 2);
		if (!pBuf)
		{
			return FALSE;
		}
		pBuf[0] = (UBYTE)gwMapWidth();
		pBuf[1] = 0;
	}

	return TRUE;
}


// release the RLE Zone map
void gwFreeZoneMap(void)
{
	SDWORD	i;

	if (apRLEZones)
	{
		for(i=0; i<gwMapHeight(); i++)
		{
			free(apRLEZones[i]);
		}
		free(apRLEZones);
		apRLEZones = NULL;
	}
}


// Look up the zone for a coordinate
SDWORD gwGetZone(SDWORD x, SDWORD y)
{
	ASSERT( x >= 0 && x < gwMapWidth() && y >= 0 && y < gwMapHeight(), "gwGetZone: invalid coordinates" );

	if ( x >= 0 && x < gwMapWidth() && y >= 0 && y < gwMapHeight() )
	{
		SDWORD xPos = 0, zone = 0, rlePos = 0;

		do
		{
			xPos += apRLEZones[y][rlePos];
			zone  = apRLEZones[y][rlePos + 1];
			rlePos += 2;
		} while (xPos <= x); // xPos is where the next zone starts

		return zone;
	}
	else
	{
		return 0;
	}
}


/******************************************************************************************************/
/*                   Zone equivalence data access functions                                           */


// create an empty equivalence table
BOOL gwNewEquivTable(SDWORD numZones)
{
	SDWORD	i;

	ASSERT( numZones < UBYTE_MAX,
		"gwNewEquivTable: invalid number of zones" );

	gwNumZones = numZones;
	aNumEquiv = (UBYTE*)malloc(sizeof(UBYTE) * numZones);
	if (aNumEquiv == NULL)
	{
		debug( LOG_ERROR, "gwNewEquivTable: out of memory" );
		abort();
		return FALSE;
	}
	for(i=0; i<numZones; i+=1)
	{
		aNumEquiv[i] = 0;
	}

	apEquivZones = (UBYTE**)malloc(sizeof(UBYTE *) * numZones);
	if (apEquivZones == NULL)
	{
		debug( LOG_ERROR, "gwNewEquivTable: out of memory" );
		abort();
		return FALSE;
	}
	for(i=0; i<numZones; i+=1)
	{
		apEquivZones[i] = NULL;
	}

	return TRUE;
}

// release the equivalence table
void gwFreeEquivTable(void)
{
	SDWORD i;

	if (aNumEquiv)
	{
		free(aNumEquiv);
		aNumEquiv = NULL;
	}

	if (apEquivZones)
	{
		for(i=0; i<gwNumZones; i+=1)
		{
			if (apEquivZones[i])
			{
				free(apEquivZones[i]);
			}
		}
		free(apEquivZones);
		apEquivZones = NULL;
	}
	gwNumZones = 0;
}


// set the zone equivalence for a zone
BOOL gwSetZoneEquiv(SDWORD zone, SDWORD numEquiv, UBYTE *pEquiv)
{
	SDWORD i;

	ASSERT( aNumEquiv != NULL && apEquivZones != NULL,
		"gwSetZoneEquiv: equivalence arrays not initialised" );
	ASSERT( zone < gwNumZones,
		"gwSetZoneEquiv: invalid zone" );
	ASSERT( numEquiv <= gwNumZones,
		"gwSetZoneEquiv: invalid number of zone equivalents" );

	apEquivZones[zone] = (UBYTE*)malloc(sizeof(UBYTE) * numEquiv);
	if (apEquivZones[zone] == NULL)
	{
		debug( LOG_ERROR, "gwSetZoneEquiv: out of memory" );
		abort();
		return FALSE;
	}

	aNumEquiv[zone] = (UBYTE)numEquiv;
	for(i=0; i<numEquiv; i+=1)
	{
		apEquivZones[zone][i] = pEquiv[i];
	}

	return TRUE;
}


/******************************************************************************************************/
/*                   Gateway data access functions                                                    */

// get the size of the map
SDWORD gwMapWidth(void)
{
	return (SDWORD)mapWidth;
}

SDWORD gwMapHeight(void)
{
	return (SDWORD)mapHeight;
}

// set the gateway flag on a tile
void gwSetGatewayFlag(SDWORD x, SDWORD y)
{
	mapTile((UDWORD)x,(UDWORD)y)->tileInfoBits |= BITS_GATEWAY;
}

// clear the gateway flag on a tile
void gwClearGatewayFlag(SDWORD x, SDWORD y)
{
	mapTile((UDWORD)x,(UDWORD)y)->tileInfoBits &= ~BITS_GATEWAY;
}

// check whether a tile is water
BOOL gwTileIsWater(UDWORD x, UDWORD y)
{
	return terrainType(mapTile(x ,y)) == TER_WATER;
}

// see if a zone is reachable
BOOL gwZoneReachable(SDWORD zone)
{
	ASSERT( zone >= 0 && zone < gwNumZones,
		"gwZoneReachable: invalid zone" );

	return aZoneReachable[zone];
}
