/*
 * AStar.c
 *
 * A* based findpath
 *
 */

// A* printf's
//#define DEBUG_GROUP0
// open list printf's
//#define DEBUG_GROUP1
// summary info printf's
//#define DEBUG_GROUP2
#include <assert.h>

#include "lib/framework/frame.h"

#include "objects.h"
#include "map.h"
#include "raycast.h"
#include "fpath.h"

#include "astar.h"

// open list storage methods :
// binary tree - 0
// ordered list - 1
// unordered list - 2
#define OPEN_LIST		2

SDWORD	firstHit, secondHit;
SDWORD	astarInner,astarOuter, astarRemove;

// The structure to store a node of the route
typedef struct _fp_node
{
	SWORD	x,y;		// map coords
	SWORD	dist, est;	// distance so far and estimate to end
	SWORD	type;		// open or closed node

#if OPEN_LIST == 0
	struct _fp_node *psLeft, *psRight;	// Open list pointer
#else
	struct _fp_node *psOpen;
#endif
	struct _fp_node	*psRoute;	// Previous point in the route

	struct _fp_node *psNext;
} FP_NODE;

// types of node
#define NT_OPEN		1
#define NT_CLOSED	2

// List of open nodes
FP_NODE		*psOpen;

// Size of closed hash table
//#define FPATH_TABLESIZE		20671
#define FPATH_TABLESIZE		4091

#if OPEN_LIST == 2
// Hash table for closed nodes
FP_NODE		**apsNodes=NULL;
#else
// Hash table for closed nodes
FP_NODE		**apsClosed;
// Hash table for open nodes
FP_NODE		**apsOpen;
#endif

// object heap to store nodes
OBJ_HEAP	*psFPNodeHeap;


/*#define NUM_DIR		4
// Convert a direction into an offset
// dir 0 => x = 0, y = -1
static POINT aDirOffset[NUM_DIR] =
{
	 0, 1,
	-1, 0,
	 0,-1,
	 1, 0,
};*/

#define NUM_DIR		8
// Convert a direction into an offset
// dir 0 => x = 0, y = -1
static POINT aDirOffset[NUM_DIR] =
{
	{ 0, 1},
	{-1, 1},
	{-1, 0},
	{-1,-1},
	{ 0,-1},
	{ 1,-1},
	{ 1, 0},
	{ 1, 1},
};


// reset the astar counters
void astarResetCounters(void)
{
	astarInner = 0;
	astarOuter = 0;
	astarRemove = 0;
}
void ClearAstarNodes(void)
{
	assert(apsNodes);
	memset(apsNodes, 0, sizeof(FP_NODE *) * FPATH_TABLESIZE);
}

UDWORD GetApsNodesSize(void)
{
	return(sizeof(FP_NODE *) * FPATH_TABLESIZE);
}

// Initialise the findpath routine
BOOL astarInitialise(void)
{
	// Create the node heap
	if (!HEAP_CREATE(&psFPNodeHeap, sizeof(FP_NODE), FPATH_NODEINIT, FPATH_NODEEXT))
	{
		return FALSE;
	}

#if OPEN_LIST == 2
	apsNodes = MALLOC(sizeof(FP_NODE *) * FPATH_TABLESIZE);
	if (!apsNodes)
	{
		return FALSE;
	}
	ClearAstarNodes();
#else
	// Create the two hash tables
	apsOpen = MALLOC(sizeof(FP_NODE *) * FPATH_TABLESIZE);
	if (!apsOpen)
	{
		return FALSE;
	}
	memset(apsOpen, 0, sizeof(FP_NODE *) * FPATH_TABLESIZE);
	apsClosed = MALLOC(sizeof(FP_NODE *) * FPATH_TABLESIZE);
	if (!apsClosed)
	{
		return FALSE;
	}
	memset(apsClosed, 0, sizeof(FP_NODE *) * FPATH_TABLESIZE);
#endif

	return TRUE;
}


// Shutdown the findpath routine
void fpathShutDown(void)
{
	HEAP_DESTROY(psFPNodeHeap);
#if OPEN_LIST == 2
	FREE(apsNodes);
#else
	FREE(apsOpen);
	FREE(apsClosed);
#endif
}

// calculate a hash table index
#define RAND_MULTI	25173
#define RAND_INC	13849
#define RAND_MOD	0xffff
/*SDWORD fpathHashFunc(SDWORD x, SDWORD y)
{
	UDWORD	index;

	index = ((UDWORD)x * RAND_MULTI + RAND_INC) & RAND_MOD;
	index = index ^ ((UDWORD)y * RAND_MULTI + RAND_INC) & RAND_MOD;
	index = index % FPATH_TABLESIZE;

	return index;
}*/

/* next four used in HashPJW */
#define	BITS_IN_int		32
#define	THREE_QUARTERS	((UINT) ((BITS_IN_int * 3) / 4))
#define	ONE_EIGHTH		((UINT) (BITS_IN_int / 8))
#define	HIGH_BITS		( ~((UINT)(~0) >> ONE_EIGHTH ))

/***************************************************************************/
/*
 * HashString
 *
 * Adaptation of Peter Weinberger's (PJW) generic hashing algorithm listed
 * in Binstock+Rex, "Practical Algorithms" p 69.
 *
 * Accepts string and returns hashed integer.
 *
 * Hack to use coordinates instead of a string by John.
 */
/***************************************************************************/
SDWORD fpathHashFunc(SDWORD x, SDWORD y)
{
	UINT	iHashValue, i;
	CHAR	*c;
	CHAR	aBuff[8];

	memcpy(&aBuff[0], &x, 4);
	memcpy(&aBuff[4], &y, 4);
	c = aBuff;

	for ( iHashValue=0; c < &aBuff[8]; ++c )
	{
		if (*c != 0)
		{
			iHashValue = ( iHashValue << ONE_EIGHTH ) + *c;

			if ( (i = iHashValue & HIGH_BITS) != 0 )
			{
				iHashValue = ( iHashValue ^ ( i >> THREE_QUARTERS ) ) &
								~HIGH_BITS;
			}
		}
	}

	iHashValue %= FPATH_TABLESIZE;

	return iHashValue;
}

// Add a node to the hash table
void fpathHashAdd(FP_NODE *apsTable[], FP_NODE *psNode)
{
	SDWORD	index;

	index = fpathHashFunc(psNode->x, psNode->y);

	psNode->psNext = apsTable[index];
	apsTable[index] = psNode;
}


// See if a node is in the hash table
FP_NODE *fpathHashPresent(FP_NODE *apsTable[], SDWORD x, SDWORD y)
{
	SDWORD		index;
	FP_NODE		*psFound;

	index = fpathHashFunc(x,y);
	psFound = apsTable[index];
	while (psFound && !(psFound->x == x && psFound->y == y))
	{
		psFound = psFound->psNext;
	}

	return psFound;
}

/*	FP_NODE		*psPrev, *psFound;

	index = fpathHashFunc(x,y);

	return psFound;
}*/


// Remove a node from the hash table
FP_NODE *fpathHashRemove(FP_NODE *apsTable[], SDWORD x, SDWORD y)
{
	SDWORD		index;
	FP_NODE		*psPrev, *psFound;

	index = fpathHashFunc(x,y);
	if (apsTable[index] &&
		apsTable[index]->x == x && apsTable[index]->y == y)
	{
		psFound = apsTable[index];
		apsTable[index] = apsTable[index]->psNext;
	}
	else
	{
		psPrev = NULL;
		for(psFound = apsTable[index]; psFound; psFound = psFound->psNext)
		{
			if (psFound->x == x && psFound->y == y)
			{
				break;
			}
			psPrev = psFound;
		}
		if (psFound)
		{
			psPrev->psNext = psFound->psNext;
		}
	}

	return psFound;
}

// Remove a node from the hash table
FP_NODE *fpathHashCondRemove(FP_NODE *apsTable[], SDWORD x, SDWORD y, SDWORD dist)
{
	SDWORD		index;
	FP_NODE		*psPrev, *psFound;

	index = fpathHashFunc(x,y);
	if (apsTable[index] &&
		apsTable[index]->x == x && apsTable[index]->y == y)
	{
		psFound = apsTable[index];
		if (psFound->dist > dist)
		{
			apsTable[index] = apsTable[index]->psNext;
		}
		firstHit ++;
	}
	else
	{
		psPrev = NULL;
		for(psFound = apsTable[index]; psFound; psFound = psFound->psNext)
		{
			if (psFound->x == x && psFound->y == y)
			{
				break;
			}
			psPrev = psFound;
		}
		if (psFound && psFound->dist > dist)
		{
			psPrev->psNext = psFound->psNext;
		}
		secondHit ++;
	}

	return psFound;
}


// Reset the hash tables
void fpathHashReset(void)
{
	SDWORD	i;
	FP_NODE	*psNext;

	for(i=0; i<FPATH_TABLESIZE; i++)
	{
#if OPEN_LIST == 2
		while (apsNodes[i])
		{
			psNext = apsNodes[i]->psNext;
			HEAP_FREE(psFPNodeHeap, apsNodes[i]);
			apsNodes[i] = psNext;
		}
#else
		while (apsOpen[i])
		{
			psNext = apsOpen[i]->psNext;
			HEAP_FREE(psFPNodeHeap, apsOpen[i]);
			apsOpen[i] = psNext;
		}
		while (apsClosed[i])
		{
			psNext = apsClosed[i]->psNext;
			HEAP_FREE(psFPNodeHeap, apsClosed[i]);
			apsClosed[i] = psNext;
		}
#endif
	}

	HEAP_RESET(psFPNodeHeap);
}


// Compare two nodes
__inline SDWORD fpathCompare(FP_NODE *psFirst, FP_NODE *psSecond)
{
	SDWORD	first,second;

	first = psFirst->dist + psFirst->est;
	second = psSecond->dist + psSecond->est;
	if (first < second)
	{
		return -1;
	}
	else if (first > second)
	{
		return 1;
	}

	// equal totals, give preference to node closer to target
	if (psFirst->est < psSecond->est)
	{
		return -1;
	}
	else if (psFirst->est > psSecond->est)
	{
		return 1;
	}

	// exactly equal
	return 0;
}


// make a 50/50 random choice
static UWORD seed = 1234;
BOOL fpathRandChoice(void)
{
	UDWORD	val;

	val = (seed * RAND_MULTI + RAND_INC) & RAND_MOD;
	seed = (UWORD)val;

	return val & 1;
}

#if OPEN_LIST == 0

// Add a node to the open list
void fpathOpenAdd(FP_NODE *psNode)
{
	FP_NODE		*psCurr, *psPrev;
	SDWORD		comp;

	psNode->psLeft = NULL;
	psNode->psRight = NULL;

	if (psOpen == NULL)
	{
		// Nothing in the open set, add at root
		psOpen = psNode;
	}
	else
	{
		// Search the tree for the insertion point
		psCurr = psOpen;
		while (psCurr != NULL)
		{
			psPrev = psCurr;
			comp = fpathCompare(psNode, psCurr);
			if (comp < 0)
			{
				psCurr = psCurr->psLeft;
			}
			else
			{
				psCurr = psCurr->psRight;
			}
		}

		// Add the node to the tree
		if (comp < 0)
		{
			psPrev->psLeft = psNode;
		}
		else
		{
			psPrev->psRight = psNode;
		}
	}
}

// Remove a node from the open list
void fpathOpenRemove(FP_NODE *psNode)
{
	FP_NODE		*psCurr, *psParent, *psSubParent, *psInsert;
	SDWORD		comp;

	if (psNode == psOpen)
	{
		// deleting the root
		psParent = NULL;
	}
	else
	{
		// find the parent of the node to delete
		psCurr = psOpen;
		while (psCurr && psCurr != psNode)
		{
			psParent = psCurr;
			comp = fpathCompare(psNode, psCurr);
			if (comp < 0)
			{
				psCurr = psCurr->psLeft;
			}
			else
			{
				psCurr = psCurr->psRight;
			}
		}
		ASSERT( psCurr != NULL,
			"fpathOpenRemove: couldn't find node" );
	}

	// Find the node to take the deleted nodes place in the tree
	if (psNode->psLeft == NULL)
	{
		// simple case only right subtree
		psInsert = psNode->psRight;
	}
	else if (psNode->psRight == NULL)
	{
		// simple case only left subtree
		psInsert = psNode->psLeft;
	}
	else
	{
		// both subtrees present - find a node to replace this one in the tree
		psInsert = psNode->psRight;
		if (psInsert->psLeft == NULL)
		{
			psInsert->psLeft = psNode->psLeft;
		}
		else
		{
			while (psInsert->psLeft != NULL)
			{
				psSubParent = psInsert;
				psInsert=psInsert->psLeft;
			}
			psSubParent->psLeft = psInsert->psRight;
			psInsert->psLeft = psNode->psLeft;
			psInsert->psRight = psNode->psRight;
		}
	}

	// replace the deleted node in the tree
	if (psParent == NULL)
	{
		psOpen = psInsert;
	}
	else
	{
		if (comp < 0)
		{
			psParent->psLeft = psInsert;
		}
		else
		{
			psParent->psRight = psInsert;
		}
	}

	psNode->psLeft = NULL;
	psNode->psRight = NULL;
}

// Get the nearest entry in the open list
FP_NODE *fpathOpenGet(void)
{
	FP_NODE	*psNode, *psPrev;

	if (psOpen == NULL)
	{
		return NULL;
	}

	psPrev = NULL;
	for(psNode = psOpen; psNode->psLeft != NULL; psNode = psNode->psLeft)
	{
		psPrev = psNode;
	}

	if (psPrev == NULL)
	{
		psOpen = psNode->psRight;
	}
	else
	{
		psPrev->psLeft = psNode->psRight;
	}

	return psNode;
}


// Check the binary tree is valid
BOOL fpathValidateTree(FP_NODE *psNode)
{
	BOOL	left, right;

	left = TRUE;
	if (psNode->psLeft)
	{
		if (fpathCompare(psNode->psLeft, psNode) >= 0)
		{
			left = FALSE;
		}
		if (left)
		{
			left = fpathValidateTree(psNode->psLeft);
		}
	}
	right = TRUE;
	if (psNode->psRight)
	{
		if (fpathCompare(psNode->psRight, psNode) < 0)
		{
			right = FALSE;
		}
		if (right)
		{
			right = fpathValidateTree(psNode->psRight);
		}
	}

	return left && right;
}

#elif OPEN_LIST == 1

// Add a node to the open list
void fpathOpenAdd(FP_NODE *psNode)
{
	FP_NODE		*psCurr,*psPrev;

	if (psOpen == NULL || fpathCompare(psNode, psOpen) < 0)
	{
		// Add to start
		psNode->psOpen = psOpen;
		psOpen = psNode;
		DBP1(("OpenAdd: start\n"));
	}
	else
	{
		// Add in the middle
		psPrev = NULL;
		for(psCurr = psOpen; psCurr && fpathCompare(psNode, psCurr) >= 0;
			psCurr = psCurr->psOpen)
		{
			psPrev = psCurr;
		}
		psNode->psOpen = psCurr;
		psPrev->psOpen = psNode;
		DBP1(("OpenAdd: after %d,%d dist %d\n",
			psPrev->x,psPrev->y, psPrev->dist));
	}
}


// Get the nearest entry in the open list
FP_NODE *fpathOpenGet(void)
{
	FP_NODE	*psNode;

	psNode = psOpen;
	psOpen = psOpen->psOpen;

	return psNode;
}

#elif OPEN_LIST == 2

// Add a node to the open list
void fpathOpenAdd(FP_NODE *psNode)
{
	psNode->psOpen = psOpen;
	psOpen = psNode;
}

// Get the nearest entry in the open list
FP_NODE *fpathOpenGet(void)
{
	FP_NODE	*psNode, *psCurr, *psPrev, *psParent = NULL;
	SDWORD	comp;

	if (psOpen == NULL)
	{
		return NULL;
	}

	// find the node with the lowest distance
	psPrev = NULL;
	psNode = psOpen;
	for(psCurr = psOpen; psCurr; psCurr = psCurr->psOpen)
	{
		comp = fpathCompare(psCurr, psNode);
		if (comp < 0 || (comp == 0 && fpathRandChoice()))
		{
			psParent = psPrev;
			psNode = psCurr;
		}
		psPrev = psCurr;
	}

	// remove the node from the list
	if (psNode == psOpen)
	{
		// node is at head of list
		psOpen = psOpen->psOpen;
	}
	else
	{
		psParent->psOpen = psNode->psOpen;
	}

	return psNode;
}

#endif

#if OPEN_LIST == 1 || OPEN_LIST == 2
// Remove a node from the open list
void fpathOpenRemove(FP_NODE *psNode)
{
	FP_NODE		*psCurr,*psPrev;

	if (psOpen == NULL)
	{
		ASSERT( FALSE, "fpathOpenRemove: NULL list" );
		return;
	}
	else if (psNode == psOpen)
	{
		// Remove from start
		psOpen = psOpen->psOpen;
	}
	else
	{
		// remove from the middle
		psPrev = NULL;
		for(psCurr = psOpen; psCurr && psCurr != psNode;
			psCurr = psCurr->psOpen)
		{
			psPrev = psCurr;
		}
		if (psCurr)
		{
			psPrev->psOpen = psCurr->psOpen;
		}
		else
		{
			ASSERT( FALSE, "fpathOpenRemove: failed to find node" );
			return;
		}
	}
}

#endif


// estimate the distance to the target point
SDWORD fpathEstimate(SDWORD x, SDWORD y, SDWORD fx, SDWORD fy)
{
	SDWORD xdiff, ydiff;

	xdiff = x > fx ? x - fx : fx - x;
	ydiff = y > fy ? y - fy : fy - y;

	xdiff = xdiff * 10;
	ydiff = ydiff * 10;

	return xdiff > ydiff ? xdiff + ydiff/2 : xdiff/2 + ydiff;
}


// Generate a new node
FP_NODE *fpathNewNode(SDWORD x, SDWORD y, SDWORD dist, FP_NODE *psRoute)
{
	FP_NODE	*psNode;

	if (!HEAP_ALLOC(psFPNodeHeap, (void*) &psNode))
	{
		return NULL;
	}

	psNode->x = (SWORD)x;
	psNode->y = (SWORD)y;
	psNode->dist = (SWORD)dist;
	psNode->psRoute = psRoute;
	psNode->type = NT_OPEN;

	return psNode;
}


// Compare a location with those in the hash table and return a new one
// if necessary
/*FP_NODE *fpathHashCompare(SDWORD x, SDWORD y, SDWORD dist)
{
	FP_NODE	*psNew, *psPrev, *psFound;
	SDWORD	index;

	index = fpathHashFunc(x,y);
	if (apsClosed[index] &&
		apsClosed[index]->x == x && apsClosed[index]->y == y)
	{
		psFound = apsClosed[index];
		apsClosed[index] = apsClosed[index]->psNext;
	}
	else
	{
		psPrev = NULL;
		for(psFound = apsClosed[index]; psFound; psFound = psFound->psNext)
		{
			if (psFound->x == x && psFound->y == y)
			{
				break;
			}
			psPrev = psFound;
		}
		if (psFound)
		{
			psPrev->psNext = psFound->psNext;
		}
	}

	return psNew;
}*/

// Variables for the callback
static SDWORD	finalX,finalY, vectorX,vectorY;
static BOOL		obstruction;

// The visibility ray callback
BOOL fpathVisCallback(SDWORD x, SDWORD y, SDWORD dist)
{
	SDWORD	vx,vy;

	dist = dist;

	// See if this point is past the final point (dot product)
	vx = x - finalX;
	vy = y - finalY;
	if (vx*vectorX + vy*vectorY <=0)
	{
		return FALSE;
	}

	if (fpathBlockingTile(x>>TILE_SHIFT,y>>TILE_SHIFT))
	{
		// found an obstruction
		obstruction = TRUE;
		return FALSE;
	}

	return TRUE;
}


// Check los between two tiles
BOOL fpathTileLOS(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2)
{
	// convert to world coords
	x1 = (x1 << TILE_SHIFT) + TILE_UNITS/2;
	y1 = (y1 << TILE_SHIFT) + TILE_UNITS/2;
	x2 = (x2 << TILE_SHIFT) + TILE_UNITS/2;
	y2 = (y2 << TILE_SHIFT) + TILE_UNITS/2;

	// Initialise the callback variables
	finalX = x2;
	finalY = y2;
	vectorX = x1 - x2;
	vectorY = y1 - y2;
	obstruction = FALSE;

	rayCast(x1,y1, rayPointsToAngle(x1,y1,x2,y2),
			RAY_MAXLEN, fpathVisCallback);

	return !obstruction;
}


// Optimise the route
void fpathOptimise(FP_NODE *psRoute)
{
	FP_NODE	*psCurr, *psSearch, *psTest;
	BOOL	los;

	ASSERT( psRoute != NULL,
		"fpathOptimise: NULL route pointer" );

	psCurr = psRoute;
	do
	{
		// work down the route looking for a failed LOS
		los = TRUE;
		psSearch = psCurr->psRoute;
		while (psSearch)
		{
			psTest = psSearch->psRoute;
			if (psTest)
			{
				los = fpathTileLOS(psCurr->x,psCurr->y, psTest->x,psTest->y);
			}
			if (!los)
			{
				break;
			}
			psSearch = psTest;
		}

		// store the previous successful point
		psCurr->psRoute = psSearch;
		psCurr = psSearch;
	} while (psCurr);
}

#if OPEN_LIST == 0 || OPEN_LIST == 1
// A* findpath
BOOL fpathAStarRoute(ASTAR_ROUTE *psRoutePoints,
					 SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy)
{
	FP_NODE		*psCurr, *psNew, *psRoute;
	FP_NODE		*psCFound, *psOFound;
	SDWORD		tileSX,tileSY,tileFX,tileFY;
	SDWORD		dir, x,y, currDist;
	SDWORD		index;
	SDWORD		numPoints;

/*	firstHit=0;
	secondHit=0;
	astarInner=0;
	astarOuter=0;
	astarRemove=0;*/

	tileSX = sx >> TILE_SHIFT; tileSY = sy >> TILE_SHIFT;
	tileFX = fx >> TILE_SHIFT; tileFY = fy >> TILE_SHIFT;

	// Add the start point to the open list
	psCurr = fpathNewNode(tileSX,tileSY, 0, NULL);
	if (!psCurr)
	{
		goto exit_error;
	}
	psCurr->est = fpathEstimate(psCurr->x, psCurr->y, tileFX, tileFY);
	psOpen = NULL;
	fpathOpenAdd(psCurr);
	fpathHashAdd(apsOpen, psCurr);

	// search for a route
	psRoute = NULL;
	while (psOpen != NULL)
	{
		psCurr = fpathOpenGet();
		DBP0(("\nStart          : %3d,%3d (%d,%d) = %d\n",
				psCurr->x,psCurr->y, psCurr->dist, psCurr->est, psCurr->dist + psCurr->est));

		if (psCurr->x == tileFX && psCurr->y == tileFY)
		{
			// reached the target
			psRoute = psCurr;
			break;
		}

		astarOuter += 1;

		for(dir=0; dir<NUM_DIR; dir+=1)
		{
			if (dir % 2 == 0)
			{
				currDist = psCurr->dist + 10;
			}
			else
			{
				currDist = psCurr->dist + 14;
			}

			// Try a new location
			x = psCurr->x + aDirOffset[dir].x;
			y = psCurr->y + aDirOffset[dir].y;
			if (fpathBlockingTile(x,y))
			{
				// tile is blocked, skip it
				DBP0(("blocked          : %3d, %3d\n", x,y));
				continue;
			}

			// See if this is already in the open list
			psOFound = fpathHashCondRemove(apsOpen, x,y, currDist);
			if (psOFound && psOFound->dist <= currDist)
			{
				// already in the open list by a shorter route
				DBP0(("blocked open     : %3d, %3d dist %d\n", x,y, currDist));
				continue;
			}

			// See if this is in the closed list
			psCFound = fpathHashCondRemove(apsClosed, x,y, currDist);
			ASSERT( !(psOFound && psCFound),
				"fpathAStarRoute: found point in open and closed lists" );
			if (psCFound && psCFound->dist <= currDist)
			{
				// already in the closed list by a shorter route
				DBP0(("blocked closed   : %3d, %3d dist %d\n", x,y, currDist));
				continue;
			}

			astarInner += 1;

			// Now insert the point into the appropriate list
			if (!psOFound && !psCFound)
			{
				// Not in open or closed lists - add to the open list
				psNew = fpathNewNode(x,y, currDist, psCurr);
				psNew->est = fpathEstimate(x,y, tileFX, tileFY);
				fpathOpenAdd(psNew);
				fpathHashAdd(apsOpen, psNew);
				DBP0(("new              : %3d, %3d (%d,%d) = %d\n",
					x,y, currDist, psNew->est, currDist + psNew->est));
			}
			else if (psOFound && !psCFound)
			{
				astarRemove += 1;

				// already in the open list but this is shorter
				fpathOpenRemove(psOFound);
				psOFound->dist = currDist;
				psOFound->psRoute = psCurr;
				fpathOpenAdd(psOFound);
				fpathHashAdd(apsOpen, psOFound);
				DBP0(("replace open     : %3d, %3d dist %d\n",
					x,y, currDist, psOFound->est, currDist + psOFound->est));
			}
			else if (!psOFound && psCFound)
			{
				// already in the closed list but this is shorter
				psCFound->dist = currDist;
				psCFound->psRoute = psCurr;
				fpathOpenAdd(psCFound);
				fpathHashAdd(apsOpen, psCFound);
				DBP0(("replace closed   : %3d, %3d dist %d\n",
					x,y, currDist, psCFound->est, currDist + psCFound->est));
			}
			else
			{
				ASSERT( FALSE,"fpathAStarRoute: the open and closed lists are f***ed" );
			}
		}

//		ASSERT( fpathValidateTree(psOpen),
//			"fpathAStarRoute: Invalid open tree" );

		// add the current point to the closed nodes
		fpathHashRemove(apsOpen, psCurr->x, psCurr->y);
		fpathHashAdd(apsClosed, psCurr);
		DBP0(("HashAdd - closed : %3d,%3d (%d,%d) = %d\n",
				psCurr->x,psCurr->y, psCurr->dist, psCurr->est, psCurr->dist+psCurr->est));
	}

	// optimise the route if one was found
	if (psRoute)
	{
		fpathOptimise(psRoute);

		numPoints = 0;
		for(psCurr = psRoute; psCurr; psCurr=psCurr->psRoute)
		{
			numPoints ++;
		}
		index = numPoints -1;
		if (numPoints >= TRAVELSIZE)
		{
			numPoints = TRAVELSIZE -1;
		}
		psCurr = psRoute;
		while (psCurr)
		{
			if (index < numPoints)
			{
				psMoveCntl->MovementList[index].XCoordinate =
					(psCurr->x << TILE_SHIFT) + TILE_UNITS/2;
				psMoveCntl->MovementList[index].YCoordinate =
					(psCurr->y << TILE_SHIFT) + TILE_UNITS/2;
			}
			index -= 1;
			psCurr = psCurr->psRoute;
		}
		psMoveCntl->MovementList[numPoints].XCoordinate = -1;
		psMoveCntl->MovementList[numPoints].YCoordinate = -1;
	}
	else
	{
		psMoveCntl->MovementList[0].XCoordinate = -1;
		psMoveCntl->MovementList[0].YCoordinate = -1;
	}

	fpathHashReset();
//	fpathOpenReset();


	return TRUE;

exit_error:
	fpathHashReset();
//	fpathOpenReset();
	return FALSE;
}

#elif OPEN_LIST == 2

// A* findpath
SDWORD fpathAStarRoute(SDWORD routeMode, ASTAR_ROUTE *psRoutePoints,
					 SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy)
{
 	FP_NODE		*psFound, *psCurr, *psNew, *psParent, *psNext;
static 	FP_NODE		*psNearest, *psRoute;
	SDWORD		tileSX,tileSY,tileFX,tileFY;
	SDWORD		dir, x,y, currDist;
	SDWORD		index;
	SDWORD		retval;

/*	firstHit=0;
	secondHit=0;
	astarInner=0;
	astarOuter=0;
	astarRemove=0;*/

//	DBPRINTF(("Astar start\n");


	tileSX = sx >> TILE_SHIFT; tileSY = sy >> TILE_SHIFT;
	tileFX = fx >> TILE_SHIFT; tileFY = fy >> TILE_SHIFT;

	if (routeMode == ASR_NEWROUTE)
	{
		fpathHashReset();

		// Add the start point to the open list
		psCurr = fpathNewNode(tileSX,tileSY, 0, NULL);
		if (!psCurr)
		{
			goto exit_error;
		}
		psCurr->est = (SWORD)fpathEstimate(psCurr->x, psCurr->y, tileFX, tileFY);
		psOpen = NULL;
		fpathOpenAdd(psCurr);
		fpathHashAdd(apsNodes, psCurr);
		psRoute = NULL;
		psNearest = NULL;
	}

	// search for a route
	while (psOpen != NULL)
	{
		if (astarInner > FPATH_LOOP_LIMIT)
		{
			return ASR_PARTIAL;
		}

		psCurr = fpathOpenGet();
		DBP0(("\nStart          : %3d,%3d (%d,%d) = %d\n",
				psCurr->x,psCurr->y, psCurr->dist, psCurr->est, psCurr->dist + psCurr->est));

		if (psCurr->x == tileFX && psCurr->y == tileFY)
		{
			// reached the target
			psRoute = psCurr;
			break;
		}

		// note the nearest node to the target so far
		if (psNearest == NULL || psCurr->est < psNearest->est)
		{
			psNearest = psCurr;
		}

		astarOuter += 1;

		for(dir=0; dir<NUM_DIR; dir+=1)
		{
			if (dir % 2 == 0)
			{
				currDist = psCurr->dist + 10;
			}
			else
			{
				currDist = psCurr->dist + 14;
			}

			// Try a new location
			x = psCurr->x + aDirOffset[dir].x;
			y = psCurr->y + aDirOffset[dir].y;


			// See if the node has already been visited
			psFound = fpathHashPresent(apsNodes, x,y);
			if (psFound && psFound->dist <= currDist)
			{
				// already visited node by a shorter route
				DBP0(("blocked (%d)     : %3d, %3d dist %d\n",
					psFound->type, x,y, currDist));
				continue;
			}

			// If the tile hasn't been visited see if it is a blocking tile
			if (!psFound && fpathBlockingTile(x,y))
			{
				// tile is blocked, skip it
				DBP0(("blocked          : %3d, %3d\n", x,y));
				continue;
			}

			astarInner += 1;

			// Now insert the point into the appropriate list
			if (!psFound)
			{
				// Not in open or closed lists - add to the open list
				psNew = fpathNewNode(x,y, currDist, psCurr);
				if (psNew)
				{
					psNew->est = (SWORD)fpathEstimate(x,y, tileFX, tileFY);
					fpathOpenAdd(psNew);
					fpathHashAdd(apsNodes, psNew);
					DBP0(("new              : %3d, %3d (%d,%d) = %d\n",
						x,y, currDist, psNew->est, currDist + psNew->est));
				}
			}
			else if (psFound->type == NT_OPEN)
			{
				astarRemove += 1;

				// already in the open list but this is shorter
				psFound->dist = (SWORD)currDist;
				psFound->psRoute = psCurr;
				DBP0(("replace open     : %3d, %3d dist %d\n",
					x,y, currDist, psFound->est, currDist + psFound->est));
			}
			else if (psFound->type == NT_CLOSED)
			{
				// already in the closed list but this is shorter
				psFound->type = NT_OPEN;
				psFound->dist = (SWORD)currDist;
				psFound->psRoute = psCurr;
				fpathOpenAdd(psFound);
				DBP0(("replace closed   : %3d, %3d dist %d\n",
					x,y, currDist, psFound->est, currDist + psFound->est));
			}
			else
			{
				ASSERT( FALSE,"fpathAStarRoute: the open and closed lists are f***ed" );
			}
		}

//		ASSERT( fpathValidateTree(psOpen),
//			"fpathAStarRoute: Invalid open tree" );

		// add the current point to the closed nodes
//		fpathHashRemove(apsOpen, psCurr->x, psCurr->y);
//		fpathHashAdd(apsClosed, psCurr);
		psCurr->type = NT_CLOSED;
		DBP0(("HashAdd - closed : %3d,%3d (%d,%d) = %d\n",
				psCurr->x,psCurr->y, psCurr->dist, psCurr->est, psCurr->dist+psCurr->est));
	}


//	DBPRINTF(("Astar fin astarOuter=%d astarInner=%d\n",astarOuter,astarInner);


	retval = ASR_OK;

	// return the nearest route if no actual route was found
	if (!psRoute && psNearest)
//		!((psNearest->x == tileSX) && (psNearest->y == tileSY)))
	{
		psRoute = psNearest;
		fx = (psNearest->x << TILE_SHIFT) + TILE_UNITS/2;
		fy = (psNearest->y << TILE_SHIFT) + TILE_UNITS/2;
		retval = ASR_NEAREST;
	}

	// optimise the route if one was found
	if (psRoute)
	{
		fpathOptimise(psRoute);

		// get the route in the correct order
		//	If as I suspect this is to reverse the list, then it's my suspicion that
		//	we could route from destination to source as opposed to source to
		//	destination. We could then save the reversal. to risky to try now...Alex M
		psParent = NULL;
		for(psCurr=psRoute; psCurr; psCurr=psNext)
		{
			psNext = psCurr->psRoute;
			psCurr->psRoute = psParent;
			psParent = psCurr;
		}
		psRoute = psParent;

		psCurr = psRoute;
		index = psRoutePoints->numPoints;
		while (psCurr && (index < ASTAR_MAXROUTE))
		{
			psRoutePoints->asPos[index].x =	psCurr->x;
			psRoutePoints->asPos[index].y =	psCurr->y;
			index += 1;
			psCurr = psCurr->psRoute;
		}
		psRoutePoints->numPoints = index;
		psRoutePoints->finalX = psRoutePoints->asPos[index-1].x;
		psRoutePoints->finalY = psRoutePoints->asPos[index-1].y;
	}
	else
	{
		retval = ASR_FAILED;
	}

	fpathHashReset();
//	fpathOpenReset();

	return retval;


exit_error:
	fpathHashReset();
//	fpathOpenReset();
	return ASR_FAILED;
}


#endif





