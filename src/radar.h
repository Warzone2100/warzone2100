#ifndef _radar_h
#define _radar_h


extern void	calcRadarColour(iBitmap *tileBitmap, UDWORD tileNumber);



#define RGB_ENTRIES	3

#define MAX_RADARZOOM 2


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



#endif
