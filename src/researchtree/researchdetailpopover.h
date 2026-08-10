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
 *  Research detail popover.
 *
 *  Built as popover contents rather than a docked panel. The canvas is several
 *  screens wide and the overview depends on seeing enough of it at once, so a
 *  panel that permanently costs a fifth of the width takes it from the axis that
 *  can least afford it. This costs nothing when nothing is open.
 */

#ifndef __INCLUDED_SRC_RESEARCHTREE_RESEARCHDETAILPOPOVER_H__
#define __INCLUDED_SRC_RESEARCHTREE_RESEARCHDETAILPOPOVER_H__

#include "lib/widget/widget.h"
#include "lib/widget/button.h"
#include "researchtreelayout.h"

#include <functional>
#include <vector>
#include <memory>

// Panel describing one topic.
// The background is deliberately not solid, so the edges the canvas draws underneath stay
// faintly readable through it, and it recedes further while the view is being moved.
// Pointing at it makes it solid.
class WzResearchDetailContents : public WIDGET
{
public:
	static constexpr int32_t MAX_PANEL_HEIGHT = 400;

	// `group` and `status` are badges beside the model
	// Either may be null, and so may the `model`, the `subtitle` and the `body`, in the case of a panel that describes nothing.
	static std::shared_ptr<WzResearchDetailContents> make(std::shared_ptr<WIDGET> model, std::shared_ptr<WIDGET> title, std::shared_ptr<WIDGET> subtitle, std::shared_ptr<WIDGET> group, std::shared_ptr<WIDGET> status, std::vector<std::shared_ptr<WIDGET>> stats, std::shared_ptr<WIDGET> body, std::shared_ptr<WIDGET> actions, int32_t contentWidth);

	// What the background settles at when the pointer is elsewhere
	void setRestingAlpha(uint8_t alpha) { m_restingAlpha = alpha; }
	// The tallest the panel may be. (Whatever the body cannot fit, scrolls.)
	// Call before the popover is made, since that takes the size as it stands.
	void setMaxHeight(int32_t maxHeight);

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;
	int32_t idealWidth() override { return width(); }
	int32_t idealHeight() override { return height(); }

private:
	void recalcHeight();
	int headerHeight() const;
	// A topic still worth starting carries data in its header, which leaves no room for
	// its status, so that goes in a band of its own underneath. A topic already researched
	// has no data and displays its status as a badge in the header instead.
	bool statusInBar() const { return !m_stats.empty(); }
	int statusBarHeight() const;
	// Whether the header has anything under it to be divided from. A researched
	// topic with no prose, no effects and nothing to unlock has nothing, and only
	// grows a button row once the panel is pinned.
	bool anythingUnderHeader() const;

	std::shared_ptr<WIDGET> m_model;
	std::shared_ptr<WIDGET> m_title;
	std::shared_ptr<WIDGET> m_subtitle;
	std::shared_ptr<WIDGET> m_group;		// null on a topic that stands alone
	std::shared_ptr<WIDGET> m_status;
	// What it costs, what it takes, how long it will be. (None on a topic already held.)
	std::vector<std::shared_ptr<WIDGET>> m_stats;
	std::shared_ptr<WIDGET> m_body;
	std::shared_ptr<WIDGET> m_actions;		// null on a panel that only describes
	int32_t m_maxHeight = MAX_PANEL_HEIGHT;
	uint8_t m_restingAlpha = 232;
};

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

// Contents for a popover describing one layout unit.
// Returns nothing when the unit does not resolve to a real topic.
//
// `subject` names which of the unit's topics the panel is about. (A pip is a topic, so
// pointing at one inquires about that topic rather than about the progression.)
// `width` is what the panel takes unless its header wants more, `maxWidth` is the limit.
// The body wraps to whatever it is given, so only the title and the badges can ask for room.
// `known` indicates whether the subject may be described at all.
std::shared_ptr<WzResearchDetailContents> makeResearchDetailContents(const ResearchGraph& graph, const LayoutUnit& unit, const ResearchTreeContext& context, int32_t width, int32_t maxWidth, const ResearchDetailActions& actions = ResearchDetailActions(), optional<uint16_t> subject = nullopt, bool known = true);

// A small bordered button, shared by a panel's actions and the view's toolbar
std::shared_ptr<W_BUTTON> makeResearchActionButton(const WzString& text, const WzString& tip);
// The same button without the accent, for secondary buttons beside whatever is the primary action
std::shared_ptr<W_BUTTON> makeResearchQuietButton(const WzString& text, const WzString& tip);

#endif // __INCLUDED_SRC_RESEARCHTREE_RESEARCHDETAILPOPOVER_H__
