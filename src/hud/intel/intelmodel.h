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

#ifndef __INCLUDED_SRC_HUD_INTEL_INTELMODEL_H__
#define __INCLUDED_SRC_HUD_INTEL_INTELMODEL_H__

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include <nonstd/optional.hpp>

struct MESSAGE;
struct RESEARCH;

/// What the intelligence screen lists for the local player: the research reports received since the campaign (or the
/// game) started, read or not, and the current level's briefings, each with when and in which level it arrived.
///
/// The engine deletes a research report once it has been read, and drops briefings at the end of their level. The
/// screen keeps the reports: a small log notes each report and briefing as it arrives (its game time and level), and
/// is saved with the game (the save's local state). What the log doesn't know (from an older save) is listed without
/// a time.

enum class HudIntelKind
{
	Research,   ///< a research report (a major topic finished)
	Update,     ///< a campaign message
	Briefing    ///< a mission briefing
};

struct HudIntelEntry
{
	HudIntelKind kind = HudIntelKind::Research;
	std::string key;                      ///< the message's view data name
	std::string title;                    ///< the topic's name, or the briefing's title (else "Mission briefing")
	std::string headline;                 ///< briefings: the headline, if any ("TRANSPORT MISSION: Transport Down")
	std::string headlineKind;             ///< briefings: the headline's kind ("TRANSPORT MISSION"), when it has one
	const RESEARCH *research = nullptr;   ///< research reports: the topic
	MESSAGE *message = nullptr;           ///< the message, while the engine keeps it
	nonstd::optional<uint32_t> time;      ///< the game time it arrived, when known
	std::string level;                    ///< the level it arrived in, when known
	bool unread = false;
	std::string searchText;               ///< the title and the text, lower case
};

/// The entries, newest first (those without a time last)
std::vector<HudIntelEntry> hudIntelEntries();
/// A level's name, from the title of the first briefing in it that has one ("Transport Down"), or empty
std::string hudIntelLevelTitle(const std::string &level);
/// The level being played
std::string hudIntelCurrentLevel();
size_t hudIntelUnreadCount();

/// A research report or a script's campaign or mission message arrived (once its view data is set).
/// `announce`: it doesn't play at once (src/hud/intel/intelnotices.h)
void hudIntelNoteMessage(const MESSAGE *message, bool announce = true);
/// The entry a message makes (its time and level not filled in), when the screen lists it
nonstd::optional<HudIntelEntry> hudIntelEntryFor(const MESSAGE *message);
/// A campaign or a game starts, or a save is loaded: the log starts empty
void hudIntelReset();

/// The log, for the save
nlohmann::ordered_json hudIntelSaveLog();
/// The saved log, once the save's messages are loaded (older saves have none)
void hudIntelRestoreLog(const nlohmann::ordered_json &log);

#endif // __INCLUDED_SRC_HUD_INTEL_INTELMODEL_H__
