#ifdef WIN32
/* Texture stuff. Calls 3dfxText functions in the 3dfx cases */
/* Alex McLean, Pumpkin Studios, EIDOS Interactive, 1997 */

#include "Frame.h"
#include "pieTypes.h"
#include "pieState.h"
#include "dx6TexMan.h"
#include "pieTexture.h"
#include "piePalette.h"
#include "display3Ddef.h"
#include "Texture.h"
#include "Radar.h"
#include "tex.h"

/* Can fit at most 32 texture pages into a 2meg texture memory */
#define MAX_TEXTURE_PAGES	32
#define MAX_TERRAIN_PAGES	20
#define TEXTURE_PAGE_SIZE	65536	// 256 * 256
#define PAGE_WIDTH	256
#define PAGE_HEIGHT	256

#define NUM_OTHER_PAGES	19

iSprite	tempTexStore;
iPalette	tempPal;
/* Stores the graphics data for the terrain tiles textures */
iSprite tilesPCX;
/* Stores the raw PCX data for the terrain tiles at load file time */
iBitmap	**tilesRAW;
/* How many tiles have we loaded */
//UDWORD	numTiles;
UDWORD	numPCXTiles;
/* How many pages have we loaded (hardware)*/
SDWORD	firstTexturePage;
SDWORD	numTexturePages;
int		pageId[MAX_TERRAIN_PAGES];

/* Presently all texture pages are 256*256 big */
typedef struct _texturePage
{
UDWORD	pageNumber;
UDWORD	cardAddress;
} TEXTURE_PAGE_3DFX;


/* shouldn't be 100 - forgot where the define is?! */
TILE_TEX_INFO	tileTexInfo[MAX_TILES];

TEXTURE_PAGE_3DFX	allPages[MAX_TEXTURE_PAGES];

void	pcxBufferTo16Bit(UBYTE *origBuffer,UWORD *newBuffer);
void	getRectFromPage(UDWORD width, UDWORD height, unsigned char *src, UDWORD bufWidth, unsigned char *dest);
void	putRectIntoPage(UDWORD width, UDWORD height, unsigned char *dest, UDWORD bufWidth, unsigned char *src);

/* Extracts the tile texture in pcx format of abc..
											  def..
											  ghi..   (say)
   and puts them into raw format of a
									b
									c
									d
									e
									f
									.
									.
*/

int makeTileTextures(void)
{
	UDWORD x, y, i, j, w, h, t;
	iBitmap *b, *s, *saved;


	w = tilesPCX.width / TILE_WIDTH;
	h = tilesPCX.height / TILE_HEIGHT;
	numPCXTiles = w * h;

	tilesRAW = (iBitmap **) MALLOC(sizeof(iBitmap *) * numPCXTiles);

	for (i=0; i<numPCXTiles; tilesRAW[i++] = NULL)
	{
		; /* NOP */
	}

	t = 0;
	if (tilesRAW) 
	{
		for (i=0; i<h; i++) 
		{
			for (j=0; j<w; j++) 
			{
				b = tilesPCX.bmp + j * TILE_WIDTH + i * tilesPCX.width * TILE_HEIGHT;
				saved = s = tilesRAW[t++] = (iBitmap *)MALLOC(sizeof(iBitmap) * TILE_SIZE);
				if (s) 
				{
					for (y=0; y<TILE_HEIGHT; y++) 
					{
						for (x=0; x<TILE_WIDTH; *s++ = b[x++])
						{
							; /* NOP */
						}
						b+=tilesPCX.width;
					}
				  	calcRadarColour(saved,t-1);
				} 
				else 
				{
					return FALSE;
				}
			}
		}
	} 
	else 
	{
		return FALSE;
	}
	return TRUE;
}

int remakeTileTextures(void)
{
	UDWORD x, y, i, j, w, h, t;
	iBitmap *b, *s, *saved;


	w = tilesPCX.width / TILE_WIDTH;
	h = tilesPCX.height / TILE_HEIGHT;
	ASSERT((numPCXTiles >= w * h,"remakeTileTextures: New Tertiles larger than existing version"));

	//tilesRAW is already set up
	t = 0;
	if (tilesRAW) 
	{
		for (i=0; i<h; i++) 
		{
			for (j=0; j<w; j++) 
			{
				b = tilesPCX.bmp + j * TILE_WIDTH + i * tilesPCX.width * TILE_HEIGHT;
				saved = s = tilesRAW[t++];
				if (s) 
				{
					for (y=0; y<TILE_HEIGHT; y++) 
					{
						for (x=0; x<TILE_WIDTH; *s++ = b[x++])
						{
							; /* NOP */
						}
						b+=tilesPCX.width;
					}
					calcRadarColour(saved,t-1);
				} 
				else 
				{
					return FALSE;
				}

			}
		}
	} 
	else 
	{
		return FALSE;
	}

	return TRUE;
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
void	makeTileTexturePages(UDWORD srcWidth,UDWORD srcHeight, UDWORD tileWidth, UDWORD tileHeight, unsigned char *src)
{
UDWORD	i,j;
UDWORD	pageNumber;
UDWORD	tilesAcross,tilesDown;
UDWORD	tilesAcrossPage,tilesDownPage,tilesPerPage,tilesPerSource;
UDWORD	tilesProcessed;
unsigned char	*tileStorage;
unsigned char	*presentLoc;
iSprite	sprite;

	/* This is how many pages are already used on hardware */
	firstTexturePage = pie_GetLastPageDownloaded() + 1;

	/* Get enough memory to store one tile */
	pageNumber = 0;
	tileStorage = MALLOC(tileWidth*tileHeight);
	sprite.bmp = MALLOC(TEXTURE_PAGE_SIZE);
	sprite.width = PAGE_WIDTH;
	sprite.height = PAGE_HEIGHT;
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
			presentLoc+=tileWidth;
			src+=tileWidth;
			/* Have we got all the tiles from the source!? */
			if((tilesProcessed == tilesPerSource))	// || (tileStorage[0] == 0))//hack probably causes too many texture pages to be used
			{
//			   	pie_Download8bitTexturePage(texturePage,PAGE_WIDTH,PAGE_HEIGHT);
				pageId[pageNumber] = pie_AddBMPtoTexPages( 	&sprite, "terrain", 0, TRUE, FALSE);
				goto exit;
			}

			/* Have we run out of texture page? */
			if(tilesProcessed%tilesPerPage == 0)
			{
				/* If so, download this one and reset to start again */
				pageId[pageNumber] = pie_AddBMPtoTexPages( 	&sprite, "terrain", 0, TRUE, FALSE);
				sprite.bmp = MALLOC(TEXTURE_PAGE_SIZE);
				pageNumber++;
				presentLoc = sprite.bmp;
			}
			else if(tilesProcessed%tilesAcrossPage == 0)
			{
				/* Right hand side of texture page */
				/* So go to one tile down */
				presentLoc+= ( (tileHeight-1) * PAGE_WIDTH);
			}
		}
		src+=( (tileHeight-1) * srcWidth);
	}

	numTexturePages = pageNumber;

exit:
	FREE(tileStorage);
	buildTileIndexes();
	return;
}

void	remakeTileTexturePages(UDWORD srcWidth,UDWORD srcHeight, UDWORD tileWidth, UDWORD tileHeight, unsigned char *src)
{
UDWORD	i,j;
UDWORD	pageNumber;
UDWORD	tilesAcross,tilesDown;
UDWORD	tilesAcrossPage,tilesDownPage,tilesPerPage,tilesPerSource;
UDWORD	tilesProcessed;
unsigned char	*tileStorage;
unsigned char	*presentLoc;
iSprite	sprite;
SDWORD  index;
	//check enough pages are allocated

	/* Get enough memory to store one tile */
	pageNumber = 0;
	tileStorage = MALLOC(tileWidth*tileHeight);
//	texturePage = MALLOC(TEXTURE_PAGE_SIZE);
	sprite.width = PAGE_WIDTH;
	sprite.height = PAGE_HEIGHT;

	sprite.bmp = _TEX_PAGE[pageId[pageNumber]].tex.bmp;
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
			presentLoc+=tileWidth;
			src+=tileWidth;
			/* Have we got all the tiles from the source!? */
			if((tilesProcessed == tilesPerSource))// || (tileStorage[0] == 0))//hack probably causes too many texture pages to be used
			{
				if(pie_GetRenderEngine() == ENGINE_GLIDE)
				{
					index = iV_TEXPAGE(firstTexturePage+pageNumber)->textPage3dfx;
					pageId[pageNumber] = index;
					pie_Reload8bitTexturePage(sprite.bmp,(UWORD) sprite.width,(UWORD) sprite.height, index);
				}
				else //D3D
				{
					dtm_LoadTexSurface(&_TEX_PAGE[pageId[pageNumber]].tex, pageId[pageNumber]);
				}
				goto exit;
			}

			/* Have we run out of texture page? */
			if(tilesProcessed%tilesPerPage == 0)
			{
				if(pie_GetRenderEngine() == ENGINE_GLIDE)
				{
					/* If so, download this one and reset to start again */
					index = iV_TEXPAGE(firstTexturePage+pageNumber)->textPage3dfx;
					pageId[pageNumber] = index;
					pie_Reload8bitTexturePage(sprite.bmp, (UWORD)sprite.width, (UWORD)sprite.height, index);
				}
				else //D3D
				{
					/* If so, download this one and reset to start again */
					dtm_LoadTexSurface(&_TEX_PAGE[pageId[pageNumber]].tex, pageId[pageNumber]);
				}
				pageNumber++;
				sprite.bmp = _TEX_PAGE[pageId[pageNumber]].tex.bmp;
				presentLoc = sprite.bmp;
			}
			else if(tilesProcessed%tilesAcrossPage == 0)
			{
				/* Right hand side of texture page */
				/* So go to one tile down */
				presentLoc+= ( (tileHeight-1) * PAGE_WIDTH);
			}
		}
		src+=( (tileHeight-1) * srcWidth);
	}

	//check numTexturePages == pageNumber;
	ASSERT((numTexturePages >= (SDWORD)pageNumber,"New Tertiles too large"));

exit:
	FREE(tileStorage);
	buildTileIndexes();
	return;
}


BOOL getTileRadarColours(void)
{
	UDWORD x, y, i, j, w, h, t;
	iBitmap *b, *s;
	UBYTE tempBMP[TILE_WIDTH * TILE_HEIGHT];

	w = tilesPCX.width / TILE_WIDTH;
	h = tilesPCX.height / TILE_HEIGHT;
	numPCXTiles = w * h;

	

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

void	freeTileTextures( void )
{
	UDWORD	i;
	if (!pie_Hardware())
	{
		for(i=0; i<numPCXTiles; i++)
		{
			FREE(tilesRAW[i]);		
		}
		FREE(tilesRAW);
	}
	else
	{
		for(i=0; i< ((UDWORD)numTexturePages); i++)
		{
			FREE(_TEX_PAGE[(firstTexturePage+i)].tex.bmp);
		}
	}
}

UDWORD	getTileXIndex(UDWORD tileNumber)
{
UDWORD	texPage;
UDWORD	tileInPage;
UDWORD	xIndex;


	texPage = tileNumber/16;
	tileInPage = tileNumber - (texPage*16);
	xIndex = tileInPage%4;
	return(xIndex);
}

UDWORD	getTileYIndex(UDWORD tileNumber)
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
void	getRectFromPage(UDWORD width, UDWORD height, unsigned char *src, UDWORD bufWidth, unsigned char *dest)
{
UDWORD	i,j;

	for (i=0; i<height; i++)
	{
		for(j=0; j<width; j++)
		{
			*dest++ = *src++;
		}
		src+=(bufWidth-width);
	}
}

/* Inserts a rectangle into a dest rectangle */
void	putRectIntoPage(UDWORD width, UDWORD height, unsigned char *dest, UDWORD bufWidth, unsigned char *src)
{
UDWORD	i,j;

	for(i=0; i<height; i++)
	{
		for(j=0; j<width; j++)
		{
			*dest++ = *src++;
		}
		dest+=(bufWidth-width);
	}
}

void	buildTileIndexes( void )
{
UDWORD	i;


	for(i=0; i<MAX_TILES; i++)
	{
		tileTexInfo[i].xOffset = getTileXIndex(i);
		tileTexInfo[i].yOffset = getTileYIndex(i);
		tileTexInfo[i].texPage = pageId[(i/16)];//(i/16) + firstTexturePage;
	}
}

/*	Converts an 8 bit raw (palettised) source image to
	a 16 bit argb destination image 
*/
void	pcxBufferTo16Bit(UBYTE *origBuffer,UWORD *newBuffer)
{
UBYTE	paletteIndex;
UWORD	newColour;
UWORD	mask;
UDWORD	i;
iColour		*psCurrentPalette;

	psCurrentPalette = pie_GetGamePal();
	/*	640*480, 8 bit colour source image 
		640*480, 16 bit colour destination image
	*/
	for(i=0; i<640*480; i++)
	{
		/* Get the next colour */
		paletteIndex = *origBuffer++;
		/* Flush out destination word (and alpha bits) */
		newColour = 0;
		/* Get red bits - 5 */
		mask = (UWORD)(psCurrentPalette[paletteIndex].r>>3);
		newColour = (UWORD)(newColour | mask);
		newColour = (UWORD)(newColour<<6);
		/* Get green bits - 6 */
		mask = (UWORD)(psCurrentPalette[paletteIndex].g>>2);
		newColour = (UWORD)(newColour | mask);
		newColour = (UWORD)(newColour<<5);
		/* Get blue bits - 5 */
		mask = (UWORD)(psCurrentPalette[paletteIndex].b>>3);
		newColour = (UWORD)(newColour | mask);
		/* Copy over */
		*newBuffer++ = newColour;
	}
}




#endif