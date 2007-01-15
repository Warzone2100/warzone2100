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
/* bucket3D.h */

#ifndef _bucket3d_h
#define _bucket3d_h

#define		BUCKET

typedef enum _render_type
{
	RENDER_DROID,
	RENDER_STRUCTURE,
	RENDER_FEATURE,
	RENDER_PROXMSG,
	RENDER_PROJECTILE,
	RENDER_PROJECTILE_TRANSPARENT,
	RENDER_SHADOW,
	RENDER_ANIMATION,
	RENDER_EXPLOSION,
	RENDER_EFFECT,
	RENDER_GRAVITON,
	RENDER_SMOKE,
	RENDER_TILE,
	RENDER_WATERTILE,
	RENDER_MIST,
	RENDER_DELIVPOINT,
	RENDER_PARTICLE
} RENDER_TYPE;

typedef struct _tile_bucket
{
	UDWORD	i;
	UDWORD	j;
	SDWORD	depth;
}
TILE_BUCKET;

//function prototypes

/* reset object list */
extern BOOL bucketSetupList(void);

/* add an object to the current render list */
extern BOOL bucketAddTypeToList(RENDER_TYPE objectType, void* object);

/* render Objects in list */
extern BOOL bucketRenderCurrentList(void);
extern SDWORD	worldMax,worldMin;


#endif
