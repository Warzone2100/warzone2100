#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "map/mapload.h"

int main(int argc, char **argv)
{
	char datapath[PATH_MAX];
	FILE *fp = fopen("maplist.txt", "r");

	if (!fp)
	{
		fprintf(stderr, "%s: Failed to open list file\n", argv[0]);
		return -1;
	}
	PHYSFS_init(argv[0]);
	strcpy(datapath, getenv("srcdir"));
	strcat(datapath, "/../data");
	PHYSFS_addToSearchPath(datapath, 1);

	while (!feof(fp))
	{
		GAMEMAP *map;
		char filename[PATH_MAX], *delim;

		if (fscanf(fp, "%254s\n", filename) != 1)
		{
			fprintf(stderr, "maptest: Couldn't fscanf maplist.txt.\n");
			return -1;
		}

		// Strip "/game.map"
		delim = strrchr(filename, '/');
		if (!delim)
		{
			fprintf(stderr, "maptest: Failed to find map directory for \"%s\"\n", filename);
			return -1;
		}
		*delim = '\0';

		printf("Testing map: %s\n", filename);
		map = mapLoad(filename);
		if (!map)
		{
			fprintf(stderr, "maptest: Failed to load \"%s\"\n", filename);
			return -1;
		}
		mapFree(map);
	}
	fclose(fp);

	return 0;
}
