/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_WRAPPERS_H__
#define __INCLUDED_SRC_WRAPPERS_H__

enum TITLECODE
{
	TITLECODE_CONTINUE,
	TITLECODE_STARTGAME,
	TITLECODE_QUITGAME,
	TITLECODE_SHOWINTRO,
	TITLECODE_SAVEGAMELOAD,
};

//used to set the scriptWinLoseVideo variable
#define PLAY_NONE   0
#define PLAY_WIN    1
#define PLAY_LOSE   2

extern int hostlaunch;

bool frontendInitVars();
TITLECODE titleLoop();

void initLoadingScreen(bool drawbdrop);
void closeLoadingScreen();
void loadingScreenCallback();

void startCreditsScreen();

bool displayGameOver(bool success, bool showBackDrop);
void setPlayerHasLost(bool val);
bool testPlayerHasLost();
bool testPlayerHasWon();
void setPlayerHasWon(bool val);
void setScriptWinLoseVideo(UBYTE val);
UBYTE getScriptWinLoseVideo();

#endif // __INCLUDED_SRC_WRAPPERS_H__
