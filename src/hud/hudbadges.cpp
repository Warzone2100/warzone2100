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
constexpr int BADGE_ICON = 9;
constexpr int BADGE_ICON_GAP = 2;

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
	HudCountBadge right;
	bool hasLeft = false;
};

std::unordered_map<uint32_t, BadgeTexts> cache;

PIELIGHT badgeColor(HudTone tone)
{
	auto &skin = currentHudSkin();
	return tone == HudTone::Neutral ? skin.colors().textPrimary : skin.toneColor(tone);
}

void drawBadgeText(WzText &text, const WzRect &rect, PIELIGHT color)
{
	text.render(rect.x() + BADGE_PADDING, rect.y() + (BADGE_HEIGHT - text.lineSize()) / 2 - text.aboveBase(), color);
}

void drawBadge(WzText &text, int x, int y, HudTone tone)
{
	const WzRect rect(x, y, text.width() + BADGE_PADDING * 2, BADGE_HEIGHT);
	currentHudSkin().drawPill(rect, tone, true);
	drawBadgeText(text, rect, badgeColor(tone));
}

}

void HudCountBadge::set(const std::string &newText, bool newLoop)
{
	if (newText != text || newLoop != loop)
	{
		text = newText;
		loop = newLoop;
		dirty = true;
	}
}

void HudCountBadge::update()
{
	const bool withIcon = loop && currentHudSkin().drawsIcons();
	if (!dirty && withIcon == renderedWithIcon)
	{
		return;
	}
	dirty = false;
	renderedWithIcon = withIcon;
	std::string shown = text;
	if (loop && !withIcon)
	{
		shown += shown.empty() ? "↻" : " ↻";
	}
	rendered.setText(WzString::fromUtf8(shown), font_bar);
}

int HudCountBadge::width()
{
	update();
	int contents = hasRenderedText() ? rendered.width() : 0;
	if (renderedWithIcon)
	{
		contents += (contents > 0 ? BADGE_ICON_GAP : 0) + BADGE_ICON;
	}
	return contents + BADGE_PADDING * 2;
}

void HudCountBadge::draw(int x, int y)
{
	if (empty())
	{
		return;
	}
	auto &skin = currentHudSkin();
	const WzRect rect(x, y, width(), BADGE_HEIGHT);
	// (over a dark backing, as it's often over a model)
	skin.drawPill(rect, HudTone::Neutral, false);
	skin.drawPill(rect, HudTone::Warn, true);
	const PIELIGHT color = badgeColor(HudTone::Warn);
	int iconX = rect.x() + BADGE_PADDING;
	if (hasRenderedText())
	{
		drawBadgeText(rendered, rect, color);
		iconX += rendered.width() + BADGE_ICON_GAP;
	}
	if (renderedWithIcon)
	{
		skin.drawHudIcon(HudIcon::Loop, WzRect(iconX, rect.y() + (BADGE_HEIGHT - BADGE_ICON) / 2, BADGE_ICON, BADGE_ICON), color);
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
		texts.hasLeft = !left.empty();
		texts.left.setText(WzString::fromUtf8(left), font_bar);
		texts.right.set(values.queued > 0 ? astringf("×%u", values.queued) : std::string(), values.loops != 0);
	}

	if (texts.hasLeft)
	{
		drawBadge(texts.left, x + 2, y + std::min(BADGE_BOTTOM, height) - BADGE_HEIGHT, HudTone::Warn);
	}
	texts.right.draw(x + 2, y + 2);
}

void hudReleaseJobBadges()
{
	cache.clear();
}
