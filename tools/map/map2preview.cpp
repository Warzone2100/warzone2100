// tool "framework"
#include "maplib.h"

#include "mapload.h"
#include "pngsave.h"

#include "vector.h"

struct PreviewColors
{
	Vector3i  cliffLow;
	Vector3i  cliffHigh;
	Vector3i  water;
	Vector3i  roadLow;
	Vector3i  roadHigh;
	Vector3i  groundLow;
	Vector3i  groundHigh;
};

// C1 - Arizona type
static const PreviewColors pArizonaColors = {
	Vector3i (0x68, 0x3C, 0x24),
	Vector3i (0xE8, 0x84, 0x5C),
	Vector3i (0x3F, 0x68, 0x9A),
	Vector3i (0x24, 0x1F, 0x16),
	Vector3i (0xB2, 0x9A, 0x66),
	Vector3i (0x24, 0x1F, 0x16),
	Vector3i (0xCC, 0xB2, 0x80)};

// C2 - Urban type
static const PreviewColors pUrbanColors = {
	Vector3i (0x3C, 0x3C, 0x3C),
	Vector3i (0x84, 0x84, 0x84),
	Vector3i (0x3F, 0x68, 0x9A),
	Vector3i (0x00, 0x00, 0x00),
	Vector3i (0x24, 0x1F, 0x16),
	Vector3i (0x1F, 0x1F, 0x1F),
	Vector3i (0xB2, 0xB2, 0xB2)};

// C3 - Rockies type
static const PreviewColors pRockiesColors = {
	Vector3i (0x3C, 0x3C, 0x3C),
	Vector3i (0xFF, 0xFF, 0xFF),
	Vector3i (0x3F, 0x68, 0x9A),
	Vector3i (0x24, 0x1F, 0x16),
	Vector3i (0x3D, 0x21, 0x0A),
	Vector3i (0x00, 0x1C, 0x0E),
	Vector3i (0xFF, 0xFF, 0xFF)};

Vector3i clanColours[]=
{   // see frontend2.png for team color order.
	// [r,g,b]
	Vector3i (0,255,0),			// green  Player 0
	Vector3i (255,210,40),		// orange Player 1
	Vector3i (255,255,255),		// grey   Player 2
	Vector3i (0,0,0),			// black  Player 3
	Vector3i (255,0,0),			// red	Player 4
	Vector3i (20,20,255),		// blue   Player 5
	Vector3i (255,0,255),		// pink   Player 6 (called purple in palette.txt)
	Vector3i (0,255,255),		// cyan   Player 7
	Vector3i (255,255,0),		// yellow Player 8
	Vector3i (192,0,255),		// pink   Player 9
	Vector3i (200,255,255),		// white  Player A (Should be brighter than grey, but grey is already maximum.)
	Vector3i (128,128,255),		// bright blue Player B
	Vector3i (128,255,128),		// neon green  Player C
	Vector3i (128,0,0),			// infrared	Player D
	Vector3i (64,0,128),		// ultraviolet Player E
	Vector3i (128,128,0)		// brown	   Player F
};

static void paintStructureData(uint8_t *pixels, GAMEMAP *map)
{
	const int height  = (int) map->height;
	uint32_t x, y;
	Vector3i color;

	for (int i = 0; i < map->numStructures; i++)
	{
		LND_OBJECT *psObj = &map->mLndObjects[IMD_STRUCTURE][i];

		if (strcmp(psObj->name, "A0PowMod1") == 0 ||
			strcmp(psObj->name, "A0FacMod1") == 0 ||
			strcmp(psObj->name, "A0ResearchModule1") == 0)
		{
			continue; // ignore modules.
		}

		color = clanColours[psObj->player];
		if (strcmp(psObj->name, "A0CommandCentre") == 0)
		{
			color = Vector3i(0xFF,0,0xFF); // base/palette.txt - WZCOL_MAP_PREVIEW_HQ
		}

		x = map_coord(psObj->x);
		y = height - 1 - map_coord(psObj->y); // Origin at the top

		uint8_t * const p = &pixels[(y * map->width + x) * 3];
		p[0] = color.x;
		p[1] = color.y;
		p[2] = color.z;
	}
}

int main(int argc, char **argv)
{
	char *filename, *p_filename;
	char *base, tmpFile[PATH_MAX];
	GAMEMAP *map;
	MAPTILE *psTile;

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
	
	map = mapLoad(filename);
	free(filename);
	
	if (!map)
	{
		return EXIT_FAILURE;
	}

	const PreviewColors* tileColors = NULL;
	switch (map->tileset)
	{
	case TILESET_ARIZONA:
		tileColors = &pArizonaColors;
		break;
	case TILESET_URBAN:
		tileColors = &pUrbanColors;
		break;
	case TILESET_ROCKIES:
		tileColors = &pRockiesColors;
		break;
	default:
		fprintf(stderr, "Unknown tileset: %d\n", (int)map->tileset);
		mapFree(map);
		physfs_shutdown();
		return EXIT_FAILURE;
	}

	const int mapWidth = (int) map->width;
	const int mapHeight = (int) map->height;
	int col;

	// RGB888 pixels
	uint8_t *pixels = (uint8_t*) malloc(sizeof(uint8_t) * mapWidth * mapHeight * 3);

	for (int y = 0; y < mapHeight; y++)
	{
		for (int x = 0; x < mapWidth; x++)
		{
			// We're placing the origin at the top for aesthetic reasons
			psTile = mapTile(map, x, mapHeight-1-y);
			col = psTile->height / 2; // 2 = ELEVATION_SCALE
			uint8_t * const p = &pixels[(y * map->width + x) * 3];
			switch(terrainType(psTile))
			{
				case TER_CLIFFFACE:
					p[0] = tileColors->cliffLow.x + (tileColors->cliffHigh.x - tileColors->cliffLow.x) * col / 256;
					p[1] = tileColors->cliffLow.y + (tileColors->cliffHigh.y - tileColors->cliffLow.y) * col / 256;
					p[2] = tileColors->cliffLow.z + (tileColors->cliffHigh.z - tileColors->cliffLow.z) * col / 256;
				break;
				case TER_WATER:
					p[0] = tileColors->water.x;
					p[1] = tileColors->water.y;
					p[2] = tileColors->water.z;
				break;
				case TER_ROAD:
					p[0] = tileColors->roadLow.x + (tileColors->roadHigh.x - tileColors->roadLow.x) * col / 256;
					p[1] = tileColors->roadLow.y + (tileColors->roadHigh.y - tileColors->roadLow.y) * col / 256;
					p[2] = tileColors->roadLow.z + (tileColors->roadHigh.z - tileColors->roadLow.z) * col / 256;
				break;
				default:
					p[0] = tileColors->groundLow.x + (tileColors->groundHigh.x - tileColors->groundLow.x) * col / 256;
					p[1] = tileColors->groundLow.y + (tileColors->groundHigh.y - tileColors->groundLow.y) * col / 256;
					p[2] = tileColors->groundLow.z + (tileColors->groundHigh.z - tileColors->groundLow.z) * col / 256;
				break;
			}
		}
	}

	paintStructureData(pixels, map);

	strcpy(tmpFile, base);
	strcat(tmpFile, ".png");

	savePng(tmpFile, pixels, mapWidth, mapHeight);

	free(pixels);

	mapFree(map);

	physfs_shutdown();

	return 0;
}
