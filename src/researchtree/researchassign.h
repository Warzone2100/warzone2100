/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
 *  Which lab a topic should go to (plus state details)
 *
 *  NOTE:
 *  Displacing research costs nothing. cancelResearch() keeps currentPoints, and
 *  power is only charged on the tick where that is zero, so a topic picked up
 *  again does not pay twice.
 */

#ifndef __INCLUDED_SRC_RESEARCHTREE_RESEARCHASSIGN_H__
#define __INCLUDED_SRC_RESEARCHTREE_RESEARCHASSIGN_H__

#include "lib/framework/frame.h"
#include "lib/framework/wzstring.h"

#include <cstdint>
#include <vector>

struct RESEARCH;

// A lab a topic could be sent to, and why it reads the way it does
struct ResearchLabOption
{
	// By id rather than by pointer, since a lab can be destroyed while an option
	// describing it is still being held. Resolve with IdToStruct() at the point of use.
	uint32_t facilityId = 0;
	int pointsPerSecond = 0;
	uint32_t modules = 0;
	bool idle = false;		// nothing assigned, so nothing is displaced
	bool onHold = false;
	bool waitingForPower = false;
	bool researchingThis = false;	// already on this very topic, so not worth offering
	bool changing = false;		// a message about this one has not come back yet
	WzString currentSubject;	// what would be displaced, empty when idle
	uint16_t subjectIndex = 0;	// index into asResearch, meaningless when idle
	int currentPercent = 0;
};

// Every lab the player has, in a fixed order, so that a row of them does
// not reshuffle itself as their states change. Ones still being built or
// destroyed are left out. One waiting on the netcode is kept and marked, since
// it is still a lab and anything describing them all has to say so.
std::vector<ResearchLabOption> researchLabsFor(uint32_t player);

// The same, ordered for choosing between them: idle first, since those displace
// nothing, then by how fast they are.
std::vector<ResearchLabOption> researchLabOptionsFor(uint32_t player, const RESEARCH& research);

// Whether the player could start this topic at all, ignoring which lab
bool canStartResearchNow(uint32_t player, uint16_t researchIndex);

#endif // __INCLUDED_SRC_RESEARCHTREE_RESEARCHASSIGN_H__
