/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
/*
 * Formation.c
 *
 * Control units moving in formation.
 *
 */

#include <string.h>

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

// The list of allocated formations
static FORMATION	*psFormationList;

static SDWORD formationObjRadius(BASE_OBJECT *psObj);

// Initialise the formation system
BOOL formationInitialise(void)
{
	psFormationList = NULL;

	return true;
}

// Shutdown the formation system
void formationShutDown(void)
{
	FORMATION	*psNext;

	while (psFormationList)
	{
		debug( LOG_NEVER, "formation with %d units still attached\n", psFormationList->refCount );
		psNext = psFormationList->psNext;
		free(psFormationList);
		psFormationList = psNext;
	}
}

// Create a new formation
BOOL formationNew(FORMATION **ppsFormation, FORMATION_TYPE type,
					SDWORD x, SDWORD y, SDWORD dir)
{
	SDWORD		i;
	FORMATION	*psNew = malloc(sizeof(FORMATION));

	// get a heap structure
	if (psNew == NULL)
	{
		debug(LOG_ERROR, "formationNew: Out of memory");
		return false;
	}

// 	debug( LOG_NEVER, "formationNew: type %d, at (%d,%d), dir %d\n", type, x, y, dir );

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
		ASSERT( false,"fmNewFormation: unknown formation type" );
		break;
	}

	psNew->psNext = psFormationList;
	psFormationList = psNew;

	*ppsFormation = psNew;

	return true;
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
static void formationUpdateSpeed( FORMATION *psFormation, BASE_OBJECT *psNew )
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

	ASSERT( psFormation != NULL,
		"formationJoin: invalid formation" );

// 	debug( LOG_NEVER, "formationJoin: %p, obj %d\n", psFormation, psObj->id );

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

	ASSERT( psFormation != NULL,
		"formationLeave: invalid formation" );
	ASSERT( psFormation->refCount > 0,
		"formationLeave: refcount is zero" );

// 	debug( LOG_NEVER, "formationLeave: %p, obj %d\n", psFormation, psObj->id );

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
		free(psFormation);
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
				ASSERT( false,
					"formationReset: unknown unit type" );
				break;
			}
		}
	}
}

// calculate the coordinates of a position on a line
static void formationCalcPos(FORMATION *psFormation, SDWORD line, SDWORD dist,
					  SDWORD *pX, SDWORD *pY)
{
	const int rank = dist / psFormation->size;

	// calculate the offset of the line based on the rank
	int dir = adjustDirection(psFormation->dir, 180);
	const int xoffset = (int)(trigSin(dir) * (float)(psFormation->rankDist * rank))
			+ psFormation->asLines[line].xoffset;
	const int yoffset = (int)(trigCos(dir) * (float)(psFormation->rankDist * rank))
			+ psFormation->asLines[line].yoffset;

	// calculate the position of the gap
	dir = psFormation->asLines[line].dir;
	dist -= psFormation->size * rank;
	*pX = (int)(trigSin(dir) * (float)dist)
			+ xoffset + psFormation->x;
	*pY = (int)(trigCos(dir) * (float)dist)
			+ yoffset + psFormation->y;
}


// assign a unit to a free spot in the formation
static void formationFindFree(FORMATION *psFormation, BASE_OBJECT *psObj,
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
		ASSERT( false,
			"formationFindFree: no members left to allocate" );
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
		found = false;
		while (!found && rank <= MAX_RANK)
		{
			ASSERT( unit < F_MAXMEMBERS, "formationFindFree: unit out of range" );

			if (unit != -1)
			{
				// See if the object can fit in the gap between this and the last unit
				radius = formationObjRadius(asMembers[unit].psObj);
				if (objRadius*2 <= asMembers[unit].dist - radius - currDist)
				{
					found = true;
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
				found = true;
			}

			if (found)
			{
				// calculate the position on the line
				formationCalcPos(psFormation, line, currDist+objRadius, &x,&y);
				if (fpathBlockingTile(map_coord(x), map_coord(y)))
				{
					// blocked, try the next rank
					found = false;
					currDist = psFormation->size * rank;
					rank += 1;
				}
			}
		}

		// see if this gap is closer to the unit than the previous one
		xdiff = x - (SDWORD)psObj->pos.x;
		ydiff = y - (SDWORD)psObj->pos.y;
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
			xdiff = (SDWORD)psObj->pos.x - psFormation->x;
			ydiff = (SDWORD)psObj->pos.y - psFormation->y;
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

// get a target position to move into a formation
BOOL formationGetPos( FORMATION *psFormation, BASE_OBJECT *psObj,
						SDWORD *pX, SDWORD *pY, BOOL bCheckLOS )
{
	SDWORD		xdiff,ydiff,distSq;//,rangeSq;
	SDWORD		member, x,y;
	F_MEMBER	*asMembers;

	ASSERT( psFormation != NULL,
		"formationGetPos: invalid formation pointer" );

/*	if (psFormation->refCount == 1)
	{
		// nothing else in the formation so don't do anything
		return false;
	}*/

	// see if the unit is close enough to join the formation
	xdiff = (SDWORD)psFormation->x - (SDWORD)psObj->pos.x;
	ydiff = (SDWORD)psFormation->y - (SDWORD)psObj->pos.y;
	distSq = xdiff*xdiff + ydiff*ydiff;
//	rangeSq = 3*psFormation->size/2;
//	rangeSq = rangeSq*rangeSq;
//	if (distSq > F_JOINRANGE*F_JOINRANGE)
//	{
//		return false;
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
		debug( LOG_NEVER, "formation new member : (%d, %d)n",
					x,y));*/
	}
	else
	{
		return false;
	}

	// check the unit can get to the formation position
	if ( bCheckLOS && !fpathTileLOS(map_coord(psObj->pos.x), map_coord(psObj->pos.y),
									map_coord(x), map_coord(y)))
	{
		return false;
	}

	*pX = x;
	*pY = y;

	return true;
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
			return true;
		}
	}

	return false;
}

SDWORD formationObjRadius(BASE_OBJECT *psObj)
{
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
		ASSERT( false,"formationObjRadius: unknown object type" );
		radius = 0;
		break;
	}

	return radius;
}
