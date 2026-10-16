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
 *  The objectives card: the local player's objectives (src/objectives.h) with their state, progress, a "New" mark and
 *  a highlight when they change, and "Show on map". One component, in three places: docked in the HUD's right column
 *  (the Objectives module: expanded, one line, or a chip), pinned in the intelligence screen, and popping up over the
 *  reticule without the card (the Objectives option).
 */

#ifndef __INCLUDED_SRC_HUD_INTEL_OBJECTIVESCARD_H__
#define __INCLUDED_SRC_HUD_INTEL_OBJECTIVESCARD_H__

#include "../../objectives.h"

#include "lib/framework/geometry.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/widget.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class HudModule;

/// The docked card's state (as the player chose it, for the rest of the mission)
enum class ObjectivesDockState : uint8_t
{
	Expanded = 1,
	OneLine = 2,
	Minimized = 3
};

enum class ObjectivesCardMode : uint8_t
{
	Docked,   ///< in the right column: its header's buttons fold it
	Pinned,   ///< in the intelligence screen: its rows are selected
	PopUp     ///< over the reticule, for a few seconds
};

class ObjectivesCard : public WIDGET
{
public:
	static std::shared_ptr<ObjectivesCard> make(ObjectivesCardMode mode);

	/// One line: the first objective still to do (docked only)
	void setOneLine(bool oneLine);
	bool isOneLine() const { return oneLine; }
	/// The height of the whole card at its width (one line: the line)
	int fullHeight();
	/// Its height at its width with every row, one line or not
	int expandedHeight();

	/// Docked: the header (or its chevron) clicked, and its minimize button
	std::function<void ()> onToggle;
	std::function<void ()> onMinimize;
	/// Pinned: a row's selection
	std::function<bool (const Objective &objective)> isSelected;
	std::function<void (const Objective &objective)> onSelect;
	/// Pinned: the rows that don't fit, clicked ("3 more")
	std::function<void ()> onMore;

	void run(W_CONTEXT *context) override;
	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;

	// (For its widgets)
	ObjectivesCardMode cardMode() const { return mode; }

private:
	void initialize(ObjectivesCardMode cardMode);
	void reload();
	void layoutRows();

	ObjectivesCardMode mode = ObjectivesCardMode::Docked;
	bool oneLine = false;
	uint32_t revision = 0;
	int cachedWidth = -1;
	int cachedHeight = 0;
	std::vector<Objective> objectives;
	std::vector<std::shared_ptr<WIDGET>> rows;
	std::shared_ptr<WIDGET> header;
	std::shared_ptr<WIDGET> minimizeButton;
	std::shared_ptr<WIDGET> toggleButton;
	std::shared_ptr<WIDGET> moreButton;
	WzText titleText;
	WzText countText;
	WzText lineText;
	WzText plusText;
};

/// The objectives' card title: the mission's name (its first briefing's title), or "Objectives"
std::string hudObjectivesTitle();
/// "1 of 2 done" (the primary objectives), or empty
std::string hudObjectivesDoneText(const std::vector<Objective> &objectives);
/// The icon of an objective's state ("objective-done"), and its color
std::string hudObjectiveIcon(const Objective &objective);
PIELIGHT hudObjectiveColor(const Objective &objective);
/// Whether its marker is a place on the map, and centering the view on it
bool hudObjectiveHasPlace(const Objective &objective);
void hudShowObjectiveOnMap(const Objective &objective);
/// A key for an objective (its id and player)
std::string hudObjectiveKey(const Objective &objective);
/// Text broken into lines of at most `width` (the last one cut with an ellipsis when drawn)
std::vector<std::string> hudWrapText(const std::string &text, iV_fonts font, int width, size_t maxLines);

/// The Objectives module (hudmodules.cpp)
std::unique_ptr<HudModule> hudMakeObjectivesCard();

/// The HUD shows the local player's objectives (registered while the in-game HUD is up: the console doesn't print them)
void hudObjectivesInitialize();
void hudObjectivesShutdown();
/// Each frame: notes what changed (its "New" mark, its highlight), pops up or marks the intelligence button without the
/// card, and shows the docked card when there are objectives
void hudObjectivesFrame();
/// A level starts (from a save: what the save brings isn't news)
void hudObjectivesLevelStarts(bool fromSave);
/// The objectives changed and the player hasn't seen it (without the card): a dot on the intelligence button
bool hudObjectivesUnseen();

/// For the HUD geometry dump: "sample" (a mission's objectives, as set by a script), "card=<expanded|line|minimized>",
/// "changed=<id>" (just changed: marked new and highlighted), "remind" (showObjectives()), "popup"
bool hudObjectivesApplyForTests(const std::string &setting);
/// Before each state: no pop-up and no marks (the objectives and card state that a state set are forgotten)
void hudObjectivesResetForTests();

#endif // __INCLUDED_SRC_HUD_INTEL_OBJECTIVESCARD_H__
