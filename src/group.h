/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
 *  Responsible for handling groups of droids.
 */

#ifndef __INCLUDED_SRC_GROUP_H__
#define __INCLUDED_SRC_GROUP_H__

#include "order.h"

enum GROUP_TYPE
{
	GT_NORMAL,      // standard group
	GT_COMMAND,     // command droid group
	GT_TRANSPORTER, // transporter group
};

class DROID_GROUP
{
public: // TODO: c++ design to members become private.
	DROID_GROUP();

	void add(DROID *psDroid);     // Add a droid to group. Remove it from its group in case it already has group
	void remove(DROID *psDroid);  // Remove droid from group. Free group in case RefCount<=0
	void removeAll();             // Remove all droids from the group
	unsigned int getNumMembers(); // Count the number of members of a group

	void orderGroup(DROID_ORDER order);                     // give an order all the droids of the group
	void orderGroup(DROID_ORDER order, UDWORD x, UDWORD y); // give an order all the droids of the group (using location)
	void orderGroup(DROID_ORDER order, BASE_OBJECT *psObj); // give an order all the droids of the group (using object)

	void setSecondary(SECONDARY_ORDER sec, SECONDARY_STATE state); // set the secondary state for a group of droids

	GROUP_TYPE	type;         // Type from the enum GROUP_TYPE above
	SWORD		refCount;     // Number of objects in the group. Group is deleted if refCount<=0. Count number of droids+NULL pointers.
	DROID		*psList;      // List of droids in the group
	DROID		*psCommander; // The command droid of a command group
	RUN_DATA	sRunData;   // Where the group should retreat to
	int		id;	// unique group id
};

// initialise the group system
bool grpInitialise(void);

// shutdown the group system
void grpShutDown(void);

/// create a new group, use -1 to generate a new ID. never use id != -1 unless loading from a savegame.
DROID_GROUP *grpCreate(int id = -1);

/// lookup group by its unique id, or create it if not found
DROID_GROUP *grpFind(int id);

#endif // __INCLUDED_SRC_GROUP_H__
