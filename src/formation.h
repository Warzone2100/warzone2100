/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2023  Warzone 2100 Project

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
 *  Control units moving in formation.
 */

#pragma once

#include "formationdef.h"

enum FORMATION_TYPE
{
	FT_LINE,
	FT_COLUMN,
};

// Initialise the formation system
bool formationInitialise();

// Shutdown the formation system
void formationShutDown();

// Create a new formation
bool formationNew(FORMATION **ppsFormation, uint32_t player, FORMATION_TYPE type,
					SDWORD x, SDWORD y, uint16_t dir);

// Try and find a formation near to a location
FORMATION* formationFind(uint32_t player, int x, int y);

// Associate a unit with a formation
void formationJoin(FORMATION *psFormation, const DROID* psDroid);

// Remove a unit from a formation
void formationLeave(FORMATION *psFormation, const DROID* psDroid);

// remove all the members from a formation and release it
void formationReset(FORMATION *psFormation);

// get a target position to move into a formation
bool formationGetPos(FORMATION *psFormation, DROID* psDroid,
					 SDWORD *pX, SDWORD *pY, bool bCheckLOS);

// See if a unit is a member of a formation (i.e. it has a position assigned)
bool formationMember(FORMATION *psFormation, const DROID* psDroid);
