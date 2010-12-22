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
/** @file
 *  Definitions for the map structure
 */

#ifndef __INCLUDED_SRC_MAP_H__
#define __INCLUDED_SRC_MAP_H__

#include "lib/framework/frame.h"
#include "lib/framework/debug.h"
#include "objects.h"
#include "terrain.h"
#include "multiplay.h"
#include "display.h"

/* The different types of terrain as far as the game is concerned */
typedef enum _terrain_type
{
	TER_SAND,
	TER_SANDYBRUSH,
	TER_BAKEDEARTH,
	TER_GREENMUD,
	TER_REDBRUSH,
	TER_PINKROCK,
	TER_ROAD,
	TER_WATER,
	TER_CLIFFFACE,
	TER_RUBBLE,
	TER_SHEETICE,
	TER_SLUSH,

	TER_MAX,
} TYPE_OF_TERRAIN;

#define TILESET_ARIZONA 0
#define TILESET_URBAN	1
#define TILESET_ROCKIES 2

#define TALLOBJECT_YMAX		(200)
#define TALLOBJECT_ADJUST	(300)

/* Flags for whether texture tiles are flipped in X and Y or rotated */
#define TILE_XFLIP		0x8000
#define TILE_YFLIP		0x4000
#define TILE_ROTMASK	0x3000
#define TILE_ROTSHIFT	12
#define TILE_TRIFLIP	0x0800	// This bit describes the direction the tile is split into 2 triangles (same as triangleFlip)
#define TILE_NUMMASK	0x01ff

#define BITS_TEMPORARY		0x04	///< For used in temporary calculations

static inline unsigned short TileNumber_tile(unsigned short tilenumber)
{
	return tilenumber & TILE_NUMMASK;
}


static inline unsigned short TileNumber_texture(unsigned short tilenumber)
{
	return tilenumber & ~TILE_NUMMASK;
}

#define BITS_DECAL		0x02	///< Does this tile has a decal? If so, the tile from "texture" is drawn on top of the terrain.
#define BITS_FPATHBLOCK		0x10	///< Bit set temporarily by find path to mark a blocking tile
#define BITS_ON_FIRE            0x20    ///< Whether tile is burning
#define BITS_GATEWAY		0x40	///< Bit set to show a gateway on the tile

typedef struct _ground_type
{
	const char *textureName;
	float textureSize;
} GROUND_TYPE;

/* Information stored with each tile */
typedef struct _maptile
{
	uint8_t			tileInfoBits;
	uint8_t			tileExploredBits;
	uint8_t			sensorBits;		// bit per player, who can see tile with sensor
	uint8_t			illumination;	// How bright is this tile?
	uint8_t			watchers[MAX_PLAYERS];		// player sees through fog of war here with this many objects
	uint16_t		texture;		// Which graphics texture is on this tile
	int32_t                 height;                 ///< The height at the top left of the tile
	float                   level;                  ///< The visibility level of the top left of the tile, for this client.
	BASE_OBJECT		*psObject;		// Any object sitting on the location (e.g. building)
	PIELIGHT		colour;
	uint16_t		limitedContinent;	///< For land or sea limited propulsion types
	uint16_t		hoverContinent;		///< For hover type propulsions
	uint8_t			ground;			///< The ground type used for the terrain renderer
	uint16_t                fireEndTime;            ///< The (uint16_t)(gameTime / GAME_TICKS_PER_UPDATE) that BITS_ON_FIRE should be cleared.
	int32_t                 waterLevel;             ///< At what height is the water for this tile
} MAPTILE;

/* The size and contents of the map */
extern SDWORD	mapWidth, mapHeight;
extern MAPTILE *psMapTiles;
extern float waterLevel;
extern GROUND_TYPE *psGroundTypes;
extern int numGroundTypes;
extern int waterGroundType;
extern int cliffGroundType;
extern char *tileset;

#define AIR_BLOCKED		0x01	///< Aircraft cannot pass tile
#define FEATURE_BLOCKED		0x02	///< Ground units cannot pass tile due to item in the way
#define WATER_BLOCKED		0x04	///< Units that cannot pass water are blocked by this tile
#define LAND_BLOCKED		0x08	///< The inverse of the above -- for propeller driven crafts

#define AUXBITS_UNUSED		0x01	///< Unused for now
#define AUXBITS_OUR_BUILDING	0x02	///< Do we or our allies have a building at this tile
#define AUXBITS_ANY_BUILDING	0x04	///< Is there any building that might be blocking here?
#define AUXBITS_TEMPORARY	0x08	///< Temporary bit used in calculations
#define AUXBITS_DANGER		0x10	///< Does AI sense danger going there?
#define AUXBITS_THREAT		0x20	///< Can hostile players shoot here?
#define AUXBITS_AATHREAT	0x40	///< Can hostile players shoot at my VTOLs here?
#define AUXBITS_BUILDING	0x80	///< Whether player has blocking building at tile, combine it with alliance bits and blockingBits
#define AUXBITS_ALL		0xff

#define AUX_MAP		0
#define AUX_ASTARMAP	1
#define AUX_DANGERMAP	2 
#define AUX_MAX		3

extern uint8_t *psBlockMap[AUX_MAX];
extern uint8_t *psAuxMap[MAX_PLAYERS + AUX_MAX];	// yes, we waste one element... eyes wide open... makes API nicer

/// Find aux bitfield for a given tile
WZ_DECL_ALWAYS_INLINE static inline uint8_t auxTile(int x, int y, int player)
{
	return psAuxMap[player][x + y * mapWidth];
}

/// Find blocking bitfield for a given tile
WZ_DECL_ALWAYS_INLINE static inline uint8_t blockTile(int x, int y, int slot)
{
	return psBlockMap[slot][x + y * mapWidth];
}

/// Store a shadow copy of a player's aux map for use in threaded calculations
static inline void auxMapStore(int player, int slot)
{
	memcpy(psBlockMap[slot], psBlockMap[0], sizeof(*psBlockMap[player]) * mapWidth * mapHeight);
	memcpy(psAuxMap[MAX_PLAYERS + slot], psAuxMap[player], sizeof(*psAuxMap[player]) * mapWidth * mapHeight);
}

/// Restore selected fields from the shadow copy of a player's aux map (ignoring the block map)
static inline void auxMapRestore(int player, int slot, int mask)
{
	int i;
	uint8_t original, cached;

	for (i = 0; i < mapHeight * mapWidth; i++)
	{
		original = psAuxMap[player][i];
		cached = psAuxMap[MAX_PLAYERS + slot][i];
		psAuxMap[player][i] = original ^ ((original ^ cached) & mask); 
	}
}

/// Set aux bits. Always set identically for all players. States not set are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxSet(int x, int y, int player, int state)
{
	psAuxMap[player][x + y * mapWidth] |= state;
}

/// Set aux bits. Always set identically for all players. States not set are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxSetAll(int x, int y, int state)
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		psAuxMap[i][x + y * mapWidth] |= state;
	}
}

/// Set aux bits. Always set identically for all players. States not set are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxSetAllied(int x, int y, int player, int state)
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (alliancebits[player] & (1 << i))
		{
			psAuxMap[i][x + y * mapWidth] |= state;
		}
	}
}

/// Clear aux bits. Always set identically for all players. States not cleared are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxClear(int x, int y, int player, int state)
{
	psAuxMap[player][x + y * mapWidth] &= ~state;
}

/// Clear all aux bits. Always set identically for all players. States not cleared are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxClearAll(int x, int y, int state)
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		psAuxMap[i][x + y * mapWidth] &= ~state;
	}
}

/// Set blocking bits. Always set identically for all players. States not set are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxSetBlocking(int x, int y, int state)
{
	psBlockMap[0][x + y * mapWidth] |= state;
}

/// Clear blocking bits. Always set identically for all players. States not cleared are retained.
WZ_DECL_ALWAYS_INLINE static inline void auxClearBlocking(int x, int y, int state)
{
	psBlockMap[0][x + y * mapWidth] &= ~state;
}

/**
 * Check if tile contains a structure or feature. Function is thread-safe,
 * but do not rely on the result if you mean to alter the object pointer.
 */
static inline bool TileIsOccupied(const MAPTILE* tile)
{
	return tile->psObject != NULL;
}

/** Check if tile contains a structure. Function is NOT thread-safe. */
static inline bool TileHasStructure(const MAPTILE* tile)
{
	return TileIsOccupied(tile)
	    && tile->psObject->type == OBJ_STRUCTURE;
}

/** Check if tile contains a feature. Function is NOT thread-safe. */
static inline bool TileHasFeature(const MAPTILE* tile)
{
	return TileIsOccupied(tile)
	    && tile->psObject->type == OBJ_FEATURE;
}

/** Check if tile contains a wall structure. Function is NOT thread-safe. */
static inline bool TileHasWall(const MAPTILE* tile)
{
	return TileHasStructure(tile)
	    && (((STRUCTURE*)tile->psObject)->pStructureType->type == REF_WALL
	     || ((STRUCTURE*)tile->psObject)->pStructureType->type == REF_WALLCORNER);
}

/** Check if tile is burning. */
static inline bool TileIsBurning(const MAPTILE *tile)
{
	return tile->tileInfoBits & BITS_ON_FIRE;
}

/** Check if tile has been explored. */
static inline bool tileIsExplored(const MAPTILE *psTile)
{
	return psTile->tileExploredBits & (1 << selectedPlayer);
}

/** Check if tile contains a small structure. Function is NOT thread-safe. */
static inline bool TileHasSmallStructure(const MAPTILE* tile)
{
	return TileHasStructure(tile)
	    && ((STRUCTURE*)tile->psObject)->pStructureType->height == 1;
}

#define SET_TILE_DECAL(x)	((x)->tileInfoBits |= BITS_DECAL)
#define CLEAR_TILE_DECAL(x)	((x)->tileInfoBits &= ~BITS_DECAL)
#define TILE_HAS_DECAL(x)	((x)->tileInfoBits & BITS_DECAL)

// Multiplier for the tile height
#define	ELEVATION_SCALE	2

/* Allows us to do if(TRI_FLIPPED(psTile)) */
#define TRI_FLIPPED(x)		((x)->texture & TILE_TRIFLIP)
/* Flips the triangle partition on a tile pointer */
#define TOGGLE_TRIFLIP(x)	((x)->texture ^= TILE_TRIFLIP)

/* Can player number p has explored tile t? */
#define TEST_TILE_VISIBLE(p,t)	((t)->tileExploredBits & (1<<(p)))

/* Set a tile to be visible for a player */
#define SET_TILE_VISIBLE(p,t) ((t)->tileExploredBits |= alliancebits[p])

/* Arbitrary maximum number of terrain textures - used in look up table for terrain type */
#define MAX_TILE_TEXTURES	255

extern UBYTE terrainTypes[MAX_TILE_TEXTURES];

static inline unsigned char terrainType(const MAPTILE * tile)
{
	return terrainTypes[TileNumber_tile(tile->texture)];
}


/* The maximum map size */

#define MAP_MAXWIDTH	256
#define MAP_MAXHEIGHT	256
#define MAP_MAXAREA		(256*256)

#define TILE_MAX_HEIGHT (255 * ELEVATION_SCALE)
#define TILE_MIN_HEIGHT 0

/* The size and contents of the map */
extern SDWORD	mapWidth, mapHeight;
extern MAPTILE *psMapTiles;

extern GROUND_TYPE *psGroundTypes;
extern int numGroundTypes;
extern int waterGroundType;
extern int cliffGroundType;
extern char *tileset;

/*
 * Usage-Example:
 * tile_coordinate = (world_coordinate / TILE_UNITS) = (world_coordinate >> TILE_SHIFT)
 * world_coordinate = (tile_coordinate * TILE_UNITS) = (tile_coordinate << TILE_SHIFT)
 */

/* The shift on a world coordinate to get the tile coordinate */
#define TILE_SHIFT 7

/* The mask to get internal tile coords from a full coordinate */
#define TILE_MASK 0x7f

/* The number of units accross a tile */
#define TILE_UNITS (1<<TILE_SHIFT)

static inline int32_t world_coord(int32_t mapCoord)
{
	return mapCoord << TILE_SHIFT;
}

static inline int32_t map_coord(int32_t worldCoord)
{
	return worldCoord >> TILE_SHIFT;
}

static inline Vector2i world_coord(Vector2i const &mapCoord)
{
	return Vector2i(world_coord(mapCoord.x), world_coord(mapCoord.y));
}

static inline Vector2i map_coord(Vector2i const &worldCoord)
{
	return Vector2i(map_coord(worldCoord.x), map_coord(worldCoord.y));
}

/* Make sure world coordinates are inside the map */
/** Clip world coordinates to make sure they're inside the map's boundaries
 *  \param worldX a pointer to a X coordinate inside the map
 *  \param worldY a pointer to a Y coordinate inside the map
 *  \post 1 <= *worldX <= world_coord(mapWidth)-1 and
 *        1 <= *worldy <= world_coord(mapHeight)-1
 */
static inline void clip_world_offmap(int* worldX, int* worldY)
{
	// x,y must be > 0
	*worldX = MAX(1, *worldX);
	*worldY = MAX(1, *worldY);
	*worldX = MIN(world_coord(mapWidth) - 1, *worldX);
	*worldY = MIN(world_coord(mapHeight) - 1, *worldY);
}

/* maps a position down to the corner of a tile */
#define map_round(coord) ((coord) & (TILE_UNITS - 1))

/* Shutdown the map module */
extern BOOL mapShutdown(void);

/* Create a new map of a specified size */
extern BOOL mapNew(UDWORD width, UDWORD height);

/* Load the map data */
extern BOOL mapLoad(char *filename, BOOL preview);

/* Save the map data */
extern BOOL mapSave(char **ppFileData, UDWORD *pFileSize);

/** Return a pointer to the tile structure at x,y in map coordinates */
static inline WZ_DECL_PURE MAPTILE *mapTile(int32_t x, int32_t y)
{
	// Clamp x and y values to actual ones
	// Give one tile worth of leeway before asserting, for units/transporters coming in from off-map.
	ASSERT(x >= -1, "mapTile: x value is too small (%d,%d) in %dx%d",x,y,mapWidth,mapHeight);
	ASSERT(y >= -1, "mapTile: y value is too small (%d,%d) in %dx%d",x,y,mapWidth,mapHeight);
	x = MAX(x, 0);
	y = MAX(y, 0);
	ASSERT(x < mapWidth + 1, "mapTile: x value is too big (%d,%d) in %dx%d",x,y,mapWidth,mapHeight);
	ASSERT(y < mapHeight + 1, "mapTile: y value is too big (%d,%d) in %dx%d",x,y,mapWidth,mapHeight);
	x = MIN(x, mapWidth - 1);
	y = MIN(y, mapHeight - 1);

	return &psMapTiles[x + (y * mapWidth)];
}

static inline WZ_DECL_PURE MAPTILE *mapTile(Vector2i const &v) { return mapTile(v.x, v.y); }

/** Return a pointer to the tile structure at x,y in world coordinates */
#define worldTile(_x, _y) mapTile(map_coord(_x), map_coord(_y))

/// Return ground height of top-left corner of tile at x,y
static inline WZ_DECL_PURE int32_t map_TileHeight(int32_t x, int32_t y)
{
	if ( x >= mapWidth || y >= mapHeight || x < 0 || y < 0 )
	{
		return 0;
	}
	return psMapTiles[x + (y * mapWidth)].height;
}

/// Return water height of top-left corner of tile at x,y
static inline WZ_DECL_PURE int32_t map_WaterHeight(int32_t x, int32_t y)
{
	if ( x >= mapWidth || y >= mapHeight || x < 0 || y < 0 )
	{
		return 0;
	}
	return psMapTiles[x + (y * mapWidth)].waterLevel;
}

/// Return max(ground, water) height of top-left corner of tile at x,y
static inline WZ_DECL_PURE int32_t map_TileHeightSurface(int32_t x, int32_t y)
{
	if ( x >= mapWidth || y >= mapHeight || x < 0 || y < 0 )
	{
		return 0;
	}
	return MAX(psMapTiles[x + (y * mapWidth)].height, psMapTiles[x + (y * mapWidth)].waterLevel);
}


/*sets the tile height */
static inline void setTileHeight(int32_t x, int32_t y, int32_t height)
{
	ASSERT_OR_RETURN( , x < mapWidth && x >= 0, "x coordinate %d bigger than map width %u", x, mapWidth);
	ASSERT_OR_RETURN( , y < mapHeight && x >= 0, "y coordinate %d bigger than map height %u", y, mapHeight);

	psMapTiles[x + (y * mapWidth)].height = height;
	markTileDirty(x, y);
}

/* Return whether a tile coordinate is on the map */
WZ_DECL_ALWAYS_INLINE static inline BOOL tileOnMap(SDWORD x, SDWORD y)
{
	return (x >= 0) && (x < (SDWORD)mapWidth) && (y >= 0) && (y < (SDWORD)mapHeight);
}

/* Return true if a tile is not too near the map edge and not outside of the map */
static inline BOOL tileInsideBuildRange(SDWORD x, SDWORD y)
{
	return (x >= TOO_NEAR_EDGE) && (x < ((SDWORD)mapWidth - TOO_NEAR_EDGE)) &&
		(y >= TOO_NEAR_EDGE) && (y < ((SDWORD)mapHeight - TOO_NEAR_EDGE));
}

/* Return whether a world coordinate is on the map */
WZ_DECL_ALWAYS_INLINE static inline BOOL worldOnMap(int x, int y)
{
	return (x >= 0) && (x < ((SDWORD)mapWidth << TILE_SHIFT)) &&
		   (y >= 0) && (y < ((SDWORD)mapHeight << TILE_SHIFT));
}


/* Return whether a world coordinate is on the map */
WZ_DECL_ALWAYS_INLINE static inline bool worldOnMap2i(Vector2i pos)
{
	return worldOnMap(pos.x, pos.y);
}


/* Return whether a world coordinate is on the map */
WZ_DECL_ALWAYS_INLINE static inline bool worldOnMap3i(Vector3i pos)
{
	return worldOnMap(pos.x, pos.y);
}


/* Return whether a world coordinate is on the map */
WZ_DECL_ALWAYS_INLINE static inline bool worldOnMap3f(Vector3f pos)
{
	return worldOnMap(pos.x, pos.y);
}


/* Store a map coordinate and it's associated tile */
typedef struct _tile_coord
{
	UDWORD	x,y;
	MAPTILE	*psTile;
} TILE_COORD;

/// The max height of the terrain and water at the specified world coordinates
extern int32_t map_Height(int x, int y);

static inline int32_t map_Height(Vector2i const &v) { return map_Height(v.x, v.y); }

/* returns true if object is above ground */
bool mapObjIsAboveGround(SIMPLE_OBJECT *psObj);

/* returns the max and min height of a tile by looking at the four corners
   in tile coords */
extern void getTileMaxMin(UDWORD x, UDWORD y, UDWORD *pMax, UDWORD *pMin);

UDWORD GetHeightOfMap(void);
UDWORD GetWidthOfMap(void);
extern bool readVisibilityData(const char* fileName);
extern bool	writeVisibilityData(const char* fileName);

//scroll min and max values
extern SDWORD		scrollMinX, scrollMaxX, scrollMinY, scrollMaxY;

void mapFloodFillContinents(void);

extern void mapTest(void);

void tileSetFire(int32_t x, int32_t y, uint32_t duration);
extern bool fireOnLocation(unsigned int x, unsigned int y);

/**
 * Transitive sensor check for tile. Has to be here rather than
 * visibility.h due to header include order issues. 
 */
WZ_DECL_ALWAYS_INLINE static inline bool hasSensorOnTile(MAPTILE *psTile, unsigned player)
{
	return ((player == selectedPlayer && godMode) || (alliancebits[selectedPlayer] & (satuplinkbits | psTile->sensorBits)));
}

void mapInit(void);
void mapUpdate(void);

#endif // __INCLUDED_SRC_MAP_H__
