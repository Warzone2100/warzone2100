/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_FRONTEND_H__
#define __INCLUDED_SRC_FRONTEND_H__

#include "lib/widget/widgbase.h"
#include "levels.h"

// determines which option screen to use. when in GS_TITLE_SCREEN mode.
enum tMode
{
	TITLE,			// 0 intro mode
	SINGLE,			// 1 single player menu
	MULTI,			// 2 multiplayer menu
	OPTIONS,		// 3 options menu
	GAME,			// 4
	TUTORIAL,		// 5  tutorial/fastplay
	TITLE_UNUSED,	// 6
	FORCESELECT,	// 7 MULTIPLAYER, Force design screen
	STARTGAME,		// 8 Fire up the game
	SHOWINTRO,		// 9 reshow the intro
	QUIT,			// 10 leaving game
	LOADSAVEGAME,	// 11 loading a save game
	KEYMAP,			// 12 keymap editor
	GRAPHICS_OPTIONS,       // 13 graphics options menu
	AUDIO_AND_ZOOM_OPTIONS, // 14 audio and zoom options menu
	VIDEO_OPTIONS,          // 15 video options menu
	MOUSE_OPTIONS,          // 16 mouse options menu
	CAMPAIGNS,              // 17 campaign selector
	MUSIC_MANAGER,			// 18 music manager
};

#define MAX_LEVEL_NAME_SIZE	(256)

extern char	aLevelName[MAX_LEVEL_NAME_SIZE + 1];	//256];			// vital! the wrf file to use.

extern bool	bLimiterLoaded;


void changeTitleMode(tMode mode);
bool runSinglePlayerMenu();
bool runCampaignSelector();
bool runMultiPlayerMenu();


bool runTutorialMenu();
void runContinue();
void startTutorialMenu();
void startSinglePlayerMenu();
void startCampaignSelector();
void startMultiPlayerMenu();



/* Tell the frontend when the screen has been resized */
void frontendScreenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight);


void SPinit(LEVEL_TYPE gameType);

#endif // __INCLUDED_SRC_FRONTEND_H__
