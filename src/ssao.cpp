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
/** @file ssao.cpp
 * Screen-space ambient occlusion (SSAO) game-side module.
 */

#include "ssao.h"

#include "display3d_render_graph.h"
#include "display3d_render_internal.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/piestate.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <utility>

namespace ssao
{

static constexpr uint32_t kSsaoFullRes = 1;
static constexpr uint32_t kSsaoHalfRes = 2;
static constexpr uint32_t kSsaoQuarterRes = 4;

static constexpr SsaoSettings kSsaoPresets[] = {
	/* OFF    */ {},
	/* LOW    */ {true, kSsaoQuarterRes, kSsaoQuarterRes, 8, 2},
	/* NORMAL */ {true, kSsaoHalfRes,    kSsaoQuarterRes, 8, 2},
	/* HIGH   */ {true, kSsaoHalfRes,    kSsaoHalfRes,   16, 4},
	/* ULTRA  */ {true, kSsaoFullRes,    kSsaoFullRes,   16, 4},
};
static_assert(sizeof(kSsaoPresets) / sizeof(kSsaoPresets[0]) == static_cast<size_t>(SSAO_MODE::ULTRA) + 1,
	"SSAO_MODE and kSsaoPresets must stay in sync");

SsaoSettings settingsFor(SSAO_MODE mode)
{
	const size_t i = static_cast<size_t>(mode);
	ASSERT_OR_RETURN(SsaoSettings{}, i < sizeof(kSsaoPresets) / sizeof(kSsaoPresets[0]), "bad SSAO_MODE");
	const SsaoSettings s = kSsaoPresets[i];
	if (s.enabled)
	{
		ASSERT(s.generateDivisor >= 1, "bad generateDivisor");
		ASSERT(s.blurDivisor >= s.generateDivisor, "blur finer than generate");
		ASSERT(s.sampleCount > 0 && s.sampleCount <= static_cast<int>(gfx_api::SSAO_KERNEL_SIZE), "bad sampleCount");
		ASSERT(s.blurTapPairs >= 1 && s.blurTapPairs <= 4, "bad tapPairs");
	}
	return s;
}

SsaoSettings activeSettings()
{
	return settingsFor(war_getSsaoMode());
}

namespace
{

/// Analog SSAO look (radius, bias, intensity, blur sigma). Shared by every quality
/// preset; SsaoSettings only varies sample count, blur width, and resolution divisors.
/// Grouped so a later table can still swap this whole set at once.
struct Tuning
{
	/// Hemisphere sample radius, scaled by view depth in the generate shader
	float radius;
	/// Depth-proportional term of the self-occlusion bias
	float biasFactor;
	/// Lower bound on the self-occlusion bias, which dominates close to the camera
	float minBias;
	/// Occluder depth difference at which a sample stops contributing, as a fraction of radius
	float rangeScale;
	/// Sigma of the blur's depth falloff, in normalized depth units
	float blurDepthSigma;
	/// Strength of the occlusion multiply applied by the compose
	float intensity;
};

constexpr Tuning DEFAULT_TUNING = {
	.radius = 0.07f,
	.biasFactor = 0.00002f,
	.minBias = 0.025f,
	.rangeScale = 0.5f,
	.blurDepthSigma = 0.0015f,
	.intensity = 1.3f,
};

Tuning s_tuning = DEFAULT_TUNING;

/// Edge length of the tiling noise texture. Structural rather than tunable (it sets both the texture allocation and the tile count the blur is sized to remove).
constexpr int NOISE_TEXTURE_SIZE = 4;

gfx_api::texture* s_noiseTexture = nullptr;
glm::vec4 s_kernel[gfx_api::SSAO_KERNEL_SIZE] = {};

/// Deterministic 32-bit hash -> [0, 1). No rand().
float hash01(uint32_t n)
{
	n = (n ^ 61u) ^ (n >> 16);
	n *= 9u;
	n = n ^ (n >> 4);
	n *= 0x27d4eb2du;
	n = n ^ (n >> 15);
	return static_cast<float>(n) / static_cast<float>(UINT32_MAX);
}

void initKernel()
{
	for (size_t i = 0; i < gfx_api::SSAO_KERNEL_SIZE; ++i)
	{
		const uint32_t base = static_cast<uint32_t>(i) * 4u + 1u;
		glm::vec3 sample(
			hash01(base + 0u) * 2.0f - 1.0f,
			hash01(base + 1u) * 2.0f - 1.0f,
			hash01(base + 2u)); // hemisphere: z in [0, 1]
		sample = glm::normalize(sample);
		const float scale = static_cast<float>(i) / static_cast<float>(gfx_api::SSAO_KERNEL_SIZE);
		sample *= glm::mix(0.1f, 1.0f, scale * scale);
		s_kernel[i] = glm::vec4(sample, 0.0f);
	}
}

bool initNoiseTexture()
{
	iV_Image noiseImage;
	if (!noiseImage.allocate(NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE, 4, false))
	{
		return false;
	}

	unsigned char* pixels = noiseImage.bmp_w();
	for (int i = 0; i < NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE; ++i)
	{
		// Random unit vectors in the XY plane (packed to [0, 1] for the texture).
		const float x = hash01(static_cast<uint32_t>(i) * 3u + 11u) * 2.0f - 1.0f;
		const float y = hash01(static_cast<uint32_t>(i) * 3u + 12u) * 2.0f - 1.0f;
		glm::vec2 v = glm::normalize(glm::vec2(x, y));
		pixels[i * 4 + 0] = static_cast<unsigned char>((v.x * 0.5f + 0.5f) * 255.0f);
		pixels[i * 4 + 1] = static_cast<unsigned char>((v.y * 0.5f + 0.5f) * 255.0f);
		pixels[i * 4 + 2] = 127; // ~0 after *2-1
		pixels[i * 4 + 3] = 255;
	}

	s_noiseTexture = gfx_api::context::get().createTextureForCompatibleImageUploads(1, noiseImage, "ssao_noise");
	return s_noiseTexture != nullptr && s_noiseTexture->upload(0, noiseImage);
}

void drawSSAOGenerate(
	const gfx_api::RenderPassContext& passCtx,
	gfx_api::abstract_texture* depthTexture,
	gfx_api::abstract_texture* normalsTexture,
	const glm::mat4& projectionMatrix,
	const glm::mat4& invProjectionMatrix,
	gfx_api::buffer* fullscreenTriVBO)
{
	const auto writeId = passCtx.writeSurfaceId(0);
	if (!writeId.has_value())
	{
		return;
	}
	const auto genDims = gfx_api::context::get().getPipelineSurfaceDimensions(*writeId)
		.value_or(std::pair<uint32_t, uint32_t>{0, 0});
	if (genDims.first == 0 || genDims.second == 0)
	{
		return;
	}

	gfx_api::constant_buffer_type<SHADER_SSAO_GENERATE> constants {};
	constants.invProjectionMatrix = invProjectionMatrix;
	constants.projectionMatrix = projectionMatrix;
	constants.params = glm::vec4(s_tuning.radius, s_tuning.biasFactor, s_tuning.minBias, s_tuning.rangeScale);
	// Noise UVs are scaled from the generate target's allocated size so the tile count
	// matches the AO buffer, not the scene-sized depth texture.
	constants.noiseScale = glm::vec2(
		static_cast<float>(genDims.first) / static_cast<float>(NOISE_TEXTURE_SIZE),
		static_cast<float>(genDims.second) / static_cast<float>(NOISE_TEXTURE_SIZE));
	constants.sampleCount = static_cast<float>(activeSettings().sampleCount);
	std::memcpy(constants.kernel, s_kernel, sizeof(s_kernel));
	display3d_fillPassReadUvScaleClamp(passCtx, 0, constants.uvScaleClamp);

	gfx_api::SSAOGeneratePSO::get().bind();
	gfx_api::SSAOGeneratePSO::get().bind_constants(constants);
	gfx_api::SSAOGeneratePSO::get().bind_vertex_buffers(fullscreenTriVBO);
	gfx_api::SSAOGeneratePSO::get().bind_textures(depthTexture, normalsTexture, s_noiseTexture);
	gfx_api::SSAOGeneratePSO::get().draw(3, 0);
	gfx_api::SSAOGeneratePSO::get().unbind_vertex_buffers(fullscreenTriVBO);
}

void drawSSAOBlur(
	const gfx_api::RenderPassContext& passCtx,
	gfx_api::abstract_texture* occlusionTexture,
	gfx_api::abstract_texture* depthTexture,
	const glm::vec2& blurDirection,
	gfx_api::buffer* fullscreenTriVBO)
{
	gfx_api::constant_buffer_type<SHADER_SSAO_BLUR> constants {};
	constants.blurDirection = blurDirection;
	constants.depthSigma = s_tuning.blurDepthSigma;
	constants.tapPairs = static_cast<float>(activeSettings().blurTapPairs);
	display3d_fillPassReadUvScaleClamp(passCtx, 0, constants.occlusionUvScaleClamp);
	display3d_fillPassReadUvScaleClamp(passCtx, 1, constants.depthUvScaleClamp);

	gfx_api::SSAOBlurPSO::get().bind();
	gfx_api::SSAOBlurPSO::get().bind_constants(constants);
	gfx_api::SSAOBlurPSO::get().bind_vertex_buffers(fullscreenTriVBO);
	gfx_api::SSAOBlurPSO::get().bind_textures(occlusionTexture, depthTexture);
	gfx_api::SSAOBlurPSO::get().draw(3, 0);
	gfx_api::SSAOBlurPSO::get().unbind_vertex_buffers(fullscreenTriVBO);
}

enum class BlurAxis
{
	Horizontal,
	Vertical,
};

void recordBlur(const gfx_api::RenderPassContext& passCtx, BlurAxis axis)
{
	ASSERT(passCtx.readCount() == 2, "SSAO blur: occlusion + depth");
	gfx_api::buffer* vbo = display3d_getScreenTriangleVBO();
	if (vbo == nullptr || passCtx.getRead(0) == nullptr || passCtx.getRead(1) == nullptr)
	{
		return;
	}
	// Taps are in pass texCoord space (0-1 over the used write viewport).
	const auto used = passCtx.writeViewportSize().value_or(std::pair<uint32_t, uint32_t>{1, 1});
	const float usedW = static_cast<float>(std::max(used.first, 1u));
	const float usedH = static_cast<float>(std::max(used.second, 1u));
	const glm::vec2 blurDirection = (axis == BlurAxis::Horizontal)
		? glm::vec2(1.0f / usedW, 0.0f)
		: glm::vec2(0.0f, 1.0f / usedH);
	drawSSAOBlur(passCtx, passCtx.getRead(0), passCtx.getRead(1), blurDirection, vbo);
}

} // namespace

void init()
{
	initKernel();
	if (s_noiseTexture == nullptr && !initNoiseTexture())
	{
		debug(LOG_ERROR, "Failed to initialize SSAO noise texture");
	}
}

void shutdown()
{
	delete s_noiseTexture;
	s_noiseTexture = nullptr;
}

void recordGenerate(const gfx_api::RenderPassContext& passCtx)
{
	ASSERT(passCtx.readCount() == 2, "SSAO generate: 0 depth, 1 normals");
	if (s_noiseTexture == nullptr || !pie_IsInGame3DFrameContextReady())
	{
		return;
	}

	gfx_api::buffer* vbo = display3d_getScreenTriangleVBO();
	gfx_api::abstract_texture* depth = passCtx.getRead(0);
	gfx_api::abstract_texture* normals = passCtx.getRead(1);
	if (vbo == nullptr || depth == nullptr || normals == nullptr)
	{
		return;
	}

	const auto& fc = pie_GetInGame3DFrameContext();
	drawSSAOGenerate(passCtx, depth, normals, fc.perspectiveMatrix, glm::inverse(fc.perspectiveMatrix), vbo);
}

void recordDownsample(const gfx_api::RenderPassContext& passCtx)
{
	ASSERT(passCtx.readCount() == 1, "SSAO downsample: 0 generate AO");
	gfx_api::buffer* vbo = display3d_getScreenTriangleVBO();
	gfx_api::abstract_texture* ao = passCtx.getRead(0);
	if (vbo == nullptr || ao == nullptr)
	{
		return;
	}

	gfx_api::constant_buffer_type<SHADER_SSAO_DOWNSAMPLE> constants {};
	display3d_fillPassReadUvScaleClamp(passCtx, 0, constants.uvScaleClamp);

	gfx_api::SSAODownsamplePSO::get().bind();
	gfx_api::SSAODownsamplePSO::get().bind_constants(constants);
	gfx_api::SSAODownsamplePSO::get().bind_vertex_buffers(vbo);
	gfx_api::SSAODownsamplePSO::get().bind_textures(ao);
	gfx_api::SSAODownsamplePSO::get().draw(3, 0);
	gfx_api::SSAODownsamplePSO::get().unbind_vertex_buffers(vbo);
}

void recordBlurH(const gfx_api::RenderPassContext& passCtx)
{
	recordBlur(passCtx, BlurAxis::Horizontal);
}

void recordBlurV(const gfx_api::RenderPassContext& passCtx)
{
	recordBlur(passCtx, BlurAxis::Vertical);
}

void recordCompose(const gfx_api::RenderPassContext& passCtx)
{
	ASSERT(passCtx.readCount() == 3, "SSAO compose: 0 scene, 1 AO, 2 normals");
	gfx_api::buffer* vbo = display3d_getScreenTriangleVBO();
	gfx_api::abstract_texture* scene = passCtx.getRead(0);
	gfx_api::abstract_texture* ao = passCtx.getRead(1);
	gfx_api::abstract_texture* prepassNormals = passCtx.getRead(2);
	if (vbo == nullptr || scene == nullptr || ao == nullptr || prepassNormals == nullptr
		|| !pie_IsInGame3DFrameContextReady())
	{
		return;
	}

	gfx_api::constant_buffer_type<SHADER_SCENE_COMPOSE_SSAO> constants {};
	constants.ssaoIntensity = s_tuning.intensity;
	display3d_fillPassReadUvScaleClamp(passCtx, 0, constants.sceneUvScaleClamp);
	display3d_fillPassReadUvScaleClamp(passCtx, 1, constants.aoUvScaleClamp);

	gfx_api::SceneComposeSSAOPSO::get().bind();
	gfx_api::SceneComposeSSAOPSO::get().bind_constants(constants);
	gfx_api::SceneComposeSSAOPSO::get().bind_vertex_buffers(vbo);
	gfx_api::SceneComposeSSAOPSO::get().bind_textures(scene, ao, prepassNormals);
	gfx_api::SceneComposeSSAOPSO::get().draw(3, 0);
	gfx_api::SceneComposeSSAOPSO::get().unbind_vertex_buffers(vbo);
}

} // namespace ssao
