// gcc -o ~/bin/mapinfo mapinfo.c mapload.c -I. -lphysfs -I../../lib/framework -Wall

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "mapload.h"

int main(int argc, char **argv)
{
	char filename[PATH_MAX];
	char path[PATH_MAX], *delim;
	GAMEMAP *map;

	if (argc != 2)
	{
		printf("Usage: %s <map>\n", argv[0]);
		return -1;
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
	PHYSFS_mount(path, NULL, 1);

	map = mapLoad(filename);
	if (map)
	{
		char tilesetName[PATH_MAX];

		if (map->tileset == TILESET_ARIZONA) { strcpy(tilesetName, "Arizona"); }
		else if (map->tileset == TILESET_URBAN) { strcpy(tilesetName, "Urban"); }
		else if (map->tileset == TILESET_ROCKIES) { strcpy(tilesetName, "Rockies"); }
		else { strcpy(tilesetName, "(unknown)"); }

		printf("Loaded map: %s\n", filename);
		printf("\tMap version: %d\n", (int)map->mapVersion);
		printf("\tGame version: %d\n", (int)map->gameVersion);
		printf("\tWidth: %d\n", (int)map->width);
		printf("\tHeight: %d\n", (int)map->height);
		printf("\tGateways: %d\n", (int)map->numGateways);
		printf("\tPlayers: %d\n", (int)map->numPlayers);
		printf("\tFeatures: %d\n", (int)map->numFeatures);
		printf("\tDroids: %d\n", (int)map->numDroids);
		printf("\tStructures: %d\n", (int)map->numStructures);
		printf("\tScroll limits: (%d, %d, %d, %d)\n",
			   (int)map->scrollMinX, (int)map->scrollMinY, (int)map->scrollMaxX, (int)map->scrollMaxY);
		printf("\tLevel name: %s\n", map->levelName);
		printf("\tTileset: %s\n", tilesetName);
	}
	mapFree(map);

	return 0;
}
