#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

// Physfs
#include <physfs.h>

// framework
#include "physfs_ext.h"
#include "wzglobal.h"

#include "maplib.h"
#include "pngsave.h"
#include "mapload.h"

int main(int argc, char **argv)
{
	char filename[PATH_MAX] = { '\0' };
    char pngfile[PATH_MAX] = { '\0' };
    char *p_filename = filename;
	char *delim;
    int namelength;
	GAMEMAP *map;
    
	if (argc != 2)
	{
		printf("Usage: %s <map>\n", argv[0]);
		return -1;
	}

    PHYSFS_init(argv[0]);
    PHYSFS_addToSearchPath(".", 1);
	
    strcat(p_filename, "multiplay/maps/");
    p_filename += strlen("multiplay/maps/");	

	delim = strrchr(argv[1], PHYSFS_getDirSeparator()[0]);
	if (delim)
	{
        delim++;
        strcpy(p_filename, delim);
        
        namelength = strlen(filename);
        if (namelength >= 3 && strcmp(&filename[namelength-3], ".wz")==0)
        {
            // remove ".wz" from end
            filename[namelength-3] = '\0';
        }
        strcpy(pngfile, delim);
        strcat(pngfile, ".png");
        PHYSFS_addToSearchPath(argv[0], 1);
	}
	else
	{
        strcpy(pngfile, argv[1]);
        namelength = strlen(pngfile);
        if (namelength >= 3 && strcmp(&pngfile[namelength-3], ".wz")==0)
        {
            // remove ".wz" from end
            pngfile[namelength-3] = '\0';
        }
        
        strcpy(p_filename, pngfile);
        strcat(pngfile, ".png");
	}
       
	map = mapLoad(filename);
	if (map)
	{
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
		savePngI8(pngfile, pixels, map->width * 4, map->height * 4);
		free(pixels);
	}
	mapFree(map);

	return 0;
}
