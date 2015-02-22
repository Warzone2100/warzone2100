/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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

#include "lib/ivis_opengl/ivisdef.h"
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

DROID	*getNearestDroid(UDWORD x, UDWORD y, bool bSelected)
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
bool inQuad(const Vector2i *pt, const QUAD *quad)
{
	// Early out.
	int minX = std::min(std::min(quad->coords[0].x, quad->coords[1].x), std::min(quad->coords[2].x, quad->coords[3].x)); if (pt->x < minX) return false;
	int maxX = std::max(std::max(quad->coords[0].x, quad->coords[1].x), std::max(quad->coords[2].x, quad->coords[3].x)); if (pt->x > maxX) return false;
	int minY = std::min(std::min(quad->coords[0].y, quad->coords[1].y), std::min(quad->coords[2].y, quad->coords[3].y)); if (pt->y < minY) return false;
	int maxY = std::max(std::max(quad->coords[0].y, quad->coords[1].y), std::max(quad->coords[2].y, quad->coords[3].y)); if (pt->y > maxY) return false;

	bool c = false;

	for (int i = 0, j = 3; i < 4; j = i++)
	{
		Vector2i edge = quad->coords[j] - quad->coords[i];
		Vector2i pos = *pt - quad->coords[i];
		if ((     0 <= pos.y && pos.y < edge.y && pos.x * edge.y < pos.y * edge.x) ||
		    (edge.y <= pos.y && pos.y < 0      && pos.x * edge.y > pos.y * edge.x))
		{
			c = !c;
		}
	}

	return c;
}

Vector2i positionInQuad(Vector2i const &pt, QUAD const &quad)
{
	long lenSq[4];
	long ptDot[4];
	for (int i = 0, j = 3; i < 4; j = i++)
	{
		Vector2i edge = quad.coords[j] - quad.coords[i];
		Vector2i pos  = quad.coords[j] - pt;
		Vector2i posRot(pos.y, -pos.x);
		lenSq[i] = edge*edge;
		ptDot[i] = posRot*edge;
	}
	int ret[2];
	for (int i = 0; i < 2; ++i)
	{
		long d1 = ptDot[i]*lenSq[i + 2];
		long d2 = ptDot[i + 2]*lenSq[i];
		ret[i] = d1 + d2 != 0? (int64_t)TILE_UNITS*d1 / (d1 + d2) : TILE_UNITS/2;
	}
	return Vector2i(ret[0], ret[1]);
}

//-----------------------------------------------------------------------------------
bool	droidOnScreen( DROID *psDroid, SDWORD tolerance )
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
