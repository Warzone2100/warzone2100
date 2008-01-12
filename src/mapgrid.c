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
 * MapGrid.c
 *
 * Functions for storing objects in a grid over the map.
 * The objects are stored in every grid over which they might
 * have some influence.
 *
 */
#include "lib/framework/frame.h"
#include "objects.h"
#include "map.h"

#include "mapgrid.h"

// The number of world units per grid
#define GRID_UNITS	(GRID_SIZE * TILE_UNITS)

static UDWORD	gridWidth, gridHeight;

// The map grid
static GRID_ARRAY	*apsMapGrid[GRID_MAXAREA];
#define GridIndex(a,b) (((b)*gridWidth) + (a))

// which grid to garbage collect on next
static SDWORD		garbageX, garbageY;

// the current state of the iterator
static GRID_ARRAY	*psIterateGrid;
static SDWORD		iterateEntry;

// what to do when calculating the coverage of an object
typedef enum _coverage_mode
{
	GRID_ADDOBJECT,
	GRID_REMOVEOBJECT,
} COVERAGE_MODE;

// Function prototypes
static BOOL	gridIntersect(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2,
				   SDWORD cx,SDWORD cy, SDWORD Rad);
static void	gridAddArrayObject(SDWORD x, SDWORD y, BASE_OBJECT *psObj);
static void	gridRemoveArrayObject(SDWORD x, SDWORD y, BASE_OBJECT *psObj);
static void	gridCompactArray(SDWORD x, SDWORD y);
static SDWORD	gridObjRange(BASE_OBJECT *psObj);

// initialise the grid system
BOOL gridInitialise(void)
{
	memset(apsMapGrid, 0, sizeof(GRID_ARRAY *) * GRID_MAXAREA);

	garbageX = 0;
	garbageY = 0;

	psIterateGrid = NULL;
	iterateEntry = 0;

	return TRUE;
}

//clear the grid of everything on it
void gridClear(void)
{
	GRID_ARRAY	*psCurr, *psNext;
	unsigned int	x, y;

	debug( LOG_NEVER, "gridClear %d %d\n", gridWidth, gridHeight );
	for(x = 0; x < gridWidth; ++x)
	{
		for(y = 0; y < gridHeight; ++y)
		{
			for(psCurr = apsMapGrid[GridIndex(x,y)]; psCurr != NULL; psCurr = psNext)
			{
				psNext = psCurr->psNext;
				free(psCurr);
			}
			apsMapGrid[GridIndex(x,y)] = NULL;
		}
	}
}

// reset the grid system
void gridReset(void)
{
	STRUCTURE	*psStruct;
	DROID		*psDroid;
	FEATURE		*psFeature;
	UBYTE		inc;

	// Setup the grid dimensions.
	gridWidth = (mapWidth+GRID_SIZE-1) / GRID_SIZE;
	gridHeight = (mapHeight+GRID_SIZE-1) / GRID_SIZE;

	gridClear();

	//put all the existing objects into the grid
	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		for (psDroid = apsDroidLists[inc]; psDroid != NULL; psDroid =
			psDroid->psNext)
		{
			gridAddObject((BASE_OBJECT *)psDroid);
		}
		for (psStruct = apsStructLists[inc]; psStruct != NULL; psStruct =
			psStruct->psNext)
		{
			gridAddObject((BASE_OBJECT *)psStruct);
		}
		for (psFeature = apsFeatureLists[inc]; psFeature != NULL; psFeature =
			psFeature->psNext)
		{
			gridAddObject((BASE_OBJECT *)psFeature);
		}
	}
}


// shutdown the grid system
void gridShutDown(void)
{
	gridClear();
}


// find the grid's that are covered by the object and either
// add or remove the object
static void gridCalcCoverage(BASE_OBJECT *psObj, SDWORD objx, SDWORD objy, COVERAGE_MODE mode)
{
	SDWORD	range, x,y, minx,maxx, miny,maxy;

	range = gridObjRange(psObj);

	// calculate the range of grids that could be covered by the object
	objx = (objx & ~TILE_MASK) + TILE_UNITS/2;
	objy = (objy & ~TILE_MASK) + TILE_UNITS/2;
	minx = objx - range;
	maxx = objx + range;
	miny = objy - range;
	maxy = objy + range;

	minx = map_coord(minx) / GRID_SIZE;
	maxx = map_coord(maxx) / GRID_SIZE;
	miny = map_coord(miny) / GRID_SIZE;
	maxy = map_coord(maxy) / GRID_SIZE;

	// see which ones are covered by the object
	for (x=minx; x<=maxx; x++)
	{
		if ( (x >= 0) && (x < gridWidth) )
		{
			for(y=miny; y<=maxy; y++)
			{
				if ( (y >= 0) && (y < gridHeight) &&
					 gridIntersect( x * GRID_UNITS, y * GRID_UNITS,
									(x+1) * GRID_UNITS, (y+1) * GRID_UNITS,
									objx, objy, range ) )
				{
					switch (mode)
					{
					case GRID_ADDOBJECT:
						gridAddArrayObject(x,y, psObj);
						break;
					case GRID_REMOVEOBJECT:
						gridRemoveArrayObject(x,y, psObj);
						break;
					}
				}
			}
		}
	}
}


// add an object to the grid system
void gridAddObject(BASE_OBJECT *psObj)
{
	ASSERT(!isDead(psObj), "Added a dead object to the map grid!");
    gridCalcCoverage(psObj, (SDWORD)psObj->pos.x, (SDWORD)psObj->pos.y, GRID_ADDOBJECT);
}

// move an object within the grid
// oldX,oldY are the old position of the object in world coords
void gridMoveObject(BASE_OBJECT *psObj, SDWORD oldX, SDWORD oldY)
{
	if (map_coord(psObj->pos.x) == map_coord(oldX)
	 && map_coord(psObj->pos.y) == map_coord(oldY))
	{
		// havn't changed the tile the object is on, don't bother updating
		return;
	}

	gridCalcCoverage(psObj, oldX,oldY, GRID_REMOVEOBJECT);
	gridCalcCoverage(psObj, (SDWORD)psObj->pos.x, (SDWORD)psObj->pos.y, GRID_ADDOBJECT);
}


// remove an object from the grid system
void gridRemoveObject(BASE_OBJECT *psObj)
{
    gridCalcCoverage(psObj, (SDWORD)psObj->pos.x, (SDWORD)psObj->pos.y, GRID_REMOVEOBJECT);

#if defined(DEBUG)
	{
		GRID_ARRAY		*psCurr;
		unsigned int		i,x,y;

		for (x = 0; x < gridWidth; x++)
		{
			for(y = 0; y < gridHeight; y++)
			{
				for (psCurr = apsMapGrid[GridIndex(x,y)]; psCurr; psCurr = psCurr->psNext)
				{
					for (i = 0; i < MAX_GRID_ARRAY_CHUNK; i++)
					{
						if (psCurr->apsObjects[i] == psObj)
						{
							ASSERT( FALSE,"gridRemoveObject: grid out of sync" );
							psCurr->apsObjects[i] = NULL;
						}
					}
				}
			}
		}
	}
#endif
}


// initialise the grid system to start iterating through units that
// could affect a location (x,y in world coords)
void gridStartIterate(SDWORD x, SDWORD y)
{
	ASSERT( (x >= 0) && (x < gridWidth*GRID_UNITS) &&
			 (y >= 0) && (y < gridHeight*GRID_UNITS),
		"gridStartIterate: coords off grid" );

	x = x / GRID_UNITS;
	y = y / GRID_UNITS;

	psIterateGrid = apsMapGrid[GridIndex(x,y)];
	iterateEntry = 0;
}

// get the next object that could affect a location,
// should only be called after gridStartIterate
BASE_OBJECT *gridIterate(void)
{
	BASE_OBJECT		*psRet;

	while (psIterateGrid != NULL)
	{
		if (psIterateGrid->apsObjects[iterateEntry] != NULL)
		{
			break;
		}

		iterateEntry += 1;
		if (iterateEntry >= MAX_GRID_ARRAY_CHUNK)
		{
			psIterateGrid = psIterateGrid->psNext;
			iterateEntry = 0;
		}
	}

	if (psIterateGrid)
	{
		psRet = psIterateGrid->apsObjects[iterateEntry];
		iterateEntry += 1;
		if (iterateEntry >= MAX_GRID_ARRAY_CHUNK)
		{
			psIterateGrid = psIterateGrid->psNext;
			iterateEntry = 0;
		}
	}
	else
	{
		psRet = NULL;
	}

	return psRet;
}


// compact some of the grid arrays
void gridGarbageCollect(void)
{
	gridCompactArray(garbageX,garbageY);

	++garbageX;
	if (garbageX >= gridWidth)
	{
		garbageX = 0;
		++garbageY;

		if (garbageY >= gridHeight)
		{
			garbageX = 0;
			garbageY = 0;
		}
	}

//#ifdef DEBUG
#if 0
	// integrity check the array
	{
		GRID_ARRAY	*psCurr, *psCheck;
		SDWORD		curr, check;
		BASE_OBJECT	*psObj;

		check = 0;
		psCheck = apsMapGrid[GridIndex(garbageX,garbageY)];
		while (psCheck != NULL)
		{
			psObj = psCheck->apsObjects[check];
			if (psObj != NULL)
			{
				// see if there is a duplicate element in the array
				curr = 0;
				psCurr = apsMapGrid[GridIndex(garbageX,garbageY)];
				while ( psCurr != NULL )
				{
					if ( !((psCurr == psCheck) && (curr == check)) &&
						(psCurr->apsObjects[curr] == psObj) )
					{
						ASSERT( FALSE, "mapGrid integrity check failed" );

						psCurr->apsObjects[curr] = NULL;
					}

					curr += 1;
					if (curr >= MAX_GRID_ARRAY_CHUNK)
					{
						psCurr=psCurr->psNext;
						curr = 0;
					}
				}
			}

			check += 1;
			if (check >= MAX_GRID_ARRAY_CHUNK)
			{
				psCheck = psCheck->psNext;
				check = 0;
			}
		}
	}
#endif
}


// add an object to a grid array
static void gridAddArrayObject(SDWORD x, SDWORD y, BASE_OBJECT *psObj)
{
	GRID_ARRAY		*psPrev, *psCurr, *psNew;
	SDWORD			i;

	// see if there is an empty slot in the currently allocated array
	psPrev = NULL;
	for (psCurr = apsMapGrid[GridIndex(x,y)]; psCurr; psCurr=psCurr->psNext)
	{
		for(i=0; i<MAX_GRID_ARRAY_CHUNK; i++)
		{
			if (psCurr->apsObjects[i] == NULL)
			{
				// found an empty slot
				psCurr->apsObjects[i] = psObj;
				return;
			}
		}
		psPrev = psCurr;
	}

	// allocate a new array chunk
	psNew = malloc(sizeof(GRID_ARRAY));
	if (psNew == NULL)
	{
		debug(LOG_ERROR, "gridAddArrayObject: Out of memory");
		return;
	}

	// store the object
	memset(psNew, 0, sizeof(GRID_ARRAY));
	psNew->apsObjects[0] = psObj;

	// add the chunk to the end of the list
	if (psPrev == NULL)
	{
		apsMapGrid[GridIndex(x,y)] = psNew;
	}
	else
	{
		psPrev->psNext = psNew;
	}
}


// remove an object from a grid array
static void gridRemoveArrayObject(SDWORD x, SDWORD y, BASE_OBJECT *psObj)
{
	GRID_ARRAY		*psCurr;
	SDWORD			i;

	for (psCurr = apsMapGrid[GridIndex(x,y)]; psCurr; psCurr = psCurr->psNext)
	{
		for (i=0; i<MAX_GRID_ARRAY_CHUNK; i++)
		{
			if (psCurr->apsObjects[i] == psObj)
			{
				psCurr->apsObjects[i] = NULL;
				return;
			}
		}
	}
}


// Compact a grid array to remove any NULL objects
static void gridCompactArray(SDWORD x, SDWORD y)
{
	GRID_ARRAY		*psDone, *psMove, *psPrev, *psNext;
	SDWORD			done, move;

	psDone = psMove = apsMapGrid[GridIndex(x,y)];
	done = move = 0;

	// move every entry down in the array
	psPrev = NULL;
	while (psMove != NULL)
	{
		if (psMove->apsObjects[move] != NULL)
		{
			psDone->apsObjects[done] = psMove->apsObjects[move];

			done += 1;
			if (done >= MAX_GRID_ARRAY_CHUNK)
			{
				psPrev = psDone;
				psDone = psDone->psNext;
				done = 0;
			}
		}

		move += 1;
		if (move >= MAX_GRID_ARRAY_CHUNK)
		{
			psMove = psMove->psNext;
			move = 0;
		}
	}

	// if the compacted array finishes half way through, NULL the rest of it
	if ( (psDone != NULL) && (done != 0) )
	{
		memset(&psDone->apsObjects[done], 0, sizeof(BASE_OBJECT *) * (MAX_GRID_ARRAY_CHUNK - done) );

		psPrev = psDone;
		psDone = psDone->psNext;
	}

	// now release any unused chunks
	if (psPrev == NULL)
	{
		apsMapGrid[GridIndex(x,y)] = NULL;
	}
	else
	{
		psPrev->psNext = NULL;
	}
	while (psDone)
	{
		psNext = psDone->psNext;
		free(psDone);
		psDone = psNext;
	}
}


// Display all the grid's an object is a member of
void gridDisplayCoverage(BASE_OBJECT *psObj)
{
#ifdef DEBUG
	{
		unsigned int	x, y, i;
		GRID_ARRAY	*psCurr;

		debug( LOG_NEVER, "Grid coverage for object %d (%d,%d) - range %d\n", psObj->id, psObj->pos.x, psObj->pos.y, gridObjRange(psObj) );
		for (x = 0; x < gridWidth; x++)
		{
			for(y = 0; y < gridHeight; y++)
			{
				psCurr = apsMapGrid[GridIndex(x, y)];
				i = 0;
				while (psCurr != NULL)
				{
					if (psCurr->apsObjects[i] == psObj)
					{
						debug( LOG_NEVER, "    %d,%d  [ %d,%d -> %d,%d ]\n", x, y, x*GRID_UNITS, y*GRID_UNITS, (x+1)*GRID_UNITS, (y+1)*GRID_UNITS );
					}

					++i;
					if (i >= MAX_GRID_ARRAY_CHUNK)
					{
						psCurr = psCurr->psNext;
						i = 0;
					}
				}
			}
		}
	}
#endif
}


// Fast circle rectangle intersection, taken from Graphics Gems I, p51
// by Clifford A Shaffer
/* Return TRUE iff rectangle R intersects circle with centerpoint C and
   radius Rad. */
static BOOL gridIntersect(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2,
				   SDWORD cx,SDWORD cy, SDWORD Rad)
{
	SDWORD	Rad2;

	Rad2 = Rad * Rad;

	/* Translate coordinates, placing C at the origin. */
	x1 -= cx;  y1 -= cy;
	x2 -= cx;  y2 -= cy;

	if (x2 < 0) 			/* R to left of circle center */
	{
   		if (y2 < 0) 		/* R in lower left corner */
		{
     		return ((x2 * x2 + y2 * y2) < Rad2);
		}
   		else if (y1 > 0) 	/* R in upper left corner */
		{
   			return ((x2 * x2 + y1 * y1) < Rad2);
		}
   		else 					/* R due West of circle */
		{
   			return(abs(x2) < Rad);
		}
	}
	else if (x1 > 0)  	/* R to right of circle center */
	{
   		if (y2 < 0) 	/* R in lower right corner */
		{
     		return ((x1 * x1 + y2 * y2) < Rad2);
		}
   		else if (y1 > 0)  	/* R in upper right corner */
		{
   			return ((x1 * x1 + y1 * y1) < Rad2);
		}
   		else 				/* R due East of circle */
		{
     		return (x1 < Rad);
		}
	}
 	else				/* R on circle vertical centerline */
	{
		if (y2 < 0) 	/* R due South of circle */
		{
     		return (abs(y2) < Rad);
		}
   		else if (y1 > 0)  	/* R due North of circle */
		{
     		return (y1 < Rad);
		}
   		else 				/* R contains circle centerpoint */
		{
     		return(TRUE);
		}
	}
}


// Get the range of effect of an object
static SDWORD gridObjRange(BASE_OBJECT *psObj)
{
/*	SDWORD	range;

	switch (psObj->type)
	{
	case OBJ_DROID:
		range = ((DROID *)psObj)->sensorRange;
		break;
	case OBJ_STRUCTURE:
		range = ((STRUCTURE *)psObj)->sensorRange;
		if (structCBSensor((STRUCTURE *)psObj) ||
			structVTOLCBSensor((STRUCTURE *)psObj))
		{
			range = MAP_MAXWIDTH > MAP_MAXHEIGHT ?
				MAP_MAXWIDTH*TILE_UNITS : MAP_MAXHEIGHT*TILE_UNITS;
		}
		break;
	case OBJ_FEATURE:
		range = 0;
		break;
	default:
		range = 0;
		break;
	}

	if (range < NAYBOR_RANGE)
	{
		range = NAYBOR_RANGE;
	}

	return range;*/

	return (TILE_UNITS * 20);
}
