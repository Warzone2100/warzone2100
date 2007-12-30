/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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

//used to calc the research power
#define RESEARCH_FACTOR		32
#define RESEARCH_MAX_POWER  450

// The stores for the research stats
RESEARCH                *asResearch;
UDWORD					numResearch;

//used for Callbacks to say which topic was last researched
RESEARCH                *psCBLastResearch;
STRUCTURE				*psCBLastResStructure;
SDWORD					CBResFacilityOwner;

//research is now loaded per campaign - this hopefully is the max there will be in any one campaign!
#define MAX_RESEARCH        (450)

//need to define max's for each of the lists associated with the research - these
//values have been chosen based on the current research stats - 21/12/98
#define MAX_RESEARCH_PR             (650)
#define MAX_RESEARCH_STRUCT_PR      (44 + 5)
#define MAX_RESEARCH_FUNC           (250 + 25)
#define MAX_RESEARCH_STRUCT_RED     (30 + 2)
#define MAX_RESEARCH_ARTE_RED       (40 + 5)
#define MAX_RESEARCH_STRUCT_RES     (84 + 5)
#define MAX_RESEARCH_ARTE_RES       (125 + 5)

//need corresponding arrays for the above
static UWORD*            pResearchPR;
static UWORD*            pResearchStructPR;
static FUNCTION**        pResearchFunc;
static UWORD*            pResearchStructRed;
static COMP_BASE_STATS** pResearchArteRed;
static UWORD*            pResearchStructRes;
static COMP_BASE_STATS** pResearchArteRes;
static COMP_BASE_STATS** pResearchArteRep;

static UWORD numResearchPR;
static UWORD numResearchStructPR;
static UWORD numResearchFunc;
static UWORD numResearchStructRed;
static UBYTE numResearchArteRed;
static UWORD numResearchStructRes;
static UBYTE numResearchArteRes;
static UBYTE numResearchArteRep;

//List of pointers to arrays of PLAYER_RESEARCH[numResearch] for each player
PLAYER_RESEARCH*		asPlayerResList[MAX_PLAYERS];

/* Default level of sensor, Repair and ECM */
UDWORD					aDefaultSensor[MAX_PLAYERS];
UDWORD					aDefaultECM[MAX_PLAYERS];
UDWORD					aDefaultRepair[MAX_PLAYERS];

//set the iconID based on the name read in in the stats
static UWORD setIconID(char *pIconName, char *pName);
static COMP_BASE_STATS * getComponentDetails(char *pName, char *pCompName);
static void replaceComponent(COMP_BASE_STATS *pNewComponent, COMP_BASE_STATS *pOldComponent,
					  UBYTE player);
static BOOL checkResearchName(RESEARCH *psRes, UDWORD numStats);

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


BOOL researchInitVars(void)
{
	int i;

	psCBLastResearch = NULL;
	psCBLastResStructure = NULL;
	CBResFacilityOwner = -1;
	asResearch = NULL;
    //research is a pre-defined size now
	asResearch = (RESEARCH *)malloc(sizeof(RESEARCH)* MAX_RESEARCH);
	if (asResearch == NULL)
	{
		debug( LOG_ERROR, "Research Stats - Out of memory" );
		abort();
		return FALSE;
	}
	memset(asResearch, 0, (MAX_RESEARCH * sizeof(RESEARCH)));

	// create the PLAYER_RESEARCH arrays
	for (i=0; i < MAX_PLAYERS; i++)
	{
		asPlayerResList[i] = (PLAYER_RESEARCH*)malloc(MAX_RESEARCH *
			sizeof(PLAYER_RESEARCH));
		if (asPlayerResList[i] == NULL)
		{
			debug( LOG_ERROR, "Out of memory assigning Player_Research" );
			abort();
			return FALSE;
		}
		memset(asPlayerResList[i], 0, (MAX_RESEARCH * sizeof(PLAYER_RESEARCH)));
	}

	numResearch = 0;

    //and deal with all the other arrays for research
    //needs to be UWORD sized for the Patches
    pResearchPR = (UWORD *) malloc(sizeof(UWORD) * MAX_RESEARCH_PR);
    //pResearchPR = (UBYTE *) malloc(sizeof(UBYTE) * MAX_RESEARCH_PR);
	if (pResearchPR == NULL)
	{
		debug( LOG_ERROR, "Research Stats - Out of memory" );
		abort();
		return FALSE;
	}
    //needs to be UWORD sized for the Patches
    memset(pResearchPR, 0, (MAX_RESEARCH_PR * sizeof(UWORD)));
    //memset(pResearchPR, 0, (MAX_RESEARCH_PR * sizeof(UBYTE)));

    pResearchStructPR = (UWORD *) malloc(sizeof(UWORD) * MAX_RESEARCH_STRUCT_PR);
	if (pResearchStructPR == NULL)
	{
		debug( LOG_ERROR, "Research Stats - Out of memory" );
		abort();
		return FALSE;
	}
    memset(pResearchStructPR, 0, (MAX_RESEARCH_STRUCT_PR * sizeof(UWORD)));

    pResearchFunc = (FUNCTION **) malloc(sizeof(FUNCTION *) * MAX_RESEARCH_FUNC);
	if (pResearchFunc == NULL)
	{
		debug( LOG_ERROR, "Research Stats - Out of memory" );
		abort();
		return FALSE;
	}
    memset(pResearchFunc, 0, (MAX_RESEARCH_FUNC * sizeof(FUNCTION *)));

    pResearchStructRed = (UWORD *) malloc(sizeof(UWORD) * MAX_RESEARCH_STRUCT_RED);
	if (pResearchStructRed == NULL)
	{
		debug( LOG_ERROR, "Research Stats - Out of memory" );
		abort();
		return FALSE;
	}
    memset(pResearchStructRed, 0, (MAX_RESEARCH_STRUCT_RED * sizeof(UWORD)));

    pResearchArteRed = (COMP_BASE_STATS **) malloc(sizeof(COMP_BASE_STATS *) * MAX_RESEARCH_ARTE_RED);
	if (pResearchArteRed == NULL)
	{
		debug( LOG_ERROR, "Research Stats - Out of memory" );
		abort();
		return FALSE;
	}
    memset(pResearchArteRed, 0, (MAX_RESEARCH_ARTE_RED * sizeof(COMP_BASE_STATS *)));

    pResearchStructRes = (UWORD *) malloc(sizeof(UWORD) * MAX_RESEARCH_STRUCT_RES);
	if (pResearchStructRes == NULL)
	{
		debug( LOG_ERROR, "Research Stats - Out of memory" );
		abort();
		return FALSE;
	}
    memset(pResearchStructRes, 0, (MAX_RESEARCH_STRUCT_RES * sizeof(UWORD)));

    pResearchArteRes = (COMP_BASE_STATS **) malloc(sizeof(COMP_BASE_STATS *) * MAX_RESEARCH_ARTE_RES);
	if (pResearchArteRes == NULL)
	{
		debug( LOG_ERROR, "Research Stats - Out of memory" );
		abort();
		return FALSE;
	}
    memset(pResearchArteRes, 0, (MAX_RESEARCH_ARTE_RES * sizeof(COMP_BASE_STATS *)));

    pResearchArteRep = (COMP_BASE_STATS **) malloc(sizeof(COMP_BASE_STATS *) * MAX_RESEARCH_ARTE_RES);
	if (pResearchArteRep == NULL)
	{
		debug( LOG_ERROR, "Research Stats - Out of memory" );
		abort();
		return FALSE;
	}
    memset(pResearchArteRep, 0, (MAX_RESEARCH_ARTE_RES * sizeof(COMP_BASE_STATS *)));

	for(i=0; i<MAX_PLAYERS; i++)
    {
		//asPlayerResList[i] = NULL;
		bSelfRepair[i] = FALSE;
	}

	return TRUE;
}


/*Load the research stats from the file exported from Access*/
BOOL loadResearch(const char *pResearchData, UDWORD bufferSize)
{
	const unsigned int researchCount = numCR(pResearchData, bufferSize);
	RESEARCH *pResearch;
	COMP_BASE_STATS *psComp;
	SDWORD structID;
	UDWORD i, keyTopic, techCode, resPoints;
	char ResearchName[MAX_NAME_SIZE];
	char msgName[MAX_STR_SIZE], iconID[MAX_NAME_SIZE];
	char imdName[MAX_NAME_SIZE], imdName2[MAX_NAME_SIZE];
	char structName[MAX_NAME_SIZE], compName[MAX_NAME_SIZE],
		compType[MAX_STR_SIZE];

	numResearch = researchCount;

	ASSERT(numResearch <= MAX_RESEARCH, "Too many ResearchStats! - max allowed %d", MAX_RESEARCH);

	//init all the counts
	numResearchPR = numResearchFunc = numResearchArteRed = numResearchArteRes = numResearchArteRep = 0;
	numResearchStructPR = numResearchStructRed = numResearchStructRes = 0;

	//get the start of the research storage
	pResearch = asResearch;

	for (i = 0; i < researchCount; i++)
	{
		memset(pResearch, 0, sizeof(RESEARCH));

		//read the data into the storage - the data is delimeted using comma's
		ResearchName[0] = '\0';
		sscanf(pResearchData,"%[^','],", ResearchName);

		//allocate storage for the name
		if (!allocateName(&pResearch->pName, ResearchName))
		{
			return FALSE;
		}

		//check the name hasn't been used already
		if (!checkResearchName(pResearch, i))
		{
			return FALSE;
		}

		pResearchData += (strlen(ResearchName)+1);

		pResearch->ref = REF_RESEARCH_START + i;

		//determine the tech level
		ResearchName[0] = '\0';
		sscanf(pResearchData,"%[^','],", ResearchName);
		if (!setTechLevel((BASE_STATS *)pResearch, ResearchName))
		{
			return FALSE;
		}

		pResearchData += (strlen(ResearchName)+1);

		ResearchName[0] = '\0';
		sscanf(pResearchData,"%[^','],", ResearchName);

		if (strcmp(ResearchName, "0"))
		{
			pResearch->subGroup = setIconID(ResearchName, pResearch->pName);
		}
		else
		{
			pResearch->subGroup = NO_RESEARCH_ICON;
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

			sscanf(pResearchData,"%d,%[^','],%[^','],%[^','],%[^','],%[^','], \
                %[^','],%[^','],%d,%d,%d,%d,%d,%d,%d,%d,%d",
				&techCode, iconID, imdName, imdName2, msgName,
                structName, compName, compType,
				&resPoints, &keyTopic, &numPRRequired,
				&numFunctions, &numStructures,
				&numRedStructs, &numStructResults,
				&numRedArtefacts, &numArteResults);

			pResearch->numPRRequired = (UBYTE)numPRRequired;
			pResearch->numFunctions = (UBYTE)numFunctions;
			pResearch->numStructures = (UBYTE)numStructures;
			pResearch->numRedStructs = (UBYTE)numRedStructs;

			pResearch->numStructResults = (UBYTE)numStructResults;
			pResearch->numRedArtefacts = (UBYTE)numRedArtefacts;
			pResearch->numArteResults = (UBYTE)numArteResults;
		}

		//set keytopic flag
		if (keyTopic)
		{
			pResearch->keyTopic = TRUE;
		}
		else
		{
			pResearch->keyTopic = FALSE;
		}

		//check the tech code is valid
		if (techCode > 1)
		//if (pResearch->techCode != TC_MAJOR && pResearch->techCode != TC_MINOR)
		{
			debug( LOG_ERROR, "Invalid tech code for research topic - %s ", getResearchName(pResearch) );
			abort();
			return FALSE;
		}
        if (techCode == 0)
        {
            pResearch->techCode = TC_MAJOR;
        }
        else
        {
            pResearch->techCode = TC_MINOR;
        }

		//set the iconID
		if (strcmp(iconID, "0"))
		{
			pResearch->iconID = setIconID(iconID, pResearch->pName);
		}
		else
		{
			pResearch->iconID = NO_RESEARCH_ICON;
		}

		//get the IMDs used in the interface
		if (strcmp(structName, "0"))
		{
			//find the structure stat
			structID = getStructStatFromName(structName);
			if (structID >= 0)
			{
				pResearch->psStat = (BASE_STATS *)(asStructureStats + structID);
			}
			else
			{
				debug( LOG_ERROR, "Cannot find the structure Stat for Research %s", getResearchName(pResearch) );
				abort();
				return FALSE;
			}
		}
		else if (strcmp(compName, "0"))
		{
			//find the component stat
			psComp = getComponentDetails(compType, compName);
			if (psComp != NULL)
			{
				pResearch->psStat = (BASE_STATS *)psComp;
			}
			else
			{
				debug( LOG_ERROR, "Cannot find the component Stat for Research %s", getResearchName(pResearch) );
				abort();
				return FALSE;
			}
		}
		else
		{
			pResearch->psStat = NULL;
		}
		if (strcmp(imdName, "0"))
		{
			pResearch->pIMD = (iIMDShape *) resGetData("IMD", imdName);
			if (pResearch->pIMD == NULL)
			{
				debug( LOG_ERROR, "Cannot find the research PIE for record %s", getResearchName(pResearch) );
				abort();
				return FALSE;
			}
		}
		else
		{
			pResearch->pIMD = NULL;
		}

		if (strcmp(imdName2, "0"))
		{
			pResearch->pIMD2 = (iIMDShape *) resGetData("IMD", imdName2);
			if (pResearch->pIMD2 == NULL)
			{
				debug( LOG_ERROR, "Cannot find the 2nd research PIE for record %s", getResearchName(pResearch) );
				abort();
				return FALSE;
			}
		}
		else
		{
			pResearch->pIMD2 = NULL;
		}

		//get the message viewdata - if any
		if (strcmp(msgName, "0"))
		{
			//check its a major tech code
			if (pResearch->techCode != TC_MAJOR)
			{
				debug( LOG_ERROR, "This research should not have a message associated with it, %s the message will be ignored!", getResearchName(pResearch) );
				abort();
			}
			else
			{
				pResearch->pViewData = getViewData(msgName);
			}
		}

		//redundancies - artefacts
		if (pResearch->numRedArtefacts > 0)
		{
			/*pResearch->pRedArtefacts = (COMP_BASE_STATS **) malloc(pResearch->
				numRedArtefacts*sizeof(COMP_BASE_STATS *));
			if (pResearch->pRedArtefacts == NULL)
			{
				DBERROR(("Out of memory assigning research artefacts - redundancies"));
				return FALSE;
			}*/
			if (numResearchArteRed >= MAX_RESEARCH_ARTE_RED)
			{
				debug( LOG_ERROR, "Out of memory assigning research artefacts - redundancies" );
				abort();
				return FALSE;
			}
			//don't MALLOC - get them from the pre-defined arrays
			pResearch->pRedArtefacts = pResearchArteRed + numResearchArteRed;
			//pResearchArteRed += pResearch->numRedArtefacts;
			//keep track on how many are being allocated
			numResearchArteRed = (UBYTE)(numResearchArteRed + pResearch->numRedArtefacts);
		}
		//results
		if (pResearch->numArteResults > 0)
		{
			if (numResearchArteRed >= MAX_RESEARCH_ARTE_RES)
			{
				debug( LOG_ERROR, "Out of memory assigning research artefacts - results" );
				abort();
				return FALSE;
			}

			//don't MALLOC - get them from the pre-defined arrays
			pResearch->pArtefactResults = pResearchArteRes + numResearchArteRes;

			//keep track on how many are being allocated
			numResearchArteRes = (UBYTE)(numResearchArteRes + pResearch->numArteResults);
		}

		//replacements
		if (pResearch->numArteResults > 0)
		{
			if (numResearchArteRep >= MAX_RESEARCH_ARTE_RES)
			{
				debug( LOG_ERROR, "Out of memory assigning research artefacts - replacements" );
				abort();
				return FALSE;
			}

			//don't MALLOC - get them from the pre-defined arrays
			pResearch->pReplacedArtefacts = pResearchArteRep + numResearchArteRep;

			//keep track on how many are being allocated
			numResearchArteRep = (UBYTE)(numResearchArteRep + pResearch->numArteResults);
		}

		//allocate storage for the functions
		if (pResearch->numFunctions > 0)
		{
			if (numResearchFunc >= MAX_RESEARCH_FUNC)
			{
				debug( LOG_ERROR, "Out of memory assigning research functions" );
				abort();
				return FALSE;
			}

			//don't MALLOC - get them from the pre-defined arrays
			pResearch->pFunctionList = pResearchFunc + numResearchFunc;

			//keep track on how many are being allocated
			numResearchFunc = (numResearchFunc + pResearch->numFunctions);
		}

		//allocate storage for the pre-requisities
		if (pResearch->numPRRequired > 0)
		{
			if (numResearchPR >= MAX_RESEARCH_PR)
			{
				debug( LOG_ERROR, "Out of memory assigning research pre-requisities" );
				abort();
				return FALSE;
			}

			//don't MALLOC - get them from the pre-defined arrays
			pResearch->pPRList = pResearchPR + numResearchPR;

			//keep track on how many are being allocated
			numResearchPR = (UWORD)(numResearchPR + pResearch->numPRRequired);
		}

		//allocate storage for the structures
		//requirements
		if (pResearch->numStructures > 0)
		{
			if (numResearchStructPR >= MAX_RESEARCH_STRUCT_PR)
			{
				debug( LOG_ERROR, "Out of memory assigning research structures - requirements" );
				abort();
				return FALSE;
			}

			//don't MALLOC - get them from the pre-defined arrays
			pResearch->pStructList = pResearchStructPR + numResearchStructPR;

			//keep track on how many are being allocated
			numResearchStructPR = (UBYTE)(numResearchStructPR + pResearch->numStructures);
		}

        //redundancies
		if (pResearch->numRedStructs > 0)
		{
			if (numResearchStructRed >= MAX_RESEARCH_STRUCT_RED)
			{
				debug( LOG_ERROR, "Out of memory assigning research structures - redundancies" );
				abort();
				return FALSE;
			}
			//don't MALLOC - get them from the pre-defined arrays
			pResearch->pRedStructs = pResearchStructRed + numResearchStructRed;

			//keep track on how many are being allocated
			numResearchStructRed = (UBYTE)(numResearchStructRed + pResearch->numRedStructs);
		}
		//results
		if (pResearch->numStructResults > 0)
		{
			if (numResearchStructRes >= MAX_RESEARCH_STRUCT_RES)
			{
				debug( LOG_ERROR, "Out of memory assigning research structures - results" );
				abort();
				return FALSE;
			}

			//don't MALLOC - get them from the pre-defined arrays
			pResearch->pStructureResults = pResearchStructRes + numResearchStructRes;

			//keep track on how many are being allocated
			numResearchStructRes = (UBYTE)(numResearchStructRes + pResearch->numStructResults);
		}

		//set the researchPoints
		if (resPoints > UWORD_MAX)
		{
			debug( LOG_ERROR, "Research Points too high for research topic - %s ", getResearchName(pResearch) );
			abort();
			return FALSE;
		}
		pResearch->researchPoints = (UWORD)resPoints;

		//set the research power
		pResearch->researchPower = pResearch->researchPoints / RESEARCH_FACTOR;
		if (pResearch->researchPower > RESEARCH_MAX_POWER)
		{
			pResearch->researchPower = RESEARCH_MAX_POWER;
		}

		//increment the pointer to the start of the next record
		pResearchData = strchr(pResearchData,'\n') + 1;
		//increment the list to the start of the next storage block
		pResearch++;
	}

	return TRUE;
}


//Load the pre-requisites for a research list
BOOL loadResearchPR(const char *pPRData, UDWORD bufferSize)
{
	const unsigned int NumToAlloc = numCR(pPRData, bufferSize);
	unsigned int i = 0;
	char				ResearchName[MAX_NAME_SIZE], PRName[MAX_NAME_SIZE];
	UWORD				incR, incPR;
	RESEARCH			*pResearch = asResearch, *pPRResearch = asResearch;
	BOOL				recFound;

    //check not going to go over max
    ASSERT( NumToAlloc <= MAX_RESEARCH_PR, "loadResearchPR: too many!" );
    numResearchPR = 0;

	for (i = 0; i < NumToAlloc; i++)
	{
		recFound = FALSE;

		//read the data into the storage - the data is delimited using commas
		ResearchName[0] = '\0';
		PRName[0] = '\0';
		sscanf(pPRData,"%[^','],%[^','],%*d", ResearchName, PRName);

		if (!getResourceName(ResearchName))
		{
			return FALSE;
		}
		if (!getResourceName(PRName))
		{
			return FALSE;
		}

		//loop through each Research to compare the name
		for (incR=0; incR < numResearch; incR++)
		{
			if (!(strcmp(ResearchName, pResearch[incR].pName)))
			{
				//Research found
				for (incPR=0; incPR < numResearch; incPR++)
				{
					if (!(strcmp(PRName, pPRResearch[incPR].pName)))
					{
						//check not allocating more than allowed
						if ((pResearch[incR].storeCount + 1) >
										(SDWORD)pResearch[incR].numPRRequired)
						{
							debug( LOG_ERROR, "Trying to allocate more pre-requisites than allowed for research %s", ResearchName );
							abort();
							return FALSE;
						}
						//PRresearch found alloc this to the current Research
						pResearch[incR].pPRList[pResearch[incR].
							storeCount] = incPR;
                        //keep tab on how many we have loaded in
                        numResearchPR++;
						pResearch[incR].storeCount++;
						recFound = TRUE;
						break;
					}
				}
				//if pre-requisite not found - error
				if (!recFound)
				{
					debug( LOG_ERROR, "Unable to find Pre-requisite %s for research %s", PRName, ResearchName );
					abort();
					return FALSE;
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
			debug( LOG_ERROR, "Unable to find Research %s", ResearchName );
			abort();
			return FALSE;
		}
		//quick check that haven't reached maxPR
		if (numResearchPR >= MAX_RESEARCH_PR)
		{
			//don't load any more since will write over memory!
			break;
		}
		//increment the pointer to the start of the next record
		pPRData = strchr(pPRData,'\n') + 1;
	}

	return TRUE;
}

//Load the artefacts for a research list
BOOL loadResearchArtefacts(const char *pArteData, UDWORD bufferSize, UDWORD listNumber)
{
	const unsigned int NumToAlloc = numCR(pArteData, bufferSize);
	unsigned int i = 0;
	char				ResearchName[MAX_NAME_SIZE], ArteName[MAX_NAME_SIZE],
						TypeName[MAX_NAME_SIZE];
	RESEARCH			*pResearch = asResearch;
	COMP_BASE_STATS		*pArtefact;
	UDWORD				newType;
	UBYTE				maxArtefacts;

	//initialise the storage flags
	for (i = 0; i < numResearch; i++)
	{
		pResearch[i].storeCount = 0;
	}
	pResearch = asResearch;

    //check not going to go over max
	switch (listNumber)
	{
		case RED_LIST:
			ASSERT( NumToAlloc <= MAX_RESEARCH_ARTE_RED,
				"loadResearchArtefacts: too many Redundant Components" );
			numResearchArteRed = 0;
			break;
		case RES_LIST:
			ASSERT( NumToAlloc <= MAX_RESEARCH_ARTE_RES,
				"loadResearchArtefacts: too many Component Results" );
			numResearchArteRes = 0;
			numResearchArteRep = 0;
			break;
	}

	for (i = 0; i < NumToAlloc; i++)
	{
		//read the data into the storage - the data is delimited using commas
		ResearchName[0] = '\0';
		ArteName[0] = '\0';
		TypeName[0] = '\0';
		sscanf(pArteData,"%[^','],%[^','],%[^',']", ResearchName, ArteName, TypeName);

		//increment the data pointer
		pArteData += (strlen(ResearchName)+1+strlen(ArteName)+1+strlen(TypeName)+1);

		if (!getResourceName(ResearchName))
		{
			return FALSE;
		}
		if (!getResourceName(ArteName))
		{
			return FALSE;
		}

		pArtefact = getComponentDetails(TypeName, ArteName);
		if (pArtefact == NULL)
		{
			return FALSE;
		}
		//get the type for comparison later
		newType = statType(pArtefact->ref);

		pResearch = getResearch(ResearchName, FALSE);
		if (pResearch == NULL)
		{
			return FALSE;
		}

		//ArtefactResearch found - alloc the artefact to the current Research topic
		switch (listNumber)
		{
			case RED_LIST:
				pResearch->pRedArtefacts[pResearch->storeCount] = pArtefact;
				//keep tab on how many we have loaded in
				numResearchArteRed++;
				maxArtefacts = pResearch->numRedArtefacts;
				break;
			case RES_LIST:
				pResearch->pArtefactResults[pResearch->storeCount] = pArtefact ;
				//keep tab on how many we have loaded in
				numResearchArteRes++;
				maxArtefacts = pResearch->numArteResults;
				break;
			default:
				debug( LOG_ERROR, "Unknown research list" );
				abort();
				return FALSE;
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
				sscanf(pArteData, "%[^','],%[^','],%*d", ArteName, TypeName);
				if (!strcmp(ArteName, "0"))
				{
					pResearch->pReplacedArtefacts[pResearch->storeCount] =  NULL;
				}
				else
				{
					if (!getResourceName(ArteName))
					{
						return FALSE;
					}
					pArtefact = getComponentDetails(TypeName, ArteName);
					if (pArtefact == NULL)
					{
						return FALSE;
					}
					//check the old and new types are the same
					if (statType(pArtefact->ref) != newType)
					{
						debug( LOG_ERROR, "You are trying to replace one type of component with a different type for research %s in ResultComponents.txt", ResearchName );
						abort();
						return FALSE;
					}
					//ArtefactResearch found - alloc the artefact to the current Research topic
					pResearch->pReplacedArtefacts[pResearch->storeCount] = pArtefact;
					numResearchArteRep++;
				}
				break;
			default:
				debug( LOG_ERROR, "Unknown research list" );
				abort();
				return FALSE;
		}

		//check not allocating more than allowed
		if (pResearch->storeCount > maxArtefacts)
		{
			debug( LOG_ERROR, "Trying to allocate more artefacts than allowed for research %s", getResearchName(pResearch) );
			abort();
			return FALSE;
		}
		pResearch->storeCount++;

		//quick check that haven't reached maxArtes
		if (numResearchArteRed >= MAX_RESEARCH_ARTE_RED || numResearchArteRes >=
			MAX_RESEARCH_ARTE_RES || numResearchArteRep > MAX_RESEARCH_ARTE_RES)
		{
			//don't load any more since will write over memory!
			break;
		}
		//increment the pointer to the start of the next record
		pArteData = strchr(pArteData,'\n') + 1;
	}

	return TRUE;
}

//Load the Structures for a research list
BOOL loadResearchStructures(const char *pStructData, UDWORD bufferSize,UDWORD listNumber)
{
	const unsigned int NumToAlloc = numCR(pStructData, bufferSize);
	unsigned int i = 0;
	char				ResearchName[MAX_NAME_SIZE], StructureName[MAX_NAME_SIZE];
	UWORD				incR, incS;
	RESEARCH			*pResearch = asResearch;
	STRUCTURE_STATS		*pStructure = asStructureStats;
	BOOL				recFound;
	UDWORD				numToFind;

	//initialise the storage flags
	for (i = 0; i < numResearch; i++)
	{
		pResearch[i].storeCount = 0;
	}
	pResearch = asResearch;

	switch (listNumber)
	{
		case REQ_LIST:
			//check not going to go over max
			ASSERT( NumToAlloc <= MAX_RESEARCH_STRUCT_PR, "loadResearchStructures: too many Struct PRs" );
			numResearchStructPR = 0;
			break;
		case RED_LIST:
			//check not going to go over max
			ASSERT( NumToAlloc <= MAX_RESEARCH_STRUCT_RED, "loadResearchStructures: too many redundant structure" );
			numResearchStructRed = 0;
			break;
		case RES_LIST:
			//check not going to go over max
			ASSERT( NumToAlloc <= MAX_RESEARCH_STRUCT_RES, "loadResearchStructures: too many structure results" );
			numResearchStructRes = 0;
			break;
	}

	for (i = 0; i < NumToAlloc; i++)
	{
		recFound = FALSE;
		numToFind = 0;

		//read the data into the storage - the data is delimited using comma's
		ResearchName[0] = '\0';
		StructureName[0] = '\0';
		sscanf(pStructData,"%[^','],%[^','],%*d,%*d", ResearchName, StructureName);

		if (!getResourceName(ResearchName))
		{
			return FALSE;
		}
		if (!getResourceName(StructureName))
		{
			return FALSE;
		}

		//loop through each Research to compare the name
		for (incR = 0; incR < numResearch; incR++)
		{
			if (!(strcmp(ResearchName, pResearch[incR].pName)))
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
								pResearch[incR].pStructList[pResearch[incR].
									storeCount] = incS;
								//keep tab on how many we have loaded in
								numResearchStructPR++;
								numToFind = pResearch[incR].numStructures;
								break;
							case RED_LIST:
								pResearch[incR].pRedStructs[pResearch[incR].
									storeCount] = incS;
								//keep tab on how many we have loaded in
								numResearchStructRed++;
								numToFind = pResearch[incR].numRedStructs;
								break;
							case RES_LIST:
								pResearch[incR].pStructureResults[pResearch[incR].
									storeCount] = incS;
								//keep tab on how many we have loaded in
								numResearchStructRes++;
								numToFind = pResearch[incR].numStructResults;
								break;
							default:
								/* NO DEFAULT CASE? Alex.... Here ya go - just for you...*/
								debug( LOG_ERROR, "Unknown research list" );
								abort();
								return FALSE;
						}
						recFound = TRUE;
						//check not allocating more than allowed
						if (pResearch[incR].storeCount >
										(SDWORD)numToFind)
						{
							debug( LOG_ERROR, "Trying to allocate more Structures than allowed for research %s", getResearchName(pResearch) );
							abort();
							return FALSE;
						}
						pResearch[incR].storeCount++;
						break;
					}
				}
				//if Structure not found - error
				if (!recFound)
				{
					debug( LOG_ERROR, "Unable to find Structure %s for research %s", StructureName, ResearchName );
					abort();
					return FALSE;
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
			debug( LOG_ERROR, "Unable to allocate all Research Structures for %s", ResearchName );
			abort();
			return FALSE;
		}

		//quick check that haven't reached max structs
		if (numResearchStructPR >= MAX_RESEARCH_STRUCT_PR ||
			numResearchStructRes >= MAX_RESEARCH_STRUCT_RES ||
			numResearchStructRed >= MAX_RESEARCH_STRUCT_RED)
		{
			//don't load any more since will write over memory!
			break;
		}
		//increment the pointer to the start of the next record
		pStructData = strchr(pStructData,'\n') + 1;
	}

	return TRUE;
}

//Load the pre-requisites for a research list
BOOL loadResearchFunctions(const char *pFunctionData, UDWORD bufferSize)
{
	const unsigned int NumToAlloc = numCR(pFunctionData, bufferSize);
	unsigned int i = 0;
	char				ResearchName[MAX_NAME_SIZE], FunctionName[MAX_NAME_SIZE];
	UDWORD				incR, incF;
	RESEARCH			*pResearch = asResearch;
	FUNCTION			**pFunction = asFunctions;
	BOOL				recFound;

	//initialise the storage flags
	for (incR = 0; incR < numResearch; incR++)
	{
		pResearch[incR].storeCount = 0;
	}
	pResearch = asResearch;

	//check not going to go over max
	ASSERT( NumToAlloc <= MAX_RESEARCH_FUNC, "loadResearchFunctions: too many" );
	numResearchFunc = 0;

	for (i=0; i < NumToAlloc; i++)
	{
		recFound = FALSE;
		//read the data into the storage - the data is delimited using comma's
		ResearchName[0] = '\0';
		FunctionName[0] = '\0';
		sscanf(pFunctionData,"%[^','],%[^','],%*d", ResearchName, FunctionName);

		if (!getResourceName(ResearchName))
		{
			return FALSE;
		}

		//loop through each Research to compare the name
		for (incR=0; incR < numResearch; incR++)
		{
			if (!(strcmp(ResearchName, pResearch[incR].pName)))
			{
				//Research found
				for (incF=0; incF < numFunctions; incF++)
				{
					if (!(strcmp(FunctionName, (*pFunction[incF]).pName)))
					{
						//Function found alloc this to the current Research
						pResearch[incR].pFunctionList[pResearch[incR].
							storeCount] = pFunction[incF];
						//keep tab on how many we have loaded in
						numResearchFunc++;

						recFound = TRUE;
						//check not allocating more than allowed
						if (pResearch[incR].storeCount >
										(SDWORD)pResearch[incR].numFunctions)
						{
							debug( LOG_ERROR, "Trying to allocate more Functions than allowed for research %s", ResearchName );
							abort();
							return FALSE;
						}
						pResearch[incR].storeCount++;
						break;
					}
				}
				//if Function not found - error
				if (!recFound)
				{
					debug( LOG_ERROR, "Unable to find Function %s for research %s", FunctionName, ResearchName );
					abort();
					return FALSE;
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
			return FALSE;
		}
		//quick check that haven't reached maxPR

		if (numResearchFunc >= MAX_RESEARCH_FUNC)
		{
			//don't load any more since will write over memory!
			break;
		}
		//increment the pointer to the start of the next record
		pFunctionData = strchr(pFunctionData,'\n') + 1;
	}

	return TRUE;
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
	UDWORD				incPR, incS;
	PLAYER_RESEARCH		*pPlayerRes = asPlayerResList[playerID];
	BOOL				bPRFound, bStructFound;

	ASSERT( numResearch < UWORD_MAX,
		"fillResearchList: only using a UWORD for storage - need more!" );
	for (inc=0; inc < numResearch; inc++)
	{
		//if the inc matches the 'topic' - automatically add to the list
		if (inc == topic)
		{
			goto add_research;
		}
		//if its a cancelled topic - add to list
		if (IsResearchCancelled(&pPlayerRes[inc]))
		{
			goto add_research;
		}
		//if the topic is possible and has not already been researched - add to list
		if ((IsResearchPossible(&pPlayerRes[inc])))
		{
			if ((IsResearchCompleted(&pPlayerRes[inc])==FALSE) && (IsResearchStarted(&pPlayerRes[inc])==FALSE))
			{
				goto add_research;
			}
		}

		//if single player mode and key topic, then ignore cos can't do it!
		if (!bMultiPlayer)

		{
			if (asResearch[inc].keyTopic)
			{
				continue;
			}
		}

		// make sure that the research is not completed  or started by another researchfac
		if ((IsResearchCompleted(&pPlayerRes[inc])==FALSE) && (IsResearchStarted(&pPlayerRes[inc])==FALSE))
		{
			// Research is not completed  ... also  it has not been started by another researchfac

			//if there aren't any PR's - go to next topic
			if (!asResearch[inc].numPRRequired)
			{
				continue;
			}

			//check for pre-requisites
			bPRFound = TRUE;
			for (incPR = 0; incPR < asResearch[inc].numPRRequired; incPR++)
			{
				if (IsResearchCompleted(&(pPlayerRes[asResearch[inc].pPRList[incPR]]))==0)
				{
					//if haven't pre-requisite - quit checking rest
					bPRFound = FALSE;
					break;
				}
			}
			if (!bPRFound)
			{
				//if haven't pre-requisites, skip the rest of the checks
				continue;
			}

			//check for structure effects
			bStructFound = TRUE;
			for (incS = 0; incS < asResearch[inc].numStructures; incS++)
			{
				//if (!checkStructureStatus(asStructureStats + asResearch[inc].
				//	pStructList[incS], playerID, SS_BUILT))
				if (!checkSpecificStructExists(asResearch[inc].pStructList[incS],
					playerID))
				{
					//if not built, quit checking
					bStructFound = FALSE;
					break;
				}
			}
			if (!bStructFound)
			{
				//if haven't all structs built, skip to next topic
				continue;
			}

add_research: //if passed all the tests - add it to the list
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
void researchResult(UDWORD researchIndex, UBYTE player, BOOL bDisplay,
					STRUCTURE *psResearchFacility)
{
	RESEARCH					*pResearch = asResearch + researchIndex;
	UDWORD						type, inc;//, upgrade;
	STRUCTURE					*psCurr;
	DROID						*psDroid;
	FUNCTION					*pFunction;
	UDWORD						compInc;
	MESSAGE						*pMessage;
	PLAYER_RESEARCH				*pPlayerRes = asPlayerResList[player];
	//the message gets sent to console
	char						consoleMsg[MAX_RESEARCH_MSG_SIZE];

	ASSERT( researchIndex < numResearch, "researchResult: invalid research index" );

	sendReseachStatus(NULL, researchIndex, player, FALSE);

	MakeResearchCompleted(&pPlayerRes[researchIndex]);

	//check for structures to be made available
	for (inc = 0; inc < pResearch->numStructResults; inc++)
	{
		apStructTypeLists[player][pResearch->pStructureResults[inc]] = AVAILABLE;
	}

	//check for structures to be made redundant
	for (inc = 0; inc < pResearch->numRedStructs; inc++)
	{
		apStructTypeLists[player][pResearch->pRedStructs[inc]] = UNAVAILABLE;
	}

	//check for artefacts to be made available
	for (inc = 0; inc < pResearch->numArteResults; inc++)
	{
		//determine the type of artefact
		type = statType(pResearch->pArtefactResults[inc]->ref);
		//set the component state to AVAILABLE
		compInc = pResearch->pArtefactResults[inc]->ref - statRefStart(type);
		apCompLists[player][type][compInc] = AVAILABLE;
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
			replaceComponent(pResearch->pArtefactResults[inc], pResearch->
				pReplacedArtefacts[inc], player);
			//set the 'old' component to unavailable
			type = statType(pResearch->pReplacedArtefacts[inc]->ref);
			//set the component state to AVAILABLE
			compInc = pResearch->pReplacedArtefacts[inc]->ref - statRefStart(type);
			apCompLists[player][type][compInc] = UNAVAILABLE;
		}
		//check if the component is a brain
		if (type == COMP_BRAIN)
		{
			cmdDroidAvailable(asBrainStats + compInc, player);
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
	for (inc = 0; inc < pResearch->numRedArtefacts; inc++)
	{
		//determine the type of artefact
		type = statType(pResearch->pRedArtefacts[inc]->ref);
		//set the component state to UNAVAILABLE
		apCompLists[player][type][pResearch->pRedArtefacts[inc]->ref -
			statRefStart(type)] = UNAVAILABLE;
	}

	//check for technology effects
	for (inc = 0; inc < pResearch->numFunctions; inc++)
	{
		pFunction = pResearch->pFunctionList[inc];

		switch (pFunction->type)
		{
			case(PRODUCTION_UPGRADE_TYPE):
			{
				productionUpgrade(pFunction, player);

				//search the list of players structures for a Factory
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					/*if (psCurr->pStructureType->type == REF_FACTORY ||
						psCurr->pStructureType->type == REF_CYBORG_FACTORY ||
						psCurr->pStructureType->type == REF_VTOL_FACTORY)
					{
						//upgrade the Output
						productionUpgrade(pFunction, psCurr);
					}*/
					if ((psCurr->pStructureType->type == REF_FACTORY &&
						((PRODUCTION_UPGRADE_FUNCTION *)pFunction)->factory) ||
						(psCurr->pStructureType->type == REF_CYBORG_FACTORY &&
						((PRODUCTION_UPGRADE_FUNCTION *)pFunction)->cyborgFactory) ||
						(psCurr->pStructureType->type == REF_VTOL_FACTORY &&
						((PRODUCTION_UPGRADE_FUNCTION *)pFunction)->vtolFactory))
					{
						//upgrade the Output for the structure
						structureProductionUpgrade(psCurr);
					}

					//set the function upgrade flag for future factories being built
					/*for (upgrade = 0; upgrade < numProductionUpgrades; upgrade++)
					{
						if (apProductionUpgrades[player][upgrade].functionInc == pFunction->
							ref - REF_FUNCTION_START)
						{
							apProductionUpgrades[player][upgrade].available = TRUE;
							break;
						}
					}*/
				}
                //and the mission structures
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					if ((psCurr->pStructureType->type == REF_FACTORY &&
						((PRODUCTION_UPGRADE_FUNCTION *)pFunction)->factory) ||
						(psCurr->pStructureType->type == REF_CYBORG_FACTORY &&
						((PRODUCTION_UPGRADE_FUNCTION *)pFunction)->cyborgFactory) ||
						(psCurr->pStructureType->type == REF_VTOL_FACTORY &&
						((PRODUCTION_UPGRADE_FUNCTION *)pFunction)->vtolFactory))
					{
						//upgrade the Output for the structure
						structureProductionUpgrade(psCurr);
    				}
    			}

			   	// message/sound in here for production boost
				break;
			}
	 		case(RESEARCH_UPGRADE_TYPE):
			{
				researchUpgrade(pFunction, player);
				//search the list of players structures for a Research Facility
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_RESEARCH)
					{
						//upgrade the research points
						//researchUpgrade(pFunction, psCurr);
						structureResearchUpgrade(psCurr);
					}
					//set the function upgrade flag for future factories being built
					/*for (upgrade = 0; upgrade < numResearchUpgrades; upgrade++)
					{
						if (apResearchUpgrades[player][upgrade].functionInc == pFunction->
							ref - REF_FUNCTION_START)
						{
							apResearchUpgrades[player][upgrade].available = TRUE;
							break;
						}
					}*/
				}
                //and the mission structures
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_RESEARCH)
					{
						//upgrade the research points
						structureResearchUpgrade(psCurr);
					}
				}
				// Stuff a message in here/sound whatever for research boost.
				break;
			}
	 		case(POWER_UPGRADE_TYPE):
			{
				powerUpgrade(pFunction, player);
				//search the list of players structures for a Power Gens
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_POWER_GEN)
					{
						//upgrade the power points
						structurePowerUpgrade(psCurr);
					}
				}
                //and the mission structure
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_POWER_GEN)
					{
						//upgrade the power points
						structurePowerUpgrade(psCurr);
					}
				}
				break;
			}
	 		case(REARM_UPGRADE_TYPE):
			{
				reArmUpgrade(pFunction, player);
				//search the list of players structures for a ReArm pad
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_REARM_PAD)
					{
						//upgrade the rearm points
						structureReArmUpgrade(psCurr);
					}
				}
                //and the mission structure
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_REARM_PAD)
					{
						//upgrade the rearm points
						structureReArmUpgrade(psCurr);
					}
				}
				break;
			}
	 		case(REPAIR_UPGRADE_TYPE):
			{
				repairFacUpgrade(pFunction, player);
				//search the list of players structures for a Power Gens
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_REPAIR_FACILITY)
					{
						//upgrade the repair points
						structureRepairUpgrade(psCurr);
					}
				}
                //and the mission structure
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					if (psCurr->pStructureType->type == REF_REPAIR_FACILITY)
					{
						//upgrade the repair points
						structureRepairUpgrade(psCurr);
					}
				}
				break;
			}
			case(WEAPON_UPGRADE_TYPE):
			{
				//for the current player, upgrade the weapon stats
				weaponUpgrade(pFunction, player);
				// message/sound for weapon upgrade
				break;
			}
			case(DROIDSENSOR_UPGRADE_TYPE):
			{
				//for the current player, upgrade the sensor stats
				sensorUpgrade(pFunction, player);
				//for each structure in the player's list, upgrade the sensor stat
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					structureSensorUpgrade(psCurr);
				}
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					structureSensorUpgrade(psCurr);
				}
				//for each droid in the player's list, upgrade the sensor stat
				for (psDroid = apsDroidLists[player]; psDroid != NULL; psDroid =
					psDroid->psNext)
				{
					droidSensorUpgrade(psDroid);
                    if (psDroid->droidType == DROID_TRANSPORTER)
                    {
                        upgradeTransporterDroids(psDroid, droidSensorUpgrade);
                    }
				}
				for (psDroid = mission.apsDroidLists[player]; psDroid != NULL; psDroid =
					psDroid->psNext)
				{
					droidSensorUpgrade(psDroid);
                    if (psDroid->droidType == DROID_TRANSPORTER)
                    {
                        upgradeTransporterDroids(psDroid, droidSensorUpgrade);
                    }
				}
				for (psDroid = apsLimboDroids[player]; psDroid != NULL; psDroid =
					psDroid->psNext)
				{
					droidSensorUpgrade(psDroid);
                    if (psDroid->droidType == DROID_TRANSPORTER)
                    {
                        upgradeTransporterDroids(psDroid, droidSensorUpgrade);
                    }
				}
				// message/sound for sensor upgrade
				break;
			}
			case(DROIDECM_UPGRADE_TYPE):
			{
				//for the current player, upgrade the ecm stats
				ecmUpgrade(pFunction, player);
				//for each structure in the player's list, upgrade the ecm stat
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					structureECMUpgrade(psCurr);
				}
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					structureECMUpgrade(psCurr);
				}
				//for each droid in the player's list, upgrade the ecm stat
				for (psDroid = apsDroidLists[player]; psDroid != NULL; psDroid =
					psDroid->psNext)
				{
					droidECMUpgrade(psDroid);
                    if (psDroid->droidType == DROID_TRANSPORTER)
                    {
                        upgradeTransporterDroids(psDroid, droidECMUpgrade);
                    }
				}
				for (psDroid = mission.apsDroidLists[player]; psDroid != NULL; psDroid =
					psDroid->psNext)
				{
					droidECMUpgrade(psDroid);
                    if (psDroid->droidType == DROID_TRANSPORTER)
                    {
                        upgradeTransporterDroids(psDroid, droidECMUpgrade);
                    }
				}
				for (psDroid = apsLimboDroids[player]; psDroid != NULL; psDroid =
					psDroid->psNext)
				{
					droidECMUpgrade(psDroid);
                    if (psDroid->droidType == DROID_TRANSPORTER)
                    {
                        upgradeTransporterDroids(psDroid, droidECMUpgrade);
                    }
				}
				// message/sound for ecm upgrade
				break;
			}
			case(DROIDREPAIR_UPGRADE_TYPE):
			{
				//for the current player, upgrade the repair stats
				repairUpgrade(pFunction, player);
				// message/sound for repair upgrade
				break;
			}
			case(DROIDCONST_UPGRADE_TYPE):
			{
				//for the current player, upgrade the constructor stats
				constructorUpgrade(pFunction, player);
				// message/sound for constructor upgrade
				break;
			}
			case(DROIDBODY_UPGRADE_TYPE):
			{
				//for each droid in the player's list, upgrade the body points
				for (psDroid = apsDroidLists[player]; psDroid != NULL; psDroid =
					psDroid->psNext)
				{
					droidBodyUpgrade(pFunction, psDroid);
				}
				for (psDroid = mission.apsDroidLists[player]; psDroid != NULL; psDroid =
					psDroid->psNext)
				{
					droidBodyUpgrade(pFunction, psDroid);
                    if (psDroid->droidType == DROID_TRANSPORTER)
                    {
                        upgradeTransporterDroids(psDroid, droidSensorUpgrade);
                    }
				}
				for (psDroid = apsLimboDroids[player]; psDroid != NULL; psDroid =
					psDroid->psNext)
				{
					droidBodyUpgrade(pFunction, psDroid);
                    if (psDroid->droidType == DROID_TRANSPORTER)
                    {
                        upgradeTransporterDroids(psDroid, droidSensorUpgrade);
                    }
				}
				//DO THIS AFTER so above calculations can use the previous upgrade values
				//for the current player, upgrade the body stats
				bodyUpgrade(pFunction, player);

				// message/sound for body upgrade
				break;
			}
			case(STRUCTURE_UPGRADE_TYPE):
			{
				//for each structure in the player's list, upgrade the stats
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					//do this for none wallDefense structs
					if (!wallDefenceStruct(psCurr->pStructureType))
					{
						structureBodyUpgrade(pFunction, psCurr);
						structureArmourUpgrade(pFunction, psCurr);
						structureResistanceUpgrade(pFunction, psCurr);
					}
                    //defense type can have resistance upgrade now - AB 19/02/99
                    if (psCurr->pStructureType->type == REF_DEFENSE)
                    {
                        structureResistanceUpgrade(pFunction, psCurr);
                    }
				}
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					//do this for none wallDefense structs
					if (!wallDefenceStruct(psCurr->pStructureType))
					{
						structureBodyUpgrade(pFunction, psCurr);
						structureArmourUpgrade(pFunction, psCurr);
						structureResistanceUpgrade(pFunction, psCurr);
					}
                    //defense type can have resistance upgrade now - AB 19/02/99
                    if (psCurr->pStructureType->type == REF_DEFENSE)
                    {
                        structureResistanceUpgrade(pFunction, psCurr);
                    }
				}
				//DO THIS AFTER so above calculations can use the previous upgrade values
				//for the current player, upgrade the structure stats
				structureUpgrade(pFunction, player);

				// message/sound for structure upgrade
				break;
			}
			case(WALLDEFENCE_UPGRADE_TYPE):
			{
				//for each structure in the player's list, upgrade the stats
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					//do this for wallDefense structs
					if (wallDefenceStruct(psCurr->pStructureType))
					{
						structureBodyUpgrade(pFunction, psCurr);
						structureArmourUpgrade(pFunction, psCurr);
					}
				}
				for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					//do this for wallDefense structs
					if (wallDefenceStruct(psCurr->pStructureType))
					{
						structureBodyUpgrade(pFunction, psCurr);
						structureArmourUpgrade(pFunction, psCurr);
					}
				}
				//DO THIS AFTER so above calculations can use the previous upgrade values
				//for the current player, upgrade the wall/defence structure stats
				wallDefenceUpgrade(pFunction, player);

				// message/sound for wall/defence structure upgrade
				break;
			}

			/*case(ARMOUR_UPGRADE_TYPE):
			{
				//for each structure in the player's list, upgrade the armour type
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					armourUpgrade(pFunction, psCurr);
				}
				//set the function upgrade flag for future factories being built
				for (upgrade = 0; upgrade < numArmourUpgrades; upgrade++)
				{
					if (apArmourUpgrades[player][upgrade].functionInc == pFunction->
						ref - REF_FUNCTION_START)
					{
						apArmourUpgrades[player][upgrade].available = TRUE;
						break;
					}
				}
				// message/sound in here for armour upgrade
				break;
			}*/
			/*case(REPAIR_UPGRADE_TYPE):
			{
				//for each structure in the player's list, upgrade the armour type
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					repairUpgrade(pFunction, psCurr);
				}
				//set the function upgrade flag for future factories being built
				for (upgrade = 0; upgrade < numRepairUpgrades; upgrade++)
				{
					if (apRepairUpgrades[player][upgrade].functionInc == pFunction->
						ref - REF_FUNCTION_START)
					{
						apRepairUpgrades[player][upgrade].available = TRUE;
						break;
					}
				}
			   	//message/sound in here for repair points upgraded
				break;
			}*/
			/*case(BODY_UPGRADE_TYPE):
			{
				//for each structure in the player's list, upgrade the armour type
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					bodyUpgrade(pFunction, psCurr);
				}
				//set the function upgrade flag for future factories being built
				for (upgrade = 0; upgrade < numBodyUpgrades; upgrade++)
				{
					if (apBodyUpgrades[player][upgrade].functionInc == pFunction->
						ref - REF_FUNCTION_START)
					{
						apBodyUpgrades[player][upgrade].available = TRUE;
						break;
					}
				}
			   	// message/sound in here for body points upgrade
				break;
			}*/
			/*case(RESISTANCE_UPGRADE_TYPE):
			{
				//for each structure in the player's list, upgrade the armour type
				for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr =
					psCurr->psNext)
				{
					resistanceUpgrade(pFunction, psCurr);
				}
				//set the function upgrade flag for future factories being built
				for (upgrade = 0; upgrade < numResistanceUpgrades; upgrade++)
				{
					if (apResistanceUpgrades[player][upgrade].functionInc == pFunction->
						ref - REF_FUNCTION_START)
					{
						apResistanceUpgrades[player][upgrade].available = TRUE;
						break;
					}
				}
				// message/sound for resistance upgrade
				break;
			}*/
			default:
			{
				ASSERT( FALSE,"Invalid function type" );
			}
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
			pMessage = addMessage(MSG_RESEARCH, FALSE, player);
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
			snprintf(consoleMsg, MAX_RESEARCH_MSG_SIZE, _("Research completed: %s"), *pResearch->pViewData->ppTextMsg);
			addConsoleMessage(consoleMsg, LEFT_JUSTIFY);
		}
		else
		{
			addConsoleMessage(_("Research Completed"), LEFT_JUSTIFY);
		}
	}

#ifdef SCRIPTS
    if ((bMultiPlayer || player == selectedPlayer) && bDisplay)
    {
        psCBLastResearch = pResearch;
		CBResFacilityOwner = player;
		psCBLastResStructure = psResearchFacility;
	    eventFireCallbackTrigger((TRIGGER_TYPE)CALL_RESEARCHCOMPLETED);
		psCBLastResStructure = NULL;
        CBResFacilityOwner = -1;
		psCBLastResearch = NULL;
    }
#endif

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
                asWeaponStats[inc].design)
            {
                if (asWeaponStats[inc].vtolAttackRuns)
                {
                    vtolCompInc++;
                }
                else
                {
                    compInc++;
                }
                if (compInc >= 32)
                {
					debug(LOG_ERROR, "researchResult - more than 32 weapons now available");
                   
					//don't bother checking any more
                    break;
                }
                if (vtolCompInc >= 32)
                {
					debug(LOG_ERROR, "researchResult - more than 32 vtol weapons now available");
                    
					//don't bother checking any more
                    break;
                }
            }
	    }
    }
#endif
}

/*This function is called when the research files are reloaded*/
BOOL ResearchShutDown(void)
{
	UBYTE   i;

	memset(asResearch, 0, (MAX_RESEARCH * sizeof(RESEARCH)));

	for (i=0; i < MAX_PLAYERS; i++)
	{
   		memset(asPlayerResList[i], 0, (MAX_RESEARCH * sizeof(PLAYER_RESEARCH)));
	}

    //and init all the other arrays used
    memset(pResearchPR, 0, (MAX_RESEARCH_PR * sizeof(UWORD)));
    //memset(pResearchPR, 0, (MAX_RESEARCH_PR * sizeof(UBYTE)));
    memset(pResearchStructPR, 0, (MAX_RESEARCH_STRUCT_PR * sizeof(UWORD)));
    memset(pResearchFunc, 0, (MAX_RESEARCH_FUNC * sizeof(FUNCTION *)));
    memset(pResearchStructRed, 0, (MAX_RESEARCH_STRUCT_RED * sizeof(UWORD)));
    memset(pResearchArteRed, 0, (MAX_RESEARCH_ARTE_RED * sizeof(COMP_BASE_STATS *)));
    memset(pResearchStructRes, 0, (MAX_RESEARCH_STRUCT_RES * sizeof(UWORD)));
    memset(pResearchArteRes, 0, (MAX_RESEARCH_ARTE_RES * sizeof(COMP_BASE_STATS *)));
    memset(pResearchArteRep, 0, (MAX_RESEARCH_ARTE_RES * sizeof(COMP_BASE_STATS *)));

    return TRUE;
}

/*This function is called when a game finishes*/
void ResearchRelease(void)
{
	unsigned int i;

	//free all the pre-defined arrays for research
	free(asResearch);
	asResearch = NULL;

	for (i=0; i < MAX_PLAYERS; i++)
	{
        	free(asPlayerResList[i]);
		asPlayerResList[i] = NULL;
	}

	free(pResearchPR);
	pResearchPR = NULL;

	free(pResearchStructPR);
	pResearchStructPR = NULL;

	free(pResearchFunc);
	pResearchFunc = NULL;

	free(pResearchStructRed);
	pResearchStructRed = NULL;

	free(pResearchArteRed);
	pResearchArteRed = NULL;

	free(pResearchStructRes);
	pResearchStructRes = NULL;

	free(pResearchArteRes);
	pResearchArteRes = NULL;

	free(pResearchArteRep);
	pResearchArteRep = NULL;
}

/*puts research facility on hold*/
void holdResearch(STRUCTURE *psBuilding)
{

	RESEARCH_FACILITY		*psResFac;

	ASSERT( psBuilding->pStructureType->type == REF_RESEARCH,
		"holdResearch: structure not a research facility" );

	psResFac = (RESEARCH_FACILITY *)psBuilding->pFunctionality;

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

}

/*release a research facility from hold*/
void releaseResearch(STRUCTURE *psBuilding)
{
	RESEARCH_FACILITY		*psResFac;

	ASSERT( psBuilding->pStructureType->type == REF_RESEARCH,
		"releaseResearch: structure not a research facility" );

	psResFac = (RESEARCH_FACILITY *)psBuilding->pFunctionality;

	if (psResFac->psSubject && psResFac->timeStartHold)
	{
		//adjust the start time for the current subject
		if (psResFac->timeStarted != ACTION_START_TIME)
		{
			psResFac->timeStarted += (gameTime - psResFac->timeStartHold);
		}
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
				cancelResearch(psCurr);
			}
		}

	}
}

/* sets the status of the topic to cancelled and stores the current research
   points accquired */
void cancelResearch(STRUCTURE *psBuilding)
{
    UDWORD              topicInc;
	PLAYER_RESEARCH	    *pPlayerRes;
	RESEARCH_FACILITY	*psResFac;

	ASSERT( psBuilding->pStructureType->type == REF_RESEARCH,
		"cancelResearch: structure not a research facility" );

	psResFac = (RESEARCH_FACILITY *)psBuilding->pFunctionality;
    topicInc = ((RESEARCH *)psResFac->psSubject) - asResearch;
    if (topicInc > numResearch)
    {
        ASSERT( FALSE, "cancelResearch: invalid research topic" );
        return;
    }
    pPlayerRes = asPlayerResList[psBuilding->player] + topicInc;

	if (psBuilding->pStructureType->type == REF_RESEARCH)
	{
		//check if waiting to accrue power
		if (psResFac->timeStarted == ACTION_START_TIME)
		{
			//return the power
			addPower(psBuilding->player, psResFac->powerAccrued);
			psResFac->powerAccrued = 0;
            //reset this topic as not having been researched
			ResetResearchStatus(pPlayerRes);
        }
		else
		{
		// only PC version saves these
			/*store the points - need to keep this so can add points after the topic
			has been cancelled and restarted*/
			pPlayerRes->currentPoints += (psResFac->researchPoints * (gameTime -
                psResFac->timeStarted)) / GAME_TICKS_PER_SEC;


            //set the researched flag
			MakeResearchCancelled(pPlayerRes);
		}

		 sendReseachStatus(psBuilding, topicInc, psBuilding->player, FALSE);

        //initialise the research facility's subject
        psResFac->psSubject = NULL;

		//set the researched flag - only set the flag to cancelled if it got past the accruePower stage
		//pPlayerRes->researched = CANCELLED_RESEARCH;
	}
}

/* For a given view data get the research this is related to */
RESEARCH * getResearchForMsg(VIEWDATA *pViewData)
{
	UDWORD		inc;
	RESEARCH	*psResearch;

	for (inc = 0; inc < numResearch; inc++)
	{
		psResearch = asResearch + inc;
		//compare the pointer
		if (psResearch->pViewData == pViewData)
		{
			return psResearch;
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

    //add more names as images are created
	ASSERT( FALSE, "Invalid icon graphic %s for topic %s", pIconName, pName );

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
		ASSERT( FALSE,"Weirdy mapping request for RID to icon" );
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
COMP_BASE_STATS * getComponentDetails(char *pName, char *pCompName)
{
	UDWORD stat, size, quantity, inc;
	COMP_BASE_STATS		*pArtefact;

	stat = componentType(pName);
	//get the stat list
	switch (stat)
	{
		case COMP_BODY:
		{
			pArtefact = (COMP_BASE_STATS*)asBodyStats;
			size = sizeof(BODY_STATS);
			quantity = numBodyStats;
			break;
		}
		case COMP_BRAIN:
		{
			pArtefact = (COMP_BASE_STATS*)asBrainStats;
			size = sizeof(BRAIN_STATS);
			quantity = numBrainStats;
			break;
		}
		case COMP_PROPULSION:
		{
			pArtefact = (COMP_BASE_STATS*)asPropulsionStats;
			size = sizeof(PROPULSION_STATS);
			quantity = numPropulsionStats;
			break;
		}
		case COMP_REPAIRUNIT:
		{
			pArtefact = (COMP_BASE_STATS*)asRepairStats;
			size = sizeof(REPAIR_STATS);
			quantity = numRepairStats;
			break;
		}
		case COMP_ECM:
		{
			pArtefact = (COMP_BASE_STATS*)asECMStats;
			size = sizeof(ECM_STATS);
			quantity = numECMStats;
			break;
		}
		case COMP_SENSOR:
		{
			pArtefact = (COMP_BASE_STATS*)asSensorStats;
			size = sizeof(SENSOR_STATS);
			quantity = numSensorStats;
			break;
		}
		case COMP_WEAPON:
		{
			pArtefact = (COMP_BASE_STATS*)asWeaponStats;
			size = sizeof(WEAPON_STATS);
			quantity = numWeaponStats;
			break;
		}
		case COMP_CONSTRUCT:
		{
			pArtefact = (COMP_BASE_STATS*)asConstructStats;
			size = sizeof(CONSTRUCT_STATS);
			quantity = numConstructStats;
			break;
		}
		default:
		{
			//COMP_UNKNOWN should be an error
			debug( LOG_ERROR, "Unknown artefact type  - %s", pName );
			abort();
			return FALSE;
		}
	}

	for (inc = 0; inc < quantity; inc++)
	{
		if (!strcmp(pArtefact->pName, pCompName))
		{
			return pArtefact;
		}
		pArtefact = (COMP_BASE_STATS*)((char*)pArtefact + size);
	}

	debug( LOG_ERROR, "Cannot find component %s", pCompName );
	abort();
	return NULL;
}

//return a pointer to a research topic based on the name
RESEARCH * getResearch(const char *pName, BOOL resName)
{
	unsigned int inc = 0;

	//need to get the in game name if a resource name has been passed in
	if (resName && !getResourceName(pName))
	{
		debug(LOG_ERROR, "getResearch: resource not found");
		return NULL;
	}

	for (inc = 0; inc < numResearch; inc++)
	{
		if (!strcasecmp(asResearch[inc].pName, pName))
		{
			return &asResearch[inc];
		}
	}

	for (inc = 0; inc < numResearch; inc++) {
		debug(LOG_ERROR, "  Research %d: %s", inc, asResearch[inc].pName);
	}
	debug(LOG_ERROR, "Unknown research - %s", pName);
	assert(FALSE);

	return NULL;
}

/* looks through the players lists of structures and droids to see if any are using
 the old component - if any then replaces them with the new component */
static void replaceComponent(COMP_BASE_STATS *pNewComponent, COMP_BASE_STATS *pOldComponent,
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
static BOOL checkResearchName(RESEARCH *psResearch, UDWORD numStats)
{
	UDWORD inc;


	char *pName=psResearch->pName;

	for (inc = 0; inc < numStats; inc++)
	{

		if (!strcmp(asResearch[inc].pName, pName))
		{
			//oops! found the name
			ASSERT( FALSE, "Research name has already been used - %s", pName );
			return FALSE;
		}
	}
	return TRUE;
}

/* Sets the 'possible' flag for a player's research so the topic will appear in
the research list next time the Research Facilty is selected */
BOOL enableResearch(RESEARCH *psResearch, UDWORD player)
{
	UDWORD				inc;
	PLAYER_RESEARCH		*pPlayerRes = asPlayerResList[player];
	STRUCTURE			*psStruct;
	BOOL				resFree = FALSE;


	inc = psResearch - asResearch;
	if (inc > numResearch)
	{
		ASSERT( FALSE, "enableResearch: Invalid research topic - %s", getResearchName(psResearch) );
		return FALSE;
	}

	//found, so set the flag
	MakeResearchPossible(&pPlayerRes[inc]);

	if(player == selectedPlayer)
	{
		//set the research reticule button to flash if research facility is free
		for (psStruct = apsStructLists[selectedPlayer]; psStruct != NULL; psStruct=psStruct->psNext)
		{
			if (psStruct->pStructureType->type == REF_RESEARCH &&
                psStruct->status == SS_BUILT &&
				((RESEARCH_FACILITY *)psStruct->pFunctionality)->psSubject == NULL)
			{
				resFree = TRUE;
				break;
			}
		}
		if (resFree)
		{
			flashReticuleButton(IDRET_RESEARCH);
		}

	}

	return TRUE;
}

/*find the last research topic of importance that the losing player did and
'give' the results to the reward player*/
void researchReward(UBYTE losingPlayer, UBYTE rewardPlayer)
{
	UDWORD				topicIndex, researchPoints, rewardID;
	STRUCTURE			*psStruct;
	RESEARCH_FACILITY	*psFacility;

	topicIndex = researchPoints = rewardID = 0;
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
		researchResult(rewardID, rewardPlayer, TRUE, NULL);
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
BOOL checkResearchStats(void)
{
	UDWORD resInc, inc;
	for (resInc = 0; resInc < numResearch; resInc++)
	{
		if (asResearch[resInc].numPRRequired == 0)
		{
			if (asResearch[resInc].pPRList != NULL)
			{
				ASSERT( FALSE,
					"checkResearchStats: PreReq for topic %s should be NULL",
					asResearch[resInc].pName );
				return FALSE;
			}
		}
		else
		{
			for (inc = 0; inc < asResearch[resInc].numPRRequired; inc++)
			{
				if (asResearch[resInc].pPRList[inc] > numResearch)
				{
					ASSERT( FALSE,
						"checkResearchStats: Invalid PreReq for topic %s",
						asResearch[resInc].pName );
					return FALSE;
				}

			}
		}
		if (asResearch[resInc].numStructures == 0)
		{
			if (asResearch[resInc].pStructList != NULL)
			{
				ASSERT( FALSE,
					"checkResearchStats: StructureList for topic %s should be NULL",
					asResearch[resInc].pName );
				return FALSE;
			}
		}
		else
		{
			for (inc = 0; inc < asResearch[resInc].numStructures; inc++)
			{
				if (asResearch[resInc].pStructList[inc] > numStructureStats)
				{
					ASSERT( FALSE,
						"checkResearchStats: Invalid Structure for topic %s",
						asResearch[resInc].pName );
					return FALSE;
				}
			}
		}
		if (asResearch[resInc].numFunctions == 0)
		{
			if (asResearch[resInc].pFunctionList != NULL)
			{
				ASSERT( FALSE,
					"checkResearchStats: FunctionList for topic %s should be NULL",
					asResearch[resInc].pName );
				return FALSE;
			}
		}
		else
		{
			for (inc = 0; inc < asResearch[resInc].numFunctions; inc++)
			{
				if (asResearch[resInc].pFunctionList[inc]->ref -
					REF_FUNCTION_START > numFunctions)
				{
					ASSERT( FALSE, "checkResearchStats: Invalid function for %s",
					asResearch[resInc].pName );
				}
			}
		}
		if (asResearch[resInc].numRedStructs == 0)
		{
			if (asResearch[resInc].pRedStructs != NULL)
			{
				ASSERT( FALSE,
					"checkResearchStats: Redundant StructList for topic %s should be NULL",
					asResearch[resInc].pName );
				return FALSE;
			}
		}
		else
		{
			for (inc = 0; inc < asResearch[resInc].numRedStructs; inc++)
			{
				if (asResearch[resInc].pRedStructs[inc] > numStructureStats)
				{
					ASSERT( FALSE,
						"checkResearchStats: Invalid Redundant Structure for topic %s",
						asResearch[resInc].pName );
					return FALSE;
				}
			}
		}
		if (asResearch[resInc].numStructResults == 0)
		{
			if (asResearch[resInc].pStructureResults != NULL)
			{
				ASSERT( FALSE,
					"checkResearchStats: Result StructList for topic %s should be NULL",
					asResearch[resInc].pName );
				return FALSE;
			}
		}
		else
		{
			for (inc = 0; inc < asResearch[resInc].numStructResults; inc++)
			{
				if (asResearch[resInc].pStructureResults[inc] > numStructureStats)
				{
					ASSERT( FALSE,
						"checkResearchStats: Invalid Result Structure for topic %s",
						asResearch[resInc].pName );
					return FALSE;
				}
			}
		}
		if (asResearch[resInc].numArteResults == 0)
		{
			if (asResearch[resInc].pArtefactResults != NULL)
			{
				ASSERT( FALSE,
					"checkResearchStats: CompResultList for topic %s should be NULL",
					asResearch[resInc].pName );
				return FALSE;
			}
		}
		else
		{
			for (inc = 0; inc < asResearch[resInc].numArteResults; inc++)
			{
				ASSERT( asResearch[resInc].pArtefactResults[inc] != NULL,
					"checkResearchStats: Invalid Comp Result for topic %s",
					asResearch[resInc].pName );
			}
		}
		if (asResearch[resInc].numRedArtefacts == 0)
		{
			if (asResearch[resInc].pRedArtefacts != NULL)
			{
				ASSERT( FALSE,
					"checkResearchStats: RedundantCompList for topic %s should be NULL",
					asResearch[resInc].pName );
				return FALSE;
			}
		}
		else
		{
			for (inc = 0; inc < asResearch[resInc].numRedArtefacts; inc++)
			{
				ASSERT( asResearch[resInc].pRedArtefacts[inc] != NULL,
					"checkResearchStats: Invalid Redundant Comp for topic %s",
					asResearch[resInc].pName );
			}
		}
	}

	return TRUE;
}

/*flag self repair so droids can start when idle*/
void enableSelfRepair(UBYTE player)
{
	bSelfRepair[player] = TRUE;
}

/*check to see if any research has been completed that enables self repair*/
BOOL selfRepairEnabled(UBYTE player)
{
	if (bSelfRepair[player])
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*checks the stat to see if its of type wall or defence*/
BOOL wallDefenceStruct(STRUCTURE_STATS *psStats)
{
	if (psStats->type == REF_DEFENSE ||
		psStats->type == REF_WALL ||
		psStats->type == REF_WALLCORNER ||
        psStats->type == REF_BLASTDOOR)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
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
        //need to replace the units inside the transporter
        if (psDroid->droidType == DROID_TRANSPORTER)
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

    ASSERT( psTransporter->droidType == DROID_TRANSPORTER,
        "replaceTransUnitComponents: invalid unit type" );

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

    //if the type is not one we are interested in, then don't bother checking
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
				if (psStructure->pStructureType->pECM == (asECMStats + oldCompInc))
				{
					psStructure->ecmPower = (UWORD)(asECMStats + newCompInc)->power;
				}
				break;
			case COMP_SENSOR:
				if (psStructure->pStructureType->pSensor == (asSensorStats + oldCompInc))
				{
					//psStructure->sensorPower = (asSensorStats + newCompInc)->power;
					psStructure->sensorPower = (UWORD)sensorPower(asSensorStats +
						newCompInc,player);
					//psStructure->sensorRange = (asSensorStats + newCompInc)->range;
					psStructure->sensorRange = (UWORD)sensorRange(asSensorStats +
						newCompInc,player);
				}
				break;
			case COMP_WEAPON:
				//Watermelon:can only be STRUCT_MAXWEAPS weapons now
				for (inc=0; inc < psStructure->numWeaps; inc++)
				{
					//can only be one weapon now
					if (psStructure->asWeaps[inc].nStat > 0)
					{
						/*if (psStructure->asWeaps[inc].nStat == oldCompInc)
						{
							psStructure->asWeaps[inc].nStat = newCompInc;
						}*/
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

    ASSERT( psDroid != NULL,
        "switchComponent:invalid droid pointer" );

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
            //can only be one weapon now
			//for (inc=0; inc < psDroid->numWeaps; inc++)
            if (psDroid->asWeaps[0].nStat > 0)
			{
				//if (psDroid->asWeaps[inc].nStat == oldCompInc)
                if (psDroid->asWeaps[0].nStat == oldCompInc)
				{
					//psDroid->asWeaps[inc].nStat = newCompInc;
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
