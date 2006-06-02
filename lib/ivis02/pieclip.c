/***************************************************************************/
/*
 * pieClip.c
 *
 * clipping routines for all render modes
 *
 */
/***************************************************************************/

#include "lib/ivis_common/pieclip.h"
#include "lib/ivis_common/ivi.h"

/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

extern iSurface	*psRendSurface;

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/

static int _xtclip_edge2d(iVertex *s1, iVertex *s2, iVertex *clip);
static int _ytclip_edge2d(iVertex *s1, iVertex *s2, iVertex *clip);

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

int pie_PolyClipTex2D(int npoints, iVertex *points, iVertex *clip)
{
	static iVertex xclip[iV_POLY_MAX_POINTS+4];
	iVertex *p0, *p1;
	int n1, n, i;


	p0 = &points[0];
	p1 = &points[1];

	for (i=0, n1=0; i<npoints; i++, p0++, p1++) {

		if (i==(npoints-1))
			p1 = &points[0];

		if ((p0->x == 1<<15) || (p0->y == -1<<15))//check for invalid points jps19aug97
			return 0;

		n1 += _xtclip_edge2d(p0,p1,&xclip[n1]);
	}

	p0 = &xclip[0];
	p1 = &xclip[1];

	for (i=0, n=0; i<n1; p0++, p1++, i++) {
		if (i==(n1-1))
			p1 = &xclip[0];
		n += _ytclip_edge2d(p0,p1,&clip[n]);
	}

	return n;

}

/***************************************************************************/

static int _xtclip_edge2d(iVertex *s1, iVertex *s2, iVertex *clip)
{
	int n, dx;
	int32 t;

	n = 1;

	if (s2->x >= s1->x) {
		if (s1->x < psRendSurface->clip.left) {

			if (s2->x <= psRendSurface->clip.left) return 0;

			dx = s2->x - s1->x;

			if (dx != 0)
				clip->y = s1->y + (s2->y - s1->y) * (psRendSurface->clip.left - s1->x) / dx;
			else
				clip->y = s1->y;

			clip->x = psRendSurface->clip.left;

			// clip uv
			t = ((clip->x - s1->x)<<iV_DIVSHIFT) / dx;

			clip->u = s1->u + ((t * (s2->u - s1->u)) >> iV_DIVSHIFT);
			clip->v = s1->v + ((t * (s2->v - s1->v)) >> iV_DIVSHIFT);
			clip->g = s1->g + ((t * (s2->g - s1->g)) >> iV_DIVSHIFT);
			clip->z = s1->z + ((t * (s2->z - s1->z)) >> iV_DIVSHIFT);


		} else
			*clip = *s1;

		if (s2->x > psRendSurface->clip.right) {

			if (s1->x > psRendSurface->clip.right) return 0;

			clip++;

			dx = s2->x - s1->x;

			if (dx != 0)
				clip->y = s2->y - (s2->y - s1->y) * (s2->x - psRendSurface->clip.right) / dx;
			else
				clip->y = s2->y;

			clip->x = psRendSurface->clip.right;

			// clip uv
			t = ((clip->x - s1->x)<<iV_DIVSHIFT) / dx;
			clip->u = s1->u + ((t * (s2->u - s1->u))>>iV_DIVSHIFT);
			clip->v = s1->v + ((t * (s2->v - s1->v))>>iV_DIVSHIFT);
  			clip->g = s1->g + ((t*(s2->g - s1->g)) >> iV_DIVSHIFT);
  			clip->z = s1->z + ((t*(s2->z - s1->z)) >> iV_DIVSHIFT);

			n = 2;
		}

		return n;

	} else {
		if (s1->x > psRendSurface->clip.right) {

			if (s2->x >= psRendSurface->clip.right) return 0;

			dx = s1->x - s2->x;

			if (dx != 0)
				clip->y = s1->y - (s1->y - s2->y) * (s1->x - psRendSurface->clip.right) / dx;
			else
				clip->y = s1->y;

			clip->x = psRendSurface->clip.right;

			// clip uv
			t = ((clip->x - s1->x)<<iV_DIVSHIFT) / dx;
			clip->u = s1->u + ((t * (s1->u - s2->u))>>iV_DIVSHIFT);
			clip->v = s1->v + ((t * (s1->v - s2->v))>>iV_DIVSHIFT);
			clip->g = s1->g + ((t*(s1->g - s2->g)) >> iV_DIVSHIFT);
			clip->z = s1->z + ((t*(s1->z - s2->z)) >> iV_DIVSHIFT);


		} else
			*clip = *s1;


		if (s2->x < psRendSurface->clip.left) {

			if (s1->x < psRendSurface->clip.left)  return 0;

			clip++;

			dx = s1->x - s2->x;

			if (dx != 0)
				clip->y = s2->y + (s1->y - s2->y) * (psRendSurface->clip.left - s2->x) / dx;
			else
				clip->y = s2->y;

   		clip->x = psRendSurface->clip.left;

			// clip uv
			t = ((clip->x - s1->x)<<iV_DIVSHIFT) / dx;
			clip->u = s1->u + ((t * (s1->u - s2->u))>>iV_DIVSHIFT);
			clip->v = s1->v + ((t * (s1->v - s2->v))>>iV_DIVSHIFT);
  			clip->g = s1->g + ((t*(s1->g - s2->g)) >> iV_DIVSHIFT);
  			clip->z = s1->z + ((t*(s1->z - s2->z)) >> iV_DIVSHIFT);


			n = 2;
   	}
		return n;
	}

}

/***************************************************************************/

static int _ytclip_edge2d(iVertex *s1, iVertex *s2, iVertex *clip)
{
	register int n, dy;
	int32 t;


	n = 1;

	if (s2->y >= s1->y) {

		if (s1->y < psRendSurface->clip.top) {

			if (s2->y <= psRendSurface->clip.top) return 0;

			dy = s2->y - s1->y;

			if (dy != 0)
				clip->x = s1->x + (s2->x - s1->x) * (psRendSurface->clip.top - s1->y) / dy;
			else
				clip->x = s1->x;

			clip->y = psRendSurface->clip.top;

			// clip uv
			t = ((clip->y - s1->y)<<iV_DIVSHIFT) / dy;
			clip->u = s1->u + ((t * (s2->u - s1->u))>>iV_DIVSHIFT);
			clip->v = s1->v + ((t * (s2->v - s1->v))>>iV_DIVSHIFT);
			clip->g = s1->g + ((t*(s2->g - s1->g)) >> iV_DIVSHIFT);
			clip->z = s1->z + ((t*(s2->z - s1->z)) >> iV_DIVSHIFT);


		} else
			*clip = *s1;


		if (s2->y > psRendSurface->clip.bottom) {

			if (s1->y > psRendSurface->clip.bottom) return 0;

			clip++;

			dy = s2->y - s1->y;

			if (dy != 0)
				clip->x = s2->x - (s2->x - s1->x) * (s2->y - psRendSurface->clip.bottom) / dy;
			else
				clip->x = s2->x;

			clip->y = psRendSurface->clip.bottom;

			// clip uv
			t = ((clip->y - s1->y)<<iV_DIVSHIFT) / dy;
			clip->u = s1->u + ((t * (s2->u - s1->u))>>iV_DIVSHIFT);
			clip->v = s1->v + ((t * (s2->v - s1->v))>>iV_DIVSHIFT);
   			clip->g = s1->g + ((t*(s2->g - s1->g)) >> iV_DIVSHIFT);
   			clip->z = s1->z + ((t*(s2->z - s1->z)) >> iV_DIVSHIFT);


			n = 2;
		}

		return n;

	} else {
		if (s1->y > psRendSurface->clip.bottom) {

			if (s2->y >= psRendSurface->clip.bottom) return 0;

			dy = s1->y - s2->y;

			if (dy != 0)
				clip->x = s1->x - (s1->x - s2->x) * (s1->y - psRendSurface->clip.bottom) / dy;
			else
				clip->x = s1->x;

			clip->y = psRendSurface->clip.bottom;

			// clip uv
			t = ((clip->y - s1->y)<<iV_DIVSHIFT) / dy;
			clip->u = s1->u + ((t * (s1->u - s2->u))>>iV_DIVSHIFT);
			clip->v = s1->v + ((t * (s1->v - s2->v))>>iV_DIVSHIFT);
   			clip->g = s1->g + ((t*(s1->g - s2->g)) >> iV_DIVSHIFT);
   			clip->z = s1->z + ((t*(s1->z - s2->z)) >> iV_DIVSHIFT);


		} else
			*clip = *s1;

		if (s2->y < psRendSurface->clip.top) {

			if (s1->y < psRendSurface->clip.top) return 0;

			clip++;

			dy = s1->y - s2->y;

			if (dy != 0)
				clip->x = s2->x + (s1->x - s2->x) * (psRendSurface->clip.top - s2->y) / dy;
			else
				clip->x = s2->x;

 			clip->y = psRendSurface->clip.top;

			// clip uv
			t = ((clip->y - s1->y)<<iV_DIVSHIFT) / dy;
			clip->u = s1->u + ((t * (s1->u - s2->u))>>iV_DIVSHIFT);
			clip->v = s1->v + ((t * (s1->v - s2->v))>>iV_DIVSHIFT);
			clip->g = s1->g + ((t*(s1->g - s2->g)) >> iV_DIVSHIFT);
			clip->z = s1->z + ((t*(s1->z - s2->z)) >> iV_DIVSHIFT);

 			n = 2;

		}

		return n;
	}

}

int	pie_ClipFlat2dLine(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1)
{
	if ((x0 > LONG_TEST) OR (y0 > LONG_TEST) )
	{
		/* bomb out for out of range points */
		return(0);
	}

	if ((x1 > LONG_TEST) OR (y1 > LONG_TEST) )
	{
		/* bomb out for out of range points */
		return(0);
	}

//check x0
	if (x0 < psRendSurface->clip.left)
	{
		if (x1 < psRendSurface->clip.left)
		{
			return(0);
		}
		//relign left edge

		y0 = y0 + ((y1 - y0) * (psRendSurface->clip.left - x0))/(x1 - x0);
		x0 = psRendSurface->clip.left;
	}
	else if (x0 > psRendSurface->clip.right)
	{
		if (x1 > psRendSurface->clip.right)
		{
			return(0);
		}
		//relign right edge
		y0 = y0 - ((y1 - y0) * (psRendSurface->clip.right - x0))/(x1 - x0);
		x0 = psRendSurface->clip.right;
	}
//check y0
	if (y0 < psRendSurface->clip.top)
	{
		if (y1 < psRendSurface->clip.top)
		{
			return(0);
		}
		//relign top

		x0 = x0 + ((x1 - x0) * (psRendSurface->clip.top - y0))/(y1 - y0);
		y0 = psRendSurface->clip.top;
	}
	else if (y0 > psRendSurface->clip.bottom)
	{
		if (y1 > psRendSurface->clip.bottom)
		{
			return(0);
		}
		//relign bottom
		x0 = x0 - ((x1 - x0) * (psRendSurface->clip.bottom - y0))/(y1 - y0);
		y0 = psRendSurface->clip.bottom;
	}
//check x1 (v1 is on screen)
	if (x1 < psRendSurface->clip.left)
	{
		//relign left edge
		y1 = y1 + ((y0 - y1) * (psRendSurface->clip.left - x1))/(x0 - x1);
		x1 = psRendSurface->clip.left;
	}
	else if (x1 > psRendSurface->clip.right)
	{
		//relign right edge
		y1 = y1 - ((y0 - y1) * (psRendSurface->clip.right - x1))/(x0 - x1);
		x1 = psRendSurface->clip.right;
	}
//check y1
	if (y1 < psRendSurface->clip.top)
	{
		//relign top
		x1 = x1 + ((x0 - x1) * (psRendSurface->clip.top - y1))/(y0 - y1);
		y1 = psRendSurface->clip.top;
	}
	else if (y1 > psRendSurface->clip.bottom)
	{
		//relign bottom
		x1 = x1 - ((x0 - x1) * (psRendSurface->clip.bottom - y1))/(y0 - y1);
		y1 = psRendSurface->clip.bottom;
	}
	return 2;
}
