/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2021  Warzone 2100 Project

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

#include <optional-lite/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

#include "terrain_type.h"

namespace WzMap {

// MARK: - Output Formats

enum class OutputFormat
{
	VER1_BINARY_OLD, // FlaME-compatible / binary .bjo
	VER2, // JSON format (WZ 3.4+?)
	VER3  // JSONv2 format + full-range game.map (WZ 4.1+)
};
constexpr OutputFormat LatestOutputFormat = OutputFormat::VER3;

// MARK: - Handling game.map / MapData files

struct MapData
{
	struct Gateway
	{
		uint8_t x1, y1, x2, y2;
	};

	/* Information stored with each tile */
	struct MapTile
	{
		uint16_t height;	  // The height at the top left of the tile
		uint16_t texture; 	  // Which graphics texture is on this tile
	};

	uint32_t crcSumMapTiles(uint32_t crc);

	uint32_t height = 0;
	uint32_t width = 0;
	std::vector<MapTile> mMapTiles;

	std::vector<Gateway> mGateways;
};

// MARK: - Structures for map objects

struct WorldPos
{
	int x = 0;
	int y = 0;
};

struct Structure
{
	optional<uint32_t> id;
	std::string name;
	WorldPos position;
	uint16_t direction = 0;
	int8_t player = 0;  // -1 = scavs
	uint8_t modules = 0; // capacity
};
struct Droid
{
	optional<uint32_t> id;
	std::string name;
	WorldPos position;
	uint16_t direction = 0;
	int8_t player = 0;  // -1 = scavs
};
struct Feature
{
	optional<uint32_t> id; // an explicit feature id# override, used by older formats
	std::string name;
	WorldPos position;
	uint16_t direction = 0;
	optional<int8_t> player;
};

// MARK: - Terrain Type data

struct TerrainTypeData
{
	std::vector<TYPE_OF_TERRAIN> terrainTypes;
};

} // namespace WzMap
