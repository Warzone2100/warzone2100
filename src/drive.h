/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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

extern BOOL DirectControl;
extern DROID *psDrivenDroid;

static inline BOOL driveHasDriven(void)
{
	return (DirectControl) && (psDrivenDroid != NULL) ? true : false;
}


// Returns true if drive mode is active.
//
static inline BOOL driveModeActive(void)
{
	return DirectControl;
}


// Return true if the specified droid is the driven droid.
//
static inline BOOL driveIsDriven(DROID *psDroid)
{
	return (DirectControl) && (psDrivenDroid != NULL) && (psDroid == psDrivenDroid) ? true : false;
}


static inline BOOL driveIsFollower(DROID *psDroid)
{
	return (DirectControl) && (psDrivenDroid != NULL) && (psDroid != psDrivenDroid) && psDroid->selected ? true : false;
}


static inline DROID *driveGetDriven(void)
{
	return psDrivenDroid;
}


void driveInitVars(BOOL Restart);
BOOL StartDriverMode(DROID *psOldDroid);
void StopDriverMode(void);
BOOL driveDroidKilled(DROID *psDroid);
void driveSelectionChanged(void);
void driveUpdate(void);
void driveSetDroidMove(DROID *psDroid);
void setDrivingStatus( BOOL val );
BOOL getDrivingStatus( void );
void driveDisableControl(void);
void driveEnableControl(void);
void driveEnableInterface(BOOL AddReticule);
void driveDisableInterface(void);
BOOL driveInterfaceEnabled(void);
BOOL driveControlEnabled(void);
void driveProcessInterfaceButtons(void);
void driveAutoToggle(void);
void driveProcessAquireButton(void);
void driveProcessAquireTarget(void);
void driveMarkTarget(void);
void driveStartBuild(void);
BOOL driveAllowControl(void);
void driveDisableTactical(void);
BOOL driveTacticalActive(void);
void driveTacticalSelectionChanged(void);
void driveProcessRadarInput(int x,int y);
BOOL driveWasDriving(void);
void driveDisableDriving(void);
void driveRestoreDriving(void);
SDWORD driveGetMoveSpeed(void);
SDWORD driveGetMoveDir(void);
BOOL driveSetDirectControl(BOOL Control);
BOOL driveSetWasDriving(BOOL Driving);

#endif // __INCLUDED_SRC_DRIVE_H__
