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
} // namespace WzMap

static bool convertMap(WzMap::MapType mapType, uint32_t mapMaxPlayers, const std::string& inputMapDirectory, const std::string& outputMapDirectory, WzMap::OutputFormat outputFormat, bool verbose)
{
	auto wzMap = WzMap::Map::loadFromPath(inputMapDirectory, mapType, mapMaxPlayers, rand(), false, std::unique_ptr<WzMap::LoggingProtocol>(new MapToolDebugLogger(verbose)));
	if (!wzMap)
	{
		// Failed to load map
		std::cerr << "Failed to load map: " << inputMapDirectory << std::endl;
		return false;
	}

	if (!wzMap->exportMapToPath(*(wzMap.get()), outputMapDirectory, mapType, mapMaxPlayers, outputFormat, std::unique_ptr<WzMap::LoggingProtocol>(new MapToolDebugLogger(verbose))))
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

static bool generateMapPreviewPNG(WzMap::MapType mapType, uint32_t mapMaxPlayers, const std::string& inputMapDirectory, const std::string& outputPNGPath, bool verbose)
{
	auto wzMap = WzMap::Map::loadFromPath(inputMapDirectory, mapType, mapMaxPlayers, rand(), false, std::unique_ptr<WzMap::LoggingProtocol>(new MapToolDebugLogger(verbose)));
	if (!wzMap)
	{
		// Failed to load map
		std::cerr << "Failed to load map: " << inputMapDirectory << std::endl;
		return false;
	}

	WzMap::MapPreviewColorScheme previewColorScheme;
	previewColorScheme.hqColor = {255, 0, 255, 255};
	previewColorScheme.oilResourceColor = {255, 255, 0, 255};
	previewColorScheme.oilBarrelColor = {128, 192, 0, 255};
	auto mapTilesetResult = guessMapTileset(*(wzMap.get()));
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

	auto previewResult = WzMap::generate2DMapPreview(*(wzMap.get()), previewColorScheme);
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

// specify string->value mappings
static const std::map<std::string, WzMap::MapType> maptype_map{{"skirmish", WzMap::MapType::SKIRMISH}, {"campaign", WzMap::MapType::CAMPAIGN}};
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

int main(int argc, char **argv)
{
	int retVal = 0;
	CLI::App app{"WZ2100 Map Tools"};
	bool verbose = false;
	app.add_flag("-v,--verbose", verbose, "Verbose output");

	// [CONVERTING MAP FORMAT]
	CLI::App* sub_convert = app.add_subcommand("convert", "Convert a map from one format to another");
	sub_convert->fallthrough();
	WzMap::MapType mapType = WzMap::MapType::SKIRMISH;
	sub_convert->add_option("-t,--maptype", mapType, "Map type")
		->transform(CLI::CheckedTransformer(maptype_map, CLI::ignore_case))
		->default_val(WzMap::MapType::SKIRMISH);
	uint32_t mapMaxPlayers = 0;
	sub_convert->add_option("-p,--maxplayers", mapMaxPlayers, "Map max players")
		->required()
		->check(CLI::Range(1, 10));
	WzMap::OutputFormat outputFormat = WzMap::LatestOutputFormat;
	sub_convert->add_option("-f,--format", outputFormat, "Output map format")
		->required()
		->transform(CLI::CheckedTransformer(outputformat_map, CLI::ignore_case).description("value in {\n\t\tbjo -> Binary .BJO (flaME-compatible / old),\n\t\tjson -> JSONv1 (WZ 3.4+),\n\t\tjsonv2 -> JSONv2 (WZ 4.1+),\n\t\tlatest -> " + CLI::detail::to_string(WzMap::LatestOutputFormat) + "}"));
	std::string inputMapDirectory;
	sub_convert->add_option("inputmapdir", inputMapDirectory, "Input map directory")
		->required()
		->check(CLI::ExistingDirectory);
	std::string outputMapDirectory;
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
	CLI::App* sub_preview = app.add_subcommand("genpreview", "Generate a map preview PNG");
	sub_preview->fallthrough();
	WzMap::MapType preview_mapType = WzMap::MapType::SKIRMISH;
	sub_preview->add_option("-t,--maptype", preview_mapType, "Map type")
		->transform(CLI::CheckedTransformer(maptype_map, CLI::ignore_case))
		->default_val(WzMap::MapType::SKIRMISH);
	uint32_t preview_mapMaxPlayers = 0;
	sub_preview->add_option("-p,--maxplayers", preview_mapMaxPlayers, "Map max players")
		->required()
		->check(CLI::Range(1, 10));
	std::string preview_inputMapDirectory;
	sub_preview->add_option("inputmapdir", preview_inputMapDirectory, "Input map directory")
		->required()
		->check(CLI::ExistingDirectory);
	std::string preview_outputPNGFilename;
	sub_preview->add_option("-o,--output,output", preview_outputPNGFilename, "Output PNG filename (+ path)")
		->required()
		->check(FileExtensionValidator(".png"));
	sub_preview->callback([&]() {
		if (!generateMapPreviewPNG(preview_mapType, preview_mapMaxPlayers, preview_inputMapDirectory, preview_outputPNGFilename, verbose))
		{
			retVal = 1;
		}
	});

	CLI11_PARSE(app, argc, argv);
	return retVal;
}
