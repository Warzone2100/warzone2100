/*
 * Disp2D.h
 *
 * Definitions for the display system structures and routines.
 *
 */
#ifdef DISP2D

#ifndef _disp2d_h
#define _disp2d_h

/* The size of map tiles on the screen in pixels */
#define TILE_SIZE2D 32
#define SCR_TILE_SHIFT 5

#define TILES_ACROSS	(DISP_WIDTH/TILE_SIZE2D)
#define TILES_DOWN		(DISP_HEIGHT/TILE_SIZE2D)

/* Number of tiles across the tile texture page */
extern UDWORD tilesPerLine;

/* The maximum tile number */
extern UDWORD maxTexTile;

/* Map Position of top right hand corner of the screen */
extern UDWORD viewX,viewY;

/* Initialise the display system */
extern BOOL disp2DInitialise(void);

/* Shutdown the display system */
extern BOOL disp2DShutdown(void);

/* Tidy up after a mode change */
extern BOOL disp2DModeChange(void);

/* Process the key presses for the 2D display */
extern BOOL process2DInput(void);

/* Display a texture tile in 2D */
extern void blitTile(RECT *psDestRect, RECT *psSrcRect, UDWORD texture);

/* Display the terrain type as a coloured block */
void dispTerrain(UDWORD x, UDWORD y, TYPE_OF_TERRAIN type);

/* Display the world */
extern void display2DWorld(void);

/* Return the world coords of a screen coordinate */
extern void disp2DToWorld(UDWORD scrX, UDWORD scrY, UDWORD *pWorldX, UDWORD *pWorldY);

/* Return the world coords of a screen coordinate */
extern void disp2DFromWorld(UDWORD worldX, UDWORD worldY, SDWORD *pScrX, SDWORD *pScrY);

/* Start looking for a structure location */
extern void disp2DStartStructPosition(BASE_STATS *psStats);

/* Stop looking for a structure location */
extern void disp2DStopStructPosition(void);

/* See if a structure location has been found */
extern BOOL disp2DGetStructPosition(UDWORD *pX, UDWORD *pY);


extern void showGameStats(void);

/* Draws an ellipse to show the range of the droids sensors */
extern void	showDroidRange(DROID *psDroid);

extern BOOL wanderAbout;
extern UDWORD	selectedPlayer;

#endif

#endif

