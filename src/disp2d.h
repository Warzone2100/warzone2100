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

#define TILES_ACROSS	(pie_GetVideoBufferWidth()/TILE_SIZE2D)
#define TILES_DOWN		(pie_GetVideoBufferHeight()/TILE_SIZE2D)

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

