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
/*	VISIBILTY.C Pumpkin Studios, Eidos Interactive 1996.	*/

#include <stdio.h>
#include <string.h>

#include "objects.h"
#include "map.h"
#include "loop.h"
#include "raycast.h"
#include "geometry.h"
#include "hci.h"
#include "lib/gamelib/gtime.h"
#include "mapgrid.h"
#include "cluster.h"
#include "lib/sound/audio.h"
#include "audio_id.h"
#include "scriptextern.h"
#include "structure.h"

#include "visibility.h"

#include "multiplay.h"
#include "advvis.h"




// accuracy for the height gradient
#define GRAD_MUL	10000

// decay constant for the power of sensors/ecms
#define POWER_DECAY		10

// maximum radius of an object
#define MAX_OBJRADIUS	(TILE_UNITS*2)

// maximum number of objects to test against a ray
#define MAX_RAYTEST		100
// List of objects that could intersect a ray
static BASE_OBJECT		*apsTestObjects[MAX_RAYTEST];
static SDWORD			numTestObjects = 0;

// maximum number of objects intersecting a ray
#define MAX_RAYOBJ		20
// list of objects intersecting a ray and the distance to them
static BASE_OBJECT		*apsRayObjects[MAX_RAYOBJ];
static SDWORD			aRayObjDist[MAX_RAYOBJ];
static SDWORD			numRayObjects = 0;
//static BOOL				gotRayObjects = FALSE;
// which object is being considered by the callback
static SDWORD			currObj;

// rate to change visibility level
#define VIS_LEVEL_INC		(255*2)
#define VIS_LEVEL_DEC		50
// fractional accumulator of how much to change visibility this frame
static FRACT			visLevelIncAcc, visLevelDecAcc;
// integer amount to change visiblility this turn
static SDWORD			visLevelInc, visLevelDec;

// percentage of power over which objects start to be visible
#define VIS_LEVEL_START		100
#define VIS_LEVEL_RANGE		60

// maximum possible sensor power
#define MAX_SENSOR_POWER	10000

// the last sensor power to see an object - used by process visibility
// to avoid having to recalculate it
//static SDWORD			lastSensorPower;

// whether a side has seen a unit on another side yet

/* Get all the objects that might intersect a ray.
 * Do not include psSource in the list as the ray comes from it
 */
void visGetTestObjects(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2,
							  BASE_OBJECT *psSource, BASE_OBJECT *psTarget);

/* Get the objects in the apsTestObjects array that intersect a ray */
void visGetRayObjects(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2);

void	setUnderTilesVis(BASE_OBJECT *psObj, UDWORD player);

/* Variables for the visibility callback */
static SDWORD		rayPlayer;				// The player the ray is being cast for
static SDWORD		startH;					// The height at the view point
static SDWORD		currG;					// The current obscuring gradient
static SDWORD		lastH, lastD;			// The last height and distance
static BOOL			rayStart;				// Whether this is the first point on the ray
static SDWORD		tarDist;				// The distance to the ray target
static BOOL			blockingWall;			// Whether walls block line of sight
static SDWORD		finalX,finalY;			// The final tile of the ray cast
static SDWORD		numWalls;				// Whether the LOS has hit a wall
static SDWORD		wallX,wallY;			// the position of a wall if it is on the LOS

// initialise the visibility stuff
BOOL visInitialise(void)
{
	visLevelIncAcc = MAKEFRACT(0);
	visLevelDecAcc = MAKEFRACT(0);
	visLevelInc = 0;
	visLevelDec = 0;

	return TRUE;
}

// update the visibility change levels
void visUpdateLevel(void)
{
	visLevelIncAcc += FRACTmul(MAKEFRACT(frameTime),
							   FRACTCONST(VIS_LEVEL_INC,GAME_TICKS_PER_SEC));
	visLevelInc = MAKEINT(visLevelIncAcc);
	visLevelIncAcc -= MAKEFRACT(visLevelInc);
	visLevelDecAcc += FRACTmul(MAKEFRACT(frameTime),
							   FRACTCONST(VIS_LEVEL_DEC,GAME_TICKS_PER_SEC));
	visLevelDec = MAKEINT(visLevelDecAcc);
	visLevelDecAcc -= MAKEFRACT(visLevelDec);
}

/* Return the radius a base object covers on the map */
static SDWORD visObjRadius(BASE_OBJECT *psObject)
{
	SDWORD	radius;

	switch (psObject->type)
	{
	case OBJ_DROID:
		radius = psObject->sDisplay.imd->radius;
		break;
	case OBJ_STRUCTURE:
		radius = psObject->sDisplay.imd->radius;
		break;
	case OBJ_FEATURE:
		radius = psObject->sDisplay.imd->radius;
		break;
	default:
		ASSERT( FALSE,"visObjRadius: unknown object type" );
		radius = 0;
		break;
	}

	return radius;
}

static SDWORD visObjHeight(BASE_OBJECT *psObject)
{
	SDWORD	height;

	switch (psObject->type)
	{
	case OBJ_DROID:
		height = 80;
//		height = psObject->sDisplay.imd->ymax;
		break;
	case OBJ_STRUCTURE:
		height = psObject->sDisplay.imd->ymax;
		break;
	case OBJ_FEATURE:
		height = psObject->sDisplay.imd->ymax;
		break;
	default:
		ASSERT( FALSE,"visObjHeight: unknown object type" );
		height = 0;
		break;
	}

	return height;
}


/* The terrain revealing ray callback */
static BOOL rayTerrainCallback(SDWORD x, SDWORD y, SDWORD dist)
{
	SDWORD		newH, newG;		// The new gradient
	MAPTILE		*psTile;

	ASSERT( x >= 0 && x < ((SDWORD)mapWidth << TILE_SHIFT) &&
			y >= 0 && y < ((SDWORD)mapHeight << TILE_SHIFT),
			"rayTerrainCallback: coords off map" );

	psTile = mapTile(x >> TILE_SHIFT, y >> TILE_SHIFT);



	/* Not true visibility - done on sensor range */

	if(dist == 0) {	//Complete hack PD.. John what should happen if dist is 0 ???

		debug( LOG_NEVER, "rayTerrainCallback: dist == 0, will divide by zero\n" );

		dist = 1;
	}

	newH = psTile->height * ELEVATION_SCALE;
	newG = (newH - startH) * GRAD_MUL / (SDWORD)dist;
	if (newG >= currG)
	{
		currG = newG;

		SET_TILE_VISIBLE(rayPlayer, psTile);
		if(rayPlayer == selectedPlayer && !bDisplaySensorRange)
		{
			psTile->inRange = UBYTE_MAX;
		}

		// new - ask alex M
		if( (selectedPlayer!=rayPlayer) &&
			(bMultiPlayer && (game.type == TEAMPLAY || game.alliance == ALLIANCES_TEAMS)
			&& aiCheckAlliances(selectedPlayer,rayPlayer)) )
		{
			SET_TILE_VISIBLE(selectedPlayer,psTile);		//reveal radar
		}

		// new - ask Alex M
	/* Not true visibility - done on sensor range */

		if(getRevealStatus())
		{
			if( ((UDWORD)rayPlayer == selectedPlayer) ||
				// new - ask AM
				(bMultiPlayer && (game.type == TEAMPLAY || game.alliance == ALLIANCES_TEAMS)
				&& aiCheckAlliances(selectedPlayer,rayPlayer)) // can see opponent moving
				// new - ask AM
				)
			{
				avInformOfChange(x>>TILE_SHIFT,y>>TILE_SHIFT);		//reveal map
			}
		}

	}

	return TRUE;
}


/* The los ray callback */
static BOOL rayLOSCallback(SDWORD x, SDWORD y, SDWORD dist)
{
	SDWORD		newG;		// The new gradient
//	MAPTILE		*psTile;
	SDWORD		distSq;
	SDWORD		tileX,tileY;
	MAPTILE		*psTile;

	ASSERT( x >= 0 && x < ((SDWORD)mapWidth << TILE_SHIFT) &&
			y >= 0 && y < ((SDWORD)mapHeight << TILE_SHIFT),
			"rayLOSCallback: coords off map" );

/*	if(dist == 0) {	//Complete hack PD.. John what should happen if dist is 0 ???
		DBPRINTF(("rayTerrainCallback: dist == 0, will divide by zero\n"));
		dist = 1;
	}*/

	distSq = dist*dist;

	if (rayStart)
	{
		rayStart = FALSE;
	}
	else
	{
		// see if the ray hit an object on the last tile
/*		if (currObj < numRayObjects &&
			distSq > aRayObjDist[currObj])
		{
			lastH += visObjHeight(apsRayObjects[currObj]);
			currObj += 1;
		}*/

		// Calculate the current LOS gradient
		newG = (lastH - startH) * GRAD_MUL / lastD;
		if (newG >= currG)
		{
			currG = newG;
		}
	}

	// See if the ray has reached the target
	if (distSq >= tarDist)
	{
		lastD = dist;
		return FALSE;
	}
	else
	{
		// Store the height at this tile for next time round
	//	psTile = mapTile(x >> TILE_SHIFT, y >> TILE_SHIFT);
	//	lastH = psTile->height * ELEVATION_SCALE;
		tileX = x>>TILE_SHIFT;
		tileY = y>>TILE_SHIFT;

		if (blockingWall && !((tileX == finalX) && (tileY == finalY)))
		{
			psTile = mapTile(x >> TILE_SHIFT, y >> TILE_SHIFT);
			if ((psTile->tileInfoBits & BITS_WALL) &&
				!TILE_HAS_SMALLSTRUCTURE(psTile))
			{
				lastH = 2*UBYTE_MAX * ELEVATION_SCALE;
	//			currG = UBYTE_MAX * ELEVATION_SCALE * GRAD_MUL / lastD;
				numWalls += 1;
				wallX = x;
				wallY = y;
	//			return FALSE;
			}
			else
			{
				lastH = map_Height((UDWORD)x, (UDWORD)y);
			}
		}
		else
		{
			lastH = map_Height((UDWORD)x, (UDWORD)y);
		}
		lastD = dist;
	}

	return TRUE;
}

//#define VTRAYSTEP	(NUM_RAYS/80)
#define VTRAYSTEP	(NUM_RAYS/120)

//SDWORD currRayAng;
//BOOL currRayPending = FALSE;

#define	DUPF_SCANTERRAIN 0x01

BOOL visTilesPending(BASE_OBJECT *psObj)
{
	ASSERT( psObj->type == OBJ_DROID,"visTilesPending : Only implemented for droids" );

	return (((DROID*)psObj)->updateFlags & DUPF_SCANTERRAIN);
}


/* Check which tiles can be seen by an object */
void visTilesUpdate(BASE_OBJECT *psObj,BOOL SpreadLoad)
{
	SDWORD	range;
	SDWORD	ray;

	// Get the sensor Range and power
	switch (psObj->type)
	{
	case OBJ_DROID:	// Done whenever a droid is built or moves to a new tile.
		range = ((DROID *)psObj)->sensorRange;
		break;
	case OBJ_STRUCTURE:	// Only done when structure initialy built.
		ASSERT( SpreadLoad == FALSE,"visTilesUpdate : Can only spread load for droids" );	// can't spread load for structures.
		range = ((STRUCTURE *)psObj)->sensorRange;
		break;
	default:
		ASSERT( FALSE,
			"visTilesUpdate: visibility checking is only implemented for"
			"units and structures" );
		return;
	}



	rayPlayer = psObj->player;

	if(SpreadLoad) {
		// Just do 4 rays at right angles.

		DROID *psDroid = (DROID*)psObj;
		SDWORD currRayAng;

		if((psDroid->updateFlags & DUPF_SCANTERRAIN) == 0) {
			psDroid->currRayAng = 0;
			psDroid->updateFlags |= DUPF_SCANTERRAIN;
		}

		currRayAng = (SDWORD)psDroid->currRayAng;

		// Cast the rays from the viewer
		startH = psObj->z + visObjHeight(psObj);
		currG = -UBYTE_MAX * GRAD_MUL;
		rayCast(psObj->x,psObj->y,currRayAng, range, rayTerrainCallback);

		startH = psObj->z + visObjHeight(psObj);
		currG = -UBYTE_MAX * GRAD_MUL;
		rayCast(psObj->x,psObj->y,(currRayAng+(NUM_RAYS/4))%360, range, rayTerrainCallback);

		startH = psObj->z + visObjHeight(psObj);
		currG = -UBYTE_MAX * GRAD_MUL;
		rayCast(psObj->x,psObj->y,(currRayAng+(NUM_RAYS/2))%360, range, rayTerrainCallback);

		startH = psObj->z + visObjHeight(psObj);
		currG = -UBYTE_MAX * GRAD_MUL;
		rayCast(psObj->x,psObj->y,(currRayAng+(NUM_RAYS/2)+(NUM_RAYS/4))%360, range, rayTerrainCallback);

		psDroid->currRayAng += VTRAYSTEP;
//DBPRINTF(("%p %d\n",psDroid,psDroid->currRayAng);
		if(psDroid->currRayAng >= (NUM_RAYS/4)) {
			psDroid->currRayAng = 0;
			psDroid->updateFlags &= ~DUPF_SCANTERRAIN;
//DBPRINTF(("%p done\n",psDroid);
		}
	} else {
		// Do the whole circle.
		for(ray=0; ray < NUM_RAYS; ray += NUM_RAYS/80)
		{
			// initialise the callback variables
	/*		startH = (mapTile(psObj->x >> TILE_SHIFT,psObj->y >> TILE_SHIFT)->height)
						* ELEVATION_SCALE;*/
			startH = psObj->z + visObjHeight(psObj);
			currG = -UBYTE_MAX * GRAD_MUL;

			// Cast the rays from the viewer
			rayCast(psObj->x,psObj->y,ray, range, rayTerrainCallback);
		}
	}
}





/* Check whether psViewer can see psTarget.
 * psViewer should be an object that has some form of sensor,
 * currently droids and structures.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 * struckBlock controls whether structures block LOS
 */
//BOOL visibleObjectBlock(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget, BOOL structBlock)
BOOL visibleObject(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget)
{
	SDWORD		x,y, ray;
	SDWORD		xdiff,ydiff, rangeSquared;
	SDWORD		range;
	UDWORD		senPower, ecmPower;
//	SDWORD		x1,y1, x2,y2;
	SDWORD		tarG, top;
	STRUCTURE	*psStruct;

	/* Get the sensor Range and power */
	switch (psViewer->type)
	{
	case OBJ_DROID:
		range = ((DROID *)psViewer)->sensorRange;
		senPower = ((DROID *)psViewer)->sensorPower;
		if (((DROID*)psViewer)->droidType == DROID_COMMAND)
		{
			range = 3 * range / 2;
		}
		break;
	case OBJ_STRUCTURE:
		psStruct = (STRUCTURE *)psViewer;

		// a structure that is being built cannot see anything
		if (psStruct->status != SS_BUILT)
		{
			return FALSE;
		}

		if ((psStruct->pStructureType->type == REF_WALL) ||
			(psStruct->pStructureType->type == REF_WALLCORNER))
		{
			return FALSE;
		}

		if ((structCBSensor((STRUCTURE *)psViewer) ||
			 structVTOLCBSensor((STRUCTURE *)psViewer)) &&
			 ((STRUCTURE *)psViewer)->psTarget[0] == psTarget)
		{
			// if a unit is targetted by a counter battery sensor
			// it is automatically seen
//			lastSensorPower = senPower;
			return TRUE;
		}

		range = ((STRUCTURE *)psViewer)->sensorRange;
		senPower = ((STRUCTURE *)psViewer)->sensorPower;

		// increase the sensor range for AA sites
		// AA sites are defensive structures that can only shoot in the air
		if ( (psStruct->pStructureType->type == REF_DEFENSE) &&
			 (asWeaponStats[psStruct->asWeaps[0].nStat].surfaceToAir == SHOOT_IN_AIR) )
		{
			range = 3 * range / 2;
		}

		break;
	default:
		ASSERT( FALSE,
			"visibleObject: visibility checking is only implemented for"
			"units and structures" );
		return FALSE;
		break;
	}

	/* Get the target's ecm power (if it has one)
	 * or that of a nearby ECM droid.
	 */
	switch (psTarget->type)
	{
	case OBJ_DROID:
		ecmPower = ((DROID *)psTarget)->ECMMod;
		break;
	case OBJ_STRUCTURE:
		ecmPower = ((STRUCTURE *)psTarget)->ecmPower;
		range = 4 * range / 3;
		break;
	default:
		/* No ecm so zero power */
		ecmPower = 0;
		break;
	}

	// fix to see units and structures of your ally in teamplay mode
	/*
	if(bMultiPlayer && game.type == TEAMPLAY && aiCheckAlliances(psViewer->player,psTarget->player))
	{
		if( (psViewer->type == OBJ_DROID) || (psViewer->type == OBJ_STRUCTURE) )
			{
				if( (psTarget->type == OBJ_DROID) || (psTarget->type == OBJ_STRUCTURE) )
				{
					return(TRUE);
				}
			}
	}
	*/


	/* First see if the target is in sensor range */
	x = (SDWORD)psViewer->x;
	xdiff = x - (SDWORD)psTarget->x;
	if (xdiff < 0)
	{
		xdiff = -xdiff;
	}
	if (xdiff > range)
	{
		// too far away, reject
		return FALSE;
	}

	y = (SDWORD)psViewer->y;
	ydiff = y - (SDWORD)psTarget->y;
	if (ydiff < 0)
	{
		ydiff = -ydiff;
	}
	if (ydiff > range)
	{
		// too far away, reject
		return FALSE;
	}

	rangeSquared = xdiff*xdiff + ydiff*ydiff;
	if (rangeSquared > (range*range))
	{
		/* Out of sensor range */
		return FALSE;
	}

//	if (rangeSquared > BASE_VISIBILITY*BASE_VISIBILITY)
//	{
		/* Not automatically seen so have to check against ecm */
//		sensorPower = visCalcPower(psViewer->x,psViewer->y, psTarget->x,psTarget->y,
//									sensorPower, sensorRange);
//		lastSensorPower = senPower;
		// ecm power was already calculated in processVisiblity
/*		if (sensorPower < ecmPower)
		{
			return FALSE;
		}*/
//	}
//	else
//	{
//		lastSensorPower = MAX_SENSOR_POWER;	// NOTE this was "lastSensorPower == 0" which I assume was wrong (PD)
//	}
	if (rangeSquared == 0)
	{
		// Should never be on top of each other, but ...
		return TRUE;
	}

	// initialise the callback variables
//	startH = mapTile(x>>TILE_SHIFT,y>>TILE_SHIFT)->height * ELEVATION_SCALE;
	startH = psViewer->z;
	startH += visObjHeight(psViewer);
	currG = -UBYTE_MAX * GRAD_MUL * ELEVATION_SCALE;
	tarDist = rangeSquared;
	rayStart = TRUE;
	currObj = 0;
	ray = NUM_RAYS-1 - calcDirection(psViewer->x,psViewer->y, psTarget->x,psTarget->y);
	finalX = psTarget->x >> TILE_SHIFT;
	finalY = psTarget->y >> TILE_SHIFT;

/*	if (structBlock)
	{
		// Get the objects that might intersect the rays for this quadrant
		if (ray < NUM_RAYS/2)
		{
			x1 = x - xDiff;
			x2 = x;
		}
		else
		{
			x1 = x;
			x2 = x + xDiff;
		}
		if (ray < NUM_RAYS/4 || ray > 3*NUM_RAYS/4)
		{
			y1 = y;
			y2 = y + yDiff;
		}
		else
		{
			y1 = y - yDiff;
			y2 = y;
		}
		visGetTestObjects(x1,y1, x2,y2, psViewer, psTarget);

		// Get the objects that actually intersect the ray
		visGetRayObjects(x,y, (SDWORD)psTarget->x,(SDWORD)psTarget->y);
	}
	else*/
	{
		// don't check for any objects intersecting the ray
		numRayObjects = 0;
	}

	// Cast a ray from the viewer to the target
	rayCast(x,y, ray, range, rayLOSCallback);

	// See if the target can be seen
	top = ((SDWORD)psTarget->z + visObjHeight(psTarget) - startH);
//	tarG = (top*top) * GRAD_MUL / rangeSquared;
//	if (top < 0)
//	{
//		tarG = - tarG;
//	}
	tarG = top * GRAD_MUL / lastD;

	return tarG >= currG;
}


// Do visibility check, but with walls completely blocking LOS.
BOOL visibleObjWallBlock(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget)
{
	BOOL	result;

	blockingWall = TRUE;
	result = visibleObject(psViewer,psTarget);
	blockingWall = FALSE;

	return result;
}


// Find the wall that is blocking LOS to a target (if any)
BOOL visGetBlockingWall(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget, STRUCTURE **ppsWall)
{
	SDWORD		tileX, tileY, player;
	STRUCTURE	*psCurr, *psWall;

	blockingWall = TRUE;
	numWalls = 0;
	visibleObject(psViewer, psTarget);
	blockingWall = FALSE;

	// see if there was a wall in the way
	psWall = NULL;
	if (numWalls == 1)
	{
		tileX = wallX >> TILE_SHIFT;
		tileY = wallY >> TILE_SHIFT;
		for(player=0; player<MAX_PLAYERS; player += 1)
		{
			for(psCurr = apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
			{
				if (((psCurr->x >> TILE_SHIFT) == tileX) &&
					((psCurr->y >> TILE_SHIFT) == tileY))
				{
					psWall = psCurr;
					goto found;
				}
			}
		}
	}

found:
	*ppsWall = psWall;

	return psWall != NULL;;
}


/* Check whether psViewer can see psTarget.
 * psViewer should be an object that has some form of sensor,
 * currently droids and structures.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 */
/*BOOL visibleObject(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget)
{
	BOOL	structBlock;

	switch (psTarget->type)
	{
	case OBJ_DROID:
		structBlock = FALSE;
//		structBlock = TRUE;
		break;
	default:
		structBlock = FALSE;
		break;
	}

	return visibleObjectBlock(psViewer,psTarget, structBlock);
}*/

/*BOOL	blockingTile(UDWORD x, UDWORD y)
{
	TILE	*psTile;

	// get a pointer to the tile
	psTile = mapTile(x,y);

	// Is it anything other than grass or sand?
	if (psTile->type != TER_GRASS && psTile->type!=TER_SAND)
		return(TRUE);
	else
		return(FALSE);
}*/

/* Find out what can see this object */
void processVisibility(BASE_OBJECT *psObj)
{
//	DROID		*psCount;
	DROID		*psDroid;
	STRUCTURE	*psBuilding;
	UDWORD		i, maxPower, ecmPoints;
//	UDWORD		currPower;
	ECM_STATS	*psECMStats;
	BOOL		prevVis[MAX_PLAYERS], currVis[MAX_PLAYERS];
//	SDWORD		maxSensor[MAX_PLAYERS];
	SDWORD		visLevel;
//	SDWORD		powerRatio;
	BASE_OBJECT	*psViewer;
//	BOOL		changed;
	MESSAGE		*psMessage;
	UDWORD		player, ally;

	// calculate the ecm power for the object based on other ECM's in the area

	maxPower = 0;

	// set the current ecm power
	switch (psObj->type)
	{
	case OBJ_DROID:
		psDroid = (DROID *)psObj;
		psECMStats = asECMStats + psDroid->asBits[COMP_ECM].nStat;
		ecmPoints = ecmPower(psECMStats, psDroid->player);
		//if (psECMStats->power < maxPower)
		if (ecmPoints < maxPower)
		{
			psDroid->ECMMod = maxPower;
		}
		else
		{
			//psDroid->ECMMod = psECMStats->power;
			psDroid->ECMMod = ecmPoints;
			maxPower = psDroid->ECMMod;
		}
		break;
	case OBJ_STRUCTURE:
		psBuilding = (STRUCTURE *)psObj;
		psECMStats = psBuilding->pStructureType->pECM;
		if (psECMStats && psECMStats->power > maxPower)
		{
			psBuilding->ecmPower = (UWORD)psECMStats->power;
		}
		else
		{
			psBuilding->ecmPower = (UWORD)maxPower;
			maxPower = psBuilding->ecmPower;
		}
		break;
	case OBJ_FEATURE:
	default:
		// no ecm's on features
		break;
	}

	// initialise the visibility array
	for (i=0; i<MAX_PLAYERS; i++)
	{
		prevVis[i] = psObj->visible[i] != 0;
	}
	if (psObj->type == OBJ_DROID)
	{
		memset (currVis, 0, sizeof(BOOL) * MAX_PLAYERS);

		// one can trivially see oneself
		currVis[psObj->player]=TRUE;
	}
	else
	{
		memcpy(currVis, prevVis, sizeof(BOOL) * MAX_PLAYERS);
	}


	// get all the objects from the grid the droid is in
	gridStartIterate((SDWORD)psObj->x, (SDWORD)psObj->y);

	// Make sure allies can see us
	if( bMultiPlayer && (game.type == TEAMPLAY || game.alliance == ALLIANCES_TEAMS) )
	{
		for(player=0; player<MAX_PLAYERS; player++)
		{
			if(player!=psObj->player)
			{
				if(aiCheckAlliances(player,psObj->player))
				{
					currVis[player] = TRUE;
				}
			}
		}
	}

    //if a player has a SAT_UPLINK structure, they can see everything!
    for (player=0; player<MAX_PLAYERS; player++)
    {
        if (getSatUplinkExists(player))
        {
            currVis[player] = TRUE;
			if (psObj->visible[player] == 0)
			{
				psObj->visible[player] = 1;
			}
        }
    }

	psViewer = gridIterate();
	while (psViewer != NULL)
	{
		// If we've got ranged line of sight...
 		if ( (psViewer->type != OBJ_FEATURE) &&
			 !currVis[psViewer->player] &&
			 visibleObject(psViewer, psObj) )
 		{
			// Tell system that this side can see this object
 			currVis[psViewer->player]=TRUE;
			if (!prevVis[psViewer->player])
			{

				if (psObj->visible[psViewer->player] == 0)
				{
					psObj->visible[psViewer->player] = 1;
				}
				clustObjectSeen(psObj, psViewer);
			}

 		}

		psViewer = gridIterate();
	}

	//forward out vision to our allies
	if (bMultiPlayer && (game.type == TEAMPLAY || game.alliance == ALLIANCES_TEAMS))
	{
		for(player = 0; player < MAX_PLAYERS; player++)
		{
			for(ally = 0; ally < MAX_PLAYERS; ally++)
			{
				if (currVis[player] && aiCheckAlliances(player, ally))
				{
					currVis[ally] = TRUE;
				}
			}
		}
	}


	// update the visibility levels
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if (i == psObj->player)
		{
			psObj->visible[i] = UBYTE_MAX;
			continue;
		}

		visLevel = 0;
		if (currVis[i])
		{
			visLevel = UBYTE_MAX;
		}

		if ( (visLevel < psObj->visible[i]) &&
			 (psObj->type == OBJ_DROID) )
		{
			if (psObj->visible[i] <= visLevelDec)
			{
				psObj->visible[i] = 0;
			}
			else
			{
				psObj->visible[i] = (UBYTE)(psObj->visible[i] - visLevelDec);
			}
		}
		else if (visLevel > psObj->visible[i])
		{
			if (psObj->visible[i] + visLevelInc >= UBYTE_MAX)
			{
				psObj->visible[i] = UBYTE_MAX;
			}
			else
			{
				psObj->visible[i] = (UBYTE)(psObj->visible[i] + visLevelInc);
			}
		}
	}

	// if a structure has just become visible set the tile flags
	if (psObj->type == OBJ_STRUCTURE && !prevVis[selectedPlayer] && psObj->visible[selectedPlayer])
	{
		setStructTileDraw((STRUCTURE *)psObj);

	}

	/* Make sure all tiles under a feature/structure become visible when you see it */
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if( (psObj->type == OBJ_STRUCTURE || psObj->type == OBJ_FEATURE) &&
			(!prevVis[i] && psObj->visible[i]) )
		{
			setUnderTilesVis(psObj,i);
		}
	}

	// if a feature has just become visible set the tile flags
	if (psObj->type == OBJ_FEATURE && !prevVis[selectedPlayer] && psObj->visible[selectedPlayer])
	{
		setFeatTileDraw((FEATURE *)psObj);
		/*if this is an oil resource we want to add a proximity message for
		the selected Player - if there isn't an Resource Extractor on it*/
		if (((FEATURE *)psObj)->psStats->subType == FEAT_OIL_RESOURCE)
		{
			if(!TILE_HAS_STRUCTURE(mapTile(psObj->x >> TILE_SHIFT,
				psObj->y >> TILE_SHIFT)))
			{
				psMessage = addMessage(MSG_PROXIMITY, TRUE, selectedPlayer);
				if (psMessage)
				{
					psMessage->pViewData = (MSG_VIEWDATA *)psObj;
				}
				if(!bInTutorial)
				{
					//play message to indicate been seen
					audio_QueueTrackPos( ID_SOUND_RESOURCE_HERE,
										psObj->x, psObj->y, psObj->z );
				}
			}
		}

			if (((FEATURE *)psObj)->psStats->subType == FEAT_GEN_ARTE)
			{
				psMessage = addMessage(MSG_PROXIMITY, TRUE, selectedPlayer);
				if (psMessage)
				{
					psMessage->pViewData = (MSG_VIEWDATA *)psObj;
				}
				if(!bInTutorial)
				{
					//play message to indicate been seen
					audio_QueueTrackPos( ID_SOUND_ARTEFACT_DISC,
									psObj->x, psObj->y, psObj->z );
				}
			}
	}
}

void	setUnderTilesVis(BASE_OBJECT *psObj,UDWORD player)
{
UDWORD		i,j;
UDWORD		mapX, mapY, width,breadth;
FEATURE		*psFeature;
STRUCTURE	*psStructure;
FEATURE_STATS	*psStats;
MAPTILE		*psTile;

	if(psObj->type == OBJ_FEATURE)
	{
		psFeature = (FEATURE*)psObj;
		psStats = psFeature->psStats;
		width = psStats->baseWidth;
		breadth = psStats->baseBreadth;
	 	mapX = (psFeature->x - width * TILE_UNITS / 2) >> TILE_SHIFT;
		mapY = (psFeature->y - breadth * TILE_UNITS / 2) >> TILE_SHIFT;
	}
	else
	{
		/* Must be a structure */
		psStructure = (STRUCTURE*)psObj;
		width = psStructure->pStructureType->baseWidth;
		breadth = psStructure->pStructureType->baseBreadth;
		mapX = (psStructure->x - width * TILE_UNITS / 2) >> TILE_SHIFT;
		mapY = (psStructure->y - breadth * TILE_UNITS / 2) >> TILE_SHIFT;
	}

	for (i = 0; i < width; i++)
	{
		for (j = 0; j < breadth; j++)
		{

			/* Slow fade up */
			if(getRevealStatus())
			{
				if(player == selectedPlayer)
				{
					avInformOfChange(mapX+i,mapY+j);
				}
			}

			psTile = mapTile(mapX+i,mapY+j);
			SET_TILE_VISIBLE(player, psTile);
		}
	}
}


/* Get all the objects that might intersect a ray.
 * Do not include psSource in the list as the ray comes from it
 */
void visGetTestObjects(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2,
							  BASE_OBJECT *psSource, BASE_OBJECT *psTarget)
{
	SDWORD		player; //, bx1,by1, bx2,by2;
	SDWORD		radius;
	BASE_OBJECT	*psObj;

	// find the structures
	numTestObjects = 0;
	for (player=0; player < MAX_PLAYERS; player++)
	{
		for(psObj = (BASE_OBJECT *)apsStructLists[player]; psObj; psObj = psObj->psNext)
		{
			radius = visObjRadius(psObj);
			if ((((SDWORD)psObj->x + radius) >= x1) && (((SDWORD)psObj->x - radius) <= x2) &&
				(((SDWORD)psObj->y + radius) >= y1) && (((SDWORD)psObj->y - radius) <= y2) &&
				psObj != psSource && psObj != psTarget &&
				numTestObjects < MAX_RAYTEST)
			{
				apsTestObjects[numTestObjects++] = psObj;
			}
		}
	}

	// find the features
	for(psObj = (BASE_OBJECT *)apsFeatureLists[0]; psObj; psObj = psObj->psNext)
	{
		radius = visObjRadius(psObj);
		if ((((SDWORD)psObj->x + radius) >= x1) && (((SDWORD)psObj->x - radius) <= x2) &&
			(((SDWORD)psObj->y + radius) >= y1) && (((SDWORD)psObj->y - radius) <= y2) &&
			psObj != psSource && psObj != psTarget &&
			numTestObjects < MAX_RAYTEST)
		{
			apsTestObjects[numTestObjects++] = psObj;
		}
	}
}


/* Get the objects in the apsTestObjects array that intersect a ray */
void visGetRayObjects(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2)
{
	BASE_OBJECT		*apsObjs[MAX_RAYOBJ];
	SDWORD			aObjDist[MAX_RAYOBJ];
	BASE_OBJECT		*psObj;
	SDWORD			i,j, x,y, dist, xdiff,ydiff, furthest = 0;

	// find the objects that intersect the ray
	numRayObjects = 0;
	for(i=0; i<numTestObjects; i++)
	{
		psObj = apsTestObjects[i];
		x = (SDWORD)psObj->x;
		y = (SDWORD)psObj->y;
		dist = rayPointDist(x1,y1,x2,y2, x,y);
		if (dist < visObjRadius(psObj) &&
			numRayObjects < MAX_RAYOBJ)
		{
			// object intersects ray, calc squared distance
			xdiff = x - x1;
			ydiff = y - y1;
			aObjDist[numRayObjects] = xdiff*xdiff + ydiff*ydiff - dist*dist;
			apsObjs[numRayObjects] = psObj;
			numRayObjects += 1;
		}
	}

	// reorder the objects on distance
	for(i=0; i<numRayObjects; i++)
	{
		dist = SDWORD_MAX;
#ifdef DEBUG
		furthest = -1;
#else
		furthest = 0;
#endif
		for(j=0; j<numRayObjects; j++)
		{
			if (aObjDist[j] < dist)
			{
				furthest = j;
				dist = aObjDist[j];
			}
		}
		ASSERT( furthest != -1,
			"visGetRayObjects: reordering failed" );

		apsRayObjects[i] = apsObjs[furthest];
		aRayObjDist[i] = aObjDist[furthest];
		aObjDist[furthest] = SDWORD_MAX;
	}
}


// calculate the power at a given distance from a sensor/ecm
/*UDWORD visCalcPower(UDWORD x1,UDWORD y1, UDWORD x2,UDWORD y2, UDWORD power, UDWORD range)
{
	SDWORD	xdiff,ydiff;
	SDWORD	distSq,rangeSq, powerBoost;
	UDWORD	finalPower;
//	SDWORD	dist, absx,absy;

	xdiff = (SDWORD)x1 - (SDWORD)x2;
	ydiff = (SDWORD)y1 - (SDWORD)y2;
	distSq = xdiff*xdiff + ydiff*ydiff;
	rangeSq = (SDWORD)(range*range);
//	absx = abs(xdiff);
//	absy = abs(ydiff);
//	dist = absx > absy ? absx + absy/2 : absx/2 + absy;

//	if (dist >= range)
//	{
//		finalPower = 0;
//	}
//	else
//	{
//		finalPower = power - power * dist / range;
//	}

	if (distSq > rangeSq)
	{
		finalPower = 0;
	}
	else
	{
		// increase the power -> will be bigger than power for some of range
//		powerBoost = 3 * power / 2;
		powerBoost = power;
		finalPower = (UDWORD)(powerBoost - powerBoost * distSq / rangeSq);
		// bring the power lower than max power
		if (finalPower > power)
		{
			finalPower = power;
		}
	}

	return finalPower;
}*/


////////////////////////////////////////////////////////////////////
// alexl's sensor range.

BOOL bDisplaySensorRange;

void startSensorDisplay(void)
{
	MAPTILE		*psTile;
	UDWORD		x;
	DROID		*psDroid;
	STRUCTURE	*psStruct;
//	SDWORD		range;
//	SDWORD		ray;

	// clear each sensor bit.
	psTile = psMapTiles;
	for(x=0; x<(SDWORD)(mapWidth*mapHeight); x+= 1)
	{
		psTile->inRange = 0;
		psTile += 1;
	}

	// process the sensor range of all droids/structs.

	// units.
	for(psDroid = apsDroidLists[selectedPlayer];psDroid;psDroid=psDroid->psNext)
	{
//		range = psDroid->sensorRange;
//		for(ray=0; ray < NUM_RAYS; ray += NUM_RAYS/80)
//		{
//			startH = psDroid->z + visObjHeight((BASE_OBJECT*)psDroid);// initialise the callback variables //rayTerrainCallback
//			currG = -UBYTE_MAX * GRAD_MUL;	// Cast the rays from the viewer
			visTilesUpdate((BASE_OBJECT*)psDroid,FALSE);
//			rayCast(psDroid->x,psDroid->y,ray, range, rayTerrainCallback);
//		}

	}
	// structs.
	for(psStruct = apsStructLists[selectedPlayer];psStruct;psStruct=psStruct->psNext)
	{
		if(  psStruct->pStructureType->type != REF_WALL
 		  && psStruct->pStructureType->type != REF_WALLCORNER)
		{
			visTilesUpdate((BASE_OBJECT*)psStruct,FALSE);
		}
	}

	// set the display flag for the tiledraw er.
	bDisplaySensorRange = TRUE;
}

void stopSensorDisplay(void)
{
	// set the display flag off.
	bDisplaySensorRange = FALSE;
}
