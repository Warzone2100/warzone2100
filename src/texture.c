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
#define PAGE_WIDTH		512
#define PAGE_HEIGHT		512
// 4 byte depth == 32 bpp
#define PAGE_DEPTH		4
#define TEXTURE_PAGE_SIZE	PAGE_WIDTH*PAGE_HEIGHT*PAGE_DEPTH

/* Stores the graphics data for the terrain tiles textures (in src/data.c) */
iTexture tilesPCX;

/* How many pages have we loaded */
SDWORD	firstTexturePage;
SDWORD	numTexturePages;
int		pageId[MAX_TERRAIN_PAGES];

/* Texture page and coordinates for each tile */
TILE_TEX_INFO	tileTexInfo[MAX_TILES];

static UDWORD	getTileXIndex(UDWORD tileNumber);
static UDWORD	getTileYIndex(UDWORD tileNumber);
static void getRectFromPage(UDWORD width, UDWORD height, char *src, UDWORD bufWidth, char *dest);
static void putRectIntoPage(UDWORD width, UDWORD height, char *dest, UDWORD bufWidth, char *src);
static void buildTileIndexes(void);

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
void	makeTileTexturePages(UDWORD srcWidth, UDWORD srcHeight, UDWORD tileWidth, UDWORD tileHeight, char *src)
{
	UDWORD	i,j;
	UDWORD	pageNumber;
	UDWORD	tilesAcross,tilesDown;
	UDWORD	tilesAcrossPage,tilesDownPage,tilesPerPage,tilesPerSource;
	UDWORD	tilesProcessed;
	char	*tileStorage;
	char	*presentLoc;
	iTexture sprite;

	/* This is how many pages are already used on hardware */
	firstTexturePage = pie_GetLastPageDownloaded() + 1;

	debug(LOG_TEXTURE, "makeTileTexturePages: src(%d,%d) tile(%d,%d) pages used=%d", srcWidth, srcHeight, tileWidth, tileHeight, firstTexturePage);

	/* Get enough memory to store one tile */
	pageNumber = 0;
	tileStorage = (char*)MALLOC(tileWidth * tileHeight * PAGE_DEPTH);
	sprite.bmp = (iBitmap*)MALLOC(TEXTURE_PAGE_SIZE);
	sprite.width = PAGE_WIDTH;
	sprite.height = PAGE_HEIGHT;
	tilesProcessed = 0;
	tilesAcross = srcWidth/tileWidth;
	tilesDown = srcHeight/tileHeight;
	tilesPerSource = tilesAcross*tilesDown;
	tilesAcrossPage = PAGE_WIDTH/tileWidth;
	tilesDownPage = PAGE_HEIGHT/tileHeight;
	tilesPerPage = tilesAcrossPage*tilesDownPage;
	presentLoc = sprite.bmp;

	for(i=0; i<tilesDown; i++)
	{
		for(j=0; j<tilesAcross; j++)
		{
			getRectFromPage(tileWidth,tileHeight,src,srcWidth,tileStorage);
			putRectIntoPage(tileWidth,tileHeight,presentLoc,PAGE_WIDTH,tileStorage);
			tilesProcessed++;
			presentLoc+=tileWidth*PAGE_DEPTH;
			src+=tileWidth*PAGE_DEPTH;
			/* Have we got all the tiles from the source!? */
			if((tilesProcessed == tilesPerSource))
			{
				pageId[pageNumber] = pie_AddBMPtoTexPages(&sprite, "terrain", 0, FALSE);
				goto exit;
			}

			/* Have we run out of texture page? */
			if(tilesProcessed%tilesPerPage == 0)
			{
				debug(LOG_TEXTURE, "makeTileTexturePages: ran out of texture page ...");
				debug(LOG_TEXTURE, "tilesDown=%d tilesAcross=%d tilesProcessed=%d tilesPerPage=%d",
				      tilesDown, tilesAcross, tilesProcessed, tilesPerPage);
				/* If so, download this one and reset to start again */
				pageId[pageNumber] = pie_AddBMPtoTexPages(&sprite, "terrain", 0, FALSE);
				sprite.bmp = (iBitmap*)MALLOC(TEXTURE_PAGE_SIZE);
				pageNumber++;
				presentLoc = sprite.bmp;
			}
			else if(tilesProcessed%tilesAcrossPage == 0)
			{
				/* Right hand side of texture page */
				/* So go to one tile down */
				presentLoc+= ( (tileHeight-1) * PAGE_WIDTH)*PAGE_DEPTH;
			}
		}
		src+=( (tileHeight-1) * srcWidth)*PAGE_DEPTH;
	}

	numTexturePages = pageNumber;

exit:
	FREE(tileStorage);
	buildTileIndexes();
	return;
}

void	remakeTileTexturePages(UDWORD srcWidth,UDWORD srcHeight, UDWORD tileWidth, UDWORD tileHeight, char *src)
{
	UDWORD	i,j;
	UDWORD	pageNumber;
	UDWORD	tilesAcross,tilesDown;
	UDWORD	tilesAcrossPage,tilesDownPage,tilesPerPage,tilesPerSource;
	UDWORD	tilesProcessed;
	char	*tileStorage;
	char	*presentLoc;
	iTexture sprite;
	//check enough pages are allocated

	debug(LOG_TEXTURE, "remakeTileTexturePages: src(%d,%d), tile(%d, %d)", srcWidth,
	      srcHeight, tileWidth, tileHeight);

	/* Get enough memory to store one tile */
	pageNumber = 0;
	tileStorage = (char*)MALLOC(tileWidth * tileHeight * PAGE_DEPTH);
	sprite.width = PAGE_WIDTH;
	sprite.height = PAGE_HEIGHT;

	sprite.bmp = (iBitmap*)MALLOC(TEXTURE_PAGE_SIZE);
//	memset(sprite.bmp,0,TEXTURE_PAGE_SIZE);
	tilesProcessed = 0;
	tilesAcross = srcWidth/tileWidth;
	tilesDown = srcHeight/tileHeight;
	tilesPerSource = tilesAcross*tilesDown;
	tilesAcrossPage = PAGE_WIDTH/tileWidth;
	tilesDownPage = PAGE_HEIGHT/tileHeight;
	tilesPerPage = tilesAcrossPage*tilesDownPage;
	presentLoc = sprite.bmp;

	for(i=0; i<tilesDown; i++)
	{
		for(j=0; j<tilesAcross; j++)
		{
			getRectFromPage(tileWidth,tileHeight,src,srcWidth,tileStorage);
			putRectIntoPage(tileWidth,tileHeight,presentLoc,PAGE_WIDTH,tileStorage);
			tilesProcessed++;
			presentLoc+=tileWidth*PAGE_DEPTH;
			src+=tileWidth*PAGE_DEPTH;
			/* Have we got all the tiles from the source!? */
			if((tilesProcessed == tilesPerSource))// || (tileStorage[0] == 0))//hack probably causes too many texture pages to be used
			{
				pie_ChangeTexPage(pageId[pageNumber], &sprite, 0, FALSE);
				goto exit;
			}

			/* Have we run out of texture page? */
			if(tilesProcessed%tilesPerPage == 0)
			{
				debug(LOG_TEXTURE, "remakeTileTexturePages: ran out of texture page ...");
				debug(LOG_TEXTURE, "tilesDown=%d tilesAcross=%d tilesProcessed=%d tilesPerPage=%d",
				      tilesDown, tilesAcross, tilesProcessed, tilesPerPage);
				pie_ChangeTexPage(pageId[pageNumber], &sprite, 0, FALSE);
				pageNumber++;
				presentLoc = sprite.bmp;
			}
			else if(tilesProcessed%tilesAcrossPage == 0)
			{
				/* Right hand side of texture page */
				/* So go to one tile down */
				presentLoc+= ( (tileHeight-1) * PAGE_WIDTH)*PAGE_DEPTH;
			}
		}
		src+=( (tileHeight-1) * srcWidth)*PAGE_DEPTH;
	}

	//check numTexturePages == pageNumber;
	ASSERT( numTexturePages >= (SDWORD)pageNumber,"New Tertiles too large" );

exit:
	FREE(tileStorage);
	buildTileIndexes();
	return;
}


BOOL getTileRadarColours(void)
{
	UDWORD x, y, i, j, w, h, t;
	iBitmap *b, *s;
	char tempBMP[TILE_WIDTH * TILE_HEIGHT];

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

void freeTileTextures(void)
{
	UDWORD	i;

	for (i = 0; i < numTexturePages; i++)
	{
		FREE(_TEX_PAGE[(firstTexturePage+i)].tex.bmp);
	}
}

static UDWORD	getTileXIndex(UDWORD tileNumber)
{
	UDWORD	texPage;
	UDWORD	tileInPage;
	UDWORD	xIndex;

	texPage = tileNumber/16;
	tileInPage = tileNumber - (texPage*16);
	xIndex = tileInPage%4;
	return(xIndex);
}

static UDWORD	getTileYIndex(UDWORD tileNumber)
{
	UDWORD	texPage;
	UDWORD	tileInPage;
	UDWORD	yIndex;

	texPage = tileNumber/16;
	tileInPage = tileNumber - (texPage*16);
	yIndex = tileInPage/4;
	return(yIndex);
}


/* Extracts a rectangular buffer from a source buffer, storing result in one contiguous
   chunk	*/
static void getRectFromPage(UDWORD width, UDWORD height, char *src, UDWORD bufWidth, char *dest)
{
	UDWORD	i,j;

	for (i=0; i<height; i++)
	{
		for(j=0; j<width*PAGE_DEPTH; j++)
		{
			*dest++ = *src++;
		}
		src+=(bufWidth-width)*PAGE_DEPTH;
	}
}

/* Inserts a rectangle into a dest rectangle */
static void putRectIntoPage(UDWORD width, UDWORD height, char *dest, UDWORD bufWidth, char *src)
{
UDWORD	i,j;

	for(i=0; i<height; i++)
	{
		for(j=0; j<width*PAGE_DEPTH; j++)
		{
			*dest++ = *src++;
		}
		dest+=(bufWidth-width)*PAGE_DEPTH;
	}
}

static void buildTileIndexes(void)
{
	UDWORD	i;

	for(i=0; i<MAX_TILES; i++)
	{
		tileTexInfo[i].xOffset = getTileXIndex(i);
		tileTexInfo[i].yOffset = getTileYIndex(i);
		tileTexInfo[i].texPage = pageId[(i/16)];//(i/16) + firstTexturePage;
	}
}
