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
WEAPON_MODIFIER		asWeaponModifier[WE_NUMEFFECTS][NUM_PROP_TYPES];

//used to hold the current upgrade level per player per weapon subclass
WEAPON_UPGRADE		asWeaponUpgrade[MAX_PLAYERS][NUM_WEAPON_SUBCLASS];
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

//stores for each players component states - can be either UNAVAILABLE, FOUND or AVAILABLE
UBYTE		*apCompLists[MAX_PLAYERS][COMP_NUMCOMPONENTS];

//store for each players Structure states
UBYTE		*apStructTypeLists[MAX_PLAYERS];

static BOOL compareYes(char *strToCompare, char *strOwner);
static MOVEMENT_MODEL	getMovementModel(char *pMovement);

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
	(list) = (type *)malloc(sizeof(type) * (numEntries)); \
	if ((list) == NULL) \
	{ \
		debug( LOG_ERROR, "Out of memory" ); \
		abort(); \
		return FALSE; \
	} \
	(listSize) = (numEntries); \
	return TRUE


/*Macro to Deallocate stats*/
#define STATS_DEALLOC(list, listSize, type) \
	statsDealloc((COMP_BASE_STATS*)(list), (listSize), sizeof(type))


void statsInitVars(void)
{
	int i,j;

	asBodyStats = NULL;
	asBrainStats = NULL;
	asPropulsionStats = NULL;
	asSensorStats = NULL;
	asECMStats = NULL;
	asRepairStats = NULL;
	asWeaponStats = NULL;
	asConstructStats = NULL;
	asPropulsionTypes = NULL;
	asTerrainTable = NULL;
	asSpecialAbility = NULL;

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

	//stores for each players component states - can be either UNAVAILABLE, FOUND or AVAILABLE
	for(i=0; i<MAX_PLAYERS; i++) {
		for(j=0; j<COMP_NUMCOMPONENTS; j++) {
			apCompLists[i][j] = NULL;
		}
	}

	//store for each players Structure states
	for(i=0; i<MAX_PLAYERS; i++) {
		apStructTypeLists[i] = NULL;
	}

	//initialise the upgrade structures
	memset(asWeaponUpgrade, 0, MAX_PLAYERS * NUM_WEAPON_SUBCLASS * sizeof(WEAPON_UPGRADE));
	memset(asSensorUpgrade, 0, MAX_PLAYERS * sizeof(SENSOR_UPGRADE));
	memset(asECMUpgrade, 0, MAX_PLAYERS * sizeof(ECM_UPGRADE));
	memset(asRepairUpgrade, 0, MAX_PLAYERS * sizeof(REPAIR_UPGRADE));
	memset(asBodyUpgrade, 0, MAX_PLAYERS * sizeof(BODY_UPGRADE) * BODY_TYPE);

	// init the max values
	maxComponentWeight = maxBodyArmour = maxBodyPower =
        maxBodyPoints = maxSensorRange = maxSensorPower = maxECMPower = maxECMRange =
        maxConstPoints = maxRepairPoints = maxWeaponRange = maxWeaponDamage =
        maxPropulsionSpeed = 0;
}


/*Deallocate all the stats assigned from input data*/
void statsDealloc(COMP_BASE_STATS* pStats, UDWORD listSize, UDWORD structureSize)
{
#if !defined (RESOURCE_NAMES) && !defined(STORE_RESOURCE_ID)

	UDWORD				inc;
	COMP_BASE_STATS		*pStatList = pStats;
	UDWORD				address = (UDWORD)pStats;

	for (inc=0; inc < listSize; inc++)
	{
		free(pStatList->pName);
		address += structureSize;
		pStatList = (COMP_BASE_STATS *) address;
	}
#endif

	free(pStats);
}


static BOOL allocateStatName(BASE_STATS* pStat, const char *Name)
{
	return (allocateName(&pStat->pName, Name));
}


/* body stats need the extra list removing */
static void deallocBodyStats(void)
{
	BODY_STATS *psStat;
	UDWORD		inc;

	for (inc = 0; inc < numBodyStats; inc++)
	{
		psStat = &asBodyStats[inc];

#if !defined (RESOURCE_NAMES) && !defined (STORE_RESOURCE_ID)

		free(psStat->pName);
#endif
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

	return TRUE;
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

const char *getStatName(void * Stat)
{
	const BASE_STATS * psStats= (BASE_STATS*)Stat;

	return(getName(psStats->pName));
}




/*******************************************************************************
*		Load stats functions
*******************************************************************************/
/*Load the weapon stats from the file exported from Access*/
BOOL loadWeaponStats(const char *pWeaponData, UDWORD bufferSize)
{
	const unsigned int NumWeapons = numCR(pWeaponData, bufferSize);
	WEAPON_STATS	sStats, *psStats = &sStats, *psStartStats = &sStats;
	UDWORD			i, rotate, maxElevation, surfaceToAir;
	SDWORD			minElevation;
	char			WeaponName[MAX_NAME_SIZE], GfxFile[MAX_NAME_SIZE];
	char			mountGfx[MAX_NAME_SIZE], flightGfx[MAX_NAME_SIZE],
					hitGfx[MAX_NAME_SIZE], missGfx[MAX_NAME_SIZE],
					waterGfx[MAX_NAME_SIZE], muzzleGfx[MAX_NAME_SIZE],
					trailGfx[MAX_NAME_SIZE], techLevel[MAX_NAME_SIZE];
	char			fireOnMove[10], weaponClass[15], weaponSubClass[15],
					weaponEffect[16], movement[15], facePlayer[5],		//weaponEffect[15] caused stack corruption. --Qamly
					faceInFlight[5],lightWorld[5];
	UDWORD			longRange, effectSize, numAttackRuns, designable;
	UDWORD			numRounds;

	char			*StatsName;
	UDWORD			penetrate;

	if (!statsAllocWeapons(NumWeapons))
	{
		return FALSE;
	}

	for (i=0; i < NumWeapons; i++)
	{
		memset(psStats, 0, sizeof(WEAPON_STATS));

		WeaponName[0] = '\0';
		techLevel[0] = '\0';
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
			(char *)&WeaponName, (char *)&techLevel, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &psStats->hitPoints, &psStats->systemPoints,
			&psStats->body, (char *)&GfxFile, (char *)&mountGfx, (char *)&muzzleGfx, (char *)&flightGfx,
			(char *)&hitGfx, (char *)&missGfx, (char *)&waterGfx, (char *)&trailGfx, &psStats->shortRange,
			&psStats->longRange,&psStats->shortHit, &psStats->longHit,
			&psStats->firePause, &psStats->numExplosions, &numRounds,
			&psStats->reloadTime, &psStats->damage, &psStats->radius,
			&psStats->radiusHit, &psStats->radiusDamage, &psStats->incenTime,
			&psStats->incenDamage, &psStats->incenRadius, &psStats->directLife,
			&psStats->radiusLife, &psStats->flightSpeed, &psStats->indirectHeight,
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
			return FALSE;
		}

		psStats->ref = REF_WEAPON_START + i;

		//determine the tech level
		if (!setTechLevel((BASE_STATS *)psStats, techLevel))
		{
			return FALSE;
		}

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

				debug( LOG_ERROR, "Cannot find the weapon PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
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
				debug( LOG_ERROR, "Cannot find the mount PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
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
				debug( LOG_ERROR, "Cannot find the muzzle PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
			}


			psStats->pInFlightGraphic = (iIMDShape *) resGetData("IMD", flightGfx);
			if (psStats->pInFlightGraphic == NULL)
			{
				debug( LOG_ERROR, "Cannot find the flight PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
			}

			psStats->pTargetHitGraphic = (iIMDShape *) resGetData("IMD", hitGfx);
			if (psStats->pTargetHitGraphic == NULL)
			{
				debug( LOG_ERROR, "Cannot find the target hit PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
			}

			psStats->pTargetMissGraphic = (iIMDShape *) resGetData("IMD", missGfx);
			if (psStats->pTargetMissGraphic == NULL)
			{
				debug( LOG_ERROR, "Cannot find the target miss PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
			}

			psStats->pWaterHitGraphic = (iIMDShape *) resGetData("IMD", waterGfx);
			if (psStats->pWaterHitGraphic == NULL)
			{
				debug( LOG_ERROR, "Cannot find the water hit PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
			}
			//trail graphic can be null
			if (strcmp(trailGfx, "0"))
			{
				psStats->pTrailGraphic = (iIMDShape *) resGetData("IMD", trailGfx);
				if (psStats->pTrailGraphic == NULL)
				{
					debug( LOG_ERROR, "Cannot find the trail PIE for record %s", getStatName(psStats) );
					abort();
					return FALSE;
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
			debug( LOG_ERROR, "Invalid fire on move flag for weapon %s", getStatName(psStats) );
			abort();
			return FALSE;
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
			debug( LOG_ERROR, "Invalid weapon class for weapon %s", getStatName(psStats) );
			abort();
			return FALSE;
		}

		//set the subClass
		psStats->weaponSubClass = getWeaponSubClass(weaponSubClass);
		if (psStats->weaponSubClass == INVALID_SUBCLASS)
		{
			return FALSE;
		}

		//set the movement model
		psStats->movementModel = getMovementModel(movement);
		if (psStats->movementModel == INVALID_MOVEMENT)
		{
			return FALSE;
		}

		//set the weapon effect
		psStats->weaponEffect = getWeaponEffect(weaponEffect);
		if (psStats->weaponEffect == INVALID_WEAPON_EFFECT)
		{
			debug( LOG_ERROR, "loadWepaonStats: Invalid weapon effect for weapon %s", getStatName(psStats) );
			abort();
			return FALSE;
		}

		StatsName=psStats->pName;

		//set the face Player value
		if (compareYes(facePlayer, StatsName))
		{
			psStats->facePlayer = TRUE;
		}
		else
		{
			psStats->facePlayer = FALSE;
		}

		//set the In flight face Player value
		if (compareYes(faceInFlight, StatsName))
		{
			psStats->faceInFlight = TRUE;
		}
		else
		{
			psStats->faceInFlight = FALSE;
		}

		//set the light world value
		if (compareYes(lightWorld, StatsName))
		{
			psStats->lightWorld = TRUE;
		}
		else
		{
			psStats->lightWorld = FALSE;
		}

		//set the effect size
		if (effectSize > UBYTE_MAX)
		{
			ASSERT( FALSE,"loadWeaponStats: effectSize is greater than 255 for weapon %s",
				getStatName(psStats) );
			return FALSE;
		}
		psStats->effectSize = (UBYTE)effectSize;

		//set the rotate angle
		if (rotate > UBYTE_MAX)
		{
			ASSERT( FALSE,"loadWeaponStats: rotate is greater than 255 for weapon %s",
				getStatName(psStats) );
			return FALSE;
		}
		psStats->rotate = (UBYTE)rotate;

		//set the minElevation
		if (minElevation > SBYTE_MAX || minElevation < SBYTE_MIN)
		{
			ASSERT( FALSE,"loadWeaponStats: minElevation is outside of limits for weapon %s",
				getStatName(psStats) );
			return FALSE;
		}
		psStats->minElevation = (SBYTE)minElevation;

		//set the maxElevation
		if (maxElevation > UBYTE_MAX)
		{
			ASSERT( FALSE,"loadWeaponStats: maxElevation is outside of limits for weapon %s",
				getStatName(psStats) );
			return FALSE;
		}
		psStats->maxElevation = (UBYTE)maxElevation;

		//set the surfaceAir
		if (surfaceToAir > UBYTE_MAX)
		{
			ASSERT( FALSE, "loadWeaponStats: Surface to Air is outside of limits for weapon %s",
				getStatName(psStats) );
			return FALSE;
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
			ASSERT( FALSE, "loadWeaponStats: num of attack runs is outside of limits for weapon %s",
				getStatName(psStats) );
			return FALSE;
		}
		psStats->vtolAttackRuns = (UBYTE)numAttackRuns;

		//set design flag
		if (designable)
		{
			psStats->design = TRUE;
		}
		else
		{
			psStats->design = FALSE;
		}

		//set penetrate flag
		if (penetrate)
		{
			psStats->penetrate = TRUE;
		}
		else
		{
			psStats->penetrate = FALSE;
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
		if (psStats->design)
		{
			setMaxWeaponRange(psStats->longRange);
			setMaxWeaponDamage(psStats->damage);
			setMaxWeaponROF(weaponROF(psStats, -1));
			setMaxComponentWeight(psStats->weight);
		}

		psStats = psStartStats;
		//increment the pointer to the start of the next record
		pWeaponData = strchr(pWeaponData,'\n') + 1;
	}

	return TRUE;
}

/*Load the Body stats from the file exported from Access*/
BOOL loadBodyStats(const char *pBodyData, UDWORD bufferSize)
{
	BODY_STATS sStats, *psStats = &sStats, *psStartStats = &sStats;
	const unsigned int NumBody = numCR(pBodyData, bufferSize);
	unsigned int i, designable;
	char BodyName[MAX_NAME_SIZE], size[MAX_NAME_SIZE],
		GfxFile[MAX_NAME_SIZE], techLevel[MAX_NAME_SIZE],
		flameIMD[MAX_NAME_SIZE];

	if (!statsAllocBody(NumBody))
	{
		return FALSE;
	}

	for (i = 0; i < NumBody; i++)
	{
		memset(psStats, 0, sizeof(BODY_STATS));

		BodyName[0] = '\0';
		techLevel[0] = '\0';
		size[0] = '\0';
		GfxFile[0] = '\0';
		flameIMD[0] = '\0';

		//Watermelon:added 10 %d to store FRONT,REAR,LEFT,RIGHT,TOP,BOTTOM armour values
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pBodyData,"%[^','],%[^','],%[^','],%d,%d,%d,%d,%[^','],\
			%d,%d,%d, \
			%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%[^','],%d",
			(char*)&BodyName, (char*)&techLevel, (char*)&size, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &psStats->body, (char*)&GfxFile, &psStats->systemPoints,
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
			return FALSE;
		}

		psStats->ref = REF_BODY_START + i;

		//determine the tech level
		if (!setTechLevel((BASE_STATS *)psStats, techLevel))
		{
			return FALSE;
		}

		if (!getBodySize(size, &psStats->size))
		{
			ASSERT( FALSE, "loadBodyStats: unknown body size for %s",
				getStatName(psStats) );
			return FALSE;
		}

		//set design flag
		psStats->design = (designable != 0);

		//get the IMD for the component
		if (strcmp(GfxFile, "0"))
		{
			psStats->pIMD = (iIMDShape *) resGetData("IMD", GfxFile);
			if (psStats->pIMD == NULL)
			{
				debug( LOG_ERROR, "Cannot find the body PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
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
				debug( LOG_ERROR, "Cannot find the flame PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
			}
		}
		else
		{
			psStats->pFlameIMD = NULL;
		}

		//save the stats
		statsSetBody(psStats, i);

		//set the max stat values for the design screen
		if (psStats->design)
		{
			//use front armour value to prevent bodyStats corrupt problems
			setMaxBodyArmour(psStats->armourValue[HIT_SIDE_FRONT][WC_KINETIC]);
			setMaxBodyArmour(psStats->armourValue[HIT_SIDE_FRONT][WC_HEAT]);
			setMaxBodyPower(psStats->powerOutput);
			setMaxBodyPoints(psStats->body);
			setMaxComponentWeight(psStats->weight);
		}

		psStats = psStartStats;
		//increment the pointer to the start of the next record
		pBodyData = strchr(pBodyData,'\n') + 1;
	}

	return TRUE;
}

/*Load the Brain stats from the file exported from Access*/
BOOL loadBrainStats(const char *pBrainData, UDWORD bufferSize)
{
	BRAIN_STATS sStats, *psStats = &sStats, *psStartStats = &sStats;
	const unsigned int NumBrain = numCR(pBrainData, bufferSize);
	unsigned int i = 0, weapon = 0;
	char		BrainName[MAX_NAME_SIZE], techLevel[MAX_NAME_SIZE],
				weaponName[MAX_NAME_SIZE];

	if (!statsAllocBrain(NumBrain))
	{
		return FALSE;
	}

	for (i = 0; i < NumBrain; i++)
	{
		memset(psStats, 0, sizeof(BRAIN_STATS));

		BrainName[0] = '\0';
		techLevel[0] = '\0';
		weaponName[0] = '\0';
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pBrainData,"%[^','],%[^','],%d,%d,%d,%d,%d,%[^','],%d",
			(char*)&BrainName, (char*)&techLevel, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &psStats->hitPoints, &psStats->systemPoints,
			(char*)&weaponName, &psStats->progCap); //, &psStats->AICap, &psStats->AISpeed);

		if (!allocateStatName((BASE_STATS *)psStats, BrainName))
		{
			return FALSE;
		}

		psStats->ref = REF_BRAIN_START + i;

		//determine the tech level
		if (!setTechLevel((BASE_STATS *)psStats, techLevel))
		{
			return FALSE;
		}

		//check weapon attached
		if (!strcmp(weaponName, "0"))
		{
			psStats->psWeaponStat = NULL;
		}
		else
		{
			//get the weapon stat
			if (!getResourceName(weaponName))
			{
				return FALSE;
			}
			weapon = getCompFromName(COMP_WEAPON, weaponName);

			//if weapon not found - error
			if (weapon == -1)
			{
				debug( LOG_ERROR, "Unable to find Weapon %s for brain %s", weaponName, BrainName );
				abort();
				return FALSE;
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
			psStats->design = FALSE;
		}
		else
		{
			psStats->design = TRUE;
		}

		//save the stats
		statsSetBrain(psStats, i);

		psStats = psStartStats;
		//increment the pointer to the start of the next record
		pBrainData = strchr(pBrainData, '\n') + 1;
	}
	return TRUE;
}


/*returns the propulsion type based on the string name passed in */
PROPULSION_TYPE getPropulsionType(const char *pType)
{
	if (!strcmp(pType, "Wheeled"))
	{
		return WHEELED;
	}
	if (!strcmp(pType, "Tracked"))
	{
		return TRACKED;
	}
	if (!strcmp(pType, "Legged"))
	{
		return LEGGED;
	}
	if (!strcmp(pType, "Hover"))
	{
		return HOVER;
	}
	if (!strcmp(pType, "Ski"))
	{
		return SKI;
	}
	if (!strcmp(pType, "Lift"))
	{
		return LIFT;
	}
	if (!strcmp(pType, "Propellor"))
	{
		return PROPELLOR;
	}
	if (!strcmp(pType, "Half-Tracked"))
	{
		return HALF_TRACKED;
	}
	if (!strcmp(pType, "Jump"))
	{
		return JUMP;
	}

	return INVALID_PROP_TYPE;
}

/*Load the Propulsion stats from the file exported from Access*/
BOOL loadPropulsionStats(const char *pPropulsionData, UDWORD bufferSize)
{
	const unsigned int NumPropulsion = numCR(pPropulsionData, bufferSize);
	PROPULSION_STATS	sStats, *psStats = &sStats, *psStartStats = &sStats;
	unsigned int i = 0, designable;
	char				PropulsionName[MAX_NAME_SIZE], imdName[MAX_NAME_SIZE],
						techLevel[MAX_NAME_SIZE], type[MAX_NAME_SIZE];

	if (!statsAllocPropulsion(NumPropulsion))
	{
		return FALSE;
	}

	for (i = 0; i < NumPropulsion; i++)
	{
		memset(psStats, 0, sizeof(PROPULSION_STATS));

		PropulsionName[0] = '\0';
		techLevel[0] = '\0';
		imdName[0] = '\0';

		//read the data into the storage - the data is delimeted using comma's
		sscanf(pPropulsionData,"%[^','],%[^','],%d,%d,%d,%d,%d,%d,%[^','],\
			%[^','],%d,%d",
			(char*)&PropulsionName, (char*)&techLevel, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &psStats->hitPoints, &psStats->systemPoints,
			&psStats->body,	(char*)&imdName, (char*)&type, &psStats->maxSpeed, &designable);

		if (!allocateStatName((BASE_STATS *)psStats, PropulsionName))
		{
			return FALSE;
		}

		psStats->ref = REF_PROPULSION_START + i;

		//determine the tech level
		if (!setTechLevel((BASE_STATS *)psStats, techLevel))
		{
			return FALSE;
		}

		//set design flag
		psStats->design = (designable != 0);

		//deal with imd - stored so that got something to see in Design Screen!
		if (strcmp(imdName, "0"))
		{
			psStats->pIMD = (iIMDShape *) resGetData("IMD", imdName);
			if (psStats->pIMD == NULL)
			{
				debug( LOG_ERROR, "Cannot find the propulsion PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
			}
		}
		else
		{
			psStats->pIMD = NULL;
		}

		//set up the stats type
		psStats->propulsionType = getPropulsionType(type);
		if (psStats->propulsionType == INVALID_PROP_TYPE)
		{
			debug( LOG_ERROR, "loadPropulsionStats: Invalid Propulsion type for %s", getStatName(psStats) );
			abort();
			return FALSE;
		}

		//save the stats
		statsSetPropulsion(psStats, i);

		//set the max stat values for the design screen
		if (psStats->design)
		{
			setMaxPropulsionSpeed(psStats->maxSpeed);
			//setMaxComponentWeight(psStats->weight);
		}

		psStats = psStartStats;
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
			if (asBodyStats[i].design)
			{
				unsigned int j = 0;
				//check against each propulsion stat
				for (j = 0; j < numPropulsionStats; j++)
				{
					//check stat is designable
					if (asPropulsionStats[j].design)
					{
						setMaxComponentWeight(asPropulsionStats[j].weight * asBodyStats[i].weight / 100);
					}
				}
			}
		}
	}

	return TRUE;
}

/*Load the Sensor stats from the file exported from Access*/
BOOL loadSensorStats(const char *pSensorData, UDWORD bufferSize)
{
	const unsigned int NumSensor = numCR(pSensorData, bufferSize);
	SENSOR_STATS sStats, *psStats = &sStats, *psStartStats = &sStats;
	unsigned int i = 0, designable;
	char			SensorName[MAX_NAME_SIZE], location[MAX_NAME_SIZE],
					GfxFile[MAX_NAME_SIZE],type[MAX_NAME_SIZE];
	char			mountGfx[MAX_NAME_SIZE], techLevel[MAX_NAME_SIZE];

	if (!statsAllocSensor(NumSensor))
	{
		return FALSE;
	}

	for (i = 0; i < NumSensor; i++)
	{
		memset(psStats, 0, sizeof(SENSOR_STATS));

		SensorName[0] = '\0';
		techLevel[0] = '\0';
		GfxFile[0] = '\0';
		mountGfx[0] = '\0';
		location[0] = '\0';
		type[0] = '\0';
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pSensorData,"%[^','],%[^','],%d,%d,%d,%d,%d,%d,%[^','],\
			%[^','],%d,%[^','],%[^','],%d,%d,%d",
			(char*)&SensorName, (char*)&techLevel, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &psStats->hitPoints, &psStats->systemPoints,
			&psStats->body,	(char*)&GfxFile,(char*)&mountGfx,
			&psStats->range, (char*)&location, (char*)&type, &psStats->time, &psStats->power, &designable);

		if (!allocateStatName((BASE_STATS *)psStats, SensorName))
		{
			return FALSE;
		}

		psStats->ref = REF_SENSOR_START + i;

		//determine the tech level
		if (!setTechLevel((BASE_STATS *)psStats, techLevel))
		{
			return FALSE;
		}

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
			ASSERT( FALSE, "Invalid Sensor location" );
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
		else
		{
			ASSERT( FALSE, "Invalid Sensor type" );
		}

		//multiply time stats
		psStats->time *= WEAPON_TIME;

		//set design flag
		psStats->design = (designable != 0);

		//get the IMD for the component
		if (strcmp(mountGfx, "0"))
		{
			psStats->pIMD = (iIMDShape *) resGetData("IMD", GfxFile);
			if (psStats->pIMD == NULL)
			{
				debug( LOG_ERROR, "Cannot find the sensor PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
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
				debug( LOG_ERROR, "Cannot find the mount PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
			}
		}
		else
		{
			psStats->pMountGraphic = NULL;
		}

		//save the stats
		statsSetSensor(psStats, i);

        //set the max stat values for the design screen
        if (psStats->design)
        {
            setMaxSensorRange(psStats->range);
            setMaxSensorPower(psStats->power);
            setMaxComponentWeight(psStats->weight);
        }

		psStats = psStartStats;
		//increment the pointer to the start of the next record
		pSensorData = strchr(pSensorData,'\n') + 1;
	}
	return TRUE;
}

/*Load the ECM stats from the file exported from Access*/
BOOL loadECMStats(const char *pECMData, UDWORD bufferSize)
{
	const unsigned int NumECM = numCR(pECMData, bufferSize);
	ECM_STATS	sStats, *psStats = &sStats, *psStartStats = &sStats;
	unsigned int i = 0, designable;
	char		ECMName[MAX_NAME_SIZE], location[MAX_NAME_SIZE],
				GfxFile[MAX_NAME_SIZE];
	char		mountGfx[MAX_NAME_SIZE], techLevel[MAX_NAME_SIZE];

	if (!statsAllocECM(NumECM))
	{
		return FALSE;
	}

	for (i=0; i < NumECM; i++)
	{
		memset(psStats, 0, sizeof(ECM_STATS));

		ECMName[0] = '\0';
		techLevel[0] = '\0';
		GfxFile[0] = '\0';
		mountGfx[0] = '\0';
		location[0] = '\0';
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pECMData,"%[^','],%[^','],%d,%d,%d,%d,%d,%d,%[^','],%[^','],\
			%[^','],%d,%d,%d",
			(char*)&ECMName, (char*)&techLevel, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &psStats->hitPoints, &psStats->systemPoints,
			&psStats->body,	(char*)&GfxFile, (char*)&mountGfx, (char*)&location, &psStats->power,
			&psStats->range, &designable);

		// set a default ECM range for now
		psStats->range = TILE_UNITS * 8;

		if (!allocateStatName((BASE_STATS *)psStats, ECMName))
		{
			return FALSE;
		}

		psStats->ref = REF_ECM_START + i;

		//determine the tech level
		if (!setTechLevel((BASE_STATS *)psStats, techLevel))
		{
			return FALSE;
		}

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
			ASSERT( FALSE, "Invalid ECM location" );
		}

		//set design flag
		psStats->design = (designable != 0);

		//get the IMD for the component
		if (strcmp(GfxFile, "0"))
		{
			psStats->pIMD = (iIMDShape *) resGetData("IMD", GfxFile);
			if (psStats->pIMD == NULL)
			{
				debug( LOG_ERROR, "Cannot find the ECM PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
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
				debug( LOG_ERROR, "Cannot find the mount PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
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
		if (psStats->design)
		{
			setMaxECMPower(psStats->power);
			setMaxECMRange(psStats->range);
			setMaxComponentWeight(psStats->weight);
		}

		psStats = psStartStats;
		//increment the pointer to the start of the next record
		pECMData = strchr(pECMData,'\n') + 1;
	}

	return TRUE;
}

/*Load the Repair stats from the file exported from Access*/
BOOL loadRepairStats(const char *pRepairData, UDWORD bufferSize)
{
	const unsigned int NumRepair = numCR(pRepairData, bufferSize);
	REPAIR_STATS sStats, *psStats = &sStats, *psStartStats = &sStats;
	unsigned int i = 0, designable;
	char			RepairName[MAX_NAME_SIZE], techLevel[MAX_NAME_SIZE],
					GfxFile[MAX_NAME_SIZE],	mountGfx[MAX_NAME_SIZE],
					location[MAX_NAME_SIZE];

	if (!statsAllocRepair(NumRepair))
	{
		return FALSE;
	}

	for (i=0; i < NumRepair; i++)
	{
		memset(psStats, 0, sizeof(REPAIR_STATS));

		RepairName[0] = '\0';
		techLevel[0] = '\0';
		GfxFile[0] = '\0';
		mountGfx[0] = '\0';
		location[0] = '\0';

	//read the data into the storage - the data is delimeted using comma's
		sscanf(pRepairData,"%[^','],%[^','],%d,%d,%d,%d,%d,%d,%[^','],\
			%[^','],%[^','],%d,%d,%d",
			(char*)&RepairName, (char*)&techLevel, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &psStats->hitPoints, &psStats->systemPoints,
			&psStats->repairArmour, (char*)&location, (char*)&GfxFile, (char*)&mountGfx,
			&psStats->repairPoints, &psStats->time,&designable);

		if (!allocateStatName((BASE_STATS *)psStats, RepairName))
		{
			return FALSE;
		}

		psStats->ref = REF_REPAIR_START + i;

		//determine the tech level
		if (!setTechLevel((BASE_STATS *)psStats, techLevel))
		{
			return FALSE;
		}

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
			ASSERT( FALSE, "Invalid Repair location" );
		}

		//multiply time stats
		psStats->time *= WEAPON_TIME;

		//check its not 0 since we will be dividing by it at a later stage
		if (psStats->time == 0)
		{

			ASSERT( FALSE, "loadRepairStats: the delay time cannot be zero for %s",
				psStats->pName );

			psStats->time = 1;
		}

		//set design flag
		psStats->design = (designable != 0);

		//get the IMD for the component
		if (strcmp(GfxFile, "0"))
		{
			psStats->pIMD = (iIMDShape *) resGetData("IMD", GfxFile);
			if (psStats->pIMD == NULL)
			{
				debug( LOG_ERROR, "Cannot find the Repair PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
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
				debug( LOG_ERROR, "Cannot find the Repair mount PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
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
        if (psStats->design)
        {
            setMaxRepairPoints(psStats->repairPoints);
            setMaxComponentWeight(psStats->weight);
        }

		psStats = psStartStats;
		//increment the pointer to the start of the next record
		pRepairData = strchr(pRepairData,'\n') + 1;
	}
//	free(pData);
//	free(psStats);
	return TRUE;
}

/*Load the Construct stats from the file exported from Access*/
BOOL loadConstructStats(const char *pConstructData, UDWORD bufferSize)
{
	const unsigned int NumConstruct = numCR(pConstructData, bufferSize);
	CONSTRUCT_STATS sStats, *psStats = &sStats, *psStartStats = &sStats;
	unsigned int i = 0, designable;
	char			ConstructName[MAX_NAME_SIZE], GfxFile[MAX_NAME_SIZE];
	char			mountGfx[MAX_NAME_SIZE], techLevel[MAX_NAME_SIZE];

	if (!statsAllocConstruct(NumConstruct))
	{
		return FALSE;
	}

	for (i=0; i < NumConstruct; i++)
	{
		memset(psStats, 0, sizeof(CONSTRUCT_STATS));

		ConstructName[0] = '\0';
		techLevel[0] = '\0';
		GfxFile[0] = '\0';
		mountGfx[0] = '\0';
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pConstructData,"%[^','],%[^','],%d,%d,%d,%d,%d,%d,%[^','],\
			%[^','],%d,%d",
			(char*)&ConstructName, (char*)&techLevel, &psStats->buildPower,&psStats->buildPoints,
			&psStats->weight, &psStats->hitPoints, &psStats->systemPoints,
			&psStats->body, (char*)&GfxFile, (char*)&mountGfx,
			&psStats->constructPoints,&designable);

		if (!allocateStatName((BASE_STATS *)psStats, ConstructName))
		{
			return FALSE;
		}

		psStats->ref = REF_CONSTRUCT_START + i;

		//determine the tech level
		if (!setTechLevel((BASE_STATS *)psStats, techLevel))
		{
			return FALSE;
		}

		//set design flag
		if (designable)
		{
			psStats->design = TRUE;
		}
		else
		{
			psStats->design = FALSE;
		}

		//get the IMD for the component
		if (strcmp(GfxFile, "0"))
		{
			psStats->pIMD = (iIMDShape *) resGetData("IMD", GfxFile);
			if (psStats->pIMD == NULL)
			{
				debug( LOG_ERROR, "Cannot find the constructor PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
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
				debug( LOG_ERROR, "Cannot find the mount PIE for record %s", getStatName(psStats) );
				abort();
				return FALSE;
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
		if (psStats->design)
		{
			setMaxConstPoints(psStats->constructPoints);
			setMaxComponentWeight(psStats->weight);
		}

		psStats = psStartStats;
		//increment the pointer to the start of the next record
		pConstructData = strchr(pConstructData,'\n') + 1;
	}

	return TRUE;
}


/*Load the Propulsion Types from the file exported from Access*/
BOOL loadPropulsionTypes(const char *pPropTypeData, UDWORD bufferSize)
{
	const unsigned int NumTypes = NUM_PROP_TYPES;
	PROPULSION_TYPES *pPropType;
	unsigned int i, multiplier, type;
	char PropulsionName[MAX_NAME_SIZE], flightName[MAX_NAME_SIZE];

	//allocate storage for the stats
	asPropulsionTypes = (PROPULSION_TYPES *)malloc(sizeof(PROPULSION_TYPES)*NumTypes);
	if (asPropulsionTypes == NULL)
	{
		debug( LOG_ERROR, "PropulsionTypes - Out of memory" );
		abort();
		return FALSE;
	}

	memset(asPropulsionTypes, 0, (sizeof(PROPULSION_TYPES)*NumTypes));

	for (i=0; i < NumTypes; i++)
	{
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pPropTypeData,"%[^','],%[^','],%d",
			(char*)&PropulsionName, (char*)&flightName, &multiplier);

		//set the pointer for this record based on the name
		type = getPropulsionType(PropulsionName);
		if (type == INVALID_PROP_TYPE)
		{
			debug( LOG_ERROR, "loadPropulsionTypes: Invalid Propulsion type - %s", PropulsionName );
			abort();
			return FALSE;
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
			ASSERT( FALSE, "Invalid travel type for Propulsion" );
		}

        //don't care about this anymore! AB FRIDAY 13/11/98
        //want it back again! AB 27/11/98
        if (multiplier > UWORD_MAX)
        {
            ASSERT( FALSE, "loadPropulsionTypes: power Ratio multiplier too high" );
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

	return TRUE;
}


/*Load the Terrain Table from the file exported from Access*/
BOOL loadTerrainTable(const char *pTerrainTableData, UDWORD bufferSize)
{
	const unsigned int NumEntries = numCR(pTerrainTableData, bufferSize);
	TERRAIN_TABLE *pTerrainTable;
	unsigned int i = 0, j = 0;
	UDWORD			terrainType, propulsionType, speedFactor;

	//allocate storage for the stats
	asTerrainTable = (TERRAIN_TABLE *)malloc(sizeof(TERRAIN_TABLE) * NUM_PROP_TYPES * TERRAIN_TYPES);

	if (asTerrainTable == NULL)
	{
		debug( LOG_ERROR, "Terrain Types - Out of memory" );
		abort();
		return FALSE;
	}

	//initialise the storage to 100
	for (i=0; i < TERRAIN_TYPES; i++)
	{
		for (j=0; j < NUM_PROP_TYPES; j++)
		{
			pTerrainTable = asTerrainTable + (i * NUM_PROP_TYPES + j);
			pTerrainTable->speedFactor = 100;
		}
	}

	for (i=0; i < NumEntries; i++)
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
	for (i=0; i < TERRAIN_TYPES; i++)
	{
		for (j=0; j < NUM_PROP_TYPES; j++)
		{
			pTerrainTable = asTerrainTable + (i * NUM_PROP_TYPES + j);
			if (pTerrainTable->speedFactor == 0)
			{
				debug( LOG_ERROR, "loadTerrainTable: Invalid propulsion/terrain table entry" );
				abort();
				return FALSE;
			}
		}
	}

	return TRUE;
}

/*Load the Special Ability stats from the file exported from Access*/
BOOL loadSpecialAbility(const char *pSAbilityData, UDWORD bufferSize)
{
	const unsigned int NumTypes = numCR(pSAbilityData, bufferSize);
	SPECIAL_ABILITY *pSAbility;
	unsigned int i, accessID;
	char SAbilityName[MAX_NAME_SIZE];

	//allocate storage for the stats
	asSpecialAbility = (SPECIAL_ABILITY *)malloc(sizeof(SPECIAL_ABILITY)*NumTypes);

	if (asSpecialAbility == NULL)
	{
		debug( LOG_ERROR, "SpecialAbility - Out of memory" );
		abort();
		return FALSE;
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
			debug( LOG_ERROR, "The Special Ability sequence is invalid" );
			abort();
			return FALSE;
		}
		//allocate storage for the name
		asSpecialAbility->pName = (char *)malloc((strlen(SAbilityName))+1);
		if (asSpecialAbility->pName == NULL)
		{
			debug( LOG_ERROR, "Special Ability Name - Out of memory" );
			abort();
			return FALSE;
		}
		strcpy(asSpecialAbility->pName,SAbilityName);

		//increment the pointer to the start of the next record
		pSAbilityData = strchr(pSAbilityData,'\n') + 1;
		asSpecialAbility++;
	}

	//reset the pointer to the start of the special ability stats
	asSpecialAbility = pSAbility;
	return TRUE;
}

/* load the IMDs to use for each body-propulsion combination */
BOOL loadBodyPropulsionIMDs(const char *pData, UDWORD bufferSize)
{
	const unsigned int NumTypes = numCR(pData, bufferSize);
	BODY_STATS *psBodyStat = asBodyStats;
	PROPULSION_STATS *psPropulsionStat = asPropulsionStats;
	unsigned int i, numStats;
	char				bodyName[MAX_NAME_SIZE], propulsionName[MAX_NAME_SIZE],
						leftIMD[MAX_NAME_SIZE], rightIMD[MAX_NAME_SIZE];
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
			debug( LOG_ERROR, "Out of memory" );
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
		found = FALSE;
		if (!getResourceName(bodyName))
		{
			return FALSE;
		}

		for (numStats = 0; numStats < numBodyStats; numStats++)
		{
			psBodyStat = &asBodyStats[numStats];
			if (!strcmp(psBodyStat->pName, bodyName))
			{
				found = TRUE;
				break;
			}
		}
		if (!found)
		{
			debug( LOG_ERROR, "loadBodyPropulsionPIEs: Invalid body name %s", bodyName );
			abort();
			return FALSE;
		}

		//get the propulsion stats
		found = FALSE;
		if (!getResourceName(propulsionName))
		{
			return FALSE;
		}

		for (numStats = 0; numStats < numPropulsionStats; numStats++)
		{
			psPropulsionStat = &asPropulsionStats[numStats];
			if (!strcmp(psPropulsionStat->pName, propulsionName))
			{
				found = TRUE;
				break;
			}
		}
		if (!found)
		{
			debug( LOG_ERROR, "Invalid propulsion name %s", propulsionName );
			abort();
			return FALSE;
		}

		//allocate the left and right propulsion IMDs
		startIMDs = psBodyStat->ppIMDList;
		psBodyStat->ppIMDList += (numStats * NUM_PROP_SIDES);
		if (strcmp(leftIMD, "0"))
		{
			*psBodyStat->ppIMDList = (iIMDShape *) resGetData("IMD", leftIMD);
			if (*psBodyStat->ppIMDList == NULL)
			{
				debug( LOG_ERROR, "Cannot find the left propulsion PIE for body %s", bodyName );
				abort();
				return FALSE;
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
				debug( LOG_ERROR, "Cannot find the right propulsion PIE for body %s", bodyName );
				abort();
				return FALSE;
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

	return(TRUE);
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
			debug( LOG_ERROR, "statsGetAudioIDFromString: couldn't get ID %d for sound %s", *piWavID, szWavName );
			abort();
			return FALSE;
		}
	}
	if ((*piWavID < 0
	  || *piWavID > ID_MAX_SOUND)
	 && *piWavID != NO_SOUND)
	{
		debug( LOG_ERROR, "statsGetAudioIDFromString: Invalid ID - %d for sound %s", *piWavID, szStatName );
		abort();
		return FALSE;
	}

	return TRUE;
}



/*Load the weapon sounds from the file exported from Access*/
BOOL loadWeaponSounds(const char *pSoundData, UDWORD bufferSize)
{
	const unsigned int NumRecords = numCR(pSoundData, bufferSize);
	SDWORD i, weaponSoundID, explosionSoundID, inc, iDum;
	char			WeaponName[MAX_NAME_SIZE];
	char			szWeaponWav[MAX_NAME_SIZE],	szExplosionWav[MAX_NAME_SIZE];
	BOOL 	Ok = TRUE;

	ASSERT( asWeaponStats != NULL, "loadWeaponSounds: Weapon stats not loaded" );

	for (i=0; i < NumRecords; i++)
	{
		WeaponName[0]     = '\0';
		szWeaponWav[0]    = '\0';
		szExplosionWav[0] = '\0';
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pSoundData,"%[^','],%[^','],%[^','],%d",
			WeaponName, szWeaponWav, szExplosionWav, &iDum);

		if ( statsGetAudioIDFromString( WeaponName, szWeaponWav, &weaponSoundID ) == FALSE )
		{
			return FALSE;
		}

		if ( statsGetAudioIDFromString( WeaponName, szExplosionWav, &explosionSoundID ) == FALSE )
		{
			return FALSE;
		}

		//find the weapon stat
		if (!getResourceName(WeaponName))
		{
			return FALSE;
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
		if (inc == (SDWORD)numWeaponStats)
		{
			debug( LOG_ERROR, "loadWeaponSounds: Weapon stat not found - %s", WeaponName );
			abort();
			Ok = FALSE;
//			return FALSE;
		}
		//increment the pointer to the start of the next record
		pSoundData = strchr(pSoundData,'\n') + 1;
	}

	return TRUE;
}

/*Load the Weapon Effect Modifiers from the file exported from Access*/
BOOL loadWeaponModifiers(const char *pWeapModData, UDWORD bufferSize)
{
	const unsigned int NumRecords = numCR(pWeapModData, bufferSize);
	PROPULSION_TYPE		propInc;
	WEAPON_EFFECT		effectInc;
	UDWORD				i, j, modifier;
	char				weaponEffectName[MAX_NAME_SIZE], propulsionName[MAX_NAME_SIZE];

	//initialise to 100%
	for (i=0; i < WE_NUMEFFECTS; i++)
	{
		for (j=0; j < NUM_PROP_TYPES; j++)
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
		effectInc = getWeaponEffect(weaponEffectName);
		if (effectInc == INVALID_WEAPON_EFFECT)
		{
			debug( LOG_ERROR, "loadWeaponModifiers: Invalid Weapon Effect - %s", weaponEffectName );
			abort();
			return FALSE;
		}
		//get the propulsion inc
		propInc = getPropulsionType(propulsionName);
		if (propInc == INVALID_PROP_TYPE)
		{
			debug( LOG_ERROR, "loadWeaponModifiers: Invalid Propulsion type - %s", propulsionName );
			abort();
			return FALSE;
		}

		if (modifier > UWORD_MAX)
		{
			debug( LOG_ERROR, "loadWeaponModifiers: modifier for effect %s, prop type %s is too large", weaponEffectName, propulsionName );
			abort();
			return FALSE;
		}
		//store in the appropriate index
		asWeaponModifier[effectInc][propInc] = (UWORD)modifier;

		//increment the pointer to the start of the next record
		pWeapModData = strchr(pWeapModData,'\n') + 1;
	}

	return TRUE;
}

/*Load the propulsion type sounds from the file exported from Access*/
BOOL loadPropulsionSounds(const char *pPropSoundData, UDWORD bufferSize)
{
	const unsigned int NumRecords = numCR(pPropSoundData, bufferSize);
	SDWORD				i, startID, idleID, moveOffID, moveID,
						hissID, shutDownID, iDum;
	char				propulsionName[MAX_NAME_SIZE], szStart[MAX_NAME_SIZE],
						szIdle[MAX_NAME_SIZE], szMoveOff[MAX_NAME_SIZE],
						szMove[MAX_NAME_SIZE], szHiss[MAX_NAME_SIZE],
						szShutDown[MAX_NAME_SIZE];
	UDWORD				type;
	PROPULSION_TYPES	*pPropType;


	ASSERT( asPropulsionTypes != NULL,
		"loadPropulsionSounds: Propulsion type stats not loaded" );

	for (i=0; i < NumRecords; i++)
	{

		propulsionName[0] = '\0';
		//read the data into the storage - the data is delimeted using comma's
		sscanf(pPropSoundData,"%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%d",
			propulsionName, szStart, szIdle, szMoveOff, szMove, szHiss, szShutDown, &iDum);

		if ( statsGetAudioIDFromString( propulsionName, szStart, &startID ) == FALSE )
		{
			return FALSE;
		}

		if ( statsGetAudioIDFromString( propulsionName, szIdle, &idleID ) == FALSE )
		{
			return FALSE;
		}

		if ( statsGetAudioIDFromString( propulsionName, szMoveOff, &moveOffID ) == FALSE )
		{
			return FALSE;
		}

		if ( statsGetAudioIDFromString( propulsionName, szMove, &moveID ) == FALSE )
		{
			return FALSE;
		}

		if ( statsGetAudioIDFromString( propulsionName, szHiss, &hissID ) == FALSE )
		{
			return FALSE;
		}

		if ( statsGetAudioIDFromString( propulsionName, szShutDown, &shutDownID ) == FALSE )
		{
			return FALSE;
		}

		type = getPropulsionType(propulsionName);
		if (type == INVALID_PROP_TYPE)
		{
			debug( LOG_ERROR, "loadPropulsionSounds: Invalid Propulsion type - %s", propulsionName );
			abort();
			return FALSE;
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

	return(TRUE);
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
	ASSERT( FALSE, "statsGetWeapon: Reference number not found in list: %x", ref );
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
	ASSERT( FALSE, "statsGetBody: Reference number not found in list: %x", ref );
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
	ASSERT( FALSE, "statsGetBrain: Reference number not found in list: %x", ref );
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
	ASSERT( FALSE, "statsGetPropulsion: Reference number not found in list: %x", ref );
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
	ASSERT( FALSE, "statsGetSensor: Reference number not found in list: %x", ref );
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
	ASSERT( FALSE, "statsGetECM: Reference number not found in list: %x", ref );
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
	ASSERT( FALSE, "statsGetRepair: Reference number not found in list: %x", ref );
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
	ASSERT( FALSE, "statsGetConstruct: Reference number not found in list: %x", ref );
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

	ASSERT( propulsionType < NUM_PROP_TYPES,
		"The propulsion type is too large" );

	pTerrainTable += (terrainType * NUM_PROP_TYPES + propulsionType);
	pTerrainTable->speedFactor = speedFactor;
}

//get the speed factor for a given terrain type and propulsion type
UDWORD getSpeedFactor(UDWORD type, UDWORD propulsionType)
{
	TERRAIN_TABLE *pTerrainTable = asTerrainTable;

	ASSERT( propulsionType < NUM_PROP_TYPES,
		"The propulsion type is too large" );

	pTerrainTable += (type * NUM_PROP_TYPES + propulsionType);

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
	ASSERT( FALSE, "Invalid stat pointer - cannot determine Stat Type" );
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
			debug( LOG_ERROR, "Invalid stat type" );
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

	ASSERT( FALSE, "Unknown Component Type" );
	return 0; // Should never get here.
}

//function to compare a value with yes/no - if neither warns player!
BOOL compareYes(char *strToCompare, char *strOwner)
{
	if (!strcmp(strToCompare, "YES"))
	{
		return TRUE;
	}
	else if (!strcmp(strToCompare, "NO"))
	{
		return FALSE;
	}
	else
	{
		//set default to FALSE but continue
		//DBERROR(("Invalid yes/no for record %s", strOwner));
		debug( LOG_ERROR, "Invalid yes/no for record %s", getName(strOwner) );
		abort();
		return FALSE;
	}
}

//get the component Inc for a stat based on the Resource name and type
//returns -1 if record not found
//used in Scripts
SDWORD	getCompFromResName(UDWORD compType, const char *pName)
{
#ifdef RESOURCE_NAMES
	if (!getResourceName(pName))
	{
		return -1;
	}
#endif

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
		debug( LOG_ERROR, "Invalid component type - game.c" );
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


//converts the name read in from Access into the name which is used in the Stat lists
BOOL getResourceName(const char *pName)
{
#ifdef RESOURCE_NAMES
	UDWORD id;

	//see if the name has a resource associated with it by trying to get the ID for the string
	if (!strresGetIDNum(psStringRes, pName, &id))
	{
		debug( LOG_ERROR, "Unable to find string resource for %s", pName );
		abort();
		return FALSE;
	}
	//get the string from the id
	strcpy(pName, strresGetString(psStringRes, id));
#endif

	return TRUE;
}

/*return the name to display for the interface - valid for OBJECTS and STATS*/
const char* getName(const char *pNameID)
{
#ifdef STORE_RESOURCE_ID
	UDWORD id;
	char *pName;
	static char Unknown[] = "Name Unknown";

	/*see if the name has a resource associated with it by trying to get
	the ID for the string*/
	if (!strresGetIDNum(psStringRes, pNameID, &id))
	{
		debug( LOG_ERROR, "Unable to find string resource for %s", pNameID );
		abort();
		return Unknown;
	}
	//get the string from the id
	pName = strresGetString(psStringRes, id);
	if (pName)
	{
		return pName;
	}
	else
	{
		return Unknown;
	}

#else
	return pNameID;
#endif
}


/*sets the tech level for the stat passed in - returns TRUE if set OK*/
BOOL setTechLevel(BASE_STATS *psStats, char *pLevel)
{
	TECH_LEVEL		techLevel = MAX_TECH_LEVELS;

	if (!strcmp(pLevel,"Level One"))
	{
		techLevel = TECH_LEVEL_ONE;
	}
	else if (!strcmp(pLevel,"Level Two"))
	{
		techLevel = TECH_LEVEL_TWO;
	}
	else if (!strcmp(pLevel,"Level Three"))
	{
		techLevel = TECH_LEVEL_THREE;
	}
	else if (!strcmp(pLevel,"Level One-Two"))
	{
		techLevel = TECH_LEVEL_ONE_TWO;
	}
	else if (!strcmp(pLevel,"Level Two-Three"))
	{
		techLevel = TECH_LEVEL_TWO_THREE;
	}
	else if (!strcmp(pLevel,"Level All"))
	{
		techLevel = TECH_LEVEL_ALL;
	}
	else if (!strcmp(pLevel,"Don't Display"))
	{
		techLevel = MAX_TECH_LEVELS;
	}
	//invalid tech level passed in
	else
	{
		ASSERT( FALSE, "Unknown Technology Level - %s", pLevel );
		return FALSE;
	}

	//store tech level in the appropriate stat
	if ((psStats->ref >= REF_BODY_START && psStats->ref <= (REF_WEAPON_START +
		REF_RANGE)) || (psStats->ref>= REF_CONSTRUCT_START && psStats->ref <=
		(REF_CONSTRUCT_START + REF_RANGE)))
	{
		((COMP_BASE_STATS *)psStats)->techLevel = techLevel;
	}
	else if (psStats->ref >= REF_STRUCTURE_START && psStats->ref <= (
		REF_STRUCTURE_START + REF_RANGE))
	{
		((STRUCTURE_STATS *)psStats)->techLevel = techLevel;
	}
	else if (psStats->ref >= REF_RESEARCH_START && psStats->ref <= (
		REF_RESEARCH_START + REF_RANGE))
	{
		((RESEARCH *)psStats)->techLevel = techLevel;
	}
	else
	{
		ASSERT( FALSE, "Invalid stat id for %s", psStats->pName );
		return FALSE;
	}
	return TRUE;
}

/*sets the store to the body size based on the name passed in - returns FALSE
if doesn't compare with any*/
BOOL getBodySize(const char *pSize, UBYTE *pStore)
{
	if (!strcmp(pSize, "LIGHT"))
	{
		*pStore = SIZE_LIGHT;
		return TRUE;
	}
	else if (!strcmp(pSize, "MEDIUM"))
	{
		*pStore = SIZE_MEDIUM;
		return TRUE;
	}
	else if (!strcmp(pSize, "HEAVY"))
	{
		*pStore = SIZE_HEAVY;
		return TRUE;
	}
	else if (!strcmp(pSize, "SUPER HEAVY"))
	{
		*pStore = SIZE_SUPER_HEAVY;
		return TRUE;
	}

	ASSERT( FALSE, "Invalid size - %s", pSize );
	return FALSE;
}

/*returns the weapon sub class based on the string name passed in */
WEAPON_SUBCLASS getWeaponSubClass(const char *pSubClass)
{
	if (!strcmp(pSubClass, "CANNON"))
	{
		return WSC_CANNON;
	}
	if (!strcmp(pSubClass, "MORTARS"))
	{
		return WSC_MORTARS;
	}
	if (!strcmp(pSubClass, "MISSILE"))
	{
		return WSC_MISSILE;
	}
	if (!strcmp(pSubClass, "ROCKET"))
	{
		return WSC_ROCKET;
	}
	if (!strcmp(pSubClass, "ENERGY"))
	{
		return WSC_ENERGY;
	}
	if (!strcmp(pSubClass, "GAUSS"))
	{
		return WSC_GAUSS;
	}
	if (!strcmp(pSubClass, "FLAME"))
	{
		return WSC_FLAME;
	}
	if (!strcmp(pSubClass, "HOWITZERS"))
	{
		return WSC_HOWITZERS;
	}
	if (!strcmp(pSubClass, "MACHINE GUN"))
	{
		return WSC_MGUN;
	}
	if (!strcmp(pSubClass, "ELECTRONIC"))
	{
		return WSC_ELECTRONIC;
	}
	if (!strcmp(pSubClass, "A-A GUN"))
	{
		return WSC_AAGUN;
	}
	if (!strcmp(pSubClass, "SLOW MISSILE"))
	{
		return WSC_SLOWMISSILE;
	}
	if (!strcmp(pSubClass, "SLOW ROCKET"))
	{
		return WSC_SLOWROCKET;
	}
	if (!strcmp(pSubClass, "LAS_SAT"))
	{
		return WSC_LAS_SAT;
	}
	if (!strcmp(pSubClass, "BOMB"))
	{
		return WSC_BOMB;
	}
	if (!strcmp(pSubClass, "COMMAND"))
	{
		return WSC_COMMAND;
	}
	if (!strcmp(pSubClass, "EMP"))
	{
		return WSC_EMP;
	}

	//problem if we've got to here
	ASSERT( FALSE, "Invalid weapon sub class - %s", pSubClass );
	return INVALID_SUBCLASS;
}

/*returns the movement model based on the string name passed in */
MOVEMENT_MODEL	getMovementModel(char *pMovement)
{
	if (!strcmp(pMovement,"DIRECT"))
	{
		return MM_DIRECT;
	}
	if (!strcmp(pMovement,"INDIRECT"))
	{
		return MM_INDIRECT;
	}
	if (!strcmp(pMovement,"HOMING-DIRECT"))
	{
		return MM_HOMINGDIRECT;
	}
	if (!strcmp(pMovement,"HOMING-INDIRECT"))
	{
		return MM_HOMINGINDIRECT;
	}
	if (!strcmp(pMovement,"ERRATIC-DIRECT"))
	{
		return MM_ERRATICDIRECT;
	}
	if (!strcmp(pMovement,"SWEEP"))
	{
		return MM_SWEEP;
	}
	//problem if we've got to here
	ASSERT( FALSE, "Invalid movement model %s", pMovement );
	return INVALID_MOVEMENT;
}


/*returns the weapon effect based on the string name passed in */
WEAPON_EFFECT getWeaponEffect(const char *pWeaponEffect)
{
	if (!strcmp(pWeaponEffect, "ANTI PERSONNEL"))
	{
		return WE_ANTI_PERSONNEL;
	}
	else if (!strcmp(pWeaponEffect, "ANTI TANK"))
	{
		return WE_ANTI_TANK;
	}
	else if (!strcmp(pWeaponEffect, "BUNKER BUSTER"))
	{
		return WE_BUNKER_BUSTER;
	}
	else if (!strcmp(pWeaponEffect, "ARTILLERY ROUND"))
	{
		return WE_ARTILLERY_ROUND;
	}
	else if (!strcmp(pWeaponEffect, "FLAMER"))
	{
		return WE_FLAMER;
	}
	else if (!strcmp(pWeaponEffect, "ANTI AIRCRAFT"))
	{
		return WE_ANTI_AIRCRAFT;
	}

	ASSERT(FALSE, "Invalid weapon effect: %s", pWeaponEffect);
	return INVALID_WEAPON_EFFECT;
}


/*
looks up the name to get the resource associated with it - or allocates space
and stores the name. Eventually ALL names will be 'resourced' for translation
*/
BOOL allocateName(char **ppStore, const char *pName)
{
#ifdef RESOURCE_NAMES

	UDWORD id;

	//see if the name has a resource associated with it by trying to get the ID for the string
	if (!strresGetIDNum(psStringRes, pName, &id))
	{
		debug( LOG_ERROR, "Unable to find string resource for %s", pName );
		abort();
		return FALSE;
	}

	//get the string from the id
	*ppStore = strresGetString(psStringRes, id);
	return TRUE;

#elif defined STORE_RESOURCE_ID
	//checks the name has been loaded as a resource and gets the storage pointer
	if (!strresGetIDString(psStringRes, pName, ppStore))
	{
		debug( LOG_ERROR, "Unable to find string resource for %s", pName );
		abort();
		return FALSE;
	}
	return TRUE;
#else
	//need to allocate space for the name
	*ppStore = (char*)malloc((strlen(pName))+1);
	if (ppStore == NULL)
	{
		debug( LOG_ERROR, "Name - Out of memory" );
		abort();
		return FALSE;
	}
	strcpy(*ppStore,pName);

	return TRUE;
#endif
}


/*Access functions for the upgradeable stats of a weapon*/
UDWORD	weaponFirePause(WEAPON_STATS *psStats, UBYTE player)
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

UDWORD	weaponShortHit(WEAPON_STATS *psStats, UBYTE player)
{
	return (psStats->shortHit + (psStats->shortHit * asWeaponUpgrade[player][
		psStats->weaponSubClass].shortHit)/100);
}

UDWORD	weaponLongHit(WEAPON_STATS *psStats, UBYTE player)
{
	return (psStats->longHit + (psStats->longHit * asWeaponUpgrade[player][
		psStats->weaponSubClass].longHit)/100);
}

UDWORD	weaponDamage(WEAPON_STATS *psStats, UBYTE player)
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

	ASSERT( FALSE,"bodyArmour() : Unknown weapon class" );
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
			return TRUE;
		}
	}
	else if(psObj->type == OBJ_STRUCTURE)
	{
		if ( ((STRUCTURE *)psObj)->numWeaps > 0 )
		{
			return TRUE;
		}
	}

	return FALSE;
}
