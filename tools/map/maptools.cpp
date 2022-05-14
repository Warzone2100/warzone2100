// Warzone 2100 MapTools
/*
	This file is part of Warzone 2100.
	Copyright (C) 2021  Warzone 2100 Project

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

#if defined(__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#endif
#include "CLI11.hpp"
#if defined(__GNUC__)
# pragma GCC diagnostic pop
#endif
#include <wzmaplib/map.h>
#include <wzmaplib/map_preview.h>
#include <wzmaplib/map_package.h>
#if !defined(WZ_MAPTOOLS_DISABLE_ARCHIVE_SUPPORT)
#include <ZipIOProvider.h>
#endif
#include <json/json.hpp>
#include <iostream>
#include <string>
#include <cstdlib>
#include "pngsave.h"

class MapToolDebugLogger : public WzMap::LoggingProtocol
{
public:
	MapToolDebugLogger(bool verbose)
	: verbose(verbose)
	{ }
	virtual ~MapToolDebugLogger() { }
	virtual void printLog(WzMap::LoggingProtocol::LogLevel level, const char *function, int line, const char *str) override
	{
		std::ostream* pOutputStream = &(std::cout);
		if (level == WzMap::LoggingProtocol::LogLevel::Error)
		{
			pOutputStream = &(std::cerr);
		}
		std::string levelStr;
		switch (level)
		{
			case WzMap::LoggingProtocol::LogLevel::Info_Verbose:
			case WzMap::LoggingProtocol::LogLevel::Info:
				if (!verbose) { return; }
				levelStr = "INFO";
				break;
			case WzMap::LoggingProtocol::LogLevel::Warning:
				levelStr = "WARNING";
				break;
			case WzMap::LoggingProtocol::LogLevel::Error:
				levelStr = "ERROR";
				break;
		}
		(*pOutputStream) << levelStr << ": [" << function << ":" << line << "] " << str << std::endl;
	}
private:
	bool verbose = false;
};

// Defining operator<<() for enum classes to override CLI11's enum streaming
namespace WzMap {
inline std::ostream &operator<<(std::ostream &os, const MapType& mapType) {
	switch(mapType) {
	case WzMap::MapType::CAMPAIGN:
		os << "Campaign";
		break;
	case WzMap::MapType::SAVEGAME:
		os << "Savegame";
		break;
	case WzMap::MapType::SKIRMISH:
		os << "Skirmish";
		break;
	}
	return os;
}
inline std::ostream &operator<<(std::ostream &os, const OutputFormat& outputFormat) {
	switch(outputFormat) {
	case WzMap::OutputFormat::VER1_BINARY_OLD:
		os << "Binary .BJO (flaME-compatible / old)";
		break;
	case WzMap::OutputFormat::VER2:
		os << "JSONv1 (WZ 3.4+)";
		break;
	case WzMap::OutputFormat::VER3:
		os << "JSONv2 (WZ 4.1+)";
		break;
	}
	return os;
}
inline std::ostream &operator<<(std::ostream &os, const Map::LoadedFormat& mapFormat) {
	switch(mapFormat) {
	case WzMap::Map::LoadedFormat::MIXED:
		os << "Mixed Formats";
		break;
	case WzMap::Map::LoadedFormat::BINARY_OLD:
		os << "Binary .BJO (old)";
		break;
	case WzMap::Map::LoadedFormat::JSON_v1:
		os << "JSONv1 (WZ 3.4+)";
		break;
	case WzMap::Map::LoadedFormat::SCRIPT_GENERATED:
		os << "Script-Generated (WZ 4.0+)";
		break;
	case WzMap::Map::LoadedFormat::JSON_v2:
		os << "JSONv2 (WZ 4.1+)";
		break;
	}
	return os;
}
inline std::ostream &operator<<(std::ostream &os, const LevelFormat& levelFormat) {
	switch(levelFormat) {
	case WzMap::LevelFormat::LEV:
		os << "LEV (flaME-compatible / old)";
		break;
	case WzMap::LevelFormat::JSON:
		os << "JSON level file (WZ 4.3+)";
		break;
	}
	return os;
}
} // namespace WzMap


static bool convertMapPackage(const std::string& mapPackageContentsPath, const std::string& outputPath, WzMap::LevelFormat levelFormat, WzMap::OutputFormat outputFormat, bool copyAdditionalFiles, bool verbose, bool exportUncompressed, std::shared_ptr<WzMap::IOProvider> mapIO = std::shared_ptr<WzMap::IOProvider>(new WzMap::StdIOProvider()))
{
	auto logger = std::make_shared<MapToolDebugLogger>(new MapToolDebugLogger(verbose));

	auto wzMapPackage = WzMap::MapPackage::loadPackage(mapPackageContentsPath, logger, mapIO);
	if (!wzMapPackage)
	{
		std::cerr << "Failed to load map archive package from: " << mapPackageContentsPath << std::endl;
		return false;
	}

	auto wzMap = wzMapPackage->loadMap(rand(), logger);
	if (!wzMap)
	{
		// Failed to load map
		std::cerr << "Failed to load map from map archive path: " << mapPackageContentsPath << std::endl;
		return false;
	}

	std::string outputBasePath;
	std::shared_ptr<WzMap::IOProvider> exportIO;
	if (exportUncompressed)
	{
		exportIO = std::make_shared<WzMap::StdIOProvider>();
		outputBasePath = outputPath;
	}
	else
	{
#if !defined(WZ_MAPTOOLS_DISABLE_ARCHIVE_SUPPORT)
		exportIO = WzMapZipIO::createZipArchiveFS(outputPath.c_str());
		if (!exportIO)
		{
			std::cerr << "Failed to open map archive file for output: " << outputPath << std::endl;
			return false;
		}
		outputBasePath.clear();
#else
		std::cerr << "maptools was not compiled with map archive (.wz) support - you must pass --output-uncompressed" << std::endl;
		return false;
#endif // !defined(WZ_MAPTOOLS_DISABLE_ARCHIVE_SUPPORT)
	}

	if (!wzMapPackage->exportMapPackageFiles(outputBasePath, levelFormat, outputFormat, nullopt, copyAdditionalFiles, logger, exportIO))
	{
		// Failed to export map package
		std::cerr << "Failed to export map package to: " << outputPath << std::endl;
		return false;
	}

	auto inputMapFormat = wzMap->loadedMapFormat();

	std::cout << "Converted map package:" << std::endl
			<< "\t - from format [";
	if (inputMapFormat.has_value())
	{
		std::cout << inputMapFormat.value();
	}
	else
	{
		std::cout << "unknown";
	}
	std::cout << "] -> [" << outputFormat << "]" << std::endl;
	std::cout << "\t - with: " << levelFormat << std::endl;
	std::cout << "\t - saved to: " << outputPath << std::endl;

	return true;
}

#if !defined(WZ_MAPTOOLS_DISABLE_ARCHIVE_SUPPORT)
static bool convertMapPackage_FromArchive(const std::string& mapArchive, const std::string& outputPath, WzMap::LevelFormat levelFormat, WzMap::OutputFormat outputFormat, bool copyAdditionalFiles, bool verbose, bool outputUncompressed)
{
	auto zipArchive = WzMapZipIO::openZipArchiveFS(mapArchive.c_str());
	if (!zipArchive)
	{
		std::cerr << "Failed to open map archive file: " << mapArchive << std::endl;
		return false;
	}

	return convertMapPackage("", outputPath, levelFormat, outputFormat, copyAdditionalFiles, verbose, outputUncompressed, zipArchive);
}
#endif // !defined(WZ_MAPTOOLS_DISABLE_ARCHIVE_SUPPORT)

static bool convertMap(WzMap::MapType mapType, uint32_t mapMaxPlayers, const std::string& inputMapDirectory, const std::string& outputMapDirectory, WzMap::OutputFormat outputFormat, bool verbose)
{
	auto wzMap = WzMap::Map::loadFromPath(inputMapDirectory, mapType, mapMaxPlayers, rand(), std::make_shared<MapToolDebugLogger>(new MapToolDebugLogger(verbose)));
	if (!wzMap)
	{
		// Failed to load map
		std::cerr << "Failed to load map: " << inputMapDirectory << std::endl;
		return false;
	}

	if (!wzMap->exportMapToPath(*(wzMap.get()), outputMapDirectory, mapType, mapMaxPlayers, outputFormat, std::make_shared<MapToolDebugLogger>(new MapToolDebugLogger(verbose))))
	{
		// Failed to export map
		std::cerr << "Failed to export map to: " << outputMapDirectory << std::endl;
		return false;
	}

	auto inputMapFormat = wzMap->loadedMapFormat();

	std::cout << "Converted map:\n"
			<< "\t - from format [";
	if (inputMapFormat.has_value())
	{
		std::cout << inputMapFormat.value();
	}
	else
	{
		std::cout << "unknown";
	}
	std::cout << "] -> [" << outputFormat << "]\n";
	std::cout << "\t - saved to: " << outputMapDirectory << std::endl;

	return true;
}

enum class MapTileset
{
	Arizona,
	Urban,
	Rockies
};

static optional<MapTileset> guessMapTileset(WzMap::Map& wzMap)
{
	auto pTerrainTypes = wzMap.mapTerrainTypes();
	if (!pTerrainTypes)
	{
		return nullopt;
	}
	auto& terrainTypes = pTerrainTypes->terrainTypes;
	if (terrainTypes[0] == 1 && terrainTypes[1] == 0 && terrainTypes[2] == 2)
	{
		return MapTileset::Arizona;
	}
	else if (terrainTypes[0] == 2 && terrainTypes[1] == 2 && terrainTypes[2] == 2)
	{
		return MapTileset::Urban;
	}
	else if (terrainTypes[0] == 0 && terrainTypes[1] == 0 && terrainTypes[2] == 2)
	{
		return MapTileset::Rockies;
	}
	else
	{
		std::cerr << "Unknown terrain types signature: " << terrainTypes[0] << terrainTypes[1] << terrainTypes[2] << "; defaulting to Arizona tilset." << std::endl;
		return MapTileset::Arizona;
	}
}

static bool generateMapPreviewPNG_FromMapObject(WzMap::Map& map, const std::string& outputPNGPath)
{
	WzMap::MapPreviewColorScheme previewColorScheme;
	previewColorScheme.hqColor = {255, 0, 255, 255};
	previewColorScheme.oilResourceColor = {255, 255, 0, 255};
	previewColorScheme.oilBarrelColor = {128, 192, 0, 255};
	auto mapTilesetResult = guessMapTileset(map);
	if (!mapTilesetResult.has_value())
	{
		// Failed to guess the map tilset - presumably an error loading the map
		std::cerr << "Failed to guess map tilset" << std::endl;
		return false;
	}
	switch (mapTilesetResult.value())
	{
	case MapTileset::Arizona:
		previewColorScheme.tilesetColors = WzMap::TilesetColorScheme::TilesetArizona();
		break;
	case MapTileset::Urban:
		previewColorScheme.tilesetColors = WzMap::TilesetColorScheme::TilesetUrban();
		break;
	case MapTileset::Rockies:
		previewColorScheme.tilesetColors = WzMap::TilesetColorScheme::TilesetRockies();
		break;
	}

	auto previewResult = WzMap::generate2DMapPreview(map, previewColorScheme);
	if (!previewResult)
	{
		std::cerr << "Failed to generate map preview" << std::endl;
		return false;
	}
	if (!savePng(outputPNGPath.c_str(), previewResult->imageData.data(), static_cast<int>(previewResult->width), static_cast<int>(previewResult->height)))
	{
		std::cerr << "Failed to save preview PNG" << std::endl;
		return false;
	}

	std::cout << "Generated map preview:\n"
			<< "\t - saved to: " << outputPNGPath << std::endl;

	return true;
}

static bool generateMapPreviewPNG_FromPackageContents(const std::string& mapPackageContentsPath, const std::string& outputPNGPath, bool verbose, std::shared_ptr<WzMap::IOProvider> mapIO = std::shared_ptr<WzMap::IOProvider>(new WzMap::StdIOProvider()))
{
	auto logger = std::make_shared<MapToolDebugLogger>(new MapToolDebugLogger(verbose));

	auto wzMapPackage = WzMap::MapPackage::loadPackage(mapPackageContentsPath, logger, mapIO);
	if (!wzMapPackage)
	{
		std::cerr << "Failed to load map archive package from: " << mapPackageContentsPath << std::endl;
		return false;
	}

	auto wzMap = wzMapPackage->loadMap(rand(), logger);
	if (!wzMap)
	{
		// Failed to load map
		std::cerr << "Failed to load map from map archive path: " << mapPackageContentsPath << std::endl;
		return false;
	}

	return generateMapPreviewPNG_FromMapObject(*(wzMap.get()), outputPNGPath);
}

#if !defined(WZ_MAPTOOLS_DISABLE_ARCHIVE_SUPPORT)
static bool generateMapPreviewPNG_FromArchive(const std::string& mapArchive, const std::string& outputPNGPath, bool verbose)
{
	auto zipArchive = WzMapZipIO::openZipArchiveFS(mapArchive.c_str());
	if (!zipArchive)
	{
		std::cerr << "Failed to open map archive file: " << mapArchive << std::endl;
		return false;
	}

	return generateMapPreviewPNG_FromPackageContents("", outputPNGPath, verbose, zipArchive);
}
#endif // !defined(WZ_MAPTOOLS_DISABLE_ARCHIVE_SUPPORT)

static bool generateMapPreviewPNG_FromMapDirectory(WzMap::MapType mapType, uint32_t mapMaxPlayers, const std::string& inputMapDirectory, const std::string& outputPNGPath, bool verbose)
{
	auto wzMap = WzMap::Map::loadFromPath(inputMapDirectory, mapType, mapMaxPlayers, rand(), std::make_shared<MapToolDebugLogger>(new MapToolDebugLogger(verbose)));
	if (!wzMap)
	{
		// Failed to load map
		std::cerr << "Failed to load map: " << inputMapDirectory << std::endl;
		return false;
	}

	return generateMapPreviewPNG_FromMapObject(*(wzMap.get()), outputPNGPath);
}

static nlohmann::ordered_json generateMapInfoJSON_FromMapStats(const WzMap::LevelDetails details, const WzMap::MapStats& stats, std::shared_ptr<MapToolDebugLogger> logger)
{
	nlohmann::ordered_json output = nlohmann::ordered_json::object();

	// Level Details
	output["name"] = details.name;
	output["type"] = WzMap::to_string(details.type);
	output["players"] = details.players;
	output["tileset"] = WzMap::to_string(details.tileset);
	if (!details.author.empty())
	{
		nlohmann::ordered_json authorinfo = nlohmann::ordered_json::object();
		authorinfo["name"] = details.author;
		output["author"] = authorinfo;
	}
	else
	{
//		debug(pCustomLogger, LOG_WARNING, "LevelDetails is missing a valid author");
	}
	if (!details.license.empty())
	{
		output["license"] = details.license;
	}
	else
	{
//		debug(pCustomLogger, LOG_WARNING, "LevelDetails is missing a valid license");
	}
	if (!details.createdDate.empty())
	{
		output["created"] = details.createdDate;
	}
	else
	{
//		debug(pCustomLogger, LOG_WARNING, "LevelDetails is missing a created-date");
	}
	if (details.generator.has_value() && !details.generator.value().empty())
	{
		output["generator"] = details.generator.value();
	}

	// Map Stats
	auto mapsize = nlohmann::ordered_json::object();
	mapsize["w"] = stats.mapWidth;
	mapsize["h"] = stats.mapHeight;
	output["mapsize"] = std::move(mapsize);
	auto scavengerCounts = nlohmann::ordered_json::object();
	scavengerCounts["units"] = stats.scavengerUnits;
	scavengerCounts["structures"] = stats.scavengerStructs;
	scavengerCounts["factories"] = stats.scavengerFactories;
	scavengerCounts["resourceExtractors"] = stats.scavengerResourceExtractors;
	output["scavenger"] = std::move(scavengerCounts);
	output["oilWells"] = stats.oilWellsTotal;
	auto perPlayerCounts = nlohmann::ordered_json::object();
	perPlayerCounts["resourceExtractors"] = stats.resourceExtractorsPerPlayer;
	perPlayerCounts["powerGenerators"] = stats.powerGeneratorsPerPlayer;
	perPlayerCounts["regFactories"] = stats.regFactoriesPerPlayer;
	perPlayerCounts["vtolFactories"] = stats.vtolFactoriesPerPlayer;
	perPlayerCounts["cyborgFactories"] = stats.cyborgFactoriesPerPlayer;
	perPlayerCounts["researchCenters"] = stats.researchCentersPerPlayer;
	output["player"] = std::move(perPlayerCounts);
	auto startEquality = nlohmann::ordered_json::object();
	startEquality["units"] = stats.playerBalance.units;
	startEquality["structures"] = stats.playerBalance.structures;
	startEquality["resourceExtractors"] = stats.playerBalance.resourceExtractors;
	startEquality["powerGenerators"] = stats.playerBalance.powerGenerators;
	startEquality["factories"] = stats.playerBalance.factories;
	startEquality["researchCenters"] = stats.playerBalance.researchCenters;
	auto balance = nlohmann::ordered_json::object();
	balance["startEquality"] = std::move(startEquality);
	output["balance"] = std::move(balance);

	return output;
}

inline std::string loadedFormatToString(optional<WzMap::Map::LoadedFormat> mapFormat)
{
	if (!mapFormat.has_value())
	{
		return "unknown";
	}
	switch(mapFormat.value())
	{
	case WzMap::Map::LoadedFormat::MIXED:
		return "mixed";
	case WzMap::Map::LoadedFormat::BINARY_OLD:
		return "binary";
	case WzMap::Map::LoadedFormat::JSON_v1:
		return "jsonv1";
	case WzMap::Map::LoadedFormat::SCRIPT_GENERATED:
		return "script";
	case WzMap::Map::LoadedFormat::JSON_v2:
		return "jsonv2";
	}
	return ""; // silence warning
}

inline std::string levelFormatToString(optional<WzMap::LevelFormat> levelFormat)
{
	if (!levelFormat.has_value())
	{
		return "";
	}
	switch(levelFormat.value())
	{
		case WzMap::LevelFormat::LEV:
			return "lev";
		case WzMap::LevelFormat::JSON:
			return "json";
	}
	return ""; // silence warning
}

static nlohmann::ordered_json generateMapInfoJSON_FromPackage(WzMap::MapPackage& mapPackage, const WzMap::MapStats& stats, std::shared_ptr<MapToolDebugLogger> logger)
{
	nlohmann::ordered_json output = generateMapInfoJSON_FromMapStats(mapPackage.levelDetails(),stats, logger);

	// Whether the map package is a "map mod"
	output["mapMod"] = static_cast<bool>(mapPackage.packageType() == WzMap::MapPackage::MapPackageType::Map_Mod);
	// Modification types (for map mods)
	nlohmann::ordered_json modTypes = nlohmann::ordered_json::object();
	bool anyModTypes = mapPackage.modTypesEnumerate([&modTypes](WzMap::MapPackage::ModTypes type) {
		modTypes[WzMap::MapPackage::to_string(type)] = true;
	});
	if (anyModTypes)
	{
		output["modTypes"] = modTypes;
	}
	// The loaded level details format
	auto levelFormat = mapPackage.loadedLevelDetailsFormat();
	if (levelFormat.has_value())
	{
		output["levelFormat"] = levelFormatToString(levelFormat);
	}
	else
	{
		std::cerr << "Loaded level details format is missing ??" << std::endl;
	}
	// The loaded map format
	auto pMap = mapPackage.loadMap(0);
	if (pMap)
	{
		auto loadedMapFormat = pMap->loadedMapFormat();
		output["mapFormat"] = loadedFormatToString(loadedMapFormat);
	}
	else
	{
		std::cerr << "Failed to load map from archive package ??" << std::endl;
	}
	// Whether the map package is a new "flat" map package
	output["flatMapPackage"] = mapPackage.isFlatMapPackage();

	return output;
}

static optional<nlohmann::ordered_json> generateMapInfoJSON_FromPackageContents(const std::string& mapPackageContentsPath, std::shared_ptr<MapToolDebugLogger> logger, std::shared_ptr<WzMap::IOProvider> mapIO = std::shared_ptr<WzMap::IOProvider>(new WzMap::StdIOProvider()))
{
	auto wzMapPackage = WzMap::MapPackage::loadPackage(mapPackageContentsPath, logger, mapIO);
	if (!wzMapPackage)
	{
		std::cerr << "Failed to load map archive package from: " << mapPackageContentsPath << std::endl;
		return nullopt;
	}

	auto mapStatsResult = wzMapPackage->calculateMapStats();
	if (!mapStatsResult.has_value())
	{
		std::cerr << "Failed to calculate map info / stats from: " << mapPackageContentsPath << std::endl;
		return nullopt;
	}

	return generateMapInfoJSON_FromPackage(*(wzMapPackage.get()), mapStatsResult.value(), logger);
}

#if !defined(WZ_MAPTOOLS_DISABLE_ARCHIVE_SUPPORT)
static optional<nlohmann::ordered_json> generateMapInfoJSON_FromArchive(const std::string& mapArchive, std::shared_ptr<MapToolDebugLogger> logger)
{
	auto zipArchive = WzMapZipIO::openZipArchiveFS(mapArchive.c_str());
	if (!zipArchive)
	{
		std::cerr << "Failed to open map archive file: " << mapArchive << std::endl;
		return nullopt;
	}

	return generateMapInfoJSON_FromPackageContents("", logger, zipArchive);
}
#endif // !defined(WZ_MAPTOOLS_DISABLE_ARCHIVE_SUPPORT)

// specify string->value mappings
static const std::map<std::string, WzMap::MapType> maptype_map{{"skirmish", WzMap::MapType::SKIRMISH}, {"campaign", WzMap::MapType::CAMPAIGN}};
static const std::map<std::string, WzMap::LevelFormat> levelformat_map{{"latest", WzMap::LatestLevelFormat}, {"json", WzMap::LevelFormat::JSON}, {"lev", WzMap::LevelFormat::LEV}};
static const std::map<std::string, WzMap::OutputFormat> outputformat_map{{"latest", WzMap::LatestOutputFormat}, {"jsonv2", WzMap::OutputFormat::VER3}, {"json", WzMap::OutputFormat::VER2}, {"bjo", WzMap::OutputFormat::VER1_BINARY_OLD}};

static bool strEndsWith(const std::string& str, const std::string& suffix)
{
	return (str.size() >= suffix.size()) && (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0);
}

/// Check for a specified file extension
class FileExtensionValidator : public CLI::Validator {
  public:
	FileExtensionValidator(std::string fileExtension) {
		std::stringstream out;
		out << "FILE(*" << fileExtension << ")";
		description(out.str());

		if (!fileExtension.empty())
		{
			if (fileExtension.front() != '.')
			{
				fileExtension = "." + fileExtension;
			}
		}

		func_ = [fileExtension](std::string &filename) {
			if (!strEndsWith(filename, fileExtension))
			{
				return "Filename does not end in extension: " + fileExtension;
			}
			return std::string();
		};
	}
};

static void addSubCommand_Package(CLI::App& app, int& retVal, bool& verbose)
{
	CLI::App* sub_package = app.add_subcommand("package", "Manipulating a map package");
	sub_package->fallthrough();

	std::string inputOptionDescription;
#if !defined(WZ_MAPTOOLS_DISABLE_ARCHIVE_SUPPORT)
	inputOptionDescription = "Input map package (.wz package, or extracted package folder)";
#else
	inputOptionDescription = "Input map package (extracted package folder)";
#endif

	auto inputPathIsFile = [](const std::string& path) -> bool {
		if (path.empty())
		{
			return false;
		}
		return CLI::ExistingFile(path).empty();
	};

	// [CONVERTING MAP PACKAGE]
	CLI::App* sub_convert = sub_package->add_subcommand("convert", "Convert a map from one format to another");
	sub_convert->fallthrough();
	static WzMap::LevelFormat outputLevelFormat = WzMap::LevelFormat::JSON;
	sub_convert->add_option("-l,--levelformat", outputLevelFormat, "Output level info format")
		->transform(CLI::CheckedTransformer(levelformat_map, CLI::ignore_case).description("value in {\n\t\tlev -> LEV (flaME-compatible / old),\n\t\tjson -> JSON level file (WZ 4.3+),\n\t\tlatest -> " + CLI::detail::to_string(WzMap::LatestLevelFormat) + "}"))
		->default_val("latest");
	static WzMap::OutputFormat outputMapFormat = WzMap::LatestOutputFormat;
	sub_convert->add_option("-f,--format", outputMapFormat, "Output map format")
		->required()
		->transform(CLI::CheckedTransformer(outputformat_map, CLI::ignore_case).description("value in {\n\t\tbjo -> Binary .BJO (flaME-compatible / old),\n\t\tjson -> JSONv1 (WZ 3.4+),\n\t\tjsonv2 -> JSONv2 (WZ 4.1+),\n\t\tlatest -> " + CLI::detail::to_string(WzMap::LatestOutputFormat) + "}"));
	static std::string inputMapPackage;
	sub_convert->add_option("-i,--input,input", inputMapPackage, inputOptionDescription)
		->required()
		->check(CLI::ExistingPath);
	static std::string outputPath;
	sub_convert->add_option("-o,--output,output", outputPath, "Output path")
		->required()
		->check(CLI::NonexistentPath);
	static bool sub_convert_copyadditionalfiles = false;
	sub_convert->add_flag("--preserve-mods", sub_convert_copyadditionalfiles, "Copy other files from the original map package (i.e. the extra files / modifications in a map-mod)");
	static bool sub_convert_uncompressed = false;
	sub_convert->add_flag("--output-uncompressed", sub_convert_uncompressed, "Output uncompressed to a folder (not in a .wz file)");
	sub_convert->callback([&]() {
		if (inputPathIsFile(inputMapPackage))
		{
#if !defined(WZ_MAPTOOLS_DISABLE_ARCHIVE_SUPPORT)
			if (!convertMapPackage_FromArchive(inputMapPackage, outputPath, outputLevelFormat, outputMapFormat, sub_convert_copyadditionalfiles, verbose, sub_convert_uncompressed))
			{
				retVal = 1;
			}
#else
			std::cerr << "ERROR: maptools was compiled without support for .wz archives, and cannot open: " << inputMapPackage << std::endl;
			retVal = 1;
#endif
		}
		else
		{
			if (!convertMapPackage(inputMapPackage, outputPath, outputLevelFormat, outputMapFormat, sub_convert_copyadditionalfiles, verbose, sub_convert_uncompressed))
			{
				retVal = 1;
			}
		}
	});

	// [GENERATING MAP PREVIEW PNG]
	CLI::App* sub_preview = sub_package->add_subcommand("genpreview", "Generate a map preview PNG");
	sub_preview->fallthrough();
	static std::string preview_inputMap;
	sub_preview->add_option("-i,--input,input", preview_inputMap, inputOptionDescription)
		->required()
		->check(CLI::ExistingPath);
	static std::string preview_outputPNGFilename;
	sub_preview->add_option("-o,--output,output", preview_outputPNGFilename, "Output PNG filename (+ path)")
		->required()
		->check(FileExtensionValidator(".png"));
	sub_preview->callback([&]() {
		if (inputPathIsFile(preview_inputMap))
		{
#if !defined(WZ_MAPTOOLS_DISABLE_ARCHIVE_SUPPORT)
			if (!generateMapPreviewPNG_FromArchive(preview_inputMap, preview_outputPNGFilename, verbose))
			{
				retVal = 1;
			}
#else
			std::cerr << "ERROR: maptools was compiled without support for .wz archives, and cannot open: " << preview_inputMap << std::endl;
			retVal = 1;
#endif
		}
		else
		{
			if (!generateMapPreviewPNG_FromPackageContents(preview_inputMap, preview_outputPNGFilename, verbose))
			{
				retVal = 1;
			}
		}
	});

	// [EXTRACTING INFORMATION FROM A MAP PACKAGE]
	CLI::App* sub_info = sub_package->add_subcommand("info", "Extract info / stats from a map package");
	sub_info->fallthrough();
	static std::string info_inputMap;
	sub_info->add_option("-i,--input,input", info_inputMap, inputOptionDescription)
		->required()
		->check(CLI::ExistingPath);
	static std::string info_outputFilename;
	sub_info->add_option("-o,--output", info_outputFilename, "Output filename (+ path)")
		->check(FileExtensionValidator(".json"));
	sub_info->callback([&]() {
		optional<nlohmann::ordered_json> mapInfoJSON;
		std::shared_ptr<MapToolDebugLogger> logger;
		if (!info_outputFilename.empty())
		{
			logger = std::make_shared<MapToolDebugLogger>(new MapToolDebugLogger(verbose));
		}
		if (inputPathIsFile(info_inputMap))
		{
#if !defined(WZ_MAPTOOLS_DISABLE_ARCHIVE_SUPPORT)
			mapInfoJSON = generateMapInfoJSON_FromArchive(info_inputMap, logger);
#else
			std::cerr << "ERROR: maptools was compiled without support for .wz archives, and cannot open: " << info_inputMap << std::endl;
			retVal = 1;
#endif
		}
		else
		{
			mapInfoJSON = generateMapInfoJSON_FromPackageContents(info_inputMap, logger);
		}

		if (!mapInfoJSON.has_value())
		{
			retVal = 1;
			return;
		}

		std::string jsonStr = mapInfoJSON.value().dump(4, ' ', false, nlohmann::ordered_json::error_handler_t::ignore);

		if (!info_outputFilename.empty())
		{
			WzMap::StdIOProvider stdOutput;
			if (!stdOutput.writeFullFile(info_outputFilename, jsonStr.c_str(), static_cast<uint32_t>(jsonStr.size())))
			{
				std::cerr << "Failed to output JSON to: " << info_outputFilename << std::endl;
				retVal = 1;
				return;
			}
			std::cout << "Wrote output JSON to: " << info_outputFilename << std::endl;
		}
		else
		{
			std::cout << jsonStr << std::endl;
		}
	});
}

static void addSubCommand_Map(CLI::App& app, int& retVal, bool& verbose)
{
	CLI::App* sub_map = app.add_subcommand("map", "Manipulating a map folder");
	sub_map->fallthrough();

	// [CONVERTING MAP FORMAT]
	CLI::App* sub_convert = sub_map->add_subcommand("convert", "Convert a map from one format to another");
	sub_convert->fallthrough();
	static WzMap::MapType mapType = WzMap::MapType::SKIRMISH;
	sub_convert->add_option("-t,--maptype", mapType, "Map type")
		->transform(CLI::CheckedTransformer(maptype_map, CLI::ignore_case))
		->default_val(WzMap::MapType::SKIRMISH);
	static uint32_t mapMaxPlayers = 0;
	sub_convert->add_option("-p,--maxplayers", mapMaxPlayers, "Map max players")
		->required()
		->check(CLI::Range(1, 10));
	static WzMap::OutputFormat outputFormat = WzMap::LatestOutputFormat;
	sub_convert->add_option("-f,--format", outputFormat, "Output map format")
		->required()
		->transform(CLI::CheckedTransformer(outputformat_map, CLI::ignore_case).description("value in {\n\t\tbjo -> Binary .BJO (flaME-compatible / old),\n\t\tjson -> JSONv1 (WZ 3.4+),\n\t\tjsonv2 -> JSONv2 (WZ 4.1+),\n\t\tlatest -> " + CLI::detail::to_string(WzMap::LatestOutputFormat) + "}"));
	static std::string inputMapDirectory;
	sub_convert->add_option("-i,--input,inputmapdir", inputMapDirectory, "Input map directory")
		->required()
		->check(CLI::ExistingDirectory);
	static std::string outputMapDirectory;
	sub_convert->add_option("-o,--output,outputmapdir", outputMapDirectory, "Output map directory")
		->required()
		->check(CLI::ExistingDirectory);
	sub_convert->callback([&]() {
		if (!convertMap(mapType, mapMaxPlayers, inputMapDirectory, outputMapDirectory, outputFormat, verbose))
		{
			retVal = 1;
		}
	});

	// [GENERATING MAP PREVIEW PNG]
	CLI::App* sub_preview = sub_map->add_subcommand("genpreview", "Generate a map preview PNG");
	sub_preview->fallthrough();
	static WzMap::MapType preview_mapType = WzMap::MapType::SKIRMISH;
	sub_preview->add_option("-t,--maptype", preview_mapType, "Map type")
		->transform(CLI::CheckedTransformer(maptype_map, CLI::ignore_case))
		->default_val(WzMap::MapType::SKIRMISH);
	static uint32_t preview_mapMaxPlayers = 0;
	sub_preview->add_option("-p,--maxplayers", preview_mapMaxPlayers, "Map max players")
		->required()
		->check(CLI::Range(1, 10));
	static std::string preview_inputMapDirectory;
	sub_preview->add_option("-i,--input,inputmapdir", preview_inputMapDirectory, "Input map directory")
		->required()
		->check(CLI::ExistingDirectory);
	static std::string preview_outputPNGFilename;
	sub_preview->add_option("-o,--output,output", preview_outputPNGFilename, "Output PNG filename (+ path)")
		->required()
		->check(FileExtensionValidator(".png"));
	sub_preview->callback([&]() {
		if (!generateMapPreviewPNG_FromMapDirectory(preview_mapType, preview_mapMaxPlayers, preview_inputMapDirectory, preview_outputPNGFilename, verbose))
		{
			retVal = 1;
		}
	});
}

int main(int argc, char **argv)
{
	int retVal = 0;
	CLI::App app{"WZ2100 Map Tools"};
	bool verbose = false;
	app.add_flag("-v,--verbose", verbose, "Verbose output");

	addSubCommand_Package(app, retVal, verbose);
	addSubCommand_Map(app, retVal, verbose);

	CLI11_PARSE(app, argc, argv);
	return retVal;
}
