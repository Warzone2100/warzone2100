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
 * Multijoin.h
 *
 * Alex Lee, pumpkin studios,
 * multijoin caters for all the player comings and goings of each player
 */

#include "lib/netplay/netplay.h"

extern BOOL intDisplayMultiJoiningStatus(UBYTE joinCount);
extern BOOL MultiPlayerLeave			(UDWORD dp);						// A player has left the game.
extern BOOL MultiPlayerJoin				(UDWORD dp);						// A Player has joined the game.
extern void setupNewPlayer				(UDWORD dpid, UDWORD player);		// stuff to do when player joins.
//extern BOOL UpdateClient				(DPID dest, UDWORD playerToSend);// send info about another player
extern void clearPlayer					(UDWORD player, BOOL quietly, BOOL removeOil);// wipe a player off the face of the earth.
//extern BOOL ProcessDroidOrders			(void);
//extern BOOL recvFeatures				(NETMSG *pMsg);
//extern UDWORD							arenaPlayersReceived;

typedef struct {
	DROID *psDroid;
	void  *psNext;
}DROIDSTORE, *LPDROIDSTORE;

extern DROIDSTORE *tempDroidList;
