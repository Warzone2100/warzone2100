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
#ifndef _texture_h
#define _texture_h

#include "display3ddef.h"

void texLoad(const char *fileName);

typedef struct _tileTexInfo
{
	float uOffset; // Offset into texture page to left hand edge
	float vOffset; // Offset into texture page to top hand edge
	unsigned int texPage; // Which textpage is the tile in? TileNumber/16 basically;
} TILE_TEX_INFO;

#define PAGE_WIDTH              2048
#define PAGE_HEIGHT             2048
#define TILES_IN_PAGE_COLUMN (PAGE_WIDTH / TILE_WIDTH)
#define TILES_IN_PAGE_ROW (PAGE_HEIGHT / TILE_HEIGHT)
#define TILES_IN_PAGE (TILES_IN_PAGE_COLUMN * TILES_IN_PAGE_ROW)

#define MAX_TILES TILES_IN_PAGE
extern TILE_TEX_INFO	tileTexInfo[MAX_TILES];
extern int terrainPage;

#endif
