// Converter from Warzone (savegame) map format to Editworld LND format.
// Note that we are missing some information, most notably the height for
// the rightmost and bottom most vertices, which in Warzone is snapped to
// sea level, while it can be freely manipulated in Editworld.

// gcc -o ~/bin/map2lnd map2lnd.c mapload.c -I. -lphysfs -g -I../../lib/framework -Wall

// Tool framework
#include "maplib.h"

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

static const char *tilesetDataSet[] = { "WarzoneDataC1.eds", "WarzoneDataC2.eds", "WarzoneDataC3.eds" };
static const char *tilesetTextures[] = { "texpages\\tertilesc1.pcx", "texpages\\tertilesc2.pcx", "texpages\\tertilesc3.pcx" };

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
		path[0] = '.';
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

	strcat(filename, ".lnd");
	fp = fopen(filename, "w");
	if (!fp)
	{
		fprintf(stderr, "Could not open target: %s", filename);
		return -1;
	}
	#define MADD(...) fprintf(fp, __VA_ARGS__); fprintf(fp, "\n");
	MADD("DataSet %s", tilesetDataSet[map->tileset]);
	MADD("GrdLand {");
	MADD("	Version %d", GRDLANDVERSION);
	MADD("	3DPosition %f %f %f", 0.0, 0.0, 0.0);	// FIXME
	MADD("	3DRotation %f %f %f", 0.0, 0.0, 0.0);	// FIXME
	MADD("	2DPosition %d %d", 0, 0);		// FIXME
	MADD("	CustomSnap %d %d", 0, 0);		// FIXME
	MADD("	SnapMode %d", SNAP_MODE);
	MADD("	Gravity %d", GRAVITY);
	MADD("	HeightScale %d", ELEVATION_SCALE);
	MADD("	MapWidth %d", map->width);
	MADD("	MapHeight %d", map->height);
	MADD("	TileWidth %d", TILE_HEIGHT);
	MADD("	TileHeight %d", TILE_WIDTH);
	MADD("	SeaLevel %d", SEALEVEL);
	MADD("	TextureWidth %d", 64);		// Hack for editworld
	MADD("	TextureHeight %d", 64);
	MADD("	NumTextures %d", 1);
	MADD("	Textures {");
	MADD("		%s", tilesetTextures[map->tileset]);
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
		//	64 : Gateway?
		//	128: TF_TEXTURESPARE0, whatever that was, unused even in Editworld
		// VH is vertex height, and gives height of all the four vertices that make up our tile
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
		MADD("		TID %d VF %d TF %d F %d VH %d %d %d %d", 
			 tid + 1, vf, tf, f, vh[0], vh[1], vh[2], vh[3]);
		x++;
		if (x == map->width)
		{
			x = 0;
			y++;
		}
	}
	MADD("	}");
	MADD("}");
	MADD("ObjectList {");
	MADD("	Version 3");
	MADD("	FeatureSet %s", tilesetDataSet[map->tileset]);
	MADD("	NumObjects %u", map->numFeatures + map->numStructures + map->numDroids);
	MADD("	Objects {");
	for (x = IMD_FEATURE; x < IMD_OBJECT; x++)
	{
		int max = 0;

		switch (x)
		{
		case IMD_FEATURE: max = map->numFeatures; break;
		case IMD_STRUCTURE: max = map->numStructures; break;
		case IMD_DROID: max = map->numDroids; break;
		default: break;
		}

		for (i = 0; i < max; i++)
		{
			LND_OBJECT *psObj = &map->mLndObjects[x][i];
			double x = (double)psObj->x - (double)map->width * TILE_WIDTH / 2.0;
			double y = ((double)psObj->y - (double)map->height * TILE_HEIGHT / 2.0) * -1.0;

			MADD("		%u %d \"%s\" %u \"NONAME\" %.02f %.02f %.02f 0.00 %.02f 0.00",
				 psObj->id, psObj->type, psObj->name, psObj->player, x, (double)psObj->z, y, (double)psObj->direction);

			// %d UniqueID, %d TypeID, \"%s\" StructureName or description or \"NONAME\", %d PlayerID, \"%s\" ScriptName or \"NONAME\" MAY BE MISSING in v<3!
			// fprintf(Stream, "%.2f %.2f %.2f ", Position.x, Position.y, Position.z);
			// fprintf(Stream, "%.2f %.2f %.2f\n", curNode->Rotation.x, curNode->Rotation.y, curNode->Rotation.z);
		}
	}
	MADD("	}");
	MADD("}");
	MADD("ScrollLimits {");
	MADD("	Version 1");
	MADD("	NumLimits 1");		// FIXME: do scroll limits go here?
	MADD("	Limits {");
	MADD("		\"Entire Map\" 0 0 0 %d %d", map->width, map->height);
	MADD("	}");
	MADD("}");
	MADD("Gateways {");
	MADD("	Version 1");
	MADD("	NumGateways %d", (int)map->numGateways);
	MADD("	Gates {");
	for (i = 0; i < map->numGateways; i++)
	{
		GATEWAY *psGate = mapGateway(map, i);

		MADD("		%u %u %u %u", (unsigned)psGate->x1, (unsigned)psGate->y1, (unsigned)psGate->x2, (unsigned)psGate->y2);
	}
	MADD("	}");
	MADD("}");
	MADD("TileTypes {");
	MADD("	NumTiles 128");		// ??? FIXME - read from ttypes file
	MADD("	Tiles {");
	// The first value of 2 is not written into the Deliverance (Warzone binary) format for some reason.
	switch (map->tileset)
	{
	case TILESET_ARIZONA:
		MADD("		2 1 0 2 2 0 2 2 2 2 1 1 1 0 7 7\n"
			 "		7 7 7 8 6 4 4 6 3 3 3 2 4 1 4 7\n"
			 "		7 7 7 4 4 2 2 2 2 1 4 0 4 4 8 8\n"
			 "		2 4 4 4 4 4 4 4 9 9 6 9 6 4 4 9\n"
			 "		9 9 9 9 9 9 9 9 8 4 4 4 8 5 6 2\n"
			 "		2 2 2 2 2 2 2 2 2 2 0 0 0 0 0 0\n"
			 "		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
			 "		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
		break;
	case TILESET_URBAN:
		MADD("		2 2 2 2 2 1 2 2 1 1 1 1 1 1 7 7\n"
			 "		7 7 7 1 8 4 4 0 7 7 7 7 4 4 2 4\n"
			 "		0 2 0 0 2 4 4 0 4 6 2 6 6 6 6 6\n"
			 "		6 4 6 3 4 4 2 2 9 9 9 2 4 2 4 9\n"
			 "		9 9 9 9 8 8 8 8 4 2 0 4 4 2 2 2\n"
			 "		3 2 2 2 2 2 2 2 2 2 0 0 0 0 0 0\n"
			 "		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
			 "		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
		break;
	case TILESET_ROCKIES:
		MADD("		2 0 0 2 2 2 2 2 2 1 8 11 2 11 6 7\n"
			 "		7 7 7 8 6 1 2 6 11 11 0 11 1 1 8 8\n"
			 "		7 7 7 0 0 1 8 0 4 5 11 8 5 8 8 8\n"
			 "		11 11 1 1 1 1 1 8 9 9 5 2 6 6 8 9\n"
			 "		8 10 10 11 11 8 8 10 8 2 10 0 10 8 8 8\n"
			 "		3 2 2 2 2 2 2 2 2 2 0 0 0 0 0 0\n"
			 "		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
			 "		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
		break;
	}
	MADD("  }");
	MADD("}");
	MADD("TileFlags {");		// ??
	MADD("	NumTiles 128");
	MADD("	Flags {");
	MADD("		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
	MADD("		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
	MADD("		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
	MADD("		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
	MADD("		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
	MADD("		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
	MADD("		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
	MADD("		0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
	MADD("	}");
	MADD("}");
	MADD("Brushes {");
	MADD("	Version 2");
	MADD("	NumEdgeBrushes 16");
	MADD("	NumUserBrushes 0");
	MADD("	EdgeBrushes {");
	switch (map->tileset)
	{
	case TILESET_ARIZONA:
		MADD("		0 128 0 -1 0 29 0 29 16 28 16 29 48 28 0 -1 0 30 0 29 32 -1 0 28 32 30 16 28 48 30 48 30 32 54 0");
		MADD("		0 128 0 -1 0 30 32 30 48 28 48 30 16 28 32 -1 0 29 32 30 0 -1 0 28 0 29 48 28 16 29 16 29 0 10 0");
		MADD("		0 128 0 -1 0 42 0 42 16 44 16 42 48 44 0 -1 0 43 0 42 32 -1 0 44 32 43 16 44 48 43 48 43 32 13 128");
		MADD("		0 128 0 -1 0 43 32 43 48 44 48 43 16 44 32 -1 0 42 32 43 0 -1 0 44 0 42 48 44 16 42 16 42 0 77 0");
		MADD("		0 128 0 -1 0 76 32 46 32 72 8 46 0 47 16 -1 0 76 0 76 0 -1 0 47 48 46 0 72 0 46 32 76 32 75 0");
		MADD("		0 128 0 -1 0 5 32 5 48 3 16 5 16 3 0 -1 0 4 32 5 0 -1 0 3 32 4 48 3 48 4 16 4 0 7 0");
		MADD("		0 128 0 -1 0 4 0 4 16 3 48 4 48 3 32 -1 0 5 0 4 32 -1 0 3 0 5 16 3 16 5 48 5 32 13 0");
		MADD("		0 128 0 -1 0 26 0 26 16 25 16 26 48 25 0 -1 0 27 0 26 32 -1 0 25 32 27 16 25 48 27 48 27 32 7 0");
		MADD("		0 128 0 -1 0 27 32 27 48 25 48 27 16 25 32 -1 0 26 32 27 0 -1 0 25 0 26 48 25 16 26 16 26 0 24 0");
		MADD("		0 128 0 -1 0 21 32 21 48 22 48 21 16 22 32 -1 0 20 32 21 0 -1 0 22 0 20 48 22 16 20 16 20 0 23 0");
		MADD("		0 128 0 -1 0 20 0 20 16 22 16 20 48 22 0 -1 0 21 0 20 32 -1 0 22 32 21 16 22 48 21 48 21 32 45 0");
		MADD("		0 128 0 -1 0 16 16 16 32 15 32 16 0 15 16 -1 0 17 16 16 48 -1 0 15 48 17 32 15 0 17 0 17 48 18 0");
		MADD("		0 128 0 -1 0 17 48 17 0 15 0 17 32 15 48 -1 0 16 48 17 16 -1 0 15 16 16 0 15 32 16 32 16 16 13 0");
		MADD("		0 128 0 -1 0 33 0 33 16 32 0 33 48 32 48 -1 0 34 0 33 32 -1 0 32 16 34 16 32 32 34 48 34 32 18 0");
		MADD("		0 128 0 -1 0 34 32 34 48 32 32 34 16 32 16 -1 0 33 32 34 0 -1 0 32 48 33 48 32 0 33 16 33 0 24 0");
		MADD("		0 128 0 -1 0 78 0 78 0 78 0 78 0 78 0 -1 0 78 0 78 0 -1 0 78 0 78 0 78 0 78 0 78 0 78 0");
		break;
	case TILESET_URBAN:
		MADD("		0 128 0 -1 0 73 0 73 16 61 48 73 48 61 32 -1 0 74 0 73 32 -1 0 61 0 74 16 61 16 74 48 74 32 39 128");
		MADD("		0 128 0 -1 0 74 32 74 48 61 16 74 16 61 0 -1 0 73 32 74 0 -1 0 61 32 73 48 61 48 73 16 73 0 79 0");
		MADD("		0 128 0 -1 0 5 0 5 16 7 48 5 48 7 32 -1 0 4 0 5 32 -1 0 7 0 4 16 7 16 4 48 4 32 1 128");
		MADD("		0 128 64 -1 0 4 32 4 48 7 16 4 16 7 0 -1 0 5 32 4 0 -1 0 7 32 5 48 7 48 5 16 5 0 79 0");
		MADD("		0 128 0 -1 0 27 0 27 16 24 16 27 48 24 0 -1 0 26 0 27 32 -1 0 24 32 26 16 24 48 26 48 26 32 18 0");
		MADD("		2 147 64 -1 0 26 32 26 48 24 48 26 16 24 32 -1 0 27 32 26 0 -1 0 24 0 27 48 24 16 27 16 27 0 8 0");
		MADD("		0 128 0 -1 0 16 16 16 32 14 16 16 0 14 0 -1 0 17 16 16 48 -1 0 14 32 17 32 14 48 17 0 17 48 18 0");
		MADD("		0 128 0 -1 0 69 16 69 32 70 32 69 0 70 48 -1 0 69 48 69 48 -1 0 70 16 69 0 70 0 69 32 69 16 79 0");
		MADD("		0 128 0 -1 0 77 0 77 16 72 16 77 48 72 0 -1 0 76 0 77 32 -1 0 72 32 76 16 72 48 76 48 76 32 52 0");
		MADD("		0 128 0 -1 0 76 32 76 48 72 48 76 16 72 32 -1 0 77 32 76 0 -1 0 72 0 77 48 72 16 77 16 77 0 23 0");
		MADD("		0 128 0 -1 0 35 0 35 16 34 16 35 48 34 0 -1 0 36 0 35 32 -1 0 34 32 36 16 34 48 36 48 36 32 1 0");
		MADD("		0 128 64 -1 0 36 32 36 48 34 48 36 16 34 32 -1 0 35 32 36 0 -1 0 34 0 35 48 34 16 35 16 35 0 23 0");
		MADD("		0 128 0 -1 0 38 0 38 16 40 48 38 48 40 32 -1 0 39 0 38 32 -1 0 40 0 39 16 40 16 39 48 39 32 23 0");
		MADD("		0 128 0 -1 0 39 32 39 48 40 16 39 16 40 0 -1 0 38 32 39 0 -1 0 40 32 38 48 40 48 38 16 38 0 51 0");
		MADD("		0 255 64 -1 0 12 48 12 0 13 48 12 32 13 32 -1 0 19 0 12 16 -1 0 13 0 19 0 13 16 19 0 19 0 19 0");
		MADD("		0 128 0 -1 0 19 0 19 0 13 16 19 0 13 0 -1 0 12 16 19 0 -1 0 13 32 12 32 13 48 12 0 12 48 52 0");
		break;
	case TILESET_ROCKIES:
		MADD("		0 128 0 -1 0 46 16 46 32 30 32 46 0 30 16 47 0 64 0 46 48 47 0 30 48 64 16 30 0 64 48 64 32 65 16");
		MADD("		0 128 0 -1 0 43 16 43 32 30 32 43 0 30 16 47 0 10 0 43 48 47 0 30 48 10 16 30 0 10 48 10 32 42 0");
		MADD("		0 128 0 -1 0 10 32 10 48 45 32 10 16 45 16 47 0 64 0 10 0 47 0 45 48 64 16 45 0 64 48 64 32 65 128");
		MADD("		0 128 0 -1 0 34 0 34 16 32 0 34 48 32 48 -1 0 33 0 34 32 -1 0 32 16 33 16 32 32 33 48 33 32 18 0");
		MADD("		0 128 0 -1 0 4 32 4 48 3 48 4 16 3 32 -1 0 5 32 4 0 -1 0 3 0 5 48 3 16 5 16 5 0 6 0");
		MADD("		0 128 0 -1 0 46 16 46 32 47 0 46 0 47 16 -1 0 19 0 46 48 -1 0 47 48 19 16 47 32 19 48 19 32 6 128");
		MADD("		0 128 0 -1 0 5 0 5 16 3 16 5 48 3 0 -1 0 4 0 5 32 -1 0 3 32 4 16 3 48 4 48 4 32 1 128");
		MADD("		0 128 0 -1 0 11 0 11 16 13 0 11 48 13 48 6 128 12 0 11 32 6 128 13 16 12 16 13 32 12 48 12 32 6 128");
		MADD("		0 128 0 -1 0 41 32 41 48 39 48 41 16 39 32 -1 0 40 32 41 0 -1 0 39 0 40 48 39 16 40 16 40 0 54 128");
		MADD("		0 128 0 -1 0 36 0 36 16 35 16 36 48 35 0 -1 0 37 0 36 32 -1 0 35 32 37 16 35 48 37 48 37 32 54 0");
		MADD("		0 128 0 -1 0 66 0 66 16 68 0 66 48 68 48 -1 0 67 0 66 32 -1 0 68 16 67 16 68 32 67 48 67 32 42 128");
		MADD("		0 128 0 -1 0 78 32 78 48 77 32 78 16 77 16 -1 0 55 32 78 0 -1 0 77 48 55 48 77 0 55 16 55 0 65 128");
		MADD("		0 128 0 -1 0 21 32 21 48 22 48 21 16 22 32 -1 0 20 32 21 0 -1 0 22 0 20 48 22 16 20 16 20 0 23 128");
		MADD("		0 128 0 -1 0 67 32 67 48 68 32 67 16 68 16 -1 0 66 32 67 0 -1 0 68 48 66 48 68 0 66 16 66 0 65 128");
		MADD("		0 128 0 -1 0 12 32 12 48 49 32 12 16 49 16 -1 0 11 32 12 0 -1 0 49 48 11 48 49 0 11 16 11 0 42 0");
		MADD("		0 128 0 -1 0 16 16 16 32 15 32 16 0 15 16 15 16 17 16 16 48 15 16 15 48 17 32 15 0 17 0 17 48 18 0");
		break;
	}
	MADD("	}");
	MADD("}");

	fclose(fp);
	mapFree(map);

	return 0;
}
