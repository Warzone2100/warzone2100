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

#include <algorithm>
#include <limits>

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
#include "profiling.h"
#include "perfcounters.h"
#include "game_world.h"
#include "mission.h"

// rate to change visibility level
static const int VIS_LEVEL_INC = 255 * 2;
static const int VIS_LEVEL_DEC = 50;

// integer amount to change visibility this turn
static SDWORD			visLevelInc, visLevelDec;

class SPOTTER
{
public:
	SPOTTER(WorldMapState& mapState, int x, int y, int plr, int radius, int type, uint32_t expiry = 0)
		: pos(x, y, 0), player(plr), sensorRadius(radius), sensorType(type), expiryTime(expiry), numWatchedTiles(0), watchedTiles(nullptr), mapState(mapState)
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
	WorldMapState& mapState;
};
static std::vector<SPOTTER *> apsInvisibleViewers;

#define MIN_VIS_HEIGHT 80

struct VisibleObjectHelp_t
{
	const bool wallsBlock;  // Whether walls block line of sight
	const Vector2i final;   // The final tile of the ray cast
	int numWalls;           // How many walls the LOS has hit
	STRUCTURE *wall;        // The last wall on the LOS, null if it did not sit on its own center tile
};

static int *gNumWalls = nullptr;
static STRUCTURE **gWall = nullptr;

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

static inline void updateTileVis(MAPTILE *psTile, int player)
{
	/// The definition of whether a player can see something on a given tile or not
	if (psTile->watchers[player] > 0 || (psTile->sensors[player] > 0 && !(psTile->jammerBits & ~alliancebits[player])))
	{
		psTile->sensorBits |= (1 << player);         // mark it as being seen
	}
	else
	{
		psTile->sensorBits &= ~(1 << player);        // mark as hidden
	}
}

// Set up the watched-tile list for a freshly-constructed spotter and register it. Shared by
// addSpotter (new script spotter) and spotterRestore (GameState reload).
static void spotterWatchAndRegister(SPOTTER *psSpot, WorldMapState& mapState, int x, int y, int player, int radius, bool radar)
{
	size_t size;
	const WavecastTile *tiles = getWavecastTable(radius, &size);
	psSpot->watchedTiles = (TILEPOS *)malloc(size * sizeof(*psSpot->watchedTiles));
	for (unsigned i = 0; i < size; ++i)
	{
		const int mapX = x + tiles[i].dx;
		const int mapY = y + tiles[i].dy;
		if (mapX < 0 || mapX >= mapState.width || mapY < 0 || mapY >= mapState.height)
		{
			continue;
		}
		MAPTILE *psTile = mapTile(mapState, mapX, mapY);
		psTile->tileExploredBits |= alliancebits[player];
		uint16_t *visionType = (!radar) ? psTile->watchers : psTile->sensors;
		if (visionType[player] < UINT16_MAX)
		{
			TILEPOS tilePos = {uint8_t(mapX), uint8_t(mapY), uint8_t(radar)};
			visionType[player]++;          // we observe this tile
			updateTileVis(psTile, player);
			psSpot->watchedTiles[psSpot->numWatchedTiles++] = tilePos;    // record having seen it
		}
	}
	apsInvisibleViewers.push_back(psSpot);
}

uint32_t addSpotter(WorldMapState& mapState, int x, int y, int player, int radius, bool radar, uint32_t expiry)
{
	ASSERT_OR_RETURN(0, player >= 0 && player < MAX_PLAYERS, "invalid player: %d", player);
	SPOTTER *psSpot = new SPOTTER(mapState, x, y, player, radius, (int)radar, expiry);
	spotterWatchAndRegister(psSpot, mapState, x, y, player, radius, radar);
	return psSpot->id;
}

std::vector<SpotterSaveData> spotterEnumerateForSave()
{
	std::vector<SpotterSaveData> out;
	out.reserve(apsInvisibleViewers.size());
	for (const SPOTTER *psSpot : apsInvisibleViewers)
	{
		out.push_back({ psSpot->pos.x, psSpot->pos.y, psSpot->player, psSpot->sensorRadius,
		                psSpot->sensorType, psSpot->expiryTime, psSpot->id });
	}
	return out;
}

void spotterRestore(WorldMapState& mapState, const SpotterSaveData& d)
{
	ASSERT_OR_RETURN(, d.player >= 0 && d.player < MAX_PLAYERS, "invalid player: %d", d.player);
	SPOTTER *psSpot = new SPOTTER(mapState, d.x, d.y, d.player, d.sensorRadius, d.sensorType, d.expiryTime);
	psSpot->id = d.id;  // keep the saved synchronised id; the ctor's id-bump is undone by counters-restored-last
	spotterWatchAndRegister(psSpot, mapState, d.x, d.y, d.player, d.sensorRadius, d.sensorType != 0);
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
		for (BASE_OBJECT *psObj : gridStartIterateUnseen(world_coord(psSpot->pos.x), world_coord(psSpot->pos.y), psSpot->sensorRadius, psSpot->player))
		{
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
		MAPTILE *psTile = mapTile(mapState, tilePos.x, tilePos.y);
		uint16_t *visionType = (tilePos.type == 0) ? psTile->watchers : psTile->sensors;
		ASSERT(visionType[player] > 0, "Not watching watched tile (%d, %d)", (int)tilePos.x, (int)tilePos.y);
		visionType[player]--;
		updateTileVis(psTile, player);
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
	uint16_t *visionType = inRange ? psTile->watchers : psTile->sensors;

	if (visionType[rayPlayer] < UINT16_MAX)
	{
		TILEPOS tilePos = {uint8_t(mapX), uint8_t(mapY), uint8_t(inRange)};

		visionType[rayPlayer]++;                        // we observe this tile
		if (psObj->flags.test(OBJECT_FLAG_JAMMED_TILES))   // we are a jammer object
		{
			psTile->jammers[rayPlayer]++;
			psTile->jammerBits |= (1 << rayPlayer); // mark it as being jammed
		}
		updateTileVis(psTile, rayPlayer);
		watchedTiles.push_back(tilePos);  // record having seen it
	}
}

/// The height of the bands that lift-propulsion droids' vision height is taken from.
static const int VIS_LIFT_HEIGHT_BAND = 64;

static bool usesLiftVisionHeight(const DROID *psDroid)
{
	return psDroid->getPropulsionStats()->propulsionType == PROPULSION_TYPE_LIFT;
}

int visLiftHeightBand(const DROID *psDroid)
{
	// Floor division, so a band is VIS_LIFT_HEIGHT_BAND high below 0 too
	const int z = psDroid->pos.z;
	return (z >= 0) ? z / VIS_LIFT_HEIGHT_BAND : -((-z + VIS_LIFT_HEIGHT_BAND - 1) / VIS_LIFT_HEIGHT_BAND);
}

/// The height an object's tile coverage is computed from (before adding its model's height).
///
/// The coverage is only recomputed on some events (a droid entering a tile, a structure being completed, ...),
/// and has to match a recompute from the object's current state at any tick boundary (all a loaded game can do).
/// So it can't use the exact height at the time of the event, which keeps changing afterwards:
/// - a ground droid sees from the terrain height at the center of its tile
/// - a lift-propulsion droid (flying or landed) sees from the middle of its current height band (visOriginBand),
///   and is recomputed whenever it changes band (moveUpdateDroid)
/// - a structure sees from its own height, which only changes when the terrain under it does
static int visOriginHeight(BASE_OBJECT *psObj, const WorldMapState& mapState)
{
	if (psObj->type == OBJ_DROID)
	{
		DROID *psDroid = static_cast<DROID *>(psObj);
		if (usesLiftVisionHeight(psDroid))
		{
			psDroid->visOriginBand = visLiftHeightBand(psDroid);
			return psDroid->visOriginBand * VIS_LIFT_HEIGHT_BAND + VIS_LIFT_HEIGHT_BAND / 2;
		}
		if (worldOnMap(mapState, psObj->pos.x, psObj->pos.y))
		{
			return map_Height(mapState, world_coord(map_coord(psObj->pos.x)) + TILE_UNITS / 2, world_coord(map_coord(psObj->pos.y)) + TILE_UNITS / 2);
		}
	}
	return psObj->pos.z;
}

/* The terrain revealing ray callback */
static void doWaveTerrain(BASE_OBJECT *psObj, WorldMapState& mapState)
{
	if (psObj == nullptr)
	{
		return;
	}

	const int sx = psObj->pos.x;
	const int sy = psObj->pos.y;
	// A structure getting a module sees from its completed part (the module's own model is in sDisplay.imd meanwhile)
	const iIMDBaseShape *visImd = psObj->sDisplay.imd;
	if (psObj->type == OBJ_STRUCTURE && static_cast<const STRUCTURE *>(psObj)->prebuiltImd != nullptr)
	{
		visImd = static_cast<const STRUCTURE *>(psObj)->prebuiltImd;
	}
	const int sz = visOriginHeight(psObj, mapState) + ((visImd != nullptr) ? MAX(MIN_VIS_HEIGHT, visImd->max.y) : MIN_VIS_HEIGHT);
	const unsigned radius = objSensorRange(psObj);
	const int rayPlayer = psObj->player;
	size_t size;
	const WavecastTile *tiles = getWavecastTable(radius, &size);
#define MAX_WAVECAST_LIST_SIZE 1360  // Trivial upper bound to what a fully upgraded WSS can use (its number of angles). Should probably be some factor times the maximum possible radius. Is probably a lot more than needed. Tested to need at least 180.
	int heights[2][MAX_WAVECAST_LIST_SIZE + 1];
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
		if (mapX < 0 || mapX >= mapState.width || mapY < 0 || mapY >= mapState.height)
		{
			continue;
		}

		MAPTILE *psTile = mapTile(mapState, mapX, mapY);
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
static bool rayLOSCallback(WorldMapState& mapState, Vector2i pos, int32_t dist, void *data)
{
	VisibleObjectHelp_t *help = (VisibleObjectHelp_t *)data;

	ASSERT(pos.x >= 0 && pos.x < world_coord(mapState.width) && pos.y >= 0 && pos.y < world_coord(mapState.height), "rayLOSCallback: coords off map");

	if (help->wallsBlock)
	{
		Vector2i tile = map_coord(pos.xy());

		if (tile != help->final)
		{
			MAPTILE *psTile = mapTile(mapState, tile);
			if (TileHasWall_raycast(psTile) && !TileHasSmallStructure(psTile))
			{
				STRUCTURE *psStruct = (STRUCTURE *)psTile->psObject;
				if (psStruct->pStructureType->type != REF_GATE || psStruct->state != SAS_OPEN)
				{
					// A structure occupies every tile it covers, but only its centre tile carries its pos,
					// so a hit on any other tile of a multi-tile one reports no wall.
					help->wall = map_coord(psStruct->pos.xy()) == tile ? psStruct : nullptr;
					help->numWalls++;
				}
			}
		}
	}

	return true;
}


/* Remove tile visibility from object */
void visRemoveVisibility(BASE_OBJECT *psObj, WorldMapState& mapState)
{
	if (mapState.width && mapState.height)
	{
		for (TILEPOS pos : psObj->watchedTiles)
		{
			MAPTILE *psTile = mapTile(mapState, pos.x, pos.y);

			ASSERT(pos.type < 2, "Invalid visibility type %d", (int)pos.type);
			uint16_t *visionType = (pos.type == 0) ? psTile->sensors : psTile->watchers;
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
			updateTileVis(psTile, psObj->player);
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
void visTilesUpdate(BASE_OBJECT *psObj, WorldMapState& mapState)
{
	ASSERT(psObj->type != OBJ_FEATURE, "visTilesUpdate: visibility updates are not for features!");

	// Remove previous map visibility provided by object
	visRemoveVisibility(psObj, mapState);

	if (psObj->type == OBJ_DROID)
	{
		const DROID *psDroid = static_cast<const DROID *>(psObj);
		if (psDroid->isTransporter() && psDroid->action == DACTION_NONE
		    && (psDroid->order.type == DORDER_TRANSPORTRETURN || psDroid->order.type == DORDER_TRANSPORTOUT))
		{
			// A transporter that has reached the map edge on its way out: updateTransporter dropped its vision there,
			// and the order update moves it off world in the next tick.
			return;
		}
	}
	if (psObj->type == OBJ_STRUCTURE)
	{
		STRUCTURE *psStruct = (STRUCTURE *)psObj;
		// A structure getting a module (prebuiltImd set) keeps the vision of its completed part.
		if ((psStruct->status != SS_BUILT && psStruct->prebuiltImd == nullptr) ||
		    psStruct->pStructureType->type == REF_WALL || psStruct->pStructureType->type == REF_WALLCORNER || psStruct->pStructureType->type == REF_GATE)
		{
			// unbuilt structures and walls do not confer visibility.
			return;
		}
	}

	// Do the whole circle in ∞ steps. No more pretty moiré patterns.
	psObj->flags.set(OBJECT_FLAG_JAMMED_TILES, objJammerPower(psObj) > 0);
	doWaveTerrain(psObj, mapState);
}

/*reveals all the terrain in the map*/
void revealAll(WorldMapState& mapState, UBYTE player)
{
	SDWORD   i, j;
	MAPTILE	*psTile;

	if (player >= MAX_PLAYERS)
	{
		return;
	}

	//reveal all tiles
	for (j = 0; j < mapState.height; j++)
	{
		for (i = 0; i < mapState.width; i++)
		{
			psTile = mapTile(mapState, i, j);
			psTile->tileExploredBits |= alliancebits[player];
		}
	}

	//the objects gets revealed in processVisibility()
}

/* Check whether psViewer can see psTarget.
 * psViewer should be an object that has some form of sensor,
 * currently droids and structures.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 * wallsBlock controls whether structures block LOS
 */
int visibleObject(const BASE_OBJECT *psViewer, const BASE_OBJECT *psTarget, bool wallsBlock)
{
	WZ_PERF_SCOPE(T_visibleObject);
	ASSERT_OR_RETURN(0, psViewer != nullptr, "Invalid viewer pointer!");
	ASSERT_OR_RETURN(0, psTarget != nullptr, "Invalid viewed pointer!");

	int range = objSensorRange(psViewer);

	if (!worldOnMap(gameWorld.map, psViewer->pos.x, psViewer->pos.y) || !worldOnMap(gameWorld.map, psTarget->pos.x, psTarget->pos.y))
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

			if (psTarget->type == OBJ_DROID && ((const DROID*)psTarget)->isVtol()
			    && psStruct->getWeaponStats(0)->surfaceToAir == SHOOT_IN_AIR)
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

	const MAPTILE *psTile = mapTile(gameWorld.map, map_coord(psTarget->pos.x), map_coord(psTarget->pos.y));
	const bool jammed = psTile->jammerBits & ~alliancebits[psViewer->player];

	// Special rule for VTOLs, as they are not affected by ECM
	if (((psTarget->type == OBJ_DROID && ((const DROID*)psTarget)->isVtol())
	     || (psViewer->type == OBJ_DROID && ((const DROID*)psViewer)->isVtol()))
	    && dist < range)
	{
		return UBYTE_MAX;
	}

	// All the ray produces is the blocking wall, and nothing below reads it: what is returned comes from
	// the target tile's per-player watcher, sensor and jammer counts, which visTilesUpdate() maintains.
	if (gWall != nullptr && gNumWalls != nullptr)
	{
		VisibleObjectHelp_t help = {
			wallsBlock,
			map_coord(psTarget->pos.xy()),
			0,
			nullptr
		};

		WZ_PERF_COUNT(C_rayCasts, 1);
		rayCast(gameWorld.map, psViewer->pos.xy(), psTarget->pos.xy(), rayLOSCallback, &help);

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
	STRUCTURE *wall = nullptr;

	// HACK Using globals to not clutter visibleObject() interface too much
	gNumWalls = &numWalls;
	gWall = &wall;

	visibleObject(psViewer, psTarget, true);

	gNumWalls = nullptr;
	gWall = nullptr;

	return numWalls > 0 ? wall : nullptr;
}

bool hasSharedVision(unsigned viewer, unsigned ally)
{
	if (viewer >= MAX_PLAYERS) { return false; }
	ASSERT_OR_RETURN(false, ally < MAX_PLAYERS, "Bad ally %u (viewer: %u)", ally, viewer);

	return viewer == ally || (bMultiPlayer && alliancesSharedVision(game.alliance) && aiCheckAlliances(viewer, ally));
}

static void setSeenBy(BASE_OBJECT *psObj, unsigned viewer, int val /*= UBYTE_MAX*/)
{
	if (viewer >= MAX_PLAYERS) { return; }

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
	if (viewer >= MAX_PLAYERS) { return; }

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

	// if a player has a SAT_UPLINK structure, or has godMode enabled (and is not a spectator!!),
	// they can see everything!
	for (unsigned viewer = 0; viewer < MAX_PLAYERS; viewer++)
	{
		if (getSatUplinkExists(viewer) || (viewer == selectedPlayer && godMode && !NetPlay.players[viewer].isSpectator))
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
	if (psStruct != nullptr && psStruct->status == SS_BUILT && (structCBSensor(psStruct) || structVTOLCBSensor(psStruct)) && psStruct->psTarget[0] != nullptr)
	{
		setSeenByInstantly(psStruct->psTarget[0], psObj->player, UBYTE_MAX);
	}
	DROID *psDroid = castDroid(psObj);
	if (psDroid != nullptr && psDroid->action == DACTION_OBSERVE && cbSensorDroid(psDroid) && psDroid->psActionTarget[0] != nullptr)
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

	// visibleObject() returns 0 for these viewers before it reads anything else.
	const STRUCTURE *psStruct = castStructure(psViewer);
	if (psStruct != nullptr
	    && (psStruct->status != SS_BUILT
	        || psStruct->pStructureType->type == REF_WALL
	        || psStruct->pStructureType->type == REF_GATE
	        || psStruct->pStructureType->type == REF_WALLCORNER))
	{
		return;
	}

	// get all the objects from the grid the droid is in
	// Will give inconsistent results if hasSharedVision is not an equivalence relation.
	for (BASE_OBJECT *psObj : gridStartIterateUnseen(psViewer->pos.x, psViewer->pos.y, objSensorRange(psViewer), psViewer->player))
	{
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
				setUnderTilesVis(psObj, gameWorld.map, player);
			}

			// if a feature has just become visible set the message blips
			if (psObj->type == OBJ_FEATURE)
			{
				MESSAGE *psMessage;
				INGAME_AUDIO type = NO_SOUND;

				/* If this is an oil resource we want to add a proximity message for
				 * the selected Player - if there isn't an Resource Extractor on it. */
				if (((FEATURE *)psObj)->psStats->subType == FEAT_OIL_RESOURCE && !TileHasStructure(mapTile(gameWorld.map, map_coord(psObj->pos.x), map_coord(psObj->pos.y))))
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
	WZ_PROFILE_SCOPE(processVisibility);
	WZ_PERF_SCOPE(T_processVisibility);
	updateSpotters();
	for (int player = 0; player < MAX_PLAYERS; ++player)
	{
		for (BASE_OBJECT* psObj : gameWorld.objects.droids[player])
		{
			processVisibilitySelf(psObj);
		}
		for (BASE_OBJECT* psObj : gameWorld.objects.structures[player])
		{
			processVisibilitySelf(psObj);
		}
	}
	for (BASE_OBJECT* psObj : gameWorld.objects.features[0])
	{
		processVisibilitySelf(psObj);
	}
	for (int player = 0; player < MAX_PLAYERS; ++player)
	{
		for (BASE_OBJECT* psObj : gameWorld.objects.droids[player])
		{
			processVisibilityVision(psObj);
		}
		for (BASE_OBJECT* psObj : gameWorld.objects.structures[player])
		{
			processVisibilityVision(psObj);
		}
	}
	for (const BASE_OBJECT *psObj : gameWorld.objects.sensors[0])
	{
		if (objRadarDetector(psObj))
		{
			for (BASE_OBJECT *psTarget : gameWorld.objects.sensors[0])
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
		for (BASE_OBJECT* psObj : gameWorld.objects.droids[player])
		{
			processVisibilityLevel(psObj, addedMessage);
		}
		for (BASE_OBJECT* psObj : gameWorld.objects.structures[player])
		{
			processVisibilityLevel(psObj, addedMessage);
		}
	}
	for (BASE_OBJECT* psObj : gameWorld.objects.features[0])
	{
		processVisibilityLevel(psObj, addedMessage);
	}
	if (addedMessage)
	{
		jsDebugMessageUpdate();
	}
}

void	setUnderTilesVis(BASE_OBJECT *psObj, WorldMapState& mapState, UDWORD player)
{
	UDWORD		i, j;
	UDWORD		mapX, mapY, width, breadth;
	FEATURE		*psFeature;
	STRUCTURE	*psStructure;
	FEATURE_STATS const *psStats;
	MAPTILE		*psTile;

	if (player >= MAX_PLAYERS)
	{
		return;
	}

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
			psTile = mapTile(mapState, mapX + i, mapY + j);
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
		psStats = ((const DROID*)psViewer)->getWeaponStats(weapon_slot);
	}
	else if (psViewer->type == OBJ_STRUCTURE)
	{
		psStats = ((const STRUCTURE*)psViewer)->getWeaponStats(weapon_slot);
	}
	// 2d distance
	int distance = iHypot((psTarget->pos - psViewer->pos).xy());
	int range = proj_GetLongRange(*psStats, psViewer->player);
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
	WZ_PERF_SCOPE(T_checkFireLine);
	WZ_PERF_COUNT(C_fireLineTraces, 1);
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
			angle_check(&angletan, partSq, map_Height(gameWorld.map, current) - pos.z, distSq, dest.z - pos.z, direct);
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
				angle_check(&angletan, partSq, map_Height(gameWorld.map, halfway) - pos.z, distSq, dest.z - pos.z, direct);
			}
		}

		// check for walls and other structures
		// TODO: if there is a structure on the same tile as the shooter (and the shooter is not that structure) check if LOF is blocked by it.
		if (wallsBlock && oldPartSq > 0)
		{
			const MAPTILE *psTile;
			halfway = current + (next - current) / 2;
			psTile = mapTile(gameWorld.map, map_coord(halfway.x), map_coord(halfway.y));
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

/// Whether a terrain change in tile rectangle r may change psObj's tile coverage: r is within its sensor range (its
/// wavecast), or under it (its own height). Tile heights are at tile corners, so a changed height affects the tiles
/// on both sides of it: keep a margin of two tiles.
static bool heightChangeReaches(const BASE_OBJECT *psObj, const std::pair<Vector2i, Vector2i> &r)
{
	const Vector2i lo = world_coord(r.first);
	const Vector2i hi = world_coord(r.second + Vector2i(1, 1));
	const int64_t dx = std::max<int64_t>({0, lo.x - psObj->pos.x, psObj->pos.x - hi.x});
	const int64_t dy = std::max<int64_t>({0, lo.y - psObj->pos.y, psObj->pos.y - hi.y});
	const int64_t reach = objSensorRange(psObj) + 2 * TILE_UNITS;
	return dx * dx + dy * dy <= reach * reach;
}

/// Players whose sensor or jammer range changed since the last visFlushCoverageChanges()
static PlayerMask visRangeChangedPlayers = 0;

void visNoteRangeChange(int player)
{
	ASSERT_OR_RETURN(, player >= 0 && player < MAX_PLAYERS, "Bad player %d", player);
	visRangeChangedPlayers |= static_cast<PlayerMask>(1) << player;
}

bool visCoverageChangesPending()
{
	return visRangeChangedPlayers != 0 || !gameWorld.map.heightChanges.empty() || !mission.gameWorld.map.heightChanges.empty();
}

void visRecomputeCoverage(GameWorld& world)
{
	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		for (DROID *psDroid : world.objects.droids[player])
		{
			if (!psDroid->died)
			{
				visTilesUpdate(psDroid, world.map);
			}
		}
		for (STRUCTURE *psStruct : world.objects.structures[player])
		{
			if (!psStruct->died)
			{
				visTilesUpdate(psStruct, world.map);
			}
		}
	}
	world.map.heightChanges.clear();
	visRangeChangedPlayers = 0;
}

static void flushWorldCoverageChanges(GameWorld& world)
{
	if (!world.map.tiles)
	{
		world.map.heightChanges.clear();
		return;
	}
	if (world.map.heightChanges.empty() && visRangeChangedPlayers == 0)
	{
		return;
	}
	auto refresh = [&world](BASE_OBJECT *psObj) {
		if (psObj->died)
		{
			return;
		}
		if (visRangeChangedPlayers & (static_cast<PlayerMask>(1) << psObj->player))
		{
			visTilesUpdate(psObj, world.map);
			return;
		}
		for (const std::pair<Vector2i, Vector2i> &r : world.map.heightChanges)
		{
			if (heightChangeReaches(psObj, r))
			{
				visTilesUpdate(psObj, world.map);
				return;
			}
		}
	};
	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		for (DROID *psDroid : world.objects.droids[player])
		{
			refresh(psDroid);
		}
		for (STRUCTURE *psStruct : world.objects.structures[player])
		{
			refresh(psStruct);
		}
	}
	world.map.heightChanges.clear();
}

void visFlushCoverageChanges()
{
	if (!visCoverageChangesPending())
	{
		return;
	}
	// Both worlds: the inactive one (a campaign home base during an off-world mission, or the off-world map) isn't
	// simulated, but range upgrades apply to its objects too, and a restore recomputes its coverage as well. (Its
	// terrain doesn't change while it is inactive.)
	flushWorldCoverageChanges(gameWorld);
	flushWorldCoverageChanges(mission.gameWorld);
	visRangeChangedPlayers = 0;
}

CoverageCheckResult visCheckCoverage(GameWorld& world)
{
	CoverageCheckResult result;
	WorldMapState &mapState = world.map;
	if (!mapState.tiles)
	{
		return result;
	}
	const size_t numTiles = static_cast<size_t>(mapState.width) * static_cast<size_t>(mapState.height);

	// The per-tile counts: the sum of every watched tile
	// (objects use type 1 for watchers and 0 for sensors, spotters the other way around), against what the tiles hold
	{
		std::vector<uint32_t> watchers(numTiles * MAX_PLAYERS, 0), sensors(numTiles * MAX_PLAYERS, 0), jammers(numTiles * MAX_PLAYERS, 0);
		auto addTile = [&](const TILEPOS &t, unsigned player, bool isWatcher, bool isJammer) {
			const size_t i = static_cast<size_t>(t.x) + static_cast<size_t>(t.y) * mapState.width;
			if (i < numTiles && player < MAX_PLAYERS)
			{
				(isWatcher ? watchers : sensors)[i * MAX_PLAYERS + player]++;
				jammers[i * MAX_PLAYERS + player] += isJammer;
			}
		};
		auto addObject = [&](const BASE_OBJECT *psObj) {
			for (const TILEPOS &t : psObj->watchedTiles)
			{
				addTile(t, psObj->player, t.type != 0, psObj->flags.test(OBJECT_FLAG_JAMMED_TILES));
			}
		};
		for (unsigned player = 0; player < MAX_PLAYERS; ++player)
		{
			for (const DROID *psDroid : world.objects.droids[player])
			{
				addObject(psDroid);
			}
			for (const STRUCTURE *psStruct : world.objects.structures[player])
			{
				addObject(psStruct);
			}
		}
		for (const BASE_OBJECT *psObj : world.objects.pendingVisRemoval)
		{
			addObject(psObj);
		}
		for (const SPOTTER *psSpot : apsInvisibleViewers)
		{
			if (&psSpot->mapState == &mapState)
			{
				for (int i = 0; i < psSpot->numWatchedTiles; ++i)
				{
					addTile(psSpot->watchedTiles[i], psSpot->player, psSpot->watchedTiles[i].type == 0, false);
				}
			}
		}
		for (size_t i = 0; i < numTiles; ++i)
		{
			const MAPTILE &tile = mapState.tiles[i];
			for (unsigned player = 0; player < MAX_PLAYERS; ++player)
			{
				const size_t k = i * MAX_PLAYERS + player;
				if (tile.watchers[player] != watchers[k] || tile.sensors[player] != sensors[k] || tile.jammers[player] != jammers[k])
				{
					if (result.inconsistentTiles++ == 0)
					{
						result.firstInconsistentTile = astringf("tile (%zu, %zu) player %u: watchers %u, sensors %u, jammers %u; the watched tiles add up to %u, %u, %u",
							i % mapState.width, i / mapState.width, player, static_cast<unsigned>(tile.watchers[player]), static_cast<unsigned>(tile.sensors[player]), static_cast<unsigned>(tile.jammers[player]), watchers[k], sensors[k], jammers[k]);
					}
				}
			}
		}
	}

	// Exploration: every watched tile explored for every player the watcher shares vision with
	auto checkExplored = [&](const TILEPOS *tiles, size_t count, unsigned player, const char *what, uint32_t id) {
		if (player >= MAX_PLAYERS)
		{
			return;
		}
		for (size_t n = 0; n < count; ++n)
		{
			const size_t i = static_cast<size_t>(tiles[n].x) + static_cast<size_t>(tiles[n].y) * mapState.width;
			if (i < numTiles && (mapState.tiles[i].tileExploredBits & alliancebits[player]) != alliancebits[player])
			{
				if (result.unexplored++ == 0)
				{
					result.firstUnexplored = astringf("%s %u of player %u: tile (%u, %u) explored for 0x%x, shares vision with 0x%x",
						what, id, player, (unsigned)tiles[n].x, (unsigned)tiles[n].y, (unsigned)mapState.tiles[i].tileExploredBits, (unsigned)alliancebits[player]);
				}
				return;
			}
		}
	};
	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		for (const DROID *psDroid : world.objects.droids[player])
		{
			checkExplored(psDroid->watchedTiles.data(), psDroid->watchedTiles.size(), psDroid->player, "droid", psDroid->id);
		}
		for (const STRUCTURE *psStruct : world.objects.structures[player])
		{
			checkExplored(psStruct->watchedTiles.data(), psStruct->watchedTiles.size(), psStruct->player, "structure", psStruct->id);
		}
	}
	for (const SPOTTER *psSpot : apsInvisibleViewers)
	{
		if (&psSpot->mapState == &mapState)
		{
			checkExplored(psSpot->watchedTiles, psSpot->numWatchedTiles, psSpot->player, "spotter", psSpot->id);
		}
	}

	// The watched tiles: recompute every object's, compare, and put back what it had
	struct SavedObject
	{
		BASE_OBJECT *psObj;
		std::vector<TILEPOS> watchedTiles;
		bool jammedTiles;
		int32_t visOriginBand;
	};
	std::vector<SavedObject> saved;
	auto save = [&saved](BASE_OBJECT *psObj) {
		if (!psObj->died)
		{
			const int32_t band = (psObj->type == OBJ_DROID) ? static_cast<DROID *>(psObj)->visOriginBand : 0;
			saved.push_back({psObj, psObj->watchedTiles, psObj->flags.test(OBJECT_FLAG_JAMMED_TILES), band});
		}
	};
	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		for (DROID *psDroid : world.objects.droids[player])
		{
			save(psDroid);
		}
		for (STRUCTURE *psStruct : world.objects.structures[player])
		{
			save(psStruct);
		}
	}
	// Recomputing changes the per-tile counts, bits and exploration: keep the whole tile array to put back.
	std::vector<MAPTILE> savedTiles(mapState.tiles.get(), mapState.tiles.get() + numTiles);

	for (const SavedObject &s : saved)
	{
		visTilesUpdate(s.psObj, mapState);
	}
	for (const SavedObject &s : saved)
	{
		++result.checked;
		const std::vector<TILEPOS> &now = s.psObj->watchedTiles;
		const bool same = now.size() == s.watchedTiles.size()
			&& std::equal(now.begin(), now.end(), s.watchedTiles.begin(), [](const TILEPOS &a, const TILEPOS &b) {
				return a.x == b.x && a.y == b.y && a.type == b.type;
			});
		if (!same || s.psObj->flags.test(OBJECT_FLAG_JAMMED_TILES) != s.jammedTiles)
		{
			if (result.differing++ == 0)
			{
				result.firstDifference = astringf("%s %u of player %d at (%d, %d, %d): %zu watched tiles, %zu after a recompute",
					s.psObj->type == OBJ_DROID ? "droid" : "structure", s.psObj->id, (int)s.psObj->player,
					s.psObj->pos.x, s.psObj->pos.y, s.psObj->pos.z, s.watchedTiles.size(), now.size());
			}
		}
	}

	for (SavedObject &s : saved)
	{
		s.psObj->watchedTiles = std::move(s.watchedTiles);
		s.psObj->flags.set(OBJECT_FLAG_JAMMED_TILES, s.jammedTiles);
		if (s.psObj->type == OBJ_DROID)
		{
			static_cast<DROID *>(s.psObj)->visOriginBand = s.visOriginBand;  // the recompute set it
		}
	}
	std::copy(savedTiles.begin(), savedTiles.end(), mapState.tiles.get());
	return result;
}
