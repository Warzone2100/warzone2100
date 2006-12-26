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

//different mini-map draw modes
typedef enum _radar_draw_mode
{
	RADAR_MODE_TERRAIN,						//draw texture map
	RADAR_MODE_DEFAULT = RADAR_MODE_TERRAIN,
	RADAR_MODE_HEIGHT_MAP,					//draw height map
	RADAR_MODE_NO_TERRAIN,					//draw only objects
	NUM_RADAR_MODES
}RADAR_DRAW_MODE;

extern BOOL		bEnemyAllyRadarColor;		//enemy/ally radar color
extern RADAR_DRAW_MODE	radarDrawMode;		//current mini-map mode

#endif
