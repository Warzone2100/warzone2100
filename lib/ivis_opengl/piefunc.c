/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
/***************************************************************************/
/*
 * piefunc.c
 *
 * extended render routines for 3D rendering
 *
 */
/***************************************************************************/

#include <string.h>
#ifdef _MSC_VER
#include <windows.h>  //needed for gl.h!  --Qamly
#endif
#include <SDL/SDL_opengl.h>

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/piefunc.h"
#include "lib/ivis_common/piestate.h"
#include "piematrix.h"
#include "pietexture.h"
#include "lib/ivis_common/pieclip.h"

/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/
static PIEVERTEX	pieVrts[pie_MAX_POLY_VERTS];
static PIEVERTEX	clippedVrts[pie_MAX_POLY_VERTS];
static UBYTE		aByteScale[256][256];

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

void pie_DownLoadBufferToScreen(void *pSrcData, UDWORD destX, UDWORD destY,UDWORD srcWidth,UDWORD srcHeight,UDWORD srcStride)
{
	/* Originally used to show video from seqdisp.c */
	return;
}

/***************************************************************************/
/*
 *	void pie_RectFilter(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1, UDWORD colour)
 *
 * Draws rectangular filter to screen ivis mode defaults to
 *
 */
/***************************************************************************/
void pie_RectFilter(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1, UDWORD colour)
{
	iV_TransBoxFill(x0, y0, x1, y1);
}

/* ---------------------------------------------------------------------------------- */

void	pie_DrawViewingWindow(iVector *v, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, UDWORD colour)
{
	SDWORD clip, i;

	pie_SetTexturePage(-1);
	pie_SetRendMode(REND_ALPHA_FLAT);
//PIE verts
	pieVrts[0].sx = v[1].x;
	pieVrts[0].sy = v[1].y;
	//cull triangles with off screen points
	pieVrts[0].sz  = (SDWORD)INTERFACE_DEPTH;


	pieVrts[0].tu = (UWORD)0.0;
	pieVrts[0].tv = (UWORD)0.0;
	pieVrts[0].light.argb = colour;//0x7fffffff;
	pieVrts[0].specular.argb = 0;

	memcpy(&pieVrts[1],&pieVrts[0],sizeof(PIEVERTEX));
	memcpy(&pieVrts[2],&pieVrts[0],sizeof(PIEVERTEX));
	memcpy(&pieVrts[3],&pieVrts[0],sizeof(PIEVERTEX));
	memcpy(&pieVrts[4],&pieVrts[0],sizeof(PIEVERTEX));

	pieVrts[1].sx = v[0].x;
	pieVrts[1].sy = v[0].y;

	pieVrts[2].sx = v[2].x;
	pieVrts[2].sy = v[2].y;

	pieVrts[3].sx = v[3].x;
	pieVrts[3].sy = v[3].y;

	pie_Set2DClip(x1,y1,x2-1,y2-1);
	clip = pie_ClipTextured(4, &pieVrts[0], &clippedVrts[0]);
	pie_Set2DClip(CLIP_BORDER,CLIP_BORDER,psRendSurface->width-CLIP_BORDER,psRendSurface->height-CLIP_BORDER);

	if (clip >= 3) {
		PIELIGHT c;

		c.argb = colour;
		glBegin(GL_TRIANGLE_FAN);
		glColor4ub(c.byte.r, c.byte.g, c.byte.b, c.byte.a >> 1);
		for (i = 0; i < clip; i++) {
			glVertex2f(clippedVrts[i].sx, clippedVrts[i].sy);
		}
		glEnd();
		glBegin(GL_LINE_STRIP);
		glColor4ub(c.byte.r, c.byte.g, c.byte.b, c.byte.a);
		for (i = 0; i < clip; i++) {
			glVertex2f(clippedVrts[i].sx, clippedVrts[i].sy);
		}
		glVertex2f(clippedVrts[0].sx, clippedVrts[0].sy);
		glEnd();
	}
}

/* ---------------------------------------------------------------------------------- */
void pie_TransColouredTriangle(PIEVERTEX *vrt, UDWORD rgb, UDWORD trans)
{
        PIELIGHT c;
	UDWORD i;

	c.argb = rgb;

	pie_SetTexturePage(-1);
	pie_SetRendMode(REND_ALPHA_ITERATED);

        glBegin(GL_TRIANGLE_FAN);
        glColor4ub(c.byte.r, c.byte.g, c.byte.b, 128);
        for (i = 0; i < 3; ++i)
        {
		glVertex3f(vrt[i].sx, vrt[i].sy, vrt[i].sz);
	}
        glEnd();}


/* ---------------------------------------------------------------------------------- */

void pie_InitMaths(void)
{
	UBYTE c;
	UDWORD a,b,bigC;

	for(a=0; a<=UBYTE_MAX; a++)
	{
		for(b=0; b<=UBYTE_MAX; b++)
		{
			bigC = a * b;
			bigC /= UBYTE_MAX;
			ASSERT( bigC <= UBYTE_MAX,"light_InitMaths; rounding error" );
			c = (UBYTE)bigC;
			aByteScale[a][b] = c;
		}
	}
}

UBYTE pie_ByteScale(UBYTE a, UBYTE b)
{
	return (((UDWORD)a)*((UDWORD)b))>>8;
}
