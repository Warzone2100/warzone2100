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

#include "../include/wzmaplib/map_preview.h"
#include "../include/wzmaplib/map.h"
#include "../include/wzmaplib/map_debug.h"
#include "map_internal.h"

namespace WzMap {

// ////////////////////////////////////////////////////////////////////////////
// tertile dependent colors for map preview

static inline MapPreviewColor mpc_Colour(uint8_t r, uint8_t g, uint8_t b)
{
	return { r, g, b, 255 };
}

// C1 - Arizona type
#define WZCOL_TERC1_CLIFF_LOW   mpc_Colour(0x68, 0x3C, 0x24)
#define WZCOL_TERC1_CLIFF_HIGH  mpc_Colour(0xE8, 0x84, 0x5C)
#define WZCOL_TERC1_WATER       mpc_Colour(0x3F, 0x68, 0x9A)
#define WZCOL_TERC1_ROAD_LOW    mpc_Colour(0x24, 0x1F, 0x16)
#define WZCOL_TERC1_ROAD_HIGH   mpc_Colour(0xB2, 0x9A, 0x66)
#define WZCOL_TERC1_GROUND_LOW  mpc_Colour(0x24, 0x1F, 0x16)
#define WZCOL_TERC1_GROUND_HIGH mpc_Colour(0xCC, 0xB2, 0x80)
// C2 - Urban type
#define WZCOL_TERC2_CLIFF_LOW   mpc_Colour(0x3C, 0x3C, 0x3C)
#define WZCOL_TERC2_CLIFF_HIGH  mpc_Colour(0x84, 0x84, 0x84)
#define WZCOL_TERC2_WATER       WZCOL_TERC1_WATER
#define WZCOL_TERC2_ROAD_LOW    mpc_Colour(0x00, 0x00, 0x00)
#define WZCOL_TERC2_ROAD_HIGH   mpc_Colour(0x24, 0x1F, 0x16)
#define WZCOL_TERC2_GROUND_LOW  mpc_Colour(0x1F, 0x1F, 0x1F)
#define WZCOL_TERC2_GROUND_HIGH mpc_Colour(0xB2, 0xB2, 0xB2)
// C3 - Rockies type
#define WZCOL_TERC3_CLIFF_LOW   mpc_Colour(0x3C, 0x3C, 0x3C)
#define WZCOL_TERC3_CLIFF_HIGH  mpc_Colour(0xFF, 0xFF, 0xFF)
#define WZCOL_TERC3_WATER       WZCOL_TERC1_WATER
#define WZCOL_TERC3_ROAD_LOW    mpc_Colour(0x24, 0x1F, 0x16)
#define WZCOL_TERC3_ROAD_HIGH   mpc_Colour(0x3D, 0x21, 0x0A)
#define WZCOL_TERC3_GROUND_LOW  mpc_Colour(0x00, 0x1C, 0x0E)
#define WZCOL_TERC3_GROUND_HIGH WZCOL_TERC3_CLIFF_HIGH

// ////////////////////////////////////////////////////////////////////////////

MapPlayerColorProvider::~MapPlayerColorProvider()
{
	// no-op
}

MapPreviewColor MapPlayerColorProvider::getPlayerColor(int8_t mapPlayer)
{
	return {0, 255, 2, 255}; // default: bright green
}

TilesetColorScheme TilesetColorScheme::TilesetArizona()
{
	TilesetColorScheme result;
	result.plCliffL = WZCOL_TERC1_CLIFF_LOW;
	result.plCliffH = WZCOL_TERC1_CLIFF_HIGH;
	result.plWater = WZCOL_TERC1_WATER;
	result.plRoadL = WZCOL_TERC1_ROAD_LOW;
	result.plRoadH = WZCOL_TERC1_ROAD_HIGH;
	result.plGroundL = WZCOL_TERC1_GROUND_LOW;
	result.plGroundH = WZCOL_TERC1_GROUND_HIGH;
	return result;
}
TilesetColorScheme TilesetColorScheme::TilesetUrban()
{
	TilesetColorScheme result;
	result.plCliffL = WZCOL_TERC2_CLIFF_LOW;
	result.plCliffH = WZCOL_TERC2_CLIFF_HIGH;
	result.plWater = WZCOL_TERC2_WATER;
	result.plRoadL = WZCOL_TERC2_ROAD_LOW;
	result.plRoadH = WZCOL_TERC2_ROAD_HIGH;
	result.plGroundL = WZCOL_TERC2_GROUND_LOW;
	result.plGroundH = WZCOL_TERC2_GROUND_HIGH;
	return result;
}
TilesetColorScheme TilesetColorScheme::TilesetRockies()
{
	TilesetColorScheme result;
	result.plCliffL = WZCOL_TERC3_CLIFF_LOW;
	result.plCliffH = WZCOL_TERC3_CLIFF_HIGH;
	result.plWater = WZCOL_TERC3_WATER;
	result.plRoadL = WZCOL_TERC3_ROAD_LOW;
	result.plRoadH = WZCOL_TERC3_ROAD_HIGH;
	result.plGroundL = WZCOL_TERC3_GROUND_LOW;
	result.plGroundH = WZCOL_TERC3_GROUND_HIGH;
	return result;
}

/*!
 * Clips x to boundaries
 * \param x Value to clip
 * \param min Lower bound
 * \param max Upper bound
 */
template<typename T>
static inline T clip(T x, T min, T max)
{
	// std::min and std::max not constexpr until C++14.
	return x < min ? min : x > max ? max : x;
}

static void plotBackdropPixel(MapPreviewImage& output, int32_t xx, int32_t yy, MapPreviewColor const &colour)
{
	xx = clip(xx, 0, static_cast<int32_t>(output.width - 1));
	yy = clip(yy, 0, static_cast<int32_t>(output.height - 1));
	uint8_t *pixel = output.imageData.data() + (yy * output.width + xx) * 3;
	pixel[0] = colour.r;
	pixel[1] = colour.g;
	pixel[2] = colour.b;
}

static inline bool isOilResource(const Feature& feature)
{
   return feature.name.rfind("OilResource", 0) == 0;
}

static inline bool isOilDrum(const Feature& feature)
{
	return feature.name.rfind("OilDrum", 0) == 0;
}

static inline bool isHQ(const Structure& structure)
{
	return structure.name.rfind("A0CommandCentre", 0) == 0;
}

// Show location of (at this time) oil on the map preview
static void plotWzMapFeature(Map &wzMap, const MapPreviewColorScheme& colorScheme, MapPreviewImage& output, LoggingProtocol* pCustomLogger)
{
	auto pFeatures = wzMap.mapFeatures();
	if (pFeatures == nullptr)
	{
		debug(pCustomLogger, LOG_ERROR, "Missing map features data");
		return;
	}

	for (auto &feature : *pFeatures)
	{
		MapPreviewColor color;
		if (isOilResource(feature))
		{
			color = colorScheme.oilResourceColor;
		}
		else if (isOilDrum(feature))
		{
			color = colorScheme.oilBarrelColor;
		}
		else
		{
			continue;
		}
		// and now we blit the color to the texture
		std::pair<int32_t, int32_t> pos = { map_coord(feature.position.x), map_coord(feature.position.y) };
		plotBackdropPixel(output, pos.first, pos.second, color);
	}
}

/**
 * \param[in]  wzMap The loaded WzMap
 * \param[out] backDropSprite The premade map texture.
 * \param[out] playeridpos    Will contain the position on the map where the player's HQ are located.
 *
 * Reads the map and colours the map preview for any structures
 * present. Additionally we load the player's HQ location into playeridpos so
 * we know the player's starting location.
 */
bool plotStructurePreviewWzMap(Map &wzMap, const MapPreviewColorScheme& colorScheme, MapPreviewImage& output, LoggingProtocol* pCustomLogger)
{
	auto pStructures = wzMap.mapStructures();
	if (pStructures == nullptr)
	{
		debug(pCustomLogger, LOG_ERROR, "Missing map structures data");
		return false;
	}

	std::unique_ptr<MapPlayerColorProvider> defaultPlayerColorProvider;

	MapPlayerColorProvider* pPlayerColorProvider = colorScheme.playerColorProvider.get();
	if (!pPlayerColorProvider)
	{
		defaultPlayerColorProvider = std::unique_ptr<MapPlayerColorProvider>(new MapPlayerColorProvider());
		pPlayerColorProvider = defaultPlayerColorProvider.get();
	}

	for (auto &structure : *pStructures)
	{
		bool HQ = isHQ(structure);
		std::pair<int32_t, int32_t> pos = { map_coord(structure.position.x), map_coord(structure.position.y) };
		if (HQ)
		{
			output.playerHQPosition[structure.player] = pos;
		}
		MapPreviewColor color = pPlayerColorProvider->getPlayerColor(structure.player);
		if (HQ)
		{
			// This shows where the HQ is on the map in a special color.
			// We could do the same for anything else (oil/whatever) also.
			// Possible future enhancement?
			color = colorScheme.hqColor;
		}
		// and now we blit the color to the texture
		plotBackdropPixel(output, pos.first, pos.second, color);
	}

	plotWzMapFeature(wzMap, colorScheme, output, pCustomLogger);

	return true;
}

static inline TYPE_OF_TERRAIN terrainTypeWzMap(const MapData::MapTile& tile, const TerrainTypeData& data)
{
	auto tileType = TileNumber_tile(tile.texture);
	if (tileType >= data.terrainTypes.size())
	{
		return static_cast<TYPE_OF_TERRAIN>(0);
	}
	return data.terrainTypes[tileType];
}

std::unique_ptr<MapPreviewImage> generate2DMapPreview(Map& wzMap, const MapPreviewColorScheme& colorScheme, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	auto mapData = wzMap.mapData();
	if (!mapData)
	{
		debug(pCustomLogger, LOG_ERROR, "Failed to load map data from: %s", wzMap.mapFolderPath().c_str());
		return nullptr;
	}
	auto mapTerrainTypes = wzMap.mapTerrainTypes();
	if (!mapTerrainTypes)
	{
		debug(pCustomLogger, LOG_ERROR, "Failed to load terrain types from: %s", wzMap.mapFolderPath().c_str());
		return nullptr;
	}

	const TilesetColorScheme& clrSch = colorScheme.tilesetColors;

	std::unique_ptr<MapPreviewImage> result = std::unique_ptr<MapPreviewImage>(new MapPreviewImage());
	result->width = mapData->width;
	result->height = mapData->height;
	result->channels = 3;

	size_t oursize = sizeof(char) * mapData->width * mapData->height;
	result->imageData.resize(oursize * 3, 0);
	auto& imageData = result->imageData;

	MapData::MapTile *psTile = &(mapData->mMapTiles[0]);
	MapData::MapTile *WTile = nullptr;

	for (uint32_t y = 0; y < mapData->height; ++y)
	{
		WTile = psTile;
		for (uint32_t x = 0; x < mapData->width; ++x)
		{
			uint8_t *const p = imageData.data() + (3 * (y * mapData->width + x));
			uint32_t col = WTile->height / ELEVATION_SCALE;

			switch (terrainTypeWzMap(*WTile, *mapTerrainTypes))
			{
			case TER_CLIFFFACE:
				p[0] = clrSch.plCliffL.r + (clrSch.plCliffH.r - clrSch.plCliffL.r) * col / 256;
				p[1] = clrSch.plCliffL.g + (clrSch.plCliffH.g - clrSch.plCliffL.g) * col / 256;
				p[2] = clrSch.plCliffL.b + (clrSch.plCliffH.b - clrSch.plCliffL.b) * col / 256;
				break;
			case TER_WATER:
				p[0] = clrSch.plWater.r;
				p[1] = clrSch.plWater.g;
				p[2] = clrSch.plWater.b;
				break;
			case TER_ROAD:
				p[0] = clrSch.plRoadL.r + (clrSch.plRoadH.r - clrSch.plRoadL.r) * col / 256;
				p[1] = clrSch.plRoadL.g + (clrSch.plRoadH.g - clrSch.plRoadL.g) * col / 256;
				p[2] = clrSch.plRoadL.b + (clrSch.plRoadH.b - clrSch.plRoadL.b) * col / 256;
				break;
			default:
				p[0] = clrSch.plGroundL.r + (clrSch.plGroundH.r - clrSch.plGroundL.r) * col / 256;
				p[1] = clrSch.plGroundL.g + (clrSch.plGroundH.g - clrSch.plGroundL.g) * col / 256;
				p[2] = clrSch.plGroundL.b + (clrSch.plGroundH.b - clrSch.plGroundL.b) * col / 256;
				break;
			}
			WTile += 1;
		}
		psTile += mapData->width;
	}

	// color our texture with clancolors @ correct position
	plotStructurePreviewWzMap(wzMap, colorScheme, *(result.get()), pCustomLogger);

	return result;
}

} // namespace WzMap
