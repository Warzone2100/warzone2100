/*
 * Formation.c
 *
 * Control units moving in formation.
 *
 */


//#define DEBUG_GROUP0
#include "lib/framework/frame.h"
#include "objects.h"
#include "map.h"
#include "astar.h"
#include "fpath.h"
#include "geometry.h"

#include "formationdef.h"
#include "formation.h"


// radius for the different body sizes
static SDWORD	fmLtRad = 80, fmMedRad = 100, fmHvyRad = 110;


// heap sizes
#define F_HEAPINIT		10
#define F_HEAPEXT		5

// default length of a formation line
#define F_DEFLENGTH		(4*fmLtRad)
//(3 * TILE_UNITS / 1)

// how close to the formation a unit has to be to receive a formation point
#define F_JOINRANGE		(TILE_UNITS * 3)

// how far apart to keep the ranks in a formation
#define RANK_DIST		(2*fmLtRad)
//(3*TILE_UNITS/2)

// maximum number of ranks for a line
#define MAX_RANK		15

// the distance for finding formations
#define FIND_RANGE		(TILE_UNITS/2)

#define FORMATION_SPEED_INIT	100000L

// The heap of formations
OBJ_HEAP	*psFHeap;

// The list of allocated formations
FORMATION	*psFormationList;

SDWORD formationObjRadius(BASE_OBJECT *psObj);

// Initialise the formation system
BOOL formationInitialise(void)
{
	if (!HEAP_CREATE(&psFHeap, sizeof(FORMATION), F_HEAPINIT, F_HEAPEXT))
	{
		return FALSE;
	}

	psFormationList = NULL;

	return TRUE;
}

// Shutdown the formation system
void formationShutDown(void)
{
	FORMATION	*psNext;

	while (psFormationList)
	{
		DBPRINTF(("formation with %d units still attached\n",psFormationList->refCount));
		psNext = psFormationList->psNext;
		HEAP_FREE(psFHeap, psFormationList);
		psFormationList = psNext;
	}

	HEAP_DESTROY(psFHeap);
}

#ifdef TEST_BED
UDWORD	adjustDirection(SDWORD present, SDWORD difference)
{
SDWORD	sum;

	sum = present+difference;
	if(sum>=0 AND sum<=360)
	{
		return(UDWORD)(sum);
	}

	if (sum<0)
	{
		return(UDWORD)(360+sum);
	}

	if (sum>360)
	{
		return(UDWORD)(sum-360);
	}
}

#endif


// Create a new formation
BOOL formationNew(FORMATION **ppsFormation, FORMATION_TYPE type,
					SDWORD x, SDWORD y, SDWORD dir)
{
	FORMATION	*psNew;
	SDWORD		i;

	// get a heap structure
	if (!HEAP_ALLOC(psFHeap, (void*) &psNew))
	{
		return FALSE;
	}

	DBP0(("formationNew: type %d, at (%d,%d), dir %d\n",
				type, x,y, dir));

	// initialise it
	psNew->refCount = 0;
	psNew->size = (SWORD)F_DEFLENGTH;
	psNew->rankDist = (SWORD)RANK_DIST;
	psNew->dir = (SWORD)dir;
	psNew->x = x;
	psNew->y = y;
	psNew->free = 0;
	psNew->iSpeed = FORMATION_SPEED_INIT;
	memset(psNew->asMembers, 0, sizeof(psNew->asMembers));
	for(i=0; i<F_MAXMEMBERS; i++)
	{
		psNew->asMembers[i].next = (SBYTE)(i+1);
	}
	psNew->asMembers[F_MAXMEMBERS - 1].next = -1;

	// add the formation lines based on the formation type
	switch (type)
	{
	case FT_LINE:
		psNew->numLines = 2;
		// line to the left
		psNew->asLines[0].xoffset = 0;
		psNew->asLines[0].yoffset = 0;
		psNew->asLines[0].dir = (SWORD)adjustDirection(dir, -110);
		psNew->asLines[0].member = -1;
		// line to the right
		psNew->asLines[1].xoffset = 0;
		psNew->asLines[1].yoffset = 0;
		psNew->asLines[1].dir = (SWORD)adjustDirection(dir, 110);
		psNew->asLines[1].member = -1;
		break;
	case FT_COLUMN:
		psNew->numLines = 1;
		// line to the left
		psNew->asLines[0].xoffset = 0;
		psNew->asLines[0].yoffset = 0;
		psNew->asLines[0].dir = (SWORD)adjustDirection(dir, 180);
		psNew->asLines[0].member = -1;
		break;
	default:
		ASSERT((FALSE,"fmNewFormation: unknown formation type"));
		break;
	}

	psNew->psNext = psFormationList;
	psFormationList = psNew;

	*ppsFormation = psNew;

	return TRUE;
}


// Try and find a formation near to a location
BOOL formationFind(FORMATION **ppsFormation, SDWORD x, SDWORD y)
{
	SDWORD		xdiff,ydiff,distSq;
	FORMATION	*psCurr;

	for(psCurr = psFormationList; psCurr; psCurr=psCurr->psNext)
	{
		// see if the positioin is close to the formation
		xdiff = psCurr->x - x;
		ydiff = psCurr->y - y;
		distSq = xdiff*xdiff + ydiff*ydiff;
		if (distSq < FIND_RANGE*FIND_RANGE)
		{
			break;
		}
	}

	*ppsFormation = psCurr;

	return psCurr != NULL;
}

// find formation speed (currently speed of slowest unit)
void formationUpdateSpeed( FORMATION *psFormation, BASE_OBJECT *psNew )
{
	DROID		*psDroid;
	SDWORD		iUnit;
	F_MEMBER	*asMembers = psFormation->asMembers;

	if ( psNew != NULL && psNew->type == OBJ_DROID )
	{
		psDroid = (DROID *) psNew;
		if ( psFormation->iSpeed > psDroid->baseSpeed )
		{
			psFormation->iSpeed = psDroid->baseSpeed;
		}
	}
	else
	{
		psFormation->iSpeed = FORMATION_SPEED_INIT;
	}

	for(iUnit=0; iUnit<F_MAXMEMBERS; iUnit++)
	{
		if ( asMembers[iUnit].psObj &&
			 asMembers[iUnit].psObj->type == OBJ_DROID )
		{
			psDroid = (DROID *) asMembers[iUnit].psObj;
			if ( psFormation->iSpeed > psDroid->baseSpeed )
			{
				psFormation->iSpeed = psDroid->baseSpeed;
			}
		}
	}
}

// Associate a unit with a formation
void formationJoin(FORMATION *psFormation, BASE_OBJECT *psObj)
{
	SDWORD	rankDist, size;

	ASSERT((PTRVALID(psFormation, sizeof(FORMATION)),
		"formationJoin: invalid formation"));

	DBP0(("formationJoin: %p, obj %d\n", psFormation, psObj->id));

	psFormation->refCount += 1;

	rankDist = formationObjRadius(psObj) * 2;
	if (psFormation->rankDist < rankDist)
	{
		psFormation->rankDist = (SWORD)rankDist;
	}

	size = formationObjRadius(psObj) * 4;
	if (psFormation->size < size)
	{
		psFormation->size = (SWORD)size;
	}

	/* update formation speed */
	formationUpdateSpeed( psFormation, psObj );
}

// Remove a unit from a formation
void formationLeave(FORMATION *psFormation, BASE_OBJECT *psObj)
{
	SDWORD		prev, curr, unit, line;
	F_LINE		*asLines;
	F_MEMBER	*asMembers;
	FORMATION	*psCurr, *psPrev;

	ASSERT((PTRVALID(psFormation, sizeof(FORMATION)),
		"formationLeave: invalid formation"));
	ASSERT((psFormation->refCount > 0,
		"formationLeave: refcount is zero"));

	DBP0(("formationLeave: %p, obj %d\n", psFormation, psObj->id));

	asMembers = psFormation->asMembers;

	// see if the unit is a member
	for(unit=0; unit<F_MAXMEMBERS; unit++)
	{
		if (asMembers[unit].psObj == psObj)
		{
			break;
		}
	}

	if (unit<F_MAXMEMBERS)
	{
		// remove the member from the members list
		asLines = psFormation->asLines;
		prev = -1;
		line = asMembers[unit].line;
		curr = asLines[line].member;
		while (curr != unit)
		{
			prev = curr;
			curr = asMembers[curr].next;
		}

		if (prev == -1)
		{
			asLines[line].member = asMembers[unit].next;
		}
		else
		{
			asMembers[prev].next = asMembers[unit].next;
		}
		asMembers[unit].next = psFormation->free;
		asMembers[unit].psObj = NULL;
		psFormation->free = (SBYTE)unit;

		/* update formation speed */
		formationUpdateSpeed( psFormation, NULL );
	}

	psFormation->refCount -= 1;
	if (psFormation->refCount == 0)
	{
		if (psFormation == psFormationList)
		{
			psFormationList = psFormationList->psNext;
		}
		else
		{
			psPrev = NULL;
			for(psCurr=psFormationList; psCurr && psCurr!= psFormation; psCurr=psCurr->psNext)
			{
				psPrev = psCurr;
			}
			psPrev->psNext = psFormation->psNext;
		}
		HEAP_FREE(psFHeap, psFormation);
	}
}


// remove all the members from a formation and release it
void formationReset(FORMATION *psFormation)
{
	SDWORD			i;
	BASE_OBJECT		*psObj;

	for(i=0; i<F_MAXMEMBERS; i++)
	{
		psObj = psFormation->asMembers[i].psObj;
		if (psObj)
		{
			formationLeave(psFormation, psFormation->asMembers[i].psObj);
			switch (psObj->type)
			{
			case OBJ_DROID:
				((DROID *)psObj)->sMove.psFormation = NULL;
				break;
			default:
				ASSERT((FALSE,
					"formationReset: unknown unit type"));
				break;
			}
		}
	}
}

// calculate the coordinates of a position on a line
void formationCalcPos(FORMATION *psFormation, SDWORD line, SDWORD dist,
					  SDWORD *pX, SDWORD *pY)
{
	SDWORD	dir, xoffset,yoffset, rank;

	rank = dist / psFormation->size;

	// calculate the offset of the line based on the rank
	dir = (SDWORD)adjustDirection(psFormation->dir, 180);
	xoffset = MAKEINT(FRACTmul(trigSin(dir),MAKEFRACT(psFormation->rankDist * rank)))
			+ psFormation->asLines[line].xoffset;
	yoffset = MAKEINT(FRACTmul(trigCos(dir),MAKEFRACT(psFormation->rankDist * rank)))
			+ psFormation->asLines[line].yoffset;

	// calculate the position of the gap
	dir = psFormation->asLines[line].dir;
	dist -= psFormation->size * rank;
	*pX = MAKEINT(FRACTmul(trigSin(dir),MAKEFRACT(dist)))
			+ xoffset + psFormation->x;
	*pY = MAKEINT(FRACTmul(trigCos(dir),MAKEFRACT(dist)))
			+ yoffset + psFormation->y;
}


// assign a unit to a free spot in the formation
void formationFindFree(FORMATION *psFormation, BASE_OBJECT *psObj,
					   SDWORD	*pX, SDWORD *pY)
{
	SDWORD		line, unit, objRadius, radius;
	SDWORD		currDist, prev, rank;
	F_LINE		*asLines = psFormation->asLines;
	F_MEMBER	*asMembers = psFormation->asMembers;
	SDWORD		x,y, xdiff,ydiff, dist, objDist;
	SDWORD		chosenLine, chosenDist, chosenPrev, chosenRank;
	BOOL		found;

	if (psFormation->free == -1)
	{
		ASSERT((FALSE,
			"formationFindFree: no members left to allocate"));
		*pX = psFormation->x;
		*pY = psFormation->y;
		return;
	}

	objRadius = formationObjRadius(psObj);

	*pX = psFormation->x;
	*pY = psFormation->y;
	chosenLine = 0;
	chosenDist = 0;
	chosenPrev = -1;
	objDist = SDWORD_MAX;
	chosenRank = SDWORD_MAX;
	for(line=0; line<psFormation->numLines; line++)
	{
		// find the first free position on this line
		currDist = 0;
		rank = 1;
		prev = -1;
		unit = asLines[line].member;
		found = FALSE;
		while (!found && rank <= MAX_RANK)
		{
			ASSERT((unit < F_MAXMEMBERS, "formationFindFree: unit out of range"));

			if (unit != -1)
			{
				// See if the object can fit in the gap between this and the last unit
				radius = formationObjRadius(asMembers[unit].psObj);
				if (objRadius*2 <= asMembers[unit].dist - radius - currDist)
				{
					found = TRUE;
				}
				else
				{
					prev = unit;
					currDist = asMembers[unit].dist + radius;
					unit = asMembers[unit].next;

					// if this line is full move onto a rank behind it
					// reset the distance to the start of a rank
					if (currDist + objRadius >= psFormation->size * rank)
					{
						currDist = psFormation->size * rank;
						rank += 1;
					}
				}
			}
			else
			{
				found = TRUE;
			}

			if (found)
			{
				// calculate the position on the line
				formationCalcPos(psFormation, line, currDist+objRadius, &x,&y);
				if (fpathBlockingTile(x>>TILE_SHIFT,y>>TILE_SHIFT))
				{
					// blocked, try the next rank
					found = FALSE;
					currDist = psFormation->size * rank;
					rank += 1;
				}
			}
		}

		// see if this gap is closer to the unit than the previous one
		xdiff = x - (SDWORD)psObj->x;
		ydiff = y - (SDWORD)psObj->y;
		dist = xdiff*xdiff + ydiff*ydiff;
//		dist += psFormation->rankDist*psFormation->rankDist * rank*rank;
		if (((dist < objDist) && (rank == chosenRank)) ||
			(rank < chosenRank))
//		if (dist < objDist)
		{
			// this gap is nearer
			objDist = dist;
//			objDist += psFormation->rankDist*psFormation->rankDist * rank*rank;

			chosenLine = line;
			chosenDist = currDist + objRadius;
			chosenPrev = prev;
			chosenRank = rank;
			*pX = x;
			*pY = y;
		}
	}

	// initialise the member
	unit = psFormation->free;
	psFormation->free = asMembers[unit].next;
	asMembers[unit].line = (SBYTE)chosenLine;
	asMembers[unit].dist = (SWORD)chosenDist;
	asMembers[unit].psObj = psObj;

	// insert the unit into the list
	if (chosenPrev == -1)
	{
		asMembers[unit].next = asLines[chosenLine].member;
		asLines[chosenLine].member = (SBYTE)unit;
	}
	else
	{
		asMembers[unit].next =  asMembers[chosenPrev].next;
		asMembers[chosenPrev].next = (SBYTE)unit;
	}
}


// re-insert all the units in the formation
void formationReorder(FORMATION *psFormation)
{
	SDWORD		numObj, i,curr,prev;
	F_MEMBER	*asMembers, asObjects[F_MAXMEMBERS];
	SDWORD		xdiff,ydiff, insert;
	BASE_OBJECT	*psObj;
	SDWORD		aDist[F_MAXMEMBERS];

	// first find all the units to insert
	asMembers = psFormation->asMembers;
	numObj = 0;
	for(i=0; i<F_MAXMEMBERS; i++)
	{
		psObj = asMembers[i].psObj;
		if (psObj != NULL)
		{
			asObjects[numObj].psObj = psObj;
			xdiff = (SDWORD)psObj->x - psFormation->x;
			ydiff = (SDWORD)psObj->y - psFormation->y;
			aDist[numObj] =  xdiff*xdiff + ydiff*ydiff;
			numObj += 1;
		}
	}

	// create a list in distance order
	insert = -1;
	for(i=0; i<numObj; i++)
	{
		if (insert == -1)
		{
			// insert at the start of the list
			asObjects[i].next = -1;
			insert = i;
		}
		else
		{
			prev = -1;
			for(curr = insert; curr != -1; curr = asObjects[curr].next)
			{
				if (aDist[i] < aDist[curr])
				{
					break;
				}
				prev = curr;
			}
			if (prev == -1)
			{
				// insert at the start of the list
				asObjects[i].next = (SBYTE)insert;
				insert = i;
			}
			else
			{
				asObjects[i].next = asObjects[prev].next;
				asObjects[prev].next = (SBYTE)i;
			}
		}
	}

	// reset the free list
	psFormation->free = 0;
	memset(psFormation->asMembers, 0, sizeof(psFormation->asMembers));
	for(i=0; i<F_MAXMEMBERS; i++)
	{
		psFormation->asMembers[i].next = (SBYTE)(i+1);
	}
	psFormation->asMembers[F_MAXMEMBERS - 1].next = -1;
	for(i=0; i<psFormation->numLines; i++)
	{
		psFormation->asLines[i].member = -1;
	}

	// insert each member again
	while (insert != -1)
	{
		formationFindFree(psFormation, asObjects[insert].psObj, &xdiff,&ydiff);
		insert = asObjects[insert].next;
	}
}


// re-insert all the units in the formation
/*void formationReorder(FORMATION *psFormation)
{
	SDWORD		numObj, i,j, rank;
	F_MEMBER	*asMembers;
	F_LINE		*asLines;
	BASE_OBJECT *apsObjects[F_MAXMEMBERS];
	SDWORD		aDist[F_MAXLINES][F_MAXMEMBERS];
	struct
	{
		SDWORD	rank, dist, prev, x,y;
	}			aFreePos[F_MAXLINES];
	SDWORD		xdiff,ydiff, line,dist,obj, unit;

	// first find all the units to insert
	asMembers = psFormation->asMembers;
	asLines = psFormation->asLines;
	numObj = 0;
	memset(apsObjects, 0, sizeof(apsObjects));
	for(i=0; i<F_MAXMEMBERS; i++)
	{
		if (asMembers[i].psObj != NULL)
		{
			apsObjects[numObj] = asMembers[i].psObj;
			numObj += 1;
		}
	}

	// reset the free list
	psFormation->free = 0;
	memset(psFormation->asMembers, 0, sizeof(psFormation->asMembers));
	for(i=0; i<F_MAXMEMBERS; i++)
	{
		psFormation->asMembers[i].next = (SBYTE)(i+1);
	}
	psFormation->asMembers[F_MAXMEMBERS - 1].next = -1;
	for(i=0; i<psFormation->numLines; i++)
	{
		psFormation->asLines[i].member = -1;
	}
	psFormation->maxRank = 0;

	// initialise the free positions in the formation
	for(i=0; i< psFormation->numLines; i++)
	{
		aFreePos[i].rank = 1;
		aFreePos[i].dist = 0;
		aFreePos[i].prev = -1;
		formationCalcPos(psFormation, i, 0,
			&aFreePos[i].x, &aFreePos[i].y);
	}

	// now insert all the objects into the formation
	while (numObj > 0)
	{
		// decide which rank to use
		rank = SDWORD_MAX;
		for(i=0; i< psFormation->numLines; i++)
		{
			if (aFreePos[i].rank < rank)
			{
				rank = aFreePos[i].rank;
			}
		}
		if (psFormation->maxRank < rank)
		{
			psFormation->maxRank = (UBYTE)rank;
		}

		// now calculate the distance between each object and the free positions
		for(i=0; i<psFormation->numLines; i++)
		{
			for(j=0; j<F_MAXMEMBERS; j++)
			{
				if ((apsObjects[j] == NULL) || (aFreePos[i].rank != rank) )
				{
					aDist[i][j] = SDWORD_MAX;
				}
				else
				{
					xdiff = aFreePos[i].x - (SDWORD)apsObjects[j]->x;
					ydiff = aFreePos[i].y - (SDWORD)apsObjects[j]->y;
					aDist[i][j] = xdiff*xdiff + ydiff*ydiff;
				}
			}
		}

		// find the object nearest to a free position
		dist = SDWORD_MAX;
		for(i=0; i<psFormation->numLines; i++)
		{
			for(j=0; j<F_MAXMEMBERS; j++)
			{
				if (aDist[i][j] < dist)
				{
					dist = aDist[i][j];
					line = i;
					obj = j;
				}
			}
		}

		// put the object into the formation
		unit = psFormation->free;
		psFormation->free = asMembers[unit].next;
		asMembers[unit].line = (SBYTE)line;
		asMembers[unit].dist = (SWORD)(aFreePos[line].dist + formationObjRadius(apsObjects[obj]));
		if (asMembers[unit].dist >= psFormation->size * aFreePos[line].rank)
		{
			asMembers[unit].dist = (SWORD)(psFormation->size * aFreePos[line].rank +
						formationObjRadius(apsObjects[obj]));
		}
		asMembers[unit].psObj = apsObjects[obj];

		// insert the unit into the list
		if (aFreePos[line].prev == -1)
		{
			asMembers[unit].next = asLines[line].member;
			asLines[line].member = (SBYTE)unit;
		}
		else
		{
			asMembers[unit].next = asMembers[aFreePos[line].prev].next;
			asMembers[aFreePos[line].prev].next = (SBYTE)unit;
		}

		// update the free position
		aFreePos[line].dist = asMembers[unit].dist + formationObjRadius(apsObjects[obj]);
		aFreePos[line].prev = unit;
		if (aFreePos[line].dist >= psFormation->size * aFreePos[line].rank)
		{
			aFreePos[line].dist = psFormation->size * aFreePos[line].rank;
//				+ formationObjRadius(apsObjects[obj]);
			aFreePos[line].rank += 1;
		}
		formationCalcPos(psFormation, line, aFreePos[line].dist,
			&aFreePos[line].x, &aFreePos[line].y);

		apsObjects[obj] = NULL;
		numObj -= 1;
	}
}*/

// get a target position to move into a formation
BOOL formationGetPos( FORMATION *psFormation, BASE_OBJECT *psObj,
						SDWORD *pX, SDWORD *pY, BOOL bCheckLOS )
{
	SDWORD		xdiff,ydiff,distSq;//,rangeSq;
	SDWORD		member, x,y;
	F_MEMBER	*asMembers;

	ASSERT((PTRVALID(psFormation, sizeof(FORMATION)),
		"formationGetPos: invalid formation pointer"));

/*	if (psFormation->refCount == 1)
	{
		// nothing else in the formation so don't do anything
		return FALSE;
	}*/

	// see if the unit is close enough to join the formation
	xdiff = (SDWORD)psFormation->x - (SDWORD)psObj->x;
	ydiff = (SDWORD)psFormation->y - (SDWORD)psObj->y;
	distSq = xdiff*xdiff + ydiff*ydiff;
//	rangeSq = 3*psFormation->size/2;
//	rangeSq = rangeSq*rangeSq;
//	if (distSq > F_JOINRANGE*F_JOINRANGE)
//	{
//		return FALSE;
//	}

	// see if the unit is already in the formation
	asMembers = psFormation->asMembers;
	for(member=0; member<F_MAXMEMBERS; member++)
	{
		if (asMembers[member].psObj == psObj)
		{
			break;
		}
	}

	if (member < F_MAXMEMBERS)
	{
		// the unit is already in the formation - return it's current position
		formationCalcPos(psFormation, asMembers[member].line, asMembers[member].dist,
							&x,&y);
	}
	else if (psFormation->free != -1)
	{
		// add the new object to the members
		psFormation->asMembers[(int)psFormation->free].psObj = psObj;
		psFormation->free = psFormation->asMembers[(int)psFormation->free].next;
		formationReorder(psFormation);

		// find the object
		for(member=0; member<F_MAXMEMBERS; member++)
		{
			if (asMembers[member].psObj == psObj)
			{
				break;
			}
		}

		// calculate its position
		formationCalcPos(psFormation, asMembers[member].line, asMembers[member].dist,
							&x,&y);
/*		// a unit has just joined the formation - find a location for it
		formationFindFree(psFormation, psObj, &x,&y);
		DBP0(("formation new member : (%d, %d)\n",
					x,y));*/
	}
	else
	{
		return FALSE;
	}

	// check the unit can get to the formation position
	if ( bCheckLOS && !fpathTileLOS(psObj->x >> TILE_SHIFT,psObj->y >> TILE_SHIFT,
									x >> TILE_SHIFT,y >> TILE_SHIFT))
	{
		return FALSE;
	}

	*pX = x;
	*pY = y;

	return TRUE;
}



// See if a unit is a member of a formation (i.e. it has a position assigned)
BOOL formationMember(FORMATION *psFormation, BASE_OBJECT *psObj)
{
	SDWORD		member;
	F_MEMBER	*asMembers;

	// see if the unit is already in the formation
	asMembers = psFormation->asMembers;
	for(member=0; member<F_MAXMEMBERS; member++)
	{
		if (asMembers[member].psObj == psObj)
		{
			return TRUE;
		}
	}

	return FALSE;
}

SDWORD formationObjRadius(BASE_OBJECT *psObj)
{
#ifdef TEST_BED
	return 70;
#else

	SDWORD		radius;
	BODY_STATS	*psBdyStats;

	switch (psObj->type)
	{
	case OBJ_DROID:
		radius = 3*psObj->sDisplay.imd->radius/2;
		psBdyStats = asBodyStats + ((DROID *)psObj)->asBits[COMP_BODY].nStat;
		switch (psBdyStats->size)
		{
		default:
		case SIZE_LIGHT:
			radius = fmLtRad;
			break;
		case SIZE_MEDIUM:
			radius = fmMedRad;
			break;
		case SIZE_HEAVY:
			radius = fmHvyRad;
			break;
		case SIZE_SUPER_HEAVY:
			radius = 500;
			break;
		}
		break;
	case OBJ_STRUCTURE:
//		radius = psObj->sDisplay.imd->visRadius;
		radius = psObj->sDisplay.imd->radius/2;
		break;
	case OBJ_FEATURE:
//		radius = psObj->sDisplay.imd->visRadius;
		radius = psObj->sDisplay.imd->radius/2;
		break;
	default:
		ASSERT((FALSE,"formationObjRadius: unknown object type"));
		radius = 0;
		break;
	}

	return radius;
#endif
}

