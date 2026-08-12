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
#include <limits>

ClipSpaceBounds clipSpaceBoundsOfBoundingBox(const glm::mat4& worldViewProjectionMatrix,
	const BoundingBox& worldSpaceBoundingBox)
{
	constexpr float inf = std::numeric_limits<float>::infinity();
	ClipSpaceBounds bounds;
	bounds.minimum = glm::vec3(inf);
	bounds.maximum = glm::vec3(-inf);

	for (size_t i = 0, end = worldSpaceBoundingBox.size(); i < end; i++)
	{
		const glm::vec4 clipPos = worldViewProjectionMatrix * glm::vec4(worldSpaceBoundingBox[i], 1.f);
		if (!(clipPos.w > 0.f))
		{
			// the box reaches the eye plane, so treat it as covering the whole region
			bounds.minimum = glm::vec3(-inf);
			bounds.maximum = glm::vec3(inf);
			return bounds;
		}
		const glm::vec3 ndcPos = glm::vec3(clipPos) / clipPos.w;
		bounds.minimum = glm::min(bounds.minimum, ndcPos);
		bounds.maximum = glm::max(bounds.maximum, ndcPos);
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
