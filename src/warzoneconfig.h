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
/** @file
 *  Warzone Global configuration functions.
 */

#ifndef __INCLUDED_SRC_WARZONECONFIG_H__
#define __INCLUDED_SRC_WARZONECONFIG_H__

#include "lib/framework/frame.h"
#include "lib/sequence/sequence.h"

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

enum FSAA_LEVEL
{
	FSAA_OFF,
	FSAA_2X,
	FSAA_4X,
	FSAA_8X,
	FSAA_MAX
};

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
extern void	war_SetDefaultStates(void);
extern void war_SetFMVmode(FMV_MODE mode);
extern FMV_MODE war_GetFMVmode(void);
extern void war_SetAllowSubtitles(bool);
extern bool war_GetAllowSubtitles(void);
extern void war_setFullscreen(bool);
extern bool war_getFullscreen(void);
extern void war_setFSAA(unsigned int);
extern unsigned int war_getFSAA(void);
extern void war_SetTrapCursor(bool b);
extern bool war_GetTrapCursor(void);
extern bool war_GetColouredCursor(void);
extern void war_SetColouredCursor(bool enabled);
extern void war_SetVsync(bool b);
extern bool war_GetVsync(void);
extern void war_SetWidth(UDWORD width);
extern UDWORD war_GetWidth(void);
void war_SetScreen(int screen);
int war_GetScreen();
extern void war_SetHeight(UDWORD height);
extern UDWORD war_GetHeight(void);
extern void war_SetPauseOnFocusLoss(bool enabled);
extern bool war_GetPauseOnFocusLoss(void);
extern bool war_GetMusicEnabled(void);
extern void war_SetMusicEnabled(bool enabled);
extern int8_t war_GetSPcolor(void);
extern void war_SetSPcolor(int color);
void war_setMPcolour(int colour);
int war_getMPcolour();
void war_setScanlineMode(SCANLINE_MODE mode);
SCANLINE_MODE war_getScanlineMode(void);

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
bool war_getSoundEnabled(void);

#endif // __INCLUDED_SRC_WARZONECONFIG_H__
