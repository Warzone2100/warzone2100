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
 *  The radar's blips in the Modern skin (proximity messages, oil resources, burning oil): a dot with a pulsing ring, a
 *  hollow dot once read, and a flickering flame for burning oil (images/hud/icons/radar-blip-*.svg), crisp at any
 *  scale. All of them are in one texture, rasterized once for the display scale, and the radar's blips are drawn in
 *  one batch (the pipeline and the texture bound once).
 */

#ifndef __INCLUDED_SRC_HUD_HUDRADARBLIPS_H__
#define __INCLUDED_SRC_HUD_HUDRADARBLIPS_H__

#include "lib/framework/vector.h"

#include <glm/mat4x4.hpp>

#include <cstdint>

enum class HudRadarBlip : uint8_t
{
	Enemy,
	Resource,
	Artifact,
	BurningResource,
};

/// Queues a blip centered at x, y (in the radar's space): `frame` of its animation (0-2: the ring's pulse, or the
/// flame's flicker), or -1 once read. False if it can't be drawn (the icons are missing): draw the image instead
bool hudQueueRadarBlip(HudRadarBlip kind, int frame, float x, float y);
/// Draws the queued blips, through the radar's transform
void hudDrawQueuedRadarBlips(const glm::mat4 &modelViewProjection);
/// Frees the texture (the interface shuts down)
void hudReleaseRadarBlips();

#endif // __INCLUDED_SRC_HUD_HUDRADARBLIPS_H__
