// gcc -o ~/bin/mapinfo mapinfo.c mapload.c -I. -lphysfs

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
	mapFree(map);

	return 0;
}
