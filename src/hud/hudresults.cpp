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

#include "hudresults.h"

#include "hudchart.h"
#include "hudsettings.h"
#include "hudskin.h"
#include "intel/intelkit.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/widget/form.h"

#include "../mission.h"
#include "../objectives.h"
#include "../scores.h"

#include <algorithm>
#include <cmath>

using namespace intelkit;

bool hudResultsEnabled()
{
	return currentHudSkin().drawsResultsScreens();
}

bool hudResultsCompact()
{
	return screenWidth < 800 || screenHeight < 520;
}

std::string hudCappedText(const std::string &text, iV_fonts font, int maxWidth)
{
	return iV_TruncateTextToWidth(WzString::fromUtf8(text), font, std::max(maxWidth, 1)).toUtf8();
}

namespace
{

struct DisplayFont
{
	iV_fonts font;
	float size;
	float compactSize;
	float letterSpacing;   ///< (in ems)
};
const DisplayFont DISPLAY_FONTS[] = {
	{RESULTS_FIGURE_FONT, 26.f, 18.f, 0.01f},
	{RESULTS_LABEL_FONT, 11.f, 9.f, 0.16f},
	{RESULTS_SUBTITLE_FONT, 13.f, 9.5f, 0.15f},
	{RESULTS_TAB_FONT, 14.f, 11.f, 0.1f},
	{RESULTS_RAIL_FONT, 11.f, 10.f, 0.04f},
};
/// DejaVu's capitals' height, in ems
constexpr float CAP_HEIGHT = 0.729f;

float titleMostSize(bool compact)
{
	return compact ? 24.f : 44.f;
}

}

float hudResultsCapHeight(iV_fonts font, bool compact)
{
	for (const DisplayFont &f : DISPLAY_FONTS)
	{
		if (f.font == font)
		{
			return (compact ? f.compactSize : f.size) * CAP_HEIGHT;
		}
	}
	return static_cast<float>(iV_GetTextAboveBase(font));
}

std::string hudResultsCaps(const std::string &text)
{
	return WzString::fromUtf8(text).toUpper().toUtf8();
}

/// When the screen was first drawn whole
static nonstd::optional<uint32_t> resultsShownAt;

float hudResultsAppear(float delayMs, float durationMs)
{
	return resultsShownAt ? hudAppearSince(*resultsShownAt, delayMs, durationMs) : (hudReduceMotion() ? 1.f : 0.f);
}

PIELIGHT hudResultsSurface(PIELIGHT color)
{
	return currentHudSkin().followAccent(color, hudSettings().panelTint == HudPanelTint::None ? 0.f : 0.5f);
}

PIELIGHT hudResultsChosen(PIELIGHT color)
{
	return currentHudSkin().followAccent(color, 1.f);
}

PIELIGHT hudResultsNeutral(PIELIGHT color)
{
	return currentHudSkin().followAccent(color, 0.f);
}

namespace
{

/// The whole screen, dimmed, with the sheet in its middle (it takes the clicks, as the Classic screen's form)
class ResultsRoot : public W_FORM
{
public:
	/// `make` makes the sheet: again when the screen is resized across the compact layout's threshold
	void initialize(std::function<std::shared_ptr<HudResultsSheet> ()> make, uint8_t dimAlpha)
	{
		makeSheet = std::move(make);
		sheet = makeSheet();
		dimAmount = dimAlpha;
		attach(sheet);
		setCalcLayout([](WIDGET *psWidget) {
			psWidget->setGeometry(0, 0, static_cast<int>(screenWidth), static_cast<int>(screenHeight));
		});
	}

	void geometryChanged() override
	{
		if (sheet && sheet->isCompact() != hudResultsCompact())
		{
			detach(sheet);
			sheet = makeSheet();
			attach(sheet);
		}
		if (sheet)
		{
			sheet->setGeometry(sheet->sheetRect(width(), height()));
		}
	}

	void display(int xOffset, int yOffset) override
	{
		if (++framesDrawn == 2)
		{
			resultsShownAt = realTime;
		}
		pie_UniTransBoxFill(static_cast<float>(xOffset + x()), static_cast<float>(yOffset + y()), static_cast<float>(xOffset + x() + width()),
		                    static_cast<float>(yOffset + y() + height()), pal_RGBA(4, 6, 9, static_cast<uint8_t>(dimAmount * hudResultsAppear(0.f, 300.f))));
	}

private:
	std::function<std::shared_ptr<HudResultsSheet> ()> makeSheet;
	std::shared_ptr<HudResultsSheet> sheet;
	uint8_t dimAmount = 150;
	int framesDrawn = 0;
};

std::string upper(const std::string &text)
{
	return hudResultsCaps(text);
}

void drawSmallCaps(WzText &text, const std::string &label, int x, int top, PIELIGHT color)
{
	text.setText(WzString::fromUtf8(upper(label)), RESULTS_LABEL_FONT);
	text.render(static_cast<float>(x), baselineAt(text, top), color);
}

}

static std::weak_ptr<WIDGET> resultsRoot;

std::shared_ptr<WIDGET> hudMakeResultsRoot(std::function<std::shared_ptr<HudResultsSheet> ()> makeSheet, uint8_t dim)
{
	resultsShownAt = nonstd::nullopt;
	auto root = std::make_shared<ResultsRoot>();
	root->initialize(std::move(makeSheet), dim);
	resultsRoot = root;
	return root;
}

std::shared_ptr<WIDGET> hudResultsRoot()
{
	auto root = resultsRoot.lock();
	return root && root->parent() ? root : nullptr;
}

// ---------------------------------------------------------------- the sheet

namespace
{

/// A results button: its label centered, the primary one in the accent's shades
class ResultsButton : public W_BUTTON
{
public:
	bool primary = false;
	bool small = false;

	int32_t idealWidth() override
	{
		return static_cast<int32_t>(iV_GetTextWidth(WzString::fromUtf8(hudResultsCaps(label)), RESULTS_TAB_FONT)) + (small ? 24 : 44);
	}

	void display(int xOffset, int yOffset) override
	{
		const float x0 = static_cast<float>(xOffset + x());
		const float y0 = static_cast<float>(yOffset + y());
		const bool hover = isMouseOverWidget();
		const bool down = (getState() & WBUT_DOWN) != 0;
		PIELIGHT fill = primary ? pal_RGBA(29, 79, 122, 255) : pal_RGBA(24, 32, 43, 255);
		PIELIGHT border = primary ? pal_RGBA(61, 138, 208, 255) : pal_RGBA(52, 66, 87, 255);
		if (hover)
		{
			fill = primary ? pal_RGBA(36, 96, 146, 255) : pal_RGBA(32, 42, 56, 255);
			border = primary ? pal_RGBA(88, 162, 230, 255) : pal_RGBA(74, 90, 114, 255);
		}
		if (down)
		{
			fill = primary ? pal_RGBA(24, 66, 104, 255) : pal_RGBA(20, 27, 36, 255);
		}
		auto follow = primary ? hudResultsChosen : hudResultsSurface;
		pie_DrawRoundedBox(x0, y0, x0 + width(), y0 + height(), follow(fill), 5.f, follow(border), 1.f);
		text.setText(WzString::fromUtf8(hudResultsCaps(label)), RESULTS_TAB_FONT);
		capitals.setText(WzString::fromUtf8("HIJKLMNOPQ"), RESULTS_TAB_FONT); // (centered on its capitals, like the HUD's names)
		text.renderCenteredOnInk(WzRect(static_cast<int>(x0), static_cast<int>(y0), width(), height() - 1), currentHudSkin().colors().textPrimary, &capitals);
	}

	std::string label;

private:
	WzText text;
	WzText capitals;
};

}

void HudResultsSheet::setHeader(const std::string &icon, PIELIGHT color, const std::string &newEyebrow, const std::string &newTitle, std::vector<std::string> newSubtitle,
                                bool tinted)
{
	iconName = icon;
	accent = color;
	bandTinted = tinted;
	eyebrow = newEyebrow;
	title = newTitle;
	subtitle = std::move(newSubtitle);
	fittedFor = -1;
	eyebrowText.setText(WzString::fromUtf8(hudResultsCaps(eyebrow)), RESULTS_LABEL_FONT);
	fitTitle();
	layoutSubtitle();
}

int HudResultsSheet::headerH() const
{
	if (compact)
	{
		return 48;
	}
	// More room around the title on a tall sheet: up to 24 px as it goes from 568 to 664
	return 103 + std::max(0, std::min(24, (height() - 568) / 4));
}

void HudResultsSheet::bindFonts()
{
	for (const DisplayFont &f : DISPLAY_FONTS)
	{
		iV_BindDisplayFont(f.font, compact ? f.compactSize : f.size, f.letterSpacing);
	}
	titleSize = titleMostSize(compact);
	iV_BindDisplayFont(RESULTS_TITLE_FONT, titleSize, 0.03f);
}

void HudResultsSheet::fitTitle()
{
	const int availableWidth = std::max(40, availableTitleWidth());
	if (availableWidth == fittedFor)
	{
		return;
	}
	fittedFor = availableWidth;
	const float most = titleMostSize(compact);
	const float least = compact ? 16.f : 26.f;
	const WzString caps = WzString::fromUtf8(hudResultsCaps(title));
	titleSize = most;
	iV_BindDisplayFont(RESULTS_TITLE_FONT, titleSize, 0.03f);
	int w = static_cast<int>(iV_GetTextWidth(caps, RESULTS_TITLE_FONT));
	if (w > availableWidth)
	{
		// (first in proportion to its width, then down a point at a time)
		titleSize = std::max(least, std::floor(most * availableWidth / w));
		iV_BindDisplayFont(RESULTS_TITLE_FONT, titleSize, 0.03f);
		w = static_cast<int>(iV_GetTextWidth(caps, RESULTS_TITLE_FONT));
		while (w > availableWidth && titleSize > least)
		{
			titleSize = std::max(least, titleSize - 1.f);
			iV_BindDisplayFont(RESULTS_TITLE_FONT, titleSize, 0.03f);
			w = static_cast<int>(iV_GetTextWidth(caps, RESULTS_TITLE_FONT));
		}
	}
	titleText.setText(WzString::fromUtf8(hudCappedText(caps.toUtf8(), RESULTS_TITLE_FONT, availableWidth)), RESULTS_TITLE_FONT);
}

void HudResultsSheet::layoutSubtitle()
{
	const int availableWidth = std::max(40, availableSubtitleWidth());
	std::vector<std::string> subtitleShown;
	for (const std::string &part : subtitle)
	{
		subtitleShown.push_back(hudResultsCaps(part));
	}
	auto widthOf = [](const std::string &text) { return static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(text), RESULTS_SUBTITLE_FONT)); };
	int total = subtitleFullWidth();
	if (total > availableWidth && !subtitleShown.empty())
	{
		// The first part gives way (ex. who won, with long names)
		const int others = total - widthOf(subtitleShown[0]);
		subtitleShown[0] = hudCappedText(subtitleShown[0], RESULTS_SUBTITLE_FONT, std::max(40, availableWidth - others));
		total = others + widthOf(subtitleShown[0]);
	}
	subtitleWidth = std::min(total, availableWidth);
	subtitleTexts.resize(subtitleShown.size());
	for (size_t k = 0; k < subtitleShown.size(); ++k)
	{
		subtitleTexts[k].setText(WzString::fromUtf8(subtitleShown[k]), RESULTS_SUBTITLE_FONT);
	}
	slashText.setText(WzString::fromUtf8(" / "), RESULTS_SUBTITLE_FONT);
}

int HudResultsSheet::subtitleFullWidth() const
{
	const int slashW = static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(" / "), RESULTS_SUBTITLE_FONT));
	int total = 0;
	for (size_t k = 0; k < subtitle.size(); ++k)
	{
		total += static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(hudResultsCaps(subtitle[k])), RESULTS_SUBTITLE_FONT)) + (k > 0 ? slashW : 0);
	}
	return total;
}

HudResultsSheet::HeaderLines HudResultsSheet::headerLines() const
{
	// The eyebrow, the title and the subtitle, by their capitals' height, centered together
	HeaderLines lines;
	const bool withEyebrow = !compact && !eyebrow.empty();
	const float eyeCap = withEyebrow ? hudResultsCapHeight(RESULTS_LABEL_FONT, compact) : 0.f;
	const float eyebrowGap = withEyebrow ? 8.f : 0.f;
	const float titleCap = titleSize * CAP_HEIGHT;
	const float subtitleGap = compact ? 6.f : 10.f;
	const float subCap = subtitle.empty() ? 0.f : hudResultsCapHeight(RESULTS_SUBTITLE_FONT, compact);
	const float total = eyeCap + eyebrowGap + titleCap + (subtitle.empty() ? 0.f : subtitleGap + subCap);
	const float top = headerCenterY() - total / 2.f;
	lines.eyebrowBase = top + eyeCap;
	lines.titleBase = top + eyeCap + eyebrowGap + titleCap;
	lines.subtitleBase = lines.titleBase + subtitleGap + subCap;
	lines.subtitleCap = subCap;
	lines.textX = padW() + badgeSize() + (compact ? 10 : 16);
	return lines;
}

namespace
{

/// A link: its text in the accent's color, underlined on hover
class ResultsLink : public W_BUTTON
{
public:
	std::string label;

	int32_t idealWidth() override
	{
		return static_cast<int32_t>(iV_GetTextWidth(label.c_str(), font_small)) + 4;
	}

	void display(int xOffset, int yOffset) override
	{
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		text.setText(WzString::fromUtf8(label), font_small);
		const PIELIGHT color = isMouseOverWidget() ? currentHudSkin().colors().textPrimary : currentHudSkin().colors().accent;
		const float base = baselineAt(text, y0 + (height() - static_cast<int>(text.lineSize())) / 2);
		text.render(static_cast<float>(x0 + 2), base, color);
		if (isMouseOverWidget())
		{
			pie_UniTransBoxFill(static_cast<float>(x0 + 2), base + 2.f, static_cast<float>(x0 + 2 + text.width()), base + 3.f, color);
		}
	}

private:
	WzText text;
};

/// An area with a tooltip, drawing nothing
class TipArea : public W_BUTTON
{
public:
	void display(int, int) override {}
};

}

void HudResultsSheet::setFooterNote(const std::string &icon, const std::string &text, const std::string &link, std::function<void ()> onLink)
{
	footerIcon = icon;
	footerText = text;
	if (footerLink)
	{
		detach(footerLink);
		footerLink.reset();
	}
	if (onLink)
	{
		auto l = std::make_shared<ResultsLink>();
		l->label = link;
		l->addOnClickHandler([onLink](W_BUTTON &) { widgScheduleTask(onLink); });
		attach(l);
		footerLink = l;
	}
	layoutButtons();
}

void HudResultsSheet::setSubtitleTip(const std::string &tip)
{
	if (!subtitleTip)
	{
		subtitleTip = std::make_shared<TipArea>();
		attach(subtitleTip);
	}
	subtitleTip->setTip(tip);
	layoutButtons();
}

void HudResultsSheet::setButtons(std::vector<HudResultsButton> list)
{
	for (auto &b : buttons)
	{
		detach(b);
	}
	buttons.clear();
	for (HudResultsButton &b : list)
	{
		auto button = std::make_shared<ResultsButton>();
		button->label = b.label;
		button->primary = b.primary;
		button->small = compact;
		button->id = b.id;
		if (b.onClick)
		{
			auto onClick = b.onClick;
			button->addOnClickHandler([onClick](W_BUTTON &) {
				widgScheduleTask(onClick);
			});
		}
		attach(button);
		buttons.push_back(button);
	}
	layoutButtons();
}

void HudResultsSheet::geometryChanged()
{
	fitTitle();
	layoutSubtitle();
	layoutButtons();
}

void HudResultsSheet::layoutButtons()
{
	const int h = compact ? 28 : 34;
	int right = width() - (compact ? 8 : 16);
	for (auto it = buttons.rbegin(); it != buttons.rend(); ++it)
	{
		const int w = std::max(compact ? 110 : 160, (*it)->idealWidth());
		(*it)->setGeometry(right - w, height() - footerH() + (footerH() - h) / 2, w, h);
		right -= w + (compact ? 6 : 10);
	}
	if (footerLink)
	{
		const int lw = footerLink->idealWidth();
		const int lx = padW() + footerNoteWidth() - lw;
		footerLink->setGeometry(lx, height() - footerH() / 2 - 10, lw, 20);
		footerLink->show(!footerText.empty() && footerNoteFits());
	}
	if (subtitleTip)
	{
		// (over the subtitle)
		const HeaderLines lines = headerLines();
		const int top = static_cast<int>(lines.subtitleBase - lines.subtitleCap) - 4;
		subtitleTip->setGeometry(lines.textX, top, std::max(1, subtitleWidth), static_cast<int>(lines.subtitleCap) + 8);
	}
}

int HudResultsSheet::availableTitleWidth() const
{
	const WzRect rect = sheetRect(static_cast<int>(screenWidth), static_cast<int>(screenHeight));
	return rect.width() - padW() * 2 - badgeSize() - (compact ? 10 : 16) - headerRightWidth() - (compact ? 12 : 24);
}

int HudResultsSheet::availableHeaderRightWidth() const
{
	// (the title's full width, scaled from its width at the size it's bound at)
	const WzRect rect = sheetRect(static_cast<int>(screenWidth), static_cast<int>(screenHeight));
	const float titleW = iV_GetTextWidth(WzString::fromUtf8(hudResultsCaps(title)), RESULTS_TITLE_FONT) * titleMostSize(compact) / std::max(1.f, titleSize);
	const int need = std::max(static_cast<int>(std::ceil(titleW)), subtitleFullWidth());
	return rect.width() - padW() * 2 - badgeSize() - (compact ? 10 : 16) - need - (compact ? 12 : 24);
}

int HudResultsSheet::availableSubtitleWidth() const
{
	// (The header's right side, a label over a figure, is centered vertically: the subtitle may only run under it with
	// some room between)
	const HeaderLines lines = headerLines();
	const float rightHalf = (hudResultsCapHeight(RESULTS_LABEL_FONT, compact) + (compact ? 4.f : 8.f) + hudResultsCapHeight(RESULTS_FIGURE_FONT, compact)) / 2.f;
	if (lines.subtitleBase - lines.subtitleCap < headerCenterY() + rightHalf + 12.f)
	{
		return availableTitleWidth();
	}
	const WzRect rect = sheetRect(static_cast<int>(screenWidth), static_cast<int>(screenHeight));
	return rect.width() - padW() * 2 - badgeSize() - (compact ? 10 : 16);
}

WzRect HudResultsSheet::contentRect() const
{
	const int top = headerH() + headerExtra();
	return WzRect(0, top, width(), height() - top - footerH());
}

void HudResultsSheet::display(int xOffset, int yOffset)
{
	HudSkin &hudSkin = currentHudSkin();
	const HudColors &colors = hudSkin.colors();
	const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
	const float x0 = static_cast<float>(rect.x());
	const float y0 = static_cast<float>(rect.y());
	const float x1 = static_cast<float>(rect.right());
	const float y1 = static_cast<float>(rect.bottom());
	// The content's surface, then the header: a lighter band (neutral, or a dark tint of the result's color), washed from
	// the left in the result's color, with an edge in that color along its top. Below it, the tabs' bar is sunk in and the
	// footer is darker, so their shades separate them without lines
	const int bandH = headerH();
	const PIELIGHT neutralBand = hudResultsNeutral(pal_RGBA(20, 26, 35, 255));
	const PIELIGHT bandBase = bandTinted ? hudSkin.inHueOf(neutralBand, accent, 0.02f) : neutralBand;
	pie_DrawRoundedBox(x0, y0, x1, y1, hudResultsSurface(pal_RGBA(14, 18, 24, 250)), 8.f);
	pie_DrawRoundedBoxClipped(x0, y0, x1, y1, bandBase, 8.f, pal_RGBA(0, 0, 0, 0), 0.f, WzRect(rect.x(), rect.y(), rect.width(), bandH));
	PIELIGHT wash = accent;
	wash.byte.a = static_cast<uint8_t>(72 * hudResultsAppear(120.f, 700.f));
	PIELIGHT washEnd = wash;
	washEnd.byte.a = 0;
	pie_DrawGradientRoundedBox(x0, y0, x0 + rect.width() * 0.62f, y0 + bandH, wash, washEnd, true, 1.6f, glm::vec4(8.f, 0.f, 0.f, 0.f), pal_RGBA(0, 0, 0, 0), 0.f);
	if (headerExtra() > 0)
	{
		const float ty = y0 + bandH;
		const float by = ty + headerExtra();
		pie_UniTransBoxFill(x0, ty, x1, by, hudResultsSurface(pal_RGBA(10, 13, 18, 255)));
		pie_UniTransBoxFill(x0, ty, x1, ty + 1.f, pal_RGBA(0, 0, 0, 150));
		pie_UniTransBoxFill(x0, by - 1.f, x1, by, pal_RGBA(255, 255, 255, 12));
	}
	pie_DrawRoundedBoxClipped(x0, y0, x1, y1, hudResultsSurface(pal_RGBA(11, 14, 19, 255)), 8.f, pal_RGBA(0, 0, 0, 0), 0.f, WzRect(rect.x(), rect.bottom() - footerH(), rect.width(), footerH()));
	pie_DrawRoundedBox(x0, y0, x1, y1, pal_RGBA(0, 0, 0, 0), 8.f, hudResultsSurface(pal_RGBA(38, 48, 64, 255)), 1.f);
	// The edge, drawn in from the left, its leading end lit while it runs
	const float edgeIn = hudResultsAppear(40.f, 460.f);
	if (edgeIn > 0.f)
	{
		const int edgeW = static_cast<int>(std::ceil(rect.width() * edgeIn));
		pie_DrawRoundedBoxClipped(x0, y0, x1, y1, accent, 8.f, pal_RGBA(0, 0, 0, 0), 0.f, WzRect(rect.x(), rect.y(), edgeW, edgeH()));
		if (edgeIn < 1.f)
		{
			const float head = x0 + edgeW;
			PIELIGHT lit = pal_RGBA(255, 255, 255, static_cast<uint8_t>(170 * (1.f - edgeIn)));
			PIELIGHT clear = lit;
			clear.byte.a = 0;
			pie_DrawGradientRoundedBoxClipped(head - 56.f, y0, head, y0 + edgeH(), clear, lit, true, 1.f, glm::vec4(0.f), pal_RGBA(0, 0, 0, 0), 0.f,
			                                  WzRect(rect.x(), rect.y(), edgeW, edgeH()));
		}
	}

	// The badge (growing in, with a ring going out once), then the eyebrow, the title and the subtitle rising in one after
	// another
	const HeaderLines lines = headerLines();
	const int cy = rect.y() + headerCenterY();
	const int badge = badgeSize();
	const float bx = static_cast<float>(rect.x() + padW());
	const float bcx = bx + badge / 2.f;
	const float badgeIn = hudResultsAppear(140.f, 340.f);
	const float pulse = hudResultsAppear(380.f, 900.f);
	if (pulse > 0.f && pulse < 1.f)
	{
		const float pr = badge / 2.f * (1.f + 0.6f * pulse);
		pie_DrawRoundedBox(bcx - pr, cy - pr, bcx + pr, cy + pr, pal_RGBA(0, 0, 0, 0), pr, alpha(accent, static_cast<uint8_t>(150 * (1.f - pulse))), 2.f);
	}
	const float br = badge / 2.f * (0.7f + 0.3f * badgeIn);
	pie_DrawRoundedBox(bcx - br, cy - br, bcx + br, cy + br, pal_RGBA(0, 0, 0, static_cast<uint8_t>(56 * badgeIn)), br,
	                   alpha(accent, static_cast<uint8_t>(170 * badgeIn)), compact ? 1.f : 2.f);
	const int iconSize = static_cast<int>(std::lround((compact ? 18 : 26) * (0.7f + 0.3f * badgeIn)));
	hudSkin.drawNamedIcon(iconName, WzRect(static_cast<int>(std::lround(bcx)) - iconSize / 2, cy - iconSize / 2, iconSize, iconSize), alpha(accent, static_cast<uint8_t>(255 * badgeIn)));
	const float tx = static_cast<float>(rect.x() + lines.textX);
	auto rise = [](float t) { return 6.f * (1.f - t); };
	if (!compact && !eyebrow.empty())
	{
		// (the result's color, lighter)
		const float in = hudResultsAppear(160.f, 320.f);
		PIELIGHT light = accent;
		light.byte.r = static_cast<uint8_t>(accent.byte.r + (255 - accent.byte.r) * 0.45f);
		light.byte.g = static_cast<uint8_t>(accent.byte.g + (255 - accent.byte.g) * 0.45f);
		light.byte.b = static_cast<uint8_t>(accent.byte.b + (255 - accent.byte.b) * 0.45f);
		eyebrowText.render(tx, rect.y() + lines.eyebrowBase + rise(in), alpha(light, static_cast<uint8_t>(235 * in)));
	}
	const float titleIn = hudResultsAppear(200.f, 340.f);
	titleText.render(tx, rect.y() + lines.titleBase + rise(titleIn), alpha(colors.textPrimary, static_cast<uint8_t>(255 * titleIn)));
	const float subIn = hudResultsAppear(260.f, 340.f);
	float sx = tx;
	for (size_t k = 0; k < subtitleTexts.size(); ++k)
	{
		if (k > 0)
		{
			slashText.render(sx, rect.y() + lines.subtitleBase + rise(subIn), alpha(colors.textSecondary, static_cast<uint8_t>(150 * subIn)));
			sx += slashText.width();
		}
		subtitleTexts[k].render(sx, rect.y() + lines.subtitleBase + rise(subIn), alpha(colors.textPrimary, static_cast<uint8_t>(205 * subIn)));
		sx += subtitleTexts[k].width();
	}
	displayHeaderRight(rect.right() - padW(), cy);

	displayContent(rect.x(), rect.y());

	// The footer: its note (an icon, the text, and its link as a child) and (as children) the buttons
	if (!footerText.empty() && footerNoteFits())
	{
		int nx = rect.x() + padW();
		const int fy = rect.bottom() - footerH() / 2;
		if (!footerIcon.empty())
		{
			hudSkin.drawNamedIcon(footerIcon, WzRect(nx, fy - 7, 14, 14), alpha(colors.textSecondary, 170));
			nx += 20;
		}
		footerTextCache.setText(WzString::fromUtf8(footerText), font_small);
		// (centered as the link's text, in the same 20 px)
		footerTextCache.render(static_cast<float>(nx), baselineAt(footerTextCache, fy - 10 + (20 - static_cast<int>(footerTextCache.lineSize())) / 2), alpha(colors.textSecondary, 190));
	}
}

int HudResultsSheet::footerNoteWidth() const
{
	return (footerIcon.empty() ? 0 : 20) + static_cast<int>(iV_GetTextWidth(footerText.c_str(), font_small)) + (footerLink ? 10 + footerLink->idealWidth() : 0);
}

bool HudResultsSheet::footerNoteFits() const
{
	int buttonsLeft = width();
	for (const auto &b : buttons)
	{
		buttonsLeft = std::min(buttonsLeft, b->x());
	}
	return padW() + footerNoteWidth() + 12 < buttonsLeft;
}

// ---------------------------------------------------------------- the campaign's mission results

namespace
{

const PIELIGHT KILL_COLOR = pal_RGBA(92, 196, 122, 255);
const PIELIGHT LOSS_COLOR = pal_RGBA(224, 96, 78, 255);
const PIELIGHT RANK_COLOR = pal_RGBA(226, 178, 62, 255);

class CampaignSheet : public HudResultsSheet
{
public:
	/// (`endStats` and `endObjectives` are as the mission ended, so a sheet made again shows the same)
	void initialize(bool missionWon, const END_GAME_STATS_DATA &endStats, const std::vector<Objective> &endObjectives)
	{
		won = missionWon;
		compact = hudResultsCompact();
		bindFonts();
		stats = endStats;
		objectives = endObjectives;
		const HudColors &colors = currentHudSkin().colors();
		char missionTime[32];
		getAsciiTime(missionTime, stats.missionData.missionEnded);
		std::vector<std::string> sub;
		if (!objectivesTitle().empty())
		{
			sub.push_back(objectivesTitle());
		}
		sub.push_back(astringf(_("Mission time %s"), missionTime));
		setHeader(won ? "objective-done" : "objective-failed", won ? colors.stateOk : colors.stateBad, _("Mission result"),
		          won ? _("Success") : _("Failed"), sub);
		for (const Objective &o : objectives)
		{
			if (o.options.state == ObjectiveState::Failed)
			{
				failedObjective = o.text;
				break;
			}
		}
	}

	WzRect sheetRect(int screenW, int screenH) const override
	{
		const int margin = compact ? 4 : 12;
		const int w = std::min(900, screenW - margin * 2);
		// (taller by what the header grew, so the content keeps its height)
		const int h = std::min((won ? 480 : 520) + (compact ? 2 : 21), screenH - margin * 2);
		return WzRect((screenW - w) / 2, (screenH - h) / 2, w, h);
	}

protected:
	std::string totalLabel() const { return upper(compact ? _("Total") : _("Total game time")); }
	std::string totalValue() const
	{
		char total[32];
		getAsciiTime(total, gameTime);
		return total;
	}

	int headerRightWidth() const override
	{
		const int w = std::max(static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(totalLabel()), RESULTS_LABEL_FONT)),
		                       static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(totalValue()), RESULTS_FIGURE_FONT)));
		return w + (Cheated && !compact ? 16 + tagWidth(_("Cheated")) : 0);
	}

	void displayHeaderRight(int right, int cy) override
	{
		// The total game time: its label over it, centered together on their capitals
		const HudColors &colors = currentHudSkin().colors();
		const float in = hudResultsAppear(300.f, 320.f);
		const float labelCap = hudResultsCapHeight(RESULTS_LABEL_FONT, compact);
		const float valueCap = hudResultsCapHeight(RESULTS_FIGURE_FONT, compact);
		const float gap = compact ? 4.f : 7.f;
		const float top = cy - (labelCap + gap + valueCap) / 2.f + 6.f * (1.f - in);
		labelText.setText(WzString::fromUtf8(totalLabel()), RESULTS_LABEL_FONT);
		valueText.setText(WzString::fromUtf8(totalValue()), RESULTS_FIGURE_FONT);
		labelText.render(static_cast<float>(right - labelText.width()), top + labelCap, alpha(colors.textSecondary, static_cast<uint8_t>(200 * in)));
		valueText.render(static_cast<float>(right - valueText.width()), top + labelCap + gap + valueCap, alpha(colors.textPrimary, static_cast<uint8_t>(255 * in)));
		if (Cheated && !compact && in > 0.f)
		{
			drawTag(tagText, _("Cheated"), right - std::max(labelText.width(), valueText.width()) - 16 - tagWidth(tagText, _("Cheated")), cy, colors.stateWarn, false);
		}
	}

	void displayContent(int x0, int y0) override
	{
		const HudColors &colors = currentHudSkin().colors();
		const WzRect content = contentRect();
		const int left = x0 + padW();
		const int top = y0 + content.y() + (compact ? 8 : 18);
		const int gutter = compact ? 21 : 29;
		const int rightW = std::min(380, (width() - padW() * 2) / 2 - 14);
		const int leftW = width() - padW() * 2 - rightW - gutter;
		const int rightX = left + leftW + gutter;
		const int bottom = y0 + content.bottom() - (compact ? 8 : 16);
		drawDivider(rightX - gutter / 2 - 1, top, rightX - gutter / 2, bottom);

		// Left: the objectives (a failure's line first), then artifacts and accuracy
		size_t t = 0;
		drawSmallCaps(texts[t++], _("Objectives"), left, top, alpha(colors.textSecondary, 170));
		int y = top + (compact ? 18 : 22);
		if (!won)
		{
			const int boxH = compact ? 32 : 54;
			pie_DrawRoundedBox(static_cast<float>(left), static_cast<float>(y), static_cast<float>(left + leftW), static_cast<float>(y + boxH),
			                   pal_RGBA(42, 22, 24, 255), 5.f, pal_RGBA(90, 42, 42, 255), 1.f);
			currentHudSkin().drawNamedIcon("objective-failed", WzRect(left + 12, y + 10, 16, 16), colors.stateBad);
			const std::string reason = failedObjective.empty() ? std::string(_("The mission was lost")) : astringf(_("Failed: %s"), failedObjective.c_str());
			drawLine(lines[0], reason, compact ? font_small : font_regular_bold, left + 36, y + (compact ? 9 : 9), leftW - 48, colors.textPrimary);
			if (!compact)
			{
				drawLine(lines[1], _("Load a saved game to try again from before this point."), font_small, left + 36, y + 31, leftW - 48, colors.textSecondary);
			}
			y += boxH + (compact ? 4 : 10);
		}
		const int rowH = compact ? 22 : 32;
		const int tilesH = compact ? 18 : 66;
		const iV_fonts rowFont = compact ? font_small : font_regular;
		const int maxRows = std::max(1, (bottom - tilesH - (compact ? 4 : 12) - y) / rowH);
		size_t shown = 0;
		for (const Objective &o : objectives)
		{
			if (static_cast<int>(shown) >= maxRows || shown + 2 >= 14)
			{
				break;
			}
			const char *icon = o.options.state == ObjectiveState::Done ? "objective-done" : (o.options.state == ObjectiveState::Failed ? "objective-failed" : (o.options.primary ? "objective-active" : "objective-secondary"));
			const PIELIGHT iconColor = o.options.state == ObjectiveState::Done ? colors.stateOk : (o.options.state == ObjectiveState::Failed ? colors.stateBad : (o.options.primary ? colors.stateWarn : colors.textSecondary));
			currentHudSkin().drawNamedIcon(icon, WzRect(left, y + (rowH - 16) / 2, 16, 16), iconColor);
			int textRight = left + leftW;
			if (o.options.progress)
			{
				texts[t].setText(WzString::fromUtf8(astringf("%d/%d", o.options.progress->first, o.options.progress->second)), rowFont);
				texts[t].render(static_cast<float>(left + leftW - texts[t].width()), baselineAt(texts[t], y + (rowH - static_cast<int>(texts[t].lineSize())) / 2), colors.textSecondary);
				textRight -= texts[t].width() + 12;
				++t;
			}
			drawLine(lines[2 + shown], o.text, rowFont, left + 26, y + (rowH - static_cast<int>(iV_GetTextLineSize(rowFont))) / 2, textRight - left - 26,
			         o.options.state == ObjectiveState::Failed ? pal_RGBA(240, 160, 144, 255) : (o.options.primary ? colors.textPrimary : colors.textSecondary));
			drawDivider(left, y + rowH - 1, left + leftW, y + rowH);
			y += rowH;
			++shown;
		}
		if (objectives.empty())
		{
			drawLine(lines[2], _("No objectives"), font_regular, left, y + 8, leftW, colors.textSecondary);
		}

		// The tiles: artifacts and accuracy
		const int tileY = bottom - tilesH;
		const int tileW = (leftW - 10) / 2;
		const uint32_t shots = stats.missionData.shotsOnTarget + stats.missionData.shotsOffTarget;
		const std::string accuracy = shots ? astringf("%u%%", static_cast<unsigned>(std::lround(100.0 * stats.missionData.shotsOnTarget / shots))) : std::string("—");
		const std::string accuracyOf = shots ? astringf(_("%u hits of %u shots"), stats.missionData.shotsOnTarget, shots) : std::string(_("no shots fired"));
		const struct
		{
			const char *label;
			const char *icon;
			PIELIGHT color;
			std::string value;
			std::string small;
		} tiles[] = {
			{N_("Artifacts recovered"), "technology", colors.stateWarn, std::to_string(stats.missionData.artefactsFound), std::string()},
			{N_("Accuracy"), "accuracy", colors.accent, accuracy, accuracyOf},
		};
		for (int k = 0; k < 2 && compact; ++k)
		{
			// One line: the icon, the value, the label
			const int tx = left + k * (tileW + 10);
			currentHudSkin().drawNamedIcon(tiles[k].icon, WzRect(tx, tileY + 2, 14, 14), tiles[k].color);
			texts[t].setText(WzString::fromUtf8(tiles[k].value), font_regular_bold);
			texts[t].render(static_cast<float>(tx + 20), baselineAt(texts[t], tileY + 1), colors.textPrimary);
			const int sx = tx + 26 + texts[t].width();
			++t;
			drawLine(lines[16 + static_cast<size_t>(k)], k == 0 ? std::string(_("artifacts")) : std::string(_("accuracy")), font_small, sx, tileY + 4, tx + tileW - sx, colors.textSecondary);
		}
		for (int k = 0; k < 2 && !compact; ++k)
		{
			const int tx = left + k * (tileW + 10);
			pie_DrawRoundedBox(static_cast<float>(tx), static_cast<float>(tileY), static_cast<float>(tx + tileW), static_cast<float>(tileY + tilesH),
			                   hudResultsSurface(pal_RGBA(20, 26, 35, 255)), 6.f, hudResultsSurface(pal_RGBA(34, 43, 56, 255)), 1.f);
			drawSmallCaps(texts[t++], gettext(tiles[k].label), tx + 12, tileY + 10, alpha(colors.textSecondary, 170));
			currentHudSkin().drawNamedIcon(tiles[k].icon, WzRect(tx + 12, tileY + 32, 18, 18), tiles[k].color);
			texts[t].setText(WzString::fromUtf8(tiles[k].value), font_medium_bold);
			texts[t].render(static_cast<float>(tx + 38), baselineAt(texts[t], tileY + 28), colors.textPrimary);
			const int sx = tx + 38 + texts[t].width() + 8;
			++t;
			if (!tiles[k].small.empty())
			{
				drawLine(lines[16 + static_cast<size_t>(k)], tiles[k].small, font_small, sx, tileY + 36, tx + tileW - 8 - sx, colors.textSecondary);
			}
		}

		// Right: the fight as exchanges, the force, its ranks
		y = top;
		drawSmallCaps(texts[t++], _("Combat"), rightX, y, alpha(colors.textSecondary, 170));
		y += compact ? 18 : 26;
		const MISSION_DATA &m = stats.missionData;
		const struct
		{
			const char *label;
			uint32_t good;
			uint32_t bad;
		} exchanges[] = {{N_("Units"), m.unitsKilled, m.unitsLost}, {N_("Structures"), m.strKilled, m.strLost}};
		for (const auto &e : exchanges)
		{
			texts[t].setText(WzString::fromUtf8(gettext(e.label)), font_regular);
			texts[t].render(static_cast<float>(rightX), baselineAt(texts[t], y), colors.textSecondary);
			++t;
			// "34 destroyed · 6 lost", right-aligned
			WzText &lostWord = texts[t++];
			WzText &lostNumber = texts[t++];
			WzText &dot = texts[t++];
			WzText &killWord = texts[t++];
			WzText &killNumber = texts[t++];
			lostWord.setText(WzString::fromUtf8(_("lost")), font_small);
			lostNumber.setText(WzString::number(e.bad), font_medium_bold);
			dot.setText(WzString::fromUtf8("·"), font_small);
			killWord.setText(WzString::fromUtf8(_("destroyed")), font_small);
			killNumber.setText(WzString::number(e.good), font_medium_bold);
			const float base = baselineAt(texts[t - 6], y) + 1.f;
			float rx = static_cast<float>(rightX + rightW);
			rx -= lostWord.width();
			lostWord.render(rx, base, colors.textSecondary);
			rx -= 4 + lostNumber.width();
			lostNumber.render(rx, base + 1.f, LOSS_COLOR);
			rx -= 8 + dot.width();
			dot.render(rx, base, colors.textSecondary);
			rx -= 8 + killWord.width();
			killWord.render(rx, base, colors.textSecondary);
			rx -= 4 + killNumber.width();
			killNumber.render(rx, base + 1.f, KILL_COLOR);
			y += compact ? 18 : 22;
			const float total = static_cast<float>(std::max<uint32_t>(1, e.good + e.bad));
			const float split = rightX + (rightW - 2) * (e.bad == 0 && e.good == 0 ? 0.5f : e.good / total);
			pie_DrawRoundedBox(static_cast<float>(rightX), static_cast<float>(y), split, static_cast<float>(y + 8), e.good ? KILL_COLOR : hudResultsSurface(pal_RGBA(30, 39, 51, 255)), glm::vec4(4.f, 0.f, 4.f, 0.f), KILL_COLOR, 0.f);
			pie_DrawRoundedBox(split + 2.f, static_cast<float>(y), static_cast<float>(rightX + rightW), static_cast<float>(y + 8), e.bad ? LOSS_COLOR : pal_RGBA(30, 39, 51, 255), glm::vec4(0.f, 4.f, 0.f, 4.f), LOSS_COLOR, 0.f);
			y += compact ? 16 : 24;
		}

		y += compact ? 2 : 6;
		drawSmallCaps(texts[t++], _("Your force"), rightX, y, alpha(colors.textSecondary, 170));
		y += compact ? 16 : 22;
		const struct
		{
			uint32_t value;
			const char *label;
		} force[] = {{m.unitsBuilt, N_("units built")}, {stats.numUnits, N_("units now")}, {m.strBuilt, N_("structures built")}};
		int fx = rightX;
		for (const auto &f : force)
		{
			texts[t].setText(WzString::number(f.value), compact ? font_regular_bold : font_medium_bold);
			texts[t].render(static_cast<float>(fx), baselineAt(texts[t], y), colors.textPrimary);
			fx += texts[t].width() + (compact ? 3 : 5);
			++t;
			texts[t].setText(WzString::fromUtf8(gettext(f.label)), font_small);
			texts[t].render(static_cast<float>(fx), baselineAt(texts[t], y + (compact ? 2 : 5)), colors.textSecondary);
			fx += texts[t].width() + (compact ? 10 : 18);
			++t;
		}
		y += compact ? 22 : 36;

		// The units by rank: a column each, its count over it and its insignia under it
		drawSmallCaps(texts[t++], _("Units by rank"), rightX, y, alpha(colors.textSecondary, 170));
		y += compact ? 16 : 22;
		uint32_t most = 1;
		for (uint32_t c : stats.numDroidsPerLevel)
		{
			most = std::max(most, c);
		}
		const size_t levels = std::min<size_t>(9, stats.numDroidsPerLevel.size());
		const int colW = rightW / 9;
		const int barH = std::max(compact ? 8 : 20, std::min(48, bottom - y - (compact ? 36 : 40)));
		for (size_t level = 0; level < levels; ++level)
		{
			const uint32_t count = stats.numDroidsPerLevel[level];
			const int cx = rightX + colW * static_cast<int>(level) + colW / 2;
			const bool any = count > 0;
			texts[t].setText(WzString::number(count), font_small);
			texts[t].render(static_cast<float>(cx - texts[t].width() / 2), baselineAt(texts[t], y), any ? colors.textPrimary : alpha(colors.textSecondary, 110));
			++t;
			const int by = y + (compact ? 14 : 16);
			pie_DrawRoundedBox(static_cast<float>(cx - 7), static_cast<float>(by), static_cast<float>(cx + 7), static_cast<float>(by + barH), hudResultsSurface(pal_RGBA(26, 33, 43, 255)), 3.f);
			if (any)
			{
				const float h = std::max(3.f, barH * count / static_cast<float>(most));
				pie_DrawRoundedBox(static_cast<float>(cx - 7), by + barH - h, static_cast<float>(cx + 7), static_cast<float>(by + barH), RANK_COLOR, 3.f);
			}
			if (level > 0)
			{
				currentHudSkin().drawNamedIcon(astringf("rank-%zu", level), WzRect(cx - 7, by + barH + (compact ? 3 : 5), 14, 14), any ? colors.textSecondary : alpha(colors.textSecondary, 90));
			}
		}
	}

private:
	bool won = true;
	END_GAME_STATS_DATA stats;
	std::vector<Objective> objectives;
	std::string failedObjective;
	WzText valueText, labelText, tagText;
	std::array<WzText, 80> texts;
	std::array<WzText, 18> lines;
};

}

std::shared_ptr<WIDGET> hudMakeCampaignResults(bool won, std::vector<HudResultsButton> buttons)
{
	const END_GAME_STATS_DATA stats = collectEndGameStatsData();
	const std::vector<Objective> objectives = selectedPlayer < MAX_PLAYERS ? objectivesFor(selectedPlayer) : std::vector<Objective>();
	return hudMakeResultsRoot([won, buttons, stats, objectives]() {
		auto sheet = std::make_shared<CampaignSheet>();
		sheet->initialize(won, stats, objectives);
		sheet->setButtons(buttons);
		return std::static_pointer_cast<HudResultsSheet>(sheet);
	}, won ? 110 : 150);
}
