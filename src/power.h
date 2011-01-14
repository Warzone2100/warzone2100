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

/** Used to determine the power cost of repairing a droid.
    Definately DON'T WANT the brackets round 1/2 - it will equate to zero! */
#define REPAIR_POWER_FACTOR 1/5

/** Used to multiply all repair calculations by to avaoid rounding errors. */
#define POWER_FACTOR        100

/** Allocate the space for the playerPower. */
extern BOOL allocPlayerPower(void);

/** Clear the playerPower. */
extern void clearPlayerPower(void);

/** Reset the power levels when a power_gen or resource_extractor is destroyed. */
extern BOOL resetPlayerPower(UDWORD player, STRUCTURE *psStruct);

/** Check the available power. */
BOOL checkPower(int player, uint32_t quantity);

extern int requestPowerFor(int player, int32_t amount, int points);
extern int requestPrecisePowerFor(int player, int64_t amount, int points);

extern void addPower(int player, int32_t quantity);

void usePower(int player, uint32_t quantity);

/** Update current power based on what was extracted during the last cycle and what Power Generators exist. */
extern void updatePlayerPower(UDWORD player);

/** Used in multiplayer to force power levels. */
void setPower(unsigned player, int32_t power);
void setPrecisePower(unsigned player, int64_t power);

/** Get the amount of power current held by the given player. */
int32_t getPower(unsigned player);
int64_t getPrecisePower(unsigned player);

/** Resets the power levels for all players when power is turned back on. */
void powerCalc(BOOL on);

/** Temp function to give all players some power when a new game has been loaded. */
void newGameInitPower(void);

/** Defines which structure types draw power - returns true if use power. */
extern BOOL structUsesPower(STRUCTURE *psStruct);

/** Defines which droid types draw power - returns true if use power. */
extern BOOL droidUsesPower(DROID *psDroid);

/** Flag used to check for power calculations to be done or not. */
extern	BOOL			powerCalculated;

extern void throttleEconomy(void);

#endif // __INCLUDED_SRC_POWER_H__
