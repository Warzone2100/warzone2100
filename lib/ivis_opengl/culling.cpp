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
#include "culling.h"
#include <array>
#include <glm/glm.hpp>
#include <algorithm>
#include <functional>

BoundingBox transformBoundingBox(const glm::mat4& worldViewProjectionMatrix, const BoundingBox& worldSpaceBoundingBox)
{
	BoundingBox bboxInClipSpace;
	for (size_t i = 0, end = bboxInClipSpace.size(); i < end; i++)
	{
		glm::vec4 tmp = worldViewProjectionMatrix * glm::vec4(worldSpaceBoundingBox[i], 1.0);
		tmp = (tmp / tmp.w);
		bboxInClipSpace[i] = glm::vec3(tmp.x, tmp.y, tmp.z);
	}
	return bboxInClipSpace;
}


bool isBBoxInClipSpace(const IntersectionOfHalfSpace& intersectionOfHalfSpace, const BoundingBox& points)
{
	// We test against the complement of the half space
	// If all points lies in the complement a half space, it can't be part of the intersection
	auto CheckAllPointsInSpace = [&points](const HalfSpaceCheck& predicate)
		{
			return std::all_of(points.begin(), points.end(), [&predicate](const auto& v) { return !predicate(v); });
		};

	for (const auto& predicate : intersectionOfHalfSpace)
	{
		if (CheckAllPointsInSpace(predicate))
			return false;
	}
	return true;
}
