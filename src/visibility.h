/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

#define LINE_OF_FIRE_MINIMUM 5

// initialise the visibility stuff
extern bool visInitialise(void);

/* Check which tiles can be seen by an object */
extern void visTilesUpdate(BASE_OBJECT *psObj);

extern void revealAll(UBYTE player);

/* Check whether psViewer can see psTarget
 * psViewer should be an object that has some form of sensor,
 * currently droids and structures.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 */
extern int visibleObject(const BASE_OBJECT* psViewer, const BASE_OBJECT* psTarget, bool wallsBlock);

/** Can shooter hit target with direct fire weapon? */
bool lineOfFire(const SIMPLE_OBJECT* psViewer, const BASE_OBJECT* psTarget, int weapon_slot, bool wallsBlock);

/** How much of target can the player hit with direct fire weapon? */
int areaOfFire(const SIMPLE_OBJECT* psViewer, const BASE_OBJECT* psTarget, int weapon_slot, bool wallsBlock);

/** How much of target can the player hit with direct fire weapon? */
int arcOfFire(const SIMPLE_OBJECT* psViewer, const BASE_OBJECT* psTarget, int weapon_slot, bool wallsBlock);

// Find the wall that is blocking LOS to a target (if any)
extern STRUCTURE* visGetBlockingWall(const BASE_OBJECT* psViewer, const BASE_OBJECT* psTarget);

bool hasSharedVision(unsigned viewer, unsigned ally);

extern void processVisibilityLevel(BASE_OBJECT *psObj);
extern void processVisibility(void);  ///< Calls processVisibilitySelf and processVisibilityVision on all objects.

// update the visibility reduction
extern void visUpdateLevel(void);

extern void setUnderTilesVis(BASE_OBJECT *psObj, UDWORD player);

void visRemoveVisibilityOffWorld(BASE_OBJECT *psObj);
void visRemoveVisibility(BASE_OBJECT *psObj);

// fast test for whether obj2 is in range of obj1
static inline bool visObjInRange(BASE_OBJECT *psObj1, BASE_OBJECT *psObj2, SDWORD range)
{
	int32_t xdiff = psObj1->pos.x - psObj2->pos.x, ydiff = psObj1->pos.y - psObj2->pos.y;

	return abs(xdiff) <= range && abs(ydiff) <= range && xdiff*xdiff + ydiff*ydiff <= range;
}

static inline int objSensorRange(const BASE_OBJECT* psObj)
{
	return psObj->sensorRange;
}

static inline int objJammerPower(const BASE_OBJECT* psObj)
{
	return psObj->ECMMod;
}

static inline int objConcealment(const BASE_OBJECT* psObj)
{
	return psObj->ECMMod;
}

void objSensorCache(BASE_OBJECT *psObj, SENSOR_STATS *psSensor);
void objEcmCache(BASE_OBJECT *psObj, ECM_STATS *psEcm);

#endif // __INCLUDED_SRC_VISIBILITY__
