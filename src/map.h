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
/** @file
 *  Definitions for the map structure
 */

#ifndef __INCLUDED_SRC_MAP_H__
#define __INCLUDED_SRC_MAP_H__

#include "lib/framework/frame.h"
#include "objects.h"

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

/* change these if you change above - maybe wrap up in enumerate? */
#define	TERRAIN_TYPES	TER_MAX

#define TALLOBJECT_YMAX		(200)
#define TALLOBJECT_ADJUST	(300)

/* Flags for whether texture tiles are flipped in X and Y or rotated */
#define TILE_XFLIP		0x8000
#define TILE_YFLIP		0x4000
#define TILE_ROTMASK	0x3000
#define TILE_ROTSHIFT	12
#define TILE_TRIFLIP	0x0800	// This bit describes the direction the tile is split into 2 triangles (same as triangleFlip)
#define TILE_HILIGHT	0x0400	// set when the tile has the structure cursor over it

// NASTY - this should be in tileInfoBits but there isn't any room left
#define TILE_NOTBLOCKING	0x0200	// units can drive on this even if there is a structure or feature on it

#define TILE_NUMMASK	0x01ff


static inline unsigned short TileNumber_tile(unsigned short tilenumber)
{
	return tilenumber & TILE_NUMMASK;
}


static inline unsigned short TileNumber_texture(unsigned short tilenumber)
{
	return tilenumber & ~TILE_NUMMASK;
}


#define BITS_FPATHBLOCK	0x10		// bit set temporarily by find path to mark a blocking tile
#define BITS_GATEWAY	0x40		// bit set to show a gateway on the tile

#define TILE_IS_NOTBLOCKING(x)	(x->texture & TILE_NOTBLOCKING)

#define TILE_OCCUPIED(x)		(x->psObject)
#define TILE_HAS_STRUCTURE(x)		(x->psObject && x->psObject->type == OBJ_STRUCTURE)
#define TILE_HAS_FEATURE(x)		(x->psObject && x->psObject->type == OBJ_FEATURE)
#define TILE_HAS_WALL(x)		(TILE_HAS_STRUCTURE(x) \
					&& (((STRUCTURE*)x->psObject)->pStructureType->type == REF_WALL \
					    || ((STRUCTURE*)x->psObject)->pStructureType->type == REF_WALLCORNER))
#define TILE_HIGHLIGHT(x)		(x->texture & TILE_HILIGHT)
#define TILE_HAS_TALLSTRUCTURE(x)	((TILE_HAS_STRUCTURE(x) && ((STRUCTURE*)x->psObject)->sDisplay.imd->max.y > TALLOBJECT_YMAX) \
                                         || (TILE_HAS_FEATURE(x) && ((FEATURE*)x->psObject)->sDisplay.imd->max.y > TALLOBJECT_YMAX))
#define TILE_HAS_SMALLSTRUCTURE(x)	(TILE_HAS_STRUCTURE(x) && ((STRUCTURE*)x->psObject)->pStructureType->height == 1)

#define SET_TILE_NOTBLOCKING(x)	(x->texture |= TILE_NOTBLOCKING)
#define CLEAR_TILE_NOTBLOCKING(x)	(x->texture &= ~TILE_NOTBLOCKING)

#define SET_TILE_HIGHLIGHT(x)	(x->texture = (UWORD)((x)->texture | TILE_HILIGHT))
#define CLEAR_TILE_HIGHLIGHT(x)	(x->texture = (UWORD)((x)->texture & (~TILE_HILIGHT)))

// Multiplier for the tile height
#define	ELEVATION_SCALE	2

/* Allows us to do if(TRI_FLIPPED(psTile)) */
#define TRI_FLIPPED(x)		(x->texture & TILE_TRIFLIP)
/* Flips the triangle partition on a tile pointer */
#define TOGGLE_TRIFLIP(x)	(x->texture = (UWORD)(x->texture ^ TILE_TRIFLIP))

/* Can player number p see tile t? */
#define TEST_TILE_VISIBLE(p,t)	( (t->tileVisBits) & (1<<p) )

/* Set a tile to be visible for a player */
#define SET_TILE_VISIBLE(p,t) t->tileVisBits = (UBYTE)(t->tileVisBits | (1<<p))

/* Arbitrary maximum number of terrain textures - used in look up table for terrain type */
#define MAX_TILE_TEXTURES	255

/* Information stored with each tile */
typedef struct _maptile
{
	UBYTE			tileInfoBits;
	uint8_t			tileVisBits;	// COMPRESSED - bit per player
	UBYTE			height;			// The height at the top left of the tile
	UBYTE			illumination;	// How bright is this tile?
	UWORD			texture;		// Which graphics texture is on this tile
	bool			bMaxed;
	bool			activeSensor;	// selected player can see through fog of war here
	float			level;
	BASE_OBJECT		*psObject;		// Any object sitting on the location (e.g. building)
	PIELIGHT		colour;

//	TYPE_OF_TERRAIN	type;			// The terrain type for the tile
} MAPTILE;


extern UBYTE terrainTypes[MAX_TILE_TEXTURES];

static inline unsigned char terrainType(MAPTILE * tile)
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
extern UDWORD	mapWidth, mapHeight;
extern MAPTILE *psMapTiles;

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
#define map_round(coord) (coord & (TILE_UNITS - 1))

/* Shutdown the map module */
extern BOOL mapShutdown(void);

/* Create a new map of a specified size */
extern BOOL mapNew(UDWORD width, UDWORD height);

/* Load the map data */
extern BOOL mapLoad(char *pFileData, UDWORD fileSize);

/* Save the map data */
extern BOOL mapSave(char **ppFileData, UDWORD *pFileSize);

/* New savegame format */
BOOL mapSaveTagged(char *pFileName);
BOOL mapLoadTagged(char *pFileName);

/* Return a pointer to the tile structure at x,y */
static inline WZ_DECL_PURE MAPTILE *mapTile(UDWORD x, UDWORD y)
{
	ASSERT( x < mapWidth,
		"mapTile: x coordinate bigger than map width" );
	ASSERT( y < mapHeight,
		"mapTile: y coordinate bigger than map height" );

	return &psMapTiles[x + (y * mapWidth)];
}

/* Return height of tile at x,y */
static inline WZ_DECL_PURE SWORD map_TileHeight(UDWORD x, UDWORD y)
{
	if ( x >= mapWidth || y >= mapHeight )
	{
		return 0;
	}
	return (SWORD)(psMapTiles[x + (y * mapWidth)].height * ELEVATION_SCALE);
}

/*sets the tile height */
static inline void setTileHeight(UDWORD x, UDWORD y, UDWORD height)
{

	ASSERT( x < mapWidth,
		"mapTile: x coordinate bigger than map width" );
	ASSERT( y < mapHeight,
		"mapTile: y coordinate bigger than map height" );

	psMapTiles[x + (y * mapWidth)].height = (UBYTE) (height / ELEVATION_SCALE);
}

/* Return whether a tile coordinate is on the map */
static inline BOOL tileOnMap(SDWORD x, SDWORD y)
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
static inline BOOL worldOnMap(SDWORD x, SDWORD y)
{
	return (x >= 0) && (x < ((SDWORD)mapWidth << TILE_SHIFT)) &&
		   (y >= 0) && (y < ((SDWORD)mapHeight << TILE_SHIFT));
}

/* Store a map coordinate and it's associated tile */
typedef struct _tile_coord
{
	UDWORD	x,y;
	MAPTILE	*psTile;
} TILE_COORD;

/* The map tiles generated by map calc line */
extern TILE_COORD	*aMapLinePoints;

/* Return height of x,y */
extern SWORD map_Height(int x, int y);

/* returns true if object is above ground */
extern BOOL mapObjIsAboveGround( BASE_OBJECT *psObj );

/* returns the max and min height of a tile by looking at the four corners
   in tile coords */
extern void getTileMaxMin(UDWORD x, UDWORD y, UDWORD *pMax, UDWORD *pMin);

UDWORD GetHeightOfMap(void);
UDWORD GetWidthOfMap(void);
extern bool readVisibilityData(const char* fileName);
extern bool	writeVisibilityData(const char* fileName);

//scroll min and max values
extern SDWORD		scrollMinX, scrollMaxX, scrollMinY, scrollMaxY;

#endif // __INCLUDED_SRC_MAP_H__
