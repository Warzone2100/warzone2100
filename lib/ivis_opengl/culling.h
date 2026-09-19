/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2024  Warzone 2100 Project

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

#include <array>
#include <glm/glm.hpp>
#include <algorithm>

using BoundingBox = std::array<glm::vec3, 8>;

/// Axis aligned extent of a clip space bounding box
struct ClipSpaceBounds
{
	glm::vec3 minimum = glm::vec3(0.f);
	glm::vec3 maximum = glm::vec3(0.f);
};

/// Project a world space bounding box and return its axis aligned clip space extent.
/// A corner at or behind the eye plane has no meaningful projection, so such a box is
/// reported as covering everything, which is conservative and true of a box around the eye.
ClipSpaceBounds clipSpaceBoundsOfBoundingBox(const glm::mat4& worldViewProjectionMatrix,
	const BoundingBox& worldSpaceBoundingBox);

/// True unless the bounds lie wholly outside the clip space region
/// [x0, x1] x [y0, y1] x [0, 1]. Culling regions are axis aligned, so comparing
/// extents answers this exactly.
bool boundsOverlapClipRegion(const ClipSpaceBounds& bounds, float x0, float x1, float y0, float y1);
