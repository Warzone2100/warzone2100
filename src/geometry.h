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

#ifndef __INCLUDED_SRC_GEOMETRY_H__
#define __INCLUDED_SRC_GEOMETRY_H__

#include "map.h"
#include "hci.h"

typedef struct _t_quad
{
	Vector2i coords[4];
} QUAD;

extern UDWORD adjustDirection( SDWORD present, SDWORD difference );
extern SDWORD calcDirection( UDWORD x0, UDWORD y0, UDWORD x1, UDWORD y1 );
extern void initBulletTable( void );
extern int inQuad( const Vector2i *pt, const QUAD *quad );
extern DROID *getNearestDroid( UDWORD x, UDWORD y, BOOL bSelected );
extern BOOL droidOnScreen( DROID *psDroid, SDWORD tolerance );

extern UDWORD dirtySqrt( SDWORD x1, SDWORD y1, SDWORD x2, SDWORD y2 ) WZ_DECL_CONST;

static inline STRUCTURE *getTileStructure(UDWORD x, UDWORD y)
{
	return (STRUCTURE *)mapTile(x,y)->psObject;
}

static inline FEATURE *getTileFeature(UDWORD x, UDWORD y)
{
	return (FEATURE *)mapTile(x,y)->psObject;
}

static inline BASE_OBJECT *getTileOccupier(UDWORD x, UDWORD y)
{
	MAPTILE *psTile = mapTile(x,y);

	if (TEST_TILE_VISIBLE(selectedPlayer, psTile))
	{
		return mapTile(x,y)->psObject;
	} else {
		return NULL;
	}
}

#endif // __INCLUDED_SRC_GEOMETRY_H__
