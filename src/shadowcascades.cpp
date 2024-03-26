/*
	This file is part of Warzone 2100.
	Copyright (C) 2023  Warzone 2100 Project

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

	------

	Portions of this file are derived from:
	https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmappingcascade/shadowmappingcascade.cpp

	The MIT License (MIT)

	Copyright (c) 2016 Sascha Willems

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#include "shadowcascades.h"
#include "display3d.h"
#include "profiling.h"
#include "lib/framework/fixedpoint.h"
#include "lib/ivis_opengl/piematrix.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_interpolation.hpp>

float cascadeSplitLambda = 0.3f;

void calculateShadowCascades(const iView *player, float terrainDistance, const glm::mat4& baseViewMatrix, const glm::vec3& lightInvDir, size_t SHADOW_MAP_CASCADE_COUNT, std::vector<Cascade>& output)
{
	WZ_PROFILE_SCOPE(calculateShadowCascades);
	output.clear();
	output.resize(SHADOW_MAP_CASCADE_COUNT);

	float pitch = UNDEG(-player->r.x);

	float adjustedZFar = static_cast<float>((pow((1.57f - pitch) / 1.57f, 0.9f) * 4000.f) + 2000.f + terrainDistance);

	std::vector<float> cascadeSplits(SHADOW_MAP_CASCADE_COUNT, 0.f);

	float nearClip = 330.f;

	float farClip = adjustedZFar; //pie_GetPerspectiveZFar();
	float clipRange = farClip - nearClip;

	float minZ = nearClip;
	float maxZ = nearClip + clipRange;

	float range = maxZ - minZ;
	float ratio = maxZ / minZ;

	// Calculate split depths based on view camera frustum
	// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
		float log = minZ * std::pow(ratio, p);
		float uniform = minZ + range * p;
		float d = cascadeSplitLambda * (log - uniform) + uniform;
		cascadeSplits[i] = (d - nearClip) / clipRange;
	}

	// Calculate orthographic projection matrix for each cascade
	float lastSplitDist = 0.0;
	for (uint32_t iSplit = 0; iSplit < SHADOW_MAP_CASCADE_COUNT; iSplit++)
	{
		float splitDist = cascadeSplits[iSplit];

		glm::vec3 frustumCorners[8] = {
			glm::vec3(-1.0f,  1.0f, -1.0f), // opengl
			glm::vec3( 1.0f,  1.0f, -1.0f),
			glm::vec3( 1.0f, -1.0f, -1.0f),
			glm::vec3(-1.0f, -1.0f, -1.0f),
			glm::vec3(-1.0f,  1.0f,  1.0f),
			glm::vec3( 1.0f,  1.0f,  1.0f),
			glm::vec3( 1.0f, -1.0f,  1.0f),
			glm::vec3(-1.0f, -1.0f,  1.0f),
		};

		// Project frustum corners into world space
		glm::mat4 invCam = glm::inverse(pie_PerspectiveGetConstrained(nearClip, adjustedZFar) * (/*glm::translate(glm::vec3(0.f, 0.f, -distance)) * */ baseViewMatrix * glm::translate(glm::vec3(-player->p.x, 0, player->p.z))));
		for (uint32_t i = 0; i < 8; i++) {
			glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
			frustumCorners[i] = invCorner / invCorner.w;
		}

		for (uint32_t i = 0; i < 4; i++) {
			glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
			frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
			frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
		}

		// Get frustum center
		glm::vec3 frustumCenter = glm::vec3(0.0f);
		for (uint32_t i = 0; i < 8; i++) {
			frustumCenter += frustumCorners[i];
		}
		frustumCenter /= 8.0f;

		float radius = 0.0f;
		for (uint32_t i = 0; i < 8; i++) {
			float distance2 = glm::length(frustumCorners[i] - frustumCenter);
			radius = glm::max(radius, distance2);
		}
		radius = std::ceil(radius * 16.0f) / 16.0f;

		glm::vec3 maxExtents = glm::vec3(radius);
		glm::vec3 minExtents = -maxExtents;

		glm::vec3 lightDir = normalize(lightInvDir); //normalize(-lightPos);
		glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter + lightDir, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightProjectionMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, minExtents.z, maxExtents.z) * glm::scale(glm::vec3(1.f, 1.f, -1.f));

		// Store split distance and matrix in cascade
		output[iSplit].splitDepth = (-nearClip + splitDist * clipRange);
		output[iSplit].viewMatrix = lightViewMatrix;
		output[iSplit].projectionMatrix = lightProjectionMatrix;

		lastSplitDist = cascadeSplits[iSplit];
	}
}
