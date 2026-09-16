/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/** \file
 *  Extended render routines for 3D rendering.
 */

#include "lib/framework/frame.h"

#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "piematrix.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/pielight_convert.h"
#include <glm/gtc/type_ptr.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>
#include <array>

static GFX *skyboxGfx = nullptr;
static GFX *radarViewGfx[2] = { nullptr, nullptr };
static bool radarViewEmpty = true;

void pie_SetViewingWindow(const std::vector<Vector3i> &points, PIELIGHT colour)
{
	if (!radarViewGfx[0])
	{
		radarViewGfx[0] = new GFX(GFX_COLOUR, 2);
		radarViewGfx[1] = new GFX(GFX_COLOUR, 2);
	}
	const size_t count = points.size();
	radarViewEmpty = count < 3;
	if (radarViewEmpty)
	{
		return;
	}
	std::vector<gfx_api::gfxFloat> vert((count + 1) * 2);
	std::vector<uint8_t> cols((count + 1) * 4);
	for (size_t i = 0; i < cols.size(); i += 4)
	{
		cols[i + 0] = colour.byte.r;
		cols[i + 1] = colour.byte.g;
		cols[i + 2] = colour.byte.b;
		cols[i + 3] = colour.byte.a >> 1;
	}

	// Inside fill: a triangle strip zigzagging across the polygon (0, 1, n-1, 2, n-2, ...)
	for (size_t i = 0, low = 1, high = count - 1; i < count; ++i)
	{
		const size_t index = i == 0 ? 0 : (i % 2 == 1 ? low++ : high--);
		vert[i * 2 + 0] = static_cast<gfx_api::gfxFloat>(points[index].x);
		vert[i * 2 + 1] = static_cast<gfx_api::gfxFloat>(points[index].y);
	}
	radarViewGfx[0]->buffers(static_cast<int>(count), vert.data(), cols.data());

	// Outline: around, back to the first point
	for (size_t i = 0; i <= count; ++i)
	{
		vert[i * 2 + 0] = static_cast<gfx_api::gfxFloat>(points[i % count].x);
		vert[i * 2 + 1] = static_cast<gfx_api::gfxFloat>(points[i % count].y);
	}
	for (size_t i = 0; i < cols.size(); i += 4)
	{
		cols[i + 3] = colour.byte.a;
	}
	radarViewGfx[1]->buffers(static_cast<int>(count + 1), vert.data(), cols.data());
}

bool pie_HasViewingWindow()
{
	return !radarViewEmpty && radarViewGfx[0] != nullptr;
}

void pie_DrawViewingWindow(const glm::mat4 &modelViewProjectionMatrix)
{
	if (radarViewEmpty || !radarViewGfx[0])
	{
		return;
	}
	gfx_api::RadarViewInsideFillPSO::get().bind();
	gfx_api::RadarViewInsideFillPSO::get().bind_constants({ modelViewProjectionMatrix });
	radarViewGfx[0]->draw<gfx_api::RadarViewInsideFillPSO>(modelViewProjectionMatrix);
	gfx_api::RadarViewOutlinePSO::get().bind();
	gfx_api::RadarViewOutlinePSO::get().bind_constants({ modelViewProjectionMatrix });
	radarViewGfx[1]->draw<gfx_api::RadarViewOutlinePSO>(modelViewProjectionMatrix);
}

void pie_ViewingWindow_Shutdown()
{
	if (radarViewGfx[0])
	{
		delete radarViewGfx[0];
		radarViewGfx[0] = nullptr;
	}
	if (radarViewGfx[1])
	{
		delete radarViewGfx[1];
		radarViewGfx[1] = nullptr;
	}
	radarViewEmpty = true;
}

void pie_DebugDrawTessellationTestPatch()
{
	static const bool enabled = []() {
		const char* env = getenv("WZ_DEBUG_TESS_TEST");
		return env != nullptr && *env != '\0' && strcmp(env, "0") != 0;
	}();
	if (!enabled)
	{
		return;
	}
	if (!gfx_api::context::get().supportsTessellationShaders())
	{
		static bool loggedOnce = false;
		if (!loggedOnce)
		{
			debug(LOG_INFO, "WZ_DEBUG_TESS_TEST: tessellation shaders are not supported on this system");
			loggedOnce = true;
		}
		return;
	}
	static bool loggedLevel = false;
	if (!loggedLevel)
	{
		debug(LOG_INFO, "WZ_DEBUG_TESS_TEST: drawing test patch (MAX_TESS_GEN_LEVEL: %d)", (int)gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_TESS_GEN_LEVEL));
		loggedLevel = true;
	}
	const float tessLevel = std::min(8.f, static_cast<float>(gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_TESS_GEN_LEVEL)));
	// 4 control points in NDC (lower-left area of the screen), CCW winding
	static const std::array<glm::vec2, 4> patchCorners = {
		glm::vec2(-0.9f, -0.9f), glm::vec2(-0.3f, -0.9f), glm::vec2(-0.3f, -0.3f), glm::vec2(-0.9f, -0.3f)
	};
	gfx_api::DebugDrawTessQuad::get().bind();
	gfx_api::DebugDrawTessQuad::get().bind_constants({ glm::mat4(1.f), glm::vec4(0.1f, 0.9f, 0.4f, 0.9f), tessLevel });
	gfx_api::context::get().bind_streamed_vertex_buffers(patchCorners.data(), patchCorners.size() * sizeof(glm::vec2));
	gfx_api::DebugDrawTessQuad::get().draw(4, 0);
	gfx_api::context::get().disable_all_vertex_buffers();
}

void pie_TransColouredTriangle(const std::array<Vector3f, 3> &vrt, PIELIGHT c, const glm::mat4 &modelViewMatrix)
{
	glm::vec4 color(c.byte.r / 255.f, c.byte.g / 255.f, c.byte.b / 255.f, 128.f / 255.f);
	const auto& renderState = getCurrentRenderState();
	const glm::vec4 fogRange(renderState.fogBegin, renderState.fogEnd, pie_GetFogEnabled() ? 1.f : 0.f, gfx_api::context::get().linearLightingEnabled() ? 1.f : 0.f);

	gfx_api::TransColouredTrianglePSO::get().bind();
	gfx_api::TransColouredTrianglePSO::get().bind_constants({
		pie_UIPerspectiveGet() * modelViewMatrix,
		modelViewMatrix,
		color,
		pie_SceneColourToVec4(pie_GetFogColour()),
		fogRange
	});
	gfx_api::context::get().bind_streamed_vertex_buffers(vrt.data(), 3 * sizeof(Vector3f));
	gfx_api::TransColouredTrianglePSO::get().draw(3, 0);
	gfx_api::context::get().disable_all_vertex_buffers();
}

void pie_Skybox_Init()
{
	float narrowingOfTop = 0.15f;
	float top = 1;
	float middle = 0.15f;
	float baseline = -0.15f - 0.3f; // Additional lowering of baseline for where fog starts fading, to reduce certain artifacts
	float bottom = -1;

	// Skybox looks like this from the side:
	//
	//             _____      top, slightly narrower to give perspective
	//            /     .
	//           /       .
	//           +-------+    middle
	//           |       |
	//           +-------+    baseline of map
	//           |       |
	//           +-------+    bottom
	// bottom cap ---^

	// The skybox is shown from baseline to top. below the baseline, the color fades to black (if fog enabled - otherwise a color of the skybox bottom).
	// We extended the skybox downwards to be able to paint more of the screen with fog.

	float narrowCoordinate = 1 - narrowingOfTop;
	const Vector3f
		northWestBottom = Vector3f(-1, bottom, 1), // nw
		northWestMiddle = Vector3f(-1, middle, 1),
		northWestBaseline = Vector3f(-1, baseline, 1),
		northWestTop = Vector3f(-narrowCoordinate, top, narrowCoordinate),
		northEastBottom = Vector3f(1, bottom, 1), // ne
		northEastTop = Vector3f(narrowCoordinate, top, narrowCoordinate),
		northEastMiddle = Vector3f(1, middle, 1),
		northEastBaseline = Vector3f(1, baseline, 1),
		southEastBottom = Vector3f(1, bottom, -1), // se
		southEastTop = Vector3f(narrowCoordinate, top, -narrowCoordinate),
		southEastMiddle = Vector3f(1, middle, -1),
		southEastBaseline = Vector3f(1, baseline, -1),
		southWestBottom = Vector3f(-1, bottom, -1), // sw
		southWestMiddle = Vector3f(-1, middle, -1),
		southWestBaseline = Vector3f(-1, baseline, -1),
		southWestTop = Vector3f(-narrowCoordinate, top, -narrowCoordinate);

	const std::array<Vector3f, 78> vertex{
		// North quads
		northWestMiddle, northWestTop, northEastMiddle,
		northEastTop, northEastMiddle, northWestTop,
		northWestBaseline, northWestMiddle, northEastBaseline,
		northEastMiddle, northEastBaseline, northWestMiddle,
		northWestBottom, northWestBaseline, northEastBottom,
		northEastBaseline, northEastBottom, northWestBaseline,
		// East quads
		northEastMiddle, northEastTop, southEastMiddle,
		southEastTop, southEastMiddle, northEastTop,
		northEastBaseline, northEastMiddle, southEastBaseline,
		southEastMiddle, southEastBaseline, northEastMiddle,
		northEastBottom, northEastBaseline, southEastBottom,
		southEastBaseline, southEastBottom, northEastBaseline,
		// South quads
		southEastMiddle, southEastTop, southWestMiddle,
		southWestTop, southWestMiddle, southEastTop,
		southEastBaseline, southEastMiddle, southWestBaseline,
		southWestMiddle, southWestBaseline, southEastMiddle,
		southEastBottom, southEastBaseline, southWestBottom,
		southWestBaseline, southWestBottom, southEastBaseline,
		// West quads
		southWestMiddle, southWestTop, northWestMiddle,
		northWestTop, northWestMiddle, southWestTop,
		southWestBaseline, southWestMiddle, northWestBaseline,
		northWestMiddle, northWestBaseline, southWestMiddle,
		southWestBottom, southWestBaseline, northWestBottom,
		northWestBaseline, northWestBottom, southWestBaseline,
		// Bottom quads
		southWestBottom, northWestBottom, northEastBottom,
		northEastBottom, southEastBottom, southWestBottom,
	};
	const Vector2f
		uvTopLeft = Vector2f(0, 0.01), // fractions of 0 and 1 to avoid mipmap bleeding
		uvMiddleLeft = Vector2f(0, 1 - middle),
		uvBaselineLeft = Vector2f(0, 0.99),
		uvTopRight = Vector2f(2, 0.01),     // 2 = each side of the skybox contains 2 repeats of the same texture
		uvMiddleRight = Vector2f(2, 1 - middle),
		uvBaselineRight = Vector2f(2, 0.99),
		uvNull = Vector2f(0, 0.99); // = single color, blend in with sides.

	const std::array<Vector2f, 78> texc =
	{
		// North quad
		uvMiddleLeft, uvTopLeft, uvMiddleRight,
		uvTopRight, uvMiddleRight, uvTopLeft,
		uvBaselineLeft, uvMiddleLeft, uvBaselineRight,
		uvMiddleRight, uvBaselineRight, uvMiddleLeft,
		uvNull, uvNull, uvNull, // bottom part has no color.
		uvNull, uvNull, uvNull,
		// East quad
		uvMiddleLeft, uvTopLeft, uvMiddleRight,
		uvTopRight, uvMiddleRight, uvTopLeft,
		uvBaselineLeft, uvMiddleLeft, uvBaselineRight,
		uvMiddleRight, uvBaselineRight, uvMiddleLeft,
		uvNull, uvNull, uvNull, // bottom part has no color.
		uvNull, uvNull, uvNull,
		// West quad
		uvMiddleLeft, uvTopLeft, uvMiddleRight,
		uvTopRight, uvMiddleRight, uvTopLeft,
		uvBaselineLeft, uvMiddleLeft, uvBaselineRight,
		uvMiddleRight, uvBaselineRight, uvMiddleLeft,
		uvNull, uvNull, uvNull, // bottom part has no color.
		uvNull, uvNull, uvNull,
		// South quad
		uvMiddleLeft, uvTopLeft, uvMiddleRight,
		uvTopRight, uvMiddleRight, uvTopLeft,
		uvBaselineLeft, uvMiddleLeft, uvBaselineRight,
		uvMiddleRight, uvBaselineRight, uvMiddleLeft,
		uvNull, uvNull, uvNull, // bottom part has no color.
		uvNull, uvNull, uvNull,
		// Bottom quad
		uvNull, uvNull, uvNull, // bottom cap has no color
		uvNull, uvNull, uvNull,
	};

	gfx_api::context::get().debugStringMarker("Initializing skybox");
	skyboxGfx = new GFX(GFX_TEXTURE, 3);
	skyboxGfx->buffers(78, vertex.data(), texc.data());
}

void pie_Skybox_Texture(const char *filename)
{
	skyboxGfx->loadTexture(filename, gfx_api::texture_type::game_texture);
}

gfx_api::texture* pie_Skybox_GetTexture()
{
	return skyboxGfx != nullptr ? skyboxGfx->getTexture() : nullptr;
}

void pie_Skybox_Shutdown()
{
	delete skyboxGfx;
	skyboxGfx = nullptr;
}

void pie_DrawSkybox(float scale, const glm::mat4& projectionMatrix, const glm::mat4 &viewMatrix)
{
	const auto& modelViewProjectionMatrix = projectionMatrix * viewMatrix * glm::scale(glm::vec3(scale, scale / 2.f, scale));

	const auto &renderState = getCurrentRenderState();
	const glm::vec4 fogColor = pielightToRGBAVec4(renderState.fogColour);

	gfx_api::SkyboxPSO::get().bind();
	gfx_api::SkyboxPSO::get().bind_constants({ modelViewProjectionMatrix, glm::vec4(1.f), fogColor, renderState.fogEnabled, gfx_api::context::get().linearLightingEnabled() ? 1 : 0 });
	skyboxGfx->draw<gfx_api::SkyboxPSO>(modelViewProjectionMatrix);
}
