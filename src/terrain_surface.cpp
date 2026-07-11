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
#include <vector>

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

static float computeCornerSharpness(const WorldMapState& mapState, int x, int y)
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
	// Deltas are measured on the standing surface (max(ground, water)), not
	// the raw ground: a shore step's height is mostly hidden under water, and
	// what remains visible is the waterline, which should smooth rather than
	// keep the legacy faceting. Steps that stay large above the water level
	// (real cliffs with a submerged foot) still measure large and stay sharp.
	const float h = cornerHeight(mapState, x, y, HeightMode::Surface);
	float maxDelta = 0.f;
	maxDelta = std::max(maxDelta, std::fabs(cornerHeight(mapState, x - 1, y, HeightMode::Surface) - h));
	maxDelta = std::max(maxDelta, std::fabs(cornerHeight(mapState, x + 1, y, HeightMode::Surface) - h));
	maxDelta = std::max(maxDelta, std::fabs(cornerHeight(mapState, x, y - 1, HeightMode::Surface) - h));
	maxDelta = std::max(maxDelta, std::fabs(cornerHeight(mapState, x, y + 1, HeightMode::Surface) - h));

	float s = (maxDelta - SHARPNESS_DELTA_SMOOTH) / (SHARPNESS_DELTA_SHARP - SHARPNESS_DELTA_SMOOTH);
	s = std::min(1.f, std::max(0.f, s));
	return s * s * (3.f - 2.f * s); // smoothstep
}

// Per-corner surface caches. heightAt() reads four corner sharpness values
// per evaluation, a 4x4 stencil of sanitized water levels in water mode, and
// the shore weights (plus the water reference they gate) near the waterline
// in Ground/Surface modes. Computing each touches several neighboring
// corners and tiles, so
// recomputation dominates the dense mesh / field bake build cost. The caches
// are rebuilt in bulk on the main thread (full map at terrain init, dirty
// regions on deformation, before any surface evaluation runs) and are
// strictly read-only during evaluation - which keeps evaluation safe for the
// parallel bake workers. Out-of-range corners (the map border ring) fall back
// to direct computation.
struct PlateauRange
{
	float lo;
	float hi;
};

static std::vector<float> sharpnessCache;
static std::vector<float> waterLatticeCache;
// 1.0 within one corner ring of real water, 0.0 elsewhere (bilinearly
// interpolated by heightAt into a continuous shore-proximity weight)
static std::vector<float> shoreWeightCache;
// plateau ranges are mode-dependent. Only Ground (mesh builds, field bake)
// and Surface (unit settle, Classic) reach them - the water branch of
// heightAt() returns before the plateau lookup
static std::vector<PlateauRange> plateauGroundCache;
static std::vector<PlateauRange> plateauSurfaceCache;
static int surfaceCacheCornersX = 0;
static int surfaceCacheCornersY = 0;

static float computeWaterCornerHeight(const WorldMapState& mapState, int x, int y);
static float computeCornerShoreWeight(const WorldMapState& mapState, int x, int y);
static void computeCornerPlateauRange(const WorldMapState& mapState, int x, int y, HeightMode mode, float &lo, float &hi);

float cornerSharpness(const WorldMapState& mapState, int x, int y)
{
	if (x >= 0 && y >= 0 && x < surfaceCacheCornersX && y < surfaceCacheCornersY)
	{
		return sharpnessCache[static_cast<size_t>(y) * surfaceCacheCornersX + x];
	}
	return computeCornerSharpness(mapState, x, y);
}

static float waterCornerHeight(const WorldMapState& mapState, int x, int y)
{
	if (x >= 0 && y >= 0 && x < surfaceCacheCornersX && y < surfaceCacheCornersY)
	{
		return waterLatticeCache[static_cast<size_t>(y) * surfaceCacheCornersX + x];
	}
	return computeWaterCornerHeight(mapState, x, y);
}

static float cornerShoreWeight(const WorldMapState& mapState, int x, int y)
{
	if (x >= 0 && y >= 0 && x < surfaceCacheCornersX && y < surfaceCacheCornersY)
	{
		return shoreWeightCache[static_cast<size_t>(y) * surfaceCacheCornersX + x];
	}
	return computeCornerShoreWeight(mapState, x, y);
}

static void cornerPlateauRange(const WorldMapState& mapState, int x, int y, HeightMode mode, float &lo, float &hi)
{
	if (x >= 0 && y >= 0 && x < surfaceCacheCornersX && y < surfaceCacheCornersY)
	{
		if (mode == HeightMode::Ground)
		{
			const PlateauRange& r = plateauGroundCache[static_cast<size_t>(y) * surfaceCacheCornersX + x];
			lo = r.lo;
			hi = r.hi;
			return;
		}
		if (mode == HeightMode::Surface)
		{
			const PlateauRange& r = plateauSurfaceCache[static_cast<size_t>(y) * surfaceCacheCornersX + x];
			lo = r.lo;
			hi = r.hi;
			return;
		}
	}
	computeCornerPlateauRange(mapState, x, y, mode, lo, hi);
}

void rebuildSurfaceCaches(const WorldMapState& mapState)
{
	surfaceCacheCornersX = mapState.width + 1;
	surfaceCacheCornersY = mapState.height + 1;
	const size_t corners = static_cast<size_t>(surfaceCacheCornersX) * surfaceCacheCornersY;
	sharpnessCache.resize(corners);
	waterLatticeCache.resize(corners);
	shoreWeightCache.resize(corners);
	plateauGroundCache.resize(corners);
	plateauSurfaceCache.resize(corners);
	for (int y = 0; y < surfaceCacheCornersY; y++)
	{
		for (int x = 0; x < surfaceCacheCornersX; x++)
		{
			const size_t idx = static_cast<size_t>(y) * surfaceCacheCornersX + x;
			sharpnessCache[idx] = computeCornerSharpness(mapState, x, y);
			waterLatticeCache[idx] = computeWaterCornerHeight(mapState, x, y);
			shoreWeightCache[idx] = computeCornerShoreWeight(mapState, x, y);
			computeCornerPlateauRange(mapState, x, y, HeightMode::Ground, plateauGroundCache[idx].lo, plateauGroundCache[idx].hi);
			computeCornerPlateauRange(mapState, x, y, HeightMode::Surface, plateauSurfaceCache[idx].lo, plateauSurfaceCache[idx].hi);
		}
	}
}

void rebuildSurfaceCachesRegion(const WorldMapState& mapState, int minCornerX, int minCornerY, int maxCornerX, int maxCornerY)
{
	if (surfaceCacheCornersX != mapState.width + 1 || surfaceCacheCornersY != mapState.height + 1)
	{
		rebuildSurfaceCaches(mapState);
		return;
	}
	minCornerX = std::max(minCornerX, 0);
	minCornerY = std::max(minCornerY, 0);
	maxCornerX = std::min(maxCornerX, surfaceCacheCornersX - 1);
	maxCornerY = std::min(maxCornerY, surfaceCacheCornersY - 1);
	for (int y = minCornerY; y <= maxCornerY; y++)
	{
		for (int x = minCornerX; x <= maxCornerX; x++)
		{
			const size_t idx = static_cast<size_t>(y) * surfaceCacheCornersX + x;
			sharpnessCache[idx] = computeCornerSharpness(mapState, x, y);
			waterLatticeCache[idx] = computeWaterCornerHeight(mapState, x, y);
			shoreWeightCache[idx] = computeCornerShoreWeight(mapState, x, y);
			computeCornerPlateauRange(mapState, x, y, HeightMode::Ground, plateauGroundCache[idx].lo, plateauGroundCache[idx].hi);
			computeCornerPlateauRange(mapState, x, y, HeightMode::Surface, plateauSurfaceCache[idx].lo, plateauSurfaceCache[idx].hi);
		}
	}
}

void clearSurfaceCaches()
{
	sharpnessCache.clear();
	sharpnessCache.shrink_to_fit();
	waterLatticeCache.clear();
	waterLatticeCache.shrink_to_fit();
	shoreWeightCache.clear();
	shoreWeightCache.shrink_to_fit();
	plateauGroundCache.clear();
	plateauGroundCache.shrink_to_fit();
	plateauSurfaceCache.clear();
	plateauSurfaceCache.shrink_to_fit();
	surfaceCacheCornersX = 0;
	surfaceCacheCornersY = 0;
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
static void computeCornerPlateauRange(const WorldMapState& mapState, int x, int y, HeightMode mode, float &lo, float &hi)
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
/// level near shorelines. (Normally served from the surface caches above.)
static float computeWaterCornerHeight(const WorldMapState& mapState, int x, int y)
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

/// 1.0 for corners within one ring of a touching-water corner, 0.0 beyond.
/// Bilinear interpolation turns this into a continuous shore-proximity weight
/// that confines the shore profile fillet to real shorelines - inland terrain
/// that merely sits near the (extended) water level is untouched, and one ring
/// of extension keeps every weighted corner's sanitized water level meaningful.
static float computeCornerShoreWeight(const WorldMapState& mapState, int x, int y)
{
	for (int dy = -1; dy <= 1; dy++)
	{
		for (int dx = -1; dx <= 1; dx++)
		{
			if (cornerTouchesWater(mapState, x + dx, y + dy))
			{
				return 1.f;
			}
		}
	}
	return 0.f;
}

// MARK: - Shore profile fillet

// Height band around the waterline (in height units, each side) within which
// the ground profile is eased into the water plane, and the profile's slope
// multiplier right at the waterline. The remap is C1: slope reaches 1 again at
// the band edges, so terrain outside the band is untouched. Tunable.
static constexpr float SHORE_FILLET_BAND = 24.f;
static constexpr float SHORE_FILLET_ENTRY_SLOPE = 0.35f;

static float shoreFilletBand()
{
	// dev override for visual tuning: WZ_TERRAIN_SHORE_FILLET=<height units> (0 disables)
	static const float value = []() {
		float r = SHORE_FILLET_BAND;
		if (const char* env = getenv("WZ_TERRAIN_SHORE_FILLET"))
		{
			r = std::min(64.f, std::max(0.f, static_cast<float>(atof(env))));
		}
		return r;
	}();
	return value;
}

/// Ease the evaluated ground/surface height into the waterline: within the
/// fillet band around the local water level, remap the height so the profile
/// crosses the water plane at a reduced slope (a beach-like entry) instead of
/// the raw step gradient - softening the knee where terrain meets the water.
static float applyShoreFillet(const WorldMapState& mapState, int i, int j, float tx, float ty, float h)
{
	const float band = shoreFilletBand();
	if (band <= 0.f)
	{
		return h;
	}
	const float g00 = cornerShoreWeight(mapState, i, j);
	const float g10 = cornerShoreWeight(mapState, i + 1, j);
	const float g01 = cornerShoreWeight(mapState, i, j + 1);
	const float g11 = cornerShoreWeight(mapState, i + 1, j + 1);
	const float weight = terrainSurfaceMath::bilinear(g00, g10, g01, g11, tx, ty);
	if (weight <= 0.f)
	{
		return h;
	}
	// weight-masked bilinear of the sanitized water levels: only corners with
	// meaningful water values contribute, and the masked blend still agrees
	// across cell edges (the bilinear basis weights of excluded corners vanish
	// exactly where a neighboring cell would disagree about excluding them)
	const float b00 = (1.f - tx) * (1.f - ty) * g00;
	const float b10 = tx * (1.f - ty) * g10;
	const float b01 = (1.f - tx) * ty * g01;
	const float b11 = tx * ty * g11;
	const float bSum = b00 + b10 + b01 + b11;
	if (bSum <= 0.f)
	{
		return h;
	}
	const float water = (b00 * waterCornerHeight(mapState, i, j)
						 + b10 * waterCornerHeight(mapState, i + 1, j)
						 + b01 * waterCornerHeight(mapState, i, j + 1)
						 + b11 * waterCornerHeight(mapState, i + 1, j + 1)) / bSum;
	const float d = h - water;
	if (d <= -band || d >= band)
	{
		return h;
	}
	// cubic Hermite on |d|/band: p(0) = 0 with slope SHORE_FILLET_ENTRY_SLOPE,
	// p(1) = 1 with slope 1. C1 at the band edges and across the waterline,
	// monotone for any entry slope in (0, 1]
	const float s0 = SHORE_FILLET_ENTRY_SLOPE;
	const float t = std::fabs(d) / band;
	const float pt = ((s0 - 1.f) * t + (2.f - 2.f * s0)) * t * t + s0 * t;
	const float remapped = water + (d < 0.f ? -band * pt : band * pt);
	return h + (remapped - h) * weight;
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
	float result = smooth;
	if (sharp > 0.f)
	{
		float lo00, hi00, lo10, hi10, lo01, hi01, lo11, hi11;
		cornerPlateauRange(mapState, i, j, mode, lo00, hi00);
		cornerPlateauRange(mapState, i + 1, j, mode, lo10, hi10);
		cornerPlateauRange(mapState, i, j + 1, mode, lo01, hi01);
		cornerPlateauRange(mapState, i + 1, j + 1, mode, lo11, hi11);
		const float hi = terrainSurfaceMath::bilinear(hi00, hi10, hi01, hi11, tx, ty);
		const float lo = terrainSurfaceMath::bilinear(lo00, lo10, lo01, lo11, tx, ty);
		const float fan = terrainSurfaceMath::filletedFanSurface(p[1][1], p[2][1], p[1][2], p[2][2], hi, lo, cliffFilletRadius(), tx, ty);
		result = smooth + (fan - smooth) * sharp;
	}
	// Ground and Surface get the same shore remap from the same water
	// reference, so the drawn ground and the unit-settle surface stay
	// consistent across the shore band.
	return applyShoreFillet(mapState, i, j, tx, ty, result);
}

// Sampling radius (in tiles) for the central-difference vertex normals.
// This intentionally does NOT scale with the mesh subdivision factor: the
// legacy renderer's angle-weighted normals average over roughly a tile of
// neighborhood, and normals sampled at the (finer) subdivided mesh spacing
// make shadow-side slopes noticeably darker as the detail setting increases.
// A fixed radius keeps terrain lighting consistent across detail levels -
// the value is a middle ground between the subdivided mesh spacing (1/3 tile
// at High) and the legacy normals' averaging neighborhood (~1 tile).
static constexpr float NORMAL_SMOOTHING_RADIUS_TILES = 0.5f;

Vector3f worldNormalAt(const WorldMapState& mapState, float worldX, float worldY, HeightMode mode)
{
	const float eps = NORMAL_SMOOTHING_RADIUS_TILES * static_cast<float>(TILE_UNITS);
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

static bool isWaterTile(const WorldMapState& mapState, int i, int j)
{
	return tileOnMap(mapState, i, j) && terrainType(mapTile(mapState, i, j)) == TER_WATER;
}

// How far shoreline outline corners are pulled, in tiles (same fold-safety
// bound as the cliff rounding above). Tunable.
static constexpr float SHORE_OUTLINE_ROUNDING_TILES = 0.3f;

static float shoreOutlineRoundingWorld()
{
	// dev override for visual tuning: WZ_TERRAIN_SHORE_ROUND=<tiles> (0 disables)
	static const float value = []() {
		float tiles = SHORE_OUTLINE_ROUNDING_TILES;
		if (const char* env = getenv("WZ_TERRAIN_SHORE_ROUND"))
		{
			tiles = std::min(0.45f, std::max(0.f, static_cast<float>(atof(env))));
		}
		return tiles * static_cast<float>(TILE_UNITS);
	}();
	return value;
}

/// The minority-diagonal corner cut shared by cliff and shoreline outline
/// rounding: at a corner where exactly 1 (convex) or 3 (concave) of the 4
/// touching tiles match, pull the corner diagonally toward the minority tile.
static Vector2f outlineCornerCut(bool c00, bool c10, bool c01, bool c11, float magnitudeWorld)
{
	const int count = static_cast<int>(c00) + static_cast<int>(c10) + static_cast<int>(c01) + static_cast<int>(c11);
	if (count != 1 && count != 3)
	{
		return Vector2f(0.f, 0.f); // not on the outline, a straight run, or an ambiguous saddle
	}
	const bool minority = (count == 1);
	Vector2f dir;
	if (c00 == minority)      { dir = Vector2f(-1.f, -1.f); }
	else if (c10 == minority) { dir = Vector2f( 1.f, -1.f); }
	else if (c01 == minority) { dir = Vector2f(-1.f,  1.f); }
	else                      { dir = Vector2f( 1.f,  1.f); }
	return dir * (magnitudeWorld * 0.70710678f); // normalize the diagonal
}

Vector2f cornerOutlineOffset(const WorldMapState& mapState, int x, int y)
{
	// corner (x, y) touches tiles (x-1, y-1) .. (x, y)
	// NOTE: at concave lip corners the cliff cut pulls the (lower) cliff face
	// under positions on the passable plateau - rendered objects standing there
	// are settled onto the drawn surface via drawnHeightAt() (see terrain.cpp's
	// getTerrainVisualObjectHeightDelta) rather than by weakening the rounding:
	// a warp-direction rule cannot both smooth the outline and avoid the overlap.
	const Vector2f cliffCut = outlineCornerCut(
		isCliffTile(mapState, x - 1, y - 1), isCliffTile(mapState, x, y - 1),
		isCliffTile(mapState, x - 1, y),     isCliffTile(mapState, x, y),
		cliffOutlineRoundingWorld());
	if (cliffCut.x != 0.f || cliffCut.y != 0.f)
	{
		// cliffs win where both would apply, keeping the total displacement
		// inside the no-fold bound
		return cliffCut;
	}
	// the same cut keyed on water tiles rounds the shoreline's horizontal
	// outline: pointy inlet tips and land fingers get cut instead of following
	// the tile grid's stair-steps
	return outlineCornerCut(
		isWaterTile(mapState, x - 1, y - 1), isWaterTile(mapState, x, y - 1),
		isWaterTile(mapState, x - 1, y),     isWaterTile(mapState, x, y),
		shoreOutlineRoundingWorld());
}

Vector2f outlineOffsetAt(const WorldMapState& mapState, float worldX, float worldY)
{
	if (cliffOutlineRoundingWorld() <= 0.f && shoreOutlineRoundingWorld() <= 0.f)
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
