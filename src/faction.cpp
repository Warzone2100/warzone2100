/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2021  Warzone 2100 Project

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
/** \file
 *  Functions for factions.
 */

#include "faction.h"
#include "lib/framework/frame.h"
#include "lib/netplay/netplay.h"

static const struct FACTION factions[NUM_FACTIONS] = {
	{ "Normal",
		{
		}
	},
	{
		"NEXUS",
		{
			{"blwallc1.pie", "blwallc_nex.pie"},
			{"blwallh.pie", "blwallh_nex.pie"},
			{"blhq.pie", "blhq_nex.pie"},
			{"blguard1.pie", "blguard_nex.pie"},
			{"blguardr.pie", "blguardr_nex.pie"},
			{"blresch0.pie", "blresch0_nex.pie"},
			{"blresch4.pie", "blresch4_nex.pie"},
			{"research_module4.pie", "research_module4_nex.pie"},
			{"blbresch.pie", "blbresch_nex.pie"},
			{"blbpower.pie", "blbpower_nex.pie"},
			{"blbhq.pie",  "blbhq_nex.pie"},
			{"blbdrdcm.pie", "blbdrdcm_nex.pie"},
			{"blpower0.pie", "blpower0_nex.pie"},
			{"blpower4.pie", "blpower4_nex.pie"},
			{"blpilbox.pie", "blpilbox_nex.pie"},
			{"blfact0.pie", "blfact0_nex.pie"},
			{"factory_module1.pie", "factory_module1_nex.pie"},
			{"blfact1.pie", "blfact1_nex.pie"},
			{"factory_module2.pie", "factory_module2_nex.pie"},
			{"blfact2.pie", "blfact2_nex.pie"},
			{"blbfact.pie", "blbfact_nex.pie"},
			{"bldrdcm0.pie", "bldrdcm0_nex.pie"},
			{"blcfact1.pie", "blcfact1_nex.pie"},
			{"blbcfact.pie", "blbcfact_nex.pie"},
			{"blcanpil.pie", "blcanpil_nex.pie"},
			{"blhowmnt.pie", "blhowmnt_nex.pie"},
			{"blguardm.pie", "blguardm_nex.pie"},
			{"blvfact0.pie", "blvfact0_nex.pie"},
			{"blvfact1.pie", "blvfact1_nex.pie"},
			{"blvfact2.pie", "blvfact2_nex.pie"},
			{"vtolfactory_module1.pie", "vtolfactory_module1_nex.pie"},
			{"vtolfactory_module2.pie", "vtolfactory_module2_nex.pie"},
			{"blderik_anim.pie", "blderik_anim_nex.pie"},
			{"blderik.pie", "blderik_nex.pie"},
			{"blmrtpit.pie", "blmrtpit_nex.pie"}
		}
	},
	{
		"Collective",
		{
			{"blwallc1.pie", "blwallc_col.pie"},
			{"blwallh.pie", "blwallh_col.pie"},
			{"blwallh_t.pie", "blwallh_t_col.pie"},
			{"blwallh_l.pie", "blwallh_l_col.pie"},
			{"blhq.pie", "blhq_col.pie"},
			{"blguard1.pie", "blguard_col.pie"},
			{"blguardr.pie", "blguardr_col.pie"},
			{"blresch0.pie", "blresch0_col.pie"},
			{"blresch4.pie", "blresch4_col.pie"},
			{"research_module4.pie", "research_module4_col.pie"},
			{"blbresch.pie", "blbresch_col.pie"},
			{"blbpower.pie", "blbpower_col.pie"},
			{"blbhq.pie",  "blbhq_col.pie"},
			{"blbdrdcm.pie", "blbdrdcm_col.pie"},
			{"blpower0.pie", "blpower0_col.pie"},
			{"blpower4.pie", "blpower4_col.pie"},
//			{"blpilbox.pie", "blpilbox_nex.pie"},
			{"blfact0.pie", "blfact0_col.pie"},
			{"factory_module1.pie", "factory_module1_col.pie"},
			{"blfact1.pie", "blfact1_col.pie"},
			{"factory_module2.pie", "factory_module2_col.pie"},
			{"blfact2.pie", "blfact2_col.pie"},
			{"blbfact.pie", "blbfact_col.pie"},
			{"bldrdcm0.pie", "bldrdcm0_col.pie"},
			{"blcfact1.pie", "blcfact1_col.pie"},
			{"blbcfact.pie", "blbcfact_col.pie"},
//			{"blcanpil.pie", "blcanpil_nex.pie"},
//			{"blhowmnt.pie", "blhowmnt_nex.pie"},
//			{"blguardm.pie", "blguardm_nex.pie"},
			{"blvfact0.pie", "blvfact0_col.pie"},
			{"blvfact1.pie", "blvfact1_col.pie"},
			{"blvfact2.pie", "blvfact2_col.pie"},
			{"vtolfactory_module1.pie", "vtolfactory_module1_col.pie"},
			{"vtolfactory_module2.pie", "vtolfactory_module2_col.pie"},
			{"blderik_anim.pie", "blderik_anim_col.pie"},
			{"blderik.pie", "blderik_col.pie"},
//			{"blmrtpit.pie", "blmrtpit_nex.pie"}
		}
	}
};

optional<WzString> getFactionModelName(const FACTION *faction, const WzString& normalFactionName)
{
	auto pos = faction->replaceIMD.find(normalFactionName);
	if (pos == faction->replaceIMD.end())
	{
		return nullopt;
	} else {
		return pos->second;
	}
}

optional<WzString> getFactionModelName(const FactionID faction, const WzString& normalFactionName)
{
	return getFactionModelName(getFactionByID(faction), normalFactionName);
}

iIMDShape* getFactionIMD(const FACTION *faction, iIMDShape* imd)
{
	WzString name = WzString::fromUtf8(modelName(imd));
	auto factionModelName = getFactionModelName(faction, name);
	if (!factionModelName.has_value())
	{
		return imd;
	}
	else
	{
		return modelGet(factionModelName.value());
	}
}

const FACTION* getPlayerFaction(uint8_t player)
{
	return &(factions[NetPlay.players[player].faction]);
}

const FACTION* getFactionByID(FactionID faction)
{
	return &(factions[(uint8_t)faction]);
}

std::unordered_set<FactionID> getEnabledFactions(bool ignoreNormalFaction /*= false*/)
{
	std::unordered_set<FactionID> enabledFactions;
	for (uint8_t f_id = 0; f_id < NUM_FACTIONS; ++f_id)
	{
		const FactionID faction = static_cast<FactionID>(f_id);
		if (ignoreNormalFaction && (faction == FACTION_NORMAL))
		{
			continue;
		}
		for (size_t player = 0; player < NetPlay.players.size(); ++player)
		{
			if (NetPlay.players[player].faction == faction)
			{
				enabledFactions.insert(faction);
				continue;
			}
		}
	}
	return enabledFactions;
}

const char* to_string(FactionID faction)
{
	switch (faction)
	{
		case FACTION_NORMAL:
			return "Normal";
		case FACTION_NEXUS:
			return "NEXUS";
		case FACTION_COLLECTIVE:
			return "Collective";
	}
	return ""; // silence warning - switch above should be complete
}

const char* to_localized_string(FactionID faction)
{
	switch (faction)
	{
		case FACTION_NORMAL:
			// TRANSLATORS: "Normal" Faction
			return _("Normal");
		case FACTION_NEXUS:
			// TRANSLATORS: "NEXUS" Faction
			return _("NEXUS");
		case FACTION_COLLECTIVE:
			// TRANSLATORS: "Collective" Faction
			return _("Collective");
	}
	return ""; // silence warning - switch above should be complete
}
