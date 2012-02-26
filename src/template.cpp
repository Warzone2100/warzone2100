/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

/* default droid design template */
extern DROID_TEMPLATE	sDefaultDesignTemplate;

// Template storage
DROID_TEMPLATE		*apsDroidTemplates[MAX_PLAYERS];
DROID_TEMPLATE		*apsStaticTemplates;	// for AIs and scripts

bool allowDesign = true;

static const StringToEnum<DROID_TYPE> map_DROID_TYPE[] =
{
	{"PERSON",              DROID_PERSON            },
	{"CYBORG",              DROID_CYBORG            },
	{"CYBORG_SUPER",        DROID_CYBORG_SUPER      },
	{"CYBORG_CONSTRUCT",    DROID_CYBORG_CONSTRUCT  },
	{"CYBORG_REPAIR",       DROID_CYBORG_REPAIR     },
	{"TRANSPORTER",         DROID_TRANSPORTER       },
	{"SUPERTRANSPORTER",    DROID_SUPERTRANSPORTER  },
	{"ZNULLDROID",          DROID_ANY               },
	{"DROID",               DROID_DEFAULT           },
};

bool researchedTemplate(DROID_TEMPLATE *psCurr, int player)
{
	// super hack -- cyborgs and transports are special, only check their body
	switch (psCurr->droidType)
	{
	case DROID_PERSON:
	case DROID_CYBORG:
	case DROID_CYBORG_SUPER:
	case DROID_CYBORG_CONSTRUCT:
	case DROID_CYBORG_REPAIR:
	case DROID_TRANSPORTER:
	case DROID_SUPERTRANSPORTER:
		return (apCompLists[player][COMP_BODY][psCurr->asParts[COMP_BODY]] == AVAILABLE);
	default:
		break; // now proceed to normal droids...
	}
	// Note the ugly special case for commanders - their weapon is unavailable
	if (apCompLists[player][COMP_BODY][psCurr->asParts[COMP_BODY]] != AVAILABLE
	    || (psCurr->asParts[COMP_BRAIN] > 0 && apCompLists[player][COMP_BRAIN][psCurr->asParts[COMP_BRAIN]] != AVAILABLE)
	    || apCompLists[player][COMP_PROPULSION][psCurr->asParts[COMP_PROPULSION]] != AVAILABLE
	    || (psCurr->asParts[COMP_SENSOR] > 0 && apCompLists[player][COMP_SENSOR][psCurr->asParts[COMP_SENSOR]] != AVAILABLE)
	    || (psCurr->asParts[COMP_ECM] > 0 && apCompLists[player][COMP_ECM][psCurr->asParts[COMP_ECM]] != AVAILABLE)
	    || (psCurr->asParts[COMP_REPAIRUNIT] > 0 && apCompLists[player][COMP_REPAIRUNIT][psCurr->asParts[COMP_REPAIRUNIT]] != AVAILABLE)
	    || (psCurr->asParts[COMP_CONSTRUCT] > 0 && apCompLists[player][COMP_CONSTRUCT][psCurr->asParts[COMP_CONSTRUCT]] != AVAILABLE)
	    || (psCurr->asParts[COMP_BRAIN] == 0 && psCurr->numWeaps > 0 && apCompLists[player][COMP_WEAPON][psCurr->asWeaps[0]] != AVAILABLE)
	    || (psCurr->numWeaps > 1 && apCompLists[player][COMP_WEAPON][psCurr->asWeaps[1]] != AVAILABLE))
	{
		return false;
	}
	return true;
}

bool initTemplates()
{
	WzConfig ini("templates.ini");
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_FATAL, "Could not open templates.ini");
		return false;
	}
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		DROID_TEMPLATE design;
		design.pName = NULL;
		design.droidType = (DROID_TYPE)ini.value("droidType").toInt();
		design.multiPlayerID = generateNewObjectId();
		design.asParts[COMP_BODY] = getCompFromName(COMP_BODY, ini.value("body", QString("ZNULLBODY")).toString().toUtf8().constData());
		design.asParts[COMP_BRAIN] = getCompFromName(COMP_BRAIN, ini.value("brain", QString("ZNULLBRAIN")).toString().toUtf8().constData());
		design.asParts[COMP_PROPULSION] = getCompFromName(COMP_PROPULSION, ini.value("propulsion", QString("ZNULLPROP")).toString().toUtf8().constData());
		design.asParts[COMP_REPAIRUNIT] = getCompFromName(COMP_REPAIRUNIT, ini.value("repair", QString("ZNULLREPAIR")).toString().toUtf8().constData());
		design.asParts[COMP_ECM] = getCompFromName(COMP_ECM, ini.value("ecm", QString("ZNULLECM")).toString().toUtf8().constData());
		design.asParts[COMP_SENSOR] = getCompFromName(COMP_SENSOR, ini.value("sensor", QString("ZNULLSENSOR")).toString().toUtf8().constData());
		design.asParts[COMP_CONSTRUCT] = getCompFromName(COMP_CONSTRUCT, ini.value("construct", QString("ZNULLCONSTRUCT")).toString().toUtf8().constData());
		design.asWeaps[0] = getCompFromName(COMP_WEAPON, ini.value("weapon/1", QString("ZNULLWEAPON")).toString().toUtf8().constData());
		design.asWeaps[1] = getCompFromName(COMP_WEAPON, ini.value("weapon/2", QString("ZNULLWEAPON")).toString().toUtf8().constData());
		design.asWeaps[2] = getCompFromName(COMP_WEAPON, ini.value("weapon/3", QString("ZNULLWEAPON")).toString().toUtf8().constData());
		design.numWeaps = ini.value("weapons").toInt();
		design.prefab = false;		// not AI template
		design.stored = true;
		bool valid = intValidTemplate(&design, ini.value("name").toString().toUtf8().constData());
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
			    && strcmp(psDestTemplate->aName, design.aName) == 0
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
	WzConfig ini("templates.ini");
	if (ini.status() != QSettings::NoError || !ini.isWritable())
	{
		debug(LOG_FATAL, "Could not open templates.ini");
		return false;
	}
	for (DROID_TEMPLATE *psCurr = apsDroidTemplates[selectedPlayer]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (!psCurr->stored) continue; // not stored
		ini.beginGroup("template_" + QString::number(psCurr->multiPlayerID));
		ini.setValue("name", psCurr->aName);
		ini.setValue("droidType", psCurr->droidType);
		ini.setValue("body", (asBodyStats + psCurr->asParts[COMP_BODY])->pName);
		ini.setValue("propulsion", (asPropulsionStats + psCurr->asParts[COMP_PROPULSION])->pName);
		if (psCurr->asParts[COMP_BRAIN] != 0)
		{
			ini.setValue("brain", (asBrainStats + psCurr->asParts[COMP_BRAIN])->pName);
		}
		if ((asRepairStats + psCurr->asParts[COMP_REPAIRUNIT])->location == LOC_TURRET) // avoid auto-repair...
		{
			ini.setValue("repair", (asRepairStats + psCurr->asParts[COMP_REPAIRUNIT])->pName);
		}
		if ((asECMStats + psCurr->asParts[COMP_ECM])->location == LOC_TURRET)
		{
			ini.setValue("ecm", (asECMStats + psCurr->asParts[COMP_ECM])->pName);
		}
		if ((asSensorStats + psCurr->asParts[COMP_SENSOR])->location == LOC_TURRET)
		{
			ini.setValue("sensor", (asSensorStats + psCurr->asParts[COMP_SENSOR])->pName);
		}
		if (psCurr->asParts[COMP_CONSTRUCT] != 0)
		{
			ini.setValue("construct", (asConstructStats + psCurr->asParts[COMP_CONSTRUCT])->pName);
		}
		ini.setValue("weapons", psCurr->numWeaps);
		for (int j = 0; j < psCurr->numWeaps; j++)
		{
			ini.setValue("weapon/" + QString::number(j + 1), (asWeaponStats + psCurr->asWeaps[j])->pName);
		}
		ini.endGroup();
	}
	return true;
}

bool shutdownTemplates()
{
	return storeTemplates();
}

static void initTemplatePoints(DROID_TEMPLATE *pDroidDesign)
{
	//calculate the total build points
	pDroidDesign->buildPoints = calcTemplateBuild(pDroidDesign);
	//calc the total power points
	pDroidDesign->powerPoints = calcTemplatePower(pDroidDesign);
}

/*initialise the template build and power points */
void initTemplatePoints(void)
{
	for (int player = 0; player < MAX_PLAYERS; ++player)
	{
		for (DROID_TEMPLATE *pDroidDesign = apsDroidTemplates[player]; pDroidDesign != NULL; pDroidDesign = pDroidDesign->psNext)
		{
			initTemplatePoints(pDroidDesign);
		}
	}
	for (DROID_TEMPLATE *pDroidDesign = apsStaticTemplates; pDroidDesign != NULL; pDroidDesign = pDroidDesign->psNext)
	{
		initTemplatePoints(pDroidDesign);
	}
	for (std::list<DROID_TEMPLATE>::iterator pDroidDesign = localTemplates.begin(); pDroidDesign != localTemplates.end(); ++pDroidDesign)
	{
		initTemplatePoints(&*pDroidDesign);
	}
}

DROID_TEMPLATE::DROID_TEMPLATE()  // This constructor replaces a memset in scrAssembleWeaponTemplate(), not needed elsewhere.
	: BASE_STATS()
	//, aName
	//, asParts
	, buildPoints(0)
	, powerPoints(0)
	, storeCount(0)
	, numWeaps(0)
	//, asWeaps
	, droidType(DROID_WEAPON)
	, multiPlayerID(0)
	, psNext(NULL)
	, prefab(false)
	, stored(false)
{
	aName[0] = '\0';
	std::fill_n(asParts, DROID_MAXCOMP, 0);
	std::fill_n(asWeaps, DROID_MAXWEAPS, 0);
}

DROID_TEMPLATE::DROID_TEMPLATE(LineView line)
	: BASE_STATS(REF_TEMPLATE_START + line.line())
	//, aName
	//, asParts
	, buildPoints(0)
	, powerPoints(0)
	, storeCount(0)
	, numWeaps(line.i(11, 0, DROID_MAXWEAPS))
	//, asWeaps
	, droidType(line.e(9, map_DROID_TYPE))
	, multiPlayerID(line.u32(1))
	, psNext(NULL)
	, prefab(false)
	, stored(false)
	, enabled(true)
	// Ignored columns: 6 - but used later to decide whether the template is for human players.
{
	std::string name = line.s(0);
	sstrcpy(aName, name.c_str());

	asParts[COMP_UNKNOWN]    = 0;  // Is this one useful for anything at all?
	asParts[COMP_BODY]       = line.stats( 2, asBodyStats,       numBodyStats)       - asBodyStats;
	asParts[COMP_BRAIN]      = line.stats( 3, asBrainStats,      numBrainStats)      - asBrainStats;
	asParts[COMP_CONSTRUCT]  = line.stats( 4, asConstructStats,  numConstructStats)  - asConstructStats;
	asParts[COMP_ECM]        = line.stats( 5, asECMStats,        numECMStats)        - asECMStats;
	asParts[COMP_PROPULSION] = line.stats( 7, asPropulsionStats, numPropulsionStats) - asPropulsionStats;
	asParts[COMP_REPAIRUNIT] = line.stats( 8, asRepairStats,     numRepairStats)     - asRepairStats;
	asParts[COMP_SENSOR]     = line.stats(10, asSensorStats,     numSensorStats)     - asSensorStats;

	std::fill_n(asWeaps, DROID_MAXWEAPS, 0);
}

/* load the Droid stats for the components from the Access database */
bool loadDroidTemplates(const char *pDroidData, UDWORD bufferSize)
{
	bool bDefaultTemplateFound = false;

	TableView table(pDroidData, bufferSize);

	for (unsigned i = 0; i < table.size(); ++i)
	{
		LineView line(table, i);

		DROID_TEMPLATE design(line);
		if (table.isError())
		{
			debug(LOG_ERROR, "%s", table.getError().toUtf8().constData());
			return false;
		}

		std::string const pNameCache = design.aName;
		design.pName = const_cast<char *>(pNameCache.c_str());

		if (getTemplateFromUniqueName(design.pName, 0))
		{
			debug(LOG_ERROR, "Duplicate template %s", design.pName);
			continue;
		}

		// Store translated name in aName
		char const *droidResourceName = getDroidResourceName(design.aName);
		sstrcpy(design.aName, droidResourceName != NULL? droidResourceName : GetDefaultTemplateName(&design));

		// Store global default design if found else store in the appropriate array
		if (design.droidType == DROID_ANY)
		{
			design.droidType = DROID_DEFAULT;
			// NOTE: sDefaultDesignTemplate.pName takes ownership
			//       of the memory allocated to pDroidDesign->pName
			//       here. Which is good because pDroidDesign leaves
			//       scope here anyway.
			sDefaultDesignTemplate = design;
			sDefaultDesignTemplate.pName = strdup(design.pName);
			bDefaultTemplateFound = true;
		}
		else
		{
			std::string playerType = line.s(6);
			// Give those meant for humans to all human players.
			// Also support the old template format, in which those meant
			// for humans were player 0 (in campaign) or 5 (in multiplayer).
			if ((!bMultiPlayer && playerType == "0") ||
			    ( bMultiPlayer && playerType == "5") ||
					      playerType == "YES"
			   )
			{
				for (int i = 0; i < MAX_PLAYERS; ++i)
				{
					if (NetPlay.players[i].allocated)  // human prototype template
					{
						design.prefab = false;
						addTemplateToList(&design, &apsDroidTemplates[i]);
					}
				}
				localTemplates.push_front(design);
				localTemplates.front().pName = strdup(localTemplates.front().pName);
			}
			// Add all templates to static template list
			design.prefab = true;  // prefabricated templates referenced from VLOs
			addTemplateToList(&design, &apsStaticTemplates);
		}

		debug(LOG_NEVER, "(default) Droid template found, aName: %s, MP ID: %d, ref: %u, pname: %s, prefab: %s, type:%d (loading)",
			design.aName, design.multiPlayerID, design.ref, design.pName, design.prefab ? "yes":"no", design.droidType);
	}

	ASSERT_OR_RETURN(false, bDefaultTemplateFound, "Default template not found");

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
			if (pTemplate->pName != sDefaultDesignTemplate.pName)	// sanity check probably no longer necessary
			{
				free(pTemplate->pName);
			}
			ASSERT(!pTemplate->prefab, "Static template %s in player template list!", pTemplate->aName);
			delete pTemplate;
		}
		apsDroidTemplates[player] = NULL;
	}
	for (pTemplate = apsStaticTemplates; pTemplate != NULL; pTemplate = pNext)
	{
		pNext = pTemplate->psNext;
		if (pTemplate->pName != sDefaultDesignTemplate.pName)		// sanity check probably no longer necessary
		{
			free(pTemplate->pName);
		}
		ASSERT(pTemplate->prefab, "Player template %s in static template list!", pTemplate->aName);
		delete pTemplate;
	}
	apsStaticTemplates = NULL;
	free(sDefaultDesignTemplate.pName);
	sDefaultDesignTemplate.pName = NULL;

	for (std::list<DROID_TEMPLATE>::iterator i = localTemplates.begin(); i != localTemplates.end(); ++i)
	{
		free(i->pName);
	}
	localTemplates.clear();

	return true;
}

/*!
 * Get a static template from its aName.
 * This checks the all the Human's apsDroidTemplates list.
 * This function is similar to getTemplateFromUniqueName() but we use aName,
 * and not pName, since we don't have that information, and we are checking all player's list.
 * THIS FUNCTION WILL GO AWAY! Only used (badly) in "clone" cheat currently.
 * \param aName Template aName
 *
 */
DROID_TEMPLATE	*GetHumanDroidTemplate(const char *aName)
{
	DROID_TEMPLATE	*templatelist, *found = NULL, *foundOtherPlayer = NULL;
	int i, playerFound = 0;

	for (i=0; i < MAX_PLAYERS; i++)
	{
		templatelist = apsDroidTemplates[i];
		while (templatelist)
		{
			if (!strcmp(templatelist->aName, aName))
			{
				debug(LOG_NEVER, "Droid template found, aName: %s, MP ID: %d, ref: %u, pname: %s (for player %d)",
					templatelist->aName, templatelist->multiPlayerID, templatelist->ref, templatelist->pName, i);
				if (i == selectedPlayer)
				{
					found = templatelist;
				}
				else
				{
					foundOtherPlayer = templatelist;
					playerFound = i;
				}
			}

			templatelist = templatelist->psNext;
		}
	}

	if (foundOtherPlayer && !found)
	{
		debug(LOG_ERROR, "The template was not in our list, but was in another players list.");
		debug(LOG_ERROR, "Droid template's aName: %s, MP ID: %d, ref: %u, pname: %s (for player %d)",
			foundOtherPlayer->aName, foundOtherPlayer->multiPlayerID, foundOtherPlayer->ref, foundOtherPlayer->pName, playerFound);
		return foundOtherPlayer;
	}

	return found;
}

/*!
 * Get a static template from its aName.
 * This checks the AI apsStaticTemplates.
 * This function is similar to getTemplateFromTranslatedNameNoPlayer() but we use aName,
 * and not pName, since we don't have that information.
 * \param aName Template aName
 *
 */
DROID_TEMPLATE *GetAIDroidTemplate(const char *aName)
{
	DROID_TEMPLATE	*templatelist, *found = NULL;

	templatelist = apsStaticTemplates;
	while (templatelist)
	{
		if (!strcmp(templatelist->aName, aName))
		{
			debug(LOG_INFO, "Droid template found, name: %s, MP ID: %d, ref: %u, pname: %s ",
				templatelist->aName, templatelist->multiPlayerID, templatelist->ref, templatelist->pName);

			found = templatelist;
		}
		templatelist = templatelist->psNext;
	}

	return found;
}

/*!
 * Gets a template from its name
 * relies on the name being unique (or it will return the first one it finds!)
 * \param pName Template name
 * \param player Player number
 * \pre pName has to be the unique, untranslated name!
 * \pre player \< MAX_PLAYERS
 */
DROID_TEMPLATE * getTemplateFromUniqueName(const char *pName, unsigned int player)
{
	DROID_TEMPLATE *psCurr;
	DROID_TEMPLATE *list = apsStaticTemplates;	// assume AI

	if (isHumanPlayer(player))
	{
		list = apsDroidTemplates[player];	// was human
	}

	for (psCurr = list; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (strcmp(psCurr->pName, pName) == 0)
		{
			return psCurr;
		}
	}

	return NULL;
}

/*!
 * Get a static template from its name. This is used from scripts. These templates must
 * never be changed or deleted.
 * \param pName Template name
 * \pre pName has to be the unique, untranslated name!
 */
DROID_TEMPLATE *getTemplateFromTranslatedNameNoPlayer(char const *pName)
{
	const char *rName;
	DROID_TEMPLATE *psCurr;

	for (psCurr = apsStaticTemplates; psCurr != NULL; psCurr = psCurr->psNext)
	{
		rName = psCurr->pName ? psCurr->pName : psCurr->aName;
		if (strcmp(rName, pName) == 0)
		{
			return psCurr;
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

const char* getTemplateName(const DROID_TEMPLATE *psTemplate)
{
	return psTemplate->aName;
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

					// Clear the factory's subject.
					psFactory->psSubject = NULL;

					if (player == productionPlayer)
					{
						//check to see if anything left to produce
						DROID_TEMPLATE *psNextTemplate = factoryProdUpdate(psStruct, NULL);
						//power is returned by factoryProdAdjust()
						if (psNextTemplate)
						{
							structSetManufacture(psStruct, psNextTemplate, ModeQueue);  // ModeQueue because production lists aren't synchronised.
						}
					}

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
	UDWORD			iCapacity = psFactory->pFunctionality->factory.capacity;

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

			if (!psCurr->enabled || !validTemplateForFactory(psCurr, psFactory) || !researchedTemplate(psCurr, player))
			{
				continue;
			}
		}

		//check the factory can cope with this sized body
		if (!((asBodyStats + psCurr->asParts[COMP_BODY])->size > iCapacity) )
		{
			pList.push_back(psCurr);
		}
	}
}
