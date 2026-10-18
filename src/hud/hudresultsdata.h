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
 *  A finished skirmish or multiplayer game, for its results screen: its players and sides, their figures over time and
 *  at the end (from the game history, `GameStoryLogger`), their research, and the Timeline's key moments and phases.
 */

#ifndef __INCLUDED_SRC_HUD_HUDRESULTSDATA_H__
#define __INCLUDED_SRC_HUD_HUDRESULTSDATA_H__

#include "lib/framework/geometry.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/piepalette.h"

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <nonstd/optional.hpp>

enum class HudResultsMetric
{
	Army,
	ArmyValue,
	Derricks,
	Kills,
	PowerDestroyed,
	PowerLost,
	Research,
	LabUse,
	Structures,
	Score,
};
constexpr size_t HUD_RESULTS_METRICS = 10;

const char *hudResultsMetricName(HudResultsMetric metric);
const char *hudResultsMetricUnit(HudResultsMetric metric);

struct HudResultsPlayer
{
	uint32_t index = 0;
	std::string name;
	PIELIGHT color;
	int32_t team = 0;
	size_t side = 0;
	std::string about;      ///< the AI and its difficulty, or a person's games played and won
	bool you = false;
	bool won = false;
	bool lost = false;

	// At the end
	uint32_t kills = 0;
	uint32_t structureKills = 0;
	uint32_t built = 0;
	uint32_t lost_ = 0;
	uint32_t structuresBuilt = 0;
	uint32_t structuresLost = 0;
	uint64_t powerDestroyed = 0;
	uint64_t powerLost = 0;
	uint32_t derricks = 0;
	uint32_t derricksPeak = 0;
	uint32_t ownResearch = 0;   ///< by its own labs
	uint32_t research = 0;      ///< researched by it or (shared) its team
	float labUse = 0.f;
	int32_t score = 0;
	uint32_t army = 0;
	uint32_t firsts = 0;        ///< main technologies before any other side
};

struct HudResultsResearch
{
	float time = 0.f;
	size_t player = 0;    ///< index into players
	size_t side = 0;
	std::string id;
	std::string name;
	std::string icon;     ///< its category's icon (images/hud/icons)
	std::string line;     ///< its weapon line (a weapon subclass's name), or a group: armor, base, systems
	bool notable = false; ///< it gives something new (a weapon, body, propulsion or structure), not only an upgrade
};

/// A topic more than one side researched, and how far apart: the "who got there first" rows
struct HudResultsRace
{
	std::string id;
	std::string name;
	std::string icon;
	std::vector<std::pair<size_t, float>> sides;   ///< (side, time), earliest first
	float lead = 0.f;                              ///< the first side's lead over the next
};

/// A line of research: a weapon subclass (as the spectator statistics' columns), or a group of the rest
struct HudResearchLine
{
	std::string key;
	std::string name;
	int weaponSubclass = -1;   ///< (WEAPON_SUBCLASS) or -1
	std::string icon;          ///< for a group: its icon (images/hud/icons)
	PIELIGHT color;
	size_t count = 0;
};

struct HudResultsMoment
{
	float time = 0.f;
	std::string kind;     ///< out, fight, derricks, lead, research, vtol, aa, end
	int weight = 0;
	std::string text;
	std::string detail;
	std::vector<size_t> players;   ///< indices into players
	nonstd::optional<size_t> side;
};

struct HudResultsPhase
{
	float from = 0.f;
	float to = 0.f;
	nonstd::optional<size_t> side;   ///< leading, or none (even)
	bool farAhead = false;
};

/// A standout: who did best at something (the Overview's cards)
struct HudResultsStandout
{
	std::string title;
	std::vector<size_t> players;   ///< indices into players: the best, ties included
	std::string value;
};

class HudGameResults
{
public:
	/// The game so far, as logged (the frames up to now). `youWon`: the local player's result, as the game reported it
	static HudGameResults collect(nonstd::optional<bool> youWon = nonstd::nullopt);

	bool empty() const { return times.empty() || players.empty(); }

	std::vector<float> times;    ///< seconds, one per frame
	float end = 1.f;
	std::vector<HudResultsPlayer> players;   ///< by side, then position
	bool teamGame = false;
	/// Allies share their research (so each has the team's research completed)
	bool sharedResearch = false;
	std::vector<std::vector<size_t>> sides;  ///< players' indices
	std::vector<std::string> sideNames;
	/// Who won and lost, when the game's rules said so for every player (the "usertype" each is given, as the end
	/// conditions do), in agreement with the local player's result. Otherwise unknown, and no one is marked
	bool resultsKnown = false;
	std::vector<bool> sideWon;
	nonstd::optional<size_t> youIndex;
	std::array<std::vector<std::vector<float>>, HUD_RESULTS_METRICS> series;   ///< [metric][player][frame]
	std::vector<HudResultsResearch> research;   ///< by their own labs (not a teammate's shared copy), in time order
	std::vector<HudResultsMoment> moments;
	size_t momentsFound = 0;
	std::vector<HudResultsPhase> phases;
	std::vector<HudResultsStandout> standouts;
	std::vector<std::string> categories;        ///< the research categories seen, most first
	std::vector<HudResearchLine> lines;         ///< the lines seen: weapon lines, most first, then the groups
	std::vector<HudResultsRace> races;          ///< notable topics two sides or more researched, the biggest leads first

	const HudResearchLine *lineFor(const std::string &key) const;

	const std::vector<float> &values(HudResultsMetric metric, size_t player) const;
	/// A side's sum of a metric over time
	std::vector<float> sideValues(HudResultsMetric metric, size_t side) const;

private:
	void computeMoments();
	void computeStandouts();
};

/// The weapon subclasses' pictures (images/intfac/wsc_*.png), each loaded when first asked for, freed with the last holder
class HudWeaponSubclassTextures
{
public:
	HudWeaponSubclassTextures() = default;
	HudWeaponSubclassTextures(const HudWeaponSubclassTextures &) = delete;
	HudWeaponSubclassTextures &operator=(const HudWeaponSubclassTextures &) = delete;
	~HudWeaponSubclassTextures();
	gfx_api::texture *get(int weaponSubclass);

private:
	std::map<int, gfx_api::texture *> textures;
};
/// The pictures, shared while held (a results screen holds them while up)
std::shared_ptr<HudWeaponSubclassTextures> hudWeaponSubclassTextures();
/// A research line's picture, in a box
void hudDrawResearchLineIcon(const HudResearchLine &line, const WzRect &rect);

/// The leader (in Who Led and the phases) leads by at least this share of their figure, with this many sides
float hudLeadShare(size_t sides);
/// And, in army value, by at least this much power
constexpr float HUD_LEAD_VALUE = 300.f;
/// A multiple, as in "Team A has 2× the army value"
std::string hudTimesText(int times);
/// The same, to one decimal (from 10 on, whole)
std::string hudTimesTextOf(double times);

/// A research category's name and color, by its icon
std::string hudResearchCategoryName(const std::string &icon);
PIELIGHT hudResearchCategoryColor(const std::string &icon);
/// The main technologies (the Timeline's research moments, firsts, "who got there first"): id and short name
const std::vector<std::pair<std::string, std::string>> &hudKeyTechnologies();

#endif // __INCLUDED_SRC_HUD_HUDRESULTSDATA_H__
