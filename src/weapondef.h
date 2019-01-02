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
/** \file
 *  Definitions for the weapons.
 */

#ifndef __INCLUDED_WEAPONDEF_H__
#define __INCLUDED_WEAPONDEF_H__

// maximum difference in direction for a fixed turret to fire
#define FIXED_TURRET_DIR DEG(1)

// %age at which a unit is considered to be heavily damaged
#define HEAVY_DAMAGE_LEVEL	25

/* who specified the target? */
enum TARGET_ORIGIN
{
	ORIGIN_UNKNOWN,		///< Default value if unknown
	ORIGIN_PLAYER,		///< Came directly from player
	ORIGIN_VISUAL,		///< Visual targeting
	ORIGIN_ALLY,		///< Came from allied unit/structure
	ORIGIN_COMMANDER,	///< Came from commander
	ORIGIN_SENSOR,		///< Came from standard sensor
	ORIGIN_CB_SENSOR,	///< Came from counter-battery sensor
	ORIGIN_AIRDEF_SENSOR,	///< Came from Air Defense sensor
	ORIGIN_RADAR_DETECTOR,	///< Came from Radar Detector sensor
};

struct WEAPON
{
	uint32_t        nStat;		///< Index into the asWeaponStats global array
	uint32_t        ammo;
	uint32_t        lastFired;	///< The gametime when this weapon last fired
	uint32_t        shotsFired;
	Rotation	rot;
	Rotation	prevRot;
	unsigned        usedAmmo;    ///< Amount of ammunition used up by a VTOL
	TARGET_ORIGIN   origin;

	WEAPON() : nStat(0), ammo(0), lastFired(0), shotsFired(0), usedAmmo(0), origin(ORIGIN_UNKNOWN) {}
};

// Defined in droid.cpp.
int getRecoil(WEAPON const &weapon);  ///< Returns how much the weapon assembly should currently be rocked back due to firing.

#endif // __INCLUDED_WEAPONDEF_H__
