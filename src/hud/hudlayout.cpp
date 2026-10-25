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

#include "hudlayout.h"
#include "hud.h"
#include "hudpanelfilters.h"
#include "hudsettings.h"

#include "lib/ivis_opengl/pieclip.h"
#include "lib/widget/widget.h"

#include "../hci.h"
#include "../hci/groups.h"
#include "../hci/objects_stats.h"
#include "../game_world.h"
#include "../radar.h"

#include <algorithm>
#include <cmath>

namespace
{

constexpr int AUTO_COMPACT_BELOW = 480;
constexpr int AUTO_COMPACT_BELOW_LARGER_TARGETS = 510;
constexpr int COMPACT_TOP = 2;
constexpr int STATS_HEADER = 18;
constexpr int STATS_HEADER_LARGER_TARGETS = 20;
constexpr int STATS_ROW = STAT_BUTHEIGHT + STAT_GAP;
constexpr int STATS_MAX_ROWS = 6;
/// With the reticule's band: the rows its height gives back
constexpr int STATS_MAX_ROWS_RETICULE_BAND = 8;
constexpr float BAND_PAD = 4.f;
constexpr float BAND_GAP = 2.f;
/// The reticule's images' height for their width
constexpr float HEX_ASPECT = 60.f / 76.f;
constexpr int RETICULE_GAP = 2;
constexpr int SLIM_GROUPS_HEIGHT = 24;
constexpr int TOUCH_HEADER_GROWTH = 6;
constexpr float RADAR_MAX_FRACTION = 0.4f;

bool laidOutCompact = false;
bool laidOutLargerTargets = false;
bool laidOutCompactReticule = false;

/// The band: five columns of a honeycomb, as the cluster's hexes flattened (the cancel hex in the middle, Commanders
/// and Build at the ends, the others above and below between them), BAND_GAP apart across every edge
struct ReticuleBand
{
	float hexWidth;
	float hexHeight;
	float column;   ///< from one column to the next
	float half;     ///< from a column's middle to its hexes above and below
	float left;
	float height;
};

ReticuleBand reticuleBand()
{
	// Across a slanted edge the next hex is (0.75w, h/2) along, plus the gap in that direction
	const float diagonal = std::hypot(0.75f, HEX_ASPECT / 2.f);
	const float alongX = 0.75f / diagonal;
	const float alongY = (HEX_ASPECT / 2.f) / diagonal;
	ReticuleBand band;
	band.hexWidth = (static_cast<float>(RET_FORMWIDTH) - 2.f * BAND_PAD - 4.f * BAND_GAP * alongX) / 4.f;
	band.hexHeight = band.hexWidth * HEX_ASPECT;
	band.column = 0.75f * band.hexWidth + BAND_GAP * alongX;
	band.half = band.hexHeight / 2.f + BAND_GAP * alongY;
	band.left = (static_cast<float>(RET_FORMWIDTH) - band.hexWidth - 4.f * band.column) / 2.f;
	band.height = std::ceil(2.f * band.half + band.hexHeight + 2.f * BAND_PAD);
	return band;
}

}

bool hudCompactLayout()
{
	switch (hudSettings().layout)
	{
	case HudLayoutChoice::Standard:
		return false;
	case HudLayoutChoice::Compact:
		return true;
	case HudLayoutChoice::Auto:
		break;
	}
	const int below = hudLargerTouchTargets() ? AUTO_COMPACT_BELOW_LARGER_TARGETS : AUTO_COMPACT_BELOW;
	return static_cast<int>(pie_GetVideoBufferHeight()) < below;
}

bool hudLargerTouchTargets()
{
	return hudSettings().largerTouchTargets;
}

bool hudCompactReticule()
{
	switch (hudSettings().reticule)
	{
	case HudReticuleChoice::Full:
		return false;
	case HudReticuleChoice::Compact:
		return true;
	case HudReticuleChoice::Auto:
		break;
	}
	return hudCompactLayout();
}

WzRect hudReticuleRect()
{
	if (!hudCompactReticule())
	{
		return WzRect(RET_X, RET_Y, RET_FORMWIDTH, RET_FORMHEIGHT);
	}
	const int height = static_cast<int>(reticuleBand().height);
	return WzRect(RET_X, RET_Y + RET_FORMHEIGHT - height, RET_FORMWIDTH, height);
}

WzRect hudReticuleBandButtonRect(int slot, int width, int height)
{
	const ReticuleBand band = reticuleBand();
	int column = 2;
	int row = 0;
	switch (slot)
	{
	case RETBUT_COMMAND: column = 0; break;
	case RETBUT_FACTORY: column = 1; row = -1; break;
	case RETBUT_INTELMAP: column = 1; row = 1; break;
	case RETBUT_CANCEL: column = 2; break;
	case RETBUT_RESEARCH: column = 3; row = -1; break;
	case RETBUT_DESIGN: column = 3; row = 1; break;
	case RETBUT_BUILD: column = 4; break;
	default: break;
	}
	const float cellX = band.left + column * band.column;
	const float cellY = (band.height - band.hexHeight) / 2.f + row * band.half;
	// The image's own shape, as large as fits the hex's place
	const float scale = (width > 0 && height > 0) ? std::min(band.hexWidth / width, band.hexHeight / height) : 1.f;
	const float w = width * scale;
	const float h = height * scale;
	return WzRect(static_cast<int>(std::lround(cellX + (band.hexWidth - w) / 2.f)), static_cast<int>(std::lround(cellY + (band.hexHeight - h) / 2.f)),
	              static_cast<int>(std::lround(w)), static_cast<int>(std::lround(h)));
}

bool hudCompactHeaderMenu()
{
	return hudCompactLayout() && hudLargerTouchTargets();
}

int hudTouchHeaderGrowth()
{
	return hudLargerTouchTargets() && !hudCompactLayout() ? TOUCH_HEADER_GROWTH : 0;
}

int hudCompactStatsHeaderHeight()
{
	return hudLargerTouchTargets() ? STATS_HEADER_LARGER_TARGETS : STATS_HEADER;
}

int hudCompactStatsRows()
{
	int bottom = hudReticuleRect().y() - RETICULE_GAP;
	const WzRect objects = hudObjectsPanelRect();
	if (STAT_X + STAT_WIDTH + hudPanelFiltersTabWidth() > objects.x())
	{
		// (At 640 wide, with the filters' tab out, the column would reach the object row: it stops above it)
		bottom = std::min(bottom, objects.y() - RETICULE_GAP);
	}
	const int availableHeight = bottom - COMPACT_TOP - hudCompactStatsHeaderHeight() + STAT_GAP;
	return std::max(1, std::min(hudCompactReticule() ? STATS_MAX_ROWS_RETICULE_BAND : STATS_MAX_ROWS, availableHeight / STATS_ROW));
}

WzRect hudStatsPanelRect()
{
	if (!hudCompactLayout())
	{
		return WzRect(STAT_X, STAT_Y, STAT_WIDTH, STAT_HEIGHT);
	}
	return WzRect(STAT_X, COMPACT_TOP, STAT_WIDTH, hudCompactStatsHeaderHeight() + hudCompactStatsRows() * STATS_ROW);
}

WzRect hudGroupsPanelRect()
{
	const WzRect classic(GROUP_BACKX, GROUP_BACKY, GROUP_BACKWIDTH, GROUP_BACKHEIGHT);
	if (!hudCompactLayout())
	{
		return classic;
	}
	return WzRect(classic.x(), classic.bottom() - SLIM_GROUPS_HEIGHT, classic.width(), SLIM_GROUPS_HEIGHT);
}

WzRect hudObjectsPanelRect()
{
	const bool groups = getGroupButtonEnabled();
	int y = OBJ_BACKY - (groups ? 80 : 0);
	if (groups && hudCompactLayout())
	{
		// Down onto the room the slim groups bar leaves
		y += GROUP_BACKHEIGHT - SLIM_GROUPS_HEIGHT;
	}
	return WzRect(OBJ_BACKX, y, OBJ_BACKWIDTH, OBJ_BACKHEIGHT);
}

size_t hudRadarMaxSize()
{
	if (!hudCompactLayout())
	{
		return 0;
	}
	return static_cast<size_t>(static_cast<float>(pie_GetVideoBufferHeight()) * RADAR_MAX_FRACTION);
}

int hudTransporterButtonX(int classicX)
{
	// Out of the left column, which the stats panel takes from the top of the screen (and right of its filters' tab)
	return hudCompactLayout() ? STAT_X + STAT_WIDTH + hudPanelFiltersTabWidth() + BASE_GAP : classicX;
}

void hudLayoutSettingsChanged()
{
	if (psWScreen != nullptr)
	{
		const int width = static_cast<int>(pie_GetVideoBufferWidth());
		const int height = static_cast<int>(pie_GetVideoBufferHeight());
		psWScreen->screenSizeDidChange(width, height, width, height);
		resizeRadar(gameWorld.map);
	}
	hudLayoutScreenSizeChanged();
}

void hudLayoutBuilt()
{
	laidOutCompact = hudCompactLayout();
	laidOutLargerTargets = hudLargerTouchTargets();
	laidOutCompactReticule = hudCompactReticule();
}

void hudLayoutScreenSizeChanged()
{
	const bool compact = hudCompactLayout();
	const bool largerTargets = hudLargerTouchTargets();
	const bool compactReticule = hudCompactReticule();
	if (compact != laidOutCompact || largerTargets != laidOutLargerTargets || compactReticule != laidOutCompactReticule)
	{
		laidOutCompact = compact;
		laidOutLargerTargets = largerTargets;
		laidOutCompactReticule = compactReticule;
		hudRequestRebuild();
	}
}
