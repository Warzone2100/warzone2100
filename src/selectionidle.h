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

/** \file
 *  Idle factories, research facilities and trucks: finding and selecting them
 */

#ifndef __INCLUDED_SRC_SELECTIONIDLE_H__
#define __INCLUDED_SRC_SELECTIONIDLE_H__

#include "selection.h"
#include "structuredef.h"

struct DROID;
struct STRUCTURE;

/// A built factory or research facility that neither works nor is on hold (even if not yet synchronized)
bool structureIsIdlePending(STRUCTURE *structure);
/// A truck or combat engineer with no order and nothing to do (nor orders not yet synchronized)
bool droidIsIdleTruck(const DROID *psDroid);

/// Select the next idle research facility, or idle factory of any type (for a factory type), and jump to it.
/// Returns false if there is none
bool selNextIdleBuilding(WorldObjectState& objState, STRUCTURE_TYPE structType, bool jump);
/// Select only the next idle truck, and jump to it. Returns false if there is none
bool selNextIdleTruck(WorldObjectState& objState, bool jump);
/// Select only the idle trucks. Returns how many
unsigned selAllIdleTrucks(WorldObjectState& objState);

#endif // __INCLUDED_SRC_SELECTIONIDLE_H__
