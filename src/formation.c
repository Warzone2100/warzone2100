/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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

// default length of a formation line
#define F_DEFLENGTH		(4*fmLtRad)

// how close to the formation a unit has to be to receive a formation point
#define F_JOINRANGE		(TILE_UNITS * 3)

// how far apart to keep the ranks in a formation
#define RANK_DIST		(2*fmLtRad)

// maximum number of ranks for a line
#define MAX_RANK		15

// the distance for finding formations
#define FIND_RANGE		(TILE_UNITS/2)

#define FORMATION_SPEED_INIT	100000L

// The list of allocated formations
static FORMATION	*psFormationList;

static SDWORD formationObjRadius(const DROID* psDroid);

/** Initialise the formation system
 */
BOOL formationInitialise(void)
{
	psFormationList = NULL;

	return true;
}

/** Shutdown the formation system
 */
void formationShutDown(void)
{
	FORMATION	*psNext;

	while (psFormationList)
	{
		debug(LOG_NEVER, "formation with %d units still attached", psFormationList->refCount);
		psNext = psFormationList->psNext;
		free(psFormationList);
		psFormationList = psNext;
	}
}

/** Create a new formation
 */
BOOL formationNew(FORMATION **ppsFormation, FORMATION_TYPE type,
					SDWORD x, SDWORD y, uint16_t dir)
{
	SDWORD		i;
	FORMATION	*psNew = malloc(sizeof(FORMATION));

	// get a heap structure
	ASSERT_OR_RETURN(false, psNew, "Out of memory");

	// initialise it
	psNew->refCount = 0;
	psNew->size = (SWORD)F_DEFLENGTH;
	psNew->rankDist = (SWORD)RANK_DIST;
	psNew->direction = dir;
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
		psNew->asLines[0].direction = dir - DEG(110);
		psNew->asLines[0].member = -1;
		// line to the right
		psNew->asLines[1].xoffset = 0;
		psNew->asLines[1].yoffset = 0;
		psNew->asLines[1].direction = dir + DEG(110);
		psNew->asLines[1].member = -1;
		break;
	case FT_COLUMN:
		psNew->numLines = 1;
		// line to the left
		psNew->asLines[0].xoffset = 0;
		psNew->asLines[0].yoffset = 0;
		psNew->asLines[0].direction = dir + DEG(180);
		psNew->asLines[0].member = -1;
		break;
	default:
		ASSERT(false, "Unknown formation type");
		return false;
	}

	psNew->psNext = psFormationList;
	psFormationList = psNew;

	*ppsFormation = psNew;

	return true;
}


/** Try to find a formation near a location
 */
FORMATION* formationFind(int x, int y)
{
	FORMATION* psFormation;

	for (psFormation = psFormationList; psFormation; psFormation = psFormation->psNext)
	{
		// see if the position is close to the formation
		const int xdiff = psFormation->x - x;
		const int ydiff = psFormation->y - y;
		const int distSq = xdiff*xdiff + ydiff*ydiff;
		if (distSq < FIND_RANGE*FIND_RANGE)
		{
			return psFormation;
		}
	}

	return NULL;
}

/** Find formation speed.
 *  This currently means the speed of the slowest unit.
 */
static void formationUpdateSpeed(FORMATION *psFormation, const DROID* psNew)
{
	SDWORD		iUnit;
	F_MEMBER	*asMembers = psFormation->asMembers;

	if (psNew != NULL)
	{
		ASSERT(psNew->type == OBJ_DROID, "We've been passed a DROID that really isn't a DROID");
		if ( psFormation->iSpeed > psNew->baseSpeed)
		{
			psFormation->iSpeed = psNew->baseSpeed;
		}
	}
	else
	{
		psFormation->iSpeed = FORMATION_SPEED_INIT;
	}

	for(iUnit=0; iUnit<F_MAXMEMBERS; iUnit++)
	{
		if (asMembers[iUnit].psDroid
		 && psFormation->iSpeed > asMembers[iUnit].psDroid->baseSpeed)
		{
			psFormation->iSpeed = asMembers[iUnit].psDroid->baseSpeed;
		}
	}
}

/** Associate a unit with a formation.
 */
void formationJoin(FORMATION *psFormation, const DROID* psDroid)
{
	SDWORD	rankDist, size;

	ASSERT_OR_RETURN(, psFormation != NULL, "Invalid formation");

	psFormation->refCount += 1;

	rankDist = formationObjRadius(psDroid) * 2;
	if (psFormation->rankDist < rankDist)
	{
		psFormation->rankDist = (SWORD)rankDist;
	}

	size = formationObjRadius(psDroid) * 4;
	if (psFormation->size < size)
	{
		psFormation->size = (SWORD)size;
	}

	/* update formation speed */
	formationUpdateSpeed(psFormation, psDroid);
}

/** Remove a unit from a formation.
 */
void formationLeave(FORMATION *psFormation, const DROID* psDroid)
{
	SDWORD		prev, curr, unit, line;
	F_LINE		*asLines;
	F_MEMBER	*asMembers;
	FORMATION	*psCurr, *psPrev;

	ASSERT_OR_RETURN(, psFormation != NULL, "Invalid formation");
	if (!psDroid)
	{
		return;
	}
	ASSERT_OR_RETURN(, psFormation->refCount > 0, "Formation refcount is zero");

	asMembers = psFormation->asMembers;

	// see if the unit is a member
	for(unit=0; unit<F_MAXMEMBERS; unit++)
	{
		if (asMembers[unit].psDroid == psDroid)
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
		asMembers[unit].psDroid = NULL;
		psFormation->free = (SBYTE)unit;

		/* update formation speed */
		formationUpdateSpeed(psFormation, NULL);
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


/** Remove all the members from a formation and release it.
 */
void formationReset(FORMATION *psFormation)
{
	int i;

	for(i = 0; i < F_MAXMEMBERS; ++i)
	{
		if (psFormation->asMembers[i].psDroid)
		{
			formationLeave(psFormation, psFormation->asMembers[i].psDroid);
		}
	}
}

/** Calculate the coordinates of a position on a line
 */
static void formationCalcPos(DROID *psDroid, FORMATION *psFormation, SDWORD line, SDWORD dist, SDWORD *pX, SDWORD *pY)
{
	const int rank = dist / psFormation->size;

	// calculate the offset of the line based on the rank
	uint16_t dir = psFormation->direction + DEG(180);
	const int xoffset = iSinR(dir, psFormation->rankDist * rank) + psFormation->asLines[line].xoffset;
	const int yoffset = iCosR(dir, psFormation->rankDist * rank) + psFormation->asLines[line].yoffset;

	// calculate the position of the gap
	dir = psFormation->asLines[line].direction;
	dist -= psFormation->size * rank;
	*pX = iSinR(dir, dist) + xoffset + psFormation->x;
	*pY = iCosR(dir, dist) + yoffset + psFormation->y;
	//objTrace(psDroid->id, "Formation: xoff=%d, yoff=%d, dir=%d, dist=%d; formxoff=%d, formyoff=%d", xoffset, yoffset, 
	//         psFormation->asLines[line].direction, dist, psFormation->asLines[line].xoffset, psFormation->asLines[line].yoffset);
}


// assign a unit to a free spot in the formation
static void formationFindFree(FORMATION *psFormation, DROID* psDroid,
					   SDWORD	*pX, SDWORD *pY)
{
	SDWORD		line, unit, objRadius, radius;
	SDWORD		currDist, prev, rank;
	F_LINE		*asLines = psFormation->asLines;
	F_MEMBER	*asMembers = psFormation->asMembers;
	SDWORD		x,y, xdiff,ydiff, dist, objDist;
	SDWORD		chosenLine, chosenDist, chosenPrev, chosenRank;
	BOOL		found;

	ASSERT(psFormation->free != -1, "No members left to allocate");
	if (psFormation->free == -1)
	{
		*pX = psFormation->x;
		*pY = psFormation->y;
		return;
	}

	objRadius = formationObjRadius(psDroid);

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
				// See if the DROID can fit in the gap between this and the last unit
				radius = formationObjRadius(asMembers[unit].psDroid);
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
				formationCalcPos(psDroid, psFormation, line, currDist + objRadius, &x, &y);
				if (fpathBlockingTile(map_coord(x), map_coord(y), getPropulsionStats(psDroid)->propulsionType))
				{
					// blocked, try the next rank
					found = false;
					currDist = psFormation->size * rank;
					rank += 1;
				}
			}
		}

		// see if this gap is closer to the unit than the previous one
		xdiff = x - psDroid->pos.x;
		ydiff = y - psDroid->pos.y;
		dist = xdiff*xdiff + ydiff*ydiff;
//		dist += psFormation->rankDist*psFormation->rankDist * rank*rank;
		if ((dist < objDist && rank == chosenRank) || rank < chosenRank)
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
	asMembers[unit].psDroid = psDroid;

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
static void formationReorder(FORMATION *psFormation)
{
	SDWORD		numObj, i,curr,prev;
	F_MEMBER	*asMembers, asDroids[F_MAXMEMBERS];
	SDWORD		xdiff,ydiff, insert;
	SDWORD		aDist[F_MAXMEMBERS];

	// first find all the units to insert
	asMembers = psFormation->asMembers;
	numObj = 0;
	for(i=0; i<F_MAXMEMBERS; i++)
	{
		DROID* psDroid = asMembers[i].psDroid;
		if (psDroid != NULL)
		{
			asDroids[numObj].psDroid = psDroid;
			xdiff = psDroid->pos.x - psFormation->x;
			ydiff = psDroid->pos.y - psFormation->y;
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
			asDroids[i].next = -1;
			insert = i;
		}
		else
		{
			prev = -1;
			for(curr = insert; curr != -1; curr = asDroids[curr].next)
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
				asDroids[i].next = (SBYTE)insert;
				insert = i;
			}
			else
			{
				asDroids[i].next = asDroids[prev].next;
				asDroids[prev].next = (SBYTE)i;
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
		formationFindFree(psFormation, asDroids[insert].psDroid, &xdiff,&ydiff);
		insert = asDroids[insert].next;
	}
}

// get a target position to move into a formation
BOOL formationGetPos( FORMATION *psFormation, DROID* psDroid,
						SDWORD *pX, SDWORD *pY, BOOL bCheckLOS )
{
	SDWORD		xdiff,ydiff,distSq;
	SDWORD		member, x,y;
	F_MEMBER	*asMembers;

	ASSERT_OR_RETURN(false, psFormation != NULL, "Invalid formation pointer");

/*	if (psFormation->refCount == 1)
	{
		// nothing else in the formation so don't do anything
		return false;
	}*/

	// see if the unit is close enough to join the formation
	xdiff = (SDWORD)psFormation->x - (SDWORD)psDroid->pos.x;
	ydiff = (SDWORD)psFormation->y - (SDWORD)psDroid->pos.y;
	distSq = xdiff*xdiff + ydiff*ydiff;

	// see if the unit is already in the formation
	asMembers = psFormation->asMembers;
	for(member=0; member<F_MAXMEMBERS; member++)
	{
		if (asMembers[member].psDroid == psDroid)
		{
			break;
		}
	}

	if (member < F_MAXMEMBERS)
	{
		// the unit is already in the formation - return it's current position
		formationCalcPos(psDroid, psFormation, asMembers[member].line, asMembers[member].dist, &x, &y);
	}
	else if (psFormation->free != -1)
	{
		// add the new DROID to the members
		psFormation->asMembers[(int)psFormation->free].psDroid = psDroid;
		psFormation->free = psFormation->asMembers[(int)psFormation->free].next;
		formationReorder(psFormation);

		// find the DROID
		for(member=0; member<F_MAXMEMBERS; member++)
		{
			if (asMembers[member].psDroid == psDroid)
			{
				break;
			}
		}

		// calculate its position
		formationCalcPos(psDroid, psFormation, asMembers[member].line, asMembers[member].dist, &x, &y);
	}
	else
	{
		return false;
	}

	// check the unit can get to the formation position
	if (bCheckLOS && !fpathTileLOS(psDroid, Vector3i_Init(x, y, 0)))
	{
		return false;
	}

	*pX = x;
	*pY = y;

	return true;
}



// See if a unit is a member of a formation (i.e. it has a position assigned)
BOOL formationMember(FORMATION *psFormation, const DROID* psDroid)
{
	SDWORD		member;
	F_MEMBER	*asMembers;

	// see if the unit is already in the formation
	asMembers = psFormation->asMembers;
	for(member=0; member<F_MAXMEMBERS; member++)
	{
		if (asMembers[member].psDroid == psDroid)
		{
			return true;
		}
	}

	return false;
}

SDWORD formationObjRadius(const DROID* psDroid)
{
	const BODY_STATS* psBdyStats;

	ASSERT(psDroid->type == OBJ_DROID, "We got passed a DROID that isn't a DROID!");

	psBdyStats = &asBodyStats[psDroid->asBits[COMP_BODY].nStat];

	switch (psBdyStats->size)
	{
		case SIZE_LIGHT:
			return fmLtRad;

		case SIZE_MEDIUM:
			return fmMedRad;

		case SIZE_HEAVY:
			return fmHvyRad;

		case SIZE_SUPER_HEAVY:
			return 500;

		default:
			return 3 * psDroid->sDisplay.imd->radius / 2;
	}
}
