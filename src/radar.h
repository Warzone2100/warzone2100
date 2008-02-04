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
#ifndef _radar_h
#define _radar_h

void radarColour(UDWORD tileNumber, uint8_t r, uint8_t g, uint8_t b);

#define RGB_ENTRIES	3
#define MAX_RADARZOOM	1

/* Radar.h */
extern void resetRadarRedraw(void);
extern BOOL InitRadar(void);
extern BOOL ShutdownRadar(void);
extern void	drawRadar(void);
extern void CalcRadarPosition(UDWORD mX,UDWORD mY,UDWORD *PosX,UDWORD *PosY);
extern void SetRadarZoom(UWORD ZoomLevel);
extern UDWORD GetRadarZoom(void);
extern BOOL CoordInRadar(int x,int y);

//different mini-map draw modes
typedef enum _radar_draw_mode
{
	RADAR_MODE_TERRAIN,						//draw texture map
	RADAR_MODE_DEFAULT = RADAR_MODE_TERRAIN,
	RADAR_MODE_HEIGHT_MAP,					//draw height map
	RADAR_MODE_COMBINED,
	RADAR_MODE_NO_TERRAIN,
	NUM_RADAR_MODES
}RADAR_DRAW_MODE;

extern BOOL		bEnemyAllyRadarColor;		//enemy/ally radar color
extern RADAR_DRAW_MODE	radarDrawMode;		//current mini-map mode

#endif
