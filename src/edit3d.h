#ifndef _edit3d_h
#define _edit3d_h

#define TILE_RAISE	1
#define TILE_LOWER	-1
#define MAX_TILE_HEIGHT 255
#define MIN_TILE_HEIGHT	0

typedef void (*BUILDCALLBACK)(UDWORD xPos, UDWORD yPos,void *UserData);

extern void Edit3DInitVars(void);
extern	BOOL	found3DBuilding		( UDWORD *x, UDWORD *y );
extern  BOOL    found3DBuildLocTwo  ( UDWORD *px1, UDWORD *py1, UDWORD *px2, UDWORD *py2);
extern void init3DBuilding(BASE_STATS *psStats,BUILDCALLBACK CallBack,void *UserData);
extern	void	kill3DBuilding		( void );
extern BOOL process3DBuilding(void);

extern void	adjustTileHeight	( MAPTILE *psTile, SDWORD adjust );
extern void	raiseTile			( UDWORD tile3dX, UDWORD tile3dY );
extern void	lowerTile			( UDWORD tile3dX, UDWORD tile3dY );
BOOL	inHighlight				( UDWORD realX, UDWORD realY );

typedef struct _highlight
{
UWORD	xTL,yTL;		// Top left of box to highlight
UWORD	xBR,yBR;		// Bottom right of box to highlight
} HIGHLIGHT;

extern HIGHLIGHT	buildSite;


#define BUILD3D_NONE		99
#define BUILD3D_POS			100
#define BUILD3D_FINISHED	101
#define BUILD3D_VALID		102


typedef struct _build_details
{
	BUILDCALLBACK	CallBack;
	void 			*UserData;  //this holds the OBJECT_POSITION pointer for a Deliv Point
	UDWORD			x,y;
	UDWORD			width,height;
	BASE_STATS		*psStats;
} BUILDDETAILS;

extern BUILDDETAILS	sBuildDetails;

extern UDWORD buildState;
extern UDWORD temp;

/*returns true if the build state is not equal to BUILD3D_NONE*/
extern BOOL   tryingToGetLocation(void);

#endif
