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

#include "pielighting.h"
#include "gfx_api.h"
#include <array>
#include <glm/glm.hpp>
#include <algorithm>
#include <functional>
#include "culling.h"
#include "src/profiling.h"


PIELIGHT& LightMap::operator()(int32_t x, int32_t y)
{
	// Clamp x and y values to actual ones
	// Give one tile worth of leeway before asserting, for units/transporters coming in from off-map.
	ASSERT(x >= -1, "mapTile: x value is too small (%d,%d) in %dx%d", x, y, mapWidth, mapHeight);
	ASSERT(y >= -1, "mapTile: y value is too small (%d,%d) in %dx%d", x, y, mapWidth, mapHeight);
	x = std::max(x, 0);
	y = std::max(y, 0);
	ASSERT(x < mapWidth + 1, "mapTile: x value is too big (%d,%d) in %dx%d", x, y, mapWidth, mapHeight);
	ASSERT(y < mapHeight + 1, "mapTile: y value is too big (%d,%d) in %dx%d", x, y, mapWidth, mapHeight);
	x = std::min(x, mapWidth - 1);
	y = std::min(y, mapHeight - 1);

	return data[x + (y * mapWidth)];
}

const PIELIGHT& LightMap::operator()(int32_t x, int32_t y) const
{
	// Clamp x and y values to actual ones
	// Give one tile worth of leeway before asserting, for units/transporters coming in from off-map.
	ASSERT(x >= -1, "mapTile: x value is too small (%d,%d) in %dx%d", x, y, mapWidth, mapHeight);
	ASSERT(y >= -1, "mapTile: y value is too small (%d,%d) in %dx%d", x, y, mapWidth, mapHeight);
	x = std::max(x, 0);
	y = std::max(y, 0);
	ASSERT(x < mapWidth + 1, "mapTile: x value is too big (%d,%d) in %dx%d", x, y, mapWidth, mapHeight);
	ASSERT(y < mapHeight + 1, "mapTile: y value is too big (%d,%d) in %dx%d", x, y, mapWidth, mapHeight);
	x = std::min(x, mapWidth - 1);
	y = std::min(y, mapHeight - 1);

	return data[x + (y * mapWidth)];
}

void LightMap::reset(size_t width, size_t height)
{
	mapWidth = static_cast<int32_t>(width);
	mapHeight = static_cast<int32_t>(height);
	data = std::make_unique<PIELIGHT[]>(width * height);
}


LightingData& getCurrentLightingData()
{
	static LightingData scene;
	return scene;
}

LightMap& getCurrentLightmapData()
{
	static LightMap lightmap;
	return lightmap;
}

namespace {
	BoundingBox getLightBoundingBox(const LIGHT& light)
	{
		glm::vec3 center = light.position;
		center.z *= -1.f;
		float range = light.range;
		glm::vec3 horizontal(1.0f, 0.f, 0.f);
		glm::vec3 vertical(0.f, 1.0f, 0.f);
		glm::vec3 forward(0.f, 0.f, 1.0f);

		return BoundingBox{
			center - horizontal * range - vertical * range - forward * range,
			center - horizontal * range - vertical * range + forward * range,
			center - horizontal * range + vertical * range - forward * range,
			center - horizontal * range + vertical * range + forward * range,
			center + horizontal * range - vertical * range - forward * range,
			center + horizontal * range - vertical * range + forward * range,
			center + horizontal * range + vertical * range - forward * range,
			center + horizontal * range + vertical * range + forward * range
		};
	}

}

void renderingNew::LightingManager::ComputeFrameData(const LightingData& data, LightMap&, const glm::mat4& worldViewProjectionMatrix)
{
	PointLightBuckets result;

	// Pick the first lights inside the view frustum
	auto viewFrustum = IntersectionOfHalfSpace{
		[](const glm::vec3& in) { return in.x >= -1.f; },
		[](const glm::vec3& in) { return in.x <= 1.f; },
		[](const glm::vec3& in) {
			if (gfx_api::context::get().isYAxisInverted())
			{
				return -in.y >= -1.f;
			}
			return in.y >= -1.f;
		},
		[](const glm::vec3& in) {
			if (gfx_api::context::get().isYAxisInverted())
			{
				return -in.y <= 1.f;
			}
			return in.y <= 1.f;
		},
		[](const glm::vec3& in) { return in.z >= 0; },
		[](const glm::vec3& in) { return in.z <= 1; }
	};

	culledLights.clear();
	for (const auto& light : data.lights)
	{
		if (culledLights.size() >= gfx_api::max_lights)
		{
			break;
		}
		auto clipSpaceBoundingBox = transformBoundingBox(worldViewProjectionMatrix, getLightBoundingBox(light));
		if (!isBBoxInClipSpace(viewFrustum, clipSpaceBoundingBox))
		{
			continue;
		}
		culledLights.push_back(light);
	}


	for (size_t lightIndex = 0, end = culledLights.size(); lightIndex < end; lightIndex++)
	{
		const auto& light = culledLights[lightIndex];
		result.positions[lightIndex].x = light.position.x;
		result.positions[lightIndex].y = light.position.y;
		result.positions[lightIndex].z = light.position.z;
		result.colorAndEnergy[lightIndex].x = light.colour.byte.r / 255.f;
		result.colorAndEnergy[lightIndex].y = light.colour.byte.g / 255.f;
		result.colorAndEnergy[lightIndex].z = light.colour.byte.b / 255.f;
		result.colorAndEnergy[lightIndex].w = light.range;
	}

	// Iterate over all buckets
	size_t overallId = 0;
	size_t bucketId = 0;

	// GLSL std layout 140 force us to store array of int with the same stride as
	// an array of ivec4, wasting 3/4 of the storage.
	// To circumvent this, we pack 4 consecutives index in a ivec4 here, and unpack the value in the shader.
	std::array<size_t, gfx_api::max_indexed_lights * 4> lightList;
	for (size_t i = 0; i < gfx_api::bucket_dimension; i++)
	{
		for (size_t j = 0; j < gfx_api::bucket_dimension; j++)
		{
			auto frustum = IntersectionOfHalfSpace{
				[i](const glm::vec3& in) { return in.x >= -1.f + 2 * static_cast<float>(i) / gfx_api::bucket_dimension; },
				[i](const glm::vec3& in) { return in.x <= -1.f + 2 * static_cast<float>(i + 1) / gfx_api::bucket_dimension; },
				[j](const glm::vec3& in) {
					if (gfx_api::context::get().isYAxisInverted())
						return -in.y >= -1.f + 2 * static_cast<float>(j) / gfx_api::bucket_dimension;
					return in.y >= -1.f + 2 * static_cast<float>(j) / gfx_api::bucket_dimension;
				},
				[j](const glm::vec3& in) {
					if (gfx_api::context::get().isYAxisInverted())
						return -in.y <= -1.f + 2 * static_cast<float>(j + 1) / gfx_api::bucket_dimension;
					return in.y <= -1.f + 2 * static_cast<float>(j + 1) / gfx_api::bucket_dimension;
				},
				[](const glm::vec3& in) { return in.z >= 0; },
				[](const glm::vec3& in) { return in.z <= 1; }
			};

			size_t bucketSize = 0;
			for (size_t lightIndex = 0; lightIndex < culledLights.size(); lightIndex++)
			{
				if (overallId + bucketSize >= lightList.size())
				{
					continue;
				}
				const LIGHT& light = culledLights[lightIndex];
				BoundingBox clipSpaceBoundingBox = transformBoundingBox(worldViewProjectionMatrix, getLightBoundingBox(light));

				if (isBBoxInClipSpace(frustum, clipSpaceBoundingBox))
				{
					lightList[overallId + bucketSize] = lightIndex;

					bucketSize++;
				}
			}

			result.bucketOffsetAndSize[bucketId] = glm::ivec4(overallId, bucketSize, 0, 0);
			overallId += bucketSize;
			bucketId++;
		}
	}

	// pack the index
	for (size_t i = 0; i < lightList.size(); i++)
	{
		result.light_index[i / 4][i % 4] = static_cast<int>(lightList[i]);
	}

	currentPointLightBuckets = std::move(result);
}

static std::unique_ptr<ILightingManager> lightingManager;

void setLightingManager(std::unique_ptr<ILightingManager> manager)
{
	lightingManager = std::move(manager);
}

ILightingManager& getCurrentLightingManager()
{
	return *lightingManager;
}

