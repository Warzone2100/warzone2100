//
// Drive.c
//
// Routines for player driving units about the map.
//							   

#define DEFINE_DRIVE_INLINE

#include <stdio.h>
#include <Math.h>
#include "imd.h"
#include "vid.h"
#include "Frame.h"
#include "Objects.h"
#include "Move.h"
#include "FindPath.h"
#include "Visibility.h"
#include "Map.h"
#include "FPath.h"
#include "Loop.h"
#include "GTime.h"
#include "audio.h"
#include "audio_id.h"
#include "Geometry.h"
#include "animobj.h"
#include "anim_id.h"
#include "FormationDef.h"
#include "Formation.h"
#include "action.h"
#include "order.h"
#include "astar.h"
#include "Combat.h"
#include "MapGrid.h"
#include "display.h"	// needed for widgetsOn flag.
#include "Effects.h"
#include "fractions.h"
#include "hci.h"
#include "warcam.h"
#include "radar.h"
#include "display3ddef.h"
#include "display3d.h"
#include "Console.h"
#include "Text.h"

// all the bollox needed for script callbacks
#include "interp.h"				// needed to define types in scripttabs.h
#include "parse.h"				// needed to define types in scripttabs.h (Arse!)
#include "scripttabs.h"			// needed to define the callback
#include "ScriptExtern.h"		// needed to include the GLOBAL for checking bInTutorial
#include "group.h"

#define DEBUG_DRIVE_SPEED

#define DRIVEFIXED
#define DRIVENOISE		// Enable driving noises.
#define ENGINEVOL (0xfff)									   
#define MINPITCH (768)
#define PITCHCHANGE (512)

#ifdef WIN32
#include "MultiPlay.h"
#endif

#ifdef PSX
#include "VPad.h"
#include "ctrlpsx.h"
#endif

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
#ifdef PSX
		EnableMouseDraw(TRUE);
		MouseMovement(TRUE);
#endif
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
#ifdef PSX
		EnableMouseDraw(FALSE);
		MouseMovement(FALSE);
#endif
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
#ifdef PSX
#ifdef DRIVENOISE
		DrivingAudioTrack=audio2_StartTrack(ID_SOUND_CONSTRUCTOR_MOVE,ENGINEVOL,ENGINEVOL,0);
#endif
	DBPRINTF(("Starting driving noise\n"));
		MouseMovement(FALSE);
#endif
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

#ifdef PSX
		// If we were orbiting because there were no droids then cancel the orbit.
		if(OrbitIsValid()) {
			CancelObjectOrbit();
		}
#endif
		return TRUE;
	} else {
#ifdef PSX
		if(!OrbitIsValid()) {
	DBPRINTF(("no droids, find a structure\n"));
			if(StartObjectOrbit((BASE_OBJECT*)intFindAStructure())) {
				camSetMode(CAMMODE_ORBIT);
				MouseMovement(FALSE);
				setDrivingStatus(TRUE);

				if(DriveInterfaceEnabled) {
					DBPRINTF(("Interface enabled2 ! Disabling drive control\n"));
					DriveControlEnabled = FALSE;
				} else {
					DriveControlEnabled = TRUE;
				}
			} else {
	DBPRINTF(("no structures, must be game over!\n"));
				StopDriverMode();
			}
		}
#endif
	}

	return FALSE;
}

void StopEngineNoise(void)
{
#ifdef PSX		
#ifdef DRIVENOISE
	audio2_StopTrack(DrivingAudioTrack);
#endif
#endif
	DrivingAudioTrack=-1;
}


void ChangeDriver(void)
{
	DROID *psDroid;

	if(psDrivenDroid != NULL) {
		DBPRINTF(("Driver Changed\n"));
#ifdef PSX		
		StopEngineNoise();
#endif
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
#ifdef PSX		
#ifdef DRIVENOISE
		audio2_StopTrack(DrivingAudioTrack);
#endif
		DrivingAudioTrack=-1;
#endif
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

	#ifdef PSX
//29	MouseMovement(TRUE);
	#endif
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
	#ifdef PSX
		CancelObjectOrbit();
	#endif
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


#ifdef PSX	// Playstation version.

static BOOL driveControl(DROID *psDroid)
{
	BOOL Input = FALSE;
	BOOL AllowDrive = DriveControlEnabled;
	SDWORD MaxSpeed =  moveCalcDroidSpeed(psDroid);
	SDWORD dirVel;
	SDWORD accVel;
	PROPULSION_STATS *psPropStats;
	UDWORD AccVal;
	UDWORD TurnVal;
	static lastTime = 0;

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;

	// Don't allow user control of a droid if interface is up.
//	if(DriveInterfaceEnabled) {
//		AllowDrive = FALSE;
//	}

	// Don't allow user control of a droid if interface is up or Tactical mode is active.
	AllowDrive = driveAllowControl();

	// Don't allow user control if it's a transporter or a vtol.
	if( (psDroid->droidType == DROID_TRANSPORTER) ||
		//((psPropStats->propulsionType == LIFT) && (psDroid->droidType != DROID_CYBORG)) ) {
        ((psPropStats->propulsionType == LIFT) && !cyborgDroid(psDroid)) ) {
		AllowDrive = FALSE;
	}

	// Don't allow user control of a droid if it's busy or orbiting a structure.
//	if((OrbitIsValid() || DroidIsBuilding(psDroid) || DroidGoingToBuild(psDroid) ||	DroidIsDemolishing(psDroid)) &&
	if( (OrbitIsValid() || driveDroidIsBusy(psDroid)) &&
		!DeliveryReposValid() && !TacticalActive && !StructurePosValid()) {

		AllowDrive = FALSE;
		// If the droid is busy then orbit.
		camSetMode(CAMMODE_ORBIT);
	} else {
//		if(!OrbitIsValid() && !DeliveryReposValid() && (gameTime < IdleTime)) {
		if(!OrbitIsValid() && !DeliveryReposValid() && !TacticalActive && !StructurePosValid()) {
			// Otherwise, follow from behind.
			camSetMode(CAMMODE_BEHIND);
		} else {
			if(!DeliveryReposValid() && !TacticalActive && !StructurePosValid()) {
				camSetMode(CAMMODE_ORBIT);
			}
		}
	}

	// Don't allow drive control if were orbiting a building.
	if(OrbitIsValid()) {
		AllowDrive = FALSE;
	} else {
//		if(VPadTriggered(VPAD_MOUSESB) && DriveControlEnabled) {
//		if(VPadTriggered(VPAD_CYCLELEADER) && !TacticalActive && DriveControlEnabled) {
		if(VPadTriggered(VPAD_CYCLELEADER) && DriveControlEnabled) {
			driveNextDriver();
		}
	}
	dirVel = accVel = 0;

	if(VPadTriggered(VPAD_SELECTNAYBORS)) 
	{
		if (bInTutorial) eventFireCallbackTrigger(CALL_ALL_ONSCREEN_DROIDS_SELECTED);

#ifndef MOUSE_EMULATION_ALLOWED
		// If were in tactical mode then select the unit under the cursor
		if(TacticalActive) {
			DROID *psClickedOn;

			psClickedOn = mouseTarget();
			if(psClickedOn != NULL) {
				dealWithDroidSelect(psClickedOn,FALSE);
				psDrivenDroid = psClickedOn;
				driveSelectionChanged();
			}
		}
#endif
		intSelectDroidsInDroidCluster(driveGetDriven());
	}


	AccVal =   (gameTime - lastTime);
	AccVal *= 4096;
	AccVal /= 80;
	AccVal *= DRIVE_ACCELERATE;
	AccVal /= 4096;

	TurnVal =   (gameTime - lastTime);
	TurnVal *= 4096;
	TurnVal /= 80;
	TurnVal *= DRIVE_TURNSPEED;
	TurnVal /= 4096;

	if(AllowDrive) {
		if(GetControllerType(0) == CON_ANALOG) {
			ANALOGDAT *Analog = GetAnalogData(0);

			dirVel = -((SWORD)Analog->joyx0)/DRIVE_TURNDAMP;

			if(dirVel > 0) {
				dirVel = TurnVal;
			} else if(dirVel < 0) {
				dirVel = -TurnVal;
			}

			accVel = -((SWORD)Analog->joyy0);
			if(accVel > 0) {
				accVel = AccVal;
			} else if(accVel < 0) {
				accVel = -AccVal;
			}

			if(dirVel || accVel) {
				Input = TRUE;
			}
		} //else {
			if(!camViewControlsActive()) {
				if(VPadDown(VPAD_MOUSELEFT)) {
					dirVel = TurnVal;	//DRIVE_TURNSPEED;
					Input = TRUE;
				} else if(VPadDown(VPAD_MOUSERIGHT)) {
					dirVel = -TurnVal;	//DRIVE_TURNSPEED;
					Input = TRUE;
				} 

				if(VPadDown(VPAD_MOUSEUP)) {
					accVel = TurnVal;	//DRIVE_ACCELERATE;
					Input = TRUE;
				} else if(VPadDown(VPAD_MOUSEDOWN)) {
					accVel = -TurnVal;	//DRIVE_ACCELERATE;
					Input = TRUE;
				}
			}
//		}
	}

	if(dirVel > 0) {
		driveDir += dirVel;
	} else if(dirVel < 0) {
		driveDir += dirVel;
		if(driveDir < 0) {
			driveDir += 360;
		}
	} 

	driveDir = driveDir % 360;

	if(accVel > 0) {
		if(driveSpeed >= 0) {
			driveSpeed += AccVal;
			if(driveSpeed > MaxSpeed) {
				driveSpeed = MaxSpeed;
			}
		} else {
			driveSpeed += AccVal;	//DRIVE_BRAKE;
			if(driveSpeed > 0) {
				driveSpeed = 0;
			}
		}
	} else if(accVel < 0) {
		if(driveSpeed <= 0) {
			driveSpeed -= AccVal;
			if(driveSpeed < -MaxSpeed) {
				driveSpeed = -MaxSpeed;
			}
		} else {
			driveSpeed -= AccVal;	//DRIVE_BRAKE;
			if(driveSpeed < 0) {
				driveSpeed = 0;
			}
		}
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


//#ifdef DEBUG_DRIVE_SPEED
//	printf("%d %d %d",gameTime - lastTime,AccVal,TurnVal);
//	printf(": %d %d %d\n",MaxSpeed,driveSpeed,MAKEINT(psDroid->sMove.speed));
//#endif

	lastTime = gameTime;

//	{
//		static SDWORD lastSpeed;
//		if(psDroid->sMove.speed != lastSpeed) {
//			printf("Fighting\n");
//		}
//		lastSpeed = psDroid->sMove.speed;
//	}


#ifdef DRIVENOISE
	if (DrivingAudioTrack!=-1)
	{
		UDWORD Pitch;

		if(MaxSpeed) {
			Pitch= ((ABS(driveSpeed))*PITCHCHANGE)/MaxSpeed;
		} else {
			Pitch = 0;
		}
		Pitch+=MINPITCH;

		audio2_Track_SetPitch(DrivingAudioTrack, Pitch);
		
	}
#endif
	if(Input) {
		IdleTime = gameTime + MAX_IDLE;
	}

	return Input;
}


#else	// PC Version...

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

#endif


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

	#ifdef WIN32
			if(bMultiPlayer && (driveBumpTime < gameTime))	// send latest info about driven droid.
			{
				SendDroidInfo(psDrivenDroid,DORDER_MOVE,psDrivenDroid->x,psDrivenDroid->y, NULL);
			}

	//TO BE DONE:
		//clear the order on taking over the droid, to stop attacks..
		//send some sort of message when droids stopo and get inrange.
	#endif

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
#ifdef PSX
	StartInterfaceSnap();
#endif
	DriveInterfaceEnabled = TRUE;
}


// Get rid of the reticule.
//
void driveDisableInterface(void)
{
	intResetScreen(FALSE);
	intRemoveReticule();
#ifdef PSX
	CancelInterfaceSnap();
#endif
	DriveInterfaceEnabled = FALSE;
}


// Get rid of the reticule.
//
void driveDisableInterface2(void)
{
#ifdef PSX
	CancelInterfaceSnap();
#endif
	DriveInterfaceEnabled = FALSE;
}


// Return TRUE if the reticule is up.
//
BOOL driveInterfaceEnabled(void)
{
	return DriveInterfaceEnabled;
}


#ifdef PSX

extern BOOL ReticuleUp;

// Incremetaly close windows, returns TRUE if all windows including the reticule are closed.
//
BOOL IncrementalClose(void)
{
	DBPRINTF(("IncrementalClose : intMode == %d\n",intMode));

	if(intMode == INT_DESIGN) {
		// Remove the design screen.
		intRemoveDesign();
	} else if(intMode == INT_INTELMAP) {
		// Remove the intelligence screen.
		intRemoveIntelMapNoAnim();
	} else if(intMode == INT_STAT) {
		// Remove the object & stats windows.
		intRemoveStats();
		intRemoveObject();
	} else if(intMode == INT_OBJECT) {
		// Remove the object window.
		intRemoveObject();
	} else if(intMode == INT_ORDER) {
		// Remove the order window.
		intRemoveOrder();
		if(!ReticuleUp) {
			if( driveModeActive() || driveWasDriving() ) {
				driveEnableControl();
				driveDisableInterface();
			}
			if(!driveModeActive()) {
				EnableMouseDraw(TRUE);
				MouseMovement(TRUE);
			}
			intMode = INT_NORMAL;
		}
	} else if(intMode == INT_CMDORDER) {
		// Remove the order window.
		intRemoveOrder();
		intRemoveObject();
	} else if(intMode == INT_TRANSPORTER) {
		// Remove the transporter windows.
		intRemoveTrans();
	} else {
		// Remove the reticule.

		if( driveModeActive() || driveWasDriving() ) {
			driveEnableControl();
			driveDisableInterface();
		}
		if(!driveModeActive()) {
			EnableMouseDraw(TRUE);
			MouseMovement(TRUE);
		}
		intMode = INT_NORMAL;

		return TRUE;
	}

	intMode = INT_NORMAL;

	return FALSE;
}


void driveProcessInterfaceButtons(void)
{
	if(intMode != INT_MISSIONRES) {
//	if(!DeliveryReposValid()) {
//	printf("%d %d\n",DriveInterfaceEnabled,DriveControlEnabled);
		if(DriveControlEnabled) {
//			if(VPadPressed(VPAD_MOUSERB)) {		// Pressing PAD_RU opens reticule in drive mode.
			if(VPadPressed(VPAD_RETICULE)) {		// Pressing PAD_RU opens reticule in drive mode.
				if(tryingToGetLocation()) {
					kill3DBuilding();
				} else {
					tboxInitialise();					// Get rid of selection box if it's up.
					driveDisableControl();
					driveEnableInterface(TRUE);
				}
				VPadClearTrig(VPAD_RETICULE);		// Invalidate it for this frame.
			}
		} else {
			if(VPadPressed(VPAD_RETICULE)) {
				VPadClearTrig(VPAD_RETICULE);		// Invalidate it for this frame.
//				if(tryingToGetLocation()) {
//					kill3DBuilding();
//				} else {
					if(IncrementalClose()) {
						driveEnableControl();
						driveDisableInterface();
					}
//				}
			}
		}

//		if( (!TacticalActive) && (!DriveInterfaceEnabled) &&
//			(!StructurePosValid()) && (!DeliveryReposValid()) ) {
		if( (!TacticalActive) &&
//			(!StructurePosValid()) &&
			(!DeliveryReposValid()) ) {

			if(VPadTriggered(VPAD_BATTLEVIEW)) {
#ifdef MOUSE_EMULATION_ALLOWED
				// Switch to point'n'click mode.
				driveDisableDriving();
				kill3DBuilding();
				VPadClearTrig(VPAD_BATTLEVIEW);			// Invalidate it for this frame.
#else
				// Switch to battle view.
				kill3DBuilding();
				driveEnableTactical();
#endif
			}
		} else {
		 	if(VPadTriggered(VPAD_BATTLEVIEW) && !tboxValid()) {
#ifdef MOUSE_EMULATION_ALLOWED
//				kill3DBuilding();
#else
				// Switch out of battle view.
		 		driveDisableTactical();
				kill3DBuilding();
#endif
//				CancelDeliveryRepos();
//				CancelStructurePosition();
			}
		}
	}
}


// Do drive specific button processing when in battle view (point'n'click mode).
//
void driveProcessInterfaceButtons2(void)
{
	if(intMode != INT_MISSIONRES) {

		// If reticule not there then open it.
		if( (intReticuleIsUp() == FALSE) && (intMode != INT_ORDER) ) {
//		if(DriveControlEnabled) {
			if(VPadPressed(VPAD_RETICULE)) {		// Pressing PAD_RU opens reticule.
				VPadClearTrig(VPAD_RETICULE);		// Invalidate it for this frame.
				if(tryingToGetLocation()) {
					kill3DBuilding();
				} else {
					tboxInitialise();					// Get rid of selection box if it's up.
					driveDisableControl();
					driveEnableInterface(TRUE);
					EnableMouseDraw(FALSE);
					MouseMovement(FALSE);
				}
			}
		} else {
			// Otherwise, incrementle close.
			if(VPadPressed(VPAD_RETICULE)) {
				VPadClearTrig(VPAD_RETICULE);		// Invalidate it for this frame.
				IncrementalClose();
			}
		}
	}

	if(!DriveInterfaceEnabled) {
		if(!GetMouseOverRadar())
		{
			if(VPadTriggered(VPAD_SELECTNAYBORS)) 
			{
				DROID *psClickedOn;

				if (bInTutorial) eventFireCallbackTrigger(CALL_ALL_ONSCREEN_DROIDS_SELECTED);

				// Select the unit under the cursor

		  		psClickedOn = mouseTarget();
		  		if(psClickedOn != NULL) {
					if( ((BASE_OBJECT*)psClickedOn)->type == OBJ_DROID) {
						clearSel();
			  			dealWithDroidSelect(psClickedOn,FALSE);
			  			psDrivenDroid = psClickedOn;
			//			driveSelectionChanged();
						intSelectDroidsInDroidCluster(psClickedOn);
						audio_PlayTrack(ID_SOUND_SELECT);
					} else {
						clearSel();
					}
				} else {
					clearSel();
				}
			}
		} else {
			if(VPadTriggered(VPAD_SELECTNAYBORS)) 
			{
				SDWORD PosX,PosY;

				// clicking on the radar orders all selected droids
				// to move to this position.
				CalcRadarPosition(mouseX(),mouseY(),(UDWORD *)&PosX,(UDWORD *)&PosY);
				orderSelectedLoc(selectedPlayer, PosX*TILE_UNITS,PosY*TILE_UNITS);
				audio_PlayTrack(ID_SOUND_SELECT);
			}
		}
	}
}


// Returns TRUE if we were in driving mode but it was disabled by
// pressing triangle to go into battle view.
//
BOOL driveWasDriving(void)
{
	return WasDriving;
}


BOOL driveSetDirectControl(BOOL Control)
{
	DirectControl = Control;
}


BOOL driveSetWasDriving(BOOL Driving)
{
	WasDriving = Driving;
}


// Disable drive mode and go into point'n'click mode.
//
void driveDisableDriving(void)
{
	DBPRINTF(("Disable Driving Mode\n"));
	DirectControl = FALSE;
	WasDriving = TRUE;
	StopDriverMode();
	driveDisableControl();

	// Need to set sensible camera distance and angle.
	camSetMode(CAMMODE_RESTORE);

	if(DriveInterfaceEnabled) {
		MouseMovement(FALSE);
		EnableMouseDraw(FALSE);
	} else {
		SetMousePos(0,iV_GetDisplayWidth()/2,iV_GetDisplayHeight()/2);
		MouseMovement(TRUE);
		EnableMouseDraw(TRUE);
	}

	// Make sure the scroll values are zero'd
	scrollInitVars();

	CONPRINTF(ConsoleString,(ConsoleString,strresGetString(psStringRes,STR_GAM_BATTLEACTIVE)));
}


// Restore driving mode.
//
void driveRestoreDriving(void)
{
	DBPRINTF(("Restore Driving Mode\n"));
	DirectControl = TRUE;
	WasDriving = FALSE;
	StartCameraMode();
	if(DriveInterfaceEnabled) {
		driveDisableControl();
	} else {
		driveEnableControl();
	}
	MouseMovement(FALSE);
	EnableMouseDraw(FALSE);

	// Selection may have changed.
	driveSelectionChanged();

	CONPRINTF(ConsoleString,(ConsoleString,strresGetString(psStringRes,STR_GAM_BATTLEDEACTIVE)));
}


//DROID DummyDroid;
//
// Automaticly enable/dissable driving mode dependant on if war cam is active.
//
// This function needs re-writing / scraping.
//
//void driveAutoToggle(void)
//{
//#ifdef PSX
//	if(DirectControl) {
//#else
//	{
//#endif
//		// Start and stop drive mode depending on war cam active status.
//	#ifdef E3DEMO
//		if( (getWarCamStatus() == TRUE)  &&
//			(driveModeActive() == FALSE) &&
//			(demoGetStatus() == FALSE) )
//	#else
//		if( (getWarCamStatus() == TRUE)  &&
//			(driveModeActive() == FALSE) )
//	#endif
//		{
//			DBPRINTF(("AutoToggle -> on\n"));
//			if(StartDriverMode(NULL)) {
////				intResetScreen(FALSE);
////				intRemoveReticule();
//			} else {
//// problem, no droids so psDrivenDroid == NULL, ok so we set up a structure orbit but
//// driveModeActive continues to return FALSE cause psDrivenDroid == NULL.....
//				if(StartObjectOrbit((BASE_OBJECT*)intFindAStructure())) {
//					MouseMovement(FALSE);
//					setDrivingStatus(TRUE);
//					DriveControlEnabled = TRUE;
////					psDrivenDroid = &DummyDroid;	// Yukity Yuk Yuk Yuk.
//				} else {
//					DBPRINTF(("** Failed to start drive mode **\n"));
//				}
//			}
//		} 
//		else if( (getWarCamStatus() == FALSE) && (driveModeActive() == TRUE) ) 
//		{
//			DBPRINTF(("AutoToggle -> off\n"));
//			if(!StartObjectOrbit((BASE_OBJECT*)intFindAStructure())) {
////				intAddReticule();
//		//		widgetsOn = TRUE;
//				StopDriverMode();
//			}
//		}
//	}
//}
//
#endif


// Check for and process a user request for a new target.
//
void driveProcessAquireButton(void)
{
#ifdef WIN32
	if(mouseReleased(MOUSE_RMB) || keyPressed(KEY_S)) {
		BASE_OBJECT	*psObj;
//		psObj = targetAquireNext(TARGET_TYPE_ANY);
//		psObj = targetAquireNearestObj(targetGetCrosshair(),TARGET_TYPE_ANY);
		psObj = targetAquireNearestObjView((BASE_OBJECT*)psDrivenDroid,TARGET_TYPE_ANY);
	}
#else
//	if(VPadPressed(VPAD_CLOSE)) {
//		BASE_OBJECT	*psObj;
//		psObj = targetAquireNext(TARGET_TYPE_ANY);
//		psObj = targetAquireNearestObj(targetGetCrosshair(),TARGET_TYPE_ANY);
//		psObj = targetAquireNearestObjView((BASE_OBJECT*)psDrivenDroid,TARGET_TYPE_ANY);
//	}

	UDWORD TargetType;

//	if(VPadTriggered(VPAD_TARGETFEATURE)) {
	if(VPadTriggered(VPAD_SELECT)) {
		TargetFeatures = TargetFeatures ? FALSE : TRUE;
		if(TargetFeatures) {
			CONPRINTF(ConsoleString,(ConsoleString,strresGetString(psStringRes,STR_GAM_SELNEUTRAL)));
		} else {
			CONPRINTF(ConsoleString,(ConsoleString,strresGetString(psStringRes,STR_GAM_SELHOSTILE)));
		}
	} 

	if(TargetFeatures) {
		TargetType = TARGET_TYPE_FEATURE | TARGET_TYPE_RESOURCE |
					 TARGET_TYPE_ARTIFACT | TARGET_TYPE_WALL | TARGET_TYPE_FRIEND;
	} else {
		TargetType = TARGET_TYPE_THREAT | 
					TARGET_TYPE_STRUCTURE | TARGET_TYPE_DROID |
					TARGET_TYPE_RESOURCE | TARGET_TYPE_ARTIFACT |
					TARGET_TYPE_FRIEND;
	}

	targetAquireNearestObjView((BASE_OBJECT*)psDrivenDroid,TargetType);

#endif
}


#ifdef PSX
// Display targeting graphics.
//
void driveMarkTarget(void)
{
	if(camGetMode() == CAMMODE_BEHIND) {
		BASE_OBJECT *psObj = targetGetCurrent();
		if(psObj != NULL) {
			if(driveAllowControl()) {
				MouseMovement(FALSE);
				targetMarkCurrent();
				SetMousePos(0,psObj->sDisplay.screenX,psObj->sDisplay.screenY);
				pie_DrawMouse(psObj->sDisplay.screenX,psObj->sDisplay.screenY);
			}
		}
	}
}
#endif


// Start structure placement for drive mode.
//
void driveStartBuild(void)
{
#ifdef PSX
	CancelInterfaceSnap();
#endif
	intRemoveReticule();
	DriveInterfaceEnabled = FALSE;
//	driveDisableInterface();
	driveEnableControl();
}


void driveStartDemolish(void)
{
#ifdef PSX
	CancelInterfaceSnap();
#endif
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
#ifdef PSX
	if( TacticalActive ||
		DriveInterfaceEnabled ||
		DeliveryReposValid() ||
		StructurePosValid() ||
		(camGetMode() == CAMMODE_ORBIT) ||
		(!DriveControlEnabled) ) {
#else
	if( TacticalActive ||
		DriveInterfaceEnabled ||
		(!DriveControlEnabled) ) {
#endif
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

#ifdef PSX
	CONPRINTF(ConsoleString,(ConsoleString,strresGetString(psStringRes,STR_GAM_BATTLEACTIVE)));
#endif
	StartTacticalScroll(TRUE);
	TacticalActive = TRUE;
	DBPRINTF(("Tactical Mode Activated\n"));
}


// Disable Tactical order mode.
//
void driveDisableTactical(void)
{
	if(driveModeActive() && TacticalActive) {
#ifdef PSX
		CONPRINTF(ConsoleString,(ConsoleString,strresGetString(psStringRes,STR_GAM_BATTLEDEACTIVE)));
#endif
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

