// tool "framework"
#include "maplib.h"
#include "pngsave.h"
#include "mapload.h"

int main(int argc, char **argv)
{
	char *filename, *p_filename;
	char *base, tmpFile[PATH_MAX];

	GAMEMAP *map;
	
	if (argc != 2)
	{
		printf("Usage: %s <map>\n", argv[0]);
		return -1;
	}

	physfs_init(argv[0]);
	filename = physfs_addmappath(argv[1]);
	p_filename = strrchr(filename, '/');
	if (p_filename)
	{
		p_filename++;
		base = strdup(p_filename);
	}
	else
	{
		base = strdup(filename);
	}
	if (!PHYSFS_exists(base))
	{
		PHYSFS_mkdir(base);
	}

	map = mapLoad(filename);
	free(filename);
	
	if (!map)
	{
		return EXIT_FAILURE;
	}
	
	uint x, y;
	uint8_t *pixels = (uint8_t *)malloc(map->width * map->height);

	for (x = 0; x < map->width; x++)
	{
		for (y = 0; y < map->height; y++)
		{
			MAPTILE *psTile = mapTile(map, x, y);
			int pixpos = y * map->width + x;

			pixels[pixpos++] = psTile->height;
		}
	}

	strcpy(tmpFile, base);
	strcat(tmpFile, "/height.png");
	
	savePngI8(tmpFile, pixels, map->width, map->height);
	free(pixels);

	mapFree(map);

	physfs_shutdown();

	return 0;
}
