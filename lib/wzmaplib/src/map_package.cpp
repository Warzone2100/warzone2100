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

#include "../include/wzmaplib/map.h"
#include "map_crc.h"
#include "map_script.h"
#include "map_internal.h"
#include "map_jsonhelpers.h"
#include "map_levparse.h"
#include "../include/wzmaplib/map_package.h"
#include <nlohmann/json.hpp>
#include <cinttypes>
#include <unordered_set>
#include <limits>
#include <algorithm>
#include "3rdparty/SDL_endian.h"
#include "../include/wzmaplib/map_version.h"

#if defined(_MSC_VER) && !(defined(__MINGW32__) || defined(__MINGW64__))
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif


// MARK: - Various helper functions

namespace WzMap {

static std::string gamFilePathFromMapFolderPath(const std::string& mapFolderPath, const std::string& pathSeparator = "/")
{
	// Remove any trailing slash(es) and append .gam on the end of the mapFolderPath
	std::string gamFilePath = mapFolderPath;
	while (!gamFilePath.empty() && strEndsWith(mapFolderPath, pathSeparator))
	{
		gamFilePath.pop_back();
	}
	if (!gamFilePath.empty())
	{
		gamFilePath += ".gam";
	}
	return gamFilePath;
}

static std::string getCurrentWzMapLibGeneratorName()
{
	return std::string("wzmaplib ") + WzMap::wzmaplib_version_string();
}

// Returns the ".gam" file path
// For an old-fashioned map, this should be a file that exists
// For a newer map, this points to a "virtual" .gam file that does not exist (but is in the parent folder of the map folder, with the same map name .gam)
std::string LevelDetails::gamFilePath() const
{
	std::string gamFilePath = gamFilePathFromMapFolderPath(mapFolderPath, "/");
	if (!gamFilePath.empty())
	{
		return gamFilePath;
	}
	else
	{
		// not sure if this can happen, but return something reasonable at least...
		if (!name.empty())
		{
			return name + ".gam";
		}
		else
		{
			return "map.gam";
		}
	}
}

void trimTechLevelFromMapName(std::string& mapName)
{
	size_t len = mapName.length();
	if (len > 2 && mapName[len - 3] == '-' && mapName[len - 2] == 'T' && isdigit(mapName[len - 1]))
	{
		mapName.resize(len - 3);
	}
}

static inline void trim_whitespace(std::string& str)
{
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int c) {
		return !std::isspace(c);
	}));
	str.erase(std::find_if(str.rbegin(), str.rend(), [](int c) {
		return !std::isspace(c);
	}).base(), str.end());
}

optional<MapType> mapTypeFromString(std::string typeStr, LoggingProtocol* pCustomLogger)
{
	if (typeStr.compare("skirmish") == 0)
	{
		return MapType::SKIRMISH;
	}
	else
	{
		debug(pCustomLogger, LOG_ERROR, "Unknown \"type\" value: %s", typeStr.c_str());
		return nullopt;
	}
}

std::string to_string(MapType mapType)
{
	switch (mapType)
	{
		case MapType::SKIRMISH:
			return "skirmish";
		case MapType::CAMPAIGN:
			return "campaign";
		case MapType::SAVEGAME:
			return "savegame";
	}
	return ""; // silence warning
}

optional<MAP_TILESET> levelTilesetFromString(std::string datasetStr, LoggingProtocol* pCustomLogger)
{
	if (datasetStr.compare("arizona") == 0)
	{
		return MAP_TILESET::ARIZONA;
	}
	else if (datasetStr.compare("urban") == 0)
	{
		return MAP_TILESET::URBAN;
	}
	else if (datasetStr.compare("rockies") == 0)
	{
		return MAP_TILESET::ROCKIES;
	}
	else
	{
		debug(pCustomLogger, LOG_ERROR, "Unknown \"tileset\" value: %s", datasetStr.c_str());
		return nullopt;
	}
}

std::string to_string(MAP_TILESET mapTileset)
{
	switch (mapTileset)
	{
		case MAP_TILESET::ARIZONA:
			return "arizona";
		case MAP_TILESET::URBAN:
			return "urban";
		case MAP_TILESET::ROCKIES:
			return "rockies";
	}
	return ""; // silence warning
}

std::string internal_baseDir(const std::string& fullPathToFile, IOProvider& mapIO, LoggingProtocol* pCustomLogger)
{
	const char* pathSeparator = mapIO.pathSeparator();
	if (pathSeparator == nullptr)
	{
		debug(pCustomLogger, LOG_ERROR, "IOProvider::pathSeparator returned null???");
		return "";
	}

	// Find the position of the last slash in the full path
	size_t lastSlash = fullPathToFile.find_last_of(pathSeparator, std::string::npos);
	if (lastSlash == std::string::npos)
	{
		// Did not find a path separator
		debug(pCustomLogger, LOG_INFO_VERBOSE, "Did not find parent path in \"%s\" (path separator: \"%s\")", fullPathToFile.c_str(), pathSeparator);
		// Default to an empty path
		return std::string();
	}

	// Trim off the last path component
	return fullPathToFile.substr(0, lastSlash);
}

// Load levels details from either:
// - a classic <map>.addon.lev / <map>.xplayers.lev file
// - the new (in-folder) level.json file
// NOTE: This function does *not* handle all of the special properties for campaign level types that are contained in .lev files (currently) - such as gamedesc.lev - it is intended for individual .lev files used by skirmish / multiplay maps
optional<LevelDetails> loadLevelDetails(const std::string& levelFile, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/, std::string* output_priorGenerator /*= nullptr*/)
{
	if (strEndsWith(levelFile, ".json"))
	{
		return loadLevelDetails_JSON(levelFile, mapIO, pCustomLogger, output_priorGenerator);
	}
	// Must do this strcasecmp comparison to match old behavior
	else if ((levelFile.length() > 10 && !strcasecmp(levelFile.c_str() + (levelFile.length() - 10), ".addon.lev"))
			 || (levelFile.length() > 13 && !strcasecmp(levelFile.c_str() + (levelFile.length() - 13), ".xplayers.lev")))
	{
		return loadLevelDetails_LEV(levelFile, mapIO, pCustomLogger);
	}
	debug(pCustomLogger, LOG_ERROR, "loadLevelDetails failure - unsupported file: %s", levelFile.c_str());
	return nullopt;
}

static bool convertDateSlashesToHyphens(std::string& dateString)
{
	// YYYY/MM/DD (...) to YYYY-MM-DD (...)
	if (dateString.size() < 10)
	{
		return false;
	}

	if (dateString[4] == '/' && dateString[7] == '/')
	{
		dateString[4] = '-';
		dateString[7] = '-';
		return true;
	}

	return false;
}

static char asciiToLower(char c)
{
	return (c >= 'A' && c <= 'Z') ? (c - ('Z' - 'z')) : c;
}

static std::string strAsciiToLower(const std::string& str)
{
	std::string result(str);
	std::transform(result.begin(), result.end(), result.begin(), asciiToLower);
	return result;
}

optional<LevelDetails> loadLevelDetails_LEV(const std::string& levelFile, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	// 1.) Load file
	std::vector<char> fileData;
	if (!mapIO.loadFullFile(levelFile, fileData))
	{
		return nullopt;
	}
	if (fileData.empty())
	{
		debug(pCustomLogger, LOG_ERROR, "Empty file: %s", levelFile.c_str());
		return nullopt;
	}

	const std::unordered_map<std::string, std::string> licenseToSPDX = {
		//	CC BY-SA 3.0 + GPL v2+
		{ "CC BY-SA 3.0 + GPL v2+", "CC-BY-SA-3.0 OR GPL-2.0-or-later" },
		//	CC BY 3.0 + GPL v2+
		{ "CC BY 3.0 + GPL v2+", "CC-BY-3.0 OR GPL-2.0-or-later" },
		//	GPL 2+
		{ "GPL 2+", "GPL-2.0-or-later" },
		//	CC0
		{ "CC0", "CC0-1.0" }
	};

	std::vector<LevelDetails> details;
	std::vector<bool> detailsFatalError;
	enum class KnownGenerators
	{
		FlaME,
		euphobos
	};
	optional<KnownGenerators> detectedGenerator;
	optional<std::string> generatorVersion;
	std::string createdDate;
	std::string uploadedDate;
	std::string author;
	std::string license;

	auto handleCommand = [&](const std::string& command, const std::string& arg) {

		if (command.compare("level") == 0)
		{
			details.push_back(LevelDetails());
			detailsFatalError.push_back(false);
			details.back().name = arg;
			details.back().createdDate = createdDate;
			details.back().uploadedDate = uploadedDate;
			details.back().author = author;
			details.back().license = license;
			details.back().generator = generatorVersion;
			return;
		}

		if (details.empty())
		{
			debug(pCustomLogger, LOG_WARNING, "LEV File Parse Error: `Syntax Error`: Received an unexpected token (prior to a `level` token): %s %s", command.c_str(), arg.c_str());
			return;
		}
		
		if (command.compare("players") == 0)
		{
			int playersValue = 0;
			try {
				playersValue = std::stoi(arg);
			}
			catch (const std::exception&) {
				debug(pCustomLogger, LOG_ERROR, "LEV File Parse Error: \"players\" value (\"%s\") must be a valid number; ignoring level[%zu](\"%s\")", arg.c_str(), details.size() - 1, details.back().name.c_str());
				detailsFatalError.back() = true;
				return;
			}
			if (static_cast<size_t>(playersValue) > static_cast<size_t>(std::numeric_limits<uint8_t>::max()))
			{
				debug(pCustomLogger, LOG_ERROR, "LEV File Parse Error: \"players\" value (\"%s\") is unsupported; ignoring level[%zu](\"%s\")", arg.c_str(), details.size() - 1, details.back().name.c_str());
				detailsFatalError.back() = true;
				return;
			}
			details.back().players = static_cast<uint8_t>(playersValue);
		}
		else if (command.compare("type") == 0)
		{
			int typeValue = 0;
			try {
				typeValue = std::stoi(arg);
			}
			catch (const std::exception&) {
				debug(pCustomLogger, LOG_ERROR, "LEV File Parse Error: \"type\" value (\"%s\") must be a valid number; ignoring level[%zu](\"%s\")", arg.c_str(), details.size() - 1, details.back().name.c_str());
				detailsFatalError.back() = true;
				return;
			}
			// Check for valid / supported type numbers from old .lev files
			if (typeValue == static_cast<int>(SUPPORTED_LEVEL_TYPES::SKIRMISH)
				|| typeValue == static_cast<int>(SUPPORTED_LEVEL_TYPES::MULTI_SKIRMISH2)
				|| typeValue == static_cast<int>(SUPPORTED_LEVEL_TYPES::MULTI_SKIRMISH3)
				|| typeValue == static_cast<int>(SUPPORTED_LEVEL_TYPES::MULTI_SKIRMISH4))
			{
				details.back().type = MapType::SKIRMISH;
			}
			else
			{
				debug(pCustomLogger, LOG_ERROR, "LEV File Parse Error: \"type\" value (\"%s\") is unknown / unsupported; ignoring level[%zu](\"%s\")", arg.c_str(), details.size() - 1, details.back().name.c_str());
				detailsFatalError.back() = true;
				return;
			}
		}
		else if (command.compare("dataset") == 0)
		{
			auto tileset = convertLevMapTilesetType(arg);
			if (!tileset.has_value())
			{
				debug(pCustomLogger, LOG_ERROR, "LEV File Parse Error: \"dataset\" value (\"%s\") is unknown / unsupported; ignoring level[%zu](\"%s\")", arg.c_str(), details.size() - 1, details.back().name.c_str());
				detailsFatalError.back() = true;
				return;
			}
			details.back().tileset = tileset.value();
		}
		else if (command.compare("game") == 0)
		{
			// Remove the ".gam" from the end of the filename
			details.back().mapFolderPath = arg;
			if (strEndsWith(arg, ".gam"))
			{
				details.back().mapFolderPath = details.back().mapFolderPath.substr(0, details.back().mapFolderPath.length() - 4);
			}
			else
			{
				debug(pCustomLogger, LOG_WARNING, "LEV File Parse Warning: Unexpected: \"game\" command argument (\"%s\") does not end with .gam", arg.c_str());
			}
		}
		else if (command.compare("data") == 0)
		{
			// explicitly ignore `data` lines
			return;
		}
		else
		{
			debug(pCustomLogger, LOG_WARNING, "LEV File Parse Error: `Syntax Error`: Unknown / unsupported token: %s", command.c_str());
		}
	};

	auto handleCommentLine = [&](const std::string& commentLine) {

		// Trim all whitespace
		std::string comment = commentLine;
		trim_whitespace(comment);

		// Detect generator comment
		if (strAsciiToLower(comment).rfind("made with flame", 0) == 0)
		{
			if (!detectedGenerator.has_value())
			{
				detectedGenerator = KnownGenerators::FlaME;
				generatorVersion = comment.substr(10); // after "Made with "
			}
			else
			{
				// multiple generator comments??
				debug(pCustomLogger, LOG_WARNING, "LEV File Parse Warning: Additional comment detected that could signify a generator (already found one) - ignoring: \"%s\"", comment.c_str());
			}
			return;
		}
		else if (comment.rfind("Generated by map database at ", 0) == 0)
		{
			if (!detectedGenerator.has_value())
			{
				detectedGenerator = KnownGenerators::euphobos;
			}
			else
			{
				// multiple generator comments??
				debug(pCustomLogger, LOG_WARNING, "LEV File Parse Warning: Additional comment detected that could signify a generator (already found one) - ignoring: \"%s\"", comment.c_str());
			}
			return;
		}

		// Handle extracting additional info from comments
		if (detectedGenerator == KnownGenerators::FlaME)
		{
			// Comment fields of interest are:
			// Date: YYYY/MM/DD HH:MM:SS
			// Author: AUTHORNAME
			// License: (see known options in licenseToSPDX above)
			if (comment.rfind("Date: ", 0) == 0)
			{
				if (!createdDate.empty())
				{
					debug(pCustomLogger, LOG_ERROR, "Duplicate `Date` comment in .lev file: \"%s\"", comment.c_str());
					return;
				}
				createdDate = comment.substr(6);
				trim_whitespace(createdDate);

				// Attempt to convert from YYYY/MM/DD HH:MM:SS to YYYY-MM-DD HH:MM:SS format (where HH:MM:SS is optional)
				convertDateSlashesToHyphens(createdDate);
				return;
			}
			// additional fields handled below
		}
		else if (detectedGenerator == KnownGenerators::euphobos)
		{
			// Comment fields of interest are:
			// Upload date: YYYY-MM-DD HH:MM:SS +0000
			// Author: AUTHORNAME
			// License: (see known options in licenseToSPDX above)

			if (comment.rfind("Upload date: ", 0) == 0)
			{
				if (!uploadedDate.empty())
				{
					debug(pCustomLogger, LOG_ERROR, "Duplicate `Upload date` comment in .lev file: \"%s\"", comment.c_str());
					return;
				}
				uploadedDate = comment.substr(13);
				trim_whitespace(uploadedDate);
				return;
			}
			// additional fields handled below
		}

		if (comment.rfind("Author: ", 0) == 0)
		{
			if (!author.empty())
			{
				debug(pCustomLogger, LOG_ERROR, "Duplicate `Author` comment in .lev file: \"%s\"", comment.c_str());
				return;
			}
			author = comment.substr(8);
			trim_whitespace(author);
		}
		else if (comment.rfind("License: ", 0) == 0)
		{
			if (!license.empty())
			{
				debug(pCustomLogger, LOG_ERROR, "Duplicate `License` comment in .lev file: \"%s\"", comment.c_str());
				return;
			}
			std::string licenseIdentifier = comment.substr(9);
			trim_whitespace(licenseIdentifier);
			auto licenseMapping = licenseToSPDX.find(licenseIdentifier);
			if (licenseMapping != licenseToSPDX.end())
			{
				license = licenseMapping->second;
			}
			else
			{
				debug(pCustomLogger, LOG_ERROR, "Unknown / unsupported license in .lev file: \"%s\"", license.c_str());
			}
		}
	};

	if (!levParseBasic(fileData.data(), fileData.size(), mapIO, handleCommand, handleCommentLine, pCustomLogger))
	{
		debug(pCustomLogger, LOG_ERROR, "Failed to parse .lev file: %s", levelFile.c_str());
		return nullopt;
	}

	if (details.empty())
	{
		return nullopt;
	}
	else if (details.size() == 1)
	{
		// Remove any tech level suffix from name of first entry
		trimTechLevelFromMapName(details.front().name);

		return details.front();
	}
	else
	{
		// more than one level entry in the file

		// find first level entry that is valid
		optional<size_t> firstValidLevelEntryIdxOpt;
		for (size_t idx = 0; idx < detailsFatalError.size(); idx++)
		{
			if (!detailsFatalError[idx])
			{
				firstValidLevelEntryIdxOpt = idx;
				break;
			}
		}
		if (!firstValidLevelEntryIdxOpt.has_value())
		{
			// no valid level entries - all had fatal errors associated with them!
			debug(pCustomLogger, LOG_ERROR, "No valid / supported levels found in .lev file");
			return nullopt;
		}
		size_t firstValidLevelEntryIdx = firstValidLevelEntryIdxOpt.value();

		// check that they all point to the same map (we don't support multi-map packs here, which have long been deprecated)
		std::string firstMapFolderPath = details[firstValidLevelEntryIdx].mapFolderPath;
		for (size_t idx = 0; idx < details.size(); idx++)
		{
			if (detailsFatalError[idx])
			{
				continue; // skip problematic levels
			}
			if (details[idx].mapFolderPath != firstMapFolderPath)
			{
				// Found map pack - unsupported!
				debug(pCustomLogger, LOG_ERROR, "Found map pack (unsupported!) - level map path (%s) does not match first level map path: (%s)", details[idx].mapFolderPath.c_str(), firstMapFolderPath.c_str());
				return nullopt;
			}
		}

		// otherwise, everything was pointing to the same map, so these were probably the old format that listed all the tech levels separately
		// (which is no longer needed)

		// Remove any tech level suffix from name of first entry
		trimTechLevelFromMapName(details[firstValidLevelEntryIdx].name);

		return details[firstValidLevelEntryIdx];
	}
}

optional<LevelDetails> loadLevelDetails_JSON(const std::string& levelJsonFile, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/, std::string* output_priorGenerator /*= nullptr*/)
{
	auto loadedResult = loadJsonObjectFromFile(levelJsonFile, mapIO, pCustomLogger);
	if (!loadedResult.has_value())
	{
		// Failed to load JSON - rely on loadJsonFromFile to handle output of errors
		return nullopt;
	}

	debug(pCustomLogger, LOG_INFO, "Loading: %s", levelJsonFile.c_str());

	nlohmann::json& mRoot = loadedResult.value();

//	{
//	  "name": "NoOilNoProblems",
//	  "type": <"skirmish", ...>
//	  "players": 4,
//	  "tileset": <"arizona", "urban", "rockies">,

//
//	  "author": {
//		  "name": "AUTHOR"
//	  },
//	  "spdx-license-id": "CC-BY-SA-3.0 OR GPL-2.0-or-later",
//	  "created-date": "YYYY-MM-DD",
//	  "generator": "wzmaplib 1.0.0",

//	  // (other optional fields for maps)
//	  "additional-authors": [{"name": "AUTHOR2" }]

//	  // (other optional fields for converted maps)
//	  "prior-generator": "FlaME 1.28 Windows"
//	}

	LevelDetails details;

	try {
		// Get required properties
		details.name = mRoot.at("name").get<std::string>();
		auto typeOpt = mapTypeFromString(mRoot.at("type").get<std::string>(), pCustomLogger);
		if (!typeOpt.has_value())
		{
			// levelTypeFromString already handled logging an error
			return nullopt;
		}
		details.type = typeOpt.value();
		auto playersNum = mRoot.at("players").get<unsigned>();
		if (playersNum > static_cast<unsigned>(std::numeric_limits<uint8_t>::max()))
		{
			debug(pCustomLogger, LOG_ERROR, "Invalid \"players\" value: %u", playersNum);
			return nullopt;
		}
		details.players = static_cast<uint8_t>(playersNum);
		auto tilesetOpt = levelTilesetFromString(mRoot.at("tileset").get<std::string>(), pCustomLogger);
		if (!tilesetOpt.has_value())
		{
			// levelTypeFromString already handled logging an error
			return nullopt;
		}
		details.tileset = tilesetOpt.value();

		// mapFolderPath = the path to this levelJsonFile with the last path component (the json filename) removed
		details.mapFolderPath = internal_baseDir(levelJsonFile, mapIO, pCustomLogger);
	}
	catch (const std::exception &e) {
		debug(pCustomLogger, LOG_ERROR, "JSON document from %s is invalid: %s", levelJsonFile.c_str(), e.what());
		return nullopt;
	}

	// Get optional info
	auto it = mRoot.find("author");
	if (it != mRoot.end())
	{
		auto nameIt = it->find("name");
		if (nameIt != it->end())
		{
			if (nameIt->is_string())
			{
				details.author = nameIt->get<std::string>();
			}
		}
	}
	it = mRoot.find("created-date");
	if (it != mRoot.end())
	{
		if (it->is_string())
		{
			details.createdDate = it->get<std::string>();
		}
	}
	it = mRoot.find("spdx-license-id");
	if (it != mRoot.end())
	{
		if (it->is_string())
		{
			details.license = it->get<std::string>();
		}
	}
	it = mRoot.find("generator");
	if (it != mRoot.end())
	{
		if (it->is_string())
		{
			details.generator = it->get<std::string>();
		}
	}
	it = mRoot.find("additional-authors");
	if (it != mRoot.end())
	{
		if (it->is_array())
		{
			for (auto otherAuthorIt = it->begin(); otherAuthorIt != it->end(); ++otherAuthorIt)
			{
				if (!otherAuthorIt->is_object()) { continue; }
				auto nameIt = otherAuthorIt->find("name");
				if (nameIt != otherAuthorIt->end())
				{
					if (nameIt->is_string())
					{
						details.additionalAuthors.push_back(nameIt->get<std::string>());
					}
				}
			}
		}
		else
		{
			debug(pCustomLogger, LOG_WARNING, "JSON document has additional-authors key that is not an array: %s", levelJsonFile.c_str());
		}
	}
	if (output_priorGenerator)
	{
		it = mRoot.find("prior-generator");
		if (it != mRoot.end())
		{
			if (it->is_string())
			{
				*output_priorGenerator = it->get<std::string>();
			}
		}
	}

	return details;
}

bool exportLevelDetails_LEV(const LevelDetails& details, const std::string& outputPath, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	const std::unordered_map<std::string, std::string> SPDXToFlaMELicenseField = {
		//	CC BY-SA 3.0 + GPL v2+
		{ "CC-BY-SA-3.0 OR GPL-2.0-or-later", "CC BY-SA 3.0 + GPL v2+" },
		//	CC BY 3.0 + GPL v2+
		{ "CC-BY-3.0 OR GPL-2.0-or-later", "CC BY 3.0 + GPL v2+" },
		//	GPL 2+
		{ "GPL-2.0-or-later", "GPL 2+" },
		//	CC0
		{ "CC0-1.0", "CC0" }
	};

	auto currentGenerator = getCurrentWzMapLibGeneratorName();

	std::string output;
	output = "// Made with FlaME 1.29 - (compatible / " + currentGenerator + ")\n";
	output += "// Date: " + details.createdDate + "\n";
	output += "// Author: " + details.author + "\n";
	if (!details.additionalAuthors.empty())
	{
		output += "// Additional Authors: ";
		output += std::accumulate(std::next(details.additionalAuthors.begin()), details.additionalAuthors.end(), std::string(details.additionalAuthors[0]),
			[](std::string a, const std::string &b)
			{
				return std::move(a) + "," + b;
			});
		output += "\n";
	}
	if (!details.license.empty())
	{
		auto it = SPDXToFlaMELicenseField.find(details.license);
		if (it != SPDXToFlaMELicenseField.end())
		{
			output += "// License: " + it->second + "\n";
		}
		else
		{
			debug(pCustomLogger, LOG_WARNING, "LevelDetails contains a license with unknown mapping to FlaME license options (will use it verbatim): \"%s\"", details.license.c_str());
			output += "// License: " + details.license + "\n";
		}
	}
	else
	{
		debug(pCustomLogger, LOG_WARNING, "LevelDetails is missing a valid license");
	}
	if (details.generator.has_value() && !details.generator.value().empty() && details.generator.value() != currentGenerator)
	{
		output += "// Prior Generator: " + details.generator.value() + "\n";
	}

	output += "\n";

	if (details.type == MapType::SKIRMISH)
	{
		// To match old FlaME behavior, output an entry for each tech level T1-T3
		for (unsigned techLevel = 1; techLevel <= 3; techLevel++)
		{
			output += "level " + details.name + "-T" + std::to_string(techLevel) + "\n";
			output += "players " + std::to_string(details.players) + "\n";
			auto typeInt = mapTypeToClassicLevType(details.type, techLevel, pCustomLogger);
			if (!typeInt.has_value())
			{
				debug(pCustomLogger, LOG_ERROR, "MapType (%s) / techLevel (%u) combination is not yet supported by mapTypeToClassicLevType", to_string(details.type).c_str(), techLevel);
				return false;
			}
			output += "type " + std::to_string(typeInt.value()) + "\n";
			auto dataSetStr = mapTilesetToClassicLevType(details.type, details.tileset, techLevel, pCustomLogger);
			if (!dataSetStr.has_value())
			{
				debug(pCustomLogger, LOG_ERROR, "MapType is not yet supported by mapTilesetToClassicLevType: %s", to_string(details.type).c_str());
				return false;
			}
			output += "dataset " + dataSetStr.value() + "\n";
			output += "game \"" + details.gamFilePath() + "\"\n";
		}
	}
	else
	{
		debug(pCustomLogger, LOG_ERROR, "Map type (%s) is not currently supported for lev file export", to_string(details.type).c_str());
	}

	return mapIO.writeFullFile(mapIO.pathJoin(outputPath, details.name + ".xplayers.lev"), output.c_str(), static_cast<uint32_t>(output.size()));
}

bool exportLevelDetails_JSON(const LevelDetails& details, const std::string& outputPath, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	nlohmann::ordered_json output = nlohmann::ordered_json::object();
	output["name"] = details.name;
	output["type"] = to_string(details.type);
	output["players"] = details.players;
	output["tileset"] = to_string(details.tileset);
	if (!details.author.empty())
	{
		nlohmann::ordered_json authorinfo = nlohmann::ordered_json::object();
		authorinfo["name"] = details.author;
		output["author"] = authorinfo;
	}
	else
	{
		debug(pCustomLogger, LOG_WARNING, "LevelDetails is missing a valid author");
	}
	if (!details.license.empty())
	{
		output["spdx-license-id"] = details.license;
	}
	else
	{
		debug(pCustomLogger, LOG_WARNING, "LevelDetails is missing a valid license");
	}
	if (!details.createdDate.empty())
	{
		output["created-date"] = details.createdDate;
	}
	else
	{
		debug(pCustomLogger, LOG_WARNING, "LevelDetails is missing a created-date");
	}
	if (!details.additionalAuthors.empty())
	{
		nlohmann::json additionalAuthorsArray = nlohmann::json::array();
		for (const auto& author : details.additionalAuthors)
		{
			nlohmann::ordered_json authorinfo = nlohmann::ordered_json::object();
			authorinfo["name"] = author;
			additionalAuthorsArray.push_back(std::move(authorinfo));
		}
		output["additional-authors"] = additionalAuthorsArray;
	}
	auto currentGenerator = getCurrentWzMapLibGeneratorName();
	output["generator"] = currentGenerator;
	if (details.generator.has_value() && !details.generator.value().empty() && details.generator.value() != currentGenerator)
	{
		output["prior-generator"] = details.generator.value();
	}

	std::string jsonStr = output.dump(4, ' ', false, nlohmann::ordered_json::error_handler_t::ignore);
#if SIZE_MAX > UINT32_MAX
	if (jsonStr.size() > static_cast<size_t>(std::numeric_limits<uint32_t>::max()))
	{
		debug(pCustomLogger, LOG_ERROR, "jsonString.size (%zu) exceeds uint32_t::max", jsonStr.size());
		return false;
	}
#endif
	return mapIO.writeFullFile(mapIO.pathJoin(outputPath, "level.json"), jsonStr.c_str(), static_cast<uint32_t>(jsonStr.size()));
}

// Export level details to a specified folder in a specified output level format
bool exportLevelDetails(const LevelDetails& details, LevelFormat format, const std::string& outputPath, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	switch (format)
	{
		case LevelFormat::LEV:
			return exportLevelDetails_LEV(details, outputPath, mapIO, pCustomLogger);
		case LevelFormat::JSON:
			return exportLevelDetails_JSON(details, outputPath, mapIO, pCustomLogger);
	}

	return false; // silence warning
}

// MARK: -

static optional<GamInfo> loadGamFile_OldBinary(const std::string& filePath, IOProvider& mapIO, LoggingProtocol* pCustomLogger)
{
	auto readStream = mapIO.openBinaryStream(filePath, BinaryIOStream::OpenMode::READ);
	if (!readStream)
	{
		// Failed to open binary stream
		return nullopt;
	}

	// get the file header
	char aFileType[4] = {};
	uint32_t version = 0;

	if (readStream->readBytes(aFileType, 4) != static_cast<size_t>(4))
	{
		return nullopt;
	}

	if (aFileType[0] != 'g'
		|| aFileType[1] != 'a'
		|| aFileType[2] != 'm'
		|| aFileType[3] != 'e')
	{
		// Invalid header
		return nullopt;
	}

	// All binary save game file versions below version 35 (i.e. _not_ version 35 itself)
	// have their version numbers stored as little endian. Versions from 35 and
	// onward use big-endian. This basically means that, because of endian
	// swapping, numbers from 35 and onward will be ridiculously high if a
	// little-endian byte-order is assumed.

	// So first attempt reading it as little endian
	if (!readStream->readULE32(&version))
	{
		return nullopt;
	}

	if (version > 34)
	{
		// Apparently we get a larger number than expected if using little-endian.
		// So assume we have a version of 35 and onward

		// Reverse the little-endian decoding
		version = SDL_Swap32(version);
	}

	GamInfo gamInfo;

	// All binary gam files from version 34 or before are little endian.
	// All from version 35, are big-endian.

	if (!(readStream->readULE32(&gamInfo.gameTime)
			&& readStream->readULE32(&gamInfo.GameType)
			&& readStream->readSLE32(&gamInfo.ScrollMinX)
			&& readStream->readSLE32(&gamInfo.ScrollMinY)
			&& readStream->readULE32(&gamInfo.ScrollMaxX)
			&& readStream->readULE32(&gamInfo.ScrollMaxY)
			&& readStream->readBytes(gamInfo.levelName, OLD_MAX_LEVEL_SIZE) == OLD_MAX_LEVEL_SIZE))
	{
		// Failed to read basic data
		return nullopt;
	}

	if (version > 34)
	{
		// Reverse the little-endian decoding
		gamInfo.gameTime = SDL_Swap32(gamInfo.gameTime);
		gamInfo.GameType = SDL_Swap32(gamInfo.GameType);
		gamInfo.ScrollMinX = SDL_Swap32(gamInfo.ScrollMinX);
		gamInfo.ScrollMinY = SDL_Swap32(gamInfo.ScrollMinY);
		gamInfo.ScrollMaxX = SDL_Swap32(gamInfo.ScrollMaxX);
		gamInfo.ScrollMaxY = SDL_Swap32(gamInfo.ScrollMaxY);
	}

	return gamInfo;
}

static optional<GamInfo> loadGamFile_JSON(const std::string& filePath, IOProvider& mapIO, LoggingProtocol* pCustomLogger)
{
	auto loadedResult = loadJsonObjectFromFile(filePath, mapIO, pCustomLogger);
	if (!loadedResult.has_value())
	{
		// Failed to load JSON - rely on loadJsonFromFile to handle output of errors
		return nullopt;
	}

	debug(pCustomLogger, LOG_INFO, "Loading: %s", filePath.c_str());

	nlohmann::json& mRoot = loadedResult.value();
	GamInfo gamInfo;
	uint32_t version = 0;

	try {
		// Get required properties
		version = mRoot.at("version").get<uint32_t>();
		if (version < 7)
		{
			debug(pCustomLogger, LOG_WARNING, "Unexpected gam.json version (%" PRIu32 ") in: %s", version, filePath.c_str());
		}
		gamInfo.gameTime = mRoot.at("gameTime").get<uint32_t>();
		gamInfo.GameType = mRoot.at("GameType").get<uint32_t>();
		gamInfo.ScrollMinX = mRoot.at("ScrollMinX").get<int32_t>();
		gamInfo.ScrollMinY = mRoot.at("ScrollMinY").get<int32_t>();
		gamInfo.ScrollMaxX = mRoot.at("ScrollMaxX").get<uint32_t>();
		gamInfo.ScrollMaxY = mRoot.at("ScrollMaxY").get<uint32_t>();
		auto levelNameStr = mRoot.at("levelName").get<std::string>();
		strncpy(gamInfo.levelName, levelNameStr.c_str(), OLD_MAX_LEVEL_SIZE - 1);
		gamInfo.levelName[OLD_MAX_LEVEL_SIZE - 1] = '\0';
	}
	catch (const std::exception &e) {
		debug(pCustomLogger, LOG_ERROR, "JSON document from %s is invalid: %s", filePath.c_str(), e.what());
		return nullopt;
	}

	return gamInfo;
}

// Load map GAM details from either:
// - a classic <map>.gam file
// - the new (in-folder) gam.json file
// NOTE: This function only handles the basic gam data used by maps
optional<GamInfo> loadGamFile(const std::string& gamFile, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	if (strEndsWith(gamFile, "gam.json"))
	{
		return loadGamFile_JSON(gamFile, mapIO, pCustomLogger);
	}
	else if (strEndsWith(gamFile, ".gam"))
	{
		return loadGamFile_OldBinary(gamFile, mapIO, pCustomLogger);
	}
	debug(pCustomLogger, LOG_ERROR, "loadGamInfo failure - unsupported file: %s", gamFile.c_str());
	return nullopt;
}

static bool writeGamFile_OldBinary(const std::string& filePath, const GamInfo& gamInfo, IOProvider& exportIO, LoggingProtocol* logger)
{
	auto writeStream = exportIO.openBinaryStream(filePath, BinaryIOStream::OpenMode::WRITE);
	if (!writeStream)
	{
		// Failed to open binary stream
		return false;
	}

	// serialize gam file header ('game')
	char aFileType[4] = {'g', 'a', 'm', 'e'};
	uint32_t version = 8; // use 8 to be FlaME-compatible
	if (writeStream->writeBytes(aFileType, 4) != static_cast<size_t>(4))
	{
		return false;
	}

	// Write version numbers below version 35 as little-endian, and those above as big-endian
//	if (version < 35) // uncomment this if version is ever >= 35
//	{
		if (!writeStream->writeULE32(version))
		{
			return false;
		}

		return (writeStream->writeULE32(gamInfo.gameTime)
				&& writeStream->writeULE32(gamInfo.GameType)
				&& writeStream->writeSLE32(gamInfo.ScrollMinX)
				&& writeStream->writeSLE32(gamInfo.ScrollMinY)
				&& writeStream->writeULE32(gamInfo.ScrollMaxX)
				&& writeStream->writeULE32(gamInfo.ScrollMaxY)
				&& writeStream->writeBytes(gamInfo.levelName, OLD_MAX_LEVEL_SIZE) == OLD_MAX_LEVEL_SIZE);
//	}
//	else	// uncomment this if version is ever >= 35
//	{
//		if (!writeStream->writeUBE32(version))
//		{
//			return false;
//		}
//
//		return (writeStream->writeUBE32(gamInfo.gameTime)
//				&& writeStream->writeUBE32(gamInfo.GameType)
//				&& writeStream->writeSBE32(gamInfo.ScrollMinX)
//				&& writeStream->writeSBE32(gamInfo.ScrollMinY)
//				&& writeStream->writeUBE32(gamInfo.ScrollMaxX)
//				&& writeStream->writeUBE32(gamInfo.ScrollMaxY)
//				&& writeStream->writeBytes(gamInfo.levelName, OLD_MAX_LEVEL_SIZE) == OLD_MAX_LEVEL_SIZE);
//	}
}

static bool writeGamFile_JSON(const std::string& filePath, const GamInfo& gamInfo, IOProvider& exportIO, LoggingProtocol* logger)
{
	nlohmann::ordered_json o = nlohmann::ordered_json::object();
	o["version"] = 7;
	o["gameTime"] = gamInfo.gameTime;
	o["GameType"] = gamInfo.GameType;
	o["ScrollMinX"] = gamInfo.ScrollMinX;
	o["ScrollMinY"] = gamInfo.ScrollMinY;
	o["ScrollMaxX"] = gamInfo.ScrollMaxX;
	o["ScrollMaxY"] = gamInfo.ScrollMaxY;
	o["levelName"] = "";
	std::string jsonStr = o.dump(4, ' ', false, nlohmann::ordered_json::error_handler_t::ignore);
	return exportIO.writeFullFile(filePath, jsonStr.c_str(), static_cast<uint32_t>(jsonStr.size()));
}

static bool writeGamFile(const std::string& fullPathToOutputMapFolder, LevelFormat format, const GamInfo& gamInfo, IOProvider& exportIO, LoggingProtocol* logger)
{
	switch (format)
	{
		case LevelFormat::LEV:
			return writeGamFile_OldBinary(gamFilePathFromMapFolderPath(fullPathToOutputMapFolder, exportIO.pathSeparator()), gamInfo, exportIO, logger);
		case LevelFormat::JSON:
			return writeGamFile_JSON(exportIO.pathJoin(fullPathToOutputMapFolder, "gam.json"), gamInfo, exportIO, logger);
	}

	return false; // silence warning
}

static bool writeGamFileForMapExport(const std::string& fullPathToOutputMapFolder, LevelFormat levelFormat, GamInfo gamInfo, MapData& mapData, IOProvider& exportIO, LoggingProtocol* logger)
{
	if (gamInfo.ScrollMaxX == 0)
	{
		// use the map size
		gamInfo.ScrollMaxX = mapData.width;
	}
	if (gamInfo.ScrollMaxY == 0)
	{
		// use the map size
		gamInfo.ScrollMaxY = mapData.height;
	}
	if (gamInfo.ScrollMaxX > mapData.width)
	{
		debug(logger, LOG_WARNING, "ScrollMaxX was too big (%" PRIu32 ") - It has been set to the map width", gamInfo.ScrollMaxX);
		gamInfo.ScrollMaxX = mapData.width;
	}
	if (gamInfo.ScrollMaxY > mapData.height)
	{
		debug(logger, LOG_WARNING, "ScrollMaxY was too big (%" PRIu32 ") - It has been set to the map height", gamInfo.ScrollMaxY);
		gamInfo.ScrollMaxY = mapData.height;
	}
	return writeGamFile(fullPathToOutputMapFolder, levelFormat, gamInfo, exportIO, logger);
}


// MARK: - High-level interface for loading / saving a map package (ex. archive)

MapPackage::MapPackage()
{ }

// Load a map package from a specified path, optionally supplying:
// - a logger
// - a WzMap::IOProvider
//
// The default StdIOProvider will assume pathToMapPackage is a path to an extracted map package (i.e. standard filesystem I/O)
//
// To load from an archive (.zip/.wz), create a custom implementation of WzMap::IOProvider that supports compressed archive files that you initialize with the path to the zip. An example of this (which uses libzip) is available in `plugins/ZipIOProvider`. You would then set `pathToMapPackage` to be the root path inside the zip. (In the case of plugins\ZipIOProvider, literally `/`).
std::unique_ptr<MapPackage> MapPackage::loadPackage(const std::string& pathToMapPackage, std::shared_ptr<LoggingProtocol> logger /*= nullptr*/, std::shared_ptr<IOProvider> pMapIO /*= std::shared_ptr<IOProvider>(new StdIOProvider())*/)
{
	LoggingProtocol* pCustomLogger = logger.get();
	if (!pMapIO)
	{
		debug(pCustomLogger, LOG_ERROR, "Null IOProvider");
		return nullptr;
	}

	IOProvider& mapIO = *pMapIO;

	// 1.) Look for a map folder
	// i.e. multiplay/maps/<map name>
	std::vector<std::string> mapFolderNames;
	std::string pathSeparator = mapIO.pathSeparator();
	std::string baseMapsPath = mapIO.pathJoin(pathToMapPackage, mapIO.pathJoin("multiplay", "maps"));
	bool enumSuccess = mapIO.enumerateFolders(baseMapsPath, [&mapFolderNames, baseMapsPath, pathSeparator](const char *pMapName) -> bool {
		if (!pMapName) { return true; }
		if (*pMapName == '\0') { return true; }
		mapFolderNames.push_back(baseMapsPath + pathSeparator + pMapName);
		return true; // continue enumerating
	});

	if (enumSuccess && !mapFolderNames.empty())
	{
		if (mapFolderNames.size() > 1)
		{
			// multi-map packs are not supported (deprecated for a while)
			debug(pCustomLogger, LOG_ERROR, "Detected multiple (%zu) map folders - multi-map packs are not supported", mapFolderNames.size());
			return nullptr;
		}

		// check the map folder for a level.json file
		auto loadedLevelDetails = loadLevelDetails_JSON(mapIO.pathJoin(mapFolderNames[0], "level.json"), mapIO, pCustomLogger);
		if (loadedLevelDetails.has_value())
		{
			// Successfully found a level.json file inside the enumerated map folder
			std::unique_ptr<MapPackage> result = std::unique_ptr<MapPackage>(new MapPackage());
			result->m_levelDetails = loadedLevelDetails.value();
			result->m_loadedLevelFormat = LevelFormat::JSON;
			result->m_mapType = result->m_levelDetails.type;
			result->m_pathToMapPackage = pathToMapPackage;
			result->m_mapIO = pMapIO;
			result->loadGamInfo();
			return result;
		}
	}

	// 2.) Look for a level.json file in the root (i.e. a "flattened" plain map package)
	auto loadedFlatLevelDetails = loadLevelDetails_JSON(mapIO.pathJoin(pathToMapPackage, "level.json"), mapIO, pCustomLogger);
	if (loadedFlatLevelDetails.has_value())
	{
		// Successfully found a level.json file inside the root folder
		std::unique_ptr<MapPackage> result = std::unique_ptr<MapPackage>(new MapPackage());
		result->m_levelDetails = loadedFlatLevelDetails.value();
		result->m_levelDetails.mapFolderPath.clear();
		result->m_loadedLevelFormat = LevelFormat::JSON;
		result->m_mapType = result->m_levelDetails.type;
		result->m_pathToMapPackage = pathToMapPackage;
		result->m_mapIO = pMapIO;
		result->m_flatMapPackage = true;
		result->loadGamInfo();
		return result;
	}

	// 3.) If no map folder with a valid level.json was found, nor a level.json in the root, look for a .lev file in the root.
	std::vector<std::string> rootLevFiles;
	enumSuccess = mapIO.enumerateFiles(pathToMapPackage, [&rootLevFiles](const char *file) -> bool {
		if (!file) { return true; }
		if (*file == '\0') { return true; }
		size_t len = strlen(file);
		if ((len > 10 && !strcasecmp(file + (len - 10), ".addon.lev"))
			|| (len > 13 && !strcasecmp(file + (len - 13), ".xplayers.lev")))
		{
			rootLevFiles.push_back(file);
		}
		return true; // continue enumerating
	});

	if (!enumSuccess)
	{
		debug(pCustomLogger, LOG_ERROR, "Failed to enumerate files");
		return nullptr;
	}

	if (rootLevFiles.empty())
	{
		debug(pCustomLogger, LOG_ERROR, "Unable to detect any in-folder map level.json files (new format), or root .lev files (older format).");
		return nullptr;
	}

	std::string levFilePath = pathToMapPackage;
	if (!levFilePath.empty() && !strEndsWith(levFilePath, pathSeparator))
	{
		levFilePath += pathSeparator;
	}
	for (size_t idx = 0; idx < rootLevFiles.size(); idx++)
	{
		const auto& levFile = rootLevFiles[idx];
		std::string originalGenerator;
		auto loadedLevelDetails = loadLevelDetails_LEV(levFilePath + levFile, mapIO, pCustomLogger);
		if (loadedLevelDetails.has_value())
		{
			enumSuccess = mapIO.enumerateFiles(loadedLevelDetails.value().mapFolderPath, [pCustomLogger](const char *file) -> bool {
				if (!file) { return true; }
				if (*file == '\0') { return true; }
				debug(pCustomLogger, LOG_INFO, "Map folder file: %s", file);
				return true; // continue enumerating
			});

			// Successfully loaded level details from a .lev file in the root
			std::unique_ptr<MapPackage> result = std::unique_ptr<MapPackage>(new MapPackage());
			result->m_levelDetails = loadedLevelDetails.value();
			result->m_loadedLevelFormat = LevelFormat::LEV;
			result->m_mapType = result->m_levelDetails.type;
			result->m_pathToMapPackage = pathToMapPackage;
			result->m_mapIO = pMapIO;
			if (!originalGenerator.empty())
			{
				result->m_originalGenerator = originalGenerator;
			}
			result->loadGamInfo();

			if (idx < rootLevFiles.size() - 1)
			{
				// There are more possible .lev files to load??
				debug(pCustomLogger, LOG_WARNING, "Loaded level details from file \"%s\", but there are %zu additional *.lev file(s) in the base path. Skipping the rest...", levFile.c_str(), rootLevFiles.size() - 1 - idx);
			}
			return result;
		}
	}
	debug(pCustomLogger, LOG_ERROR, "Found %zu root *.lev files (older format), but unable to load level details from any of them.", rootLevFiles.size());
	return nullptr;
}

MapPackage::MapPackage(const LevelDetails& levelDetails, WzMap::MapType mapType, std::shared_ptr<WzMap::Map> map)
: m_levelDetails(levelDetails)
, m_mapType(mapType)
, m_loadedMap(map)
{ }

static bool copyFile_IOProviders(const std::string& readPath, const std::string& writePath, std::shared_ptr<IOProvider> readIO, std::shared_ptr<IOProvider> writeIO, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	std::vector<char> fileContents;
	if (!readIO->loadFullFile(readPath, fileContents))
	{
		// Failed reading file
		debug(pCustomLogger, LOG_ERROR, "copyFile: Failed reading file from source: %s", readPath.c_str());
		return false;
	}
	// get the directory this file should be in and ensure it is created
	std::string writePathDirName = writeIO->pathDirName(writePath);
	if (!writePathDirName.empty())
	{
		if (!writeIO->makeDirectory(writePathDirName))
		{
			debug(pCustomLogger, LOG_WARNING, "copyFile: Failed to create / verify destination directory: %s", writePathDirName.c_str());
		}
	}
	if (!writeIO->writeFullFile(writePath, fileContents.data(), static_cast<uint32_t>(fileContents.size())))
	{
		// Failed writing file
		debug(pCustomLogger, LOG_ERROR, "copyFile: Failed writing file to destination: %s", writePath.c_str());
		return false;
	}

	return true;
}

// Export the currently-loaded map package to a specified path in a specified format
// Can convert both the LevelFormat and the WzMap::OutputFormat
bool MapPackage::exportMapPackageFiles(std::string basePath, LevelFormat levelFormat, WzMap::OutputFormat mapOutputFormat,
									   optional<std::string> mapFolderRelativePathOverride /*= nullopt*/, bool copyAdditionalFilesFromOriginalLoadedPackage /*= false*/, std::shared_ptr<LoggingProtocol> logger /*= nullptr*/, std::shared_ptr<IOProvider> exportIO /*= std::shared_ptr<IOProvider>(new StdIOProvider())*/)
{
	LoggingProtocol* pCustomLogger = logger.get();
	if (!exportIO)
	{
		debug(pCustomLogger, LOG_ERROR, "Null IOProvider");
		return false;
	}

	auto currentPackageType = packageType();

	// 1.) Determine the path for the map folder, relative to the basePath
	// For Skirmish maps, this is "multiplay/maps/<map name>" (unless it's a "flat" map package)
	// For Campaign maps, a `mapFolderRelativePathOverride` **MUST** be provided [although campaign maps generally require a mod, and so this really shouldn't be used for campaign maps yet - instead, use the higher-level functions to output a map to the desired folder directly]
	std::string mapFolderPath = m_levelDetails.mapFolderPath;
	if (mapFolderPath.empty() && levelFormat == LevelFormat::LEV)
	{
		// old LEV format does not support "flattened" map packages
		switch (m_mapType)
		{
			case MapType::CAMPAIGN:
			case MapType::SAVEGAME:
				// TODO?
				break;
			case MapType::SKIRMISH:
				mapFolderPath = std::string("multiplay/maps/") + m_levelDetails.name;
				break;
		}
	}
	if (mapFolderRelativePathOverride.has_value())
	{
		debug(pCustomLogger, LOG_INFO, "Overriding map folder relative path (prior: %s) to: %s", mapFolderPath.c_str(), mapFolderRelativePathOverride.value().c_str());
		mapFolderPath = mapFolderRelativePathOverride.value();
	}

	size_t additionalFilesCopied = 0;
	if (m_mapIO)
	{
		// 2.) Copy any additional files from the original archive / directories (if configured + needed)
		if (copyAdditionalFilesFromOriginalLoadedPackage)
		{
			// Copy any additional files from the original archive / directories
			// (Such as texture image overrides...)
			// *Explicitly exclude any:
			//	- alternative level files (root *.lev files, map folder "level.json" files)
			//  - additional data files in the *map* folder itself (that may be older versions / map file formats)

			std::string newMapFolderPrefix = mapFolderPath;
			if (!newMapFolderPrefix.empty() && !strEndsWith(newMapFolderPrefix, exportIO->pathSeparator()))
			{
				newMapFolderPrefix += exportIO->pathSeparator();
			}
			if (newMapFolderPrefix == exportIO->pathSeparator())
			{
				newMapFolderPrefix = "";
			}

			std::string sourceMapFolderPrefix = m_levelDetails.mapFolderPath;
			std::string sourceMapGamFile = m_levelDetails.gamFilePath();
			if (!sourceMapFolderPrefix.empty() && !strEndsWith(sourceMapFolderPrefix, m_mapIO->pathSeparator()))
			{
				sourceMapFolderPrefix += m_mapIO->pathSeparator();
			}
			if (sourceMapFolderPrefix == m_mapIO->pathSeparator())
			{
				sourceMapFolderPrefix = "";
			}

			bool differentDestinationMapPath = sourceMapFolderPrefix != newMapFolderPrefix;

			auto possibleMapDataFilenames = Map::expectedFileNames();

			auto sourceIO = m_mapIO;
			auto sourceBasePath = m_pathToMapPackage;
			/*bool enumSuccess =*/ m_mapIO->enumerateFilesRecursive(m_pathToMapPackage, [basePath, sourceBasePath, newMapFolderPrefix, sourceMapFolderPrefix, possibleMapDataFilenames, sourceMapGamFile, sourceIO, exportIO, differentDestinationMapPath, pCustomLogger, &additionalFilesCopied](const char* filePath) -> bool {

				size_t filePathLen = strlen(filePath);

				// Filter out alternative root "level" info files
				if ((filePathLen > 10 && !strcasecmp(filePath + (filePathLen - 10), ".addon.lev"))
					|| (filePathLen > 13 && !strcasecmp(filePath + (filePathLen - 13), ".xplayers.lev")))
				{
					return true; // skip and continue enumerating
				}
				if (filePathLen >= 10 && !strcasecmp(filePath + (filePathLen - 10), "level.json"))
				{
					return true; // skip and continue enumerating
				}

				std::string destinationFilePath = filePath;

				// If it's something from the old map folder...
				if (strncmp(sourceMapFolderPrefix.c_str(), filePath, sourceMapFolderPrefix.size()) == 0)
				{
					// Filter out map data file names from the old map folder
					const char* fileName = filePath + sourceMapFolderPrefix.size();
					if (filePathLen > sourceMapFolderPrefix.size() && possibleMapDataFilenames.count(fileName) > 0)
					{
						return true; // skip and continue enumerating
					}

					// Copy to the *new* map folder
					destinationFilePath = exportIO->pathJoin(newMapFolderPrefix, fileName);
				}

				// If it's the old map .gam file, ignore it!
				if (sourceMapGamFile.compare(filePath) == 0)
				{
					return true; // skip and continue enumerating
				}

				if (differentDestinationMapPath)
				{
					// Warn about anything that will be copied directly into what will now be the map folder
					if (!newMapFolderPrefix.empty() && strncmp(newMapFolderPrefix.c_str(), filePath, newMapFolderPrefix.size()) == 0)
					{
						debug(pCustomLogger, LOG_WARNING, "Copied additional file will output into the map folder: %s", filePath);
					}
				}

				// Copy additional file from old to new package
				if (!copyFile_IOProviders(sourceIO->pathJoin(sourceBasePath, filePath), exportIO->pathJoin(basePath, destinationFilePath), sourceIO, exportIO, pCustomLogger))
				{
					debug(pCustomLogger, LOG_ERROR, "Failed to copy from source to destination: %s -> %s", filePath, destinationFilePath.c_str());
					return false;
				}
				++additionalFilesCopied;
				return true; // continue enumerating
			});
		}
		else if (currentPackageType != MapPackage::MapPackageType::Map_Plain)
		{
			debug(pCustomLogger, LOG_WARNING, "Input map is a map-mod, and has additional files. Configuration will ignore those files and create a plain map package (removing texture / stat overrides, for example).");
		}
	}

	// 3.) Determine if a "flattened" map package can be output
	// Based on whether additional files have been copied, and the output format (and other overrides), a "flattened" plain map package might be possible
	if (!mapFolderRelativePathOverride.has_value() && levelFormat == LevelFormat::JSON && additionalFilesCopied == 0)
	{
		// Since the new JSON level format is requested, and a "plain" map package is being output,
		// Can create a "flattened" map package! (where everything is in the root of the archive)
		debug(pCustomLogger, LOG_INFO, "Output map supports a \"flattened\" plain map archive");
		mapFolderPath = "";
	}

	// 4.) Determine the output path for the level info file based on the levelFormat
	// For classic LEV, this is the root basePath
	// For JSON, this is the same folder we put the map into
	std::string levelFileOutputFolder;
	switch (levelFormat)
	{
		case LevelFormat::LEV:
			levelFileOutputFolder = "";
			break;
		case LevelFormat::JSON:
			levelFileOutputFolder = mapFolderPath;
			break;
	}

	// 5.) Output the level info file
	std::string fullPathToOutputLevelDetailsFolder = exportIO->pathJoin(basePath, levelFileOutputFolder);
	if (!fullPathToOutputLevelDetailsFolder.empty())
	{
		if (!exportIO->makeDirectory(fullPathToOutputLevelDetailsFolder))
		{
			debug(pCustomLogger, LOG_WARNING, "Failed to create / verify destination directory: %s", fullPathToOutputLevelDetailsFolder.c_str());
			// for now, treat this as non-fatal...
		}
	}
	if (!exportLevelDetails(m_levelDetails, levelFormat, fullPathToOutputLevelDetailsFolder, *exportIO, pCustomLogger))
	{
		debug(pCustomLogger, LOG_ERROR, "Failed to export level details");
		return false;
	}

	// 6.) Load the map data
	auto pLoadedMap = loadMap(0, logger);
	if (pLoadedMap == nullptr)
	{
		debug(pCustomLogger, LOG_ERROR, "Failed to load map for conversion");
		return false;
	}

	std::string fullPathToOutputMapFolder = exportIO->pathJoin(basePath, mapFolderPath);

	// 7.) Ensure the fullPathToOutputMapFolder is created
	if (!fullPathToOutputMapFolder.empty())
	{
		if (!exportIO->makeDirectory(fullPathToOutputMapFolder))
		{
			debug(pCustomLogger, LOG_WARNING, "Failed to create / verify destination directory: %s", fullPathToOutputMapFolder.c_str());
			// for now, treat this as non-fatal...
		}
	}

	// 8.) Output the .gam file / gam.json file
	auto pMapData = pLoadedMap->mapData();
	if (pMapData == nullptr)
	{
		debug(pCustomLogger, LOG_ERROR, "Failed to load map data for conversion");
		return false;
	}
	writeGamFileForMapExport(fullPathToOutputMapFolder, levelFormat, m_gamInfo, *(pMapData.get()), *exportIO, pCustomLogger);

	// 9.) Output the map into the map folder
	if (pLoadedMap->wasScriptGenerated() && !(mapOutputFormat == WzMap::OutputFormat::VER1_BINARY_OLD || mapOutputFormat == WzMap::OutputFormat::VER2))
	{
		// NOTE: If the original map was a script map, and we're outputting to a modern format, just copy it over directly from the original package!

		// Copy the script data to game.js
		std::string gameJSOutputPath = exportIO->pathJoin(fullPathToOutputMapFolder, "game.js");
		auto pScriptData = pLoadedMap->scriptMapContents();
		if (!pScriptData)
		{
			debug(pCustomLogger, LOG_ERROR, "Failed to script contents from source map");
			return false;
		}
		if (!exportIO->writeFullFile(gameJSOutputPath, pScriptData->data(), static_cast<uint32_t>(pScriptData->size())))
		{
			// Failed writing file
			debug(pCustomLogger, LOG_ERROR, "Failed writing game.js to destination: %s", gameJSOutputPath.c_str());
			return false;
		}

		// Then write out the ttypes.ttp file
		auto mapTerrainTypes = pLoadedMap->mapTerrainTypes();
		if (!mapTerrainTypes)
		{
			debug(pCustomLogger, LOG_ERROR, "Failed to load / retrieve map terrain type data from source map");
			return false;
		}
		if (!writeTerrainTypes(*(mapTerrainTypes.get()), exportIO->pathJoin(fullPathToOutputMapFolder, "ttypes.ttp"), *exportIO, mapOutputFormat, pCustomLogger))
		{
			debug(pCustomLogger, LOG_ERROR, "Failed to write map terrain type data to export file");
			return false;
		}
	}
	else
	{
		if (pLoadedMap->wasScriptGenerated())
		{
			debug(pCustomLogger, LOG_WARNING, "Input map is a script-generated map. Due to output format, this will be converted to a static map.");
		}
		// Write out the map data
		if (!Map::exportMapToPath(*pLoadedMap, fullPathToOutputMapFolder, m_mapType, m_levelDetails.players, mapOutputFormat, logger, exportIO))
		{
			debug(pCustomLogger, LOG_ERROR, "Failed to export map data");
			return false;
		}
	}

	return true;
}

// Obtain the LevelDetails
const LevelDetails& MapPackage::levelDetails() const
{
	return m_levelDetails;
}

// Get the loaded level details format
// Note: Returns a value only if the MapPackage was loaded (i.e. via loadPackage)
optional<LevelFormat> MapPackage::loadedLevelDetailsFormat() const
{
	return m_loadedLevelFormat;
}

// Get the map data
// Returns nullptr if the loading failed
std::shared_ptr<Map> MapPackage::loadMap(uint32_t seed, std::shared_ptr<LoggingProtocol> logger /*= nullptr*/)
{
	if (m_loadedMap)
	{
		if (m_loadedMap->scriptGeneratedMapSeed().value_or(seed) == seed)
		{
			return m_loadedMap;
		}
		else
		{
			// Request for a map load of a script-generated map with a different seed
			// This must generate a new Map instance
			if (!m_loadedMap->wasScriptGenerated())
			{
				// Should not happen
				debug(logger.get(), LOG_ERROR, "Map has a script generation seed (so supposedly script-generated), but flag for wasScriptGenerated is false??");
				return nullptr;
			}
			return m_loadedMap->generateFromExistingScriptMap(seed, std::move(logger));
		}
	}
	else
	{
		// No map is loaded yet - verify this was instantiated with a map IO provider
		if (!m_mapIO)
		{
			// Should not happen
			debug(logger.get(), LOG_ERROR, "Missing IOProvider - no map can be loaded");
			return nullptr;
		}
	}

	std::string fullPathToMapFolder = m_mapIO->pathJoin(m_pathToMapPackage, m_levelDetails.mapFolderPath);
	m_loadedMap = Map::loadFromPath(fullPathToMapFolder, m_levelDetails.type, m_levelDetails.players, seed, std::move(logger), m_mapIO);
	return m_loadedMap;
}

static inline bool safeDirectoryPathCompare(const char* folderPath, const char* desiredPath, size_t desiredPathLen, const std::string& pathSeparator)
{
	if (!folderPath || !desiredPath) { return false; }
	if (*folderPath == '\0') { return false; }
	if (strncmp(desiredPath, folderPath, desiredPathLen) == 0)
	{
		size_t folderPathLen = strlen(folderPath);
		if (folderPathLen == desiredPathLen)
		{
			return true; // found match
		}
		else if (folderPathLen == (desiredPathLen + pathSeparator.size()))
		{
			if (strncmp(pathSeparator.c_str(), folderPath + desiredPathLen, pathSeparator.size()) == 0)
			{
				return true; // found match
			}
		}
	}

	return false;
}

MapPackage::MapPackageType MapPackage::packageType()
{
	if (m_packageType.has_value())
	{
		return m_packageType.value();
	}

	if (m_modTypes.has_value() && m_modTypes.value() != 0)
	{
		m_packageType = MapPackage::MapPackageType::Map_Mod;
		return m_packageType.value();
	}

	if (!m_mapIO)
	{
		m_packageType = MapPackage::MapPackageType::Map_Plain;
		return m_packageType.value();
	}

	// First, check if this is a "flat" modern map package
	// i.e. if it has "level.json" in its root
	// (if so, we can skip all of the older checks because it won't be mounted in a path that conflicts with core game files)
	if (m_flatMapPackage)
	{
		m_packageType = MapPackage::MapPackageType::Map_Plain;
		return m_packageType.value();
	}

	// otherwise, check package (older checks, non-flat map packages)
	bool foundUnapprovedPath = false;
	const std::string multiplay = "multiplay";
	const std::string maps = "maps";
	const std::string pathSeparator = m_mapIO->pathSeparator();

	// - First pass (root) looks for any folder that isn't "multiplay"
	bool enumSuccess = m_mapIO->enumerateFolders(m_pathToMapPackage, [&multiplay, &pathSeparator, &foundUnapprovedPath](const char *name) -> bool {
		if (!name) { return true; }
		if (*name == '\0') { return true; }
		if (strncmp(multiplay.c_str(), name, multiplay.size()) == 0)
		{
			size_t nameLen = strlen(name);
			if (nameLen == multiplay.size())
			{
				return true; // skip and continue enumerating
			}
			else if (nameLen == (multiplay.size() + pathSeparator.size()))
			{
				if (strncmp(pathSeparator.c_str(), name + multiplay.size(), pathSeparator.size()) == 0)
				{
					return true; // skip and continue enumerating
				}
			}
		}
		foundUnapprovedPath = true;
		return false; // found a non-map path
	});
	if (foundUnapprovedPath)
	{
		m_packageType = MapPackage::MapPackageType::Map_Mod;
		return m_packageType.value();
	}
	if (!enumSuccess)
	{
		// failed to enumerate??
		debug(m_logger.get(), LOG_ERROR, "Failed enumerating package");
	}

	// - Second pass (multiplay) looks for any folder that isn't "maps"
	enumSuccess = m_mapIO->enumerateFolders(m_mapIO->pathJoin(m_pathToMapPackage, multiplay), [&maps, &pathSeparator, &foundUnapprovedPath](const char *name) -> bool {
		if (!name) { return true; }
		if (*name == '\0') { return true; }
		if (strncmp(maps.c_str(), name, maps.size()) == 0)
		{
			size_t nameLen = strlen(name);
			if (nameLen == maps.size())
			{
				return true; // skip and continue enumerating
			}
			else if (nameLen == (maps.size() + pathSeparator.size()))
			{
				if (strncmp(pathSeparator.c_str(), name + maps.size(), pathSeparator.size()) == 0)
				{
					return true; // skip and continue enumerating
				}
			}
		}
		foundUnapprovedPath = true;
		return false; // found a non-map path
	});
	if (foundUnapprovedPath)
	{
		m_packageType = MapPackage::MapPackageType::Map_Mod;
		return m_packageType.value();
	}
	if (!enumSuccess)
	{
		// failed to enumerate??
		debug(m_logger.get(), LOG_ERROR, "Failed enumerating package (2)");
	}

	// - Third pass (root) look for specific files that might affect game behavior
	enumSuccess = m_mapIO->enumerateFiles(m_pathToMapPackage, [&foundUnapprovedPath](const char* file) -> bool {
		if (strcmp(file, "gamedesc.lev") == 0 || strcmp(file, "palette.txt") == 0)
		{
			// Found one!
			foundUnapprovedPath = true;
			return false;
		}
		return true; // continue enumerating
	});
	if (foundUnapprovedPath)
	{
		m_packageType = MapPackage::MapPackageType::Map_Mod;
		return m_packageType.value();
	}
	if (!enumSuccess)
	{
		// failed to enumerate??
		debug(m_logger.get(), LOG_ERROR, "Failed enumerating package (3)");
	}

	m_packageType = MapPackage::MapPackageType::Map_Plain;
	return m_packageType.value();
}

uint64_t MapPackage::baseModificationTypes()
{
	if (m_modTypes.has_value())
	{
		return m_modTypes.value();
	}

	if (m_packageType.has_value() && m_packageType.value() == MapPackage::MapPackageType::Map_Plain)
	{
		m_modTypes = 0;
		return 0;
	}

	if (!m_mapIO)
	{
		m_modTypes = 0;
		return 0;
	}

	uint64_t modTypes = 0;

	const std::string pathSeparator = m_mapIO->pathSeparator();
#define dirCmp(a, b) safeDirectoryPathCompare(a, b, strlen(b), pathSeparator)

	// - First pass looks in the root folder
	bool enumSuccess = m_mapIO->enumerateFolders(m_pathToMapPackage, [&modTypes, &pathSeparator](const char *name) -> bool {
		if (!name) { return true; }
		if (*name == '\0') { return true; }

		if (dirCmp(name, "components") || dirCmp(name, "features") || dirCmp(name, "structs") || dirCmp(name, "misc"))
		{
			modTypes |= ModTypes::GameModels;
			return true; // continue with next
		}
		if (dirCmp(name, "effects"))
		{
			modTypes |= ModTypes::Effects;
			return true; // continue with next
		}
		if (dirCmp(name, "shaders"))
		{
			modTypes |= ModTypes::Shaders;
			return true; // continue with next
		}
		if (dirCmp(name, "stats"))
		{
			modTypes |= ModTypes::Stats;
			return true; // continue with next
		}
		if (dirCmp(name, "texpages"))
		{
			modTypes |= ModTypes::Textures;
			return true; // continue with next
		}
		if (dirCmp(name, "script"))
		{
			modTypes |= ModTypes::Scripts;
			return true; // continue with next
		}
		if (dirCmp(name, "messages"))
		{
			modTypes |= ModTypes::Messages;
			return true; // continue with next
		}
		if (dirCmp(name, "fonts") || dirCmp(name, "icons") || dirCmp(name, "images"))
		{
			modTypes |= ModTypes::MiscUI;
			return true; // continue with next
		}
		if (dirCmp(name, "audio") || dirCmp(name, "sequenceaudio"))
		{
			modTypes |= ModTypes::Sound;
			return true; // continue with next
		}
		if (dirCmp(name, "tileset"))
		{
			modTypes |= ModTypes::Tilesets;
			return true; // continue with next
		}
		if (dirCmp(name, "wrf"))
		{
			modTypes |= ModTypes::Datasets;
			return true; // continue with next
		}

		return true;
	});
	if (!enumSuccess)
	{
		// failed to enumerate??
		debug(m_logger.get(), LOG_ERROR, "Failed enumerating package");
	}

	// - Second pass (multiplay folder)
	enumSuccess = m_mapIO->enumerateFolders(m_mapIO->pathJoin(m_pathToMapPackage, "multiplay"), [&modTypes, &pathSeparator](const char *name) -> bool {
		if (!name) { return true; }
		if (*name == '\0') { return true; }

		if (dirCmp(name, "script"))
		{
			modTypes |= ModTypes::Scripts;
			return true; // continue with next
		}
		if (dirCmp(name, "skirmish"))
		{
			modTypes |= ModTypes::SkirmishAI;
			return true; // continue with next
		}

		return true;
	});
	if (!enumSuccess)
	{
		// failed to enumerate??
		debug(m_logger.get(), LOG_ERROR, "Failed enumerating package (2)");
	}

	m_modTypes = modTypes;
	return modTypes;
}

bool MapPackage::modTypesEnumerate(std::function<void (ModTypes modType)> func)
{
	return modTypesEnumerate(baseModificationTypes(), func);
}

bool MapPackage::modTypesEnumerate(uint64_t modTypesValue, std::function<void (ModTypes modType)> func)
{
	if (modTypesValue == 0) { return false; }
	for (size_t i = 0; (static_cast<size_t>(1) << i) <= static_cast<size_t>(LastModType); i++)
	{
		size_t modTypeCurrent = (static_cast<size_t>(1) << i);
		if ((modTypesValue & modTypeCurrent) == modTypeCurrent)
		{
			func(static_cast<ModTypes>(modTypeCurrent));
		}
	}
	return true;
}

// NOTE: There may be clients that rely on the specific output for each of these ModTypes
// DO NOT EDIT the string output for existing entries!!
std::string MapPackage::to_string(MapPackage::ModTypes modType)
{
	switch (modType)
	{
		case MapPackage::ModTypes::GameModels: return "gamemodels";
		case MapPackage::ModTypes::Effects: return "effects";
		case MapPackage::ModTypes::Shaders: return "shaders";
		case MapPackage::ModTypes::Stats: return "stats";
		case MapPackage::ModTypes::Textures: return "textures";
		case MapPackage::ModTypes::Scripts: return "scripts";
		case MapPackage::ModTypes::SkirmishAI: return "skirmishai";
		case MapPackage::ModTypes::Messages: return "messages";
		case MapPackage::ModTypes::MiscUI: return "miscui";
		case MapPackage::ModTypes::Sound: return "sound";
		case MapPackage::ModTypes::Tilesets: return "tilesets";
		case MapPackage::ModTypes::Datasets: return "datasets";
	}
	return "";	// silence warning
}

bool MapPackage::loadGamInfo()
{
	if (!m_mapIO)
	{
		return false;
	}

	std::string fullPathToMapFolder = m_mapIO->pathJoin(m_pathToMapPackage, m_levelDetails.mapFolderPath);
	auto loadedGamInfo = loadGamFile_JSON(m_mapIO->pathJoin(fullPathToMapFolder, "gam.json"), *m_mapIO, m_logger.get());
	if (!loadedGamInfo.has_value())
	{
		// fall back to older <map>.gam file
		loadedGamInfo = loadGamFile_OldBinary(m_mapIO->pathJoin(m_pathToMapPackage, m_levelDetails.gamFilePath()), *m_mapIO, m_logger.get());
	}
	if (!loadedGamInfo.has_value())
	{
		return false;
	}

	m_gamInfo = loadedGamInfo.value();
	return true;
}

// Extract various map stats / info
optional<MapStats> MapPackage::calculateMapStats(MapStatsConfiguration statsConfig)
{
	LoggingProtocol* pCustomLogger = m_logger.get();
	auto pLoadedMap = loadMap(0, m_logger);
	if (pLoadedMap == nullptr)
	{
		debug(pCustomLogger, LOG_ERROR, "Failed to load map");
		return nullopt;
	}

	if (packageType() == MapPackageType::Map_Mod && m_mapIO != nullptr)
	{
		// Try to load stats configuration from the map package (if it's a map mod and is overriding those stats)
		if (statsConfig.loadFromTemplatesJSON(m_mapIO->pathJoin("stats", "templates.json"), *m_mapIO, pCustomLogger))
		{
			debug(pCustomLogger, LOG_INFO, "Using map mod stats overrides: templates.json");
		}
		if (statsConfig.loadFromStructureJSON(m_mapIO->pathJoin("stats", "structure.json"), *m_mapIO, pCustomLogger))
		{
			debug(pCustomLogger, LOG_INFO, "Using map mod stats overrides: structure.json");
		}
		if (statsConfig.loadFromFeaturesJSON(m_mapIO->pathJoin("stats", "features.json"), *m_mapIO, pCustomLogger))
		{
			debug(pCustomLogger, LOG_INFO, "Using map mod stats overrides: features.json");
		}
	}

	return pLoadedMap->calculateMapStats(m_levelDetails.players, statsConfig);
}

} // namespace WzMap
