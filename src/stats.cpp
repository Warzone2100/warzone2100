/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
#include <algorithm>
#include <QtCore/QHash>

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
static int		*asTerrainTable;

//used to hold the modifiers cross refd by weapon effect and propulsion type
WEAPON_MODIFIER		asWeaponModifier[WE_NUMEFFECTS][PROPULSION_TYPE_NUM];
WEAPON_MODIFIER		asWeaponModifierBody[WE_NUMEFFECTS][SIZE_NUM];

/* The number of different stats stored */
UDWORD		numBodyStats;
UDWORD		numBrainStats;
UDWORD		numPropulsionStats;
UDWORD		numSensorStats;
UDWORD		numECMStats;
UDWORD		numRepairStats;
UDWORD		numWeaponStats;
UDWORD		numConstructStats;

//the max values of the stats used in the design screen
static UDWORD	maxComponentWeight;
static UDWORD	maxBodyArmour;
static UDWORD	maxBodyPower;
static UDWORD	maxBodyPoints;
static UDWORD	maxSensorRange;
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

static QHash<QString, BASE_STATS *> lookupStatPtr;

static bool getMovementModel(const char *movementModel, MOVEMENT_MODEL *model);
static bool statsGetAudioIDFromString(const QString &szStatName, const QString &szWavName, int *piWavID);

//Access functions for the max values to be used in the Design Screen
static void setMaxComponentWeight(UDWORD weight);
static void setMaxBodyArmour(UDWORD armour);
static void setMaxBodyPower(UDWORD power);
static void setMaxBodyPoints(UDWORD points);
static void setMaxSensorRange(UDWORD range);
static void setMaxECMRange(UDWORD power);
static void setMaxConstPoints(UDWORD points);
static void setMaxRepairPoints(UDWORD repair);
static void setMaxWeaponRange(UDWORD range);
static void setMaxWeaponDamage(UDWORD damage);
static void setMaxWeaponROF(UDWORD rof);
static void setMaxPropulsionSpeed(UDWORD speed);

//determine the effect this upgrade would have on the max values
static void updateMaxWeaponStats(UWORD maxValue);
static void updateMaxSensorStats(UWORD maxRange);
static void updateMaxRepairStats(UWORD maxValue);
static void updateMaxECMStats(UWORD maxValue);
static void updateMaxBodyStats(UWORD maxBody, UWORD maxPower, UWORD maxArmour);
static void updateMaxConstStats(UWORD maxValue);

static inline bool stringToEnumFindFunction(std::pair<char const *, unsigned> const &a, char const *b)
{
	return strcmp(a.first, b) < 0;
}

/***********************************************************************************
*	Dealloc the extra storage tables
***********************************************************************************/
//Deallocate the storage assigned for the Propulsion Types table
static void deallocPropulsionTypes(void)
{
	free(asPropulsionTypes);
	asPropulsionTypes = NULL;
}

//dealloc the storage assigned for the terrain table
static void deallocTerrainTable(void)
{
	free(asTerrainTable);
	asTerrainTable = NULL;
}

/*******************************************************************************
*		Generic stats macros/functions
*******************************************************************************/

/* Macro to allocate memory for a set of stats */
#define ALLOC_STATS(numEntries, list, listSize, type) \
	ASSERT((numEntries) < REF_RANGE, "Number of stats entries too large for " #type);\
	if ((list))	delete [] (list);	\
	(list) = new type[numEntries]; \
	(listSize) = (numEntries); \
	return true

/*Macro to Deallocate stats*/
#define STATS_DEALLOC(list, listSize) \
	delete [] (list); \
	listSize = 0; \
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

	// init the max values
	maxComponentWeight = maxBodyArmour = maxBodyPower =
	        maxBodyPoints = maxSensorRange = maxECMRange =
	                maxConstPoints = maxRepairPoints = maxWeaponRange = maxWeaponDamage =
	                        maxPropulsionSpeed = 0;
}

/*Deallocate all the stats assigned from input data*/
bool statsShutDown(void)
{
	lookupStatPtr.clear();

	STATS_DEALLOC(asWeaponStats, numWeaponStats);
	STATS_DEALLOC(asBrainStats, numBrainStats);
	STATS_DEALLOC(asPropulsionStats, numPropulsionStats);
	STATS_DEALLOC(asRepairStats, numRepairStats);
	STATS_DEALLOC(asConstructStats, numConstructStats);
	STATS_DEALLOC(asECMStats, numECMStats);
	STATS_DEALLOC(asSensorStats, numSensorStats);
	STATS_DEALLOC(asBodyStats, numBodyStats);
	deallocPropulsionTypes();
	deallocTerrainTable();

	return true;
}

/* Return the number of newlines in a file buffer */
UDWORD numCR(const char *pFileBuffer, UDWORD fileSize)
{
	UDWORD  lines = 0;

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
bool statsAllocWeapons(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asWeaponStats, numWeaponStats, WEAPON_STATS);
}
/* Allocate Body Stats */
bool statsAllocBody(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asBodyStats, numBodyStats, BODY_STATS);
}
/* Allocate Brain Stats */
bool statsAllocBrain(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asBrainStats, numBrainStats, BRAIN_STATS);
}
/* Allocate Propulsion Stats */
bool statsAllocPropulsion(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asPropulsionStats, numPropulsionStats, PROPULSION_STATS);
}
/* Allocate Sensor Stats */
bool statsAllocSensor(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asSensorStats, numSensorStats, SENSOR_STATS);
}
/* Allocate Ecm Stats */
bool statsAllocECM(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asECMStats, numECMStats, ECM_STATS);
}

/* Allocate Repair Stats */
bool statsAllocRepair(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asRepairStats, numRepairStats, REPAIR_STATS);
}

/* Allocate Construct Stats */
bool statsAllocConstruct(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asConstructStats, numConstructStats, CONSTRUCT_STATS);
}

/*******************************************************************************
*		Load stats functions
*******************************************************************************/

static iIMDShape *statsGetIMD(WzConfig &json, BASE_STATS *psStats, QString key, QString key2 = QString())
{
	iIMDShape *retval = NULL;
	if (json.contains(key))
	{
		QJsonValue value = json.json(key);
		if (value.isObject())
		{
			ASSERT(!key2.isEmpty(), "Cannot look up a JSON object with an empty key!");
			QJsonObject obj = value.toObject();
			if (!obj.contains(key2))
			{
				return NULL;
			}
			value = obj[key2];
		}
		retval = modelGet(value.toString());
		ASSERT(retval != NULL, "Cannot find the PIE model %s for stat %s in %s",
		       value.toString().toUtf8().constData(), getName(psStats), json.fileName().toUtf8().constData());
	}
	return retval;
}

void loadStats(WzConfig &json, BASE_STATS *psStats, int index)
{
	psStats->id = json.group();
	psStats->name = json.value("name").toString();
	psStats->index = index;
	ASSERT(!lookupStatPtr.contains(psStats->id), "Duplicate ID found! (%s)", psStats->id.toUtf8().constData());
	lookupStatPtr.insert(psStats->id, psStats);
}

static void loadCompStats(WzConfig &json, COMPONENT_STATS *psStats, int index)
{
	loadStats(json, psStats, index);
	psStats->buildPower = json.value("buildPower", 0).toUInt();
	psStats->buildPoints = json.value("buildPoints", 0).toUInt();
}

/*Load the weapon stats from the file exported from Access*/
bool loadWeaponStats(const char *pFileName)
{
	WzConfig ini(pFileName, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	statsAllocWeapons(list.size());
	// Hack to make sure ZNULLWEAPON is always first in list
	int nullweapon = list.indexOf("ZNULLWEAPON");
	ASSERT_OR_RETURN(false, nullweapon >= 0, "ZNULLWEAPON is mandatory");
	list.swap(nullweapon, 0);
	for (int i = 0; i < list.size(); ++i)
	{
		WEAPON_STATS *psStats = &asWeaponStats[i];
		QStringList flags;

		ini.beginGroup(list[i]);
		loadCompStats(ini, psStats, i);
		psStats->compType = COMP_WEAPON;

		psStats->weight = ini.value("weight", 0).toUInt();
		psStats->body = ini.value("hitpoints", 0).toUInt();
		psStats->radiusLife = ini.value("radiusLife", 0).toUInt();

		psStats->base.maxRange = ini.value("longRange").toUInt();
		psStats->base.minRange = ini.value("minRange", 0).toUInt();
		psStats->base.hitChance = ini.value("longHit", 100).toUInt();
		psStats->base.firePause = ini.value("firePause").toUInt();
		psStats->base.numRounds = ini.value("numRounds").toUInt();
		psStats->base.reloadTime = ini.value("reloadTime").toUInt();
		psStats->base.damage = ini.value("damage").toUInt();
		psStats->base.minimumDamage = ini.value("minimumDamage", 0).toInt();
		psStats->base.radius = ini.value("radius", 0).toUInt();
		psStats->base.radiusDamage = ini.value("radiusDamage", 0).toUInt();
		psStats->base.periodicalDamageTime = ini.value("periodicalDamageTime", 0).toUInt();
		psStats->base.periodicalDamage = ini.value("periodicalDamage", 0).toUInt();
		psStats->base.periodicalDamageRadius = ini.value("periodicalDamageRadius", 0).toUInt();
		// multiply time stats
		psStats->base.firePause *= WEAPON_TIME;
		psStats->base.periodicalDamageTime *= WEAPON_TIME;
		psStats->radiusLife *= WEAPON_TIME;
		psStats->base.reloadTime *= WEAPON_TIME;
		// copy for upgrades
		for (int j = 0; j < MAX_PLAYERS; j++)
		{
			psStats->upgrade[j] = psStats->base;
		}

		psStats->numExplosions = ini.value("numExplosions").toUInt();
		psStats->flightSpeed = ini.value("flightSpeed", 1).toUInt();
		psStats->rotate = ini.value("rotate").toUInt();
		psStats->minElevation = ini.value("minElevation").toInt();
		psStats->maxElevation = ini.value("maxElevation").toInt();
		psStats->recoilValue = ini.value("recoilValue").toUInt();
		psStats->effectSize = ini.value("effectSize").toUInt();
		flags = ini.value("flags", 0).toStringList();
		psStats->vtolAttackRuns = ini.value("numAttackRuns", 0).toUInt();
		psStats->designable = ini.value("designable").toBool();
		psStats->penetrate = ini.value("penetrate", false).toBool();
		// weapon size limitation
		int weaponSize = ini.value("weaponSize", WEAPON_SIZE_ANY).toInt();
		ASSERT(weaponSize <= WEAPON_SIZE_ANY, "Bad weapon size for %s", list[i].toUtf8().constData());
		psStats->weaponSize = (WEAPON_SIZE)weaponSize;

		ASSERT(psStats->flightSpeed > 0, "Invalid flight speed for %s", list[i].toUtf8().constData());

		psStats->ref = REF_WEAPON_START + i;

		//get the IMD for the component
		psStats->pIMD = statsGetIMD(ini, psStats, "model");
		psStats->pMountGraphic = statsGetIMD(ini, psStats, "mountModel");
		if (GetGameMode() == GS_NORMAL)
		{
			psStats->pMuzzleGraphic = statsGetIMD(ini, psStats, "muzzleGfx");
			psStats->pInFlightGraphic = statsGetIMD(ini, psStats, "flightGfx");
			psStats->pTargetHitGraphic = statsGetIMD(ini, psStats, "hitGfx");
			psStats->pTargetMissGraphic = statsGetIMD(ini, psStats, "missGfx");
			psStats->pWaterHitGraphic = statsGetIMD(ini, psStats, "waterGfx");
			psStats->pTrailGraphic = statsGetIMD(ini, psStats, "trailGfx");
		}
		psStats->fireOnMove = ini.value("fireOnMove", true).toBool();

		//set the weapon class
		if (!getWeaponClass(ini.value("weaponClass").toString(), &psStats->weaponClass))
		{
			debug(LOG_ERROR, "Invalid weapon class for weapon %s - assuming KINETIC", getName(psStats));
			psStats->weaponClass = WC_KINETIC;
		}

		//set the subClass
		if (!getWeaponSubClass(ini.value("weaponSubClass").toString().toUtf8().data(), &psStats->weaponSubClass))
		{
			return false;
		}

		// set max extra weapon range on misses, make this modifiable one day by mod makers
		if (psStats->weaponSubClass == WSC_MGUN || psStats->weaponSubClass == WSC_COMMAND)
		{
			psStats->distanceExtensionFactor = 120;
		}
		else if (psStats->weaponSubClass == WSC_AAGUN)
		{
			psStats->distanceExtensionFactor = 100;
		}
		else // default
		{
			psStats->distanceExtensionFactor = 150;
		}

		//set the weapon effect
		if (!getWeaponEffect(ini.value("weaponEffect").toString().toUtf8().constData(), &psStats->weaponEffect))
		{
			debug(LOG_FATAL, "loadWepaonStats: Invalid weapon effect for weapon %s", getName(psStats));
			return false;
		}

		//set periodical damage weapon class
		QString periodicalDamageWeaponClass = ini.value("periodicalDamageWeaponClass", "").toString();
		if (periodicalDamageWeaponClass.compare("") == 0)
		{
			//was not setted in ini - use default value
			psStats->periodicalDamageWeaponClass = psStats->weaponClass;
		}
		else if (!getWeaponClass(periodicalDamageWeaponClass, &psStats->periodicalDamageWeaponClass))
		{
			debug(LOG_ERROR, "Invalid periodicalDamageWeaponClass for weapon %s - assuming same class as weapon", getName(psStats));
			psStats->periodicalDamageWeaponClass = psStats->weaponClass;
		}

		//set periodical damage weapon subclass
		QString periodicalDamageWeaponSubClass = ini.value("periodicalDamageWeaponSubClass", "").toString();
		if (periodicalDamageWeaponSubClass.compare("") == 0)
		{
			//was not setted in ini - use default value
			psStats->periodicalDamageWeaponSubClass = psStats->weaponSubClass;
		}
		else if (!getWeaponSubClass(periodicalDamageWeaponSubClass.toUtf8().data(), &psStats->periodicalDamageWeaponSubClass))
		{
			debug(LOG_ERROR, "Invalid periodicalDamageWeaponSubClass for weapon %s - assuming same subclass as weapon", getName(psStats));
			psStats->periodicalDamageWeaponSubClass = psStats->weaponSubClass;
		}

		//set periodical damage weapon effect
		QString periodicalDamageWeaponEffect = ini.value("periodicalDamageWeaponEffect", "").toString();
		if (periodicalDamageWeaponEffect.compare("") == 0)
		{
			//was not setted in ini - use default value
			psStats->periodicalDamageWeaponEffect = psStats->weaponEffect;
		}
		else if (!getWeaponEffect(periodicalDamageWeaponEffect.toUtf8().data(), &psStats->periodicalDamageWeaponEffect))
		{
			debug(LOG_ERROR, "Invalid periodicalDamageWeaponEffect for weapon %s - assuming same effect as weapon", getName(psStats));
			psStats->periodicalDamageWeaponEffect = psStats->weaponEffect;
		}

		//set the movement model
		if (!getMovementModel(ini.value("movement").toString().toUtf8().constData(), &psStats->movementModel))
		{
			return false;
		}

		// set the face Player value
		psStats->facePlayer = ini.value("facePlayer", false).toBool();

		// set the In flight face Player value
		psStats->faceInFlight = ini.value("faceInFlight", false).toBool();

		//set the light world value
		psStats->lightWorld = ini.value("lightWorld", false).toBool();

		// interpret flags
		psStats->surfaceToAir = SHOOT_ON_GROUND; // default
		if (flags.contains("AirOnly", Qt::CaseInsensitive))
		{
			psStats->surfaceToAir = SHOOT_IN_AIR;
		}
		else if (flags.contains("ShootAir", Qt::CaseInsensitive))
		{
			psStats->surfaceToAir |= SHOOT_IN_AIR;
		}

		//set the weapon sounds to default value
		psStats->iAudioFireID = NO_SOUND;
		psStats->iAudioImpactID = NO_SOUND;

		// load sounds
		int weaponSoundID, explosionSoundID;
		QString szWeaponWav = ini.value("weaponWav", "-1").toString();
		QString szExplosionWav = ini.value("explosionWav", "-1").toString();
		bool result = statsGetAudioIDFromString(list[i], szWeaponWav, &weaponSoundID);
		ASSERT_OR_RETURN(false, result, "Weapon sound %s not found for %s", szWeaponWav.toUtf8().constData(), getName(psStats));
		result = statsGetAudioIDFromString(list[i], szExplosionWav, &explosionSoundID);
		ASSERT_OR_RETURN(false, result, "Explosion sound %s not found for %s", szExplosionWav.toUtf8().constData(), getName(psStats));
		psStats->iAudioFireID = weaponSoundID;
		psStats->iAudioImpactID = explosionSoundID;

		// Set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxWeaponRange(psStats->base.maxRange);
			setMaxWeaponDamage(psStats->base.damage);
			setMaxWeaponROF(weaponROF(psStats, 0));
			setMaxComponentWeight(psStats->weight);
		}

		ini.endGroup();
	}

	return true;
}

bool loadBodyStats(const char *pFileName)
{
	WzConfig ini(pFileName, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	statsAllocBody(list.size());
	// Hack to make sure ZNULLBODY is always first in list
	int nullbody = list.indexOf("ZNULLBODY");
	ASSERT_OR_RETURN(false, nullbody >= 0, "ZNULLBODY is mandatory");
	list.swap(nullbody, 0);
	for (int i = 0; i < list.size(); ++i)
	{
		BODY_STATS *psStats = &asBodyStats[i];

		ini.beginGroup(list[i]);
		loadCompStats(ini, psStats, i);
		psStats->compType = COMP_BODY;

		psStats->weight = ini.value("weight", 0).toInt();
		psStats->body = ini.value("hitpoints").toInt();
		psStats->weaponSlots = ini.value("weaponSlots").toInt();
		psStats->designable = ini.value("designable", false).toBool();
		psStats->bodyClass = ini.value("class").toString();
		psStats->base.thermal = ini.value("armourHeat").toInt();
		psStats->base.armour = ini.value("armourKinetic").toInt();
		psStats->base.power = ini.value("powerOutput").toInt();
		psStats->base.body = psStats->body; // special exception hack
		psStats->base.resistance = ini.value("resistance", 30).toInt();
		for (int j = 0; j < MAX_PLAYERS; j++)
		{
			psStats->upgrade[j] = psStats->base;
		}
		QString dtype = ini.value("droidType", "DROID").toString();
		psStats->droidTypeOverride = DROID_DEFAULT;
		if (dtype.compare("PERSON") == 0)
		{
			psStats->droidTypeOverride = DROID_PERSON;
		}
		else if (dtype.compare("TRANSPORTER") == 0)
		{
			psStats->droidTypeOverride = DROID_TRANSPORTER;
		}
		else if (dtype.compare("CYBORG") == 0)
		{
			psStats->droidTypeOverride = DROID_CYBORG;
		}
		else if (dtype.compare("CYBORG_SUPER") == 0)
		{
			psStats->droidTypeOverride = DROID_CYBORG_SUPER;
		}
		else if (dtype.compare("CYBORG_CONSTRUCT") == 0)
		{
			psStats->droidTypeOverride = DROID_CYBORG_CONSTRUCT;
		}
		else if (dtype.compare("CYBORG_REPAIR") == 0)
		{
			psStats->droidTypeOverride = DROID_CYBORG_REPAIR;
		}
		psStats->ref = REF_BODY_START + i;
		if (!getBodySize(ini.value("size").toString().toUtf8().constData(), &psStats->size))
		{
			ASSERT(false, "Unknown body size for %s", getName(psStats));
			return false;
		}
		psStats->pIMD = statsGetIMD(ini, psStats, "model");

		ini.endGroup();

		//set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxBodyArmour(psStats->base.armour);
			setMaxBodyArmour(psStats->base.thermal);
			setMaxBodyPower(psStats->base.power);
			setMaxBodyPoints(psStats->body);
			setMaxComponentWeight(psStats->weight);
		}
	}

	// now get the extra stuff ... hack it together with above later, moved here from
	// separate function

	// allocate space
	for (int numStats = 0; numStats < numBodyStats; ++numStats)
	{
		BODY_STATS *psBodyStat = &asBodyStats[numStats];
		psBodyStat->ppIMDList.resize(numPropulsionStats * NUM_PROP_SIDES, NULL);
		psBodyStat->ppMoveIMDList.resize(numPropulsionStats * NUM_PROP_SIDES, NULL);
		psBodyStat->ppStillIMDList.resize(numPropulsionStats * NUM_PROP_SIDES, NULL);
	}
	for (int i = 0; i < list.size(); ++i)
	{
		QString propulsionName, leftIMD, rightIMD;
		BODY_STATS *psBodyStat = NULL;
		int numStats;

		ini.beginGroup(list[i]);
		if (!ini.contains("propulsionExtraModels"))
		{
			ini.endGroup();
			continue;
		}
		ini.beginGroup("propulsionExtraModels");
		//get the body stats
		for (numStats = 0; numStats < numBodyStats; ++numStats)
		{
			psBodyStat = &asBodyStats[numStats];
			if (list[i].compare(psBodyStat->id) == 0)
			{
				break;
			}
		}
		if (numStats == numBodyStats) // not found
		{
			debug(LOG_FATAL, "Invalid body name %s", list[i].toUtf8().constData());
			return false;
		}
		QStringList keys = ini.childKeys();
		for (int j = 0; j < keys.size(); j++)
		{
			for (numStats = 0; numStats < numPropulsionStats; numStats++)
			{
				PROPULSION_STATS *psPropulsionStat = &asPropulsionStats[numStats];
				if (keys[j].compare(psPropulsionStat->id) == 0)
				{
					break;
				}
			}
			if (numStats == numPropulsionStats)
			{
				debug(LOG_FATAL, "Invalid propulsion name %s", keys[j].toUtf8().constData());
				return false;
			}
			//allocate the left and right propulsion IMDs + movement and standing still animations
			psBodyStat->ppIMDList[numStats * NUM_PROP_SIDES + LEFT_PROP] = statsGetIMD(ini, psBodyStat, keys[j], "left");
			psBodyStat->ppIMDList[numStats * NUM_PROP_SIDES + RIGHT_PROP] = statsGetIMD(ini, psBodyStat, keys[j], "right");
			psBodyStat->ppMoveIMDList[numStats] = statsGetIMD(ini, psBodyStat, keys[j], "moving");
			psBodyStat->ppStillIMDList[numStats] = statsGetIMD(ini, psBodyStat, keys[j], "still");
		}
		ini.endGroup();
		ini.endGroup();
	}

	return true;
}

/*Load the Brain stats from the file exported from Access*/
bool loadBrainStats(const char *pFileName)
{
	WzConfig ini(pFileName, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	statsAllocBrain(list.size());
	// Hack to make sure ZNULLBRAIN is always first in list
	int nullbrain = list.indexOf("ZNULLBRAIN");
	ASSERT_OR_RETURN(false, nullbrain >= 0, "ZNULLBRAIN is mandatory");
	list.swap(nullbrain, 0);
	for (int i = 0; i < list.size(); ++i)
	{
		BRAIN_STATS *psStats = &asBrainStats[i];

		ini.beginGroup(list[i]);
		loadCompStats(ini, psStats, i);
		psStats->compType = COMP_BRAIN;

		psStats->weight = ini.value("weight", 0).toInt();
		psStats->maxDroids = ini.value("maxDroids").toInt();
		psStats->maxDroidsMult = ini.value("maxDroidsMult").toInt();
		psStats->ref = REF_BRAIN_START + i;

		// check weapon attached
		psStats->psWeaponStat = NULL;
		if (ini.contains("turret"))
		{
			int weapon = getCompFromName(COMP_WEAPON, ini.value("turret").toString());
			ASSERT_OR_RETURN(false, weapon >= 0, "Unable to find weapon for brain %s", getName(psStats));
			psStats->psWeaponStat = asWeaponStats + weapon;
		}
		psStats->designable = ini.value("designable", false).toBool();
		ini.endGroup();
	}
	return true;
}


/*returns the propulsion type based on the string name passed in */
bool getPropulsionType(const char *typeName, PROPULSION_TYPE *type)
{
	if (strcmp(typeName, "Wheeled") == 0)
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
	else
	{
		debug(LOG_ERROR, "getPropulsionType: Invalid Propulsion type %s - assuming Hover", typeName);
		*type = PROPULSION_TYPE_HOVER;
	}

	return true;
}

/*Load the Propulsion stats from the file exported from Access*/
bool loadPropulsionStats(const char *pFileName)
{
	WzConfig ini(pFileName, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	statsAllocPropulsion(list.size());
	// Hack to make sure ZNULLPROP is always first in list
	int nullprop = list.indexOf("ZNULLPROP");
	ASSERT_OR_RETURN(false, nullprop >= 0, "ZNULLPROP is mandatory");
	list.swap(nullprop, 0);
	for (int i = 0; i < list.size(); ++i)
	{
		PROPULSION_STATS *psStats = &asPropulsionStats[i];

		ini.beginGroup(list[i]);
		loadCompStats(ini, psStats, i);
		psStats->compType = COMP_PROPULSION;

		psStats->weight = ini.value("weight").toInt();
		psStats->body = ini.value("hitpoints").toInt();
		psStats->maxSpeed = ini.value("speed").toInt();
		psStats->designable = ini.value("designable", false).toBool();
		psStats->ref = REF_PROPULSION_START + i;
		psStats->turnSpeed = ini.value("turnSpeed", DEG(1) / 3).toInt();
		psStats->spinSpeed = ini.value("spinSpeed", DEG(3) / 4).toInt();
		psStats->spinAngle = ini.value("spinAngle", 180).toInt();
		psStats->acceleration = ini.value("acceleration", 250).toInt();
		psStats->deceleration = ini.value("deceleration", 800).toInt();
		psStats->skidDeceleration = ini.value("skidDeceleration", 600).toInt();
		psStats->pIMD = NULL;
		psStats->pIMD = statsGetIMD(ini, psStats, "model");
		if (!getPropulsionType(ini.value("type").toString().toUtf8().constData(), &psStats->propulsionType))
		{
			debug(LOG_FATAL, "loadPropulsionStats: Invalid Propulsion type for %s", getName(psStats));
			return false;
		}
		ini.endGroup();

		// set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxPropulsionSpeed(psStats->maxSpeed);
		}
	}

	/* since propulsion weight is a multiple of body weight we may need to adjust the max component weight value */
	if (asBodyStats && asPropulsionStats)	// check we've loaded them both in
	{
		//check against each body stat
		for (int i = 0; i < numBodyStats; ++i)
		{
			//check stat is designable
			if (asBodyStats[i].designable)
			{
				//check against each propulsion stat
				for (int j = 0; j < numPropulsionStats; ++j)
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
bool loadSensorStats(const char *pFileName)
{
	WzConfig ini(pFileName, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	statsAllocSensor(list.size());
	// Hack to make sure ZNULLSENSOR is always first in list
	int nullsensor = list.indexOf("ZNULLSENSOR");
	ASSERT_OR_RETURN(false, nullsensor >= 0, "ZNULLSENSOR is mandatory");
	list.swap(nullsensor, 0);
	for (int i = 0; i < list.size(); ++i)
	{
		SENSOR_STATS *psStats = &asSensorStats[i];

		ini.beginGroup(list[i]);
		loadCompStats(ini, psStats, i);
		psStats->compType = COMP_SENSOR;

		psStats->weight = ini.value("weight", 0).toInt();
		psStats->body = ini.value("hitpoints", 0).toInt();
		psStats->base.range = ini.value("range").toInt();
		for (int j = 0; j < MAX_PLAYERS; j++)
		{
			psStats->upgrade[j].range = psStats->base.range;
		}
		psStats->time = ini.value("time").toInt();
		psStats->designable = ini.value("designable", false).toBool();

		psStats->ref = REF_SENSOR_START + i;

		QString location = ini.value("location").toString();
		if (location.compare("DEFAULT") == 0)
		{
			psStats->location = LOC_DEFAULT;
		}
		else if (location.compare("TURRET") == 0)
		{
			psStats->location = LOC_TURRET;
		}
		else
		{
			ASSERT(false, "Invalid Sensor location");
		}
		QString type = ini.value("type").toString();
		if (type.compare("STANDARD") == 0)
		{
			psStats->type = STANDARD_SENSOR;
		}
		else if (type.compare("INDIRECT CB") == 0)
		{
			psStats->type = INDIRECT_CB_SENSOR;
		}
		else if (type.compare("VTOL CB") == 0)
		{
			psStats->type = VTOL_CB_SENSOR;
		}
		else if (type.compare("VTOL INTERCEPT") == 0)
		{
			psStats->type = VTOL_INTERCEPT_SENSOR;
		}
		else if (type.compare("SUPER") == 0)
		{
			psStats->type = SUPER_SENSOR;
		}
		else if (type.compare("RADAR DETECTOR") == 0)
		{
			psStats->type = RADAR_DETECTOR_SENSOR;
		}
		else
		{
			ASSERT(false, "Invalid Sensor type");
		}
		//multiply time stats
		psStats->time *= WEAPON_TIME;

		//get the IMD for the component
		psStats->pIMD = statsGetIMD(ini, psStats, "sensorModel");
		psStats->pMountGraphic = statsGetIMD(ini, psStats, "mountModel");

		ini.endGroup();

		// set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxSensorRange(psStats->base.range);
			setMaxComponentWeight(psStats->weight);
		}

	}
	return true;
}

/*Load the ECM stats from the file exported from Access*/
bool loadECMStats(const char *pFileName)
{
	WzConfig ini(pFileName, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	statsAllocECM(list.size());
	// Hack to make sure ZNULLECM is always first in list
	int nullecm = list.indexOf("ZNULLECM");
	ASSERT_OR_RETURN(false, nullecm >= 0, "ZNULLECM is mandatory");
	list.swap(nullecm, 0);
	for (int i = 0; i < list.size(); ++i)
	{
		ECM_STATS *psStats = &asECMStats[i];

		ini.beginGroup(list[i]);
		loadCompStats(ini, psStats, i);
		psStats->compType = COMP_ECM;

		psStats->weight = ini.value("weight", 0).toInt();
		psStats->body = ini.value("hitpoints", 0).toInt();
		psStats->base.range = ini.value("range").toInt();
		for (int j = 0; j < MAX_PLAYERS; j++)
		{
			psStats->upgrade[j].range = psStats->base.range;
		}
		psStats->designable = ini.value("designable", false).toBool();

		psStats->ref = REF_ECM_START + i;

		QString location = ini.value("location").toString();
		if (location.compare("DEFAULT") == 0)
		{
			psStats->location = LOC_DEFAULT;
		}
		else if (location.compare("TURRET") == 0)
		{
			psStats->location = LOC_TURRET;
		}
		else
		{
			ASSERT(false, "Invalid ECM location");
		}

		//get the IMD for the component
		psStats->pIMD = statsGetIMD(ini, psStats, "sensorModel");
		psStats->pMountGraphic = statsGetIMD(ini, psStats, "mountModel");

		ini.endGroup();

		// Set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxECMRange(psStats->base.range);
			setMaxComponentWeight(psStats->weight);
		}
	}
	return true;
}

/*Load the Repair stats from the file exported from Access*/
bool loadRepairStats(const char *pFileName)
{
	WzConfig ini(pFileName, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	statsAllocRepair(list.size());
	// Hack to make sure ZNULLREPAIR is always first in list
	int nullrepair = list.indexOf("ZNULLREPAIR");
	ASSERT_OR_RETURN(false, nullrepair >= 0, "ZNULLREPAIR is mandatory");
	list.swap(nullrepair, 0);
	for (int i = 0; i < list.size(); ++i)
	{
		REPAIR_STATS *psStats = &asRepairStats[i];

		ini.beginGroup(list[i]);
		loadCompStats(ini, psStats, i);
		psStats->compType = COMP_REPAIRUNIT;

		psStats->weight = ini.value("weight", 0).toInt();
		psStats->base.repairPoints = ini.value("repairPoints").toInt();
		for (int j = 0; j < MAX_PLAYERS; j++)
		{
			psStats->upgrade[j].repairPoints = psStats->base.repairPoints;
		}
		psStats->time = ini.value("time", 0).toInt() * WEAPON_TIME;
		psStats->designable = ini.value("designable", false).toBool();

		psStats->ref = REF_REPAIR_START + i;

		QString location = ini.value("location").toString();
		if (location.compare("DEFAULT") == 0)
		{
			psStats->location = LOC_DEFAULT;
		}
		else if (location.compare("TURRET") == 0)
		{
			psStats->location = LOC_TURRET;
		}
		else
		{
			ASSERT(false, "Invalid Repair location");
		}

		//check its not 0 since we will be dividing by it at a later stage
		ASSERT_OR_RETURN(false, psStats->time > 0, "Repair delay cannot be zero for %s", getName(psStats));

		//get the IMD for the component
		psStats->pIMD = statsGetIMD(ini, psStats, "model");
		psStats->pMountGraphic = statsGetIMD(ini, psStats, "mountModel");

		ini.endGroup();

		//set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxRepairPoints(psStats->base.repairPoints);
			setMaxComponentWeight(psStats->weight);
		}
	}
	return true;
}

/*Load the Construct stats from the file exported from Access*/
bool loadConstructStats(const char *pFileName)
{
	WzConfig ini(pFileName, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	statsAllocConstruct(list.size());
	// Hack to make sure ZNULLCONSTRUCT is always first in list
	int nullconstruct = list.indexOf("ZNULLCONSTRUCT");
	ASSERT_OR_RETURN(false, nullconstruct >= 0, "ZNULLCONSTRUCT is mandatory");
	list.swap(nullconstruct, 0);
	for (int i = 0; i < list.size(); ++i)
	{
		CONSTRUCT_STATS *psStats = &asConstructStats[i];

		ini.beginGroup(list[i]);
		loadCompStats(ini, psStats, i);
		psStats->compType = COMP_CONSTRUCT;

		psStats->weight = ini.value("weight", 0).toInt();
		psStats->body = ini.value("hitpoints", 0).toInt();
		psStats->base.constructPoints = ini.value("constructPoints").toInt();
		for (int j = 0; j < MAX_PLAYERS; j++)
		{
			psStats->upgrade[j].constructPoints = psStats->base.constructPoints;
		}
		psStats->designable = ini.value("designable", false).toBool();
		psStats->ref = REF_CONSTRUCT_START + i;

		//get the IMD for the component
		psStats->pIMD = statsGetIMD(ini, psStats, "sensorModel");
		psStats->pMountGraphic = statsGetIMD(ini, psStats, "mountModel");

		ini.endGroup();

		// Set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxConstPoints(psStats->base.constructPoints);
			setMaxComponentWeight(psStats->weight);
		}
	}
	return true;
}


/*Load the Propulsion Types from the file exported from Access*/
bool loadPropulsionTypes(const char *pFileName)
{
	const unsigned int NumTypes = PROPULSION_TYPE_NUM;
	PROPULSION_TYPES *pPropType;
	unsigned int multiplier;
	PROPULSION_TYPE type;

	//allocate storage for the stats
	asPropulsionTypes = (PROPULSION_TYPES *)malloc(sizeof(PROPULSION_TYPES) * NumTypes);
	memset(asPropulsionTypes, 0, (sizeof(PROPULSION_TYPES)*NumTypes));
	WzConfig ini(pFileName, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();

	for (int i = 0; i < NumTypes; ++i)
	{
		ini.beginGroup(list[i]);
		multiplier = ini.value("multiplier").toInt();

		//set the pointer for this record based on the name
		if (!getPropulsionType(list[i].toUtf8().constData(), &type))
		{
			debug(LOG_FATAL, "Invalid Propulsion type - %s", list[i].toUtf8().constData());
			return false;
		}

		pPropType = asPropulsionTypes + type;

		QString flightName = ini.value("flightName").toString();
		if (flightName.compare("GROUND") == 0)
		{
			pPropType->travel = GROUND;
		}
		else if (flightName.compare("AIR") == 0)
		{
			pPropType->travel = AIR;
		}
		else
		{
			ASSERT(false, "Invalid travel type for Propulsion");
		}

		//don't care about this anymore! AB FRIDAY 13/11/98
		//want it back again! AB 27/11/98
		if (multiplier > UWORD_MAX)
		{
			ASSERT(false, "loadPropulsionTypes: power Ratio multiplier too high");
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

		ini.endGroup();
	}

	return true;
}

/*Load the Terrain Table from the file exported from Access*/
bool loadTerrainTable(const char *pFileName)
{
	asTerrainTable = (int *)malloc(sizeof(*asTerrainTable) * PROPULSION_TYPE_NUM * TER_MAX);
	WzConfig ini(pFileName, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		int terrainType = list[i].toUInt();
		QStringList speedFactors = ini.value("speedFactor").toStringList();
		for (int j = 0; j < PROPULSION_TYPE_NUM; j++)
		{
			asTerrainTable[terrainType * PROPULSION_TYPE_NUM + j] = speedFactors[j].toUInt();
		}
		ini.endGroup();
	}
	return true;
}

static bool statsGetAudioIDFromString(const QString &szStatName, const QString &szWavName, int *piWavID)
{
	if (szWavName.compare("-1") == 0)
	{
		*piWavID = NO_SOUND;
	}
	else if ((*piWavID = audio_GetIDFromStr(szWavName.toUtf8().constData())) == NO_SOUND)
	{
		debug(LOG_FATAL, "Could not get ID %d for sound %s", *piWavID, szWavName.toUtf8().constData());
		return false;
	}
	if ((*piWavID < 0 || *piWavID > ID_MAX_SOUND) && *piWavID != NO_SOUND)
	{
		debug(LOG_FATAL, "Invalid ID - %d for sound %s", *piWavID, szStatName.toUtf8().constData());
		return false;
	}
	return true;
}

bool loadWeaponModifiers(const char *pFileName)
{
	//initialise to 100%
	for (int i = 0; i < WE_NUMEFFECTS; i++)
	{
		for (int j = 0; j < PROPULSION_TYPE_NUM; j++)
		{
			asWeaponModifier[i][j] = 100;
		}
		for (int j = 0; j < SIZE_NUM; j++)
		{
			asWeaponModifierBody[i][j] = 100;
		}
	}
	WzConfig ini(pFileName, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); i++)
	{
		WEAPON_EFFECT effectInc;
		PROPULSION_TYPE propInc;

		ini.beginGroup(list[i]);
		//get the weapon effect inc
		if (!getWeaponEffect(list[i].toUtf8().constData(), &effectInc))
		{
			debug(LOG_FATAL, "Invalid Weapon Effect - %s", list[i].toUtf8().constData());
			continue;
		}
		QStringList keys = ini.childKeys();
		for (int j = 0; j < keys.size(); j++)
		{
			int modifier = ini.value(keys.at(j)).toInt();
			if (!getPropulsionType(keys.at(j).toUtf8().data(), &propInc))
			{
				// If not propulsion, must be body
				BODY_SIZE body = SIZE_NUM;
				if (!getBodySize(keys.at(j).toUtf8().data(), &body))
				{
					debug(LOG_FATAL, "Invalid Propulsion or Body type - %s", keys.at(j).toUtf8().constData());
					continue;
				}
				asWeaponModifierBody[effectInc][body] = modifier;
			}
			else // is propulsion
			{
				asWeaponModifier[effectInc][propInc] = modifier;
			}
		}
		ini.endGroup();
	}
	return true;
}

/*Load the propulsion type sounds from the file exported from Access*/
bool loadPropulsionSounds(const char *pFileName)
{
	SDWORD	i, startID, idleID, moveOffID, moveID, hissID, shutDownID;
	PROPULSION_TYPE type;
	PROPULSION_TYPES *pPropType;

	ASSERT(asPropulsionTypes != NULL, "loadPropulsionSounds: Propulsion type stats not loaded");

	WzConfig ini(pFileName, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	for (i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		if (!statsGetAudioIDFromString(list[i], ini.value("szStart").toString(), &startID))
		{
			return false;
		}
		if (!statsGetAudioIDFromString(list[i], ini.value("szIdle").toString(), &idleID))
		{
			return false;
		}
		if (!statsGetAudioIDFromString(list[i], ini.value("szMoveOff").toString(), &moveOffID))
		{
			return false;
		}
		if (!statsGetAudioIDFromString(list[i], ini.value("szMove").toString(), &moveID))
		{
			return false;
		}
		if (!statsGetAudioIDFromString(list[i], ini.value("szHiss").toString(), &hissID))
		{
			return false;
		}
		if (!statsGetAudioIDFromString(list[i], ini.value("szShutDown").toString(), &shutDownID))
		{
			return false;
		}
		if (!getPropulsionType(list[i].toUtf8().constData(), &type))
		{
			debug(LOG_FATAL, "Invalid Propulsion type - %s", list[i].toUtf8().constData());
			return false;
		}
		pPropType = asPropulsionTypes + type;
		pPropType->startID = (SWORD)startID;
		pPropType->idleID = (SWORD)idleID;
		pPropType->moveOffID = (SWORD)moveOffID;
		pPropType->moveID = (SWORD)moveID;
		pPropType->hissID = (SWORD)hissID;
		pPropType->shutDownID = (SWORD)shutDownID;

		ini.endGroup();
	}

	return(true);
}

//get the speed factor for a given terrain type and propulsion type
UDWORD getSpeedFactor(UDWORD type, UDWORD propulsionType)
{
	ASSERT(propulsionType < PROPULSION_TYPE_NUM, "The propulsion type is too large");
	return asTerrainTable[type * PROPULSION_TYPE_NUM + propulsionType];
}

//return the type of stat this stat is!
UDWORD statType(UDWORD ref)
{
	if (ref >= REF_BODY_START && ref < REF_BODY_START +
	    REF_RANGE)
	{
		return COMP_BODY;
	}
	if (ref >= REF_BRAIN_START && ref < REF_BRAIN_START +
	    REF_RANGE)
	{
		return COMP_BRAIN;
	}
	if (ref >= REF_PROPULSION_START && ref <
	    REF_PROPULSION_START + REF_RANGE)
	{
		return COMP_PROPULSION;
	}
	if (ref >= REF_SENSOR_START && ref < REF_SENSOR_START +
	    REF_RANGE)
	{
		return COMP_SENSOR;
	}
	if (ref >= REF_ECM_START && ref < REF_ECM_START +
	    REF_RANGE)
	{
		return COMP_ECM;
	}
	if (ref >= REF_REPAIR_START && ref < REF_REPAIR_START +
	    REF_RANGE)
	{
		return COMP_REPAIRUNIT;
	}
	if (ref >= REF_WEAPON_START && ref < REF_WEAPON_START +
	    REF_RANGE)
	{
		return COMP_WEAPON;
	}
	if (ref >= REF_CONSTRUCT_START && ref < REF_CONSTRUCT_START +
	    REF_RANGE)
	{
		return COMP_CONSTRUCT;
	}
	//else
	ASSERT(false, "Invalid stat pointer - cannot determine Stat Type");
	return COMP_NUMCOMPONENTS;
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
			debug(LOG_FATAL, "Invalid stat type");
			start = 0;
		}
	}
	return start;
}

int getCompFromName(COMPONENT_TYPE compType, const QString &name)
{
	return getCompFromID(compType, name);
}

int getCompFromID(COMPONENT_TYPE compType, const QString &name)
{
	COMPONENT_STATS *psComp = (COMPONENT_STATS *)lookupStatPtr.value(name, NULL);
	ASSERT_OR_RETURN(-1, psComp, "No such component ID [%s] found", name.toUtf8().constData());
	ASSERT_OR_RETURN(-1, compType == psComp->compType, "Wrong component type for ID %s", name.toUtf8().constData());
	return psComp->index;
}

/// Get the component for a stat based on the name alone.
/// Returns NULL if record not found
COMPONENT_STATS *getCompStatsFromName(const QString &name)
{
	COMPONENT_STATS *psComp = (COMPONENT_STATS *)lookupStatPtr.value(name, NULL);
	/*if (!psComp)
	{
		debug(LOG_ERROR, "Not found: %s", name.toUtf8().constData());
		foreach(BASE_STATS *psStat, lookupStatPtr)
		{
			debug(LOG_ERROR, "    %s", psStat->name.toUtf8().constData());
		}
	}*/
	return psComp;
}

/*sets the store to the body size based on the name passed in - returns false
if doesn't compare with any*/
bool getBodySize(const char *pSize, BODY_SIZE *pStore)
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

	ASSERT(false, "Invalid size - %s", pSize);
	return false;
}

/*returns the weapon sub class based on the string name passed in */
bool getWeaponSubClass(const char *subClass, WEAPON_SUBCLASS *wclass)
{
	if (strcmp(subClass, "CANNON") == 0)
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

/*returns the weapon sub class based on the string name passed in */
const char *getWeaponSubClass(WEAPON_SUBCLASS wclass)
{
	switch (wclass)
	{
	case WSC_CANNON: return "CANNON";
	case WSC_MORTARS: return "MORTARS";
	case WSC_MISSILE: return "MISSILE";
	case WSC_ROCKET: return "ROCKET";
	case WSC_ENERGY: return "ENERGY";
	case WSC_GAUSS: return "GAUSS";
	case WSC_FLAME: return "FLAME";
	case WSC_HOWITZERS: return "HOWITZERS";
	case WSC_MGUN: return "MACHINE GUN";
	case WSC_ELECTRONIC: return "ELECTRONIC";
	case WSC_AAGUN: return "A-A GUN";
	case WSC_SLOWMISSILE: return "SLOW MISSILE";
	case WSC_SLOWROCKET: return "SLOW ROCKET";
	case WSC_LAS_SAT: return "LAS_SAT";
	case WSC_BOMB: return "BOMB";
	case WSC_COMMAND: return "COMMAND";
	case WSC_EMP: return "EMP";
	case WSC_NUM_WEAPON_SUBCLASSES: break;
	}
	ASSERT(false, "No such weapon subclass");
	return "Bad weapon subclass";
}

/*returns the movement model based on the string name passed in */
bool getMovementModel(const char *movementModel, MOVEMENT_MODEL *model)
{
	if (strcmp(movementModel, "DIRECT") == 0)
	{
		*model = MM_DIRECT;
	}
	else if (strcmp(movementModel, "INDIRECT") == 0)
	{
		*model = MM_INDIRECT;
	}
	else if (strcmp(movementModel, "HOMING-DIRECT") == 0)
	{
		*model = MM_HOMINGDIRECT;
	}
	else if (strcmp(movementModel, "HOMING-INDIRECT") == 0)
	{
		*model = MM_HOMINGINDIRECT;
	}
	else
	{
		// We've got problem if we got here
		ASSERT(!"Invalid movement model", "Invalid movement model: %s", movementModel);
		return false;
	}

	return true;
}

const StringToEnum<WEAPON_EFFECT> mapUnsorted_WEAPON_EFFECT[] =
{
	{"ANTI PERSONNEL",      WE_ANTI_PERSONNEL       },
	{"ANTI TANK",           WE_ANTI_TANK            },
	{"BUNKER BUSTER",       WE_BUNKER_BUSTER        },
	{"ARTILLERY ROUND",     WE_ARTILLERY_ROUND      },
	{"FLAMER",              WE_FLAMER               },
	{"ANTI AIRCRAFT",       WE_ANTI_AIRCRAFT        },
	{"ALL ROUNDER",         WE_ANTI_AIRCRAFT        },  // Alternative name for WE_ANTI_AIRCRAFT.
};
const StringToEnumMap<WEAPON_EFFECT> map_WEAPON_EFFECT = mapUnsorted_WEAPON_EFFECT;

bool getWeaponEffect(const char *weaponEffect, WEAPON_EFFECT *effect)
{
	if (strcmp(weaponEffect, "ANTI PERSONNEL") == 0)
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

bool getWeaponClass(QString weaponClassStr, WEAPON_CLASS *weaponClass)
{
	if (weaponClassStr.compare("KINETIC") == 0)
	{
		*weaponClass = WC_KINETIC;
	}
	else if (weaponClassStr.compare("HEAT") == 0)
	{
		*weaponClass = WC_HEAT;
	}
	else
	{
		ASSERT(false, "Bad weapon class %s", weaponClassStr.toUtf8().constData());
		return false;
	};
	return true;
}

/*Access functions for the upgradeable stats of a weapon*/
int weaponFirePause(const WEAPON_STATS *psStats, int player)
{
	return psStats->upgrade[player].firePause;
}

/* Reload time is reduced for weapons with salvo fire */
int weaponReloadTime(const WEAPON_STATS *psStats, int player)
{
	return psStats->upgrade[player].reloadTime;
}

int weaponLongHit(const WEAPON_STATS *psStats, int player)
{
	return psStats->upgrade[player].hitChance;
}

int weaponDamage(const WEAPON_STATS *psStats, int player)
{
	return psStats->upgrade[player].damage;
}

int weaponRadDamage(const WEAPON_STATS *psStats, int player)
{
	return psStats->upgrade[player].radiusDamage;
}

int weaponPeriodicalDamage(const WEAPON_STATS *psStats, int player)
{
	return psStats->upgrade[player].periodicalDamage;
}

int sensorRange(const SENSOR_STATS *psStats, int player)
{
	return psStats->upgrade[player].range;
}

int ecmRange(const ECM_STATS *psStats, int player)
{
	return psStats->upgrade[player].range;
}

int repairPoints(const REPAIR_STATS *psStats, int player)
{
	return psStats->upgrade[player].repairPoints;
}

int constructorPoints(const CONSTRUCT_STATS *psStats, int player)
{
	return psStats->upgrade[player].constructPoints;
}

int bodyPower(const BODY_STATS *psStats, int player)
{
	return psStats->upgrade[player].power;
}

int bodyArmour(const BODY_STATS *psStats, int player, WEAPON_CLASS weaponClass)
{
	switch (weaponClass)
	{
	case WC_KINETIC:
		return psStats->upgrade[player].armour;
	case WC_HEAT:
		return psStats->upgrade[player].thermal;
	case WC_NUM_WEAPON_CLASSES:
		break;
	}
	ASSERT(false, "Unknown weapon class");
	return 0;	// Should never get here.
}

//calculates the weapons ROF based on the fire pause and the salvos
int weaponROF(const WEAPON_STATS *psStat, int player)
{
	int rof = 0;
	// if there are salvos
	if (player >= 0
	    && psStat->upgrade[player].numRounds
	    && psStat->upgrade[player].reloadTime != 0)
    {
        // Rounds per salvo multiplied with the number of salvos per minute
        rof = psStat->upgrade[player].numRounds * 60 * GAME_TICKS_PER_SEC / weaponReloadTime(psStat, player);
    }

	if (rof == 0)
	{
		rof = weaponFirePause(psStat, selectedPlayer);
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

void setMaxECMRange(UDWORD range)
{
	if (range > maxECMRange)
	{
		maxECMRange = range;
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

void updateMaxSensorStats(UWORD maxRange)
{
	UDWORD currentMaxValue = getMaxSensorRange();

	if (currentMaxValue < (currentMaxValue + currentMaxValue * maxRange / 100))
	{
		currentMaxValue += currentMaxValue * maxRange / 100;
		setMaxSensorRange(currentMaxValue);
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
	int currentMaxValue = getMaxECMRange();

	if (currentMaxValue < (currentMaxValue + currentMaxValue * maxValue / 100))
	{
		currentMaxValue += currentMaxValue * maxValue / 100;
		setMaxECMRange(currentMaxValue);
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

void adjustMaxDesignStats(void)
{
	UWORD       weaponDamage, sensorRange, repairPoints,
	            ecmRange, constPoints, bodyPoints, bodyPower, bodyArmour;

	// init all the values
	weaponDamage = sensorRange = repairPoints = ecmRange = constPoints = bodyPoints = bodyPower = bodyArmour = 0;

	//go thru' all the functions getting the max upgrade values for the stats

	for (int j = 0; j < numBodyStats; j++)
	{
		BODY_STATS *psStats = asBodyStats + j;
		bodyPoints = MAX(bodyPoints, psStats->upgrade[selectedPlayer].body);
		bodyArmour = MAX(bodyArmour, psStats->upgrade[selectedPlayer].armour);
		bodyPower = MAX(bodyPower, psStats->upgrade[selectedPlayer].power);
	}
	for (int j = 0; j < numSensorStats; j++)
	{
		SENSOR_STATS *psStats = asSensorStats + j;
		sensorRange = MAX(sensorRange, psStats->upgrade[selectedPlayer].range);
	}
	for (int j = 0; j < numECMStats; j++)
	{
		ECM_STATS *psStats = asECMStats + j;
		ecmRange = MAX(ecmRange, psStats->upgrade[selectedPlayer].range);
	}
	for (int j = 0; j < numRepairStats; j++)
	{
		REPAIR_STATS *psStats = asRepairStats + j;
		repairPoints = MAX(repairPoints, psStats->upgrade[selectedPlayer].repairPoints);
	}
	for (int j = 0; j < numConstructStats; j++)
	{
		CONSTRUCT_STATS *psStats = asConstructStats + j;
		constPoints = MAX(constPoints, psStats->upgrade[selectedPlayer].constructPoints);
	}
	for (int j = 0; j < numWeaponStats; j++)
	{
		WEAPON_STATS *psStats = asWeaponStats + j;
		weaponDamage = MAX(weaponDamage, psStats->upgrade[selectedPlayer].damage);
	}

	//determine the effect on the max values for the stats
	updateMaxWeaponStats(weaponDamage);
	updateMaxSensorStats(sensorRange);
	updateMaxRepairStats(repairPoints);
	updateMaxECMStats(ecmRange);
	updateMaxBodyStats(bodyPoints, bodyPower, bodyArmour);
	updateMaxConstStats(constPoints);
}

/* Check if an object has a weapon */
bool objHasWeapon(const BASE_OBJECT *psObj)
{
	//check if valid type
	if (psObj->type == OBJ_DROID)
	{
		if (((DROID *)psObj)->numWeaps > 0)
		{
			return true;
		}
	}
	else if (psObj->type == OBJ_STRUCTURE)
	{
		if (((STRUCTURE *)psObj)->numWeaps > 0)
		{
			return true;
		}
	}

	return false;
}

SENSOR_STATS *objActiveRadar(const BASE_OBJECT *psObj)
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
		compIndex = ((DROID *)psObj)->asBits[COMP_SENSOR];
		ASSERT_OR_RETURN(NULL, compIndex < numSensorStats, "Invalid range referenced for numSensorStats, %d > %d", compIndex, numSensorStats);
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

bool objRadarDetector(const BASE_OBJECT *psObj)
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
