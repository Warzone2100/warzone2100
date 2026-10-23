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
#include "hudradarblips.h"
#include "hudranks.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/widget/bar.h"

#include "../hci.h"
#include "../intdisplay.h"
#include "../intimage.h"
#include "../vectorimageloader.h"

#include <algorithm>
#include <array>
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

// OKLab and OKLCH (https://bottosson.github.io/posts/oklab/), only to compute colors (which stay display-referred sRGB)
struct Oklch
{
	float l = 0.f;
	float c = 0.f;
	float h = 0.f;  ///< (degrees)
};

float srgbToLinear(uint8_t value)
{
	const float v = static_cast<float>(value) / 255.f;
	return v <= 0.04045f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f);
}

uint8_t linearToSrgb(float v)
{
	v = clamp01(v);
	const float encoded = v <= 0.0031308f ? 12.92f * v : 1.055f * std::pow(v, 1.f / 2.4f) - 0.055f;
	return static_cast<uint8_t>(std::lround(encoded * 255.f));
}

Oklch toOklch(PIELIGHT color)
{
	const float r = srgbToLinear(color.byte.r);
	const float g = srgbToLinear(color.byte.g);
	const float b = srgbToLinear(color.byte.b);
	const float l = std::cbrt(0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b);
	const float m = std::cbrt(0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b);
	const float s = std::cbrt(0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b);
	const float labL = 0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s;
	const float labA = 1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s;
	const float labB = 0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s;
	Oklch result;
	result.l = labL;
	result.c = std::sqrt(labA * labA + labB * labB);
	result.h = std::atan2(labB, labA) * 180.f / PI;
	return result;
}

/// Inside the sRGB gamut: less chroma until it fits
PIELIGHT fromOklch(Oklch color, uint8_t alpha)
{
	for (float c = std::max(0.f, color.c);; c = std::max(0.f, c - 0.002f))
	{
		const float a = c * std::cos(color.h * PI / 180.f);
		const float b = c * std::sin(color.h * PI / 180.f);
		const float l = std::pow(color.l + 0.3963377774f * a + 0.2158037573f * b, 3.f);
		const float m = std::pow(color.l - 0.1055613458f * a - 0.0638541728f * b, 3.f);
		const float s = std::pow(color.l - 0.0894841775f * a - 1.2914855480f * b, 3.f);
		const float r = 4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
		const float g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
		const float bl = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;
		const float tolerance = 0.0005f;
		if (c <= 0.f || (std::min({r, g, bl}) >= -tolerance && std::max({r, g, bl}) <= 1.f + tolerance))
		{
			return pal_RGBA(linearToSrgb(r), linearToSrgb(g), linearToSrgb(bl), alpha);
		}
	}
}

/// The accents' colors (sRGB, chosen in OKLCH: lightness about 0.66–0.80, chroma up to 0.19, Blue the original)
PIELIGHT accentColor(HudAccent accent)
{
	switch (accent)
	{
	case HudAccent::Blue: break;
	case HudAccent::Teal: return pal_RGBA(16, 202, 202, 255);
	case HudAccent::Green: return pal_RGBA(116, 206, 96, 255);
	case HudAccent::Purple: return pal_RGBA(174, 132, 242, 255);
	case HudAccent::Magenta: return pal_RGBA(240, 112, 190, 255);
	case HudAccent::Red: return pal_RGBA(240, 85, 91, 255);
	case HudAccent::Orange: return pal_RGBA(249, 143, 58, 255);
	case HudAccent::Gray: return pal_RGBA(190, 190, 190, 255);
	}
	return pal_RGBA(70, 175, 255, 255);
}

constexpr size_t ICON_COUNT = static_cast<size_t>(HudIcon::Statistics) + 1;

struct IconFiles
{
	std::string outline;
	std::string fill;  ///< (optional: a light fill under the outline)
};

IconFiles namedIconFiles(const std::string &name)
{
	const std::string base = "images/hud/icons/" + name;
	return {base + ".svg", base + ".fill.svg"};
}

bool validIconName(const std::string &name)
{
	return !name.empty() && std::all_of(name.begin(), name.end(), [](char c) {
		return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
	});
}

const IconFiles &iconFiles(HudIcon icon)
{
	static const std::array<IconFiles, ICON_COUNT> files = [] {
		const char *names[] = {"research-droid", "research-weapon", "research-cyborg", "research-sensor", "research-computer",
		                       "research-power", "research-defense", "research-structure", "upgrade", "damage", "accuracy",
		                       "rate-of-fire", "repair", "ally-research", "favorite", "delivery-point", "obsolete-shown",
		                       "obsolete-hidden", "loop", "options", "statistics"};
		static_assert(sizeof(names) / sizeof(names[0]) == ICON_COUNT, "an icon without a file");
		std::array<IconFiles, ICON_COUNT> result;
		for (size_t i = 0; i < result.size(); ++i)
		{
			result[i] = namedIconFiles(names[i]);
		}
		return result;
	}();
	return files[static_cast<size_t>(icon)];
}

constexpr int ICON_FILL_ALPHA = 72;
constexpr float ORDER_ICON_HEIGHT = 15.f;
constexpr float ORDER_ICON_MARGIN = 3.f;
constexpr float TIMER_ICON_MARGIN = 2.f;
constexpr float TIMER_CLOCK_MARGIN = 2.f;
/// The room added before the mission timer's time (from TIMER_LABELX, 15, in mission.cpp), and the stopwatch's
/// segment, up to the time
constexpr int TIMER_CLOCK_GUTTER = 6;
constexpr float TIMER_CLOCK_SEGMENT = 19.f;
/// The transporter's ETA button: the right edge of the ETA's well, and where the load's label starts (TRAN_TIMER_X,
/// IDTRANS_CAPACITY in mission.cpp)
constexpr float TRANSPORTER_ETA_WELL = 41.f;
constexpr float TRANSPORTER_LOAD_LABEL = 70.f;
constexpr float ICON_HALO_WIDTH = 1.3f;

PIELIGHT iconColor(HudIcon icon)
{
	switch (icon)
	{
	case HudIcon::Upgrade:
	case HudIcon::Favorite: return pal_RGBA(242, 190, 72, 255);
	case HudIcon::Damage: return pal_RGBA(238, 88, 72, 255);
	case HudIcon::Accuracy: return pal_RGBA(86, 178, 255, 255);
	case HudIcon::RateOfFire: return pal_RGBA(79, 209, 197, 255);
	case HudIcon::Repair: return pal_RGBA(96, 205, 128, 255);
	default: return pal_RGBA(188, 198, 212, 255);
	}
}

/// Markers (a favorite, an ally researching) are solid, the rest outlines over a light fill
int iconFillAlpha(HudIcon icon)
{
	return (icon == HudIcon::Favorite || icon == HudIcon::AllyResearch) ? 255 : ICON_FILL_ALPHA;
}

/// The icon as a square centered in the rectangle: a halo (dark, or light under a dark color), the fill, the outline
bool drawIconShape(const IconFiles &files, const WzRect &rect, PIELIGHT color, int fillAlpha, bool halo)
{
	if (!loadGameVectorImage(files.outline))
	{
		return false;
	}
	const float size = static_cast<float>(std::min(rect.width(), rect.height()));
	const float x = static_cast<float>(rect.x()) + (static_cast<float>(rect.width()) - size) / 2.f;
	const float y = static_cast<float>(rect.y()) + (static_cast<float>(rect.height()) - size) / 2.f;
	if (halo)
	{
		const int luminance = (color.byte.r * 299 + color.byte.g * 587 + color.byte.b * 114) / 1000;
		const PIELIGHT haloColor = luminance < 90 ? pal_RGBA(200, 206, 216, 200) : pal_RGBA(8, 10, 14, 210);
		drawGameVectorImage(files.outline, x, y, size, size, haloColor, ICON_HALO_WIDTH);
	}
	if (fillAlpha > 0)
	{
		drawGameVectorImage(files.fill, x, y, size, size, withAlpha(color, fillAlpha * color.byte.a / 255), 0.f, true);
	}
	drawGameVectorImage(files.outline, x, y, size, size, color);
	return true;
}

/// The replay controls: the bar's opacity (about two thirds), the buttons' icons, the timeline's track and playhead
constexpr int REPLAY_BAR_ALPHA = 170;
constexpr float REPLAY_ICON_SIZE = 14.f;
constexpr float REPLAY_TRACK_HEIGHT = 4.f;
constexpr float REPLAY_PLAYHEAD_RADIUS = 6.f;
// The spectator statistics: more see-through than the replay bar (it covers more of the game)
constexpr int SPECTATOR_STATS_ALPHA = 140;

const char *replayIconPath(HudReplayButton button)
{
	switch (button)
	{
	case HudReplayButton::Start: return "images/hud/icons/replay-start.svg";
	case HudReplayButton::Back: return "images/hud/icons/replay-back.svg";
	case HudReplayButton::Pause: return "images/hud/icons/replay-pause.svg";
	case HudReplayButton::Play: return "images/hud/icons/replay-play.svg";
	case HudReplayButton::Forward: return "images/hud/icons/replay-forward.svg";
	case HudReplayButton::FastForward: return "images/hud/icons/replay-fast-forward.svg";
	}
	return "images/hud/icons/replay-play.svg";
}

/// The radar's frame (the panel's padding around the map, and its corners) and the north arrow's height
constexpr float RADAR_FRAME_PADDING = 3.f;
constexpr float RADAR_FRAME_RADIUS = 5.f;
constexpr float RADAR_NORTH_HEIGHT = 30.f;

/// The Modern skin's colors, made for the default accent (blue): the other accents' are computed from them
HudColors modernColors()
{
	return {
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
}

/// Its other colors that follow the accent: tinted surfaces and borders, and the accent's own shades
struct ModernTints
{
	PIELIGHT cardTop = pal_RGBA(34, 42, 56, 215);
	PIELIGHT cardBottom = pal_RGBA(20, 25, 34, 225);
	PIELIGHT cardBorder = pal_RGBA(84, 102, 128, 170);
	PIELIGHT cardHoverTop = pal_RGBA(46, 58, 78, 230);
	PIELIGHT cardHoverBorder = pal_RGBA(140, 170, 210, 210);
	PIELIGHT cardDownTop = pal_RGBA(18, 52, 86, 235);
	PIELIGHT cardDownBottom = pal_RGBA(12, 34, 58, 240);
	PIELIGHT wellFill = pal_RGBA(6, 9, 14, 255);      ///< (the darkest: pills, fields, tab gaps)
	PIELIGHT fieldFill = pal_RGBA(10, 14, 20, 255);   ///< (page control, unfilled pills and chips)
	PIELIGHT troughFill = pal_RGBA(8, 11, 16, 255);   ///< (the power bar's track)
	PIELIGHT popoverFill = pal_RGBA(14, 18, 26, 255);
	PIELIGHT onAccent = pal_RGBA(236, 240, 245, 255); ///< (text on an accent fill)
};

class ModernHudSkin : public ClassicHudSkin
{
public:
	HudSkinId id() const override { return HudSkinId::Modern; }

	/// The colors for an accent: the accent itself, and the tinted colors with its hue (and their chroma scaled down for a
	/// grayer accent), each at its own lightness. The meanings (states, progress, power) stay as they are
	void useAccent(HudAccent accent, HudPanelTint tint)
	{
		if (accent == accentInUse && tint == tintInUse)
		{
			return;
		}
		accentInUse = accent;
		tintInUse = tint;
		palette = modernColors();
		tints = ModernTints();
		if (accent == HudAccent::Blue && tint == HudPanelTint::Full)
		{
			return;
		}
		const Oklch from = toOklch(accentColor(HudAccent::Blue));
		const Oklch to = toOklch(accentColor(accent));
		// The accent's own shades take all of its chroma, the tinted colors as much as the panel tint asks for
		const float accentChroma = std::min(1.f, to.c / from.c);
		const float tintChroma = accentChroma * (tint == HudPanelTint::Full ? 1.f : (tint == HudPanelTint::Subtle ? 0.5f : 0.f));
		auto follow = [&](PIELIGHT &color, float chroma) {
			Oklch shade = toOklch(color);
			shade.c *= chroma;
			shade.h += to.h - from.h;
			color = fromOklch(shade, color.byte.a);
		};
		for (PIELIGHT *color : {&palette.panelBackground, &palette.panelBackgroundBottom, &palette.panelBorder, &palette.textPrimary,
		                        &palette.textSecondary, &tints.cardTop, &tints.cardBottom, &tints.cardBorder, &tints.cardHoverTop,
		                        &tints.cardHoverBorder, &tints.wellFill, &tints.fieldFill, &tints.troughFill, &tints.popoverFill})
		{
			follow(*color, tintChroma);
		}
		follow(tints.cardDownTop, accentChroma);
		follow(tints.cardDownBottom, accentChroma);
		palette.accent = accentColor(accent);
		palette.highlight = withAlpha(palette.accent, palette.highlight.byte.a);
		// Light text on the accent unless it is light itself (then a dark one: the panels' darkest)
		const float luminance = 0.2126f * srgbToLinear(palette.accent.byte.r) + 0.7152f * srgbToLinear(palette.accent.byte.g)
			+ 0.0722f * srgbToLinear(palette.accent.byte.b);
		tints.onAccent = luminance > 0.45f ? withAlpha(tints.wellFill, 255) : palette.textPrimary;
	}

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
		PIELIGHT top = tints.cardTop;
		PIELIGHT bottom = tints.cardBottom;
		PIELIGHT border = tints.cardBorder;
		if (look.down)
		{
			top = tints.cardDownTop;
			bottom = tints.cardDownBottom;
			border = withAlpha(palette.accent, 220);
		}
		else if (look.hovered)
		{
			top = tints.cardHoverTop;
			border = tints.cardHoverBorder;
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
		const PIELIGHT outline = look.down ? withAlpha(palette.accent, 230) : (look.hovered ? tints.cardHoverBorder : withAlpha(tints.cardBorder, 200));
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
		if (look.count == 0 && look.dot)
		{
			// The dot in the accent, where the count goes
			const float d = 9.f;
			const float bx = std::round(x - 1.f);
			const float by = std::round(y - 1.f);
			pie_DrawRoundedBox(bx, by, bx + d, by + d, palette.accent, d / 2.f, pal_RGBA(0, 0, 0, 220), 1.f);
		}
		if (look.count > 0)
		{
			// The count on the accent, at the button's top left (the warning dot is at its top right)
			countText.setText(WzString::fromUtf8(look.count > 99 ? std::string("99+") : std::to_string(look.count)), font_bar);
			countDigits.setText(WzString::fromUtf8("0123456789"), font_bar);
			const float d = 14.f;
			const float bw = std::max(d, static_cast<float>(countText.width()) + 7.f);
			const float bx = std::round(x - 3.f);
			const float by = std::round(y - 3.f);
			pie_DrawRoundedBox(bx, by, bx + bw, by + d, palette.accent, d / 2.f, pal_RGBA(0, 0, 0, 220), 1.f);
			countText.renderCenteredOnInk(WzRect(static_cast<int>(bx), static_cast<int>(by), static_cast<int>(bw), static_cast<int>(d)), palette.textPrimary, &countDigits);
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

		pie_DrawRoundedBox(x0, y0, x1, y1, withAlpha(tints.troughFill, 215), h / 2.f, palette.panelBorder, params.borderWidth);

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
				const int side = static_cast<int>(icon);
				if (!drawNamedIcon("power", WzRect(static_cast<int>(tx - icon - 2.f), static_cast<int>(y0 + h - icon + 1.f), side, side), palette.progressPowerWait))
				{
					drawIcon(IMAGE_DES_POWER, tx - icon - 2.f, y0 + h - icon + 1.f, icon, palette.progressPowerWait);
				}
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

	bool drawsProductionRunBadge() const override { return true; }

	int panelTopPadding() const override
	{
		return static_cast<int>(std::ceil(params.accentEdge)) + 2;
	}

	bool drawPageTab(const WzRect &rect, TabSelectionButtonRole role, bool selected, bool hovered) override
	{
		const float x0 = static_cast<float>(rect.x());
		const float y0 = static_cast<float>(rect.y());
		const float x1 = static_cast<float>(rect.right());
		const float y1 = static_cast<float>(rect.bottom());
		if (role != TabSelectionButtonRole::Tab)
		{
			// (nudged toward the edge of the list, where a page tab's scroll button is)
			drawChevron(x0, y0, x1, y1, role == TabSelectionButtonRole::PreviousPage, hovered ? palette.textPrimary : palette.textSecondary, 1.f);
			return true;
		}
		// A ring, a dark gap and an inset fill, as the Classic tab images have
		PIELIGHT ring = tints.cardBorder;
		PIELIGHT fill = tints.cardTop;
		if (selected)
		{
			ring = palette.accent;
			fill = mixColors(palette.accent, pal_RGBA(255, 255, 255, 255), 0.45f);
		}
		else if (hovered)
		{
			ring = tints.cardHoverBorder;
			fill = tints.cardHoverTop;
		}
		const float outer = 1.f;
		const float inner = outer + params.borderWidth + 1.f;
		pie_DrawRoundedBox(x0 + outer, y0 + outer, x1 - outer, y1 - outer, withAlpha(tints.wellFill, 220), (y1 - y0) / 2.f - outer, ring, params.borderWidth);
		pie_DrawRoundedBox(x0 + inner, y0 + inner, x1 - inner, y1 - inner, fill, (y1 - y0) / 2.f - inner, pal_RGBA(0, 0, 0, 0), 0.f);
		return true;
	}

	void drawPageControlFrame(const WzRect &rect) override
	{
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()),
		                   static_cast<float>(rect.bottom()), withAlpha(tints.fieldFill, 200), params.cardRadius, tints.cardBorder, params.borderWidth);
	}

	nonstd::optional<PIELIGHT> drawPageSegment(const HudPageSegmentLook &look) override
	{
		const float x0 = static_cast<float>(look.rect.x());
		const float y0 = static_cast<float>(look.rect.y());
		const float x1 = static_cast<float>(look.rect.right());
		const float y1 = static_cast<float>(look.rect.bottom());
		const bool hovered = look.hovered && !look.disabled;
		if (look.large)
		{
			PIELIGHT fill = tints.cardTop;
			PIELIGHT border = tints.cardBorder;
			if (look.selected)
			{
				fill = withAlpha(palette.accent, 230);
				border = palette.accent;
			}
			else if (hovered)
			{
				fill = tints.cardHoverTop;
				border = tints.cardHoverBorder;
			}
			pie_DrawRoundedBox(x0, y0, x1, y1, fill, params.cardRadius + 1.f, border, params.borderWidth);
		}
		else
		{
			// (the rectangle is the cell between the frame's border or the separators, and the frame's whole height)
			if (look.separator)
			{
				pie_BoxFill(look.rect.x() - 1, look.rect.y() + 3, look.rect.x(), look.rect.bottom() - 3, tints.cardBorder);
			}
			if (look.role == TabSelectionButtonRole::Tab && (look.selected || hovered))
			{
				// Whole pixels, 1 px clear of the frame's border and the cells beside it
				pie_DrawRoundedBox(x0 + 1.f, y0 + 2.f, x1 - 1.f, y1 - 2.f, look.selected ? withAlpha(palette.accent, 230) : tints.cardHoverTop,
				                   params.cardRadius - 1.f, pal_RGBA(0, 0, 0, 0), 0.f);
			}
		}
		const PIELIGHT ink = look.disabled ? withAlpha(palette.textSecondary, 90) : (look.selected ? tints.onAccent : (hovered ? palette.textPrimary : palette.textSecondary));
		if (look.role != TabSelectionButtonRole::Tab)
		{
			drawChevron(x0, y0, x1, y1, look.role == TabSelectionButtonRole::PreviousPage, ink, 0.f);
			return nonstd::nullopt;
		}
		return ink;
	}

	void drawTimerBackground(int x, int y, uint16_t image, bool flashOn, uint16_t) override
	{
		const float x0 = static_cast<float>(x);
		const float y0 = static_cast<float>(y);
		const float x1 = x0 + static_cast<float>(iV_GetImageWidth(IntImages, image));
		const float y1 = y0 + static_cast<float>(iV_GetImageHeight(IntImages, image));
		pie_DrawGradientRoundedBox(x0, y0, x1, y1, palette.panelBackground, palette.panelBackgroundBottom, false, 1.f, params.cardRadius + 1.f,
		                           flashOn ? palette.stateWarn : palette.panelBorder, flashOn ? 1.5f : params.borderWidth);
		if (image == IMAGE_MISSION_CLOCK || image == IMAGE_MISSION_CLOCK_UP)
		{
			// The stopwatch on a tinted segment, in the room the time leaves before it (its label is at TIMER_LABELX)
			const PIELIGHT color = flashOn ? palette.stateWarn : palette.accent;
			const float inset = flashOn ? 1.5f : params.borderWidth;
			const float radius = std::max(0.f, params.cardRadius + 1.f - inset);
			const float segmentRight = x0 + TIMER_CLOCK_SEGMENT;
			pie_DrawRoundedBox(x0 + inset, y0 + inset, segmentRight, y1 - inset, withAlpha(color, 56), glm::vec4(radius, 0.f, radius, 0.f),
			                   pal_RGBA(0, 0, 0, 0), 0.f);
			const float size = y1 - y0 - 2 * TIMER_CLOCK_MARGIN;
			drawGameVectorImage("images/hud/icons/mission-clock.svg", std::round(x0 + inset + (segmentRight - x0 - inset - size) / 2.f),
			                    y0 + TIMER_CLOCK_MARGIN, size, size, color);
		}
	}

	int timerIconGutter() const override { return TIMER_CLOCK_GUTTER; }

	bool drawTransporterButton(const HudTransporterButtonLook &look) override
	{
		const std::string icon = look.launch ? "images/hud/icons/transporter-launch.svg" : "images/hud/icons/transporter-arriving.svg";
		if (!loadGameVectorImage(icon))
		{
			return false;
		}
		const WzRect &rect = look.rect;
		const float x0 = static_cast<float>(rect.x());
		const float y0 = static_cast<float>(rect.y());
		const float x1 = static_cast<float>(rect.right());
		const float y1 = static_cast<float>(rect.bottom());
		PIELIGHT border = palette.panelBorder;
		PIELIGHT ink = palette.textSecondary;
		if (look.flashOn)
		{
			border = palette.accent;
			ink = palette.accent;
		}
		else if (look.highlighted)
		{
			border = tints.cardHoverBorder;
			ink = palette.textPrimary;
		}
		pie_DrawGradientRoundedBox(x0, y0, x1, y1, palette.panelBackground, palette.panelBackgroundBottom, false, 1.f, params.cardRadius + 1.f,
		                           border, look.flashOn ? 1.5f : params.borderWidth);
		float iconLeft = x0;
		float iconRight = x1;
		if (!look.launch)
		{
			// The ETA's well (where its label is), then the icon, before the load's label
			pie_DrawRoundedBox(x0 + 2.f, y0 + 2.f, x0 + TRANSPORTER_ETA_WELL, y1 - 2.f, tints.wellFill, params.cardRadius - 1.f, pal_RGBA(0, 0, 0, 0), 0.f);
			iconLeft = x0 + TRANSPORTER_ETA_WELL;
			iconRight = x0 + TRANSPORTER_LOAD_LABEL;
		}
		const float height = (y1 - y0) - 2 * TIMER_ICON_MARGIN;
		const float width = height * gameVectorImageAspect(icon);
		drawGameVectorImage(icon, std::round(iconLeft + (iconRight - iconLeft - width) / 2.f), y0 + TIMER_ICON_MARGIN, width, height, ink);
		return true;
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
		PIELIGHT fill = tints.cardTop;
		PIELIGHT border = tints.cardBorder;
		switch (state)
		{
		case HudCardState::Normal: break;
		case HudCardState::Highlighted: fill = tints.cardHoverTop; border = tints.cardHoverBorder; break;
		case HudCardState::Selected: fill = tints.cardDownTop; border = withAlpha(palette.accent, 230); break;
		case HudCardState::Disabled: fill = scaledAlpha(tints.cardTop, 0.6f); border = withAlpha(palette.stateDisabled, 160); break;
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

	void drawInputField(const WzRect &rect, bool editing) override
	{
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()),
		                   static_cast<float>(rect.bottom()), withAlpha(tints.wellFill, 190), params.cardRadius,
		                   editing ? withAlpha(palette.accent, 230) : palette.panelBorder, params.borderWidth);
	}

	nonstd::optional<PIELIGHT> drawHeaderButton(const WzRect &rect, bool hovered, bool down) override
	{
		PIELIGHT fill = tints.cardTop;
		PIELIGHT border = tints.cardBorder;
		if (down)
		{
			fill = tints.cardDownTop;
			border = withAlpha(palette.accent, 230);
		}
		else if (hovered)
		{
			fill = tints.cardHoverTop;
			border = tints.cardHoverBorder;
		}
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()),
		                   static_cast<float>(rect.bottom()), fill, params.cardRadius, border, params.borderWidth);
		return (down || hovered) ? palette.textPrimary : palette.textSecondary;
	}

	bool drawsIcons() const override { return true; }
	bool drawsRankInsignia() const override { return true; }
	bool drawsRadarBlips() const override { return true; }
	bool drawsConnectionPanel() const override { return true; }
	bool drawsIntelligenceScreen() const override { return true; }
	bool drawsPregameScreen() const override { return true; }
	bool drawsQuickChat() const override { return true; }
	bool drawsResultsScreens() const override { return true; }
	bool drawsDesignScreen() const override { return true; }
	bool drawsConsole() const override { return true; }

	PIELIGHT followAccent(PIELIGHT color, float tint) const override
	{
		const Oklch from = toOklch(accentColor(HudAccent::Blue));
		const Oklch to = toOklch(accentColor(accentInUse));
		Oklch shade = toOklch(color);
		shade.c *= std::min(1.f, to.c / from.c) * tint;
		shade.h += to.h - from.h;
		return fromOklch(shade, color.byte.a);
	}

	PIELIGHT inHueOf(PIELIGHT color, PIELIGHT hueOf, float chroma) const override
	{
		Oklch shade = toOklch(color);
		shade.c = chroma;
		shade.h = toOklch(hueOf).h;
		return fromOklch(shade, color.byte.a);
	}

	bool drawReplayBar(const WzRect &rect) override
	{
		// See-through over the game, in the darkest fill, with no shadow
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
		                   withAlpha(tints.wellFill, REPLAY_BAR_ALPHA), params.cardRadius + 1.f, withAlpha(palette.panelBorder, 200), 1.f);
		return true;
	}

	bool drawSpectatorStrip(const WzRect &rect) override
	{
		return drawReplayBar(rect);
	}

	bool drawsSpectatorStats() const override { return true; }

	void drawSpectatorStatsPanel(const WzRect &rect) override
	{
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
		                   withAlpha(tints.wellFill, SPECTATOR_STATS_ALPHA), params.cardRadius + 1.f, withAlpha(palette.panelBorder, 200), 1.f);
	}

	void drawSpectatorStatsHeader(const WzRect &rect, const std::vector<int> &columnLinesX) override
	{
		const float x0 = static_cast<float>(rect.x());
		const float x1 = static_cast<float>(rect.right());
		const float y1 = static_cast<float>(rect.bottom());
		pie_DrawRoundedBox(x0, static_cast<float>(rect.y()), x1, y1, withAlpha(tints.popoverFill, 235), params.cardRadius, pal_RGBA(0, 0, 0, 0), 0.f);
		pie_UniTransBoxFill(x0, y1 - 1.f, x1, y1, withAlpha(palette.accent, 170));
		// Not after the last column (the header's end)
		for (size_t i = 0; i + 1 < columnLinesX.size(); ++i)
		{
			const float x = static_cast<float>(columnLinesX[i]);
			pie_UniTransBoxFill(x, static_cast<float>(rect.y()) + 6.f, x + 1.f, y1 - 6.f, withAlpha(palette.panelBorder, 140));
		}
	}

	PIELIGHT spectatorStatsAlternateRowFill() const override
	{
		return pal_RGBA(255, 255, 255, 12);
	}

	bool drawReplayButton(const HudReplayButtonLook &look) override
	{
		const WzRect &rect = look.rect;
		const bool fast = look.fastForward > 0;
		drawCard(rect, look.disabled ? HudCardState::Disabled
		               : (look.down || fast ? HudCardState::Selected : (look.highlighted ? HudCardState::Highlighted : HudCardState::Normal)));
		const PIELIGHT color = look.disabled ? palette.stateDisabled : (fast ? palette.accent : palette.textPrimary);
		const float size = REPLAY_ICON_SIZE;
		drawGameVectorImage(replayIconPath(look.button), std::round(static_cast<float>(rect.x()) + (static_cast<float>(rect.width()) - size) / 2.f),
		                    std::round(static_cast<float>(rect.y()) + (static_cast<float>(rect.height()) - size) / 2.f), size, size, color);
		if (fast)
		{
			// How much faster, at most, as a badge on the button's top right
			replaySpeed.setText(WzString::fromUtf8(astringf("×%d", look.fastForward)), font_bar);
			const float w = static_cast<float>(replaySpeed.width()) + 6.f;
			const float h = 10.f;
			const float bx1 = static_cast<float>(rect.right()) + 3.f;
			const float by0 = static_cast<float>(rect.y()) - 4.f;
			pie_DrawRoundedBox(bx1 - w, by0, bx1, by0 + h, palette.accent, h / 2.f, withAlpha(tints.wellFill, 255), 1.f);
			replaySpeed.renderCenteredOnInk(WzRect(static_cast<int>(bx1 - w), static_cast<int>(by0), static_cast<int>(w), static_cast<int>(h)), tints.wellFill);
		}
		return true;
	}

	bool drawReplayTrack(const WzRect &track, float playedX) override
	{
		const float x0 = static_cast<float>(track.x());
		const float x1 = static_cast<float>(track.right());
		const float cy = static_cast<float>(track.y()) + static_cast<float>(track.height()) / 2.f;
		const float half = REPLAY_TRACK_HEIGHT / 2.f;
		pie_DrawRoundedBox(x0, cy - half, x1, cy + half, tints.wellFill, half, withAlpha(palette.panelBorder, 200), 1.f);
		if (playedX > x0 + 1.f)
		{
			pie_DrawRoundedBox(x0, cy - half, std::min(playedX, x1), cy + half, palette.accent, half);
		}
		return true;
	}

	bool drawReplayPlayhead(float centerX, float centerY, HudReplayPlayhead kind) override
	{
		const float r = REPLAY_PLAYHEAD_RADIUS;
		const PIELIGHT edge = withAlpha(tints.wellFill, 255);
		switch (kind)
		{
		case HudReplayPlayhead::Current:
			pie_DrawRoundedBox(centerX - r, centerY - r, centerX + r, centerY + r, palette.textPrimary, r, edge, 1.f);
			break;
		case HudReplayPlayhead::Drag:
			pie_DrawRoundedBox(centerX - r, centerY - r, centerX + r, centerY + r, withAlpha(palette.textPrimary, 150), r, edge, 1.f);
			break;
		case HudReplayPlayhead::SeekTarget:
			pie_DrawRoundedBox(centerX - r, centerY - r, centerX + r, centerY + r, withAlpha(palette.accent, 70), r, palette.accent, 1.5f);
			break;
		}
		return true;
	}

	bool drawReplayTimeTip(const WzRect &rect) override
	{
		drawPopoverFrame(rect);
		return true;
	}

	bool drawRadarFrame(const glm::mat4 &mapSpace, float width, float height, bool north) override
	{
		// A panel 3 px around the map, under it, with no accent edge (it turns with the camera)
		const float x0 = -RADAR_FRAME_PADDING - 1.f;
		const float y0 = -RADAR_FRAME_PADDING - 1.f;
		const float x1 = width + RADAR_FRAME_PADDING + 1.f;
		const float y1 = height + RADAR_FRAME_PADDING + 1.f;
		pie_DrawRoundedBoxTransformed(mapSpace, x0 - 2.f, y0 - 1.f, x1 + 2.f, y1 + 3.f, pal_RGBA(0, 0, 0, 70), RADAR_FRAME_RADIUS + 2.f, pal_RGBA(0, 0, 0, 0), 0.f, 4.f);
		pie_DrawRoundedBoxTransformed(mapSpace, x0, y0, x1, y1, withAlpha(palette.panelBackground, 235), RADAR_FRAME_RADIUS, palette.panelBorder, params.borderWidth);
		if (north)
		{
			// As the Classic arrow: against the frame's west side, its tip level with the frame's top
			const std::string path = "images/hud/icons/radar-north.svg";
			const float arrowHeight = RADAR_NORTH_HEIGHT;
			const float arrowWidth = arrowHeight * gameVectorImageAspect(path, true);
			const float ax = x0 - 1.f - arrowWidth;
			drawGameVectorImageTransformed(path, mapSpace, ax, y0, arrowWidth, arrowHeight, pal_RGBA(8, 10, 14, 200), 1.f);
			drawGameVectorImageTransformed(path, mapSpace, ax, y0, arrowWidth, arrowHeight, palette.textSecondary);
		}
		return true;
	}

	bool drawHudIcon(HudIcon icon, const WzRect &rect, nonstd::optional<PIELIGHT> color) override
	{
		return drawIconShape(iconFiles(icon), rect, color.value_or(iconColor(icon)), iconFillAlpha(icon), true);
	}

	bool hasNamedIcon(const std::string &name) override
	{
		return validIconName(name) && loadGameVectorImage(namedIconFiles(name).outline, true);
	}

	bool drawNamedIcon(const std::string &name, const WzRect &rect, PIELIGHT color) override
	{
		return validIconName(name) && drawIconShape(namedIconFiles(name), rect, color, 0, false);
	}

	bool drawIconButton(const HudIconButtonLook &look) override
	{
		const WzRect &rect = look.rect;
		const float opacity = clamp01(look.opacity) * (look.disabled ? 0.5f : 1.f);
		auto faded = [opacity](PIELIGHT color) { return scaledAlpha(color, opacity); };
		if (!loadGameVectorImage(iconFiles(look.icon).outline))
		{
			return false;
		}
		if (look.style == HudIconButtonStyle::Bare)
		{
			const PIELIGHT color = look.active ? iconColor(look.icon) : (look.highlighted ? palette.textPrimary : palette.textSecondary);
			return drawIconShape(iconFiles(look.icon), rect, faded(color), look.active ? iconFillAlpha(look.icon) : 0, false);
		}
		WzRect frame = rect;
		int iconSize = rect.height() - 2;
		if (look.style == HudIconButtonStyle::Card)
		{
			const int side = std::min(rect.width(), rect.height());
			frame = WzRect(rect.x() + (rect.width() - side) / 2, rect.y() + (rect.height() - side) / 2, side, side);
			iconSize = side * 3 / 5;
		}
		const float x0 = static_cast<float>(frame.x());
		const float y0 = static_cast<float>(frame.y());
		const float x1 = static_cast<float>(frame.right());
		const float y1 = static_cast<float>(frame.bottom());
		PIELIGHT border = look.style == HudIconButtonStyle::Card ? palette.panelBorder : tints.cardBorder;
		if (look.down || look.active)
		{
			border = withAlpha(palette.accent, 230);
		}
		else if (look.highlighted)
		{
			border = tints.cardHoverBorder;
		}
		if (look.style == HudIconButtonStyle::Card)
		{
			const float radius = std::min(params.panelRadius, frame.height() / 4.f);
			pie_DrawGradientRoundedBox(x0, y0, x1, y1, faded(look.down ? tints.cardDownTop : palette.panelBackground),
			                           faded(look.down ? tints.cardDownBottom : palette.panelBackgroundBottom), false, 1.f, radius, faded(border),
			                           params.borderWidth);
		}
		else
		{
			PIELIGHT fill = tints.cardTop;
			if (look.down || look.active)
			{
				fill = tints.cardDownTop;
			}
			else if (look.highlighted)
			{
				fill = tints.cardHoverTop;
			}
			pie_DrawRoundedBox(x0, y0, x1, y1, faded(fill), std::min(params.cardRadius, frame.height() / 2.f), faded(border), params.borderWidth);
		}
		const int shift = look.down ? 1 : 0;
		PIELIGHT color = look.active ? palette.accent : ((look.down || look.highlighted) ? palette.textPrimary : palette.textSecondary);
		if (look.disabled)
		{
			color = palette.stateDisabled;
		}
		return drawIconShape(iconFiles(look.icon), WzRect(frame.x() + (frame.width() - iconSize) / 2, frame.y() + (frame.height() - iconSize) / 2 + shift,
		                                       iconSize, iconSize), faded(color), ICON_FILL_ALPHA, false);
	}

	bool drawOrderButton(const HudOrderButtonLook &look) override
	{
		const bool numbered = look.icon == nullptr;
		if (numbered && (look.number < 0 || static_cast<size_t>(look.number) >= orderNumbers.size()))
		{
			return false;
		}
		const IconFiles files = numbered ? IconFiles() : namedIconFiles(look.icon);
		if (!numbered && !loadGameVectorImage(files.outline))
		{
			return false;
		}
		const WzRect &rect = look.rect;
		const float opacity = look.disabled ? 0.5f : 1.f;
		PIELIGHT fill = tints.cardTop;
		PIELIGHT border = tints.cardBorder;
		PIELIGHT ink = palette.textSecondary;
		if (look.down)
		{
			fill = tints.cardDownTop;
			border = withAlpha(palette.accent, 230);
			ink = palette.accent;
		}
		else if (look.highlighted && !look.disabled)
		{
			fill = tints.cardHoverTop;
			border = tints.cardHoverBorder;
			ink = palette.textPrimary;
		}
		if (look.disabled)
		{
			ink = palette.stateDisabled;
		}
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()),
		                   static_cast<float>(rect.bottom()), scaledAlpha(fill, opacity), std::min(params.cardRadius, rect.height() / 2.f),
		                   scaledAlpha(border, opacity), params.borderWidth);
		ink = scaledAlpha(ink, opacity);
		if (numbered)
		{
			if (orderNumberDigits.getText().isEmpty())
			{
				orderNumberDigits.setText(WzString::fromUtf8("0123456789"), font_regular_bold);
			}
			WzText &text = orderNumbers[look.number];
			if (text.getText().isEmpty())
			{
				text.setText(WzString::number(look.number), font_regular_bold);
			}
			text.renderCenteredOnInk(rect, ink, &orderNumberDigits);
			return true;
		}
		// 24 units high, at most ORDER_ICON_HEIGHT px, and narrower than the button by ORDER_ICON_MARGIN on each side
		const float aspect = gameVectorImageAspect(files.outline);
		const float height = std::min(ORDER_ICON_HEIGHT, (rect.width() - 2 * ORDER_ICON_MARGIN) / aspect);
		const float width = height * aspect;
		const float x = std::round(rect.x() + (rect.width() - width) / 2.f);
		const float y = std::round(rect.y() + (rect.height() - height) / 2.f);
		drawGameVectorImage(files.fill, x, y, width, height, scaledAlpha(toneColor(look.fillTone), opacity), 0.f, true);
		drawGameVectorImage(files.outline, x, y, width, height, ink);
		return true;
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
		pie_DrawRoundedBox(x0, y0, x1, y1, withAlpha(tints.popoverFill, 240), params.cardRadius + 1.f, withAlpha(palette.panelBorder, 200), 1.f);
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
		                   static_cast<float>(rect.bottom()), filled ? withAlpha(color, 70) : withAlpha(tints.fieldFill, 200), radius,
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

	/// Pointing left (previous) or right, `outward` px from the middle in the direction it points
	static void drawChevron(float x0, float y0, float x1, float y1, bool previous, PIELIGHT color, float outward)
	{
		const float lean = previous ? -1.f : 1.f;
		const float cx = (x0 + x1) / 2.f + lean * outward;
		const float cy = (y0 + y1) / 2.f;
		const float arm = std::min(x1 - x0, y1 - y0) * 0.6f;
		pie_DrawSoftRoundedBoxRotated(cx, cy - arm * 0.35f, arm, 1.8f, lean * PI / 4.f, color, 0.9f, 0.9f);
		pie_DrawSoftRoundedBoxRotated(cx, cy + arm * 0.35f, arm, 1.8f, -lean * PI / 4.f, color, 0.9f, 0.9f);
	}

	Params params;
	HudColors palette = modernColors();
	HudMetrics sizes = {6.f, 3.f, 1.f, 6, font_regular_bold, font_small, font_small};
	ModernTints tints;
	HudAccent accentInUse = HudAccent::Blue;
	HudPanelTint tintInUse = HudPanelTint::Full;
	PIELIGHT powerFillFrom = pal_RGBA(60, 150, 230, 235);
	PIELIGHT powerFillTo = pal_RGBA(90, 205, 255, 235);

public:
	void releaseCaches() override
	{
		powerText = WzText();
		countText = WzText();
		countDigits = WzText();
		costText = WzText();
		needText = WzText();
		bangText = WzText();
		orderNumbers = {};
		replaySpeed = WzText();
		orderNumberDigits = WzText();
		hudReleaseRanks();
		hudReleaseRadarBlips();
	}

private:
	WzText powerText;
	WzText countText;
	WzText countDigits;
	WzText costText;
	WzText needText;
	WzText bangText;
	std::array<WzText, 10> orderNumbers;
	WzText replaySpeed;
	WzText orderNumberDigits;
};

}

HudSkin &modernHudSkin()
{
	static ModernHudSkin skin;
	skin.useAccent(hudSettings().accent, hudSettings().panelTint);
	return skin;
}
