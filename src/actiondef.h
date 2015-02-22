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
 * Data types for droid actions
 */

#ifndef __INCLUDED_SRC_ACTIONDEF_H__
#define __INCLUDED_SRC_ACTIONDEF_H__

/**
 * What a droid is currently doing. Not necessarily the same as its order as the micro-AI may get a droid to do
 * something else whilst carrying out an order.
 */
enum DROID_ACTION
{
	DACTION_NONE,					///< 0 not doing anything
	DACTION_MOVE,					///< 1 moving to a location
	DACTION_BUILD,					///< 2 building a structure
	DACTION_UNUSED3,				///< 3 used to be building a foundation for a structure
	DACTION_DEMOLISH,				///< 4 demolishing a structure
	DACTION_REPAIR,					///< 5 repairing a structure
	DACTION_ATTACK,					///< 6 attacking something
	DACTION_OBSERVE,				///< 7 observing something
	DACTION_FIRESUPPORT,				///< 8 attacking something visible by a sensor droid
	DACTION_SULK,					///< 9 refuse to do anything aggresive for a fixed time
	DACTION_DESTRUCT,				///< 10 self destruct
	DACTION_TRANSPORTOUT,				///< 11 move transporter offworld
	DACTION_TRANSPORTWAITTOFLYIN,			///< 12 wait for timer to move reinforcements in
	DACTION_TRANSPORTIN,				///< 13 move transporter onworld
	DACTION_DROIDREPAIR,				///< 14 repairing a droid
	DACTION_RESTORE,				///< 15 restore resistance points of a structure
	DACTION_UNUSED,

	// The states below are used by the action system
	// but should not be given as an action
	DACTION_MOVEFIRE,				///< 17
	DACTION_MOVETOBUILD,				///< 18 moving to a new building location
	DACTION_MOVETODEMOLISH,				///< 19 moving to a new demolition location
	DACTION_MOVETOREPAIR,				///< 20 moving to a new repair location
	DACTION_BUILDWANDER,				///< 21 moving around while building
	DACTION_UNUSED4,				///< 22 used to be moving around while building the foundation
	DACTION_MOVETOATTACK,				///< 23 moving to a target to attack
	DACTION_ROTATETOATTACK,				///< 24 rotating to a target to attack
	DACTION_MOVETOOBSERVE,				///< 25 moving to be able to see a target
	DACTION_WAITFORREPAIR,				///< 26 waiting to be repaired by a facility
	DACTION_MOVETOREPAIRPOINT,			///< 27 move to repair facility repair point
	DACTION_WAITDURINGREPAIR,			///< 28 waiting to be repaired by a facility
	DACTION_MOVETODROIDREPAIR,			///< 29 moving to a new location next to droid to be repaired
	DACTION_MOVETORESTORE,				///< 30 moving to a low resistance structure
	DACTION_UNUSED2,
	DACTION_MOVETOREARM,				///< 32 moving to a rearming pad - VTOLS
	DACTION_WAITFORREARM,				///< 33 waiting for rearm - VTOLS
	DACTION_MOVETOREARMPOINT,			///< 34 move to rearm point - VTOLS - this actually moves them onto the pad
	DACTION_WAITDURINGREARM,			///< 35 waiting during rearm process- VTOLS
	DACTION_VTOLATTACK,				///< 36 a VTOL droid doing attack runs
	DACTION_CLEARREARMPAD,				///< 37 a VTOL droid being told to get off a rearm pad
	DACTION_RETURNTOPOS,				///< 38 used by scout/patrol order when returning to route
	DACTION_FIRESUPPORT_RETREAT,			///< 39 used by firesupport order when sensor retreats
	DACTION_CIRCLE = 41,				///< 41 circling while engaging
};

#endif // __INCLUDED_SRC_ACTIONDEF_H__
