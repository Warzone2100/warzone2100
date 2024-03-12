/*
	This file is part of Warzone 2100.
	Copyright (C) 2022-2023  Warzone 2100 Project

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

enum TerrainShaderQuality
{
	UNINITIALIZED_PICK_DEFAULT = -1, // a non-selectable option for first-run - pick a default based on the current system settings
	CLASSIC = 0, // classic, pixel-art, tile-based textures
	MEDIUM = 1, // the mode used by at least WZ 3.2.x - 4.3.x
	NORMAL_MAPPING = 2	// the highest-quality mode, which adds normal / specular / height maps and advanced lighting
};
constexpr TerrainShaderQuality TerrainShaderQuality_MAX = TerrainShaderQuality::NORMAL_MAPPING;
