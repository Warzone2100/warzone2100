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

#include "hudchart.h"

#include "hudsettings.h"
#include "hudskin.h"
#include "hudwidgets.h"
#include "intel/intelkit.h"

#include "lib/framework/input.h"
#include "lib/framework/wzglobal.h"
#include "lib/gamelib/gtime.h"

#include <algorithm>
#include <cmath>

#include <fmt/core.h>

using namespace intelkit;

namespace
{

constexpr int TOP = 8;
constexpr int BOTTOM = 18;
constexpr int GRID_LINES = 4;
constexpr int LABEL_GAP = 14;

float luminance(PIELIGHT c)
{
	return (0.2126f * c.byte.r + 0.7152f * c.byte.g + 0.0722f * c.byte.b) / 255.f;
}

float niceMax(float value)
{
	if (value <= 0.f)
	{
		return 1.f;
	}
	const float magnitude = std::pow(10.f, std::floor(std::log10(value)));
	for (float f : {1.f, 1.2f, 1.5f, 2.f, 2.5f, 3.f, 4.f, 5.f, 6.f, 8.f, 10.f})
	{
		if (f * magnitude >= value - 0.0001f)
		{
			return f * magnitude;
		}
	}
	return 10.f * magnitude;
}

void hline(float x0, float x1, float y, PIELIGHT color)
{
	pie_UniTransBoxFill(x0, y, x1, y + 1.f, color);
}

void vline(float x, float y0, float y1, PIELIGHT color)
{
	pie_UniTransBoxFill(x, y0, x + 1.f, y1, color);
}

}

bool hudIsDarkColor(PIELIGHT color)
{
	// (Black, purple, red and the blues, but not pink)
	return luminance(color) < 0.25f;
}

float hudAppearSince(uint32_t since, float delayMs, float durationMs)
{
	if (hudReduceMotion())
	{
		return 1.f;
	}
	const float t = std::max(0.f, std::min(1.f, (static_cast<float>(realTime - since) - delayMs) / durationMs));
	return 1.f - (1.f - t) * (1.f - t) * (1.f - t);
}

PIELIGHT hudReadableColor(PIELIGHT color)
{
	// Toward white, keeping the hue, until it reads on the dark panels
	const float l = luminance(color);
	if (l >= 0.38f)
	{
		return color;
	}
	const float k = (0.38f - l) / (1.f - l);
	const auto up = [k](uint8_t c) { return static_cast<uint8_t>(std::lround(c + (255 - c) * k)); };
	return pal_RGBA(up(color.byte.r), up(color.byte.g), up(color.byte.b), 255);
}

std::string hudClockText(uint32_t seconds)
{
	if (seconds >= 3600)
	{
		return fmt::format("{}:{:02}:{:02}", seconds / 3600, (seconds / 60) % 60, seconds % 60);
	}
	return fmt::format("{}:{:02}", seconds / 60, seconds % 60);
}

std::string hudFullNumber(long long v)
{
	std::string digits = std::to_string(std::llabs(v));
	for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3)
	{
		digits.insert(static_cast<size_t>(i), ",");
	}
	return (v < 0 ? "-" : "") + digits;
}

std::string hudChartNumber(float value)
{
	const long long v = std::llround(value);
	if (std::llabs(v) >= 10000)
	{
		std::string s = fmt::format("{:.1f}k", value / 1000.f);
		const size_t dot = s.find(".0k");
		if (dot != std::string::npos)
		{
			s.erase(dot, 2);
		}
		return s;
	}
	std::string digits = std::to_string(std::llabs(v));
	for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3)
	{
		digits.insert(static_cast<size_t>(i), ",");
	}
	return (v < 0 ? "-" : "") + digits;
}

// ---------------------------------------------------------------- HudChart

std::shared_ptr<HudChart> HudChart::make(Mode mode)
{
	auto chart = std::make_shared<HudChart>();
	chart->mode = mode;
	return chart;
}

void HudChart::setTimes(std::vector<float> seconds, float end)
{
	times = std::move(seconds);
	endTime = std::max(end, 1.f);
	dirty = true;
}

void HudChart::setSeries(std::vector<HudChartSeries> newSeries)
{
	series = std::move(newSeries);
	dirty = true;
}

void HudChart::setUnit(const std::string &newUnit)
{
	unit = newUnit;
	dirty = true;
}

void HudChart::setValueDecimals(int decimals)
{
	valueDecimals = decimals;
	dirty = true;
}

std::string HudChart::valueText(float value) const
{
	return (valueDecimals > 0 ? fmt::format("{:.{}f}", value, valueDecimals) : hudChartNumber(value)) + unit;
}

void HudChart::setFixedMax(nonstd::optional<float> max)
{
	fixedMax = max;
	dirty = true;
}

void HudChart::setMarkers(std::vector<HudChartMarker> newMarkers)
{
	markers = std::move(newMarkers);
	dirty = true;
}

void HudChart::setLeadColors(PIELIGHT above, PIELIGHT below, const std::string &newAboveLabel, const std::string &newBelowLabel)
{
	aboveColor = above;
	belowColor = below;
	aboveLabel = newAboveLabel;
	belowLabel = newBelowLabel;
	dirty = true;
}

void HudChart::setFill(bool newFill)
{
	fill = newFill;
	dirty = true;
}

void HudChart::setTooltipTitle(const std::string &title)
{
	tooltipTitle = title;
}

void HudChart::setCursor(nonstd::optional<size_t> index)
{
	cursor = index;
}

void HudChart::geometryChanged()
{
	dirty = true;
}

WzRect HudChart::plotRect() const
{
	if (mode == Mode::Sparkline)
	{
		return WzRect(1, 2, width() - 2, height() - 3);
	}
	const int right = (mode == Mode::Lines || mode == Mode::Lead) ? HUD_CHART_RIGHT_LABELS : HUD_CHART_RIGHT;
	const int bottom = mode == Mode::Lead ? 2 : BOTTOM;
	return WzRect(HUD_CHART_LEFT, TOP, std::max(10, width() - HUD_CHART_LEFT - right), std::max(10, height() - TOP - bottom));
}

float HudChart::xAt(float time, const WzRect &plot) const
{
	return plot.x() + plot.width() * std::min(time, endTime) / endTime;
}

float HudChart::yAt(float value, const WzRect &plot) const
{
	const float span = std::max(maxValue - minValue, 0.0001f);
	return plot.y() + plot.height() * (1.f - (value - minValue) / span);
}

void HudChart::reveal()
{
	revealAt = realTime;
	revealing = true;
}

void HudChart::setBackground(PIELIGHT color)
{
	background = color;
}

void HudChart::rebuild()
{
	dirty = false;
	shapes.clear();
	yLabels.clear();
	xLabels.clear();
	endLabels.clear();
	markerTexts.clear();
	if (times.empty() || series.empty())
	{
		return;
	}
	const WzRect plot = plotRect();

	// The range: from 0 to a round maximum (for leads, around 0)
	float hi = 0.f;
	float lo = 0.f;
	for (const HudChartSeries &s : series)
	{
		for (float v : s.values)
		{
			hi = std::max(hi, v);
			lo = std::min(lo, v);
		}
	}
	if (mode == Mode::Lead)
	{
		// Fitted to the leads, each side keeping at least a quarter of the height (so a brief lead still shows)
		const float above = std::max(hi, 0.f);
		const float below = std::max(-lo, 0.f);
		const float m = std::max(std::max(above, below), 1.f);
		maxValue = std::max(above, m / 3.f);
		minValue = -std::max(below, m / 3.f);
	}
	else
	{
		maxValue = fixedMax.value_or(niceMax(hi * 1.05f));
		minValue = lo < 0.f ? -niceMax(-lo * 1.05f) : 0.f;
	}

	if (mode == Mode::Lines)
	{
		for (int k = 0; k <= GRID_LINES; ++k)
		{
			const float v = minValue + (maxValue - minValue) * k / GRID_LINES;
			yLabels.emplace_back();
			yLabels.back().setText(WzString::fromUtf8(hudChartNumber(v) + unit), font_small);
		}
		// Time ticks at least 70 px apart, on round minutes
		tickStep = 3600.f;
		for (float step : {30.f, 60.f, 120.f, 180.f, 300.f, 600.f, 900.f, 1200.f, 1800.f, 3600.f})
		{
			if (plot.width() * step / endTime >= 70.f)
			{
				tickStep = step;
				break;
			}
		}
		for (float t = 0.f; t <= endTime + 0.5f; t += tickStep)
		{
			xLabels.emplace_back();
			xLabels.back().setText(WzString::fromUtf8(hudClockText(static_cast<uint32_t>(t))), font_small);
		}
	}

	const size_t n = times.size();
	if (mode == Mode::Lead)
	{
		// Above zero in one color, below in the other: split where the line crosses zero
		const std::vector<float> &v = series.front().values;
		const float zero = yAt(0.f, plot);
		std::vector<glm::vec2> piece;
		bool above = true;
		auto flush = [&]() {
			if (piece.size() >= 2)
			{
				PIELIGHT c = above ? aboveColor : belowColor;
				PIELIGHT faint = c;
				c.byte.a = 90;
				faint.byte.a = 40;
				shapes.addArea(piece, zero, c, faint);
			}
			piece.clear();
		};
		for (size_t i = 0; i < n && i < v.size(); ++i)
		{
			const glm::vec2 p(xAt(times[i], plot), yAt(v[i], plot));
			const bool nowAbove = v[i] >= 0.f;
			if (!piece.empty() && nowAbove != above && i > 0)
			{
				const float t = v[i - 1] / (v[i - 1] - v[i]);
				const glm::vec2 cross(xAt(times[i - 1] + (times[i] - times[i - 1]) * t, plot), zero);
				piece.push_back(cross);
				flush();
				piece.push_back(cross);
			}
			above = nowAbove;
			piece.push_back(p);
		}
		flush();
		std::vector<glm::vec2> line;
		for (size_t i = 0; i < n && i < v.size(); ++i)
		{
			line.emplace_back(xAt(times[i], plot), yAt(v[i], plot));
		}
		shapes.addPolyline(line, 1.5f, pal_RGBA(201, 211, 224, 230));
		if (!v.empty())
		{
			const float last = v[std::min(n, v.size()) - 1];
			EndLabel label;
			const std::string who = last >= 0.f ? aboveLabel : belowLabel;
			label.nameString = who + " +" + hudChartNumber(std::fabs(last));
			label.y = yAt(last, plot);
			label.color = last >= 0.f ? aboveColor : belowColor;
			label.bold = false;
			endLabels.push_back(std::move(label));
		}
		return;
	}

	// Lines (yours last, so on top), with a fill under the emphasized one where asked
	std::vector<size_t> order(series.size());
	for (size_t i = 0; i < order.size(); ++i)
	{
		order[i] = i;
	}
	std::stable_sort(order.begin(), order.end(), [this](size_t a, size_t b) { return !series[a].emphasized && series[b].emphasized; });
	auto linePoints = [&](const HudChartSeries &s) {
		std::vector<glm::vec2> line;
		for (size_t i = 0; i < n && i < s.values.size(); ++i)
		{
			line.emplace_back(xAt(times[i], plot), yAt(s.values[i], plot));
		}
		return line;
	};
	if (fill && mode == Mode::Lines)
	{
		// (under every line, so it doesn't tint the others)
		for (const HudChartSeries &s : series)
		{
			if (s.emphasized)
			{
				PIELIGHT top = hudReadableColor(s.color);
				PIELIGHT bottom = top;
				top.byte.a = 64;
				bottom.byte.a = 0;
				shapes.addArea(linePoints(s), yAt(std::max(minValue, 0.f), plot), top, bottom);
			}
		}
	}
	for (size_t idx : order)
	{
		const HudChartSeries &s = series[idx];
		const std::vector<glm::vec2> line = linePoints(s);
		const PIELIGHT c = hudReadableColor(s.color);
		if (mode == Mode::Sparkline)
		{
			PIELIGHT top = c;
			PIELIGHT bottom = c;
			top.byte.a = 50;
			bottom.byte.a = 8;
			shapes.addArea(line, yAt(std::max(minValue, 0.f), plot), top, bottom);
		}
		const float w = mode == Mode::Sparkline ? 1.4f : (s.emphasized ? 2.6f : 1.8f);
		if (mode != Mode::Sparkline)
		{
			// A dark edge under each line, so lines stay apart where they cross
			const PIELIGHT edge = pal_RGBA(static_cast<uint8_t>(background.byte.r * 0.55f), static_cast<uint8_t>(background.byte.g * 0.55f),
			                               static_cast<uint8_t>(background.byte.b * 0.55f), 235);
			shapes.addPolyline(line, w + (hudIsDarkColor(s.color) ? 5.4f : 3.f), edge);
		}
		if (hudIsDarkColor(s.color) && mode != Mode::Sparkline)
		{
			// A dark color keeps its own color, between light edges (so black isn't another gray)
			PIELIGHT edge = c;
			edge.byte.a = 235;
			shapes.addPolyline(line, w + 2.4f, edge);
			PIELIGHT core = s.color;
			core.byte.a = 255;
			shapes.addPolyline(line, std::max(1.2f, w - 0.4f), core);
		}
		else
		{
			shapes.addPolyline(line, w, c);
		}
	}

	if (mode != Mode::Lines)
	{
		return;
	}

	// Markers: a dashed line and a small triangle on the axis
	for (const HudChartMarker &m : markers)
	{
		const float x = std::round(xAt(m.time, plot)) + 0.5f;
		PIELIGHT c = m.color;
		c.byte.a = 180;
		shapes.addPolyline({glm::vec2(x, static_cast<float>(plot.y())), glm::vec2(x, static_cast<float>(plot.bottom()))}, 1.f, c, 3.f, 3.f);
		shapes.addConvexPolygon({glm::vec2(x - 4.f, plot.bottom() + 1.f), glm::vec2(x, plot.bottom() - 5.f), glm::vec2(x + 4.f, plot.bottom() + 1.f)}, m.color);
		markerTexts.emplace_back();
		markerTexts.back().setText(WzString::fromUtf8(m.text), font_small);
	}

	// End labels: the name and the last value, spread so they don't overlap, within the plot
	for (const HudChartSeries &s : series)
	{
		if (s.values.empty())
		{
			continue;
		}
		EndLabel label;
		label.nameString = s.label;
		label.value.setText(WzString::fromUtf8(valueText(s.values[std::min(n, s.values.size()) - 1])), font_small);
		label.y = yAt(s.values[std::min(n, s.values.size()) - 1], plot);
		label.color = hudReadableColor(s.color);
		label.bold = s.emphasized;
		endLabels.push_back(std::move(label));
	}
	std::sort(endLabels.begin(), endLabels.end(), [](const EndLabel &a, const EndLabel &b) { return a.y < b.y; });
	// More than fit: the lowest go unlabeled (not an emphasized one)
	const size_t fit = static_cast<size_t>(std::max(1.f, (plot.bottom() - 4.f - plot.top()) / LABEL_GAP + 1.f));
	for (size_t i = endLabels.size(); endLabels.size() > fit && i-- > 0;)
	{
		if (!endLabels[i].bold)
		{
			endLabels.erase(endLabels.begin() + static_cast<std::ptrdiff_t>(i));
		}
	}
	for (size_t i = 1; i < endLabels.size(); ++i)
	{
		endLabels[i].y = std::max(endLabels[i].y, endLabels[i - 1].y + LABEL_GAP);
	}
	if (!endLabels.empty())
	{
		const float over = endLabels.back().y - (plot.bottom() - 4.f);
		if (over > 0.f)
		{
			for (EndLabel &l : endLabels)
			{
				l.y -= over;
			}
			for (size_t i = endLabels.size() - 1; i-- > 0;)
			{
				endLabels[i].y = std::min(endLabels[i].y, endLabels[i + 1].y - LABEL_GAP);
			}
		}
	}
}

void HudChart::run(W_CONTEXT *)
{
	if (mode == Mode::Sparkline || times.empty())
	{
		return;
	}
	nonstd::optional<size_t> hovered;
	const WzRect plot = plotRect();
	const int mx = mouseX() - screenPosX();
	const int my = mouseY() - screenPosY();
	if (isMouseOverWidget() && mx >= plot.x() - 4 && mx <= plot.right() + 4 && my >= 0 && my <= height())
	{
		const float t = std::max(0.f, (mx - plot.x()) / static_cast<float>(plot.width())) * endTime;
		size_t best = 0;
		for (size_t i = 1; i < times.size(); ++i)
		{
			if (std::fabs(times[i] - t) < std::fabs(times[best] - t))
			{
				best = i;
			}
		}
		hovered = best;
	}
	if (hovered != cursor)
	{
		cursor = hovered;
		if (onCursor)
		{
			onCursor(cursor);
		}
	}
}

void HudChart::display(int xOffset, int yOffset)
{
	if (dirty)
	{
		rebuild();
	}
	const HudColors &colors = currentHudSkin().colors();
	const int x0 = xOffset + x();
	const int y0 = yOffset + y();
	const WzRect local = plotRect();
	const WzRect plot(x0 + local.x(), y0 + local.y(), local.width(), local.height());

	if (mode == Mode::Lines)
	{
		for (int k = 0; k <= GRID_LINES && k < static_cast<int>(yLabels.size()); ++k)
		{
			const float v = minValue + (maxValue - minValue) * k / GRID_LINES;
			const float y = std::round(yAt(v, local)) + y0;
			hline(plot.x(), plot.right(), y, k == 0 ? pal_RGBA(42, 52, 66, 255) : pal_RGBA(28, 36, 48, 255));
			WzText &label = yLabels[static_cast<size_t>(k)];
			label.render(static_cast<float>(plot.x() - 6 - label.width()), y + 3.f, alpha(colors.textSecondary, 150));
		}
		for (size_t k = 0; k < xLabels.size(); ++k)
		{
			const float x = xAt(tickStep * k, local) + x0;
			WzText &label = xLabels[k];
			label.render(std::round(x - label.width() / 2.f), static_cast<float>(plot.bottom() + 13), alpha(colors.textSecondary, 150));
		}
	}
	else if (mode == Mode::Lead)
	{
		const float zero = std::round(yAt(0.f, local)) + y0;
		hline(plot.x(), plot.right(), zero, pal_RGBA(58, 70, 88, 255));
	}

	shapes.draw(static_cast<float>(x0), static_cast<float>(y0));

	// Drawing in: the lines past the reveal covered (the grid drawn again over the cover), the labels after
	const float drawn = revealing ? hudAppearSince(revealAt, 0.f, 800.f) : 1.f;
	revealing = revealing && drawn < 1.f;
	if (drawn < 1.f)
	{
		const float rx = std::round(plot.x() + plot.width() * drawn);
		pie_UniTransBoxFill(rx, static_cast<float>(plot.y() - 6), static_cast<float>(x0 + width()), static_cast<float>(plot.bottom() + 2), background);
		if (mode == Mode::Lines)
		{
			for (int k = 0; k <= GRID_LINES && k < static_cast<int>(yLabels.size()); ++k)
			{
				const float y = std::round(yAt(minValue + (maxValue - minValue) * k / GRID_LINES, local)) + y0;
				hline(rx, plot.right(), y, k == 0 ? pal_RGBA(42, 52, 66, 255) : pal_RGBA(28, 36, 48, 255));
			}
		}
		else if (mode == Mode::Lead)
		{
			hline(rx, plot.right(), std::round(yAt(0.f, local)) + y0, pal_RGBA(58, 70, 88, 255));
		}
		return;
	}

	if (mode == Mode::Lines)
	{
		for (size_t i = 0; i < markers.size() && i < markerTexts.size(); ++i)
		{
			const float x = std::round(xAt(markers[i].time, local)) + x0;
			if (!markers[i].text.empty())
			{
				WzText &t = markerTexts[i];
				const bool left = x + 4 + t.width() > plot.right();
				t.render(left ? x - 4 - t.width() : x + 4, static_cast<float>(plot.y() + 9), markers[i].color);
			}
		}
	}
	if (mode != Mode::Sparkline)
	{
		const int lx = plot.right() + 8;
		const int availableWidth = x0 + width() - lx;
		for (EndLabel &label : endLabels)
		{
			// The name, cut short to leave the value its room
			const int valueW = label.value.getText().isEmpty() ? 0 : label.value.width() + 4;
			const int nameW = drawLine(label.name, label.nameString, font_small, lx, static_cast<int>(std::lround(label.y)) + y0 - 6, availableWidth - valueW, label.color);
			if (valueW)
			{
				label.value.render(static_cast<float>(lx + nameW + 4),
				                   baselineAt(label.name, static_cast<int>(std::lround(label.y)) + y0 - 6), colors.textSecondary);
			}
		}
		if (mode == Mode::Lines && !times.empty())
		{
			const float ex = xAt(times.back(), local) + x0;
			for (const HudChartSeries &s : series)
			{
				if (s.values.empty())
				{
					continue;
				}
				const float ey = yAt(s.values.back(), local) + y0;
				const PIELIGHT c = hudReadableColor(s.color);
				if (hudIsDarkColor(s.color))
				{
					PIELIGHT core = s.color;
					core.byte.a = 255;
					pie_DrawRoundedBox(ex - 4.f, ey - 4.f, ex + 4.f, ey + 4.f, core, 4.f, c, 1.5f);
				}
				else
				{
					pie_DrawRoundedBox(ex - 3.f, ey - 3.f, ex + 3.f, ey + 3.f, c, 3.f);
				}
			}
		}
	}

	// The cursor: a line, the values' points, and (in a chart with axes) the values ranked
	if (cursor && *cursor < times.size() && mode != Mode::Sparkline)
	{
		const size_t i = *cursor;
		const float cx = std::round(xAt(times[i], local)) + x0;
		vline(cx, static_cast<float>(plot.y() - 2), static_cast<float>(plot.bottom()), pal_RGBA(201, 211, 224, 140));
		if (mode == Mode::Lead)
		{
			return;
		}
		for (const HudChartSeries &s : series)
		{
			if (i >= s.values.size())
			{
				continue;
			}
			const float cy = yAt(s.values[i], local) + y0;
			pie_DrawRoundedBox(cx - 4.f, cy - 4.f, cx + 4.f, cy + 4.f, pal_RGBA(14, 19, 26, 255), 4.f, hudReadableColor(s.color), 2.f);
		}
		std::vector<size_t> ranked;
		for (size_t k = 0; k < series.size(); ++k)
		{
			if (i < series[k].values.size())
			{
				ranked.push_back(k);
			}
		}
		std::stable_sort(ranked.begin(), ranked.end(), [this, i](size_t a, size_t b) { return series[a].values[i] > series[b].values[i]; });
		const std::vector<std::pair<std::string, std::string>> extra = tooltipExtra ? tooltipExtra(i) : std::vector<std::pair<std::string, std::string>>();
		const int rowH = 19;
		tipTexts.resize(ranked.size() * 3 + extra.size() * 2);
		tipNames.resize(std::max(tipNames.size(), ranked.size()));
		tipNameWidths.resize(tipNames.size(), 0);
		tipChips.resize(std::max(tipChips.size(), ranked.size()));
		// Wide enough for the names and values (to a limit: longer names end in an ellipsis)
		int nameW = 0;
		int valueW = 0;
		for (size_t k = 0; k < ranked.size(); ++k)
		{
			const HudChartSeries &s = series[ranked[k]];
			WzText &name = tipNames[k];
			WzText &value = tipTexts[k * 3 + 2];
			name.setText(WzString::fromUtf8(s.label), font_small, tipNameWidths[k]);
			value.setText(WzString::fromUtf8(valueText(s.values[i])), font_small);
			nameW = std::max(nameW, name.fit().naturalWidth + (s.withChip ? 21 : 0));
			valueW = std::max(valueW, value.width());
		}
		for (size_t k = 0; k < extra.size(); ++k)
		{
			WzText &label = tipTexts[ranked.size() * 3 + k * 2];
			WzText &value = tipTexts[ranked.size() * 3 + k * 2 + 1];
			label.setText(WzString::fromUtf8(extra[k].first), font_small);
			value.setText(WzString::fromUtf8(extra[k].second), font_small);
			nameW = std::max(nameW, label.width() - 14);
			valueW = std::max(valueW, value.width());
		}
		const int tipW = std::max(170, std::min(280, 24 + nameW + 14 + valueW + 10));
		const int tipH = 30 + rowH * static_cast<int>(ranked.size()) + (extra.empty() ? 0 : 8 + rowH * static_cast<int>(extra.size())) + 6;
		int tx = static_cast<int>(cx) + 14;
		if (tx + tipW > plot.right() + HUD_CHART_RIGHT_LABELS)
		{
			tx = static_cast<int>(cx) - 14 - tipW;
		}
		const int ty = plot.y() + 6;
		pie_DrawRoundedBox(static_cast<float>(tx), static_cast<float>(ty), static_cast<float>(tx + tipW), static_cast<float>(ty + tipH),
		                   pal_RGBA(13, 18, 24, 245), 5.f, pal_RGBA(58, 70, 88, 255), 1.f);
		tipTitle.setText(WzString::fromUtf8(tooltipTitle + "  " + hudClockText(static_cast<uint32_t>(times[i]))), font_small);
		tipTitle.render(static_cast<float>(tx + 10), static_cast<float>(ty + 17), colors.textSecondary);
		int ry = ty + 26;
		for (size_t k = 0; k < ranked.size(); ++k)
		{
			const HudChartSeries &s = series[ranked[k]];
			WzText &rank = tipTexts[k * 3];
			WzText &name = tipNames[k];
			WzText &value = tipTexts[k * 3 + 2];
			rank.setText(WzString::number(static_cast<int>(k + 1)), font_small);
			const float base = static_cast<float>(ry + 13);
			rank.render(static_cast<float>(tx + 10), base, colors.textSecondary);
			int nx = tx + 24;
			if (s.withChip)
			{
				drawPositionChip(tipChips[k], s.chip, WzRect(nx, ry + 2, 15, 15));
				nx += 21;
			}
			tipNameWidths[k] = std::max(1, tx + tipW - 10 - value.width() - 14 - nx);
			hudDrawTextLine(name, static_cast<float>(nx), base, tipNameWidths[k], colors.textPrimary);
			value.render(static_cast<float>(tx + tipW - 10 - value.width()), base, colors.textPrimary);
			ry += rowH;
		}
		if (!extra.empty())
		{
			hline(static_cast<float>(tx + 10), static_cast<float>(tx + tipW - 10), static_cast<float>(ry + 3), pal_RGBA(42, 52, 66, 255));
			ry += 8;
			for (size_t k = 0; k < extra.size(); ++k)
			{
				WzText &label = tipTexts[ranked.size() * 3 + k * 2];
				WzText &value = tipTexts[ranked.size() * 3 + k * 2 + 1];
				const float base = static_cast<float>(ry + 13);
				label.render(static_cast<float>(tx + 10), base, colors.textSecondary);
				value.render(static_cast<float>(tx + tipW - 10 - value.width()), base, colors.textPrimary);
				ry += rowH;
			}
		}
	}
}

// ---------------------------------------------------------------- HudTimeBand

std::shared_ptr<HudTimeBand> HudTimeBand::make(Look look)
{
	auto band = std::make_shared<HudTimeBand>();
	band->look = look;
	return band;
}

void HudTimeBand::setStretches(std::vector<HudTimeBandStretch> newStretches, float end)
{
	stretches = std::move(newStretches);
	endTime = std::max(end, 1.f);
	labels.clear();
	for (const HudTimeBandStretch &s : stretches)
	{
		labels.emplace_back();
		labels.back().setText(WzString::fromUtf8(s.label), font_small);
	}
}

void HudTimeBand::setMargins(int left, int right)
{
	marginLeft = left;
	marginRight = right;
}

void HudTimeBand::setCursorTime(nonstd::optional<float> time)
{
	cursorTime = time;
}

void HudTimeBand::display(int xOffset, int yOffset)
{
	const int x0 = xOffset + x() + marginLeft;
	const int y0 = yOffset + y();
	const int w = std::max(1, width() - marginLeft - marginRight);
	const auto X = [&](float t) { return x0 + w * std::min(t, endTime) / endTime; };
	for (size_t i = 0; i < stretches.size(); ++i)
	{
		const HudTimeBandStretch &s = stretches[i];
		const float a = X(s.from);
		const float b = X(s.to);
		if (look == Look::Band)
		{
			PIELIGHT c = s.color.value_or(pal_RGBA(30, 39, 51, 255));
			if (s.color)
			{
				c.byte.a = 230;
			}
			pie_UniTransBoxFill(a, static_cast<float>(y0 + 2), b, static_cast<float>(y0 + 14), c);
			if (s.color && hudIsDarkColor(*s.color))
			{
				// As its line: the lifted color outside, its own in the middle
				pie_UniTransBoxFill(a, static_cast<float>(y0 + 2), b, static_cast<float>(y0 + 14), hudReadableColor(*s.color));
				PIELIGHT core = *s.color;
				core.byte.a = 255;
				pie_UniTransBoxFill(a, static_cast<float>(y0 + 5), b, static_cast<float>(y0 + 11), core);
			}
			if (!s.label.empty() && b - a >= 30.f)
			{
				labels[i].render(std::round(a + 3.f), static_cast<float>(y0 + 28), s.labelColor);
			}
		}
		else
		{
			pie_DrawRoundedBox(a, static_cast<float>(y0), b - 2.f, static_cast<float>(y0 + 16), s.color ? pal_RGBA(29, 42, 58, 255) : pal_RGBA(24, 32, 43, 255), 3.f);
			pie_UniTransBoxFill(a, static_cast<float>(y0 + 14), b - 2.f, static_cast<float>(y0 + 16), s.color.value_or(pal_RGBA(42, 52, 66, 255)));
			WzText &label = labels[i];
			if (label.width() < b - a - 6.f)
			{
				label.render(std::round((a + b) / 2.f - label.width() / 2.f), static_cast<float>(y0 + 11), currentHudSkin().colors().textSecondary);
			}
		}
	}
	if (cursorTime && look == Look::Band)
	{
		vline(std::round(X(*cursorTime)), static_cast<float>(y0), static_cast<float>(y0 + 16), pal_RGBA(201, 211, 224, 180));
	}
}
