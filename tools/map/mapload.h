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

#ifndef __INCLUDED_TOOLS_MAPLOAD_H__
#define __INCLUDED_TOOLS_MAPLOAD_H__

#include <stdint.h>
#include <stdbool.h>
#include "wzglobal.h"
#include "physfs_ext.h"

#define MAX_LEVEL_SIZE	20
#define TILE_HEIGHT	128
#define TILE_WIDTH	128

enum {
	IMD_FEATURE,
	IMD_STRUCTURE,
	IMD_DROID,
	IMD_OBJECT,
};

typedef struct _lnd_object_type
{
	uint32_t	id;
	uint32_t	player;
	int 		type;	// "IMD" LND object type
	char 		name[128];
	char 		script[32];
	uint32_t	x, y, z;
	uint32_t	direction;
} LND_OBJECT;

typedef enum _tileset_type
{
        TILESET_ARIZONA	= 0,
	TILESET_URBAN	= 1,
	TILESET_ROCKIES	= 2
} TILESET;

typedef struct _gateway
{
	uint8_t x1, y1, x2, y2;
} GATEWAY;

/* Information stored with each tile */
typedef struct _maptile_type
{
	uint8_t			tileInfoBits;
	uint8_t			tileVisBits;	// COMPRESSED - bit per player
	uint8_t			height;			// The height at the top left of the tile
	uint8_t			illumination;	// How bright is this tile?
	uint32_t		texture;		// Which graphics texture is on this tile
	float			level;
} MAPTILE;

typedef struct _mapfile_type
{
	uint32_t		height, width, mapVersion, gameVersion, numGateways, numFeatures, numTerrainTypes, power[8];
	uint32_t		numPlayers, numDroids, numStructures, droidVersion, structVersion, featVersion, terrainVersion;
	uint32_t		gameType, gameTime;
	int32_t			scrollMinX;
	int32_t			scrollMinY;
	uint32_t		scrollMaxX;
	uint32_t		scrollMaxY;
	char			levelName[MAX_LEVEL_SIZE];
	TILESET			tileset;

	// private members - don't touch! :-)
	GATEWAY			*mGateways;
	MAPTILE			*mMapTiles;
	LND_OBJECT		*mLndObjects[3];	// for map2lnd only
} GAMEMAP;

static inline MAPTILE *mapTile(GAMEMAP *map, int x, int y)
{
        return &map->mMapTiles[y * map->width + x];
}

static inline GATEWAY *mapGateway(GAMEMAP *map, int index)
{
	return &map->mGateways[index];
}

/* Load the map data */
GAMEMAP *mapLoad(char *filename);
void mapFree(GAMEMAP *map);

#endif
