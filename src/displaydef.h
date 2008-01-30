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
/** \file
 *  Display structures.
 */

#ifndef __INCLUDED_DISPLAYDEF_H__
#define __INCLUDED_DISPLAYDEF_H__

#include "lib/ivis_common/imd.h"
		// ffs am
#include "lib/ivis_common/pieclip.h"

#define	BOUNDARY_X			(16)
#define BOUNDARY_Y			(16)
//#define BOUNDARY_X		(DISP_WIDTH/20)	   // proportional to resolution - Alex M
//#define	BOUNDARY_Y		(DISP_WIDTH/16)
//#define	BOUNDARY_X		(24)
//#define	BOUNDARY_Y		(24)

typedef struct _screen_disp_data
{
	//UDWORD		dummy;
	iIMDShape	*imd;
//	BOOL		drawnThisFrame;		// for sorting - have we drawn the imd already?
	UDWORD		frameNumber;		// last frame it was drawn
//	UDWORD		animFrame;			// anim Frame
//	SDWORD		bucketDepth;
//	BOOL		onScreen;
//	UDWORD		numTiles;
	UDWORD		screenX,screenY;
	UDWORD		screenR;
} SCREEN_DISP_DATA;

#endif // __INCLUDED_DISPLAYDEF_H__
