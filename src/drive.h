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

#ifndef __INCLUDED_SRC_DRIVE_H__
#define __INCLUDED_SRC_DRIVE_H__

#include "droid.h"

bool driveHasDriven();
bool driveModeActive();	// Returns true if drive mode is active.
bool driveIsDriven(DROID *psDroid);	// Return true if the specified droid is the driven droid.
bool driveIsFollower(DROID *psDroid);
DROID *driveGetDriven();

void driveInitVars(bool Restart);
bool StartDriverMode(DROID *psOldDroid);
void StopDriverMode(void);
bool driveDroidKilled(DROID *psDroid);
void driveSelectionChanged(void);
void driveUpdate(void);
void driveSetDroidMove(DROID *psDroid);
void setDrivingStatus( bool val );
bool getDrivingStatus( void );
void driveDisableControl(void);
void driveEnableControl(void);
void driveEnableInterface(bool AddReticule);
void driveDisableInterface(void);
bool driveInterfaceEnabled(void);
bool driveControlEnabled(void);
void driveProcessInterfaceButtons(void);
void driveAutoToggle(void);
void driveProcessAquireButton(void);
void driveProcessAquireTarget(void);
void driveMarkTarget(void);
void driveStartBuild(void);
bool driveAllowControl(void);
void driveDisableTactical(void);
bool driveTacticalActive(void);
void driveProcessRadarInput(int x,int y);
bool driveWasDriving(void);
void driveDisableDriving(void);
void driveRestoreDriving(void);
SDWORD driveGetMoveSpeed(void);
SDWORD driveGetMoveDir(void);
bool driveSetDirectControl(bool Control);
bool driveSetWasDriving(bool Driving);

#endif // __INCLUDED_SRC_DRIVE_H__
