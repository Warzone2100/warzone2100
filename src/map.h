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
/** @file
 *  Definitions for the map structure
 */

#ifndef __INCLUDED_SRC_MAP_H__
#define __INCLUDED_SRC_MAP_H__

#include "lib/framework/frame.h"
#include "lib/framework/debug.h"
#include <wzmaplib/map.h>
#include <wzmaplib/terrain_type.h>
#include "objects.h"
#include "terrain.h"
#include "multiplay.h"
#include "display.h"
#include "ai.h"
#include "world_map_state.h"

#define TALLOBJECT_YMAX		(200)
#define TALLOBJECT_ADJUST	(300)

#define BITS_MARKED             0x01    ///< Is this tile marked?
#define BITS_DECAL              0x02    ///< Does this tile has a decal? If so, the tile from "texture" is drawn on top of the terrain.
#define BITS_FPATHBLOCK         0x10    ///< Bit set temporarily by find path to mark a blocking tile
#define BITS_ON_FIRE            0x20    ///< Whether tile is burning
#define BITS_GATEWAY            0x40    ///< Bit set to show a gateway on the tile

struct GROUND_TYPE
{
	std::string textureName;
	float textureSize;
	std::string normalMapTextureName = "";
	std::string specularMapTextureName = "";
	std::string heightMapTextureName = "";
	bool highQualityTextures = false; // whether this ground_type has normal / specular / height maps
};

extern float waterLevel;
extern char *tilesetDir;
extern MAP_TILESET currentMapTileset;

const GROUND_TYPE& getGroundType(size_t idx);
size_t getNumGroundTypes();
#define MAX_GROUND_TYPES 12

#define AIR_BLOCKED		0x01	///< Aircraft cannot pass tile
#define FEATURE_BLOCKED		0x02	///< Ground units cannot pass tile due to item in the way
#define WATER_BLOCKED		0x04	///< Units that cannot pass water are blocked by this tile
#define LAND_BLOCKED		0x08	///< The inverse of the above -- for propeller driven crafts

#define AUXBITS_NONPASSABLE     0x01    ///< Is there any building blocking here, other than a gate that would open for us?
#define AUXBITS_OUR_BUILDING	0x02	///< Do we or our allies have a building at this tile
#define AUXBITS_BLOCKING        0x04    ///< Is there any building currently blocking here?
#define AUXBITS_TEMPORARY	0x08	///< Temporary bit used in calculations
#define AUXBITS_DANGER		0x10	///< Does AI sense danger going there?
#define AUXBITS_THREAT		0x20	///< Can hostile players shoot here?
#define AUXBITS_AATHREAT	0x40	///< Can hostile players shoot at my VTOLs here?
#define AUXBITS_UNUSED          0x80    ///< Unused
#define AUXBITS_ALL		0xff

#define AUX_MAP		0
#define AUX_ASTARMAP	1
#define AUX_DANGERMAP	2
#define AUX_MAX		3

/// Find aux bitfield for a given tile
WZ_DECL_ALWAYS_INLINE static inline uint8_t auxTile(const WorldMapState& mapState, int x, int y, int player)
{
	ASSERT_OR_RETURN(AUXBITS_ALL, player >= 0 && player < MAX_PLAYERS + AUX_MAX, "invalid player: %d", player);
	return mapState.auxMap[player][x + y * mapState.width];
}

/// Find blocking bitfield for a given tile
WZ_DECL_ALWAYS_INLINE static inline uint8_t blockTile(const WorldMapState& mapState, int x, int y, int slot)
{
	return mapState.blockMap[slot][x + y * mapState.width];
}

/// Store a shadow copy of a player's aux map for use in threaded calculations
static inline void auxMapStore(WorldMapState& mapState, int player, int slot)
{
	memcpy(mapState.blockMap[slot].get(), mapState.blockMap[0].get(), sizeof(uint8_t) * mapState.width * mapState.height);
	memcpy(mapState.auxMap[MAX_PLAYERS + slot].get(), mapState.auxMap[player].get(), sizeof(uint8_t) * mapState.width * mapState.height);
}

/// Restore selected fields from the shadow copy of a player's aux map (ignoring the block map)
static inline void auxMapRestore(WorldMapState& mapState, int player, int slot, int mask)
{
	int i;
	uint8_t original, cached;

	for (i = 0; i < mapState.height * mapState.width; i++)
	{
		original = mapState.auxMap[player][i];
		cached = mapState.auxMap[MAX_PLAYERS + slot][i];
		mapState.auxMap[player][i] = original ^ ((original ^ cached) & mask);
	}
}

/// Set aux bits. Always set identically for all players. States not set are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxSet(WorldMapState& mapState, int x, int y, int player, int state)
{
	mapState.auxMap[player][x + y * mapState.width] |= state;
}

/// Set aux bits. Always set identically for all players. States not set are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxSetAll(WorldMapState& mapState, int x, int y, int state)
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		mapState.auxMap[i][x + y * mapState.width] |= state;
	}
}

/// Set aux bits. Always set identically for all players. States not set are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxSetAllied(WorldMapState& mapState, int x, int y, int player, int state)
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (aiCheckAlliances(player, i))
		{
			mapState.auxMap[i][x + y * mapState.width] |= state;
		}
	}
}

/// Set aux bits. Always set identically for all players. States not set are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxSetEnemy(WorldMapState& mapState, int x, int y, int player, int state)
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (!aiCheckAlliances(player, i))
		{
			mapState.auxMap[i][x + y * mapState.width] |= state;
		}
	}
}

/// Clear aux bits. Always set identically for all players. States not cleared are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxClear(WorldMapState& mapState, int x, int y, int player, int state)
{
	mapState.auxMap[player][x + y * mapState.width] &= ~state;
}

/// Clear all aux bits. Always set identically for all players. States not cleared are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxClearAll(WorldMapState& mapState, int x, int y, int state)
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		mapState.auxMap[i][x + y * mapState.width] &= ~state;
	}
}

/// Set blocking bits. Always set identically for all players. States not set are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxSetBlocking(WorldMapState& mapState, int x, int y, int state)
{
	mapState.blockMap[0][x + y * mapState.width] |= state;
}

/// Clear blocking bits. Always set identically for all players. States not cleared are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxClearBlocking(WorldMapState& mapState, int x, int y, int state)
{
	mapState.blockMap[0][x + y * mapState.width] &= ~state;
}

/**
 * Check if tile contains a structure or feature. Function is thread-safe,
 * but do not rely on the result if you mean to alter the object pointer.
 */
WZ_DECL_ALWAYS_INLINE static inline bool TileIsOccupied(const MAPTILE *tile)
{
	return tile->psObject != nullptr;
}

static inline bool TileIsKnownOccupied(MAPTILE const *tile, unsigned player)
{
	return TileIsOccupied(tile) &&
	       (tile->psObject->type != OBJ_STRUCTURE || ((STRUCTURE *)tile->psObject)->visible[player] || aiCheckAlliances(player, ((STRUCTURE *)tile->psObject)->player));
}

/** Check if tile contains a structure. Function is NOT thread-safe. */
static inline bool TileHasStructure(const MAPTILE *tile)
{
	return TileIsOccupied(tile)
	       && tile->psObject->type == OBJ_STRUCTURE;
}

/** Check if tile contains a feature. Function is NOT thread-safe. */
static inline bool TileHasFeature(const MAPTILE *tile)
{
	return TileIsOccupied(tile)
	       && tile->psObject->type == OBJ_FEATURE;
}

/** Check if tile contains a wall structure. Function is NOT thread-safe. */
static inline bool TileHasWall(const MAPTILE *tile)
{
	const auto *psStruct = (STRUCTURE *)tile->psObject;
	return TileHasStructure(tile)
	       && (psStruct->pStructureType->type == REF_WALL
	           || psStruct->pStructureType->type == REF_GATE
	           || psStruct->pStructureType->type == REF_WALLCORNER);
}

/** Check if tile contains a wall structure. Function is NOT thread-safe.
 * This function is specifically for raycast callback, because "TileHasWall" is used
 * to decide wether or not it's possible to place a new defense structure on top of a wall.
 * We do not want to allow place more defense structures on top of already existing ones.
*/
static inline bool TileHasWall_raycast(const MAPTILE *tile)
{
	const auto *psStruct = (STRUCTURE *)tile->psObject;
	return TileHasStructure(tile)
	       && (psStruct->pStructureType->type == REF_WALL
	           || psStruct->pStructureType->type == REF_GATE
	           // hadrcrete towers are technically a WALL + weapon, so count them as walls too
	           || psStruct->pStructureType->combinesWithWall
	           || psStruct->pStructureType->type == REF_WALLCORNER);
}

/** Check if tile is burning. */
static inline bool TileIsBurning(const MAPTILE *tile)
{
	return tile->tileInfoBits & BITS_ON_FIRE;
}

/** Check if tile has been explored. */
static inline bool tileIsExplored(const MAPTILE *psTile)
{
	if (selectedPlayer >= MAX_PLAYERS) { return true; }
	return psTile->tileExploredBits & (1 << selectedPlayer);
}

/** Is the tile ACTUALLY, 100% visible? -- (For DISPLAY-ONLY purposes - *NOT* game-state calculations!)
 * This is not the same as for ex. psStructure->visible[selectedPlayer],
 * because that would only mean the psStructure is in *explored Tile*
 * psDroid->visible on the other hand, works correctly,
 * because its visibility fades away in fog of war
*/
static inline bool tileIsClearlyVisible(const MAPTILE *psTile)
{
	if (selectedPlayer >= MAX_PLAYERS || godMode) { return true; }
	return psTile->sensorBits & (1 << selectedPlayer);
}

/** Check if tile contains a small structure. Function is NOT thread-safe. */
static inline bool TileHasSmallStructure(const MAPTILE *tile)
{
	return TileHasStructure(tile)
	       && ((STRUCTURE *)tile->psObject)->pStructureType->height == 1;
}

#define SET_TILE_DECAL(x)	((x)->tileInfoBits |= BITS_DECAL)
#define CLEAR_TILE_DECAL(x)	((x)->tileInfoBits &= ~BITS_DECAL)
#define TILE_HAS_DECAL(x)	((x)->tileInfoBits & BITS_DECAL)

/* Allows us to do if(TRI_FLIPPED(psTile)) */
#define TRI_FLIPPED(x)		((x)->texture & TILE_TRIFLIP)
/* Flips the triangle partition on a tile pointer */
#define TOGGLE_TRIFLIP(x)	((x)->texture ^= TILE_TRIFLIP)

/* Can player number p has explored tile t? */
#define TEST_TILE_VISIBLE(p,t)	((t)->tileExploredBits & (1<<(p)))

/* Can selectedPlayer see tile t? */
/* To be used for *DISPLAY* purposes only (*not* game-state/calculation related) */
static inline bool TEST_TILE_VISIBLE_TO_SELECTEDPLAYER(MAPTILE *pTile)
{
	if (godMode)
	{
		// always visible
		return true;
	}
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return false;
	}
	return TEST_TILE_VISIBLE(selectedPlayer, pTile);
}

/* Set a tile to be visible for a player */
#define SET_TILE_VISIBLE(p,t) ((t)->tileExploredBits |= alliancebits[p])

/* Arbitrary maximum number of terrain textures - used in look up table for terrain type */
#define MAX_TILE_TEXTURES	255

extern UBYTE terrainTypes[MAX_TILE_TEXTURES];

static inline unsigned char terrainType(const MAPTILE *tile)
{
	return terrainTypes[TileNumber_tile(tile->texture)];
}


/* Additional tile <-> world coordinate overloads */

static inline Vector2i world_coord(Vector2i const &mapCoord)
{
	return {world_coord(mapCoord.x), world_coord(mapCoord.y)};
}

static inline Vector2i map_coord(Vector2i const &worldCoord)
{
	return {map_coord(worldCoord.x), map_coord(worldCoord.y)};
}

static inline Vector2i round_to_nearest_tile(Vector2i const &worldCoord)
{
	return {round_to_nearest_tile(worldCoord.x), round_to_nearest_tile(worldCoord.y)};
}

/* Make sure world coordinates are inside the map */
/** Clip world coordinates to make sure they're inside the map's boundaries
 *  \param worldX a pointer to a X coordinate inside the map
 *  \param worldY a pointer to a Y coordinate inside the map
 *  \post 1 <= *worldX <= world_coord(mapWidth)-1 and
 *        1 <= *worldy <= world_coord(mapHeight)-1
 */
static inline void clip_world_offmap(const WorldMapState& mapState, int *worldX, int *worldY)
{
	// x,y must be > 0
	*worldX = MAX(1, *worldX);
	*worldY = MAX(1, *worldY);
	*worldX = MIN(world_coord(mapState.width) - 1, *worldX);
	*worldY = MIN(world_coord(mapState.height) - 1, *worldY);
}

/* Shutdown the map module */
bool mapShutdown();

/* Load the map data */
bool mapLoad(char const *filename, WorldMapState& mapState);
struct ScriptMapData;
bool loadTerrainTypeMap(const std::shared_ptr<WzMap::TerrainTypeData>& ttypeData);
bool mapLoadFromWzMapData(std::shared_ptr<WzMap::MapData> mapData, WorldMapState& mapState);

// used to reload decal + ground types types when switching terrain overrides
bool mapReloadGroundTypes();

class WzMapPhysFSIO : public WzMap::IOProvider
{
public:
	WzMapPhysFSIO() { }
	WzMapPhysFSIO(const std::string& baseMountPath)
	: m_basePath(baseMountPath)
	{ }
public:
	virtual std::unique_ptr<WzMap::BinaryIOStream> openBinaryStream(const std::string& filename, WzMap::BinaryIOStream::OpenMode mode) override;
	virtual WzMap::IOProvider::LoadFullFileResult loadFullFile(const std::string& filename, std::vector<char>& fileData, uint32_t maxFileSize = 0, bool appendNullCharacter = false) override;
	virtual bool writeFullFile(const std::string& filename, const char *ppFileData, uint32_t fileSize) override;
	virtual bool makeDirectory(const std::string& directoryPath) override;
	virtual const char* pathSeparator() const override;
	virtual bool fileExists(const std::string& filename) override;

	bool folderExists(const std::string& dirPath);

	virtual bool enumerateFiles(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc) override;
	virtual bool enumerateFolders(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc) override;
private:
	std::string m_basePath;
};

class WzMapDebugLogger : public WzMap::LoggingProtocol
{
public:
	virtual ~WzMapDebugLogger();
	virtual void printLog(WzMap::LoggingProtocol::LogLevel level, const char *function, int line, const char *str) override;
};

/* Save the map data */
bool mapSaveToWzMapData(WzMap::MapData& output, const WorldMapState& mapState);

/** Return a pointer to the tile structure at x,y in map coordinates */
static inline WZ_DECL_PURE MAPTILE *mapTile(WorldMapState& mapState, int32_t x, int32_t y)
{
	// Clamp x and y values to actual ones
	// Give one tile worth of leeway before asserting, for units/transporters coming in from off-map.
	ASSERT(x >= -1, "mapTile: x value is too small (%d,%d) in %dx%d", x, y, mapState.width, mapState.height);
	ASSERT(y >= -1, "mapTile: y value is too small (%d,%d) in %dx%d", x, y, mapState.width, mapState.height);
	x = MAX(x, 0);
	y = MAX(y, 0);
	ASSERT(x < mapState.width + 1, "mapTile: x value is too big (%d,%d) in %dx%d", x, y, mapState.width, mapState.height);
	ASSERT(y < mapState.height + 1, "mapTile: y value is too big (%d,%d) in %dx%d", x, y, mapState.width, mapState.height);
	x = MIN(x, mapState.width - 1);
	y = MIN(y, mapState.height - 1);

	return &mapState.tiles[x + (y * mapState.width)];
}

static inline WZ_DECL_PURE const MAPTILE* mapTile(const WorldMapState& mapState, int32_t x, int32_t y)
{
	return const_cast<const MAPTILE*>(mapTile(const_cast<WorldMapState&>(mapState), x, y));
}

static inline WZ_DECL_PURE MAPTILE *mapTile(WorldMapState& mapState, Vector2i const &v)
{
	return mapTile(mapState, v.x, v.y);
}

static inline WZ_DECL_PURE const MAPTILE* mapTile(const WorldMapState& mapState, Vector2i const &v)
{
	return const_cast<const MAPTILE*>(mapTile(const_cast<WorldMapState&>(mapState), v.x, v.y));
}

/** Return a pointer to the tile structure at x,y in world coordinates */
static inline WZ_DECL_PURE MAPTILE *worldTile(WorldMapState& mapState, int32_t x, int32_t y)
{
	return mapTile(mapState, map_coord(x), map_coord(y));
}

static inline WZ_DECL_PURE const MAPTILE* worldTile(const WorldMapState& mapState, int32_t x, int32_t y)
{
	return const_cast<const MAPTILE*>(worldTile(const_cast<WorldMapState&>(mapState), x, y));
}

static inline WZ_DECL_PURE MAPTILE *worldTile(WorldMapState& mapState, Vector2i const &v)
{
	return mapTile(mapState, map_coord(v));
}

static inline WZ_DECL_PURE const MAPTILE* worldTile(const WorldMapState& mapState, Vector2i const &v)
{
	return const_cast<const MAPTILE*>(worldTile(const_cast<WorldMapState&>(mapState), v));
}

/// Return ground height of top-left corner of tile at x,y
static inline WZ_DECL_PURE int32_t map_TileHeight(const WorldMapState& mapState, int32_t x, int32_t y)
{
	if (x >= mapState.width || y >= mapState.height || x < 0 || y < 0)
	{
		return 0;
	}
	return mapState.tiles[x + (y * mapState.width)].height;
}

/// Return water height of top-left corner of tile at x,y
static inline WZ_DECL_PURE int32_t map_WaterHeight(const WorldMapState& mapState, int32_t x, int32_t y)
{
	if (x >= mapState.width || y >= mapState.height || x < 0 || y < 0)
	{
		return 0;
	}
	return mapState.tiles[x + (y * mapState.width)].waterLevel;
}

/// Return max(ground, water) height of top-left corner of tile at x,y
static inline WZ_DECL_PURE int32_t map_TileHeightSurface(const WorldMapState& mapState, int32_t x, int32_t y)
{
	if (x >= mapState.width || y >= mapState.height || x < 0 || y < 0)
	{
		return 0;
	}
	return MAX(mapState.tiles[x + (y * mapState.width)].height, mapState.tiles[x + (y * mapState.width)].waterLevel);
}


/*sets the tile height */
static inline void setTileHeight(WorldMapState& mapState, int32_t x, int32_t y, int32_t height)
{
	ASSERT_OR_RETURN(, x < mapState.width && x >= 0, "x coordinate %d bigger than map width %u", x, mapState.width);
	ASSERT_OR_RETURN(, y < mapState.height && x >= 0, "y coordinate %d bigger than map height %u", y, mapState.height);

	mapState.tiles[x + (y * mapState.width)].height = height;
	markTileDirty(x, y);
}

/* Return whether a tile coordinate is on the map */
WZ_DECL_ALWAYS_INLINE static inline bool tileOnMap(const WorldMapState& mapState, SDWORD x, SDWORD y)
{
	return (x >= 0) && (x < (SDWORD)mapState.width) && (y >= 0) && (y < (SDWORD)mapState.height);
}

WZ_DECL_ALWAYS_INLINE static inline bool tileOnMap(const WorldMapState& mapState, Vector2i pos)
{
	return tileOnMap(mapState, pos.x, pos.y);
}

/* Return whether a world coordinate is on the map */
WZ_DECL_ALWAYS_INLINE static inline bool worldOnMap(const WorldMapState& mapState, int x, int y)
{
	return (x >= 0) && (x < ((SDWORD)mapState.width << TILE_SHIFT)) &&
	       (y >= 0) && (y < ((SDWORD)mapState.height << TILE_SHIFT));
}


/* Return whether a world coordinate is on the map */
WZ_DECL_ALWAYS_INLINE static inline bool worldOnMap(const WorldMapState& mapState, Vector2i pos)
{
	return worldOnMap(mapState, pos.x, pos.y);
}

static inline void makeTileRubbleTexture(MAPTILE *psTile, const unsigned int x, const unsigned int y, const unsigned int newTexture)
{
	psTile->texture = TileNumber_texture(psTile->texture) | newTexture;
	SET_TILE_DECAL(psTile);
	markTileDirty(x, y);
}


/* Intersect a line with the map and report tile intersection points */
bool map_Intersect(int *Cx, int *Cy, int *Vx, int *Vy, int *Sx, int *Sy);

/// Finds the smallest 0 ≤ t ≤ 1 such that the line segment given by src + t * (dst - src) intersects the terrain.
/// An intersection is defined to be the part of the line segment which is strictly inside the terrain (so if the
/// line segment is exactly parallel with the terrain and leaves it again, it does not count as an intersection).
/// Returns UINT32_MAX if no such 0 ≤ t ≤ 1 exists, otherwise returns t*tMax, rounded down to the nearest integer.
/// If src is strictly inside the terrain, the line segment is only considered to intersect if it exits and reenters
/// the terrain.
unsigned map_LineIntersect(Vector3i src, Vector3i dst, unsigned tMax);

/// The max height of the terrain and water at the specified world coordinates
int32_t map_Height(const WorldMapState& mapState, int x, int y);

static inline int32_t map_Height(const WorldMapState& mapState, Vector2i const &v)
{
	return map_Height(mapState, v.x, v.y);
}

/* returns true if object is above ground */
bool mapObjIsAboveGround(const SIMPLE_OBJECT *psObj);

/* returns the max and min height of a tile by looking at the four corners
   in tile coords */
void getTileMaxMin(const WorldMapState& mapState, int x, int y, int *pMax, int *pMin);

bool readVisibilityData(const char *fileName, WorldMapState& mapState);
bool writeVisibilityData(const char *fileName, const WorldMapState& mapState);

void mapFloodFillContinents(WorldMapState& mapState);

void tileSetFire(WorldMapState& mapState, int32_t x, int32_t y, uint32_t duration);
bool fireOnLocation(WorldMapState& mapState, unsigned int x, unsigned int y); // FIXME: add const overload

/**
 * Transitive sensor check for tile. Has to be here rather than
 * visibility.h due to header include order issues. -- (For DISPLAY-ONLY purposes - *NOT* game-state calculations!)
 */
WZ_DECL_ALWAYS_INLINE static inline bool hasSensorOnTile(MAPTILE *psTile, unsigned player)
{
	return ((player == selectedPlayer && godMode) ||
			((player < MAX_PLAYER_SLOTS) && (alliancebits[selectedPlayer] & (satuplinkbits | psTile->sensorBits)))
			);
}

struct GameWorld;

void mapInit(GameWorld& world);
void mapUpdate(GameWorld& world);

bool shouldLoadTerrainTypeOverrides(const std::string& name);
bool loadTerrainTypeMapOverride(MAP_TILESET tileSet);

//For saves to determine if loading the terrain type override should occur
extern bool builtInMap;
extern bool useTerrainOverrides;

#endif // __INCLUDED_SRC_MAP_H__
