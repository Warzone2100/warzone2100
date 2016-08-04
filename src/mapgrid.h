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
 *  Allows querying which objects are within a given radius of a given location.
 */

#ifndef __INCLUDED_SRC_MAPGRID_H__
#define __INCLUDED_SRC_MAPGRID_H__


typedef std::vector<BASE_OBJECT *> GridList;
typedef GridList::const_iterator GridIterator;


// initialise the grid system
extern bool gridInitialise(void);

// shutdown the grid system
extern void gridShutDown(void);

// Reset the grid system. Called once per update.
// Resets seenThisTick[] to false.
extern void gridReset(void);

/// Find all objects within radius.
GridList const &gridStartIterate(int32_t x, int32_t y, uint32_t radius);

/// Find all objects within radius.
GridList const &gridStartIterateArea(int32_t x, int32_t y, uint32_t x2, uint32_t y2);

// Isn't, but could be used by some cluster system. Don't really understand what cluster.c is for.
/// Find all objects within radius where object->type == OBJ_DROID && object->player == player.
GridList const &gridStartIterateDroidsByPlayer(int32_t x, int32_t y, uint32_t radius, int player);

// Used for visibility.
/// Find all objects within radius where object->seenThisTick[player] != 255.
GridList const &gridStartIterateUnseen(int32_t x, int32_t y, uint32_t radius, int player);

#endif // __INCLUDED_SRC_MAPGRID_H__
