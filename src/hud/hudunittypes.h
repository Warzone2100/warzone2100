// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __INCLUDED_SRC_HUD_HUDUNITTYPES_H__
#define __INCLUDED_SRC_HUD_HUDUNITTYPES_H__

#include <array>
#include <cstddef>
#include <cstdint>

struct DROID;

/// Coarse unit types, for what a group or a selection is made of
enum class HudUnitType : uint8_t
{
	Tank,
	Cyborg,
	Vtol,
	Truck,
	Sensor,
	Repair,
	Commander,
	Transport,
};
constexpr size_t HUD_UNIT_TYPE_COUNT = static_cast<size_t>(HudUnitType::Transport) + 1;

HudUnitType hudUnitTypeOf(const DROID *psDroid);
const char *hudUnitTypeName(HudUnitType type, unsigned count);
/// A short symbol for the type, for chips where there is no room for the name
const char *hudUnitTypeSymbol(HudUnitType type);

using HudUnitCounts = std::array<unsigned, HUD_UNIT_TYPE_COUNT>;

#endif // __INCLUDED_SRC_HUD_HUDUNITTYPES_H__
