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

#include "lib/ivis_opengl/pietypes.h"
#include "gfx_api.h"
#include "culling.h"
#include <glm/glm.hpp>
#include <memory>
#include <array>
#include <vector>
#include <functional>
#include <utility>
#include <climits>
#include <unordered_map>

struct LIGHT
{
	Vector3i position = Vector3i(0, 0, 0);
	UDWORD range;
	PIELIGHT colour;
};


struct LightMap
{
	PIELIGHT& operator()(int32_t x, int32_t y);
	const PIELIGHT& operator()(int32_t x, int32_t y) const;

	void reset(size_t width, size_t height);
private:
	std::unique_ptr<PIELIGHT[]> data = nullptr;
	int32_t mapWidth;
	int32_t mapHeight;
};

struct LightingData
{
	std::vector<LIGHT> lights;
};

LightingData& getCurrentLightingData();
LightMap& getCurrentLightmapData();




/// Scene facts the lighting managers need beyond the lights themselves.
struct LightingSceneInfo
{
	//! camera position in world space (z negated, matching getLightBoundingBox)
	glm::vec3 cameraPosition = glm::vec3(0.f);
	//! terrain height under a world XY, in raw light coordinates. Consulted once per
	//! candidate light, so the indirection here costs nothing measurable.
	std::function<int32_t(int32_t, int32_t)> groundHeightAt;
};

struct ILightingManager
{
	struct PointLightBuckets
	{
		std::vector<glm::vec4> positions;
		std::vector<glm::vec4> colorAndEnergy;

		// z and y components are used for padding, keep ivec4 !
		std::array<glm::ivec4, gfx_api::bucket_dimension * gfx_api::bucket_dimension> bucketOffsetAndSize = {};
		// Unfortunately due to std140 constraint, we pack indexes in glm::ivec4 and unpack them in shader later
		std::vector<glm::ivec4> light_index;

		size_t bucketDimensionUsed = gfx_api::bucket_dimension;

		//! Size the arrays to the active capacity and clear them
		void reset()
		{
			const auto& capacity = gfx_api::activeLightCapacity();
			positions.assign(capacity.maxLights, glm::vec4(0.f));
			colorAndEnergy.assign(capacity.maxLights, glm::vec4(0.f));
			bucketOffsetAndSize = {};
			light_index.assign(capacity.maxIndexedLights, glm::ivec4(0));
			bucketDimensionUsed = gfx_api::bucket_dimension;
		}
	};

	//! The same light data laid out for a buffer transport:
	//! - two vec4 per light, position then colour and energy
	//! - the index list (without the ivec4 packing)
	struct FlatPointLightData
	{
		std::vector<glm::vec4> lights;
		std::vector<int32_t> indices;
	};

	virtual ~ILightingManager() = default;

	void SetFrameStart()
	{
		currentPointLightBuckets.reset();
	}

	virtual void ComputeFrameData(const LightingData& data, LightMap& lightmap, const glm::mat4& worldViewProjectionMatrix, const LightingSceneInfo& scene) = 0;

	const PointLightBuckets& getPointLightBuckets() const
	{
		return currentPointLightBuckets;
	}

	//! Rebuilt from the buckets on demand (so zero cost while the uniform block transport is in use).
	const FlatPointLightData& getFlatPointLightData();

	protected:
		PointLightBuckets currentPointLightBuckets;
		FlatPointLightData currentFlatPointLightData;
};


namespace renderingNew
{
	struct TileCoordsHasher
	{
		std::size_t operator()(const std::pair<int32_t, int32_t>& p) const
		{
			return std::hash<long long>()(static_cast<long long>(p.first) * (static_cast<long long>(INT_MAX) + 1) + p.second);
		}
	};

	//! This lighting manager generate a proper PointLightBuckets for per pixel point lights
	struct LightingManager final : ILightingManager
	{
		void ComputeFrameData(const LightingData& data, LightMap& lightmap, const glm::mat4& worldViewProjectionMatrix, const LightingSceneInfo& scene) override;

		struct CalculatedPointLight
		{
			glm::vec3 position = glm::vec3(0, 0, 0);
			glm::vec3 colour;
			float range;
		};

		struct CulledLightInfo
		{
			CalculatedPointLight light;
			ClipSpaceBounds clipSpaceBounds;
			float importance = 0.f;
		};
	private:
		// cached containers to avoid frequent reallocations
		std::vector<CulledLightInfo> culledLights;
		//! tile coordinates to indices into culledLights, rebuilt per frame
		std::unordered_map<std::pair<int32_t, int32_t>, std::vector<size_t>, TileCoordsHasher> tileRangeLights;
		//! per screen bucket candidate lists, reused across frames
		std::vector<std::vector<size_t>> bucketCandidates;
		//! scratch for ordering one bucket's candidates, reused across frames
		std::vector<std::pair<float, size_t>> scoredCandidates;
		//! selection bookkeeping, reused across frames
		std::vector<bool> selectedLights;
		std::vector<size_t> nextCandidate;
	};
}

/// Set the active lighting manager. Ownership stays with the caller, so the manager
/// (and its cached containers) can persist across frames.
void setLightingManager(ILightingManager* manager);

ILightingManager& getCurrentLightingManager();
