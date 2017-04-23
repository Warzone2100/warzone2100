/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_DISPLAY3DDEF_H__
#define __INCLUDED_SRC_DISPLAY3DDEF_H__

#define TILE_WIDTH 128
#define TILE_HEIGHT 128
#define TILE_SIZE (TILE_WIDTH*TILE_HEIGHT)

// Amount of visible terrain tiles in x/y direction
#define VISIBLE_XTILES 64
#define VISIBLE_YTILES 64

#define	RADTLX		(OBJ_BACKX + OBJ_BACKWIDTH + BASE_GAP + 1 +D_W)	// Paul's settings (492+12)
#define	RADTLY		(RET_Y + 1)									// Paul's settings (332-17)
#define	RADWIDTH	128
#define RADHEIGHT	128

#define SKY_MULT	1
#define SKY_SHIMMY_BASE	((DEG(1)*SKY_MULT)/2)
#define SKY_SHIMMY (SKY_SHIMMY_BASE - (rand()%(2*SKY_SHIMMY_BASE)))

#endif // __INCLUDED_SRC_DISPLAY3DDEF_H__
