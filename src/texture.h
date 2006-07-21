#ifndef _texture_h
#define _texture_h

extern iSprite tilesPCX;

int makeTileTextures(void);
int remakeTileTextures(void);
void makeTileTexturePages(UDWORD srcWidth,UDWORD srcHeight, UDWORD tileWidth, UDWORD tileHeight, unsigned char *src);
void remakeTileTexturePages(UDWORD srcWidth,UDWORD srcHeight, UDWORD tileWidth, UDWORD tileHeight, unsigned char *src);
int	getTileRadarColours(void);
void	freeTileTextures( void );

typedef struct _tileTexInfo
{
UDWORD	xOffset;	// Offset into texture page to left hand edge
UDWORD	yOffset;	// Offset into texture page to top hand edge
UDWORD	texPage;	// Which textpage is the tile in? TileNumber/16 basically;
} TILE_TEX_INFO;

#define MAX_TILES 100
extern TILE_TEX_INFO	tileTexInfo[MAX_TILES];

#endif
