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
 *  Definitions for the droid object.
 */

#ifndef __INCLUDED_SRC_DROID_H__
#define __INCLUDED_SRC_DROID_H__

#include "lib/framework/string_ext.h"
#include "lib/gamelib/gtime.h"

#include "objectdef.h"
#include "stats.h"
#include "visibility.h"

#define OFF_SCREEN 9999		// world->screen check - alex

#define REPAIRLEV_LOW	50	// percentage of body points remaining at which to repair droid automatically.
#define REPAIRLEV_HIGH	75	// ditto, but this will repair much sooner..

#define DROID_RESISTANCE_FACTOR     30

// Changing this breaks campaign saves!
#define MAX_RECYCLED_DROIDS 450

//used to stop structures being built too near the edge and droids being placed down
#define TOO_NEAR_EDGE	3

/* Experience modifies */
#define EXP_REDUCE_DAMAGE		6		// damage of a droid is reduced by this value per experience level, in %
#define EXP_ACCURACY_BONUS		5		// accuracy of a droid is increased by this value per experience level, in %
#define EXP_SPEED_BONUS			5		// speed of a droid is increased by this value per experience level, in %

enum PICKTILE
{
	NO_FREE_TILE,
	FREE_TILE,
};

// the structure that was last hit
extern DROID	*psLastDroidHit;

extern UWORD	aDroidExperience[MAX_PLAYERS][MAX_RECYCLED_DROIDS];


// initialise droid module
extern bool droidInit(void);

bool removeDroidBase(DROID *psDel);

extern bool loadDroidWeapons(const char *pWeaponData, UDWORD bufferSize);

struct INITIAL_DROID_ORDERS
{
	uint32_t secondaryOrder;
	int32_t moveToX;
	int32_t moveToY;
	uint32_t factoryId;
};
/*Builds an instance of a Structure - the x/y passed in are in world coords.*/
/// Sends a GAME_DROID message if bMultiMessages is true, or actually creates it if false. Only uses initialOrders if sending a GAME_DROID message.
extern DROID* buildDroid(DROID_TEMPLATE *pTemplate, UDWORD x, UDWORD y, UDWORD player, bool onMission, const INITIAL_DROID_ORDERS *initialOrders);
/// Creates a droid locally, instead of sending a message, even if the bMultiMessages HACK is set to true.
DROID *reallyBuildDroid(DROID_TEMPLATE *pTemplate, Position pos, UDWORD player, bool onMission, Rotation rot = Rotation());

/* Set the asBits in a DROID structure given it's template. */
extern void droidSetBits(DROID_TEMPLATE *pTemplate,DROID *psDroid);

/* Calculate the weight of a droid from it's template */
extern UDWORD calcDroidWeight(DROID_TEMPLATE *psTemplate);

/* Calculate the power points required to build/maintain a droid */
extern UDWORD calcDroidPower(DROID *psDroid);

// Calculate the number of points required to build a droid
extern UDWORD calcDroidPoints(DROID *psDroid);

/* Calculate the body points of a droid from it's template */
extern UDWORD calcTemplateBody(DROID_TEMPLATE *psTemplate, UBYTE player);

/* Calculate the base body points of a droid without upgrades*/
extern UDWORD calcDroidBaseBody(DROID *psDroid);

/* Calculate the base speed of a droid from it's template */
extern UDWORD calcDroidBaseSpeed(DROID_TEMPLATE *psTemplate, UDWORD weight, UBYTE player);

/* Calculate the speed of a droid over a terrain */
extern UDWORD calcDroidSpeed(UDWORD baseSpeed, UDWORD terrainType, UDWORD propIndex, UDWORD level);

/* Calculate the points required to build the template */
extern UDWORD calcTemplateBuild(DROID_TEMPLATE *psTemplate);

/* Calculate the power points required to build/maintain the droid */
extern UDWORD	calcTemplatePower(DROID_TEMPLATE *psTemplate);

// return whether a droid is IDF
bool idfDroid(DROID *psDroid);

/* Do damage to a droid */
int32_t droidDamage(DROID *psDroid, unsigned damage, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass, unsigned impactTime, bool isDamagePerSecond);

/* The main update routine for all droids */
extern void droidUpdate(DROID *psDroid);

/* Set up a droid to build a structure - returns true if successful */
enum DroidStartBuild {DroidStartBuildFailed, DroidStartBuildSuccess, DroidStartBuildPending};
DroidStartBuild droidStartBuild(DROID *psDroid);

/* Sets a droid to start demolishing - returns true if successful */
extern bool	droidStartDemolishing( DROID *psDroid );

/* Update a construction droid while it is demolishing
   returns true while demolishing */
extern bool	droidUpdateDemolishing( DROID *psDroid );

/* Sets a droid to start repairing - returns true if successful */
extern bool	droidStartRepair( DROID *psDroid );

/* Update a construction droid while it is repairing
   returns true while repairing */
extern bool	droidUpdateRepair( DROID *psDroid );

/*Start a Repair Droid working on a damaged droid - returns true if successful*/
extern bool droidStartDroidRepair( DROID *psDroid );

/*Updates a Repair Droid working on a damaged droid - returns true whilst repairing*/
extern bool droidUpdateDroidRepair(DROID *psRepairDroid);

/* Update a construction droid while it is building
   returns true while building continues */
extern bool droidUpdateBuild(DROID *psDroid);

/*Start a EW weapon droid working on a low resistance structure*/
extern bool droidStartRestore( DROID *psDroid );

/*continue restoring a structure*/
extern bool droidUpdateRestore( DROID *psDroid );

// recycle a droid (retain it's experience and some of it's cost)
extern void recycleDroid(DROID *psDel);

/* Remove a droid and free it's memory */
bool destroyDroid(DROID *psDel, unsigned impactTime);

/* Same as destroy droid except no graphical effects */
extern void	vanishDroid(DROID *psDel);

/* Burn a barbarian then destroy it */
extern void droidBurn( DROID * psDroid );

/* Remove a droid from the apsDroidLists so doesn't update or get drawn etc*/
//returns true if successfully removed from the list
extern bool droidRemove(DROID *psDroid, DROID *pList[MAX_PLAYERS]);

//free the storage for the droid templates
extern bool droidTemplateShutDown(void);

/* Return the type of a droid */
extern DROID_TYPE droidType(DROID *psDroid);

/* Return the type of a droid from it's template */
extern DROID_TYPE droidTemplateType(DROID_TEMPLATE *psTemplate);

extern void assignDroidsToGroup(UDWORD	playerNumber, UDWORD groupNumber);

extern bool activateGroup(UDWORD playerNumber, UDWORD groupNumber);

extern UDWORD	getNumDroidsForLevel(UDWORD	level);

extern bool activateGroupAndMove(UDWORD playerNumber, UDWORD groupNumber);
/* calculate muzzle tip location in 3d world added int weapon_slot to fix the always slot 0 hack*/
bool calcDroidMuzzleLocation(DROID *psDroid, Vector3i *muzzle, int weapon_slot);
/* calculate muzzle base location in 3d world added int weapon_slot to fix the always slot 0 hack*/
bool calcDroidMuzzleBaseLocation(DROID *psDroid, Vector3i *muzzle, int weapon_slot);

// finds a droid for the player and sets it to be the current selected droid
extern bool selectDroidByID(UDWORD id, UDWORD player);

/* Droid experience stuff */
extern unsigned int getDroidLevel(const DROID* psDroid);
extern UDWORD	getDroidEffectiveLevel(DROID *psDroid);
extern const char *getDroidLevelName(DROID *psDroid);

// Get a droid's name.
extern const char *droidGetName(const DROID *psDroid);

// Set a droid's name.
extern void droidSetName(DROID *psDroid, const char *pName);

// returns true when no droid on x,y square.
extern bool	noDroid					(UDWORD x, UDWORD y);				// true if no droid at x,y
// returns an x/y coord to place a droid
extern PICKTILE pickHalfATile		(UDWORD *x, UDWORD *y, UBYTE numIterations);
extern	bool	zonedPAT(UDWORD x, UDWORD y);
extern	bool	pickATileGen(UDWORD *x, UDWORD *y, UBYTE numIterations,
					 bool (*function)(UDWORD x, UDWORD y));
bool pickATileGen(Vector2i *pos, unsigned numIterations, bool (*function)(UDWORD x, UDWORD y));
extern	bool	pickATileGenThreat(UDWORD *x, UDWORD *y, UBYTE numIterations, SDWORD threatRange,
					 SDWORD player, bool (*function)(UDWORD x, UDWORD y));


//initialises the droid movement model
extern void initDroidMovement(DROID *psDroid);

/* Looks through the players list of droids to see if any of them are
building the specified structure - returns true if finds one*/
extern bool checkDroidsBuilding(STRUCTURE *psStructure);

/* Looks through the players list of droids to see if any of them are
demolishing the specified structure - returns true if finds one*/
extern bool checkDroidsDemolishing(STRUCTURE *psStructure);

/// Returns the next module which can be built after lastOrderedModule, or returns 0 if not possible.
int nextModuleToBuild(STRUCTURE const *psStruct, int lastOrderedModule);

/*Deals with building a module - checking if any droid is currently doing this
 - if so, helping to build the current one*/
extern void setUpBuildModule(DROID *psDroid);

/* Just returns true if the droid's present body points aren't as high as the original*/
extern bool	droidIsDamaged(DROID *psDroid);

/* Returns currently active (selected) group */
extern UDWORD	getSelectedGroup( void );
extern void	setSelectedGroup(UDWORD groupNumber);
extern UDWORD	getSelectedCommander( void );
extern void	setSelectedCommander(UDWORD commander);

extern char const *getDroidResourceName(char const *pName);

/*checks to see if an electronic warfare weapon is attached to the droid*/
extern bool electronicDroid(DROID *psDroid);

/*checks to see if the droid is currently being repaired by another*/
extern bool droidUnderRepair(DROID *psDroid);

//count how many Command Droids exist in the world at any one moment
extern UBYTE checkCommandExist(UBYTE player);

/*For a given repair droid, check if there are any damaged droids within
a defined range*/
extern BASE_OBJECT * checkForRepairRange(DROID *psDroid,DROID *psTarget);

/// Returns true iff the droid has VTOL propulsion, and is not a transport.
extern bool isVtolDroid(const DROID* psDroid);
/// Returns true iff the droid has VTOL propulsion and is moving.
extern bool isFlying(const DROID* psDroid);
/*returns true if a VTOL weapon droid which has completed all runs*/
extern bool vtolEmpty(DROID *psDroid);
/*returns true if a VTOL weapon droid which still has full ammo*/
extern bool vtolFull(DROID *psDroid);
/*Checks a vtol for being fully armed and fully repaired to see if ready to
leave reArm pad */
extern bool  vtolHappy(const DROID* psDroid);
/*checks if the droid is a VTOL droid and updates the attack runs as required*/
extern void updateVtolAttackRun(DROID *psDroid, int weapon_slot);
/*returns a count of the base number of attack runs for the weapon attached to the droid*/
extern UWORD   getNumAttackRuns(DROID *psDroid, int weapon_slot);
//assign rearmPad to the VTOL
extern void assignVTOLPad(DROID *psNewDroid, STRUCTURE *psReArmPad);
// true if a vtol is waiting to be rearmed by a particular rearm pad
extern bool vtolReadyToRearm(DROID *psDroid, STRUCTURE *psStruct);
// true if a vtol droid currently returning to be rearmed
extern bool vtolRearming(DROID *psDroid);
// true if a droid is currently attacking
extern bool droidAttacking(DROID *psDroid);
// see if there are any other vtols attacking the same target
// but still rearming
extern bool allVtolsRearmed(DROID *psDroid);

/*compares the droid sensor type with the droid weapon type to see if the
FIRE_SUPPORT order can be assigned*/
extern bool droidSensorDroidWeapon(BASE_OBJECT *psObj, DROID *psDroid);

// return whether a droid has a CB sensor on it
extern bool cbSensorDroid(DROID *psDroid);
// return whether a droid has a standard sensor on it (standard, VTOL strike, or wide spectrum)
extern bool standardSensorDroid(DROID *psDroid);

// give a droid from one player to another - used in Electronic Warfare and multiplayer
extern DROID * giftSingleDroid(DROID *psD, UDWORD to);
/*calculates the electronic resistance of a droid based on its experience level*/
extern SWORD   droidResistance(DROID *psDroid);

/*this is called to check the weapon is 'allowed'. Check if VTOL, the weapon is
direct fire. Also check numVTOLattackRuns for the weapon is not zero - return
true if valid weapon*/
extern bool checkValidWeaponForProp(DROID_TEMPLATE *psTemplate);

extern const char *getDroidNameForRank(UDWORD rank);

/*called when a Template is deleted in the Design screen*/
void deleteTemplateFromProduction(DROID_TEMPLATE *psTemplate, unsigned player, QUEUE_MODE mode);  // ModeQueue deletes from production queues, which are not yet synchronised. ModeImmediate deletes from current production which is synchronised.

// Select a droid and do any necessary housekeeping.
extern void SelectDroid(DROID *psDroid);

// De-select a droid and do any necessary housekeeping.
extern void DeSelectDroid(DROID *psDroid);

/* audio finished callback */
extern bool droidAudioTrackStopped( void *psObj );

/*returns true if droid type is one of the Cyborg types*/
extern bool cyborgDroid(const DROID* psDroid);

bool isConstructionDroid(DROID const *psDroid);
bool isConstructionDroid(BASE_OBJECT const *psObject);

// check for illegal references to droid we want to release
bool droidCheckReferences(DROID *psVictimDroid);

/** Check if droid is in a legal world position and is not on its way to drive off the map. */
bool droidOnMap(const DROID *psDroid);

void droidSetPosition(DROID *psDroid, int x, int y);

/// Return a percentage of how fully armed the object is, or -1 if N/A.
int droidReloadBar(BASE_OBJECT *psObj, WEAPON *psWeap, int weapon_slot);

static inline int droidSensorRange(const DROID* psDroid)
{
	return objSensorRange((const BASE_OBJECT*)psDroid);
}

static inline int droidJammerPower(const DROID* psDroid)
{
	return objJammerPower((const BASE_OBJECT*)psDroid);
}

static inline int droidConcealment(const DROID* psDroid)
{
	return objConcealment((const BASE_OBJECT*)psDroid);
}

/*
 * Component stat helper functions
 */
static inline BODY_STATS *getBodyStats(DROID *psDroid)
{
	return asBodyStats + psDroid->asBits[COMP_BODY].nStat;
}

static inline BRAIN_STATS *getBrainStats(DROID *psDroid)
{
	return asBrainStats + psDroid->asBits[COMP_BRAIN].nStat;
}

static inline PROPULSION_STATS *getPropulsionStats(DROID *psDroid)
{
	return asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
}

static inline SENSOR_STATS *getSensorStats(DROID *psDroid)
{
	return asSensorStats + psDroid->asBits[COMP_SENSOR].nStat;
}

static inline ECM_STATS *getECMStats(DROID *psDroid)
{
	return asECMStats + psDroid->asBits[COMP_ECM].nStat;
}

static inline REPAIR_STATS *getRepairStats(DROID *psDroid)
{
	return asRepairStats + psDroid->asBits[COMP_REPAIRUNIT].nStat;
}

static inline CONSTRUCT_STATS *getConstructStats(DROID *psDroid)
{
	return asConstructStats + psDroid->asBits[COMP_CONSTRUCT].nStat;
}

static inline WEAPON_STATS *getWeaponStats(DROID *psDroid, int weapon_slot)
{
	return asWeaponStats + psDroid->asWeaps[weapon_slot].nStat;
}

static inline Rotation getInterpolatedWeaponRotation(DROID *psDroid, int weaponSlot, uint32_t time)
{
	return interpolateRot(psDroid->asWeaps[weaponSlot].prevRot, psDroid->asWeaps[weaponSlot].rot, psDroid->prevSpacetime.time, psDroid->time, time);
}

/** helper functions for future refcount patch **/

#define setDroidTarget(_psDroid, _psNewTarget) _setDroidTarget(_psDroid, _psNewTarget, __LINE__, __FUNCTION__)
static inline void _setDroidTarget(DROID *psDroid, BASE_OBJECT *psNewTarget, int line, const char *func)
{
	psDroid->order.psObj = psNewTarget;
	ASSERT(psNewTarget == NULL || !psNewTarget->died, "setDroidTarget: Set dead target");
	ASSERT(psNewTarget == NULL || !psNewTarget->died || (psNewTarget->died == NOT_CURRENT_LIST && psDroid->died == NOT_CURRENT_LIST),
	       "setDroidTarget: Set dead target");
#ifdef DEBUG
	psDroid->targetLine = line;
	sstrcpy(psDroid->targetFunc, func);
#else
	// Prevent warnings about unused parameters
	(void)line;
	(void)func;
#endif
}

#define setDroidActionTarget(_psDroid, _psNewTarget, _idx) _setDroidActionTarget(_psDroid, _psNewTarget, _idx, __LINE__, __FUNCTION__)
static inline void _setDroidActionTarget(DROID *psDroid, BASE_OBJECT *psNewTarget, UWORD idx, int line, const char *func)
{
	psDroid->psActionTarget[idx] = psNewTarget;
	ASSERT(psNewTarget == NULL || !psNewTarget->died || (psNewTarget->died == NOT_CURRENT_LIST && psDroid->died == NOT_CURRENT_LIST),
	       "setDroidActionTarget: Set dead target");
#ifdef DEBUG
	psDroid->actionTargetLine[idx] = line;
	sstrcpy(psDroid->actionTargetFunc[idx], func);
#else
	// Prevent warnings about unused parameters
	(void)line;
	(void)func;
#endif
}

#define setDroidBase(_psDroid, _psNewTarget) _setDroidBase(_psDroid, _psNewTarget, __LINE__, __FUNCTION__)
static inline void _setDroidBase(DROID *psDroid, STRUCTURE *psNewBase, int line, const char *func)
{
	psDroid->psBaseStruct = psNewBase;
	ASSERT(psNewBase == NULL || !psNewBase->died, "setDroidBase: Set dead target");
#ifdef DEBUG
	psDroid->baseLine = line;
	sstrcpy(psDroid->baseFunc, func);
#else
	// Prevent warnings about unused parameters
	(void)line;
	(void)func;
#endif
}

static inline void setSaveDroidTarget(DROID *psSaveDroid, BASE_OBJECT *psNewTarget)
{
	psSaveDroid->order.psObj = psNewTarget;
#ifdef DEBUG
	psSaveDroid->targetLine = 0;
	sstrcpy(psSaveDroid->targetFunc, "savegame");
#endif
}

static inline void setSaveDroidActionTarget(DROID *psSaveDroid, BASE_OBJECT *psNewTarget, UWORD idx)
{
	psSaveDroid->psActionTarget[idx] = psNewTarget;
#ifdef DEBUG
	psSaveDroid->actionTargetLine[idx] = 0;
	sstrcpy(psSaveDroid->actionTargetFunc[idx], "savegame");
#endif
}

static inline void setSaveDroidBase(DROID *psSaveDroid, STRUCTURE *psNewBase)
{
	psSaveDroid->psBaseStruct = psNewBase;
#ifdef DEBUG
	psSaveDroid->baseLine = 0;
	sstrcpy(psSaveDroid->baseFunc, "savegame");
#endif
}

void checkDroid(const DROID *droid, const char * const location_description, const char * function, const int recurse);

/** assert if droid is bad */
#define CHECK_DROID(droid) checkDroid(droid, AT_MACRO, __FUNCTION__, max_check_object_recursion)

/** If droid can get to given object using its current propulsion, return the square distance. Otherwise return -1. */
int droidSqDist(DROID *psDroid, BASE_OBJECT *psObj);

// Minimum damage a weapon will deal to its target
#define	MIN_WEAPON_DAMAGE	1

void templateSetParts(const DROID *psDroid, DROID_TEMPLATE *psTemplate);

void cancelBuild(DROID *psDroid);

#define syncDebugDroid(psDroid, ch) _syncDebugDroid(__FUNCTION__, psDroid, ch)
void _syncDebugDroid(const char *function, DROID const *psDroid, char ch);


// True iff object is a droid.
static inline bool isDroid(SIMPLE_OBJECT const *psObject)           { return psObject != NULL && psObject->type == OBJ_DROID; }
// Returns DROID * if droid or NULL if not.
static inline DROID *castDroid(SIMPLE_OBJECT *psObject)             { return isDroid(psObject)? (DROID *)psObject : (DROID *)NULL; }
// Returns DROID const * if droid or NULL if not.
static inline DROID const *castDroid(SIMPLE_OBJECT const *psObject) { return isDroid(psObject)? (DROID const *)psObject : (DROID const *)NULL; }


#endif // __INCLUDED_SRC_DROID_H__
