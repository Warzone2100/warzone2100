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
/***************************************************************************/
/*
 * pieclip.h
 *
 * clipping for all pumpkin image library functions.
 *
 */
/***************************************************************************/

#ifndef _pieclip_h
#define _pieclip_h

/***************************************************************************/

#include "lib/framework/frame.h"
#include "piedef.h"


/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

typedef struct
{
	Vector3i pos;
	unsigned int u, v;
	PIELIGHT light;
} CLIP_VERTEX;

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/

extern BOOL pie_SetVideoBufferDepth(UDWORD depth);
extern BOOL pie_SetVideoBufferWidth(UDWORD width);
extern BOOL pie_SetVideoBufferHeight(UDWORD height);
extern UDWORD pie_GetVideoBufferDepth( void ) WZ_DECL_PURE;
extern UDWORD pie_GetVideoBufferWidth( void ) WZ_DECL_PURE;
extern UDWORD pie_GetVideoBufferHeight( void ) WZ_DECL_PURE;

#endif // _pieclip_h
