/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
 *  Typedef's for command droids
 */

#ifndef __INCLUDED_SRC_CMDDROID_H__
#define __INCLUDED_SRC_CMDDROID_H__

#include "cmddroiddef.h"
#include "droiddef.h"

// Initialise the command droids
extern BOOL cmdDroidInit(void);

// ShutDown the command droids
extern void cmdDroidShutDown(void);

// Make new command droids available
extern void cmdDroidAvailable(BRAIN_STATS *psBrainStats, SDWORD player);

// update the command droids
extern void cmdDroidUpdate(void);

// add a droid to a command group
extern void cmdDroidAddDroid(DROID *psCommander, DROID *psDroid);

// return the current target designator for a player
extern DROID *cmdDroidGetDesignator(UDWORD player);

// set the current target designator for a player
extern void cmdDroidSetDesignator(DROID *psDroid);

// set the current target designator for a player
extern void cmdDroidClearDesignator(UDWORD player);

// get the index of the command droid
extern SDWORD cmdDroidGetIndex(DROID *psCommander);

// get the maximum group size for a command droid
extern unsigned int cmdDroidMaxGroup(const DROID* psCommander);

// update the kills of a command droid if psKiller is in a command group
extern void cmdDroidUpdateKills(DROID *psKiller, uint32_t experienceInc);

// get the level of a droids commander, if any
extern unsigned int cmdGetCommanderLevel(const DROID* psDroid);

// returns true if a unit in question has is assigned to a commander
extern bool hasCommander(const DROID* psDroid);

// Select all droids assigned to the passed in command droids
extern void	cmdSelectSubDroids(DROID *psDroid);

// note that commander experience should be increased
extern void cmdDroidMultiExpBoost(BOOL bDoit);

// check whether commander experience should be increased
extern BOOL cmdGetDroidMultiExpBoost(void);

#endif // __INCLUDED_SRC_CMDDROID_H__
