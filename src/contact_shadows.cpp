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
/** @file contact_shadows.cpp
 * Screen-space contact shadows game-side module (sun directional, lighting-prepare).
 */

#include "contact_shadows.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/piestate.h"

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

void init()
{
}

void shutdown()
{
}

void syncLightingConstant()
{
}

void recordGenerate(const gfx_api::RenderPassContext& passCtx)
{
	(void)passCtx;
}

void recordDownsample(const gfx_api::RenderPassContext& passCtx)
{
	(void)passCtx;
}

void recordBlurH(const gfx_api::RenderPassContext& passCtx)
{
	(void)passCtx;
}

void recordBlurV(const gfx_api::RenderPassContext& passCtx)
{
	(void)passCtx;
}

} // namespace contact_shadows
