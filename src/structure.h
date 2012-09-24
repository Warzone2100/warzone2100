/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

/* explosion data for when a structure is blown up - used by features as well*/
#define	FLAME_MAX_ANIMS		4
#define FLAME_MAX_OFFSET	50
#define FLAME_MIN_DELAY		2000
#define FLAME_MAX_DELAY		8000
#define FLAME_CYCLES_MAX	10

#define NON_STOP_PRODUCTION	(STAT_SLDSTOPS + 1)
//production loop max
#define INFINITE_PRODUCTION	 9//10

/*This should correspond to the structLimits! */
#define	MAX_FACTORY			5



//used to flag when the Factory is ready to start building
#define ACTION_START_TIME	0

//distance that VTOLs can be away from the reArm pad
#define REARM_DIST			(TILE_UNITS*2)

extern iIMDShape * factoryModuleIMDs[NUM_FACTORY_MODULES][NUM_FACMOD_TYPES];
extern iIMDShape * researchModuleIMDs[NUM_RESEARCH_MODULES];
extern iIMDShape * powerModuleIMDs[NUM_POWER_MODULES];

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
extern STRUCTURE_LIMITS		*asStructLimits[MAX_PLAYERS];
//holds the upgrades attained through research for structure stats
extern STRUCTURE_UPGRADE	asStructureUpgrade[MAX_PLAYERS];
extern WALLDEFENCE_UPGRADE	asWallDefenceUpgrade[MAX_PLAYERS];
//holds the upgrades for the functionality of structures through research
extern RESEARCH_UPGRADE	asResearchUpgrade[MAX_PLAYERS];
extern POWER_UPGRADE		asPowerUpgrade[MAX_PLAYERS];
extern REPAIR_FACILITY_UPGRADE	asRepairFacUpgrade[MAX_PLAYERS];
extern PRODUCTION_UPGRADE	asProductionUpgrade[MAX_PLAYERS][NUM_FACTORY_TYPES];
extern REARM_UPGRADE		asReArmUpgrade[MAX_PLAYERS];

//used to hold the modifiers cross refd by weapon effect and structureStrength
extern STRUCTSTRENGTH_MODIFIER		asStructStrengthModifier[WE_NUMEFFECTS][
													NUM_STRUCT_STRENGTH];

extern void handleAbandonedStructures(void);

extern bool IsPlayerDroidLimitReached(UDWORD PlayerNumber);
extern bool IsPlayerStructureLimitReached(UDWORD PlayerNumber);
extern bool CheckHaltOnMaxUnitsReached(STRUCTURE *psStructure);

extern bool loadStructureStats(const char *pStructData, UDWORD bufferSize);
extern bool loadStructureWeapons(const char *pWeaponData, UDWORD bufferSize);
extern bool loadStructureFunctions(const char *pFunctionData, UDWORD bufferSize);
/*Load the Structure Strength Modifiers from the file exported from Access*/
extern bool loadStructureStrengthModifiers(const char *pStrengthModData, UDWORD bufferSize);

extern bool	structureStatsShutDown(void);

int requestOpenGate(STRUCTURE *psStructure);
int gateCurrentOpenHeight(STRUCTURE const *psStructure, uint32_t time, int minimumStub);  ///< Returns how far open the gate is, or 0 if the structure is not a gate.

int32_t structureDamage(STRUCTURE *psStructure, unsigned damage, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass, unsigned impactTime, bool isDamagePerSecond);
extern void structureBuild(STRUCTURE *psStructure, DROID *psDroid, int buildPoints, int buildRate = 1);
extern void structureDemolish(STRUCTURE *psStructure, DROID *psDroid, int buildPoints);
void structureRepair(STRUCTURE *psStruct, DROID *psDroid, int buildRate);
/* Set the type of droid for a factory to build */
extern bool structSetManufacture(STRUCTURE *psStruct, DROID_TEMPLATE *psTempl, QUEUE_MODE mode);

//temp test function for creating structures at the start of the game
extern void createTestStructures(void);

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
extern UDWORD fillStructureList(STRUCTURE_STATS **ppList, UDWORD selectedPlayer,
						 UDWORD limit);

/// Checks if the two structures would be too close to build together.
bool isBlueprintTooClose(STRUCTURE_STATS const *stats1, Vector2i pos1, uint16_t dir1, STRUCTURE_STATS const *stats2, Vector2i pos2, uint16_t dir2);

/// Checks that the location is valid to build on.
/// pos in world coords
bool validLocation(BASE_STATS *psStats, Vector2i pos, uint16_t direction, unsigned player, bool bCheckBuildQueue);

bool isWall(STRUCTURE_TYPE type);                                    ///< Structure is a wall. Not completely sure it handles all cases.
bool isBuildableOnWalls(STRUCTURE_TYPE type);                        ///< Structure can be built on walls. Not completely sure it handles all cases.

/* for a new structure, find a location along an edge which the droid can get
to and return this as the destination for the droid */
//extern bool getDroidDestination(STRUCTURE_STATS *psPositionStats, UDWORD structX,
//	UDWORD structY, UDWORD * pDroidX, UDWORD *pDroidY);
/*for a structure or feature, find a location along an edge which the droid can get
to and return this as the destination for the droid*/
extern bool getDroidDestination(BASE_STATS *psPositionStats, UDWORD structX,
	UDWORD structY, UDWORD * pDroidX, UDWORD *pDroidY);
/* check along the width of a structure for an empty space */
extern bool checkWidth(UDWORD maxRange, UDWORD x, UDWORD y, UDWORD *pDroidX, UDWORD *pDroidY);

/* check along the length of a structure for an empty space */
extern bool checkLength(UDWORD maxRange, UDWORD x, UDWORD y, UDWORD *pDroidX, UDWORD *pDroidY);

extern void alignStructure(STRUCTURE *psBuilding);

//initialise the structure limits structure
extern void initStructLimits(void);
/* set the current number of structures of each type built */
extern void setCurrentStructQuantity(bool displayError);
/* get a stat inc based on the name */
extern int32_t getStructStatFromName(char const *pName);
/*check to see if the structure is 'doing' anything  - return true if idle*/
extern bool  structureIdle(STRUCTURE *psBuilding);
/*checks to see if any structure exists of a specified type with a specified status */
extern bool checkStructureStatus( STRUCTURE_STATS *psStats, UDWORD player, UDWORD status);
/*sets the point new droids go to - x/y in world coords for a Factory*/
extern void setAssemblyPoint(FLAG_POSITION *psAssemblyPoint, UDWORD x, UDWORD y,
                             UDWORD player, bool bCheck);

/*called when a structure has been built - checks through the list of callbacks
for the scripts*/
extern void structureCompletedCallback(STRUCTURE_STATS *psStructType);

/*initialises the flag before a new data set is loaded up*/
extern void initFactoryNumFlag(void);
//called at start of missions
extern void resetFactoryNumFlag(void);

/* get demolish stat */
extern STRUCTURE_STATS * structGetDemolishStat( void );

/*find a location near to the factory to start the droid of*/
extern bool placeDroid(STRUCTURE *psStructure, UDWORD *droidX, UDWORD *droidY);

/*sets the flag to indicate a Power Generator Exists - so do Oil Derrick anim*/
//extern void setPowerGenExists(bool state, UDWORD player);
/*returns teh status of the flag*/
//extern bool getPowerGenExists(UDWORD player);

/* is this a lassat structure? */
static inline bool isLasSat(STRUCTURE_STATS *pStructureType)
{
	ASSERT_OR_RETURN(false, pStructureType != NULL, "LasSat is invalid?");

	return (pStructureType->psWeapStat[0]
	        && pStructureType->psWeapStat[0]->weaponSubClass == WSC_LAS_SAT);
}

/*sets the flag to indicate a SatUplink Exists - so draw everything!*/
extern void setSatUplinkExists(bool state, UDWORD player);
/*returns the status of the flag*/
extern bool getSatUplinkExists(UDWORD player);
/*sets the flag to indicate a Las Sat Exists - ONLY EVER WANT ONE*/
extern void setLasSatExists(bool state, UDWORD player);
/*returns the status of the flag*/
extern bool getLasSatExists(UDWORD player);

/* added int weapon_slot to fix the alway slot 0 hack */
bool calcStructureMuzzleLocation(STRUCTURE *psStructure, Vector3i *muzzle, int weapon_slot);
bool calcStructureMuzzleBaseLocation(STRUCTURE *psStructure, Vector3i *muzzle, int weapon_slot);

/*this is called whenever a structure has finished building*/
extern void buildingComplete(STRUCTURE *psBuilding);
// these functions are used in game.c inplace of  building complete
extern void checkForResExtractors(STRUCTURE *psPowerGen);
extern void checkForPowerGen(STRUCTURE *psPowerGen);

// Set the command droid that factory production should go to
//struct _command_droid;
extern void assignFactoryCommandDroid(STRUCTURE *psStruct, struct DROID *psCommander);

// remove all factories from a command droid
void clearCommandDroidFactory(DROID *psDroid);

/*for a given structure, return a pointer to its module stat */
extern STRUCTURE_STATS* getModuleStat(const STRUCTURE* psStruct);

/*called when a Res extractor is destroyed or runs out of power or is disconnected
adjusts the owning Power Gen so that it can link to a different Res Extractor if one
is available*/
extern void releaseResExtractor(STRUCTURE *psRelease);

/*called when a Power Gen is destroyed or is disconnected
adjusts the associated Res Extractors so that they can link to different Power
Gens if any are available*/
extern void releasePowerGen(STRUCTURE *psRelease);

//print some info at the top of the screen dependant on the structure
extern void printStructureInfo(STRUCTURE *psStructure);

/*Checks the template type against the factory type - returns false
if not a good combination!*/
extern bool validTemplateForFactory(DROID_TEMPLATE *psTemplate, STRUCTURE *psFactory);

/*calculates the damage caused to the resistance levels of structures*/
//extern bool electronicDamage(STRUCTURE *psStructure, UDWORD damage, UBYTE attackPlayer);
//electronic damage can be targetted at droids as well as structures now - AB 5/11/98
extern bool electronicDamage(BASE_OBJECT *psTarget, UDWORD damage, UBYTE attackPlayer);

/* EW works differently in multiplayer mode compared with single player.*/
extern bool validStructResistance(STRUCTURE *psStruct);

/*checks to see if a specific structure type exists -as opposed to a structure
stat type*/
extern bool checkSpecificStructExists(UDWORD structInc, UDWORD player);

extern int32_t getStructureDamage(const STRUCTURE* psStructure);

/*Access functions for the upgradeable stats of a structure*/
unsigned structureBodyBuilt(STRUCTURE const *psStruct);  ///< Returns the maximum body points of a structure with the current number of build points.
extern UDWORD	structureBody(const STRUCTURE *psStruct);
extern UDWORD	structureArmour(STRUCTURE_STATS *psStats, UBYTE player);
extern UDWORD	structureResistance(STRUCTURE_STATS *psStats, UBYTE player);
/*this returns the Base Body points of a structure - regardless of upgrade*/
extern UDWORD	structureBaseBody(const STRUCTURE *psStructure);

extern void hqReward(UBYTE losingPlayer, UBYTE rewardPlayer);

// Is a structure a factory of somekind?
extern bool StructIsFactory(STRUCTURE *Struct);

// Is a flag a factory delivery point?
extern bool FlagIsFactory(FLAG_POSITION *psCurrFlag);

// Find a factories corresonding delivery point.
extern FLAG_POSITION *FindFactoryDelivery(STRUCTURE *Struct);

//Find the factory associated with the delivery point - returns NULL if none exist
extern STRUCTURE	*findDeliveryFactory(FLAG_POSITION *psDelPoint);

/*this is called when a factory produces a droid. The Template returned is the next
one to build - if any*/
extern DROID_TEMPLATE * factoryProdUpdate(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate);

//increment the production run for this type
extern void factoryProdAdjust(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate, bool add);

//returns the quantity of a specific template in the production list
ProductionRunEntry getProduction(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate);

//looks through a players production list to see if a command droid is being built
extern UBYTE checkProductionForCommand(UBYTE player);

//check that delivery points haven't been put down in invalid location
extern void checkDeliveryPoints(UDWORD version);

//adjust the loop quantity for this factory
extern void factoryLoopAdjust(STRUCTURE *psStruct, bool add);

/*cancels the production run for the factory and returns any power that was
accrued but not used*/
void cancelProduction(STRUCTURE *psBuilding, QUEUE_MODE mode, bool mayClearProductionRun = true);

/*set a factory's production run to hold*/
extern void holdProduction(STRUCTURE *psBuilding, QUEUE_MODE mode);

/*release a factory's production run from hold*/
extern void releaseProduction(STRUCTURE *psBuilding, QUEUE_MODE mode);

/// Does the next item in the production list.
void doNextProduction(STRUCTURE *psStructure, DROID_TEMPLATE *current, QUEUE_MODE mode);

/*This function is called after a game is loaded so that any resource extractors
that are active are initialised for when to start*/
extern void checkResExtractorsActive(void);

// Count number of factories assignable to a command droid.
extern UWORD countAssignableFactories(UBYTE player,UWORD FactoryType);

/*Used for determining how much of the structure to draw as being built or demolished*/
extern float structHeightScale(STRUCTURE *psStruct);

/*compares the structure sensor type with the droid weapon type to see if the
FIRE_SUPPORT order can be assigned*/
extern bool structSensorDroidWeapon(STRUCTURE *psStruct, DROID *psDroid);

/*checks if the structure has a Counter Battery sensor attached - returns
true if it has*/
extern bool structCBSensor(const STRUCTURE* psStruct);
/*checks if the structure has a Standard Turret sensor attached - returns
true if it has*/
extern bool structStandardSensor(const STRUCTURE* psStruct);

/*checks if the structure has a VTOL Intercept sensor attached - returns
true if it has*/
extern bool structVTOLSensor(const STRUCTURE* psStruct);

/*checks if the structure has a VTOL Counter Battery sensor attached - returns
true if it has*/
extern bool structVTOLCBSensor(const STRUCTURE* psStruct);

// return the nearest rearm pad
// if bClear is true it tries to find the nearest clear rearm pad in
// the same cluster as psTarget
// psTarget can be NULL
STRUCTURE *	findNearestReArmPad(DROID *psDroid, STRUCTURE *psTarget, bool bClear);

// check whether a rearm pad is clear
bool clearRearmPad(STRUCTURE *psStruct);

// clear a rearm pad for a vtol to land on it
void ensureRearmPadClear(STRUCTURE *psStruct, DROID *psDroid);

// return whether a rearm pad has a vtol on it
bool vtolOnRearmPad(STRUCTURE *psStruct, DROID *psDroid);

/* Just returns true if the structure's present body points aren't as high as the original*/
extern bool	structIsDamaged(STRUCTURE *psStruct);

// give a structure from one player to another - used in Electronic Warfare
extern STRUCTURE * giftSingleStructure(STRUCTURE *psStructure, UBYTE attackPlayer, bool bFromScript);

/*Initialise the production list and set up the production player*/
extern void changeProductionPlayer(UBYTE player);

// La!
bool IsStatExpansionModule(STRUCTURE_STATS const *psStats);

/// is this a blueprint and not a real structure?
bool structureIsBlueprint(STRUCTURE *psStructure);
bool isBlueprint(BASE_OBJECT *psObject);

/*checks that the structure stats have loaded up as expected - must be done after
all StructureStats parts have been loaded*/
extern bool checkStructureStats(void);

/*returns the power cost to build this structure*/
extern UDWORD structPowerToBuild(const STRUCTURE* psStruct);

extern UDWORD getMaxDroids(UDWORD PlayerNumber);

// check whether a factory of a certain number and type exists
extern bool checkFactoryExists(UDWORD player, UDWORD factoryType, UDWORD inc);

/*checks the structure passed in is a Las Sat structure which is currently
selected - returns true if valid*/
extern bool lasSatStructSelected(STRUCTURE *psStruct);

bool structureCheckReferences(STRUCTURE *psVictimStruct);

void cbNewDroid(STRUCTURE *psFactory, DROID *psDroid);

WZ_DECL_PURE Vector2i getStructureSize(STRUCTURE const *psBuilding);
WZ_DECL_PURE Vector2i getStructureStatsSize(STRUCTURE_STATS const *pStructureType, uint16_t direction);
StructureBounds getStructureBounds(STRUCTURE const *object);
StructureBounds getStructureBounds(STRUCTURE_STATS const *stats, Vector2i pos, uint16_t direction);

static inline unsigned getStructureWidth(const STRUCTURE *psBuilding)   { return getStructureSize(psBuilding).x; }
static inline unsigned getStructureBreadth(const STRUCTURE *psBuilding) { return getStructureSize(psBuilding).y; }
static inline WZ_DECL_PURE unsigned getStructureStatsWidth(const STRUCTURE_STATS *pStructureType, uint16_t direction)   { return getStructureStatsSize(pStructureType, direction).x; }
static inline WZ_DECL_PURE unsigned getStructureStatsBreadth(const STRUCTURE_STATS *pStructureType, uint16_t direction) { return getStructureStatsSize(pStructureType, direction).y; }

static inline int structSensorRange(const STRUCTURE* psObj)
{
	return objSensorRange((const BASE_OBJECT*)psObj);
}

static inline int structJammerPower(const STRUCTURE* psObj)
{
	return objJammerPower((const BASE_OBJECT*)psObj);
}

static inline int structConcealment(const STRUCTURE* psObj)
{
	return objConcealment((const BASE_OBJECT*)psObj);
}

static inline Rotation structureGetInterpolatedWeaponRotation(STRUCTURE *psStructure, int weaponSlot, uint32_t time)
{
	return interpolateRot(psStructure->asWeaps[weaponSlot].prevRot, psStructure->asWeaps[weaponSlot].rot, psStructure->prevTime, psStructure->time, time);
}

#define setStructureTarget(_psBuilding, _psNewTarget, _idx, _targetOrigin) _setStructureTarget(_psBuilding, _psNewTarget, _idx, _targetOrigin, __LINE__, __FUNCTION__)
static inline void _setStructureTarget(STRUCTURE *psBuilding, BASE_OBJECT *psNewTarget, UWORD idx, UWORD targetOrigin, int line, const char *func)
{
	assert(idx < STRUCT_MAXWEAPS);
	psBuilding->psTarget[idx] = psNewTarget;
	psBuilding->targetOrigin[idx] = targetOrigin;
	ASSERT(psNewTarget == NULL || !psNewTarget->died, "setStructureTarget set dead target");
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
	functionality.psSubjectPending = NULL;
	functionality.statusPending = FACTORY_CANCEL_PENDING;
	++functionality.pendingCount;
}

template<typename Functionality>
static inline void setStatusPendingHold(Functionality &functionality)
{
	if (functionality.psSubjectPending == NULL)
	{
		functionality.psSubjectPending = functionality.psSubject;
	}
	functionality.statusPending = FACTORY_HOLD_PENDING;
	++functionality.pendingCount;
}

template<typename Functionality>
static inline void setStatusPendingRelease(Functionality &functionality)
{
	if (functionality.psSubjectPending == NULL && functionality.statusPending != FACTORY_CANCEL_PENDING)
	{
		functionality.psSubjectPending = functionality.psSubject;
	}
	if (functionality.psSubjectPending != NULL)
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
		functionality.psSubjectPending = NULL;
		functionality.statusPending = FACTORY_NOTHING_PENDING;
	}
}

void checkStructure(const STRUCTURE* psStructure, const char * const location_description, const char * function, const int recurse);

#define CHECK_STRUCTURE(object) checkStructure((object), AT_MACRO, __FUNCTION__, max_check_object_recursion)

extern void     structureInitVars(void);

#define syncDebugStructure(psStruct, ch) _syncDebugStructure(__FUNCTION__, psStruct, ch)
void _syncDebugStructure(const char *function, STRUCTURE const *psStruct, char ch);


// True iff object is a structure.
static inline bool isStructure(SIMPLE_OBJECT const *psObject)               { return psObject != NULL && psObject->type == OBJ_STRUCTURE; }
// Returns STRUCTURE * if structure or NULL if not.
static inline STRUCTURE *castStructure(SIMPLE_OBJECT *psObject)             { return isStructure(psObject)? (STRUCTURE *)psObject : (STRUCTURE *)NULL; }
// Returns STRUCTURE const * if structure or NULL if not.
static inline STRUCTURE const *castStructure(SIMPLE_OBJECT const *psObject) { return isStructure(psObject)? (STRUCTURE const *)psObject : (STRUCTURE const *)NULL; }


#endif // __INCLUDED_SRC_STRUCTURE_H__
