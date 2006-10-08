/* display3d.h */

#ifndef _display3d_h
#define _display3d_h

#include "display3ddef.h"	// This should be the only place including this file
#include "lib/ivis_common/pietypes.h"
#include "lib/ivis_common/piedef.h"
#include "objectdef.h"
#include "message.h"

extern BOOL	xInOrder,yInOrder,yBeforeX,spinScene;

extern UDWORD	mapX,mapY;
extern void	setViewAngle(SDWORD angle);
extern UDWORD getViewDistance(void);
extern void	setViewDistance(UDWORD dist);
extern BOOL	radarOnScreen;
extern BOOL   rangeOnScreen;	// Added to get sensor/gun range on screen.  -Q 5-10-05
extern void	scaleMatrix( UDWORD percent );
extern void setViewPos( UDWORD x, UDWORD y, BOOL Pan);
extern void getPlayerPos(SDWORD *px, SDWORD *py);
extern void setPlayerPos(SDWORD x, SDWORD y);
extern void disp3d_setView(iView *newView);
extern void disp3d_getView(iView *newView);

extern void draw3DScene (void);
extern void	renderDroid					( DROID *psDroid );
extern void	renderStructure				( STRUCTURE *psStructure );
extern void	renderFeature				( FEATURE *psFeature );
extern void	renderProximityMsg			( PROXIMITY_DISPLAY	*psProxDisp);
extern void	drawTerrainTile				( UDWORD i, UDWORD j );	//fast version - optimised
void drawTerrainWaterTile(UDWORD i, UDWORD j);
extern void	drawTexturedTile			( UDWORD	i, UDWORD j );
extern void	renderProjectile			( PROJ_OBJECT *psCurr);
extern void	renderAnimComponent			( COMPONENT_OBJECT *psObj );
extern void	renderDeliveryPoint			( FLAG_POSITION *psPosition );
extern void debugToggleSensorDisplay	( void );

extern void displayFeatures( void );
extern void	displayStaticObjects( void );
extern void	displayDynamicObjects( void );
extern void displayProximityMsgs( void );
extern void displayDelivPoints(void);
extern void	calcScreenCoords(DROID *psDroid);
extern void	toggleReloadBarDisplay( void );
extern void	toggleEnergyBars( void );

extern BOOL	doWeDrawRadarBlips( void );
extern BOOL	doWeDrawProximitys( void );
extern void	setBlipDraw(BOOL val);
extern void	setProximityDraw(BOOL val);
extern void renderShadow( DROID *psDroid, iIMDShape *psShadowIMD );


extern UDWORD	getSuggestedPitch			( void );

extern BOOL	clipXY ( SDWORD x, SDWORD y);

extern int init3DView(void);
extern void initViewPosition(void);
extern iView player,camera;
extern iVector	imdRot;
extern UDWORD	distance;
extern UDWORD  terrainOutline;
extern SDWORD mouseTileX;
extern SDWORD mouseTileY;
extern UDWORD xOffset,yOffset;
extern BOOL	selectAttempt;
extern BOOL draggingTile;
extern struct iIMDShape *g_imd;
extern BOOL	droidSelected;
extern UDWORD terrainMidX,terrainMidY;
extern int32 playerXTile, playerZTile, rx, rz;

extern SDWORD scrollSpeed;
extern iBitmap	**tilesRAW;
extern UDWORD worldAngle;
extern iPalette	gamePal;
//extern void	assignSensorTarget( DROID *psDroid );
extern void	assignSensorTarget( BASE_OBJECT *psObj );
extern void	assignDestTarget( void );
extern void	processSensorTarget( void );
extern void	setEnergyBarDisplay( BOOL val );
extern UDWORD getWaterTileNum( void);
extern void	setUnderwaterTile(UDWORD num);
extern UDWORD	getRubbleTileNum( void );
extern void	setRubbleTile(UDWORD num);

extern SDWORD	getCentreX( void );
extern SDWORD	getCentreZ( void );


extern SDWORD	mouseTileX,mouseTileY;
extern BOOL	yBeforeX;
extern UDWORD numDroidsSelected;
extern UDWORD	intensity1,intensity2,intensity3;
extern UDWORD	lightLevel;
//extern BOOL		bScreenClose;
//extern UDWORD closingTimeStart;
//extern UDWORD screenCloseState;
//extern BOOL	bPlayerHasHQ;

#define	INITIAL_DESIRED_PITCH		(325)
#define INITIAL_STARTING_PITCH		(-75)
#define INITIAL_DESIRED_ROTATION	(-45)

extern BOOL bRender3DOnly;

extern UDWORD visibleXTiles;
extern UDWORD visibleYTiles;

// Expanded PIEVERTEX.
typedef struct {
	// PIEVERTEX.
	SDWORD x, y, z; UWORD tu, tv; PIELIGHT light, specular;
	SDWORD sx, sy, sz;
	// Extra data for water.
	SDWORD wx, wy, wz;
	SDWORD water_height;
	PIELIGHT wlight;
	UBYTE	drawInfo;
	UBYTE	bWater;
} SVMESH;

extern SVMESH tileScreenInfo[LAND_YGRD][LAND_XGRD];

/* load IMDs AFTER RESOURCE FILE */
extern BOOL loadExtraIMDs(void);

/*returns the graphic ID for a droid rank*/
extern UDWORD  getDroidRankGraphic(DROID *psDroid);

/* Visualize radius at position */
extern void showRangeAtPos(SDWORD centerX, SDWORD centerY, SDWORD radius);

#define	BASE_MUZZLE_FLASH_DURATION	(GAME_TICKS_PER_SEC/10)
#define	EFFECT_MUZZLE_ADDITIVE		128

#define CLOSING_TIME	800
#define LINE_TIME		600
#define DOT_TIME		200
#define SC_INACTIVE		1
#define SC_CLOSING_DOWN	2
#define SC_CLOSING_IN	3
#define SC_WAITING		4
#define SC_DOT_KILL		5

#define BAR_FULL	0
#define BAR_BASIC	1
#define BAR_DOT		2
#define BAR_NONE	3

extern UDWORD	barMode;

extern UDWORD	geoOffset;
#endif
