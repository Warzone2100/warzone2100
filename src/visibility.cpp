/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
#include "research.h"
#include "scriptextern.h"
#include "structure.h"
#include "projectile.h"

#include "display.h"
#include "multiplay.h"

#include "wavecast.h"

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
#define MAX_SEEN_TILES (29*29 * 355/113)  // Increased hack to support 28 tile sensor radius. - Cyp

struct VisibleObjectHelp_t
{
	bool rayStart; // Whether this is the first point on the ray
	const bool wallsBlock; // Whether walls block line of sight
	const int startHeight; // The height at the view point
	const Vector2i final; // The final tile of the ray cast
	int lastHeight, lastDist; // The last height and distance
	int currGrad; // The current obscuring gradient
	int numWalls; // Whether the LOS has hit a wall
	Vector2i wall; // The position of a wall if it is on the LOS
};


static int *gNumWalls = NULL;
static Vector2i *gWall = NULL;


// initialise the visibility stuff
bool visInitialise(void)
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
	visLevelIncAcc += gameTimeAdjustedIncrement(VIS_LEVEL_INC);
	visLevelInc = visLevelIncAcc;
	visLevelIncAcc -= visLevelInc;
	visLevelDecAcc += gameTimeAdjustedIncrement(VIS_LEVEL_DEC);
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
static inline void visMarkTile(int mapX, int mapY, MAPTILE *psTile, int rayPlayer, TILEPOS *recordTilePos, int *lastRecordTilePos)
{
	if (psTile->watchers[rayPlayer] < UBYTE_MAX && *lastRecordTilePos < MAX_SEEN_TILES)
	{
		TILEPOS tilePos = {uint8_t(mapX), uint8_t(mapY)};
		psTile->watchers[rayPlayer]++;                  // we see this tile
		psTile->sensorBits |= (1 << rayPlayer);		// mark it as being seen
		recordTilePos[*lastRecordTilePos] = tilePos;    // record having seen it
		++*lastRecordTilePos;
	}
}

/* The terrain revealing ray callback */
static void doWaveTerrain(int sx, int sy, int sz, unsigned radius, int rayPlayer, TILEPOS *recordTilePos, int *lastRecordTilePos)
{
	size_t i;
	size_t size;
	const WavecastTile *tiles = getWavecastTable(radius, &size);
	int tileHeight, perspectiveHeight;
#define MAX_WAVECAST_LIST_SIZE 1360  // Trivial upper bound to what a fully upgraded WSS can use (its number of angles). Should probably be some factor times the maximum possible radius. Is probably a lot more than needed. Tested to need at least 180.
	int heights[2][MAX_WAVECAST_LIST_SIZE];
	int angles[2][MAX_WAVECAST_LIST_SIZE + 1];
	int readListSize = 0, readListPos = 0, writeListPos = 0;  // readListSize, readListPos dummy initialisations.
	int readList = 0;  // Reading from this list, writing to the other. Could also initialise to rand()%2.
	int lastHeight = 0;  // lastHeight dummy initialisation.
	int lastAngle = 0x7FFFFFFF;

	// Start with full vision of all angles. (If someday wanting to make droids that can only look in one direction, change here, after getting the original angle values saved in the wavecast table.)
	heights[!readList][writeListPos] = -0x7FFFFFFF-1;  // Smallest integer.
	angles[!readList][writeListPos] = 0;               // Smallest angle.
	++writeListPos;

	for (i = 0; i < size; ++i)
	{
		const int mapX = map_coord(sx) + tiles[i].dx;
		const int mapY = map_coord(sy) + tiles[i].dy;
		MAPTILE *psTile;
		bool seen = false;

		if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight)
		{
			continue;
		}
		psTile = mapTile(mapX, mapY);
		tileHeight = psTile->height;
		perspectiveHeight = (tileHeight - sz) * tiles[i].invRadius;

		if (tiles[i].angBegin < lastAngle)
		{
			// Gone around the circle. (Or just started scan.)
			angles[!readList][writeListPos] = lastAngle;

			// Flip the lists.
			readList = !readList;
			readListPos = 0;
			readListSize = writeListPos;
			writeListPos = 0;
			lastHeight = 1;  // Impossible value since tiles[i].invRadius > 1 for all i, so triggers writing first entry in list.
		}
		lastAngle = tiles[i].angEnd;

		while (angles[readList][readListPos + 1] <= tiles[i].angBegin && readListPos < readListSize)
		{
			++readListPos;  // Skip, not relevant.
		}

		while (angles[readList][readListPos] < tiles[i].angEnd && readListPos < readListSize)
		{
			int oldHeight = heights[readList][readListPos];
			int newHeight = MAX(oldHeight, perspectiveHeight);
			seen = seen || perspectiveHeight >= oldHeight;
			if (newHeight != lastHeight)
			{
				heights[!readList][writeListPos] = newHeight;
				angles[!readList][writeListPos] = MAX(angles[readList][readListPos], tiles[i].angBegin);
				lastHeight = newHeight;
				++writeListPos;
				ASSERT_OR_RETURN( , writeListPos <= MAX_WAVECAST_LIST_SIZE, "Visibility too complicated! Need to increase MAX_WAVECAST_LIST_SIZE.");
			}
			++readListPos;
		}
		--readListPos;

		if (seen)
		{
			// Can see this tile.
			psTile->tileExploredBits |= alliancebits[rayPlayer];                            // Share exploration with allies too
			visMarkTile(mapX, mapY, psTile, rayPlayer, recordTilePos, lastRecordTilePos);   // Mark this tile as seen by our sensor
		}
	}
}

/* The los ray callback */
static bool rayLOSCallback(Vector3i pos, int32_t dist, void *data)
{
	VisibleObjectHelp_t *help = (VisibleObjectHelp_t *)data;

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
	help->lastHeight = map_Height(removeZ(pos));

	if (help->wallsBlock)
	{
		// Store the height at this tile for next time round
		Vector2i tile = map_coord(removeZ(pos));

		if (tile != help->final)
		{
			MAPTILE *psTile = mapTile(tile);
			if (TileHasWall(psTile) && !TileHasSmallStructure(psTile))
			{
				help->lastHeight = 2*UBYTE_MAX * ELEVATION_SCALE;
				help->wall = removeZ(pos);
				help->numWalls++;
			}
		}
	}

	return true;
}

/* Remove tile visibility from object */
void visRemoveVisibility(BASE_OBJECT *psObj)
{
	if (psObj->watchedTiles && psObj->numWatchedTiles > 0 && mapWidth && mapHeight)
	{
		int i = 0;

		for (i = 0; i < psObj->numWatchedTiles; i++)
		{
			const TILEPOS pos = psObj->watchedTiles[i];
			MAPTILE *psTile = mapTile(pos.x, pos.y);

			// FIXME: the mapTile might have been swapped out, see swapMissionPointers()
			if (psTile->watchers[psObj->player] == 0 && game.type == CAMPAIGN)
			{
				continue;
			}
			ASSERT(psTile->watchers[psObj->player] > 0, "Not watching watched tile (%d, %d)", (int)pos.x, (int)pos.y);
			psTile->watchers[psObj->player]--;
			if (psTile->watchers[psObj->player] == 0)
			{
				psTile->sensorBits &= ~(1 << psObj->player);
			}
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
void visTilesUpdate(BASE_OBJECT *psObj)
{
	TILEPOS recordTilePos[MAX_SEEN_TILES];
	int lastRecordTilePos = 0;

	ASSERT(psObj->type != OBJ_FEATURE, "visTilesUpdate: visibility updates are not for features!");

	// Remove previous map visibility provided by object
	visRemoveVisibility(psObj);

	if (psObj->type == OBJ_STRUCTURE)
	{
		STRUCTURE * psStruct = (STRUCTURE *)psObj;
		if (psStruct->status != SS_BUILT ||
		    psStruct->pStructureType->type == REF_WALL || psStruct->pStructureType->type == REF_WALLCORNER || psStruct->pStructureType->type == REF_GATE)
		{
			// unbuilt structures and walls do not confer visibility.
			return;
		}
	}

	// Do the whole circle in ∞ steps. No more pretty moiré patterns.
	doWaveTerrain(psObj->pos.x, psObj->pos.y, psObj->pos.z + visObjHeight(psObj), objSensorRange(psObj), psObj->player, recordTilePos, &lastRecordTilePos);

	// Record new map visibility provided by object
	if (lastRecordTilePos > 0)
	{
		psObj->watchedTiles = (TILEPOS *)malloc(lastRecordTilePos * sizeof(*psObj->watchedTiles));
		psObj->numWatchedTiles = lastRecordTilePos;
		memcpy(psObj->watchedTiles, recordTilePos, lastRecordTilePos * sizeof(*psObj->watchedTiles));
	}
}

/*reveals all the terrain in the map*/
void revealAll(UBYTE player)
{
	UWORD   i, j;
	MAPTILE	*psTile;
	
	//reveal all tiles
	for(i=0; i<mapWidth; i++)
	{
		for(j=0; j<mapHeight; j++)
		{
			psTile = mapTile(i,j);
			psTile->tileExploredBits |= alliancebits[player];
		}
	}
	
	//the objects gets revealed in processVisibility()
}

/* Check whether psViewer can see psTarget.
 * psViewer should be an object that has some form of sensor,
 * currently droids and structures.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 * struckBlock controls whether structures block LOS
 */
int visibleObject(const BASE_OBJECT* psViewer, const BASE_OBJECT* psTarget, bool wallsBlock)
{
	ASSERT_OR_RETURN(0, psViewer != NULL, "Invalid viewer pointer!");
	ASSERT_OR_RETURN(0, psTarget != NULL, "Invalid viewed pointer!");

	int range = objSensorRange(psViewer);

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
			    && (cbSensorDroid(psDroid) || objRadarDetector(psDroid)))
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
				return 0;
			}

			if (psStruct->pStructureType->type == REF_WALL
				|| psStruct->pStructureType->type == REF_GATE
				|| psStruct->pStructureType->type == REF_WALLCORNER)
			{
				return 0;
			}

			if (psStruct->psTarget[0] == psTarget
			    && (structCBSensor(psStruct) || structVTOLCBSensor(psStruct) || objRadarDetector((BASE_OBJECT *)psStruct)))
			{
				// if a unit is targetted by a counter battery sensor
				// it is automatically seen
				return UBYTE_MAX;
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
			ASSERT(false, "Visibility checking is only implemented for units and structures");
			return 0;
			break;
	}

	// Structures can be seen from further away
	if (psTarget->type == OBJ_STRUCTURE)
	{
		range = 4 * range / 3;
	}

	Vector2i diff = removeZ(psTarget->pos - psViewer->pos);

	/* First see if the target is in sensor range */
	int dist = iHypot(diff);
	if (dist == 0)
	{
		// Should never be on top of each other, but ...
		return UBYTE_MAX;
	}

	if (dist > range)
	{
		/* Out of sensor range */
		return 0;
	}

	// initialise the callback variables
	VisibleObjectHelp_t help = { true, wallsBlock, psViewer->pos.z + visObjHeight(psViewer), map_coord(removeZ(psTarget->pos)), 0, 0, -UBYTE_MAX * GRAD_MUL * ELEVATION_SCALE, 0, Vector2i(0, 0)};
	int targetGrad, top;

	// Cast a ray from the viewer to the target
	rayCast(psViewer->pos, iAtan2(diff), dist, rayLOSCallback, &help);

	if (gWall != NULL && gNumWalls != NULL) // Out globals are set
	{
		*gWall = help.wall;
		*gNumWalls = help.numWalls;
	}

	// See if the target can be seen
	top = psTarget->pos.z + visObjHeight(psTarget) - help.startHeight;
	targetGrad = top * GRAD_MUL / MAX(1, help.lastDist);

	if (targetGrad >= help.currGrad)
	{
		return UBYTE_MAX;
	}
	return 0;
}

// Find the wall that is blocking LOS to a target (if any)
STRUCTURE* visGetBlockingWall(const BASE_OBJECT* psViewer, const BASE_OBJECT* psTarget)
{
	int numWalls = 0;
	Vector2i wall;

	// HACK Using globals to not clutter visibleObject() interface too much
	gNumWalls = &numWalls;
	gWall = &wall;

	visibleObject(psViewer, psTarget, true);

	gNumWalls = NULL;
	gWall = NULL;

	// see if there was a wall in the way
	if (numWalls > 0)
	{
		Vector2i tile = map_coord(wall);
		unsigned int player;

		for (player = 0; player < MAX_PLAYERS; player++)
		{
			STRUCTURE* psWall;

			for (psWall = apsStructLists[player]; psWall; psWall = psWall->psNext)
			{
				if (map_coord(removeZ(psWall->pos)) == tile)
				{
					return psWall;
				}
			}
		}
	}

	return NULL;
}

bool hasSharedVision(unsigned viewer, unsigned ally)
{
	ASSERT_OR_RETURN(false, viewer < MAX_PLAYERS && ally < MAX_PLAYERS, "Bad viewer %u or ally %u.", viewer, ally);

	return viewer == ally || (bMultiPlayer && game.alliance == ALLIANCES_TEAMS && aiCheckAlliances(viewer, ally));
}

static void setSeenBy(BASE_OBJECT *psObj, unsigned viewer, int val /*= UBYTE_MAX*/)
{
	//forward out vision to our allies
	int ally;
	for (ally = 0; ally < MAX_PLAYERS; ++ally)
	{
		if (hasSharedVision(viewer, ally))
		{
			psObj->seenThisTick[ally] = MAX(psObj->seenThisTick[ally], val);
		}
	}
}

// Calculate which objects we should know about based on alliances and satellite view.
static void processVisibilitySelf(BASE_OBJECT *psObj)
{
	int viewer;

	if (psObj->type == OBJ_DROID)  // Why only droids? Would psObj->type != OBJ_FEATURE && psObj->sensorRange != 0 be a better check?
	{
		// one can trivially see oneself
		setSeenBy(psObj, psObj->player, UBYTE_MAX);
	}

	// if a player has a SAT_UPLINK structure, or has godMode enabled,
	// they can see everything!
	for (viewer = 0; viewer < MAX_PLAYERS; viewer++)
	{
		if (getSatUplinkExists(viewer) || (viewer == selectedPlayer && godMode))
		{
			setSeenBy(psObj, viewer, UBYTE_MAX);
		}
	}

	psObj->bTargetted = false;	// Remove any targetting locks from last update.
}

// Calculate which objects we can see. Better to call after processVisibilitySelf, since that check is cheaper.
static void processVisibilityVision(BASE_OBJECT *psViewer)
{
	BASE_OBJECT *psObj;

	if (psViewer->type == OBJ_FEATURE)
	{
		return;
	}

	// get all the objects from the grid the droid is in
	// Will give inconsistent results if hasSharedVision is not an equivalence relation.
	gridStartIterateUnseen(psViewer->pos.x, psViewer->pos.y, psViewer->sensorRange, psViewer->player);
	while (psObj = gridIterate(), psObj != NULL)
	{
		int val = visibleObject(psViewer, psObj, false);

		// If we've got ranged line of sight...
		if (val > 0)
		{
			// Tell system that this side can see this object
			setSeenBy(psObj, psViewer->player, val);

			// This looks like some kind of weird hack.
			if(psObj->type != OBJ_FEATURE && psObj->visible[psViewer->player] <= 0)
			{
				// features are not in the cluster system
				clustObjectSeen(psObj, psViewer);
			}
		}
	}
}

/* Find out what can see this object */
// Fade in/out of view. Must be called after calculation of which objects are seen.
void processVisibilityLevel(BASE_OBJECT *psObj)
{
	int player;

	// update the visibility levels
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		bool justBecameVisible = false;
		int visLevel = psObj->seenThisTick[player];

		if (player == psObj->player)
		{
			// owner can always see it fully
			psObj->visible[player] = UBYTE_MAX;
			continue;
		}

		// Droids can vanish from view, other objects will stay
		if (psObj->type != OBJ_DROID)
		{
			visLevel = MAX(visLevel, psObj->visible[player]);
		}

		if (visLevel > psObj->visible[player])
		{
			justBecameVisible = psObj->visible[player] <= 0;

			psObj->visible[player] = MIN(psObj->visible[player] + visLevelInc, visLevel);
		}
		else if(visLevel < psObj->visible[player])
		{
			psObj->visible[player] = MAX(psObj->visible[player] - visLevelDec, visLevel);
		}

		if (justBecameVisible)
		{
			/* Make sure all tiles under a feature/structure become visible when you see it */
			if (psObj->type == OBJ_STRUCTURE || psObj->type == OBJ_FEATURE)
			{
				setUnderTilesVis(psObj, player);
			}

			// if a feature has just become visible set the message blips
			if (psObj->type == OBJ_FEATURE)
			{
				MESSAGE *psMessage;
				INGAME_AUDIO type = NO_SOUND;

				/* If this is an oil resource we want to add a proximity message for
				 * the selected Player - if there isn't an Resource Extractor on it. */
				if (((FEATURE *)psObj)->psStats->subType == FEAT_OIL_RESOURCE && !TileHasStructure(mapTile(map_coord(psObj->pos.x), map_coord(psObj->pos.y))))
				{
					type = ID_SOUND_RESOURCE_HERE;
				}
				else if (((FEATURE *)psObj)->psStats->subType == FEAT_GEN_ARTE)
				{
					type = ID_SOUND_ARTEFACT_DISC;
				}

				if (type != NO_SOUND)
				{
					psMessage = addMessage(MSG_PROXIMITY, true, player);
					if (psMessage)
					{
						psMessage->pViewData = (MSG_VIEWDATA *)psObj;
					}
					if (!bInTutorial && player == selectedPlayer)
					{
						// play message to indicate been seen
						audio_QueueTrackPos(type, psObj->pos.x, psObj->pos.y, psObj->pos.z);
					}
					debug(LOG_MSG, "Added message for oil well or artefact, pViewData=%p", psMessage->pViewData);
				}
			}
		}
	}
}

void processVisibility()
{
	int player;
	for (player = 0; player < MAX_PLAYERS; ++player)
	{
		BASE_OBJECT *lists[] = {apsDroidLists[player], apsStructLists[player], apsFeatureLists[player]};
		unsigned list;
		for (list = 0; list < sizeof(lists)/sizeof(*lists); ++list)
		{
			BASE_OBJECT *psObj;
			for (psObj = lists[list]; psObj != NULL; psObj = psObj->psNext)
			{
				processVisibilitySelf(psObj);
			}
		}
	}
	for (player = 0; player < MAX_PLAYERS; ++player)
	{
		BASE_OBJECT *lists[] = {apsDroidLists[player], apsStructLists[player]};
		unsigned list;
		for (list = 0; list < sizeof(lists)/sizeof(*lists); ++list)
		{
			BASE_OBJECT *psObj;
			for (psObj = lists[list]; psObj != NULL; psObj = psObj->psNext)
			{
				processVisibilityVision(psObj);
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
FEATURE_STATS const *psStats;
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

	for (i = 0; i < width + 1; i++)  // + 1 because visibility is for top left of tile.
	{
		for (j = 0; j < breadth + 1; j++)  // + 1 because visibility is for top left of tile.
		{
			psTile = mapTile(mapX+i,mapY+j);
			if (psTile)
			{
				psTile->tileExploredBits |= alliancebits[player];
			}
		}
	}
}

//forward declaration
static int checkFireLine(const SIMPLE_OBJECT* psViewer, const BASE_OBJECT* psTarget, int weapon_slot, bool wallsBlock, bool direct);

/**
 * Check whether psViewer can fire directly at psTarget.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 */
bool lineOfFire(const SIMPLE_OBJECT* psViewer, const BASE_OBJECT* psTarget, int weapon_slot, bool wallsBlock)
{
	WEAPON_STATS *psStats = NULL;

	ASSERT_OR_RETURN(false, psViewer != NULL, "Invalid shooter pointer!");
	ASSERT_OR_RETURN(false, psTarget != NULL, "Invalid target pointer!");

	if (psViewer->type == OBJ_DROID)
	{
		psStats = asWeaponStats + ((DROID*)psViewer)->asWeaps[weapon_slot].nStat;
	}
	else if (psViewer->type == OBJ_STRUCTURE)
	{
		psStats = asWeaponStats + ((STRUCTURE*)psViewer)->asWeaps[weapon_slot].nStat;
	}
	// 2d distance
	int distance = iHypot(removeZ(psTarget->pos - psViewer->pos));
	int range = proj_GetLongRange(psStats);
	if (proj_Direct(psStats))
	{
		/** direct shots could collide with ground **/
		return range >= distance && LINE_OF_FIRE_MINIMUM <= checkFireLine(psViewer, psTarget, weapon_slot, wallsBlock, true);
	}
	else
	{
		/** 
		 * indirect shots always have a line of fire, IF the forced 
		 * minimum angle doesn't move it out of range
		 **/
		int min_angle = checkFireLine(psViewer, psTarget, weapon_slot, wallsBlock, false);
		// NOTE This code seems similar to the code in combFire in combat.cpp.
		if (min_angle > DEG(PROJ_MAX_PITCH))
		{
			if (iSin(2*min_angle) < iSin(2*DEG(PROJ_MAX_PITCH)))
			{
				range = (range * iSin(2*min_angle)) / iSin(2*DEG(PROJ_MAX_PITCH));
			}
		}
		return range >= distance;
	}
}

/* Check how much of psTarget is hitable from psViewer's gun position */
int areaOfFire(const SIMPLE_OBJECT* psViewer, const BASE_OBJECT* psTarget, int weapon_slot, bool wallsBlock)
{
	if (psViewer == NULL)
	{
		return 0;  // Lassat special case, avoid assertion.
	}

	return checkFireLine(psViewer, psTarget, weapon_slot, wallsBlock, true);
}

/* Check the minimum angle to hitpsTarget from psViewer via indirect shots */
int arcOfFire(const SIMPLE_OBJECT* psViewer, const BASE_OBJECT* psTarget, int weapon_slot, bool wallsBlock)
{
	return checkFireLine(psViewer, psTarget, weapon_slot, wallsBlock, false);
}

/* helper function for checkFireLine */
static inline void angle_check(int64_t* angletan, int positionSq, int height, int distanceSq, int targetHeight, bool direct)
{
	int64_t current;
	if (direct)
	{
		current = (65536*height)/iSqrt(positionSq);
	}
	else
	{
		int dist = iSqrt(distanceSq);
		int pos = iSqrt(positionSq);
		current = (pos*targetHeight) / dist;
		if (current < height && pos > TILE_UNITS/2 && pos<dist-TILE_UNITS/2)
		{
			// solve the following trajectory parabolic equation
			// ( targetHeight ) = a * distance^2 + factor * distance
			// ( height ) = a * position^2 + factor * position
			//  "a" depends on angle, gravity and shooting speed.
			//  luckily we dont need it for this at all, since
			// factor = tan(firing_angle)
			current = ((int64_t)65536*((int64_t)distanceSq * (int64_t)height -  (int64_t)positionSq * (int64_t)targetHeight))
			        / ((int64_t)distanceSq * (int64_t)pos - (int64_t)dist * (int64_t)positionSq);
		}
		else
		{
			current = 0;
		}
	}
	*angletan = std::max(*angletan, current);
}

/**
 * Check fire line from psViewer to psTarget
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 */
static int checkFireLine(const SIMPLE_OBJECT* psViewer, const BASE_OBJECT* psTarget, int weapon_slot, bool wallsBlock, bool direct)
{
	Vector3i pos, dest;
	Vector2i start,diff, current, halfway, next, part;
	Vector3i muzzle;
	int distSq, partSq, oldPartSq;
	int64_t angletan;

	ASSERT(psViewer != NULL, "Invalid shooter pointer!");
	ASSERT(psTarget != NULL, "Invalid target pointer!");
	if (!psViewer || !psTarget)
	{
		return -1;
	}
	
	/* CorvusCorax: get muzzle offset (code from projectile.c)*/
	if (psViewer->type == OBJ_DROID && weapon_slot >= 0)
	{
		calcDroidMuzzleBaseLocation((DROID *)psViewer, &muzzle, weapon_slot);
	}
	else if (psViewer->type == OBJ_STRUCTURE && weapon_slot >= 0)
	{
		calcStructureMuzzleBaseLocation((STRUCTURE *)psViewer, &muzzle, weapon_slot);
	}
	else // incase anything wants a projectile
	{
		muzzle = psViewer->pos;
	}

	pos = muzzle;
	dest = psTarget->pos;
	diff = removeZ(dest - pos);

	distSq = diff*diff;
	if (distSq == 0)
	{
		// Should never be on top of each other, but ...
		return 1000;
	}

	current = removeZ(pos);
	start = current;
	angletan = -1000*65536;
	partSq = 0;
	// run a manual trace along the line of fire until target is reached
	while (partSq < distSq)
	{
		bool hasSplitIntersection;

		oldPartSq = partSq;
		
		if (partSq > 0)
		{
			angle_check(&angletan, partSq, map_Height(current) - pos.z, distSq, dest.z - pos.z, direct);
		}

		// intersect current tile with line of fire
		next = diff;
		hasSplitIntersection = map_Intersect(&current.x, &current.y, &next.x, &next.y, &halfway.x, &halfway.y);
		
		if (hasSplitIntersection)
		{
			// check whether target was reached before tile split line:
			part = halfway - start;
			partSq = part*part;

			if (partSq >= distSq)
			{
				break;
			}

			if (partSq > 0)
			{
				angle_check(&angletan, partSq, map_Height(halfway) - pos.z, distSq, dest.z - pos.z, direct);
			}
		}

		// check for walls and other structures
		// TODO: if there is a structure on the same tile as the shooter (and the shooter is not that structure) check if LOF is blocked by it.
		if (wallsBlock && oldPartSq > 0)
		{
			const MAPTILE *psTile;
			halfway = current + (next - current)/2;
			psTile = mapTile(map_coord(halfway.x), map_coord(halfway.y));
			if (TileHasStructure(psTile) && psTile->psObject!=psTarget)
			{
				// check whether target was reached before tile's "half way" line
				part = halfway - start;
				partSq = part*part;

				if (partSq >= distSq)
				{
					break;
				}

				// allowed to shoot over enemy structures if they are NOT the target
				if (partSq>0)
				{
					angle_check(&angletan, oldPartSq,
					            psTile->psObject->pos.z + establishTargetHeight(psTile->psObject) - pos.z,
					            distSq, dest.z - pos.z, direct);
				}
			}
		}

		// next
		current=next;
		part = current - start;
		partSq = part*part;
		ASSERT(partSq > oldPartSq, "areaOfFire(): no progress in tile-walk! From: %i,%i to %i,%i stuck in %i,%i", map_coord(pos.x), map_coord(pos.y), map_coord(dest.x), map_coord(dest.y), map_coord(current.x), map_coord(current.y));

	}
	if (direct)
	{
		return establishTargetHeight(psTarget) - (pos.z + (angletan * iSqrt(distSq))/65536 - dest.z);
	}
	else
	{
		angletan = iAtan2(angletan, 65536);
		angletan = angleDelta(angletan);
		return DEG(1) + angletan;
	}

}

void objSensorCache(BASE_OBJECT *psObj, SENSOR_STATS *psSensor)
{
	if (psSensor)
	{
		psObj->sensorRange = sensorRange(psSensor, psObj->player);
	}
	else if (psObj->type == OBJ_DROID || psObj->type == OBJ_STRUCTURE)
	{
		// Give them the default sensor if not
		psObj->sensorRange = sensorRange(asSensorStats + aDefaultSensor[psObj->player], psObj->player);
	}
	else
	{
		psObj->sensorRange = 0;
	}
}

void objEcmCache(BASE_OBJECT *psObj, ECM_STATS *psEcm)
{
	if (psEcm)
	{
		psObj->ECMMod = ecmRange(psEcm, psObj->player);
	}
	else
	{
		psObj->ECMMod = 0;
	}
}
