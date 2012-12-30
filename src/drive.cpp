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
 * @file drive.c
 * Routines for player driving units about the map.
 */

#define DEFINE_DRIVE_INLINE

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/input.h"

#include "lib/gamelib/gtime.h"
#include "lib/gamelib/animobj.h"
#include "lib/sound/audio.h"

#include "drive.h"
#include "objects.h"
#include "move.h"
#include "visibility.h"
#include "map.h"
#include "loop.h"
#include "geometry.h"
#include "anim_id.h"
#include "action.h"
#include "order.h"
#include "combat.h"
#include "mapgrid.h"
#include "display.h"
#include "effects.h"
#include "hci.h"
#include "warcam.h"
#include "radar.h"
#include "display3ddef.h"
#include "display3d.h"
#include "console.h"
#include "intdisplay.h"
#include "multiplay.h"

// all the bollox needed for script callbacks
#include "lib/script/parse.h"				// needed to define types in scripttabs.h (Arse!)
#include "scripttabs.h"			// needed to define the callback
#include "scriptextern.h"		// needed to include the GLOBAL for checking bInTutorial
#include "group.h"

#define DRIVENOISE		// Enable driving noises.
#define ENGINEVOL (0xfff)
#define MINPITCH (768)
#define PITCHCHANGE (512)

static	SWORD DrivingAudioTrack=-1;		// Which hardware channel are we using for the car driving noise

extern UDWORD selectedPlayer;
static bool DirectControl = false;

// Driving characteristics.
#define DRIVE_TURNSPEED	(4)
#define DRIVE_ACCELERATE (16)
#define DRIVE_BRAKE (32)
#define DRIVE_DECELERATE (16)
#define	FOLLOW_STOP_RANGE (256)		// How close followers need to get to the driver before they can stop.
#define DRIVE_TURNDAMP	(32)	// Damping value for analogue turning.
#define MAX_IDLE	(GAME_TICKS_PER_SEC*60)	// Start to orbit if idle for 60 seconds.

enum CONTROLMODE_TYPE
{
	CONTROLMODE_POINTNCLICK,
	CONTROLMODE_DRIVE,
};

static DROID *psDrivenDroid = NULL;		// The droid that's being driven.
static bool bDriveMode = false;
static SDWORD driveDir;					// Driven droid's direction.
static SDWORD driveSpeed;				// Driven droid's speed.
static UDWORD driveBumpTime;				// Time that followers get a kick up the ass.
static bool	DoFollowRangeCheck = true;
static bool AllInRange = true;
static bool	ClearFollowRangeCheck = false;
static bool DriveControlEnabled = false;
static bool DriveInterfaceEnabled = false;
static UDWORD IdleTime;
static bool TacticalActive = false;
static bool WasDriving = false;
static CONTROLMODE_TYPE ControlMode = CONTROLMODE_DRIVE;
static bool TargetFeatures = false;

bool driveHasDriven()
{
	return (DirectControl) && (psDrivenDroid != NULL) ? true : false;
}

bool driveModeActive()
{
	return DirectControl;
}

bool driveIsDriven(DROID *psDroid)
{
	return (DirectControl) && (psDrivenDroid != NULL) && (psDroid == psDrivenDroid) ? true : false;
}

bool driveIsFollower(DROID *psDroid)
{
	return (DirectControl) && (psDrivenDroid != NULL) && (psDroid != psDrivenDroid) && psDroid->selected ? true : false;
}

DROID *driveGetDriven(void)
{
	return psDrivenDroid;
}

// Intialise drive statics, call with true if coming from frontend, false if
// coming from a mission.
//
void driveInitVars(bool Restart)
{
	if(WasDriving && !Restart)
	{
		debug( LOG_NEVER, "driveInitVars: WasDriving\n" );
		DrivingAudioTrack=-1;
		psDrivenDroid = NULL;
		DoFollowRangeCheck = true;
		ClearFollowRangeCheck = false;
		bDriveMode = false;
		DriveControlEnabled = true;	//false;
		DriveInterfaceEnabled = false;	//true;
		TacticalActive = false;
		ControlMode = CONTROLMODE_DRIVE;
		TargetFeatures = false;

	}
	else
	{
		debug( LOG_NEVER, "driveInitVars: Driving\n" );
		DrivingAudioTrack=-1;
		psDrivenDroid = NULL;
		DoFollowRangeCheck = true;
		ClearFollowRangeCheck = false;
		bDriveMode = false;
		DriveControlEnabled = true;	//false;
		DriveInterfaceEnabled = false;
		TacticalActive = false;
		ControlMode = CONTROLMODE_DRIVE;
		TargetFeatures = false;
		WasDriving = false;

	}
}


void	setDrivingStatus( bool val )
{
	bDriveMode = val;
}

bool	getDrivingStatus( void )
{
	return(bDriveMode);
}


// Start droid driving mode.
//
bool StartDriverMode(DROID *psOldDroid)
{
	DROID *psLastDriven;

	IdleTime = gameTime;

	psLastDriven = psDrivenDroid;
	psDrivenDroid = NULL;

	// Find a selected droid and make that the driven one.
	for(DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if(psDroid->selected)
		{
			if ((psDrivenDroid == NULL) && (psDroid != psOldDroid))
			{
				// The first droid found becomes the driven droid.
				if (!(DroidIsBuilding(psDroid) || DroidGoingToBuild(psDroid)))
				{
					//					psDroid->sMove.Status = MOVEDRIVE;
				}
				psDrivenDroid = psDroid;
				debug( LOG_NEVER, "New driven droid" );
			}
		}
	}

	// If that failed then find any droid and make it the driven one.
	if (psDrivenDroid == NULL)
	{
		psLastDriven = NULL;
		psDrivenDroid = intGotoNextDroidType(NULL,DROID_ANY,true);

		// If it's the same droid then try again
		if(psDrivenDroid == psOldDroid)
		{
			psDrivenDroid = intGotoNextDroidType(NULL,DROID_ANY,true);
		}

		if(psDrivenDroid == psOldDroid)
		{
			psDrivenDroid = NULL;
		}

		// If it failed then try for a transporter.
		if(psDrivenDroid == NULL)
		{
			// FIXME: for DROID_SUPER_TRANSPORTER
			psDrivenDroid = intGotoNextDroidType(NULL, DROID_TRANSPORTER, true);
		}

		//		DBPRINTF(("Selected a new driven droid : %p\n",psDrivenDroid));
	}

	if(psDrivenDroid)
	{
		driveDir = UNDEG(psDrivenDroid->rot.direction);
		driveSpeed = 0;
		driveBumpTime = gameTime;

		setDrivingStatus(true);

		if(DriveInterfaceEnabled)
		{
			debug( LOG_NEVER, "Interface enabled1 ! Disabling drive control" );
			DriveControlEnabled = false;
			DirectControl = false;
		}
		else
		{
			DriveControlEnabled = true;
			DirectControl = true; // we are taking over the unit.
		}

		if(psLastDriven != psDrivenDroid)
		{
			debug( LOG_NEVER, "camAllignWithTarget" );
			camAllignWithTarget((BASE_OBJECT*)psDrivenDroid);
		}

		return true;
	}

	return false;
}

static void ChangeDriver(void)
{
	if(psDrivenDroid != NULL)
	{
		debug( LOG_NEVER, "Driver Changed\n" );

		for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
		{
			if (psDroid->sMove.Status == MOVEDRIVE)
			{
				ASSERT((psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER), "Tried to control a transporter" );
				psDroid->sMove.Status = MOVEINACTIVE;
			}
		}
	}
}

// Stop droid driving mode.
//
void StopDriverMode(void)
{
	if(psDrivenDroid != NULL)
	{
		debug( LOG_NEVER, "Drive mode canceled" );
		addConsoleMessage("Driver mode canceled.", LEFT_JUSTIFY,SYSTEM_MESSAGE);

		psDrivenDroid = NULL;

		for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
		{
			if (psDroid->sMove.Status == MOVEDRIVE)
			{
				ASSERT((psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER), "Tried to control a transporter");
				psDroid->sMove.Status = MOVEINACTIVE;
			}
		}
	}

	setDrivingStatus(false);
	driveInitVars(false);	// reset everything again
	DriveControlEnabled = false;
	DirectControl = false;
}


// Call this whenever a droid gets killed or removed.
// returns true if ok, returns false if resulted in driving mode being stopped, ie could'nt find
// a selected droid to drive.
//
bool driveDroidKilled(DROID *psDroid)
{
	if(driveModeActive()) {
		if(psDroid == psDrivenDroid) {
			ChangeDriver();

			psDrivenDroid = NULL;
			DeSelectDroid(psDroid);

			if(!StartDriverMode(psDroid))
			{
				return false;
			}
		}
	}

	return true;
}


// Call whenever the selections change to re initialise drive mode.
//
void driveSelectionChanged(void)
{
	if(driveModeActive()) {
		if(psDrivenDroid) {
			ChangeDriver();
			StartDriverMode(NULL);
		}
	}
}


// Cycle to next droid in group and make it the driver.
//
static void driveNextDriver(void)
{
	DROID *psDroid;

	// Start from the current driven droid.
	for(psDroid = psDrivenDroid; psDroid; psDroid = psDroid->psNext) {
		if( (psDroid->selected) && (psDroid != psDrivenDroid) ) {
			// Found one so make it the driven droid.
			psDrivenDroid = psDroid;
			camAllignWithTarget((BASE_OBJECT*)psDroid);
			return;
		}
	}

		for(psDroid = apsDroidLists[selectedPlayer];
			psDroid && (psDroid != psDrivenDroid);
			psDroid = psDroid->psNext) {

			if(psDroid->selected) {
				// Found one so make it the driven droid.
				psDrivenDroid = psDroid;
				camAllignWithTarget((BASE_OBJECT*)psDroid);
				return;
			}
		}
	// No other selected droids found. Oh well...
}


static bool driveControl(DROID *psDroid)
{
	bool Input = false;
	SDWORD MaxSpeed = moveCalcDroidSpeed(psDroid);

	if(!DriveControlEnabled) {
		return false;
	}

	if(keyPressed(KEY_N)) {
		driveNextDriver();
	}

	if(keyDown(KEY_LEFTARROW)) {
		driveDir += DRIVE_TURNSPEED;
		Input = true;
	} else if(keyDown(KEY_RIGHTARROW)) {
		driveDir -= DRIVE_TURNSPEED;
		if(driveDir < 0) {
			driveDir += 360;
		}
		Input = true;
	}

	driveDir = driveDir % 360;

	if(keyDown(KEY_UPARROW)) {
		if(driveSpeed >= 0) {
			driveSpeed += DRIVE_ACCELERATE;
			if(driveSpeed > MaxSpeed) {
				driveSpeed = MaxSpeed;
			}
		} else {
			driveSpeed += DRIVE_BRAKE;
			if(driveSpeed > 0) {
				driveSpeed = 0;
			}
		}
		Input = true;
	} else if(keyDown(KEY_DOWNARROW)) {
		if(driveSpeed <= 0) {
			driveSpeed -= DRIVE_ACCELERATE;
			if(driveSpeed < -MaxSpeed) {
				driveSpeed = -MaxSpeed;
			}
		} else {
			driveSpeed -= DRIVE_BRAKE;
			if(driveSpeed < 0) {
				driveSpeed = 0;
			}
		}
		Input = true;
	} else {
		if(driveSpeed > 0) {
			driveSpeed -= DRIVE_DECELERATE;
			if(driveSpeed < 0) {
				driveSpeed = 0;
			}
		} else {
			driveSpeed += DRIVE_DECELERATE;
			if(driveSpeed > 0) {
				driveSpeed = 0;
			}
		}
	}

	return Input;
}


static bool driveInDriverRange(DROID *psDroid)
{
	if( (abs(psDroid->pos.x-psDrivenDroid->pos.x) < FOLLOW_STOP_RANGE) &&
		(abs(psDroid->pos.y-psDrivenDroid->pos.y) < FOLLOW_STOP_RANGE) ) {
		return true;
	}

	return false;
}


static void driveMoveFollower(DROID *psDroid)
{
	if(driveBumpTime < gameTime) {
		// Update the driven droid's followers.
		if(!driveInDriverRange(psDroid)) {

			//psDroid->secondaryOrder&=~DSS_MOVEHOLD_SET;		// Remove secondary order ... this stops the droid from jumping back to GUARD mode ... see order.c #111 - tjc
			// if the droid is currently guarding we need to change the order to a move
			if (psDroid->order.type == DORDER_GUARD)
			{
				orderDroidLoc(psDroid, DORDER_MOVE, psDrivenDroid->pos.x, psDrivenDroid->pos.y, ModeQueue);
			}
			else
			{
				// otherwise we just adjust its move-to location
				moveDroidTo(psDroid,psDrivenDroid->pos.x,psDrivenDroid->pos.y);
			}

		}
	}

	// Stop it when it gets within range of the driver.
	if(DoFollowRangeCheck) {
		if(driveInDriverRange(psDroid)) {
			psDroid->sMove.Status = MOVEINACTIVE;
		} else {
			AllInRange = false;
		}
	}
}


static void driveMoveCommandFollowers(DROID *psDroid)
{
	DROID_GROUP *psGroup = psDroid->psGroup;

	for (DROID *psCurr = psGroup->psList; psCurr!=NULL; psCurr=psCurr->psGrpNext)
	{
		driveMoveFollower(psCurr);
	}
}


// Call once per frame.
//
void driveUpdate(void)
{
	DROID *psDroid;
	PROPULSION_STATS *psPropStats;

	AllInRange = true;

	if (!DirectControl)
	{
		return;
	}

		if (psDrivenDroid != NULL)
		{
			if (bMultiMessages && (driveBumpTime < gameTime))	// send latest info about driven droid.
			{
				sendDroidInfo(psDrivenDroid, DroidOrder(DORDER_MOVE, removeZ(psDrivenDroid->pos)), false);
			}

			// Check the driven droid is still selected
			if (psDrivenDroid->selected == false)
			{
				// if it's not then reset the driving system.
				driveSelectionChanged();
				return;
			}

			// Update the driven droid.
			if (driveControl(psDrivenDroid))
			{
				// If control did something then force the droid's move status.
				if (psDrivenDroid->sMove.Status != MOVEDRIVE)
				{
					psDrivenDroid->sMove.Status = MOVEDRIVE;
					ASSERT((psDrivenDroid->droidType != DROID_TRANSPORTER && psDrivenDroid->droidType != DROID_SUPERTRANSPORTER), "Tried to control a transporter");
					driveDir = UNDEG(psDrivenDroid->rot.direction);
				}
				DoFollowRangeCheck = true;
			}

			// Is the driven droid under user control?
			if (psDrivenDroid->sMove.Status == MOVEDRIVE)
			{
				// Is it a command droid
				if ((psDrivenDroid->droidType == DROID_COMMAND) &&
					(psDrivenDroid->psGroup != NULL))
				{
					driveMoveCommandFollowers(psDrivenDroid);
				}

				for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
				{
					psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;

					if ((psDroid->selected) &&
						(psDroid != psDrivenDroid) &&
						(psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER) &&
                        ((psPropStats->propulsionType != PROPULSION_TYPE_LIFT) || cyborgDroid(psDroid)) )
                    {
						// Send new orders to it's followers.
						driveMoveFollower(psDroid);
					}
				}
			}

			if (AllInRange)
			{
				DoFollowRangeCheck = false;
			}
			if(driveBumpTime < gameTime)
			{
				// Send next order in 1 second.
				driveBumpTime = gameTime+GAME_TICKS_PER_SEC;
			}
		}
		else
		{
			if (StartDriverMode(NULL) == false)
			{
				// nothing
			}
		}
}


SDWORD driveGetMoveSpeed(void)
{
	return driveSpeed;
}


SDWORD driveGetMoveDir(void)
{
	return driveDir;
}


void driveSetDroidMove(DROID *psDroid)
{
	psDroid->rot.direction = DEG(driveDir);
}


// Dissable user control of droid, ie when interface is up.

void driveDisableControl(void)
{
	DriveControlEnabled = false;
	DirectControl = false;
	DriveInterfaceEnabled = true;
}


// Enable user control of droid, ie when interface is down.
//
void driveEnableControl(void)
{
	DriveControlEnabled = true;
	DirectControl = true;
	DriveInterfaceEnabled = false;
}


// Return true if drive control is enabled.
//
bool driveControlEnabled(void)
{
	return DriveControlEnabled;
}


// Bring up the reticule.
//
void driveEnableInterface(bool AddReticule)
{
	if(AddReticule) {
		intAddReticule();
	}

	DriveInterfaceEnabled = true;
}


// Get rid of the reticule.
//
void driveDisableInterface(void)
{
	intResetScreen(false);
	intRemoveReticule();

	DriveInterfaceEnabled = false;
}


// Return true if the reticule is up.
//
bool driveInterfaceEnabled(void)
{
	return DriveInterfaceEnabled;
}


// Check for and process a user request for a new target.
//
void driveProcessAquireButton(void)
{
	if(mouseReleased(MOUSE_RMB) || keyPressed(KEY_S))
	{
//		BASE_OBJECT	*psObj;
//		psObj = targetAquireNearestObjView((BASE_OBJECT*)psDrivenDroid);
//		driveMarkTarget();
//		wzSetCursor(CURSOR_ATTACK);
	}
}


// Start structure placement for drive mode.
//
void driveStartBuild(void)
{
	intRemoveReticule();
	DriveInterfaceEnabled = false;
	driveEnableControl();
}


// Return true if all the conditions for allowing user control of the droid are met.
//
bool driveAllowControl(void)
{
	if (TacticalActive || DriveInterfaceEnabled || !DriveControlEnabled)
	{
		return false;
	}
	return true;
}


// Disable Tactical order mode.
//
void driveDisableTactical(void)
{
	if(driveModeActive() && TacticalActive)
	{
		TacticalActive = false;
	}
}


// Return true if Tactical order mode is active.
//
bool driveTacticalActive(void)
{
	return TacticalActive;
}


// Player clicked in the radar window.
//
void driveProcessRadarInput(int x,int y)
{
	int PosX, PosY;

	// when drive mode is active, clicking on the radar orders all selected droids
	// to move to this position.
	CalcRadarPosition(x, y, &PosX, &PosY);
	orderSelectedLoc(selectedPlayer, PosX*TILE_UNITS, PosY*TILE_UNITS, ctrlShiftDown());  // ctrlShiftDown() = ctrl clicked a destination, add an order
}
