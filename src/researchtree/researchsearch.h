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
 *  Finding a research topic by what the player calls it.
 *
 *  A topic may be searched for by the thing it gives as well as by its own name,
 *  so what it unlocks is searched too. Someone after the Lancer is thinking of
 *  the weapon, not of "R-Wpn-Rocket01-LtAT".
 *
 *  Nothing here may depend on lib/widget.
 */

#ifndef __INCLUDED_SRC_RESEARCHTREE_RESEARCHSEARCH_H__
#define __INCLUDED_SRC_RESEARCHTREE_RESEARCHSEARCH_H__

#include "lib/framework/frame.h"
#include "lib/framework/wzstring.h"

#include <cstdint>
#include <string>
#include <vector>

struct ResearchSearchHit
{
	uint16_t researchIndex = 0;
	// What was matched, when it was not the topic's own name.
	// Worth showing, since otherwise a result has no visible reason to be in the list.
	WzString via;
};

class ResearchSearchIndex
{
public:
	// Over the whole research list, so it:
	// - has to be rebuilt if stats reload visible
	// - is indexed by research index
	// - omits anything that the player has not discovered
	void build(const std::vector<bool> *visible = nullptr);

	// Best matches first. An empty or too-short query finds nothing.
	std::vector<ResearchSearchHit> find(const WzString& query, size_t maxResults) const;

private:
	// One searchable string, and where it came from
	struct Entry
	{
		std::string folded;		// what is matched against
		WzString original;		// what is shown, when this is not the name
		uint16_t researchIndex = 0;
		uint8_t kind = 0;		// lower sorts first, see SOURCE_ order
	};

	std::vector<Entry> m_entries;
};

#endif // __INCLUDED_SRC_RESEARCHTREE_RESEARCHSEARCH_H__
