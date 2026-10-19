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

#include "huddesign.h"
#include "hudskin.h"

#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/editbox.h"
#include "lib/widget/widget.h"

#include "../design.h"
#include "../hci.h"
#include "../intdisplay.h"
#include "../intimage.h"
#include "../vectorimageloader.h"

#include <algorithm>
#include <cmath>

namespace
{

const PIELIGHT ICON_COLOR = pal_RGBA(188, 198, 212, 255);
const PIELIGHT WELL = pal_RGBA(6, 9, 14, 225);
const PIELIGHT WELL_BORDER = pal_RGBA(84, 102, 128, 128);
const PIELIGHT WELL_BORDER_HOVER = pal_RGBA(140, 170, 210, 190);
const PIELIGHT FIELD = pal_RGBA(10, 14, 20, 215);
const PIELIGHT LIGHT = pal_RGBA(34, 50, 72, 150);
const PIELIGHT TRACK = pal_RGBA(255, 255, 255, 18);
constexpr float RADIUS = 3.f;
constexpr int STAT_ICON_SIZE = 13;
constexpr float BAR_HEIGHT = 5.f;
/// The part silhouettes (40×20) in a part button
constexpr int PART_ICON_WIDTH = 32;
constexpr int PART_ICON_HEIGHT = 16;
constexpr int BUTTON_ICON_SIZE = 14;
constexpr int SLOT_DOT = 4;

PIELIGHT alpha(PIELIGHT color, uint8_t value)
{
	color.byte.a = value;
	return color;
}

std::string iconPath(const char *name)
{
	return std::string("images/hud/icons/") + name + ".svg";
}

const char *buttonIcon(UDWORD id, bool &wide)
{
	wide = true;
	switch (id)
	{
	case IDDES_BODYBUTTON: return "design-body";
	case IDDES_PROPBUTTON: return "design-propulsion";
	case IDDES_SYSTEMBUTTON:
	case IDDES_WPABUTTON:
	case IDDES_WPBBUTTON: return "design-turret";
	default: break;
	}
	wide = false;
	switch (id)
	{
	case IDDES_STOREBUTTON: return "store";
	case IDDES_BIN: return "delete";
	case IDDES_WEAPONS: return "research-weapon";
	case IDDES_SYSTEMS: return "research-sensor";
	default: return nullptr;
	}
}

struct StatRow
{
	UDWORD label;
	UDWORD bar;
	const char *icon;
};

const StatRow STAT_ROWS[] = {
	{IDDES_BODYARMOURKLAB, IDDES_BODYARMOUR_K, "armor-kinetic"},
	{IDDES_BODYARMOURHLAB, IDDES_BODYARMOUR_H, "armor-thermal"},
	{IDDES_BODYPOWERLAB, IDDES_BODYPOWER, "engine"},
	{IDDES_BODYWEIGHTLAB, IDDES_BODYWEIGHT, "weight"},
	{IDDES_PROPROADLAB, IDDES_PROPROAD, "speed-road"},
	{IDDES_PROPCOUNTRYLAB, IDDES_PROPCOUNTRY, "speed-offroad"},
	{IDDES_PROPWATERLAB, IDDES_PROPWATER, "speed-water"},
	{IDDES_PROPAIRLAB, IDDES_PROPAIR, "speed-air"},
	{IDDES_PROPWEIGHTLAB, IDDES_PROPWEIGHT, "weight"},
	{IDDES_SENSORRANGELAB, IDDES_SENSORRANGE, "sensor-range"},
	{IDDES_ECMPOWERLAB, IDDES_ECMPOWER, "jamming-range"},
	{IDDES_CONSTPOINTSLAB, IDDES_CONSTPOINTS, "build-points"},
	{IDDES_REPAIRPTLAB, IDDES_REPAIRPOINTS, "repair"},
	{IDDES_WEAPRANGELAB, IDDES_WEAPRANGE, "weapon-range"},
	{IDDES_WEAPDAMAGELAB, IDDES_WEAPDAMAGE, "damage"},
	{IDDES_WEAPROFLAB, IDDES_WEAPROF, "rate-of-fire"},
	{IDDES_SYSTEMSWEIGHTLAB, IDDES_SYSTEMSWEIGHT, "weight"},
	{IDDES_REPAIRWGTLAB, IDDES_SYSTEMSWEIGHT, "weight"},
	{IDDES_TEMPPOWERLAB, IDDES_POWERBAR, "power"},
	{IDDES_TEMPBODYLAB, IDDES_BODYPOINTS, "hit-points"},
};

const StatRow *statRow(UDWORD label)
{
	for (const StatRow &row : STAT_ROWS)
	{
		if (row.label == label)
		{
			return &row;
		}
	}
	return nullptr;
}

/// The weapon slot's number, once the body has more than one
int slotNumber(UDWORD id)
{
	WIDGET *second = widgGetFromID(psWScreen, IDDES_WPABUTTON);
	if (second == nullptr || !second->visible())
	{
		return 0;
	}
	switch (id)
	{
	case IDDES_SYSTEMBUTTON: return 1;
	case IDDES_WPABUTTON: return 2;
	case IDDES_WPBBUTTON: return 3;
	default: return 0;
	}
}

bool drawIcon(const char *name, float x, float y, float width, float height, PIELIGHT color)
{
	return drawGameVectorImage(iconPath(name), std::round(x), std::round(y), width, height, color);
}

bool drawButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, bool missing)
{
	if (!hudDrawsDesignScreen())
	{
		return false;
	}
	bool wide = false;
	const char *icon = buttonIcon(psWidget->id, wide);
	if (icon == nullptr || !loadGameVectorImage(iconPath(icon)))
	{
		return false;
	}
	HudSkin &skin = currentHudSkin();
	const HudColors &colors = skin.colors();
	const WzRect rect(xOffset + psWidget->x(), yOffset + psWidget->y(), psWidget->width(), psWidget->height());
	const unsigned state = psWidget->getState();
	const bool disabled = (state & WBUT_DISABLE) != 0;
	const bool highlighted = !disabled && (state & WBUT_HIGHLIGHT) != 0;
	// (Store shows the delete image while the design is stored: lit, as a toggle that's on)
	const bool on = (state & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0
		|| (psWidget->id == IDDES_STOREBUTTON && UNPACKDWORD_TRI_C(psWidget->UserData) == IMAGE_DES_DELETE);
	HudCardState card = HudCardState::Normal;
	if (disabled)
	{
		card = HudCardState::Disabled;
	}
	else if (on)
	{
		card = HudCardState::Selected;
	}
	else if (highlighted)
	{
		card = HudCardState::Highlighted;
	}
	skin.drawCard(rect, card);
	if (missing && !on)
	{
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()),
		                   static_cast<float>(rect.bottom()), pal_RGBA(0, 0, 0, 0), skin.metrics().cardRadius, colors.stateWarn, 1.f);
		pie_DrawRoundedBox(rect.x() + 3.f, rect.y() + 3.f, rect.x() + 3.f + SLOT_DOT, rect.y() + 3.f + SLOT_DOT, colors.stateWarn, SLOT_DOT / 2.f);
	}

	PIELIGHT color = (on || highlighted) ? colors.textPrimary : ICON_COLOR;
	if (psWidget->id == IDDES_BIN && highlighted)
	{
		color = colors.stateBad;
	}
	if (disabled)
	{
		color = colors.stateDisabled;
	}
	const float width = static_cast<float>(wide ? PART_ICON_WIDTH : BUTTON_ICON_SIZE);
	const float height = static_cast<float>(wide ? PART_ICON_HEIGHT : BUTTON_ICON_SIZE);
	drawIcon(icon, rect.x() + (rect.width() - width) / 2.f, rect.y() + (rect.height() - height) / 2.f, width, height, color);

	if (const int number = slotNumber(psWidget->id))
	{
		static WzText numbers[3];
		WzText &text = numbers[number - 1];
		text.setText(WzString::number(number), font_small);
		text.render(rect.right() - text.width() - 3, rect.y() + 2 - text.aboveBase(), on ? colors.textPrimary : colors.textSecondary);
	}
	return true;
}

}

bool hudDrawsDesignScreen()
{
	return currentHudSkin().drawsDesignScreen();
}

void hudDesignDisplayButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	if (!drawButton(psWidget, xOffset, yOffset, false))
	{
		intDisplayButtonHilight(psWidget, xOffset, yOffset);
	}
}

void hudDesignDisplayButtonFlash(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	if (!drawButton(psWidget, xOffset, yOffset, true))
	{
		intDisplayButtonFlash(psWidget, xOffset, yOffset);
	}
}

void hudDesignDisplayStatIcon(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	const StatRow *row = hudDrawsDesignScreen() ? statRow(psWidget->id) : nullptr;
	if (row == nullptr || !loadGameVectorImage(iconPath(row->icon)))
	{
		intDisplayImage(psWidget, xOffset, yOffset);
		return;
	}
	HudSkin &skin = currentHudSkin();
	const PIELIGHT color = psWidget->id == IDDES_TEMPPOWERLAB ? skin.toneColor(HudTone::Warn) : ICON_COLOR;
	// Level with its bar (a sibling), which the label isn't
	float centerY = psWidget->y() + psWidget->height() / 2.f;
	WIDGET *bar = widgGetFromID(psWScreen, row->bar);
	if (bar != nullptr && bar->parent() == psWidget->parent())
	{
		centerY = bar->y() + bar->height() / 2.f;
	}
	const float x = xOffset + psWidget->x() + (psWidget->width() - STAT_ICON_SIZE) / 2.f;
	const float y = yOffset + centerY - STAT_ICON_SIZE / 2.f;
	drawIcon(row->icon, x, y, STAT_ICON_SIZE, STAT_ICON_SIZE, color);
}

void hudDesignDisplayEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	if (!hudDrawsDesignScreen())
	{
		intDisplayEditBox(psWidget, xOffset, yOffset);
		return;
	}
	const unsigned state = static_cast<W_EDITBOX *>(psWidget)->state;
	const WzRect rect(xOffset + psWidget->x(), yOffset + psWidget->y(), psWidget->width(), psWidget->height());
	currentHudSkin().drawInputField(rect, (state & WEDBS_MASK) != WEDBS_FIXED || (state & WEDBS_HILITE) != 0);
}

bool hudDesignDrawForm(const WIDGET &form, int x, int y)
{
	if (!hudDrawsDesignScreen())
	{
		return false;
	}
	if (form.id == IDDES_POWERFORM)
	{
		pie_DrawRoundedBox(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + form.width()),
		                   static_cast<float>(y + form.height()), FIELD, RADIUS, WELL_BORDER, 1.f);
	}
	return true;
}

bool hudDesignDrawView(int x, int y, int width, int height)
{
	if (!hudDrawsDesignScreen())
	{
		return false;
	}
	pie_DrawRoundedBox(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + width), static_cast<float>(y + height),
	                   WELL, RADIUS, WELL_BORDER, 1.f);
	// (under the droid, which is drawn a quarter of the way down and 32 px lower)
	const float cx = x + width / 2.f;
	const float cy = y + height * 0.6f;
	const float rx = width * 0.36f;
	const float ry = height * 0.26f;
	pie_DrawSoftRoundedBox(cx - rx, cy - ry, cx + rx, cy + ry, LIGHT, ry, ry);
	return true;
}

bool hudDesignDrawStatForm(int x, int y, int width, int height, bool highlighted)
{
	if (!hudDrawsDesignScreen())
	{
		return false;
	}
	// (the part's picture, left of the stat icons)
	const int pictureWidth = std::min(width, width * 2 / 5);
	pie_DrawRoundedBox(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + pictureWidth), static_cast<float>(y + height),
	                   WELL, RADIUS, highlighted ? WELL_BORDER_HOVER : WELL_BORDER, 1.f);
	return true;
}

HudDesignBarMetrics hudDesignBarMetrics()
{
	HudDesignBarMetrics metrics;
	metrics.leftGutter = iV_GetTextWidth("00000", font_small) + 4;
	metrics.rightGutter = iV_GetTextWidth("+0000", font_small) + 4;
	return metrics;
}

bool hudDesignDrawStatBar(const HudDesignBarLook &look, WzText &valueText, WzText &deltaText)
{
	if (!hudDrawsDesignScreen())
	{
		return false;
	}
	HudSkin &skin = currentHudSkin();
	const HudColors &colors = skin.colors();
	const float cy = look.y + look.height / 2.f;

	valueText.setText(WzString::fromUtf8(look.value), font_small);
	valueText.render(look.x, static_cast<int>(std::round(cy - valueText.lineSize() / 2.f)) - valueText.aboveBase(), colors.textPrimary);

	const float x0 = static_cast<float>(look.x + look.metrics.leftGutter);
	const float x1 = static_cast<float>(look.x + look.width - look.metrics.rightGutter);
	if (x1 <= x0)
	{
		return true;
	}
	const float y0 = std::round(cy - BAR_HEIGHT / 2.f);
	const float y1 = y0 + BAR_HEIGHT;
	const float r = BAR_HEIGHT / 2.f;
	pie_DrawRoundedBox(x0, y0, x1, y1, TRACK, r);
	auto at = [&](float fraction) { return x0 + (x1 - x0) * std::max(0.f, std::min(1.f, fraction)); };
	const float valueX = at(look.fraction);
	if (look.compared < 0.f)
	{
		if (valueX > x0)
		{
			pie_DrawRoundedBox(x0, y0, std::max(valueX, x0 + BAR_HEIGHT), y1, colors.accent, r);
		}
		return true;
	}

	const PIELIGHT tone = look.better ? colors.stateOk : colors.stateBad;
	const float comparedX = at(look.compared);
	const float low = std::min(valueX, comparedX);
	const float high = std::max(valueX, comparedX);
	if (high > x0)
	{
		// The change, from the low end to the high: solid where the value grows, striped where it drops
		if (comparedX >= valueX)
		{
			pie_DrawRoundedBox(x0, y0, std::max(high, x0 + BAR_HEIGHT), y1, tone, r);
		}
		else
		{
			pie_DrawRoundedBox(x0, y0, std::max(high, x0 + BAR_HEIGHT), y1, alpha(tone, 70), r);
			for (float sx = std::floor(low) + 1.f; sx < high - 1.f; sx += 3.f)
			{
				pie_UniTransBoxFill(sx, y0, sx + 1.f, y1, tone);
			}
		}
		if (low > x0)
		{
			pie_DrawRoundedBox(x0, y0, std::max(low, x0 + r), y1, colors.accent, glm::vec4(r, 0.f, r, 0.f), pal_RGBA(0, 0, 0, 0), 0.f);
		}
	}
	if (!look.delta.empty())
	{
		deltaText.setText(WzString::fromUtf8(look.delta), font_small);
		deltaText.render(look.x + look.width - deltaText.width(), static_cast<int>(std::round(cy - deltaText.lineSize() / 2.f)) - deltaText.aboveBase(), tone);
	}
	return true;
}
