/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_TERRAIN_H__
#define __INCLUDED_SRC_TERRAIN_H__

#include <glm/fwd.hpp>
#include "lib/ivis_opengl/pietypes.h"
#include <wzmaplib/terrain_type.h>

void loadTerrainTextures(MAP_TILESET mapTileset);

bool initTerrain();
void shutdownTerrain();

void drawTerrain(const glm::mat4 &mvp, const Vector3f &cameraPos, const Vector3f &sunPos);
void drawWater(const glm::mat4 &viewMatrix);

PIELIGHT getTileColour(int x, int y);
void setTileColour(int x, int y, PIELIGHT colour);

void markTileDirty(int i, int j);

enum TerrainShaderQuality
{
	CLASSIC = 0, // the mode used by at least WZ 3.2.x - 4.3.x
	NORMAL_MAPPING = 1	// the highest-quality mode, which adds normal / specular / height maps and advanced lighting
};
constexpr TerrainShaderQuality TerrainShaderQuality_MAX = TerrainShaderQuality::NORMAL_MAPPING;

enum TerrainShaderType
{
	FALLBACK, // old multi-pass method, which only supports "classic" rendering
	SINGLE_PASS // new terrain rendering method, supports all TerrainShaderQuality modes
};

extern TerrainShaderQuality terrainShaderQuality;

TerrainShaderQuality getTerrainShaderQuality();
TerrainShaderType getTerrainShaderType();
bool setTerrainShaderQuality(TerrainShaderQuality newValue);
std::vector<TerrainShaderQuality> getAllTerrainShaderQualityOptions();
bool isSupportedTerrainShaderQualityOption(TerrainShaderQuality value);
std::string to_display_string(TerrainShaderQuality value);

void initTerrainShaderType(); // must be called after the graphics context is initialized

#endif
