/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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

#include "weapondef.h"

/* Fire a weapon at something added int weapon_slot*/
bool combFire(WEAPON *psWeap, BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget, int weapon_slot);

/*checks through the target players list of structures and droids to see
if any support a counter battery sensor*/
void counterBatteryFire(BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget);

int32_t objDamage(BASE_OBJECT *psObj, unsigned damage, unsigned originalhp, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass, bool isDamagePerSecond, int minDamage);

unsigned int objGuessFutureDamage(WEAPON_STATS *psStats, unsigned int player, BASE_OBJECT *psTarget);

int objArmour(BASE_OBJECT *psObj, WEAPON_CLASS weaponClass);

#endif // __INCLUDED_SRC_COMBAT_H__
