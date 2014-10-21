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
/*
 * Research.c
 *
 * Research tree and functions!
 *
 */
#include <string.h>
#include <map>
#include <QtCore/QJsonArray>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/wzconfig.h"
#include "objects.h"
#include "lib/gamelib/gtime.h"
#include "research.h"
#include "message.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "hci.h"
#include "console.h"
#include "cmddroid.h"
#include "power.h"
#include "mission.h"
#include "frend.h"		// frontend ids.
#include "intimage.h"
#include "multiplay.h"
#include "template.h"
#include "qtscript.h"
#include "stats.h"

// The stores for the research stats
std::vector<RESEARCH> asResearch;

//used for Callbacks to say which topic was last researched
RESEARCH                *psCBLastResearch;
STRUCTURE				*psCBLastResStructure;
SDWORD					CBResFacilityOwner;

//List of pointers to arrays of PLAYER_RESEARCH[numResearch] for each player
std::vector<PLAYER_RESEARCH> asPlayerResList[MAX_PLAYERS];

/* Default level of sensor, Repair and ECM */
UDWORD					aDefaultSensor[MAX_PLAYERS];
UDWORD					aDefaultECM[MAX_PLAYERS];
UDWORD					aDefaultRepair[MAX_PLAYERS];

//set the iconID based on the name read in in the stats
static UWORD setIconID(char *pIconName, const char *pName);
static void replaceComponent(COMPONENT_STATS *pNewComponent, COMPONENT_STATS *pOldComponent,
					  UBYTE player);
static bool checkResearchName(RESEARCH *psRes, UDWORD numStats);

//flag that indicates whether the player can self repair
static UBYTE bSelfRepair[MAX_PLAYERS];
static void replaceDroidComponent(DROID *pList, UDWORD oldType, UDWORD oldCompInc,
                      UDWORD newCompInc);
static void replaceStructureComponent(STRUCTURE *pList, UDWORD oldType, UDWORD oldCompInc,
                      UDWORD newCompInc, UBYTE player);
static void switchComponent(DROID *psDroid,UDWORD oldType, UDWORD oldCompInc,
                     UDWORD newCompInc);
static void replaceTransDroidComponents(DROID *psTransporter, UDWORD oldType,
                                 UDWORD oldCompInc, UDWORD newCompInc);


bool researchInitVars(void)
{
	psCBLastResearch = NULL;
	psCBLastResStructure = NULL;
	CBResFacilityOwner = -1;
	asResearch.clear();

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		bSelfRepair[i] = false;
		aDefaultSensor[i] = 0;
		aDefaultECM[i] = 0;
		aDefaultRepair[i] = 0;
	}

	return true;
}

/** Load the research stats */
bool loadResearch(QString filename)
{
	WzConfig ini(filename, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	PLAYER_RESEARCH dummy;
	memset(&dummy, 0, sizeof(dummy));
	QVector<QStringList> preResearch;
	preResearch.resize(list.size());
	for (int inc = 0; inc < list.size(); ++inc)
	{
		// HACK FIXME: the code assumes we have empty PLAYER_RESEARCH entries to throw around
		for (int j = 0; j < MAX_PLAYERS; j++)
		{
			asPlayerResList[j].push_back(dummy);
		}
		
		ini.beginGroup(list[inc]);
		RESEARCH research;
		research.index = inc;
		research.name = ini.value("name").toString();
		research.id = list[inc];

		//check the name hasn't been used already
		ASSERT_OR_RETURN(false, checkResearchName(&research, inc), "Research name '%s' used already", getName(&research));

		research.ref = REF_RESEARCH_START + inc;

		research.results = ini.json("results", QJsonArray());
		
		//set subGroup icon
		QString subGroup = ini.value("subgroupIconID", "").toString();
		if (subGroup.compare("") != 0)
		{
			research.subGroup = setIconID(subGroup.toUtf8().data(), getName(&research));
		}
		else
		{
			research.subGroup = NO_RESEARCH_ICON;
		}

		//set key topic
		unsigned int keyTopic = ini.value("keyTopic", 0).toUInt();
		ASSERT(keyTopic <= 1, "Invalid keyTopic for research topic - '%s' ", getName(&research));
		if(keyTopic <= 1)
			research.keyTopic = ini.value("keyTopic", 0).toUInt();
		else 
			research.keyTopic = 0;

		//set tech code
		UBYTE techCode = ini.value("techCode", 0).toUInt();
		ASSERT(techCode <= 1, "Invalid tech code for research topic - '%s' ", getName(&research));
		if (techCode == 0)
		{
			research.techCode = TC_MAJOR;
		}
		else
		{
			research.techCode = TC_MINOR;
		}

		//set the iconID
		QString iconID = ini.value("iconID", "").toString();
		if (iconID.compare("") != 0)
		{
			research.iconID = setIconID(iconID.toUtf8().data(), getName(&research));
		}
		else
		{
			research.iconID = NO_RESEARCH_ICON;
		}

		//get the IMDs used in the interface
		QString statID = ini.value("statID", "").toString();
		research.psStat = NULL;
		if (statID.compare("") != 0)
		{
			//try find the structure stat with given name
			research.psStat = getCompStatsFromName(statID);
			ASSERT_OR_RETURN(false, research.psStat, "Could not find stats for %s research %s", statID.toUtf8().constData(), getName(&research));
		}

		QString imdName = ini.value("imdName", "").toString();
		if (imdName.compare("") != 0)
		{
			research.pIMD = modelGet(imdName);
			ASSERT(research.pIMD != NULL, "Cannot find the research PIE '%s' for record '%s'",imdName.toUtf8().data(), getName(&research));
		}

		QString imdName2 = ini.value("imdName2", "").toString();
		if (imdName2.compare("") != 0)
		{
			research.pIMD2 = modelGet(imdName2);
			ASSERT(research.pIMD2 != NULL, "Cannot find the 2nd research '%s' PIE for record '%s'",imdName2.toUtf8().data(), getName(&research));
		}

		QString msgName = ini.value("msgName", "").toString();
		if (msgName.compare("") != 0)
		{
			//check its a major tech code
			ASSERT(research.techCode == TC_MAJOR, "This research should not have a message associated with it, '%s' the message will be ignored!", getName(&research));
			if (research.techCode == TC_MAJOR)
			{
				research.pViewData = getViewData(msgName.toUtf8().data());
			}
		}

		//set the researchPoints
		unsigned int resPoints = ini.value("researchPoints", 0).toUInt();
		ASSERT_OR_RETURN(false, resPoints <= UWORD_MAX, "Research Points too high for research topic - '%s' ", getName(&research));
		research.researchPoints = resPoints;

		//set the research power
		unsigned int resPower = ini.value("researchPower", 0).toUInt();
		ASSERT_OR_RETURN(false, resPower <= UWORD_MAX, "Research Power too high for research topic - '%s' ", getName(&research));
		research.researchPower = resPower;

		//rememeber research pre-requisites for futher checking
		preResearch[inc] = ini.value("requiredResearch").toStringList();

		//set components results
		QStringList compResults = ini.value("resultComponents").toStringList();
		for (int j = 0; j < compResults.size(); j++)
		{
			QString compID = compResults[j].trimmed();
			COMPONENT_STATS *pComp = getCompStatsFromName(compID);
			if (pComp != NULL)
			{
				research.componentResults.push_back(pComp);
			}else
			{
				ASSERT(false, "Invalid item '%s' in list of result components of research '%s' ", compID.toUtf8().constData(), getName(&research));
			}
		}

		//set replaced components
		QStringList replacedComp = ini.value("replacedComponents").toStringList();
		for (int j = 0; j < replacedComp.size(); j++)
		{
			//read pair of components oldComponent:newComponent
			QStringList pair = replacedComp[j].split(':');
			ASSERT(pair.size() == 2, "Invalid item '%s' in list of replaced components of research '%s'. Required format: 'oldItem:newItem, item1:item2'", replacedComp[j].toUtf8().constData(), getName(&research));
			if (pair.size() != 2)
			{
				continue; //skip invalid entries
			}
			QString oldCompID = pair[0].trimmed();
			QString newCompID = pair[1].trimmed();
			COMPONENT_STATS *oldComp = getCompStatsFromName(oldCompID);
			if (oldComp == NULL)
			{
				ASSERT(false, "Invalid item '%s' in list of replaced components of research '%s'. Wrong component code.", oldCompID.toUtf8().constData(), getName(&research));
				continue;
			}
			COMPONENT_STATS *newComp = getCompStatsFromName(newCompID);
			if(newComp == NULL)
			{
				ASSERT(false, "Invalid item '%s' in list of replaced components of research '%s'. Wrong component code.", newCompID.toUtf8().constData(), getName(&research));
				continue;
			}
			RES_COMP_REPLACEMENT replItem;
			replItem.pOldComponent = oldComp;
			replItem.pNewComponent = newComp;
			research.componentReplacement.push_back(replItem);
		}

		//set redundant components
		QStringList redComp = ini.value("redComponents").toStringList();
		for (int j = 0; j < redComp.size(); j++)
		{
			QString compID = redComp[j].trimmed();
			COMPONENT_STATS *pComp = getCompStatsFromName(compID);
			if (pComp == NULL)
			{
				ASSERT(false, "Invalid item '%s' in list of redundant components of research '%s' ", compID.toUtf8().constData(), getName(&research));
			}else
			{
				research.pRedArtefacts.push_back(pComp);
			}
		}

		//set result structures
		QStringList resStruct = ini.value("resultStructures").toStringList();
		for (int j = 0; j < resStruct.size(); j++)
		{
			QString strucID = resStruct[j].trimmed();
			int structIndex = getStructStatFromName(strucID.toUtf8().data());
			ASSERT(structIndex >= 0, "Invalid item '%s' in list of result structures of research '%s' ", strucID.toUtf8().constData(), getName(&research));
			if (structIndex >= 0)
			{
				research.pStructureResults.push_back(structIndex);
			}
		}

		//set required structures
		QStringList reqStruct = ini.value("requiredStructures").toStringList();
		for (int j = 0; j < reqStruct.size(); j++)
		{
			QString strucID = reqStruct[j].trimmed();
			int structIndex = getStructStatFromName(strucID.toUtf8().data());
			ASSERT(structIndex >= 0, "Invalid item '%s' in list of required structures of research '%s' ", strucID.toUtf8().constData(), getName(&research));
			if(structIndex >= 0)
			{
				research.pStructList.push_back(structIndex);
			}
		}

		//set redundant structures
		QStringList redStruct = ini.value("redStructures").toStringList();
		for (int j = 0; j < redStruct.size(); j++)
		{
			QString strucID = redStruct[j].trimmed();
			int structIndex = getStructStatFromName(strucID.toUtf8().data());
			ASSERT(structIndex >= 0, "Invalid item '%s' in list of redundant structures of research '%s' ", strucID.toUtf8().constData(), getName(&research));
			if (structIndex >= 0)
			{
				research.pRedStructs.push_back(structIndex);
			}
		}

		asResearch.push_back(research);
		ini.endGroup();
	}

	//Load and check research pre-requisites (need do it AFTER loading research items)
	for (int inc = 0; inc < asResearch.size(); inc++)
	{
		QStringList preRes = preResearch[inc];
		for (int j = 0; j < preRes.size(); j++)
		{
			QString resID = preRes[j].trimmed();
			RESEARCH *preResItem = getResearch(resID.toUtf8().constData());
			ASSERT(preResItem != NULL, "Invalid item '%s' in list of pre-requisites of research '%s' ", resID.toUtf8().constData(), getName(&asResearch[inc]));
			if (preResItem != NULL)
				asResearch[inc].pPRList.push_back(preResItem->index);
		}
	}

	return true;
}

bool researchAvailable(int inc, int playerID, QUEUE_MODE mode)
{
	// Decide whether to use IsResearchCancelledPending/IsResearchStartedPending or IsResearchCancelled/IsResearchStarted.
	bool (*IsResearchCancelledFunc)(PLAYER_RESEARCH const *) = IsResearchCancelledPending;
	bool (*IsResearchStartedFunc)(PLAYER_RESEARCH const *) = IsResearchStartedPending;
	if (mode == ModeImmediate)
	{
		IsResearchCancelledFunc = IsResearchCancelled;
		IsResearchStartedFunc = IsResearchStarted;
	}

	UDWORD				incPR, incS;
	bool				bPRFound, bStructFound;

	// if its a cancelled topic - add to list
	if (IsResearchCancelledFunc(&asPlayerResList[playerID][inc]))
	{
		return true;
	}
	// if the topic is possible and has not already been researched - add to list
	if ((IsResearchPossible(&asPlayerResList[playerID][inc])))
	{
		if (!IsResearchCompleted(&asPlayerResList[playerID][inc])
		    && !IsResearchStartedFunc(&asPlayerResList[playerID][inc]))
		{
			return true;
		}
	}

	// if single player mode and key topic, then ignore cos can't do it!
	if (!bMultiPlayer && asResearch[inc].keyTopic)
	{
		return false;
	}

	bool researchStarted = IsResearchStartedFunc(&asPlayerResList[playerID][inc]);
	if (researchStarted)
	{
		STRUCTURE *psBuilding = findResearchingFacilityByResearchIndex(playerID, inc);  // May fail to find the structure here, if the research is merely pending, not actually started.
		if (psBuilding != NULL && psBuilding->status == SS_BEING_BUILT)
		{
			researchStarted = false;  // Although research is started, the facility is currently being upgraded or demolished, so we want to be able to research this elsewhere.
		}
	}

	// make sure that the research is not completed  or started by another researchfac
	if (!IsResearchCompleted(&asPlayerResList[playerID][inc]) && !researchStarted)
	{
		// Research is not completed  ... also  it has not been started by another researchfac

		// if there aren't any PR's - go to next topic
		if (asResearch[inc].pPRList.empty())
		{
			return false;
		}

		// check for pre-requisites
		bPRFound = true;
		for (incPR = 0; incPR < asResearch[inc].pPRList.size(); incPR++)
		{
			if (IsResearchCompleted(&(asPlayerResList[playerID][asResearch[inc].pPRList[incPR]]))==0)
			{
				// if haven't pre-requisite - quit checking rest
				bPRFound = false;
				break;
			}
		}
		if (!bPRFound)
		{
			// if haven't pre-requisites, skip the rest of the checks
			return false;
		}

		// check for structure effects
		bStructFound = true;
		for (incS = 0; incS < asResearch[inc].pStructList.size(); incS++)
		{
			if (!checkSpecificStructExists(asResearch[inc].pStructList[incS], playerID))
			{
				//if not built, quit checking
				bStructFound = false;
				break;
			}
		}
		if (!bStructFound)
		{
			// if haven't all structs built, skip to next topic
			return false;
		}
		return true;
	}
	return false;
}

/*
Function to check what can be researched for a particular player at any one
instant.

A topic can be researched if the playerRes 'possible' flag has been set (by script)
or if the research pre-req topics have been researched. A check is made for any
structures that are required to have been built for topics that do not have
the 'possible' flag set.

 **NB** A topic with zero PR's can ONLY be researched once the 'possible' flag
 has been set.

There can only be 'limit' number of entries
'topic' is the currently researched topic
*/
// NOTE by AJL may 99 - skirmish now has it's own version of this, skTopicAvail.
UWORD fillResearchList(UWORD *plist, UDWORD playerID, UWORD topic, UWORD limit)
{
	UWORD				inc, count=0;

	for (inc=0; inc < asResearch.size(); inc++)
	{
		// if the inc matches the 'topic' - automatically add to the list
		if (inc == topic || researchAvailable(inc, playerID, ModeQueue))
		{
			*plist++ = inc;
			count++;
			if (count == limit)
			{
				return count;
			}
		}
	}
	return count;
}

/* process the results of a completed research topic */
void researchResult(UDWORD researchIndex, UBYTE player, bool bDisplay, STRUCTURE *psResearchFacility, bool bTrigger)
{
	RESEARCH *                                      pResearch = &asResearch[researchIndex];
	MESSAGE						*pMessage;
	//the message gets sent to console
	char						consoleMsg[MAX_RESEARCH_MSG_SIZE];

	ASSERT_OR_RETURN( , researchIndex < asResearch.size(), "Invalid research index %u", researchIndex);

	MakeResearchCompleted(&asPlayerResList[player][researchIndex]);

	//check for structures to be made available
	for (int inc = 0; inc < pResearch->pStructureResults.size(); inc++)
	{
		if (apStructTypeLists[player][pResearch->pStructureResults[inc]] != REDUNDANT)
		{
			apStructTypeLists[player][pResearch->pStructureResults[inc]] = AVAILABLE;
		}
	}

	//check for structures to be made redundant
	for (int inc = 0; inc < pResearch->pRedStructs.size(); inc++)
	{
		apStructTypeLists[player][pResearch->pRedStructs[inc]] = REDUNDANT;
	}

	//check for component replacement
	if (pResearch->componentReplacement.size() != 0)
	{
		for (int ri = 0; ri < pResearch->componentReplacement.size(); ri++)
		{
			COMPONENT_STATS *pOldComp = pResearch->componentReplacement[ri].pOldComponent;
			replaceComponent(pResearch->componentReplacement[ri].pNewComponent, pOldComp, player);
			apCompLists[player][pOldComp->compType][pOldComp->index] = REDUNDANT;
		}
	}

	//check for artefacts to be made available
	for (int inc = 0; inc < pResearch->componentResults.size(); inc++)
	{
		//determine the type of artefact
		COMPONENT_TYPE type = pResearch->componentResults[inc]->compType;
		//set the component state to AVAILABLE
		int compInc = pResearch->componentResults[inc]->index;
		if (apCompLists[player][type][compInc] != REDUNDANT)
		{
			apCompLists[player][type][compInc] = AVAILABLE;
		}
		//check for default sensor
		if (type == COMP_SENSOR && (asSensorStats + compInc)->location == LOC_DEFAULT)
		{
			aDefaultSensor[player] = compInc;
		}
		//check for default ECM
		else if (type == COMP_ECM && (asECMStats + compInc)->location == LOC_DEFAULT)
		{
			aDefaultECM[player] = compInc;
		}
		//check for default Repair
		else if (type == COMP_REPAIRUNIT && (asRepairStats + compInc)->location == LOC_DEFAULT)
		{
			aDefaultRepair[player] = compInc;
			enableSelfRepair(player);
		}
	}

	//check for artefacts to be made redundant
	for (int inc = 0; inc < pResearch->pRedArtefacts.size(); inc++)
	{
		COMPONENT_TYPE type = pResearch->pRedArtefacts[inc]->compType;
		apCompLists[player][type][pResearch->pRedArtefacts[inc]->index] = REDUNDANT;
	}

	//Add message to player's list if Major Topic
	if ((pResearch->techCode == TC_MAJOR) && bDisplay)
	{
		//only play sound if major topic
		if (player == selectedPlayer)
		{
			audio_QueueTrack(ID_SOUND_MAJOR_RESEARCH);
		}

		//check there is viewdata for the research topic - just don't add message if not!
		if (pResearch->pViewData != NULL)
		{
			pMessage = addMessage(MSG_RESEARCH, false, player);
			if (pMessage != NULL)
			{
				pMessage->pViewData = (MSG_VIEWDATA *)pResearch->pViewData;
			}
		}
	}
	else if (player == selectedPlayer && bDisplay)
	{
		audio_QueueTrack(ID_SOUND_RESEARCH_COMPLETED);
	}

	if (player == selectedPlayer && bDisplay)
	{
		//add console text message
		if (pResearch->pViewData != NULL)
		{
			snprintf(consoleMsg, MAX_RESEARCH_MSG_SIZE, _("Research completed: %s"), _(pResearch->pViewData->textMsg[0].toUtf8().constData()));
			addConsoleMessage(consoleMsg, LEFT_JUSTIFY, SYSTEM_MESSAGE);
		}
		else
		{
			addConsoleMessage(_("Research Completed"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
		}
	}

	if (psResearchFacility)
	{
		psResearchFacility->pFunctionality->researchFacility.psSubject = NULL;		// Make sure topic is cleared
	}
	if ((bMultiPlayer || player == selectedPlayer) && bTrigger)
	{
		psCBLastResearch = pResearch;  // Fun with pointers. Throw them into some random global variable, and get Nexus to absorb them.
		CBResFacilityOwner = player;
		psCBLastResStructure = psResearchFacility;
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_RESEARCHCOMPLETED);
		psCBLastResStructure = NULL;
		CBResFacilityOwner = -1;
		psCBLastResearch = NULL;
	}
	triggerEventResearched(pResearch, psResearchFacility, player);
}

/*This function is called when the research files are reloaded*/
bool ResearchShutDown(void)
{
	ResearchRelease();
	return true;
}

/*This function is called when a game finishes*/
void ResearchRelease(void)
{
	asResearch.clear();
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		asPlayerResList[i].clear();
	}
}

/*puts research facility on hold*/
void holdResearch(STRUCTURE *psBuilding, QUEUE_MODE mode)
{
	ASSERT( psBuilding->pStructureType->type == REF_RESEARCH,
		"holdResearch: structure not a research facility" );

	RESEARCH_FACILITY *psResFac = &psBuilding->pFunctionality->researchFacility;

	if (mode == ModeQueue)
	{
		sendStructureInfo(psBuilding, STRUCTUREINFO_HOLDRESEARCH, NULL);
		setStatusPendingHold(*psResFac);
		return;
	}

	if (psResFac->psSubject)
	{
		//set the time the research facilty was put on hold
		psResFac->timeStartHold = gameTime;
		//play audio to indicate on hold
		if (psBuilding->player == selectedPlayer)
		{
			audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
		}
	}

	delPowerRequest(psBuilding);
}

/*release a research facility from hold*/
void releaseResearch(STRUCTURE *psBuilding, QUEUE_MODE mode)
{
	ASSERT( psBuilding->pStructureType->type == REF_RESEARCH,
		"releaseResearch: structure not a research facility" );

	RESEARCH_FACILITY *psResFac = &psBuilding->pFunctionality->researchFacility;

	if (mode == ModeQueue)
	{
		sendStructureInfo(psBuilding, STRUCTUREINFO_RELEASERESEARCH, NULL);
		setStatusPendingRelease(*psResFac);
		return;
	}

	if (psResFac->psSubject && psResFac->timeStartHold)
	{
		//adjust the start time for the current subject
		psResFac->timeStartHold = 0;
	}
}


/*

	Cancel All Research for player 0

*/
void CancelAllResearch(UDWORD pl)
{
	STRUCTURE	*psCurr;

	for (psCurr=apsStructLists[pl]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (psCurr->pStructureType->type == REF_RESEARCH)
		{
			if (
				 ( ((RESEARCH_FACILITY *)psCurr->pFunctionality)!=NULL )
			 &&  ( ((RESEARCH_FACILITY *)psCurr->pFunctionality)->psSubject !=NULL )
			   )
			{
				debug( LOG_NEVER, "canceling research for %p\n", psCurr );
				cancelResearch(psCurr, ModeQueue);
			}
		}

	}
}

/** Sets the status of the topic to cancelled and stores the current research points accquired */
void cancelResearch(STRUCTURE *psBuilding, QUEUE_MODE mode)
{
	UDWORD              topicInc;
	PLAYER_RESEARCH	    *pPlayerRes;

	ASSERT(psBuilding->pStructureType->type == REF_RESEARCH, "Structure not a research facility");

	RESEARCH_FACILITY *psResFac = &psBuilding->pFunctionality->researchFacility;
	if( !(RESEARCH *)psResFac->psSubject)
	{
		debug(LOG_SYNC, "Invalid research topic");
		return;
	}
	topicInc = ((RESEARCH *)psResFac->psSubject)->index;
	ASSERT_OR_RETURN(, topicInc <= asResearch.size(), "Invalid research topic %u (max %d)", topicInc, (int)asResearch.size());
	pPlayerRes = &asPlayerResList[psBuilding->player][topicInc];
	if (psBuilding->pStructureType->type == REF_RESEARCH)
	{
		if (mode == ModeQueue)
		{
			// Tell others that we want to stop researching something.
			sendResearchStatus(psBuilding, topicInc, psBuilding->player, false);
			// Immediately tell the UI that we can research this now. (But don't change the game state.)
			MakeResearchCancelledPending(pPlayerRes);
			setStatusPendingCancel(*psResFac);
			return;  // Wait for our message before doing anything. (Whatever this function does...)
		}

		//check if waiting to accrue power
		if (pPlayerRes->currentPoints == 0)
		{
			// Reset this topic as not having been researched
			ResetResearchStatus(pPlayerRes);
		}
		else
		{
			// Set the researched flag
			MakeResearchCancelled(pPlayerRes);
		}

		// Initialise the research facility's subject
		psResFac->psSubject = NULL;

		delPowerRequest(psBuilding);
	}
}

/* For a given view data get the research this is related to */
RESEARCH *getResearchForMsg(VIEWDATA *pViewData)
{
	for (int inc = 0; inc < asResearch.size(); inc++)
	{
		if (asResearch[inc].pViewData == pViewData)	// compare the pointer
		{
			return &asResearch[inc];
		}
	}
	return NULL;
}

//set the iconID based on the name read in in the stats
static UWORD setIconID(char *pIconName, const char *pName)
{
	//compare the names with those created in 'Framer'
	if (!strcmp(pIconName, "IMAGE_ROCKET"))
	{
		return IMAGE_ROCKET;
	}
	if (!strcmp(pIconName, "IMAGE_CANNON"))
	{
		return IMAGE_CANNON;
	}
	if (!strcmp(pIconName, "IMAGE_HOVERCRAFT"))
	{
		return IMAGE_HOVERCRAFT;
	}
	if (!strcmp(pIconName, "IMAGE_ECM"))
	{
		return IMAGE_ECM;
	}
	if (!strcmp(pIconName, "IMAGE_PLASCRETE"))
	{
		return IMAGE_PLASCRETE;
	}
	if (!strcmp(pIconName, "IMAGE_TRACKS"))
	{
		return IMAGE_TRACKS;
	}

	if (!strcmp(pIconName, "IMAGE_RES_DROIDTECH"))
	{
		return IMAGE_RES_DROIDTECH;
	}

	if (!strcmp(pIconName,"IMAGE_RES_WEAPONTECH"))
	{
		return IMAGE_RES_WEAPONTECH;
	}

	if (!strcmp(pIconName,"IMAGE_RES_COMPUTERTECH"))
	{
		return IMAGE_RES_COMPUTERTECH;
	}

	if (!strcmp(pIconName,"IMAGE_RES_POWERTECH"))
	{
		return IMAGE_RES_POWERTECH;
	}

	if (!strcmp(pIconName,"IMAGE_RES_SYSTEMTECH"))
	{
		return IMAGE_RES_SYSTEMTECH;
	}

	if (!strcmp(pIconName,"IMAGE_RES_STRUCTURETECH"))
	{
		return IMAGE_RES_STRUCTURETECH;
	}

	if (!strcmp(pIconName,"IMAGE_RES_CYBORGTECH"))
	{
		return IMAGE_RES_CYBORGTECH;
	}

	if (!strcmp(pIconName,"IMAGE_RES_DEFENCE"))
	{
		return IMAGE_RES_DEFENCE;
	}

	if (!strcmp(pIconName,"IMAGE_RES_QUESTIONMARK"))
	{
		return IMAGE_RES_QUESTIONMARK;
	}

	if (!strcmp(pIconName,"IMAGE_RES_GRPACC"))
	{
		return IMAGE_RES_GRPACC;
	}

	if (!strcmp(pIconName,"IMAGE_RES_GRPUPG"))
	{
		return IMAGE_RES_GRPUPG;
	}

	if (!strcmp(pIconName,"IMAGE_RES_GRPREP"))
	{
		return IMAGE_RES_GRPREP;
	}

	if (!strcmp(pIconName,"IMAGE_RES_GRPROF"))
	{
		return IMAGE_RES_GRPROF;
	}

	if (!strcmp(pIconName,"IMAGE_RES_GRPDAM"))
	{
		return IMAGE_RES_GRPDAM;
	}

	// Add more names as images are created
	ASSERT( false, "Invalid icon graphic %s for topic %s", pIconName, pName );

	return NO_RESEARCH_ICON;	// Should never get here.
}


SDWORD	mapRIDToIcon( UDWORD rid )
{
	switch(rid)
	{
	case RID_ROCKET:
		return(IMAGE_ROCKET);
		break;
	case RID_CANNON:
		return(IMAGE_CANNON);
		break;
	case RID_HOVERCRAFT:
		return(IMAGE_HOVERCRAFT);
		break;
	case RID_ECM:
		return(IMAGE_ECM);
		break;
	case RID_PLASCRETE:
		return(IMAGE_PLASCRETE);
		break;
	case RID_TRACKS:
		return(IMAGE_TRACKS);
		break;
	case RID_DROIDTECH:
		return(IMAGE_RES_DROIDTECH);
		break;
	case RID_WEAPONTECH:
		return(IMAGE_RES_WEAPONTECH);
		break;
	case RID_COMPUTERTECH:
		return(IMAGE_RES_COMPUTERTECH);
		break;
	case RID_POWERTECH:
		return(IMAGE_RES_POWERTECH);
		break;
	case RID_SYSTEMTECH:
		return(IMAGE_RES_SYSTEMTECH);
		break;
	case RID_STRUCTURETECH:
		return(IMAGE_RES_STRUCTURETECH);
		break;
	case RID_CYBORGTECH:
		return(IMAGE_RES_CYBORGTECH);
		break;
	case RID_DEFENCE:
		return(IMAGE_RES_DEFENCE);
		break;
	case RID_QUESTIONMARK:
		return(IMAGE_RES_QUESTIONMARK);
		break;
	case RID_GRPACC:
		return(IMAGE_RES_GRPACC);
		break;
	case RID_GRPUPG:
		return(IMAGE_RES_GRPUPG);
		break;
	case RID_GRPREP:
		return(IMAGE_RES_GRPREP);
		break;
	case RID_GRPROF:
		return(IMAGE_RES_GRPROF);
		break;
	case RID_GRPDAM:
		return(IMAGE_RES_GRPDAM);
		break;

	default:
		ASSERT( false,"Weirdy mapping request for RID to icon" );
		return(-1); //pass back a value that can never have been set up
		break;
	}
}

SDWORD	mapIconToRID(UDWORD iconID)
{
	switch(iconID)
	{
	case IMAGE_ROCKET:
		return(RID_ROCKET);
		break;
	case IMAGE_CANNON:
		return(RID_CANNON);
		break;
	case IMAGE_HOVERCRAFT:
		return(RID_HOVERCRAFT);
		break;
	case IMAGE_ECM:
		return(RID_ECM);
		break;
	case IMAGE_PLASCRETE:
		return(RID_PLASCRETE);
		break;
	case IMAGE_TRACKS:
		return(RID_TRACKS);
		break;
	case IMAGE_RES_DROIDTECH:
		return(RID_DROIDTECH);
		break;
	case IMAGE_RES_WEAPONTECH:
		return(RID_WEAPONTECH);
		break;
	case IMAGE_RES_COMPUTERTECH:
		return(RID_COMPUTERTECH);
		break;
	case IMAGE_RES_POWERTECH:
		return(RID_POWERTECH);
		break;
	case IMAGE_RES_SYSTEMTECH:
		return(RID_SYSTEMTECH);
		break;
	case IMAGE_RES_STRUCTURETECH:
		return(RID_STRUCTURETECH);
		break;
	case IMAGE_RES_CYBORGTECH:
		return(RID_CYBORGTECH);
		break;
	case IMAGE_RES_DEFENCE:
		return(RID_DEFENCE);
		break;
	case IMAGE_RES_QUESTIONMARK:
		return(RID_QUESTIONMARK);
		break;
	case IMAGE_RES_GRPACC:
		return(RID_GRPACC);
		break;
	case IMAGE_RES_GRPUPG:
		return(RID_GRPUPG);
		break;
	case IMAGE_RES_GRPREP:
		return(RID_GRPREP);
		break;
	case IMAGE_RES_GRPROF:
		return(RID_GRPROF);
		break;
	case IMAGE_RES_GRPDAM:
		return(RID_GRPDAM);
		break;
	default:
		return(-1); //pass back a value that can never have been set up
		break;
	}
}

//return a pointer to a research topic based on the name
RESEARCH *getResearch(const char *pName)
{
	for (int inc = 0; inc < asResearch.size(); inc++)
	{
		if (asResearch[inc].id.compare(pName) == 0)
		{
			return &asResearch[inc];
		}
	}
	debug(LOG_WARNING, "Unknown research - %s", pName);
	return NULL;
}

/* looks through the players lists of structures and droids to see if any are using
 the old component - if any then replaces them with the new component */
static void replaceComponent(COMPONENT_STATS *pNewComponent, COMPONENT_STATS *pOldComponent,
					  UBYTE player)
{
	DROID_TEMPLATE	*psTemplates;
	COMPONENT_TYPE oldType = pOldComponent->compType;
	int oldCompInc = pOldComponent->index;
	COMPONENT_TYPE newType = pNewComponent->compType;
	int newCompInc = pNewComponent->index;

	//check old and new type are the same
	if (oldType != newType)
	{
		return;
	}

	replaceDroidComponent(apsDroidLists[player], oldType, oldCompInc, newCompInc);
	replaceDroidComponent(mission.apsDroidLists[player], oldType, oldCompInc, newCompInc);
	replaceDroidComponent(apsLimboDroids[player], oldType, oldCompInc, newCompInc);

	//check thru the templates
	for (psTemplates = apsDroidTemplates[player]; psTemplates != NULL; psTemplates = psTemplates->psNext)
	{
		switch(oldType)
		{
			case COMP_BODY:
			case COMP_BRAIN:
			case COMP_PROPULSION:
			case COMP_REPAIRUNIT:
			case COMP_ECM:
			case COMP_SENSOR:
			case COMP_CONSTRUCT:
				if (psTemplates->asParts[oldType] == (SDWORD)oldCompInc)
				{
					psTemplates->asParts[oldType] = newCompInc;
				}
				break;
			case COMP_WEAPON:
				for (int inc = 0; inc < psTemplates->numWeaps; inc++)
				{
					if (psTemplates->asWeaps[inc] == oldCompInc)
					{
						psTemplates->asWeaps[inc] = newCompInc;
					}
				}
				break;
			default:
				//unknown comp type
				debug( LOG_ERROR, "Unknown component type - invalid Template" );
				return;
		}
	}

	replaceStructureComponent(apsStructLists[player], oldType, oldCompInc, newCompInc, player);
	replaceStructureComponent(mission.apsStructLists[player], oldType, oldCompInc, newCompInc, player);
}

/*Looks through all the currently allocated stats to check the name is not
a duplicate*/
static bool checkResearchName(RESEARCH *psResearch, UDWORD numStats)
{
	for (int inc = 0; inc < numStats; inc++)
	{

		ASSERT_OR_RETURN(false, asResearch[inc].id.compare(psResearch->id) != 0,
		                 "Research name has already been used - %s", getName(psResearch));
	}
	return true;
}

/* Sets the 'possible' flag for a player's research so the topic will appear in
the research list next time the Research Facilty is selected */
bool enableResearch(RESEARCH *psResearch, UDWORD player)
{
	UDWORD				inc;

	ASSERT_OR_RETURN(false, psResearch, "No such research topic");

	inc = psResearch->index;
	if (inc > asResearch.size())
	{
		ASSERT( false, "enableResearch: Invalid research topic - %s", getName(psResearch) );
		return false;
	}

	int prevState = intGetResearchState();

	//found, so set the flag
	MakeResearchPossible(&asPlayerResList[player][inc]);

	if (player == selectedPlayer)
	{
		//set the research reticule button to flash if research facility is free
		intNotifyResearchButton(prevState);
	}

	return true;
}

/*find the last research topic of importance that the losing player did and
'give' the results to the reward player*/
void researchReward(UBYTE losingPlayer, UBYTE rewardPlayer)
{
	UDWORD topicIndex = 0, researchPoints = 0, rewardID = 0;
	STRUCTURE			*psStruct;
	RESEARCH_FACILITY	*psFacility;

	//look through the losing players structures to find a research facility
	for (psStruct = apsStructLists[losingPlayer]; psStruct != NULL; psStruct =
		psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_RESEARCH)
		{
			psFacility = (RESEARCH_FACILITY *)psStruct->pFunctionality;
			if (psFacility->psBestTopic)
			{
				topicIndex = ((RESEARCH *)psFacility->psBestTopic)->ref -
					REF_RESEARCH_START;
				if (topicIndex)
				{
					//if it cost more - it is better (or should be)
					if (researchPoints < asResearch[topicIndex].researchPoints)
					{
						//store the 'best' topic
						researchPoints = asResearch[topicIndex].researchPoints;
						rewardID = topicIndex;
					}
				}
			}
		}
	}

	//if a topic was found give the reward player the results of that research
	if (rewardID)
	{
		researchResult(rewardID, rewardPlayer, true, NULL, true);
		if (rewardPlayer == selectedPlayer)
		{
			//name the actual reward
			CONPRINTF(ConsoleString,(ConsoleString,"%s :- %s",
				_("Research Award"),
				getName(&asResearch[rewardID])));
		}
	}
}

/*flag self repair so droids can start when idle*/
void enableSelfRepair(UBYTE player)
{
	bSelfRepair[player] = true;
}

/*check to see if any research has been completed that enables self repair*/
bool selfRepairEnabled(UBYTE player)
{
	if (bSelfRepair[player])
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*checks the stat to see if its of type wall or defence*/
bool wallDefenceStruct(STRUCTURE_STATS *psStats)
{
	if (psStats->type == REF_DEFENSE || psStats->type == REF_WALL || psStats->type == REF_GATE
	    || psStats->type == REF_WALLCORNER || psStats->type == REF_GENERIC)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*for a given list of droids, replace the old component if exists*/
void replaceDroidComponent(DROID *pList, UDWORD oldType, UDWORD oldCompInc,
                      UDWORD newCompInc)
{
	DROID   *psDroid;

	//check thru the droids
	for (psDroid = pList; psDroid != NULL; psDroid = psDroid->psNext)
	{
		switchComponent(psDroid, oldType, oldCompInc, newCompInc);
		// Need to replace the units inside the transporter
		if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
		{
			replaceTransDroidComponents(psDroid, oldType, oldCompInc, newCompInc);
		}
	}
}

/*replaces any components necessary for units that are inside a transporter*/
void replaceTransDroidComponents(DROID *psTransporter, UDWORD oldType,
                                 UDWORD oldCompInc, UDWORD newCompInc)
{
    DROID       *psCurr;

    ASSERT ((psTransporter->droidType == DROID_TRANSPORTER || psTransporter->droidType == DROID_SUPERTRANSPORTER), "invalid unit type" );

    for (psCurr = psTransporter->psGroup->psList; psCurr != NULL; psCurr =
        psCurr->psGrpNext)
    {
        if (psCurr != psTransporter)
        {
            switchComponent(psCurr, oldType, oldCompInc, newCompInc);
        }
    }
}

void replaceStructureComponent(STRUCTURE *pList, UDWORD oldType, UDWORD oldCompInc,
                      UDWORD newCompInc, UBYTE player)
{
	STRUCTURE   *psStructure;
	int			inc;

	// If the type is not one we are interested in, then don't bother checking
	if (!(oldType == COMP_ECM || oldType == COMP_SENSOR || oldType == COMP_WEAPON))
	{
		return;
	}

	//check thru the structures
	for (psStructure = pList; psStructure != NULL; psStructure = psStructure->psNext)
	{
		switch (oldType)
		{
			case COMP_WEAPON:
				for (inc=0; inc < psStructure->numWeaps; inc++)
				{
					if (psStructure->asWeaps[inc].nStat > 0)
					{
						if (psStructure->asWeaps[inc].nStat == oldCompInc)
						{
							psStructure->asWeaps[inc].nStat = newCompInc;
						}
					}
				}
				break;
			default:
				//ignore all other component types
				break;
		}
	}
}

/*swaps the old component for the new one for a specific droid*/
static void switchComponent(DROID *psDroid, UDWORD oldType, UDWORD oldCompInc,
                            UDWORD newCompInc)
{
	ASSERT(psDroid != NULL, "switchComponent:invalid droid pointer");

	switch(oldType)
	{
		case COMP_BODY:
		case COMP_BRAIN:
		case COMP_PROPULSION:
		case COMP_REPAIRUNIT:
		case COMP_ECM:
		case COMP_SENSOR:
		case COMP_CONSTRUCT:
			if (psDroid->asBits[oldType] == oldCompInc)
			{
				psDroid->asBits[oldType] = (UBYTE)newCompInc;
			}
			break;
		case COMP_WEAPON:
			// Can only be one weapon now
			if (psDroid->asWeaps[0].nStat > 0)
			{
				if (psDroid->asWeaps[0].nStat == oldCompInc)
				{
					psDroid->asWeaps[0].nStat = newCompInc;
				}
			}
			break;
		default:
			//unknown comp type
			debug( LOG_ERROR, "Unknown component type - invalid droid" );
			abort();
			return;
	}
}

static inline bool allyResearchSortFunction(AllyResearch const &a, AllyResearch const &b)
{
	if (a.active         != b.active)         { return a.active; }
	if (a.timeToResearch != b.timeToResearch) { return (unsigned)a.timeToResearch < (unsigned)b.timeToResearch; }  // Unsigned cast = sort -1 as infinite.
	if (a.powerNeeded    != b.powerNeeded)    { return (unsigned)a.powerNeeded    < (unsigned)b.powerNeeded;    }
	if (a.completion     != b.completion)     { return           a.completion     >           b.completion;     }
	                                            return           a.player         <           b.player;
}

std::vector<AllyResearch> const &listAllyResearch(unsigned ref)
{
	static uint32_t lastGameTime = ~0;
	static std::map<unsigned, std::vector<AllyResearch> > researches;
	static const std::vector<AllyResearch> noAllyResearch;

	if (gameTime != lastGameTime)
	{
		lastGameTime = gameTime;
		researches.clear();

		for (int player = 0; player < MAX_PLAYERS; ++player)
		{
			if (player == selectedPlayer || !aiCheckAlliances(selectedPlayer, player) || !alliancesSharedResearch(game.alliance))
			{
				continue;  // Skip this player, not an ally.
			}

			// Check each research facility to see if they are doing this topic. (As opposed to having started the topic, but stopped researching it.)
			for (STRUCTURE *psStruct = apsStructLists[player]; psStruct != NULL; psStruct = psStruct->psNext)
			{
				RESEARCH_FACILITY *res = (RESEARCH_FACILITY *)psStruct->pFunctionality;
				if (psStruct->pStructureType->type != REF_RESEARCH || res->psSubject == NULL)
				{
					continue;  // Not a researching research facility.
				}

				RESEARCH const &subject = *res->psSubject;
				PLAYER_RESEARCH const &playerRes = asPlayerResList[player][subject.index];
				unsigned cRef = subject.ref;

				AllyResearch r;
				r.player = player;
				r.completion = playerRes.currentPoints;
				r.powerNeeded = checkPowerRequest(psStruct);
				r.timeToResearch = -1;
				if (r.powerNeeded == -1)
				{
					r.timeToResearch = (subject.researchPoints - playerRes.currentPoints)
					                   / MAX(psStruct->pStructureType->base.research, 1u);
				}
				r.active = psStruct->status == SS_BUILT;
				researches[cRef].push_back(r);
			}
		}
		for (std::map<unsigned, std::vector<AllyResearch> >::iterator i = researches.begin(); i != researches.end(); ++i)
		{
			std::sort(i->second.begin(), i->second.end(), allyResearchSortFunction);
		}
	}

	std::map<unsigned, std::vector<AllyResearch> >::const_iterator i = researches.find(ref);
	if (i == researches.end())
	{
		return noAllyResearch;
	}
	return i->second;
}
