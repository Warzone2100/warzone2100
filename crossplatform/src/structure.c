/*									   
 * Structure.c
 *
 * Store Structure stats.
 * WARNING!!!!!! 
 * Something wicked this way comes. This file is almost as evil as HCI.c
 */
#include <stdio.h>
#include <assert.h>
/* extra structures required for demo */
//#define DEMO

/* Structure damage printf's */
//#define DEBUG_GROUP1
/* Struct clear tile printf's */
//#define DEBUG_GROUP2
/* fillStructureList printf's */
//#define DEBUG_GROUP3
#include "objects.h"
#include "ai.h"
#include "map.h"
#include "gtime.h"
#include "visibility.h"
#include "structure.h"
#include "research.h"
#include "hci.h"
#include "player.h"
#include "power.h"
#include "miscimd.h"
#include "effects.h"
#include "combat.h"
#include "audio.h"
#include "audio_id.h"
#include "stats.h"
#include "fractions.h"
#include "edit3d.h"
#include "anim_id.h"
#include "anim.h"
#include "display3d.h"
#include "geometry.h"
#include "piematrix.h"
#include "order.h"
#include "droid.h"
#include "script.h"
#include "scripttabs.h"
#include "scriptcb.h"
#include "formationdef.h"
#include "formation.h"
#include "text.h"
#include "action.h"
#include "group.h"
#include "transporter.h"
#include "fpath.h"
#include "mission.h"
#include "levels.h"
#include "console.h"
#include "cmddroid.h"
#include "feature.h"
#include "mapgrid.h"
#include "projectile.h"
#include "cluster.h"
#include "intdisplay.h"
#include "display.h"
#include "difficulty.h"
#include "scriptextern.h"
#include "keymap.h"
#include "game.h"

#include "advvis.h"
#include "multiplay.h" 
#include "netplay.h"
#include "multigifts.h"
#include "loop.h"

#include "scores.h"
#include "gateway.h"

//testing the new electronic warfare for multiPlayer - AB don't want to release with this in the game!!!!!!
//#define TEST_EW 1


#define MODULE_PIES_ENABLED

// Possible types of wall to build
#define WALL_HORIZ		0
#define WALL_VERT		1
#define WALL_CORNER		2

#define STR_RECOIL_TIME	(GAME_TICKS_PER_SEC/4)

// Maximum Distance allowed between a friendly structure and an assembly point.
#define ASSEMBLY_RANGE	(10*TILE_UNITS)	

//Maximium slope of the terrin for building a structure
#define MAX_INCLINE		50//80//40

/* percentage of repair points cost to player power */
#define	REPAIR_FACILITY_COST_PERCENT	10

/* droid construction smoke cloud constants */
#define	DROID_CONSTRUCTION_SMOKE_OFFSET	30
#define	DROID_CONSTRUCTION_SMOKE_HEIGHT	20

//used to calculate how often to increase the resistance level of a structure
#define RESISTANCE_INTERVAL			2000

//used to calculate the time required for rearming
#define REARM_FACTOR                10
//used to calculate the time  required for repairing
#define VTOL_REPAIR_FACTOR          10

//holds the IMD pointers for the modules - ONLY VALID for player0 
iIMDShape * factoryModuleIMDs[NUM_FACTORY_MODULES][NUM_FACMOD_TYPES];
iIMDShape * researchModuleIMDs[NUM_RESEARCH_MODULES];
iIMDShape * powerModuleIMDs[NUM_POWER_MODULES];

//Value is stored for easy access to this structure stat
UDWORD			factoryModuleStat;
UDWORD			powerModuleStat;
UDWORD			researchModuleStat;

//holder for all StructureStats
STRUCTURE_STATS		*asStructureStats;
UDWORD				numStructureStats;
//holder for the limits of each structure per map
STRUCTURE_LIMITS	*asStructLimits[MAX_PLAYERS];

//holds the upgrades attained through research for structure stats
STRUCTURE_UPGRADE	asStructureUpgrade[MAX_PLAYERS];
WALLDEFENCE_UPGRADE	asWallDefenceUpgrade[MAX_PLAYERS];

//holds the upgrades for the functionality of structures through research
RESEARCH_UPGRADE	asResearchUpgrade[MAX_PLAYERS];
POWER_UPGRADE		asPowerUpgrade[MAX_PLAYERS];
REPAIR_FACILITY_UPGRADE		asRepairFacUpgrade[MAX_PLAYERS];
PRODUCTION_UPGRADE	asProductionUpgrade[MAX_PLAYERS][NUM_FACTORY_TYPES];
REARM_UPGRADE		asReArmUpgrade[MAX_PLAYERS];

//used to hold the modifiers cross refd by weapon effect and structureStrength
STRUCTSTRENGTH_MODIFIER		asStructStrengthModifier[WE_NUMEFFECTS][NUM_STRUCT_STRENGTH];

////the three different types of factory (currently) - FACTORY, CYBORG_FACTORY, VTOL_FACTORY
//#define NUM_FACTORY_TYPES	3
//#define FACTORY_FLAG		0
//#define CYBORG_FLAG		1
//#define VTOL_FLAG			2
//specifies which numbers have been allocated for the assembly points for the factories
//UBYTE				factoryNumFlag[MAX_PLAYERS][NUM_FACTORY_TYPES];
UBYTE				factoryNumFlag[MAX_PLAYERS][NUM_FLAG_TYPES];

//the number of different (types of) droids that can be put into a production run

//max number of one type of droid in production run
#define MAX_IN_RUN		9
//the list of what to build - only for selectedPlayer
PRODUCTION_RUN		asProductionRun[NUM_FACTORY_TYPES][MAX_FACTORY][MAX_PROD_RUN];
//stores which player the production list has been set up for
SBYTE               productionPlayer;

/* destroy building construction droid stat pointer */
static	STRUCTURE_STATS	*g_psStatDestroyStruct = NULL;

// store the last time a structure was hit for a side
// this controls when the CALL_STRUCT_ATTACKED is made
//UDWORD	aLastStructHit[MAX_PLAYERS];

// the structure that was last hit
STRUCTURE	*psLastStructHit;

//flag for oil derrick anim
//static	UBYTE	powerGenExists[MAX_PLAYERS];

//flag for drawing radar
static		UBYTE	hqExists[MAX_PLAYERS];
//flag for drawing all sat uplink sees
static		UBYTE	satUplinkExists[MAX_PLAYERS];
//flag for when the player has one built - either completely or partially
static		UBYTE	lasSatExists[MAX_PLAYERS];

static BOOL setFunctionality(STRUCTURE* psBuilding, UDWORD functionType);
static void setFlagPositionInc(void *pFunctionality, UDWORD player, UBYTE factoryType);
static void informPowerGen(STRUCTURE *psStruct);
static BOOL electronicReward(STRUCTURE *psStructure, UBYTE attackPlayer);
static void factoryReward(UBYTE losingPlayer, UBYTE rewardPlayer);
static void repairFacilityReward(UBYTE losingPlayer, UBYTE rewardPlayer);
static void findAssemblyPointPosition(UDWORD *pX, UDWORD *pY, UDWORD player);
static void removeStructFromMap(STRUCTURE *psStruct);
//static void	getNearestBestValidTile(UDWORD *x, UDWORD *y);
static void	structUpdateRecoil( STRUCTURE *psStruct );
static void resetResistanceLag(STRUCTURE *psBuilding);
static void revealAll(UBYTE player);


// last time the maximum units message was displayed
static UDWORD	lastMaxUnitMessage;
#define MAX_UNIT_MESSAGE_PAUSE 20000




// remove a structure from a game without any visible effects
// (for example used to change the type of wall at a location)
//BOOL removeStruct(STRUCTURE *psDel);

//#ifdef DEMO
//BOOL demoStructs(void);
//BOOL createStructureStat(STRUCTURE_STATS *psBuilding, STRUCTURE_STATS *psNewStructure,
//						 UDWORD ref, STRING *pName, UDWORD type);
//void printAvailStructs(void);
//#endif

/* New function from Alex M */
/* Tells you if a point is inside the footprint of a building */

BOOL	ptInStructure(STRUCTURE *psStruct, UDWORD x, UDWORD y)
{
UDWORD	tlX,tlY,brX,brY;
UDWORD	width,height;

	width = (psStruct->pStructureType->baseWidth * TILE_UNITS);
	height = (psStruct->pStructureType->baseBreadth * TILE_UNITS);


	tlX = psStruct->x - (width/2);
	tlY = psStruct->y - (height/2);

	brX = psStruct->x + (width/2);
	brY = psStruct->y + (height/2);

	if(x > tlX AND x < brX)
		if(y > tlY AND y < brY)
			return(TRUE);
	return(FALSE);

}

/*
Check to see if the stats is some kind of expansion module
	
... this replaces the thousands of occurance that is spread through out the code

 ... There were a couple of places where it skipping around a routine if the stat was a expansion module
	(loadSaveStructureV7 & 9) this code seemed suspect, and to clarify it we replaced it with the routine below
... the module stuff seemed to work though ...     TJC (& AB) 8-DEC-98
*/

BOOL IsStatExpansionModule(STRUCTURE_STATS *psStats)
{
	// If the stat is any of the 3 expansion types ... then return TRUE
	if(	psStats->type == REF_POWER_MODULE  ||
		psStats->type == REF_FACTORY_MODULE  ||
		psStats->type == REF_RESEARCH_MODULE )
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
}

void structureInitVars(void)
{
	int i, j;

	for(i=0; i<NUM_FACTORY_MODULES ; i++) {
		factoryModuleIMDs[i][0] = NULL;
		factoryModuleIMDs[i][1] = NULL;
	}
	for(i=0; i<NUM_RESEARCH_MODULES ; i++) {
		researchModuleIMDs[i] = NULL;
	}
	for(i=0; i<NUM_POWER_MODULES ; i++) {
		powerModuleIMDs[i] = NULL;
	}

	asStructureStats = NULL;
	numStructureStats = 0;
	factoryModuleStat = 0;
	powerModuleStat = 0;
	researchModuleStat = 0;
	lastMaxUnitMessage = 0;

	for(i=0; i< MAX_PLAYERS; i++) {
		asStructLimits[i] = NULL;
		//for (j=0; j < NUM_FACTORY_TYPES; j++)
        for (j=0; j < NUM_FLAG_TYPES; j++)
		{
			factoryNumFlag[i][j] = (UBYTE)0;
		}
//		aLastStructHit[i] = 0;
	}
	/*for (i = 0; i < MAX_PLAYERS; i++)
	{
		powerGenExists[i] = FALSE;
	}*/
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		hqExists[i] = FALSE;
        satUplinkExists[i] = FALSE;
        lasSatExists[i] = FALSE;
	}
	//initialise the selectedPlayer's production run
	memset(&asProductionRun, 0, (sizeof(PRODUCTION_RUN) * NUM_FACTORY_TYPES * 
		MAX_FACTORY * MAX_PROD_RUN));
    //set up at beginning of game which player will have a production list
    productionPlayer = (SBYTE)selectedPlayer;
}

/*Initialise the production list and set up the production player*/
void changeProductionPlayer(UBYTE player)
{
    //clear the production run
    memset(&asProductionRun, 0, (sizeof(PRODUCTION_RUN) * NUM_FACTORY_TYPES * 
    	MAX_FACTORY * MAX_PROD_RUN));
    //set this player to have the production list
    productionPlayer = player;
}


/*initialises the flag before a new data set is loaded up*/
void initFactoryNumFlag(void)
{
	UDWORD			i, j;

	for(i=0; i< MAX_PLAYERS; i++) 
	{
		//initialise the flag
		//for (j=0; j < NUM_FACTORY_TYPES; j++)
        for (j=0; j < NUM_FLAG_TYPES; j++)
		{
			factoryNumFlag[i][j] = (UBYTE)0;
		}
	}
}

//called at start of missions
void resetFactoryNumFlag(void)
{
	UDWORD			i;
	STRUCTURE		*psStruct;
	UBYTE			mask;
	FACTORY			*psFactory;

	for(i=0; i< MAX_PLAYERS; i++) 
	{
		//initialise the flag
		//factoryNumFlag[i] = (UBYTE)0;
		
		//look throu the list of structures to see which have been used
		for (psStruct = apsStructLists[i]; psStruct != NULL; psStruct = psStruct->psNext)
		{
			if (psStruct->pStructureType->type == REF_FACTORY)
			{
				psFactory = ((FACTORY*)psStruct->pFunctionality);
				if (psFactory->psAssemblyPoint)
				{
					mask = (UBYTE)(1 << psFactory->psAssemblyPoint->factoryInc);
					factoryNumFlag[i][FACTORY_FLAG] |= mask;
				}
			}
			if (psStruct->pStructureType->type == REF_CYBORG_FACTORY)
			{
				psFactory = ((FACTORY*)psStruct->pFunctionality);
				if (psFactory->psAssemblyPoint)
				{
					mask = (UBYTE)(1 << psFactory->psAssemblyPoint->factoryInc);
					factoryNumFlag[i][CYBORG_FLAG] |= mask;
				}
			}
			if (psStruct->pStructureType->type == REF_VTOL_FACTORY)
			{
				psFactory = ((FACTORY*)psStruct->pFunctionality);
				if (psFactory->psAssemblyPoint)
				{
					mask = (UBYTE)(1 << psFactory->psAssemblyPoint->factoryInc);
					factoryNumFlag[i][VTOL_FLAG] |= mask;
				}
			}
		}
	}
}

void structureType(STRUCTURE_STATS *pStructure, char *pType)
{
	if (!strcmp(pType,"HQ"))
	{
		pStructure->type = REF_HQ;
		return;
	}
	if (!strcmp(pType,"FACTORY"))
	{
		pStructure->type = REF_FACTORY;
		return;
	}
	if (!strcmp(pType,"FACTORY MODULE"))
	{
		pStructure->type = REF_FACTORY_MODULE;
		return;
	}
	if (!strcmp(pType,"RESEARCH"))
	{
		pStructure->type = REF_RESEARCH;
		return;
	}
	if (!strcmp(pType,"RESEARCH MODULE"))
	{
		pStructure->type = REF_RESEARCH_MODULE;
		return;
	}
	if (!strcmp(pType,"POWER GENERATOR"))
	{
		pStructure->type = REF_POWER_GEN;
		return;
	}
	if (!strcmp(pType,"POWER MODULE"))
	{
		pStructure->type = REF_POWER_MODULE;
		return;
	}
	if (!strcmp(pType, "RESOURCE EXTRACTOR"))
	{
		pStructure->type = REF_RESOURCE_EXTRACTOR;
		return;
	}
	if (!strcmp(pType, "DEFENSE"))
	{
		pStructure->type = REF_DEFENSE;
		return;
	}
	if (!strcmp(pType, "WALL"))
	{
		pStructure->type = REF_WALL;
		return;
	}
	if (!strcmp(pType, "CORNER WALL"))
	{
		pStructure->type = REF_WALLCORNER;
		return;
	}
	if (!strcmp(pType, "REPAIR FACILITY"))
	{
		pStructure->type = REF_REPAIR_FACILITY;
		return;
	}
	if (!strcmp(pType, "COMMAND RELAY"))
	{
		pStructure->type = REF_COMMAND_CONTROL;
		return;
	}
	if (!strcmp(pType, "DEMOLISH"))
	{
		pStructure->type = REF_DEMOLISH;
		return;
	}
	if (!strcmp(pType, "CYBORG FACTORY"))
	{
		pStructure->type = REF_CYBORG_FACTORY;
		return;
	}
	if (!strcmp(pType, "VTOL FACTORY"))
	{
		pStructure->type = REF_VTOL_FACTORY;
		return;
	}
	if (!strcmp(pType, "LAB"))
	{
		pStructure->type = REF_LAB;
		return;
	}
	if (!strcmp(pType, "DOOR"))
	{
		pStructure->type = REF_BLASTDOOR;
		return;
	}
	if (!strcmp(pType, "REARM PAD"))
	{
		pStructure->type = REF_REARM_PAD;
		return;
	}
	if (!strcmp(pType, "MISSILE SILO"))
	{
		pStructure->type = REF_MISSILE_SILO;
		return;
	}
	if (!strcmp(pType, "SAT UPLINK"))
	{
		pStructure->type = REF_SAT_UPLINK;
		return;
	}
	ASSERT((FALSE, "Unknown Structure Type"));
}


char *getStructName(STRUCTURE_STATS	 *psStruct)
{
#ifdef HASH_NAMES
	return(	strresGetString(NULL,psStruct->NameHash));	
#else
	return(	getName(psStruct->pName));
#endif
}	

/*returns the structure strength based on the string name passed in */
UBYTE	getStructStrength(STRING *pStrength)
{
	if (!strcmp(pStrength, "SOFT"))
	{
		return STRENGTH_SOFT;
	}
	else if (!strcmp(pStrength, "MEDIUM"))
	{
		return STRENGTH_MEDIUM;
	}
	else if (!strcmp(pStrength, "HARD"))
	{
		return STRENGTH_HARD;
	}
	else if (!strcmp(pStrength, "BUNKER"))
	{
		return STRENGTH_BUNKER;
	}
	else
	{
		return INVALID_STRENGTH;
	}
}


#ifdef MODULE_PIES_ENABLED

void initModulePIEs(char *PIEName,UDWORD i,STRUCTURE_STATS *psStructure)
{
	STRING GfxFile[MAX_NAME_SIZE];
	STRING charNum[2];
	UDWORD length, module = 0;

	strcpy(GfxFile,PIEName);

		//need to work out the IMD's for the modules - HACK!
		if (psStructure->type == REF_FACTORY_MODULE)
		{
			length = strlen(GfxFile) - 5;
			for (module = 1; module < NUM_FACTORY_MODULES+1; module++)
			{
				sprintf(charNum,"%d",module);
				GfxFile[length] = *charNum;
				factoryModuleIMDs[module-1][0] = (iIMDShape*) resGetData("IMD", 
					GfxFile);
				if (factoryModuleIMDs[module-1][0] == NULL)
				{
					DBERROR(("Cannot find the PIE for factory module %d - %s", 
							module, GfxFile));
					return;// FALSE;
				}
			}
			//store the stat for easy access later on
			factoryModuleStat = i;
		}
		if (psStructure->type == REF_VTOL_FACTORY)
		{
			length = strlen(GfxFile) - 5;
			for (module = 1; module < NUM_FACTORY_MODULES+1; module++)
			{
				sprintf(charNum,"%d",module);
				GfxFile[length] = *charNum;
				factoryModuleIMDs[module-1][1] = (iIMDShape*) resGetData("IMD", 
					GfxFile);
				if (factoryModuleIMDs[module-1][1] == NULL)
				{
					DBERROR(("Cannot find the PIE for vtol factory module %d - %s", 
							module, GfxFile));
					return;// FALSE;
				}
			}
		}

		// Setup the PIE's for the research modules.
		if (psStructure->type == REF_RESEARCH_MODULE)
		{
 #ifdef MULTI_UPGRADE
			length = strlen(GfxFile) - 5;
			for (module = 1; module < NUM_RESEARCH_MODULES+1; module++)
			{
				sprintf(charNum,"%d",module);
				GfxFile[length] = *charNum;
				researchModuleIMDs[module-1] = (iIMDShape*) resGetData(
					"IMD", GfxFile);
				if (researchModuleIMDs[module-1] == NULL)
				{
					DBERROR(("Cannot find the PIE for research module %d - %s", 
							module, GfxFile));
					return FALSE;
				}
			}
			//store the stat for easy access later on
			researchModuleStat = i;
 #else
			length = strlen(GfxFile) - 5;


			GfxFile[length] = '4';

			researchModuleIMDs[0] = (iIMDShape*) resGetData("IMD", GfxFile);
			if (researchModuleIMDs[0] == NULL)
			{
				DBERROR(("Cannot find the PIE for research module %d - %s", module, GfxFile));
				return;// FALSE;
			}

			researchModuleIMDs[1] = researchModuleIMDs[0];
			researchModuleIMDs[2] = researchModuleIMDs[0];
			researchModuleIMDs[3] = researchModuleIMDs[0];

			//store the stat for easy access later on
			researchModuleStat = i;
 #endif
		}

		// Setup the PIE's for the power modules.
		if (psStructure->type == REF_POWER_MODULE)
		{
 #ifdef MULTI_UPGRADE
			length = strlen(GfxFile) - 5;
			for (module = 1; module < NUM_POWER_MODULES+1; module++)
			{
				sprintf(charNum,"%d",module);
				GfxFile[length] = *charNum;
				powerModuleIMDs[module-1] = (iIMDShape*) resGetData(
					"IMD", GfxFile);
				if (powerModuleIMDs[module-1] == NULL)
				{
					DBERROR(("Cannot find the PIE for power module %d - %s", 
							module, GfxFile));
					return FALSE;
				}
			}
			//store the stat for easy access later on
			powerModuleStat = i;
 #else
			length = strlen(GfxFile) - 5;

			GfxFile[length] = '4';

			powerModuleIMDs[0] = (iIMDShape*) resGetData("IMD", GfxFile);
			if (powerModuleIMDs[0] == NULL)
			{
				DBERROR(("Cannot find the PIE for power module %d - %s", module, GfxFile));
				return;// FALSE;
			}

			powerModuleIMDs[1] = powerModuleIMDs[0];
			powerModuleIMDs[2] = powerModuleIMDs[0];
			powerModuleIMDs[3] = powerModuleIMDs[0];

			//store the stat for easy access later on
			powerModuleStat = i;
 #endif
		}
}

#else

void initModulePIEsNoMods(char *GfxFile,UDWORD i,STRUCTURE_STATS *psStructure)
{
	UDWORD length, module;

		//need to work out the IMD's for the modules - HACK!
		if (psStructure->type == REF_FACTORY_MODULE)
		{
			factoryModuleIMDs[0][0] = (iIMDShape*) resGetData("IMD", GfxFile);
			if (factoryModuleIMDs[0][0] == NULL)
			{
				DBERROR(("Cannot find the PIE for factory module %d - %s",module, GfxFile));
				return FALSE;
			}
			for (module = 1; module < NUM_FACTORY_MODULES; module++)
			{
				factoryModuleIMDs[module][0] = factoryModuleIMDs[0][0];
			}
			factoryModuleStat = i;
		}

		if (psStructure->type == REF_VTOL_FACTORY)
		{
 			factoryModuleIMDs[0][1] = (iIMDShape*) resGetData("IMD", GfxFile);
			if (factoryModuleIMDs[0][1] == NULL)
			{
				DBERROR(("Cannot find the PIE for vtol factory module %d - %s", module, GfxFile));
				return FALSE;
			}
			for (module = 1; module < NUM_FACTORY_MODULES; module++)
			{
				factoryModuleIMDs[module][1] = factoryModuleIMDs[0][1];
			}
		}

		// Setup the PIE's for the research modules.
		if (psStructure->type == REF_RESEARCH_MODULE)
		{
			length = strlen(GfxFile) - 5;
			GfxFile[length] = '0';
			researchModuleIMDs[0] = (iIMDShape*) resGetData("IMD", GfxFile);
			if (researchModuleIMDs[0] == NULL)
			{
				DBERROR(("Cannot find the PIE for research module %d - %s", module, GfxFile));
				return FALSE;
			}

			researchModuleIMDs[1] = researchModuleIMDs[0];
			researchModuleIMDs[2] = researchModuleIMDs[0];
			researchModuleIMDs[3] = researchModuleIMDs[0];

			//store the stat for easy access later on
			researchModuleStat = i;
		}

		// Setup the PIE's for the power modules.
		if (psStructure->type == REF_POWER_MODULE)
		{
			length = strlen(GfxFile) - 5;
			GfxFile[length] = '0';
			powerModuleIMDs[0] = (iIMDShape*) resGetData("IMD", GfxFile);
			if (powerModuleIMDs[0] == NULL)
			{
				DBERROR(("Cannot find the PIE for power module %d - %s", module, GfxFile));
				return FALSE;
			}

			powerModuleIMDs[1] = powerModuleIMDs[0];
			powerModuleIMDs[2] = powerModuleIMDs[0];
			powerModuleIMDs[3] = powerModuleIMDs[0];

			//store the stat for easy access later on
			powerModuleStat = i;
		}
}
#endif

/* load the Structure stats from the Access database */
BOOL loadStructureStats(SBYTE *pStructData, UDWORD bufferSize)
{
	SBYTE				*pData;
	UDWORD				NumStructures = 0, i, inc, player, numWeaps, weapSlots;
	STRING				StructureName[MAX_NAME_SIZE], foundation[MAX_NAME_SIZE], 
						type[MAX_NAME_SIZE], techLevel[MAX_NAME_SIZE],
						strength[MAX_NAME_SIZE];
	STRING				GfxFile[MAX_NAME_SIZE], baseIMD[MAX_NAME_SIZE];
	STRING				ecmType[MAX_NAME_SIZE],sensorType[MAX_NAME_SIZE];
	STRUCTURE_STATS		*psStructure, *pStartStats;
	ECM_STATS*			pECMType;
	SENSOR_STATS*		pSensorType;
    UDWORD				module;
	//UDWORD				length, module;
	//STRING				charNum[2];
	UDWORD				iID;
    UDWORD              dummyVal;


#ifdef HASH_NAMES
	UDWORD				HashedType;
#endif

#if (MAX_PLAYERS != 8)
	char NotUsedString[MAX_NAME_SIZE];
#endif

	//initialise the module IMD structs
	for (module = 0; module < NUM_FACTORY_MODULES; module++)
	{
		factoryModuleIMDs[module][0] = NULL;
		factoryModuleIMDs[module][1] = NULL;
	}
	for (module = 0; module < NUM_RESEARCH_MODULES; module++)
	{
		researchModuleIMDs[module] = NULL;
	}
	for (module = 0; module < NUM_POWER_MODULES; module++)
	{
		powerModuleIMDs[module] = NULL;
	}


	//keep the start so we release it at the end
	pData = pStructData;

	NumStructures = numCR((UBYTE *)pStructData, bufferSize);

//#ifdef DEMO
//	asStructureStats = (STRUCTURE_STATS *)MALLOC(sizeof(STRUCTURE_STATS)*
//		(NumStructures + NUM_DEMO_STRUCTS));
//	//numStructureStats is added to in in demoStructs()
//#else
	asStructureStats = (STRUCTURE_STATS *)MALLOC(sizeof(STRUCTURE_STATS)*
		NumStructures);
//#endif
	numStructureStats = NumStructures;

	if (asStructureStats == NULL)
	{
		DBERROR(("Structure Stats - Out of memory"));
		return FALSE;
	}

	//save the starting address
	pStartStats = asStructureStats;

	//get the start of the structure_stats storage
	psStructure = asStructureStats;

	for (i=0; i < NumStructures; i++)
	{
		memset(psStructure, 0, sizeof(STRUCTURE_STATS));
		//read the data into the storage - the data is delimeted using comma's
		GfxFile[0] = '\0';
		StructureName[0] = '\0';
		type[0] = '\0';
		strength[0] = '\0';
		foundation[0] = '\0';
		ecmType[0] = '\0';
		sensorType[0] = '\0';
		baseIMD[0] = '\0';

		sscanf(pStructData,"%[^','],%[^','],%[^','],%[^','],%d,%d,%d,%[^','],\
			%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%[^','],%[^','],%d,%[^','],%[^','],\
			%d,%d",
			StructureName, type, techLevel, strength, &psStructure->terrainType,
			&psStructure->baseWidth, &psStructure->baseBreadth, foundation,
			&psStructure->buildPoints, &psStructure->height,
			&psStructure->armourValue, &psStructure->bodyPoints,
			&psStructure->repairSystem, &psStructure->powerToBuild,
			//&psStructure->minimumPower, &psStructure->resistance, 
            &dummyVal, &psStructure->resistance, 
			//&psStructure->quantityLimit, &psStructure->sizeModifier, 
            &dummyVal, &psStructure->sizeModifier, 
			//&ecmType, &sensorType, &psStructure->weaponSlots, &GfxFile,
            ecmType, sensorType, &weapSlots, GfxFile,
			//&baseIMD, &psStructure->numFuncs, &psStructure->numWeaps);
            baseIMD, &psStructure->numFuncs, &numWeaps);

//		DBPRINTF(("%s: height %d\n", StructureName, psStructure->height));

#if (MAX_PLAYERS!=4 && MAX_PLAYERS!=8)
#error Invalid number of players
#endif

//#ifdef FILTER_WALLS
//		if(strcmp(type,"WALL") == 0) {
//			DBPRINTF(("Filtered out WALL : %s\n",StructureName));
//			pStructData = strchr(pStructData,'\n') + 1;
//			continue;
//		}
//		if(strcmp(type,"CORNER WALL") == 0) {
//			DBPRINTF(("Filtered out CORNER WALL : %s\n",StructureName));
//			pStructData = strchr(pStructData,'\n') + 1;
//			continue;
//		}
//#endif

		//allocate storage for the name
 		/*psStructure->pName = (STRING *)MALLOC((strlen(StructureName))+1);
		if (psStructure->pName == NULL)
		{
			DBERROR(("Structure Stats Name - Out of memory"));
			return FALSE;
		}	
		strcpy(psStructure->pName,StructureName);*/
#ifdef HASH_NAMES
		psStructure->NameHash=HashString(StructureName);
#else
		if (!allocateName(&psStructure->pName, StructureName))
		{
			return FALSE;
		}
#endif

		psStructure->ref = REF_STRUCTURE_START + i;

		//determine the structure type
		structureType(psStructure, type);

		//determine the tech level
		if (!setTechLevel((BASE_STATS *)psStructure, techLevel))
		{
			return FALSE;
		}

		//set the struct strength
		psStructure->strength = getStructStrength(strength);
		if (psStructure->strength == INVALID_STRENGTH)
		{
			DBERROR(("loadStructureStats: Unknown structure strength for %s", 
				getStatName(psStructure)));	//->pName));
			return FALSE;
		}

		//get the ecm stats pointer
		if (!strcmp(ecmType,"0"))
		{
			psStructure->pECM = NULL;
		}
		else
		{
			pECMType = asECMStats;
			if (!getResourceName(ecmType))
			{
				return FALSE;
			}	
#ifdef HASH_NAMES
			HashedType=HashString(ecmType);
#endif

			for (inc=0; inc < numECMStats; inc++)
			{
				//compare the names
#ifdef HASH_NAMES
				if (pECMType->NameHash==HashedType)
#else
				if (!strcmp(ecmType, pECMType->pName))
#endif
				{
					psStructure->pECM = pECMType;
					break;
				}
				pECMType++;
			}
		}
		//get the sensor stats pointer
		if (!strcmp(sensorType,"0"))
		{
			psStructure->pSensor = NULL;
		}
		else
		{
			if (!getResourceName(sensorType))
			{
				return FALSE;
			}	
			pSensorType = asSensorStats;
#ifdef HASH_NAMES
			HashedType=HashString(sensorType);
#endif
			for (inc=0; inc < numSensorStats; inc++)
			{
				//compare the names
#ifdef HASH_NAMES
				if (pSensorType->NameHash==HashedType)
#else
				if (!strcmp(sensorType, pSensorType->pName))
#endif
				{
					psStructure->pSensor = pSensorType;
					break;
				}
				pSensorType++;
			}
			//check not allocating a turret sensor if have weapons attached
			ASSERT((psStructure->pSensor != NULL, 
				"loadStructureStats: should have a sensor attached to %s!", StructureName));
			//if (psStructure->pSensor->location == LOC_TURRET AND psStructure->numWeaps)
            if (psStructure->pSensor->location == LOC_TURRET AND numWeaps)
			{
				DBERROR(("loadStructureStats: a Turret Sensor and weapon \
					have been assigned to %s", StructureName));
			}
		}

		//get the IMD for the structure
		psStructure->pIMD = (iIMDShape *) resGetData("IMD", GfxFile);
		if (psStructure->pIMD == NULL)
		{
			DBERROR(("Cannot find the structure PIE for record %s", getStructName(psStructure) ));
			return FALSE;
		}

		if (strcmp(baseIMD, "0"))
		{
			psStructure->pBaseIMD = (iIMDShape *) resGetData("IMD", baseIMD);
			if (psStructure->pIMD == NULL)
			{
				DBERROR(("Cannot find the structure base PIE for record %s", getStructName(psStructure) ));
				return FALSE;
			}
		}
		else
		{
			psStructure->pBaseIMD = NULL;
		}

#ifdef MODULE_PIES_ENABLED
		initModulePIEs(GfxFile,i,psStructure);
#else
		initModulePIEsNoMods(GfxFile,i,psStructure);
#endif

		//allocate storage for the weapons - if any
		//psStructure->defaultWeap = -1;
		//check haven't allocated more than allowed
		//if (psStructure->numWeaps > psStructure->weaponSlots)
        //Only having one weapon per structure now...AB 24/01/99
        if (weapSlots > STRUCT_MAXWEAPS OR numWeaps > weapSlots)
		{
			//DBERROR(("Allocated more weapons than allowed in Access DB for Structure"));
            DBERROR(("Allocated more weapons than allowed for Structure"));
			return FALSE;
		}
        //Don't need to allocate space since thereis only one possible pointer now! AB 24/01/99
		//if (psStructure->numWeaps > 0)
        /*if (numWeaps > 0)
		{
			//psStructure->asWeapList = (WEAPON_STATS **)MALLOC(psStructure->weaponSlots*
			//	sizeof(WEAPON_STATS*));
			psStructure->asWeapList = (WEAPON_STATS **)MALLOC(numWeaps * 
                sizeof(WEAPON_STATS*));
			if (psStructure->asWeapList == NULL)
			{
				DBERROR(("Out of memory assigning structure weapons"));
				return FALSE;
			}
		}*/
		//allocate storage for the functions - if any
		psStructure->defaultFunc = -1;
		if (psStructure->numFuncs > 0)
		{
			psStructure->asFuncList = (FUNCTION **)MALLOC(psStructure->numFuncs*
				sizeof(FUNCTION*));
			if (psStructure->asFuncList == NULL)
			{
				DBERROR(("Out of memory assigning structure Functions"));
				return FALSE;
			}
		}
		//increment the pointer to the start of the next record
		pStructData = strchr(pStructData,'\n') + 1;
		//increment the list to the start of the next storage block
		psStructure++;
	}

//	FREE(pData);
	
	asStructureStats = pStartStats;

	/* get global dummy stat pointer - GJ */
	/*strcpy(StructureName, "A00DemolishStructure");
	iID = getStructStatFromName( StructureName );*/
	for (iID = 0; iID < numStructureStats; iID++)
	{
		if (asStructureStats[iID].type == REF_DEMOLISH)
		{
			break;
		}
	}
	//if ( iID == -1 )
	if (iID > numStructureStats)
	{
		DBERROR( ("intAddObjectStats: destroy structure stat not found\n") );
	}
	g_psStatDestroyStruct = asStructureStats + iID;


//#ifdef DEMO
//	if (!demoStructs())
//	{
//		return FALSE;
//	}
//#endif

	//allocate the structureLimits structure
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		asStructLimits[player] = (STRUCTURE_LIMITS *)MALLOC (sizeof(STRUCTURE_LIMITS) * 
			numStructureStats);
		if (asStructLimits[player] == NULL)
		{
			DBERROR(("Unable to allocate structure limits"));
			return FALSE;
		}
	}
	initStructLimits();

	//initialise the structure upgrade arrays
	memset(asStructureUpgrade, 0, MAX_PLAYERS * sizeof(STRUCTURE_UPGRADE));
	memset(asWallDefenceUpgrade, 0, MAX_PLAYERS * sizeof(WALLDEFENCE_UPGRADE));
	memset(asResearchUpgrade, 0, MAX_PLAYERS * sizeof(RESEARCH_UPGRADE));
	memset(asPowerUpgrade, 0, MAX_PLAYERS * sizeof(POWER_UPGRADE));
	memset(asRepairFacUpgrade, 0, MAX_PLAYERS * sizeof(REPAIR_FACILITY_UPGRADE));
	memset(asProductionUpgrade, 0, MAX_PLAYERS * NUM_FACTORY_TYPES * 
		sizeof(PRODUCTION_UPGRADE));
	memset(asReArmUpgrade, 0, MAX_PLAYERS * sizeof(REARM_UPGRADE));
	
	return TRUE;
}

//initialise the structure limits structure
void initStructLimits(void)
{
	UDWORD				i, player;
	STRUCTURE_LIMITS	*psStructLimits;

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		psStructLimits = asStructLimits[player];
		for (i=0; i < numStructureStats; i++)
		{
			psStructLimits[i].limit = LOTS_OF;
			psStructLimits[i].currentQuantity = 0;

			psStructLimits[i].globalLimit = LOTS_OF;

		}
	}
}

/* set the current number of structures of each type built */
void setCurrentStructQuantity(BOOL displayError)
{
	UDWORD		player, inc;
	STRUCTURE	*psCurr;
	STRUCTURE_LIMITS	*psStructLimits;

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		psStructLimits = asStructLimits[player];
		//initialise the current quantity for all structures
		for (inc = 0; inc < numStructureStats; inc++)
		{
			psStructLimits[inc].currentQuantity = 0;
		}

		for (psCurr = apsStructLists[player]; psCurr != NULL; psCurr = 
			psCurr->psNext)
		{
			inc = psCurr->pStructureType - asStructureStats;
			psStructLimits[inc].currentQuantity++;
			if (displayError)
			{
				//check quantity never exceeds the limit
				if (psStructLimits[inc].currentQuantity > psStructLimits[inc].limit)
				{
					ASSERT((FALSE, "There appears to be too many %s on this map!", 
						getStructName(&asStructureStats[inc] )));
				}
			}
		}
/*		//also check the mission structures lists

		for (psCurr = mission.apsStructLists[player]; psCurr != NULL; psCurr = 
			psCurr->psNext)
		{
			inc = psCurr->pStructureType - asStructureStats;
			psStructLimits[inc].currentQuantity++;
			if (displayError)
			{
				//check quantity never exceeds the limit
				if (psStructLimits[inc].currentQuantity > psStructLimits[inc].limit)
				{
					ASSERT((FALSE, "There appears to be too many %s on this map!", 
						getStructName(&asStructureStats[inc] )));
				}
			}
		}
*/
	}
}

//Load the weapons assigned to Structure in the Access database
BOOL loadStructureWeapons(SBYTE *pWeaponData, UDWORD bufferSize)
{
	SBYTE				*pStartWeaponData;
	UDWORD				NumToAlloc = 0, i,incS, incW;
	STRING				StructureName[MAX_NAME_SIZE], WeaponName[MAX_NAME_SIZE];
	STRUCTURE_STATS		*pStructure = asStructureStats;
	WEAPON_STATS		*pWeapon = asWeaponStats;
	BOOL				weaponFound, structureFound;

#ifdef HASH_NAMES
	UDWORD				StructureHash;
	UDWORD				WeaponHash;
#endif


	pStartWeaponData = pWeaponData;

	NumToAlloc = numCR((UBYTE *)pWeaponData, bufferSize);

	for (i=0; i < NumToAlloc; i++)
	{
		//read the data into the storage - the data is delimeted using comma's
		StructureName[0] = '\0';
		WeaponName[0] = '\0';
		sscanf(pWeaponData, "%[^','],%[^','],%*d", StructureName, WeaponName);
	
		if (!getResourceName(StructureName))
		{
			return FALSE;
		}
		if (!getResourceName(WeaponName))
		{
			return FALSE;
		}
		weaponFound = structureFound = FALSE;
		//loop through each Structure_Stat to compare the name

#ifdef HASH_NAMES
		StructureHash=HashString(StructureName);
		WeaponHash=HashString(WeaponName);
#endif

		for (incS=0; incS < numStructureStats; incS++)
		{

#ifdef HASH_NAMES
			if (pStructure[incS].NameHash==StructureHash)
#else
			if (!(strcmp(StructureName, pStructure[incS].pName)))
#endif
			{
				//Structure found, so loop through each weapon
				structureFound = TRUE;
				for (incW=0; incW < numWeaponStats; incW++)
				{
#ifdef HASH_NAMES
					if (pWeapon[incW].NameHash==WeaponHash)
#else
					if (!(strcmp(WeaponName, pWeapon[incW].pName)))
#endif
					{
						weaponFound = TRUE;
						//weapon found alloc this weapon to the current Structure
						//pStructure[incS].defaultWeap++;
						//pStructure[incS].asWeapList[pStructure[incS].defaultWeap] = 
						//	&pWeapon[incW];
						//check not allocating more than allowed
						//if (pStructure[incS].defaultWeap > 
						//				(SDWORD)pStructure[incS].weaponSlots)
                        //see if we have already allocated one
                        if (pStructure[incS].psWeapStat != NULL)
						{
							DBERROR(("Trying to allocate more weapons than allowed for Structure"));
							return FALSE;
						}
                        pStructure[incS].psWeapStat = &pWeapon[incW];
						break;
					}
				}
				//if weapon not found - error
				if (!weaponFound)
				{
					DBERROR(("Unable to find stats for weapon %s for structure %s", 
						WeaponName, StructureName));
					return FALSE;
				}
			}
		}
		//if structure not found - error
		if (!structureFound)
		{
			DBERROR(("Unable to find stats for structure %s", StructureName));
			return FALSE;
		}
		//increment the pointer to the start of the next record
		pWeaponData = strchr(pWeaponData,'\n') + 1;
	}
//	FREE(pStartWeaponData);
	return TRUE;
}

//Load the programs assigned to Droids in the Access database
BOOL loadStructureFunctions(SBYTE *pFunctionData, UDWORD bufferSize)
{
	SBYTE				*pStartFunctionData;
	UDWORD				NumToAlloc = 0,i,incS, incF;
	STRING				StructureName[MAX_NAME_SIZE], FunctionName[MAX_NAME_SIZE];
	STRUCTURE_STATS		*pStructure = asStructureStats;
	FUNCTION			*pFunction, **pStartFunctions = asFunctions;
	BOOL				functionFound, structureFound;
#ifdef HASH_NAMES
	UDWORD				StructureHash;
	UDWORD				FunctionHash;
#endif

	pStartFunctionData = pFunctionData;

	NumToAlloc = numCR((UBYTE *)pFunctionData, bufferSize);

	for (i=0; i < NumToAlloc; i++)
	{
		//read the data into the storage - the data is delimeted using comma's
		StructureName[0] = '\0';
		FunctionName[0] = '\0';
		sscanf(pFunctionData, "%[^','],%[^','],%*d", StructureName, FunctionName);
		functionFound = structureFound = FALSE;
	
		if (!getResourceName(StructureName))
		{
			return FALSE;
		}
		/*if (!getResourceName(FunctionName))
		{
			return FALSE;
		}*/
#ifdef HASH_NAMES
		StructureHash=HashString(StructureName);
		FunctionHash=HashString(FunctionName);
#endif

		//loop through each Structure_Stat to compare the name
		for (incS=0; incS < numStructureStats; incS++)
		{
#ifdef HASH_NAMES
			if (pStructure[incS].NameHash==StructureHash)
#else
			if (!(strcmp(StructureName, pStructure[incS].pName)))
#endif
			{
				//Structure found, so loop through each Function
				structureFound = TRUE;
				pStartFunctions = asFunctions;
				for (incF=0; incF < numFunctions; incF++)
				{
					pFunction = *pStartFunctions;
#ifdef HASH_NAMES
					if (pFunction->NameHash==FunctionHash)
#else
					if (!(strcmp(FunctionName, pFunction->pName)))
#endif
					{
						//function found alloc this function to the current Structure
						functionFound = TRUE;
						pStructure[incS].defaultFunc++;
						//check not allocating more than allowed
						if (pStructure[incS].defaultFunc > 
										(SDWORD)pStructure[incS].numFuncs)
						{
							DBERROR(("Trying to allocate more functions than allowed for Structure"));
							return FALSE;
						}
						pStructure[incS].asFuncList[pStructure[incS].defaultFunc] = 
							pFunction;
						break;
					}
					pStartFunctions++;
				}
				//if function not found - error
				if (!functionFound)
				{
					DBERROR(("Unable to find stats for function %s for structure %s", 
						FunctionName, StructureName));
					return FALSE;
				}
			}
		}
		//if structure not found - error
		if (!structureFound)
		{
			DBERROR(("Unable to find stats for structure %s", StructureName));
			return FALSE;
		}
		//increment the pointer to the start of the next record
		pFunctionData = strchr(pFunctionData,'\n') + 1;
	}
//	FREE(pStartFunctionData);

	/**************************************************************************/
	//Wall Function requires a structure stat so can allocate it now
	pStartFunctions = asFunctions;
	for (incF=0; incF < numFunctions; incF++)
	{
		pFunction = *pStartFunctions;
		if (pFunction->type == WALL_TYPE)
		{
			//loop through all structures to find the stat
			pStructure = asStructureStats;
			((WALL_FUNCTION *)pFunction)->pCornerStat = NULL;

			for (i=0; i < numStructureStats; i++)
			{
				//compare the names
#ifdef HASH_NAMES
				if (((WALL_FUNCTION *)pFunction)->StructNameHash == pStructure->NameHash)
#else
				if (!strcmp(((WALL_FUNCTION *)pFunction)->pStructName, pStructure->pName))
#endif
				{
					((WALL_FUNCTION *)pFunction)->pCornerStat = pStructure;
					break;
				}
				pStructure++;
			}
			//if haven't found the STRUCTURE STAT, then problem
			if (!((WALL_FUNCTION *)pFunction)->pCornerStat)
			{
#ifdef HASH_NAMES
				DBERROR(("Unknown Corner Wall stat for function %x", pFunction->NameHash));
#else
				DBERROR(("Unknown Corner Wall stat for function %s", pFunction->pName));
#endif
				return FALSE;
			}
		}
		pStartFunctions++;
	}

	return TRUE;
}

/*Load the Structure Strength Modifiers from the file exported from Access*/
BOOL loadStructureStrengthModifiers(SBYTE *pStrengthModData, UDWORD bufferSize)
{
	STRUCT_STRENGTH		strengthInc;
	WEAPON_EFFECT		effectInc;
	UDWORD				NumRecords = 0, i, j, modifier;
	STRING				weaponEffectName[MAX_NAME_SIZE], strengthName[MAX_NAME_SIZE];

	//memset(asStructStrengthModifier, 0, (sizeof(STRUCTSTRENGTH_MODIFIER) *
	//	WE_NUMEFFECTS * NUM_STRUCT_STRENGTH));
	//initialise to 100%
	for (i=0; i < WE_NUMEFFECTS; i++)
	{
		for (j=0; j < NUM_STRUCT_STRENGTH; j++)
		{
			asStructStrengthModifier[i][j] = 100;
		}
	}
	
	NumRecords = numCR((UBYTE *)pStrengthModData, bufferSize);

	for (i=0; i < NumRecords; i++)
	{
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pStrengthModData,"%[^','],%[^','],%d",
			weaponEffectName, strengthName, &modifier);

		//get the weapon effect inc
		effectInc = getWeaponEffect(weaponEffectName);
		if (effectInc == INVALID_WEAPON_EFFECT)
		{
			DBERROR(("loadStructureStrengthModifiers: Invalid Weapon Effect - %s", 
				weaponEffectName));
			return FALSE;
		}
		//get the propulsion inc
		strengthInc = getStructStrength(strengthName);
		if (strengthInc == INVALID_STRENGTH)
		{
			DBERROR(("loadStructureStrengthModifiers: Invalid Strength type - %s", 
				strengthName));
			return FALSE;
		}

		if (modifier > UWORD_MAX)
		{
			DBERROR(("loadStructureStrengthModifiers: modifier for effect %s, strength %s is too large", 
				weaponEffectName, strengthName));
			return FALSE;
		}
		//store in the appropriate index
		asStructStrengthModifier[effectInc][strengthInc] = (UWORD)modifier;

		//increment the pointer to the start of the next record
		pStrengthModData = strchr(pStrengthModData,'\n') + 1;
	}

	return TRUE;
}


BOOL structureStatsShutDown(void)
{	
	UDWORD	inc;
	STRUCTURE_STATS*	pStructure = asStructureStats;

	for(inc=0; inc < numStructureStats; inc++, pStructure++)
	{
//#ifndef RESOURCE_NAMES
#if !defined (RESOURCE_NAMES) && !defined (STORE_RESOURCE_ID)
		FREE(pStructure->pName);
#endif
		/*if (pStructure->numWeaps > 0)
		{
			FREE(pStructure->asWeapList);
		}*/
		if (pStructure->numFuncs > 0)
		{
			FREE(pStructure->asFuncList);
		}
	}

	if(numStructureStats) {
		FREE(asStructureStats);
	}

	//free up the structLimits structure
	for (inc = 0; inc < MAX_PLAYERS ; inc++)
	{
		if(asStructLimits[inc]) {
			FREE(asStructLimits[inc]);
		}
	}

	return TRUE;
}


/* Do damage to a Structure.
 * Returns TRUE if the Structure is destroyed
 */
BOOL structureDamage(STRUCTURE *psStructure, UDWORD damage, UDWORD weaponClass, 
                     UDWORD weaponSubClass)
{
	UDWORD		penDamage, armourDamage;

	ASSERT((PTRVALID(psStructure, sizeof(STRUCTURE)),
		"structureDamage: Invalid Structure pointer"));

	DBP1(("structureDamage(%d): body %d armour %d damage: %d\n",
		psStructure->id, psStructure->body, psStructure->armour, damage));

    //EMP cannons do not work on Structures
    if (weaponSubClass == WSC_EMP)
    {
        return FALSE;
    }

//	if(selectedPlayer==0)
	if(psStructure->player != selectedPlayer)
	{	
		// Player inflicting damage on enemy.
		damage = (UDWORD) modifyForDifficultyLevel( (SDWORD) damage,TRUE);
	} else {
		// Enemy inflicting damage on player.
		damage = (UDWORD) modifyForDifficultyLevel( (SDWORD) damage,FALSE);
	}

	//store the time it was hit
	psStructure->timeLastHit = gameTime;

	// tell the cluster system it has been attacked
	clustObjectAttacked((BASE_OBJECT *)psStructure);

	if (damage > psStructure->armour)
	{
		/* Damage has penetrated - reduce armour and body points */
		penDamage = damage - psStructure->armour;
		DBP1(("        penetrated: %d\n", penDamage));
		if (penDamage >= psStructure->body)
		{
			/* structure destroyed */
			DBP1(("        DESTROYED\n"));
			return destroyStruct(psStructure);
		}
		else
		{
			psStructure->body = (UWORD)(psStructure->body  - (UWORD)penDamage);
		}

		/* Do damage to armour */
		armourDamage = (damage / PEN_ARMOUR_DAMAGE_FACTOR) + 1;

//		DBP1(("penetrated: %d, armour: %d\n", penDamage, armourDamage));
	}
	else
	{
		/* Damage didn't penetrate - only reduce armour */
		armourDamage = (damage / ARMOUR_DAMAGE_FACTOR) + 1;

		/* Do one point of damage to body */
		DBP1(("        not penetrated - 1 point damage\n"));
		if (psStructure->body == 1)
		{
			DBP1(("        DESTROYED\n"));
			return destroyStruct(psStructure);
//			return TRUE;
		}
		else
		{
			psStructure->body -= 1;
		}

//		DBP1(("armour: %d\n", armourDamage));
	}

	/* Actually reduce the Structure's armour */
#if 0
	if (armourDamage >= psStructure->armour)
	{
		psStructure->armour = 0;
	}
	else
	{
		psStructure->armour -= armourDamage;
	}
#endif

	DBP1(("        body left: %d armour left: %d\n",
		psStructure->body, psStructure->armour));

    //only overwrite if the last weapon to hit was not an EMP - need the time value for this
    if (psStructure->lastHitWeapon != WSC_EMP)
    {
	    psStructure->timeLastHit = gameTime;
	    psStructure->lastHitWeapon = weaponSubClass;
    }

	return FALSE;
}


/* Set the type of droid for a factory to build */
BOOL structSetManufacture(STRUCTURE *psStruct, DROID_TEMPLATE *psTempl, UBYTE quantity)
{
	FACTORY		*psFact;

	ASSERT((PTRVALID(psStruct, sizeof(STRUCTURE)) && psStruct->type == OBJ_STRUCTURE &&
			(psStruct->pStructureType->type == REF_FACTORY OR
			psStruct->pStructureType->type == REF_CYBORG_FACTORY OR
			psStruct->pStructureType->type == REF_VTOL_FACTORY),
		"structSetManufacture: invalid Factory pointer"));
	/* psTempl might be NULL if the build is being cancelled in the middle */
	ASSERT((psTempl == NULL || PTRVALID(psTempl, sizeof(DROID_TEMPLATE)),
		"structSetManufacture: invalid Template pointer"));
	
	//assign it to the Factory
	psFact = (FACTORY*)psStruct->pFunctionality;
	psFact->psSubject = (BASE_STATS *)psTempl;

	//set up the start time and build time
	if (psTempl != NULL)
	{
		//only use this for non selectedPlayer
		if (psStruct->player != selectedPlayer)
		{
			//set quantity to produce 
			psFact->quantity = quantity;
		}

		psFact->timeStarted = ACTION_START_TIME;//gameTime;
		psFact->powerAccrued = 0;
		psFact->timeStartHold = 0;
		
		psFact->timeToBuild = psTempl->buildPoints / psFact->productionOutput;
		//check for zero build time - usually caused by 'silly' data!
		if (psFact->timeToBuild == 0)
		{
			//set to 1/1000th sec - ie very fast!
			psFact->timeToBuild = 1;
		}
	}
	return TRUE;
}


/*****************************************************************************/
/*
 * All this wall type code is horrible, but I really can't think of a better way to do it.
 *        John.
 */

// look at where other walls are to decide what type of wall to build
static SDWORD structWallScan(BOOL aWallPresent[5][5], SDWORD x,SDWORD y)
{
	SDWORD cx,cy, numWalls;

	numWalls = 0;
	for(cx = x-1; cx <= x+1; cx += 1)
	{
		for(cy = y-1; cy <= y+1; cy += 1)
		{
			// do not look at walls diagonally from this wall
			if (((cx == x && cy != y) ||
				 (cx != x && cy == y))
				&& aWallPresent[cx][cy])
			{
				numWalls += 1;
			}
		}
	}

	// Now decide what type of wall is needed
	if (numWalls == 1)
	{
		if (aWallPresent[x-1][y] || aWallPresent[x+1][y])
		{
			// there is a wall horizontally adjacent
			return WALL_HORIZ;
		}
		else
		{
			return WALL_VERT;
		}
	}
	else if (numWalls == 2)
	{
		if (aWallPresent[x-1][y] && aWallPresent[x+1][y])
		{
			// there is a wall horizontally adjacent
			return WALL_HORIZ;
		}
		else if (aWallPresent[x][y-1] && aWallPresent[x][y+1])
		{
			// there is a wall vertically adjacent
			return WALL_VERT;
		}
		else
		{
			return WALL_CORNER;
		}
	}
	else if (numWalls > 2)
	{
		// definately need a corner wall
		return WALL_CORNER;
	}

	return WALL_VERT;
}

// Choose a type of wall for a location - and update any neighbouring walls
SDWORD structChooseWallType(UDWORD player, UDWORD mapX, UDWORD mapY)
{
	BOOL		aWallPresent[5][5];
	SDWORD		xdiff,ydiff, x,y;
	STRUCTURE	*psStruct;
	STRUCTURE	*apsStructs[5][5];
	SDWORD		nayborType, scanType;
	STRUCTURE_STATS	*psStats;
	UDWORD		sx,sy;
	UDWORD		oldBuildPoints;

	// scan around the location looking for walls
	memset(aWallPresent, 0, sizeof(aWallPresent));
	for(psStruct=apsStructLists[player]; psStruct; psStruct=psStruct->psNext)
	{
		xdiff = (SDWORD)mapX - ((SDWORD)psStruct->x >> TILE_SHIFT);
		ydiff = (SDWORD)mapY - ((SDWORD)psStruct->y >> TILE_SHIFT);
		if (xdiff >= -2 && xdiff <= 2 &&
			ydiff >= -2 && ydiff <= 2 &&
			(psStruct->pStructureType->type == REF_WALL ||
			 psStruct->pStructureType->type == REF_WALLCORNER ||
			 psStruct->pStructureType->type == REF_DEFENSE))
		{
			aWallPresent[xdiff+2][ydiff+2] = TRUE;
			apsStructs[xdiff+2][ydiff+2] = psStruct;
		}
	}
	// add in the wall about to be built
	aWallPresent[2][2] = TRUE;

	// now make sure that all the walls around this one are OK
	for(x=1; x<=3; x+=1)
	{
		for(y=1; y<=3; y+=1)
		{
			// do not look at walls diagonally from this wall
			if (((x == 2 && y != 2) ||
				 (x != 2 && y == 2)) &&
				aWallPresent[x][y])
			{
				// figure out what type the wall currently is
				psStruct = apsStructs[x][y];
				if (psStruct->pStructureType->type == REF_WALL)
				{
					if (psStruct->direction == 90)
					{
						nayborType = WALL_VERT;
					}
					else
					{
						nayborType = WALL_HORIZ;
					}
				}
				else
				{
					// do not need to adjust anything apart from walls
					continue;
//					nayborType = WALL_CORNER;
				}

				// see what type the wall should be
				scanType = structWallScan(aWallPresent, x,y);

				if (nayborType != scanType)
				{
					// Got to change the wall
					if (scanType == WALL_CORNER)
					{
						// change to a corner
						if (psStruct->pStructureType->asFuncList[0]->type == WALL_TYPE)
						{
							/* Still being built - so save and load build points */
							if(psStruct->status == SS_BEING_BUILT)
							{
								oldBuildPoints = psStruct->currentBuildPts;
								psStats = ((WALL_FUNCTION *)psStruct
												->pStructureType->asFuncList[0])
														->pCornerStat;
								sx = psStruct->x; sy = psStruct->y;
								removeStruct(psStruct, TRUE);
								powerCalc(FALSE);
								psStruct = buildStructure(psStats, sx,sy, player, TRUE);
								powerCalc(TRUE);
								if(psStruct !=NULL)
								{
									psStruct->status = SS_BEING_BUILT;
									psStruct->currentBuildPts = (SWORD)oldBuildPoints;
								}
							}
							else
							{
								psStats = ((WALL_FUNCTION *)psStruct
												->pStructureType->asFuncList[0])
														->pCornerStat;
								sx = psStruct->x; sy = psStruct->y;
								removeStruct(psStruct, TRUE);
								powerCalc(FALSE);
								psStruct = buildStructure(psStats, sx,sy, player, TRUE);
								powerCalc(TRUE);
								if(psStruct !=NULL)
								{
									psStruct->status = SS_BUILT;
									buildingComplete(psStruct);
								}
							}
						}
					}
					else if (scanType == WALL_HORIZ)
					{
						// change to a horizontal wall
						psStruct->direction = 0;
					}
					else
					{
						// change to a vertical wall
						psStruct->direction = 90;
					}
				}
			}
		}
	}

	// finally return the type for this wall
	return structWallScan(aWallPresent, 2,2);
}


// Set the tile no draw flags for a structure
void setStructTileDraw(STRUCTURE *psStruct)
{
	STRUCTURE_STATS		*pStructureType = psStruct->pStructureType;
	UDWORD	width, breadth, mapX,mapY;

	mapX = (psStruct->x >> TILE_SHIFT) - (pStructureType->baseWidth/2);
	mapY = (psStruct->y >> TILE_SHIFT) - (pStructureType->baseBreadth/2);
	for (width = 0; width < pStructureType->baseWidth; width++)
	{
		for (breadth = 0; breadth < pStructureType->baseBreadth; breadth++)
		{
			if (pStructureType->pBaseIMD)
			{
				SET_TILE_NODRAW(mapTile(mapX+width,mapY+breadth));
			}
		}
	}
}



VOID buildFlatten(STRUCTURE_STATS *pStructureType, UDWORD atx, UDWORD aty,UDWORD h )
{
	UDWORD				x = atx >> TILE_SHIFT;
	UDWORD				y = aty >> TILE_SHIFT;
	UBYTE				width;
	UBYTE				breadth;
    //STRUCTURE           *psStruct;

	for (breadth=0; breadth <= (UBYTE)(pStructureType->baseBreadth/* + 1*/); breadth++)
	{
		for (width=0; width <= (UBYTE)(pStructureType->baseWidth /*+ 1*/); width++)
		{	
//			while((x+width-1)<0)width++;
//			while((y+breadth-1)<0)breadth++;

			if ((pStructureType->type != REF_WALL) &&
				(pStructureType->type != REF_WALLCORNER) ) /*&&*/
/*				(pStructureType->type != REF_DEFENSE))*/
			{
				setTileHeight(x + width, y + breadth, h);//-1
			    // We need to raise features on raised tiles to the new height
			    if(TILE_HAS_FEATURE(mapTile(x+width,y+breadth)))
			    {
				    getTileFeature(x+width, y+breadth)->z = (UWORD)h;
			    }
                //and defensive structures - the code for drawing defensive structures seems to cope
                /*if(TILE_HAS_STRUCTURE(mapTile(x+width,y+breadth)))
                {
                    psStruct = getTileStructure(x+width, y+breadth);
                    if (psStruct)
                    {
                        psStruct->z = (UWORD)h;
                    }
                }*/
			}
		}
	}
	return ;
}

/*Builds an instance of a Structure - the x/y passed in are in world coords. */
STRUCTURE* buildStructure(STRUCTURE_STATS* pStructureType, UDWORD x, UDWORD y, 
						  UDWORD player,BOOL FromSave)
{
	STRUCTURE*	psBuilding;
	FEATURE		*psFeature, *psFNext;
	//MESSAGE		*psMessage;
	UDWORD		mapX, mapY, mapH;
	UDWORD		width, breadth, weapon, capacity, bodyDiff = 0;
	SDWORD		wallType = 0, preScrollMinX = 0, preScrollMinY = 0, preScrollMaxX = 0, preScrollMaxY = 0;
	MAPTILE		*psTile;
	BOOL		bUpgraded;
//	BOOL bTemp=FALSE;
	UDWORD		min,max;

//#ifdef FILTER_WALLS
//		if( (pStructureType->type == REF_WALL) || (pStructureType->type == REF_WALLCORNER) ) {
//			DBPRINTF(("Filtered out WALL\n"));
//			return NULL;
//		}
//#endif

	if (IsStatExpansionModule(pStructureType)==FALSE)
	{
        //this happends gradually now - AB 5/1/99
	    /*subtract the power required to build
	    if (!usePower(player, pStructureType->powerToBuild))
	    {
		    return NULL;
	    }*/

        //some prelim tests...
        max = pStructureType - asStructureStats;
        if (max > numStructureStats)
        {
            ASSERT((FALSE, "buildStructure:Invalid structure type"));
            return NULL;
        }

        if (player == selectedPlayer)
        {
            //don't allow more than interface limits of certain structures
            if (pStructureType->type == REF_FACTORY OR
			    pStructureType->type == REF_CYBORG_FACTORY OR
			    pStructureType->type == REF_VTOL_FACTORY)
            {
                //NEVER EVER EVER WANT MORE THAN 5 FACTORIES
                if (asStructLimits[selectedPlayer][max].currentQuantity > MAX_FACTORY)
                {
                    ASSERT((FALSE, 
                        "buildStructure: trying to build too many factories"));
                    return NULL;
                }
            }
            if (pStructureType->type == REF_RESEARCH)
            {
                //can only cope with MAX_OBJECTS research facilities
                if (asStructLimits[selectedPlayer][max].currentQuantity > MAX_OBJECTS)
                {
                    ASSERT((FALSE, 
                        "buildStructure: trying to build too many research facilities"));
                    return NULL;
                }
            }
            //HARD_CODE don't ever want more than one Las Sat structure
            if (pStructureType->psWeapStat AND pStructureType->psWeapStat->
                weaponSubClass == WSC_LAS_SAT AND getLasSatExists(selectedPlayer))
            {
                return NULL;
            }
            //HARD_CODE don't ever want more than one Sat Uplink structure
            if (pStructureType->type == REF_SAT_UPLINK)
            {
                if (asStructLimits[selectedPlayer][max].currentQuantity > 0)
                {
                    ASSERT((FALSE, 
                        "buildStructure: trying to build too many Sat Uplinks"));
                    return NULL;
                }
            }
        }

		// snap the coords to a tile
		x = ((pStructureType->baseWidth % 2) == 0) ? (x & ~TILE_MASK) : (x & ~TILE_MASK) + TILE_UNITS/2;
		y = ((pStructureType->baseBreadth % 2) == 0) ? (y & ~TILE_MASK) : (y & ~TILE_MASK) + TILE_UNITS/2;

        //check not trying to build too near the edge
    	if(((x >> TILE_SHIFT) < TOO_NEAR_EDGE) || ((x >> TILE_SHIFT) > (SDWORD)(
            mapWidth - TOO_NEAR_EDGE)))
        {
            ASSERT((FALSE, "buildStructure: x coord too near edge"));
	    	return NULL;
        }
	    if(((y >> TILE_SHIFT) < TOO_NEAR_EDGE) || ((y >> TILE_SHIFT) > (SDWORD)(
            mapHeight - TOO_NEAR_EDGE)))
        {
            ASSERT((FALSE, "buildStructure: y coord too near edge"));
		    return NULL;
        }

		if (!FromSave && pStructureType->type == REF_WALL)
		{
			wallType = structChooseWallType(player, x >> TILE_SHIFT, y >> TILE_SHIFT);
			if (wallType == WALL_CORNER)
			{
				if (pStructureType->asFuncList[0]->type == WALL_TYPE)
				{
					pStructureType = ((WALL_FUNCTION *)pStructureType->asFuncList[0])->pCornerStat;
				}
			}
		}
/*		if (!FromSave && pStructureType->type == REF_DEFENSE)
		{
			// get any walls next to a defensive structure to adjust
			structChooseWallType(player, x >> TILE_SHIFT, y >> TILE_SHIFT);
		}*/
	
		if (pStructureType->type == REF_DEFENSE &&
			mapTile(x >> TILE_SHIFT, y >> TILE_SHIFT)->tileInfoBits & BITS_WALL)
		{
			// building a gun tower over a wall, replace it
			psBuilding = getTileStructure(x >> TILE_SHIFT, y >> TILE_SHIFT);
			if (psBuilding != NULL)
			{
				removeStruct(psBuilding, TRUE);
			}
		}

		//try and create the Structure
		if (!createStruct(player, &psBuilding))
		{
			return NULL;
		}
//DBPRINTF(("create structure called\n");
//if (PTRVALID(psBuilding,sizeof(STRUCTURE)) != TRUE) DBPRINTF(("...and its not valid\n");

		psBuilding->psCurAnim = NULL;

		/* GJ HACK! - add anim to deriks */
		/*if ( pStructureType->type == REF_RESOURCE_EXTRACTOR )
		{
			// add anim now if loading from save file
			if ( FromSave == TRUE )
			{
				psBuilding->psCurAnim = animObj_Add( psBuilding, ID_ANIM_DERIK, 0, 0 );
			}
		}*/
	
		//fill in other details
		psBuilding->pStructureType = pStructureType;

		//copy the name across for now
		//psBuilding->pName = pStructureType->pName;
//		if(FromSave==TRUE) {
			psBuilding->x = (UWORD)x;
			psBuilding->y = (UWORD)y;
/*		} else {
			psBuilding->x = (UWORD)((x & (~TILE_MASK)) + pStructureType->
				baseWidth * TILE_UNITS / 2);
			psBuilding->y = (UWORD)((y & (~TILE_MASK)) + pStructureType->
				baseBreadth * TILE_UNITS / 2);
		}*/

		//This needs to be done before the functionality bit...
		//load into the map data and structure list if not an upgrade
//		if(FromSave==TRUE) {
			mapX = (x >> TILE_SHIFT) - (pStructureType->baseWidth/2);
			mapY = (y >> TILE_SHIFT) - (pStructureType->baseBreadth/2);
/*		} else {
			mapX = x >> TILE_SHIFT;
			mapY = y >> TILE_SHIFT;
		}*/

		//set up the imd to use for the display
		psBuilding->sDisplay.imd = pStructureType->pIMD;

//		psBuilding->sDisplay.animFrame = 0;

//DBPRINTF(("%d\n",psBuilding->sDisplay.imd->ymax);

		//mapH = buildFoundation(pStructureType, mapX*TILE_UNITS,mapY*TILE_UNITS);
        mapH = buildFoundation(pStructureType, x, y);
		for (width = 0; width < pStructureType->baseWidth; width++)
		{
			for (breadth = 0; breadth < pStructureType->baseBreadth; breadth++)
			{
//psor				mapTile(mapX+width, mapY+breadth)->psObject = (BASE_OBJECT*) psBuilding;
				psTile = mapTile(mapX+width,mapY+breadth);

				// don't really think this should be done here, but dont know otherwise.alexl
				if(pStructureType->type == REF_WALLCORNER || pStructureType->type == REF_WALL)
				{
					if(TILE_HAS_STRUCTURE(mapTile(mapX+width,mapY+breadth)))
					{
						if(getTileStructure(mapX+width,mapY+breadth)->pStructureType->type == REF_WALLCORNER)
						{
							return NULL; // dont build.
						}
					}
				}
				// end of dodgy stuff
				else
				{
					ASSERT((!(TILE_HAS_STRUCTURE(mapTile(mapX+width,mapY+breadth))),
						"buildStructure - structure - %d already found at %d, %d", 
						psBuilding->id, mapX+width,mapY+breadth));
				}

				SET_TILE_STRUCTURE(psTile);
				// if it's a tall structure then flag it in the map.
				if(psBuilding->sDisplay.imd->ymax > TALLOBJECT_YMAX) {

					SET_TILE_TALLSTRUCTURE(psTile);
				}
				if ((pStructureType->type == REF_WALL) ||
					(pStructureType->type == REF_WALLCORNER))
				{
					psTile->tileInfoBits |= BITS_WALL;
				}
				if (pStructureType->height == 1)
				{
					SET_TILE_SMALLSTRUCTURE(psTile);
				}
//				if ((pStructureType->type != REF_WALL) &&
//					(pStructureType->type != REF_WALLCORNER) &&
//					(pStructureType->type != REF_DEFENSE))
//				{
//					setTileHeight(mapX+width, mapY+breadth, mapH);
//				}
			}
		}


		/* DEFENSIVE structures are pulled to the terrain */
		if(pStructureType->type != REF_DEFENSE)
		{
			buildFlatten(pStructureType, mapX<<TILE_SHIFT, mapY<<TILE_SHIFT ,mapH );
			psBuilding->z = (UWORD)mapH;//(UWORD)(map_TileHeight(mapX,mapY)+1);//jps 18july97 - shift it up a wee bit - am
		}
		else
		{
			/* Set it at the higher coord */
			getTileMaxMin(mapX,mapY,&max,&min);
			psBuilding->z = (UWORD)max;	// Got to be - don't change!!!! ALEXM
		}


		//set up the rest of the data
		//psBuilding->z = pStructureType->height;
		psBuilding->turretRotation = 0;
		//psBuilding->turretRotRate = 360;
		psBuilding->turretPitch = 0;
		psBuilding->targetted = 0;
		psBuilding->psTarget = 0;
		// these three will come out and into stats
		//psBuilding->lastEventTime = 0;
		//psBuilding->eventFrame = 0;
		//psBuilding->eventInterval = (3*GAME_TICKS_PER_SEC);
		
		psBuilding->lastEmission = 0;
		//psBuilding->emissionInterval = 100;

		psBuilding->timeLastHit = 0;

		psBuilding->lastHitWeapon = UDWORD_MAX;	// no such weapon


		psBuilding->inFire = FALSE;
		psBuilding->burnStart = 0;
		psBuilding->burnDamage = 0;

		psBuilding->direction = 0;
		psBuilding->pitch = 0;
		psBuilding->roll = 0;
		//psBuilding->damage = structureDamage;
		psBuilding->selected = FALSE;
		//psBuilding->heightScale = (FRACT)0;
		psBuilding->status = SS_BEING_BUILT;
		psBuilding->currentBuildPts = 0;
        psBuilding->currentPowerAccrued = 0;
		//psBuilding->sensorRange = 2 * BASE_VISIBILITY;
		//psBuilding->sensorPower = 100;
		psBuilding->cluster = 0;

		// rotate a wall if necessary
		if (!FromSave && pStructureType->type == REF_WALL && wallType == WALL_VERT)
		{
			psBuilding->direction = 90;
		}

		//set up the sensor stats
		if (psBuilding->pStructureType->pSensor)
		{
			//psBuilding->sensorRange = psBuilding->pStructureType->pSensor->range;
			//psBuilding->sensorPower = psBuilding->pStructureType->pSensor->power;
			psBuilding->sensorRange = (UWORD)sensorRange(psBuilding->pStructureType->
				pSensor, (UBYTE)player);
			psBuilding->sensorPower = (UWORD)sensorPower(psBuilding->pStructureType->
				pSensor, (UBYTE)player);
			//psBuilding->sensorRange = 6 * BASE_VISIBILITY;
			//psBuilding->sensorPower = 200;
		}
		else
		{
			//give them the default sensor for droids if not
			//psBuilding->sensorRange = (asSensorStats + aDefaultSensor[player])->range;
			//psBuilding->sensorPower = (asSensorStats + aDefaultSensor[player])->power;
			psBuilding->sensorRange = (UWORD)sensorRange(asSensorStats + 
				aDefaultSensor[player], (UBYTE)player);
			psBuilding->sensorPower = (UWORD)sensorPower(asSensorStats + 
				aDefaultSensor[player], (UBYTE)player);
		}
		//set up the ecm stat
		if (psBuilding->pStructureType->pECM)
		{
			//psBuilding->ecmPower = psBuilding->pStructureType->pECM->power;
			psBuilding->ecmPower = (UWORD)ecmPower(psBuilding->pStructureType->pECM, 
				psBuilding->player);
		}
		else
		{
			psBuilding->ecmPower = 0;
		}

		/* Store the weapons */
		//psBuilding->numWeaps = (UWORD)pStructureType->numWeaps;
//        psBuilding->asWeaps[0].nStat = 0;
		memset(psBuilding->asWeaps, 0, sizeof(WEAPON));
		//for(weapon=0; weapon < pStructureType->numWeaps; weapon++)
        //can only have the one weapon now - AB 24/01/99
        if (pStructureType->psWeapStat)
		{
            weapon = 0;
			memset(psBuilding->asWeaps + weapon, 0, sizeof(WEAPON));
			psBuilding->asWeaps[weapon].lastFired = 0;
            //in multiPlayer make the Las-Sats require re-loading from the start
            if (bMultiPlayer AND pStructureType->psWeapStat->
                weaponSubClass == WSC_LAS_SAT)
            {
                psBuilding->asWeaps[weapon].lastFired = gameTime;
            }
			//psBuilding->asWeaps[weapon].nStat =	pStructureType->
			//	asWeapList[weapon] - asWeaponStats;
            psBuilding->asWeaps[weapon].nStat =	pStructureType->psWeapStat - asWeaponStats;
			psBuilding->asWeaps[weapon].hitPoints = (asWeaponStats + psBuilding->
				asWeaps[weapon].nStat)->hitPoints;
			psBuilding->asWeaps[weapon].ammo = (asWeaponStats + psBuilding->
				asWeaps[weapon].nStat)->numRounds;
			psBuilding->asWeaps[weapon].recoilValue = 0;
		}

		//psBuilding->baseBodyPoints = psBuilding->body;
		psBuilding->armour = (UWORD)structureArmour(pStructureType, (UBYTE)player);
		psBuilding->resistance = (UWORD)structureResistance(pStructureType, (UBYTE)player);
        psBuilding->lastResistance = ACTION_START_TIME;
		//repair not upgradeable at present!
//		psBuilding->repair = (UWORD)pStructureType->repairSystem;

		//psBuilding->body = pStructureType->bodyPoints;
		//psBuilding->baseBodyPoints = psBuilding->body;
		//check for upgrades - work backwards since want the most recent
		/*for (upgrade = numBodyUpgrades-1; upgrade >= 0; upgrade --)
		{
			if (apBodyUpgrades[psBuilding->player][upgrade].available)
			{
				bodyUpgrade(asFunctions[apBodyUpgrades[psBuilding->player][
					upgrade].functionInc], psBuilding);
				break;
			}
		}*/
		//psBuilding->armour = pStructureType->armourValue;
		//check for upgrades - work backwards since want the most recent
		/*for (upgrade = numArmourUpgrades-1; upgrade >= 0; upgrade --)
		{
			if (apArmourUpgrades[psBuilding->player][upgrade].available)
			{
				armourUpgrade(asFunctions[apArmourUpgrades[psBuilding->player][
					upgrade].functionInc], psBuilding);
				break;
			}
		}*/
		//psBuilding->resistance = pStructureType->resistance;
		//check for upgrades - work backwards since want the most recent
		/*for (upgrade = numResistanceUpgrades-1; upgrade >= 0; upgrade --)
		{
			if (apResistanceUpgrades[psBuilding->player][upgrade].available)
			{
				resistanceUpgrade(asFunctions[apResistanceUpgrades[psBuilding->player][
					upgrade].functionInc], psBuilding);
				break;
			}
		}*/
		//psBuilding->repair = pStructureType->repairSystem;
		//check for upgrades - work backwards since want the most recent
		/*for (upgrade = numRepairUpgrades-1; upgrade >= 0; upgrade --)
		{
			if (apRepairUpgrades[psBuilding->player][upgrade].available)
			{
				repairUpgrade(asFunctions[apRepairUpgrades[psBuilding->player][
					upgrade].functionInc], psBuilding);
				break;
			}
		}*/

//		//set up the imd to use for the display
//		psBuilding->sDisplay.imd = pStructureType->pIMD;
//#ifndef PSX
//		psBuilding->sDisplay.animFrame = 0;
//#endif

        //do the visiblilty stiff before setFunctionality - so placement of DP's can work
//		memset(psBuilding->visible, 0, sizeof(psBuilding->visible));
		/* Not sure about above line - it concerns me, so... */
		memset(psBuilding->visible, 0, (sizeof(UBYTE) * MAX_PLAYERS) );

		/* Structure is trivially visible to the builder (owner) */
		psBuilding->visible[player] = UBYTE_MAX;
		
		if (player == selectedPlayer)
		{ 
			setStructTileDraw(psBuilding);
		}

//		if(FromSave)
//		{
			// If we're coming in from a save game, then make buildings visible
			// if the tile they're on is visible.
//		if(TEST_TILE_VISIBLE(selectedPlayer,mapTile(psBuilding->x>>TILE_SHIFT,psBuilding->y>>TILE_SHIFT)))
//		{
//			psBuilding->visible[selectedPlayer] = UBYTE_MAX;
//		}
 //		}

		// Reveal any tiles that can be seen by the structure
		visTilesUpdate((BASE_OBJECT *)psBuilding,FALSE);

        /*if we're coming from a SAVEGAME and we're on an Expand_Limbo mission, 
        any factories that were built previously for the selectedPlayer will
        have DP's in an invalid location - the scroll limits will have been
        changed to not include them. This is the only HACK I can think of to 
        enable them to be loaded up. So here goes...*/
        if (FromSave AND player == selectedPlayer AND missionLimboExpand())
        {
            //save the current values
            preScrollMinX = scrollMinX;
            preScrollMinY = scrollMinY;
            preScrollMaxX = scrollMaxX;
            preScrollMaxY = scrollMaxY;
            //set the current values to mapWidth/mapHeight
            scrollMinX = 0;
            scrollMinY = 0;
            scrollMaxX = mapWidth;
            scrollMaxY = mapHeight;
        }
		//set the functionality dependant on the type of structure
		if(!setFunctionality(psBuilding, pStructureType->type))
		{
			removeStructFromMap(psBuilding);
			HEAP_FREE(psStructHeap, psBuilding);
            //better reset these if you couldn't build the structure!
            if (FromSave AND player == selectedPlayer AND missionLimboExpand())
            {
                //reset the current values
                scrollMinX = preScrollMinX;
                scrollMinY = preScrollMinY;
                scrollMaxX = preScrollMaxX;
                scrollMaxY = preScrollMaxY;
            }
			return NULL;
		}

        //reset the scroll values if adjusted
        if (FromSave AND player == selectedPlayer AND missionLimboExpand())
        {
            //reset the current values
            scrollMinX = preScrollMinX;
            scrollMinY = preScrollMinY;
            scrollMaxX = preScrollMaxX;
            scrollMaxY = preScrollMaxY;
        }

		psBuilding->body = (UWORD)structureBody(psBuilding);

		//add the structure to the list - this enables it to be drawn whilst being built
		addStructure(psBuilding);

/*
//		memset(psBuilding->visible, 0, sizeof(psBuilding->visible));
		// Not sure about above line - it concerns me, so... 
		memset(psBuilding->visible, 0, (sizeof(UBYTE) * MAX_PLAYERS) );

		// Structure is trivially visible to the builder (owner) 
		psBuilding->visible[player] = UBYTE_MAX;
		
		if (player == selectedPlayer)
		{ 
			setStructTileDraw(psBuilding);
		}

//		if(FromSave)
//		{
			// If we're coming in from a save game, then make buildings visible
			// if the tile they're on is visible.
//		if(TEST_TILE_VISIBLE(selectedPlayer,mapTile(psBuilding->x>>TILE_SHIFT,psBuilding->y>>TILE_SHIFT)))
//		{
//			psBuilding->visible[selectedPlayer] = UBYTE_MAX;
//		}
 //		}

		// Reveal any tiles that can be seen by the structure
		visTilesUpdate((BASE_OBJECT *)psBuilding,FALSE);
*/
		gridAddObject((BASE_OBJECT *)psBuilding);

		clustNewStruct(psBuilding);

		//if resource extractor - need to remove oil feature and prox Msg
		if (pStructureType->type == REF_RESOURCE_EXTRACTOR)
		{
			//find the resource at this point
			for (psFeature = apsFeatureLists[0]; psFeature != NULL; psFeature = psFNext)
			{
				psFNext = psFeature->psNext;
				if (psFeature->psStats->subType == FEAT_OIL_RESOURCE)
				{
					if ((psFeature->x == psBuilding->x) AND (psFeature->y == psBuilding->y))
					{
					//	//see if there is a proximity message FOR THE SELECTED PLAYER at this location
					//	psMessage = findMessage((MSG_VIEWDATA *)psFeature, 
					//		MSG_PROXIMITY, selectedPlayer);
					//	if (psMessage)
					//	{
					//		removeMessage(psMessage, selectedPlayer);
					//	}
					
						//remove it from the map
						turnOffMultiMsg(TRUE);			// dont send this one!
						removeFeature(psFeature);
						turnOffMultiMsg(FALSE);

						//set the map to hold the resource extractor again
						SET_TILE_STRUCTURE(mapTile(psFeature->x >> TILE_SHIFT,
							psFeature->y >> TILE_SHIFT));
						// if it's a tall structure then flag it in the map.
						if(psBuilding->sDisplay.imd->ymax > TALLOBJECT_YMAX) {

							SET_TILE_TALLSTRUCTURE(mapTile(psFeature->x >> TILE_SHIFT,
								psFeature->y >> TILE_SHIFT));
						}
					}
				}
			}
		}

		//return psBuilding;
	}
	else //its an upgrade
	{
		bUpgraded = FALSE;
		//don't create the Structure use existing one
//psor		psBuilding = (STRUCTURE *)mapTile(x >> TILE_SHIFT, y >> TILE_SHIFT)->psObject;
		psBuilding = getTileStructure(x>>TILE_SHIFT, y>>TILE_SHIFT);
		if (psBuilding == NULL)
		{
			//DBERROR(("No owning structure for this module - %s", pStructureType->pName));
			DBERROR(("No owning structure for this module - %s", getStructName(pStructureType)));
			return FALSE;
		}
		if (pStructureType->type == REF_FACTORY_MODULE)
		{
			if (psBuilding->pStructureType->type != REF_FACTORY AND 
				psBuilding->pStructureType->type != REF_VTOL_FACTORY)
			{
				return NULL;
			}
			//increment the capacity and output for the owning structure
			if (((FACTORY*)psBuilding->pFunctionality)->capacity < SIZE_SUPER_HEAVY)
			{
                //this happends gradually now - AB 5/1/99
				/*subtract the power required to build BEFORE do anything*/
				/*if (!usePower(player, pStructureType->powerToBuild))
				{
					return NULL;
				}*/
				//store the % difference in body points before upgrading
				bodyDiff = PERCENT(psBuilding->body, structureBody(psBuilding));

				((FACTORY*)psBuilding->pFunctionality)->capacity++;
				bUpgraded = TRUE;
                //put any production on hold
                holdProduction(psBuilding);
		
				//quick check not trying to add too much
				ASSERT((((FACTORY*)psBuilding->pFunctionality)->productionOutput +
					((PRODUCTION_FUNCTION*)pStructureType->asFuncList[0])->
					productionOutput < UBYTE_MAX, 
					"building factory module - productionOutput too big"));

				((FACTORY*)psBuilding->pFunctionality)->productionOutput += ((
					PRODUCTION_FUNCTION*)pStructureType->asFuncList[0])->
					productionOutput;
				//need to change which IMD is used for player 0
				//Need to do a check its not Barbarian really!

				if (   (bMultiPlayer && isHumanPlayer(psBuilding->player))
					OR (bMultiPlayer && (game.type == SKIRMISH) && (psBuilding->player < game.maxPlayers))
					OR !bMultiPlayer)
					//|| (psBuilding->player ==0) )
//#else
//				if (psBuilding->player == 0)

				{
					capacity = ((FACTORY*)psBuilding->pFunctionality)->capacity;
					if (capacity < NUM_FACTORY_MODULES)
					{
						if (psBuilding->pStructureType->type == REF_FACTORY)
						{
							psBuilding->sDisplay.imd = factoryModuleIMDs[
								capacity-1][0];
						}
						else
						{
							psBuilding->sDisplay.imd = factoryModuleIMDs[
								capacity-1][1];
						}
					}
					else
					{
						if (psBuilding->pStructureType->type == REF_FACTORY)
						{
							psBuilding->sDisplay.imd = factoryModuleIMDs[
								NUM_FACTORY_MODULES-1][0];
						}
						else
						{
							psBuilding->sDisplay.imd = factoryModuleIMDs[
								NUM_FACTORY_MODULES-1][1];
						}
					}
				}
			}
		}

		if (pStructureType->type == REF_RESEARCH_MODULE)
		{
			if (psBuilding->pStructureType->type != REF_RESEARCH)
			{
				return NULL;
			}
			//increment the capacity and research points for the owning structure
			if (((RESEARCH_FACILITY*)psBuilding->pFunctionality)->capacity < NUM_RESEARCH_MODULES)
			{
                //this happends gradually now - AB 5/1/99
				/*subtract the power required to build BEFORE do anything*/
				/*if (!usePower(player, pStructureType->powerToBuild))
				{
					return NULL;
				}*/

				//store the % difference in body points before upgrading
				bodyDiff = PERCENT(psBuilding->body, structureBody(psBuilding));

				//add all the research modules in one go AB 24/06/98
				//((RESEARCH_FACILITY*)psBuilding->pFunctionality)->capacity++;
				((RESEARCH_FACILITY*)psBuilding->pFunctionality)->capacity = NUM_RESEARCH_MODULES;
				((RESEARCH_FACILITY*)psBuilding->pFunctionality)->researchPoints += ((
					RESEARCH_FUNCTION*)pStructureType->asFuncList[0])->
					researchPoints;
				bUpgraded = TRUE;
                //cancel any research - put on hold now
                if (((RESEARCH_FACILITY *)psBuilding->pFunctionality)->psSubject)
                {
                    //cancel the topic
                    //cancelResearch(psBuilding, ((RESEARCH *)((RESEARCH_FACILITY *)psBuilding->
                      //  pFunctionality)->psSubject) - asResearch, psBuilding->player);
                    holdResearch(psBuilding);
                }
				
				//need to change which IMD is used for player 0
				//Need to do a check its not Barbarian really!

				if (   (bMultiPlayer && isHumanPlayer(psBuilding->player))
					OR (bMultiPlayer && (game.type == SKIRMISH) && (psBuilding->player < game.maxPlayers))
					OR !bMultiPlayer)

				{
					capacity = ((RESEARCH_FACILITY*)psBuilding->pFunctionality)->capacity;
					if (capacity < NUM_RESEARCH_MODULES)
					{
						psBuilding->sDisplay.imd = researchModuleIMDs[capacity-1];
					}
					else
					{
						psBuilding->sDisplay.imd = researchModuleIMDs[NUM_RESEARCH_MODULES-1];
					}
				}
			}
		}

		if (pStructureType->type == REF_POWER_MODULE)
		{
			if (psBuilding->pStructureType->type != REF_POWER_GEN)
			{
				return NULL;
			}
			//increment the capacity and research points for the owning structure
			if (((POWER_GEN*)psBuilding->pFunctionality)->capacity < NUM_POWER_MODULES)
			{
                //this happends gradually now - AB 5/1/99
				/*subtract the power required to build BEFORE do anything*/
				/*if (!usePower(player, pStructureType->powerToBuild))
				{
					return NULL;
				}*/
				//store the % difference in body points before upgrading
				bodyDiff = PERCENT(psBuilding->body, structureBody(psBuilding));
 
				//increment the power output, multiplier and capacity
				//add all the research modules in one go AB 24/06/98
				//((POWER_GEN*)psBuilding->pFunctionality)->capacity++;
				((POWER_GEN*)psBuilding->pFunctionality)->capacity = NUM_POWER_MODULES;
				((POWER_GEN*)psBuilding->pFunctionality)->power += ((
					POWER_GEN_FUNCTION*)pStructureType->asFuncList[0])->
					powerOutput;
				((POWER_GEN*)psBuilding->pFunctionality)->multiplier += ((
					POWER_GEN_FUNCTION*)pStructureType->asFuncList[0])->
					powerMultiplier;
				bUpgraded = TRUE;

				//need to change which IMD is used for player 0
				//Need to do a check its not Barbarian really!

				if (   (bMultiPlayer && isHumanPlayer(psBuilding->player))
					OR (bMultiPlayer && (game.type == SKIRMISH) && (psBuilding->player < game.maxPlayers))
					OR !bMultiPlayer)
//#else
//				if (psBuilding->player == 0)

				{
					capacity = ((POWER_GEN*)psBuilding->pFunctionality)->capacity;
					if (capacity < NUM_POWER_MODULES)
					{
						psBuilding->sDisplay.imd = powerModuleIMDs[capacity-1];
					}
					else
					{
						psBuilding->sDisplay.imd = powerModuleIMDs[NUM_POWER_MODULES-1];
					}
				}
				//need to inform any res Extr associated that not digging until complete
				releasePowerGen(psBuilding);
			}
		}
		if (bUpgraded)
		{
			//calculate the new body points of the owning structure
			//psBuilding->body = (UWORD)(psBuilding->body  + pStructureType->bodyPoints);
			psBuilding->body = (UWORD)(structureBody(psBuilding) * bodyDiff / 100);

			//and the base body points
			//psBuilding->baseBodyPoints += pStructureType->bodyPoints;
			//initialise the build points
			psBuilding->currentBuildPts = 0;
            psBuilding->currentPowerAccrued = 0;
			//start building again
			psBuilding->status = SS_BEING_BUILT;
			if (psBuilding->player == selectedPlayer)
			{
				intRefreshScreen();
			}
            //inform power system that won't be needing power until built
            powerDestroyObject((BASE_OBJECT *)psBuilding);
		}
		//return psBuilding;
	}
	if(pStructureType->type!=REF_WALL AND pStructureType->type!=REF_WALLCORNER)
	{
		if(player == selectedPlayer)
		{
			scoreUpdateVar(WD_STR_BUILT);
		}
	}
	
	/* why is this necessary - it makes tiles under the structure visible */
	setUnderTilesVis((BASE_OBJECT*)psBuilding,player);
	return psBuilding;
}

/*
void createAssemblyPoint(STRUCTURE* psStruct)
{
	FACTORY* psFactory;
	UDWORD	x, y;

		psFactory = (FACTORY *)psStruct->pFunctionality;
		addFlagPosition(psFactory->psAssemblyPoint);
		switch(psStruct->pStructureType->type)
		{
		case REF_FACTORY:
			setFlagPositionInc(psFactory, psStruct->player, FACTORY_FLAG);
			break;
		case REF_CYBORG_FACTORY:
			setFlagPositionInc(psFactory, psStruct->player, CYBORG_FLAG);
			break;
		case REF_VTOL_FACTORY:
			setFlagPositionInc(psFactory, psStruct->player, VTOL_FLAG);
			break;
		default:
			ASSERT((FALSE, "setFunctionality: Invalid factory type"));
		}
		//initialise the assembly point position
		x = psStruct->x+256 >> TILE_SHIFT;
		y = psStruct->y+256 >> TILE_SHIFT;
		// Belt and braces - shouldn't be able to build too near edge
		getNearestBestValidTile(&x,&y);
		setAssemblyPoint( psFactory->psAssemblyPoint, x << TILE_SHIFT, 
			y << TILE_SHIFT );

}
*/

BOOL setFunctionality(STRUCTURE	*psBuilding, UDWORD functionType)
{
	//SDWORD					upgrade;
	FACTORY					*psFactory;
	RESEARCH_FACILITY		*psResFac;
	POWER_GEN				*psPowerGen;
	REPAIR_FACILITY			*psRepairFac;
	REPAIR_DROID_FUNCTION	*pFuncRepair;
	REARM_PAD				*psReArmPad;
	UDWORD					x, y;
	
	psBuilding->pFunctionality = NULL;
	switch(functionType)
	{
		case REF_FACTORY:
		case REF_CYBORG_FACTORY:
		case REF_VTOL_FACTORY:
		{
			//this structure must have a function assigned to the stat
			if (psBuilding->pStructureType->numFuncs == 0)
			{
#ifdef HASH_NAMES
				DBERROR(("There must be a function assigned to this building - %x",	psBuilding));
#else
				DBERROR(("There must be a function assigned to this building - %s",	getName(psBuilding->pStructureType->pName)));
#endif
				return FALSE;
			}
			//allocate the necessary space
			/*psBuilding->pFunctionality = (FUNCTIONALITY *) MALLOC(sizeof(FACTORY));
			if (psBuilding->pFunctionality == NULL)
			{
				DBERROR(("Out of memory"));
				return FALSE;
			}*/
			//allocate the necessary space
			if (!createStructFunc(&psBuilding->pFunctionality))
			{
				DBERROR((FALSE, "Out of memory"));
				return FALSE;
			}

			//initialise the memory
			memset(psBuilding->pFunctionality, 0, sizeof(FUNCTIONALITY));

			psFactory = (FACTORY *)psBuilding->pFunctionality;
			psFactory->capacity = (UBYTE)((PRODUCTION_FUNCTION*)psBuilding->
				pStructureType->asFuncList[0])->capacity;
			//psFactory->propulsionType = ((PRODUCTION_FUNCTION*)psBuilding->
			//	pStructureType->asFuncList[0])->propulsionType;
			psFactory->productionOutput = (UBYTE)((PRODUCTION_FUNCTION*)psBuilding->
				pStructureType->asFuncList[0])->productionOutput;
			psFactory->psSubject = NULL;
            //default the secondary order - AB 22/04/99
            psFactory->secondaryOrder = DSS_ARANGE_DEFAULT | DSS_REPLEV_NEVER | 
                DSS_ALEV_ALWAYS | DSS_HALT_GUARD;
			
			//add the assembly point to the factory
			if (!createFlagPosition(&psFactory->psAssemblyPoint, psBuilding->player))
			{
				return FALSE;
			}
//			psFactory->psAssemblyPoint->primary= TRUE;		// set prim. point.

			addFlagPosition(psFactory->psAssemblyPoint);
			switch(functionType)
			{
			case REF_FACTORY:
				setFlagPositionInc(psFactory, psBuilding->player, FACTORY_FLAG);
				break;
			case REF_CYBORG_FACTORY:
				setFlagPositionInc(psFactory, psBuilding->player, CYBORG_FLAG);
				break;
			case REF_VTOL_FACTORY:
				setFlagPositionInc(psFactory, psBuilding->player, VTOL_FLAG);
				break;
			default:
				ASSERT((FALSE, "setFunctionality: Invalid factory type"));
			}
			//initialise the assembly point position
			x = (psBuilding->x+256) >> TILE_SHIFT;
			y = (psBuilding->y+256) >> TILE_SHIFT;
			// Belt and braces - shouldn't be able to build too near edge
			//getNearestBestValidTile(&x,&y);
			setAssemblyPoint( psFactory->psAssemblyPoint, x << TILE_SHIFT, 
				y << TILE_SHIFT, psBuilding->player, TRUE);

			psFactory->psFormation = NULL;

            //this isn't used anymore-  AB 11/02/99
			//Set the factory group
			/*if (!grpCreate(&psFactory->psGroup))
			{
				DBPRINTF(("setFunctionality: unable to create group\n"));
			}
			else
			{
				//add the factory as a null droid
				grpJoin(psFactory->psGroup, NULL);
			}*/
			//check for upgrades - work backwards since want the most recent
			/*for (upgrade = numProductionUpgrades-1; upgrade >= 0; upgrade --)
			{
				if (apProductionUpgrades[psBuilding->player][upgrade].available)
				{
					productionUpgrade(asFunctions[apProductionUpgrades[
						psBuilding->player][upgrade].functionInc], psBuilding);
					break;
				}
			}*/
			structureProductionUpgrade(psBuilding);
			break;
		}
		/*SET IN BUILD STRUCTURE case REF_FACTORY_MODULE:
		{
			// Find the owner structure if any 
//psor			psOwner = (STRUCTURE *)mapTile(psBuilding->x >> TILE_SHIFT, psBuilding->
//psor				y >> TILE_SHIFT)->psObject;
			psOwner = getTileStructure(psBuilding->x>>TILE_SHIFT,psBuilding->y>>TILE_SHIFT);
			// Check we've got the right owner type 
			if (psOwner && psOwner->type != OBJ_STRUCTURE && psOwner->
				pStructureType->type != REF_FACTORY)
			{
				return FALSE;
			}

			//increment the capacity and output for the owning structure
			if (((FACTORY*)psOwner->pFunctionality)->capacity < SIZE_SUPER_HEAVY)
			{
				((FACTORY*)psOwner->pFunctionality)->capacity++;
			}
			((FACTORY*)psOwner->pFunctionality)->productionOutput += ((PRODUCTION_FUNCTION*)
				psBuilding->pStructureType->asFuncList[0])->productionOutput;
			break;
		}*/
		case REF_RESEARCH:
		{
			//this structure must have a function assigned to the stat
			if (psBuilding->pStructureType->numFuncs == 0)
			{
#ifdef HASH_NAMES
				DBERROR(("There must be a function assigned to this building - %s",	strresGetString(NULL,psBuilding->pStructureType->NameHash) ));
#else
				DBERROR(("There must be a function assigned to this building - %s",	getName(psBuilding->pStructureType->pName)));
#endif
				return FALSE;
			}
			//allocate the necessary space
			/*psBuilding->pFunctionality = (FUNCTIONALITY *) MALLOC(sizeof(RESEARCH_FACILITY));
			if (psBuilding->pFunctionality == NULL)
			{
				DBERROR(("Out of memory"));
				return FALSE;
			}*/
			//try and create the Structure
			if (!createStructFunc(&psBuilding->pFunctionality))
			{
				DBERROR((FALSE, "Out of memory"));
				return FALSE;
			}
			//initialise the memory
			memset(psBuilding->pFunctionality, 0, sizeof(FUNCTIONALITY));

			psResFac = (RESEARCH_FACILITY *)psBuilding->pFunctionality;
			psResFac->researchPoints = ((RESEARCH_FUNCTION*)psBuilding->
				pStructureType->asFuncList[0])->researchPoints;
			//check for upgrades - work backwards since want the most recent
			/*for (upgrade = numResearchUpgrades-1; upgrade >= 0; upgrade --)
			{
				if (apResearchUpgrades[psBuilding->player][upgrade].available)
				{
					researchUpgrade(asFunctions[apResearchUpgrades[
						psBuilding->player][upgrade].functionInc], psBuilding);
					break;
				}
			}*/
			structureResearchUpgrade(psBuilding);
			break;
		}
		/*SET IN BUILD STRUCTURE case REF_RESEARCH_MODULE:
		{
			// Find the owner structure if any 
//psor			psOwner = (STRUCTURE *)mapTile(psBuilding->x >> TILE_SHIFT, psBuilding->
//psor				y >> TILE_SHIFT)->psObject;
			psOwner = getTileStructure(psBuilding->x>>TILE_SHIFT,psBuilding->y>>TILE_SHIFT);
			// Check we've got the right owner type 
			if (psOwner && psOwner->type != OBJ_STRUCTURE && psOwner->
				pStructureType->type != REF_RESEARCH)
			{
				return FALSE;
			}

			//increment the research points for the owning structure
			if (((RESEARCH_FACILITY*)psOwner->pFunctionality)->capacity < 4)
			{
				((RESEARCH_FACILITY*)psOwner->pFunctionality)->capacity++;
			}
			((RESEARCH_FACILITY*)psOwner->pFunctionality)->researchPoints += ((
				RESEARCH_UPGRADE_FUNCTION*)psBuilding->pStructureType->asFuncList[0])->
				researchPoints;
			break;
		}*/
		case REF_POWER_GEN:
		{
			//this structure must have a function assigned to the stat
			if (psBuilding->pStructureType->numFuncs == 0)
			{
#ifdef HASH_NAMES
				DBERROR(("There must be a function assigned to this building - %s",	strresGetString(NULL,psBuilding->pStructureType->NameHash) ));
#else
				DBERROR(("There must be a function assigned to this building - %s",	getName(psBuilding->pStructureType->pName)));
#endif
				return FALSE;
			}
			//allocate the necessary space
			/*psBuilding->pFunctionality = (FUNCTIONALITY *) MALLOC(sizeof(POWER_GEN));
			if (psBuilding->pFunctionality == NULL)
			{
				DBERROR(("Out of memory"));
				return FALSE;
			}*/
			//try and create the Structure
			if (!createStructFunc(&psBuilding->pFunctionality))
			{
				DBERROR((FALSE, "Out of memory"));
				return FALSE;
			}

			//initialise the memory
			memset(psBuilding->pFunctionality, 0, sizeof(FUNCTIONALITY));

			psPowerGen = (POWER_GEN *)psBuilding->pFunctionality;
			psPowerGen->power = ((POWER_GEN_FUNCTION*)psBuilding->
				pStructureType->asFuncList[0])->powerOutput;
			psPowerGen->multiplier = ((POWER_GEN_FUNCTION*)psBuilding->
				pStructureType->asFuncList[0])->powerMultiplier;
			psPowerGen->capacity = 0;
			//check for upgrades 
			structurePowerUpgrade(psBuilding);
			break;
		}
		case REF_RESOURCE_EXTRACTOR:
		{
			//this structure must have a function assigned to the stat
			if (psBuilding->pStructureType->numFuncs == 0)
			{
#ifdef HASH_NAMES
				DBERROR(("There must be a function assigned to this building - %s",	strresGetString(NULL,psBuilding->pStructureType->NameHash) ));
#else
				DBERROR(("There must be a function assigned to this building - %s",	getName(psBuilding->pStructureType->pName)));
#endif
				return FALSE;
			}
			//allocate the necessary space
			/*psBuilding->pFunctionality = (FUNCTIONALITY *) MALLOC(sizeof(RES_EXTRACTOR));
			if (psBuilding->pFunctionality == NULL)
			{
				DBERROR(("Out of memory"));
				return FALSE;
			}*/
			//try and create the Structure
			if (!createStructFunc(&psBuilding->pFunctionality))
			{
				DBERROR((FALSE, "Out of memory"));
				return FALSE;
			}
			//initialise the memory
			memset(psBuilding->pFunctionality, 0, sizeof(FUNCTIONALITY));

			((RES_EXTRACTOR*)psBuilding->pFunctionality)->power = ((
				//POWER_REG_FUNCTION*)psBuilding->pStructureType->asFuncList[0])->
				RESOURCE_FUNCTION*)psBuilding->pStructureType->asFuncList[0])->
				maxPower;
			//set the structure going
			//((RES_EXTRACTOR*)psBuilding->pFunctionality)->timeLastUpdated = gameTime;
			//set the structure to inactive
			((RES_EXTRACTOR*)psBuilding->pFunctionality)->active = FALSE;
			((RES_EXTRACTOR*)psBuilding->pFunctionality)->psPowerGen = NULL;
			break;
		}
		//this just checks that a function has been assigned
		case REF_HQ:
		{
			radarOnScreen = TRUE;
			//this structure must have a function assigned to the stat
			if (psBuilding->pStructureType->numFuncs == 0)
			{
#ifdef HASH_NAMES
				DBERROR(("There must be a function assigned to this building - %s",	strresGetString(NULL,psBuilding->pStructureType->NameHash) ));
#else
				DBERROR(("There must be a function assigned to this building - %s",	getName(psBuilding->pStructureType->pName)));
#endif
				return FALSE;
			}
			//this function is called once the structure has been built
		//	extractedPowerUpdate(psBuilding);
			break;
		}
		case REF_WALL:
		{
			//this structure must have a function assigned to the stat - this is just a check!
			if (psBuilding->pStructureType->numFuncs == 0)
			{
#ifdef HASH_NAMES
				DBERROR(("There must be a function assigned to this building - %s",	strresGetString(NULL,psBuilding->pStructureType->NameHash) ));
#else
				DBERROR(("There must be a function assigned to this building - %s",	getName(psBuilding->pStructureType->pName)));
#endif
				return FALSE;
			}
			break;
		}
		case REF_REPAIR_FACILITY:
		{
			//this structure must have a function assigned to the stat
			if (psBuilding->pStructureType->numFuncs == 0)
			{
#ifdef HASH_NAMES
				DBERROR(("There must be a function assigned to this building - %s",	strresGetString(NULL,psBuilding->pStructureType->NameHash) ));
#else
				DBERROR(("There must be a function assigned to this building - %s",	getName(psBuilding->pStructureType->pName)));
#endif
				return FALSE;
			}

			//allocate the necessary space
			/*psBuilding->pFunctionality = (FUNCTIONALITY *) MALLOC(sizeof(REPAIR_FACILITY));
			if (psBuilding->pFunctionality == NULL)
			{
				DBERROR(("Out of memory"));
				return FALSE;
			}*/
			//try and create the Structure
			if (!createStructFunc(&psBuilding->pFunctionality))
			{
				DBERROR((FALSE, "Out of memory"));
				return FALSE;
			}
			//initialise the memory
			memset(psBuilding->pFunctionality, 0, sizeof(FUNCTIONALITY));

			psRepairFac = (REPAIR_FACILITY *) psBuilding->pFunctionality;
			pFuncRepair = (REPAIR_DROID_FUNCTION *) psBuilding->pStructureType->asFuncList[0];
			psRepairFac->power = pFuncRepair->repairPoints;
			psRepairFac->psObj = NULL;

			if ( !grpCreate(&((REPAIR_FACILITY *) psBuilding->pFunctionality)->psGroup) )
			{
				DBPRINTF( ("setFunctionality: couldn't create repair facility group") );
			}
			else
			{
				/* add null droid */
				grpJoin( psRepairFac->psGroup, NULL );
			}
			//check for upgrades 
			structureRepairUpgrade(psBuilding);

			// add an assembly point
			if (!createFlagPosition(&psRepairFac->psDeliveryPoint, psBuilding->player))
			{
				return FALSE;
			}

			addFlagPosition(psRepairFac->psDeliveryPoint);
			setFlagPositionInc(psRepairFac, psBuilding->player, REPAIR_FLAG);
			//initialise the assembly point position
			x = (psBuilding->x+256) >> TILE_SHIFT;
			y = (psBuilding->y+256) >> TILE_SHIFT;
			// Belt and braces - shouldn't be able to build too near edge
			//getNearestBestValidTile(&x,&y);
			setAssemblyPoint( psRepairFac->psDeliveryPoint, x << TILE_SHIFT, 
				y << TILE_SHIFT, psBuilding->player, TRUE);
			break;
		}
		case REF_REARM_PAD:
		{
			//this structure must have a function assigned to the stat
			if (psBuilding->pStructureType->numFuncs == 0)
			{
#ifdef HASH_NAMES
				DBERROR(("There must be a function assigned to this building - %s",	
					strresGetString(NULL,psBuilding->pStructureType->NameHash) ));
#else
				DBERROR(("There must be a function assigned to this building - %s",	
					getName(psBuilding->pStructureType->pName)));
#endif
				return FALSE;
			}
			//try and create the Structure
			if (!createStructFunc(&psBuilding->pFunctionality))
			{
				DBERROR((FALSE, "Out of memory"));
				return FALSE;
			}
			//initialise the memory
			memset(psBuilding->pFunctionality, 0, sizeof(FUNCTIONALITY));

			psReArmPad = (REARM_PAD *)psBuilding->pFunctionality;
			psReArmPad->reArmPoints = ((REARM_PAD *)psBuilding->
				pStructureType->asFuncList[0])->reArmPoints;
			//check for upgrades 
			structureReArmUpgrade(psBuilding);
			break;
		}
	}//end of switch
	return TRUE;
}

/*
//this is surplus to requirements now!
// Using pickATile - find the nearest best tile to the one we've specified.
void	getNearestBestValidTile(UDWORD *x, UDWORD *y)
{
BOOL	gotPos;
UBYTE	iterations;

	// PickAtile - will succeed even for 0 iterations if tile is valid 
	iterations = 0;
	gotPos = 0;
	// Keep going until we get it 
	while(!gotPos)
	{
		// Have another go...
		gotPos = pickATileGen(x,y,iterations++,normalPAT);
	}
}
*/

// Set the command droid that factory production should go to
void assignFactoryCommandDroid(STRUCTURE *psStruct, DROID *psCommander)
{
	FACTORY			*psFact;
	FLAG_POSITION	*psFlag, *psNext, *psPrev;
	SDWORD			factoryInc,typeFlag;

	ASSERT((StructIsFactory(psStruct),"assignFactoryCommandUnit: structure not a factory"));

	psFact = (FACTORY *)psStruct->pFunctionality;

	switch(psStruct->pStructureType->type)
	{
	case REF_FACTORY:
		typeFlag = FACTORY_FLAG;
		break;
	case REF_VTOL_FACTORY:
		typeFlag = VTOL_FLAG;
		break;
	case REF_CYBORG_FACTORY:
		typeFlag = CYBORG_FLAG;
		break;
	default:
		ASSERT((FALSE,"assignfactorycommandUnit: unknown factory type"));
		typeFlag = FACTORY_FLAG;
		break;
	}

	// removing a commander from a factory
	if ( psFact->psCommander != NULL )
	{
		if (typeFlag == FACTORY_FLAG)
		{
			secondarySetState(psFact->psCommander, DSO_CLEAR_PRODUCTION,
					1 << ( psFact->psAssemblyPoint->factoryInc + DSS_ASSPROD_SHIFT) );
		}
		else if (typeFlag == CYBORG_FLAG)
		{
			secondarySetState(psFact->psCommander, DSO_CLEAR_PRODUCTION,
					1 << ( psFact->psAssemblyPoint->factoryInc + DSS_ASSPROD_CYBORG_SHIFT) );
		}
		else
		{
			secondarySetState(psFact->psCommander, DSO_CLEAR_PRODUCTION,
					1 << ( psFact->psAssemblyPoint->factoryInc + DSS_ASSPROD_VTOL_SHIFT) );
		}

		psFact->psCommander = NULL;
		if (!missionIsOffworld())
		{
			addFlagPosition(psFact->psAssemblyPoint);	// add the assembly point back into the list
		}
		else
		{
			psFact->psAssemblyPoint->psNext = mission.apsFlagPosLists[psFact->psAssemblyPoint->player];
			mission.apsFlagPosLists[psFact->psAssemblyPoint->player] = psFact->psAssemblyPoint;
		}
	}

	if ( psCommander != NULL )									
	{
	// adding a commander to a factory
/*		if ( psFact->psCommander != NULL )			// have to remove the factory from any previous commander
		{
			if (typeFlag == FACTORY_FLAG)
			{
				secondarySetState(psFact->psCommander, DSO_CLEAR_PRODUCTION,
						1 << ( psFact->psAssemblyPoint->factoryInc + DSS_ASSPROD_SHIFT) );
			}
			else if (typeFlag == CYBORG_FLAG)
			{
				secondarySetState(psFact->psCommander, DSO_CLEAR_PRODUCTION,
						1 << ( psFact->psAssemblyPoint->factoryInc + DSS_ASSPROD_CYBORG_SHIFT) );
			}
			else
			{
				secondarySetState(psFact->psCommander, DSO_CLEAR_PRODUCTION,
						1 << ( psFact->psAssemblyPoint->factoryInc + DSS_ASSPROD_VTOL_SHIFT) );
			}
		
			psFact->psCommander = NULL;				// remove commander from a factory		
			addFlagPosition(psFact->psAssemblyPoint);// add the assembly point back into the list (only temporaryily.)
		}*/

		ASSERT((!missionIsOffworld(),
			"assignFactoryCommandDroid: cannot assign a commander to a factory when off world"));

		
		factoryInc = psFact->psAssemblyPoint->factoryInc;
		psPrev = NULL;
	
		for (psFlag = apsFlagPosLists[psStruct->player]; psFlag; psFlag = psNext)
		{
			psNext = psFlag->psNext;

			if ( (psFlag->factoryInc == factoryInc) && (psFlag->factoryType == typeFlag)   )
			{
				if ( psFlag != psFact->psAssemblyPoint )
				{
					removeFlagPosition(psFlag);
				}
				else
				{
					// need to keep the assembly point(s) for the factory
					// but remove it(the primary) from the list so it doesn't get
					// displayed
					if ( psPrev == NULL )
					{
						apsFlagPosLists[psStruct->player] = psFlag->psNext;
					}
					else
					{
						psPrev->psNext = psFlag->psNext;
					}
					psFlag->psNext = NULL;
				}
			}
			else
			{
				psPrev = psFlag;
			}
		}
//		psFact->psAssemblyPoint->factorySub = 0;			// reset factory count of subpoints.

		psFact->psCommander = psCommander;
	}
}


// remove all factories from a command droid
void clearCommandDroidFactory(DROID *psDroid)
{
	STRUCTURE	*psCurr;

	for(psCurr = apsStructLists[selectedPlayer]; psCurr; psCurr=psCurr->psNext)
	{
		if ( (psCurr->pStructureType->type == REF_FACTORY) ||
			 (psCurr->pStructureType->type == REF_CYBORG_FACTORY) ||
			 (psCurr->pStructureType->type == REF_VTOL_FACTORY) )
		{
			if (((FACTORY*)psCurr->pFunctionality)->psCommander == psDroid)
			{
				assignFactoryCommandDroid(psCurr, NULL);
			}
		}
	}
	for(psCurr = mission.apsStructLists[selectedPlayer]; psCurr; psCurr=psCurr->psNext)
	{
		if ( (psCurr->pStructureType->type == REF_FACTORY) ||
			 (psCurr->pStructureType->type == REF_CYBORG_FACTORY) ||
			 (psCurr->pStructureType->type == REF_VTOL_FACTORY) )
		{
			if (((FACTORY*)psCurr->pFunctionality)->psCommander == psDroid)
			{
				assignFactoryCommandDroid(psCurr, NULL);
			}
		}
	}
}

/* Check that a tile is vacant for a droid to be placed */
static BOOL structClearTile(UWORD x, UWORD y)
{
	UDWORD	player;
	DROID	*psCurr;

	/* Check for a structure */
//psor	if (mapTile(x,y)->psObject != NULL)
	if(fpathBlockingTile(x,y))
	{
		DBP2(("structClearTile: failed\n"));
		return FALSE;
	}

	/* Check for a droid */
	for(player=0; player< MAX_PLAYERS; player++)
	{
		for(psCurr = apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
		{
			if (psCurr->x >> TILE_SHIFT == x &&
				psCurr->y >> TILE_SHIFT == y)
			{
				DBP2(("structClearTile: failed\n"));
				return FALSE;
			}
		}
	}

	DBP2(("structClearTile: succeeded\n"));
	return TRUE;
}

/*find a location near to a structure to start the droid of*/
BOOL placeDroid(STRUCTURE *psStructure, UDWORD *droidX, UDWORD *droidY)
{
	SWORD			sx,sy, xmin,xmax, ymin,ymax, x,y, xmid;
	BOOL			placed;

	/* Get the tile coords for the top left of the structure */
	sx = (SWORD)(psStructure->x - psStructure->pStructureType->baseWidth * TILE_UNITS/2);
	sx = (SWORD)(sx >> TILE_SHIFT);
	sy = (SWORD)(psStructure->y - psStructure->pStructureType->baseBreadth * TILE_UNITS/2);
	sy = (SWORD)(sy >> TILE_SHIFT);

	/* Find the four corners of the square */
	xmin = (SWORD)(sx - 1);
	xmax = (SWORD)(sx + psStructure->pStructureType->baseWidth);
	xmid = (SWORD)(sx + (psStructure->pStructureType->baseWidth-1)/2);
	ymin = (SWORD)(sy - 1);
	ymax = (SWORD)(sy + psStructure->pStructureType->baseBreadth);
	if (xmin < 0)
	{
		xmin = 0;
	}
	if (xmax > (SDWORD)mapWidth)
	{
		xmax = (SWORD)mapWidth;
	}
	if (ymin < 0)
	{
		ymin = 0;
	}
	if (ymax > (SDWORD)mapHeight)
	{
		ymax = (SWORD)mapHeight;
	}

	/* Look for a clear location for the droid across the bottom */
	/* start in the middle */
	placed = FALSE;
	y = ymax;
	/* middle to right */
	for(x = xmid; x < xmax; x++)
	{
		if (structClearTile(x, y))
		{
			placed = TRUE;
			break;
		}
	}
	/* left to middle */
	if (!placed)
	{
		for(x = xmin; x < xmid; x++)
		{
			if (structClearTile(x, y))
			{
				placed = TRUE;
				break;
			}
		}
	}
	/* across the top */
	if (!placed)
	{
		y = ymin;
		for(x = xmin; x < xmax; x++)
		{
			if (structClearTile(x, y))
			{
				placed = TRUE;
				break;
			}
		}
	}
	/* the left */
	if (!placed)
	{
		x = xmin;
		for(y = ymin; y < ymax; y++)
		{
			if (structClearTile(x, y))
			{
				placed = TRUE;
				break;
			}
		}
	}
	/* the right */
	if (!placed)
	{
		x = xmax;
		for(y = ymin; y < ymax; y++)
		{
			if (structClearTile(x, y))
			{
				placed = TRUE;
				break;
			}
		}
	}
	*droidX = x;
	*droidY = y;
	return placed;
}

/* Place a newly manufactured droid next to a factory  and then send if off 
to the assembly point*/
static void structPlaceDroid(STRUCTURE *psStructure, DROID_TEMPLATE *psTempl,
							 DROID **ppsDroid)
{
	UDWORD			x,y;
	BOOL			placed;//bTemp = FALSE;
	DROID			*psNewDroid;
	FACTORY			*psFact;
	SDWORD			apx,apy;
	FLAG_POSITION	*psFlag;
//	UDWORD			i;
	iVector			iVecEffect;
	UBYTE			factoryType;
	BOOL			assignCommander;
    //STRUCTURE       *psReArmPad;

	//	DBPRINTF(("Called structPlaceDroid.\n");

	/* set blocking tile before placing droid */
//	fpathSetBlockingTile( asPropulsionStats[psTempl->asParts[COMP_PROPULSION]].propulsionType );

	placed = placeDroid(psStructure, &x, &y);

	if (placed)
	{
		//create a droid near to the structure
		psNewDroid = buildDroid(psTempl, x << TILE_SHIFT, y << TILE_SHIFT, 
			psStructure->player, FALSE);
		if (!psNewDroid)
		{
			*ppsDroid = NULL;
			return;
		}

        //set the droids order to that of the factory - AB 22/04/99
        psNewDroid->secondaryOrder = ((FACTORY *)psStructure->pFunctionality)->secondaryOrder;

    	if(bMultiPlayer)		
	    {
            sendDroidSecondaryAll(psNewDroid);
        }

		if(psStructure->visible[selectedPlayer])
		{
			/* add smoke effect to cover the droid's emergence from the factory */
			iVecEffect.x = psNewDroid->x;
			iVecEffect.y = map_Height( psNewDroid->x, psNewDroid->y ) + DROID_CONSTRUCTION_SMOKE_HEIGHT;
			iVecEffect.z = psNewDroid->y;
			addEffect( &iVecEffect,EFFECT_CONSTRUCTION,CONSTRUCTION_TYPE_DRIFTING,FALSE,NULL,0 );
			iVecEffect.x = psNewDroid->x - DROID_CONSTRUCTION_SMOKE_OFFSET;
			iVecEffect.z = psNewDroid->y - DROID_CONSTRUCTION_SMOKE_OFFSET;
			addEffect( &iVecEffect,EFFECT_CONSTRUCTION,CONSTRUCTION_TYPE_DRIFTING,FALSE,NULL,0 );
			iVecEffect.z = psNewDroid->y + DROID_CONSTRUCTION_SMOKE_OFFSET;
			addEffect( &iVecEffect,EFFECT_CONSTRUCTION,CONSTRUCTION_TYPE_DRIFTING,FALSE,NULL,0 );
			iVecEffect.x = psNewDroid->x + DROID_CONSTRUCTION_SMOKE_OFFSET;
			addEffect( &iVecEffect,EFFECT_CONSTRUCTION,CONSTRUCTION_TYPE_DRIFTING,FALSE,NULL,0 );
			iVecEffect.z = psNewDroid->y - DROID_CONSTRUCTION_SMOKE_OFFSET;
			addEffect( &iVecEffect,EFFECT_CONSTRUCTION,CONSTRUCTION_TYPE_DRIFTING,FALSE,NULL,0 );
		}
		/* add the droid to the list */
		addDroid(psNewDroid, apsDroidLists);
		*ppsDroid = psNewDroid;
		if ( psNewDroid->player == selectedPlayer )
		{
			audio_QueueTrack( ID_SOUND_DROID_COMPLETED );
			intRefreshScreen();	// update any interface implications.
		}

		// update the droid counts
		incNumDroids(psNewDroid->player);
		switch (psNewDroid->droidType)
		{
		case DROID_COMMAND:
			incNumCommandDroids(psNewDroid->player);
			break;
		case DROID_CONSTRUCT:
        case DROID_CYBORG_CONSTRUCT:
			incNumConstructorDroids(psNewDroid->player);
			break;
		default:
			break;
		}

		psFact = (FACTORY *)psStructure->pFunctionality;
		apx = psFact->psAssemblyPoint->coords.x;
		apy = psFact->psAssemblyPoint->coords.y;
//		orderDroidLoc(psNewDroid, DORDER_MOVE, apx,apy);		// factory waypoints handle this now.

		//add it to the factory group
/*		if (psNewDroid->droidType != DROID_COMMAND)
		{
			if (!psFact->psGroup)
			{
				//create the factory group
				if (!grpCreate(&psFact->psGroup))
				{
					DBPRINTF(("structPlaceDroid: unable to create group\n"));
				}
				else
				{
					grpJoin(psFact->psGroup, psNewDroid);
				}
			}
			else
			{
				grpJoin(psFact->psGroup, psNewDroid);
			}
		}*/

		// if we've built a command droid - make sure that it isn't assigned to another commander
		assignCommander = FALSE;
		if ((psNewDroid->droidType == DROID_COMMAND) &&
			(psFact->psCommander != NULL))
		{
			assignFactoryCommandDroid(psStructure, NULL);
			assignCommander = TRUE;
		}

		if ( psFact->psCommander != NULL )
		{
			if (idfDroid(psNewDroid) ||
				vtolDroid(psNewDroid))
			{
				orderDroidObj(psNewDroid, DORDER_FIRESUPPORT, (BASE_OBJECT *)psFact->psCommander);
				moveToRearm(psNewDroid);
			}
			else
			{
				cmdDroidAddDroid(psFact->psCommander, psNewDroid);
/*				orderDroidLoc(psNewDroid, DORDER_MOVE,
								psFact->psCommander->x,
								psFact->psCommander->y);*/
			}
		}
		else
		{
			//check flag against factory type
			factoryType = FACTORY_FLAG;
			if (psStructure->pStructureType->type == REF_CYBORG_FACTORY)
			{
				factoryType = CYBORG_FLAG;
			}
			else if (psStructure->pStructureType->type == REF_VTOL_FACTORY)
			{
				factoryType = VTOL_FLAG;
			}
            //if vtol droid - send it to ReArm Pad if one exists
            placed = FALSE;
            if (vtolDroid(psNewDroid))
            {
				moveToRearm(psNewDroid);
/*                psReArmPad = findNearestReArmPad(psNewDroid, NULL, FALSE);
                if (psReArmPad)
                {
                    orderDroidLoc(psNewDroid, DORDER_MOVE, psReArmPad->x, psReArmPad->y);
                    placed = TRUE;
                    //assign rearmPad to the VTOL
                    assignVTOLPad(psNewDroid, psReArmPad);
                }*/
            }
            if (!placed)
            {

			 //find flag in question.
			    for(psFlag = apsFlagPosLists[psFact->psAssemblyPoint->player];
						!( (psFlag->factoryInc == psFact->psAssemblyPoint->factoryInc)	// correct fact.
				        &&(psFlag->factoryType == factoryType));						// correct type
				    psFlag = psFlag->psNext);
	
		
                if (psNewDroid->droidType == DROID_TRANSPORTER)
                {
                    UDWORD droidX = psFlag->coords.x;
                    UDWORD droidY = psFlag->coords.y;
                    //find a suitable location near the delivery point
                    actionVTOLLandingPos(psNewDroid, &droidX, &droidY);
                    actionDroidLoc(psNewDroid, DACTION_MOVE, droidX,droidY);
                }
                else
                {
	    		    orderDroidLoc(psNewDroid,DORDER_MOVE,psFlag->coords.x,psFlag->coords.y);
                }
				

/*			    // check and process factory waypoints for this droid.
			    // order droid to each in turn.
			    for(i = 0; i <= psFact->psAssemblyPoint->factorySub; i++)
			    {
				    //find flag in question.
				    for(psFlag = apsFlagPosLists[psFact->psAssemblyPoint->player];
					    !( (psFlag->factoryInc == psFact->psAssemblyPoint->factoryInc)	// correct fact.
					      &&(psFlag->factoryType == factoryType)						// correct type
					      &&(psFlag->factorySub == i));									// correct flag.
					    psFlag = psFlag->psNext);

#ifndef PSX
					    if(bMultiPlayer)
					    {
						    bMultiPlayer = FALSE;
						    bTemp = TRUE;
						    SendSingleDroidWaypoint(psNewDroid,psFlag->coords.x,psFlag->coords.y);
					    }
					    else
					    {
						    bTemp = FALSE;
					    }
					    orderAddWayPoint(psNewDroid ,psFlag->coords.x,psFlag->coords.y);
					    if(bTemp)
					    {
						    bMultiPlayer = TRUE;
					    }
#else
    					// psx version, only one delivery point, no waypoint, so just order droid.
	    				orderDroidLoc(psNewDroid,DORDER_MOVE,psFlag->coords.x,psFlag->coords.y);
//#endif
               }
			*/
			}
		}

		if (assignCommander)
		{
			assignFactoryCommandDroid(psStructure, psNewDroid);
		}

/*

		// remove any out of date formation
		if (psFact->psFormation &&
			(psFact->psFormation->x != apx || psFact->psFormation->y != apy))
		{
			formationLeave(psFact->psFormation, (BASE_OBJECT *)psStructure);
			psFact->psFormation = NULL;
		}			
		// setup the formation for the droid if necessary
		if (psFact->psFormation == NULL)
		{
			if (formationNew(&psFact->psFormation, FT_LINE, apx,apy,
					(SDWORD)calcDirection((SDWORD)psStructure->x,(SDWORD)psStructure->y, apx,apy)))
			{
				formationJoin(psFact->psFormation, (BASE_OBJECT *)psStructure);
			}
		}
		// add the droid to the formation
		if (psFact->psFormation)
		{
			formationJoin(psFact->psFormation, (BASE_OBJECT *)psNewDroid);
			psNewDroid->sMove.psFormation = psFact->psFormation;
		}
*/
#ifdef SCRIPTS
		if ( psNewDroid->player == selectedPlayer )
		{
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DROIDBUILT);
		}
#endif
		psScrCBNewDroid = psNewDroid;
		psScrCBNewDroidFact = psStructure;
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_NEWDROID);
		psScrCBNewDroid = NULL;
		psScrCBNewDroidFact = NULL;
	}
	else
	{
		*ppsDroid = NULL;
	}
}


static BOOL IsFactoryCommanderGroupFull(FACTORY *psFactory)
{
	SDWORD DroidsInGroup;	

	// If we don't have a commander return FALSE (group not full)
	if (psFactory->psCommander==NULL) return FALSE;

	// allow any number of IDF droids
	if (templateIsIDF((DROID_TEMPLATE *)psFactory->psSubject))
	{
		return FALSE;
	}

	// Get the number of droids in the commanders group
	DroidsInGroup = psFactory->psCommander->psGroup ? grpNumMembers( psFactory->psCommander->psGroup ) : 0;

	// if the number in group is less than the maximum allowed then return FALSE (group not full)
	if (DroidsInGroup < cmdDroidMaxGroup( psFactory->psCommander )) return FALSE;

	// the number in group has reached the maximum
	return TRUE;	 
}


// Disallow manufacture of units once these limits are reached,
// dos'nt mean that these numbers can't be exceeded if units are
// put down in the editor or by the scripts.

static UWORD MaxDroidsAllowedPerPlayer[MAX_PLAYERS]={100,999,999,999,999,999,999,999};
static UWORD MaxDroidsAllowedPerPlayerMultiPlayer[MAX_PLAYERS]={300,300,300,300,300,300,300,300};
//static UWORD MaxDroidsAllowedPerPlayerMultiPlayer[MAX_PLAYERS]={10,10,10,10,10,10,10,10};


UDWORD getMaxStructures(UDWORD PlayerNumber)
{
	// PC currently doesn't limit number of structures a player can build, so just
	// return an absurdly large number.
	return 99999;
}


BOOL IsPlayerStructureLimitReached(UDWORD PlayerNumber)
{
	// PC currently doesn't limit number of structures a player can build.
	return FALSE;
} 


UDWORD getMaxDroids(UDWORD PlayerNumber)
{

	return (bMultiPlayer ? MaxDroidsAllowedPerPlayerMultiPlayer[PlayerNumber] : MaxDroidsAllowedPerPlayer[PlayerNumber] );

}


BOOL IsPlayerDroidLimitReached(UDWORD PlayerNumber)
{

	if (getNumDroids(PlayerNumber)+getNumMissionDroids(PlayerNumber)+getNumTransporterDroids(PlayerNumber) 
		>=
		(bMultiPlayer ? (MaxDroidsAllowedPerPlayerMultiPlayer[PlayerNumber]) : (MaxDroidsAllowedPerPlayer[PlayerNumber])) )

	{
		return TRUE;
	}

	return FALSE;
} 


BOOL maxDroidsByTypeReached(STRUCTURE *psStructure)
{
	FACTORY		*psFact = (FACTORY *)psStructure->pFunctionality;

	if ( (droidTemplateType((DROID_TEMPLATE *)psFact->psSubject) == DROID_COMMAND) &&
		 (getNumCommandDroids(psStructure->player) >= 10) )
	{
		return TRUE;
	}

	if ( (droidTemplateType((DROID_TEMPLATE *)psFact->psSubject) == DROID_CONSTRUCT OR
        droidTemplateType((DROID_TEMPLATE *)psFact->psSubject) == DROID_CYBORG_CONSTRUCT) &&
		 (getNumConstructorDroids(psStructure->player) >= 15) )
	{
		return TRUE;
	}

	return FALSE;
}


// Check for max number of units reached and halt production.
//
BOOL CheckHaltOnMaxUnitsReached(STRUCTURE *psStructure)
{
	// if the players that owns the factory has reached his (or hers) droid limit
	// then put production on hold & return - we need a message to be displayed here !!!!!!!
	if (IsPlayerDroidLimitReached(psStructure->player) ||
		maxDroidsByTypeReached(psStructure))
	{
		if ((psStructure->player == selectedPlayer) &&
			(lastMaxUnitMessage + MAX_UNIT_MESSAGE_PAUSE < gameTime))
		{
			addConsoleMessage(strresGetString(psStringRes,STR_GAM_MAXUNITSREACHED),DEFAULT_JUSTIFY);
			lastMaxUnitMessage = gameTime;
		}
		/*
#ifdef DEBUG
		else
		{
			addConsoleMessage("DEBUG:OPPONENT HAS REACHED DROID LIMIT",DEFAULT_JUSTIFY);
		}
#endif*/

//		holdProduction(psStructure);
		return TRUE;
	}

	return FALSE;
 }


void aiUpdateStructure(STRUCTURE *psStructure)
{
	BASE_STATS			*pSubject = NULL;
	UDWORD				pointsToAdd;//, iPower;
	PLAYER_RESEARCH		*pPlayerRes = asPlayerResList[psStructure->player];
	UDWORD				structureMode = 0;
	DROID				*psDroid;
	BASE_OBJECT			*psChosenObj = NULL;
	UBYTE				Quantity;
	SDWORD				iDt;
	FACTORY				*psFactory;
	REPAIR_FACILITY		*psRepairFac = NULL;
	RESEARCH_FACILITY	*psResFacility;
	REARM_PAD			*psReArmPad;
	iVector				iVecEffect;
	BOOL				bFinishAction;//bFinishRepair;
	WEAPON_STATS		*psWStats;
	BASE_OBJECT			*psTarget;
	SDWORD				xdiff,ydiff, mindist, currdist;
#ifdef INCLUDE_FACTORYLISTS
	DROID_TEMPLATE		*psNextTemplate;
#endif

	ASSERT((PTRVALID(psStructure, sizeof(STRUCTURE)),
		"aiUpdateStructure: invalid Structure pointer"));

	if (psStructure->psTarget &&
		psStructure->psTarget->died)
	{
		psStructure->psTarget = NULL;
	}

	// Will go out into a building EVENT stats/text file
	/* Spin round yer sensors! */
	//if (psStructure->numWeaps == 0)
    if ((psStructure->asWeaps[0].nStat == 0) &&
		(psStructure->pStructureType->type != REF_REPAIR_FACILITY))
	{

//////
// - radar should rotate every three seconds ... 'cause we timed it at Heathrow !
// gameTime is in milliseconds - one rotation every 3 seconds = 1 rotation event 3000 millisecs
		psStructure->turretRotation = (UWORD)(((gameTime*360)/3000)%360);

		psStructure->turretPitch = 0;
	}

	structUpdateRecoil(psStructure);

	/* See if there is an enemy to attack */
	//if (psStructure->numWeaps > 0) - don't bother looking for a target if Las Sat weapon
    if (psStructure->asWeaps[0].nStat > 0 AND 
        asWeaponStats[psStructure->asWeaps[0].nStat].weaponSubClass != WSC_LAS_SAT)
	{
		if ((psStructure->id % 20) == (frameGetFrameNumber() % 20))
		{
			if (aiChooseTarget((BASE_OBJECT *)psStructure, &psChosenObj))
			{
				DBP1(("Struct(%d) attacking : %d\n",
						psStructure->id, psChosenObj->id));
				psStructure->psTarget = psChosenObj;
			}
			else
			{
				psStructure->psTarget = NULL;
				psChosenObj = NULL;
			}
		}
		else
		{
			psChosenObj = psStructure->psTarget;
		}

		if (psChosenObj != NULL)
		{
			// get the weapon stat to see if there is a visible turret to rotate
			psWStats = asWeaponStats + psStructure->asWeaps[0].nStat;

			//if were going to shoot at something move the turret first then fire when locked on
			if (psWStats->pMountGraphic == NULL)//no turret so lock on whatever
			{
				psStructure->turretRotation = (UWORD)calcDirection(psStructure->x, 
					psStructure->y, psChosenObj->x, psChosenObj->y);
				combFire(psStructure->asWeaps, (BASE_OBJECT *)psStructure, psChosenObj);
			}
			/*else if(actionTargetTurret((BASE_OBJECT*)psStructure, psChosenObj,
									&(psStructure->turretRotation), 
									&(psStructure->turretPitch),psStructure->turretRotRate, 
									(UWORD)(psStructure->turretRotRate/2),
									proj_Direct(psWStats),   FALSE))*/
			else if(actionTargetTurret((BASE_OBJECT*)psStructure, psChosenObj,
									&(psStructure->turretRotation), 
									&(psStructure->turretPitch),
									psWStats, FALSE))
			{
				combFire(psStructure->asWeaps, (BASE_OBJECT *)psStructure, psChosenObj);
			}
		}
		else
		{
			// realign the turret
			if ( ((psStructure->turretRotation % 90) != 0) ||
				 (psStructure->turretPitch != 0) )
			{
				actionAlignTurret((BASE_OBJECT *)psStructure);
			}
		}
	}

	/* See if there is an enemy to attack for Sensor Towers that have weapon droids attached*/
	else if (psStructure->pStructureType->pSensor)
	{
		if (structStandardSensor(psStructure) OR structVTOLSensor(psStructure))
		{
			if ((psStructure->id % 20) == (frameGetFrameNumber() % 20))
			{
				if (aiChooseSensorTarget((BASE_OBJECT *)psStructure, &psChosenObj))
				{
					DBP1(("Struct(%d) attacking : %d\n",
						psStructure->id, psChosenObj->id));
					psStructure->psTarget = psChosenObj;
				}
				else
				{
					psStructure->psTarget = NULL;
				}
			}
			psChosenObj = psStructure->psTarget;
		}
		else
		{
			psChosenObj = psStructure->psTarget;
		}

		// you can always see anything that a CB sensor is targeting
		// Anyone commenting this out again will get a knee capping from John.
		// You have been warned!!
		if ((structCBSensor(psStructure) OR structVTOLCBSensor(psStructure)) &&
			psStructure->psTarget != NULL)
		{
			psStructure->psTarget->visible[psStructure->player] = UBYTE_MAX;
		}
	}
	//only interested if the Structure "does" something!
	if (psStructure->pFunctionality == NULL)
	{
		return;
	}

	//check if any power available
    if (structUsesPower(psStructure))
    {
//	    if ((asPower[psStructure->player]->currentPower > POWER_PER_CYCLE) OR 
//		    (!powerCalculated))
		if (checkPower(psStructure->player, POWER_PER_CYCLE, FALSE))
	    {
		    //check if this structure is due some power
		    if (getLastPowered((BASE_OBJECT *)psStructure))
		    {

//DBPRINTF(("pStructureType  %d\n",psStructure->pStructureType->type));
//if(psStructure->pStructureType->type == REF_FACTORY) {
//	FACTORY *Fact = psStructure->pFunctionality;
//	DBPRINTF(("Subject %p\n",Fact->psSubject));
//	DBPRINTF(("Accrued %d\n",Fact->powerAccrued));
//	DBPRINTF(("Started %d\n",Fact->timeStarted));
//	DBPRINTF(("Start Hold %d\n",Fact->timeStartHold));
//	if(Fact->psSubject) {
//		DBPRINTF(("Required %d\n",((DROID_TEMPLATE*)Fact->psSubject)->powerPoints ));
//	}
//}

			    //get some power if necessary
			    if (accruePower((BASE_OBJECT *)psStructure))
			    {
				    updateLastPowered((BASE_OBJECT *)psStructure, psStructure->player);
			    }
		    }
	    }
    }

	/* Process the functionality according to type
	 * determine the subject stats (for research or manufacture)
	 * or base object (for repair) or update power levels for resourceExtractor
	 */
	switch (psStructure->pStructureType->type)
	{
		/*case REF_RESOURCE_EXTRACTOR:
			updateExtractedPower(psStructure);
			break;*/
		case REF_RESEARCH:
		{
			pSubject = ((RESEARCH_FACILITY*)psStructure->pFunctionality)->psSubject;
			structureMode = REF_RESEARCH;
			break;
		}
		case REF_FACTORY:
		case REF_CYBORG_FACTORY:
		case REF_VTOL_FACTORY:
		{
			pSubject = ((FACTORY*)psStructure->pFunctionality)->psSubject;
			structureMode = REF_FACTORY;
            //check here to see if the factory's commander has died
            if (((FACTORY*)psStructure->pFunctionality)->psCommander AND
                ((FACTORY*)psStructure->pFunctionality)->psCommander->died)
            {
                //remove the commander from the factory
                assignFactoryCommandDroid(psStructure, NULL);
            }
			break;
		}
		case REF_REPAIR_FACILITY:
		{
			psRepairFac = (REPAIR_FACILITY *) psStructure->pFunctionality;
			psChosenObj = psRepairFac->psObj;
			structureMode = REF_REPAIR_FACILITY;
			psDroid = (DROID *)psChosenObj;

			// skip droids that are doing anything else
			if (	(psDroid != NULL)	 
				&&	(!orderState(psDroid, DORDER_RTR)
				||   psDroid->psTarget != (BASE_OBJECT *)psStructure) )
			{
//				if(psDroid->psGroup != NULL)
//				{
//					grpLeave(psRepairFac->psGroup, (DROID *)psChosenObj);
//				}
				psChosenObj = NULL;
				psDroid = NULL;
				psRepairFac->psObj = NULL;
			}

			/* select next droid if none being repaired */
			if ( psChosenObj == NULL )
			{
				ASSERT( (PTRVALID( psRepairFac->psGroup, sizeof(DROID_GROUP) ),
					"aiUpdateStructure: invalid repair facility group pointer" ) );

				// get droid next in repair queue
/*				changed this just to scan for the first droid waiting for repair
				cos using groups here causes all sorts of problems elsewhere. John
				while ( (psRepairFac->psGroup != NULL) &&
						(psRepairFac->psGroup->psList != NULL) )
				{
					psDroid = psRepairFac->psGroup->psList;
					iPower = (psDroid->originalBody - psDroid->body) *
								REPAIR_FACILITY_COST_PERCENT / 100;

					// only move to repair if droid still waiting else remove from group
					if ( psDroid->order != DORDER_RTR ||
						 !checkPower( psStructure->player, iPower, TRUE ) )
					{
						grpLeave( psRepairFac->psGroup, psDroid );
						// return droid back to front of repair facility 
						orderDroidLoc( psDroid, DORDER_MOVE, psStructure->x,
						psStructure->y +
						(psStructure->pStructureType->baseBreadth<<TILE_SHIFT)/2 + 2*TILE_UNITS );
					}
					else
					{
						break;
					}
				}*/
				mindist = SDWORD_MAX;
				for(psDroid = apsDroidLists[psStructure->player]; psDroid; psDroid = psDroid->psNext)
				{
					if (orderStateObj(psDroid, DORDER_RTR, &psTarget) &&
						(psTarget == (BASE_OBJECT *)psStructure) &&
						psDroid->action == DACTION_WAITFORREPAIR)
					{
						xdiff = (SDWORD)psDroid->x - (SDWORD)psStructure->x;
						ydiff = (SDWORD)psDroid->y - (SDWORD)psStructure->y;
						currdist = xdiff*xdiff + ydiff*ydiff;
						if (currdist < mindist)
						{
							mindist = currdist;
							psChosenObj = (BASE_OBJECT *)psDroid;
						}
					}
				}
				psDroid = (DROID *)psChosenObj;
			}

			// send the droid to be repaired
			if ( psDroid != NULL && 
				 psDroid->action == DACTION_WAITFORREPAIR )
			{
				/* set chosen object */
				psChosenObj = (BASE_OBJECT *)psDroid;
				psRepairFac->psObj = (BASE_OBJECT *)psDroid;

				/* move droid to repair point at rear of facility */
/*				actionDroidObjLoc( psDroid, DACTION_MOVETOREPAIRPOINT,
						(BASE_OBJECT *) psStructure, psStructure->x,
						psStructure->y -
						(psStructure->pStructureType->baseBreadth<<TILE_SHIFT)/2 -
						TILE_UNITS/2 );*/
				actionDroidObjLoc( psDroid, DACTION_MOVETOREPAIRPOINT,
						(BASE_OBJECT *) psStructure, psStructure->x, psStructure->y);
				/* reset repair started */
				psRepairFac->timeStarted = ACTION_START_TIME;
                psRepairFac->currentPtsAdded = 0;
				break;
			}
			break;
		}
		case REF_REARM_PAD:
		{
			psReArmPad = (REARM_PAD *) psStructure->pFunctionality;
			psChosenObj = psReArmPad->psObj;
			structureMode = REF_REARM_PAD;
			psDroid = NULL;

			/* select next droid if none being rearmed*/
			if (psChosenObj == NULL)
			{
				for(psDroid = apsDroidLists[psStructure->player]; psDroid; 
					psDroid = psDroid->psNext)
				{
					// move next droid waiting on ground to rearm pad
					if (vtolReadyToRearm(psDroid, psStructure) &&
						(psChosenObj == NULL || (((DROID *)psChosenObj)->actionStarted > psDroid->actionStarted)) )
					{
						psChosenObj = (BASE_OBJECT *)psDroid;
					}
				}
				psDroid = (DROID *)psChosenObj;
				if (psDroid != NULL)
				{
					actionDroidObj( psDroid, DACTION_MOVETOREARMPOINT,
									(BASE_OBJECT *) psStructure );
				}
			}
			else
			{
				psDroid = (DROID *) psChosenObj;
				if ( (psDroid->sMove.Status == MOVEINACTIVE ||
					  psDroid->sMove.Status == MOVEHOVER       ) &&
					 psDroid->action == DACTION_WAITFORREARM        )
				{
					actionDroidObj( psDroid, DACTION_MOVETOREARMPOINT,
									(BASE_OBJECT *) psStructure );
				}
			}

			// if found a droid to rearm assign it to the rearm pad
			if (psDroid != NULL)
			{
				/* set chosen object */
				psChosenObj = (BASE_OBJECT *)psDroid;
				psReArmPad->psObj = psChosenObj;
				if ( psDroid->action == DACTION_MOVETOREARMPOINT )
				{
					/* reset rearm started */
					psReArmPad->timeStarted = ACTION_START_TIME;
                    psReArmPad->currentPtsAdded = 0;
				}
			}
		}
		break;
	}

	/* check subject stats (for research or manufacture) */
	if (pSubject != NULL)
	{
		//if subject is research...
		if (structureMode == REF_RESEARCH)
		{
			psResFacility = (RESEARCH_FACILITY*)psStructure->pFunctionality;

			//if on hold don't do anything
			if (psResFacility->timeStartHold)
			{
				return;
			}

#ifdef TEST_EW
    bMultiPlayer = TRUE;
#endif
            //electronic warfare affects the functionality of some structures in multiPlayer
            if (bMultiPlayer)
            {
                if (psStructure->resistance < (SWORD)structureResistance(psStructure->
                    pStructureType, psStructure->player))
                {
                    return;
                }
            }

#ifdef TEST_EW
    bMultiPlayer = FALSE;
#endif
			pPlayerRes += (pSubject->ref - REF_RESEARCH_START);
			//check research has not already been completed by another structure
			if (IsResearchCompleted(pPlayerRes)==0)
			{
				//check to see if enough power to research has accrued
				if (psResFacility->powerAccrued < ((RESEARCH *)pSubject)->researchPower)
				{
					//wait until enough power
					return;
				}


				// don't update if not responsible (106)
				if(bMultiPlayer && !myResponsibility(psStructure->player))
				{
					return;
				}

				if (psResFacility->timeStarted == ACTION_START_TIME)
				{
					//set the time started
					psResFacility->timeStarted = gameTime;
				}

				pointsToAdd = (psResFacility->researchPoints * (gameTime - 
                    psResFacility->timeStarted)) / GAME_TICKS_PER_SEC; 

				//check if Research is complete

				//if ((pointsToAdd + pPlayerRes->currentPoints) > psResFacility->
				//	timeToResearch)
                if ((pointsToAdd + pPlayerRes->currentPoints) > (
                    (RESEARCH *)pSubject)->researchPoints)

				{
					/*Done in research Result now - AB 31/1/98
					gameTimeStop();
					audio_PlayTrack( ID_SOUND_MAJOR_RESEARCH );
					gameTimeStart();*/
					//pPlayerRes->researched = RESEARCHED;


					if(bMultiPlayer)
					{
						SendResearch(psStructure->player,pSubject->ref - REF_RESEARCH_START);
					}


					//store the last topic researched - if its the best
					if (psResFacility->psBestTopic == NULL)
					{
						psResFacility->psBestTopic = psResFacility->psSubject;
					}
					else
					{
						if (((RESEARCH *)psResFacility->psSubject)->researchPoints > 
							((RESEARCH *)psResFacility->psBestTopic)->researchPoints)
						{
							psResFacility->psSubject = psResFacility->psSubject;
						}
					}
					psResFacility->psSubject = NULL;
					intResearchFinished(psStructure);
					researchResult(pSubject->ref - REF_RESEARCH_START, 
						psStructure->player, TRUE);
					//check if this result has enabled another topic
					intCheckResearchButton();
				}
			}
			else
			{
				//cancel this Structure's research since now complete
				psResFacility->psSubject = NULL;
				intResearchFinished(psStructure);
			}
		}
		//check for manufacture
		else if (structureMode == REF_FACTORY)
		{
			psFactory = (FACTORY *)psStructure->pFunctionality;
			Quantity = psFactory->quantity;

			//if on hold don't do anything
			if (psFactory->timeStartHold)
			{
				return;
			}

#ifdef TEST_EW
    bMultiPlayer = TRUE;
#endif
            //electronic warfare affects the functionality of some structures in multiPlayer
            if (bMultiPlayer)
            {
                if (psStructure->resistance < (SWORD)structureResistance(psStructure->
                    pStructureType, psStructure->player))
                {
                    return;
                }
            }

#ifdef TEST_EW
    bMultiPlayer = FALSE;
#endif
			if (psFactory->timeStarted == ACTION_START_TIME)
			{
				/*if (Quantity == NON_STOP_PRODUCTION)
				{
					if (!checkPower(psStructure->player, 
						((DROID_TEMPLATE *)pSubject)->powerPoints, FALSE))
					{
						return;
					}
				}*/

				// also need to check if a command droid's group is full

				// If the factory commanders group is full - return 
				if (IsFactoryCommanderGroupFull(psFactory)==TRUE)
				{
					return;
				}

				if(CheckHaltOnMaxUnitsReached(psStructure) == TRUE) {
					return;
				}

				/*subtract the power required to build*/
				/*if (usePower(psStructure->player, ((DROID_TEMPLATE *)pSubject)->
					powerPoints))
				{
					//set the time started
					psFactory->timeStarted = gameTime;
				}
				else
				{
					if(psStructure->player == selectedPlayer)
					{
						addConsoleMessage("Droid build: No Power",
							DEFAULT_JUSTIFY);
					}
					//not got enough power so cancel
					psFactory->quantity = 0;
					psFactory->psSubject = NULL;
					intManufactureFinished(psStructure);
					return;
				}*/
			}

			//check enough power has accrued to build the droid
			if (psFactory->powerAccrued < ((DROID_TEMPLATE *)pSubject)->
					powerPoints)
			{
				//wait until enough power
				return;
			}

			/*must be enough power so subtract that required to build*/
			if (psFactory->timeStarted == ACTION_START_TIME)
			{
				/*if (usePower(psStructure->player, ((DROID_TEMPLATE *)pSubject)->
					powerPoints))
				{
					//set the time started
					psFactory->timeStarted = gameTime;
				}
				else
				{
					//This shouldn't happen so cancel!
					psFactory->quantity = 0;
					psFactory->psSubject = NULL;
					intManufactureFinished(psStructure);
					return;
				}*/

				//set the time started
				psFactory->timeStarted = gameTime;
			}

			pointsToAdd = (gameTime - psFactory->timeStarted) / 
				GAME_TICKS_PER_SEC;

			//check for manufacture to be complete
			if ((pointsToAdd > psFactory->timeToBuild) &&
				!IsFactoryCommanderGroupFull(psFactory) &&
				!CheckHaltOnMaxUnitsReached(psStructure))
			{
				/* Place the droid on the map */
				structPlaceDroid(psStructure, (DROID_TEMPLATE *)pSubject, &psDroid);

				//reset the start time
				psFactory->timeStarted = ACTION_START_TIME;
				psFactory->powerAccrued = 0;

#ifdef INCLUDE_FACTORYLISTS
				//next bit for productionPlayer only 
				if (productionPlayer == psStructure->player)
				{
					psNextTemplate = factoryProdUpdate(psStructure,(DROID_TEMPLATE *)pSubject);
					if (psNextTemplate)
					{
						structSetManufacture(psStructure, psNextTemplate,Quantity);
						return;
					}
					else 
					{
						//nothing more to manufacture - reset the Subject and Tab on HCI Form
						intManufactureFinished(psStructure);
						psFactory->psSubject = NULL;
					}
				}
				else
#endif
				{
					//decrement the quantity to manufacture if not set to infinity
					if (Quantity AND Quantity != NON_STOP_PRODUCTION)
					{
						psFactory->quantity--;
						Quantity--;
					}
					
					// If quantity not 0 then kick of another manufacture.
					if(Quantity) 
					{	
						// Check enough power to continue production run.
						/*if (!checkPower(psStructure->player, ((DROID_TEMPLATE *)pSubject)->
							powerPoints, (Quantity != NON_STOP_PRODUCTION)))
						{
							// If not then stop the run. - only if not set to NON_STOP PRODUCTION
							if (Quantity != NON_STOP_PRODUCTION)
							{
								psFactory->quantity = 0;
								psFactory->psSubject = NULL;
								intManufactureFinished(psStructure);
								//playerNewDroid(psDroid);
							}
							return;
						}*/

						// Manufacture another.
						structSetManufacture(psStructure, (DROID_TEMPLATE*)pSubject,Quantity);
						//playerNewDroid(psDroid);
						return;
					} 
					else 
					{
						//when quantity = 0, reset the Subject and Tab on HCI Form
						psFactory->psSubject = NULL;
						intManufactureFinished(psStructure);
					}
				}
			}
		}
	}

	/* check base object (for repair / rearm) */
	if ( psChosenObj != NULL )
	{
		if ( structureMode == REF_REPAIR_FACILITY )
		{
            UDWORD  powerCost;//, iPower;

			psDroid = (DROID *) psChosenObj;
			ASSERT( (PTRVALID(psDroid, sizeof(DROID)),
					"aiUpdateStructure: invalid droid pointer" ) );
			psRepairFac = (REPAIR_FACILITY*)psStructure->pFunctionality;

			if ( psDroid->action == DACTION_WAITDURINGREPAIR &&
				 actionTargetTurret((BASE_OBJECT*)psStructure, psChosenObj,
									&(psStructure->turretRotation), 
									&(psStructure->turretPitch),
									NULL, FALSE))
			{
                //Do repair gradually as power comes in
//				POWER REQUIRMENTS REMOVED - AB  22/09/98 - and back again 07/01/99
                /*iPower = powerReqForDroidRepair(psDroid);
				//check to see if enough power to research has accrued
				if (psRepairFac->powerAccrued < iPower)
				{
					//wait until enough power
					return;
				}*/

                //check droid is not healthy
                if (psDroid->body < psDroid->originalBody)
                {
                    //if in multiPlayer, and a Transporter - make sure its on the ground before repairing
                    if (bMultiPlayer AND psDroid->droidType == DROID_TRANSPORTER)
                    {
                        if (!(psDroid->sMove.Status == MOVEINACTIVE AND 
                            psDroid->sMove.iVertSpeed == 0))
                        {
                            return;
                        }
                    }
#ifdef TEST_EW
    bMultiPlayer = TRUE;
#endif
                    //don't do anything if the resistance is low in multiplayer
                    if (bMultiPlayer)
                    {
                        if (psStructure->resistance < (SWORD)structureResistance(psStructure->
                            pStructureType, psStructure->player))
                        {
                            return;
                        }
                    }
#ifdef TEST_EW
    bMultiPlayer = FALSE;
#endif
                    //check if enough power to do any
                    //powerCost = (powerReqForDroidRepair(psDroid) - 
                      //  psDroid->powerAccrued) / POWER_FACTOR;
                    powerCost = powerReqForDroidRepair(psDroid);
                    if (powerCost > psDroid->powerAccrued)
                    {
                        powerCost = (powerCost - psDroid->powerAccrued) / POWER_FACTOR;
                    }
                    else
                    {
                        powerCost = 0;
                    }
                    //if the power cost is 0 (due to rounding) then do for free!
                    if (powerCost)
                    {
                        if (!psDroid->powerAccrued)
                        {
                            //reset the actionStarted time and actionPoints added so the correct 
                            //amount of points are added when there is more power
			    		    psRepairFac->timeStarted = gameTime;
                            //init so repair points to add won't be huge when start up again
                            psRepairFac->currentPtsAdded = 0;
                            return;
                        }
                    }

				    if (psRepairFac->timeStarted == ACTION_START_TIME)
				    {
					    //must be enough power so subtract that required to research
					    /*if (usePower(psStructure->player, iPower))
					    {
						    //set the time started
						    psRepairFac->timeStarted = gameTime;
					    }
					    else
					    {
						    //This shouldn't happen so cancel!
						    psRepairFac->psObj = NULL;
						    return;
					    }*/
					    //set the time started
					    psRepairFac->timeStarted = gameTime;
                        //reset the points added
                        psRepairFac->currentPtsAdded = 0;
				    }

				    /* do repairing */
				    iDt = gameTime - psRepairFac->timeStarted;
                     //- this was a bit exponential ...
				    pointsToAdd = (iDt * psRepairFac->power / GAME_TICKS_PER_SEC) - 
                        psRepairFac->currentPtsAdded;
				    //bFinishRepair = FALSE;
				    bFinishAction = FALSE;

				    //do some repair
                    if (pointsToAdd)
                    {
                        //just add the points if the power cost is negligable
                        //if these points would make the droid healthy again then just add
                        if (!powerCost OR (psDroid->body + pointsToAdd >= psDroid->originalBody))
                        {
                            //anothe HACK but sorts out all the rounding errors when values get small
                            psDroid->body += pointsToAdd;
                            psRepairFac->currentPtsAdded += pointsToAdd;
                        }
                        else
                        {
                            //see if we have enough power to do this amount of repair
                            powerCost = pointsToAdd * repairPowerPoint(psDroid);
                            if (powerCost <= psDroid->powerAccrued)
                            {
                                psDroid->body += pointsToAdd;
                                psRepairFac->currentPtsAdded += pointsToAdd;
                                //subtract the power cost for these points
                                psDroid->powerAccrued -= powerCost;
                            }
                            else
                            {
                                /*reset the actionStarted time and actionPoints added so the correct 
                                amount of points are added when there is more power*/
                                psRepairFac->timeStarted = gameTime;
                                psRepairFac->currentPtsAdded = 0;
                            }
                        }
                    }
                }

				if ( psDroid->body >= psDroid->originalBody )
				{
					DBPRINTF( ("aiUpdateStructure: repair completed\n") );

					psRepairFac->psObj = NULL;

					/* set droid points to max */
					psDroid->body = psDroid->originalBody;
                    //reset the power accrued
                    psDroid->powerAccrued = 0;

					/* flag repair finished */
			 //		bFinishRepair = TRUE;

					// if completely repaired reset order 
					secondarySetState(psDroid, DSO_RETURN_TO_LOC, 0);

					if ((psDroid->psGroup != NULL) &&
						(psDroid->psGroup->type == GT_COMMAND) &&
						(psDroid->droidType != DROID_COMMAND))
					{
						// return a droid to it's command group
						DROID	*psCommander = psDroid->psGroup->psCommander;

//						orderDroidLoc(psDroid, DORDER_MOVE, psCommander->x, psCommander->y);
						orderDroidObj(psDroid, DORDER_GUARD, (BASE_OBJECT *)psCommander);
					}
					else if (psRepairFac->psDeliveryPoint != NULL)
					{
						// move the droid out the way
						orderDroidLoc( psDroid, DORDER_MOVE,
							 psRepairFac->psDeliveryPoint->coords.x,
							 psRepairFac->psDeliveryPoint->coords.y );
/*						orderDroidLoc( psDroid, DORDER_MOVE, psStructure->x,
							psStructure->y +
							(psStructure->pStructureType->baseBreadth<<TILE_SHIFT)/2 + 2*TILE_UNITS );*/
//						moveShuffleDroid(psDroid, -TILE_UNITS, 0);
					}
				}

				/* check whether repair finished */
				/*if ( bFinishRepair == TRUE )
				{
					// release droid from structure 
					((REPAIR_FACILITY*)psStructure->pFunctionality)->psObj = NULL;
#ifndef PSX
					// This is very new Alex M - Ask John????
			   //		if(psDroid->psOldCommander AND !psDroid->psOldCommander->died)
			   //		{
			   //			cmdDroidAddDroid(psDroid->psOldCommander, psDroid);
			   //		}
					// This is very new Alex M - Ask John????
#endif
				}
				else*/
				if (psStructure->visible[selectedPlayer] AND psDroid->visible[selectedPlayer])
				{
					/* add plasma repair effect whilst being repaired */
					iVecEffect.x = psDroid->x + (10-rand()%20);
					iVecEffect.y = psDroid->z + (10-rand()%20);
					iVecEffect.z = psDroid->y + (10-rand()%20);
					effectSetSize(100);
					addEffect( &iVecEffect,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,
								TRUE,getImdFromIndex(MI_FLAME),0 );
				}
			}
		}
		//check for rearming
		else if (structureMode == REF_REARM_PAD)
		{
			psReArmPad = (REARM_PAD *)psStructure->pFunctionality;

			psDroid = (DROID *)psChosenObj;
			ASSERT( (PTRVALID(psDroid, sizeof(DROID)),
					"aiUpdateStructure: invalid droid pointer" ) );
			ASSERT((vtolDroid(psDroid),"aiUpdateStructure: invalid droid type"));

			//check hasn't died whilst waiting to be rearmed
			// also clear out any previously repaired droid
			if ( psDroid->died ||
				 ( //vtolHappy(psDroid) &&
// anything moving to/waiting for rearm will get chosen again if it is the nearest
// but it could be a long way from the rearm pad
//				   (psDroid->action != DACTION_MOVETOREARM) &&
//				   (psDroid->action != DACTION_WAITFORREARM) &&
				   (psDroid->action != DACTION_MOVETOREARMPOINT) &&
				   (psDroid->action != DACTION_WAITDURINGREARM) ) )
			{
				psReArmPad->psObj = NULL;
				return;
			}

			//if waiting to be rearmed
			if ( psDroid->action == DACTION_WAITDURINGREARM &&
				 psDroid->sMove.Status == MOVEINACTIVE )
			{
				if (psReArmPad->timeStarted == ACTION_START_TIME)
				{
					//set the time started
					psReArmPad->timeStarted = gameTime;
				}

				bFinishAction = FALSE;


				if(!bMultiPlayer || myResponsibility(psDroid->player))

				{	// dont rearm on remote pcs.

					/* do rearming */
					if (psDroid->sMove.iAttackRuns != 0)
					{
						UDWORD      pointsRequired;

						//amount required is a factor of the droids' weight
						pointsRequired = psDroid->weight / REARM_FACTOR;
						pointsToAdd = psReArmPad->reArmPoints * (gameTime - psReArmPad->timeStarted) / 
							GAME_TICKS_PER_SEC;
						//if ((SDWORD)(psDroid->sMove.iAttackRuns - pointsToAdd) <= 0)
						if (pointsToAdd >= pointsRequired)
						{
							/* set rearm value to no runs made */
							psDroid->sMove.iAttackRuns = 0;
							//reset ammo and lastTimeFired
							psDroid->asWeaps[0].ammo = asWeaponStats[psDroid->
								asWeaps[0].nStat].numRounds;
							psDroid->asWeaps[0].lastFired = 0;
						}
						else
						{
							if (pointsToAdd >= pointsRequired/psDroid->sMove.iAttackRuns)
							{
								psDroid->sMove.iAttackRuns--;
							}
						}
					}
				}

				/* do repairing */
				if (psDroid->body < psDroid->originalBody)
				{
					pointsToAdd =  VTOL_REPAIR_FACTOR * (gameTime - 
                        psReArmPad->timeStarted) / GAME_TICKS_PER_SEC;
                    //this was exponential...
					//psDroid->body += pointsToAdd;
                    if ((pointsToAdd - psReArmPad->currentPtsAdded) > 0)
                    {
		    		    psDroid->body += (pointsToAdd - psReArmPad->currentPtsAdded);
                        psReArmPad->currentPtsAdded = pointsToAdd;
                    }
					if (psDroid->body >= psDroid->originalBody)
					{
						/* set droid points to max */
						psDroid->body = psDroid->originalBody;
					}
				}

				//check for fully armed and fully repaired
				if (vtolHappy(psDroid))
				{
					/*if ((psDroid->psGroup != NULL) &&
						(psDroid->psGroup->type == GT_COMMAND) &&
						(psDroid->droidType != DROID_COMMAND))
					{
						// return a droid to it's command group
						DROID	*psCommander = psDroid->psGroup->psCommander;
						orderDroidLoc(psDroid, DORDER_MOVE, psCommander->x, psCommander->y);

					}
					else
					{
						//don't use an order since could be in the middle of a FIRE_SUPPORT order
						actionDroid(psDroid, DACTION_NONE);
						//if not on fire support duty return back to previous place
						if (psDroid->order != DORDER_FIRESUPPORT)
						{
							//don't remove target
							//psDroid->psTarget = NULL;
							secondarySetState(psDroid, DSO_RETURN_TO_LOC, 0);
						}
					}*/
                    /*move the droid off the pad - don't want it to follow Commander - it 
                    just has to sit next to the rearm pad until it gets another target
                    By moving the vtol to the rearm pad it will actually move it off it!*/
                    //if the droid no longer has a target move it back to its base ReArm Pad

					/*  Totally ditch all this now - John
						The action just gets set to DACTION_NONE - the droid's order can
						then catch this and tell it what to do if anything.
						The droid will also now just sit on the pad until another droid arrives
                    if (psDroid->psTarget == NULL OR (psDroid->order == 
                        DORDER_FIRESUPPORT AND psDroid->psActionTarget == NULL))
                    {
                        if (psDroid->psBaseStruct)
                        {
                            actionDroidLoc(psDroid, DACTION_MOVE, 
                                psDroid->psBaseStruct->x, psDroid->psBaseStruct->y);
                        }
                        else
                        {
                            //just move it off
                            actionDroidLoc(psDroid, DACTION_MOVE, psStructure->x, 
                                psStructure->y);
                        }
                    }
                    else
                    {
                        //just move it off
                        actionDroidLoc(psDroid, DACTION_MOVE, psStructure->x, 
                            psStructure->y);
                    }*/


					if( bMultiPlayer)
					{
						sendHappyVtol(psDroid);
					}			

					//clear the rearm pad
					psDroid->action = DACTION_NONE;
					bFinishAction = TRUE;
					psReArmPad->psObj = NULL;

				}
				/* check whether rearm finished */
#if 0
				if ( !bFinishAction &&
					 ((ONEINFIVE) AND (psStructure->visible[selectedPlayer])) )
				{
					/* add plasma repair effect whilst being repaired */
					iVecEffect.x = psDroid->x;
					iVecEffect.y = psDroid->z;
					iVecEffect.z = psDroid->y;
					effectSetSize(100);
					addEffect( &iVecEffect,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED_SOLID,
								TRUE,getImdFromIndex(MI_PLASMA),0 );
				}
#endif
			}
			////check for full vtol droid being assigned
			//else if (!vtolEmpty(psDroid))
			//check for fully armed and repaired vtol droid being assigned
/*			else if (vtolHappy(psDroid))
			{
				// release droid from structure
				psReArmPad->psObj = NULL;
			}*/
			/*else if (psDroid->action == DACTION_NONE)
			{
				// release droid from structure
				psReArmPad->psObj = NULL;
				// halt droid if no order set else assume was waiting for rearm 
				if ( psDroid->order == DORDER_NONE )
				{
					actionDroid(psDroid, DACTION_NONE );
				}
				else
				{
					actionDroidObj(psDroid, DACTION_MOVETOREARM, (BASE_OBJECT *)psStructure);
				}
			}*/
		}
	}
}

/* Decides whether a structure should emit smoke when it's damaged */
BOOL	canSmoke(STRUCTURE *psStruct)
{
	if(psStruct->pStructureType->type == REF_WALL OR
		psStruct->pStructureType->type == REF_WALLCORNER)
	{
		return(FALSE);
	}
	else
	{
		return(TRUE);
	}
}

/* The main update routine for all Structures */
void structureUpdate(STRUCTURE *psBuilding)
{
UDWORD			widthScatter,breadthScatter;
UDWORD			percentDamage, emissionInterval, iPointsToAdd, iPointsRequired;
iVector			dv;

	ASSERT((PTRVALID(psBuilding, sizeof(STRUCTURE)),
		"structureUpdate: Invalid Structure pointer"));

	//update the manufacture/research of the building once complete
	if (psBuilding->status == SS_BUILT)
	{
		aiUpdateStructure(psBuilding);
	}

	if(psBuilding->status!=SS_BUILT)
	{
		if(psBuilding->selected)
		{
			psBuilding->selected = FALSE;
		}
	}

	//--

	/* Only add smoke if they're visible and they can 'burn' */
	if(psBuilding->visible[selectedPlayer] AND canSmoke(psBuilding))
	{
		//percentDamage = (100 - PERCENT(psBuilding->body, psBuilding->
		//	baseBodyPoints));
		percentDamage = (100 - PERCENT(psBuilding->body, structureBody(psBuilding)));
		// Is there any damage? 
		if(percentDamage!=0)
		{
			if(percentDamage>=100)
			{
				percentDamage = 99;
			}
			emissionInterval = CALC_STRUCTURE_SMOKE_INTERVAL(percentDamage);
			//if(gameTime > (psBuilding->lastEmission+psBuilding->emissionInterval))
			if(gameTime > (psBuilding->lastEmission + emissionInterval))
			{
   				widthScatter = ((psBuilding->pStructureType->baseWidth) * TILE_UNITS/2)/3;
				breadthScatter = ((psBuilding->pStructureType->baseBreadth) * TILE_UNITS/2)/3;
				dv.x = psBuilding->x + widthScatter - rand()%(2*widthScatter);
				dv.z = psBuilding->y + breadthScatter - rand()%(2*breadthScatter);
				dv.y = psBuilding->z;
				dv.y += ((psBuilding->sDisplay.imd->ymax)*3)/4;
				addEffect(&dv,EFFECT_SMOKE,SMOKE_TYPE_DRIFTING_HIGH,FALSE,NULL,0);
				psBuilding->lastEmission = gameTime;
				//psBuilding->emissionInterval = CALC_STRUCTURE_SMOKE_INTERVAL(percentDamage);
			}
		}
	}

	if ((psBuilding->id % 10) == (frameGetFrameNumber() % 10))
	{
		processVisibility((BASE_OBJECT*)psBuilding);
	}

	/* Update the fire damage data */
	if (psBuilding->inFire & IN_FIRE)
	{
		/* Still in a fire, reset the fire flag to see if we get out this turn */
		psBuilding->inFire = 0;
	}
	else
	{
		/* The fire flag has not been set so we must be out of the fire */
		psBuilding->burnStart = 0;
		psBuilding->burnDamage = 0;
	}

	//check the resistance level of the structure
	//if (psBuilding->resistance < (SDWORD)psBuilding->pStructureType->resistance)
    iPointsRequired = structureResistance(psBuilding->pStructureType, 
        psBuilding->player);
	if (psBuilding->resistance < (SWORD)iPointsRequired)
	{
        //start the resistance increase
        if (psBuilding->lastResistance == ACTION_START_TIME)
        {
            psBuilding->lastResistance = gameTime;
        }
		//increase over time if low
		if ((gameTime - psBuilding->lastResistance) > RESISTANCE_INTERVAL)
		{
			psBuilding->resistance++;
#ifdef TEST_EW
    bMultiPlayer = TRUE;
#endif
            //in multiplayer, certain structures do not function whilst low resistance
            if (bMultiPlayer)
            {
                resetResistanceLag(psBuilding);
            }
#ifdef TEST_EW
    bMultiPlayer = FALSE;
#endif
			psBuilding->lastResistance = gameTime;
            //once the resistance is back up reset the last time increased
            if (psBuilding->resistance >= (SWORD)iPointsRequired)
            {
                psBuilding->lastResistance = ACTION_START_TIME;
            }
		}
	}
    else 
    {
        //if selfrepair has been researched then check the health level of the 
        //structure once resistance is fully up
        iPointsRequired = structureBody(psBuilding);
        if (selfRepairEnabled(psBuilding->player) AND (psBuilding->body < (SWORD)
            iPointsRequired))
        {
            //start the self repair off
            if (psBuilding->lastResistance == ACTION_START_TIME)
            {
                psBuilding->lastResistance = gameTime;
            }

       	    /*since self repair, then add half repair points depending on the time delay 
            for the stat*/
	        iPointsToAdd = (repairPoints(asRepairStats + aDefaultRepair[
                psBuilding->player], psBuilding->player) / 4) * ((gameTime - 
                psBuilding->lastResistance) / (asRepairStats + 
                aDefaultRepair[psBuilding->player])->time);

            //add the blue flashing effect for multiPlayer
            if(bMultiPlayer AND ONEINTEN)
	        {
                iVector position, *point;
                SDWORD	realY;
                UDWORD	pointIndex;

                pointIndex = rand()%(psBuilding->sDisplay.imd->npoints-1);
                point = &(psBuilding->sDisplay.imd->points[pointIndex]);
	            position.x = psBuilding->x + point->x;
	            realY = MAKEINT((structHeightScale(psBuilding) * point->y));
	            position.y = psBuilding->z + realY;
	            position.z = psBuilding->y - point->z;

		        effectSetSize(30);
    			addEffect(&position,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,TRUE,
                    getImdFromIndex(MI_PLASMA),0);
        	}

		    if (iPointsToAdd)
    	    {
	    	    psBuilding->body = (UWORD)(psBuilding->body + iPointsToAdd);
		        psBuilding->lastResistance = gameTime;
	            if ( psBuilding->body > iPointsRequired)
	            {
		            psBuilding->body = (UWORD)iPointsRequired;
                    psBuilding->lastResistance = ACTION_START_TIME;
                }          
		    }
        }
    }
}

/* Release all resources associated with a structure */
void structureRelease(STRUCTURE *psBuilding)
{
	FLAG_POSITION	*psAssemblyPoint;

	if (psBuilding->pFunctionality != NULL)
	{
		psAssemblyPoint = NULL;
		if (StructIsFactory(psBuilding))
		{
			// free up factory stuff
			if (((FACTORY *)psBuilding->pFunctionality)->psFormation)
			{
				formationReset(((FACTORY *)psBuilding->pFunctionality)->psFormation);
			}
            //not used anymore - AB 11/02/99
			//remove from the group it is in
			/*if (((FACTORY *)psBuilding->pFunctionality)->psGroup != NULL)
			{
				grpLeave(((FACTORY *)psBuilding->pFunctionality)->psGroup, NULL);
			}*/

			psAssemblyPoint = ((FACTORY *)psBuilding->pFunctionality)->psAssemblyPoint;

			// remove any commander from the factory
			if (((FACTORY *)psBuilding->pFunctionality)->psCommander != NULL)
			{
				assignFactoryCommandDroid(psBuilding, NULL);
			}
		}
		else if (psBuilding->pStructureType->type == REF_REPAIR_FACILITY)
		{
			// free up repair fac stuff
			psAssemblyPoint = ((REPAIR_FACILITY *)psBuilding->pFunctionality)->psDeliveryPoint;
		}

		// remove any assembly points
		if (psAssemblyPoint != NULL)
		{
			removeFlagPosition(psAssemblyPoint);
		}

		//free up the space used by the functionality array
		removeStructFunc(psBuilding->pFunctionality);
	}

	// remove the object from the grid
	gridRemoveObject((BASE_OBJECT *)psBuilding);
}


/* 
fills the list with Structure that can be built. There is a limit on how many can 
be built at any one time. Pass back the number available.
There is now a limit of how many of each type of structure are allowed per mission
*/
// PhilStructureList()  -- ha ha ha
UDWORD fillStructureList(STRUCTURE_STATS **ppList, UDWORD selectedPlayer, UDWORD limit)
{
	UDWORD			inc, count;
	BOOL			researchModule, factoryModule, powerModule;
	STRUCTURE		*psCurr;
	STRUCTURE_STATS	*psBuilding;

	//check to see if able to build research/factory modules
	researchModule = factoryModule = powerModule = FALSE;

	//if currently on a mission can't build factory/research/power/derricks
	//if (mission.type != LDS_MKEEP AND mission.type != LDS_MCLEAR)
    if (!missionIsOffworld())
	{
		for (psCurr = apsStructLists[selectedPlayer]; psCurr != NULL; psCurr = 
			psCurr->psNext)
		{
			if (psCurr->pStructureType->type == REF_RESEARCH AND psCurr->status == 
				SS_BUILT)
			{
				researchModule = TRUE;
			}
			else if (psCurr->pStructureType->type == REF_FACTORY AND psCurr->status == 
				SS_BUILT)
			{
				factoryModule = TRUE;
			}
			else if (psCurr->pStructureType->type == REF_POWER_GEN AND psCurr->status == 
				SS_BUILT)
			{
				powerModule = TRUE;
			}
		}
	}

	count = 0;
	//set the list of Structures to build
	for (inc=0; inc < numStructureStats; inc++)
	{
		//if the structure is flagged as available, add it to the list
		if (apStructTypeLists[selectedPlayer][inc] & AVAILABLE)
		{
/*DBPRINTF(("Structure %d is avail - current = %d limit = %d\n",inc,
			asStructLimits[selectedPlayer][inc].currentQuantity , 
					asStructLimits[selectedPlayer][inc].limit));*/

			//check not built the maximum allowed already
			if (asStructLimits[selectedPlayer][inc].currentQuantity < 
					asStructLimits[selectedPlayer][inc].limit)
			{
				psBuilding = asStructureStats + inc;

                //don't want corner wall to appear in list
                if (psBuilding->type == REF_WALLCORNER)
                {
                    continue;
                }

// Remove the demolish stat from the list for tutorial
// tjc 4-dec-98  ... 
				if (bInTutorial)
				{
					if (psBuilding->type == REF_DEMOLISH) continue;
				}


				//can't build list when offworld
				//if (mission.type == LDS_MKEEP OR mission.type == LDS_MCLEAR)
                if (missionIsOffworld())
				{
					if (psBuilding->type == REF_FACTORY OR
						psBuilding->type == REF_POWER_GEN OR
						psBuilding->type == REF_RESOURCE_EXTRACTOR OR
						psBuilding->type == REF_RESEARCH OR
						psBuilding->type == REF_CYBORG_FACTORY OR
						psBuilding->type == REF_VTOL_FACTORY)
					{
						continue;
					}
				}

				if (psBuilding->type == REF_RESEARCH_MODULE)
				{
					//don't add to list if Research Facility not presently built
					if (!researchModule)
					{
						continue;
					}
				}
				else if (psBuilding->type == REF_FACTORY_MODULE)
				{
					//don't add to list if Factory not presently built
					if (!factoryModule)
					{
						continue;
					}
				}
				else if (psBuilding->type == REF_POWER_MODULE)
				{
					//don't add to list if Power Gen not presently built
					if (!powerModule)
					{
						continue;
					}
				}
                //paranoid check!!
                if (psBuilding->type == REF_FACTORY OR
					psBuilding->type == REF_CYBORG_FACTORY OR
					psBuilding->type == REF_VTOL_FACTORY)
                {
                    //NEVER EVER EVER WANT MORE THAN 5 FACTORIES
                    if (asStructLimits[selectedPlayer][inc].currentQuantity >= MAX_FACTORY)
                    {
                        continue;
                    }
                }
                //HARD_CODE don't ever want more than one Las Sat structure
                if (psBuilding->psWeapStat AND psBuilding->psWeapStat->
                    weaponSubClass == WSC_LAS_SAT AND getLasSatExists(selectedPlayer))
                {
                    continue;
                }
                //HARD_CODE don't ever want more than one Sat Uplink structure
                if (psBuilding->type == REF_SAT_UPLINK)
                {
                    if (asStructLimits[selectedPlayer][inc].currentQuantity >= 1)
                    {
                        continue;
                    }
                }

				DBP3(("fillStructureList: adding %s (%x)\n",
					psBuilding->pName, apStructTypeLists[selectedPlayer][inc]));
				ppList[count++] = psBuilding;
				//count++;
				//check haven't reached limit
				if (count == limit)
				{
					return count;
				}
			}
		}
	}
	return count;
}





/* checks that the location is a valid one to build on and sets the outline colour
x and y in tile-coords*/
BOOL validLocation(BASE_STATS *psStats, UDWORD x, UDWORD y, UDWORD player, 
                   BOOL bCheckBuildQueue)
{
	STRUCTURE			*psStruct;
	FEATURE				*psFeat;
	STRUCTURE_STATS		*psBuilding;
	BOOL				valid = TRUE;
	//UDWORD				i, j, min, max;
	SDWORD				i, j;
	UDWORD				min, max;
	HIGHLIGHT			site;
	FLAG_POSITION		*psCurrFlag;
	MAPTILE				*psTile;

	psBuilding = (STRUCTURE_STATS *)psStats;

	// initialise the buildsite structure
	// gets rid of the nasty bug when the GLOBAL buildSite was
	// used in here
    // Now now...we can quite easily hack this a bit more...
	if (psStats->ref >= REF_STRUCTURE_START &&
		psStats->ref < (REF_STRUCTURE_START + REF_RANGE))
	{
		site.xTL = (UWORD)x;
		site.yTL = (UWORD)y;
		site.xBR = (UWORD)(x + psBuilding->baseWidth - 1);
		site.yBR = (UWORD)(y + psBuilding->baseBreadth - 1);

		// increase the size of a repair facility
		if (psBuilding->type == REF_REPAIR_FACILITY)
		{
			site.xTL -= 1;
			site.yTL -= 1;
			site.xBR += 1;
			site.yBR += 1;
		}
        //if we're dragging the wall/defense we need to check along the current dragged size
        if (wallDrag.status != DRAG_INACTIVE)
        {

            if (psBuilding->type == REF_WALL OR psBuilding->type == REF_DEFENSE)

            {
				UWORD    dx,dy;

				wallDrag.x2 = mouseTileX;
				wallDrag.y2 = mouseTileY;

				dx = (UWORD)(abs(mouseTileX - wallDrag.x1));
				dy = (UWORD)(abs(mouseTileY - wallDrag.y1));

				if(dx >= dy) 
                {
                    //build in x direction
                    site.xTL = (UWORD)wallDrag.x1;
                    site.xBR = (UWORD)wallDrag.x2;
                    site.yTL = (UWORD)wallDrag.y1;
                    if (dy > 0)
                    {
                        site.yBR = (UWORD)(site.yTL+1);
                    }
                    else
                    {
                        site.yBR = (UWORD)(site.yTL);
                    }
                    if (site.xTL > site.xBR)
                    {
                        dx = site.xBR;
                        site.xBR = site.xTL;
                        site.xTL = dx;
                    }


				} 
                else if(dx < dy) 
                {
                    //build in y direction
                    site.yTL = (UWORD)wallDrag.y1;
                    site.yBR = (UWORD)wallDrag.y2;
                    site.xTL = (UWORD)wallDrag.x1;
                    if (dx > 0)
                    {
                        site.xBR = (UWORD)(site.xTL+1);
                    }
                    else
                    {
                        site.xBR = (UWORD)(site.xTL);
                    }
                    if (site.yTL > site.yBR)
                    {
                        dy = site.yBR;
                        site.yBR = site.yTL;
                        site.yTL = dy;
                    }


				}
            }
        }
	} 
    else 
    {
		// If the stat's not a structure then assume it's size is 1x1.
		site.xTL = (UWORD)x;
		site.yTL = (UWORD)y;
		site.xBR = (UWORD)x;
		site.yBR = (UWORD)y;
	}

// This is now handled below by checking against scroll limits. PD,17/01/99.
//	/* Hackety hack hack hack!! */
//	/* Can't build too near the edge - problem solved - at least for now */
//	if(x<=2 OR y<=2 OR x>=mapWidth-5 OR y>=mapHeight-5)
//	{
//#ifdef PSX
//		SetHilightColourNotOK();
//#else
//		outlineColour = outlineNotOK;
//		outlineColour3D = outlineNotOK3D;
//#endif
//		return(FALSE);
//	}

	for (i = site.xTL; i <= site.xBR AND valid; i++) {
		for (j = site.yTL; j <= site.yBR AND valid; j++) {

            //do this after the off-map check
/*#ifdef DEBUG
			if( gwZoneReachable(gwGetZone(i,j)) == FALSE)
			{
				// Can't ever drive there 
				valid = FALSE;
				goto failed;
			}
#endif*/
			
			// Can't build outside of scroll limits.
			if( ((SDWORD)i < scrollMinX+2) || ((SDWORD)i > scrollMaxX-5) ||
				((SDWORD)j < scrollMinY+2) || ((SDWORD)j > scrollMaxY-5)) {
				valid = FALSE;
				goto failed;
			}

			// check i or j off map.
			if(i<=2 OR j<=2 OR i>=(SDWORD)mapWidth-2 OR j>=(SDWORD)mapHeight-2)
			{
				valid = FALSE;
				goto failed;
			}

#ifdef NDEBUG
			/*God Awful HACK!! - AB 30/04/99 - gets round a problem with 
			UrbanDuel map where an oil derrick cannot be built - when the 
			map has been edited this hack can be removed*/
        	if (psBuilding->type != REF_RESOURCE_EXTRACTOR)
			{
				if( gwZoneReachable(gwGetZone(i,j)) == FALSE)
				{
					// Can't ever drive there 
					valid = FALSE;
					goto failed;
				}
			}
#endif

            //don't check tile is visible for placement of a delivery point
        	if (psStats->ref >= REF_STRUCTURE_START &&
		        psStats->ref < (REF_STRUCTURE_START + REF_RANGE))
            {

                //allow us to do so in debug mode!
                if (!getDebugMappingStatus() && !bMultiPlayer)

                {
    			    // Can't build where we haven't been yet.
	    		    if(!TEST_TILE_VISIBLE(player,mapTile(i,j))) {
					    valid = FALSE;
    					goto failed;
                    }
		    	}
		    }
        }
	}


	// cant place on top of a delivery point...
	for (psCurrFlag = apsFlagPosLists[selectedPlayer]; psCurrFlag; psCurrFlag = psCurrFlag->psNext)
	{
		// Need to check for whole building site, not just one tiles worth.	PD,17/01/99
		// How about a simple point in rect test - not a wacky loop test - John.
		// Hmm, you seem to have introduced a dirty smelly GOTO along the way too - Alex.
		// Anyone else care to comment?
		// It may be dirty and smelly, but it stops you going through about a hundred other
		// if's before the function quits, and if you care to check your K&R this is a perfectly valid
		// use of a goto - go on someone else pitch in !! - John.
		// K&R notwithstanding, aint no place for gotos in my code, hack-boy - Alex.
/*		for (i = site.xTL; i <= site.xBR AND valid; i++) {
			for (j = site.yTL; j <= site.yBR AND valid; j++) {
				if( ((psCurrFlag->coords.x)>>TILE_SHIFT == (SDWORD)i) &&
					((psCurrFlag->coords.y)>>TILE_SHIFT == (SDWORD)j) ) */
		i = psCurrFlag->coords.x >>TILE_SHIFT;
		j = psCurrFlag->coords.y >>TILE_SHIFT;
		if (i >= site.xTL && i <= site.xBR &&
			j >= site.yTL && j <= site.yBR)
		{
			valid = FALSE;
			goto failed;
		}
	}

	// can't build next to a repair facility
	for (psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_REPAIR_FACILITY)
		{
			// get the top left of the struct
			i = (psStruct->x >> TILE_SHIFT) - 1;
			j = (psStruct->y >> TILE_SHIFT) - 1;
			
			// see if the x extents overlap
			if ((site.xTL >= i && site.xTL <= (i+2)) ||
				(site.xBR >= i && site.xBR <= (i+2)))
			{
				// now see if y extents overlap
				if ((site.yTL >= j && site.yTL <= (j+2)) ||
					(site.yBR >= j && site.yBR <= (j+2)))
				{
					valid = FALSE;
					goto failed;
				}
			}
		}
	}

	if (psStats->ref >= REF_STRUCTURE_START &&
		psStats->ref < (REF_STRUCTURE_START + REF_RANGE))
	{
//		Now done at top of function.
//		psBuilding = (STRUCTURE_STATS *)psStats;
//
//		// initialise the buildsite structure
//		// gets rid of the nasty bug when the GLOBAL buildSite was
//		// used in here
//		site.xTL = (UWORD)x;
//		site.yTL = (UWORD)y;
//		site.xBR = (UWORD)(x + psBuilding->baseWidth - 1);
//		site.yBR = (UWORD)(y + psBuilding->baseBreadth - 1);

		switch(psBuilding->type)
		{
			case REF_HQ:
			case REF_FACTORY:
			case REF_LAB:
			case REF_RESEARCH:
			case REF_POWER_GEN:
			case REF_WALL:
			case REF_WALLCORNER:
			case REF_DEFENSE:
			case REF_REPAIR_FACILITY:
			case REF_COMMAND_CONTROL:
			case REF_CYBORG_FACTORY:
			case REF_VTOL_FACTORY:
			case REF_BLASTDOOR:
			case REF_REARM_PAD:
            case REF_MISSILE_SILO:
				/*need to check each tile the structure will sit on is not water*/
				for (i = site.xTL; i <= site.xBR AND valid; i++) 
				{
					for (j = site.yTL; j <= site.yBR AND valid; j++) 
					{
						psTile = mapTile(i,j);
						if ((TERRAIN_TYPE(psTile) == TER_WATER) ||
							(TERRAIN_TYPE(psTile) == TER_CLIFFFACE) )
						{
							valid = FALSE;
						}
					}
				}
				//don't bother checking if already found a problem
				if (valid)
				{
					//check not within landing zone
					for (i = site.xTL; i <= site.xBR AND valid; i++) 
					{
						for (j = site.yTL; j <= site.yBR AND valid; j++) 
						{
							if (withinLandingZone(i, j))
							{
								valid = FALSE;
							}
						}
					}
				}

				// special droid/max-min test for repair facility
				if ( valid && (psBuilding->type == REF_REPAIR_FACILITY))
				{
					getTileMaxMin(x, y, &max, &min);
					if ((max - min) > MAX_INCLINE)
					{
						valid = FALSE;
					}
					if (valid &&
						!noDroid(x,y))
					{
						valid = FALSE;
					}
				}
				
				if (valid &&	// only do if necessary
					(psBuilding->type != REF_REPAIR_FACILITY))
				{
					for (i = site.xTL; i <= site.xBR AND valid; i++) 
					{
						for (j = site.yTL; j <= site.yBR AND valid; j++) 
						{
// This really needs to check to see if the droid that's in the way is the droid that wants to build
// in which case it should'nt invalidate the location.
							if(noDroid(i,j) == FALSE)
							{
								valid = FALSE;
							}
					//		if(TERRAIN_TYPE(mapTile(i,j)) == TER_WATER) 
					//		{
					//			valid = FALSE;
					//		}
						}
					}
				}
				
                //walls/defensive structures can be built along any ground
				if (valid &&	// only do if necessary
					(!(psBuilding->type == REF_REPAIR_FACILITY OR
                       psBuilding->type == REF_DEFENSE OR 
                       psBuilding->type == REF_WALL)))
				{
					/*cannot build on ground that is too steep*/
					min = 0;
					max = 0;
					for (i = site.xTL; i <= site.xBR AND valid; i++) 
					{
						for (j = site.yTL; j <= site.yBR AND valid; j++) 
						{
							getTileMaxMin(i, j, &max, &min);
   							if ((max - min) > MAX_INCLINE)
    						{
	    						valid = FALSE;
		    				}
						}
					}
				}
				//don't bother checking if already found a problem
				if (valid)
				{



					//on PC - defence structures can be built next to anything now- AB 22/09/98
                    //and the Missile_Silo (special case) - AB 01/03/99
                    if (!(psBuilding->type == REF_DEFENSE OR

						psBuilding->type == REF_WALL OR
						psBuilding->type == REF_WALLCORNER OR
                        psBuilding->type == REF_MISSILE_SILO))

					{
						/*need to check there is one tile between buildings*/
						for (i = (UWORD)(site.xTL-1); i <= (UWORD)(site.xBR+1); i++) 
						{
							for (j = (UWORD)(site.yTL-1); j <= (UWORD)(site.yBR+1); j++) 
							{
								//skip the actual area the structure will cover
								if (i < site.xTL OR i > site.xBR OR
									j < site.yTL OR j > site.yBR)
								{
									if (TILE_HAS_STRUCTURE(mapTile(i,j)))
									{
//// On Playstation, to try and reduce the number of structures on screen.. defence
//// structures can only be build next to walls, otherwise there has to be a gap.
//#ifdef PSX
//										psStruct = getTileStructure(i,j);
//										if (psStruct)
//                                        {
//										    //defence structures can be built next to a wall, but nothing else.
//										    if (psBuilding->type == REF_DEFENSE)
//										    {   
//												if (!(psStruct->pStructureType->type == REF_WALL OR
//													psStruct->pStructureType->type == REF_WALLCORNER))
//												{
//													valid = FALSE;
//												}
//    										}
//                                            //and walls can be built next to corner walls and defensive structures
//	    									else if (psBuilding->type == REF_WALL)
//                                            {
//												if (!(psStruct->pStructureType->type == REF_DEFENSE OR
//													psStruct->pStructureType->type == REF_WALLCORNER))
//												{
//													valid = FALSE;
//												}
//                                            }
//                                            else
//		    								{
//			    								valid = FALSE;
//				    						}
//                                        }
//#else
										psStruct = getTileStructure(i,j);
										
										
										if (psStruct)
                                        {
                                            //you can build anything next to a defensive structure
                                            if ((psStruct->pStructureType->type != REF_DEFENSE) AND
												(psStruct->pStructureType->type != REF_WALL)	AND
												(psStruct->pStructureType->type != REF_WALLCORNER)
											   )
                                            {
                                                //Walls can be built next to walls and defenses - AB 03/03/99
	        									if (psBuilding->type == REF_WALL)
		        								{
    			    								if (!(psStruct->pStructureType->type == REF_WALL OR
													psStruct->pStructureType->type == REF_WALLCORNER))
												    {
													    valid = FALSE;
												    }   
											    }
                                                else
                                                {
    		    									valid = FALSE;
                                                }
											}


											else	// is a defense.
											{		// skirmish players don't build defensives next to each other.(route hack)
												if( bMultiPlayer && game.type == SKIRMISH && !isHumanPlayer(player) )
												{
													valid = FALSE;
												}
											}
											
										
                                        }

									
//#endif
									}
									//cannot build within one tile of a oil resource
									if(TILE_HAS_FEATURE(mapTile(i,j)))
									{
										psFeat = getTileFeature(i,j);
										if (psFeat AND psFeat->psStats->subType == 
											FEAT_OIL_RESOURCE)
										{
											valid = FALSE;
										}
									}
								}
							}
						}
					}

				}
				//don't bother checking if already found a problem
				if (valid)
				{
					/*need to check each tile the structure will sit on*/
					for (i = site.xTL; i <= site.xBR AND valid; i++) 
					{
						for (j = site.yTL; j <= site.yBR AND valid; j++) 
						{
							psTile = mapTile(i,j);
							if (TILE_OCCUPIED(psTile))
							{
								if ((psBuilding->type == REF_DEFENSE OR
                                     psBuilding->type == REF_WALL) &&
									(psTile->tileInfoBits & BITS_WALL))
								{
									psStruct = getTileStructure(i,j);
									if (psStruct != NULL &&
										psStruct->player != player)
									{
										valid = FALSE;
									}
								}
								else
								{
									valid = FALSE;
								}
							}
						}
					}
				}
				break;
			case REF_FACTORY_MODULE:
				valid = FALSE;
				if(TILE_HAS_STRUCTURE(mapTile(x,y)))
				{
					psStruct = getTileStructure(x,y);
					if(psStruct AND (
						psStruct->pStructureType->type == REF_FACTORY OR	
						psStruct->pStructureType->type == REF_VTOL_FACTORY) AND
                        psStruct->status == SS_BUILT)
					{
						valid = TRUE;
					}
				}
				break;
			case REF_RESEARCH_MODULE:
				valid = FALSE;
				//check that there is a research facility at the location
				if(TILE_HAS_STRUCTURE(mapTile(x,y)))
				{
					psStruct = getTileStructure(x,y);
					if(psStruct AND psStruct->pStructureType->type == REF_RESEARCH AND
                        psStruct->status == SS_BUILT)
					{
						valid = TRUE;
					}
				}

				break;
			case REF_POWER_MODULE:
				valid = FALSE;
				if(TILE_HAS_STRUCTURE(mapTile(x,y)))
				{
					psStruct = getTileStructure(x,y);
					if(psStruct AND psStruct->pStructureType->type == REF_POWER_GEN AND
                        psStruct->status == SS_BUILT)
					{
						valid = TRUE;
					}
				}
				break;
			case REF_RESOURCE_EXTRACTOR:
				valid = FALSE;
				//check that there is a oil resource at the location
				if(TILE_HAS_FEATURE(mapTile(x,y)))
				{
					psFeat = getTileFeature(x,y);
					if(psFeat AND psFeat->psStats->subType == FEAT_OIL_RESOURCE)
					{
						valid = TRUE;
					}
				}
				break;
		
			default:
				valid = TRUE;
				break;
		}
        //if setting up a build queue need to check against future sites as well - AB 4/5/99
        if (ctrlShiftDown() AND player == selectedPlayer AND bCheckBuildQueue)
        {
            DROID   *psDroid;
            SDWORD  order,left,right,up,down,size;
            BOOL    validCombi;

            //defense and missile silo's can be built next to anything so don't need to check
            if (!(psBuilding->type == REF_DEFENSE OR psBuilding->type == 
                REF_MISSILE_SILO))
            {
                for (psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
                {
                    //once its invalid stop checking
                    if (valid == FALSE)
                    {
                        break;
                    }
                    if (psDroid->droidType == DROID_CONSTRUCT OR
                        psDroid->droidType == DROID_CYBORG_CONSTRUCT)
                    {
                        //look thru' the list of orders to see if more building sites
                        for (order = 0; order < psDroid->listSize; order++)
                        {
                            if (psDroid->asOrderList[order].order == DORDER_BUILD)
                            {
                                validCombi = FALSE;
                                if (((STRUCTURE_STATS *)psDroid->asOrderList[order].
                                    psOrderTarget)->type == REF_DEFENSE OR 
                                    ((STRUCTURE_STATS *)psDroid->asOrderList[order].
                                    psOrderTarget)->type == REF_MISSILE_SILO)
                                {
                                    validCombi = TRUE;
                                }
                                //walls can be built next to walls and defence
                                if ((psBuilding->type == REF_WALL || psBuilding->type == REF_WALLCORNER)
				    && (((STRUCTURE_STATS *)psDroid->asOrderList[order].psOrderTarget)->type == REF_WALL
				    || ((STRUCTURE_STATS *)psDroid->asOrderList[order].psOrderTarget)->type == REF_WALLCORNER))
                                {
                                    validCombi = TRUE;
                                }
                                //don't bother checking if valid combination of building types
                                if (!validCombi)
                                {
						            /*need to check there is one tile between buildings*/
                                    //check if any corner is within the build site
                                    size = ((STRUCTURE_STATS *)psDroid->asOrderList[order].
                                        psOrderTarget)->baseWidth;
                                    left = ((psDroid->asOrderList[order].x) >> TILE_SHIFT) - size/2;
                                    right = left + size;
                                    size = ((STRUCTURE_STATS *)psDroid->asOrderList[order].
                                        psOrderTarget)->baseBreadth;
                                    up = ((psDroid->asOrderList[order].y) >> TILE_SHIFT) - size/2;
                                    down = up + size;
		                            // increase the size of a repair facility
		                            if (((STRUCTURE_STATS *)psDroid->asOrderList[
                                        order].psOrderTarget)->type == REF_REPAIR_FACILITY)
		                            {
			                            left -= 1;
			                            up -= 1;
			                            right += 1;
			                            down += 1;
		                            }
								    if (((left > site.xTL-1 AND left <= site.xBR+1) AND
                                        (up > site.yTL-1 AND up <= site.yBR+1)) OR
                                        ((right > site.xTL-1 AND right <= site.xBR+1) AND
                                        (up > site.yTL-1 AND up <= site.yBR+1)) OR
                                        ((left > site.xTL-1 AND left <= site.xBR+1) AND
                                        (down > site.yTL-1 AND down <= site.yBR+1)) OR
                                        ((right > site.xTL-1 AND right <= site.xBR+1) AND
                                        (down > site.yTL-1 AND down <= site.yBR+1)))
                                    {
                                        valid = FALSE;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
	}
	else
	{
		// not positioning a structure
		valid = TRUE;
		if (fpathGroundBlockingTile(x,y))
		{
			valid = FALSE;
		}
	}

failed:
	if (!valid)
	{
		// Only set the hilight colour if it's the selected player.
		if(player == selectedPlayer) {

			outlineColour = outlineNotOK;
			outlineColour3D = outlineNotOK3D;

		}

		return FALSE;
	}

	// Only set the hilight colour if it's the selected player.
	if(player == selectedPlayer) {

		outlineColour = outlineOK;
		outlineColour3D = outlineOK3D;

	}

	return TRUE;
}

/* 
for a new structure, find a location along an edge which the droid can get
to and return this as the destination for the droid.
*/
//BOOL getDroidDestination(STRUCTURE_STATS *psBuildingStats, UDWORD structX, 
BOOL getDroidDestination(BASE_STATS *psStats, UDWORD structX, 
	UDWORD structY, UDWORD *pDroidX, UDWORD *pDroidY)
{
	UWORD				start;
	UDWORD				structTileX, structTileY, width = 0, breadth = 0;

/*	UDWORD	numIterations;
	UDWORD	desiredX,desiredY;
	BOOL	gotTarget;
	UDWORD	xMod,yMod;
	SDWORD	xVar,yVar;*/


	if (StatIsStructure(psStats))
	{
		width = ((STRUCTURE_STATS *)psStats)->baseWidth;
		breadth = ((STRUCTURE_STATS *)psStats)->baseBreadth;
	}
	else if (StatIsFeature(psStats))
	{
		width = ((FEATURE_STATS *)psStats)->baseWidth;
		breadth = ((FEATURE_STATS *)psStats)->baseBreadth;
	}
// -----------------------------------------------------------
	/*
	flushConsoleMessages();

	numIterations = max(width,breadth);
	xMod = (width);
	yMod = (breadth);

	gotTarget = FALSE;
	while(!gotTarget)
	{
   	   	xVar = xMod - rand()%((xMod*2)+1);
		yVar = yMod - rand()%((yMod*2)+1);
		desiredX = (structX>>TILE_SHIFT) + xVar;
		desiredY = (structY>>TILE_SHIFT) + yVar;
		if(desiredX <=0) desiredX = 1;
		if(desiredY <=0) desiredY = 1;
		if(desiredX >= mapWidth-1) desiredX = mapWidth-1;
		if(desiredY >= mapHeight-1) desiredY = mapHeight-1;



   		DBCONPRINTF(ConsoleString,(ConsoleString,"Desired X : %d",desiredX));
		DBCONPRINTF(ConsoleString,(ConsoleString,"Desired Y : %d",desiredY));
		gotTarget = unfussyPickATile(&desiredX,&desiredY,numIterations);
		numIterations++;
	}
	
	*pDroidX = desiredX<<TILE_SHIFT;
	*pDroidY = desiredY<<TILE_SHIFT;
	DBCONPRINTF(ConsoleString,(ConsoleString,"Given X : %d",desiredX));
	DBCONPRINTF(ConsoleString,(ConsoleString,"Given Y : %d",desiredY));

	return(TRUE);
	*/

// -----------------------------------------------------------

	//get a random starting place 0=top left
	//start = (UWORD)(rand() % ((psBuildingStats->baseWidth + psBuildingStats->
	//	baseBreadth) * 2));
	start = (UWORD)(rand() % ((width + breadth) * 2));

	//search in a clockwise direction around the structure from the starting point
	//if (start == 0 OR start < psBuildingStats->baseWidth)
	if (start == 0 OR start < width)
	{
		//top side first
		structTileX = structX >> TILE_SHIFT;
		structTileY = (structY >> TILE_SHIFT) - 1;
		//if (checkWidth(psBuildingStats->baseWidth, structTileX, structTileY, 
		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return TRUE;
		}
		//structTileX += psBuildingStats->baseWidth;
		structTileX += width;
		structTileY += 1;
		//if (checkLength(psBuildingStats->baseBreadth, structTileX, structTileY,
		if (checkLength(breadth, structTileX, structTileY,pDroidX, pDroidY))
		{
			return TRUE;
		}
		structTileX = structX >> TILE_SHIFT;
		//structTileY += psBuildingStats->baseBreadth;
		structTileY += breadth;
		//if (checkWidth(psBuildingStats->baseWidth, structTileX, structTileY, 
		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return TRUE;
		}
		structTileX -= 1;
		structTileY = structY >> TILE_SHIFT;
		//if (checkLength(psBuildingStats->baseBreadth, structTileX, structTileY, 
		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return TRUE;
		}
	}
	//else if (start == psBuildingStats->baseWidth OR start < (psBuildingStats->
	//	baseWidth + psBuildingStats->baseBreadth))
	else if (start == width OR start < (width + breadth))
	{
		//right side first
		//structTileX = (structX >> TILE_SHIFT) + psBuildingStats->baseWidth;
		structTileX = (structX >> TILE_SHIFT) + width;
		structTileY = structY >> TILE_SHIFT;
		//if (checkLength(psBuildingStats->baseBreadth, structTileX, structTileY,
		if (checkLength(breadth, structTileX, structTileY,pDroidX, pDroidY))
		{
			return TRUE;
		}
		structTileX = structX >> TILE_SHIFT;
		//structTileY += psBuildingStats->baseBreadth;
		structTileY += breadth;
		//if (checkWidth(psBuildingStats->baseWidth, structTileX, structTileY, 
		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return TRUE;
		}
		structTileX -= 1;
		structTileY = structY >> TILE_SHIFT;
		//if (checkLength(psBuildingStats->baseBreadth, structTileX, structTileY, 
		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return TRUE;
		}
		structTileX += 1;
		structTileY -= 1;
		//if (checkWidth(psBuildingStats->baseWidth, structTileX, structTileY, 
		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return TRUE;
		}
	}
	//else if (start == (psBuildingStats->baseWidth + psBuildingStats->
	//	baseBreadth) OR start < (psBuildingStats->baseWidth * 
	//	psBuildingStats->baseBreadth))
	else if (start == (width + breadth) OR start < (width * breadth))
	{
		//bottom first
		structTileX = structX >> TILE_SHIFT;
		//structTileY = (structY >> TILE_SHIFT) + psBuildingStats->baseBreadth;
		structTileY = (structY >> TILE_SHIFT) + breadth;
		//if (checkWidth(psBuildingStats->baseWidth, structTileX, structTileY, 
		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return TRUE;
		}
		structTileX -= 1;
		structTileY = structY >> TILE_SHIFT;
		//if (checkLength(psBuildingStats->baseBreadth, structTileX, structTileY, 
		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return TRUE;
		}
		structTileX += 1;
		structTileY -= 1;
		//if (checkWidth(psBuildingStats->baseWidth, structTileX, structTileY, 
		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return TRUE;
		}
		//structTileX += psBuildingStats->baseWidth;
		structTileX += width;
		structTileY += 1;
		//if (checkLength(psBuildingStats->baseBreadth, structTileX, structTileY,
		if (checkLength(breadth, structTileX, structTileY,pDroidX, pDroidY))
		{
			return TRUE;
		}
	}
	else
	{
		//left side first
		structTileX = (structX >> TILE_SHIFT) - 1;
		structTileY = structY >> TILE_SHIFT;
		//if (checkLength(psBuildingStats->baseBreadth, structTileX, structTileY, 
		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return TRUE;
		}
		structTileX += 1;
		structTileY -= 1;
		//if (checkWidth(psBuildingStats->baseWidth, structTileX, structTileY, 
		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return TRUE;
		}
		//structTileX += psBuildingStats->baseWidth;
		structTileX += width;
		structTileY += 1;
		//if (checkLength(psBuildingStats->baseBreadth, structTileX, structTileY,
		if (checkLength(breadth, structTileX, structTileY,pDroidX, pDroidY))
		{
			return TRUE;
		}
		structTileX = structX >> TILE_SHIFT;
		//structTileY += psBuildingStats->baseBreadth;
		structTileY += breadth;
		//if (checkWidth(psBuildingStats->baseWidth, structTileX, structTileY, 
		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return TRUE;
		}
	}

	//not found a valid location so return FALSE
	return FALSE;
}

/* check along the width of a structure for an empty space */
BOOL checkWidth(UDWORD maxRange, UDWORD x, UDWORD y, UDWORD *pDroidX, UDWORD *pDroidY)
{
	UDWORD		side;
	
	for (side = 0; side < maxRange; side++)
	{
//psor		if (mapTile(x + side, y)->psObject == NULL)
		if(x+side<mapWidth AND y<mapHeight)
		{
			if(!TILE_OCCUPIED(mapTile(x+side,y)) )
			{
				*pDroidX = (x + side) << TILE_SHIFT;
				*pDroidY = y << TILE_SHIFT;
			
				ASSERT((worldOnMap(*pDroidX,*pDroidY),"checkWidth : Insane droid position"));

				return TRUE;
			}
		}
	}

	return FALSE;
}

/* check along the length of a structure for an empty space */
BOOL checkLength(UDWORD maxRange, UDWORD x, UDWORD y, UDWORD *pDroidX, UDWORD *pDroidY)
{
	UDWORD		side;
	
	for (side = 0; side < maxRange; side++)
	{
//psor		if (mapTile(x, y + side)->psObject == NULL)
		if(y+side<mapHeight AND x<mapWidth)
		{
			if(!TILE_OCCUPIED(mapTile(x,y+side)) )
			{
				*pDroidX = x << TILE_SHIFT;
				*pDroidY = (y + side) << TILE_SHIFT;

				ASSERT((worldOnMap(*pDroidX,*pDroidY),"checkHeight : Insane droid position"));

				return TRUE;
			}
		}
	}

	return FALSE;
}

//remove a structure from the map
void removeStructFromMap(STRUCTURE *psStruct)
{
	UDWORD		i,j;
	UDWORD		mapX, mapY;
	MAPTILE		*psTile;

	/* set tiles drawing */
	mapX = (psStruct->x - psStruct->pStructureType->baseWidth * TILE_UNITS / 2) >> TILE_SHIFT;
	mapY = (psStruct->y - psStruct->pStructureType->baseBreadth * TILE_UNITS / 2) >> TILE_SHIFT;
	for (i = 0; i < psStruct->pStructureType->baseWidth; i++)
	{
		for (j = 0; j < psStruct->pStructureType->baseBreadth; j++)
		{
			psTile = mapTile(mapX+i, mapY+j);
			SET_TILE_EMPTY(psTile);
			CLEAR_TILE_NODRAW(psTile);
			CLEAR_TILE_TALLSTRUCTURE(psTile);
			CLEAR_TILE_NOTBLOCKING(psTile);
			CLEAR_TILE_SMALLSTRUCTURE(psTile);
		}
	}
}

// remove a structure from a game without any visible effects
// bDestroy = TRUE if the object is to be destroyed
// (for example used to change the type of wall at a location)
BOOL removeStruct(STRUCTURE *psDel, BOOL bDestroy)
{
	//UDWORD		i,j;
	BOOL		resourceFound = FALSE;
	UBYTE		mask;
	FACTORY		*psFactory;
	SDWORD		cluster;
	//UDWORD		mapX, mapY;
	FLAG_POSITION	*psAssemblyPoint=NULL;

	ASSERT( (PTRVALID(psDel, sizeof(STRUCTURE)),
		"destroyStruct: invalid structure pointer\n") );


    	
    if (bDestroy)
    {
	    removeStructFromMap(psDel);
    }

	//tell the power system its gone
	powerDestroyObject((BASE_OBJECT *)psDel);

    if (bDestroy)
    {
	    //if the structure is a resource extractor, need to put the resource back in the map 
	    /*ONLY IF ANY POWER LEFT - HACK HACK HACK!!!! OIL POOLS NEED TO KNOW 
	    HOW MUCH IS THERE AND NOT RES EXTRACTORS */
	    if (psDel->pStructureType->type == REF_RESOURCE_EXTRACTOR)
	    {
		    if (((RES_EXTRACTOR *)psDel->pFunctionality)->power)
		    {
			    //clear the tile nodraw attribute so that will get set up when building the feature
			    CLEAR_TILE_NODRAW(mapTile(psDel->x >> TILE_SHIFT,psDel->y >> TILE_SHIFT));
			    buildFeature(&asFeatureStats[oilResFeature], psDel->x, psDel->y, FALSE);
			    resourceFound = TRUE;
		    }
    //		else
    //		{
    //			addConsoleMessage("removeStruct - NO OIL LEFT IN POOL", 
    //				DEFAULT_JUSTIFY);
    //		}
        }
    }

    if (psDel->pStructureType->type == REF_RESOURCE_EXTRACTOR)
	{
	    //tell associated Power Gen
	    releaseResExtractor(psDel);
    }

	//Player only gets the power back if demolish structure themselves!******
	//put back the power required to maintain this structure (=power to build)
	//returnPower(psDel->player, psDel->pStructureType->powerToBuild);

	/*if (psDel->pStructureType->type == REF_POWER_GEN OR psDel->
		pStructureType->type == REF_RESOURCE_EXTRACTOR OR
		psDel->pStructureType->type == REF_HQ)
	{
		resetPlayerPower(psDel->player, psDel);
	}*/
	if (psDel->pStructureType->type == REF_POWER_GEN)
	{
		//tell associated Res Extractors
		releasePowerGen(psDel);
	}

    //the droid check for this each frame - consistent with the rest of the game
    /*if (psDel->pStructureType->type == REF_REARM_PAD)
    {
        //tell associated VTOL droids
        releaseVTOLPad(psDel);
    }*/

    //check for a research topic currently under way
    if (psDel->pStructureType->type == REF_RESEARCH)
    {
        if (((RESEARCH_FACILITY *)psDel->pFunctionality)->psSubject)
        {
            //cancel the topic
            cancelResearch(psDel);
        }
    }

	//subtract one from the structLimits list so can build another - don't allow to go less than zero!
    if (asStructLimits[psDel->player][psDel->pStructureType - asStructureStats].
        currentQuantity)
    {
	    asStructLimits[psDel->player][psDel->pStructureType - asStructureStats].
            currentQuantity--;
    }

	//if it is a factory - need to reset the factoryNumFlag
	if (StructIsFactory(psDel))
	{
		psFactory = ((FACTORY*)psDel->pFunctionality) ;
        //not used anymore - 11/02/99
		/*if ( psFactory->psGroup != NULL )
		{
			grpLeave(psFactory->psGroup, NULL);
		}
		psFactory->psGroup = NULL;*/

		//need to initialise the production run as well
		cancelProduction(psDel);

		psAssemblyPoint = psFactory->psAssemblyPoint;
	}
	else if (psDel->pStructureType->type == REF_REPAIR_FACILITY)
	{
		psAssemblyPoint = ((REPAIR_FACILITY*)psDel->pFunctionality)->psDeliveryPoint;
	}

	if (psAssemblyPoint != NULL)
	{
		mask = (UBYTE)(1 << psAssemblyPoint->factoryInc);
		factoryNumFlag[psDel->player][psAssemblyPoint->factoryType] ^= mask;

        //need to cancel the repositioning of the DP if selectedPlayer and currently moving
        if (psDel->player == selectedPlayer)
        {
            //if currently trying to place a DP
            if (tryingToGetLocation())
            {
                //need to check if this factory's DP is trying to be re-positioned
                if (psAssemblyPoint == sBuildDetails.UserData)
                {
                    kill3DBuilding();
                }
            }
        }
	}

	// remove the structure from the grid
	gridRemoveObject((BASE_OBJECT *)psDel);

	// remove the structure from the cluster
	cluster = psDel->cluster;
	clustRemoveObject((BASE_OBJECT *)psDel);

    if (bDestroy)
    {
	    killStruct(psDel);
    }

	/* remove animation if present */
	if ( psDel->psCurAnim != NULL )
	{
		animObj_Remove( &psDel->psCurAnim, psDel->psCurAnim->psAnim->uwID );
		psDel->psCurAnim = NULL;
	}

	clustUpdateCluster((BASE_OBJECT *)apsStructLists[psDel->player], cluster);

	if(psDel->player==selectedPlayer)
	{
		intRefreshScreen();
	}

	return resourceFound;
}

 /* Remove a structure and free it's memory */
BOOL destroyStruct(STRUCTURE *psDel)
{
	UDWORD			mapX, mapY, width,breadth;
	UDWORD			i;
	UDWORD			widthScatter,breadthScatter,heightScatter;
	iVector			pos;
	BOOL			resourceFound = FALSE;
	MAPTILE			*psTile;
	BOOL			bMinor;

	bMinor = FALSE;
	ASSERT( (PTRVALID(psDel, sizeof(STRUCTURE)),
		"destroyStruct: invalid structure pointer\n") );




	if (bMultiPlayer)
	{
//		if(!myResponsibility(psDel->player) )
//			&& (psDel->body != 0)) // can blow up utterly shagged structs.
//		{
//			return FALSE; // cant blow this up!
//		}
		SendDestroyStructure(psDel);
	}


		if(psDel->pStructureType->type == REF_HQ)
		{
			
		}

//---------------------------------------
 	/* Only add if visible */
	if(psDel->visible[selectedPlayer])
	{
		/* Firstly, are we dealing with a wall section */
		if(psDel->pStructureType->type == REF_WALL OR psDel->pStructureType->type == REF_WALLCORNER)
		{
			bMinor = TRUE;
		}

//---------------------------------------  Do we add immediate explosions?
		/* Set off some explosions, but not for walls */
		/* First Explosions */
		widthScatter = TILE_UNITS;
		breadthScatter = TILE_UNITS;
		heightScatter = TILE_UNITS;
		for(i=0; i<(UDWORD)(bMinor ? 2 : 4); i++)	// only add two for walls - gets crazy otherwise
		{
			pos.x = psDel->x + widthScatter - rand()%(2*widthScatter);
			pos.z = psDel->y + breadthScatter - rand()%(2*breadthScatter);
			pos.y = psDel->z + 32 + rand()%heightScatter;
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_MEDIUM,FALSE,NULL,0);
		}			

		/* Get coordinates for everybody! */
		pos.x = psDel->x;
		pos.z = psDel->y;
		pos.y = map_Height((UWORD)pos.x,(UWORD)pos.z);

//--------------------------------------- Do we add a fire?
		// Set off a fire, provide dimensions for the fire
		if(bMinor)
		{
	   	 	effectGiveAuxVar( (psDel->pStructureType->baseWidth<<TILE_SHIFT)/4 );
		}
		else
		{
			effectGiveAuxVar( (psDel->pStructureType->baseWidth<<TILE_SHIFT)/3 );
		}
		if(bMinor)							 // walls
		{
			/* Give a duration */
			effectGiveAuxVarSec(1000);
			/* Normal fire - no smoke */
			addEffect(&pos,EFFECT_FIRE,FIRE_TYPE_LOCALISED,FALSE,NULL,0);

		}
		else if(psDel->pStructureType->type == REF_RESOURCE_EXTRACTOR) // oil resources
		{
			/* Oil resources burn AND puff out smoke AND for longer*/
			effectGiveAuxVarSec(60000);
			addEffect(&pos,EFFECT_FIRE,FIRE_TYPE_SMOKY,FALSE,NULL,0);
		}
		else	// everything else
		{
			/* Give a duration */
			effectGiveAuxVarSec(10000);
			addEffect(&pos,EFFECT_FIRE,FIRE_TYPE_LOCALISED,FALSE,NULL,0);
		}

//--------------------------------------- Do we add a destruction seq, and if so, which?
		/* Power stations have their own desctruction sequence */
		if(psDel->pStructureType->type == REF_POWER_GEN)
		{
			addEffect(&pos,EFFECT_DESTRUCTION,DESTRUCTION_TYPE_POWER_STATION,FALSE,NULL,0);
		   //	addEffect(&pos,EFFECT_SAT_LASER,SAT_LASER_STANDARD,FALSE,NULL,0);
			pos.y += SHOCK_WAVE_HEIGHT;
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SHOCKWAVE,FALSE,NULL,0);
			// give some power back to the player.
			//addPower(psDel->player,psDel->pStructureType->powerToBuild);
			addPower(psDel->player, structPowerToBuild(psDel));
            //if it had a module attached, need to add the power for the base struct as well
            if (((POWER_GEN *)psDel->pFunctionality)->capacity)
            {
                addPower(psDel->player, psDel->pStructureType->powerToBuild);
            }

		}
		/* As do wall sections */
		else if(bMinor)
		{
			addEffect(&pos,EFFECT_DESTRUCTION,DESTRUCTION_TYPE_WALL_SECTION,FALSE,NULL,0);
		}
		else // and everything else goes here.....
		{
			addEffect(&pos,EFFECT_DESTRUCTION,DESTRUCTION_TYPE_STRUCTURE,FALSE,NULL,0);
		}

//		addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SKYSCRAPER,TRUE,psDel->sDisplay.imd,0);

//--------------------------------------- Start an earthquake...!
		/* shake the screen if we're near enough */
		if(clipXY(pos.x,pos.z))
		{
			shakeStart();	//attemptScreenShake();
		}

//--------------------------------------- And finally, add a boom sound!!!! 
		/* and add a sound effect */
		audio_PlayStaticTrack( psDel->x, psDel->y, ID_SOUND_EXPLOSION );
	}
//---------------------------------------------------------------------------------------


	resourceFound = removeStruct(psDel, TRUE);

	//once a struct is destroyed - it leaves a wrecked struct FEATURE in its place
	// Wall's don't leave wrecked features
	if(psDel->visible[selectedPlayer])
	{
		if (!resourceFound AND !(psDel->pStructureType->type == REF_WALL) AND 
			!(psDel->pStructureType->type == REF_WALLCORNER))
		{
			mapX = (psDel->x - psDel->pStructureType->baseWidth * TILE_UNITS / 2) >> TILE_SHIFT;
			mapY = (psDel->y - psDel->pStructureType->baseBreadth * TILE_UNITS / 2) >> TILE_SHIFT;
			for (width = 0; width < psDel->pStructureType->baseWidth; width++)
			{
				for (breadth = 0; breadth < psDel->pStructureType->baseBreadth; breadth++)
				{
					psTile = mapTile(mapX+width,mapY+breadth);
					if(TEST_TILE_VISIBLE(selectedPlayer,psTile))
					{
						psTile->illumination /= 2;
						if(psTile->bMaxed AND psTile->level!=UBYTE_MAX) //only do one's already seen
						{
							psTile->level/=2;
						}
					   //	if(!TILE_OCCUPIED(psTile))
					   //	{
					   //	buildFeature((asFeatureStats + structFeature),
					   //			(mapX+width) << TILE_SHIFT, (mapY+breadth) << TILE_SHIFT, FALSE);
					   //	}
					}
				}
			}
		 //	if(mapTile(mapX+1,mapY+1)->illumination>64)
			{
		   //		mapTile(mapX+1,mapY+1)->illumination/=4;
			}
		}
	}


	/* remove animation if present */
	if ( psDel->psCurAnim != NULL )
	{
		animObj_Remove( &psDel->psCurAnim, psDel->psCurAnim->psAnim->uwID );
		psDel->psCurAnim = NULL;
	}

	// updates score stats only if not wall
	if(psDel->pStructureType->type != REF_WALL AND psDel->pStructureType->type != REF_WALLCORNER)
	{
		if(psDel->player == selectedPlayer)
		{
			scoreUpdateVar(WD_STR_LOST);
		}
		else
		{
			scoreUpdateVar(WD_STR_KILLED);
		}
	}

	return TRUE;
}

/* For now all this does is work out what height the terrain needs to be set to
   An actual foundation structure may end up being placed down
   The x and y passed in are the CENTRE of the structure*/
SWORD buildFoundation(STRUCTURE_STATS *psStructStats, UDWORD x, UDWORD y)
{
	UDWORD	width, breadth;//, foundationMin, foundationMax;
	//UDWORD	max, min, startX, startY;
	UDWORD	startX, startY;
	SWORD	height,foundationMin, foundationMax;

	startX = (x >> TILE_SHIFT) - psStructStats->baseWidth/2;
	startY = (y >> TILE_SHIFT) - psStructStats->baseBreadth/2;

	//check the terrain is the correct type return -1 if not

	//shouldn't need to do this but doesn't take long hey?!
	/*check if there is a structure next to the new one - return the height of the
	  structure if found*/
	for (breadth = 0; breadth <= psStructStats->baseBreadth; breadth++)
	{
		for (width = 0; width <= psStructStats->baseWidth; width++)
		{
//psor			if (mapTile(startX+width, startY+breadth)->psObject && mapTile(startX+width, 
//psor				startY+breadth)->psObject->type == OBJ_STRUCTURE)
			if(TILE_HAS_STRUCTURE(mapTile(startX+width,startY+breadth)))
			{
	 			return((SWORD)map_TileHeight(startX+width,startY+breadth));
//				return ((SWORD)mapTile(startX+width, startY+breadth)->height);
			}
		}
	}

	//may also have to check that overlapping terrain can be set to the average height
	//eg water - don't want it to 'flow' into the structure if this effect is coded!

	startX = (x >> TILE_SHIFT) - psStructStats->baseWidth/2;
	startY = (y >> TILE_SHIFT) - psStructStats->baseBreadth/2;

	//initialise the starting values so they get set in loop
	foundationMin = TILE_MAX_HEIGHT;
	foundationMax = TILE_MIN_HEIGHT;

	for (breadth = 0; breadth <= psStructStats->baseBreadth; breadth++)
	{
		for (width = 0; width <= psStructStats->baseWidth; width++)
		{
			/*getTileMaxMin(startX + width, startY + breadth, &max, &min);
			if (foundationMin > min)
			{
				foundationMin = min;
			}
			if (foundationMax < max)
			{
				foundationMax = max;
			}*/
			height = map_TileHeight(startX+width,startY+breadth);
			if (foundationMin > height)
			{
				foundationMin = height;
			}
			if (foundationMax < height)
			{
				foundationMax = height;
			}
		}
	}
	//return the average of max/min height
	return ((SWORD)((foundationMin + foundationMax) / 2));
}

/* gets a structure stat from its name - relies on the name being unique (or it will 
   return the first one it finds!! */
#ifdef HASH_NAMES
SDWORD getStructStatFromName(STRING *pName)
{
	UDWORD				inc;
	STRUCTURE_STATS		*psStat;
	UDWORD	HashValue;

	HashValue=HashString(pName);


	for (inc = 0; inc < numStructureStats; inc++)
	{
		psStat = &asStructureStats[inc];
		if (psStat->NameHash==HashValue)
		{
			return inc;
		}
	}
	return -1;
}

#else
SDWORD getStructStatFromName(STRING *pName)
{
	UDWORD				inc;
	STRUCTURE_STATS		*psStat;

#ifdef RESOURCE_NAMES
	
	if (!getResourceName(pName))
	{
		return -1;
	}

#endif
	for (inc = 0; inc < numStructureStats; inc++)
	{
		psStat = &asStructureStats[inc];
		if (!strcmp(psStat->pName, pName))
		{
			return inc;
		}
	}
	return -1;
}
#endif

/*check to see if the structure is 'doing' anything  - return TRUE if idle*/
BOOL  structureIdle(STRUCTURE *psBuilding)
{
	BASE_STATS		*pSubject = NULL;

	if (psBuilding->pFunctionality != NULL)
	{
		//determine the Subject
		switch (psBuilding->pStructureType->type)
		{
			case REF_RESEARCH:
			{
				pSubject = ((RESEARCH_FACILITY*)psBuilding->pFunctionality)->psSubject;
				break;
			}
			case REF_FACTORY:
			case REF_CYBORG_FACTORY:
			case REF_VTOL_FACTORY:
			{
				pSubject = ((FACTORY*)psBuilding->pFunctionality)->psSubject;
				break;
			}
		}

		if (pSubject != NULL)
		{
			return FALSE;
		}
	}
	return TRUE;
}

/*checks to see if any structure exists of a specified type with a specified status */
BOOL checkStructureStatus( STRUCTURE_STATS *psStats, UDWORD player, UDWORD status)
{
	STRUCTURE	*psStructure;
	BOOL		found = FALSE;

	for (psStructure = apsStructLists[player]; psStructure != NULL; 
		psStructure = psStructure->psNext)
	{
		if (psStructure->pStructureType->type == psStats->type)
		{
			//need to check if THIS instance of the type has the correct status
			if (psStructure->status == status)
			{
				found = TRUE;
				break;
			}
		}
	}
	return found;
}

/*checks to see if a specific structure type exists -as opposed to a structure 
stat type*/
BOOL checkSpecificStructExists(UDWORD structInc, UDWORD player)
{
	STRUCTURE	*psStructure;
	BOOL		found = FALSE;

	ASSERT((structInc < numStructureStats, 
		"checkSpecificStructExists: invalid structure inc"));

	for (psStructure = apsStructLists[player]; psStructure != NULL; 
		psStructure = psStructure->psNext)
	{
		if (psStructure->status == SS_BUILT)
		{
			if ((psStructure->pStructureType->ref - REF_STRUCTURE_START) == 
				structInc)
			{
				found = TRUE;
				break;
			}
		}
	}
	return found;
}

//This function not used
/* return true if within range of a building */
/*BOOL validAssemblyPoint(UDWORD x, UDWORD y,UDWORD player,FLAG_POSITION *psCurr,BOOL bNew)
{
	return TRUE;
}*/
/*
	STRUCTURE		*psStruct;
	DWORD			xd,yd;
	FLAG_POSITION	*psFlag;
	
	// valid if within x squares of another ass. point, or x squares within factory.
	// close to the factory....
	for(psStruct = apsStructLists[player];psStruct; psStruct=psStruct->psNext)
	{
		if ((psStruct->pStructureType->type == REF_FACTORY OR
			psStruct->pStructureType->type == REF_CYBORG_FACTORY OR
			psStruct->pStructureType->type == REF_VTOL_FACTORY )
			&& ((FACTORY *)psStruct->pFunctionality)->psAssemblyPoint->factoryInc == psCurr->factoryInc)
		{
			xd = (psStruct->x - x);
			yd = (psStruct->y - y);
			if( ((xd*xd)+(yd*yd)) < (DWORD)(ASSEMBLY_RANGE*ASSEMBLY_RANGE))
			{
				return TRUE;
			}
		}
	}

	// check the previous point.
	if(bNew)
	{
		psFlag  = psCurr;
	}
	else
	{
		if(psCurr->factorySub == 0 )		// don't check first point.
		{
			return FALSE;
		}
		for(psFlag = apsFlagPosLists[player];
		psFlag->factorySub != psCurr->factorySub-1;
		psFlag= psFlag->psNext);
	}

	//check it.
	xd = (psFlag->coords.x - x);
	yd = (psFlag->coords.y - y);
	if( ((xd*xd)+(yd*yd)) < (DWORD)(ASSEMBLY_RANGE*ASSEMBLY_RANGE))
	{
		return TRUE;
	}

	return FALSE;
#endif
*/

/*finds a suitable position for the assembly point based on one passed in*/
void findAssemblyPointPosition(UDWORD *pX, UDWORD *pY, UDWORD player)
{
	//check if valid location
	/*if (TILE_OCCUPIED(mapTile(*pX,*pY)))
	{
		if (!pickATileGen(pX, pY, LOOK_FOR_EMPTY_TILE,normalPAT))
		{
			ASSERT((FALSE, "findAssemblyPointPosition: Unable to find a free location"));
		}
	}
	else
	{
		//check its not blocking or anything like that
		if (!sensiblePlace(*pX,*pY))
		{
			if (!pickATileGen(pX, pY, LOOK_FOR_EMPTY_TILE,normalPAT))
			{
				ASSERT((FALSE, "findAssemblyPointPosition: Unable to find a free location"));
			}
		}
	}*/

    //set up a dummy stat pointer
    STRUCTURE_STATS     sStats; 
    UDWORD              passes = 0;
    SDWORD	            i,j,startX,endX,startY,endY;

    sStats.ref = 0;
    sStats.baseWidth = 1;
    sStats.baseBreadth = 1;

	/* Initial box dimensions and set iteration count to zero */
	startX = endX = *pX;	startY = endY = *pY;	
    passes = 0;

    //if the value passed in is not a valid location - find one!
    if (!validLocation((BASE_STATS *)&sStats, *pX, *pY, player, FALSE))
    {
	    /* Keep going until we get a tile or we exceed distance */
	    while(passes<LOOK_FOR_EMPTY_TILE)
	    {
		    /* Process whole box */
		    for(i = startX; i <= endX; i++)
		    {
			    for(j = startY; j<= endY; j++)
			    {
				    /* Test only perimeter as internal tested previous iteration */
				    if(i==startX OR i==endX OR j==startY OR j==endY)
				    {
					    /* Good enough? */
					    if(validLocation((BASE_STATS *)&sStats, i, j, player, FALSE))
					    {
						    /* Set exit conditions and get out NOW */
						    *pX = i;	
                            *pY = j;
                            //jump out of the loop
                            return;
					    }
				    }
			    }
		    }
		    /* Expand the box out in all directions - off map handled by validLocation() */
		    startX--; startY--;	endX++;	endY++;	passes++;
	    }
    }
    else
    {
        //the first location was valid
        return;
    }
	/* If we got this far, then we failed - passed in values will be unchanged */
    ASSERT((FALSE, "findAssemblyPointPosition: unable to find a valid location!"));
}

/*sets the point new droids go to - x/y in world coords for a Factory
bCheck is set to TRUE for initial placement of the Assembly Point*/
void setAssemblyPoint(FLAG_POSITION *psAssemblyPoint, UDWORD x, UDWORD y, 
                      UDWORD player, BOOL bCheck)
{
	ASSERT((PTRVALID(psAssemblyPoint, sizeof(FLAG_POSITION)),
		"setAssemblyPoint: invalid AssemblyPoint pointer"));
	
	//check its valid
	x = x >> TILE_SHIFT;
	y = y >> TILE_SHIFT;
    if (bCheck)
    {
	    findAssemblyPointPosition(&x, &y, player);
    }
	//add half a tile so the centre is in the middle of the tile
	x = (x << TILE_SHIFT) + TILE_UNITS/2;
	y = (y << TILE_SHIFT) + TILE_UNITS/2;

	psAssemblyPoint->coords.x = x;
	psAssemblyPoint->coords.y = y;

	// Deliv Point sits at the height of the tile it's centre is on + arbitary amount!
	psAssemblyPoint->coords.z = map_Height((UWORD)x, (UWORD)y) + 10;
}

/*sets the factory Inc for the Assembly Point*/
void setFlagPositionInc(void *pFunctionality, UDWORD player, UBYTE factoryType)
{
	UBYTE			inc;
	UBYTE			mask = 1;
	FACTORY			*psFactory;
	REPAIR_FACILITY *psRepair;
#ifdef DEBUG
	STRING			*pType;			//if you are going to do this, then make SURE you also do the same to anything
#endif									//that uses the variable.  

	ASSERT((player < MAX_PLAYERS, "setFlagPositionInc: invalid player number"));
	//find the first vacant slot
	for (inc = 0; inc < MAX_FACTORY; inc++)
	{
		if ((factoryNumFlag[player][factoryType] & mask) != mask)
		{
			break;
		}
		mask <<= 1;
	}

	if (inc >= MAX_FACTORY)
	{
        //this may happen now with electronic warfare
#ifdef DEBUG
		switch (factoryType)
		{
		case FACTORY_FLAG:
			pType = "Factory";
			break;
		case CYBORG_FLAG:
			pType = "Cyborg Factory";
			break;
		case VTOL_FLAG:
			pType = "VTOL Factory";
			break;
		case REPAIR_FLAG:
			pType = "Repair Facility";
			break;
		default:
			pType = "";
			break;
		}
#endif
#ifdef DEBUG
	    ASSERT((FALSE, "Building more than %d %s for player %d", MAX_FACTORY, pType, player));
#endif
		inc = 1;
	}

	if (factoryType == REPAIR_FLAG)
	{
		psRepair = (REPAIR_FACILITY *)pFunctionality;
//		psRepair->psAssemblyPoint->factoryInc = inc;
		psRepair->psDeliveryPoint->factoryInc = 0;
		psRepair->psDeliveryPoint->factoryType = factoryType;
//		factoryNumFlag[player][factoryType] |= mask;
	}
	else
	{
		psFactory = (FACTORY *)pFunctionality;
		psFactory->psAssemblyPoint->factoryInc = inc;
		psFactory->psAssemblyPoint->factoryType = factoryType;
		factoryNumFlag[player][factoryType] |= mask;
	}
}

/* called from order.c.. delivery/assembly point handler*/
/*now called from display.c */
void processDeliveryPoint(UDWORD player, UDWORD x, UDWORD y)
{
	FLAG_POSITION	*psCurrFlag;//,*psFlag;//,*psNewFlag
//	STRUCTURE		*psStruct;
//	FACTORY			*psFactory = NULL;
//	UBYTE			factoryFlag = 0;

//	if(bInTutorial)
//	{
//		eventFireCallbackTrigger(CALL_DELIVPOINTMOVED);
///	}

	for (psCurrFlag = apsFlagPosLists[player]; psCurrFlag; psCurrFlag = psCurrFlag->psNext)
	{
		// must be selected and have a valid pos.
		if (psCurrFlag->selected)
		{
			//find the factory belonging to this flag.
/*			for(psStruct=apsStructLists[player];psStruct;psStruct=psStruct->psNext)
			{
				if (((psStruct->pStructureType->type == REF_FACTORY AND 
					psCurrFlag->factoryType == FACTORY_FLAG)
					OR
					(psStruct->pStructureType->type == REF_CYBORG_FACTORY AND 
					psCurrFlag->factoryType == CYBORG_FLAG)
					OR
					(psStruct->pStructureType->type == REF_VTOL_FACTORY AND 
					psCurrFlag->factoryType == VTOL_FLAG)) && 
					((FACTORY *)psStruct->pFunctionality)->psAssemblyPoint->
					factoryInc == psCurrFlag->factoryInc)
				{
					factoryFlag = FACTORY_FLAG;
					if (psStruct->pStructureType->type == REF_CYBORG_FACTORY)
					{
						factoryFlag = CYBORG_FLAG;
					}
					else if (psStruct->pStructureType->type == REF_VTOL_FACTORY)
					{
						factoryFlag = VTOL_FLAG;
					}
					psFactory = (FACTORY *)psStruct->pFunctionality;
					break;
				}
			}*/

			//add additional assembly point
	/*		if( (keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL))
				&&	validAssemblyPoint(x,y,player,psCurrFlag,TRUE) )						
			{
				psFactory->psAssemblyPoint->primary = FALSE;			//disable old priamry

				if(createFlagPosition(&psNewFlag, psCurrFlag->player))
				{
					addFlagPosition(psNewFlag);								//create
			
					psNewFlag->factoryInc = psCurrFlag->factoryInc;			// factory to bind to.
					psNewFlag->factoryType = factoryFlag;
					psNewFlag->primary	  = TRUE;							// set primary.
					psNewFlag->factorySub = (UBYTE)(psFactory->psAssemblyPoint->factorySub + 1);// number of fact points.
					setAssemblyPoint(psNewFlag,x,y);
					psFactory->psAssemblyPoint = psNewFlag;					//update factory to point to new point.
				}
				return;
			}
			else
			{
*/
            //this is not the right function to call anymore - and since there are no
            //intermediate DPs is no longer necessary
			//if(validAssemblyPoint(x,y,player,psCurrFlag,FALSE))
			{
				// move existing point
                /*we do need to check if its valid since the mouse click can 
                be processed the frame after the interface has been up*/
				//setAssemblyPoint(psCurrFlag, x, y, player, FALSE);
                setAssemblyPoint(psCurrFlag, x, y, player, TRUE);

				//deselect once moved
				psCurrFlag->selected = FALSE;
				//if primary deliv point, then remove the other midpoint flags..
/*				if(psFactory->psAssemblyPoint == psCurrFlag)		// it's the primary
				{						
					psFlag = apsFlagPosLists[player];
					while(psFlag)
					{
						// correct factory & not primary.
						if( (psFlag->factoryInc == psFactory->psAssemblyPoint->factoryInc)
							&& (psFlag->factoryType == factoryFlag) 
							&& (psFlag != psFactory->psAssemblyPoint))
						{
							// remove waypoint.
							removeFlagPosition(psFlag);
							// reset back to begining incase we deleted the list head.
							psFlag = apsFlagPosLists[player];
						}
						else
						{
							psFlag = psFlag->psNext;
						}
						// reset subpoints.
//						((FACTORY *)psStruct->pFunctionality)->psAssemblyPoint->factorySub = 0;
					}
				}*/
			}
			return;	//will want to break if more than one can be selected?
		}
	}
}



/*called when a structure has been built - checks through the list of callbacks 
for the scripts*/
void structureCompletedCallback(STRUCTURE_STATS *psStructType)
{

	if (psStructType->type == REF_POWER_GEN)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_POWERGEN_BUILT);
	}
	if (psStructType->type == REF_RESOURCE_EXTRACTOR)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_RESEX_BUILT);
	}
	if (psStructType->type == REF_RESEARCH)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_RESEARCH_BUILT);
	}
	if (psStructType->type == REF_FACTORY OR
		psStructType->type == REF_CYBORG_FACTORY OR
		psStructType->type == REF_VTOL_FACTORY)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_FACTORY_BUILT);
	}

}

STRUCTURE_STATS * structGetDemolishStat( void )
{
	if ( g_psStatDestroyStruct == NULL )
	{
		DBERROR( ("structGetDemolishStat: stat not initialised1\n") );
	}

	return g_psStatDestroyStruct;
}

/*sets the flag to indicate a Power Generator Exists - so do Oil Derrick anim*/
/*void setPowerGenExists(BOOL state, UDWORD player)
{
	powerGenExists[player] = (UBYTE)state;
}*/

/*returns the status of the flag*/
/*BOOL getPowerGenExists(UDWORD player)
{
	return powerGenExists[player];
}*/

/*sets the flag to indicate a HQ Exists - so draw Radar*/
void setHQExists(BOOL state, UDWORD player)
{
	hqExists[player] = (UBYTE)state;
}

/*returns the status of the flag*/
BOOL getHQExists(UDWORD player)
{
//#ifndef PSX
//	if(bMultiPlayer && game.type == DMATCH)
//	{
//		return TRUE;
//	}
//#endif

	return hqExists[player];
}

/*sets the flag to indicate a SatUplink Exists - so draw everything!*/
void setSatUplinkExists(BOOL state, UDWORD player)
{
	satUplinkExists[player] = (UBYTE)state;
}

/*returns the status of the flag*/
BOOL getSatUplinkExists(UDWORD player)
{
	return satUplinkExists[player];
}

/*sets the flag to indicate a Las Sat Exists - ONLY EVER WANT ONE*/
void setLasSatExists(BOOL state, UDWORD player)
{
	lasSatExists[player] = (UBYTE)state;
}

/*returns the status of the flag*/
BOOL getLasSatExists(UDWORD player)
{
	return lasSatExists[player];
}



/* calculate muzzle tip location in 3d world */
BOOL calcStructureMuzzleLocation(STRUCTURE *psStructure, iVector *muzzle)
{
	iVector			barrel;
 	iIMDShape		*psShape, *psWeaponImd;

	psShape       = psStructure->pStructureType->pIMD;
	//if (psStructure->numWeaps > 0)
    if (psStructure->asWeaps[0].nStat > 0)
	{
		psWeaponImd =  asWeaponStats[psStructure->asWeaps[0].nStat].pIMD;
	}
	else
	{
		psWeaponImd =  NULL;
	}

	if(psShape AND psShape->nconnectors)
	{

		// This code has not been translated to the PSX Yet !!!!                                     (sorry)
		pie_MatBegin();

		pie_TRANSLATE(psStructure->x,-(SDWORD)psStructure->z,psStructure->y);
		//matrix = the center of droid
		pie_MatRotY(DEG((SDWORD) psStructure->direction));
		pie_MatRotX(DEG(psStructure->pitch));
		pie_MatRotZ(DEG(-(SDWORD)psStructure->roll));
//		pie_TRANSLATE(100,0,0);			//	(left,-height,forward)
		pie_TRANSLATE( psShape->connectors->x, -psShape->connectors->z,
					  -psShape->connectors->y);//note y and z flipped

		//matrix = the gun and turret mount on the body
		pie_MatRotY(DEG((SDWORD)psStructure->turretRotation));//+ve anticlockwise
		pie_MatRotX(DEG(psStructure->turretPitch));//+ve up
   		pie_MatRotZ(DEG(0));
		//matrix = the muzzle mount on turret
		if( psWeaponImd AND psWeaponImd->nconnectors )
		{
			barrel.x = psWeaponImd->connectors->x;
			barrel.y = -psWeaponImd->connectors->y;
			barrel.z = -psWeaponImd->connectors->z;
		}
		else
		{
			barrel.x = 0;
			barrel.y = 0;
			barrel.z = 0;
		}

		pie_ROTATE_TRANSLATE(barrel.x, barrel.z, barrel.y, muzzle->x, muzzle->z, muzzle->y);
		muzzle->z = -muzzle->z;

		pie_MatEnd();


	}
	else
	{
		muzzle->x = psStructure->x;
		muzzle->y = psStructure->y;
		muzzle->z = psStructure->z + psStructure->sDisplay.imd->ymax;;
	}
	return TRUE;
}

/*Looks through the list of structures to see if there are any inactive 
resource extractors*/
void checkForResExtractors(STRUCTURE *psBuilding)
{
	STRUCTURE		*psCurr;
	POWER_GEN		*psPowerGen;
	RES_EXTRACTOR	*psResExtractor;
	UDWORD			i;
	SDWORD			slot;

	if (psBuilding->pStructureType->type != REF_POWER_GEN)
	{
		ASSERT((FALSE, "checkForResExtractors: invalid structure type"));
		return;
	}
	psPowerGen = (POWER_GEN *)psBuilding->pFunctionality; 
	//count the number of allocated slots
	slot = 0;//-1;
	//for (i=0; i < (NUM_POWER_MODULES + 1); i++)
	for (i=0; i < NUM_POWER_MODULES; i++)
	{
		if (psPowerGen->apResExtractors[i] != NULL)
		{
			slot++;
			//make sure the derrrick is active if any oil left
			psResExtractor = (RES_EXTRACTOR *)psPowerGen->apResExtractors[i]->
				pFunctionality;
			if (psResExtractor->power)
			{
				psResExtractor->active = TRUE;
			}
		}
	}

	psResExtractor = NULL;
	//each Power Gen can cope with 4 Extractors now - 9/6/98 AB
	//check capacity against number of filled slots
	//if (slot < (SDWORD)psPowerGen->capacity)
	if (slot < NUM_POWER_MODULES)
	{
		for (psCurr = apsStructLists[psBuilding->player]; psCurr != NULL; 
			psCurr = psCurr->psNext)
		{
			if (psCurr->pStructureType->type == REF_RESOURCE_EXTRACTOR)
			{
				psResExtractor = (RES_EXTRACTOR *)psCurr->pFunctionality;

				//check not connected and power left and built!
				if (!((RES_EXTRACTOR *)psCurr->pFunctionality)->active  AND 
					psCurr->status == SS_BUILT AND ((RES_EXTRACTOR *)psCurr->
					pFunctionality)->power)
				{
					//assign the extractor to the power generator - use first vacant slot
					//for (i = 0; i < (NUM_POWER_MODULES+1); i++)
					for (i = 0; i < NUM_POWER_MODULES; i++)
					{
						if (psPowerGen->apResExtractors[i] == NULL)
						{
							psPowerGen->apResExtractors[i] = psCurr;
							break;
						}
					}
					//set the owning power gen up for the resource extractor
					psResExtractor->psPowerGen = psBuilding;
					//set the res Extr to active
					psResExtractor->active = TRUE;
					psResExtractor->timeLastUpdated = gameTime;
					slot++;
					//each Power Gen can cope with 4 Extractors now - 9/6/98 AB
					//check to see if any more vacant slots
					//if (slot >= (SDWORD)psPowerGen->capacity)
					if (slot >= NUM_POWER_MODULES)
					{
						//full up so quit out
						break;
					}
				}
			}
		}
	}
}

/*Looks through the list of structures to see if there are any Power Gens
with available slots for the new Res Ext*/
void checkForPowerGen(STRUCTURE *psBuilding)
{
	STRUCTURE		*psCurr;
	UDWORD			i;
	SDWORD			slot;
	POWER_GEN		*psPG;
	RES_EXTRACTOR	*psRE;

	if (psBuilding->pStructureType->type != REF_RESOURCE_EXTRACTOR)
	{
		ASSERT((FALSE, "checkForPowerGen: invalid structure type"));
		return;
	}
	psRE = (RES_EXTRACTOR *)psBuilding->pFunctionality;
	if (psRE->active)
	{
		return;
	}
	
	//loop thru the current structures
	for (psCurr = apsStructLists[psBuilding->player]; psCurr != NULL; 
		psCurr = psCurr->psNext)
	{
		if (psCurr->pStructureType->type == REF_POWER_GEN AND psCurr->status == SS_BUILT)
		{
			psPG = ((POWER_GEN *)psCurr->pFunctionality);
			//check capacity against number of filled slots
			slot = 0;//-1;
			//for (i=0; i < (NUM_POWER_MODULES + 1); i++)
			for (i=0; i < NUM_POWER_MODULES; i++)
			{
				if (psPG->apResExtractors[i] != NULL)
				{
					slot++;
				}
			}
			//each Power Gen can cope with 4 Extractors now - 9/6/98 AB
			//each Power Gen can cope with 4 extractors
			//if ((SDWORD)psPG->capacity > slot)
			if (slot < NUM_POWER_MODULES )
			{
				//find the first vacant slot
				//for (i=0; i < (NUM_POWER_MODULES + 1); i++)
				for (i=0; i < NUM_POWER_MODULES; i++)
				{
					if (psPG->apResExtractors[i] == NULL)
					{
						psPG->apResExtractors[i] = psBuilding;
						psRE->psPowerGen = psCurr;
						psRE->active = TRUE;
						psRE->timeLastUpdated = gameTime;
						return;
					}
				}
			}
		}
	}
}

/*initialise the slot the Resource Extractor filled in the owning Power Gen*/
void informPowerGen(STRUCTURE *psStruct)
{
	UDWORD		i;
	POWER_GEN	*psPowerGen;

	if (psStruct->pStructureType->type != REF_RESOURCE_EXTRACTOR)
	{
		ASSERT((FALSE, "informPowerGen: invalid structure type"));
		return;
	}
	
	//get the owning power generator
	psPowerGen = (POWER_GEN *)((RES_EXTRACTOR *)psStruct->pFunctionality)->
		psPowerGen->pFunctionality;
	if (psPowerGen)
	{
		//for (i=0; i < NUM_POWER_MODULES + 1; i++)
		for (i=0; i < NUM_POWER_MODULES; i++)
		{
			if (psPowerGen->apResExtractors[i] == psStruct)
			{
				//initialise the 'slot'
				psPowerGen->apResExtractors[i] = NULL;
				break;
			}
		}
	}
}

/*called when a Res extractor is destroyed or runs out of power or is disconnected
adjusts the owning Power Gen so that it can link to a different Res Extractor if one
is available*/
void releaseResExtractor(STRUCTURE *psRelease)
{
	STRUCTURE	*psCurr;

	if (psRelease->pStructureType->type != REF_RESOURCE_EXTRACTOR)
	{
		ASSERT((FALSE, "releaseResExtractor:Invalid structure type"));
		return;
	}

	//tell associated Power Gen
	if (((RES_EXTRACTOR *)psRelease->pFunctionality)->psPowerGen)
	{
		informPowerGen(psRelease);
	}

	((RES_EXTRACTOR *)psRelease->pFunctionality)->psPowerGen = NULL;

	//there may be spare resource extractors
	for (psCurr = apsStructLists[psRelease->player]; psCurr != NULL; psCurr =
		psCurr->psNext)
	{
		//check not connected and power left and built!
		if (psCurr->pStructureType->type == REF_RESOURCE_EXTRACTOR AND
			psCurr != psRelease AND !((RES_EXTRACTOR *)psCurr->pFunctionality)->
			active AND ((RES_EXTRACTOR *)psCurr->pFunctionality)->power AND
			psCurr->status == SS_BUILT)
		{
			checkForPowerGen(psCurr);
		}
	}
}

/*called when a Power Gen is destroyed or is disconnected
adjusts the associated Res Extractors so that they can link to different Power 
Gens if any are available*/
void releasePowerGen(STRUCTURE *psRelease)
{
	STRUCTURE	*psCurr;
	POWER_GEN	*psPowerGen;
	UDWORD		i;

	if (psRelease->pStructureType->type != REF_POWER_GEN)
	{
		ASSERT((FALSE, "releasePowerGen:Invalid structure type"));
		return;
	}

	psPowerGen = (POWER_GEN *)psRelease->pFunctionality;
	//go through list of res extractors, setting them to inactive
	//for (i=0; i < (NUM_POWER_MODULES + 1); i++)
	for (i=0; i < NUM_POWER_MODULES; i++)
	{
		if (psPowerGen->apResExtractors[i])
		{
			((RES_EXTRACTOR *)psPowerGen->apResExtractors[i]->
				pFunctionality)->active = FALSE;
			((RES_EXTRACTOR *)psPowerGen->apResExtractors[i]->
				pFunctionality)->psPowerGen = NULL;
			psPowerGen->apResExtractors[i] = NULL;
		}
	}
	//may have a power gen with spare capacity
	for (psCurr = apsStructLists[psRelease->player]; psCurr != NULL; psCurr =
		psCurr->psNext)
	{
		if (psCurr->pStructureType->type == REF_POWER_GEN AND
			psCurr != psRelease AND psCurr->status == SS_BUILT)
		{
			checkForResExtractors(psCurr);
		}
	}
}

/*this is called whenever a structure has finished building*/
void buildingComplete(STRUCTURE *psBuilding)
{
	psBuilding->currentBuildPts = (SWORD)psBuilding->pStructureType->buildPoints;
    psBuilding->status = SS_BUILT;

	switch (psBuilding->pStructureType->type)
	{
		case REF_POWER_GEN:
			checkForResExtractors(psBuilding);

			if(selectedPlayer == psBuilding->player)
			{
				audio_PlayObjStaticTrack( (void *) psBuilding, ID_SOUND_POWER_HUM );
			}

			break;
		case REF_RESOURCE_EXTRACTOR:
			checkForPowerGen(psBuilding);
			/* GJ HACK! - add anim to deriks */
			if (psBuilding->psCurAnim == NULL)
			{
				psBuilding->psCurAnim = animObj_Add(psBuilding, ID_ANIM_DERIK, 0, 0);
			}

			break;
		case REF_RESEARCH:
			intCheckResearchButton();
            //this deals with researc facilities that are upgraded whilst mid-research
            releaseResearch(psBuilding);
			break;
        case REF_FACTORY:
        case REF_CYBORG_FACTORY:
        case REF_VTOL_FACTORY:
            //this deals with factories that are upgraded whilst mid-production
            releaseProduction(psBuilding);
            break;
        case REF_SAT_UPLINK:
            revealAll(psBuilding->player);
		default:
			//do nothing
			break;
	}
}

/*Looks through the players list of structures to see if a HQ exists - will look
through the list of structures at Home Base when on an offWorld mission map*/
BOOL radarCheckForHQ(UDWORD player)
{
//	BOOL		bPlayerHasHQ = FALSE;
	//STRUCTURE	*psStructure;

	return getHQExists(player);


	//if on a mission, need to check mission structs
	/*if (mission.type == LDS_MKEEP OR mission.type == LDS_MCLEAR)
	{
		for (psStructure = mission.apsStructLists[player]; psStructure AND 
			!bPlayerHasHQ; psStructure = psStructure->psNext)
		{
			if (psStructure->pStructureType->type == REF_HQ AND psStructure->
				status == SS_BUILT)
			{
				bPlayerHasHQ = TRUE;
			}
		}
	}
	else
	{
		for (psStructure = apsStructLists[player]; psStructure AND 
			!bPlayerHasHQ; psStructure = psStructure->psNext)
		{
			if (psStructure->pStructureType->type == REF_HQ AND psStructure->
				status == SS_BUILT)
			{
				bPlayerHasHQ = TRUE;
			}
		}
	}
	return bPlayerHasHQ;*/
}

/*for a given structure, return a pointer to its module stat */
STRUCTURE_STATS* getModuleStat(STRUCTURE *psStruct)
{
	STRUCTURE_STATS		*psStat;
	//UDWORD				i;

	ASSERT((PTRVALID(psStruct, sizeof(STRUCTURE)),
		"getModuleStat: Invalid structure pointer"));

	psStat = NULL;
	switch (psStruct->pStructureType->type)
	{
	case REF_POWER_GEN:
		/*for (i = 0; (i < numStructureStats) && (asStructureStats[i].type != 
			REF_POWER_MODULE);i++)
		{
			//keep looking for the Power Module stat...
		}*/
		psStat = &asStructureStats[powerModuleStat];
		break;
	case REF_FACTORY:
	case REF_VTOL_FACTORY:
		/*for (i = 0; (i < numStructureStats) && (asStructureStats[i].type != 
			REF_FACTORY_MODULE);i++)
		{
			//keep looking for the Factory Module stat...
		}*/
		psStat = &asStructureStats[factoryModuleStat];
		break;
	case REF_RESEARCH:
		/*for (i = 0; (i < numStructureStats) && (asStructureStats[i].type != 
			REF_RESEARCH_MODULE);i++)
		{
			//keep looking for the Research Module stat...
		}*/
		psStat = &asStructureStats[researchModuleStat];
		break;
	default:
		//no other structures can have modules attached
		break;
	}

	return psStat;
}


//print some info at the top of the screen dependant on the structure
void printStructureInfo(STRUCTURE *psStructure)
{
	UBYTE		numConnected, i;
	POWER_GEN	*psPowerGen;

	ASSERT((PTRVALID(psStructure, sizeof(STRUCTURE)),
		"printStructureInfo: Invalid Structure pointer"));

	switch (psStructure->pStructureType->type)
	{
	case REF_RESOURCE_EXTRACTOR:
/*#ifdef DEBUG
		CONPRINTF(ConsoleString,(ConsoleString,"%s - Resource Remaining %d - Unique ID %d",
			getStatName(psStructure->pStructureType), ((RES_EXTRACTOR *)psStructure->
			pFunctionality)->power,psStructure->id));
#else
		CONPRINTF(ConsoleString,(ConsoleString,strresGetString(psStringRes,STR_GAM_RESREM),
			getStatName(psStructure->pStructureType), ((RES_EXTRACTOR *)psStructure->
			pFunctionality)->power));
#endif*/
#ifdef DEBUG
		CONPRINTF(ConsoleString,(ConsoleString,"%s - Unique ID %d",
			getStatName(psStructure->pStructureType), psStructure->id));
#else
		CONPRINTF(ConsoleString,(ConsoleString,getStatName(psStructure->pStructureType)));
#endif
		break;
	case REF_POWER_GEN:
		psPowerGen = (POWER_GEN *)psStructure->pFunctionality;
		numConnected = 0;
		for (i = 0; i < NUM_POWER_MODULES; i++)
		{
			if (psPowerGen->apResExtractors[i])
			{
				numConnected++;
			}
		}
#ifdef DEBUG
		CONPRINTF(ConsoleString,(ConsoleString,"%s -  Connected %d of %d - Unique ID %d",
			getStatName(psStructure->pStructureType), numConnected, NUM_POWER_MODULES,
			psStructure->id));
#else
		CONPRINTF(ConsoleString,(ConsoleString,strresGetString(psStringRes,STR_GAM_POWCON),
			getStatName(psStructure->pStructureType), numConnected, NUM_POWER_MODULES));
#endif
		break;
	default:
#ifdef DEBUG
		CONPRINTF(ConsoleString,(ConsoleString,"%s - Damage %d%% - Unique ID %d",
			getStatName(psStructure->pStructureType), 100 - PERCENT(psStructure->body,
			//psStructure->baseBodyPoints),psStructure->id));
			structureBody(psStructure)), psStructure->id));
#else
		CONPRINTF(ConsoleString,(ConsoleString,
			getStatName(psStructure->pStructureType), 100 - PERCENT(psStructure->body,
			//psStructure->baseBodyPoints)));
			structureBody(psStructure))));
#endif
		break;
	}
}

/*Checks the template type against the factory type - returns FALSE 
if not a good combination!*/
BOOL validTemplateForFactory(DROID_TEMPLATE *psTemplate, STRUCTURE *psFactory)
{
    //not in multiPlayer! - AB 26/5/99
    if (!bMultiPlayer)
    {
	    //ignore Transporter Droids
	    if (psTemplate->droidType == DROID_TRANSPORTER)
	    {
		    return FALSE;
	    }
    }

	//check if droid is a cyborg
	if (psTemplate->droidType == DROID_CYBORG OR
		psTemplate->droidType == DROID_CYBORG_SUPER OR
        psTemplate->droidType == DROID_CYBORG_CONSTRUCT OR
        psTemplate->droidType == DROID_CYBORG_REPAIR)
	{
		if (psFactory->pStructureType->type != REF_CYBORG_FACTORY)
		{
			return FALSE;
		}
	}
	//check for VTOL droid
	else if ((asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->
		propulsionType == LIFT)
	{
		if (psFactory->pStructureType->type != REF_VTOL_FACTORY)
		{
			return FALSE;
		}
	}

	//check if cyborg factory 
	if (psFactory->pStructureType->type == REF_CYBORG_FACTORY)
	{
		//if (psTemplate->droidType != DROID_CYBORG)
        if (!(psTemplate->droidType == DROID_CYBORG OR
			  psTemplate->droidType == DROID_CYBORG_SUPER OR
              psTemplate->droidType == DROID_CYBORG_CONSTRUCT OR
              psTemplate->droidType == DROID_CYBORG_REPAIR))
		{
			return FALSE;
		}
	}
	//check if vtol factory
	else if (psFactory->pStructureType->type == REF_VTOL_FACTORY)
	{
		if ((asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->
			propulsionType != LIFT)
		{
			return FALSE;
		}
	}

	//got through all the tests...
	return TRUE;
}

/*calculates the damage caused to the resistance levels of structures - returns
TRUE when captured*/
//BOOL electronicDamage(STRUCTURE *psStructure, UDWORD damage, UBYTE attackPlayer)
//electronic damage can be targetted at droids as well as structures now - AB 5/11/98
BOOL electronicDamage(BASE_OBJECT *psTarget, UDWORD damage, UBYTE attackPlayer)
{
    STRUCTURE   *psStructure;
    DROID       *psDroid;
    BOOL        bCompleted = TRUE;

	NETMSG	m;
	iVector		pos;
	UDWORD		i;


    ASSERT((attackPlayer < MAX_PLAYERS,
	    "electronicDamage: invalid player id"));

    //structure electronic damage
    if (psTarget->type == OBJ_STRUCTURE)
    {
        psStructure = (STRUCTURE *)psTarget;
        bCompleted = FALSE;

	    ASSERT((PTRVALID(psStructure, sizeof(STRUCTURE)),
		    "electronicDamage: Invalid Structure pointer"));

	    ASSERT((psStructure->pStructureType->resistance != 0, 
		    "electronicDamage: invalid structure for EW"));

	    //if resistance is already less than 0 don't do any more 
	    if (psStructure->resistance < 0)
	    {
            bCompleted = TRUE;
		    //return TRUE;
	    }
        else
        {
    	    //store the time it was hit
	        psStructure->timeLastHit = gameTime;

			psStructure->lastHitWeapon = WSC_ELECTRONIC;

	        // tell the cluster system it has been attacked
    	    clustObjectAttacked((BASE_OBJECT *)psStructure);

	        psStructure->resistance = (SWORD)(psStructure->resistance - damage);

	        if (psStructure->resistance < 0)
	        {
                //add a console message for the selected Player
		        if (psStructure->player == selectedPlayer)
		        {
       			    CONPRINTF(ConsoleString,(ConsoleString,strresGetString(
                        psStringRes,STR_GAM_ELECDAM),
		    		    getStatName(psStructure->pStructureType)));
                    //tell the scripts if selectedPlayer has lost a structure
                    eventFireCallbackTrigger((TRIGGER_TYPE)CALL_ELECTRONIC_TAKEOVER);
	    	    }
		        //give a reward to the attacking player
    		    //researchReward(psStructure->player, attackPlayer);
	    	    //electronicReward(psStructure, attackPlayer);
		        //return TRUE;
                bCompleted = TRUE;
                //give the structure to the attacking player
                (void)giftSingleStructure(psStructure, attackPlayer, FALSE);
	        }
        }
	    //return FALSE;
    }
    //droid electronic damage
    else if (psTarget->type == OBJ_DROID)
    {
        psDroid = (DROID *)psTarget;
        bCompleted = FALSE;

	    ASSERT((PTRVALID(psDroid, sizeof(DROID)),
		    "electronicDamage: Invalid Droid pointer"));

        //in multiPlayer cannot attack a Transporter with EW
        if (bMultiPlayer)
        {
            if (psDroid->droidType == DROID_TRANSPORTER)
            {
                ASSERT((FALSE, "electronicDamage: Cannot attack a Transporter in multiPlayer"));
                return TRUE;
            }
        }
        
        if (psDroid->resistance == ACTION_START_TIME)
        {
            //need to set the current resistance level since not been previously attacked (by EW)
            psDroid->resistance = droidResistance(psDroid); 
        }

	    if (psDroid->resistance < 0)
	    {
            bCompleted = TRUE;
	    }
        else
        {
	        // tell the cluster system it has been attacked
    	    clustObjectAttacked((BASE_OBJECT *)psDroid);

	        psDroid->resistance = (SWORD)(psDroid->resistance - damage);

	        if (psDroid->resistance <= 0)
	        {
                //add a console message for the selected Player
		        if (psDroid->player == selectedPlayer)
		        {
    			    CONPRINTF(ConsoleString,(ConsoleString,strresGetString(
                        psStringRes,STR_GAM_ELECDAM)));
                    //tell the scripts if selectedPlayer has lost a droid
                    eventFireCallbackTrigger((TRIGGER_TYPE)CALL_ELECTRONIC_TAKEOVER);
				}
                bCompleted = TRUE;

                //give the droid to the attacking player

			   	if(psDroid->visible[selectedPlayer])
				{
					for(i=0; i<5; i++)
					{
						pos.x = psDroid->x + (30-rand()%60);
						pos.z = psDroid->y + (30-rand()%60); 
						pos.y = psDroid->z + (rand()%8);
						effectGiveAuxVar(80);
						addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_FLAMETHROWER,FALSE,NULL,0);
					}
				}


               
				
				(void)giftSingleDroid(psDroid, attackPlayer);
					
				// tell the world!
				if(bMultiPlayer)
				{
					m.body[0] = DROID_GIFT;
					m.body[1] = (UBYTE)psDroid->player;
					m.body[2] = (UBYTE)attackPlayer;
					m.type = NET_GIFT;
					m.size = 3;
					NetAdd(m,m.size,psDroid->id);
					m.size += sizeof(psDroid->id);
					NETbcast(&m,TRUE);	//send it
				}

				//check to see if droid limit reached, if so recycle.
				// don't check for transporter/mission coz multiplayer only issue.
				if( (getNumDroids(attackPlayer)+1 ) > getMaxDroids(attackPlayer) )
				{					
					recycleDroid(psDroid);	
				}
		
			}
        }
        /* called in droidDamage now - AB 17/06/99
#ifndef PSX
		psDroid->timeLastHit = gameTime;
		psDroid->lastHitWeapon = WSC_ELECTRONIC;
#endif
        */

    }

    return bCompleted;
}

/* EW works differently in multiplayer mode compared with single player.*/
BOOL validStructResistance(STRUCTURE *psStruct)
{
    BOOL    bTarget = FALSE;

	ASSERT((PTRVALID(psStruct, sizeof(STRUCTURE)),
		"invalidStructResistance: invalid structure pointer"));

#ifdef TEST_EW
    bMultiPlayer = TRUE;
#endif

    if (psStruct->pStructureType->resistance != 0)
    {
        /*certain structures will only provide rewards in multiplayer so
        before they can become valid targets their resistance must be at least 
        half the base value*/
        if (bMultiPlayer)
        {
            switch(psStruct->pStructureType->type)
            {
	        case REF_RESEARCH:
	        case REF_FACTORY:
            case REF_VTOL_FACTORY:
            case REF_CYBORG_FACTORY:
	        case REF_HQ:
	        case REF_REPAIR_FACILITY:
    		    if (psStruct->resistance >= (SDWORD) (structureResistance(psStruct->
                    pStructureType, psStruct->player) / 2))
                {
                    bTarget = TRUE;
                }
		        break;
	        default:
                bTarget = TRUE;
            }
        }
        else
        {
            bTarget = TRUE;
        }
    }

#ifdef TEST_EW
    bMultiPlayer = FALSE;
#endif

    return bTarget;
}


/*Access functions for the upgradeable stats of a structure*/
UDWORD	structureBody(STRUCTURE *psStructure)
{
	STRUCTURE_STATS		*psStats;
	UBYTE				player;//, i;

	psStats = psStructure->pStructureType;
	player = psStructure->player;

	switch(psStats->type)
	{
		//wall/defence structures
	case REF_DEFENSE:
	case REF_WALL:
	case REF_WALLCORNER:
	case REF_BLASTDOOR:
		return (psStats->bodyPoints + (psStats->bodyPoints * 
			asWallDefenceUpgrade[player].body)/100);
		break;
	default:
		//all other structures
		return (structureBaseBody(psStructure) + (structureBaseBody(psStructure) * 
			asStructureUpgrade[player].body)/100);
		break;
	}
}

/*this returns the Base Body points of a structure - regardless of upgrade*/
UDWORD	structureBaseBody(STRUCTURE *psStructure)
{
	STRUCTURE_STATS		*psStats;
	UBYTE				player, capacity;
	UDWORD				body;

	ASSERT((PTRVALID(psStructure, sizeof(STRUCTURE)),
		"structureBaseBody: invalid structure pointer"));

	psStats = psStructure->pStructureType;
	player = psStructure->player;

	switch(psStats->type)
	{
		//modules may be attached
	case REF_FACTORY:
	case REF_VTOL_FACTORY:
		ASSERT((PTRVALID(psStructure->pFunctionality, sizeof(FUNCTIONALITY)),
			"structureBaseBody: invalid structure functionality pointer"));
		if (((FACTORY *)psStructure->pFunctionality)->capacity > 0)
		{
			body = 0;
			capacity = ((FACTORY *)psStructure->pFunctionality)->capacity;
			while (capacity)
			{
				body += asStructureStats[factoryModuleStat].bodyPoints;
				capacity--;
			}
			//add on the default for the factory
			body += psStats->bodyPoints;
			return body;
		}
		else
		{
			//no modules
			return psStats->bodyPoints;
		}
		break;
	case REF_RESEARCH:
		ASSERT((PTRVALID(psStructure->pFunctionality, sizeof(FUNCTIONALITY)),
			"structureBaseBody: invalid structure functionality pointer"));
		if (((RESEARCH_FACILITY *)psStructure->pFunctionality)->capacity > 0)
		{
			body = 0;
			body = asStructureStats[researchModuleStat].bodyPoints;
			//add on the default for the factory
			body += psStats->bodyPoints;
			return body;
		}
		else
		{
			//no modules
			return psStats->bodyPoints;
		}
		break;
	case REF_POWER_GEN:
		ASSERT((PTRVALID(psStructure->pFunctionality, sizeof(FUNCTIONALITY)),
			"structureBaseBody: invalid structure functionality pointer"));
		if (((POWER_GEN *)psStructure->pFunctionality)->capacity > 0)
		{
			body = 0;
			body = asStructureStats[powerModuleStat].bodyPoints;
			//add on the default for the factory
			body += psStats->bodyPoints;
			return body;
		}
		else
		{
			//no modules
			return psStats->bodyPoints;
		}

		//all other structures
	default:
		return psStats->bodyPoints;
		break;
	}
}

UDWORD	structureArmour(STRUCTURE_STATS *psStats, UBYTE player)
{
	switch(psStats->type)
	{
	case REF_DEFENSE:
	case REF_WALL:
	case REF_WALLCORNER:
	case REF_BLASTDOOR:
		return (psStats->armourValue + (psStats->armourValue * 
			asWallDefenceUpgrade[player].armour)/100);
		break;
	default:
		return (psStats->armourValue + (psStats->armourValue * 
			asStructureUpgrade[player].armour)/100);
		break;
	}
}

UDWORD	structureResistance(STRUCTURE_STATS *psStats, UBYTE player)
{
	switch(psStats->type)
	{
        //defense type can now be upgradede
	//case REF_DEFENSE: 
	case REF_WALL:
	case REF_WALLCORNER:
	case REF_BLASTDOOR:
		//not upgradeable
		return psStats->resistance;
		break;
	default:
		return (psStats->resistance + (psStats->resistance * 
			asStructureUpgrade[player].resistance)/100);
		break;
	}
}

//access function for sensor stats
UDWORD structureSensorRange(STRUCTURE_STATS *psStats)
{
	if (psStats->pSensor)
	{
		return psStats->pSensor->range;
	}
	else
	{
		return 0;
	}
}
UDWORD structureSensorPower(STRUCTURE_STATS *psStats)
{
	if (psStats->pSensor)
	{
		return psStats->pSensor->power;
	}
	else
	{
		return 0;
	}
}



/*gives the attacking player a reward based on the type of structure that has 
been attacked*/
BOOL electronicReward(STRUCTURE *psStructure, UBYTE attackPlayer)
{
    BOOL    bRewarded = FALSE;

	switch(psStructure->pStructureType->type)
	{
	case REF_RESEARCH:
		researchReward(psStructure->player, attackPlayer);
        bRewarded = TRUE;
		break;
	case REF_FACTORY:
    case REF_VTOL_FACTORY:
    case REF_CYBORG_FACTORY:
		factoryReward(psStructure->player, attackPlayer);
        bRewarded = TRUE;
		break;
	case REF_HQ:
		hqReward(psStructure->player,attackPlayer);
		if (attackPlayer == selectedPlayer)
		{
			addConsoleMessage(strresGetString(psStringRes,STR_GAM_REWELEC),	DEFAULT_JUSTIFY);
		}
        bRewarded = TRUE;
		break;
	case REF_REPAIR_FACILITY:
		repairFacilityReward(psStructure->player,attackPlayer);
        bRewarded = TRUE;
		break;
	default:
		//ASSERT((FALSE, "electronic Reward for a building not catered for - CANCEL will continue the game!"));
        bRewarded = FALSE;
	}

    return bRewarded;
}


/*find the 'best' prop/body/weapon component the losing player has and 
'give' it to the reward player*/
void factoryReward(UBYTE losingPlayer, UBYTE rewardPlayer)
{
	UDWORD		inc, comp = 0;

	//search through the propulsions first
	for (inc=0; inc < numPropulsionStats; inc++)
	{
		if (apCompLists[losingPlayer][COMP_PROPULSION][inc] == AVAILABLE AND
			apCompLists[rewardPlayer][COMP_PROPULSION][inc] != AVAILABLE)
		{
			if (asPropulsionStats[inc].buildPower > asPropulsionStats[comp].buildPower)
			{
				comp = inc;
			}
		}
	}
	if (comp != 0)
	{
		apCompLists[rewardPlayer][COMP_PROPULSION][comp] = AVAILABLE;
		if (rewardPlayer == selectedPlayer)
		{
			//addConsoleMessage(strresGetString(psStringRes,STR_GAM_REWPROP), DEFAULT_JUSTIFY);
           	CONPRINTF(ConsoleString,(ConsoleString,"%s :- %s",
        	    strresGetString(psStringRes,STR_GAM_REWPROP), 
                getName(asPropulsionStats[comp].pName)));
		}
		return;
	}
	
	//haven't found a propulsion - look for a body
	for (inc=0; inc < numBodyStats; inc++)
	{
		if (apCompLists[losingPlayer][COMP_BODY][inc] == AVAILABLE AND
			apCompLists[rewardPlayer][COMP_BODY][inc] != AVAILABLE)
		{
			if (asBodyStats[inc].buildPower > asBodyStats[comp].buildPower)
			{
				comp = inc;
			}
		}
	}
	if (comp != 0)
	{
		apCompLists[rewardPlayer][COMP_BODY][comp] = AVAILABLE;
		if (rewardPlayer == selectedPlayer)
		{
			//addConsoleMessage(strresGetString(psStringRes,STR_GAM_REWBODY), DEFAULT_JUSTIFY);
           	CONPRINTF(ConsoleString,(ConsoleString,"%s :- %s",
        	    strresGetString(psStringRes,STR_GAM_REWBODY), 
                getName(asBodyStats[comp].pName)));
		}
		return;
	}

	//haven't found a body - look for a weapon
	for (inc=0; inc < numWeaponStats; inc++)
	{
		if (apCompLists[losingPlayer][COMP_WEAPON][inc] == AVAILABLE AND
			apCompLists[rewardPlayer][COMP_WEAPON][inc] != AVAILABLE)
		{
			if (asWeaponStats[inc].buildPower > asWeaponStats[comp].buildPower)
			{
				comp = inc;
			}
		}
	}
	if (comp != 0)
	{
		apCompLists[rewardPlayer][COMP_WEAPON][comp] = AVAILABLE;
		if (rewardPlayer == selectedPlayer)
		{
			//addConsoleMessage(strresGetString(psStringRes,STR_GAM_REWWEAP), DEFAULT_JUSTIFY);
           	CONPRINTF(ConsoleString,(ConsoleString,"%s :- %s",
        	    strresGetString(psStringRes,STR_GAM_REWWEAP), 
                getName(asWeaponStats[comp].pName)));
		}
		return;
	}
	
	//losing Player hasn't got anything better so don't gain anything!
	if (rewardPlayer == selectedPlayer)
	{
		addConsoleMessage(strresGetString(psStringRes,STR_GAM_REWNOWT), DEFAULT_JUSTIFY);
	}
}

/*find the 'best' repair component the losing player has and 
'give' it to the reward player*/
void repairFacilityReward(UBYTE losingPlayer, UBYTE rewardPlayer)
{
	UDWORD		inc, comp = 0;

	//search through the repair stats
	for (inc=0; inc < numRepairStats; inc++)
	{
		if (apCompLists[losingPlayer][COMP_REPAIRUNIT][inc] == AVAILABLE AND
			apCompLists[rewardPlayer][COMP_REPAIRUNIT][inc] != AVAILABLE)
		{
			if (asRepairStats[inc].buildPower > asRepairStats[comp].buildPower)
			{
				comp = inc;
			}
		}
	}
	if (comp != 0)
	{
		apCompLists[rewardPlayer][COMP_REPAIRUNIT][comp] = AVAILABLE;
		if (rewardPlayer == selectedPlayer)
		{
			//addConsoleMessage(strresGetString(psStringRes,STR_GAM_REWREPA), DEFAULT_JUSTIFY);
           	CONPRINTF(ConsoleString,(ConsoleString,"%s :- %s",
        	    strresGetString(psStringRes,STR_GAM_REWREPA), 
                getName(asRepairStats[comp].pName)));
		}
		return;
	}
	if (rewardPlayer == selectedPlayer)
	{
		addConsoleMessage(strresGetString(psStringRes,STR_GAM_REWREPN), DEFAULT_JUSTIFY);
	}
}

/*makes the losing players tiles/structures/features visible to the reward player*/
void hqReward(UBYTE losingPlayer, UBYTE rewardPlayer)
{
	STRUCTURE *psStruct;
	FEATURE	  *psFeat;
	DROID	  *psDroid;
	UDWORD	x,y,i;
	MAPTILE	*psTile;

	//tiles
	for(x = 0; x < mapWidth; x++)
	{
		for(y = 0; y < mapHeight; y++)
		{
			psTile = mapTile(x,y);
			
			if(TEST_TILE_VISIBLE(losingPlayer,psTile))
			{
				SET_TILE_VISIBLE(rewardPlayer,psTile);

				if(getRevealStatus())
				{
					if(rewardPlayer == selectedPlayer)
					{
						avInformOfChange(x,y);
					}
				}


			}
		}
	}
	
	//struct
	for(i=0;i<MAX_PLAYERS;i++)
	{
		for(psStruct = apsStructLists[i]; psStruct != NULL; psStruct = psStruct->psNext)
  		{
			if( psStruct->visible[losingPlayer] && !psStruct->died)
			{
				psStruct->visible[rewardPlayer] = psStruct->visible[losingPlayer];

				if(rewardPlayer == selectedPlayer)
				{
					setStructTileDraw(psStruct);
				}
			}	
		}

		//feature
		for(psFeat = apsFeatureLists[i]; psFeat != NULL; psFeat = psFeat->psNext)
		{
			if(psFeat->visible[losingPlayer] )
			{
				psFeat->visible[rewardPlayer] = psFeat->visible[losingPlayer];
				if(rewardPlayer == selectedPlayer)
				{
					setFeatTileDraw(psFeat);
				}
			}	
		}

		//droids.
		for(psDroid = apsDroidLists[i]; psDroid != NULL; psDroid = psDroid->psNext)
		{
			if(psDroid->visible[losingPlayer] || psDroid->player == losingPlayer)
			{
				psDroid->visible[rewardPlayer] =UBYTE_MAX;
			}	
		}

	}
}


// Return TRUE if structure is a factory of any type.
//
BOOL StructIsFactory(STRUCTURE *Struct)
{
	if( (Struct->pStructureType->type == REF_FACTORY) || 
		(Struct->pStructureType->type == REF_CYBORG_FACTORY) ||
		(Struct->pStructureType->type == REF_VTOL_FACTORY) )
	{
		return TRUE;
	}

	return FALSE;
}


// Return true if flag is a delivery point for a factory.
//
BOOL FlagIsFactory(FLAG_POSITION *psCurrFlag)
{
	if( (psCurrFlag->factoryType == FACTORY_FLAG) || (psCurrFlag->factoryType == CYBORG_FLAG) ||
		(psCurrFlag->factoryType == VTOL_FLAG) )
	{
		return TRUE;
	}

	return FALSE;
}


// Find a structure's delivery point , only if it's a factory.
// Returns NULL if not found or the structure isn't a factory.
//
FLAG_POSITION *FindFactoryDelivery(STRUCTURE *Struct)
{
	FLAG_POSITION	*psCurrFlag;

	if(StructIsFactory(Struct))
	{
		// Find the factories delivery point.
		for (psCurrFlag = apsFlagPosLists[selectedPlayer]; psCurrFlag;
			psCurrFlag = psCurrFlag->psNext)
		{
			if(FlagIsFactory(psCurrFlag) && 
				((FACTORY *)Struct->pFunctionality)->psAssemblyPoint->
				factoryInc == psCurrFlag->factoryInc &&
				((FACTORY *)Struct->pFunctionality)->psAssemblyPoint->
				factoryType == psCurrFlag->factoryType) 
			{
				return psCurrFlag;
			}
		}
	}

	return NULL;
}

//Find the factory associated with the delivery point - returns NULL if none exist
STRUCTURE	*findDeliveryFactory(FLAG_POSITION *psDelPoint)
{
	STRUCTURE	*psCurr;
	FACTORY		*psFactory;
	REPAIR_FACILITY *psRepair;

	for (psCurr = apsStructLists[psDelPoint->player]; psCurr != NULL; psCurr = 
		psCurr->psNext)
	{
		if(StructIsFactory(psCurr))
		{
			psFactory = (FACTORY *)psCurr->pFunctionality;
			if (psFactory->psAssemblyPoint->factoryInc == psDelPoint->factoryInc AND
				psFactory->psAssemblyPoint->factoryType == psDelPoint->factoryType)
			{
				return psCurr;
			}
		}
		else if (psCurr->pStructureType->type == REF_REPAIR_FACILITY)
		{
			psRepair = (REPAIR_FACILITY *)psCurr->pFunctionality;
			if (psRepair->psDeliveryPoint == psDelPoint)
			{
				return psCurr;
			}
		}
	}
	return NULL;
}

/*cancels the production run for the factory and returns any power that was 
accrued but not used*/
void cancelProduction(STRUCTURE *psBuilding)
{
	FACTORY		*psFactory;

	ASSERT((StructIsFactory(psBuilding), 
		"cancelProduction: structure not a factory"));

	psFactory = (FACTORY *)psBuilding->pFunctionality;

    //check its the correct factory
   	if (psBuilding->player == productionPlayer AND psFactory->psSubject)
    {
		//clear the production run for this factory
		memset(asProductionRun[psFactory->psAssemblyPoint->factoryType][
			psFactory->psAssemblyPoint->factoryInc], 0, sizeof(PRODUCTION_RUN) * 
    		MAX_PROD_RUN);
	    //return any accrued power
		if (psFactory->powerAccrued)
		{
			addPower(psBuilding->player, psFactory->powerAccrued);
		}
		//clear the factories subject and quantity
		psFactory->psSubject = NULL;
		psFactory->quantity = 0;
		//tell the interface
		intManufactureFinished(psBuilding);
    }
}

/*set a factory's production run to hold*/
void holdProduction(STRUCTURE *psBuilding)
{

	FACTORY		*psFactory;

	ASSERT((StructIsFactory(psBuilding), 
		"holdProduction: structure not a factory"));

	psFactory = (FACTORY *)psBuilding->pFunctionality;

	if (psFactory->psSubject)
	{
		//set the time the factory was put on hold
		psFactory->timeStartHold = gameTime;
		//play audio to indicate on hold
		if (psBuilding->player == selectedPlayer)
		{
			audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
		}
	}

}

/*release a factory's production run from hold*/
void releaseProduction(STRUCTURE *psBuilding)
{
	FACTORY		*psFactory;

	ASSERT((StructIsFactory(psBuilding), 
		"releaseProduction: structure not a factory"));

	psFactory = (FACTORY *)psBuilding->pFunctionality;

	if (psFactory->psSubject AND psFactory->timeStartHold)
	{
		//adjust the start time for the current subject
		if (psFactory->timeStarted != ACTION_START_TIME)
		{
			psFactory->timeStarted += (gameTime - psFactory->timeStartHold);
		}
		psFactory->timeStartHold = 0;
	}
}

/*this is called when a factory produces a droid. The Template returned is the next 
one to build - if any*/
DROID_TEMPLATE * factoryProdUpdate(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate)
{
	UDWORD		inc, factoryType, factoryInc;
	FACTORY		*psFactory;

	ASSERT((psStructure->player == productionPlayer, 
		"factoryProdUpdate: called for incorrect player"));

	psFactory = (FACTORY *)psStructure->pFunctionality;
	factoryType = psFactory->psAssemblyPoint->factoryType;
	factoryInc = psFactory->psAssemblyPoint->factoryInc;

	if (psTemplate != NULL)
	{
		//find the entry in the array for this template
		for (inc=0; inc < MAX_PROD_RUN; inc++)
		{
			if (asProductionRun[factoryType][factoryInc][inc].psTemplate == psTemplate)
			{
				asProductionRun[factoryType][factoryInc][inc].built++;
				if (asProductionRun[factoryType][factoryInc][inc].built < 
					asProductionRun[factoryType][factoryInc][inc].quantity)
				{
					return psTemplate;
				}
				break;
			}
		}
	}
	//find the next template to build - this just looks for the first uncompleted run
	for (inc=0; inc < MAX_PROD_RUN; inc++)
	{
		if (asProductionRun[factoryType][factoryInc][inc].built < 
			asProductionRun[factoryType][factoryInc][inc].quantity)
		{
			return asProductionRun[factoryType][factoryInc][inc].psTemplate;
		}
	}
	/*If you've got here there's nothing left to build unless factory is 
      on loop production*/
	if (psFactory->quantity)
	{
		//reduce the loop count if not infinite
		if (psFactory->quantity != INFINITE_PRODUCTION)
		{
			psFactory->quantity--;
		}

		//need to reset the quantity built for each entry in the production list
		for (inc=0; inc < MAX_PROD_RUN; inc++)
		{
			if (asProductionRun[factoryType][factoryInc][inc].psTemplate)
			{
				asProductionRun[factoryType][factoryInc][inc].built = 0;
			}
		}
		//get the first to build again
		return (factoryProdUpdate(psStructure, NULL));
	}
	//if got to here then nothing left to produce so clear the array
	memset(asProductionRun[factoryType][factoryInc], 0, 
		sizeof(PRODUCTION_RUN) * MAX_PROD_RUN);
	return NULL;
}

//adjust the production run for this template type
void factoryProdAdjust(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate, BOOL add)
{
	SDWORD		spare = -1;
	UDWORD		inc, factoryType, factoryInc;
	FACTORY		*psFactory;
	BOOL		bAssigned = FALSE, bCheckForCancel = FALSE;

	ASSERT((psStructure->player == productionPlayer, 
		"factoryProdAdjust: called for incorrect player"));

	psFactory = (FACTORY *)psStructure->pFunctionality;
	factoryType = psFactory->psAssemblyPoint->factoryType;
	factoryInc = psFactory->psAssemblyPoint->factoryInc;

	//see if the template is already in the list
	for (inc=0; inc < MAX_PROD_RUN; inc++)
	{
		if (asProductionRun[factoryType][factoryInc][inc].psTemplate == psTemplate)
		{
			//adjust the prod run
			if (add)
			{
				asProductionRun[factoryType][factoryInc][inc].quantity++;
				if (asProductionRun[factoryType][factoryInc][inc].quantity > MAX_IN_RUN)
				{
//#ifdef PSX	// Don't wrap around, just max out.
//					asProductionRun[factoryType][factoryInc][inc].quantity = MAX_IN_RUN;
//#else
					asProductionRun[factoryType][factoryInc][inc].quantity = 0;
					//initialise the template
					asProductionRun[factoryType][factoryInc][inc].psTemplate = NULL;
                    bCheckForCancel = TRUE;
                    //add power back if we were working on this one
                    if (psFactory->psSubject == (BASE_STATS *)psTemplate)
                    {
                        addPower(psStructure->player, psFactory->powerAccrued);
                        //set the factory's power accrued back to zero
                        psFactory->powerAccrued = 0;
                    }
//#endif
				}
			}
			else
			{
				if (asProductionRun[factoryType][factoryInc][inc].quantity == 0)
				{
//#ifdef PSX	// Don't wrap around
//#else
					asProductionRun[factoryType][factoryInc][inc].quantity = MAX_IN_RUN;
//#endif
				}
				else
				{
					asProductionRun[factoryType][factoryInc][inc].quantity--;
                    //add power back if we were working on this one
                    if (psFactory->psSubject == (BASE_STATS *)psTemplate)
                    {
                        addPower(psStructure->player, psFactory->powerAccrued);
                        //set the factory's power accrued back to zero
                        psFactory->powerAccrued = 0;
                    }

					if (asProductionRun[factoryType][factoryInc][inc].quantity == 0)
					{
//#ifdef PSX	// Don't wrap around
//#else
						//initialise the template
						asProductionRun[factoryType][factoryInc][inc].psTemplate = NULL;
						bCheckForCancel = TRUE;
//#endif
					}
				}
			}
			bAssigned = TRUE;
			break;
		}
		//check to see if any empty slots
		if (spare == -1 AND asProductionRun[factoryType][factoryInc][inc].quantity == 0)
		{
			spare = inc;
		}
	}

	if (!bAssigned AND spare != -1)
	{
		//start off a new template
		asProductionRun[factoryType][factoryInc][spare].psTemplate = psTemplate;
		if (add)
		{
			asProductionRun[factoryType][factoryInc][spare].quantity = 1;
		}
		else
		{
//#ifdef PSX	// Don't wrap around.
//#else
			//wrap around to max value
			asProductionRun[factoryType][factoryInc][spare].quantity = MAX_IN_RUN;
//#endif
		}
	}
	//if nothing is allocated then the current factory may have been cancelled
	if (bCheckForCancel)
	{
		for (inc=0; inc < MAX_PROD_RUN; inc++)
		{
			if (asProductionRun[factoryType][factoryInc][inc].psTemplate != NULL)
			{
				break;
			}
		}
		if (inc == MAX_PROD_RUN)
		{
			//must have cancelled eveything - so tell the struct
			psFactory->quantity = 0;
            //all the power is returned when the quantity is decreased now (see above addPower())- AB 8/2/99
			//if started the subject then return half power, else return all accrued
			/*if (psFactory->timeStarted == ACTION_START_TIME)
			{
				addPower(psStructure->player, psFactory->powerAccrued);
			}
			else
			{
				addPower(psStructure->player, ((DROID_TEMPLATE *)psFactory->
					psSubject)->powerPoints / 2);
			}*/
			//psFactory->psSubject = NULL;
			//intManufactureFinished(psStructure);
		}
	}
}

//returns the quantity of a specific template in the production list
UDWORD	getProductionQuantity(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate)
{
	UDWORD		inc, factoryType, factoryInc;
	FACTORY		*psFactory;

	//ASSERT((psStructure->player == selectedPlayer, 
	//	"getProductionQuantity: should only be called for selectedPlayer"));

    if (psStructure->player == productionPlayer)
    {
        psFactory = (FACTORY *)psStructure->pFunctionality;
        factoryType = psFactory->psAssemblyPoint->factoryType;
        factoryInc = psFactory->psAssemblyPoint->factoryInc;

        //see if the template is in the list
        for (inc=0; inc < MAX_PROD_RUN; inc++)
        {
            if (asProductionRun[factoryType][factoryInc][inc].psTemplate == psTemplate)
		    {
                return asProductionRun[factoryType][factoryInc][inc].quantity;
            }
        }
    }

	//not in the list so none being produced
	return 0;
}

/*returns the quantity of a specific template in the production list that 
have already been built*/
UDWORD	getProductionBuilt(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate)
{
	UDWORD		inc, factoryType, factoryInc;
	FACTORY		*psFactory;

	//ASSERT((psStructure->player == selectedPlayer, 
	//	"getProductionBuilt: should only be called for selectedPlayer"));

    if (psStructure->player == productionPlayer)
    {
	    psFactory = (FACTORY *)psStructure->pFunctionality;
	    factoryType = psFactory->psAssemblyPoint->factoryType;
	    factoryInc = psFactory->psAssemblyPoint->factoryInc;

        //see if the template is in the list
        for (inc=0; inc < MAX_PROD_RUN; inc++)
        {
		    if (asProductionRun[factoryType][factoryInc][inc].psTemplate == psTemplate)
		    {
			    return asProductionRun[factoryType][factoryInc][inc].built;
		    }
        }
    }

	//not in the list so none being produced
	return 0;
}


/*looks through a players production list to see how many command droids 
are being built*/
UBYTE checkProductionForCommand(UBYTE player)
{
	UBYTE		factoryInc, inc, factoryType;
	UBYTE		mask = 1, quantity = 0;

	//ASSERT((player == selectedPlayer, 
	//	"checkProductionForCommand: should only be called for selectedPlayer"));

    if (player == productionPlayer)
    {
	    //assumes Cyborg or VTOL droids are not Command types!
	    factoryType = FACTORY_FLAG;

	    for (factoryInc = 0; factoryInc < MAX_FACTORY; factoryInc++)
	    {
		    //check to see if there is a factory
		    if ((factoryNumFlag[player][factoryType] & mask) == mask)
		    {
			    for (inc=0; inc < MAX_PROD_RUN; inc++)
			    {
				    if (asProductionRun[factoryType][factoryInc][inc].psTemplate)
				    {
					    if (asProductionRun[factoryType][factoryInc][inc].psTemplate->
						    droidType == DROID_COMMAND)
					    {
						    quantity = (UBYTE)(quantity  + (asProductionRun[factoryType][
							    factoryInc][inc].quantity -	asProductionRun[
							    factoryType][factoryInc][inc].built));
					    }
				    }
			    }
		    }
		    mask <<= 1;
	    }
    }
	return quantity;
}


// Count number of factories assignable to a command droid.
//
UWORD countAssignableFactories(UBYTE player,UWORD factoryType)
{
	UWORD		factoryInc;
	UBYTE		mask = 1, quantity = 0;

	ASSERT((player == selectedPlayer, 
		"countAssignableFactories: should only be called for selectedPlayer"));

	for (factoryInc = 0; factoryInc < MAX_FACTORY; factoryInc++)
	{
		//check to see if there is a factory
		if ((factoryNumFlag[player][factoryType] & mask) == mask)
		{
			quantity++;
		}
		mask <<= 1;
	}

	return quantity;
}

// check whether a factory of a certain number and type exists
BOOL checkFactoryExists(UDWORD player, UDWORD factoryType, UDWORD inc)
{
	ASSERT((player < MAX_PLAYERS,
		"checkFactoryExists: invalid player"));
	ASSERT((factoryType < NUM_FACTORY_TYPES,
		"checkFactoryExists: invalid factoryType"));

	return (factoryNumFlag[player][factoryType] & (1 << inc)) != 0;
}


//check that delivery points haven't been put down in invalid location
void checkDeliveryPoints(UDWORD version)
{
	UBYTE			inc;
	STRUCTURE		*psStruct;
	FACTORY			*psFactory;
	REPAIR_FACILITY	*psRepair;
	UDWORD					x, y;

	//find any factories
	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
        //don't bother checking selectedPlayer's - causes problems when try and use
        //validLocation since it finds that the DP is on itself! And validLocation
        //will have been called to put in down in the first place 
        if (inc != selectedPlayer)
        {
		    for (psStruct = apsStructLists[inc]; psStruct != NULL; psStruct = 
			    psStruct->psNext)
		    {
			    if(StructIsFactory(psStruct))
			    {
				    //check the DP
				    psFactory = (FACTORY *)psStruct->pFunctionality;
				    if (psFactory->psAssemblyPoint == NULL)//need to add one
				    {
					    ASSERT((psFactory->psAssemblyPoint != NULL,
                            "checkDeliveryPoints: no delivery point for factory"));
				    }
				    else
				    {
					    setAssemblyPoint(psFactory->psAssemblyPoint, psFactory->psAssemblyPoint->
						    coords.x, psFactory->psAssemblyPoint->coords.y, inc, TRUE);
				    }
			    }
			    else if (psStruct->pStructureType->type == REF_REPAIR_FACILITY)
			    {
				    psRepair = (REPAIR_FACILITY *)psStruct->pFunctionality;

				    if (psRepair->psDeliveryPoint == NULL)//need to add one
				    {
					    if (version >= VERSION_19)
					    {
						    ASSERT((psRepair->psDeliveryPoint != NULL,"checkDeliveryPoints: no delivery point for repair facility"));
					    }
					    else
					    {
						    // add an assembly point
						    if (!createFlagPosition(&psRepair->psDeliveryPoint, psStruct->player))
						    {
							    ASSERT((FALSE,"checkDeliveryPoints: unable to create new delivery point for repair facility"));
							    return;
						    }
						    addFlagPosition(psRepair->psDeliveryPoint);
						    setFlagPositionInc(psRepair, psStruct->player, REPAIR_FLAG);
						    //initialise the assembly point position
						    x = (psStruct->x+256) >> TILE_SHIFT;
						    y = (psStruct->y+256) >> TILE_SHIFT;
						    // Belt and braces - shouldn't be able to build too near edge
						    //getNearestBestValidTile(&x,&y);
						    setAssemblyPoint( psRepair->psDeliveryPoint, x << TILE_SHIFT, 
							    y << TILE_SHIFT, inc, TRUE);
					    }
				    }
				    else//check existing one
				    {
					    setAssemblyPoint(psRepair->psDeliveryPoint, psRepair->psDeliveryPoint->
						    coords.x, psRepair->psDeliveryPoint->coords.y, inc, TRUE);
				    }
			    }
		    }
        }
	}
}


//adjust the loop quantity for this factory
void factoryLoopAdjust(STRUCTURE *psStruct, BOOL add)
{
	FACTORY		*psFactory;

	ASSERT((StructIsFactory(psStruct), 
		"factoryLoopAdjust: structure is not a factory"));
	ASSERT((psStruct->player == selectedPlayer, 
		"factoryLoopAdjust: should only be called for selectedPlayer"));

	psFactory = (FACTORY *)psStruct->pFunctionality;

	if (add)
	{
		//check for wrapping to infinite production
		if (psFactory->quantity == MAX_IN_RUN)
		{
			psFactory->quantity = 0;
		}
		else
		{
			//increment the count
			psFactory->quantity++;
			//check for limit - this caters for when on infinite production and want to wrap around
			if (psFactory->quantity > MAX_IN_RUN)
			{
				psFactory->quantity = INFINITE_PRODUCTION;
			}
		}
	}
	else
	{
		//decrement the count
		if (psFactory->quantity == 0)
		{
			psFactory->quantity = INFINITE_PRODUCTION;
		}
		else
		{
			psFactory->quantity--;
		}
	}
}

/*This function is called after a game is loaded so that any resource extractors 
that are active are initialised for when to start*/
void checkResExtractorsActive(void)
{
	STRUCTURE		*psStruct;
	UBYTE			inc;


	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		for (psStruct = apsStructLists[inc]; psStruct != NULL; psStruct = 
			psStruct->psNext)
		{
			if (psStruct->pStructureType->type == REF_RESOURCE_EXTRACTOR)
			{
				if (((RES_EXTRACTOR *)psStruct->pFunctionality)->active)
				{
					((RES_EXTRACTOR *)psStruct->pFunctionality)->
						timeLastUpdated = gameTime;
				}
			}
		}
	}
}

/*Used for determining how much of the structure to draw as being built or demolished*/
FRACT structHeightScale(STRUCTURE *psStruct)
{

FRACT	retVal;
	retVal = (MAKEFRACT(psStruct->currentBuildPts)/psStruct->pStructureType->buildPoints);
	if(retVal<0.05f)
	{
		retVal = 0.05f;
	}
	return(retVal);

}


/*compares the structure sensor type with the droid weapon type to see if the 
FIRE_SUPPORT order can be assigned*/
BOOL structSensorDroidWeapon(STRUCTURE *psStruct, DROID *psDroid)
{
	//Standard Sensor Tower + indirect weapon droid (non VTOL)
	//else if (structStandardSensor(psStruct) AND (psDroid->numWeaps AND 
    if (structStandardSensor(psStruct) AND (psDroid->asWeaps[0].nStat > 0 AND 
		!proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat)) AND
		!vtolDroid(psDroid))
	{
		return TRUE;
	}
	//CB Sensor Tower + indirect weapon droid (non VTOL)
	//if (structCBSensor(psStruct) AND (psDroid->numWeaps AND 
    else if (structCBSensor(psStruct) AND (psDroid->asWeaps[0].nStat > 0 AND 
		!proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat)) AND
		!vtolDroid(psDroid))
	{
		return TRUE;
	}
	//VTOL Intercept Sensor Tower + any weapon VTOL droid
	//else if (structVTOLSensor(psStruct) AND psDroid->numWeaps AND
    else if (structVTOLSensor(psStruct) AND psDroid->asWeaps[0].nStat > 0 AND
		vtolDroid(psDroid))
	{
		return TRUE;
	}
	//VTOL CB Sensor Tower + any weapon VTOL droid 
	//else if (structVTOLCBSensor(psStruct) AND psDroid->numWeaps AND
    else if (structVTOLCBSensor(psStruct) AND psDroid->asWeaps[0].nStat > 0 AND
			vtolDroid(psDroid))
	{ 
		return TRUE;
	}

	//case not matched
	return FALSE;
}

/*checks if the structure has a Counter Battery sensor attached - returns 
TRUE if it has*/
BOOL structCBSensor(STRUCTURE *psStruct)
{
	if (psStruct->pStructureType->pSensor)
	{
        //Super Sensor works as any type
		if ((psStruct->pStructureType->pSensor->type == INDIRECT_CB_SENSOR OR
            psStruct->pStructureType->pSensor->type == SUPER_SENSOR) AND
			psStruct->pStructureType->pSensor->location == LOC_TURRET)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*checks if the structure has a Standard Turret sensor attached - returns 
TRUE if it has*/
BOOL structStandardSensor(STRUCTURE *psStruct)
{
	if (psStruct->pStructureType->pSensor)
	{
        /*Super Sensor works as any type*/
		if ((psStruct->pStructureType->pSensor->type == STANDARD_SENSOR OR
            psStruct->pStructureType->pSensor->type == SUPER_SENSOR) AND
			psStruct->pStructureType->pSensor->location == LOC_TURRET)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*checks if the structure has a VTOL Intercept sensor attached - returns 
TRUE if it has*/
BOOL structVTOLSensor(STRUCTURE *psStruct)
{
	if (psStruct->pStructureType->pSensor)
	{
        //Super Sensor works as any type
		if ((psStruct->pStructureType->pSensor->type == VTOL_INTERCEPT_SENSOR OR
            psStruct->pStructureType->pSensor->type == SUPER_SENSOR) AND
			psStruct->pStructureType->pSensor->location == LOC_TURRET)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*checks if the structure has a VTOL Counter Battery sensor attached - returns 
TRUE if it has*/
BOOL structVTOLCBSensor(STRUCTURE *psStruct)
{
	if (psStruct->pStructureType->pSensor)
	{
        //Super Sensor works as any type
		if ((psStruct->pStructureType->pSensor->type == VTOL_CB_SENSOR OR
            psStruct->pStructureType->pSensor->type == SUPER_SENSOR) AND
			psStruct->pStructureType->pSensor->location == LOC_TURRET)
		{
			return TRUE;
		}
	}
	return FALSE;
}

// check whether a rearm pad is clear
BOOL clearRearmPad(STRUCTURE *psStruct)
{
	if ((psStruct->pStructureType->type == REF_REARM_PAD) &&
		( ((REARM_PAD *)psStruct->pFunctionality)->psObj == NULL ||
		  vtolHappy((DROID *)((REARM_PAD *)psStruct->pFunctionality)->psObj) ))
	{
		return TRUE;
	}

	return FALSE;
}

// return the nearest rearm pad
// if bClear is true it tries to find the nearest clear rearm pad in
// the same cluster as psTarget
// psTarget can be NULL
STRUCTURE *	findNearestReArmPad(DROID *psDroid, STRUCTURE *psTarget, BOOL bClear)
{	
	STRUCTURE		*psStruct, *psNearest, *psTotallyClear;
	SDWORD			xdiff,ydiff, mindist, currdist, totallyDist;
	SDWORD			cx,cy;

	if (psTarget != NULL)
	{
		cx = (SDWORD)psTarget->x;
		cy = (SDWORD)psTarget->y;
	}
	else
	{
		cx = (SDWORD)psDroid->x;
		cy = (SDWORD)psDroid->y;
	}

	mindist = SDWORD_MAX;
	totallyDist = SDWORD_MAX;
	psNearest = NULL;
	psTotallyClear = NULL;
	for(psStruct = apsStructLists[psDroid->player]; psStruct; psStruct=psStruct->psNext)
	{
		if ((psStruct->pStructureType->type == REF_REARM_PAD) &&
			(psTarget == NULL || psTarget->cluster == psStruct->cluster) &&
			(!bClear || clearRearmPad(psStruct)))
		{
			xdiff = (SDWORD)psStruct->x - cx;
			ydiff = (SDWORD)psStruct->y - cy;
			currdist = xdiff*xdiff + ydiff*ydiff;
			if (bClear && !vtolOnRearmPad(psStruct, psDroid))
			{
				if (currdist < totallyDist)
				{
					totallyDist = currdist;
					psTotallyClear = psStruct;
				}
			}
			else
			{
				if (currdist < mindist)
				{
					mindist = currdist;
					psNearest = psStruct;
				}
			}
		}
	}

	if (bClear && (psTotallyClear != NULL))
	{
		psNearest = psTotallyClear;
	}

	return psNearest;

}

// clear a rearm pad for a droid to land on it
void ensureRearmPadClear(STRUCTURE *psStruct, DROID *psDroid)
{
	DROID	*psCurr;
	SDWORD	tx,ty;

	tx = psStruct->x >> TILE_SHIFT;
	ty = psStruct->y >> TILE_SHIFT;

	for(psCurr = apsDroidLists[psDroid->player]; psCurr; psCurr=psCurr->psNext)
	{
		if ( (psCurr != psDroid) &&
			 (psCurr->x >> TILE_SHIFT == tx) &&
			 (psCurr->y >> TILE_SHIFT == ty) &&
			 (vtolDroid(psCurr))
			)
		{
			actionDroidObj(psCurr, DACTION_CLEARREARMPAD, (BASE_OBJECT *)psStruct);
		}
	}
}


// return whether a rearm pad has a vtol on it
BOOL vtolOnRearmPad(STRUCTURE *psStruct, DROID *psDroid)
{
	DROID	*psCurr;
	SDWORD	tx,ty;
	BOOL	found;

	tx = psStruct->x >> TILE_SHIFT;
	ty = psStruct->y >> TILE_SHIFT;

	found = FALSE;
	for(psCurr = apsDroidLists[psDroid->player]; psCurr; psCurr=psCurr->psNext)
	{
		if ( (psCurr != psDroid) &&
			 (psCurr->x >> TILE_SHIFT == tx) &&
			 (psCurr->y >> TILE_SHIFT == ty) )
		{
			found = TRUE;
			break;
		}
	}

	return found;
}


/* Just returns true if the structure's present body points aren't as high as the original*/
BOOL	structIsDamaged(STRUCTURE *psStruct)
{
	if(psStruct->body < structureBody(psStruct))
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}

// give a structure from one player to another - used in Electronic Warfare
//returns pointer to the new structure
STRUCTURE * giftSingleStructure(STRUCTURE *psStructure, UBYTE attackPlayer, BOOL bFromScript)
{
    STRUCTURE           *psNewStruct, *psStruct;
    DROID               *psCurr;
    STRUCTURE_STATS     *psType, *psModule;
    UDWORD              x, y;
    UBYTE               capacity = 0, originalPlayer;
    SWORD               buildPoints = 0, i;
    BOOL                bPowerOn;
    UWORD               direction;

#ifdef TEST_EW
    bMultiPlayer = TRUE;
#endif

    //this is not the case for EW in multiPlayer mode
    if (!bMultiPlayer)
    {
        //added 'selectedPlayer == 0' to be able to test the game by changing player...
        //in this version of Warzone, the attack Player can NEVER be the selectedPlayer (unless from the script)
        if (!bFromScript AND selectedPlayer == 0 AND attackPlayer == selectedPlayer)
        {
            ASSERT((FALSE, 
                "giftSingleStructure: EW attack by selectedPlayer on a structure"));
            return NULL;
        }
    }

    //don't want the hassle in multiplayer either
    //and now we do! - AB 13/05/99

    if (bMultiPlayer)
    {
        //certain structures give specific results - the rest swap sides!
        if (!electronicReward(psStructure, attackPlayer))
        {
            originalPlayer = psStructure->player;

            //tell the system the structure no longer exists
            (void)removeStruct(psStructure, FALSE);
            
            // remove structure from one list
            removeStructureFromList(psStructure, apsStructLists);
            
            psStructure->selected = FALSE;
            
            // change player id
	        psStructure->player	= (UBYTE)attackPlayer;

            //restore the resistance value
            psStructure->resistance = (UWORD)structureResistance(psStructure->
                pStructureType, psStructure->player);
            
            // add to other list.
	        addStructure(psStructure);

            //check through the 'attackPlayer' players list of droids to see if any are targetting it
            for (psCurr = apsDroidLists[attackPlayer]; psCurr != NULL; psCurr = psCurr->psNext)
            {
                if (psCurr->psTarget == (BASE_OBJECT *)psStructure OR 
                    psCurr->psActionTarget == (BASE_OBJECT *)psStructure)
                {
                    orderDroid(psCurr, DORDER_STOP);
                }
                //check through order list
                for (i = 0; i < psCurr->listSize; i++)
                {
                    if (psCurr->asOrderList[i].psOrderTarget == (BASE_OBJECT *)psStructure)
                    {
            		    // move the rest of the list down
		                memmove(&psCurr->asOrderList[i], &psCurr->asOrderList[i] + 1, 
                            (psCurr->listSize - i) * sizeof(ORDER_LIST));
                        //adjust list size
                        psCurr->listSize -= 1;
                        //initialise the empty last slot
		                memset(psCurr->asOrderList + psCurr->listSize, 0, 
                            sizeof(ORDER_LIST));
                    }
                }
            }
            //check through the 'attackPlayer' players list of structures to see if any are targetting it
            for (psStruct = apsStructLists[attackPlayer]; psStruct != NULL; psStruct = 
                psStruct->psNext)
            {
                if (psStruct->psTarget == (BASE_OBJECT *)psStructure)
                {
                    psStruct->psTarget = NULL;
                }
            }


            //add back into cluster system
            clustNewStruct(psStructure);

            //add back into the grid system
            gridAddObject((BASE_OBJECT *)psStructure);

            if (psStructure->status == SS_BUILT)
            {
                buildingComplete(psStructure);
            }
            //since the structure isn't being rebuilt, the visibility code needs to be adjusted
            if (attackPlayer == selectedPlayer)
            {
                //make sure this structure is visible to selectedPlayer
                psStructure->visible[selectedPlayer] = UBYTE_MAX;
                //make sure the tiles don't get drawn
                setStructTileDraw(psStructure);
            }                
        }

#ifdef TEST_EW
        bMultiPlayer = FALSE;
#endif

        //ASSERT((FALSE,
        //    "giftSingleStructure: EW attack in multiplayer"));
        return NULL;
    }


    //save info about the structure
    psType = psStructure->pStructureType;
    x = psStructure->x;
    y = psStructure->y;
    direction = psStructure->direction;
    originalPlayer = psStructure->player;
    //save how complete the build process is
    if (psStructure->status == SS_BEING_BUILT)
    {
        buildPoints = psStructure->currentBuildPts;
    }
    //check module not attached
    psModule = getModuleStat(psStructure);
    if (psModule)
    {
        switch(psType->type)
        {
        case REF_POWER_GEN:
            capacity = (UBYTE)((POWER_GEN *)psStructure->pFunctionality)->capacity;
            break;
        case REF_RESEARCH:
            capacity = (UBYTE)((RESEARCH_FACILITY *)psStructure->pFunctionality)->capacity;
            break;
        case REF_FACTORY:
        case REF_VTOL_FACTORY:
            capacity = ((FACTORY *)psStructure->pFunctionality)->capacity;
            break;
        //no default case cos don't care!
        }
    }
    //get rid of the structure
    (void)removeStruct(psStructure, TRUE);

    //make sure power is not used to build
    bPowerOn = powerCalculated;
    powerCalculated = FALSE;
    //build a new one for the attacking player - set last element to TRUE so it doesn't adjust x/y
    psNewStruct = buildStructure(psType, x, y, attackPlayer, TRUE);
    if (psNewStruct)
    {
        psNewStruct->direction = direction;
        if (capacity)
        {
            switch(psType->type)
            {
            case REF_POWER_GEN:
            case REF_RESEARCH:
                //build the module for powerGen and research
                buildStructure(psModule, psNewStruct->x, psNewStruct->y, 
                    attackPlayer, FALSE);
                break;
            case REF_FACTORY:
            case REF_VTOL_FACTORY:
                //build the appropriate number of modules
                while (capacity)
                {
                    buildStructure(psModule, psNewStruct->x, psNewStruct->y, 
                        attackPlayer, FALSE);
                    capacity--;
                }
                break;
            }
        }
        if (buildPoints)
        {
            psNewStruct->status = SS_BEING_BUILT;
            psNewStruct->currentBuildPts = buildPoints;
        }
        else
        {
            psNewStruct->status = SS_BUILT;
            buildingComplete(psNewStruct);
        }

        if (!bMultiPlayer)

        {
            //inform selectedPlayer that takeover has happened
            if (originalPlayer == selectedPlayer)
            {
                if (wallDefenceStruct(psNewStruct->pStructureType))
                {

			        audio_QueueTrackPos( ID_SOUND_NEXUS_DEFENCES_ABSORBED,
					    psNewStruct->x, psNewStruct->y, psNewStruct->z );

                }
                else
                {

			        audio_QueueTrackPos( ID_SOUND_NEXUS_STRUCTURE_ABSORBED,
					    psNewStruct->x, psNewStruct->y, psNewStruct->z );

                }
                //make sure this structure is visible to selectedPlayer if the structure used to be selectedPlayers'
                psNewStruct->visible[selectedPlayer] = UBYTE_MAX;
                //make sure the tiles don't get drawn
                setStructTileDraw(psNewStruct);
            }
        }
    }
    powerCalculated = bPowerOn;
    return psNewStruct;
}
 /* Code to have the structure's weapon assembly rock back upon firing */
void	structUpdateRecoil( STRUCTURE *psStruct )
{
UDWORD	percent;
UDWORD	recoil;
FRACT	fraction;

	/* Check it's actually got a weapon */
    if(psStruct->asWeaps[0].nStat == 0)
	{
		return;
	}

	/* We have a weapon */
	if(gameTime > (psStruct->asWeaps[0].lastFired + STR_RECOIL_TIME) ) 
	{
		/* Recoil effect is over */
		psStruct->asWeaps[0].recoilValue = 0;
		return;
	}

	/* Where should the assembly be? */
	percent = PERCENT((gameTime-psStruct->asWeaps[0].lastFired),STR_RECOIL_TIME);

	/* Outward journey */
	if(percent >= 50)
	{
		recoil = (100 - percent)/5;
	}
	/* Return journey */
	else
	{
		recoil = percent/5;
	}

	fraction = 	
		MAKEFRACT(asWeaponStats[psStruct->asWeaps[0].nStat].recoilValue)/
		(MAKEFRACT(100));

	recoil = MAKEINT( MAKEFRACT(recoil) * fraction);

	/* Put it into the weapon data */
	psStruct->asWeaps[0].recoilValue = recoil;
}

/*checks that the structure stats have loaded up as expected - must be done after 
all StructureStats parts have been loaded*/
BOOL checkStructureStats(void)
{
	UDWORD	structInc, inc;

	for (structInc=0; structInc < numStructureStats; structInc++)
	{
        if (asStructureStats[structInc].numFuncs != 0)
        {
            for (inc = 0; inc < asStructureStats[structInc].numFuncs; inc++)
            {

                ASSERT((PTRVALID(asStructureStats[structInc].asFuncList[inc], 
                    sizeof(FUNCTION *)),"checkStructureStats: \
                    Invalid function for structure %s", 
                    asStructureStats[structInc].pName));

            }
        }
        else
        {
            if (asStructureStats[structInc].asFuncList != NULL)
            {

                ASSERT((FALSE, "checkStructureStats:Invalid functions attached to structure %s", 
                    asStructureStats[structInc].pName));

                return FALSE;
            }
        }
	}
	return TRUE;
}

/*returns the power cost to build this structure*/
UDWORD  structPowerToBuild(STRUCTURE *psStruct)
{
    STRUCTURE_STATS     *psStat;
    UBYTE               capacity;

    //see if this structure can have a module attached
    psStat = getModuleStat(psStruct);
    if (psStat)
    {
        capacity = 0;
        //are we building the module or the base structure?
        switch(psStruct->pStructureType->type)
        {
        case REF_POWER_GEN:
            capacity = (UBYTE)((POWER_GEN *)psStruct->pFunctionality)->capacity;
            break;
        case REF_RESEARCH:
            capacity = (UBYTE)((RESEARCH_FACILITY *)psStruct->pFunctionality)->capacity;
            break;
        case REF_FACTORY:
        case REF_VTOL_FACTORY:
            capacity = ((FACTORY *)psStruct->pFunctionality)->capacity;
            break;
        //no default case cos don't care!
        }
        if (capacity)
        {
            //return the cost to build the module
            return psStat->powerToBuild;
        }
        else
        {
            //no module attached so building the base structure
            return psStruct->pStructureType->powerToBuild;
        }
    }
    else
    {
        //not a module structure, so power cost is the one associated with the stat
        return psStruct->pStructureType->powerToBuild;
    }
}

//for MULTIPLAYER ONLY
//this adjusts the time the relevant action started if the building is attacked by EW weapon
void resetResistanceLag(STRUCTURE *psBuilding)
{
    if (bMultiPlayer)
    {
        switch (psBuilding->pStructureType->type)
        {
            case REF_RESEARCH:
            {
                RESEARCH_FACILITY *psResFacility = (RESEARCH_FACILITY*)
                    psBuilding->pFunctionality;
                
                //if working on a topic
                if (psResFacility->psSubject)
                {
	    	        //adjust the start time for the current subject
		            if (psResFacility->timeStarted != ACTION_START_TIME)
		            {
			            psResFacility->timeStarted += (gameTime - psBuilding->lastResistance);
		            }
                }
            }
            case REF_FACTORY:
            case REF_VTOL_FACTORY:
            case REF_CYBORG_FACTORY:
            {
                FACTORY    *psFactory = (FACTORY *)psBuilding->pFunctionality;

                //if working on a unit
                if (psFactory->psSubject)
                {
	    	        //adjust the start time for the current subject
		            if (psFactory->timeStarted != ACTION_START_TIME)
		            {
			            psFactory->timeStarted += (gameTime - psBuilding->lastResistance);
		            }
                }
            }
            /*
            case REF_REPAIR_FACILITY: - this is catered for in the aiUpdateStructure function
            case REF_REARM_PAD: - this structure is taken over completely
            */
            //default: //do nothing
        }
    }
}

/*reveals all the terrain in the map*/
void revealAll(UBYTE player)
{
    UWORD   i, j;
    MAPTILE	*psTile;

    //reveal all tiles
	for(i=0; i<mapWidth; i++)
	{
		for(j=0; j<mapHeight; j++)
		{
			psTile = mapTile(i,j);
            SET_TILE_VISIBLE(player, psTile);
            avInformOfChange(i,j);
        }
    }

    //the objects gets revealed in processVisibility()
}

/*checks the structure passed in is a Las Sat structure which is currently 
selected - returns TRUE if valid*/
BOOL lasSatStructSelected(STRUCTURE *psStruct)
{
    if ( (psStruct->selected OR (bMultiPlayer && !isHumanPlayer(psStruct->player)))
		AND psStruct->asWeaps[0].nStat 
		AND (asWeaponStats[psStruct->asWeaps[0].nStat].weaponSubClass == WSC_LAS_SAT))
    {
        return TRUE;
    }


    return FALSE;

}

/******************** demo stuff ************************/
/*
#ifdef DEMO

void printAvailStructs(void)
{
	UDWORD	stNum;

	for (stNum = 0; stNum<numStructureStats; stNum++)
	{
		if (apStructTypeLists[0][stNum] == AVAILABLE)
		{
			DBPRINTF(("%s available\n", asStructureStats[stNum].pName));
		}
	}
}

//function to create the structures necessary for the demo. Allocates the IMD models
BOOL demoStructs(void)
{
	UDWORD				count;
	STRUCTURE_STATS		*psBuilding = asStructureStats;
	STRUCTURE_STATS		*psNewStructure = asStructureStats + numStructureStats;

	//get the stats for a wall
	for (count=0; count < numStructureStats; count++, psBuilding++)
	{
		if (psBuilding->type == REF_WALL)
		{
			//create Corner Wall 1
			if (!createStructureStat(psBuilding, psNewStructure, REF_STRUCTURE_START + 
				numStructureStats, "Corner Wall 1", REF_CORNER1))
			{
				return FALSE;
			}
			//set the IMD for the structure
			psNewStructure->pIMD = (iIMDShape *) resGetData("IMD", "blbrbcr1.imd");
			if (psNewStructure->pIMD == NULL)
			{
				DBERROR(("Cannot find the structure IMD for record %s", 
					psNewStructure->pName));
				return FALSE;
			}
			psNewStructure++;

			//create Corner Wall 2
			if (!createStructureStat(psBuilding, psNewStructure, REF_STRUCTURE_START + 
				numStructureStats + 1, "Corner Wall 2", REF_CORNER2))
			{
				return FALSE;
			}
			//set the IMD for the structure
			psNewStructure->pIMD = (iIMDShape *) resGetData("IMD", "blbrbcr2.imd");
			if (psNewStructure->pIMD == NULL)
			{
				DBERROR(("Cannot find the structure IMD for record %s", 
					psNewStructure->pName));
				return FALSE;
			}
			psNewStructure++;

			//create Corner Wall 3
			if (!createStructureStat(psBuilding, psNewStructure, REF_STRUCTURE_START + 
				numStructureStats + 2, "Corner Wall 3", REF_CORNER3))
			{
				return FALSE;
			}
			//set the IMD for the structure
			psNewStructure->pIMD = (iIMDShape *) resGetData("IMD", "blbrbcr3.imd");
			if (psNewStructure->pIMD == NULL)
			{
				DBERROR(("Cannot find the structure IMD for record %s", 
					psNewStructure->pName));
				return FALSE;
			}
			psNewStructure++;

			//Create Corner Wall 4
			if (!createStructureStat(psBuilding, psNewStructure, REF_STRUCTURE_START + 
				numStructureStats + 3, "Corner Wall 4", REF_CORNER4))
			{
				return FALSE;
			}
			//set the IMD for the structure
			psNewStructure->pIMD = (iIMDShape *) resGetData("IMD", "blbrbcr4.imd");
			if (psNewStructure->pIMD == NULL)
			{
				DBERROR(("Cannot find the structure IMD for record %s", 
					psNewStructure->pName));
				return FALSE;
			}
			psNewStructure++;

			//create Gate1
			if (!createStructureStat(psBuilding, psNewStructure, REF_STRUCTURE_START + 
				numStructureStats + 4, "Gate 1", REF_GATE1))
			{
				return FALSE;
			}
			//set the IMD for the structure
			psNewStructure->pIMD = (iIMDShape *) resGetData("IMD", "blbrbgt1.imd");
			if (psNewStructure->pIMD == NULL)
			{
				DBERROR(("Cannot find the structure IMD for record %s", 
					psNewStructure->pName));
				return FALSE;
			}
			psNewStructure++;

			//create Gate2
			if (!createStructureStat(psBuilding, psNewStructure, REF_STRUCTURE_START + 
				numStructureStats + 5, "Gate 2", REF_GATE2))
			{
				return FALSE;
			}
			//set the IMD for the structure
			psNewStructure->pIMD = (iIMDShape *) resGetData("IMD", "blbrbgt2.imd");
			if (psNewStructure->pIMD == NULL)
			{
				DBERROR(("Cannot find the structure IMD for record %s", 
					psNewStructure->pName));
				return FALSE;
			}
			psNewStructure++;

			//create Gate3
			if (!createStructureStat(psBuilding, psNewStructure, REF_STRUCTURE_START + 
				numStructureStats + 6, "Gate 3", REF_GATE3))
			{
				return FALSE;
			}
			//set the IMD for the structure
			psNewStructure->pIMD = (iIMDShape *) resGetData("IMD", "blbrbgt3.imd");
			if (psNewStructure->pIMD == NULL)
			{
				DBERROR(("Cannot find the structure IMD for record %s", 
					psNewStructure->pName));
				return FALSE;
			}
			psNewStructure++;

			//create Gate4
			if (!createStructureStat(psBuilding, psNewStructure, REF_STRUCTURE_START + 
				numStructureStats + 7, "Gate 4", REF_GATE4))
			{
				return FALSE;
			}
			//set the IMD for the structure
			psNewStructure->pIMD = (iIMDShape *) resGetData("IMD", "blbrbgt4.imd");
			if (psNewStructure->pIMD == NULL)
			{
				DBERROR(("Cannot find the structure IMD for record %s", 
					psNewStructure->pName));
				return FALSE;
			}
			psNewStructure++;

			break;
		}
	}

	psBuilding = asStructureStats;
	//get the stats for a defense structure
	for (count=0; count < numStructureStats; count++, psBuilding++)
	{
		if (strcmp(psBuilding->pName, "Gun Tower") == 0)
		{
			//create Corner Tower 1
			if (!createStructureStat(psBuilding, psNewStructure, REF_STRUCTURE_START + 
				numStructureStats + 8, "Corner Tower 1", REF_TOWER1))
			{
				return FALSE;
			}
			//set the IMD for the structure
			psNewStructure->pIMD = (iIMDShape *) resGetData("IMD", "blbrbtw1.imd");
			if (psNewStructure->pIMD == NULL)
			{
				DBERROR(("Cannot find the structure IMD for record %s", 
					psNewStructure->pName));
				return FALSE;
			}
			psNewStructure++;

			//create Corner Tower 2
			if (!createStructureStat(psBuilding, psNewStructure, REF_STRUCTURE_START + 
				numStructureStats + 9, "Corner Tower 2", REF_TOWER2))
			{
				return FALSE;
			}
			//set the IMD for the structure
			psNewStructure->pIMD = (iIMDShape *) resGetData("IMD", "blbrbtw2.imd");
			if (psNewStructure->pIMD == NULL)
			{
				DBERROR(("Cannot find the structure IMD for record %s", 
					psNewStructure->pName));
				return FALSE;
			}
		psNewStructure++;

			//create Corner Tower 3
			if (!createStructureStat(psBuilding, psNewStructure, REF_STRUCTURE_START + 
				numStructureStats + 10, "Corner Tower 3", REF_TOWER3))
			{
				return FALSE;
			}
			//set the IMD for the structure
			psNewStructure->pIMD = (iIMDShape *) resGetData("IMD", "blbrbtw3.imd");
			if (psNewStructure->pIMD == NULL)
			{
				DBERROR(("Cannot find the structure IMD for record %s", 
					psNewStructure->pName));
				return FALSE;
			}
			psNewStructure++;

			//create Corner Tower 4
			if (!createStructureStat(psBuilding, psNewStructure, REF_STRUCTURE_START + 
				numStructureStats + 11, "Corner Tower 4", REF_TOWER4))
			{
				return FALSE;
			}
			//set the IMD for the structure
			psNewStructure->pIMD = (iIMDShape *) resGetData("IMD", "blbrbtw4.imd");
			if (psNewStructure->pIMD == NULL)
			{
				DBERROR(("Cannot find the structure IMD for record %s", 
					psNewStructure->pName));
				return FALSE;
			}
			psNewStructure++;
			break;
		}
	}
	
	numStructureStats += NUM_DEMO_STRUCTS;

	return TRUE;
}

BOOL createStructureStat(STRUCTURE_STATS *psBuilding, STRUCTURE_STATS *psNewStructure,
						 UDWORD ref, STRING *pName, UDWORD type)
{
	UDWORD		i;

	memcpy(psNewStructure, psBuilding, sizeof(STRUCTURE_STATS));
	//change the name, ref and type
	psNewStructure->ref = ref;
	psNewStructure->type = type;

	//allocate storage for the name
	psNewStructure->pName = (STRING *) MALLOC(MAX_NAME_SIZE);
	if (psNewStructure->pName == NULL)
	{
		DBERROR(("Structure Stats Name - Out of memory"));
		return FALSE;
	}	
	strcpy(psNewStructure->pName, pName);

	//allocate storage for the weapons and functions - if any
	if (psNewStructure->numWeaps > 0)
	{
		psNewStructure->asWeapList = (WEAPON_STATS **)MALLOC(psNewStructure->
			weaponSlots * sizeof(WEAPON_STATS*));
		if (psNewStructure->asWeapList == NULL)
		{
			DBERROR(("Out of memory assigning structure weapons"));
			return FALSE;
		}
		//copy the stats across
		for (i = 0; i < psNewStructure->numWeaps; i++)
		{
			psNewStructure->asWeapList[i] = psBuilding->asWeapList[i];
		}
	}
	//allocate storage for the functions - if any
	if (psNewStructure->numFuncs > 0)
	{
		psNewStructure->asFuncList = (FUNCTION **)MALLOC(psNewStructure->
			numFuncs * sizeof(FUNCTION*));
		if (psNewStructure->asFuncList == NULL)
		{
			DBERROR(("Out of memory assigning structure Functions"));
			return FALSE;
		}
		//copy the stats across
		for (i = 0; i < psNewStructure->numFuncs; i++)
		{
			psNewStructure->asFuncList[i] = psBuilding->asFuncList[i];
		}
	}
	return TRUE;
}

#endif
  */
