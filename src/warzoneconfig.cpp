/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
/*
 *	Global Variables
 */
/***************************************************************************/


/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/

struct WARZONE_GLOBALS
{
	FMV_MODE	FMVmode;
	SWORD		effectsLevel;
	UDWORD		width;
	UDWORD		height;
	int8_t		SPcolor;
	int			MPcolour;
	FSAA_LEVEL  fsaa;
	RENDER_MODE shaders;
	bool		Fullscreen;
	bool		soundEnabled;
	bool		trapCursor;
	bool		vsync;
	bool		pauseOnFocusLoss;
	bool		ColouredCursor;
	bool		MusicEnabled;
};

/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

static WARZONE_GLOBALS	warGlobs;//STATIC use or write an access function if you need any of this

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/
void war_SetDefaultStates(void)//Sets all states
{
	//set those here and reset in clParse or loadConfig
	war_setFSAA(0);
	war_SetVsync(true);
	war_setSoundEnabled( true );
	war_SetPauseOnFocusLoss(false);
	war_SetMusicEnabled(true);
	war_SetSPcolor(0);		//default color is green
	war_setMPcolour(-1);            // Default color is random.
	war_SetShaders(SHADERS_ON);
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

void war_setFSAA(unsigned int fsaa)
{
	warGlobs.fsaa = (FSAA_LEVEL)(fsaa % FSAA_MAX);
}

unsigned int war_getFSAA()
{
	return warGlobs.fsaa;
}

void war_SetShaders(unsigned shaders)
{
	warGlobs.shaders = (RENDER_MODE)shaders;
}

unsigned war_GetShaders()
{
	return warGlobs.shaders;
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

/***************************************************************************/
/***************************************************************************/
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

void war_setSoundEnabled( bool soundEnabled )
{
	warGlobs.soundEnabled = soundEnabled;
}

bool war_getSoundEnabled( void )
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
