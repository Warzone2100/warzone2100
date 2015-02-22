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
/** @file
 *  Definitions for the Power Functionality.
 */

#ifndef __INCLUDED_SRC_POWER_H__
#define __INCLUDED_SRC_POWER_H__

/** Free power on collection of oildrum. */
#define OILDRUM_POWER		100

/** Allocate the space for the playerPower. */
extern bool allocPlayerPower(void);

/** Clear the playerPower. */
extern void clearPlayerPower(void);

/// Removes any pending power request from this structure.
void delPowerRequest(STRUCTURE *psStruct);

/// Checks how much power must be accumulated, before the power request from this structure can be satisfied.
/// Returns -1 if there is no power request or if there is enough power already.
int32_t checkPowerRequest(STRUCTURE *psStruct);

/** Reset the power levels when a power_gen or resource_extractor is destroyed. */
extern bool resetPlayerPower(UDWORD player, STRUCTURE *psStruct);

/** Check the available power. */
bool checkPower(int player, uint32_t quantity);

bool requestPowerFor(STRUCTURE *psStruct, int32_t amount);
bool requestPrecisePowerFor(STRUCTURE *psStruct, int64_t amount);

extern void addPower(int player, int32_t quantity);

void usePower(int player, uint32_t quantity);

/** Update current power based on what was extracted during the last cycle and what Power Generators exist. 
  * If ticks is set, this is the number of game ticks to process for at once. */
extern void updatePlayerPower(int player, int ticks = 1);

/** Used in multiplayer to force power levels. */
void setPower(unsigned player, int32_t power);
void setPrecisePower(unsigned player, int64_t power);

void setPowerModifier(int player, int modifier);

/** Get the amount of power current held by the given player. */
int32_t getPower(unsigned player);
int64_t getPrecisePower(unsigned player);
int32_t getPowerMinusQueued(unsigned player);
int getQueuedPower(int player);

/** Resets the power levels for all players when power is turned back on. */
void powerCalc(bool on);

/** Flag used to check for power calculations to be done or not. */
extern	bool			powerCalculated;

#endif // __INCLUDED_SRC_POWER_H__
