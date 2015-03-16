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

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/rendmode.h"

iSurface *iV_SurfaceCreate(int width, int height)
{
	iSurface *s = (iSurface *)malloc(sizeof(iSurface));

	if (!s)
	{
		debug(LOG_ERROR, "of memory");
		return NULL;
	}

	s->xcentre = width / 2;
	s->ycentre = height / 2;
	s->width = width;
	s->height = height;
	s->clip.left = 0;
	s->clip.right = width - 1;
	s->clip.top = 0;
	s->clip.bottom = height - 1;

	return s;
}

void iV_SurfaceDestroy(iSurface *s)
{
	free(s);
}
