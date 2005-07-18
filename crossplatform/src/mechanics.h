/*
 * Mechanics.h
 *
 * Game world mechanics.
 *
 */
#ifndef _mechanics_h
#define _mechanics_h

#include "frame.h"
#include "combat.h"
#include "findpath.h"
#include "gtime.h"
#include "map.h"
#include "mechanics.h"
#include "move.h"
#include "stats.h"
#include "function.h"
#include "research.h"
#include "visibility.h"

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

