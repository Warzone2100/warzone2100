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

#ifndef __INCLUDED_SRC_VISIBILITY__
#define __INCLUDED_SRC_VISIBILITY__

#include "objectdef.h"

// initialise the visibility stuff
extern BOOL visInitialise(void);

extern BOOL visTilesPending(BASE_OBJECT *psObj);

/* Check which tiles can be seen by an object */
extern void visTilesUpdate(BASE_OBJECT *psObj);

/* Check whether psViewer can see psTarget
 * psViewer should be an object that has some form of sensor,
 * currently droids and structures.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 */
extern BOOL visibleObject(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget);

/* Check whether psViewer can see psTarget.
 * struckBlock controls whether structures block LOS
 */
extern BOOL visibleObjectBlock(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget,
							   BOOL structBlock);

// Do visibility check, but with walls completely blocking LOS.
extern BOOL visibleObjWallBlock(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget);

// Find the wall that is blocking LOS to a target (if any)
extern BOOL visGetBlockingWall(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget, STRUCTURE **ppsWall);

extern void	processVisibility(BASE_OBJECT *psCurr);

// update the visibility reduction
extern void visUpdateLevel(void);

extern	void	setUnderTilesVis(BASE_OBJECT *psObj, UDWORD player);

// sensor range display
extern BOOL	bDisplaySensorRange;
extern void updateSensorDisplay(void);

// fast test for whether obj2 is in range of obj1
static inline BOOL visObjInRange(BASE_OBJECT *psObj1, BASE_OBJECT *psObj2, SDWORD range)
{
	SDWORD	xdiff,ydiff, distSq, rangeSq;

	xdiff = (SDWORD)psObj1->pos.x - (SDWORD)psObj2->pos.x;
	if (xdiff < 0)
	{
		xdiff = -xdiff;
	}
	if (xdiff > range)
	{
		// too far away, reject
		return false;
	}

	ydiff = (SDWORD)psObj1->pos.y - (SDWORD)psObj2->pos.y;
	if (ydiff < 0)
	{
		ydiff = -ydiff;
	}
	if (ydiff > range)
	{
		// too far away, reject
		return false;
	}

	distSq = xdiff*xdiff + ydiff*ydiff;
	rangeSq = range*range;
	if (distSq > rangeSq)
	{
		// too far away, reject
		return false;
	}

	return true;
}

static inline int objSensorRange(BASE_OBJECT *psObj)
{
	return psObj->sensorRange;
}

static inline int objSensorPower(BASE_OBJECT *psObj)
{
	return psObj->sensorPower;
}

static inline int objJammerPower(BASE_OBJECT *psObj)
{
	return 0;
}

static inline int objJammerRange(BASE_OBJECT *psObj)
{
	return 0;
}

static inline int objConcealment(BASE_OBJECT *psObj)
{
	return psObj->ECMMod;
}

#endif // __INCLUDED_SRC_VISIBILITY__
