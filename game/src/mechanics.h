/*
 * Mechanics.h
 *
 * Game world mechanics.
 *
 */
#ifndef _mechanics_h
#define _mechanics_h

#include "Frame.h"
#include "Combat.h"
#include "FindPath.h"
#include "GTime.h"
#include "Map.h"
#include "Mechanics.h"
#include "Move.h"
#include "Stats.h"
#include "Function.h"
#include "Research.h"
#include "Visibility.h"

/* Initialise the mechanics system */
//extern BOOL mechInitialise(void);

/* Shutdown the mechanics system */
extern BOOL mechShutdown(void);

// Allocate the list for a component
extern BOOL allocComponentList(COMPONENT_TYPE	type, SDWORD number);

// release all the component lists
extern void freeComponentLists(void);

//allocate the space for the Players' structure lists
extern BOOL allocStructLists(void);

// release the structure lists
extern void freeStructureLists(void);

//TEST FUNCTION - MAKE EVERYTHING AVAILABLE
extern void makeAllAvailable(void);

#endif

