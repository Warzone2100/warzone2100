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
/* Texture stuff. Alex McLean, Pumpkin Studios, EIDOS Interactive, 1997 */
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/ivis_common/pietypes.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/tex.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_opengl/pietexture.h"
#include "display3ddef.h"
#include "texture.h"
#include "radar.h"

#define MAX_TERRAIN_PAGES	20
// 4 byte depth == 32 bpp
#define PAGE_DEPTH		4
#define TEXTURE_PAGE_SIZE	PAGE_WIDTH*PAGE_HEIGHT*PAGE_DEPTH

/* Stores the graphics data for the terrain tiles textures (in src/data.c) */
static iTexture tilesPCX = { 0, 0, 0, NULL };

/* How many pages have we loaded */
static SDWORD	firstTexturePage;
static SDWORD	numTexturePages;
static int	pageId[MAX_TERRAIN_PAGES];

/* Texture page and coordinates for each tile */
TILE_TEX_INFO tileTexInfo[MAX_TILES];

static void getRectFromPage(UDWORD width, UDWORD height, unsigned char *src, UDWORD bufWidth, unsigned char *dest);
static void putRectIntoPage(UDWORD width, UDWORD height, unsigned char *dest, UDWORD bufWidth, unsigned char *src);
static void buildTileIndexes(void);
static void makeTileTexturePages(iV_Image * src, UDWORD tileWidth, UDWORD tileHeight);
static void remakeTileTexturePages(iV_Image * src, UDWORD tileWidth, UDWORD tileHeight);
static void freeTileTextures(void);

void texInit()
{
	tilesPCX.bmp = NULL;
}

void texDone()
{
	freeTileTextures();
	iV_unloadImage(&tilesPCX);
}

// just return a pointer because the resource handler wants to cuddle one
void *texLoad(const char *fileName)
{
	BOOL bTilesPCXLoaded = (tilesPCX.bmp != NULL);

	if (bTilesPCXLoaded)
	{
		debug(LOG_TEXTURE, "Unloading terrain tiles");
		iV_unloadImage(&tilesPCX);
	}

	if(!iV_loadImage_PNG(fileName, &tilesPCX))
	{
		debug( LOG_ERROR, "TERTILES load failed" );
		return NULL;
	}

	getTileRadarColours();
	if (bTilesPCXLoaded)
	{
		remakeTileTexturePages(&tilesPCX, TILE_WIDTH, TILE_HEIGHT);
	}
	else
	{
		makeTileTexturePages(&tilesPCX, TILE_WIDTH, TILE_HEIGHT);
	}
	return &tilesPCX;
}

/*
	Extracts the tile textures into separate texture pages and builds
	a table of which texture page to find each tile in, as well as which one it is
	within that page.

	0123
	4567
	89AB
	CDEF

	The above shows the different possible locations for a tile in the page.
	So we have a table of MAX_TILES showing

	pageNumber and [0..15]

	We must then make sure that we source in that texture page and set the
	texture coordinate for a complete tile to be its position.
*/
void makeTileTexturePages(iV_Image * src, UDWORD tileWidth, UDWORD tileHeight)
{
	UDWORD i, j;
	UDWORD pageNumber = 0;
	UDWORD tilesAcross = src->width / tileWidth, tilesDown = src->height / tileHeight;
	UDWORD tilesAcrossPage = PAGE_WIDTH / tileWidth, tilesDownPage = PAGE_HEIGHT / tileHeight;
	UDWORD tilesPerPage = tilesAcrossPage * tilesDownPage, tilesPerSource = tilesAcross * tilesDown;
	UDWORD tilesProcessed = 0;
	iTexture sprite = { PAGE_WIDTH, PAGE_HEIGHT, PAGE_DEPTH, malloc(TEXTURE_PAGE_SIZE) };
	/* Get enough memory to store one tile */
	unsigned char *tileStorage = malloc(tileWidth * tileHeight * PAGE_DEPTH);
	unsigned char *presentLoc = sprite.bmp;
	unsigned char *srcBmp = src->bmp;

	/* This is how many pages are already used on hardware */
	firstTexturePage = pie_GetLastPageDownloaded();

	debug(LOG_TEXTURE, "makeTileTexturePages: src(%d,%d) tile(%d,%d) pages used=%d", src->width, src->height, tileWidth, tileHeight, firstTexturePage);

	for (i=0; i<tilesDown; i++)
	{
		for (j=0; j<tilesAcross; j++)
		{
			getRectFromPage(tileWidth, tileHeight, srcBmp, src->width, tileStorage);
			putRectIntoPage(tileWidth, tileHeight, presentLoc, PAGE_WIDTH, tileStorage);
			tilesProcessed++;
			presentLoc += tileWidth * PAGE_DEPTH;
			srcBmp += tileWidth * PAGE_DEPTH;
			/* Have we got all the tiles from the source!? */
			if (tilesProcessed == tilesPerSource)
			{
				pageId[pageNumber] = pie_AddTexPage(&sprite, "terrain", 0, FALSE);
				goto exit;
			}

			/* Have we run out of texture page? */
			if (tilesProcessed % tilesPerPage == 0)
			{
				debug(LOG_TEXTURE, "makeTileTexturePages: ran out of texture page ...");
				debug(LOG_TEXTURE, "tilesDown=%d tilesAcross=%d tilesProcessed=%d tilesPerPage=%d", tilesDown, tilesAcross, tilesProcessed, tilesPerPage);
				/* If so, download this one and reset to start again */
				pageId[pageNumber] = pie_AddTexPage(&sprite, "terrain", 0, FALSE);
				sprite.bmp = malloc(TEXTURE_PAGE_SIZE);
				presentLoc = sprite.bmp;
				pageNumber++;
			}
			else if (tilesProcessed % tilesAcrossPage == 0)
			{
				/* Right hand side of texture page */
				/* So go to one tile down */
				presentLoc += (tileHeight-1) * PAGE_WIDTH * PAGE_DEPTH;
			}
		}
		srcBmp += (tileHeight-1) * src->width * PAGE_DEPTH;
	}
	ASSERT(FALSE, "we should have exited the loop using the goto");

exit:
	numTexturePages = pageNumber+1;
	free(tileStorage);
	buildTileIndexes();
	return;
}

void remakeTileTexturePages(iV_Image * src, UDWORD tileWidth, UDWORD tileHeight)
{
	UDWORD i, j;
	UDWORD pageNumber = 0;
	UDWORD tilesAcross = src->width / tileWidth, tilesDown = src->height / tileHeight;
	UDWORD tilesAcrossPage = PAGE_WIDTH / tileWidth, tilesDownPage = PAGE_HEIGHT / tileHeight;
	UDWORD tilesPerPage = tilesAcrossPage * tilesDownPage, tilesPerSource = tilesAcross * tilesDown;
	UDWORD tilesProcessed = 0;
	iTexture sprite = { PAGE_WIDTH, PAGE_HEIGHT, PAGE_DEPTH, malloc(TEXTURE_PAGE_SIZE) };
	/* Get enough memory to store one tile */
	unsigned char *tileStorage = malloc(tileWidth * tileHeight * PAGE_DEPTH);
	unsigned char *presentLoc = sprite.bmp;
	unsigned char *srcBmp = src->bmp;

	debug(LOG_TEXTURE, "remakeTileTexturePages: src(%d,%d), tile(%d, %d)", src->width, src->height, tileWidth, tileHeight);

	for (i=0; i<tilesDown; i++)
	{
		for (j=0; j<tilesAcross; j++)
		{
			getRectFromPage(tileWidth, tileHeight, srcBmp, src->width, tileStorage);
			putRectIntoPage(tileWidth, tileHeight, presentLoc, PAGE_WIDTH, tileStorage);
			tilesProcessed++;
			presentLoc += tileWidth * PAGE_DEPTH;
			srcBmp += tileWidth * PAGE_DEPTH;
			/* Have we got all the tiles from the source!? */
			if (tilesProcessed == tilesPerSource)
			{
				pie_ChangeTexPage(pageId[pageNumber], &sprite, 0, FALSE);
				goto exit;
			}

			/* Have we run out of texture page? */
			if (tilesProcessed % tilesPerPage == 0)
			{
				debug(LOG_TEXTURE, "remakeTileTexturePages: ran out of texture page ...");
				debug(LOG_TEXTURE, "tilesDown=%d tilesAcross=%d tilesProcessed=%d tilesPerPage=%d", tilesDown, tilesAcross, tilesProcessed, tilesPerPage);
				pie_ChangeTexPage(pageId[pageNumber], &sprite, 0, FALSE);
				pageNumber++;
				presentLoc = sprite.bmp;
			}
			else if (tilesProcessed % tilesAcrossPage == 0)
			{
				/* Right hand side of texture page */
				/* So go to one tile down */
				presentLoc += (tileHeight-1) * PAGE_WIDTH * PAGE_DEPTH;
			}
		}
		srcBmp += (tileHeight-1) * src->width * PAGE_DEPTH;
	}

	//check numTexturePages == pageNumber;
	ASSERT( numTexturePages >= (SDWORD)pageNumber, "New Tertiles too large" );

exit:
	free(tileStorage);
	buildTileIndexes();
	return;
}


BOOL getTileRadarColours(void)
{
	UDWORD x, y, i, j, w, h, t;
	iBitmap *b, *s;
	unsigned char tempBMP[TILE_WIDTH * TILE_HEIGHT];

	w = tilesPCX.width / TILE_WIDTH;
	h = tilesPCX.height / TILE_HEIGHT;

	t = 0;
	for (i=0; i<h; i++)
	{
		for (j=0; j<w; j++)
		{
			b = tilesPCX.bmp + j * TILE_WIDTH + i * tilesPCX.width * TILE_HEIGHT;
			s = &tempBMP[0];
			if (s)
			{
				//copy the bitmap to temp buffer for colour calc
				for (y=0; y<TILE_HEIGHT; y++)
				{
					for (x=0; x<TILE_WIDTH; *s++ = b[x++])
					{
						; /* NOP */
					}
					b+=tilesPCX.width;
				}
				calcRadarColour(&tempBMP[0],t);
				t++;
			}
			else
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

static void freeTileTextures(void)
{
	unsigned int i;

	for (i = 0; i < numTexturePages; i++)
	{
		iV_unloadImage(&_TEX_PAGE[(firstTexturePage+i)].tex);
	}
}

static inline WZ_DECL_CONST unsigned int getTileUIndex(unsigned int tileNumber)
{
	return (tileNumber % TILES_IN_PAGE) % TILES_IN_PAGE_COLUMN;
}

static inline WZ_DECL_CONST unsigned int getTileVIndex(unsigned int tileNumber)
{
	return (tileNumber % TILES_IN_PAGE) / TILES_IN_PAGE_ROW;
}

/* Extracts a rectangular buffer from a source buffer, storing result in one contiguous
   chunk	*/
static void getRectFromPage(UDWORD width, UDWORD height, unsigned char *src, UDWORD bufWidth, unsigned char *dest)
{
	unsigned int i, j;

	for (i = 0; i < height; i++)
	{
		for(j = 0; j < width * PAGE_DEPTH; j++)
		{
			*dest++ = *src++;
		}
		src += (bufWidth - width) * PAGE_DEPTH;
	}
}

/* Inserts a rectangle into a dest rectangle */
static void putRectIntoPage(UDWORD width, UDWORD height, unsigned char *dest, UDWORD bufWidth, unsigned char *src)
{
	unsigned int i, j;

	for(i = 0; i < height; i++)
	{
		for(j = 0; j < width * PAGE_DEPTH; j++)
		{
			*dest++ = *src++;
		}
		dest += (bufWidth - width) * PAGE_DEPTH;
	}
}

static void buildTileIndexes(void)
{
	unsigned int i;

	for(i = 0; i < MAX_TILES; i++)
	{
		tileTexInfo[i].uOffset = getTileUIndex(i) * (256 / TILES_IN_PAGE_COLUMN);
		tileTexInfo[i].vOffset = getTileVIndex(i) * (256 / TILES_IN_PAGE_ROW);
		tileTexInfo[i].texPage = pageId[i / TILES_IN_PAGE];
	}
}
