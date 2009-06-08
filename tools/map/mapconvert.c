// Converter from old Warzone (savegame) map format to new format.

// gcc -o ~/bin/mapconvert mapconvert.c mapload.c pngsave.c -I. -lphysfs -g -I../../lib/framework -lpng -Wall

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "pngsave.h"
#include "mapload.h"

#define GRDLANDVERSION	4
#define	ELEVATION_SCALE	2
#define GRAVITY		1
#define SEALEVEL	0
#define TILE_NUMMASK	0x01ff
#define TILE_XFLIP	0x8000
#define TILE_YFLIP	0x4000
#define TILE_ROTMASK	0x3000
#define TILE_ROTSHIFT	12
#define TILE_TRIFLIP	0x0800
#define TRI_FLIPPED(x)	((x)->texture & TILE_TRIFLIP)
#define SNAP_MODE	0

static const char *tilesetTextures[] = { "Arizona", "Urban", "Rockies" };

int main(int argc, char **argv)
{
	char filename[PATH_MAX], base[PATH_MAX];
	char path[PATH_MAX], *delim;
	GAMEMAP *map;
	FILE *fp;
	uint16_t *terrain;
	uint8_t *height;
	int i;
	MAPTILE *psTile;

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
		path[1] = '.';	// TODO FIXME this must be bugged...
		path[1] = '\0';
		strcpy(filename, argv[1]);
	}
	PHYSFS_init(argv[0]);
	PHYSFS_addToSearchPath(path, 1);

	map = mapLoad(filename);
	if (!map)
	{
		fprintf(stderr, "Failed to load map\n");
		return -1;
	}

	mkdir(filename, 0777);
	strcpy(base, filename);

	strcat(filename, "/map.ini");
	fp = fopen(filename, "w");
	if (!fp)
	{
		fprintf(stderr, "Could not open target: %s", filename);
		return -1;
	}
	#define MADD(...) fprintf(fp, __VA_ARGS__); fprintf(fp, "\n");
	MADD("[map]");
	MADD("SnapMode = %d", SNAP_MODE);
	MADD("Gravity = %d", GRAVITY);
	MADD("HeightScale = %d", ELEVATION_SCALE);
	MADD("MapWidth = %d", map->width);
	MADD("MapHeight = %d", map->height);
	MADD("TileWidth = %d", TILE_HEIGHT);
	MADD("TileHeight = %d", TILE_WIDTH);
	MADD("SeaLevel = %d", SEALEVEL);
	MADD("Tileset = %s", tilesetTextures[map->tileset]);
	MADD("NumTiles = %d", map->width * map->height);
	fclose(fp);

	terrain = malloc(map->width * map->height * 2);
	height = malloc(map->width * map->height);
	psTile = mapTile(map, 0, 0);
	for (i = 0; i < map->width * map->height; i++)
	{
		height[i] = psTile->height;
		terrain[i] = psTile->texture & TILE_NUMMASK;

		psTile++;
	}
	strcpy(filename, base);
	strcat(filename, "/terrain.png");
	savePngI16(filename, terrain, map->width, map->height);
	strcpy(filename, base);
	strcat(filename, "/height.png");
	savePngI8(filename, height, map->width, map->height);

#if 0
	for (i = 0, x = 0, y = 0; i < map->width * map->height; i++)
	{
		MAPTILE *psTile = mapTile(map, x, y);
		int tid = psTile->texture & TILE_NUMMASK;
		int vf = TRI_FLIPPED(psTile);
		int tf = (psTile->texture & TILE_XFLIP) > 0 ? 1 : 0;
		int f = 0;
		int vh[4];

		// Compose flag
		if (TRI_FLIPPED(psTile)) f += 2;
		if (psTile->texture & TILE_XFLIP) f += 4;
		if (psTile->texture & TILE_YFLIP) f += 8;
		switch ((psTile->texture & TILE_ROTMASK) >> TILE_ROTSHIFT)
		{
		case 0:		break;
		case 1:		f += 16; break;
		case 2:		f += 32; break;
		case 3:		f += 48; break;
		default:	fprintf(stderr, "Bad rotation value: %d\n", (psTile->texture & TILE_ROTMASK) >> TILE_ROTSHIFT); return -1;
		}

		// Vertex positions set in clockwise order
		vh[0] = psTile->height;
		vh[1] = vh[2] = vh[3] = 0;
		if (x + 1 < map->width)
		{
			vh[1] = mapTile(map, x + 1, y)->height;
		}
		if (x + 1 < map->width && y + 1 < map->height)
		{
			vh[2] = mapTile(map, x + 1, y + 1)->height;
		}
		if (y + 1 < map->height)
		{
			vh[3] = mapTile(map, x, y + 1)->height;
		}

		// No idea why +1 to TID. In EditWorld source, it is a "hide" flag.
		MADD("        TID %d VF %d TF %d F %d VH %d %d %d %d", 
		     tid + 1, vf, tf, f, vh[0], vh[1], vh[2], vh[3]);
		x++;
		if (x == map->width)
		{
			x = 0;
			y++;
		}
	}
	MADD("    }");
	MADD("}");
#endif

	strcpy(filename, base);
	strcat(filename, "/features.ini");
	fp = fopen(filename, "w");
	MADD("[feature_header]");
	MADD("entries = %u", map->numFeatures);
	for (i = 0; i < map->numDroids; i++)
	{
		LND_OBJECT *psObj = &map->mLndObjects[IMD_FEATURE][i];

		MADD("\n[droid_%04u]", i);
		MADD("pos.x = %u", psObj->x);
		MADD("pos.y = %u", psObj->y);
		MADD("pos.z = %u", psObj->z);
		MADD("direction = %u", psObj->direction);
		MADD("player = %u", psObj->player);
		MADD("template = %s", psObj->name);
	}
	fclose(fp);

	strcpy(filename, base);
	strcat(filename, "/structure.ini");
	fp = fopen(filename, "w");
	MADD("[structure_header]");
	MADD("entries = %u", map->numStructures);
	for (i = 0; i < map->numDroids; i++)
	{
		LND_OBJECT *psObj = &map->mLndObjects[IMD_STRUCTURE][i];

		MADD("\n[droid_%04u]", i);
		MADD("pos.x = %u", psObj->x);
		MADD("pos.y = %u", psObj->y);
		MADD("pos.z = %u", psObj->z);
		MADD("direction = %u", psObj->direction);
		MADD("player = %u", psObj->player);
		MADD("template = %s", psObj->name);
	}
	fclose(fp);

	strcpy(filename, base);
	strcat(filename, "/droids.ini");
	fp = fopen(filename, "w");
	MADD("[droid_header]");
	MADD("entries = %u", map->numDroids);
	for (i = 0; i < map->numDroids; i++)
	{
		LND_OBJECT *psObj = &map->mLndObjects[IMD_DROID][i];

		MADD("\n[droid_%04u]", i);
		MADD("pos.x = %u", psObj->x);
		MADD("pos.y = %u", psObj->y);
		MADD("pos.z = %u", psObj->z);
		MADD("direction = %u", psObj->direction);
		MADD("player = %u", psObj->player);
		MADD("template = %s", psObj->name);
	}
	fclose(fp);

	strcpy(filename, base);
	strcat(filename, "/gateways.ini");
	fp = fopen(filename, "w");
	MADD("[gateway_header]");
	MADD("entries = %u", map->numGateways);
	for (i = 0; i < map->numGateways; i++)
	{
		GATEWAY *psGate = mapGateway(map, i);

		MADD("\n[gateway_%04d]", i);
		MADD("x1=%hhu", psGate->x1);
		MADD("y1=%hhu",	psGate->y1);
		MADD("x2=%hhu",	psGate->x2);
		MADD("y2=%hhu",	psGate->y2);
	}

	mapFree(map);

	return 0;
}
