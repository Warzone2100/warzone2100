/*
 * GatewayRoute.h
 *
 * Interface to the gateway routing code
 *
 */
#ifndef _gatewayroute_h
#define _gatewayroute_h

#include "gateway.h"

// what type of terrain the unit can move over
enum _gwr_terrain_types
{
	GWR_TER_LAND	= 0x01,
	GWR_TER_WATER	= 0x02,

	GWR_TER_ALL		= 0xff,
};

// return codes for the router
enum _gwr_return_codes
{
	GWR_OK,				// found a route
	GWR_NEAREST,		// chose the nearest gateway to the target
	GWR_FAILED,			// couldn't find a route
	GWR_SAMEZONE,		// start and end points in the same zone
	GWR_NOZONE,			// the route did not start on a valid zone
};

// A* findpath for the gateway system
extern SDWORD gwrAStarRoute(SDWORD player, UDWORD terrain,
							SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy,
							GATEWAY **ppsRoute);

#endif

