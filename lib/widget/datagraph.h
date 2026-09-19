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
 *  Definitions for a generic point/line data graph widget
 */

#ifndef __INCLUDED_LIB_WIDGET_DATAGRAPH_H__
#define __INCLUDED_LIB_WIDGET_DATAGRAPH_H__

#include "widget.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/ivis_opengl/piepalette.h"

#include <vector>
#include <functional>
#include <nonstd/optional.hpp>
using nonstd::optional;

/**
 * A widget that displays one or more data series as a point/line graph,
 * with axis tick labels, a legend, and a mouse-over tooltip showing the
 * exact value of the hovered point.
 */
class DataGraphWidget : public WIDGET
{
public:
	struct Series
	{
		WzString label;					///< name shown in the legend and hover tooltip
		PIELIGHT color;
		std::vector<Vector2f> points;	///< x ascending
	};

	typedef std::function<WzString (float value)> ValueFormatter;

public:
	void setSeries(std::vector<Series> newSeries);
	void setXAxisFormatter(const ValueFormatter& formatter);
	void setYAxisFormatter(const ValueFormatter& formatter);
	/// Restrict x axis tick spacing to one of the given (ascending) steps - e.g. "nice" time intervals
	void setXAxisPreferredTickSteps(std::vector<float> steps);
	/// Enforce a minimum y axis tick spacing - e.g. 1 for integer-valued data
	void setYAxisMinTickStep(float step);
	void setBackgroundColor(PIELIGHT color);

	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;
	void highlightLost() override;
	void geometryChanged() override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;

private:
	struct HoverTarget
	{
		size_t seriesIdx = 0;
		size_t pointIdx = 0;
		bool operator ==(const HoverTarget& o) const { return seriesIdx == o.seriesIdx && pointIdx == o.pointIdx; }
	};

	struct TooltipEntry
	{
		WzText label;
		WzText value;
		PIELIGHT color = WZCOL_WHITE;
	};

	WzRect plotRect() const;
	Vector2i dataToLocal(Vector2f dataPoint, const WzRect& plot) const;
	std::vector<HoverTarget> hoverTargetsAt(int localMx, int localMy) const;
	void recalcDataBounds();
	void rebuildCachedLayoutIfNeeded();
	void displayTooltip(int x0, int y0, const WzRect& plot);

private:
	std::vector<Series> series;
	ValueFormatter xFormatter;
	ValueFormatter yFormatter;
	std::vector<float> xPreferredTickSteps;
	float yMinTickStep = 0.f;
	PIELIGHT backgroundColor = pal_RGBA(0, 0, 0, 130);

	// data bounds (recalculated by setSeries)
	float minX = 0.f;
	float maxX = 0.f;
	float minY = 0.f;
	float maxY = 0.f;
	float axisMinY = 0.f;	// minY rounded down to the y tick step (may be > 0, or < 0 for negative data)
	float axisMaxY = 0.f;	// maxY rounded up to the y tick step

	// hover state (updated by run)
	optional<Vector2i> lastLocalMouse;
	std::vector<HoverTarget> hoverTargets;	///< all points overlapping the hovered location (primary first)

	// cached layout / text
	bool layoutDirty = true;
	int leftGutter = 0;
	int bottomGutter = 0;
	int legendHeight = 0;
	std::vector<std::pair<float, WzText>> xTickLabels;
	std::vector<std::pair<float, WzText>> yTickLabels;
	std::vector<WzText> legendLabels;
	std::vector<Vector2i> legendItemPositions;	// local position of each legend item, relative to (0, GRAPH_PADDING_TOP)
	WzText noDataLabel;
	std::vector<HoverTarget> tooltipBuiltFor;
	std::vector<TooltipEntry> tooltipEntries;
};

#endif // __INCLUDED_LIB_WIDGET_DATAGRAPH_H__
