// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/
/** @file contact_shadows.cpp
 * Screen-space contact shadows game-side module (sun directional, lighting-prepare).
 */

#include "contact_shadows.h"

#include "display3d_render_graph.h"
#include "display3d_render_internal.h"
#include "depth_aware_blur.h"

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piematrix.h"

Vector3f getTheSun();

#include <algorithm>
#include <cstdint>

namespace contact_shadows
{

static constexpr uint32_t kFullRes = 1;
static constexpr uint32_t kHalfRes = 2;
static constexpr uint32_t kQuarterRes = 4;

static constexpr ContactShadowSettings kPresets[] = {
	/* OFF    */ {},
	/* LOW    */ {true, kHalfRes,    kQuarterRes, 12, 2},
	/* NORMAL */ {true, kHalfRes,    kHalfRes,    16, 4},
	/* HIGH   */ {true, kFullRes,    kFullRes,    24, 4},
};
static_assert(sizeof(kPresets) / sizeof(kPresets[0]) == static_cast<size_t>(CONTACT_SHADOWS_MODE::HIGH) + 1,
	"CONTACT_SHADOWS_MODE and kPresets must stay in sync");

ContactShadowSettings settingsFor(CONTACT_SHADOWS_MODE mode)
{
	const size_t i = static_cast<size_t>(mode);
	ASSERT_OR_RETURN(ContactShadowSettings{}, i < sizeof(kPresets) / sizeof(kPresets[0]), "bad CONTACT_SHADOWS_MODE");
	const ContactShadowSettings s = kPresets[i];
	if (s.enabled)
	{
		ASSERT(s.generateDivisor >= 1, "bad generateDivisor");
		ASSERT(s.blurDivisor >= s.generateDivisor, "blur finer than generate");
		ASSERT(s.stepCount > 0, "bad stepCount");
		ASSERT(s.blurTapPairs >= 1 && s.blurTapPairs <= 4, "bad tapPairs");
	}
	return s;
}

ContactShadowSettings activeSettings()
{
	ContactShadowSettings s = settingsFor(war_getContactShadowsMode());
	if (s.enabled && !pie_supportsShadowMapping().value_or(false))
	{
		s.enabled = false;
	}
	return s;
}

namespace
{

struct Tuning
{
	float maxRayLength = 96.f;
	float thicknessCap = 24.f;
	float rayBias = 2.f;
	float minRayStart = 0.5f;
	float fadeStartViewZ = 1500.f;
	float fadeEndViewZ = 4000.f;
	float edgeFadeWidth = 0.05f;
	float blurDepthSigma = 0.0015f;
};

Tuning s_tuning {};
gfx_api::texture* s_dummyLit = nullptr;

} // namespace

void init()
{
	if (s_dummyLit != nullptr)
	{
		return;
	}
	iV_Image white;
	if (!white.allocate(1, 1, 1, true))
	{
		debug(LOG_ERROR, "Failed to allocate contact-shadow dummy texture");
		return;
	}
	white.bmp_w()[0] = 255;
	s_dummyLit = gfx_api::context::get().createTextureForCompatibleImageUploads(1, white, "contact_shadow_dummy");
	if (s_dummyLit == nullptr || !s_dummyLit->upload(0, white))
	{
		debug(LOG_ERROR, "Failed to upload contact-shadow dummy texture");
		delete s_dummyLit;
		s_dummyLit = nullptr;
	}
	syncLightingConstant();
}

void shutdown()
{
	delete s_dummyLit;
	s_dummyLit = nullptr;
}

void syncLightingConstant()
{
	if (!gfx_api::context::isInitialized())
	{
		return;
	}
	auto c = gfx_api::context::get().getShadowConstants();
	c.contactShadowsEnabled = activeSettings().enabled;
	gfx_api::context::get().setShadowConstants(c);
}

gfx_api::texture* dummyLitTexture()
{
	return s_dummyLit;
}

void recordGenerate(const gfx_api::RenderPassContext& passCtx)
{
	ASSERT(passCtx.readCount() == 2, "Contact shadow generate: 0 depth, 1 normals");
	if (!pie_IsInGame3DFrameContextReady())
	{
		return;
	}
	gfx_api::abstract_texture* depth = passCtx.getRead(0);
	gfx_api::abstract_texture* normals = passCtx.getRead(1);
	if (depth == nullptr || normals == nullptr)
	{
		return;
	}

	const auto& fc = pie_GetInGame3DFrameContext();
	gfx_api::constant_buffer_type<SHADER_CONTACT_SHADOW_GENERATE> c {};
	c.invProjectionMatrix = glm::inverse(fc.perspectiveMatrix);
	c.projectionMatrix = fc.perspectiveMatrix;
	c.viewMatrix = fc.viewMatrix;
	c.sunDirWorld = glm::vec4(glm::normalize(-getTheSun()), 0.f);
	c.params = glm::vec4(s_tuning.maxRayLength, s_tuning.thicknessCap,
		pie_getPerspectiveZClose(), s_tuning.rayBias);
	c.fadeParams = glm::vec4(s_tuning.minRayStart, s_tuning.fadeStartViewZ,
		s_tuning.fadeEndViewZ, s_tuning.edgeFadeWidth);
	const auto vp = passCtx.writeViewportSize();
	const float invW = (vp && vp->first > 0) ? 1.f / static_cast<float>(vp->first) : 0.f;
	const float invH = (vp && vp->second > 0) ? 1.f / static_cast<float>(vp->second) : 0.f;
	c.generatePixelUV = glm::vec4(invW, invH, 0.f, 0.f);
	c.projZCoeffs = glm::vec2(fc.perspectiveMatrix[2][2], fc.perspectiveMatrix[3][2]);
	c.stepCount = float(activeSettings().stepCount);
	display3d_fillPassReadUvScaleClamp(passCtx, 0, c.prepassUvScaleClamp);
	display3d_drawFullscreenTriangle<gfx_api::ContactShadowGeneratePSO>(c, depth, normals);
}

void recordDownsample(const gfx_api::RenderPassContext& passCtx)
{
	post_effect_blur::recordBilinearResample(passCtx, "ContactShadow downsample");
}

void recordBlurH(const gfx_api::RenderPassContext& passCtx)
{
	post_effect_blur::recordDepthAwareBlur<SHADER_CONTACT_SHADOW_BLUR, gfx_api::ContactShadowBlurPSO>(
		passCtx, post_effect_blur::Axis::Horizontal, s_tuning.blurDepthSigma,
		static_cast<uint32_t>(activeSettings().blurTapPairs), "ContactShadow blur");
}

void recordBlurV(const gfx_api::RenderPassContext& passCtx)
{
	post_effect_blur::recordDepthAwareBlur<SHADER_CONTACT_SHADOW_BLUR, gfx_api::ContactShadowBlurPSO>(
		passCtx, post_effect_blur::Axis::Vertical, s_tuning.blurDepthSigma,
		static_cast<uint32_t>(activeSettings().blurTapPairs), "ContactShadow blur");
}

} // namespace contact_shadows
