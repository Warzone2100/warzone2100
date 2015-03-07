/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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

/// the list of gateways on the current map
static GATEWAY_LIST psGateways;

// Prototypes
static void gwFreeGateway(GATEWAY *psDel);

/******************************************************************************************************/
/*                   Gateway data access functions                                                    */

// get the size of the map
static SDWORD gwMapWidth(void)
{
	return (SDWORD)mapWidth;
}

static SDWORD gwMapHeight(void)
{
	return (SDWORD)mapHeight;
}

// set the gateway flag on a tile
static void gwSetGatewayFlag(SDWORD x, SDWORD y)
{
	mapTile((UDWORD)x,(UDWORD)y)->tileInfoBits |= BITS_GATEWAY;
}

// clear the gateway flag on a tile
static void gwClearGatewayFlag(SDWORD x, SDWORD y)
{
	mapTile((UDWORD)x,(UDWORD)y)->tileInfoBits &= ~BITS_GATEWAY;
}


/******************************************************************************************************/
/*                   Gateway functions                                                                */

// Initialise the gateway system
bool gwInitialise(void)
{
	psGateways.clear();
	return true;
}

// Shutdown the gateway system
void gwShutDown(void)
{
	for (auto psGateway : psGateways)
	{
		gwFreeGateway(psGateway);
	}
	psGateways.clear();
}

// Add a gateway to the system
bool gwNewGateway(SDWORD x1, SDWORD y1, SDWORD x2, SDWORD y2)
{
	GATEWAY		*psNew;
	SDWORD		pos, temp;

	ASSERT_OR_RETURN(false, x1 >= 0 && x1 < gwMapWidth() && y1 >= 0 && y1 < gwMapHeight()
	                 && x2 >= 0 && x2 < gwMapWidth() && y2 >= 0 && y2 < gwMapHeight()
	                 && (x1 == x2 || y1 == y2), "Invalid gateway coordinates (%d, %d, %d, %d)",
	                 x1, y1, x2, y2);
	psNew = (GATEWAY*)malloc(sizeof(GATEWAY));

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
	psNew->x2 = MIN(x2, mapWidth - 4);
	psNew->y2 = MIN(y2, mapHeight - 4);

	// add the gateway to the list
	psGateways.push_back(psNew);

	// set the map flags
	if (psNew->x1 == psNew->x2)
	{
		// vertical gateway
		for(pos = psNew->y1; pos <= psNew->y2; pos++)
		{
			gwSetGatewayFlag(psNew->x1, pos);
		}
	}
	else
	{
		// horizontal gateway
		for(pos = psNew->x1; pos <= psNew->x2; pos++)
		{
			gwSetGatewayFlag(pos, psNew->y1);
		}
	}

	return true;
}

// Return the number of gateways.
int gwNumGateways()
{
	return psGateways.size();
}

GATEWAY_LIST &gwGetGateways()
{
	return psGateways;
}

// Release a gateway
static void gwFreeGateway(GATEWAY *psDel)
{
	int pos;

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

	free(psDel);
}
