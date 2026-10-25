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
 *  Detail panels describing a research topic: the same description in the research tree and in the HUD.
 */

#ifndef __INCLUDED_SRC_WIDGETS_DETAILPANEL_RESEARCH_RESEARCHDETAIL_H__
#define __INCLUDED_SRC_WIDGETS_DETAILPANEL_RESEARCH_RESEARCHDETAIL_H__

#include "../detailpanel.h"
#include "../../../researchtree/researchtreelayout.h"

#include <functional>

// A panel that only describes leaves these empty and gets no buttons.
struct ResearchDetailActions
{
	std::function<void()> onTracePath;
	bool tracingPath = false;		// the button says how to get back out again
	// Offered only where a topic can actually be started, so a finished game or a
	// spectator gets a panel that only describes. Handed the button it came from, since
	// anything it opens has to anchor to that.
	std::function<void(const std::shared_ptr<WIDGET>& from)> onResearch;
};

/// The topic to describe, and what whoever shows it knows about it
struct ResearchDetailSubject
{
	uint16_t researchIndex = 0;
	NodeState state = NodeState::Locked;
	/// The progression it is in (if any): shown as a badge. Defaults to the topic's category.
	WzString groupName;
	/// Its place in the progression, ex. "step 2 of 5" (if it has one)
	WzString stepText;
	/// Whether one of its prerequisites is done, for the "Requires" checklist
	std::function<bool (uint16_t researchIndex)> prerequisiteDone;
};

struct ResearchDetailOptions
{
	DetailPanelStyle style = DetailPanelStyle::researchTree();
	/// A smaller panel (the HUD's): a smaller model, no authored description, and short lists
	bool compact = false;
	/// (compact) The most entries in a list ("Unlocks", "Replaces") before "+N more"
	size_t maxListEntries = 4;
	/// The status of a topic that's available or being researched (without it, only the allies researching it)
	bool routineStatus = true;
	/// The list of what the topic requires
	bool requirements = true;
	/// (compact) For a small screen: no model
	bool small = false;
};

/// Contents for a popover describing one research topic.
/// `width` is what the panel takes unless its header wants more, `maxWidth` is the limit.
std::shared_ptr<DetailPanelContents> makeResearchTopicDetailContents(const ResearchDetailSubject& subject, const ResearchTreeContext& context, int32_t width, int32_t maxWidth, const ResearchDetailActions& actions = ResearchDetailActions(), const ResearchDetailOptions& options = ResearchDetailOptions());

/// The topic as the live game sees it for a player (its state from the player's research list)
ResearchDetailSubject researchDetailSubjectForPlayer(uint16_t researchIndex, uint32_t player);

/// All a panel says about a topic that may not be described: that there is something there
std::shared_ptr<DetailPanelContents> makeUndiscoveredResearchDetailContents(int32_t width);

#endif // __INCLUDED_SRC_WIDGETS_DETAILPANEL_RESEARCH_RESEARCHDETAIL_H__
