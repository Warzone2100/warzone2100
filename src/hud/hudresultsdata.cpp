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

#include "hudresultsdata.h"
#include "hudchart.h"
#include "hudresults.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"

#include "../ai.h"
#include "../component.h"
#include "../gamehistorylogger.h"
#include "../intimage.h"
#include "../multiint.h"
#include "../multiplay.h"
#include "../multistat.h"
#include "../research.h"
#include "../stats.h"
#include "../structure.h"
#include "hudskin.h"

#include "lib/ivis_opengl/pieblitfunc.h"

#include <algorithm>
#include <functional>
#include <cmath>
#include <map>
#include <set>

#include <fmt/core.h>

namespace
{

constexpr float WINDOW = 120.f;   // s: fights, derrick losses and leads are measured over two minutes
constexpr float FAR_VALUE = 750.f; // power: a lead by far (with 2× the army value)
constexpr size_t MAX_MOMENTS = 10;

const std::map<std::string, size_t> CAPS = {{"fight", 3}, {"battle", 2}, {"research", 3}, {"derricks", 2}, {"lead", 2}, {"out", 99}, {"end", 1},
                                             {"vtol", 1}, {"aa", 1}};

std::string researchIconName(UWORD iconId)
{
	switch (iconId)
	{
	case IMAGE_RES_DROIDTECH: return "research-droid";
	case IMAGE_RES_WEAPONTECH: return "research-weapon";
	case IMAGE_RES_CYBORGTECH: return "research-cyborg";
	case IMAGE_RES_SYSTEMTECH: return "research-sensor";
	case IMAGE_RES_COMPUTERTECH: return "research-computer";
	case IMAGE_RES_POWERTECH: return "research-power";
	case IMAGE_RES_DEFENCE: return "research-defense";
	default: return "research-structure";
	}
}

/// The weapon subclass with this name, if it is one (getWeaponSubClass(const char *) asserts when it isn't)
nonstd::optional<WEAPON_SUBCLASS> weaponSubclassNamed(const std::string &name)
{
	for (unsigned i = 0; i < WSC_NUM_WEAPON_SUBCLASSES; ++i)
	{
		const WEAPON_SUBCLASS subclass = static_cast<WEAPON_SUBCLASS>(i);
		if (name == getWeaponSubClass(subclass))
		{
			return subclass;
		}
	}
	return nonstd::nullopt;
}

/// A research topic's line: the weapon subclass it gives (a weapon, a defense with one) or upgrades, else a group
std::string researchLineKey(const RESEARCH *research)
{
	if (!research)
	{
		return "base";
	}
	for (const COMPONENT_STATS *c : research->componentResults)
	{
		if (c && c->compType == COMP_WEAPON)
		{
			return getWeaponSubClass(static_cast<const WEAPON_STATS *>(c)->weaponSubClass);
		}
	}
	if (research->results.is_array())
	{
		for (const auto &r : research->results)
		{
			if (r.is_object() && r.value("class", "") == "Weapon" && r.value("filterParameter", "") == "ImpactClass" && r.contains("filterValue") && r["filterValue"].is_string())
			{
				const std::string value = r["filterValue"].get<std::string>();
				if (weaponSubclassNamed(value))
				{
					return value;
				}
			}
		}
	}
	for (UWORD id : research->pStructureResults)
	{
		if (asStructureStats && id < numStructureStats && asStructureStats[id].numWeaps > 0 && asStructureStats[id].psWeapStat[0])
		{
			return getWeaponSubClass(asStructureStats[id].psWeapStat[0]->weaponSubClass);
		}
	}
	if (research->results.is_array())
	{
		for (const auto &r : research->results)
		{
			if (r.is_object() && r.value("class", "") == "Body")
			{
				return "armor";
			}
		}
	}
	for (const COMPONENT_STATS *c : research->componentResults)
	{
		if (c && (c->compType == COMP_BODY || c->compType == COMP_PROPULSION))
		{
			return "armor";
		}
		if (c && (c->compType == COMP_SENSOR || c->compType == COMP_ECM || c->compType == COMP_REPAIRUNIT || c->compType == COMP_BRAIN || c->compType == COMP_CONSTRUCT))
		{
			return "systems";
		}
	}
	return "base";
}

bool isVtolResearch(const std::string &id)
{
	return id == "R-Vehicle-Prop-VTOL" || id == "R-Struc-VTOLFactory";
}

bool isAntiAirResearch(const std::string &id)
{
	for (const char *prefix : {"R-Wpn-AAGun", "R-Defense-AASite", "R-Defense-WallTower-DoubleAAgun", "R-Defense-WallTower-QuadRotAA",
	                           "R-Wpn-AALaser", "R-Defense-AA-Laser", "R-Defense-Sunburst", "R-Wpn-Sunburst"})
	{
		if (id.compare(0, strlen(prefix), prefix) == 0)
		{
			return true;
		}
	}
	return false;
}

std::string clock(float seconds)
{
	const uint32_t s = static_cast<uint32_t>(std::max(0.f, seconds));
	return s >= 3600 ? fmt::format("{}:{:02}:{:02}", s / 3600, (s / 60) % 60, s % 60) : fmt::format("{}:{:02}", s / 60, s % 60);
}

}

const char *hudResultsMetricName(HudResultsMetric metric)
{
	switch (metric)
	{
	case HudResultsMetric::Army: return _("Army size");
	case HudResultsMetric::ArmyValue: return _("Army value");
	case HudResultsMetric::Derricks: return _("Derricks");
	case HudResultsMetric::Kills: return _("Kills");
	case HudResultsMetric::PowerDestroyed: return _("Power destroyed");
	case HudResultsMetric::PowerLost: return _("Power lost");
	case HudResultsMetric::Research: return _("Research completed");
	case HudResultsMetric::LabUse: return _("Lab use");
	case HudResultsMetric::Structures: return _("Structures");
	case HudResultsMetric::Score: return _("Score");
	}
	return "";
}

const char *hudResultsMetricUnit(HudResultsMetric metric)
{
	return metric == HudResultsMetric::LabUse ? "%" : "";
}

std::string hudResearchCategoryName(const std::string &icon)
{
	static const std::map<std::string, const char *> names = {
		{"research-weapon", N_("Weapons")}, {"research-defense", N_("Defenses")}, {"research-droid", N_("Vehicles")},
		{"research-sensor", N_("Systems")}, {"research-cyborg", N_("Cyborgs")}, {"research-structure", N_("Structures")},
		{"research-computer", N_("Computers")}, {"research-power", N_("Power")}};
	auto it = names.find(icon);
	return it != names.end() ? gettext(it->second) : icon;
}

PIELIGHT hudResearchCategoryColor(const std::string &icon)
{
	static const std::map<std::string, PIELIGHT> colors = {
		{"research-weapon", pal_RGBA(232, 135, 90, 255)}, {"research-defense", pal_RGBA(201, 166, 74, 255)},
		{"research-droid", pal_RGBA(90, 168, 232, 255)}, {"research-sensor", pal_RGBA(154, 122, 224, 255)},
		{"research-cyborg", pal_RGBA(88, 192, 168, 255)}, {"research-structure", pal_RGBA(138, 154, 176, 255)},
		{"research-computer", pal_RGBA(208, 122, 184, 255)}, {"research-power", pal_RGBA(232, 208, 74, 255)}};
	auto it = colors.find(icon);
	return it != colors.end() ? it->second : pal_RGBA(138, 154, 176, 255);
}

const std::vector<std::pair<std::string, std::string>> &hudKeyTechnologies()
{
	static const std::vector<std::pair<std::string, std::string>> list = {
		{"R-Struc-PowerModuleMk1", N_("Power Module")}, {"R-Struc-Research-Module", N_("Research Module")},
		{"R-Struc-Factory-Module", N_("Factory Module")}, {"R-Vehicle-Body05", N_("Cobra body")}, {"R-Vehicle-Body11", N_("Python body")},
		{"R-Vehicle-Prop-Halftracks", N_("Half-tracks")}, {"R-Vehicle-Prop-Tracks", N_("Tracks")}, {"R-Vehicle-Prop-Hover", N_("Hover")},
		{"R-Struc-RepairFacility", N_("Repair Facility")}, {"R-Wpn-Rocket01-LtAT", N_("Lancer")}, {"R-Wpn-Cannon2Mk1", N_("Medium Cannon")},
		{"R-Struc-Power-Upgrade01", N_("Gas Turbine Generator")}};
	return list;
}

HudWeaponSubclassTextures::~HudWeaponSubclassTextures()
{
	for (auto &texture : textures)
	{
		delete texture.second;
	}
}

gfx_api::texture *HudWeaponSubclassTextures::get(int weaponSubclass)
{
	if (weaponSubclass < 0 || weaponSubclass >= WSC_NUM_WEAPON_SUBCLASSES)
	{
		return nullptr;
	}
	auto it = textures.find(weaponSubclass);
	if (it == textures.end())
	{
		it = textures.emplace(weaponSubclass, loadImageForWeapSubclass(static_cast<WEAPON_SUBCLASS>(weaponSubclass))).first;
	}
	return it->second;
}

std::shared_ptr<HudWeaponSubclassTextures> hudWeaponSubclassTextures()
{
	static std::weak_ptr<HudWeaponSubclassTextures> shared;
	std::shared_ptr<HudWeaponSubclassTextures> textures = shared.lock();
	if (!textures)
	{
		textures = std::make_shared<HudWeaponSubclassTextures>();
		shared = textures;
	}
	return textures;
}

void hudDrawResearchLineIcon(const HudResearchLine &line, const WzRect &rect)
{
	if (gfx_api::texture *texture = hudWeaponSubclassTextures()->get(line.weaponSubclass))
	{
		iV_DrawImageAnisotropic(*texture, Vector2i(rect.x(), rect.y()), Vector2f(0.f, 0.f), Vector2f(static_cast<float>(rect.width()), static_cast<float>(rect.height())), 0.f, WZCOL_WHITE);
		return;
	}
	if (!line.icon.empty())
	{
		currentHudSkin().drawNamedIcon(line.icon, rect, line.color);
	}
}

const HudResearchLine *HudGameResults::lineFor(const std::string &key) const
{
	for (const HudResearchLine &l : lines)
	{
		if (l.key == key)
		{
			return &l;
		}
	}
	return nullptr;
}

std::string hudTimesText(int times)
{
	// TRANSLATORS: a multiple, as in "Team A has 2× the army value"
	return astringf(_("%d×"), times);
}

std::string hudTimesTextOf(double times)
{
	// TRANSLATORS: a multiple with one decimal, as in "3.7× the power lost"
	return times >= 10.0 ? hudTimesText(static_cast<int>(std::lround(times))) : astringf(_("%.1f×"), times);
}

float hudLeadShare(size_t sides)
{
	// (Among many, a smaller lead stands out)
	return sides > 4 ? 0.1f : 0.2f;
}

const std::vector<float> &HudGameResults::values(HudResultsMetric metric, size_t player) const
{
	return series[static_cast<size_t>(metric)][player];
}

std::vector<float> HudGameResults::sideValues(HudResultsMetric metric, size_t side) const
{
	// The sum (for lab use, the average. For research completed when shared, the most, as each ally's is the same)
	const bool most = metric == HudResultsMetric::Research && sharedResearch;
	std::vector<float> sum(times.size(), 0.f);
	for (size_t p : sides[side])
	{
		const std::vector<float> &v = values(metric, p);
		for (size_t i = 0; i < sum.size() && i < v.size(); ++i)
		{
			sum[i] = most ? std::max(sum[i], v[i]) : sum[i] + v[i];
		}
	}
	if (metric == HudResultsMetric::LabUse && !sides[side].empty())
	{
		for (float &v : sum)
		{
			v /= static_cast<float>(sides[side].size());
		}
	}
	return sum;
}

HudGameResults HudGameResults::collect(nonstd::optional<bool> youWon)
{
	HudGameResults r;
	GameStoryLogger &logger = GameStoryLogger::instance();
	const auto &frames = logger.getGameFrames();
	const auto &attributes = logger.getFixedPlayerAttributes();
	if (frames.empty())
	{
		return r;
	}

	// The frames up to now (the game may go on behind the results)
	const uint32_t now = gameTime;
	std::vector<size_t> used;
	for (size_t i = 0; i < frames.size(); ++i)
	{
		if (frames[i].currGameTime <= now + 1000)
		{
			used.push_back(i);
			r.times.push_back(frames[i].currGameTime / 1000.f);
		}
	}
	if (used.empty())
	{
		return r;
	}
	r.end = std::max(r.times.back(), 1.f);

	// The players (not spectators, nor empty slots), by team
	std::vector<uint32_t> indices;
	for (uint32_t i = 0; i < attributes.size() && i < MAX_PLAYERS; ++i)
	{
		if (attributes[i].slotWasOccupied && !NetPlay.players[i].isSpectator)
		{
			indices.push_back(i);
		}
	}
	std::set<int32_t> teams;
	for (uint32_t i : indices)
	{
		teams.insert(attributes[i].team);
	}
	// (A free-for-all keeps the lobby's team numbers, which it ignores)
	r.teamGame = teams.size() < indices.size() && alliancesSetTeamsBeforeGame(game.alliance);
	r.sharedResearch = r.teamGame && alliancesSharedResearch(game.alliance);
	std::stable_sort(indices.begin(), indices.end(), [&](uint32_t a, uint32_t b) {
		if (r.teamGame && attributes[a].team != attributes[b].team)
		{
			return attributes[a].team < attributes[b].team;
		}
		return attributes[a].position < attributes[b].position;
	});

	std::map<int32_t, size_t> teamSide;
	for (uint32_t i : indices)
	{
		HudResultsPlayer p;
		p.index = i;
		const char *name = getPlayerName(i);
		p.name = (name && *name) ? name : attributes[i].name;
		p.color = pal_GetTeamColour(getPlayerColour(i));
		p.team = attributes[i].team;
		p.you = i == selectedPlayer && !NetPlay.players[selectedPlayer].isSpectator;
		if (NetPlay.players[i].ai >= 0)
		{
			static const std::array<const char *, 5> difficulties = {N_("Super Easy"), N_("Easy"), N_("Medium"), N_("Hard"), N_("Insane")};
			const int d = static_cast<int>(NetPlay.players[i].difficulty);
			const char *ai = getAIName(static_cast<int>(i));
			p.about = std::string(ai ? ai : "AI") + ((d >= 0 && d < 5) ? std::string(" · ") + gettext(difficulties[static_cast<size_t>(d)]) : std::string());
		}
		else
		{
			const PLAYERSTATS &stats = getMultiStats(i);
			if (stats.played > 0)
			{
				p.about = astringf(_("%u played · %u won"), stats.played, stats.wins);
			}
		}
		if (i < NetPlay.scriptSetPlayerDataStrings.size())
		{
			auto it = NetPlay.scriptSetPlayerDataStrings[i].find("usertype");
			if (it != NetPlay.scriptSetPlayerDataStrings[i].end())
			{
				p.won = it->second == "winner";
				p.lost = it->second == "loser";
			}
		}
		if (r.teamGame)
		{
			auto it = teamSide.find(p.team);
			if (it == teamSide.end())
			{
				it = teamSide.emplace(p.team, r.sides.size()).first;
				r.sides.emplace_back();
				r.sideNames.push_back(astringf(_("Team %c"), 'A' + static_cast<char>(p.team)));
			}
			p.side = it->second;
		}
		else
		{
			p.side = r.sides.size();
			r.sides.emplace_back();
			r.sideNames.push_back(p.name);
		}
		r.sides[p.side].push_back(r.players.size());
		if (p.you)
		{
			r.youIndex = r.players.size();
		}
		r.players.push_back(p);
	}
	r.resultsKnown = !r.players.empty()
		&& std::all_of(r.players.begin(), r.players.end(), [](const HudResultsPlayer &p) { return p.won || p.lost; })
		&& std::any_of(r.players.begin(), r.players.end(), [](const HudResultsPlayer &p) { return p.won; })
		&& (!youWon.has_value() || !r.youIndex.has_value() || r.players[*r.youIndex].won == *youWon);
	r.sideWon.assign(r.sides.size(), false);
	for (HudResultsPlayer &p : r.players)
	{
		if (!r.resultsKnown)
		{
			p.won = false;
			p.lost = false;
		}
		if (p.won)
		{
			r.sideWon[p.side] = true;
		}
	}

	// Their figures over time, and at the end
	for (auto &metric : r.series)
	{
		metric.assign(r.players.size(), std::vector<float>());
	}
	for (size_t k = 0; k < r.players.size(); ++k)
	{
		HudResultsPlayer &p = r.players[k];
		for (size_t f : used)
		{
			const auto &all = frames[f].playerData;
			if (p.index >= all.size())
			{
				continue;
			}
			const auto &s = all[p.index];
			auto put = [&](HudResultsMetric m, float v) { r.series[static_cast<size_t>(m)][k].push_back(v); };
			put(HudResultsMetric::Army, static_cast<float>(s.droids));
			put(HudResultsMetric::ArmyValue, static_cast<float>(s.armyValue));
			put(HudResultsMetric::Derricks, static_cast<float>(s.oilRigs));
			put(HudResultsMetric::Kills, static_cast<float>(s.kills + s.structureKills));
			put(HudResultsMetric::PowerDestroyed, static_cast<float>(s.recentPowerWon));
			put(HudResultsMetric::PowerLost, static_cast<float>(s.recentPowerLost));
			put(HudResultsMetric::Research, static_cast<float>(s.researchComplete));
			put(HudResultsMetric::Structures, static_cast<float>(s.structs));
			put(HudResultsMetric::Score, static_cast<float>(s.score));
			p.derricksPeak = std::max(p.derricksPeak, s.oilRigs);
		}
		// Lab use over the last minute
		std::vector<float> &lab = r.series[static_cast<size_t>(HudResultsMetric::LabUse)][k];
		for (size_t j = 0; j < used.size(); ++j)
		{
			const size_t i0 = used[j >= 4 ? j - 4 : 0];
			const size_t i1 = used[j];
			if (p.index >= frames[i1].playerData.size())
			{
				lab.push_back(0.f);
				continue;
			}
			const auto &a = frames[i0].playerData[p.index];
			const auto &b = frames[i1].playerData[p.index];
			const double potential = static_cast<double>(b.recentResearchPotential) - static_cast<double>(a.recentResearchPotential);
			const double performance = static_cast<double>(b.recentResearchPerformance) - static_cast<double>(a.recentResearchPerformance);
			lab.push_back(potential > 0 ? static_cast<float>(100.0 * performance / potential) : 0.f);
		}
		const auto &all = frames[used.back()].playerData;
		if (p.index < all.size())
		{
			const auto &s = all[p.index];
			p.kills = s.kills;
			p.structureKills = s.structureKills;
			p.built = s.droidsBuilt;
			p.lost_ = s.droidsLost;
			p.structuresBuilt = s.structuresBuilt;
			p.structuresLost = s.structuresLost;
			p.powerDestroyed = s.recentPowerWon;
			p.powerLost = s.recentPowerLost;
			p.derricks = s.oilRigs;
			p.research = s.researchComplete;
			p.labUse = s.recentResearchPotential > 0 ? static_cast<float>(100.0 * s.recentResearchPerformance / s.recentResearchPotential) : 0.f;
			p.score = s.score;
			p.army = s.droids;
		}
	}

	// Research by their own labs (a teammate's shared copy has no lab)
	std::map<std::string, size_t> categoryCounts;
	for (const auto &e : logger.getResearchLog())
	{
		if (e.structureId == 0 || e.gameTime > now + 1000)
		{
			continue;
		}
		auto it = std::find_if(r.players.begin(), r.players.end(), [&e](const HudResultsPlayer &p) { return static_cast<int>(p.index) == e.player; });
		if (it == r.players.end())
		{
			continue;
		}
		HudResultsResearch item;
		item.time = e.gameTime / 1000.f;
		item.player = static_cast<size_t>(it - r.players.begin());
		item.side = it->side;
		item.id = e.researchId.toUtf8();
		const RESEARCH *research = getResearch(item.id.c_str());
		item.name = research ? getLocalizedStatsName(research) : item.id;
		item.icon = research ? researchIconName(research->iconID) : "research-structure";
		item.line = researchLineKey(research);
		if (research)
		{
			item.notable = !research->pStructureResults.empty();
			for (const COMPONENT_STATS *c : research->componentResults)
			{
				item.notable = item.notable || (c && (c->compType == COMP_WEAPON || c->compType == COMP_BODY || c->compType == COMP_PROPULSION));
			}
		}
		it->ownResearch++;
		categoryCounts[item.icon]++;
		r.research.push_back(item);
	}
	std::stable_sort(r.research.begin(), r.research.end(), [](const HudResultsResearch &a, const HudResultsResearch &b) { return a.time < b.time; });
	for (const auto &c : categoryCounts)
	{
		r.categories.push_back(c.first);
	}

	// The lines: weapon lines, most researched first, each a color of its own, then the groups
	std::map<std::string, size_t> lineCounts;
	for (const HudResultsResearch &e : r.research)
	{
		lineCounts[e.line]++;
	}
	static const PIELIGHT palette[] = {pal_RGBA(232, 135, 90, 255), pal_RGBA(90, 168, 232, 255), pal_RGBA(232, 208, 74, 255), pal_RGBA(88, 192, 168, 255),
	                                   pal_RGBA(208, 122, 184, 255), pal_RGBA(154, 122, 224, 255), pal_RGBA(140, 200, 90, 255), pal_RGBA(232, 96, 96, 255),
	                                   pal_RGBA(120, 210, 230, 255), pal_RGBA(220, 160, 110, 255)};
	std::vector<HudResearchLine> weaponLines;
	for (const auto &c : lineCounts)
	{
		if (const auto subclass = weaponSubclassNamed(c.first))
		{
			HudResearchLine line;
			line.key = c.first;
			line.name = getWeaponSubClassDisplayName(*subclass, true);
			line.weaponSubclass = static_cast<int>(*subclass);
			line.count = c.second;
			weaponLines.push_back(line);
		}
	}
	std::stable_sort(weaponLines.begin(), weaponLines.end(), [](const HudResearchLine &a, const HudResearchLine &b) { return a.count > b.count; });
	for (size_t k = 0; k < weaponLines.size(); ++k)
	{
		weaponLines[k].color = palette[k % (sizeof(palette) / sizeof(palette[0]))];
		r.lines.push_back(weaponLines[k]);
	}
	const struct
	{
		const char *key;
		const char *name;
		const char *icon;
		PIELIGHT color;
	} groups[] = {{"armor", N_("Armor and bodies"), "research-droid", pal_RGBA(150, 162, 180, 255)},
	              {"systems", N_("Systems"), "research-sensor", pal_RGBA(120, 132, 150, 255)},
	              {"base", N_("Base and economy"), "research-structure", pal_RGBA(96, 108, 126, 255)}};
	for (const auto &g : groups)
	{
		auto it = lineCounts.find(g.key);
		if (it != lineCounts.end())
		{
			HudResearchLine line;
			line.key = g.key;
			line.name = gettext(g.name);
			line.icon = g.icon;
			line.color = g.color;
			line.count = it->second;
			r.lines.push_back(line);
		}
	}
	std::stable_sort(r.categories.begin(), r.categories.end(), [&](const std::string &a, const std::string &b) { return categoryCounts[a] > categoryCounts[b]; });

	// Firsts: main technologies a player's side had before any other side
	for (const auto &tech : hudKeyTechnologies())
	{
		std::map<size_t, const HudResultsResearch *> firstBySide;
		for (const HudResultsResearch &e : r.research)
		{
			if (e.id == tech.first && !firstBySide.count(e.side))
			{
				firstBySide[e.side] = &e;
			}
		}
		const HudResultsResearch *first = nullptr;
		for (const auto &f : firstBySide)
		{
			if (!first || f.second->time < first->time)
			{
				first = f.second;
			}
		}
		if (first)
		{
			r.players[first->player].firsts++;
		}
	}

	// The races: notable topics (and the main technologies) two sides or more researched, the biggest leads first, those
	// of 30 s or more
	std::map<std::string, std::map<size_t, const HudResultsResearch *>> bySide;
	for (const HudResultsResearch &e : r.research)
	{
		const bool key = std::any_of(hudKeyTechnologies().begin(), hudKeyTechnologies().end(), [&e](const std::pair<std::string, std::string> &k) { return k.first == e.id; });
		if (e.notable || key)
		{
			bySide[e.id].emplace(e.side, &e);
		}
	}
	for (const auto &topic : bySide)
	{
		if (topic.second.size() < 2)
		{
			continue;
		}
		HudResultsRace race;
		race.id = topic.first;
		const HudResultsResearch *any = topic.second.begin()->second;
		race.name = any->name;
		for (const auto &k : hudKeyTechnologies())
		{
			if (k.first == race.id)
			{
				race.name = gettext(k.second.c_str());
			}
		}
		race.icon = any->icon;
		for (const auto &s : topic.second)
		{
			race.sides.emplace_back(s.first, s.second->time);
		}
		std::sort(race.sides.begin(), race.sides.end(), [](const std::pair<size_t, float> &a, const std::pair<size_t, float> &b) { return a.second < b.second; });
		race.lead = race.sides[1].second - race.sides[0].second;
		if (race.lead >= 30.f)
		{
			r.races.push_back(race);
		}
	}
	std::stable_sort(r.races.begin(), r.races.end(), [](const HudResultsRace &a, const HudResultsRace &b) { return a.lead > b.lead; });

	r.computeMoments();
	r.computeStandouts();
	return r;
}

void HudGameResults::computeMoments()
{
	// The names in the moments' lines: at most so wide, so the line still says what happened
	constexpr int MOMENT_NAME_W = 220;
	std::vector<std::string> momentName;
	for (const std::string &name : sideNames)
	{
		momentName.push_back(hudCappedText(name, font_regular, MOMENT_NAME_W));
	}
	auto playerName = [&](size_t player) { return hudCappedText(players[player].name, font_regular, MOMENT_NAME_W); };
	const size_t n = times.size();
	moments.clear();
	phases.clear();
	if (n < 2 || sides.empty())
	{
		return;
	}
	const float step = std::max(1.f, (times.back() - times.front()) / static_cast<float>(n - 1));
	const size_t w = std::max<size_t>(1, static_cast<size_t>(std::lround(WINDOW / step)));
	std::vector<std::vector<float>> army, value, oil, structs, unitsLost;
	for (size_t s = 0; s < sides.size(); ++s)
	{
		army.push_back(sideValues(HudResultsMetric::Army, s));
		value.push_back(sideValues(HudResultsMetric::ArmyValue, s));
		oil.push_back(sideValues(HudResultsMetric::Derricks, s));
		structs.push_back(sideValues(HudResultsMetric::Structures, s));
	}
	// Units lost over time, from the frames
	const auto &frames = GameStoryLogger::instance().getGameFrames();
	for (size_t s = 0; s < sides.size(); ++s)
	{
		std::vector<float> v;
		size_t f = 0;
		for (size_t i = 0; i < n; ++i)
		{
			while (f < frames.size() && frames[f].currGameTime / 1000.f < times[i] - 0.01f)
			{
				++f;
			}
			float sum = 0.f;
			if (f < frames.size())
			{
				for (size_t p : sides[s])
				{
					const uint32_t idx = players[p].index;
					if (idx < frames[f].playerData.size())
					{
						sum += static_cast<float>(frames[f].playerData[idx].droidsLost);
					}
				}
			}
			v.push_back(sum);
		}
		unitsLost.push_back(v);
	}

	std::vector<HudResultsMoment> cand;
	auto sidePlayers = [this](size_t s) { return sides[s]; };

	// A side out: no units and no structures left
	for (size_t s = 0; s < sides.size(); ++s)
	{
		for (size_t i = 0; i < n; ++i)
		{
			if (army[s][i] == 0.f && structs[s][i] == 0.f)
			{
				HudResultsMoment m;
				m.time = times[i];
				m.kind = "out";
				m.weight = 100;
				m.text = astringf(_("%s has no units or structures left"), momentName[s].c_str());
				m.players = sidePlayers(s);
				m.side = s;
				cand.push_back(m);
				break;
			}
		}
	}

	// Battles: two sides or more losing heavily in the same two minutes, nobody clearly ahead: the two that lost most each
	// lost 5 or more, the second at least 60% of the first, and 14 or more between them. The biggest, none overlapping.
	auto windowLosses = [&](size_t i) {
		std::vector<std::pair<int, size_t>> l;
		for (size_t o = 0; o < sides.size(); ++o)
		{
			l.emplace_back(static_cast<int>(unitsLost[o][i + w] - unitsLost[o][i]), o);
		}
		std::sort(l.begin(), l.end(), std::greater<std::pair<int, size_t>>());
		return l;
	};
	std::vector<size_t> battleStarts;
	{
		std::vector<std::pair<int, size_t>> windows;   // (total, start)
		for (size_t i = 0; i + w < n && sides.size() >= 2; ++i)
		{
			const auto l = windowLosses(i);
			if (l[1].first >= 5 && l[1].first * 5 >= l[0].first * 3 && l[0].first + l[1].first >= 14)
			{
				windows.emplace_back(l[0].first + l[1].first, i);
			}
		}
		std::stable_sort(windows.begin(), windows.end(), [](const std::pair<int, size_t> &a, const std::pair<int, size_t> &b) { return a.first > b.first; });
		for (const auto &win : windows)
		{
			if (std::any_of(battleStarts.begin(), battleStarts.end(), [&](size_t b) { return (b > win.second ? b - win.second : win.second - b) < w; }))
			{
				continue;
			}
			battleStarts.push_back(win.second);
			const size_t i = win.second;
			const auto l = windowLosses(i);
			HudResultsMoment m;
			m.time = times[i];
			m.kind = "battle";
			m.weight = win.first * 2;
			m.text = astringf(_("Big battle: %s loses %d, %s %d"), momentName[l[0].second].c_str(), l[0].first, momentName[l[1].second].c_str(), l[1].first);
			m.detail = astringf(_("%s to %s"), clock(times[i]).c_str(), clock(times[i + w]).c_str());
			for (const auto &x : l)
			{
				if (x.first >= 5)
				{
					m.players.insert(m.players.end(), sides[x.second].begin(), sides[x.second].end());
				}
			}
			cand.push_back(m);
		}
	}
	auto inBattle = [&](size_t i) {
		return std::any_of(battleStarts.begin(), battleStarts.end(), [&](size_t b) { return (b > i ? b - i : i - b) <= w / 2; });
	};

	// Fights: a side losing at least max(8, a quarter of its peak army) units in two minutes, more than the others (not
	// in a battle). One moment per fight
	for (size_t s = 0; s < sides.size(); ++s)
	{
		const float peak = *std::max_element(army[s].begin(), army[s].end());
		const float need = std::max(8.f, std::floor(peak / 4.f));
		size_t i = 0;
		while (i + w < n)
		{
			if (unitsLost[s][i + w] - unitsLost[s][i] >= need)
			{
				size_t best = i;
				for (size_t j = i; j < std::min(i + w, n - w); ++j)
				{
					if (unitsLost[s][j + w] - unitsLost[s][j] > unitsLost[s][best + w] - unitsLost[s][best])
					{
						best = j;
					}
				}
				const int count = static_cast<int>(unitsLost[s][best + w] - unitsLost[s][best]);
				if (inBattle(best))
				{
					i = best + w;
					continue;   // (the battle's moment says it)
				}
				nonstd::optional<size_t> other;
				int otherCount = 0;
				for (size_t o = 0; o < sides.size(); ++o)
				{
					const int c = static_cast<int>(unitsLost[o][best + w] - unitsLost[o][best]);
					if (o != s && c > otherCount)
					{
						other = o;
						otherCount = c;
					}
				}
				HudResultsMoment m;
				m.time = times[best];
				m.kind = "fight";
				m.weight = count * 3;
				m.text = astringf(_("%s loses %d units"), momentName[s].c_str(), count);
				if (other)
				{
					// TRANSLATORS: A fight's start and end, then how many units the other side lost in it
					m.detail = astringf(_("%s to %s, %s loses %d"), clock(times[best]).c_str(), clock(times[best + w]).c_str(), momentName[*other].c_str(),
					                    otherCount);
				}
				else
				{
					m.detail = astringf(_("%s to %s"), clock(times[best]).c_str(), clock(times[best + w]).c_str());
				}
				m.players = sidePlayers(s);
				m.side = s;
				cand.push_back(m);
				i = best + w;
			}
			else
			{
				++i;
			}
		}
	}

	// Derricks: a side losing at least 3, and 40% of them, within two minutes of having them
	for (size_t s = 0; s < sides.size(); ++s)
	{
		size_t j = 0;
		while (j + 1 < n)
		{
			const float h = oil[s][j];
			size_t k = j + 1;
			for (size_t q = j + 1; q < std::min(n, j + w + 1); ++q)
			{
				if (oil[s][q] < oil[s][k])
				{
					k = q;
				}
			}
			const float drop = h - oil[s][k];
			if (drop >= 3.f && drop >= 0.4f * h)
			{
				HudResultsMoment m;
				m.time = times[j];
				m.kind = "derricks";
				m.weight = 30 + 5 * static_cast<int>(drop);
				m.text = astringf(_("%s loses %d of %d derricks"), momentName[s].c_str(), static_cast<int>(drop), static_cast<int>(h));
				m.detail = astringf(_("%s to %s"), clock(times[j]).c_str(), clock(times[k]).c_str());
				m.players = sidePlayers(s);
				m.side = s;
				cand.push_back(m);
				j = k;
			}
			++j;
		}
	}

	// The lead (army value, the power cost of the units): who leads by the lead share and at least LEAD_VALUE.
	// "Far" at twice the next and FAR_VALUE more
	std::vector<nonstd::optional<size_t>> leadAt(n), farAt(n);
	for (size_t i = 0; i < n; ++i)
	{
		size_t first = 0;
		for (size_t s = 1; s < sides.size(); ++s)
		{
			if (value[s][i] > value[first][i])
			{
				first = s;
			}
		}
		float second = 0.f;
		for (size_t s = 0; s < sides.size(); ++s)
		{
			if (s != first)
			{
				second = std::max(second, value[s][i]);
			}
		}
		const float v1 = value[first][i];
		if (sides.size() > 1 && v1 - second >= HUD_LEAD_VALUE && v1 - second >= hudLeadShare(sides.size()) * v1)
		{
			leadAt[i] = first;
		}
		if (sides.size() > 1 && v1 - second >= FAR_VALUE && v1 >= 2.f * second)
		{
			farAt[i] = first;
		}
	}
	std::vector<std::pair<size_t, size_t>> held;
	nonstd::optional<size_t> current;
	for (size_t i = 0; i < n; ++i)
	{
		if (!leadAt[i] || leadAt[i] == current)
		{
			continue;
		}
		const size_t s = *leadAt[i];
		bool keeps = true;
		size_t count = 0;
		for (size_t j = i; j < std::min(n, i + w + 1); ++j)
		{
			keeps = keeps && (!leadAt[j] || *leadAt[j] == s);
			count += (leadAt[j] && *leadAt[j] == s) ? 1 : 0;
		}
		if (keeps && count >= static_cast<size_t>(std::ceil(w * 0.75f)))
		{
			held.emplace_back(i, s);
			current = s;
		}
	}
	for (size_t k = 0; k < held.size(); ++k)
	{
		if (k == 0 && sides.size() == 2)
		{
			continue;  // two sides: the first lead only counts once it's big (below)
		}
		HudResultsMoment m;
		m.time = times[held[k].first];
		m.kind = "lead";
		m.weight = 45;
		m.text = astringf(_("%s takes the lead"), momentName[held[k].second].c_str());
		m.detail = _("army value");
		m.players = sidePlayers(held[k].second);
		m.side = held[k].second;
		cand.push_back(m);
	}
	if (sides.size() == 2)
	{
		for (size_t s = 0; s < 2; ++s)
		{
			const size_t o = 1 - s;
			for (size_t i = 0; i < n; ++i)
			{
				auto twice = [&](size_t j) { return value[s][j] - value[o][j] >= FAR_VALUE && value[s][j] >= 2.f * value[o][j]; };
				if (twice(i))
				{
					bool keeps = true;
					for (size_t j = i; j < n; ++j)
					{
						keeps = keeps && twice(j);
					}
					HudResultsMoment m;
					m.time = times[i];
					m.kind = "lead";
					m.weight = keeps ? 60 : 40;
					// TRANSLATORS: the first %s is a side (a player or a team), the second a multiple, as "2×"
					m.text = astringf(_("%s has %s the army value"), momentName[s].c_str(), hudTimesText(2).c_str());
					m.detail = keeps ? _("and never drops below it") : _("army value");
					m.players = sidePlayers(s);
					m.side = s;
					cand.push_back(m);
					break;
				}
			}
		}
	}

	// Research: a main technology one side has 90 s or more before any other (or alone), and the first to VTOLs
	// and to anti-air
	auto sideTimes = [this](const std::function<bool (const std::string &)> &match) {
		std::map<size_t, const HudResultsResearch *> bySide;
		for (const HudResultsResearch &e : research)
		{
			if (match(e.id) && !bySide.count(e.side))
			{
				bySide[e.side] = &e;
			}
		}
		return bySide;
	};
	auto nextTime = [](const std::map<size_t, const HudResultsResearch *> &bySide, size_t side) -> nonstd::optional<float> {
		nonstd::optional<float> t;
		for (const auto &b : bySide)
		{
			if (b.first != side && (!t || b.second->time < *t))
			{
				t = b.second->time;
			}
		}
		return t;
	};
	for (const auto &tech : hudKeyTechnologies())
	{
		const auto bySide = sideTimes([&tech](const std::string &id) { return id == tech.first; });
		if (bySide.empty())
		{
			continue;
		}
		const HudResultsResearch *first = std::min_element(bySide.begin(), bySide.end(), [](const auto &a, const auto &b) { return a.second->time < b.second->time; })->second;
		const nonstd::optional<float> next = nextTime(bySide, first->side);
		if (next && *next - first->time < 90.f)
		{
			continue;
		}
		const float gap = next ? *next - first->time : end - first->time;
		HudResultsMoment m;
		m.time = first->time;
		m.kind = "research";
		m.weight = std::min(40, 10 + static_cast<int>(gap / 20.f));
		m.text = astringf(_("%s researches %s"), playerName(first->player).c_str(), gettext(tech.second.c_str()));
		if (next)
		{
			m.detail = astringf(_("next: %s"), clock(*next).c_str());
		}
		else if (sides.size() == 2)
		{
			m.detail = astringf(_("%s never does"), momentName[1 - first->side].c_str());
		}
		else
		{
			m.detail = _("nobody else gets it");
		}
		m.players = {first->player};
		m.side = first->side;
		cand.push_back(m);
	}
	for (const auto &milestone : {std::make_pair(std::string("vtol"), &isVtolResearch), std::make_pair(std::string("aa"), &isAntiAirResearch)})
	{
		const auto bySide = sideTimes([&milestone](const std::string &id) { return milestone.second(id); });
		if (bySide.empty())
		{
			continue;
		}
		const HudResultsResearch *first = std::min_element(bySide.begin(), bySide.end(), [](const auto &a, const auto &b) { return a.second->time < b.second->time; })->second;
		const nonstd::optional<float> next = nextTime(bySide, first->side);
		HudResultsMoment m;
		m.time = first->time;
		m.kind = milestone.first;
		m.weight = 55;
		m.text = milestone.first == "vtol" ? astringf(_("%s is first to VTOLs"), playerName(first->player).c_str())
		                                   : astringf(_("%s is first to anti-air"), playerName(first->player).c_str());
		m.detail = next ? astringf(_("next: %s"), clock(*next).c_str()) : std::string(_("nobody else"));
		m.players = {first->player};
		m.side = first->side;
		cand.push_back(m);
	}

	// The game's end
	{
		HudResultsMoment m;
		m.time = end;
		m.kind = "end";
		m.weight = 1000;
		std::vector<size_t> winners;
		for (size_t s = 0; s < sides.size(); ++s)
		{
			if (sideWon[s])
			{
				winners.push_back(s);
			}
		}
		if (winners.size() == 1)
		{
			m.text = astringf(_("The game ends: %s wins"), momentName[winners.front()].c_str());
			m.detail = astringf(_("with %d units left"), static_cast<int>(army[winners.front()].back()));
			m.players = sides[winners.front()];
			m.side = winners.front();
		}
		else if (!winners.empty())
		{
			std::string names;
			for (size_t s : winners)
			{
				names += (names.empty() ? "" : ", ") + momentName[s];
				m.players.insert(m.players.end(), sides[s].begin(), sides[s].end());
			}
			m.text = _("The game ends");
			m.detail = astringf(_("won: %s"), names.c_str());
		}
		else
		{
			m.text = _("The game ends");
		}
		// The end replaces any moment in its last step
		cand.erase(std::remove_if(cand.begin(), cand.end(), [&](const HudResultsMoment &c) { return c.time > end - step; }), cand.end());
		cand.push_back(m);
	}

	momentsFound = cand.size() - 1;
	std::stable_sort(cand.begin(), cand.end(), [](const HudResultsMoment &a, const HudResultsMoment &b) { return a.weight > b.weight; });
	for (const HudResultsMoment &c : cand)
	{
		const size_t ofKind = static_cast<size_t>(std::count_if(moments.begin(), moments.end(), [&c](const HudResultsMoment &m) { return m.kind == c.kind; }));
		auto cap = CAPS.find(c.kind);
		if (cap != CAPS.end() && ofKind >= cap->second)
		{
			continue;
		}
		if (std::any_of(moments.begin(), moments.end(), [&c](const HudResultsMoment &m) { return m.kind == c.kind && std::fabs(m.time - c.time) < 45.f; }))
		{
			continue;
		}
		if (moments.size() >= MAX_MOMENTS)
		{
			break;
		}
		moments.push_back(c);
	}
	std::stable_sort(moments.begin(), moments.end(), [](const HudResultsMoment &a, const HudResultsMoment &b) { return a.time < b.time; });

	// Phases: who leads (the army's value), in stretches of at least 90 s
	struct Stretch
	{
		float from, to;
		nonstd::optional<size_t> side;
		bool farAhead;
	};
	std::vector<Stretch> seg;
	for (size_t i = 0; i < n; ++i)
	{
		const nonstd::optional<size_t> s = farAt[i] ? farAt[i] : leadAt[i];
		const bool farAhead = farAt[i].has_value();
		if (!seg.empty() && seg.back().side == s && seg.back().farAhead == farAhead)
		{
			seg.back().to = times[i] + step;
		}
		else
		{
			seg.push_back({times[i], times[i] + step, s, farAhead});
		}
	}
	bool changed = true;
	while (changed && seg.size() > 1)
	{
		changed = false;
		size_t k = 0;
		for (size_t q = 1; q < seg.size(); ++q)
		{
			if (seg[q].to - seg[q].from < seg[k].to - seg[k].from)
			{
				k = q;
			}
		}
		if (seg[k].to - seg[k].from < 90.f)
		{
			if (k == 0)
			{
				seg[1].from = seg[0].from;
			}
			else
			{
				seg[k - 1].to = seg[k].to;
			}
			seg.erase(seg.begin() + static_cast<std::ptrdiff_t>(k));
			std::vector<Stretch> merged;
			for (const Stretch &x : seg)
			{
				if (!merged.empty() && merged.back().side == x.side && merged.back().farAhead == x.farAhead)
				{
					merged.back().to = x.to;
				}
				else
				{
					merged.push_back(x);
				}
			}
			seg = merged;
			changed = true;
		}
	}
	for (const Stretch &x : seg)
	{
		phases.push_back({x.from, std::min(x.to, end), x.side, x.farAhead});
	}
}

void HudGameResults::computeStandouts()
{
	standouts.clear();
	if (players.size() < 2)
	{
		return;
	}
	struct Candidate
	{
		const char *title;
		std::function<double (size_t)> score;     ///< (higher is better, and below `least`, nobody stands out)
		std::function<std::string (size_t)> value;
		double least;
	};
	const auto &armyValues = series[static_cast<size_t>(HudResultsMetric::ArmyValue)];
	auto peakAt = [&](size_t p) {
		const std::vector<float> &v = armyValues[p];
		return static_cast<size_t>(std::max_element(v.begin(), v.end()) - v.begin());
	};
	auto trade = [&](size_t p) {
		const HudResultsPlayer &pl = players[p];
		if (pl.powerDestroyed < 1000)
		{
			return 0.0;
		}
		return pl.powerLost == 0 ? 1e9 : static_cast<double>(pl.powerDestroyed) / static_cast<double>(pl.powerLost);
	};
	// Four areas, each its candidates by priority
	const std::vector<std::vector<Candidate>> areas = {
		{{N_("Most units killed"), [&](size_t p) { return static_cast<double>(players[p].kills); }, [&](size_t p) { return std::to_string(players[p].kills); }, 1.0},
		 {N_("Most structures killed"), [&](size_t p) { return static_cast<double>(players[p].structureKills); }, [&](size_t p) { return std::to_string(players[p].structureKills); }, 1.0}},
		{{N_("Most cost-effective"), trade, [&](size_t p) {
			  return players[p].powerLost == 0 ? std::string(_("lost nothing")) : astringf(_("destroyed %s the power lost"), hudTimesTextOf(trade(p)).c_str());
		  }, 1.0},
		 {N_("Most power destroyed"), [&](size_t p) { return static_cast<double>(players[p].powerDestroyed); }, [&](size_t p) { return hudChartNumber(static_cast<float>(players[p].powerDestroyed)); }, 1.0}},
		{{N_("Most tech researched"), [&](size_t p) { return static_cast<double>(players[p].ownResearch); }, [&](size_t p) { return astringf(ngettext("%u topic", "%u topics", players[p].ownResearch), players[p].ownResearch); }, 1.0},
		 {N_("Most derricks"), [&](size_t p) { return static_cast<double>(players[p].derricksPeak); }, [&](size_t p) { return astringf(_("%u at the peak"), players[p].derricksPeak); }, 1.0}},
		{{N_("Biggest army"), [&](size_t p) { return armyValues[p].empty() ? 0.0 : static_cast<double>(armyValues[p][peakAt(p)]); }, [&](size_t p) {
			  const size_t i = peakAt(p);
			  return astringf(_("%s army value, at %s"), hudChartNumber(armyValues[p][i]).c_str(), clock(times[std::min(i, times.size() - 1)]).c_str());
		  }, 1.0},
		 {N_("Most units built"), [&](size_t p) { return static_cast<double>(players[p].built); }, [&](size_t p) { return std::to_string(players[p].built); }, 1.0}},
	};
	// Each area's candidates nobody tops, with their best (ties included)
	using Pick = std::pair<const Candidate *, std::vector<size_t>>;
	std::vector<std::vector<Pick>> valid;
	for (const std::vector<Candidate> &area : areas)
	{
		valid.emplace_back();
		for (const Candidate &c : area)
		{
			double top = 0.0;
			for (size_t p = 0; p < players.size(); ++p)
			{
				top = std::max(top, c.score(p));
			}
			if (top < c.least)
			{
				continue;
			}
			std::vector<size_t> best;
			for (size_t p = 0; p < players.size(); ++p)
			{
				if (c.score(p) >= top)
				{
					best.push_back(p);
				}
			}
			valid.back().emplace_back(&c, best);
		}
	}
	// Each area starts with its first candidate. Then, in order, an area whose best players already have another area's
	// card moves on to its next candidate whose best have none, spreading the cards across the players
	std::vector<size_t> chosen(valid.size(), 0);
	for (size_t a = 0; a < valid.size(); ++a)
	{
		auto hasCard = [&](const std::vector<size_t> &ps) {
			for (size_t o = 0; o < valid.size(); ++o)
			{
				if (o == a || valid[o].empty())
				{
					continue;
				}
				const std::vector<size_t> &theirs = valid[o][chosen[o]].second;
				if (std::any_of(ps.begin(), ps.end(), [&](size_t p) { return std::find(theirs.begin(), theirs.end(), p) != theirs.end(); }))
				{
					return true;
				}
			}
			return false;
		};
		if (valid[a].empty() || !hasCard(valid[a][0].second))
		{
			continue;
		}
		for (size_t k = 1; k < valid[a].size(); ++k)
		{
			if (!hasCard(valid[a][k].second))
			{
				chosen[a] = k;
				break;
			}
		}
	}
	for (size_t a = 0; a < valid.size(); ++a)
	{
		if (valid[a].empty())
		{
			continue;
		}
		const Pick &pick = valid[a][chosen[a]];
		HudResultsStandout standout;
		standout.title = gettext(pick.first->title);
		standout.players = pick.second;
		standout.value = pick.first->value(pick.second.front());
		standouts.push_back(standout);
	}
}
