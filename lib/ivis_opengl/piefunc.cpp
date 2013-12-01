/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

void pie_DrawViewingWindow()
{
	pie_SetRendMode(REND_ALPHA);
	radarViewGfx[0]->draw();
	radarViewGfx[1]->draw();
}

void pie_TransColouredTriangle(Vector3f *vrt, PIELIGHT c)
{
	UDWORD i;

	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetRendMode(REND_ADDITIVE);

	glColor4ub(c.byte.r, c.byte.g, c.byte.b, 128);

	glBegin(GL_TRIANGLE_FAN);
		for (i = 0; i < 3; ++i)
		{
			glVertex3f(vrt[i].x, vrt[i].y, vrt[i].z);
		}
	glEnd();
}

void pie_Skybox_Init()
{
	const int u = 0;
	const int v = 0;
	const int w = 1;
	const int h = 1;
	const float r = 1.0f; // just because it is shorter than 1.0f
	GLfloat vert[] = { -r, 0, r, -r, r, r, r, 0, r, r, r, r, // front
	                    r, 0,-r, r, r,-r, // right
	                   -r, 0, -r, -r, r, -r, // back
	                   -r, 0, r, -r, r, r };
	GLfloat texc[] = { u + w * 0, v + h, u + w * 0, v, u + w * 2, v + h, u + w * 2, v,
	                   u + w * 4, v + h, u + w * 4, v,
	                   u + w * 6, v + h, u + w * 6, v,
	                   u + w * 8, v + h, u + w * 8, v };

	GL_DEBUG("Initializing skybox");
	skyboxGfx = new GFX(GFX_TEXTURE, GL_QUAD_STRIP, 3);
	skyboxGfx->buffers(10, vert, texc);
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

void pie_DrawSkybox(float scale)
{
	GL_DEBUG("Drawing skybox");

	glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_FOG_BIT);
	// no use in updating the depth buffer
	glDepthMask(GL_FALSE);

	// fog should not affect the sky
	glDisable(GL_FOG);

	// So we have realistic colors
	glColor4ub(0xFF,0xFF,0xFF,0xFF);

	// enable alpha
	pie_SetRendMode(REND_ALPHA);

	// Apply scale matrix
	glScalef(scale, scale/2.0f, scale);

	skyboxGfx->draw();

	glPopAttrib();
}
