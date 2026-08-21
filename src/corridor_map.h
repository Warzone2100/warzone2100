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
 *  Corridor detection.
 *
 *  Finds the narrow passages in a map so units can be coordinated into lanes
 *  before they enter one, rather than colliding as a blob inside it. A corridor
 *  is a stretch of passable ground whose width stays below a threshold, bounded
 *  where it opens back into free space or where passages meet.
 *
 *  The geometry is the medial axis of the passable area (the skeleton of a
 *  distance transform), kept only where the channel is narrow. Each corridor
 *  carries a centerline polyline in world coordinates, a width profile in world
 *  units, and its two mouths. Lane count later divides the physical width by the
 *  unit body diameter, so the width is stored in world units, not tiles.
 *
 *  Built from the tile passability of one ground propulsion. Every step is
 *  integer and runs in a fixed order, so the result is identical on every
 *  client, which matters because corridor ids and geometry will feed synced
 *  lane allocation.
 */

#pragma once

#include "lib/framework/vector.h"

#include <cstdint>
#include <memory>
#include <vector>

struct WorldMapState;

/// One narrow passage. The centerline runs mouth to mouth, and widthProfile is
/// parallel to it, one physical width in world units per centerline point.
struct Corridor
{
	int id = -1;
	std::vector<Vector2i> centerline;   ///< world coordinates, one point per skeleton tile
	std::vector<int32_t>  widthProfile; ///< physical passable width in world units, parallel to centerline
	std::vector<int32_t>  rightExtent;  ///< passable distance right of the centerline (index-increasing direction), world units
	std::vector<int32_t>  leftExtent;   ///< passable distance left of the centerline, world units
	Vector2i mouthA = Vector2i(0, 0);   ///< world coordinates of one end
	Vector2i mouthB = Vector2i(0, 0);   ///< world coordinates of the other end
	int32_t  minWidth = 0;              ///< narrowest cross-section, world units, governs lane count
};

/// Every corridor on a map, plus a per-tile lookup from tile index to the
/// corridor whose centerline runs through it, or -1.
struct CorridorMap
{
	int width = 0;
	int height = 0;
	std::vector<Corridor> corridors;
	std::vector<int16_t> tileCorridor;  ///< width*height, corridor id per centerline tile, -1 otherwise
	std::vector<uint8_t> tileClaimed;   ///< width*height, 1 where the corridor layer claims the ground, see corridorMapBuild
	std::vector<uint8_t> tileInterior;  ///< width*height, 1 near a centerline only, mouth approach zones excluded
	std::vector<int16_t> tileInteriorCorridor; ///< width*height, corridor id per interior tile, -1 otherwise, junction overlaps keep the later corridor
	std::vector<uint8_t> debugSkel;     ///< full one-tile skeleton, for the dump overlay
	std::vector<uint8_t> debugNarrow;   ///< skeleton restricted to narrow tiles, for the dump overlay

	int16_t at(int x, int y) const
	{
		if (x < 0 || y < 0 || x >= width || y >= height)
		{
			return -1;
		}
		return tileCorridor[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)];
	}

	/// True where corridor coordination owns or influences the ground, so
	/// other mechanisms can keep off it with one lookup. Out of bounds and
	/// corridor-free maps read as unclaimed.
	bool claimed(int x, int y) const
	{
		if (x < 0 || y < 0 || x >= width || y >= height || tileClaimed.empty())
		{
			return false;
		}
		return tileClaimed[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] != 0;
	}

	/// The corridor whose passage body covers this tile, or -1 outside every
	/// interior. Junction tiles shared by chained corridors carry one of them,
	/// which chain-level contest state makes equivalent.
	int16_t interiorCorridor(int x, int y) const
	{
		if (x < 0 || y < 0 || x >= width || y >= height || tileInteriorCorridor.empty())
		{
			return -1;
		}
		return tileInteriorCorridor[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)];
	}

	/// True only near a corridor centerline, the passage body itself.
	/// The mouth approach zones outside it are claimed but not interior.
	bool interior(int x, int y) const
	{
		if (x < 0 || y < 0 || x >= width || y >= height || tileInterior.empty())
		{
			return false;
		}
		return tileInterior[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] != 0;
	}
};

/// Detects the corridors on a map from its ground passability. Deterministic and
/// integer throughout. Returns an empty map if the state has no tiles.
std::unique_ptr<CorridorMap> corridorMapBuild(const WorldMapState& mapState);
