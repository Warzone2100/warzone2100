/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2023  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_PATH_CONTINENTS_H__
#define __INCLUDED_SRC_PATH_CONTINENTS_H__

#include <cstdint>
#include <vector>

/// Container for a "Continents"
/// A continent is an isolated area on the map.
class PathContinents {
public:
	struct ContinentInfo {
		/// Number of tiles in a continent.
		size_t size = 0;
		/// Blocking flags of a continent.
		uint8_t blockedBits;
		/// Starting position of a floodFill, used to build this continent.
		int x;
		int y;
	};
	using ContinentId = uint16_t;

	void generate();

	void clear();

	/// Get continent ID from tile coordinate.
	ContinentId getLand(int x, int y) const;

	/// Get continent ID from world coordinate.
	ContinentId getLand(const Position& worldPos) const;

	/// Get continent ID from tile coordinate.
	ContinentId getHover(int x, int y) const;

	/// Get continent ID from world coordinate.
	ContinentId getHover(const Position& worldPos) const;

protected:
	size_t mapFloodFill(int x, int y, ContinentId continent, uint8_t blockedBits, ContinentId* continentGrid);

	/// Local geometry of a map.
	/// It can differ from dimensions of global map.
	int width = 0, height = 0;
	int tileShift = 0;

	std::vector<ContinentInfo> continents;
	/// 2d layer of continent IDs for each map tile.
	std::vector<ContinentId> landGrid;
	std::vector<ContinentId> hoverGrid;

	ContinentId limitedContinents = 0;
	ContinentId hoverContinents = 0;
};

#endif // __INCLUDED_SRC_PATH_CONTINENTS_H__
