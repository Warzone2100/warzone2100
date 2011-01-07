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
 *  Link droids together into a group for AI etc.
 */

#ifndef __INCLUDED_SRC_GROUP_H__
#define __INCLUDED_SRC_GROUP_H__

#include "order.h"

enum GROUP_TYPE
{
	GT_NORMAL,			// standard group
	GT_COMMAND,			// command droid group
	GT_TRANSPORTER,		// transporter group
};

struct DROID_GROUP
{
	SWORD		type;
	SWORD		refCount;
	DROID		*psList;			// list of droids in the group
	DROID		*psCommander;		// the command droid of a command group
	RUN_DATA	sRunData;			// where the group should retreat to
	DROID_GROUP     *psNext, *psPrev;       // keep linked to destroy all (a workaround hack)
};

// initialise the group system
BOOL grpInitialise(void);

// shutdown the group system
void grpShutDown(void);

// create a new group
BOOL grpCreate(DROID_GROUP	**ppsGroup);

// add a droid to a group
void grpJoin(DROID_GROUP *psGroup, DROID *psDroid);

// remove a droid from a group
void grpLeave(DROID_GROUP *psGroup, DROID *psDroid);

// count the members of a group
unsigned int grpNumMembers(const DROID_GROUP* psGroup);

// remove all droids from a group
void grpReset(DROID_GROUP *psGroup);

/* Give a group an order */
void orderGroup(DROID_GROUP *psGroup, DROID_ORDER order);

/* Give a group of droids an order */
void orderGroupLoc(DROID_GROUP *psGroup, DROID_ORDER order, UDWORD x, UDWORD y);

/* Give a group of droids an order */
void orderGroupObj(DROID_GROUP *psGroup, DROID_ORDER order, BASE_OBJECT *psObj);

/* set the secondary state for a group of droids */
void grpSetSecondary(DROID_GROUP *psGroup, SECONDARY_ORDER sec, SECONDARY_STATE state);

#endif // __INCLUDED_SRC_GROUP_H__
