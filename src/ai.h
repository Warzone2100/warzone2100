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
extern BOOL aiNearestTarget(DROID *psDroid, BASE_OBJECT **ppsObj);

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

