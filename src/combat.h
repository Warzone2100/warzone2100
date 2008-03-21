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
 *  Interface to the combat routines.
 */

#ifndef __INCLUDED_SRC_COMBAT_H__
#define __INCLUDED_SRC_COMBAT_H__

#include "lib/framework/frame.h"
#include "objects.h"

/* The range out of which the random number for the to hit should be taken */
#define HIT_DICE	100

/* set a variable to the role of a die between 0 and HIT_DICE */
#define HIT_ROLL(d)  (d) = rand() % HIT_DICE

// maximum difference in direction for a fixed turret to fire
#define FIXED_TURRET_DIR	1

// %age at which a unit is considered to be heavily damaged
#define HEAVY_DAMAGE_LEVEL	25

/* Initialise the combat system */
extern BOOL combInitialise(void);

/* Shutdown the combat system */
extern BOOL combShutdown(void);

/* Fire a weapon at something added int weapon_slot*/
extern void combFire(WEAPON *psWeap, BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget, int weapon_slot);

/*checks through the target players list of structures and droids to see
if any support a counter battery sensor*/
extern void counterBatteryFire(BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget);

extern float objDamage(BASE_OBJECT *psObj, UDWORD damage, UDWORD originalhp, UDWORD weaponClass, 
                       UDWORD weaponSubClass, HIT_SIDE impactSide);

#endif // __INCLUDED_SRC_COMBAT_H__
