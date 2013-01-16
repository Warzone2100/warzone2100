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
 * GatewayDef.h
 *
 * Structure definitions for routing gateways.
 *
 */

#ifndef __INCLUDED_GATEWAYDEF_H__
#define __INCLUDED_GATEWAYDEF_H__

typedef struct _gateway_link
{
	struct _gateway  *psGateway;
	SDWORD			 dist;
} GATEWAY_LINK;

typedef struct _gateway
{
	UBYTE		x1,y1, x2,y2;

	UBYTE		zone1;		// zone to the left/above the gateway
	UBYTE		zone2;		// zone to the right/below the gateway

	struct _gateway		*psNext;

	GATEWAY_LINK		*psLinks;	// array of links to other zones
	UBYTE				zone1Links;	// number of links
	UBYTE				zone2Links;

	// Data for the gateway router
	UBYTE	flags;		// open or closed node
	SWORD	dist, est;	// distance so far and estimate to end

	struct _gateway *psOpen;
	struct _gateway	*psRoute;	// Previous point in the route

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
	GWR_WATERLINK	= 0x80,		// the gateway is a land/water link
};

// the flags reset by the router
#define GWR_RESET_MASK	0x7f

// the maximum width and height of the map
#define GW_MAP_MAXWIDTH		(MAP_MAXWIDTH - 1)
#define GW_MAP_MAXHEIGHT	(MAP_MAXHEIGHT - 1)



#endif // __INCLUDED_GATEWAYDEF_H__
