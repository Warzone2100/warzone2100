/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

	$Revision$
	$Id$
	$HeadURL$
*/
/*
 * Gateway.c
 *
 * Routing gateway code.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <string.h>

#define FREE(a) free(a); a = NULL;

#include "typedefs.h"

#define MAP_MAXWIDTH	256
#define MAP_MAXHEIGHT	256

#include "debugprint.hpp"

#include "gateway.hpp"

#include "heightmap.h"
#include "tiletypes.h"

static inline list_remove(GATEWAY*& psHead, GATEWAY* psEntry)
{
	GATEWAY* psCurr;
	GATEWAY* psPrev = NULL;

	for(psCurr = psHead; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (psCurr == psEntry)
		{
			break;
		}
		psPrev = psCurr;
	}

	ASSERT(psCurr != NULL, "list_remove: entry not found");

	if (psPrev == NULL)
	{
		psHead = psHead->psNext;
	}
	else if (psCurr != NULL)
	{
		psPrev->psNext = psCurr->psNext;
	}
}

// the list of gateways on the current map
GATEWAY		*psGateways;


// the RLE map zones for each tile
char** apRLEZones;

// the number of map zones
SDWORD		gwNumZones;

// The zone equivalence tables - shows which land zones
// border on a water zone
char*  aNumEquiv;
char** apEquivZones;


CHeightMap* g_MapData;

// link all the gateways together
BOOL gwLinkGateways();


// Initialise the gateway system
bool gwInitialise()
{
	ASSERT((psGateways == NULL,
		"gwInitialise: gatway list has not been reset"));

	psGateways = NULL;

	for(unsigned int i = 0; i < g_MapData->GetNumGateways(); ++i)
	{
		int x0, y0, x1, y1;

		if (!g_MapData->GetGateway(i, x0, y0, x1, y1)
		 || !gwNewGateway(x0,y0,x1,y1))
			return false;
	}

	// need to handle FALSE.
	if(!gwProcessMap())
		return false;

//	if (!gwLinkGateways()) return FALSE;

	return true;
}

// Shutdown the gateway system
void gwShutDown(void)
{
	GATEWAY		*psNext;

	while (psGateways != NULL)
	{
		psNext = psGateways->psNext;
		gwFreeGateway(psGateways);
		psGateways = psNext;
	}

	gwFreeZoneMap();
	gwFreeEquivTable();
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
		ASSERT((FALSE,"gwNewGateway: invalid coordinates"));
		return FALSE;
	}

	psNew = reinterpret_cast<GATEWAY*>(malloc(sizeof(GATEWAY)));
	if (!psNew)
	{
		DBERROR(("gwNewGateway: out of memory"));
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
		ASSERT((FALSE,"gwNewLinkGateway: invalid coordinates"));
		return FALSE;
	}

	psNew = reinterpret_cast<GATEWAY*>(malloc(sizeof(GATEWAY)));
	if (!psNew)
	{
		DBERROR(("gwNewGateway: out of memory"));
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

	ASSERT((numtiles != 0,
		"gwCalcZoneCenter: zone not found on map"));

	x = xsum / numtiles;
	y = ysum / numtiles;

	if (!gwFindZone(zone, x,y, px,py))
	{
		*px = x;
		*py = y;
	}
}

// add the land/water link gateways
BOOL gwGenerateLinkGates(void)
{
	SDWORD		zone, cx,cy;

	ASSERT((apEquivZones != NULL,
		"gwGenerateLinkGates: no zone equivalence table"));

	DBPRINTF(("Generating water link Gateways...."));

	for(zone=0; zone<gwNumZones; zone += 1)
	{
		if (aNumEquiv[zone] > 0)
		{
			// got a water zone that borders on land
			// find it's center
			gwCalcZoneCenter(zone, &cx,&cy);
			if (!gwNewLinkGateway(cx,cy))
			{
				return FALSE;
			}
			DBP1(("new water link gateway at (%d,%d) for zone %d\n", cx,cy, zone));
		}
	}

	DBPRINTF(("Done\n"));

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

	list_remove(psGateways, psDel);

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

	if(psDel->psLinks != NULL)
	{
		FREE(psDel->psLinks);
	}

	free(psDel);
}


// load a gateway list
BOOL gwLoadGateways(const char* pFileBuffer, UDWORD fileSize)
{
	SDWORD	numGW, x1,y1, x2,y2;
	const char* pPos;

	// get the number of gateways
	pPos = pFileBuffer;
	sscanf(pPos, "%d", &numGW);
	for (; *pPos != '\n' && pPos < (pFileBuffer + fileSize); pPos += 1)
		;
	pPos += 1;

	while ((pPos < (pFileBuffer + fileSize)) && (numGW > 0))
	{
		sscanf(pPos, "%d %d %d %d", &x1,&y1, &x2, &y2);

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
//	ASSERT((apEquivZones != NULL,
//		"gwZoneInEquiv: no zone equivalence table"));

	for(i=0; i<aNumEquiv[mainZone]; i+= 1)
	{
		if (apEquivZones[mainZone][i] == checkZone)
		{
			return TRUE;
		}
	}

	return FALSE;
}


// link all the gateways together
BOOL gwLinkGateways(void)
{
	GATEWAY		*psCurr, *psLink;
	SDWORD		x,y, gwX,gwY, zone1Links,zone2Links, link, zone, otherZone;
	SDWORD		xdiff,ydiff, zoneLinks;
	BOOL		bZone1, bAddLink;

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
	}

	// now link all the gateways together
	for(psCurr = psGateways; psCurr; psCurr = psCurr->psNext)
	{
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
			psCurr->psLinks = reinterpret_cast<GATEWAY_LINK*>(malloc(sizeof(GATEWAY_LINK) * (zone1Links+zone2Links)));
			if (psCurr->psLinks == NULL)
			{
				DBERROR(("gwLinkGateways: out of memory"));
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
					DBP0(("Linking %sgateway (%d,%d)->(%d,%d) through %s to gateway (%d,%d)->(%d,%d)\n",
						(psCurr->flags & GWR_WATERLINK) ? "water " : "",
						psCurr->x1,psCurr->y1, psCurr->x2,psCurr->y2,
						bZone1 ? "zone1" : "zone2",
						psLink->x1,psLink->y1, psLink->x2,psLink->y2));
					psCurr->psLinks[link].psGateway = psLink;
					x = (psLink->x1 + psLink->x2)/2;
					y = (psLink->y1 + psLink->y2)/2;
					xdiff = x - gwX;
					ydiff = y - gwY;
#ifdef WIN32
					psCurr->psLinks[link].dist = (SDWORD)sqrt(xdiff*xdiff + ydiff*ydiff);
#else
					psCurr->psLinks[link].dist = iSQRT(xdiff*xdiff + ydiff*ydiff);
#endif
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
	char* pCode;
	UDWORD pos = 0;
	UDWORD x = 0;

	ASSERT((Line < (UDWORD)gwMapHeight(),"gwNewZoneLine : Invalid line requested"));
	ASSERT((apRLEZones != NULL,"gwNewZoneLine : NULL Zone map"));

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

	apRLEZones = reinterpret_cast<char**>(malloc(sizeof(char*) * gwMapHeight()));
	if (apRLEZones == NULL)
	{
		DBERROR(("gwNewZoneMap: Out of memory"));
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
char* gwNewZoneLine(UDWORD Line,UDWORD Size)
{
	ASSERT((Line < (UDWORD)gwMapHeight(),"gwNewZoneLine : Invalid line requested"));
	ASSERT((apRLEZones != NULL,"gwNewZoneLine : NULL Zone map"));

	FREE(apRLEZones[Line]);

	apRLEZones[Line] = reinterpret_cast<char*>(malloc(Size));
	if (apRLEZones[Line] == NULL)
	{
		DBERROR(("gwNewZoneLine: Out of memory"));
		return NULL;
	}

	return apRLEZones[Line];
}


// Create a NULL zone map for when there is no zone info loaded
BOOL gwCreateNULLZoneMap(void)
{
	SDWORD	y;
	char* pBuf;

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
		for(i = 0; i < gwMapHeight(); i++)
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
	SDWORD		xPos, zone, rlePos;
	ASSERT(((x >= 0) && (x < gwMapWidth()) && (y >= 0) && (y < gwMapHeight()),
		"gwGetZone: invalid coordinates"));

	rlePos = 0;
	xPos = 0;
	do
	{
		xPos += *(apRLEZones[y] + rlePos);
		zone  = *(apRLEZones[y] + rlePos + 1);
		rlePos += 2;
	} while (xPos <= x); // xPos is where the next zone starts

	return zone;
}


/******************************************************************************************************/
/*                   Zone equivalence data access functions                                           */


// create an empty equivalence table
BOOL gwNewEquivTable(SDWORD numZones)
{
	SDWORD	i;

	ASSERT((numZones < UBYTE_MAX,
		"gwNewEquivTable: invalid number of zones"));

	gwNumZones = numZones;
	aNumEquiv = reinterpret_cast<char*>(malloc(sizeof(char) * numZones));
	if (aNumEquiv == NULL)
	{
		DBERROR(("gwNewEquivTable: out of memory"));
		return FALSE;
	}
	for(i=0; i<numZones; i+=1)
	{
		aNumEquiv[i] = 0;
	}

	apEquivZones = reinterpret_cast<char**>(malloc(sizeof(char*) * numZones));
	if (apEquivZones == NULL)
	{
		DBERROR(("gwNewEquivTable: out of memory"));
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
		FREE(aNumEquiv);
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
}


// set the zone equivalence for a zone
BOOL gwSetZoneEquiv(SDWORD zone, SDWORD numEquiv, UBYTE *pEquiv)
{
	SDWORD i;

	ASSERT((aNumEquiv != NULL && apEquivZones != NULL,
		"gwSetZoneEquiv: equivalence arrays not initialised"));
	ASSERT((zone < gwNumZones,
		"gwSetZoneEquiv: invalid zone"));
	ASSERT((numEquiv <= gwNumZones,
		"gwSetZoneEquiv: invalid number of zone equivalents"));

	apEquivZones[zone] = reinterpret_cast<char*>(malloc(sizeof(char) * numEquiv));
	if (apEquivZones[zone] == NULL)
	{
		DBERROR(("gwSetZoneEquiv: out of memory"));
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
	return g_MapData->GetMapWidth();
}

SDWORD gwMapHeight(void)
{
	return g_MapData->GetMapHeight();
}


// set the gateway flag on a tile
void gwSetGatewayFlag(SDWORD x, SDWORD y)
{
	g_MapData->SetTileGateway(x, y, TRUE);
}

// clear the gateway flag on a tile
void gwClearGatewayFlag(SDWORD x, SDWORD y)
{
	g_MapData->SetTileGateway(x, y, FALSE);
}

// check whether a gateway is on water
BOOL gwTileIsWater(UDWORD x, UDWORD y)
{
	return g_MapData->GetTileType(x, y) == TF_TYPEWATER;
}


// previous interface functions
bool giWriteZones(FILE *Stream)
{
	ZoneMapHeader Header;

	Header.version = 2;
	Header.numStrips = g_MapData->GetMapHeight();
	Header.numZones = gwNumZones;

	fwrite(&Header,sizeof(Header),1,Stream);

	if(apRLEZones) {
		UWORD Size = 0;
		for(DWORD y=0; y < g_MapData->GetMapHeight(); y++) {
			char* pCode = apRLEZones[y];
			UWORD pos = 0, x =0;

			while (x < g_MapData->GetMapWidth())
			{
				x += pCode[pos];
				pos += 2;
			}

			Size += pos;

			fwrite(&pos, sizeof(pos), 1, Stream);		// Size of this strip.
			fwrite(apRLEZones[y], pos, 1, Stream);		// The strip.
		}

		DebugPrint("Total zone map size %d\n",Size);

		Size = 0;
		for(unsigned int i = 0; i < gwNumZones; ++i)
		{
			Size += aNumEquiv[i] + sizeof(char);

			fwrite(&aNumEquiv[i], sizeof(char), 1, Stream);
			fwrite(apEquivZones[i], aNumEquiv[i], 1, Stream);
		}

		DebugPrint("Total equivalence tables size %d\n",Size);

		return true;
	}

	return false;
}

bool giCreateZones()
{
	giClearGatewayFlags();

	// Check for and act on returning false;
	return gwInitialise();
}

void giDeleteZones()
{
	gwShutDown();
}

void giUpdateMapZoneIDs()
{
	for (unsigned int z = 0; z < g_MapData->GetMapHeight(); ++z)
	{
		for (unsigned int x = 0; x < g_MapData->GetMapWidth(); ++x)
		{
			g_MapData->SetMapZoneID(x, z, gwGetZone(x, z));
		}
	}
}

void giClearGatewayFlags()
{
	for (unsigned int z = 0; z < g_MapData->GetMapHeight(); ++z)
	{
		for (unsigned int x = 0; x < g_MapData->GetMapWidth(); ++x)
		{
			g_MapData->SetTileGateway(x, z, false);
		}
	}
}

void giSetMapData(CHeightMap *MapData)
{
	g_MapData = MapData;
}
