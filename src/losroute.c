/*
 * LOSRoute.c
 *
 * Route finding code.
 *
 */

#include <stdlib.h>

// Wall hug printf's
//#define DEBUG_GROUP0
// optimise printf's
#define DEBUG_GROUP1
#include "Frame.h"

#include "Objects.h"
#include "Map.h"
#include "RayCast.h"

#ifdef TEST_BED
#include "main.h"
#endif

#include "LOSRoute.h"
#include "FPath.h"

// General findpath routing point
typedef struct _fp_point
{
	SDWORD		x,y;
	SDWORD		dist;
} FP_POINT;

// Maximum points in the temporary route
#define FPATH_ROUTEMAX		1000

// Tile line of sight callback state
typedef enum _tlos_state
{
	TLOS_NOOBST,		// no obstruction
	TLOS_NEARSIDE,		// found a clear tile on the near side of obstruction
	TLOS_FARSIDE,		// found a clear tile on the far side of obstruction
	TLOS_REPEAT,		// found a second obstruction

	TLOS_FAILED,		// no LOS for the ray
} TLOS_STATE;


// store wall hug positions and directions
typedef struct _fp_hugpoint
{
	SDWORD		x,y;
	SDWORD		leftDir, rightDir;
} FP_HUGPOINT;

// Types of route stack entries
typedef enum _stack_type
{
	ST_START,			// first entry in a route
	ST_INTERMEDIATE,	// in the middle of a route
	ST_END,				// final entry in a route

	ST_FAILED,			// route failed this way
} STACK_TYPE;

// store the current position in the search
typedef struct _fp_stack
{
	UBYTE	type;			// what type of route point this is
	UBYTE	dir;			// which direction to wall hug
	SWORD	start, end;		// start and end indexes of the wall hug
							// in asNearPoints and asFarPoints
	SWORD	prev;			// previous wall hug in the stack
	SDWORD	dist;
} FP_STACK;

// mask for the direction in the stack
#define ST_DIRMASK	0x0f
#define ST_HUGLEFT	0x10
#define ST_HUGRIGHT 0x20

// Current status of the route
typedef enum _route_state
{
	RS_SEARCHING,	// looking for a route
	RS_SUCCESS,		// route found
	RS_FAIL,		// no route found
} ROUTE_STATE;


// wall hug start and end points for the current path
#define FPATH_HUGPOINTMAX	100
static SDWORD		nearPoints, farPoints;
static FP_HUGPOINT asNearPoints[FPATH_HUGPOINTMAX];
static FP_HUGPOINT asFarPoints[FPATH_HUGPOINTMAX];

// Temporary store for routing points
static FP_POINT		asRightHug[FPATH_ROUTEMAX], asLeftHug[FPATH_ROUTEMAX];
static SDWORD		pathPoints;
static FP_POINT		asRoutePoints[FPATH_ROUTEMAX];

// Maximum number of points to wall hug for
#define FPATH_HUGMAX	1000

// stack to store the current position in the search
#define FPATH_STACKMAX		300
static FP_STACK	asRouteStack[FPATH_STACKMAX];

// Number of directions for wall following
#define NUM_DIR		4

// Direction changes for right and left turns
#define RIGHT_TURN	1
#define LEFT_TURN	-1

// Convert a direction into an offset
// dir 0 => x = 0, y = -1
static POINT aDirOffset[NUM_DIR] =
{
	 0, 1,
	-1, 0,
	 0,-1,
	 1, 0,
};

// the start and end of the current route for path optimisation
static SDWORD fpathRouteSX,fpathRouteSY, fpathRouteFX,fpathRouteFY;

// check if the tile in the specified direction is actually clear
BOOL fpathCheckHugDir(SDWORD x,SDWORD y, SDWORD dir)
{
	x = (x >> TILE_SHIFT) + aDirOffset[dir].x;
	y = (y >> TILE_SHIFT) + aDirOffset[dir].y;

	return !fpathBlockingTile(x,y);
}

// Convert two world coordinate points for a clear tile and an obstruction tile
// into a wall hug direction
void fpathPointsToHugDir(SDWORD clearx,SDWORD cleary, SDWORD obstx,SDWORD obsty,
						 SDWORD *pLeftDir, SDWORD *pRightDir)
{
	SDWORD	xdiff, ydiff;
	SDWORD	leftDir, rightDir;

	// get the difference in tile coords
	xdiff = (obstx >> TILE_SHIFT) - (clearx >> TILE_SHIFT);
	ydiff = (obsty >> TILE_SHIFT) - (cleary >> TILE_SHIFT);

	ASSERT(((xdiff >= -1 && xdiff <= 1 && ydiff >= -1 && ydiff <= 1),
		"fpathPointsToHugDir: points are more than one tile apart"));
	ASSERT((xdiff != 0 || ydiff != 0,
		"fpathPointsToHugDir: points are on same tile"));

	// not the most elegant solution but it works
	switch (xdiff + ydiff * 10)
	{
	case 10:	// xdiff == 0, ydiff == 1
		leftDir = 3; rightDir = 1;
		break;
	case 9:		// xdiff == -1, ydiff == 1
		leftDir = 0; rightDir = 1;
		break;
	case -1:	// xdiff == -1, ydiff == 0
		leftDir = 0; rightDir = 2;
		break;
	case -11:	// xdiff == -1, ydiff == -1
		leftDir = 1; rightDir = 2;
		break;
	case -10:	// xdiff == 0, ydiff == -1
		leftDir = 1; rightDir = 3;
		break;
	case -9:	// xdiff == 1, ydiff == -1
		leftDir = 2; rightDir = 3;
		break;
	case 1:		// xdiff == 1, ydiff == 0
		leftDir = 2; rightDir = 0;
		break;
	case 11:	// xdiff == 1, ydiff == 1
		leftDir = 3; rightDir = 0;
		break;
	default:
		ASSERT((FALSE, "fpathPointsToHugDir: unexpected point relationship"));
		leftDir = 0;
		rightDir = 2;
		break;
	}

	if (!fpathCheckHugDir(clearx,cleary, leftDir))
	{
		leftDir += LEFT_TURN;
		if (leftDir < 0)
		{
			leftDir += NUM_DIR;
		}
	}
	if (!fpathCheckHugDir(clearx,cleary, rightDir))
	{
		rightDir += RIGHT_TURN;
		if (rightDir >= NUM_DIR)
		{
			rightDir -= NUM_DIR;
		}
	}

	*pLeftDir = leftDir;
	*pRightDir = rightDir;
}


// The variables for the ray callbacks
static FP_POINT		sCurrBlock, sNearBlock, sFarBlock;
static TLOS_STATE	tlosState;
static SDWORD		finalX, finalY, vectorX,vectorY;

// initialise all the callback variables
void fpathInitCallback(SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy)
{
	sCurrBlock.x = sx;
	sCurrBlock.y = sy;
	sCurrBlock.dist = 0;
	finalX = fx;
	finalY = fy;
	vectorX = sx - fx;
	vectorY = sy - fy;
	tlosState = TLOS_NOOBST;
}

// callback to store all the obstacle intersections with a ray
BOOL fpathObstructionCallback(SDWORD x, SDWORD y, SDWORD dist)
{
	BOOL	updateCurr = TRUE;
	BOOL	cont = TRUE;
	SDWORD	vx,vy;
	SDWORD	leftHugDir, rightHugDir;
#ifdef TEST_BED
	MAPTILE	*psTile;
#endif

	// See if this point is past the final point (dot product)
	vx = x - finalX;
	vy = y - finalY;
	if (vx*vectorX + vy*vectorY <=0)
	{
		return FALSE;
	}

	switch (tlosState)
	{
	case TLOS_NOOBST:
		if (fpathBlockingTile(x>>TILE_SHIFT,y>>TILE_SHIFT))
		{
			tlosState = TLOS_NEARSIDE;

			// found an obstruction, note the last clear tile
			if (nearPoints < FPATH_HUGPOINTMAX)
			{
				asNearPoints[nearPoints].x = (sCurrBlock.x & ~TILE_MASK) + TILE_UNITS/2;
				asNearPoints[nearPoints].y = (sCurrBlock.y & ~TILE_MASK) + TILE_UNITS/2;

				// decide which direction the obstruction was hit for wall hugging
				fpathPointsToHugDir(sCurrBlock.x,sCurrBlock.y, x,y,
					&leftHugDir, &rightHugDir);
				asNearPoints[nearPoints].leftDir = leftHugDir;
				asNearPoints[nearPoints].rightDir = rightHugDir;

				nearPoints += 1;

				// DEBUG
#ifdef TEST_BED
				psTile = mapTile(sCurrBlock.x >> TILE_SHIFT, sCurrBlock.y >> TILE_SHIFT);
				psTile->tileInfoBits |= START;
#endif
			}
			else
			{
				ASSERT((FALSE, "fpathObstructionCallback: out of near points"));
				return FALSE;
			}
		}
		break;
	case TLOS_NEARSIDE:
		if (!fpathBlockingTile(x>>TILE_SHIFT,y>>TILE_SHIFT))
		{
			tlosState = TLOS_NOOBST;

			// got past the obstruction - note the clear tile
			if (farPoints < FPATH_HUGPOINTMAX)
			{
				asFarPoints[farPoints].x = (x & ~TILE_MASK) + TILE_UNITS/2;
				asFarPoints[farPoints].y = (y & ~TILE_MASK) + TILE_UNITS/2;

				// decide which direction the obstruction was hit for wall hugging
				fpathPointsToHugDir(x,y, sCurrBlock.x,sCurrBlock.y,
					&leftHugDir, &rightHugDir);
				asFarPoints[farPoints].leftDir = leftHugDir;
				asFarPoints[farPoints].rightDir = rightHugDir;
				farPoints += 1;

				// DEBUG
#ifdef TEST_BED
				psTile = mapTile(x >> TILE_SHIFT, y >> TILE_SHIFT);
				psTile->tileInfoBits |= FINISH;
#endif
			}
			else
			{
				ASSERT((FALSE, "fpathObstructionCallback: out of far points"));
				return FALSE;
			}
		}
		break;
	default:
		ASSERT((FALSE,"fpathObstructionCallback: unknown state"));
		break;
	}

	// store the current callback position
	sCurrBlock.x = x;
	sCurrBlock.y = y;
	sCurrBlock.dist = dist;

	return TRUE;
}


// The visibility ray callback
BOOL fpathTileVisCallback(SDWORD x, SDWORD y, SDWORD dist)
{
	SDWORD	vx,vy;

	// See if this point is past the final point (dot product)
	vx = x - finalX;
	vy = y - finalY;
	if (vx*vectorX + vy*vectorY <=0)
	{
		return FALSE;
	}

	// store the current callback position
	sCurrBlock.x = x;
	sCurrBlock.y = y;
	sCurrBlock.dist = dist;

	if (fpathBlockingTile(x>>TILE_SHIFT,y>>TILE_SHIFT))
	{
		// found an obstruction
		tlosState = TLOS_FAILED;
		return FALSE;
	}

	return TRUE;
}

// The visibility ray callback
BOOL fpathTileLOSCallback(SDWORD x, SDWORD y, SDWORD dist)
{
	BOOL	updateCurr = TRUE;
	BOOL	cont = TRUE;
	SDWORD	vx,vy;

	// See if this point is past the final point (dot product)
	vx = x - finalX;
	vy = y - finalY;
	if (vx*vectorX + vy*vectorY <=0)
	{
		return FALSE;
	}

	switch (tlosState)
	{
	case TLOS_NOOBST:
		if (fpathBlockingTile(x>>TILE_SHIFT,y>>TILE_SHIFT))
		{
			// found an obstruction, note the last clear tile
//			memcpy(&sNearBlock, &sCurrBlock, sizeof(FP_POINT));
			tlosState = TLOS_NEARSIDE;
		}
		break;
	case TLOS_NEARSIDE:
		if (!fpathBlockingTile(x>>TILE_SHIFT,y>>TILE_SHIFT))
		{
			// got past the obstruction - note the clear tile
//			sFarBlock.x = x;
//			sFarBlock.y = y;
//			sFarBlock.dist = dist;
			tlosState = TLOS_FARSIDE;
		}
		break;
	case TLOS_FARSIDE:
		if (fpathBlockingTile(x>>TILE_SHIFT,y>>TILE_SHIFT))
		{
			// found a second obstruction stop the callback for now
			// This obstruction will get dealt with on the next pass
			tlosState = TLOS_REPEAT;
			updateCurr = FALSE;
			cont = FALSE;
		}
		break;
	default:
		ASSERT((FALSE,"fpathTileLOSCallback: unknown state"));
		break;
	}

	// store the current callback position if necessary
	if (updateCurr)
	{
		sCurrBlock.x = x;
		sCurrBlock.y = y;
		sCurrBlock.dist = dist;
	}

	return cont;
}


// update a direction for wall hugging
static SDWORD fpathUpdateDir(SDWORD dir, SDWORD change)
{
		dir = dir + change;
		dir = dir < 0 ? dir += NUM_DIR : dir;
		dir = dir >= NUM_DIR ? dir -= NUM_DIR : dir;

		return dir;
}


// Get the direction for the next tile in a wall hug
BOOL fpathNextHugDir(SDWORD x, SDWORD y, SDWORD *pDir, SDWORD hugDir)
{
	SDWORD	nextDir, i;
	BOOL	blocked;

	// search around the current location for an unblocked tile
	// starting by turning in the hug direction
	nextDir = fpathUpdateDir(*pDir, hugDir);
	blocked = TRUE;
	for(i=0; i<NUM_DIR; i++)
	{
		if (!fpathBlockingTile(x + aDirOffset[nextDir].x,
						  y + aDirOffset[nextDir].y))
		{
			*pDir = nextDir;
			blocked = FALSE;
			break;
		}
		nextDir = fpathUpdateDir(nextDir, -hugDir);
	}

	return !blocked;
}


// Wall hug around an obstruction (using tile coords)
static BOOL fpathWallHug(SDWORD sx,SDWORD sy,	// start pos
						 SDWORD *pFarIndex,		// end position in asFarPoints
						 SDWORD dir,			// which way to face at start of wall hug
						 SDWORD hugDir,			// which way round the obstruction to wall hug
						 FP_POINT *asPoints, SDWORD *pNumPoints)	// wall hug points
{
	SDWORD	numPoints;
	SDWORD	x,y, testDir;
	SDWORD	i;
	BOOL	hugFinished;

	// decide which way the obstruction is
/*	for(dir=0; dir<NUM_DIR; dir++)
	{
		x = sx + aDirOffset[dir].x;
		y = sy + aDirOffset[dir].y;
		if (fpathBlockingTile(x,y))
		{
			break;
		}
	}*/
	// now turn away from the obstruction to wall hug
//	dir = fpathUpdateDir(dir, -hugDir);

	// Now wall hug around the obstacle
	numPoints = 1;
	asPoints[0].x = (sx << TILE_SHIFT) + TILE_UNITS/2;
	asPoints[0].y = (sy << TILE_SHIFT) + TILE_UNITS/2;
	x = sx;
	y = sy;
#ifdef TEST_BED
	mapTile(x,y)->tileInfoBits |= HUG;
#endif
	hugFinished = FALSE;
	while (!hugFinished)
	{
/*		// search around the current location for an unblocked tile
		// starting by turning in the hug direction
		testDir = fpathUpdateDir(dir, hugDir);
		blocked = TRUE;
		for(i=0; i<NUM_DIR; i++)
		{
			if (!fpathBlockingTile(x + aDirOffset[testDir].x,
							  y + aDirOffset[testDir].y))
			{
				dir = testDir;
				blocked = FALSE;
				break;
			}
			testDir = fpathUpdateDir(testDir, -hugDir);
		}*/

		if (!fpathNextHugDir(x,y, &dir, hugDir))
		{
			// we are in a location totally surrounded by blocking tiles
			DBP0(("wallHug: surrounded location\n"));
			goto exit_error;
		}

		// now move in the unblocked direction
		x += aDirOffset[dir].x;
		y += aDirOffset[dir].y;

		// check the hug hasn't looped back on itself
		// can't check for a repeated point earlier in the route because
		// there might be a dead end one tile wide
		if (x == sx && y == sy)
		{
			DBP0(("wallHug: revisited start\n"));
			goto exit_error;
		}
/*		for(i=0; i<numPoints; i++)
		{
			if (asPoints[i].x == (x << TILE_SHIFT) + TILE_UNITS/2 &&
				asPoints[i].y == (y << TILE_SHIFT) + TILE_UNITS/2)
			{
				DBP0(("wallHug: revisited point\n"));
				goto exit_error;
			}
		}*/

		// store the current location
		if (numPoints < FPATH_HUGMAX)
		{
#ifdef TEST_BED
			mapTile(x,y)->tileInfoBits |= HUG;
#endif
			asPoints[numPoints].x = (x << TILE_SHIFT) + TILE_UNITS/2;
			asPoints[numPoints].y = (y << TILE_SHIFT) + TILE_UNITS/2;
			numPoints += 1;
		}
		else
		{
			DBP0(("wallHug: out of points\n"));
			goto exit_error;
		}

		ASSERT((!fpathBlockingTile(x,y),
			"fpathWallHug: wall hugged onto a blocking tile"));

		// see if the route has got to a finish point
		for(i=0; i<farPoints; i++)
		{
			if (x == asFarPoints[i].x >> TILE_SHIFT &&
				y == asFarPoints[i].y >> TILE_SHIFT)
			{
				testDir = dir;
				if (hugDir == LEFT_TURN &&
					fpathNextHugDir(x,y,&testDir,hugDir) &&
					testDir == asFarPoints[i].rightDir)
				{
					*pFarIndex = i;
					hugFinished = TRUE;
					break;
				}
				else if (hugDir == RIGHT_TURN &&
						 fpathNextHugDir(x,y,&testDir,hugDir) &&
						 testDir == asFarPoints[i].leftDir)
				{
					*pFarIndex = i;
					hugFinished = TRUE;
					break;
				}
			}
		}
		// see if the route has got to a start point
		for(i=0; i<nearPoints; i++)
		{
			if (x == asNearPoints[i].x >> TILE_SHIFT &&
				y == asNearPoints[i].y >> TILE_SHIFT)
			{
				testDir = dir;
				if (hugDir == LEFT_TURN &&
					fpathNextHugDir(x,y,&testDir,hugDir) &&
					testDir == asNearPoints[i].rightDir)
				{
					*pFarIndex = i-1;
					hugFinished = TRUE;
					break;
				}
				else if (hugDir == RIGHT_TURN &&
						 fpathNextHugDir(x,y,&testDir,hugDir) &&
						 testDir == asNearPoints[i].leftDir)
				{
					*pFarIndex = i-1;
					hugFinished = TRUE;
					break;
				}
			}
		}
	}

	*pNumPoints = numPoints;

	return TRUE;

exit_error:
	*pNumPoints = numPoints;
	return FALSE;
}


// Get a semi-optimised distance for a wall hug
SDWORD fpathHugDistance(SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy,
						FP_POINT *asPoints, SDWORD numPoints)
{
	SDWORD		currIndex, search;
	SDWORD		ray;
	SDWORD		raySX,raySY, rayFX,rayFY;	// ray start and end coords
											// NB. ray goes from destination to start
	BOOL		targetVisible = FALSE;
	BOOL		losFailed;
	SDWORD		totalDist, currDist;
#ifdef DEBUG
	SDWORD		lastIndex = numPoints;
#endif

	ASSERT((numPoints != 0,
		"fpathHugDistance: no points to optimise"));

	raySX = fx;
	raySY = fy;
	currIndex = numPoints;
	search = numPoints-1;
	totalDist = 0;
	while (search >= 0)
	{
		// work down the list of points until there is no line of sight
		// or the target can be seen
		do
		{
			// get the current search coords
			search -= 1;
			if (search >= 0)
			{
				rayFX = asPoints[search].x;
				rayFY = asPoints[search].y;

			}
			else
			{
				rayFX = sx;
				rayFY = sy;
			}
			ray = rayPointsToAngle(raySX,raySY, rayFX,rayFY);
			fpathInitCallback(raySX,raySY, rayFX,rayFY);
			rayCast(raySX,raySY,
					ray, RAY_MAXLEN,
					fpathTileVisCallback);
			currDist = sCurrBlock.dist;
			losFailed = tlosState == TLOS_FAILED;

			// first see if the target point can be seen
			ray = rayPointsToAngle(rayFX,rayFY, sx,sy);
			fpathInitCallback(rayFX,rayFY, sx,sy);
			rayCast(rayFX,rayFY,
					ray, RAY_MAXLEN,
					fpathTileVisCallback);
			if (tlosState == TLOS_NOOBST)
			{
				// can see the target
				targetVisible = TRUE;
				break;
			}
		}
		while (!losFailed && search >= 0);

		if (losFailed)
		{
#ifdef DEBUG
			ASSERT((lastIndex != search,
				"fpathHugDist: LOS failed for neighbouring tile"));
			lastIndex = search;
#endif
			// move the start of the ray to the previous point then continue
			// scanning the route
			raySX = asPoints[search+1].x;
			raySY = asPoints[search+1].y;
			totalDist += currDist;

#ifdef TEST_BED
			mapTile(raySX>>TILE_SHIFT, raySY>>TILE_SHIFT)->tileInfoBits |= ROUTE;
#endif
		}

		if (targetVisible)
		{
			// there is LOS to the end of the route so add up the final distance
			totalDist += sCurrBlock.dist;

#ifdef TEST_BED
			mapTile(rayFX>>TILE_SHIFT, rayFY>>TILE_SHIFT)->tileInfoBits |= ROUTE;
#endif
			break;
		}
	}

	return totalDist;
}


// Calculate the distance between two points
SDWORD fpathCalcDistance(SDWORD x1, SDWORD y1, SDWORD x2, SDWORD y2)
{
	SDWORD	xdiff,ydiff, dist;

	xdiff = labs(x1 - x2);
	ydiff = labs(y1 - y2);

	dist = xdiff > ydiff ? xdiff + ydiff/2 : xdiff/2 + ydiff;

	return dist;
}


// Optimise a route
void fpathOptimiseRoute(SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy,
						FP_POINT *asPoints, SDWORD *pNumPoints, SDWORD *pDist)
{
	SDWORD		currIndex, search;
	SDWORD		ray;
	SDWORD		raySX,raySY, rayFX,rayFY;	// ray start and end coords
											// NB. ray goes from destination to start
	SDWORD		rayDist;
	SDWORD		newPoints, point;
	SDWORD		cx, cy;
	BOOL		targetVisible = FALSE;
	BOOL		losFailed;
#ifdef DEBUG
	SDWORD		lastIndex = *pNumPoints;
#endif

	ASSERT((*pNumPoints != 0,
		"fpathOptimiseRoute: no points to optimise"));

	raySX = fx;
	raySY = fy;
	currIndex = *pNumPoints;
	search = *pNumPoints-1;
	while (search >= 0)
	{
		// work down the list of points until there is no line of sight
		// or the target can be seen
		do
		{

			search -= 1;
			if (search >= 0)
			{
				// get the current search coords
				rayFX = asPoints[search].x;
				rayFY = asPoints[search].y;
			}
			else
			{
				rayFX = sx;
				rayFY = sy;
			}
			ray = rayPointsToAngle(raySX,raySY, rayFX,rayFY);
			fpathInitCallback(raySX,raySY, rayFX,rayFY);
			rayCast(raySX,raySY,
					ray, RAY_MAXLEN,
					fpathTileVisCallback);
			losFailed = tlosState == TLOS_FAILED;
			rayDist = sCurrBlock.dist;

/*			if (search >= 0)
			{
				// see if the target point can be seen
				ray = rayPointsToAngle(rayFX,rayFY, fpathRouteFX,fpathRouteFY);
				fpathInitCallback(rayFX,rayFY, fpathRouteFX,fpathRouteFY);
				rayCast(rayFX,rayFY,
						ray, RAY_MAXLEN,
						fpathTileVisCallback);
				if (tlosState == TLOS_NOOBST)
				{
					// can see the target
					targetVisible = TRUE;
					break;
				}
			}*/
		}
		while (!losFailed && search >= 0);

		// only store the point if there was an obstacle
		// i.e. not just reached the end of the points
		if (losFailed)
		{
#ifdef DEBUG
			ASSERT((lastIndex != search,
				"fpathOptimiseRoute: LOS failed for neighbouring tile"));
			lastIndex = search;
#endif
			// store the previous point (there was LOS for it)
			// and move the ray start to this point
			currIndex -= 1;
			raySX = asPoints[currIndex].x = asPoints[search+1].x;
			raySY = asPoints[currIndex].y = asPoints[search+1].y;
					asPoints[currIndex].dist = rayDist;

#ifdef TEST_BED
			mapTile(raySX>>TILE_SHIFT, raySY>>TILE_SHIFT)->tileInfoBits |= ROUTE;
#endif
		}

		if (targetVisible)
		{
			// store the current point cos there is LOS to the end of the route
			currIndex -= 1;
			asPoints[currIndex].x = asPoints[search].x;
			asPoints[currIndex].y = asPoints[search].y;
			asPoints[currIndex].dist = sCurrBlock.dist;

			targetVisible = FALSE;

#ifdef TEST_BED
			mapTile(asPoints[currIndex].x>>TILE_SHIFT,
					asPoints[currIndex].y>>TILE_SHIFT)->tileInfoBits |= ROUTE;
#endif
//			break;
		}
	}

	// Now move the optimised route to the start of the array
	newPoints = *pNumPoints - currIndex;
	memmove(asPoints, asPoints + currIndex, sizeof(FP_POINT) * newPoints);
	*pNumPoints = newPoints;

	//  add up the route length
	*pDist = 0;
	cx = sx; cy = sy;
	for(point=0; point<newPoints; point++)
	{
		*pDist += fpathCalcDistance(cx,cy,
						asPoints[point].x,asPoints[point].y);
		cx = asPoints[point].x;
		cy = asPoints[point].y;
	}
	*pDist += fpathCalcDistance(cx,cy, fx,fy);
}


// Append points from a wall hug to the end of a route
BOOL fpathAppendRoute(FP_POINT *asPoints, SDWORD numPoints)
{
	if (numPoints + pathPoints >= FPATH_ROUTEMAX)
	{
		// out of space in the route buffer
		return FALSE;
	}

	memcpy(&asRoutePoints[pathPoints], asPoints, numPoints * sizeof(FP_POINT));
	pathPoints = pathPoints + numPoints;

	return TRUE;
}


// Recursive part of function to:
// Build a full set of wall hug points for all obstacles given an end
// point of the route on the stack
BOOL fpathBuildRouteRec(SDWORD index)
{
	BOOL	success = TRUE;
	SDWORD	nearIndex, hugIndex, hugPoints;

	if (asRouteStack[index].prev != -1)
	{
		success = fpathBuildRouteRec(asRouteStack[index].prev);
	}

	if (success)
	{
		nearIndex = asRouteStack[index].start;
		if (asRouteStack[index].dir & ST_HUGLEFT)
		{
			// wall hug to the left
			success = fpathWallHug(asNearPoints[nearIndex].x >> TILE_SHIFT,
								   asNearPoints[nearIndex].y >> TILE_SHIFT,
								   &hugIndex, asNearPoints[nearIndex].leftDir,
								   RIGHT_TURN, asLeftHug, &hugPoints);
			success = success && fpathAppendRoute(asLeftHug, hugPoints);
		}
		else if (asRouteStack[index].dir & ST_HUGRIGHT)
		{
			// wall hug to the right
			success = fpathWallHug(asNearPoints[nearIndex].x >> TILE_SHIFT,
								   asNearPoints[nearIndex].y >> TILE_SHIFT,
								   &hugIndex, asNearPoints[nearIndex].rightDir,
								   LEFT_TURN, asRightHug, &hugPoints);
			success = success && fpathAppendRoute(asRightHug, hugPoints);
		}
	}

	return success;
}

// Build a full set of wall hug points for all obstacles given an end
// point of the route on the stack
BOOL fpathBuildRoute(SDWORD index)
{
	pathPoints = 0;
	return fpathBuildRouteRec(index);
}


// Check a route on the stack for loops
BOOL fpathRouteValid(SDWORD index)
{
	SDWORD	search, end;
	BOOL	valid;

	valid = TRUE;
	while (index > 0)
	{
		end = asRouteStack[index].end;
		for(search = asRouteStack[index].prev; search >=0;
			search = asRouteStack[search].prev)
		{
			if (asRouteStack[search].end == end)
			{
				valid = FALSE;
				break;
			}
		}
		index = asRouteStack[index].prev;
	}

	return valid;
}


// Find a route through the map on a tile basis
BOOL fpathTileRoute(MOVE_CONTROL *psMoveCntl,
					SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy)
{
	UDWORD		ray;		// angle to cast the ray
	SDWORD		leftPoints, rightPoints, prevPoints;
	SDWORD		leftDist, rightDist, routeDist, finalDist;
	BOOL		leftSuccess, rightSuccess;
	ROUTE_STATE	routeState;
	SDWORD		nearIndex, leftIndex, rightIndex;
	SDWORD		stackTop, stackCurr, stackMinDist;
	SDWORD		i;
	SDWORD		optSX,optSY, optFX,optFY;

	// If the route is all in the same tile, just return

	// See if the route starts on a blocking tile
	// move the start point to the nearest non blocking tile
	if (fpathBlockingTile(sx>>TILE_SHIFT,sy>>TILE_SHIFT))
	{
	}

	// Find the tiles on either side of any obstruction
	ray = rayPointsToAngle(sx,sy, fx,fy);
	fpathInitCallback(sx,sy, fx,fy);
	nearPoints = 0; farPoints = 0;
	rayCast(sx,sy,
			ray, RAY_MAXLEN,
			fpathObstructionCallback);

	if (nearPoints == 0)
	{
		// no obstructions - route succeeds
		asRoutePoints[0].x = fx;
		asRoutePoints[0].y = fy;
		pathPoints = 1;
		return TRUE;
	}

	// Initialise the stack
	stackTop = 0;
/*	asRouteStack[0].type = ST_START;
	asRouteStack[0].start = -1;
	asRouteStack[0].end = -1;
	asRouteStack[0].prev = -1;
	asRouteStack[0].dist = 0;*/

	// search for a route
	routeState = RS_SEARCHING;
	nearIndex = 0;
	routeDist = 0;
	pathPoints = 0;
	prevPoints = 0;
	stackCurr = -1;
	optSX = sx;
	optSY = sy;
	fpathRouteSX = sx;
	fpathRouteSY = sy;
	fpathRouteFX = fx;
	fpathRouteFY = fy;
	do
	{
		// wall hug to the left
		leftSuccess = fpathWallHug(asNearPoints[nearIndex].x >> TILE_SHIFT,
								   asNearPoints[nearIndex].y >> TILE_SHIFT,
								   &leftIndex, asNearPoints[nearIndex].leftDir,
								   RIGHT_TURN, asLeftHug, &leftPoints);
		// wall hug to the right
		rightSuccess = fpathWallHug(asNearPoints[nearIndex].x >> TILE_SHIFT,
									asNearPoints[nearIndex].y >> TILE_SHIFT,
									&rightIndex, asNearPoints[nearIndex].rightDir,
									LEFT_TURN, asRightHug, &rightPoints);

		// Now build two complete sets of points
		// the left route is in asRoutePoints, the right route is in asRightHug
		if (rightSuccess && rightPoints + pathPoints < FPATH_ROUTEMAX)
		{
			memmove(asRightHug + pathPoints, asRightHug, sizeof(FP_POINT)*rightPoints);
			memcpy(asRightHug, asRoutePoints, sizeof(FP_POINT)*pathPoints);
			rightPoints += pathPoints;
		}
		else
		{
			rightSuccess = FALSE;
		}
		if (leftSuccess && leftPoints + pathPoints < FPATH_ROUTEMAX)
		{
			memcpy(asRoutePoints + pathPoints, asLeftHug, sizeof(FP_POINT)*leftPoints);
			pathPoints += leftPoints;
		}
		else
		{
			leftSuccess = FALSE;
		}

//		pathPoints = prevPoints;
//		leftSuccess = leftSuccess && fpathAppendRoute(asLeftHug,leftPoints);
		if (leftSuccess)
		{


			// optimise the route
			if (leftIndex + 1 < nearPoints)
			{
				optFX = asNearPoints[leftIndex + 1].x;
				optFY = asNearPoints[leftIndex + 1].y;
			}
			else
			{
				optFX = fx;
				optFY = fy;
			}
/*			leftDist = fpathHugDistance(optSX, optSY, optFX, optFY,
					asRoutePoints, pathPoints);*/
			do
			{
				prevPoints = pathPoints;
				fpathOptimiseRoute(sx, sy, optFX, optFY,
						asRoutePoints, &pathPoints, &leftDist);
			} while (pathPoints != 0 && prevPoints != pathPoints);

			// store the result on the stack
			if (stackTop < FPATH_STACKMAX)
			{
				// store the new route end
				asRouteStack[stackTop].type = ST_END;
				asRouteStack[stackTop].dir = (UBYTE)(asNearPoints[nearIndex].leftDir & ST_DIRMASK);
				asRouteStack[stackTop].dir |= (UBYTE)ST_HUGLEFT;
				asRouteStack[stackTop].start = (SWORD)nearIndex;
				asRouteStack[stackTop].end = (SWORD)leftIndex;
				asRouteStack[stackTop].dist = leftDist;// + routeDist;
				asRouteStack[stackTop].prev = (SWORD)stackCurr;

				// check the route so far is valid
				if (!fpathRouteValid(stackTop))
				{
					asRouteStack[stackTop].type = ST_FAILED;
				}
				stackTop += 1;

				// mark the previous point as visitied
				asRouteStack[stackCurr].type = ST_INTERMEDIATE;
			}
			else
			{
				routeState = RS_FAIL;
			}
		}

//		pathPoints = prevPoints;
//		rightSuccess = rightSuccess && fpathAppendRoute(asRightHug,rightPoints);
		if (rightSuccess)
		{
			// optimise the route
			if (rightIndex + 1 < nearPoints)
			{
				optFX = asNearPoints[rightIndex + 1].x;
				optFY = asNearPoints[rightIndex + 1].y;
			}
			else
			{
				optFX = fx;
				optFY = fy;
			}
/*			rightDist = fpathHugDistance(optSX, optSY, optFX, optFY,
					asRoutePoints, pathPoints);*/
			do
			{
				prevPoints = rightPoints;
				fpathOptimiseRoute(sx, sy, optFX, optFY,
						asRightHug, &rightPoints, &rightDist);
			} while (rightPoints != 0 && prevPoints != rightPoints);

			// store the result on the stack
			if (stackTop < FPATH_STACKMAX)
			{
				// store the new route end
				asRouteStack[stackTop].type = ST_END;
				asRouteStack[stackTop].dir = (UBYTE)(asNearPoints[nearIndex].rightDir & ST_DIRMASK);
				asRouteStack[stackTop].dir |= (UBYTE)ST_HUGRIGHT;
				asRouteStack[stackTop].start = (SWORD)nearIndex;
				asRouteStack[stackTop].end = (SWORD)rightIndex;
				asRouteStack[stackTop].dist = (SWORD)rightDist;// + routeDist;
				asRouteStack[stackTop].prev = (SWORD)stackCurr;

				// check the route so far is valid
				if (!fpathRouteValid(stackTop))
				{
					asRouteStack[stackTop].type = ST_FAILED;
				}
				stackTop += 1;

				// mark the previous point as visitied
				asRouteStack[stackCurr].type = ST_INTERMEDIATE;
			}
			else
			{
				routeState = RS_FAIL;
			}
		}

		if (!leftSuccess && !rightSuccess)
		{
			// mark the previous point as visitied
			asRouteStack[stackCurr].type = ST_FAILED;
		}

		if (routeState == RS_SEARCHING)
		{
			// find the route with the lowest distance so far
			stackMinDist = SDWORD_MAX;
			stackCurr = -1;
			for(i=0; i<stackTop; i++)
			{
				if (asRouteStack[i].type == ST_END &&
					asRouteStack[i].dist < stackMinDist)
				{
					stackMinDist = asRouteStack[i].dist;
					stackCurr = i;
				}
			}
			if (stackCurr == -1)
			{
				// failed to find a route
				routeState = RS_FAIL;
			}
			else
			{
				nearIndex = asRouteStack[stackCurr].end + 1;
				routeDist = asRouteStack[stackCurr].dist;

				// find the end of the previous route
				optSX = asFarPoints[asRouteStack[stackCurr].end].x;
				optSY = asFarPoints[asRouteStack[stackCurr].end].y;

				// build the route points into the route buffer
				fpathBuildRoute(stackCurr);
//				prevPoints = pathPoints;
			}

			// See if the route has got to the target
			if (nearIndex == nearPoints)
			{
				routeState = RS_SUCCESS;
			}
		}

	} while (routeState == RS_SEARCHING);

	if (routeState == RS_SUCCESS)
	{
		// Set the hug flags
/*		for(i=0; i<pathPoints; i++)
		{
			mapTile(asRoutePoints[i].x >> TILE_SHIFT,
					asRoutePoints[i].y >> TILE_SHIFT)->tileInfoBits |= HUG;
		}*/

		// Do a final optimise on the route
		do
		{
			prevPoints = pathPoints;
			fpathOptimiseRoute(sx, sy, fx, fy,
						   asRoutePoints, &pathPoints, &finalDist);
		} while (pathPoints != 0 && prevPoints != pathPoints);

		if (pathPoints == 0)
		{
			asRoutePoints[0].x = fx;
			asRoutePoints[0].y = fy;
			pathPoints = 1;
		}

		// copy the route into the move control structure
		if (pathPoints >= TRAVELSIZE)
		{
			pathPoints = TRAVELSIZE-1;
		}
		for(i=0; i<pathPoints; i++)
		{
			psMoveCntl->MovementList[i].XCoordinate = asRoutePoints[i].x;
			psMoveCntl->MovementList[i].YCoordinate = asRoutePoints[i].y;
		}
		psMoveCntl->MovementList[pathPoints].XCoordinate = -1;
		psMoveCntl->MovementList[pathPoints].YCoordinate = -1;
	}
	else
	{
		psMoveCntl->MovementList[0].XCoordinate = -1;
		psMoveCntl->MovementList[0].YCoordinate = -1;
	}

	return routeState == RS_SUCCESS;
}



/*BOOL fpathTileRoute(SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy)
{
	UDWORD		ray;		// angle to cast the ray
//	UDWORD		length;
//	SDWORD		xdiff,ydiff;
	MAPTILE		*psTile;
	SDWORD		leftPoints, rightPoints;
	SDWORD		leftDist, rightDist, finalDist;
	SDWORD		raySX, raySY;
	SDWORD		hugSX,hugSY, hugFX,hugFY;
	SDWORD		leftDir, rightDir;
	BOOL		leftSuccess, rightSuccess, routeFailed;
	TLOS_STATE	cbState;
	SDWORD		joinX,joinY;

	// If the route is all in the same tile, just return

	// See if the route starts on a blocking tile
	// move the start point to the nearest non blocking tile
	if (fpathBlockingTile(sx>>TILE_SHIFT,sy>>TILE_SHIFT))
	{
	}

	routePoints = 0;
	joinX = raySX = sx;
	joinY = raySY = sy;

	// work down the los ray wall hugging around all the obstacles
	routeFailed = FALSE;
	do
	{
		// Find the tiles on either side of any obstruction
		ray = rayPointsToAngle(raySX,raySY, fx,fy);
		fpathInitCallback(raySX,raySY, fx,fy);
		rayCast(raySX,raySY,
				ray, SDWORD_MAX,
				fpathTileLOSCallback);
		raySX = sCurrBlock.x;
		raySY = sCurrBlock.y;
		cbState = tlosState;

		switch (cbState)
		{
		case TLOS_NOOBST:
			// This will be the final point on the route
			asRoutePoints[routePoints].x = fx;
			asRoutePoints[routePoints].y = fy;
			routePoints += 1;
			break;
		case TLOS_NEARSIDE:
			psTile = mapTile(sNearBlock.x >> TILE_SHIFT, sNearBlock.y >> TILE_SHIFT);
			psTile->tileInfoBits |= START;

			// This will be the final point on the route
			asRoutePoints[routePoints].x = sNearBlock.x;
			asRoutePoints[routePoints].y = sNearBlock.y;
			routePoints += 1;
			break;
		case TLOS_FARSIDE:
		case TLOS_REPEAT:
			psTile = mapTile(sNearBlock.x >> TILE_SHIFT, sNearBlock.y >> TILE_SHIFT);
			psTile->tileInfoBits |= START;
			psTile = mapTile(sFarBlock.x >> TILE_SHIFT, sFarBlock.y >> TILE_SHIFT);
			psTile->tileInfoBits |= FINISH;

			hugSX = sNearBlock.x;
			hugSY = sNearBlock.y;
			hugFX = sFarBlock.x;
			hugFY = sFarBlock.y;
			leftDir = leftHugDir;
			rightDir = rightHugDir;

			// Got an obstruction wall hug around it both ways
			leftSuccess = fpathWallHug(hugSX >> TILE_SHIFT, hugSY >> TILE_SHIFT,
									 hugFX >> TILE_SHIFT, hugFY >> TILE_SHIFT,
									 leftDir, RIGHT_TURN, asLeftHug, &leftPoints);
			rightSuccess = fpathWallHug(hugSX >> TILE_SHIFT, hugSY >> TILE_SHIFT,
									 hugFX >> TILE_SHIFT, hugFY >> TILE_SHIFT,
									 rightDir, LEFT_TURN, asRightHug, &rightPoints);

			// Optimise the wall hugs to see which is the shortest
			if (leftSuccess)
			{
				fpathOptimiseRoute(joinX, joinY, raySX, raySY,
							   asLeftHug, &leftPoints, &leftDist);
			}
			if (rightSuccess)
			{
				fpathOptimiseRoute(joinX, joinY, raySX, raySY,
							   asRightHug, &rightPoints, &rightDist);
			}

			// Now add the shorter route to what we have so far
			if (leftSuccess && rightSuccess)
			{
				if (leftDist < rightDist)
				{
					// Add the left wall hug to the route
					if (leftPoints + routePoints >= FPATH_ROUTEMAX)
					{
						leftPoints = FPATH_ROUTEMAX - routePoints;
						routeFailed = TRUE;
					}
					memcpy(asRoutePoints + routePoints, asLeftHug, sizeof(FP_POINT) * leftPoints);
					routePoints += leftPoints;
				}
				else
				{
					// Add the right wall hug to the route
					if (rightPoints + routePoints >= FPATH_ROUTEMAX)
					{
						rightPoints = FPATH_ROUTEMAX - routePoints;
						routeFailed = TRUE;
					}
					memcpy(asRoutePoints + routePoints, asRightHug, sizeof(FP_POINT) * rightPoints);
					routePoints += rightPoints;
				}
			}
			else if (leftSuccess)
			{
				// Add the left wall hug to the route
				if (leftPoints + routePoints >= FPATH_ROUTEMAX)
				{
					leftPoints = FPATH_ROUTEMAX - routePoints;
					routeFailed = TRUE;
				}
				memcpy(asRoutePoints + routePoints, asLeftHug, sizeof(FP_POINT) * leftPoints);
				routePoints += leftPoints;
			}
			else if (rightSuccess)
			{
				// Add the right wall hug to the route
				if (rightPoints + routePoints >= FPATH_ROUTEMAX)
				{
					rightPoints = FPATH_ROUTEMAX - routePoints;
					routeFailed = TRUE;
				}
				memcpy(asRoutePoints + routePoints, asRightHug, sizeof(FP_POINT) * rightPoints);
				routePoints += rightPoints;
			}
			else
			{
				// neither wall hug found a route, move to the obstacle and fail
				asRoutePoints[routePoints].x = hugSX;
				asRoutePoints[routePoints].y = hugSY;
				routePoints += 1;

				routeFailed = TRUE;
			}

			// Update the join position for the next bit of route
			joinX = asRoutePoints[routePoints -1].x;
			joinY = asRoutePoints[routePoints -1].y;
			break;
		}
	} while (cbState == TLOS_REPEAT && !routeFailed);

	// Do a final optimise on the route
	fpathOptimiseRoute(sx, sy, fx, fy,
				   asRoutePoints, &routePoints, &finalDist);

	return !routeFailed;
}*/

