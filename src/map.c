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
 * @file map.c
 *
 * Utility functions for the map data structure.
 *
 */
#include <time.h>

#include "lib/framework/frame.h"
#include "lib/framework/endian_hack.h"
#include "lib/framework/file.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/tagfile.h"
#include "lib/ivis_common/tex.h"

#include "map.h"
#include "hci.h"
#include "projectile.h"
#include "display3d.h"
#include "lighting.h"
#include "game.h"
#include "texture.h"
#include "environ.h"
#include "advvis.h"
#include "research.h"
#include "mission.h"
#include "formationdef.h"
#include "gateway.h"
#include "wrappers.h"
#include "mapgrid.h"
#include "astar.h"
#include "fpath.h"
#include "levels.h"

struct ffnode
{
	struct ffnode *next;
	int x, y;
};

//scroll min and max values
SDWORD		scrollMinX, scrollMaxX, scrollMinY, scrollMaxY;

/* Structure definitions for loading and saving map data */
typedef struct _map_save_header
{
	char		aFileType[4];
	UDWORD		version;
	UDWORD		width;
	UDWORD		height;
} MAP_SAVEHEADER;

typedef struct _map_save_tilev2
{
	UWORD		texture;
	UBYTE		height;
} MAP_SAVETILE;

typedef struct _gateway_save_header
{
	UDWORD		version;
	UDWORD		numGateways;
} GATEWAY_SAVEHEADER;

typedef struct _gateway_save
{
	UBYTE	x0,y0,x1,y1;
} GATEWAY_SAVE;

typedef struct _zonemap_save_header {
	UWORD version;
	UWORD numZones;
	UWORD numEquivZones;
	UWORD pad;
} ZONEMAP_SAVEHEADER;

/* Sanity check definitions for the save struct file sizes */
#define SAVE_HEADER_SIZE	16
#define SAVE_TILE_SIZE		3
#define SAVE_TILE_SIZEV1	6
#define SAVE_TILE_SIZEV2	3

// Maximum expected return value from get height
#define	MAX_HEIGHT			(256 * ELEVATION_SCALE)

/* Number of entries in the sqrt(1/(1+x*x)) table for aaLine */
#define	ROOT_TABLE_SIZE		1024

/* The size and contents of the map */
UDWORD	mapWidth = 0, mapHeight = 0;
MAPTILE	*psMapTiles = NULL;

#define WATER_DEPTH	180

static void SetGroundForTile(const char *filename, const char *nametype);
static int getTextureType(const char *textureType);
static BOOL hasDecals(int i, int j);
static void SetDecals(const char *filename, const char *decal_type);
static void init_tileNames(int type);

/// The different ground types
GROUND_TYPE *psGroundTypes;
int numGroundTypes;
int waterGroundType;
int cliffGroundType;
char *tileset = NULL;
static int numTile_names;
static char *Tile_names = NULL;
#define ARIZONA 1
#define URBAN 2
#define ROCKIE 3

static int *map;			// 3D array pointer that holds the texturetype
static int *mapDecals;		// array that tells us what tile is a decal
#define MAX_TERRAIN_TILES 100		// max that we support (for now)

/* Look up table that returns the terrain type of a given tile texture */
UBYTE terrainTypes[MAX_TILE_TEXTURES];

/* Create a new map of a specified size */
BOOL mapNew(UDWORD width, UDWORD height)
{
	MAPTILE *psTile;
	UDWORD	i;

	/* See if a map has already been allocated */
	if (psMapTiles != NULL)
	{
		/* Clear all the objects off the map and free up the map memory */
		gwShutDown();
		releaseAllProxDisp();
		freeAllDroids();
		freeAllStructs();
		freeAllFeatures();
		freeAllFlagPositions();
		proj_FreeAllProjectiles();
		free(psMapTiles);
		psMapTiles = NULL;
		initStructLimits();
		
		free(psGroundTypes);
	}

	if (width*height > MAP_MAXAREA)
	{
		debug(LOG_ERROR, "Map too large : %u %u", width, height);
		return false;
	}

	if (width <=1 || height <=1)
	{
		debug(LOG_ERROR, "Map is too small : %u, %u", width, height);
		return false;
	}

	psMapTiles = calloc(width * height, sizeof(MAPTILE));
	if (psMapTiles == NULL)
	{
		debug(LOG_FATAL, "Out of memory");
		abort();
		return false;
	}

	psTile = psMapTiles;
	for (i = 0; i < width * height; i++)
	{
		psTile->height = MAX_HEIGHT / 4;
		psTile->illumination = 255;
		psTile->level = psTile->illumination;
		memset(psTile->watchers, 0, sizeof(psTile->watchers));
		psTile->colour= WZCOL_WHITE;
		CLEAR_TILE_EXPLORED(psTile);
		psTile++;
	}

	mapWidth = width;
	mapHeight = height;

	intSetMapPos(mapWidth * TILE_UNITS/2, mapHeight * TILE_UNITS/2);

	environReset();

	/*set up the scroll mins and maxs - set values to valid ones for a new map*/
	scrollMinX = scrollMinY = 0;
	scrollMaxX = mapWidth;
	scrollMaxY = mapHeight;

	gridReset();

	return true;
}

static void init_tileNames(int type)
{
	char	*pFileData = NULL;
	char	name[100] = {'\0'};
	int		numlines = 0, i = 0, cnt = 0;
	uint32_t	fileSize = 0;

	pFileData = fileLoadBuffer;

	switch (type)
	{
		case ARIZONA:
		{
			if (!loadFileToBuffer("tileset/arizona_enum.txt", pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
			{
				debug(LOG_FATAL, "tileset/arizona_enum.txt not found.  Aborting.");
				abort();
			}

			sscanf(pFileData, "%[^','],%d%n", name, &numlines, &cnt);
			pFileData += cnt;

			if (strcmp("arizona_enum", name))
			{
				debug(LOG_FATAL, "%s found, but was expecting arizona_enum, aborting.", name);
				abort();
			}
			break;
		}
		case URBAN:
		{
			if (!loadFileToBuffer("tileset/urban_enum.txt", pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
			{
				debug(LOG_FATAL, "tileset/urban_enum.txt not found.  Aborting.");
				abort();
			}

			sscanf(pFileData, "%[^','],%d%n", name, &numlines, &cnt);
			pFileData += cnt;

			if (strcmp("urban_enum", name))
			{
				debug(LOG_FATAL, "%s found, but was expecting urban_enum, aborting.", name);
				abort();
			}
			break;
		}
		case ROCKIE:
		{
			if (!loadFileToBuffer("tileset/rockie_enum.txt", pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
			{
				debug(LOG_FATAL, "tileset/rockie_enum.txt not found.  Aborting.");
				abort();
			}

			sscanf(pFileData, "%[^','],%d%n", name, &numlines, &cnt);
			pFileData += cnt;

			if (strcmp("rockie_enum", name))
			{
				debug(LOG_FATAL, "%s found, but was expecting rockie_enum, aborting.", name);
				abort();
			}
			break;
		}
		default:
		debug(LOG_FATAL, "Unknown type (%d) given.  Aborting.", type);
		abort();
	}

	debug(LOG_TERRAIN, "name: %s, with %d entries", name, numlines);
	if (numlines == 0 || numlines > MAX_TERRAIN_TILES)
	{
		debug(LOG_FATAL, "Rockie_enum paramater is out of range (%d). Aborting.", numlines);
		abort();
	}

	numTile_names = numlines;
	//increment the pointer to the start of the next record
	pFileData = strchr(pFileData,'\n') + 1;

	Tile_names = malloc(numlines * sizeof(char[40]) );
	memset(Tile_names, 0x0, (numlines * sizeof(char[40])));

	for (i=0; i < numlines; i++)
	{
		sscanf(pFileData, "%s%n", &Tile_names[i*40], &cnt);
		pFileData += cnt;
		//increment the pointer to the start of the next record
		pFileData = strchr(pFileData,'\n') + 1;
	}
}

// This is the main loading routine to get all the map's parameters set.
// Once it figures out what tileset we need, we then parse the files for that tileset.
// Currently, we only support 3 tilesets.  Arizona, Urban, and Rockie
static BOOL mapLoadGroundTypes(void)
{
	char	*pFileData = NULL;
	char	tilename[255] = {'\0'};
	char	textureName[255] = {'\0'};
	char	textureType[255] = {'\0'};
	double	textureSize = 0.f;
	int		numlines = 0;
	int		cnt = 0, i = 0;
	uint32_t	fileSize = 0;

	pFileData = fileLoadBuffer;

	debug(LOG_TERRAIN, "tileset: %s", tileset);
	// For Arizona
	if (strcmp(tileset, "texpages/tertilesc1hw") == 0)
	{
fallback:
		init_tileNames(ARIZONA);
		if (!loadFileToBuffer("tileset/tertilesc1hwGtype.txt", pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug(LOG_FATAL, "tileset/tertilesc1hwGtype.txt not found, aborting.");
			abort();
		}

		sscanf(pFileData, "%[^','],%d%n", tilename, &numlines, &cnt);
		pFileData += cnt;

		if (strcmp(tilename, "tertilesc1hw"))
		{
			debug(LOG_FATAL, "%s found, but was expecting tertilesc1hw!  Aborting.", tilename);
			abort();
		}

		debug(LOG_TERRAIN, "tilename: %s, with %d entries", tilename, numlines);
		//increment the pointer to the start of the next record
		pFileData = strchr(pFileData,'\n') + 1;
		numGroundTypes = numlines;
		psGroundTypes = malloc(sizeof(GROUND_TYPE)*numlines);

		for (i=0; i < numlines; i++)
		{
			sscanf(pFileData, "%[^','],%[^','],%lf%n", textureType, textureName, &textureSize, &cnt);
			pFileData += cnt;
			//increment the pointer to the start of the next record
			pFileData = strchr(pFileData,'\n') + 1;

			psGroundTypes[getTextureType(textureType)].textureName = strdup(textureName);
			psGroundTypes[getTextureType(textureType)].textureSize = textureSize ;
		}

		waterGroundType = getTextureType("a_water");
		cliffGroundType = getTextureType("a_cliff");

		SetGroundForTile("tileset/arizonaground.txt", "arizona_ground");
		SetDecals("tileset/arizonadecals.txt", "arizona_decals");
	}
	// for Urban
	else if (strcmp(tileset, "texpages/tertilesc2hw") == 0)
	{
		init_tileNames(URBAN);
		if (!loadFileToBuffer("tileset/tertilesc2hwGtype.txt", pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug(LOG_POPUP, "tileset/tertilesc2hwGtype.txt not found, using default terrain ground types.");
			goto fallback;
		}

		sscanf(pFileData, "%[^','],%d%n", tilename, &numlines, &cnt);
		pFileData += cnt;

		if (strcmp(tilename, "tertilesc2hw"))
		{
			debug(LOG_POPUP, "%s found, but was expecting tertilesc2hw!", tilename);
			goto fallback;
		}

		debug(LOG_TERRAIN, "tilename: %s, with %d entries", tilename, numlines);
		//increment the pointer to the start of the next record
		pFileData = strchr(pFileData,'\n') + 1;
		numGroundTypes = numlines;
		psGroundTypes = malloc(sizeof(GROUND_TYPE)*numlines);

		for (i=0; i < numlines; i++)
		{
			sscanf(pFileData, "%[^','],%[^','],%lf%n", textureType, textureName, &textureSize, &cnt);
			pFileData += cnt;
			//increment the pointer to the start of the next record
			pFileData = strchr(pFileData,'\n') + 1;

			psGroundTypes[getTextureType(textureType)].textureName = strdup(textureName);
			psGroundTypes[getTextureType(textureType)].textureSize = textureSize;
		}

		waterGroundType = getTextureType("u_water");
		cliffGroundType = getTextureType("u_cliff");

		SetGroundForTile("tileset/urbanground.txt", "urban_ground");
		SetDecals("tileset/urbandecals.txt", "urban_decals");
	}
	// for Rockie
	else if (strcmp(tileset, "texpages/tertilesc3hw") == 0)
	{
		init_tileNames(ROCKIE);
		if (!loadFileToBuffer("tileset/tertilesc3hwGtype.txt", pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug(LOG_POPUP, "tileset/tertilesc3hwGtype.txt not found, using default terrain ground types.");
			goto fallback;
		}

		sscanf(pFileData, "%[^','],%d%n", tilename, &numlines, &cnt);
		pFileData += cnt;

		if (strcmp(tilename, "tertilesc3hw"))
		{
			debug(LOG_POPUP, "%s found, but was expecting tertilesc3hw!", tilename);
			goto fallback;
		}

		debug(LOG_TERRAIN, "tilename: %s, with %d entries", tilename, numlines);
		//increment the pointer to the start of the next record
		pFileData = strchr(pFileData,'\n') + 1;
		numGroundTypes = numlines;
		psGroundTypes = malloc(sizeof(GROUND_TYPE)*numlines);

		for (i=0; i < numlines; i++)
		{
			sscanf(pFileData, "%[^','],%[^','],%lf%n", textureType, textureName, &textureSize, &cnt);
			pFileData += cnt;
			//increment the pointer to the start of the next record
			pFileData = strchr(pFileData,'\n') + 1;

			psGroundTypes[getTextureType(textureType)].textureName = strdup(textureName);
			psGroundTypes[getTextureType(textureType)].textureSize = textureSize;
		}

		waterGroundType = getTextureType("r_water");
		cliffGroundType = getTextureType("r_cliff");

		SetGroundForTile("tileset/rockieground.txt", "rockie_ground");
		SetDecals("tileset/rockiedecals.txt", "rockie_decals");
	}
	// When a map uses something other than the above, we fallback to Arizona
	else
	{
		debug(LOG_ERROR, "unsupported tileset: %s", tileset);
		debug(LOG_POPUP, "This is a UNSUPPORTED map with a custom tileset.\nDefaulting to tertilesc1hw -- map may look strange!");
		// HACK: / FIXME: For now, we just pretend this is a tertilesc1hw map.
		goto fallback;
	}
	return true;
}

// Parse the file to set up the ground type
static void SetGroundForTile(const char *filename, const char *nametype)
{
	char	*pFileData = NULL;
	char	tilename[255] = {'\0'};
	char	val1[25], val2[25], val3[25], val4[25];
	int		numlines = 0;
	int		cnt = 0, i = 0;
	uint32_t	fileSize = 0;

	pFileData = fileLoadBuffer;
	if (!loadFileToBuffer(filename, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
	{
		debug(LOG_FATAL, "%s not found, aborting.", filename);
		abort();
	}

	sscanf(pFileData, "%[^','],%d%n", tilename, &numlines, &cnt);
	pFileData += cnt;

	if (strcmp(tilename, nametype))
	{
		debug(LOG_FATAL, "%s found, but was expecting %s, aborting.", tilename, nametype);
		abort();
	}

	debug(LOG_TERRAIN, "tilename: %s, with %d entries", tilename, numlines);
	//increment the pointer to the start of the next record
	pFileData = strchr(pFileData,'\n') + 1;

	map = malloc(sizeof(int) * numlines * 2 * 2 );	// this is a 3D array map[numlines][2][2]

	for (i=0; i < numlines; i++)
	{
		sscanf(pFileData, "%[^','],%[^','],%[^','],%s%n", val1, val2, val3, val4, &cnt);
		pFileData += cnt;
		//increment the pointer to the start of the next record
		pFileData = strchr(pFileData,'\n') + 1;

		// inline int iA(int i, int j, int k){ return i*N2*N3 + j*N3 + k; }
		// in case it isn't obvious, this is a 3D array, and using pointer math to access each element.
		// so map[10][0][1] would be map[10*2*2 + 0 + 1] == map[41]
		// map[10][1][0] == map[10*2*2 + 2 + 0] == map[42]
		map[i*2*2+0*2+0] = getTextureType(val1);
		map[i*2*2+0*2+1] = getTextureType(val2);
		map[i*2*2+1*2+0] = getTextureType(val3);
		map[i*2*2+1*2+1] = getTextureType(val4);
	}
}

// getTextureType() -- just returns the value for that texture type.
static int getTextureType(const char *textureType)
{
	int i = 0;
	for (i=0; i < numTile_names; i++)
	{
		if (!strcmp(textureType, &Tile_names[i*40]))
		{
			return i;
		}
	}
	debug(LOG_FATAL, "unknown type [%s] found, aborting!", textureType);
	abort();
}

// groundFromMapTile() just a simple lookup table, using pointers to access the 3D map array
//	(quasi) pointer math is: map[num elements][2][2]
//	so map[10][0][1] would be map[10*2*2 + 0*2 + 1] == map[41]
static int groundFromMapTile(int tile, int j, int k)
{
	return map[TileNumber_tile(tile)* 2 * 2 + j * 2 + k];
}

static void rotFlip(int tile, int *i, int *j)
{
	int texture = TileNumber_texture(tile);
	int rot;
	int map[2][2], invmap[4][2];

	if (texture & TILE_XFLIP)
	{
		*i = 1 - *i;
	}
	if (texture & TILE_YFLIP)
	{
		*j = 1 - *j;
	}

	map[0][0] = 0; invmap[0][0] = 0; invmap[0][1] = 0;
	map[1][0] = 1; invmap[1][0] = 1; invmap[1][1] = 0;
	map[1][1] = 2; invmap[2][0] = 1; invmap[2][1] = 1;
	map[0][1] = 3; invmap[3][0] = 0; invmap[3][1] = 1;
	rot = map[*i][*j];
	rot -= (texture & TILE_ROTMASK) >> TILE_ROTSHIFT;
	while(rot < 0) rot += 4;
	*i = invmap[rot][0];
	*j = invmap[rot][1];
}

/// Tries to figure out what ground type a grid point is from the surrounding tiles
static int determineGroundType(int x, int y, const char *tileset)
{
	int ground[2][2];
	int votes[2][2];
	int i,j, tile;
	int a,b, best;
	BOOL arizona, rockies, urban;
	arizona = rockies = urban = false;
	if (strcmp(tileset, "texpages/tertilesc1hw") == 0)
	{
		arizona = true;
	} else if (strcmp(tileset, "texpages/tertilesc2hw") == 0)
	{
		urban = true;
	} else if (strcmp(tileset, "texpages/tertilesc3hw") == 0)
	{
		rockies = true;
	} else
	{
		debug(LOG_ERROR, "unknown tileset");
		return 0;
	}

	if (x <= 0 || y <= 0 || x >= mapWidth || y >= mapHeight)
	{
		return 0; // just return the first ground type
	}
	
	// check what tiles surround this grid point
	for(i=0;i<2;i++)
	{
		for(j=0;j<2;j++)
		{
			if (x+i-1 <= 0 || y+j-1 <= 0 || x+i-1 >= mapWidth || y+j-1 >= mapHeight)
			{
				tile = 0;
			}
			else
			{
				tile = mapTile(x+i-1, y+j-1)->texture;
			}
			a = i;
			b = j;
			rotFlip(tile, &a, &b);
			ground[i][j] = groundFromMapTile(tile, a, b);
			
			votes[i][j] = 0;

			// cliffs are so small they won't show up otherwise
			if (urban)
			{
				if (ground[i][j] == getTextureType("u_cliff"))
					return ground[i][j];
			}
			else if (arizona)
			{
				if (ground[i][j] == getTextureType("a_cliff"))
					return ground[i][j];
			}
			else if (rockies)
			{
				if (ground[i][j] == getTextureType("r_cliff"))
					return ground[i][j];
			}
		}
	}

	// now vote, because some maps have seams
	for(i=0;i<2;i++)
	{
		for(j=0;j<2;j++)
		{
			for(a=0;a<2;a++)
			{
				for(b=0;b<2;b++)
				{
					if (ground[i][j] == ground[a][b])
					{
						votes[i][j]++;
					}
				}
			}
		}
	}
	// and determine the winner
	best = -1;
	for(i=0;i<2;i++)
	{
		for(j=0;j<2;j++)
		{
			if (votes[i][j] > best)
			{
				best = votes[i][j];
				a = i;
				b = j;
			}
		}
	}
	return ground[a][b];
}

// SetDecals()
// reads in the decal array for the requested tileset.
static void SetDecals(const char *filename, const char *decal_type)
{
	char decalname[50], *pFileData;
	int numlines, cnt, i, tiledecal;
	uint32_t fileSize;

	pFileData = fileLoadBuffer;

	if (!loadFileToBuffer(filename, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
	{
		debug(LOG_POPUP, "%s not found, aborting.", filename);
		abort();
	}

	sscanf(pFileData, "%[^','],%d%n", decalname, &numlines, &cnt);
	pFileData += cnt;

	if (strcmp(decalname, decal_type))
	{
		debug(LOG_POPUP, "%s found, but was expecting %s, aborting.", decalname, decal_type);
		abort();
	}

	debug(LOG_TERRAIN, "reading: %s, with %d entries", filename, numlines);
	//increment the pointer to the start of the next record
	pFileData = strchr(pFileData,'\n') + 1;
	if (numlines > MAX_TERRAIN_TILES)
	{
		debug(LOG_FATAL, "Too many tiles, we only support %d max at this time", MAX_TERRAIN_TILES);
		abort();
	}
	mapDecals = malloc(sizeof(int)*MAX_TERRAIN_TILES);		// max of 80 tiles that we support
	memset(mapDecals, 0x0, sizeof(int)*MAX_TERRAIN_TILES);	// set everything to false;

	for (i=0; i < numlines; i++)
	{
		sscanf(pFileData, "%d%n", &tiledecal, &cnt);
		pFileData += cnt;
		//increment the pointer to the start of the next record
		pFileData = strchr(pFileData,'\n') + 1;
		mapDecals[tiledecal] = 1;
	}
}
// hasDecals()
// Checks to see if the requested tile has a decal on it or not.
static BOOL hasDecals(int i, int j)
{
	int index = 0;
	index = TileNumber_tile(mapTile(i, j)->texture);
	if (index > MAX_TERRAIN_TILES)
	{
		debug(LOG_FATAL, "Tile index is out of range!  Was %d, our max is %d", index, MAX_TERRAIN_TILES);
		abort();
	}
	return mapDecals[index];
}
// mapSetGroundTypes()
// Sets the ground type to be a decal or not
static BOOL mapSetGroundTypes(void)
{
	int i,j;

	for (i=0;i<mapWidth;i++)
	{
		for (j=0;j<mapHeight;j++)
		{
			MAPTILE *psTile = mapTile(i, j);

			psTile->ground = determineGroundType(i,j,tileset);

			if (hasDecals(i,j))
			{
				SET_TILE_DECAL(psTile);
			}
			else
			{
				CLEAR_TILE_DECAL(psTile);
			}
		}
	}
	return true;
}

/* Initialise the map structure */
BOOL mapLoad(char *filename)
{
	UDWORD		numGw, width, height;
	char		aFileType[4];
	UDWORD		version;
	UDWORD		i, j;
	PHYSFS_file	*fp = PHYSFS_openRead(filename);

	if (!fp)
	{
		debug(LOG_ERROR, "%s not found", filename);
		return false;
	}
	else if (PHYSFS_read(fp, aFileType, 4, 1) != 1
	    || !PHYSFS_readULE32(fp, &version)
	    || !PHYSFS_readULE32(fp, &width)
	    || !PHYSFS_readULE32(fp, &height)
	    || aFileType[0] != 'm'
	    || aFileType[1] != 'a'
	    || aFileType[2] != 'p')
	{
		debug(LOG_ERROR, "Bad header in %s", filename);
		goto failure;
	}
	else if (version <= VERSION_9)
	{
		debug(LOG_ERROR, "%s: Unsupported save format version %u", filename, version);
		goto failure;
	}
	else if (version > CURRENT_VERSION_NUM)
	{
		debug(LOG_ERROR, "%s: Undefined save format version %u", filename, version);
		goto failure;
	}
	else if (width * height > MAP_MAXAREA)
	{
		debug(LOG_ERROR, "Map %s too large : %d %d", filename, width, height);
		goto failure;
	}

	/* See if this is the first time a map has been loaded */
	ASSERT(psMapTiles == NULL, "Map has not been cleared before calling mapLoad()!");

	/* Allocate the memory for the map */
	psMapTiles = calloc(width * height, sizeof(MAPTILE));
	ASSERT(psMapTiles != NULL, "Out of memory" );

	mapWidth = width;
	mapHeight = height;
	
	// FIXME: the map preview code loads the map without setting the tileset
	if (!tileset)
	{
		debug(LOG_WARNING, "tileset not loaded, using arizona (map preview?)");
		tileset = strdup("texpages/tertilesc1hw");
	}
	
	// load the ground types
	if (!mapLoadGroundTypes())
	{
		goto failure;
		return false;
	}
	
	//load in the map data itself

	/* Load in the map data */
	for (i = 0; i < mapWidth * mapHeight; i++)
	{
		UWORD	texture;
		UBYTE	height;

		if (!PHYSFS_readULE16(fp, &texture) || !PHYSFS_readULE8(fp, &height))
		{
			debug(LOG_ERROR, "%s: Error during savegame load", filename);
			return false;
		}

		psMapTiles[i].texture = texture;
		psMapTiles[i].height = height;
		for (j = 0; j < MAX_PLAYERS; j++)
		{
			psMapTiles[i].tileVisBits =(UBYTE)(psMapTiles[i].tileVisBits &~ (UBYTE)(1 << j));
		}
	}

	if (!PHYSFS_readULE32(fp, &version) || !PHYSFS_readULE32(fp, &numGw) || version != 1)
	{
		debug(LOG_ERROR, "Bad gateway in %s", filename);
		return false;
	}

	for (i = 0; i < numGw; i++)
	{
		UBYTE	x0, y0, x1, y1;

		if (!PHYSFS_readULE8(fp, &x0) || !PHYSFS_readULE8(fp, &y0) || !PHYSFS_readULE8(fp, &x1) || !PHYSFS_readULE8(fp, &y1))
		{
			debug(LOG_ERROR, "%s: Failed to read gateway info", filename);
			return false;
		}
		if (!gwNewGateway(x0, y0, x1, y1))
		{
			debug(LOG_ERROR, "%s: Unable to add gateway", filename);
			return false;
		}
	}

	if (!mapSetGroundTypes())
	{
		goto failure;
	}

	// reset the random water bottom heights
	environReset();
	
	// set the river bed
	for (i=0;i<mapWidth;i++)
	{
		for (j=0;j<mapHeight;j++)
		{
			setTileHeight(i, j, map_TileHeight(i, j));
			// FIXME: magic number
			mapTile(i, j)->waterLevel = mapTile(i, j)->height - world_coord(1) / 3.0f / (float)ELEVATION_SCALE;
			// lower riverbed
			if (mapTile(i, j)->ground == waterGroundType)
			{
				mapTile(i, j)->height = mapTile(i, j)->height - (WATER_DEPTH - 2.0f * environGetData(i, j)) / (float)ELEVATION_SCALE;
			}
		}
	}
	
	/* set up the scroll mins and maxs - set values to valid ones for any new map */
	scrollMinX = scrollMinY = 0;
	scrollMaxX = mapWidth;
	scrollMaxY = mapHeight;

	mapFloodFillContinents();

	PHYSFS_close(fp);
	return true;
	
failure:
	PHYSFS_close(fp);
	return false;
}

// Object macro group
static void objectSaveTagged(BASE_OBJECT *psObj)
{
	uint16_t v[MAX_PLAYERS], i;

	// not written: sDisplay

	tagWriteEnter(0x01, 1);
	tagWrite(0x01, psObj->type);
	tagWrite(0x02, psObj->id);
	v[0] = psObj->pos.x;
	v[1] = psObj->pos.y;
	v[2] = psObj->pos.z;
	tagWrite16v(0x03, 3, v);
	tagWritef(0x04, psObj->direction);
	tagWrites(0x05, psObj->pitch);
	tagWrites(0x06, psObj->roll);
	tagWrite(0x07, psObj->player);
	tagWrite(0x08, psObj->group);
	tagWrite(0x09, psObj->selected);
	tagWrite(0x0a, psObj->cluster);
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		v[i] = psObj->visible[i];
	}
	tagWrite16v(0x0b, MAX_PLAYERS, v);
	tagWrite(0x0c, psObj->died);
	tagWrite(0x0d, psObj->lastEmission);
	tagWriteBool(0x0e, psObj->inFire);
	tagWrite(0x0f, psObj->burnStart);
	tagWrite(0x10, psObj->burnDamage);
	tagWriteLeave(0x01);
}

static void objectSensorTagged(int sensorRange, int sensorPower, int ecmRange, int ecmPower)
{
	tagWriteEnter(0x02, 1);
	tagWrite(0x01, sensorRange);
	tagWrite(0x02, sensorPower);
	tagWrite(0x03, ecmRange);
	tagWrite(0x04, ecmPower);
	tagWriteLeave(0x02);
}

static void objectStatTagged(BASE_OBJECT *psObj, int body, int resistance)
{
	int i;

	tagWriteEnter(0x03, 1);
	tagWrite(0x01, body);
	tagWrite(0x02, WC_NUM_WEAPON_CLASSES);
	tagWriteEnter(0x03, NUM_HIT_SIDES);
	for (i = 0; i < NUM_HIT_SIDES; i++)
	{
		tagWrite(0x01, psObj->armour[i][WC_KINETIC]);
		tagWrite(0x02, psObj->armour[i][WC_HEAT]);
		tagWriteNext();
	}
	tagWriteLeave(0x03);
	tagWrite(0x04, resistance);
	tagWriteLeave(0x03);
}

static void objectWeaponTagged(int num, WEAPON *asWeaps, BASE_OBJECT **psTargets)
{
	int i;

	tagWriteEnter(0x04, num);
	for (i = 0; i < num; i++)
	{
		tagWrite(0x01, asWeaps[i].nStat);
		tagWrite(0x02, asWeaps[i].rotation);
		tagWrite(0x03, asWeaps[i].pitch);
		tagWrite(0x05, asWeaps[i].ammo);
		tagWrite(0x06, asWeaps[i].lastFired);
		tagWrite(0x07, asWeaps[i].recoilValue);
		if (psTargets[i] != NULL)
		{
			tagWrites(0x08, psTargets[i]->id); // else default -1
		}
		tagWriteNext();
	}
	tagWriteLeave(0x04);
}

static void droidSaveTagged(DROID *psDroid)
{
	int plr = psDroid->player;
	uint16_t v[ARRAY_SIZE(psDroid->asBits)], i, order[4], ammo[VTOL_MAXWEAPS];
	int32_t sv[2];
	float fv[3];

	/* common groups */

	objectSaveTagged((BASE_OBJECT *)psDroid); /* 0x01 */
	objectSensorTagged(psDroid->sensorRange, psDroid->sensorPower, 0, psDroid->ECMMod); /* 0x02 */
	objectStatTagged((BASE_OBJECT *)psDroid, psDroid->originalBody, psDroid->resistance); /* 0x03 */
	objectWeaponTagged(psDroid->numWeaps, psDroid->asWeaps, psDroid->psActionTarget);

	/* DROID GROUP */

	tagWriteEnter(0x0a, 1);
	tagWrite(0x01, psDroid->droidType);
	for (i = 0; i < ARRAY_SIZE(v); ++i)
	{
		v[i] = psDroid->asBits[i].nStat;
	}
	tagWrite16v(0x02, ARRAY_SIZE(v), v);
	// transporter droid in the mission list
	if (psDroid->droidType == DROID_TRANSPORTER && apsDroidLists[plr] == mission.apsDroidLists[plr])
	{
		tagWriteBool(0x03, true);
	}
	tagWrite(0x07, psDroid->weight);
	tagWrite(0x08, psDroid->baseSpeed);
	tagWriteString(0x09, psDroid->aName);
	tagWrite(0x0a, psDroid->body);
	tagWritef(0x0b, psDroid->experience);
	tagWrite(0x0c, psDroid->NameVersion);
	if (psDroid->psTarget)
	{
		tagWrites(0x0e, psDroid->psTarget->id); // else -1
	}
	if (psDroid->psTarStats)
	{
		tagWrites(0x0f, psDroid->psTarStats->ref); // else -1
	}
	if (psDroid->psBaseStruct)
	{
		tagWrites(0x10, psDroid->psBaseStruct->id); // else -1
	}
	// current order
	tagWrite(0x11, psDroid->order);
	order[0] = psDroid->orderX;
	order[1] = psDroid->orderY;
	order[2] = psDroid->orderX2;
	order[3] = psDroid->orderX2;
	tagWrite16v(0x12, 4, order);
	// queued orders
	tagWriteEnter(0x13, psDroid->listSize);
	for (i = 0; i < psDroid->listSize; i++)
	{
		tagWrite(0x01, psDroid->asOrderList[i].order);
		order[0] = psDroid->asOrderList[i].x;
		order[1] = psDroid->asOrderList[i].y;
		order[2] = psDroid->asOrderList[i].x2;
		order[3] = psDroid->asOrderList[i].y2;
		tagWrite16v(0x02, 4, order);
		tagWriteNext();
	}
	tagWriteLeave(0x13);
	if (psDroid->sMove.psFormation != NULL)
	{
		tagWrites(0x14, psDroid->sMove.psFormation->dir);
		tagWrites(0x15, psDroid->sMove.psFormation->x);
		tagWrites(0x16, psDroid->sMove.psFormation->y);
	} // else these are zero as by default
	// vtol ammo
	for (i = 0; i < VTOL_MAXWEAPS; i++)
	{
		ammo[i] = psDroid->sMove.iAttackRuns[i];
	}
	tagWrite16v(0x17, VTOL_MAXWEAPS, ammo);
	// other movement related stuff
	sv[0] = psDroid->sMove.DestinationX;
	sv[1] = psDroid->sMove.DestinationY;
	tagWrites32v(0x18, 2, sv);
	sv[0] = psDroid->sMove.srcX;
	sv[1] = psDroid->sMove.srcY;
	tagWrites32v(0x19, 2, sv);
	sv[0] = psDroid->sMove.targetX;
	sv[1] = psDroid->sMove.targetY;
	tagWrites32v(0x1a, 2, sv);
	fv[0] = psDroid->sMove.fx;
	fv[1] = psDroid->sMove.fy;
	fv[2] = psDroid->sMove.fz;
	tagWritefv(0x1b, 3, fv);
	tagWritef(0x1c, psDroid->sMove.speed);
	sv[0] = psDroid->sMove.boundX;
	sv[1] = psDroid->sMove.boundY;
	tagWrites32v(0x1d, 2, sv);
	v[0] = psDroid->sMove.bumpX;
	v[1] = psDroid->sMove.bumpY;
	tagWrite16v(0x1e, 2, v);
	tagWrites(0x1f, psDroid->sMove.moveDir);
	tagWrites(0x20, psDroid->sMove.bumpDir);
	tagWrite(0x21, psDroid->sMove.bumpTime);
	tagWrite(0x22, psDroid->sMove.lastBump);
	tagWrite(0x23, psDroid->sMove.pauseTime);
	tagWrite(0x24, psDroid->sMove.iVertSpeed);
	tagWriteEnter(0x25, psDroid->sMove.numPoints);
	for (i = 0; i < psDroid->sMove.numPoints; i++)
	{
		v[0] = psDroid->sMove.asPath[i].x;
		v[1] = psDroid->sMove.asPath[i].y;
		tagWrite16v(0x01, 2, v);
		tagWriteNext();
	}
	tagWriteLeave(0x25);
	tagWrite(0x26, psDroid->sMove.Status);
	tagWrite(0x27, psDroid->sMove.Position);

	tagWriteLeave(0x0a);
}

static void structureSaveTagged(STRUCTURE *psStruct)
{
	int stype = NUM_DIFF_BUILDINGS;

	if (psStruct->pFunctionality)
	{
		stype = psStruct->pStructureType->type;
	}

	/* common groups */

	objectSaveTagged((BASE_OBJECT *)psStruct); /* 0x01 */
	objectSensorTagged(psStruct->sensorRange, psStruct->sensorPower, 0, psStruct->ECMMod); /* 0x02 */
	objectStatTagged((BASE_OBJECT *)psStruct, psStruct->pStructureType->bodyPoints, psStruct->resistance); /* 0x03 */
	objectWeaponTagged(psStruct->numWeaps, psStruct->asWeaps, psStruct->psTarget);

	/* STRUCTURE GROUP */

	tagWriteEnter(0x0b, 1);
	tagWrite(0x01, psStruct->pStructureType->type);
	tagWrites(0x02, psStruct->currentPowerAccrued);
	tagWrite(0x03, psStruct->lastResistance);
	tagWrite(0x04, psStruct->targetted);
	tagWrite(0x05, psStruct->timeLastHit);
	tagWrite(0x06, psStruct->lastHitWeapon);
	tagWrite(0x07, psStruct->status);
	tagWrites(0x08, psStruct->currentBuildPts);
	tagWriteLeave(0x0b);

	/* Functionality groups */

	switch (psStruct->pStructureType->type)
	{
		case REF_FACTORY:
		case REF_CYBORG_FACTORY:
		case REF_VTOL_FACTORY:
		{
			FACTORY *psFactory = (FACTORY *)psStruct->pFunctionality;

			tagWriteEnter(0x0d, 1); // FACTORY GROUP
			tagWrite(0x01, psFactory->capacity);
			tagWrite(0x02, psFactory->quantity);
			tagWrite(0x03, psFactory->loopsPerformed);
			//tagWrite(0x04, psFactory->productionOutput); // not used in original code, recalculated instead
			tagWrite(0x05, psFactory->powerAccrued);
			if (psFactory->psSubject)
			{
				tagWrites(0x06, ((DROID_TEMPLATE *)psFactory->psSubject)->multiPlayerID);
			}
			tagWrite(0x07, psFactory->timeStarted);
			tagWrite(0x08, psFactory->timeToBuild);
			tagWrite(0x09, psFactory->timeStartHold);
			tagWrite(0x0a, psFactory->secondaryOrder);
			if (psFactory->psAssemblyPoint)
			{
				// since we save our type, the combination of type and number is enough
				// to find our flag back on load
				tagWrites(0x0b, psFactory->psAssemblyPoint->factoryInc);
			}
			if (psFactory->psCommander)
			{
				tagWrites(0x0c, psFactory->psCommander->id);
			}
			tagWriteLeave(0x0d);
		} break;
		case REF_RESEARCH:
		{
			RESEARCH_FACILITY *psResearch = (RESEARCH_FACILITY *)psStruct->pFunctionality;

			tagWriteEnter(0x0e, 1);
			tagWrite(0x01, psResearch->capacity); // number of upgrades it has
			tagWrite(0x02, psResearch->powerAccrued);
			tagWrite(0x03, psResearch->timeStartHold);
			if (psResearch->psSubject)
			{
				tagWrite(0x04, psResearch->psSubject->ref - REF_RESEARCH_START);
				tagWrite(0x05, psResearch->timeStarted);
			}
			tagWriteLeave(0x0e);
		} break;
		case REF_RESOURCE_EXTRACTOR:
		{
			RES_EXTRACTOR *psExtractor = (RES_EXTRACTOR *)psStruct->pFunctionality;

			tagWriteEnter(0x0f, 1);
			tagWrite(0x01, psExtractor->power);
			if (psExtractor->psPowerGen)
			{
				tagWrites(0x02, psExtractor->psPowerGen->id);
			}
			tagWriteLeave(0x0f);
		} break;
		case REF_POWER_GEN:
		{
			POWER_GEN *psPower = (POWER_GEN *)psStruct->pFunctionality;

			tagWriteEnter(0x10, 1);
			tagWrite(0x01, psPower->capacity); // number of upgrades
			tagWriteLeave(0x10);
		} break;
		case REF_REPAIR_FACILITY:
		{
			REPAIR_FACILITY *psRepair = (REPAIR_FACILITY *)psStruct->pFunctionality;
			FLAG_POSITION *psFlag = psRepair->psDeliveryPoint;

			tagWriteEnter(0x11, 1);
			tagWrite(0x01, psRepair->timeStarted);
			tagWrite(0x02, psRepair->powerAccrued);
			if (psRepair->psObj)
			{
				tagWrites(0x03, psRepair->psObj->id);
			}
			if (psFlag)
			{
				tagWrites(0x04, psFlag->factoryInc);
			}
			tagWriteLeave(0x11);
		} break;
		case REF_REARM_PAD:
		{
			REARM_PAD *psRearm = (REARM_PAD *)psStruct->pFunctionality;

			tagWriteEnter(0x12, 1);
			tagWrite(0x01, psRearm->reArmPoints);
			tagWrite(0x02, psRearm->timeStarted);
			tagWrite(0x03, psRearm->timeLastUpdated);
			if (psRearm->psObj)
			{
				tagWrites(0x04, psRearm->psObj->id);
			}
			tagWriteLeave(0x12);
		} break;
		case REF_HQ:
		case REF_FACTORY_MODULE:
		case REF_POWER_MODULE:
		case REF_DEFENSE:
		case REF_WALL:
		case REF_WALLCORNER:
		case REF_BLASTDOOR:
		case REF_RESEARCH_MODULE:
		case REF_COMMAND_CONTROL:
		case REF_BRIDGE:
		case REF_DEMOLISH:
		case REF_LAB:
		case REF_MISSILE_SILO:
		case REF_SAT_UPLINK:
		case NUM_DIFF_BUILDINGS:
		{
			// nothing
		} break;
	}
}

static void featureSaveTagged(FEATURE *psFeat)
{
	/* common groups */

	objectSaveTagged((BASE_OBJECT *)psFeat); /* 0x01 */
	objectStatTagged((BASE_OBJECT *)psFeat, psFeat->psStats->body, 0); /* 0x03 */

	/* FEATURE GROUP */

	tagWriteEnter(0x0c, 1);
	tagWrite(0x01, psFeat->startTime);
	tagWriteLeave(0x0c);
}

// the maximum number of flags: each factory type can have two, one for itself, and one for a commander
#define MAX_FLAGS (NUM_FLAG_TYPES * MAX_FACTORY * 2)

BOOL mapSaveTagged(char *pFileName)
{
	MAPTILE *psTile;
	GATEWAY *psCurrGate;
	int numGateways = 0, i = 0, x = 0, y = 0, plr, droids, structures, features;
	float cam[3];
	const char *definition = "tagdefinitions/savegame/map.def";
	DROID *psDroid;
	FEATURE *psFeat;
	STRUCTURE *psStruct;

	// find the number of non water gateways
	for (psCurrGate = gwGetGateways(); psCurrGate; psCurrGate = psCurrGate->psNext)
	{
		if (!(psCurrGate->flags & GWR_WATERLINK))
		{
			numGateways += 1;
		}
	}

	if (!tagOpenWrite(definition, pFileName))
	{
		debug(LOG_ERROR, "mapSaveTagged: Failed to create savegame %s", pFileName);
		return false;
	}
	debug(LOG_MAP, "Creating tagged savegame %s with definition %s:", pFileName, definition);

	tagWriteEnter(0x03, 1); // map info group
	tagWrite(0x01, mapWidth);
	tagWrite(0x02, mapHeight);
	tagWriteLeave(0x03);

	tagWriteEnter(0x04, 1); // camera info group
	cam[0] = player.p.x;
	cam[1] = player.p.y;
	cam[2] = player.p.z; /* Transform position to float */
	tagWritefv(0x01, 3, cam);
	cam[0] = player.r.x;
	cam[1] = player.r.y;
	cam[2] = player.r.z; /* Transform rotation to float */
	tagWritefv(0x02, 3, cam);
	debug(LOG_MAP, " * Writing player position(%d, %d, %d) and rotation(%d, %d, %d)",
	      (int)player.p.x, (int)player.p.y, (int)player.p.z, (int)player.r.x,
	      (int)player.r.y, (int)player.r.z);
	tagWriteLeave(0x04);

	tagWriteEnter(0x05, _TEX_INDEX - 1); // texture group
	for (i = 0; i < _TEX_INDEX - 1; i++)
	{
		tagWriteString(0x01, _TEX_PAGE[i].name);
		tagWriteNext(); // add repeating group separator
	}
	debug(LOG_MAP, " * Writing info about %d texture pages", (int)_TEX_INDEX - 1);
	tagWriteLeave(0x05);

	tagWriteEnter(0x0a, mapWidth * mapHeight); // tile group
	psTile = psMapTiles;
	for (i = 0, x = 0, y = 0; i < mapWidth * mapHeight; i++)
	{
		tagWrite(0x01, terrainType(psTile));
		tagWrite(0x02, TileNumber_tile(psTile->texture));
		tagWrite(0x03, TRI_FLIPPED(psTile));
		tagWrite(0x04, psTile->texture & TILE_XFLIP);
		tagWrite(0x05, psTile->texture & TILE_YFLIP);
		tagWrite(0x06, TileIsNotBlocking(psTile)); // Redundant, since already included in tileInfoBits
		tagWrite(0x08, psTile->height);
		tagWrite(0x09, psTile->tileVisBits);
		tagWrite(0x0a, psTile->tileInfoBits);
		tagWrite(0x0b, (psTile->texture & TILE_ROTMASK) >> TILE_ROTSHIFT);

		psTile++;
		x++;
		if (x == mapWidth)
		{
			x = 0; y++;
		}
		tagWriteNext();
	}
	debug(LOG_MAP, " * Writing info about %d tiles", (int)mapWidth * mapHeight);
	tagWriteLeave(0x0a);

	tagWriteEnter(0x0b, numGateways); // gateway group
	for (psCurrGate = gwGetGateways(); psCurrGate; psCurrGate = psCurrGate->psNext)
	{
		if (!(psCurrGate->flags & GWR_WATERLINK))
		{
			uint16_t p[4];

			p[0] = psCurrGate->x1;
			p[1] = psCurrGate->y1;
			p[2] = psCurrGate->x2;
			p[3] = psCurrGate->y2;
			tagWrite16v(0x01, 4, p);
			tagWriteNext();
		}
	}
	debug(LOG_MAP, " * Writing info about %d gateways", (int)numGateways);
	tagWriteLeave(0x0b);

	tagWriteEnter(0x0c, MAX_TILE_TEXTURES); // terrain type <=> texture mapping group
	for (i = 0; i < MAX_TILE_TEXTURES; i++)
        {
                tagWrite(0x01, terrainTypes[i]);
		tagWriteNext();
        }
	debug(LOG_MAP, " * Writing info about %d textures' type", (int)MAX_TILE_TEXTURES);
	tagWriteLeave(0x0c);

	tagWriteEnter(0x0d, MAX_PLAYERS); // player info group
	for (plr = 0; plr < MAX_PLAYERS; plr++)
	{
		RESEARCH *psResearch = asResearch;
		STRUCTURE_STATS *psStructStats = asStructureStats;
		FLAG_POSITION *psFlag;
		FLAG_POSITION *flagList[MAX_FLAGS];
		MESSAGE *psMessage;

		memset(flagList, 0, sizeof(flagList));

		tagWriteEnter(0x01, numResearch); // research group
		for (i = 0; i < numResearch; i++, psResearch++)
		{
			tagWrite(0x01, IsResearchPossible(&asPlayerResList[plr][i]));
			tagWrite(0x02, asPlayerResList[plr][i].ResearchStatus & RESBITS);
			tagWrite(0x03, asPlayerResList[plr][i].currentPoints);
			tagWriteNext();
		}
		tagWriteLeave(0x01);

		tagWriteEnter(0x02, numStructureStats); // structure limits group
		for (i = 0; i < numStructureStats; i++, psStructStats++)
		{
			tagWriteString(0x01, psStructStats->pName);
			tagWrite(0x02, asStructLimits[plr][i].limit);
			tagWriteNext();
		}
		tagWriteLeave(0x02);

		// count and list flags in list
		for (i = 0, psFlag = apsFlagPosLists[plr]; psFlag != NULL; psFlag = psFlag->psNext)
		{
			ASSERT(i < MAX_FLAGS, "More flags than we can handle (1)!");
			flagList[i] = psFlag;
			i++;
		}
		y = i; // remember separation point between registered and unregistered flags
		// count flags not in list (commanders)
		for (psStruct = apsStructLists[plr]; psStruct != NULL; psStruct = psStruct->psNext)
		{
			ASSERT(i < MAX_FLAGS, "More flags than we can handle (2)!");
			if (psStruct->pFunctionality
			    && (psStruct->pStructureType->type == REF_FACTORY
			        || psStruct->pStructureType->type == REF_CYBORG_FACTORY
			        || psStruct->pStructureType->type == REF_VTOL_FACTORY))
			{
				FACTORY *psFactory = ((FACTORY *)psStruct->pFunctionality);

				if (psFactory->psCommander && psFactory->psAssemblyPoint != NULL)
				{
					flagList[i] = psFactory->psAssemblyPoint;
					i++;
				}
			}
		}
		tagWriteEnter(0x03, i); // flag group
		for (x = 0; x < i; x++)
		{
			uint16_t p[3];

			tagWrite(0x01, flagList[x]->type);
			tagWrite(0x02, flagList[x]->frameNumber);
			p[0] = flagList[x]->screenX;
			p[1] = flagList[x]->screenY;
			p[2] = flagList[x]->screenR;
			tagWrite16v(0x03, 3, p);
			tagWrite(0x04, flagList[x]->player);
			tagWriteBool(0x05, flagList[x]->selected);
			p[0] = flagList[x]->coords.x;
			p[1] = flagList[x]->coords.y;
			p[2] = flagList[x]->coords.z;
			tagWrite16v(0x06, 3, p);
			tagWrite(0x07, flagList[x]->factoryInc);
			tagWrite(0x08, flagList[x]->factoryType);
			if (flagList[x]->factoryType == REPAIR_FLAG)
			{
				tagWrite(0x09, getRepairIdFromFlag(flagList[x]));
			}
			if (x >= y)
			{
				tagWriteBool(0x0a, 1); // do not register this flag in flag list
			}
			tagWriteNext();
		}
		tagWriteLeave(0x03);

		// FIXME: Structured after old savegame logic, but surely it must be possible
		// to simplify the mess below. It refers to non-saved file containing messages.
		for (psMessage = apsMessages[plr], i = 0; psMessage != NULL; psMessage = psMessage->psNext, i++);
		tagWriteEnter(0x04, i); // message group
		for (psMessage = apsMessages[plr]; psMessage != NULL; psMessage = psMessage->psNext)
		{
			if (psMessage->type == MSG_PROXIMITY)
			{
				PROXIMITY_DISPLAY *psProx = apsProxDisp[plr];

				// find the matching proximity message
				for (; psProx != NULL && psProx->psMessage != psMessage; psProx = psProx->psNext);

				ASSERT(psProx != NULL, "Save message; proximity display not found for message");
				if (psProx->type == POS_PROXDATA)
				{
					// message has viewdata so store the name
					VIEWDATA *pViewData = (VIEWDATA*)psMessage->pViewData;

					tagWriteString(0x02, pViewData->pName);
				}
				else
				{
					BASE_OBJECT *psObj = (BASE_OBJECT*)psMessage->pViewData;

					tagWrites(0x01, psObj->id);
				}
			}
			else
			{
				VIEWDATA *pViewData = (VIEWDATA*)psMessage->pViewData;

				tagWriteString(0x2, pViewData->pName);
			}
			tagWriteBool(0x3, psMessage->read);
			tagWriteNext();
		}
		tagWriteLeave(0x04);

		tagWriteNext();
	}
	debug(LOG_MAP, " * Writing info about %d players", (int)MAX_PLAYERS);
	tagWriteLeave(0x0d);

	tagWriteEnter(0x0e, NUM_FACTORY_TYPES); // production runs group
	for (i = 0; i < NUM_FACTORY_TYPES; i++)
	{
		int factoryNum, runNum;

		tagWriteEnter(0x01, MAX_FACTORY);
		for (factoryNum = 0; factoryNum < MAX_FACTORY; factoryNum++)
		{
			tagWriteEnter(0x01, MAX_PROD_RUN);
			for (runNum = 0; runNum < MAX_PROD_RUN; runNum++)
			{
				PRODUCTION_RUN *psCurrentProd = &asProductionRun[i][factoryNum][runNum];

				tagWrite(0x01, psCurrentProd->quantity);
				tagWrite(0x02, psCurrentProd->built);
				if (psCurrentProd->psTemplate != NULL)
                                {
					tagWrites(0x03, psCurrentProd->psTemplate->multiPlayerID); // -1 if none
                                }
				tagWriteNext();
			}
			tagWriteLeave(0x01);
			tagWriteNext();
		}
		tagWriteLeave(0x01);
		tagWriteNext();
	}
	tagWriteLeave(0x0e);

	objCount(&droids, &structures, &features);
	tagWriteEnter(0x0f, droids + structures + features); // object group
	for (plr = 0; plr < MAX_PLAYERS; plr++)
	{
		for (psDroid = apsDroidLists[plr]; psDroid != NULL; psDroid = psDroid->psNext)
		{
			droidSaveTagged(psDroid);
			tagWriteNext();
			if (psDroid->droidType == DROID_TRANSPORTER)
			{
				DROID *psTrans = psDroid->psGroup->psList;
				for(psTrans = psTrans->psGrpNext; psTrans != NULL; psTrans = psTrans->psGrpNext)
				{
					droidSaveTagged(psTrans);
					tagWriteNext();
				}
			}
		}
		for (psStruct = apsStructLists[plr]; psStruct; psStruct = psStruct->psNext)
		{
			structureSaveTagged(psStruct);
			tagWriteNext();
		}
	}
	for (psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
	{
		featureSaveTagged(psFeat);
		tagWriteNext();
	}
	tagWriteLeave(0x0f);

	tagClose();
	return true;
}

BOOL mapLoadTagged(char *pFileName)
{
	int count, i, mapx, mapy;
	float cam[3];
	const char *definition = "tagdefinitions/savegame/map.def";
	MAPTILE	*psTile;

	if (!tagOpenRead(definition, pFileName))
	{
		debug(LOG_ERROR, "mapLoadTagged: Failed to open savegame %s", pFileName);
		return false;
	}
	debug(LOG_MAP, "Reading tagged savegame %s with definition %s:", pFileName, definition);

	tagReadEnter(0x03); // map info group
	mapx = tagRead(0x01);
	mapy = tagRead(0x02);
	debug(LOG_MAP, " * Map size: %d, %d", (int)mapx, (int)mapy);
	ASSERT(mapx == mapWidth && mapy == mapHeight, "mapLoadTagged: Wrong map size");
	ASSERT(mapWidth * mapHeight <= MAP_MAXAREA, "mapLoadTagged: Map too large");
	tagReadLeave(0x03);

	tagReadEnter(0x04); // camera info group
	tagReadfv(0x01, 3, cam); debug(LOG_MAP, " * Camera position: %f, %f, %f", cam[0], cam[1], cam[2]);
	tagReadfv(0x02, 3, cam); debug(LOG_MAP, " * Camera rotation: %f, %f, %f", cam[0], cam[1], cam[2]);
	tagReadLeave(0x04);

	count = tagReadEnter(0x05); // texture group
	for (i = 0; i < count; i++)
	{
		char mybuf[200];

		tagReadString(0x01, 200, mybuf);
		debug(LOG_MAP, " * Texture[%d]: %s", i, mybuf);
		tagReadNext();
	}
	tagReadLeave(0x05);

	i = tagReadEnter(0x0a); // tile group
	ASSERT(i == mapWidth * mapHeight, "Map size (%d, %d) is not equal to number of tiles (%d) in mapLoadTagged()!",
	       (int)mapWidth, (int)mapHeight, i);
	psTile = psMapTiles;
	for (i = 0; i < mapWidth * mapHeight; i++)
	{
		BOOL triflip, notblock, xflip, yflip;
		int texture, height, terrain;

		terrain = tagRead(0x01); ASSERT(terrainType(psTile) == terrain, "Wrong terrain");
		texture = tagRead(0x02); ASSERT(TileNumber_tile(psTile->texture) == texture, "Wrong texture");
		triflip = tagRead(0x03);
		xflip = tagRead(0x04);
		yflip = tagRead(0x05);
		notblock = tagRead(0x06);
		height = tagRead(0x08); ASSERT(psTile->height == height, "Wrong height");

		psTile++;
		tagReadNext();
	}
	tagReadLeave(0x0a);

	tagClose();
	return true;
}

/* Save the map data */
BOOL mapSave(char **ppFileData, UDWORD *pFileSize)
{
	UDWORD	i;
	MAP_SAVEHEADER	*psHeader = NULL;
	MAP_SAVETILE	*psTileData = NULL;
	MAPTILE	*psTile = NULL;
	GATEWAY *psCurrGate = NULL;
	GATEWAY_SAVEHEADER *psGateHeader = NULL;
	GATEWAY_SAVE *psGate = NULL;
	SDWORD	numGateways = 0;
	ZONEMAP_SAVEHEADER *psZoneHeader = NULL;

	// find the number of non water gateways
	for(psCurrGate = gwGetGateways(); psCurrGate; psCurrGate = psCurrGate->psNext)
	{
		if (!(psCurrGate->flags & GWR_WATERLINK))
		{
			numGateways += 1;
		}
	}

	/* Allocate the data buffer */
	*pFileSize = SAVE_HEADER_SIZE + mapWidth*mapHeight * SAVE_TILE_SIZE;
	// Add on the size of the gateway data.
	*pFileSize += sizeof(GATEWAY_SAVEHEADER) + sizeof(GATEWAY_SAVE)*numGateways;
	// Add on the size of the zone data header. For backwards compatibility.
	*pFileSize += sizeof(ZONEMAP_SAVEHEADER);

	*ppFileData = (char*)malloc(*pFileSize);
	if (*ppFileData == NULL)
	{
		debug( LOG_FATAL, "Out of memory" );
		abort();
		return false;
	}

	/* Put the file header on the file */
	psHeader = (MAP_SAVEHEADER *)*ppFileData;
	psHeader->aFileType[0] = 'm';
	psHeader->aFileType[1] = 'a';
	psHeader->aFileType[2] = 'p';
	psHeader->aFileType[3] = ' ';
	psHeader->version = CURRENT_VERSION_NUM;
	psHeader->width = mapWidth;
	psHeader->height = mapHeight;

	/* MAP_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->width);
	endian_udword(&psHeader->height);

	/* Put the map data into the buffer */
	psTileData = (MAP_SAVETILE *)(*ppFileData + SAVE_HEADER_SIZE);
	psTile = psMapTiles;
	for(i=0; i<mapWidth*mapHeight; i++)
	{
		psTileData->texture = psTile->texture;
		psTileData->height = psTile->height;

		/* MAP_SAVETILE */
		endian_uword(&psTileData->texture);

		psTileData = (MAP_SAVETILE *)((UBYTE *)psTileData + SAVE_TILE_SIZE);
		psTile ++;
	}

	// Put the gateway header.
	psGateHeader = (GATEWAY_SAVEHEADER*)psTileData;
	psGateHeader->version = 1;
	psGateHeader->numGateways = numGateways;

	/* GATEWAY_SAVEHEADER */
	endian_udword(&psGateHeader->version);
	endian_udword(&psGateHeader->numGateways);

	psGate = (GATEWAY_SAVE*)(psGateHeader+1);

	i=0;
	// Put the gateway data.
	for(psCurrGate = gwGetGateways(); psCurrGate; psCurrGate = psCurrGate->psNext)
	{
		if (!(psCurrGate->flags & GWR_WATERLINK))
		{
			psGate->x0 = psCurrGate->x1;
			psGate->y0 = psCurrGate->y1;
			psGate->x1 = psCurrGate->x2;
			psGate->y1 = psCurrGate->y2;
			psGate++;
			i++;
		}
	}

	// Put the zone header.
	psZoneHeader = (ZONEMAP_SAVEHEADER*)psGate;
	psZoneHeader->version = 3;
	psZoneHeader->numZones = 0;
	psZoneHeader->numEquivZones = 0;

	/* ZONEMAP_SAVEHEADER */
	endian_uword(&psZoneHeader->version);
	endian_uword(&psZoneHeader->numZones);
	endian_uword(&psZoneHeader->numEquivZones);
	endian_uword(&psZoneHeader->pad);

	return true;
}

/* Shutdown the map module */
BOOL mapShutdown(void)
{
	if (psMapTiles)
	{
		free(psMapTiles);
	}
	if (mapDecals)
	{
		free(mapDecals);
	}
	if (psGroundTypes)
	{
		free(psGroundTypes);
	}
	if (map)
	{
		free(map);
	}
	if (Tile_names)
	{
		free(Tile_names);
	}

	map = NULL;
	psGroundTypes = NULL;
	mapDecals = NULL;
	psMapTiles = NULL;
	mapWidth = mapHeight = 0;
	numTile_names = 0;
	Tile_names = NULL;
	return true;
}

/* Return linear interpolated height of x,y */
extern SWORD map_Height(int x, int y)
{
	int	retVal, tileX, tileY, tileYOffset, tileX2, tileY2Offset, dx, dy, ox, oy;
	int	h0, hx, hy, hxy, wTL = 0, wTR = 0, wBL = 0, wBR = 0;
	BOOL	bWaterTile = false;

	// Clamp x and y values to actual ones
	// Give one tile worth of leeway before asserting, for units/transporters coming in from off-map.
	ASSERT(x >= -TILE_UNITS, "map_Height: x value is too small (%d,%d) in %dx%d",map_coord(x),map_coord(y),mapWidth,mapHeight);
	ASSERT(y >= -TILE_UNITS, "map_Height: y value is too small (%d,%d) in %dx%d",map_coord(x),map_coord(y),mapWidth,mapHeight);
	x = (x < 0 ? 0 : x);
	y = (y < 0 ? 0 : y);
	ASSERT(x < world_coord(mapWidth)+TILE_UNITS, "map_Height: x value is too big (%d,%d) in %dx%d",map_coord(x),map_coord(y),mapWidth,mapHeight);
	ASSERT(y < world_coord(mapHeight)+TILE_UNITS, "map_Height: y value is too big (%d,%d) in %dx%d",map_coord(x),map_coord(y),mapWidth,mapHeight);
	x = (x >= world_coord(mapWidth) ? world_coord(mapWidth) - 1 : x);
	y = (y >= world_coord(mapHeight) ? world_coord(mapHeight) - 1 : y);

	/* Turn into tile coordinates */
	tileX = map_coord(x);
	tileY = map_coord(y);

	/* Inter tile comp */
	ox = map_round(x);
	oy = map_round(y);

	if (terrainType(mapTile(tileX,tileY)) == TER_WATER)
	{
		bWaterTile = true;
		wTL = environGetValue(tileX,tileY)/2;
		wTR = environGetValue(tileX+1,tileY)/2;
		wBL = environGetValue(tileX,tileY+1)/2;
		wBR = environGetValue(tileX+1,tileY+1)/2;
	}

	// to account for the border of the map
	if(tileX + 1 < mapWidth)
	{
		tileX2 = tileX + 1;
	}
	else
	{
		tileX2 = tileX;
	}
	tileYOffset = (tileY * mapWidth);
	if(tileY + 1 < mapHeight)
	{
		tileY2Offset = tileYOffset + mapWidth;
	}
	else
	{
		tileY2Offset = tileYOffset;
	}

	ASSERT( ox < TILE_UNITS, "mapHeight: x offset too big" );
	ASSERT( oy < TILE_UNITS, "mapHeight: y offset too big" );
	ASSERT( ox >= 0, "mapHeight: x offset too small" );
	ASSERT( oy >= 0, "mapHeight: y offset too small" );

	//different code for 4 different triangle cases
	if (psMapTiles[tileX + tileYOffset].texture & TILE_TRIFLIP)
	{
		if ((ox + oy) > TILE_UNITS)//tile split top right to bottom left object if in bottom right half
		{
			ox = TILE_UNITS - ox;
			oy = TILE_UNITS - oy;
			hy = psMapTiles[tileX + tileY2Offset].height;
			hx = psMapTiles[tileX2 + tileYOffset].height;
			hxy= psMapTiles[tileX2 + tileY2Offset].height;
			if(bWaterTile)
			{
				hy+=wBL;
				hx+=wTR;
				hxy+=wBR;
			}

			dx = ((hy - hxy) * ox )/ TILE_UNITS;
			dy = ((hx - hxy) * oy )/ TILE_UNITS;

			retVal = (SDWORD)(((hxy + dx + dy)) * ELEVATION_SCALE);
			ASSERT( retVal<MAX_HEIGHT,"Map height's gone weird!!!" );
			return ((SWORD)retVal);
		}
		else //tile split top right to bottom left object if in top left half
		{
			h0 = psMapTiles[tileX + tileYOffset].height;
			hy = psMapTiles[tileX + tileY2Offset].height;
			hx = psMapTiles[tileX2 + tileYOffset].height;

			if(bWaterTile)
			{
				h0+=wTL;
				hy+=wBL;
				hx+=wTR;
			}
			dx = ((hx - h0) * ox )/ TILE_UNITS;
			dy = ((hy - h0) * oy )/ TILE_UNITS;

			retVal = (SDWORD)((h0 + dx + dy) * ELEVATION_SCALE);
			ASSERT( retVal<MAX_HEIGHT,"Map height's gone weird!!!" );
			return ((SWORD)retVal);
		}
	}
	else
	{
		if (ox > oy) //tile split topleft to bottom right object if in top right half
		{
			h0 = psMapTiles[tileX + tileYOffset].height;
			hx = psMapTiles[tileX2 + tileYOffset].height;
			ASSERT( tileX2 + tileY2Offset < mapWidth*mapHeight, "array out of bounds");
			hxy= psMapTiles[tileX2 + tileY2Offset].height;

			if(bWaterTile)
			{
				h0+=wTL;
				hx+=wTR;
				hxy+=wBR;
			}
			dx = ((hx - h0) * ox )/ TILE_UNITS;
			dy = ((hxy - hx) * oy )/ TILE_UNITS;
			retVal = (SDWORD)(((h0 + dx + dy)) * ELEVATION_SCALE);
			ASSERT( retVal<MAX_HEIGHT,"Map height's gone weird!!!" );
			return ((SWORD)retVal);
		}
		else //tile split topleft to bottom right object if in bottom left half
		{
			h0 = psMapTiles[tileX + tileYOffset].height;
			hy = psMapTiles[tileX + tileY2Offset].height;
			hxy = psMapTiles[tileX2 + tileY2Offset].height;

			if(bWaterTile)
			{
				h0+=wTL;
				hy+=wBL;
				hxy+=wBR;
			}
			dx = ((hxy - hy) * ox )/ TILE_UNITS;
			dy = ((hy - h0) * oy )/ TILE_UNITS;

			retVal = (SDWORD)((h0 + dx + dy) * ELEVATION_SCALE);
			ASSERT( retVal<MAX_HEIGHT,"Map height's gone weird!!!" );
			return ((SWORD)retVal);
		}
	}
	return 0;
}

/* returns true if object is above ground */
extern BOOL mapObjIsAboveGround( BASE_OBJECT *psObj )
{
	// min is used to make sure we don't go over array bounds!
	SDWORD	iZ,
			tileX = map_coord(psObj->pos.x),
			tileY = map_coord(psObj->pos.y),
			tileYOffset1 = (tileY * mapWidth),
			tileYOffset2 = ((tileY+1) * mapWidth),
			h1 = psMapTiles[MIN(mapWidth * mapHeight, tileYOffset1 + tileX)    ].height,
			h2 = psMapTiles[MIN(mapWidth * mapHeight, tileYOffset1 + tileX + 1)].height,
			h3 = psMapTiles[MIN(mapWidth * mapHeight, tileYOffset2 + tileX)    ].height,
			h4 = psMapTiles[MIN(mapWidth * mapHeight, tileYOffset2 + tileX + 1)].height;

	/* trivial test above */
	if ( (psObj->pos.z > h1) && (psObj->pos.z > h2) &&
		 (psObj->pos.z > h3) && (psObj->pos.z > h4)    )
	{
		return true;
	}

	/* trivial test below */
	if ( (psObj->pos.z <= h1) && (psObj->pos.z <= h2) &&
		 (psObj->pos.z <= h3) && (psObj->pos.z <= h4)    )
	{
		return false;
	}

	/* exhaustive test */
	iZ = map_Height( psObj->pos.x, psObj->pos.y );
	if ( psObj->pos.z > iZ )
	{
		return true;
	}
	else
	{
		return false;
	}
}

/* returns the max and min height of a tile by looking at the four corners
   in tile coords */
void getTileMaxMin(UDWORD x, UDWORD y, UDWORD *pMax, UDWORD *pMin)
{
	UDWORD	height, i, j;

	*pMin = TILE_MAX_HEIGHT;
	*pMax = TILE_MIN_HEIGHT;

	for (j=0; j < 2; j++)
	{
		for (i=0; i < 2; i++)
		{
			height = map_TileHeight(x+i, y+j);
			if (*pMin > height)
			{
				*pMin = height;
			}
			if (*pMax < height)
			{
				*pMax = height;
			}
		}
	}
}

UDWORD GetWidthOfMap(void)
{
	return mapWidth;
}



UDWORD GetHeightOfMap(void)
{
	return mapHeight;
}


// -----------------------------------------------------------------------------------
/* This will save out the visibility data */
bool writeVisibilityData(const char* fileName)
{
	unsigned int i;
	VIS_SAVEHEADER fileHeader;

	PHYSFS_file* fileHandle = openSaveFile(fileName);
	if (!fileHandle)
	{
		return false;
	}

	fileHeader.aFileType[0] = 'v';
	fileHeader.aFileType[1] = 'i';
	fileHeader.aFileType[2] = 's';
	fileHeader.aFileType[3] = 'd';

	fileHeader.version = CURRENT_VERSION_NUM;

	// Write out the current file header
	if (PHYSFS_write(fileHandle, fileHeader.aFileType, sizeof(fileHeader.aFileType), 1) != 1
	 || !PHYSFS_writeUBE32(fileHandle, fileHeader.version))
	{
		debug(LOG_ERROR, "writeVisibilityData: could not write header to %s; PHYSFS error: %s", fileName, PHYSFS_getLastError());
		PHYSFS_close(fileHandle);
		return false;
	}

	for (i = 0; i < mapWidth * mapHeight; ++i)
	{
		if (!PHYSFS_writeUBE8(fileHandle, psMapTiles[i].tileVisBits))
		{
			debug(LOG_ERROR, "writeVisibilityData: could not write to %s; PHYSFS error: %s", fileName, PHYSFS_getLastError());
			PHYSFS_close(fileHandle);
			return false;
		}
	}

	// Everything is just fine!
	PHYSFS_close(fileHandle);
	return true;
}

// -----------------------------------------------------------------------------------
/* This will read in the visibility data */
bool readVisibilityData(const char* fileName)
{
	VIS_SAVEHEADER fileHeader;
	unsigned int expectedFileSize, fileSize;
	unsigned int i;

	PHYSFS_file* fileHandle = openLoadFile(fileName, false);
	if (!fileHandle)
	{
		// Failure to open the file is no failure to read it
		return true;
	}

	// Read the header from the file
	if (PHYSFS_read(fileHandle, fileHeader.aFileType, sizeof(fileHeader.aFileType), 1) != 1
	 || !PHYSFS_readUBE32(fileHandle, &fileHeader.version))
	{
		debug(LOG_ERROR, "readVisibilityData: error while reading header from file: %s", PHYSFS_getLastError());
		PHYSFS_close(fileHandle);
		return false;
	}

	// Check the header to see if we've been given a file of the right type
	if (fileHeader.aFileType[0] != 'v'
	 || fileHeader.aFileType[1] != 'i'
	 || fileHeader.aFileType[2] != 's'
	 || fileHeader.aFileType[3] != 'd')
	{
		debug(LOG_ERROR, "readVisibilityData: Weird file type found? Has header letters - '%c' '%c' '%c' '%c' (should be 'v' 'i' 's' 'd')",
		      fileHeader.aFileType[0],
		      fileHeader.aFileType[1],
		      fileHeader.aFileType[2],
		      fileHeader.aFileType[3]);

		PHYSFS_close(fileHandle);
		return false;
	}

	// Validate the filesize
	expectedFileSize = sizeof(fileHeader.aFileType) + sizeof(fileHeader.version) + mapWidth * mapHeight * sizeof(uint8_t);
	fileSize = PHYSFS_fileLength(fileHandle);
	if (fileSize != expectedFileSize)
	{
		PHYSFS_close(fileHandle);
		ASSERT(!"readVisibilityData: unexpected filesize", "readVisibilityData: unexpected filesize; should be %u, but is %u", expectedFileSize, fileSize);

		return false;
	}

	// For every tile...
	for(i=0; i<mapWidth*mapHeight; i++)
	{
		/* Get the visibility data */
		if (!PHYSFS_readUBE8(fileHandle, &psMapTiles[i].tileVisBits))
		{
			debug(LOG_ERROR, "readVisibilityData: could not read from %s; PHYSFS error: %s", fileName, PHYSFS_getLastError());
			PHYSFS_close(fileHandle);
			return false;
		}
	}

	// Close the file
	PHYSFS_close(fileHandle);

	/* Hopefully everything's just fine by now */
	return true;
}

static void astarTest(const char *name, int x1, int y1, int x2, int y2)
{
	int		asret, i;
	MOVE_CONTROL	route;
	int		x = world_coord(x1);
	int		y = world_coord(y1);
	int		endx = world_coord(x2);
	int		endy = world_coord(y2);
	clock_t		stop;
	clock_t		start = clock();
	bool		retval;

	retval = levLoadData(name, NULL, 0);
	ASSERT(retval, "Could not load %s", name);
	route.asPath = NULL;
	for (i = 0; i < 100; i++)
	{
		PATHJOB job;

		route.numPoints = 0;
		astarResetCounters();
		job.origX = x;
		job.origY = y;
		job.destX = endx;
		job.destY = endy;
		job.propulsion = PROPULSION_TYPE_WHEELED;
		job.droidID = 1;
		job.owner = 0;
		asret = fpathAStarRoute(&route, &job);
		free(route.asPath);
		route.asPath = NULL;
	}
	stop = clock();
	fprintf(stdout, "\t\tA* timing %s: %.02f (%d nodes)\n", name,
	        (double)(stop - start) / (double)CLOCKS_PER_SEC, route.numPoints);
	start = clock();
	fpathTest(x, y, endx, endy);
	stop = clock();
	fprintf(stdout, "\t\tfPath timing %s: %.02f (%d nodes)\n", name,
	        (double)(stop - start) / (double)CLOCKS_PER_SEC, route.numPoints);
	retval = levReleaseAll();
	assert(retval);
}

void mapTest()
{
	fprintf(stdout, "\tMap self-test...\n");

	astarTest("BeggarsKanyon-T1", 16, 5, 119, 182);
	astarTest("MizaMaze-T3", 5, 5, 108, 112);

	fprintf(stdout, "\tMap self-test: PASSED\n");
}

// Convert a direction into an offset.
// dir 0 => x = 0, y = -1
#define NUM_DIR		8
static const Vector2i aDirOffset[NUM_DIR] =
{
        { 0, 1},
        {-1, 1},
        {-1, 0},
        {-1,-1},
        { 0,-1},
        { 1,-1},
        { 1, 0},
        { 1, 1},
};

// Flood fill a "continent". Note that we reuse x, y inside this function.
static void mapFloodFill(int x, int y, int continent, PROPULSION_TYPE propulsion)
{
	struct ffnode *open = NULL;
	int i;

	do
	{
		MAPTILE *currTile = mapTile(x, y);

		// Add accessible neighbouring tiles to the open list
		for (i = 0; i < NUM_DIR; i++)
		{
			// rely on the fact that all border tiles are inaccessible to avoid checking explicitly
			Vector2i npos = { x + aDirOffset[i].x, y + aDirOffset[i].y };
			MAPTILE *psTile;
			bool limitedTile = (propulsion == PROPULSION_TYPE_PROPELLOR || propulsion == PROPULSION_TYPE_WHEELED);

			if (!tileOnMap(npos.x, npos.y) || (npos.x == x && npos.y == y))
			{
				continue;
			}
			psTile = mapTile(npos.x, npos.y);

			if (!fpathBlockingTile(npos.x, npos.y, propulsion) && ((limitedTile && psTile->limitedContinent == 0) || (!limitedTile && psTile->hoverContinent == 0)))
			{
				struct ffnode *node = malloc(sizeof(*node));

				node->next = open;	// add to beginning of open list
				node->x = npos.x;
				node->y = npos.y;
				open = node;
			}
		}

		// Set continent value
		if (propulsion == PROPULSION_TYPE_PROPELLOR || propulsion == PROPULSION_TYPE_WHEELED)
		{
			currTile->limitedContinent = continent;
		}
		else
		{
			currTile->hoverContinent = continent;
		}

		// Pop the first open node off the list for the next iteration
		if (open)
		{
			struct ffnode *tmp = open;

			x = open->x;
			y = open->y;
			open = open->next;
			free(tmp);
		}
	} while (open);
}

void mapFloodFillContinents()
{
	int x, y, limitedContinents = 0, hoverContinents = 0;

	/* Clear continents */
	for (x = 0; x < mapWidth; x++)
	{
		for (y = 0; y < mapHeight; y++)
		{
			MAPTILE *psTile = mapTile(x, y);

			psTile->limitedContinent = 0;
			psTile->hoverContinent = 0;
		}
	}

	/* Iterate over the whole map, looking for unset continents */
	for (x = 0; x < mapWidth; x++)
	{
		for (y = 0; y < mapHeight; y++)
		{
			MAPTILE *psTile = mapTile(x, y);

			if (psTile->limitedContinent == 0 && !fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
			{
				mapFloodFill(x, y, 1 + limitedContinents++, PROPULSION_TYPE_WHEELED);
			}
			else if (psTile->limitedContinent == 0 && !fpathBlockingTile(x, y, PROPULSION_TYPE_PROPELLOR))
			{
				mapFloodFill(x, y, 1 + limitedContinents++, PROPULSION_TYPE_PROPELLOR);
			}
		}
	}
	debug(LOG_MAP, "Found %d limited continents", (int)limitedContinents);
	for (x = 0; x < mapWidth; x++)
	{
		for (y = 0; y < mapHeight; y++)
		{
			MAPTILE *psTile = mapTile(x, y);

			if (psTile->hoverContinent == 0 && !fpathBlockingTile(x, y, PROPULSION_TYPE_HOVER))
			{
				mapFloodFill(x, y, 1 + hoverContinents++, PROPULSION_TYPE_HOVER);
			}
		}
	}
	debug(LOG_MAP, "Found %d hover continents", (int)hoverContinents);
}

/** Check if tile contained within the given world coordinates is burning. */
bool fireOnLocation(unsigned int x, unsigned int y)
{
	const int posX = map_coord(x);
	const int posY = map_coord(y);
	const MAPTILE *psTile = mapTile(posX, posY);

	ASSERT(psTile, "Checking fire on tile outside the map (%d, %d)", posX, posY);
	return psTile != NULL && TileIsBurning(psTile);
}
