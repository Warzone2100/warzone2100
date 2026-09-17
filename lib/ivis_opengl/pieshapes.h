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

#ifndef __INCLUDED_LIB_IVIS_OPENGL_PIESHAPES_H__
#define __INCLUDED_LIB_IVIS_OPENGL_PIESHAPES_H__

#include "pieblitfunc.h"

#include <glm/vec2.hpp>
#include <memory>
#include <vector>

/// UI shapes: lines of any width (with joins, and dashes) and filled areas, gathered and drawn at once.
/// Anti-aliased by a pixel-wide fade at their edges (in the vertices' alpha, so the colored-vertex shader draws them)
class PieShapes
{
public:
	void clear();
	bool empty() const { return colours.empty(); }

	/// A line through `points`, `width` px wide, dashed when dashLength > 0 (dashes `dashLength` long, `dashGap` apart)
	void addPolyline(const std::vector<glm::vec2> &points, float width, PIELIGHT colour, float dashLength = 0.f, float dashGap = 0.f);
	/// The area between the line through `points` and the level `baseY`: `atLine` at the line, fading to `atBase` at baseY
	void addArea(const std::vector<glm::vec2> &points, float baseY, PIELIGHT atLine, PIELIGHT atBase);
	/// A filled convex polygon, its edge anti-aliased
	void addConvexPolygon(const std::vector<glm::vec2> &points, PIELIGHT colour);

	/// Draws them, moved by (x, y)
	void draw(float x, float y);

private:
	void vertex(glm::vec2 position, PIELIGHT colour);
	void quad(glm::vec2 a, PIELIGHT ca, glm::vec2 b, PIELIGHT cb, glm::vec2 c, PIELIGHT cc, glm::vec2 d, PIELIGHT cd);

	std::vector<gfx_api::gfxFloat> vertices;
	std::vector<uint8_t> colours;
	std::unique_ptr<GFX> gfx;
	bool uploaded = false;
};

#endif // __INCLUDED_LIB_IVIS_OPENGL_PIESHAPES_H__
