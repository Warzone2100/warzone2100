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
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/
/** @file
 *  Construct descriptions of research effects, generated from the topic's results.
 *
 *  Nothing here may depend on lib/widget.
 */

#ifndef __INCLUDED_SRC_RESEARCHTREE_RESEARCHEFFECTTEXT_H__
#define __INCLUDED_SRC_RESEARCHTREE_RESEARCHEFFECTTEXT_H__

#include "lib/framework/frame.h"
#include "lib/framework/wzstring.h"
#include "../researchdef.h"

#include <vector>

struct EffectPhrase
{
	WzString text;		// ex. "+25% damage - A-A"
	bool isBuff = true;	// which way this moves things (for coloring)
};

// One phrase per idea, not per row: a topic that raises the same thing for several weapon kinds,
// or that lowers fire pause and reload time together, is described as one change.
std::vector<EffectPhrase> describeResearchEffects(const RESEARCH& research);

// A title for a progression of upgrades, from the same reading of its results,
// ex. "Cannon Damage". Empty when the topic changes nothing describable.
WzString nameResearchProgression(const RESEARCH& research);

#endif // __INCLUDED_SRC_RESEARCHTREE_RESEARCHEFFECTTEXT_H__
