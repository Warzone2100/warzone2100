/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

/**
 *      @file radar.h
 *      Minimap code.
 *      @defgroup Minimap Minimap (radar) subsystem.
 *      @{
 */

#ifndef __INCLUDED_SRC_RADAR_H__
#define __INCLUDED_SRC_RADAR_H__

void radarColour(UDWORD tileNumber, uint8_t r, uint8_t g, uint8_t b);	///< Set radar colour for given terrain type.

#define MAX_RADARZOOM		(16 * 4)    // 3.00x
#define MIN_RADARZOOM		(16 * 2/4)  // 0.75x
#define DEFAULT_RADARZOOM	(16)        // 1.00x
#define RADARZOOM_STEP		(16 * 1/4)  // 0.25x

extern bool InitRadar(void);				///< Initialize minimap subsystem.
extern bool ShutdownRadar(void);			///< Shutdown minimap subsystem.
extern bool resizeRadar(void);				///< Recalculate minimap size. For initialization code only.
extern void drawRadar(void);				///< Draw the minimap on the screen.
extern void CalcRadarPosition(int mX, int mY, int *PosX, int *PosY);	///< Given a position within the radar, returns a world coordinate.
extern void SetRadarZoom(uint8_t ZoomLevel);		///< Set current zoom level. 1.0 is 1:1 resolution.
extern uint8_t GetRadarZoom(void);			///< Get current zoom level.
extern bool CoordInRadar(int x, int y);			///< Is screen coordinate inside minimap?

/** Different mini-map draw modes. */
enum RADAR_DRAW_MODE
{
	RADAR_MODE_TERRAIN,				///< Draw terrain map
	RADAR_MODE_DEFAULT = RADAR_MODE_TERRAIN,	///< Default is terrain map
	RADAR_MODE_HEIGHT_MAP,				///< Draw height map
	RADAR_MODE_COMBINED,
	RADAR_MODE_NO_TERRAIN,				///< Only display objects
	NUM_RADAR_MODES
};

extern bool		bEnemyAllyRadarColor;		///< Enemy/ally minimap color
extern RADAR_DRAW_MODE	radarDrawMode;			///< Current minimap mode
extern bool rotateRadar;

extern void radarInitVars(void);			///< Recalculate minimap variables. For initialization code only.
extern PIELIGHT clanColours[];

/** @} */

#endif // __INCLUDED_SRC_RADAR_H__
