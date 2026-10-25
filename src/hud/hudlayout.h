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
 *  The HUD's layout. The Standard layout is the Classic one: every element where it always was. The Compact layout
 *  fits a height of 360:
 *  - the stats panel from the top of the screen down to the reticule, as many rows as fit
 *  - the groups bar one slim row of chips, the object row moved down onto the room it leaves
 *  - the radar capped at 40% of the height
 *  - the transporter's button right of the left column
 *  Geometry only, so both skins get it.
 */

#ifndef __INCLUDED_SRC_HUD_HUDLAYOUT_H__
#define __INCLUDED_SRC_HUD_HUDLAYOUT_H__

#include "lib/framework/geometry.h"

#include <cstddef>

/// The layout in use (the Layout setting, and with Auto the screen's height)
bool hudCompactLayout();
/// Larger Touch Targets (the setting)
bool hudLargerTouchTargets();
/// The reticule as a band of hexes two high (the Reticule setting, and with Auto the layout)
bool hudCompactReticule();
/// The reticule's form: the Classic cluster, or the band at the cluster's bottom
WzRect hudReticuleRect();
/// In the band: a reticule button's rectangle in the form, its image (halved, as in the cluster) fitted to its hex,
/// which stays on the same side of the cancel hex as in the cluster
WzRect hudReticuleBandButtonRect(int slot, int width, int height);

/// Larger Touch Targets in the Standard layout: how much larger the panels' header controls and close buttons are
int hudTouchHeaderGrowth();

/// The stats panel (build, manufacture, research): its Classic place, or the compact one
WzRect hudStatsPanelRect();
/// Compact with Larger Touch Targets: the stats panels' header buttons are in one menu
bool hudCompactHeaderMenu();
/// Compact: the stats panel's header row, above its list
int hudCompactStatsHeaderHeight();
/// Compact: the stats list's rows (the page control in the header's row, no page tabs)
int hudCompactStatsRows();
/// The object row (with the groups bar under it, or not)
WzRect hudObjectsPanelRect();
/// The groups bar: Classic, or the slim row of chips
WzRect hudGroupsPanelRect();
/// The radar's largest side, or none
size_t hudRadarMaxSize();
/// The transporter's timer and launch buttons' left edge
int hudTransporterButtonX(int classicX);

/// The settings changed: everything is laid out again, and what the layout builds differently is rebuilt
void hudLayoutSettingsChanged();
/// The screen's size changed (after the widgets were laid out again): rebuilt when the layout changed with it, or
/// Larger Touch Targets did
void hudLayoutScreenSizeChanged();
/// The HUD was (re)built for the layout in use
void hudLayoutBuilt();

#endif // __INCLUDED_SRC_HUD_HUDLAYOUT_H__
