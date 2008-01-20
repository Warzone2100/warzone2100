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
 * Gateway.h
 *
 * Interface to routing gateway code.
 *
 */
#ifndef _gateway_h
#define _gateway_h

#include "gatewaydef.h"

// the list of gateways on the current map
extern GATEWAY		*psGateways;
// the RLE map zones for each tile
extern UBYTE		**apRLEZones;

// the number of map zones
extern SDWORD		gwNumZones;

// The zone equivalence tables
extern UBYTE		*aNumEquiv;
extern UBYTE		**apEquivZones;

extern UBYTE		*aZoneReachable;

// Initialise the gateway system
BOOL gwInitialise(void);

// Shutdown the gateway system
void gwShutDown(void);

// Add a gateway to the system
BOOL gwNewGateway(SDWORD x1, SDWORD y1, SDWORD x2, SDWORD y2);

// Add a land/water link gateway to the system
BOOL gwNewLinkGateway(SDWORD x, SDWORD y);

// add the land/water link gateways
BOOL gwGenerateLinkGates(void);

// Release a gateway
void gwFreeGateway(GATEWAY *psDel);

// load a gateway list
BOOL gwLoadGateways(char *pFileBuffer, UDWORD fileSize);

// link all the gateways together
BOOL gwLinkGateways(void);

// check all the zones are of reasonable sizes
void gwCheckZoneSizes(void);

// check if a zone is in the equivalence table for a water zone
BOOL gwZoneInEquiv(SDWORD mainZone, SDWORD checkZone);

// Look up the zone for a coordinate
SDWORD gwGetZone(SDWORD x, SDWORD y);

// Create a new empty zone map but don't allocate the actual zones yet.
BOOL gwNewZoneMap(void);

// Create a new empty zone map line in the zone map.
UBYTE * gwNewZoneLine(UDWORD Line,UDWORD Size);

// Create a NULL zone map for when there is no zone info loaded
BOOL gwCreateNULLZoneMap(void);

// release the RLE Zone map
void gwFreeZoneMap(void);

// get the size of the map
SDWORD gwMapWidth(void);
SDWORD gwMapHeight(void);

// set the gateway flag on a tile
void gwSetGatewayFlag(SDWORD x, SDWORD y);
// clear the gateway flag on a tile
void gwClearGatewayFlag(SDWORD x, SDWORD y);

// check whether a tile is water
BOOL gwTileIsWater(UDWORD x, UDWORD y);

// Get number of gateways.
UDWORD gwNumGateways(void);

// Get the gateway list.
GATEWAY *gwGetGateways(void);

// Get number of zone lines.
UDWORD gwNumZoneLines(void);

// Get size of a zone line in bytes.
UDWORD gwZoneLineSize(UDWORD Line);

// create an empty equivalence table
BOOL gwNewEquivTable(SDWORD numZones);

// release the equivalence table
void gwFreeEquivTable(void);

// set the zone equivalence for a zone
BOOL gwSetZoneEquiv(SDWORD zone, SDWORD numEquiv, UBYTE *pEquiv);

// see if a zone is reachable
BOOL gwZoneReachable(SDWORD zone);

#endif
