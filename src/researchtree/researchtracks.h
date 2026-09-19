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
 *  Deriving research progressions (ex. MG Damage 1..10).
 *
 *  Nothing here may depend on lib/widget.
 */

#ifndef __INCLUDED_SRC_RESEARCHTREE_RESEARCHTRACKS_H__
#define __INCLUDED_SRC_RESEARCHTREE_RESEARCHTRACKS_H__

#include "lib/framework/frame.h"
#include "lib/framework/wzstring.h"
#include "../researchprereq.h"

#include <cstdint>
#include <vector>

// Where a progression was found.
// Higher tiers only see topics that the lower ones did not claim, so a topic belongs to at most one track.
enum class TrackSource : uint8_t
{
	// The category field, already validated as a chain when the stats loaded.
	Category,
	// Topics whose results blocks change exactly the same things.
	// (Example: every step of an armor upgrade.)
	// Ordered by prerequisites.
	UpgradeSignature,
	// Topics that make each other's components or structures redundant,
	// (Example: a weapon tier line.)
	// Ordered by obsolescence, since tiers are usually prerequisite siblings rather than ancestors.
	Obsolescence,
};

struct ResearchTrack
{
	TrackSource source = TrackSource::Category;
	std::vector<uint16_t> members;	// in progression order, always 2 or more
	// A display name (empty for a progression that cannot be named).
	// - Category tracks carry the authored name
	// - Upgrade signatures carry a name based on what they change.
	// - Obsolescence tracks have neither - a view falls back to naming them by the member the player has reached.
	WzString name;
};

// A track may not swallow an unreasonable part of the tree.
constexpr size_t MAX_RESEARCH_TRACK_MEMBERS = 32;

// Find every progression in the research list.
// Topics not in any track are left out, so the caller keeps them as single nodes.
std::vector<ResearchTrack> deriveResearchTracks(const ResearchPrereqClosure& closure);

// Give a name to every group that has none, from what its members' names have in common.
// (Since that is already translated, nothing here needs a string of its own.)
// A group with less than two words in common, or one that would end up sharing a name
// with another, keeps none (and whatever draws it names it after one of its members).
void nameUnnamedTracks(std::vector<ResearchTrack>& tracks);

// The tech category a topic sits in (ex: "Weapons").
// Empty where the icon maps to a value this list does not name.
WzString researchTechCategoryName(const RESEARCH& research);

#endif // __INCLUDED_SRC_RESEARCHTREE_RESEARCHTRACKS_H__
