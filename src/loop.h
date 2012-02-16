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
/** @file
 * Interface to the main game loop routine.
 */

#ifndef __INCLUDED_SRC_LOOP_H__
#define __INCLUDED_SRC_LOOP_H__

#include "lib/framework/frame.h"

enum GAMECODE
{
	GAMECODE_CONTINUE,
	GAMECODE_RESTARTGAME,
	GAMECODE_QUITGAME,
	GAMECODE_PLAYVIDEO,
	GAMECODE_NEWLEVEL,
	GAMECODE_FASTEXIT,
	GAMECODE_LOADGAME,
};

// the states the loop goes through before starting a new level
enum LOOP_MISSION_STATE
{
	LMS_NORMAL,			// normal state of the loop
	LMS_SETUPMISSION,	// make the call to set up mission
	LMS_SAVECONTINUE,	// the save/continue box is up between missions
	LMS_NEWLEVEL,		// start a new level
	LMS_LOADGAME,		// load a savegame
	LMS_CLEAROBJECTS,	// make the call to destroy objects
};
extern LOOP_MISSION_STATE		loopMissionState;

// this is set by scrStartMission to say what type of new level is to be started
extern SDWORD	nextMissionType;

extern unsigned int loopPieCount;
extern unsigned int loopPolyCount;
extern unsigned int loopStateChanges;

extern GAMECODE gameLoop(void);
extern void videoLoop(void);
extern void loop_SetVideoPlaybackMode(void);
extern void loop_ClearVideoPlaybackMode(void);
extern bool loop_GetVideoStatus(void);
extern SDWORD loop_GetVideoMode(void);
extern bool	gamePaused( void );
extern void	setGamePauseStatus( bool val );
extern void loopFastExit(void);

extern bool gameUpdatePaused(void);
extern bool audioPaused(void);
extern bool scriptPaused(void);
extern bool scrollPaused(void);
extern bool consolePaused(void);
extern bool editPaused(void);

extern void setGameUpdatePause(bool state);
extern void setEditPause(bool state);
extern void setAudioPause(bool state);
extern void setScriptPause(bool state);
extern void setScrollPause(bool state);
extern void setConsolePause(bool state);
//set all the pause states to the state value
extern void setAllPauseStates(bool state);

// Number of units in the current list.
extern UDWORD	getNumDroids(UDWORD	player);
// Number of units on transporters.
extern UDWORD	getNumTransporterDroids(UDWORD player);
// Number of units in the mission list.
extern UDWORD	getNumMissionDroids(UDWORD player);
UDWORD	getNumCommandDroids(UDWORD player);
UDWORD	getNumConstructorDroids(UDWORD player);
// increase the droid counts - used by update factory to keep the counts in sync
void incNumDroids(UDWORD player);
void incNumCommandDroids(UDWORD player);
void incNumConstructorDroids(UDWORD player);

#endif // __INCLUDED_SRC_LOOP_H__
