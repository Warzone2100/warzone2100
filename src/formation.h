/*
 * Formation.h
 *
 * Control units moving in formation.
 *
 */
#ifndef _formation_h
#define _formation_h

#include "formationdef.h"

typedef enum _formation_type
{
	FT_LINE,
	FT_COLUMN,
} FORMATION_TYPE;

// Initialise the formation system
extern BOOL formationInitialise(void);

// Shutdown the formation system
extern void formationShutDown(void);

// Create a new formation
extern BOOL formationNew(FORMATION **ppsFormation, FORMATION_TYPE type,
					SDWORD x, SDWORD y, SDWORD dir);

// Try and find a formation near to a location
extern BOOL formationFind(FORMATION **ppsFormation, SDWORD x, SDWORD y);

// Associate a unit with a formation
extern void formationJoin(FORMATION *psFormation, BASE_OBJECT *psObj);

// Remove a unit from a formation
extern void formationLeave(FORMATION *psFormation, BASE_OBJECT *psObj);

// remove all the members from a formation and release it
extern void formationReset(FORMATION *psFormation);

// re-insert all the units in the formation
extern void formationReorder(FORMATION *psFormation);

// get a target position to move into a formation
extern BOOL formationGetPos(FORMATION *psFormation, BASE_OBJECT *psObj,
					 SDWORD *pX, SDWORD *pY, BOOL bCheckLOS);

// See if a unit is a member of a formation (i.e. it has a position assigned)
extern BOOL formationMember(FORMATION *psFormation, BASE_OBJECT *psObj);

extern SDWORD formationObjRadius(BASE_OBJECT *psObj);

extern SDWORD formationGetSpeed( FORMATION *psFormation );

#endif


