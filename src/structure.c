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
/*!
 * \file structure.c
 *
 * Store Structure stats.
 * WARNING!!!!!!
 * Something wicked this way comes. This file is almost as evil as HCI.c
 */
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/frameresource.h"
#include "objects.h"
#include "ai.h"
#include "map.h"
#include "lib/gamelib/gtime.h"
#include "visibility.h"
#include "structure.h"
#include "research.h"
#include "hci.h"
#include "power.h"
#include "miscimd.h"
#include "effects.h"
#include "combat.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "stats.h"
#include "lib/framework/math-help.h"
#include "edit3d.h"
#include "anim_id.h"
#include "lib/gamelib/anim.h"
#include "display3d.h"
#include "geometry.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"
#include "order.h"
#include "droid.h"
#include "lib/script/script.h"
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
#include "lib/netplay/netplay.h"
#include "multigifts.h"
#include "loop.h"

#include "scores.h"
#include "gateway.h"

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

//specifies which numbers have been allocated for the assembly points for the factories
UBYTE				factoryNumFlag[MAX_PLAYERS][NUM_FLAG_TYPES];

// the number of different (types of) droids that can be put into a production run
#define MAX_IN_RUN		9

//the list of what to build - only for selectedPlayer
PRODUCTION_RUN		asProductionRun[NUM_FACTORY_TYPES][MAX_FACTORY][MAX_PROD_RUN];

//stores which player the production list has been set up for
SBYTE               productionPlayer;

/* destroy building construction droid stat pointer */
static	STRUCTURE_STATS	*g_psStatDestroyStruct = NULL;

// the structure that was last hit
STRUCTURE	*psLastStructHit;

//flag for drawing radar
static		UBYTE	hqExists[MAX_PLAYERS];
//flag for drawing all sat uplink sees
static		UBYTE	satUplinkExists[MAX_PLAYERS];
//flag for when the player has one built - either completely or partially
static		UBYTE	lasSatExists[MAX_PLAYERS];

static BOOL setFunctionality(STRUCTURE* psBuilding, UDWORD functionType);
static void setFlagPositionInc(FUNCTIONALITY* pFunctionality, UDWORD player, UBYTE factoryType);
static void informPowerGen(STRUCTURE *psStruct);
static BOOL electronicReward(STRUCTURE *psStructure, UBYTE attackPlayer);
static void factoryReward(UBYTE losingPlayer, UBYTE rewardPlayer);
static void repairFacilityReward(UBYTE losingPlayer, UBYTE rewardPlayer);
static void findAssemblyPointPosition(UDWORD *pX, UDWORD *pY, UDWORD player);
static void removeStructFromMap(STRUCTURE *psStruct);
static void	structUpdateRecoil( STRUCTURE *psStruct );
static void resetResistanceLag(STRUCTURE *psBuilding);
static void revealAll(UBYTE player);
static void cbNewDroid(STRUCTURE *psFactory, DROID *psDroid);


// last time the maximum units message was displayed
static UDWORD	lastMaxUnitMessage;

#define MAX_UNIT_MESSAGE_PAUSE 20000


/* New function from Alex M */
/* Tells you if a point is inside the footprint of a building */
BOOL	ptInStructure(STRUCTURE *psStruct, UDWORD x, UDWORD y)
{
	UDWORD tlX, tlY, brX, brY;
	UDWORD width, height;

	CHECK_STRUCTURE(psStruct);

	width = (psStruct->pStructureType->baseWidth * TILE_UNITS);
	height = (psStruct->pStructureType->baseBreadth * TILE_UNITS);


	tlX = psStruct->pos.x - (width/2);
	tlY = psStruct->pos.y - (height/2);

	brX = psStruct->pos.x + (width/2);
	brY = psStruct->pos.y + (height/2);

	if (x > tlX && x < brX && y > tlY && y < brY)
		return(true);
	return(false);
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
	// If the stat is any of the 3 expansion types ... then return true
	if(	psStats->type == REF_POWER_MODULE  ||
		psStats->type == REF_FACTORY_MODULE  ||
		psStats->type == REF_RESEARCH_MODULE )
		{
			return true;
		}
		else
		{
			return false;
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

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		asStructLimits[i] = NULL;
		for (j = 0; j < NUM_FLAG_TYPES; j++)
		{
			factoryNumFlag[i][j] = 0;
		}
	}
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		hqExists[i] = false;
		satUplinkExists[i] = false;
		lasSatExists[i] = false;
	}
	//initialise the selectedPlayer's production run
	memset(&asProductionRun, 0, sizeof(PRODUCTION_RUN) * NUM_FACTORY_TYPES *
		MAX_FACTORY * MAX_PROD_RUN);
	//set up at beginning of game which player will have a production list
	productionPlayer = (SBYTE)selectedPlayer;
}

/*Initialise the production list and set up the production player*/
void changeProductionPlayer(UBYTE player)
{
	//clear the production run
	memset(&asProductionRun, 0, sizeof(PRODUCTION_RUN) * NUM_FACTORY_TYPES *
		MAX_FACTORY * MAX_PROD_RUN);
	//set this player to have the production list
	productionPlayer = player;
}


/*initialises the flag before a new data set is loaded up*/
void initFactoryNumFlag(void)
{
	UDWORD i, j;

	for(i=0; i< MAX_PLAYERS; i++)
	{
		//initialise the flag
		for (j=0; j < NUM_FLAG_TYPES; j++)
		{
			factoryNumFlag[i][j] = (UBYTE)0;
		}
	}
}

//called at start of missions
void resetFactoryNumFlag(void)
{
	STRUCTURE*   psStruct;
	uint8_t      mask = 0;
	unsigned int i;

	for(i = 0; i < MAX_PLAYERS; i++)
	{
		//look throu the list of structures to see which have been used
		for (psStruct = apsStructLists[i]; psStruct != NULL; psStruct = psStruct->psNext)
		{
			if (psStruct->pStructureType->type == REF_FACTORY
			 || psStruct->pStructureType->type == REF_CYBORG_FACTORY
			 || psStruct->pStructureType->type == REF_VTOL_FACTORY)
			{
				FACTORY* psFactory = &psStruct->pFunctionality->factory;
				if (psFactory->psAssemblyPoint)
				{
					mask = (1 << psFactory->psAssemblyPoint->factoryInc);
				}
			}

			if (psStruct->pStructureType->type == REF_FACTORY)
					factoryNumFlag[i][FACTORY_FLAG] |= mask;
			else if (psStruct->pStructureType->type == REF_CYBORG_FACTORY)
					factoryNumFlag[i][CYBORG_FLAG] |= mask;
			else if (psStruct->pStructureType->type == REF_VTOL_FACTORY)
					factoryNumFlag[i][VTOL_FLAG] |= mask;
		}
	}
}

static void structureType(STRUCTURE_STATS *pStructure, char *pType)
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
	ASSERT(!"unknown structure type", "structureType: Unknown Structure Type");
}


static const char *getStructName(STRUCTURE_STATS *psStruct)
{
	return getName(psStruct->pName);
}

/*returns the structure strength based on the string name passed in */
static UBYTE getStructStrength(const char *pStrength)
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

	return INVALID_STRENGTH;
}

static void initModulePIEs(char *PIEName,UDWORD i,STRUCTURE_STATS *psStructure)
{
	char GfxFile[MAX_STR_LENGTH];
	char charNum[2];
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
					debug( LOG_ERROR, "Cannot find the PIE for factory module %d - %s", module, GfxFile );
					abort();
					return;
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
					debug( LOG_ERROR, "Cannot find the PIE for vtol factory module %d - %s", module, GfxFile );
					abort();
					return;
				}
			}
		}

		// Setup the PIE's for the research modules.
		if (psStructure->type == REF_RESEARCH_MODULE)
		{
			length = strlen(GfxFile) - 5;
			GfxFile[length] = '4';

			researchModuleIMDs[0] = (iIMDShape*) resGetData("IMD", GfxFile);
			if (researchModuleIMDs[0] == NULL)
			{
				debug( LOG_ERROR, "Cannot find the PIE for research module %d - %s", module, GfxFile );
				abort();
				return;
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

			GfxFile[length] = '4';

			powerModuleIMDs[0] = (iIMDShape*) resGetData("IMD", GfxFile);
			if (powerModuleIMDs[0] == NULL)
			{
				debug( LOG_ERROR, "Cannot find the PIE for power module %d - %s", module, GfxFile );
				abort();
				return;
			}

			powerModuleIMDs[1] = powerModuleIMDs[0];
			powerModuleIMDs[2] = powerModuleIMDs[0];
			powerModuleIMDs[3] = powerModuleIMDs[0];

			//store the stat for easy access later on
			powerModuleStat = i;
		}
}

/* load the Structure stats from the Access database */
BOOL loadStructureStats(const char *pStructData, UDWORD bufferSize)
{
	const unsigned int NumStructures = numCR(pStructData, bufferSize);
	UDWORD i, inc, player, numWeaps, weapSlots;
	char				StructureName[MAX_STR_LENGTH], foundation[MAX_STR_LENGTH],
						type[MAX_STR_LENGTH], techLevel[MAX_STR_LENGTH],
						strength[MAX_STR_LENGTH];
	char				GfxFile[MAX_STR_LENGTH], baseIMD[MAX_STR_LENGTH];
	char				ecmType[MAX_STR_LENGTH], sensorType[MAX_STR_LENGTH];
	STRUCTURE_STATS		*psStructure, *pStartStats;
	ECM_STATS*			pECMType;
	SENSOR_STATS*		pSensorType;
	UDWORD				module;
	UDWORD				iID;
	UDWORD              dummyVal;

#if (MAX_PLAYERS != 8)
	char NotUsedString[MAX_STR_LENGTH];
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

	asStructureStats = (STRUCTURE_STATS*)malloc(sizeof(STRUCTURE_STATS)* NumStructures);
	numStructureStats = NumStructures;

	if (asStructureStats == NULL)
	{
		debug( LOG_ERROR, "Structure Stats - Out of memory" );
		abort(); // FIXME exit(EXIT_FAILURE)?
		return false;
	}

	//save the starting address
	pStartStats = asStructureStats;

	//get the start of the structure_stats storage
	psStructure = asStructureStats;

	for (i = 0; i < NumStructures; i++)
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
			&dummyVal, &psStructure->resistance,
			&dummyVal, &psStructure->sizeModifier,
			ecmType, sensorType, &weapSlots, GfxFile,
			baseIMD, &psStructure->numFuncs, &numWeaps);

#if MAX_PLAYERS != 4 && MAX_PLAYERS != 8
# error Invalid number of players
#endif

		//allocate storage for the name
		if (!allocateName(&psStructure->pName, StructureName))
		{
			return false;
		}

		psStructure->ref = REF_STRUCTURE_START + i;

		//determine the structure type
		structureType(psStructure, type);

		//determine the tech level
		if (!setTechLevel((BASE_STATS *)psStructure, techLevel))
		{
			return false;
		}

		//set the struct strength
		psStructure->strength = getStructStrength(strength);
		if (psStructure->strength == INVALID_STRENGTH)
		{
			debug( LOG_ERROR, "loadStructureStats: Unknown structure strength for %s", getStatName(psStructure) );
			abort();
			return false;
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
				return false;
			}

			for (inc=0; inc < numECMStats; inc++)
			{
				//compare the names
				if (!strcmp(ecmType, pECMType->pName))
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
				return false;
			}
			pSensorType = asSensorStats;
			for (inc=0; inc < numSensorStats; inc++)
			{
				//compare the names
				if (!strcmp(sensorType, pSensorType->pName))
				{
					psStructure->pSensor = pSensorType;
					break;
				}
				pSensorType++;
			}
			//check not allocating a turret sensor if have weapons attached
			ASSERT( psStructure->pSensor != NULL,
				"loadStructureStats: should have a sensor attached to %s!", StructureName );
			if (psStructure->pSensor->location == LOC_TURRET && numWeaps)
			{
				debug( LOG_ERROR, "loadStructureStats: a Turret Sensor and weapon \
					have been assigned to %s", StructureName );
				abort();
			}
		}

		//get the IMD for the structure
		psStructure->pIMD = (iIMDShape *) resGetData("IMD", GfxFile);
		if (psStructure->pIMD == NULL)
		{
			debug( LOG_ERROR, "Cannot find the structure PIE for record %s", getStructName(psStructure) );
			abort();
			return false;
		}

		if (strcmp(baseIMD, "0"))
		{
			psStructure->pBaseIMD = (iIMDShape *) resGetData("IMD", baseIMD);
			if (psStructure->pIMD == NULL)
			{
				debug( LOG_ERROR, "Cannot find the structure base PIE for record %s", getStructName(psStructure) );
				abort();
				return false;
			}
		}
		else
		{
			psStructure->pBaseIMD = NULL;
		}

		initModulePIEs(GfxFile,i,psStructure);

		//Only having one weapon per structure now...AB 24/01/99
		if (weapSlots > STRUCT_MAXWEAPS || numWeaps > weapSlots)
		{
			debug( LOG_ERROR, "Allocated more weapons than allowed for Structure" );
			abort();
			return false;
		}
		//Watermelon:I need numWeaps to draw multiple weapons
		psStructure->numWeaps = numWeaps;

		//allocate storage for the functions - if any
		psStructure->defaultFunc = -1;
		if (psStructure->numFuncs > 0)
		{
			psStructure->asFuncList = (FUNCTION **)malloc(psStructure->numFuncs *
				sizeof(FUNCTION*));
			if (psStructure->asFuncList == NULL)
			{
				debug( LOG_ERROR, "Out of memory assigning structure Functions" );
				abort();
				return false;
			}
		}
		//increment the pointer to the start of the next record
		pStructData = strchr(pStructData,'\n') + 1;
		//increment the list to the start of the next storage block
		psStructure++;
	}

	asStructureStats = pStartStats;

	/* get global dummy stat pointer - GJ */
	for (iID = 0; iID < numStructureStats; iID++)
	{
		if (asStructureStats[iID].type == REF_DEMOLISH)
		{
			break;
		}
	}
	if (iID > numStructureStats)
	{
		debug( LOG_ERROR, "intAddObjectStats: destroy structure stat not found\n" );
		abort();
	}
	g_psStatDestroyStruct = asStructureStats + iID;

	//allocate the structureLimits structure
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		asStructLimits[player] = malloc(sizeof(STRUCTURE_LIMITS) * numStructureStats);
		if (asStructLimits[player] == NULL)
		{
			debug( LOG_ERROR, "Unable to allocate structure limits" );
			abort();
			return false;
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

	return true;
}

//initialise the structure limits structure
void initStructLimits(void)
{
	UDWORD				i, player;

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		STRUCTURE_LIMITS	*psStructLimits = asStructLimits[player];
		STRUCTURE_STATS		*psStat = asStructureStats;

		for (i = 0; i < numStructureStats; i++)
		{
			psStructLimits[i].limit = LOTS_OF;
			psStructLimits[i].currentQuantity = 0;
			psStructLimits[i].globalLimit = LOTS_OF;
			if (isLasSat(psStat) || psStat->type == REF_SAT_UPLINK)
			{
				psStructLimits[i].limit = 1;
				psStructLimits[i].globalLimit = 1;
			}
			psStat++;
		}
	}
}

/* set the current number of structures of each type built */
void setCurrentStructQuantity(BOOL displayError)
{
	UDWORD		player, inc;

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		STRUCTURE_LIMITS	*psStructLimits = asStructLimits[player];
		STRUCTURE		*psCurr;

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
					ASSERT( false, "There appears to be too many %s on this map!",
						getStructName(&asStructureStats[inc] ) );
				}
			}
		}
	}
}

//Load the weapons assigned to Structure in the Access database
BOOL loadStructureWeapons(const char *pWeaponData, UDWORD bufferSize)
{
	const unsigned int NumToAlloc = numCR(pWeaponData, bufferSize);
	UDWORD				i, incS, incW;
	char				StructureName[MAX_STR_LENGTH];//, WeaponName[MAX_STR_LENGTH];
	char				WeaponName[STRUCT_MAXWEAPS][MAX_STR_LENGTH];
	STRUCTURE_STATS		*pStructure = asStructureStats;
	WEAPON_STATS		*pWeapon = asWeaponStats;
	BOOL				weaponFound, structureFound;
	UBYTE				j;

	for (i=0; i < NumToAlloc; i++)
	{
		//read the data into the storage - the data is delimeted using comma's
		StructureName[0] = '\0';
		for (j = 0;j < STRUCT_MAXWEAPS;j++)
		{
			WeaponName[j][0] = '\0';
		}

		sscanf(pWeaponData, "%[^','],%[^','],%[^','],%[^','],%[^','],%*d", StructureName, WeaponName[0], WeaponName[1], WeaponName[2], WeaponName[3]);

		if (!getResourceName(StructureName))
		{
			return false;
		}
		if (!getResourceName(WeaponName[0]))
		{
			return false;
		}
		weaponFound = structureFound = false;
		//loop through each Structure_Stat to compare the name

		for (incS=0; incS < numStructureStats; incS++)
		{
			if (!(strcmp(StructureName, pStructure[incS].pName)))
			{
				//Structure found, so loop through each weapon
				structureFound = true;
				for (j = 0;j < pStructure[incS].numWeaps;j++)
				{
					for (incW=0; incW < numWeaponStats; incW++)
					{
						if (!(strcmp(WeaponName[j], pWeapon[incW].pName)))
						{
							weaponFound = true;

							//Watermelon:read and store multiple weapon Stats
							pStructure[incS].psWeapStat[j] = &pWeapon[incW];
							break;
						}
					}
					//if weapon not found - error
					if (!weaponFound)
					{
						debug( LOG_ERROR, "Unable to find stats for weapon %s for structure %s", WeaponName[i], StructureName );
						abort();
						return false;
					}
				}
			}
		}
		//if structure not found - error
		if (!structureFound)
		{
			debug( LOG_ERROR, "Unable to find stats for structure %s", StructureName );
			abort();
			return false;
		}
		//increment the pointer to the start of the next record
		pWeaponData = strchr(pWeaponData,'\n') + 1;
	}
	return true;
}

//Load the programs assigned to Droids in the Access database
BOOL loadStructureFunctions(const char *pFunctionData, UDWORD bufferSize)
{
	const unsigned int NumToAlloc = numCR(pFunctionData, bufferSize);
	UDWORD				i, incS, incF;
	char				StructureName[MAX_STR_LENGTH], FunctionName[MAX_STR_LENGTH];
	STRUCTURE_STATS		*pStructure = asStructureStats;
	FUNCTION			*pFunction, **pStartFunctions = asFunctions;
	BOOL				functionFound, structureFound;



	for (i=0; i < NumToAlloc; i++)
	{
		//read the data into the storage - the data is delimeted using comma's
		StructureName[0] = '\0';
		FunctionName[0] = '\0';
		sscanf(pFunctionData, "%[^','],%[^','],%*d", StructureName, FunctionName);
		functionFound = structureFound = false;

		if (!getResourceName(StructureName))
		{
			return false;
		}

		//loop through each Structure_Stat to compare the name
		for (incS=0; incS < numStructureStats; incS++)
		{
			if (!(strcmp(StructureName, pStructure[incS].pName)))
			{
				//Structure found, so loop through each Function
				structureFound = true;
				pStartFunctions = asFunctions;
				for (incF=0; incF < numFunctions; incF++)
				{
					pFunction = *pStartFunctions;
					if (!(strcmp(FunctionName, pFunction->pName)))
					{
						//function found alloc this function to the current Structure
						functionFound = true;
						pStructure[incS].defaultFunc++;
						//check not allocating more than allowed
						if (pStructure[incS].defaultFunc >
										(SDWORD)pStructure[incS].numFuncs)
						{
							debug( LOG_ERROR, "Trying to allocate more functions than allowed for Structure" );
							abort();
							return false;
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
					debug( LOG_ERROR, "Unable to find stats for function %s for structure %s", FunctionName, StructureName );
					abort();
					return false;
				}
			}
		}
		//if structure not found - error
		if (!structureFound)
		{
			debug( LOG_ERROR, "Unable to find stats for structure %s", StructureName );
			abort();
			return false;
		}
		//increment the pointer to the start of the next record
		pFunctionData = strchr(pFunctionData,'\n') + 1;
	}

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
				if (!strcmp(((WALL_FUNCTION *)pFunction)->pStructName, pStructure->pName))
				{
					((WALL_FUNCTION *)pFunction)->pCornerStat = pStructure;
					break;
				}
				pStructure++;
			}
			//if haven't found the STRUCTURE STAT, then problem
			if (!((WALL_FUNCTION *)pFunction)->pCornerStat)
			{
				debug( LOG_ERROR, "Unknown Corner Wall stat for function %s", pFunction->pName );
				abort();
				return false;
			}
		}
		pStartFunctions++;
	}

	return true;
}

/*Load the Structure Strength Modifiers from the file exported from Access*/
BOOL loadStructureStrengthModifiers(const char *pStrengthModData, UDWORD bufferSize)
{
	const unsigned int NumRecords = numCR(pStrengthModData, bufferSize);
	STRUCT_STRENGTH		strengthInc;
	WEAPON_EFFECT		effectInc;
	UDWORD				i, j, modifier;
	char				weaponEffectName[MAX_STR_LENGTH], strengthName[MAX_STR_LENGTH];

	//initialise to 100%
	for (i=0; i < WE_NUMEFFECTS; i++)
	{
		for (j=0; j < NUM_STRUCT_STRENGTH; j++)
		{
			asStructStrengthModifier[i][j] = 100;
		}
	}

	for (i=0; i < NumRecords; i++)
	{
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pStrengthModData,"%[^','],%[^','],%d",
			weaponEffectName, strengthName, &modifier);

		//get the weapon effect inc
		effectInc = getWeaponEffect(weaponEffectName);
		if (effectInc == INVALID_WEAPON_EFFECT)
		{
			debug( LOG_ERROR, "loadStructureStrengthModifiers: Invalid Weapon Effect - %s", weaponEffectName );
			abort();
			return false;
		}
		//get the propulsion inc
		strengthInc = getStructStrength(strengthName);
		if (strengthInc == INVALID_STRENGTH)
		{
			debug( LOG_ERROR, "loadStructureStrengthModifiers: Invalid Strength type - %s", strengthName );
			abort();
			return false;
		}

		if (modifier > UWORD_MAX)
		{
			debug( LOG_ERROR, "loadStructureStrengthModifiers: modifier for effect %s, strength %s is too large", weaponEffectName, strengthName );
			abort();
			return false;
		}
		//store in the appropriate index
		asStructStrengthModifier[effectInc][strengthInc] = (UWORD)modifier;

		//increment the pointer to the start of the next record
		pStrengthModData = strchr(pStrengthModData,'\n') + 1;
	}

	return true;
}


BOOL structureStatsShutDown(void)
{
	UDWORD	inc;
	STRUCTURE_STATS*	pStructure = asStructureStats;

	for(inc=0; inc < numStructureStats; inc++, pStructure++)
	{
		if (pStructure->numFuncs > 0)
		{
			free(pStructure->asFuncList);
			pStructure->asFuncList = NULL;
		}
	}

	if(numStructureStats) {
		free(asStructureStats);
		asStructureStats = NULL;
	}

	//free up the structLimits structure
	for (inc = 0; inc < MAX_PLAYERS ; inc++)
	{
		if (asStructLimits[inc])
		{
			free(asStructLimits[inc]);
			asStructLimits[inc] = NULL;
		}
	}

	return true;
}

// TODO: The abandoned code needs to be factored out, see: saveMissionData
void handleAbandonedStructures()
{
	// FIXME: We should control the calling frequency externally from this
	//        function, rather than controlling the amount of times this
	//        function performs work internally.
	static int lastHandled = 0;
	int player;

	// We only need to run once every two seconds (2000ms)
	if (gameTime - lastHandled < 2000)
	{
		return;
	}

	// Update when we last ran
	lastHandled = gameTime;

	for (player = 0; player < MAX_PLAYERS; ++player)
	{
		STRUCTURE *psCurr, *psNext;

		for (psCurr = apsStructLists[player]; psCurr; psCurr = psNext)
		{
			// Save the next structure in the list
			psNext = psCurr->psNext;

			// We are only interested in structures accruing
			if (psCurr->status == SS_BEING_BUILT
			 && psCurr->currentPowerAccrued < structPowerToBuild(psCurr))
			{
				DROID *psDroid;
				bool beingBuilt = false;

				// See is there are any droids building it
				for (psDroid = apsDroidLists[player];
				     psDroid;
				     psDroid = psDroid->psNext)
				{
					// The droid is working on it and therefore not abandoned
					if ((STRUCTURE *) orderStateObj(psDroid, DORDER_BUILD) == psCurr)
					{
						beingBuilt = true;
						break;
					}
				}

				// Being worked on, nothing to see here
				if (beingBuilt)
				{
					continue;
				}
				// Abandoned
				else
				{
					int reductionAmount = 8;

					// Work out how much power to deduct
					CLIP(reductionAmount, 0, psCurr->currentPowerAccrued);

					// Do the reduction
					psCurr->currentPowerAccrued -= reductionAmount;
					addPower(player, reductionAmount);

					// Remove the structure if no power is accrued
					if (!psCurr->currentPowerAccrued)
					{
						removeStruct(psCurr, true);
					}
				}
			}
		}
	}
}

/* Deals damage to a Structure.
 * \param psStructure structure to deal damage to
 * \param damage amount of damage to deal
 * \param weaponClass the class of the weapon that deals the damage
 * \param weaponSubClass the subclass of the weapon that deals the damage
 * \return < 0 when the dealt damage destroys the structure, > 0 when the structure survives
 */
float structureDamage(STRUCTURE *psStructure, UDWORD damage, UDWORD weaponClass,
                       UDWORD weaponSubClass, HIT_SIDE impactSide)
{
	float		relativeDamage;

	CHECK_STRUCTURE(psStructure);

	debug(LOG_ATTACK, "structureDamage(%d): body %d armour %d damage: %d",
	      psStructure->id, psStructure->body, psStructure->armour[impactSide][weaponClass], damage);

	relativeDamage = objDamage((BASE_OBJECT *)psStructure, damage, structureBody(psStructure), weaponClass, weaponSubClass, impactSide);

	// If the shell did sufficient damage to destroy the structure
	if (relativeDamage < 0.0f)
	{
		debug(LOG_ATTACK, "structureDamage(%d): DESTROYED", psStructure->id);
		destroyStruct(psStructure);
	}
	else
	{
		// Survived
		CHECK_STRUCTURE(psStructure);
	}
	
	return relativeDamage;
}

float getStructureDamage(const STRUCTURE* psStructure)
{
	float health;
	CHECK_STRUCTURE(psStructure);

	health = (float)psStructure->body / (float)structureBody(psStructure);
	CLIP(health, 0., 1.f);

	return 1. - health;
}

/* Set the type of droid for a factory to build */
BOOL structSetManufacture(STRUCTURE *psStruct, DROID_TEMPLATE *psTempl, UBYTE quantity)
{
	FACTORY		*psFact;

	CHECK_STRUCTURE(psStruct);

	ASSERT( psStruct != NULL && psStruct->type == OBJ_STRUCTURE &&
			(psStruct->pStructureType->type == REF_FACTORY ||
			psStruct->pStructureType->type == REF_CYBORG_FACTORY ||
			psStruct->pStructureType->type == REF_VTOL_FACTORY),
		"structSetManufacture: invalid Factory pointer" );
	/* psTempl might be NULL if the build is being cancelled in the middle */

	//assign it to the Factory
	psFact = &psStruct->pFunctionality->factory;
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
	return true;
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
static SDWORD structChooseWallType(UDWORD player, UDWORD mapX, UDWORD mapY)
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
		xdiff = (SDWORD)mapX - map_coord((SDWORD)psStruct->pos.x);
		ydiff = (SDWORD)mapY - map_coord((SDWORD)psStruct->pos.y);
		if (xdiff >= -2 && xdiff <= 2 &&
			ydiff >= -2 && ydiff <= 2 &&
			(psStruct->pStructureType->type == REF_WALL ||
			psStruct->pStructureType->type == REF_WALLCORNER ||
			psStruct->pStructureType->type == REF_DEFENSE))
		{
			aWallPresent[xdiff+2][ydiff+2] = true;
			apsStructs[xdiff+2][ydiff+2] = psStruct;
		}
	}
	// add in the wall about to be built
	aWallPresent[2][2] = true;

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
					if ( (int)psStruct->direction == 90 )
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
								sx = psStruct->pos.x; sy = psStruct->pos.y;
								removeStruct(psStruct, true);
								powerCalc(false);
								psStruct = buildStructure(psStats, sx,sy, player, true);
								powerCalc(true);
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
								sx = psStruct->pos.x; sy = psStruct->pos.y;
								removeStruct(psStruct, true);
								powerCalc(false);
								psStruct = buildStructure(psStats, sx,sy, player, true);
								powerCalc(true);
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


static void buildFlatten(STRUCTURE_STATS *pStructureType, UDWORD atx, UDWORD aty,UDWORD h )
{
	UDWORD				x = map_coord(atx);
	UDWORD				y = map_coord(aty);
	UBYTE				width;
	UBYTE				breadth;

	for (breadth=0; breadth <= (UBYTE)(pStructureType->baseBreadth/* + 1*/); breadth++)
	{
		for (width=0; width <= (UBYTE)(pStructureType->baseWidth /*+ 1*/); width++)
		{
			if ( (pStructureType->type != REF_WALL) &&
				 (pStructureType->type != REF_WALLCORNER) )
			{
				setTileHeight(x + width, y + breadth, h);//-1
				// We need to raise features on raised tiles to the new height
				if(TILE_HAS_FEATURE(mapTile(x+width,y+breadth)))
				{
					getTileFeature(x+width, y+breadth)->pos.z = (UWORD)h;
				}
			}
		}
	}
	return ;
}

/*Builds an instance of a Structure - the x/y passed in are in world coords. */
STRUCTURE* buildStructure(STRUCTURE_STATS* pStructureType, UDWORD x, UDWORD y, UDWORD player, BOOL FromSave)
{
	UDWORD		mapX, mapY, mapH;
	UDWORD		width, breadth, weapon, capacity;
	float bodyDiff = 0.f;
	SDWORD		wallType = 0, preScrollMinX = 0, preScrollMinY = 0, preScrollMaxX = 0, preScrollMaxY = 0;
	int			i;
	STRUCTURE	*psBuilding = NULL;

	assert(pStructureType);
	ASSERT(pStructureType->type != REF_DEMOLISH, "You cannot build demolition!");

	if (IsStatExpansionModule(pStructureType)==false)
	{
		//some prelim tests...
		UDWORD	max = pStructureType - asStructureStats;

		if (max > numStructureStats)
		{
			ASSERT(!"invalid structure type", "buildStructure: Invalid structure type");
			return NULL;
		}
		// Don't allow more than interface limits
		if (asStructLimits[player][max].currentQuantity + 1 > asStructLimits[player][max].limit)
		{
			debug(LOG_ERROR, "Player %u: Building %s could not be built due to building limits (has %d, max %d)!", 
			      player, pStructureType->pName, asStructLimits[player][max].currentQuantity, 
			      asStructLimits[player][max].limit);
			return NULL;
		}

		// snap the coords to a tile
		x = ((pStructureType->baseWidth % 2) == 0) ? (x & ~TILE_MASK) : (x & ~TILE_MASK) + TILE_UNITS/2;
		y = ((pStructureType->baseBreadth % 2) == 0) ? (y & ~TILE_MASK) : (y & ~TILE_MASK) + TILE_UNITS/2;

		//check not trying to build too near the edge
		if (map_coord(x) < TOO_NEAR_EDGE
		 || map_coord(x) > (mapWidth - TOO_NEAR_EDGE))
		{
			debug(LOG_ERROR, "buildStructure: attempting to build too closely to map-edge, "
			      "x coord (%u) too near edge (req. distance is %u)", x, TOO_NEAR_EDGE);
			return NULL;
		}
		if (map_coord(y) < TOO_NEAR_EDGE
		 || map_coord(y) > (mapHeight - TOO_NEAR_EDGE))
		{
			debug(LOG_ERROR, "buildStructure: attempting to build too closely to map-edge, "
			      "y coord (%u) too near edge (req. distance is %u)", y, TOO_NEAR_EDGE);
			return NULL;
		}

		if (!FromSave && pStructureType->type == REF_WALL)
		{
			wallType = structChooseWallType(player, map_coord(x), map_coord(y));
			if (wallType == WALL_CORNER)
			{
				if (pStructureType->asFuncList[0]->type == WALL_TYPE)
				{
					pStructureType = ((WALL_FUNCTION *)pStructureType->asFuncList[0])->pCornerStat;
				}
			}
		}

		// allocate memory for and initialize a structure object
		psBuilding = createStruct(player);
		if (psBuilding == NULL)
		{
			return NULL;
		}

		psBuilding->psCurAnim = NULL;

		//fill in other details
		psBuilding->pStructureType = pStructureType;

		psBuilding->pos.x = (UWORD)x;
		psBuilding->pos.y = (UWORD)y;

		//This needs to be done before the functionality bit...
		//load into the map data and structure list if not an upgrade
		mapX = map_coord(x) - (pStructureType->baseWidth/2);
		mapY = map_coord(y) - (pStructureType->baseBreadth/2);

		//set up the imd to use for the display
		psBuilding->sDisplay.imd = pStructureType->pIMD;

		/* if resource extractor - need to remove oil feature first, but do not do any
		 * consistency checking here - save games do not have any feature to remove
		 * to remove when placing oil derricks! */
		if (pStructureType->type == REF_RESOURCE_EXTRACTOR)
		{
			FEATURE *psFeature = getTileFeature(map_coord(x), map_coord(y));

			if (psFeature && psFeature->psStats->subType == FEAT_OIL_RESOURCE)
			{
				// remove it from the map
				turnOffMultiMsg(true); // dont send this one!
				removeFeature(psFeature);
				turnOffMultiMsg(false);
			}
		}

		mapH = buildFoundation(pStructureType, x, y);
		for (width = 0; width < pStructureType->baseWidth; width++)
		{
			for (breadth = 0; breadth < pStructureType->baseBreadth; breadth++)
			{
				MAPTILE *psTile = mapTile(mapX+width,mapY+breadth);

				/* Remove any walls underneath the building. You can build defense buildings on top
				 * of walls, you see. This is not the place to test whether we own it! */
				if (pStructureType->type == REF_DEFENSE && TILE_HAS_WALL(psTile))
				{
					removeStruct((STRUCTURE *)psTile->psObject, true);
				}

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
				else if (TILE_HAS_STRUCTURE(psTile))
				{
					debug(LOG_ERROR,
					       "buildStructure: building %s at (%d, %d) but found %s already at (%d, %d)",
					       pStructureType->pName, mapX, mapY,
					       getTileStructure(mapX + width, mapY + breadth)->pStructureType->pName,
					       mapX + width, mapY + breadth);
					free(psBuilding);
					return NULL;
				}

				psTile->psObject = (BASE_OBJECT*)psBuilding;
			}
		}

		/* DEFENSIVE structures are pulled to the terrain */
		if(pStructureType->type != REF_DEFENSE)
		{
			buildFlatten(pStructureType, world_coord(mapX), world_coord(mapY), mapH);
			psBuilding->pos.z = (UWORD)mapH;
		}
		else
		{
			psBuilding->pos.z = TILE_MIN_HEIGHT;

			/* Set it at the higher coord */
			for (width = 0; width < pStructureType->baseWidth; width++)
			{
				for (breadth = 0; breadth < pStructureType->baseBreadth; breadth++)
				{
					UDWORD tmpMax, tmpMin;

					getTileMaxMin(mapX + width, mapY + breadth, &tmpMax, &tmpMin);
					psBuilding->pos.z = MAX(tmpMax, psBuilding->pos.z);
				}
			}
		}

		//set up the rest of the data
		for (i = 0;i < STRUCT_MAXWEAPS;i++)
		{
			psBuilding->turretRotation[i] = 0;
			psBuilding->turretPitch[i] = 0;
			psBuilding->psTarget[i] = NULL;
		}
		psBuilding->targetted = 0;

		psBuilding->lastEmission = 0;
		psBuilding->timeLastHit = 0;
		psBuilding->lastHitWeapon = UDWORD_MAX;	// no such weapon

		psBuilding->inFire = false;
		psBuilding->burnStart = 0;
		psBuilding->burnDamage = 0;

		psBuilding->direction = 0;
		psBuilding->pitch = 0;
		psBuilding->roll = 0;
		psBuilding->selected = false;
		psBuilding->status = SS_BEING_BUILT;
		psBuilding->currentBuildPts = 0;
		psBuilding->currentPowerAccrued = 0;
		psBuilding->cluster = 0;

		// rotate a wall if necessary
		if (!FromSave && pStructureType->type == REF_WALL && wallType == WALL_VERT)
		{
			psBuilding->direction = 90;
		}

		//set up the sensor stats
		if (psBuilding->pStructureType->pSensor)
		{
			psBuilding->sensorRange = (UWORD)sensorRange(psBuilding->pStructureType->
				pSensor, (UBYTE)player);
			psBuilding->sensorPower = (UWORD)sensorPower(psBuilding->pStructureType->
				pSensor, (UBYTE)player);
		}
		else
		{
			//give them the default sensor for droids if not
			psBuilding->sensorRange = (UWORD)sensorRange(asSensorStats +
				aDefaultSensor[player], (UBYTE)player);
			psBuilding->sensorPower = (UWORD)sensorPower(asSensorStats +
				aDefaultSensor[player], (UBYTE)player);
		}
		//set up the ecm stat
		if (psBuilding->pStructureType->pECM)
		{
			psBuilding->ECMMod = (UWORD)ecmPower(psBuilding->pStructureType->pECM,
				psBuilding->player);
		}
		else
		{
			psBuilding->ECMMod = 0;
		}

		/* Store the weapons */
		memset(psBuilding->asWeaps, 0, sizeof(WEAPON));
		//Watermelon:can only have the STRUCT_MAXWEAPS weapon now
		psBuilding->numWeaps = 0;
		if (pStructureType->numWeaps > 0)
		{
			for(weapon=0; weapon < pStructureType->numWeaps; weapon++)
			{
				//can only have the one weapon now - AB 24/01/99
				if (pStructureType->psWeapStat[weapon])
				{
					psBuilding->asWeaps[weapon].lastFired = 0;
					//in multiPlayer make the Las-Sats require re-loading from the start
					if (bMultiPlayer && pStructureType->psWeapStat[0]->
						weaponSubClass == WSC_LAS_SAT)
					{
						psBuilding->asWeaps[0].lastFired = gameTime;
					}
					psBuilding->asWeaps[weapon].nStat =	pStructureType->psWeapStat[weapon] - asWeaponStats;
					psBuilding->asWeaps[weapon].hitPoints = (asWeaponStats + psBuilding->
						asWeaps[weapon].nStat)->hitPoints;
					psBuilding->asWeaps[weapon].ammo = (asWeaponStats + psBuilding->
						asWeaps[weapon].nStat)->numRounds;
					psBuilding->asWeaps[weapon].recoilValue = 0;
					psBuilding->numWeaps++;
				}
			}
		}
		else
		{
			if (pStructureType->psWeapStat[0])
			{
				psBuilding->asWeaps[0].lastFired = 0;
				//in multiPlayer make the Las-Sats require re-loading from the start
				if (bMultiPlayer && pStructureType->psWeapStat[0]->
					weaponSubClass == WSC_LAS_SAT)
				{
					psBuilding->asWeaps[0].lastFired = gameTime;
				}
				psBuilding->asWeaps[0].nStat =	pStructureType->psWeapStat[0] - asWeaponStats;
				psBuilding->asWeaps[0].hitPoints = (asWeaponStats + psBuilding->
					asWeaps[0].nStat)->hitPoints;
				psBuilding->asWeaps[0].ammo = (asWeaponStats + psBuilding->
					asWeaps[0].nStat)->numRounds;
				psBuilding->asWeaps[0].recoilValue = 0;
			}
		}

		// Structures currently do not have varied armour
		for (i = 0; i < NUM_HIT_SIDES; i++)
		{
			int j;

			for (j = 0; j < NUM_WEAPON_CLASS; j++)
			{
				psBuilding->armour[i][j] = (UWORD)structureArmour(pStructureType, (UBYTE)player);
			}
		}
		psBuilding->resistance = (UWORD)structureResistance(pStructureType, (UBYTE)player);
		psBuilding->lastResistance = ACTION_START_TIME;

		//do the visiblilty stiff before setFunctionality - so placement of DP's can work
		memset(psBuilding->visible, 0, (sizeof(UBYTE) * MAX_PLAYERS) );

		/* Structure is trivially visible to the builder (owner) */
		psBuilding->visible[player] = UBYTE_MAX;

		// Reveal any tiles that can be seen by the structure
		visTilesUpdate((BASE_OBJECT *)psBuilding);

		/*if we're coming from a SAVEGAME and we're on an Expand_Limbo mission,
		any factories that were built previously for the selectedPlayer will
		have DP's in an invalid location - the scroll limits will have been
		changed to not include them. This is the only HACK I can think of to
		enable them to be loaded up. So here goes...*/
		if (FromSave && player == selectedPlayer && missionLimboExpand())
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
			free(psBuilding);
			//better reset these if you couldn't build the structure!
			if (FromSave && player == selectedPlayer && missionLimboExpand())
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
		if (FromSave && player == selectedPlayer && missionLimboExpand())
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

		gridAddObject((BASE_OBJECT *)psBuilding);

		clustNewStruct(psBuilding);
		asStructLimits[player][max].currentQuantity++;
	}
	else //its an upgrade
	{
		BOOL bUpgraded = false;

		psBuilding = getTileStructure(map_coord(x), map_coord(y));

		//don't create the Structure use existing one

		if (psBuilding == NULL)
		{
			ASSERT(!"module has no owning structure", "No owning structure for this module - %s", getStructName(pStructureType));
			return false;
		}
		if (pStructureType->type == REF_FACTORY_MODULE)
		{
			if (psBuilding->pStructureType->type != REF_FACTORY &&
				psBuilding->pStructureType->type != REF_VTOL_FACTORY)
			{
				return NULL;
			}
			//increment the capacity and output for the owning structure
			if (psBuilding->pFunctionality->factory.capacity < SIZE_SUPER_HEAVY)
			{
				//store the % difference in body points before upgrading
				bodyDiff = 1. - getStructureDamage(psBuilding);

				++psBuilding->pFunctionality->factory.capacity;
				bUpgraded = true;
				//put any production on hold
				holdProduction(psBuilding);

				//quick check not trying to add too much
				ASSERT(psBuilding->pFunctionality->factory.productionOutput +
					((PRODUCTION_FUNCTION*)pStructureType->asFuncList[0])->productionOutput < UBYTE_MAX,
					"building factory module - productionOutput too big");

				psBuilding->pFunctionality->factory.productionOutput += ((
					PRODUCTION_FUNCTION*)pStructureType->asFuncList[0])->productionOutput;
				//need to change which IMD is used for player 0
				//Need to do a check its not Barbarian really!

				if (   (bMultiPlayer && isHumanPlayer(psBuilding->player))
					|| (bMultiPlayer && (game.type == SKIRMISH) && (psBuilding->player < game.maxPlayers))
					|| !bMultiPlayer)
				{
					capacity = psBuilding->pFunctionality->factory.capacity;
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
			if (psBuilding->pFunctionality->researchFacility.capacity < NUM_RESEARCH_MODULES)
			{
				//store the % difference in body points before upgrading
				bodyDiff = 1. - getStructureDamage(psBuilding);

				//add all the research modules in one go AB 24/06/98
				//((RESEARCH_FACILITY*)psBuilding->pFunctionality)->capacity++;
				psBuilding->pFunctionality->researchFacility.capacity = NUM_RESEARCH_MODULES;
				psBuilding->pFunctionality->researchFacility.researchPoints += ((
					RESEARCH_FUNCTION*)pStructureType->asFuncList[0])->
					researchPoints;
				bUpgraded = true;
				//cancel any research - put on hold now
				if (psBuilding->pFunctionality->researchFacility.psSubject)
				{
					//cancel the topic
					holdResearch(psBuilding);
				}

				//need to change which IMD is used for player 0
				//Need to do a check its not Barbarian really!
				if (   (bMultiPlayer && isHumanPlayer(psBuilding->player))
					|| (bMultiPlayer && (game.type == SKIRMISH) && (psBuilding->player < game.maxPlayers))
					|| !bMultiPlayer)
				{
					capacity = psBuilding->pFunctionality->researchFacility.capacity;
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
			if (psBuilding->pFunctionality->powerGenerator.capacity < NUM_POWER_MODULES)
			{
				//store the % difference in body points before upgrading
				bodyDiff = 1. - getStructureDamage(psBuilding);

				//increment the power output, multiplier and capacity
				//add all the research modules in one go AB 24/06/98
				psBuilding->pFunctionality->powerGenerator.capacity = NUM_POWER_MODULES;
				psBuilding->pFunctionality->powerGenerator.power += ((
					POWER_GEN_FUNCTION*)pStructureType->asFuncList[0])->
					powerOutput;
				psBuilding->pFunctionality->powerGenerator.multiplier += ((
					POWER_GEN_FUNCTION*)pStructureType->asFuncList[0])->
					powerMultiplier;
				bUpgraded = true;

				//need to change which IMD is used for player 0
				//Need to do a check its not Barbarian really!

				if (   (bMultiPlayer && isHumanPlayer(psBuilding->player))
					|| (bMultiPlayer && (game.type == SKIRMISH) && (psBuilding->player < game.maxPlayers))
					|| !bMultiPlayer)
				{
					capacity = psBuilding->pFunctionality->powerGenerator.capacity;
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
			psBuilding->body = (UWORD)(structureBody(psBuilding) * bodyDiff);

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
	}
	if(pStructureType->type!=REF_WALL && pStructureType->type!=REF_WALLCORNER)
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


BOOL setFunctionality(STRUCTURE	*psBuilding, UDWORD functionType)
{
	CHECK_STRUCTURE(psBuilding);

	switch (functionType)
	{
		case REF_FACTORY:
		case REF_CYBORG_FACTORY:
		case REF_VTOL_FACTORY:
		case REF_RESEARCH:
		case REF_POWER_GEN:
		case REF_RESOURCE_EXTRACTOR:
		case REF_REPAIR_FACILITY:
		case REF_REARM_PAD:
			// Allocate space for the buildings functionality
			psBuilding->pFunctionality = calloc(1, sizeof(FUNCTIONALITY));

			if (psBuilding->pFunctionality == NULL)
			{
				debug(LOG_ERROR, "setFunctionality: Out of memory");
				abort();
				return false;
			}
			break;

		default:
			psBuilding->pFunctionality = NULL;
			break;
	}

	switch (functionType)
	{
		case REF_FACTORY:
		case REF_CYBORG_FACTORY:
		case REF_VTOL_FACTORY:
		{
			FACTORY* psFactory = &psBuilding->pFunctionality->factory;
			unsigned int x, y;

			psFactory->capacity = (UBYTE) ((PRODUCTION_FUNCTION*)psBuilding->pStructureType->asFuncList[0])->capacity;
			psFactory->productionOutput = (UBYTE) ((PRODUCTION_FUNCTION*)psBuilding->pStructureType->asFuncList[0])->productionOutput;
			psFactory->psSubject = NULL;

			// Default the secondary order - AB 22/04/99
			psFactory->secondaryOrder = DSS_ARANGE_DEFAULT | DSS_REPLEV_NEVER
                                      | DSS_ALEV_ALWAYS | DSS_HALT_GUARD;

			// Create the assembly point for the factory
			if (!createFlagPosition(&psFactory->psAssemblyPoint, psBuilding->player))
			{
				return false;
			}

			// initialise the assembly point position
			x = map_coord(psBuilding->pos.x + 256);
			y = map_coord(psBuilding->pos.y + 256);

			// Set the assembly point
			setAssemblyPoint(psFactory->psAssemblyPoint, world_coord(x), world_coord(y), psBuilding->player, true);

			// Add the flag to the list
			addFlagPosition(psFactory->psAssemblyPoint);
			switch (functionType)
			{
				case REF_FACTORY:
					setFlagPositionInc(psBuilding->pFunctionality, psBuilding->player, FACTORY_FLAG);
					break;
				case REF_CYBORG_FACTORY:
					setFlagPositionInc(psBuilding->pFunctionality, psBuilding->player, CYBORG_FLAG);
					break;
				case REF_VTOL_FACTORY:
					setFlagPositionInc(psBuilding->pFunctionality, psBuilding->player, VTOL_FLAG);
					break;
				default:
					ASSERT(!"invalid factory type", "setFunctionality: Invalid factory type");
			}

			// Take advantage of upgrades
			structureProductionUpgrade(psBuilding);
			break;
		}
		case REF_RESEARCH:
		{
			RESEARCH_FACILITY* psResFac = &psBuilding->pFunctionality->researchFacility;

			psResFac->researchPoints = ((RESEARCH_FUNCTION *) psBuilding->pStructureType->asFuncList[0])->researchPoints;

			// Take advantage of upgrades
			structureResearchUpgrade(psBuilding);
			break;
		}
		case REF_POWER_GEN:
		{
			POWER_GEN* psPowerGen = &psBuilding->pFunctionality->powerGenerator;

			psPowerGen->power = ((POWER_GEN_FUNCTION *) psBuilding->pStructureType->asFuncList[0])->powerOutput;
			psPowerGen->multiplier = ((POWER_GEN_FUNCTION *) psBuilding->pStructureType->asFuncList[0])->powerMultiplier;
			psPowerGen->capacity = 0;

			// Take advantage of upgrades
			structurePowerUpgrade(psBuilding);
			break;
		}
		case REF_RESOURCE_EXTRACTOR:
		{
			RES_EXTRACTOR* psResExtracter = &psBuilding->pFunctionality->resourceExtractor;

			psResExtracter->power = ((RESOURCE_FUNCTION*)psBuilding->pStructureType->asFuncList[0])->maxPower;

			// Make the structure inactive
			psResExtracter->active = false;
			psResExtracter->psPowerGen = NULL;
			break;
		}
		case REF_HQ:
		{
			// If an HQ has just been built make sure the radar is displayed!
			radarOnScreen = true;
			break;
		}
		case REF_REPAIR_FACILITY:
		{
			REPAIR_FACILITY* psRepairFac = &psBuilding->pFunctionality->repairFacility;
			REPAIR_DROID_FUNCTION* pFuncRepair = (REPAIR_DROID_FUNCTION*)psBuilding->pStructureType->asFuncList[0];
			unsigned int x, y;

			psRepairFac->power = pFuncRepair->repairPoints;
			psRepairFac->psObj = NULL;

			if (!grpCreate(&((REPAIR_FACILITY*)psBuilding->pFunctionality)->psGroup))
			{
				debug( LOG_NEVER, "setFunctionality: couldn't create repair facility group" );
			}
			else
			{
				// Add NULL droid to the group
				grpJoin(psRepairFac->psGroup, NULL);
			}

			// Take advantage of upgrades
			structureRepairUpgrade(psBuilding);

			// Create an assembly point for repaired droids
			if (!createFlagPosition(&psRepairFac->psDeliveryPoint, psBuilding->player))
			{
				return false;
			}

			// Initialise the assembly point
			x = map_coord(psBuilding->pos.x+256);
			y = map_coord(psBuilding->pos.y+256);

			// Set the assembly point
			setAssemblyPoint(psRepairFac->psDeliveryPoint, world_coord(x),
			                 world_coord(y), psBuilding->player, true);

			// Add the flag (triangular marker on the ground) at the delivery point
			addFlagPosition(psRepairFac->psDeliveryPoint);
			setFlagPositionInc(psBuilding->pFunctionality, psBuilding->player, REPAIR_FLAG);
			break;
		}
		case REF_REARM_PAD:
		{
			REARM_PAD* psReArmPad = &psBuilding->pFunctionality->rearmPad;

			psReArmPad->reArmPoints = ((REARM_PAD *)psBuilding->pStructureType->asFuncList[0])->reArmPoints;

			// Take advantage of upgrades
			structureReArmUpgrade(psBuilding);
			break;
		}
	}

	return true;
}


// Set the command droid that factory production should go to
void assignFactoryCommandDroid(STRUCTURE *psStruct, DROID *psCommander)
{
	FACTORY			*psFact;
	FLAG_POSITION	*psFlag, *psNext, *psPrev;
	SDWORD			factoryInc,typeFlag;

	CHECK_STRUCTURE(psStruct);
	ASSERT( StructIsFactory(psStruct),"assignFactoryCommandUnit: structure not a factory" );

	psFact = &psStruct->pFunctionality->factory;

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
			ASSERT(!"unknown factory type", "assignfactorycommandUnit: unknown factory type");
			typeFlag = FACTORY_FLAG;
			break;
	}

	// removing a commander from a factory
	if ( psFact->psCommander != NULL )
	{
		if (typeFlag == FACTORY_FLAG)
		{
			secondarySetState(psFact->psCommander, DSO_CLEAR_PRODUCTION,
					(SECONDARY_STATE)(1 << ( psFact->psAssemblyPoint->factoryInc + DSS_ASSPROD_SHIFT)) );
		}
		else if (typeFlag == CYBORG_FLAG)
		{
			secondarySetState(psFact->psCommander, DSO_CLEAR_PRODUCTION,
					(SECONDARY_STATE)(1 << ( psFact->psAssemblyPoint->factoryInc + DSS_ASSPROD_CYBORG_SHIFT)) );
		}
		else
		{
			secondarySetState(psFact->psCommander, DSO_CLEAR_PRODUCTION,
					(SECONDARY_STATE)(1 << ( psFact->psAssemblyPoint->factoryInc + DSS_ASSPROD_VTOL_SHIFT)) );
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
		ASSERT( !missionIsOffworld(), "assignFactoryCommandDroid: cannot assign a commander to a factory when off world" );

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
			if (psCurr->pFunctionality->factory.psCommander == psDroid)
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
			if (psCurr->pFunctionality->factory.psCommander == psDroid)
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
	if(fpathBlockingTile(x,y))
	{
		debug( LOG_NEVER, "structClearTile: failed\n");
		return false;
	}

	/* Check for a droid */
	for(player=0; player< MAX_PLAYERS; player++)
	{
		for(psCurr = apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
		{
			if (map_coord(psCurr->pos.x) == x
			 && map_coord(psCurr->pos.y) == y)
			{
				debug( LOG_NEVER, "structClearTile: failed\n");
				return false;
			}
		}
	}

	debug( LOG_NEVER, "structClearTile: succeeded\n");
	return true;
}

/*find a location near to a structure to start the droid of*/
BOOL placeDroid(STRUCTURE *psStructure, UDWORD *droidX, UDWORD *droidY)
{
	SWORD			sx,sy, xmin,xmax, ymin,ymax, x,y, xmid;
	BOOL			placed;

	CHECK_STRUCTURE(psStructure);

	/* Get the tile coords for the top left of the structure */
	sx = (SWORD)(psStructure->pos.x - psStructure->pStructureType->baseWidth * TILE_UNITS/2);
	sx = map_coord(sx);
	sy = (SWORD)(psStructure->pos.y - psStructure->pStructureType->baseBreadth * TILE_UNITS/2);
	sy = map_coord(sy);

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
	placed = false;
	y = ymax;
	/* middle to right */
	for(x = xmid; x < xmax; x++)
	{
		if (structClearTile(x, y))
		{
			placed = true;
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
				placed = true;
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
				placed = true;
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
				placed = true;
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
				placed = true;
				break;
			}
		}
	}
	*droidX = x;
	*droidY = y;
	return placed;
}

/* Place a newly manufactured droid next to a factory  and then send if off
to the assembly point, returns true if droid was placed successfully */
static BOOL structPlaceDroid(STRUCTURE *psStructure, DROID_TEMPLATE *psTempl,
							DROID **ppsDroid)
{
	UDWORD			x,y;
	BOOL			placed;//bTemp = false;
	DROID			*psNewDroid;
	FACTORY			*psFact;
	SDWORD			apx,apy;
	FLAG_POSITION	*psFlag;
	Vector3i iVecEffect;
	UBYTE			factoryType;
	BOOL			assignCommander;

	CHECK_STRUCTURE(psStructure);

	placed = placeDroid(psStructure, &x, &y);

	if (placed)
	{
		//create a droid near to the structure
		psNewDroid = buildDroid(psTempl, world_coord(x), world_coord(y),
			psStructure->player, false);
		if (!psNewDroid)
		{
			*ppsDroid = NULL;
			return false;
		}

		//set the droids order to that of the factory - AB 22/04/99
		psNewDroid->secondaryOrder = psStructure->pFunctionality->factory.secondaryOrder;

		if(bMultiPlayer)
		{
			sendDroidSecondaryAll(psNewDroid);
		}

		if(psStructure->visible[selectedPlayer])
		{
			/* add smoke effect to cover the droid's emergence from the factory */
			iVecEffect.x = psNewDroid->pos.x;
			iVecEffect.y = map_Height( psNewDroid->pos.x, psNewDroid->pos.y ) + DROID_CONSTRUCTION_SMOKE_HEIGHT;
			iVecEffect.z = psNewDroid->pos.y;
			addEffect( &iVecEffect,EFFECT_CONSTRUCTION,CONSTRUCTION_TYPE_DRIFTING,false,NULL,0 );
			iVecEffect.x = psNewDroid->pos.x - DROID_CONSTRUCTION_SMOKE_OFFSET;
			iVecEffect.z = psNewDroid->pos.y - DROID_CONSTRUCTION_SMOKE_OFFSET;
			addEffect( &iVecEffect,EFFECT_CONSTRUCTION,CONSTRUCTION_TYPE_DRIFTING,false,NULL,0 );
			iVecEffect.z = psNewDroid->pos.y + DROID_CONSTRUCTION_SMOKE_OFFSET;
			addEffect( &iVecEffect,EFFECT_CONSTRUCTION,CONSTRUCTION_TYPE_DRIFTING,false,NULL,0 );
			iVecEffect.x = psNewDroid->pos.x + DROID_CONSTRUCTION_SMOKE_OFFSET;
			addEffect( &iVecEffect,EFFECT_CONSTRUCTION,CONSTRUCTION_TYPE_DRIFTING,false,NULL,0 );
			iVecEffect.z = psNewDroid->pos.y - DROID_CONSTRUCTION_SMOKE_OFFSET;
			addEffect( &iVecEffect,EFFECT_CONSTRUCTION,CONSTRUCTION_TYPE_DRIFTING,false,NULL,0 );
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

		psFact = &psStructure->pFunctionality->factory;
		apx = psFact->psAssemblyPoint->coords.x;
		apy = psFact->psAssemblyPoint->coords.y;

		// if we've built a command droid - make sure that it isn't assigned to another commander
		assignCommander = false;
		if ((psNewDroid->droidType == DROID_COMMAND) &&
			(psFact->psCommander != NULL))
		{
			assignFactoryCommandDroid(psStructure, NULL);
			assignCommander = true;
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
			placed = false;
			if (vtolDroid(psNewDroid))
			{
				moveToRearm(psNewDroid);
			}
			if (!placed)
			{
				//find flag in question.
				for(psFlag = apsFlagPosLists[psFact->psAssemblyPoint->player];
						!( (psFlag->factoryInc == psFact->psAssemblyPoint->factoryInc) // correct fact.
						&&(psFlag->factoryType == factoryType)); // correct type
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
			}
		}

		if (assignCommander)
		{
			assignFactoryCommandDroid(psStructure, psNewDroid);
		}
		if ( psNewDroid->player == selectedPlayer )
		{
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DROIDBUILT);
		}

		return true;
	}
	else
	{
		*ppsDroid = NULL;
	}

	return false;
}


static BOOL IsFactoryCommanderGroupFull(FACTORY *psFactory)
{
	SDWORD DroidsInGroup;

	// If we don't have a commander return false (group not full)
	if (psFactory->psCommander==NULL) return false;

	// allow any number of IDF droids
	if (templateIsIDF((DROID_TEMPLATE *)psFactory->psSubject))
	{
		return false;
	}

	// Get the number of droids in the commanders group
	DroidsInGroup = psFactory->psCommander->psGroup ? grpNumMembers( psFactory->psCommander->psGroup ) : 0;

	// if the number in group is less than the maximum allowed then return false (group not full)
	if (DroidsInGroup < cmdDroidMaxGroup( psFactory->psCommander )) return false;

	// the number in group has reached the maximum
	return true;
}


// Disallow manufacture of units once these limits are reached,
// dos'nt mean that these numbers can't be exceeded if units are
// put down in the editor or by the scripts.
static UWORD MaxDroidsAllowedPerPlayer[MAX_PLAYERS] = {100, 999, 999, 999, 999, 999, 999, 999};
static UWORD MaxDroidsAllowedPerPlayerMultiPlayer[MAX_PLAYERS] = {300, 300, 300, 300, 300, 300, 300, 300};

BOOL IsPlayerStructureLimitReached(UDWORD PlayerNumber)
{
	// PC currently doesn't limit number of structures a player can build.
	return false;
}


UDWORD getMaxDroids(UDWORD PlayerNumber)
{
	return (bMultiPlayer ? MaxDroidsAllowedPerPlayerMultiPlayer[PlayerNumber] : MaxDroidsAllowedPerPlayer[PlayerNumber] );

}


BOOL IsPlayerDroidLimitReached(UDWORD PlayerNumber)
{
	unsigned int numDroids = getNumDroids(PlayerNumber) + getNumMissionDroids(PlayerNumber) + getNumTransporterDroids(PlayerNumber);

	if (bMultiPlayer)
	{
		if ( numDroids >= MaxDroidsAllowedPerPlayerMultiPlayer[PlayerNumber] )
			return true;
	}
	else
	{
		if( numDroids >= MaxDroidsAllowedPerPlayer[PlayerNumber] )
			return true;
	}

	return false;
}


static BOOL maxDroidsByTypeReached(STRUCTURE *psStructure)
{
	FACTORY		*psFact = &psStructure->pFunctionality->factory;

	CHECK_STRUCTURE(psStructure);

	if ( (droidTemplateType((DROID_TEMPLATE *)psFact->psSubject) == DROID_COMMAND) &&
		(getNumCommandDroids(psStructure->player) >= MAX_COMMAND_DROIDS) )
	{
		return true;
	}

	if ( (droidTemplateType((DROID_TEMPLATE *)psFact->psSubject) == DROID_CONSTRUCT ||
		droidTemplateType((DROID_TEMPLATE *)psFact->psSubject) == DROID_CYBORG_CONSTRUCT) &&
		(getNumConstructorDroids(psStructure->player) >= MAX_CONSTRUCTOR_DROIDS) )
	{
		return true;
	}

	return false;
}


// Check for max number of units reached and halt production.
//
BOOL CheckHaltOnMaxUnitsReached(STRUCTURE *psStructure)
{
	CHECK_STRUCTURE(psStructure);

	// if the players that owns the factory has reached his (or hers) droid limit
	// then put production on hold & return - we need a message to be displayed here !!!!!!!
	if (IsPlayerDroidLimitReached(psStructure->player) ||
		maxDroidsByTypeReached(psStructure))
	{
		if ((psStructure->player == selectedPlayer) &&
			(lastMaxUnitMessage + MAX_UNIT_MESSAGE_PAUSE < gameTime))
		{
			addConsoleMessage(_("Command Control Limit Reached - Production Halted"),DEFAULT_JUSTIFY,SYSTEM_MESSAGE);
			lastMaxUnitMessage = gameTime;
		}
		return true;
	}

	return false;
}


static void aiUpdateStructure(STRUCTURE *psStructure)
{
	BASE_STATS			*pSubject = NULL;
	UDWORD				pointsToAdd;//, iPower;
	PLAYER_RESEARCH		*pPlayerRes = asPlayerResList[psStructure->player];
	UDWORD				structureMode = 0;
	DROID				*psDroid;
	BASE_OBJECT			*psChosenObjs[STRUCT_MAXWEAPS] = {NULL};
	BASE_OBJECT			*psChosenObj = NULL;
	UBYTE				Quantity;
	SDWORD				iDt;
	FACTORY				*psFactory;
	REPAIR_FACILITY		*psRepairFac = NULL;
	RESEARCH_FACILITY	*psResFacility;
	REARM_PAD			*psReArmPad;
	Vector3i iVecEffect;
	BOOL				bFinishAction,bDroidPlaced;
	WEAPON_STATS		*psWStats;
	SDWORD				xdiff,ydiff, mindist, currdist;
#ifdef INCLUDE_FACTORYLISTS
	DROID_TEMPLATE		*psNextTemplate;
#endif
	UDWORD				i;

	CHECK_STRUCTURE(psStructure);

	for (i = 0; i < DROID_MAXWEAPS; i++)
	{
		if (psStructure->psTarget[i] && psStructure->psTarget[i]->died)
		{
			setStructureTarget(psStructure, NULL, i);
		}
	}

	// Will go out into a building EVENT stats/text file
	/* Spin round yer sensors! */
	if (psStructure->numWeaps == 0)
	{
		if ((psStructure->asWeaps[0].nStat == 0) &&
			(psStructure->pStructureType->type != REF_REPAIR_FACILITY))
		{

	//////
	// - radar should rotate every three seconds ... 'cause we timed it at Heathrow !
	// gameTime is in milliseconds - one rotation every 3 seconds = 1 rotation event 3000 millisecs
			psStructure->turretRotation[0] = (UWORD)(((gameTime*360)/3000)%360);

			psStructure->turretPitch[0] = 0;
		}
	}

	structUpdateRecoil(psStructure);

	/* See if there is an enemy to attack */
	if (psStructure->numWeaps > 0)
	{
		//structures always update their targets
		for (i = 0;i < psStructure->numWeaps;i++)
		{
			if (psStructure->asWeaps[i].nStat > 0 &&
				asWeaponStats[psStructure->asWeaps[i].nStat].weaponSubClass != WSC_LAS_SAT)
			{
				if ((psStructure->id % 20) == (frameGetFrameNumber() % 20))
				{
					if ( aiChooseTarget((BASE_OBJECT *)psStructure, &psChosenObjs[i], i, true) )
					{
						debug( LOG_ATTACK, "Struct(%d) attacking : %d\n",
								psStructure->id, psChosenObjs[i]->id );
						setStructureTarget(psStructure, psChosenObjs[i], i);
					}
					else
					{
						if ( aiChooseTarget((BASE_OBJECT *)psStructure, &psChosenObjs[0], 0, true) )
						{
							if (psChosenObjs[0])
							{
								debug( LOG_ATTACK, "Struct(%d) attacking : %d\n",
										psStructure->id, psChosenObjs[0]->id );
								setStructureTarget(psStructure, psChosenObjs[0], i);
								psChosenObjs[i] = psChosenObjs[0];
							}
							else
							{
								setStructureTarget(psStructure, NULL, i);
								psChosenObjs[i] = NULL;
							}
						}
						else
						{
							setStructureTarget(psStructure, NULL, i);
							psChosenObjs[i] = NULL;
						}
					}
				}
				else
				{
					psChosenObjs[i] = psStructure->psTarget[0];
				}

				if (psChosenObjs[i] != NULL)
				{
					// get the weapon stat to see if there is a visible turret to rotate
					psWStats = asWeaponStats + psStructure->asWeaps[i].nStat;

					//if were going to shoot at something move the turret first then fire when locked on
					if (psWStats->pMountGraphic == NULL)//no turret so lock on whatever
					{
						psStructure->turretRotation[i] = (UWORD)calcDirection(psStructure->pos.x,
							psStructure->pos.y, psChosenObjs[i]->pos.x, psChosenObjs[i]->pos.y);
						combFire(&psStructure->asWeaps[i], (BASE_OBJECT *)psStructure, psChosenObjs[i], i);
					}
					else if(actionTargetTurret((BASE_OBJECT*)psStructure, psChosenObjs[i],
											&(psStructure->turretRotation[i]),
											&(psStructure->turretPitch[i]),
											psWStats, false, i))
					{
						combFire(&psStructure->asWeaps[i], (BASE_OBJECT *)psStructure, psChosenObjs[i], i);
					}
				}
				else
				{
					// realign the turret
					if ( ((psStructure->turretRotation[i] % 90) != 0) ||
						(psStructure->turretPitch[i] != 0) )
					{
						actionAlignTurret((BASE_OBJECT *)psStructure, i);
					}
				}
			}
		}
	}

	/* See if there is an enemy to attack for Sensor Towers that have weapon droids attached*/
	else if (psStructure->pStructureType->pSensor)
	{
		if (structStandardSensor(psStructure) || structVTOLSensor(psStructure))
		{
			if ((psStructure->id % 20) == (frameGetFrameNumber() % 20))
			{
				if (aiChooseSensorTarget((BASE_OBJECT *)psStructure, &psChosenObj))
				{
					debug( LOG_ATTACK, "Struct(%d) attacking : %d\n",
						psStructure->id, psChosenObj->id );
					setStructureTarget(psStructure, psChosenObj, 0);
				}
				else
				{
					setStructureTarget(psStructure, NULL, 0);
				}
			}
			psChosenObj = psStructure->psTarget[0];
		}
		else
		{
			psChosenObj = psStructure->psTarget[0];
		}

		// you can always see anything that a CB sensor is targeting
		// Anyone commenting this out again will get a knee capping from John.
		// You have been warned!!
		if ((structCBSensor(psStructure) || structVTOLCBSensor(psStructure)) &&
			psStructure->psTarget[0] != NULL)
		{
			psStructure->psTarget[0]->visible[psStructure->player] = UBYTE_MAX;
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
		if (checkPower(psStructure->player, POWER_PER_CYCLE, false))
		{
			//check if this structure is due some power
			if (getLastPowered((BASE_OBJECT *)psStructure))
			{
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
		case REF_RESEARCH:
		{
			pSubject = psStructure->pFunctionality->researchFacility.psSubject;
			structureMode = REF_RESEARCH;
			break;
		}
		case REF_FACTORY:
		case REF_CYBORG_FACTORY:
		case REF_VTOL_FACTORY:
		{
			pSubject = psStructure->pFunctionality->factory.psSubject;
			structureMode = REF_FACTORY;
			//check here to see if the factory's commander has died
			if (psStructure->pFunctionality->factory.psCommander &&
				psStructure->pFunctionality->factory.psCommander->died)
			{
				//remove the commander from the factory
				assignFactoryCommandDroid(psStructure, NULL);
			}
			break;
		}
		case REF_REPAIR_FACILITY:
		{
			psRepairFac = &psStructure->pFunctionality->repairFacility;
			psChosenObj = psRepairFac->psObj;
			structureMode = REF_REPAIR_FACILITY;
			psDroid = (DROID *)psChosenObj;

			// skip droids that are doing anything else
			if (psDroid != NULL
			    && (!orderState(psDroid, DORDER_RTR)
			        || psDroid->psTarget != (BASE_OBJECT *)psStructure))
			{
				psChosenObj = NULL;
				psDroid = NULL;
				psRepairFac->psObj = NULL;
			}

			/* select next droid if none being repaired */
			if ( psChosenObj == NULL )
			{
				ASSERT( psRepairFac->psGroup != NULL,
					"aiUpdateStructure: invalid repair facility group pointer" );

				mindist = SDWORD_MAX;
				for(psDroid = apsDroidLists[psStructure->player]; psDroid; psDroid = psDroid->psNext)
				{
					BASE_OBJECT * const psTarget = orderStateObj(psDroid, DORDER_RTR);

					if (psTarget
					 && psTarget == (BASE_OBJECT *)psStructure
					 && psDroid->action == DACTION_WAITFORREPAIR)
					{
						xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
						ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
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
				actionDroidObjLoc( psDroid, DACTION_MOVETOREPAIRPOINT,
						(BASE_OBJECT *) psStructure, psStructure->pos.x, psStructure->pos.y);
				/* reset repair started */
				psRepairFac->timeStarted = ACTION_START_TIME;
				psRepairFac->currentPtsAdded = 0;
				break;
			}
			break;
		}
		case REF_REARM_PAD:
		{
			psReArmPad = &psStructure->pFunctionality->rearmPad;
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
					actionDroidObj( psDroid, DACTION_MOVETOREARMPOINT, (BASE_OBJECT *)psStructure);
				}
			}
			else
			{
				psDroid = (DROID *) psChosenObj;
				if ( (psDroid->sMove.Status == MOVEINACTIVE ||
					psDroid->sMove.Status == MOVEHOVER       ) &&
					psDroid->action == DACTION_WAITFORREARM        )
				{
					actionDroidObj( psDroid, DACTION_MOVETOREARMPOINT, (BASE_OBJECT *)psStructure);
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
			break;
		}
		default:
			break;
	}

	/* check subject stats (for research or manufacture) */
	if (pSubject != NULL)
	{
		//if subject is research...
		if (structureMode == REF_RESEARCH)
		{
			psResFacility = &psStructure->pFunctionality->researchFacility;

			//if on hold don't do anything
			if (psResFacility->timeStartHold)
			{
				return;
			}

			//electronic warfare affects the functionality of some structures in multiPlayer
			if (bMultiPlayer)
			{
				if (psStructure->resistance < (SWORD)structureResistance(psStructure->
					pStructureType, psStructure->player))
				{
					return;
				}
			}

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
				if ((pointsToAdd + pPlayerRes->currentPoints) > (
					(RESEARCH *)pSubject)->researchPoints)

				{
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
						psStructure->player, true,psStructure);
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
			psFactory = &psStructure->pFunctionality->factory;
			Quantity = psFactory->quantity;

			//if on hold don't do anything
			if (psFactory->timeStartHold)
			{
				return;
			}

			//electronic warfare affects the functionality of some structures in multiPlayer
			if (bMultiPlayer)
			{
				if (psStructure->resistance < (SWORD)structureResistance(psStructure->
					pStructureType, psStructure->player))
				{
					return;
				}
			}

			if (psFactory->timeStarted == ACTION_START_TIME)
			{
				// also need to check if a command droid's group is full

				// If the factory commanders group is full - return
				if (IsFactoryCommanderGroupFull(psFactory)==true)
				{
					return;
				}

				if(CheckHaltOnMaxUnitsReached(psStructure) == true) {
					return;
				}
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
				//set the time started
				psFactory->timeStarted = gameTime;
			}

			pointsToAdd = (gameTime - psFactory->timeStarted) / GAME_TICKS_PER_SEC;

			//check for manufacture to be complete
			if ((pointsToAdd > psFactory->timeToBuild) &&
				!IsFactoryCommanderGroupFull(psFactory) &&
				!CheckHaltOnMaxUnitsReached(psStructure))
			{
				/* Place the droid on the map */
				bDroidPlaced = structPlaceDroid(psStructure, (DROID_TEMPLATE *)pSubject, &psDroid);

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

						//script callback, must be called after factory was flagged as idle
						if(bDroidPlaced)
						{
							cbNewDroid(psStructure, psDroid);
						}
					}
				}
				else
#endif
				{
					//decrement the quantity to manufacture if not set to infinity
					if (Quantity && Quantity != NON_STOP_PRODUCTION)
					{
						psFactory->quantity--;
						Quantity--;
					}

					// If quantity not 0 then kick of another manufacture.
					if(Quantity)
					{
						// Manufacture another.
						structSetManufacture(psStructure, (DROID_TEMPLATE*)pSubject,Quantity);
						return;
					}
					else
					{
						//when quantity = 0, reset the Subject and Tab on HCI Form
						psFactory->psSubject = NULL;
						intManufactureFinished(psStructure);

						//script callback, must be called after factory was flagged as idle
						if(bDroidPlaced)
						{
							cbNewDroid(psStructure, psDroid);
						}
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
			UDWORD  powerCost;

			psDroid = (DROID *) psChosenObj;
			ASSERT( psDroid != NULL,
					"aiUpdateStructure: invalid droid pointer" );
			psRepairFac = &psStructure->pFunctionality->repairFacility;

			if ( psDroid->action == DACTION_WAITDURINGREPAIR &&
				actionTargetTurret((BASE_OBJECT*)psStructure, psChosenObj,
									&(psStructure->turretRotation[0]),
									&(psStructure->turretPitch[0]),
									NULL, false, 0))
			{
				//check droid is not healthy
				if (psDroid->body < psDroid->originalBody)
				{
					//if in multiPlayer, and a Transporter - make sure its on the ground before repairing
					if (bMultiPlayer && psDroid->droidType == DROID_TRANSPORTER)
					{
						if (!(psDroid->sMove.Status == MOVEINACTIVE &&
							psDroid->sMove.iVertSpeed == 0))
						{
							return;
						}
					}

					//don't do anything if the resistance is low in multiplayer
					if (bMultiPlayer)
					{
						if (psStructure->resistance < (SWORD)structureResistance(psStructure->
							pStructureType, psStructure->player))
						{
							return;
						}
					}

					//check if enough power to do any
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
					bFinishAction = false;

					//do some repair
					if (pointsToAdd)
					{
						//just add the points if the power cost is negligable
						//if these points would make the droid healthy again then just add
						if (!powerCost || (psDroid->body + pointsToAdd >= psDroid->originalBody))
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
					debug( LOG_NEVER, "aiUpdateStructure: repair completed\n" );

					psRepairFac->psObj = NULL;

					/* set droid points to max */
					psDroid->body = psDroid->originalBody;
					//reset the power accrued
					psDroid->powerAccrued = 0;

					// if completely repaired reset order
					secondarySetState(psDroid, DSO_RETURN_TO_LOC, DSS_NONE);

					if ((psDroid->psGroup != NULL) &&
						(psDroid->psGroup->type == GT_COMMAND) &&
						(psDroid->droidType != DROID_COMMAND))
					{
						// return a droid to it's command group
						DROID	*psCommander = psDroid->psGroup->psCommander;

						orderDroidObj(psDroid, DORDER_GUARD, (BASE_OBJECT *)psCommander);
					}
					else if (psRepairFac->psDeliveryPoint != NULL)
					{
						// move the droid out the way
						orderDroidLoc( psDroid, DORDER_MOVE,
							psRepairFac->psDeliveryPoint->coords.x,
							psRepairFac->psDeliveryPoint->coords.y );
					}
				}

				if (psStructure->visible[selectedPlayer] && psDroid->visible[selectedPlayer])
				{
					/* add plasma repair effect whilst being repaired */
					iVecEffect.x = psDroid->pos.x + (10-rand()%20);
					iVecEffect.y = psDroid->pos.z + (10-rand()%20);
					iVecEffect.z = psDroid->pos.y + (10-rand()%20);
					effectSetSize(100);
					addEffect( &iVecEffect,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,
								true,getImdFromIndex(MI_FLAME),0 );
				}
			}
		}
		//check for rearming
		else if (structureMode == REF_REARM_PAD)
		{
			psReArmPad = &psStructure->pFunctionality->rearmPad;

			psDroid = (DROID *)psChosenObj;
			ASSERT( psDroid != NULL,
					"aiUpdateStructure: invalid droid pointer" );
			ASSERT( vtolDroid(psDroid),"aiUpdateStructure: invalid droid type" );

			//check hasn't died whilst waiting to be rearmed
			// also clear out any previously repaired droid
			if ( psDroid->died ||
					( psDroid->action != DACTION_MOVETOREARMPOINT &&
					  psDroid->action != DACTION_WAITDURINGREARM ) )
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

				bFinishAction = false;

				// dont rearm on remote pcs.
				if(!bMultiPlayer || myResponsibility(psDroid->player))
				{
					/* do rearming */
					if (psDroid->sMove.iAttackRuns != 0)
					{
						UDWORD      pointsRequired;

						//amount required is a factor of the droids' weight
						pointsRequired = psDroid->weight / REARM_FACTOR;
						//Watermelon:take numWeaps into consideration
						pointsToAdd = psReArmPad->reArmPoints * (gameTime - psReArmPad->timeStarted) * psDroid->numWeaps /
							GAME_TICKS_PER_SEC;
						//if ((SDWORD)(psDroid->sMove.iAttackRuns - pointsToAdd) <= 0)
						if (pointsToAdd >= pointsRequired)
						{
							for (i = 0;i < psDroid->numWeaps;i++)
							{
								/* set rearm value to no runs made */
								psDroid->sMove.iAttackRuns[i] = 0;
								//reset ammo and lastTimeFired
								psDroid->asWeaps[i].ammo = asWeaponStats[psDroid->
									asWeaps[i].nStat].numRounds;
								psDroid->asWeaps[i].lastFired = 0;
							}
						}
						else
						{
							for (i = 0;i < psDroid->numWeaps;i++)
							{
								//Watermelon:make sure that slot is depleted and dont divide integer by zero
								if ( psDroid->sMove.iAttackRuns[i] > 0 )
								{
									if (pointsToAdd >= pointsRequired/psDroid->sMove.iAttackRuns[i])
									{
										psDroid->sMove.iAttackRuns[i]--;
									}
								}
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
					if( bMultiPlayer)
					{
						sendHappyVtol(psDroid);
					}

					//clear the rearm pad
					psDroid->action = DACTION_NONE;
					bFinishAction = true;
					psReArmPad->psObj = NULL;

				}
			}
		}
	}
}


/* Decides whether a structure should emit smoke when it's damaged */
static BOOL canSmoke(STRUCTURE *psStruct)
{
	if(psStruct->pStructureType->type == REF_WALL ||
		psStruct->pStructureType->type == REF_WALLCORNER)
	{
		return(false);
	}
	else
	{
		return(true);
	}
}

static float CalcStructureSmokeInterval(float damage)
{
	return (((1. - damage) + 0.1) * 10) * STRUCTURE_DAMAGE_SCALING;
}

/* The main update routine for all Structures */
void structureUpdate(STRUCTURE *psBuilding)
{
	UDWORD widthScatter,breadthScatter;
	UDWORD emissionInterval, iPointsToAdd, iPointsRequired;
	Vector3i dv;

	//update the manufacture/research of the building once complete
	if (psBuilding->status == SS_BUILT)
	{
		aiUpdateStructure(psBuilding);
	}

	// must be after aiUpdateStructure because this is where we clean out dead targets
	CHECK_STRUCTURE(psBuilding);

	if(psBuilding->status!=SS_BUILT)
	{
		if(psBuilding->selected)
		{
			psBuilding->selected = false;
		}
	}

	/* Only add smoke if they're visible and they can 'burn' */
	if(psBuilding->visible[selectedPlayer] && canSmoke(psBuilding))
	{
		const float damage = getStructureDamage(psBuilding);

		// Is there any damage?
		if (damage > 0.)
		{
			emissionInterval = CalcStructureSmokeInterval(damage);
			if(gameTime > (psBuilding->lastEmission + emissionInterval))
			{
				widthScatter = ((psBuilding->pStructureType->baseWidth) * TILE_UNITS/2)/3;
				breadthScatter = ((psBuilding->pStructureType->baseBreadth) * TILE_UNITS/2)/3;
				dv.x = psBuilding->pos.x + widthScatter - rand()%(2*widthScatter);
				dv.z = psBuilding->pos.y + breadthScatter - rand()%(2*breadthScatter);
				dv.y = psBuilding->pos.z;
				dv.y += (psBuilding->sDisplay.imd->max.y * 3) / 4;
				addEffect(&dv,EFFECT_SMOKE,SMOKE_TYPE_DRIFTING_HIGH,false,NULL,0);
				psBuilding->lastEmission = gameTime;
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

			//in multiplayer, certain structures do not function whilst low resistance
			if (bMultiPlayer)
			{
				resetResistanceLag(psBuilding);
			}

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
		if (selfRepairEnabled(psBuilding->player) && (psBuilding->body < (SWORD)
			iPointsRequired))
		{
			//start the self repair off
			if (psBuilding->lastResistance == ACTION_START_TIME)
			{
				psBuilding->lastResistance = gameTime;
			}

			/*since self repair, then add half repair points depending on the time delay for the stat*/
			iPointsToAdd = (repairPoints(asRepairStats + aDefaultRepair[
				psBuilding->player], psBuilding->player) / 4) * ((gameTime -
				psBuilding->lastResistance) / (asRepairStats +
				aDefaultRepair[psBuilding->player])->time);

			//add the blue flashing effect for multiPlayer
			if(bMultiPlayer && ONEINTEN)
			{
				Vector3i position;
				Vector3f *point;
				SDWORD	realY;
				UDWORD	pointIndex;

				pointIndex = rand()%(psBuilding->sDisplay.imd->npoints-1);
				point = &(psBuilding->sDisplay.imd->points[pointIndex]);
				position.x = psBuilding->pos.x + point->x;
				realY = structHeightScale(psBuilding) * point->y;
				position.y = psBuilding->pos.z + realY;
				position.z = psBuilding->pos.y - point->z;

				effectSetSize(30);
				addEffect(&position, EFFECT_EXPLOSION, EXPLOSION_TYPE_SPECIFIED, true,
					getImdFromIndex(MI_PLASMA), 0);
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
	CHECK_STRUCTURE(psBuilding);
}


/* Release all resources associated with a structure */
void structureRelease(STRUCTURE *psBuilding)
{
	/* remove animation if present */
	if ( psBuilding->psCurAnim != NULL )
	{
		animObj_Remove( &psBuilding->psCurAnim, psBuilding->psCurAnim->psAnim->uwID );
		psBuilding->psCurAnim = NULL;
	}

	// free up the space used by the functionality array
	free(psBuilding->pFunctionality);
	psBuilding->pFunctionality = NULL;
}


/*
fills the list with Structure that can be built. There is a limit on how many can
be built at any one time. Pass back the number available.
There is now a limit of how many of each type of structure are allowed per mission
*/
UDWORD fillStructureList(STRUCTURE_STATS **ppList, UDWORD selectedPlayer, UDWORD limit)
{
	UDWORD			inc, count;
	BOOL			researchModule, factoryModule, powerModule;
	STRUCTURE		*psCurr;
	STRUCTURE_STATS	*psBuilding;

	//check to see if able to build research/factory modules
	researchModule = factoryModule = powerModule = false;

	//if currently on a mission can't build factory/research/power/derricks
	if (!missionIsOffworld())
	{
		for (psCurr = apsStructLists[selectedPlayer]; psCurr != NULL; psCurr =
			psCurr->psNext)
		{
			if (psCurr->pStructureType->type == REF_RESEARCH && psCurr->status ==
				SS_BUILT)
			{
				researchModule = true;
			}
			else if (psCurr->pStructureType->type == REF_FACTORY && psCurr->status ==
				SS_BUILT)
			{
				factoryModule = true;
			}
			else if (psCurr->pStructureType->type == REF_POWER_GEN && psCurr->status == SS_BUILT)
			{
				powerModule = true;
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
			//check not built the maximum allowed already
			if (asStructLimits[selectedPlayer][inc].currentQuantity < asStructLimits[selectedPlayer][inc].limit)
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
				if (missionIsOffworld())
				{
					if (psBuilding->type == REF_FACTORY ||
						psBuilding->type == REF_POWER_GEN ||
						psBuilding->type == REF_RESOURCE_EXTRACTOR ||
						psBuilding->type == REF_RESEARCH ||
						psBuilding->type == REF_CYBORG_FACTORY ||
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
				if (psBuilding->type == REF_FACTORY ||
					psBuilding->type == REF_CYBORG_FACTORY ||
					psBuilding->type == REF_VTOL_FACTORY)
				{
					//NEVER EVER EVER WANT MORE THAN 5 FACTORIES
					if (asStructLimits[selectedPlayer][inc].currentQuantity >= MAX_FACTORY)
					{
						continue;
					}
				}
				//HARD_CODE don't ever want more than one Las Sat structure
				if (isLasSat(psBuilding) && getLasSatExists(selectedPlayer))
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

				debug( LOG_NEVER, "fillStructureList: adding %s (%x)\n",
					psBuilding->pName, apStructTypeLists[selectedPlayer][inc]);
				ppList[count++] = psBuilding;
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
	BOOL				valid = true;
	SDWORD				i, j;
	UDWORD				min, max;
	HIGHLIGHT			site;
	FLAG_POSITION		*psCurrFlag;

	//make sure we are not too near map edge and not going to go over it
	if( !tileInsideBuildRange((SDWORD)x, (SDWORD)y) )
	{
		return false;
	}

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
		if (wallDrag.status != DRAG_INACTIVE
		    && (psBuilding->type == REF_WALL || psBuilding->type == REF_DEFENSE)
		    && !isLasSat(psBuilding))
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
	else
	{
		// If the stat's not a structure then assume it's size is 1x1.
		site.xTL = (UWORD)x;
		site.yTL = (UWORD)y;
		site.xBR = (UWORD)x;
		site.yBR = (UWORD)y;
	}

	for (i = site.xTL; i <= site.xBR && valid; i++) {
		for (j = site.yTL; j <= site.yBR && valid; j++) {
			// Can't build outside of scroll limits.
			if( ((SDWORD)i < scrollMinX+2) || ((SDWORD)i > scrollMaxX-5) ||
				((SDWORD)j < scrollMinY+2) || ((SDWORD)j > scrollMaxY-5)) {
				valid = false;
				goto failed;
			}

			// check i or j off map.
			if(i<=2 || j<=2 || i>=(SDWORD)mapWidth-2 || j>=(SDWORD)mapHeight-2)
			{
				valid = false;
				goto failed;
			}

#ifdef NDEBUG
			/*God Awful HACK!! - AB 30/04/99 - gets round a problem with
			UrbanDuel map where an oil derrick cannot be built - when the
			map has been edited this hack can be removed*/
			if (psBuilding->type != REF_RESOURCE_EXTRACTOR)
			{
				if( gwZoneReachable(gwGetZone(i,j)) == false)
				{
					// Can't ever drive there
					valid = false;
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
						valid = false;
						goto failed;
					}
				}
			}
		}
	}

	// cant place on top of a delivery point...
	for (psCurrFlag = apsFlagPosLists[selectedPlayer]; psCurrFlag; psCurrFlag = psCurrFlag->psNext)
	{
		ASSERT(psCurrFlag->coords.x != ~0, "flag has invalid position");
		i = map_coord(psCurrFlag->coords.x);
		j = map_coord(psCurrFlag->coords.y);

		if (i >= site.xTL && i <= site.xBR &&
			j >= site.yTL && j <= site.yBR)
		{
			valid = false;
			goto failed;
		}
	}

	// can't build next to a repair facility
	for (psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_REPAIR_FACILITY)
		{
			// get the top left of the struct
			i = map_coord(psStruct->pos.x) - 1;
			j = map_coord(psStruct->pos.y) - 1;

			// see if the x extents overlap
			if ((site.xTL >= i && site.xTL <= (i+2)) ||
				(site.xBR >= i && site.xBR <= (i+2)))
			{
				// now see if y extents overlap
				if ((site.yTL >= j && site.yTL <= (j+2)) ||
					(site.yBR >= j && site.yBR <= (j+2)))
				{
					valid = false;
					goto failed;
				}
			}
		}
	}

	if (psStats->ref >= REF_STRUCTURE_START &&
		psStats->ref < (REF_STRUCTURE_START + REF_RANGE))
	{
		MAPTILE	*psTile;

		switch(psBuilding->type)
		{
			case REF_DEMOLISH:
				break;
			case NUM_DIFF_BUILDINGS:
			case REF_BRIDGE:
				ASSERT(!"invalid structure type", "validLocation: Bad structure type %u", psBuilding->type);
				break;
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
			case REF_SAT_UPLINK:
				/*need to check each tile the structure will sit on is not water*/
				for (i = site.xTL; i <= site.xBR && valid; i++)
				{
					for (j = site.yTL; j <= site.yBR && valid; j++)
					{
						psTile = mapTile(i,j);
						if ((terrainType(psTile) == TER_WATER) ||
							(terrainType(psTile) == TER_CLIFFFACE) )
						{
							valid = false;
						}
					}
				}
				//don't bother checking if already found a problem
				if (valid)
				{
					//check not within landing zone
					for (i = site.xTL; i <= site.xBR && valid; i++)
					{
						for (j = site.yTL; j <= site.yBR && valid; j++)
						{
							if (withinLandingZone(i, j))
							{
								valid = false;
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
						valid = false;
					}
					if (valid &&
						!noDroid(x,y))
					{
						valid = false;
					}
				}

				if (valid &&	// only do if necessary
					(psBuilding->type != REF_REPAIR_FACILITY))
				{
					for (i = site.xTL; i <= site.xBR && valid; i++)
					{
						for (j = site.yTL; j <= site.yBR && valid; j++)
						{
							// This really needs to check to see if the droid that's in the way is the droid that wants to build
							// in which case it should'nt invalidate the location.
							if(noDroid(i,j) == false)
							{
								valid = false;
							}
						}
					}
				}

				//walls/defensive structures can be built along any ground
				if (valid &&	// only do if necessary
					(!(psBuilding->type == REF_REPAIR_FACILITY ||
					psBuilding->type == REF_DEFENSE ||
					psBuilding->type == REF_WALL)))
				{
					/*cannot build on ground that is too steep*/
					min = 0;
					max = 0;
					for (i = site.xTL; i <= site.xBR && valid; i++)
					{
						for (j = site.yTL; j <= site.yBR && valid; j++)
						{
							getTileMaxMin(i, j, &max, &min);
							if ((max - min) > MAX_INCLINE)
							{
								valid = false;
							}
						}
					}
				}
				//don't bother checking if already found a problem
				if (valid)
				{
					//on PC - defence structures can be built next to anything now- AB 22/09/98
					//and the Missile_Silo (special case) - AB 01/03/99
					if (!(psBuilding->type == REF_DEFENSE ||
						psBuilding->type == REF_WALL ||
						psBuilding->type == REF_WALLCORNER ||
						psBuilding->type == REF_MISSILE_SILO))
					{
						/*need to check there is one tile between buildings*/
						for (i = (UWORD)(site.xTL-1); i <= (UWORD)(site.xBR+1); i++)
						{
							for (j = (UWORD)(site.yTL-1); j <= (UWORD)(site.yBR+1); j++)
							{
								//skip the actual area the structure will cover
								if (i < site.xTL || i > site.xBR ||
									j < site.yTL || j > site.yBR)
								{
									if (TILE_HAS_STRUCTURE(mapTile(i,j)))
									{
										psStruct = getTileStructure(i,j);
										if (psStruct)
										{
											//you can build anything next to a defensive structure
											if ((psStruct->pStructureType->type != REF_DEFENSE) &&
												(psStruct->pStructureType->type != REF_WALL)	&&
												(psStruct->pStructureType->type != REF_WALLCORNER)
											)
											{
												//Walls can be built next to walls and defenses - AB 03/03/99
												if (psBuilding->type == REF_WALL)
												{
													if (!(psStruct->pStructureType->type == REF_WALL ||
													psStruct->pStructureType->type == REF_WALLCORNER))
													{
														valid = false;
													}
												}
												else
												{
													valid = false;
												}
											}
											else	// is a defense.
											{		// skirmish players don't build defensives next to each other.(route hack)
												if( bMultiPlayer && game.type == SKIRMISH && !isHumanPlayer(player) )
												{
													valid = false;
												}
											}
										}
									}
									//cannot build within one tile of a oil resource
									if(TILE_HAS_FEATURE(mapTile(i,j)))
									{
										psFeat = getTileFeature(i,j);
										if (psFeat && psFeat->psStats->subType ==
											FEAT_OIL_RESOURCE)
										{
											valid = false;
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
					for (i = site.xTL; i <= site.xBR && valid; i++)
					{
						for (j = site.yTL; j <= site.yBR && valid; j++)
						{
							psTile = mapTile(i,j);
							if (TILE_OCCUPIED(psTile))
							{
								if (TILE_HAS_WALL(psTile)
								    && (psBuilding->type == REF_DEFENSE ||
-                                                                       psBuilding->type == REF_WALL))
								{
									psStruct = getTileStructure(i,j);
									if (psStruct != NULL &&
										psStruct->player != player)
									{
										valid = false;
									}
								}
								else
								{
									valid = false;
								}
							}
						}
					}
				}
				break;
			case REF_FACTORY_MODULE:
				valid = false;
				if(TILE_HAS_STRUCTURE(mapTile(x,y)))
				{
					psStruct = getTileStructure(x,y);
					if(psStruct && (
						psStruct->pStructureType->type == REF_FACTORY ||
						psStruct->pStructureType->type == REF_VTOL_FACTORY) &&
						psStruct->status == SS_BUILT)
					{
						valid = true;
					}
				}
				break;
			case REF_RESEARCH_MODULE:
				valid = false;
				//check that there is a research facility at the location
				if(TILE_HAS_STRUCTURE(mapTile(x,y)))
				{
					psStruct = getTileStructure(x,y);
					if(psStruct && psStruct->pStructureType->type == REF_RESEARCH &&
						psStruct->status == SS_BUILT)
					{
						valid = true;
					}
				}

				break;
			case REF_POWER_MODULE:
				valid = false;
				if(TILE_HAS_STRUCTURE(mapTile(x,y)))
				{
					psStruct = getTileStructure(x,y);
					if(psStruct && psStruct->pStructureType->type == REF_POWER_GEN &&
						psStruct->status == SS_BUILT)
					{
						valid = true;
					}
				}
				break;
			case REF_RESOURCE_EXTRACTOR:
				valid = false;
				//check that there is a oil resource at the location
				if(TILE_HAS_FEATURE(mapTile(x,y)))
				{
					psFeat = getTileFeature(x,y);
					if(psFeat && psFeat->psStats->subType == FEAT_OIL_RESOURCE)
					{
						valid = true;
					}
				}
				break;
		}
		//if setting up a build queue need to check against future sites as well - AB 4/5/99
		if (ctrlShiftDown() && player == selectedPlayer && bCheckBuildQueue)
		{
			DROID   *psDroid;
			SDWORD  order,left,right,up,down,size;
			BOOL    validCombi;

			//defense and missile silo's can be built next to anything so don't need to check
			if (!(psBuilding->type == REF_DEFENSE || psBuilding->type ==
				REF_MISSILE_SILO))
			{
				for (psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
				{
					//once its invalid stop checking
					if (valid == false)
					{
						break;
					}
					if (psDroid->droidType == DROID_CONSTRUCT ||
						psDroid->droidType == DROID_CYBORG_CONSTRUCT)
					{
						//look thru' the list of orders to see if more building sites
						for (order = 0; order < psDroid->listSize; order++)
						{
							if (psDroid->asOrderList[order].order == DORDER_BUILD)
							{
								validCombi = false;
								if (((STRUCTURE_STATS *)psDroid->asOrderList[order].
									psOrderTarget)->type == REF_DEFENSE ||
									((STRUCTURE_STATS *)psDroid->asOrderList[order].
									psOrderTarget)->type == REF_MISSILE_SILO)
								{
									validCombi = true;
								}
								//walls can be built next to walls and defence
								if ((psBuilding->type == REF_WALL || psBuilding->type == REF_WALLCORNER)
					&& (((STRUCTURE_STATS *)psDroid->asOrderList[order].psOrderTarget)->type == REF_WALL
					|| ((STRUCTURE_STATS *)psDroid->asOrderList[order].psOrderTarget)->type == REF_WALLCORNER))
								{
									validCombi = true;
								}
								//don't bother checking if valid combination of building types
								if (!validCombi)
								{
									/*need to check there is one tile between buildings*/
									//check if any corner is within the build site
									size = ((STRUCTURE_STATS *)psDroid->asOrderList[order].
										psOrderTarget)->baseWidth;
									left = map_coord(psDroid->asOrderList[order].x) - size/2;
									right = left + size;
									size = ((STRUCTURE_STATS *)psDroid->asOrderList[order].
										psOrderTarget)->baseBreadth;
									up = map_coord(psDroid->asOrderList[order].y) - size/2;
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
									if (((left > site.xTL-1 && left <= site.xBR+1) &&
										(up > site.yTL-1 && up <= site.yBR+1)) ||
										((right > site.xTL-1 && right <= site.xBR+1) &&
										(up > site.yTL-1 && up <= site.yBR+1)) ||
										((left > site.xTL-1 && left <= site.xBR+1) &&
										(down > site.yTL-1 && down <= site.yBR+1)) ||
										((right > site.xTL-1 && right <= site.xBR+1) &&
										(down > site.yTL-1 && down <= site.yBR+1)))
									{
										valid = false;
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
		valid = true;
		if (fpathGroundBlockingTile(x,y))
		{
			valid = false;
		}
	}

failed:
	if (!valid)
	{
		// Only set the hilight colour if it's the selected player.
		if(player == selectedPlayer)
		{
			outlineTile = false;
		}

		return false;
	}

	// Only set the hilight colour if it's the selected player.
	if (player == selectedPlayer)
	{
		outlineTile = true;
	}

	return true;
}

/*
for a new structure, find a location along an edge which the droid can get
to and return this as the destination for the droid.
*/
BOOL getDroidDestination(BASE_STATS *psStats, UDWORD structX,
	UDWORD structY, UDWORD *pDroidX, UDWORD *pDroidY)
{
	UWORD				start;
	UDWORD				structTileX, structTileY, width = 0, breadth = 0;

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

	//get a random starting place 0=top left
	start = (UWORD)(rand() % ((width + breadth) * 2));

	//search in a clockwise direction around the structure from the starting point
	if (start == 0 || start < width)
	{
		//top side first
		structTileX = map_coord(structX);
		structTileY = map_coord(structY) - 1;
		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX += width;
		structTileY += 1;

		if (checkLength(breadth, structTileX, structTileY,pDroidX, pDroidY))
		{
			return true;
		}
		structTileX = map_coord(structX);
		structTileY += breadth;

		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX -= 1;
		structTileY = map_coord(structY);

		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
	}
	else if (start == width || start < (width + breadth))
	{
		//right side first
		structTileX = (map_coord(structX)) + width;
		structTileY = map_coord(structY);

		if (checkLength(breadth, structTileX, structTileY,pDroidX, pDroidY))
		{
			return true;
		}
		structTileX = map_coord(structX);
		structTileY += breadth;

		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX -= 1;
		structTileY = map_coord(structY);

		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX += 1;
		structTileY -= 1;

		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
	}
	else if (start == (width + breadth) || start < (width * breadth))
	{
		//bottom first
		structTileX = map_coord(structX);
		structTileY = map_coord(structY) + breadth;

		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX -= 1;
		structTileY = map_coord(structY);

		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX += 1;
		structTileY -= 1;

		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX += width;
		structTileY += 1;

		if (checkLength(breadth, structTileX, structTileY,pDroidX, pDroidY))
		{
			return true;
		}
	}
	else
	{
		//left side first
		structTileX = (map_coord(structX)) - 1;
		structTileY = map_coord(structY);

		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX += 1;
		structTileY -= 1;

		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX += width;
		structTileY += 1;

		if (checkLength(breadth, structTileX, structTileY,pDroidX, pDroidY))
		{
			return true;
		}
		structTileX = map_coord(structX);
		structTileY += breadth;

		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
	}

	//not found a valid location so return false
	return false;
}

/* check along the width of a structure for an empty space */
BOOL checkWidth(UDWORD maxRange, UDWORD x, UDWORD y, UDWORD *pDroidX, UDWORD *pDroidY)
{
	UDWORD		side;

	for (side = 0; side < maxRange; side++)
	{
		if( x+side < mapWidth && y < mapHeight && !TILE_OCCUPIED(mapTile(x+side,y)) )
		{
			*pDroidX = world_coord(x + side);
			*pDroidY = world_coord(y);

			ASSERT( worldOnMap(*pDroidX,*pDroidY),"checkWidth : Insane droid position" );

			return true;
		}
	}

	return false;
}

/* check along the length of a structure for an empty space */
BOOL checkLength(UDWORD maxRange, UDWORD x, UDWORD y, UDWORD *pDroidX, UDWORD *pDroidY)
{
	UDWORD		side;

	for (side = 0; side < maxRange; side++)
	{
		if(y+side < mapHeight && x < mapWidth && !TILE_OCCUPIED(mapTile(x,y+side)) )
		{
			*pDroidX = world_coord(x);
			*pDroidY = world_coord(y + side);

			ASSERT( worldOnMap(*pDroidX,*pDroidY),"checkHeight : Insane droid position" );

			return true;
		}
	}

	return false;
}


//remove a structure from the map
static void removeStructFromMap(STRUCTURE *psStruct)
{
	UDWORD		i,j;
	UDWORD		mapX, mapY;
	MAPTILE		*psTile;

	/* set tiles drawing */
	mapX = map_coord(psStruct->pos.x - psStruct->pStructureType->baseWidth * TILE_UNITS / 2);
	mapY = map_coord(psStruct->pos.y - psStruct->pStructureType->baseBreadth * TILE_UNITS / 2);
	for (i = 0; i < psStruct->pStructureType->baseWidth; i++)
	{
		for (j = 0; j < psStruct->pStructureType->baseBreadth; j++)
		{
			psTile = mapTile(mapX+i, mapY+j);
			psTile->psObject = NULL;
			CLEAR_TILE_NOTBLOCKING(psTile);
		}
	}
}

// remove a structure from a game without any visible effects
// bDestroy = true if the object is to be destroyed
// (for example used to change the type of wall at a location)
BOOL removeStruct(STRUCTURE *psDel, BOOL bDestroy)
{
	BOOL		resourceFound = false;
	UBYTE		mask;
	FACTORY		*psFactory;
	SDWORD		cluster;
	FLAG_POSITION	*psAssemblyPoint=NULL;

	ASSERT( psDel != NULL, "destroyStruct: invalid structure pointer\n" );

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
		HOW MUCH IS THERE && NOT RES EXTRACTORS */
		if (psDel->pStructureType->type == REF_RESOURCE_EXTRACTOR)
		{
			if (psDel->pFunctionality->resourceExtractor.power)
			{
				buildFeature(&asFeatureStats[oilResFeature], psDel->pos.x, psDel->pos.y, false);
				resourceFound = true;
			}
		}
	}

	if (psDel->pStructureType->type == REF_RESOURCE_EXTRACTOR)
	{
		//tell associated Power Gen
		releaseResExtractor(psDel);
	}

	if (psDel->pStructureType->type == REF_POWER_GEN)
	{
		//tell associated Res Extractors
		releasePowerGen(psDel);
	}

	//check for a research topic currently under way
	if (psDel->pStructureType->type == REF_RESEARCH)
	{
		if (psDel->pFunctionality->researchFacility.psSubject)
		{
			//cancel the topic
			cancelResearch(psDel);
		}
	}

	//subtract one from the structLimits list so can build another - don't allow to go less than zero!
	if (asStructLimits[psDel->player][psDel->pStructureType - asStructureStats].currentQuantity)
	{
		asStructLimits[psDel->player][psDel->pStructureType - asStructureStats].currentQuantity--;
	}

	//if it is a factory - need to reset the factoryNumFlag
	if (StructIsFactory(psDel))
	{
		psFactory = &psDel->pFunctionality->factory;

		//need to initialise the production run as well
		cancelProduction(psDel);

		psAssemblyPoint = psFactory->psAssemblyPoint;
	}
	else if (psDel->pStructureType->type == REF_REPAIR_FACILITY)
	{
		psAssemblyPoint = psDel->pFunctionality->repairFacility.psDeliveryPoint;
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

/* Remove a structure */
BOOL destroyStruct(STRUCTURE *psDel)
{
	UDWORD			mapX, mapY, width,breadth;
	UDWORD			i;
	UDWORD			widthScatter,breadthScatter,heightScatter;
	Vector3i pos;
	BOOL			resourceFound = false;
	MAPTILE			*psTile;
	BOOL			bMinor = false;

	CHECK_STRUCTURE(psDel);

	if (bMultiPlayer)
	{
		SendDestroyStructure(psDel);
	}

//---------------------------------------
	/* Only add if visible */
	if(psDel->visible[selectedPlayer])
	{
		/* Firstly, are we dealing with a wall section */
		if(psDel->pStructureType->type == REF_WALL || psDel->pStructureType->type == REF_WALLCORNER)
		{
			bMinor = true;
		}

//---------------------------------------  Do we add immediate explosions?
		/* Set off some explosions, but not for walls */
		/* First Explosions */
		widthScatter = TILE_UNITS;
		breadthScatter = TILE_UNITS;
		heightScatter = TILE_UNITS;
		for(i=0; i<(UDWORD)(bMinor ? 2 : 4); i++)	// only add two for walls - gets crazy otherwise
		{
			pos.x = psDel->pos.x + widthScatter - rand()%(2*widthScatter);
			pos.z = psDel->pos.y + breadthScatter - rand()%(2*breadthScatter);
			pos.y = psDel->pos.z + 32 + rand()%heightScatter;
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_MEDIUM,false,NULL,0);
		}

		/* Get coordinates for everybody! */
		pos.x = psDel->pos.x;
		pos.z = psDel->pos.y;
		pos.y = map_Height((UWORD)pos.x,(UWORD)pos.z);

//--------------------------------------- Do we add a fire?
		// Set off a fire, provide dimensions for the fire
		if(bMinor)
		{
			effectGiveAuxVar(world_coord(psDel->pStructureType->baseWidth) / 4);
		}
		else
		{
			effectGiveAuxVar(world_coord(psDel->pStructureType->baseWidth) / 3);
		}
		if(bMinor)							 // walls
		{
			/* Give a duration */
			effectGiveAuxVarSec(1000);
			/* Normal fire - no smoke */
			addEffect(&pos,EFFECT_FIRE,FIRE_TYPE_LOCALISED,false,NULL,0);

		}
		else if(psDel->pStructureType->type == REF_RESOURCE_EXTRACTOR) // oil resources
		{
			/* Oil resources burn AND puff out smoke AND for longer*/
			effectGiveAuxVarSec(60000);
			addEffect(&pos,EFFECT_FIRE,FIRE_TYPE_SMOKY,false,NULL,0);
		}
		else	// everything else
		{
			/* Give a duration */
			effectGiveAuxVarSec(10000);
			addEffect(&pos,EFFECT_FIRE,FIRE_TYPE_LOCALISED,false,NULL,0);
		}

//--------------------------------------- Do we add a destruction seq, and if so, which?
		/* Power stations have their own desctruction sequence */
		if(psDel->pStructureType->type == REF_POWER_GEN)
		{
			addEffect(&pos,EFFECT_DESTRUCTION,DESTRUCTION_TYPE_POWER_STATION,false,NULL,0);
			pos.y += SHOCK_WAVE_HEIGHT;
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SHOCKWAVE,false,NULL,0);
			// give some power back to the player.
			addPower(psDel->player, structPowerToBuild(psDel));
			//if it had a module attached, need to add the power for the base struct as well
			if (psDel->pFunctionality->powerGenerator.capacity)
			{
				addPower(psDel->player, psDel->pStructureType->powerToBuild);
			}
		}
		/* As do wall sections */
		else if(bMinor)
		{
			addEffect(&pos,EFFECT_DESTRUCTION,DESTRUCTION_TYPE_WALL_SECTION,false,NULL,0);
		}
		else // and everything else goes here.....
		{
			addEffect(&pos,EFFECT_DESTRUCTION,DESTRUCTION_TYPE_STRUCTURE,false,NULL,0);
		}

//--------------------------------------- Start an earthquake...!
		/* shake the screen if we're near enough */
		if(clipXY(pos.x,pos.z))
		{
			shakeStart();
		}

//--------------------------------------- And finally, add a boom sound!!!!
		/* and add a sound effect */
		audio_PlayStaticTrack( psDel->pos.x, psDel->pos.y, ID_SOUND_EXPLOSION );
	}
//---------------------------------------------------------------------------------------

	resourceFound = removeStruct(psDel, true);

	//once a struct is destroyed - it leaves a wrecked struct FEATURE in its place
	// Wall's don't leave wrecked features
	if(psDel->visible[selectedPlayer])
	{
		if (!resourceFound && !(psDel->pStructureType->type == REF_WALL) &&
			!(psDel->pStructureType->type == REF_WALLCORNER))
		{
			mapX = map_coord(psDel->pos.x - psDel->pStructureType->baseWidth * TILE_UNITS / 2);
			mapY = map_coord(psDel->pos.y - psDel->pStructureType->baseBreadth * TILE_UNITS / 2);
			for (width = 0; width < psDel->pStructureType->baseWidth; width++)
			{
				for (breadth = 0; breadth < psDel->pStructureType->baseBreadth; breadth++)
				{
					psTile = mapTile(mapX+width,mapY+breadth);
					if(TEST_TILE_VISIBLE(selectedPlayer,psTile))
					{
						psTile->illumination /= 2;
						if(psTile->bMaxed && psTile->level > 0) //only do one's already seen
						{
							psTile->level/=2;
						}
					}
				}
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
	if(psDel->pStructureType->type != REF_WALL && psDel->pStructureType->type != REF_WALLCORNER)
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

	return true;
}


/* For now all this does is work out what height the terrain needs to be set to
An actual foundation structure may end up being placed down
The x and y passed in are the CENTRE of the structure*/
SWORD buildFoundation(STRUCTURE_STATS *psStructStats, UDWORD x, UDWORD y)
{
	UDWORD	width, breadth;
	UDWORD	startX, startY;
	SWORD	height,foundationMin, foundationMax;

	startX = map_coord(x) - psStructStats->baseWidth/2;
	startY = map_coord(y) - psStructStats->baseBreadth/2;

	//check the terrain is the correct type return -1 if not

	//shouldn't need to do this but doesn't take long hey?!
	/*check if there is a structure next to the new one - return the height of the
	structure if found*/
	for (breadth = 0; breadth <= psStructStats->baseBreadth; breadth++)
	{
		for (width = 0; width <= psStructStats->baseWidth; width++)
		{
			if(TILE_HAS_STRUCTURE(mapTile(startX+width,startY+breadth)))
			{
				return((SWORD)map_TileHeight(startX+width,startY+breadth));
			}
		}
	}

	//may also have to check that overlapping terrain can be set to the average height
	//eg water - don't want it to 'flow' into the structure if this effect is coded!

	startX = map_coord(x) - psStructStats->baseWidth/2;
	startY = map_coord(y) - psStructStats->baseBreadth/2;

	//initialise the starting values so they get set in loop
	foundationMin = TILE_MAX_HEIGHT;
	foundationMax = TILE_MIN_HEIGHT;

	for (breadth = 0; breadth <= psStructStats->baseBreadth; breadth++)
	{
		for (width = 0; width <= psStructStats->baseWidth; width++)
		{
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
SDWORD getStructStatFromName(char *pName)
{
	UDWORD				inc;
	STRUCTURE_STATS		*psStat;

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


/*check to see if the structure is 'doing' anything  - return true if idle*/
BOOL  structureIdle(STRUCTURE *psBuilding)
{
	BASE_STATS		*pSubject = NULL;

	CHECK_STRUCTURE(psBuilding);

	if (psBuilding->pFunctionality == NULL)
		return true;

	//determine the Subject
	switch (psBuilding->pStructureType->type)
	{
		case REF_RESEARCH:
		{
			pSubject = psBuilding->pFunctionality->researchFacility.psSubject;
			break;
		}
		case REF_FACTORY:
		case REF_CYBORG_FACTORY:
		case REF_VTOL_FACTORY:
		{
			pSubject = psBuilding->pFunctionality->factory.psSubject;
			break;
		}
		default:
			break;
	}

	if (pSubject != NULL)
	{
		return false;
	}

	return true;
}


/*checks to see if any structure exists of a specified type with a specified status */
BOOL checkStructureStatus( STRUCTURE_STATS *psStats, UDWORD player, UDWORD status)
{
	STRUCTURE	*psStructure;
	BOOL		found = false;

	for (psStructure = apsStructLists[player]; psStructure != NULL;
		psStructure = psStructure->psNext)
	{
		if (psStructure->pStructureType->type == psStats->type)
		{
			//need to check if THIS instance of the type has the correct status
			if (psStructure->status == status)
			{
				found = true;
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
	BOOL		found = false;

	ASSERT( structInc < numStructureStats,
		"checkSpecificStructExists: invalid structure inc" );

	for (psStructure = apsStructLists[player]; psStructure != NULL;
		psStructure = psStructure->psNext)
	{
		if (psStructure->status == SS_BUILT)
		{
			if ((psStructure->pStructureType->ref - REF_STRUCTURE_START) ==
				structInc)
			{
				found = true;
				break;
			}
		}
	}
	return found;
}


/*finds a suitable position for the assembly point based on one passed in*/
void findAssemblyPointPosition(UDWORD *pX, UDWORD *pY, UDWORD player)
{
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
	if (!validLocation((BASE_STATS *)&sStats, *pX, *pY, player, false))
	{
		/* Keep going until we get a tile or we exceed distance */
		while(passes < LOOK_FOR_EMPTY_TILE)
		{
			/* Process whole box */
			for(i = startX; i <= endX; i++)
			{
				for(j = startY; j<= endY; j++)
				{
					/* Test only perimeter as internal tested previous iteration */
					if(i==startX || i==endX || j==startY || j==endY)
					{
						/* Good enough? */
						if(validLocation((BASE_STATS *)&sStats, i, j, player, false))
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
	ASSERT(!"unable to find a valid location", "findAssemblyPointPosition: unable to find a valid location!");
}


/*sets the point new droids go to - x/y in world coords for a Factory
bCheck is set to true for initial placement of the Assembly Point*/
void setAssemblyPoint(FLAG_POSITION *psAssemblyPoint, UDWORD x, UDWORD y,
					UDWORD player, BOOL bCheck)
{
	ASSERT( psAssemblyPoint != NULL,
		"setAssemblyPoint: invalid AssemblyPoint pointer" );

	//check its valid
	x = map_coord(x);
	y = map_coord(y);
	if (bCheck)
	{
		findAssemblyPointPosition(&x, &y, player);
	}
	//add half a tile so the centre is in the middle of the tile
	x = world_coord(x) + TILE_UNITS/2;
	y = world_coord(y) + TILE_UNITS/2;

	psAssemblyPoint->coords.x = x;
	psAssemblyPoint->coords.y = y;

	// Deliv Point sits at the height of the tile it's centre is on + arbitary amount!
	psAssemblyPoint->coords.z = map_Height((UWORD)x, (UWORD)y) + 10;
}


/*sets the factory Inc for the Assembly Point*/
void setFlagPositionInc(FUNCTIONALITY* pFunctionality, UDWORD player, UBYTE factoryType)
{
	UBYTE			inc;
	UBYTE			mask = 1;
	FACTORY			*psFactory;
	REPAIR_FACILITY *psRepair;
#ifdef DEBUG
	char			*pType;			//if you are going to do this, then make SURE you also do the same to anything
#endif									//that uses the variable.

	ASSERT( player < MAX_PLAYERS, "setFlagPositionInc: invalid player number" );
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
		ASSERT(!"building more factories than allowed", "Building more than %d %s for player %d", MAX_FACTORY, pType, player);
#endif
		inc = 1;
	}

	if (factoryType == REPAIR_FLAG)
	{
		psRepair = &pFunctionality->repairFacility;
		psRepair->psDeliveryPoint->factoryInc = 0;
		psRepair->psDeliveryPoint->factoryType = factoryType;
	}
	else
	{
		psFactory = &pFunctionality->factory;
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

	for (psCurrFlag = apsFlagPosLists[player]; psCurrFlag; psCurrFlag = psCurrFlag->psNext)
	{
		// must be selected and have a valid pos.
		if (psCurrFlag->selected)
		{
			setAssemblyPoint(psCurrFlag, x, y, player, true);

			//deselect once moved
			psCurrFlag->selected = false;
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
	if (psStructType->type == REF_FACTORY ||
		psStructType->type == REF_CYBORG_FACTORY ||
		psStructType->type == REF_VTOL_FACTORY)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_FACTORY_BUILT);
	}
}


STRUCTURE_STATS * structGetDemolishStat( void )
{
	if ( g_psStatDestroyStruct == NULL )
	{
		debug( LOG_ERROR, "structGetDemolishStat: stat not initialised1\n" );
		abort();
	}

	return g_psStatDestroyStruct;
}


/*sets the flag to indicate a HQ Exists - so draw Radar*/
void setHQExists(BOOL state, UDWORD player)
{
	hqExists[player] = (UBYTE)state;
}


/*returns the status of the flag*/
BOOL getHQExists(UDWORD player)
{
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
BOOL calcStructureMuzzleLocation(STRUCTURE *psStructure, Vector3i *muzzle, int weapon_slot)
{
	Vector3i barrel;
	iIMDShape		*psShape = psStructure->pStructureType->pIMD, *psWeaponImd = NULL;

	CHECK_STRUCTURE(psStructure);

	if (psStructure->asWeaps[weapon_slot].nStat > 0)
	{
		psWeaponImd =  asWeaponStats[psStructure->asWeaps[weapon_slot].nStat].pIMD;
	}
	else
	{
		psWeaponImd =  NULL;
	}

	if(psShape && psShape->nconnectors)
	{
		pie_MatBegin();

		pie_TRANSLATE(psStructure->pos.x,-(SDWORD)psStructure->pos.z,psStructure->pos.y);
		//matrix = the center of droid
		pie_MatRotY( DEG( (SDWORD)psStructure->direction ) );
		pie_MatRotX( DEG( psStructure->pitch ) );
		pie_MatRotZ( DEG( -(SDWORD)psStructure->roll ) );
		pie_TRANSLATE( psShape->connectors[weapon_slot].x, -psShape->connectors[weapon_slot].z,
					-psShape->connectors[weapon_slot].y);//note y and z flipped

		//matrix = the gun and turret mount on the body
		pie_MatRotY(DEG((SDWORD)psStructure->turretRotation[weapon_slot]));//+ve anticlockwise
		pie_MatRotX(DEG(psStructure->turretPitch[weapon_slot]));//+ve up
		pie_MatRotZ(DEG(0));
		//matrix = the muzzle mount on turret
		if( psWeaponImd && psWeaponImd->nconnectors )
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

		pie_RotateTranslate3iv(&barrel, muzzle);
		muzzle->z = -muzzle->z;

		pie_MatEnd();
	}
	else
	{
		muzzle->x = psStructure->pos.x;
		muzzle->y = psStructure->pos.y;
		muzzle->z = psStructure->pos.z + psStructure->sDisplay.imd->max.y;
	}

	return true;
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
		ASSERT(!"invalid structure type", "checkForResExtractors: invalid structure type");
		return;
	}
	psPowerGen = &psBuilding->pFunctionality->powerGenerator;
	//count the number of allocated slots
	slot = 0;//-1;
	for (i=0; i < NUM_POWER_MODULES; i++)
	{
		if (psPowerGen->apResExtractors[i] != NULL)
		{
			slot++;
			//make sure the derrrick is active if any oil left
			psResExtractor = &psPowerGen->apResExtractors[i]->pFunctionality->resourceExtractor;
			if (psResExtractor->power)
			{
				psResExtractor->active = true;
			}
		}
	}

	psResExtractor = NULL;
	//each Power Gen can cope with 4 Extractors now - 9/6/98 AB
	//check capacity against number of filled slots
	if (slot < NUM_POWER_MODULES)
	{
		for (psCurr = apsStructLists[psBuilding->player]; psCurr != NULL;
			psCurr = psCurr->psNext)
		{
			if (psCurr->pStructureType->type == REF_RESOURCE_EXTRACTOR)
			{
				psResExtractor = &psCurr->pFunctionality->resourceExtractor;

				//check not connected and power left and built!
				if (!psResExtractor->active
				 && psCurr->status == SS_BUILT
				 && psResExtractor->power)
				{
					//assign the extractor to the power generator - use first vacant slot
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
					psResExtractor->active = true;
					psResExtractor->timeLastUpdated = gameTime;
					slot++;
					//each Power Gen can cope with 4 Extractors now - 9/6/98 AB
					//check to see if any more vacant slots
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
		ASSERT(!"invalid structure type", "checkForPowerGen: invalid structure type");
		return;
	}
	psRE = &psBuilding->pFunctionality->resourceExtractor;
	if (psRE->active)
	{
		return;
	}

	//loop thru the current structures
	for (psCurr = apsStructLists[psBuilding->player]; psCurr != NULL;
		psCurr = psCurr->psNext)
	{
		if (psCurr->pStructureType->type == REF_POWER_GEN && psCurr->status == SS_BUILT)
		{
			psPG = &psCurr->pFunctionality->powerGenerator;
			//check capacity against number of filled slots
			slot = 0;//-1;
			for (i=0; i < NUM_POWER_MODULES; i++)
			{
				if (psPG->apResExtractors[i] != NULL)
				{
					slot++;
				}
			}
			//each Power Gen can cope with 4 Extractors now - 9/6/98 AB
			//each Power Gen can cope with 4 extractors
			if (slot < NUM_POWER_MODULES )
			{
				//find the first vacant slot
				for (i=0; i < NUM_POWER_MODULES; i++)
				{
					if (psPG->apResExtractors[i] == NULL)
					{
						psPG->apResExtractors[i] = psBuilding;
						psRE->psPowerGen = psCurr;
						psRE->active = true;
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
		ASSERT(!"invalid structure type", "informPowerGen: invalid structure type");
		return;
	}

	//get the owning power generator
	psPowerGen = &psStruct->pFunctionality->resourceExtractor.psPowerGen->pFunctionality->powerGenerator;
	if (psPowerGen)
	{
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
		ASSERT(!"invalid structure type", "releaseResExtractor:Invalid structure type");
		return;
	}

	//tell associated Power Gen
	if (psRelease->pFunctionality->resourceExtractor.psPowerGen)
	{
		informPowerGen(psRelease);
	}

	psRelease->pFunctionality->resourceExtractor.psPowerGen = NULL;

	//there may be spare resource extractors
	for (psCurr = apsStructLists[psRelease->player]; psCurr != NULL; psCurr =
		psCurr->psNext)
	{
		//check not connected and power left and built!
		if (psCurr->pStructureType->type == REF_RESOURCE_EXTRACTOR
		 && psCurr != psRelease
		 && !psCurr->pFunctionality->resourceExtractor.active
		 && psCurr->pFunctionality->resourceExtractor.power
		 && psCurr->status == SS_BUILT)
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
		ASSERT(!"invalid structure type", "releasePowerGen: Invalid structure type");
		return;
	}

	psPowerGen = &psRelease->pFunctionality->powerGenerator;
	//go through list of res extractors, setting them to inactive
	for (i=0; i < NUM_POWER_MODULES; i++)
	{
		if (psPowerGen->apResExtractors[i])
		{
			psPowerGen->apResExtractors[i]->pFunctionality->resourceExtractor.active = false;
			psPowerGen->apResExtractors[i]->pFunctionality->resourceExtractor.psPowerGen = NULL;
			psPowerGen->apResExtractors[i] = NULL;
		}
	}
	//may have a power gen with spare capacity
	for (psCurr = apsStructLists[psRelease->player]; psCurr != NULL; psCurr =
		psCurr->psNext)
	{
		if (psCurr->pStructureType->type == REF_POWER_GEN &&
			psCurr != psRelease && psCurr->status == SS_BUILT)
		{
			checkForResExtractors(psCurr);
		}
	}
}


/*this is called whenever a structure has finished building*/
void buildingComplete(STRUCTURE *psBuilding)
{
	CHECK_STRUCTURE(psBuilding);

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


/*for a given structure, return a pointer to its module stat */
STRUCTURE_STATS* getModuleStat(STRUCTURE *psStruct)
{
	STRUCTURE_STATS		*psStat;

	ASSERT( psStruct != NULL,
		"getModuleStat: Invalid structure pointer" );

	psStat = NULL;
	switch (psStruct->pStructureType->type)
	{
	case REF_POWER_GEN:
		psStat = &asStructureStats[powerModuleStat];
		break;
	case REF_FACTORY:
	case REF_VTOL_FACTORY:
		psStat = &asStructureStats[factoryModuleStat];
		break;
	case REF_RESEARCH:
		psStat = &asStructureStats[researchModuleStat];
		break;
	default:
		//no other structures can have modules attached
		break;
	}

	return psStat;
}

/* count the artillery droids assigned to a structure (only makes sence by sensor towers and headquarters) */
unsigned int countAssignedDroids(STRUCTURE *psStructure)
{
	DROID *psCurr;
	SDWORD weapontype, hasindirect;
	unsigned int num;

	if(psStructure == NULL)
		return 0;

	for (num = 0, psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr->psTarget && psCurr->player == psStructure->player)
		{
			hasindirect = 0;
			weapontype = asWeaponStats[psCurr->asWeaps[0].nStat].movementModel;

			if(weapontype == MM_INDIRECT || weapontype == MM_HOMINGINDIRECT)
			{
				hasindirect = 1;
			}

			if (psCurr->psTarget->id == psStructure->id && hasindirect)
			{
				num++;
			}
		}
	}

	return num;
}

//print some info at the top of the screen dependant on the structure
void printStructureInfo(STRUCTURE *psStructure)
{
	unsigned int numConnected, i;
	POWER_GEN	*psPowerGen;

	ASSERT( psStructure != NULL, "printStructureInfo: Invalid Structure pointer" );

	switch (psStructure->pStructureType->type)
	{
	case REF_HQ:
#ifdef DEBUG
		if (getDebugMappingStatus())
		{
			CONPRINTF(ConsoleString, (ConsoleString, "%s - %d Units assigned - ID %d - sensor range %hu power %hu - ECM %u",
			          getStatName(psStructure->pStructureType), countAssignedDroids(psStructure),
			          psStructure->id, structSensorRange(psStructure), structSensorPower(psStructure), structConcealment(psStructure)));
		}
		else
#endif
		{
			unsigned int assigned_droids = countAssignedDroids(psStructure);

			CONPRINTF(ConsoleString, (ConsoleString, ngettext("%s - %u Unit assigned", "%s - %u Units assigned", assigned_droids),
			          getStatName(psStructure->pStructureType), assigned_droids));
		}
		break;
	case REF_DEFENSE:
		if (psStructure->pStructureType->pSensor == NULL)
		{
			break;
		}
#ifdef DEBUG
		else if (getDebugMappingStatus())
		{
			CONPRINTF(ConsoleString, (ConsoleString, "%s - %d Units assigned - ID %d - armour %d|%d - sensor range %hu power %hu - ECM %u",
				getStatName(psStructure->pStructureType), countAssignedDroids(psStructure),
				psStructure->id, psStructure->armour[0][WC_KINETIC], psStructure->armour[0][WC_HEAT],
			        structSensorRange(psStructure), structSensorPower(psStructure), structConcealment(psStructure)));
		}
#endif
		else
		{
			unsigned int assigned_droids = countAssignedDroids(psStructure);

			CONPRINTF(ConsoleString, (ConsoleString, ngettext("%s - %u Unit assigned", "%s - %u Units assigned", assigned_droids),
				getStatName(psStructure->pStructureType), assigned_droids));
		}
		break;
	case REF_RESOURCE_EXTRACTOR:
#ifdef DEBUG
		if (getDebugMappingStatus())
		{
			CONPRINTF(ConsoleString,(ConsoleString, "%s - Unique ID %d",
			          getStatName(psStructure->pStructureType), psStructure->id));
		}
		else
#endif
		{
			CONPRINTF(ConsoleString, (ConsoleString, getStatName(psStructure->pStructureType)));
		}
		break;
	case REF_POWER_GEN:
		psPowerGen = &psStructure->pFunctionality->powerGenerator;
		numConnected = 0;
		for (i = 0; i < NUM_POWER_MODULES; i++)
		{
			if (psPowerGen->apResExtractors[i])
			{
				numConnected++;
			}
		}
#ifdef DEBUG
		if (getDebugMappingStatus())
		{
			CONPRINTF(ConsoleString, (ConsoleString, "%s -  Connected %u of %u - Unique ID %u",
			          getStatName(psStructure->pStructureType), numConnected, NUM_POWER_MODULES,
			          psStructure->id));
		}
		else
#endif
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("%s - Connected %u of %u"),
			          getStatName(psStructure->pStructureType), numConnected, NUM_POWER_MODULES));
		}
		break;
	default:
#ifdef DEBUG
		if (getDebugMappingStatus())
		{
			CONPRINTF(ConsoleString, (ConsoleString, "%s - Damage % 3.2f%% - Unique ID %u",
			          getStatName(psStructure->pStructureType), getStructureDamage(psStructure) * 100.f, psStructure->id));
		}
		else
#endif
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("%s - Damage %3.0f%%"),
			          getStatName(psStructure->pStructureType), getStructureDamage(psStructure) * 100.f));
		}
		break;
	}
}


/*Checks the template type against the factory type - returns false
if not a good combination!*/
BOOL validTemplateForFactory(DROID_TEMPLATE *psTemplate, STRUCTURE *psFactory)
{
	//not in multiPlayer! - AB 26/5/99
	if (!bMultiPlayer)
	{
		//ignore Transporter Droids
		if (psTemplate->droidType == DROID_TRANSPORTER)
		{
			return false;
		}
	}

	//check if droid is a cyborg
	if (psTemplate->droidType == DROID_CYBORG ||
		psTemplate->droidType == DROID_CYBORG_SUPER ||
		psTemplate->droidType == DROID_CYBORG_CONSTRUCT ||
		psTemplate->droidType == DROID_CYBORG_REPAIR)
	{
		if (psFactory->pStructureType->type != REF_CYBORG_FACTORY)
		{
			return false;
		}
	}
	//check for VTOL droid
	else if ((asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->
		propulsionType == LIFT)
	{
		if (psFactory->pStructureType->type != REF_VTOL_FACTORY)
		{
			return false;
		}
	}

	//check if cyborg factory
	if (psFactory->pStructureType->type == REF_CYBORG_FACTORY)
	{
		//if (psTemplate->droidType != DROID_CYBORG)
		if (!(psTemplate->droidType == DROID_CYBORG ||
			psTemplate->droidType == DROID_CYBORG_SUPER ||
			psTemplate->droidType == DROID_CYBORG_CONSTRUCT ||
			psTemplate->droidType == DROID_CYBORG_REPAIR))
		{
			return false;
		}
	}
	//check if vtol factory
	else if (psFactory->pStructureType->type == REF_VTOL_FACTORY)
	{
		if ((asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->
			propulsionType != LIFT)
		{
			return false;
		}
	}

	//got through all the tests...
	return true;
}

/*calculates the damage caused to the resistance levels of structures - returns
true when captured*/
BOOL electronicDamage(BASE_OBJECT *psTarget, UDWORD damage, UBYTE attackPlayer)
{
	STRUCTURE   *psStructure;
	DROID       *psDroid;
	BOOL        bCompleted = true;

	Vector3i pos;
	UDWORD		i;

	ASSERT( attackPlayer < MAX_PLAYERS,
		"electronicDamage: invalid player id" );

	//structure electronic damage
	if (psTarget->type == OBJ_STRUCTURE)
	{
		psStructure = (STRUCTURE *)psTarget;
		bCompleted = false;

		ASSERT( psStructure != NULL,
			"electronicDamage: Invalid Structure pointer" );

		ASSERT( psStructure->pStructureType->resistance != 0,
			"electronicDamage: invalid structure for EW" );

		//if resistance is already less than 0 don't do any more
		if (psStructure->resistance < 0)
		{
			bCompleted = true;
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
					CONPRINTF(ConsoleString,(ConsoleString,
						_("%s - Electronically Damaged"),
						getStatName(psStructure->pStructureType)));
					//tell the scripts if selectedPlayer has lost a structure
					eventFireCallbackTrigger((TRIGGER_TYPE)CALL_ELECTRONIC_TAKEOVER);
				}
				bCompleted = true;
				//give the structure to the attacking player
				(void)giftSingleStructure(psStructure, attackPlayer, false);
			}
		}
	}
	//droid electronic damage
	else if (psTarget->type == OBJ_DROID)
	{
		psDroid = (DROID *)psTarget;
		bCompleted = false;

		ASSERT(psDroid != NULL, "electronicDamage: Invalid Droid pointer");

		//in multiPlayer cannot attack a Transporter with EW
		if (bMultiPlayer)
		{
			if (psDroid->droidType == DROID_TRANSPORTER)
			{
				ASSERT(!"can't attack a Transporter while in multiplayer", "electronicDamage: Cannot attack a Transporter in multiPlayer");
				return true;
			}
		}

		if (psDroid->resistance == ACTION_START_TIME)
		{
			//need to set the current resistance level since not been previously attacked (by EW)
			psDroid->resistance = droidResistance(psDroid);
		}

		if (psDroid->resistance < 0)
		{
			bCompleted = true;
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
					CONPRINTF(ConsoleString, (ConsoleString, _("%s - Electronically Damaged"), "Unit"));
					//tell the scripts if selectedPlayer has lost a droid
					eventFireCallbackTrigger((TRIGGER_TYPE)CALL_ELECTRONIC_TAKEOVER);
				}
				bCompleted = true;

				//give the droid to the attacking player

				if(psDroid->visible[selectedPlayer])
				{
					for(i=0; i<5; i++)
					{
						pos.x = psDroid->pos.x + (30-rand()%60);
						pos.z = psDroid->pos.y + (30-rand()%60);
						pos.y = psDroid->pos.z + (rand()%8);
						effectGiveAuxVar(80);
						addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_FLAMETHROWER,false,NULL,0);
					}
				}

				(void)giftSingleDroid(psDroid, attackPlayer);

				// tell the world!
				if (bMultiPlayer)
				{
					uint8_t giftType = DROID_GIFT, droid_count = 1;

					NETbeginEncode(NET_GIFT, NET_ALL_PLAYERS);
					{
						// We need to distinguish between gift types
						NETuint8_t(&giftType);

						// send the player originally owning this droid
						// and the new owner
						NETuint8_t(&psDroid->player);
						NETuint8_t(&attackPlayer);

						// the amount of droids (1 in this case)
						// followed by the droid's ID
						NETuint8_t(&droid_count);
						NETuint32_t(&psDroid->id);
					}
					NETend();
				}

				//check to see if droid limit reached, if so recycle.
				// don't check for transporter/mission coz multiplayer only issue.
				if( (getNumDroids(attackPlayer)+1 ) > getMaxDroids(attackPlayer) )
				{
					recycleDroid(psDroid);
				}
			}
		}
	}

	return bCompleted;
}


/* EW works differently in multiplayer mode compared with single player.*/
BOOL validStructResistance(STRUCTURE *psStruct)
{
	BOOL    bTarget = false;

	ASSERT( psStruct != NULL, "invalidStructResistance: invalid structure pointer" );

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
					bTarget = true;
				}
				break;
			default:
				bTarget = true;
			}
		}
		else
		{
			bTarget = true;
		}
	}

	return bTarget;
}


/*Access functions for the upgradeable stats of a structure*/
UDWORD structureBody(const STRUCTURE *psStructure)
{
	const STRUCTURE_STATS *psStats = psStructure->pStructureType;
	const UBYTE player = psStructure->player;

	switch(psStats->type)
	{
		// wall/defence structures:
		case REF_DEFENSE:
		case REF_WALL:
		case REF_WALLCORNER:
		case REF_BLASTDOOR:
			return psStats->bodyPoints + (psStats->bodyPoints * asWallDefenceUpgrade[player].body)/100;
		// all other structures:
		default:
			return structureBaseBody(psStructure) + (structureBaseBody(psStructure) * asStructureUpgrade[player].body)/100;
	}
}


/*this returns the Base Body points of a structure - regardless of upgrade*/
UDWORD structureBaseBody(const STRUCTURE *psStructure)
{
	const STRUCTURE_STATS *psStats = psStructure->pStructureType;

	ASSERT( psStructure != NULL, "structureBaseBody: invalid structure pointer" );

	switch(psStats->type)
	{
		// modules may be attached:
		case REF_FACTORY:
		case REF_VTOL_FACTORY:
			ASSERT(psStructure->pFunctionality != NULL, "structureBaseBody: invalid structure functionality pointer");
			if (psStructure->pFunctionality->factory.capacity > 0)
			{
				//add on the default for the factory
				return psStats->bodyPoints + asStructureStats[factoryModuleStat].bodyPoints * psStructure->pFunctionality->factory.capacity;
			}
			else
			{
				//no modules
				return psStats->bodyPoints;
			}
		case REF_RESEARCH:
			ASSERT(psStructure->pFunctionality != NULL, "structureBaseBody: invalid structure functionality pointer");
			if (psStructure->pFunctionality->researchFacility.capacity > 0)
			{
				//add on the default for the factory
				return psStats->bodyPoints + asStructureStats[researchModuleStat].bodyPoints;
			}
			else
			{
				//no modules
				return psStats->bodyPoints;
			}
		case REF_POWER_GEN:
			ASSERT(psStructure->pFunctionality != NULL, "structureBaseBody: invalid structure functionality pointer");
			if (psStructure->pFunctionality->powerGenerator.capacity > 0)
			{
				//add on the default for the factory
				return psStats->bodyPoints + asStructureStats[powerModuleStat].bodyPoints;
			}
			else
			{
				//no modules
				return psStats->bodyPoints;
			}
		// all other structures:
		default:
			return psStats->bodyPoints;
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


/*gives the attacking player a reward based on the type of structure that has
been attacked*/
BOOL electronicReward(STRUCTURE *psStructure, UBYTE attackPlayer)
{
	BOOL    bRewarded = false;

	switch(psStructure->pStructureType->type)
	{
	case REF_RESEARCH:
		researchReward(psStructure->player, attackPlayer);
		bRewarded = true;
		break;
	case REF_FACTORY:
	case REF_VTOL_FACTORY:
	case REF_CYBORG_FACTORY:
		factoryReward(psStructure->player, attackPlayer);
		bRewarded = true;
		break;
	case REF_HQ:
		hqReward(psStructure->player,attackPlayer);
		if (attackPlayer == selectedPlayer)
		{
			addConsoleMessage(_("Electronic Reward - Visibility Report"),	DEFAULT_JUSTIFY,SYSTEM_MESSAGE);
		}
		bRewarded = true;
		break;
	case REF_REPAIR_FACILITY:
		repairFacilityReward(psStructure->player,attackPlayer);
		bRewarded = true;
		break;
	default:
		bRewarded = false;
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
		if (apCompLists[losingPlayer][COMP_PROPULSION][inc] == AVAILABLE &&
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
			CONPRINTF(ConsoleString,(ConsoleString,"%s :- %s",
				_("Factory Reward - Propulsion"),
				getName(asPropulsionStats[comp].pName)));
		}
		return;
	}

	//haven't found a propulsion - look for a body
	for (inc=0; inc < numBodyStats; inc++)
	{
		if (apCompLists[losingPlayer][COMP_BODY][inc] == AVAILABLE &&
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
			CONPRINTF(ConsoleString,(ConsoleString,"%s :- %s",
				_("Factory Reward - Body"),
				getName(asBodyStats[comp].pName)));
		}
		return;
	}

	//haven't found a body - look for a weapon
	for (inc=0; inc < numWeaponStats; inc++)
	{
		if (apCompLists[losingPlayer][COMP_WEAPON][inc] == AVAILABLE &&
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
			CONPRINTF(ConsoleString,(ConsoleString,"%s :- %s",
				_("Factory Reward - Weapon"),
				getName(asWeaponStats[comp].pName)));
		}
		return;
	}

	//losing Player hasn't got anything better so don't gain anything!
	if (rewardPlayer == selectedPlayer)
	{
		addConsoleMessage(_("Factory Reward - Nothing"), DEFAULT_JUSTIFY,SYSTEM_MESSAGE);
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
		if (apCompLists[losingPlayer][COMP_REPAIRUNIT][inc] == AVAILABLE &&
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
			CONPRINTF(ConsoleString,(ConsoleString,"%s :- %s",
				_("Repair Facility Award - Repair"),
				getName(asRepairStats[comp].pName)));
		}
		return;
	}
	if (rewardPlayer == selectedPlayer)
	{
		addConsoleMessage(_("Repair Facility Award - Nothing"), DEFAULT_JUSTIFY,SYSTEM_MESSAGE);
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
			}
		}

		//feature
		for(psFeat = apsFeatureLists[i]; psFeat != NULL; psFeat = psFeat->psNext)
		{
			if(psFeat->visible[losingPlayer] )
			{
				psFeat->visible[rewardPlayer] = psFeat->visible[losingPlayer];
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


// Return true if structure is a factory of any type.
//
BOOL StructIsFactory(STRUCTURE *Struct)
{
	if( (Struct->pStructureType->type == REF_FACTORY) ||
		(Struct->pStructureType->type == REF_CYBORG_FACTORY) ||
		(Struct->pStructureType->type == REF_VTOL_FACTORY) )
	{
		return true;
	}

	return false;
}


// Return true if flag is a delivery point for a factory.
//
BOOL FlagIsFactory(FLAG_POSITION *psCurrFlag)
{
	if( (psCurrFlag->factoryType == FACTORY_FLAG) || (psCurrFlag->factoryType == CYBORG_FLAG) ||
		(psCurrFlag->factoryType == VTOL_FLAG) )
	{
		return true;
	}

	return false;
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
			if(FlagIsFactory(psCurrFlag)
			 && Struct->pFunctionality->factory.psAssemblyPoint->factoryInc == psCurrFlag->factoryInc
			 && Struct->pFunctionality->factory.psAssemblyPoint->factoryType == psCurrFlag->factoryType)
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
			psFactory = &psCurr->pFunctionality->factory;
			if (psFactory->psAssemblyPoint->factoryInc == psDelPoint->factoryInc &&
				psFactory->psAssemblyPoint->factoryType == psDelPoint->factoryType)
			{
				return psCurr;
			}
		}
		else if (psCurr->pStructureType->type == REF_REPAIR_FACILITY)
		{
			psRepair = &psCurr->pFunctionality->repairFacility;
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

	ASSERT( StructIsFactory(psBuilding),
		"cancelProduction: structure not a factory" );

	psFactory = &psBuilding->pFunctionality->factory;

	//check its the correct factory
	if (psBuilding->player == productionPlayer && psFactory->psSubject)
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

	ASSERT( StructIsFactory(psBuilding), "holdProduction: structure not a factory" );

	psFactory = &psBuilding->pFunctionality->factory;

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

	ASSERT( StructIsFactory(psBuilding),
		"releaseProduction: structure not a factory" );

	psFactory = &psBuilding->pFunctionality->factory;

	if (psFactory->psSubject && psFactory->timeStartHold)
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

	CHECK_STRUCTURE(psStructure);
	ASSERT( psStructure->player == productionPlayer,
		"factoryProdUpdate: called for incorrect player" );

	psFactory = &psStructure->pFunctionality->factory;
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
	BOOL		bAssigned = false, bCheckForCancel = false;
	UBYTE	built, quantity, remaining;

	CHECK_STRUCTURE(psStructure);
	ASSERT( psStructure->player == productionPlayer,
		"factoryProdAdjust: called for incorrect player" );

	psFactory = &psStructure->pFunctionality->factory;
	factoryType = psFactory->psAssemblyPoint->factoryType;
	factoryInc = psFactory->psAssemblyPoint->factoryInc;

	//see if the template is already in the list
	for (inc=0; inc < MAX_PROD_RUN; inc++)
	{
		if (asProductionRun[factoryType][factoryInc][inc].psTemplate == psTemplate)
		{
			//adjust the prod run
			if (add)	//user left clicked, so increase # in queue
			{
				// Allows us to queue up more units up to MAX_IN_RUN instead of ignoring how many we have built from that queue
				quantity = ++asProductionRun[factoryType][factoryInc][inc].quantity;
				built = asProductionRun[factoryType][factoryInc][inc].built;
				remaining = quantity - built;
				// check to see if user canceled all orders in queue
				if (remaining > MAX_IN_RUN)
				{
					asProductionRun[factoryType][factoryInc][inc].quantity = 0;
					//initialise the template
					asProductionRun[factoryType][factoryInc][inc].psTemplate = NULL;
					bCheckForCancel = true;
					//add power back if we were working on this one
					if (psFactory->psSubject == (BASE_STATS *)psTemplate)
					{
						addPower(psStructure->player, psFactory->powerAccrued);
						//set the factory's power accrued back to zero
						psFactory->powerAccrued = 0;
					}
				}
			}
			else	//user right clicked, so we subtract form queue
			{
				if (asProductionRun[factoryType][factoryInc][inc].quantity == 0)
				{
					asProductionRun[factoryType][factoryInc][inc].quantity = MAX_IN_RUN;
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
						//initialise the template
						asProductionRun[factoryType][factoryInc][inc].psTemplate = NULL;
						bCheckForCancel = true;
					}
				}
			}
			bAssigned = true;
			break;
		}
		//check to see if any empty slots
		if (spare == -1 && asProductionRun[factoryType][factoryInc][inc].quantity == 0)
		{
			spare = inc;
		}
	}

	if (!bAssigned && spare != -1)
	{
		//start off a new template
		asProductionRun[factoryType][factoryInc][spare].psTemplate = psTemplate;
		if (add)
		{
			asProductionRun[factoryType][factoryInc][spare].quantity = 1;
		}
		else
		{
			//wrap around to max value
			asProductionRun[factoryType][factoryInc][spare].quantity = MAX_IN_RUN;
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
		}
	}
}


//returns the quantity of a specific template in the production list
UDWORD	getProductionQuantity(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate)
{
	UDWORD		inc, factoryType, factoryInc;
	FACTORY		*psFactory;

	if (psStructure == NULL) return 0;
	if (psStructure->player == productionPlayer)
	{
		psFactory = &psStructure->pFunctionality->factory;
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

	if (psStructure == NULL) return 0;
	if (psStructure->player == productionPlayer)
	{
		psFactory = &psStructure->pFunctionality->factory;
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

	ASSERT( player == selectedPlayer,
		"countAssignableFactories: should only be called for selectedPlayer" );

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
	ASSERT( player < MAX_PLAYERS,
		"checkFactoryExists: invalid player" );
	ASSERT( factoryType < NUM_FACTORY_TYPES,
		"checkFactoryExists: invalid factoryType" );

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
					psFactory = &psStruct->pFunctionality->factory;
					if (psFactory->psAssemblyPoint == NULL)//need to add one
					{
						ASSERT( psFactory->psAssemblyPoint != NULL,
							"checkDeliveryPoints: no delivery point for factory" );
					}
					else
					{
						setAssemblyPoint(psFactory->psAssemblyPoint, psFactory->psAssemblyPoint->
							coords.x, psFactory->psAssemblyPoint->coords.y, inc, true);
					}
				}
				else if (psStruct->pStructureType->type == REF_REPAIR_FACILITY)
				{
					psRepair = &psStruct->pFunctionality->repairFacility;

					if (psRepair->psDeliveryPoint == NULL)//need to add one
					{
						if (version >= VERSION_19)
						{
							ASSERT( psRepair->psDeliveryPoint != NULL,"checkDeliveryPoints: no delivery point for repair facility" );
						}
						else
						{
							// add an assembly point
							if (!createFlagPosition(&psRepair->psDeliveryPoint, psStruct->player))
							{
								ASSERT(!"can't create new deilivery point for repair facility", "checkDeliveryPoints: unable to create new delivery point for repair facility");
								return;
							}
							addFlagPosition(psRepair->psDeliveryPoint);
							setFlagPositionInc(psStruct->pFunctionality, psStruct->player, REPAIR_FLAG);
							//initialise the assembly point position
							x = map_coord(psStruct->pos.x + 256);
							y = map_coord(psStruct->pos.y + 256);
							// Belt and braces - shouldn't be able to build too near edge
							setAssemblyPoint( psRepair->psDeliveryPoint, world_coord(x),
								world_coord(y), inc, true);
						}
					}
					else//check existing one
					{
						setAssemblyPoint(psRepair->psDeliveryPoint, psRepair->psDeliveryPoint->
							coords.x, psRepair->psDeliveryPoint->coords.y, inc, true);
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

	ASSERT( StructIsFactory(psStruct),
		"factoryLoopAdjust: structure is not a factory" );
	ASSERT( psStruct->player == selectedPlayer,
		"factoryLoopAdjust: should only be called for selectedPlayer" );

	psFactory = &psStruct->pFunctionality->factory;

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
				if (psStruct->pFunctionality->resourceExtractor.active)
				{
					psStruct->pFunctionality->resourceExtractor.timeLastUpdated = gameTime;
				}
			}
		}
	}
}


/*Used for determining how much of the structure to draw as being built or demolished*/
float structHeightScale(STRUCTURE *psStruct)
{
	float retVal = (float)psStruct->currentBuildPts / (float)psStruct->pStructureType->buildPoints;

	if(retVal < 0.05f)
		retVal = 0.05f;

	return retVal;

}


/*compares the structure sensor type with the droid weapon type to see if the
FIRE_SUPPORT order can be assigned*/
BOOL structSensorDroidWeapon(STRUCTURE *psStruct, DROID *psDroid)
{
	//Watermelon:another crash when nStat is marked as 0xcd... FIXME: Why is nStat not initialized properly?
	//Added a safety check: Only units with weapons can be assigned.
	if (psDroid->numWeaps > 0)
	{
		//Standard Sensor Tower + indirect weapon droid (non VTOL)
		//else if (structStandardSensor(psStruct) && (psDroid->numWeaps &&
		if (structStandardSensor(psStruct) && (psDroid->asWeaps[0].nStat > 0 &&
			!proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat)) &&
			!vtolDroid(psDroid))
		{
			return true;
		}
		//CB Sensor Tower + indirect weapon droid (non VTOL)
		//if (structCBSensor(psStruct) && (psDroid->numWeaps &&
		else if (structCBSensor(psStruct) && (psDroid->asWeaps[0].nStat > 0 &&
			!proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat)) &&
			!vtolDroid(psDroid))
		{
			return true;
		}
		//VTOL Intercept Sensor Tower + any weapon VTOL droid
		//else if (structVTOLSensor(psStruct) && psDroid->numWeaps &&
		else if (structVTOLSensor(psStruct) && psDroid->asWeaps[0].nStat > 0 &&
			vtolDroid(psDroid))
		{
			return true;
		}
		//VTOL CB Sensor Tower + any weapon VTOL droid
		//else if (structVTOLCBSensor(psStruct) && psDroid->numWeaps &&
		else if (structVTOLCBSensor(psStruct) && psDroid->asWeaps[0].nStat > 0 &&
			vtolDroid(psDroid))
		{
			return true;
		}
	}

	//case not matched
	return false;
}


/*checks if the structure has a Counter Battery sensor attached - returns
true if it has*/
BOOL structCBSensor(STRUCTURE *psStruct)
{
	if (psStruct->pStructureType->pSensor)
	{
		//Super Sensor works as any type
		if ((psStruct->pStructureType->pSensor->type == INDIRECT_CB_SENSOR ||
			psStruct->pStructureType->pSensor->type == SUPER_SENSOR) &&
			psStruct->pStructureType->pSensor->location == LOC_TURRET)
		{
			return true;
		}
	}
	return false;
}


/*checks if the structure has a Standard Turret sensor attached - returns
true if it has*/
BOOL structStandardSensor(STRUCTURE *psStruct)
{
	if (psStruct->pStructureType->pSensor)
	{
		/*Super Sensor works as any type*/
		if ((psStruct->pStructureType->pSensor->type == STANDARD_SENSOR ||
			psStruct->pStructureType->pSensor->type == SUPER_SENSOR) &&
			psStruct->pStructureType->pSensor->location == LOC_TURRET)
		{
			return true;
		}
	}
	return false;
}


/*checks if the structure has a VTOL Intercept sensor attached - returns
true if it has*/
BOOL structVTOLSensor(STRUCTURE *psStruct)
{
	if (psStruct->pStructureType->pSensor)
	{
		//Super Sensor works as any type
		if ((psStruct->pStructureType->pSensor->type == VTOL_INTERCEPT_SENSOR ||
			psStruct->pStructureType->pSensor->type == SUPER_SENSOR) &&
			psStruct->pStructureType->pSensor->location == LOC_TURRET)
		{
			return true;
		}
	}
	return false;
}


/*checks if the structure has a VTOL Counter Battery sensor attached - returns
true if it has*/
BOOL structVTOLCBSensor(STRUCTURE *psStruct)
{
	if (psStruct->pStructureType->pSensor)
	{
		//Super Sensor works as any type
		if ((psStruct->pStructureType->pSensor->type == VTOL_CB_SENSOR ||
			psStruct->pStructureType->pSensor->type == SUPER_SENSOR) &&
			psStruct->pStructureType->pSensor->location == LOC_TURRET)
		{
			return true;
		}
	}
	return false;
}


// check whether a rearm pad is clear
BOOL clearRearmPad(STRUCTURE *psStruct)
{
	if (psStruct->pStructureType->type == REF_REARM_PAD
	 && (psStruct->pFunctionality->rearmPad.psObj == NULL
	 || vtolHappy((DROID*)psStruct->pFunctionality->rearmPad.psObj)))
	{
		return true;
	}

	return false;
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
		cx = (SDWORD)psTarget->pos.x;
		cy = (SDWORD)psTarget->pos.y;
	}
	else
	{
		cx = (SDWORD)psDroid->pos.x;
		cy = (SDWORD)psDroid->pos.y;
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
			xdiff = (SDWORD)psStruct->pos.x - cx;
			ydiff = (SDWORD)psStruct->pos.y - cy;
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

	tx = map_coord(psStruct->pos.x);
	ty = map_coord(psStruct->pos.y);

	for(psCurr = apsDroidLists[psDroid->player]; psCurr; psCurr=psCurr->psNext)
	{
		if (psCurr != psDroid
		 && map_coord(psCurr->pos.x) == tx
		 && map_coord(psCurr->pos.y) == ty
		 && vtolDroid(psCurr))
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

	tx = map_coord(psStruct->pos.x);
	ty = map_coord(psStruct->pos.y);

	found = false;
	for(psCurr = apsDroidLists[psDroid->player]; psCurr; psCurr=psCurr->psNext)
	{
		if (psCurr != psDroid
		 && map_coord(psCurr->pos.x) == tx
		 && map_coord(psCurr->pos.y) == ty)
		{
			found = true;
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
		return(true);
	}
	else
	{
		return(false);
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

	CHECK_STRUCTURE(psStructure);

	//this is not the case for EW in multiPlayer mode
	if (!bMultiPlayer)
	{
		//added 'selectedPlayer == 0' to be able to test the game by changing player...
		//in this version of Warzone, the attack Player can NEVER be the selectedPlayer (unless from the script)
		if (!bFromScript && selectedPlayer == 0 && attackPlayer == selectedPlayer)
		{
			ASSERT(bFromScript || selectedPlayer != 0 || attackPlayer != selectedPlayer, "giftSingleStructure: EW attack by selectedPlayer on a structure");
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
			(void)removeStruct(psStructure, false);

			// remove structure from one list
			removeStructureFromList(psStructure, apsStructLists);

			psStructure->selected = false;

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
				if (psCurr->psTarget == (BASE_OBJECT *)psStructure)
				{
					orderDroid(psCurr, DORDER_STOP);
					break;
				}
				for (i = 0;i < psCurr->numWeaps;i++)
				{
					if (psCurr->psActionTarget[i] == (BASE_OBJECT *)psStructure)
					{
						orderDroid(psCurr, DORDER_STOP);
						break;
					}
				}
				//check through order list
				for (i = 0; i < psCurr->listSize; i++)
				{
					if (psCurr->asOrderList[i].psOrderTarget == (BASE_OBJECT *)psStructure)
					{
						removeDroidOrderTarget(psCurr, i);
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
				if (psStruct->psTarget[0] == (BASE_OBJECT *)psStructure)
				{
					setStructureTarget(psStruct, NULL, 0);
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
			}
		}

		//ASSERT( false,
		//    "giftSingleStructure: EW attack in multiplayer" );
		return NULL;
	}


	//save info about the structure
	psType = psStructure->pStructureType;
	x = psStructure->pos.x;
	y = psStructure->pos.y;
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
			capacity = (UBYTE)psStructure->pFunctionality->powerGenerator.capacity;
			break;
		case REF_RESEARCH:
			capacity = (UBYTE)psStructure->pFunctionality->researchFacility.capacity;
			break;
		case REF_FACTORY:
		case REF_VTOL_FACTORY:
			capacity = psStructure->pFunctionality->factory.capacity;
			break;
		default:
			break;
		}
	}
	//get rid of the structure
	(void)removeStruct(psStructure, true);

	//make sure power is not used to build
	bPowerOn = powerCalculated;
	powerCalculated = false;
	//build a new one for the attacking player - set last element to true so it doesn't adjust x/y
	psNewStruct = buildStructure(psType, x, y, attackPlayer, true);
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
				buildStructure(psModule, psNewStruct->pos.x, psNewStruct->pos.y,
					attackPlayer, false);
				break;
			case REF_FACTORY:
			case REF_VTOL_FACTORY:
				//build the appropriate number of modules
				while (capacity)
				{
					buildStructure(psModule, psNewStruct->pos.x, psNewStruct->pos.y,
						attackPlayer, false);
					capacity--;
				}
				break;
			default:
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
						psNewStruct->pos.x, psNewStruct->pos.y, psNewStruct->pos.z );

				}
				else
				{

					audio_QueueTrackPos( ID_SOUND_NEXUS_STRUCTURE_ABSORBED,
						psNewStruct->pos.x, psNewStruct->pos.y, psNewStruct->pos.z );

				}
				//make sure this structure is visible to selectedPlayer if the structure used to be selectedPlayers'
				psNewStruct->visible[selectedPlayer] = UBYTE_MAX;
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
	float	fraction;

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

	fraction = (float)asWeaponStats[psStruct->asWeaps[0].nStat].recoilValue / 100.f;

	recoil = (float)recoil * fraction;

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

				ASSERT( asStructureStats[structInc].asFuncList[inc] != NULL,
			"checkStructureStats: \
					Invalid function for structure %s",
					asStructureStats[structInc].pName );

			}
		}
		else
		{
			if (asStructureStats[structInc].asFuncList != NULL)
			{

				ASSERT( false, "checkStructureStats:Invalid functions attached to structure %s",
					asStructureStats[structInc].pName );

				return false;
			}
		}
	}
	return true;
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
			capacity = (UBYTE)psStruct->pFunctionality->powerGenerator.capacity;
			break;
		case REF_RESEARCH:
			capacity = (UBYTE)psStruct->pFunctionality->researchFacility.capacity;
			break;
		case REF_FACTORY:
		case REF_VTOL_FACTORY:
			capacity = psStruct->pFunctionality->factory.capacity;
			break;
		default:
			break;
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
				RESEARCH_FACILITY *psResFacility = &psBuilding->pFunctionality->researchFacility;

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
				FACTORY    *psFactory = &psBuilding->pFunctionality->factory;

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
			default: //do nothing
				break;
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
selected - returns true if valid*/
BOOL lasSatStructSelected(STRUCTURE *psStruct)
{
	if ( (psStruct->selected || (bMultiPlayer && !isHumanPlayer(psStruct->player)))
		&& psStruct->asWeaps[0].nStat
		&& (asWeaponStats[psStruct->asWeaps[0].nStat].weaponSubClass == WSC_LAS_SAT))
	{
		return true;
	}

	return false;
}


/* Call CALL_NEWDROID script callback */
static void cbNewDroid(STRUCTURE *psFactory, DROID *psDroid)
{
	ASSERT(psDroid != NULL,
		"cbNewDroid: no droid assigned for CALL_NEWDROID callback");

	psScrCBNewDroid = psDroid;
	psScrCBNewDroidFact = psFactory;
	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_NEWDROID);
	psScrCBNewDroid = NULL;
	psScrCBNewDroidFact = NULL;
}

// Check that psVictimStruct is not referred to by any other object in the game
BOOL structureCheckReferences(STRUCTURE *psVictimStruct)
{
	int plr, i;

	for (plr = 0; plr < MAX_PLAYERS; plr++)
	{
		STRUCTURE *psStruct;
		DROID *psDroid;

		for (psStruct = apsStructLists[plr]; psStruct != NULL; psStruct = psStruct->psNext)
		{
			for (i = 0; i < psStruct->numWeaps; i++)
			{
				if ((STRUCTURE *)psStruct->psTarget[i] == psVictimStruct && psVictimStruct != psStruct)
				{
#ifdef DEBUG
					ASSERT(!"Illegal reference to structure", "Illegal reference to structure from %s line %d",
					       psStruct->targetFunc[i], psStruct->targetLine[i]);
#endif
					return false;
				}
			}
		}
		for (psDroid = apsDroidLists[plr]; psDroid != NULL; psDroid = psDroid->psNext)
		{
			if ((STRUCTURE *)psDroid->psTarget == psVictimStruct)
			{
#ifdef DEBUG
				ASSERT(!"Illegal reference to structure", "Illegal reference to structure from %s line %d",
				       psDroid->targetFunc, psDroid->targetLine);
#endif
				return false;
			}
			for (i = 0; i < psDroid->numWeaps; i++)
			{
				if ((STRUCTURE *)psDroid->psActionTarget[i] == psVictimStruct)
				{
#ifdef DEBUG
					ASSERT(!"Illegal reference to structure", "Illegal action reference to structure from %s line %d",
					       psDroid->actionTargetFunc[i], psDroid->actionTargetLine[i]);
#endif
					return false;
				}
			}
			if ((STRUCTURE *)psDroid->psBaseStruct == psVictimStruct)
			{
#ifdef DEBUG
				ASSERT(!"Illegal reference to structure", "Illegal action reference to structure from %s line %d",
				       psDroid->baseFunc, psDroid->baseLine);
#endif
				return false;
			}
		}
	}
	return true;
}
