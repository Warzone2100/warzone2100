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
 *  Charts over a game's time, in the Modern skin: lines with labels at their ends and a cursor, the lead of one side
 *  over another, a sparkline, and a band of colored stretches (who led).
 */

#ifndef __INCLUDED_SRC_HUD_HUDCHART_H__
#define __INCLUDED_SRC_HUD_HUDCHART_H__

#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/pieshapes.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/widget.h"
#include "intel/intelkit.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <nonstd/optional.hpp>

/// A player's color as a line or text on the dark panels (dark colors lifted)
PIELIGHT hudReadableColor(PIELIGHT color);
/// How far an animation that began at `since` (realTime), after `delayMs`, over `durationMs`, has come: 0 to 1, eased out
/// (always 1 when the HUD reduces motion)
float hudAppearSince(uint32_t since, float delayMs, float durationMs);
/// Whether a color is too dark to show on the dark panels by itself (drawn with a light edge instead)
bool hudIsDarkColor(PIELIGHT color);
/// m:ss (h:mm:ss from an hour)
std::string hudClockText(uint32_t seconds);
/// A chart's number: 1,234, and 12.3k from 10,000
std::string hudChartNumber(float value);
/// A number in full: 1,234,567
std::string hudFullNumber(long long value);

struct HudChartSeries
{
	std::string label;
	PIELIGHT color;
	std::vector<float> values;  ///< one per time
	bool emphasized = false;    ///< (yours: thicker)
	unsigned chip = 0;          ///< the player whose chip the tooltip shows
	bool withChip = false;
};

struct HudChartMarker
{
	float time;
	std::string text;
	PIELIGHT color;
};

class HudChart : public WIDGET
{
public:
	enum class Mode
	{
		Lines,      ///< axes, end labels, cursor and tooltip
		Lead,       ///< one series: above zero in `aboveColor`, below in `belowColor`
		Sparkline,  ///< the lines only
	};

	static std::shared_ptr<HudChart> make(Mode mode);

	void setTimes(std::vector<float> seconds, float end);
	void setSeries(std::vector<HudChartSeries> series);
	void setUnit(const std::string &unit);
	/// The values' (not the axis's) decimal places
	void setValueDecimals(int decimals);
	void setFixedMax(nonstd::optional<float> max);
	void setMarkers(std::vector<HudChartMarker> markers);
	void setLeadColors(PIELIGHT above, PIELIGHT below, const std::string &aboveLabel, const std::string &belowLabel);
	/// (Lines mode) A fill under the emphasized series, fading downward
	void setFill(bool fill);
	void setTooltipTitle(const std::string &title);
	/// Draws its lines in, from the left, from now
	void reveal();
	/// The color it's drawn on (the reveal covers the lines not drawn yet with it)
	void setBackground(PIELIGHT color);
	/// More tooltip rows at a time's index (ex. the teams' totals)
	std::function<std::vector<std::pair<std::string, std::string>> (size_t)> tooltipExtra;
	/// The cursor's time index (hovering), shared with other charts and bands
	std::function<void (nonstd::optional<size_t>)> onCursor;
	void setCursor(nonstd::optional<size_t> index);

	void geometryChanged() override;
	void run(W_CONTEXT *psContext) override;
	void display(int xOffset, int yOffset) override;

private:
	std::string valueText(float value) const;
	WzRect plotRect() const;
	float yAt(float value, const WzRect &plot) const;
	float xAt(float time, const WzRect &plot) const;
	void rebuild();

	Mode mode = Mode::Lines;
	std::vector<float> times;
	float endTime = 1.f;
	std::vector<HudChartSeries> series;
	std::string unit;
	int valueDecimals = 0;
	nonstd::optional<float> fixedMax;
	std::vector<HudChartMarker> markers;
	PIELIGHT aboveColor = WZCOL_WHITE;
	PIELIGHT belowColor = WZCOL_WHITE;
	std::string aboveLabel, belowLabel;
	bool fill = false;
	std::string tooltipTitle;
	uint32_t revealAt = 0;
	bool revealing = false;
	PIELIGHT background = pal_RGBA(14, 18, 24, 255);
	nonstd::optional<size_t> cursor;

	float minValue = 0.f;
	float maxValue = 1.f;
	float tickStep = 60.f;
	bool dirty = true;
	PieShapes shapes;
	std::vector<WzText> yLabels;
	std::vector<WzText> xLabels;
	struct EndLabel
	{
		std::string nameString;
		WzText name;
		WzText value;
		float y;
		PIELIGHT color;
		bool bold;
	};
	std::vector<EndLabel> endLabels;
	std::vector<WzText> markerTexts;
	WzText tipTitle;
	std::vector<WzText> tipTexts;
	std::vector<WzText> tipNames;
	std::vector<int> tipNameWidths;   ///< the width each name was drawn in
	std::vector<intelkit::ChipText> tipChips;
};

struct HudTimeBandStretch
{
	float from;
	float to;
	nonstd::optional<PIELIGHT> color;  ///< none: nobody (gray)
	std::string label;
	PIELIGHT labelColor;
};

/// Colored stretches over a game's time, labeled where they last (who led, the Timeline's phases)
class HudTimeBand : public WIDGET
{
public:
	enum class Look
	{
		Band,    ///< a bar, labels under it
		Phases,  ///< labeled boxes with a colored line under each
	};

	static std::shared_ptr<HudTimeBand> make(Look look);
	void setStretches(std::vector<HudTimeBandStretch> stretches, float end);
	/// The chart's left and right margins (to line up with it)
	void setMargins(int left, int right);
	void setCursorTime(nonstd::optional<float> time);

	void display(int xOffset, int yOffset) override;

private:
	Look look = Look::Band;
	std::vector<HudTimeBandStretch> stretches;
	float endTime = 1.f;
	int marginLeft = 0;
	int marginRight = 0;
	nonstd::optional<float> cursorTime;
	std::vector<WzText> labels;
};

/// Where a chart's plot starts and ends inside its widget (for bands lined up under it)
constexpr int HUD_CHART_LEFT = 38;
constexpr int HUD_CHART_RIGHT_LABELS = 86;
constexpr int HUD_CHART_RIGHT = 10;

#endif // __INCLUDED_SRC_HUD_HUDCHART_H__
