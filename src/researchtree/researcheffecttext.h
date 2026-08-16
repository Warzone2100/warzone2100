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
