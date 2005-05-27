/*
 * AStar.h
 *
 */
#ifndef _AStar_h
#define _AStar_h


// the buffer to store a route in
#define ASTAR_MAXROUTE		50
typedef struct _astar_route
{
	POINT	asPos[ASTAR_MAXROUTE];
	SDWORD	finalX,finalY,numPoints;
} ASTAR_ROUTE;

// Sizes for the node heap
#define FPATH_NODEINIT		600
#define FPATH_NODEEXT		0

// counters for a-star
extern SDWORD astarInner,astarOuter, astarRemove;

// reset the astar counters
extern void astarResetCounters(void);

// Initialise the findpath routine
extern BOOL astarInitialise(void);

// Shutdown the findpath routine
extern void fpathShutDown(void);

// return codes for astar
enum
{
	ASR_OK,			// found a route
	ASR_FAILED,		// no route
	ASR_PARTIAL,	// routing cannot be finished this frame
					// and should be continued the next frame
	ASR_NEAREST,	// found a partial route to a nearby position
};

// route modes for astar
enum
{
	ASR_NEWROUTE,	// start a new route
	ASR_CONTINUE,	// continue a route that was partially completed the last frame
};

// A* findpath
extern SDWORD fpathAStarRoute(SDWORD routeMode, ASTAR_ROUTE *psRoutePoints,
							SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy);

// Check los between two tiles
extern BOOL fpathTileLOS(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2);

#endif
