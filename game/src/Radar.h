#ifndef _radar_h
#define _radar_h

#ifdef WIN32
extern void	calcRadarColour(iBitmap *tileBitmap, UDWORD tileNumber);
#else
extern void	calcRadarColour(iBitmap *tileBitmap,UWORD *tileClut, UDWORD tileNumber);
#endif


#define RGB_ENTRIES	3
#ifdef WIN32
#define MAX_RADARZOOM 2
#else
#define MAX_RADARZOOM 1
#endif

//#define RADAR_POSITION_AT_ZOOM

/* Radar.h */
extern void resetRadarRedraw(void);
extern BOOL InitRadar(void);
extern BOOL ShutdownRadar(void);
extern void	drawRadar(void);
extern void CalcRadarPosition(UDWORD mX,UDWORD mY,UDWORD *PosX,UDWORD *PosY);
extern void worldPosToRadarPos(UDWORD wX,UDWORD wY,SDWORD *rX, SDWORD *rY);
extern void SetRadarStrobe(UDWORD x,UDWORD y);
extern void SetRadarZoom(UWORD ZoomLevel);
extern UDWORD GetRadarZoom(void);
extern BOOL CoordInRadar(int x,int y);
extern void GetRadarPlayerPos(UDWORD *XPos,UDWORD *YPos);
extern	void	downloadAtStartOfFrame( void );

//#define RADAR_ROT	1

#ifdef PSX
extern BOOL InitRadar_PSX(UWORD Width,UWORD Height);
extern void ReleaseRadar_PSX(void);
extern void UpdateRadar_PSX(UWORD mapWidth,UWORD mapHeight);
extern void DrawRadar_PSX(void);
extern void DrawRadarExtras_PSX(void);
#endif

#endif