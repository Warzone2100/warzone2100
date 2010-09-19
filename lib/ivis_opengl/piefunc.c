/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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

#include <GLee.h>
#include "lib/framework/frame.h"
#include "lib/framework/frameint.h"

#include "lib/gamelib/gtime.h"
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/piefunc.h"
#include "lib/ivis_common/piestate.h"
#include "piematrix.h"
#include "lib/ivis_common/piemode.h"
#include "lib/ivis_common/pieclip.h"

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

void pie_DrawViewingWindow(Vector3i *v, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, PIELIGHT colour)
{
	const Vector2i vertices[] = {
		{ v[1].x, v[1].y },
		{ v[0].x, v[0].y },
		{ v[2].x, v[2].y },
		{ v[3].x, v[3].y },
		{ v[1].x, v[1].y },
	};
	const int alpha = colour.byte.a;

	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetRendMode(REND_ALPHA);

	glVertexPointer(2, GL_INT, 0, vertices);
	glEnableClientState(GL_VERTEX_ARRAY);

	colour.byte.a = alpha / 2;
	glColor4ubv(colour.vector);
	glDrawArrays(GL_TRIANGLE_FAN, 0, ARRAY_SIZE(vertices));

	colour.byte.a = alpha;
	glColor4ubv(colour.vector);
	glDrawArrays(GL_LINE_STRIP, 0, ARRAY_SIZE(vertices));

	glDisableClientState(GL_VERTEX_ARRAY);
}

void pie_TransColouredTriangle(Vector3f *vrt, PIELIGHT c)
{
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetRendMode(REND_ADDITIVE);

	glColor4ub(c.byte.r, c.byte.g, c.byte.b, 128);

	glVertexPointer(3, GL_FLOAT, 0, vrt);
	glEnableClientState(GL_VERTEX_ARRAY);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 3);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void pie_DrawSkybox(float scale, int u, int v, int w, int h)
{
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

	glMatrixMode(GL_TEXTURE);
	glPushMatrix(); // texture matrix
	glTranslatef(u, v, 0);
	glScalef(w, h, 1);
	{
		const Vector3i vertices[] = {
			// Front
			{ -1,  0,  1 }, // bottom left
			{ -1,  1,  1 }, // top left
			{  1,  0,  1 }, // bottom right
			{  1,  1,  1 }, // top right
			// Right
			{  1,  0, -1 }, // bottom r
			{  1,  1, -1 }, // top r
			// Back
			{ -1,  0, -1 }, // bottom right
			{ -1,  1, -1 }, // top right
			// Left
			{ -1,  0,  1 }, // bottom r
			{ -1,  1,  1 }, // top r
		};
		const Vector2i texcoords[] = {
			// Front
			{ 0, 1 }, // bottom left
			{ 0, 0 }, // top left
			{ 2, 1 }, // bottom right
			{ 2, 0 }, // top right
			// Right
			{ 4, 1 }, // bottom r
			{ 4, 0 }, // top r
			// Back
			{ 6, 1 }, // bottom right
			{ 6, 0 }, // top right
			// Left
			{ 8, 1 }, // bottom r
			{ 8, 0 }, // top r
		};

		glVertexPointer(3, GL_INT, 0, vertices);
		glEnableClientState(GL_VERTEX_ARRAY);

		glTexCoordPointer(2, GL_INT, 0, texcoords);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glDrawArrays(GL_QUAD_STRIP, 0, ARRAY_SIZE(vertices));
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
	glPopMatrix(); // texture matrix
	glMatrixMode(GL_MODELVIEW);

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
	glPushMatrix();
	glScalef(1, height, 1);
	{
		const Vector3f vertices[] = {
			// Front
			{ -(         left), 0,  (        front) }, // bottom left
			{ -(wider +  left), 1,  (wider + front) }, // top left
			{  (        right), 0,  (        front) }, // bottom right
			{  (wider + right), 1,  (wider + front) }, // top right
			// Right
			{  (        right), 0, -(         back) }, // bottom r
			{  (wider + right), 1, -(wider +  back) }, // top r
			// Back
			{ -(         left), 0, -(         back) }, // bottom right
			{ -(wider +  left), 1, -(wider +  back) }, // top right
			// Left
			{ -(         left), 0,  (        front) }, // bottom r
			{ -(wider +  left), 1,  (wider + front) }, // top r
		};

		glVertexPointer(3, GL_FLOAT, 0, vertices);
		glEnableClientState(GL_VERTEX_ARRAY);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, ARRAY_SIZE(vertices));
		glDisableClientState(GL_VERTEX_ARRAY);
	}
	glPopMatrix();
	glPopAttrib();
}

UBYTE pie_ByteScale(UBYTE a, UBYTE b)
{
	return ((UDWORD)a * (UDWORD)b) >> 8;
}
