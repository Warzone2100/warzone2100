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
/** @file
 *  The Modern HUD skin: shapes drawn with the rounded-box primitives instead of frame images.
 *  The script-supplied reticule images are kept, and only their states are drawn differently.
 *  Colors are display-referred, like every other UI color.
 */

#include "hudclassicskin.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/widget/bar.h"

#include "../hci.h"
#include "../intdisplay.h"
#include "../intimage.h"

#include <cmath>
#include <unordered_map>

namespace
{

constexpr float PI = 3.14159265f;

PIELIGHT withAlpha(PIELIGHT color, int alpha)
{
	color.byte.a = static_cast<uint8_t>(std::max(0, std::min(alpha, 255)));
	return color;
}

PIELIGHT scaledAlpha(PIELIGHT color, float factor)
{
	return withAlpha(color, static_cast<int>(color.byte.a * factor));
}

bool sameColor(PIELIGHT a, PIELIGHT b)
{
	return a.rgba() == b.rgba();
}

/// IntImages entries carry a drawing offset, meant for their place on the old panels
void drawIcon(uint16_t id, float x, float y, float height, PIELIGHT color)
{
	const float scale = height / static_cast<float>(iV_GetImageHeight(IntImages, id));
	iV_DrawImageTint(IntImages, id, x - iV_GetImageXOffset(IntImages, id), y - iV_GetImageYOffset(IntImages, id), color,
	                 Vector2f(static_cast<float>(iV_GetImageWidth(IntImages, id)) * scale, height));
}

// The bottom (object) button's card sits under the arrow up to its job, where the Classic image's body is
constexpr int BOTTOM_ARROW_TOP = OBJ_BUTHEIGHT - OBJ_STARTY;
constexpr int BOTTOM_CARD_TOP = 8;
constexpr int BOTTOM_CARD_HEIGHT = 42;

WzRect cardRect(const HudFancyButtonLook &look)
{
	if (!look.bottomButton)
	{
		return WzRect(look.x, look.y, look.width, look.height);
	}
	return WzRect(look.x, look.y + BOTTOM_CARD_TOP, look.width, BOTTOM_CARD_HEIGHT);
}

float clamp01(float v)
{
	return std::max(0.f, std::min(v, 1.f));
}

PIELIGHT mixColors(PIELIGHT from, PIELIGHT to, float amount)
{
	auto channel = [amount](uint8_t a, uint8_t b) {
		return static_cast<uint8_t>(std::lround(a + (b - a) * clamp01(amount)));
	};
	return pal_RGBA(channel(from.byte.r, to.byte.r), channel(from.byte.g, to.byte.g), channel(from.byte.b, to.byte.b), channel(from.byte.a, to.byte.a));
}

class ModernHudSkin : public ClassicHudSkin
{
public:
	HudSkinId id() const override { return HudSkinId::Modern; }

	const HudColors &colors() const override { return palette; }
	const HudMetrics &metrics() const override { return sizes; }

	void drawPanelFrame(const WzRect &rect, const WzRect &openRect) override
	{
		if (rect.width() <= 0 || rect.height() <= 0)
		{
			return;
		}
		if (hudReduceMotion() && rect != openRect)
		{
			return;
		}
		const float x0 = static_cast<float>(rect.x());
		const float y0 = static_cast<float>(rect.y());
		const float x1 = static_cast<float>(rect.right());
		const float y1 = static_cast<float>(rect.bottom());
		const float radius = std::min(params.panelRadius, std::min(rect.width(), rect.height()) / 2.f);
		if (params.shadowSpread > 0.f)
		{
			const float spread = params.shadowSpread;
			pie_DrawSoftRoundedBox(x0 - spread, y0 - spread + 2.f, x1 + spread, y1 + spread + 2.f,
			                       palette.shadow, radius + spread, spread);
		}
		pie_DrawGradientRoundedBox(x0, y0, x1, y1, palette.panelBackground, palette.panelBackgroundBottom, false, 1.f,
		                           radius, palette.panelBorder, params.borderWidth);
		if (rect.height() > radius * 2.f + 4.f)
		{
			pie_DrawRoundedBox(x0 + 1.f, y0 + 1.f, x1 - 1.f, y0 + 1.f + params.accentEdge,
			                   withAlpha(palette.accent, params.accentEdgeAlpha), glm::vec4(radius - 1.f, radius - 1.f, 0.f, 0.f),
			                   pal_RGBA(0, 0, 0, 0), 0.f);
		}
	}

	void drawFancyButtonBackground(const HudFancyButtonLook &look) override
	{
		const WzRect card = cardRect(look);
		const float x0 = static_cast<float>(card.x());
		const float y0 = static_cast<float>(card.y());
		const float x1 = static_cast<float>(card.right());
		const float y1 = static_cast<float>(card.bottom());
		PIELIGHT top = cardTop;
		PIELIGHT bottom = cardBottom;
		PIELIGHT border = cardBorder;
		if (look.down)
		{
			top = cardDownTop;
			bottom = cardDownBottom;
			border = withAlpha(palette.accent, 220);
		}
		else if (look.hovered)
		{
			top = cardHoverTop;
			border = cardHoverBorder;
		}
		if (look.emptyBackground)
		{
			top = scaledAlpha(top, 0.6f);
			bottom = scaledAlpha(bottom, 0.6f);
		}
		pie_DrawGradientRoundedBox(x0, y0, x1, y1, top, bottom, false, 1.f, params.cardRadius, border, params.borderWidth);
		if (look.bottomButton)
		{
			// The Classic image's arrow up to the job, in the gap between the two cards
			const float cx = (x0 + x1) / 2.f;
			const float cy = y0 - static_cast<float>(BOTTOM_CARD_TOP - BOTTOM_ARROW_TOP) / 2.f;
			const PIELIGHT mark = look.down || look.hovered ? border : withAlpha(palette.textSecondary, 170);
			pie_DrawSoftRoundedBoxRotated(cx - 1.6f, cy, 4.6f, 1.6f, -PI / 4.f, mark, 0.8f, 0.8f);
			pie_DrawSoftRoundedBoxRotated(cx + 1.6f, cy, 4.6f, 1.6f, PI / 4.f, mark, 0.8f, 0.8f);
		}
	}

	void drawFancyButtonHighlight(const HudFancyButtonLook &look) override
	{
		const WzRect card = cardRect(look);
		pie_DrawRoundedBox(static_cast<float>(card.x()), static_cast<float>(card.y()), static_cast<float>(card.right()), static_cast<float>(card.bottom()),
		                   withAlpha(palette.accent, 34), params.cardRadius, withAlpha(palette.accent, 235), 1.5f);
	}

	void drawFancyButtonOnHold(const HudFancyButtonLook &look) override
	{
		const WzRect card = cardRect(look);
		const float x0 = static_cast<float>(card.x());
		const float y0 = static_cast<float>(card.y());
		const float pulse = hudReduceMotion() ? 1.f : 0.65f + 0.35f * (0.5f + 0.5f * std::cos(2.f * PI * (realTime % 1600) / 1600.f));
		pie_DrawRoundedBox(x0, y0, x0 + card.width(), y0 + card.height(), withAlpha(palette.stateWarn, static_cast<int>(28 * pulse)),
		                   params.cardRadius, withAlpha(palette.stateWarn, static_cast<int>(230 * pulse)), 1.5f);
		const float badge = 11.f;
		const float bx = x0 + card.width() - badge - 3.f;
		const float by = y0 + 3.f;
		pie_DrawRoundedBox(bx, by, bx + badge, by + badge, withAlpha(palette.stateWarn, 235), badge / 2.f);
		pie_DrawRoundedBox(bx + 3.f, by + 3.f, bx + 4.8f, by + badge - 3.f, pal_RGBA(20, 16, 6, 255), 0.6f);
		pie_DrawRoundedBox(bx + badge - 4.8f, by + 3.f, bx + badge - 3.f, by + badge - 3.f, pal_RGBA(20, 16, 6, 255), 0.6f);
	}

	bool drawFancyButtonEmpty(const HudFancyButtonLook &look) override
	{
		// An empty slot: a dashed outline and a "+", as the button opens the list to choose from
		const WzRect card = cardRect(look);
		const float inset = 1.f;
		const float x0 = static_cast<float>(card.x()) + inset;
		const float y0 = static_cast<float>(card.y()) + inset;
		const float x1 = static_cast<float>(card.right()) - inset;
		const float y1 = static_cast<float>(card.bottom()) - inset;
		const bool lit = look.hovered || look.down;
		const PIELIGHT outline = look.down ? withAlpha(palette.accent, 230) : (look.hovered ? cardHoverBorder : withAlpha(cardBorder, 200));
		pie_DrawDashedRoundedBox(x0, y0, x1, y1, withAlpha(palette.accent, lit ? 30 : 10), params.cardRadius, outline, params.borderWidth, 3.f, 2.5f);
		if (params.idleJobDot)
		{
			const float dot = 2.6f;
			pie_DrawRoundedBox(x0 + 5.5f - dot, y0 + 5.5f - dot, x0 + 5.5f + dot, y0 + 5.5f + dot, palette.stateWarn, dot);
		}
		const float cx = (x0 + x1) / 2.f;
		const float cy = (y0 + y1) / 2.f;
		const float arm = 6.f;
		const float thickness = 2.f;
		const PIELIGHT mark = lit ? palette.accent : withAlpha(palette.textSecondary, 180);
		pie_DrawRoundedBox(cx - arm, cy - thickness / 2.f, cx + arm, cy + thickness / 2.f, mark, thickness / 2.f);
		pie_DrawRoundedBox(cx - thickness / 2.f, cy - arm, cx + thickness / 2.f, cy - thickness / 2.f, mark, thickness / 2.f);
		pie_DrawRoundedBox(cx - thickness / 2.f, cy + thickness / 2.f, cx + thickness / 2.f, cy + arm, mark, thickness / 2.f);
		return true;
	}

	void drawReticuleButton(const HudReticuleButtonLook &look) override
	{
		const float x = static_cast<float>(look.x);
		const float y = static_cast<float>(look.y);
		const float w = static_cast<float>(look.width);
		const float h = static_cast<float>(look.height);
		const float cx = x + w / 2.f;
		const float cy = y + h / 2.f;

		if (look.disabled)
		{
			iV_DrawImage2(look.isCancel ? look.downImage : WzString("image_reticule_grey.png"), x, y, w, h);
			return;
		}

		float glow = 0.f;
		float glowScale = 1.f;
		bool ring = look.panelOpen && !look.isCancel;
		bool dot = false;
		if (look.pressed)
		{
			glow = params.glowPressed;
			glowScale = 0.85f;
		}
		else if (look.flashing)
		{
			if (hudReduceMotion())
			{
				ring = true;
				dot = true;
			}
			else if (look.paused)
			{
				glow = params.glowFlash;
				glowScale = 0.85f;
			}
			else
			{
				const float phase = static_cast<float>(realTime % params.pulsePeriodMs) / params.pulsePeriodMs;
				const float eased = 0.5f - 0.5f * std::cos(2.f * PI * phase);
				glow = params.glowHover + (params.glowFlash - params.glowHover) * eased + 0.1f;
				glowScale = 0.75f + 0.15f * eased;
			}
		}
		if (look.highlighted)
		{
			glow = std::max(glow, params.glowHover);
		}

		if (glow > 0.f)
		{
			const float rx = w * 0.85f * glowScale;
			const float ry = h * 0.85f * glowScale;
			const float soft = std::min(rx, ry) * 0.55f;
			pie_DrawSoftRoundedBox(cx - rx, cy - ry, cx + rx, cy + ry, withAlpha(palette.accent, static_cast<int>(255 * clamp01(glow))),
			                       std::min(rx, ry), soft);
		}

		iV_DrawImage2(look.pressed ? look.downImage : look.upImage, x, y, w, h);

		if (look.highlighted && !ring)
		{
			const float r = std::min(w, h) / 2.f + 1.f;
			pie_DrawRoundedBox(x - 1.f, y - 1.f, x + w + 1.f, y + h + 1.f, pal_RGBA(0, 0, 0, 0), r, withAlpha(palette.accent, 110), 1.f);
		}
		if (ring)
		{
			const float r = std::min(w, h) / 2.f + 1.f;
			pie_DrawRoundedBox(x - 1.f, y - 1.f, x + w + 1.f, y + h + 1.f, pal_RGBA(0, 0, 0, 0), r,
			                   withAlpha(palette.accent, look.flashing ? 240 : 150), 1.5f);
		}
		if (dot)
		{
			const float d = 7.f;
			pie_DrawRoundedBox(x + w - d, y, x + w, y + d, palette.stateWarn, d / 2.f, pal_RGBA(0, 0, 0, 200), 1.f);
		}
	}

	void drawPowerBar(PowerBar &bar, int xOffset, int yOffset) override
	{
		const auto values = bar.takeDisplayValues();
		const float x0 = static_cast<float>(xOffset + bar.x());
		const float y0 = static_cast<float>(yOffset + bar.y());
		const float x1 = x0 + static_cast<float>(bar.width());
		const float y1 = y0 + static_cast<float>(bar.height());
		const float h = y1 - y0;

		const int realPower = static_cast<int>(values.power + 1e-8) - values.hoverCost;
		const bool negative = values.power < 0;
		powerText.setText(WzString::number(realPower), font_regular);
		const float textColumnWidth = std::max(42.f, static_cast<float>(powerText.width()) + 12.f) + (negative ? h : 0.f);

		pie_DrawRoundedBox(x0, y0, x1, y1, pal_RGBA(8, 11, 16, 215), h / 2.f, palette.panelBorder, params.borderWidth);

		const float trackX0 = x0 + textColumnWidth;
		const float trackX1 = x1 - 3.f;
		const float trackY0 = y0 + 3.f;
		const float trackY1 = y1 - 3.f;
		const float trackWidth = std::max(0.f, trackX1 - trackX0);
		const float scale = static_cast<float>(POWERBAR_SCALE);
		const float available = clamp01((static_cast<float>(std::max(0.0, values.power)) / scale) / std::max(1.f, trackWidth)) * trackWidth;
		const float cost = clamp01((static_cast<float>(values.hoverCost) / scale) / std::max(1.f, trackWidth)) * trackWidth;
		const WzRect clip(static_cast<int>(trackX0), static_cast<int>(trackY0), static_cast<int>(trackWidth), static_cast<int>(trackY1 - trackY0));

		pie_DrawRoundedBox(trackX0, trackY0, trackX1, trackY1, pal_RGBA(255, 255, 255, 18), (trackY1 - trackY0) / 2.f);
		if (available > 0.5f)
		{
			pie_DrawGradientRoundedBoxClipped(trackX0, trackY0, trackX0 + available, trackY1, powerFillFrom, powerFillTo, true, 1.f,
			                                  glm::vec4((trackY1 - trackY0) / 2.f), pal_RGBA(0, 0, 0, 0), 0.f, clip);
		}
		if (cost > 0.5f)
		{
			const bool affordable = cost <= available + 0.5f;
			const PIELIGHT costColor = affordable ? palette.stateWarn : palette.stateBad;
			pie_DrawRoundedBoxClipped(trackX0, trackY0, trackX0 + cost, trackY1, withAlpha(costColor, 150), (trackY1 - trackY0) / 2.f,
			                          withAlpha(costColor, 255), 1.f, clip);
			costText.setText(WzString::fromUtf8(astringf("−%d", values.hoverCost)), font_small);
			const float labelX = std::min(trackX0 + cost + 4.f, trackX1 - static_cast<float>(costText.width()) - 4.f);
			drawShadowedText(costText, labelX, baselineFor(costText, y0, h), affordable ? palette.stateWarn : palette.stateBad);
		}

		const float textX = x0 + 8.f;
		if (negative)
		{
			const float d = h - 6.f;
			pie_DrawRoundedBox(textX, y0 + 3.f, textX + d, y0 + 3.f + d, palette.stateBad, d / 2.f);
			bangText.setText("!", font_small);
			bangText.render(textX + (d - bangText.width()) / 2.f, baselineFor(bangText, y0, h), pal_RGBA(20, 6, 6, 255));
		}
		powerText.render(textX + (negative ? h - 3.f : 0.f), baselineFor(powerText, y0, h), negative ? palette.stateBad : palette.textPrimary);

		const char *need = nullptr;
		std::string needBuffer;
		if (values.unusedDerricks > 0)
		{
			needBuffer = astringf(_("%u derrick(s) inactive"), values.unusedDerricks);
			need = needBuffer.c_str();
		}
		else if (negative)
		{
			need = _("Need more resources!");
		}
		if (need != nullptr)
		{
			needText.setText(WzString::fromUtf8(need), font_small);
			const float w = static_cast<float>(needText.width()) + 16.f;
			const float cx = (trackX0 + trackX1) / 2.f;
			pie_DrawRoundedBox(cx - w / 2.f, y0 + 1.f, cx + w / 2.f, y1 - 1.f, pal_RGBA(40, 30, 8, 225), (h - 2.f) / 2.f,
			                   withAlpha(palette.stateWarn, 220), 1.f);
			needText.render(cx - static_cast<float>(needText.width()) / 2.f, baselineFor(needText, y0, h), palette.stateWarn);
		}
	}

	void drawButtonBar(W_BARGRAPH &bar, int xOffset, int yOffset) override
	{
		const float x0 = static_cast<float>(xOffset + bar.x());
		const float y0 = static_cast<float>(yOffset + bar.y());
		const float w = static_cast<float>(bar.width());
		const float h = std::max(3.f, static_cast<float>(bar.height()));
		const float r = h / 2.f;
		const bool powerWait = sameColor(bar.majorCol, WZCOL_GREEN) && !bar.text.isEmpty();

		pie_DrawRoundedBox(x0, y0, x0 + w, y0 + h, pal_RGBA(0, 0, 0, 110), r);
		if (bar.minorSize > 0)
		{
			const float minor = w * std::min<int>(bar.minorSize, WBAR_SCALE) / WBAR_SCALE;
			pie_DrawRoundedBox(x0, y0, x0 + minor, y0 + h, withAlpha(bar.minorCol, 150), r);
		}
		const float major = w * std::min<int>(bar.majorSize, WBAR_SCALE) / WBAR_SCALE;
		if (major >= 1.f)
		{
			if (powerWait)
			{
				const WzRect clip(static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(major + 0.5f), static_cast<int>(h + 0.5f));
				for (float at = x0; at < x0 + major; at += 5.f)
				{
					pie_DrawRoundedBoxClipped(at, y0, at + 3.f, y0 + h, palette.progressPowerWait, r, pal_RGBA(0, 0, 0, 0), 0.f, clip);
				}
			}
			else
			{
				pie_DrawRoundedBox(x0, y0, x0 + major, y0 + h, bar.majorCol, r);
			}
		}
		if (!bar.text.isEmpty())
		{
			bar.wzCachedText.setText(bar.text, font_bar);
			const float textWidth = static_cast<float>(bar.wzCachedText.width());
			float tx = x0 + (w - textWidth) / 2.f;
			if (powerWait)
			{
				const float icon = 9.f;
				tx += icon / 2.f + 1.f;
				drawIcon(IMAGE_DES_POWER, tx - icon - 2.f, y0 + h - icon + 1.f, icon, palette.progressPowerWait);
			}
			bar.wzCachedText.renderOutlined(static_cast<int>(tx), static_cast<int>(y0 + h), bar.textCol, pal_RGBA(0, 0, 0, 230));
		}
	}

	void drawCloseButton(int x, int y, int width, int height, bool highlighted) override
	{
		const float y0 = static_cast<float>(y) + 1.f;
		const float y1 = static_cast<float>(y + height) - 1.f;
		const float x1 = static_cast<float>(x + width) - 2.f;
		const float x0 = std::max(static_cast<float>(x), x1 - (y1 - y0));
		pie_DrawRoundedBox(x0, y0, x1, y1, highlighted ? withAlpha(palette.stateBad, 200) : pal_RGBA(255, 255, 255, 22), 3.f,
		                   highlighted ? withAlpha(palette.stateBad, 255) : palette.panelBorder, 1.f);
		const float cx = (x0 + x1) / 2.f;
		const float cy = (y0 + y1) / 2.f;
		const float len = std::min(x1 - x0, y1 - y0) * 0.62f;
		const PIELIGHT mark = highlighted ? pal_RGBA(255, 255, 255, 255) : palette.textSecondary;
		pie_DrawSoftRoundedBoxRotated(cx, cy, len, 1.8f, PI / 4.f, mark, 0.9f, 0.9f);
		pie_DrawSoftRoundedBoxRotated(cx, cy, len, 1.8f, -PI / 4.f, mark, 0.9f, 0.9f);
	}

	PIELIGHT statsCostBarColor(PIELIGHT normal, bool affordable) const override
	{
		return affordable ? normal : palette.stateBad;
	}

	bool drawPageTab(const WzRect &rect, TabSelectionButtonRole role, bool selected, bool hovered) override
	{
		const float x0 = static_cast<float>(rect.x());
		const float y0 = static_cast<float>(rect.y());
		const float x1 = static_cast<float>(rect.right());
		const float y1 = static_cast<float>(rect.bottom());
		if (role != TabSelectionButtonRole::Tab)
		{
			const float cx = (x0 + x1) / 2.f + (role == TabSelectionButtonRole::PreviousPage ? -1.f : 1.f);
			const float cy = (y0 + y1) / 2.f;
			const float arm = std::min(x1 - x0, y1 - y0) * 0.6f;
			const float lean = role == TabSelectionButtonRole::PreviousPage ? -1.f : 1.f;
			const PIELIGHT mark = hovered ? palette.textPrimary : palette.textSecondary;
			pie_DrawSoftRoundedBoxRotated(cx, cy - arm * 0.35f, arm, 1.8f, lean * PI / 4.f, mark, 0.9f, 0.9f);
			pie_DrawSoftRoundedBoxRotated(cx, cy + arm * 0.35f, arm, 1.8f, -lean * PI / 4.f, mark, 0.9f, 0.9f);
			return true;
		}
		// A ring, a dark gap and an inset fill, as the Classic tab images have
		PIELIGHT ring = cardBorder;
		PIELIGHT fill = cardTop;
		if (selected)
		{
			ring = palette.accent;
			fill = mixColors(palette.accent, pal_RGBA(255, 255, 255, 255), 0.45f);
		}
		else if (hovered)
		{
			ring = cardHoverBorder;
			fill = cardHoverTop;
		}
		const float outer = 1.f;
		const float inner = outer + params.borderWidth + 1.f;
		pie_DrawRoundedBox(x0 + outer, y0 + outer, x1 - outer, y1 - outer, pal_RGBA(6, 9, 14, 220), (y1 - y0) / 2.f - outer, ring, params.borderWidth);
		pie_DrawRoundedBox(x0 + inner, y0 + inner, x1 - inner, y1 - inner, fill, (y1 - y0) / 2.f - inner, pal_RGBA(0, 0, 0, 0), 0.f);
		return true;
	}

	void drawTimerBackground(int x, int y, uint16_t image, bool flashOn, uint16_t) override
	{
		const float x0 = static_cast<float>(x);
		const float y0 = static_cast<float>(y);
		const float x1 = x0 + static_cast<float>(iV_GetImageWidth(IntImages, image));
		const float y1 = y0 + static_cast<float>(iV_GetImageHeight(IntImages, image));
		pie_DrawGradientRoundedBox(x0, y0, x1, y1, palette.panelBackground, palette.panelBackgroundBottom, false, 1.f, params.cardRadius + 1.f,
		                           flashOn ? palette.stateWarn : palette.panelBorder, flashOn ? 1.5f : params.borderWidth);
	}

	nonstd::optional<WzTipStyle> tooltipStyle() const override
	{
		WzTipStyle style;
		style.font = font_regular;
		style.textColor = palette.textPrimary;
		style.horizontalGap = 8;
		style.verticalGap = 4;
		style.drawFrame = [](const WzRect &rect) {
			currentHudSkin().drawPopoverFrame(rect);
		};
		return style;
	}

	void drawCard(const WzRect &rect, HudCardState state) override
	{
		PIELIGHT fill = cardTop;
		PIELIGHT border = cardBorder;
		switch (state)
		{
		case HudCardState::Normal: break;
		case HudCardState::Highlighted: fill = cardHoverTop; border = cardHoverBorder; break;
		case HudCardState::Selected: fill = cardDownTop; border = withAlpha(palette.accent, 230); break;
		case HudCardState::Disabled: fill = scaledAlpha(cardTop, 0.6f); border = withAlpha(palette.stateDisabled, 160); break;
		}
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()),
		                   static_cast<float>(rect.bottom()), fill, params.cardRadius, border, params.borderWidth);
	}

	void drawPill(const WzRect &rect, HudTone tone, bool filled) override
	{
		drawToned(rect, tone, filled, rect.height() / 2.f);
	}

	void drawChip(const WzRect &rect, HudTone tone, bool filled) override
	{
		drawToned(rect, tone, filled, params.cardRadius);
	}

	nonstd::optional<PIELIGHT> drawHeaderButton(const WzRect &rect, bool hovered, bool down) override
	{
		PIELIGHT fill = cardTop;
		PIELIGHT border = cardBorder;
		if (down)
		{
			fill = cardDownTop;
			border = withAlpha(palette.accent, 230);
		}
		else if (hovered)
		{
			fill = cardHoverTop;
			border = cardHoverBorder;
		}
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()),
		                   static_cast<float>(rect.bottom()), fill, params.cardRadius, border, params.borderWidth);
		return (down || hovered) ? palette.textPrimary : palette.textSecondary;
	}

	void drawProgress(const WzRect &rect, float fraction, HudProgressKind kind) override
	{
		const float x0 = static_cast<float>(rect.x());
		const float y0 = static_cast<float>(rect.y());
		const float x1 = static_cast<float>(rect.right());
		const float y1 = static_cast<float>(rect.bottom());
		const float r = (y1 - y0) / 2.f;
		pie_DrawRoundedBox(x0, y0, x1, y1, pal_RGBA(0, 0, 0, 170), r);
		PIELIGHT color = palette.progressBuild;
		switch (kind)
		{
		case HudProgressKind::Build: break;
		case HudProgressKind::Research: color = palette.progressResearch; break;
		case HudProgressKind::PowerWait: color = palette.progressPowerWait; break;
		case HudProgressKind::Neutral: color = palette.textSecondary; break;
		}
		const float fill = (x1 - x0) * clamp01(fraction);
		if (fill >= 1.f)
		{
			pie_DrawRoundedBoxClipped(x0, y0, x0 + std::max(fill, 2.f * r), y1, color, r, pal_RGBA(0, 0, 0, 0), 0.f,
			                          WzRect(rect.x(), rect.y(), static_cast<int>(fill + 0.5f), rect.height()));
		}
	}

	void drawPopoverFrame(const WzRect &rect) override
	{
		const float x0 = static_cast<float>(rect.x());
		const float y0 = static_cast<float>(rect.y());
		const float x1 = static_cast<float>(rect.right());
		const float y1 = static_cast<float>(rect.bottom());
		pie_DrawSoftRoundedBox(x0 - 4.f, y0 - 2.f, x1 + 4.f, y1 + 6.f, palette.shadow, params.cardRadius + 6.f, 5.f);
		pie_DrawRoundedBox(x0, y0, x1, y1, pal_RGBA(14, 18, 26, 240), params.cardRadius + 1.f, withAlpha(palette.panelBorder, 200), 1.f);
	}

	PIELIGHT toneColor(HudTone tone) const override
	{
		switch (tone)
		{
		case HudTone::Neutral: return palette.textSecondary;
		case HudTone::Accent: return palette.accent;
		case HudTone::Ok: return palette.stateOk;
		case HudTone::Warn: return palette.stateWarn;
		case HudTone::Bad: return palette.stateBad;
		}
		return palette.textSecondary;
	}

private:
	void drawToned(const WzRect &rect, HudTone tone, bool filled, float radius)
	{
		const PIELIGHT color = toneColor(tone);
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()),
		                   static_cast<float>(rect.bottom()), filled ? withAlpha(color, 70) : pal_RGBA(10, 14, 20, 200), radius,
		                   withAlpha(color, filled ? 255 : 170), 1.f);
	}

	struct Params
	{
		float panelRadius = 6.f;
		float cardRadius = 3.f;
		float borderWidth = 1.f;
		float accentEdge = 2.f;
		int accentEdgeAlpha = 150;
		float shadowSpread = 5.f;
		float glowHover = 0.42f;
		float glowPressed = 0.62f;
		float glowFlash = 0.9f;
		uint32_t pulsePeriodMs = 1000;
		bool idleJobDot = true;
	};

	static float baselineFor(WzText &text, float top, float height)
	{
		return top + (height - static_cast<float>(text.lineSize())) / 2.f - static_cast<float>(text.aboveBase());
	}

	static void drawShadowedText(WzText &text, float x, float y, PIELIGHT color)
	{
		text.render(x + 1.f, y + 1.f, pal_RGBA(0, 0, 0, 200));
		text.render(x, y, color);
	}

	Params params;
	HudColors palette = {
		pal_RGBA(24, 30, 40, 205),
		pal_RGBA(12, 15, 21, 225),
		pal_RGBA(120, 150, 190, 110),
		pal_RGBA(70, 175, 255, 255),
		pal_RGBA(236, 240, 245, 255),
		pal_RGBA(160, 172, 190, 255),
		pal_RGBA(96, 205, 128, 255),
		pal_RGBA(242, 190, 72, 255),
		pal_RGBA(238, 88, 72, 255),
		pal_RGBA(110, 115, 125, 255),
		pal_RGBA(242, 190, 72, 255),
		pal_RGBA(86, 178, 255, 255),
		pal_RGBA(96, 205, 128, 255),
		pal_RGBA(70, 175, 255, 70),
		pal_RGBA(0, 0, 0, 95),
	};
	HudMetrics sizes = {6.f, 3.f, 1.f, 6, font_regular_bold, font_small, font_small};

	PIELIGHT cardTop = pal_RGBA(34, 42, 56, 215);
	PIELIGHT cardBottom = pal_RGBA(20, 25, 34, 225);
	PIELIGHT cardBorder = pal_RGBA(84, 102, 128, 170);
	PIELIGHT cardHoverTop = pal_RGBA(46, 58, 78, 230);
	PIELIGHT cardHoverBorder = pal_RGBA(140, 170, 210, 210);
	PIELIGHT cardDownTop = pal_RGBA(18, 52, 86, 235);
	PIELIGHT cardDownBottom = pal_RGBA(12, 34, 58, 240);
	PIELIGHT powerFillFrom = pal_RGBA(60, 150, 230, 235);
	PIELIGHT powerFillTo = pal_RGBA(90, 205, 255, 235);

public:
	void releaseCaches() override
	{
		powerText = WzText();
		costText = WzText();
		needText = WzText();
		bangText = WzText();
	}

private:
	WzText powerText;
	WzText costText;
	WzText needText;
	WzText bangText;
};

}

HudSkin &modernHudSkin()
{
	static ModernHudSkin skin;
	return skin;
}
