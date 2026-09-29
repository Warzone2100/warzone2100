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
 *  Research detail popover contents.
 */

#include "researchdetailpopover.h"

#include "../research.h"

#include <algorithm>

std::shared_ptr<DetailPanelContents> makeResearchDetailContents(const ResearchGraph& graph, const LayoutUnit& unit, const ResearchTreeContext& context, int32_t width, int32_t maxWidth, const ResearchDetailActions& actions, optional<uint16_t> subjectAsked, bool known)
{
	if (unit.members.empty() || unit.members.front() >= asResearch.size())
	{
		return nullptr;
	}

	// Describe whichever step was asked about.
	// Defaults to the one the player looks at next, which for a progression is the first non-researched.
	uint16_t subject = unit.members.front();
	for (const auto member : unit.members)
	{
		const auto node = graph.nodeForResearchIndex(member);
		if (node && graph.nodes()[*node].state == NodeState::Researched)
		{
			subject = member;
		}
	}
	for (const auto member : unit.members)
	{
		const auto node = graph.nodeForResearchIndex(member);
		if (node && graph.nodes()[*node].state != NodeState::Researched)
		{
			subject = member;
			break;
		}
	}
	if (subjectAsked && std::find(unit.members.begin(), unit.members.end(), *subjectAsked) != unit.members.end())
	{
		subject = *subjectAsked;
	}

	if (!known)
	{
		return makeUndiscoveredResearchDetailContents(width);
	}

	ResearchDetailSubject topic;
	topic.researchIndex = subject;
	const auto subjectNode = graph.nodeForResearchIndex(subject);
	topic.state = subjectNode ? graph.nodes()[*subjectNode].state : NodeState::Locked;
	topic.groupName = (unit.members.size() > 1) ? unit.name : WzString();

	// A knot has no order to count along, so nothing there is a step.
	if (!unit.merged && unit.members.size() > 1)
	{
		// The subject's own place in the progression
		const size_t step = static_cast<size_t>(
			std::find(unit.members.begin(), unit.members.end(), subject) - unit.members.begin());
		topic.stepText = WzString::format(_("step %zu of %zu"), step + 1, unit.members.size());
	}
	topic.prerequisiteDone = [&graph](uint16_t prereqIndex) {
		const auto prereqNode = graph.nodeForResearchIndex(prereqIndex);
		return prereqNode && graph.nodes()[*prereqNode].state == NodeState::Researched;
	};

	return makeResearchTopicDetailContents(topic, context, width, maxWidth, actions);
}
