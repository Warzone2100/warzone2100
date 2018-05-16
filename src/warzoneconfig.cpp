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
/**
 * @file warzoneconfig.c
 *
 * Warzone Global configuration functions.
 */

#include "lib/framework/frame.h"
#include "warzoneconfig.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piepalette.h"
#include "advvis.h"
#include "component.h"
#include "display.h"
#include "keybind.h"
#include "radar.h"

/***************************************************************************/

struct WARZONE_GLOBALS
{
	FMV_MODE FMVmode = FMV_FULLSCREEN;
	UDWORD width = 1024;
	UDWORD height = 768;
	int displayScale = 100;
	int screen = 0;
	int8_t SPcolor = 0;
	int MPcolour = -1;
	int antialiasing = 0;
	bool Fullscreen = false;
	bool soundEnabled = true;
	bool trapCursor = false;
	bool vsync = true;
	bool pauseOnFocusLoss = true;
	bool ColouredCursor = true;
	bool MusicEnabled = true;
	int mapZoom = STARTDISTANCE;
	int mapZoomRate = MAP_ZOOM_RATE_DEFAULT;
	int radarZoom = DEFAULT_RADARZOOM;
	int cameraSpeed = CAMERASPEED_DEFAULT;
	int scrollEvent = 0; // map/radar zoom
	bool radarJump = false;
};

static WARZONE_GLOBALS warGlobs;

/***************************************************************************/

void war_SetDefaultStates()
{
	warGlobs = WARZONE_GLOBALS();
}

void war_SetSPcolor(int color)
{
	if (color >= 1 && color <= 3)		// only 0,4,5,6,7 are allowed for SP games, AI uses the other colors.
	{
		color = 0;
	}
	warGlobs.SPcolor = color;
	setPlayerColour(0, color);
}

int8_t war_GetSPcolor()
{
	return warGlobs.SPcolor;
}

void war_setMPcolour(int colour)
{
	warGlobs.MPcolour = colour;
}

int war_getMPcolour()
{
	return warGlobs.MPcolour;
}

void war_setFullscreen(bool b)
{
	warGlobs.Fullscreen = b;
}

bool war_getFullscreen()
{
	return warGlobs.Fullscreen;
}

void war_setAntialiasing(int antialiasing)
{
	warGlobs.antialiasing = antialiasing;
}

int war_getAntialiasing()
{
	return warGlobs.antialiasing;
}

void war_SetTrapCursor(bool b)
{
	warGlobs.trapCursor = b;
}

bool war_GetTrapCursor()
{
	return warGlobs.trapCursor;
}

void war_SetVsync(bool b)
{
	warGlobs.vsync = b;
}

bool war_GetVsync()
{
	return warGlobs.vsync;
}

unsigned int war_GetDisplayScale()
{
	return warGlobs.displayScale;
}

void war_SetDisplayScale(unsigned int scale)
{
	warGlobs.displayScale = scale;
}

void war_SetWidth(UDWORD width)
{
	warGlobs.width = width;
}

UDWORD war_GetWidth()
{
	return warGlobs.width;
}

void war_SetHeight(UDWORD height)
{
	warGlobs.height = height;
}

UDWORD war_GetHeight()
{
	return warGlobs.height;
}

void war_SetScreen(int screen)
{
	warGlobs.screen = screen;
}

int war_GetScreen()
{
	return warGlobs.screen;
}

void war_SetFMVmode(FMV_MODE mode)
{
	warGlobs.FMVmode = (FMV_MODE)(mode % FMV_MAX);
}

FMV_MODE war_GetFMVmode()
{
	return  warGlobs.FMVmode;
}

void war_setScanlineMode(SCANLINE_MODE mode)
{
	debug(LOG_VIDEO, "%d", mode);
	seq_setScanlineMode(mode);
}

SCANLINE_MODE war_getScanlineMode()
{
	debug(LOG_VIDEO, "%d", seq_getScanlineMode());
	return seq_getScanlineMode();
}

void war_SetPauseOnFocusLoss(bool enabled)
{
	warGlobs.pauseOnFocusLoss = enabled;
}

bool war_GetPauseOnFocusLoss()
{
	return warGlobs.pauseOnFocusLoss;
}

void war_SetColouredCursor(bool enabled)
{
	warGlobs.ColouredCursor = enabled;
}

bool war_GetColouredCursor()
{
	return warGlobs.ColouredCursor;
}

void war_setSoundEnabled(bool soundEnabled)
{
	warGlobs.soundEnabled = soundEnabled;
}

bool war_getSoundEnabled()
{
	return warGlobs.soundEnabled;
}

bool war_GetMusicEnabled()
{
	return warGlobs.MusicEnabled;
}

void war_SetMusicEnabled(bool enabled)
{
	warGlobs.MusicEnabled = enabled;
}

int war_GetMapZoom()
{
	return warGlobs.mapZoom;
}

void war_SetMapZoom(int mapZoom)
{
        if (mapZoom % MAP_ZOOM_RATE_MIN == 0 && ! (mapZoom < MINDISTANCE || mapZoom > MAXDISTANCE))
	{
	    warGlobs.mapZoom = mapZoom;
	}
}

int war_GetMapZoomRate()
{
	return warGlobs.mapZoomRate;
}

void war_SetMapZoomRate(int mapZoomRate)
{
        if (mapZoomRate % MAP_ZOOM_RATE_STEP == 0 && ! (mapZoomRate < MAP_ZOOM_RATE_MIN || mapZoomRate > MAP_ZOOM_RATE_MAX))
	{
	    warGlobs.mapZoomRate = mapZoomRate;
	}
}

int war_GetRadarZoom()
{
	return warGlobs.radarZoom;
}

void war_SetRadarZoom(int radarZoom)
{
        if (radarZoom % RADARZOOM_STEP == 0 && ! (radarZoom < MIN_RADARZOOM || radarZoom > MAX_RADARZOOM))
	{
	    warGlobs.radarZoom = radarZoom;
	}
}

int war_GetCameraSpeed()
{
	return warGlobs.cameraSpeed;
}

void war_SetCameraSpeed(int cameraSpeed)
{
        if (cameraSpeed % CAMERASPEED_STEP == 0 && ! (cameraSpeed < CAMERASPEED_MIN || cameraSpeed > CAMERASPEED_MAX))
	{
	    warGlobs.cameraSpeed = cameraSpeed;
	}
}

int war_GetScrollEvent()
{
	return warGlobs.scrollEvent;
}

void war_SetScrollEvent(int scrollEvent)
{
	warGlobs.scrollEvent = scrollEvent;
}

bool war_GetRadarJump()
{
	return warGlobs.radarJump;
}

void war_SetRadarJump(bool radarJump)
{
	warGlobs.radarJump = radarJump;
}
