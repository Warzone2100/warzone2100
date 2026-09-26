/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2021  Warzone 2100 Project

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
/** @file
 *  The tool tip display system.
 */

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/math_ext.h"
#include "lib/ivis_opengl/screen.h"
#include "widget.h"
#include "widgint.h"
#include "tip.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <string>

/* Time (in milliseconds) after pointing widget required to show the tool tip */
const uint32_t DEFAULT_TIP_PAUSE = 200;
static uint32_t tipPause = DEFAULT_TIP_PAUSE;

/* Time (in milliseconds) required for a tip to be refreshed since the last refresh */
const auto TIP_REFRESH_COOLDOWN = 500;

/* Size of border around tip text */
const auto TIP_HGAP = 6;
const auto TIP_VGAP = 3;

static optional<WzTipStyle> tipStyle;
static std::function<bool (const WIDGET &)> tipSuppressed;

static enum
{
	TIP_INACTIVE,
	TIP_ACTIVE,
	TIP_CLICKED,
} tipState;

struct TipDisplayCache {
	std::vector<WzText> wzTip;
	std::vector<int> wzTipYStart;
};

static int32_t startTime;
static Vector2i lastMouseCoords;
static WzRect tipRect;
static Vector2i textOffset;
static std::string tipText;
static WIDGET *tipSourceWidget;
static PIELIGHT TipColour;
static TipDisplayCache displayCache;
static nonstd::optional<uint32_t> lastRefreshTime;

/* Initialise the tool tip module */
void tipInitialise(void)
{
	tipState = TIP_INACTIVE;
	TipColour = WZCOL_WHITE;
}

void tipShutdown()
{
	displayCache.wzTip.clear();
}

// Set the global toop tip text colour.
void widgSetTipColour(PIELIGHT colour)
{
	TipColour = colour;
}

void widgSetTipStyle(const optional<WzTipStyle> &style)
{
	tipStyle = style;
	tipText.clear();
	lastRefreshTime = nonstd::nullopt;
}

void widgSetTipDelay(uint32_t milliseconds)
{
	tipPause = milliseconds;
}

void widgSetTipSuppressed(const std::function<bool (const WIDGET &)> &predicate)
{
	tipSuppressed = predicate;
}

static std::vector<std::string> splitString(const std::string& str, char delimiter)
{
	std::vector<std::string> strings;
	std::istringstream str_stream(str);
	std::string s;
	while (getline(str_stream, s, delimiter)) {
		strings.push_back(s);
	}
	return strings;
}

static void tipStop()
{
	tipState = TIP_INACTIVE;
	displayCache.wzTip.clear();
	tipText = "";
	startTime = wzGetTicks();
	tipSourceWidget = nullptr;
}

static bool isHighlightedChanged(std::shared_ptr<WIDGET> mouseOverWidget)
{
	return !mouseOverWidget || tipSourceWidget != mouseOverWidget.get();
}

static void refreshTip(std::shared_ptr<WIDGET> mouseOverWidget)
{
	if (isHighlightedChanged(mouseOverWidget))
	{
		tipStop();
		return;
	}

	if (lastRefreshTime.has_value() && wzGetTicks() - lastRefreshTime.value() < TIP_REFRESH_COOLDOWN)
	{
		return;
	}

	auto newTip = (tipSuppressed && tipSuppressed(*mouseOverWidget)) ? std::string() : mouseOverWidget->getTip();
	lastRefreshTime = wzGetTicks();
	if (newTip == tipText)
	{
		return;
	}

	tipText = newTip;
	if (tipText == "")
	{
		return;
	}

	auto fontId = font_regular;
	if (auto lockedScreen = mouseOverWidget->screenPointer.lock()) {
		fontId = lockedScreen->TipFontID;
	}
	if (tipStyle.has_value() && tipStyle->font.has_value())
	{
		fontId = tipStyle->font.value();
	}
	const int hgap = tipStyle.has_value() ? tipStyle->horizontalGap : TIP_HGAP;
	const int vgap = tipStyle.has_value() ? tipStyle->verticalGap : TIP_VGAP;

	auto maxLineWidth = 0;
	auto lines = splitString(tipText, '\n');
	auto totalLineSize = 0;
	displayCache.wzTip.resize(lines.size());
	displayCache.wzTipYStart.resize(lines.size());
	for (size_t n = 0; n < lines.size(); ++n)
	{
		displayCache.wzTip[n].setText(WzString::fromUtf8(lines[n]), fontId);
		maxLineWidth = std::max<int>(maxLineWidth, displayCache.wzTip[n].width());
		displayCache.wzTipYStart[n] = totalLineSize;
		totalLineSize += displayCache.wzTip[n].lineSize();
	}

	/* Position the tip box */
	auto width = maxLineWidth + hgap * 2;
	int32_t height = vgap * 2 + totalLineSize;
	if (!lines.empty())
	{
		height += displayCache.wzTip.back().belowBase();
	}
	auto x = clip<SDWORD>(mouseOverWidget->screenPosX() + mouseOverWidget->width() / 2, 0, screenWidth - width - 1);
	auto y = std::min(mouseOverWidget->screenPosY() - height - vgap, (int)screenHeight);
	if (y < 0)
	{
		/* Position the tip below the button */
		y = std::max(mouseOverWidget->screenPosY() + mouseOverWidget->height() + vgap, 0);
	}
	tipRect = WzRect(x - 1, y - 1, width + 2, height + 2);

	/* Position the text */
	int yOffset = (height - totalLineSize) / 2;
	if (!lines.empty())
	{
		yOffset -=  displayCache.wzTip.front().aboveBase();
	}
	textOffset = Vector2i(
		x + hgap,
		y + yOffset
	);
}

static void handleTipInactive()
{
	auto mouseCoords = Vector2i(mouseX(), mouseY());
	std::shared_ptr<WIDGET> mouseOverWidget;
	if (mouseCoords != lastMouseCoords || isHighlightedChanged(mouseOverWidget = getMouseOverWidget().lock())) {
		lastMouseCoords = mouseCoords;
		startTime = wzGetTicks();
		tipSourceWidget = mouseOverWidget.get();
		return;
	}

	if (wzGetTicks() - startTime > static_cast<int32_t>(tipPause))
	{
		tipState = TIP_ACTIVE;
		lastRefreshTime = nonstd::nullopt;
		refreshTip(mouseOverWidget);
	}
}

static void handleTipActive()
{
	if (mousePressed(MOUSE_LMB))
	{
		tipState = TIP_CLICKED;
		return;
	}

	refreshTip(getMouseOverWidget().lock());

	if (tipText.empty())
	{
		return;
	}

	/* Draw the tool tip */
	if (tipStyle.has_value() && tipStyle->drawFrame)
	{
		tipStyle->drawFrame(tipRect);
	}
	else
	{
		iV_ShadowBox(tipRect.x(), tipRect.y(), tipRect.right(), tipRect.bottom(), 1, WZCOL_FORM_LIGHT, WZCOL_FORM_DARK, WZCOL_FORM_TIP_BACKGROUND);
	}

	const PIELIGHT textColor = (tipStyle.has_value() && tipStyle->textColor.has_value()) ? tipStyle->textColor.value() : TipColour;
	size_t n = 0;
	for (auto &line: displayCache.wzTip)
	{
		line.render(textOffset.x, textOffset.y + displayCache.wzTipYStart[n], textColor);
		++n;
	}
}

static void handleTipClicked()
{
	if (isHighlightedChanged(getMouseOverWidget().lock()))
	{
		tipStop();
	}
}

/* Update and possibly display the tip */
void tipDisplay()
{
	switch (tipState)
	{
	case TIP_INACTIVE: handleTipInactive(); break;
	case TIP_ACTIVE: handleTipActive(); break;
	case TIP_CLICKED: handleTipClicked(); break;
	}
}
