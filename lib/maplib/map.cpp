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

#define VERSION_39              39	        // lots of changes, breaking everything
#define CURRENT_VERSION_NUM     VERSION_39

#define MAX_PLAYERS         11                 ///< Maximum number of players in the game.

#define PLAYER_SCAVENGERS -1

// 65536 / 360 = 8192 / 45, with a bit less overflow risk.
#define DEG(degrees) ((degrees) * 8192 / 45)

// MARK: - Handling game.map / MapData files

namespace WzMap {

std::shared_ptr<MapData> loadMapData(const std::string &filename, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	const auto &path = filename.c_str();
	auto pStream = mapIO.openBinaryStream(filename, BinaryIOStream::OpenMode::READ);

	if (!pStream)
	{
		debug(pCustomLogger, LOG_ERROR, "%s not found", path);
		return nullptr;
	}

	debug(pCustomLogger, LOG_INFO, "Loading: %s", path);

	MapData map;
	char aFileType[4];
	if (pStream->readBytes(aFileType, 4) != static_cast<size_t>(4)
		|| !pStream->readULE32(&map.mapVersion)
		|| !pStream->readULE32(&map.width)
		|| !pStream->readULE32(&map.height)
		|| aFileType[0] != 'm'
		|| aFileType[1] != 'a'
		|| aFileType[2] != 'p')
	{
		debug(pCustomLogger, LOG_ERROR, "Bad header in %s", path);
		return nullptr;
	}

	if (map.mapVersion <= 9)
	{
		debug(pCustomLogger, LOG_ERROR, "%s: Unsupported save format version %u", path, map.mapVersion);
		return nullptr;
	}
	if (map.mapVersion > CURRENT_VERSION_NUM)
	{
		debug(pCustomLogger, LOG_ERROR, "%s: Undefined save format version %u", path, map.mapVersion);
		return nullptr;
	}
	if ((uint64_t)map.width * map.height > MAP_MAXAREA)
	{
		debug(pCustomLogger, LOG_ERROR, "Map %s too large : %d %d", path, map.width, map.height);
		return nullptr;
	}

	if (map.width <= 1 || map.height <= 1)
	{
		debug(pCustomLogger, LOG_ERROR, "Map %s is too small : %u, %u", path, map.width, map.height);
		return nullptr;
	}

	/* Load in the map data */
	uint32_t numMapTiles = map.width * map.height;
	for (uint32_t i = 0; i < numMapTiles; i++)
	{
		uint16_t texture;
		uint8_t height;

		if (!pStream->readULE16(&texture) || !pStream->readULE8(&height))
		{
			debug(pCustomLogger, LOG_ERROR, "%s: Error during savegame load", path);
			return nullptr;
		}
		MapData::MapTile currentMapTile{};
		currentMapTile.texture = texture;
		currentMapTile.height = static_cast<uint32_t>(height);
		map.mMapTiles.emplace_back(currentMapTile);
	}

	uint32_t gwVersion;
	uint32_t numGateways;
	if (!pStream->readULE32(&gwVersion) || !pStream->readULE32(&numGateways) || gwVersion != 1)
	{
		debug(pCustomLogger, LOG_ERROR, "Bad gateway in %s", path);
		return nullptr;
	}

	for (uint32_t i = 0; i < numGateways; i++)
	{
		MapData::Gateway gw = {};
		if (!pStream->readULE8(&gw.x1) || !pStream->readULE8(&gw.y1) || !pStream->readULE8(&gw.x2) ||
			!pStream->readULE8(&gw.y2))
		{
			debug(pCustomLogger, LOG_ERROR, "%s: Failed to read gateway info", path);
			return nullptr;
		}
		map.mGateways.emplace_back(gw);
	}

	return std::make_shared<MapData>(map);
}

// MARK: - Helper functions for loading JSON files
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

struct JsonParsingContext
{
	const char* filename;
	const char* jsonPath;
};

template <typename T>
static inline optional<std::vector<T>> jsonGetListOfType(const nlohmann::json& obj, const std::string& key, size_t minItems, size_t maxItems, const JsonParsingContext& jsonContext, LoggingProtocol* pCustomLogger = nullptr)
{
	std::vector<T> result;
	auto it = obj.find(key);
	if (it == obj.end())
	{
		return nullopt;
	}

	if (it->size() < minItems)
	{
		// Invalid rotation - requires at least one components (x)
		debug(pCustomLogger, LOG_ERROR, "%s: Invalid \"%s\" (requires at least %zu array members) for: %s", jsonContext.filename, key.c_str(), minItems, jsonContext.jsonPath);
		return nullopt;
	}

	for (size_t idx = 0; idx < minItems; idx++)
	{
		result.push_back(it->at(idx).get<T>());
	}

	if (it->size() > maxItems)
	{
		// Too many items
		debug(pCustomLogger, LOG_ERROR, "%s: Invalid \"%s\" (too many array members) for: %s", jsonContext.filename, key.c_str(), jsonContext.jsonPath);
	}
	else if (it->size() > minItems)
	{
		// check for non-zero additional members, and warn that they are being ignored
		for (size_t idx = minItems; idx < it->size(); idx++)
		{
			if (it->at(idx).get<T>() != 0)
			{
				debug(pCustomLogger, LOG_INFO_VERBOSE, "%s: Ignoring non-0 \"%s[%zu]\" for: %s", jsonContext.filename, key.c_str(), idx, jsonContext.jsonPath);
			}
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

uint32_t bjoScavengerSlot(uint32_t mapMaxPlayer)
{
	// For old binary file formats:
	// Scavengers used to always be in position 7, when scavengers were only supported in less than 8 player maps.
	// Scavengers should be in position N in N-player maps, where N ≥ 8.
	return std::max<uint32_t>(mapMaxPlayer, 7);
}

int8_t bjoConvertPlayer(uint32_t bjoPlayer, uint32_t mapMaxPlayers)
{
	if (bjoScavengerSlot(mapMaxPlayers) == bjoPlayer)
	{
		return PLAYER_SCAVENGERS;
	}

	return bjoPlayer;
}

// MARK: - Structure loading functions

#define SS_BUILT 1

static optional<std::vector<Structure>> loadBJOStructureInit(const std::string& filename, uint32_t mapMaxPlayers, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	std::vector<Structure> result;
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
		result.push_back(std::move(structure));
	}
	// Check: extra bytes at end
	if (!pStream->endOfStream())
	{
		debug(pCustomLogger, LOG_WARNING, "%s: Unexpectedly did not reach end of stream - data may be corrupted", path);
	}
	return result;
}

static const std::unordered_set<std::string> knownStructureJSONKeys = { "name", "id", "position", "rotation", "player", "startpos", "modules" };

static optional<std::vector<Structure>> loadJsonStructureInit(const std::string& filename, MapType mapType, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	std::vector<Structure> result;
	const auto &path = filename.c_str();

	auto loadedResult = loadJsonObjectFromFile(filename, mapIO, pCustomLogger);
	if (!loadedResult.has_value())
	{
		// Failed to load JSON - rely on loadJsonFromFile to handle output of errors
		return nullopt;
	}

	debug(pCustomLogger, LOG_INFO, "Loading: %s", path);

	nlohmann::json& mRoot = loadedResult.value();
	for (auto it : mRoot.items())
	{
		nlohmann::json& structureJson = it.value();
		if (!structureJson.is_object())
		{
			continue;
		}
		Structure structure;
		if (!structureJson.contains("name"))
		{
			// Missing required "template" key for droid
			debug(pCustomLogger, LOG_ERROR, "%s: Missing required \"name\" key for structure: %s", path, it.key().c_str());
			continue;
		}
		structure.name = structureJson["name"].get<std::string>();
		// "id" is explicitly optional - synchronized unit ids will be generated if it is omitted
		if (structureJson.contains("id"))
		{
			uint32_t id = structureJson["id"].get<uint32_t>();
			if (id == 0)
			{
				// Invalid droid id - cannot be 0
				debug(pCustomLogger, LOG_ERROR, "%s: Structure has invalid \"id\" = 0: %s", path, it.key().c_str());
				continue;
			}
			structure.id = id;
		}
		// "position" must contain at least two components [x, y]
		auto position = jsonGetListOfType<int>(structureJson, "position", 2, 3, {path, it.key().c_str()}, pCustomLogger);
		if (position.has_value())
		{
			structure.position.x = position.value()[0];
			structure.position.y = position.value()[1];
		}
		// "rotation" must contain at least one components [x]
		auto rotation = jsonGetListOfType<uint16_t>(structureJson, "rotation", 1, 3, {path, it.key().c_str()}, pCustomLogger);
		if (rotation.has_value())
		{
			structure.direction = rotation.value()[0];
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

		result.push_back(std::move(structure));
	}

	return result;
}

// MARK: - Droid loading functions

static optional<std::vector<Droid>> loadBJODroidInit(const std::string& filename, uint32_t mapMaxPlayers, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	std::vector<Droid> result;
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
		result.push_back(std::move(droid));
	}
	// Check: extra bytes at end
	if (!pStream->endOfStream())
	{
		debug(pCustomLogger, LOG_WARNING, "%s: Unexpectedly did not reach end of stream - data may be corrupted", path);
	}
	return result;
}

static const std::unordered_set<std::string> knownDroidJSONKeys = { "template", "id", "position", "rotation", "player", "startpos" };

static optional<std::vector<Droid>> loadJsonDroidInit(const std::string& filename, MapType mapType, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	std::vector<Droid> result;
	const auto &path = filename.c_str();

	auto loadedResult = loadJsonObjectFromFile(filename, mapIO, pCustomLogger);
	if (!loadedResult.has_value())
	{
		// Failed to load JSON - rely on loadJsonFromFile to handle output of errors
		return nullopt;
	}

	debug(pCustomLogger, LOG_INFO, "Loading: %s", path);

	nlohmann::json& mRoot = loadedResult.value();
	
	for (auto it : mRoot.items())
	{
		nlohmann::json& droidJson = it.value();
		if (!droidJson.is_object())
		{
			continue;
		}
		Droid droid;
		if (!droidJson.contains("template"))
		{
			// Missing required "template" key for droid
			debug(pCustomLogger, LOG_ERROR, "%s: Missing required \"template\" key for droid: %s", path, it.key().c_str());
			continue;
		}
		droid.name = droidJson["template"].get<std::string>();
		// "id" is explicitly optional - synchronized unit ids will be generated if it is omitted
		if (droidJson.contains("id"))
		{
			uint32_t id = droidJson["id"].get<uint32_t>();
			if (id == 0)
			{
				// Invalid droid id - cannot be 0
				debug(pCustomLogger, LOG_ERROR, "%s: Droid has invalid \"id\" = 0: %s", path, it.key().c_str());
				continue;
			}
			droid.id = id;
		}
		// "position" must contain at least two components [x, y]
		auto position = jsonGetListOfType<int>(droidJson, "position", 2, 3, {path, it.key().c_str()}, pCustomLogger);
		if (position.has_value())
		{
			droid.position.x = position.value()[0];
			droid.position.y = position.value()[1];
		}
		// "rotation" must contain at least one components [x]
		auto rotation = jsonGetListOfType<uint16_t>(droidJson, "rotation", 1, 3, {path, it.key().c_str()}, pCustomLogger);
		if (rotation.has_value())
		{
			droid.direction = rotation.value()[0];
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

		result.push_back(std::move(droid));
	}

	return result;
}

// MARK: - Feature loading functions

static optional<std::vector<Feature>> loadBJOFeatureInit(const std::string& filename, uint32_t mapMaxPlayers, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	std::vector<Feature> result;
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
		result.push_back(std::move(feature));
	}
	// Check: extra bytes at end
	if (!pStream->endOfStream())
	{
		debug(pCustomLogger, LOG_WARNING, "%s: Unexpectedly did not reach end of stream - data may be corrupted", path);
	}
	return result;
}

static const std::unordered_set<std::string> knownFeatureJSONKeys = { "name", "id", "position", "rotation", "player" };

static optional<std::vector<Feature>> loadJsonFeatureInit(const std::string& filename, MapType mapType, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr)
{
	std::vector<Feature> result;
	const auto &path = filename.c_str();

	auto loadedResult = loadJsonObjectFromFile(filename, mapIO, pCustomLogger);
	if (!loadedResult.has_value())
	{
		// Failed to load JSON - rely on loadJsonFromFile to handle output of errors
		return nullopt;
	}

	debug(pCustomLogger, LOG_INFO, "Loading: %s", path);

	nlohmann::json& mRoot = loadedResult.value();

	for (auto it : mRoot.items())
	{
		nlohmann::json& featureJson = it.value();
		if (!featureJson.is_object())
		{
			continue;
		}
		Feature feature;
		if (!featureJson.contains("name"))
		{
			// Missing required "template" key for droid
			debug(pCustomLogger, LOG_ERROR, "%s: Missing required \"name\" key for feature: %s", path, it.key().c_str());
			continue;
		}
		feature.name = featureJson["name"].get<std::string>();
		// "id" is explicitly optional - synchronized object ids will be generated if it is omitted
		if (featureJson.contains("id"))
		{
			uint32_t id = featureJson["id"].get<uint32_t>();
			if (id == 0)
			{
				// Invalid droid id - cannot be 0
				debug(pCustomLogger, LOG_ERROR, "%s: Feature has invalid \"id\" = 0: %s", path, it.key().c_str());
				continue;
			}
			feature.id = id;
		}
		// "position" must contain at least two components [x, y]
		auto position = jsonGetListOfType<int>(featureJson, "position", 2, 3, {path, it.key().c_str()}, pCustomLogger);
		if (position.has_value())
		{
			feature.position.x = position.value()[0];
			feature.position.y = position.value()[1];
		}
		// "rotation" must contain at least one components [x]
		auto rotation = jsonGetListOfType<uint16_t>(featureJson, "rotation", 1, 3, {path, it.key().c_str()}, pCustomLogger);
		if (rotation.has_value())
		{
			feature.direction = rotation.value()[0];
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

		result.push_back(std::move(feature));
	}

	return result;
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

// Load a map from a specified folder path + mayType + maxPlayers + random seed (only used for script-generated maps), optionally supplying:
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

// Get the map data
// Returns nullptr if constructed for loading and the loading failed
std::shared_ptr<MapData> Map::mapData()
{
	if (m_mapData) { return m_mapData; }

	// otherwise, load the map data on first request
	if (!m_mapIO) { return nullptr; }
	m_mapData = loadMapData(m_mapFolderPath + "/" + "game.map", *m_mapIO, m_logger.get());
	return m_mapData;
}

// Get the structures
// Returns nullptr if constructed for loading and the loading failed
std::shared_ptr<std::vector<Structure>> Map::mapStructures()
{
	if (m_structures) { return m_structures; }

	// Otherwise, load the data on first request
	if (!m_mapIO) { return nullptr; }
	// Try JSON first
	auto loadResult = loadJsonStructureInit(m_mapFolderPath + "/" + "struct.json", m_mapType, *m_mapIO, m_logger.get());
	if (!loadResult.has_value())
	{
		// Fallback to .bjo (old binary format)
		loadResult = loadBJOStructureInit(m_mapFolderPath + "/" + "struct.bjo", m_mapMaxPlayers, *m_mapIO, m_logger.get());
	}
	if (loadResult.has_value())
	{
		m_structures = std::make_shared<std::vector<Structure>>(std::move(loadResult.value()));
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
	auto loadResult = loadJsonDroidInit(m_mapFolderPath + "/" + "droid.json", m_mapType, *m_mapIO, m_logger.get());
	if (!loadResult.has_value())
	{
		// Fallback to .bjo (old binary format)
		loadResult = loadBJODroidInit(m_mapFolderPath + "/" + "dinit.bjo", m_mapMaxPlayers, *m_mapIO, m_logger.get());
	}
	if (loadResult.has_value())
	{
		m_droids = std::make_shared<std::vector<Droid>>(std::move(loadResult.value()));
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
	auto loadResult = loadJsonFeatureInit(m_mapFolderPath + "/" + "feature.json", m_mapType, *m_mapIO, m_logger.get());
	if (!loadResult.has_value())
	{
		// Fallback to .bjo (old binary format)
		loadResult = loadBJOFeatureInit(m_mapFolderPath + "/" + "feat.bjo", m_mapMaxPlayers, *m_mapIO, m_logger.get());
	}
	if (loadResult.has_value())
	{
		m_features = std::make_shared<std::vector<Feature>>(std::move(loadResult.value()));
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
		crc = crcSumU32(crc, &o.height, 1);
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

} // namespace WzMap
