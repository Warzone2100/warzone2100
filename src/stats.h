/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
 *  Interface to the common stats module
 */
#ifndef __INCLUDED_SRC_STATS_H__
#define __INCLUDED_SRC_STATS_H__

#include <utility>

#include "objectdef.h"

/**************************************************************************************
 *
 * Function prototypes and data storage for the stats
 */

/* The stores for the different stats */
extern BODY_STATS			*asBodyStats;
extern BRAIN_STATS			*asBrainStats;
extern PROPULSION_STATS		*asPropulsionStats;
extern SENSOR_STATS			*asSensorStats;
extern ECM_STATS			*asECMStats;
//extern ARMOUR_STATS			*asArmourStats;
extern REPAIR_STATS			*asRepairStats;
extern WEAPON_STATS			*asWeaponStats;
extern CONSTRUCT_STATS		*asConstructStats;
extern PROPULSION_TYPES		*asPropulsionTypes;

//used to hold the modifiers cross refd by weapon effect and propulsion type
extern WEAPON_MODIFIER		asWeaponModifier[WE_NUMEFFECTS][PROPULSION_TYPE_NUM];
extern WEAPON_MODIFIER		asWeaponModifierBody[WE_NUMEFFECTS][SIZE_NUM];

//used to hold the current upgrade level per player per weapon subclass
extern WEAPON_UPGRADE		asWeaponUpgrade[MAX_PLAYERS][WSC_NUM_WEAPON_SUBCLASSES];
extern SENSOR_UPGRADE		asSensorUpgrade[MAX_PLAYERS];
extern ECM_UPGRADE			asECMUpgrade[MAX_PLAYERS];
extern REPAIR_UPGRADE		asRepairUpgrade[MAX_PLAYERS];
extern CONSTRUCTOR_UPGRADE	asConstUpgrade[MAX_PLAYERS];
//body upgrades are possible for droids and/or cyborgs
#define		DROID_BODY_UPGRADE	0
#define		CYBORG_BODY_UPGRADE	1
#define		BODY_TYPE		2
extern BODY_UPGRADE			asBodyUpgrade[MAX_PLAYERS][BODY_TYPE];

/* The number of different stats stored */
extern UDWORD		numBodyStats;
extern UDWORD		numBrainStats;
extern UDWORD		numPropulsionStats;
extern UDWORD		numSensorStats;
extern UDWORD		numECMStats;
extern UDWORD		numRepairStats;
extern UDWORD		numWeaponStats;
extern UDWORD		numConstructStats;
extern UDWORD		numTerrainTypes;

/* What number the ref numbers start at for each type of stat */
#define REF_BODY_START			0x010000
#define REF_BRAIN_START			0x020000
//#define REF_POWER_START			0x030000
#define REF_PROPULSION_START	0x040000
#define REF_SENSOR_START		0x050000
#define REF_ECM_START			0x060000
//#define REF_ARMOUR_START		0x070000
#define REF_REPAIR_START		0x080000
#define REF_WEAPON_START		0x0a0000
#define REF_RESEARCH_START		0x0b0000
#define REF_TEMPLATE_START		0x0c0000
#define REF_STRUCTURE_START		0x0d0000
#define REF_FUNCTION_START		0x0e0000
#define REF_CONSTRUCT_START		0x0f0000
#define REF_FEATURE_START		0x100000

/* The maximum number of refs for a type of stat */
#define REF_RANGE				0x010000


//stores for each players component states - see below
extern UBYTE		*apCompLists[MAX_PLAYERS][COMP_NUMCOMPONENTS];

//store for each players Structure states
extern UBYTE		*apStructTypeLists[MAX_PLAYERS];

//flags to fill apCompLists and apStructTypeLists
#define AVAILABLE				0x01		//this item can be used to design droids
#define UNAVAILABLE				0x02		//the player does not know about this item
#define FOUND					0x04		//this item has been found, but is unresearched
#define REDUNDANT				0x0A		//the player no longer needs this item

/*******************************************************************************
*		Allocate stats functions
*******************************************************************************/
/* Allocate Weapon stats */
extern bool statsAllocWeapons(UDWORD numEntries);

/*Allocate Armour stats*/
//extern bool statsAllocArmour(UDWORD numEntries);

/*Allocate Body stats*/
extern bool statsAllocBody(UDWORD numEntries);

/*Allocate Brain stats*/
extern bool statsAllocBrain(UDWORD numEntries);

/*Allocate Power stats*/
//extern bool statsAllocPower(UDWORD numEntries);

/*Allocate Propulsion stats*/
extern bool statsAllocPropulsion(UDWORD numEntries);

/*Allocate Sensor stats*/
extern bool statsAllocSensor(UDWORD numEntries);

/*Allocate Ecm Stats*/
extern bool statsAllocECM(UDWORD numEntries);

/*Allocate Repair Stats*/
extern bool statsAllocRepair(UDWORD numEntries);

/*Allocate Construct Stats*/
extern bool statsAllocConstruct(UDWORD numEntries);

extern UWORD weaponROF(WEAPON_STATS *psStat, SBYTE player);

/*******************************************************************************
*		Load stats functions
*******************************************************************************/
/* Return the number of newlines in a file buffer */
extern UDWORD numCR(const char *pFileBuffer, UDWORD fileSize);

/*Load the weapon stats from the file exported from Access*/
extern bool loadWeaponStats(const char *pFileName);

/*Load the armour stats from the file exported from Access*/
//extern bool loadArmourStats(void);

/*Load the body stats from the file exported from Access*/
extern bool loadBodyStats(const char *pFileName);

/*Load the brain stats from the file exported from Access*/
extern bool loadBrainStats(const char *pFileName);

/*Load the power stats from the file exported from Access*/
//extern bool loadPowerStats(void);

/*Load the propulsion stats from the file exported from Access*/
extern bool loadPropulsionStats(const char *pFileName);

/*Load the sensor stats from the file exported from Access*/
extern bool loadSensorStats(const char *pFileName);

/*Load the ecm stats from the file exported from Access*/
extern bool loadECMStats(const char *fileName);

/*Load the repair stats from the file exported from Access*/
extern bool loadRepairStats(const char *pFileName);

/*Load the construct stats from the file exported from Access*/
extern bool loadConstructStats(const char *pFileName);

/*Load the Propulsion Types from the file exported from Access*/
extern bool loadPropulsionTypes(const char *pFileName);

/*Load the propulsion sounds from the file exported from Access*/
extern bool loadPropulsionSounds(const char *pFileName);

/*Load the Terrain Table from the file exported from Access*/
extern bool loadTerrainTable(const char *pFileName);

/* load the IMDs to use for each body-propulsion combination */
extern bool loadBodyPropulsionIMDs(const char *pFileName);

/*Load the weapon sounds from the file exported from Access*/
extern bool loadWeaponSounds(const char *pFileName);

/*Load the Weapon Effect Modifiers from the file exported from Access*/
extern bool loadWeaponModifiers(const char *pFileName);

/*******************************************************************************
*		Generic stats functions
*******************************************************************************/

/*calls the STATS_DEALLOC macro for each set of stats*/
extern bool statsShutDown(void);

extern UDWORD getSpeedFactor(UDWORD terrainType, UDWORD propulsionType);
//return the type of stat this ref refers to!
extern UDWORD statType(UDWORD ref);
//return the REF_START value of this type of stat
extern UDWORD statRefStart(UDWORD stat);
/*Returns the component type based on the string - used for reading in data */
extern UDWORD componentType(const char* pType);
//get the component Inc for a stat based on the name
extern SDWORD getCompFromName(UDWORD compType, const char *pName);
//get the component Inc for a stat based on the Resource name held in Names.txt
extern SDWORD getCompFromResName(UDWORD compType, const char *pName);
/*returns the weapon sub class based on the string name passed in */
extern bool getWeaponSubClass(const char* subClass, WEAPON_SUBCLASS* wclass);
/*either gets the name associated with the resource (if one) or allocates space and copies pName*/
extern char* allocateName(const char* name);
/*return the name to display for the interface - valid for OBJECTS and STATS*/
extern const char* getName(const char *pNameID);
/*sets the store to the body size based on the name passed in - returns false
if doesn't compare with any*/
extern bool getBodySize(const char *pSize, BODY_SIZE *pStore);

// Pass in a stat and get its name
extern const char* getStatName(const void * pStat);

/**
 * Determines the propulsion type indicated by the @c typeName string passed
 * in.
 *
 * @param typeName  name of the propulsion type to determine the enumerated
 *                  constant for.
 * @param[out] type Will contain an enumerated constant representing the given
 *                  propulsion type, if successful (as indicated by the return
 *                  value).
 *
 * @return true if successful, false otherwise. If successful, @c *type will
 *         contain a valid propulsion type enumerator, otherwise its value will
 *         be left unchanged.
 */
extern bool getPropulsionType(const char* typeName, PROPULSION_TYPE* type);

/**
 * Determines the weapon effect indicated by the @c weaponEffect string passed
 * in.
 *
 * @param weaponEffect name of the weapon effect to determine the enumerated
 *                     constant for.
 * @param[out] effect  Will contain an enumerated constant representing the
 *                     given weapon effect, if successful (as indicated by the
 *                     return value).
 *
 * @return true if successful, false otherwise. If successful, @c *effect will
 *         contain a valid weapon effect enumerator, otherwise its value will
 *         be left unchanged.
 */
extern const StringToEnumMap<WEAPON_EFFECT> map_WEAPON_EFFECT;

extern UWORD weaponROF(WEAPON_STATS *psStat, SBYTE player);
/*Access functions for the upgradeable stats of a weapon*/
extern UDWORD	weaponFirePause(const WEAPON_STATS* psStats, UBYTE player);
extern UDWORD	weaponReloadTime(WEAPON_STATS *psStats, UBYTE player);
extern UDWORD	weaponShortHit(const WEAPON_STATS* psStats, UBYTE player);
extern UDWORD	weaponLongHit(const WEAPON_STATS* psStats, UBYTE player);
extern UDWORD	weaponDamage(const WEAPON_STATS* psStats, UBYTE player);
extern UDWORD	weaponRadDamage(WEAPON_STATS *psStats, UBYTE player);
extern UDWORD	weaponPeriodicalDamage(WEAPON_STATS *psStats, UBYTE player);
extern UDWORD	weaponRadiusHit(WEAPON_STATS *psStats, UBYTE player);
/*Access functions for the upgradeable stats of a sensor*/
extern UDWORD	sensorRange(SENSOR_STATS *psStats, UBYTE player);
/*Access functions for the upgradeable stats of a ECM*/
extern UDWORD	ecmRange(ECM_STATS *psStats, UBYTE player);
/*Access functions for the upgradeable stats of a repair*/
extern UDWORD	repairPoints(REPAIR_STATS *psStats, UBYTE player);
/*Access functions for the upgradeable stats of a constructor*/
extern UDWORD	constructorPoints(CONSTRUCT_STATS *psStats, UBYTE player);
/*Access functions for the upgradeable stats of a body*/
extern UDWORD	bodyPower(BODY_STATS *psStats, UBYTE player, UBYTE bodyType);
extern UDWORD	bodyArmour(BODY_STATS *psStats, UBYTE player, UBYTE bodyType,
				   WEAPON_CLASS weaponClass);

/*dummy function for John*/
extern void brainAvailable(BRAIN_STATS *psStat);

extern void adjustMaxDesignStats(void);

//Access functions for the max values to be used in the Design Screen
extern UDWORD getMaxComponentWeight(void);
extern UDWORD getMaxBodyArmour(void);
extern UDWORD getMaxBodyPower(void);
extern UDWORD getMaxBodyPoints(void);
extern UDWORD getMaxSensorRange(void);
extern UDWORD getMaxECMRange(void);
extern UDWORD getMaxConstPoints(void);
extern UDWORD getMaxRepairPoints(void);
extern UDWORD getMaxWeaponRange(void);
extern UDWORD getMaxWeaponDamage(void);
extern UDWORD getMaxWeaponROF(void);
extern UDWORD getMaxPropulsionSpeed(void);

extern bool objHasWeapon(const BASE_OBJECT *psObj);

extern void statsInitVars(void);

bool getWeaponEffect(const char* weaponEffect, WEAPON_EFFECT* effect);
bool getWeaponClass(QString weaponClassStr, WEAPON_CLASS *weaponClass);

/* Wrappers */

/** If object is an active radar (has sensor turret), then return a pointer to its sensor stats. If not, return NULL. */
SENSOR_STATS *objActiveRadar(const BASE_OBJECT *psObj);

/** Returns whether object has a radar detector sensor. */
bool objRadarDetector(const BASE_OBJECT *psObj);

#endif // __INCLUDED_SRC_STATS_H__
