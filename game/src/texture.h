#ifndef _texture_h
#define _texture_h

extern iSprite tilesPCX;

extern	int		makeTileTextures(void);
extern	int		remakeTileTextures(void);
extern	void	makeTileTexturePages(UDWORD srcWidth,UDWORD srcHeight, UDWORD tileWidth, UDWORD tileHeight, unsigned char *src);
extern	void	remakeTileTexturePages(UDWORD srcWidth,UDWORD srcHeight, UDWORD tileWidth, UDWORD tileHeight, unsigned char *src);
extern	int		getTileRadarColours(void);
extern	void	freeTileTextures( void );
extern	UDWORD	getTileXIndex(UDWORD tileNumber);
extern	UDWORD	getTileYIndex(UDWORD tileNumber);
extern	void	buildTileIndexes( void );
extern	void	pcxBufferTo16Bit(UBYTE *origBuffer,UWORD *newBuffer);

typedef struct _tileTexInfo
{
UDWORD	xOffset;	// Offset into texture page to left hand edge
UDWORD	yOffset;	// Offset into texture page to top hand edge
UDWORD	texPage;	// Which textpage is the tile in? TileNumber/16 basically;
} TILE_TEX_INFO;

#define MAX_TILES 100
extern TILE_TEX_INFO	tileTexInfo[MAX_TILES];

#endif
