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

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/pieshapes.h"
#include "lib/ivis_opengl/piematrix.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

void PieShapes::clear()
{
	vertices.clear();
	colours.clear();
	uploaded = false;
}

void PieShapes::vertex(glm::vec2 position, PIELIGHT colour)
{
	vertices.push_back(position.x);
	vertices.push_back(position.y);
	colours.push_back(colour.byte.r);
	colours.push_back(colour.byte.g);
	colours.push_back(colour.byte.b);
	colours.push_back(colour.byte.a);
	uploaded = false;
}

void PieShapes::quad(glm::vec2 a, PIELIGHT ca, glm::vec2 b, PIELIGHT cb, glm::vec2 c, PIELIGHT cc, glm::vec2 d, PIELIGHT cd)
{
	vertex(a, ca);
	vertex(b, cb);
	vertex(c, cc);
	vertex(a, ca);
	vertex(c, cc);
	vertex(d, cd);
}

void PieShapes::addPolyline(const std::vector<glm::vec2> &input, float width, PIELIGHT colour, float dashLength, float dashGap)
{
	std::vector<glm::vec2> points;
	for (const glm::vec2 &p : input)
	{
		if (points.empty() || glm::length(p - points.back()) > 0.01f)
		{
			points.push_back(p);
		}
	}
	if (points.size() < 2 || width <= 0.f)
	{
		return;
	}

	if (dashLength > 0.f)
	{
		const float gap = std::max(dashGap, 0.f);
		std::vector<glm::vec2> dash;
		bool drawing = true;
		float left = dashLength;
		for (size_t i = 0; i + 1 < points.size(); ++i)
		{
			glm::vec2 from = points[i];
			const glm::vec2 to = points[i + 1];
			if (drawing && dash.empty())
			{
				dash.push_back(from);
			}
			float segment = glm::length(to - from);
			while (segment > left)
			{
				from += (to - from) * (left / segment);
				segment -= left;
				if (drawing)
				{
					dash.push_back(from);
					addPolyline(dash, width, colour);
					dash.clear();
					drawing = gap <= 0.f;
					left = drawing ? dashLength : gap;
				}
				else
				{
					drawing = true;
					left = dashLength;
				}
				if (drawing)
				{
					dash.push_back(from);
				}
			}
			left -= segment;
			if (drawing)
			{
				dash.push_back(to);
			}
		}
		if (dash.size() >= 2)
		{
			addPolyline(dash, width, colour);
		}
		return;
	}

	// The normals at each point (mitered at the joins, the miter's length limited)
	const size_t count = points.size();
	std::vector<glm::vec2> normals(count);
	auto segmentNormal = [&points](size_t i) {
		const glm::vec2 d = glm::normalize(points[i + 1] - points[i]);
		return glm::vec2(-d.y, d.x);
	};
	for (size_t i = 0; i < count; ++i)
	{
		if (i == 0)
		{
			normals[i] = segmentNormal(0);
		}
		else if (i + 1 == count)
		{
			normals[i] = segmentNormal(i - 1);
		}
		else
		{
			glm::vec2 m = (segmentNormal(i - 1) + segmentNormal(i)) * 0.5f;
			const float lengthSquared = glm::dot(m, m);
			if (lengthSquared > 0.000001f)
			{
				m *= std::min(1.f / lengthSquared, 4.f);
			}
			normals[i] = m;
		}
	}

	// A solid core, and a pixel's fade on each side (a line thinner than a pixel is a fainter pixel-wide line)
	const float core = std::max(0.f, (width - 1.f) * 0.5f);
	PIELIGHT solid = colour;
	if (width < 1.f)
	{
		solid.byte.a = static_cast<uint8_t>(colour.byte.a * width);
	}
	PIELIGHT clear = solid;
	clear.byte.a = 0;
	for (size_t i = 0; i + 1 < count; ++i)
	{
		const glm::vec2 p0 = points[i], p1 = points[i + 1];
		const glm::vec2 n0 = normals[i], n1 = normals[i + 1];
		const glm::vec2 in0a = p0 + n0 * core, in0b = p0 - n0 * core, in1a = p1 + n1 * core, in1b = p1 - n1 * core;
		const glm::vec2 out0a = p0 + n0 * (core + 1.f), out0b = p0 - n0 * (core + 1.f), out1a = p1 + n1 * (core + 1.f), out1b = p1 - n1 * (core + 1.f);
		quad(out0a, clear, in0a, solid, in1a, solid, out1a, clear);
		if (core > 0.f)
		{
			quad(in0a, solid, in0b, solid, in1b, solid, in1a, solid);
		}
		quad(in0b, solid, out0b, clear, out1b, clear, in1b, solid);
	}
}

void PieShapes::addArea(const std::vector<glm::vec2> &points, float baseY, PIELIGHT atLine, PIELIGHT atBase)
{
	for (size_t i = 0; i + 1 < points.size(); ++i)
	{
		const glm::vec2 a = points[i], b = points[i + 1];
		quad(a, atLine, glm::vec2(a.x, baseY), atBase, glm::vec2(b.x, baseY), atBase, b, atLine);
	}
}

void PieShapes::addConvexPolygon(const std::vector<glm::vec2> &points, PIELIGHT colour)
{
	if (points.size() < 3)
	{
		return;
	}
	glm::vec2 centre(0.f, 0.f);
	for (const glm::vec2 &p : points)
	{
		centre += p;
	}
	centre /= static_cast<float>(points.size());
	// Inset by half a pixel, then a pixel-wide outline fades the edge
	std::vector<glm::vec2> inset;
	for (const glm::vec2 &p : points)
	{
		const glm::vec2 d = p - centre;
		const float length = glm::length(d);
		inset.push_back(length > 0.5f ? centre + d * ((length - 0.5f) / length) : centre);
	}
	for (size_t i = 0; i < inset.size(); ++i)
	{
		vertex(centre, colour);
		vertex(inset[i], colour);
		vertex(inset[(i + 1) % inset.size()], colour);
	}
	inset.push_back(inset.front());
	addPolyline(inset, 1.f, colour);
}

void PieShapes::draw(float x, float y)
{
	if (colours.empty())
	{
		return;
	}
	if (!gfx)
	{
		gfx = std::make_unique<GFX>(GFX_COLOUR, 2);
	}
	if (!uploaded)
	{
		gfx->buffers(static_cast<int>(colours.size() / 4), vertices.data(), colours.data());
		uploaded = true;
	}
	const glm::mat4 mvp = defaultProjectionMatrix() * glm::translate(glm::vec3(x, y, 0.f));
	gfx_api::Shapes2DPSO::get().bind();
	gfx->draw<gfx_api::Shapes2DPSO>(mvp);
}
