/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
#include "template.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/math_ext.h"
#include "lib/framework/strres.h"
#include "lib/netplay/netplay.h"
#include "cmddroiddef.h"
#include "mission.h"
#include "objects.h"
#include "droid.h"
#include "design.h"
#include "hci.h"
#include "multiplay.h"
#include "projectile.h"
#include "main.h"

// Template storage
DROID_TEMPLATE		*apsDroidTemplates[MAX_PLAYERS];


bool allowDesign = true;
bool includeRedundantDesigns = false;


static bool researchedItem(DROID_TEMPLATE *psCurr, int player, COMPONENT_TYPE partIndex, int part, bool allowZero, bool allowRedundant)
{
	if (allowZero && part <= 0)
	{
		return true;
	}
	int availability = apCompLists[player][partIndex][part];
	return availability == AVAILABLE || (allowRedundant && availability == REDUNDANT);
}

static bool researchedPart(DROID_TEMPLATE *psCurr, int player, COMPONENT_TYPE partIndex, bool allowZero, bool allowRedundant)
{
	return researchedItem(psCurr, player, partIndex, psCurr->asParts[partIndex], allowZero, allowRedundant);
}

static bool researchedWeap(DROID_TEMPLATE *psCurr, int player, int weapIndex, bool allowRedundant)
{
	int availability = apCompLists[player][COMP_WEAPON][psCurr->asWeaps[weapIndex]];
	return availability == AVAILABLE || (allowRedundant && availability == REDUNDANT);
}

bool researchedTemplate(DROID_TEMPLATE *psCurr, int player, bool allowRedundant, bool verbose)
{
	ASSERT_OR_RETURN(false, psCurr, "Given a null template");
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
		debug(LOG_ERROR, "%s : not researched : body=%d brai=%d prop=%d sensor=%d ecm=%d rep=%d con=%d", getName(psCurr),
		      (int)resBody, (int)resBrain, (int)resProp, (int)resSensor, (int)resEcm, (int)resRepair, (int)resConstruct);
	}
	for (unsigned weapIndex = 0; weapIndex < psCurr->numWeaps && researchedEverything; ++weapIndex)
	{
		researchedEverything = researchedWeap(psCurr, player, weapIndex, allowRedundant);
		if (!researchedEverything && verbose)
		{
			debug(LOG_ERROR, "%s : not researched weapon %u", getName(psCurr), weapIndex);
		}
	}
	return researchedEverything;
}

static DROID_TEMPLATE loadTemplateCommon(WzConfig &ini)
{
	DROID_TEMPLATE design;
	QString droidType = ini.value("type").toString();

	if (droidType == "PERSON") design.droidType = DROID_PERSON;
	else if (droidType == "CYBORG") design.droidType = DROID_CYBORG;
	else if (droidType == "CYBORG_SUPER") design.droidType = DROID_CYBORG_SUPER;
	else if (droidType == "CYBORG_CONSTRUCT") design.droidType = DROID_CYBORG_CONSTRUCT;
	else if (droidType == "CYBORG_REPAIR") design.droidType = DROID_CYBORG_REPAIR;
	else if (droidType == "TRANSPORTER") design.droidType = DROID_TRANSPORTER;
	else if (droidType == "SUPERTRANSPORTER") design.droidType = DROID_SUPERTRANSPORTER;
	else if (droidType == "DROID") design.droidType = DROID_DEFAULT;
	else ASSERT(false, "No such droid type \"%s\" for %s", droidType.toUtf8().constData(), getID(&design));

	design.asParts[COMP_BODY] = getCompFromName(COMP_BODY, ini.value("body").toString());
	design.asParts[COMP_BRAIN] = getCompFromName(COMP_BRAIN, ini.value("brain", QString("ZNULLBRAIN")).toString());
	design.asParts[COMP_PROPULSION] = getCompFromName(COMP_PROPULSION, ini.value("propulsion", QString("ZNULLPROP")).toString());
	design.asParts[COMP_REPAIRUNIT] = getCompFromName(COMP_REPAIRUNIT, ini.value("repair", QString("ZNULLREPAIR")).toString());
	design.asParts[COMP_ECM] = getCompFromName(COMP_ECM, ini.value("ecm", QString("ZNULLECM")).toString());
	design.asParts[COMP_SENSOR] = getCompFromName(COMP_SENSOR, ini.value("sensor", QString("ZNULLSENSOR")).toString());
	design.asParts[COMP_CONSTRUCT] = getCompFromName(COMP_CONSTRUCT, ini.value("construct", QString("ZNULLCONSTRUCT")).toString());
	QStringList weapons = ini.value("weapons").toStringList();
	design.numWeaps = weapons.size();
	design.asWeaps[0] = getCompFromName(COMP_WEAPON, weapons.value(0, QString("ZNULLWEAPON")));
	design.asWeaps[1] = getCompFromName(COMP_WEAPON, weapons.value(1, QString("ZNULLWEAPON")));
	design.asWeaps[2] = getCompFromName(COMP_WEAPON, weapons.value(2, QString("ZNULLWEAPON")));

	return design;
}

bool initTemplates()
{
	WzConfig ini("userdata/" + QString(rulesettag) + "/templates.json", WzConfig::ReadOnly);
	if (!ini.status())
	{
		debug(LOG_WZ, "Could not open %s", ini.fileName().toUtf8().constData());
		return false;
	}
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		DROID_TEMPLATE design = loadTemplateCommon(ini);
		design.multiPlayerID = generateNewObjectId();
		design.prefab = false;		// not AI template
		design.stored = true;
		if (!(asBodyStats + design.asParts[COMP_BODY])->designable
		    || !(asPropulsionStats + design.asParts[COMP_PROPULSION])->designable
		    || (design.asParts[COMP_BRAIN] > 0 && !(asBrainStats + design.asParts[COMP_BRAIN])->designable)
		    || (design.asParts[COMP_REPAIRUNIT] > 0 && !(asRepairStats + design.asParts[COMP_REPAIRUNIT])->designable)
		    || (design.asParts[COMP_ECM] > 0 && !(asECMStats + design.asParts[COMP_ECM])->designable)
		    || (design.asParts[COMP_SENSOR] > 0 && !(asSensorStats + design.asParts[COMP_SENSOR])->designable)
		    || (design.asParts[COMP_CONSTRUCT] > 0 && !(asConstructStats + design.asParts[COMP_CONSTRUCT])->designable)
		    || (design.numWeaps > 0 && !(asWeaponStats + design.asWeaps[0])->designable)
		    || (design.numWeaps > 1 && !(asWeaponStats + design.asWeaps[1])->designable)
		    || (design.numWeaps > 2 && !(asWeaponStats + design.asWeaps[2])->designable))
		{
			debug(LOG_ERROR, "Template %d / %s from stored templates cannot be designed", i, list[i].toUtf8().constData());
			continue;
		}
		bool valid = intValidTemplate(&design, ini.value("name").toString().toUtf8().constData(), false, selectedPlayer);
		if (!valid)
		{
			debug(LOG_ERROR, "Invalid template %d / %s from stored templates", i, list[i].toUtf8().constData());
			continue;
		}
		DROID_TEMPLATE *psDestTemplate = apsDroidTemplates[selectedPlayer];
		while (psDestTemplate != NULL)
		{
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
			psDestTemplate = psDestTemplate->psNext;
		}
		if (psDestTemplate)
		{
			psDestTemplate->stored = true; // assimilate it
			ini.endGroup();
			continue; // next!
		}
		design.enabled = allowDesign;
		addTemplateToList(&design, &apsDroidTemplates[selectedPlayer]);
		sendTemplate(selectedPlayer, &design);
		localTemplates.push_back(design);
		ini.endGroup();
	}
	return true;
}

bool storeTemplates()
{
	// Write stored templates (back) to file
	WzConfig ini("userdata/" + QString(rulesettag) + "/templates.json");
	if (!ini.status() || !ini.isWritable())
	{
		debug(LOG_ERROR, "Could not open %s", ini.fileName().toUtf8().constData());
		return false;
	}
	for (DROID_TEMPLATE *psCurr = apsDroidTemplates[selectedPlayer]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (!psCurr->stored) continue; // not stored
		ini.beginGroup("template_" + QString::number(psCurr->multiPlayerID));
		ini.setValue("name", psCurr->name);
		switch (psCurr->droidType)
		{
		case DROID_PERSON: ini.setValue("type", "PERSON"); break;
		case DROID_CYBORG: ini.setValue("type", "CYBORG"); break;
		case DROID_CYBORG_SUPER: ini.setValue("type", "CYBORG_SUPER"); break;
		case DROID_CYBORG_CONSTRUCT: ini.setValue("type", "CYBORG_CONSTRUCT"); break;
		case DROID_CYBORG_REPAIR: ini.setValue("type", "CYBORG_REPAIR"); break;
		case DROID_TRANSPORTER: ini.setValue("type", "TRANSPORTER"); break;
		case DROID_SUPERTRANSPORTER: ini.setValue("type", "SUPERTRANSPORTER"); break;
		case DROID_DEFAULT: ini.setValue("type", "DROID"); break;
		default: ASSERT(false, "No such droid type \"%d\" for %s", psCurr->droidType, psCurr->name.toUtf8().constData());
		}
		ini.setValue("body", (asBodyStats + psCurr->asParts[COMP_BODY])->id);
		ini.setValue("propulsion", (asPropulsionStats + psCurr->asParts[COMP_PROPULSION])->id);
		if (psCurr->asParts[COMP_BRAIN] != 0)
		{
			ini.setValue("brain", (asBrainStats + psCurr->asParts[COMP_BRAIN])->id);
		}
		if ((asRepairStats + psCurr->asParts[COMP_REPAIRUNIT])->location == LOC_TURRET) // avoid auto-repair...
		{
			ini.setValue("repair", (asRepairStats + psCurr->asParts[COMP_REPAIRUNIT])->id);
		}
		if ((asECMStats + psCurr->asParts[COMP_ECM])->location == LOC_TURRET)
		{
			ini.setValue("ecm", (asECMStats + psCurr->asParts[COMP_ECM])->id);
		}
		if ((asSensorStats + psCurr->asParts[COMP_SENSOR])->location == LOC_TURRET)
		{
			ini.setValue("sensor", (asSensorStats + psCurr->asParts[COMP_SENSOR])->id);
		}
		if (psCurr->asParts[COMP_CONSTRUCT] != 0)
		{
			ini.setValue("construct", (asConstructStats + psCurr->asParts[COMP_CONSTRUCT])->id);
		}
		ini.setValue("weapons", psCurr->numWeaps);
		for (int j = 0; j < psCurr->numWeaps; j++)
		{
			ini.setValue("weapon/" + QString::number(j + 1), (asWeaponStats + psCurr->asWeaps[j])->id);
		}
		ini.endGroup();
	}
	return true;
}

bool shutdownTemplates()
{
	return storeTemplates();
}

DROID_TEMPLATE::DROID_TEMPLATE()  // This constructor replaces a memset in scrAssembleWeaponTemplate(), not needed elsewhere.
	: BASE_STATS(REF_TEMPLATE_START)
	//, asParts
	, numWeaps(0)
	//, asWeaps
	, droidType(DROID_WEAPON)
	, multiPlayerID(0)
	, psNext(NULL)
	, prefab(false)
	, stored(false)
	, enabled(false)
{
	std::fill_n(asParts, DROID_MAXCOMP, 0);
	std::fill_n(asWeaps, DROID_MAXWEAPS, 0);
}

bool loadDroidTemplates(const char *filename)
{
	WzConfig ini(filename, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		DROID_TEMPLATE design = loadTemplateCommon(ini);
		design.id = list[i];
		design.name = ini.value("name").toString();
		design.multiPlayerID = generateNewObjectId();
		design.prefab = true;
		design.stored = false;
		design.enabled = true;
		bool available = ini.value("available", false).toBool();
		char const *droidResourceName = getDroidResourceName(list[i].toUtf8().constData());
		design.name = droidResourceName != NULL? droidResourceName : GetDefaultTemplateName(&design);
		ini.endGroup();

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			// Give those meant for humans to all human players.
			if (NetPlay.players[i].allocated && available)
			{
				design.prefab = false;
				addTemplateToList(&design, &apsDroidTemplates[i]);

				// This sets up the UI templates for display purposes ONLY--we still only use apsDroidTemplates for making them.
				// FIXME: Why are we doing this here, and not on demand ?
				// Only add unique designs to the UI list (Note, perhaps better to use std::map instead?)
				std::list<DROID_TEMPLATE>::iterator it;
				for (it = localTemplates.begin(); it != localTemplates.end(); ++it)
				{
					DROID_TEMPLATE *psCurr = &*it;
					if (psCurr->multiPlayerID == design.multiPlayerID)
					{
						debug(LOG_ERROR, "Design id:%d (%s) *NOT* added to UI list (duplicate), player= %d", design.multiPlayerID, getName(&design), i);
						break;
					}
				}
				if (it == localTemplates.end())
				{
					debug(LOG_NEVER, "Design id:%d (%s) added to UI list, player =%d", design.multiPlayerID, getName(&design), i);
					localTemplates.push_front(design);
				}
			}
			else if (!NetPlay.players[i].allocated)	// AI template
			{
				design.prefab = true;  // prefabricated templates referenced from VLOs
				addTemplateToList(&design, &apsDroidTemplates[i]);
			}
		}
		debug(LOG_NEVER, "Droid template found, Name: %s, MP ID: %d, ref: %u, ID: %s, prefab: %s, type:%d (loading)",
		      getName(&design), design.multiPlayerID, design.ref, getID(&design), design.prefab ? "yes":"no", design.droidType);
	}

	return true;
}

//free the storage for the droid templates
bool droidTemplateShutDown(void)
{
	unsigned int player;
	DROID_TEMPLATE *pTemplate, *pNext;

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for (pTemplate = apsDroidTemplates[player]; pTemplate != NULL; pTemplate = pNext)
		{
			pNext = pTemplate->psNext;
			delete pTemplate;
		}
		apsDroidTemplates[player] = NULL;
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
DROID_TEMPLATE *getTemplateFromTranslatedNameNoPlayer(char const *pName)
{
	for (int i=0; i < MAX_PLAYERS; i++)
	{
		for (DROID_TEMPLATE *psCurr = apsDroidTemplates[i]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			if (psCurr->id.compare(pName) == 0)
			{
				return psCurr;
			}
		}
	}
	return NULL;
}

/*getTemplatefFromMultiPlayerID gets template for unique ID  searching all lists */
DROID_TEMPLATE* getTemplateFromMultiPlayerID(UDWORD multiPlayerID)
{
	UDWORD		player;
	DROID_TEMPLATE	*pDroidDesign;

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for(pDroidDesign = apsDroidTemplates[player]; pDroidDesign != NULL; pDroidDesign = pDroidDesign->psNext)
		{
			if (pDroidDesign->multiPlayerID == multiPlayerID)
			{
				return pDroidDesign;
			}
		}
	}
	return NULL;
}

/*called when a Template is deleted in the Design screen*/
void deleteTemplateFromProduction(DROID_TEMPLATE *psTemplate, unsigned player, QUEUE_MODE mode)
{
	STRUCTURE   *psStruct;
	STRUCTURE	*psList;

	//see if any factory is currently using the template
	for (unsigned i = 0; i < 2; ++i)
	{
		psList = NULL;
		switch (i)
		{
		case 0:
			psList = apsStructLists[player];
			break;
		case 1:
			psList = mission.apsStructLists[player];
			break;
		}
		for (psStruct = psList; psStruct != NULL; psStruct = psStruct->psNext)
		{
			if (StructIsFactory(psStruct))
			{
				FACTORY             *psFactory = &psStruct->pFunctionality->factory;

				if (psFactory->psAssemblyPoint->factoryInc < asProductionRun[psFactory->psAssemblyPoint->factoryType].size())
				{
					ProductionRun &productionRun = asProductionRun[psFactory->psAssemblyPoint->factoryType][psFactory->psAssemblyPoint->factoryInc];
					for (unsigned inc = 0; inc < productionRun.size(); ++inc)
					{
						if (productionRun[inc].psTemplate->multiPlayerID == psTemplate->multiPlayerID && mode == ModeQueue)
						{
							//just need to erase this production run entry
							productionRun.erase(productionRun.begin() + inc);
							--inc;
						}
					}
				}

				if (psFactory->psSubject == NULL)
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
					doNextProduction(psStruct, NULL, ModeImmediate);

					//tell the interface
					intManufactureFinished(psStruct);

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
	if (!(psTemplate->droidType == DROID_WEAPON || psTemplate->droidType == DROID_CYBORG ||
		psTemplate->droidType == DROID_CYBORG_SUPER))
	{
		return false;
	}

	if (proj_Direct(psTemplate->asWeaps[0] + asWeaponStats))
	{
		return false;
	}

	return true;
}

/*
fills the list with Templates that can be manufactured
in the Factory - based on size. There is a limit on how many can be manufactured
at any one time. Pass back the number available.
*/
void fillTemplateList(std::vector<DROID_TEMPLATE *> &pList, STRUCTURE *psFactory)
{
	const int player = psFactory->player;
	pList.clear();

	DROID_TEMPLATE	*psCurr;
	BODY_SIZE	iCapacity = (BODY_SIZE)psFactory->capacity;

	/* Add the templates to the list*/
	for (std::list<DROID_TEMPLATE>::iterator i = localTemplates.begin(); i != localTemplates.end(); ++i)
	{
		psCurr = &*i;
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

			if (!psCurr->enabled || !validTemplateForFactory(psCurr, psFactory, false)
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
			if((asBodyStats + psCurr->asParts[COMP_BODY])->size == SIZE_SUPER_HEAVY)
			{
				pList.push_back(psCurr);
			}
		}
	}
}
