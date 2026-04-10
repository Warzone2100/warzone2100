// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** \file
 * Per-world map state storage.
 */

#pragma once

#include "lib/framework/frame.h"
#include "gateway.h"

#include <memory>

#include <stdint.h>

#define AUX_MAP		0
#define AUX_ASTARMAP	1
#define AUX_DANGERMAP	2
#define AUX_MAX		3

struct BASE_OBJECT;

/// <summary>
/// Information stored with each tile on a given map.
/// </summary>
struct MAPTILE
{
	uint8_t         tileInfoBits;
	PlayerMask      tileExploredBits;
	PlayerMask      sensorBits;             ///< bit per player, who can see tile with sensor
	uint16_t        watchers[MAX_PLAYERS];  // player sees through fog of war here with this many objects
	uint16_t        texture;                // Which graphics texture is on this tile
	int32_t         height;                 ///< The height at the top left of the tile
	BASE_OBJECT *   psObject;               // Any object sitting on the location (e.g. building)
	uint16_t        limitedContinent;       ///< For land or sea limited propulsion types
	uint16_t        hoverContinent;         ///< For hover type propulsions
	uint16_t        fireEndTime;            ///< The (uint16_t)(gameTime / GAME_TICKS_PER_UPDATE) that BITS_ON_FIRE should be cleared.
	int32_t         waterLevel;             ///< At what height is the water for this tile
	PlayerMask      jammerBits;             ///< bit per player, who is jamming tile
	uint16_t        sensors[MAX_PLAYERS];   ///< player sees this tile with this many radar sensors
	uint16_t        jammers[MAX_PLAYERS];   ///< player jams the tile with this many objects

	// DISPLAY ONLY (NOT for use in game calculations)
	uint8_t         ground;                 ///< The ground type used for the terrain renderer
	uint8_t         illumination;           // How bright is this tile? = diffuseSunLight * ambientOcclusion
	uint8_t         ambientOcclusion;       // ambient occlusion. from 1 (max occlusion) to 254 (no occlusion), similar to illumination.
	float           level;                  ///< The visibility level of the top left of the tile, for this client. for terrain lightmap
};

/// <summary>
/// Scroll min and max values (per-world).
/// </summary>
struct WorldScrollLimits
{
	int32_t minX = 0;
	int32_t minY = 0;
	int32_t maxX = 0;
	int32_t maxY = 0;
};

/// <summary>
/// A simple wrapper around the map state (per-world).
/// </summary>
struct WorldMapState
{
	std::unique_ptr<MAPTILE[]> tiles;
	int32_t width = 0;
	int32_t height = 0;
	std::unique_ptr<uint8_t[]> blockMap[AUX_MAX];
	std::unique_ptr<uint8_t[]> auxMap[MAX_PLAYERS + AUX_MAX]; ///< yes, we waste one element... eyes wide open... makes API nicer
	WorldScrollLimits scroll;
	GATEWAY_LIST gateways;
};
