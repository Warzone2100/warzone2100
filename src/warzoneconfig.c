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
/**
 * @file warzoneConfig.c
 *
 * Warzone Global configuration functions.
 */

#include "lib/framework/frame.h"
#include "warzoneconfig.h"
#include "lib/ivis_common/piestate.h"
#include "advvis.h"

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

typedef struct _warzoneGlobals
{
	SEQ_MODE	seqMode;
	BOOL		bFog;
	SWORD		effectsLevel;
	BOOL		allowSubtitles;
	BOOL		playAudioCDs;
	BOOL		Fullscreen;
	BOOL		soundEnabled;
	BOOL		trapCursor;
	UDWORD		width;
	UDWORD		height;
	bool            pauseOnFocusLoss;
} WARZONE_GLOBALS;

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
	war_SetFog(false);
	war_SetPlayAudioCDs(true);
	war_setSoundEnabled( true );
	war_SetPauseOnFocusLoss(true);
}

void war_SetPlayAudioCDs(BOOL b) {
	warGlobs.playAudioCDs = b;
}

BOOL war_GetPlayAudioCDs(void) {
	return warGlobs.playAudioCDs;
}

void war_SetAllowSubtitles(BOOL b) {
	warGlobs.allowSubtitles = b;
}

BOOL war_GetAllowSubtitles(void) {
	return warGlobs.allowSubtitles;
}

void war_setFullscreen(BOOL b) {
	warGlobs.Fullscreen = b;
}

BOOL war_getFullscreen(void) {
	return warGlobs.Fullscreen;
}

void war_SetTrapCursor(BOOL b)
{
	warGlobs.trapCursor = b;
}

BOOL war_GetTrapCursor(void)
{
	return warGlobs.trapCursor;
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
void war_SetFog(BOOL val)
{
	debug(LOG_FOG, "Visual fog turned %s", val ? "ON" : "OFF");

	if (warGlobs.bFog != val)
	{
		warGlobs.bFog = val;
	}
	if (warGlobs.bFog == true)
	{
		setRevealStatus(false);
	}
	else
	{
		PIELIGHT black;

		setRevealStatus(true);
		black.argb = 0;
		black.byte.a = 255;
		pie_SetFogColour(black);
	}
}

BOOL war_GetFog(void)
{
	return  warGlobs.bFog;
}

/***************************************************************************/
/***************************************************************************/
void war_SetSeqMode(SEQ_MODE mode)
{
	warGlobs.seqMode = mode;
}

SEQ_MODE war_GetSeqMode(void)
{
	return  warGlobs.seqMode;
}

void war_SetPauseOnFocusLoss(bool enabled)
{
	warGlobs.pauseOnFocusLoss = enabled;
}

bool war_GetPauseOnFocusLoss()
{
	return warGlobs.pauseOnFocusLoss;
}

void war_setSoundEnabled( BOOL soundEnabled )
{
	warGlobs.soundEnabled = soundEnabled;
}

BOOL war_getSoundEnabled( void )
{
	return warGlobs.soundEnabled;
}
