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

#ifndef __INCLUDED_SRC_HUD_INTEL_TEAMSTRATEGYVIEW_H__
#define __INCLUDED_SRC_HUD_INTEL_TEAMSTRATEGYVIEW_H__

#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/widget.h"

#include "../../hci/teamstrategy.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <nonstd/optional.hpp>

class ScrollableListWidget;
namespace gfx_api { struct texture; }

/// A teammate's state before the game starts (the pre-game screen), shown in their row
enum class HudPlanRowStatus : uint8_t
{
	None,
	Loading,
	Ready,
	Left
};

/// What the pre-game screen changes in the view
struct HudTeamStrategyEmbedding
{
	std::string factsLine;                                        ///< instead of "3 on your team · …"
	std::function<HudPlanRowStatus (uint32_t player)> status;
	/// Whether a player's name opens a menu (the host's, for a player still loading), and opening it under `anchor`
	std::function<bool (uint32_t player)> playerHasMenu;
	std::function<void (uint32_t player, const std::shared_ptr<WIDGET> &anchor)> openPlayerMenu;
	int legendRightGutter = 0;                                    ///< kept free at the legend's right
	bool plain = false;                                           ///< no group headings, and a slim legend

	bool sending = true;                                          ///< the local plan goes to the team as it changes
};

/// The team strategy view: what each of the team plans to build, a weapon class or unit type a column, the local
/// player's row first and theirs to set (src/hci/teamstrategy.cpp's plans and rules).
/// Compact: transposed, a class a row and a player a column.
class HudTeamStrategyView : public WIDGET
{
public:
	static std::shared_ptr<HudTeamStrategyView> make(bool compact);
	static std::shared_ptr<HudTeamStrategyView> make(bool compact, HudTeamStrategyEmbedding embedding);
	~HudTeamStrategyView() override;

	/// On: the local plan goes to the team now, and each change as it's made
	void setSending(bool sending);
	/// Read-only: the plans shown, dimmed, and not changed (the game is starting)
	void setReadOnly(bool readOnly) { readOnlyPlans = readOnly; }
	bool isReadOnly() const { return readOnlyPlans; }
	HudPlanRowStatus rowStatus(uint32_t player) const;
	void setFactsLine(std::string line) { embed.factsLine = std::move(line); }
	const HudTeamStrategyEmbedding &embedding() const { return embed; }

	/// The height it needs for its rows at `width` (more scroll)
	int heightFor(int width) const;

	// Keys: the arrows move along the local player's plans, Enter steps the focused one forward, Backspace back
	void moveFocus(int delta);
	void stepFocus(bool forward);

	/// For the HUD geometry dump: "focus=<class>" (0 to 16 the weapon classes, 17 to 19 the unit types)
	bool applyForTests(const std::string &key, const std::string &value);

	void geometryChanged() override;
	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;

	// (For the cells)
	struct PlanClass
	{
		bool unit = false;
		WEAPON_SUBCLASS weapon = WSC_MGUN;
		WzStrategyPlanningUnitTypes unitType = WzStrategyPlanningUnitTypes::Tanks;
	};
	static size_t classCount();
	static PlanClass planClass(size_t index);
	static std::string className(size_t index, bool shortForm);
	static WzStrategyPlanningState state(uint32_t player, size_t index);
	static bool classDisabled(size_t index);
	gfx_api::texture *image(size_t index);
	bool isCompact() const { return compact; }
	int tileSize() const;
	bool isFocused(uint32_t player, size_t index) const;
	void setHovered(nonstd::optional<size_t> index) { hovered = index; }
	const std::vector<uint32_t> &rowPlayers() const { return players; }
	/// The local player's plan for a class, a step forward or back (a click, a right-click or a held click)
	void step(size_t index, bool forward);

	struct Layout
	{
		int nameWidth = 0;
		int pitch = 0;
		int groupGap = 0;
		int rowHeight = 0;
		int header = 0;
		int columnX(size_t index) const;
	};
	const Layout &layout() const { return lay; }

private:
	void initialize(bool compactLayout);
	void layoutChildren();
	WzRect tableRect() const;
	int factsHeight() const;
	int headerHeight() const;
	int legendHeight() const;

	bool compact = false;
	bool readOnlyPlans = false;
	HudTeamStrategyEmbedding embed;
	std::vector<uint32_t> players;      ///< the local player first, then the teammates
	std::vector<gfx_api::texture *> images;
	std::shared_ptr<ScrollableListWidget> list;
	std::vector<std::shared_ptr<WIDGET>> rows;
	std::vector<std::shared_ptr<WIDGET>> heads;   ///< (compact) the players' heads
	size_t focus = 0;
	bool focusShown = false;
	nonstd::optional<size_t> hovered;
	Layout lay;
	WzText factsText;
	WzText groupTexts[3];
	std::vector<WzText> columnNames;
	WzText titleText;
	WzText legendTexts[5];
};

#endif // __INCLUDED_SRC_HUD_INTEL_TEAMSTRATEGYVIEW_H__
