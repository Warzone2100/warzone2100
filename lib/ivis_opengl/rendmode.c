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
#include <stdio.h>
#include <stdlib.h>

#include "lib/ivis_common/ivi.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/pieclip.h"
#include "lib/ivis_common/rendfunc.h"
#include "lib/ivis_common/textdraw.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/ivispatch.h"
#include "lib/framework/fractions.h"

//*************************************************************************

iSurface	rendSurface;
iSurface	*psRendSurface;

//*************************************************************************
//***
//*
//*
//******

iSurface *iV_SurfaceCreate(Uint32 flags, int width, int height, int xp, int yp, UBYTE *buffer)
{
	iSurface *s;
	int i;

	assert(buffer!=NULL);	// on playstation this MUST be null

	if ((s = (iSurface *) iV_HeapAlloc(sizeof(iSurface))) == NULL)
		return NULL;

	s->flags = flags;
	s->xcentre = width>>1;
	s->ycentre = height>>1;
	s->xpshift = xp;
	s->ypshift = yp;
	s->width = width;
	s->height = height;
	s->size = width * height;

	s->buffer = buffer;
	for (i=0; i<iV_SCANTABLE_MAX; i++)
		s->scantable[i] = i * width;

	s->clip.left = 0;
	s->clip.right = width-1;
	s->clip.top = 0;
	s->clip.bottom = height-1;

	iV_DEBUG2("vid[SurfaceCreate] = created surface width %d, height %d\n",width,height);

	return s;
}


// user must free s->buffer before calling
void iV_SurfaceDestroy(iSurface *s)
{
	// if renderer assigned to surface
	if (psRendSurface == s)
		psRendSurface = NULL;

	if (s)
		iV_HeapFree(s,sizeof(iSurface));
}

//*************************************************************************
//*** assign renderer
//*
//******

void rend_Assign(iSurface *s)
{
	iV_RenderAssign(s);

	/* Need to look into this - won't the unwanted called still set render surface? */
	psRendSurface = s;
}


// pre VideoOpen
void rend_AssignScreen(void)
{
	iV_RenderAssign(&rendSurface);
}

int iV_GetDisplayWidth(void)
{
	return rendSurface.width;
}

int iV_GetDisplayHeight(void)
{
	return rendSurface.height;
}

//*************************************************************************
//
// function pointers for render assign
//
//*************************************************************************

void (*iV_ppBitmap)(iBitmap *bmp, int x, int y, int w, int h, int ow);
void (*iV_ppBitmapTrans)(iBitmap *bmp, int x, int y, int w, int h, int ow);
void (*iV_SetTransFilter)(UDWORD rgb,UDWORD tablenumber);
void (*iV_UniBitmapDepth)(int texPage, int u, int v, int srcWidth, int srcHeight,
						int x, int y, int destWidth, int destHeight, unsigned char brightness, int depth);
void (*iV_SetTransImds)(BOOL trans);

//*************************************************************************
//
// function pointers for render assign
//
//*************************************************************************

#ifndef PIETOOL
void iV_RenderAssign(iSurface *s)
{
	/* Need to look into this - won't the unwanted called still set render surface? */
	psRendSurface = s;

	iV_SetTransFilter = SetTransFilter;

	iV_DEBUG0("vid[RenderAssign] = assigned renderer :\n");
	iV_DEBUG5("usr %d\nflags %x\nxcentre, ycentre %d\nbuffer %p\n",
			s->usr,s->flags,s->xcentre,s->ycentre,s->buffer);
}
#endif	// don't want this function at all if we have PIETOOL defined
