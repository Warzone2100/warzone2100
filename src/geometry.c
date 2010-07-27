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
/* Geometry.c - holds trig/vector deliverance specific stuff for 3D */
/* Alex McLean, Pumpkin Studios, EIDOS Interactive */

#include "lib/framework/frame.h"

#include "lib/ivis_common/ivisdef.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/gamelib/gtime.h"

#include "geometry.h"
#include "objectdef.h"
#include "map.h"
#include "display3d.h"
#include "hci.h"
#include "display.h"

uint16_t calcDirection(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
	return iAtan2(x1 - x0, y1 - y0);
}


/*	A useful function and one that should have been written long ago, assuming of course
	that is hasn't been!!!! Alex M, 24th Sept, 1998. Returns the nearest unit
	to a given world coordinate - we can choose whether we require that the unit be
	selected or not... Makes sending the most logical unit to do something very easy.

  NB*****THIS WON'T PICK A VTOL DROID*****
*/

DROID	*getNearestDroid(UDWORD x, UDWORD y, BOOL bSelected)
{
DROID	*psDroid,*psBestUnit;
UDWORD	bestSoFar;

	/* Go thru' all the droids  - how often have we seen this - a MACRO maybe? */
	for(psDroid = apsDroidLists[selectedPlayer],psBestUnit = NULL, bestSoFar = UDWORD_MAX;
		psDroid; psDroid = psDroid->psNext)
	{
        if (!isVtolDroid(psDroid))
        {
		    /* Clever (?) bit that reads whether we're interested in droids being selected or not */
			if (!bSelected || psDroid->selected)
		    {
				uint32_t dist = iHypot(psDroid->pos.x - x, psDroid->pos.y - y);
			    /* Is this the nearest one we got so far? */
			    if(dist<bestSoFar)
			    {
				    /* Yes, then keep a record of the distance for comparison... */
				    bestSoFar = dist;
				    /* ..and store away the droid responsible */
				    psBestUnit = psDroid;
			    }
            }
		}
	}
	return(psBestUnit);
}
// -------------------------------------------------------------------------------------------

/* Returns non-zero if a point is in a 4 sided polygon */
/* See header file for definition of QUAD */
int inQuad(const Vector2i *pt, const QUAD *quad)
{
	int i, j, c = 0;

	for (i = 0, j = 3; i < 4; j = i++)
	{
		if (((quad->coords[i].y <= pt->y && pt->y < quad->coords[j].y) ||
			(quad->coords[j].y <= pt->y && pt->y < quad->coords[i].y)) &&
			(pt->x <
				(quad->coords[j].x - quad->coords[i].x)
				* (pt->y - quad->coords[i].y)
				/ (quad->coords[j].y - quad->coords[i].y)
				+ quad->coords[i].x))
		{
			c = !c;
		}
	}

	return c;
}

//-----------------------------------------------------------------------------------
BOOL	droidOnScreen( DROID *psDroid, SDWORD tolerance )
{
SDWORD	dX,dY;

	if (DrawnInLastFrame(psDroid->sDisplay.frameNumber)==true)
		{
			dX = psDroid->sDisplay.screenX;
			dY = psDroid->sDisplay.screenY;
			/* Is it on screen */
			if(dX > (0-tolerance) && dY > (0-tolerance)
				&& dX < (SDWORD)(pie_GetVideoBufferWidth()+tolerance)
				&& dY < (SDWORD)(pie_GetVideoBufferHeight()+tolerance))
			{
				return(true);
			}
		}
	return(false);
}
