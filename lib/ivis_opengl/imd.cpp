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
#include "imd.h"
#include "ivisdef.h"
#include "tex.h"
#include "pietypes.h"

iIMDShape::iIMDShape()
{
	flags = 0;
	nconnectors = 0; // Default number of connectors must be 0
	npoints = 0;
	npolys = 0;
	points = NULL;
	polys = NULL;
	connectors = NULL;
	next = NULL;
	shadowEdgeList = NULL;
	nShadowEdges = 0;
	texpage = iV_TEX_INVALID;
	tcmaskpage = iV_TEX_INVALID;
	normalpage = iV_TEX_INVALID;
	specularpage = iV_TEX_INVALID;
	numFrames = 0;
}

//*************************************************************************
//*** free IMD shape memory
//*
//* pre		shape successfully allocated
//*
//* params	shape = pointer to IMD shape
//*
//******
void iV_IMDRelease(iIMDShape *s)
{
	unsigned int i;
	iIMDShape *d;

	if (s)
	{
		if (s->points)
		{
			free(s->points);
		}
		if (s->connectors)
		{
			free(s->connectors);
		}
		if (s->polys)
		{
			for (i = 0; i < s->npolys; i++)
			{
				if (s->polys[i].texCoord)
				{
					free(s->polys[i].texCoord);
				}
			}
			free(s->polys);
		}
		if (s->shadowEdgeList)
		{
			free(s->shadowEdgeList);
			s->shadowEdgeList = NULL;
		}
		glDeleteBuffers(VBO_COUNT, s->buffers);
		d = s->next;
		delete s;
		iV_IMDRelease(d);
	}
}
