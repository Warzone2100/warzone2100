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
 *  The objectives scripts set for the players (setObjective() and the like): what to do, its state and progress, kept
 *  with the game state (saved, and in its snapshots) and shown by the HUD, or printed to the console when the HUD
 *  doesn't show them.
 */

#ifndef __INCLUDED_SRC_OBJECTIVES_H__
#define __INCLUDED_SRC_OBJECTIVES_H__

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <nonstd/optional.hpp>

enum class ObjectiveState : uint8_t
{
	Active,
	Done,
	Failed
};

/// Everyone's objectives: a player of -1
constexpr int32_t OBJECTIVES_ALL_PLAYERS = -1;
/// The version of objectivesWrite()'s document (the game state's "objectives" section)
constexpr uint32_t OBJECTIVES_SECTION_VERSION = 1;

struct ObjectiveOptions
{
	ObjectiveState state = ObjectiveState::Active;
	bool primary = true;                                     ///< secondary ones are listed after the primary ones
	nonstd::optional<std::pair<int32_t, int32_t>> progress;  ///< current, total (ex. 2 of 3 artifacts)
	std::string marker;                                      ///< a proximity message's view data ("Show on map")
	int32_t order = 0;                                       ///< the display order, among the primary or secondary ones
	int32_t player = OBJECTIVES_ALL_PLAYERS;
};

struct Objective
{
	std::string id;
	std::string text;
	ObjectiveOptions options;
	uint32_t addedAt = 0;      ///< the game time it was first set
	uint32_t changedAt = 0;    ///< the game time its text, state or progress last changed
	uint32_t stateAt = 0;      ///< the game time its state last changed (when it was added, while active)
};

/// Adds the objective `id` (for `options.player`, or everyone), or replaces it: options that are left out take their
/// defaults
void objectivesSet(const std::string &id, const std::string &text, const ObjectiveOptions &options);
/// Removes `id` from `player`'s list, or from every list (no player). False when there was none.
bool objectivesRemove(const std::string &id, nonstd::optional<int32_t> player);
/// Empties `player`'s list, or every list (no player)
void objectivesClear(nonstd::optional<int32_t> player);
/// The title over the objectives (empty: the display's own)
void objectivesSetTitle(const std::string &title);
const std::string &objectivesTitle();
/// Brings `player`'s objectives to their attention: the HUD shows them, or the console lists them. A periodic
/// `reminder` is only for the console: the HUD keeps them in view.
void objectivesShow(int32_t player, bool reminder = false);

/// A player's objectives (theirs, and everyone's), in display order: the primary ones first, by order, then as added
std::vector<Objective> objectivesFor(uint32_t player);
/// Changes with every change to any list
uint32_t objectivesRevision();
/// Whether a player has any objectives
bool objectivesAny(uint32_t player);

/// Who shows the local player's objectives instead of the console (the HUD): whether it does now, and how it brings them
/// to the player's attention (showObjectives())
struct ObjectivesDisplay
{
	std::function<bool ()> shows;
	std::function<void ()> bringToAttention;
};
void objectivesSetDisplay(ObjectivesDisplay display);

const char *objectiveStateName(ObjectiveState state);
nonstd::optional<ObjectiveState> objectiveStateFromName(const std::string &name);

/// A level starts: no objectives
void objectivesReset();
nlohmann::ordered_json objectivesWrite();
/// Replaces the lists with a saved document's (an empty one: no objectives)
void objectivesRead(const nlohmann::ordered_json &j);

#endif // __INCLUDED_SRC_OBJECTIVES_H__
