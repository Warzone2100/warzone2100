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

#ifndef __INCLUDED_SRC_HUD_INTEL_PLAYERSVIEW_H__
#define __INCLUDED_SRC_HUD_INTEL_PLAYERSVIEW_H__

#include "intelmodel.h"

#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/widget.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class ScrollableListWidget;

namespace intelkit
{
class IntelButton;
class IntelConfirm;
}

/// The players view: the multiplayer menu's table, with its columns, rules and actions (src/multimenu.cpp's), the
/// alliances in words, one Give menu and one "…" menu per row, breaking an alliance after a confirmation, and beside
/// it, the latest intelligence. Compact: the row's actions in its "…" menu.
class HudPlayersView : public WIDGET
{
public:
	static std::shared_ptr<HudPlayersView> make(bool compact, bool withLatestIntelligence);

	/// The height it needs for its players at `width`
	int heightFor(int width) const;
	/// A row of the latest intelligence (or "Open intelligence", with an empty key) was clicked
	std::function<void (HudIntelKind kind, const std::string &key)> onOpenIntelligence;

	// Keys: Up and Down through the rows, Enter opens the row's "…" menu
	void moveFocus(int delta);
	void actOnFocus();
	/// Escape: closes the confirmation first. False when there was nothing to close.
	bool backOut();

	/// For the HUD geometry dump: "menu=<give|more>:<row>", "confirm=<row>" (breaking the alliance), "focus=<row>"
	bool applyForTests(const std::string &key, const std::string &value);

	void geometryChanged() override;
	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;

	// (For the rows)
	bool isCompact() const { return compact; }
	bool isFocused(uint32_t player) const;
	void openGiveMenu(uint32_t player, const std::shared_ptr<WIDGET> &anchor);
	void openMoreMenu(uint32_t player, const std::shared_ptr<WIDGET> &anchor);
	void askToBreakAlliance(uint32_t player);
	struct Columns
	{
		int name = 0;
		int alliance = 0;
		int allianceWidth = 0;
		int scoreRight = 0;
		int killsRight = 0;          ///< 0: no column (compact)
		int unitsRight = 0;
		int last = 0;
		int actions = 0;
	};
	const Columns &columns() const { return cols; }

private:
	void initialize(bool compactLayout, bool withLatestIntelligence);
	void rebuildRows();
	void layoutChildren();
	WzRect tableRect() const;
	WzRect railRect() const;

	bool compact = false;
	bool withRail = false;
	std::vector<uint32_t> players;
	nonstd::optional<size_t> focus;
	Columns cols;
	std::shared_ptr<ScrollableListWidget> list;
	std::vector<std::shared_ptr<WIDGET>> rows;
	std::vector<std::shared_ptr<WIDGET>> railRows;
	std::shared_ptr<intelkit::IntelButton> openIntelligence;
	std::shared_ptr<intelkit::IntelConfirm> confirm;
	std::vector<HudIntelEntry> latest;
	uint32_t latestRefreshedAt = 0;
	WzText factsText;
	WzText heads[7];
	WzText railTitle;
	WzText noneText;
};

#endif // __INCLUDED_SRC_HUD_INTEL_PLAYERSVIEW_H__
