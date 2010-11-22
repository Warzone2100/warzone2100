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

#include "display.h"
#include "multiplay.h"
#include "advvis.h"


// accuracy for the height gradient
#define GRAD_MUL	10000

// rate to change visibility level
static const float VIS_LEVEL_INC = 255 * 2;
static const float VIS_LEVEL_DEC = 50;

// fractional accumulator of how much to change visibility this frame
static float			visLevelIncAcc, visLevelDecAcc;

// integer amount to change visiblility this turn
static SDWORD			visLevelInc, visLevelDec;

// horrible hack because this code is full of them and I ain't rewriting it all - Per
#define MAX_SEEN_TILES 1024
static TILEPOS recordTilePos[MAX_SEEN_TILES];
static int lastRecordTilePos = -1;

typedef struct {
	bool rayStart; // Whether this is the first point on the ray
	const bool wallsBlock; // Whether walls block line of sight
	const int targetDistSq; // The distance to the ray target, squared
	const int startHeight; // The height at the view point
	const Vector2i final; // The final tile of the ray cast
	int lastHeight, lastDist; // The last height and distance
	int currGrad; // The current obscuring gradient
	int numWalls; // Whether the LOS has hit a wall
	Vector2i wall; // The position of a wall if it is on the LOS
} VisibleObjectHelp_t;


int *gNumWalls = NULL;
Vector2i * gWall = NULL;


/* Variables for the visibility callback / visTilesUpdate */
static SDWORD		rayPlayer;				// The player the ray is being cast for
static SDWORD		startH;					// The height at the view point
static SDWORD		currG;					// The current obscuring gradient

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

static int visObjHeight(const BASE_OBJECT * psObject)
{
	switch (psObject->type)
	{
		case OBJ_DROID:
	//		return psObject->sDisplay.imd->pos.max.y;
			return 80;
		case OBJ_STRUCTURE:
		case OBJ_FEATURE:
			return psObject->sDisplay.imd->max.y;
		default:
			ASSERT( false,"visObjHeight: unknown object type" );
			return 0;
	}
}

/* Record all tiles that some object confers visibility to. Only record each tile
 * once. Note that there is both a limit to how many objects can watch any given
 * tile, and a limit to how many tiles each object can watch. Strange but non fatal
 * things will happen if these limits are exceeded. This function uses icky globals. */
static inline void visMarkTile(int mapX, int mapY, MAPTILE *psTile)
{
	bool alreadySeen = false;
	int i;

	if (psTile->watchers[rayPlayer] < UBYTE_MAX && lastRecordTilePos < MAX_SEEN_TILES)
	{
		for (i = 0; i < lastRecordTilePos; i++)
		{
			if (recordTilePos[i].x == mapX && recordTilePos[i].y == mapY)
			{
				alreadySeen = true;
				break;
			}
		}
		if (!alreadySeen)
		{
			psTile->watchers[rayPlayer]++;			// we see this tile
			recordTilePos[lastRecordTilePos].x = mapX;	// record having seen it
			recordTilePos[lastRecordTilePos].y = mapY;
			lastRecordTilePos++;
		}
	}
}

/* The terrain revealing ray callback */
bool rayTerrainCallback(Vector3i pos, int distSq, void * data)
{
	const int mapX = map_coord(pos.x);
	const int mapY = map_coord(pos.y);
	int dist = sqrtf(distSq);
	int newH, newG; // The new gradient
	MAPTILE *psTile;

	ASSERT(pos.x >= 0 && pos.x < world_coord(mapWidth)
		&& pos.y >= 0 && pos.y < world_coord(mapHeight),
			"rayTerrainCallback: coords off map" );

	psTile = mapTile(mapX, mapY);

	/* Not true visibility - done on sensor range */

	if (dist == 0)
	{
		debug(LOG_ERROR, "rayTerrainCallback: dist is 0, which is not a valid distance");
		dist = 1;
	}

	newH = psTile->height * ELEVATION_SCALE;
	newG = (newH - startH) * GRAD_MUL / dist;
	if (newG >= currG)
	{
		currG = newG;

		SET_TILE_VISIBLE(rayPlayer, psTile);

		if (selectedPlayer != rayPlayer && bMultiPlayer && game.alliance == ALLIANCES_TEAMS
		    && aiCheckAlliances(selectedPlayer, rayPlayer))
		{
			SET_TILE_VISIBLE(selectedPlayer,psTile);		//reveal radar
		}

		visMarkTile(mapX, mapY, psTile);	// Mark this tile as seen by our sensor

		/* Single player visibility system - fix me one day to work properly in MP */
		if (rayPlayer == selectedPlayer
		    || (bMultiPlayer && game.alliance == ALLIANCES_TEAMS
			&& aiCheckAlliances(selectedPlayer, rayPlayer)))
		{
			avInformOfChange(map_coord(pos.x), map_coord(pos.y));		//reveal map
		}
	}

	return true;
}

/* The los ray callback */
static bool rayLOSCallback(Vector3i pos, int distSq, void *data)
{
	VisibleObjectHelp_t * help = data;
	int dist = sqrtf(distSq);

	ASSERT(pos.x >= 0 && pos.x < world_coord(mapWidth)
		&& pos.y >= 0 && pos.y < world_coord(mapHeight),
			"rayLOSCallback: coords off map" );

	if (help->rayStart)
	{
		help->rayStart = false;
	}
	else
	{
		// Calculate the current LOS gradient
		int newGrad = (help->lastHeight - help->startHeight) * GRAD_MUL / MAX(1, help->lastDist);
		if (newGrad >= help->currGrad)
		{
			help->currGrad = newGrad;
		}
	}

	help->lastDist = dist;
	help->lastHeight = map_Height(pos.x, pos.y);

	// See if the ray has reached the target
	if (distSq >= help->targetDistSq)
	{
		return false;
	}

	if (help->wallsBlock)
	{
		// Store the height at this tile for next time round
		Vector2i tile = { map_coord(pos.x), map_coord(pos.y) };

		if (!Vector2i_Compare(tile, help->final))
		{
			MAPTILE *psTile = mapTile(tile.x, tile.y);
			if (TileHasWall(psTile) && !TileHasSmallStructure(psTile))
			{
				help->lastHeight = 2*UBYTE_MAX * ELEVATION_SCALE;
				help->wall = Vector2i_Init(pos.x, pos.y);
				help->numWalls++;
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

/* Remove tile visibility from object */
void visRemoveVisibility(BASE_OBJECT *psObj)
{
	if (psObj->watchedTiles && psObj->numWatchedTiles > 0)
	{
		int i = 0;

		ASSERT_OR_RETURN(, psObj->player < MAX_PLAYERS, "Player index %d out of bounds for %s", (int)psObj->player, objInfo(psObj));
		for (i = 0; i < psObj->numWatchedTiles; i++)
		{
			const TILEPOS pos = psObj->watchedTiles[i];
			MAPTILE *psTile = mapTile(pos.x, pos.y);

			ASSERT(psTile->watchers[psObj->player] > 0, "Not watching watched tile (%d, %d)", (int)pos.x, (int)pos.y);
			psTile->watchers[psObj->player]--;
		}
		free(psObj->watchedTiles);
		psObj->watchedTiles = NULL;
		psObj->numWatchedTiles = 0;
	}
}

void visRemoveVisibilityOffWorld(BASE_OBJECT *psObj)
{
	free(psObj->watchedTiles);
	psObj->watchedTiles = NULL;
	psObj->numWatchedTiles = 0;
}

/* Check which tiles can be seen by an object */
void visTilesUpdate(BASE_OBJECT *psObj, RAY_CALLBACK callback)
{
	Vector3i pos = { psObj->pos.x, psObj->pos.y, 0 };
	int range = objSensorRange(psObj);
	int ray;

	ASSERT(psObj->type != OBJ_FEATURE, "visTilesUpdate: visibility updates are not for features!");

	lastRecordTilePos = 0;
	memset(recordTilePos, 0, sizeof(recordTilePos));
	rayPlayer = psObj->player;

	// Remove previous map visibility provided by object
	visRemoveVisibility(psObj);

	if (psObj->type == OBJ_STRUCTURE)
	{
		STRUCTURE * psStruct = (STRUCTURE *)psObj;
		if (psStruct->status != SS_BUILT ||
		    psStruct->pStructureType->type == REF_WALL || psStruct->pStructureType->type == REF_WALLCORNER)
		{
			// unbuilt structures and walls do not confer visibility.
			return;
		}
	}

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

	// Make sure we also watch tiles underneath ourselves
	if (psObj->type == OBJ_STRUCTURE)
	{
		int i, j;

		for (i = 0; i <= ((STRUCTURE *)psObj)->pStructureType->baseBreadth; i++)
		{
			for (j = 0; j <= ((STRUCTURE *)psObj)->pStructureType->baseWidth; j++)
			{
				int	mapX = map_coord(psObj->pos.x) + i;
				int	mapY = map_coord(psObj->pos.y) + j;
				MAPTILE	*psTile = mapTile(mapX, mapY);

				visMarkTile(mapX, mapY, psTile);
			}
		}
	}
	else
	{
		MAPTILE	*psTile = mapTile(map_coord(psObj->pos.x), map_coord(psObj->pos.y));
		int	mapX = map_coord(psObj->pos.x);
		int	mapY = map_coord(psObj->pos.y);

		visMarkTile(mapX, mapY, psTile);
	}

	// Record new map visibility provided by object
	if (lastRecordTilePos > 0)
	{
		psObj->watchedTiles = calloc(lastRecordTilePos, sizeof(*psObj->watchedTiles));
		psObj->numWatchedTiles = lastRecordTilePos;
		memcpy(psObj->watchedTiles, recordTilePos, lastRecordTilePos * sizeof(*psObj->watchedTiles));
	}
}

/* Check whether psViewer can see psTarget.
 * psViewer should be an object that has some form of sensor,
 * currently droids and structures.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 * struckBlock controls whether structures block LOS
 */
bool visibleObject(const BASE_OBJECT* psViewer, const BASE_OBJECT* psTarget, bool wallsBlock)
{
	Vector3i pos, dest, diff;
	int range, distSq;

	ASSERT(psViewer != NULL, "Invalid viewer pointer!");
	ASSERT(psTarget != NULL, "Invalid viewed pointer!");

	if (!psViewer || !psTarget)
	{
		return false;
	}

	// FIXME HACK Needed since we got those ugly Vector3uw floating around in BASE_OBJECT...
	pos = Vector3uw_To3i(psViewer->pos);
	dest = Vector3uw_To3i(psTarget->pos);
	diff = Vector3i_Sub(dest, pos);
	range = objSensorRange(psViewer);

	/* Get the sensor Range and power */
	switch (psViewer->type)
	{
		case OBJ_DROID:
		{
			DROID *psDroid = (DROID *)psViewer;

			if (psDroid->droidType == DROID_COMMAND)
			{
				range = 3 * range / 2;
			}
			if (psDroid->psTarget == psTarget
			    && (cbSensorDroid(psDroid) || objRadarDetector((BASE_OBJECT *)psDroid)))
			{
				// if it is targetted by a counter battery sensor, it is seen
				return UBYTE_MAX;
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

			if (psStruct->psTarget[0] == psTarget
			    && (structCBSensor(psStruct) || structVTOLCBSensor(psStruct) || objRadarDetector((BASE_OBJECT *)psStruct)))
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
	distSq = Vector3i_ScalarP(diff, diff);
	if (distSq == 0)
	{
		// Should never be on top of each other, but ...
		return true;
	}

	if (distSq > (range*range))
	{
		/* Out of sensor range */
		return false;
	}

	{
		// initialise the callback variables
		VisibleObjectHelp_t help = { true, wallsBlock, distSq, pos.z + visObjHeight(psViewer), { map_coord(dest.x), map_coord(dest.y) }, 0, 0, -UBYTE_MAX * GRAD_MUL * ELEVATION_SCALE, 0, { 0, 0 } };
		int targetGrad, top;

		// Cast a ray from the viewer to the target
		rayCast(pos, diff, range, rayLOSCallback, &help);

		if (gWall != NULL && gNumWalls != NULL) // Out globals are set
		{
			*gWall = help.wall;
			*gNumWalls = help.numWalls;
		}

		// See if the target can be seen
		top = dest.z + visObjHeight(psTarget) - help.startHeight;
		targetGrad = top * GRAD_MUL / MAX(1, help.lastDist);

		return targetGrad >= help.currGrad;
	}
}


// Find the wall that is blocking LOS to a target (if any)
STRUCTURE* visGetBlockingWall(const BASE_OBJECT* psViewer, const BASE_OBJECT* psTarget)
{
	int numWalls = 0;
	Vector2i wall = { 0, 0 };

	// HACK Using globals to not clutter visibleObject() interface too much
	gNumWalls = &numWalls;
	gWall = &wall;

	visibleObject(psViewer, psTarget, true);

	gNumWalls = NULL;
	gWall = NULL;

	// see if there was a wall in the way
	if (numWalls > 0)
	{
		Vector2i tile = { map_coord(wall.x), map_coord(wall.y) };
		unsigned int player;

		for (player = 0; player < MAX_PLAYERS; player++)
		{
			STRUCTURE* psWall;

			for (psWall = apsStructLists[player]; psWall; psWall = psWall->psNext)
			{
				if (map_coord(psWall->pos.x) == tile.x
				 && map_coord(psWall->pos.y) == tile.y)
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
	BOOL		prevVis[MAX_PLAYERS], currVis[MAX_PLAYERS];
	SDWORD		visLevel;
	BASE_OBJECT	*psViewer;
	MESSAGE		*psMessage;
	unsigned int player;

	// initialise the visibility array
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		prevVis[player] = (psObj->visible[player] != 0);
	}

	// Droids can vanish from view, other objects will stay
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
	gridStartIterate(psObj->pos.x, psObj->pos.y);

	// Make sure allies can see us
	if (bMultiPlayer && game.alliance == ALLIANCES_TEAMS)
	{
		unsigned int player;
		for (player = 0; player < MAX_PLAYERS; player++)
		{
			if (player != psObj->player && aiCheckAlliances(player, psObj->player))
			{
				currVis[player] = true;
			}
		}
	}

	// if a player has a SAT_UPLINK structure, or has godMode enabled,
	// they can see everything!
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		if (getSatUplinkExists(player) || (player == selectedPlayer && godMode))
		{
			currVis[player] = true;
			if (psObj->visible[player] == 0)
			{
				psObj->visible[player] = 1;
			}
		}
	}

	while (psViewer = gridIterate(), psViewer != NULL)
	{
		// If we've got ranged line of sight...
		if ( (psViewer->type != OBJ_FEATURE) &&
			 !currVis[psViewer->player] &&
			 visibleObject(psViewer, psObj, false) )
 		{
			// Tell system that this side can see this object
			currVis[psViewer->player] = true;
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
	}

	//forward out vision to our allies
	if (bMultiPlayer && game.alliance == ALLIANCES_TEAMS)
	{
		unsigned int player;
		for (player = 0; player < MAX_PLAYERS; player++)
		{
			unsigned int ally;
			for (ally = 0; ally < MAX_PLAYERS; ally++)
			{
				if (currVis[player] && aiCheckAlliances(player, ally))
				{
					currVis[ally] = true;
				}
			}
		}
	}

	// update the visibility levels
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		if (player == psObj->player)
		{
			// owner can always see it fully
			psObj->visible[player] = UBYTE_MAX;
			continue;
		}

		visLevel = (currVis[player] ? UBYTE_MAX : 0);

		// Droids can vanish from view, other objects will stay
		if ( (visLevel < psObj->visible[player]) &&
			 (psObj->type == OBJ_DROID) )
		{
			if (psObj->visible[player] <= visLevelDec)
			{
				psObj->visible[player] = 0;
			}
			else
			{
				psObj->visible[player] -= visLevelDec;
			}
		}
		else if (visLevel > psObj->visible[player])
		{
			if (psObj->visible[player] + visLevelInc >= UBYTE_MAX)
			{
				psObj->visible[player] = UBYTE_MAX;
			}
			else
			{
				psObj->visible[player] += visLevelInc;
			}
		}
	}

	/* Make sure all tiles under a feature/structure become visible when you see it */
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		if ( (psObj->type == OBJ_STRUCTURE || psObj->type == OBJ_FEATURE) &&
			!prevVis[player] && psObj->visible[player] &&
			!godMode )
		{
			setUnderTilesVis(psObj, player);
		}
	}

	// if a feature has just become visible set the message blips
	if (psObj->type == OBJ_FEATURE)
	{
		for (player = 0; player < MAX_PLAYERS; player++)
		{
			if (!prevVis[player] && psObj->visible[player])
			{
				/* If this is an oil resource we want to add a proximity message for
				 * the selected Player - if there isn't an Resource Extractor on it. */
				if (((FEATURE *)psObj)->psStats->subType == FEAT_OIL_RESOURCE)
				{
					if (!TileHasStructure(mapTile(map_coord(psObj->pos.x), map_coord(psObj->pos.y))))
					{
						psMessage = addMessage(MSG_PROXIMITY, true, player);
						if (psMessage)
						{
							psMessage->pViewData = (MSG_VIEWDATA *)psObj;
						}
						if (!bInTutorial && player == selectedPlayer)
						{
							//play message to indicate been seen
							audio_QueueTrackPos( ID_SOUND_RESOURCE_HERE, psObj->pos.x, psObj->pos.y, psObj->pos.z );
						}
						debug(LOG_MSG, "Added message for oil well, pViewData=%p", psMessage->pViewData);
					}
				}
				else if (((FEATURE *)psObj)->psStats->subType == FEAT_GEN_ARTE)
				{
					psMessage = addMessage(MSG_PROXIMITY, true, player);
					if (psMessage)
					{
						psMessage->pViewData = (MSG_VIEWDATA *)psObj;
					}
					if (!bInTutorial && player == selectedPlayer)
					{
						// play message to indicate been seen
						audio_QueueTrackPos( ID_SOUND_ARTEFACT_DISC, psObj->pos.x, psObj->pos.y, psObj->pos.z );
					}
					debug(LOG_MSG, "Added message for artefact, pViewData=%p", psMessage->pViewData);
				}
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

bool scrRayTerrainCallback(Vector3i pos, int distSq, void *data)
{
	int dist = sqrtf(distSq);
	int newH, newG; // The new gradient
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
	newG = (newH - startH) * GRAD_MUL / dist;
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

/* Check whether psViewer can fire directly at psTarget.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 */
bool lineOfFire(const BASE_OBJECT* psViewer, const BASE_OBJECT* psTarget, bool wallsBlock)
{
	Vector3i pos, dest, diff;
	int range, distSq;

	ASSERT(psViewer != NULL, "Invalid shooter pointer!");
	ASSERT(psTarget != NULL, "Invalid target pointer!");
	if (!psViewer || !psTarget)
	{
		return false;
	}

	// FIXME HACK Needed since we got those ugly Vector3uw floating around in BASE_OBJECT...
	pos = Vector3uw_To3i(psViewer->pos);
	dest = Vector3uw_To3i(psTarget->pos);
	diff = Vector3i_Sub(dest, pos);
	range = objSensorRange(psViewer);

	distSq = Vector3i_ScalarP(diff, diff);
	if (distSq == 0)
	{
		// Should never be on top of each other, but ...
		return true;
	}

	// initialise the callback variables
	{
		VisibleObjectHelp_t help = { true, wallsBlock, distSq, pos.z + visObjHeight(psViewer), { map_coord(dest.x), map_coord(dest.y) }, 0, 0, -UBYTE_MAX * GRAD_MUL * ELEVATION_SCALE, 0, { 0, 0 } };
		int targetGrad, top;

		// Cast a ray from the viewer to the target
		rayCast(pos, diff, range, rayLOSCallback, &help);

		if (gWall != NULL && gNumWalls != NULL) // Out globals are set
		{
			*gWall = help.wall;
			*gNumWalls = help.numWalls;
		}

		// See if the target can be seen
		top = dest.z + visObjHeight(psTarget) - help.startHeight;
		targetGrad = top * GRAD_MUL / MAX(1, help.lastDist);

		return targetGrad >= help.currGrad;
	}
}
