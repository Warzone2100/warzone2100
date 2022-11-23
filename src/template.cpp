/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2021  Warzone 2100 Project

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
/**
 * @file template.cpp
 *
 * Droid template functions.
 *
 */
#include "lib/framework/frame.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/math_ext.h"
#include "lib/framework/strres.h"
#include "lib/netplay/netplay.h"
#include "template.h"
#include "cmddroiddef.h"
#include "mission.h"
#include "objects.h"
#include "droid.h"
#include "design.h"
#include "hci.h"
#include "multiplay.h"
#include "projectile.h"
#include "main.h"
#include "research.h"

// Template storage
std::map<UDWORD, std::unique_ptr<DROID_TEMPLATE>> droidTemplates[MAX_PLAYERS];
std::vector<std::unique_ptr<DROID_TEMPLATE>> replacedDroidTemplates[MAX_PLAYERS];

#define ASSERT_PLAYER_OR_RETURN(retVal, player) \
	ASSERT_OR_RETURN(retVal, player >= 0 && player < MAX_PLAYERS, "Invalid player: %" PRIu32 "", player);

bool allowDesign = true;
bool includeRedundantDesigns = false;
bool playerBuiltHQ = false;


static bool researchedItem(const DROID_TEMPLATE* /*psCurr*/, int player, COMPONENT_TYPE partIndex, int part, bool allowZero, bool allowRedundant)
{
	ASSERT_PLAYER_OR_RETURN(false, player);
	if (allowZero && part <= 0)
	{
		return true;
	}
	int availability = apCompLists[player][partIndex][part];
	return availability == AVAILABLE || (allowRedundant && availability == REDUNDANT);
}

static bool researchedPart(const DROID_TEMPLATE *psCurr, int player, COMPONENT_TYPE partIndex, bool allowZero, bool allowRedundant)
{
	return researchedItem(psCurr, player, partIndex, psCurr->asParts[partIndex], allowZero, allowRedundant);
}

static bool researchedWeap(const DROID_TEMPLATE *psCurr, int player, int weapIndex, bool allowRedundant)
{
	ASSERT_PLAYER_OR_RETURN(false, player);
	int availability = apCompLists[player][COMP_WEAPON][psCurr->asWeaps[weapIndex]];
	return availability == AVAILABLE || (allowRedundant && availability == REDUNDANT);
}

bool researchedTemplate(const DROID_TEMPLATE *psCurr, int player, bool allowRedundant, bool verbose)
{
	ASSERT_OR_RETURN(false, psCurr, "Given a null template");
	ASSERT_PLAYER_OR_RETURN(false, player);
	bool resBody = researchedPart(psCurr, player, COMP_BODY, false, allowRedundant);
	bool resBrain = researchedPart(psCurr, player, COMP_BRAIN, true, allowRedundant);
	bool resProp = researchedPart(psCurr, player, COMP_PROPULSION, false, allowRedundant);
	bool resSensor = researchedPart(psCurr, player, COMP_SENSOR, true, allowRedundant);
	bool resEcm = researchedPart(psCurr, player, COMP_ECM, true, allowRedundant);
	bool resRepair = researchedPart(psCurr, player, COMP_REPAIRUNIT, true, allowRedundant);
	bool resConstruct = researchedPart(psCurr, player, COMP_CONSTRUCT, true, allowRedundant);
	bool researchedEverything = resBody && resBrain && resProp && resSensor && resEcm && resRepair && resConstruct;
	if (verbose && !researchedEverything)
	{
		debug(LOG_ERROR, "%s : not researched : body=%d brai=%d prop=%d sensor=%d ecm=%d rep=%d con=%d", getStatsName(psCurr),
		      (int)resBody, (int)resBrain, (int)resProp, (int)resSensor, (int)resEcm, (int)resRepair, (int)resConstruct);
	}
	for (int weapIndex = 0; weapIndex < psCurr->numWeaps && researchedEverything; ++weapIndex)
	{
		researchedEverything = researchedWeap(psCurr, player, weapIndex, allowRedundant);
		if (!researchedEverything && verbose)
		{
			debug(LOG_ERROR, "%s : not researched weapon %u", getStatsName(psCurr), weapIndex);
		}
	}
	return researchedEverything;
}

bool droidTemplate_LoadPartByName(COMPONENT_TYPE compType, const WzString &name, DROID_TEMPLATE &outputTemplate)
{
	int index = getCompFromName(compType, name);
	if (index < 0)
	{
		debug(LOG_ERROR, "Stored template contains an unknown (type: %d) component: %s.", compType, name.toUtf8().c_str());
		return false;
	}
	if (index > UINT8_MAX)
	{
		// returned index exceeds uint8_t max - consider changing type of asParts in DROID_TEMPLATE?
		debug(LOG_ERROR, "Stored template contains a (type: %d) component (%s) index that exceeds UINT8_MAX: %d", compType, name.toUtf8().c_str(), index);
		return false;
	}
	outputTemplate.asParts[compType] = static_cast<uint8_t>(index);
	return true;
}

bool droidTemplate_LoadWeapByName(size_t destIndex, const WzString &name, DROID_TEMPLATE &outputTemplate)
{
	int index = getCompFromName(COMP_WEAPON, name);
	if (index < 0)
	{
		debug(LOG_ERROR, "Stored template contains an unknown (type: %d) component: %s.", COMP_WEAPON, name.toUtf8().c_str());
		return false;
	}
#if INT_MAX > UINT32_MAX
	if (index > (int)UINT32_MAX)
	{
		// returned index exceeds uint32_t max - consider changing type of asWeaps in DROID_TEMPLATE?
		debug(LOG_ERROR, "Stored template contains a (type: %d) component (%s) index that exceeds UINT32_MAX: %d", COMP_WEAPON, name.toUtf8().c_str(), index);
		return false;
	}
#endif
	outputTemplate.asWeaps[destIndex] = static_cast<uint32_t>(index);
	return true;
}

bool loadTemplateCommon(WzConfig &ini, DROID_TEMPLATE &outputTemplate)
{
	DROID_TEMPLATE &design = outputTemplate;
	design.name = ini.string("name");
	WzString droidType = ini.value("type").toWzString();

	if (droidType == "ECM")
	{
		design.droidType = DROID_ECM;
	}
	else if (droidType == "SENSOR")
	{
		design.droidType = DROID_SENSOR;
	}
	else if (droidType == "CONSTRUCT")
	{
		design.droidType = DROID_CONSTRUCT;
	}
	else if (droidType == "WEAPON")
	{
		design.droidType = DROID_WEAPON;
	}
	else if (droidType == "PERSON")
	{
		design.droidType = DROID_PERSON;
	}
	else if (droidType == "CYBORG")
	{
		design.droidType = DROID_CYBORG;
	}
	else if (droidType == "CYBORG_SUPER")
	{
		design.droidType = DROID_CYBORG_SUPER;
	}
	else if (droidType == "CYBORG_CONSTRUCT")
	{
		design.droidType = DROID_CYBORG_CONSTRUCT;
	}
	else if (droidType == "CYBORG_REPAIR")
	{
		design.droidType = DROID_CYBORG_REPAIR;
	}
	else if (droidType == "TRANSPORTER")
	{
		design.droidType = DROID_TRANSPORTER;
	}
	else if (droidType == "SUPERTRANSPORTER")
	{
		design.droidType = DROID_SUPERTRANSPORTER;
	}
	else if (droidType == "DROID")
	{
		design.droidType = DROID_DEFAULT;
	}
	else if (droidType == "DROID_COMMAND")
	{
		design.droidType = DROID_COMMAND;
	}
	else if (droidType == "REPAIR")
	{
		design.droidType = DROID_REPAIR;
	}
	else
	{
		ASSERT(false, "No such droid type \"%s\" for %s", droidType.toUtf8().c_str(), getID(&design));
	}

	if (!droidTemplate_LoadPartByName(COMP_BODY, ini.value("body").toWzString(), design)) return false;
	if (!droidTemplate_LoadPartByName(COMP_BRAIN, ini.value("brain", WzString("ZNULLBRAIN")).toWzString(), design)) return false;
	if (!droidTemplate_LoadPartByName(COMP_PROPULSION, ini.value("propulsion", WzString("ZNULLPROP")).toWzString(), design)) return false;
	if (!droidTemplate_LoadPartByName(COMP_REPAIRUNIT, ini.value("repair", WzString("ZNULLREPAIR")).toWzString(), design)) return false;
	if (!droidTemplate_LoadPartByName(COMP_ECM, ini.value("ecm", WzString("ZNULLECM")).toWzString(), design)) return false;
	if (!droidTemplate_LoadPartByName(COMP_SENSOR, ini.value("sensor", WzString("ZNULLSENSOR")).toWzString(), design)) return false;
	if (!droidTemplate_LoadPartByName(COMP_CONSTRUCT, ini.value("construct", WzString("ZNULLCONSTRUCT")).toWzString(), design)) return false;

	std::vector<WzString> weapons = ini.value("weapons").toWzStringList();
	ASSERT(weapons.size() <= MAX_WEAPONS, "Number of weapons (%zu) exceeds MAX_WEAPONS (%d)", weapons.size(), MAX_WEAPONS);
	design.numWeaps = (weapons.size() <= MAX_WEAPONS) ? (int8_t)weapons.size() : MAX_WEAPONS;
	if (!droidTemplate_LoadWeapByName(0, (weapons.size() >= 1) ? weapons[0] : "ZNULLWEAPON", design)) return false;
	if (!droidTemplate_LoadWeapByName(1, (weapons.size() >= 2) ? weapons[1] : "ZNULLWEAPON", design)) return false;
	if (!droidTemplate_LoadWeapByName(2, (weapons.size() >= 3) ? weapons[2] : "ZNULLWEAPON", design)) return false;

	return true;
}

// A way to check if a design is something someone could legitimately have in multiplayer
bool designableTemplate(DROID_TEMPLATE *psTempl, int player)
{
	if (!bMultiPlayer || !isHumanPlayer(player))
	{
		return true; // Don't care about AIs
	}

	char const *failPart = nullptr;
	WzString failPartName;
	auto designablePart = [&](COMPONENT_STATS const &component, char const *part) {
		if (!component.designable)
		{
			failPart = part;
			failPartName = component.name;
		}
		return component.designable;
	};

	bool designable = false;

	if (psTempl->droidType == DROID_CYBORG ||
		psTempl->droidType == DROID_CYBORG_SUPER ||
		psTempl->droidType == DROID_CYBORG_CONSTRUCT ||
		psTempl->droidType == DROID_CYBORG_REPAIR)
	{
		bool repair = ((psTempl->asParts[COMP_REPAIRUNIT] == 0) ||
						(!designablePart(asRepairStats[psTempl->asParts[COMP_REPAIRUNIT]], "Repair unit") && asRepairStats[psTempl->asParts[COMP_REPAIRUNIT]].usageClass == UsageClass::Cyborg) ||
						(psTempl->asParts[COMP_REPAIRUNIT] != 0 && selfRepairEnabled(player)));
		bool isSuperCyborg = asBodyStats[psTempl->asParts[COMP_BODY]].usageClass == UsageClass::SuperCyborg;

		designable =
			   !designablePart(asBodyStats[psTempl->asParts[COMP_BODY]], "Body")
			&& (strcmp(asBodyStats[psTempl->asParts[COMP_BODY]].bodyClass.toStdString().c_str(), "Cyborgs") == 0)
			&& !designablePart(asPropulsionStats[psTempl->asParts[COMP_PROPULSION]], "Propulsion")
			&& asPropulsionStats[psTempl->asParts[COMP_PROPULSION]].propulsionType == PROPULSION_TYPE_LEGGED
			&& asPropulsionStats[psTempl->asParts[COMP_PROPULSION]].usageClass == UsageClass::Cyborg
			&& repair
			&& ((psTempl->asParts[COMP_BRAIN] == 0) || (!designablePart(asBrainStats[psTempl->asParts[COMP_BRAIN]], "Brain") && asBrainStats[psTempl->asParts[COMP_BRAIN]].usageClass == UsageClass::Cyborg))
			&& ((psTempl->asParts[COMP_ECM] == 0) || (!designablePart(asECMStats[psTempl->asParts[COMP_ECM]], "ECM") && asECMStats[psTempl->asParts[COMP_ECM]].usageClass == UsageClass::Cyborg))
			&& ((psTempl->asParts[COMP_SENSOR] == 0) || (!designablePart(asSensorStats[psTempl->asParts[COMP_SENSOR]], "Sensor") && asSensorStats[psTempl->asParts[COMP_SENSOR]].usageClass == UsageClass::Cyborg))
			&& ((psTempl->asParts[COMP_CONSTRUCT] == 0) || (!designablePart(asConstructStats[psTempl->asParts[COMP_CONSTRUCT]], "Construction part") && asConstructStats[psTempl->asParts[COMP_CONSTRUCT]].usageClass == UsageClass::Cyborg))
			&& ((psTempl->numWeaps <= 0) || (asBrainStats[psTempl->asParts[COMP_BRAIN]].psWeaponStat == &asWeaponStats[psTempl->asWeaps[0]])
				|| (!designablePart(asWeaponStats[psTempl->asWeaps[0]], "Weapon 0") && ((!isSuperCyborg && asWeaponStats[psTempl->asWeaps[0]].usageClass == UsageClass::Cyborg) || (isSuperCyborg && asWeaponStats[psTempl->asWeaps[0]].usageClass == UsageClass::SuperCyborg))))
			&& ((psTempl->numWeaps <= 1) || (!designablePart(asWeaponStats[psTempl->asWeaps[1]], "Weapon 1") && ((!isSuperCyborg && asWeaponStats[psTempl->asWeaps[1]].usageClass == UsageClass::Cyborg) || (isSuperCyborg && asWeaponStats[psTempl->asWeaps[1]].usageClass == UsageClass::SuperCyborg))))
			&& ((psTempl->numWeaps <= 2) || (!designablePart(asWeaponStats[psTempl->asWeaps[2]], "Weapon 2") && ((!isSuperCyborg && asWeaponStats[psTempl->asWeaps[2]].usageClass == UsageClass::Cyborg) || (isSuperCyborg && asWeaponStats[psTempl->asWeaps[2]].usageClass == UsageClass::SuperCyborg))));
	}
	else
	{
		bool repair = ((psTempl->asParts[COMP_REPAIRUNIT] == 0) ||
						designablePart(asRepairStats[psTempl->asParts[COMP_REPAIRUNIT]], "Repair unit") ||
						(psTempl->asParts[COMP_REPAIRUNIT] != 0 && selfRepairEnabled(player)));
		bool transporter = (psTempl->droidType == DROID_TRANSPORTER) || (psTempl->droidType == DROID_SUPERTRANSPORTER);

		designable =
			   (transporter || (!transporter && designablePart(asBodyStats[psTempl->asParts[COMP_BODY]], "Body")))
			&& designablePart(asPropulsionStats[psTempl->asParts[COMP_PROPULSION]], "Propulsion")
			&& (psTempl->asParts[COMP_BRAIN]      == 0 || designablePart(asBrainStats    [psTempl->asParts[COMP_BRAIN]],      "Brain"))
			&& repair
			&& (psTempl->asParts[COMP_ECM]        == 0 || designablePart(asECMStats      [psTempl->asParts[COMP_ECM]],        "ECM"))
			&& (psTempl->asParts[COMP_SENSOR]     == 0 || designablePart(asSensorStats   [psTempl->asParts[COMP_SENSOR]],     "Sensor"))
			&& (psTempl->asParts[COMP_CONSTRUCT]  == 0 || designablePart(asConstructStats[psTempl->asParts[COMP_CONSTRUCT]],  "Construction part"))
			&& (psTempl->numWeaps <= 0 || asBrainStats[psTempl->asParts[COMP_BRAIN]].psWeaponStat == &asWeaponStats[psTempl->asWeaps[0]]
									 || designablePart(asWeaponStats[psTempl->asWeaps[0]], "Weapon 0"))
			&& (psTempl->numWeaps <= 1 || designablePart(asWeaponStats[psTempl->asWeaps[1]], "Weapon 1"))
			&& (psTempl->numWeaps <= 2 || designablePart(asWeaponStats[psTempl->asWeaps[2]], "Weapon 2"));

		if (transporter && designable)
		{
			designable = (asPropulsionStats[psTempl->asParts[COMP_PROPULSION]].propulsionType == PROPULSION_TYPE_LIFT) &&
						!designablePart(asBodyStats[psTempl->asParts[COMP_BODY]], "Body");
			if (designable)
			{
				designable = strcmp(asBodyStats[psTempl->asParts[COMP_BODY]].bodyClass.toStdString().c_str(), "Transports") == 0;
			}
		}
	}

	if (!designable)
	{
		debug(LOG_ERROR, "%s \"%s\" for \"%s\" cannot be designed", failPart, failPartName.toUtf8().c_str(), psTempl->name.toUtf8().c_str());
	}

	return designable;
}

bool initTemplates()
{
	if (selectedPlayer >= MAX_PLAYERS) { return false; }

	WzConfig ini("userdata/" + WzString(rulesettag) + "/templates.json", WzConfig::ReadOnly);
	if (!ini.status())
	{
		debug(LOG_WZ, "Could not open %s", ini.fileName().toUtf8().c_str());
		return false;
	}
	int version = ini.value("version", 0).toInt();
	if (version == 0)
	{
		return true; // too old version
	}
	for (ini.beginArray("templates"); ini.remainingArrayItems(); ini.nextArrayItem())
	{
		DROID_TEMPLATE design;
		bool loadCommonSuccess = loadTemplateCommon(ini, design);
		design.multiPlayerID = generateNewObjectId();
		design.prefab = false;		// not AI template
		design.stored = true;

		if (!loadCommonSuccess)
		{
			debug(LOG_ERROR, "Stored template \"%s\" contains an unknown component.", design.name.toUtf8().c_str());
			continue;
		}
		bool valid = intValidTemplate(&design, ini.value("name").toWzString().toUtf8().c_str(), false, selectedPlayer);
		if (!valid)
		{
			debug(LOG_ERROR, "Invalid template \"%s\" from stored templates", design.name.toUtf8().c_str());
			continue;
		}
		DROID_TEMPLATE *psDestTemplate = nullptr;
		for (auto &keyvaluepair : droidTemplates[selectedPlayer])
		{
			psDestTemplate = keyvaluepair.second.get();
			// Check if template is identical to a loaded template
			if (psDestTemplate->droidType == design.droidType
			    && psDestTemplate->name.compare(design.name) == 0
			    && psDestTemplate->numWeaps == design.numWeaps
			    && psDestTemplate->asWeaps[0] == design.asWeaps[0]
			    && psDestTemplate->asWeaps[1] == design.asWeaps[1]
			    && psDestTemplate->asWeaps[2] == design.asWeaps[2]
			    && psDestTemplate->asParts[COMP_BODY] == design.asParts[COMP_BODY]
			    && psDestTemplate->asParts[COMP_PROPULSION] == design.asParts[COMP_PROPULSION]
			    && psDestTemplate->asParts[COMP_REPAIRUNIT] == design.asParts[COMP_REPAIRUNIT]
			    && psDestTemplate->asParts[COMP_ECM] == design.asParts[COMP_ECM]
			    && psDestTemplate->asParts[COMP_SENSOR] == design.asParts[COMP_SENSOR]
			    && psDestTemplate->asParts[COMP_CONSTRUCT] == design.asParts[COMP_CONSTRUCT]
			    && psDestTemplate->asParts[COMP_BRAIN] == design.asParts[COMP_BRAIN])
			{
				break;
			}
			psDestTemplate = nullptr;
		}
		if (psDestTemplate)
		{
			psDestTemplate->stored = true; // assimilate it
			continue; // next!
		}
		design.enabled = allowDesign;
		copyTemplate(selectedPlayer, &design);
		localTemplates.push_back(design);
	}
	ini.endArray();
	return true;
}

nlohmann::json saveTemplateCommon(const DROID_TEMPLATE *psCurr)
{
	nlohmann::json templateObj = nlohmann::json::object();
	templateObj["name"] = psCurr->name;
	switch (psCurr->droidType)
	{
	case DROID_ECM: templateObj["type"] = "ECM"; break;
	case DROID_SENSOR: templateObj["type"] = "SENSOR"; break;
	case DROID_CONSTRUCT: templateObj["type"] = "CONSTRUCT"; break;
	case DROID_WEAPON: templateObj["type"] = "WEAPON"; break;
	case DROID_PERSON: templateObj["type"] = "PERSON"; break;
	case DROID_CYBORG: templateObj["type"] = "CYBORG"; break;
	case DROID_CYBORG_SUPER: templateObj["type"] = "CYBORG_SUPER"; break;
	case DROID_CYBORG_CONSTRUCT: templateObj["type"] = "CYBORG_CONSTRUCT"; break;
	case DROID_CYBORG_REPAIR: templateObj["type"] = "CYBORG_REPAIR"; break;
	case DROID_TRANSPORTER: templateObj["type"] = "TRANSPORTER"; break;
	case DROID_SUPERTRANSPORTER: templateObj["type"] = "SUPERTRANSPORTER"; break;
	case DROID_COMMAND: templateObj["type"] = "DROID_COMMAND"; break;
	case DROID_REPAIR: templateObj["type"] = "REPAIR"; break;
	case DROID_DEFAULT: templateObj["type"] = "DROID"; break;
	default: ASSERT(false, "No such droid type \"%d\" for %s", psCurr->droidType, psCurr->name.toUtf8().c_str());
	}
	ASSERT(psCurr->asParts[COMP_BODY] < numBodyStats, "asParts[COMP_BODY] (%d) exceeds numBodyStats (%" PRIu32 ")", (int)psCurr->asParts[COMP_BODY], numBodyStats);
	templateObj["body"] = (asBodyStats + psCurr->asParts[COMP_BODY])->id;
	ASSERT(psCurr->asParts[COMP_PROPULSION] < numPropulsionStats, "asParts[COMP_PROPULSION] (%d) exceeds numPropulsionStats (%" PRIu32 ")", (int)psCurr->asParts[COMP_PROPULSION], numPropulsionStats);
	templateObj["propulsion"] = (asPropulsionStats + psCurr->asParts[COMP_PROPULSION])->id;
	if (psCurr->asParts[COMP_BRAIN] != 0)
	{
		ASSERT(psCurr->asParts[COMP_BRAIN] < numBrainStats, "asParts[COMP_BRAIN] (%d) exceeds numBrainStats (%" PRIu32 ")", (int)psCurr->asParts[COMP_BRAIN], numBrainStats);
		templateObj["brain"] = (asBrainStats + psCurr->asParts[COMP_BRAIN])->id;
	}
	if ((asRepairStats + psCurr->asParts[COMP_REPAIRUNIT])->location == LOC_TURRET) // avoid auto-repair...
	{
		ASSERT(psCurr->asParts[COMP_REPAIRUNIT] < numRepairStats, "asParts[COMP_REPAIRUNIT] (%d) exceeds numRepairStats (%" PRIu32 ")", (int)psCurr->asParts[COMP_REPAIRUNIT], numRepairStats);
		templateObj["repair"] = (asRepairStats + psCurr->asParts[COMP_REPAIRUNIT])->id;
	}
	if ((asECMStats + psCurr->asParts[COMP_ECM])->location == LOC_TURRET)
	{
		ASSERT(psCurr->asParts[COMP_ECM] < numECMStats, "asParts[COMP_ECM] (%d) exceeds numECMStats (%" PRIu32 ")", (int)psCurr->asParts[COMP_ECM], numECMStats);
		templateObj["ecm"] = (asECMStats + psCurr->asParts[COMP_ECM])->id;
	}
	if ((asSensorStats + psCurr->asParts[COMP_SENSOR])->location == LOC_TURRET)
	{
		ASSERT(psCurr->asParts[COMP_SENSOR] < numSensorStats, "asParts[COMP_SENSOR] (%d) exceeds numSensorStats (%" PRIu32 ")", (int)psCurr->asParts[COMP_SENSOR], numSensorStats);
		templateObj["sensor"] = (asSensorStats + psCurr->asParts[COMP_SENSOR])->id;
	}
	if (psCurr->asParts[COMP_CONSTRUCT] != 0)
	{
		ASSERT(psCurr->asParts[COMP_CONSTRUCT] < numConstructStats, "asParts[COMP_CONSTRUCT] (%d) exceeds numConstructStats (%" PRIu32 ")", (int)psCurr->asParts[COMP_CONSTRUCT], numConstructStats);
		templateObj["construct"] = (asConstructStats + psCurr->asParts[COMP_CONSTRUCT])->id;
	}
	nlohmann::json weapons = nlohmann::json::array();
	for (int j = 0; j < psCurr->numWeaps; j++)
	{
		ASSERT(psCurr->asWeaps[j] < numWeaponStats, "psCurr->asWeaps[%d] (%d) exceeds numWeaponStats (%" PRIu32 ")", j, (int)psCurr->asWeaps[j], numWeaponStats);
		weapons.push_back((asWeaponStats + psCurr->asWeaps[j])->id);
	}
	if (!weapons.empty())
	{
		templateObj["weapons"] = std::move(weapons);
	}
	return templateObj;
}

bool storeTemplates()
{
	if (selectedPlayer >= MAX_PLAYERS) { return false; }

	// Write stored templates (back) to file
	WzConfig ini("userdata/" + WzString(rulesettag) + "/templates.json", WzConfig::ReadAndWrite);
	if (!ini.status() || !ini.isWritable())
	{
		debug(LOG_ERROR, "Could not open %s", ini.fileName().toUtf8().c_str());
		return false;
	}
	ini.setValue("version", 1); // for breaking backwards compatibility in a nice way
	ini.beginArray("templates");
	for (auto &keyvaluepair : droidTemplates[selectedPlayer])
	{
		const DROID_TEMPLATE *psCurr = keyvaluepair.second.get();
		if (psCurr->stored)
		{
			ini.currentJsonValue() = saveTemplateCommon(psCurr);
			ini.nextArrayItem();
		}
	}
	ini.endArray();
	return true;
}

bool shutdownTemplates()
{
	return storeTemplates();
}

DROID_TEMPLATE::DROID_TEMPLATE()  // This constructor replaces a memset in scrAssembleWeaponTemplate(), not needed elsewhere.
	: BASE_STATS(STAT_TEMPLATE)
	  //, asParts
	, numWeaps(0)
	  //, asWeaps
	, droidType(DROID_WEAPON)
	, multiPlayerID(0)
	, prefab(false)
	, stored(false)
	, enabled(false)
{
	std::fill_n(asParts, DROID_MAXCOMP, static_cast<uint8_t>(0));
	std::fill_n(asWeaps, MAX_WEAPONS, 0);
}

bool loadDroidTemplates(const char *filename)
{
	WzConfig ini(filename, WzConfig::ReadOnlyAndRequired);
	std::vector<WzString> list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		DROID_TEMPLATE design;
		if (!loadTemplateCommon(ini, design))
		{
			debug(LOG_ERROR, "Stored template \"%s\" contains an unknown component.", ini.string("name").toUtf8().c_str());
			continue;
		}
		design.id = list[i];
		design.name = ini.string("name");
		design.multiPlayerID = generateNewObjectId();
		design.prefab = true;
		design.stored = false;
		design.enabled = true;
		bool available = ini.value("available", false).toBool();
		char const *droidResourceName = getDroidResourceName(list[i].toUtf8().c_str());
		design.name = WzString::fromUtf8(droidResourceName != nullptr ? droidResourceName : GetDefaultTemplateName(&design));
		ini.endGroup();

		for (int playerIdx = 0; playerIdx < MAX_PLAYERS; ++playerIdx)
		{
			// Give those meant for humans to all human players.
			if (NetPlay.players[playerIdx].allocated && available)
			{
				design.prefab = false;
				copyTemplate(playerIdx, &design);

				// This sets up the UI templates for display purposes ONLY--we still only use droidTemplates for making them.
				// FIXME: Why are we doing this here, and not on demand ?
				// Only add unique designs to the UI list (Note, perhaps better to use std::map instead?)
				std::list<DROID_TEMPLATE>::iterator it;
				for (it = localTemplates.begin(); it != localTemplates.end(); ++it)
				{
					DROID_TEMPLATE *psCurr = &*it;
					if (psCurr->multiPlayerID == design.multiPlayerID)
					{
						debug(LOG_WARNING, "Design id:%d (%s) *NOT* added to UI list (duplicate), player= %d", design.multiPlayerID, getStatsName(&design), playerIdx);
						break;
					}
				}
				if (it == localTemplates.end())
				{
					debug(LOG_NEVER, "Design id:%d (%s) added to UI list, player =%d", design.multiPlayerID, getStatsName(&design), playerIdx);
					localTemplates.push_back(design);
				}
			}
			else if (!NetPlay.players[playerIdx].allocated)	// AI template
			{
				design.prefab = true;  // prefabricated templates referenced from VLOs
				copyTemplate(playerIdx, &design);
			}
		}
		debug(LOG_NEVER, "Droid template found, Name: %s, MP ID: %d, ref: %u, ID: %s, prefab: %s, type:%d (loading)",
		      getStatsName(&design), design.multiPlayerID, design.ref, getID(&design), design.prefab ? "yes" : "no", design.droidType);
	}

	return true;
}

DROID_TEMPLATE *copyTemplate(int player, DROID_TEMPLATE *psTemplate)
{
	auto dup = std::unique_ptr<DROID_TEMPLATE>(new DROID_TEMPLATE(*psTemplate));
	return addTemplate(player, std::move(dup));
}

DROID_TEMPLATE* addTemplate(int player, std::unique_ptr<DROID_TEMPLATE> psTemplate)
{
	ASSERT_PLAYER_OR_RETURN(nullptr, player);
	UDWORD multiPlayerID = psTemplate->multiPlayerID;
	auto it = droidTemplates[player].find(multiPlayerID);
	if (it != droidTemplates[player].end())
	{
		// replacing existing template
		it->second.swap(psTemplate);
		replacedDroidTemplates[player].push_back(std::move(psTemplate));
		return it->second.get();
	}
	else
	{
		// new template
		auto result = droidTemplates[player].insert(std::pair<UDWORD, std::unique_ptr<DROID_TEMPLATE>>(multiPlayerID, std::move(psTemplate)));
		return result.first->second.get();
	}
}

void enumerateTemplates(int player, const std::function<bool (DROID_TEMPLATE* psTemplate)>& func)
{
	ASSERT_PLAYER_OR_RETURN(, player);
	for (auto &keyvaluepair : droidTemplates[player])
	{
		if (!func(keyvaluepair.second.get()))
		{
			break;
		}
	}
}

DROID_TEMPLATE* findPlayerTemplateById(int player, UDWORD templateId)
{
	ASSERT_PLAYER_OR_RETURN(nullptr, player);
	auto it = droidTemplates[player].find(templateId);
	if (it != droidTemplates[player].end())
	{
		return it->second.get();
	}
	return nullptr;
}

size_t templateCount(int player)
{
	ASSERT_PLAYER_OR_RETURN(0, player);
	return droidTemplates[player].size();
}

void clearTemplates(int player)
{
	ASSERT_PLAYER_OR_RETURN(, player);
	droidTemplates[player].clear();
	replacedDroidTemplates[player].clear();
}

//free the storage for the droid templates
bool droidTemplateShutDown()
{
	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		clearTemplates(player);
	}
	localTemplates.clear();
	return true;
}

/*!
 * Get a static template from its name. This is used from scripts. These templates must
 * never be changed or deleted.
 * \param pName Template name
 * \pre pName has to be the unique, untranslated name!
 */
const DROID_TEMPLATE *getTemplateFromTranslatedNameNoPlayer(char const *pName)
{
	for (auto &droidTemplate : droidTemplates)
	{
		for (auto &keyvaluepair : droidTemplate)
		{
			if (keyvaluepair.second->id.compare(pName) == 0)
			{
				return keyvaluepair.second.get();
			}
		}
	}
	return nullptr;
}

/*getTemplatefFromMultiPlayerID gets template for unique ID  searching all lists */
DROID_TEMPLATE *getTemplateFromMultiPlayerID(UDWORD multiPlayerID)
{
	for (auto &droidTemplate : droidTemplates)
	{
		if (droidTemplate.count(multiPlayerID) > 0)
		{
			return droidTemplate[multiPlayerID].get();
		}
	}
	return nullptr;
}

/*called when a Template is deleted in the Design screen*/
void deleteTemplateFromProduction(DROID_TEMPLATE *psTemplate, unsigned player, QUEUE_MODE mode)
{
	STRUCTURE   *psStruct;
	STRUCTURE	*psList;

	ASSERT_OR_RETURN(, psTemplate != nullptr, "Null psTemplate");
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Invalid player: %u", player);

	//see if any factory is currently using the template
	for (unsigned i = 0; i < 2; ++i)
	{
		psList = nullptr;
		switch (i)
		{
		case 0:
			psList = apsStructLists[player];
			break;
		case 1:
			psList = mission.apsStructLists[player];
			break;
		}
		for (psStruct = psList; psStruct != nullptr; psStruct = psStruct->psNext)
		{
			if (StructIsFactory(psStruct))
			{
				if (psStruct->pFunctionality == nullptr)
				{
					continue;
				}
				FACTORY *psFactory = &psStruct->pFunctionality->factory;

				if (psFactory->psAssemblyPoint && psFactory->psAssemblyPoint->factoryType < NUM_FACTORY_TYPES
					&& psFactory->psAssemblyPoint->factoryInc < asProductionRun[psFactory->psAssemblyPoint->factoryType].size())
				{
					ProductionRun &productionRun = asProductionRun[psFactory->psAssemblyPoint->factoryType][psFactory->psAssemblyPoint->factoryInc];
					for (unsigned inc = 0; inc < productionRun.size(); ++inc)
					{
						if (productionRun[inc].psTemplate && productionRun[inc].psTemplate->multiPlayerID == psTemplate->multiPlayerID && mode == ModeQueue)
						{
							//just need to erase this production run entry
							productionRun.erase(productionRun.begin() + inc);
							--inc;
						}
					}
				}

				if (psFactory->psSubject == nullptr)
				{
					continue;
				}

				// check not being built in the factory for the template player
				if (psTemplate->multiPlayerID == psFactory->psSubject->multiPlayerID && mode == ModeImmediate)
				{
					syncDebugStructure(psStruct, '<');
					syncDebug("Clearing production");

					// Clear the factory's subject, and returns power.
					cancelProduction(psStruct, ModeImmediate, false);
					// Check to see if anything left to produce. (Also calls cancelProduction again, if nothing left to produce, which is a no-op. But if other things are left to produce, doesn't call cancelProduction, so wouldn't return power without the explicit cancelProduction call above.)
					doNextProduction(psStruct, nullptr, ModeImmediate);

					syncDebugStructure(psStruct, '>');
				}
			}
		}
	}
}

// return whether a template is for an IDF droid
bool templateIsIDF(DROID_TEMPLATE *psTemplate)
{
	//add Cyborgs
	if (!(psTemplate->droidType == DROID_WEAPON || psTemplate->droidType == DROID_CYBORG || psTemplate->droidType == DROID_CYBORG_SUPER))
	{
		return false;
	}

	if (proj_Direct(psTemplate->asWeaps[0] + asWeaponStats))
	{
		return false;
	}

	return true;
}

void listTemplates()
{
	ASSERT_OR_RETURN(, selectedPlayer < MAX_PLAYERS, "selectedPlayer (%" PRIu32 ") >= MAX_PLAYERS", selectedPlayer);
	for (auto &keyvaluepair : droidTemplates[selectedPlayer])
	{
		DROID_TEMPLATE *t = keyvaluepair.second.get();
		debug(LOG_INFO, "template %s : %ld : %s : %s : %s", getStatsName(t), (long)t->multiPlayerID, t->enabled ? "Enabled" : "Disabled", t->stored ? "Stored" : "Temporal", t->prefab ? "Prefab" : "Designed");
	}
}

/*
fills the list with Templates that can be manufactured
in the Factory - based on size. There is a limit on how many can be manufactured
at any one time.
*/
std::vector<DROID_TEMPLATE *> fillTemplateList(STRUCTURE *psFactory)
{
	std::vector<DROID_TEMPLATE *> pList;
	const int player = psFactory->player;

	BODY_SIZE	iCapacity = (BODY_SIZE)psFactory->capacity;

	/* Add the templates to the list*/
	for (DROID_TEMPLATE &i : localTemplates)
	{
		DROID_TEMPLATE *psCurr = &i;
		// Must add droids if currently in production.
		if (!getProduction(psFactory, psCurr).quantity)
		{
			//can only have (MAX_CMDDROIDS) in the world at any one time
			if (psCurr->droidType == DROID_COMMAND)
			{
				if (checkProductionForCommand(player) + checkCommandExist(player) >= (MAX_CMDDROIDS))
				{
					continue;
				}
			}

			if (!psCurr->enabled
				|| (bMultiPlayer && !playerBuiltHQ && (psCurr->droidType != DROID_CONSTRUCT && psCurr->droidType != DROID_CYBORG_CONSTRUCT))
				|| !validTemplateForFactory(psCurr, psFactory, false)
				|| !researchedTemplate(psCurr, player, includeRedundantDesigns))
			{
				continue;
			}
		}

		//check the factory can cope with this sized body
		if (((asBodyStats + psCurr->asParts[COMP_BODY])->size <= iCapacity))
		{
			pList.push_back(psCurr);
		}
		else if (bMultiPlayer && (iCapacity == SIZE_HEAVY))
		{
			// Special case for Super heavy bodyies (Super Transporter)
			if ((asBodyStats + psCurr->asParts[COMP_BODY])->size == SIZE_SUPER_HEAVY)
			{
				pList.push_back(psCurr);
			}
		}
	}

	return pList;
}

void checkPlayerBuiltHQ(const STRUCTURE *psStruct)
{
	if (selectedPlayer == psStruct->player && psStruct->pStructureType->type == REF_HQ)
	{
		playerBuiltHQ = true;
	}
}
