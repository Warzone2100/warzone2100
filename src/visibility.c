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
/**
 * @file visibility.c
 * Handles object visibility.
 * Pumpkin Studios, Eidos Interactive 1996.
 */
#include "lib/framework/frame.h"

#include "lib/gamelib/gtime.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"

#include "visibility.h"

#include "objects.h"
#include "map.h"
#include "loop.h"
#include "raycast.h"
#include "geometry.h"
#include "hci.h"
#include "mapgrid.h"
#include "cluster.h"
#include "scriptextern.h"
#include "structure.h"

#include "multiplay.h"
#include "advvis.h"


// accuracy for the height gradient
#define GRAD_MUL	10000

// which object is being considered by the callback
static SDWORD			currObj;

// rate to change visibility level
const static float VIS_LEVEL_INC = 255 * 2;
const static float VIS_LEVEL_DEC = 50;

// fractional accumulator of how much to change visibility this frame
static float			visLevelIncAcc, visLevelDecAcc;

// integer amount to change visiblility this turn
static SDWORD			visLevelInc, visLevelDec;

// alexl's sensor range.
BOOL bDisplaySensorRange;

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
	visLevelIncAcc = 0;
	visLevelDecAcc = 0;
	visLevelInc = 0;
	visLevelDec = 0;

	return true;
}

// update the visibility change levels
void visUpdateLevel(void)
{
	visLevelIncAcc += timeAdjustedIncrement(VIS_LEVEL_INC, true);
	visLevelInc = visLevelIncAcc;
	visLevelIncAcc -= visLevelInc;
	visLevelDecAcc += timeAdjustedIncrement(VIS_LEVEL_DEC, true);
	visLevelDec = visLevelDecAcc;
	visLevelDecAcc -= visLevelDec;
}

static SDWORD visObjHeight(const BASE_OBJECT * const psObject)
{
	SDWORD	height;

	switch (psObject->type)
	{
	case OBJ_DROID:
		height = 80;
//		height = psObject->sDisplay.imd->pos.max.y;
		break;
	case OBJ_STRUCTURE:
		height = psObject->sDisplay.imd->max.y;
		break;
	case OBJ_FEATURE:
		height = psObject->sDisplay.imd->max.y;
		break;
	default:
		ASSERT( false,"visObjHeight: unknown object type" );
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

	ASSERT(x >= 0
	    && x < world_coord(mapWidth)
	    && y >= 0
	    && y < world_coord(mapHeight),
			"rayTerrainCallback: coords off map" );

	psTile = mapTile(map_coord(x), map_coord(y));

	/* Not true visibility - done on sensor range */

	if (dist == 0)
	{
		debug(LOG_ERROR, "rayTerrainCallback: dist is 0, which is not a valid distance");
		dist = 1;
	}

	newH = psTile->height * ELEVATION_SCALE;
	newG = (newH - startH) * GRAD_MUL / (SDWORD)dist;
	if (newG >= currG)
	{
		currG = newG;

		SET_TILE_VISIBLE(rayPlayer, psTile);

		if (selectedPlayer != rayPlayer && bMultiPlayer && game.alliance == ALLIANCES_TEAMS
		    && aiCheckAlliances(selectedPlayer, rayPlayer))
		{
			SET_TILE_VISIBLE(selectedPlayer,psTile);		//reveal radar
		}

		/* Not true visibility - done on sensor range */

		if(getRevealStatus())
		{
			if ((UDWORD)rayPlayer == selectedPlayer
			    || (bMultiPlayer && game.alliance == ALLIANCES_TEAMS
				&& aiCheckAlliances(selectedPlayer, rayPlayer)))
			{
				// can see opponent moving
				avInformOfChange(map_coord(x), map_coord(y));		//reveal map
				psTile->activeSensor = true;
			}
		}
	}

	return true;
}

/* The los ray callback */
static BOOL rayLOSCallback(SDWORD x, SDWORD y, SDWORD dist)
{
	SDWORD		newG;		// The new gradient
	SDWORD		distSq;
	SDWORD		tileX,tileY;
	MAPTILE		*psTile;

	ASSERT(x >= 0
	    && x < world_coord(mapWidth)
	    && y >= 0
	    && y < world_coord(mapHeight),
			"rayLOSCallback: coords off map" );

	distSq = dist*dist;

	if (rayStart)
	{
		rayStart = false;
	}
	else
	{
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
		return false;
	}
	else
	{
		// Store the height at this tile for next time round
		tileX = map_coord(x);
		tileY = map_coord(y);

		if (blockingWall && !((tileX == finalX) && (tileY == finalY)))
		{
			psTile = mapTile(tileX, tileY);
			if (TILE_HAS_WALL(psTile) && !TILE_HAS_SMALLSTRUCTURE(psTile))
			{
				lastH = 2*UBYTE_MAX * ELEVATION_SCALE;
	//			currG = UBYTE_MAX * ELEVATION_SCALE * GRAD_MUL / lastD;
				numWalls += 1;
				wallX = x;
				wallY = y;
	//			return false;
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

	return true;
}

#define VTRAYSTEP	(NUM_RAYS/120)
#define	DUPF_SCANTERRAIN 0x01

BOOL visTilesPending(BASE_OBJECT *psObj)
{
	ASSERT( psObj->type == OBJ_DROID,"visTilesPending : Only implemented for droids" );

	return (((DROID*)psObj)->updateFlags & DUPF_SCANTERRAIN);
}

/* Check which tiles can be seen by an object */
void visTilesUpdate(BASE_OBJECT *psObj)
{
	SDWORD	range = objSensorRange(psObj);
	SDWORD	ray;

	ASSERT(psObj->type != OBJ_FEATURE, "visTilesUpdate: visibility updates are not for features!");

	rayPlayer = psObj->player;

	// Do the whole circle.
	for(ray = 0; ray < NUM_RAYS; ray += NUM_RAYS / 80)
	{
		// initialise the callback variables
		startH = psObj->pos.z + visObjHeight(psObj);
		currG = -UBYTE_MAX * GRAD_MUL;

		// Cast the rays from the viewer
		rayCast(psObj->pos.x, psObj->pos.y,ray, range, rayTerrainCallback);
	}
}

/* Check whether psViewer can see psTarget.
 * psViewer should be an object that has some form of sensor,
 * currently droids and structures.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 * struckBlock controls whether structures block LOS
 */
BOOL visibleObject(const BASE_OBJECT* psViewer, const BASE_OBJECT* psTarget)
{
	SDWORD		x,y, ray;
	SDWORD		xdiff,ydiff, rangeSquared;
	SDWORD		range = objSensorRange(psViewer);
	SDWORD		tarG, top;

	ASSERT(psViewer != NULL, "Invalid viewer pointer!");
	ASSERT(psTarget != NULL, "Invalid viewed pointer!");
	if (!psViewer || !psTarget)
	{
		return false;
	}

	/* Get the sensor Range and power */
	switch (psViewer->type)
	{
	case OBJ_DROID:
	{
		const DROID * const psDroid = (const DROID *)psViewer;

		if (psDroid->droidType == DROID_COMMAND)
		{
			range = 3 * range / 2;
		}
		break;
	}
	case OBJ_STRUCTURE:
	{
		const STRUCTURE * const psStruct = (const STRUCTURE *)psViewer;

		// a structure that is being built cannot see anything
		if (psStruct->status != SS_BUILT)
		{
			return false;
		}

		if (psStruct->pStructureType->type == REF_WALL
		 || psStruct->pStructureType->type == REF_WALLCORNER)
		{
			return false;
		}

		if ((structCBSensor(psStruct)
		  || structVTOLCBSensor(psStruct))
		 && psStruct->psTarget[0] == psTarget)
		{
			// if a unit is targetted by a counter battery sensor
			// it is automatically seen
			return true;
		}

		// increase the sensor range for AA sites
		// AA sites are defensive structures that can only shoot in the air
		if (psStruct->pStructureType->type == REF_DEFENSE
		 && asWeaponStats[psStruct->asWeaps[0].nStat].surfaceToAir == SHOOT_IN_AIR)
		{
			range = 3 * range / 2;
		}

		break;
	}
	default:
		ASSERT( false,
			"visibleObject: visibility checking is only implemented for"
			"units and structures" );
		return false;
		break;
	}

	// Structures can be seen from further away
	if (psTarget->type == OBJ_STRUCTURE)
	{
		range = 4 * range / 3;
	}

	/* First see if the target is in sensor range */
	x = psViewer->pos.x;
	xdiff = x - (SDWORD)psTarget->pos.x;
	if (xdiff < 0)
	{
		xdiff = -xdiff;
	}
	if (xdiff > range)
	{
		// too far away, reject
		return false;
	}

	y = psViewer->pos.y;
	ydiff = y - (SDWORD)psTarget->pos.y;
	if (ydiff < 0)
	{
		ydiff = -ydiff;
	}
	if (ydiff > range)
	{
		// too far away, reject
		return false;
	}

	rangeSquared = xdiff*xdiff + ydiff*ydiff;
	if (rangeSquared > (range*range))
	{
		/* Out of sensor range */
		return false;
	}

	if (rangeSquared == 0)
	{
		// Should never be on top of each other, but ...
		return true;
	}

	// initialise the callback variables
	startH = psViewer->pos.z;
	startH += visObjHeight(psViewer);
	currG = -UBYTE_MAX * GRAD_MUL * ELEVATION_SCALE;
	tarDist = rangeSquared;
	rayStart = true;
	currObj = 0;
	ray = NUM_RAYS-1 - calcDirection(psViewer->pos.x,psViewer->pos.y, psTarget->pos.x,psTarget->pos.y);
	finalX = map_coord(psTarget->pos.x);
	finalY = map_coord(psTarget->pos.y);

	// Cast a ray from the viewer to the target
	rayCast(x,y, ray, range, rayLOSCallback);

	// See if the target can be seen
	top = ((SDWORD)psTarget->pos.z + visObjHeight(psTarget) - startH);
	tarG = top * GRAD_MUL / lastD;

	return tarG >= currG;
}

// Do visibility check, but with walls completely blocking LOS.
BOOL visibleObjWallBlock(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget)
{
	BOOL	result;

	blockingWall = true;
	result = visibleObject(psViewer,psTarget);
	blockingWall = false;

	return result;
}

// Find the wall that is blocking LOS to a target (if any)
STRUCTURE* visGetBlockingWall(const BASE_OBJECT* psViewer, const BASE_OBJECT* psTarget)
{
	blockingWall = true;
	numWalls = 0;
	visibleObject(psViewer, psTarget);
	blockingWall = false;

	// see if there was a wall in the way
	if (numWalls == 1)
	{
		const int tileX = map_coord(wallX);
		const int tileY = map_coord(wallY);
		unsigned int player;

		for (player = 0; player < MAX_PLAYERS; ++player)
		{
			STRUCTURE* psWall;

			for (psWall = apsStructLists[player]; psWall; psWall = psWall->psNext)
			{
				if (map_coord(psWall->pos.x) == tileX
				 && map_coord(psWall->pos.y) == tileY)
				{
					return psWall;
				}
			}
		}
	}

	return NULL;
}

/* Find out what can see this object */
void processVisibility(BASE_OBJECT *psObj)
{
	UDWORD		i;
	BOOL		prevVis[MAX_PLAYERS], currVis[MAX_PLAYERS];
	SDWORD		visLevel;
	BASE_OBJECT	*psViewer;
	MESSAGE		*psMessage;
	UDWORD		player, ally;

	// initialise the visibility array
	for (i=0; i<MAX_PLAYERS; i++)
	{
		prevVis[i] = psObj->visible[i] != 0;
	}
	if (psObj->type == OBJ_DROID)
	{
		memset (currVis, 0, sizeof(BOOL) * MAX_PLAYERS);

		// one can trivially see oneself
		currVis[psObj->player]=true;
	}
	else
	{
		memcpy(currVis, prevVis, sizeof(BOOL) * MAX_PLAYERS);
	}

	// get all the objects from the grid the droid is in
	gridStartIterate((SDWORD)psObj->pos.x, (SDWORD)psObj->pos.y);

	// Make sure allies can see us
	if (bMultiPlayer && game.alliance == ALLIANCES_TEAMS)
	{
		for(player=0; player<MAX_PLAYERS; player++)
		{
			if(player!=psObj->player)
			{
				if(aiCheckAlliances(player,psObj->player))
				{
					currVis[player] = true;
				}
			}
		}
	}

	// if a player has a SAT_UPLINK structure, they can see everything!
	for (player=0; player<MAX_PLAYERS; player++)
	{
		if (getSatUplinkExists(player))
		{
			currVis[player] = true;
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
 			currVis[psViewer->player]=true;
			if (!prevVis[psViewer->player])
			{

				if (psObj->visible[psViewer->player] == 0)
				{
					psObj->visible[psViewer->player] = 1;
				}
				if(psObj->type != OBJ_FEATURE)
				{
					// features are not in the cluster system
					clustObjectSeen(psObj, psViewer);
				}
			}

 		}

		psViewer = gridIterate();
	}

	//forward out vision to our allies
	if (bMultiPlayer && game.alliance == ALLIANCES_TEAMS)
	{
		for(player = 0; player < MAX_PLAYERS; player++)
		{
			for(ally = 0; ally < MAX_PLAYERS; ally++)
			{
				if (currVis[player] && aiCheckAlliances(player, ally))
				{
					currVis[ally] = true;
				}
			}
		}
	}

	// update the visibility levels
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if (i == psObj->player)
		{
			// owner can always see it fully
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
		/*if this is an oil resource we want to add a proximity message for
		the selected Player - if there isn't an Resource Extractor on it*/
		if (((FEATURE *)psObj)->psStats->subType == FEAT_OIL_RESOURCE)
		{
			if(!TILE_HAS_STRUCTURE(mapTile(map_coord(psObj->pos.x),
			                               map_coord(psObj->pos.y))))
			{
				psMessage = addMessage(MSG_PROXIMITY, true, selectedPlayer);
				if (psMessage)
				{
					psMessage->pViewData = (MSG_VIEWDATA *)psObj;
				}
				if(!bInTutorial)
				{
					//play message to indicate been seen
					audio_QueueTrackPos( ID_SOUND_RESOURCE_HERE,
										psObj->pos.x, psObj->pos.y, psObj->pos.z );
				}
			}
		}

			if (((FEATURE *)psObj)->psStats->subType == FEAT_GEN_ARTE)
			{
				psMessage = addMessage(MSG_PROXIMITY, true, selectedPlayer);
				if (psMessage)
				{
					psMessage->pViewData = (MSG_VIEWDATA *)psObj;
				}
				if(!bInTutorial)
				{
					//play message to indicate been seen
					audio_QueueTrackPos( ID_SOUND_ARTEFACT_DISC,
									psObj->pos.x, psObj->pos.y, psObj->pos.z );
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
	 	mapX = map_coord(psFeature->pos.x - width * TILE_UNITS / 2);
		mapY = map_coord(psFeature->pos.y - breadth * TILE_UNITS / 2);
	}
	else
	{
		/* Must be a structure */
		psStructure = (STRUCTURE*)psObj;
		width = psStructure->pStructureType->baseWidth;
		breadth = psStructure->pStructureType->baseBreadth;
		mapX = map_coord(psStructure->pos.x - width * TILE_UNITS / 2);
		mapY = map_coord(psStructure->pos.y - breadth * TILE_UNITS / 2);
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

void updateSensorDisplay()
{
	MAPTILE		*psTile = psMapTiles;
	int		x;
	DROID		*psDroid;
	STRUCTURE	*psStruct;

	// clear sensor info
	for (x = 0; x < mapWidth * mapHeight; x++)
	{
		psTile->activeSensor = false;
		psTile++;
	}

	// process the sensor range of all droids/structs.

	// units.
	for(psDroid = apsDroidLists[selectedPlayer];psDroid;psDroid=psDroid->psNext)
	{
		visTilesUpdate((BASE_OBJECT*)psDroid);
	}

	// structs.
	for(psStruct = apsStructLists[selectedPlayer];psStruct;psStruct=psStruct->psNext)
	{
		if (psStruct->pStructureType->type != REF_WALL
 		    && psStruct->pStructureType->type != REF_WALLCORNER)
		{
			visTilesUpdate((BASE_OBJECT*)psStruct);
		}
	}
}
