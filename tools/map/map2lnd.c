// gcc -o ~/bin/map2lnd map2lnd.c mapload.c -I. -lphysfs

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "mapload.h"

#define GRDLANDVERSION	4
#define	ELEVATION_SCALE	2
#define GRAVITY		1
#define SEALEVEL	0
#define TILE_HEIGHT	128
#define TILE_WIDTH	128
#define TILE_NUMMASK	0x01ff
#define TILE_XFLIP	0x8000
#define TILE_TRIFLIP	0x0800
#define TRI_FLIPPED(x)	((x)->texture & TILE_TRIFLIP)

static MAPTILE *mapTile(GAMEMAP *map, int x, int y)
{
	return &map->psMapTiles[x * map->width + y];
}

int main(int argc, char **argv)
{
	char filename[PATH_MAX];
	char path[PATH_MAX], *delim;
	GAMEMAP *map;
	FILE *fp;
	int i, x, y;

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

	delim = strrchr(filename, '.');
	if (!delim)
	{
		fprintf(stderr, "Invalid filename\n");
		return -1;
	}
	delim[1] = 'l';
	delim[2] = 'n';
	delim[3] = 'd';
	fp = fopen(filename, "w");
	if (!fp)
	{
		fprintf(stderr, "Could not open target: %s", filename);
		return -1;
	}
	#define MADD(...) fprintf(fp, __VA_ARGS__); fprintf(fp, "\n");
	MADD("DataSet %s", filename);
	MADD("GrdLand {");
	MADD("  Version %d", GRDLANDVERSION);
	MADD("  3DPosition %f %f %f", 0.0, 0.0, 0.0);	// FIXME
	MADD("  3DRotation %f %f %f", 0.0, 0.0, 0.0);	// FIXME
	MADD("  2DPosition %d %d", 0, 0);		// FIXME
	MADD("  CustomSnap %d %d", 0, 0);		// FIXME
	MADD("	Gravity %d", GRAVITY);
	MADD("	HeightScale %d", ELEVATION_SCALE);
	MADD("	MapWidth %d", map->width);
	MADD("	MapHeight %d", map->height);
	MADD("	TileWidth %d", TILE_HEIGHT);
	MADD("	TileHeight %d", TILE_WIDTH);
	MADD("	SeaLevel %d", SEALEVEL);
	MADD("	TextureWidth %d", 64);			// Hack for editworld
	MADD("	TextureHeight %d", 64);
	MADD("	NumTextures %d", 0);			// FIXME
	MADD("	Textures {");
	MADD("	}");
	MADD("	NumTiles %d",  map->width * map->height);
	MADD("	Tiles {");
	for (i = 0, x = 0, y = 0; i < map->width * map->height; i++)
	{
		MAPTILE *psTile = mapTile(map, x, y);

		// Example: TID 1 VF 0 TF 0 F 0 VH 128 128 128 128
		// TID is texture identification
		// VF is triangle (vertex) flip. If value of one, it is flipped.
		// TF is tile or texture flip. If value of one, it is flipped in the X direction. Otherwise Y direction.
		// F are bitflags, with these values: 
		//	 2 : Triangle flip
		//	 4 : Texture flip X (yes, duplicate info)
		//	 8 : Texture flip Y (ditto)
		//	16 : Rotate 90 degrees
		//	32 : Rotate 180 degrees (yes, a 270 degree is possible)
		// VH is vertex height, and gives height of all the four vertices that make up our tile
		int tid = psTile->texture & TILE_NUMMASK;
		int vf = TRI_FLIPPED(psTile);
		int tf = psTile->texture & TILE_XFLIP;
		int f = psTile->tileInfoBits;
		int vh[4];

		// CHECK: Should these be multiplied by ELEVATION_SCALE?
		// CHECK: I am simply assuming counter-clockwise orientation here
		vh[0] = psTile->height;
		if (y + 1 < map->height)
		{
			vh[1] = mapTile(map, x, y + 1)->height;
		}
		else
		{
			vh[1] = 0;
		}

		if (x + 1 < map->width)
		{
			vh[3] = mapTile(map, x + 1, y)->height;
		}
		else
		{
			vh[3] = 0;
		}

		if (x + 1 < map->width && y + 1 < map->height)
		{
			vh[2] = mapTile(map, x + 1, y + 1)->height;
		}
		else
		{
			vh[2] = 0;
		}

		MADD("	\tTID %d VF %d TF %d F %d VH %d %d %d %d", 
		     tid, vf, tf, f, vh[0], vh[1], vh[2], vh[3]);
		x++;
		if (x == map->width)
		{
			x = 0; y++;
		}
	}
	MADD("	}");
	MADD("}");

	mapFree(map);

	return 0;
}
