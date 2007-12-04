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

static void pie_ClipUV(CLIP_VERTEX *s1, CLIP_VERTEX *s2, CLIP_VERTEX *clip, Sint32 t)
{
	clip->u = s1->u + ((t * (s2->u - s1->u)) >> iV_DIVSHIFT);
	clip->v = s1->v + ((t * (s2->v - s1->v)) >> iV_DIVSHIFT);
	clip->pos.z = s1->pos.z + ((t * (s2->pos.z - s1->pos.z)) >> iV_DIVSHIFT);

	clip->light.byte.r = s1->light.byte.r + ((t * (s2->light.byte.r - s1->light.byte.r)) >> iV_DIVSHIFT);
	clip->light.byte.g = s1->light.byte.g + ((t * (s2->light.byte.g - s1->light.byte.g)) >> iV_DIVSHIFT);
	clip->light.byte.b = s1->light.byte.b + ((t * (s2->light.byte.b - s1->light.byte.b)) >> iV_DIVSHIFT);
	clip->light.byte.a = s1->light.byte.a + ((t * (s2->light.byte.a - s1->light.byte.a)) >> iV_DIVSHIFT);
}

static int pie_ClipXT(CLIP_VERTEX *s1, CLIP_VERTEX *s2, CLIP_VERTEX *clip)
{
	int n = 1, dx;
	Sint32 t;

	if (s2->pos.x >= s1->pos.x)
	{
		if (s1->pos.x < psRendSurface->clip.left)
		{
			if (s2->pos.x <= psRendSurface->clip.left)
			{
				return 0;
			}

			dx = s2->pos.x - s1->pos.x;

			if (dx != 0)
			{
				clip->pos.y = s1->pos.y + (s2->pos.y - s1->pos.y) * (psRendSurface->clip.left - s1->pos.x) / dx;
			}
			else
			{
				clip->pos.y = s1->pos.y;
			}

			clip->pos.x = psRendSurface->clip.left;

			// clip uv
			t = ((clip->pos.x - s1->pos.x) << iV_DIVSHIFT) / dx;
			pie_ClipUV(s1, s2, clip, t);
		}
		else
		{
			*clip = *s1;
		}

		if (s2->pos.x > psRendSurface->clip.right)
		{
			if (s1->pos.x > psRendSurface->clip.right)
			{
				return 0;
			}

			clip++;
			dx = s2->pos.x - s1->pos.x;

			if (dx != 0)
			{
				clip->pos.y = s2->pos.y - (s2->pos.y - s1->pos.y) * (s2->pos.x - psRendSurface->clip.right) / dx;
			}
			else
			{
				clip->pos.y = s2->pos.y;
			}

			clip->pos.x = psRendSurface->clip.right;

			// clip uv
			t = ((clip->pos.x - s1->pos.x) << iV_DIVSHIFT) / dx;
			pie_ClipUV(s1, s2, clip, t);

			n = 2;
		}

		return n;
	}
	else
	{
		if (s1->pos.x > psRendSurface->clip.right)
		{
			if (s2->pos.x >= psRendSurface->clip.right)
			{
				return 0;
			}

			dx = s1->pos.x - s2->pos.x;

			if (dx != 0)
			{
				clip->pos.y = s1->pos.y - (s1->pos.y - s2->pos.y) * (s1->pos.x - psRendSurface->clip.right) / dx;
			}
			else
			{
				clip->pos.y = s1->pos.y;
			}

			clip->pos.x = psRendSurface->clip.right;

			// clip uv
			t = ((clip->pos.x - s1->pos.x) << iV_DIVSHIFT) / dx;
			pie_ClipUV(s1, s2, clip, t);
		}
		else
		{
			*clip = *s1;
		}

		if (s2->pos.x < psRendSurface->clip.left)
		{
			if (s1->pos.x < psRendSurface->clip.left)
			{
				return 0;
			}

			clip++;
			dx = s1->pos.x - s2->pos.x;

			if (dx != 0)
			{
				clip->pos.y = s2->pos.y + (s1->pos.y - s2->pos.y) * (psRendSurface->clip.left - s2->pos.x) / dx;
			}
			else
			{
				clip->pos.y = s2->pos.y;
			}

			clip->pos.x = psRendSurface->clip.left;

			// clip uv
			t = ((clip->pos.x - s1->pos.x)<<iV_DIVSHIFT) / dx;
			pie_ClipUV(s1, s2, clip, t);

			n = 2;
		}

		return n;
	}
}

static int pie_ClipYT(CLIP_VERTEX *s1, CLIP_VERTEX *s2, CLIP_VERTEX *clip)
{
	int n = 1, dy;
	Sint32 t;

	if (s2->pos.y >= s1->pos.y)
	{
		if (s1->pos.y < psRendSurface->clip.top)
		{
			if (s2->pos.y <= psRendSurface->clip.top)
			{
				return 0;
			}

			dy = s2->pos.y - s1->pos.y;

			if (dy != 0)
			{
				clip->pos.x = s1->pos.x + (s2->pos.x - s1->pos.x) * (psRendSurface->clip.top - s1->pos.y) / dy;
			}
			else
			{
				clip->pos.x = s1->pos.x;
			}

			clip->pos.y = psRendSurface->clip.top;

			// clip uv
			t = ((clip->pos.y - s1->pos.y) << iV_DIVSHIFT) / dy;
			pie_ClipUV(s1, s2, clip, t);
		}
		else
		{
			*clip = *s1;
		}

		if (s2->pos.y > psRendSurface->clip.bottom)
		{
			if (s1->pos.y > psRendSurface->clip.bottom)
			{
				return 0;
			}

			clip++;

			dy = s2->pos.y - s1->pos.y;

			if (dy != 0)
			{
				clip->pos.x = s2->pos.x - (s2->pos.x - s1->pos.x) * (s2->pos.y - psRendSurface->clip.bottom) / dy;
			}
			else
			{
				clip->pos.x = s2->pos.x;
			}

			clip->pos.y = psRendSurface->clip.bottom;

			t = ((clip->pos.y - s1->pos.y) << iV_DIVSHIFT) / dy;
			pie_ClipUV(s1, s2, clip, t);

			n = 2;
		}

		return n;

	}
	else
	{
		if (s1->pos.y > psRendSurface->clip.bottom)
		{
			if (s2->pos.y >= psRendSurface->clip.bottom)
			{
				return 0;
			}

			dy = s1->pos.y - s2->pos.y;

			if (dy != 0)
			{
				clip->pos.x = s1->pos.x - (s1->pos.x - s2->pos.x) * (s1->pos.y - psRendSurface->clip.bottom) / dy;
			}
			else
			{
				clip->pos.x = s1->pos.x;
			}

			clip->pos.y = psRendSurface->clip.bottom;

			// clip uv
			t = ((clip->pos.y - s1->pos.y) << iV_DIVSHIFT) / dy;
			pie_ClipUV(s1, s2, clip, t);

		}
		else
		{
			*clip = *s1;
		}

		if (s2->pos.y < psRendSurface->clip.top)
		{
			if (s1->pos.y < psRendSurface->clip.top)
			{
				return 0;
			}

			clip++;

			dy = s1->pos.y - s2->pos.y;

			if (dy != 0)
			{
				clip->pos.x = s2->pos.x + (s1->pos.x - s2->pos.x) * (psRendSurface->clip.top - s2->pos.y) / dy;
			}
			else
			{
				clip->pos.x = s2->pos.x;
			}

			clip->pos.y = psRendSurface->clip.top;

			t = ((clip->pos.y - s1->pos.y) << iV_DIVSHIFT) / dy;
			pie_ClipUV(s1, s2, clip, t);

			n = 2;
		}

		return n;
	}
}

int pie_ClipTextured(int npoints, CLIP_VERTEX *points, CLIP_VERTEX *clip)
{
	static CLIP_VERTEX xclip[iV_POLY_MAX_POINTS+4];
	CLIP_VERTEX *p0, *p1;
	int n1, n, i;

	p0 = &points[0];
	p1 = &points[1];

	for (i = 0, n1 = 0; i < npoints; i++, p0++, p1++)
	{
		if (i == (npoints-1))
		{
			p1 = &points[0];
		}

		// FIXME MAGIC
		if ((p0->pos.x == 1<<15) || (p0->pos.y == -1<<15))//check for invalid points jps19aug97
		{
			return 0;
		}

		n1 += pie_ClipXT(p0, p1, &xclip[n1]);
	}

	p0 = &xclip[0];
	p1 = &xclip[1];

	for (i = 0, n = 0; i < n1; p0++, p1++, i++)
	{
		if (i == (n1-1))
		{
			p1 = &xclip[0];
		}
		n += pie_ClipYT(p0, p1, &clip[n]);
	}

	return n;
}
