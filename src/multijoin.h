/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
 *  multijoin caters for all the player comings and goings of each player
 */

#ifndef __INCLUDED_SRC_MULTIJOIN_H__
#define __INCLUDED_SRC_MULTIJOIN_H__

#include "droiddef.h"

extern bool intDisplayMultiJoiningStatus(UBYTE joinCount);
void recvPlayerLeft(NETQUEUE queue);
extern bool MultiPlayerLeave			(UDWORD playerIndex);						// A player has left the game.
extern bool MultiPlayerJoin				(UDWORD playerIndex);						// A Player has joined the game.
extern void setupNewPlayer				(UDWORD player);		// stuff to do when player joins.
extern void clearPlayer					(UDWORD player, bool quietly);// wipe a player off the face of the earth.
//extern bool ProcessDroidOrders			(void);
//extern UDWORD							arenaPlayersReceived;

extern void ShowMOTD(void);
extern bool recvDataCheck(NETQUEUE queue);
extern bool sendDataCheck(void);

#endif // __INCLUDED_SRC_MULTIJOIN_H__
