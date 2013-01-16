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
#include "lib/ivis_opengl/pieclip.h"

/*
 *	Source
 */

void pie_ClipBegin(int x1, int y1, int x2, int y2)
{
	glEnable(GL_SCISSOR_TEST);
	glScissor(x1, screenHeight - y2, x2 - x1, y2 - y1);
}

void pie_ClipEnd()
{
	glDisable(GL_SCISSOR_TEST);
}

#define VW_VERTICES 5
void pie_DrawViewingWindow(Vector3i *v, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, PIELIGHT colour)
{
	CLIP_VERTEX pieVrts[VW_VERTICES];
	SDWORD i;

	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetRendMode(REND_ALPHA);

	pieVrts[0].pos.x = v[1].x;
	pieVrts[0].pos.y = v[1].y;
	//cull triangles with off screen points
	pieVrts[0].pos.z  = INTERFACE_DEPTH;

	pieVrts[0].u = 0;
	pieVrts[0].v = 0;
	pieVrts[0].light = colour;

	pieVrts[1] = pieVrts[0];
	pieVrts[2] = pieVrts[0];
	pieVrts[3] = pieVrts[0];
	pieVrts[4] = pieVrts[0];

	pieVrts[1].pos.x = v[0].x;
	pieVrts[1].pos.y = v[0].y;

	pieVrts[2].pos.x = v[2].x;
	pieVrts[2].pos.y = v[2].y;

	pieVrts[3].pos.x = v[3].x;
	pieVrts[3].pos.y = v[3].y;

	glColor4ub(colour.byte.r, colour.byte.g, colour.byte.b, colour.byte.a >> 1);
	glBegin(GL_TRIANGLE_FAN);
		for (i = 0; i < VW_VERTICES; i++)
		{
			glVertex2f(pieVrts[i].pos.x, pieVrts[i].pos.y);
		}
	glEnd();

	glColor4ub(colour.byte.r, colour.byte.g, colour.byte.b, colour.byte.a);
	glBegin(GL_LINE_STRIP);
		for (i = 0; i < VW_VERTICES; i++)
		{
			glVertex2f(pieVrts[i].pos.x, pieVrts[i].pos.y);
		}
	glVertex2f(pieVrts[0].pos.x, pieVrts[0].pos.y);
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

void pie_DrawSkybox(float scale, int u, int v, int w, int h)
{
	const float r = 1.0f; // just because it is shorter than 1.0f

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

	glBegin(GL_QUAD_STRIP);
		// Front
		glTexCoord2f(u + w * 0, v + h);	glVertex3f(-r, 0, r); // bottom left
		glTexCoord2f(u + w * 0, v);		glVertex3f(-r, r, r); // top left
		glTexCoord2f(u + w * 2, v + h);	glVertex3f( r, 0, r); // bottom right
		glTexCoord2f(u + w * 2, v); 	glVertex3f( r, r, r); // top right

		// Right
		glTexCoord2f(u + w * 4, v + h);	glVertex3f( r, 0,-r); // bottom r
		glTexCoord2f(u + w * 4, v); 	glVertex3f( r, r,-r); // top r

		// Back
		glTexCoord2f(u + w * 6, v + h);	glVertex3f(-r, 0, -r); // bottom right
		glTexCoord2f(u + w * 6, v); 	glVertex3f(-r, r, -r); // top right

		// Left
		glTexCoord2f(u + w * 8, v + h);	glVertex3f(-r, 0, r); // bottom r
		glTexCoord2f(u + w * 8, v); 	glVertex3f(-r, r, r); // top r
	glEnd();

	glPopAttrib();
}

/// Draws a fog colored box which is wider at the top
void pie_DrawFogBox(float left, float right, float front, float back, float height, float wider)
{
	PIELIGHT fog_colour = pie_GetFogColour();

	pie_SetTexturePage(TEXPAGE_NONE);

	glColor4ub(fog_colour.byte.r,fog_colour.byte.g,fog_colour.byte.b,0xFF);

	pie_SetRendMode(REND_OPAQUE);

	glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_FOG_BIT);
	// no use in updating the depth buffer
	glDepthMask(GL_FALSE);
	glDisable(GL_FOG);
	glBegin(GL_QUAD_STRIP);
		// Front
		glVertex3f(-left, 0, front); // bottom left
		glVertex3f(-left-wider, height, front+wider); // top left
		glVertex3f( right, 0, front); // bottom right
		glVertex3f( right+wider, height, front+wider); // top right

		// Right
		glVertex3f( right, 0,-back); // bottom r
		glVertex3f( right+wider, height,-back-wider); // top r

		// Back
		glVertex3f(-left, 0, -back); // bottom right
		glVertex3f(-left-wider, height, -back-wider); // top right

		// Left
		glVertex3f(-left, 0, front); // bottom r
		glVertex3f(-left-wider, height, front+wider); // top r
	glEnd();
	glPopAttrib();
}

UBYTE pie_ByteScale(UBYTE a, UBYTE b)
{
	return ((UDWORD)a * (UDWORD)b) >> 8;
}
