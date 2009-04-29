/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/pieclip.h"
#include "lib/ivis_common/textdraw.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/ivispatch.h"

//*************************************************************************

iSurface	rendSurface;
iSurface	*psRendSurface;

//*************************************************************************
//***
//*
//*
//******

iSurface *iV_SurfaceCreate(uint32_t flags, int width, int height, int xp, int yp, UBYTE *buffer)
{
	iSurface *s = malloc(sizeof(iSurface));

	assert(buffer!=NULL);	// on playstation this MUST be null

	if (!s)
	{
		debug(LOG_ERROR, "iV_SurfaceCreate: out of memory");
		return NULL;
	}

	s->flags = flags;
	s->xcentre = width>>1;
	s->ycentre = height>>1;
	s->xpshift = xp;
	s->ypshift = yp;
	s->width = width;
	s->height = height;
	s->size = width * height;
	s->buffer = buffer;
	s->clip.left = 0;
	s->clip.right = width-1;
	s->clip.top = 0;
	s->clip.bottom = height-1;

	return s;
}


// user must free s->buffer before calling
void iV_SurfaceDestroy(iSurface *s)
{
	// if renderer assigned to surface
	if (psRendSurface == s)
	{
		psRendSurface = NULL;
	}

	if (s)
	{
		free(s);
	}
}


//*************************************************************************
//
// function pointers for render assign
//
//*************************************************************************

void iV_RenderAssign(iSurface *s)
{
	/* Need to look into this - won't the unwanted called still set render surface? */
	psRendSurface = s;

	debug(LOG_3D, "iV_RenderAssign: flags %x; xcentre %d; ycentre %d; buffer %p",
			s->flags, s->xcentre, s->ycentre, s->buffer);
}
