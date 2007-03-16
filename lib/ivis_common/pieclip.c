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
#include "pieclip.h"
#include "ivi.h"

static UDWORD videoBufferDepth = 32, videoBufferWidth = 640, videoBufferHeight = 480;
extern iSurface	*psRendSurface;

BOOL pie_SetVideoBufferDepth(UDWORD depth)
{
	videoBufferDepth = depth;
	return(TRUE);
}

BOOL pie_SetVideoBufferWidth(UDWORD width)
{
	videoBufferWidth = width;
	return(TRUE);
}

BOOL pie_SetVideoBufferHeight(UDWORD height)
{
	videoBufferHeight = height;
	return(TRUE);
}

UDWORD pie_GetVideoBufferDepth(void)
{
	return(videoBufferDepth);
}

UDWORD pie_GetVideoBufferWidth(void)
{
	return(videoBufferWidth);
}

UDWORD pie_GetVideoBufferHeight(void)
{
	return(videoBufferHeight);
}

void pie_Set2DClip(int x0, int y0, int x1, int y1)
{
	psRendSurface->clip.left = x0;
	psRendSurface->clip.top = y0;
	psRendSurface->clip.right = x1;
	psRendSurface->clip.bottom = y1;
}

static void pie_ClipUV(PIEVERTEX *s1, PIEVERTEX *s2, PIEVERTEX *clip, Sint32 t)
{
	clip->tu = s1->tu + ((t * (s2->tu - s1->tu)) >> iV_DIVSHIFT);
	clip->tv = s1->tv + ((t * (s2->tv - s1->tv)) >> iV_DIVSHIFT);
	clip->sz = s1->sz + ((t * (s2->sz - s1->sz)) >> iV_DIVSHIFT);
	clip->light.byte.r = s1->light.byte.r + ((t * (s2->light.byte.r - s1->light.byte.r)) >> iV_DIVSHIFT);
	clip->light.byte.g = s1->light.byte.g + ((t * (s2->light.byte.g - s1->light.byte.g)) >> iV_DIVSHIFT);
	clip->light.byte.b = s1->light.byte.b + ((t * (s2->light.byte.b - s1->light.byte.b)) >> iV_DIVSHIFT);
	clip->light.byte.a = s1->light.byte.a + ((t * (s2->light.byte.a - s1->light.byte.a)) >> iV_DIVSHIFT);
}

static int pie_ClipXT(PIEVERTEX *s1, PIEVERTEX *s2, PIEVERTEX *clip)
{
	int n = 1, dx;
	Sint32 t;

	if (s2->sx >= s1->sx) {
		if (s1->sx < psRendSurface->clip.left) {
			if (s2->sx <= psRendSurface->clip.left)
				return 0;

			dx = s2->sx - s1->sx;

			if (dx != 0)
				clip->sy = s1->sy + (s2->sy - s1->sy) * (psRendSurface->clip.left - s1->sx) / dx;
			else
				clip->sy = s1->sy;

			clip->sx = psRendSurface->clip.left;

			// clip uv
			t = ((clip->sx - s1->sx)<<iV_DIVSHIFT) / dx;
			pie_ClipUV(s1, s2, clip, t);
		} else
			*clip = *s1;

		if (s2->sx > psRendSurface->clip.right) {
			if (s1->sx > psRendSurface->clip.right)
				return 0;

			clip++;
			dx = s2->sx - s1->sx;

			if (dx != 0)
				clip->sy = s2->sy - (s2->sy - s1->sy) * (s2->sx - psRendSurface->clip.right) / dx;
			else
				clip->sy = s2->sy;

			clip->sx = psRendSurface->clip.right;

			// clip uv
			t = ((clip->sx - s1->sx)<<iV_DIVSHIFT) / dx;
			pie_ClipUV(s1, s2, clip, t);

			n = 2;
		}
		return n;

	} else {
		if (s1->sx > psRendSurface->clip.right) {

			if (s2->sx >= psRendSurface->clip.right) return 0;

			dx = s1->sx - s2->sx;

			if (dx != 0)
				clip->sy = s1->sy - (s1->sy - s2->sy) * (s1->sx - psRendSurface->clip.right) / dx;
			else
				clip->sy = s1->sy;

			clip->sx = psRendSurface->clip.right;

			// clip uv
			t = ((clip->sx - s1->sx)<<iV_DIVSHIFT) / dx;
			pie_ClipUV(s1, s2, clip, t);
		} else {
			*clip = *s1;
		}

		if (s2->sx < psRendSurface->clip.left) {
			if (s1->sx < psRendSurface->clip.left)
				return 0;

			clip++;
			dx = s1->sx - s2->sx;

			if (dx != 0)
				clip->sy = s2->sy + (s1->sy - s2->sy) * (psRendSurface->clip.left - s2->sx) / dx;
			else
				clip->sy = s2->sy;

			clip->sx = psRendSurface->clip.left;

			// clip uv
			t = ((clip->sx - s1->sx)<<iV_DIVSHIFT) / dx;
			pie_ClipUV(s1, s2, clip, t);

			n = 2;
		}
		return n;
	}
}

static int pie_ClipYT(PIEVERTEX *s1, PIEVERTEX *s2, PIEVERTEX *clip)
{
	int n = 1, dy;
	Sint32 t;

	if (s2->sy >= s1->sy) {

		if (s1->sy < psRendSurface->clip.top) {

			if (s2->sy <= psRendSurface->clip.top) return 0;

			dy = s2->sy - s1->sy;

			if (dy != 0)
				clip->sx = s1->sx + (s2->sx - s1->sx) * (psRendSurface->clip.top - s1->sy) / dy;
			else
				clip->sx = s1->sx;

			clip->sy = psRendSurface->clip.top;

			// clip uv
			t = ((clip->sy - s1->sy)<<iV_DIVSHIFT) / dy;
			pie_ClipUV(s1, s2, clip, t);
		} else {
			*clip = *s1;
		}

		if (s2->sy > psRendSurface->clip.bottom)
		{
			if (s1->sy > psRendSurface->clip.bottom)
			{
				return 0;
			}
			clip++;

			dy = s2->sy - s1->sy;

			if (dy != 0)
			{
				clip->sx = s2->sx - (s2->sx - s1->sx) * (s2->sy - psRendSurface->clip.bottom) / dy;
			} else {
				clip->sx = s2->sx;
			}

			clip->sy = psRendSurface->clip.bottom;

			t = ((clip->sy - s1->sy)<<iV_DIVSHIFT) / dy;
			pie_ClipUV(s1, s2, clip, t);

			n = 2;
		}

		return n;

	} else {
		if (s1->sy > psRendSurface->clip.bottom)
		{
			if (s2->sy >= psRendSurface->clip.bottom) return 0;

			dy = s1->sy - s2->sy;

			if (dy != 0)
				clip->sx = s1->sx - (s1->sx - s2->sx) * (s1->sy - psRendSurface->clip.bottom) / dy;
			else
				clip->sx = s1->sx;

			clip->sy = psRendSurface->clip.bottom;

			// clip uv
			t = ((clip->sy - s1->sy)<<iV_DIVSHIFT) / dy;
			pie_ClipUV(s1, s2, clip, t);

		} else {
			*clip = *s1;
		}

		if (s2->sy < psRendSurface->clip.top)
		{
			if (s1->sy < psRendSurface->clip.top)
			{
				return 0;
			}

			clip++;

			dy = s1->sy - s2->sy;

			if (dy != 0)
			{
				clip->sx = s2->sx + (s1->sx - s2->sx) * (psRendSurface->clip.top - s2->sy) / dy;
			} else {
				clip->sx = s2->sx;
			}

			clip->sy = psRendSurface->clip.top;

			t = ((clip->sy - s1->sy)<<iV_DIVSHIFT) / dy;
			pie_ClipUV(s1, s2, clip, t);

			n = 2;
		}

		return n;
	}
}

int pie_ClipTextured(int npoints, PIEVERTEX *points, PIEVERTEX *clip)
{
	static PIEVERTEX xclip[iV_POLY_MAX_POINTS+4];
	PIEVERTEX *p0, *p1;
	int n1, n, i;

	p0 = &points[0];
	p1 = &points[1];

	for (i=0, n1=0; i<npoints; i++, p0++, p1++) {

		if (i==(npoints-1))
			p1 = &points[0];

		if ((p0->sx == 1<<15) || (p0->sy == -1<<15))//check for invalid points jps19aug97
			return 0;

		n1 += pie_ClipXT(p0,p1,&xclip[n1]);
	}

	p0 = &xclip[0];
	p1 = &xclip[1];

	for (i=0, n=0; i<n1; p0++, p1++, i++) {
		if (i==(n1-1))
			p1 = &xclip[0];
		n += pie_ClipYT(p0,p1,&clip[n]);
	}

	return n;
}

#ifdef NEVER_USED_ANYWHERE
// I leave this around just for potential inspirational purposes. - Per

//*************************************************************************
/* Alex - much faster tri clipper - won't clip owt else tho' */
int	pie_ClipTexturedTriangleFast(PIEVERTEX *v1, PIEVERTEX *v2, PIEVERTEX *v3, PIEVERTEX *clipped)
{
	static	PIEVERTEX	xClip[iV_POLY_MAX_POINTS+4];	// plus 4 hopefully is limit?
	static	PIEVERTEX	*p0,*p1;
	UDWORD	numPreY,numAll;
	UDWORD	i;

	numPreY = 0;
	if( (v1->sx > LONG_TEST) || (v1->sy > LONG_TEST) )
	{
		/* bomb out for out of range points */
		return(0);
	}
	numPreY += pie_ClipXT(v1,v2,&xClip[numPreY]);

	if( (v2->sx > LONG_TEST) || (v2->sy > LONG_TEST) )
	{
		/* bomb out for out of range points */
		return(0);
	}
	numPreY += pie_ClipXT(v2,v3,&xClip[numPreY]);

	if( (v3->sx > LONG_TEST) || (v3->sy > LONG_TEST) )
	{
		/* bomb out for out of range points */
		return(0);
	}
	numPreY += pie_ClipXT(v3,v1,&xClip[numPreY]);

	/* We've now clipped against x axis - now for Y */

	p0 = &xClip[0];
	p1 = &xClip[1];

	for (i=0, numAll=0; i<numPreY; p0++, p1++, i++) {
		if (i==(numPreY-1))
			p1 = &xClip[0];
		numAll += pie_ClipYT(p0,p1,&clipped[numAll]);
	}

	return numAll;
}
#endif
