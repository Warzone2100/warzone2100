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
#ifndef _display3ddef_h
#define _display3ddef_h

#define TILE_WIDTH 128
#define TILE_HEIGHT 128
#define TILE_SIZE (TILE_WIDTH*TILE_HEIGHT)

#define	RADTLX		(OBJ_BACKX + OBJ_BACKWIDTH + BASE_GAP + 1 +D_W)	// Paul's settings (492+12)
#define	RADTLY		(RET_Y + 1)									// Paul's settings (332-17)
#define	RADWIDTH	128
#define RADHEIGHT	128
#define	RADBRX		(RADTLX + RADWIDTH) -1
#define	RADBRY		(RADTLY + RADHEIGHT) -1
//assigned to variable visibleXtiles, visibleYTiles 25/02/98 AB
#define VISIBLE_XTILES 64
#define VISIBLE_YTILES 64

#define MIN_TILE_X		(VISIBLE_XTILES/4)
#define MAX_TILE_X		((3*VISIBLE_XTILES)/4)

#define MIN_TILE_Y		(VISIBLE_YTILES/4)
#define MAX_TILE_Y		((3*VISIBLE_YTILES)/4)


#define LAND_XGRD	(VISIBLE_XTILES + 1)
#define LAND_YGRD	(VISIBLE_YTILES + 1)
#define MAXDISTANCE	(4500)
#define MINDISTANCE	(200)
#define START_DISTANCE	(2500)

#endif
