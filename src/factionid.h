/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2021  Warzone 2100 Project

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
/** \file
 *  FactionID definitions.
 */

#ifndef __INCLUDED_FACTIONID_H__
#define __INCLUDED_FACTIONID_H__

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

enum FactionID : uint8_t {
	FACTION_NORMAL = 0,
	FACTION_NEXUS = 1,
	FACTION_COLLECTIVE = 2
};
#define MAX_FACTION_ID FACTION_COLLECTIVE

inline optional<FactionID> uintToFactionID(uint8_t value)
{
	if (value > static_cast<uint8_t>(MAX_FACTION_ID))
	{
		return nullopt;
	}
	return static_cast<FactionID>(value);
}

namespace std {
	template <> struct hash<FactionID>
	{
		size_t operator()(const FactionID& faction) const
		{
			return hash<uint8_t>()(static_cast<uint8_t>(faction));
		}
	};
}

#endif
