// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** @file ssr.cpp
 * Screen-space reflections (SSR) game-side module (water-only v1).
 */

#include "ssr.h"

#include "display3d.h"
#include "display3d_render_graph.h"
#include "display3d_render_internal.h"
#include "depth_aware_blur.h"
#include "terrain.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/pielight_convert.h"
#include "lib/ivis_opengl/piestate.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstdint>

namespace ssr
{

static constexpr uint32_t kFull = 1;
static constexpr uint32_t kHalf = 2;
static constexpr uint32_t kQuarter = 4;

static constexpr SsrSettings kSsrPresets[] = {
	/* OFF    */ {},
	/* LOW    */ {true, kQuarter, kQuarter, 12, 2},
	/* NORMAL */ {true, kHalf,    kQuarter, 16, 2},
	/* HIGH   */ {true, kHalf,    kHalf,    32, 4},
	/* ULTRA  */ {true, kFull,    kFull,    48, 4},
};
static_assert(sizeof(kSsrPresets) / sizeof(kSsrPresets[0]) == static_cast<size_t>(SSR_MODE::ULTRA) + 1,
	"SSR_MODE and kSsrPresets must stay in sync");

SsrSettings settingsFor(SSR_MODE mode)
{
	const size_t i = static_cast<size_t>(mode);
	ASSERT_OR_RETURN(SsrSettings{}, i < sizeof(kSsrPresets) / sizeof(kSsrPresets[0]), "bad SSR_MODE");
	const SsrSettings s = kSsrPresets[i];
	if (s.enabled)
	{
		ASSERT(s.generateDivisor >= 1, "bad generateDivisor");
		ASSERT(s.blurDivisor >= s.generateDivisor, "blur finer than generate");
		ASSERT(s.stepCount > 0 && s.stepCount <= 64, "bad stepCount");
		ASSERT(s.blurTapPairs >= 1 && s.blurTapPairs <= 4, "bad tapPairs");
	}
	return s;
}

SsrSettings activeSettings()
{
	SsrSettings s = settingsFor(war_getSsrMode());
	if (s.enabled && getTerrainShaderQuality() != TerrainShaderQuality::NORMAL_MAPPING)
	{
		s.enabled = false;
	}
	return s;
}

bool surfacesRequested()
{
	return activeSettings().enabled;
}

namespace
{

struct Tuning
{
	/// View-space march length. Fog-scale map units (default zoom ~2600, fog end 8000).
	float maxRayLength;
	/// Upper cap on the generate slab min(cap, max(8, 0.01*|surfZ|)). At typical
	/// view-Z the relative term is smaller. McGuire JCGT 3(4) Figure 3.
	float thickness;
	/// Unused by the generate shader (start offset is MIN_RAY_START_ABS). Packed in params.z.
	float minRayStart;
	/// Sigma of the blur's depth falloff, in normalized depth units
	float blurDepthSigma;
	/// Analog compose gain after Schlick F. Not a water-shader blend and not a
	/// facing floor.
	float intensity;
	/// Scales how much Fresnel-weighted SSR replaces the ScenePass water+bed
	/// color. Water was already drawn; this does not change the water shader.
	/// 1 leaves little of that ScenePass look.
	float overWaterMix;
	/// Schlick F0 when looking straight down. 0.1 is above water (~0.02) /
	/// dielectric (~0.04) so default-tilt hits read; look-down is still F0,
	/// not a min floor.
	float F0;
};

constexpr Tuning DEFAULT_TUNING = {
	.maxRayLength = 8000.f,
	.thickness = 80.f,
	.minRayStart = 0.004f,
	.blurDepthSigma = 0.0025f,
	.intensity = 2.0f,
	.overWaterMix = 0.8f,
	.F0 = 0.1f,
};

Tuning s_tuning = DEFAULT_TUNING;

// Graph reads are depth, normals, and scene. skyboxTexture is CPU-bound and may be null.
void drawSSRGenerate(
	const gfx_api::RenderPassContext& passCtx,
	gfx_api::abstract_texture* depthTexture,
	gfx_api::abstract_texture* normalsTexture,
	gfx_api::abstract_texture* sceneTexture,
	gfx_api::abstract_texture* skyboxTexture,
	const glm::mat4& projectionMatrix,
	const glm::mat4& invProjectionMatrix,
	const glm::mat4& viewMatrix)
{
	gfx_api::constant_buffer_type<SHADER_SSR_GENERATE> constants {};
	constants.invProjectionMatrix = invProjectionMatrix;
	constants.projectionMatrix = projectionMatrix;
	const float skyScale = std::max(getCurrentSkyboxScale(), 1.f);
	const glm::mat3 invViewRot = glm::inverse(glm::mat3(viewMatrix));
	const glm::mat3 invWind = glm::mat3(glm::rotate(glm::mat4(1.f), glm::radians(-getCurrentSkyboxWindAngle()), glm::vec3(0.f, 1.f, 0.f)));
	const glm::mat3 invScale(
		glm::vec3(1.f / skyScale, 0.f, 0.f),
		glm::vec3(0.f, 2.f / skyScale, 0.f),
		glm::vec3(0.f, 0.f, 1.f / skyScale));
	constants.viewToSkyLocal = glm::mat4(invScale * invWind * invViewRot);
	// params.z is packed for std140; generate uses MIN_RAY_START_ABS instead of minRayStart.
	constants.params = glm::vec4(s_tuning.maxRayLength, s_tuning.thickness, s_tuning.minRayStart, 0.f);
	display3d_fillPassReadUvScaleClamp(passCtx, 0, constants.prepassUvScaleClamp);
	display3d_fillPassReadUvScaleClamp(passCtx, 2, constants.sceneUvScaleClamp);
	const auto& renderState = getCurrentRenderState();
	const glm::vec4 fog = pielightToRGBAVec4(renderState.fogColour);
	constants.skyFogColor = glm::vec4(fog.r, fog.g, fog.b, renderState.fogEnabled ? 1.f : 0.f);
	// Packed for std140; generate uses min(pixelCount, MAX_STEPS) instead of this preset.
	constants.stepCount = static_cast<float>(activeSettings().stepCount);
	constants.skyboxAvailable = skyboxTexture != nullptr ? 1.f : 0.f;

	display3d_drawFullscreenTriangle<gfx_api::SSRGeneratePSO>(constants, depthTexture, normalsTexture, sceneTexture, skyboxTexture);
}

} // namespace

void init()
{
}

void shutdown()
{
}

void recordGenerate(const gfx_api::RenderPassContext& passCtx)
{
	// Graph reads: 0 depth, 1 normals, 2 scene. The skybox is pie_Skybox_GetTexture(),
	// a persistent CPU texture, not a PassId read.
	ASSERT(passCtx.readCount() == 3, "SSR generate: 0 depth, 1 normals, 2 scene");
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}

	gfx_api::abstract_texture* depth = passCtx.getRead(0);
	gfx_api::abstract_texture* normals = passCtx.getRead(1);
	gfx_api::abstract_texture* scene = passCtx.getRead(2);
	if (depth == nullptr || normals == nullptr || scene == nullptr)
	{
		return;
	}

	const auto& fc = pie_GetInGame3DFrameContext();
	drawSSRGenerate(passCtx, depth, normals, scene, pie_Skybox_GetTexture(), fc.perspectiveMatrix, glm::inverse(fc.perspectiveMatrix), fc.viewMatrix);
}

void recordDownsample(const gfx_api::RenderPassContext& passCtx)
{
	post_effect_blur::recordBilinearResample(passCtx, "SSR downsample");
}

void recordBlurH(const gfx_api::RenderPassContext& passCtx)
{
	post_effect_blur::recordDepthAwareBlur<SHADER_SSR_BLUR, gfx_api::SSRBlurPSO>(
		passCtx, post_effect_blur::Axis::Horizontal, s_tuning.blurDepthSigma,
		activeSettings().blurTapPairs, "SSR blur");
}

void recordBlurV(const gfx_api::RenderPassContext& passCtx)
{
	post_effect_blur::recordDepthAwareBlur<SHADER_SSR_BLUR, gfx_api::SSRBlurPSO>(
		passCtx, post_effect_blur::Axis::Vertical, s_tuning.blurDepthSigma,
		activeSettings().blurTapPairs, "SSR blur");
}

void recordCompose(const gfx_api::RenderPassContext& passCtx)
{
	ASSERT(passCtx.readCount() == 4, "SSR compose: 0 scene, 1 ssr, 2 normals, 3 depth");
	gfx_api::abstract_texture* scene = passCtx.getRead(0);
	gfx_api::abstract_texture* ssrTex = passCtx.getRead(1);
	gfx_api::abstract_texture* prepassNormals = passCtx.getRead(2);
	gfx_api::abstract_texture* prepassDepth = passCtx.getRead(3);
	if (scene == nullptr || ssrTex == nullptr || prepassNormals == nullptr || prepassDepth == nullptr
		|| !pie_IsInGame3DFrameContextReady())
	{
		return;
	}

	const auto& fc = pie_GetInGame3DFrameContext();
	gfx_api::constant_buffer_type<SHADER_SCENE_COMPOSE_SSR> constants {};
	constants.invProjectionMatrix = glm::inverse(fc.perspectiveMatrix);
	constants.intensity = s_tuning.intensity * s_tuning.overWaterMix;
	constants.F0 = s_tuning.F0;
	display3d_fillPassReadUvScaleClamp(passCtx, 0, constants.sceneUvScaleClamp);
	display3d_fillPassReadUvScaleClamp(passCtx, 1, constants.ssrUvScaleClamp);
	display3d_fillPassReadUvScaleClamp(passCtx, 2, constants.normalsUvScaleClamp);
	display3d_fillPassReadUvScaleClamp(passCtx, 3, constants.depthUvScaleClamp);
	display3d_drawFullscreenTriangle<gfx_api::SceneComposeSSRPSO>(constants, scene, ssrTex, prepassNormals, prepassDepth);
}

} // namespace ssr
