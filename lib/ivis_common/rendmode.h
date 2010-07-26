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
// vid.c 0.1 10-01-96.22-11-96
#ifndef _rendmode_h_
#define _rendmode_h_

#include "ivisdef.h"
#include "pieblitfunc.h"
#include "bitimage.h"
#include "textdraw.h"

//*************************************************************************
//patch

#define iV_Line				pie_Line
#define iV_Box				pie_Box
#define iV_TransBoxFill			pie_TransBoxFill
#define iV_DrawImage			pie_ImageFileID
#define iV_DrawImageRect		pie_ImageFileIDTile

//*************************************************************************
// polygon flags	b0..b7: col, b24..b31: anim index

#define PIE_NO_CULL			0x00002000

//*************************************************************************

#define REND_SURFACE_UNDEFINED		0
#define REND_SURFACE_SCREEN		1
#define REND_SURFACE_USR		2

//*************************************************************************

extern iSurface rendSurface;
extern iSurface *psRendSurface;

//*************************************************************************

extern void iV_RenderAssign(iSurface *s);
extern void iV_SurfaceDestroy(iSurface *s);
extern iSurface *iV_SurfaceCreate(uint32_t flags, int width, int height, int xp, int yp, uint8_t *buffer);

#endif
