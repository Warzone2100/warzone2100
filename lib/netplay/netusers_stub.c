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
 * netusers.c
 * functions regarding specific players
 */

#include "lib/framework/frame.h"
#include "netplay.h"

// call to disable/enable ALL comms. Absolute arse of a thing, be very careful!
BOOL NETuseNetwork(BOOL val)
{
	NetPlay.bComms = val;
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// Functions for spectators.

BOOL NETspectate(void) // FIXME Remove if unused
{
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETisSpectator(UDWORD dpid) // FIXME Remove if unused
{
	UBYTE i;

	// was "dpid = NetPlay.dpidPlayer == dpid", but that didn't make sense.
	if((dpid = NetPlay.dpidPlayer) == dpid)	// checking ourselves
	{
		return NetPlay.bSpectator;
	}

	// could enumerate the spectators and check if he's there!
	// bugger it, just check that dpid isn't a player instead!
	for(i=0; i<MaxNumberOfPlayers; i++)
	{
		if(NetPlay.players[i].dpid == dpid)
		{
			return FALSE;
		}
	}

	return TRUE;
}

