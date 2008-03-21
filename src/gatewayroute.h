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
/** @file
 *  Interface to the gateway routing code
 */

#ifndef __INCLUDED_SRC_GATEWAYROUTE_H__
#define __INCLUDED_SRC_GATEWAYROUTE_H__

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

#endif // __INCLUDED_SRC_GATEWAYROUTE_H__
