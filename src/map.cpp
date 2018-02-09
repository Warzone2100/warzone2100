/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "lib/ivis_opengl/tex.h"
#include "lib/netplay/netplay.h"  // For syncDebug

#include "map.h"
#include "hci.h"
#include "projectile.h"
#include "display3d.h"
#include "game.h"
#include "texture.h"
#include "advvis.h"
#include "random.h"
#include "research.h"
#include "mission.h"
#include "gateway.h"
#include "wrappers.h"
#include "mapgrid.h"
#include "astar.h"
#include "fpath.h"
#include "levels.h"
#include "scriptfuncs.h"
#include "lib/framework/wzapp.h"

#define GAME_TICKS_FOR_DANGER (GAME_TICKS_PER_SEC * 2)

static WZ_THREAD *dangerThread = nullptr;
static WZ_SEMAPHORE *dangerSemaphore = nullptr;
static WZ_SEMAPHORE *dangerDoneSemaphore = nullptr;
struct floodtile
{
	uint8_t x;
	uint8_t y;
};
static struct floodtile *floodbucket = nullptr;
static int bucketcounter;
static UDWORD lastDangerUpdate = 0;
static int lastDangerPlayer = -1;

//scroll min and max values
SDWORD		scrollMinX, scrollMaxX, scrollMinY, scrollMaxY;

/* Structure definitions for loading and saving map data */
struct MAP_SAVEHEADER  // : public GAME_SAVEHEADER
{
	char		aFileType[4];
	UDWORD		version;
	UDWORD		width;
	UDWORD		height;
};

struct MAP_SAVETILE
{
	UWORD		texture;
	UBYTE		height;
};

struct GATEWAY_SAVEHEADER
{
	UDWORD		version;
	UDWORD		numGateways;
};

struct GATEWAY_SAVE
{
	UBYTE	x0, y0, x1, y1;
};

/* Sanity check definitions for the save struct file sizes */
#define SAVE_HEADER_SIZE	16
#define SAVE_TILE_SIZE		3

// Maximum expected return value from get height
#define	MAX_HEIGHT			(256 * ELEVATION_SCALE)

/* The size and contents of the map */
SDWORD	mapWidth = 0, mapHeight = 0;
MAPTILE	*psMapTiles = nullptr;
uint8_t *psBlockMap[AUX_MAX];
uint8_t *psAuxMap[MAX_PLAYERS + AUX_MAX];        // yes, we waste one element... eyes wide open... makes API nicer

#define WATER_MIN_DEPTH 500
#define WATER_MAX_DEPTH (WATER_MIN_DEPTH + 400)

static void SetGroundForTile(const char *filename, const char *nametype);
static int getTextureType(const char *textureType);
static bool hasDecals(int i, int j);
static void SetDecals(const char *filename, const char *decal_type);
static void init_tileNames(int type);

/// The different ground types
GROUND_TYPE *psGroundTypes;
int numGroundTypes;
char *tilesetDir = nullptr;
static int numTile_names;
static char *Tile_names = nullptr;
#define ARIZONA 1
#define URBAN 2
#define ROCKIE 3

static int *map;			// 3D array pointer that holds the texturetype
static bool *mapDecals;           // array that tells us what tile is a decal
#define MAX_TERRAIN_TILES 0x0200  // max that we support (for now), see TILE_NUMMASK

/* Look up table that returns the terrain type of a given tile texture */
UBYTE terrainTypes[MAX_TILE_TEXTURES];

static void init_tileNames(int type)
{
	char	*pFileData = nullptr;
	char	name[MAX_STR_LENGTH] = {'\0'};
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

			sscanf(pFileData, "%255[^,'\r\n],%d%n", name, &numlines, &cnt);
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

			sscanf(pFileData, "%255[^,'\r\n],%d%n", name, &numlines, &cnt);
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

			sscanf(pFileData, "%255[^,'\r\n],%d%n", name, &numlines, &cnt);
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
		debug(LOG_FATAL, "Rockie_enum parameter is out of range (%d). Aborting.", numlines);
		abort();
	}

	numTile_names = numlines;
	//increment the pointer to the start of the next record
	pFileData = strchr(pFileData, '\n') + 1;

	Tile_names = (char *)malloc(numlines * sizeof(char[MAX_STR_LENGTH]));
	memset(Tile_names, 0x0, (numlines * sizeof(char[MAX_STR_LENGTH])));

	for (i = 0; i < numlines; i++)
	{
		sscanf(pFileData, "%255[^,'\r\n]%n", &Tile_names[i * MAX_STR_LENGTH], &cnt);
		pFileData += cnt;
		//increment the pointer to the start of the next record
		pFileData = strchr(pFileData, '\n') + 1;
	}
}

// This is the main loading routine to get all the map's parameters set.
// Once it figures out what tileset we need, we then parse the files for that tileset.
// Currently, we only support 3 tilesets.  Arizona, Urban, and Rockie
static bool mapLoadGroundTypes()
{
	char	*pFileData = nullptr;
	char	tilename[MAX_STR_LENGTH] = {'\0'};
	char	textureName[MAX_STR_LENGTH] = {'\0'};
	char	textureType[MAX_STR_LENGTH] = {'\0'};
	double	textureSize = 0.f;
	int		numlines = 0;
	int		cnt = 0, i = 0;
	uint32_t	fileSize = 0;

	pFileData = fileLoadBuffer;

	debug(LOG_TERRAIN, "tileset: %s", tilesetDir);
	// For Arizona
	if (strcmp(tilesetDir, "texpages/tertilesc1hw") == 0)
	{
fallback:
		init_tileNames(ARIZONA);
		if (!loadFileToBuffer("tileset/tertilesc1hwGtype.txt", pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug(LOG_FATAL, "tileset/tertilesc1hwGtype.txt not found, aborting.");
			abort();
		}

		sscanf(pFileData, "%255[^,'\r\n],%d%n", tilename, &numlines, &cnt);
		pFileData += cnt;

		if (strcmp(tilename, "tertilesc1hw"))
		{
			debug(LOG_FATAL, "%s found, but was expecting tertilesc1hw!  Aborting.", tilename);
			abort();
		}

		debug(LOG_TERRAIN, "tilename: %s, with %d entries", tilename, numlines);
		//increment the pointer to the start of the next record
		pFileData = strchr(pFileData, '\n') + 1;
		numGroundTypes = numlines;
		psGroundTypes = (GROUND_TYPE *)malloc(sizeof(GROUND_TYPE) * numlines);

		for (i = 0; i < numlines; i++)
		{
			sscanf(pFileData, "%255[^,'\r\n],%255[^,'\r\n],%lf%n", textureType, textureName, &textureSize, &cnt);
			pFileData += cnt;
			//increment the pointer to the start of the next record
			pFileData = strchr(pFileData, '\n') + 1;

			psGroundTypes[getTextureType(textureType)].textureName = strdup(textureName);
			psGroundTypes[getTextureType(textureType)].textureSize = textureSize ;
		}

		SetGroundForTile("tileset/arizonaground.txt", "arizona_ground");
		SetDecals("tileset/arizonadecals.txt", "arizona_decals");
	}
	// for Urban
	else if (strcmp(tilesetDir, "texpages/tertilesc2hw") == 0)
	{
		init_tileNames(URBAN);
		if (!loadFileToBuffer("tileset/tertilesc2hwGtype.txt", pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug(LOG_POPUP, "tileset/tertilesc2hwGtype.txt not found, using default terrain ground types.");
			goto fallback;
		}

		sscanf(pFileData, "%255[^,'\r\n],%d%n", tilename, &numlines, &cnt);
		pFileData += cnt;

		if (strcmp(tilename, "tertilesc2hw"))
		{
			debug(LOG_POPUP, "%s found, but was expecting tertilesc2hw!", tilename);
			goto fallback;
		}

		debug(LOG_TERRAIN, "tilename: %s, with %d entries", tilename, numlines);
		//increment the pointer to the start of the next record
		pFileData = strchr(pFileData, '\n') + 1;
		numGroundTypes = numlines;
		psGroundTypes = (GROUND_TYPE *)malloc(sizeof(GROUND_TYPE) * numlines);

		for (i = 0; i < numlines; i++)
		{
			sscanf(pFileData, "%255[^,'\r\n],%255[^,'\r\n],%lf%n", textureType, textureName, &textureSize, &cnt);
			pFileData += cnt;
			//increment the pointer to the start of the next record
			pFileData = strchr(pFileData, '\n') + 1;

			psGroundTypes[getTextureType(textureType)].textureName = strdup(textureName);
			psGroundTypes[getTextureType(textureType)].textureSize = textureSize;
		}

		SetGroundForTile("tileset/urbanground.txt", "urban_ground");
		SetDecals("tileset/urbandecals.txt", "urban_decals");
	}
	// for Rockie
	else if (strcmp(tilesetDir, "texpages/tertilesc3hw") == 0)
	{
		init_tileNames(ROCKIE);
		if (!loadFileToBuffer("tileset/tertilesc3hwGtype.txt", pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
		{
			debug(LOG_POPUP, "tileset/tertilesc3hwGtype.txt not found, using default terrain ground types.");
			goto fallback;
		}

		sscanf(pFileData, "%255[^,'\r\n],%d%n", tilename, &numlines, &cnt);
		pFileData += cnt;

		if (strcmp(tilename, "tertilesc3hw"))
		{
			debug(LOG_POPUP, "%s found, but was expecting tertilesc3hw!", tilename);
			goto fallback;
		}

		debug(LOG_TERRAIN, "tilename: %s, with %d entries", tilename, numlines);
		//increment the pointer to the start of the next record
		pFileData = strchr(pFileData, '\n') + 1;
		numGroundTypes = numlines;
		psGroundTypes = (GROUND_TYPE *)malloc(sizeof(GROUND_TYPE) * numlines);

		for (i = 0; i < numlines; i++)
		{
			sscanf(pFileData, "%255[^,'\r\n],%255[^,'\r\n],%lf%n", textureType, textureName, &textureSize, &cnt);
			pFileData += cnt;
			//increment the pointer to the start of the next record
			pFileData = strchr(pFileData, '\n') + 1;

			psGroundTypes[getTextureType(textureType)].textureName = strdup(textureName);
			psGroundTypes[getTextureType(textureType)].textureSize = textureSize;
		}

		SetGroundForTile("tileset/rockieground.txt", "rockie_ground");
		SetDecals("tileset/rockiedecals.txt", "rockie_decals");
	}
	// When a map uses something other than the above, we fallback to Arizona
	else
	{
		debug(LOG_ERROR, "unsupported tileset: %s", tilesetDir);
		debug(LOG_POPUP, "This is a UNSUPPORTED map with a custom tileset.\nDefaulting to tertilesc1hw -- map may look strange!");
		// HACK: / FIXME: For now, we just pretend this is a tertilesc1hw map.
		goto fallback;
	}
	return true;
}

// Parse the file to set up the ground type
static void SetGroundForTile(const char *filename, const char *nametype)
{
	char	*pFileData = nullptr;
	char	tilename[MAX_STR_LENGTH] = {'\0'};
	char	val1[MAX_STR_LENGTH], val2[MAX_STR_LENGTH], val3[MAX_STR_LENGTH], val4[MAX_STR_LENGTH];
	int		numlines = 0;
	int		cnt = 0, i = 0;
	uint32_t	fileSize = 0;

	pFileData = fileLoadBuffer;
	if (!loadFileToBuffer(filename, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
	{
		debug(LOG_FATAL, "%s not found, aborting.", filename);
		abort();
	}

	sscanf(pFileData, "%255[^,'\r\n],%d%n", tilename, &numlines, &cnt);
	pFileData += cnt;

	if (strcmp(tilename, nametype))
	{
		debug(LOG_FATAL, "%s found, but was expecting %s, aborting.", tilename, nametype);
		abort();
	}

	debug(LOG_TERRAIN, "tilename: %s, with %d entries", tilename, numlines);
	//increment the pointer to the start of the next record
	pFileData = strchr(pFileData, '\n') + 1;

	map = (int *)malloc(sizeof(int) * numlines * 2 * 2);	// this is a 3D array map[numlines][2][2]

	for (i = 0; i < numlines; i++)
	{
		sscanf(pFileData, "%255[^,'\r\n],%255[^,'\r\n],%255[^,'\r\n],%255[^,'\r\n]%n", val1, val2, val3, val4, &cnt);
		pFileData += cnt;
		//increment the pointer to the start of the next record
		pFileData = strchr(pFileData, '\n') + 1;

		// inline int iA(int i, int j, int k){ return i*N2*N3 + j*N3 + k; }
		// in case it isn't obvious, this is a 3D array, and using pointer math to access each element.
		// so map[10][0][1] would be map[10*2*2 + 0 + 1] == map[41]
		// map[10][1][0] == map[10*2*2 + 2 + 0] == map[42]
		map[i * 2 * 2 + 0 * 2 + 0] = getTextureType(val1);
		map[i * 2 * 2 + 0 * 2 + 1] = getTextureType(val2);
		map[i * 2 * 2 + 1 * 2 + 0] = getTextureType(val3);
		map[i * 2 * 2 + 1 * 2 + 1] = getTextureType(val4);
	}
}

// getTextureType() -- just returns the value for that texture type.
static int getTextureType(const char *textureType)
{
	int i = 0;
	for (i = 0; i < numTile_names; i++)
	{
		if (!strcmp(textureType, &Tile_names[i * MAX_STR_LENGTH]))
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
	return map[TileNumber_tile(tile) * 2 * 2 + j * 2 + k];
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
	while (rot < 0)
	{
		rot += 4;
	}
	*i = invmap[rot][0];
	*j = invmap[rot][1];
}

/// Tries to figure out what ground type a grid point is from the surrounding tiles
static int determineGroundType(int x, int y, const char *tileset)
{
	int ground[2][2];
	int votes[2][2];
	int weight[2][2];
	int i, j, tile;
	int a, b, best;
	MAPTILE *psTile;

	if (x < 0 || y < 0 || x >= mapWidth || y >= mapHeight)
	{
		return 0; // just return the first ground type
	}

	// check what tiles surround this grid point
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			if (x + i - 1 < 0 || y + j - 1 < 0 || x + i - 1 >= mapWidth || y + j - 1 >= mapHeight)
			{
				psTile = nullptr;
				tile = 0;
			}
			else
			{
				psTile = mapTile(x + i - 1, y + j - 1);
				tile = psTile->texture;
			}
			a = i;
			b = j;
			rotFlip(tile, &a, &b);
			ground[i][j] = groundFromMapTile(tile, a, b);

			votes[i][j] = 0;
			// votes are weighted, some tiles have more weight than others
			weight[i][j] = 10;

			if (psTile)
			{
				// cliff tiles have higher priority, to be clearly visible
				if (terrainType(psTile) == TER_CLIFFFACE)
				{
					weight[i][j] = 100;
				}
				// water bottom has lower priority, to stay inside water
				if (terrainType(psTile) == TER_WATER)
				{
					weight[i][j] = 1;
				}
			}
		}
	}

	// now vote, because some maps have seams
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			for (a = 0; a < 2; a++)
			{
				for (b = 0; b < 2; b++)
				{
					if (ground[i][j] == ground[a][b])
					{
						votes[i][j] += weight[a][b];
					}
				}
			}
		}
	}
	// and determine the winner
	best = -1;
	a = 0;
	b = 0;
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			if (votes[i][j] > best || (votes[i][j] == best && ground[i][j] < ground[a][b]))
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
	char decalname[MAX_STR_LENGTH], *pFileData;
	int numlines, cnt, i, tiledecal;
	uint32_t fileSize;

	pFileData = fileLoadBuffer;

	if (!loadFileToBuffer(filename, pFileData, FILE_LOAD_BUFFER_SIZE, &fileSize))
	{
		debug(LOG_POPUP, "%s not found, aborting.", filename);
		abort();
	}

	sscanf(pFileData, "%255[^,'\r\n],%d%n", decalname, &numlines, &cnt);
	pFileData += cnt;

	if (strcmp(decalname, decal_type))
	{
		debug(LOG_POPUP, "%s found, but was expecting %s, aborting.", decalname, decal_type);
		abort();
	}

	debug(LOG_TERRAIN, "reading: %s, with %d entries", filename, numlines);
	//increment the pointer to the start of the next record
	pFileData = strchr(pFileData, '\n') + 1;
	mapDecals = new bool[MAX_TERRAIN_TILES];
	std::fill_n(mapDecals, MAX_TERRAIN_TILES, false);  // set everything to false.

	for (i = 0; i < numlines; i++)
	{
		tiledecal = -1;
		sscanf(pFileData, "%d%n", &tiledecal, &cnt);
		pFileData += cnt;
		//increment the pointer to the start of the next record
		pFileData = strchr(pFileData, '\n') + 1;
		if ((unsigned)tiledecal > MAX_TERRAIN_TILES)
		{
			debug(LOG_ERROR, "Tile index is out of range!  Was %d, our max is %d", tiledecal, MAX_TERRAIN_TILES);
			continue;
		}
		mapDecals[tiledecal] = true;
	}
}
// hasDecals()
// Checks to see if the requested tile has a decal on it or not.
static bool hasDecals(int i, int j)
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
static bool mapSetGroundTypes()
{
	int i, j;

	for (i = 0; i < mapWidth; i++)
	{
		for (j = 0; j < mapHeight; j++)
		{
			MAPTILE *psTile = mapTile(i, j);

			psTile->ground = determineGroundType(i, j, tilesetDir);

			if (hasDecals(i, j))
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

static bool isWaterVertex(int x, int y)
{
	if (x < 1 || y < 1 || x > mapWidth - 1 || y > mapHeight - 1)
	{
		return false;
	}
	return terrainType(mapTile(x, y)) == TER_WATER && terrainType(mapTile(x - 1, y)) == TER_WATER
	       && terrainType(mapTile(x, y - 1)) == TER_WATER && terrainType(mapTile(x - 1, y - 1)) == TER_WATER;
}

static void generateRiverbed()
{
	MersenneTwister mt(12345);  // 12345 = random seed.
	int maxIdx = 1, idx[MAP_MAXWIDTH][MAP_MAXHEIGHT];
	int i, j, l = 0;

	for (i = 0; i < mapWidth; i++)
	{
		for (j = 0; j < mapHeight; j++)
		{
			// initially set the seabed index to 0 for ground and 100 for water
			idx[i][j] = 100 * isWaterVertex(i, j);
			if (idx[i][j] > 0)
			{
				l++;
			}
		}
	}
	debug(LOG_TERRAIN, "Generating riverbed for %d water tiles.", l);
	if (l == 0) // no water on map
	{
		return;
	}
	l = 0;
	do
	{
		maxIdx = 1;
		for (i = 1; i < mapWidth - 2; i++)
		{
			for (j = 1; j < mapHeight - 2; j++)
			{

				if (idx[i][j] > 0)
				{
					idx[i][j] = (idx[i - 1][j] + idx[i][j - 1] + idx[i][j + 1] + idx[i + 1][j]) / 4;
					if (idx[i][j] > maxIdx)
					{
						maxIdx = idx[i][j];
					}
				}
			}
		}
		++l;
		debug(LOG_TERRAIN, "%d%% completed after %d iterations", 10 * (100 - maxIdx), l);
	}
	while (maxIdx > 90 && l < 20);

	for (i = 0; i < mapWidth; i++)
	{
		for (j = 0; j < mapHeight; j++)
		{
			if (idx[i][j] > maxIdx)
			{
				idx[i][j] = maxIdx;
			}
			if (idx[i][j] < 1)
			{
				idx[i][j] = 1;
			}
			if (isWaterVertex(i, j))
			{
				l = (WATER_MAX_DEPTH + 1 - WATER_MIN_DEPTH) * (maxIdx - idx[i][j] - mt.u32() % (maxIdx / 6 + 1));
				mapTile(i, j)->height -= WATER_MIN_DEPTH - (l / maxIdx);
			}
		}
	}

}

/* Initialise the map structure */
bool mapLoad(char *filename, bool preview)
{
	UDWORD		numGw, width, height;
	char		aFileType[4];
	UDWORD		version;
	UDWORD		i, x, y;
	PHYSFS_file	*fp = PHYSFS_openRead(filename);
	MersenneTwister mt(12345);  // 12345 = random seed.

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

	if (width <= 1 || height <= 1)
	{
		debug(LOG_ERROR, "Map is too small : %u, %u", width, height);
		goto failure;
	}

	/* See if this is the first time a map has been loaded */
	ASSERT(psMapTiles == nullptr, "Map has not been cleared before calling mapLoad()!");

	/* Allocate the memory for the map */
	psMapTiles = (MAPTILE *)calloc(width * height, sizeof(MAPTILE));
	ASSERT(psMapTiles != nullptr, "Out of memory");

	mapWidth = width;
	mapHeight = height;

	// FIXME: the map preview code loads the map without setting the tileset
	if (!tilesetDir)
	{
		tilesetDir = strdup("texpages/tertilesc1hw");
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
		psMapTiles[i].height = height * ELEVATION_SCALE;

		// Visibility stuff
		memset(psMapTiles[i].watchers, 0, sizeof(psMapTiles[i].watchers));
		memset(psMapTiles[i].sensors, 0, sizeof(psMapTiles[i].sensors));
		memset(psMapTiles[i].jammers, 0, sizeof(psMapTiles[i].jammers));
		psMapTiles[i].sensorBits = 0;
		psMapTiles[i].jammerBits = 0;
		psMapTiles[i].tileExploredBits = 0;
	}

	if (preview)
	{
		// no need to do anything else for the map preview
		goto ok;
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
			debug(LOG_ERROR, "%s: Unable to add gateway %d - dropping it", filename, i);
		}
	}

	if (!mapSetGroundTypes())
	{
		goto failure;
	}

	for (y = 0; y < mapHeight; y++)
	{
		for (x = 0; x < mapWidth; x++)
		{
			// FIXME: magic number
			mapTile(x, y)->waterLevel = mapTile(x, y)->height - world_coord(1) / 3;
		}
	}
	generateRiverbed();

	/* set up the scroll mins and maxs - set values to valid ones for any new map */
	scrollMinX = scrollMinY = 0;
	scrollMaxX = mapWidth;
	scrollMaxY = mapHeight;

	/* Allocate aux maps */
	psBlockMap[AUX_MAP] = (uint8_t *)malloc(mapWidth * mapHeight * sizeof(*psBlockMap[0]));
	psBlockMap[AUX_ASTARMAP] = (uint8_t *)malloc(mapWidth * mapHeight * sizeof(*psBlockMap[0]));
	psBlockMap[AUX_DANGERMAP] = (uint8_t *)malloc(mapWidth * mapHeight * sizeof(*psBlockMap[0]));
	for (x = 0; x < MAX_PLAYERS + AUX_MAX; x++)
	{
		psAuxMap[x] = (uint8_t *)malloc(mapWidth * mapHeight * sizeof(*psAuxMap[0]));
	}

	// Set our blocking bits
	for (y = 0; y < mapHeight; y++)
	{
		for (x = 0; x < mapWidth; x++)
		{
			MAPTILE *psTile = mapTile(x, y);

			auxClearBlocking(x, y, AUXBITS_ALL);
			auxClearAll(x, y, AUXBITS_ALL);

			/* All tiles outside of the map and on map border are blocking. */
			if (x < 1 || y < 1 || x > mapWidth - 1 || y > mapHeight - 1)
			{
				auxSetBlocking(x, y, AUXBITS_ALL);	// block everything
			}
			if (terrainType(psTile) == TER_WATER)
			{
				auxSetBlocking(x, y, WATER_BLOCKED);
			}
			else
			{
				auxSetBlocking(x, y, LAND_BLOCKED);
			}
			if (terrainType(psTile) == TER_CLIFFFACE)
			{
				auxSetBlocking(x, y, FEATURE_BLOCKED);
			}
		}
	}

	/* Set continents. This should ideally be done in advance by the map editor. */
	mapFloodFillContinents();
ok:
	PHYSFS_close(fp);
	return true;

failure:
	PHYSFS_close(fp);
	return false;
}

/* Save the map data */
bool mapSave(char **ppFileData, UDWORD *pFileSize)
{
	MAP_SAVEHEADER	*psHeader = nullptr;
	MAP_SAVETILE	*psTileData = nullptr;
	MAPTILE	*psTile = nullptr;
	GATEWAY_SAVEHEADER *psGateHeader = nullptr;
	GATEWAY_SAVE *psGate = nullptr;
	SDWORD	numGateways = gwNumGateways();

	/* Allocate the data buffer */
	*pFileSize = SAVE_HEADER_SIZE + mapWidth * mapHeight * SAVE_TILE_SIZE;
	// Add on the size of the gateway data.
	*pFileSize += sizeof(GATEWAY_SAVEHEADER) + sizeof(GATEWAY_SAVE) * numGateways;

	*ppFileData = (char *)malloc(*pFileSize);
	if (*ppFileData == nullptr)
	{
		debug(LOG_FATAL, "Out of memory");
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
	for (int i = 0; i < mapWidth * mapHeight; i++)
	{
		psTileData->texture = psTile->texture;
		if (terrainType(psTile) == TER_WATER)
		{
			psTileData->height = (psTile->waterLevel + world_coord(1) / 3) / ELEVATION_SCALE;
		}
		else
		{
			psTileData->height = psTile->height / ELEVATION_SCALE;
		}

		/* MAP_SAVETILE */
		endian_uword(&psTileData->texture);

		psTileData = (MAP_SAVETILE *)((UBYTE *)psTileData + SAVE_TILE_SIZE);
		psTile++;
	}

	// Put the gateway header.
	psGateHeader = (GATEWAY_SAVEHEADER *)psTileData;
	psGateHeader->version = 1;
	psGateHeader->numGateways = numGateways;

	/* GATEWAY_SAVEHEADER */
	endian_udword(&psGateHeader->version);
	endian_udword(&psGateHeader->numGateways);

	psGate = (GATEWAY_SAVE *)(psGateHeader + 1);

	// Put the gateway data.
	for (auto psCurrGate : gwGetGateways())
	{
		psGate->x0 = psCurrGate->x1;
		psGate->y0 = psCurrGate->y1;
		psGate->x1 = psCurrGate->x2;
		psGate->y1 = psCurrGate->y2;
		ASSERT(psGate->x0 == psGate->x1 || psGate->y0 == psGate->y1, "Invalid gateway coordinates (%d, %d, %d, %d)",
		       psGate->x0, psGate->y0, psGate->x1, psGate->y1);
		ASSERT(psGate->x0 < mapWidth && psGate->y0 < mapHeight && psGate->x1 < mapWidth && psGate->y1 < mapHeight,
		       "Bad gateway dimensions for savegame");
		psGate++;
	}

	return true;
}

/* Shutdown the map module */
bool mapShutdown()
{
	int x;

	if (dangerThread)
	{
		wzSemaphoreWait(dangerDoneSemaphore);
		lastDangerPlayer = -1;
		wzSemaphorePost(dangerSemaphore);
		wzThreadJoin(dangerThread);
		wzSemaphoreDestroy(dangerSemaphore);
		wzSemaphoreDestroy(dangerDoneSemaphore);
		dangerThread = nullptr;
		dangerSemaphore = nullptr;
		dangerDoneSemaphore = nullptr;
	}

	free(psMapTiles);
	delete[] mapDecals;
	free(psGroundTypes);
	free(map);
	free(Tile_names);
	free(psBlockMap[AUX_MAP]);
	psBlockMap[AUX_MAP] = nullptr;
	free(psBlockMap[AUX_ASTARMAP]);
	psBlockMap[AUX_ASTARMAP] = nullptr;
	free(psBlockMap[AUX_DANGERMAP]);
	free(floodbucket);
	psBlockMap[AUX_DANGERMAP] = nullptr;
	for (x = 0; x < MAX_PLAYERS + AUX_MAX; x++)
	{
		free(psAuxMap[x]);
		psAuxMap[x] = nullptr;
	}

	map = nullptr;
	floodbucket = nullptr;
	psGroundTypes = nullptr;
	mapDecals = nullptr;
	psMapTiles = nullptr;
	mapWidth = mapHeight = 0;
	numTile_names = 0;
	Tile_names = nullptr;
	return true;
}

/**
 * Intersect a tile with a line and report the points of intersection
 * line is gives as point plus 2d directional vector
 * returned are two coordinates at the edge
 * true if the intersection also crosses the tile split line
 * (which has to be taken into account)
 **/
bool map_Intersect(int *Cx, int *Cy, int *Vx, int *Vy, int *Sx, int *Sy)
{
	int	 x, y, ox, oy, Dx, Dy, tileX, tileY;
	int	 ily, iry, itx, ibx;

	// dereference pointers
	x = *Cx;
	y = *Cy;
	Dx = *Vx;
	Dy = *Vy;

	/* Turn into tile coordinates */
	tileX = map_coord(x);
	tileY = map_coord(y);

	/* Inter tile comp */
	ox = map_round(x);
	oy = map_round(y);

	/* allow backwards tracing */
	if (ox == 0 && Dx < 0)
	{
		tileX--;
		ox = TILE_UNITS;
	}
	if (oy == 0 && Dy < 0)
	{
		tileY--;
		oy = TILE_UNITS;
	}

	*Cx = -4 * TILE_UNITS; // to trigger assertion
	*Cy = -4 * TILE_UNITS;
	*Vx = -4 * TILE_UNITS;
	*Vy = -4 * TILE_UNITS;

	// calculate intersection point on the left and right (if any)
	ily = y - 4 * TILE_UNITS; // make sure initial value is way outside of tile
	iry = y - 4 * TILE_UNITS;
	if (Dx != 0)
	{
		ily = y - ox * Dy / Dx;
		iry = y + (TILE_UNITS - ox) * Dy / Dx;
	}
	// calculate intersection point on top and bottom (if any)
	itx = x - 4 * TILE_UNITS; // make sure initial value is way outside of tile
	ibx = x - 4 * TILE_UNITS;
	if (Dy != 0)
	{
		itx = x - oy * Dx / Dy;
		ibx = x + (TILE_UNITS - oy) * Dx / Dy;
	}

	// line comes from the left?
	if (Dx >= 0)
	{
		if (map_coord(ily) == tileY || map_coord(ily - 1) == tileY)
		{
			*Cx = world_coord(tileX);
			*Cy = ily;
		}
		if (map_coord(iry) == tileY || map_coord(iry - 1) == tileY)
		{
			*Vx = world_coord(tileX + 1);
			*Vy = iry;
		}
	}
	else
	{
		if (map_coord(ily) == tileY || map_coord(ily - 1) == tileY)
		{
			*Vx = world_coord(tileX);
			*Vy = ily;
		}
		if (map_coord(iry) == tileY || map_coord(iry - 1) == tileY)
		{
			*Cx = world_coord(tileX + 1);
			*Cy = iry;
		}
	}
	// line comes from the top?
	if (Dy >= 0)
	{
		if (map_coord(itx) == tileX || map_coord(itx - 1) == tileX)
		{
			*Cx = itx;
			*Cy = world_coord(tileY);
		}
		if (map_coord(ibx) == tileX || map_coord(ibx - 1) == tileX)
		{
			*Vx = ibx;
			*Vy = world_coord(tileY + 1);
		}
	}
	else
	{
		if (map_coord(itx) == tileX || map_coord(itx - 1) == tileX)
		{
			*Vx = itx;
			*Vy = world_coord(tileY);
		}
		if (map_coord(ibx) == tileX || map_coord(ibx - 1) == tileX)
		{
			*Cx = ibx;
			*Cy = world_coord(tileY + 1);
		}
	}
	// assertions, no intersections outside of tile
	ASSERT(*Cx >= world_coord(tileX) && *Cx <= world_coord(tileX + 1), "map_Intersect(): tile Bounds %i %i, %i %i -> %i,%i,%i,%i", x, y, Dx, Dy, *Cx, *Cy, *Vx, *Vy);
	ASSERT(*Cy >= world_coord(tileY) && *Cy <= world_coord(tileY + 1), "map_Intersect(): tile Bounds %i %i, %i %i -> %i,%i,%i,%i", x, y, Dx, Dy, *Cx, *Cy, *Vx, *Vy);
	ASSERT(*Vx >= world_coord(tileX) && *Vx <= world_coord(tileX + 1), "map_Intersect(): tile Bounds %i %i, %i %i -> %i,%i,%i,%i", x, y, Dx, Dy, *Cx, *Cy, *Vx, *Vy);
	ASSERT(*Vy >= world_coord(tileY) && *Vy <= world_coord(tileY + 1), "map_Intersect(): tile Bounds %i %i, %i %i -> %i,%i,%i,%i", x, y, Dx, Dy, *Cx, *Cy, *Vx, *Vy);
	ASSERT(tileX >= 0 && tileY >= 0 && tileX < mapWidth && tileY < mapHeight, "map_Intersect(): map Bounds %i %i, %i %i -> %i,%i,%i,%i", x, y, Dx, Dy, *Cx, *Cy, *Vx, *Vy);

	//calculate midway line intersection points
	if (((map_coord(itx) == tileX) == (map_coord(ily) == tileY)) && ((map_coord(ibx) == tileX) == (map_coord(iry) == tileY)))
	{
		// line crosses diagonal only
		if (Dx - Dy == 0)
		{
			return false;
		}
		*Sx = world_coord(tileX) + (Dx * oy - Dy * ox) / (Dx - Dy);
		*Sy = world_coord(tileY) + (Dx * oy - Dy * ox) / (Dx - Dy);
		if (map_coord(*Sx) != tileX || map_coord(*Sy) != tileY)
		{
			return false;
		}
		return true;
	}
	else if (((map_coord(ibx) == tileX) == (map_coord(ily) == tileY)) && ((map_coord(itx) == tileX) == (map_coord(iry) == tileY)))
	{
		//line crosses anti-diagonal only
		if (Dx + Dy == 0)
		{
			return false;
		}
		*Sx = world_coord(tileX) + (Dx * (TILE_UNITS - oy) + Dy * ox) / (Dx + Dy);
		*Sy = world_coord(tileY) + (Dy * (TILE_UNITS - ox) + Dx * oy) / (Dx + Dy);
		if (map_coord(*Sx) != tileX || map_coord(*Sy) != tileY)
		{
			return false;
		}
		return true;
	}
	else
	{
		//line crosses both tile diagonals
		//TODO: trunk divides tiles into 4 parts instead of 2 in 2.3.
		//We would need to check and return both intersections here now,
		//but that would require an additional return parameter!
		//Instead we check only one of them and know it might be wrong!
		if (Dx + Dy != 0)
		{
			// check anti-diagonal
			*Sx = world_coord(tileX) + (Dx * (TILE_UNITS - oy) + Dy * ox) / (Dx + Dy);
			*Sy = world_coord(tileY) + (Dy * (TILE_UNITS - ox) + Dx * oy) / (Dx + Dy);
			if (map_coord(*Sx) == tileX && map_coord(*Sy) == tileY)
			{
				return true;
			}
		}
		if (Dx - Dy != 0)
		{
			// check diagonal
			*Sx = world_coord(tileX) + (Dx * oy - Dy * ox) / (Dx - Dy);
			*Sy = world_coord(tileY) + (Dx * oy - Dy * ox) / (Dx - Dy);
			if (map_coord(*Sx) == tileX && map_coord(*Sy) == tileY)
			{
				return true;
			}
		}
	}

	return false;
}

// Rotate vector clockwise by quadrant*90° around (TILE_UNITS/2, TILE_UNITS/2). (Considering x to be to the right, and y down.)
static Vector3i rotateWorldQuadrant(Vector3i v, int quadrant)
{
	switch (quadrant & 3)
	{
	default:  // Can't get here.
	case 0: return v;                                                 break;  // 0°.
	case 1: return Vector3i(TILE_UNITS - v.y,              v.x, v.z); break;  // 90° clockwise.
	case 2: return Vector3i(TILE_UNITS - v.x, TILE_UNITS - v.y, v.z); break;  // 180°.
	case 3: return Vector3i(v.y, TILE_UNITS - v.x, v.z); break;               // 90° anticlockwise.
	}
}

// Returns (0, 0) rotated clockwise quadrant*90° around (½, ½). (Considering x to be to the right, and y down.)
static Vector2i quadrantCorner(int quadrant)
{
	int dx[4] = {0, 1, 1, 0};
	int dy[4] = {0, 0, 1, 1};
	return Vector2i(dx[quadrant & 3], dy[quadrant & 3]);
}

// Returns (0, -1) rotated clockwise quadrant*90° around (0, 0). (Considering x to be to the right, and y down.)
static Vector2i quadrantDelta(int quadrant)
{
	int dx[4] = {0,  1, 0, -1};
	int dy[4] = { -1, 0, 1,  0};
	return Vector2i(dx[quadrant & 3], dy[quadrant & 3]);
}


static inline bool fracTest(int numerA, int denomA, int numerB, int denomB)
{
	return denomA > 0 && numerA >= 0 && (denomB <= 0 || numerB < 0 || (int64_t)numerA * denomB < (int64_t)numerB * denomA);
}

unsigned map_LineIntersect(Vector3i src, Vector3i dst, unsigned tMax)
{
	// Transform src and dst to a coordinate system such that the tile quadrant containing src has
	// corners at (0, 0), (TILE_UNITS, 0), (TILE_UNITS/2, TILE_UNITS/2).
	Vector2i tile = map_coord(src.xy);
	src -= Vector3i(world_coord(tile), 0);
	dst -= Vector3i(world_coord(tile), 0);
	//            +0+
	// quadrant = 3×1
	//            +2+
	int quadrant = ((src.x < src.y) * 3) ^ (TILE_UNITS - src.x < src.y);
	src = rotateWorldQuadrant(src, -quadrant);
	dst = rotateWorldQuadrant(dst, -quadrant);
	while (true)
	{
		int height[4];
		for (int q = 0; q < 4; ++q)
		{
			Vector2i corner = tile + quadrantCorner(quadrant + q);
			height[q] = map_TileHeightSurface(corner.x, corner.y);
		}
		Vector3i dif = dst - src;
		//     We are considering the volume of a quadrant (the volume above a quarter of a map tile, which is
		// a degenerate tetrahedron with a point at infinity). We have a line segment, and want to know where
		// it exits the quadrant volume.
		//     There are 5 possible cases. Cases 0-2, our line can exit one of the three sides of the quadrant
		// volume (and pass into a neighbouring quadrant volume), or case 3, can exit through the bottom of the
		// quadrant volume (which means intersecting the terrain), or case 4, the line segment can end (which
		// means reaching the destination with no intersection.
		//     Note that the height of the centre of the tile is the average of the corners, such that a tile
		// consists of 4 flat triangles (which are not (in general) parallel to each other).
		// +--0--+
		//  \ 3 /
		//   2 1
		//    +
		// Denominators are positive iff we are going in the direction of the line. First line crossed has the smallest fraction.
		// numer/denom gives the intersection times for the 5 cases.
		int numer[5], denom[5];
		numer[0] = -(-src.y);
		denom[0] =   -dif.y;
		numer[1] = TILE_UNITS - (src.x + src.y);
		denom[1] =               dif.x + dif.y;
		numer[2] = -(-src.x + src.y);
		denom[2] =   -dif.x + dif.y;
		Vector3i normal(2 * (height[1] - height[0]), height[2] + height[3] - height[0] - height[1], -2 * TILE_UNITS); // Normal pointing down, and not normalised.
		numer[3] = height[0] * normal.z - dot(src, normal);
		denom[3] =                      dot(dif, normal);
		numer[4] = 1;
		denom[4] = 1;
		int firstIntersection = 0;
		for (int test = 0; test < 5; ++test)
		{
			if (!fracTest(numer[firstIntersection], denom[firstIntersection], numer[test], denom[test]))
			{
				firstIntersection = test;
			}
		}
		switch (firstIntersection)
		{
		case 0:  // Cross top line first (the tile boundary).
			tile += quadrantDelta(quadrant);
			quadrant += 2;
			src = rotateWorldQuadrant(src, -2) + Vector3i(0, -TILE_UNITS, 0);
			dst = rotateWorldQuadrant(dst, -2) + Vector3i(0, -TILE_UNITS, 0);

			if (tile.x < 0 || tile.x >= mapWidth || tile.y < 0 || tile.y >= mapHeight)
			{
				// Intersect edge of map.
				return (int64_t)tMax * numer[firstIntersection] / denom[firstIntersection];
			}
			break;
		case 1:  // Cross bottom-right line first.
			// Change to the new quadrant, and transform appropriately.
			++quadrant;
			src = rotateWorldQuadrant(src, -1);
			dst = rotateWorldQuadrant(dst, -1);
			break;
		case 2:  // Cross bottom-left line first.
			// Change to the new quadrant, and transform appropriately.
			--quadrant;
			src = rotateWorldQuadrant(src, 1);
			dst = rotateWorldQuadrant(dst, 1);
			break;
		case 3:  // Intersect terrain!
			return (int64_t)tMax * numer[firstIntersection] / denom[firstIntersection];
		case 4:  // Line segment ends.
			return UINT32_MAX;
		}
	}
}

/// The max height of the terrain and water at the specified world coordinates
extern int32_t map_Height(int x, int y)
{
	int tileX, tileY;
	int i, j;
	int32_t height[2][2], center;
	int32_t onTileX, onTileY;
	int32_t left, right, middle;
	int32_t onBottom, result;
	int towardsCenter, towardsRight;

	// Clamp x and y values to actual ones
	// Give one tile worth of leeway before asserting, for units/transporters coming in from off-map.
	ASSERT(x >= -TILE_UNITS, "map_Height: x value is too small (%d,%d) in %dx%d", map_coord(x), map_coord(y), mapWidth, mapHeight);
	ASSERT(y >= -TILE_UNITS, "map_Height: y value is too small (%d,%d) in %dx%d", map_coord(x), map_coord(y), mapWidth, mapHeight);
	x = MAX(x, 0);
	y = MAX(y, 0);
	ASSERT(x < world_coord(mapWidth) + TILE_UNITS, "map_Height: x value is too big (%d,%d) in %dx%d", map_coord(x), map_coord(y), mapWidth, mapHeight);
	ASSERT(y < world_coord(mapHeight) + TILE_UNITS, "map_Height: y value is too big (%d,%d) in %dx%d", map_coord(x), map_coord(y), mapWidth, mapHeight);
	x = MIN(x, world_coord(mapWidth) - 1);
	y = MIN(y, world_coord(mapHeight) - 1);

	// on which tile are these coords?
	tileX = map_coord(x);
	tileY = map_coord(y);

	// where on the tile? (scale to (0,1))
	onTileX = x - world_coord(tileX);
	onTileY = y - world_coord(tileY);

	// get the height for the corners and center
	center = 0;
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			height[i][j] = map_TileHeightSurface(tileX + i, tileY + j);
			center += height[i][j];
		}
	}
	center /= 4;

	// we have:
	//   x ->
	// y 0,0--D--1,0
	// | |  \    / |
	// V A  centre C
	//   | /     \ |
	//   0,1--B--1,1

	// get heights for left and right corners and the distances
	if (onTileY > onTileX)
	{
		if (onTileY < TILE_UNITS - onTileX)
		{
			// A
			right = height[0][0];
			left  = height[0][1];
			towardsCenter = onTileX;
			towardsRight  = TILE_UNITS - onTileY;
		}
		else
		{
			// B
			right = height[0][1];
			left  = height[1][1];
			towardsCenter = TILE_UNITS - onTileY;
			towardsRight  = TILE_UNITS - onTileX;
		}
	}
	else
	{
		if (onTileX > TILE_UNITS - onTileY)
		{
			// C
			right = height[1][1];
			left  = height[1][0];
			towardsCenter = TILE_UNITS - onTileX;
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
	ASSERT(towardsCenter <= TILE_UNITS / 2, "towardsCenter is too high");

	// now we have:
	//    left   m    right
	//         center

	middle = (left + right) / 2;
	onBottom = left * (TILE_UNITS - towardsRight) + right * towardsRight;
	result = onBottom + (center - middle) * towardsCenter * 2;

	return (result + TILE_UNITS / 2) / TILE_UNITS;
}

/* returns true if object is above ground */
bool mapObjIsAboveGround(const SIMPLE_OBJECT *psObj)
{
	// min is used to make sure we don't go over array bounds!
	// TODO Using the corner of the map instead doesn't make sense. Fix this...
	const int mapsize = mapWidth * mapHeight - 1;
	const int tileX = map_coord(psObj->pos.x);
	const int tileY = map_coord(psObj->pos.y);
	const int tileYOffset1 = (tileY * mapWidth);
	const int tileYOffset2 = ((tileY + 1) * mapWidth);
	const int h1 = psMapTiles[MIN(mapsize, tileYOffset1 + tileX)    ].height;
	const int h2 = psMapTiles[MIN(mapsize, tileYOffset1 + tileX + 1)].height;
	const int h3 = psMapTiles[MIN(mapsize, tileYOffset2 + tileX)    ].height;
	const int h4 = psMapTiles[MIN(mapsize, tileYOffset2 + tileX + 1)].height;

	/* trivial test above */
	if (psObj->pos.z > h1 && psObj->pos.z > h2 && psObj->pos.z > h3 && psObj->pos.z > h4)
	{
		return true;
	}

	/* trivial test below */
	if (psObj->pos.z <= h1 && psObj->pos.z <= h2 && psObj->pos.z <= h3 && psObj->pos.z <= h4)
	{
		return false;
	}

	/* exhaustive test */
	return psObj->pos.z > map_Height(psObj->pos.x, psObj->pos.y);
}

/* returns the max and min height of a tile by looking at the four corners
   in tile coords */
void getTileMaxMin(int x, int y, int *pMax, int *pMin)
{
	*pMin = INT32_MAX;
	*pMax = INT32_MIN;

	for (int j = 0; j < 2; ++j)
		for (int i = 0; i < 2; ++i)
		{
			int height = map_TileHeight(x + i, y + j);
			*pMin = std::min(*pMin, height);
			*pMax = std::max(*pMax, height);
		}
}


// -----------------------------------------------------------------------------------
/* This will save out the visibility data */
bool writeVisibilityData(const char *fileName)
{
	unsigned int i;
	VIS_SAVEHEADER fileHeader;

	PHYSFS_file *fileHandle = openSaveFile(fileName);
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

	int planes = (game.maxPlayers + 7) / 8;

	for (unsigned plane = 0; plane < planes; ++plane)
	{
		for (i = 0; i < mapWidth * mapHeight; ++i)
		{
			if (!PHYSFS_writeUBE8(fileHandle, psMapTiles[i].tileExploredBits >> (plane * 8)))
			{
				debug(LOG_ERROR, "writeVisibilityData: could not write to %s; PHYSFS error: %s", fileName, PHYSFS_getLastError());
				PHYSFS_close(fileHandle);
				return false;
			}
		}
	}

	// Everything is just fine!
	PHYSFS_close(fileHandle);
	return true;
}

// -----------------------------------------------------------------------------------
/* This will read in the visibility data */
bool readVisibilityData(const char *fileName)
{
	VIS_SAVEHEADER fileHeader;
	unsigned int expectedFileSize, fileSize;
	unsigned int i;

	PHYSFS_file *fileHandle = openLoadFile(fileName, false);
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

	int planes = (game.maxPlayers + 7) / 8;

	// Validate the filesize
	expectedFileSize = sizeof(fileHeader.aFileType) + sizeof(fileHeader.version) + mapWidth * mapHeight * planes;
	fileSize = PHYSFS_fileLength(fileHandle);
	if (fileSize != expectedFileSize)
	{
		PHYSFS_close(fileHandle);
		ASSERT(!"readVisibilityData: unexpected filesize", "readVisibilityData: unexpected filesize; should be %u, but is %u", expectedFileSize, fileSize);

		return false;
	}

	// For every tile...
	for (i = 0; i < mapWidth * mapHeight; i++)
	{
		psMapTiles[i].tileExploredBits = 0;
	}
	for (unsigned plane = 0; plane < planes; ++plane)
	{
		for (i = 0; i < mapWidth * mapHeight; i++)
		{
			/* Get the visibility data */
			uint8_t val = 0;
			if (!PHYSFS_readUBE8(fileHandle, &val))
			{
				debug(LOG_ERROR, "readVisibilityData: could not read from %s; PHYSFS error: %s", fileName, PHYSFS_getLastError());
				PHYSFS_close(fileHandle);
				return false;
			}
			psMapTiles[i].tileExploredBits |= val << (plane * 8);
		}
	}

	// Close the file
	PHYSFS_close(fileHandle);

	/* Hopefully everything's just fine by now */
	return true;
}

// Convert a direction into an offset.
// dir 0 => x = 0, y = -1
#define NUM_DIR		8
static const Vector2i aDirOffset[] =
{
	Vector2i(0, 1),
	Vector2i(-1, 1),
	Vector2i(-1, 0),
	Vector2i(-1, -1),
	Vector2i(0, -1),
	Vector2i(1, -1),
	Vector2i(1, 0),
	Vector2i(1, 1),
};

// Flood fill a "continent".
// TODO take into account scroll limits and update continents on scroll limit changes
static void mapFloodFill(int x, int y, int continent, uint8_t blockedBits, uint16_t MAPTILE::*varContinent)
{
	std::vector<Vector2i> open;
	open.push_back(Vector2i(x, y));
	mapTile(x, y)->*varContinent = continent;  // Set continent value

	while (!open.empty())
	{
		// Pop the first open node off the list for this iteration
		Vector2i pos = open.back();
		open.pop_back();

		// Add accessible neighbouring tiles to the open list
		for (int i = 0; i < NUM_DIR; ++i)
		{
			// rely on the fact that all border tiles are inaccessible to avoid checking explicitly
			Vector2i npos = pos + aDirOffset[i];

			if (npos.x < 1 || npos.y < 1 || npos.x > mapWidth - 2 || npos.y > mapHeight - 2)
			{
				continue;
			}
			MAPTILE *psTile = mapTile(npos);

			if (!(blockTile(npos.x, npos.y, AUX_MAP) & blockedBits) && psTile->*varContinent == 0)
			{
				open.push_back(npos);               // add to open list
				psTile->*varContinent = continent;  // Set continent value
			}
		}
	}
}

void mapFloodFillContinents()
{
	int x, y, limitedContinents = 0, hoverContinents = 0;

	/* Clear continents */
	for (y = 0; y < mapHeight; y++)
	{
		for (x = 0; x < mapWidth; x++)
		{
			MAPTILE *psTile = mapTile(x, y);

			psTile->limitedContinent = 0;
			psTile->hoverContinent = 0;
		}
	}

	/* Iterate over the whole map, looking for unset continents */
	for (y = 1; y < mapHeight - 2; y++)
	{
		for (x = 1; x < mapWidth - 2; x++)
		{
			MAPTILE *psTile = mapTile(x, y);

			if (psTile->limitedContinent == 0 && !fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
			{
				mapFloodFill(x, y, 1 + limitedContinents++, WATER_BLOCKED | FEATURE_BLOCKED, &MAPTILE::limitedContinent);
			}
			else if (psTile->limitedContinent == 0 && !fpathBlockingTile(x, y, PROPULSION_TYPE_PROPELLOR))
			{
				mapFloodFill(x, y, 1 + limitedContinents++, LAND_BLOCKED | FEATURE_BLOCKED, &MAPTILE::limitedContinent);
			}

			if (psTile->hoverContinent == 0 && !fpathBlockingTile(x, y, PROPULSION_TYPE_HOVER))
			{
				mapFloodFill(x, y, 1 + hoverContinents++, FEATURE_BLOCKED, &MAPTILE::hoverContinent);
			}
		}
	}
	debug(LOG_MAP, "Found %d limited and %d hover continents", limitedContinents, hoverContinents);
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
	return psTile != nullptr && TileIsBurning(psTile);
}

// This function runs in a separate thread!
static int dangerFloodFill(int player)
{
	int i;
	Vector2i pos = getPlayerStartPosition(player);
	Vector2i npos;
	uint8_t aux, block;
	int x, y;
	bool start = true;	// hack to disregard the blocking status of any building exactly on the starting position

	// Set our danger bits
	for (y = 0; y < mapHeight; y++)
	{
		for (x = 0; x < mapWidth; x++)
		{
			auxSet(x, y, MAX_PLAYERS + AUX_DANGERMAP, AUXBITS_DANGER);
			auxClear(x, y, MAX_PLAYERS + AUX_DANGERMAP, AUXBITS_TEMPORARY);
		}
	}

	pos.x = map_coord(pos.x);
	pos.y = map_coord(pos.y);
	bucketcounter = 0;

	do
	{
		// Add accessible neighbouring tiles to the open list
		for (i = 0; i < NUM_DIR; i++)
		{
			npos.x = pos.x + aDirOffset[i].x;
			npos.y = pos.y + aDirOffset[i].y;
			if (!tileOnMap(npos.x, npos.y))
			{
				continue;
			}
			aux = auxTile(npos.x, npos.y, MAX_PLAYERS + AUX_DANGERMAP);
			block = blockTile(pos.x, pos.y, AUX_DANGERMAP);
			if (!(aux & AUXBITS_TEMPORARY) && !(aux & AUXBITS_THREAT) && (aux & AUXBITS_DANGER))
			{
				// Note that we do not consider water to be a blocker here. This may or may not be a feature...
				if (!(block & FEATURE_BLOCKED) && (!(aux & AUXBITS_NONPASSABLE) || start))
				{
					floodbucket[bucketcounter].x = npos.x;
					floodbucket[bucketcounter].y = npos.y;
					bucketcounter++;
					if (start && !(aux & AUXBITS_NONPASSABLE))
					{
						start = false;
					}
				}
				else
				{
					auxClear(npos.x, npos.y, MAX_PLAYERS + AUX_DANGERMAP, AUXBITS_DANGER);
				}
				auxSet(npos.x, npos.y, MAX_PLAYERS + AUX_DANGERMAP, AUXBITS_TEMPORARY); // make sure we do not process it more than once
			}
		}

		// Clear danger
		auxClear(pos.x, pos.y, MAX_PLAYERS + AUX_DANGERMAP, AUXBITS_DANGER);

		// Pop the last open node off the bucket list for the next iteration
		if (bucketcounter)
		{
			bucketcounter--;
			pos.x = floodbucket[bucketcounter].x;
			pos.y = floodbucket[bucketcounter].y;
		}
	}
	while (bucketcounter);
	return 0;
}

// This function runs in a separate thread!
static int dangerThreadFunc(WZ_DECL_UNUSED void *data)
{
	while (lastDangerPlayer != -1)
	{
		dangerFloodFill(lastDangerPlayer);	// Do the actual work
		wzSemaphorePost(dangerDoneSemaphore);   // Signal that we are done
		wzSemaphoreWait(dangerSemaphore);	// Go to sleep until needed.
	}
	return 0;
}

static inline void threatUpdateTarget(int player, BASE_OBJECT *psObj, bool ground, bool air)
{
	int i;

	if (psObj->visible[player] || psObj->born == 2)
	{
		for (i = 0; i < psObj->numWatchedTiles; i++)
		{
			const TILEPOS pos = psObj->watchedTiles[i];

			if (ground)
			{
				auxSet(pos.x, pos.y, MAX_PLAYERS + AUX_DANGERMAP, AUXBITS_THREAT);	// set ground threat for this tile
			}
			if (air)
			{
				auxSet(pos.x, pos.y, MAX_PLAYERS + AUX_DANGERMAP, AUXBITS_AATHREAT);	// set air threat for this tile
			}
		}
	}
}

static void threatUpdate(int player)
{
	int i, weapon, x, y;

	// Step 1: Clear our threat bits
	for (y = 0; y < mapHeight; y++)
	{
		for (x = 0; x < mapWidth; x++)
		{
			auxClear(x, y, MAX_PLAYERS + AUX_DANGERMAP, AUXBITS_THREAT | AUXBITS_AATHREAT);
		}
	}

	// Step 2: Set threat bits
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		DROID *psDroid;
		STRUCTURE *psStruct;

		if (aiCheckAlliances(player, i))
		{
			// No need to iterate friendly objects
			continue;
		}

		for (psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			UBYTE mode = 0;

			if (psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT
			    || psDroid->droidType == DROID_REPAIR || psDroid->droidType == DROID_CYBORG_REPAIR)
			{
				continue;	// hack that really should not be needed, but is -- trucks can SHOOT_ON_GROUND...!
			}
			for (weapon = 0; weapon < psDroid->numWeaps; weapon++)
			{
				mode |= asWeaponStats[psDroid->asWeaps[weapon].nStat].surfaceToAir;
			}
			if (psDroid->droidType == DROID_SENSOR)	// special treatment for sensor turrets, no multiweapon support
			{
				mode |= SHOOT_ON_GROUND;		// assume it only shoots at ground targets for now
			}
			if (mode > 0)
			{
				threatUpdateTarget(player, (BASE_OBJECT *)psDroid, mode & SHOOT_ON_GROUND, mode & SHOOT_IN_AIR);
			}
		}

		for (psStruct = apsStructLists[i]; psStruct; psStruct = psStruct->psNext)
		{
			UBYTE mode = 0;

			for (weapon = 0; weapon < psStruct->numWeaps; weapon++)
			{
				mode |= asWeaponStats[psStruct->asWeaps[weapon].nStat].surfaceToAir;
			}
			if (psStruct->pStructureType->pSensor && psStruct->pStructureType->pSensor->location == LOC_TURRET)	// special treatment for sensor turrets
			{
				mode |= SHOOT_ON_GROUND;		// assume it only shoots at ground targets for now
			}
			if (mode > 0)
			{
				threatUpdateTarget(player, (BASE_OBJECT *)psStruct, mode & SHOOT_ON_GROUND, mode & SHOOT_IN_AIR);
			}
		}
	}
}

void mapInit()
{
	int player;

	free(floodbucket);
	floodbucket = (struct floodtile *)malloc(mapWidth * mapHeight * sizeof(*floodbucket));

	lastDangerUpdate = 0;
	lastDangerPlayer = -1;

	// Start danger thread (not used for campaign for now - mission map swaps too icky)
	ASSERT(dangerSemaphore == nullptr && dangerThread == nullptr, "Map data not cleaned up before starting!");
	if (game.type == SKIRMISH)
	{
		for (player = 0; player < MAX_PLAYERS; player++)
		{
			auxMapStore(player, AUX_DANGERMAP);
			threatUpdate(player);
			dangerFloodFill(player);
			auxMapRestore(player, AUX_DANGERMAP, AUXBITS_DANGER | AUXBITS_THREAT | AUXBITS_AATHREAT);
		}
		lastDangerPlayer = 0;
		dangerSemaphore = wzSemaphoreCreate(0);
		dangerDoneSemaphore = wzSemaphoreCreate(0);
		dangerThread = wzThreadCreate(dangerThreadFunc, nullptr);
		wzThreadStart(dangerThread);
	}
}

void mapUpdate()
{
	const uint16_t currentTime = gameTime / GAME_TICKS_PER_UPDATE;
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

	if (gameTime > lastDangerUpdate + GAME_TICKS_FOR_DANGER && game.type == SKIRMISH)
	{
		syncDebug("Do danger maps.");
		lastDangerUpdate = gameTime;

		// Lock if previous job not done yet
		wzSemaphoreWait(dangerDoneSemaphore);

		auxMapRestore(lastDangerPlayer, AUX_DANGERMAP, AUXBITS_THREAT | AUXBITS_AATHREAT | AUXBITS_DANGER);
		lastDangerPlayer = (lastDangerPlayer + 1) % game.maxPlayers;
		auxMapStore(lastDangerPlayer, AUX_DANGERMAP);
		threatUpdate(lastDangerPlayer);
		wzSemaphorePost(dangerSemaphore);
	}
}
