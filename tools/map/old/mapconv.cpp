// Converter from old Warzone (savegame) map format to new format.

#include "maplib.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

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

#define DEG(degrees) ((degrees) * 8192 / 45)

static const char *tilesetTextures[] = { "Arizona", "Urban", "Rockies" };

#define MADD(...) do { fprintf(fp, __VA_ARGS__); fprintf(fp, "\n"); } while(0)

int main(int argc, char **argv)
{
	char filename[PATH_MAX], *p_filename, *base, *mapname;
	GAMEMAP *map;
	FILE *fp;
	int i, argn = 1;
	bool campaign = false;

	if (argc < 2)
	{
		printf("Usage: %s <map>\n", argv[0]);
		return -1;
	}
	if (argc == 3 && strcmp(argv[1], "-cam") == 0)
	{
		campaign = true;
		argn++;
	}
	
	physfs_init(argv[0]);
	strcpy(filename, physfs_addmappath(argv[argn]));

	map = mapLoad(filename);
	if (!map)
	{
		fprintf(stderr, "Failed to load map %s\n", filename);
		return -1;
	}
	
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
	physfs_shutdown();

	p_filename = strstr(base, "c-");
	if (p_filename)
	{
		mapname = strdup(p_filename + 2);
	}
	else
	{
		mapname = strdup(base);
	}

	/*** Map configuration ***/
#if 0
	if (map->mapVersion > 0)
	{
		strcat(filename, "/map.ini");
		fp = fopen(filename, "w");
		if (!fp)
		{
			fprintf(stderr, "Could not open target: %s", filename);
			return -1;
		}
		MADD("[map]");
		if (map->levelName[0] != '\0')
		{
			MADD("Name = %s", map->levelName);
		}
		MADD("SnapMode = %d", SNAP_MODE);
		MADD("Gravity = %d", GRAVITY);
		MADD("HeightScale = %d", ELEVATION_SCALE);
		MADD("MapWidth = %d", map->width);
		MADD("MapHeight = %d", map->height);
		MADD("TileWidth = %d", TILE_HEIGHT);
		MADD("TileHeight = %d", TILE_WIDTH);
		MADD("SeaLevel = %d", SEALEVEL);
		MADD("Tileset = %s", tilesetTextures[map->tileset]);

		fclose(fp);
	}
#endif

	/*** Game data ***/
	strcpy(filename, base);
	strcat(filename, "/game.ini");
	fp = fopen(filename, "w");
	if (!fp)
	{
		fprintf(stderr, "Could not open target: %s", filename);
		return -1;
	}
	MADD("[game]");
	MADD("players = %d", map->numPlayers);
	if (map->gameTime > 0) MADD("GameTime = %u", map->gameTime);
	switch (map->gameType)
	{
	case 0: MADD("GameType = Start"); break;
	case 1: MADD("GameType = Expand"); break;
	case 2: MADD("GameType = Mission"); break;
	case 3: MADD("GameType = Autosave"); break;
	case 4: MADD("GameType = Savegame"); break;
	default: fprintf(stderr, "%s: Bad gametype %d", filename, map->gameType); break;
	}
	MADD("Tileset = %s", tilesetTextures[map->tileset]);
	if (map->levelName[0] == '\0')
	{
		MADD("LevelName = %s", mapname);
		MADD("Description = %s", mapname);
	}
	else
	{
		MADD("LevelName = %s", map->levelName);
		MADD("Description = %s", map->levelName);
	}
	if (map->scrollMinX + map->scrollMinY + map->scrollMaxX + map->scrollMaxY > 0)
	{
		MADD("\n[scroll_limits]");
		MADD("x1 = %d", map->scrollMinX);
		MADD("y1 = %d", map->scrollMinY);
		MADD("x2 = %u", map->scrollMaxX);
		MADD("y2 = %u", map->scrollMaxY);
	}
	for (i = 0; i < 8; i++)
	{
		if (map->power[i] > 0)
		{
			MADD("\n[player_%d]", i);
			MADD("power = %d", map->power[i]);
		}
	}
	fclose(fp);

#if 0
	/*** Terrain data ***/
	if (map->mapVersion > 0)
	{
		uint16_t *terrain, *rotate;
		uint8_t *height, *flip;
		MAPTILE *psTile = mapTile(map, 0, 0);

		terrain = malloc(map->width * map->height * 2);
		height = malloc(map->width * map->height);
		rotate = malloc(map->width * map->height * 2);
		flip = malloc(map->width * map->height);
		for (i = 0; i < map->width * map->height; i++)
		{
			height[i] = psTile->height;
			terrain[i] = psTile->texture & TILE_NUMMASK;
			rotate[i] = ((psTile->texture & TILE_ROTMASK) >> TILE_ROTSHIFT) * 90;
			flip[i] = TRI_FLIPPED(psTile) ? 255 : 0;

			psTile++;
		}
		strcpy(filename, base);
		strcat(filename, "/map-001/terrain.png");
		savePngI16(filename, terrain, map->width, map->height);
		strcpy(filename, base);
		strcat(filename, "/map-001/height.png");
		savePngI8(filename, height, map->width, map->height);
		strcpy(filename, base);
		strcat(filename, "/map-001/rotations.png");
		savePngI16(filename, rotate, map->width, map->height);
		strcpy(filename, base);
		strcat(filename, "/map-001/flips.png");
		savePngI8(filename, flip, map->width, map->height);
		free(height);
		free(terrain);
		free(rotate);
		free(flip);
	}

#endif
	/*** Features ***/
	if (map->featVersion > 0)
	{
		strcpy(filename, base);
		strcat(filename, "/feature.ini");
		printf("writing %s\n", filename);
		fp = fopen(filename, "w");
		if (!fp) printf("%s: %s\n", filename, strerror(errno));
		for (i = 0; i < map->numFeatures; i++)
		{
			LND_OBJECT *psObj = &map->mLndObjects[IMD_FEATURE][i];

			if (psObj->id == 0) psObj->id = 0xFEDBCA98; // fix broken ID
			MADD("\n[feature_%04u]", psObj->id);
			MADD("id = %u", psObj->id);
			MADD("position = %u, %u, %u", psObj->x, psObj->y, psObj->z);
			MADD("rotation = %u, 0, 0", DEG(psObj->direction));
			MADD("name = %s", psObj->name);
		}
		fclose(fp);
	}

	/*** Structures ***/
	if (map->structVersion)
	{
		strcpy(filename, base);
		strcat(filename, "/struct.ini");
		printf("writing %s\n", filename);
		fp = fopen(filename, "w");
		if (!fp) printf("%s: %s\n", filename, strerror(errno));
		for (i = 0; i < map->numStructures; i++)
		{
			LND_OBJECT *psMod, *psObj = &map->mLndObjects[IMD_STRUCTURE][i];
			int j, capacity = 0;

			if (strcmp(psObj->name, "A0PowMod1") == 0 || strcmp(psObj->name, "A0FacMod1") == 0
				|| strcmp(psObj->name, "A0ResearchModule1") == 0)
			{
				continue; // do not write modules as separate entries
			}

			if (psObj->id == 0) psObj->id = 0xFEDBCA98; // fix broken ID
			MADD("\n[structure_%04u]", psObj->id);
			MADD("id = %u", psObj->id);
			MADD("position = %u, %u, %u", psObj->x, psObj->y, psObj->z);
			MADD("rotation = %u, 0, 0", DEG(psObj->direction));
			MADD("name = %s", psObj->name);
			if (campaign)
			{
				MADD("player = %u", psObj->player);
			}
			else if (psObj->player < map->numPlayers)
			{
				MADD("startpos = %u", psObj->player);
			}
			else
			{
				MADD("player = scavenger");
			}

			// Merge modules into host building entry
			if (strcmp(psObj->name, "A0LightFactory") == 0)
			{
				for (j = 0; j < map->numStructures; j++)
				{
					psMod = &map->mLndObjects[IMD_STRUCTURE][j];
					if (strcmp(psMod->name, "A0FacMod1") == 0 && psObj->x == psMod->x && psObj->y == psMod->y)
					{
						capacity++;
					}
				}
			}
			else if (strcmp(psObj->name, "A0PowerGenerator") == 0)
			{
				for (j = 0; j < map->numStructures; j++)
				{
					psMod = &map->mLndObjects[IMD_STRUCTURE][j];
					if (strcmp(psMod->name, "A0PowMod1") == 0 && psObj->x == psMod->x && psObj->y == psMod->y)
					{
						capacity++;
					}
				}
			}
			else if (strcmp(psObj->name, "A0ResearchFacility") == 0)
			{
				for (j = 0; j < map->numStructures; j++)
				{
					psMod = &map->mLndObjects[IMD_STRUCTURE][j];
					if (strcmp(psMod->name, "A0ResearchModule1") == 0 && psObj->x == psMod->x && psObj->y == psMod->y)
					{
						capacity++;
					}
				}
			}
			if (capacity > 0)
			{
				MADD("modules = %d", capacity);
			}
		}
		fclose(fp);
	}

	/*** Droids ***/
	if (map->droidVersion > 0)
	{
		strcpy(filename, base);
		strcat(filename, "/droid.ini");
		printf("writing %s\n", filename);
		fp = fopen(filename, "w");
		if (!fp) printf("%s: %s\n", filename, strerror(errno));
		for (i = 0; i < map->numDroids; i++)
		{
			LND_OBJECT *psObj = &map->mLndObjects[IMD_DROID][i];

			if (psObj->id == 0) psObj->id = 0xFEDBCA98; // fix broken ID
			MADD("\n[droid_%04u]", psObj->id);
			MADD("id = %u", psObj->id);
			MADD("position = %u, %u, %u", psObj->x, psObj->y, psObj->z);
			MADD("rotation = %u, 0, 0", DEG(psObj->direction));
			if (campaign)
			{
				MADD("player = %u", psObj->player);
			}
			else if (psObj->player < map->numPlayers)
			{
				MADD("startpos = %u", psObj->player);
			}
			else
			{
				MADD("player = scavenger");
			}
			MADD("template = %s", psObj->name);
		}
		fclose(fp);
	}

#if 0
	/*** Gateways ***/
	if (map->mapVersion > 0)
	{
		strcpy(filename, base);
		strcat(filename, "/map-001/gateways.ini");
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
	}
#endif

	mapFree(map);

	return 0;
}
