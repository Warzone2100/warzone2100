/*
 * Mechanics.c
 *
 * Game world mechanics.
 *
 */

/* extra structures required for demo */
//#define DEMO

#include "frame.h"
#include "mechanics.h"
#include "stats.h"
#include "structure.h"
#include "game.h"
#include "power.h"
#include "projectile.h"
#include "move.h"
#include "message.h"
#include "deliverance.h"
#include "astar.h"
#include "visibility.h"

BOOL gameStatStart(void);
void gameStatEnd(void);



/* Initialise the mechanics system */
//BOOL mechInitialise(void)
//{
//	UBYTE	*pFileData;
//	UDWORD	fileSize;

	/* Initialise the map */
	/*if (!loadFile("blank.map", &pFileData, &fileSize))
	{
		return FALSE;
	}

	if (!mapLoad(pFileData, fileSize))
	{
		return FALSE;
	}

	FREE(pFileData); */

	//load all the component stats from the access database
	/*if (!loadStats())
	{
		return FALSE;
	}*/

	//load the droid Templates
	/*if (!loadDroidTemplates())
	{
		return FALSE;
	}*/

	/*if (!loadFunctionStats())
	{
		return FALSE;
	}*/

	/*if (!loadStructureStats())
	{
		return FALSE;
	}*/

	/*if (!loadStructureWeapons())
	{
		return FALSE;
	}

	if (!loadStructureFunctions())
	{
		return FALSE;
	}*/

	//load the research stats - must have loaded the structure and functions stats first
/*	if (!loadResearch())
	{
		return FALSE;
	}
*/
	/*the template weapons and programs should have been read in through the wrf file by now
	 so calculate build points and power points*/
	//initTemplatePoints(); DONE IN SAVE GAME NOW SINCE ALWAYS STARTING IN ONE

	//sets up the initial game stats - what the player's got etc
//	gameStatStart();	- moved to data.c when stats are loaded from WRF - John.

	/*set up the Power levels for each player - again this is set up in load game now*/
	//initPlayerPower();

//	return TRUE;
//}


/* Shutdown the mechanics system */
BOOL mechShutdown(void)
{
//	UDWORD	i;
//	DROID	*psCurr;
	BASE_OBJECT		*psObj, *psNext;

  /*	for(i=0; i<MAX_PLAYERS; i++)
	{
		for(psCurr=apsDroidLists[i]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			FREE(psCurr->pName);
			if (psCurr->numWeaps > 0)
			{
				FREE(psCurr->asWeaps);
			}
			if (psCurr->numProgs > 0)
			{
				FREE(psCurr->asProgs);
			}
		}
	}*/

	for(psObj = psDestroyedObj; psObj; psObj = psNext)
	{
		psNext = psObj->psNext;
		if (psObj->type == OBJ_DROID)
		{
		/*	FREE(((DROID *)psObj)->pName);
			if (((DROID *)psObj)->numWeaps > 0)
			{
				FREE(((DROID *)psObj)->asWeaps);
			}
			if (((DROID *)psObj)->numProgs > 0)
			{
				FREE(((DROID *)psObj)->asProgs);
			}*/
			droidRelease((DROID *)psObj);
			heapFree(psDroidHeap, (DROID *)psObj);
		}
		if (psObj->type == OBJ_STRUCTURE)
		{
			structureRelease((STRUCTURE *)psObj);
			heapFree(psStructHeap, (STRUCTURE *)psObj);
		}
		if (psObj->type == OBJ_FEATURE)
		{
			featureRelease((FEATURE *)psObj);
			heapFree(psFeatureHeap, (FEATURE *)psObj);
		}
	}
	psDestroyedObj = NULL;

	//Free the space allocated for the players lists
//	gameStatEnd();

	return TRUE;
}


// Allocate the list for a component
BOOL allocComponentList(COMPONENT_TYPE	type, SDWORD number)
{
	SDWORD	inc, comp;

	//allocate the space for the Players' component lists
	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		apCompLists[inc][type] = (UBYTE *) MALLOC(sizeof(UBYTE) * number);
		if (apCompLists[inc][type] == NULL)
		{
			DBERROR(("Out of memory assigning Player Component Lists"));
			return FALSE;
		}

		//initialise the players' lists
		for (comp=0; comp <number; comp++)
		{
			apCompLists[inc][type][comp] = UNAVAILABLE;
		}
	}

	return TRUE;
}

// release all the component lists
void freeComponentLists(void)
{
	UDWORD	inc;

	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		//free the component lists
		FREE(apCompLists[inc][COMP_BODY]);
		FREE(apCompLists[inc][COMP_BRAIN]);
		FREE(apCompLists[inc][COMP_PROPULSION]);
		FREE(apCompLists[inc][COMP_SENSOR]);
		FREE(apCompLists[inc][COMP_ECM]);
		FREE(apCompLists[inc][COMP_REPAIRUNIT]);
		FREE(apCompLists[inc][COMP_CONSTRUCT]);
		FREE(apCompLists[inc][COMP_WEAPON]);
		//FREE(apCompLists[inc][COMP_PROGRAM]);
	}
}

//allocate the space for the Players' structure lists
BOOL allocStructLists(void)
{
	SDWORD	inc, stat;

	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		if(numStructureStats) 
		{
			apStructTypeLists[inc] = (UBYTE *) MALLOC(sizeof(UBYTE) * 
								numStructureStats);
			if (apStructTypeLists[inc] == NULL)
			{
				DBERROR(("Out of memory assigning Player Structure Lists"));
				return FALSE;
			}
			for (stat = 0; stat < (SDWORD)numStructureStats; stat++)
			{
				apStructTypeLists[inc][stat] = UNAVAILABLE;
			}
		} 
		else 
		{
			apStructTypeLists[inc] = NULL;
		}
	}

	return TRUE;
}


// release the structure lists
void freeStructureLists(void)
{
	UDWORD	inc;

	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		//free the structure lists
		if(apStructTypeLists[inc]) {
			FREE(apStructTypeLists[inc]);
		}
	}
}


//initialises the players list for the start of the DEMO
BOOL gameStatStart(void)
{
//	UDWORD			inc, comp, stat;
//	BOOL			builtRes = FALSE, builtGen = FALSE;
#if 0
	STRUCTURE		*psStructure;
	DROID_TEMPLATE	*pTemplate;
	DROID_TEMPLATE	*pNewTempl, *pCurrTempl;
	UDWORD			posX, posY;
#endif

/**********************************************************************
 *              All this gets done by calls from data.c to
 *              allocComponentList and allocStructLists (above)
 *              John.

	//allocate the space for the Players' component lists
	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		apCompLists[inc][COMP_BODY] = (UDWORD *) MALLOC(sizeof(UDWORD) * numBodyStats);
		if (apCompLists[inc][COMP_BODY] == NULL)
		{
			DBERROR(("Out of memory assigning Player Component Lists"));
			return FALSE;
		}
		apCompLists[inc][COMP_BRAIN] = (UDWORD *) MALLOC(sizeof(UDWORD) * numBrainStats);
		if (apCompLists[inc][COMP_BRAIN] == NULL)
		{
			DBERROR(("Out of memory assigning Player Component Lists"));
			return FALSE;
		}
		apCompLists[inc][COMP_ECM] = (UDWORD *) MALLOC(sizeof(UDWORD) * numECMStats);
		if (apCompLists[inc][COMP_ECM] == NULL)
		{
			DBERROR(("Out of memory assigning Player Component Lists"));
			return FALSE;
		}
		apCompLists[inc][COMP_PROPULSION] = (UDWORD *) MALLOC(sizeof(UDWORD) * numPropulsionStats);
		if (apCompLists[inc][COMP_PROPULSION] == NULL)
		{
			DBERROR(("Out of memory assigning Player Component Lists"));
			return FALSE;
		}
		apCompLists[inc][COMP_SENSOR] = (UDWORD *) MALLOC(sizeof(UDWORD) * numSensorStats);
		if (apCompLists[inc][COMP_SENSOR] == NULL)
		{
			DBERROR(("Out of memory assigning Player Component Lists"));
			return FALSE;
		}
		apCompLists[inc][COMP_REPAIRUNIT] = (UDWORD *) MALLOC(sizeof(UDWORD) * numRepairStats);
		if (apCompLists[inc][COMP_REPAIRUNIT] == NULL)
		{
			DBERROR(("Out of memory assigning Player Component Lists"));
			return FALSE;
		}
		apCompLists[inc][COMP_CONSTRUCT] = (UDWORD *) MALLOC(sizeof(UDWORD) * numConstructStats);
		if (apCompLists[inc][COMP_REPAIRUNIT] == NULL)
		{
			DBERROR(("Out of memory assigning Player Component Lists"));
			return FALSE;
		}
		apCompLists[inc][COMP_WEAPON] = (UDWORD *) MALLOC(sizeof(UDWORD) * numWeaponStats);
		if (apCompLists[inc][COMP_WEAPON] == NULL)
		{
			DBERROR(("Out of memory assigning Player Component Lists"));
			return FALSE;
		}
		apCompLists[inc][COMP_PROGRAM] = (UDWORD *) MALLOC(sizeof(UDWORD) * numProgramStats);
		if (apCompLists[inc][COMP_PROGRAM] == NULL)
		{
			DBERROR(("Out of memory assigning Player Component Lists"));
			return FALSE;
		}
	}

	//initialise the players' lists
	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		for (comp=0; comp <numBodyStats; comp++)
		{
			apCompLists[inc][COMP_BODY][comp] = UNAVAILABLE;
		}
		for (comp=0; comp < numBrainStats; comp++)
		{
			apCompLists[inc][COMP_BRAIN][comp] = UNAVAILABLE;
		}
		for (comp=0; comp < numPropulsionStats; comp++)
		{
			apCompLists[inc][COMP_PROPULSION][comp] = UNAVAILABLE;
		}
		for (comp=0; comp < numSensorStats; comp++)
		{
			apCompLists[inc][COMP_SENSOR][comp] = UNAVAILABLE;
		}
		for (comp=0; comp < numECMStats; comp++)
		{
			apCompLists[inc][COMP_ECM][comp] = UNAVAILABLE;
		}
		for (comp=0; comp < numRepairStats; comp++)
		{
			apCompLists[inc][COMP_REPAIRUNIT][comp] = UNAVAILABLE;
		}
		for (comp=0; comp < numConstructStats; comp++)
		{
			apCompLists[inc][COMP_CONSTRUCT][comp] = UNAVAILABLE;
		}
		for (comp=0; comp < numWeaponStats; comp++)
		{
			apCompLists[inc][COMP_WEAPON][comp] = UNAVAILABLE;
		}
		for (comp=0; comp < numProgramStats; comp++)
		{
			apCompLists[inc][COMP_PROGRAM][comp] = UNAVAILABLE;
		}
	}

	//allocate the space for the Players' structure lists
	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		if(numStructureStats) 
		{
#ifdef DEMO
			apStructTypeLists[inc] = (UDWORD *) MALLOC(sizeof(UDWORD) * 
								(numStructureStats + NUM_DEMO_STRUCTS));
#else
			apStructTypeLists[inc] = (UDWORD *) MALLOC(sizeof(UDWORD) * 
								numStructureStats);
#endif
			if (apStructTypeLists[inc] == NULL)
			{
				DBERROR(("Out of memory assigning Player Structure Lists"));
				return FALSE;
			}
#ifdef DEMO
			for (stat = 0; stat < numStructureStats + NUM_DEMO_STRUCTS; stat++)
			{
				apStructTypeLists[inc][stat] = UNAVAILABLE;
			}
#else
			for (stat = 0; stat < numStructureStats; stat++)
			{
				apStructTypeLists[inc][stat] = UNAVAILABLE;
			}
#endif
		} 
		else 
		{
			apStructTypeLists[inc] = NULL;
		}
	}
*/
	//don't want any of the next bit if using scripts
#ifndef SCRIPTS
	//initialise the players' structure lists
	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{

		for (comp=0; comp <numStructureStats; comp++)
		{
			apStructTypeLists[inc][comp] = UNAVAILABLE;
		}
	}
	//set up the starting structures
	for (comp=0; comp < numStructureStats; comp++)
	{
		if (!strcmp(asStructureStats[comp].pName, "Droid Factory"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apStructTypeLists[inc][comp] = AVAILABLE;
			}
		}
		if (!strcmp(asStructureStats[comp].pName, "Factory Module"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apStructTypeLists[inc][comp] = AVAILABLE;
			}
		}
		if (!strcmp(asStructureStats[comp].pName, "Power Generator"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apStructTypeLists[inc][comp] = AVAILABLE;
			}
		}
		if (!strcmp(asStructureStats[comp].pName, "Oil Derrick"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apStructTypeLists[inc][comp] = AVAILABLE;
			}
		}
		if (!strcmp(asStructureStats[comp].pName, "Command Center"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apStructTypeLists[inc][comp] = AVAILABLE;
			}
		}
		if (!strcmp(asStructureStats[comp].pName, "Research Lab"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apStructTypeLists[inc][comp] = AVAILABLE;
			}
		}
		if (!strcmp(asStructureStats[comp].pName, "Research Module"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apStructTypeLists[inc][comp] = AVAILABLE;
			}
		}
	}

	//set up the starting components
	for (comp=0; comp < numWeaponStats; comp++)
	{
		if (!strcmp(asWeaponStats[comp].pName, "Light Machine Gun"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apCompLists[inc][COMP_WEAPON][comp] = AVAILABLE;
			}
			break;
		}
	}	
	for (comp=0; comp < numSensorStats; comp++)
	{
		if (!strcmp(asSensorStats[comp].pName, "Default Sensor"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apCompLists[inc][COMP_SENSOR][comp] = AVAILABLE;
			}
			break;
		}
	}		
	for (comp=0; comp < numRepairStats; comp++)
	{
		if (!strcmp(asRepairStats[comp].pName, "Light Repair #1"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apCompLists[inc][COMP_REPAIRUNIT][comp] = AVAILABLE;
			}
			break;
		}
	}		
	for (comp=0; comp < numPropulsionStats; comp++)
	{
		if (!strcmp(asPropulsionStats[comp].pName, "Wheeled Propulsion"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apCompLists[inc][COMP_PROPULSION][comp] = AVAILABLE;
			}
			break;
		}
	}		
	/*for (comp=0; comp < numProgramStats; comp++)
	{
		if (!strcmp(asProgramStats[comp].pName, "Radar Program"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apCompLists[inc][COMP_PROGRAM][comp] = AVAILABLE;
			}
		}
		else if (!strcmp(asProgramStats[comp].pName, "program #1"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apCompLists[inc][COMP_PROGRAM][comp] = AVAILABLE;
			}
		}
		else if (!strcmp(asProgramStats[comp].pName, "Construct"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apCompLists[inc][COMP_PROGRAM][comp] = AVAILABLE;
			}
		}
	}*/		
	for (comp=0; comp < numConstructStats; comp++)
	{
		if (!strcmp(asConstructStats[comp].pName, "Building Constructor"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apCompLists[inc][COMP_CONSTRUCT][comp] = AVAILABLE;
			}
			break;
		}
	}		
	for (comp=0; comp < numBrainStats; comp++)
	{
		if (!strcmp(asBrainStats[comp].pName, "Standard Brain"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apCompLists[inc][COMP_BRAIN][comp] = AVAILABLE;
			}
			break;
		}
	}		
	for (comp=0; comp < numBodyStats; comp++)
	{
		if (!strcmp(asBodyStats[comp].pName, "Viper Body"))
		{
			for (inc = 0; inc < MAX_PLAYERS; inc++)
			{
				apCompLists[inc][COMP_BODY][comp] = AVAILABLE;
			}
			break;
		}
	}		


	// set the default sensor and ECM 
	for(comp=0; comp < numSensorStats; comp++)
	{
		if (!strcmp(asSensorStats[comp].pName, "Default Sensor"))
		{
			for(inc = 0; inc < MAX_PLAYERS; inc ++)
			{
				aDefaultSensor[inc] = comp;
			}
			break;
		}
	}
	for(comp=0; comp < numECMStats; comp++)
	{
		if (!strcmp(asECMStats[comp].pName, "Light ECM #1"))
		{
			for(inc = 0; inc < MAX_PLAYERS; inc ++)
			{
				aDefaultECM[inc] = comp;
			}
			break;
		}
	}
#endif
#if 0
	//give each player a resource extractor and a power generator
	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		builtGen = builtRes = FALSE;
		//make the structures available in the lists
		for (comp = 0; comp < numStructureStats; comp++)
		{
			if (asStructureStats[comp].type == REF_RESOURCE_EXTRACTOR)
			{
//				apStructTypeLists[inc][comp] = AVAILABLE;
				do
				{
					posX = rand()%mapWidth/2+mapWidth/4;
					posY = rand()%mapHeight/2+mapHeight/4;
				}
				while(blockingTile(posX,posY,TER_ALL));
/*				psStructure = buildStructure(&asStructureStats[comp], posX << 
					TILE_SHIFT + TILE_UNITS/2 , (posY << TILE_SHIFT) + TILE_UNITS/2,
					inc); */
				psStructure = buildStructure(&asStructureStats[comp], 
					posX << TILE_SHIFT, (posY << TILE_SHIFT), inc,FALSE); 
				psStructure->status = SS_BUILT;
				builtRes = TRUE;
			}
			if (asStructureStats[comp].type == REF_POWER_GEN)
			{
//				apStructTypeLists[inc][comp] = AVAILABLE;
				do
				{
					posX = rand()%mapWidth/2+mapWidth/4;
					posY = rand()%mapHeight/2+mapHeight/4;
				}
				while(blockingTile(posX,posY,TER_ALL));
/*				psStructure = buildStructure(&asStructureStats[comp], posX << 
					TILE_SHIFT + TILE_UNITS/2, (posY << TILE_SHIFT) + TILE_UNITS/2, 
					inc); */
				psStructure = buildStructure(&asStructureStats[comp], 
					posX << TILE_SHIFT, posY << TILE_SHIFT, inc,FALSE); 
				psStructure->status = SS_BUILT;
				builtGen = TRUE;
			}
			if (builtRes & builtGen)
			{
				break;
			}
		}
	}
#endif

//#ifdef DEMO
#if	0
	for (inc = 1; inc < MAX_PLAYERS; inc++)
	{
		pCurrTempl = NULL;
		for(pTemplate = apsDroidTemplates[0]; pTemplate; pTemplate=pTemplate->psNext)
		{
			if (createTemplate(pTemplate, &pNewTempl))
			{
				if (pCurrTempl == NULL)
				{
					apsDroidTemplates[inc] = pNewTempl;
				}
				else
				{
					pCurrTempl->psNext = pNewTempl;
				}
				pCurrTempl = pNewTempl;
			}
		}
	}
#endif

	return TRUE;
}


void gameStatEnd(void)
{
	UDWORD	inc;

	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		//free the component lists
		FREE(apCompLists[inc][COMP_BODY]);
		FREE(apCompLists[inc][COMP_BRAIN]);
		FREE(apCompLists[inc][COMP_PROPULSION]);
		FREE(apCompLists[inc][COMP_SENSOR]);
		FREE(apCompLists[inc][COMP_ECM]);
		FREE(apCompLists[inc][COMP_REPAIRUNIT]);
		FREE(apCompLists[inc][COMP_CONSTRUCT]);
		FREE(apCompLists[inc][COMP_WEAPON]);
		//FREE(apCompLists[inc][COMP_PROGRAM]);

		//free the structure lists
		if(apStructTypeLists[inc]) {
			FREE(apStructTypeLists[inc]);
		}
	}
}




//TEST FUNCTION - MAKE EVERYTHING AVAILABLE
void makeAllAvailable(void)
{
	UDWORD	comp,i;

	for(i=0;i<MAX_PLAYERS;i++)
	{
		for (comp=0; comp <numWeaponStats; comp++)
		{
			apCompLists[i][COMP_WEAPON][comp] = AVAILABLE;
		}
		for (comp=0; comp <numBodyStats; comp++)
		{
			apCompLists[i][COMP_BODY][comp] = AVAILABLE;
		}
		for (comp=0; comp <numPropulsionStats; comp++)
		{
			apCompLists[i][COMP_PROPULSION][comp] = AVAILABLE;
		}
		for (comp=0; comp <numSensorStats; comp++)
		{
			apCompLists[i][COMP_SENSOR][comp] = AVAILABLE;
		}
		for (comp=0; comp <numECMStats; comp++)
		{
			apCompLists[i][COMP_ECM][comp] = AVAILABLE;
		}
		for (comp=0; comp <numConstructStats; comp++)
		{
			apCompLists[i][COMP_CONSTRUCT][comp] = AVAILABLE;
		}
		for (comp=0; comp <numBrainStats; comp++)
		{
			apCompLists[i][COMP_BRAIN][comp] = AVAILABLE;
		}
		for (comp=0; comp <numRepairStats; comp++)
		{
			apCompLists[i][COMP_REPAIRUNIT][comp] = AVAILABLE;
		}
		/*for (comp=i; comp <numProgramStats; comp++)
		{
			apCompLists[i][COMP_PROGRAM][comp] = AVAILABLE;
		}*/

		//make all the structures available
		for (comp=0; comp < numStructureStats; comp++)
		{
			apStructTypeLists[i][comp] = AVAILABLE;
		}
		//make all research availble to be performed
		for (comp = 0; comp < numResearch; comp++)
		{
			enableResearch(&asResearch[comp], i);
		}
	}
}

