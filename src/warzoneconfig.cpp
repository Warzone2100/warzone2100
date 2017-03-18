/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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

/***************************************************************************/

struct WARZONE_GLOBALS
{
	FMV_MODE FMVmode = FMV_FULLSCREEN;
	UDWORD width = 1024;
	UDWORD height = 768;
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

int8_t war_GetSPcolor(void)
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

bool war_getFullscreen(void)
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

bool war_GetTrapCursor(void)
{
	return warGlobs.trapCursor;
}

void war_SetVsync(bool b)
{
	warGlobs.vsync = b;
}

bool war_GetVsync(void)
{
	return warGlobs.vsync;
}

void war_SetWidth(UDWORD width)
{
	warGlobs.width = width;
}

UDWORD war_GetWidth(void)
{
	return warGlobs.width;
}

void war_SetHeight(UDWORD height)
{
	warGlobs.height = height;
}

UDWORD war_GetHeight(void)
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

FMV_MODE war_GetFMVmode(void)
{
	return  warGlobs.FMVmode;
}

void war_setScanlineMode(SCANLINE_MODE mode)
{
	debug(LOG_VIDEO, "%d", mode);
	seq_setScanlineMode(mode);
}

SCANLINE_MODE war_getScanlineMode(void)
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

bool war_GetColouredCursor(void)
{
	return warGlobs.ColouredCursor;
}

void war_setSoundEnabled(bool soundEnabled)
{
	warGlobs.soundEnabled = soundEnabled;
}

bool war_getSoundEnabled(void)
{
	return warGlobs.soundEnabled;
}

bool war_GetMusicEnabled(void)
{
	return warGlobs.MusicEnabled;
}

void war_SetMusicEnabled(bool enabled)
{
	warGlobs.MusicEnabled = enabled;
}
