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


ClipSpaceBounds boundsOfBoundingBox(const BoundingBox& points)
{
	ClipSpaceBounds bounds;
	bounds.minimum = points[0];
	bounds.maximum = points[0];
	for (size_t i = 1, end = points.size(); i < end; i++)
	{
		bounds.minimum = glm::min(bounds.minimum, points[i]);
		bounds.maximum = glm::max(bounds.maximum, points[i]);
	}
	return bounds;
}

bool boundsOverlapClipRegion(const ClipSpaceBounds& bounds, float x0, float x1, float y0, float y1)
{
	if (bounds.maximum.x < x0 || bounds.minimum.x > x1)
	{
		return false;
	}
	if (bounds.maximum.y < y0 || bounds.minimum.y > y1)
	{
		return false;
	}
	if (bounds.maximum.z < 0.f || bounds.minimum.z > 1.f)
	{
		return false;
	}
	return true;
}
