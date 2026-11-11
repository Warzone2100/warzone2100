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
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

struct WZVideoFrameSpan
{
	const uint8_t *data = nullptr;
	size_t len = 0;
};

/** Split one VP9 chunk on its superframe index into the frames it carries.
 * Yields one span covering the whole chunk for an ordinary frame, or when the index is absent or malformed. */
void wzVp9SplitSuperframe(const uint8_t *data, size_t len, std::vector<WZVideoFrameSpan>& out);

bool wzVp9FrameIsKeyframe(const uint8_t *data, size_t len);

struct WZVp9ColorConfig
{
	unsigned colorSpace = 0;	// VP9 color_space: 0 unknown, 1 BT.601, 2 BT.709, 3 SMPTE 170, 4 SMPTE 240, 5 BT.2020, 7 sRGB
	bool fullRange = false;
};

/** Read the color description a key frame (or an intra-only frame) carries.
 * \returns false for a frame that carries none */
bool wzVp9ReadColorConfig(const uint8_t *data, size_t len, WZVp9ColorConfig& out);
