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
 *  Readable health bars in the 3D view: thicker, with a dark outline, quarter ticks, a short trail of recent damage,
 *  and a palette that reads without telling red from green. Same objects, same places as Classic.
 */

#ifndef __INCLUDED_SRC_HUD_HUDWORLDBARS_H__
#define __INCLUDED_SRC_HUD_HUDWORLDBARS_H__

#include <cstddef>
#include <cstdint>

class BatchedMultiRectRenderer;

bool hudReadableBarsActive();
/// A health bar from x0 to x1 whose fill starts at y (its outline goes around it), at `fraction` of full health
void hudQueueReadableBar(BatchedMultiRectRenderer &renderer, size_t rectGroup, float x0, float x1, float y, float fillHeight, float fraction, uint32_t objectId);
void hudReleaseReadableBars();

#endif // __INCLUDED_SRC_HUD_HUDWORLDBARS_H__
