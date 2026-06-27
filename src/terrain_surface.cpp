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
 * Smooth terrain surface for RENDERING ONLY - see terrain_surface.h.
 */

#include "terrain_surface.h"
#include "terrain_surface_math.h"
#include "map.h"

#include <glm/geometric.hpp>

#include <cmath>

namespace terrainSurface
{

// Sharpness fade for steep non-cliff steps, in height units (TILE_MAX_HEIGHT = 510):
// corner-height deltas up to SHARPNESS_DELTA_SMOOTH stay fully smooth, deltas of
// SHARPNESS_DELTA_SHARP and above stay on the legacy linear geometry. Tunable.
static constexpr float SHARPNESS_DELTA_SMOOTH = 48.f;
static constexpr float SHARPNESS_DELTA_SHARP = 128.f;

float cornerHeight(const WorldMapState& mapState, int x, int y, HeightMode mode)
{
	// the renderer (getGridPos) pins the map border ring to height 0
	if (x <= 0 || y <= 0 || x >= mapState.width || y >= mapState.height)
	{
		return 0.f;
	}
	switch (mode)
	{
		case HeightMode::Ground:  return static_cast<float>(map_TileHeight(mapState, x, y));
		case HeightMode::Water:   return static_cast<float>(map_WaterHeight(mapState, x, y));
		case HeightMode::Surface: return static_cast<float>(map_TileHeightSurface(mapState, x, y));
	}
	return 0.f;
}

float cornerSharpness(const WorldMapState& mapState, int x, int y)
{
	// cliffs must stay sharp: check the (up to) 4 tiles sharing this corner
	for (int dy = -1; dy <= 0; dy++)
	{
		for (int dx = -1; dx <= 0; dx++)
		{
			if (tileOnMap(mapState, x + dx, y + dy) && terrainType(mapTile(mapState, x + dx, y + dy)) == TER_CLIFFFACE)
			{
				return 1.f;
			}
		}
	}

	// steep non-cliff steps fade toward the legacy linear geometry, both to
	// avoid smoothing away deliberate map features and to bound the visual
	// deviation from map_Height() where the terrain is dramatic.
	const float h = cornerHeight(mapState, x, y, HeightMode::Ground);
	float maxDelta = 0.f;
	maxDelta = std::max(maxDelta, std::fabs(cornerHeight(mapState, x - 1, y, HeightMode::Ground) - h));
	maxDelta = std::max(maxDelta, std::fabs(cornerHeight(mapState, x + 1, y, HeightMode::Ground) - h));
	maxDelta = std::max(maxDelta, std::fabs(cornerHeight(mapState, x, y - 1, HeightMode::Ground) - h));
	maxDelta = std::max(maxDelta, std::fabs(cornerHeight(mapState, x, y + 1, HeightMode::Ground) - h));

	float s = (maxDelta - SHARPNESS_DELTA_SMOOTH) / (SHARPNESS_DELTA_SHARP - SHARPNESS_DELTA_SMOOTH);
	s = std::min(1.f, std::max(0.f, s));
	return s * s * (3.f - 2.f * s); // smoothstep
}

float heightAt(const WorldMapState& mapState, float worldX, float worldY, HeightMode mode)
{
	const float fx = worldX / static_cast<float>(TILE_UNITS);
	const float fy = worldY / static_cast<float>(TILE_UNITS);
	const int i = static_cast<int>(std::floor(fx));
	const int j = static_cast<int>(std::floor(fy));
	const float tx = fx - static_cast<float>(i);
	const float ty = fy - static_cast<float>(j);

	float p[4][4];
	for (int a = 0; a < 4; a++)
	{
		for (int b = 0; b < 4; b++)
		{
			p[a][b] = cornerHeight(mapState, i - 1 + a, j - 1 + b, mode);
		}
	}

	const float s00 = cornerSharpness(mapState, i, j);
	const float s10 = cornerSharpness(mapState, i + 1, j);
	const float s01 = cornerSharpness(mapState, i, j + 1);
	const float s11 = cornerSharpness(mapState, i + 1, j + 1);

	return terrainSurfaceMath::blendedSurfaceHeight(p, s00, s10, s01, s11, tx, ty);
}

Vector3f worldNormalAt(const WorldMapState& mapState, float worldX, float worldY, HeightMode mode)
{
	// central differences. The surface is piecewise-polynomial so a fixed
	// fraction of a tile is plenty of resolution
	const float eps = static_cast<float>(TILE_UNITS) / 8.f;
	const float hx = (heightAt(mapState, worldX + eps, worldY, mode) - heightAt(mapState, worldX - eps, worldY, mode)) / (2.f * eps);
	const float hy = (heightAt(mapState, worldX, worldY + eps, mode) - heightAt(mapState, worldX, worldY - eps, mode)) / (2.f * eps);
	// renderer world space: world x = map x, world z = -map y, +y up
	return glm::normalize(Vector3f(-hx, 1.f, hy));
}

void debugLogSurfaceStats(const WorldMapState& mapState)
{
	float maxDev = 0.f;
	double sumDev = 0.0;
	size_t samples = 0;
	int maxDevTileX = 0, maxDevTileY = 0;
	const int SUBSAMPLES = 4;
	for (int j = 0; j < mapState.height; j++)
	{
		for (int i = 0; i < mapState.width; i++)
		{
			for (int b = 0; b < SUBSAMPLES; b++)
			{
				for (int a = 0; a < SUBSAMPLES; a++)
				{
					const float tx = (static_cast<float>(a) + 0.5f) / SUBSAMPLES;
					const float ty = (static_cast<float>(b) + 0.5f) / SUBSAMPLES;
					const float wx = (static_cast<float>(i) + tx) * TILE_UNITS;
					const float wy = (static_cast<float>(j) + ty) * TILE_UNITS;
					const float smooth = heightAt(mapState, wx, wy, HeightMode::Ground);
					const float fan = terrainSurfaceMath::fanSurface(
						cornerHeight(mapState, i, j, HeightMode::Ground),
						cornerHeight(mapState, i + 1, j, HeightMode::Ground),
						cornerHeight(mapState, i, j + 1, HeightMode::Ground),
						cornerHeight(mapState, i + 1, j + 1, HeightMode::Ground),
						tx, ty);
					const float dev = std::fabs(smooth - fan);
					if (dev > maxDev)
					{
						maxDev = dev;
						maxDevTileX = i;
						maxDevTileY = j;
					}
					sumDev += dev;
					samples++;
				}
			}
		}
	}
	debug(LOG_TERRAIN, "smooth surface deviation from legacy surface: max %.2f at tile (%d, %d), mean %.3f (world height units, %zu samples)",
	      maxDev, maxDevTileX, maxDevTileY, (samples > 0) ? (sumDev / samples) : 0.0, samples);
}

} // namespace terrainSurface
