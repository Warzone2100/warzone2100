/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include "lib/framework/fixedpoint.h"

#include "lib/gamelib/gtime.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/ivis_opengl/ivisdef.h"

#include "visibility.h"

#include "objects.h"
#include "map.h"
#include "loop.h"
#include "raycast.h"
#include "geometry.h"
#include "hci.h"
#include "mapgrid.h"
#include "research.h"
#include "structure.h"
#include "projectile.h"
#include "display.h"
#include "multiplay.h"
#include "qtscript.h"
#include "wavecast.h"

// accuracy for the height gradient
#define GRAD_MUL 10000

// rate to change visibility level
static const int VIS_LEVEL_INC = 255 * 2;
static const int VIS_LEVEL_DEC = 50;

// integer amount to change visibility this turn
static SDWORD			visLevelInc, visLevelDec;

class SPOTTER
{
public:
	SPOTTER(int x, int y, int plr, int radius, int type, uint32_t expiry = 0)
		: pos(x, y, 0), player(plr), sensorRadius(radius), sensorType(type), expiryTime(expiry), numWatchedTiles(0), watchedTiles(nullptr)
	{
		id = generateSynchronisedObjectId();
	}
	~SPOTTER();

	Position pos;
	int player;
	int sensorRadius;
	int sensorType; // 0 - vision, 1 - radar
	uint32_t expiryTime; // when to self-destruct, zero if never
	int numWatchedTiles;
	TILEPOS *watchedTiles;
	uint32_t id;
};
static std::vector<SPOTTER *> apsInvisibleViewers;

#define MIN_VIS_HEIGHT 80

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

static int *gNumWalls = nullptr;
static Vector2i *gWall = nullptr;

// forward declarations
static void setSeenBy(BASE_OBJECT *psObj, unsigned viewer, int val);

// initialise the visibility stuff
bool visInitialise()
{
	visLevelInc = 1;
	visLevelDec = 0;

	return true;
}

// update the visibility change levels
void visUpdateLevel()
{
	visLevelInc = gameTimeAdjustedAverage(VIS_LEVEL_INC);
	visLevelDec = gameTimeAdjustedAverage(VIS_LEVEL_DEC);
}

static inline void updateTileVis(MAPTILE *psTile)
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		/// The definition of whether a player can see something on a given tile or not
		if (psTile->watchers[i] > 0 || (psTile->sensors[i] > 0 && !(psTile->jammerBits & ~alliancebits[i])))
		{
			psTile->sensorBits |= (1 << i);         // mark it as being seen
		}
		else
		{
			psTile->sensorBits &= ~(1 << i);        // mark as hidden
		}
	}
}

uint32_t addSpotter(int x, int y, int player, int radius, bool radar, uint32_t expiry)
{
	SPOTTER *psSpot = new SPOTTER(x, y, player, radius, (int)radar, expiry);
	size_t size;
	const WavecastTile *tiles = getWavecastTable(radius, &size);
	psSpot->watchedTiles = (TILEPOS *)malloc(size * sizeof(*psSpot->watchedTiles));
	for (unsigned i = 0; i < size; ++i)
	{
		const int mapX = x + tiles[i].dx;
		const int mapY = y + tiles[i].dy;
		if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight)
		{
			continue;
		}
		MAPTILE *psTile = mapTile(mapX, mapY);
		psTile->tileExploredBits |= alliancebits[player];
		uint8_t *visionType = (!radar) ? psTile->watchers : psTile->sensors;
		if (visionType[player] < UBYTE_MAX)
		{
			TILEPOS tilePos = {uint8_t(mapX), uint8_t(mapY), uint8_t(radar)};
			visionType[player]++;          // we observe this tile
			updateTileVis(psTile);
			psSpot->watchedTiles[psSpot->numWatchedTiles++] = tilePos;    // record having seen it
		}
	}
	apsInvisibleViewers.push_back(psSpot);
	return psSpot->id;
}

bool removeSpotter(uint32_t id)
{
	for (unsigned i = 0; i < apsInvisibleViewers.size(); i++)
	{
		SPOTTER *psSpot = apsInvisibleViewers.at(i);
		if (psSpot->id == id)
		{
			delete psSpot;
			apsInvisibleViewers.erase(apsInvisibleViewers.begin() + i);
			return true;
		}
	}
	return false;
}

void removeSpotters()
{
	while (!apsInvisibleViewers.empty())
	{
		SPOTTER *psSpot = apsInvisibleViewers.back();
		delete psSpot;
		apsInvisibleViewers.pop_back();
	}
}

static void updateSpotters()
{
	static GridList gridList;  // static to avoid allocations.
	for (unsigned i = 0; i < apsInvisibleViewers.size(); i++)
	{
		SPOTTER *psSpot = apsInvisibleViewers.at(i);
		if (psSpot->expiryTime != 0 && psSpot->expiryTime < gameTime)
		{
			delete psSpot;
			apsInvisibleViewers.erase(apsInvisibleViewers.begin() + i);
			continue;
		}
		// else, ie if not expired, show objects around it
		gridList = gridStartIterateUnseen(world_coord(psSpot->pos.x), world_coord(psSpot->pos.y), psSpot->sensorRadius, psSpot->player);
		for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
		{
			BASE_OBJECT *psObj = *gi;

			// Tell system that this side can see this object
			setSeenBy(psObj, psSpot->player, UBYTE_MAX);
		}
	}
}

SPOTTER::~SPOTTER()
{
	for (int i = 0; i < numWatchedTiles; i++)
	{
		const TILEPOS tilePos = watchedTiles[i];
		MAPTILE *psTile = mapTile(tilePos.x, tilePos.y);
		uint8_t *visionType = (tilePos.type == 0) ? psTile->watchers : psTile->sensors;
		ASSERT(visionType[player] > 0, "Not watching watched tile (%d, %d)", (int)tilePos.x, (int)tilePos.y);
		visionType[player]--;
		updateTileVis(psTile);
	}
	free(watchedTiles);
}

/* Record all tiles that some object confers visibility to. Only record each tile
 * once. Note that there is both a limit to how many objects can watch any given
 * tile. Strange but non fatal things will happen if these limits are exceeded. */
static inline void visMarkTile(const BASE_OBJECT *psObj, int mapX, int mapY, MAPTILE *psTile, std::vector<TILEPOS> &watchedTiles)
{
	const int rayPlayer = psObj->player;
	const int xdiff = map_coord(psObj->pos.x) - mapX;
	const int ydiff = map_coord(psObj->pos.y) - mapY;
	const int distSq = xdiff * xdiff + ydiff * ydiff;
	const bool inRange = (distSq < 16);
	uint8_t *visionType = inRange ? psTile->watchers : psTile->sensors;

	if (visionType[rayPlayer] < UBYTE_MAX)
	{
		TILEPOS tilePos = {uint8_t(mapX), uint8_t(mapY), uint8_t(inRange)};

		visionType[rayPlayer]++;                        // we observe this tile
		if (psObj->flags.test(OBJECT_FLAG_JAMMED_TILES))   // we are a jammer object
		{
			psTile->jammers[rayPlayer]++;
			psTile->jammerBits |= (1 << rayPlayer); // mark it as being jammed
		}
		updateTileVis(psTile);
		watchedTiles.push_back(tilePos);  // record having seen it
	}
}

/* The terrain revealing ray callback */
static void doWaveTerrain(BASE_OBJECT *psObj)
{
	const int sx = psObj->pos.x;
	const int sy = psObj->pos.y;
	const int sz = psObj->pos.z + MAX(MIN_VIS_HEIGHT, psObj->sDisplay.imd->max.y);
	const unsigned radius = objSensorRange(psObj);
	const int rayPlayer = psObj->player;
	size_t size;
	const WavecastTile *tiles = getWavecastTable(radius, &size);
#define MAX_WAVECAST_LIST_SIZE 1360  // Trivial upper bound to what a fully upgraded WSS can use (its number of angles). Should probably be some factor times the maximum possible radius. Is probably a lot more than needed. Tested to need at least 180.
	int heights[2][MAX_WAVECAST_LIST_SIZE];
	size_t angles[2][MAX_WAVECAST_LIST_SIZE + 1];
	int readListSize = 0, readListPos = 0, writeListPos = 0;  // readListSize, readListPos dummy initialisations.
	int readList = 0;  // Reading from this list, writing to the other. Could also initialise to rand()%2.
	int lastHeight = 0;  // lastHeight dummy initialisation.
	size_t lastAngle = std::numeric_limits<size_t>::max();

	// Start with full vision of all angles. (If someday wanting to make droids that can only look in one direction, change here, after getting the original angle values saved in the wavecast table.)
	heights[!readList][writeListPos] = -0x7FFFFFFF - 1; // Smallest integer.
	angles[!readList][writeListPos] = 0;               // Smallest angle.
	++writeListPos;

	psObj->watchedTiles.clear();
	for (size_t i = 0; i < size; ++i)
	{
		const int mapX = map_coord(sx) + tiles[i].dx;
		const int mapY = map_coord(sy) + tiles[i].dy;
		if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight)
		{
			continue;
		}

		MAPTILE *psTile = mapTile(mapX, mapY);
		int tileHeight = std::max(psTile->height, psTile->waterLevel);  // If we can see the water surface, then let us see water-covered tiles too.
		int perspectiveHeight = (tileHeight - sz) * tiles[i].invRadius;
		int perspectiveHeightLeeway = (tileHeight - sz + MIN_VIS_HEIGHT) * tiles[i].invRadius;

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

		bool seen = false;
		while (angles[readList][readListPos] < tiles[i].angEnd && readListPos < readListSize)
		{
			int oldHeight = heights[readList][readListPos];
			int newHeight = MAX(oldHeight, perspectiveHeight);
			seen = seen || perspectiveHeightLeeway >= oldHeight; // consider point slightly above ground in case there is something on the tile
			if (newHeight != lastHeight)
			{
				heights[!readList][writeListPos] = newHeight;
				angles[!readList][writeListPos] = MAX(angles[readList][readListPos], tiles[i].angBegin);
				lastHeight = newHeight;
				++writeListPos;
				ASSERT_OR_RETURN(, writeListPos <= MAX_WAVECAST_LIST_SIZE, "Visibility too complicated! Need to increase MAX_WAVECAST_LIST_SIZE.");
			}
			++readListPos;
		}
		--readListPos;

		if (seen)
		{
			// Can see this tile.
			psTile->tileExploredBits |= alliancebits[rayPlayer];                        // Share exploration with allies too
			visMarkTile(psObj, mapX, mapY, psTile, psObj->watchedTiles);   // Mark this tile as seen by our sensor
		}
	}
}

/* The los ray callback */
static bool rayLOSCallback(Vector2i pos, int32_t dist, void *data)
{
	VisibleObjectHelp_t *help = (VisibleObjectHelp_t *)data;

	ASSERT(pos.x >= 0 && pos.x < world_coord(mapWidth) && pos.y >= 0 && pos.y < world_coord(mapHeight), "rayLOSCallback: coords off map");

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

	if (help->wallsBlock)
	{
		// Store the height at this tile for next time round
		Vector2i tile = map_coord(pos.xy());

		if (tile != help->final)
		{
			MAPTILE *psTile = mapTile(tile);
			if (TileHasWall(psTile) && !TileHasSmallStructure(psTile))
			{
				STRUCTURE *psStruct = (STRUCTURE *)psTile->psObject;
				if (psStruct->pStructureType->type != REF_GATE || psStruct->state != SAS_OPEN)
				{
					help->lastHeight = 2 * UBYTE_MAX * ELEVATION_SCALE;
					help->wall = pos.xy();
					help->numWalls++;
				}
			}
		}
	}

	return true;
}


/* Remove tile visibility from object */
void visRemoveVisibility(BASE_OBJECT *psObj)
{
	if (mapWidth && mapHeight)
	{
		for (TILEPOS pos : psObj->watchedTiles)
		{
			// FIXME: the mapTile might have been swapped out, see swapMissionPointers()
			MAPTILE *psTile = mapTile(pos.x, pos.y);

			ASSERT(pos.type < 2, "Invalid visibility type %d", (int)pos.type);
			uint8_t *visionType = (pos.type == 0) ? psTile->sensors : psTile->watchers;
			if (visionType[psObj->player] == 0 && game.type == LEVEL_TYPE::CAMPAIGN)	// hack
			{
				continue;
			}
			ASSERT(visionType[psObj->player] > 0, "No %s on watched tile (%d, %d)", pos.type ? "radar" : "vision", (int)pos.x, (int)pos.y);
			visionType[psObj->player]--;
			if (psObj->flags.test(OBJECT_FLAG_JAMMED_TILES))  // we are a jammer object — we cannot check objJammerPower(psObj) > 0 directly here, we may be in the BASE_OBJECT destructor).
			{
				// No jammers in campaign, no need for special hack
				ASSERT(psTile->jammers[psObj->player] > 0, "Not jamming watched tile (%d, %d)", (int)pos.x, (int)pos.y);
				psTile->jammers[psObj->player]--;
				if (psTile->jammers[psObj->player] == 0)
				{
					psTile->jammerBits &= ~(1 << psObj->player);
				}
			}
			updateTileVis(psTile);
		}
	}
	psObj->watchedTiles.clear();
	psObj->flags.set(OBJECT_FLAG_JAMMED_TILES, false);
}

void visRemoveVisibilityOffWorld(BASE_OBJECT *psObj)
{
	psObj->watchedTiles.clear();
}

/* Check which tiles can be seen by an object */
void visTilesUpdate(BASE_OBJECT *psObj)
{
	ASSERT(psObj->type != OBJ_FEATURE, "visTilesUpdate: visibility updates are not for features!");

	// Remove previous map visibility provided by object
	visRemoveVisibility(psObj);

	if (psObj->type == OBJ_STRUCTURE)
	{
		STRUCTURE *psStruct = (STRUCTURE *)psObj;
		if (psStruct->status != SS_BUILT ||
		    psStruct->pStructureType->type == REF_WALL || psStruct->pStructureType->type == REF_WALLCORNER || psStruct->pStructureType->type == REF_GATE)
		{
			// unbuilt structures and walls do not confer visibility.
			return;
		}
	}

	// Do the whole circle in ∞ steps. No more pretty moiré patterns.
	psObj->flags.set(OBJECT_FLAG_JAMMED_TILES, objJammerPower(psObj) > 0);
	doWaveTerrain(psObj);
}

/*reveals all the terrain in the map*/
void revealAll(UBYTE player)
{
	SDWORD   i, j;
	MAPTILE	*psTile;

	//reveal all tiles
	for (i = 0; i < mapWidth; i++)
	{
		for (j = 0; j < mapHeight; j++)
		{
			psTile = mapTile(i, j);
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
int visibleObject(const BASE_OBJECT *psViewer, const BASE_OBJECT *psTarget, bool wallsBlock)
{
	ASSERT_OR_RETURN(0, psViewer != nullptr, "Invalid viewer pointer!");
	ASSERT_OR_RETURN(0, psTarget != nullptr, "Invalid viewed pointer!");

	int range = objSensorRange(psViewer);

	if (!worldOnMap(psViewer->pos.x, psViewer->pos.y) || !worldOnMap(psTarget->pos.x, psTarget->pos.y))
	{
		//Most likely a VTOL or transporter
		debug(LOG_WARNING, "Trying to view something off map!");
		return 0;
	}

	/* Get the sensor range */
	switch (psViewer->type)
	{
	case OBJ_DROID:
		{
			const DROID *psDroid = (const DROID *)psViewer;

			if (psDroid->order.psObj == psTarget && cbSensorDroid(psDroid))
			{
				// if it is targetted by a counter battery sensor, it is seen
				return UBYTE_MAX;
			}
			break;
		}
	case OBJ_STRUCTURE:
		{
			const STRUCTURE *psStruct = (const STRUCTURE *)psViewer;

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

			if (psTarget->type == OBJ_DROID && isVtolDroid((const DROID *)psTarget)
			    && asWeaponStats[psStruct->asWeaps[0].nStat].surfaceToAir == SHOOT_IN_AIR)
			{
				range = 3 * range / 2;	// increase vision range of AA vs VTOL
			}

			if (psStruct->psTarget[0] == psTarget && (structCBSensor(psStruct) || structVTOLCBSensor(psStruct)))
			{
				// if a unit is targetted by a counter battery sensor
				// it is automatically seen
				return UBYTE_MAX;
			}
			break;
		}
	default:
		ASSERT(false, "Visibility checking is only implemented for units and structures");
		return 0;
		break;
	}

	/* First see if the target is in sensor range */
	const int dist = iHypot((psTarget->pos - psViewer->pos).xy());
	if (dist == 0)
	{
		return UBYTE_MAX;	// Should never be on top of each other, but ...
	}

	const MAPTILE *psTile = mapTile(map_coord(psTarget->pos.x), map_coord(psTarget->pos.y));
	const bool jammed = psTile->jammerBits & ~alliancebits[psViewer->player];

	// Special rule for VTOLs, as they are not affected by ECM
	if (((psTarget->type == OBJ_DROID && isVtolDroid((const DROID *)psTarget))
	     || (psViewer->type == OBJ_DROID && isVtolDroid((const DROID *)psViewer)))
	    && dist < range)
	{
		return UBYTE_MAX;
	}

	// initialise the callback variables
	VisibleObjectHelp_t help = {
		true,
		wallsBlock,
		psViewer->pos.z + map_Height(psViewer->pos.x, psViewer->pos.y),
		map_coord(psTarget->pos.xy()),
		0,
		0,
		-UBYTE_MAX * GRAD_MUL * ELEVATION_SCALE,
		0,
		Vector2i(0, 0)
	};

	// Cast a ray from the viewer to the target
	rayCast(psViewer->pos.xy(), psTarget->pos.xy(), rayLOSCallback, &help);

	if (gWall != nullptr && gNumWalls != nullptr) // Out globals are set
	{
		*gWall = help.wall;
		*gNumWalls = help.numWalls;
	}

	bool tileWatched = psTile->watchers[psViewer->player] > 0;
	bool tileWatchedSensor = psTile->sensors[psViewer->player] > 0;

	// Show objects hidden by ECM jamming with radar blips
	if (jammed)
	{
		if (!tileWatched && tileWatchedSensor)
		{
			return UBYTE_MAX / 2;
		}
	}
	// Show objects that are seen directly
	if (tileWatched || (!jammed && tileWatchedSensor))
	{
		return UBYTE_MAX;
	}
	// Show detected sensors as radar blips
	if (objRadarDetector(psViewer) && objActiveRadar(psTarget) && dist < range * 10)
	{
		return UBYTE_MAX / 2;
	}
	// else not seen
	return 0;
}

// Find the wall that is blocking LOS to a target (if any)
STRUCTURE *visGetBlockingWall(const BASE_OBJECT *psViewer, const BASE_OBJECT *psTarget)
{
	int numWalls = 0;
	Vector2i wall;

	// HACK Using globals to not clutter visibleObject() interface too much
	gNumWalls = &numWalls;
	gWall = &wall;

	visibleObject(psViewer, psTarget, true);

	gNumWalls = nullptr;
	gWall = nullptr;

	// see if there was a wall in the way
	if (numWalls > 0)
	{
		Vector2i tile = map_coord(wall);
		unsigned int player;

		for (player = 0; player < MAX_PLAYERS; player++)
		{
			STRUCTURE *psWall;

			for (psWall = apsStructLists[player]; psWall; psWall = psWall->psNext)
			{
				if (map_coord(psWall->pos) == tile)
				{
					return psWall;
				}
			}
		}
	}
	return nullptr;
}

bool hasSharedVision(unsigned viewer, unsigned ally)
{
	ASSERT_OR_RETURN(false, viewer < MAX_PLAYERS && ally < MAX_PLAYERS, "Bad viewer %u or ally %u.", viewer, ally);

	return viewer == ally || (bMultiPlayer && alliancesSharedVision(game.alliance) && aiCheckAlliances(viewer, ally));
}

static void setSeenBy(BASE_OBJECT *psObj, unsigned viewer, int val /*= UBYTE_MAX*/)
{
	//forward out vision to our allies
	for (int ally = 0; ally < MAX_PLAYERS; ++ally)
	{
		if (hasSharedVision(viewer, ally))
		{
			psObj->seenThisTick[ally] = MAX(psObj->seenThisTick[ally], val);
		}
	}
}

static void setSeenByInstantly(BASE_OBJECT *psObj, unsigned viewer, int val /*= UBYTE_MAX*/)
{
	//forward out vision to our allies
	for (int ally = 0; ally < MAX_PLAYERS; ++ally)
	{
		if (hasSharedVision(viewer, ally))
		{
			psObj->seenThisTick[ally] = MAX(psObj->seenThisTick[ally], val);
			psObj->visible[ally] = MAX(psObj->visible[ally], val);
		}
	}
}

// Calculate which objects we should know about based on alliances and satellite view.
static void processVisibilitySelf(BASE_OBJECT *psObj)
{
	if (psObj->type != OBJ_FEATURE && objSensorRange(psObj) > 0)
	{
		// one can trivially see oneself
		setSeenBy(psObj, psObj->player, UBYTE_MAX);
	}

	// if a player has a SAT_UPLINK structure, or has godMode enabled,
	// they can see everything!
	for (unsigned viewer = 0; viewer < MAX_PLAYERS; viewer++)
	{
		if (getSatUplinkExists(viewer) || (viewer == selectedPlayer && godMode))
		{
			setSeenBy(psObj, viewer, UBYTE_MAX);
		}
	}

	psObj->flags.set(OBJECT_FLAG_TARGETED, false); // Remove any targetting locks from last update.

	// If we're a CB sensor, make our target visible instantly. Although this is actually checking visibility of our target, we do it here anyway.
	STRUCTURE *psStruct = castStructure(psObj);
	// you can always see anything that a CB sensor is targetting
	// Anyone commenting this out again will get a knee capping from John.
	// You have been warned!!
	if (psStruct != nullptr && (structCBSensor(psStruct) || structVTOLCBSensor(psStruct)) && psStruct->psTarget[0] != nullptr)
	{
		setSeenByInstantly(psStruct->psTarget[0], psObj->player, UBYTE_MAX);
	}
	DROID *psDroid = castDroid(psObj);
	if (psDroid != nullptr && psDroid->action == DACTION_OBSERVE && cbSensorDroid(psDroid))
	{
		// Anyone commenting this out will get a knee capping from John.
		// You have been warned!!
		setSeenByInstantly(psDroid->psActionTarget[0], psObj->player, UBYTE_MAX);
	}
}

// Calculate which objects we can see. Better to call after processVisibilitySelf, since that check is cheaper.
static void processVisibilityVision(BASE_OBJECT *psViewer)
{
	if (psViewer->type == OBJ_FEATURE)
	{
		return;
	}

	// get all the objects from the grid the droid is in
	// Will give inconsistent results if hasSharedVision is not an equivalence relation.
	static GridList gridList;  // static to avoid allocations.
	gridList = gridStartIterateUnseen(psViewer->pos.x, psViewer->pos.y, objSensorRange(psViewer), psViewer->player);
	for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
	{
		BASE_OBJECT *psObj = *gi;

		int val = visibleObject(psViewer, psObj, false);

		// If we've got ranged line of sight...
		if (val > 0)
		{
			// Tell system that this side can see this object
			setSeenBy(psObj, psViewer->player, val);

			// Check if scripting system wants to trigger an event for this
			triggerEventSeen(psViewer, psObj);
		}
	}
}

/* Find out what can see this object */
// Fade in/out of view. Must be called after calculation of which objects are seen.
static void processVisibilityLevel(BASE_OBJECT *psObj, bool& addedMessage)
{
	// update the visibility levels
	for (unsigned player = 0; player < MAX_PLAYERS; player++)
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
		else if (visLevel < psObj->visible[player])
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
						psMessage->psObj = psObj;
						debug(LOG_MSG, "Added message for oil well or artefact, pViewData=%p", static_cast<void *>(psMessage->pViewData));
						addedMessage = true;
					}
					if (!bInTutorial && player == selectedPlayer)
					{
						// play message to indicate been seen
						audio_QueueTrackPos(type, psObj->pos.x, psObj->pos.y, psObj->pos.z);
					}
				}
			}
		}
	}
}

void processVisibility()
{
	updateSpotters();
	for (int player = 0; player < MAX_PLAYERS; ++player)
	{
		BASE_OBJECT *lists[] = {apsDroidLists[player], apsStructLists[player], apsFeatureLists[player]};
		unsigned list;
		for (list = 0; list < sizeof(lists) / sizeof(*lists); ++list)
		{
			for (BASE_OBJECT *psObj = lists[list]; psObj != nullptr; psObj = psObj->psNext)
			{
				processVisibilitySelf(psObj);
			}
		}
	}
	for (int player = 0; player < MAX_PLAYERS; ++player)
	{
		BASE_OBJECT *lists[] = {apsDroidLists[player], apsStructLists[player]};
		unsigned list;
		for (list = 0; list < sizeof(lists) / sizeof(*lists); ++list)
		{
			for (BASE_OBJECT *psObj = lists[list]; psObj != nullptr; psObj = psObj->psNext)
			{
				processVisibilityVision(psObj);
			}
		}
	}
	for (BASE_OBJECT *psObj = apsSensorList[0]; psObj != nullptr; psObj = psObj->psNextFunc)
	{
		if (objRadarDetector(psObj))
		{
			for (BASE_OBJECT *psTarget = apsSensorList[0]; psTarget != nullptr; psTarget = psTarget->psNextFunc)
			{
				if (psObj != psTarget && psTarget->visible[psObj->player] < UBYTE_MAX / 2
				    && objActiveRadar(psTarget)
				    && iHypot((psTarget->pos - psObj->pos).xy()) < objSensorRange(psObj) * 10)
				{
					psTarget->visible[psObj->player] = UBYTE_MAX / 2;
				}
			}
		}
	}
	bool addedMessage = false;
	for (int player = 0; player < MAX_PLAYERS; ++player)
	{
		BASE_OBJECT *lists[] = {apsDroidLists[player], apsStructLists[player], apsFeatureLists[player]};
		unsigned list;
		for (list = 0; list < sizeof(lists) / sizeof(*lists); ++list)
		{
			for (BASE_OBJECT *psObj = lists[list]; psObj != nullptr; psObj = psObj->psNext)
			{
				processVisibilityLevel(psObj, addedMessage);
			}
		}
	}
	if (addedMessage)
	{
		jsDebugMessageUpdate();
	}
}

void	setUnderTilesVis(BASE_OBJECT *psObj, UDWORD player)
{
	UDWORD		i, j;
	UDWORD		mapX, mapY, width, breadth;
	FEATURE		*psFeature;
	STRUCTURE	*psStructure;
	FEATURE_STATS const *psStats;
	MAPTILE		*psTile;

	if (psObj->type == OBJ_FEATURE)
	{
		psFeature = (FEATURE *)psObj;
		psStats = psFeature->psStats;
		width = psStats->baseWidth;
		breadth = psStats->baseBreadth;
		mapX = map_coord(psFeature->pos.x - width * TILE_UNITS / 2);
		mapY = map_coord(psFeature->pos.y - breadth * TILE_UNITS / 2);
	}
	else
	{
		/* Must be a structure */
		psStructure = (STRUCTURE *)psObj;
		width = psStructure->pStructureType->baseWidth;
		breadth = psStructure->pStructureType->baseBreadth;
		mapX = map_coord(psStructure->pos.x - width * TILE_UNITS / 2);
		mapY = map_coord(psStructure->pos.y - breadth * TILE_UNITS / 2);
	}

	for (i = 0; i < width + 1; i++)  // + 1 because visibility is for top left of tile.
	{
		for (j = 0; j < breadth + 1; j++)  // + 1 because visibility is for top left of tile.
		{
			psTile = mapTile(mapX + i, mapY + j);
			if (psTile)
			{
				psTile->tileExploredBits |= alliancebits[player];
			}
		}
	}
}

//forward declaration
static int checkFireLine(const SIMPLE_OBJECT *psViewer, const BASE_OBJECT *psTarget, int weapon_slot, bool wallsBlock, bool direct);

/**
 * Check whether psViewer can fire directly at psTarget.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 */
bool lineOfFire(const SIMPLE_OBJECT *psViewer, const BASE_OBJECT *psTarget, int weapon_slot, bool wallsBlock)
{
	WEAPON_STATS *psStats = nullptr;

	ASSERT_OR_RETURN(false, psViewer != nullptr, "Invalid shooter pointer!");
	ASSERT_OR_RETURN(false, psTarget != nullptr, "Invalid target pointer!");
	ASSERT_OR_RETURN(false, psViewer->type == OBJ_DROID || psViewer->type == OBJ_STRUCTURE, "Bad viewer type");

	if (psViewer->type == OBJ_DROID)
	{
		psStats = asWeaponStats + ((const DROID *)psViewer)->asWeaps[weapon_slot].nStat;
	}
	else if (psViewer->type == OBJ_STRUCTURE)
	{
		psStats = asWeaponStats + ((const STRUCTURE *)psViewer)->asWeaps[weapon_slot].nStat;
	}
	// 2d distance
	int distance = iHypot((psTarget->pos - psViewer->pos).xy());
	int range = proj_GetLongRange(psStats, psViewer->player);
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
			if (iSin(2 * min_angle) < iSin(2 * DEG(PROJ_MAX_PITCH)))
			{
				range = (range * iSin(2 * min_angle)) / iSin(2 * DEG(PROJ_MAX_PITCH));
			}
		}
		return range >= distance;
	}
}

/* Check how much of psTarget is hitable from psViewer's gun position */
int areaOfFire(const SIMPLE_OBJECT *psViewer, const BASE_OBJECT *psTarget, int weapon_slot, bool wallsBlock)
{
	if (psViewer == nullptr)
	{
		return 0;  // Lassat special case, avoid assertion.
	}

	return checkFireLine(psViewer, psTarget, weapon_slot, wallsBlock, true);
}

/* Check the minimum angle to hitpsTarget from psViewer via indirect shots */
int arcOfFire(const SIMPLE_OBJECT *psViewer, const BASE_OBJECT *psTarget, int weapon_slot, bool wallsBlock)
{
	return checkFireLine(psViewer, psTarget, weapon_slot, wallsBlock, false);
}

/* helper function for checkFireLine */
static inline void angle_check(int64_t *angletan, int positionSq, int height, int distanceSq, int targetHeight, bool direct)
{
	int64_t current;
	if (direct)
	{
		current = (65536 * height) / iSqrt(positionSq);
	}
	else
	{
		int dist = iSqrt(distanceSq);
		int pos = iSqrt(positionSq);
		current = (pos * targetHeight) / dist;
		if (current < height && pos > TILE_UNITS / 2 && pos < dist - TILE_UNITS / 2)
		{
			// solve the following trajectory parabolic equation
			// ( targetHeight ) = a * distance^2 + factor * distance
			// ( height ) = a * position^2 + factor * position
			//  "a" depends on angle, gravity and shooting speed.
			//  luckily we don't need it for this at all, since
			// factor = tan(firing_angle)
			current = ((int64_t)65536 * ((int64_t)distanceSq * (int64_t)height - (int64_t)positionSq * (int64_t)targetHeight))
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
static int checkFireLine(const SIMPLE_OBJECT *psViewer, const BASE_OBJECT *psTarget, int weapon_slot, bool wallsBlock, bool direct)
{
	Vector3i pos(0, 0, 0), dest(0, 0, 0);
	Vector2i start(0, 0), diff(0, 0), current(0, 0), halfway(0, 0), next(0, 0), part(0, 0);
	Vector3i muzzle(0, 0, 0);
	int distSq, partSq, oldPartSq;
	int64_t angletan;

	ASSERT(psViewer != nullptr, "Invalid shooter pointer!");
	ASSERT(psTarget != nullptr, "Invalid target pointer!");
	if (!psViewer || !psTarget)
	{
		return -1;
	}

	/* CorvusCorax: get muzzle offset (code from projectile.c)*/
	if (psViewer->type == OBJ_DROID && weapon_slot >= 0)
	{
		calcDroidMuzzleBaseLocation((const DROID *)psViewer, &muzzle, weapon_slot);
	}
	else if (psViewer->type == OBJ_STRUCTURE && weapon_slot >= 0)
	{
		calcStructureMuzzleBaseLocation((const STRUCTURE *)psViewer, &muzzle, weapon_slot);
	}
	else // incase anything wants a projectile
	{
		muzzle = psViewer->pos;
	}

	pos = muzzle;
	dest = psTarget->pos;
	diff = (dest - pos).xy();

	distSq = dot(diff, diff);
	if (distSq == 0)
	{
		// Should never be on top of each other, but ...
		return 1000;
	}

	current = pos.xy();
	start = current;
	angletan = -1000 * 65536;
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
			partSq = dot(part, part);

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
			halfway = current + (next - current) / 2;
			psTile = mapTile(map_coord(halfway.x), map_coord(halfway.y));
			if (TileHasStructure(psTile) && psTile->psObject != psTarget)
			{
				// check whether target was reached before tile's "half way" line
				part = halfway - start;
				partSq = dot(part, part);

				if (partSq >= distSq)
				{
					break;
				}

				// allowed to shoot over enemy structures if they are NOT the target
				if (partSq > 0)
				{
					angle_check(&angletan, oldPartSq,
					            psTile->psObject->pos.z + establishTargetHeight(psTile->psObject) - pos.z,
					            distSq, dest.z - pos.z, direct);
				}
			}
		}

		// next
		current = next;
		part = current - start;
		partSq = dot(part, part);
		ASSERT(partSq > oldPartSq, "areaOfFire(): no progress in tile-walk! From: %i,%i to %i,%i stuck in %i,%i", map_coord(pos.x), map_coord(pos.y), map_coord(dest.x), map_coord(dest.y), map_coord(current.x), map_coord(current.y));

	}
	if (direct)
	{
		return establishTargetHeight(psTarget) - (pos.z + (angletan * iSqrt(distSq)) / 65536 - dest.z);
	}
	else
	{
		angletan = iAtan2(angletan, 65536);
		angletan = angleDelta(angletan);
		return DEG(1) + angletan;
	}

}
