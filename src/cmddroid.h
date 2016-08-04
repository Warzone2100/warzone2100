/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
/**
 *
 * @file
 * Typedef's for command droids
 *
 */

#ifndef __INCLUDED_SRC_CMDDROID_H__
#define __INCLUDED_SRC_CMDDROID_H__

#include "cmddroiddef.h"
#include "droiddef.h"

/** \brief Initialises the global instance for command droids.*/
extern bool cmdDroidInit(void);

/** \brief Shut down commander code module.
 */
extern void cmdDroidShutDown(void);

/** \brief Checks the validity of all target designators.*/
extern void cmdDroidUpdate(void);

/** \brief Adds a droid to a command group.*/
extern void cmdDroidAddDroid(DROID *psCommander, DROID *psDroid);

/** \brief Returns the current target designator for a player.*/
extern DROID *cmdDroidGetDesignator(UDWORD player);

/** \brief Sets the current target designator for a player.*/
extern void cmdDroidSetDesignator(DROID *psDroid);

/** \brief Clears the current target designator for a player.*/
extern void cmdDroidClearDesignator(UDWORD player);

/** \brief Gets the index of the command droid.*/
extern SDWORD cmdDroidGetIndex(DROID *psCommander);

/** \brief Gets the maximum group size for a command droid.*/
extern unsigned int cmdDroidMaxGroup(const DROID *psCommander);

/** \brief Updates the experinence of a command droid if psKiller is in a command group.*/
extern void cmdDroidUpdateKills(DROID *psKiller, uint32_t experienceInc);

/** \brief Gets the level of the droid group's commander, if any.*/
extern unsigned int cmdGetCommanderLevel(const DROID *psDroid);

/** \brief Returns if the droid has commander.*/
extern bool hasCommander(const DROID *psDroid);

/** \brief Sets whether commander experience should be increased in multiplayer games.*/
extern void cmdDroidMultiExpBoost(bool bDoit);

/** \brief Checks whether commander experience should be increased in multiplayer games.*/
extern bool cmdGetDroidMultiExpBoost(void);

#endif // __INCLUDED_SRC_CMDDROID_H__
