/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "lib/framework/opengl.h"

#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "piematrix.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/pieclip.h"
#include <glm/gtc/type_ptr.hpp>
#include <array>

static GFX *skyboxGfx = NULL;
static GFX *radarViewGfx[2] = { NULL, NULL };

#define VW_VERTICES 5

void pie_SetViewingWindow(Vector3i *v, PIELIGHT colour)
{
	Vector3i pieVrts[VW_VERTICES];

	if (!radarViewGfx[0])
	{
		radarViewGfx[0] = new GFX(GFX_COLOUR, GL_TRIANGLE_FAN, 2);
		radarViewGfx[1] = new GFX(GFX_COLOUR, GL_LINE_STRIP, 2);
	}

	pieVrts[0] = v[1];
	pieVrts[1] = v[0];
	pieVrts[2] = v[2];
	pieVrts[3] = v[3];
	pieVrts[4] = v[1];

	GLfloat vert[VW_VERTICES * 2];
	GLbyte cols[VW_VERTICES * 4];
	for (int i = 0; i < VW_VERTICES; i++)
	{
		vert[i * 2 + 0] = pieVrts[i].x;
		vert[i * 2 + 1] = pieVrts[i].y;
	}
	for (int i = 0; i < VW_VERTICES * 4; i += 4)
	{
		cols[i + 0] = colour.byte.r;
		cols[i + 1] = colour.byte.g;
		cols[i + 2] = colour.byte.b;
		cols[i + 3] = colour.byte.a >> 1;
	}
	radarViewGfx[0]->buffers(VW_VERTICES, vert, cols);
	for (int i = 0; i < VW_VERTICES * 4; i += 4) // set proper alpha
	{
		cols[i + 3] = colour.byte.a;
	}
	radarViewGfx[1]->buffers(VW_VERTICES, vert, cols);
}

void pie_DrawViewingWindow(const glm::mat4 &modelViewProjectionMatrix)
{
	pie_SetRendMode(REND_ALPHA);
	radarViewGfx[0]->draw(modelViewProjectionMatrix);
	radarViewGfx[1]->draw(modelViewProjectionMatrix);
}

namespace
{
	struct glBufferWrapper
	{
		GLuint id;
		glBufferWrapper()
		{
			glGenBuffers(1, &id);
		}

		~glBufferWrapper()
		{
			glDeleteBuffers(1, &id);
		}
	};
}

void pie_TransColouredTriangle(const std::array<Vector3f, 3> &vrt, PIELIGHT c, const glm::mat4 &modelViewMatrix)
{
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetRendMode(REND_ADDITIVE);
	glm::vec4 color(c.byte.r / 255.f, c.byte.g / 255.f, c.byte.b / 255.f, 128.f / 255.f);
	const auto &program = pie_ActivateShader(SHADER_GENERIC_COLOR, pie_PerspectiveGet() * modelViewMatrix, color);

	static glBufferWrapper buffer;
	glBindBuffer(GL_ARRAY_BUFFER, buffer.id);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(Vector3f), vrt.data(), GL_STREAM_DRAW);
	glEnableVertexAttribArray(program.locVertex);
	glVertexAttribPointer(program.locVertex, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 3);
	glDisableVertexAttribArray(program.locVertex);
}

void pie_Skybox_Init()
{
	const int u = 0;
	const int v = 0;
	const int w = 1;
	const int h = 1;
	const float r = 1.0f; // just because it is shorter than 1.0f

	const Vector3f
		vertexFront0 = Vector3f(-r, 0, r), // front
		vertexFront1 = Vector3f(-r, r, r),
		vertexFront2 = Vector3f(r, 0, r),
		vertexFront3 = Vector3f(r, r, r),
		vertexRight0 = Vector3f(r, 0, -r), // right
		vertexRight1 = Vector3f(r, r, -r),
		vertexBack0 = Vector3f(-r, 0, -r), // back
		vertexBack1 = Vector3f(-r, r, -r),
		vertexLeft0 = Vector3f(-r, 0, r),
		vertexLeft1 = Vector3f(-r, r, r);

	const std::array<Vector3f, 24> vertex{
		// Front quad
		vertexFront0, vertexFront1, vertexFront2,
		vertexFront3, vertexFront2, vertexFront1,
		// Right quad
		vertexFront2, vertexFront3, vertexRight0,
		vertexRight1, vertexRight0, vertexFront3,
		// Back quad
		vertexRight0, vertexRight1, vertexBack0,
		vertexBack1, vertexBack0, vertexRight1,
		// Left quad
		vertexBack0, vertexBack1, vertexLeft0,
		vertexLeft1, vertexLeft0, vertexBack1,

	};
	const Vector2f
		uvFront0 = Vector2f(u + w * 0, (v + h)),
		uvFront1 = Vector2f(u + w * 0, v),
		uvFront2 = Vector2f(u + w * 2, v + h),
		uvFront3 = Vector2f(u + w * 2, v),
		uvRight0 = Vector2f(u + w * 4, v + h),
		uvRight1 = Vector2f(u + w * 4, v),
		uvBack0 = Vector2f(u + w * 6, v + h),
		uvBack1 = Vector2f(u + w * 6, v),
		uvLeft0 = Vector2f(u + w * 8, v + h),
		uvLeft1 = Vector2f(u + w * 8, v);

	const std::array<Vector2f, 24> texc =
	{
		// Front quad
		uvFront0, uvFront1, uvFront2,
		uvFront3, uvFront2, uvFront1,
		// Right quad
		uvFront2, uvFront3, uvRight0,
		uvRight1, uvRight0, uvFront3,
		// Back quad
		uvRight0, uvRight1, uvBack0,
		uvBack1, uvBack0, uvRight1,
		// Left quad
		uvBack0, uvBack1, uvLeft0,
		uvLeft1, uvLeft0, uvBack1,
	};

	GL_DEBUG("Initializing skybox");
	skyboxGfx = new GFX(GFX_TEXTURE, GL_TRIANGLES, 3);
	skyboxGfx->buffers(24, vertex.data(), texc.data());
}

void pie_Skybox_Texture(const char *filename)
{
	skyboxGfx->loadTexture(filename);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
}

void pie_Skybox_Shutdown()
{
	delete skyboxGfx;
}

void pie_DrawSkybox(float scale, const glm::mat4 &viewMatrix)
{
	// no use in updating the depth buffer
	glDepthMask(GL_FALSE);
	// enable alpha
	pie_SetRendMode(REND_ALPHA);

	// Apply scale matrix
	skyboxGfx->draw(pie_PerspectiveGet() * viewMatrix * glm::scale(scale, scale / 2.f, scale));
}
