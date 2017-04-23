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
/**
 *
 * @file
 * Definitions for command droids.
 *
 */

#ifndef __INCLUDED_CMDDROIDDEF_H__
#define __INCLUDED_CMDDROIDDEF_H__

#include "statsdef.h"

struct DROID;

/** This defines the maximum number of command droids allowed per player.*/
#define MAX_CMDDROIDS	5

/** This struct defines a command droid.
 * Notice that this struct is not a droid: it is just responsible for the command's logic.
 * This struct uses the psDroid member to point to the actual droid where it lives in.
 * @todo died and psDroid are the only members that are being used somewhere in the code. Consider removing all the others.
 */
struct COMMAND_DROID : public COMPONENT_STATS
{
	UDWORD  died;		/**< Defines if the command has died or not.*/
	SWORD   aggression;
	SWORD   survival;
	SWORD   nWeapStat;
	UWORD   kills;
	DROID  *psDroid;	/**< The droid where the command droid is living in.*/
};

#endif // __INCLUDED_CMDDROIDDEF_H__
