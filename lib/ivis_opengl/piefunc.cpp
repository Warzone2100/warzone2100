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
	const Vector3f
		northWestBelow = Vector3f(-1, 0, 1), // nw
		northWestMiddle = Vector3f(-1, 0.5, 1),
		northWestAbove = Vector3f(-0.75, 1, 0.75),
		northEastBelow = Vector3f(1, 0, 1), // ne
		northEastAbove = Vector3f(0.75, 1, 0.75),
		northEastMiddle = Vector3f(1, 0.5, 1),
		southEastBelow = Vector3f(1, 0, -1), // se
		southEastAbove = Vector3f(0.75, 1, -0.75),
		southEastMiddle = Vector3f(1, 0.5, -1),
		southWestBelow = Vector3f(-1, 0, -1), // sw
		southWestMiddle = Vector3f(-1, 0.5, -1),
		southWestAbove = Vector3f(-0.75, 1, -0.75);

	const std::array<Vector3f, 54> vertex{
		// North quads
		northWestMiddle, northWestAbove, northEastMiddle,
		northEastAbove, northEastMiddle, northWestAbove,
		northWestBelow, northWestMiddle, northEastBelow,
		northEastMiddle, northEastBelow, northWestMiddle,
		// East quads
		northEastMiddle, northEastAbove, southEastMiddle,
		southEastAbove, southEastMiddle, northEastAbove,
		northEastBelow, northEastMiddle, southEastBelow,
		southEastMiddle, southEastBelow, northEastMiddle,
		// South quads
		southEastMiddle, southEastAbove, southWestMiddle,
		southWestAbove, southWestMiddle, southEastAbove,
		southEastBelow, southEastMiddle, southWestBelow,
		southWestMiddle, southWestBelow, southEastMiddle,
		// West quads
		southWestMiddle, southWestAbove, northWestMiddle,
		northWestAbove, northWestMiddle, southWestAbove,
		southWestBelow, southWestMiddle, northWestBelow,
		northWestMiddle, northWestBelow, southWestMiddle,
		// Bottom quads
		southWestBelow, northWestBelow, northEastBelow,
		northEastBelow, southEastBelow, southWestBelow,
	};
	const Vector2f
		uvSouthWest = Vector2f(0, 0.99),     // 0.99 avoids an ugly 1px border
		uvMidWest = Vector2f(0, 0.5),
		uvNorthWest = Vector2f(0, 0),
		uvSouthEast = Vector2f(2, 0.99),     // 2 = each side of the skybox contains 2 repeats of the same texture
		uvMidEast = Vector2f(2, 0.5),
		uvNorthEast = Vector2f(2, 0);

	const std::array<Vector2f, 54> texc =
	{
		// North quad
		uvMidWest, uvNorthWest, uvMidEast,
		uvNorthEast, uvMidEast, uvNorthWest,
		uvSouthWest, uvMidWest, uvSouthEast,
		uvMidEast, uvSouthEast, uvMidWest,
		// East quad
		uvMidWest, uvNorthWest, uvMidEast,
		uvNorthEast, uvMidEast, uvNorthWest,
		uvSouthWest, uvMidWest, uvSouthEast,
		uvMidEast, uvSouthEast, uvMidWest,
		// West quad
		uvMidWest, uvNorthWest, uvMidEast,
		uvNorthEast, uvMidEast, uvNorthWest,
		uvSouthWest, uvMidWest, uvSouthEast,
		uvMidEast, uvSouthEast, uvMidWest,
		// South quad
		uvMidWest, uvNorthWest, uvMidEast,
		uvNorthEast, uvMidEast, uvNorthWest,
		uvSouthWest, uvMidWest, uvSouthEast,
		uvMidEast, uvSouthEast, uvMidWest,
		// Bottom quad
		uvSouthWest, uvSouthWest, uvSouthWest, // same coordinates in bottom part = blend in with walls.
		uvSouthWest, uvSouthWest, uvSouthWest,
	};

	gfx_api::context::get().debugStringMarker("Initializing skybox");
	skyboxGfx = new GFX(GFX_TEXTURE, 3);
	skyboxGfx->buffers(54, vertex.data(), texc.data());
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
	gfx_api::SkyboxPSO::get().bind_constants({ modelViewProjectionMatrix, glm::vec2(0.f), glm::vec2(0.f), glm::vec4(1.f), 0, renderState.fogEnabled, fogColor });
	skyboxGfx->draw<gfx_api::SkyboxPSO>(modelViewProjectionMatrix);
}
