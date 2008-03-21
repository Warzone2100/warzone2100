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
 * Game world mechanics.
 */

#ifndef __INCLUDED_SRC_MECHANICS_H__
#define __INCLUDED_SRC_MECHANICS_H__

#include "lib/framework/frame.h"
#include "combat.h"
#include "lib/gamelib/gtime.h"
#include "map.h"
#include "mechanics.h"
#include "move.h"
#include "stats.h"
#include "function.h"
#include "research.h"
#include "visibility.h"

/* Shutdown the mechanics system */
extern BOOL mechShutdown(void);

// Allocate the list for a component
extern BOOL allocComponentList(COMPONENT_TYPE	type, SDWORD number);

// release all the component lists
extern void freeComponentLists(void);

//allocate the space for the Players' structure lists
extern BOOL allocStructLists(void);

// release the structure lists
extern void freeStructureLists(void);

//TEST FUNCTION - MAKE EVERYTHING AVAILABLE
extern void makeAllAvailable(void);

#endif // __INCLUDED_SRC_MECHANICS_H__
