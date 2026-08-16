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
 *  The research tree view widget.
 *
 *  The canvas draws one player's tree. This adds what surrounds it.
 */

#ifndef __INCLUDED_SRC_RESEARCHTREE_RESEARCHTREEVIEW_H__
#define __INCLUDED_SRC_RESEARCHTREE_RESEARCHTREEVIEW_H__

#include "lib/widget/widget.h"

#include "researchsearch.h"
#include "researchtreelayout.h"

#include <memory>
#include <vector>

class WzResearchTreeCanvas;
class DropdownWidget;
class ScrollableListWidget;
class W_EDITBOX;
class WzResearchLabStrip;
class WzResearchLegend;
class WzResearchTraceBar;

// How far in from its own left edge the view starts its toolbar controls.
// Anything laid out above the view that should line up with them starts here.
int researchTreeContentInset();
// How far the seat switcher writes its text in from its own left edge
// Placing it this much further left is what makes its text, rather than its edge, land on the inset above.
int researchTreeSwitcherTextInset();

class WzResearchTreeView : public WIDGET
{
public:
	static std::shared_ptr<WzResearchTreeView> make(const ResearchTreeContext& context);

	// Show another seat's progress through the same tree, or a whole team's
	void setPerspective(const ResearchPerspective& perspective);
	// Step along the perspectives on offer (wraps)
	void cyclePerspective(int delta);
	// Put a player back on the seat they hold
	void showOwnSeat();
	// Take up any change in what has been researched, without moving anything
	void refreshState();
	// Take down anything standing on an overlay of its own, before whatever is
	// holding the view goes away and leaves it behind
	void closePopovers();

	// Give up one thing at a time: a search, then a traced path, then whatever is picked.
	// False once there is nothing left to give up, which is when the host should close.
	bool backOut();
	// Put the cursor in the search box
	void focusSearch();
	bool searchIsFocused() const;
	// Show only what the selected topic rests on, or go back to the whole tree
	void toggleFocus();
	// Move the pick along the tree (for non-pointer interaction - keybinds, etc)
	void stepSelection(int delta);
	// Whether label names view mode is enabled
	void setLabeled(bool labeled);
	// Jump to the first thing the player could start (for the idle-lab chip)
	void revealSomethingToResearch();
	void toggleLabeled();

	// The seat switcher and the way back to the viewer's own seat belong to the
	// window's title bar, so a match offering more than one perspective does not cost the
	// toolbar a second row. The view makes them and keeps them in step with the seat
	// being read, and whoever holds the view attaches them. Null when there is only one
	// perspective, or nowhere to go back to.
	std::shared_ptr<WIDGET> seatSwitcher() const;
	std::shared_ptr<W_BUTTON> ownSeatChip() const { return m_ownSeatChip; }
	static int ownSeatChipWidth();
	// Fired once the seat being read has changed and everything following from it has settled
	void setOnSeatChanged(std::function<void()> handler) { m_onSeatChanged = std::move(handler); }

	void geometryChanged() override;
	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;

private:
	void initialize(const ResearchTreeContext& context);
	void addPerspectives();
	void addSearch();
	void updateTraceBar();
	// Describe whichever laboratory is being pointed at
	void describeLab(optional<size_t> slot, bool pinned);
	void updateResults();
	bool pointerOverResults() const;
	void revealHit(size_t resultIndex);
	void revealDuplicatedResearch();

	ResearchTreeContext m_context;
	uint32_t m_ownPlayer = 0;			// the seat the viewer holds, whatever they are looking at
	bool m_ownAllowAssignment = false;
	std::shared_ptr<WzResearchTreeCanvas> m_canvas;
	std::shared_ptr<DropdownWidget> m_perspectives;	// null when there is only one perspective to look from
	std::shared_ptr<W_EDITBOX> m_searchBox;
	std::shared_ptr<WIDGET> m_resultsPanel;
	std::shared_ptr<W_BUTTON> m_names;
	std::shared_ptr<WzResearchLabStrip> m_labStrip;	// null outside a live game
	bool m_labPinned = false;
	optional<size_t> m_labPinnedSlot;
	std::shared_ptr<W_BUTTON> m_idleChip;
	std::shared_ptr<W_BUTTON> m_wasteChip;
	std::shared_ptr<W_BUTTON> m_ownSeatChip;	// puts a player back on their own seat
	std::vector<ResearchPerspective> m_seats;
	size_t m_seatAt = 0;
	optional<size_t> m_ownSeat;			// the viewer's own seat, where they have one
	int m_toolbarHeight = 0;
	bool m_namesShown = false;
	size_t m_wastedShown = SIZE_MAX;
	size_t m_idleShown = SIZE_MAX;
	size_t m_labsShown = SIZE_MAX;
	std::shared_ptr<WzResearchLegend> m_legend;
	std::vector<uint8_t> m_lanesShown;
	std::shared_ptr<WzResearchTraceBar> m_traceBar;
	std::shared_ptr<W_BUTTON> m_traceExit;
	std::shared_ptr<ScrollableListWidget> m_results;
	ResearchSearchIndex m_searchIndex;
	std::vector<ResearchSearchHit> m_hits;
	bool m_resultsPutAway = false;
	bool m_searchWasEditing = false;
	std::function<void()> m_onSeatChanged;
	int m_resultsWidth = 0;				// what the current search results want, so the panel is as wide as the names in it
	WzString m_lastQuery;
	bool m_escapeHandled = false;
};

#endif // __INCLUDED_SRC_RESEARCHTREE_RESEARCHTREEVIEW_H__
