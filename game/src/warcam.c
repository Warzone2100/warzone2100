/* WarCAM - Handles tracking/following of in game objects */
/* Alex McLean, Pumpkin Studios, EIDOS Interactive, 1998 */
/*	23rd September, 1998 - This code is now so hideously complex
	and unreadable that's it's inadvisable to attempt changing
	how the camera works, since I'm not sure that I'll be able to even
	get it working the way it used to, should anything get broken. 
	I really hope that no further changes are needed here...:-(
	Alex M. */
#include "stdio.h"
#include "Frame.h"
#include "geo.h"
#include "piedef.h" //ivis matrix code
#include "Objects.h"
#include "WarCAM.h"
#include "Display.h"
#include "Display3d.h"
#include "HCI.h"
#include "Console.h"
#include "GTime.h"
#include "Effects.h"
#include "Map.h"
#include "Geometry.h"
#include "oprint.h"
#include "MiscImd.h"
#include "Loop.h"
#include "drive.h"
#include "Move.h"
#include "Order.h"
#include "Action.h"
#include "IntDisplay.h"
#include "E3Demo.h"
#include "RayCast.h"
#include "Display3d.h"
#ifndef PAUL
#include "Selection.h"
#endif
#ifdef PSX
#include "drawIMD_psx.h"
#include "VPad.h"
#include "ctrlpsx.h"
#endif

#ifdef WIN32
#define MODFRACT(value,mod) \
	while((value) < 0)	{ (value) += (mod); } \
	while((value) > (mod)) { (value) -= (mod); }
#else 
#define MODFRACT(value,mod) \
	(value) = (value) % (mod)
#endif

#define MIN_TRACK_HEIGHT 16

extern BOOL bTrackingTransporter;

static	UDWORD	testAngle=250;
/* Holds all the details of our camera */
static	WARCAM	trackingCamera;

/* Present rotation for the 3d camera logo */
static	SDWORD	warCamLogoRotation;

/* The fake target that we track when jumping to a new location on the radar */
static	BASE_OBJECT	radarTarget;

/* Do we trun to face when doing a radar jump? */
static	BOOL	bRadarAllign;

/* How far we track relative to the droids location - direction matters */
SDWORD	camDroidXOffset;
SDWORD	camDroidYOffset;
SDWORD	presAvAngle = 0;;
/* Camera logo spins at 120 degrees a second */
#define	LOGO_ROT_SPEED DEG(120)

/*	These are the DEFAULT offsets that make us track _behind_ a droid and allow
	it to be pretty far _down_ the screen, so we can see more 
*/
#ifdef WIN32
#define	CAM_DEFAULT_X_OFFSET	-400
#define CAM_DEFAULT_Y_OFFSET	-400	
#define	MINCAMROTX	-20
#else
#define	CAM_DEFAULT_X_OFFSET	200
#define CAM_DEFAULT_Y_OFFSET	200	
#define MINCAMROTX  -10
#endif

/* Function Prototypes... */
/* Firstly for tracking position */
void	updateCameraAcceleration	( UBYTE update );
void	updateCameraVelocity		( UBYTE update );
void	updateCameraPosition		( UBYTE update );

/* And now, rotation */
void	updateCameraRotationAcceleration( UBYTE update );
void	updateCameraRotationVelocity( UBYTE update );
void	updateCameraRotationPosition( UBYTE update );

void	initWarCam					( void );
BOOL	processWarCam				( void );
void	setWarCamActive				( BOOL status );
BASE_OBJECT	*camFindTarget			( void );
void	camAllignWithTarget			( BASE_OBJECT *psTarget );
BOOL	camTrackCamera				( void );
void	camSwitchOff				( void );
BOOL	getWarCamStatus				( void );
void	camToggleInfo				( void );
//void	setUpRadarTarget			( SDWORD x, SDWORD y );
void	setUpRadarTarget			( SDWORD x, SDWORD y );
void	requestRadarTrack			( SDWORD x, SDWORD y );
BOOL	getRadarTrackingStatus		( void );
void	dispWarCamLogo				( void );
UDWORD	getPositionMagnitude		( void );
UDWORD	getRotationMagnitude		( void );
void	toggleRadarAllignment		( void );
void	camInformOfRotation			( iVector *rotation );
void	processLeaderSelection		( void );
SDWORD	getAverageTrackAngle		( BOOL bCheckOnScreen );
SDWORD	getGroupAverageTrackAngle	( UDWORD groupNumber, BOOL bCheckOnScreen );
void	getTrackingConcerns			( SDWORD *x,SDWORD *y, SDWORD *z );
void	getGroupTrackingConcerns	( SDWORD *x,SDWORD *y, SDWORD *z,UDWORD groupNumber, BOOL bOnScreen );

UDWORD	getNumDroidsSelected		( void );

/*	These used to be #defines but they're variable now as it may be necessary
	to allow the player	to customise tracking speed? Jim? 
*/
FRACT	accelConstant,velocityConstant, rotAccelConstant, rotVelocityConstant;

/* How much info do you want when tracking a droid - this toggles full stat info */
static	BOOL bFullInfo = FALSE;

/* Are we requesting a new track to start that is a radar (location) track? */
static	BOOL bRadarTrackingRequested = FALSE;

/* World coordinates for a radar track/jump */
//static  SDWORD	 radarX,radarY;	
static  FRACT	 radarX,radarY;	

/*	Where we were up to (pos and rot) last update - allows us to see whether
	we are sufficently near our target to disable further tracking */
static	iVector	oldPosition,oldRotation;

/* The fraction of a second that the last game frame took */
static	FRACT	fraction;

static BOOL OldViewValid;

//-----------------------------------------------------------------------------------
/* Sets the camera to inactive to begin with */
void	initWarCam( void )
{
	/* We're not intitially following anything */
	trackingCamera.status = CAM_INACTIVE;	

	/* Set up the default tracking variables */
	accelConstant = ACCEL_CONSTANT;
	velocityConstant = VELOCITY_CONSTANT;
	rotAccelConstant = ROT_ACCEL_CONSTANT;
	rotVelocityConstant = ROT_VELOCITY_CONSTANT;

	/* Logo setup */
	warCamLogoRotation = 0;

  /* Offset from droid's world coords */
	camDroidXOffset = CAM_DEFAULT_X_OFFSET;
	camDroidYOffset = CAM_DEFAULT_Y_OFFSET;
	OldViewValid = FALSE;
}

//-----------------------------------------------------------------------------------


// Just turn it off.
//
void CancelWarCam(void)
{
   	if(trackingCamera.target->type == OBJ_DROID) {
		if( bTrackingTransporter && (((DROID*)trackingCamera.target)->droidType == DROID_TRANSPORTER) ) {
			return;
		}
	}

	trackingCamera.status = CAM_INACTIVE;
}


/* Updates the camera position/angle along with the object movement */
BOOL	processWarCam( void )
{
BASE_OBJECT	*foundTarget;
BOOL Status = TRUE;

	/* Get out if the camera isn't active */
	if(trackingCamera.status == CAM_INACTIVE)
	{
		return(TRUE);
	}

	/* Calculate fraction of a second for last game frame */
	fraction = (MAKEFRACT(frameTime2) / MAKEFRACT(GAME_TICKS_PER_SEC));

	/* Ensure that the camera only ever flips state within this routine! */
	switch(trackingCamera.status)
	{
	case CAM_REQUEST:
			
			/* See if we can find the target to follow */
			foundTarget = camFindTarget();
			
			if(foundTarget AND !foundTarget->died)
			{
				/* We've got one, so store away info */
				camAllignWithTarget(foundTarget);
				/* We're now into tracking status */
				trackingCamera.status = CAM_TRACKING;
				/* Inform via console */
				if(foundTarget->type == OBJ_DROID)
				{
					if(!getWarCamStatus())
					{
						CONPRINTF(ConsoleString,(ConsoleString,"WZ/CAM  - %s",droidGetName((DROID*)foundTarget)));
					}
				}
				else
				{
//					CONPRINTF(ConsoleString,(ConsoleString,"DROID-CAM V0.1 Enabled - Now tracking new location"));
				}
			}
			else
			{
				/* We've requested a track with no droid selected */
//				addConsoleMessage("Droid-CAM V0.1 ERROR - No targets(s) selected",DEFAULT_JUSTIFY);
				trackingCamera.status = CAM_INACTIVE;
			}
		break;

	case CAM_TRACKING:
			/* Track the droid unless routine comes back false */
			if(!camTrackCamera())
			{
				/*
					Camera track came back false, either because droid died or is
					no longer selected, so reset to old values 
				*/
				foundTarget = camFindTarget();
				if(foundTarget AND !foundTarget->died)
				{
					trackingCamera.status = CAM_REQUEST;
				}
				else
				{
					trackingCamera.status = CAM_RESET;
				}
			}
#ifdef WIN32
		processLeaderSelection();
#endif
		break;
	case CAM_RESET:
			/* Reset camera to pre-droid tracking status */
			if( (trackingCamera.target==NULL)
			  ||(trackingCamera.target->type!=OBJ_TARGET))
			{
				camSwitchOff();
			}
			/* Switch to inactive mode */
			trackingCamera.status = CAM_INACTIVE;
//			addConsoleMessage("Droid-CAM V0.1 Disabled",DEFAULT_JUSTIFY);
			Status = FALSE;
		break;
	default:
		DBERROR(("Weirdy status for tracking Camera"));
		break;
	}
	/* TBR
	flushConsoleMessages();
	CONPRINTF(ConsoleString,(ConsoleString,"Acceleration of movement constant : %.2f",accelConstant));
	CONPRINTF(ConsoleString,(ConsoleString,"Velocity of movement constant : %.2f",velocityConstant));
	CONPRINTF(ConsoleString,(ConsoleString,"Acceleration of rotation constant : %.2f",rotAccelConstant));
	CONPRINTF(ConsoleString,(ConsoleString,"Velocity of rotation constant : %.2f",rotVelocityConstant));
	CONPRINTF(ConsoleString,(ConsoleString,"Tracking droid direction : %d",trackingCamera.droid->direction));
	CONPRINTF(ConsoleString,(ConsoleString,"Tracking droid pitch : %d",trackingCamera.droid->pitch));
	CONPRINTF(ConsoleString,(ConsoleString,"Tracking droid roll : %d",trackingCamera.droid->roll));
	CONPRINTF(ConsoleString,(ConsoleString,"Tracking droid height (z) : %d",trackingCamera.droid->z));
	CONPRINTF(ConsoleString,(ConsoleString,"position.p.y : %d",player.p.y));
	*/

	return Status;
}

//-----------------------------------------------------------------------------------

/* Flips states for camera active */
void	setWarCamActive(BOOL status)
{
	DBPRINTF(("setWarCamActive(%d)\n",status));

	/* We're trying to switch it on */
	if(status == TRUE)
	{
		/* If it's not inactive then it's already in use - so return */
		/* We're tracking a droid */
		if(trackingCamera.status!=CAM_INACTIVE)
		{
			if(bRadarTrackingRequested)
			{
				trackingCamera.status = CAM_REQUEST;
			}
			else
			{
				return;
			}
		}
		else
		{
			/* Otherwise request the camera to track */
			trackingCamera.status = CAM_REQUEST;
		}
	}
	else
		/* We trying to switch off */
	{
		/* Is it already off? */
		if(trackingCamera.status == CAM_INACTIVE)
		{
			return;
		}
		else
		{
			/* Attempt to set to normal */
			trackingCamera.status = CAM_RESET;
		}
	}
}

//-----------------------------------------------------------------------------------

BASE_OBJECT	*camFindDroidTarget(void)
{
	DROID	*psDroid;

	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
#ifdef PSX
		if( (psDroid->selected) || (bTrackingTransporter && (psDroid->droidType == DROID_TRANSPORTER) ) )
#else
		if(psDroid->selected)
#endif
		{
			/* Return the first one found */
			return( (BASE_OBJECT*)psDroid);
		}
	}

//printf("camFindUnitTarget : NOT FOUND\n");
	/* We didn't find one */
	return(NULL);
}



/* Attempts to find the target for the camera to track */
BASE_OBJECT	*camFindTarget(void)
{
	/*	See if we can find a selected droid. If there's more than one
		droid selected for the present player, then we track the oldest
		one. */

	if(bRadarTrackingRequested)
	{
		setUpRadarTarget(radarX, radarY);
		bRadarTrackingRequested = FALSE;
		return(&radarTarget);
	}

	return camFindDroidTarget();
}

//-----------------------------------------------------------------------------------
UDWORD	getTestAngle(void)
{
	return(testAngle);
}
//-----------------------------------------------------------------------------------
void	updateTestAngle( void )
{
	testAngle += 1;
	if(testAngle>=360)
	{
		testAngle = 0;
	}
}
//-----------------------------------------------------------------------------------

/* Stores away old viewangle info and sets up new distance and angles */
void	camAllignWithTarget(BASE_OBJECT *psTarget)
{
	/* Store away the target */
	trackingCamera.target = psTarget;
	
	/* Save away all the view angles */
	trackingCamera.oldView.r.x = trackingCamera.rotation.x = MAKEFRACT(player.r.x);
	trackingCamera.oldView.r.y = trackingCamera.rotation.y = MAKEFRACT(player.r.y);
	trackingCamera.oldView.r.z = trackingCamera.rotation.z = MAKEFRACT(player.r.z);

	/* Store away the old positions and set the start position too */
	trackingCamera.oldView.p.x = trackingCamera.position.x = MAKEFRACT(player.p.x);
	trackingCamera.oldView.p.y = trackingCamera.position.y = MAKEFRACT(player.p.y);
	trackingCamera.oldView.p.z = trackingCamera.position.z = MAKEFRACT(player.p.z);

   //	trackingCamera.rotation.x = player.r.x = DEG(-90);
	/* No initial velocity for moving */
	trackingCamera.velocity.x = trackingCamera.velocity.y = trackingCamera.velocity.z = MAKEFRACT(0);
	/* Nor for rotation */
	trackingCamera.rotVel.x = trackingCamera.rotVel.y = trackingCamera.rotVel.z = MAKEFRACT(0);
	/* No initial acceleration for moving */
	trackingCamera.acceleration.x = trackingCamera.acceleration.y = trackingCamera.acceleration.z =MAKEFRACT(0);
	/* Nor for rotation */
	trackingCamera.rotAccel.x = trackingCamera.rotAccel.y = trackingCamera.rotAccel.z = MAKEFRACT(0);

	/* Sote the old distance */
	trackingCamera.oldDistance = getViewDistance();	//distance;

	/* Store away when we started */
	trackingCamera.lastUpdate = gameTime2;

#ifdef PSX
	trackingCamera.status = CAM_TRACKING;
#endif
	OldViewValid = TRUE;
}

//-----------------------------------------------------------------------------------


#ifdef WIN32


//-----------------------------------------------------------------------------------
							/* How this all works */
/*
Each frame we calculate the new acceleration, velocity and positions for the location
and rotation of the camera. The velocity is obviously based on the acceleration and this
in turn is based on the separation between the two objects. This separation is distance
in the case of location and degrees of arc in the case of rotation.

  Each frame:-

  ACCELERATION	-	A
  VELOCITY		-	V
  POSITION		-	P
  Location of camera	(x1,y1)
  Location of droid		(x2,y2)
  Separation(distance) = D. This is the distance between (x1,y1) and (x2,y2)

  A = c1D - c2V		Where c1 and c2 are two constants to be found (by experiment)
  V = V + A(frameTime/GAME_TICKS_PER_SEC)
  P = P + V(frameTime/GAME_TICKS_PER_SEC)

  Things are the same for the rotation except that D is then the difference in angles 
  between the way the camera and droid being tracked are facing. AND.... the two
  constants c1 and c2 will be different as we're dealing with entirely different scales
  and units. Separation in terms of distance could be in the thousands whereas degrees
  cannot exceed 180.

  This all works because acceleration is based on how far apart they are minus some factor
  times the camera's present velocity. This minus factor is what slows it down when the 
  separation gets very small. Without this, it would continually oscillate about it's target
  point. The four constants (two each for rotation and position) need to be found 
  by trial and error since the metrics of time,space and rotation are entirely warzone
  specific.

  And that's all folks.
*/  

//-----------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------
/*	N.B. the code from here on in is not very PSX friendly as there's lots of 
	unfriendly little floats - essentially the next 6 functions */

void	updateCameraAcceleration(UBYTE update)
{
FRACT	separation;
SDWORD	realPos;
SDWORD	xConcern,yConcern,zConcern;
SDWORD	xBehind,yBehind;
BOOL	bFlying;
DROID	*psDroid;
UDWORD	multiAngle;
PROPULSION_STATS	*psPropStats;
//SDWORD	pitch;
SDWORD	angle;

	angle = abs(((player.r.x/182)%90));
	angle = 90-angle;

	bFlying = FALSE;
	if(trackingCamera.target->type == OBJ_DROID)
	{
		psDroid = (DROID*)trackingCamera.target;
		psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
		if(psPropStats->propulsionType == LIFT)
		{
			bFlying = TRUE;
		}
	}
	/*	This is where we check what it is we're tracking. 
		Were we to track a building or location - this is
		where it'd be set up */
	
	/*	If we're tracking a droid, then we nned to track slightly in front
		of it in order that the droid appears down the screen a bit. This means
		that we need to find an offset point from it relative to it's present 
		direction 
	*/
	if(trackingCamera.target->type == OBJ_DROID)
	{
		/* Present direction is important */
		if(getNumDroidsSelected()>2)
		{
			if(trackingCamera.target->selected)
			{
				multiAngle = getAverageTrackAngle(TRUE);
			}
			else
			{
				multiAngle = getGroupAverageTrackAngle(trackingCamera.target->group,TRUE);
			}
			xBehind = ((camDroidYOffset*SIN(DEG(multiAngle))) >> FP12_SHIFT);
			yBehind = ((camDroidXOffset*COS(DEG(multiAngle))) >> FP12_SHIFT);
		}
		else
		{
		 	xBehind = ((camDroidYOffset*SIN(DEG(trackingCamera.target->direction))) >> FP12_SHIFT);
			yBehind = ((camDroidXOffset*COS(DEG(trackingCamera.target->direction))) >> FP12_SHIFT);
		}
	}
	else
	{
		/* Irrelevant for normal radar tracking */
		xBehind = 0;
		yBehind = 0;
	}

	/*	Get these new coordinates */
	if(getNumDroidsSelected()>2 AND trackingCamera.target->type == OBJ_DROID)
	{
	 	xConcern = trackingCamera.target->x;		  // nb - still NEED to be set
		yConcern = trackingCamera.target->z;
		zConcern = trackingCamera.target->y;
		if(trackingCamera.target->selected)
		{
			getTrackingConcerns(&xConcern,&yConcern,&zConcern);
		}
		else
		{
			getGroupTrackingConcerns(&xConcern,&yConcern,&zConcern,trackingCamera.target->group,TRUE);
		}
//		getBestPitchToEdgeOfGrid(xConcern,zConcern,360-((getAverageTrackAngle(TRUE)+180)%360),&pitch);
		yConcern+=angle*5;

	}
	else
	{
		xConcern = trackingCamera.target->x;
		yConcern = trackingCamera.target->z;
		zConcern = trackingCamera.target->y;
	}

	if(trackingCamera.target->type == OBJ_DROID AND getNumDroidsSelected()<=2)
	{
//		getBestPitchToEdgeOfGrid(trackingCamera.target->x,trackingCamera.target->z,
//			360-((trackingCamera.target->direction+180)%360),&pitch);
		yConcern+=angle*5;

	}


	if(update & X_UPDATE)
	{
		/* Need to update acceleration along x axis */
		realPos = xConcern - (CAM_X_SHIFT) - xBehind;
		separation = (FRACT)(realPos - trackingCamera.position.x);
		if(!bFlying)
		{
		 	trackingCamera.acceleration.x = 
				(accelConstant*separation - velocityConstant*(FRACT)trackingCamera.velocity.x);
		}
		else
		{
			trackingCamera.acceleration.x = 
				((accelConstant*separation*4) - (velocityConstant*2*(FRACT)trackingCamera.velocity.x));

		}
	}					          
						          
	if(update & Y_UPDATE)	          
	{					          
//		flushConsoleMessages();
//		CONPRINTF(ConsoleString,(ConsoleString,"Attempted height : %d",yConcern));

		/* Need to update acceleration along y axis */
		realPos = (yConcern);
		separation = (FRACT)(realPos - trackingCamera.position.y);
		if(bFlying) separation = separation/2;
//		CONPRINTF(ConsoleString,(ConsoleString,"Separation : %f",separation));
//		CONPRINTF(ConsoleString,(ConsoleString,"Distance : %d",distance));
		if(!bFlying)
		{
		 	trackingCamera.acceleration.y =
				((accelConstant)*separation - (velocityConstant)*trackingCamera.velocity.y);
		}
		else
		{
			trackingCamera.acceleration.y = 
				(((accelConstant)*separation*4) - ((velocityConstant)*2*trackingCamera.velocity.y));
		}
	}					          
						          
	if(update & Z_UPDATE)	          
	{					          
		/* Need to update acceleration along z axis */
		realPos = zConcern - (CAM_Z_SHIFT) - yBehind;
		separation = (FRACT)(realPos - trackingCamera.position.z);
		if(!bFlying)
		{
			trackingCamera.acceleration.z =  
				(accelConstant*separation - velocityConstant*trackingCamera.velocity.z);
		}
		else
		{
			trackingCamera.acceleration.z =  
				((accelConstant*separation*4) - (velocityConstant*2*trackingCamera.velocity.z));

		}
	}
}

//-----------------------------------------------------------------------------------

void	updateCameraVelocity( UBYTE	update )
{
//UDWORD	frameTime;
FRACT	fraction;
	
	/*	Get the time fraction of a second - the next two lines are present in 4
		of the next six functions. All 4 of these functions are called every frame, so
		it may be an idea to calculate these higher up and store them in a static but 
		I've left them in for clarity for now */

//	frameTime = gameTime - trackingCamera.lastUpdate;
	fraction = (MAKEFRACT(frameTime2) / (FRACT)GAME_TICKS_PER_SEC);

	if(update & X_UPDATE)
	{
		trackingCamera.velocity.x += (trackingCamera.acceleration.x * fraction);
	}

	if(update & Y_UPDATE)
	{
		trackingCamera.velocity.y += (trackingCamera.acceleration.y * fraction);
	}

	if(update & Z_UPDATE)
	{
		trackingCamera.velocity.z += (trackingCamera.acceleration.z * fraction);
	}
}

//-----------------------------------------------------------------------------------

void	updateCameraPosition(UBYTE update)
{
//UDWORD	frameTime;
BOOL	bFlying;
FRACT	fraction;
DROID	*psDroid;
PROPULSION_STATS	*psPropStats;

	bFlying = FALSE;
	if(trackingCamera.target->type == OBJ_DROID)
	{
		psDroid = (DROID*)trackingCamera.target;
		psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
		if(psPropStats->propulsionType == LIFT)
		{
			bFlying = TRUE;
		}
	}
	/* See above */
//	frameTime = gameTime - trackingCamera.lastUpdate;
	fraction = (MAKEFRACT(frameTime2) / (FRACT)GAME_TICKS_PER_SEC);

	if(update & X_UPDATE)
	{
		/* Need to update position along x axis */
		trackingCamera.position.x += (trackingCamera.velocity.x * fraction);
	}					          
						          
	if(update & Y_UPDATE)	          
	{					          
//		if(trackingCamera.target->type == OBJ_TARGET)
//		{
			/* Need to update position along y axis */
//		if(!bFlying)
//		{
			trackingCamera.position.y +=(trackingCamera.velocity.y * fraction);
//		}
//		else
//		{
//			trackingCamera.position.y += MAKEINT(((FRACT)trackingCamera.velocity.y * fraction));
//		}
//		}
	}					          
						          
	if(update & Z_UPDATE)	          
	{					          
		/* Need to update position along z axis */
		trackingCamera.position.z += (trackingCamera.velocity.z * fraction);
	}
}

//-----------------------------------------------------------------------------------
/* Calculate the acceleration that the camera spins around at */
void	updateCameraRotationAcceleration( UBYTE update )
{
SDWORD	worldAngle;
FRACT	separation;
SDWORD	xConcern, yConcern, zConcern;
BOOL	bTooLow;
DROID	*psDroid;
UDWORD	droidHeight,mapHeight,difHeight;
PROPULSION_STATS	*psPropStats;
SDWORD	pitch;
BOOL	bGotFlying = FALSE;
SDWORD	xPos,yPos,zPos;

	bTooLow = FALSE;
	if(trackingCamera.target->type == OBJ_DROID)
	{
		psDroid = (DROID*)trackingCamera.target;
		psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
		if(psPropStats->propulsionType == LIFT)
		{
			bGotFlying = TRUE;
			droidHeight = psDroid->z;
			mapHeight = map_Height(psDroid->x,psDroid->y);
			difHeight = abs(droidHeight - mapHeight);
			if(difHeight < MIN_TRACK_HEIGHT)
			{
				bTooLow = TRUE;
			}
		}
	}

	if(update & Y_UPDATE)
	{
		/* Presently only y rotation being calculated - but same idea for other axes */
		/* Check what we're tracking */
		if(getNumDroidsSelected()>2 AND trackingCamera.target->type == OBJ_DROID)
		{
			if(trackingCamera.target->selected)
			{
				yConcern =	DEG(getAverageTrackAngle(FALSE));//DEG(trackingCamera.target->direction);	
			}
			else
			{
				yConcern =	DEG(getGroupAverageTrackAngle(trackingCamera.target->group,FALSE));//DEG(trackingCamera.target->direction);	
			}
		}
		else
		{
			yConcern =	DEG(trackingCamera.target->direction);	
		}
		yConcern +=	DEG(180);
		
  		while(trackingCamera.rotation.y<0)
		{
			trackingCamera.rotation.y+=DEG(360);
		}
		
		/* Which way are we facing? */
		worldAngle =  trackingCamera.rotation.y;
		separation = (FRACT) ((yConcern - worldAngle));		
		if(separation<DEG(-180))
		{
			separation+=DEG(360);
		}
		else if(separation>DEG(180))
		{
			separation-=DEG(360);
		}

		/* Make new acceleration */
		trackingCamera.rotAccel.y = 
			(rotAccelConstant*separation - rotVelocityConstant*trackingCamera.rotVel.y);
	}

	if(update & X_UPDATE)
	{
		if(trackingCamera.target->type == OBJ_DROID AND !bGotFlying)
		{
			psDroid = (DROID*)trackingCamera.target;
			getTrackingConcerns(&xPos,&yPos,&zPos);
			if(trackingCamera.target->selected)
			{
				getBestPitchToEdgeOfGrid(xPos,zPos,360-((getAverageTrackAngle(TRUE)+180)%360),&pitch);
			}
			else
			{
				getBestPitchToEdgeOfGrid(xPos,zPos,360-((getGroupAverageTrackAngle(trackingCamera.target->group,TRUE)+180)%360),&pitch);
			}
			if(pitch<14) pitch = 14;
			xConcern = DEG(-pitch);
		}
		else
		{
			xConcern = DEG(trackingCamera.target->pitch);
			xConcern += DEG(-16);
		}	
	
		//xConcern = DEG(trackingCamera.target->pitch);
	   //	if(xConcern>DEG(MINCAMROTX))
	   //	{
	   //		xConcern = DEG(MINCAMROTX);
	   //	}
		while(trackingCamera.rotation.x<0)
			{
				trackingCamera.rotation.x+=DEG(360);
			}
		worldAngle =  trackingCamera.rotation.x;
		separation = (FRACT) ((xConcern - worldAngle));		

		MODFRACT(separation,DEG(360));

		if(separation<DEG(-180))
		{
			separation+=DEG(360);
		}
		else if(separation>DEG(180))
		{
			separation-=DEG(360);
		}

		/* Make new acceleration */
		trackingCamera.rotAccel.x = 
			/* Make this really slow */
			((rotAccelConstant)*separation - rotVelocityConstant*(FRACT)trackingCamera.rotVel.x);
	}

	/* This looks a bit arse - looks like a flight sim */
	if(update & Z_UPDATE)
	{
		if(bTooLow)
		{
			zConcern = 0;
		}
		else
		{
			zConcern = DEG(trackingCamera.target->roll);
		}
		while(trackingCamera.rotation.z<0)
			{
				trackingCamera.rotation.z+=DEG(360);
			}
		worldAngle =  trackingCamera.rotation.z;
		separation = (FRACT) ((zConcern - worldAngle));		
		if(separation<DEG(-180))
		{
			separation+=DEG(360);
		}
		else if(separation>DEG(180))
		{
			separation-=DEG(360);
		}

		/* Make new acceleration */
		trackingCamera.rotAccel.z = 
			/* Make this really slow */
			((rotAccelConstant/1)*separation - rotVelocityConstant*(FRACT)trackingCamera.rotVel.z);
	}

}

//-----------------------------------------------------------------------------------
/*	Calculate the velocity that the camera spins around at - just add previously
	calculated acceleration */
void	updateCameraRotationVelocity( UBYTE update )
{
//UDWORD	frameTime;
FRACT	fraction;

//	frameTime = gameTime - trackingCamera.lastUpdate;
	fraction = (MAKEFRACT(frameTime2) / (FRACT)GAME_TICKS_PER_SEC);

	if(update & Y_UPDATE)
	{
		trackingCamera.rotVel.y += ((FRACT)trackingCamera.rotAccel.y * fraction);	
	}
	if(update & X_UPDATE)
	{
		trackingCamera.rotVel.x += ((FRACT)trackingCamera.rotAccel.x * fraction);	
	}
	if(update & Z_UPDATE)
	{
		trackingCamera.rotVel.z += ((FRACT)trackingCamera.rotAccel.z * fraction);	
	}

}

//-----------------------------------------------------------------------------------
/* Move the camera around by adding the velocity */
void	updateCameraRotationPosition( UBYTE update )										
{
//UDWORD	frameTime;
FRACT	fraction;

//	frameTime = gameTime - trackingCamera.lastUpdate;
	fraction = (MAKEFRACT(frameTime2) / (FRACT)GAME_TICKS_PER_SEC);

 	if(update & Y_UPDATE)
	{
		trackingCamera.rotation.y += (trackingCamera.rotVel.y * fraction);	
	}
	if(update & X_UPDATE)
	{
		trackingCamera.rotation.x += (trackingCamera.rotVel.x * fraction);	
	}
	if(update & Z_UPDATE)
	{
		trackingCamera.rotation.z += (trackingCamera.rotVel.z * fraction);	
	}
}

BOOL	nearEnough(void)
{
BOOL	retVal = FALSE;
SDWORD	xPos;
SDWORD	yPos;

	xPos = player.p.x +(VISIBLE_XTILES*TILE_UNITS)/2;
	yPos = player.p.z +(VISIBLE_YTILES*TILE_UNITS)/2;

	if( (abs(xPos-trackingCamera.target->x) <= 256) AND
		(abs(yPos-trackingCamera.target->y) <= 256) )
		{
			retVal = TRUE;
		}
	return(retVal);
}

/* Updates the viewpoint according to the object being tracked */
BOOL	camTrackCamera( void )
{
PROPULSION_STATS	*psPropStats;
DROID	*psDroid;
BOOL	bFlying;

	bFlying = FALSE;

	/* Most importantly - see if the target we're tracking is dead! */
	if(trackingCamera.target->died)
	{
		setFindNewTarget();
		return(FALSE);
	}

	/*	Cancel tracking if it's no longer selected.
		This may not be desirable? 	*/
   	if(trackingCamera.target->type == OBJ_DROID)
	{

//		if(!trackingCamera.target->selected)
//		{
//			return(FALSE);
//		}
	}

	/* Update the acceleration,velocity and position of the camera for movement */
	updateCameraAcceleration(CAM_ALL);
	updateCameraVelocity(CAM_ALL);
	updateCameraPosition(CAM_ALL);

	/* Update the acceleration,velocity and rotation of the camera for rotation */
	/*	You can track roll as well (z axis) but it makes you ill and looks 
		like a flight sim, so for now just pitch and orientation */

	
	if(trackingCamera.target->type == OBJ_DROID)
	{
		psDroid = (DROID*)trackingCamera.target;
		psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
		if (psPropStats->propulsionType == LIFT)
		{
				bFlying = TRUE;
		}
	}
/*	
	bIsBuilding = FALSE;
	if(trackingCamera.target->type == OBJ_DROID)
	{
		psDroid= (DROID*)trackingCamera.target;
		if(DroidIsBuilding(psDroid))
		{
			bIsBuilding = TRUE;
		}
	}
*/
	
	
	if(bRadarAllign OR trackingCamera.target->type == OBJ_DROID)
	{
		if(bFlying)
		{
			updateCameraRotationAcceleration(CAM_ALL);
		}
		else
		{
			updateCameraRotationAcceleration(CAM_X_AND_Y);
		}
	}
	if(bFlying)
	{
	 	updateCameraRotationVelocity(CAM_ALL);
		updateCameraRotationPosition(CAM_ALL);
	}
	/*
	else if(bIsBuilding)
	{
		updateCameraRotationVelocity(CAM_X_ONLY);
	}
	*/
	else
	{
		updateCameraRotationVelocity(CAM_X_AND_Y);
		updateCameraRotationPosition(CAM_X_AND_Y);
	}

	/* Record the old positions for comparison */
	oldPosition.x = player.p.x;
	oldPosition.y = player.p.y;
	oldPosition.z = player.p.z;

	/* Update the position that's now stored in trackingCamera.position (iVector) */
	player.p.x = trackingCamera.position.x;
	player.p.y = trackingCamera.position.y;
	player.p.z = trackingCamera.position.z;

	/* Record the old positions for comparison */
	oldRotation.x = player.r.x;
	oldRotation.y = player.r.y;
	oldRotation.z = player.r.z;

	/* Update the rotations that're now stored in trackingCamera.rotation (iVector) */
	player.r.x = trackingCamera.rotation.x;
	/*if(!bIsBuilding)*/	
	player.r.y = trackingCamera.rotation.y;
	player.r.z = trackingCamera.rotation.z;

	/* There's a minimum for this - especially when John's VTOL code lets them land vertically on cliffs */
	if(player.r.x>DEG(360+MAX_PLAYER_X_ANGLE))
  	{
   		player.r.x = DEG(360+MAX_PLAYER_X_ANGLE);
   	}

	/*
	if(bIsBuilding)
	{
		player.r.y+=DEG(1);
	}
	*/
	/* Clip the position to the edge of the map */
	CheckScrollLimits();

	/* Store away our last update as acceleration and velocity are all fn()/dt */
	trackingCamera.lastUpdate = gameTime2;
	if(bFullInfo)
	{
		flushConsoleMessages();
		if(trackingCamera.target->type == OBJ_DROID)
		{
			printDroidInfo((DROID*)trackingCamera.target);
		}
	}

	/* Switch off if we're jumping to a new location and we've got there */
	if(getRadarTrackingStatus())
	{
		/*	This will ensure we come to a rest and terminate the tracking
			routine once we're close enough 
		*/
		if(getRotationMagnitude()<10000)
		{
			if(nearEnough() AND getPositionMagnitude() < 60)
			{
				camToggleStatus();
			}
		}
	}
	return(TRUE);
}
//-----------------------------------------------------------------------------------
#define	LEADER_LEFT			1
#define	LEADER_RIGHT		2
#define	LEADER_UP			3
#define	LEADER_DOWN			4
#define LEADER_STATIC		5

void	processLeaderSelection( void )
{
DROID	*psDroid;
DROID	*psPresent;
DROID	*psNew=NULL;
UDWORD	leaderClass;
BOOL	bSuccess;
UDWORD	dif;
UDWORD	bestSoFar;

	if(demoGetStatus())
	{
		return;
	}

	if(getWarCamStatus())
	{
		/* Only do if we're tracking a droid */
		if(trackingCamera.target->type != OBJ_DROID)
		{
			return;
		}
	}
	else
	{
		return;
	}

	/* Don't do if we're driving?! */
	if(getDrivingStatus())
	{
		return;
	}

	psPresent = (DROID*)trackingCamera.target;

	if(keyPressed(KEY_LEFTARROW))
	{
		leaderClass = LEADER_LEFT;		
	}
	
	else if(keyPressed(KEY_RIGHTARROW))
	{
		leaderClass = LEADER_RIGHT;		
	}

	else if(keyPressed(KEY_UPARROW))
	{
		leaderClass = LEADER_UP;		
	}

	else if(keyPressed(KEY_DOWNARROW))
	{
		leaderClass = LEADER_DOWN;		
	}
	else
	{
		leaderClass = LEADER_STATIC;
	}

	bSuccess = FALSE;
	bestSoFar = UDWORD_MAX;
	switch(leaderClass)
	{
	case	LEADER_LEFT:
		for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid=psDroid->psNext)
		{
			/* Is it even on the sscreen? */
			if(DrawnInLastFrame(psDroid->sDisplay.frameNumber) AND psDroid->selected AND psDroid!=psPresent)
			{
				if(psDroid->sDisplay.screenX<psPresent->sDisplay.screenX)
				{
					dif = psPresent->sDisplay.screenX - psDroid->sDisplay.screenX;
					if(dif<bestSoFar)
					{
						bestSoFar = dif;
						bSuccess = TRUE;
						psNew = psDroid;
					}
				}
			}
		}
		break;
	case	LEADER_RIGHT:
		for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid=psDroid->psNext)
		{
			/* Is it even on the sscreen? */
			if(DrawnInLastFrame(psDroid->sDisplay.frameNumber) AND psDroid->selected AND psDroid!=psPresent)
			{
				if(psDroid->sDisplay.screenX>psPresent->sDisplay.screenX)
				{
					dif = psDroid->sDisplay.screenX - psPresent->sDisplay.screenX;
					if(dif<bestSoFar)
					{
						bestSoFar = dif;
						bSuccess = TRUE;
						psNew = psDroid;
					}
				}
			}
		}
		break;
	case	LEADER_UP:
		for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid=psDroid->psNext)
		{
			/* Is it even on the sscreen? */
			if(DrawnInLastFrame(psDroid->sDisplay.frameNumber) AND psDroid->selected AND psDroid!=psPresent)
			{
				if(psDroid->sDisplay.screenY<psPresent->sDisplay.screenY)
				{
					dif = psPresent->sDisplay.screenY - psDroid->sDisplay.screenY;
					if(dif<bestSoFar)
					{
						bestSoFar = dif;
						bSuccess = TRUE;
						psNew = psDroid;
					}
				}
			}
		}
		break;
	case	LEADER_DOWN:
		for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid=psDroid->psNext)
		{
			/* Is it even on the sscreen? */
			if(DrawnInLastFrame(psDroid->sDisplay.frameNumber) AND psDroid->selected AND psDroid!=psPresent)
			{
				if(psDroid->sDisplay.screenY>psPresent->sDisplay.screenY)
				{
					dif = psDroid->sDisplay.screenY - psPresent->sDisplay.screenY;
					if(dif<bestSoFar)
					{
						bestSoFar = dif;
						bSuccess = TRUE;
						psNew = psDroid;
					}
				}
			}
		}
		break;
	case	LEADER_STATIC:
		break;
	}
	if(bSuccess)
	{
		camAllignWithTarget((BASE_OBJECT*)psNew);
	}
}
//-----------------------------------------------------------------------------------
DROID *getTrackingDroid( void )
{
	if(!getWarCamStatus()) return(NULL);
	if(trackingCamera.status != CAM_TRACKING) return(NULL);
	if(trackingCamera.target->type != OBJ_DROID) return(NULL);
	return((DROID*)trackingCamera.target);
}

//-----------------------------------------------------------------------------------
SDWORD	getGroupAverageTrackAngle(UDWORD groupNumber, BOOL bCheckOnScreen )
{
DROID	*psDroid;
FRACT	xShift,yShift;
FRACT	xTotal,yTotal;
FRACT	averageAngleFloat;
SDWORD	droidCount, averageAngle;
SDWORD	retVal;

	/* Initialise all the stuff */
	droidCount = 0;
	averageAngle = 0;
	/* Set totals to zero */
	xTotal = yTotal = 0.0f;

	/* Got thru' all droids */
	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		/* Is he worth considering? */
		if(psDroid->group == groupNumber)
		{
			if(bCheckOnScreen ? droidOnScreen(psDroid,DISP_WIDTH/6) : TRUE)
			{
					droidCount++;
					averageAngle+=psDroid->direction;
					xShift = trigSin(psDroid->direction);
					yShift = trigCos(psDroid->direction);
					xTotal += xShift;
					yTotal += yShift;
			}
			/*
			if(bCheckOnScreen)
			{
				if(droidOnScreen(psDroid,DISP_WIDTH/6))
				{
					droidCount++;
					averageAngle+=psDroid->direction;
					xShift = trigSin(psDroid->direction);
					yShift = trigCos(psDroid->direction);
					xTotal += xShift;
					yTotal += yShift;

				}
			}
			else
			{
					droidCount++;
					averageAngle+=psDroid->direction;
					xShift = trigSin(psDroid->direction);
					yShift = trigCos(psDroid->direction);
					xTotal += xShift;
					yTotal += yShift;


			}
			*/
		}
	}
	if(droidCount)												     
	{
		retVal = (averageAngle/droidCount);
		averageAngleFloat = RAD_TO_DEG(atan2(xTotal,yTotal));
	}
	else
	{
		retVal = 0;
	}
	presAvAngle = MAKEINT(averageAngleFloat);//retVal;
	return(presAvAngle);
}

//-----------------------------------------------------------------------------------
SDWORD	getAverageTrackAngle( BOOL bCheckOnScreen )
{
DROID	*psDroid;
FRACT	xShift,yShift;
FRACT	xTotal,yTotal;
FRACT	averageAngleFloat;
SDWORD	droidCount, averageAngle;
SDWORD	retVal;

	/* Initialise all the stuff */
	droidCount = 0;
	averageAngle = 0;
	/* Set totals to zero */
	xTotal = yTotal = 0.0f;

	/* Got thru' all droids */
	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		/* Is he worth selecting? */
		if(psDroid->selected)
		{
			if(bCheckOnScreen ? droidOnScreen(psDroid,DISP_WIDTH/6) : TRUE)
			{
					droidCount++;
					averageAngle+=psDroid->direction;
					xShift = trigSin(psDroid->direction);
					yShift = trigCos(psDroid->direction);
					xTotal += xShift;
					yTotal += yShift;
			}
			/*
			if(bCheckOnScreen)
			{
				if(droidOnScreen(psDroid,DISP_WIDTH/6))
				{
			 		droidCount++;
					averageAngle+=psDroid->direction;
					xShift = trigSin(psDroid->direction);
					yShift = trigCos(psDroid->direction);
					xTotal += xShift;
					yTotal += yShift;

				}
			}
			else
			{
					droidCount++;
					averageAngle+=psDroid->direction;
					xShift = trigSin(psDroid->direction);
					yShift = trigCos(psDroid->direction);
					xTotal += xShift;
					yTotal += yShift;


			}
			*/
		}
	}
	if(droidCount)
	{
		retVal = (averageAngle/droidCount);
		averageAngleFloat = (float)RAD_TO_DEG(atan2(xTotal,yTotal));
	}
	else
	{
		retVal = 0;
	}
	presAvAngle = MAKEINT(averageAngleFloat);//retVal;
	return(presAvAngle);
}

//-----------------------------------------------------------------------------------
SDWORD	getPresAngle( void )
{
	return(presAvAngle);
}
//-----------------------------------------------------------------------------------

#ifdef WIN32
//-----------------------------------------------------------------------------------
UDWORD	getNumDroidsSelected( void )
{
	return(selNumSelected(selectedPlayer));
/*
DROID	*psDroid;
UDWORD	count;

	for(psDroid = apsDroidLists[selectedPlayer],count = 0;
		psDroid; psDroid = psDroid->psNext)
	{
		if(psDroid->selected)
		{
			count++;
		}
	}
	return(count);
*/
}
#endif
//-----------------------------------------------------------------------------------
void	getTrackingConcerns(SDWORD *x,SDWORD *y, SDWORD *z)
{
SDWORD	xTotals,yTotals,zTotals;
DROID	*psDroid;
UDWORD	count;

	xTotals = yTotals = zTotals = 0;
	for(count = 0, psDroid = apsDroidLists[selectedPlayer]; 
		psDroid; psDroid = psDroid->psNext)
		{
			if(psDroid->selected)
			{
				if(droidOnScreen(psDroid,DISP_WIDTH/4))
				{
					count++;
					xTotals+=psDroid->x;
					yTotals+=psDroid->z;	// note the flip
					zTotals+=psDroid->y;
				}
			}
		}

	if(count)	// necessary!!!!!!!
	{
		*x = xTotals/count;
		*y = yTotals/count;
		*z = zTotals/count;
	}
}
//-----------------------------------------------------------------------------------
void	getGroupTrackingConcerns(SDWORD *x,SDWORD *y, SDWORD *z,UDWORD groupNumber, BOOL bOnScreen)
{
SDWORD	xTotals,yTotals,zTotals;
DROID	*psDroid;
UDWORD	count;

	xTotals = yTotals = zTotals = 0;
	for(count = 0, psDroid = apsDroidLists[selectedPlayer]; 
		psDroid; psDroid = psDroid->psNext)
		{
			if(psDroid->group == groupNumber)
			{
				if(bOnScreen ? droidOnScreen(psDroid,DISP_WIDTH/4) : TRUE)
				{
//					if(droidOnScreen(psDroid,DISP_WIDTH/4))
//					{
				 		count++;
						xTotals+=psDroid->x;
						yTotals+=psDroid->z;	// note the flip
						zTotals+=psDroid->y;
//					}
				}
//				else
//				{
//						count++;
//						xTotals+=psDroid->x;
//						yTotals+=psDroid->z;	// note the flip
//						zTotals+=psDroid->y;
//				}
			}
		}

	if(count)	// necessary!!!!!!!
	{
		*x = xTotals/count;
		*y = yTotals/count;
		*z = zTotals/count;
	}
}
//-----------------------------------------------------------------------------------

#else	// Start of PSX version.

static iVector targPos;
static iVector targRot;
static SDWORD targDist;
static UWORD OrbitAngle;

#define VIEWDIST	(1000)
#define VIEWRAYDIST (1500)
#define VIEWALT		(200)
#define VLENGTH(a,b) ((UDWORD)(a)/2+(UDWORD)(b)/2)
#define	VIEWRAYSTEP	(64)
#define	VIEWDAMP 	(8)
#define VIEWTOPDOWNANG	(270)
#define VIEWTOPDOWNDIST	(2048)
#define VIEW45ANG		(315)
#define VIEWHEIGHT		(300) // Was 200 but the wind changed
#define MAX_CAM_DELTA	(256)
#define MAX_CAMROT_BEHIND_DELTA (2048)
#define MAX_CAMROT_BEHIND_SLOWDELTA (512)
#define MAX_CAMROT_TOPDOWN_DELTA (16384)
#define MAX_CAMROT_ORBIT_DELTA (16384)
#define MAX_CAMROT_USER_DELTA (16384)

#define VIEW_MIN_PITCH	(0)
#define VIEW_MAX_PITCH	(16384)
#define VIEW_MIN_DIST	(0)	//(1000)
#define VIEW_MAX_DIST	(1000)	//(2400)

#define ROTACC			(128*4*2)
#define MAX_ROTVEL		(1024*4)
#define PITCHACC		(64*4*2)
#define MAX_PITCHVEL	(512*4)
#define SCROLLACC		(64*2)
#define MAX_SCROLLVEL	(1024)
#define ZOOMACC			(16*2)
#define MAX_ZOOMVEL		(64)

static UWORD CameraMode = CAMMODE_INACTIVE;
static UWORD CameraControlMode = CAMCONMODE_PITCH | CAMCONMODE_YAW | CAMCONMODE_DISTANCE;
static SWORD UserPitch = 0;
static SWORD UserYaw = 0;
static int UserDistance = 0;
static int NoInputTime = 0;
static int ViewPosDamp = VIEWDAMP;
static int ViewRotDamp = VIEWDAMP;
static int ViewDistDamp = VIEWDAMP;


DROID *getTrackingDroid( void )
{
	if(!getWarCamStatus()) return(NULL);
	if(trackingCamera.status != CAM_TRACKING) return(NULL);
	return((DROID*)trackingCamera.target);
}


// Initialise statics.
//
void camInitVars(void)

{
	DBPRINTF(("** camInitVars **\n"));
	OldViewValid = FALSE;
	CameraControlMode = CAMCONMODE_PITCH | CAMCONMODE_YAW | CAMCONMODE_DISTANCE;
	camSetMode(CAMMODE_BEHIND);
	camResetDamping();
}


void camResetDamping(void)
{
	ViewPosDamp = VIEWDAMP;
	ViewRotDamp = VIEWDAMP;
	ViewDistDamp = VIEWDAMP;
}


void camSetDamping(int PosDamp,int RotDamp,int DistDamp)
{
	ViewPosDamp = PosDamp;
	ViewRotDamp = RotDamp;
	ViewDistDamp = DistDamp;
}


void camGetDamping(int *PosDamp,int *RotDamp,int *DistDamp)
{
	if(PosDamp != NULL) {
		*PosDamp = ViewPosDamp;
	}
	if(RotDamp != NULL) {
		*RotDamp = ViewRotDamp;
	}
	if(DistDamp != NULL) {
		*DistDamp = ViewDistDamp;
	}
}

static BASE_OBJECT PanTarget;
static int PanTargetDistance;


void camPanToLocation(int x,int y)
{
	DBPRINTF(("camPanToLocation(%d %d)\n",x,y));


	PanTarget.x = x;
	PanTarget.y = y;
	if( (x<0) OR (y<0) OR (x > (SDWORD)((mapWidth-1)*TILE_UNITS)) OR (y > (SDWORD)((mapHeight-1)*TILE_UNITS)) )
	{
		PanTarget.z = 128 * ELEVATION_SCALE;
	}
	else
	{
		PanTarget.z = map_Height(x,y);
	}

	// Only set new target rotation and distance if were not interupting a current pan.
	if(CameraMode != CAMMODE_PANTOLOCATION) {
		PanTarget.direction = player.r.y;
		PanTarget.pitch = player.r.x;
		PanTarget.roll = player.r.z;
		PanTargetDistance = getViewDistance();
	}
	PanTarget.type = OBJ_TARGET;
	PanTarget.died = 0;

	camAllignWithTarget(&PanTarget);
	CameraMode = CAMMODE_PANTOLOCATION;

 	setWarCamActive(TRUE);
}


// Set the camera mode, top down,behind object or orbit object.
//
void camSetMode(UWORD CamMode)
{
	if(CameraMode != CamMode) {
		CameraMode = CamMode;
		OrbitAngle = 0;

		switch(CameraMode) {
			case CAMMODE_TOPDOWN:
				UserPitch = VIEW_MAX_PITCH;
				UserYaw = 0;
				UserDistance = 0;
				NoInputTime = 0;
				break;
			case CAMMODE_BEHIND:
			case CAMMODE_ORBIT:
				UserPitch = 0;
				UserYaw = 0;
				UserDistance = 0;
				NoInputTime = 0;
				break;
		}
	}
}


void camSetControlMode(UWORD CamConMode)
{
	CameraControlMode = CamConMode;
}


UWORD camGetControlMode(void)
{
	return CameraControlMode;
}


// Return the current camera mode, one of the CAMMODE_? enum.
//
UWORD camGetMode(void)
{
	return CameraMode;
}


// Return TRUE if the user is playing with the view controls.
//
BOOL camViewControlsActive(void)
{
	if( VPadDown(VPAD_ROTLEFT) || 
		VPadDown(VPAD_ROTRIGHT) ||
		VPadDown(VPAD_PITCHUP) ||
		VPadDown(VPAD_PITCHDOWN) ||
		VPadDown(VPAD_ZOOMIN) ||
		VPadDown(VPAD_ZOOMOUT) ) {
		return TRUE;
	}

	return FALSE;
}


// Given a value, cap it to the specified range.
//
SDWORD ForceValRange(SDWORD Val,SDWORD Min,SDWORD Max)
{
	if(Val < Min) {
		return Min;
	} else if(Val > Max) {
		return Max;
	}

	return Val;
}


// Update the camera.
//
BOOL camTrackCamera(void)
{
	SDWORD DeltaX;
	SDWORD DeltaZ;
	SDWORD DeltaY;
	SDWORD DeltaD;
	SDWORD DeltaRX;
	SDWORD DeltaRZ;
	SDWORD DeltaRY;
	SWORD Pitch,camPitch,TPitch;
	SDWORD ViewDist = VIEWDIST;
	SDWORD curDist;
 	int xRotVel = 0;
	int yRotVel = 0;
	int xVel = 0;
	int yVel = 0;
	int dVel = 0;
	BOOL ABut0,ABut1;
	SWORD Direction;
	SDWORD MaxDelta = MAX_CAM_DELTA;
	SDWORD MaxRotDelta;

	/* Most importantly - see if the target we're tracking is dead! */
	if(trackingCamera.target->died)
	{
		return(FALSE);
	}

	/*	Cancel tracking if it's no longer selected.
		This may not be desirable? 	*/
   	if(trackingCamera.target->type == OBJ_DROID)
	{
		if( (!trackingCamera.target->selected) && 
			(!(bTrackingTransporter && (((DROID*)trackingCamera.target)->droidType == DROID_TRANSPORTER) )) )
		{
printf("camTrackCamera : NO\n");
			return(FALSE);
		}
	}

	/* Record the old positions for comparison */
	oldPosition.x = player.p.x;
	oldPosition.y = player.p.y;
	oldPosition.z = player.p.z;


	// Handle camera user control.
	if((CameraControlMode) && !gamePaused()) {
		if(GetControllerType(0) == CON_ANALOG) {
			GetAnalogVelocities(0,&xVel,&yVel,&ABut0,&yRotVel,&xRotVel,&ABut1);

			if(ABut1)
			{
				dVel = xRotVel;
				xRotVel = yRotVel = 0;
			}

			yRotVel *= 16;
			xRotVel *= 2;
			yRotVel = -yRotVel;

		} if(GetControllerType(0) == CON_MOUSE) {
			if(MouseRightDown()) {
				GetMouseVel(0,&yRotVel,&xRotVel);
				yRotVel *= 128;
				xRotVel *= 96;
			}
		} else {
			BOOL Action = FALSE;

			Action |= ConVel(VPAD_ROTRIGHT,VPAD_ROTLEFT,&yRotVel,ROTACC,MAX_ROTVEL);
			Action |= ConVel(VPAD_PITCHUP,VPAD_PITCHDOWN,&xRotVel,PITCHACC,MAX_PITCHVEL);
			Action |= ConVel(VPAD_SCROLLRIGHT,VPAD_SCROLLLEFT,&xVel,SCROLLACC,MAX_SCROLLVEL);
			Action |= ConVel(VPAD_SCROLLDOWN,VPAD_SCROLLUP,&yVel,SCROLLACC,MAX_SCROLLVEL);
			Action |= ConVel(VPAD_ZOOMOUT,VPAD_ZOOMIN,&dVel,ZOOMACC,MAX_ZOOMVEL);
		}

		if((xRotVel == 0) && (yRotVel == 0) && (dVel == 0)) {
			NoInputTime++;
			// If there's been no user input for a while
			if(NoInputTime == 16) {
				if( CameraMode == CAMMODE_TOPDOWN ||
					CameraMode == CAMMODE_RESTORE ||
					CameraMode == CAMMODE_PANTOLOCATION) {
//					UserPitch = VIEW_MAX_PITCH;
//					UserYaw = 0;
				} else {
					// Reset user pitch and yaw.
#if(0)
					UserPitch = 0;
					UserYaw = 0;
					UserDistance = 0;
#else
// Rather than just set to 0, decrement over a period of time.
					if(UserPitch > 0) {
						UserPitch -= 512;
						if(UserPitch < 0) {
							UserPitch = 0;
						}
					} else if(UserPitch < 0) {
						UserPitch += 512;
						if(UserPitch > 0) {
							UserPitch = 0;
						}
					}

					if(UserYaw > 0) {
						UserYaw -= 512;
						if(UserYaw < 0) {
							UserYaw = 0;
						}
					} else if(UserYaw < 0) {
						UserYaw += 512;
						if(UserYaw > 0) {
							UserYaw = 0;
						}
					}

					UserDistance = 0;

					if((UserYaw != 0) || (UserPitch != 0)) {
						NoInputTime = 15;
					}
#endif
//					MaxRotDelta = 64;	//MAX_CAMROT_DELTA/8;
				}
			}
			if(NoInputTime > 16) {
				NoInputTime = 17;
			}
		} else {
			// User input so set fast rotation delta.
//			MaxRotDelta = MAX_CAMROT_DELTA;
			NoInputTime = 0;
		}

		if(CameraControlMode & CAMCONMODE_YAW) {
//			UserYaw += yRotVel;
			UserYaw -= yRotVel;
			UserYaw = UserYaw & 65535;
		}

		if(CameraControlMode & CAMCONMODE_PITCH) {
			UserPitch -= xRotVel;

			if(UserPitch > VIEW_MAX_PITCH) {
				UserPitch = VIEW_MAX_PITCH;
			}

			if(CameraMode == CAMMODE_TOPDOWN) {
				if(UserPitch < 4618) {
					UserPitch = 4618;
				}
			} else {
				if(UserPitch < VIEW_MIN_PITCH) {
					UserPitch = VIEW_MIN_PITCH;
				}
			}
		}

		if(CameraControlMode & CAMCONMODE_DISTANCE) {
			UserDistance += dVel;
			if(UserDistance < VIEW_MIN_DIST) {
				UserDistance = VIEW_MIN_DIST;
			} else if(UserDistance > VIEW_MAX_DIST) {
				UserDistance = VIEW_MAX_DIST;
			}
		}
	}

	switch(CameraMode) {
		// Camera following an object.
		case	CAMMODE_BEHIND:
			MaxRotDelta = MAX_CAMROT_BEHIND_DELTA;
			if((UserPitch != 0) || (UserYaw !=0)) {
				MaxRotDelta = MAX_CAMROT_BEHIND_SLOWDELTA;
			}

			camPitch = (SDWORD)angle_PSX2World( -ratan2(VIEWHEIGHT+trackingCamera.target->z/2, ViewDist) );
//			camPitch = (SDWORD)angle_PSX2World( -ratan2(VIEWHEIGHT, ViewDist) );

			if(CameraControlMode & CAMCONMODE_YAW) {
				Direction = (trackingCamera.target->direction + ((UserYaw*360)/65536)) %360;
			} else {
				Direction = trackingCamera.target->direction;
			}

			getBestPitchToEdgeOfGrid(trackingCamera.target->x,trackingCamera.target->y,
									360-(((player.r.y*360)>>16)%360),&Pitch);

//printf("%d %d %d : ",trackingCamera.target->x,trackingCamera.target->y,trackingCamera.target->z);
//printf("%d %d\n",camPitch,Pitch);

			if(CameraControlMode & CAMCONMODE_PITCH) {
				Pitch += (UserPitch*360)/65536;
			}

			Pitch += 10;
			Pitch = -Pitch;
			if(Pitch < camPitch) {
				camPitch = Pitch;
			}

			if(camPitch < -90) {
				camPitch = -90;
			}

			// Varie the view distance according to the view pitch.
			TPitch = -(camPitch+12);
			if(TPitch < 0) {
				TPitch = 0;
			}
			targDist = VIEWDIST+TPitch*16 + UserDistance;
			if(targDist > 2400) {
				targDist = 2400;
				UserDistance = 2400-VIEWDIST+TPitch*16;
			}

			camPitch = DEG(camPitch);
			targRot.y = DEG(Direction-180);
//			targRot.y = DEG(trackingCamera.target->direction-180);

		//	DBPRINTF(("%d,%d\n",Pitch+10,ViewDist);
			// Position camera over object.
			targPos.x = trackingCamera.target->x-(VISIBLE_XTILES*TILE_UNITS)/2;
			targPos.y = -(trackingCamera.target->z);
			targPos.z = trackingCamera.target->y-(VISIBLE_YTILES*TILE_UNITS)/2;
			break;

		case	CAMMODE_TOPDOWN:
			// Camera top down view.
			MaxRotDelta = MAX_CAMROT_TOPDOWN_DELTA;

			targDist = VIEWTOPDOWNDIST + UserDistance;
			if(targDist > 2400) {
				targDist = 2400;
				UserDistance = 2400-VIEWTOPDOWNDIST;
			}

			Direction = 0;

			if(CameraControlMode & CAMCONMODE_YAW) {
				Direction = ((UserYaw*360)/65536)%360;
			}

			if(CameraControlMode & CAMCONMODE_PITCH) {
				camPitch = DEG(((-UserPitch*360)/65536)%360);
//				DBPRINTF(("%d %d\n",UserPitch);
				if(camPitch > -24) {
					camPitch = -24;
				}
			} else {
				camPitch = VIEWTOPDOWNANG;
			}

			targRot.y = DEG(Direction);

//			DBPRINTF(("cp %d\n",camPitch);
			// Position camera over object.
			targPos.x = trackingCamera.target->x-(VISIBLE_XTILES*TILE_UNITS)/2;
			targPos.y = -(trackingCamera.target->z);
			targPos.z = trackingCamera.target->y-(VISIBLE_YTILES*TILE_UNITS)/2;
			break;

		case	CAMMODE_ORBIT:
			// Camera orbiting an object.
			MaxRotDelta = MAX_CAMROT_ORBIT_DELTA;

			camPitch = DEG(VIEW45ANG);
			targDist = VIEWTOPDOWNDIST;
			targRot.y = DEG(OrbitAngle % 360);
			OrbitAngle += 2;
			// Position camera over object.
			targPos.x = trackingCamera.target->x-(VISIBLE_XTILES*TILE_UNITS)/2;
			targPos.y = -(trackingCamera.target->z);
			targPos.z = trackingCamera.target->y-(VISIBLE_YTILES*TILE_UNITS)/2;
			break;

		case	CAMMODE_RESTORE:
			MaxRotDelta = MAX_CAMROT_TOPDOWN_DELTA;
			camPitch = trackingCamera.oldView.r.x;	//DEG(330);
			targRot.y = trackingCamera.oldView.r.y;	//DEG(330);
			targDist = trackingCamera.oldDistance;	//1700;
//			ViewRotDamp = 4;
//			ViewDistDamp = 2;
//			ViewPosDamp = 2;

			// Position camera over object.
			targPos.x = trackingCamera.target->x-(VISIBLE_XTILES*TILE_UNITS)/2;
			targPos.y = -(trackingCamera.target->z);
			targPos.z = trackingCamera.target->y-(VISIBLE_YTILES*TILE_UNITS)/2;
			break;

		case	CAMMODE_PANTOLOCATION:
			MaxRotDelta = MAX_CAMROT_ORBIT_DELTA;
			camPitch = PanTarget.pitch;
			targRot.y = PanTarget.direction;
			targRot.z = PanTarget.roll;
			targDist = PanTargetDistance;

			// Position camera over object.
			targPos.x = PanTarget.x-(VISIBLE_XTILES*TILE_UNITS)/2;
			targPos.y = -(PanTarget.z);
			targPos.z = PanTarget.y-(VISIBLE_YTILES*TILE_UNITS)/2;

//			ViewRotDamp = 4;
//			ViewDistDamp = 2;
//			ViewPosDamp = 2;
			break;

		default:
//			DBPRINTF(("Unknown camera mode\n"));
			MaxRotDelta = MAX_CAMROT_BEHIND_DELTA;
			camPitch = DEG(VIEW45ANG);
	}

	// If the target is along way from the current position then
	// pitch the camera down and pull back.
	if(	CameraMode == CAMMODE_BEHIND ) {
		BOOL CheckSpeed = TRUE; 

		// Don't do this if it's a vtol or a transporter.
		if(trackingCamera.target->type == OBJ_DROID) {
			DROID *psDroid = trackingCamera.target;
			PROPULSION_STATS *psPropStats;

			psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
			if (psPropStats->propulsionType == LIFT) {
				CheckSpeed = FALSE;
			}
		}

		if (CheckSpeed) {

			DeltaX = abs(targPos.x - player.p.x);
			DeltaY = abs(targPos.z - player.p.z);
			if((DeltaX + DeltaY) > 512) {
				camPitch = DEG(VIEWTOPDOWNANG);
				targDist = VIEWTOPDOWNDIST;
			}
		}
	}

	// Rotate to view from behind object.
	targRot.x = camPitch;	//DEG(camPitch);
	targRot.z = 0;

	if((xRotVel |= 0) || (yRotVel |= 0)) {
		MaxRotDelta = MAX_CAMROT_USER_DELTA;
	}

//	DBPRINTF(("%d\n",MaxRotDelta);

	// Set position with damping.
	DeltaX = ForceValRange((targPos.x - player.p.x)/ViewPosDamp,-MaxDelta,MaxDelta);
	DeltaY = ForceValRange((targPos.y - player.p.y)/ViewPosDamp,-MaxDelta,MaxDelta);
	DeltaZ = ForceValRange((targPos.z - player.p.z)/ViewPosDamp,-MaxDelta,MaxDelta);
	player.p.x += DeltaX;
	player.p.y += DeltaY;
	player.p.z += DeltaZ;

//printf("%d %d %d\n",DeltaX,DeltaY,DeltaZ);

//	if(!getRadarTrackingStatus()) {
		// Set rotation with damping.
		DeltaRX = ForceValRange((SWORD)((UWORD)targRot.x - (UWORD)player.r.x)/ViewRotDamp,-MaxRotDelta,MaxRotDelta);
		DeltaRY = ForceValRange((SWORD)((UWORD)targRot.y - (UWORD)player.r.y)/ViewRotDamp,-MaxRotDelta,MaxRotDelta);
		DeltaRZ = ForceValRange((SWORD)((UWORD)targRot.z - (UWORD)player.r.z)/ViewRotDamp,-MaxRotDelta,MaxRotDelta);
		player.r.x += DeltaRX;
		player.r.y += DeltaRY;
		player.r.z += DeltaRZ;

		// Set view distance with damping.
		curDist = getViewDistance();
		DeltaD = (targDist - curDist)/ViewDistDamp;
		curDist += DeltaD;
		setViewDistance(curDist);

		switch(CameraMode) {
			case	CAMMODE_RESTORE:
//				DBPRINTF(("%d %d %d %d\n",DeltaRX,DeltaRY,DeltaRZ,DeltaD));
				if( (abs(DeltaRX) + abs(DeltaRY) + abs(DeltaRZ) + abs(DeltaD))/4 == 0) {
					DBPRINTF(("View restored\n"));
					camResetDamping();
//					camSetOldView(player.p.x,player.p.y,player.p.z,
//								player.r.x,player.r.y,getViewDistance());
//								DEG(330),DEG(330),1700);
					camToggleStatus();
					CameraMode = CAMMODE_INACTIVE;
					DBPRINTF(("View restored done\n"));
				}
				break;

			case	CAMMODE_PANTOLOCATION:
//				DBPRINTF(("%d %d %d\n",DeltaX,DeltaY,DeltaZ));
				if( (abs(DeltaX) + abs(DeltaY) + abs(DeltaZ))/2 == 0) {
					DBPRINTF(("Target reached\n"));
					camResetDamping();
//					camSetOldView(	PanTarget.x,PanTarget.y,PanTarget.z,
//									PanTarget.pitch,PanTarget.direction,
//									PanTargetDistance);
//									trackingCamera.oldView.r.x,trackingCamera.oldView.r.y,
//									trackingCamera.oldDistance);
//					camSetOldView(	player.p.x,player.p.y,player.p.z,
//									player.r.x,player.r.y
//									getViewDistance());
					camToggleStatus();
					CameraMode = CAMMODE_INACTIVE;
					DBPRINTF(("Target reached done\n"));
				}
				break;
		}
//	}

//	// Switch off if we're jumping to a new location and we've got there.
//	if(getRadarTrackingStatus())
//	{
//		if(getPositionMagnitude() < 50) {
//			camToggleStatus();
//		}
//	}

	return TRUE;
}

#endif


void camSetOldView(int x,int y,int z,int rx,int ry,int dist)
{
	UNUSEDPARAMETER(x);
	UNUSEDPARAMETER(y);
	UNUSEDPARAMETER(z);
	UNUSEDPARAMETER(rx);
	UNUSEDPARAMETER(ry);
	UNUSEDPARAMETER(dist);

//DBPRINTF(("camSetOldView(%d %d %d %d %d %d)\n",x,y,z,rx,ry,dist));
//DBPRINTF(("%d %d %d %d %d %d\n",player.p.x,player.p.y,player.r.x,player.r.y,getViewDistance()));
//	trackingCamera.oldView.p.x = x;
//	trackingCamera.oldView.p.y = y;
//	trackingCamera.oldView.p.z = z;
//	trackingCamera.oldView.r.x = rx;
//	trackingCamera.oldView.r.y = ry;
//	trackingCamera.oldDistance = dist;
}


/* Static function that switches off tracking - and might not be desirable? - Jim?*/
void	camSwitchOff( void )
{
 	/* Restore the angles */
#ifdef PSX
	if(OldViewValid) {
		player.r.x = trackingCamera.oldView.r.x;
		player.r.y = trackingCamera.oldView.r.y;
		player.r.z = trackingCamera.oldView.r.z;
		setViewDistance(trackingCamera.oldDistance);
	}
#else
//	player.r.x = trackingCamera.oldView.r.x;
	player.r.z = trackingCamera.oldView.r.z;

	/* And height */
	/* Is this desirable??? */
//	player.p.y = trackingCamera.oldView.p.y;	

	/* Restore distance */
	setViewDistance(trackingCamera.oldDistance);
#endif
}

//-----------------------------------------------------------------------------------

/* Returns whether or not the tracking camera is active */
BOOL	getWarCamStatus( void )
{
	/* Is it switched off? */
	if(trackingCamera.status == CAM_INACTIVE)
	{
		return(FALSE);
	}
	else
	{
		/* Tracking is ON */
		return(TRUE);
	}
}

//-----------------------------------------------------------------------------------

/* Flips the status of tracking to the opposite of what it presently is */
void	camToggleStatus( void )
{
#ifdef PSX
	if(trackingCamera.status != CAM_INACTIVE) {
		if(bTrackingTransporter) {
			return;
		}
	}
#endif

 	/* If it's off */
	if(trackingCamera.status == CAM_INACTIVE)
	{
		/* Switch it on */
		setWarCamActive(TRUE);
	}
	else
	{
		/* Otherwise, switch it off */
		setWarCamActive(FALSE);
//		if(getDrivingStatus())
//		{
//			StopDriverMode();
//		}
	}
}


/*	Flips on/off whether we print out full info about the droid being tracked.
	If ON then this info is permanent on screen and realtime updating */
void	camToggleInfo(void)
{
	bFullInfo = !bFullInfo;
}

/* Sets up the dummy target for the camera */
//void	setUpRadarTarget(SDWORD x, SDWORD y)
void	setUpRadarTarget(SDWORD x, SDWORD y)
{

	radarTarget.x = x;
	radarTarget.y = y;
	if( (x<0) OR (y<0) OR (x > (SDWORD)((mapWidth-1)*TILE_UNITS)) OR (y > (SDWORD)((mapHeight-1)*TILE_UNITS)) )
	{
		radarTarget.z = 128 * ELEVATION_SCALE;
	}
	else
	{
		radarTarget.z = map_Height(x,y);
	}
	radarTarget.direction = (UWORD)calcDirection(player.p.x,player.p.z,x,y);
	radarTarget.pitch = 0;
	radarTarget.roll = 0;
	radarTarget.type = OBJ_TARGET;
	radarTarget.died = 0;
}

/* Informs the tracking camera that we want to start tracking to a new radar target */
void	requestRadarTrack(SDWORD x, SDWORD y)
{
/*	
	ASSERT((x<mapWidth*TILE_UNITS,"Weirdy x coordinate for tracking"));
	ASSERT((y<mapHeight*TILE_UNITS,"Weirdy y coordinate for tracking"));
*/
	
	radarX = (SWORD)x;
 	radarY = (SWORD)y;
 	bRadarTrackingRequested = TRUE;
	trackingCamera.status = CAM_REQUEST;
	processWarCam();
// 	setWarCamActive(TRUE);
}

/* Returns whether we're presently tracking to a new _location_ */
BOOL	getRadarTrackingStatus( void )
{
BOOL	retVal;
 
	if(trackingCamera.status == CAM_INACTIVE)
	{
		retVal = FALSE;
	}
	else
	{
		//if(/*trackingCamera.target && */trackingCamera.target->type == OBJ_TARGET)
        //if you know why the above check was commented out please tell me AB 19/11/98
        if(trackingCamera.target && trackingCamera.target->type == OBJ_TARGET)
		{
			retVal = TRUE;
		}
		else
		{
			retVal = FALSE;
		}
	}
	return(retVal);
}

/* Displays a spinning MTV style logo in the top right of the screen */
void	dispWarCamLogo( void )
{
#ifdef WIN32
//iVector		dv;
//
//	if(gamePaused())
//	{
//		/* get out if we're paused */
//		return;
//	}
//
//	warCamLogoRotation += MAKEINT( (MAKEFRACT(LOGO_ROT_SPEED) * fraction) );
//	dv.x = 280;
//	dv.y = 165;
//	dv.z = 1000;
//	iV_MatrixBegin();							/* Push the indentity matrix */
//	iV_TRANSLATE(dv.x,dv.y,dv.z);
//	scaleMatrix(15);
//	iV_MatrixRotateY(warCamLogoRotation);
//	iV_MatrixRotateX(player.r.x);
//
//	pie_Draw3DShape(cameraImd, 0, 0, pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);
//	iV_MatrixEnd();
#else	// PSX camera renderer.
//	iVector dv;
//	SWORD dummyscreen[2];
//	SVECTOR ZeroVector={0,0,0};
//	FRACT	fraction;
//	PIE PieParams;
//
//	PieParams.Flags = PIE_PALETTEID;
//	PieParams.PaletteID = 0;
//
//	fraction = (MAKEFRACT(frameTime) / (FRACT)GAME_TICKS_PER_SEC);
//
//	warCamLogoRotation += MAKEINT( ((FRACT)(LOGO_ROT_SPEED) * fraction) );
//	dv.x = 282;
//	dv.y = 165;
//	dv.z = 1000;
//
//	SetBSPObjectPos(dv.x,dv.y,dv.z);	// this isn't right is it ?
//	dv.y = -dv.y;
//
//	psxPushMatrix();
//	psxiV_MatrixBegin();
//
//    psxiV_TRANSLATE(dv.x,dv.y,dv.z);
//	scaleMatrix(15);
//	psxiV_MatrixRotateY(warCamLogoRotation);
//	psxiV_MatrixRotateX(player.r.x);
//
//	psxUseMatrix();
//
//	psxiv_SetIMDZ(psxiV_RotateProject(&ZeroVector,dummyscreen));
//   	DrawPie(cameraImd,&PieParams);
//	psxPopMatrix();
#endif
}

void	toggleRadarAllignment( void )
{
	bRadarAllign = !bRadarAllign;
}

/* Returns how far away we are from our goal in a radar track */
UDWORD	getPositionMagnitude( void )
{
iVector	dif;
UDWORD	val;

	dif.x = abs(player.p.x - oldPosition.x);
	dif.y = abs(player.p.y - oldPosition.y);
	dif.z = abs(player.p.z - oldPosition.z);
	val = (dif.x*dif.x) + (dif.y*dif.y) + (dif.z*dif.z);
	return(val);
}

/* Rteurns how far away we are from our goal in rotation */
UDWORD	getRotationMagnitude( void )
{
iVector	dif;
UDWORD	val;

	dif.x = abs(player.r.x - oldRotation.x);
	dif.y = abs(player.r.y - oldRotation.y);
	dif.z = abs(player.r.z - oldRotation.z);
	val = (dif.x*dif.x) + (dif.y*dif.y) + (dif.z*dif.z);
	return(val);
}

void	camInformOfRotation( iVector *rotation )
{
	trackingCamera.rotation.x = rotation->x;
	trackingCamera.rotation.y = rotation->y;
	trackingCamera.rotation.z = rotation->z;
}