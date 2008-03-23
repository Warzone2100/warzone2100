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
 *  Definitions for the Power Functionality.
 */

#ifndef __INCLUDED_SRC_POWER_H__
#define __INCLUDED_SRC_POWER_H__

/** Free power on collection of oildrum. */
#define OILDRUM_POWER		100

#define	POWER_PER_CYCLE		5

/** Used to determine the power cost of repairing a droid.
    Definately DON'T WANT the brackets round 1/2 - it will equate to zero! */
#define REPAIR_POWER_FACTOR 1/5

/** Used to multiply all repair calculations by to avaoid rounding errors. */
#define POWER_FACTOR        100

typedef struct _player_power
{
	UDWORD		currentPower;		/**< The current amount of power avaialble to the player. */
	UDWORD		extractedPower;		/**< The power extracted but not converted. */
	BASE_OBJECT	*psLastPowered;		/**< The last object that received power before it ran out. */
} PLAYER_POWER;

/** Allocate the space for the playerPower. */
extern BOOL allocPlayerPower(void);

/** Clear the playerPower. */
extern void clearPlayerPower(void);

/** Reset the power levels when a power_gen or resource_extractor is destroyed. */
extern BOOL resetPlayerPower(UDWORD player, STRUCTURE *psStruct);

/** Free the space used for playerPower. */
extern void releasePlayerPower(void);

/** Check the available power. */
extern BOOL checkPower(UDWORD player, UDWORD quantity, BOOL playAudio);

/** Subtract the power required. */
extern BOOL usePower(UDWORD player, UDWORD quantity);

/** Return the power when a structure/droid is deliberately destroyed. */
extern void addPower(UDWORD player, UDWORD quantity);

/** Each Resource Extractor yields EXTRACT_POINTS per second until there are none left in the resource. */
extern UDWORD updateExtractedPower(STRUCTURE	*psBuilding);

/** Update current power based on what was extracted during the last cycle and what Power Generators exist. */
extern void updatePlayerPower(UDWORD player);

/** Used in multiplayer to force power levels. */
extern void setPower(UDWORD player, UDWORD avail); 

/** Get the amount of power current held by the given player. */
extern UDWORD getPower(UDWORD player);

/** Resets the power levels for all players when power is turned back on. */
void powerCalc(BOOL on);

/** Sets the initial value for the power. */
void setPlayerPower(UDWORD power, UDWORD player);

/** Temp function to give all players some power when a new game has been loaded. */
void newGameInitPower(void);

/** Informs the power array that a Object has been destroyed. */
extern void powerDestroyObject(BASE_OBJECT *psObject);

/** Accrue the power in the facilities that require it. */
extern BOOL accruePower(BASE_OBJECT *psObject);

/** Inform the players power struct that the last Object to receive power has changed. */
extern void updateLastPowered(BASE_OBJECT *psObject,UBYTE player);

/**	Returns the next res. Ext. in the list from the one passed in. returns 1st one
	in list if passed in is NULL and NULL if there's none?
*/
extern STRUCTURE *getRExtractor(STRUCTURE *psStruct);

/** Checks if the object to be powered next - returns TRUE if power. */
extern BOOL getLastPowered(BASE_OBJECT *psStructure);

/** Defines which structure types draw power - returns TRUE if use power. */
extern BOOL structUsesPower(STRUCTURE *psStruct);

/** Defines which droid types draw power - returns TRUE if use power. */
extern BOOL droidUsesPower(DROID *psDroid);

/** This is a check cos there is a problem with the power but not sure where!! */
extern void powerCheck(BOOL bBeforePowerUsed, UBYTE player);

/** @todo Wrap access to this global and make it static for easier sanity checking! */
extern PLAYER_POWER		asPower[MAX_PLAYERS];

/** Flag used to check for power calculations to be done or not. */
extern	BOOL			powerCalculated;

#endif // __INCLUDED_SRC_POWER_H__
