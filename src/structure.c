/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
 * By the picking of these code-bombs, something wicked this way comes. This
 * file is almost as evil as hci.c
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
#include "lib/framework/math_ext.h"
#include "edit3d.h"
#include "anim_id.h"
#include "lib/gamelib/anim.h"
#include "display3d.h"
#include "geometry.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"
#include "lib/framework/fixedpoint.h"
#include "order.h"
#include "droid.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "scriptcb.h"
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

#include "random.h"

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
// Is a bitmask, holding up to MAX_FACTORIES bits.
uint32_t                factoryNumFlag[MAX_PLAYERS][NUM_FLAG_TYPES];

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

static BOOL setFunctionality(STRUCTURE* psBuilding, STRUCTURE_TYPE functionType);
static void setFlagPositionInc(FUNCTIONALITY* pFunctionality, UDWORD player, UBYTE factoryType);
static void informPowerGen(STRUCTURE *psStruct);
static BOOL electronicReward(STRUCTURE *psStructure, UBYTE attackPlayer);
static void factoryReward(UBYTE losingPlayer, UBYTE rewardPlayer);
static void repairFacilityReward(UBYTE losingPlayer, UBYTE rewardPlayer);
static void findAssemblyPointPosition(UDWORD *pX, UDWORD *pY, UDWORD player);
static void removeStructFromMap(STRUCTURE *psStruct);
static void	structUpdateRecoil( STRUCTURE *psStruct );
static void resetResistanceLag(STRUCTURE *psBuilding);
static int structureTotalReturn(STRUCTURE *psStruct);

// last time the maximum units message was displayed
static UDWORD	lastMaxUnitMessage;

#define MAX_UNIT_MESSAGE_PAUSE 20000

/*
Check to see if the stats is some kind of expansion module

... this replaces the thousands of occurance that is spread through out the code

... There were a couple of places where it skipping around a routine if the stat was a expansion module
	(loadSaveStructureV7 & 9) this code seemed suspect, and to clarify it we replaced it with the routine below
... the module stuff seemed to work though ...     TJC (& AB) 8-DEC-98
*/

static void auxStructureNonblocking(STRUCTURE *psStructure)
{
	int i, j;
	int sWidth = getStructureWidth(psStructure);
	int sBreadth = getStructureBreadth(psStructure);
	int mapX = map_coord(psStructure->pos.x - sWidth * TILE_UNITS / 2);
	int mapY = map_coord(psStructure->pos.y - sBreadth * TILE_UNITS / 2);

	for (i = 0; i < sWidth; i++)
	{
		for (j = 0; j < sBreadth; j++)
		{
			auxClearAll(mapX + i, mapY + j, AUXBITS_ANY_BUILDING | AUXBITS_OUR_BUILDING);
		}
	}
}

static void auxStructureBlocking(STRUCTURE *psStructure)
{
	int i, j;
	int sWidth = getStructureWidth(psStructure);
	int sBreadth = getStructureBreadth(psStructure);
	int mapX = map_coord(psStructure->pos.x - sWidth * TILE_UNITS / 2);
	int mapY = map_coord(psStructure->pos.y - sBreadth * TILE_UNITS / 2);

	for (i = 0; i < sWidth; i++)
	{
		for (j = 0; j < sBreadth; j++)
		{
			auxSetAllied(mapX + i, mapY + j, psStructure->player, AUXBITS_OUR_BUILDING);
			auxSetAll(mapX + i, mapY + j, AUXBITS_ANY_BUILDING);
		}
	}
}

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

BOOL structureIsBlueprint(STRUCTURE *psStructure)
{
	return (psStructure->status == SS_BLUEPRINT_VALID ||
	        psStructure->status == SS_BLUEPRINT_INVALID ||
	        psStructure->status == SS_BLUEPRINT_PLANNED);
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

	STATIC_ASSERT(MAX_FACTORY <= 8*sizeof(**factoryNumFlag));  // MAX_FACTORY too big for the 'factoryNumFlag' bitmask. (If changing 'factoryNumFlag', change 'mask' everywhere in this file, too.)
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
	uint32_t     mask = 0;
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

static const struct
{
	const char*     typeName;
	STRUCTURE_TYPE  type;
} structureTypeNames[] =
{
	{ "HQ",                 REF_HQ                  },
	{ "FACTORY",            REF_FACTORY             },
	{ "FACTORY MODULE",     REF_FACTORY_MODULE      },
	{ "RESEARCH",           REF_RESEARCH            },
	{ "RESEARCH MODULE",    REF_RESEARCH_MODULE     },
	{ "POWER GENERATOR",    REF_POWER_GEN           },
	{ "POWER MODULE",       REF_POWER_MODULE        },
	{ "RESOURCE EXTRACTOR", REF_RESOURCE_EXTRACTOR  },
	{ "DEFENSE",            REF_DEFENSE             },
	{ "WALL",               REF_WALL                },
	{ "CORNER WALL",        REF_WALLCORNER          },
	{ "REPAIR FACILITY",    REF_REPAIR_FACILITY     },
	{ "COMMAND RELAY",      REF_COMMAND_CONTROL     },
	{ "DEMOLISH",           REF_DEMOLISH            },
	{ "CYBORG FACTORY",     REF_CYBORG_FACTORY      },
	{ "VTOL FACTORY",       REF_VTOL_FACTORY        },
	{ "LAB",                REF_LAB                 },
	{ "DOOR",               REF_BLASTDOOR           },
	{ "REARM PAD",          REF_REARM_PAD           },
	{ "MISSILE SILO",       REF_MISSILE_SILO        },
	{ "SAT UPLINK",         REF_SAT_UPLINK          },
	{ "GATE",               REF_GATE                },
};

static STRUCTURE_TYPE structureType(const char* typeName)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(structureTypeNames); ++i)
	{
		if (strcmp(typeName, structureTypeNames[i].typeName) == 0)
		{
			return structureTypeNames[i].type;
		}
	}

	ASSERT(!"unknown structure type", "Unknown Structure Type (%s)", typeName);

	// Dummy value to prevent warnings about missing return from function
	return NUM_DIFF_BUILDINGS;
}


static const char* getStructName(const STRUCTURE_STATS* psStruct)
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
					debug(LOG_ERROR, "Cannot find the PIE for factory module %d - %s", module, GfxFile);
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
					debug(LOG_ERROR, "Cannot find the PIE for vtol factory module %d - %s", module, GfxFile);
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
				debug(LOG_ERROR, "Cannot find the PIE for research module %d - %s", module, GfxFile);
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
				debug(LOG_ERROR, "Cannot find the PIE for power module %d - %s", module, GfxFile);
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
	unsigned int        NumStructures = numCR(pStructData, bufferSize);
	UDWORD i, inc, player, numWeaps, weapSlots;
	char				StructureName[MAX_STR_LENGTH], foundation[MAX_STR_LENGTH],
						type[MAX_STR_LENGTH], dummy[MAX_STR_LENGTH],
						strength[MAX_STR_LENGTH];
	char				GfxFile[MAX_STR_LENGTH], baseIMD[MAX_STR_LENGTH];
	char				ecmType[MAX_STR_LENGTH], sensorType[MAX_STR_LENGTH];
	STRUCTURE_STATS		*psStructure, *pStartStats;
	ECM_STATS*			pECMType;
	SENSOR_STATS*		pSensorType;
	UDWORD				module;
	UDWORD				iID;
	UDWORD              dummyVal;

	// Skip descriptive header
	if (strncmp(pStructData,"Structure ",10)==0)
	{
		pStructData = strchr(pStructData,'\n') + 1;
		NumStructures--;
	}
	
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
		debug( LOG_FATAL, "Structure Stats - Out of memory" );
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
			StructureName, type, dummy, strength, &dummyVal,
			&psStructure->baseWidth, &psStructure->baseBreadth, foundation,
			&psStructure->buildPoints, &psStructure->height,
			&psStructure->armourValue, &psStructure->bodyPoints,
			&dummyVal, &psStructure->powerToBuild,
			&dummyVal, &psStructure->resistance,
			&dummyVal, &dummyVal,
			ecmType, sensorType, &weapSlots, GfxFile,
			baseIMD, &psStructure->numFuncs, &numWeaps);

#if MAX_PLAYERS != 4 && MAX_PLAYERS != 8
# error Invalid number of players
#endif

		//allocate storage for the name
		psStructure->pName = allocateName(StructureName);
		if (!psStructure->pName)
		{
			return false;
		}

		psStructure->ref = REF_STRUCTURE_START + i;

		//determine the structure type
		psStructure->type = structureType(type);

		//set the struct strength
		psStructure->strength = getStructStrength(strength);
		if (psStructure->strength == INVALID_STRENGTH)
		{
			debug(LOG_ERROR, "Unknown structure strength for %s", getStatName(psStructure));
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
			ASSERT_OR_RETURN(false, psStructure->pSensor != NULL, "Should have a sensor attached to %s!", StructureName);
			ASSERT_OR_RETURN(false, !(numWeaps && psStructure->pSensor->location == LOC_TURRET),
			                 "Both turret sensor and weapon have been assigned to %s", StructureName);
		}

		//get the IMD for the structure
		psStructure->pIMD = (iIMDShape *) resGetData("IMD", GfxFile);
		ASSERT_OR_RETURN(false, psStructure->pIMD != NULL, "Cannot find the structure PIE for record %s", getStructName(psStructure));

		if (strcmp(baseIMD, "0"))
		{
			psStructure->pBaseIMD = (iIMDShape *) resGetData("IMD", baseIMD);
			ASSERT_OR_RETURN(false, psStructure->pIMD != NULL, "Cannot find the structure base PIE for record %s", getStructName(psStructure));
		}
		else
		{
			psStructure->pBaseIMD = NULL;
		}

		initModulePIEs(GfxFile,i,psStructure);

		//Only having one weapon per structure now...AB 24/01/99
		if (weapSlots > STRUCT_MAXWEAPS || numWeaps > weapSlots)
		{
			debug(LOG_ERROR, "Allocated more weapons than allowed for Structure");
			return false;
		}
		//I need numWeaps to draw multiple weapons
		psStructure->numWeaps = numWeaps;

		//allocate storage for the functions - if any
		psStructure->defaultFunc = -1;
		if (psStructure->numFuncs > 0)
		{
			psStructure->asFuncList = (FUNCTION **)malloc(psStructure->numFuncs *
				sizeof(FUNCTION*));
			if (psStructure->asFuncList == NULL)
			{
				debug( LOG_FATAL, "Out of memory assigning structure Functions" );
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
		debug(LOG_ERROR, "destroy structure stat not found");
		return false;
	}
	g_psStatDestroyStruct = asStructureStats + iID;

	//allocate the structureLimits structure
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		asStructLimits[player] = malloc(sizeof(STRUCTURE_LIMITS) * numStructureStats);
		if (asStructLimits[player] == NULL)
		{
			debug( LOG_FATAL, "Unable to allocate structure limits" );
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
				ASSERT(psStructLimits[inc].currentQuantity <= psStructLimits[inc].limit,
				       "There appears to be too many %s on this map!", getStructName(&asStructureStats[inc]));
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

							//read and store multiple weapon Stats
							pStructure[incS].psWeapStat[j] = &pWeapon[incW];
							break;
						}
					}
					//if weapon not found - error
					if (!weaponFound)
					{
						debug(LOG_ERROR, "Unable to find stats for weapon %s for structure %s", WeaponName[i], StructureName);
						return false;
					}
				}
			}
		}
		//if structure not found - error
		if (!structureFound)
		{
			debug(LOG_ERROR, "Unable to find stats for structure %s", StructureName);
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
							debug(LOG_ERROR, "Trying to allocate more functions than allowed for Structure");
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
					debug(LOG_ERROR, "Unable to find stats for function %s for structure %s", FunctionName, StructureName);
					return false;
				}
			}
		}
		//if structure not found - error
		if (!structureFound)
		{
			debug(LOG_ERROR, "Unable to find stats for structure %s", StructureName);
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
				debug(LOG_ERROR, "Unknown Corner Wall stat for function %s", pFunction->pName);
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
		if (!getWeaponEffect(weaponEffectName, &effectInc))
		{
			debug(LOG_ERROR, "Invalid Weapon Effect - %s", weaponEffectName);
			return false;
		}
		//get the propulsion inc
		strengthInc = getStructStrength(strengthName);
		if (strengthInc == INVALID_STRENGTH)
		{
			debug(LOG_ERROR, "Invalid Strength type - %s", strengthName);
			return false;
		}

		if (modifier > UWORD_MAX)
		{
			debug(LOG_ERROR, "modifier for effect %s, strength %s is too large", weaponEffectName, strengthName);
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
	// TODO: do something here
}

/* Deals damage to a Structure.
 * \param psStructure structure to deal damage to
 * \param damage amount of damage to deal
 * \param weaponClass the class of the weapon that deals the damage
 * \param weaponSubClass the subclass of the weapon that deals the damage
 * \return < 0 when the dealt damage destroys the structure, > 0 when the structure survives
 */
int32_t structureDamage(STRUCTURE *psStructure, UDWORD damage, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass, HIT_SIDE impactSide)
{
	int32_t relativeDamage;

	CHECK_STRUCTURE(psStructure);

	debug(LOG_ATTACK, "structure id %d, body %d, armour %d, damage: %d",
		  psStructure->id, psStructure->body, psStructure->armour[impactSide][weaponClass], damage);

	relativeDamage = objDamage((BASE_OBJECT *)psStructure, damage, structureBody(psStructure), weaponClass, weaponSubClass, impactSide);

	// If the shell did sufficient damage to destroy the structure
	if (relativeDamage < 0)
	{
		debug(LOG_ATTACK, "Structure (id %d) DESTROYED", psStructure->id);
		destroyStruct(psStructure);
	}
	else
	{
		// Survived
		CHECK_STRUCTURE(psStructure);
	}

	return relativeDamage;
}

int32_t getStructureDamage(const STRUCTURE *psStructure)
{
	int32_t health;
	CHECK_STRUCTURE(psStructure);

	health = (int64_t)65536 * psStructure->body / structureBody(psStructure);
	CLIP(health, 0, 65536);

	return 65536 - health;
}

/// Add buildPoints to the structures currentBuildPts, due to construction work by the droid
/// Also can deconstruct (demolish) a building if passed negative buildpoints
void structureBuild(STRUCTURE *psStruct, DROID *psDroid, int buildPoints)
{
	int before, after;
	int powerNeeded;
	int buildPointsToAdd;
	int player;
	int newBuildPoints = (int)psStruct->currentBuildPts; // beware, unsigned
	DROID *psCurr;
	newBuildPoints += buildPoints;
	ASSERT(newBuildPoints <= 1 + 3 * (int)psStruct->pStructureType->buildPoints, "unsigned int underflow?");
	CLIP(newBuildPoints, 0, psStruct->pStructureType->buildPoints);

	if (psDroid && !aiCheckAlliances(psStruct->player,psDroid->player))
	{
		// Enemy structure
		buildPoints = 0;
		powerNeeded = 0;
		buildPointsToAdd = 0;
	}
	else if (psStruct->pStructureType->type != REF_FACTORY_MODULE)
	{
		for (player = 0; player < 8; player++)
		{
			for (psCurr = apsDroidLists[player]; psCurr; psCurr = psCurr->psNext)
			{
				// An enemy droid is blocking it
				if ((STRUCTURE *) orderStateObj(psCurr, DORDER_BUILD) == psStruct
				 && !aiCheckAlliances(psStruct->player,psCurr->player))
				{
					buildPoints = 0;
					powerNeeded = 0;
					buildPointsToAdd = 0;
					break;
				}
			}
		}
	}
	if (buildPoints > 0)
	{
		// Check if there is enough power to perform this construction work
		powerNeeded = (newBuildPoints * structPowerToBuild(psStruct))/psStruct->pStructureType->buildPoints -
		               (psStruct->currentBuildPts * structPowerToBuild(psStruct))/psStruct->pStructureType->buildPoints;
		if (buildPoints > newBuildPoints - psStruct->currentBuildPts)
		{
			buildPoints = newBuildPoints - psStruct->currentBuildPts + 1;
		}
		buildPointsToAdd = requestPowerFor(psStruct->player, powerNeeded, buildPoints);
	}
	else
	{
		// get half the power back for demolishing 
		powerNeeded = (newBuildPoints * structureTotalReturn(psStruct))/(psStruct->pStructureType->buildPoints) -
		               (psStruct->currentBuildPts * structureTotalReturn(psStruct))/(psStruct->pStructureType->buildPoints);
		buildPointsToAdd = buildPoints;
		addPower(psStruct->player, -powerNeeded);
	}

	newBuildPoints = (int)psStruct->currentBuildPts; // beware, unsigned
	newBuildPoints += buildPointsToAdd;

	ASSERT(newBuildPoints <= 1 + 3 * (int)psStruct->pStructureType->buildPoints, "unsigned int underflow?");
	CLIP(newBuildPoints, 0, psStruct->pStructureType->buildPoints);

	before = (9 * psStruct->currentBuildPts * structureBody(psStruct) ) / (10 * psStruct->pStructureType->buildPoints);
	psStruct->currentBuildPts = newBuildPoints;
	after =  (9 * psStruct->currentBuildPts * structureBody(psStruct) ) / (10 * psStruct->pStructureType->buildPoints);
	psStruct->body += after - before;

	//check if structure is built
	if (buildPoints > 0 && psStruct->currentBuildPts >= (SDWORD)psStruct->pStructureType->buildPoints)
	{
		psStruct->currentBuildPts = (SWORD)psStruct->pStructureType->buildPoints;
		psStruct->status = SS_BUILT;
		buildingComplete(psStruct);

		if (psDroid)
		{
			intBuildFinished(psDroid);
		}

		if((bMultiMessages) && myResponsibility(psStruct->player))
		{
			SendBuildFinished(psStruct);
		}

		//only play the sound if selected player
		if (psDroid &&
		    psStruct->player == selectedPlayer
		 && (psDroid->order != DORDER_LINEBUILD
		  || (map_coord(psDroid->orderX) == map_coord(psDroid->orderX2)
		   && map_coord(psDroid->orderY) == map_coord(psDroid->orderY2))))
		{
			audio_QueueTrackPos( ID_SOUND_STRUCTURE_COMPLETED,
					psStruct->pos.x, psStruct->pos.y, psStruct->pos.z );
			intRefreshScreen();		// update any open interface bars.
		}

		/* must reset here before the callback, droid must have DACTION_NONE
		     in order to be able to start a new built task, doubled in actionUpdateDroid() */
		if (psDroid)
		{
			DROID	*psIter;

			// Clear all orders for helping hands. Needed for AI script which runs next frame.
			for (psIter = apsDroidLists[psDroid->player]; psIter; psIter = psIter->psNext)
			{
				if ((psIter->order == DORDER_BUILD || psIter->order == DORDER_HELPBUILD || psIter->order == DORDER_LINEBUILD)
				    && psIter->psTarget == (BASE_OBJECT *)psStruct
				    && (psIter->order != DORDER_LINEBUILD || (map_coord(psIter->orderX) == map_coord(psIter->orderX2)
				                                              && map_coord(psIter->orderY) == map_coord(psIter->orderY2))))
				{
					objTrace(psIter->id, "Construction order %s complete (%d, %d -> %d, %d)", getDroidOrderName(psDroid->order),
					         (int)psIter->orderX, (int)psIter->orderY, (int)psIter->orderX2, (int)psIter->orderY2);
					psIter->action = DACTION_NONE;
					psIter->order = DORDER_NONE;
					setDroidTarget(psIter, NULL);
					setDroidActionTarget(psIter, NULL, 0);
					psIter->psTarStats = NULL;
				}
			}

			/* Notify scripts we just finished building a structure, pass builder and what was built */
			psScrCBNewStruct	= psStruct;
			psScrCBNewStructTruck	= psDroid;
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_STRUCTBUILT);

			audio_StopObjTrack( psDroid, ID_SOUND_CONSTRUCTION_LOOP );
		}

		/* Not needed, but left for backward compatibility */
		structureCompletedCallback(psStruct->pStructureType);
	}
	else
	{
		psStruct->status = SS_BEING_BUILT;
	}
	if (buildPoints < 0 && psStruct->currentBuildPts == 0)
	{
		removeStruct(psStruct, true);
	}
}

static int structureTotalReturn(STRUCTURE *psStruct)
{
	int result = structPowerToBuild(psStruct)/2.0f;

	if(psStruct->pStructureType->type == REF_POWER_GEN)
	{
		//if had module attached - the base must have been completely built
		if (psStruct->pFunctionality->powerGenerator.capacity)
		{
			//so add the power required to build the base struct
			result += psStruct->pStructureType->powerToBuild/2.0f;
		}
	}
	else
	{
		//if it had a module attached, need to add the power for the base struct as well
		if (StructIsFactory(psStruct))
		{
			if (psStruct->pFunctionality->factory.capacity)
			{
				//if large factory - add half power for one upgrade
				if (psStruct->pFunctionality->factory.capacity > SIZE_MEDIUM)
				{
					result += structPowerToBuild(psStruct) / 2.0f;
				}
			}
		}
		else if (psStruct->pStructureType->type == REF_RESEARCH)
		{
			if (psStruct->pFunctionality->researchFacility.capacity)
			{
				//add half power for base struct
				result += psStruct->pStructureType->powerToBuild/2.0f;
			}
		}
	}
	return result;
}

void structureDemolish(STRUCTURE *psStruct, DROID *psDroid, int buildPoints)
{
	structureBuild(psStruct, psDroid, -buildPoints);
}

BOOL structureRepair(STRUCTURE *psStruct, DROID *psDroid, int buildPoints)
{
	int repairAmount = (buildPoints * structureBody(psStruct))/psStruct->pStructureType->buildPoints;
	/*	(droid construction power * current max hitpoints [incl. upgrades])
			/ construction power that was necessary to build structure in the first place
	
	=> to repair a building from 1HP to full health takes as much time as building it.
	=> if buildPoints = 1 and structureBody < buildPoints, repairAmount might get truncated to zero.
		This happens with expensive, but weak buildings like mortar pits. In this case, do nothing
		and notify the caller (read: droid) of your idleness by returning false.
	*/
	if (repairAmount != 0)  // didn't get truncated to zero
	{
		psStruct->body += repairAmount;
		psStruct->body = MIN(psStruct->body, structureBody(psStruct));
		if (psStruct->body == 0)
		{
			removeStruct(psStruct, true);
		}
		return true;
	} 
	else
	{
		// got truncated to zero; wait until droid has accumulated enough buildpoints
		return false;
	}
}

/* Set the type of droid for a factory to build */
BOOL structSetManufacture(STRUCTURE *psStruct, DROID_TEMPLATE *psTempl, QUEUE_MODE mode)
{
	FACTORY *psFact;

	CHECK_STRUCTURE(psStruct);

	ASSERT_OR_RETURN(false, psStruct != NULL && psStruct->type == OBJ_STRUCTURE, "Invalid factory pointer");
	ASSERT_OR_RETURN(false, psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
	                 || psStruct->pStructureType->type == REF_VTOL_FACTORY, "Invalid structure type %d for factory",
	                 (int)psStruct->pStructureType->type);
	/* psTempl might be NULL if the build is being cancelled in the middle */

	psFact = &psStruct->pFunctionality->factory;

	if (mode == ModeQueue)
	{
		sendStructureInfo(psStruct, STRUCTUREINFO_MANUFACTURE, psTempl);
		psStruct->pFunctionality->factory.psSubjectPending = (BASE_STATS *)psTempl;
		psStruct->pFunctionality->factory.statusPending = FACTORY_START_PENDING;
		++psFact->pendingCount;

		return true;  // Wait for our message before doing anything.
	}

	//assign it to the Factory
	psFact->psSubject = (BASE_STATS *)psTempl;

	//set up the start time and build time
	if (psTempl != NULL)
	{
		//only use this for non selectedPlayer
		if (psStruct->player != selectedPlayer)
		{
			//set quantity to produce
			psFact->productionLoops = 1;
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
	if (psStruct->player == productionPlayer)
	{
		intUpdateManufacture(psStruct);
	}
	return true;
}


/*****************************************************************************/
/*
* All this wall type code is horrible, but I really can't think of a better way to do it.
*        John.
*/

// look at where other walls are to decide what type of wall to build
static SDWORD structWallScan(BOOL aWallPresent[5][5], SDWORD x, SDWORD y)
{
	if (aWallPresent[x-1][y] || aWallPresent[x+1][y])
	{
		if (aWallPresent[x][y-1] || aWallPresent[x][y+1])
		{
			return WALL_CORNER;
		}
		else
		{
			return WALL_HORIZ;
		}
	}
	else
	{
		return WALL_VERT;
	}
}

// Choose a type of wall for a location - and update any neighbouring walls
static SDWORD structChooseWallType(UDWORD player, UDWORD mapX, UDWORD mapY)
{
	BOOL		aWallPresent[5][5];
	SDWORD		xdiff,ydiff, x,y;
	STRUCTURE	*psStruct;
	STRUCTURE	*apsStructs[5][5];
	SDWORD		neighbourType, scanType;
	STRUCTURE_STATS	*psStats;
	UDWORD		sx,sy;

	// scan around the location looking for walls
	memset(aWallPresent, 0, sizeof(aWallPresent));
	for(psStruct=apsStructLists[player]; psStruct; psStruct=psStruct->psNext)
	{
		xdiff = (SDWORD)mapX - map_coord((SDWORD)psStruct->pos.x);
		ydiff = (SDWORD)mapY - map_coord((SDWORD)psStruct->pos.y);
		if (xdiff >= -2 && xdiff <= 2 &&
			ydiff >= -2 && ydiff <= 2 &&
			(psStruct->pStructureType->type == REF_WALL ||
			psStruct->pStructureType->type == REF_GATE ||
			psStruct->pStructureType->type == REF_WALLCORNER ||
			(psStruct->pStructureType->type == REF_DEFENSE && psStruct->pStructureType->strength == STRENGTH_HARD) || 
			(psStruct->pStructureType->type == REF_BLASTDOOR && psStruct->pStructureType->strength == STRENGTH_HARD))) // fortresses
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
				if (psStruct->pStructureType->type == REF_WALL || psStruct->pStructureType->type == REF_GATE)
				{
					if (psStruct->rot.direction == DEG(90))
					{
						neighbourType = WALL_VERT;
					}
					else
					{
						neighbourType = WALL_HORIZ;
					}
				}
				else
				{
					// do not need to adjust anything apart from walls
					continue;
				}

				// see what type the wall should be
				scanType = structWallScan(aWallPresent, x,y);

				if (neighbourType != scanType)
				{
					// Got to change the wall
					if (scanType == WALL_CORNER)
					{
						// change to a corner
						if (psStruct->pStructureType->asFuncList && psStruct->pStructureType->asFuncList[0]->type == WALL_TYPE)
						{
							const int     oldBody = psStruct->body;
							UDWORD        oldBuildPoints = psStruct->currentBuildPts;
							STRUCT_STATES oldStatus = psStruct->status;

							psStats = ((WALL_FUNCTION *)psStruct->pStructureType->asFuncList[0])->pCornerStat;
							sx = psStruct->pos.x; sy = psStruct->pos.y;
							removeStruct(psStruct, true);
							powerCalc(false);
							psStruct = buildStructure(psStats, sx,sy, player, true);
							powerCalc(true);
							if (psStruct != NULL)
							{
								psStruct->status = oldStatus;
								psStruct->body = oldBody;
								psStruct->currentBuildPts = (SWORD)oldBuildPoints;
								if (oldStatus != SS_BEING_BUILT)
								{
									buildingComplete(psStruct);
								}
							}
						}
					}
					else if (scanType == WALL_HORIZ)
					{
						// change to a horizontal wall
						psStruct->rot.direction = 0;
					}
					else
					{
						// change to a vertical wall
						psStruct->rot.direction = DEG(90);
					}
				}
			}
		}
	}

	// finally return the type for this wall
	return structWallScan(aWallPresent, 2,2);
}


static void buildFlatten(STRUCTURE *pStructure, UDWORD x, UDWORD y, UDWORD h)
{
	unsigned width;
	unsigned breadth;

	for (breadth = 0; breadth <= getStructureBreadth(pStructure); ++breadth)
	{
		for (width = 0; width <= getStructureWidth(pStructure); ++width)
		{
			if (pStructure->pStructureType->type != REF_WALL
			    && pStructure->pStructureType->type != REF_WALLCORNER
			    && pStructure->pStructureType->type != REF_GATE)
			{
				setTileHeight(x + width, y + breadth, h);//-1
				// We need to raise features on raised tiles to the new height
				if(TileHasFeature(mapTile(x+width,y+breadth)))
				{
					getTileFeature(x+width, y+breadth)->pos.z = (UWORD)h;
				}
			}
		}
	}
}

void alignStructure(STRUCTURE *psBuilding)
{
	int x = psBuilding->pos.x;
	int y = psBuilding->pos.y;
	unsigned sWidth   = getStructureWidth(psBuilding);
	unsigned sBreadth = getStructureBreadth(psBuilding);
	int mapX = map_coord(x) - sWidth/2;
	int mapY = map_coord(y) - sBreadth/2;

	/* DEFENSIVE structures are pulled to the terrain */
	if (psBuilding->pStructureType->type != REF_DEFENSE)
	{
		int mapH = buildFoundation(psBuilding, x, y);

		buildFlatten(psBuilding, mapX, mapY, mapH);
		psBuilding->pos.z = mapH;
		psBuilding->foundationDepth = 0.0f;
	}
	else
	{
		iIMDShape	*strImd = psBuilding->sDisplay.imd;
		int i;
		float pointHeight;

		psBuilding->pos.z = TILE_MIN_HEIGHT;
		psBuilding->foundationDepth = TILE_MAX_HEIGHT;

		// Now we got through the shape looking for vertices on the edge
                for (i = 0; i < strImd->npoints; i++)
		{
			pointHeight = map_Height(psBuilding->pos.x + strImd->points[i].x, psBuilding->pos.y - strImd->points[i].z);
			if (pointHeight > psBuilding->pos.z)
			{
				psBuilding->pos.z = pointHeight;
			}
			if (pointHeight < psBuilding->foundationDepth)
			{
				psBuilding->foundationDepth = pointHeight;
			}
		}
	}
}

/*Builds an instance of a Structure - the x/y passed in are in world coords. */
STRUCTURE *buildStructure(STRUCTURE_STATS *pStructureType, UDWORD x, UDWORD y, UDWORD player, BOOL FromSave)
{
	return buildStructureDir(pStructureType, x, y, 0, player, FromSave);
}

STRUCTURE* buildStructureDir(STRUCTURE_STATS *pStructureType, UDWORD x, UDWORD y, uint16_t direction, UDWORD player, BOOL FromSave)
{
	STRUCTURE	*psBuilding = NULL;
	unsigned sWidth   = getStructureStatsWidth  (pStructureType, direction);
	unsigned sBreadth = getStructureStatsBreadth(pStructureType, direction);

	ASSERT_OR_RETURN(NULL, pStructureType && pStructureType->type != REF_DEMOLISH, "You cannot build demolition!");

	if (IsStatExpansionModule(pStructureType)==false)
	{
		SDWORD	wallType = 0, preScrollMinX = 0, preScrollMinY = 0, preScrollMaxX = 0, preScrollMaxY = 0;
		UDWORD	max = pStructureType - asStructureStats;
		UDWORD	mapX, mapY;
		UDWORD	width, breadth;
		int	i;

		ASSERT_OR_RETURN(NULL, max <= numStructureStats, "Invalid structure type");

		if (!strcmp(pStructureType->pName, "A0CyborgFactory") && player == 0 && !bMultiPlayer)
		{
			// HACK: correcting SP bug, needs fixing in script(!!) (only applies for player 0)
			// should be OK for Skirmish/MP games, since that is set correctly.
			// scrSetStructureLimits() is called by scripts to set this normally.
			asStructLimits[player][max].limit = MAX_FACTORY;
			asStructLimits[player][max].globalLimit = MAX_FACTORY;
		}

		if (!strcmp(pStructureType->pName, "A0CyborgFactory") && player == 0 && !bMultiPlayer)
		{
			// HACK: correcting SP bug, needs fixing in script(!!) (only applies for player 0)
			// should be OK for Skirmish/MP games, since that is set correctly.
			// scrSetStructureLimits() is called by scripts to set this normally.
			asStructLimits[player][max].limit = MAX_FACTORY;
			asStructLimits[player][max].globalLimit = MAX_FACTORY;
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
		x = (x & ~TILE_MASK) + sWidth  %2 * TILE_UNITS/2;
		y = (y & ~TILE_MASK) + sBreadth%2 * TILE_UNITS/2;

		//check not trying to build too near the edge
		if (map_coord(x) < TOO_NEAR_EDGE || map_coord(x) > (mapWidth - TOO_NEAR_EDGE))
		{
			debug(LOG_ERROR, "attempting to build too closely to map-edge, "
				  "x coord (%u) too near edge (req. distance is %u)", x, TOO_NEAR_EDGE);
			return NULL;
		}
		if (map_coord(y) < TOO_NEAR_EDGE || map_coord(y) > (mapHeight - TOO_NEAR_EDGE))
		{
			debug(LOG_ERROR, "attempting to build too closely to map-edge, "
				  "y coord (%u) too near edge (req. distance is %u)", y, TOO_NEAR_EDGE);
			return NULL;
		}

		if (!FromSave && (pStructureType->type == REF_WALL || pStructureType->type == REF_GATE))
		{
			wallType = structChooseWallType(player, map_coord(x), map_coord(y));
			if (wallType == WALL_CORNER && pStructureType->type != REF_GATE)
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

		psBuilding->pos.x = x;
		psBuilding->pos.y = y;
		psBuilding->rot.direction = (direction + 0x2000)&0xC000;
		psBuilding->rot.pitch = 0;
		psBuilding->rot.roll = 0;

		//This needs to be done before the functionality bit...
		//load into the map data and structure list if not an upgrade
		mapX = map_coord(x) - sWidth/2;
		mapY = map_coord(y) - sBreadth/2;

		//set up the imd to use for the display
		psBuilding->sDisplay.imd = pStructureType->pIMD;

		psBuilding->state = SAS_NORMAL;
		psBuilding->lastStateTime = gameTime;

		/* if resource extractor - need to remove oil feature first, but do not do any
		 * consistency checking here - save games do not have any feature to remove
		 * to remove when placing oil derricks! */
		if (pStructureType->type == REF_RESOURCE_EXTRACTOR)
		{
			FEATURE *psFeature = getTileFeature(map_coord(x), map_coord(y));

			if (psFeature && psFeature->psStats->subType == FEAT_OIL_RESOURCE)
			{
				if (fireOnLocation(psFeature->pos.x,psFeature->pos.y))
				{
					// Can't build on burning oil resource
					return NULL;
				}
				// remove it from the map
				turnOffMultiMsg(true); // dont send this one!
				removeFeature(psFeature);
				turnOffMultiMsg(false);
			}
		}

		for (width = 0; width < sWidth; width++)
		{
			for (breadth = 0; breadth < sBreadth; breadth++)
			{
				MAPTILE *psTile = mapTile(mapX+width,mapY+breadth);

				/* Remove any walls underneath the building. You can build defense buildings on top
				 * of walls, you see. This is not the place to test whether we own it! */
				if (pStructureType->type == REF_DEFENSE && TileHasWall(psTile))
				{
					removeStruct((STRUCTURE *)psTile->psObject, true);
				}

				// don't really think this should be done here, but dont know otherwise.alexl
				if(pStructureType->type == REF_WALLCORNER || pStructureType->type == REF_WALL)
				{
					if(TileHasStructure(mapTile(mapX+width,mapY+breadth)))
					{
						if(getTileStructure(mapX+width,mapY+breadth)->pStructureType->type == REF_WALLCORNER)
						{
							return NULL; // dont build.
						}
					}
				}
				// end of dodgy stuff
				else if (TileHasStructure(psTile))
				{
					debug(LOG_ERROR,
						   "Player %u (%s): is building %s at (%d, %d) but found %s already at (%d, %d)",
						   player,isHumanPlayer(player) ? "Human" : "AI", pStructureType->pName, mapX, mapY,
						   getTileStructure(mapX + width, mapY + breadth)->pStructureType->pName,
						   mapX + width, mapY + breadth);
					free(psBuilding);
					return NULL;
				}

				psTile->psObject = (BASE_OBJECT*)psBuilding;

				// if it's a tall structure then flag it in the map.
				if (psBuilding->sDisplay.imd->max.y > TALLOBJECT_YMAX)
				{
					auxSetBlocking(mapX + width, mapY + breadth, AIR_BLOCKED);
				}
			}
		}

		if (pStructureType->type != REF_REARM_PAD && pStructureType->type != REF_GATE)
		{
			auxStructureBlocking(psBuilding);
		}

		//set up the rest of the data
		for (i = 0;i < STRUCT_MAXWEAPS;i++)
		{
			psBuilding->asWeaps[i].rot.direction = 0;
			psBuilding->asWeaps[i].rot.pitch = 0;
			psBuilding->asWeaps[i].rot.roll = 0;
			psBuilding->asWeaps[i].prevRot = psBuilding->asWeaps[i].rot;
			psBuilding->psTarget[i] = NULL;
			psBuilding->targetOrigin[i] = ORIGIN_UNKNOWN;
		}
		psBuilding->bTargetted = false;

		psBuilding->lastEmission = 0;
		psBuilding->timeLastHit = 0;
		psBuilding->lastHitWeapon = WSC_NUM_WEAPON_SUBCLASSES;  // no such weapon

		psBuilding->inFire = 0;
		psBuilding->burnStart = 0;
		psBuilding->burnDamage = 0;

		psBuilding->selected = false;
		psBuilding->status = SS_BEING_BUILT;
		psBuilding->currentBuildPts = 0;
		psBuilding->currentPowerAccrued = 0;
		psBuilding->cluster = 0;

		alignStructure(psBuilding);

		// rotate a wall if necessary
		if (!FromSave && (pStructureType->type == REF_WALL || pStructureType->type == REF_GATE ) && wallType == WALL_VERT)
		{
			psBuilding->rot.direction = DEG(90);
		}

		//set up the sensor stats
		objSensorCache((BASE_OBJECT *)psBuilding, psBuilding->pStructureType->pSensor);
		objEcmCache((BASE_OBJECT *)psBuilding, psBuilding->pStructureType->pECM);

		/* Store the weapons */
		memset(psBuilding->asWeaps, 0, sizeof(WEAPON));
		psBuilding->numWeaps = 0;
		if (pStructureType->numWeaps > 0)
		{
			UDWORD weapon;

			for(weapon=0; weapon < pStructureType->numWeaps; weapon++)
			{
				if (pStructureType->psWeapStat[weapon])
				{
					psBuilding->asWeaps[weapon].lastFired = 0;
					psBuilding->asWeaps[weapon].shotsFired = 0;
					//in multiPlayer make the Las-Sats require re-loading from the start
					if (bMultiPlayer && pStructureType->psWeapStat[0]->weaponSubClass == WSC_LAS_SAT)
					{
						psBuilding->asWeaps[0].lastFired = gameTime;
					}
					psBuilding->asWeaps[weapon].nStat =	pStructureType->psWeapStat[weapon] - asWeaponStats;
					psBuilding->asWeaps[weapon].ammo = (asWeaponStats + psBuilding->asWeaps[weapon].nStat)->numRounds;
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
				psBuilding->asWeaps[0].shotsFired = 0;
				//in multiPlayer make the Las-Sats require re-loading from the start
				if (bMultiPlayer && pStructureType->psWeapStat[0]->weaponSubClass == WSC_LAS_SAT)
				{
					psBuilding->asWeaps[0].lastFired = gameTime;
				}
				psBuilding->asWeaps[0].nStat =	pStructureType->psWeapStat[0] - asWeaponStats;
				psBuilding->asWeaps[0].ammo = (asWeaponStats + psBuilding->asWeaps[0].nStat)->numRounds;
				psBuilding->asWeaps[0].recoilValue = 0;
			}
		}

		// Structures currently do not have varied armour
		for (i = 0; i < NUM_HIT_SIDES; i++)
		{
			int j;

			for (j = 0; j < WC_NUM_WEAPON_CLASSES; j++)
			{
				psBuilding->armour[i][j] = (UWORD)structureArmour(pStructureType, (UBYTE)player);
			}
		}
		psBuilding->resistance = (UWORD)structureResistance(pStructureType, (UBYTE)player);
		psBuilding->lastResistance = ACTION_START_TIME;

		// Do the visibility stuff before setFunctionality - so placement of DP's can work
		memset(psBuilding->visible, 0, sizeof(psBuilding->visible));
		memset(psBuilding->seenThisTick, 0, sizeof(psBuilding->seenThisTick));

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
			// NOTE: resizeRadar() may be required here, since we change scroll limits?
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
				// NOTE: resizeRadar() may be required here, since we change scroll limits?
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
			// NOTE: resizeRadar() may be required here, since we change scroll limits?
		}

		psBuilding->body = (UWORD)structureBody(psBuilding);
		psBuilding->expectedDamage = 0;  // Begin life optimistically.

		//add the structure to the list - this enables it to be drawn whilst being built
		addStructure(psBuilding);

		clustNewStruct(psBuilding);
		asStructLimits[player][max].currentQuantity++;
	}
	else //its an upgrade
	{
		BOOL		bUpgraded = false;
		int32_t         bodyDiff = 0;

		//don't create the Structure use existing one
		psBuilding = getTileStructure(map_coord(x), map_coord(y));

		if (!psBuilding)
		{
			return NULL;
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
				bodyDiff = 65536 - getStructureDamage(psBuilding);

				++psBuilding->pFunctionality->factory.capacity;
				bUpgraded = true;
				//put any production on hold
				holdProduction(psBuilding, ModeImmediate);

				//quick check not trying to add too much
				ASSERT_OR_RETURN(NULL, psBuilding->pFunctionality->factory.productionOutput +
					((PRODUCTION_FUNCTION*)pStructureType->asFuncList[0])->productionOutput < UBYTE_MAX,
					"building factory module - production Output is too big");

				psBuilding->pFunctionality->factory.productionOutput += ((
					PRODUCTION_FUNCTION*)pStructureType->asFuncList[0])->productionOutput;
				//need to change which IMD is used for player 0
				//Need to do a check its not Barbarian really!

				if (   (bMultiPlayer && isHumanPlayer(psBuilding->player))
					|| (bMultiPlayer && (game.type == SKIRMISH) && (psBuilding->player < game.maxPlayers))
					|| !bMultiPlayer)
				{
					int capacity = psBuilding->pFunctionality->factory.capacity;

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
				bodyDiff = 65536 - getStructureDamage(psBuilding);

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
					holdResearch(psBuilding, ModeImmediate);
				}

				//need to change which IMD is used for player 0
				//Need to do a check its not Barbarian really!
				if (   (bMultiPlayer && isHumanPlayer(psBuilding->player))
					|| (bMultiPlayer && (game.type == SKIRMISH) && (psBuilding->player < game.maxPlayers))
					|| !bMultiPlayer)
				{
					int capacity = psBuilding->pFunctionality->researchFacility.capacity;

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
				bodyDiff = 65536 - getStructureDamage(psBuilding);

				//increment the power output, multiplier and capacity
				//add all the research modules in one go AB 24/06/98
				psBuilding->pFunctionality->powerGenerator.capacity = NUM_POWER_MODULES;
				bUpgraded = true;

				//need to change which IMD is used for player 0
				//Need to do a check its not Barbarian really!

				if (   (bMultiPlayer && isHumanPlayer(psBuilding->player))
					|| (bMultiPlayer && (game.type == SKIRMISH) && (psBuilding->player < game.maxPlayers))
					|| !bMultiPlayer)
				{
					psBuilding->sDisplay.imd = powerModuleIMDs[0];
				}
				//need to inform any res Extr associated that not digging until complete
				releasePowerGen(psBuilding);
				structurePowerUpgrade(psBuilding);
			}
		}
		if (bUpgraded)
		{
			//calculate the new body points of the owning structure
			psBuilding->body = (uint64_t)structureBody(psBuilding) * bodyDiff / 65536;

			//initialise the build points
			psBuilding->currentBuildPts = 0;
			psBuilding->currentPowerAccrued = 0;
			//start building again
			psBuilding->status = SS_BEING_BUILT;
			if (psBuilding->player == selectedPlayer && !FromSave)
			{
				intRefreshScreen();
			}
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

	psBuilding->prevTime = gameTime - deltaGameTime;  // Structure hasn't been updated this tick, yet.
	psBuilding->time = psBuilding->prevTime - 1;      // -1, so the times are different, even before updating.

	return psBuilding;
}

STRUCTURE *buildBlueprint(STRUCTURE_STATS *psStats, int32_t x, int32_t y, uint16_t direction, STRUCT_STATES state)
{
	STRUCTURE *blueprint;

	ASSERT_OR_RETURN(NULL, psStats != NULL, "No blueprint stats");
	ASSERT_OR_RETURN(NULL, psStats->pIMD != NULL, "No blueprint model for %s", getStatName(psStats));

	blueprint = malloc(sizeof(STRUCTURE));
	// construct the fake structure
	psStats = (STRUCTURE_STATS *)psStats;
	blueprint->pStructureType = psStats;
	blueprint->visible[selectedPlayer] = UBYTE_MAX;
	blueprint->player = selectedPlayer;
	blueprint->sDisplay.imd = psStats->pIMD;
	blueprint->pos.x = x;
	blueprint->pos.y = y;
	blueprint->pos.z = map_Height(blueprint->pos.x, blueprint->pos.y) + world_coord(1)/10;
	blueprint->rot.direction = (direction + 0x2000)&0xC000;
	blueprint->rot.pitch = 0;
	blueprint->rot.roll = 0;
	blueprint->selected = false;

	blueprint->timeLastHit = 0;
	blueprint->lastHitWeapon = UDWORD_MAX;  // Noone attacked the blueprint. Do not render special effects.

	blueprint->numWeaps = 0;
	blueprint->asWeaps[0].nStat = 0;

	// give defensive structures a weapon
	if (psStats->psWeapStat[0])
	{
		blueprint->asWeaps[0].nStat = psStats->psWeapStat[0] - asWeaponStats;
	}
	// things with sensors or ecm (or repair facilities) need these set, even if they have no official weapon
	blueprint->numWeaps = 0;
	blueprint->asWeaps[0].recoilValue = 0;
	blueprint->asWeaps[0].rot.pitch = 0;
	blueprint->asWeaps[0].rot.direction = 0;
	blueprint->asWeaps[0].rot.roll = 0;
	blueprint->asWeaps[0].prevRot = blueprint->asWeaps[0].rot;

	blueprint->expectedDamage = 0;

	// Times must be different, but don't otherwise matter.
	blueprint->time = 23;
	blueprint->prevTime = 42;

	blueprint->status = state;
	return blueprint;
}

static BOOL setFunctionality(STRUCTURE	*psBuilding, STRUCTURE_TYPE functionType)
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
			psBuilding->pFunctionality = calloc(1, sizeof(*psBuilding->pFunctionality));
			ASSERT_OR_RETURN(false, psBuilding != NULL, "Out of memory");
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
			x = map_coord(psBuilding->pos.x + (getStructureWidth(psBuilding)   + 1) * TILE_UNITS/2);
			y = map_coord(psBuilding->pos.y + (getStructureBreadth(psBuilding) + 1) * TILE_UNITS/2);

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
					ASSERT_OR_RETURN(false, false, "Invalid factory type");
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

			psPowerGen->capacity = 0;

			// Take advantage of upgrades
			structurePowerUpgrade(psBuilding);
			break;
		}
		case REF_RESOURCE_EXTRACTOR:
		{
			RES_EXTRACTOR* psResExtracter = &psBuilding->pFunctionality->resourceExtractor;

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
			psRepairFac->droidQueue = 0;

			if (!grpCreate(&((REPAIR_FACILITY*)psBuilding->pFunctionality)->psGroup))
			{
				debug(LOG_NEVER, "couldn't create repair facility group");
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
			x = map_coord(psBuilding->pos.x + (getStructureWidth(psBuilding)   + 1) * TILE_UNITS/2);
			y = map_coord(psBuilding->pos.y + (getStructureBreadth(psBuilding) + 1) * TILE_UNITS/2);

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

		// Structure types without a FUNCTIONALITY
		default:
			break;
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
	ASSERT_OR_RETURN( , StructIsFactory(psStruct),"structure not a factory");

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
			ASSERT(!"unknown factory type", "unknown factory type");
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
		ASSERT_OR_RETURN( , !missionIsOffworld(), "cannot assign a commander to a factory when off world" );

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
	if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
	{
		debug(LOG_NEVER, "failed - blocked");
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
				debug(LOG_NEVER, "failed - not vacant");
				return false;
			}
		}
	}

	debug(LOG_NEVER, "succeeded");
	return true;
}

/*find a location near to a structure to start the droid of*/
BOOL placeDroid(STRUCTURE *psStructure, UDWORD *droidX, UDWORD *droidY)
{
	SWORD			sx,sy, xmin,xmax, ymin,ymax, x,y, xmid;
	BOOL			placed;
	unsigned sWidth   = getStructureWidth(psStructure);
	unsigned sBreadth = getStructureBreadth(psStructure);

	CHECK_STRUCTURE(psStructure);

	/* Get the tile coords for the top left of the structure */
	sx = (SWORD)(psStructure->pos.x - sWidth * TILE_UNITS/2);
	sx = map_coord(sx);
	sy = (SWORD)(psStructure->pos.y - sBreadth * TILE_UNITS/2);
	sy = map_coord(sy);

	/* Find the four corners of the square */
	xmin = (SWORD)(sx - 1);
	xmax = (SWORD)(sx + sWidth);
	xmid = (SWORD)(sx + (sWidth-1)/2);
	ymin = (SWORD)(sy - 1);
	ymax = (SWORD)(sy + sBreadth);
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
		INITIAL_DROID_ORDERS initialOrders = {psStructure->pFunctionality->factory.secondaryOrder, psStructure->pFunctionality->factory.psAssemblyPoint->coords.x, psStructure->pFunctionality->factory.psAssemblyPoint->coords.y, psStructure->id};
		//create a droid near to the structure
		syncDebug("Placing new droid at (%d,%d)", x, y);
		turnOffMultiMsg(true);
		psNewDroid = buildDroid(psTempl, world_coord(x), world_coord(y), psStructure->player, false, &initialOrders);
		turnOffMultiMsg(false);
		if (!psNewDroid)
		{
			*ppsDroid = NULL;
			return false;
		}

		//set the droids order to that of the factory - AB 22/04/99
		//psNewDroid->secondaryOrder = psStructure->pFunctionality->factory.secondaryOrder;
		if (myResponsibility(psStructure->player))
		{
			uint32_t newState = psStructure->pFunctionality->factory.secondaryOrder;
			uint32_t diff = newState ^ psNewDroid->secondaryOrder;
			if ((diff & DSS_ARANGE_MASK) != 0)
			{  // TODO Should either do this for all states, or synchronise factory.secondaryOrder.
				secondarySetState(psNewDroid, DSO_ATTACK_RANGE, newState & DSS_ARANGE_MASK);
			}
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
			syncDebug("Has commander.");
			if (idfDroid(psNewDroid) ||
				isVtolDroid(psNewDroid))
			{
				orderDroidObj(psNewDroid, DORDER_FIRESUPPORT, (BASE_OBJECT *)psFact->psCommander, ModeImmediate);
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
			if (isVtolDroid(psNewDroid) && psNewDroid->droidType != DROID_TRANSPORTER)
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

				if (isVtolDroid(psNewDroid))
				{
					UDWORD droidX = psFlag->coords.x;
					UDWORD droidY = psFlag->coords.y;
					//find a suitable location near the delivery point
					actionVTOLLandingPos(psNewDroid, &droidX, &droidY);
					orderDroidLoc(psNewDroid, DORDER_MOVE, droidX, droidY, ModeQueue);
				}
				else
				{
					orderDroidLoc(psNewDroid, DORDER_MOVE, psFlag->coords.x, psFlag->coords.y, ModeQueue);
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


static bool IsFactoryCommanderGroupFull(const FACTORY* psFactory)
{
	unsigned int DroidsInGroup;

	// If we don't have a commander return false (group not full)
	if (psFactory->psCommander==NULL) return false;

	// allow any number of IDF droids
	if (templateIsIDF((DROID_TEMPLATE *)psFactory->psSubject))
	{
		return false;
	}

	// Get the number of droids in the commanders group
	DroidsInGroup = psFactory->psCommander->psGroup ? grpNumMembers(psFactory->psCommander->psGroup) : 0;

	// if the number in group is less than the maximum allowed then return false (group not full)
	if (DroidsInGroup < cmdDroidMaxGroup(psFactory->psCommander))
		return false;

	// the number in group has reached the maximum
	return true;
}


// Disallow manufacture of units once these limits are reached,
// doesn't mean that these numbers can't be exceeded if units are
// put down in the editor or by the scripts.
static UWORD MaxDroidsAllowedPerPlayer[MAX_PLAYERS] = {100, 999, 999, 999, 999, 999, 999, 999};
// FIXME: We should have this variable user defined.
static UWORD MaxDroidsAllowedPerPlayerMultiPlayer[MAX_PLAYERS] = {450, 450, 450, 450, 450, 450, 450, 450};

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

	if (droidTemplateType((DROID_TEMPLATE *)psFact->psSubject) == DROID_COMMAND
	 && getNumCommandDroids(psStructure->player) >= MAX_COMMAND_DROIDS)
	{
		return true;
	}

	if ((droidTemplateType((DROID_TEMPLATE *)psFact->psSubject) == DROID_CONSTRUCT
	  || droidTemplateType((DROID_TEMPLATE *)psFact->psSubject) == DROID_CYBORG_CONSTRUCT)
	 && getNumConstructorDroids(psStructure->player) >= MAX_CONSTRUCTOR_DROIDS)
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


static void aiUpdateStructure(STRUCTURE *psStructure, bool isMission)
{
	BASE_STATS			*pSubject = NULL;
	UDWORD				pointsToAdd;//, iPower;
	PLAYER_RESEARCH		*pPlayerRes = asPlayerResList[psStructure->player];
	RESEARCH			*pResearch;
	UDWORD				structureMode = 0;
	DROID				*psDroid;
	BASE_OBJECT			*psChosenObjs[STRUCT_MAXWEAPS] = {NULL};
	BASE_OBJECT			*psChosenObj = NULL;
	SDWORD				iDt;
	FACTORY				*psFactory;
	REPAIR_FACILITY		*psRepairFac = NULL;
	RESEARCH_FACILITY	*psResFacility;
	Vector3i iVecEffect;
	BOOL				bFinishAction, bDroidPlaced = false;
	WEAPON_STATS		*psWStats;
	SDWORD				xdiff,ydiff, mindist, currdist;
	UDWORD				i;
	UWORD 				tmpOrigin = ORIGIN_UNKNOWN;

	CHECK_STRUCTURE(psStructure);

	if (psStructure->time == gameTime)
	{
		// This isn't supposed to happen, and really shouldn't be possible - if this happens, maybe a structure is being updated twice?
		int count1 = 0, count2 = 0;
		STRUCTURE *s;
		for (s =         apsStructLists[psStructure->player]; s != NULL; s = s->psNext) count1 += s == psStructure;
		for (s = mission.apsStructLists[psStructure->player]; s != NULL; s = s->psNext) count2 += s == psStructure;
		debug(LOG_ERROR, "psStructure->prevTime = %u, psStructure->time = %u, gameTime = %u, count1 = %d, count2 = %d", psStructure->prevTime, psStructure->time, gameTime, count1, count2);
		--psStructure->time;
	}
	psStructure->prevTime = psStructure->time;
	psStructure->time = gameTime;
	for (i = 0; i < MAX(1, psStructure->numWeaps); ++i)
	{
		psStructure->asWeaps[i].prevRot = psStructure->asWeaps[i].rot;
	}

	if (isMission)
	{
		switch (psStructure->pStructureType->type)
		{
			case REF_RESEARCH:
			case REF_FACTORY:
			case REF_CYBORG_FACTORY:
			case REF_VTOL_FACTORY:
				break;
			default:
				return; // nothing to do
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
			psStructure->asWeaps[0].rot.direction = (uint16_t)((uint64_t)gameTime * 65536 / 3000);  // Cast wrapping intended.
			psStructure->asWeaps[0].rot.pitch = 0;
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
				if (aiChooseTarget((BASE_OBJECT *)psStructure, &psChosenObjs[i], i, true, &tmpOrigin) )
				{
					objTrace(psStructure->id, "Weapon %d is targeting %d at (%d, %d)", i, psChosenObjs[i]->id,
						psChosenObjs[i]->pos.x, psChosenObjs[i]->pos.y);
					setStructureTarget(psStructure, psChosenObjs[i], i, tmpOrigin);
				}
				else
				{
					if ( aiChooseTarget((BASE_OBJECT *)psStructure, &psChosenObjs[0], 0, true, &tmpOrigin) )
					{
						if (psChosenObjs[0])
						{
							objTrace(psStructure->id, "Weapon %d is supporting main weapon: %d at (%d, %d)", i,
								psChosenObjs[0]->id, psChosenObjs[0]->pos.x, psChosenObjs[0]->pos.y);
							setStructureTarget(psStructure, psChosenObjs[0], i, tmpOrigin);
							psChosenObjs[i] = psChosenObjs[0];
						}
						else
						{
							setStructureTarget(psStructure, NULL, i, ORIGIN_UNKNOWN);
							psChosenObjs[i] = NULL;
						}
					}
					else
					{
						setStructureTarget(psStructure, NULL, i, ORIGIN_UNKNOWN);
						psChosenObjs[i] = NULL;
					}
				}

				if (psChosenObjs[i] != NULL && !aiObjectIsProbablyDoomed(psChosenObjs[i]))
				{
					// get the weapon stat to see if there is a visible turret to rotate
					psWStats = asWeaponStats + psStructure->asWeaps[i].nStat;

					//if were going to shoot at something move the turret first then fire when locked on
					if (psWStats->pMountGraphic == NULL)//no turret so lock on whatever
					{
						psStructure->asWeaps[i].rot.direction = calcDirection(psStructure->pos.x, psStructure->pos.y, psChosenObjs[i]->pos.x, psChosenObjs[i]->pos.y);
						combFire(&psStructure->asWeaps[i], (BASE_OBJECT *)psStructure, psChosenObjs[i], i);
					}
					else if (actionTargetTurret((BASE_OBJECT*)psStructure, psChosenObjs[i], &psStructure->asWeaps[i]))
					{
						combFire(&psStructure->asWeaps[i], (BASE_OBJECT *)psStructure, psChosenObjs[i], i);
					}
				}
				else
				{
					// realign the turret
					if ((psStructure->asWeaps[i].rot.direction % DEG(90)) != 0 || psStructure->asWeaps[i].rot.pitch != 0)
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
		if (structStandardSensor(psStructure) || structVTOLSensor(psStructure) || objRadarDetector((BASE_OBJECT *)psStructure))
		{
			if (aiChooseSensorTarget((BASE_OBJECT *)psStructure, &psChosenObj))
			{
				objTrace(psStructure->id, "Sensing (%d)", psChosenObj->id);
				if (objRadarDetector((BASE_OBJECT *)psStructure))
				{
					setStructureTarget(psStructure, psChosenObj, 0, ORIGIN_RADAR_DETECTOR);
				}
				else
				{
					setStructureTarget(psStructure, psChosenObj, 0, ORIGIN_SENSOR);
				}
			}
			else
			{
				setStructureTarget(psStructure, NULL, 0, ORIGIN_UNKNOWN);
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
		if ((structCBSensor(psStructure) || structVTOLCBSensor(psStructure) || objRadarDetector((BASE_OBJECT *)psStructure)) &&
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

			// If the droid we're repairing just died, find a new one
			if (psDroid && psDroid->died)
			{
				psDroid = NULL;
				psChosenObj = NULL;
			}
			
			// skip droids that are trying to get to other repair factories
			if (psDroid != NULL
				&& (!orderState(psDroid, DORDER_RTR)
				|| psDroid->psTarget != (BASE_OBJECT *)psStructure))
			{
				psDroid = (DROID *)psChosenObj;
				xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
				ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
				// unless it has orders to repair here, forget about it when it gets out of range
				if (xdiff * xdiff + ydiff * ydiff > (TILE_UNITS*5/2)*(TILE_UNITS*5/2))
				{
					psChosenObj = NULL;
					psDroid = NULL;
					psRepairFac->psObj = NULL;
				}
			}

			// select next droid if none being repaired,
			// or look for a better droid if not repairing one with repair orders
			if (psChosenObj == NULL ||
				(((DROID *)psChosenObj)->order != DORDER_RTR && ((DROID *)psChosenObj)->order != DORDER_RTR_SPECIFIED))
			{
				//FIX ME: (doesn't look like we need this?)
				ASSERT(psRepairFac->psGroup != NULL, "invalid repair facility group pointer");

				// Tries to find most important droid to repair
				// Lower dist = more important
				// mindist contains lowest dist found so far
				mindist = (TILE_UNITS*8)*(TILE_UNITS*8)*3;
				if (psChosenObj)
				{
					// We already have a valid droid to repair, no need to look at
					// droids without a repair order.
					mindist = (TILE_UNITS*8)*(TILE_UNITS*8)*2;
				}
				psRepairFac->droidQueue = 0;
				for (psDroid = apsDroidLists[psStructure->player]; psDroid; psDroid = psDroid->psNext)
				{
					BASE_OBJECT * const psTarget = orderStateObj(psDroid, DORDER_RTR);

					// Highest priority:
					// Take any droid with orders to Return to Repair (DORDER_RTR),
					// or that have been ordered to this repair facility (DORDER_RTR_SPECIFIED),
					// or any "lost" unit with one of those two orders.
					if (((psDroid->order == DORDER_RTR || (psDroid->order == DORDER_RTR_SPECIFIED
					      && (!psTarget || psTarget == (BASE_OBJECT *)psStructure)))
					  && psDroid->action != DACTION_WAITFORREPAIR && psDroid->action != DACTION_MOVETOREPAIRPOINT
					  && psDroid->action != DACTION_WAITDURINGREPAIR)
					  || (psTarget && psTarget == (BASE_OBJECT *)psStructure))
					{
						if (psDroid->body >= psDroid->originalBody)
						{
							objTrace(psStructure->id, "Repair not needed of droid %d", (int)psDroid->id);

							/* set droid points to max */
							psDroid->body = psDroid->originalBody;

							// if completely repaired reset order
							secondarySetState(psDroid, DSO_RETURN_TO_LOC, DSS_NONE);

							if (hasCommander(psDroid))
							{
								// return a droid to it's command group
								DROID	*psCommander = psDroid->psGroup->psCommander;

								orderDroidObj(psDroid, DORDER_GUARD, (BASE_OBJECT *)psCommander, ModeImmediate);
							}
							else if (psRepairFac->psDeliveryPoint != NULL)
							{
								// move the droid out the way
								objTrace(psDroid->id, "Repair not needed - move to delivery point");
								orderDroidLoc(psDroid, DORDER_MOVE,
									psRepairFac->psDeliveryPoint->coords.x,
									psRepairFac->psDeliveryPoint->coords.y, ModeQueue);  // ModeQueue because delivery points are not yet synchronised!
							}
							continue;
						}
						xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
						ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
						currdist = xdiff*xdiff + ydiff*ydiff;
						if (currdist < mindist && currdist < (TILE_UNITS*8)*(TILE_UNITS*8))
						{
							mindist = currdist;
							psChosenObj = (BASE_OBJECT *)psDroid;
						}
						if (psTarget && psTarget == (BASE_OBJECT *)psStructure)
						{
							psRepairFac->droidQueue++;
						}
					}
					// Second highest priority:
					// Help out another nearby repair facility
					else if (psTarget && mindist > (TILE_UNITS*8)*(TILE_UNITS*8)
						   && psTarget != (BASE_OBJECT *)psStructure && psDroid->action == DACTION_WAITFORREPAIR)
					{
						REPAIR_FACILITY *stealFrom = &((STRUCTURE *)psTarget)->pFunctionality->repairFacility;
						// make a wild guess about what is a good distance
						int distLimit = world_coord(stealFrom->droidQueue) * world_coord(stealFrom->droidQueue) * 10;

						xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
						ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
						currdist = xdiff * xdiff + ydiff * ydiff + (TILE_UNITS*8)*(TILE_UNITS*8); // lower priority
						if (currdist < mindist && currdist - (TILE_UNITS*8)*(TILE_UNITS*8) < distLimit)
						{
							mindist = currdist;
							psChosenObj = (BASE_OBJECT *)psDroid;
							psRepairFac->droidQueue++;	// shared queue
							objTrace(psChosenObj->id, "Stolen by another repair facility, currdist=%d, mindist=%d, distLimit=%d", (int)currdist, (int)mindist, distLimit);
						}
					}
					// Lowest priority:
					// Just repair whatever's nearby and needs repairing.
					else if (mindist > (TILE_UNITS*8)*(TILE_UNITS*8)*2 && psDroid->body < psDroid->originalBody)
					{
						xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
						ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
						currdist = xdiff*xdiff + ydiff*ydiff + (TILE_UNITS*8)*(TILE_UNITS*8)*2; // even lower priority
						if (currdist < mindist && currdist < (TILE_UNITS*5/2)*(TILE_UNITS*5/2) + (TILE_UNITS*8)*(TILE_UNITS*8)*2)
						{
							mindist = currdist;
							psChosenObj = (BASE_OBJECT *)psDroid;
						}
					}
				}
				if (!psChosenObj) // Nothing to repair? Repair allied units!
				{
					mindist = (TILE_UNITS*5/2)*(TILE_UNITS*5/2);

					for (i=0; i<MAX_PLAYERS; i++)
					{
						if (aiCheckAlliances(i, psStructure->player) && i != psStructure->player)
						{
							for (psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
							{
								if (psDroid->body < psDroid->originalBody)
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
						}
					}
				}
				psDroid = (DROID *)psChosenObj;
				if (psDroid)
				{
					if (psDroid->order == DORDER_RTR || psDroid->order == DORDER_RTR_SPECIFIED)
					{
						// Hey, droid, it's your turn! Stop what you're doing and get ready to get repaired!
						psDroid->action = DACTION_WAITFORREPAIR;
						psDroid->psTarget = (BASE_OBJECT *)psStructure;
					}
					objTrace(psStructure->id, "Chose to repair droid %d", (int)psDroid->id);
					objTrace(psDroid->id, "Chosen to be repaired by repair structure %d", (int)psStructure->id);
				}
			}

			// send the droid to be repaired
			if (psDroid)
			{
				/* set chosen object */
				psChosenObj = (BASE_OBJECT *)psDroid;

				/* move droid to repair point at rear of facility */
				xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
				ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
				if (psDroid->action == DACTION_WAITFORREPAIR ||
					(psDroid->action == DACTION_WAITDURINGREPAIR
					&& xdiff*xdiff + ydiff*ydiff > (TILE_UNITS*5/2)*(TILE_UNITS*5/2)))
				{
					objTrace(psStructure->id, "Requesting droid %d to come to us", (int)psDroid->id);
					actionDroidObjLoc(psDroid, DACTION_MOVETOREPAIRPOINT,
									  (BASE_OBJECT *) psStructure, psStructure->pos.x, psStructure->pos.y);
				}
				/* reset repair started if we were previously repairing something else */
				if (psRepairFac->psObj != (BASE_OBJECT *)psDroid)
				{
					psRepairFac->timeStarted = ACTION_START_TIME;
					psRepairFac->currentPtsAdded = 0;

					psRepairFac->psObj = (BASE_OBJECT *)psDroid;
				}
			}

			// update repair arm position
			if (psChosenObj)
			{
				actionTargetTurret((BASE_OBJECT*)psStructure, psChosenObj, &psStructure->asWeaps[0]);
			}
			else if ((psStructure->asWeaps[0].rot.direction % DEG(90)) != 0 || psStructure->asWeaps[0].rot.pitch != 0)
			{
				// realign the turret
				actionAlignTurret((BASE_OBJECT *)psStructure, 0);
			}

			break;
		}
		case REF_REARM_PAD:
		{
			REARM_PAD	*psReArmPad = &psStructure->pFunctionality->rearmPad;

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
				if (!psChosenObj) // None available? Try allies.
				{
					for (i=0; i<MAX_PLAYERS; i++)
					{
						if (aiCheckAlliances(i, psStructure->player) && i != psStructure->player)
						{
							for (psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
							{
								// move next droid waiting on ground to rearm pad
								if (vtolReadyToRearm(psDroid, psStructure))
								{
									psChosenObj = (BASE_OBJECT *)psDroid;
									break;
								}
							}
						}
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
					psReArmPad->timeLastUpdated = 0;
				}
				auxStructureBlocking(psStructure);
			}
			else
			{
				auxStructureNonblocking(psStructure);
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
			if (!IsResearchCompleted(pPlayerRes))
			{
				pResearch = (RESEARCH *)pSubject;

				if (psResFacility->timeStarted == ACTION_START_TIME)
				{
					//set the time started
					psResFacility->timeStarted = gameTime;
				}

				ASSERT_OR_RETURN(, gameTime >= psResFacility->timeStarted, "research seems to have started in the future");
				// formula written this way to prevent rounding error
				pointsToAdd = (psResFacility->researchPoints * gameTime) / GAME_TICKS_PER_SEC -
				              (psResFacility->researchPoints * psResFacility->timeStarted) / GAME_TICKS_PER_SEC;
				pointsToAdd = MIN(pointsToAdd, pResearch->researchPoints - pPlayerRes->currentPoints);

				if (pointsToAdd > 0 &&
				    pResearch->researchPoints > 0) // might be a "free" research
				{
					int64_t powerNeeded = ((int64_t)(pResearch->researchPower * pointsToAdd) >> 32) / pResearch->researchPoints;
					pPlayerRes->currentPoints += requestPrecisePowerFor(psStructure->player, powerNeeded, pointsToAdd);
					psResFacility->timeStarted = gameTime;
				}

				//check if Research is complete
				if (pPlayerRes->currentPoints >= pResearch->researchPoints)
				{
					if(bMultiMessages)
					{
						if (myResponsibility(psStructure->player))
						{
							// This message should have no effect if in synch.
							SendResearch(psStructure->player, pSubject->ref - REF_RESEARCH_START, true);
						}
					}

					//store the last topic researched - if its the best
					if (psResFacility->psBestTopic == NULL)
					{
						psResFacility->psBestTopic = psResFacility->psSubject;
					}
					else
					{
						if (pResearch->researchPoints >
							((RESEARCH *)psResFacility->psBestTopic)->researchPoints)
						{
							psResFacility->psBestTopic = psResFacility->psSubject;
						}
					}
					psResFacility->psSubject = NULL;
					intResearchFinished(psStructure);
					researchResult(pSubject->ref - REF_RESEARCH_START, psStructure->player, true, psStructure, true);
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
				if (IsFactoryCommanderGroupFull(psFactory))
				{
					return;
				}

				if(CheckHaltOnMaxUnitsReached(psStructure) == true) {
					return;
				}
			}

			/*must be enough power so subtract that required to build*/
			if (psFactory->timeStarted == ACTION_START_TIME)
			{
				//set the time started
				psFactory->timeStarted = gameTime;
			}

			if (psFactory->timeToBuild > 0)
			{
				int progress;
				int secondsElapsed = (gameTime - psFactory->timeStarted) / GAME_TICKS_PER_SEC;
				int64_t powerNeeded = ((int64_t)(((DROID_TEMPLATE *)pSubject)->powerPoints*secondsElapsed*psFactory->productionOutput) << 32)/((DROID_TEMPLATE*)pSubject)->buildPoints;
				if (secondsElapsed > 0)
				{
					progress = requestPrecisePowerFor(psStructure->player, powerNeeded, secondsElapsed);
					psFactory->timeToBuild -= progress;
					psFactory->timeStarted = psFactory->timeStarted + secondsElapsed*GAME_TICKS_PER_SEC;
				}
			}

			//check for manufacture to be complete
			if ((psFactory->timeToBuild <= 0) &&
				!IsFactoryCommanderGroupFull(psFactory) &&
				!CheckHaltOnMaxUnitsReached(psStructure))
			{
				if (isMission)
				{
					// put it in the mission list
					psDroid = buildMissionDroid((DROID_TEMPLATE *)pSubject,
					                            psStructure->pos.x, psStructure->pos.y,
					                            psStructure->player);
					if (psDroid)
					{
						setDroidBase(psDroid, psStructure);
						bDroidPlaced = true;
					}
				}
				else
				{
					// place it on the map
					bDroidPlaced = structPlaceDroid(psStructure, (DROID_TEMPLATE *)pSubject, &psDroid);
				}

				//reset the start time
				psFactory->timeStarted = ACTION_START_TIME;
				psFactory->psSubject = NULL;

				doNextProduction(psStructure, (DROID_TEMPLATE *)pSubject, ModeImmediate);

				//script callback, must be called after factory was flagged as idle
				if (bDroidPlaced)
				{
					cbNewDroid(psStructure, psDroid);
				}
			}
		}
	}

	/* check base object (for repair / rearm) */
	if ( psChosenObj != NULL )
	{
		if ( structureMode == REF_REPAIR_FACILITY )
		{
			psDroid = (DROID *) psChosenObj;
			ASSERT_OR_RETURN( , psDroid != NULL, "invalid droid pointer");
			psRepairFac = &psStructure->pFunctionality->repairFacility;

			xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
			ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
			if (xdiff * xdiff + ydiff * ydiff <= (TILE_UNITS*5/2)*(TILE_UNITS*5/2))
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
							objTrace(psStructure->id, "Waiting for transporter to land");
							return;
						}
					}

					//don't do anything if the resistance is low in multiplayer
					if (bMultiPlayer)
					{
						if (psStructure->resistance < (SWORD)structureResistance(psStructure->
							pStructureType, psStructure->player))
						{
							objTrace(psStructure->id, "Resistance too low for repair");
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
					// FIXME: duplicate code, make repairing cost power again
					/* do repairing */
					iDt = gameTime - psRepairFac->timeStarted;
					//- this was a bit exponential ...
					pointsToAdd = (iDt * psRepairFac->power / GAME_TICKS_PER_SEC) -
						psRepairFac->currentPtsAdded;
					bFinishAction = false;

					// do some repair
					if (!pointsToAdd)
					{
						// We need to at least repair SOMETHING
						pointsToAdd = 1;
					}
					// just add the points; these are integers, not floats
					psDroid->body += pointsToAdd;
					psRepairFac->currentPtsAdded += pointsToAdd;
				}

				if (psDroid->body >= psDroid->originalBody)
				{
					objTrace(psStructure->id, "Repair complete of droid %d", (int)psDroid->id);

					psRepairFac->psObj = NULL;

					/* set droid points to max */
					psDroid->body = psDroid->originalBody;

					if ((psDroid->order == DORDER_RTR || psDroid->order == DORDER_RTR_SPECIFIED)
					  && psDroid->psTarget == (BASE_OBJECT *)psStructure)
					{
						// if completely repaired reset order
						secondarySetState(psDroid, DSO_RETURN_TO_LOC, DSS_NONE);

						if (hasCommander(psDroid))
						{
							// return a droid to it's command group
							DROID	*psCommander = psDroid->psGroup->psCommander;

							objTrace(psDroid->id, "Repair complete - move to commander");
							orderDroidObj(psDroid, DORDER_GUARD, (BASE_OBJECT *)psCommander, ModeImmediate);
						}
						else if (psRepairFac->psDeliveryPoint != NULL)
						{
							// move the droid out the way
							objTrace(psDroid->id, "Repair complete - move to delivery point");
							orderDroidLoc( psDroid, DORDER_MOVE,
								psRepairFac->psDeliveryPoint->coords.x,
								psRepairFac->psDeliveryPoint->coords.y, ModeQueue);  // ModeQueue because delivery points are not yet synchronised!
						}
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
			REARM_PAD	*psReArmPad = &psStructure->pFunctionality->rearmPad;
			UDWORD pointsAlreadyAdded;

			psDroid = (DROID *)psChosenObj;
			ASSERT_OR_RETURN( , psDroid != NULL, "invalid droid pointer");
			ASSERT_OR_RETURN( , isVtolDroid(psDroid), "invalid droid type");

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
					//set the time started and last updated
					psReArmPad->timeStarted = gameTime;
					psReArmPad->timeLastUpdated = gameTime;
				}

				bFinishAction = false;

				// dont rearm on remote pcs.
				// Huh?! Why not?! if(!bMultiPlayer || myResponsibility(psDroid->player))
				{
					/* do rearming */
					if (psDroid->sMove.iAttackRuns != 0)
					{
						UDWORD      pointsRequired;

						//amount required is a factor of the droids' weight
						pointsRequired = psDroid->weight / REARM_FACTOR;
						//take numWeaps into consideration
						pointsToAdd = psReArmPad->reArmPoints * (gameTime - psReArmPad->timeStarted) /
						    GAME_TICKS_PER_SEC;
						pointsAlreadyAdded = psReArmPad->reArmPoints * (psReArmPad->timeLastUpdated - psReArmPad->timeStarted) /
						    GAME_TICKS_PER_SEC;
						if (pointsToAdd >= pointsRequired)
						{
							// We should be fully loaded by now.
							for (i = 0; i < psDroid->numWeaps; i++)
							{
								// set rearm value to no runs made
								psDroid->sMove.iAttackRuns[i] = 0;
								// reset ammo and lastFired
								psDroid->asWeaps[i].ammo = asWeaponStats[psDroid->asWeaps[i].nStat].numRounds;
								psDroid->asWeaps[i].lastFired = 0;
							}
						}
						else
						{
							for (i = 0; i < psDroid->numWeaps; i++)
							{
								// Make sure it's a rearmable weapon (and so we don't divide by zero)
								if (psDroid->sMove.iAttackRuns[i] > 0 && asWeaponStats[psDroid->asWeaps[i].nStat].numRounds > 0)
								{
									// Do not "simplify" this formula.
									// It is written this way to prevent rounding errors.
									int ammoToAddThisTime =
									    pointsToAdd*getNumAttackRuns(psDroid,i)/pointsRequired -
									    pointsAlreadyAdded*getNumAttackRuns(psDroid,i)/pointsRequired;
									if (ammoToAddThisTime > psDroid->sMove.iAttackRuns[i])
									{
										psDroid->sMove.iAttackRuns[i] = 0;
									}
									else if (ammoToAddThisTime > 0)
									{
										psDroid->sMove.iAttackRuns[i] -= ammoToAddThisTime;
									}
									if (ammoToAddThisTime)
									{
										// reset ammo and lastFired
										psDroid->asWeaps[i].ammo = asWeaponStats[psDroid->asWeaps[i].nStat].numRounds;
										psDroid->asWeaps[i].lastFired = 0;
										break;
									}
								}
							}
						}
					}
				}
				/* do repairing */
				if (psDroid->body < psDroid->originalBody)
				{
					// Do not "simplify" this formula.
					// It is written this way to prevent rounding errors.
					pointsToAdd =  VTOL_REPAIR_FACTOR * (100+asReArmUpgrade[psStructure->player].modifier) * (gameTime -
					               psReArmPad->timeStarted) / (GAME_TICKS_PER_SEC * 100);
					pointsAlreadyAdded =  VTOL_REPAIR_FACTOR * (100+asReArmUpgrade[psStructure->player].modifier) * (psReArmPad->timeLastUpdated -
					               psReArmPad->timeStarted) / (GAME_TICKS_PER_SEC * 100);

					if ((pointsToAdd - pointsAlreadyAdded) > 0)
					{
						psDroid->body += (pointsToAdd - pointsAlreadyAdded);
					}
					if (psDroid->body >= psDroid->originalBody)
					{
						/* set droid points to max */
						psDroid->body = psDroid->originalBody;
					}
				}
				psReArmPad->timeLastUpdated = gameTime;

				//check for fully armed and fully repaired
				if (vtolHappy(psDroid))
				{
					//clear the rearm pad
					psDroid->action = DACTION_NONE;
					bFinishAction = true;
					psReArmPad->psObj = NULL;
					auxClearAll(map_coord(psStructure->pos.x), map_coord(psStructure->pos.y), AUXBITS_ANY_BUILDING | AUXBITS_OUR_BUILDING);
				}
			}
		}
	}
}


/** Decides whether a structure should emit smoke when it's damaged */
static BOOL canSmoke(STRUCTURE *psStruct)
{
	if (psStruct->pStructureType->type == REF_WALL || psStruct->pStructureType->type == REF_WALLCORNER
	    || psStruct->status == SS_BEING_BUILT || psStruct->pStructureType->type == REF_GATE)
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

void _syncDebugStructure(const char *function, STRUCTURE *psStruct, char ch)
{
	int ref = 0;
	char const *refStr = "";

	// Print what the structure is producing, too.
	switch (psStruct->pStructureType->type)
	{
		case REF_RESEARCH:
			if (psStruct->pFunctionality->researchFacility.psSubject != NULL)
			{
				ref = psStruct->pFunctionality->researchFacility.psSubject->ref;
				refStr = ",research";
			}
			break;
		case REF_FACTORY:
		case REF_CYBORG_FACTORY:
		case REF_VTOL_FACTORY:
			if (psStruct->pFunctionality->factory.psSubject != NULL)
			{
				ref = psStruct->pFunctionality->factory.psSubject->ref;
				refStr = ",production";
			}
			break;
		default:
			break;
	}

	_syncDebug(function, "%c structure%d = p%d;pos(%d,%d,%d),stat%d,type%d%s%.0d,bld%d,pwr%d,bp%d, power = %"PRId64"", ch,
	          psStruct->id,

	          psStruct->player,
	          psStruct->pos.x, psStruct->pos.y, psStruct->pos.z,
	          psStruct->status,
	          psStruct->pStructureType->type, refStr, ref,
	          psStruct->currentBuildPts,
	          psStruct->currentPowerAccrued,
	          psStruct->body,

	          getPrecisePower(psStruct->player));
}

/* The main update routine for all Structures */
void structureUpdate(STRUCTURE *psBuilding, bool mission)
{
	UDWORD widthScatter,breadthScatter;
	UDWORD emissionInterval, iPointsToAdd, iPointsRequired;
	Vector3i dv;
	int i;

	syncDebugStructure(psBuilding, '<');

	if (psBuilding->pStructureType->type == REF_GATE)
	{
		if (psBuilding->state == SAS_OPEN && psBuilding->lastStateTime + SAS_STAY_OPEN_TIME < gameTime)
		{
			bool		found = false;
			BASE_OBJECT	*psObj;

			gridStartIterate(psBuilding->pos.x, psBuilding->pos.y, TILE_UNITS);
			while (!found && (psObj = gridIterate()))
			{
				found = (psObj->type == OBJ_DROID);
			}

			if (!found)	// no droids on our tile, safe to close
			{
				psBuilding->state = SAS_CLOSING;
				auxStructureBlocking(psBuilding); // closed
				psBuilding->lastStateTime = gameTime;	// reset timer
			}
		}
		else if (psBuilding->state == SAS_OPENING && psBuilding->lastStateTime + SAS_OPEN_SPEED < gameTime)
		{
			psBuilding->state = SAS_OPEN;
			auxStructureNonblocking(psBuilding); // opened
			psBuilding->lastStateTime = gameTime;	// reset timer
		}
		else if (psBuilding->state == SAS_CLOSING && psBuilding->lastStateTime + SAS_OPEN_SPEED < gameTime)
		{
			psBuilding->state = SAS_NORMAL;
			psBuilding->lastStateTime = gameTime;	// reset timer
		}
	}

	// Remove invalid targets. This must be done each frame.
	for (i = 0; i < STRUCT_MAXWEAPS; i++)
	{
		if (psBuilding->psTarget[i] && psBuilding->psTarget[i]->died)
		{
			setStructureTarget(psBuilding, NULL, i, ORIGIN_UNKNOWN);
		}
	}

	//update the manufacture/research of the building once complete
	if (psBuilding->status == SS_BUILT)
	{
		aiUpdateStructure(psBuilding, mission);
	}

	if(psBuilding->status!=SS_BUILT)
	{
		if(psBuilding->selected)
		{
			psBuilding->selected = false;
		}
	}

	/* Only add smoke if they're visible and they can 'burn' */
	if (!mission && psBuilding->visible[selectedPlayer] && canSmoke(psBuilding))
	{
		const int32_t damage = getStructureDamage(psBuilding);

		// Is there any damage?
		if (damage > 0.)
		{
			emissionInterval = CalcStructureSmokeInterval(damage/65536.f);
			if(gameTime > (psBuilding->lastEmission + emissionInterval))
			{
				widthScatter   = getStructureWidth(psBuilding)   * TILE_UNITS/2/3;
				breadthScatter = getStructureBreadth(psBuilding) * TILE_UNITS/2/3;
				dv.x = psBuilding->pos.x + widthScatter - rand()%(2*widthScatter);
				dv.z = psBuilding->pos.y + breadthScatter - rand()%(2*breadthScatter);
				dv.y = psBuilding->pos.z;
				dv.y += (psBuilding->sDisplay.imd->max.y * 3) / 4;
				addEffect(&dv,EFFECT_SMOKE,SMOKE_TYPE_DRIFTING_HIGH,false,NULL,0);
				psBuilding->lastEmission = gameTime;
			}
		}
	}

	if (!mission)
	{
		processVisibilityLevel((BASE_OBJECT*)psBuilding);
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
			if(bMultiPlayer && ONEINTEN && !mission)
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

	syncDebugStructure(psBuilding, '>');

	CHECK_STRUCTURE(psBuilding);
}


/* Release all resources associated with a structure */
void structureRelease(STRUCTURE *psBuilding)
{
	/* remove animation if present */
	if (psBuilding->psCurAnim != NULL)
	{
		animObj_Remove(psBuilding->psCurAnim, psBuilding->psCurAnim->psAnim->uwID);
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

				debug(LOG_NEVER, "adding %s (%x)", psBuilding->pName, apStructTypeLists[selectedPlayer][inc]);
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


typedef enum
{
	PACKABILITY_EMPTY = 0, PACKABILITY_DEFENSE = 1, PACKABILITY_NORMAL = 2, PACKABILITY_REPAIR = 3
} STRUCTURE_PACKABILITY;

static inline bool canPack(STRUCTURE_PACKABILITY a, STRUCTURE_PACKABILITY b)
{
	return (int)a + (int)b <= 3;  // Defense can be put next to anything except repair facilities, normal base structures can't be put next to each other, and anything goes next to empty tiles.
}

static STRUCTURE_PACKABILITY baseStructureTypePackability(STRUCTURE_TYPE type)
{
	switch (type)
	{
		case REF_DEFENSE:
		case REF_WALL:
		case REF_WALLCORNER:
		case REF_GATE:
		case REF_REARM_PAD:
		case REF_MISSILE_SILO:
			return PACKABILITY_DEFENSE;
		default:
			return PACKABILITY_NORMAL;
		case REF_REPAIR_FACILITY:
			return PACKABILITY_REPAIR;
	}
}

static STRUCTURE_PACKABILITY baseObjectPackability(BASE_OBJECT *psObject)
{
	if (psObject == NULL)
	{
		return PACKABILITY_EMPTY;
	}
	switch (psObject->type)
	{
		case OBJ_STRUCTURE: return baseStructureTypePackability(((STRUCTURE *)psObject)->pStructureType->type);
		case OBJ_FEATURE:   return ((FEATURE *)psObject)->psStats->subType == FEAT_OIL_RESOURCE? PACKABILITY_NORMAL : PACKABILITY_EMPTY;
		default:            return PACKABILITY_EMPTY;
	}
}

/* checks that the location is a valid one to build on and sets the outline colour
x and y in tile-coords*/
bool validLocation(BASE_STATS *psStats, unsigned x, unsigned y, uint16_t direction, unsigned player, bool bCheckBuildQueue)
{
	STRUCTURE			*psStruct;
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
		unsigned sWidth   = getStructureStatsWidth((STRUCTURE_STATS *)psStats, direction);
		unsigned sBreadth = getStructureStatsBreadth((STRUCTURE_STATS *)psStats, direction);
		site.xTL = (UWORD)x;
		site.yTL = (UWORD)y;
		site.xBR = (UWORD)(x + sWidth - 1);
		site.yBR = (UWORD)(y + sBreadth - 1);

		//if we're dragging the wall/defense we need to check along the current dragged size
		if (wallDrag.status != DRAG_INACTIVE
			&& (psBuilding->type == REF_WALL || psBuilding->type == REF_DEFENSE || psBuilding->type == REF_REARM_PAD ||  psBuilding->type == REF_GATE)
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

	// Check to make sure we are not on/close to water...
	for (j=0; j < 2; j++)
	{
		for (i=0; i < 2; i++)
		{
			if (map_TileHeight(x+i, y+j) < 0)
			{
				valid = false;
				goto failed;
			}
		}
	}


	for (i = site.xTL; i <= site.xBR && valid; i++)
	{
		for (j = site.yTL; j <= site.yBR && valid; j++)
		{
			// Can't build outside of scroll limits.
			if( ((SDWORD)i < scrollMinX+2) || ((SDWORD)i > scrollMaxX-5) ||
				((SDWORD)j < scrollMinY+2) || ((SDWORD)j > scrollMaxY-5))
			{
				valid = false;
				goto failed;
			}

			// check i or j off map.
			if(i<=2 || j<=2 || i>=(SDWORD)mapWidth-2 || j>=(SDWORD)mapHeight-2)
			{
				valid = false;
				goto failed;
			}
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

	if (bCheckBuildQueue)
	{
		// cant place on top of a delivery point...
		for (psCurrFlag = apsFlagPosLists[selectedPlayer]; psCurrFlag; psCurrFlag = psCurrFlag->psNext)
		{
			ASSERT_OR_RETURN(false, psCurrFlag->coords.x != ~0, "flag has invalid position");
			i = map_coord(psCurrFlag->coords.x);
			j = map_coord(psCurrFlag->coords.y);

			if (i >= site.xTL && i <= site.xBR &&
				j >= site.yTL && j <= site.yBR)
			{
				valid = false;
				goto failed;
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
				ASSERT(!"invalid structure type", "Bad structure type %u", psBuilding->type);
				break;
			case REF_HQ:
			case REF_FACTORY:
			case REF_LAB:
			case REF_RESEARCH:
			case REF_POWER_GEN:
			case REF_WALL:
			case REF_WALLCORNER:
			case REF_GATE:
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
					STRUCTURE_PACKABILITY packThis = baseStructureTypePackability(psBuilding->type);

					// skirmish AIs don't build nondefensives next to anything. (route hack)
					if (packThis == PACKABILITY_NORMAL && bMultiPlayer && game.type == SKIRMISH && !isHumanPlayer(player))
					{
						packThis = PACKABILITY_REPAIR;
					}

					/* need to check there is one tile between buildings */
					for (j = site.yTL - 1; j <= site.yBR + 1 && valid; j++)
					{
						for (i = site.xTL - 1; i <= site.xBR + 1 && valid; i++)
						{
							//skip the actual area the structure will cover
							if (i < site.xTL || i > site.xBR || j < site.yTL || j > site.yBR)
							{
								STRUCTURE_PACKABILITY packObj = baseObjectPackability(mapTile(i, j)->psObject);

								valid = valid && canPack(packThis, packObj);
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
							if (TileIsOccupied(psTile))
							{
								if (TileHasWall(psTile) && (psBuilding->type == REF_DEFENSE || psBuilding->type == REF_WALL))
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
				if(TileHasStructure(mapTile(x,y)))
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
				if(TileHasStructure(mapTile(x,y)))
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
				if(TileHasStructure(mapTile(x,y)))
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
				if(TileHasFeature(mapTile(x,y)))
				{
					FEATURE	*psFeat = getTileFeature(x, y);

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
			if (!(psBuilding->type == REF_DEFENSE || psBuilding->type == REF_MISSILE_SILO || psBuilding->type == REF_REARM_PAD))
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
						for (order = psDroid->listPendingBegin; order < psDroid->listPendingEnd; order++)
						{
							if (psDroid->asOrderList[order].order == DORDER_BUILD)
							{
								STRUCTURE_STATS *orderTarget = (STRUCTURE_STATS *)psDroid->asOrderList[order].psOrderTarget;

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
								    && (orderTarget->type == REF_WALL || orderTarget->type == REF_WALLCORNER))
								{
									validCombi = true;
								}
								//don't bother checking if valid combination of building types
								if (!validCombi)
								{
									/*need to check there is one tile between buildings*/
									//check if any corner is within the build site
									STRUCTURE_STATS *target = (STRUCTURE_STATS *)psDroid->asOrderList[order].psOrderTarget;
									uint16_t dir = psDroid->asOrderList[order].direction;
									size = getStructureStatsWidth(target, dir);
									left = map_coord(psDroid->asOrderList[order].x) - size/2;
									right = left + size;
									size = getStructureStatsBreadth(target, dir);
									up = map_coord(psDroid->asOrderList[order].y) - size/2;
									down = up + size;
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
	else if (psStats->ref >= REF_TEMPLATE_START
			 && psStats->ref < REF_TEMPLATE_START + REF_RANGE)
	{
		DROID_TEMPLATE	*psTemplate = (DROID_TEMPLATE *)psStats;
		PROPULSION_STATS *psPropStats = asPropulsionStats + psTemplate->asParts[COMP_PROPULSION];

		valid = !fpathBlockingTile(x, y, psPropStats->propulsionType);
	}
	else
	{
		// not positioning a structure or droid, ie positioning a feature
		valid = !fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED);
	}

failed:  // Succeeded if got here without jumping.
	// Only set the hilight colour if it's the selected player.
	if (player == selectedPlayer)
	{
		outlineTile = true;
	}

	return valid;
}

/*
for a new structure, find a location along an edge which the droid can get
to and return this as the destination for the droid.
*/
BOOL getDroidDestination(BASE_STATS *psStats, UDWORD structX,
	UDWORD structY, UDWORD *pDroidX, UDWORD *pDroidY)
{
	int32_t                         start;
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
	ASSERT_OR_RETURN(false, width + breadth > 0, "Weird droid destination");

	//get a random starting place 0=top left
	start = gameRand((width + breadth) * 2);

	//search in a clockwise direction around the structure from the starting point
	// TODO Fix 4x code duplication.
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
		if( x+side < mapWidth && y < mapHeight && !TileIsOccupied(mapTile(x+side,y)) )
		{
			*pDroidX = world_coord(x + side);
			*pDroidY = world_coord(y);

			ASSERT_OR_RETURN(false, worldOnMap(*pDroidX,*pDroidY), "Insane droid position generated at width (%u, %u)", *pDroidX, *pDroidY);

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
		if(y+side < mapHeight && x < mapWidth && !TileIsOccupied(mapTile(x,y+side)) )
		{
			*pDroidX = world_coord(x);
			*pDroidY = world_coord(y + side);

			ASSERT_OR_RETURN(false, worldOnMap(*pDroidX,*pDroidY), "Insane droid position generated at length (%u, %u)", *pDroidX, *pDroidY);

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
	unsigned sWidth   = getStructureWidth(psStruct);
	unsigned sBreadth = getStructureBreadth(psStruct);

	auxStructureNonblocking(psStruct);

	/* set tiles drawing */
	mapX = map_coord(psStruct->pos.x) - sWidth/2;
	mapY = map_coord(psStruct->pos.y) - sBreadth/2;
	for (i = 0; i < sWidth; i++)
	{
		for (j = 0; j < sBreadth; j++)
		{
			psTile = mapTile(mapX+i, mapY+j);
			psTile->psObject = NULL;
			auxClearBlocking(mapX + i, mapY + j, AIR_BLOCKED);
		}
	}
}

// remove a structure from a game without any visible effects
// bDestroy = true if the object is to be destroyed
// (for example used to change the type of wall at a location)
BOOL removeStruct(STRUCTURE *psDel, BOOL bDestroy)
{
	BOOL		resourceFound = false;
	uint32_t        mask;
	FACTORY		*psFactory;
	SDWORD		cluster;
	FLAG_POSITION	*psAssemblyPoint=NULL;

	ASSERT_OR_RETURN(false, psDel != NULL, "Invalid structure pointer");

	if (bDestroy)
	{
		removeStructFromMap(psDel);
	}

	if (bDestroy)
	{
		//if the structure is a resource extractor, need to put the resource back in the map
		/*ONLY IF ANY POWER LEFT - HACK HACK HACK!!!! OIL POOLS NEED TO KNOW
		HOW MUCH IS THERE && NOT RES EXTRACTORS */
		if (psDel->pStructureType->type == REF_RESOURCE_EXTRACTOR)
		{
			FEATURE *psOil = buildFeature(oilResFeature, psDel->pos.x, psDel->pos.y, false);
			memcpy(psOil->seenThisTick, psDel->visible, sizeof(psOil->seenThisTick));
			resourceFound = true;
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
			cancelResearch(psDel, ModeImmediate);
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
		cancelProduction(psDel, ModeImmediate);

		psAssemblyPoint = psFactory->psAssemblyPoint;
	}
	else if (psDel->pStructureType->type == REF_REPAIR_FACILITY)
	{
		psAssemblyPoint = psDel->pFunctionality->repairFacility.psDeliveryPoint;
	}

	if (psAssemblyPoint != NULL)
	{
		mask = 1 << psAssemblyPoint->factoryInc;
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

	// remove the structure from the cluster
	cluster = psDel->cluster;
	clustRemoveObject((BASE_OBJECT *)psDel);

	if (bDestroy)
	{
		killStruct(psDel);
	}

	/* remove animation if present */
	if (psDel->psCurAnim != NULL)
	{
		animObj_Remove(psDel->psCurAnim, psDel->psCurAnim->psAnim->uwID);
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
	UDWORD			widthScatter,breadthScatter,heightScatter;
	BOOL			resourceFound = false;
	MAPTILE			*psTile;
	bool                    bMinor;

	const unsigned          burnDurationWall    =  1000;
	const unsigned          burnDurationOilWell = 60000;
	const unsigned          burnDurationOther   = 10000;

	CHECK_STRUCTURE(psDel);

	if (bMultiMessages)
	{
		// Every player should be sending this message at once, and ignoring it later.
		SendDestroyStructure(psDel);
	}

	/* Firstly, are we dealing with a wall section */
	bMinor = psDel->pStructureType->type == REF_WALL || psDel->pStructureType->type == REF_WALLCORNER;

//---------------------------------------
	/* Only add if visible */
	if(psDel->visible[selectedPlayer])
	{
		Vector3i pos;
		int      i;

//---------------------------------------  Do we add immediate explosions?
		/* Set off some explosions, but not for walls */
		/* First Explosions */
		widthScatter = TILE_UNITS;
		breadthScatter = TILE_UNITS;
		heightScatter = TILE_UNITS;
		for (i = 0; i < (bMinor ? 2 : 4); ++i)  // only add two for walls - gets crazy otherwise
		{
			pos.x = psDel->pos.x + widthScatter - rand()%(2*widthScatter);
			pos.z = psDel->pos.y + breadthScatter - rand()%(2*breadthScatter);
			pos.y = psDel->pos.z + 32 + rand()%heightScatter;
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_MEDIUM,false,NULL,0);
		}

		/* Get coordinates for everybody! */
		pos.x = psDel->pos.x;
		pos.z = psDel->pos.y;  // z = y [sic] intentional
		pos.y = map_Height(pos.x, pos.z);

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
		if (bMinor)  // walls
		{
			/* Give a duration */
			effectGiveAuxVarSec(burnDurationWall);
			/* Normal fire - no smoke */
			addEffect(&pos,EFFECT_FIRE,FIRE_TYPE_LOCALISED,false,NULL,0);
		}
		else if(psDel->pStructureType->type == REF_RESOURCE_EXTRACTOR) // oil resources
		{
			/* Oil resources burn AND puff out smoke AND for longer*/
			effectGiveAuxVarSec(burnDurationOilWell);
			addEffect(&pos,EFFECT_FIRE,FIRE_TYPE_SMOKY,false,NULL,0);
		}
		else	// everything else
		{
			/* Give a duration */
			effectGiveAuxVarSec(burnDurationOther);
			addEffect(&pos,EFFECT_FIRE,FIRE_TYPE_LOCALISED,false,NULL,0);
		}

//--------------------------------------- Do we add a destruction seq, and if so, which?
		/* Power stations have their own desctruction sequence */
		if(psDel->pStructureType->type == REF_POWER_GEN)
		{
			addEffect(&pos,EFFECT_DESTRUCTION,DESTRUCTION_TYPE_POWER_STATION,false,NULL,0);
			pos.y += SHOCK_WAVE_HEIGHT;
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SHOCKWAVE,false,NULL,0);
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

	// Actually set the tiles on fire - even if the effect is not visible.
	if (bMinor)  // walls
	{
		tileSetFire(psDel->pos.x, psDel->pos.y, burnDurationWall);
	}
	else if(psDel->pStructureType->type == REF_RESOURCE_EXTRACTOR)  // oil resources
	{
		/* Oil resources burn AND puff out smoke AND for longer*/
		tileSetFire(psDel->pos.x, psDel->pos.y, burnDurationOilWell);
	}
	else  // everything else
	{
		tileSetFire(psDel->pos.x, psDel->pos.y, burnDurationOther);
	}

	// Power generators give back their power when destroyed.
	if (psDel->pStructureType->type == REF_POWER_GEN)
	{
		// Give some power back to the player, whether or not the power generator is visible.
		addPower(psDel->player, structPowerToBuild(psDel));
		// If it had a module attached, need to add the power for the base struct as well, whether or not the power generator is visible.
		if (psDel->pFunctionality->powerGenerator.capacity)
		{
			addPower(psDel->player, psDel->pStructureType->powerToBuild);
		}
	}

	resourceFound = removeStruct(psDel, true);

	//once a struct is destroyed - it leaves a wrecked struct FEATURE in its place
	// Wall's don't leave wrecked features
	if(psDel->visible[selectedPlayer])
	{
		if (!resourceFound && !(psDel->pStructureType->type == REF_WALL) &&
			!(psDel->pStructureType->type == REF_WALLCORNER))
		{
			unsigned sWidth   = getStructureWidth(psDel);
			unsigned sBreadth = getStructureBreadth(psDel);
			mapX = map_coord(psDel->pos.x - sWidth * TILE_UNITS / 2);
			mapY = map_coord(psDel->pos.y - sBreadth * TILE_UNITS / 2);
			for (width = 0; width < sWidth; width++)
			{
				for (breadth = 0; breadth < sBreadth; breadth++)
				{
					psTile = mapTile(mapX+width,mapY+breadth);
					if(TEST_TILE_VISIBLE(selectedPlayer,psTile))
					{
						psTile->illumination /= 2;
					}
				}
			}
		}
	}

	/* remove animation if present */
	if (psDel->psCurAnim != NULL)
	{
		animObj_Remove(psDel->psCurAnim, psDel->psCurAnim->psAnim->uwID);
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
SWORD buildFoundation(STRUCTURE *psStruct, UDWORD x, UDWORD y)
{
	UDWORD	width, breadth;
	UDWORD	startX, startY;
	SWORD	height,foundationMin, foundationMax;
	unsigned sWidth   = getStructureWidth(psStruct);
	unsigned sBreadth = getStructureBreadth(psStruct);

	startX = map_coord(x) - sWidth/2;
	startY = map_coord(y) - sBreadth/2;

	//check the terrain is the correct type return -1 if not

	//shouldn't need to do this but doesn't take long hey?!
	/*check if there is a structure next to the new one - return the height of the
	structure if found*/
	for (breadth = 0; breadth <= sBreadth; breadth++)
	{
		for (width = 0; width <= sWidth; width++)
		{
			if(TileHasStructure(mapTile(startX+width,startY+breadth)))
			{
				return map_TileHeight(startX+width, startY+breadth);
			}
		}
	}

	//may also have to check that overlapping terrain can be set to the average height
	//eg water - don't want it to 'flow' into the structure if this effect is coded!

	startX = map_coord(x) - sWidth/2;
	startY = map_coord(y) - sBreadth/2;

	//initialise the starting values so they get set in loop
	foundationMin = TILE_MAX_HEIGHT;
	foundationMax = TILE_MIN_HEIGHT;

	for (breadth = 0; breadth <= sBreadth; breadth++)
	{
		for (width = 0; width <= sWidth; width++)
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
int32_t getStructStatFromName(char const *pName)
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

	ASSERT_OR_RETURN(false, structInc < numStructureStats, "Invalid structure inc");

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
	if (!validLocation((BASE_STATS *)&sStats, *pX, *pY, 0, player, false))
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
						if(validLocation((BASE_STATS *)&sStats, i, j, 0, player, false))
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
	ASSERT(!"unable to find a valid location", "unable to find a valid location!");
}


/*sets the point new droids go to - x/y in world coords for a Factory
bCheck is set to true for initial placement of the Assembly Point*/
void setAssemblyPoint(FLAG_POSITION *psAssemblyPoint, UDWORD x, UDWORD y,
					UDWORD player, BOOL bCheck)
{
	ASSERT_OR_RETURN( , psAssemblyPoint != NULL, "invalid AssemblyPoint pointer");

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
	psAssemblyPoint->coords.z = map_Height(x, y) + ASSEMBLY_POINT_Z_PADDING;
}


/*sets the factory Inc for the Assembly Point*/
void setFlagPositionInc(FUNCTIONALITY* pFunctionality, UDWORD player, UBYTE factoryType)
{
	UBYTE			inc;
	uint32_t                mask = 1;
	FACTORY			*psFactory;
	REPAIR_FACILITY *psRepair;

	ASSERT_OR_RETURN( , player < MAX_PLAYERS, "invalid player number");
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
		const char* pType;

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
	ASSERT_OR_RETURN(NULL, g_psStatDestroyStruct != NULL , "Demolish stat not initialised");
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
	if (state)
	{
		satuplinkbits |= (1 << player);
	}
	else
	{
		satuplinkbits &= ~(1 << player);
	}
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
bool calcStructureMuzzleLocation(STRUCTURE *psStructure, Vector3i *muzzle, int weapon_slot)
{
	iIMDShape *psShape = psStructure->pStructureType->pIMD;

	CHECK_STRUCTURE(psStructure);

	if(psShape && psShape->nconnectors)
	{
		Vector3i barrel = {0, 0, 0};
		unsigned int nWeaponStat = psStructure->asWeaps[weapon_slot].nStat;
		iIMDShape *psWeaponImd = 0, *psMountImd = 0;

		if (nWeaponStat)
		{
			psWeaponImd = asWeaponStats[nWeaponStat].pIMD;
			psMountImd = asWeaponStats[nWeaponStat].pMountGraphic;
		}

		pie_MatBegin();

		pie_TRANSLATE(psStructure->pos.x, -psStructure->pos.z, psStructure->pos.y);

		//matrix = the center of droid
		pie_MatRotY(psStructure->rot.direction);
		pie_MatRotX(psStructure->rot.pitch);
		pie_MatRotZ(-psStructure->rot.roll);
		pie_TRANSLATE( psShape->connectors[weapon_slot].x, -psShape->connectors[weapon_slot].z,
					-psShape->connectors[weapon_slot].y);//note y and z flipped

		//matrix = the weapon[slot] mount on the body
		pie_MatRotY(psStructure->asWeaps[weapon_slot].rot.direction);  // +ve anticlockwise

		// process turret mount
		if (psMountImd && psMountImd->nconnectors)
		{
			pie_TRANSLATE(psMountImd->connectors->x, -psMountImd->connectors->z, -psMountImd->connectors->y);
		}

		//matrix = the turret connector for the gun
		pie_MatRotX(psStructure->asWeaps[weapon_slot].rot.pitch);      // +ve up

		//process the gun
		if (psWeaponImd && psWeaponImd->nconnectors)
		{
			unsigned int connector_num = 0;

			// which barrel is firing if model have multiple muzzle connectors?
			if (psStructure->asWeaps[weapon_slot].shotsFired && (psWeaponImd->nconnectors > 1))
			{
				// shoot first, draw later - substract one shot to get correct results
				connector_num = (psStructure->asWeaps[weapon_slot].shotsFired - 1) % (psWeaponImd->nconnectors);
			}
			
			barrel = Vector3i_Init(psWeaponImd->connectors[connector_num].x,
									-psWeaponImd->connectors[connector_num].y,
									-psWeaponImd->connectors[connector_num].z);
		}

		pie_RotateTranslate3i(&barrel, muzzle);
		muzzle->z = -muzzle->z;

		pie_MatEnd();
	}
	else
	{
		*muzzle = Vector3i_Init(psStructure->pos.x, psStructure->pos.y, psStructure->pos.z + psStructure->sDisplay.imd->max.y);
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
		ASSERT(!"invalid structure type", "invalid structure type");
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
			psResExtractor->active = true;
		}
	}

	psResExtractor = NULL;
	//each Power Gen can cope with 4 Extractors now - 9/6/98 AB
	//check capacity against number of filled slots
	if (slot < NUM_POWER_MODULES)
	{
		for (psCurr = apsExtractorLists[psBuilding->player]; psCurr != NULL; psCurr = psCurr->psNextFunc)
		{
				psResExtractor = &psCurr->pFunctionality->resourceExtractor;

				//check not connected and power left and built!
				if (!psResExtractor->active
				 && psCurr->status == SS_BUILT)
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
		ASSERT(!"invalid structure type", "invalid structure type");
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
		ASSERT(!"invalid structure type", "invalid structure type");
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
		ASSERT(!"invalid structure type", "Invalid structure type");
		return;
	}

	//tell associated Power Gen
	if (psRelease->pFunctionality->resourceExtractor.psPowerGen)
	{
		informPowerGen(psRelease);
	}

	psRelease->pFunctionality->resourceExtractor.psPowerGen = NULL;

	//there may be spare resource extractors
	for (psCurr = apsExtractorLists[psRelease->player]; psCurr != NULL; psCurr = psCurr->psNextFunc)
	{
		//check not connected and power left and built!
		if (psCurr != psRelease && !psCurr->pFunctionality->resourceExtractor.active && psCurr->status == SS_BUILT)
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
		ASSERT(!"invalid structure type", "Invalid structure type");
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

	visTilesUpdate((BASE_OBJECT *)psBuilding);

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
			//this deals with research facilities that are upgraded whilst mid-research
			releaseResearch(psBuilding, ModeImmediate);
			break;
		case REF_FACTORY:
		case REF_CYBORG_FACTORY:
		case REF_VTOL_FACTORY:
			//this deals with factories that are upgraded whilst mid-production
			releaseProduction(psBuilding, ModeImmediate);
			break;
		case REF_SAT_UPLINK:
			revealAll(psBuilding->player);
		default:
			//do nothing
			break;
	}
}


/*for a given structure, return a pointer to its module stat */
STRUCTURE_STATS* getModuleStat(const STRUCTURE* psStruct)
{
	ASSERT_OR_RETURN(NULL, psStruct != NULL, "Invalid structure pointer");

	switch (psStruct->pStructureType->type)
	{
		case REF_POWER_GEN:
			return &asStructureStats[powerModuleStat];

		case REF_FACTORY:
		case REF_VTOL_FACTORY:
			return &asStructureStats[factoryModuleStat];

		case REF_RESEARCH:
			return &asStructureStats[researchModuleStat];

		default:
			//no other structures can have modules attached
			return NULL;
	}
}

/**
 * Count the artillery and VTOL droids assigned to a structure.
 */
static unsigned int countAssignedDroids(const STRUCTURE* psStructure)
{
	const DROID* psCurr;
	unsigned int num;

	CHECK_STRUCTURE(psStructure);

	// For non-debug builds
	if (psStructure == NULL)
		return 0;

	num = 0;
	for (psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr->psTarget
		 && psCurr->psTarget->id == psStructure->id
		 && psCurr->player == psStructure->player)
		{
			const MOVEMENT_MODEL weapontype = asWeaponStats[psCurr->asWeaps[0].nStat].movementModel;

			if (weapontype == MM_INDIRECT
			 || weapontype == MM_HOMINGINDIRECT
			 || isVtolDroid(psCurr))
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

	ASSERT_OR_RETURN( , psStructure != NULL, "Invalid Structure pointer");

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
#ifdef DEBUG
		if (getDebugMappingStatus())
		{
			CONPRINTF(ConsoleString, (ConsoleString, "%s - %d Units assigned - ID %d - armour %d|%d - sensor range %hu power %hu - ECM %u - born %u - depth %.02f",
				getStatName(psStructure->pStructureType), countAssignedDroids(psStructure),
				psStructure->id, psStructure->armour[0][WC_KINETIC], psStructure->armour[0][WC_HEAT],
					structSensorRange(psStructure), structSensorPower(psStructure), structConcealment(psStructure), psStructure->born, psStructure->foundationDepth));
		} else
#endif
		if (psStructure->pStructureType->pSensor != NULL
			   && (psStructure->pStructureType->pSensor->type == STANDARD_SENSOR
			   || psStructure->pStructureType->pSensor->type == INDIRECT_CB_SENSOR
			   || psStructure->pStructureType->pSensor->type == VTOL_INTERCEPT_SENSOR
			   || psStructure->pStructureType->pSensor->type == VTOL_CB_SENSOR
			   || psStructure->pStructureType->pSensor->type == SUPER_SENSOR
			   || psStructure->pStructureType->pSensor->type == RADAR_DETECTOR_SENSOR)
			   && psStructure->pStructureType->pSensor->location == LOC_TURRET)
		{
			unsigned int assigned_droids = countAssignedDroids(psStructure);

			CONPRINTF(ConsoleString, (ConsoleString, ngettext("%s - %u Unit assigned", "%s - %u Units assigned", assigned_droids),
				getStatName(psStructure->pStructureType), assigned_droids));
		}
		else
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("%s - Damage %3.0f%%"),
									  getStatName(psStructure->pStructureType), getStructureDamage(psStructure) * (100.f/65536.f)));
		}
		break;
	case REF_REPAIR_FACILITY:
#ifdef DEBUG
		if (getDebugMappingStatus())
		{
			CONPRINTF(ConsoleString,(ConsoleString, "%s - Unique ID %d - Queue %d",
					  getStatName(psStructure->pStructureType), psStructure->id, psStructure->pFunctionality->repairFacility.droidQueue));
		}
		else
#endif
		{
			CONPRINTF(ConsoleString, (ConsoleString, "%s", getStatName(psStructure->pStructureType)));
		}
		break;
	case REF_RESOURCE_EXTRACTOR:
#ifdef DEBUG
		if (getDebugMappingStatus())
		{
			CONPRINTF(ConsoleString,(ConsoleString, "%s - Unique ID %d - %s",
			          getStatName(psStructure->pStructureType), psStructure->id, (auxTile(map_coord(psStructure->pos.x), map_coord(psStructure->pos.y), selectedPlayer) & AUXBITS_DANGER) ? "danger" : "safe"));
		}
		else
#endif
		{
			CONPRINTF(ConsoleString, (ConsoleString, "%s", getStatName(psStructure->pStructureType)));
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
			CONPRINTF(ConsoleString, (ConsoleString, "%s -  Connected %u of %u - Unique ID %u - Multiplier: %u",
					  getStatName(psStructure->pStructureType), numConnected, NUM_POWER_MODULES,
					  psStructure->id, psPowerGen->multiplier));
		}
		else
#endif
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("%s - Connected %u of %u"),
					  getStatName(psStructure->pStructureType), numConnected, NUM_POWER_MODULES));
		}
		break;
	case REF_CYBORG_FACTORY:
	case REF_VTOL_FACTORY:
	case REF_FACTORY:
#ifdef DEBUG
		if (getDebugMappingStatus())
		{
			CONPRINTF(ConsoleString, (ConsoleString, "%s - Damage % 3.2f%% - Unique ID %u - Production Output: %u - TimeToBuild: %u",
					  getStatName(psStructure->pStructureType), getStructureDamage(psStructure) * (100.f/65536.f), psStructure->id,
					  psStructure->pFunctionality->factory.productionOutput,
					  psStructure->pFunctionality->factory.timeToBuild));
		}
		else
#endif
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("%s - Damage %3.0f%%"),
					  getStatName(psStructure->pStructureType), getStructureDamage(psStructure) * (100.f/65536.f)));
		}
		break;
	case REF_RESEARCH:
#ifdef DEBUG
		if (getDebugMappingStatus())
		{
			CONPRINTF(ConsoleString, (ConsoleString, "%s - Damage % 3.2f%% - Unique ID %u - Research Points: %u - TimeToResearch: %u",
					  getStatName(psStructure->pStructureType), getStructureDamage(psStructure) * (100.f/65536.f), psStructure->id,
					  psStructure->pFunctionality->researchFacility.researchPoints,
					  psStructure->pFunctionality->researchFacility.timeToResearch));
		}
		else
#endif
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("%s - Damage %3.0f%%"),
					  getStatName(psStructure->pStructureType), getStructureDamage(psStructure) * (100.f/65536.f)));
		}
		break;
	default:
#ifdef DEBUG
		if (getDebugMappingStatus())
		{
			CONPRINTF(ConsoleString, (ConsoleString, "%s - Damage % 3.2f%% - Unique ID %u",
					  getStatName(psStructure->pStructureType), getStructureDamage(psStructure) * (100.f/65536.f), psStructure->id));
		}
		else
#endif
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("%s - Damage %3.0f%%"),
					  getStatName(psStructure->pStructureType), getStructureDamage(psStructure) * (100.f/65536.f)));
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
	else if (psTemplate->asParts[COMP_PROPULSION] &&
	         ((asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->propulsionType == PROPULSION_TYPE_LIFT))
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
		if (!psTemplate->asParts[COMP_PROPULSION] ||
		    ((asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->propulsionType != PROPULSION_TYPE_LIFT))
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
	Vector3i	pos;
	UDWORD		i;

	ASSERT_OR_RETURN(false, attackPlayer < MAX_PLAYERS, "Invalid player id %d", (int)attackPlayer);
	ASSERT_OR_RETURN(false, psTarget != NULL, "Target is NULL");

	//structure electronic damage
	if (psTarget->type == OBJ_STRUCTURE)
	{
		psStructure = (STRUCTURE *)psTarget;
		bCompleted = false;

		if (psStructure->pStructureType->resistance == 0)
		{
			return false;	// this structure type cannot be taken over
		}

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

		//in multiPlayer cannot attack a Transporter with EW
		if (bMultiPlayer)
		{
			ASSERT_OR_RETURN(true, psDroid->droidType != DROID_TRANSPORTER, "Cannot attack a Transporter in multiPlayer");
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

				giftSingleDroid(psDroid, attackPlayer);

				// tell the world!
				// If the world is in synch, the world already knows, though.
				if (bMultiMessages)
				{
					uint8_t giftType = DROID_GIFT;

					NETbeginEncode(NETgameQueue(selectedPlayer), GAME_GIFT);
					{
						// We need to distinguish between gift types
						NETuint8_t(&giftType);

						// send the player originally owning this droid
						// and the new owner
						NETuint8_t(&psDroid->player);
						NETuint8_t(&attackPlayer);

						// followed by the droid's ID
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

	ASSERT_OR_RETURN(false, psStruct != NULL, "Invalid structure pointer");

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
		case REF_GATE:
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

	ASSERT_OR_RETURN(0, psStructure != NULL, "Invalid structure pointer");

	switch(psStats->type)
	{
		// modules may be attached:
		case REF_FACTORY:
		case REF_VTOL_FACTORY:
			ASSERT_OR_RETURN(0, psStructure->pFunctionality != NULL, "Invalid structure functionality pointer for factory base body");
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
			ASSERT_OR_RETURN(0, psStructure->pFunctionality != NULL, "Invalid structure functionality pointer for research base body");
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
			ASSERT_OR_RETURN(0, psStructure->pFunctionality != NULL, "Invalid structure functionality pointer for power gen base body");
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
	case REF_GATE:
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
	case REF_GATE:
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

	// share exploration info - pretty useless but perhaps a nice touch?
	for(x = 0; x < mapWidth; x++)
	{
		for(y = 0; y < mapHeight; y++)
		{
			MAPTILE *psTile = mapTile(x, y);
			if (TEST_TILE_VISIBLE(losingPlayer, psTile))
			{
				psTile->tileExploredBits |= alliancebits[rewardPlayer];
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
	ASSERT_OR_RETURN(false, Struct != NULL, "Invalid structure!");
	ASSERT_OR_RETURN(false, Struct->pStructureType != NULL, "Invalid structureType!");

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
		for (psCurrFlag = apsFlagPosLists[Struct->player]; psCurrFlag;
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
void cancelProduction(STRUCTURE *psBuilding, QUEUE_MODE mode)
{
	FACTORY *psFactory;

	ASSERT_OR_RETURN( , StructIsFactory(psBuilding), "structure not a factory");

	psFactory = &psBuilding->pFunctionality->factory;

	if (mode == ModeQueue)
	{
		sendStructureInfo(psBuilding, STRUCTUREINFO_CANCELPRODUCTION, NULL);

		psFactory->psSubjectPending = NULL;
		psFactory->statusPending = FACTORY_CANCEL_PENDING;
		++psFactory->pendingCount;

		if (psBuilding->player == productionPlayer)
		{
			//clear the production run for this factory
			memset(asProductionRun[psFactory->psAssemblyPoint->factoryType][psFactory->psAssemblyPoint->factoryInc], 0, sizeof(PRODUCTION_RUN) * MAX_PROD_RUN);
			psFactory->productionLoops = 0;

			//tell the interface
			intManufactureFinished(psBuilding);
		}

		return;
	}

	//check its the correct factory
	if (psFactory->psSubject)
	{
		// give the power back that was used until now
		int secondsToBuild = ((DROID_TEMPLATE*)psFactory->psSubject)->buildPoints/psFactory->productionOutput;
		int secondsElapsed = secondsToBuild - psFactory->timeToBuild;
		int powerUsed = 0;
		if (secondsElapsed > secondsToBuild) // can happen if factory's been upgraded since droid was created
		{
			secondsElapsed = secondsToBuild;
		}
		if (secondsElapsed > 0)
		{
			powerUsed = (int)(((DROID_TEMPLATE *)psFactory->psSubject)->powerPoints*secondsElapsed)/secondsToBuild;
		}
		addPower(psBuilding->player, powerUsed);

		//clear the factory's subject
		psFactory->psSubject = NULL;
	}
}


/*set a factory's production run to hold*/
void holdProduction(STRUCTURE *psBuilding, QUEUE_MODE mode)
{
	FACTORY *psFactory;

	ASSERT_OR_RETURN( , StructIsFactory(psBuilding), "structure not a factory");

	psFactory = &psBuilding->pFunctionality->factory;

	if (mode == ModeQueue)
	{
		sendStructureInfo(psBuilding, STRUCTUREINFO_HOLDPRODUCTION, NULL);

		if (psFactory->psSubjectPending == NULL)
		{
			psFactory->psSubjectPending = psFactory->psSubject;
		}
		else
		{
			psFactory->statusPending = FACTORY_HOLD_PENDING;
		}
		++psFactory->pendingCount;

		return;
	}

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
void releaseProduction(STRUCTURE *psBuilding, QUEUE_MODE mode)
{
	FACTORY *psFactory = &psBuilding->pFunctionality->factory;

	ASSERT_OR_RETURN( , StructIsFactory(psBuilding), "structure not a factory");

	psFactory = &psBuilding->pFunctionality->factory;

	if (mode == ModeQueue)
	{
		sendStructureInfo(psBuilding, STRUCTUREINFO_RELEASEPRODUCTION, NULL);

		if (psFactory->psSubjectPending == NULL && psFactory->statusPending != FACTORY_CANCEL_PENDING)
		{
			psFactory->psSubjectPending = psFactory->psSubject;
		}
		if (psFactory->psSubjectPending != NULL)
		{
			psFactory->statusPending = FACTORY_START_PENDING;
		}
		++psFactory->pendingCount;

		return;
	}

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

void doNextProduction(STRUCTURE *psStructure, DROID_TEMPLATE *current, QUEUE_MODE mode)
{
	DROID_TEMPLATE *psNextTemplate = factoryProdUpdate(psStructure, current);

	if (psNextTemplate != NULL)
	{
		if (mode == ModeImmediate)
		{
			cancelProduction(psStructure, ModeImmediate);
		}
		structSetManufacture(psStructure, psNextTemplate, ModeQueue);
	}
	else
	{
		cancelProduction(psStructure, mode);
	}
}

/*this is called when a factory produces a droid. The Template returned is the next
one to build - if any*/
DROID_TEMPLATE * factoryProdUpdate(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate)
{
	UDWORD		inc, factoryType, factoryInc;
	FACTORY		*psFactory;
	bool            somethingInQueue = false;

	CHECK_STRUCTURE(psStructure);
	if (psStructure->player != productionPlayer)
	{
		return NULL;
	}

	psFactory = &psStructure->pFunctionality->factory;
	factoryType = psFactory->psAssemblyPoint->factoryType;
	factoryInc = psFactory->psAssemblyPoint->factoryInc;

	if (psTemplate != NULL)
	{
		//find the entry in the array for this template
		for (inc=0; inc < MAX_PROD_RUN; inc++)
		{
			somethingInQueue = somethingInQueue || asProductionRun[factoryType][factoryInc][inc].quantity != 0;

			if (asProductionRun[factoryType][factoryInc][inc].psTemplate != NULL && asProductionRun[factoryType][factoryInc][inc].psTemplate->multiPlayerID == psTemplate->multiPlayerID)
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
	// Check that we aren't looping doing nothing.
	if (!somethingInQueue)
	{
		psFactory->productionLoops = 0;  // Don't do nothing infinitely many times.
	}
	/*If you've got here there's nothing left to build unless factory is
	on loop production*/
	if (psFactory->productionLoops != 0)
	{
		//reduce the loop count if not infinite
		if (psFactory->productionLoops != INFINITE_PRODUCTION)
		{
			psFactory->productionLoops--;
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
		return factoryProdUpdate(psStructure, NULL);
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
	ASSERT_OR_RETURN( , psStructure->player == productionPlayer, "called for incorrect player");

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
						// FIXME: give power back
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
						// FIXME: give power back
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
			psFactory->productionLoops = 0;
		}
	}
}

/** checks the status of the production of a template
 * if totalquantity is true, it will return the total ordered amount
 * it it is false, it will return the amount that has already been built
 */
static int getProduction(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate, BOOL totalQuantity)
{
	if (psTemplate == NULL)
	{
		return 0;  // Not producing any NULL pointers.
	}

	if (psStructure != NULL && psStructure->player == productionPlayer)
	{
		FACTORY *psFactory = &psStructure->pFunctionality->factory;
		unsigned factoryType = psFactory->psAssemblyPoint->factoryType;
		unsigned factoryInc = psFactory->psAssemblyPoint->factoryInc;

		//see if the template is in the list
		unsigned inc;
		for (inc=0; inc < MAX_PROD_RUN; inc++)
		{
			if (asProductionRun[factoryType][factoryInc][inc].psTemplate == NULL)
			{
				continue;
			}

			if (asProductionRun[factoryType][factoryInc][inc].psTemplate->multiPlayerID == psTemplate->multiPlayerID)
			{
				if (totalQuantity)
				{
					return asProductionRun[factoryType][factoryInc][inc].quantity;
				}
				else
				{
					return asProductionRun[factoryType][factoryInc][inc].built;
				}
			}
		}
	}

	//not in the list so none being produced
	return 0;
}

/// the total amount ordered of a specific template in the production list
UDWORD	getProductionQuantity(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate)
{
	return getProduction(psStructure, psTemplate, true);
}


/// the number of times a specific template in the production list has been built
UDWORD	getProductionBuilt(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate)
{
	return getProduction(psStructure, psTemplate, false);
}


/*looks through a players production list to see how many command droids
are being built*/
UBYTE checkProductionForCommand(UBYTE player)
{
	UBYTE		factoryInc, inc, factoryType;
	uint32_t        mask = 1;
	UBYTE           quantity = 0;

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
	uint32_t        mask = 1;
	UBYTE           quantity = 0;

	ASSERT_OR_RETURN(0, player == selectedPlayer, "%s should only be called for selectedPlayer", __FUNCTION__);

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
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Invalid player");
	ASSERT_OR_RETURN(false, factoryType < NUM_FACTORY_TYPES, "Invalid factoryType");

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
						ASSERT_OR_RETURN( , psFactory->psAssemblyPoint != NULL, "no delivery point for factory");
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
							ASSERT_OR_RETURN( , psRepair->psDeliveryPoint != NULL, "no delivery point for repair facility");
						}
						else
						{
							// add an assembly point
							if (!createFlagPosition(&psRepair->psDeliveryPoint, psStruct->player))
							{
								ASSERT(!"can't create new deilivery point for repair facility", "unable to create new delivery point for repair facility");
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

	ASSERT_OR_RETURN( , StructIsFactory(psStruct), "structure is not a factory");
	ASSERT_OR_RETURN( , psStruct->player == selectedPlayer, "should only be called for selectedPlayer");

	psFactory = &psStruct->pFunctionality->factory;

	if (add)
	{
		//check for wrapping to infinite production
		if (psFactory->productionLoops == MAX_IN_RUN)
		{
			psFactory->productionLoops = 0;
		}
		else
		{
			//increment the count
			psFactory->productionLoops++;
			//check for limit - this caters for when on infinite production and want to wrap around
			if (psFactory->productionLoops > MAX_IN_RUN)
			{
				psFactory->productionLoops = INFINITE_PRODUCTION;
			}
		}
	}
	else
	{
		//decrement the count
		if (psFactory->productionLoops == 0)
		{
			psFactory->productionLoops = INFINITE_PRODUCTION;
		}
		else
		{
			psFactory->productionLoops--;
		}
	}
}


/*Used for determining how much of the structure to draw as being built or demolished*/
float structHeightScale(STRUCTURE *psStruct)
{
	float retVal = (float)psStruct->currentBuildPts / (float)psStruct->pStructureType->buildPoints;

	return MAX(retVal, 0.05f);
}


/*compares the structure sensor type with the droid weapon type to see if the
FIRE_SUPPORT order can be assigned*/
BOOL structSensorDroidWeapon(STRUCTURE *psStruct, DROID *psDroid)
{
	//another crash when nStat is marked as 0xcd... FIXME: Why is nStat not initialized properly?
	//Added a safety check: Only units with weapons can be assigned.
	if (psDroid->numWeaps > 0)
	{
		//Standard Sensor Tower + indirect weapon droid (non VTOL)
		//else if (structStandardSensor(psStruct) && (psDroid->numWeaps &&
		if (structStandardSensor(psStruct) && (psDroid->asWeaps[0].nStat > 0 &&
			!proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat)) &&
			!isVtolDroid(psDroid))
		{
			return true;
		}
		//CB Sensor Tower + indirect weapon droid (non VTOL)
		//if (structCBSensor(psStruct) && (psDroid->numWeaps &&
		else if (structCBSensor(psStruct) && (psDroid->asWeaps[0].nStat > 0 &&
			!proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat)) &&
			!isVtolDroid(psDroid))
		{
			return true;
		}
		//VTOL Intercept Sensor Tower + any weapon VTOL droid
		//else if (structVTOLSensor(psStruct) && psDroid->numWeaps &&
		else if (structVTOLSensor(psStruct) && psDroid->asWeaps[0].nStat > 0 &&
			isVtolDroid(psDroid))
		{
			return true;
		}
		//VTOL CB Sensor Tower + any weapon VTOL droid
		//else if (structVTOLCBSensor(psStruct) && psDroid->numWeaps &&
		else if (structVTOLCBSensor(psStruct) && psDroid->asWeaps[0].nStat > 0 &&
			isVtolDroid(psDroid))
		{
			return true;
		}
	}

	//case not matched
	return false;
}


/*checks if the structure has a Counter Battery sensor attached - returns
true if it has*/
BOOL structCBSensor(const STRUCTURE* psStruct)
{
	// Super Sensor works as any type
	if (psStruct->pStructureType->pSensor
	 && (psStruct->pStructureType->pSensor->type == INDIRECT_CB_SENSOR
	  || psStruct->pStructureType->pSensor->type == SUPER_SENSOR)
	 && psStruct->pStructureType->pSensor->location == LOC_TURRET)
	{
		return true;
	}

	return false;
}


/*checks if the structure has a Standard Turret sensor attached - returns
true if it has*/
BOOL structStandardSensor(const STRUCTURE* psStruct)
{
	// Super Sensor works as any type
	if (psStruct->pStructureType->pSensor
	 && (psStruct->pStructureType->pSensor->type == STANDARD_SENSOR
	  || psStruct->pStructureType->pSensor->type == SUPER_SENSOR)
	 && psStruct->pStructureType->pSensor->location == LOC_TURRET)
	{
		return true;
	}

	return false;
}


/*checks if the structure has a VTOL Intercept sensor attached - returns
true if it has*/
BOOL structVTOLSensor(const STRUCTURE* psStruct)
{
	// Super Sensor works as any type
	if (psStruct->pStructureType->pSensor
	 && (psStruct->pStructureType->pSensor->type == VTOL_INTERCEPT_SENSOR
	  || psStruct->pStructureType->pSensor->type == SUPER_SENSOR)
	 && psStruct->pStructureType->pSensor->location == LOC_TURRET)
	{
		return true;
	}

	return false;
}


/*checks if the structure has a VTOL Counter Battery sensor attached - returns
true if it has*/
BOOL structVTOLCBSensor(const STRUCTURE* psStruct)
{
	// Super Sensor works as any type
	if (psStruct->pStructureType->pSensor
	 && (psStruct->pStructureType->pSensor->type == VTOL_CB_SENSOR
	  || psStruct->pStructureType->pSensor->type == RADAR_DETECTOR_SENSOR
	  || psStruct->pStructureType->pSensor->type == SUPER_SENSOR)
	 && psStruct->pStructureType->pSensor->location == LOC_TURRET)
	{
		return true;
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

	ASSERT_OR_RETURN(NULL, psDroid != NULL, "findNearestReArmPad: No droid was passed.");

	if (psTarget != NULL)
	{
		if (!vtolOnRearmPad(psTarget, psDroid))
		{
			return psTarget;
		}
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
	SDWORD	tx,ty, i;

	tx = map_coord(psStruct->pos.x);
	ty = map_coord(psStruct->pos.y);

	for (i=0; i<MAX_PLAYERS; i++)
	{
		if (aiCheckAlliances(psStruct->player, i))
		{
			for (psCurr = apsDroidLists[i]; psCurr; psCurr=psCurr->psNext)
			{
				if (psCurr != psDroid
				 && map_coord(psCurr->pos.x) == tx
				 && map_coord(psCurr->pos.y) == ty
				 && isVtolDroid(psCurr))
				{
					actionDroidObj(psCurr, DACTION_CLEARREARMPAD, (BASE_OBJECT *)psStruct);
				}
			}
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
	for (psCurr = apsDroidLists[psStruct->player]; psCurr; psCurr=psCurr->psNext)
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
		//added 'selectedPlayer != 0' to be able to test the game by changing player...
		//in this version of Warzone, the attack Player can NEVER be the selectedPlayer (unless from the script)
		ASSERT_OR_RETURN(NULL, bFromScript || selectedPlayer != 0 || attackPlayer != selectedPlayer, "EW attack by selectedPlayer on a structure");
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
					orderDroid(psCurr, DORDER_STOP, ModeImmediate);
					break;
				}
				for (i = 0;i < psCurr->numWeaps;i++)
				{
					if (psCurr->psActionTarget[i] == (BASE_OBJECT *)psStructure)
					{
						orderDroid(psCurr, DORDER_STOP, ModeImmediate);
						break;
					}
				}
				//check through order list
				orderClearTargetFromDroidList(psCurr, (BASE_OBJECT *)psStructure);
			}

			//check through the 'attackPlayer' players list of structures to see if any are targetting it
			for (psStruct = apsStructLists[attackPlayer]; psStruct != NULL; psStruct =
				psStruct->psNext)
			{
				if (psStruct->psTarget[0] == (BASE_OBJECT *)psStructure)
				{
					setStructureTarget(psStruct, NULL, 0, ORIGIN_UNKNOWN);
				}
			}

			//add back into cluster system
			clustNewStruct(psStructure);

			if (psStructure->status == SS_BUILT)
			{
				buildingComplete(psStructure);
			}
			//since the structure isn't being rebuilt, the visibility code needs to be adjusted
			//make sure this structure is visible to selectedPlayer
			psStructure->visible[attackPlayer] = UINT8_MAX;
		}
		return NULL;
	}


	//save info about the structure
	psType = psStructure->pStructureType;
	x = psStructure->pos.x;
	y = psStructure->pos.y;
	direction = psStructure->rot.direction;
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
		psNewStruct->rot.direction = direction;
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

	recoil = recoil * asWeaponStats[psStruct->asWeaps[0].nStat].recoilValue / 100;

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

				ASSERT_OR_RETURN(false, asStructureStats[structInc].asFuncList[inc] != NULL,
				                 "Invalid function for structure %s", asStructureStats[structInc].pName);

			}
		}
		else
		{
			ASSERT_OR_RETURN(false, asStructureStats[structInc].asFuncList == NULL, "Invalid functions attached to structure %s",
			                 asStructureStats[structInc].pName);
		}
	}
	return true;
}


/*returns the power cost to build this structure*/
UDWORD structPowerToBuild(const STRUCTURE* psStruct)
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
			capacity = psStruct->pFunctionality->powerGenerator.capacity;
			break;
		case REF_RESEARCH:
			capacity = psStruct->pFunctionality->researchFacility.capacity;
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
void cbNewDroid(STRUCTURE *psFactory, DROID *psDroid)
{
	ASSERT_OR_RETURN( , psDroid != NULL, "no droid assigned for CALL_NEWDROID callback");

	psScrCBNewDroid = psDroid;
	psScrCBNewDroidFact = psFactory;
	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_NEWDROID);
	psScrCBNewDroid = NULL;
	psScrCBNewDroidFact = NULL;
}

unsigned getStructureWidth(const STRUCTURE *psBuilding)
{
	return getStructureStatsWidth(psBuilding->pStructureType, psBuilding->rot.direction);
}

unsigned getStructureBreadth(const STRUCTURE *psBuilding)
{
	return getStructureStatsBreadth(psBuilding->pStructureType, psBuilding->rot.direction);
}

unsigned getStructureStatsWidth(const STRUCTURE_STATS *pStructureType, uint16_t direction)
{
	if (((direction + 0x2000) & 0x4000) != 0)
	{
		// Building is rotated left or right by 90°, swap width and height.
		return pStructureType->baseBreadth;
	}

	// Building has normal orientation (or upsidedown).
	return pStructureType->baseWidth;
}

unsigned getStructureStatsBreadth(const STRUCTURE_STATS *pStructureType, uint16_t direction)
{
	if (((direction + 0x2000) & 0x4000) != 0)
	{
		// Building is rotated left or right by 90°, swap width and height.
		return pStructureType->baseWidth;
	}

	// Building has normal orientation (or upsidedown).
	return pStructureType->baseBreadth;
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

void checkStructure(const STRUCTURE* psStructure, const char * const location_description, const char * function, const int recurse)
{
	unsigned int i;

	if (recurse < 0)
		return;

	ASSERT_HELPER(psStructure != NULL, location_description, function, "CHECK_STRUCTURE: NULL pointer");
	ASSERT_HELPER(psStructure->type == OBJ_STRUCTURE, location_description, function, "CHECK_STRUCTURE: No structure (type num %u)", (unsigned int)psStructure->type);
	ASSERT_HELPER(psStructure->player < MAX_PLAYERS, location_description, function, "CHECK_STRUCTURE: Out of bound player num (%u)", (unsigned int)psStructure->player);
	ASSERT_HELPER(psStructure->pStructureType->type < NUM_DIFF_BUILDINGS, location_description, function, "CHECK_STRUCTURE: Out of bound structure type (%u)", (unsigned int)psStructure->pStructureType->type);
	ASSERT_HELPER(psStructure->numWeaps <= STRUCT_MAXWEAPS, location_description, function, "CHECK_STRUCTURE: Out of bound weapon count (%u)", (unsigned int)psStructure->numWeaps);

	for (i = 0; i < ARRAY_SIZE(psStructure->asWeaps); ++i)
	{
		if (psStructure->psTarget[i])
		{
			checkObject(psStructure->psTarget[i], location_description, function, recurse - 1);
		}
	}
}
