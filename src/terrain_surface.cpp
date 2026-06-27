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

#include <algorithm>
#include <cmath>
#include <cstdlib>

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

// Fillet radius (in height units) for rounding the lip/foot creases of the
// sharp (legacy-geometry) regions. Each rounded zone spans 2x this. Tunable.
static constexpr float CLIFF_FILLET_RADIUS = 20.f;

static float cliffFilletRadius()
{
	// dev override for visual tuning: WZ_TERRAIN_CLIFF_FILLET=<height units> (0 disables)
	static const float value = []() {
		float r = CLIFF_FILLET_RADIUS;
		if (const char* env = getenv("WZ_TERRAIN_CLIFF_FILLET"))
		{
			r = std::min(64.f, std::max(0.f, static_cast<float>(atof(env))));
		}
		return r;
	}();
	return value;
}

// Min/max corner height over the 3x3 corner neighborhood - the local floor and
// plateau reference levels for crease filleting. Lattice fields (bilinearly
// interpolated by the caller), so adjacent cells always agree. Every corner of
// a cell lies within every other corner's 3x3 neighborhood, so the interpolated
// range always brackets the cell's fan surface.
static void cornerPlateauRange(const WorldMapState& mapState, int x, int y, HeightMode mode, float &lo, float &hi)
{
	lo = hi = cornerHeight(mapState, x, y, mode);
	for (int dy = -1; dy <= 1; dy++)
	{
		for (int dx = -1; dx <= 1; dx++)
		{
			const float h = cornerHeight(mapState, x + dx, y + dy, mode);
			lo = std::min(lo, h);
			hi = std::max(hi, h);
		}
	}
}

/// Does lattice corner (x, y) touch a water tile? Only these corners' waterLevel
/// values are treated as meaningful by the renderer.
static bool cornerTouchesWater(const WorldMapState& mapState, int x, int y)
{
	for (int dy = -1; dy <= 0; dy++)
	{
		for (int dx = -1; dx <= 0; dx++)
		{
			if (tileOnMap(mapState, x + dx, y + dy) && terrainType(mapTile(mapState, x + dx, y + dy)) == TER_WATER)
			{
				return true;
			}
		}
	}
	return false;
}

/// The water lattice sanitized for smoothing: corners that don't touch water
/// take the highest touching-water level in their 3x3 neighborhood (one ring of
/// extension covers the bicubic stencil), so meaningless inland waterLevel
/// values can never drag the smoothed water surface below the water body's
/// level near shorelines.
static float waterCornerHeight(const WorldMapState& mapState, int x, int y)
{
	if (cornerTouchesWater(mapState, x, y))
	{
		return cornerHeight(mapState, x, y, HeightMode::Water);
	}
	bool found = false;
	float best = 0.f;
	for (int dy = -1; dy <= 1; dy++)
	{
		for (int dx = -1; dx <= 1; dx++)
		{
			if ((dx != 0 || dy != 0) && cornerTouchesWater(mapState, x + dx, y + dy))
			{
				const float h = cornerHeight(mapState, x + dx, y + dy, HeightMode::Water);
				best = found ? std::max(best, h) : h;
				found = true;
			}
		}
	}
	return found ? best : cornerHeight(mapState, x, y, HeightMode::Water);
}

float heightAt(const WorldMapState& mapState, float worldX, float worldY, HeightMode mode)
{
	const float fx = worldX / static_cast<float>(TILE_UNITS);
	const float fy = worldY / static_cast<float>(TILE_UNITS);
	const int i = static_cast<int>(std::floor(fx));
	const int j = static_cast<int>(std::floor(fy));
	const float tx = fx - static_cast<float>(i);
	const float ty = fy - static_cast<float>(j);

	if (mode == HeightMode::Water)
	{
		// Smooth over the sanitized water lattice (see waterCornerHeight): over
		// open water the lattice is locally constant and the monotone bicubic
		// reproduces it exactly (drawn water == simulation water level), while
		// shorelines get a smooth waterline instead of tile-resolution kinks.
		float p[4][4];
		for (int a = 0; a < 4; a++)
		{
			for (int b = 0; b < 4; b++)
			{
				p[a][b] = waterCornerHeight(mapState, i - 1 + a, j - 1 + b);
			}
		}
		return terrainSurfaceMath::monotoneBicubic(p, tx, ty);
	}

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

	// same blend as terrainSurfaceMath::blendedSurfaceHeight, but with the
	// sharp component's lip/foot creases filleted against the local plateau levels
	const float smooth = terrainSurfaceMath::monotoneBicubic(p, tx, ty);
	const float sharp = terrainSurfaceMath::bilinear(s00, s10, s01, s11, tx, ty);
	if (sharp <= 0.f)
	{
		return smooth;
	}
	float lo00, hi00, lo10, hi10, lo01, hi01, lo11, hi11;
	cornerPlateauRange(mapState, i, j, mode, lo00, hi00);
	cornerPlateauRange(mapState, i + 1, j, mode, lo10, hi10);
	cornerPlateauRange(mapState, i, j + 1, mode, lo01, hi01);
	cornerPlateauRange(mapState, i + 1, j + 1, mode, lo11, hi11);
	const float hi = terrainSurfaceMath::bilinear(hi00, hi10, hi01, hi11, tx, ty);
	const float lo = terrainSurfaceMath::bilinear(lo00, lo10, lo01, lo11, tx, ty);
	const float fan = terrainSurfaceMath::filletedFanSurface(p[1][1], p[2][1], p[1][2], p[2][2], hi, lo, cliffFilletRadius(), tx, ty);
	return smooth + (fan - smooth) * sharp;
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

// MARK: - Cliff outline rounding

// How far outline corners are pulled, in tiles. Must stay well below 0.5 so
// opposing offsets on a 1-tile-wide cliff strip can neither cross nor fold the
// warped mesh (worst-case component is this / sqrt(2) per corner).
static constexpr float CLIFF_OUTLINE_ROUNDING_TILES = 0.3f;

static float cliffOutlineRoundingWorld()
{
	// dev override for visual tuning: WZ_TERRAIN_CLIFF_ROUND=<tiles> (0 disables)
	static const float value = []() {
		float tiles = CLIFF_OUTLINE_ROUNDING_TILES;
		if (const char* env = getenv("WZ_TERRAIN_CLIFF_ROUND"))
		{
			tiles = std::min(0.45f, std::max(0.f, static_cast<float>(atof(env))));
		}
		return tiles * static_cast<float>(TILE_UNITS);
	}();
	return value;
}

static bool isCliffTile(const WorldMapState& mapState, int i, int j)
{
	return tileOnMap(mapState, i, j) && terrainType(mapTile(mapState, i, j)) == TER_CLIFFFACE;
}

Vector2f cornerOutlineOffset(const WorldMapState& mapState, int x, int y)
{
	// corner (x, y) touches tiles (x-1, y-1) .. (x, y)
	const bool c00 = isCliffTile(mapState, x - 1, y - 1);
	const bool c10 = isCliffTile(mapState, x,     y - 1);
	const bool c01 = isCliffTile(mapState, x - 1, y);
	const bool c11 = isCliffTile(mapState, x,     y);
	const int count = static_cast<int>(c00) + static_cast<int>(c10) + static_cast<int>(c01) + static_cast<int>(c11);
	if (count != 1 && count != 3)
	{
		return Vector2f(0.f, 0.f); // not on the outline, a straight run, or an ambiguous saddle
	}
	// cut the outline corner by pulling it diagonally toward the minority tile:
	// the lone cliff tile at convex corners (count 1), the lone gap at concave ones (count 3)
	const bool minority = (count == 1);
	Vector2f dir;
	if (c00 == minority)      { dir = Vector2f(-1.f, -1.f); }
	else if (c10 == minority) { dir = Vector2f( 1.f, -1.f); }
	else if (c01 == minority) { dir = Vector2f(-1.f,  1.f); }
	else                      { dir = Vector2f( 1.f,  1.f); }
	// NOTE: at concave lip corners this pulls the (lower) cliff face under
	// positions on the passable plateau - rendered objects standing there are
	// settled onto the drawn surface via drawnHeightAt() (see terrain.cpp's
	// getTerrainVisualObjectHeightDelta) rather than by weakening the rounding:
	// a warp-direction rule cannot both smooth the outline and avoid the overlap.
	return dir * (cliffOutlineRoundingWorld() * 0.70710678f); // normalize the diagonal
}

Vector2f outlineOffsetAt(const WorldMapState& mapState, float worldX, float worldY)
{
	if (cliffOutlineRoundingWorld() <= 0.f)
	{
		return Vector2f(0.f, 0.f);
	}
	const float fx = worldX / static_cast<float>(TILE_UNITS);
	const float fy = worldY / static_cast<float>(TILE_UNITS);
	const int i = static_cast<int>(std::floor(fx));
	const int j = static_cast<int>(std::floor(fy));
	const float tx = fx - static_cast<float>(i);
	const float ty = fy - static_cast<float>(j);

	const Vector2f o00 = cornerOutlineOffset(mapState, i, j);
	const Vector2f o10 = cornerOutlineOffset(mapState, i + 1, j);
	const Vector2f o01 = cornerOutlineOffset(mapState, i, j + 1);
	const Vector2f o11 = cornerOutlineOffset(mapState, i + 1, j + 1);
	const Vector2f b = o00 + (o10 - o00) * tx;
	const Vector2f t = o01 + (o11 - o01) * tx;
	return b + (t - b) * ty;
}

float drawnHeightAt(const WorldMapState& mapState, float worldX, float worldY, HeightMode mode)
{
	// The renderers draw the height sampled at u at the displaced position
	// u + outlineOffsetAt(u), so the drawn height at a fixed position P is
	// heightAt(u) with u + off(u) = P. Invert with two fixed-point steps -
	// off is small (well under half a tile) and smooth, so this converges
	// to sub-unit accuracy on standable ground.
	Vector2f u(worldX, worldY);
	for (int iteration = 0; iteration < 2; iteration++)
	{
		const Vector2f off = outlineOffsetAt(mapState, u.x, u.y);
		u = Vector2f(worldX - off.x, worldY - off.y);
	}
	return heightAt(mapState, u.x, u.y, mode);
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
