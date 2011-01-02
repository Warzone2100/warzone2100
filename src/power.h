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
 *  Definitions for the Power Functionality.
 */

#ifndef __INCLUDED_SRC_POWER_H__
#define __INCLUDED_SRC_POWER_H__

/** Free power on collection of oildrum. */
#define OILDRUM_POWER		100

#define	POWER_PER_SECOND        25

/** Used to determine the power cost of repairing a droid.
    Definately DON'T WANT the brackets round 1/2 - it will equate to zero! */
#define REPAIR_POWER_FACTOR 1/5

/** Used to multiply all repair calculations by to avaoid rounding errors. */
#define POWER_FACTOR        100

typedef struct _player_power
{
	int32_t		currentPower;		/**< The current amount of power avaialble to the player. */
	int32_t		extractedPower;		/**< The power extracted but not converted. */
} PLAYER_POWER;

/** Allocate the space for the playerPower. */
BOOL allocPlayerPower(void);

/** Clear the playerPower. */
void clearPlayerPower(void);

/** Check the available power. */
BOOL checkPower(UDWORD player, UDWORD quantity);

/** Subtract the power required. */
BOOL usePower(UDWORD player, UDWORD quantity);

/** Return the power when a structure/droid is deliberately destroyed. */
void addPower(UDWORD player, UDWORD quantity);

/** Update current power based on what was extracted during the last cycle and what Power Generators exist. */
void updatePlayerPower(UDWORD player);

/** Used in multiplayer to force power levels. */
void setPower(UDWORD player, UDWORD avail);

/** Get the amount of power current held by the given player. */
UDWORD getPower(UDWORD player);

/** Resets the power levels for all players when power is turned back on. */
void powerCalc(BOOL on);

/** Sets the initial value for the power. */
void setPlayerPower(UDWORD power, UDWORD player);

/** Temp function to give all players some power when a new game has been loaded. */
void newGameInitPower(void);

/** Accrue the power in the facilities that require it. */
void accruePower(BASE_OBJECT *psObject);

/**	Returns the next res. Ext. in the list from the one passed in. returns 1st one
	in list if passed in is NULL and NULL if there's none?
*/
STRUCTURE *getRExtractor(STRUCTURE *psStruct);

/** Defines which structure types draw power - returns true if use power. */
BOOL structUsesPower(STRUCTURE *psStruct);

/** Defines which droid types draw power - returns true if use power. */
BOOL droidUsesPower(DROID *psDroid);

/** @todo Wrap access to this global and make it static for easier sanity checking! */
extern PLAYER_POWER		asPower[MAX_PLAYERS];

/** Flag used to check for power calculations to be done or not. */
extern	BOOL			powerCalculated;

#endif // __INCLUDED_SRC_POWER_H__
