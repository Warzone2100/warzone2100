/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_VISIBILITY__
#define __INCLUDED_SRC_VISIBILITY__

#include "objectdef.h"
#include "raycast.h"
#include "stats.h"

#define LINE_OF_FIRE_MINIMUM 5

// initialise the visibility stuff
bool visInitialise();

/* Check which tiles can be seen by an object */
void visTilesUpdate(BASE_OBJECT *psObj);

void revealAll(UBYTE player);

/* Check whether psViewer can see psTarget
 * psViewer should be an object that has some form of sensor,
 * currently droids and structures.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 */
int visibleObject(const BASE_OBJECT *psViewer, const BASE_OBJECT *psTarget, bool wallsBlock);

/** Can shooter hit target with direct fire weapon? */
bool lineOfFire(const SIMPLE_OBJECT *psViewer, const BASE_OBJECT *psTarget, int weapon_slot, bool wallsBlock);

/** How much of target can the player hit with direct fire weapon? */
int areaOfFire(const SIMPLE_OBJECT *psViewer, const BASE_OBJECT *psTarget, int weapon_slot, bool wallsBlock);

/** How much of target can the player hit with direct fire weapon? */
int arcOfFire(const SIMPLE_OBJECT *psViewer, const BASE_OBJECT *psTarget, int weapon_slot, bool wallsBlock);

// Find the wall that is blocking LOS to a target (if any)
STRUCTURE *visGetBlockingWall(const BASE_OBJECT *psViewer, const BASE_OBJECT *psTarget);

bool hasSharedVision(unsigned viewer, unsigned ally);

void processVisibility();  ///< Calls processVisibilitySelf and processVisibilityVision on all objects.

// update the visibility reduction
void visUpdateLevel();

void setUnderTilesVis(BASE_OBJECT *psObj, UDWORD player);

void visRemoveVisibilityOffWorld(BASE_OBJECT *psObj);
void visRemoveVisibility(BASE_OBJECT *psObj);

// fast test for whether obj2 is in range of obj1
static inline bool visObjInRange(BASE_OBJECT *psObj1, BASE_OBJECT *psObj2, SDWORD range)
{
	int32_t xdiff = psObj1->pos.x - psObj2->pos.x, ydiff = psObj1->pos.y - psObj2->pos.y;

	return abs(xdiff) <= range && abs(ydiff) <= range && xdiff * xdiff + ydiff * ydiff <= range;
}

// If we have ECM, use this for range instead. Otherwise, the sensor's range will be used for
// jamming range, which we do not want. Rather limit ECM unit sensor range to jammer range.
static inline int objSensorRange(const BASE_OBJECT *psObj)
{
	if (psObj->type == OBJ_DROID)
	{
		const int ecmrange = asECMStats[((const DROID *)psObj)->asBits[COMP_ECM]].upgrade[psObj->player].range;
		if (ecmrange > 0)
		{
			return ecmrange;
		}
		return asSensorStats[((const DROID *)psObj)->asBits[COMP_SENSOR]].upgrade[psObj->player].range;
	}
	else if (psObj->type == OBJ_STRUCTURE)
	{
		const int ecmrange = ((const STRUCTURE *)psObj)->pStructureType->pECM->upgrade[psObj->player].range;
		if (ecmrange)
		{
			return ecmrange;
		}
		return ((const STRUCTURE *)psObj)->pStructureType->pSensor->upgrade[psObj->player].range;
	}
	return 0;
}

static inline int objJammerPower(const BASE_OBJECT *psObj)
{
	if (psObj->type == OBJ_DROID)
	{
		return asECMStats[((const DROID *)psObj)->asBits[COMP_ECM]].upgrade[psObj->player].range;
	}
	else if (psObj->type == OBJ_STRUCTURE)
	{
		return ((const STRUCTURE *)psObj)->pStructureType->pECM->upgrade[psObj->player].range;
	}
	return 0;
}

void removeSpotters();
bool removeSpotter(uint32_t id);
uint32_t addSpotter(int x, int y, int player, int radius, bool radar, uint32_t expiry = 0);

#endif // __INCLUDED_SRC_VISIBILITY__
