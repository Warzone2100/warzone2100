/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
 *  Functions for a generic point/line data graph widget
 */

#include "datagraph.h"
#include "widget.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/math_ext.h"

#include <glm/vec4.hpp>
#include <algorithm>
#include <cmath>

constexpr int GRAPH_PADDING_TOP = 4;
constexpr int GRAPH_PADDING_RIGHT = 12;
constexpr int AXIS_LABEL_GAP = 4;
constexpr int LEGEND_SWATCH_SIZE = 8;
constexpr int LEGEND_ITEM_GAP = 14;
constexpr int LEGEND_BOTTOM_PADDING = 6;
constexpr int POINT_HOVER_RADIUS = 8;
constexpr int LINE_HOVER_RADIUS = 5;
constexpr int OVERLAP_MERGE_RADIUS = 3;
constexpr int TOOLTIP_PADDING = 6;
constexpr int TOOLTIP_POINT_GAP = 12;
constexpr int TOOLTIP_LABEL_VALUE_GAP = 8;

static WzString defaultValueFormat(float value)
{
	return WzString::fromUtf8(astringf("%g", value));
}

// Returns a "nice" (1/2/5 * 10^n) tick step for the given range
static float niceTickStep(float range, int targetTicks)
{
	if (range <= 0.f || targetTicks < 1)
	{
		return 1.f;
	}
	float rough = range / static_cast<float>(targetTicks);
	float magnitude = powf(10.f, floorf(log10f(rough)));
	for (float mult : {1.f, 2.f, 5.f})
	{
		if (magnitude * mult >= rough)
		{
			return magnitude * mult;
		}
	}
	return magnitude * 10.f;
}

void DataGraphWidget::setSeries(std::vector<Series> newSeries)
{
	series = std::move(newSeries);
	recalcDataBounds();
	hoverTargets.clear();
	tooltipBuiltFor.clear();
	layoutDirty = true;
}

void DataGraphWidget::setXAxisFormatter(const ValueFormatter& formatter)
{
	xFormatter = formatter;
	tooltipBuiltFor.clear();
	layoutDirty = true;
}

void DataGraphWidget::setYAxisFormatter(const ValueFormatter& formatter)
{
	yFormatter = formatter;
	tooltipBuiltFor.clear();
	layoutDirty = true;
}

void DataGraphWidget::setXAxisPreferredTickSteps(std::vector<float> steps)
{
	xPreferredTickSteps = std::move(steps);
	layoutDirty = true;
}

void DataGraphWidget::setYAxisMinTickStep(float step)
{
	yMinTickStep = step;
	layoutDirty = true;
}

void DataGraphWidget::setBackgroundColor(PIELIGHT color)
{
	backgroundColor = color;
}

void DataGraphWidget::geometryChanged()
{
	layoutDirty = true;
}

int32_t DataGraphWidget::idealWidth()
{
	return 580;
}

int32_t DataGraphWidget::idealHeight()
{
	return 250;
}

void DataGraphWidget::recalcDataBounds()
{
	minX = 0.f;
	maxX = 0.f;
	minY = 0.f;
	maxY = 0.f;
	bool first = true;
	for (const auto& s : series)
	{
		for (const auto& p : s.points)
		{
			if (first)
			{
				minX = maxX = p.x;
				minY = maxY = p.y;
				first = false;
			}
			minX = std::min(minX, p.x);
			maxX = std::max(maxX, p.x);
			minY = std::min(minY, p.y);
			maxY = std::max(maxY, p.y);
		}
	}
}

void DataGraphWidget::rebuildCachedLayoutIfNeeded()
{
	if (!layoutDirty)
	{
		return;
	}
	layoutDirty = false;

	const auto& xFormat = xFormatter ? xFormatter : defaultValueFormat;
	const auto& yFormat = yFormatter ? yFormatter : defaultValueFormat;

	// y axis ticks - based on the data range, so the axis minimum may sit above 0 (or the maximum below 0)
	yTickLabels.clear();
	float yStep = std::max(niceTickStep(std::max(maxY - minY, 1.f), 4), yMinTickStep);
	axisMinY = floorf(minY / yStep) * yStep;
	axisMaxY = ceilf(maxY / yStep) * yStep;
	if (axisMaxY <= axisMinY)
	{
		axisMaxY = axisMinY + yStep;
	}
	for (float v = axisMinY; v <= axisMaxY + yStep * 0.5f; v += yStep)
	{
		if (fabsf(v) < yStep * 1e-4f)
		{
			v = 0.f; // avoid float drift producing a "-0" tick label
		}
		yTickLabels.emplace_back(v, WzText(yFormat(v), font_small));
	}

	int maxYLabelWidth = 0;
	for (auto& tick : yTickLabels)
	{
		maxYLabelWidth = std::max(maxYLabelWidth, tick.second.width());
	}
	leftGutter = maxYLabelWidth + AXIS_LABEL_GAP * 2;
	bottomGutter = iV_GetTextLineSize(font_small) + AXIS_LABEL_GAP;

	// legend (wrapping onto multiple lines when items exceed the widget width)
	legendLabels.clear();
	legendItemPositions.clear();
	int legendLineSize = iV_GetTextLineSize(font_small);
	int legendX = leftGutter;
	int legendRow = 0;
	int legendMaxX = std::max(width() - GRAPH_PADDING_RIGHT, leftGutter + 1);
	for (const auto& s : series)
	{
		legendLabels.emplace_back(s.label, font_small);
		int itemWidth = LEGEND_SWATCH_SIZE + AXIS_LABEL_GAP + legendLabels.back().width();
		if (legendX + itemWidth > legendMaxX && legendX > leftGutter)
		{
			legendX = leftGutter;
			++legendRow;
		}
		legendItemPositions.emplace_back(legendX, legendRow * legendLineSize);
		legendX += itemWidth + LEGEND_ITEM_GAP;
	}
	legendHeight = series.empty() ? 0 : (legendRow + 1) * legendLineSize + LEGEND_BOTTOM_PADDING;

	// x axis ticks
	xTickLabels.clear();
	float xSpan = maxX - minX;
	if (xSpan > 0.f)
	{
		int plotWidth = std::max(width() - leftGutter - GRAPH_PADDING_RIGHT, 1);
		int targetTicks = std::max(plotWidth / 90, 2);
		float xStep = 0.f;
		for (float candidate : xPreferredTickSteps)
		{
			xStep = candidate;
			if (xSpan / candidate <= static_cast<float>(targetTicks))
			{
				break;
			}
		}
		if (xStep <= 0.f)
		{
			xStep = niceTickStep(xSpan, targetTicks);
		}
		for (float v = ceilf(minX / xStep) * xStep; v <= maxX + xStep * 0.01f; v += xStep)
		{
			xTickLabels.emplace_back(v, WzText(xFormat(v), font_small));
		}
	}

	noDataLabel.setText(_("(no data)"), font_small);
}

WzRect DataGraphWidget::plotRect() const
{
	int plotX = leftGutter;
	int plotY = legendHeight + GRAPH_PADDING_TOP;
	return WzRect(plotX, plotY,
	              std::max(width() - plotX - GRAPH_PADDING_RIGHT, 1),
	              std::max(height() - plotY - bottomGutter, 1));
}

Vector2i DataGraphWidget::dataToLocal(Vector2f dataPoint, const WzRect& plot) const
{
	float xSpan = maxX - minX;
	float xFraction = (xSpan > 0.f) ? (dataPoint.x - minX) / xSpan : 0.5f;
	float ySpan = axisMaxY - axisMinY;
	float yFraction = (ySpan > 0.f) ? ((dataPoint.y - axisMinY) / ySpan) : 0.f;
	int lx = plot.left() + static_cast<int>(roundf(xFraction * static_cast<float>(plot.width() - 1)));
	int ly = plot.bottom() - 1 - static_cast<int>(roundf(yFraction * static_cast<float>(plot.height() - 1)));
	return Vector2i(lx, ly);
}

static int distanceSqToSegment(Vector2i p, Vector2i a, Vector2i b)
{
	Vector2i ab = b - a;
	Vector2i ap = p - a;
	int lengthSq = ab.x * ab.x + ab.y * ab.y;
	float t = (lengthSq > 0) ? clipf(static_cast<float>(ap.x * ab.x + ap.y * ab.y) / static_cast<float>(lengthSq), 0.f, 1.f) : 0.f;
	float dx = static_cast<float>(p.x) - (static_cast<float>(a.x) + t * static_cast<float>(ab.x));
	float dy = static_cast<float>(p.y) - (static_cast<float>(a.y) + t * static_cast<float>(ab.y));
	return static_cast<int>(dx * dx + dy * dy);
}

std::vector<DataGraphWidget::HoverTarget> DataGraphWidget::hoverTargetsAt(int localMx, int localMy) const
{
	WzRect plot = plotRect();
	Vector2i mouse(localMx, localMy);

	// nearest point within POINT_HOVER_RADIUS
	optional<HoverTarget> best;
	int bestDistSq = POINT_HOVER_RADIUS * POINT_HOVER_RADIUS + 1;
	for (size_t s = 0; s < series.size(); ++s)
	{
		const auto& points = series[s].points;
		for (size_t i = 0; i < points.size(); ++i)
		{
			Vector2i p = dataToLocal(points[i], plot);
			int dx = p.x - mouse.x;
			int dy = p.y - mouse.y;
			int distSq = dx * dx + dy * dy;
			if (distSq < bestDistSq)
			{
				bestDistSq = distSq;
				best = HoverTarget{s, i};
			}
		}
	}
	if (!best.has_value())
	{
		// otherwise, nearest line segment within LINE_HOVER_RADIUS - snap to the closer endpoint
		bestDistSq = LINE_HOVER_RADIUS * LINE_HOVER_RADIUS + 1;
		for (size_t s = 0; s < series.size(); ++s)
		{
			const auto& points = series[s].points;
			for (size_t i = 1; i < points.size(); ++i)
			{
				Vector2i a = dataToLocal(points[i - 1], plot);
				Vector2i b = dataToLocal(points[i], plot);
				int distSq = distanceSqToSegment(mouse, a, b);
				if (distSq < bestDistSq)
				{
					bestDistSq = distSq;
					Vector2i da = mouse - a;
					Vector2i db = mouse - b;
					bool aCloser = (da.x * da.x + da.y * da.y) <= (db.x * db.x + db.y * db.y);
					best = HoverTarget{s, aCloser ? (i - 1) : i};
				}
			}
		}
	}
	if (!best.has_value())
	{
		return {};
	}

	// collect overlapping points at the primary point's screen position - at most one per series (the closest)
	std::vector<HoverTarget> targets;
	targets.push_back(best.value());
	Vector2i primaryPos = dataToLocal(series[best->seriesIdx].points[best->pointIdx], plot);
	for (size_t s = 0; s < series.size(); ++s)
	{
		if (s == best->seriesIdx)
		{
			continue; // the primary point already represents this series
		}
		const auto& points = series[s].points;
		optional<size_t> closestIdx;
		int closestDistSq = OVERLAP_MERGE_RADIUS * OVERLAP_MERGE_RADIUS + 1;
		for (size_t i = 0; i < points.size(); ++i)
		{
			Vector2i p = dataToLocal(points[i], plot);
			int dx = p.x - primaryPos.x;
			int dy = p.y - primaryPos.y;
			int distSq = dx * dx + dy * dy;
			if (distSq < closestDistSq)
			{
				closestDistSq = distSq;
				closestIdx = i;
			}
		}
		if (closestIdx.has_value())
		{
			targets.push_back(HoverTarget{s, closestIdx.value()});
		}
	}
	return targets;
}

void DataGraphWidget::run(W_CONTEXT *psContext)
{
	if (!isMouseOverWidget())
	{
		lastLocalMouse.reset();
		hoverTargets.clear();
		return;
	}
	lastLocalMouse = Vector2i(psContext->mx - x(), psContext->my - y());
	hoverTargets = hoverTargetsAt(lastLocalMouse->x, lastLocalMouse->y);
}

void DataGraphWidget::highlightLost()
{
	lastLocalMouse.reset();
	hoverTargets.clear();
}

void DataGraphWidget::display(int xOffset, int yOffset)
{
	const int x0 = xOffset + x();
	const int y0 = yOffset + y();
	const int w = width();
	const int h = height();

	rebuildCachedLayoutIfNeeded();

	pie_UniTransBoxFill(x0, y0, x0 + w, y0 + h, backgroundColor);
	iV_Box(x0, y0, x0 + w, y0 + h, pal_RGBA(255, 255, 255, 50));

	WzRect plot = plotRect();
	const PIELIGHT gridColor = pal_RGBA(255, 255, 255, 35);
	const PIELIGHT axisColor = pal_RGBA(255, 255, 255, 100);

	// legend
	int legendTop = y0 + GRAPH_PADDING_TOP;
	for (size_t s = 0; s < series.size() && s < legendLabels.size() && s < legendItemPositions.size(); ++s)
	{
		auto& label = legendLabels[s];
		int itemX = x0 + legendItemPositions[s].x;
		int itemY = legendTop + legendItemPositions[s].y;
		int swatchY = itemY + (label.lineSize() - LEGEND_SWATCH_SIZE) / 2;
		pie_BoxFill(itemX, swatchY, itemX + LEGEND_SWATCH_SIZE, swatchY + LEGEND_SWATCH_SIZE, series[s].color);
		label.render(itemX + LEGEND_SWATCH_SIZE + AXIS_LABEL_GAP, itemY - label.aboveBase(), WZCOL_TEXT_BRIGHT);
	}

	// y gridlines and labels (with the zero baseline emphasized when the axis extends below zero)
	for (auto& tick : yTickLabels)
	{
		Vector2i p = dataToLocal(Vector2f(minX, tick.first), plot);
		bool isZeroBaseline = (tick.first == 0.f) && (axisMinY < 0.f);
		iV_Line(x0 + plot.left(), y0 + p.y, x0 + plot.right() - 1, y0 + p.y, isZeroBaseline ? axisColor : gridColor);
		auto& label = tick.second;
		int labelTop = y0 + p.y - label.lineSize() / 2;
		label.render(x0 + plot.left() - AXIS_LABEL_GAP - label.width(), labelTop - label.aboveBase(), WZCOL_TEXT_MEDIUM);
	}

	// x gridlines and labels
	for (auto& tick : xTickLabels)
	{
		Vector2i p = dataToLocal(Vector2f(tick.first, 0.f), plot);
		iV_Line(x0 + p.x, y0 + plot.top(), x0 + p.x, y0 + plot.bottom() - 1, gridColor);
		auto& label = tick.second;
		int labelLeft = clip<int>(p.x - label.width() / 2, 0, width() - label.width());
		label.render(x0 + labelLeft, y0 + plot.bottom() + AXIS_LABEL_GAP - label.aboveBase(), WZCOL_TEXT_MEDIUM);
	}

	// axes
	iV_Line(x0 + plot.left(), y0 + plot.top(), x0 + plot.left(), y0 + plot.bottom() - 1, axisColor);
	iV_Line(x0 + plot.left(), y0 + plot.bottom() - 1, x0 + plot.right() - 1, y0 + plot.bottom() - 1, axisColor);

	bool hasData = std::any_of(series.begin(), series.end(), [](const Series& s) { return !s.points.empty(); });
	if (!hasData)
	{
		noDataLabel.render(x0 + (w - noDataLabel.width()) / 2, y0 + h / 2, WZCOL_TEXT_MEDIUM);
		return;
	}

	// series lines and points - hovered series draw last (on top) at full opacity, the rest at 75%
	std::vector<bool> seriesHovered(series.size(), false);
	for (const auto& target : hoverTargets)
	{
		if (target.seriesIdx < series.size())
		{
			seriesHovered[target.seriesIdx] = true;
		}
	}
	std::vector<size_t> drawOrder;
	drawOrder.reserve(series.size());
	for (size_t s = 0; s < series.size(); ++s)
	{
		if (!seriesHovered[s]) { drawOrder.push_back(s); }
	}
	for (size_t s = 0; s < series.size(); ++s)
	{
		if (seriesHovered[s]) { drawOrder.push_back(s); }
	}

	std::vector<glm::ivec4> segments;
	for (size_t seriesIdx : drawOrder)
	{
		const auto& s = series[seriesIdx];
		PIELIGHT color = s.color;
		if (!seriesHovered[seriesIdx])
		{
			color.byte.a = static_cast<uint8_t>((color.byte.a * 3) / 4);
		}
		segments.clear();
		optional<Vector2i> prev;
		for (const auto& dataPoint : s.points)
		{
			Vector2i p = dataToLocal(dataPoint, plot);
			if (prev.has_value())
			{
				segments.emplace_back(x0 + prev->x, y0 + prev->y, x0 + p.x, y0 + p.y);
			}
			prev = p;
		}
		if (!segments.empty())
		{
			iV_Lines(segments, color);
		}
		for (const auto& dataPoint : s.points)
		{
			Vector2i p = dataToLocal(dataPoint, plot);
			pie_UniTransBoxFill(x0 + p.x - 1, y0 + p.y - 1, x0 + p.x + 2, y0 + p.y + 2, color);
		}
	}

	// hovered point highlight
	if (!hoverTargets.empty())
	{
		const auto& primary = hoverTargets.front();
		if (primary.seriesIdx < series.size() && primary.pointIdx < series[primary.seriesIdx].points.size())
		{
			const auto& s = series[primary.seriesIdx];
			Vector2i p = dataToLocal(s.points[primary.pointIdx], plot);
			pie_BoxFill(x0 + p.x - 3, y0 + p.y - 3, x0 + p.x + 4, y0 + p.y + 4, s.color);
			iV_Box(x0 + p.x - 4, y0 + p.y - 4, x0 + p.x + 5, y0 + p.y + 5, WZCOL_WHITE);

			displayTooltip(x0, y0, plot);
		}
	}
}

void DataGraphWidget::displayTooltip(int x0, int y0, const WzRect& plot)
{
	if (tooltipBuiltFor != hoverTargets)
	{
		const auto& xFormat = xFormatter ? xFormatter : defaultValueFormat;
		const auto& yFormat = yFormatter ? yFormatter : defaultValueFormat;
		tooltipEntries.clear();
		for (const auto& target : hoverTargets)
		{
			if (target.seriesIdx >= series.size() || target.pointIdx >= series[target.seriesIdx].points.size())
			{
				continue;
			}
			const auto& s = series[target.seriesIdx];
			const auto& dataPoint = s.points[target.pointIdx];
			tooltipEntries.emplace_back();
			auto& entry = tooltipEntries.back();
			entry.label.setText(s.label, font_small);
			entry.value.setText(yFormat(dataPoint.y) + " @ " + xFormat(dataPoint.x), font_small);
			entry.color = s.color;
		}
		tooltipBuiltFor = hoverTargets;
	}
	if (tooltipEntries.empty())
	{
		return;
	}

	int lineHeight = iV_GetTextLineSize(font_small);
	int maxLineWidth = 0;
	for (auto& entry : tooltipEntries)
	{
		maxLineWidth = std::max(maxLineWidth, entry.label.width() + TOOLTIP_LABEL_VALUE_GAP + entry.value.width());
	}
	int boxWidth = maxLineWidth + TOOLTIP_PADDING * 2;
	int boxHeight = lineHeight * static_cast<int>(tooltipEntries.size()) + TOOLTIP_PADDING * 2;

	const auto& primarySeries = series[hoverTargets.front().seriesIdx];
	Vector2i p = dataToLocal(primarySeries.points[hoverTargets.front().pointIdx], plot);
	int boxX = p.x + TOOLTIP_POINT_GAP;
	if (boxX + boxWidth > width())
	{
		boxX = p.x - TOOLTIP_POINT_GAP - boxWidth;
	}
	boxX = clip<int>(boxX, 0, std::max(width() - boxWidth, 0));
	int boxY = clip<int>(p.y - boxHeight / 2, 0, std::max(height() - boxHeight, 0));

	pie_UniTransBoxFill(x0 + boxX, y0 + boxY, x0 + boxX + boxWidth, y0 + boxY + boxHeight, pal_RGBA(15, 15, 15, 220));
	iV_Box(x0 + boxX, y0 + boxY, x0 + boxX + boxWidth, y0 + boxY + boxHeight, (tooltipEntries.size() == 1) ? primarySeries.color : WZCOL_TEXT_MEDIUM);

	int textTop = y0 + boxY + TOOLTIP_PADDING;
	for (auto& entry : tooltipEntries)
	{
		entry.label.render(x0 + boxX + TOOLTIP_PADDING, textTop - entry.label.aboveBase(), entry.color);
		entry.value.render(x0 + boxX + TOOLTIP_PADDING + entry.label.width() + TOOLTIP_LABEL_VALUE_GAP, textTop - entry.value.aboveBase(), WZCOL_TEXT_BRIGHT);
		textTop += lineHeight;
	}
}
