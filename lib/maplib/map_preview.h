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
#pragma once
#include <vector>
#include <utility>
#include <unordered_map>
#include <memory>
#include <cinttypes>
#include "map.h"

namespace WzMap {

// High-level API for generating map preview images

class Map; // forward-declare

struct MapPreviewColor
{
	uint8_t r, g, b, a;
};

struct TilesetColorScheme
{
	MapPreviewColor plCliffL;
	MapPreviewColor plCliffH;
	MapPreviewColor plWater;
	MapPreviewColor plRoadL;
	MapPreviewColor plRoadH;
	MapPreviewColor plGroundL;
	MapPreviewColor plGroundH;

	// classic defines for various tilesets
	static TilesetColorScheme TilesetArizona();
	static TilesetColorScheme TilesetUrban();
	static TilesetColorScheme TilesetRockies();
};

class MapPlayerColorProvider
{
public:
	virtual ~MapPlayerColorProvider();

	// -1 = scavs
	virtual MapPreviewColor getPlayerColor(int8_t mapPlayer);
};

struct MapPreviewColorScheme
{
	TilesetColorScheme tilesetColors;
	MapPreviewColor hqColor;
	MapPreviewColor oilResourceColor;
	MapPreviewColor oilBarrelColor;
	std::unique_ptr<MapPlayerColorProvider> playerColorProvider;
};

struct MapPreviewImage
{
	std::vector<uint8_t> imageData;
	std::unordered_map<int8_t, std::pair<int32_t, int32_t>> playerHQPosition; // player hq position as pixel location / map coords
	uint32_t width;
	uint32_t height;
	uint32_t channels; // ex. 3 for RGB
};

std::unique_ptr<MapPreviewImage> generate2DMapPreview(WzMap::Map& wzMap, const MapPreviewColorScheme& colorScheme, WzMap::LoggingProtocol* pCustomLogger = nullptr);

} // namespace WzMap
