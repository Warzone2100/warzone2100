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




struct ILightingManager
{
	struct PointLightBuckets
	{
		std::array<glm::vec4, gfx_api::max_lights> positions = {};
		std::array<glm::vec4, gfx_api::max_lights> colorAndEnergy = {};

		// z and y components are used for padding, keep ivec4 !
		std::array<glm::ivec4, gfx_api::bucket_dimension * gfx_api::bucket_dimension> bucketOffsetAndSize = {};
		// Unfortunately due to std140 constraint, we pack indexes in glm::ivec4 and unpack them in shader later
		std::array<glm::ivec4, gfx_api::max_indexed_lights> light_index = {};

		size_t bucketDimensionUsed = gfx_api::bucket_dimension;
	};

	virtual ~ILightingManager() = default;

	void SetFrameStart()
	{
		currentPointLightBuckets = {};
	}

	virtual void ComputeFrameData(const LightingData& data, LightMap& lightmap, const glm::mat4& worldViewProjectionMatrix) = 0;

	const PointLightBuckets& getPointLightBuckets() const
	{
		return currentPointLightBuckets;
	}

	protected:
		PointLightBuckets currentPointLightBuckets;
};


namespace renderingNew
{
	//! This lighting manager generate a proper PointLightBuckets for per pixel point lights
	struct LightingManager final : ILightingManager
	{
		void ComputeFrameData(const LightingData& data, LightMap& lightmap, const glm::mat4& worldViewProjectionMatrix) override;

		struct CalculatedPointLight
		{
			glm::vec3 position = glm::vec3(0, 0, 0);
			glm::vec3 colour;
			float range;
		};
	private:
		// cached containers to avoid frequent reallocations
		struct CulledLightInfo
		{
			CalculatedPointLight light;
			BoundingBox clipSpaceBoundingBox;
		};
		std::vector<CulledLightInfo> culledLights;
	};
}

void setLightingManager(std::unique_ptr<ILightingManager> manager);

ILightingManager& getCurrentLightingManager();
