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

// holds information about map tiles visible by droids
static bool	scrTileVisible[MAX_PLAYERS][UBYTE_MAX][UBYTE_MAX] = {{{false}}};

bool scrTileIsVisible(SDWORD player, SDWORD x, SDWORD y);
void scrResetPlayerTileVisibility(SDWORD player);

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
bool rayTerrainCallback(Vector3i pos, int dist, void * data)
{
	SDWORD newH, newG; // The new gradient
	MAPTILE *psTile;

	ASSERT(pos.x >= 0 && pos.x < world_coord(mapWidth)
		&& pos.y >= 0 && pos.y < world_coord(mapHeight),
			"rayTerrainCallback: coords off map" );

	psTile = mapTile(map_coord(pos.x), map_coord(pos.y));

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
		if (rayPlayer == selectedPlayer
		    || (bMultiPlayer && game.alliance == ALLIANCES_TEAMS
			&& aiCheckAlliances(selectedPlayer, rayPlayer)))
		{
			// can see opponent moving
			if(getRevealStatus())
			{
				avInformOfChange(map_coord(pos.x), map_coord(pos.y));		//reveal map
			}
			psTile->activeSensor = true;
		}
	}

	return true;
}

/* The los ray callback */
static bool rayLOSCallback(Vector3i pos, int dist, void *data)
{
	int newG; // The new gradient
	int distSq = dist*dist;
	MAPTILE *psTile;

	ASSERT(pos.x >= 0 && pos.x < world_coord(mapWidth)
		&& pos.y >= 0 && pos.y < world_coord(mapHeight),
			"rayLOSCallback: coords off map" );

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

	lastD = dist;
	lastH = map_Height(pos.x, pos.y);

	// See if the ray has reached the target
	if (distSq >= tarDist)
	{
		return false;
	}

	{
		// Store the height at this tile for next time round
		Vector2i tile = { map_coord(pos.x), map_coord(pos.y) };

		if (blockingWall && !((tile.x == finalX) && (tile.y == finalY)))
		{
			psTile = mapTile(tile.x, tile.y);
			if (TileHasWall(psTile) && !TileHasSmallStructure(psTile))
			{
				lastH = 2*UBYTE_MAX * ELEVATION_SCALE;
				numWalls += 1;
				wallX = pos.x;
				wallY = pos.y;
			}
		}
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
void visTilesUpdate(BASE_OBJECT *psObj, RAY_CALLBACK callback)
{
	Vector3i pos = { psObj->pos.x, psObj->pos.y, 0 };
	int range = objSensorRange(psObj);
	int ray;

	ASSERT(psObj->type != OBJ_FEATURE, "visTilesUpdate: visibility updates are not for features!");

	rayPlayer = psObj->player;

	// Do the whole circle in 80 steps
	for (ray = 0; ray < NUM_RAYS; ray += NUM_RAYS / 80)
	{
		Vector3i dir = rayAngleToVector3i(ray);

		// initialise the callback variables
		startH = psObj->pos.z + visObjHeight(psObj);
		currG = -UBYTE_MAX * GRAD_MUL;

		// Cast the rays from the viewer
		rayCast(pos, dir, range, callback, NULL);
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
	Vector3i pos, dest, diff;
	int ray, range, rangeSquared;
	int tarG, top;

	ASSERT(psViewer != NULL, "Invalid viewer pointer!");
	ASSERT(psTarget != NULL, "Invalid viewed pointer!");

	if (!psViewer || !psTarget)
	{
		return false;
	}

	pos = Vector3i_New(psViewer->pos.x, psViewer->pos.y, 0);
	dest = Vector3i_New(psTarget->pos.x, psTarget->pos.y, 0);
	diff = Vector3i_Sub(pos, dest);
	range = objSensorRange(psViewer);

	/* Get the sensor Range and power */
	switch (psViewer->type)
	{
		case OBJ_DROID:
		{
			const DROID * psDroid = (DROID *)psViewer;

			if (psDroid->droidType == DROID_COMMAND)
			{
				range = 3 * range / 2;
			}
			break;
		}
		case OBJ_STRUCTURE:
		{
			const STRUCTURE * psStruct = (STRUCTURE *)psViewer;

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
	if (diff.x < 0)
	{
		diff.x = -diff.x;
	}
	if (diff.x > range)
	{
		// too far away, reject
		return false;
	}

	if (diff.y < 0)
	{
		diff.y = -diff.y;
	}
	if (diff.y > range)
	{
		// too far away, reject
		return false;
	}

	rangeSquared = Vector3i_ScalarP(diff, diff);
	if (rangeSquared == 0)
	{
		// Should never be on top of each other, but ...
		return true;
	}

	if (rangeSquared > (range*range))
	{
		/* Out of sensor range */
		return false;
	}

	// initialise the callback variables
	startH = pos.z;
	startH += visObjHeight(psViewer);
	currG = -UBYTE_MAX * GRAD_MUL * ELEVATION_SCALE;
	tarDist = rangeSquared;
	rayStart = true;
	currObj = 0;
	ray = NUM_RAYS-1 - calcDirection(pos.x, pos.y, dest.x, dest.y);
	finalX = map_coord(dest.x);
	finalY = map_coord(dest.y);

	// Cast a ray from the viewer to the target
	rayCast(pos, rayAngleToVector3i(ray), range, rayLOSCallback, NULL);

	// See if the target can be seen
	top = (dest.z + visObjHeight(psTarget) - startH);
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
			if(!TileHasStructure(mapTile(map_coord(psObj->pos.x),
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
				debug(LOG_MSG, "Added message for oil well, pViewData=%p", psMessage->pViewData);
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
				debug(LOG_MSG, "Added message for artefact, pViewData=%p", psMessage->pViewData);
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
		visTilesUpdate((BASE_OBJECT*)psDroid, rayTerrainCallback);
	}

	// structs.
	for(psStruct = apsStructLists[selectedPlayer];psStruct;psStruct=psStruct->psNext)
	{
		if (psStruct->pStructureType->type != REF_WALL
 		    && psStruct->pStructureType->type != REF_WALLCORNER)
		{
			visTilesUpdate((BASE_OBJECT*)psStruct, rayTerrainCallback);
		}
	}
}

bool scrRayTerrainCallback(Vector3i pos, int dist, void *data)
{
	SDWORD newH, newG; // The new gradient
	MAPTILE *psTile;

	ASSERT(pos.x >= 0 && pos.x < world_coord(mapWidth)
		&& pos.y >= 0 && pos.y < world_coord(mapHeight),
			"rayTerrainCallback: coords off map" );

	ASSERT(rayPlayer >= 0 && rayPlayer < MAX_PLAYERS, "rayScrTerrainCallback: wrong player index");

	psTile = mapTile(map_coord(pos.x), map_coord(pos.y));

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

		scrTileVisible[rayPlayer][map_coord(pos.x)][map_coord(pos.y)] = true;
	}

	return true;
}

void scrResetPlayerTileVisibility(SDWORD player)
{
	int	x,y;

	// clear script visibility info
	for (x = 0; x < mapWidth; x++)
	{
		for(y = 0; y < mapHeight; y++)
		{
			scrTileVisible[player][x][y] = false;
		}
	}
}

bool scrTileIsVisible(SDWORD player, SDWORD x, SDWORD y)
{
	ASSERT(x >= 0 && y >= 0 && x < UBYTE_MAX && y < UBYTE_MAX,
		"invalid tile coordinates");

	return scrTileVisible[player][x][y];
}
