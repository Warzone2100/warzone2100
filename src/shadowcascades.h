/*
	This file is part of Warzone 2100.
	Copyright (C) 2023  Warzone 2100 Project

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

#include <vector>
#include "lib/framework/vector.h"
#include <glm/mat4x4.hpp>

struct iView;

// Contains all resources required for a single shadow map cascade
struct Cascade
{
	float splitDepth = 0.f;
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
};

void calculateShadowCascades(const iView *player, float terrainDistance, const glm::mat4& baseViewMatrix, const glm::vec3& lightInvDir, size_t SHADOW_MAP_CASCADE_COUNT, std::vector<Cascade>& output);

