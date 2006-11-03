/*
 * Research.h
 *
 * structures required for research stats
 *
 */
#ifndef _research_h
#define _research_h

#include "objectdef.h"

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


#define NO_RESEARCH_ICON	0


/* The store for the research stats */
extern		RESEARCH				*asResearch;
extern		UDWORD					numResearch;

//List of pointers to arrays of PLAYER_RESEARCH[numResearch] for each player
extern PLAYER_RESEARCH*		asPlayerResList[MAX_PLAYERS];

//used for Callbacks to say which topic was last researched
extern RESEARCH                *psCBLastResearch;

/* Default level of sensor, repair and ECM */
extern UDWORD	aDefaultSensor[MAX_PLAYERS];
extern UDWORD	aDefaultECM[MAX_PLAYERS];
extern UDWORD	aDefaultRepair[MAX_PLAYERS];

//extern BOOL loadResearch(void);
extern BOOL loadResearch(char *pResearchData, UDWORD bufferSize);
//Load the pre-requisites for a research list
extern BOOL loadResearchPR(char *pPRData, UDWORD bufferSize);
//Load the artefacts for a research list
extern BOOL loadResearchArtefacts(char *pArteData, UDWORD bufferSize, 
								  UDWORD listNumber);
//Load the pre-requisites for a research list
extern BOOL loadResearchFunctions(char *pFunctionData, UDWORD bufferSize);
//Load the Structures for a research list
extern BOOL loadResearchStructures(char *pStructData, UDWORD bufferSize,
								   UDWORD listNumber);

/*function to check what can be researched for a particular player at any one 
  instant. Returns the number to research*/
//extern UBYTE fillResearchList(UBYTE *plist, UDWORD playerID, UWORD topic, 
//							   UWORD limit);
//needs to be UWORD sized for Patches
extern UWORD fillResearchList(UWORD *plist, UDWORD playerID, UWORD topic, 
                              UWORD limit);

/* process the results of a completed research topic */
extern void researchResult(UDWORD researchIndex, UBYTE player, BOOL bDisplay);

//this just inits all the research arrays
extern BOOL ResearchShutDown(void);
//this free the memory used for the research
extern BOOL ResearchRelease(void);

/* For a given view data get the research this is related to */
extern RESEARCH * getResearch(char *pName, BOOL resName);

/* sets the status of the topic to cancelled and stores the current research
   points accquired */
extern void cancelResearch(STRUCTURE *psBuilding);

/* For a given view data get the research this is related to */
extern RESEARCH * getResearchForMsg(struct _viewdata *pViewData);

/* Sets the 'possible' flag for a player's research so the topic will appear in 
the research list next time the Research Facilty is selected */
extern BOOL enableResearch(RESEARCH *psResearch, UDWORD player);

/*find the last research topic of importance that the losing player did and 
'give' the results to the reward player*/
extern void researchReward(UBYTE losingPlayer, UBYTE rewardPlayer);

/*check to see if any research has been completed that enables self repair*/
extern BOOL selfRepairEnabled(UBYTE player);

extern SDWORD	mapRIDToIcon( UDWORD rid );
extern SDWORD	mapIconToRID(UDWORD iconID);
extern BOOL checkResearchStats(void);

/*puts research facility on hold*/
extern void holdResearch(STRUCTURE *psBuilding);
/*release a research facility from hold*/
extern void releaseResearch(STRUCTURE *psBuilding);

/*checks the stat to see if its of type wall or defence*/
extern BOOL wallDefenceStruct(STRUCTURE_STATS *psStats);

extern void enableSelfRepair(UBYTE player);

void CancelAllResearch(UDWORD pl);

#endif //research.h
