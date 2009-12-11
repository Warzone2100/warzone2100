// gcc -o ~/bin/map2png map2png.c mapload.c pngsave.c -I. -lphysfs -lpng -I../../lib/framework -Wall

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "pngsave.h"
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
		int x, y;
		uint8_t *pixels = malloc(map->width * map->height);

		for (x = 0; x < map->width; x++)
		{
			for (y = 0; y < map->height; y++)
			{
				MAPTILE *psTile = mapTile(map, x, y);
				int pixpos = y * map->width + x;

				pixels[pixpos++] = psTile->height;
			}
		}
		strcat(filename, ".png");
		savePngI8(filename, pixels, map->width, map->height);
		free(pixels);
	}
	mapFree(map);

	return 0;
}
