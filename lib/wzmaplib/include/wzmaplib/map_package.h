/*
	This file is part of Warzone 2100.
	Copyright (C) 2022  Warzone 2100 Project

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

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

#include "map_types.h"
#include "map_debug.h"
#include "map_io.h"

//

namespace WzMap {

struct LevelDetails {
	std::string name; // the level / map name
	MapType type;
	uint8_t players = 0; // the max number of players the map is designed for
	MAP_TILESET tileset = MAP_TILESET::ARIZONA; // the base level data set used for terrain and tiles
	std::string mapFolderPath; // the path to the folder containing the map
	// other info
	std::string createdDate;
	std::string uploadedDate;
	std::string author;
	std::vector<std::string> additionalAuthors;
	std::string license;
	optional<std::string> generator;	// the program / script / library used to generate (or export) the map

public:
	// Returns the ".gam" file path
	// For an old-fashioned map, this should be a file that exists
	// For a newer map, this points to a "virtual" .gam file that does not exist (but is in the parent folder of the map folder, with the same map name .gam)
	std::string gamFilePath() const;
};

enum class LevelFormat {
	LEV, // the old (classic) .lev file format
	JSON // the new (in-folder) level.json format
};
constexpr LevelFormat LatestLevelFormat = LevelFormat::JSON;

constexpr size_t OLD_MAX_LEVEL_SIZE = 20;
struct GamInfo
{
	uint32_t    gameTime = 0;
	uint32_t    GameType = 0;                   /* Type of game, one of the GTYPE_... enums. */
	int32_t     ScrollMinX = 0;                 /* Scroll Limits */
	int32_t     ScrollMinY = 0;
	uint32_t    ScrollMaxX = 0;
	uint32_t    ScrollMaxY = 0;
	char        levelName[OLD_MAX_LEVEL_SIZE] = {};  // name of the level to load up when mid game
};

// MARK: - Various helper functions

void trimTechLevelFromMapName(std::string& mapName);

// MARK: - Handling "level" files for maps (<map>.addon.lev / <map>.xplayers.lev, or the new level.json)

// Load levels details from either:
// - a classic <map>.addon.lev / <map>.xplayers.lev file
// - the new (in-folder) level.json file
// NOTE: This function does *not* handle all of the special properties for campaign level types that are contained in .lev files (currently) - such as gamedesc.lev - it is intended for individual .lev files used by skirmish / multiplay maps
optional<LevelDetails> loadLevelDetails(const std::string& levelFile, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr, std::string* output_priorGenerator = nullptr);

// This function is similar to loadLevelDetails, but is limited to supporting the old .lev format
optional<LevelDetails> loadLevelDetails_LEV(const std::string& levelFile, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr);

// This function is like loadLevelDetails, but is limited to supporting the new JSON format
optional<LevelDetails> loadLevelDetails_JSON(const std::string& levelJsonFile, IOProvider& mapIO, LoggingProtocol* logger = nullptr, std::string* output_priorGenerator = nullptr);

// Export level details to a specified folder in a specified output level format
bool exportLevelDetails(const LevelDetails& details, LevelFormat format, const std::string& outputPath, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr);

// MARK: - High-level interface for loading / saving a map package (ex. archive)

class MapPackage
{
private:
	// Construct an empty Map Package, for modification
	MapPackage();

public:
	// Load a map package from a specified path, optionally supplying:
	// - a logger
	// - a WzMap::IOProvider
	//
	// The default StdIOProvider will assume pathToMapPackage is a path to an extracted map package (i.e. standard filesystem I/O)
	//
	// To load from an archive (.zip/.wz), create a custom implementation of WzMap::IOProvider that supports compressed archive files that you initialize with the path to the zip. An example of this (which uses libzip) is available in `plugins/ZipIOProvider`. You would then set `pathToMapPackage` to be the root path inside the zip. (In the case of plugins\ZipIOProvider, literally "/" or "").
	static std::unique_ptr<MapPackage> loadPackage(const std::string& pathToMapPackage, std::shared_ptr<LoggingProtocol> logger = nullptr, std::shared_ptr<IOProvider> mapIO = std::shared_ptr<IOProvider>(new StdIOProvider()));

	// Construct a new MapPackage object (which can then be exported)
	MapPackage(const LevelDetails& levelDetails, MapType mapType, std::shared_ptr<Map> map);

	// Export the currently-loaded map package to a specified path in a specified format
	// Can convert both the LevelFormat and the WzMap::OutputFormat
	bool exportMapPackageFiles(std::string basePath, LevelFormat levelFormat, WzMap::OutputFormat mapOutputFormat,
							   optional<std::string> mapFolderRelativePathOverride = nullopt, bool copyAdditionalFilesFromOriginalLoadedPackage = false, std::shared_ptr<LoggingProtocol> logger = nullptr, std::shared_ptr<IOProvider> exportIO = std::shared_ptr<IOProvider>(new StdIOProvider()));

	// High-level data loading functions

	// Obtain the LevelDetails
	const LevelDetails& levelDetails() const;

	// Get the loaded level details format
	// Note: Returns a value only if the MapPackage was loaded (i.e. via loadPackage)
	optional<LevelFormat> loadedLevelDetailsFormat() const;

	// Get the map data
	// Returns nullptr if the loading failed
	std::shared_ptr<Map> loadMap(uint32_t seed, std::shared_ptr<LoggingProtocol> logger = nullptr);

	// Get whether the package is a plain map, or a "map mod"
	enum class MapPackageType {
		Map_Plain,	// a package that contains only a map
		Map_Mod		// a package that contains a map and (potentially) other modifications to core game files / textures / data (i.e. a "map mod")
	};
	MapPackageType packageType();
	bool isFlatMapPackage() { return m_flatMapPackage; }

	// Types of base file modifications in a "map mod"
	enum ModTypes {
		GameModels = (1 << 0),	// .pie models (for components, features, structs, misc)
		Effects = (1 << 1),		// .pie models (for effects)
		Shaders = (1 << 2),		// shaders
		Stats = (1 << 3), 		// stats modifications
		Textures = (1 << 4),	// texpages
		Scripts = (1 << 5),		// script / rule modifications
		SkirmishAI = (1 << 6),	// skirmish AI modifications
		Messages = (1 << 7),	// messages
		MiscUI = (1 << 8),		// fonts, icons, images
		Sound = (1 << 9),		// audio, sequenceaudio
		Tilesets = (1 << 10),	// tileset
		Datasets = (1 << 11),	// wrf
	};
	static constexpr ModTypes LastModType = ModTypes::Datasets;
	static std::string to_string(ModTypes modType);

	bool modTypesEnumerate(uint64_t modTypesValue, std::function<void (ModTypes modType)> func);

	// Get the type of base file modifications in the "map mod"
	uint64_t baseModificationTypes();
	bool modTypesEnumerate(std::function<void (ModTypes modType)> func);

	// Extract various map stats / info
	optional<MapStats> calculateMapStats(MapStatsConfiguration statsConfig = MapStatsConfiguration());

private:
	bool loadGamInfo();

private:
	std::string m_pathToMapPackage;
	optional<LevelFormat> m_loadedLevelFormat;
	LevelDetails m_levelDetails;
	GamInfo	m_gamInfo;
	MapType m_mapType;
	optional<MapPackageType> m_packageType;
	bool m_flatMapPackage = false;
	optional<uint64_t> m_modTypes;
	optional<std::string> m_originalGenerator;

	std::shared_ptr<WzMap::IOProvider> m_mapIO;
	std::shared_ptr<LoggingProtocol> m_logger;
	std::shared_ptr<Map> m_loadedMap;
};

} // namespace WzMap
