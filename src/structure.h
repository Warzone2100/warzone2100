/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
 *  Definitions for the structures.
 */

#ifndef __INCLUDED_SRC_STRUCTURE_H__
#define __INCLUDED_SRC_STRUCTURE_H__

#include "lib/framework/string_ext.h"

#include "objectdef.h"
#include "structuredef.h"
#include "visibility.h"
#include "baseobject.h"

// how long to wait between CALL_STRUCT_ATTACKED's - plus how long to flash on radar for
#define ATTACK_CB_PAUSE		5000

/// Extra z padding for assembly points
#define ASSEMBLY_POINT_Z_PADDING 10

#define	STRUCTURE_DAMAGE_SCALING	400

//production loop max
#define INFINITE_PRODUCTION	 9//10

/*This should correspond to the structLimits! */
#define	MAX_FACTORY			5

//used to flag when the Factory is ready to start building
#define ACTION_START_TIME	0

extern std::vector<ProductionRun> asProductionRun[NUM_FACTORY_TYPES];

//Value is stored for easy access to this structure stat
extern UDWORD	factoryModuleStat;
extern UDWORD	powerModuleStat;
extern UDWORD	researchModuleStat;

// the structure that was last hit
extern STRUCTURE	*psLastStructHit;

//stores which player the production list has been set up for
extern SBYTE         productionPlayer;

//holder for all StructureStats
extern STRUCTURE_STATS		*asStructureStats;
extern UDWORD				numStructureStats;

//used to hold the modifiers cross refd by weapon effect and structureStrength
extern STRUCTSTRENGTH_MODIFIER		asStructStrengthModifier[WE_NUMEFFECTS][NUM_STRUCT_STRENGTH];

void handleAbandonedStructures();

int getMaxDroids(int player);
int getMaxCommanders(int player);
int getMaxConstructors(int player);
void setMaxDroids(int player, int value);
void setMaxCommanders(int player, int value);
void setMaxConstructors(int player, int value);

bool IsPlayerDroidLimitReached(int player);
bool CheckHaltOnMaxUnitsReached(STRUCTURE *psStructure);

bool loadStructureStats(const QString& filename);
/*Load the Structure Strength Modifiers from the file exported from Access*/
bool loadStructureStrengthModifiers(const char *pFileName);

bool structureStatsShutDown();

int requestOpenGate(STRUCTURE *psStructure);
int gateCurrentOpenHeight(const STRUCTURE *psStructure, uint32_t time, int minimumStub);  ///< Returns how far open the gate is, or 0 if the structure is not a gate.

int32_t structureDamage(STRUCTURE *psStructure, unsigned damage, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass, unsigned impactTime, bool isDamagePerSecond, int minDamage);
void structureBuild(STRUCTURE *psStructure, DROID *psDroid, int buildPoints, int buildRate = 1);
void structureDemolish(STRUCTURE *psStructure, DROID *psDroid, int buildPoints);
void structureRepair(STRUCTURE *psStruct, DROID *psDroid, int buildRate);
/* Set the type of droid for a factory to build */
bool structSetManufacture(STRUCTURE *psStruct, DROID_TEMPLATE *psTempl, QUEUE_MODE mode);

//temp test function for creating structures at the start of the game
void createTestStructures();

//builds a specified structure at a given location
STRUCTURE *buildStructure(STRUCTURE_STATS *pStructureType, UDWORD x, UDWORD y, UDWORD player, bool FromSave);
STRUCTURE *buildStructureDir(STRUCTURE_STATS *pStructureType, UDWORD x, UDWORD y, uint16_t direction, UDWORD player, bool FromSave);
/// Create a blueprint structure, with just enough information to render it
STRUCTURE *buildBlueprint(STRUCTURE_STATS const *psStats, Vector2i xy, uint16_t direction, unsigned moduleIndex, STRUCT_STATES state);
/* The main update routine for all Structures */
void structureUpdate(STRUCTURE *psBuilding, bool mission);

/* Remove a structure and free it's memory */
bool destroyStruct(STRUCTURE *psDel, unsigned impactTime);

// remove a structure from a game without any visible effects
// bDestroy = true if the object is to be destroyed
// (for example used to change the type of wall at a location)
bool removeStruct(STRUCTURE *psDel, bool bDestroy);

//fills the list with Structures that can be built
UDWORD fillStructureList(STRUCTURE_STATS **ppList, UDWORD selectedPlayer, UDWORD limit);

/// Checks if the two structures would be too close to build together.
bool isBlueprintTooClose(STRUCTURE_STATS const *stats1, Vector2i pos1, uint16_t dir1, STRUCTURE_STATS const *stats2, Vector2i pos2, uint16_t dir2);

/// Checks that the location is valid to build on.
/// pos in world coords
bool validLocation(BASE_STATS *psStats, Vector2i pos, uint16_t direction, unsigned player, bool bCheckBuildQueue);

bool isWall(STRUCTURE_TYPE type);                                    ///< Structure is a wall. Not completely sure it handles all cases.
bool isBuildableOnWalls(STRUCTURE_TYPE type);                        ///< Structure can be built on walls. Not completely sure it handles all cases.

/* for a new structure, find a location along an edge which the droid can get
to and return this as the destination for the droid */

/* for a structure or feature, find a location along an edge which the droid can get
to and return this as the destination for the droid*/
bool getDroidDestination(BASE_STATS *psPositionStats, UDWORD structX, UDWORD structY, UDWORD *pDroidX, UDWORD *pDroidY);

/* check along the width of a structure for an empty space */
bool checkWidth(UDWORD maxRange, UDWORD x, UDWORD y, UDWORD *pDroidX, UDWORD *pDroidY);

/* check along the length of a structure for an empty space */
bool checkLength(UDWORD maxRange, UDWORD x, UDWORD y, UDWORD *pDroidX, UDWORD *pDroidY);

void alignStructure(STRUCTURE *psBuilding);

/* set the current number of structures of each type built */
void setCurrentStructQuantity(bool displayError);
/* get a stat inc based on the name */
int32_t getStructStatFromName(char const *pName);
/*check to see if the structure is 'doing' anything  - return true if idle*/
bool  structureIdle(STRUCTURE *psBuilding);
/*checks to see if any structure exists of a specified type with a specified status */
bool checkStructureStatus(STRUCTURE_STATS *psStats, UDWORD player, UDWORD status);
/*sets the point new droids go to - x/y in world coords for a Factory*/
void setAssemblyPoint(FLAG_POSITION *psAssemblyPoint, UDWORD x, UDWORD y, UDWORD player, bool bCheck);

/*initialises the flag before a new data set is loaded up*/
void initFactoryNumFlag();

//called at start of missions
void resetFactoryNumFlag();

/* get demolish stat */
STRUCTURE_STATS *structGetDemolishStat();

/*find a location near to the factory to start the droid of*/
bool placeDroid(STRUCTURE *psStructure, UDWORD *droidX, UDWORD *droidY);

/* is this a lassat structure? */
static inline bool isLasSat(STRUCTURE_STATS *pStructureType)
{
	ASSERT_OR_RETURN(false, pStructureType != nullptr, "LasSat is invalid?");

	return (pStructureType->psWeapStat[0]
	        && pStructureType->psWeapStat[0]->weaponSubClass == WSC_LAS_SAT);
}

/*sets the flag to indicate a SatUplink Exists - so draw everything!*/
void setSatUplinkExists(bool state, UDWORD player);

/*returns the status of the flag*/
bool getSatUplinkExists(UDWORD player);

/*sets the flag to indicate a Las Sat Exists - ONLY EVER WANT ONE*/
void setLasSatExists(bool state, UDWORD player);

/*returns the status of the flag*/
bool getLasSatExists(UDWORD player);

/* added int weapon_slot to fix the alway slot 0 hack */
bool calcStructureMuzzleLocation(STRUCTURE *psStructure, Vector3i *muzzle, int weapon_slot);
bool calcStructureMuzzleBaseLocation(STRUCTURE *psStructure, Vector3i *muzzle, int weapon_slot);

/*this is called whenever a structure has finished building*/
void buildingComplete(STRUCTURE *psBuilding);

// these functions are used in game.c inplace of  building complete
void checkForResExtractors(STRUCTURE *psPowerGen);
void checkForPowerGen(STRUCTURE *psPowerGen);

// Set the command droid that factory production should go to struct _command_droid;
void assignFactoryCommandDroid(STRUCTURE *psStruct, struct DROID *psCommander);

// remove all factories from a command droid
void clearCommandDroidFactory(DROID *psDroid);

/*for a given structure, return a pointer to its module stat */
STRUCTURE_STATS *getModuleStat(const STRUCTURE *psStruct);

/*called when a Res extractor is destroyed or runs out of power or is disconnected
adjusts the owning Power Gen so that it can link to a different Res Extractor if one
is available*/
void releaseResExtractor(STRUCTURE *psRelease);

/*called when a Power Gen is destroyed or is disconnected
adjusts the associated Res Extractors so that they can link to different Power
Gens if any are available*/
void releasePowerGen(STRUCTURE *psRelease);

//print some info at the top of the screen dependent on the structure
void printStructureInfo(STRUCTURE *psStructure);

/*Checks the template type against the factory type - returns false
if not a good combination!*/
bool validTemplateForFactory(const DROID_TEMPLATE *psTemplate, STRUCTURE *psFactory, bool complain);

/*calculates the damage caused to the resistance levels of structures*/
bool electronicDamage(BASE_OBJECT *psTarget, UDWORD damage, UBYTE attackPlayer);

/* EW works differently in multiplayer mode compared with single player.*/
bool validStructResistance(const STRUCTURE *psStruct);

/*checks to see if a specific structure type exists -as opposed to a structure
stat type*/
bool checkSpecificStructExists(UDWORD structInc, UDWORD player);

int32_t getStructureDamage(const STRUCTURE *psStructure);

unsigned structureBodyBuilt(const STRUCTURE *psStruct);  ///< Returns the maximum body points of a structure with the current number of build points.
UDWORD structureBody(const STRUCTURE *psStruct);
UDWORD structureArmour(const STRUCTURE_STATS *psStats, UBYTE player);
UDWORD structureResistance(const STRUCTURE_STATS *psStats, UBYTE player);

void hqReward(UBYTE losingPlayer, UBYTE rewardPlayer);

// Is a structure a factory of somekind?
bool StructIsFactory(const STRUCTURE *Struct);

// Is a flag a factory delivery point?
bool FlagIsFactory(const FLAG_POSITION *psCurrFlag);

// Find a factories corresonding delivery point.
FLAG_POSITION *FindFactoryDelivery(const STRUCTURE *Struct);

//Find the factory associated with the delivery point - returns NULL if none exist
STRUCTURE *findDeliveryFactory(FLAG_POSITION *psDelPoint);

/*this is called when a factory produces a droid. The Template returned is the next
one to build - if any*/
DROID_TEMPLATE *factoryProdUpdate(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate);

//increment the production run for this type
void factoryProdAdjust(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate, bool add);

//returns the quantity of a specific template in the production list
ProductionRunEntry getProduction(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate);

//looks through a players production list to see if a command droid is being built
UBYTE checkProductionForCommand(UBYTE player);

//check that delivery points haven't been put down in invalid location
void checkDeliveryPoints(UDWORD version);

//adjust the loop quantity for this factory
void factoryLoopAdjust(STRUCTURE *psStruct, bool add);

/*cancels the production run for the factory and returns any power that was
accrued but not used*/
void cancelProduction(STRUCTURE *psBuilding, QUEUE_MODE mode, bool mayClearProductionRun = true);

/*set a factory's production run to hold*/
void holdProduction(STRUCTURE *psBuilding, QUEUE_MODE mode);

/*release a factory's production run from hold*/
void releaseProduction(STRUCTURE *psBuilding, QUEUE_MODE mode);

/// Does the next item in the production list.
void doNextProduction(STRUCTURE *psStructure, DROID_TEMPLATE *current, QUEUE_MODE mode);

/*This function is called after a game is loaded so that any resource extractors
that are active are initialised for when to start*/
void checkResExtractorsActive();

// Count number of factories assignable to a command droid.
UWORD countAssignableFactories(UBYTE player, UWORD FactoryType);

/*Used for determining how much of the structure to draw as being built or demolished*/
float structHeightScale(const STRUCTURE *psStruct);

/*compares the structure sensor type with the droid weapon type to see if the
FIRE_SUPPORT order can be assigned*/
bool structSensorDroidWeapon(const STRUCTURE *psStruct, const DROID *psDroid);

/*checks if the structure has a Counter Battery sensor attached - returns
true if it has*/
bool structCBSensor(const STRUCTURE *psStruct);
/*checks if the structure has a Standard Turret sensor attached - returns
true if it has*/
bool structStandardSensor(const STRUCTURE *psStruct);

/*checks if the structure has a VTOL Intercept sensor attached - returns
true if it has*/
bool structVTOLSensor(const STRUCTURE *psStruct);

/*checks if the structure has a VTOL Counter Battery sensor attached - returns
true if it has*/
bool structVTOLCBSensor(const STRUCTURE *psStruct);

// return the nearest rearm pad
// psTarget can be NULL
STRUCTURE *findNearestReArmPad(DROID *psDroid, STRUCTURE *psTarget, bool bClear);

// check whether a rearm pad is clear
bool clearRearmPad(const STRUCTURE *psStruct);

// clear a rearm pad for a vtol to land on it
void ensureRearmPadClear(STRUCTURE *psStruct, DROID *psDroid);

// return whether a rearm pad has a vtol on it
bool vtolOnRearmPad(STRUCTURE *psStruct, DROID *psDroid);

/* Just returns true if the structure's present body points aren't as high as the original*/
bool	structIsDamaged(STRUCTURE *psStruct);

// give a structure from one player to another - used in Electronic Warfare
STRUCTURE *giftSingleStructure(STRUCTURE *psStructure, UBYTE attackPlayer, bool electronic_warfare = true);

/*Initialise the production list and set up the production player*/
void changeProductionPlayer(UBYTE player);

// La!
bool IsStatExpansionModule(const STRUCTURE_STATS *psStats);

/// is this a blueprint and not a real structure?
bool structureIsBlueprint(const STRUCTURE *psStructure);
bool isBlueprint(const BASE_OBJECT *psObject);

/*returns the power cost to build this structure*/
UDWORD structPowerToBuild(const STRUCTURE *psStruct);

// check whether a factory of a certain number and type exists
bool checkFactoryExists(UDWORD player, UDWORD factoryType, UDWORD inc);

/*checks the structure passed in is a Las Sat structure which is currently
selected - returns true if valid*/
bool lasSatStructSelected(STRUCTURE *psStruct);

void cbNewDroid(STRUCTURE *psFactory, DROID *psDroid);

WZ_DECL_PURE Vector2i getStructureSize(STRUCTURE const *psBuilding);
WZ_DECL_PURE Vector2i getStructureStatsSize(STRUCTURE_STATS const *pStructureType, uint16_t direction);
StructureBounds getStructureBounds(const STRUCTURE *object);
StructureBounds getStructureBounds(const STRUCTURE_STATS *stats, Vector2i pos, uint16_t direction);

static inline unsigned getStructureWidth(const STRUCTURE *psBuilding)
{
	return getStructureSize(psBuilding).x;
}
static inline unsigned getStructureBreadth(const STRUCTURE *psBuilding)
{
	return getStructureSize(psBuilding).y;
}
static inline WZ_DECL_PURE unsigned getStructureStatsWidth(const STRUCTURE_STATS *pStructureType, uint16_t direction)
{
	return getStructureStatsSize(pStructureType, direction).x;
}
static inline WZ_DECL_PURE unsigned getStructureStatsBreadth(const STRUCTURE_STATS *pStructureType, uint16_t direction)
{
	return getStructureStatsSize(pStructureType, direction).y;
}

static inline int structSensorRange(const STRUCTURE *psObj)
{
	return objSensorRange((const BASE_OBJECT *)psObj);
}

static inline int structJammerPower(const STRUCTURE *psObj)
{
	return objJammerPower((const BASE_OBJECT *)psObj);
}

static inline Rotation structureGetInterpolatedWeaponRotation(STRUCTURE *psStructure, int weaponSlot, uint32_t time)
{
	return interpolateRot(psStructure->asWeaps[weaponSlot].prevRot, psStructure->asWeaps[weaponSlot].rot, psStructure->prevTime, psStructure->time, time);
}

#define setStructureTarget(_psBuilding, _psNewTarget, _idx, _targetOrigin) _setStructureTarget(_psBuilding, _psNewTarget, _idx, _targetOrigin, __LINE__, __FUNCTION__)
static inline void _setStructureTarget(STRUCTURE *psBuilding, BASE_OBJECT *psNewTarget, UWORD idx, TARGET_ORIGIN targetOrigin, int line, const char *func)
{
	ASSERT_OR_RETURN(, idx < MAX_WEAPONS, "Bad index");
	ASSERT_OR_RETURN(, psNewTarget == nullptr || !psNewTarget->died, "setStructureTarget set dead target");
	psBuilding->psTarget[idx] = psNewTarget;
	psBuilding->asWeaps[idx].origin = targetOrigin;
#ifdef DEBUG
	psBuilding->targetLine[idx] = line;
	sstrcpy(psBuilding->targetFunc[idx], func);
#else
	// Prevent warnings about unused parameters
	(void)line;
	(void)func;
#endif
}

// Functions for the GUI to know what's pending, before it's synchronised.
template<typename Functionality, typename Subject>
static inline void setStatusPendingStart(Functionality &functionality, Subject *subject)
{
	functionality.psSubjectPending = subject;
	functionality.statusPending = FACTORY_START_PENDING;
	++functionality.pendingCount;
}

template<typename Functionality>
static inline void setStatusPendingCancel(Functionality &functionality)
{
	functionality.psSubjectPending = nullptr;
	functionality.statusPending = FACTORY_CANCEL_PENDING;
	++functionality.pendingCount;
}

template<typename Functionality>
static inline void setStatusPendingHold(Functionality &functionality)
{
	if (functionality.psSubjectPending == nullptr)
	{
		functionality.psSubjectPending = functionality.psSubject;
	}
	functionality.statusPending = FACTORY_HOLD_PENDING;
	++functionality.pendingCount;
}

template<typename Functionality>
static inline void setStatusPendingRelease(Functionality &functionality)
{
	if (functionality.psSubjectPending == nullptr && functionality.statusPending != FACTORY_CANCEL_PENDING)
	{
		functionality.psSubjectPending = functionality.psSubject;
	}
	if (functionality.psSubjectPending != nullptr)
	{
		functionality.statusPending = FACTORY_START_PENDING;
	}
	++functionality.pendingCount;
}

template<typename Functionality>
static inline void popStatusPending(Functionality &functionality)
{
	if (functionality.pendingCount == 0)
	{
		++functionality.pendingCount;
	}
	if (--functionality.pendingCount == 0)
	{
		// Subject is now synchronised, remove pending.
		functionality.psSubjectPending = nullptr;
		functionality.statusPending = FACTORY_NOTHING_PENDING;
	}
}

void checkStructure(const STRUCTURE *psStructure, const char *const location_description, const char *function, const int recurse);

#define CHECK_STRUCTURE(object) checkStructure((object), AT_MACRO, __FUNCTION__, max_check_object_recursion)

void structureInitVars();
void initStructLimits();

#define syncDebugStructure(psStruct, ch) _syncDebugStructure(__FUNCTION__, psStruct, ch)
void _syncDebugStructure(const char *function, STRUCTURE const *psStruct, char ch);


// True iff object is a structure.
static inline bool isStructure(SIMPLE_OBJECT const *psObject)
{
	return psObject != nullptr && psObject->type == OBJ_STRUCTURE;
}
// Returns STRUCTURE * if structure or NULL if not.
static inline STRUCTURE *castStructure(SIMPLE_OBJECT *psObject)
{
	return isStructure(psObject) ? (STRUCTURE *)psObject : (STRUCTURE *)nullptr;
}
// Returns STRUCTURE const * if structure or NULL if not.
static inline STRUCTURE const *castStructure(SIMPLE_OBJECT const *psObject)
{
	return isStructure(psObject) ? (STRUCTURE const *)psObject : (STRUCTURE const *)nullptr;
}

static inline int getBuildingResearchPoints(STRUCTURE *psStruct)
{
	auto &upgrade = psStruct->pStructureType->upgrade[psStruct->player];
	return upgrade.research + upgrade.moduleResearch * psStruct->capacity;
}

static inline int getBuildingProductionPoints(STRUCTURE *psStruct)
{
	auto &upgrade = psStruct->pStructureType->upgrade[psStruct->player];
	return upgrade.production + upgrade.moduleProduction * psStruct->capacity;
}

static inline int getBuildingPowerPoints(STRUCTURE *psStruct)
{
	auto &upgrade = psStruct->pStructureType->upgrade[psStruct->player];
	return upgrade.power + upgrade.modulePower * psStruct->capacity;
}

static inline int getBuildingRepairPoints(STRUCTURE *psStruct)
{
	return psStruct->pStructureType->upgrade[psStruct->player].repair;
}

static inline int getBuildingRearmPoints(STRUCTURE *psStruct)
{
	return psStruct->pStructureType->upgrade[psStruct->player].rearm;
}

#endif // __INCLUDED_SRC_STRUCTURE_H__
