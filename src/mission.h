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
 * Mission defines for the game
 */

#ifndef __INCLUDED_SRC_MISSION_H__
#define __INCLUDED_SRC_MISSION_H__

#include "map.h"
#include "power.h"
#include "levels.h"
#include "missiondef.h"
#include "group.h"

/**
 * The number of areas that can be defined to prevent buildings being placed -
 * used for Transporter Landing Zones 0-7 are for players, 8 = LIMBO_LANDING
 */
#define         MAX_NOGO_AREAS          (MAX_PLAYERS + 1)
#define         LIMBO_LANDING           MAX_PLAYERS

extern MISSION		mission;
extern bool			offWorldKeepLists;
extern DROID       *apsLimboDroids[MAX_PLAYERS];

/** Return positions for vtols. */
extern Vector2i asVTOLReturnPos[MAX_PLAYERS];
extern bool Cheated;

void initMission();
bool missionShutDown();
void missionDestroyObjects();

/** This is called everytime the game is quit. */
void releaseMission();

/** On the PC - sets the countdown played flag. */
void setMissionCountDown();

bool startMission(LEVEL_TYPE missionType, char *pGame);
void endMission();

/** Initialise the mission stuff for a save game. */
bool startMissionSave(SDWORD missionType);

/** Sets up the game to start a new mission. */
bool setUpMission(UDWORD type);

/** This causes the new mission data to be loaded up. */
void launchMission();

/** The update routine for all droids left back at home base. Only interested in Transporters at present. */
void missionDroidUpdate(DROID *psDroid);

bool missionIsOffworld();
bool missionCanReEnforce();
bool missionForReInforcements();

/** Returns true if the mission is a Limbo Expand mission. */
bool missionLimboExpand();

/** This is called mid Limbo mission via the script. */
void resetLimboMission();

void swapMissionPointers();

// mission results.
#define		IDTIMER_FORM			11000
#define		IDTIMER_DISPLAY			11001
#define		IDMISSIONRES_FORM		11002
#define     IDMISSIONRES_QUIT		11007
#define     IDMISSIONRES_SAVE		11006

//timer display for transporter timer
#define		IDTRANTIMER_DISPLAY		11010

// Timer position defines
#define		TIMER_X					(568 + E_W)
#define		TIMER_Y					22

// status of the mission result screens.
extern bool MissionResUp;

void intRemoveMissionResult();
void intRemoveMissionResultNoAnim();
void intProcessMissionResult(UDWORD id);
void intRunMissionResult();

void unloadTransporter(DROID *psTransporter, UDWORD x, UDWORD y, bool goingHome);

/** Sets the appropriate pause states for when the interface is up but the game needs to be paused. */
void setMissionPauseState();

/** Resets the pause states. */
void resetMissionPauseState();

/** Returns the x coord for where the Transporter can land. */
UWORD getLandingX(SDWORD iPlayer);

/** Returns the y coord for where the Transporter can land. */
UWORD getLandingY(SDWORD iPlayer);

/** Checks that the timer has been set and that a Transporter exists before adding the timer button. */
void addTransporterTimerInterface();

void intRemoveTransporterTimer();

/** Update routine for mission details. */
void missionTimerUpdate();

/** Checks the time has been set and then adds the timer if not already on the display. */
void addMissionTimerInterface();

void intRemoveMissionTimer();

//access functions for bPlayCountDown flag
void setPlayCountDown(UBYTE set);
bool getPlayCountDown();

/** Checks the x,y passed in are not within the boundary of the Landing Zone x and y in tile coords. */
bool withinLandingZone(UDWORD x, UDWORD y);

//sets the coords for the Transporter to land
LANDING_ZONE *getLandingZone(SDWORD i);

/** Initialises all the nogo areas to 0. */
void initNoGoAreas();

/** Sets the coords for a no go area. */
void setNoGoArea(UBYTE x1, UBYTE y1, UBYTE x2, UBYTE y2, UBYTE area);

/** Fly in transporters at start of level. */
void missionFlyTransportersIn(SDWORD iPlayer, bool bTrackTransporter);

/** Move transporter offworld. */
void missionMoveTransporterOffWorld(DROID *psTransporter);

/** Pick nearest map edge to point. */
void missionGetNearestCorner(UWORD iX, UWORD iY, UWORD *piOffX, UWORD *piOffY);

void missionSetReinforcementTime(UDWORD iTime);
UDWORD  missionGetReinforcementTime();

/** Builds a droid back at the home base whilst on a mission - stored in a list made available to the transporter interface. */
DROID *buildMissionDroid(DROID_TEMPLATE *psTempl, UDWORD x, UDWORD y, UDWORD player);

/** This is just a very big number - bigger than a map width/height could ever be! */
#define		INVALID_XY				(512 * 127)

void missionSetTransporterEntry(SDWORD iPlayer, SDWORD iEntryTileX, SDWORD iEntryTileY);
void missionSetTransporterExit(SDWORD iPlayer, SDWORD iExitTileX, SDWORD iExitTileY);
void missionGetTransporterEntry(SDWORD iPlayer, UWORD *iX, UWORD *iY);
void missionGetTransporterExit(SDWORD iPlayer, UDWORD *iX, UDWORD *iY);

//access functions for droidsToSafety flag
void setDroidsToSafetyFlag(bool set);
bool getDroidsToSafetyFlag();

/** Checks to see if the player has any droids (except Transporters left). */
bool missionDroidsRemaining(UDWORD player);

/**
 * Called when a Transporter gets to the edge of the world and the droids are being flown to safety.
 * The droids inside the Transporter are placed into the mission list for later use.
 */
void moveDroidsToSafety(DROID *psTransporter);

/** Called when ESC is pressed. */
void clearMissionWidgets();

/** Resets if return to game after an ESC. */
void resetMissionWidgets();

UDWORD	getCampaignNumber();
void	setCampaignNumber(UDWORD number);
bool intAddMissionResult(bool result, bool bPlaySuccess);

/** Reset the vtol landing pos. */
void resetVTOLLandingPos();

/** This is called via a script function to place the Limbo droids once the mission has started. */
void placeLimboDroids();

/** bCheating = true == start of cheat, bCheating = false == end of cheat. */
void setMissionCheatTime(bool bCheating);


#define		MISSIONRES_X			20	// pos & size of box.
#define		MISSIONRES_Y			380
#define		MISSIONRES_W			600
#define		MISSIONRES_H			80

#define		MISSIONRES_TITLE_X		20
#define		MISSIONRES_TITLE_Y		20
#define		MISSIONRES_TITLE_W		600
#define		MISSIONRES_TITLE_H		40

#endif // __INCLUDED_SRC_MISSION_H__
