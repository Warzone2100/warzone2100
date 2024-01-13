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
#include <functional>

using BoundingBox = std::array<glm::vec3, 8>;

/// Project a bounding box in clip space
BoundingBox transformBoundingBox(const glm::mat4& worldViewProjectionMatrix, const BoundingBox& worldSpaceBoundingBox);

/// Define a half space
using HalfSpaceCheck = std::function<bool(const glm::vec3&)>;
/// Define a view frustum (as an intersection of half space
using IntersectionOfHalfSpace = std::array< HalfSpaceCheck, 6>;

bool isBBoxInClipSpace(const IntersectionOfHalfSpace& intersectionOfHalfSpace, const BoundingBox& points);
