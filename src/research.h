/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
 *  structures required for research stats
 */

#ifndef __INCLUDED_SRC_RESEARCH_H__
#define __INCLUDED_SRC_RESEARCH_H__

#include "objectdef.h"

#define NO_RESEARCH_ICON 0
//max 'research complete' console message length
#define MAX_RESEARCH_MSG_SIZE 200


//used for loading in the research stats into the appropriate list
enum
{
	REQ_LIST,
	RED_LIST,
	RES_LIST
};


enum
{
RID_ROCKET,
RID_CANNON,
RID_HOVERCRAFT,
RID_ECM,
RID_PLASCRETE,
RID_TRACKS,
RID_DROIDTECH,
RID_WEAPONTECH,
RID_COMPUTERTECH,
RID_POWERTECH,
RID_SYSTEMTECH,
RID_STRUCTURETECH,
RID_CYBORGTECH,
RID_DEFENCE,
RID_QUESTIONMARK,
RID_GRPACC,
RID_GRPUPG,
RID_GRPREP,
RID_GRPROF,
RID_GRPDAM,
RID_MAXRID
};


/* The store for the research stats */
extern std::vector<RESEARCH> asResearch;

//List of pointers to arrays of PLAYER_RESEARCH[numResearch] for each player
extern std::vector<PLAYER_RESEARCH> asPlayerResList[MAX_PLAYERS];

//used for Callbacks to say which topic was last researched
extern RESEARCH				*psCBLastResearch;
extern STRUCTURE			*psCBLastResStructure;
extern SDWORD				CBResFacilityOwner;

/* Default level of sensor, repair and ECM */
extern UDWORD	aDefaultSensor[MAX_PLAYERS];
extern UDWORD	aDefaultECM[MAX_PLAYERS];
extern UDWORD	aDefaultRepair[MAX_PLAYERS];

//extern bool loadResearch(void);
extern bool loadResearch(const char *pResearchData, UDWORD bufferSize);
//Load the pre-requisites for a research list
extern bool loadResearchPR(const char *pPRData, UDWORD bufferSize);
//Load the artefacts for a research list
extern bool loadResearchArtefacts(const char *pArteData, UDWORD bufferSize, UDWORD listNumber);
//Load the pre-requisites for a research list
extern bool loadResearchFunctions(const char *pFunctionData, UDWORD bufferSize);
//Load the Structures for a research list
extern bool loadResearchStructures(const char *pStructData, UDWORD bufferSize, UDWORD listNumber);

/*function to check what can be researched for a particular player at any one
  instant. Returns the number to research*/
//extern UBYTE fillResearchList(UBYTE *plist, UDWORD playerID, UWORD topic,
//							   UWORD limit);
//needs to be UWORD sized for Patches
extern UWORD fillResearchList(UWORD *plist, UDWORD playerID, UWORD topic,
                              UWORD limit);

/* process the results of a completed research topic */
extern void researchResult(UDWORD researchIndex, UBYTE player, bool bDisplay, STRUCTURE *psResearchFacility, bool bTrigger);

//this just inits all the research arrays
extern bool ResearchShutDown(void);
//this free the memory used for the research
extern void ResearchRelease(void);

/* For a given view data get the research this is related to */
extern RESEARCH * getResearch(const char *pName);

/* sets the status of the topic to cancelled and stores the current research
   points accquired */
extern void cancelResearch(STRUCTURE *psBuilding, QUEUE_MODE mode);

/* For a given view data get the research this is related to */
struct VIEWDATA;
RESEARCH *getResearchForMsg(VIEWDATA *pViewData);

/* Sets the 'possible' flag for a player's research so the topic will appear in
the research list next time the Research Facilty is selected */
extern bool enableResearch(RESEARCH *psResearch, UDWORD player);

/*find the last research topic of importance that the losing player did and
'give' the results to the reward player*/
extern void researchReward(UBYTE losingPlayer, UBYTE rewardPlayer);

/*check to see if any research has been completed that enables self repair*/
extern bool selfRepairEnabled(UBYTE player);

extern SDWORD	mapRIDToIcon( UDWORD rid );
extern SDWORD	mapIconToRID(UDWORD iconID);
extern bool checkResearchStats(void);

/*puts research facility on hold*/
extern void holdResearch(STRUCTURE *psBuilding, QUEUE_MODE mode);
/*release a research facility from hold*/
extern void releaseResearch(STRUCTURE *psBuilding, QUEUE_MODE mode);

/*checks the stat to see if its of type wall or defence*/
extern bool wallDefenceStruct(STRUCTURE_STATS *psStats);

extern void enableSelfRepair(UBYTE player);

void CancelAllResearch(UDWORD pl);

extern bool researchInitVars(void);

bool researchAvailable(int inc, int playerID);

#endif // __INCLUDED_SRC_RESEARCH_H__
