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
#include <glm/gtc/type_ptr.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>
#include <array>

static GFX *skyboxGfx = nullptr;
static GFX *radarViewGfx[2] = { nullptr, nullptr };

#define VW_VERTICES 5

void pie_SetViewingWindow(Vector3i *v, PIELIGHT colour)
{
	Vector3i pieVrts_insidefill[VW_VERTICES - 1];
	Vector3i pieVrts_outline[VW_VERTICES];

	if (!radarViewGfx[0])
	{
		radarViewGfx[0] = new GFX(GFX_COLOUR, 2);
		radarViewGfx[1] = new GFX(GFX_COLOUR, 2);
	}

	static_assert(VW_VERTICES == 5, "Assumption that VW_VERTICES == 5 invalid. Update the following code.");
	pieVrts_outline[0] = v[1];
	pieVrts_outline[1] = v[0];
	pieVrts_outline[2] = v[2];
	pieVrts_outline[3] = v[3];
	pieVrts_outline[4] = v[1];
	pieVrts_insidefill[0] = v[2];
	pieVrts_insidefill[1] = v[3];
	pieVrts_insidefill[2] = v[0];
	pieVrts_insidefill[3] = v[1];

	gfx_api::gfxFloat vert[VW_VERTICES * 2];
	uint8_t cols[VW_VERTICES * 4];

	// Initialize the RGB values for both, and the alpha for the inside fill
	for (int i = 0; i < VW_VERTICES * 4; i += 4)
	{
		cols[i + 0] = colour.byte.r;
		cols[i + 1] = colour.byte.g;
		cols[i + 2] = colour.byte.b;
		cols[i + 3] = colour.byte.a >> 1;
	}

	// Inside fill
	for (int i = 0; i < (VW_VERTICES - 1); i++)
	{
		vert[i * 2 + 0] = pieVrts_insidefill[i].x;
		vert[i * 2 + 1] = pieVrts_insidefill[i].y;
	}
	radarViewGfx[0]->buffers(VW_VERTICES - 1, vert, cols);

	// Outline
	for (int i = 0; i < VW_VERTICES; i++)
	{
		vert[i * 2 + 0] = pieVrts_outline[i].x;
		vert[i * 2 + 1] = pieVrts_outline[i].y;
	}
	for (int i = 0; i < VW_VERTICES * 4; i += 4) // set the proper alpha for the outline
	{
		cols[i + 3] = colour.byte.a;
	}
	radarViewGfx[1]->buffers(VW_VERTICES, vert, cols);
}

void pie_DrawViewingWindow(const glm::mat4 &modelViewProjectionMatrix)
{
	gfx_api::RadarViewInsideFillPSO::get().bind();
	gfx_api::RadarViewInsideFillPSO::get().bind_constants({ modelViewProjectionMatrix });
	radarViewGfx[0]->draw<gfx_api::RadarViewInsideFillPSO>(modelViewProjectionMatrix);
	gfx_api::RadarViewOutlinePSO::get().bind();
	gfx_api::RadarViewOutlinePSO::get().bind_constants({ modelViewProjectionMatrix });
	radarViewGfx[1]->draw<gfx_api::RadarViewOutlinePSO>(modelViewProjectionMatrix);
}

void pie_ViewingWindow_Shutdown()
{
	delete radarViewGfx[0];
	radarViewGfx[0] = nullptr;
	delete radarViewGfx[1];
	radarViewGfx[1] = nullptr;
}

void pie_TransColouredTriangle(const std::array<Vector3f, 3> &vrt, PIELIGHT c, const glm::mat4 &modelViewMatrix)
{
	glm::vec4 color(c.byte.r / 255.f, c.byte.g / 255.f, c.byte.b / 255.f, 128.f / 255.f);

	gfx_api::TransColouredTrianglePSO::get().bind();
	gfx_api::TransColouredTrianglePSO::get().bind_constants({ pie_PerspectiveGet() * modelViewMatrix, glm::vec2(0), glm::vec2(0), color });
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
	skyboxGfx->loadTexture(filename);
}

void pie_Skybox_Shutdown()
{
	delete skyboxGfx;
	skyboxGfx = nullptr;
}

void pie_DrawSkybox(float scale, const glm::mat4 &viewMatrix)
{
	const auto& modelViewProjectionMatrix = pie_PerspectiveGet() * viewMatrix * glm::scale(glm::vec3(scale, scale / 2.f, scale));

	const auto &renderState = getCurrentRenderState();
	const glm::vec4 fogColor(
		renderState.fogColour.vector[0] / 255.f,
		renderState.fogColour.vector[1] / 255.f,
		renderState.fogColour.vector[2] / 255.f,
		renderState.fogColour.vector[3] / 255.f
	);

	gfx_api::SkyboxPSO::get().bind();
	gfx_api::SkyboxPSO::get().bind_constants({ modelViewProjectionMatrix, glm::vec4(1.f), fogColor, renderState.fogEnabled });
	skyboxGfx->draw<gfx_api::SkyboxPSO>(modelViewProjectionMatrix);
}
