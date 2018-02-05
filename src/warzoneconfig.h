/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
/** @file
 *  Warzone Global configuration functions.
 */

#ifndef __INCLUDED_SRC_WARZONECONFIG_H__
#define __INCLUDED_SRC_WARZONECONFIG_H__

#include "lib/framework/frame.h"
#include "lib/sequence/sequence.h"

#define	CAMERASPEED_MAX		(5000)
#define	CAMERASPEED_MIN		(100)
#define	CAMERASPEED_DEFAULT	(2500)
#define	CAMERASPEED_STEP	(100)

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/
enum FMV_MODE
{
	FMV_FULLSCREEN,
	FMV_1X,
	FMV_2X,
	FMV_MAX
};

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
void war_SetDefaultStates();
void war_SetFMVmode(FMV_MODE mode);
FMV_MODE war_GetFMVmode();
void war_SetAllowSubtitles(bool);
bool war_GetAllowSubtitles();
void war_setFullscreen(bool);
bool war_getFullscreen();
void war_setAntialiasing(int);
int war_getAntialiasing();
void war_SetTrapCursor(bool b);
bool war_GetTrapCursor();
bool war_GetColouredCursor();
void war_SetColouredCursor(bool enabled);
void war_SetVsync(bool b);
bool war_GetVsync();
void war_SetDisplayScale(unsigned int scale);
unsigned int war_GetDisplayScale();
void war_SetWidth(UDWORD width);
UDWORD war_GetWidth();
void war_SetScreen(int screen);
int war_GetScreen();
void war_SetHeight(UDWORD height);
UDWORD war_GetHeight();
void war_SetPauseOnFocusLoss(bool enabled);
bool war_GetPauseOnFocusLoss();
bool war_GetMusicEnabled();
void war_SetMusicEnabled(bool enabled);
int war_GetMapZoom();
void war_SetMapZoom(int mapZoom);
int war_GetMapZoomRate();
void war_SetMapZoomRate(int mapZoomRate);
int war_GetRadarZoom();
void war_SetRadarZoom(int radarZoom);
bool war_GetRadarJump();
void war_SetRadarJump(bool radarJump);
int war_GetCameraSpeed();
void war_SetCameraSpeed(int cameraSpeed);
int war_GetScrollEvent();
void war_SetScrollEvent(int scrollEvent);
int8_t war_GetSPcolor();
void war_SetSPcolor(int color);
void war_setMPcolour(int colour);
int war_getMPcolour();
void war_setScanlineMode(SCANLINE_MODE mode);
SCANLINE_MODE war_getScanlineMode();

/**
 * Enable or disable sound initialization
 * Has no effect after systemInitialize()!
 *
 * \param	soundEnabled	enable sound (or not)
 */
void war_setSoundEnabled(bool soundEnabled);

/**
 * Whether we should initialize sound or not
 *
 * \return	Enable sound (or not)
 */
bool war_getSoundEnabled();

#endif // __INCLUDED_SRC_WARZONECONFIG_H__
