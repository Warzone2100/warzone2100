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

extern iTexture tilesPCX;

extern void makeTileTexturePages(iV_Image * src, UDWORD tileWidth, UDWORD tileHeight);
extern void remakeTileTexturePages(iV_Image * src, UDWORD tileWidth, UDWORD tileHeight);
extern BOOL getTileRadarColours(void);
extern void freeTileTextures( void );

typedef struct _tileTexInfo
{
	UDWORD xOffset; // Offset into texture page to left hand edge
	UDWORD yOffset; // Offset into texture page to top hand edge
	UDWORD texPage; // Which textpage is the tile in? TileNumber/16 basically;
} TILE_TEX_INFO;

#define PAGE_WIDTH              512
#define PAGE_HEIGHT             512
#define TILES_IN_PAGE_COLUMN (PAGE_WIDTH / TILE_WIDTH)
#define TILES_IN_PAGE_ROW (PAGE_HEIGHT / TILE_HEIGHT)
#define TILES_IN_PAGE (TILES_IN_PAGE_COLUMN * TILES_IN_PAGE_ROW)

#define MAX_TILES 100
extern TILE_TEX_INFO	tileTexInfo[MAX_TILES];

#endif
