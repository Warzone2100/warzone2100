/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2021  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include <optional-lite/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

#include "map_types.h"
#include "map_debug.h"
#include "map_io.h"
#include "terrain_type.h"
#include "map_terrain_types.h"

// MARK: - Various defines needed by both maplib and the game's map-handling code

#define MAP_MAXWIDTH	256
#define MAP_MAXHEIGHT	256
#define MAP_MAXAREA		(256*256)

// Multiplier for the tile height - used for older map formats (which stored tile height as a byte)
#define ELEVATION_SCALE	2

#define TILE_MAX_HEIGHT	(255 * ELEVATION_SCALE)
#define TILE_MIN_HEIGHT	0

/* Flags for whether texture tiles are flipped in X and Y or rotated */
#define TILE_XFLIP		0x8000
#define TILE_YFLIP		0x4000
#define TILE_ROTMASK	0x3000
#define TILE_ROTSHIFT	12
#define TILE_TRIFLIP	0x0800	// This bit describes the direction the tile is split into 2 triangles (same as triangleFlip)
#define TILE_NUMMASK	0x01ff

static inline unsigned short TileNumber_tile(unsigned short tilenumber)
{
	return tilenumber & TILE_NUMMASK;
}

static inline unsigned short TileNumber_texture(unsigned short tilenumber)
{
	return tilenumber & ~TILE_NUMMASK;
}

/*
 * Usage-Example:
 * tile_coordinate = (world_coordinate / TILE_UNITS) = (world_coordinate >> TILE_SHIFT)
 * world_coordinate = (tile_coordinate * TILE_UNITS) = (tile_coordinate << TILE_SHIFT)
 */

/* The shift on a world coordinate to get the tile coordinate */
#define TILE_SHIFT 7

/* The mask to get internal tile coords from a full coordinate */
#define TILE_MASK 0x7f

/* The number of units accross a tile */
#define TILE_UNITS (1<<TILE_SHIFT)

static inline int32_t world_coord(int32_t mapCoord)
{
	return (uint32_t)mapCoord << TILE_SHIFT;  // Cast because -1 << 7 is undefined, but (unsigned)-1 << 7 gives -128 as desired.
}

static inline int32_t map_coord(int32_t worldCoord)
{
	return worldCoord >> TILE_SHIFT;
}

/// Only for graphics!
static inline float map_coordf(int32_t worldCoord)
{
	return (float)worldCoord / TILE_UNITS;
}

static inline int32_t round_to_nearest_tile(int32_t worldCoord)
{
	return (worldCoord + TILE_UNITS/2) & ~TILE_MASK;
}

/* maps a position down to the corner of a tile */
#define map_round(coord) ((coord) & (TILE_UNITS - 1))

//

namespace WzMap {

// MARK: - Handling game.map / MapData files

// Load the map data
std::shared_ptr<MapData> loadMapData(const std::string &mapFile, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr);

// Write the map data
bool writeMapData(const MapData& map, const std::string &filename, IOProvider& mapIO, OutputFormat format, LoggingProtocol* pCustomLogger = nullptr);

// MARK: - High-level interface for loading a map

enum class MapType
{
	CAMPAIGN,
	SAVEGAME,
	SKIRMISH
};

class Map
{
private:
	Map(const std::string& mapFolderPath, MapType mapType, uint32_t mapMaxPlayers, std::unique_ptr<LoggingProtocol> logger, std::unique_ptr<IOProvider> mapIO = std::unique_ptr<IOProvider>(new StdIOProvider()));

public:
	// Construct an empty Map, for modification
	Map();

	// Load a map from a specified folder path + mapType + maxPlayers + random seed (only used for script-generated maps), optionally supplying:
	// - previewOnly (set to true to shortcut processing of map details that don't factor into preview generation)
	// - a logger
	// - a WzMap::IOProvider
	static std::unique_ptr<Map> loadFromPath(const std::string& mapFolderPath, MapType mapType, uint32_t mapMaxPlayers, uint32_t seed, bool previewOnly = false, std::unique_ptr<LoggingProtocol> logger = nullptr, std::unique_ptr<IOProvider> mapIO = std::unique_ptr<IOProvider>(new StdIOProvider()));

	// Export a map to a specified folder path in a specified output format (version)
	static bool exportMapToPath(Map& map, const std::string& mapFolderPath, MapType mapType, uint32_t mapMaxPlayers, OutputFormat format, std::unique_ptr<LoggingProtocol> logger = nullptr, std::unique_ptr<IOProvider> mapIO = std::unique_ptr<IOProvider>(new StdIOProvider()));

	// High-level data loading functions

	// Get the map data
	// Returns nullptr if constructed for loading and the loading failed
	std::shared_ptr<MapData> mapData();

	// Get the structures
	// Returns nullptr if constructed for loading and the loading failed
	std::shared_ptr<std::vector<Structure>> mapStructures();

	// Get the droids
	// Returns nullptr if constructed for loading and the loading failed
	std::shared_ptr<std::vector<Droid>> mapDroids();

	// Get the features
	// Returns nullptr if constructed for loading and the loading failed
	std::shared_ptr<std::vector<Feature>> mapFeatures();

	// Get the terrain type map
	std::shared_ptr<TerrainTypeData> mapTerrainTypes();

	// Obtaining CRC values
	uint32_t crcSumMapTiles(uint32_t crc);
	uint32_t crcSumStructures(uint32_t crc);
	uint32_t crcSumDroids(uint32_t crc);
	uint32_t crcSumFeatures(uint32_t crc);

	// Map Format information (for a loaded map)
	enum class LoadedFormat
	{
		MIXED,				// Doesn't fully fit a map format - may have a mix of files from different formats / versions (loadable, but not ideal)
		BINARY_OLD,			// Old binary .bjo
		JSON_v1, 			// JSON (v1) format (WZ 3.4+?)
		SCRIPT_GENERATED,	// Script-generated map (WZ 4.0+)
		JSON_v2				// JSON (v2) format + full-range game.map (WZ 4.1+)
	};
	// Obtain the map format of a loaded map
	// Returns nullopt if loading the map data failed, the format can't be determined, or if this WzMap instance was not created by loading a map
	optional<LoadedFormat> loadedMapFormat();

	// Other Map instance data
	bool wasScriptGenerated() const { return m_wasScriptGenerated; }
	const std::string& mapFolderPath() const { return m_mapFolderPath; }

private:
	std::string m_mapFolderPath;
	MapType m_mapType;
	uint32_t m_mapMaxPlayers = 8;
	std::unique_ptr<LoggingProtocol> m_logger;
	std::unique_ptr<WzMap::IOProvider> m_mapIO;
	bool m_wasScriptGenerated = false;
	std::shared_ptr<MapData> m_mapData;
	std::shared_ptr<std::vector<Structure>> m_structures;
	std::shared_ptr<std::vector<Droid>> m_droids;
	std::shared_ptr<std::vector<Feature>> m_features;
	std::shared_ptr<TerrainTypeData> m_terrainTypes;
private:
	// Info about loaded file versions
	struct LoadedFileVersion
	{
		enum FileType
		{
			BinaryBJO,
			JSON,
			ScriptGenerated
		};
	public:
		LoadedFileVersion()
		{ }
		LoadedFileVersion(FileType type, uint32_t version)
		: m_fileType(type)
		, m_fileVersion(version)
		{ }
	public:
		inline FileType fileType() const { return m_fileType; }
		inline uint32_t fileVersion() const { return m_fileVersion; }
	private:
		FileType m_fileType = FileType::ScriptGenerated;
		uint32_t m_fileVersion = 0;
	};

	struct FileTypeFileHash
	{
		std::size_t operator()(LoadedFileVersion::FileType t) const
		{
			return static_cast<std::size_t>(t);
		}
	};

	enum class MapFile
	{
		MapData,
		Structures,
		Droids,
		Features,
		TerrainTypes
	};

	struct MapFileHash
	{
		std::size_t operator()(MapFile t) const
		{
			return static_cast<std::size_t>(t);
		}
	};

	std::unordered_map<MapFile, LoadedFileVersion, MapFileHash> m_fileVersions;
};

} // namespace WzMap
