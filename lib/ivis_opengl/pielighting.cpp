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
#include <unordered_map>
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

		auto horizRange = horizontal * range;
		auto verticalRange = vertical * range;
		auto forwardRange = forward * range;

		auto centerMinusHorizRange = center - horizRange;
		auto centerPlusHorizRange = center + horizRange;
		auto verticalRangeMinusForwardRange = verticalRange - forwardRange;
		auto verticalRangePlusForwardRange = verticalRange + forwardRange;

		return BoundingBox{
			centerMinusHorizRange - verticalRangeMinusForwardRange,
			centerMinusHorizRange - verticalRangePlusForwardRange,
			centerMinusHorizRange + verticalRangeMinusForwardRange,
			centerMinusHorizRange + verticalRangePlusForwardRange,
			centerPlusHorizRange - verticalRangeMinusForwardRange,
			centerPlusHorizRange - verticalRangePlusForwardRange,
			centerPlusHorizRange + verticalRangeMinusForwardRange,
			centerPlusHorizRange + verticalRangePlusForwardRange
		};
	}

}

/* The shift on a world coordinate to get the tile coordinate */
#define TILE_SHIFT 7

static inline int32_t pielight_maptile_coord(int32_t worldCoord)
{
	return worldCoord >> TILE_SHIFT;
}

struct TileCoordsHasher
{
	std::size_t operator()(const std::pair<int32_t, int32_t>& p) const
	{
		return std::hash<long long>()(static_cast<long long>(p.first) * (static_cast<long long>(INT_MAX) + 1) + p.second);
	}
};

static float pointLightDistanceCalc(const renderingNew::LightingManager::CalculatedPointLight& a, const LIGHT& b)
{
	glm::vec3 pointLightVector = a.position - glm::vec3(b.position);
	auto length = glm::length(pointLightVector);
	return length;
}

void renderingNew::LightingManager::ComputeFrameData(const LightingData& data, LightMap&, const glm::mat4& worldViewProjectionMatrix)
{
	PointLightBuckets result;
	const bool yAxisInverted = gfx_api::context::get().isYAxisInverted();

	// Pick the first lights inside the view frustum
	auto viewFrustum = IntersectionOfHalfSpace{
		[](const glm::vec3& in) { return in.x >= -1.f; },
		[](const glm::vec3& in) { return in.x <= 1.f; },
		[yAxisInverted](const glm::vec3& in) {
			if (yAxisInverted)
			{
				return -in.y >= -1.f;
			}
			return in.y >= -1.f;
		},
		[yAxisInverted](const glm::vec3& in) {
			if (yAxisInverted)
			{
				return -in.y <= 1.f;
			}
			return in.y <= 1.f;
		},
		[](const glm::vec3& in) { return in.z >= 0; },
		[](const glm::vec3& in) { return in.z <= 1; }
	};

	std::unordered_map<std::pair<int32_t, int32_t>, std::vector<size_t>, TileCoordsHasher> tileRangeLights; // map tile coordinates to vector of culledLight indexes
	constexpr size_t maxRangedLightsPerTile = 16;
	constexpr size_t minLightRange = 5;
	constexpr float distanceCalcCombineThreshold = 32.f;
	size_t lightsCombined = 0;
	size_t lightsSkipped = 0;
	size_t tinyLightsSkipped = 0;

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

		if (light.range >= minLightRange)
		{
			std::pair<int32_t, int32_t> lightTileCoords(pielight_maptile_coord(light.position.x), pielight_maptile_coord(light.position.y));
			auto it = tileRangeLights.find(lightTileCoords);
			if (it != tileRangeLights.end())
			{
				// merge point lights (if possible)
				bool combinedLight = false;
				for (auto& o : it->second)
				{
					auto& existingLight = culledLights[o];
					auto newLightRange = static_cast<float>(light.range);
					auto distanceCalc = pointLightDistanceCalc(existingLight.light, light);
					if ((distanceCalc < distanceCalcCombineThreshold)
						&& (distanceCalc < (existingLight.light.range + newLightRange)))
					{
						// Found two lights close to each other - combine them
						if (newLightRange > existingLight.light.range)
						{
							// If the new light has a greater range, use that as the "base"
							CalculatedPointLight calcLight;
							calcLight.position = glm::vec3(light.position.x,  light.position.y, light.position.z);
							calcLight.colour = glm::vec3(light.colour.byte.r / 255.f, light.colour.byte.g / 255.f, light.colour.byte.b / 255.f);
							calcLight.range = light.range;

							float weight = existingLight.light.range / calcLight.range;
							calcLight.colour.x += (existingLight.light.colour.x) * weight;
							calcLight.colour.y += (existingLight.light.colour.y) * weight;
							calcLight.colour.z += (existingLight.light.colour.z) * weight;

							existingLight.light = calcLight;
							existingLight.clipSpaceBoundingBox = clipSpaceBoundingBox;
						}
						else
						{
							float weight = light.range / existingLight.light.range;
							existingLight.light.colour.x += (light.colour.byte.r / 255.f) * weight;
							existingLight.light.colour.y += (light.colour.byte.g / 255.f) * weight;
							existingLight.light.colour.z += (light.colour.byte.b / 255.f) * weight;
						}
						combinedLight = true;
						break;
					}
				}
				if (combinedLight)
				{
					++lightsCombined;
					continue;
				}
				if (it->second.size() >= maxRangedLightsPerTile)
				{
					++lightsSkipped;
					continue;
				}

				it->second.push_back(culledLights.size());
			}
			else
			{
				tileRangeLights[lightTileCoords].push_back(culledLights.size());
			}
		}
		else
		{
			++tinyLightsSkipped;
			continue;
		}

		CalculatedPointLight calcLight;
		calcLight.position = glm::vec3(light.position.x,  light.position.y, light.position.z);
		calcLight.colour = glm::vec3(light.colour.byte.r / 255.f, light.colour.byte.g / 255.f, light.colour.byte.b / 255.f);
		calcLight.range = light.range;

		culledLights.push_back({std::move(calcLight), std::move(clipSpaceBoundingBox)});
	}

	if (lightsSkipped > 0 || lightsCombined > 0 || tinyLightsSkipped > 0)
	{
		// debug(LOG_INFO, "Point lights - merged: %zu, skipped (tile limit): %zu, skipped (tiny): %zu", lightsCombined, lightsSkipped, tinyLightsSkipped);
	}

	for (size_t lightIndex = 0, end = culledLights.size(); lightIndex < end; lightIndex++)
	{
		const auto& light = culledLights[lightIndex].light;
		result.positions[lightIndex].x = light.position.x;
		result.positions[lightIndex].y = light.position.y;
		result.positions[lightIndex].z = light.position.z;
		result.colorAndEnergy[lightIndex].x = light.colour.x;
		result.colorAndEnergy[lightIndex].y = light.colour.y;
		result.colorAndEnergy[lightIndex].z = light.colour.z;
		result.colorAndEnergy[lightIndex].w = light.range;
	}

	// Iterate over all buckets
	size_t overallId = 0;
	size_t bucketId = 0;

	size_t bucketDimension = gfx_api::bucket_dimension; // start off at the maximum number of buckets
	constexpr size_t minBucketDimension = 4;

	// GLSL std layout 140 force us to store array of int with the same stride as
	// an array of ivec4, wasting 3/4 of the storage.
	// To circumvent this, we pack 4 consecutives index in a ivec4 here, and unpack the value in the shader.
	std::array<size_t, gfx_api::max_indexed_lights * 4> lightList;
	bool reduceNumberOfBucketsNeeded = false;
	do {
		overallId = 0;
		bucketId = 0;
		reduceNumberOfBucketsNeeded = false;
		for (size_t i = 0; i < bucketDimension; i++)
		{
			auto bucketFrustumX0 = -1.f + 2 * static_cast<float>(i) / bucketDimension;
			auto bucketFrustumX1 = -1.f + 2 * static_cast<float>(i + 1) / bucketDimension;

			for (size_t j = 0; j < bucketDimension; j++)
			{
				auto bucketFrustumY0 = -1.f + 2 * static_cast<float>(j) / bucketDimension;
				auto bucketFrustumY1 = -1.f + 2 * static_cast<float>(j + 1) / bucketDimension;

				auto frustum = IntersectionOfHalfSpace{
					[bucketFrustumX0](const glm::vec3& in) { return in.x >= bucketFrustumX0; },
					[bucketFrustumX1](const glm::vec3& in) { return in.x <= bucketFrustumX1; },
					[bucketFrustumY0, yAxisInverted](const glm::vec3& in) {
						if (yAxisInverted)
							return -in.y >= bucketFrustumY0;
						return in.y >= bucketFrustumY0;
					},
					[bucketFrustumY1, yAxisInverted](const glm::vec3& in) {
						if (yAxisInverted)
							return -in.y <= bucketFrustumY1;
						return in.y <= bucketFrustumY1;
					},
					[](const glm::vec3& in) { return in.z >= 0; },
					[](const glm::vec3& in) { return in.z <= 1; }
				};

				size_t bucketSize = 0;
				for (size_t lightIndex = 0; lightIndex < culledLights.size(); lightIndex++)
				{
					if (overallId + bucketSize >= lightList.size())
					{
						// number of indexed lights will exceed max permitted - too many buckets
						reduceNumberOfBucketsNeeded = true;
						break;
					}
					const BoundingBox& clipSpaceBoundingBox = culledLights[lightIndex].clipSpaceBoundingBox;

					if (isBBoxInClipSpace(frustum, clipSpaceBoundingBox))
					{
						lightList[overallId + bucketSize] = lightIndex;

						bucketSize++;
					}
				}

				result.bucketOffsetAndSize[bucketId] = glm::ivec4(overallId, bucketSize, 0, 0);
				overallId += bucketSize;
				bucketId++;

				if (reduceNumberOfBucketsNeeded)
				{
					break;
				}
			}

			if (reduceNumberOfBucketsNeeded)
			{
				if (bucketDimension > minBucketDimension)
				{
					--bucketDimension;
				}
				else
				{
					// reached minimum number of buckets, but still hit max indexed point lights
					reduceNumberOfBucketsNeeded = false;

					// zero out remaining buckets
					// (note: will mean no point lights for those parts of the screen, but at least we won't have garbage data used)
					for (size_t z = bucketId; z < result.bucketOffsetAndSize.size(); z++)
					{
						result.bucketOffsetAndSize[z] = glm::ivec4(overallId, 0, 0, 0);
					}
				}
				break;
			}
		}
	} while (reduceNumberOfBucketsNeeded);

	// pack the index
	for (size_t i = 0; i < lightList.size(); i++)
	{
		result.light_index[i / 4][i % 4] = static_cast<int>(lightList[i]);
	}

	result.bucketDimensionUsed = bucketDimension;

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

