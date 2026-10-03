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

#include <nlohmann/json.hpp> // Must come before WZ includes

#include "hudbuildcategories.h"

#include "lib/framework/frame.h"
#include "lib/framework/physfs_ext.h"

#include "../structure.h"
#include "../wzjsonhelpers.h"

#include <algorithm>
#include <unordered_map>

namespace
{

const char *const CATEGORIES_FILE = "stats/buildcategories.json";

std::vector<HudBuildCategory> categories;
bool loaded = false;
std::unordered_map<std::string, std::string> ownCategories;

void addDefault(const char *id, const char *name, int order)
{
	HudBuildCategory category;
	category.id = id;
	category.name = WzString::fromUtf8(gettext(name));
	category.order = order;
	categories.push_back(category);
}

void load()
{
	if (loaded)
	{
		return;
	}
	loaded = true;
	categories.clear();
	if (PHYSFS_exists(CATEGORIES_FILE))
	{
		auto json = wzLoadJsonObjectFromFile(CATEGORIES_FILE);
		if (json.has_value() && json->is_object())
		{
			for (auto it = json->begin(); it != json->end(); ++it)
			{
				if (!it.value().is_object())
				{
					continue;
				}
				HudBuildCategory category;
				category.id = it.key();
				const auto &entry = it.value();
				const std::string name = entry.contains("name") && entry["name"].is_string() ? entry["name"].get<std::string>() : it.key();
				category.name = WzString::fromUtf8(gettext(name.c_str()));
				category.icon = entry.contains("icon") && entry["icon"].is_string() ? entry["icon"].get<std::string>() : std::string();
				category.order = entry.contains("order") && entry["order"].is_number_integer() ? entry["order"].get<int>() : 100;
				categories.push_back(category);
			}
		}
		else
		{
			debug(LOG_WARNING, "%s: not a JSON object; using the default build categories", CATEGORIES_FILE);
		}
	}
	if (categories.empty())
	{
		addDefault("base", N_("Base"), 10);
		addDefault("defense", N_("Defense"), 20);
		addDefault("support", N_("Support"), 30);
	}
	categories.erase(std::remove_if(categories.begin(), categories.end(), [](const HudBuildCategory &c) { return c.id == "other"; }), categories.end());
	std::stable_sort(categories.begin(), categories.end(), [](const HudBuildCategory &a, const HudBuildCategory &b) { return a.order < b.order; });
	HudBuildCategory other;
	other.id = "other";
	other.name = WzString::fromUtf8(_("Other"));
	other.order = 1000;
	categories.push_back(other);
}

}

const std::vector<HudBuildCategory> &hudBuildCategories()
{
	load();
	return categories;
}

const HudBuildCategory *hudFindBuildCategory(const std::string &id)
{
	for (const auto &category : hudBuildCategories())
	{
		if (category.id == id)
		{
			return &category;
		}
	}
	return nullptr;
}

std::string hudDerivedBuildCategory(const STRUCTURE_STATS *stats)
{
	if (stats == nullptr)
	{
		return "other";
	}
	switch (stats->type)
	{
	case REF_HQ:
	case REF_FACTORY:
	case REF_FACTORY_MODULE:
	case REF_CYBORG_FACTORY:
	case REF_VTOL_FACTORY:
	case REF_POWER_GEN:
	case REF_POWER_MODULE:
	case REF_RESOURCE_EXTRACTOR:
	case REF_RESEARCH:
	case REF_RESEARCH_MODULE:
	case REF_COMMAND_CONTROL:
		return "base";
	case REF_DEFENSE:
	case REF_WALL:
	case REF_WALLCORNER:
	case REF_GATE:
	case REF_FORTRESS:
		return "defense";
	case REF_REPAIR_FACILITY:
	case REF_REARM_PAD:
	case REF_SAT_UPLINK:
	case REF_LASSAT:
	case REF_MISSILE_SILO:
		return "support";
	case REF_GENERIC:
		return stats->numWeaps > 0 ? "defense" : "support";
	default:
		return "other";
	}
}

std::string hudBuildCategoryOf(const STRUCTURE_STATS *stats)
{
	if (stats == nullptr)
	{
		return "other";
	}
	auto it = ownCategories.find(stats->id.toUtf8());
	if (it != ownCategories.end() && hudFindBuildCategory(it->second) != nullptr)
	{
		return it->second;
	}
	const std::string derived = hudDerivedBuildCategory(stats);
	return hudFindBuildCategory(derived) != nullptr ? derived : std::string("other");
}

void hudSetStructureBuildCategory(const std::string &structureId, const std::string &category)
{
	ownCategories[structureId] = category;
}

void hudResetBuildCategories()
{
	ownCategories.clear();
	categories.clear();
	loaded = false;
}

void hudLogBuildCategories()
{
	for (unsigned i = 0; i < numStructureStats; ++i)
	{
		const STRUCTURE_STATS *stats = &asStructureStats[i];
		const std::string id = stats->id.toUtf8();
		const bool own = ownCategories.count(id) != 0;
		debug(LOG_INFO, "build category of %s: %s (%s)", id.c_str(), hudBuildCategoryOf(stats).c_str(), own ? "structure.json" : "derived from its type");
	}
}
