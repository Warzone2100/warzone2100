/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
 *  order releated structures.
 */

#ifndef __INCLUDED_SRC_ORDERDEF_H__
#define __INCLUDED_SRC_ORDERDEF_H__

#include "lib/framework/vector.h"

#include "basedef.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

// The droid orders
typedef enum _droid_order
{
	DORDER_NONE,				// no order set

	DORDER_STOP,				// stop the current order
	DORDER_MOVE,				// 2 - move to a location
	DORDER_ATTACK,				// attack an enemy
	DORDER_BUILD,				// 4 - build a structure
	DORDER_HELPBUILD,			// help to build a structure
	DORDER_LINEBUILD,			// 6 - build a number of structures in a row (walls + bridges)
	DORDER_DEMOLISH,			// demolish a structure
	DORDER_REPAIR,				// 8 - repair a structure
	DORDER_OBSERVE,				// keep a target in sensor view
	DORDER_FIRESUPPORT,			// 10 - attack whatever the linked sensor droid attacks
	DORDER_RETREAT,				// return to the players retreat position
	DORDER_DESTRUCT,			// 12 - self destruct
	DORDER_RTB,					// return to base
	DORDER_RTR,					// 14 - return to repair at any repair facility
	DORDER_RUN,					// run away after moral failure
	DORDER_EMBARK,				// 16 - board a transporter
	DORDER_DISEMBARK,			// get off a transporter
	DORDER_ATTACKTARGET,		// 18 - a suggestion to attack something
								// i.e. the target was chosen because the droid could see it
	DORDER_COMMAND,				// a command droid issuing orders to it's group
	DORDER_BUILDMODULE,			// 20 - build a module (power, research or factory)
	DORDER_RECYCLE,				// return to factory to be recycled
	DORDER_TRANSPORTOUT,		// 22 - offworld transporter order
	DORDER_TRANSPORTIN,			// onworld transporter order
	DORDER_TRANSPORTRETURN,		// 24 - transporter return after unloading
	DORDER_GUARD,				// guard a structure
	DORDER_DROIDREPAIR,			// 26 - repair a droid
	DORDER_RESTORE,				// restore resistance points for a structure
	DORDER_SCOUT,				// 28 - same as move, but stop if an enemy is seen
	DORDER_RUNBURN,				// run away on fire
	DORDER_CLEARWRECK,			// 30 - constructor droid to clear up building wreckage
	DORDER_PATROL,				// move between two way points
	DORDER_REARM,				// 32 - order a vtol to rearming pad
	DORDER_MOVE_ATTACKWALL,		// move to a location taking out a blocking wall on the way
	DORDER_SCOUT_ATTACKWALL,	// 34 - scout to a location taking out a blocking wall on the way
	DORDER_RECOVER,				// pick up an artifact
	DORDER_LEAVEMAP,			// 36 - vtol flying off the map
	DORDER_RTR_SPECIFIED,		// return to repair at a specified repair center
	DORDER_CIRCLE = 40,				// circles target location and engage
	DORDER_TEMP_HOLD,		// hold position until given next order
} DROID_ORDER;

// data for barbarians retreating
typedef struct _run_data
{
	Vector2i    sPos; // position to retreat to
	uint8_t     forceLevel; // number of units below which might run
	uint8_t     healthLevel; // %health value below which to turn and run - FOR GROUPS ONLY
	uint8_t     leadership; // basic chance to run
} RUN_DATA;

typedef struct _droid_order_data
{
	SDWORD			order;
	UWORD			x,y;
	UWORD			x2,y2;
	uint16_t                direction;
	BASE_OBJECT		*psObj;
	BASE_STATS		*psStats;
} DROID_ORDER_DATA;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_SRC_ORDERDEF_H__
