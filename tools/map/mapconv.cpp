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

static const char *tilesetTextures[] = { "arizona", "urban", "rockies" };

#define MADD(...) do { fprintf(fp, __VA_ARGS__); fprintf(fp, "\n"); } while(0)

static const char *conversionTable[3][255];   // convert old terrain numbers to decal labels
static int terrainTable[3][255];              // convert old terrain numbers of new ones

#define T_WATER               0
#define T_CLIFF               1
#define T_CLIFF_VARIANT       2
#define T_LOW                 3
#define T_LOW_VARIANT         4              
#define T_STANDARD            5
#define T_STANDARD_VARIANT    6
#define T_TILES               7
#define T_TILES_VARIANT       8
#define T_HIGH                9
#define T_HIGH_VARIANT       10
#define T_WET                11
#define T_TOUGH              12 // also used as underwater tile?
#define T_SPECIAL1           13
#define T_SPECIAL2           14
#define T_REARM              15

#define INVALID 255

static void validateTables(void)
{
	int i;
	for (i = 0; i < 78; i++) if (terrainTable[0][i] == INVALID) { printf("invalid arizona %d\n", i); abort(); }
	for (i = 0; i < 80; i++) if (terrainTable[1][i] == INVALID) { printf("invalid urban %d\n", i); abort(); }
	for (i = 0; i < 79; i++) if (terrainTable[2][i] == INVALID) { printf("invalid rockies %d\n", i); abort(); }
}

static void fillConversionTables(void)
{
	int i;
	for (i = 0; i < 255; i++) terrainTable[0][i] = INVALID;
	for (i = 0; i < 255; i++) terrainTable[1][i] = INVALID;
	for (i = 0; i < 255; i++) terrainTable[2][i] = INVALID;

	// arizona

	terrainTable[0][0] = T_LOW_VARIANT;	// yellow sand
	terrainTable[0][1] = T_LOW;
	terrainTable[0][2] = T_LOW;
	terrainTable[0][4] = T_LOW;
	terrainTable[0][9] = T_LOW_VARIANT;
	terrainTable[0][10] = T_LOW;
	terrainTable[0][11] = T_LOW_VARIANT;
	terrainTable[0][12] = T_LOW;
	terrainTable[0][13] = T_LOW;
	terrainTable[0][14] = T_LOW;
	terrainTable[0][15] = T_LOW;
	terrainTable[0][16] = T_LOW;
	terrainTable[0][27] = T_LOW;
	terrainTable[0][28] = T_LOW;
	terrainTable[0][40] = T_LOW;
	terrainTable[0][42] = T_LOW;
	terrainTable[0][43] = T_LOW;
	terrainTable[0][55] = T_LOW;
	terrainTable[0][63] = T_LOW;
	terrainTable[0][64] = T_LOW;
	terrainTable[0][65] = T_LOW;
	terrainTable[0][66] = T_LOW;

	terrainTable[0][20] = T_STANDARD;	// red sand
	terrainTable[0][29] = T_STANDARD_VARIANT;
	terrainTable[0][34] = T_STANDARD;
	terrainTable[0][35] = T_STANDARD_VARIANT;
	terrainTable[0][37] = T_STANDARD;
	terrainTable[0][41] = T_STANDARD;
	terrainTable[0][44] = T_STANDARD;
	terrainTable[0][47] = T_STANDARD;
	terrainTable[0][48] = T_STANDARD;
	terrainTable[0][49] = T_STANDARD;
	terrainTable[0][50] = T_STANDARD;
	terrainTable[0][51] = T_STANDARD;
	terrainTable[0][52] = T_STANDARD;
	terrainTable[0][53] = T_STANDARD_VARIANT;
	terrainTable[0][54] = T_STANDARD_VARIANT;
	terrainTable[0][56] = T_STANDARD;
	terrainTable[0][57] = T_STANDARD;
	terrainTable[0][58] = T_STANDARD;
	terrainTable[0][59] = T_STANDARD;
	terrainTable[0][60] = T_STANDARD;
	terrainTable[0][61] = T_STANDARD;
	terrainTable[0][67] = T_STANDARD;
	terrainTable[0][68] = T_STANDARD;
	terrainTable[0][69] = T_STANDARD;
	terrainTable[0][70] = T_STANDARD;
	terrainTable[0][72] = T_STANDARD;
	terrainTable[0][73] = T_STANDARD;
	terrainTable[0][76] = T_STANDARD;

	terrainTable[0][74] = T_SPECIAL1;	// another red sand variant

	terrainTable[0][3] = T_TOUGH;	// mud
	terrainTable[0][5] = T_TOUGH;
	terrainTable[0][6] = T_TOUGH;
	terrainTable[0][7] = T_TOUGH;
	terrainTable[0][8] = T_TOUGH;
	terrainTable[0][26] = T_TOUGH;
	terrainTable[0][36] = T_TOUGH;
	terrainTable[0][38] = T_TOUGH;
	terrainTable[0][39] = T_TOUGH;
	terrainTable[0][62] = T_TOUGH;

	terrainTable[0][17] = T_WATER;

	terrainTable[0][18] = T_CLIFF;
	terrainTable[0][45] = T_CLIFF; // ?
	terrainTable[0][46] = T_CLIFF; // ?
	terrainTable[0][71] = T_CLIFF;
	terrainTable[0][75] = T_CLIFF; // ?

	terrainTable[0][19] = T_TILES;
	terrainTable[0][21] = T_TILES;
	terrainTable[0][22] = T_TILES;
	terrainTable[0][77] = T_TILES;

	terrainTable[0][23] = T_WET;	// green grass
	terrainTable[0][24] = T_WET;
	terrainTable[0][25] = T_WET;
	terrainTable[0][30] = T_WET;
	terrainTable[0][31] = T_WET;
	terrainTable[0][32] = T_WET;
	terrainTable[0][33] = T_WET;

	// urban

	terrainTable[1][0] = T_LOW;	// urban burned
	terrainTable[1][1] = T_LOW_VARIANT;
	terrainTable[1][2] = T_LOW;
	terrainTable[1][3] = T_LOW;
	terrainTable[1][32] = T_LOW;
	terrainTable[1][35] = T_LOW;
	terrainTable[1][41] = T_LOW;
	terrainTable[1][57] = T_LOW;
	terrainTable[1][63] = T_LOW;
	terrainTable[1][64] = T_LOW;
	terrainTable[1][65] = T_LOW;
	terrainTable[1][66] = T_LOW;

	terrainTable[1][17] = T_WATER;

	terrainTable[1][11] = T_SPECIAL1; // dark tiles
	terrainTable[1][12] = T_SPECIAL1;
	terrainTable[1][18] = T_SPECIAL1;
	terrainTable[1][40] = T_SPECIAL1; // road tiles
	terrainTable[1][42] = T_SPECIAL1;
	terrainTable[1][46] = T_SPECIAL1;
	terrainTable[1][47] = T_SPECIAL1;
	terrainTable[1][49] = T_SPECIAL1;
	terrainTable[1][53] = T_SPECIAL1;
	terrainTable[1][62] = T_SPECIAL1;

	terrainTable[1][4] = T_STANDARD;
	terrainTable[1][5] = T_STANDARD_VARIANT;
	terrainTable[1][6] = T_STANDARD;
	terrainTable[1][7] = T_STANDARD;
	terrainTable[1][8] = T_STANDARD_VARIANT;
	terrainTable[1][9] = T_STANDARD;
	terrainTable[1][10] = T_STANDARD;
	terrainTable[1][23] = T_STANDARD;
	terrainTable[1][24] = T_STANDARD;
	terrainTable[1][25] = T_STANDARD;
	terrainTable[1][26] = T_STANDARD;
	terrainTable[1][29] = T_STANDARD;
	terrainTable[1][43] = T_STANDARD;
	terrainTable[1][44] = T_STANDARD;
	terrainTable[1][54] = T_STANDARD;
	terrainTable[1][55] = T_STANDARD;
	terrainTable[1][58] = T_STANDARD;
	terrainTable[1][67] = T_STANDARD;
	terrainTable[1][68] = T_STANDARD;
	terrainTable[1][72] = T_STANDARD;
	terrainTable[1][78] = T_STANDARD;
	terrainTable[1][80] = T_STANDARD;

	terrainTable[1][22] = T_HIGH;
	terrainTable[1][31] = T_HIGH_VARIANT;
	terrainTable[1][33] = T_HIGH;
	terrainTable[1][34] = T_HIGH;
	terrainTable[1][36] = T_HIGH;
	terrainTable[1][38] = T_HIGH;
	terrainTable[1][60] = T_HIGH;
	terrainTable[1][73] = T_HIGH;
	terrainTable[1][76] = T_HIGH;

	terrainTable[1][37] = T_WET;
	terrainTable[1][39] = T_WET;
	terrainTable[1][45] = T_WET;
	terrainTable[1][50] = T_WET;
	terrainTable[1][77] = T_WET;
	terrainTable[1][79] = T_WET;

	terrainTable[1][13] = T_TILES;
	terrainTable[1][14] = T_TILES;
	terrainTable[1][15] = T_TILES;
	terrainTable[1][16] = T_TILES;
	terrainTable[1][19] = T_TILES_VARIANT;
	terrainTable[1][20] = T_TILES;
	terrainTable[1][21] = T_TILES;
	terrainTable[1][27] = T_TILES;
	terrainTable[1][28] = T_TILES;
	terrainTable[1][30] = T_TILES;
	terrainTable[1][48] = T_TILES;
	terrainTable[1][51] = T_TILES;
	terrainTable[1][52] = T_TILES;
	terrainTable[1][56] = T_TILES;
	terrainTable[1][59] = T_TILES;
	terrainTable[1][61] = T_TILES;
	terrainTable[1][71] = T_TILES;
	terrainTable[1][74] = T_TILES;
	terrainTable[1][75] = T_TILES;

	terrainTable[1][69] = T_CLIFF;
	terrainTable[1][70] = T_CLIFF;

	// rockies

	terrainTable[2][0] = T_LOW;
	terrainTable[2][1] = T_LOW;
	terrainTable[2][2] = T_LOW;
	terrainTable[2][3] = T_LOW;
	terrainTable[2][4] = T_STANDARD;
	terrainTable[2][5] = T_STANDARD;
	terrainTable[2][6] = T_STANDARD;
	terrainTable[2][7] = T_STANDARD;
	terrainTable[2][8] = T_STANDARD;
	terrainTable[2][9] = T_CLIFF;
	terrainTable[2][10] = T_STANDARD;
	terrainTable[2][11] = T_STANDARD;
	terrainTable[2][12] = T_STANDARD;
	terrainTable[2][13] = T_WET;
	terrainTable[2][14] = T_LOW;
	terrainTable[2][15] = T_LOW;
	terrainTable[2][16] = T_LOW;
	terrainTable[2][17] = T_WATER;
	terrainTable[2][18] = T_CLIFF;
	terrainTable[2][19] = T_TILES;
	terrainTable[2][20] = T_TOUGH;
	terrainTable[2][21] = T_TILES;
	terrainTable[2][22] = T_TILES;
	terrainTable[2][23] = T_LOW_VARIANT;
	terrainTable[2][24] = T_LOW_VARIANT;
	terrainTable[2][25] = T_LOW;
	terrainTable[2][26] = T_LOW;
	terrainTable[2][27] = T_TOUGH;
	terrainTable[2][28] = T_TOUGH;
	terrainTable[2][29] = T_CLIFF;
	terrainTable[2][30] = T_CLIFF;
	terrainTable[2][31] = T_STANDARD;
	terrainTable[2][32] = T_STANDARD;
	terrainTable[2][33] = T_STANDARD;
	terrainTable[2][34] = T_LOW;
	terrainTable[2][35] = T_LOW;
	terrainTable[2][36] = T_TOUGH;
	terrainTable[2][37] = T_TOUGH;
	terrainTable[2][38] = T_TOUGH;
	terrainTable[2][39] = T_TOUGH;
	terrainTable[2][40] = T_STANDARD;
	terrainTable[2][41] = T_STANDARD;
	terrainTable[2][42] = T_CLIFF;
	terrainTable[2][43] = T_STANDARD;
	terrainTable[2][44] = T_CLIFF;
	terrainTable[2][45] = T_CLIFF;
	terrainTable[2][46] = T_CLIFF;
	terrainTable[2][47] = T_LOW_VARIANT;
	terrainTable[2][48] = T_STANDARD;
	terrainTable[2][49] = T_TOUGH;
	terrainTable[2][50] = T_TOUGH;
	terrainTable[2][51] = T_TOUGH;
	terrainTable[2][52] = T_TOUGH;
	terrainTable[2][53] = T_TOUGH;
	terrainTable[2][54] = T_HIGH;
	terrainTable[2][55] = T_STANDARD;
	terrainTable[2][56] = T_TOUGH;
	terrainTable[2][57] = T_STANDARD_VARIANT;
	terrainTable[2][58] = T_LOW;
	terrainTable[2][59] = T_TOUGH;
	terrainTable[2][60] = T_TOUGH;
	terrainTable[2][61] = T_CLIFF;
	terrainTable[2][62] = T_STANDARD;
	terrainTable[2][63] = T_CLIFF;
	terrainTable[2][64] = T_HIGH;
	terrainTable[2][65] = T_HIGH;
	terrainTable[2][66] = T_HIGH;
	terrainTable[2][67] = T_HIGH;
	terrainTable[2][68] = T_CLIFF;
	terrainTable[2][69] = T_CLIFF_VARIANT;
	terrainTable[2][70] = T_HIGH;
	terrainTable[2][71] = T_CLIFF;
	terrainTable[2][72] = T_TOUGH;
	terrainTable[2][73] = T_TOUGH; // ?
	terrainTable[2][74] = T_LOW_VARIANT;
	terrainTable[2][75] = T_TOUGH; // ?
	terrainTable[2][76] = T_CLIFF;
	terrainTable[2][77] = T_CLIFF_VARIANT;
	terrainTable[2][78] = T_CLIFF;

	// decals

	memset(conversionTable, 0, sizeof(conversionTable));
	// TODO: 0, 9, 11, 28 should be 'dry_grass', but we need to make it first
	conversionTable[0][37] = "road_damage";
	conversionTable[0][47] = "road_end";
	conversionTable[0][49] = "tracks_faint";
	conversionTable[0][50] = "tracks_turn";
	conversionTable[0][51] = "tracks";
	conversionTable[0][52] = "tracks_end";
	conversionTable[0][55] = "crater";
	conversionTable[0][56] = "crater2";
	conversionTable[0][57] = "road_junction";
	conversionTable[0][58] = "crater3";
	conversionTable[0][59] = "road";
	conversionTable[0][62] = "crater4";
	conversionTable[0][63] = "crater_slice_1";
	conversionTable[0][64] = "crater_slice_2";
	conversionTable[0][65] = "crater_slice_3";
	conversionTable[0][66] = "crater_slice_4";
	conversionTable[0][67] = "crater_slice_5";
	conversionTable[0][68] = "crater_slice_6";
	conversionTable[0][69] = "crater_slice_7";
	conversionTable[0][72] = "tracks_junction";
	conversionTable[0][73] = "tracks_cross";
	//conversionTable[0][73] = "rubble"; TODO
	conversionTable[1][1] = "rocks";
	//conversionTable[1][5] = "rubble_piece"; TODO
	conversionTable[1][21] = "rocks2";
	//conversionTable[1][28] = "sewer_hole"; TODO
	conversionTable[1][36] = "grass";
	conversionTable[1][40] = "wide_road";
	conversionTable[1][41] = "wide_road_end";
	conversionTable[1][42] = "wide_road";
	conversionTable[1][43] = "wide_road_end";
	conversionTable[1][44] = "wide_road_end";
	conversionTable[1][45] = "wide_road_end";
	conversionTable[1][46] = "wide_road_damage";
	conversionTable[1][47] = "wide_road_rubble";
	conversionTable[1][52] = "rocks3";
	conversionTable[1][55] = "dark_crater";
	conversionTable[1][56] = "dark_crater2";
	conversionTable[1][57] = "dark_crater3";
	conversionTable[2][1] = "rocks3";
	conversionTable[2][8] = "snow_tiny";
	conversionTable[2][13] = "road_junction";
	conversionTable[2][27] = "rocks4";
	conversionTable[2][28] = "rocks5";
	conversionTable[2][37] = "road_damage";
	conversionTable[2][43] = "snow_tiny2";
	conversionTable[2][47] = "snow_tiny3";
	conversionTable[2][49] = "tracks_faint";
	conversionTable[2][50] = "tracks_turn";
	conversionTable[2][51] = "tracks";
	conversionTable[2][52] = "tracks_end";
	conversionTable[2][56] = "crater5";
	conversionTable[2][57] = "splatter";
	conversionTable[2][58] = "crater6";
	conversionTable[2][59] = "road";
	conversionTable[2][60] = "road_end";
	conversionTable[2][62] = "crater7";
	conversionTable[2][70] = "snow_tiny4";
	conversionTable[2][72] = "tracks_junction";
	conversionTable[2][73] = "snow_massive";
	conversionTable[2][74] = "snow_light";
	conversionTable[2][75] = "snow";
}

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
	
	fillConversionTables();
	validateTables();
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

	/*** Terrain data ***/
	if (map->mapVersion > 0)
	{
		MAPTILE *psTile = mapTile(map, 0, 0);
		uint8_t *terrain = new uint8_t[map->width * map->height];
		uint8_t *height = new uint8_t[map->width * 2 * map->height * 2];
		uint16_t rotate;
		uint8_t flip;

		memset(height, 0, map->width * 2 * map->height * 2);
		strcpy(filename, base);
		strcat(filename, "/decals.ini");
		printf("writing %s\n", filename);
		fp = fopen(filename, "w");
		if (!fp) printf("%s: %s\n", filename, strerror(errno));
		for (i = 0; i < map->width * map->height; i++)
		{
			const char *decal = conversionTable[map->tileset][psTile->texture & TILE_NUMMASK];
			const int x = i % map->width;
			const int y = i / map->width;

			height[y * 2 * map->width * 2 + x * 2 + 0] = psTile->height;
			height[y * 2 * map->width * 2 + x * 2 + 1] = psTile->height;
			height[(y * 2 + 1) * map->width * 2 + x * 2 + 0] = psTile->height;
			height[(y * 2 + 1) * map->width * 2 + x * 2 + 1] = psTile->height;

			terrain[i] = terrainTable[map->tileset][psTile->texture & TILE_NUMMASK];
			rotate = ((psTile->texture & TILE_ROTMASK) >> TILE_ROTSHIFT) * 90;
			flip = TRI_FLIPPED(psTile) ? 255 : 0;

			psTile++;

			if (!decal) continue;

			// Write decal info
			MADD("[decal_%d]", i);
			MADD("decal = %s", decal);
			MADD("x = %u", i % map->width);
			MADD("y = %u", i / map->height);
			MADD("flip = %u", (unsigned)flip);
			MADD("rotate = %u\n", (unsigned)rotate);
		}
		fclose(fp);

		strcpy(filename, base);
		strcat(filename, "/height.png");
		savePngI8(filename, height, map->width * 2, map->height * 2);

		strcpy(filename, base);
		strcat(filename, "/terrain.png");
		savePngI8(filename, terrain, map->width, map->height);

		delete [] height;
		delete [] terrain;
	}

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
