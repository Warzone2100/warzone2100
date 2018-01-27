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

#ifndef __INCLUDED_SRC_LOADSAVE_H__
#define __INCLUDED_SRC_LOADSAVE_H__

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

enum LOADSAVE_MODE
{
	LOAD_FRONTEND_MISSION,
	LOAD_MISSIONEND,
	LOAD_INGAME_MISSION,
	LOAD_FRONTEND_SKIRMISH,
	LOAD_INGAME_SKIRMISH,
	SAVE_MISSIONEND,
	SAVE_INGAME_MISSION,
	SAVE_INGAME_SKIRMISH
};

/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

extern bool		bLoadSaveUp;							// true when interface is up and should be run.
//the name of the save game to load from the front end
extern char saveGameName[256];
extern char	sRequestResult[PATH_MAX];
extern bool		bRequestLoad;

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/

void drawBlueBox(UDWORD x, UDWORD y, UDWORD w, UDWORD h);

bool addLoadSave(LOADSAVE_MODE mode, const char *title);
bool closeLoadSave();
bool runLoadSave(bool bResetMissionWidgets);
bool displayLoadSave();

void removeWildcards(char *pStr);

// return whether the save screen was displayed in the mission results screen
bool saveInMissionRes();

// return whether the save screen was displayed in the middle of a mission
bool saveMidMission();

void deleteSaveGame(char *saveGameName);

void loadSaveScreenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight);

#endif // __INCLUDED_SRC_LOADSAVE_H__
