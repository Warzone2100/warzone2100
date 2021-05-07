/*
	This file is part of Warzone 2100.
	Copyright (C) 2013-2021  Warzone 2100 Project

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

#include "map_terrain_types.h"
#include "map.h"
#include "map_debug.h"
#include "map_internal.h"
#include <cinttypes>

/* Arbitrary maximum number of terrain textures - used in look up table for terrain type */
#define MAX_TILE_TEXTURES	255

namespace WzMap {

std::unique_ptr<TerrainTypeData> loadTerrainTypes(const std::string &filename, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	const auto &path = filename.c_str();

	auto pStream = mapIO.openBinaryStream(filename, BinaryIOStream::OpenMode::READ);
	if (!pStream)
	{
		// Failed to open file
		return nullptr;
	}

	debug(pCustomLogger, LOG_INFO, "Loading: %s", path);
	std::unique_ptr<TerrainTypeData> result(new TerrainTypeData());

	char aFileType[4];
	uint32_t version = 0;
	uint32_t quantity = 0;
	if (pStream->readBytes(aFileType, 4) != static_cast<size_t>(4)
		|| aFileType[0] != 't'
		|| aFileType[1] != 't'
		|| aFileType[2] != 'y'
		|| aFileType[3] != 'p'
		|| !pStream->readULE32(&version)
		|| !pStream->readULE32(&quantity))
	{
		debug(pCustomLogger, LOG_ERROR, "%s: Bad header", path);
		return nullptr;
	}

	if (version < 7 || version > 39)
	{
		debug(pCustomLogger, LOG_ERROR, "%s: Unsupported binary ttp file version: %" PRIu32 "", path, version);
		return nullptr;
	}

	if (quantity >= MAX_TILE_TEXTURES)
	{
		// Workaround for fugly map editor bug, since we can't fix the map editor
		quantity = MAX_TILE_TEXTURES - 1;
	}

	for (uint32_t i = 0; i < quantity; i++)
	{
		uint16_t pType = 0;
		if (!pStream->readULE16(&pType))
		{
			// Failed to read value
			debug(pCustomLogger, LOG_ERROR, "%s: Failed to read value # %" PRIu32 "", path, i);
			return nullptr;
		}

		if (pType > static_cast<uint16_t>(TER_MAX))
		{
			debug(pCustomLogger, LOG_ERROR, "%s: Terrain type #%" PRIu32 " (value: %" PRIu16 ") out of range", path, i, pType);
			return nullptr;
		}

		result->terrainTypes.push_back(static_cast<TYPE_OF_TERRAIN>(pType));
	}

	// Check: extra bytes at end
	if (!pStream->endOfStream())
	{
		debug(pCustomLogger, LOG_WARNING, "%s: Unexpectedly did not reach end of stream - data may be corrupted", path);
	}

	return result;
}

bool writeTerrainTypes(const TerrainTypeData& ttypeData, const std::string& filename, IOProvider& mapIO, OutputFormat format, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	const auto &path = filename.c_str();
	auto pStream = mapIO.openBinaryStream(filename, BinaryIOStream::OpenMode::WRITE);

	if (!pStream)
	{
		debug(pCustomLogger, LOG_ERROR, "Failed to open file for writing: %s", path);
		return false;
	}

	debug(pCustomLogger, LOG_INFO, "Writing: %s", path);

	uint32_t fileVersion = 8; // the format hasn't really changed
	switch (format)
	{
		case OutputFormat::VER1_BINARY_OLD:
			fileVersion = 8;	// flaME expects version 8
			break;
		case OutputFormat::VER2:
			fileVersion = 8; 	// stick with version 8 for now
			break;
		case OutputFormat::VER3:
			fileVersion = 39;	// use version 39 (the last version before maplib refactor) for now
			break;
	}
	uint32_t numTtypes = static_cast<uint32_t>(ttypeData.terrainTypes.size());

	// write header
	char aFileType[4];
	aFileType[0] = 't';
	aFileType[1] = 't';
	aFileType[2] = 'y';
	aFileType[3] = 'p';
	if (pStream->writeBytes(aFileType, 4) != static_cast<size_t>(4)
		|| !pStream->writeULE32(fileVersion)
		|| !pStream->writeULE32(numTtypes))
	{
		debug(pCustomLogger, LOG_ERROR, "Failed writing header to: %s", path);
		return false;
	}

	for (uint32_t i = 0; i < numTtypes; i++)
	{
		const auto& ttype = ttypeData.terrainTypes[i];
		uint16_t pType = static_cast<uint16_t>(ttype);
		if (!pStream->writeULE16(pType))
		{
			debug(pCustomLogger, LOG_ERROR, "Error writing terrain type data to: %s", path);
			return false;
		}
	}

	return true;
}

} // namespace WzMap
