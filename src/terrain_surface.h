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
 * Smooth terrain surface for RENDERING ONLY.
 *
 * IMPORTANT: This surface is a per-client visual refinement of the height
 * lattice. It must NEVER be used by simulation code (movement, projectiles,
 * scripts, wzapi, anything synchronized) - the authoritative gameplay
 * surface is and remains map_Height() (src/map.h / src/map.cpp).
 * Including this header from simulation code is a bug.
 *
 * The interpolation math itself lives in terrain_surface_math.h (pure,
 * unit-tested standalone). This module binds it to WorldMapState using the
 * same corner-height rules as the terrain renderer's getGridPos()
 * (src/terrain.cpp), including the "map edge corners have height 0" rule.
 */

#pragma once

#include "lib/framework/vector.h"

struct WorldMapState;

namespace terrainSurface
{

/// Which height lattice to interpolate (mirrors the renderer's getGridPos modes)
enum class HeightMode
{
	Ground,   ///< tile ground heights (map_TileHeight)
	Water,    ///< water surface heights (map_WaterHeight)
	Surface,  ///< max(ground, water) (map_TileHeightSurface) - used by the unit ground settle and Classic mode
};

/// Height of a lattice corner as the renderer defines it:
/// 0 at/outside the map border (x <= 0, y <= 0, x >= width, y >= height),
/// otherwise the tile's height for the requested mode.
float cornerHeight(const WorldMapState& mapState, int x, int y, HeightMode mode);

/// Sharpness of a lattice corner in [0, 1]: 1 = the surface locally stays on
/// the legacy piecewise-linear geometry (cliffs, extreme steps),
/// 0 = fully smooth. Continuous in between, based on adjacent-tile terrain
/// type (TER_CLIFFFACE) and local corner-height deltas.
float cornerSharpness(const WorldMapState& mapState, int x, int y);

/// Smooth surface height at a map-space position in world units
/// (same x/y convention as map_Height() - the renderer's z = -y flip is NOT
/// applied here). C1-continuous where sharpness is 0. Follows the
/// (crease-filleted) legacy fan surface where sharpness is 1.
float heightAt(const WorldMapState& mapState, float worldX, float worldY, HeightMode mode);

/// Surface normal at a map-space position, returned in the renderer's world
/// space (world x = map x, world z = -map y, +y up) - directly usable as a
/// TerrainDecalVertex normal.
Vector3f worldNormalAt(const WorldMapState& mapState, float worldX, float worldY, HeightMode mode);

/// Horizontal displacement of a lattice corner, in map-space world
/// units, used to round the cliff outline: corners where the 4 adjacent tiles
/// are 1-vs-3 cliff/non-cliff are pulled toward the minority tile
/// (marching-squares-style corner cutting). Zero away from cliff outlines.
Vector2f cornerOutlineOffset(const WorldMapState& mapState, int x, int y);

/// Bilinear interpolation of cornerOutlineOffset at a map-space position.
/// Renderers add this to vertex x/y (z = -y in renderer world space) while
/// sampling heightAt() at the UNDISPLACED position - warping the drawn height
/// field so cliff lip/foot contours round. Bounded magnitude
/// (well below half a tile) keeps the warped mesh fold-free.
Vector2f outlineOffsetAt(const WorldMapState& mapState, float worldX, float worldY);

/// The height the renderer actually draws at a fixed map-space position:
/// heightAt() with the horizontal outline warp inverted. Use this (not
/// heightAt) when settling rendered objects onto the drawn ground.
float drawnHeightAt(const WorldMapState& mapState, float worldX, float worldY, HeightMode mode);

/// Log (LOG_TERRAIN) deviation statistics between the smooth surface and the
/// legacy fan surface over the whole map - cheap sanity metric for tuning.
void debugLogSurfaceStats(const WorldMapState& mapState);

} // namespace terrainSurface
