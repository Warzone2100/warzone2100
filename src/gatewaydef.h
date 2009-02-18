/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
/** \file
 *  Definitions for routing/pathfinding gateways.
 */

#ifndef __INCLUDED_GATEWAYDEF_H__
#define __INCLUDED_GATEWAYDEF_H__

typedef struct _gateway
{
	UBYTE			x1,y1, x2,y2;
	UBYTE			flags;		// open or closed node
	struct _gateway		*psNext;
} GATEWAY;

// types of node for the gateway router
enum _gw_node_flags
{
	GWR_OPEN		= 0x01,
	GWR_CLOSED		= 0x02,
	GWR_ZONE1		= 0x04,		// the route goes from zone1 to zone2
	GWR_ZONE2		= 0x08,		// the route goes from zone2 to zone1
	GWR_INROUTE		= 0x10,		// the gateway is part of the final route
	GWR_BLOCKED		= 0x20,		// the gateway is totally blocked
	GWR_IGNORE		= 0x40,		// the gateway is to be ignored by the router
	GWR_WATERLINK	= 0x80,		// the gateway is a land/water link
};

#endif // __INCLUDED_GATEWAYDEF_H__
