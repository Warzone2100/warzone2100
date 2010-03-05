/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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

#ifndef __INCLUDED_SRC_FORMATION_H__
#define __INCLUDED_SRC_FORMATION_H__

#include "formationdef.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

typedef enum _formation_type
{
	FT_LINE,
	FT_COLUMN,
} FORMATION_TYPE;

// Initialise the formation system
extern BOOL formationInitialise(void);

// Shutdown the formation system
extern void formationShutDown(void);

// Create a new formation
extern BOOL formationNew(FORMATION **ppsFormation, FORMATION_TYPE type,
					SDWORD x, SDWORD y, uint16_t dir);

// Try and find a formation near to a location
extern FORMATION* formationFind(int x, int y);

// Associate a unit with a formation
extern void formationJoin(FORMATION *psFormation, const DROID* psDroid);

// Remove a unit from a formation
extern void formationLeave(FORMATION *psFormation, const DROID* psDroid);

// remove all the members from a formation and release it
extern void formationReset(FORMATION *psFormation);

// get a target position to move into a formation
extern BOOL formationGetPos(FORMATION *psFormation, DROID* psDroid,
					 SDWORD *pX, SDWORD *pY, BOOL bCheckLOS);

// See if a unit is a member of a formation (i.e. it has a position assigned)
extern BOOL formationMember(FORMATION *psFormation, const DROID* psDroid);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_SRC_FORMATION_H__
