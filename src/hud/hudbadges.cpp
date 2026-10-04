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

#include "hudbadges.h"
#include "hudobjectinfo.h"
#include "hudsettings.h"
#include "hudskin.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/textdraw.h"

#include "../structure.h"

#include <unordered_map>

namespace
{

constexpr int BADGE_HEIGHT = 11;
constexpr int BADGE_BOTTOM = 35;
constexpr int BADGE_PADDING = 3;

struct BadgeValues
{
	unsigned queued = 0;
	int loops = 0;
	bool onHold = false;
	bool waiting = false;
	bool hasJob = false;

	bool operator==(const BadgeValues &other) const
	{
		return queued == other.queued && loops == other.loops && onHold == other.onHold
			&& waiting == other.waiting && hasJob == other.hasJob;
	}
};

struct BadgeTexts
{
	BadgeValues values;
	bool initialized = false;
	WzText left;
	WzText right;
	bool hasLeft = false;
	bool hasRight = false;
};

std::unordered_map<uint32_t, BadgeTexts> cache;

void drawBadge(WzText &text, int x, int y, HudTone tone)
{
	auto &skin = currentHudSkin();
	const WzRect rect(x, y, text.width() + BADGE_PADDING * 2, BADGE_HEIGHT);
	skin.drawPill(rect, tone, true);
	const PIELIGHT color = tone == HudTone::Neutral ? skin.colors().textPrimary : skin.toneColor(tone);
	text.render(rect.x() + BADGE_PADDING, rect.y() + (BADGE_HEIGHT - text.lineSize()) / 2 - text.aboveBase(), color);
}

}

void hudDrawJobBadges(STRUCTURE *psStruct, int x, int y, int height)
{
	if (psStruct == nullptr || !hudModuleActive(HudModuleId::ObjectBadges))
	{
		return;
	}
	const auto job = hudStructureJob(psStruct);
	if (!job.has_value() || job->subject == nullptr)
	{
		return;
	}
	BadgeValues values;
	values.hasJob = true;
	values.queued = job->queuedOfSubject;
	values.loops = job->loops;
	values.onHold = job->onHold;
	values.waiting = job->powerNeeded.has_value();

	BadgeTexts &texts = cache[psStruct->id];
	if (!texts.initialized || !(texts.values == values))
	{
		texts.initialized = true;
		texts.values = values;
		std::string left;
		if (values.onHold)
		{
			left = "‖";
		}
		else if (values.waiting)
		{
			left = "⚡";
		}
		std::string right;
		if (values.queued > 0)
		{
			right = astringf("×%u", values.queued);
		}
		if (values.loops != 0)
		{
			right += right.empty() ? "↻" : " ↻";
		}
		texts.hasLeft = !left.empty();
		texts.hasRight = !right.empty();
		texts.left.setText(WzString::fromUtf8(left), font_bar);
		texts.right.setText(WzString::fromUtf8(right), font_bar);
	}

	if (texts.hasLeft)
	{
		drawBadge(texts.left, x + 2, y + std::min(BADGE_BOTTOM, height) - BADGE_HEIGHT, HudTone::Warn);
	}
	if (texts.hasRight)
	{
		drawBadge(texts.right, x + 2, y + 2, HudTone::Warn);
	}
}

void hudReleaseJobBadges()
{
	cache.clear();
}
