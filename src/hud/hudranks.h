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

#ifndef __INCLUDED_SRC_HUD_HUDRANKS_H__
#define __INCLUDED_SRC_HUD_HUDRANKS_H__

/// Where a unit's rank insignia is shown: a unit's button, or beside a selected unit in the 3D view (smaller)
enum class HudRankSize
{
	Button,
	World,
};

/// The rank insignia (images/hud/icons/rank-N.svg): the Classic images' shapes and colors, crisp at any scale, with a
/// dark outline. All of them are in one texture, so many are drawn in one batch.
///
/// Queues rank `level` (1-8, as getDroidLevel) with the top left of its place at x, y (as wide as the star, the narrower
/// chevrons right-aligned). False if it can't be drawn (the icons are missing): draw the image instead
bool hudQueueRank(unsigned level, float x, float y, HudRankSize size);
void hudDrawQueuedRanks();
bool hudDrawRank(unsigned level, float x, float y, HudRankSize size);
/// Frees the texture (the interface shuts down)
void hudReleaseRanks();

#endif // __INCLUDED_SRC_HUD_HUDRANKS_H__
