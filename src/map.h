/*
 * Map.h
 *
 * Definitions for the map structure
 *
 */
#ifndef _map_h
#define _map_h

#include <stdio.h>
#include "lib/framework/frame.h"
#include "objects.h"


#define	TIB_VIS0		=	0x1,	// Visibility bits - can also be accessed as a byte (as a whole).
#define TIB_VIS1		=	0x2,
#define TIB_VIS2		=	0x4,
#define	TIB_VIS3		=	0x8,
#define	TIB_VIS4		=	0x10,
#define	TIB_VIS5		=	0x20,
#define	TIB_VIS6		=	0x40,
#define	TIB_VIS7		=	0x80,

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

typedef enum _tts
{
TILE_TYPE,
SPEED,
MARKER,
} TYPE_SPEEDS;
extern UDWORD	relativeSpeeds[TERRAIN_TYPES][MARKER];

#define TALLOBJECT_YMAX		(200)
#define TALLOBJECT_ADJUST	(200)

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
#define TILE_BITMASK	0xfe00

#define BITS_STRUCTURE	0x1
#define BITS_FEATURE	0x2
#define BITS_NODRAWTILE	0x4
#define BITS_SMALLSTRUCTURE	0x8			// show small structures - tank traps / bunkers
#define BITS_FPATHBLOCK	0x10		// bit set temporarily by find path to mark a blocking tile
#define BITS_WALL		0x20
#define BITS_GATEWAY	0x40		// bit set to show a gateway on the tile
#define BITS_TALLSTRUCTURE 0x80		// bit set to show a tall structure which camera needs to avoid.

#define BITS_STRUCTURE_MASK	0xfe
#define BITS_FEATURE_MASK	0xfd
#define BITS_OCCUPIED_MASK	0xfc

#define TILE_IS_NOTBLOCKING(x)	(x->texture & TILE_NOTBLOCKING)

#define TILE_IMPOSSIBLE(x)		((TILE_HAS_STRUCTURE(x)) && (TILE_HAS_FEATURE(x)))
#define BITS_OCCUPIED			(BITS_STRUCTURE | BITS_FEATURE | BITS_WALL)
#define TILE_OCCUPIED(x)		(x->tileInfoBits & BITS_OCCUPIED)
#define TILE_HAS_STRUCTURE(x)	(x->tileInfoBits & BITS_STRUCTURE)
#define TILE_HAS_FEATURE(x)		(x->tileInfoBits & BITS_FEATURE)
#define TILE_DRAW(x)			(!((x)->tileInfoBits & BITS_NODRAWTILE))
//#define TILE_HIGHLIGHT(x)		(x->tileInfoBits & BITS_TILE_HIGHLIGHT)
#define TILE_HIGHLIGHT(x)		(x->texture & TILE_HILIGHT)
#define TILE_HAS_TALLSTRUCTURE(x)	(x->tileInfoBits & BITS_TALLSTRUCTURE)
#define TILE_HAS_SMALLSTRUCTURE(x)	(x->tileInfoBits & BITS_SMALLSTRUCTURE)

#define SET_TILE_NOTBLOCKING(x)	(x->texture |= TILE_NOTBLOCKING)
#define CLEAR_TILE_NOTBLOCKING(x)	(x->texture &= ~TILE_NOTBLOCKING)

#define SET_TILE_STRUCTURE(x)	(x->tileInfoBits = (UBYTE)(((x->tileInfoBits & (~BITS_OCCUPIED))) | BITS_STRUCTURE))
#define SET_TILE_FEATURE(x)		(x->tileInfoBits = (UBYTE)(((x->tileInfoBits & (~BITS_OCCUPIED))) | BITS_FEATURE))
#define SET_TILE_EMPTY(x)		(x->tileInfoBits = (UBYTE) (x->tileInfoBits & (~BITS_OCCUPIED)) )
#define SET_TILE_NODRAW(x)		(x->tileInfoBits = (UBYTE)((x)->tileInfoBits | BITS_NODRAWTILE))
#define CLEAR_TILE_NODRAW(x)	(x->tileInfoBits = (UBYTE)((x)->tileInfoBits & (~BITS_NODRAWTILE)))
#define SET_TILE_HIGHLIGHT(x)	(x->texture = (UWORD)((x)->texture | TILE_HILIGHT))
#define CLEAR_TILE_HIGHLIGHT(x)	(x->texture = (UWORD)((x)->texture & (~TILE_HILIGHT)))
#define SET_TILE_TALLSTRUCTURE(x)	(x->tileInfoBits = (UBYTE)((x)->tileInfoBits | BITS_TALLSTRUCTURE))
#define CLEAR_TILE_TALLSTRUCTURE(x)	(x->tileInfoBits = (UBYTE)((x)->tileInfoBits & (~BITS_TALLSTRUCTURE)))
#define SET_TILE_SMALLSTRUCTURE(x)	(x->tileInfoBits = (UBYTE)((x)->tileInfoBits | BITS_SMALLSTRUCTURE))
#define CLEAR_TILE_SMALLSTRUCTURE(x)	(x->tileInfoBits = (UBYTE)((x)->tileInfoBits & (~BITS_SMALLSTRUCTURE)))

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

extern UBYTE terrainTypes[MAX_TILE_TEXTURES];

#define TERRAIN_TYPE(x) terrainTypes[x->texture & TILE_NUMMASK]

/* Information stored with each tile */
typedef struct _maptile
{
	UBYTE			tileInfoBits;
	UBYTE			tileVisBits;	// COMPRESSED - bit per player
	UBYTE			height;			// The height at the top left of the tile
	UBYTE			illumination;	// How bright is this tile?
	UWORD			texture;		// Which graphics texture is on this tile
	UBYTE			bMaxed;
	UBYTE			level;
	UBYTE			inRange;		// sensor range display.

									// This is also used to store the tile flip flags
//  What's been removed - 46 bytes per tile so far
//	BASE_OBJECT		*psObject;		// Any object sitting on the location (e.g. building)
//	UBYTE			onFire;			// Is tile on fire?
//	UBYTE			rippleIndex;	// Current value in ripple table?
//	BOOL			tileVisible[MAX_PLAYERS]; // Which players can see the tile?
//	BOOL			triangleFlip;	// Is the triangle flipped?
//	TYPE_OF_TERRAIN	type;			// The terrain type for the tile
} MAPTILE;



/* The maximum map size */

#define MAP_MAXWIDTH	256
#define MAP_MAXHEIGHT	256
#define MAP_MAXAREA		(256*256)

#define TILE_MAX_HEIGHT		(255 * ELEVATION_SCALE)
#define TILE_MIN_HEIGHT		  0

/* The size and contents of the map */
extern UDWORD	mapWidth, mapHeight;
extern MAPTILE *psMapTiles;

/* The shift on the y coord when calculating into the map */
extern UDWORD	mapShift;

/* The number of units accross a tile */
#define TILE_UNITS	128

/* The shift on a coordinate to get the tile coordinate */
#define TILE_SHIFT	7

/* The mask to get internal tile coords from a full coordinate */
#define TILE_MASK	0x7f

/* Shutdown the map module */
extern BOOL mapShutdown(void);

/* Create a new map of a specified size */
extern BOOL mapNew(UDWORD width, UDWORD height);

/* Load the map data */
extern BOOL mapLoad(char *pFileData, UDWORD fileSize);

/* Save the map data */
extern BOOL mapSave(char **ppFileData, UDWORD *pFileSize);

/* A post process for the water tiles in map to ensure height integrity */
extern void	mapWaterProcess( void );


/* Return a pointer to the tile structure at x,y */
static inline MAPTILE *mapTile(UDWORD x, UDWORD y)
{

	ASSERT( x < mapWidth,
		"mapTile: x coordinate bigger than map width" );
	ASSERT( y < mapHeight,
		"mapTile: y coordinate bigger than map height" );

	//return psMapTiles + x + (y << mapShift); //width no longer a power of 2
	return psMapTiles + x + (y * mapWidth);
}

/* Return height of tile at x,y */
static inline SWORD map_TileHeight(UDWORD x, UDWORD y)
{
	if((x>=mapWidth) || (y>=mapHeight)) {
		return 0;
	}
	return (SWORD)((psMapTiles[x + (y * mapWidth)].height) * ELEVATION_SCALE);
}

/*sets the tile height */
static inline void setTileHeight(UDWORD x, UDWORD y, UDWORD height)
{

	ASSERT( x < mapWidth,
		"mapTile: x coordinate bigger than map width" );
	ASSERT( y < mapHeight,
		"mapTile: y coordinate bigger than map height" );

	//psMapTiles[x + (y << mapShift)].height = height;//width no longer a power of 2
	psMapTiles[x + (y * mapWidth)].height = (UBYTE) (height / ELEVATION_SCALE);
}

/*increases the tile height by one */
/*static inline void incTileHeight(UDWORD x, UDWORD y)
{
	psMapTiles[x + (y << mapShift)].height++;
}*/

/*decreases the tile height by one */
/*static inline void decTileHeight(UDWORD x, UDWORD y)
{
	psMapTiles[x + (y << mapShift)].height--;
}*/

/* Return whether a tile coordinate is on the map */
static inline BOOL tileOnMap(SDWORD x, SDWORD y)
{
	return (x >= 0) && (x < (SDWORD)mapWidth) && (y >= 0) && (y < (SDWORD)mapHeight);
}

/* Return whether a world coordinate is on the map */
static inline BOOL worldOnMap(SDWORD x, SDWORD y)
{
	return (x >= 0) && (x < ((SDWORD)mapWidth << TILE_SHIFT)) &&
		   (y >= 0) && (y < ((SDWORD)mapHeight << TILE_SHIFT));
}

/* Store a map coordinate and it's associated tile, used by mapCalcLine */
typedef struct _tile_coord
{
	UDWORD	x,y;
	MAPTILE	*psTile;
} TILE_COORD;

/* The map tiles generated by map calc line */
extern TILE_COORD	*aMapLinePoints;

/* work along a line on the map storing the points in aMapLinePoints.
 * pNumPoints is set to the number of points generated.
 * The start and end points are in TILE coordinates.
 */
extern void mapCalcLine(UDWORD startX, UDWORD startY,
						UDWORD endX, UDWORD endY,
						UDWORD *pNumPoints);

/* Same as mapCalcLine, but does a wider line in the map */
extern void mapCalcAALine(SDWORD X1, SDWORD Y1,
				   SDWORD X2, SDWORD Y2,
				   UDWORD *pNumPoints);

/* Return height of x,y */
//extern SDWORD map_Height(UDWORD x, UDWORD y);
extern SWORD map_Height(UDWORD x, UDWORD y);

/* returns TRUE if object is above ground */
extern BOOL mapObjIsAboveGround( BASE_OBJECT *psObj );

/* returns the max and min height of a tile by looking at the four corners
   in tile coords */
extern void getTileMaxMin(UDWORD x, UDWORD y, UDWORD *pMax, UDWORD *pMin);

MAPTILE *GetCurrentMap(void);	// returns a pointer to the current loaded map data
UDWORD GetHeightOfMap(void);
UDWORD GetWidthOfMap(void);
extern BOOL	readVisibilityData(char *pFileData, UDWORD fileSize);
extern BOOL	writeVisibilityData( char *pFileName );
extern void	mapFreeTilesAndStrips( void );

//scroll min and max values
extern SDWORD		scrollMinX, scrollMaxX, scrollMinY, scrollMaxY;
extern BOOL	bDoneWater;

#endif
