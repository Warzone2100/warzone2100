/*
 * AI.h
 *
 * Definitions for the AI system structures
 *
 */
#ifndef _ai_h
#define _ai_h

#include "objectdef.h"

#define ALLIANCE_BROKEN		0			// states of alliance between players
#define ALLIANCE_REQUESTED	1
#define ALLIANCE_INVITATION	2
#define ALLIANCE_FORMED		3
#define	ALLIANCE_NULL		4			// for setting values only.

#define NO_ALLIANCES		0			//alliance possibilities for games.
#define ALLIANCES			1
#define	ALLIANCES_TEAMS		2			//locked teams
//#define GROUP_WINS		2

/* Weights used for target selection code */
#define	WEIGHT_DIST_TILE							11									//In points used in weaponmodifier.txt and structuremodifier.txt
#define	WEIGHT_DIST_TILE_DROID				WEIGHT_DIST_TILE		//How much weight a distance of 1 tile (128 world units) has when looking for the best nearest target
#define	WEIGHT_DIST_TILE_STRUCT			WEIGHT_DIST_TILE
#define	WEIGHT_HEALTH_DROID					WEIGHT_DIST_TILE		//How much weight unit damage has (per 10% of damage, ie for each 10% of damage we add WEIGHT_HEALTH_DROID)
																													//~100% damage should be ~8 tiles (max sensor range)
#define	WEIGHT_HEALTH_STRUCT				WEIGHT_DIST_TILE

#define	WEIGHT_NOT_VISIBLE_FACTOR		10														//We really don't like objects we can't see

#define	WEIGHT_SERVICE_DROIDS				(WEIGHT_DIST_TILE_DROID * 5)		//We don't want them to be repairing droids or structures while we are after them
#define	WEIGHT_WEAPON_DROIDS				(WEIGHT_DIST_TILE_DROID * 3)		//We prefere to go after anything that has a gun and can hurt us
#define	WEIGHT_MILITARY_STRUCT				WEIGHT_DIST_TILE_STRUCT			//Droid/cyborg factories, repair facility; shouldn't have too much weight
#define	WEIGHT_WEAPON_STRUCT				WEIGHT_WEAPON_DROIDS				//Same as weapon droids (?)

// alliances
extern UBYTE alliances[MAX_PLAYERS][MAX_PLAYERS];

/* Check no alliance has formed*/
extern BOOL aiCheckAlliances(UDWORD s1,UDWORD s2);

/* Initialise the AI system */
extern BOOL aiInitialise(void);

/* Shutdown the AI system */
extern BOOL aiShutdown(void);

/* Initialise a droid structure for AI */
extern BOOL aiInitDroid(DROID *psDroid);

/* Do the AI for a droid */
extern void aiUpdateDroid(DROID *psDroid);

// Find the nearest target to a droid
extern BOOL aiBestNearestTarget(DROID *psDroid, BASE_OBJECT **ppsObj);

/* See if there is a target in range */
extern BOOL aiChooseTarget(BASE_OBJECT *psObj,
						   BASE_OBJECT **ppsTarget);

/*set the droid to attack if wihin range otherwise move to target*/
extern void attackTarget(DROID *psDroid, BASE_OBJECT *psTarget);

/* See if there is a target in range for Sensor objects*/
extern BOOL aiChooseSensorTarget(BASE_OBJECT *psObj, BASE_OBJECT **ppsTarget);

/*set of rules which determine whether the weapon associated with the object
can fire on the propulsion type of the target*/
extern BOOL validTarget(BASE_OBJECT *psObject, BASE_OBJECT *psTarget);

#endif

