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
/** @file dynamicresolution.cpp
 * Dynamic render resolution controller driven by GPU frame timing.
 *
 * Active while the Render Resolution option is set to Dynamic. Keeps the scene
 * targets at full size and steers the per frame scene render fraction from a
 * smoothed GPU frame time: stepping resolution down when the GPU nears the
 * frame budget and back up when there is comfortable headroom. On systems
 * whose timestamps are unusable or implausibly small the fraction simply
 * stays at native, so the failure mode is no scaling rather than bad scaling.
 */

#include "dynamicresolution.h"

#include "warzoneconfig.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/ivis_opengl/gfx_api.h"

#include <algorithm>

namespace
{

constexpr float DRS_FRACTION_STEP = 0.05f;
constexpr size_t DRS_ADJUST_INTERVAL_FRAMES = 30;
// step down when the smoothed GPU frame time exceeds this share of the budget
constexpr double DRS_UPPER_BUDGET_SHARE = 0.90;
// step back up when it drops below this share
constexpr double DRS_LOWER_BUDGET_SHARE = 0.70;
constexpr double DRS_SMOOTHING_ALPHA = 0.10;

struct DynamicResolutionState
{
	bool active = false;
	bool warnedUnsupported = false;
	float fraction = 1.f;
	double smoothedGpuFrameNs = 0.0;
	bool haveSample = false;
	size_t lastSampleFrame = 0;
	size_t framesSinceAdjust = 0;
};

DynamicResolutionState drsState;

double targetFrameTimeNs()
{
	// the display the window is currently on is the authority on the frame
	// budget (cached by the SDL layer and invalidated by display events, so
	// querying per evaluation is cheap)
	float refreshRate = wzGetCurrentDisplayRefreshRate();
	if (!(refreshRate > 0.f))
	{
		refreshRate = war_GetFullscreenModeRefreshRate();
	}
	if (!(refreshRate > 0.f))
	{
		refreshRate = 60.f;
	}
	return 1e9 / static_cast<double>(refreshRate);
}

void deactivateDynamicResolution(gfx_api::context& ctx)
{
	ctx.setSceneRenderFraction(1.f);
	ctx.setSceneDynamicResolution(false);
	ctx.setGpuFrameTimingEnabled(false);
	const bool keepWarned = drsState.warnedUnsupported;
	drsState = DynamicResolutionState();
	drsState.warnedUnsupported = keepWarned;
}

} // anonymous namespace

void dynamicResolutionUpdate()
{
	auto& ctx = gfx_api::context::get();
	const bool wantDynamic = (war_getRenderResolutionPercent() == 0);
	if (!wantDynamic)
	{
		if (drsState.active)
		{
			deactivateDynamicResolution(ctx);
		}
		return;
	}

	if (!ctx.supportsGpuFrameTiming())
	{
		if (drsState.active)
		{
			deactivateDynamicResolution(ctx);
		}
		if (!drsState.warnedUnsupported)
		{
			drsState.warnedUnsupported = true;
			debug(LOG_INFO, "Dynamic render resolution unavailable (no usable GPU frame timing), rendering at native resolution");
		}
		return;
	}

	if (!drsState.active)
	{
		if (!ctx.setGpuFrameTimingEnabled(true))
		{
			return;
		}
		ctx.setSceneDynamicResolution(true);
		drsState.active = true;
		drsState.fraction = 1.f;
		drsState.framesSinceAdjust = 0;
	}

	const auto timing = ctx.getLastGpuFrameTiming();
	if (timing.has_value() && timing->frameNum != drsState.lastSampleFrame)
	{
		drsState.lastSampleFrame = timing->frameNum;
		if (!drsState.haveSample)
		{
			drsState.smoothedGpuFrameNs = static_cast<double>(timing->durationNs);
			drsState.haveSample = true;
		}
		else
		{
			drsState.smoothedGpuFrameNs += DRS_SMOOTHING_ALPHA * (static_cast<double>(timing->durationNs) - drsState.smoothedGpuFrameNs);
		}
	}

	++drsState.framesSinceAdjust;
	if (drsState.haveSample && drsState.framesSinceAdjust >= DRS_ADJUST_INTERVAL_FRAMES)
	{
		drsState.framesSinceAdjust = 0;
		const double budgetNs = targetFrameTimeNs();
		float newFraction = drsState.fraction;
		if (drsState.smoothedGpuFrameNs > DRS_UPPER_BUDGET_SHARE * budgetNs)
		{
			newFraction -= DRS_FRACTION_STEP;
		}
		else if (drsState.smoothedGpuFrameNs < DRS_LOWER_BUDGET_SHARE * budgetNs)
		{
			newFraction += DRS_FRACTION_STEP;
		}
		newFraction = std::clamp(newFraction, gfx_api::context::minSceneRenderFraction, 1.f);
		if (newFraction != drsState.fraction)
		{
			drsState.fraction = newFraction;
			debug(LOG_3D, "Dynamic resolution: %d%% (smoothed GPU frame time %.2f ms, budget %.2f ms)",
				static_cast<int>(std::lround(drsState.fraction * 100.f)),
				drsState.smoothedGpuFrameNs / 1e6, budgetNs / 1e6);
		}
	}

	ctx.setSceneRenderFraction(drsState.fraction);
}
