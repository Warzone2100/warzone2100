//
// Drive.c
//
// Routines for player driving units about the map.
//

#define DEFINE_DRIVE_INLINE

#include <stdio.h>
#include <math.h>
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/vid.h"
#include "lib/framework/frame.h"
#include "objects.h"
#include "move.h"
#include "findpath.h"
#include "visibility.h"
#include "map.h"
#include "fpath.h"
#include "loop.h"
#include "lib/gamelib/gtime.h"
#include "lib/sound/audio.h"
#include "audio_id.h"
#include "geometry.h"
#include "lib/gamelib/animobj.h"
#include "anim_id.h"
#include "formationdef.h"
#include "formation.h"
#include "action.h"
#include "order.h"
#include "astar.h"
#include "combat.h"
#include "mapgrid.h"
#include "display.h"	// needed for widgetsOn flag.
#include "effects.h"
#include "lib/framework/fractions.h"
#include "hci.h"
#include "warcam.h"
#include "radar.h"
#include "display3ddef.h"
#include "display3d.h"
#include "console.h"
#include "text.h"
#include "intdisplay.h"

// all the bollox needed for script callbacks
#include "lib/script/interp.h"				// needed to define types in scripttabs.h
#include "lib/script/parse.h"				// needed to define types in scripttabs.h (Arse!)
#include "scripttabs.h"			// needed to define the callback
#include "scriptextern.h"		// needed to include the GLOBAL for checking bInTutorial
#include "group.h"

#define DEBUG_DRIVE_SPEED

#define DRIVEFIXED
#define DRIVENOISE		// Enable driving noises.
#define ENGINEVOL (0xfff)
#define MINPITCH (768)
#define PITCHCHANGE (512)


#include "multiplay.h"




#include "target.h"
#include "drive.h"

static	SWORD DrivingAudioTrack=-1;		// Which hardware channel are we using for the car driving noise

extern UDWORD selectedPlayer;
extern BOOL DirectControl;

// Driving characteristics.
#define DRIVE_TURNSPEED	(4)
#define DRIVE_ACCELERATE (16)
#define DRIVE_BRAKE (32)
#define DRIVE_DECELERATE (16)

#define	FOLLOW_STOP_RANGE (256)		// How close followers need to get to the driver before they can stop.

#define DRIVE_TURNDAMP	(32)	// Damping value for analogue turning.

#define MAX_IDLE	(GAME_TICKS_PER_SEC*60)	// Start to orbit if idle for 60 seconds.

DROID *psDrivenDroid = NULL;		// The droid that's being driven.
static BOOL bDriveMode = FALSE;
static SDWORD driveDir;					// Driven droid's direction.
static SDWORD driveSpeed;				// Driven droid's speed.
static int driveBumpTime;				// Time that followers get a kick up the ass.
static BOOL	DoFollowRangeCheck = TRUE;
static BOOL AllInRange = TRUE;
static BOOL	ClearFollowRangeCheck = FALSE;
static BOOL DriveControlEnabled = FALSE;
static BOOL DriveInterfaceEnabled = FALSE;
static UDWORD IdleTime;
static BOOL TacticalActive = FALSE;
static BOOL WasDriving = FALSE;

enum {
	CONTROLMODE_POINTNCLICK,
	CONTROLMODE_DRIVE,
};

UWORD ControlMode = CONTROLMODE_DRIVE;
BOOL TargetFeatures = FALSE;


// Intialise drive statics, call with TRUE if coming from frontend, FALSE if
// coming from a mission.
//
void driveInitVars(BOOL Restart)
{
	if(WasDriving && !Restart)
	{
		DBPRINTF(("driveInitVars: WasDriving\n"));
		DrivingAudioTrack=-1;
		psDrivenDroid = NULL;
		DoFollowRangeCheck = TRUE;
		ClearFollowRangeCheck = FALSE;
		bDriveMode = FALSE;
		DriveControlEnabled = TRUE;	//FALSE;
		DriveInterfaceEnabled = FALSE;	//TRUE;
		TacticalActive = FALSE;
		ControlMode = CONTROLMODE_DRIVE;
		TargetFeatures = FALSE;

	} else {
		DBPRINTF(("driveInitVars: Driving\n"));
		DrivingAudioTrack=-1;
		psDrivenDroid = NULL;
		DoFollowRangeCheck = TRUE;
		ClearFollowRangeCheck = FALSE;
		bDriveMode = FALSE;
		DriveControlEnabled = TRUE;	//FALSE;
		DriveInterfaceEnabled = FALSE;
		TacticalActive = FALSE;
		ControlMode = CONTROLMODE_DRIVE;
		TargetFeatures = FALSE;
		WasDriving = FALSE;

	}
}


UWORD controlModeGet(void)
{
	return ControlMode;
}


void controlModeSet(UWORD Mode)
{
	ControlMode = Mode;
}


void	setDrivingStatus( BOOL val )
{
	bDriveMode = val;
}

BOOL	getDrivingStatus( void )
{
	return(bDriveMode);
}



// Start droid driving mode.
//
BOOL StartDriverMode(DROID *psOldDroid)
{
	DROID *psDroid;
	DROID *psLastDriven;

//	DBPRINTF(("StartDriveMode\n"));

	IdleTime = gameTime;


	psLastDriven = psDrivenDroid;
	psDrivenDroid = NULL;

	// Find a selected droid and make that the driven one.
	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if(psDroid->selected) {
			if((psDrivenDroid == NULL) && (psDroid != psOldDroid)) {
				// The first droid found becomes the driven droid.
				if(!(DroidIsBuilding(psDroid) || DroidGoingToBuild(psDroid))) {
//					psDroid->sMove.Status = MOVEDRIVE;
				}
				psDrivenDroid = psDroid;
				DBPRINTF(("New driven droid\n"));
			} else if(psDroid != psDrivenDroid) {
				// All the others become followers of the driven droid.
//				psDroid->sMove.Status = MOVEDRIVEFOLLOW;
			}
		}
	}

	// If that failed then find any droid and make it the driven one.
	if(psDrivenDroid == NULL) {
		psLastDriven = NULL;
		psDrivenDroid = intGotoNextDroidType(NULL,DROID_ANY,TRUE);

		// If it's the same droid then try again
		if(psDrivenDroid == psOldDroid) {
			psDrivenDroid = intGotoNextDroidType(NULL,DROID_ANY,TRUE);
		}

		if(psDrivenDroid == psOldDroid) {
			psDrivenDroid = NULL;
		}

		// If it failed then try for a transporter.
		if(psDrivenDroid == NULL) {
			psDrivenDroid = intGotoNextDroidType(NULL,DROID_TRANSPORTER,TRUE);
		}

//		DBPRINTF(("Selected a new driven droid : %p\n",psDrivenDroid));
	}

	if(psDrivenDroid) {

		driveDir = psDrivenDroid->direction % 360;
		driveSpeed = 0;
		driveBumpTime = gameTime;

		setDrivingStatus(TRUE);

		if(DriveInterfaceEnabled) {
			DBPRINTF(("Interface enabled1 ! Disabling drive control\n"));
			DriveControlEnabled = FALSE;
		} else {
			DriveControlEnabled = TRUE;
		}

		if(psLastDriven != psDrivenDroid) {
DBPRINTF(("camAllignWithTarget\n"));
			camAllignWithTarget((BASE_OBJECT*)psDrivenDroid);
		}


		return TRUE;
	} else {

	}

	return FALSE;
}

void StopEngineNoise(void)
{

	DrivingAudioTrack=-1;
}


void ChangeDriver(void)
{
	DROID *psDroid;

	if(psDrivenDroid != NULL) {
		DBPRINTF(("Driver Changed\n"));

//		audio_StopObjTrack(psDrivenDroid,ID_SOUND_SMALL_DROID_RUN);

//		psDrivenDroid = NULL;

		for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext) {
			if( (psDroid->sMove.Status == MOVEDRIVE) ) {
				ASSERT(((psDroid->droidType != DROID_TRANSPORTER),"Tried to control a transporter"));
				secondarySetState(psDroid, DSO_HALTTYPE, DSS_HALT_GUARD);
				psDroid->sMove.Status = MOVEINACTIVE;
			}
		}
	}

//	#ifdef PSX
//	MouseMovement(TRUE);
//	#endif
//	setDrivingStatus(FALSE);
//	DriveControlEnabled = FALSE;
}


// Stop droid driving mode.
//
void StopDriverMode(void)
{
	DROID *psDroid;

	if(psDrivenDroid != NULL) {
		DBPRINTF(("Drive mode canceled\n"));

//		audio_StopObjTrack(psDrivenDroid,ID_SOUND_SMALL_DROID_RUN);

		psDrivenDroid = NULL;

		for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext) {
			if( (psDroid->sMove.Status == MOVEDRIVE) ) {
				ASSERT(((psDroid->droidType != DROID_TRANSPORTER),"Tried to control a transporter"));
				secondarySetState(psDroid, DSO_HALTTYPE, DSS_HALT_GUARD);
				psDroid->sMove.Status = MOVEINACTIVE;
			}
		}
	}


	setDrivingStatus(FALSE);
	DriveControlEnabled = FALSE;
}



// Returns TRUE if we have a driven droid.
//
//INLINED BOOL driveHasDriven(void)
//INLINED {
//INLINED 	return (DirectControl) && (psDrivenDroid != NULL) ? TRUE : FALSE;
//INLINED }


// Returns TRUE if drive mode is active.
//
//INLINED BOOL driveModeActive(void)
//INLINED {
//INLINED //DBPRINTF(("%d\n",DirectControl);
//INLINED #ifdef DRIVEFIXED
//INLINED 	return DirectControl;
//INLINED #else
//INLINED 	return psDrivenDroid != NULL ? TRUE : FALSE;
//INLINED #endif
//INLINED }


// Return TRUE if the specified droid is the driven droid.
//
//INLINED BOOL driveIsDriven(DROID *psDroid)
//INLINED {
//INLINED #ifdef DRIVEFIXED
//INLINED 	return (DirectControl) && (psDrivenDroid != NULL) && (psDroid == psDrivenDroid) ? TRUE : FALSE;
//INLINED #else
//INLINED 	return (psDrivenDroid != NULL) && (psDroid == psDrivenDroid) ? TRUE : FALSE;
//INLINED #endif
//INLINED }


//INLINED BOOL driveIsFollower(DROID *psDroid)
//INLINED {
//INLINED #ifdef DRIVEFIXED
//INLINED 	return (DirectControl) && (psDrivenDroid != NULL) && (psDroid != psDrivenDroid) && psDroid->selected ? TRUE : FALSE;
//INLINED #else
//INLINED 	return (psDrivenDroid != NULL) && (psDroid != psDrivenDroid) && psDroid->selected ? TRUE : FALSE;
//INLINED #endif
//INLINED }


//INLINED DROID *driveGetDriven(void)
//INLINED {
//INLINED 	return psDrivenDroid;
//INLINED }


// Call this whenever a droid gets killed or removed.
// returns TRUE if ok, returns FALSE if resulted in driving mode being stopped, ie could'nt find
// a selected droid to drive.
//
BOOL driveDroidKilled(DROID *psDroid)
{
//	DROID *NewDroid;

	if(driveModeActive()) {
		if(psDroid == psDrivenDroid) {
			ChangeDriver();
	//		StopDriverMode();

			psDrivenDroid = NULL;

//			psDroid->selected = FALSE;
			DeSelectDroid(psDroid);

			if(!StartDriverMode(psDroid)) {
	//			NewDroid = intGotoNextDroidType(NULL,DROID_ANY,TRUE);
	//
	//			DBPRINTF(("Droid Killed %p new %p\n",psDroid,NewDroid));
	//
	//			if(NewDroid == psDroid) {
	//				NewDroid = intGotoNextDroidType(NULL,DROID_ANY,TRUE);
	//				DBPRINTF(("Droid Killed %p new %p\n",psDroid,NewDroid));
	//			}
	//
	//			if((!StartDriverMode()) || (NewDroid == psDroid)) {
	//#ifdef PSX
	//				// Failed to find a droid to track!
	//				DBPRINTF(("No droid to drive!\n"));
	//
	//		DBPRINTF(("no droids, find a structure\n");
	//				if(StartObjectOrbit((BASE_OBJECT*)intFindAStructure())) {
	//				} else {
	//		DBPRINTF(("no structures or droids, should be game over!\n");
	//				}
	//#endif
					return FALSE;
	//			}
			}
		}
	}

	return TRUE;
}



BOOL driveDroidIsBusy(DROID *psDroid)
{
	if( DroidIsBuilding(psDroid) ||
		DroidGoingToBuild(psDroid) ||
		DroidIsDemolishing(psDroid) ) {

		return TRUE;
	}

	return FALSE;
}


// Call whenever the selections change to re initialise drive mode.
//
void driveSelectionChanged(void)
{
	if(driveModeActive()) {

		if(psDrivenDroid) {
	//		StopDriverMode();
			ChangeDriver();
			StartDriverMode(NULL);
			driveTacticalSelectionChanged();
		}
	}
}


// Cycle to next droid in group and make it the driver.
//
void driveNextDriver(void)
{
	DROID *psDroid;
//	BOOL Found = FALSE;

//DBPRINTF(("driveNextDriver %p\n",psDrivenDroid);

	// Start from the current driven droid.
	for(psDroid = psDrivenDroid; psDroid; psDroid = psDroid->psNext) {
		if( (psDroid->selected) && (psDroid != psDrivenDroid) ) {
			// Found one so make it the driven droid.
//			psDrivenDroid->sMove.Status = MOVEINACTIVE;
//			psDroid->sMove.Status = MOVEDRIVE;
			psDrivenDroid = psDroid;
			camAllignWithTarget((BASE_OBJECT*)psDroid);
			driveTacticalSelectionChanged();
			return;
		}
	}

//	if(!Found) {
		// Not found so start at the begining.
		for(psDroid = apsDroidLists[selectedPlayer];
			psDroid && (psDroid != psDrivenDroid);
			psDroid = psDroid->psNext) {

			if(psDroid->selected) {
				// Found one so make it the driven droid.
//				psDrivenDroid->sMove.Status = MOVEINACTIVE;
//				psDroid->sMove.Status = MOVEDRIVE;
				psDrivenDroid = psDroid;
				camAllignWithTarget((BASE_OBJECT*)psDroid);
				driveTacticalSelectionChanged();
				return;
			}
		}
//	}
	// No other selected droids found. Oh well...
}


	// PC Version...

static BOOL driveControl(DROID *psDroid)
{
	BOOL Input = FALSE;
	SDWORD MaxSpeed = moveCalcDroidSpeed(psDroid);

	if(!DriveControlEnabled) {
		return FALSE;
	}

	if(keyPressed(KEY_N)) {
		driveNextDriver();
	}

	if(keyDown(KEY_LEFTARROW)) {
		driveDir += DRIVE_TURNSPEED;
		Input = TRUE;
	} else if(keyDown(KEY_RIGHTARROW)) {
		driveDir -= DRIVE_TURNSPEED;
		if(driveDir < 0) {
			driveDir += 360;
		}
		Input = TRUE;
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
		Input = TRUE;
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
		Input = TRUE;
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




static BOOL driveInDriverRange(DROID *psDroid)
{
	if( (abs(psDroid->x-psDrivenDroid->x) < FOLLOW_STOP_RANGE) &&
		(abs(psDroid->y-psDrivenDroid->y) < FOLLOW_STOP_RANGE) ) {
		return TRUE;
	}

	return FALSE;
}


static void driveMoveFollower(DROID *psDroid)
{
	if(driveBumpTime < gameTime) {
		// Update the driven droid's followers.
		if(!driveInDriverRange(psDroid)) {

			//psDroid->secondaryOrder&=~DSS_MOVEHOLD_SET;		// Remove secondary order ... this stops the droid from jumping back to GUARD mode ... see order.c #111 - tjc
			secondarySetState(psDroid, DSO_HALTTYPE, DSS_HALT_GUARD);
			// if the droid is currently guarding we need to change the order to a move
			if (psDroid->order==DORDER_GUARD)
			{
				orderDroidLoc(psDroid,DORDER_MOVE, psDrivenDroid->x,psDrivenDroid->y);
			}
			else
			{
				// otherwise we just adjust its move-to location
				moveDroidTo(psDroid,psDrivenDroid->x,psDrivenDroid->y);
			}

		}
	}

	// Stop it when it gets within range of the driver.
	if(DoFollowRangeCheck) {
		if(driveInDriverRange(psDroid)) {
			psDroid->sMove.Status = MOVEINACTIVE;
		} else {
			AllInRange = FALSE;
		}
	}
}


static void driveMoveCommandFollowers(DROID *psDroid)
{
	DROID	*psCurr;
	DROID_GROUP *psGroup = psDroid->psGroup;

	for (psCurr = psGroup->psList; psCurr!=NULL; psCurr=psCurr->psGrpNext)
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

	AllInRange = TRUE;

	if(DirectControl) {
		if(psDrivenDroid != NULL) {


			if(bMultiPlayer && (driveBumpTime < gameTime))	// send latest info about driven droid.
			{
				SendDroidInfo(psDrivenDroid,DORDER_MOVE,psDrivenDroid->x,psDrivenDroid->y, NULL);
			}

	//TO BE DONE:
		//clear the order on taking over the droid, to stop attacks..
		//send some sort of message when droids stopo and get inrange.


			// Check the driven droid is still selected
			if(psDrivenDroid->selected == FALSE) {
				// if it's not then reset the driving system.
				driveSelectionChanged();
				return;
			}

			// Update the driven droid.
			if(driveControl(psDrivenDroid)) {
				// If control did something then force the droid's move status.
				if(psDrivenDroid->sMove.Status != MOVEDRIVE) {
					psDrivenDroid->sMove.Status = MOVEDRIVE;
					ASSERT(((psDrivenDroid->droidType != DROID_TRANSPORTER),"Tried to control a transporter"));
					driveDir = psDrivenDroid->direction % 360;
				}

				DoFollowRangeCheck = TRUE;
			}

			// Is the driven droid under user control?
			if(psDrivenDroid->sMove.Status == MOVEDRIVE) {
				// Is it a command droid
				if( (psDrivenDroid->droidType == DROID_COMMAND) &&
					(psDrivenDroid->psGroup != NULL) ) {
					driveMoveCommandFollowers(psDrivenDroid);
				}

				for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext) {

					psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;

					if(	(psDroid->selected) &&
						(psDroid != psDrivenDroid) &&
						(psDroid->droidType != DROID_TRANSPORTER) &&
						//((psPropStats->propulsionType != LIFT) || (psDroid->droidType == DROID_CYBORG)) ) {
                        ((psPropStats->propulsionType != LIFT) || cyborgDroid(psDroid)) )
                    {
						// Send new orders to it's followers.
						driveMoveFollower(psDroid);
					}
				}
			}

			if(AllInRange) {
				DoFollowRangeCheck = FALSE;
			}

			if(driveBumpTime < gameTime) {
				// Send next order in 1 second.
				driveBumpTime = gameTime+GAME_TICKS_PER_SEC;
			}
		} else {
#ifdef DRIVEFIXED
			// Start driving
//DBPRINTF(("StartDriveMode\n");
			if(StartDriverMode(NULL) == FALSE) {
//DBPRINTF(("StartObjectOrbit\n");
//				// No droids to drive so start orbiting a structure.
//				if(!OrbitIsValid()) {
//					if(StartObjectOrbit((BASE_OBJECT*)intFindAStructure())) {
//						MouseMovement(FALSE);
//						setDrivingStatus(TRUE);
//						DriveControlEnabled = TRUE;
//					} else {
//	DBPRINTF(("** Failed to start drive mode **\n");
//					}
//				}
			}
#endif
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
//	psDroid->sMove.speed = MAKEFRACT(driveSpeed);
//	psDroid->sMove.dir = driveDir;
	psDroid->direction = (UWORD)driveDir;
}


// Dissable user control of droid, ie when interface is up.

void driveDisableControl(void)
{
	DriveControlEnabled = FALSE;
}


// Enable user control of droid, ie when interface is down.
//
void driveEnableControl(void)
{
	DriveControlEnabled = TRUE;
}



// Return TRUE if drive control is enabled.
//
BOOL driveControlEnabled(void)
{
	return DriveControlEnabled;
}


// Bring up the reticule.
//
void driveEnableInterface(BOOL AddReticule)
{
	if(AddReticule) {
		intAddReticule();
	}

	DriveInterfaceEnabled = TRUE;
}


// Get rid of the reticule.
//
void driveDisableInterface(void)
{
	intResetScreen(FALSE);
	intRemoveReticule();

	DriveInterfaceEnabled = FALSE;
}


// Get rid of the reticule.
//
void driveDisableInterface2(void)
{

	DriveInterfaceEnabled = FALSE;
}


// Return TRUE if the reticule is up.
//
BOOL driveInterfaceEnabled(void)
{
	return DriveInterfaceEnabled;
}





// Check for and process a user request for a new target.
//
void driveProcessAquireButton(void)
{

	if(mouseReleased(MOUSE_RMB) || keyPressed(KEY_S)) {
		BASE_OBJECT	*psObj;
//		psObj = targetAquireNext(TARGET_TYPE_ANY);
//		psObj = targetAquireNearestObj(targetGetCrosshair(),TARGET_TYPE_ANY);
		psObj = targetAquireNearestObjView((BASE_OBJECT*)psDrivenDroid,TARGET_TYPE_ANY);
	}

}





// Start structure placement for drive mode.
//
void driveStartBuild(void)
{

	intRemoveReticule();
	DriveInterfaceEnabled = FALSE;
//	driveDisableInterface();
	driveEnableControl();
}


void driveStartDemolish(void)
{

	intRemoveReticule();
	DriveInterfaceEnabled = FALSE;
	driveEnableControl();
}


// Stop structure placement for drive mode.
//
void driveStopBuild(void)
{
}


// Return TRUE if all the conditions for allowing user control of the droid are met.
//
BOOL driveAllowControl(void)
{

	if( TacticalActive ||
		DriveInterfaceEnabled ||
		(!DriveControlEnabled) ) {

//		if( TacticalActive )	DBPRINTF(("TacticalActive\n");
//		if( DriveInterfaceEnabled )	DBPRINTF(("DriveInterfaceEnabled\n");
//		if( DeliveryReposValid() ) DBPRINTF(("DeliveryReposValid\n");
//		if( (camGetMode() == CAMMODE_ORBIT) ) DBPRINTF(("CAMMODE_ORBIT\n");
//		if( (!DriveControlEnabled) ) DBPRINTF(("!DriveControlEnabled\n");

		return FALSE;
	}

	return TRUE;
}


// Enable Tactical order mode.
//
void driveEnableTactical(void)
{
//	MouseMovement(TRUE);
//	SetMouseRange(0,RADTLX,RADTLY,RADTLX+RADWIDTH,RADTLY+RADHEIGHT);
//	SetMousePos(0,RADTLX+RADWIDTH/2,RADTLY+RADHEIGHT/2);


	StartTacticalScroll(TRUE);
	TacticalActive = TRUE;
	DBPRINTF(("Tactical Mode Activated\n"));
}


// Disable Tactical order mode.
//
void driveDisableTactical(void)
{
	if(driveModeActive() && TacticalActive) {

		CancelTacticalScroll();
	//	MouseMovement(FALSE);
	//	SetMouseRange(0,16,16,639-16,479-16);
	//	SetMousePos(0,320,160);
		TacticalActive = FALSE;
		DBPRINTF(("Tactical Mode Canceled\n"));
	}
}


// Return TRUE if Tactical order mode is active.
//
BOOL driveTacticalActive(void)
{
	return TacticalActive;
}



void driveTacticalSelectionChanged(void)
{
	if(TacticalActive && psDrivenDroid) {
		StartTacticalScrollObj(TRUE,(BASE_OBJECT *)psDrivenDroid);
		DBPRINTF(("driveTacticalSelectionChanged\n"));
	}
}


// Player clicked in the radar window.
//
void driveProcessRadarInput(int x,int y)
{
	SDWORD PosX,PosY;

	// when drive mode is active, clicking on the radar orders all selected droids
	// to move to this position.
	CalcRadarPosition(x,y,(UDWORD *)&PosX,(UDWORD *)&PosY);
	orderSelectedLoc(selectedPlayer, PosX*TILE_UNITS,PosY*TILE_UNITS);
}

