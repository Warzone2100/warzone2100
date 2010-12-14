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
/**
 * @file stats.c
 *
 * Store common stats for weapons, components, brains, etc.
 *
 */
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/frameresource.h"
#include "lib/gamelib/gtime.h"
#include "objects.h"
#include "stats.h"
#include "map.h"
#include "main.h"
#include "lib/sound/audio_id.h"
#include "projectile.h"
#include "text.h"

#define WEAPON_TIME		100

/* The stores for the different stats */
BODY_STATS		*asBodyStats;
BRAIN_STATS		*asBrainStats;
PROPULSION_STATS	*asPropulsionStats;
SENSOR_STATS		*asSensorStats;
ECM_STATS		*asECMStats;
REPAIR_STATS		*asRepairStats;
WEAPON_STATS		*asWeaponStats;
CONSTRUCT_STATS		*asConstructStats;
PROPULSION_TYPES	*asPropulsionTypes;
static TERRAIN_TABLE	*asTerrainTable;
static SPECIAL_ABILITY	*asSpecialAbility;

//used to hold the modifiers cross refd by weapon effect and propulsion type
WEAPON_MODIFIER		asWeaponModifier[WE_NUMEFFECTS][PROPULSION_TYPE_NUM];

//used to hold the current upgrade level per player per weapon subclass
WEAPON_UPGRADE		asWeaponUpgrade[MAX_PLAYERS][WSC_NUM_WEAPON_SUBCLASSES];
SENSOR_UPGRADE		asSensorUpgrade[MAX_PLAYERS];
ECM_UPGRADE		asECMUpgrade[MAX_PLAYERS];
REPAIR_UPGRADE		asRepairUpgrade[MAX_PLAYERS];
CONSTRUCTOR_UPGRADE	asConstUpgrade[MAX_PLAYERS];
BODY_UPGRADE		asBodyUpgrade[MAX_PLAYERS][BODY_TYPE];

/* The number of different stats stored */
UDWORD		numBodyStats;
UDWORD		numBrainStats;
UDWORD		numPropulsionStats;
UDWORD		numSensorStats;
UDWORD		numECMStats;
UDWORD		numRepairStats;
UDWORD		numWeaponStats;
UDWORD		numConstructStats;
static UDWORD	numSpecialAbility;

//the max values of the stats used in the design screen
static UDWORD	maxComponentWeight;
static UDWORD	maxBodyArmour;
static UDWORD	maxBodyPower;
static UDWORD	maxBodyPoints;
static UDWORD	maxSensorRange;
static UDWORD	maxSensorPower;
static UDWORD	maxECMPower;
static UDWORD	maxECMRange;
static UDWORD	maxConstPoints;
static UDWORD	maxRepairPoints;
static UDWORD	maxWeaponRange;
static UDWORD	maxWeaponDamage;
static UDWORD	maxWeaponROF;
static UDWORD	maxPropulsionSpeed;

//stores for each players component states - can be either UNAVAILABLE, REDUNDANT, FOUND or AVAILABLE
UBYTE		*apCompLists[MAX_PLAYERS][COMP_NUMCOMPONENTS];

//store for each players Structure states
UBYTE		*apStructTypeLists[MAX_PLAYERS];

static BOOL compareYes(const char *strToCompare, const char *strOwner);
static bool getMovementModel(const char* movementModel, MOVEMENT_MODEL* model);

//Access functions for the max values to be used in the Design Screen
static void setMaxComponentWeight(UDWORD weight);
static void setMaxBodyArmour(UDWORD armour);
static void setMaxBodyPower(UDWORD power);
static void setMaxBodyPoints(UDWORD points);
static void setMaxSensorRange(UDWORD range);
static void setMaxSensorPower(UDWORD power);
static void setMaxECMRange(UDWORD power);
static void setMaxECMPower(UDWORD power);
static void setMaxConstPoints(UDWORD points);
static void setMaxRepairPoints(UDWORD repair);
static void setMaxWeaponRange(UDWORD range);
static void setMaxWeaponDamage(UDWORD damage);
static void setMaxWeaponROF(UDWORD rof);
static void setMaxPropulsionSpeed(UDWORD speed);

//determine the effect this upgrade would have on the max values
static void updateMaxWeaponStats(UWORD maxValue);
static void updateMaxSensorStats(UWORD maxRange, UWORD maxPower);
static void updateMaxRepairStats(UWORD maxValue);
static void updateMaxECMStats(UWORD maxValue);
static void updateMaxBodyStats(UWORD maxBody, UWORD maxPower, UWORD maxArmour);
static void updateMaxConstStats(UWORD maxValue);

/*******************************************************************************
*		Generic stats macros/functions
*******************************************************************************/

/* Macro to allocate memory for a set of stats */
#define ALLOC_STATS(numEntries, list, listSize, type) \
	ASSERT( (numEntries) < REF_RANGE, \
	"allocStats: number of stats entries too large for " #type );\
	if ((list))	free((list));	\
	(list) = (type *)malloc(sizeof(type) * (numEntries)); \
	if ((list) == NULL) \
	{ \
		debug( LOG_FATAL, "Out of memory" ); \
		abort(); \
		return false; \
	} \
	(listSize) = (numEntries); \
	return true


/*Macro to Deallocate stats*/
#define STATS_DEALLOC(list, listSize, type) \
	statsDealloc((COMPONENT_STATS*)(list), (listSize), sizeof(type)); \
	(list) = NULL 


void statsInitVars(void)
{
	/* The number of different stats stored */
	numBodyStats = 0;
	numBrainStats = 0;
	numPropulsionStats = 0;
	numSensorStats = 0;
	numECMStats = 0;
	numRepairStats = 0;
	numWeaponStats = 0;
	numConstructStats = 0;
	numSpecialAbility = 0;

	//initialise the upgrade structures
	memset(asWeaponUpgrade, 0, sizeof(asWeaponUpgrade));
	memset(asSensorUpgrade, 0, sizeof(asSensorUpgrade));
	memset(asECMUpgrade, 0, sizeof(asECMUpgrade));
	memset(asRepairUpgrade, 0, sizeof(asRepairUpgrade));
	memset(asBodyUpgrade, 0, sizeof(asBodyUpgrade));

	// init the max values
	maxComponentWeight = maxBodyArmour = maxBodyPower =
		maxBodyPoints = maxSensorRange = maxSensorPower = maxECMPower = maxECMRange =
		maxConstPoints = maxRepairPoints = maxWeaponRange = maxWeaponDamage =
		maxPropulsionSpeed = 0;
}


/*Deallocate all the stats assigned from input data*/
void statsDealloc(COMPONENT_STATS* pStats, UDWORD listSize, UDWORD structureSize)
{
	if (pStats)
	{
		free(pStats);
	}
}


static BOOL allocateStatName(BASE_STATS* pStat, const char *Name)
{
	pStat->pName = allocateName(Name);

	return pStat != NULL;
}


/* body stats need the extra list removing */
static void deallocBodyStats(void)
{
	BODY_STATS *psStat;
	UDWORD		inc;

	for (inc = 0; inc < numBodyStats; inc++)
	{
		psStat = &asBodyStats[inc];
		free(psStat->ppIMDList);
	}
	free(asBodyStats);
	asBodyStats = NULL;
}

/*Deallocate all the stats assigned from input data*/
BOOL statsShutDown(void)
{
	STATS_DEALLOC(asWeaponStats, numWeaponStats, WEAPON_STATS);
	//STATS_DEALLOC(asBodyStats, numBodyStats, BODY_STATS);
	deallocBodyStats();
	STATS_DEALLOC(asBrainStats, numBrainStats, BRAIN_STATS);
	STATS_DEALLOC(asPropulsionStats, numPropulsionStats, PROPULSION_STATS);
	STATS_DEALLOC(asSensorStats, numSensorStats, SENSOR_STATS);
	STATS_DEALLOC(asECMStats, numECMStats, ECM_STATS);
	STATS_DEALLOC(asRepairStats, numRepairStats, REPAIR_STATS);
	STATS_DEALLOC(asConstructStats, numConstructStats, CONSTRUCT_STATS);
	deallocPropulsionTypes();
	deallocTerrainTable();
	deallocSpecialAbility();

	return true;
}

/* Macro to set the stats for a particular ref
 * The macro uses the ref number in the stats structure to
 * index the correct array entry
 */
#define SET_STATS(stats, list, index, type, refStart) \
	ASSERT( ((stats)->ref >= (refStart)) && ((stats)->ref < (refStart) + REF_RANGE), \
		"setStats: Invalid " #type " ref number" ); \
	memcpy((list) + (index), (stats), sizeof(type))


/* Return the number of newlines in a file buffer */
UDWORD numCR(const char *pFileBuffer, UDWORD fileSize)
{
	UDWORD  lines=0;

	while (fileSize-- > 0)
	{
		if (*pFileBuffer++ == '\n')
		{
			lines++;
		}
	}

	return lines;
}


/*******************************************************************************
*		Allocate stats functions
*******************************************************************************/
/* Allocate Weapon stats */
BOOL statsAllocWeapons(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asWeaponStats, numWeaponStats, WEAPON_STATS);
}
/* Allocate Body Stats */
BOOL statsAllocBody(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asBodyStats, numBodyStats, BODY_STATS);
}
/* Allocate Brain Stats */
BOOL statsAllocBrain(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asBrainStats, numBrainStats, BRAIN_STATS);
}
/* Allocate Propulsion Stats */
BOOL statsAllocPropulsion(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asPropulsionStats, numPropulsionStats, PROPULSION_STATS);
}
/* Allocate Sensor Stats */
BOOL statsAllocSensor(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asSensorStats, numSensorStats, SENSOR_STATS);
}
/* Allocate Ecm Stats */
BOOL statsAllocECM(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asECMStats, numECMStats, ECM_STATS);
}

/* Allocate Repair Stats */
BOOL statsAllocRepair(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asRepairStats, numRepairStats, REPAIR_STATS);
}

/* Allocate Construct Stats */
BOOL statsAllocConstruct(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asConstructStats, numConstructStats, CONSTRUCT_STATS);
}

const char *getStatName(const void * Stat)
{
	const BASE_STATS * const psStats= (const BASE_STATS*)Stat;

	return getName(psStats->pName);
}




/*******************************************************************************
*		Load stats functions
*******************************************************************************/

/*Load the weapon stats from the file exported from Access*/
BOOL loadWeaponStats(const char *pWeaponData, UDWORD bufferSize)
{
	unsigned int	NumWeapons = numCR(pWeaponData, bufferSize);
	WEAPON_STATS	sStats, * const psStats = &sStats;
	UDWORD			i, rotate, maxElevation, surfaceToAir;
	SDWORD			minElevation;
	char			WeaponName[MAX_STR_LENGTH], GfxFile[MAX_STR_LENGTH];
	char			mountGfx[MAX_STR_LENGTH], flightGfx[MAX_STR_LENGTH],
					hitGfx[MAX_STR_LENGTH], missGfx[MAX_STR_LENGTH],
					waterGfx[MAX_STR_LENGTH], muzzleGfx[MAX_STR_LENGTH],
					trailGfx[MAX_STR_LENGTH], dummy[MAX_STR_LENGTH];
	char			fireOnMove[10], weaponClass[15], weaponSubClass[15],
					weaponEffect[16], movement[15], facePlayer[5],		//weaponEffect[15] caused stack corruption. --Qamly
					faceInFlight[5],lightWorld[5];
	UDWORD			longRange, effectSize, numAttackRuns, designable;
	UDWORD			numRounds;

	char			*StatsName;
	UDWORD			penetrate;
	UDWORD dummyVal;

	// Skip descriptive header
	if (strncmp(pWeaponData,"Weapon ",7)==0)
	{
		pWeaponData = strchr(pWeaponData,'\n') + 1;
		NumWeapons--;
	}

	if (!statsAllocWeapons(NumWeapons))
	{
		return false;
	}

	for (i=0; i < NumWeapons; i++)
	{
		memset(psStats, 0, sizeof(WEAPON_STATS));

		WeaponName[0] = '\0';
		GfxFile[0] = '\0';
		mountGfx[0] = '\0';
		muzzleGfx[0] = '\0';
		flightGfx[0] = '\0';
		hitGfx[0] = '\0';
		missGfx[0] = '\0';
		waterGfx[0] = '\0';
		trailGfx[0] = '\0';
		fireOnMove[0] = '\0';
		weaponClass[0] = '\0';
		weaponSubClass[0] = '\0';
		movement[0] = '\0';
		weaponEffect[0] = '\0';
		facePlayer[0] = '\0';
		faceInFlight[0] = '\0';


		//read the data into the storage - the data is delimeted using comma's
		sscanf(pWeaponData,"%[^','],%[^','],%d,%d,%d,%d,%d,%d,%[^','],\
			%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%d,\
			%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%[^','],\
			%[^','],%[^','],%[^','],%[^','],%d,%d,%d,%[^','],%[^','],%d,%d,\
			%[^','],%d,%d,%d,%d,%d",
			(char *)&WeaponName, (char *)&dummy, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &dummyVal, &dummyVal,
			&psStats->body, (char *)&GfxFile, (char *)&mountGfx, (char *)&muzzleGfx, (char *)&flightGfx,
			(char *)&hitGfx, (char *)&missGfx, (char *)&waterGfx, (char *)&trailGfx, &psStats->shortRange,
			&psStats->longRange,&psStats->shortHit, &psStats->longHit,
			&psStats->firePause, &psStats->numExplosions, &numRounds,
			&psStats->reloadTime, &psStats->damage, &psStats->radius,
			&psStats->radiusHit, &psStats->radiusDamage, &psStats->incenTime,
			&psStats->incenDamage, &psStats->incenRadius, &psStats->directLife,
			&psStats->radiusLife, &psStats->flightSpeed, &dummyVal,
			(char *)&fireOnMove, (char *)&weaponClass, (char *)&weaponSubClass, (char *)&movement, (char *)&weaponEffect,
			&rotate, &maxElevation, &minElevation, (char *)&facePlayer, (char *)&faceInFlight,
			&psStats->recoilValue, &psStats->minRange,	(char *)&lightWorld,
			&effectSize, &surfaceToAir, &numAttackRuns, &designable, &penetrate);

			psStats->numRounds = (UBYTE)numRounds;

//#ifdef DEBUG
// Hack to get the current stats working... a zero flight speed value will cause an assert in projectile.c line 957
//  I'm not sure if this should be on debug only...
//    ... the last thing we want is for a zero value to get through on release (with no asserts!)
//
// Anyway if anyone has a problem with this, take it up with Tim ... we have a frank and open discussion about it.
#define DEFAULT_FLIGHTSPEED (500)
		if (psStats->flightSpeed==0)
		{
			debug( LOG_NEVER, "STATS: Zero Flightspeed for %s - using default of %d\n", WeaponName, DEFAULT_FLIGHTSPEED );
			psStats->flightSpeed=DEFAULT_FLIGHTSPEED;
		}


		if (!allocateStatName((BASE_STATS *)psStats, WeaponName))
		{
			return false;
		}

		psStats->ref = REF_WEAPON_START + i;

		//multiply time stats
		psStats->firePause *= WEAPON_TIME;
		psStats->incenTime *= WEAPON_TIME;
		psStats->directLife *= WEAPON_TIME;
		psStats->radiusLife *= WEAPON_TIME;
		psStats->reloadTime *= WEAPON_TIME;

		//get the IMD for the component
		if (strcmp(GfxFile, "0"))
		{
			psStats->pIMD = (iIMDShape *) resGetData("IMD", GfxFile);
			if (psStats->pIMD == NULL)
			{

				debug( LOG_FATAL, "Cannot find the weapon PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}
		}
		else
		{
			psStats->pIMD = NULL;
		}
		//get the rest of the imd's
		if (strcmp(mountGfx, "0"))
		{
			psStats->pMountGraphic = (iIMDShape *) resGetData("IMD", mountGfx);
			if (psStats->pMountGraphic == NULL)
			{
				debug( LOG_FATAL, "Cannot find the mount PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}
		}
		else
		{
			psStats->pMountGraphic = NULL;
		}

		if(GetGameMode() == GS_NORMAL)
		{
			psStats->pMuzzleGraphic = (iIMDShape *) resGetData("IMD", muzzleGfx);
			if (psStats->pMuzzleGraphic == NULL)
			{
				debug( LOG_FATAL, "Cannot find the muzzle PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}


			psStats->pInFlightGraphic = (iIMDShape *) resGetData("IMD", flightGfx);
			if (psStats->pInFlightGraphic == NULL)
			{
				debug( LOG_FATAL, "Cannot find the flight PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}

			psStats->pTargetHitGraphic = (iIMDShape *) resGetData("IMD", hitGfx);
			if (psStats->pTargetHitGraphic == NULL)
			{
				debug( LOG_FATAL, "Cannot find the target hit PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}

			psStats->pTargetMissGraphic = (iIMDShape *) resGetData("IMD", missGfx);
			if (psStats->pTargetMissGraphic == NULL)
			{
				debug( LOG_FATAL, "Cannot find the target miss PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}

			psStats->pWaterHitGraphic = (iIMDShape *) resGetData("IMD", waterGfx);
			if (psStats->pWaterHitGraphic == NULL)
			{
				debug( LOG_FATAL, "Cannot find the water hit PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}
			//trail graphic can be null
			if (strcmp(trailGfx, "0"))
			{
				psStats->pTrailGraphic = (iIMDShape *) resGetData("IMD", trailGfx);
				if (psStats->pTrailGraphic == NULL)
				{
					debug( LOG_FATAL, "Cannot find the trail PIE for record %s", getStatName(psStats) );
					abort();
					return false;
				}
			}
			else
			{
				psStats->pTrailGraphic = NULL;
			}
		}

		//set the fireOnMove
		if (!strcmp(fireOnMove, "NO"))
		{
			psStats->fireOnMove = FOM_NO;
		}
		else if (!strcmp(fireOnMove,"PARTIAL"))
		{
			psStats->fireOnMove = FOM_PARTIAL;
		}
		else if (!strcmp(fireOnMove,"YES"))
		{
			psStats->fireOnMove = FOM_YES;
		}
		else
		{
			debug( LOG_ERROR, "Invalid fire on move flag for weapon %s - assuming YES", getStatName(psStats) );
			psStats->fireOnMove = FOM_YES;
		}

		//set the weapon class
		if (!strcmp(weaponClass, "KINETIC"))
		{
			psStats->weaponClass = WC_KINETIC;
		}
		else if (!strcmp(weaponClass,"EXPLOSIVE"))
		{
			//psStats->weaponClass = WC_EXPLOSIVE;
			psStats->weaponClass = WC_KINETIC; 	// explosives were removed from release version of Warzone
		}
		else if (!strcmp(weaponClass,"HEAT"))
		{
			psStats->weaponClass = WC_HEAT;
		}
		else if (!strcmp(weaponClass,"MISC"))
		{
			//psStats->weaponClass = WC_MISC;
			psStats->weaponClass = WC_HEAT;		// removed from release version of Warzone
		}
		else
		{
			debug( LOG_ERROR, "Invalid weapon class for weapon %s - assuming KINETIC", getStatName(psStats) );
			psStats->weaponClass = WC_KINETIC;
		}

		//set the subClass
		if (!getWeaponSubClass(weaponSubClass, &psStats->weaponSubClass))
		{
			return false;
		}

		//set the movement model
		if (!getMovementModel(movement, &psStats->movementModel))
		{
			return false;
		}

		//set the weapon effect
		if (!getWeaponEffect(weaponEffect, &psStats->weaponEffect))
		{
			debug( LOG_FATAL, "loadWepaonStats: Invalid weapon effect for weapon %s", getStatName(psStats) );
			abort();
			return false;
		}

		StatsName=psStats->pName;

		//set the face Player value
		if (compareYes(facePlayer, StatsName))
		{
			psStats->facePlayer = true;
		}
		else
		{
			psStats->facePlayer = false;
		}

		//set the In flight face Player value
		if (compareYes(faceInFlight, StatsName))
		{
			psStats->faceInFlight = true;
		}
		else
		{
			psStats->faceInFlight = false;
		}

		//set the light world value
		if (compareYes(lightWorld, StatsName))
		{
			psStats->lightWorld = true;
		}
		else
		{
			psStats->lightWorld = false;
		}

		//set the effect size
		if (effectSize > UBYTE_MAX)
		{
			ASSERT( false,"loadWeaponStats: effectSize is greater than 255 for weapon %s",
				getStatName(psStats) );
			return false;
		}
		psStats->effectSize = (UBYTE)effectSize;

		//set the rotate angle
		if (rotate > UBYTE_MAX)
		{
			ASSERT( false,"loadWeaponStats: rotate is greater than 255 for weapon %s",
				getStatName(psStats) );
			return false;
		}
		psStats->rotate = (UBYTE)rotate;

		//set the minElevation
		if (minElevation > SBYTE_MAX || minElevation < SBYTE_MIN)
		{
			ASSERT( false,"loadWeaponStats: minElevation is outside of limits for weapon %s",
				getStatName(psStats) );
			return false;
		}
		psStats->minElevation = (SBYTE)minElevation;

		//set the maxElevation
		if (maxElevation > UBYTE_MAX)
		{
			ASSERT( false,"loadWeaponStats: maxElevation is outside of limits for weapon %s",
				getStatName(psStats) );
			return false;
		}
		psStats->maxElevation = (UBYTE)maxElevation;

		//set the surfaceAir
		if (surfaceToAir > UBYTE_MAX)
		{
			ASSERT( false, "loadWeaponStats: Surface to Air is outside of limits for weapon %s",
				getStatName(psStats) );
			return false;
		}
		if (surfaceToAir == 0)
		{
			psStats->surfaceToAir = (UBYTE)SHOOT_ON_GROUND;
		}
		else if (surfaceToAir <= 50)
		{
			psStats->surfaceToAir = (UBYTE)SHOOT_IN_AIR;
		}
		else
		{
			psStats->surfaceToAir = (UBYTE)(SHOOT_ON_GROUND | SHOOT_IN_AIR);
		}

		//set the attackRuns for VTOLs
		if (numAttackRuns > UBYTE_MAX)
		{
			ASSERT( false, "loadWeaponStats: num of attack runs is outside of limits for weapon %s",
				getStatName(psStats) );
			return false;
		}
		psStats->vtolAttackRuns = (UBYTE)numAttackRuns;

		//set design flag
		if (designable)
		{
			psStats->designable = true;
		}
		else
		{
			psStats->designable = false;
		}

		//set penetrate flag
		if (penetrate)
		{
			psStats->penetrate = true;
		}
		else
		{
			psStats->penetrate = false;
		}

		// error check the ranges
		if (psStats->flightSpeed > 0 && !proj_Direct(psStats))
		{
			longRange = (UDWORD)proj_GetLongRange(psStats);
		}
		else
		{
			longRange = UDWORD_MAX;
		}
		if (psStats->shortRange > longRange)
		{
			debug( LOG_NEVER, "%s, flight speed is too low to reach short range (max range %d)\n", WeaponName, longRange );
		}
		else if (psStats->longRange > longRange)
		{
			debug( LOG_NEVER, "%s, flight speed is too low to reach long range (max range %d)\n", WeaponName, longRange );
		}

		//set the weapon sounds to default value
		psStats->iAudioFireID = NO_SOUND;
		psStats->iAudioImpactID = NO_SOUND;

		//save the stats
		statsSetWeapon(psStats, i);

		// Set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxWeaponRange(psStats->longRange);
			setMaxWeaponDamage(psStats->damage);
			setMaxWeaponROF(weaponROF(psStats, -1));
			setMaxComponentWeight(psStats->weight);
		}

		//increment the pointer to the start of the next record
		pWeaponData = strchr(pWeaponData,'\n') + 1;
	}

	return true;
}

/*Load the Body stats from the file exported from Access*/
BOOL loadBodyStats(const char *pBodyData, UDWORD bufferSize)
{
	BODY_STATS sStats, * const psStats = &sStats;
	unsigned int NumBody = numCR(pBodyData, bufferSize);
	unsigned int i, designable;
	char BodyName[MAX_STR_LENGTH], size[MAX_STR_LENGTH],
		GfxFile[MAX_STR_LENGTH], dummy[MAX_STR_LENGTH],
		flameIMD[MAX_STR_LENGTH];
	UDWORD dummyVal;

	// Skip descriptive header
	if (strncmp(pBodyData,"Body ",5)==0)
	{
		pBodyData = strchr(pBodyData,'\n') + 1;
		NumBody--;
	}

	if (!statsAllocBody(NumBody))
	{
		return false;
	}

	for (i = 0; i < NumBody; i++)
	{
		memset(psStats, 0, sizeof(BODY_STATS));

		BodyName[0] = '\0';
		size[0] = '\0';
		GfxFile[0] = '\0';
		flameIMD[0] = '\0';

		//Watermelon:added 10 %d to store FRONT,REAR,LEFT,RIGHT,TOP,BOTTOM armour values
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pBodyData,"%[^','],%[^','],%[^','],%d,%d,%d,%d,%[^','],\
			%d,%d,%d, \
			%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%[^','],%d",
			(char*)&BodyName, (char*)&dummy, (char*)&size, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &psStats->body, (char*)&GfxFile, &dummyVal,
			&psStats->weaponSlots, &psStats->powerOutput,
			(int*)&psStats->armourValue[HIT_SIDE_FRONT][WC_KINETIC], (int*)&psStats->armourValue[HIT_SIDE_FRONT][WC_HEAT],
			(int*)&psStats->armourValue[HIT_SIDE_REAR][WC_KINETIC],  (int*)&psStats->armourValue[HIT_SIDE_REAR][WC_HEAT],
			(int*)&psStats->armourValue[HIT_SIDE_LEFT][WC_KINETIC],  (int*)&psStats->armourValue[HIT_SIDE_LEFT][WC_HEAT],
			(int*)&psStats->armourValue[HIT_SIDE_RIGHT][WC_KINETIC], (int*)&psStats->armourValue[HIT_SIDE_RIGHT][WC_HEAT],
			(int*)&psStats->armourValue[HIT_SIDE_TOP][WC_KINETIC],   (int*)&psStats->armourValue[HIT_SIDE_TOP][WC_HEAT],
			(int*)&psStats->armourValue[HIT_SIDE_BOTTOM][WC_KINETIC],(int*)&psStats->armourValue[HIT_SIDE_BOTTOM][WC_HEAT],
			(char*)&flameIMD, &designable);//, &psStats->armourValue[WC_EXPLOSIVE],
			//&psStats->armourValue[WC_MISC]);

		//allocate storage for the name
		if (!allocateStatName((BASE_STATS *)psStats, BodyName))
		{
			return false;
		}

		psStats->ref = REF_BODY_START + i;

		if (!getBodySize(size, &psStats->size))
		{
			ASSERT( false, "loadBodyStats: unknown body size for %s",
				getStatName(psStats) );
			return false;
		}

		//set design flag
		psStats->designable = (designable != 0);

		//get the IMD for the component
		if (strcmp(GfxFile, "0"))
		{
			psStats->pIMD = (iIMDShape *) resGetData("IMD", GfxFile);
			if (psStats->pIMD == NULL)
			{
				debug( LOG_FATAL, "Cannot find the body PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}
		}
		else
		{
			psStats->pIMD = NULL;
		}

		//get the flame graphic
		if (strcmp(flameIMD, "0"))
		{
			psStats->pFlameIMD = (iIMDShape *) resGetData("IMD", flameIMD);
			if (psStats->pFlameIMD == NULL)
			{
				debug( LOG_FATAL, "Cannot find the flame PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}
		}
		else
		{
			psStats->pFlameIMD = NULL;
		}

		//save the stats
		statsSetBody(psStats, i);

		//set the max stat values for the design screen
		if (psStats->designable)
		{
			//use front armour value to prevent bodyStats corrupt problems
			setMaxBodyArmour(psStats->armourValue[HIT_SIDE_FRONT][WC_KINETIC]);
			setMaxBodyArmour(psStats->armourValue[HIT_SIDE_FRONT][WC_HEAT]);
			setMaxBodyPower(psStats->powerOutput);
			setMaxBodyPoints(psStats->body);
			setMaxComponentWeight(psStats->weight);
		}

		//increment the pointer to the start of the next record
		pBodyData = strchr(pBodyData,'\n') + 1;
	}

	return true;
}

/*Load the Brain stats from the file exported from Access*/
BOOL loadBrainStats(const char *pBrainData, UDWORD bufferSize)
{
	BRAIN_STATS sStats, * const psStats = &sStats;
	const unsigned int NumBrain = numCR(pBrainData, bufferSize);
	unsigned int i = 0, weapon = 0;
	char		BrainName[MAX_STR_LENGTH], dummy[MAX_STR_LENGTH],
				weaponName[MAX_STR_LENGTH];
	UDWORD dummyVal;

	if (!statsAllocBrain(NumBrain))
	{
		return false;
	}

	for (i = 0; i < NumBrain; i++)
	{
		memset(psStats, 0, sizeof(BRAIN_STATS));

		BrainName[0] = '\0';
		weaponName[0] = '\0';
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pBrainData,"%[^','],%[^','],%d,%d,%d,%d,%d,%[^','],%d",
			(char*)&BrainName, (char*)&dummy, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &dummyVal, &dummyVal,
			(char*)&weaponName, &psStats->progCap); //, &psStats->AICap, &psStats->AISpeed);

		if (!allocateStatName((BASE_STATS *)psStats, BrainName))
		{
			return false;
		}

		psStats->ref = REF_BRAIN_START + i;

		//check weapon attached
		if (!strcmp(weaponName, "0"))
		{
			psStats->psWeaponStat = NULL;
		}
		else
		{
			weapon = getCompFromName(COMP_WEAPON, weaponName);

			//if weapon not found - error
			if (weapon == -1)
			{
				debug( LOG_FATAL, "Unable to find Weapon %s for brain %s", weaponName, BrainName );
				abort();
				return false;
			}
			else
			{
				//Weapon found, alloc this to the brain
				psStats->psWeaponStat = asWeaponStats + weapon;
			}
		}

		// All brains except ZNULLBRAIN available in design screen
		if ( strcmp( BrainName, "ZNULLBRAIN" ) == 0 )
		{
			psStats->designable = false;
		}
		else
		{
			psStats->designable = true;
		}

		//save the stats
		statsSetBrain(psStats, i);

		//increment the pointer to the start of the next record
		pBrainData = strchr(pBrainData, '\n') + 1;
	}
	return true;
}


/*returns the propulsion type based on the string name passed in */
bool getPropulsionType(const char* typeName, PROPULSION_TYPE* type)
{
	if      (strcmp(typeName, "Wheeled") == 0)
	{
		*type = PROPULSION_TYPE_WHEELED;
	}
	else if (strcmp(typeName, "Tracked") == 0)
	{
		*type = PROPULSION_TYPE_TRACKED;
	}
	else if (strcmp(typeName, "Legged") == 0)
	{
		*type = PROPULSION_TYPE_LEGGED;
	}
	else if (strcmp(typeName, "Hover") == 0)
	{
		*type = PROPULSION_TYPE_HOVER;
	}
	else if (strcmp(typeName, "Ski") == 0)
	{
		*type = PROPULSION_TYPE_SKI;
	}
	else if (strcmp(typeName, "Lift") == 0)
	{
		*type = PROPULSION_TYPE_LIFT;
	}
	else if (strcmp(typeName, "Propellor") == 0)
	{
		*type = PROPULSION_TYPE_PROPELLOR;
	}
	else if (strcmp(typeName, "Half-Tracked") == 0)
	{
		*type = PROPULSION_TYPE_HALF_TRACKED;
	}
	else if (strcmp(typeName, "Jump") == 0)
	{
		*type = PROPULSION_TYPE_JUMP;
	}
	else
	{
		debug( LOG_ERROR, "getPropulsionType: Invalid Propulsion type %s - assuming Hover", typeName );
		*type = PROPULSION_TYPE_HOVER;
	}

	return true;
}

/*Load the Propulsion stats from the file exported from Access*/
BOOL loadPropulsionStats(const char *pPropulsionData, UDWORD bufferSize)
{
	const unsigned int NumPropulsion = numCR(pPropulsionData, bufferSize);
	PROPULSION_STATS	sStats, * const psStats = &sStats;
	unsigned int i = 0, designable;
	char				PropulsionName[MAX_STR_LENGTH], imdName[MAX_STR_LENGTH],
						dummy[MAX_STR_LENGTH], type[MAX_STR_LENGTH];
	UDWORD dummyVal;

	if (!statsAllocPropulsion(NumPropulsion))
	{
		return false;
	}

	for (i = 0; i < NumPropulsion; i++)
	{
		memset(psStats, 0, sizeof(PROPULSION_STATS));

		PropulsionName[0] = '\0';
		imdName[0] = '\0';

		//read the data into the storage - the data is delimeted using comma's
		sscanf(pPropulsionData,"%[^','],%[^','],%d,%d,%d,%d,%d,%d,%[^','],\
			%[^','],%d,%d",
			(char*)&PropulsionName, (char*)&dummy, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &dummyVal, &dummyVal,
			&psStats->body,	(char*)&imdName, (char*)&type, &psStats->maxSpeed, &designable);

		if (!allocateStatName((BASE_STATS *)psStats, PropulsionName))
		{
			return false;
		}

		psStats->ref = REF_PROPULSION_START + i;

		//set design flag
		psStats->designable = (designable != 0);

		//deal with imd - stored so that got something to see in Design Screen!
		if (strcmp(imdName, "0"))
		{
			psStats->pIMD = (iIMDShape *) resGetData("IMD", imdName);
			if (psStats->pIMD == NULL)
			{
				debug( LOG_FATAL, "Cannot find the propulsion PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}
		}
		else
		{
			psStats->pIMD = NULL;
		}

		//set up the stats type
		if (!getPropulsionType(type, &psStats->propulsionType))
		{
			debug( LOG_FATAL, "loadPropulsionStats: Invalid Propulsion type for %s", getStatName(psStats) );
			abort();
			return false;
		}

		//save the stats
		statsSetPropulsion(psStats, i);

		//set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxPropulsionSpeed(psStats->maxSpeed);
			//setMaxComponentWeight(psStats->weight);
		}

		//increment the pointer to the start of the next record
		pPropulsionData = strchr(pPropulsionData,'\n') + 1;
	}

	/*since propulsion weight is a multiple of body weight we may need to
	adjust the max component weight value*/
	//check we've loaded them both in
	if (asBodyStats && asPropulsionStats)
	{
		//check against each body stat
		for (i = 0; i < numBodyStats; i++)
		{
			//check stat is designable
			if (asBodyStats[i].designable)
			{
				unsigned int j = 0;
				//check against each propulsion stat
				for (j = 0; j < numPropulsionStats; j++)
				{
					//check stat is designable
					if (asPropulsionStats[j].designable)
					{
						setMaxComponentWeight(asPropulsionStats[j].weight * asBodyStats[i].weight / 100);
					}
				}
			}
		}
	}

	return true;
}

/*Load the Sensor stats from the file exported from Access*/
BOOL loadSensorStats(const char *pSensorData, UDWORD bufferSize)
{
	unsigned int NumSensor = numCR(pSensorData, bufferSize);
	SENSOR_STATS sStats, * const psStats = &sStats;
	unsigned int i = 0, designable;
	char			SensorName[MAX_STR_LENGTH], location[MAX_STR_LENGTH],
					GfxFile[MAX_STR_LENGTH],type[MAX_STR_LENGTH];
	char			mountGfx[MAX_STR_LENGTH], dummy[MAX_STR_LENGTH];
	UDWORD dummyVal;

	// Skip descriptive header
	if (strncmp(pSensorData,"Sensor ",7)==0)
	{
		pSensorData = strchr(pSensorData,'\n') + 1;
		NumSensor--;
	}
	
	if (!statsAllocSensor(NumSensor))
	{
		return false;
	}

	for (i = 0; i < NumSensor; i++)
	{
		memset(psStats, 0, sizeof(SENSOR_STATS));

		SensorName[0] = '\0';
		GfxFile[0] = '\0';
		mountGfx[0] = '\0';
		location[0] = '\0';
		type[0] = '\0';
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pSensorData,"%[^','],%[^','],%d,%d,%d,%d,%d,%d,%[^','],\
			%[^','],%d,%[^','],%[^','],%d,%d,%d",
			(char*)&SensorName, (char*)&dummy, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &dummyVal, &dummyVal,
			&psStats->body,	(char*)&GfxFile,(char*)&mountGfx,
			&psStats->range, (char*)&location, (char*)&type, &psStats->time, &psStats->power, &designable);

		if (!allocateStatName((BASE_STATS *)psStats, SensorName))
		{
			return false;
		}

		psStats->ref = REF_SENSOR_START + i;

		if (!strcmp(location,"DEFAULT"))
		{
			psStats->location = LOC_DEFAULT;
		}
		else if(!strcmp(location, "TURRET"))
		{
			psStats->location = LOC_TURRET;
		}
		else
		{
			ASSERT( false, "Invalid Sensor location" );
		}

		if (!strcmp(type,"STANDARD"))
		{
			psStats->type = STANDARD_SENSOR;
		}
		else if(!strcmp(type, "INDIRECT CB"))
		{
			psStats->type = INDIRECT_CB_SENSOR;
		}
		else if(!strcmp(type, "VTOL CB"))
		{
			psStats->type = VTOL_CB_SENSOR;
		}
		else if(!strcmp(type, "VTOL INTERCEPT"))
		{
			psStats->type = VTOL_INTERCEPT_SENSOR;
		}
		else if(!strcmp(type, "SUPER"))
		{
			psStats->type = SUPER_SENSOR;
		}
		else if (!strcmp(type, "RADAR DETECTOR"))
		{
			psStats->type = RADAR_DETECTOR_SENSOR;
		}
		else
		{
			ASSERT( false, "Invalid Sensor type" );
		}

		//multiply time stats
		psStats->time *= WEAPON_TIME;

		//set design flag
		psStats->designable = (designable != 0);

		//get the IMD for the component
		if (strcmp(GfxFile, "0"))
		{
			psStats->pIMD = (iIMDShape *) resGetData("IMD", GfxFile);
			if (psStats->pIMD == NULL)
			{
				debug( LOG_FATAL, "Cannot find the sensor PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}
		}
		else
		{
			psStats->pIMD = NULL;
		}

		if (strcmp(mountGfx, "0"))
		{
			psStats->pMountGraphic = (iIMDShape *) resGetData("IMD", mountGfx);
			if (psStats->pMountGraphic == NULL)
			{
				debug( LOG_FATAL, "Cannot find the mount PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}
		}
		else
		{
			psStats->pMountGraphic = NULL;
		}

		//save the stats
		statsSetSensor(psStats, i);

 		// set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxSensorRange(psStats->range);
			setMaxSensorPower(psStats->power);
			setMaxComponentWeight(psStats->weight);
		}

		//increment the pointer to the start of the next record
		pSensorData = strchr(pSensorData,'\n') + 1;
	}
	return true;
}

/*Load the ECM stats from the file exported from Access*/
BOOL loadECMStats(const char *pECMData, UDWORD bufferSize)
{
	const unsigned int NumECM = numCR(pECMData, bufferSize);
	ECM_STATS	sStats, * const psStats = &sStats;
	unsigned int i = 0, designable;
	char		ECMName[MAX_STR_LENGTH], location[MAX_STR_LENGTH],
				GfxFile[MAX_STR_LENGTH];
	char		mountGfx[MAX_STR_LENGTH], dummy[MAX_STR_LENGTH];
	UDWORD dummyVal;

	if (!statsAllocECM(NumECM))
	{
		return false;
	}

	for (i=0; i < NumECM; i++)
	{
		memset(psStats, 0, sizeof(ECM_STATS));

		ECMName[0] = '\0';
		GfxFile[0] = '\0';
		mountGfx[0] = '\0';
		location[0] = '\0';
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pECMData,"%[^','],%[^','],%d,%d,%d,%d,%d,%d,%[^','],%[^','],\
			%[^','],%d,%d,%d",
			(char*)&ECMName, (char*)&dummy, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &dummyVal, &dummyVal,
			&psStats->body,	(char*)&GfxFile, (char*)&mountGfx, (char*)&location, &psStats->power,
			&psStats->range, &designable);

		// set a default ECM range for now
		psStats->range = TILE_UNITS * 8;

		if (!allocateStatName((BASE_STATS *)psStats, ECMName))
		{
			return false;
		}

		psStats->ref = REF_ECM_START + i;

		if (!strcmp(location,"DEFAULT"))
		{
			psStats->location = LOC_DEFAULT;
		}
		else if(!strcmp(location, "TURRET"))
		{
			psStats->location = LOC_TURRET;
		}
		else
		{
			ASSERT( false, "Invalid ECM location" );
		}

		//set design flag
		psStats->designable = (designable != 0);

		//get the IMD for the component
		if (strcmp(GfxFile, "0"))
		{
			psStats->pIMD = (iIMDShape *) resGetData("IMD", GfxFile);
			if (psStats->pIMD == NULL)
			{
				debug( LOG_FATAL, "Cannot find the ECM PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}
		}
		else
		{
			psStats->pIMD = NULL;
		}

		if (strcmp(mountGfx, "0"))
		{
			psStats->pMountGraphic = (iIMDShape *) resGetData("IMD", mountGfx);
			if (psStats->pMountGraphic == NULL)
			{
				debug( LOG_FATAL, "Cannot find the mount PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}
		}
		else
		{
			//set to NULL
			psStats->pMountGraphic = NULL;
		}

		//save the stats
		statsSetECM(psStats, i);

		// Set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxECMPower(psStats->power);
			setMaxECMRange(psStats->range);
			setMaxComponentWeight(psStats->weight);
		}

		//increment the pointer to the start of the next record
		pECMData = strchr(pECMData,'\n') + 1;
	}

	return true;
}

/*Load the Repair stats from the file exported from Access*/
BOOL loadRepairStats(const char *pRepairData, UDWORD bufferSize)
{
	const unsigned int NumRepair = numCR(pRepairData, bufferSize);
	REPAIR_STATS sStats, * const psStats = &sStats;
	unsigned int i = 0, designable, repairArmour;
	char			RepairName[MAX_STR_LENGTH], dummy[MAX_STR_LENGTH],
					GfxFile[MAX_STR_LENGTH],	mountGfx[MAX_STR_LENGTH],
					location[MAX_STR_LENGTH];
	UDWORD dummyVal;

	if (!statsAllocRepair(NumRepair))
	{
		return false;
	}

	for (i=0; i < NumRepair; i++)
	{
		memset(psStats, 0, sizeof(REPAIR_STATS));

		RepairName[0] = '\0';
		GfxFile[0] = '\0';
		mountGfx[0] = '\0';
		location[0] = '\0';

	//read the data into the storage - the data is delimeted using comma's
		sscanf(pRepairData,"%[^','],%[^','],%d,%d,%d,%d,%d,%d,%[^','],\
			%[^','],%[^','],%d,%d,%d",
			(char*)&RepairName, (char*)&dummy, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &dummyVal, &dummyVal,
			&repairArmour, (char*)&location, (char*)&GfxFile, (char*)&mountGfx,
			&psStats->repairPoints, &psStats->time,&designable);

		if (!allocateStatName((BASE_STATS *)psStats, RepairName))
		{
			return false;
		}

		psStats->ref = REF_REPAIR_START + i;

		if (!strcmp(location,"DEFAULT"))
		{
			psStats->location = LOC_DEFAULT;
		}
		else if(!strcmp(location, "TURRET"))
		{
			psStats->location = LOC_TURRET;
		}
		else
		{
			ASSERT( false, "Invalid Repair location" );
		}

		//multiply time stats
		psStats->time *= WEAPON_TIME;

		//check its not 0 since we will be dividing by it at a later stage
		if (psStats->time == 0)
		{

			ASSERT( false, "loadRepairStats: the delay time cannot be zero for %s",
				psStats->pName );

			psStats->time = 1;
		}

		psStats->repairArmour = (repairArmour != 0);

		//set design flag
		psStats->designable = (designable != 0);

		//get the IMD for the component
		if (strcmp(GfxFile, "0"))
		{
			psStats->pIMD = (iIMDShape *) resGetData("IMD", GfxFile);
			if (psStats->pIMD == NULL)
			{
				debug( LOG_FATAL, "Cannot find the Repair PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}
		}
		else
		{
			psStats->pIMD = NULL;
		}

		if (strcmp(mountGfx, "0"))
		{
			psStats->pMountGraphic = (iIMDShape *) resGetData("IMD", mountGfx);
			if (psStats->pMountGraphic == NULL)
			{
				debug( LOG_FATAL, "Cannot find the Repair mount PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}
		}
		else
		{
			//set to NULL
			psStats->pMountGraphic = NULL;
		}

		//save the stats
		statsSetRepair(psStats, i);

        //set the max stat values for the design screen
        if (psStats->designable)
        {
            setMaxRepairPoints(psStats->repairPoints);
            setMaxComponentWeight(psStats->weight);
        }

		//increment the pointer to the start of the next record
		pRepairData = strchr(pRepairData,'\n') + 1;
	}
//	free(pData);
//	free(psStats);
	return true;
}

/*Load the Construct stats from the file exported from Access*/
BOOL loadConstructStats(const char *pConstructData, UDWORD bufferSize)
{
	const unsigned int NumConstruct = numCR(pConstructData, bufferSize);
	CONSTRUCT_STATS sStats, * const psStats = &sStats;
	unsigned int i = 0, designable;
	char			ConstructName[MAX_STR_LENGTH], GfxFile[MAX_STR_LENGTH];
	char			mountGfx[MAX_STR_LENGTH], dummy[MAX_STR_LENGTH];
	UDWORD dummyVal;

	if (!statsAllocConstruct(NumConstruct))
	{
		return false;
	}

	for (i=0; i < NumConstruct; i++)
	{
		memset(psStats, 0, sizeof(CONSTRUCT_STATS));

		ConstructName[0] = '\0';
		GfxFile[0] = '\0';
		mountGfx[0] = '\0';
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pConstructData,"%[^','],%[^','],%d,%d,%d,%d,%d,%d,%[^','],\
			%[^','],%d,%d",
			(char*)&ConstructName, (char*)&dummy, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &dummyVal, &dummyVal,
			&psStats->body, (char*)&GfxFile, (char*)&mountGfx,
			&psStats->constructPoints,&designable);

		if (!allocateStatName((BASE_STATS *)psStats, ConstructName))
		{
			return false;
		}

		psStats->ref = REF_CONSTRUCT_START + i;

		//set design flag
		if (designable)
		{
			psStats->designable = true;
		}
		else
		{
			psStats->designable = false;
		}

		//get the IMD for the component
		if (strcmp(GfxFile, "0"))
		{
			psStats->pIMD = (iIMDShape *) resGetData("IMD", GfxFile);
			if (psStats->pIMD == NULL)
			{
				debug( LOG_FATAL, "Cannot find the constructor PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}
		}
		else
		{
			psStats->pIMD = NULL;
		}

		if (strcmp(mountGfx, "0"))
		{
			psStats->pMountGraphic = (iIMDShape *) resGetData("IMD", mountGfx);
			if (psStats->pMountGraphic == NULL)
			{
				debug( LOG_FATAL, "Cannot find the mount PIE for record %s", getStatName(psStats) );
				abort();
				return false;
			}
		}
		else
		{
			//set to NULL
			psStats->pMountGraphic = NULL;
		}

		//save the stats
		statsSetConstruct(psStats, i);

		// Set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxConstPoints(psStats->constructPoints);
			setMaxComponentWeight(psStats->weight);
		}

		//increment the pointer to the start of the next record
		pConstructData = strchr(pConstructData,'\n') + 1;
	}

	return true;
}


/*Load the Propulsion Types from the file exported from Access*/
BOOL loadPropulsionTypes(const char *pPropTypeData, UDWORD bufferSize)
{
	const unsigned int NumTypes = PROPULSION_TYPE_NUM;
	PROPULSION_TYPES *pPropType;
	unsigned int i, multiplier;
	PROPULSION_TYPE type;
	char PropulsionName[MAX_STR_LENGTH], flightName[MAX_STR_LENGTH];

	//allocate storage for the stats
	asPropulsionTypes = (PROPULSION_TYPES *)malloc(sizeof(PROPULSION_TYPES)*NumTypes);
	if (asPropulsionTypes == NULL)
	{
		debug( LOG_FATAL, "PropulsionTypes - Out of memory" );
		abort();
		return false;
	}

	memset(asPropulsionTypes, 0, (sizeof(PROPULSION_TYPES)*NumTypes));

	for (i=0; i < NumTypes; i++)
	{
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pPropTypeData,"%[^','],%[^','],%d",
			(char*)&PropulsionName, (char*)&flightName, &multiplier);

		//set the pointer for this record based on the name
		if (!getPropulsionType(PropulsionName, &type))
		{
			debug( LOG_FATAL, "loadPropulsionTypes: Invalid Propulsion type - %s", PropulsionName );
			abort();
			return false;
		}

		pPropType = asPropulsionTypes + type;

		if (!strcmp(flightName, "GROUND"))
		{
			pPropType->travel = GROUND;
		}
		else if (!strcmp(flightName, "AIR"))
		{
			pPropType->travel = AIR;
		}
		else
		{
			ASSERT( false, "Invalid travel type for Propulsion" );
		}

        //don't care about this anymore! AB FRIDAY 13/11/98
        //want it back again! AB 27/11/98
        if (multiplier > UWORD_MAX)
        {
            ASSERT( false, "loadPropulsionTypes: power Ratio multiplier too high" );
            //set to a default value since not life threatening!
            multiplier = 100;
        }
		pPropType->powerRatioMult = (UWORD)multiplier;

		//initialise all the sound variables
		pPropType->startID = NO_SOUND;
		pPropType->idleID = NO_SOUND;
		pPropType->moveOffID = NO_SOUND;
		pPropType->moveID = NO_SOUND;
		pPropType->hissID = NO_SOUND;
		pPropType->shutDownID = NO_SOUND;

		//increment the pointer to the start of the next record
		pPropTypeData = strchr(pPropTypeData,'\n') + 1;
	}

	return true;
}


/*Load the Terrain Table from the file exported from Access*/
BOOL loadTerrainTable(const char *pTerrainTableData, UDWORD bufferSize)
{
	const unsigned int NumEntries = numCR(pTerrainTableData, bufferSize);
	unsigned int i;
	UDWORD			terrainType, propulsionType, speedFactor;

	//allocate storage for the stats
	asTerrainTable = (TERRAIN_TABLE *)malloc(sizeof(*asTerrainTable) * PROPULSION_TYPE_NUM * TER_MAX);

	if (asTerrainTable == NULL)
	{
		debug( LOG_FATAL, "Terrain Types - Out of memory" );
		abort();
		return false;
	}

	//initialise the storage to 100
	for (i = 0; i < TER_MAX; ++i)
	{
		unsigned int j;
		for (j = 0; j < PROPULSION_TYPE_NUM; j++)
		{
			TERRAIN_TABLE * const pTerrainTable = &asTerrainTable[i * PROPULSION_TYPE_NUM + j];
			pTerrainTable->speedFactor = 100;
		}
	}

	for (i = 0; i < NumEntries; ++i)
	{
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pTerrainTableData,"%d,%d,%d",
			&terrainType, &propulsionType, &speedFactor);
		//store the speed factor at the correct location from the start
		storeSpeedFactor(terrainType, propulsionType, speedFactor);
		//increment the pointer to the start of the next record
		pTerrainTableData = strchr(pTerrainTableData,'\n') + 1;
	}

	//check that none of the entries are 0 otherwise this will stop a droid dead in its tracks
	//and it will not be able to move again!
	for (i = 0; i < TER_MAX; ++i)
	{
		unsigned int j;
		for (j = 0; j < PROPULSION_TYPE_NUM; ++j)
		{
			TERRAIN_TABLE * const pTerrainTable = asTerrainTable + (i * PROPULSION_TYPE_NUM + j);
			if (pTerrainTable->speedFactor == 0)
			{
				debug( LOG_FATAL, "loadTerrainTable: Invalid propulsion/terrain table entry" );
				abort();
				return false;
			}
		}
	}

	return true;
}

/*Load the Special Ability stats from the file exported from Access*/
BOOL loadSpecialAbility(const char *pSAbilityData, UDWORD bufferSize)
{
	const unsigned int NumTypes = numCR(pSAbilityData, bufferSize);
	SPECIAL_ABILITY *pSAbility;
	unsigned int i, accessID;
	char SAbilityName[MAX_STR_LENGTH];

	//allocate storage for the stats
	asSpecialAbility = (SPECIAL_ABILITY *)malloc(sizeof(SPECIAL_ABILITY)*NumTypes);

	if (asSpecialAbility == NULL)
	{
		debug( LOG_FATAL, "SpecialAbility - Out of memory" );
		abort();
		return false;
	}

	numSpecialAbility = NumTypes;

	memset(asSpecialAbility, 0, (sizeof(SPECIAL_ABILITY)*NumTypes));

	//copy the start location
	pSAbility = asSpecialAbility;

	for (i=0; i < NumTypes; i++)
	{
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pSAbilityData,"%[^','],%d",(char*)&SAbilityName, &accessID);
		//check that the data is ordered in the way it will be stored
		if (accessID != i)
		{
			debug( LOG_FATAL, "The Special Ability sequence is invalid" );
			abort();
			return false;
		}
		//allocate storage for the name
		asSpecialAbility->pName = (char *)malloc((strlen(SAbilityName))+1);
		if (asSpecialAbility->pName == NULL)
		{
			debug( LOG_FATAL, "Special Ability Name - Out of memory" );
			abort();
			return false;
		}
		strcpy(asSpecialAbility->pName,SAbilityName);

		//increment the pointer to the start of the next record
		pSAbilityData = strchr(pSAbilityData,'\n') + 1;
		asSpecialAbility++;
	}

	//reset the pointer to the start of the special ability stats
	asSpecialAbility = pSAbility;
	return true;
}

/* load the IMDs to use for each body-propulsion combination */
BOOL loadBodyPropulsionIMDs(const char *pData, UDWORD bufferSize)
{
	const unsigned int NumTypes = numCR(pData, bufferSize);
	BODY_STATS *psBodyStat = asBodyStats;
	PROPULSION_STATS *psPropulsionStat = asPropulsionStats;
	unsigned int i, numStats;
	char				bodyName[MAX_STR_LENGTH], propulsionName[MAX_STR_LENGTH],
						leftIMD[MAX_STR_LENGTH], rightIMD[MAX_STR_LENGTH];
	iIMDShape			**startIMDs;
	BOOL				found;

	//check that the body and propulsion stats have already been read in

	ASSERT( asBodyStats != NULL, "Body Stats have not been set up" );
	ASSERT( asPropulsionStats != NULL, "Propulsion Stats have not been set up" );

	//allocate space
	for (numStats = 0; numStats < numBodyStats; numStats++)
	{
		psBodyStat = &asBodyStats[numStats];
		psBodyStat->ppIMDList = (iIMDShape **) malloc(numPropulsionStats * NUM_PROP_SIDES * sizeof(iIMDShape *));
		if (psBodyStat->ppIMDList == NULL)
		{
			debug( LOG_FATAL, "Out of memory" );
			abort();
		}
		//initialise the pointer space
		memset(psBodyStat->ppIMDList, 0, (numPropulsionStats *
			NUM_PROP_SIDES * sizeof(iIMDShape *)));
	}



	for (i=0; i < NumTypes; i++)
	{
		bodyName[0] = '\0';
		propulsionName[0] = '\0';
		leftIMD[0] = '\0';
		rightIMD[0] = '\0';

		/*read the data into the storage - the data is delimited using comma's
		not interested in the last number - needed for sscanf*/
		sscanf(pData,"%[^','],%[^','],%[^','],%[^','],%*d",(char*)&bodyName,
            (char*)&propulsionName, (char*)&leftIMD, (char*)&rightIMD);

		//get the body stats
		found = false;

		for (numStats = 0; numStats < numBodyStats; numStats++)
		{
			psBodyStat = &asBodyStats[numStats];
			if (!strcmp(psBodyStat->pName, bodyName))
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			debug( LOG_FATAL, "loadBodyPropulsionPIEs: Invalid body name %s", bodyName );
			abort();
			return false;
		}

		//get the propulsion stats
		found = false;

		for (numStats = 0; numStats < numPropulsionStats; numStats++)
		{
			psPropulsionStat = &asPropulsionStats[numStats];
			if (!strcmp(psPropulsionStat->pName, propulsionName))
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			debug( LOG_FATAL, "Invalid propulsion name %s", propulsionName );
			abort();
			return false;
		}

		//allocate the left and right propulsion IMDs
		startIMDs = psBodyStat->ppIMDList;
		psBodyStat->ppIMDList += (numStats * NUM_PROP_SIDES);
		if (strcmp(leftIMD, "0"))
		{
			*psBodyStat->ppIMDList = (iIMDShape *) resGetData("IMD", leftIMD);
			if (*psBodyStat->ppIMDList == NULL)
			{
				debug( LOG_FATAL, "Cannot find the left propulsion PIE for body %s", bodyName );
				abort();
				return false;
			}
		}
		else
		{
			*psBodyStat->ppIMDList = NULL;
		}

		psBodyStat->ppIMDList++;
		//right IMD might not be there
		if (strcmp(rightIMD, "0"))
		{
			*psBodyStat->ppIMDList = (iIMDShape *) resGetData("IMD", rightIMD);
			if (*psBodyStat->ppIMDList == NULL)
			{
				debug( LOG_FATAL, "Cannot find the right propulsion PIE for body %s", bodyName );
				abort();
				return false;
			}
		}
		else
		{
			*psBodyStat->ppIMDList = NULL;
		}

		//reset the IMDList pointer
		psBodyStat->ppIMDList = startIMDs;

		//increment the pointer to the start of the next record
		pData = strchr(pData,'\n') + 1;
	}

	return(true);
}



static BOOL
statsGetAudioIDFromString( char *szStatName, char *szWavName, SDWORD *piWavID )
{
	if ( strcmp( szWavName, "-1" ) == 0 )
	{
		*piWavID = NO_SOUND;
	}
	else
	{
		if ( (*piWavID = audio_GetIDFromStr(szWavName)) == NO_SOUND )
		{
			debug( LOG_FATAL, "statsGetAudioIDFromString: couldn't get ID %d for sound %s", *piWavID, szWavName );
			abort();
			return false;
		}
	}
	if ((*piWavID < 0
	  || *piWavID > ID_MAX_SOUND)
	 && *piWavID != NO_SOUND)
	{
		debug( LOG_FATAL, "statsGetAudioIDFromString: Invalid ID - %d for sound %s", *piWavID, szStatName );
		abort();
		return false;
	}

	return true;
}



/*Load the weapon sounds from the file exported from Access*/
BOOL loadWeaponSounds(const char *pSoundData, UDWORD bufferSize)
{
	const unsigned int NumRecords = numCR(pSoundData, bufferSize);
	SDWORD i, weaponSoundID, explosionSoundID, inc, iDum;
	char			WeaponName[MAX_STR_LENGTH];
	char			szWeaponWav[MAX_STR_LENGTH],	szExplosionWav[MAX_STR_LENGTH];

	ASSERT( asWeaponStats != NULL, "loadWeaponSounds: Weapon stats not loaded" );

	for (i=0; i < NumRecords; i++)
	{
		WeaponName[0]     = '\0';
		szWeaponWav[0]    = '\0';
		szExplosionWav[0] = '\0';
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pSoundData,"%[^','],%[^','],%[^','],%d",
			WeaponName, szWeaponWav, szExplosionWav, &iDum);

		if ( statsGetAudioIDFromString( WeaponName, szWeaponWav, &weaponSoundID ) == false )
		{
			return false;
		}

		if ( statsGetAudioIDFromString( WeaponName, szExplosionWav, &explosionSoundID ) == false )
		{
			return false;
		}

		for (inc = 0; inc < (SDWORD)numWeaponStats; inc++)
		{
			if (!strcmp(asWeaponStats[inc].pName, WeaponName))
			{
				asWeaponStats[inc].iAudioFireID = weaponSoundID;
				asWeaponStats[inc].iAudioImpactID = explosionSoundID;
				break;
			}
		}
		ASSERT_OR_RETURN(false, inc != (SDWORD)numWeaponStats, "Weapon stat not found - %s", WeaponName);

		//increment the pointer to the start of the next record
		pSoundData = strchr(pSoundData,'\n') + 1;
	}

	return true;
}

/*Load the Weapon Effect Modifiers from the file exported from Access*/
BOOL loadWeaponModifiers(const char *pWeapModData, UDWORD bufferSize)
{
	const unsigned int NumRecords = numCR(pWeapModData, bufferSize);
	PROPULSION_TYPE		propInc;
	WEAPON_EFFECT		effectInc;
	UDWORD				i, j, modifier;
	char				weaponEffectName[MAX_STR_LENGTH], propulsionName[MAX_STR_LENGTH];

	//initialise to 100%
	for (i=0; i < WE_NUMEFFECTS; i++)
	{
		for (j=0; j < PROPULSION_TYPE_NUM; j++)
		{
			asWeaponModifier[i][j] = 100;
		}
	}

	for (i=0; i < NumRecords; i++)
	{
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pWeapModData,"%[^','],%[^','],%d",
			(char*)&weaponEffectName, (char*)&propulsionName, &modifier);

		//get the weapon effect inc
		if (!getWeaponEffect(weaponEffectName, &effectInc))
		{
			debug( LOG_FATAL, "loadWeaponModifiers: Invalid Weapon Effect - %s", weaponEffectName );
			abort();
			return false;
		}
		//get the propulsion inc
		if (!getPropulsionType(propulsionName, &propInc))
		{
			debug( LOG_FATAL, "loadWeaponModifiers: Invalid Propulsion type - %s", propulsionName );
			abort();
			return false;
		}

		if (modifier > UWORD_MAX)
		{
			debug( LOG_FATAL, "loadWeaponModifiers: modifier for effect %s, prop type %s is too large", weaponEffectName, propulsionName );
			abort();
			return false;
		}
		//store in the appropriate index
		asWeaponModifier[effectInc][propInc] = (UWORD)modifier;

		//increment the pointer to the start of the next record
		pWeapModData = strchr(pWeapModData,'\n') + 1;
	}

	return true;
}

/*Load the propulsion type sounds from the file exported from Access*/
BOOL loadPropulsionSounds(const char *pPropSoundData, UDWORD bufferSize)
{
	const unsigned int NumRecords = numCR(pPropSoundData, bufferSize);
	SDWORD				i, startID, idleID, moveOffID, moveID,
						hissID, shutDownID, iDum;
	char				propulsionName[MAX_STR_LENGTH], szStart[MAX_STR_LENGTH],
						szIdle[MAX_STR_LENGTH], szMoveOff[MAX_STR_LENGTH],
						szMove[MAX_STR_LENGTH], szHiss[MAX_STR_LENGTH],
						szShutDown[MAX_STR_LENGTH];
	PROPULSION_TYPE type;
	PROPULSION_TYPES	*pPropType;


	ASSERT( asPropulsionTypes != NULL,
		"loadPropulsionSounds: Propulsion type stats not loaded" );

	for (i=0; i < NumRecords; i++)
	{

		propulsionName[0] = '\0';
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pPropSoundData,"%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%d",
			propulsionName, szStart, szIdle, szMoveOff, szMove, szHiss, szShutDown, &iDum);

		if ( statsGetAudioIDFromString( propulsionName, szStart, &startID ) == false )
		{
			return false;
		}

		if ( statsGetAudioIDFromString( propulsionName, szIdle, &idleID ) == false )
		{
			return false;
		}

		if ( statsGetAudioIDFromString( propulsionName, szMoveOff, &moveOffID ) == false )
		{
			return false;
		}

		if ( statsGetAudioIDFromString( propulsionName, szMove, &moveID ) == false )
		{
			return false;
		}

		if ( statsGetAudioIDFromString( propulsionName, szHiss, &hissID ) == false )
		{
			return false;
		}

		if ( statsGetAudioIDFromString( propulsionName, szShutDown, &shutDownID ) == false )
		{
			return false;
		}

		if (!getPropulsionType(propulsionName, &type))
		{
			debug( LOG_FATAL, "loadPropulsionSounds: Invalid Propulsion type - %s", propulsionName );
			abort();
			return false;
		}
		pPropType = asPropulsionTypes + type;
		pPropType->startID = (SWORD)startID;
		pPropType->idleID = (SWORD)idleID;
		pPropType->moveOffID = (SWORD)moveOffID;
		pPropType->moveID = (SWORD)moveID;
		pPropType->hissID = (SWORD)hissID;
		pPropType->shutDownID = (SWORD)shutDownID;

		//increment the pointer to the start of the next record
		pPropSoundData = strchr(pPropSoundData,'\n') + 1;
	}

	return(true);
}

/*******************************************************************************
*		Set stats functions
*******************************************************************************/
/* Set the stats for a particular weapon type */
void statsSetWeapon(WEAPON_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asWeaponStats, index, WEAPON_STATS, REF_WEAPON_START);
}
/* Set the stats for a particular body type */
void statsSetBody(BODY_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asBodyStats, index, BODY_STATS, REF_BODY_START);
}
/* Set the stats for a particular brain type */
void statsSetBrain(BRAIN_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asBrainStats, index, BRAIN_STATS, REF_BRAIN_START);
}
/* Set the stats for a particular power type */
void statsSetPropulsion(PROPULSION_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asPropulsionStats, index, PROPULSION_STATS,
		REF_PROPULSION_START);
}
/* Set the stats for a particular sensor type */
void statsSetSensor(SENSOR_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asSensorStats, index, SENSOR_STATS, REF_SENSOR_START);
}
/* Set the stats for a particular ecm type */
void statsSetECM(ECM_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asECMStats, index, ECM_STATS, REF_ECM_START);
}
/* Set the stats for a particular repair type */
void statsSetRepair(REPAIR_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asRepairStats, index, REPAIR_STATS, REF_REPAIR_START);
}
/* Set the stats for a particular construct type */
void statsSetConstruct(CONSTRUCT_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asConstructStats, index, CONSTRUCT_STATS, REF_CONSTRUCT_START);
}

/*******************************************************************************
*		Get stats functions
*******************************************************************************/
WEAPON_STATS *statsGetWeapon(UDWORD ref)
{
	UDWORD index;
	ASSERT( (ref >= REF_WEAPON_START) && (ref < REF_WEAPON_START + REF_RANGE),
		"statsGetWeapon: Invalid reference number: %x", ref );

	for (index = 0; index < numWeaponStats; index++)
	{
		if (asWeaponStats[index].ref == ref)
		{
			return &asWeaponStats[index];
		}
	}
	ASSERT( false, "statsGetWeapon: Reference number not found in list: %x", ref );
	return NULL;	// should never get here, but this stops the compiler complaining.
}

BODY_STATS *statsGetBody(UDWORD ref)
{
	UDWORD index;
	ASSERT( (ref >= REF_BODY_START) && (ref < REF_BODY_START + REF_RANGE),
		"statsGetBody: Invalid reference number: %x", ref );

	for (index = 0; index < numBodyStats; index++)
	{
		if (asBodyStats[index].ref == ref)
		{
			return &asBodyStats[index];
		}
	}
	ASSERT( false, "statsGetBody: Reference number not found in list: %x", ref );
	return NULL;	// should never get here, but this stops the compiler complaining.
}

BRAIN_STATS *statsGetBrain(UDWORD ref)
{
	UDWORD index;
	ASSERT( (ref >= REF_BRAIN_START) && (ref < REF_BRAIN_START + REF_RANGE),
		"statsGetBrain: Invalid reference number: %x", ref );

	for (index = 0; index < numBrainStats; index++)
	{
		if (asBrainStats[index].ref == ref)
		{
			return &asBrainStats[index];
		}
	}
	ASSERT( false, "statsGetBrain: Reference number not found in list: %x", ref );
	return NULL;	// should never get here, but this stops the compiler complaining.
}

PROPULSION_STATS *statsGetPropulsion(UDWORD ref)
{
	UDWORD index;
	ASSERT( (ref >= REF_PROPULSION_START) && (ref < REF_PROPULSION_START +
		REF_RANGE),
		"statsGetPropulsion: Invalid reference number: %x", ref );

	for (index = 0; index < numPropulsionStats; index++)
	{
		if (asPropulsionStats[index].ref == ref)
		{
			return &asPropulsionStats[index];
		}
	}
	ASSERT( false, "statsGetPropulsion: Reference number not found in list: %x", ref );
	return NULL;	// should never get here, but this stops the compiler complaining.
}

SENSOR_STATS *statsGetSensor(UDWORD ref)
{
	UDWORD index;
	ASSERT( (ref >= REF_SENSOR_START) && (ref < REF_SENSOR_START + REF_RANGE),
		"statsGetSensor: Invalid reference number: %x", ref );

	for (index = 0; index < numSensorStats; index++)
	{
		if (asSensorStats[index].ref == ref)
		{
			return &asSensorStats[index];
		}
	}
	ASSERT( false, "statsGetSensor: Reference number not found in list: %x", ref );
	return NULL;	// should never get here, but this stops the compiler complaining.
}

ECM_STATS *statsGetECM(UDWORD ref)
{
	UDWORD index;
	ASSERT( (ref >= REF_ECM_START) && (ref < REF_ECM_START + REF_RANGE),
		"statsGetECM: Invalid reference number: %x", ref );

	for (index = 0; index < numECMStats; index++)
	{
		if (asECMStats[index].ref == ref)
		{
			return &asECMStats[index];
		}
	}
	ASSERT( false, "statsGetECM: Reference number not found in list: %x", ref );
	return NULL;	// should never get here, but this stops the compiler complaining.
}

REPAIR_STATS *statsGetRepair(UDWORD ref)
{
	UDWORD index;
	ASSERT( (ref >= REF_REPAIR_START) && (ref < REF_REPAIR_START + REF_RANGE),
		"statsGetRepair: Invalid reference number: %x", ref );

	for (index = 0; index < numRepairStats; index++)
	{
		if (asRepairStats[index].ref == ref)
		{
			return &asRepairStats[index];
		}
	}
	ASSERT( false, "statsGetRepair: Reference number not found in list: %x", ref );
	return NULL;	// should never get here, but this stops the compiler complaining.
}

CONSTRUCT_STATS *statsGetConstruct(UDWORD ref)
{
	UDWORD index;
	ASSERT( (ref >= REF_CONSTRUCT_START) && (ref < REF_CONSTRUCT_START + REF_RANGE),
		"statsGetConstruct: Invalid reference number: %x", ref );

	for (index = 0; index < numConstructStats; index++)
	{
		if (asConstructStats[index].ref == ref)
		{
			return &asConstructStats[index];
		}
	}
	ASSERT( false, "statsGetConstruct: Reference number not found in list: %x", ref );
	return NULL;	// should never get here, but this stops the compiler complaining.
}

/***********************************************************************************
*	Dealloc the extra storage tables
***********************************************************************************/
//Deallocate the storage assigned for the Propulsion Types table
void deallocPropulsionTypes(void)
{
	free(asPropulsionTypes);
	asPropulsionTypes = NULL;
}

//dealloc the storage assigned for the terrain table
void deallocTerrainTable(void)
{
	free(asTerrainTable);
	asTerrainTable = NULL;
}

//dealloc the storage assigned for the Special Ability stats
void deallocSpecialAbility(void)
{
	UBYTE inc;
	SPECIAL_ABILITY* pList = asSpecialAbility;

	for (inc=0; inc < numSpecialAbility; inc++, pList++)
	{
		free(pList->pName);
	}
	free(asSpecialAbility);
	asSpecialAbility = NULL;
}

//store the speed Factor in the terrain table
void storeSpeedFactor(UDWORD terrainType, UDWORD propulsionType, UDWORD speedFactor)
{
	TERRAIN_TABLE *pTerrainTable = asTerrainTable;

	ASSERT( propulsionType < PROPULSION_TYPE_NUM,
		"The propulsion type is too large" );

	pTerrainTable += (terrainType * PROPULSION_TYPE_NUM + propulsionType);
	pTerrainTable->speedFactor = speedFactor;
}

//get the speed factor for a given terrain type and propulsion type
UDWORD getSpeedFactor(UDWORD type, UDWORD propulsionType)
{
	TERRAIN_TABLE *pTerrainTable = asTerrainTable;

	ASSERT( propulsionType < PROPULSION_TYPE_NUM,
		"The propulsion type is too large" );

	pTerrainTable += (type * PROPULSION_TYPE_NUM + propulsionType);

	return pTerrainTable->speedFactor;
}

//return the type of stat this stat is!
UDWORD statType(UDWORD ref)
{
	if (ref >=REF_BODY_START && ref < REF_BODY_START +
		REF_RANGE)
	{
		return COMP_BODY;
	}
	if (ref >=REF_BRAIN_START && ref < REF_BRAIN_START +
		REF_RANGE)
	{
		return COMP_BRAIN;
	}
	if (ref >=REF_PROPULSION_START && ref <
		REF_PROPULSION_START + REF_RANGE)
	{
		return COMP_PROPULSION;
	}
	if (ref >=REF_SENSOR_START && ref < REF_SENSOR_START +
		REF_RANGE)
	{
		return COMP_SENSOR;
	}
	if (ref >=REF_ECM_START && ref < REF_ECM_START +
		REF_RANGE)
	{
		return COMP_ECM;
	}
	if (ref >=REF_REPAIR_START && ref < REF_REPAIR_START +
		REF_RANGE)
	{
		return COMP_REPAIRUNIT;
	}
	if (ref >=REF_WEAPON_START && ref < REF_WEAPON_START +
		REF_RANGE)
	{
		return COMP_WEAPON;
	}
	if (ref >=REF_CONSTRUCT_START && ref < REF_CONSTRUCT_START +
		REF_RANGE)
	{
		return COMP_CONSTRUCT;
	}
	//else
	ASSERT( false, "Invalid stat pointer - cannot determine Stat Type" );
	return COMP_UNKNOWN;
}

//return the REF_START value of this type of stat
UDWORD statRefStart(UDWORD stat)
{
	UDWORD start;

	switch (stat)
	{
		case COMP_BODY:
		{
			start = REF_BODY_START;
			break;
		}
		case COMP_BRAIN:
		{
			start = REF_BRAIN_START;
			break;
		}
		case COMP_PROPULSION:
		{
			start = REF_PROPULSION_START;
			break;
		}
		case COMP_SENSOR:
		{
			start = REF_SENSOR_START;
			break;
		}
		case COMP_ECM:
		{
			start = REF_ECM_START;
			break;
		}
		case COMP_REPAIRUNIT:
		{
			start = REF_REPAIR_START;
			break;
		}
		case COMP_WEAPON:
		{
			start = REF_WEAPON_START;
			break;
		}
		case COMP_CONSTRUCT:
		{
			start = REF_CONSTRUCT_START;
			break;
		}
		default:
		{
			//COMP_UNKNOWN should be an error
			debug( LOG_FATAL, "Invalid stat type" );
			abort();
			start = 0;
		}
	}
	return start;
}

/*Returns the component type based on the string - used for reading in data */
unsigned int componentType(const char* pType)
{
	if (!strcmp(pType, "BODY"))
	{
		return COMP_BODY;
	}
	if (!strcmp(pType, "PROPULSION"))
	{
		return COMP_PROPULSION;
	}
	if (!strcmp(pType, "BRAIN"))
	{
		return COMP_BRAIN;
	}
	if (!strcmp(pType, "REPAIR"))
	{
		return COMP_REPAIRUNIT;
	}
	if (!strcmp(pType, "ECM"))
	{
		return COMP_ECM;
	}
	if (!strcmp(pType, "SENSOR"))
	{
		return COMP_SENSOR;
	}
	if (!strcmp(pType, "WEAPON"))
	{
		return COMP_WEAPON;
	}
	if (!strcmp(pType, "CONSTRUCT"))
	{
		return COMP_CONSTRUCT;
	}

	ASSERT( false, "Unknown Component Type" );
	return 0; // Should never get here.
}

//function to compare a value with yes/no - if neither warns player!
BOOL compareYes(const char* strToCompare, const char* strOwner)
{
	if (!strcmp(strToCompare, "YES"))
	{
		return true;
	}
	else if (!strcmp(strToCompare, "NO"))
	{
		return false;
	}
	else
	{
		//set default to false but continue
		//DBERROR(("Invalid yes/no for record %s", strOwner));
		debug( LOG_FATAL, "Invalid yes/no for record %s", getName(strOwner) );
		abort();
		return false;
	}
}

//get the component Inc for a stat based on the Resource name and type
//returns -1 if record not found
//used in Scripts
SDWORD	getCompFromResName(UDWORD compType, const char *pName)
{
	return getCompFromName(compType, pName);
}

static void getStatsDetails(UDWORD compType, BASE_STATS **ppsStats, UDWORD *pnumStats, UDWORD *pstatSize)
{

	switch (compType)
	{
	case COMP_BODY:
		*ppsStats = (BASE_STATS*)asBodyStats;
		*pnumStats = numBodyStats;
		*pstatSize = sizeof(BODY_STATS);
		break;
	case COMP_BRAIN:
		*ppsStats = (BASE_STATS*)asBrainStats;
		*pnumStats = numBrainStats;
		*pstatSize = sizeof(BRAIN_STATS);
		break;
	case COMP_PROPULSION:
		*ppsStats = (BASE_STATS*)asPropulsionStats;
		*pnumStats = numPropulsionStats;
		*pstatSize = sizeof(PROPULSION_STATS);
		break;
	case COMP_REPAIRUNIT:
		*ppsStats = (BASE_STATS*)asRepairStats;
		*pnumStats = numRepairStats;
		*pstatSize = sizeof(REPAIR_STATS);
		break;
	case COMP_ECM:
		*ppsStats = (BASE_STATS*)asECMStats;
		*pnumStats = numECMStats;
		*pstatSize = sizeof(ECM_STATS);
		break;
	case COMP_SENSOR:
		*ppsStats = (BASE_STATS*)asSensorStats;
		*pnumStats = numSensorStats;
		*pstatSize = sizeof(SENSOR_STATS);
		break;
	case COMP_CONSTRUCT:
		*ppsStats = (BASE_STATS*)asConstructStats;
		*pnumStats = numConstructStats;
		*pstatSize = sizeof(CONSTRUCT_STATS);
		break;
	case COMP_WEAPON:
		*ppsStats = (BASE_STATS*)asWeaponStats;
		*pnumStats = numWeaponStats;
		*pstatSize = sizeof(WEAPON_STATS);
		break;
	default:
		//COMP_UNKNOWN should be an error
		debug( LOG_FATAL, "Invalid component type - game.c" );
		abort();
	}
}


//get the component Inc for a stat based on the name and type
//returns -1 if record not found
SDWORD getCompFromName(UDWORD compType, const char *pName)
{
	BASE_STATS	*psStats = NULL;
	UDWORD		numStats = 0, count, statSize = 0;

	getStatsDetails(compType, &psStats,&numStats,&statSize);

	//find the stat with the same name
	for(count = 0; count < numStats; count++)
	{
		if (!strcmp(pName, psStats->pName))
		{
			return count;
		}
		psStats = (BASE_STATS*)((char*)psStats + statSize);
	}

	//return -1 if record not found or an invalid component type is passed in
	return -1;
}

/*return the name to display for the interface - valid for OBJECTS and STATS*/
const char* getName(const char *pNameID)
{
	/* See if the name has a string resource associated with it by trying
	 * to get the string resource.
	 */
	const char * const name = strresGetString(psStringRes, pNameID);
	if (!name)
	{
		debug( LOG_ERROR, "Unable to find string resource for %s", pNameID );
		return "Name Unknown";
	}

	return name;
}

/*sets the store to the body size based on the name passed in - returns false
if doesn't compare with any*/
BOOL getBodySize(const char *pSize, UBYTE *pStore)
{
	if (!strcmp(pSize, "LIGHT"))
	{
		*pStore = SIZE_LIGHT;
		return true;
	}
	else if (!strcmp(pSize, "MEDIUM"))
	{
		*pStore = SIZE_MEDIUM;
		return true;
	}
	else if (!strcmp(pSize, "HEAVY"))
	{
		*pStore = SIZE_HEAVY;
		return true;
	}
	else if (!strcmp(pSize, "SUPER HEAVY"))
	{
		*pStore = SIZE_SUPER_HEAVY;
		return true;
	}

	ASSERT( false, "Invalid size - %s", pSize );
	return false;
}

/*returns the weapon sub class based on the string name passed in */
bool getWeaponSubClass(const char* subClass, WEAPON_SUBCLASS* wclass)
{
	if      (strcmp(subClass, "CANNON") == 0)
	{
		*wclass = WSC_CANNON;
	}
	else if (strcmp(subClass, "MORTARS") == 0)
	{
		*wclass = WSC_MORTARS;
	}
	else if (strcmp(subClass, "MISSILE") == 0)
	{
		*wclass = WSC_MISSILE;
	}
	else if (strcmp(subClass, "ROCKET") == 0)
	{
		*wclass = WSC_ROCKET;
	}
	else if (strcmp(subClass, "ENERGY") == 0)
	{
		*wclass = WSC_ENERGY;
	}
	else if (strcmp(subClass, "GAUSS") == 0)
	{
		*wclass = WSC_GAUSS;
	}
	else if (strcmp(subClass, "FLAME") == 0)
	{
		*wclass = WSC_FLAME;
	}
	else if (strcmp(subClass, "HOWITZERS") == 0)
	{
		*wclass = WSC_HOWITZERS;
	}
	else if (strcmp(subClass, "MACHINE GUN") == 0)
	{
		*wclass = WSC_MGUN;
	}
	else if (strcmp(subClass, "ELECTRONIC") == 0)
	{
		*wclass = WSC_ELECTRONIC;
	}
	else if (strcmp(subClass, "A-A GUN") == 0)
	{
		*wclass = WSC_AAGUN;
	}
	else if (strcmp(subClass, "SLOW MISSILE") == 0)
	{
		*wclass = WSC_SLOWMISSILE;
	}
	else if (strcmp(subClass, "SLOW ROCKET") == 0)
	{
		*wclass = WSC_SLOWROCKET;
	}
	else if (strcmp(subClass, "LAS_SAT") == 0)
	{
		*wclass = WSC_LAS_SAT;
	}
	else if (strcmp(subClass, "BOMB") == 0)
	{
		*wclass = WSC_BOMB;
	}
	else if (strcmp(subClass, "COMMAND") == 0)
	{
		*wclass = WSC_COMMAND;
	}
	else if (strcmp(subClass, "EMP") == 0)
	{
		*wclass = WSC_EMP;
	}
        else
        {
	    ASSERT(!"Invalid weapon sub class", "Invalid weapon sub class: %s", subClass);
            return false;
        }

	return true;
}

/*returns the movement model based on the string name passed in */
bool getMovementModel(const char* movementModel, MOVEMENT_MODEL* model)
{
	if (strcmp(movementModel,"DIRECT") == 0)
	{
		*model = MM_DIRECT;
	}
	else if (strcmp(movementModel,"INDIRECT") == 0)
	{
		*model = MM_INDIRECT;
	}
	else if (strcmp(movementModel,"HOMING-DIRECT") == 0)
	{
		*model = MM_HOMINGDIRECT;
	}
	else if (strcmp(movementModel,"HOMING-INDIRECT") == 0)
	{
		*model = MM_HOMINGINDIRECT;
	}
	else if (strcmp(movementModel,"ERRATIC-DIRECT") == 0)
	{
		*model = MM_ERRATICDIRECT;
	}
	else if (strcmp(movementModel,"SWEEP") == 0)
	{
		*model = MM_SWEEP;
	}
	else
	{
		// We've got problem if we got here
		ASSERT(!"Invalid movement model", "Invalid movement model: %s", movementModel);
		return false;
	}

	return true;
}

bool getWeaponEffect(const char* weaponEffect, WEAPON_EFFECT* effect)
{
	if      (strcmp(weaponEffect, "ANTI PERSONNEL") == 0)
	{
		*effect = WE_ANTI_PERSONNEL;
	}
	else if (strcmp(weaponEffect, "ANTI TANK") == 0)
	{
		*effect = WE_ANTI_TANK;
	}
	else if (strcmp(weaponEffect, "BUNKER BUSTER") == 0)
	{
		*effect = WE_BUNKER_BUSTER;
	}
	else if (strcmp(weaponEffect, "ARTILLERY ROUND") == 0)
	{
		*effect = WE_ARTILLERY_ROUND;
	}
	else if (strcmp(weaponEffect, "FLAMER") == 0)
	{
		*effect = WE_FLAMER;
	}
	else if (strcmp(weaponEffect, "ANTI AIRCRAFT") == 0 || strcmp(weaponEffect, "ALL ROUNDER") == 0)
	{
		*effect = WE_ANTI_AIRCRAFT;
	}
	else
	{
		ASSERT(!"Invalid weapon effect", "Invalid weapon effect: %s", weaponEffect);
		return false;
	}

	return true;
}


/*
looks up the name to get the resource associated with it - or allocates space
and stores the name. Eventually ALL names will be 'resourced' for translation
*/
char* allocateName(const char* name)
{
	char * storeName;

	/* Check whether the given string has a string resource associated with
	 * it.
	 */
	if (!strresGetString(psStringRes, name))
	{
		debug(LOG_FATAL, "Unable to find string resource for %s", name);
		abort();
		return NULL;
	}

	storeName = strdup(name);
	if (!storeName)
	{
		debug(LOG_FATAL, "Out of memory");
		abort();
		return NULL;
	}

	return storeName;
}


/*Access functions for the upgradeable stats of a weapon*/
UDWORD	weaponFirePause(const WEAPON_STATS* psStats, UBYTE player)
{
	if(psStats->reloadTime == 0)
	{
		return (psStats->firePause - (psStats->firePause * asWeaponUpgrade[player][
			psStats->weaponSubClass].firePause)/100);
	}
	else
	{
		return psStats->firePause;	//fire pause is neglectable for weapons with reload time
	}
}

/* Reload time is reduced for weapons with salvo fire */
UDWORD	weaponReloadTime(WEAPON_STATS *psStats, UBYTE player)
{
	return (psStats->reloadTime - (psStats->reloadTime * asWeaponUpgrade[player][
		psStats->weaponSubClass].firePause)/100);
}

UDWORD	weaponShortHit(const WEAPON_STATS* psStats, UBYTE player)
{
	return (psStats->shortHit + (psStats->shortHit * asWeaponUpgrade[player][
		psStats->weaponSubClass].shortHit)/100);
}

UDWORD	weaponLongHit(const WEAPON_STATS* psStats, UBYTE player)
{
	return (psStats->longHit + (psStats->longHit * asWeaponUpgrade[player][
		psStats->weaponSubClass].longHit)/100);
}

UDWORD	weaponDamage(const WEAPON_STATS* psStats, UBYTE player)
{
	return (psStats->damage + (psStats->damage * asWeaponUpgrade[player][
		psStats->weaponSubClass].damage)/100);
}

UDWORD	weaponRadDamage(WEAPON_STATS *psStats, UBYTE player)
{
	return  (psStats->radiusDamage + (psStats->radiusDamage * asWeaponUpgrade[player][
		psStats->weaponSubClass].radiusDamage)/100);
}

UDWORD	weaponIncenDamage(WEAPON_STATS *psStats, UBYTE player)
{
	return (psStats->incenDamage + (psStats->incenDamage * asWeaponUpgrade[player][
		psStats->weaponSubClass].incenDamage)/100);
}

UDWORD	weaponRadiusHit(WEAPON_STATS *psStats, UBYTE player)
{
	return (psStats->radiusHit + (psStats->radiusHit * asWeaponUpgrade[player][
		psStats->weaponSubClass].radiusHit)/100);
}

/*Access functions for the upgradeable stats of a sensor*/
UDWORD	sensorPower(SENSOR_STATS *psStats, UBYTE player)
{
	return (UWORD)(psStats->power + (psStats->power * asSensorUpgrade[player].
		power)/100);
}

UDWORD	sensorRange(SENSOR_STATS *psStats, UBYTE player)
{
	return (UWORD)(psStats->range + (psStats->range * asSensorUpgrade[player].
		range)/100);
}

/*Access functions for the upgradeable stats of a ECM*/
UDWORD	ecmPower(ECM_STATS *psStats, UBYTE player)
{
	return (UWORD)(psStats->power + (psStats->power * asECMUpgrade[player].power)/100);
}

/*Access functions for the upgradeable stats of a ECM*/
UDWORD	ecmRange(ECM_STATS *psStats, UBYTE player)
{
	return (UWORD)(psStats->range + (psStats->range * asECMUpgrade[player].range)/100);
}

/*Access functions for the upgradeable stats of a repair*/
UDWORD	repairPoints(REPAIR_STATS *psStats, UBYTE player)
{
	return (psStats->repairPoints + (psStats->repairPoints *
		asRepairUpgrade[player].repairPoints)/100);
}

/*Access functions for the upgradeable stats of a constructor*/
UDWORD	constructorPoints(CONSTRUCT_STATS *psStats, UBYTE player)
{
	return (psStats->constructPoints + (psStats->constructPoints *
		asConstUpgrade[player].constructPoints)/100);
}

/*Access functions for the upgradeable stats of a body*/
UDWORD	bodyPower(BODY_STATS *psStats, UBYTE player, UBYTE bodyType)
{
	return (psStats->powerOutput + (psStats->powerOutput *
		asBodyUpgrade[player][bodyType].powerOutput)/100);
}

UDWORD	bodyArmour(BODY_STATS *psStats, UBYTE player, UBYTE bodyType,
				   WEAPON_CLASS weaponClass, int side)
{
	switch (weaponClass)
	{
	case WC_KINETIC:
	//case WC_EXPLOSIVE:
		return (psStats->armourValue[side][WC_KINETIC] + (psStats->
			armourValue[side][WC_KINETIC] * asBodyUpgrade[player][bodyType].
			armourValue[WC_KINETIC])/100);
		break;
	case WC_HEAT:
	//case WC_MISC:
		return (psStats->armourValue[side][WC_HEAT] + (psStats->
			armourValue[side][WC_HEAT] * asBodyUpgrade[player][bodyType].
			armourValue[WC_HEAT])/100);
		break;
	default:
		break;
	}

	ASSERT( false,"bodyArmour() : Unknown weapon class" );
	return 0;	// Should never get here.
}

//calculates the weapons ROF based on the fire pause and the salvos
UWORD weaponROF(WEAPON_STATS *psStat, SBYTE player)
{
	UWORD rof = 0;

	//if there are salvos
	if (psStat->numRounds)
	{
		if (psStat->reloadTime != 0)
		{
			// Rounds per salvo multiplied with the number of salvos per minute
			rof = (UWORD)(psStat->numRounds * 60 * GAME_TICKS_PER_SEC  /
				(player >= 0 ? weaponReloadTime(psStat, player) : psStat->reloadTime) );
		}
	}
	if (rof == 0)
	{
		rof = (UWORD)weaponFirePause(psStat, (UBYTE)selectedPlayer);
		if (rof != 0)
		{
			rof = (UWORD)(60 * GAME_TICKS_PER_SEC / rof);
		}
		//else leave it at 0
	}
	return rof;
}

//Access functions for the max values to be used in the Design Screen
void setMaxComponentWeight(UDWORD weight)
{
    if (weight > maxComponentWeight)
    {
        maxComponentWeight = weight;
    }
}
UDWORD getMaxComponentWeight(void)
{
    return maxComponentWeight;
}

void setMaxBodyArmour(UDWORD armour)
{
    if (armour > maxBodyArmour)
    {
        maxBodyArmour = armour;
    }
}
UDWORD getMaxBodyArmour(void)
{
    return maxBodyArmour;
}

void setMaxBodyPower(UDWORD power)
{
    if (power > maxBodyPower)
    {
        maxBodyPower = power;
    }
}
UDWORD getMaxBodyPower(void)
{
    return maxBodyPower;
}

void setMaxBodyPoints(UDWORD points)
{
    if (points > maxBodyPoints)
    {
        maxBodyPoints = points;
    }
}
UDWORD getMaxBodyPoints(void)
{
    return maxBodyPoints;
}

void setMaxSensorRange(UDWORD range)
{
    if (range > maxSensorRange)
    {
        maxSensorRange = range;
    }
}
UDWORD getMaxSensorRange(void)
{
    return maxSensorRange;
}

void setMaxSensorPower(UDWORD power)
{
    if (power > maxSensorPower)
    {
        maxSensorPower = power;
    }
}
UDWORD getMaxSensorPower(void)
{
    return maxSensorPower;
}

void setMaxECMPower(UDWORD power)
{
    if (power > maxECMPower)
    {
        maxECMPower = power;
    }
}
UDWORD getMaxECMPower(void)
{
    return maxECMPower;
}

void setMaxECMRange(UDWORD power)
{
    if (power > maxECMRange)
    {
        maxECMPower = power;
    }
}
UDWORD getMaxECMRange(void)
{
    return maxECMRange;
}

void setMaxConstPoints(UDWORD points)
{
    if (points > maxConstPoints)
    {
        maxConstPoints = points;
    }
}
UDWORD getMaxConstPoints(void)
{
    return maxConstPoints;
}

void setMaxRepairPoints(UDWORD repair)
{
    if (repair > maxRepairPoints)
    {
        maxRepairPoints = repair;
    }
}
UDWORD getMaxRepairPoints(void)
{
    return maxRepairPoints;
}

void setMaxWeaponRange(UDWORD range)
{
    if (range > maxWeaponRange)
    {
        maxWeaponRange = range;
    }
}
UDWORD getMaxWeaponRange(void)
{
    return maxWeaponRange;
}

void setMaxWeaponDamage(UDWORD damage)
{
    if (damage > maxWeaponDamage)
    {
        maxWeaponDamage = damage;
    }
}
UDWORD getMaxWeaponDamage(void)
{
    return maxWeaponDamage;
}

void setMaxWeaponROF(UDWORD rof)
{
    if (rof > maxWeaponROF)
    {
        maxWeaponROF = rof;
    }
}
UDWORD getMaxWeaponROF(void)
{
    return maxWeaponROF;
}

void setMaxPropulsionSpeed(UDWORD speed)
{
    if (speed > maxPropulsionSpeed)
    {
        maxPropulsionSpeed = speed;
    }
}
UDWORD getMaxPropulsionSpeed(void)
{
    return maxPropulsionSpeed;
}

//determine the effect this upgrade would have on the max values
void updateMaxWeaponStats(UWORD maxValue)
{
    UDWORD currentMaxValue = getMaxWeaponDamage();

    if (currentMaxValue < (currentMaxValue + maxValue / 100))
    {
        currentMaxValue += currentMaxValue * maxValue / 100;
        setMaxWeaponDamage(currentMaxValue);
    }

    //the fire pause is dealt with differently
}

void updateMaxSensorStats(UWORD maxRange, UWORD maxPower)
{
    UDWORD currentMaxValue = getMaxSensorRange();

    if (currentMaxValue < (currentMaxValue + currentMaxValue * maxRange / 100))
    {
        currentMaxValue += currentMaxValue * maxRange / 100;
        setMaxSensorRange(currentMaxValue);
    }

    currentMaxValue = getMaxSensorPower();
    if (currentMaxValue < (currentMaxValue + currentMaxValue * maxPower / 100))
    {
        currentMaxValue += currentMaxValue * maxPower / 100;
        setMaxSensorPower(currentMaxValue);
    }
}

void updateMaxRepairStats(UWORD maxValue)
{
    UDWORD currentMaxValue = getMaxRepairPoints();

    if (currentMaxValue < (currentMaxValue + currentMaxValue * maxValue / 100))
    {
        currentMaxValue += currentMaxValue * maxValue / 100;
        setMaxRepairPoints(currentMaxValue);
    }
}

void updateMaxECMStats(UWORD maxValue)
{
    UDWORD currentMaxValue = getMaxECMPower();

    if (currentMaxValue < (currentMaxValue + currentMaxValue * maxValue / 100))
    {
        currentMaxValue += currentMaxValue * maxValue / 100;
        setMaxECMPower(currentMaxValue);
    }
}

void updateMaxBodyStats(UWORD maxBody, UWORD maxPower, UWORD maxArmour)
{
    UDWORD currentMaxValue = getMaxBodyPoints();

    if (currentMaxValue < (currentMaxValue + currentMaxValue * maxBody / 100))
    {
        currentMaxValue += currentMaxValue * maxBody / 100;
        setMaxBodyPoints(currentMaxValue);
    }

    currentMaxValue = getMaxBodyPower();
    if (currentMaxValue < (currentMaxValue + currentMaxValue * maxPower / 100))
    {
        currentMaxValue += currentMaxValue * maxPower / 100;
        setMaxBodyPower(currentMaxValue);
    }

    currentMaxValue = getMaxBodyArmour();
    if (currentMaxValue < (currentMaxValue + currentMaxValue * maxArmour / 100))
    {
        currentMaxValue += currentMaxValue * maxArmour / 100;
        setMaxBodyArmour(currentMaxValue);
    }
}

void updateMaxConstStats(UWORD maxValue)
{
    UDWORD currentMaxValue = getMaxConstPoints();

    if (currentMaxValue < (currentMaxValue + currentMaxValue * maxValue / 100))
    {
        currentMaxValue += currentMaxValue * maxValue / 100;
        setMaxConstPoints(currentMaxValue);
    }
}

//propulsion stats are not upgradeable

void adjustMaxDesignStats(void)
{
    UWORD       weaponDamage, sensorRange, sensorPower, repairPoints,
                ecmPower, constPoints, bodyPoints, bodyPower, bodyArmour, inc;

    //init all the values
    weaponDamage = sensorRange = sensorPower = repairPoints =
        ecmPower = constPoints = bodyPoints = bodyPower = bodyArmour = (UWORD)0;

    //go thru' all the functions getting the max upgrade values for the stats
    for (inc = 0; inc < numFunctions; inc++)
    {
        switch(asFunctions[inc]->type)
        {
        case DROIDREPAIR_UPGRADE_TYPE:
            if (repairPoints < ((UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints)
            {
                repairPoints = ((UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints;
            }
            break;
        case DROIDECM_UPGRADE_TYPE:
            if (ecmPower < ((UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints)
            {
                ecmPower = ((UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints;
            }
            break;
        case DROIDBODY_UPGRADE_TYPE:
            if (bodyPoints < ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->body)
            {
                bodyPoints = ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->body;
            }
            if (bodyPower < ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints)
            {
                bodyPower = ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints;
            }
            if (bodyArmour < ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->armourValue[WC_KINETIC])
            {
                bodyArmour = ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->armourValue[WC_KINETIC];
            }
            if (bodyArmour < ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->armourValue[WC_HEAT])
            {
                bodyArmour = ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->armourValue[WC_HEAT];
            }
            break;
        case DROIDSENSOR_UPGRADE_TYPE:
            if (sensorRange < ((DROIDSENSOR_UPGRADE_FUNCTION *)asFunctions[inc])->range)
            {
                sensorRange = ((DROIDSENSOR_UPGRADE_FUNCTION *)asFunctions[inc])->range;
            }
            if (sensorPower < ((DROIDSENSOR_UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints)
            {
                sensorPower = ((DROIDSENSOR_UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints;
            }
            break;
        case DROIDCONST_UPGRADE_TYPE:
            if (constPoints < ((UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints)
            {
                constPoints = ((UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints;
            }
            break;
        case WEAPON_UPGRADE_TYPE:
            if (weaponDamage < ((WEAPON_UPGRADE_FUNCTION *)asFunctions[inc])->damage)
            {
                weaponDamage = ((WEAPON_UPGRADE_FUNCTION *)asFunctions[inc])->damage;
            }
            break;
        default:
            //not interested in other function types
            break;
        }
    }

    //determine the effect on the max values for the stats
    updateMaxWeaponStats(weaponDamage);
    updateMaxSensorStats(sensorRange, sensorPower);
    updateMaxRepairStats(repairPoints);
    updateMaxECMStats(ecmPower);
    updateMaxBodyStats(bodyPoints, bodyPower, bodyArmour);
    updateMaxConstStats(constPoints);
}

/* Check if an object has a weapon */
BOOL objHasWeapon(BASE_OBJECT *psObj)
{

	//check if valid type
	if(psObj->type == OBJ_DROID)
	{
		if ( ((DROID *)psObj)->numWeaps > 0 )
		{
			return true;
		}
	}
	else if(psObj->type == OBJ_STRUCTURE)
	{
		if ( ((STRUCTURE *)psObj)->numWeaps > 0 )
		{
			return true;
		}
	}

	return false;
}

SENSOR_STATS *objActiveRadar(BASE_OBJECT *psObj)
{
	SENSOR_STATS	*psStats = NULL;
	int				compIndex;

	switch (psObj->type)
	{
	case OBJ_DROID:
		if (((DROID *)psObj)->droidType != DROID_SENSOR && ((DROID *)psObj)->droidType != DROID_COMMAND)
		{
			return NULL;
		}
		compIndex = ((DROID *)psObj)->asBits[COMP_SENSOR].nStat;
		ASSERT_OR_RETURN( NULL, compIndex < numSensorStats, "Invalid range referenced for numSensorStats, %d > %d", compIndex, numSensorStats);
		psStats = asSensorStats + compIndex;
		break;
	case OBJ_STRUCTURE:
		psStats = ((STRUCTURE *)psObj)->pStructureType->pSensor;
		if (psStats == NULL || psStats->location != LOC_TURRET || ((STRUCTURE *)psObj)->status != SS_BUILT)
		{
			return NULL;
		}
		break;
	default:
		break;
	}
	return psStats;
}

bool objRadarDetector(BASE_OBJECT *psObj)
{
	if (psObj->type == OBJ_STRUCTURE)
	{
		STRUCTURE *psStruct = (STRUCTURE *)psObj;

		return (psStruct->status == SS_BUILT && psStruct->pStructureType->pSensor && psStruct->pStructureType->pSensor->type == RADAR_DETECTOR_SENSOR);
	}
	else if (psObj->type == OBJ_DROID)
	{
		DROID *psDroid = (DROID *)psObj;
		SENSOR_STATS *psSensor = getSensorStats(psDroid);

		return (psSensor && psSensor->type == RADAR_DETECTOR_SENSOR);
	}
	return false;
}
