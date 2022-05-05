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

#include "../include/wzmaplib/map_debug.h"
#include "../include/wzmaplib/map_io.h"
#include "../include/wzmaplib/terrain_type.h"
#include "../include/wzmaplib/map.h"

#include <string>
#include <memory>

namespace WzMap {

bool levParseBasic(const char* data, size_t dataLen, IOProvider& mapIO, std::function<void (const std::string& command, const std::string& arg)> handleCommand, std::function<void (const std::string& commentLine)> handleCommentLine = nullptr, LoggingProtocol* pCustomLogger = nullptr);

optional<MAP_TILESET> convertLevMapTilesetType(std::string dataset);

optional<std::string> mapTilesetToClassicLevType(MapType mapType, MAP_TILESET mapTileset, unsigned techLevel, LoggingProtocol* pCustomLogger);

enum class SUPPORTED_LEVEL_TYPES : int
{
	CAMPAIGN = 12,
	SKIRMISH = 14,
	MULTI_SKIRMISH2 = 18, // all these old SKIRMISH entries in .lev files should be mapped to SKIRMISH
	MULTI_SKIRMISH3 = 19,
	MULTI_SKIRMISH4 = 20
};

optional<int> mapTypeToClassicLevType(MapType mapType, unsigned techLevel, LoggingProtocol* pCustomLogger);

} // namespace WzMap
