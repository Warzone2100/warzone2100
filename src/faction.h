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
/** \file
 *  Definitions for factions.
 */
#ifndef __INCLUDED_FACTION_H__
#define __INCLUDED_FACTION_H__

#include "lib/framework/wzstring.h"
#include "lib/ivis_opengl/imd.h"
#include "src/factionid.h"
#include "structure.h"
#include <map>
#include <unordered_set>
#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

#define NUM_FACTIONS 3

struct FACTION {
	WzString name;
	std::map<WzString, WzString> replaceIMD;
};

optional<WzString> getFactionModelName(const FactionID faction, const WzString& normalFactionName);
iIMDShape* getFactionIMD(const FACTION *faction, iIMDShape* imd);

const FACTION* getPlayerFaction(uint8_t player);
const FACTION* getFactionByID(FactionID faction);

std::unordered_set<FactionID> getEnabledFactions(bool ignoreNormalFaction = false);

const char* to_string(FactionID faction);
const char* to_localized_string(FactionID faction);

#endif
