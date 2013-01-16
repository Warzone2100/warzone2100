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

#ifndef __INCLUDED_SRC_TEXTURE_H__
#define __INCLUDED_SRC_TEXTURE_H__

#include "display3ddef.h"

bool texLoad(const char *fileName);

struct TILE_TEX_INFO
{
	float uOffset; // Offset into texture page to left hand edge
	float vOffset; // Offset into texture page to top hand edge
	unsigned int texPage; // Which textpage is the tile in? TileNumber/16 basically;
};

// these constants are adapted for fitting 256 textures of size 128x128 into a 2048x2048
// texture page; if such large texture pages are not available, just scaled everything down
#define TILES_IN_PAGE_COLUMN	16
#define TILES_IN_PAGE_ROW	16
#define TILES_IN_PAGE		(TILES_IN_PAGE_COLUMN * TILES_IN_PAGE_ROW)
#define MAX_TILES		TILES_IN_PAGE

extern TILE_TEX_INFO	tileTexInfo[MAX_TILES];
extern int terrainPage;

void setTextureSize(int texSize);
int getTextureSize(void);

#endif // __INCLUDED_SRC_TEXTURE_H__
