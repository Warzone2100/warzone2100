/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** @file
 *  Research effect description generation.
 */

#include "researcheffecttext.h"

#include "../stats.h"
#include "../structure.h"

#include <algorithm>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace
{

// Which direction of change is an improvement.
// Example: Fire pause and reload time are ones where a negative == faster shooting, so describing it as a "loss" would be incorrect.
enum class Polarity : uint8_t
{
	HigherIsBetter,
	LowerIsBetter,
};

// "Reload" is what the shipped multiplayer data calls these lines, ex. "Cannon Reload",
// so a derived name reads beside authored ones. Here so the translators line properly ties to it.
// TRANSLATORS: A title-length name for how fast a weapon fires (i.e. "Reload")
const char *const TITLE_RELOAD = NP_("research progression", "Reload");

// Titles are their own strings rather than the labels with their capitals changed.
// Which words a title capitalizes is often a property of the language.
// A title is also free to be shorter, and to use the word the shipped data uses,
// which is where "rate of fire" becomes "Reload".
//
// The context is written out at every one because xgettext does not expand macros.
struct EffectRule
{
	const char *statClass;
	const char *parameter;
	const char *label;		// what changed - as a noun phrase in a sentence
	const char *title;		// what changed - as a name on its own
	Polarity polarity;
};

// Two rows sharing a label merge, which is why fire pause and reload time carry the
// same one: they move together and mean one thing to a player. Order matters within
// a class, the defining parameter coming first, so damage leads splash and burn
// damage and nameResearchProgression() can take the earliest match.
const EffectRule EFFECT_RULES[] = {
	{ "Weapon",    "Damage",              N_("damage"),                 NP_("research progression", "Damage"),                 Polarity::HigherIsBetter },
	{ "Weapon",    "RadiusDamage",        N_("splash damage"),          NP_("research progression", "Splash Damage"),          Polarity::HigherIsBetter },
	{ "Weapon",    "RepeatDamage",        N_("burn damage"),            NP_("research progression", "Burn Damage"),            Polarity::HigherIsBetter },
	{ "Weapon",    "FirePause",           N_("rate of fire"),           TITLE_RELOAD,                                          Polarity::LowerIsBetter },
	{ "Weapon",    "ReloadTime",          N_("rate of fire"),           TITLE_RELOAD,                                          Polarity::LowerIsBetter },
	{ "Weapon",    "HitChance",           N_("accuracy"),               NP_("research progression", "Accuracy"),               Polarity::HigherIsBetter },
	{ "Weapon",    "ShortHitChance",      N_("accuracy"),               NP_("research progression", "Accuracy"),               Polarity::HigherIsBetter },
	{ "Weapon",    "MaxRange",            N_("range"),                  NP_("research progression", "Range"),                  Polarity::HigherIsBetter },
	{ "Weapon",    "MinRange",            N_("minimum range"),          NP_("research progression", "Minimum Range"),          Polarity::LowerIsBetter },
	{ "Weapon",    "MinimumDamage",       N_("minimum damage"),         NP_("research progression", "Minimum Damage"),         Polarity::HigherIsBetter },
	{ "Weapon",    "Radius",              N_("splash radius"),          NP_("research progression", "Splash Radius"),          Polarity::HigherIsBetter },
	{ "Weapon",    "RepeatRadius",        N_("burn radius"),            NP_("research progression", "Burn Radius"),            Polarity::HigherIsBetter },
	{ "Weapon",    "EmpRadius",           N_("EMP radius"),             NP_("research progression", "EMP Radius"),             Polarity::HigherIsBetter },
	{ "Weapon",    "Rounds",              N_("rounds per salvo"),       NP_("research progression", "Rounds per Salvo"),       Polarity::HigherIsBetter },
	{ "Weapon",    "HitPoints",           N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },
	{ "Weapon",    "HitPointPct",         N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },

	// "Armour" is the key the data uses, so it stays as the data spells it.
	// Only the wording beside it is ours to spell.
	{ "Body",      "Armour",              N_("armor"),                  NP_("research progression", "Armor"),                  Polarity::HigherIsBetter },
	{ "Body",      "Thermal",             N_("thermal armor"),          NP_("research progression", "Thermal Armor"),          Polarity::HigherIsBetter },
	{ "Body",      "HitPointPct",         N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },
	{ "Body",      "HitPoints",           N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },
	{ "Body",      "Power",               N_("engine output"),          NP_("research progression", "Engine Output"),          Polarity::HigherIsBetter },
	{ "Body",      "Resistance",          N_("resistance to takeover"), NP_("research progression", "Takeover Resistance"),    Polarity::HigherIsBetter },

	{ "Building",  "Armour",              N_("armor"),                  NP_("research progression", "Armor"),                  Polarity::HigherIsBetter },
	{ "Building",  "Thermal",             N_("thermal armor"),          NP_("research progression", "Thermal Armor"),          Polarity::HigherIsBetter },
	{ "Building",  "HitPoints",           N_("hit points"),             NP_("research progression", "Hit Points"),             Polarity::HigherIsBetter },
	{ "Building",  "ResearchPoints",      N_("research speed"),         NP_("research progression", "Research Speed"),         Polarity::HigherIsBetter },
	{ "Building",  "ModuleResearchPoints", N_("research speed"),        NP_("research progression", "Research Speed"),         Polarity::HigherIsBetter },
	{ "Building",  "PowerPoints",         N_("power output"),           NP_("research progression", "Power Output"),           Polarity::HigherIsBetter },
	{ "Building",  "ModulePowerPoints",   N_("power output"),           NP_("research progression", "Power Output"),           Polarity::HigherIsBetter },
	{ "Building",  "ProductionPoints",    N_("production speed"),       NP_("research progression", "Production Speed"),       Polarity::HigherIsBetter },
	{ "Building",  "ModuleProductionPoints", N_("production speed"),    NP_("research progression", "Production Speed"),       Polarity::HigherIsBetter },
	{ "Building",  "RearmPoints",         N_("rearming speed"),         NP_("research progression", "Rearming Speed"),         Polarity::HigherIsBetter },
	{ "Building",  "RepairPoints",        N_("repair speed"),           NP_("research progression", "Repair Speed"),           Polarity::HigherIsBetter },
	{ "Building",  "Resistance",          N_("resistance to takeover"), NP_("research progression", "Takeover Resistance"),    Polarity::HigherIsBetter },
	{ "Building",  "Limit",               N_("how many can be built"),  NP_("research progression", "Build Limit"),            Polarity::HigherIsBetter },

	{ "Repair",    "RepairPoints",        N_("repair rate"),            NP_("research progression", "Repair Rate"),            Polarity::HigherIsBetter },
	{ "Repair",    "HitPoints",           N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },
	{ "Repair",    "HitPointPct",         N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },

	{ "Construct", "ConstructorPoints",   N_("build speed"),            NP_("research progression", "Build Speed"),            Polarity::HigherIsBetter },
	{ "Construct", "HitPoints",           N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },
	{ "Construct", "HitPointPct",         N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },

	{ "Sensor",    "Range",               N_("sensor range"),           NP_("research progression", "Sensor Range"),           Polarity::HigherIsBetter },
	{ "Sensor",    "HitPoints",           N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },
	{ "Sensor",    "HitPointPct",         N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },

	{ "ECM",       "Range",               N_("ECM range"),              NP_("research progression", "ECM Range"),              Polarity::HigherIsBetter },
	{ "ECM",       "HitPoints",           N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },
	{ "ECM",       "HitPointPct",         N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },

	// The engine reads this one from its stats under a lowercase p, which is the
	// spelling a data set has to use
	{ "Propulsion", "HitpointPctOfBody",  N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },
	{ "Propulsion", "HitPoints",          N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },
	{ "Propulsion", "HitPointPct",        N_("body points"),            NP_("research progression", "Body Points"),            Polarity::HigherIsBetter },

	{ "Brain",     "BaseCommandLimit",    N_("units commandable"),      NP_("research progression", "Command Limit"),          Polarity::HigherIsBetter },
	{ "Brain",     "CommandLimitByLevel", N_("units commandable per rank"), NP_("research progression", "Command Limit per Rank"), Polarity::HigherIsBetter },
	{ "Brain",     "HitPoints",           N_("hit points"),             NP_("research progression", "Hit Points"),             Polarity::HigherIsBetter },
	{ "Brain",     "HitPointPct",         N_("hit points"),             NP_("research progression", "Hit Points"),             Polarity::HigherIsBetter },
	{ "Brain",     "RankThresholds",      N_("rank progress speed"),    NP_("research progression", "Rank Progress"),          Polarity::LowerIsBetter },
};

const EffectRule *ruleFor(const std::string& statClass, const std::string& parameter)
{
	for (const auto& rule : EFFECT_RULES)
	{
		if (statClass == rule.statClass && parameter == rule.parameter)
		{
			return &rule;
		}
	}

	// A dataset using something not covered here would otherwise describe itself as doing nothing, so log it once
	static std::set<std::string> reported;
	const std::string key = statClass + "." + parameter;
	if (reported.insert(key).second)
	{
		debug(LOG_INFO, "No research effect wording for \"%s\", its topics will not say what they change", key.c_str());
	}
	return nullptr;
}

// A weapon kind standing in front of what it improves - ex. the "Cannon" of "Cannon Damage".
// English wants the singular, where the display names the rest of the interface uses are plural.
// Every kind is written out even where English repeats the display name, since which form a word
// takes in front of another is a property of the language.
const char *weaponClassAsModifier(WEAPON_SUBCLASS subClass)
{
	switch (subClass)
	{
	case WSC_MGUN:			return P_("research progression", "MG");
	case WSC_CANNON:		return P_("research progression", "Cannon");
	case WSC_MORTARS:		return P_("research progression", "Mortar");
	case WSC_MISSILE:		return P_("research progression", "Missile");
	case WSC_ROCKET:		return P_("research progression", "Rocket");
	case WSC_ENERGY:		return P_("research progression", "Energy");
	case WSC_GAUSS:			return P_("research progression", "Gauss");
	case WSC_FLAME:			return P_("research progression", "Flame");
	case WSC_HOWITZERS:		return P_("research progression", "Howitzer");
	case WSC_ELECTRONIC:		return P_("research progression", "Electronic");
	// TRANSLATORS: Anti-air, in front of what it improves, ex. as in "A-A Damage"
	case WSC_AAGUN:			return P_("research progression", "A-A");
	case WSC_SLOWMISSILE:		return P_("research progression", "Slow Missile");
	case WSC_SLOWROCKET:		return P_("research progression", "Slow Rocket");
	case WSC_LAS_SAT:		return P_("research progression", "Las-Sat");
	case WSC_BOMB:			return P_("research progression", "Bomb");
	case WSC_COMMAND:		return P_("research progression", "Command");
	case WSC_EMP:			return P_("research progression", "EMP");
	case WSC_NUM_WEAPON_SUBCLASSES: break;
	}
	return getWeaponSubClassDisplayName(subClass, true);
}

// What the change applies to.
WzString subjectName(const std::string& filterParameter, const std::string& filterValue, bool asTitle = false)
{
	if (filterParameter == "ImpactClass")
	{
		WEAPON_SUBCLASS subClass = WSC_NUM_WEAPON_SUBCLASSES;
		if (getWeaponSubClass(filterValue.c_str(), &subClass))
		{
			return WzString::fromUtf8(asTitle ? weaponClassAsModifier(subClass)
			                                  : getWeaponSubClassDisplayName(subClass, true));
		}
		return WzString();
	}
	if (filterParameter == "BodyClass")
	{
		if (filterValue == "Droids")		{ return WzString::fromUtf8(asTitle ? P_("research progression", "Droids") : _("droids")); }
		if (filterValue == "Cyborgs")		{ return WzString::fromUtf8(asTitle ? P_("research progression", "Cyborgs") : _("cyborgs")); }
		if (filterValue == "Transports")	{ return WzString::fromUtf8(asTitle ? P_("research progression", "Transports") : _("transports")); }
		return WzString();
	}
	if (filterParameter == "Type")
	{
		if (filterValue == "Structure")	{ return WzString::fromUtf8(asTitle ? P_("research progression", "Structures") : _("structures")); }
		if (filterValue == "Wall")	{ return WzString::fromUtf8(asTitle ? P_("research progression", "Walls") : _("walls")); }
		return WzString();
	}
	if (filterParameter == "Id")
	{
		const WzString name = WzString::fromUtf8(filterValue);
		if (const COMPONENT_STATS *component = getCompStatsFromName(name))
		{
			return WzString::fromUtf8(getLocalizedStatsName(component));
		}
		if (const STRUCTURE_STATS *structure = getStructStatsFromName(name))
		{
			return WzString::fromUtf8(getLocalizedStatsName(structure));
		}
		return WzString();
	}
	return WzString();
}

std::string jsonString(const nlohmann::json& object, const char *key)
{
	const auto it = object.find(key);
	return (it != object.end() && it->is_string()) ? it->get<std::string>() : std::string();
}

struct Grouped
{
	const char *label = nullptr;
	int64_t shownValue = 0;
	std::vector<WzString> subjects;
};

} // anonymous namespace

std::vector<EffectPhrase> describeResearchEffects(const RESEARCH& research)
{
	std::vector<EffectPhrase> phrases;
	if (!research.results.is_array())
	{
		return phrases;
	}

	std::vector<Grouped> groups;
	for (const auto& result : research.results)
	{
		if (!result.is_object())
		{
			continue;
		}
		const auto itValue = result.find("value");
		if (itValue == result.end() || !itValue->is_number_integer())
		{
			continue;
		}
		const EffectRule *rule = ruleFor(jsonString(result, "class"), jsonString(result, "parameter"));
		if (rule == nullptr)
		{
			continue;
		}

		// Stored as a change to the underlying number, so a lower-is-better one is flipped
		// to read as the improvement that it is
		const int64_t stored = itValue->get<int64_t>();
		const int64_t shown = (rule->polarity == Polarity::LowerIsBetter) ? -stored : stored;
		if (shown == 0)
		{
			continue;
		}

		const WzString subject = subjectName(jsonString(result, "filterParameter"), jsonString(result, "filterValue"));

		auto existing = std::find_if(groups.begin(), groups.end(), [rule, shown](const Grouped& group) {
			return strcmp(group.label, rule->label) == 0 && group.shownValue == shown;
		});
		if (existing == groups.end())
		{
			groups.push_back({rule->label, shown, {}});
			existing = groups.end() - 1;
		}
		if (!subject.isEmpty() && std::find(existing->subjects.begin(), existing->subjects.end(), subject) == existing->subjects.end())
		{
			existing->subjects.push_back(subject);
		}
	}

	phrases.reserve(groups.size());
	for (const auto& group : groups)
	{
		WzString text = WzString::format("%+d%% %s", static_cast<int>(group.shownValue), gettext(group.label));
		if (!group.subjects.empty())
		{
			WzString subjects = group.subjects.front();
			for (size_t i = 1; i < group.subjects.size(); ++i)
			{
				subjects += WzString::fromUtf8(", ") + group.subjects[i];
			}
			text += WzString::fromUtf8(" - ") + subjects;
		}
		phrases.push_back({text, group.shownValue > 0});
	}
	return phrases;
}

WzString nameResearchProgression(const RESEARCH& research)
{
	if (!research.results.is_array())
	{
		return WzString();
	}

	// What the progression is about is the first thing it changes, in the order the rules are written.
	// The rows are one array, so the earliest match is the smallest pointer.
	const EffectRule *naming = nullptr;
	for (const auto& result : research.results)
	{
		if (!result.is_object())
		{
			continue;
		}
		const EffectRule *rule = ruleFor(jsonString(result, "class"), jsonString(result, "parameter"));
		if (rule != nullptr && (naming == nullptr || rule < naming))
		{
			naming = rule;
		}
	}
	if (naming == nullptr)
	{
		return WzString();
	}

	// Only the rows saying that same thing say what it applies to, so the two halves of the title always agree.
	std::vector<WzString> subjects;
	for (const auto& result : research.results)
	{
		if (!result.is_object())
		{
			continue;
		}
		const EffectRule *rule = ruleFor(jsonString(result, "class"), jsonString(result, "parameter"));
		if (rule == nullptr || strcmp(rule->label, naming->label) != 0)
		{
			continue;
		}
		const WzString subject = subjectName(jsonString(result, "filterParameter"), jsonString(result, "filterValue"), true);
		if (!subject.isEmpty() && std::find(subjects.begin(), subjects.end(), subject) == subjects.end())
		{
			subjects.push_back(subject);
		}
	}

	WzString name;
	for (const auto& subject : subjects)
	{
		if (!name.isEmpty())
		{
			name += WzString::fromUtf8(", ");
		}
		name += subject;
	}
	if (!name.isEmpty())
	{
		name += " ";
	}
	name += WzString::fromUtf8(PE_("research progression", naming->title));
	return name;
}
