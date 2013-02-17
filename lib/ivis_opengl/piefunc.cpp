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
#include "lib/framework/frameint.h"
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

#define VW_VERTICES 5
static Vector3i pieVrts[VW_VERTICES];
static PIELIGHT pieColour;

void pie_SetViewingWindow(Vector3i *v, PIELIGHT colour)
{
	pieColour = colour;

	pieVrts[0] = v[1];
	pieVrts[0].z = INTERFACE_DEPTH;	// cull triangles with off screen points

	pieVrts[1] = v[0];
	pieVrts[1].z = INTERFACE_DEPTH;

	pieVrts[2] = v[2];
	pieVrts[2].z = INTERFACE_DEPTH;

	pieVrts[3] = v[3];
	pieVrts[3].z = INTERFACE_DEPTH;

	pieVrts[4] = pieVrts[0];
}

void pie_DrawViewingWindow()
{
	SDWORD i;
	PIELIGHT colour = pieColour;

	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetRendMode(REND_ALPHA);

	glColor4ub(colour.byte.r, colour.byte.g, colour.byte.b, colour.byte.a >> 1);
	glBegin(GL_TRIANGLE_FAN);
		for (i = 0; i < VW_VERTICES; i++)
		{
			glVertex2f(pieVrts[i].x, pieVrts[i].y);
		}
	glEnd();

	glColor4ub(colour.byte.r, colour.byte.g, colour.byte.b, colour.byte.a);
	glBegin(GL_LINE_STRIP);
		for (i = 0; i < VW_VERTICES; i++)
		{
			glVertex2f(pieVrts[i].x, pieVrts[i].y);
		}
		glVertex2f(pieVrts[0].x, pieVrts[0].y);
	glEnd();
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
	skyboxGfx = new GFX(GL_QUAD_STRIP, 3);
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

	// for the nice blend of the sky with the fog
	glDisable(GL_ALPHA_TEST);

	// Apply scale matrix
	glScalef(scale, scale/2.0f, scale);

	skyboxGfx->draw();

	glPopAttrib();
}
