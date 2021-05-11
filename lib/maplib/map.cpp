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

#include "map.h"
#include "map_crc.h"
#include "map_script.h"
#include "map_internal.h"
#include <json/json.hpp>
#include <cinttypes>
#include <unordered_set>
#include <limits>

#define VERSION_39              39	        // last game.map version in the old format (with bytes for tile height, must be multiplied by ELEVATION_SCALE)
#define VERSION_40              40			// game.map version with full-range tile height
#define CURRENT_VERSION_NUM     VERSION_40

#define MAX_PLAYERS         11                 ///< Maximum number of players in the game.

#define PLAYER_SCAVENGERS -1

// 65536 / 360 = 8192 / 45, with a bit less overflow risk.
#define DEG(degrees) ((degrees) * 8192 / 45)

// MARK: - Handling game.map / MapData files

namespace WzMap {

template<typename T>
struct FileLoadResult
{
	std::vector<T> objects;
	uint32_t fileFormatVersion = 0;
};

struct MapDataLoadResult
{
	MapData mapData;
	uint32_t fileFormatVersion = 0;
};

static optional<MapDataLoadResult> loadMapData_Internal(const std::string &filename, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	const auto &path = filename.c_str();
	auto pStream = mapIO.openBinaryStream(filename, BinaryIOStream::OpenMode::READ);

	if (!pStream)
	{
		debug(pCustomLogger, LOG_ERROR, "%s not found", path);
		return nullopt;
	}

	debug(pCustomLogger, LOG_INFO, "Loading: %s", path);

	MapData map;
	uint32_t mapVersion = 0;
	char aFileType[4];
	if (pStream->readBytes(aFileType, 4) != static_cast<size_t>(4)
		|| !pStream->readULE32(&mapVersion)
		|| !pStream->readULE32(&map.width)
		|| !pStream->readULE32(&map.height)
		|| aFileType[0] != 'm'
		|| aFileType[1] != 'a'
		|| aFileType[2] != 'p')
	{
		debug(pCustomLogger, LOG_ERROR, "Bad header in %s", path);
		return nullopt;
	}

	if (mapVersion <= 9)
	{
		debug(pCustomLogger, LOG_ERROR, "%s: Unsupported save format version %u", path, mapVersion);
		return nullopt;
	}
	if (mapVersion > CURRENT_VERSION_NUM)
	{
		debug(pCustomLogger, LOG_ERROR, "%s: Undefined save format version %u", path, mapVersion);
		return nullopt;
	}
	if ((uint64_t)map.width * map.height > MAP_MAXAREA)
	{
		debug(pCustomLogger, LOG_ERROR, "Map %s too large : %d %d", path, map.width, map.height);
		return nullopt;
	}

	if (map.width <= 1 || map.height <= 1)
	{
		debug(pCustomLogger, LOG_ERROR, "Map %s is too small : %u, %u", path, map.width, map.height);
		return nullopt;
	}

	/* Load in the map data */
	uint32_t numMapTiles = map.width * map.height;
	map.mMapTiles.reserve(numMapTiles);
	if (mapVersion >= VERSION_40)
	{
		// load full-range map tile heights
		for (uint32_t i = 0; i < numMapTiles; i++)
		{
			MapData::MapTile currentMapTile{};
			if (!pStream->readULE16(&(currentMapTile.texture)) || !pStream->readULE16(&(currentMapTile.height)))
			{
				debug(pCustomLogger, LOG_ERROR, "%s: Error during savegame load", path);
				return nullopt;
			}
			if (currentMapTile.height > TILE_MAX_HEIGHT)
			{
				debug(pCustomLogger, LOG_ERROR, "%s: Tile height (%" PRIu16 ") exceeds TILE_MAX_HEIGHT (%zu)", path, currentMapTile.height, static_cast<size_t>(TILE_MAX_HEIGHT));
				return nullopt;
			}
			map.mMapTiles.emplace_back(currentMapTile);
		}
	}
	else
	{
		// load old map tile heights where tile-heights fit into a byte (and raw value is divided by ELEVATION_SCALE)
		for (uint32_t i = 0; i < numMapTiles; i++)
		{
			uint16_t texture;
			uint8_t height;

			if (!pStream->readULE16(&texture) || !pStream->readULE8(&height))
			{
				debug(pCustomLogger, LOG_ERROR, "%s: Error during savegame load", path);
				return nullopt;
			}
			MapData::MapTile currentMapTile{};
			currentMapTile.texture = texture;
			currentMapTile.height = static_cast<uint16_t>(height) * ELEVATION_SCALE;
			map.mMapTiles.emplace_back(currentMapTile);
		}
	}

	uint32_t gwVersion;
	uint32_t numGateways;
	if (!pStream->readULE32(&gwVersion) || !pStream->readULE32(&numGateways) || gwVersion != 1)
	{
		debug(pCustomLogger, LOG_ERROR, "Bad gateway in %s", path);
		return nullopt;
	}

	map.mGateways.reserve(numGateways);
	for (uint32_t i = 0; i < numGateways; i++)
	{
		MapData::Gateway gw = {};
		if (!pStream->readULE8(&gw.x1) || !pStream->readULE8(&gw.y1) || !pStream->readULE8(&gw.x2) ||
			!pStream->readULE8(&gw.y2))
		{
			debug(pCustomLogger, LOG_ERROR, "%s: Failed to read gateway info", path);
			return nullopt;
		}
		map.mGateways.emplace_back(gw);
	}

	MapDataLoadResult result;
	result.mapData = std::move(map);
	result.fileFormatVersion = mapVersion;
	return result;
}

std::shared_ptr<MapData> loadMapData(const std::string &filename, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	auto result = loadMapData_Internal(filename, mapIO, pCustomLogger);
	if (!result.has_value())
	{
		return nullptr;
	}
	return std::make_shared<MapData>(result.value().mapData);
}

bool writeMapData(const MapData& map, const std::string &filename, IOProvider& mapIO, OutputFormat format, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	const auto &path = filename.c_str();
	auto pStream = mapIO.openBinaryStream(filename, BinaryIOStream::OpenMode::WRITE);

	if (!pStream)
	{
		debug(pCustomLogger, LOG_ERROR, "Failed to open file for writing: %s", path);
		return false;
	}

	debug(pCustomLogger, LOG_INFO, "Writing: %s", path);

	uint32_t mapVersion = CURRENT_VERSION_NUM;
	switch (format)
	{
		case OutputFormat::VER1_BINARY_OLD:
			mapVersion = 10;
			break;
		case OutputFormat::VER2:
			mapVersion = 39;
			break;
		case OutputFormat::VER3:
			mapVersion = 40;
			break;
	}

	// write header
	char aFileType[4];
	aFileType[0] = 'm';
	aFileType[1] = 'a';
	aFileType[2] = 'p';
	aFileType[3] = ' ';
	if (pStream->writeBytes(aFileType, 4) != static_cast<size_t>(4)
		|| !pStream->writeULE32(mapVersion)
		|| !pStream->writeULE32(map.width)
		|| !pStream->writeULE32(map.height))
	{
		debug(pCustomLogger, LOG_ERROR, "Failed writing header to: %s", path);
		return false;
	}

	if ((uint64_t)map.width * map.height > MAP_MAXAREA)
	{
		debug(pCustomLogger, LOG_ERROR, "Map too large : %" PRIu32 " x %" PRIu32 "", map.width, map.height);
		return false;
	}
	if (mapVersion <= 9)
	{
		debug(pCustomLogger, LOG_ERROR, "Unsupported save format version %" PRIu32 "", mapVersion);
		return false;
	}
	if (map.width <= 1 || map.height <= 1)
	{
		debug(pCustomLogger, LOG_ERROR, "Map is too small : %" PRIu32 " x %" PRIu32 "", map.width, map.height);
		return false;
	}

	/* Write out the map tiles */
	uint32_t numMapTiles = map.width * map.height;
	if (numMapTiles != static_cast<uint32_t>(map.mMapTiles.size()))
	{
		debug(pCustomLogger, LOG_ERROR, "Map width x height (%" PRIu32 " x %" PRIu32 ") != number of map tiles (%zu)", map.width, map.height, map.mMapTiles.size());
		return false;
	}

	if (mapVersion >= VERSION_40)
	{
		// write new tile info, with full-range tile heights
		for (uint32_t i = 0; i < numMapTiles; i++)
		{
			const auto& tile = map.mMapTiles[i];
			if (tile.height > TILE_MAX_HEIGHT)
			{
				// tile height exceeds maximum
				debug(pCustomLogger, LOG_ERROR, "Error writing maptile; tile height (%" PRIu16 ") exceeds maximum tile height (%zu)", tile.height, static_cast<size_t>(TILE_MAX_HEIGHT));
				return false;
			}
			if (!pStream->writeULE16(tile.texture) || !pStream->writeULE16(tile.height))
			{
				debug(pCustomLogger, LOG_ERROR, "Error writing maptile to: %s", path);
				return false;
			}
		}
	}
	else
	{
		// write old tile info, where tile-heights fit into a byte (and raw value is divided by ELEVATION_SCALE)
		for (uint32_t i = 0; i < numMapTiles; i++)
		{
			const auto& tile = map.mMapTiles[i];
			if (tile.height > (static_cast<uint16_t>(std::numeric_limits<uint8_t>::max()) * ELEVATION_SCALE))
			{
				// tile height exceeds maximum supported by this old format
				debug(pCustomLogger, LOG_ERROR, "Error writing maptile; tile height (%" PRIu16 ") exceeds maximum supported by output format (%zu)", tile.height, static_cast<size_t>(std::numeric_limits<uint8_t>::max()));
				return false;
			}
			uint8_t tileHeight = static_cast<uint8_t>(tile.height / ELEVATION_SCALE);
			if (!pStream->writeULE16(tile.texture) || !pStream->writeULE8(tileHeight))
			{
				debug(pCustomLogger, LOG_ERROR, "Error writing maptile to: %s", path);
				return false;
			}
		}
	}

	// Write the gateway header
	uint32_t gwVersion = 1;
	uint32_t numGateways = static_cast<uint32_t>(map.mGateways.size());
	if (!pStream->writeULE32(gwVersion) || !pStream->writeULE32(numGateways))
	{
		debug(pCustomLogger, LOG_ERROR, "Error writing gateway header to: %s", path);
		return false;
	}

	// Write out the gateways
	for (uint32_t i = 0; i < numGateways; i++)
	{
		const MapData::Gateway& gw = map.mGateways[i];
		if ((gw.x1 != gw.x2) && (gw.y1 != gw.y2))
		{
			debug(pCustomLogger, LOG_WARNING, "Invalid gateway coordinates (%u, %u, %u, %u)", static_cast<unsigned>(gw.x1), static_cast<unsigned>(gw.y1), static_cast<unsigned>(gw.x2), static_cast<unsigned>(gw.y2));
			// for now, write anyway
		}
		if (gw.x1 > map.width || gw.x2 > map.width
			|| gw.y1 > map.height || gw.y2 > map.height)
		{
			debug(pCustomLogger, LOG_WARNING, "Bad gateway dimensions (exceed map width or height)");
			// for now, write anyway
		}

		if (!pStream->writeULE8(gw.x1) || !pStream->writeULE8(gw.y1) || !pStream->writeULE8(gw.x2) ||
			!pStream->writeULE8(gw.y2))
		{
			debug(pCustomLogger, LOG_ERROR, "Error writing gateway to: %s", path);
			return false;
		}
	}

	pStream.reset();
	return true;
}

// MARK: - Helper functions for loading / saving JSON files
static optional<nlohmann::json> loadJsonObjectFromFile(const std::string& filename, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	const auto &path = filename.c_str();
	std::vector<char> data;
	if (!mapIO.loadFullFile(filename, data))
	{
		return nullopt;
	}
	if (data.empty())
	{
		debug(pCustomLogger, LOG_ERROR, "Empty file: %s", path);
		return nullopt;
	}
	if (data.back() != 0)
	{
		data.push_back('\0'); // always ensure data is null-terminated
	}

	// parse JSON
	nlohmann::json mRoot;
	try {
		mRoot = nlohmann::json::parse(data.begin(), data.end() - 1);
	}
	catch (const std::exception &e) {
		debug(pCustomLogger, LOG_ERROR, "JSON document from %s is invalid: %s", path, e.what());
		return nullopt;
	}
	catch (...) {
		debug(pCustomLogger, LOG_ERROR, "Unexpected exception parsing JSON %s", path);
		return nullopt;
	}
	if (mRoot.is_null())
	{
		debug(pCustomLogger, LOG_ERROR, "JSON document from %s is null", path);
		return nullopt;
	}
	if (!mRoot.is_object())
	{
		debug(pCustomLogger, LOG_ERROR, "JSON document from %s is not an object. Read: \n%s", path, data.data());
		return nullopt;
	}
	data.clear();

	return mRoot;
}

static bool saveOrderedJsonObjectToFile(const nlohmann::ordered_json& jsonObj, const std::string& filename, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	std::string jsonStr = jsonObj.dump(4, ' ', false, nlohmann::ordered_json::error_handler_t::ignore);
#if SIZE_MAX > UINT32_MAX
	if (jsonStr.size() > static_cast<size_t>(std::numeric_limits<uint32_t>::max()))
	{
		debug(pCustomLogger, LOG_ERROR, "jsonString.size (%zu) exceeds uint32_t::max", jsonStr.size());
		return false;
	}
#endif
	return mapIO.writeFullFile(filename, jsonStr.c_str(), static_cast<uint32_t>(jsonStr.size()));
}

// Left-pads the current string with codepoint ch up to the minimumStringLength
// If the current string length() is already >= minimumStringLength, no padding occurs.
static std::string& leftPadStrToMinimumLength(std::string& str, size_t minimumStringLength, const char paddingChar = '0')
{
	if (str.length() >= minimumStringLength)
	{
		return str;
	}

	size_t leftPaddingRequired = minimumStringLength - str.length();
	std::string strPadding;
	for (size_t i = 0; i < leftPaddingRequired; i++)
	{
		strPadding.resize(leftPaddingRequired, paddingChar);
	}
	str = str + strPadding;
	return str;
}

template<typename T>
static size_t numberOfDigitsInNumber(T num)
{
	size_t digits = 0;
	do {
		num /= 10;
		digits++;
	} while (num != 0);
	return digits;
}

struct JsonParsingContext
{
	const char* filename;
	const char* jsonPath;
};

static inline nlohmann::json* jsonGetRootMapObjectsContainer(const std::string& filename, nlohmann::json& rootObject, uint32_t jsonFileFormat, const char *pRootContainerName, LoggingProtocol* pCustomLogger = nullptr)
{
	assert(pRootContainerName != nullptr);
	nlohmann::json& mMapObjectsContainer = rootObject;
	if (jsonFileFormat > 1)
	{
		if (!rootObject.contains(pRootContainerName))
		{
			// Missing required "droid" key for list of droids
			debug(pCustomLogger, LOG_ERROR, "%s: Missing required \"%s\" key in root object", filename.c_str(), pRootContainerName);
			return nullptr;
		}
		mMapObjectsContainer = rootObject.at(pRootContainerName);
		if (!mMapObjectsContainer.is_array())
		{
			debug(pCustomLogger, LOG_ERROR, "%s: \"%s\" value should be an array", filename.c_str(), pRootContainerName);
			return nullptr;
		}
	}
	return &mMapObjectsContainer;
}

static inline optional<uint32_t> jsonGetFileFormatVersion(const std::string& filename, nlohmann::json& rootObject, LoggingProtocol* pCustomLogger = nullptr, uint32_t maxSupportedFileFormatVersion = 2)
{
	uint32_t fileFormatVersion = 1;
	auto it_version = rootObject.find("version");
	if (it_version != rootObject.end())
	{
		if (!it_version->is_number())
		{
			debug(pCustomLogger, LOG_ERROR, "%s: \"version\" key is not a number", filename.c_str());
			return nullopt;
		}
		fileFormatVersion = it_version->get<uint32_t>();
		if (fileFormatVersion == 1)
		{
			debug(pCustomLogger, LOG_ERROR, "%s: Unsupported file \"version\" (%" PRIu32 ") - version 1 lacks a \"version\" key", filename.c_str(), fileFormatVersion);
			return nullopt;
		}
		if (fileFormatVersion == 0 || (fileFormatVersion > maxSupportedFileFormatVersion))
		{
			debug(pCustomLogger, LOG_ERROR, "%s: Unsupported file \"version\" (%" PRIu32 ")", filename.c_str(), fileFormatVersion);
			return nullopt;
		}
	}
	return fileFormatVersion;
}

template <typename T>
static inline optional<std::vector<T>> jsonGetListOfType(const nlohmann::json& obj, const std::string& key, size_t minItems, size_t maxItems, const JsonParsingContext& jsonContext, LoggingProtocol* pCustomLogger = nullptr, bool allowNonList = false)
{
	std::vector<T> result;
	auto it = obj.find(key);
	if (it == obj.end())
	{
		return nullopt;
	}

	if (it->size() < minItems)
	{
		// Invalid value - requires at least minItems components
		debug(pCustomLogger, LOG_ERROR, "%s: Invalid \"%s\" (requires at least %zu members) for: %s", jsonContext.filename, key.c_str(), minItems, jsonContext.jsonPath);
		return nullopt;
	}

	if (!it->is_array())
	{
		if (!allowNonList)
		{
			// Invalid value - expecting an array
			debug(pCustomLogger, LOG_ERROR, "%s: Invalid \"%s\" (expecting an array) for: %s", jsonContext.filename, key.c_str(), jsonContext.jsonPath);
			return nullopt;
		}
		// process non-array
		try {
			result.push_back(it->get<T>());
		}
		catch (const std::exception &e) {
			debug(pCustomLogger, LOG_ERROR, "%s: Invalid \"%s\" (unable to convert to desired output type) for: %s; error: %s", jsonContext.filename, key.c_str(), jsonContext.jsonPath, e.what());
			return nullopt;
		}
		return result;
	}

	try {
		for (size_t idx = 0; idx < minItems; idx++)
		{
			result.push_back(it->at(idx).get<T>());
		}
	}
	catch (const std::exception &e) {
		debug(pCustomLogger, LOG_ERROR, "%s: Invalid \"%s\" (unable to convert to desired output type) for: %s; error: %s", jsonContext.filename, key.c_str(), jsonContext.jsonPath, e.what());
		return nullopt;
	}

	if (it->size() > maxItems)
	{
		// Too many items
		debug(pCustomLogger, LOG_ERROR, "%s: Invalid \"%s\" (too many array members) for: %s", jsonContext.filename, key.c_str(), jsonContext.jsonPath);
	}
	else if (it->size() > minItems)
	{
		// check for non-zero additional members, and warn that they are being ignored
		try {
			for (size_t idx = minItems; idx < it->size(); idx++)
			{
				if (it->at(idx).get<T>() != 0)
				{
					debug(pCustomLogger, LOG_INFO_VERBOSE, "%s: Ignoring non-0 \"%s[%zu]\" for: %s", jsonContext.filename, key.c_str(), idx, jsonContext.jsonPath);
				}
			}
		}
		catch (const std::exception &e) {
			debug(pCustomLogger, LOG_WARNING, "%s: Invalid ignored \"%s\" member (unable to convert to desired output type) for: %s; error: %s", jsonContext.filename, key.c_str(), jsonContext.jsonPath, e.what());
		}
	}
	return result;
}

// the player is extracted from either "player" or "startpos"
// for multiplayer / skirmish maps, "startpos" should be set to the player slot index
// for campaign maps, "player" can be set to the player number
// in all cases, "player" can be set to the string "scavenger" to identify the object as belonging to the scavengers player
static optional<int8_t> jsonGetPlayerFromObj(const nlohmann::json& obj, MapType mapType, const JsonParsingContext& jsonContext, LoggingProtocol* pCustomLogger = nullptr, bool playerFieldOnly = false)
{
	auto it = obj.find("player");
	if (it != obj.end())
	{
		if (obj.contains("startpos"))
		{
			debug(pCustomLogger, LOG_SYNTAX_WARNING, "%s: Processing \"player\", ignoring \"startpos\" for: %s", jsonContext.filename, jsonContext.jsonPath);
			// continue to process "player"
		}
		if (it->is_string() && it->get<std::string>().rfind("scavenger", 0) == 0)
		{
			return PLAYER_SCAVENGERS;
		}
		else if (it->is_number())
		{
			if (mapType != MapType::CAMPAIGN && mapType != MapType::SAVEGAME)
			{
				// "player" is generally expected only for campaign and savegames (and for specifying scavengers)
				debug(pCustomLogger, LOG_INFO_VERBOSE, "Found \"player\" for non-campaign/savegame.");
				// however, proceed anyway (for now)
			}
			return it->get<int8_t>();
		}
		// otherwise, invalid json type
		return nullopt;
	}
	if (playerFieldOnly)
	{
		return nullopt;
	}
	it = obj.find("startpos");
	if (it != obj.end())
	{
		if (!it->is_number())
		{
			// invalid json type
			return nullopt;
		}
		return it->get<int8_t>();
	}
	// No player info found!
	return nullopt;
}

static bool jsonSetPlayerOnObject(nlohmann::ordered_json& jsonObj, MapType mapType, int8_t player)
{
	if (player == PLAYER_SCAVENGERS)
	{
		jsonObj["player"] = "scavenger";
	}
	else
	{
		if (player < 0)
		{
			return false;
		}
		switch (mapType)
		{
			case MapType::CAMPAIGN:
			case MapType::SAVEGAME:
				jsonObj["player"] = player;
				break;
			case MapType::SKIRMISH:
				jsonObj["startpos"] = player;
				break;
		}
	}

	return true;
}

template<typename T>
static inline bool jsonGetBaseMapObjectInfo(T& mapObj, uint32_t jsonFileVersion, const nlohmann::json& jsonObj, const JsonParsingContext& jsonContext, LoggingProtocol* pCustomLogger = nullptr, const char *pNameKey = "name")
{
	assert(pNameKey != nullptr);
	size_t maxComponentsPosition = 3;
	size_t maxComponentsRotation = 3;
	bool rotationAllowNonArray = false;
	if (jsonFileVersion > 1)
	{
		maxComponentsPosition = 2; // [x, y] only
		maxComponentsRotation = 1; // [x] only
		rotationAllowNonArray = true;
	}
	auto it_name = jsonObj.find(pNameKey);
	if (it_name == jsonObj.end())
	{
		// Missing required "template" key for map object
		debug(pCustomLogger, LOG_ERROR, "%s: Missing required \"%s\" key for: %s", jsonContext.filename, pNameKey, jsonContext.jsonPath);
		return false;
	}
	if (!it_name->is_string())
	{
		// Expecting a string value for name
		debug(pCustomLogger, LOG_ERROR, "%s: Unexpected type of required \"%s\" key (expecting string) for: %s", jsonContext.filename, pNameKey, jsonContext.jsonPath);
		return false;
	}
	mapObj.name = it_name->get<std::string>();
	// "id" is explicitly optional - synchronized unit ids will be generated if it is omitted
	auto it_id = jsonObj.find("id");
	if (it_id != jsonObj.end())
	{
		if (!it_id->is_number())
		{
			// Expecting a numeric value for id
			debug(pCustomLogger, LOG_ERROR, "%s: Unexpected type of \"id\" key (expecting number) for: %s", jsonContext.filename, jsonContext.jsonPath);
			return false;
		}
		uint32_t id = jsonObj["id"].get<uint32_t>();
		if (id == 0)
		{
			// Invalid droid id - cannot be 0
			debug(pCustomLogger, LOG_ERROR, "%s: Invalid \"id\" = 0 for: %s", jsonContext.filename, jsonContext.jsonPath);
			return false;
		}
		mapObj.id = id;
	}
	// "position" must contain at least two components [x, y]
	auto position = jsonGetListOfType<int>(jsonObj, "position", 2, maxComponentsPosition, jsonContext, pCustomLogger);
	if (position.has_value())
	{
		mapObj.position.x = position.value()[0];
		mapObj.position.y = position.value()[1];
	}
	else
	{
		// Missing required "position" key for map object
		debug(pCustomLogger, LOG_ERROR, "%s: Missing required \"position\" key for: %s", jsonContext.filename, jsonContext.jsonPath);
		return false;
	}
	// "rotation" must contain at least one components [x]
	auto rotation = jsonGetListOfType<uint16_t>(jsonObj, "rotation", 1, maxComponentsRotation, jsonContext, pCustomLogger, rotationAllowNonArray);
	if (rotation.has_value())
	{
		mapObj.direction = rotation.value()[0];
	}

	return true;
}

template<typename T>
static void jsonSetBaseMapObjectInfo(nlohmann::ordered_json& jsonObj, uint32_t jsonFileVersion, const T& mapObj, const char *pNameKey = "name")
{
	assert(pNameKey != nullptr);
	jsonObj[pNameKey] = mapObj.name;
	if (mapObj.id.has_value())
	{
		jsonObj["id"] = mapObj.id.value();
	}
	// "position" must contain at least two components [x, y] - jsonFileVersion_v1 always expects 3 (and ignores the third)
	nlohmann::ordered_json position = nlohmann::ordered_json::array();
	position.push_back(mapObj.position.x);
	position.push_back(mapObj.position.y);
	if (jsonFileVersion == 1)
	{
		position.push_back(0);
	}
	jsonObj["position"] = std::move(position);
	nlohmann::ordered_json rotation;
	if (jsonFileVersion == 1)
	{
		// "rotation" must contain at least one components [x] - jsonFileVersion_v1 always expects 3 (and ignores all but the first)
		rotation = nlohmann::ordered_json::array();
		rotation.push_back(mapObj.direction);
		position.push_back(0);
		position.push_back(0);
	}
	else
	{
		// "rotation" - v2+ JSON file format supports direct (non-array) single value
		rotation = mapObj.direction;
	}
	jsonObj["rotation"] = std::move(rotation);
}

// MARK: - Helper functions for loading / saving binary (BJO) files

static uint32_t bjoScavengerSlot(uint32_t mapMaxPlayer)
{
	// For old binary file formats:
	// Scavengers used to always be in position 7, when scavengers were only supported in less than 8 player maps.
	// Scavengers should be in position N in N-player maps, where N ≥ 8.
	return std::max<uint32_t>(mapMaxPlayer, 7);
}

static int8_t bjoConvertPlayer(uint32_t bjoPlayer, uint32_t mapMaxPlayers)
{
	if (bjoScavengerSlot(mapMaxPlayers) == bjoPlayer)
	{
		return PLAYER_SCAVENGERS;
	}

	return bjoPlayer;
}

static uint32_t convertPlayerToBJOPlayer(int8_t player, uint32_t mapMaxPlayers)
{
	if (player == PLAYER_SCAVENGERS)
	{
		return bjoScavengerSlot(mapMaxPlayers);
	}

	return static_cast<uint32_t>(player);
}

template<typename T>
static uint32_t getLargestSpecifiedId(const std::vector<T>& container)
{
	uint32_t largestId = 0;
	for (const auto& item : container)
	{
		if (item.id.has_value() && item.id.value() > largestId)
		{
			largestId = item.id.value();
		}
	}
	return largestId;
}

// MARK: - Structure loading functions

#define SS_BUILT 1

static optional<FileLoadResult<Structure>> loadBJOStructureInit(const std::string& filename, uint32_t mapMaxPlayers, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	FileLoadResult<Structure> result;
	const auto &path = filename.c_str();

	auto pStream = mapIO.openBinaryStream(filename, BinaryIOStream::OpenMode::READ);
	if (!pStream)
	{
		// Failed to open file
		return nullopt;
	}

	debug(pCustomLogger, LOG_INFO, "Loading: %s", path);

	char aFileType[4];
	uint32_t version = 0;
	uint32_t quantity = 0;
	if (pStream->readBytes(aFileType, 4) != static_cast<size_t>(4)
		|| aFileType[0] != 's'
		|| aFileType[1] != 't'
		|| aFileType[2] != 'r'
		|| aFileType[3] != 'u'
		|| !pStream->readULE32(&version)
		|| !pStream->readULE32(&quantity))
	{
		debug(pCustomLogger, LOG_ERROR, "%s: Bad header", path);
		return nullopt;
	}
	result.fileFormatVersion = version;

	if (version < 7 || version > 8)
	{
		debug(pCustomLogger, LOG_ERROR, "%s: Unsupported binary struct file version: %" PRIu32 "", path, version);
		return nullopt;
	}

	size_t nameLength = 60;
	if (version <= 19)
	{
		nameLength = 40;
	}
	std::vector<char> nameBuff(nameLength, '\0');

	for (uint32_t i = 0; i < quantity; i++)
	{
		uint32_t id = 0;
		uint32_t x = 0, y = 0, z = 0;
		uint32_t direction = 0;
		uint32_t player = 0;

		int32_t inFire = 0;
		uint32_t periodicalDamageStart = 0;
		uint32_t periodicalDamage = 0;
		uint8_t status = 0;
		uint32_t capacity = 0;

		uint32_t dummy;
		uint8_t dummy8;
		int32_t dummyS32;

		if (pStream->readBytes(nameBuff.data(), nameLength) != nameLength
			|| !pStream->readULE32(&id)
			|| !pStream->readULE32(&x) || !pStream->readULE32(&y) || !pStream->readULE32(&z)
			|| !pStream->readULE32(&direction)
			|| !pStream->readULE32(&player)
			|| !pStream->readSLE32(&inFire) // BOOL inFire
			|| !pStream->readULE32(&periodicalDamageStart) // burnStart
			|| !pStream->readULE32(&periodicalDamage) // burnDamage
			|| !pStream->readULE8(&status)	// status - causes structure padding
			|| !pStream->readULE8(&dummy8)	// structure padding
			|| !pStream->readULE8(&dummy8)	// structure padding
			|| !pStream->readULE8(&dummy8) // structure padding
			|| !pStream->readSLE32(&dummyS32) // currentBuildPts - aligned on 4 byte boundary
			|| !pStream->readULE32(&dummy) // body
			|| !pStream->readULE32(&dummy) // armour
			|| !pStream->readULE32(&dummy) // resistance
			|| !pStream->readULE32(&dummy) // dummy1
			|| !pStream->readULE32(&dummy) // subjectInc
			|| !pStream->readULE32(&dummy) // timeStarted
			|| !pStream->readULE32(&dummy) // output
			|| !pStream->readULE32(&capacity) // capacity
			|| !pStream->readULE32(&dummy)) // quantity
		{
			debug(pCustomLogger, LOG_ERROR, "%s: Failed to read structure %" PRIu32 "", path, i);
			return nullopt;
		}

		Structure structure;
		structure.id = id;
		structure.name.assign(nameBuff.begin(), nameBuff.end());
		// TODO: Possibly handle collecting modules?
		structure.position.x = x;
		structure.position.y = y;
		// ignore z component
		structure.direction = DEG(direction);
		structure.player = bjoConvertPlayer(player, mapMaxPlayers);
		// check inFire
		if (inFire != 0)
		{
			debug(pCustomLogger, LOG_WARNING, "%s: Ignoring inFire(%" PRIi32 ") for struct %" PRIu32 "", path, inFire, i);
		}
		// check periodicalDamageStart
		if (periodicalDamageStart != 0)
		{
			debug(pCustomLogger, LOG_WARNING, "%s: Ignoring periodicalDamageStart(%" PRIu32 ") for struct %" PRIu32 "", path, periodicalDamageStart, i);
		}
		// check periodicalDamage
		if (periodicalDamage != 0)
		{
			debug(pCustomLogger, LOG_WARNING, "%s: Ignoring periodicalDamage(%" PRIu32 ") for struct %" PRIu32 "", path, periodicalDamage, i);
		}
		if (status != SS_BUILT)
		{
			debug(pCustomLogger, LOG_WARNING, "%s: Ignoring status(%" PRIu32 ") for struct %" PRIu32 "", path, static_cast<uint32_t>(status), i);
		}
		if (capacity != 0)
		{
			debug(pCustomLogger, LOG_WARNING, "%s: Ignoring capacity(%" PRIu32 ") for struct %" PRIu32 "", path, capacity, i);
		}
		// TODO: Sanity check struct position ?
		result.objects.push_back(std::move(structure));
	}
	// Check: extra bytes at end
	if (!pStream->endOfStream())
	{
		debug(pCustomLogger, LOG_WARNING, "%s: Unexpectedly did not reach end of stream - data may be corrupted", path);
	}
	return result;
}

static const std::unordered_set<std::string> knownStructureJSONKeys = { "name", "id", "position", "rotation", "player", "startpos", "modules" };

static optional<FileLoadResult<Structure>> loadJsonStructureInit(const std::string& filename, MapType mapType, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	FileLoadResult<Structure> result;
	const auto &path = filename.c_str();

	auto loadedResult = loadJsonObjectFromFile(filename, mapIO, pCustomLogger);
	if (!loadedResult.has_value())
	{
		// Failed to load JSON - rely on loadJsonFromFile to handle output of errors
		return nullopt;
	}

	debug(pCustomLogger, LOG_INFO, "Loading: %s", path);

	nlohmann::json& mRoot = loadedResult.value();

	auto detectedFormatVersion = jsonGetFileFormatVersion(filename, mRoot, pCustomLogger, 2);
	if (!detectedFormatVersion.has_value())
	{
		// jsonGetFileFormatVersion should have already logged the reason
		return nullopt;
	}
	result.fileFormatVersion = detectedFormatVersion.value();

	nlohmann::json* pStructuresRoot = jsonGetRootMapObjectsContainer(filename, mRoot, result.fileFormatVersion, "structures");
	if (!pStructuresRoot)
	{
		// jsonGetRootMapObjectsContainer should have already logged the reason
		return nullopt;
	}

	for (auto it : pStructuresRoot->items())
	{
		nlohmann::json& structureJson = it.value();
		if (!structureJson.is_object())
		{
			continue;
		}
		Structure structure;
		if (!jsonGetBaseMapObjectInfo(structure, result.fileFormatVersion, structureJson, {path, it.key().c_str()}, pCustomLogger, "name"))
		{
			// jsonGetBaseMapObjectInfo should have handled logging an error
			continue;
		}
		// the player is extracted from either "player" or "startpos" - see jsonGetPlayerFromObj
		auto player = jsonGetPlayerFromObj(structureJson, mapType, {path, it.key().c_str()}, pCustomLogger);
		if (player.has_value())
		{
			structure.player = player.value();
		}
		else
		{
			// Missing required "player" or "startpos" key for droid
			debug(pCustomLogger, LOG_ERROR, "%s: Missing required player/startpos key for structure: %s", path, it.key().c_str());
			continue;
		}
		// "modules" (capacity)
		auto it_modules = structureJson.find("modules");
		if (it_modules != structureJson.end())
		{
			if (!it_modules->is_number())
			{
				debug(pCustomLogger, LOG_ERROR, "%s: Unexpected type of \"modules\" key (expecting number) for: %s", path, it.key().c_str());
				continue;
			}
			nlohmann::json::number_unsigned_t modulesCount = it_modules->get<nlohmann::json::number_unsigned_t>();
			if (modulesCount > static_cast<nlohmann::json::number_unsigned_t>(std::numeric_limits<uint8_t>::max()))
			{
				// "modules" value exceeds maximum allowable value
				debug(pCustomLogger, LOG_ERROR, "%s: \"modules\" value exceeds maximum allowable value: %s", path, it.key().c_str());
				continue;
			}
			structure.modules = static_cast<uint8_t>(modulesCount);
		}

		// Sanity check for unknown / unprocessed keys
		for (auto& subItem : structureJson.items())
		{
			if (knownStructureJSONKeys.count(subItem.key()) == 0)
			{
				debug(pCustomLogger, LOG_SYNTAX_WARNING, "%s: Unexpected structure key \"%s\" for structure: %s", path, subItem.key().c_str(), it.key().c_str());
			}
		}

		result.objects.push_back(std::move(structure));
	}

	return result;
}

// MARK: - Structure writing functions

bool writeMapStructureInitBJO(const std::vector<Structure>& structures, uint32_t mapMaxPlayers, const std::string &filename, IOProvider& mapIO, uint32_t bjoFileVersion, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	const auto &path = filename.c_str();

	if (bjoFileVersion < 7 || bjoFileVersion > 8)
	{
		// Currently, we only support versions 7 + 8
		debug(pCustomLogger, LOG_ERROR, "Unsupported binary struct file version: %" PRIu32 "; cannot write to: %s", bjoFileVersion, path);
		return false;
	}

	auto pStream = mapIO.openBinaryStream(filename, BinaryIOStream::OpenMode::WRITE);

	if (!pStream)
	{
		debug(pCustomLogger, LOG_ERROR, "Failed to open file for writing: %s", path);
		return false;
	}

	debug(pCustomLogger, LOG_INFO, "Writing: %s", path);

	// write header
	char aFileType[4];
	aFileType[0] = 's';
	aFileType[1] = 't';
	aFileType[2] = 'r';
	aFileType[3] = 'u';
	uint32_t quantity = static_cast<uint32_t>(structures.size());
	if (pStream->writeBytes(aFileType, 4) != static_cast<size_t>(4)
		|| !pStream->writeULE32(bjoFileVersion)
		|| !pStream->writeULE32(quantity))
	{
		debug(pCustomLogger, LOG_ERROR, "Failed writing header to: %s", path);
		return false;
	}

	size_t nameFieldLength = 60;
	if (bjoFileVersion <= 19)
	{
		nameFieldLength = 40;
	}
	std::vector<char> bigZeroBuff(256, '\0');

	uint32_t nextAvailableStructureId = getLargestSpecifiedId(structures) + 1;

	for (uint32_t i = 0; i < quantity; i++)
	{
		const auto& structure = structures[i];
		size_t nameStrWriteLen = std::min(nameFieldLength, structure.name.size());
		if (nameStrWriteLen < structure.name.size())
		{
			// Warn about truncation!!
			debug(pCustomLogger, LOG_WARNING, "Structure's name exceeds the length supported by binary struct file version: %" PRIu32 "; output will be truncated", bjoFileVersion);
		}
		if (pStream->writeBytes(structure.name.c_str(), nameStrWriteLen) != nameStrWriteLen)
		{
			// Failed to write
			debug(pCustomLogger, LOG_ERROR, "Error writing structure %" PRIu32 "'s name to: %s", i, path);
			return false;
		}
		if (nameStrWriteLen < nameFieldLength
			&& pStream->writeBytes(bigZeroBuff.data(), (nameFieldLength - nameStrWriteLen)) != (nameFieldLength - nameStrWriteLen))
		{
			// Failed to pad with zeros
			debug(pCustomLogger, LOG_ERROR, "Error writing structure %" PRIu32 " to: %s", i, path);
			return false;
		}

		uint32_t structureId = 0;
		if (structure.id.has_value())
		{
			structureId = structure.id.value();
		}
		else
		{
			structureId = nextAvailableStructureId++;
		}

		uint32_t bjoPlayer = convertPlayerToBJOPlayer(structure.player, mapMaxPlayers);
		// TODO: Do campaign maps require scanvengers at player ID 7? (Theoretically only an issue for old campaign maps with > 7 players? And there shouldn't be any of those?)

		// TODO: Better way to handle if structure.modules > 0 ?
		if (structure.modules > 0)
		{
			debug(pCustomLogger, LOG_WARNING, "Structure modules (%" PRIu8 ") > 0. Conversion to old binary struct format (version: %" PRIu32 ") may be missing the modules.", structure.modules, bjoFileVersion);
		}

		if (!pStream->writeULE32(structureId)
			|| !pStream->writeULE32(structure.position.x) || !pStream->writeULE32(structure.position.y) || !pStream->writeULE32(0)
			|| !pStream->writeULE32(static_cast<uint32_t>(structure.direction)) // TODO: Conversion / manipulation needed?
			|| !pStream->writeULE32(bjoPlayer)
			|| !pStream->writeSLE32(0) // BOOL inFire
			|| !pStream->writeULE32(0) // burnStart
			|| !pStream->writeULE32(0) // burnDamage
			|| !pStream->writeULE8(SS_BUILT)	// status - causes structure padding
			// the following magic padding bytes match the behavior of flaME's structure output - they don't seem to be used, but why not?
			|| !pStream->writeULE8(26)	// structure padding
			|| !pStream->writeULE8(127)	// structure padding
			|| !pStream->writeULE8(0)   // structure padding
			//
			|| !pStream->writeSLE32(0) // currentBuildPts - aligned on 4 byte boundary
			|| !pStream->writeULE32(0) // body
			|| !pStream->writeULE32(0) // armour
			|| !pStream->writeULE32(0) // resistance
			|| !pStream->writeULE32(0) // dummy1
			|| !pStream->writeULE32(0) // subjectInc
			|| !pStream->writeULE32(0) // timeStarted
			|| !pStream->writeULE32(0) // output
			|| !pStream->writeULE32(0) // capacity
			|| !pStream->writeULE32(0)) // quantity
		{
			debug(pCustomLogger, LOG_ERROR, "Error writing structure %" PRIu32 " to: %s", i, path);
			return false;
		}
	}

	pStream.reset();
	return true;
}

bool writeMapStructureInitJSON(const std::vector<Structure>& structures, MapType mapType, const std::string &filename, IOProvider& mapIO, uint32_t jsonFileVersion, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	const auto &path = filename.c_str();
	if (jsonFileVersion > 2)
	{
		// Currently, we only support up to version 2 JSON files
		debug(pCustomLogger, LOG_ERROR, "Unsupported JSON structure file version: %" PRIu32 "; cannot write to: %s", jsonFileVersion, path);
		return false;
	}

	nlohmann::ordered_json mRoot = nlohmann::ordered_json::object();
	nlohmann::ordered_json* pStructuresRoot = &mRoot;
	if (jsonFileVersion > 1)
	{
		// Version 2+ JSON format contains a "version" key
		// And stores the objects in an array
		mRoot["version"] = jsonFileVersion;
		mRoot["structures"] = nlohmann::ordered_json::array();
		pStructuresRoot = &(mRoot.at("structures"));
	}

	size_t counter = 0;
	size_t structureCountMinLength = std::max<size_t>(numberOfDigitsInNumber(structures.size()), 1);
	for (const auto& structure : structures)
	{
		nlohmann::ordered_json structureObj = nlohmann::ordered_json::object();
		// write base object info (name, <id>, position, rotation)
		jsonSetBaseMapObjectInfo(structureObj, jsonFileVersion, structure, "name");
		// player
		if (!jsonSetPlayerOnObject(structureObj, mapType, structure.player))
		{
			debug(pCustomLogger, LOG_ERROR, "Invalid player number (%" PRIi8 ")", structure.player);
			return false;
		}
		// "modules" (capacity)
		if (structure.modules > 0)
		{
			structureObj["modules"] = structure.modules;
		}

		if (pStructuresRoot->is_object())
		{
			// add to the root object
			std::string counterStr = std::to_string(counter);
			leftPadStrToMinimumLength(counterStr, structureCountMinLength, '0');
			(*pStructuresRoot)["structure_" + counterStr] = std::move(structureObj);
		}
		else //if (pStructuresRoot->is_array())
		{
			pStructuresRoot->push_back(std::move(structureObj));
		}
	}

	// write out to file
	return saveOrderedJsonObjectToFile(mRoot, filename, mapIO);
}

bool writeMapStructureInit(const std::vector<Structure>& structures, uint32_t mapMaxPlayers, MapType mapType, const std::string &mapFolderPath, IOProvider& mapIO, OutputFormat format, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	bool retVal = false;
	switch (format)
	{
		case OutputFormat::VER1_BINARY_OLD:
			retVal = writeMapStructureInitBJO(structures, mapMaxPlayers, mapFolderPath + "/struct.bjo", mapIO, 8, pCustomLogger);
			break;
		case OutputFormat::VER2:
			retVal = writeMapStructureInitJSON(structures, mapType, mapFolderPath + "/struct.json", mapIO, 1, pCustomLogger);
			break;
		case OutputFormat::VER3:
			retVal = writeMapStructureInitJSON(structures, mapType, mapFolderPath + "/struct.json", mapIO, 2, pCustomLogger);
			break;
	}
	return retVal;
}

// MARK: - Droid loading functions

static optional<FileLoadResult<Droid>> loadBJODroidInit(const std::string& filename, uint32_t mapMaxPlayers, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	FileLoadResult<Droid> result;
	const auto &path = filename.c_str();

	auto pStream = mapIO.openBinaryStream(filename, BinaryIOStream::OpenMode::READ);
	if (!pStream)
	{
		// Failed to open file
		return nullopt;
	}

	debug(pCustomLogger, LOG_INFO, "Loading: %s", path);

	char aFileType[4];
	uint32_t version = 0;
	uint32_t quantity = 0;
	if (pStream->readBytes(aFileType, 4) != static_cast<size_t>(4)
		|| aFileType[0] != 'd'
		|| aFileType[1] != 'i'
		|| aFileType[2] != 'n'
		|| aFileType[3] != 't'
		|| !pStream->readULE32(&version)
		|| !pStream->readULE32(&quantity))
	{
		debug(pCustomLogger, LOG_ERROR, "%s: Bad header", path);
		return nullopt;
	}
	result.fileFormatVersion = version;

	size_t nameLength = 60;
	if (version <= 19)
	{
		nameLength = 40;
	}
	std::vector<char> nameBuff(nameLength, '\0');

	for (uint32_t i = 0; i < quantity; i++)
	{
		uint32_t id = 0;
		uint32_t x = 0, y = 0, z = 0;
		uint32_t direction = 0;
		uint32_t player = 0;
		int32_t inFire = 0;
		uint32_t periodicalDamageStart = 0;
		uint32_t periodicalDamage = 0;
		if (pStream->readBytes(nameBuff.data(), nameLength) != nameLength
			|| !pStream->readULE32(&id)
			|| !pStream->readULE32(&x) || !pStream->readULE32(&y) || !pStream->readULE32(&z)
			|| !pStream->readULE32(&direction)
			|| !pStream->readULE32(&player)
			|| !pStream->readSLE32(&inFire) // BOOL inFire
			|| !pStream->readULE32(&periodicalDamageStart) // burnStart
			|| !pStream->readULE32(&periodicalDamage)) // burnDamage
		{
			debug(pCustomLogger, LOG_ERROR, "%s: Failed to read droid %" PRIu32 "", path, i);
			return nullopt;
		}
		Droid droid;
		droid.id = id;
		droid.name.assign(nameBuff.begin(), nameBuff.end());
		droid.position.x = (x & ~TILE_MASK) + TILE_UNITS / 2;
		droid.position.y = (y & ~TILE_MASK) + TILE_UNITS / 2;
		// ignore z component
		droid.direction = DEG(direction);
		droid.player = bjoConvertPlayer(player, mapMaxPlayers);
		// check inFire
		if (inFire != 0)
		{
			debug(pCustomLogger, LOG_WARNING, "%s: Ignoring inFire(%" PRIi32 ") for droid %" PRIu32 "", path, inFire, i);
		}
		// check periodicalDamageStart
		if (periodicalDamageStart != 0)
		{
			debug(pCustomLogger, LOG_WARNING, "%s: Ignoring periodicalDamageStart(%" PRIu32 ") for droid %" PRIu32 "", path, periodicalDamageStart, i);
		}
		// check periodicalDamage
		if (periodicalDamage != 0)
		{
			debug(pCustomLogger, LOG_WARNING, "%s: Ignoring periodicalDamage(%" PRIu32 ") for droid %" PRIu32 "", path, periodicalDamage, i);
		}
		// TODO: Sanity check droid position ?
		result.objects.push_back(std::move(droid));
	}
	// Check: extra bytes at end
	if (!pStream->endOfStream())
	{
		debug(pCustomLogger, LOG_WARNING, "%s: Unexpectedly did not reach end of stream - data may be corrupted", path);
	}
	return result;
}

static const std::unordered_set<std::string> knownDroidJSONKeys = { "template", "id", "position", "rotation", "player", "startpos" };

static optional<FileLoadResult<Droid>> loadJsonDroidInit(const std::string& filename, MapType mapType, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	FileLoadResult<Droid> result;
	const auto &path = filename.c_str();

	auto loadedResult = loadJsonObjectFromFile(filename, mapIO, pCustomLogger);
	if (!loadedResult.has_value())
	{
		// Failed to load JSON - rely on loadJsonFromFile to handle output of errors
		return nullopt;
	}

	debug(pCustomLogger, LOG_INFO, "Loading: %s", path);

	nlohmann::json& mRoot = loadedResult.value();

	auto detectedFormatVersion = jsonGetFileFormatVersion(filename, mRoot, pCustomLogger, 2);
	if (!detectedFormatVersion.has_value())
	{
		// jsonGetFileFormatVersion should have already logged the reason
		return nullopt;
	}
	result.fileFormatVersion = detectedFormatVersion.value();

	nlohmann::json* pDroidsRoot = jsonGetRootMapObjectsContainer(filename, mRoot, result.fileFormatVersion, "droids");
	if (!pDroidsRoot)
	{
		// jsonGetRootMapObjectsContainer should have already logged the reason
		return nullopt;
	}

	for (auto it : pDroidsRoot->items())
	{
		nlohmann::json& droidJson = it.value();
		if (!droidJson.is_object())
		{
			continue;
		}
		Droid droid;
		if (!jsonGetBaseMapObjectInfo(droid, result.fileFormatVersion, droidJson, {path, it.key().c_str()}, pCustomLogger, "template"))
		{
			// jsonGetBaseMapObjectInfo should have handled logging an error
			continue;
		}
		// the player is extracted from either "player" or "startpos" - see jsonGetPlayerFromObj
		auto player = jsonGetPlayerFromObj(droidJson, mapType, {path, it.key().c_str()}, pCustomLogger);
		if (player.has_value())
		{
			droid.player = player.value();
		}
		else
		{
			// Missing required "player" or "startpos" key for droid
			debug(pCustomLogger, LOG_ERROR, "%s: Missing required player/startpos key for droid: %s", path, it.key().c_str());
			continue;
		}

		// Sanity check for unknown / unprocessed keys
		for (auto& subItem : droidJson.items())
		{
			if (knownDroidJSONKeys.count(subItem.key()) == 0)
			{
				debug(pCustomLogger, LOG_SYNTAX_WARNING, "%s: Unexpected structure key \"%s\" for structure: %s", path, subItem.key().c_str(), it.key().c_str());
			}
		}

		result.objects.push_back(std::move(droid));
	}

	return result;
}

// MARK: - Droid writing functions

bool writeMapDroidInitBJO(const std::vector<Droid>& droids, uint32_t mapMaxPlayers, const std::string &filename, IOProvider& mapIO, uint32_t bjoFileVersion, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	const auto &path = filename.c_str();

	if (bjoFileVersion > 39)
	{
		// Currently, we only support up to version 39
		debug(pCustomLogger, LOG_ERROR, "Unsupported binary droidinit file version: %" PRIu32 "; cannot write to: %s", bjoFileVersion, path);
		return false;
	}

	auto pStream = mapIO.openBinaryStream(filename, BinaryIOStream::OpenMode::WRITE);

	if (!pStream)
	{
		debug(pCustomLogger, LOG_ERROR, "Failed to open file for writing: %s", path);
		return false;
	}

	debug(pCustomLogger, LOG_INFO, "Writing: %s", path);

	// write header
	char aFileType[4];
	aFileType[0] = 'd';
	aFileType[1] = 'i';
	aFileType[2] = 'n';
	aFileType[3] = 't';
	uint32_t quantity = static_cast<uint32_t>(droids.size());
	if (pStream->writeBytes(aFileType, 4) != static_cast<size_t>(4)
		|| !pStream->writeULE32(bjoFileVersion)
		|| !pStream->writeULE32(quantity))
	{
		debug(pCustomLogger, LOG_ERROR, "Failed writing header to: %s", path);
		return false;
	}

	size_t nameFieldLength = 60;
	if (bjoFileVersion <= 19)
	{
		nameFieldLength = 40;
	}
	std::vector<char> bigZeroBuff(256, '\0');

	uint32_t nextAvailableDroidId = getLargestSpecifiedId(droids) + 1;

	for (uint32_t i = 0; i < quantity; i++)
	{
		const auto& droid = droids[i];
		size_t nameStrWriteLen = std::min(nameFieldLength, droid.name.size());
		if (nameStrWriteLen < droid.name.size())
		{
			// Warn about truncation!!
			debug(pCustomLogger, LOG_WARNING, "Droids's name exceeds the length supported by binary struct file version: %" PRIu32 "; output will be truncated", bjoFileVersion);
		}
		if (pStream->writeBytes(droid.name.c_str(), nameStrWriteLen) != nameStrWriteLen)
		{
			// Failed to write
			debug(pCustomLogger, LOG_ERROR, "Error writing structure %" PRIu32 "'s name to: %s", i, path);
			return false;
		}
		if (nameStrWriteLen < nameFieldLength
			&& pStream->writeBytes(bigZeroBuff.data(), (nameFieldLength - nameStrWriteLen)) != (nameFieldLength - nameStrWriteLen))
		{
			// Failed to pad with zeros
			debug(pCustomLogger, LOG_ERROR, "Error writing structure %" PRIu32 " to: %s", i, path);
			return false;
		}

		uint32_t droidId = 0;
		if (droid.id.has_value())
		{
			droidId = droid.id.value();
		}
		else
		{
			droidId = nextAvailableDroidId++;
		}

		uint32_t bjoPlayer = convertPlayerToBJOPlayer(droid.player, mapMaxPlayers);
		// TODO: Do campaign maps require scanvengers at player ID 7? (Theoretically only an issue for old campaign maps with > 7 players? And there shouldn't be any of those?)

		if (!pStream->writeULE32(droidId)
			|| !pStream->writeULE32(droid.position.x) || !pStream->writeULE32(droid.position.y) || !pStream->writeULE32(0)
			|| !pStream->writeULE32(static_cast<uint32_t>(droid.direction)) // TODO: Conversion / manipulation needed?
			|| !pStream->writeULE32(bjoPlayer)
			|| !pStream->writeSLE32(0) // BOOL inFire
			|| !pStream->writeULE32(0) // burnStart
			|| !pStream->writeULE32(0)) // burnDamage
		{
			debug(pCustomLogger, LOG_ERROR, "Error writing droid %" PRIu32 " to: %s", i, path);
			return false;
		}
	}

	pStream.reset();
	return true;
}

bool writeMapDroidInitJSON(const std::vector<Droid>& droids, MapType mapType, const std::string &filename, IOProvider& mapIO, uint32_t jsonFileVersion, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	const auto &path = filename.c_str();
	if (jsonFileVersion > 2)
	{
		// Currently, we only support up to version 2 JSON files
		debug(pCustomLogger, LOG_ERROR, "Unsupported JSON droidinit file version: %" PRIu32 "; cannot write to: %s", jsonFileVersion, path);
		return false;
	}

	nlohmann::ordered_json mRoot = nlohmann::ordered_json::object();
	nlohmann::ordered_json* pDroidsRoot = &mRoot;
	if (jsonFileVersion > 1)
	{
		// Version 2+ JSON format contains a "version" key
		// And stores the objects in an array
		mRoot["version"] = jsonFileVersion;
		mRoot["droids"] = nlohmann::ordered_json::array();
		pDroidsRoot = &(mRoot.at("droids"));
	}

	size_t counter = 0;
	size_t structureCountMinLength = std::max<size_t>(numberOfDigitsInNumber(droids.size()), 1);
	for (const auto& droid : droids)
	{
		nlohmann::ordered_json droidObj = nlohmann::ordered_json::object();
		// write base object info (name, <id>, position, rotation)
		jsonSetBaseMapObjectInfo(droidObj, jsonFileVersion, droid, "template");
		// player
		if (!jsonSetPlayerOnObject(droidObj, mapType, droid.player))
		{
			debug(pCustomLogger, LOG_ERROR, "Invalid player number (%" PRIi8 ")", droid.player);
			return false;
		}

		if (pDroidsRoot->is_object())
		{
			// add to the root object
			std::string counterStr = std::to_string(counter);
			leftPadStrToMinimumLength(counterStr, structureCountMinLength, '0');
			(*pDroidsRoot)["droid_" + counterStr] = std::move(droidObj);
		}
		else //if (pDroidsRoot->is_array())
		{
			pDroidsRoot->push_back(std::move(droidObj));
		}
	}

	// write out to file
	return saveOrderedJsonObjectToFile(mRoot, filename, mapIO);
}

bool writeMapDroidInit(const std::vector<Droid>& droids, uint32_t mapMaxPlayers, MapType mapType, const std::string &mapFolderPath, IOProvider& mapIO, OutputFormat format, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	bool retVal = false;
	switch (format)
	{
		case OutputFormat::VER1_BINARY_OLD:
			retVal = writeMapDroidInitBJO(droids, mapMaxPlayers, mapFolderPath + "/dinit.bjo", mapIO, 8, pCustomLogger);
			break;
		case OutputFormat::VER2:
			retVal = writeMapDroidInitJSON(droids, mapType, mapFolderPath + "/droid.json", mapIO, 1, pCustomLogger);
			break;
		case OutputFormat::VER3:
			retVal = writeMapDroidInitJSON(droids, mapType, mapFolderPath + "/droid.json", mapIO, 2, pCustomLogger);
			break;
	}
	return retVal;
}

// MARK: - Feature loading functions

static optional<FileLoadResult<Feature>> loadBJOFeatureInit(const std::string& filename, uint32_t mapMaxPlayers, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	FileLoadResult<Feature> result;
	const auto &path = filename.c_str();

	auto pStream = mapIO.openBinaryStream(filename, BinaryIOStream::OpenMode::READ);
	if (!pStream)
	{
		// Failed to open file
		return nullopt;
	}

	debug(pCustomLogger, LOG_INFO, "Loading: %s", path);

	char aFileType[4];
	uint32_t version = 0;
	uint32_t quantity = 0;
	if (pStream->readBytes(aFileType, 4) != static_cast<size_t>(4)
		|| aFileType[0] != 'f'
		|| aFileType[1] != 'e'
		|| aFileType[2] != 'a'
		|| aFileType[3] != 't'
		|| !pStream->readULE32(&version)
		|| !pStream->readULE32(&quantity))
	{
		debug(pCustomLogger, LOG_ERROR, "%s: Bad header", path);
		return nullopt;
	}
	result.fileFormatVersion = version;

	if (version < 7 || version > 19)
	{
		debug(pCustomLogger, LOG_ERROR, "%s: Unsupported binary feat file version: %" PRIu32 "", path, version);
		return nullopt;
	}

	size_t nameLength = 60;
	if (version <= 19)
	{
		nameLength = 40;
	}
	std::vector<char> nameBuff(nameLength, '\0');

	for (uint32_t i = 0; i < quantity; i++)
	{
		uint32_t id = 0;
		uint32_t x = 0, y = 0, z = 0;
		uint32_t direction = 0;
		uint32_t player = 0;

		int32_t inFire = 0;
		uint32_t periodicalDamageStart = 0;
		uint32_t periodicalDamage = 0;
		uint8_t visibility[8];

		if (pStream->readBytes(nameBuff.data(), nameLength) != nameLength
			|| !pStream->readULE32(&id)
			|| !pStream->readULE32(&x) || !pStream->readULE32(&y) || !pStream->readULE32(&z)
			|| !pStream->readULE32(&direction)
			|| !pStream->readULE32(&player)
			|| !pStream->readSLE32(&inFire) // BOOL inFire
			|| !pStream->readULE32(&periodicalDamageStart) // burnStart
			|| !pStream->readULE32(&periodicalDamage)) // burnDamage
		{
			debug(pCustomLogger, LOG_ERROR, "%s: Failed to read feature %" PRIu32 "", path, i);
			return nullopt;
		}
		if (version >= 14 && pStream->readBytes(&visibility, 8) != static_cast<size_t>(8))
		{
			debug(pCustomLogger, LOG_ERROR, "%s: Failed to read feature %" PRIu32 " visibility", path, i);
			return nullopt;
		}

		Feature feature;
		feature.id = id;
		feature.name.assign(nameBuff.begin(), nameBuff.end());
		feature.position.x = x;
		feature.position.y = y;
		// ignore z component
		feature.direction = DEG(direction);
		// check player - ONLY POSSIBLY USED FOR CAMPAIGN, but prior code always ignored it??
		if (player != mapMaxPlayers)
		{
			// Since the prior code did not actually utilize this value, just check and print a warning (for now) if we are ignoring it
			debug(pCustomLogger, LOG_WARNING, "%s: Ignoring player(%" PRIi32 ") for feature %" PRIu32 "", path, player, i);
		}
		// check inFire
		if (inFire != 0)
		{
			debug(pCustomLogger, LOG_WARNING, "%s: Ignoring inFire(%" PRIi32 ") for feature %" PRIu32 "", path, inFire, i);
		}
		// check periodicalDamageStart
		if (periodicalDamageStart != 0)
		{
			debug(pCustomLogger, LOG_WARNING, "%s: Ignoring periodicalDamageStart(%" PRIu32 ") for feature %" PRIu32 "", path, periodicalDamageStart, i);
		}
		// check periodicalDamage
		if (periodicalDamage != 0)
		{
			debug(pCustomLogger, LOG_WARNING, "%s: Ignoring periodicalDamage(%" PRIu32 ") for feature %" PRIu32 "", path, periodicalDamage, i);
		}
		if (version >= 14)
		{
			for (size_t visIdx = 0; visIdx < 8; visIdx++)
			{
				if (visibility[visIdx] != 0)
				{
					debug(pCustomLogger, LOG_WARNING, "%s: Ignoring non-0 visibility[%zu]=%" PRIu32 " for feature %" PRIu32 "", path, visIdx, static_cast<uint32_t>(visibility[visIdx]), i);
				}
			}
		}
		// TODO: Sanity check feature position ?
		result.objects.push_back(std::move(feature));
	}
	// Check: extra bytes at end
	if (!pStream->endOfStream())
	{
		debug(pCustomLogger, LOG_WARNING, "%s: Unexpectedly did not reach end of stream - data may be corrupted", path);
	}
	return result;
}

static const std::unordered_set<std::string> knownFeatureJSONKeys = { "name", "id", "position", "rotation", "player" };

static optional<FileLoadResult<Feature>> loadJsonFeatureInit(const std::string& filename, MapType mapType, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	FileLoadResult<Feature> result;
	const auto &path = filename.c_str();

	auto loadedResult = loadJsonObjectFromFile(filename, mapIO, pCustomLogger);
	if (!loadedResult.has_value())
	{
		// Failed to load JSON - rely on loadJsonFromFile to handle output of errors
		return nullopt;
	}

	debug(pCustomLogger, LOG_INFO, "Loading: %s", path);

	nlohmann::json& mRoot = loadedResult.value();

	auto detectedFormatVersion = jsonGetFileFormatVersion(filename, mRoot, pCustomLogger, 2);
	if (!detectedFormatVersion.has_value())
	{
		// jsonGetFileFormatVersion should have already logged the reason
		return nullopt;
	}
	result.fileFormatVersion = detectedFormatVersion.value();

	nlohmann::json* pFeaturesRoot = jsonGetRootMapObjectsContainer(filename, mRoot, result.fileFormatVersion, "features");
	if (!pFeaturesRoot)
	{
		// jsonGetRootMapObjectsContainer should have already logged the reason
		return nullopt;
	}

	for (auto it : pFeaturesRoot->items())
	{
		nlohmann::json& featureJson = it.value();
		if (!featureJson.is_object())
		{
			continue;
		}
		Feature feature;
		if (!jsonGetBaseMapObjectInfo(feature, result.fileFormatVersion, featureJson, {path, it.key().c_str()}, pCustomLogger, "name"))
		{
			// jsonGetBaseMapObjectInfo should have handled logging an error
			continue;
		}
		// Optional:
		// the player is extracted from "player" *only*
		// FIXME: Is this actually used for skirmish map feature init?
		auto player = jsonGetPlayerFromObj(featureJson, mapType, {path, it.key().c_str()}, pCustomLogger, true);
		if (player.has_value())
		{
			if (mapType == MapType::CAMPAIGN || mapType == MapType::SAVEGAME)
			{
				feature.player = player.value();
			}
			else
			{
				// player assignment for features is not expected for skirmish map init
				debug(pCustomLogger, LOG_WARNING, "%s: Ignoring assigned player (%" PRIu8 ") for feature: %s", path, player.value(), it.key().c_str());
			}
		}

		// Sanity check for unknown / unprocessed keys
		for (auto& subItem : featureJson.items())
		{
			if (knownFeatureJSONKeys.count(subItem.key()) == 0)
			{
				debug(pCustomLogger, LOG_SYNTAX_WARNING, "%s: Unexpected structure key \"%s\" for structure: %s", path, subItem.key().c_str(), it.key().c_str());
			}
		}

		result.objects.push_back(std::move(feature));
	}

	return result;
}

// MARK: - Feature writing functions

bool writeMapFeatureInitBJO(const std::vector<Feature>& features, uint32_t mapMaxPlayers, const std::string &filename, IOProvider& mapIO, uint32_t bjoFileVersion, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	const auto &path = filename.c_str();

	if (bjoFileVersion < 7 || bjoFileVersion > 19)
	{
		// Currently, we only support versions 7 - 19
		debug(pCustomLogger, LOG_ERROR, "Unsupported binary feat file version: %" PRIu32 "; cannot write to: %s", bjoFileVersion, path);
		return false;
	}

	auto pStream = mapIO.openBinaryStream(filename, BinaryIOStream::OpenMode::WRITE);

	if (!pStream)
	{
		debug(pCustomLogger, LOG_ERROR, "Failed to open file for writing: %s", path);
		return false;
	}

	debug(pCustomLogger, LOG_INFO, "Writing: %s", path);

	// write header
	char aFileType[4];
	aFileType[0] = 'f';
	aFileType[1] = 'e';
	aFileType[2] = 'a';
	aFileType[3] = 't';
	uint32_t quantity = static_cast<uint32_t>(features.size());
	if (pStream->writeBytes(aFileType, 4) != static_cast<size_t>(4)
		|| !pStream->writeULE32(bjoFileVersion)
		|| !pStream->writeULE32(quantity))
	{
		debug(pCustomLogger, LOG_ERROR, "Failed writing header to: %s", path);
		return false;
	}

	size_t nameFieldLength = 60;
	if (bjoFileVersion <= 19)
	{
		nameFieldLength = 40;
	}
	std::vector<char> bigZeroBuff(256, '\0');

	uint32_t nextAvailableFeatureId = getLargestSpecifiedId(features) + 1;

	for (uint32_t i = 0; i < quantity; i++)
	{
		const auto& feature = features[i];
		size_t nameStrWriteLen = std::min(nameFieldLength, feature.name.size());
		if (nameStrWriteLen < feature.name.size())
		{
			// Warn about truncation!!
			debug(pCustomLogger, LOG_WARNING, "Droids's name exceeds the length supported by binary struct file version: %" PRIu32 "; output will be truncated", bjoFileVersion);
		}
		if (pStream->writeBytes(feature.name.c_str(), nameStrWriteLen) != nameStrWriteLen)
		{
			// Failed to write
			debug(pCustomLogger, LOG_ERROR, "Error writing structure %" PRIu32 "'s name to: %s", i, path);
			return false;
		}
		if (nameStrWriteLen < nameFieldLength
			&& pStream->writeBytes(bigZeroBuff.data(), (nameFieldLength - nameStrWriteLen)) != (nameFieldLength - nameStrWriteLen))
		{
			// Failed to pad with zeros
			debug(pCustomLogger, LOG_ERROR, "Error writing structure %" PRIu32 " to: %s", i, path);
			return false;
		}

		uint32_t featureId = 0;
		if (feature.id.has_value())
		{
			featureId = feature.id.value();
		}
		else
		{
			featureId = nextAvailableFeatureId++;
		}

		uint32_t bjoPlayer = mapMaxPlayers;
		if (feature.player.has_value())
		{
			bjoPlayer = convertPlayerToBJOPlayer(feature.player.value(), mapMaxPlayers);
		}
		// TODO: Do campaign maps require scanvengers at player ID 7? (Theoretically only an issue for old campaign maps with > 7 players? And there shouldn't be any of those?)

		if (!pStream->writeULE32(featureId)
			|| !pStream->writeULE32(feature.position.x) || !pStream->writeULE32(feature.position.y) || !pStream->writeULE32(0)
			|| !pStream->writeULE32(static_cast<uint32_t>(feature.direction)) // TODO: Conversion / manipulation needed?
			|| !pStream->writeULE32(bjoPlayer)
			|| !pStream->writeSLE32(0) // BOOL inFire
			|| !pStream->writeULE32(0) // burnStart
			|| !pStream->writeULE32(0)) // burnDamage
		{
			debug(pCustomLogger, LOG_ERROR, "Error writing feature %" PRIu32 " to: %s", i, path);
			return false;
		}
		if (bjoFileVersion >= 14 && pStream->writeBytes(bigZeroBuff.data(), 8) != static_cast<size_t>(8))
		{
			debug(pCustomLogger, LOG_ERROR, "Error writing feature %" PRIu32 " visibility to: %s", i, path);
			return false;
		}
	}

	pStream.reset();
	return true;
}

bool writeMapFeatureInitJSON(const std::vector<Feature>& features, MapType mapType, const std::string &filename, IOProvider& mapIO, uint32_t jsonFileVersion, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	const auto &path = filename.c_str();
	if (jsonFileVersion > 2)
	{
		// Currently, we only support up to version 2 JSON files
		debug(pCustomLogger, LOG_ERROR, "Unsupported JSON feature file version: %" PRIu32 "; cannot write to: %s", jsonFileVersion, path);
		return false;
	}

	nlohmann::ordered_json mRoot = nlohmann::ordered_json::object();
	nlohmann::ordered_json *pFeaturesRoot = &mRoot;
	if (jsonFileVersion > 1)
	{
		// Version 2+ JSON format contains a "version" key
		// And stores the objects in an array
		mRoot["version"] = jsonFileVersion;
		mRoot["features"] = nlohmann::ordered_json::array();
		pFeaturesRoot = &(mRoot.at("features"));
	}

	size_t counter = 0;
	size_t structureCountMinLength = std::max<size_t>(numberOfDigitsInNumber(features.size()), 1);
	for (const auto& feature : features)
	{
		nlohmann::ordered_json featureObj = nlohmann::ordered_json::object();
		// write base object info (name, <id>, position, rotation)
		jsonSetBaseMapObjectInfo(featureObj, jsonFileVersion, feature, "name");
		// Optional:
		// the player is extracted from "player" *only*
		if (feature.player.has_value())
		{
			if (mapType == MapType::CAMPAIGN || mapType == MapType::SAVEGAME)
			{
				if (!jsonSetPlayerOnObject(featureObj, mapType, feature.player.value()))
				{
					debug(pCustomLogger, LOG_ERROR, "Invalid player number (%" PRIi8 ")", feature.player.value());
					return false;
				}
			}
			else
			{
				// player assignment for features is not expected for skirmish map init
				debug(pCustomLogger, LOG_WARNING, "Ignoring assigned player (%" PRIu8 ") for feature #%zu", feature.player.value(), counter);
			}
		}

		if (pFeaturesRoot->is_object())
		{
			// add to the root object
			std::string counterStr = std::to_string(counter);
			leftPadStrToMinimumLength(counterStr, structureCountMinLength, '0');
			(*pFeaturesRoot)["feature_" + counterStr] = std::move(featureObj);
		}
		else //if (pFeaturesRoot->is_array())
		{
			pFeaturesRoot->push_back(std::move(featureObj));
		}
	}

	// write out to file
	return saveOrderedJsonObjectToFile(mRoot, filename, mapIO);
}

bool writeMapFeatureInit(const std::vector<Feature>& features, uint32_t mapMaxPlayers, MapType mapType, const std::string &mapFolderPath, IOProvider& mapIO, OutputFormat format, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	bool retVal = false;
	switch (format)
	{
		case OutputFormat::VER1_BINARY_OLD:
			retVal = writeMapFeatureInitBJO(features, mapMaxPlayers, mapFolderPath + "/feat.bjo", mapIO, 8, pCustomLogger);
			break;
		case OutputFormat::VER2:
			retVal = writeMapFeatureInitJSON(features, mapType, mapFolderPath + "/feature.json", mapIO, 1, pCustomLogger);
			break;
		case OutputFormat::VER3:
			retVal = writeMapFeatureInitJSON(features, mapType, mapFolderPath + "/feature.json", mapIO, 2, pCustomLogger);
			break;
	}
	return retVal;
}

// MARK: - High-level Map implementation

// Construct an empty Map, for modification
Map::Map()
{
	m_mapData = std::make_shared<MapData>();
	m_structures = std::make_shared<std::vector<Structure>>();
	m_droids = std::make_shared<std::vector<Droid>>();
	m_features = std::make_shared<std::vector<Feature>>();
}

Map::Map(const std::string& mapFolderPath, MapType mapType, uint32_t mapMaxPlayers, std::unique_ptr<LoggingProtocol> logger, std::unique_ptr<IOProvider> mapIO)
: m_mapFolderPath(mapFolderPath)
, m_mapType(mapType)
, m_mapMaxPlayers(mapMaxPlayers)
, m_logger(std::move(logger))
, m_mapIO(std::move(mapIO))
{ }

// Load a map from a specified folder path + mapType + maxPlayers + random seed (only used for script-generated maps), optionally supplying:
// - previewOnly (set to true to shortcut processing of map details that don't factor into preview generation)
// - a logger
// - a WzMap::IOProvider
std::unique_ptr<Map> Map::loadFromPath(const std::string& mapFolderPath, MapType mapType, uint32_t mapMaxPlayers, uint32_t seed, bool previewOnly, std::unique_ptr<LoggingProtocol> logger, std::unique_ptr<IOProvider> mapIO)
{
	if (!mapIO)
	{
		debug(logger.get(), LOG_ERROR, "Missing required mapIO");
		return nullptr;
	}

	std::vector<char> fileData;
	// First, check for new game.js format
	std::string gameJSPath = mapFolderPath + "/" + "game.js";
	if (mapIO->loadFullFile(gameJSPath, fileData))
	{
		debug(logger.get(), LOG_INFO, "Loading: %s", gameJSPath.c_str());
		// Load script map, which actually loads everything
		auto result = runMapScript(fileData, gameJSPath, seed, false, logger.get());
		if (result)
		{
			result->m_mapFolderPath = mapFolderPath;
			result->m_mapType = mapType;
			result->m_mapMaxPlayers = mapMaxPlayers;
			result->m_mapIO = std::move(mapIO);
			result->m_logger = std::move(logger);
			result->m_wasScriptGenerated = true;
		}
		return result;
	}

	// Otherwise, construct a lazy-loading Map
	std::unique_ptr<Map> pMap = std::unique_ptr<Map>(new Map(mapFolderPath, mapType, mapMaxPlayers, std::move(logger), std::move(mapIO)));
	return pMap;
}

bool Map::exportMapToPath(Map& map, const std::string& mapFolderPath, MapType mapType, uint32_t mapMaxPlayers, OutputFormat format, std::unique_ptr<LoggingProtocol> logger, std::unique_ptr<IOProvider> mapIO)
{
	if (!mapIO)
	{
		debug(logger.get(), LOG_ERROR, "Missing required mapIO");
		return false;
	}

	auto mapData = map.mapData();
	if (!mapData)
	{
		debug(logger.get(), LOG_ERROR, "Failed to load / retrieve map data from: %s", map.mapFolderPath().c_str());
		return false;
	}
	if (!writeMapData(*(mapData.get()), mapFolderPath + "/game.map", *(mapIO.get()), format, logger.get()))
	{
		debug(logger.get(), LOG_ERROR, "Failed to write map data to path: %s", mapFolderPath.c_str());
		return false;
	}

	auto mapTerrainTypes = map.mapTerrainTypes();
	if (!mapTerrainTypes)
	{
		debug(logger.get(), LOG_ERROR, "Failed to load / retrieve map terrain type data from: %s", map.mapFolderPath().c_str());
		return false;
	}
	if (!writeTerrainTypes(*(mapTerrainTypes.get()), mapFolderPath + "/ttypes.ttp", *(mapIO.get()), format, logger.get()))
	{
		debug(logger.get(), LOG_ERROR, "Failed to write map terrain type data to path: %s", mapFolderPath.c_str());
		return false;
	}

	auto mapDroids = map.mapDroids();
	if (!mapDroids)
	{
		debug(logger.get(), LOG_ERROR, "Failed to load / retrieve map droids from: %s", map.mapFolderPath().c_str());
		return false;
	}
	if (!writeMapDroidInit(*(mapDroids.get()), mapMaxPlayers, mapType, mapFolderPath, *(mapIO.get()), format, logger.get()))
	{
		debug(logger.get(), LOG_ERROR, "Failed to write map droids to path: %s", mapFolderPath.c_str());
		return false;
	}

	auto mapFeatures = map.mapFeatures();
	if (!mapFeatures)
	{
		debug(logger.get(), LOG_ERROR, "Failed to load / retrieve map features from: %s", map.mapFolderPath().c_str());
		return false;
	}
	if (!writeMapFeatureInit(*(mapFeatures.get()), mapMaxPlayers, mapType, mapFolderPath, *(mapIO.get()), format, logger.get()))
	{
		debug(logger.get(), LOG_ERROR, "Failed to write map features to path: %s", mapFolderPath.c_str());
		return false;
	}

	auto mapStructures = map.mapStructures();
	if (!mapStructures)
	{
		debug(logger.get(), LOG_ERROR, "Failed to load / retrieve map structures from: %s", map.mapFolderPath().c_str());
		return false;
	}
	if (!writeMapStructureInit(*(mapStructures.get()), mapMaxPlayers, mapType, mapFolderPath, *(mapIO.get()), format, logger.get()))
	{
		debug(logger.get(), LOG_ERROR, "Failed to write map structures to path: %s", mapFolderPath.c_str());
		return false;
	}

	return true;
}

// Get the map data
// Returns nullptr if constructed for loading and the loading failed
std::shared_ptr<MapData> Map::mapData()
{
	if (m_mapData) { return m_mapData; }

	// otherwise, load the map data on first request
	if (!m_mapIO) { return nullptr; }
	auto loadResult = loadMapData_Internal(m_mapFolderPath + "/" + "game.map", *m_mapIO, m_logger.get());
	if (loadResult.has_value())
	{
		m_mapData = std::make_shared<MapData>(std::move(loadResult.value().mapData));
		m_fileVersions[MapFile::MapData] = LoadedFileVersion(LoadedFileVersion::FileType::BinaryBJO, loadResult.value().fileFormatVersion);
		return m_mapData;
	}
	return nullptr;
}

// Get the structures
// Returns nullptr if constructed for loading and the loading failed
std::shared_ptr<std::vector<Structure>> Map::mapStructures()
{
	if (m_structures) { return m_structures; }

	// Otherwise, load the data on first request
	if (!m_mapIO) { return nullptr; }
	// Try JSON first
	auto fileType = LoadedFileVersion::FileType::JSON;
	auto loadResult = loadJsonStructureInit(m_mapFolderPath + "/" + "struct.json", m_mapType, *m_mapIO, m_logger.get());
	if (!loadResult.has_value())
	{
		// Fallback to .bjo (old binary format)
		fileType = LoadedFileVersion::FileType::BinaryBJO;
		loadResult = loadBJOStructureInit(m_mapFolderPath + "/" + "struct.bjo", m_mapMaxPlayers, *m_mapIO, m_logger.get());
	}
	if (loadResult.has_value())
	{
		m_structures = std::make_shared<std::vector<Structure>>(std::move(loadResult.value().objects));
		m_fileVersions[MapFile::Structures] = LoadedFileVersion(fileType, loadResult.value().fileFormatVersion);
		return m_structures;
	}
	return nullptr;
}

// Get the droids
// Returns nullptr if constructed for loading and the loading failed
std::shared_ptr<std::vector<Droid>> Map::mapDroids()
{
	if (m_droids) { return m_droids; }

	// Otherwise, load the data on first request
	if (!m_mapIO) { return nullptr; }
	// Try JSON first
	auto fileType = LoadedFileVersion::FileType::JSON;
	auto loadResult = loadJsonDroidInit(m_mapFolderPath + "/" + "droid.json", m_mapType, *m_mapIO, m_logger.get());
	if (!loadResult.has_value())
	{
		// Fallback to .bjo (old binary format)
		fileType = LoadedFileVersion::FileType::BinaryBJO;
		loadResult = loadBJODroidInit(m_mapFolderPath + "/" + "dinit.bjo", m_mapMaxPlayers, *m_mapIO, m_logger.get());
	}
	if (loadResult.has_value())
	{
		m_droids = std::make_shared<std::vector<Droid>>(std::move(loadResult.value().objects));
		m_fileVersions[MapFile::Droids] = LoadedFileVersion(fileType, loadResult.value().fileFormatVersion);
		return m_droids;
	}
	return nullptr;
}

// Get the features
// Returns nullptr if constructed for loading and the loading failed
std::shared_ptr<std::vector<Feature>> Map::mapFeatures()
{
	if (m_features) { return m_features; }

	// Otherwise, load the data on first request
	if (!m_mapIO) { return nullptr; }
	// Try JSON first
	auto fileType = LoadedFileVersion::FileType::JSON;
	auto loadResult = loadJsonFeatureInit(m_mapFolderPath + "/" + "feature.json", m_mapType, *m_mapIO, m_logger.get());
	if (!loadResult.has_value())
	{
		// Fallback to .bjo (old binary format)
		fileType = LoadedFileVersion::FileType::BinaryBJO;
		loadResult = loadBJOFeatureInit(m_mapFolderPath + "/" + "feat.bjo", m_mapMaxPlayers, *m_mapIO, m_logger.get());
	}
	if (loadResult.has_value())
	{
		m_features = std::make_shared<std::vector<Feature>>(std::move(loadResult.value().objects));
		m_fileVersions[MapFile::Features] = LoadedFileVersion(fileType, loadResult.value().fileFormatVersion);
		return m_features;
	}
	return nullptr;
}

std::shared_ptr<TerrainTypeData> Map::mapTerrainTypes()
{
	if (m_terrainTypes) { return m_terrainTypes; }

	// Otherwise, load the data on first request
	if (!m_mapIO) { return nullptr; }
	auto loadResult = loadTerrainTypes(m_mapFolderPath + "/" + "ttypes.ttp", *m_mapIO, m_logger.get());
	if (loadResult)
	{
		m_terrainTypes = std::make_shared<TerrainTypeData>();
		m_terrainTypes = std::move(loadResult);
		return m_terrainTypes;
	}
	return nullptr;
}

uint32_t MapData::crcSumMapTiles(uint32_t crc)
{
	for (auto &o : mMapTiles)
	{
		crc = crcSumU16(crc, &o.height, 1);
		crc = crcSumU16(crc, &o.texture, 1);
	}
	return crc;
}

uint32_t Map::crcSumMapTiles(uint32_t crc)
{
	auto pMapData = mapData();
	if (!pMapData) { return crc; }
	return pMapData->crcSumMapTiles(crc);
}

uint32_t Map::crcSumStructures(uint32_t crc)
{
	auto pStructures = mapStructures();
	if (!pStructures) { return crc; }
	for (auto &o : *pStructures)
	{
		if (o.id.has_value())
		{
			crc = crcSumU32(crc, &(o.id.value()), 1);
		}
		crc = crcSum(crc, o.name.data(), o.name.length());
		crc = crcSumWorldPos(crc, &o.position, 1);
		crc = crcSumU16(crc, &o.direction, 1);
		crc = crcSum(crc, &o.modules, 1);
		crc = crcSum(crc, &o.player, 1);
	}
	return crc;
}

uint32_t Map::crcSumDroids(uint32_t crc)
{
	auto pDroids = mapDroids();
	if (!pDroids) { return crc; }
	for (auto &o : *pDroids)
	{
		if (o.id.has_value())
		{
			crc = crcSumU32(crc, &(o.id.value()), 1);
		}
		crc = crcSum(crc, o.name.data(), o.name.length());
		crc = crcSumWorldPos(crc, &o.position, 1);
		crc = crcSumU16(crc, &o.direction, 1);
		crc = crcSum(crc, &o.player, 1);
	}
	return crc;
}

uint32_t Map::crcSumFeatures(uint32_t crc)
{
	auto pFeatures = mapFeatures();
	if (!pFeatures) { return crc; }
	for (auto &o : *pFeatures)
	{
		if (o.id.has_value())
		{
			crc = crcSumU32(crc, &(o.id.value()), 1);
		}
		crc = crcSum(crc, o.name.data(), o.name.length());
		crc = crcSumWorldPos(crc, &o.position, 1);
		crc = crcSumU16(crc, &o.direction, 1);
		if (o.player.has_value())
		{
			crc = crcSum(crc, &(o.player.value()), 1);
		}
	}
	return crc;
}

optional<Map::LoadedFormat> Map::loadedMapFormat()
{
	// make sure we have all the pieces of a valid map
	if (!mapData()) { return nullopt; }
	if (!mapTerrainTypes()) { return nullopt; }
	if (!mapDroids()) { return nullopt; }
	if (!mapFeatures()) { return nullopt; }
	if (!mapStructures()) { return nullopt; }

	if (m_wasScriptGenerated)
	{
		return Map::LoadedFormat::SCRIPT_GENERATED;
	}

	auto getFileVersion = [this](MapFile file) -> optional<LoadedFileVersion>
	{
		auto it = m_fileVersions.find(file);
		if (it == m_fileVersions.end())
		{
			return nullopt;
		}
		return it->second;
	};

	// process the loaded map object file versions
	std::unordered_map<LoadedFileVersion::FileType, size_t, FileTypeFileHash> numFilesOfType;
	optional<uint32_t> lastFileVersion;
	for (auto& it : m_fileVersions)
	{
		// skip MapData and TerrainData, which are currently always binary file types
		if (it.first == MapFile::MapData || it.first == MapFile::TerrainTypes)
		{
			continue;
		}
		const auto& fileType = it.second.fileType();
		numFilesOfType[fileType]++;
		// check if there are any other file types
		for (auto& it_ft : numFilesOfType)
		{
			if (it_ft.first != fileType && it_ft.second > 0)
			{
				return Map::LoadedFormat::MIXED;
			}
		}
		if (fileType == LoadedFileVersion::FileType::JSON)
		{
			// Need to check for consistent version
			if (lastFileVersion.has_value())
			{
				if (lastFileVersion.value() != it.second.fileVersion())
				{
					// File version is different from all prior file versions - JSON maps are expected to have files of the same version
					return Map::LoadedFormat::MIXED;
				}
			}
			else
			{
				lastFileVersion = it.second.fileVersion();
			}
		}
	}

	// if we reached here, we should have all map object files of the same FileType
	if (numFilesOfType.size() != 1 || !lastFileVersion.has_value())
	{
		// or not? (presumably this isn't a loaded map)
		return nullopt;
	}

	auto fileType = numFilesOfType.begin()->first;
	switch (fileType)
	{
		case LoadedFileVersion::FileType::BinaryBJO:
			// for now, just return that everything is in binary file format
			return Map::LoadedFormat::BINARY_OLD;
			break;
		case LoadedFileVersion::FileType::JSON:
		{
			auto mapDataFileVersionResult = getFileVersion(MapFile::MapData);
			if (!mapDataFileVersionResult.has_value())
			{
				// should not happen
				return nullopt;
			}
			auto mapDataFileVersion = mapDataFileVersionResult.value();
			if (lastFileVersion.value() == 1)
			{
				return ((mapDataFileVersion.fileType() == LoadedFileVersion::FileType::BinaryBJO) && (mapDataFileVersion.fileVersion() < VERSION_40)) ? Map::LoadedFormat::JSON_v1 : Map::LoadedFormat::MIXED;
			}
			else if (lastFileVersion.value() == 2)
			{
				return ((mapDataFileVersion.fileType() == LoadedFileVersion::FileType::BinaryBJO) && (mapDataFileVersion.fileVersion() == VERSION_40)) ? Map::LoadedFormat::JSON_v2 : Map::LoadedFormat::MIXED;
			}
			else
			{
				// unknown JSON file version?
				return nullopt;
			}
			break;
		}
		case LoadedFileVersion::FileType::ScriptGenerated:
			return Map::LoadedFormat::SCRIPT_GENERATED;
			break;
	}

	return nullopt; // silence warning
}

} // namespace WzMap
