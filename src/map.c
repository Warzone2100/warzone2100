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
#include "lib/netplay/netplay.h"  // For syncDebug

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
#include "gateway.h"
#include "wrappers.h"
#include "mapgrid.h"
#include "astar.h"
#include "fpath.h"
#include "levels.h"
#include "scriptfuncs.h"

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
SDWORD	mapWidth = 0, mapHeight = 0;
MAPTILE	*psMapTiles = NULL;

#define WATER_DEPTH	180

/// The different ground types
GROUND_TYPE *psGroundTypes;
int numGroundTypes;
int waterGroundType;
int cliffGroundType;
char *tileset = NULL;

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
		debug(LOG_ERROR, "map too large : %u %u", width, height);

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
		psTile->tileExploredBits = 0;
		psTile->sensorBits = 0;
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

// arizona
enum {
	a_red = 0,
	a_cliff,
	a_yellow,
	a_concrete,
	a_mud,
	a_green,
	a_water,
};

// urban
enum
{
	u_blue,
	u_gray,
	u_stone,
	u_dark,
	u_water,
	u_brown,
	u_cliff,
	u_green,
};

enum
{
	r_grass,
	r_rock,
	r_cliff,
	r_snowrock,
	r_brown,
	r_water,
	r_snowgrass,
	r_tiles,
	r_snow,
};

static BOOL mapLoadGroundTypes(void)
{
	debug(LOG_TERRAIN, "tileset: %s", tileset);
	// FIXME: Read these from a config file
	if (strcmp(tileset, "texpages/tertilesc1hw") == 0)
	{
		numGroundTypes = 7;
		psGroundTypes = malloc(sizeof(GROUND_TYPE)*numGroundTypes);
		
		psGroundTypes[a_yellow].textureName = "page-45";
		psGroundTypes[a_yellow].textureSize = 4.6;
		psGroundTypes[a_red].textureName = "page-44";
		psGroundTypes[a_red].textureSize = 6.2;
		psGroundTypes[a_concrete].textureName = "page-47";
		psGroundTypes[a_concrete].textureSize = 3.2;
		psGroundTypes[a_cliff].textureName = "page-46";
		psGroundTypes[a_cliff].textureSize = 5.8;
		psGroundTypes[a_water].textureName = "page-42";
		psGroundTypes[a_water].textureSize = 7.3;
		
		psGroundTypes[a_mud] = psGroundTypes[a_red];
		psGroundTypes[a_green] = psGroundTypes[a_red];
		
		waterGroundType = a_water;
		cliffGroundType = a_cliff;
	}
	else if (strcmp(tileset, "texpages/tertilesc2hw") == 0)
	{
		numGroundTypes = 8;
		psGroundTypes = malloc(sizeof(GROUND_TYPE)*numGroundTypes);

		psGroundTypes[u_gray].textureName = "page-51"; // rubble
		psGroundTypes[u_gray].textureSize = 3.9;
		psGroundTypes[u_green].textureName = "page-52"; // close plants
		psGroundTypes[u_green].textureSize = 2.6;
		psGroundTypes[u_stone].textureName = "page-47"; // pavement
		psGroundTypes[u_stone].textureSize = 3.2;
		psGroundTypes[u_cliff].textureName = "page-49"; // crater walls
		psGroundTypes[u_cliff].textureSize = 4.7;
		psGroundTypes[u_brown].textureName = "page-49"; // rocks
		psGroundTypes[u_brown].textureSize = 4.7;

		psGroundTypes[u_blue] = psGroundTypes[u_brown]; // burned ground
		psGroundTypes[u_dark] = psGroundTypes[u_stone];
		psGroundTypes[u_water] = psGroundTypes[u_cliff];
		
		waterGroundType = u_water;
		cliffGroundType = u_cliff;
	}
	else if (strcmp(tileset, "texpages/tertilesc3hw") == 0)
	{
		numGroundTypes = 9;
		psGroundTypes = malloc(sizeof(GROUND_TYPE)*numGroundTypes);

		psGroundTypes[r_grass].textureName = "page-55";
		psGroundTypes[r_grass].textureSize = 4.6;
		psGroundTypes[r_rock].textureName = "page-56";
		psGroundTypes[r_rock].textureSize = 3.9;
		psGroundTypes[r_cliff].textureName = "page-46";
		psGroundTypes[r_cliff].textureSize = 5.8;
		psGroundTypes[r_water].textureName = "page-49";
		psGroundTypes[r_water].textureSize = 5.8;
		psGroundTypes[r_tiles].textureName = "page-47";
		psGroundTypes[r_tiles].textureSize = 3.2;
		psGroundTypes[r_snowgrass].textureName = "page-54";
		psGroundTypes[r_snowgrass].textureSize = 2.1;
		psGroundTypes[r_snow].textureName = "page-57";
		psGroundTypes[r_snow].textureSize = 10.2;
		psGroundTypes[r_snowrock].textureName = "page-58";
		psGroundTypes[r_snowrock].textureSize = psGroundTypes[r_rock].textureSize;
		
		psGroundTypes[r_brown] = psGroundTypes[r_water];

		waterGroundType = r_water;
		cliffGroundType = r_cliff;
	}
	else
	{
		debug(LOG_ERROR, "unsupported tileset: %s", tileset);
		debug(LOG_POPUP, "This is a UNSUPPORTED map with a custom tileset.\nDefaulting to tertilesc1hw -- map may look strange!");
		// HACK: / FIXME: For now, we just pretend this is a tertilesc1hw map.
		free(tileset);
		tileset = strdup("texpages/tertilesc1hw");
		numGroundTypes = 7;
		psGroundTypes = malloc(sizeof(GROUND_TYPE)*numGroundTypes);
		
		psGroundTypes[a_yellow].textureName = "page-45";
		psGroundTypes[a_yellow].textureSize = 4.6;
		psGroundTypes[a_red].textureName = "page-44";
		psGroundTypes[a_red].textureSize = 6.2;
		psGroundTypes[a_concrete].textureName = "page-47";
		psGroundTypes[a_concrete].textureSize = 3.2;
		psGroundTypes[a_cliff].textureName = "page-46";
		psGroundTypes[a_cliff].textureSize = 5.8;
		psGroundTypes[a_water].textureName = "page-42";
		psGroundTypes[a_water].textureSize = 7.3;
		
		psGroundTypes[a_mud] = psGroundTypes[a_red];
		psGroundTypes[a_green] = psGroundTypes[a_red];
		
		waterGroundType = a_water;
		cliffGroundType = a_cliff;

	}
	return true;
}

static int groundFromArizonaTile(int tile, int i, int j)
{
	const int map[][2][2] = {
		{{a_yellow,a_yellow}, {a_yellow,a_yellow}},
		{{a_yellow,a_yellow}, {a_yellow,a_yellow}},
		{{a_yellow,a_mud}, {a_yellow,a_mud}},
		{{a_mud,a_mud}, {a_mud,a_yellow}},
		{{a_mud,a_yellow}, {a_yellow,a_yellow}},
		{{a_mud,a_mud}, {a_mud,a_mud}},
		{{a_mud,a_mud}, {a_mud,a_mud}},
		{{a_mud,a_mud}, {a_mud,a_mud}},
		{{a_mud,a_mud}, {a_mud,a_mud}},
		{{a_yellow,a_yellow}, {a_yellow,a_yellow}},
		{{a_yellow,a_yellow}, {a_yellow,a_yellow}},
		{{a_yellow,a_yellow}, {a_yellow,a_yellow}},
		{{a_yellow,a_yellow}, {a_yellow,a_yellow}},
		{{a_water,a_yellow}, {a_yellow,a_yellow}},
		{{a_water,a_water}, {a_yellow,a_yellow}},
		{{a_yellow,a_water}, {a_yellow,a_yellow}},
		{{a_water,a_water}, {a_yellow,a_water}},
		{{a_water,a_water}, {a_water,a_water}},
		{{a_cliff,a_cliff}, {a_cliff,a_cliff}},
		{{a_concrete,a_concrete}, {a_concrete,a_red}},
		{{a_concrete,a_red}, {a_red,a_red}},
		{{a_concrete,a_red}, {a_concrete,a_red}},
		{{a_concrete,a_concrete}, {a_concrete,a_concrete}},
		{{a_green,a_green}, {a_green,a_green}},
		{{a_green,a_mud}, {a_green,a_mud}},
		{{a_green,a_green}, {a_green,a_mud}},
		{{a_green,a_mud}, {a_mud,a_mud}},
		{{a_yellow,a_red}, {a_yellow,a_red}},
		{{a_yellow,a_yellow}, {a_yellow,a_red}},
		{{a_yellow,a_red}, {a_red,a_red}},
		{{a_water,a_water}, {a_red,a_green}},
		{{a_green,a_green}, {a_water,a_water}},
		{{a_green,a_green}, {a_green,a_water}},
		{{a_green,a_water}, {a_water,a_water}},
		{{a_red,a_mud}, {a_red,a_mud}},
		{{a_red,a_red}, {a_red,a_mud}},
		{{a_red,a_mud}, {a_mud,a_mud}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_yellow,a_mud}, {a_yellow,a_mud}},
		{{a_mud,a_mud}, {a_mud,a_yellow}},
		{{a_mud,a_yellow}, {a_yellow,a_yellow}},
		{{a_red,a_red}, {a_red,a_yellow}},
		{{a_red,a_yellow}, {a_yellow,a_yellow}},
		{{a_red,a_yellow}, {a_red,a_yellow}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_cliff,a_cliff}, {a_red,a_cliff}},
		{{a_cliff,a_cliff}, {a_cliff,a_cliff}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_yellow,a_yellow}, {a_yellow,a_yellow}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_mud,a_mud}, {a_mud,a_mud}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_green,a_green}, {a_green,a_green}},
		{{a_yellow,a_yellow}, {a_yellow,a_yellow}},
		{{a_yellow,a_yellow}, {a_yellow,a_yellow}},
		{{a_yellow,a_yellow}, {a_yellow,a_yellow}},
		{{a_yellow,a_yellow}, {a_yellow,a_yellow}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_cliff,a_cliff}, {a_cliff,a_cliff}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_cliff,a_cliff}, {a_cliff,a_red}},
		{{a_red,a_red}, {a_red,a_red}},
		{{a_concrete,a_concrete}, {a_concrete,a_concrete}},
		};
		
	// look up
	return map[TileNumber_tile(tile)][i][j];
}


static int groundFromUrbanTile(int tile, int i, int j)
{
	const int map[][2][2] = {
		{{u_blue, u_blue}, {u_blue, u_blue}},
		{{u_blue, u_blue}, {u_blue, u_blue}},
		{{u_blue, u_blue}, {u_blue, u_blue}},
		{{u_gray, u_blue}, {u_blue, u_blue}},
		{{u_gray, u_gray}, {u_gray, u_blue}},
		{{u_gray, u_gray}, {u_gray, u_gray}},
		{{u_blue, u_gray}, {u_blue, u_gray}},
		{{u_gray, u_gray}, {u_gray, u_gray}},
		{{u_gray, u_gray}, {u_gray, u_gray}},
		{{u_gray, u_stone}, {u_gray, u_stone}},
		{{u_gray, u_stone}, {u_gray, u_gray}},
		{{u_stone, u_stone}, {u_dark, u_stone}},
		{{u_dark, u_stone}, {u_dark, u_stone}},
		{{u_stone, u_water}, {u_stone, u_water}},
		{{u_water, u_water}, {u_stone, u_stone}},
		{{u_stone, u_water}, {u_stone, u_stone}},
		{{u_water, u_water}, {u_stone, u_water}},
		{{u_water, u_water}, {u_water, u_water}},
		{{u_dark, u_dark}, {u_dark, u_dark}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_brown, u_brown}, {u_brown, u_brown}},
		{{u_gray, u_water}, {u_gray, u_water}},
		{{u_gray, u_gray}, {u_water, u_water}},
		{{u_gray, u_water}, {u_water, u_water}},
		{{u_gray, u_gray}, {u_gray, u_water}},
		{{u_stone, u_gray}, {u_stone, u_gray}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_gray, u_stone}, {u_gray, u_gray}},
		{{u_stone, u_stone}, {u_gray, u_stone}},
		{{u_brown, u_brown}, {u_brown, u_brown}},
		{{u_blue, u_blue}, {u_blue, u_blue}},
		{{u_brown, u_blue}, {u_brown, u_blue}},
		{{u_brown, u_brown}, {u_brown, u_blue}},
		{{u_brown, u_blue}, {u_blue, u_blue}},
		{{u_brown, u_brown}, {u_brown, u_brown}},
		{{u_green, u_green}, {u_green, u_brown}},
		{{u_green, u_brown}, {u_brown, u_brown}},
		{{u_brown, u_green}, {u_brown, u_green}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_blue, u_blue}, {u_blue, u_blue}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_gray, u_gray}, {u_gray, u_gray}},
		{{u_gray, u_gray}, {u_gray, u_gray}},
		{{u_green, u_green}, {u_green, u_green}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_gray, u_gray}, {u_gray, u_gray}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_green, u_green}, {u_green, u_green}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_gray, u_gray}, {u_gray, u_gray}},
		{{u_gray, u_gray}, {u_gray, u_gray}},
		{{u_gray, u_gray}, {u_gray, u_gray}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_blue, u_blue}, {u_blue, u_blue}},
		{{u_gray, u_gray}, {u_gray, u_gray}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_brown, u_gray}, {u_brown, u_gray}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_gray, u_gray}, {u_gray, u_gray}},
		{{u_blue, u_blue}, {u_blue, u_blue}},
		{{u_blue, u_blue}, {u_blue, u_blue}},
		{{u_blue, u_blue}, {u_blue, u_blue}},
		{{u_blue, u_blue}, {u_blue, u_blue}},
		{{u_gray, u_gray}, {u_gray, u_gray}},
		{{u_cliff, u_cliff}, {u_gray, u_cliff}},
		{{u_cliff, u_cliff}, {u_cliff, u_cliff}},
		{{u_cliff, u_cliff}, {u_cliff, u_cliff}},
		{{u_brown, u_stone}, {u_brown, u_stone}},
		{{u_gray, u_gray}, {u_gray, u_brown}},
		{{u_gray, u_brown}, {u_brown, u_brown}},
		{{u_stone, u_stone}, {u_stone, u_stone}},
		{{u_brown, u_stone}, {u_stone, u_stone}},
		{{u_brown, u_brown}, {u_brown, u_stone}},
		{{u_gray, u_green}, {u_gray, u_green}},
		{{u_gray, u_gray}, {u_gray, u_gray}},
		{{u_green, u_gray}, {u_green, u_green}},
		{{u_gray, u_gray}, {u_gray, u_gray}},
		};
		
	// look up
	return map[TileNumber_tile(tile)][i][j];
}

static int groundFromRockiesTile(int tile, int i, int j)
{
	const int map[][2][2] = {
		{{r_grass, r_grass}, {r_grass, r_grass}},
		{{r_grass, r_grass}, {r_grass, r_grass}},
		{{r_rock, r_grass}, {r_rock, r_grass}},
		{{r_rock, r_grass}, {r_grass, r_grass}},
		{{r_rock, r_rock}, {r_rock, r_grass}},
		{{r_rock, r_rock}, {r_rock, r_rock}},
		{{r_rock, r_rock}, {r_rock, r_rock}},
		{{r_rock, r_rock}, {r_rock, r_rock}},
		{{r_rock, r_rock}, {r_rock, r_rock}},
		{{r_cliff, r_cliff}, {r_cliff, r_snowrock}},
		{{r_snowrock, r_snowrock}, {r_snowrock, r_snowrock}},
		{{r_snowrock, r_rock}, {r_rock, r_rock}},
		{{r_snowrock, r_snowrock}, {r_rock, r_rock}},
		{{r_brown, r_brown}, {r_brown, r_brown}},
		{{r_water, r_water}, {r_grass, r_grass}},
		{{r_grass, r_water}, {r_grass, r_grass}},
		{{r_water, r_water}, {r_grass, r_water}},
		{{r_water, r_water}, {r_water, r_water}},
		{{r_cliff, r_cliff}, {r_cliff, r_rock}},
		{{r_tiles, r_brown}, {r_brown, r_brown}},
		{{r_tiles, r_tiles}, {r_tiles, r_brown}},
		{{r_tiles, r_brown}, {r_tiles, r_brown}},
		{{r_tiles, r_tiles}, {r_tiles, r_tiles}},
		{{r_snowgrass, r_snowgrass}, {r_snowgrass, r_snowgrass}},
		{{r_snowgrass, r_snowgrass}, {r_snowgrass, r_grass}},
		{{r_snowgrass, r_grass}, {r_grass, r_grass}},
		{{r_snowgrass, r_snowgrass}, {r_grass, r_grass}},
		{{r_brown, r_brown}, {r_brown, r_brown}},
		{{r_brown, r_brown}, {r_brown, r_brown}},
		{{r_cliff, r_cliff}, {r_cliff, r_cliff}},
		{{r_cliff, r_cliff}, {r_cliff, r_cliff}},
		{{r_rock, r_rock}, {r_water, r_water}},
		{{r_rock, r_water}, {r_water, r_water}},
		{{r_rock, r_rock}, {r_rock, r_water}},
		{{r_grass, r_brown}, {r_grass, r_brown}},
		{{r_grass, r_grass}, {r_grass, r_brown}},
		{{r_grass, r_brown}, {r_brown, r_brown}},
		{{r_brown, r_brown}, {r_brown, r_brown}},
		{{r_brown, r_rock}, {r_brown, r_rock}},
		{{r_brown, r_brown}, {r_brown, r_rock}},
		{{r_brown, r_rock}, {r_rock, r_rock}},
		{{r_snowrock, r_snowrock}, {r_snowrock, r_snowrock}},
		{{r_cliff, r_cliff}, {r_rock, r_cliff}},
		{{r_cliff, r_rock}, {r_rock, r_cliff}},
		{{r_cliff, r_cliff}, {r_rock, r_cliff}},
		{{r_cliff, r_cliff}, {r_rock, r_cliff}},
		{{r_cliff, r_cliff}, {r_cliff, r_cliff}},
		{{r_grass, r_grass}, {r_grass, r_grass}},
		{{r_snowrock, r_snowrock}, {r_rock, r_rock}},
		{{r_brown, r_brown}, {r_brown, r_brown}},
		{{r_brown, r_brown}, {r_brown, r_brown}},
		{{r_brown, r_brown}, {r_brown, r_brown}},
		{{r_brown, r_brown}, {r_brown, r_brown}},
		{{r_brown, r_brown}, {r_brown, r_brown}},
		{{r_snow, r_snow}, {r_snow, r_rock}},
		{{r_rock, r_rock}, {r_rock, r_rock}},
		{{r_brown, r_brown}, {r_brown, r_brown}},
		{{r_rock, r_rock}, {r_rock, r_rock}},
		{{r_grass, r_grass}, {r_grass, r_grass}},
		{{r_brown, r_brown}, {r_brown, r_brown}},
		{{r_brown, r_brown}, {r_brown, r_brown}},
		{{r_cliff, r_cliff}, {r_cliff, r_cliff}},
		{{r_rock, r_rock}, {r_rock, r_rock}},
		{{r_cliff, r_cliff}, {r_cliff, r_snow}},
		{{r_snow, r_snow}, {r_snow, r_snow}},
		{{r_snow, r_snow}, {r_snow, r_snowrock}},
		{{r_snow, r_snowrock}, {r_snowrock, r_snowrock}},
		{{r_snow, r_snow}, {r_snowrock, r_snowrock}},
		{{r_cliff, r_cliff}, {r_cliff, r_snow}},
		{{r_cliff, r_cliff}, {r_cliff, r_snow}},
		{{r_snow, r_snow}, {r_snow, r_snow}},
		{{r_cliff, r_cliff}, {r_cliff, r_snow}},
		{{r_brown, r_brown}, {r_brown, r_brown}},
		{{r_snowrock, r_snowrock}, {r_snowrock, r_snowrock}},
		{{r_grass, r_grass}, {r_grass, r_grass}},
		{{r_snowrock, r_snowrock}, {r_snowrock, r_snowrock}},
		{{r_cliff, r_cliff}, {r_cliff, r_cliff}},
		{{r_cliff, r_cliff}, {r_cliff, r_cliff}},
		{{r_cliff, r_cliff}, {r_cliff, r_cliff}},
		{{r_tiles, r_tiles}, {r_tiles, r_tiles}},
		};
		
	// look up
	return map[TileNumber_tile(tile)][i][j];
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

	
	if (x < 0 || y < 0 || x >= mapWidth || y >= mapHeight)
	{
		return 0; // just return the first ground type
	}
	
	// check what tiles surround this grid point
	for(i=0;i<2;i++)
	{
		for(j=0;j<2;j++)
		{
			if (x+i-1 < 0 || y+j-1 < 0 || x+i-1 >= mapWidth || y+j-1 >= mapHeight)
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
			if (arizona)
			{
				ground[i][j] = groundFromArizonaTile(tile, a, b);
			}
			if (urban)
			{
				ground[i][j] = groundFromUrbanTile(tile, a, b);
			}
			if (rockies)
			{
				ground[i][j] = groundFromRockiesTile(tile, a, b);
			}
			
			votes[i][j] = 0;
			
			if ((ground[i][j] == u_cliff && urban) ||
			    (ground[i][j] == a_cliff && arizona) ||
			    (ground[i][j] == r_cliff && rockies))
			{
				// cliffs are so small they won't show up otherwise
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

static BOOL hasArizonaDecal(UDWORD i, UDWORD j)
{
	switch ( TileNumber_tile(mapTile(i, j)->texture))
	{
		case 37:
		case 47:
		case 49:
		case 50:
		case 51:
		case 52:
		case 55:
		case 56:
		case 57:
		case 58:
		case 59:
		case 60:
		case 61:
		case 62:
		case 63:
		case 64:
		case 65:
		case 66:
		case 67:
		case 68:
		case 69:
		case 70:
		case 72:
		case 73:
			return true;
		default:
			return false;
	}
}

static BOOL hasUrbanDecal(UDWORD i, UDWORD j)
{
	switch ( TileNumber_tile(mapTile(i, j)->texture))
	{
		case 11:
		case 12:
		case 28:
		case 32:
		case 40:
		case 41:
		case 42:
		case 43:
		case 44:
		case 45:
		case 46:
		case 47:
		case 48:
		case 49:
		case 52:
		case 53:
		case 54:
		case 55:
		case 56:
		case 57:
		case 59:
		case 61:
		case 62:
		case 63:
		case 64:
		case 65:
		case 66:
		case 67:
		case 80:
			return true;
		default:
			return false;
	}
}

static BOOL hasRockiesDecal(UDWORD i, UDWORD j)
{
	switch ( TileNumber_tile(mapTile(i, j)->texture))
	{
		case 13:
		case 37:
		case 49:
		case 50:
		case 51:
		case 52:
		case 56:
		case 58:
		case 59:
		case 60:
		case 62:
		case 72:
		case 79:
			return true;
		default:
			return false;
	}
}

static BOOL mapSetGroundTypes(void)
{
	int i,j;

	for (i=0;i<mapWidth;i++)
	{
		for (j=0;j<mapHeight;j++)
		{
			MAPTILE *psTile = mapTile(i, j);

			psTile->ground = determineGroundType(i,j,tileset);

			if ((strcmp(tileset, "texpages/tertilesc1hw") == 0 && hasArizonaDecal(i, j))
			    || (strcmp(tileset, "texpages/tertilesc2hw") == 0 && hasUrbanDecal(i, j))
			    || (strcmp(tileset, "texpages/tertilesc3hw") == 0 && hasRockiesDecal(i, j)))
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
			goto failure;
		}

		psMapTiles[i].texture = texture;
		psMapTiles[i].height = height;

		// Visibility stuff
		memset(psMapTiles[i].watchers, 0, sizeof(psMapTiles[i].watchers));
		psMapTiles[i].sensorBits = 0;
		psMapTiles[i].tileExploredBits = 0;
	}

	if (!PHYSFS_readULE32(fp, &version) || !PHYSFS_readULE32(fp, &numGw) || version != 1)
	{
		debug(LOG_ERROR, "Bad gateway in %s", filename);
		goto failure;
	}

	for (i = 0; i < numGw; i++)
	{
		UBYTE	x0, y0, x1, y1;

		if (!PHYSFS_readULE8(fp, &x0) || !PHYSFS_readULE8(fp, &y0) || !PHYSFS_readULE8(fp, &x1) || !PHYSFS_readULE8(fp, &y1))
		{
			debug(LOG_ERROR, "%s: Failed to read gateway info", filename);
			goto failure;
		}
		if (!gwNewGateway(x0, y0, x1, y1))
		{
			debug(LOG_ERROR, "%s: Unable to add gateway", filename);
			goto failure;
		}
	}
	
	if (!mapSetGroundTypes())
	{
		goto failure;
	}

	// reset the random water bottom heights
	environReset();

	// set the river bed
	for (i = 0; i < mapWidth; i++)
	{
		for (j = 0; j < mapHeight; j++)
		{
			// FIXME: magic number
			mapTile(i, j)->waterLevel = mapTile(i, j)->height - world_coord(1) / 3.0f / (float)ELEVATION_SCALE;
			// lower riverbed
			if (mapTile(i, j)->ground == waterGroundType)
			{
				mapTile(i, j)->height -= (WATER_DEPTH - 2.0f * environGetData(i, j)) / (float)ELEVATION_SCALE;
			}
		}
	}

	/* set up the scroll mins and maxs - set values to valid ones for any new map */
	scrollMinX = scrollMinY = 0;
	scrollMaxX = mapWidth;
	scrollMaxY = mapHeight;

	/* Set continents. This should ideally be done in advance by the map editor. */
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
	tagWriteEnter(0x01, 1);
	tagWrite(0x01, psObj->type);
	tagWrite(0x02, psObj->id);
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
	tagWriteEnter(0x04, num);
	tagWriteLeave(0x04);
}

static void droidSaveTagged(DROID *psDroid)
{
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
	tagWrite(0x04, psStruct->bTargetted);
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
			tagWrite(0x02, psFactory->productionLoops);
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
		case REF_GATE:
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
	tagWrite(0x01, psFeat->born);
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
		numGateways += 1;
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
		uint16_t p[4];

		p[0] = psCurrGate->x1;
		p[1] = psCurrGate->y1;
		p[2] = psCurrGate->x2;
		p[3] = psCurrGate->y2;
		tagWrite16v(0x01, 4, p);
		tagWriteNext();
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
		numGateways += 1;
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
		if (psTile->ground == waterGroundType)
		{
			psTileData->height = MIN(255.0f, psTile->height + (WATER_DEPTH - 2.0f * environGetData(i % mapWidth, i / mapWidth)) / (float)ELEVATION_SCALE);
		}
		else
		{
			psTileData->height = psTile->height;
		}

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
		psGate->x0 = psCurrGate->x1;
		psGate->y0 = psCurrGate->y1;
		psGate->x1 = psCurrGate->x2;
		psGate->y1 = psCurrGate->y2;
		psGate++;
		i++;
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
	if(psMapTiles)
	{
		free(psMapTiles);
	}
	psMapTiles = NULL;
	mapWidth = mapHeight = 0;

	return true;
}

/// The max height of the terrain and water at the specified world coordinates
extern int32_t map_Height(int x, int y)
{
	int tileX, tileY;
	int i, j;
	float height[2][2], center;
	float onTileX, onTileY;
	float left, right, middle;
	float onBottom, result;
	float towardsCenter, towardsRight;

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

	// on which tile are these coords?
	tileX = map_coord(x);
	tileY = map_coord(y);

	// where on the tile? (scale to (0,1))
	onTileX = (x - world_coord(tileX))/(float)world_coord(1);
	onTileY = (y - world_coord(tileY))/(float)world_coord(1);

	// get the height for the corners and center
	center = 0;
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			height[i][j] = map_TileHeightSurface(tileX+i, tileY+j);
			center += height[i][j];
		}
	}
	center /= 4;

	// we have:
	//   y ->
	// x 0,0  A  0,1
	// |
	// V D  center B
	//
	//   1,0  C  1,1

	// get heights for left and right corners and the distances
	if (onTileY > onTileX)
	{
		if (onTileY < 1 - onTileX)
		{
			// A
			right = height[0][0];
			left  = height[0][1];
			towardsCenter = onTileX;
			towardsRight  = 1 - onTileY;
		}
		else
		{
			// B
			right = height[0][1];
			left  = height[1][1];
			towardsCenter = 1 - onTileY;
			towardsRight  = 1 - onTileX;
		}
	}
	else
	{
		if (onTileX > 1 - onTileY)
		{
			// C
			right = height[1][1];
			left  = height[1][0];
			towardsCenter = 1 - onTileX;
			towardsRight  = onTileY;
		}
		else
		{
			// D
			right = height[1][0];
			left  = height[0][0];
			towardsCenter = onTileY;
			towardsRight  = onTileX;
		}
	}
	ASSERT(towardsCenter <= 0.5, "towardsCenter is too high");

	// now we have:
	//         center
	//    left   m    right

	middle = (left + right)/2;
	onBottom = left * (1 - towardsRight) + right * towardsRight;
	result = onBottom + (center - middle) * towardsCenter * 2;

	return (SDWORD)(result+0.5f);
}

/* returns true if object is above ground */
extern BOOL mapObjIsAboveGround( BASE_OBJECT *psObj )
{
	// min is used to make sure we don't go over array bounds!
	// TODO Using the corner of the map instead doesn't make sense. Fix this...
	SDWORD	iZ,
			tileX = map_coord(psObj->pos.x),
			tileY = map_coord(psObj->pos.y),
			tileYOffset1 = (tileY * mapWidth),
			tileYOffset2 = ((tileY+1) * mapWidth),
			h1 = psMapTiles[MIN(mapWidth * mapHeight - 1, tileYOffset1 + tileX)    ].height,
			h2 = psMapTiles[MIN(mapWidth * mapHeight - 1, tileYOffset1 + tileX + 1)].height,
			h3 = psMapTiles[MIN(mapWidth * mapHeight - 1, tileYOffset2 + tileX)    ].height,
			h4 = psMapTiles[MIN(mapWidth * mapHeight - 1, tileYOffset2 + tileX + 1)].height;

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
		if (!PHYSFS_writeUBE8(fileHandle, psMapTiles[i].tileExploredBits))
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
		if (!PHYSFS_readUBE8(fileHandle, &psMapTiles[i].tileExploredBits))
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

	scriptInit();
	retval = levLoadData(name, NULL, 0);
	ASSERT(retval, "Could not load %s", name);
	route.asPath = NULL;
	for (i = 0; i < 100; i++)
	{
		PATHJOB job;

		route.numPoints = 0;
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

	astarTest("Sk-BeggarsKanyon-T1", 16, 5, 119, 182);
	astarTest("Sk-MizaMaze-T3", 5, 5, 108, 112);

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

void tileSetFire(int32_t x, int32_t y, uint32_t duration)
{
	const int posX = map_coord(x);
	const int posY = map_coord(y);
	MAPTILE *const tile = mapTile(posX, posY);

	uint16_t currentTime =  gameTime             / GAME_TICKS_PER_UPDATE;
	uint16_t fireEndTime = (gameTime + duration) / GAME_TICKS_PER_UPDATE;
	if (currentTime == fireEndTime)
	{
		return;  // Fire already ended.
	}
	if ((tile->tileInfoBits & BITS_ON_FIRE) != 0 && (uint16_t)(fireEndTime - currentTime) < (uint16_t)(tile->fireEndTime - currentTime))
	{
		return;  // Tile already on fire, and that fire lasts longer.
	}

	// Burn, tile, burn!
	tile->tileInfoBits |= BITS_ON_FIRE;
	tile->fireEndTime = fireEndTime;

	syncDebug("Fire tile{%d, %d} dur%u end%d", posX, posY, duration, fireEndTime);
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

void mapUpdate()
{
	uint16_t currentTime = gameTime / GAME_TICKS_PER_UPDATE;

	int posX, posY;
	for (posY = 0; posY < mapHeight; ++posY)
		for (posX = 0; posX < mapWidth; ++posX)
	{
		MAPTILE *const tile = mapTile(posX, posY);

		if ((tile->tileInfoBits & BITS_ON_FIRE) != 0 && tile->fireEndTime == currentTime)
		{
			// Extinguish, tile, extinguish!
			tile->tileInfoBits &= ~BITS_ON_FIRE;

			syncDebug("Extinguished tile{%d, %d}", posX, posY);
		}
	}

	// TODO Make waves in the water?
}
