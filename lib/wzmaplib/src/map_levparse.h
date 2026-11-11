// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2022-2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
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
