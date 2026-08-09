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

#include "warzoneconfig.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <utility>

namespace ssao
{

namespace
{

constexpr float SSAO_RADIUS = 0.07f;
constexpr float SSAO_BIAS_FACTOR = 0.00002f;
constexpr float SSAO_MIN_BIAS = 0.025f;
constexpr float SSAO_RANGE_SCALE = 0.5f;
constexpr float SSAO_BLUR_DEPTH_SIGMA = 0.0015f;
constexpr float SSAO_INTENSITY = 1.3f;
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

void ssaoFillUvScaleClamp(gfx_api::abstract_texture* sourceTex, glm::vec4& uvScaleClamp)
{
	const auto renderedDims = gfx_api::context::get().getSceneRenderTargetDimensions();
	const auto textureDims = gfx_api::context::get().getRenderTargetDimensions(sourceTex).value_or(renderedDims);
	const float texW = static_cast<float>(std::max<uint32_t>(textureDims.first, 1));
	const float texH = static_cast<float>(std::max<uint32_t>(textureDims.second, 1));
	uvScaleClamp = glm::vec4(
		renderedDims.first / texW, renderedDims.second / texH,
		(renderedDims.first - 0.5f) / texW, (renderedDims.second - 0.5f) / texH);
}

void drawSSAOGenerate(
	gfx_api::abstract_texture* depthTexture,
	gfx_api::abstract_texture* normalsTexture,
	const glm::mat4& projectionMatrix,
	const glm::mat4& invProjectionMatrix,
	gfx_api::buffer* fullscreenTriVBO)
{
	const auto renderedDims = gfx_api::context::get().getSceneRenderTargetDimensions();
	if (renderedDims.first == 0 || renderedDims.second == 0)
	{
		return;
	}

	gfx_api::constant_buffer_type<SHADER_SSAO_GENERATE> constants {};
	constants.invProjectionMatrix = invProjectionMatrix;
	constants.projectionMatrix = projectionMatrix;
	constants.params = glm::vec4(SSAO_RADIUS, SSAO_BIAS_FACTOR, SSAO_MIN_BIAS, SSAO_RANGE_SCALE);
	constants.noiseScale = glm::vec2(
		static_cast<float>(renderedDims.first) / static_cast<float>(NOISE_TEXTURE_SIZE),
		static_cast<float>(renderedDims.second) / static_cast<float>(NOISE_TEXTURE_SIZE));
	std::memcpy(constants.kernel, s_kernel, sizeof(s_kernel));
	ssaoFillUvScaleClamp(depthTexture, constants.uvScaleClamp);

	gfx_api::SSAOGeneratePSO::get().bind();
	gfx_api::SSAOGeneratePSO::get().bind_constants(constants);
	gfx_api::SSAOGeneratePSO::get().bind_vertex_buffers(fullscreenTriVBO);
	gfx_api::SSAOGeneratePSO::get().bind_textures(depthTexture, normalsTexture, s_noiseTexture);
	gfx_api::SSAOGeneratePSO::get().draw(3, 0);
	gfx_api::SSAOGeneratePSO::get().unbind_vertex_buffers(fullscreenTriVBO);
}

void drawSSAOBlur(
	gfx_api::abstract_texture* occlusionTexture,
	gfx_api::abstract_texture* depthTexture,
	const glm::vec2& blurDirection,
	gfx_api::buffer* fullscreenTriVBO)
{
	gfx_api::constant_buffer_type<SHADER_SSAO_BLUR> constants {};
	constants.blurDirection = blurDirection;
	constants.depthSigma = SSAO_BLUR_DEPTH_SIGMA;
	ssaoFillUvScaleClamp(occlusionTexture, constants.uvScaleClamp);

	gfx_api::SSAOBlurPSO::get().bind();
	gfx_api::SSAOBlurPSO::get().bind_constants(constants);
	gfx_api::SSAOBlurPSO::get().bind_vertex_buffers(fullscreenTriVBO);
	gfx_api::SSAOBlurPSO::get().bind_textures(occlusionTexture, depthTexture);
	gfx_api::SSAOBlurPSO::get().draw(3, 0);
	gfx_api::SSAOBlurPSO::get().unbind_vertex_buffers(fullscreenTriVBO);
}

void recordBlur(const gfx_api::RenderPassContext& passCtx, const glm::vec2& blurDirection)
{
	ASSERT(passCtx.readCount() == 2, "SSAO blur: occlusion + depth");
	gfx_api::buffer* vbo = display3d_getScreenTriangleVBO();
	if (vbo == nullptr || passCtx.getRead(0) == nullptr || passCtx.getRead(1) == nullptr)
	{
		return;
	}
	drawSSAOBlur(passCtx.getRead(0), passCtx.getRead(1), blurDirection, vbo);
}

} // namespace

void applyConfigToGfx()
{
	auto& ctx = gfx_api::context::get();
	ctx.setSSAOSurfacesEnabled(war_getSSAO());
	if (!ctx.syncPipelineSurfaces())
	{
		debug(LOG_ERROR, "Failed to sync pipeline surfaces after SSAO config change (enabled=%d)",
			static_cast<int>(war_getSSAO()));
	}
}

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
	ASSERT(passCtx.readCount() == 2, "SSAO generate: depth + normals");
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
	drawSSAOGenerate(depth, normals, fc.perspectiveMatrix, glm::inverse(fc.perspectiveMatrix), vbo);
}

void recordBlurH(const gfx_api::RenderPassContext& passCtx)
{
	const auto dims = gfx_api::context::get().getSceneRenderTargetDimensions();
	if (dims.first != 0)
	{
		recordBlur(passCtx, glm::vec2(1.0f / static_cast<float>(dims.first), 0.0f));
	}
}

void recordBlurV(const gfx_api::RenderPassContext& passCtx)
{
	const auto dims = gfx_api::context::get().getSceneRenderTargetDimensions();
	if (dims.second != 0)
	{
		recordBlur(passCtx, glm::vec2(0.0f, 1.0f / static_cast<float>(dims.second)));
	}
}

void recordCompose(const gfx_api::RenderPassContext& passCtx)
{
	ASSERT(passCtx.readCount() == 4, "SSAO compose: scene + AO + normals + depth");
	gfx_api::buffer* vbo = display3d_getScreenTriangleVBO();
	gfx_api::abstract_texture* scene = passCtx.getRead(0);
	gfx_api::abstract_texture* ao = passCtx.getRead(1);
	gfx_api::abstract_texture* prepassNormals = passCtx.getRead(2);
	gfx_api::abstract_texture* prepassDepth = passCtx.getRead(3);
	if (vbo == nullptr || scene == nullptr || ao == nullptr || prepassNormals == nullptr || prepassDepth == nullptr
		|| !pie_IsInGame3DFrameContextReady())
	{
		return;
	}

	const RENDER_STATE renderState = getCurrentRenderState();
	const auto& fc = pie_GetInGame3DFrameContext();

	gfx_api::constant_buffer_type<SHADER_SCENE_COMPOSE_SSAO> constants {};
	constants.ssaoIntensity = SSAO_INTENSITY;
	constants.fogColor = pal_PIELIGHTtoVec4(pie_GetFogColour());
	// Match scene-pass fog uniform binding (terrain / meshes):
	// shader fogEnd <- fogBegin, shader fogStart <- fogEnd.
	constants.fogStart = renderState.fogEnd;
	constants.fogEnd = renderState.fogBegin;
	constants.fogEnabled = renderState.fogEnabled ? 1 : 0;
	constants.invProjectionMatrix = glm::inverse(fc.perspectiveMatrix);
	ssaoFillUvScaleClamp(scene, constants.uvScaleClamp);

	gfx_api::SceneComposeSSAOPSO::get().bind();
	gfx_api::SceneComposeSSAOPSO::get().bind_constants(constants);
	gfx_api::SceneComposeSSAOPSO::get().bind_vertex_buffers(vbo);
	gfx_api::SceneComposeSSAOPSO::get().bind_textures(scene, ao, prepassNormals, prepassDepth);
	gfx_api::SceneComposeSSAOPSO::get().draw(3, 0);
	gfx_api::SceneComposeSSAOPSO::get().unbind_vertex_buffers(vbo);
}

} // namespace ssao
