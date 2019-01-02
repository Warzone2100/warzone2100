/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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
 *  Interface to routing gateway code.
 */

#ifndef __INCLUDED_SRC_GATEWAY_H__
#define __INCLUDED_SRC_GATEWAY_H__

struct GATEWAY
{
	UBYTE x1, y1, x2, y2;
};

typedef std::list<GATEWAY *> GATEWAY_LIST;

/// Initialise the gateway system
bool gwInitialise();

/// Shutdown the gateway system
void gwShutDown();

/// Add a gateway to the system
bool gwNewGateway(int x1, int y1, int x2, int y2);

/// Get number of gateways.
int gwNumGateways();

/// Get the gateway list.
GATEWAY_LIST &gwGetGateways();

#endif // __INCLUDED_SRC_GATEWAY_H__
