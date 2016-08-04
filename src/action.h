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
 * Function prototypes for setting the action of a droid
 */

#ifndef __INCLUDED_SRC_ACTION_H__
#define __INCLUDED_SRC_ACTION_H__

#include "actiondef.h"
#include "droiddef.h"

/**
 *	@file action.h
 *	Droid actions.
 *	@defgroup Actions Droid action subsystem
 *	@{
 */

const char *getDroidActionName(DROID_ACTION action);

/** After failing a route ... this is the amount of time that the droid goes all defensive until it can start going aggressive. */
#define MIN_SULK_TIME (1500)		// 1.5 sec
#define MAX_SULK_TIME (4000)		// 4 secs

/** This is how long a droid is disabled for when its been attacked by an EMP weapon. */
#define EMP_DISABLE_TIME (10000)     // 10 secs

/** How far away the repair droid can be from the damaged droid to function. */
#define REPAIR_RANGE  (2 * TILE_UNITS)

/// The maximum distance a repair droid will automatically go in guard mode.
#define REPAIR_MAXDIST  (5 * TILE_UNITS)

/**
 * Update the action state for a droid.
 *
 * @todo FIXME: Bad design, "updating" the action state is too fuzzy a goal for
 *              a function. As a result this beast is <em>way</em> too large.
 *              The scope of this function should be significantly limited, or
 *              implemented only as a series of subroutine calls with almost no
 *              data manipulation in this function itself. In either case, this
 *              function requires a major refactoring...
 */
void actionUpdateDroid(DROID *psDroid);

/** Do sanity update only. Called from actionUpdateDroid() normally. */
void actionSanity(DROID *psDroid);

/** Give a droid an action. */
void actionDroid(DROID *psDroid, DROID_ACTION action);

/** Give a droid an action with a location target. */
void actionDroid(DROID *psDroid, DROID_ACTION action, UDWORD x, UDWORD y);

/** Give a droid an action with an object target. */
void actionDroid(DROID *psDroid, DROID_ACTION action, BASE_OBJECT *psObj);

/** Give a droid an action with an object target and a location. */
void actionDroid(DROID *psDroid, DROID_ACTION action,
                 BASE_OBJECT *psObj, UDWORD x, UDWORD y);

/** Rotate turret toward  target return True if locked on (Droid and Structure). */
bool actionTargetTurret(BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget, WEAPON *psWeapon);

/** Realign turret. */
void actionAlignTurret(BASE_OBJECT *psObj, int weapon_slot);

/** Check if a target is within weapon range. */
bool actionInRange(const DROID *psDroid, const BASE_OBJECT *psObj, int weapon_slot);

/** Return whether a droid can see a target to fire on it. */
bool actionVisibleTarget(DROID *psDroid, BASE_OBJECT *psTarget, int weapon_slot);

/** Check whether a droid is in the neighboring tile to a build position. */
bool actionReachedBuildPos(DROID const *psDroid, int x, int y, uint16_t direction, BASE_STATS const *psStats);

/** Send the vtol droid back to the nearest rearming pad - if there is one, otherwise return to base. */
void moveToRearm(DROID *psDroid);

/** Choose a landing position for a VTOL when it goes to rearm. */
bool actionVTOLLandingPos(DROID const *psDroid, Vector2i *p);

/** How many frames to skip before looking for a better target. */
#define TARGET_UPD_SKIP_FRAMES 1000

/** @} */

#endif // __INCLUDED_SRC_ACTION_H__
