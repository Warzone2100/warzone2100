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
/*
 * Gateway.c
 *
 * Routing gateway code.
 *
 */

#include "lib/framework/frame.h"

#include "map.h"
#include "wrappers.h"

#include "gateway.h"
#include "game_world.h"

// Prototypes
static void gwFreeGateway(WorldMapState& mapState, GATEWAY *psDel);

/******************************************************************************************************/
/*                   Gateway data access functions                                                    */

// get the size of the map
static SDWORD gwMapWidth(const WorldMapState& mapState)
{
	return (SDWORD)mapState.width;
}

static SDWORD gwMapHeight(const WorldMapState& mapState)
{
	return (SDWORD)mapState.height;
}

// set the gateway flag on a tile
static void gwSetGatewayFlag(WorldMapState& mapState, SDWORD x, SDWORD y)
{
	mapTile(mapState, (UDWORD)x, (UDWORD)y)->tileInfoBits |= BITS_GATEWAY;
}

// clear the gateway flag on a tile
static void gwClearGatewayFlag(WorldMapState& mapState, SDWORD x, SDWORD y)
{
	mapTile(mapState, (UDWORD)x, (UDWORD)y)->tileInfoBits &= ~BITS_GATEWAY;
}


/******************************************************************************************************/
/*                   Gateway functions                                                                */

// Initialise the gateway system
bool gwInitialise(WorldMapState& mapState)
{
	mapState.gateways.clear();
	return true;
}

// Shutdown the gateway system
void gwShutDown(WorldMapState& mapState)
{
	for (auto psGateway : mapState.gateways)
	{
		gwFreeGateway(mapState, psGateway);
	}
	mapState.gateways.clear();
}

// Add a gateway to the system
bool gwNewGateway(WorldMapState& mapState, SDWORD x1, SDWORD y1, SDWORD x2, SDWORD y2)
{
	GATEWAY		*psNew;
	SDWORD		pos, temp;

	ASSERT_OR_RETURN(false, x1 >= 0 && x1 < gwMapWidth(mapState) && y1 >= 0 && y1 < gwMapHeight(mapState)
	                 && x2 >= 0 && x2 < gwMapWidth(mapState) && y2 >= 0 && y2 < gwMapHeight(mapState)
	                 && (x1 == x2 || y1 == y2), "Invalid gateway coordinates (%d, %d, %d, %d)",
	                 x1, y1, x2, y2);
	psNew = (GATEWAY *)malloc(sizeof(GATEWAY));

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

	// Initialise the gateway, correct out-of-map gateways
	psNew->x1 = MAX(3, x1);
	psNew->y1 = MAX(3, y1);
	psNew->x2 = MIN(x2, mapState.width - 4);
	psNew->y2 = MIN(y2, mapState.height - 4);

	// add the gateway to the list
	mapState.gateways.push_back(psNew);

	// set the map flags
	if (psNew->x1 == psNew->x2)
	{
		// vertical gateway
		for (pos = psNew->y1; pos <= psNew->y2; pos++)
		{
			gwSetGatewayFlag(mapState, psNew->x1, pos);
		}
	}
	else
	{
		// horizontal gateway
		for (pos = psNew->x1; pos <= psNew->x2; pos++)
		{
			gwSetGatewayFlag(mapState, pos, psNew->y1);
		}
	}

	return true;
}

// Return the number of gateways.
size_t gwNumGateways(const WorldMapState& mapState)
{
	return mapState.gateways.size();
}

GATEWAY_LIST &gwGetGateways(WorldMapState& mapState)
{
	return mapState.gateways;
}

// Release a gateway
static void gwFreeGateway(WorldMapState& mapState, GATEWAY *psDel)
{
	int pos;

	if (mapState.tiles) // this lines fixes the bug where we were closing the gateways after freeing the map
	{
		// clear the map flags
		if (psDel->x1 == psDel->x2)
		{
			// vertical gateway
			for (pos = psDel->y1; pos <= psDel->y2; pos++)
			{
				gwClearGatewayFlag(mapState, psDel->x1, pos);
			}
		}
		else
		{
			// horizontal gateway
			for (pos = psDel->x1; pos <= psDel->x2; pos++)
			{
				gwClearGatewayFlag(mapState, pos, psDel->y1);
			}
		}

	}

	free(psDel);
}
