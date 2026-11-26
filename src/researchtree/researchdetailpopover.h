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
/** @file
 *  Research detail popover.
 *
 *  Built as popover contents rather than a docked panel. The canvas is several
 *  screens wide and the overview depends on seeing enough of it at once, so a
 *  panel that permanently costs a fifth of the width takes it from the axis that
 *  can least afford it. This costs nothing when nothing is open.
 *
 *  The panel itself is a detail panel (src/widgets/detailpanel/), shared with the HUD:
 *  this only works out which topic a layout unit stands for.
 */

#ifndef __INCLUDED_SRC_RESEARCHTREE_RESEARCHDETAILPOPOVER_H__
#define __INCLUDED_SRC_RESEARCHTREE_RESEARCHDETAILPOPOVER_H__

#include "../widgets/detailpanel/research/researchdetail.h"
#include "researchtreelayout.h"

// Contents for a popover describing one layout unit.
// Returns nothing when the unit does not resolve to a real topic.
//
// `subject` names which of the unit's topics the panel is about. (A pip is a topic, so
// pointing at one inquires about that topic rather than about the progression.)
// `width` is what the panel takes unless its header wants more, `maxWidth` is the limit.
// The body wraps to whatever it is given, so only the title and the badges can ask for room.
// `known` indicates whether the subject may be described at all.
std::shared_ptr<DetailPanelContents> makeResearchDetailContents(const ResearchGraph& graph, const LayoutUnit& unit, const ResearchTreeContext& context, int32_t width, int32_t maxWidth, const ResearchDetailActions& actions = ResearchDetailActions(), optional<uint16_t> subject = nullopt, bool known = true);

#endif // __INCLUDED_SRC_RESEARCHTREE_RESEARCHDETAILPOPOVER_H__
