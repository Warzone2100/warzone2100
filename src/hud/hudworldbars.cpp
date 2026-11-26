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
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#include "hudworldbars.h"
#include "hudsettings.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"

#include <unordered_map>

namespace
{

constexpr float HIGH = 0.75f;
constexpr float LOW = 0.5f;
constexpr float TRAIL_DECAY_PER_SECOND = 0.6f;
constexpr uint32_t TRAIL_HOLD_MS = 250;
constexpr uint32_t FORGET_AFTER_MS = 3000;
constexpr float MIN_WIDTH_FOR_TICKS = 24.f;

struct Trail
{
	float shown = 1.f;
	uint32_t lastDropTime = 0;
	uint32_t lastSeenTime = 0;
};

std::unordered_map<uint32_t, Trail> trails;
uint32_t lastPruneTime = 0;

PIELIGHT fillColor(float fraction)
{
	if (fraction > HIGH)
	{
		return pal_RGBA(0, 158, 115, 255);
	}
	if (fraction > LOW)
	{
		return pal_RGBA(240, 190, 40, 255);
	}
	return pal_RGBA(213, 94, 0, 255);
}

float trailFor(uint32_t objectId, float fraction)
{
	Trail &trail = trails[objectId];
	if (trail.lastSeenTime == 0 || fraction >= trail.shown)
	{
		trail.shown = fraction;
	}
	else if (realTime - trail.lastDropTime > TRAIL_HOLD_MS)
	{
		const float elapsed = static_cast<float>(realTime - trail.lastSeenTime) / 1000.f;
		trail.shown = std::max(fraction, trail.shown - elapsed * TRAIL_DECAY_PER_SECOND);
	}
	if (fraction < trail.shown && trail.lastDropTime + TRAIL_HOLD_MS < realTime && trail.shown - fraction > 0.2f)
	{
		trail.lastDropTime = realTime;
	}
	trail.lastSeenTime = realTime;
	return trail.shown;
}

void prune()
{
	if (realTime - lastPruneTime < FORGET_AFTER_MS)
	{
		return;
	}
	lastPruneTime = realTime;
	for (auto it = trails.begin(); it != trails.end();)
	{
		it = realTime - it->second.lastSeenTime > FORGET_AFTER_MS ? trails.erase(it) : std::next(it);
	}
}

}

bool hudReadableBarsActive()
{
	return hudModuleActive(HudModuleId::ReadableBars);
}

void hudQueueReadableBar(BatchedMultiRectRenderer &renderer, size_t rectGroup, float x0, float x1, float y, float fillHeight, float fraction, uint32_t objectId)
{
	prune();
	fraction = std::max(0.f, std::min(fraction, 1.f));
	const float width = x1 - x0;
	const float trail = trailFor(objectId, fraction);
	renderer.addRectF(PIERECT_DrawRequest_f(x0 - 1, y - 1, x1 + 1, y + fillHeight + 1, pal_RGBA(0, 0, 0, 220)), rectGroup);
	renderer.addRectF(PIERECT_DrawRequest_f(x0, y, x1, y + fillHeight, pal_RGBA(45, 45, 50, 230)), rectGroup);
	if (trail > fraction)
	{
		renderer.addRectF(PIERECT_DrawRequest_f(x0 + width * fraction, y, x0 + width * trail, y + fillHeight, pal_RGBA(250, 250, 250, 210)), rectGroup);
	}
	const PIELIGHT color = fillColor(fraction);
	renderer.addRectF(PIERECT_DrawRequest_f(x0, y, x0 + width * fraction, y + fillHeight, color), rectGroup);
	renderer.addRectF(PIERECT_DrawRequest_f(x0, y, x0 + width * fraction, y + 1, pal_RGBA(255, 255, 255, 70)), rectGroup);
	if (width >= MIN_WIDTH_FOR_TICKS)
	{
		for (int tick = 1; tick < 4; ++tick)
		{
			const float tickX = x0 + width * static_cast<float>(tick) / 4.f;
			renderer.addRectF(PIERECT_DrawRequest_f(tickX, y, tickX + 1, y + fillHeight, pal_RGBA(0, 0, 0, 150)), rectGroup);
		}
	}
}

void hudReleaseReadableBars()
{
	trails.clear();
}
