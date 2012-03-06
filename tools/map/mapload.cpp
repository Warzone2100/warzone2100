// tool "framework"
#include "maplib.h"

// Self
#include "mapload.h"

// Global
uint8_t terrainTypes[MAX_TILE_TEXTURES];

void mapFree(GAMEMAP *map)
{
	if (map)
	{
		unsigned int i;

		free(map->mGateways);
		free(map->mMapTiles);

		for (i = 0; i < ARRAY_SIZE(map->mLndObjects); ++i)
		{
			free(map->mLndObjects[i]);
		}
	}
	free(map);
}

/* Initialise the map structure */
GAMEMAP *mapLoad(char *filename)
{
	char		path[PATH_MAX];
	GAMEMAP		*map = (GAMEMAP *)malloc(sizeof(*map));
	uint32_t	i, j, gwVersion;
	char		aFileType[4];
	bool		littleEndian = true;
	PHYSFS_file	*fp = NULL;
	bool		counted[MAX_PLAYERS];
	uint16_t	pType;

	// this cries out for a class based design
	#define readU8(v) ( littleEndian ? PHYSFS_readULE8(fp, v) : PHYSFS_readUBE8(fp, v) )
	#define readU16(v) ( littleEndian ? PHYSFS_readULE16(fp, v) : PHYSFS_readUBE16(fp, v) )
	#define readU32(v) ( littleEndian ? PHYSFS_readULE32(fp, v) : PHYSFS_readUBE32(fp, v) )
	#define readS8(v) ( littleEndian ? PHYSFS_readSLE8(fp, v) : PHYSFS_readSBE8(fp, v) )
	#define readS16(v) ( littleEndian ? PHYSFS_readSLE16(fp, v) : PHYSFS_readSBE16(fp, v) )
	#define readS32(v) ( littleEndian ? PHYSFS_readSLE32(fp, v) : PHYSFS_readSBE32(fp, v) )
	
	/* === Load map data === */

	strcpy(path, filename);
	strcat(path, "/game.map");
	fp = PHYSFS_openRead(path);
	map->mGateways = NULL;
	map->mMapTiles = NULL;

	if (!fp)
	{
		debug(LOG_ERROR, "Could not open %s", path);
		map->mapVersion = 0;
		map->width = UINT32_MAX;
		map->height = UINT32_MAX;
		map->mMapTiles = NULL;
		goto mapfailure;
	}
	else if (PHYSFS_read(fp, aFileType, 4, 1) != 1
		|| !readU32(&map->mapVersion)
		|| !readU32(&map->width)
		|| !readU32(&map->height)
		|| aFileType[0] != 'm'
		|| aFileType[1] != 'a'
		|| aFileType[2] != 'p')
	{
		debug(LOG_ERROR, "Bad header in %s", path);
		goto failure;
	}
	else if (map->mapVersion <= 9)
	{
		debug(LOG_ERROR, "%s: Unsupported save format version %u", path, map->mapVersion);
		goto failure;
	}
	else if (map->mapVersion > 36)
	{
		debug(LOG_ERROR, "%s: Undefined save format version %u", path, map->mapVersion);
		goto failure;
	}
	else if (map->width * map->height > MAP_MAXAREA)
	{
		debug(LOG_ERROR, "Map %s too large : %d %d", path, map->width, map->height);
		goto failure;
	}

	/* Allocate the memory for the map */
	map->mMapTiles = (MAPTILE *)calloc(map->width * map->height, sizeof(*map->mMapTiles));
	if (!map->mMapTiles)
	{
		debug(LOG_ERROR, "Out of memory");
		goto failure;
	}
	
	/* Load in the map data */
	for (i = 0; i < map->width * map->height; i++)
	{
		uint16_t	texture;
		uint8_t		height;

		if (!readU16(&texture) || !readU8(&height))
		{
			debug(LOG_ERROR, "%s: Error during savegame load", path);
			goto failure;
		}

		map->mMapTiles[i].texture = static_cast<TerrainType>(texture);
		map->mMapTiles[i].height = height;
		for (j = 0; j < MAX_PLAYERS; j++)
		{
			map->mMapTiles[i].tileVisBits = (uint8_t)(map->mMapTiles[i].tileVisBits &~ (uint8_t)(1 << j));
		}
	}

	if (!readU32(&gwVersion) || !readU32(&map->numGateways) || gwVersion != 1)
	{
		debug(LOG_ERROR, "Bad gateway in %s", path);
		goto failure;
	}

	map->mGateways = (GATEWAY *)calloc(map->numGateways, sizeof(*map->mGateways));
	for (i = 0; i < map->numGateways; i++)
	{
		if (!readU8(&map->mGateways[i].x1) || !readU8(&map->mGateways[i].y1)
			|| !readU8(&map->mGateways[i].x2) || !readU8(&map->mGateways[i].y2))
		{
			debug(LOG_ERROR, "%s: Failed to read gateway info", path);
			goto failure;
		}
	}
	PHYSFS_close(fp);
mapfailure:

	/* === Load game data === */

	strcpy(path, filename);
	strcat(path, ".gam");
	fp = PHYSFS_openRead(path);
	if (!fp)
	{
		debug(LOG_ERROR, "Game file %s not found", path);
		goto failure;
	}
	else if (PHYSFS_read(fp, aFileType, 4, 1) != 1
		|| aFileType[0] != 'g'
		|| aFileType[1] != 'a'
		|| aFileType[2] != 'm'
		|| aFileType[3] != 'e'
		|| !readU32(&map->gameVersion))
	{
		debug(LOG_ERROR, "Bad header in %s", path);
		goto failure;
	}
	if (map->gameVersion > 35)	// big-endian
	{
		littleEndian = false;
	}
	if (!readU32(&map->gameTime)
		|| !readU32(&map->gameType)
		|| !readS32(&map->scrollMinX)
		|| !readS32(&map->scrollMinY)
		|| !readU32(&map->scrollMaxX)
		|| !readU32(&map->scrollMaxY)
		|| PHYSFS_read(fp, map->levelName, 20, 1) != 1)
	{
		debug(LOG_ERROR, "Bad data in %s", filename);
		goto failure;
	}
	for (i = 0; i < 8; i++)
	{
		if (map->gameVersion >= 10)
		{
			uint32_t dummy;	// extracted power, not used

			if (!readU32(&map->power[i]) || !readU32(&dummy))
			{
				debug(LOG_ERROR, "Bad power data in %s", filename);
				goto failure;
			}
		}
		else
		{
			map->power[i] = 0;	// TODO... is there a default?
		}
	}
	PHYSFS_close(fp);


	/* === Load feature data === */

	littleEndian = true;
	strcpy(path, filename);
	strcat(path, "/feat.bjo");
	fp = PHYSFS_openRead(path);
	if (!fp)
	{
		debug(LOG_ERROR, "Feature file %s not found", path);
		map->featVersion = 0;
		map->numFeatures = 0;
		map->mLndObjects[IMD_FEATURE] = NULL;
		goto featfailure;
	}
	else if (PHYSFS_read(fp, aFileType, 4, 1) != 1
		|| aFileType[0] != 'f'
		|| aFileType[1] != 'e'
		|| aFileType[2] != 'a'
		|| aFileType[3] != 't'
		|| !readU32(&map->featVersion)
		|| !readU32(&map->numFeatures))
	{
		debug(LOG_ERROR, "Bad features header in %s", path);
		goto failure;
	}
	map->mLndObjects[IMD_FEATURE] = (LND_OBJECT *)malloc(sizeof(*map->mLndObjects[IMD_FEATURE]) * map->numFeatures);
	for(i = 0; i < map->numFeatures; i++)
	{
		LND_OBJECT *psObj = &map->mLndObjects[IMD_FEATURE][i];
		int nameLength = 60;
		uint32_t dummy;
		uint8_t visibility[8];

		if (map->featVersion <= 19)
		{
			nameLength = 40;
		}
		if (PHYSFS_read(fp, psObj->name, nameLength, 1) != 1
			|| !readU32(&psObj->id)
			|| !readU32(&psObj->x) || !readU32(&psObj->y) || !readU32(&psObj->z)
			|| !readU32(&psObj->direction)
			|| !readU32(&psObj->player)
			|| !readU32(&dummy) // BOOL inFire
			|| !readU32(&dummy) // burnStart
			|| !readU32(&dummy)) // burnDamage
		{
			debug(LOG_ERROR, "Failed to read feature from %s", path);
			goto failure;
		}
		psObj->player = 0;	// work around invalid feature owner
		if (map->featVersion >= 14 && PHYSFS_read(fp, &visibility, 1, 8) != 8)
		{
			debug(LOG_ERROR, "Failed to read feature visibility from %s", path);
			goto failure;
		}
		psObj->type = 0;	// IMD LND type for feature
		// Sanity check data
		if (psObj->x >= map->width * TILE_WIDTH || psObj->y >= map->height * TILE_HEIGHT)
		{
			debug(LOG_ERROR, "Bad feature coordinate %u(%u, %u)", psObj->id, psObj->x, psObj->y);
			goto failure;
		}
	}
	PHYSFS_close(fp);
featfailure:


	/* === Load terrain data === */

	littleEndian = true;
	strcpy(path, filename);
	strcat(path, "/ttypes.ttp");
	fp = PHYSFS_openRead(path);
	if (!fp)
	{
		map->terrainVersion = 0;
		goto terrainfailure;
	}
	else if (PHYSFS_read(fp, aFileType, 4, 1) != 1
		|| aFileType[0] != 't'
		|| aFileType[1] != 't'
		|| aFileType[2] != 'y'
		|| aFileType[3] != 'p'
		|| !readU32(&map->terrainVersion)
		|| !readU32(&map->numTerrainTypes))
	{
		debug(LOG_ERROR, "Bad features header in %s", path);
		goto failure;
	}
	
	if (map->numTerrainTypes >= MAX_TILE_TEXTURES)
	{
		// Workaround for fugly map editor bug, since we can't fix the map editor
		map->numTerrainTypes = MAX_TILE_TEXTURES - 1;
	}

	// reset the terrain table
	memset(terrainTypes, 0, sizeof(terrainTypes));

	for (i = 0; i < map->numTerrainTypes; i++)
	{
		readU16(&pType);
		
		if (pType > TER_MAX)
		{
			debug(LOG_ERROR, "loadTerrainTypeMap: terrain type out of range");
			goto terrainfailure;
		}

		terrainTypes[i] = (uint8_t)pType;
	}

	if (terrainTypes[0] == 1 && terrainTypes[1] == 0 && terrainTypes[2] == 2)
	{
		map->tileset = TILESET_ARIZONA;
	}
	else if (terrainTypes[0] == 2 && terrainTypes[1] == 2 && terrainTypes[2] == 2)
	{
		map->tileset = TILESET_URBAN;
	}
	else if (terrainTypes[0] == 0 && terrainTypes[1] == 0 && terrainTypes[2] == 2)
	{
		map->tileset = TILESET_ROCKIES;
	}
	else
	{
		debug(LOG_ERROR, "Unknown terrain signature in %s: %u %u %u", path,
			  terrainTypes[0], terrainTypes[1], terrainTypes[2]);
		goto failure;
	}
	
	PHYSFS_close(fp);
terrainfailure:

	/* === Load structure data === */

	map->mLndObjects[IMD_STRUCTURE] = NULL;
	map->numStructures = 0;
	littleEndian = true;
	strcpy(path, filename);
	strcat(path, "/struct.bjo");
	map->mLndObjects[IMD_STRUCTURE] = NULL;
	fp = PHYSFS_openRead(path);
	if (fp)
	{
		if (PHYSFS_read(fp, aFileType, 4, 1) != 1
			|| aFileType[0] != 's'
			|| aFileType[1] != 't'
			|| aFileType[2] != 'r'
			|| aFileType[3] != 'u'
			|| !readU32(&map->structVersion)
			|| !readU32(&map->numStructures))
		{
			debug(LOG_ERROR, "Bad structure header in %s", path);
			goto failure;
		}
		map->mLndObjects[IMD_STRUCTURE] = (LND_OBJECT *)malloc(sizeof(*map->mLndObjects[IMD_STRUCTURE]) * map->numStructures);
		for (i = 0; i < map->numStructures; i++)
		{
			LND_OBJECT *psObj = &map->mLndObjects[IMD_STRUCTURE][i];
			int nameLength = 60;
			uint32_t dummy;
			uint8_t visibility[8], dummy8;
			int16_t dummyS16;
			int32_t dummyS32;
			char researchName[60];

			if (map->structVersion <= 19)
			{
				nameLength = 40;
			}
			if (PHYSFS_read(fp, psObj->name, nameLength, 1) != 1
				|| !readU32(&psObj->id)
				|| !readU32(&psObj->x) || !readU32(&psObj->y) || !readU32(&psObj->z)
				|| !readU32(&psObj->direction)
				|| !readU32(&psObj->player)
				|| !readU32(&dummy) // BOOL inFire
				|| !readU32(&dummy) // burnStart
				|| !readU32(&dummy) // burnDamage
				|| !readU8(&dummy8)	// status - causes structure padding
				|| !readU8(&dummy8)	// structure padding
				|| !readU8(&dummy8)	// structure padding
				|| !readU8(&dummy8) // structure padding
				|| !readS32(&dummyS32) // currentBuildPts - aligned on 4 byte boundary
				|| !readU32(&dummy) // body
				|| !readU32(&dummy) // armour
				|| !readU32(&dummy) // resistance
				|| !readU32(&dummy) // dummy1
				|| !readU32(&dummy) // subjectInc
				|| !readU32(&dummy) // timeStarted
				|| !readU32(&dummy) // output
				|| !readU32(&dummy) // capacity
				|| !readU32(&dummy)) // quantity
			{
				debug(LOG_ERROR, "Failed to read structure from %s", path);
				goto failure;
			}
			if (map->structVersion >= 12
				&& (!readU32(&dummy)	// factoryInc
					|| !readU8(&dummy8) // loopsPerformed - causes structure padding
					|| !readU8(&dummy8) // structure padding
					|| !readU8(&dummy8) // structure padding
					|| !readU8(&dummy8) // structure padding
					|| !readU32(&dummy) // powerAccrued - aligned on 4 byte boundary
					|| !readU32(&dummy) // dummy2
					|| !readU32(&dummy) // droidTimeStarted
					|| !readU32(&dummy) // timeToBuild
					|| !readU32(&dummy))) // timeStartHold
			{
				debug(LOG_ERROR, "Failed to read structure v12 part from %s", path);
				goto failure;
			}
			if (map->structVersion >= 14 && PHYSFS_read(fp, &visibility, 1, 8) != 8)
			{
				debug(LOG_ERROR, "Failed to read structure visibility from %s", path);
				goto failure;
			}
			if (map->structVersion >= 15 && PHYSFS_read(fp, researchName, nameLength, 1) != 1)
			{
				// If version < 20, then this causes no padding, but the short below
				// will still cause two bytes padding; however, if version >= 20, we
				// will cause 4 bytes padding, but the short below will eat 2 of them,
				// leaving us again with only two bytes padding before the next word.
				debug(LOG_ERROR, "Failed to read structure v15 part from %s", path);
				goto failure;
			}
			if (map->structVersion >= 17 && !readS16(&dummyS16))
			{
				debug(LOG_ERROR, "Failed to read structure v17 part from %s", path);
				goto failure;
			}
			if (map->structVersion >= 15 && !readS16(&dummyS16))	// structure padding
			{
				debug(LOG_ERROR, "Failed to read 16 bits of structure padding from %s", path);
				goto failure;
			}
			if (map->structVersion >= 21 && !readU32(&dummy))
			{
				debug(LOG_ERROR, "Failed to read structure v21 part from %s", path);
				goto failure;
			}
			psObj->type = IMD_STRUCTURE;
			// Sanity check data
			if (psObj->player > MAX_PLAYERS)
			{
				debug(LOG_ERROR, "Bad structure owner %u for structure %d id=%u", psObj->player, i, psObj->id);
				goto failure;
			}
			if (psObj->x >= map->width * TILE_WIDTH || psObj->y >= map->height * TILE_HEIGHT)
			{
				debug(LOG_ERROR, "Bad structure %d coordinate %u(%u, %u)", i, psObj->id, psObj->x, psObj->y);
				goto failure;
			}
		}
		PHYSFS_close(fp);
	}


	/* === Load droid data === */

	map->mLndObjects[IMD_DROID] = NULL;
	map->numDroids = 0;
	littleEndian = true;
	strcpy(path, filename);
	strcat(path, "/dinit.bjo");
	map->mLndObjects[IMD_DROID] = NULL;
	fp = PHYSFS_openRead(path);
	if (fp)
	{
		if (PHYSFS_read(fp, aFileType, 4, 1) != 1
			|| aFileType[0] != 'd'
			|| aFileType[1] != 'i'
			|| aFileType[2] != 'n'
			|| aFileType[3] != 't'
			|| !readU32(&map->droidVersion)
			|| !readU32(&map->numDroids))
		{
			debug(LOG_ERROR, "Bad droid header in %s", path);
			goto failure;
		}
		map->mLndObjects[IMD_DROID] = (LND_OBJECT *)malloc(sizeof(*map->mLndObjects[IMD_DROID]) * map->numDroids);
		for (i = 0; i < map->numDroids; i++)
		{
			LND_OBJECT *psObj = &map->mLndObjects[IMD_DROID][i];
			int nameLength = 60;
			uint32_t dummy;

			if (map->droidVersion <= 19)
			{
				nameLength = 40;
			}
			if (PHYSFS_read(fp, psObj->name, nameLength, 1) != 1
				|| !readU32(&psObj->id)
				|| !readU32(&psObj->x) || !readU32(&psObj->y) || !readU32(&psObj->z)
				|| !readU32(&psObj->direction)
				|| !readU32(&psObj->player)
				|| !readU32(&dummy) // BOOL inFire
				|| !readU32(&dummy) // burnStart
				|| !readU32(&dummy)) // burnDamage
			{
				debug(LOG_ERROR, "Failed to read droid from %s", path);
				goto failure;
			}
			psObj->type = IMD_DROID;
			// Sanity check data
			if (psObj->x >= map->width * TILE_WIDTH || psObj->y >= map->height * TILE_HEIGHT)
			{
				debug(LOG_ERROR, "Bad droid coordinate %u(%u, %u)", psObj->id, psObj->x, psObj->y);
				goto failure;
			}
		}
		PHYSFS_close(fp);
	}

	// Count players by looking for the obligatory construction droids
	map->numPlayers = 0;
	memset(counted, 0, sizeof(counted));
	for(i = 0; i < map->numDroids; i++)
	{
		LND_OBJECT *psObj = &map->mLndObjects[IMD_DROID][i];

		if (counted[psObj->player] == false && (strcmp(psObj->name, "ConstructorDroid") == 0 || strcmp(psObj->name, "ConstructionDroid") == 0))
		{
			counted[psObj->player] = true;
			map->numPlayers++;
		}
	}

	return map;

failure:
	mapFree(map);
	if (fp)
	{
		PHYSFS_close(fp);
	}
	return NULL;
}
