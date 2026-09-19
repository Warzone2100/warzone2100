/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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

struct GameWorld;

// initialise the grid system
bool gridInitialise();

// shutdown the grid system
void gridShutDown();

// Reset the grid system. Called once per update.
// Resets seenThisTick[] to false.
void gridReset(GameWorld& world);

/// A query's results, held in a pooled buffer for the handle's lifetime. Handles are strictly LIFO, so a
/// query made from inside a loop over another query's results has a buffer of its own and neither one
/// disturbs the other.
class GridQuery
{
public:
	~GridQuery();
	// A handle owns a pool slot, so it is neither copied nor moved - a moved-from handle would release
	// the slot twice. Guaranteed copy elision is what lets the factories return one anyway.
	GridQuery(const GridQuery &) = delete;
	GridQuery &operator=(const GridQuery &) = delete;
	GridQuery(GridQuery &&) = delete;
	GridQuery &operator=(GridQuery &&) = delete;

	// The accessors are ref-qualified, so `const GridList &r = gridStartIterate(...).results();` does not
	// compile: the buffer is released at the end of that full expression. Range-for over a factory call is
	// fine, since it names the temporary for the length of the loop.
	const GridList &results() const &;
	const GridList &results() const && = delete;
	GridList::const_iterator begin() const &;
	GridList::const_iterator end() const &;
	GridList::const_iterator begin() const && = delete;
	GridList::const_iterator end() const && = delete;

private:
	explicit GridQuery(unsigned slot_) : slot(slot_) {}
	unsigned slot;

	friend GridQuery gridStartIterate(int32_t x, int32_t y, uint32_t radius);
	friend GridQuery gridStartIterateArea(int32_t x, int32_t y, uint32_t x2, uint32_t y2);
	friend GridQuery gridStartIterateDroidsByPlayer(int32_t x, int32_t y, uint32_t radius, int player);
	friend GridQuery gridStartIterateRepairCandidates(int32_t x, int32_t y, uint32_t radius, int player);
	friend GridQuery gridStartIterateUnseen(int32_t x, int32_t y, uint32_t radius, int player);
};

/// Find all objects within radius.
GridQuery gridStartIterate(int32_t x, int32_t y, uint32_t radius);

/// Find all objects within the rectangle.
GridQuery gridStartIterateArea(int32_t x, int32_t y, uint32_t x2, uint32_t y2);

/// Find all objects within radius where object->type == OBJ_DROID && object->player == player.
GridQuery gridStartIterateDroidsByPlayer(int32_t x, int32_t y, uint32_t radius, int player);

/// Find all objects within radius where (object->type == OBJ_DROID && !object->died)
GridQuery gridStartIterateRepairCandidates(int32_t x, int32_t y, uint32_t radius, int player);

// Used for visibility.
/// Find all objects within radius where object->seenThisTick[player] != 255.
GridQuery gridStartIterateUnseen(int32_t x, int32_t y, uint32_t radius, int player);

#endif // __INCLUDED_SRC_MAPGRID_H__
