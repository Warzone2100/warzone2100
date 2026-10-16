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

#include "hudrightcolumn.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/widget/widget.h"

#include "../hci.h"
#include "../radar.h"

#include <algorithm>
#include <array>
#include <tuple>

constexpr int GAP = 10;
constexpr int FALLBACK_RIGHT_MARGIN = 20;
constexpr int FALLBACK_TOP = 58;

static std::array<std::weak_ptr<WIDGET>, static_cast<size_t>(HudRightColumnSlot::Count)> slots;

void hudRightColumnSet(HudRightColumnSlot slot, const std::shared_ptr<WIDGET> &widget)
{
	slots[static_cast<size_t>(slot)] = widget;
}

/// The column's right edge and top: under the options button's card (a square centered in the button), right-aligned
/// with it
static std::pair<int, int> columnOrigin()
{
	int right = static_cast<int>(screenWidth) - FALLBACK_RIGHT_MARGIN;
	int top = FALLBACK_TOP;
	if (WIDGET *options = psWScreen ? widgGetFromID(psWScreen, IDRET_OPTIONS) : nullptr)
	{
		if (options->visible())
		{
			const int side = std::min(options->width(), options->height());
			right = options->screenPosX() + (options->width() + side) / 2;
			top = options->screenPosY() + options->height() + GAP;
		}
	}
	return {right, top};
}

void hudLayoutRightColumn()
{
	int right;
	int top;
	std::tie(right, top) = columnOrigin();
	for (const auto &slot : slots)
	{
		auto widget = slot.lock();
		if (!widget || !widget->visible())
		{
			continue;
		}
		widget->setGeometry(right - widget->width(), top, widget->width(), widget->height());
		top += widget->height() + GAP;
	}
}

int hudRightColumnAvailableHeight(HudRightColumnSlot slot)
{
	int bottom = static_cast<int>(screenHeight);
	auto radar = getRadarWidget();
	if (radar && radar->visible())
	{
		bottom = radar->screenPosY();
	}
	int availableHeight = bottom - GAP - columnOrigin().second;
	for (size_t i = 0; i < slots.size(); ++i)
	{
		auto widget = slots[i].lock();
		if (i != static_cast<size_t>(slot) && widget && widget->visible())
		{
			availableHeight -= widget->height() + GAP;
		}
	}
	return std::max(0, availableHeight);
}
