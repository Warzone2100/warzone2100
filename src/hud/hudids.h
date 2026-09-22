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

#ifndef __INCLUDED_SRC_HUD_HUDIDS_H__
#define __INCLUDED_SRC_HUD_HUDIDS_H__

#include <cstdint>

/// Widget IDs of HUD additions (modules, panel extensions), which the geometry contract (tests/hud) tells apart
/// from the Classic HUD's own widgets.
/// Wrappers (the first block) only reposition Classic widgets and are skipped when naming them.
static constexpr uint32_t IDHUD_WRAPPER_FIRST = 0x7E000000;
static constexpr uint32_t IDHUD_WRAPPER_LAST = 0x7E0000FF;
static constexpr uint32_t IDHUD_ADDITION_FIRST = 0x7E000100;
static constexpr uint32_t IDHUD_ADDITION_LAST = 0x7EFFFFFF;

static constexpr uint32_t IDHUD_STATS_CONTENT = IDHUD_WRAPPER_FIRST + 1;
static constexpr uint32_t IDHUD_OBJECTS_CONTENT = IDHUD_WRAPPER_FIRST + 2;

static constexpr uint32_t IDHUD_STATS_EXTENSION = IDHUD_ADDITION_FIRST + 0x10;
static constexpr uint32_t IDHUD_OBJECTS_EXTENSION = IDHUD_ADDITION_FIRST + 0x20;
static constexpr uint32_t IDHUD_STATUS_LINE = IDHUD_ADDITION_FIRST + 0x30;
static constexpr uint32_t IDHUD_IDLE_INDICATORS = IDHUD_ADDITION_FIRST + 0x40;
static constexpr uint32_t IDHUD_SELECTION_READOUT = IDHUD_ADDITION_FIRST + 0x50;
static constexpr uint32_t IDHUD_SPECTATOR_STRIP = IDHUD_ADDITION_FIRST + 0x60;

inline bool hudIsWrapperId(uint32_t id)
{
	return id >= IDHUD_WRAPPER_FIRST && id <= IDHUD_WRAPPER_LAST;
}

inline bool hudIsAdditionId(uint32_t id)
{
	return id >= IDHUD_ADDITION_FIRST && id <= IDHUD_ADDITION_LAST;
}

#endif // __INCLUDED_SRC_HUD_HUDIDS_H__
