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
 *  Focus filter: What it takes to reach one research topic.
 *
 *  Everything a target depends on, and what is left to pay for it.
 *
 *  Nothing here may depend on lib/widget.
 */

#ifndef __INCLUDED_SRC_RESEARCHTREE_RESEARCHFOCUS_H__
#define __INCLUDED_SRC_RESEARCHTREE_RESEARCHFOCUS_H__

#include "lib/framework/frame.h"

#include "researchtreelayout.h"

#include <cstdint>
#include <vector>

struct ResearchFocus
{
	// By research index, and what the layout filter wants
	std::vector<bool> included;

	size_t topics = 0;		// how much of the tree the target rests on
	size_t remaining = 0;		// of those, how many still to research
	uint64_t remainingPower = 0;
	uint64_t remainingPoints = 0;
	// Longest run of still-unresearched topics that have to follow one another,
	// which is what no amount of extra labs can shorten
	int32_t criticalPath = 0;
};

// Everything `target` depends on, plus the target itself.
// Prerequisites already researched stay in, but they are not counted towards what is left.
ResearchFocus computeResearchFocus(const ResearchGraph& graph, const std::vector<uint16_t>& targetMembers);

#endif // __INCLUDED_SRC_RESEARCHTREE_RESEARCHFOCUS_H__
