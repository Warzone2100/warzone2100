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

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/frameresource.h"
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


//used to calc the research power
#define RESEARCH_FACTOR		32
#define RESEARCH_MAX_POWER  450

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
static UWORD setIconID(char *pIconName, char *pName);
static COMPONENT_STATS * getComponentDetails(char *pName, char *pCompName);
static void replaceComponent(COMPONENT_STATS *pNewComponent, COMPONENT_STATS *pOldComponent,
					  UBYTE player);
static bool checkResearchName(RESEARCH *psRes, UDWORD numStats);

static const char *getResearchName(RESEARCH *pResearch)
{
	return(getName(pResearch->pName));
}

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
bool loadResearch(const char *pResearchData, UDWORD bufferSize)
{
	unsigned int researchCount = numCR(pResearchData, bufferSize);
	COMPONENT_STATS *psComp;
	SDWORD structID;
	UDWORD i, keyTopic, techCode, resPoints;
	char ResearchName[MAX_STR_LENGTH];
	char msgName[MAX_STR_LENGTH], iconID[MAX_STR_LENGTH];
	char imdName[MAX_STR_LENGTH], imdName2[MAX_STR_LENGTH];
	char structName[MAX_STR_LENGTH], compName[MAX_STR_LENGTH],
		compType[MAX_STR_LENGTH];
	PLAYER_RESEARCH dummy;

	memset(&dummy, 0, sizeof(dummy));

	// Skip descriptive header
	if (strncmp(pResearchData,"Research ",9)==0)
	{
		pResearchData = strchr(pResearchData,'\n') + 1;
		researchCount--;
	}

	for (i = 0; i < researchCount; i++)
	{
		// HACK FIXME: the code assumes we have empty PLAYER_RESEARCH entries to throw around
		for (int j = 0; j < MAX_PLAYERS; j++)
		{
			asPlayerResList[j].push_back(dummy);
		}

		RESEARCH research;

		research.index = i;
		//read the data into the storage - the data is delimeted using comma's
		ResearchName[0] = '\0';
		sscanf(pResearchData,"%255[^,'\r\n],", ResearchName);

		//allocate storage for the name
		research.pName = allocateName(ResearchName);
		ASSERT_OR_RETURN(false, research.pName != NULL, "Failed allocating research name");

		//check the name hasn't been used already
		ASSERT_OR_RETURN(false, checkResearchName(&research, i), "Research name %s used already", research.pName);

		pResearchData += (strlen(ResearchName)+1);
		research.ref = REF_RESEARCH_START + i;

		//determine the tech level (unused, so we don't use the resulting string)
		ResearchName[0] = '\0';
		sscanf(pResearchData,"%255[^,'\r\n],", ResearchName);
		pResearchData += (strlen(ResearchName)+1);

		ResearchName[0] = '\0';
		sscanf(pResearchData,"%255[^,'\r\n],", ResearchName);

		if (strcmp(ResearchName, "0"))
		{
			research.subGroup = setIconID(ResearchName, research.pName);
		}
		else
		{
			research.subGroup = NO_RESEARCH_ICON;
		}

		pResearchData += (strlen(ResearchName)+1);

		iconID[0] = '\0';
		imdName[0] = '\0';
		imdName2[0] = '\0';
		msgName[0] = '\0';
		structName[0] = '\0';
		compName[0] = '\0';
		compType[0] = '\0';

		{
			UDWORD numPRRequired;
			UDWORD numFunctions;
			UDWORD numStructures;
			UDWORD numRedStructs;
			UDWORD numStructResults;
			UDWORD numRedArtefacts;
			UDWORD numArteResults;

			sscanf(pResearchData,"%d,%255[^,'\r\n],%255[^,'\r\n],%255[^,'\r\n],%255[^,'\r\n],%255[^,'\r\n], \
			       %255[^,'\r\n],%255[^,'\r\n],%d,%d,%d,%d,%d,%d,%d,%d,%d",
				&techCode, iconID, imdName, imdName2, msgName,
			       structName, compName, compType,
				&resPoints, &keyTopic, &numPRRequired,
				&numFunctions, &numStructures,
				&numRedStructs, &numStructResults,
				&numRedArtefacts, &numArteResults);
		}

		//set keytopic flag
		if (keyTopic)
		{
			research.keyTopic = true;
		}
		else
		{
			research.keyTopic = false;
		}

		//check the tech code is valid
		ASSERT_OR_RETURN(false, techCode <= 1, "Invalid tech code for research topic - %s ", getResearchName(&research));
		if (techCode == 0)
		{
			research.techCode = TC_MAJOR;
		}
		else
		{
			research.techCode = TC_MINOR;
		}

		//set the iconID
		if (strcmp(iconID, "0"))
		{
			research.iconID = setIconID(iconID, research.pName);
		}
		else
		{
			research.iconID = NO_RESEARCH_ICON;
		}

		//get the IMDs used in the interface
		if (strcmp(structName, "0"))
		{
			//find the structure stat
			structID = getStructStatFromName(structName);
			ASSERT_OR_RETURN(false, structID >= 0, "Cannot find the structure Stat for Research %s", getResearchName(&research));
			research.psStat = (BASE_STATS *)(asStructureStats + structID);
		}
		else if (strcmp(compName, "0"))
		{
			//find the component stat
			psComp = getComponentDetails(compType, compName);
			ASSERT_OR_RETURN(false, psComp != NULL, "Cannot find the component Stat for Research %s", getResearchName(&research));
			research.psStat = (BASE_STATS *)psComp;
		}
		else
		{
			research.psStat = NULL;
		}
		if (strcmp(imdName, "0"))
		{
			research.pIMD = (iIMDShape *) resGetData("IMD", imdName);
			ASSERT_OR_RETURN(false, research.pIMD != NULL, "Cannot find the research PIE for record %s", getResearchName(&research));
		}
		else
		{
			research.pIMD = NULL;
		}

		if (strcmp(imdName2, "0"))
		{
			research.pIMD2 = (iIMDShape *) resGetData("IMD", imdName2);
			ASSERT_OR_RETURN(false, research.pIMD2 != NULL, "Cannot find the 2nd research PIE for record %s", getResearchName(&research));
		}
		else
		{
			research.pIMD2 = NULL;
		}

		//get the message viewdata - if any
		if (strcmp(msgName, "0"))
		{
			//check its a major tech code
			ASSERT(research.techCode == TC_MAJOR, "This research should not have a message associated with it, %s the message will be ignored!", getResearchName(&research));
			if (research.techCode == TC_MAJOR)
			{
				research.pViewData = getViewData(msgName);
			}
		}

		//set the researchPoints
		ASSERT_OR_RETURN(false, resPoints <= UWORD_MAX, "Research Points too high for research topic - %s ", getResearchName(&research));
		research.researchPoints = (UWORD)resPoints;

		//set the research power
		research.researchPower = research.researchPoints / RESEARCH_FACTOR;
		if (research.researchPower > RESEARCH_MAX_POWER)
		{
			research.researchPower = RESEARCH_MAX_POWER;
		}

		//increment the pointer to the start of the next record
		pResearchData = strchr(pResearchData,'\n') + 1;
		asResearch.push_back(research);
	}

	return true;
}


//Load the pre-requisites for a research list
bool loadResearchPR(const char *pPRData, UDWORD bufferSize)
{
	unsigned NumToAlloc = numCR(pPRData, bufferSize);
	char				ResearchName[MAX_STR_LENGTH], PRName[MAX_STR_LENGTH];

	for (int i = 0; i < NumToAlloc; i++)
	{
		bool recFound = false;

		//read the data into the storage - the data is delimited using commas
		ResearchName[0] = '\0';
		PRName[0] = '\0';
		sscanf(pPRData,"%255[^,'\r\n],%255[^,'\r\n],%*d", ResearchName, PRName);

		//loop through each Research to compare the name
		for (int incR = 0; incR < asResearch.size(); incR++)
		{
			if (!(strcmp(ResearchName, asResearch[incR].pName)))
			{
				//Research found
				for (int incPR = 0; incPR < asResearch.size(); incPR++)
				{
					if (!(strcmp(PRName, asResearch[incPR].pName)))
					{
						//PRresearch found alloc this to the current Research
						asResearch[incR].pPRList.push_back(incPR);
						recFound = true;
						break;
					}
				}
				ASSERT_OR_RETURN(false, recFound, "Unable to find Pre-requisite %s for research %s", PRName, ResearchName);
				break;
			}
		}
		ASSERT_OR_RETURN(false, recFound, "Unable to find Research %s", ResearchName);
		// increment the pointer to the start of the next record
		pPRData = strchr(pPRData,'\n') + 1;
	}
	return true;
}

//Load the artefacts for a research list
bool loadResearchArtefacts(const char *pArteData, UDWORD bufferSize, UDWORD listNumber)
{
	unsigned NumToAlloc = numCR(pArteData, bufferSize);
	char			ResearchName[MAX_STR_LENGTH], ArteName[MAX_STR_LENGTH], TypeName[MAX_STR_LENGTH];
	COMPONENT_STATS		*pArtefact;
	UDWORD				newType;

	// Skip descriptive header
	if (strncmp(pArteData, "Research ", 9) == 0)
	{
		pArteData = strchr(pArteData, '\n') + 1;
		NumToAlloc--;
	}

	for (int i = 0; i < NumToAlloc; i++)
	{
		//read the data into the storage - the data is delimited using commas
		ResearchName[0] = '\0';
		ArteName[0] = '\0';
		TypeName[0] = '\0';
		sscanf(pArteData,"%255[^,'\r\n],%255[^,'\r\n],%255[^,'\r\n]", ResearchName, ArteName, TypeName);

		//increment the data pointer
		pArteData += (strlen(ResearchName)+1+strlen(ArteName)+1+strlen(TypeName)+1);

		pArtefact = getComponentDetails(TypeName, ArteName);
		if (pArtefact == NULL)
		{
			return false;
		}
		//get the type for comparison later
		newType = statType(pArtefact->ref);

		RESEARCH *pResearch = getResearch(ResearchName);
		if (pResearch == NULL)
		{
			return false;
		}

		//ArtefactResearch found - alloc the artefact to the current Research topic
		switch (listNumber)
		{
			case RED_LIST:
				pResearch->pRedArtefacts.push_back(pArtefact);
				break;
			case RES_LIST:
				pResearch->pArtefactResults.push_back(pArtefact);
				break;
			default:
				debug( LOG_ERROR, "Unknown research list" );
				abort();
				return false;
		}
		//deal with extra data
		switch (listNumber)
		{
			case RED_LIST:
				//ignore the last character
				sscanf(pArteData,",%*d");
				break;
			case RES_LIST:
				ArteName[0] = '\0';
				TypeName[0] = '\0';
				sscanf(pArteData, "%255[^,'\r\n],%255[^,'\r\n],%*d", ArteName, TypeName);
				if (!strcmp(ArteName, "0"))
				{
					pResearch->pReplacedArtefacts.push_back(NULL);
				}
				else
				{
					pArtefact = getComponentDetails(TypeName, ArteName);
					if (pArtefact == NULL)
					{
						return false;
					}
					//check the old and new types are the same
					if (statType(pArtefact->ref) != newType)
					{
						debug( LOG_ERROR, "You are trying to replace one type of component with a different type for research %s in ResultComponents.txt", ResearchName );
						abort();
						return false;
					}
					// ArtefactResearch found - alloc the artefact to the current Research topic
					pResearch->pReplacedArtefacts.push_back(pArtefact);
				}
				break;
			default:
				debug( LOG_ERROR, "Unknown research list" );
				abort();
				return false;
		}
		//increment the pointer to the start of the next record
		pArteData = strchr(pArteData,'\n') + 1;
	}

	return true;
}

//Load the Structures for a research list
bool loadResearchStructures(const char *pStructData, UDWORD bufferSize,UDWORD listNumber)
{
	unsigned NumToAlloc = numCR(pStructData, bufferSize);
	unsigned int i = 0;
	char				ResearchName[MAX_STR_LENGTH], StructureName[MAX_STR_LENGTH];
	UWORD				incR, incS;
	STRUCTURE_STATS		*pStructure = asStructureStats;
	bool				recFound;

	// Skip descriptive header
	if (strncmp(pStructData, "Research ", 9) == 0)
	{
		pStructData = strchr(pStructData, '\n') + 1;
		NumToAlloc--;
	}

	for (i = 0; i < NumToAlloc; i++)
	{
		recFound = false;

		//read the data into the storage - the data is delimited using comma's
		ResearchName[0] = '\0';
		StructureName[0] = '\0';
		sscanf(pStructData,"%255[^,'\r\n],%255[^,'\r\n],%*d,%*d", ResearchName, StructureName);

		//loop through each Research to compare the name
		for (incR = 0; incR < asResearch.size(); incR++)
		{
			if (!(strcmp(ResearchName, asResearch[incR].pName)))
			{
				//Research found
				for (incS = 0; incS < numStructureStats; incS++)
				{
					if (!(strcmp(StructureName, pStructure[incS].pName)))
					{
						//Structure found - alloc this to the current Research
						switch (listNumber)
						{
							case REQ_LIST:
								asResearch[incR].pStructList.push_back(incS);
								break;
							case RED_LIST:
								asResearch[incR].pRedStructs.push_back(incS);
								break;
							case RES_LIST:
								asResearch[incR].pStructureResults.push_back(incS);
								break;
							default:
								/* NO DEFAULT CASE? Alex.... Here ya go - just for you...*/
								debug( LOG_FATAL, "Unknown research list" );
								abort();
								return false;
						}
						recFound = true;
						break;
					}
				}
				//if Structure not found - error
				if (!recFound)
				{
					debug(LOG_FATAL, "Unable to find Structure %s for research %s", StructureName, ResearchName);
					return false;
				}
				else
				{
					break;
				}
			}
		}
		//if Research not found - error
		if (!recFound)
		{
			debug(LOG_FATAL, "Unable to allocate all Research Structures for %s", ResearchName);
			return false;
		}
		//increment the pointer to the start of the next record
		pStructData = strchr(pStructData,'\n') + 1;
	}

	return true;
}

//Load the pre-requisites for a research list
bool loadResearchFunctions(const char *pFunctionData, UDWORD bufferSize)
{
	unsigned NumToAlloc = numCR(pFunctionData, bufferSize);
	unsigned int i = 0;
	char				ResearchName[MAX_STR_LENGTH], FunctionName[MAX_STR_LENGTH];
	UDWORD				incR, incF;
	FUNCTION			**pFunction = asFunctions;
	bool				recFound;

	// Skip descriptive header
	if (strncmp(pFunctionData, "Research ", 9) == 0)
	{
		pFunctionData = strchr(pFunctionData, '\n') + 1;
		NumToAlloc--;
	}

	for (i=0; i < NumToAlloc; i++)
	{
		recFound = false;
		//read the data into the storage - the data is delimited using comma's
		ResearchName[0] = '\0';
		FunctionName[0] = '\0';
		sscanf(pFunctionData,"%255[^,'\r\n],%255[^,'\r\n],%*d", ResearchName, FunctionName);

		//loop through each Research to compare the name
		for (incR=0; incR < asResearch.size(); incR++)
		{
			if (!(strcmp(ResearchName, asResearch[incR].pName)))
			{
				//Research found
				for (incF=0; incF < numFunctions; incF++)
				{
					if (!(strcmp(FunctionName, (*pFunction[incF]).pName)))
					{
						// Function found alloc this to the current Research
						asResearch[incR].pFunctionList.push_back(pFunction[incF]);
						recFound = true;
						break;
					}
				}
				//if Function not found - error
				if (!recFound)
				{
					debug( LOG_ERROR, "Unable to find Function %s for research %s", FunctionName, ResearchName );
					abort();
					return false;
				}
				else
				{
					break;
				}
			}
		}
		//if Research not found - error
		if (!recFound)
		{
			debug( LOG_ERROR, "Unable to allocate all research Functions for %s", ResearchName );
			abort();
			return false;
		}

		//increment the pointer to the start of the next record
		pFunctionData = strchr(pFunctionData,'\n') + 1;
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
	UDWORD						type, inc;
	STRUCTURE					*psCurr;
	DROID						*psDroid;
	FUNCTION					*pFunction;
	UDWORD						compInc;
	MESSAGE						*pMessage;
	//the message gets sent to console
	char						consoleMsg[MAX_RESEARCH_MSG_SIZE];

	ASSERT_OR_RETURN( , researchIndex < asResearch.size(), "Invalid research index %u", researchIndex);

	MakeResearchCompleted(&asPlayerResList[player][researchIndex]);

	//check for structures to be made available
	for (inc = 0; inc < pResearch->pStructureResults.size(); inc++)
	{
		if (apStructTypeLists[player][pResearch->pStructureResults[inc]] != REDUNDANT)
		{
			apStructTypeLists[player][pResearch->pStructureResults[inc]] = AVAILABLE;
		}
	}

	//check for structures to be made redundant
	for (inc = 0; inc < pResearch->pRedStructs.size(); inc++)
	{
		apStructTypeLists[player][pResearch->pRedStructs[inc]] = REDUNDANT;
	}

	//check for artefacts to be made available
	for (inc = 0; inc < pResearch->pArtefactResults.size(); inc++)
	{
		//determine the type of artefact
		type = statType(pResearch->pArtefactResults[inc]->ref);
		//set the component state to AVAILABLE
		compInc = pResearch->pArtefactResults[inc]->ref - statRefStart(type);
		if (apCompLists[player][type][compInc] != REDUNDANT)
		{
			apCompLists[player][type][compInc] = AVAILABLE;
		}
		//check for default sensor
		if (type == COMP_SENSOR)
		{
			if ((asSensorStats + compInc)->location == LOC_DEFAULT)
			{
				aDefaultSensor[player] = compInc;
			}
		}
		//check for default ECM
		if (type == COMP_ECM)
		{
			if ((asECMStats + compInc)->location == LOC_DEFAULT)
			{
				aDefaultECM[player] = compInc;
			}
		}
		//check for default Repair
		if (type == COMP_REPAIRUNIT)
		{
			if ((asRepairStats + compInc)->location == LOC_DEFAULT)
			{
				aDefaultRepair[player] = compInc;
			}
		}
		//check if this component replaces an 'older' component
		if (pResearch->pReplacedArtefacts[inc] != NULL)
		{
			replaceComponent(pResearch->pArtefactResults[inc], pResearch->pReplacedArtefacts[inc], player);
			//set the 'old' component to unavailable
			type = statType(pResearch->pReplacedArtefacts[inc]->ref);
			//set the component state to REDUNDANT
			compInc = pResearch->pReplacedArtefacts[inc]->ref - statRefStart(type);
			apCompLists[player][type][compInc] = REDUNDANT;
		}
		//check if self repair has come on line
		if (type == COMP_REPAIRUNIT)
		{
			if (asRepairStats[compInc].location == LOC_DEFAULT)
			{
				enableSelfRepair(player);
			}
		}
	}

	//check for artefacts to be made redundant
	for (inc = 0; inc < pResearch->pRedArtefacts.size(); inc++)
	{
		// determine the type of artefact
		type = statType(pResearch->pRedArtefacts[inc]->ref);
		// set the component state to REDUNDANT
		apCompLists[player][type][pResearch->pRedArtefacts[inc]->ref - statRefStart(type)] = REDUNDANT;
	}

	//check for technology effects
	for (inc = 0; inc < pResearch->pFunctionList.size(); inc++)
	{
		pFunction = pResearch->pFunctionList[inc];

		switch (pFunction->type)
		{
			case(PRODUCTION_UPGRADE_TYPE):
				productionUpgrade(pFunction, player);
				// search the list of players structures for a Factory
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					if ((psCurr->pStructureType->type == REF_FACTORY &&
						((PRODUCTION_UPGRADE_FUNCTION *)pFunction)->factory) ||
						(psCurr->pStructureType->type == REF_CYBORG_FACTORY &&
						((PRODUCTION_UPGRADE_FUNCTION *)pFunction)->cyborgFactory) ||
						(psCurr->pStructureType->type == REF_VTOL_FACTORY &&
						((PRODUCTION_UPGRADE_FUNCTION *)pFunction)->vtolFactory))
					{
						// upgrade the Output for the structure
						structureProductionUpgrade(psCurr);
					}
				}
				// and the mission structures
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					if ((psCurr->pStructureType->type == REF_FACTORY &&
						((PRODUCTION_UPGRADE_FUNCTION *)pFunction)->factory) ||
						(psCurr->pStructureType->type == REF_CYBORG_FACTORY &&
						((PRODUCTION_UPGRADE_FUNCTION *)pFunction)->cyborgFactory) ||
						(psCurr->pStructureType->type == REF_VTOL_FACTORY &&
						((PRODUCTION_UPGRADE_FUNCTION *)pFunction)->vtolFactory))
					{
						// upgrade the Output for the structure
						structureProductionUpgrade(psCurr);
					}
				}
			   	// message/sound in here for production boost
				break;
			case(RESEARCH_UPGRADE_TYPE):
				researchUpgrade(pFunction, player);
				//search the list of players structures for a Research Facility
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_RESEARCH)
					{
						// upgrade the research points
						structureResearchUpgrade(psCurr);
					}
				}
				// and the mission structures
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_RESEARCH)
					{
						// upgrade the research points
						structureResearchUpgrade(psCurr);
					}
				}
				// Stuff a message in here/sound whatever for research boost.
				break;
			case(POWER_UPGRADE_TYPE):
				powerUpgrade(pFunction, player);
				// search the list of players structures for a Power Gens
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_POWER_GEN)
					{
						// upgrade the power points
						structurePowerUpgrade(psCurr);
					}
				}
				// and the mission structure
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_POWER_GEN)
					{
						// upgrade the power points
						structurePowerUpgrade(psCurr);
					}
				}
				break;
			case(REARM_UPGRADE_TYPE):
				reArmUpgrade(pFunction, player);
				// search the list of players structures for a ReArm pad
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_REARM_PAD)
					{
						// upgrade the rearm points
						structureReArmUpgrade(psCurr);
					}
				}
				// and the mission structure
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_REARM_PAD)
					{
						// upgrade the rearm points
						structureReArmUpgrade(psCurr);
					}
				}
				break;
			case(REPAIR_UPGRADE_TYPE):
				repairFacUpgrade(pFunction, player);
				// search the list of players structures for a Power Gens
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_REPAIR_FACILITY)
					{
						// upgrade the repair points
						structureRepairUpgrade(psCurr);
					}
				}
				// and the mission structure
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_REPAIR_FACILITY)
					{
						// upgrade the repair points
						structureRepairUpgrade(psCurr);
					}
				}
				break;
			case(WEAPON_UPGRADE_TYPE):
				// for the current player, upgrade the weapon stats
				weaponUpgrade(pFunction, player);
				// message/sound for weapon upgrade
				break;
			case(DROIDSENSOR_UPGRADE_TYPE):
				// for the current player, upgrade the sensor stats
				sensorUpgrade(pFunction, player);
				// for each structure in the player's list, upgrade the sensor stat
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					structureSensorUpgrade(psCurr);
					visTilesUpdate(psCurr);
				}
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					structureSensorUpgrade(psCurr);
				}
				// for each droid in the player's list, upgrade the sensor stat
				for (psDroid = apsDroidLists[player]; psDroid != NULL; psDroid = psDroid->psNext)
				{
					droidSensorUpgrade(psDroid);
					visTilesUpdate(psDroid);
					if (isTransporter(psDroid))
					{
						upgradeTransporterDroids(psDroid, droidSensorUpgrade);
					}
				}
				for (psDroid = mission.apsDroidLists[player]; psDroid != NULL; psDroid = psDroid->psNext)
				{
					droidSensorUpgrade(psDroid);
					if (isTransporter(psDroid))
					{
						upgradeTransporterDroids(psDroid, droidSensorUpgrade);
					}
				}
				for (psDroid = apsLimboDroids[player]; psDroid != NULL; psDroid = psDroid->psNext)
				{
					droidSensorUpgrade(psDroid);
					if (isTransporter(psDroid))
					{
						upgradeTransporterDroids(psDroid, droidSensorUpgrade);
					}
				}
				// message/sound for sensor upgrade
				break;
			case(DROIDECM_UPGRADE_TYPE):
				// for the current player, upgrade the ecm stats
				ecmUpgrade(pFunction, player);
				// for each structure in the player's list, upgrade the ecm stat
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					structureECMUpgrade(psCurr);
				}
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					structureECMUpgrade(psCurr);
				}
				// for each droid in the player's list, upgrade the ecm stat
				for (psDroid = apsDroidLists[player]; psDroid != NULL; psDroid = psDroid->psNext)
				{
					droidECMUpgrade(psDroid);
					if (isTransporter(psDroid))
					{
						upgradeTransporterDroids(psDroid, droidECMUpgrade);
					}
				}
				for (psDroid = mission.apsDroidLists[player]; psDroid != NULL; psDroid = psDroid->psNext)
				{
					droidECMUpgrade(psDroid);
					if (isTransporter(psDroid))
					{
						upgradeTransporterDroids(psDroid, droidECMUpgrade);
					}
				}
				for (psDroid = apsLimboDroids[player]; psDroid != NULL; psDroid = psDroid->psNext)
				{
					droidECMUpgrade(psDroid);
					if (isTransporter(psDroid))
					{
						upgradeTransporterDroids(psDroid, droidECMUpgrade);
					}
				}
				// message/sound for ecm upgrade
				break;
			case(DROIDREPAIR_UPGRADE_TYPE):
				// for the current player, upgrade the repair stats
				repairUpgrade(pFunction, player);
				// message/sound for repair upgrade
				break;
			case(DROIDCONST_UPGRADE_TYPE):
				// for the current player, upgrade the constructor stats
				constructorUpgrade(pFunction, player);
				// message/sound for constructor upgrade
				break;
			case(DROIDBODY_UPGRADE_TYPE):
				// for each droid in the player's list, upgrade the body points
				for (psDroid = apsDroidLists[player]; psDroid != NULL; psDroid = psDroid->psNext)
				{
					droidBodyUpgrade(pFunction, psDroid);
				}
				for (psDroid = mission.apsDroidLists[player]; psDroid != NULL; psDroid = psDroid->psNext)
				{
					droidBodyUpgrade(pFunction, psDroid);
					if (isTransporter(psDroid))
					{
						upgradeTransporterDroids(psDroid, droidSensorUpgrade);
					}
				}
				for (psDroid = apsLimboDroids[player]; psDroid != NULL; psDroid = psDroid->psNext)
				{
					droidBodyUpgrade(pFunction, psDroid);
					if (isTransporter(psDroid))
					{
						upgradeTransporterDroids(psDroid, droidSensorUpgrade);
					}
				}
				// DO THIS AFTER so above calculations can use the previous upgrade values for
				// the current player, upgrade the body stats
				bodyUpgrade(pFunction, player);
				// message/sound for body upgrade
				break;
			case(STRUCTURE_UPGRADE_TYPE):
				// for each structure in the player's list, upgrade the stats
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					//do this for none wallDefense structs
					if (!wallDefenceStruct(psCurr->pStructureType))
					{
						structureBodyUpgrade(pFunction, psCurr);
						structureArmourUpgrade(pFunction, psCurr);
						structureResistanceUpgrade(pFunction, psCurr);
					}
					// Defense type can have resistance upgrade now - AB 19/02/99
					if (psCurr->pStructureType->type == REF_DEFENSE)
					{
						structureResistanceUpgrade(pFunction, psCurr);
					}
				}
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					//do this for none wallDefense structs
					if (!wallDefenceStruct(psCurr->pStructureType))
					{
						structureBodyUpgrade(pFunction, psCurr);
						structureArmourUpgrade(pFunction, psCurr);
						structureResistanceUpgrade(pFunction, psCurr);
					}
					// Defense type can have resistance upgrade now - AB 19/02/99
					if (psCurr->pStructureType->type == REF_DEFENSE)
					{
						structureResistanceUpgrade(pFunction, psCurr);
					}
				}
				// DO THIS AFTER so above calculations can use the previous upgrade values
				// for the current player, upgrade the structure stats
				structureUpgrade(pFunction, player);
				// message/sound for structure upgrade
				break;
			case(WALLDEFENCE_UPGRADE_TYPE):
				//for each structure in the player's list, upgrade the stats
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					// do this for wallDefense structs
					if (wallDefenceStruct(psCurr->pStructureType))
					{
						structureBodyUpgrade(pFunction, psCurr);
						structureArmourUpgrade(pFunction, psCurr);
					}
				}
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr = psCurr->psNext)
				{
					// do this for wallDefense structs
					if (wallDefenceStruct(psCurr->pStructureType))
					{
						structureBodyUpgrade(pFunction, psCurr);
						structureArmourUpgrade(pFunction, psCurr);
					}
				}
				// DO THIS AFTER so above calculations can use the previous upgrade values
				// for the current player, upgrade the wall/defence structure stats
				wallDefenceUpgrade(pFunction, player);
				// message/sound for wall/defence structure upgrade
				break;
		//	default:
		//		ASSERT(false,"Invalid function type");
		//		break;
		}//end of switch
	}//end of function loop

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
	else
	{
		if ((player == selectedPlayer) && bDisplay)
		{
			audio_QueueTrack(ID_SOUND_RESEARCH_COMPLETED);
		}
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

		triggerEventResearched(pResearch, psResearchFacility, player);
	}

#ifdef DEBUG
    /*this just checks that there are not more than 32 weapons now available for
    the design screen - it'll give us grief in the design screen (which we may HAVE TO fix)!*/
    //only check if selectedPlayer has done the research
    if (player == selectedPlayer)
    {
        UDWORD    vtolCompInc;

        compInc = vtolCompInc =0;
	    for (inc = 0; inc < numWeaponStats; inc++)
	    {
		    if (apCompLists[selectedPlayer][COMP_WEAPON][inc] == AVAILABLE &&
                asWeaponStats[inc].designable)
            {
                if (asWeaponStats[inc].vtolAttackRuns)
                {
                    vtolCompInc++;
                }
                else
                {
                    compInc++;
                }
                if (compInc >= 128)
                {
					debug(LOG_ERROR, "researchResult - more than 128 weapons now available");
                   
					//don't bother checking any more
                    break;
                }
                if (vtolCompInc >= 128)
                {
					debug(LOG_ERROR, "researchResult - more than 128 vtol weapons now available");
                    
					//don't bother checking any more
                    break;
                }
            }
	    }
    }
#endif
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
static UWORD setIconID(char *pIconName, char *pName)
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

	return 0;	// Should never get here.
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

/* returns a pointer to a component based on the name - used to load in the research */
COMPONENT_STATS * getComponentDetails(char *pName, char *pCompName)
{
	UDWORD stat, size, quantity, inc;
	COMPONENT_STATS		*pArtefact;

	stat = componentType(pName);
	//get the stat list
	switch (stat)
	{
		case COMP_BODY:
		{
			pArtefact = (COMPONENT_STATS*)asBodyStats;
			size = sizeof(BODY_STATS);
			quantity = numBodyStats;
			break;
		}
		case COMP_BRAIN:
		{
			pArtefact = (COMPONENT_STATS*)asBrainStats;
			size = sizeof(BRAIN_STATS);
			quantity = numBrainStats;
			break;
		}
		case COMP_PROPULSION:
		{
			pArtefact = (COMPONENT_STATS*)asPropulsionStats;
			size = sizeof(PROPULSION_STATS);
			quantity = numPropulsionStats;
			break;
		}
		case COMP_REPAIRUNIT:
		{
			pArtefact = (COMPONENT_STATS*)asRepairStats;
			size = sizeof(REPAIR_STATS);
			quantity = numRepairStats;
			break;
		}
		case COMP_ECM:
		{
			pArtefact = (COMPONENT_STATS*)asECMStats;
			size = sizeof(ECM_STATS);
			quantity = numECMStats;
			break;
		}
		case COMP_SENSOR:
		{
			pArtefact = (COMPONENT_STATS*)asSensorStats;
			size = sizeof(SENSOR_STATS);
			quantity = numSensorStats;
			break;
		}
		case COMP_WEAPON:
		{
			pArtefact = (COMPONENT_STATS*)asWeaponStats;
			size = sizeof(WEAPON_STATS);
			quantity = numWeaponStats;
			break;
		}
		case COMP_CONSTRUCT:
		{
			pArtefact = (COMPONENT_STATS*)asConstructStats;
			size = sizeof(CONSTRUCT_STATS);
			quantity = numConstructStats;
			break;
		}
		default:
		{
			//COMP_UNKNOWN should be an error
			debug( LOG_ERROR, "Unknown artefact type  - %s", pName );
			abort();
			return NULL;
		}
	}

	for (inc = 0; inc < quantity; inc++)
	{
		if (!strcmp(pArtefact->pName, pCompName))
		{
			return pArtefact;
		}
		pArtefact = (COMPONENT_STATS*)((char*)pArtefact + size);
	}

	debug( LOG_ERROR, "Cannot find component %s", pCompName );
	abort();
	return NULL;
}

//return a pointer to a research topic based on the name
RESEARCH *getResearch(const char *pName)
{
	unsigned int inc = 0;

	for (inc = 0; inc < asResearch.size(); inc++)
	{
		if (!strcasecmp(asResearch[inc].pName, pName))
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
	UDWORD			inc, oldType, newType, oldCompInc, newCompInc;

	//get the type and index of the old component
	oldType = statType(pOldComponent->ref);
	oldCompInc = pOldComponent->ref - statRefStart(oldType);
	//get the type and index of the new component
	newType = statType(pNewComponent->ref);
	newCompInc = pNewComponent->ref - statRefStart(newType);

	//check old and new type are the same
	if (oldType != newType)
	{
		return;
	}

	replaceDroidComponent(apsDroidLists[player], oldType, oldCompInc, newCompInc);
	replaceDroidComponent(mission.apsDroidLists[player], oldType, oldCompInc, newCompInc);
	replaceDroidComponent(apsLimboDroids[player], oldType, oldCompInc, newCompInc);

	//check thru the templates
	for (psTemplates = apsDroidTemplates[player]; psTemplates != NULL;
		psTemplates = psTemplates->psNext)
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
				for (inc=0; inc < psTemplates->numWeaps; inc++)
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
				abort();
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
	UDWORD inc;
	char *pName=psResearch->pName;

	for (inc = 0; inc < numStats; inc++)
	{

		if (!strcmp(asResearch[inc].pName, pName))
		{
			//oops! found the name
			ASSERT( false, "Research name has already been used - %s", pName );
			return false;
		}
	}
	return true;
}

/* Sets the 'possible' flag for a player's research so the topic will appear in
the research list next time the Research Facilty is selected */
bool enableResearch(RESEARCH *psResearch, UDWORD player)
{
	UDWORD				inc;

	inc = psResearch->index;
	if (inc > asResearch.size())
	{
		ASSERT( false, "enableResearch: Invalid research topic - %s", getResearchName(psResearch) );
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
				getName(asResearch[rewardID].pName)));
		}
	}
}


/*checks that the research has loaded up as expected - must be done after
all research parts have been loaded*/
bool checkResearchStats(void)
{
	UDWORD resInc, inc;
	for (resInc = 0; resInc < asResearch.size(); resInc++)
	{
		for (inc = 0; inc < asResearch[resInc].pPRList.size(); inc++)
		{
			ASSERT(asResearch[resInc].pPRList[inc] <= asResearch.size(), "Invalid PreReq for topic %s", asResearch[resInc].pName);
		}
		for (inc = 0; inc < asResearch[resInc].pStructList.size(); inc++)
		{
			ASSERT(asResearch[resInc].pStructList[inc] <= numStructureStats, "Invalid Structure for topic %s", asResearch[resInc].pName);
		}
		for (inc = 0; inc < asResearch[resInc].pFunctionList.size(); inc++)
		{
			ASSERT(asResearch[resInc].pFunctionList[inc]->ref - REF_FUNCTION_START <= numFunctions,
			       "Invalid function for %s", asResearch[resInc].pName);
		}
		for (inc = 0; inc < asResearch[resInc].pRedStructs.size(); inc++)
		{
			ASSERT(asResearch[resInc].pRedStructs[inc] <= numStructureStats,
			       "Invalid Redundant Structure for topic %s", asResearch[resInc].pName);
		}
		for (inc = 0; inc < asResearch[resInc].pStructureResults.size(); inc++)
		{
			ASSERT(asResearch[resInc].pStructureResults[inc] <= numStructureStats,
			       "Invalid Result Structure for topic %s", asResearch[resInc].pName);
		}
		for (inc = 0; inc < asResearch[resInc].pArtefactResults.size(); inc++)
		{
			ASSERT(asResearch[resInc].pArtefactResults[inc] != NULL,
			       "Invalid Comp Result for topic %s", asResearch[resInc].pName);
		}
		for (inc = 0; inc < asResearch[resInc].pRedArtefacts.size(); inc++)
		{
			ASSERT(asResearch[resInc].pRedArtefacts[inc] != NULL,
			       "Invalid Redundant Comp for topic %s", asResearch[resInc].pName);
		}
	}

	return true;
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
	    || psStats->type == REF_WALLCORNER || psStats->type == REF_BLASTDOOR)
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
		if (isTransporter(psDroid))
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

	ASSERT(isTransporter(psTransporter), "invalid unit type");

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
			case COMP_ECM:
				objEcmCache((BASE_OBJECT *)psStructure, asECMStats + newCompInc);
				break;
			case COMP_SENSOR:
				objSensorCache((BASE_OBJECT *)psStructure, asSensorStats + newCompInc);
				break;
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
			if (psDroid->asBits[oldType].nStat == oldCompInc)
			{
				psDroid->asBits[oldType].nStat = (UBYTE)newCompInc;
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
			if (player == selectedPlayer || !aiCheckAlliances(selectedPlayer, player))
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
					r.timeToResearch = (subject.researchPoints - playerRes.currentPoints) / std::max(res->researchPoints, 1u);
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
