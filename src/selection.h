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

#ifndef __INCLUDED_SRC_SELECTION_H__
#define __INCLUDED_SRC_SELECTION_H__

enum SELECTION_CLASS
{
	DS_ALL_UNITS,
	DS_BY_TYPE
};

enum SELECTIONTYPE
{
	DST_UNUSED,
	DST_VTOL,
	DST_VTOL_ARMED,
	DST_HOVER,
	DST_WHEELED,
	DST_TRACKED,
	DST_HALF_TRACKED,
	DST_CYBORG,
	DST_ENGINEER,
	DST_MECHANIC,
	DST_TRANSPORTER,
	DST_REPAIR_TANK,
	DST_SENSOR,
	DST_TRUCK,
	DST_ALL_COMBAT,
	DST_ALL_COMBAT_LAND,
	DST_ALL_COMBAT_CYBORG,
	DST_ALL_DAMAGED,
	DST_ALL_SAME
};

// EXTERNALLY REFERENCED FUNCTIONS
unsigned int selDroidSelection(unsigned int player, SELECTION_CLASS droidClass, SELECTIONTYPE droidType, bool bOnScreen);
unsigned int selDroidDeselect(unsigned int player);
unsigned int selNumSelected(unsigned int player);
void selNextUnassignedUnit();
void selNextSpecifiedBuilding(STRUCTURE_TYPE structType);
void selNextSpecifiedUnit(DROID_TYPE unitType);
// select the n'th command droid
void selCommander(int n);

#endif // __INCLUDED_SRC_SELECTION_H__
