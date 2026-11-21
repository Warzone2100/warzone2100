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

#include "hudskin.h"
#include "hudclassicskin.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/widget/bar.h"

#include "../intdisplay.h"
#include "../intimage.h"
#include "../main.h"

#include <algorithm>

/// The research panel's View Tree button's colors, which go with the Classic images' buttons
const PIELIGHT BUTTON_FILL = pal_RGBA(47, 51, 87, 255);
const PIELIGHT BUTTON_FILL_OVER = pal_RGBA(72, 78, 125, 255);
const PIELIGHT BUTTON_BORDER = pal_RGBA(89, 90, 144, 255);
constexpr float BUTTON_RADIUS = 2.f;

static void drawButtonBox(const WzRect &rect, PIELIGHT fill, PIELIGHT border)
{
	pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
	                   fill, BUTTON_RADIUS, border, 1.f);
}

const HudColors &ClassicHudSkin::colors() const
{
	static const HudColors classicColors = {
		WZCOL_TRANSPARENT_BOX,
		WZCOL_TRANSPARENT_BOX,
		BUTTON_BORDER,
		WZCOL_FORM_HILITE,
		WZCOL_TEXT_BRIGHT,
		WZCOL_TEXT_MEDIUM,
		WZCOL_GREEN,
		WZCOL_YELLOW,
		WZCOL_RED,
		WZCOL_FORM_DISABLE,
		WZCOL_YELLOW,
		WZCOL_YELLOW,
		WZCOL_GREEN,
		WZCOL_FORM_HILITE,
		WZCOL_MENU_SHADOW,
	};
	return classicColors;
}

const HudMetrics &ClassicHudSkin::metrics() const
{
	static const HudMetrics classicMetrics = {0.f, 0.f, 1.f, 4, font_regular_bold, font_small, font_small};
	return classicMetrics;
}

void ClassicHudSkin::drawPanelFrame(const WzRect &rect, const WzRect &, bool)
{
	RenderWindowFrame(FRAME_NORMAL, rect.x(), rect.y(), rect.width(), rect.height());
}

void ClassicHudSkin::drawFancyButtonBackground(const HudFancyButtonLook &look)
{
	UWORD buttonId = 0;
	if (!look.bottomButton)
	{
		if (look.emptyBackground)
		{
			buttonId = (look.down ? IMAGE_BUT_EMPTY_DOWN : IMAGE_BUT_EMPTY_UP);
		}
		else
		{
			buttonId = (look.down ? IMAGE_BUT0_DOWN : IMAGE_BUT0_UP);
		}
	}
	else
	{
		buttonId = (look.down ? IMAGE_BUTB0_DOWN : IMAGE_BUTB0_UP);
	}
	iV_DrawImage(IntImages, buttonId, look.x, look.y);
}

void ClassicHudSkin::drawFancyButtonHighlight(const HudFancyButtonLook &look)
{
	iV_DrawImage(IntImages, look.bottomButton ? IMAGE_BUTB_HILITE : IMAGE_BUT_HILITE, look.x, look.y);
}

void ClassicHudSkin::drawFancyButtonOnHold(const HudFancyButtonLook &look)
{
	iV_DrawImage(IntImages, ((realTime / 250) % 2) == 0 ? IMAGE_BUT0_DOWN : IMAGE_BUT_HILITE, look.x, look.y);
}

void ClassicHudSkin::drawReticuleButton(const HudReticuleButtonLook &look)
{
	const int x = look.x;
	const int y = look.y;
	const int w = look.width;
	const int h = look.height;
	if (look.disabled)
	{
		if (!look.isCancel)
		{
			iV_DrawImage2("image_reticule_grey.png", x, y, w, h);
		}
		else
		{
			iV_DrawImage2(look.downImage, x, y, w, h);
		}
		return;
	}

	if (look.pressed)
	{
		if (look.firstPressedFrame && !look.isCancel)
		{
			iV_DrawImage2("image_reticule_butdown.png", x, y, w, h);
		}
		else
		{
			iV_DrawImage2(look.downImage, x, y, w, h);
		}
	}
	else if (look.flashing && !look.paused)
	{
		iV_DrawImage2(((realTime / 250) % 2) != 0 ? look.downImage : look.upImage, x, y, w, h);
	}
	else
	{
		iV_DrawImage2(look.upImage, x, y, w, h);
	}

	if (look.highlighted)
	{
		iV_DrawImage2(look.isCancel ? "image_cancel_hilight.png" : "image_reticule_hilight.png", x - 1, y - 1, w + 2, h + 2);
	}
}

void ClassicHudSkin::drawPowerBar(PowerBar &bar, int xOffset, int yOffset)
{
	bar.displayClassic(xOffset, yOffset);
}

void ClassicHudSkin::drawButtonBar(W_BARGRAPH &bar, int xOffset, int yOffset)
{
	bar.W_BARGRAPH::display(xOffset, yOffset);
}

void ClassicHudSkin::drawCloseButton(int x, int y, int width, int height, bool highlighted)
{
	// (in the middle of a larger button: Larger Touch Targets)
	x += std::max(0, (width - static_cast<int>(iV_GetImageWidth(IntImages, IMAGE_CLOSE))) / 2);
	y += std::max(0, (height - static_cast<int>(iV_GetImageHeight(IntImages, IMAGE_CLOSE))) / 2);
	iV_DrawImage(IntImages, IMAGE_CLOSE, x, y);
	if (highlighted)
	{
		iV_DrawImage(IntImages, IMAGE_CLOSEHILIGHT, x, y);
	}
}

void ClassicHudSkin::drawTimerBackground(int x, int y, uint16_t image, bool flashOn, uint16_t flashImage)
{
	iV_DrawImage(IntImages, image, x, y);
	if (flashOn)
	{
		iV_DrawImage(IntImages, flashImage, x, y);
	}
}

nonstd::optional<WzTipStyle> ClassicHudSkin::tooltipStyle() const
{
	return nonstd::nullopt;
}

void ClassicHudSkin::drawCard(const WzRect &rect, HudCardState state)
{
	switch (state)
	{
	case HudCardState::Normal: drawButtonBox(rect, BUTTON_FILL, BUTTON_BORDER); break;
	case HudCardState::Highlighted: drawButtonBox(rect, BUTTON_FILL_OVER, WZCOL_TEXT_BRIGHT); break;
	case HudCardState::Selected: drawButtonBox(rect, WZCOL_MENU_SCORE_BUILT, WZCOL_TEXT_BRIGHT); break;
	case HudCardState::Disabled: drawButtonBox(rect, BUTTON_FILL, WZCOL_FORM_DISABLE); break;
	}
}

// (Filled when hovered, or in the accent when chosen, like the View Tree button when hovered or pressed. The other
// tones keep their color on the border)
void ClassicHudSkin::drawPill(const WzRect &rect, HudTone tone, bool filled)
{
	switch (tone)
	{
	case HudTone::Neutral: drawButtonBox(rect, filled ? BUTTON_FILL_OVER : BUTTON_FILL, filled ? WZCOL_TEXT_BRIGHT : BUTTON_BORDER); break;
	case HudTone::Accent: drawButtonBox(rect, filled ? WZCOL_MENU_SCORE_BUILT : BUTTON_FILL, filled ? WZCOL_TEXT_BRIGHT : BUTTON_BORDER); break;
	default: drawButtonBox(rect, filled ? BUTTON_FILL_OVER : BUTTON_FILL, toneColor(tone)); break;
	}
}

void ClassicHudSkin::drawProgress(const WzRect &rect, float fraction, HudProgressKind kind)
{
	const float clamped = std::max(0.f, std::min(fraction, 1.f));
	pie_BoxFill(rect.x(), rect.y(), rect.right(), rect.bottom(), WZCOL_BLACK);
	const PIELIGHT color = kind == HudProgressKind::PowerWait ? WZCOL_GREEN : WZCOL_YELLOW;
	pie_BoxFill(rect.x(), rect.y(), rect.x() + static_cast<int>(rect.width() * clamped), rect.bottom(), color);
}

void ClassicHudSkin::drawPopoverFrame(const WzRect &rect)
{
	iV_ShadowBox(rect.x(), rect.y(), rect.right(), rect.bottom(), 1, WZCOL_FORM_LIGHT, WZCOL_FORM_DARK, WZCOL_FORM_TIP_BACKGROUND);
}

void ClassicHudSkin::drawInputField(const WzRect &rect, bool editing)
{
	drawButtonBox(rect, WZCOL_TRANSPARENT_BOX, editing ? WZCOL_TEXT_BRIGHT : BUTTON_BORDER);
}

// (Barely rounded boxes: their borders and fills line up on the pixels, which the Classic boxes' lines don't)
void ClassicHudSkin::drawPageControlFrame(const WzRect &rect)
{
	pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
	                   WZCOL_TRANSPARENT_BOX, 1.5f, WZCOL_FORM_DARK, 1.f);
}

nonstd::optional<PIELIGHT> ClassicHudSkin::drawPageSegment(const HudPageSegmentLook &look)
{
	const float x0 = static_cast<float>(look.rect.x());
	const float y0 = static_cast<float>(look.rect.y());
	const float x1 = static_cast<float>(look.rect.right());
	const float y1 = static_cast<float>(look.rect.bottom());
	const PIELIGHT none = pal_RGBA(0, 0, 0, 0);
	const bool hovered = look.hovered && !look.disabled;
	if (look.large)
	{
		pie_DrawRoundedBox(x0, y0, x1, y1, look.selected ? WZCOL_FORM_HILITE : (hovered ? BUTTON_FILL_OVER : WZCOL_TRANSPARENT_BOX), 1.5f, WZCOL_FORM_DARK, 1.f);
	}
	else if (look.role == TabSelectionButtonRole::Tab && (look.selected || hovered))
	{
		pie_DrawRoundedBox(x0 + 1.f, y0 + 2.f, x1 - 1.f, y1 - 2.f, look.selected ? WZCOL_FORM_HILITE : BUTTON_FILL_OVER, 1.f, none, 0.f);
	}
	if (look.separator && !look.large)
	{
		pie_BoxFill(look.rect.x() - 1, look.rect.y() + 3, look.rect.x(), look.rect.bottom() - 3, WZCOL_FORM_DARK);
	}
	if (look.disabled)
	{
		return WZCOL_FORM_DISABLE;
	}
	return look.selected || look.hovered ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
}

PIELIGHT ClassicHudSkin::toneColor(HudTone tone) const
{
	switch (tone)
	{
	case HudTone::Neutral: return WZCOL_FORM_LIGHT;
	case HudTone::Accent: return WZCOL_FORM_HILITE;
	case HudTone::Ok: return WZCOL_GREEN;
	case HudTone::Warn: return WZCOL_YELLOW;
	case HudTone::Bad: return WZCOL_RED;
	}
	return WZCOL_FORM_LIGHT;
}

void ClassicHudSkin::drawOverlayText(WzText &text, float x, float y, PIELIGHT color)
{
	if (hudSettings().textOutline)
	{
		// (Fading with the text)
		PIELIGHT outline = WZCOL_BLACK;
		outline.byte.a = static_cast<uint8_t>(outline.byte.a * color.byte.a / 255);
		text.renderOutlined(static_cast<int>(x), static_cast<int>(y), color, outline);
	}
	else
	{
		text.render(x, y, color);
	}
}

HudSkin &classicHudSkin()
{
	static ClassicHudSkin skin;
	return skin;
}

HudSkin &hudSkinForSetting()
{
	return hudSettings().skin == HudSkinId::Modern ? modernHudSkin() : classicHudSkin();
}

HudSkin &currentHudSkin()
{
	if (hudSettings().skin == HudSkinId::Modern && GetGameMode() == GS_NORMAL)
	{
		return modernHudSkin();
	}
	return classicHudSkin();
}
