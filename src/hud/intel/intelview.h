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

#ifndef __INCLUDED_SRC_HUD_INTEL_INTELVIEW_H__
#define __INCLUDED_SRC_HUD_INTEL_INTELVIEW_H__

#include "intelmodel.h"

#include "lib/widget/widget.h"

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

class HudChip;
class W_EDITBOX;
class ScrollableListWidget;
struct W_SCREEN;

enum class IntelFilter : uint8_t
{
	All,
	Briefings,
	Research,
	Unread
};

enum class IntelOrder : uint8_t
{
	Newest,
	ByKind
};

/// The intelligence screen's list and the item it shows: the filters and search, the latest mission briefing pinned,
/// the reports and briefings newest first, grouped (new, then by mission), and the selected one beside them.
/// Compact: the filters and search fold into the header's accessory.
class IntelView : public WIDGET
{
public:
	static std::shared_ptr<IntelView> make(bool compact);

	/// Selects the entry of `message` (or the first unread, or the first)
	void selectMessage(const MESSAGE *message);
	/// Plays the selected briefing's videos, full screen
	void playSelected();
	void moveSelection(int delta);
	void focusSearch();
	/// Escape: gives up the search first. False when there was nothing to give up.
	bool backOut();
	bool searchIsEditing() const;
	/// For the panel's header, beside its close button: the search field (compact layout: the filter and search buttons,
	/// the search field over them once it's opened)
	std::shared_ptr<WIDGET> headerAccessory() const { return accessory; }
	size_t unreadCount() const;
	/// The screen the view is shown on (the research tree opens over it)
	std::function<std::shared_ptr<W_SCREEN> ()> screen;
	std::function<void ()> onUnreadChanged;

	bool applyForTests(const std::string &key, const std::string &value);

	void run(W_CONTEXT *psContext) override;
	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;

	// (For the widgets of the view)
	const HudIntelEntry *entry(size_t index) const;
	bool isSelected(size_t index) const;
	bool isCompact() const { return compact; }
	void select(size_t index, bool byPointer);
	std::string subtitle(const HudIntelEntry &entry) const;

private:
	void initialize(bool compactLayout);
	void reload();
	void rebuildList();
	void rebuildDetail();
	void updateChips();
	bool passes(const HudIntelEntry &entry) const;
	void setFilter(IntelFilter newFilter);
	void setOrder(IntelOrder newOrder);
	void openFilterMenu(const std::shared_ptr<WIDGET> &anchor, bool withFilters);
	void markUnread();
	void showInResearchTree();
	void layoutChildren();
	WzRect listRect() const;
	WzRect detailRect() const;

	bool compact = false;
	std::vector<HudIntelEntry> entries;
	std::set<std::string> newAtOpen;                  ///< what was unread when the screen opened: the "New" section
	std::vector<size_t> shown;                        ///< the list's rows, in order (indexes into entries)
	nonstd::optional<size_t> selected;
	nonstd::optional<size_t> pinned;
	IntelFilter filter = IntelFilter::All;
	IntelOrder order = IntelOrder::Newest;
	std::string search;
	std::shared_ptr<ScrollableListWidget> list;
	std::shared_ptr<WIDGET> pinnedCard;
	std::shared_ptr<WIDGET> emptyLabel;
	std::shared_ptr<WIDGET> detail;
	std::vector<std::shared_ptr<HudChip>> chips;      ///< (full layout) All, Briefings, Research, Unread
	std::shared_ptr<WIDGET> orderChip;                ///< (full layout) the order's menu
	std::shared_ptr<W_EDITBOX> searchBox;
	std::shared_ptr<WIDGET> searchField;              ///< the search box with its icon
	std::shared_ptr<WIDGET> accessory;
	std::shared_ptr<WIDGET> filterButton;             ///< (compact layout)
	std::shared_ptr<WIDGET> searchButton;             ///< (compact layout)
	bool searchShown = false;                         ///< (compact layout) the search field over the header's buttons
};

#endif // __INCLUDED_SRC_HUD_INTEL_INTELVIEW_H__
