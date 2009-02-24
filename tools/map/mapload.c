// gcc -o ~/bin/mapinfo mapload.c -I. -lphysfs

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "mapload.h"

#define MAX_PLAYERS	8
#define MAP_MAXAREA	(256 * 256)
#define debug(z, ...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while (0)

/* Initialise the map structure */
GAMEMAP *mapLoad(char *filename)
{
	GAMEMAP		*map = malloc(sizeof(*map));
	uint32_t	gwVersion, i, j;
	char		aFileType[4];
	PHYSFS_file	*fp = PHYSFS_openRead(filename);

	if (!fp)
	{
		debug(LOG_ERROR, "%s not found", filename);
		return NULL;
	}
	else if (PHYSFS_read(fp, aFileType, 4, 1) != 1
	    || !PHYSFS_readULE32(fp, &map->version)
	    || !PHYSFS_readULE32(fp, &map->width)
	    || !PHYSFS_readULE32(fp, &map->height)
	    || aFileType[0] != 'm'
	    || aFileType[1] != 'a'
	    || aFileType[2] != 'p')
	{
		debug(LOG_ERROR, "Bad header in %s", filename);
		return NULL;
	}
	else if (map->version <= 9)
	{
		debug(LOG_ERROR, "%s: Unsupported save format version %u", filename, map->version);
		return NULL;
	}
	else if (map->version > 36)
	{
		debug(LOG_ERROR, "%s: Undefined save format version %u", filename, map->version);
		return NULL;
	}
	else if (map->width * map->height > MAP_MAXAREA)
	{
		debug(LOG_ERROR, "Map %s too large : %d %d", filename, map->width, map->height);
		return NULL;
	}

	/* Allocate the memory for the map */
	map->psMapTiles = calloc(map->width * map->height, sizeof(*map->psMapTiles));
	if (!map->psMapTiles)
	{
		debug(LOG_ERROR, "Out of memory");
		return NULL;
	}
	
	/* Load in the map data */
	for (i = 0; i < map->width * map->height; i++)
	{
		uint16_t	texture;
		uint8_t		height;

		if (!PHYSFS_readULE16(fp, &texture) || !PHYSFS_readULE8(fp, &height))
		{
			debug(LOG_ERROR, "%s: Error during savegame load", filename);
			return NULL;
		}

		map->psMapTiles[i].texture = texture;
		map->psMapTiles[i].height = height;
		for (j = 0; j < MAX_PLAYERS; j++)
		{
			map->psMapTiles[i].tileVisBits = (uint8_t)(map->psMapTiles[i].tileVisBits &~ (uint8_t)(1 << j));
		}
	}

	if (!PHYSFS_readULE32(fp, &gwVersion) || !PHYSFS_readULE32(fp, &map->numGateways) || gwVersion != 1)
	{
		debug(LOG_ERROR, "Bad gateway in %s", filename);
		return NULL;
	}

	for (i = 0; i < map->numGateways; i++)
	{
		uint8_t		x0, y0, x1, y1;

		if (!PHYSFS_readULE8(fp, &x0) || !PHYSFS_readULE8(fp, &y0) || !PHYSFS_readULE8(fp, &x1) || !PHYSFS_readULE8(fp, &y1))
		{
			debug(LOG_ERROR, "%s: Failed to read gateway info", filename);
			return NULL;
		}
	}
	
	return map;
}

int main(int argc, char **argv)
{
	char filename[PATH_MAX];
	char path[PATH_MAX], *delim;
        PHYSFS_Version compiled;
        PHYSFS_Version linked;
	GAMEMAP *map;

	if (argc != 2)
	{
		printf("Usage: %s <map>\n", argv[0]);
	}
	strcpy(path, argv[1]);
	delim = strrchr(path, '/');
	if (delim)
	{
		*delim = '\0';
		delim++;
		strcpy(filename, delim);
	}
	else
	{
		path[1] = '.';
		path[1] = '\0';
		strcpy(filename, argv[1]);
	}
	PHYSFS_init(argv[0]);
        PHYSFS_VERSION(&compiled);
        PHYSFS_getLinkedVersion(&linked);
	PHYSFS_addToSearchPath(path, 1);

	map = mapLoad(filename);
	if (map)
	{
		printf("Loaded map: %s\n", filename);
		printf("\tMap version: %d\n", (int)map->version);
		printf("\tWidth: %d\n", (int)map->width);
		printf("\tHeight: %d\n", (int)map->height);
		printf("\tGateways: %d\n", (int)map->numGateways);
	}

	return 0;
}
