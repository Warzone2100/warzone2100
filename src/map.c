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

/* load the map data - for version 3 */
static BOOL mapLoadV3(char *pFileData, UDWORD fileSize)
{
	UDWORD				i,j;
	MAP_SAVETILE		*psTileData;
	GATEWAY_SAVEHEADER	*psGateHeader;
	GATEWAY_SAVE		*psGate;

	/* Load in the map data */
	psTileData = (MAP_SAVETILE *)(pFileData + SAVE_HEADER_SIZE);
	for(i=0; i< mapWidth * mapHeight; i++)
	{
		/* MAP_SAVETILE */
		endian_uword(&psTileData->texture);

		psMapTiles[i].texture = psTileData->texture;
		psMapTiles[i].height = psTileData->height;

		// Visibility stuff
		memset(psMapTiles[i].watchers, 0, sizeof(psMapTiles[i].watchers));
		for (j=0; j<MAX_PLAYERS; j++)
		{
			psMapTiles[i].tileVisBits =(UBYTE)(( (psMapTiles[i].tileVisBits) &~ (UBYTE)(1<<j) ));
		}
		psTileData = (MAP_SAVETILE *)(((UBYTE *)psTileData) + SAVE_TILE_SIZE);
	}

	psGateHeader = (GATEWAY_SAVEHEADER*)psTileData;
	psGate = (GATEWAY_SAVE*)(psGateHeader+1);

	/* GATEWAY_SAVEHEADER */
	endian_udword(&psGateHeader->version);
	endian_udword(&psGateHeader->numGateways);

	ASSERT( psGateHeader->version == 1,"Invalid gateway version" );

	for(i=0; i<psGateHeader->numGateways; i++) {
		if (!gwNewGateway(psGate->x0,psGate->y0, psGate->x1,psGate->y1)) {
			debug( LOG_ERROR, "mapLoadV3: Unable to add gateway" );

			return false;
		}
		psGate++;
	}

	return true;
}


/* Initialise the map structure */
BOOL mapLoad(char *pFileData, UDWORD fileSize)
{
	UDWORD				width,height;
	MAP_SAVEHEADER		*psHeader;

	/* Check the file type */
	psHeader = (MAP_SAVEHEADER *)pFileData;
	if (psHeader->aFileType[0] != 'm' || psHeader->aFileType[1] != 'a' ||
		psHeader->aFileType[2] != 'p' || psHeader->aFileType[3] != ' ')
	{
		ASSERT(false, "Incorrect map type");
		free(pFileData);
		return false;
	}

	/* MAP_SAVEHEADER */
	endian_udword(&psHeader->version);
	endian_udword(&psHeader->width);
	endian_udword(&psHeader->height);

	/* Check the file version */
	if (psHeader->version <= VERSION_9)
	{
		ASSERT(false, "Unsupported save format version %d", psHeader->version);
		free(pFileData);
		return false;
	}
	else if (psHeader->version > CURRENT_VERSION_NUM)
	{
		ASSERT(false, "Undefined save format version %d", psHeader->version);
		free(pFileData);
		return false;
	}

	/* Get the width and height */
	width = psHeader->width;
	height = psHeader->height;

	if (width*height > MAP_MAXAREA)
	{
		debug( LOG_ERROR, "Map too large : %d %d\n", width, height );
		free(pFileData);
		return false;
	}

	if (width <=1 || height <=1)
	{
		debug(LOG_ERROR, "Map is too small : %u, %u", width, height);
		free(pFileData);
		return false;
	}

	/* See if this is the first time a map has been loaded */
	ASSERT(psMapTiles == NULL, "Map has not been cleared before calling mapLoad()!");

	/* Allocate the memory for the map */
	psMapTiles = calloc(width * height, sizeof(MAPTILE));
	ASSERT(psMapTiles != NULL, "Out of memory" );

	mapWidth = width;
	mapHeight = height;

	//load in the map data itself
	mapLoadV3(pFileData, fileSize);

	environReset();

	/* set up the scroll mins and maxs - set values to valid ones for any new map */
	scrollMinX = scrollMinY = 0;
	scrollMaxX = mapWidth;
	scrollMaxY = mapHeight;

	mapFloodFillContinents();

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
	if(psMapTiles)
	{
		free(psMapTiles);
	}
	psMapTiles = NULL;
	mapWidth = mapHeight = 0;

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
			h1 = psMapTiles[MIN(mapWidth * mapHeight, tileYOffset1 + tileX)    ].height * ELEVATION_SCALE,
			h2 = psMapTiles[MIN(mapWidth * mapHeight, tileYOffset1 + tileX + 1)].height * ELEVATION_SCALE,
			h3 = psMapTiles[MIN(mapWidth * mapHeight, tileYOffset2 + tileX)    ].height * ELEVATION_SCALE,
			h4 = psMapTiles[MIN(mapWidth * mapHeight, tileYOffset2 + tileX + 1)].height * ELEVATION_SCALE;

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
	int		i;
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
		route.numPoints = 0;
		astarResetCounters();
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
