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
const auto TIP_PAUSE = 200;

/* Time (in milliseconds) required for a tip to be refreshed since the last refresh */
const auto TIP_REFRESH_COOLDOWN = 500;

/* Size of border around tip text */
const auto TIP_HGAP = 6;
const auto TIP_VGAP = 3;

static enum
{
	TIP_INACTIVE,
	TIP_ACTIVE,
	TIP_CLICKED,
} tipState;

struct TipDisplayCache {
	std::vector<WzText> wzTip;
};

static int32_t startTime;
static Vector2i lastMouseCoords;
static WzRect tipRect;
static Vector2i textOffset;
static int lineHeight;
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

	auto newTip = mouseOverWidget->getTip();
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
	lineHeight = iV_GetTextLineSize(fontId);

	auto maxLineWidth = 0;
	auto lines = splitString(tipText, '\n');
	displayCache.wzTip.resize(lines.size());
	for (size_t n = 0; n < lines.size(); ++n)
	{
		displayCache.wzTip[n].setText(lines[n], fontId);
		maxLineWidth = std::max<int>(maxLineWidth, displayCache.wzTip[n].width());
	}

	/* Position the tip box */
	auto width = maxLineWidth + TIP_HGAP * 2;
	auto height = TIP_VGAP * 2 + lineHeight * static_cast<int32_t>(lines.size()) + iV_GetTextBelowBase(fontId);
	auto x = clip<SDWORD>(mouseOverWidget->screenPosX() + mouseOverWidget->width() / 2, 0, screenWidth - width - 1);
	auto y = std::max(mouseOverWidget->screenPosY() + mouseOverWidget->height() + TIP_VGAP, 0);
	if (y + height >= (int)screenHeight)
	{
		/* Position the tip above the button */
		y = mouseOverWidget->screenPosY() - height - TIP_VGAP;
	}
	tipRect = WzRect(x - 1, y - 1, width + 2, height + 2);

	/* Position the text */
	textOffset = Vector2i(
		x + TIP_HGAP,
		y + (height - lineHeight * static_cast<int32_t>(lines.size())) / 2 - iV_GetTextAboveBase(fontId)
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

	if (wzGetTicks() - startTime > TIP_PAUSE)
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
	iV_ShadowBox(tipRect.x(), tipRect.y(), tipRect.right(), tipRect.bottom(), 1, WZCOL_FORM_LIGHT, WZCOL_FORM_DARK, WZCOL_FORM_TIP_BACKGROUND);

	size_t n = 0;
	for (auto &line: displayCache.wzTip)
	{
		line.render(textOffset.x, textOffset.y + lineHeight * static_cast<int32_t>(n), TipColour);
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
