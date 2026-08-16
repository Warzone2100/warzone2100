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
#include <cmath>
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

// Rough measure of how much a light matters this frame: reach against distance, then
// discounted for lights hanging further above the terrain than their own range.
// Screen coverage is deliberately not used, as it just rewards proximity to the camera.
static float pointLightImportance(const renderingNew::LightingManager::CalculatedPointLight& light,
	const LightingSceneInfo& scene)
{
	// strongest channel rather than luminance, which would score red lights far too low
	const float intensity = std::max({light.colour.x, light.colour.y, light.colour.z});
	const float range = std::max(light.range, 1.f);

	// light positions keep the raw coordinates, world space negates z
	const glm::vec3 lightWorldPos(light.position.x, light.position.y, -light.position.z);
	const glm::vec3 toCamera = lightWorldPos - scene.cameraPosition;
	const float distanceSq = std::max(glm::dot(toCamera, toCamera), 1.f);

	float importance = (range * range) * intensity / distanceSq;

	if (scene.groundHeightAt)
	{
		const float groundHeight = static_cast<float>(
			scene.groundHeightAt(static_cast<int32_t>(light.position.x), static_cast<int32_t>(light.position.z)));
		const float heightAboveGround = std::max(0.f, light.position.y - groundHeight);
		// fades out once the light is its own range above the terrain
		importance *= std::max(0.f, 1.f - heightAboveGround / range);
	}
	return importance;
}

// How many buckets of a dim x dim grid a light's bounds touch. Bounds reaching the eye
// plane clamp to the whole grid, which is what they do in fact cover.
static size_t pointLightBucketFootprint(const ClipSpaceBounds& bounds, size_t bucketDimension)
{
	if (bounds.maximum.z < 0.f || bounds.minimum.z > 1.f)
	{
		return 0;
	}
	const auto cellIndex = [bucketDimension](float coord) -> size_t {
		const float clamped = std::min(std::max(coord, -1.f), 1.f);
		const long index = static_cast<long>(std::floor((clamped + 1.f) * 0.5f * static_cast<float>(bucketDimension)));
		return static_cast<size_t>(std::min<long>(std::max<long>(index, 0), static_cast<long>(bucketDimension) - 1));
	};
	const size_t spanX = cellIndex(bounds.maximum.x) - cellIndex(bounds.minimum.x) + 1;
	const size_t spanY = cellIndex(bounds.maximum.y) - cellIndex(bounds.minimum.y) + 1;
	return spanX * spanY;
}

// How much a light matters at one part of the screen rather than overall, weighting its
// global score by how central the region is within the light's extent. A wide light
// crossing a region delivers little there, so it should not outrank one sitting on it.
static float pointLightLocalImportance(const renderingNew::LightingManager::CulledLightInfo& light,
	float regionCentreX, float regionCentreY)
{
	const glm::vec3& lo = light.clipSpaceBounds.minimum;
	const glm::vec3& hi = light.clipSpaceBounds.maximum;
	const float radiusX = (hi.x - lo.x) * 0.5f;
	const float radiusY = (hi.y - lo.y) * 0.5f;
	if (!(radiusX > 0.f) || !(radiusY > 0.f))
	{
		return 0.f;
	}
	const float offsetX = (regionCentreX - (lo.x + hi.x) * 0.5f) / radiusX;
	const float offsetY = (regionCentreY - (lo.y + hi.y) * 0.5f) / radiusY;
	const float normalisedDistance = std::sqrt(offsetX * offsetX + offsetY * offsetY);
	// infinite radius lands at 1, so such a light keeps its full score everywhere
	return light.importance * std::max(0.f, 1.f - normalisedDistance);
}

static float pointLightDistanceCalc(const renderingNew::LightingManager::CalculatedPointLight& a, const LIGHT& b)
{
	glm::vec3 pointLightVector = a.position - glm::vec3(b.position);
	auto length = glm::length(pointLightVector);
	return length;
}

void renderingNew::LightingManager::ComputeFrameData(const LightingData& data, LightMap&, const glm::mat4& worldViewProjectionMatrix, const LightingSceneInfo& scene)
{
	PointLightBuckets result;
	result.reset();
	const auto& lightCapacity = gfx_api::activeLightCapacity();
	const bool yAxisInverted = gfx_api::context::get().isYAxisInverted();

	// Pick the first lights inside the view frustum
	// the frustum is [-1, 1] on x and y, which mirrors onto itself when y is inverted

	constexpr size_t maxRangedLightsPerTile = 16;
	constexpr size_t minLightRange = 5;
	constexpr float distanceCalcCombineThreshold = 32.f;
	size_t lightsCombined = 0;
	size_t lightsSkipped = 0;
	size_t tinyLightsSkipped = 0;

	culledLights.clear();
	tileRangeLights.clear();
	for (const auto& light : data.lights)
	{
		const ClipSpaceBounds clipSpaceBounds =
			clipSpaceBoundsOfBoundingBox(worldViewProjectionMatrix, getLightBoundingBox(light));
		if (!boundsOverlapClipRegion(clipSpaceBounds, -1.f, 1.f, -1.f, 1.f))
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
							existingLight.clipSpaceBounds = clipSpaceBounds;
							existingLight.importance = pointLightImportance(existingLight.light, scene);
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

		const float importance = pointLightImportance(calcLight, scene);
		culledLights.push_back({std::move(calcLight), clipSpaceBounds, importance});
	}

	if (lightsSkipped > 0 || lightsCombined > 0 || tinyLightsSkipped > 0)
	{
		// debug(LOG_INFO, "Point lights - merged: %zu, skipped (tile limit): %zu, skipped (tiny): %zu", lightsCombined, lightsSkipped, tinyLightsSkipped);
	}

	// order by significance so any later truncation drops the least noticeable lights
	std::sort(culledLights.begin(), culledLights.end(),
		[](const CulledLightInfo& a, const CulledLightInfo& b) { return a.importance > b.importance; });
	if (culledLights.size() > lightCapacity.maxLights)
	{
		// More lights survived culling than can be uploaded. Keeping the globally highest
		// scoring bunches the winners near the camera and darkens the rest of the screen,
		// so instead deal each region its best remaining light in turn, ranked by what
		// each contributes there. Lights stay whole, so no edge appears at a bucket
		// boundary.
		const size_t selectionDimension = gfx_api::bucket_dimension;
		constexpr size_t maxDealtPerBucket = 16;
		// clear rather than reassign, to keep the per bucket capacity
		bucketCandidates.resize(selectionDimension * selectionDimension);
		for (auto& candidates : bucketCandidates)
		{
			candidates.clear();
		}
		for (size_t i = 0; i < selectionDimension; i++)
		{
			const float x0 = -1.f + 2 * static_cast<float>(i) / selectionDimension;
			const float x1 = -1.f + 2 * static_cast<float>(i + 1) / selectionDimension;
			for (size_t j = 0; j < selectionDimension; j++)
			{
				const float rawY0 = -1.f + 2 * static_cast<float>(j) / selectionDimension;
				const float rawY1 = -1.f + 2 * static_cast<float>(j + 1) / selectionDimension;
				const float y0 = yAxisInverted ? -rawY1 : rawY0;
				const float y1 = yAxisInverted ? -rawY0 : rawY1;
				const size_t bucket = i * selectionDimension + j;
				// score once and order by it, so dealing is a cursor walk
				const float centreX = (x0 + x1) * 0.5f;
				const float centreY = (y0 + y1) * 0.5f;
				scoredCandidates.clear();
				for (size_t lightIndex = 0; lightIndex < culledLights.size(); lightIndex++)
				{
					if (boundsOverlapClipRegion(culledLights[lightIndex].clipSpaceBounds, x0, x1, y0, y1))
					{
						scoredCandidates.emplace_back(
							pointLightLocalImportance(culledLights[lightIndex], centreX, centreY), lightIndex);
					}
				}
				// dealing takes only the leading few, so a full sort would be wasted
				const size_t keepPerBucket = std::min<size_t>(scoredCandidates.size(), maxDealtPerBucket);
				std::partial_sort(scoredCandidates.begin(), scoredCandidates.begin() + keepPerBucket,
					scoredCandidates.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
				auto& candidates = bucketCandidates[bucket];
				for (size_t k = 0; k < keepPerBucket; k++)
				{
					candidates.push_back(scoredCandidates[k].second);
				}
			}
		}

		selectedLights.assign(culledLights.size(), false);
		nextCandidate.assign(bucketCandidates.size(), 0);
		auto& selected = selectedLights;
		size_t selectedCount = 0;
		bool dealtThisRound = true;
		while (selectedCount < lightCapacity.maxLights && dealtThisRound)
		{
			dealtThisRound = false;
			for (size_t bucket = 0; bucket < bucketCandidates.size(); bucket++)
			{
				if (selectedCount >= lightCapacity.maxLights)
				{
					break;
				}
				const auto& candidates = bucketCandidates[bucket];
				size_t& cursor = nextCandidate[bucket];
				while (cursor < candidates.size() && selected[candidates[cursor]])
				{
					++cursor;
				}
				if (cursor < candidates.size())
				{
					selected[candidates[cursor]] = true;
					++cursor;
					++selectedCount;
					dealtThisRound = true;
				}
			}
		}

		// dealing can run out early if the lights crowd into few buckets, so top up
		for (size_t i = 0; i < culledLights.size() && selectedCount < lightCapacity.maxLights; i++)
		{
			if (!selected[i])
			{
				selected[i] = true;
				++selectedCount;
			}
		}

		size_t writeIndex = 0;
		for (size_t readIndex = 0; readIndex < culledLights.size(); readIndex++)
		{
			if (selected[readIndex])
			{
				culledLights[writeIndex++] = culledLights[readIndex];
			}
		}
		culledLights.resize(writeIndex);
	}

	// Pick the finest grid whose index demand fits, then admit whole lights. A light's
	// index count follows from its bounds, so the grid can be chosen up front rather than
	// by binning and starting over. Coarsening drops no light, it just spends fewer
	// entries on each. Lights are admitted whole, since dropping one from a single bucket
	// would leave an edge along that boundary.
	size_t bucketDimension = gfx_api::bucket_dimension;
	{
		constexpr size_t minBucketDimension = 4;
		const size_t indexBudget = lightCapacity.maxIndexedLights * 4;
		const auto totalFootprint = [this](size_t dimension) {
			size_t total = 0;
			for (const auto& culled : culledLights)
			{
				total += pointLightBucketFootprint(culled.clipSpaceBounds, dimension);
			}
			return total;
		};
		while (bucketDimension > minBucketDimension && totalFootprint(bucketDimension) > indexBudget)
		{
			--bucketDimension;
		}

		size_t indicesUsed = 0;
		size_t admittedLights = 0;
		for (const auto& culled : culledLights)
		{
			const size_t footprint = pointLightBucketFootprint(culled.clipSpaceBounds, bucketDimension);
			if (indicesUsed + footprint > indexBudget)
			{
				break;
			}
			indicesUsed += footprint;
			++admittedLights;
		}
		culledLights.resize(admittedLights);
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



	// GLSL std layout 140 force us to store array of int with the same stride as
	// an array of ivec4, wasting 3/4 of the storage.
	// To circumvent this, we pack 4 consecutives index in a ivec4 here, and unpack the value in the shader.
	std::vector<size_t> lightList(lightCapacity.maxIndexedLights * 4);
	// Give every bucket an equal share of the index list, which cannot overflow. Filling
	// greedily and halving the grid on overflow cost repeated binning passes and zeroed
	// the remaining buckets once the grid hit its floor, leaving parts of the screen
	// unlit. Lights are in significance order, so an overflowing bucket keeps the best.
	{
		overallId = 0;
		bucketId = 0;
		for (size_t i = 0; i < bucketDimension; i++)
		{
			auto bucketFrustumX0 = -1.f + 2 * static_cast<float>(i) / bucketDimension;
			auto bucketFrustumX1 = -1.f + 2 * static_cast<float>(i + 1) / bucketDimension;

			for (size_t j = 0; j < bucketDimension; j++)
			{
				auto bucketFrustumY0 = -1.f + 2 * static_cast<float>(j) / bucketDimension;
				auto bucketFrustumY1 = -1.f + 2 * static_cast<float>(j + 1) / bucketDimension;

				// mirror the bucket's y span so it matches the stored bounds
				const float regionY0 = yAxisInverted ? -bucketFrustumY1 : bucketFrustumY0;
				const float regionY1 = yAxisInverted ? -bucketFrustumY0 : bucketFrustumY1;

				size_t bucketSize = 0;
				for (size_t lightIndex = 0; lightIndex < culledLights.size(); lightIndex++)
				{
					if (overallId + bucketSize >= lightList.size())
					{
						break; // guard only, the admission above already guarantees the fit
					}
					if (boundsOverlapClipRegion(culledLights[lightIndex].clipSpaceBounds,
							bucketFrustumX0, bucketFrustumX1, regionY0, regionY1))
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
	}

	// pack the index
	for (size_t i = 0; i < lightList.size(); i++)
	{
		result.light_index[i / 4][i % 4] = static_cast<int>(lightList[i]);
	}

	result.bucketDimensionUsed = bucketDimension;

	currentPointLightBuckets = std::move(result);
}

const ILightingManager::FlatPointLightData& ILightingManager::getFlatPointLightData()
{
	const auto& buckets = currentPointLightBuckets;
	const auto& capacity = gfx_api::activeLightCapacity();
	currentFlatPointLightData.lights.resize(capacity.maxLights * 2);
	currentFlatPointLightData.indices.resize(capacity.maxIndexedLights * 4);
	for (size_t i = 0; i < capacity.maxLights; ++i)
	{
		currentFlatPointLightData.lights[i * 2] = buckets.positions[i];
		currentFlatPointLightData.lights[i * 2 + 1] = buckets.colorAndEnergy[i];
	}
	for (size_t i = 0; i < capacity.maxIndexedLights; ++i)
	{
		const glm::ivec4& packed = buckets.light_index[i];
		currentFlatPointLightData.indices[i * 4] = packed.x;
		currentFlatPointLightData.indices[i * 4 + 1] = packed.y;
		currentFlatPointLightData.indices[i * 4 + 2] = packed.z;
		currentFlatPointLightData.indices[i * 4 + 3] = packed.w;
	}
	return currentFlatPointLightData;
}


static ILightingManager* lightingManager = nullptr;

void setLightingManager(ILightingManager* manager)
{
	lightingManager = manager;
}

ILightingManager& getCurrentLightingManager()
{
	return *lightingManager;
}

