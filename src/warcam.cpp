/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
 * @file warcam.c
 * Handles tracking/following of in game objects.
 */
/* Alex McLean, Pumpkin Studios, EIDOS Interactive, 1998 */
/*	23rd September, 1998 - This code is now so hideously complex
	and unreadable that's it's inadvisable to attempt changing
	how the camera works, since I'm not sure that I'll be able to even
	get it working the way it used to, should anything get broken.
	I really hope that no further changes are needed here...:-(
	Alex M. */

#include "lib/framework/frame.h"
#include "lib/framework/trig.h"
#include "lib/framework/input.h"

#include "lib/ivis_opengl/piematrix.h"
#include "lib/framework/fixedpoint.h" //ivis matrix code

#include "lib/gamelib/gtime.h"

#include "warcam.h"

#include "objects.h"
#include "display.h"
#include "display3d.h"
#include "hci.h"
#include "console.h"
#include "effects.h"
#include "map.h"
#include "geometry.h"
#include "oprint.h"
#include "miscimd.h"
#include "loop.h"
#include "drive.h"
#include "move.h"
#include "order.h"
#include "action.h"
#include "intdisplay.h"
#include "e3demo.h"
#include "display3d.h"
#include "selection.h"


#define MIN_TRACK_HEIGHT 16

/* Holds all the details of our camera */
static	WARCAM	trackingCamera;

/* Present rotation for the 3d camera logo */
static	SDWORD	warCamLogoRotation;

/* The fake target that we track when jumping to a new location on the radar */
static BASE_OBJECT radarTarget(OBJ_TARGET, 0, 0);

/* Do we trun to face when doing a radar jump? */
static	bool	bRadarAllign;

static SDWORD	presAvAngle = 0;;

/*	These are the DEFAULT offsets that make us track _behind_ a droid and allow
	it to be pretty far _down_ the screen, so we can see more
*/

/* Offset from droid's world coords */
/* How far we track relative to the droids location - direction matters */
#define	CAM_DEFAULT_OFFSET	-400
#define	MINCAMROTX	-20


/* How much info do you want when tracking a droid - this toggles full stat info */
static	bool bFullInfo = false;

/* Are we requesting a new track to start that is a radar (location) track? */
static	bool bRadarTrackingRequested = false;

/* World coordinates for a radar track/jump */
static  float	 radarX,radarY;

//-----------------------------------------------------------------------------------
/* Sets the camera to inactive to begin with */
void	initWarCam( void )
{
	/* We're not intitially following anything */
	trackingCamera.status = CAM_INACTIVE;

	/* Logo setup */
	warCamLogoRotation = 0;
}


/* Static function that switches off tracking - and might not be desirable? - Jim?*/
static void camSwitchOff( void )
{
 	/* Restore the angles */
//	player.r.x = trackingCamera.oldView.r.x;
	player.r.z = trackingCamera.oldView.r.z;

	/* And height */
	/* Is this desirable??? */
//	player.p.y = trackingCamera.oldView.p.y;

	/* Restore distance */
	setViewDistance(trackingCamera.oldDistance);
}


#define	LEADER_LEFT			1
#define	LEADER_RIGHT		2
#define	LEADER_UP			3
#define	LEADER_DOWN			4
#define LEADER_STATIC		5

static void processLeaderSelection( void )
{
	DROID *psDroid;
	DROID *psPresent;
	DROID *psNew = NULL;
	UDWORD leaderClass;
	bool bSuccess;
	UDWORD dif;
	UDWORD bestSoFar;

	if (demoGetStatus())
	{
		return;
	}

	if (getWarCamStatus())
	{
		/* Only do if we're tracking a droid */
		if (trackingCamera.target->type != OBJ_DROID)
		{
			return;
		}
	}
	else
	{
		return;
	}

	/* Don't do if we're driving?! */
	if (getDrivingStatus())
	{
		return;
	}

	psPresent = (DROID*)trackingCamera.target;

	if (keyPressed(KEY_LEFTARROW))
	{
		leaderClass = LEADER_LEFT;
	}

	else if (keyPressed(KEY_RIGHTARROW))
	{
		leaderClass = LEADER_RIGHT;
	}

	else if (keyPressed(KEY_UPARROW))
	{
		leaderClass = LEADER_UP;
	}

	else if (keyPressed(KEY_DOWNARROW))
	{
		leaderClass = LEADER_DOWN;
	}
	else
	{
		leaderClass = LEADER_STATIC;
	}

	bSuccess = false;
	bestSoFar = UDWORD_MAX;

	switch (leaderClass)
	{
	case	LEADER_LEFT:
		for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
		{
			/* Is it even on the sscreen? */
			if (DrawnInLastFrame(psDroid->sDisplay.frameNumber) && psDroid->selected && psDroid != psPresent)
			{
				if (psDroid->sDisplay.screenX < psPresent->sDisplay.screenX)
				{
					dif = psPresent->sDisplay.screenX - psDroid->sDisplay.screenX;
					if (dif < bestSoFar)
					{
						bestSoFar = dif;
						bSuccess = true;
						psNew = psDroid;
					}
				}
			}
		}
		break;
	case	LEADER_RIGHT:
		for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
		{
			/* Is it even on the sscreen? */
			if (DrawnInLastFrame(psDroid->sDisplay.frameNumber) && psDroid->selected && psDroid != psPresent)
			{
				if (psDroid->sDisplay.screenX > psPresent->sDisplay.screenX)
				{
					dif = psDroid->sDisplay.screenX - psPresent->sDisplay.screenX;
					if (dif < bestSoFar)
					{
						bestSoFar = dif;
						bSuccess = true;
						psNew = psDroid;
					}
				}
			}
		}
		break;
	case	LEADER_UP:
		for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
		{
			/* Is it even on the sscreen? */
			if (DrawnInLastFrame(psDroid->sDisplay.frameNumber) && psDroid->selected && psDroid != psPresent)
			{
				if (psDroid->sDisplay.screenY < psPresent->sDisplay.screenY)
				{
					dif = psPresent->sDisplay.screenY - psDroid->sDisplay.screenY;
					if (dif < bestSoFar)
					{
						bestSoFar = dif;
						bSuccess = true;
						psNew = psDroid;
					}
				}
			}
		}
		break;
	case	LEADER_DOWN:
		for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
		{
			/* Is it even on the sscreen? */
			if (DrawnInLastFrame(psDroid->sDisplay.frameNumber) && psDroid->selected && psDroid != psPresent)
			{
				if (psDroid->sDisplay.screenY > psPresent->sDisplay.screenY)
				{
					dif = psDroid->sDisplay.screenY - psPresent->sDisplay.screenY;
					if (dif < bestSoFar)
					{
						bestSoFar = dif;
						bSuccess = true;
						psNew = psDroid;
					}
				}
			}
		}
		break;
	case	LEADER_STATIC:
		break;
	}
	if (bSuccess)
	{
		camAllignWithTarget((BASE_OBJECT*)psNew);
	}
}


/* Sets up the dummy target for the camera */
static void setUpRadarTarget(SDWORD x, SDWORD y)
{
	radarTarget.pos.x = x;
	radarTarget.pos.y = y;

	if ((x < 0) || (y < 0) || (x > world_coord(mapWidth - 1))
	    || (y > world_coord(mapHeight - 1)))
	{
		radarTarget.pos.z = world_coord(1) * ELEVATION_SCALE + CAMERA_PIVOT_HEIGHT;
	}
	else
	{
		radarTarget.pos.z = map_Height(x,y) + CAMERA_PIVOT_HEIGHT;
	}
	radarTarget.rot.direction = calcDirection(player.p.x, player.p.z, x, y);
	radarTarget.rot.pitch = 0;
	radarTarget.rot.roll = 0;
}


/* Attempts to find the target for the camera to track */
static BASE_OBJECT *camFindTarget(void)
{
	/*	See if we can find a selected droid. If there's more than one
		droid selected for the present player, then we track the oldest
		one. */

	if (bRadarTrackingRequested)
	{
		setUpRadarTarget(radarX, radarY);
		bRadarTrackingRequested = false;
		return(&radarTarget);
	}

	return camFindDroidTarget();
}


bool camTrackCamera(void);

/* Updates the camera position/angle along with the object movement */
bool	processWarCam( void )
{
BASE_OBJECT	*foundTarget;
bool Status = true;

	/* Get out if the camera isn't active */
	if(trackingCamera.status == CAM_INACTIVE)
	{
		return(true);
	}

	/* Ensure that the camera only ever flips state within this routine! */
	switch(trackingCamera.status)
	{
	case CAM_REQUEST:

			/* See if we can find the target to follow */
			foundTarget = camFindTarget();

			if(foundTarget && !foundTarget->died)
			{
				/* We've got one, so store away info */
				camAllignWithTarget(foundTarget);
				/* We're now into tracking status */
				trackingCamera.status = CAM_TRACKING;
				/* Inform via console */
				if(foundTarget->type == OBJ_DROID)
				{
					if(getWarCamStatus())
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
//				addConsoleMessage("Droid-CAM V0.1 ERROR - No targets(s) selected",DEFAULT_JUSTIFY,SYSTEM_MESSAGE);
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
				if(foundTarget && !foundTarget->died)
				{
					trackingCamera.status = CAM_REQUEST;
				}
				else
				{
					trackingCamera.status = CAM_RESET;
				}
			}

		processLeaderSelection();

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
//			addConsoleMessage("Droid-CAM V0.1 Disabled",DEFAULT_JUSTIFY,SYSTEM_MESSAGE);
			Status = false;
		break;
	default:
		debug( LOG_FATAL, "Weirdy status for tracking Camera" );
		abort();
		break;
	}

	return Status;
}

//-----------------------------------------------------------------------------------

/* Flips states for camera active */
void	setWarCamActive(bool status)
{
	debug( LOG_NEVER, "setWarCamActive(%d)\n", status );

	/* We're trying to switch it on */
	if(status == true)
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

		if(psDroid->selected)

		{
			/* Return the first one found */
			return( (BASE_OBJECT*)psDroid);
		}
	}

	/* We didn't find one */
	return(NULL);
}


//-----------------------------------------------------------------------------------

/* Stores away old viewangle info and sets up new distance and angles */
void	camAllignWithTarget(BASE_OBJECT *psTarget)
{
	/* Store away the target */
	trackingCamera.target = psTarget;

	/* Save away all the view angles */
	trackingCamera.oldView.r.x = trackingCamera.rotation.x = (float)player.r.x;
	trackingCamera.oldView.r.y = trackingCamera.rotation.y = (float)player.r.y;
	trackingCamera.oldView.r.z = trackingCamera.rotation.z = (float)player.r.z;

	/* Store away the old positions and set the start position too */
	trackingCamera.oldView.p.x = trackingCamera.position.x = (float)player.p.x;
	trackingCamera.oldView.p.y = trackingCamera.position.y = (float)player.p.y;
	trackingCamera.oldView.p.z = trackingCamera.position.z = (float)player.p.z;

   //	trackingCamera.rotation.x = player.r.x = DEG(-90);
	/* No initial velocity for moving */
	trackingCamera.velocity.x = trackingCamera.velocity.y = trackingCamera.velocity.z = 0.f;
	/* Nor for rotation */
	trackingCamera.rotVel.x = trackingCamera.rotVel.y = trackingCamera.rotVel.z = 0.f;
	/* No initial acceleration for moving */
	trackingCamera.acceleration.x = trackingCamera.acceleration.y = trackingCamera.acceleration.z = 0.f;
	/* Nor for rotation */
	trackingCamera.rotAccel.x = trackingCamera.rotAccel.y = trackingCamera.rotAccel.z = 0.f;

	/* Sote the old distance */
	trackingCamera.oldDistance = getViewDistance();	//distance;

	/* Store away when we started */
	trackingCamera.lastUpdate = gameTime2;
}

#define GROUP_SELECTED 0xFFFFFFFF

//-----------------------------------------------------------------------------------
static uint16_t getAverageTrackAngle(unsigned groupNumber, bool bCheckOnScreen)
{
	DROID *psDroid;
	int32_t xTotal = 0, yTotal = 0;

	/* Got thru' all droids */
	for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		/* Is he worth selecting? */
		if (groupNumber == GROUP_SELECTED ? psDroid->selected : psDroid->group == groupNumber)
		{
			if (bCheckOnScreen ? droidOnScreen(psDroid, pie_GetVideoBufferWidth() / 6) : true)
			{
				xTotal += iSin(psDroid->rot.direction);
				yTotal += iCos(psDroid->rot.direction);
			}
		}
	}
	presAvAngle = iAtan2(xTotal, yTotal);
	return presAvAngle;
}


//-----------------------------------------------------------------------------------
static void getTrackingConcerns(SDWORD *x, SDWORD *y, SDWORD *z, UDWORD groupNumber, bool bOnScreen)
{
	SDWORD xTotals = 0, yTotals = 0, zTotals = 0;
	DROID *psDroid;
	UDWORD count;

	for (count = 0, psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (groupNumber == GROUP_SELECTED ? psDroid->selected : psDroid->group == groupNumber)
		{
			if (!bOnScreen || droidOnScreen(psDroid, pie_GetVideoBufferWidth() / 4))
			{
				count++;
				xTotals += psDroid->pos.x;
				yTotals += psDroid->pos.z;	// note the flip
				zTotals += psDroid->pos.y;
			}
		}
	}

	if (count)	// necessary!!!!!!!
	{
		*x = xTotals / count;
		*y = yTotals / count;
		*z = zTotals / count;
	}
}


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


static void updateCameraAcceleration(UBYTE update)
{
	Vector3i concern = swapYZ(trackingCamera.target->pos);
	Vector2i behind(0, 0); /* Irrelevant for normal radar tracking */
	bool bFlying = false;

	/*
		This is where we check what it is we're tracking.
		Were we to track a building or location - this is
		where it'd be set up
	*/
	/*
		If we're tracking a droid, then we need to track slightly in front
		of it in order that the droid appears down the screen a bit. This means
		that we need to find an offset point from it relative to it's present
		direction
	*/
	if (trackingCamera.target->type == OBJ_DROID)
	{
		const int angle = 90 - abs((player.r.x/182) % 90);

		const DROID *psDroid = (DROID*)trackingCamera.target;
		const PROPULSION_STATS *psPropStats = &asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat];

		if (psPropStats->propulsionType == PROPULSION_TYPE_LIFT)
		{
			bFlying = true;
		}

		/* Present direction is important */
		if (getNumDroidsSelected() > 2)
		{
			unsigned group = trackingCamera.target->selected ? GROUP_SELECTED : trackingCamera.target->group;

			uint16_t multiAngle = getAverageTrackAngle(group, true);
			getTrackingConcerns(&concern.x, &concern.y, &concern.z, group, true);

			behind = iSinCosR(multiAngle, CAM_DEFAULT_OFFSET);
		}
		else
		{
			behind = iSinCosR(trackingCamera.target->rot.direction, CAM_DEFAULT_OFFSET);
		}

		concern.y += angle*5;
	}

	Vector3i realPos = concern - Vector3i(-behind.x, 0, -behind.y);
	Vector3f separation = realPos - trackingCamera.position;
	Vector3f acceleration;
	if (!bFlying)
	{
		acceleration = separation*ACCEL_CONSTANT - trackingCamera.velocity*VELOCITY_CONSTANT;
	}
	else
	{
		separation.y /= 2.0f;
		acceleration = separation*(ACCEL_CONSTANT*4) - trackingCamera.velocity*(VELOCITY_CONSTANT*2);
	}

	if (update & X_UPDATE)
	{
		/* Need to update acceleration along x axis */
		trackingCamera.acceleration.x = acceleration.x;
	}

	if (update & Y_UPDATE)
	{
		/* Need to update acceleration along y axis */
		trackingCamera.acceleration.y = acceleration.y;
	}

	if (update & Z_UPDATE)
	{
		/* Need to update acceleration along z axis */
		trackingCamera.acceleration.z = acceleration.z;
	}
}

//-----------------------------------------------------------------------------------

static void updateCameraVelocity(UBYTE update)
{
	if(update & X_UPDATE)
	{
		trackingCamera.velocity.x += realTimeAdjustedIncrement(trackingCamera.acceleration.x);
	}

	if(update & Y_UPDATE)
	{
		trackingCamera.velocity.y += realTimeAdjustedIncrement(trackingCamera.acceleration.y);
	}

	if(update & Z_UPDATE)
	{
		trackingCamera.velocity.z += realTimeAdjustedIncrement(trackingCamera.acceleration.z);
	}
}

//-----------------------------------------------------------------------------------

static void	updateCameraPosition(UBYTE update)
{
	if(update & X_UPDATE)
	{
		/* Need to update position along x axis */
		trackingCamera.position.x += realTimeAdjustedIncrement(trackingCamera.velocity.x);
	}

	if(update & Y_UPDATE)
	{
		/* Need to update position along y axis */
		trackingCamera.position.y += realTimeAdjustedIncrement(trackingCamera.velocity.y);
	}

	if(update & Z_UPDATE)
	{
		/* Need to update position along z axis */
		trackingCamera.position.z += realTimeAdjustedIncrement(trackingCamera.velocity.z);
	}
}

//-----------------------------------------------------------------------------------
/* Calculate the acceleration that the camera spins around at */
static void updateCameraRotationAcceleration( UBYTE update )
{
	SDWORD	worldAngle;
	float	separation;
	SDWORD	xConcern, yConcern, zConcern;
	bool	bTooLow;
	PROPULSION_STATS *psPropStats;
	bool	bGotFlying = false;
	SDWORD	xPos = 0, yPos = 0, zPos = 0;

	bTooLow = false;
	if(trackingCamera.target->type == OBJ_DROID)
	{
		DROID *psDroid = (DROID*)trackingCamera.target;
		psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
		if(psPropStats->propulsionType == PROPULSION_TYPE_LIFT)
		{
			int droidHeight, difHeight, droidMapHeight;

			bGotFlying = true;
			droidHeight = psDroid->pos.z;
			droidMapHeight = map_Height(psDroid->pos.x, psDroid->pos.y);
			difHeight = abs(droidHeight - droidMapHeight);
			if(difHeight < MIN_TRACK_HEIGHT)
			{
				bTooLow = true;
			}
		}
	}

	if(update & Y_UPDATE)
	{
		/* Presently only y rotation being calculated - but same idea for other axes */
		/* Check what we're tracking */
		if(getNumDroidsSelected()>2 && trackingCamera.target->type == OBJ_DROID)
		{
			unsigned group = trackingCamera.target->selected ? GROUP_SELECTED : trackingCamera.target->group;
			yConcern = getAverageTrackAngle(group, false);
		}
		else
		{
			yConcern = trackingCamera.target->rot.direction;
		}
		yConcern += DEG(180);

  		while(trackingCamera.rotation.y < 0)
		{
			trackingCamera.rotation.y += DEG(360);
		}

		/* Which way are we facing? */
		worldAngle = trackingCamera.rotation.y;
		separation = angleDelta(yConcern - worldAngle);

		/* Make new acceleration */
		trackingCamera.rotAccel.y = ROT_ACCEL_CONSTANT * separation - ROT_VELOCITY_CONSTANT * trackingCamera.rotVel.y;
	}

	if(update & X_UPDATE)
	{
		if(trackingCamera.target->type == OBJ_DROID && !bGotFlying)
		{
			uint16_t pitch;
			unsigned group = trackingCamera.target->selected ? GROUP_SELECTED : trackingCamera.target->group;
			getTrackingConcerns(&xPos, &yPos, &zPos, GROUP_SELECTED, true);  // FIXME Should this be group instead of GROUP_SELECTED?
			getBestPitchToEdgeOfGrid(xPos, zPos, DEG(180) - getAverageTrackAngle(group, true), &pitch);
			pitch = MAX(angleDelta(pitch), DEG(14));
			xConcern = -pitch;
		}
		else
		{
			xConcern = trackingCamera.target->rot.pitch;
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
		separation = angleDelta(xConcern - worldAngle);

		/* Make new acceleration */
		trackingCamera.rotAccel.x =
			/* Make this really slow */
			((ROT_ACCEL_CONSTANT)*separation - ROT_VELOCITY_CONSTANT*(float)trackingCamera.rotVel.x);
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
			zConcern = trackingCamera.target->rot.roll;
		}
		while(trackingCamera.rotation.z<0)
			{
				trackingCamera.rotation.z+=DEG(360);
			}
		worldAngle =  trackingCamera.rotation.z;
		separation = (float) ((zConcern - worldAngle));
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
			((ROT_ACCEL_CONSTANT/1)*separation - ROT_VELOCITY_CONSTANT*(float)trackingCamera.rotVel.z);
	}

}

//-----------------------------------------------------------------------------------
/*	Calculate the velocity that the camera spins around at - just add previously
	calculated acceleration */
static void updateCameraRotationVelocity( UBYTE update )
{
	if(update & Y_UPDATE)
	{
		trackingCamera.rotVel.y += realTimeAdjustedIncrement(trackingCamera.rotAccel.y);
	}
	if(update & X_UPDATE)
	{
		trackingCamera.rotVel.x += realTimeAdjustedIncrement(trackingCamera.rotAccel.x);
	}
	if(update & Z_UPDATE)
	{
		trackingCamera.rotVel.z += realTimeAdjustedIncrement(trackingCamera.rotAccel.z);
	}

}

//-----------------------------------------------------------------------------------
/* Move the camera around by adding the velocity */
static void updateCameraRotationPosition( UBYTE update )
{
 	if (update & Y_UPDATE)
	{
		trackingCamera.rotation.y += realTimeAdjustedIncrement(trackingCamera.rotVel.y);
	}
	if (update & X_UPDATE)
	{
		trackingCamera.rotation.x += realTimeAdjustedIncrement(trackingCamera.rotVel.x);
	}
	if (update & Z_UPDATE)
	{
		trackingCamera.rotation.z += realTimeAdjustedIncrement(trackingCamera.rotVel.z);
	}
}

/* Returns how far away we are from our goal in rotation */
/* Updates the viewpoint according to the object being tracked */
bool	camTrackCamera( void )
{
PROPULSION_STATS	*psPropStats;
DROID	*psDroid;
bool	bFlying;

	bFlying = false;

	/* Most importantly - see if the target we're tracking is dead! */
	if(trackingCamera.target->died)
	{
		setFindNewTarget();
		return(false);
	}

	/*	Cancel tracking if it's no longer selected.
		This may not be desirable? 	*/
   	if(trackingCamera.target->type == OBJ_DROID)
	{

//		if(!trackingCamera.target->selected)
//		{
//			return(false);
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
		if (psPropStats->propulsionType == PROPULSION_TYPE_LIFT)
		{
				bFlying = true;
		}
	}
/*
	bIsBuilding = false;
	if(trackingCamera.target->type == OBJ_DROID)
	{
		psDroid= (DROID*)trackingCamera.target;
		if(DroidIsBuilding(psDroid))
		{
			bIsBuilding = true;
		}
	}
*/


	if(bRadarAllign || trackingCamera.target->type == OBJ_DROID)
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

	/* Update the position that's now stored in trackingCamera.position */
	player.p.x = trackingCamera.position.x;
	player.p.y = trackingCamera.position.y;
	player.p.z = trackingCamera.position.z;

	/* Update the rotations that're now stored in trackingCamera.rotation */
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
		if (trackingCamera.velocity*trackingCamera.velocity + trackingCamera.acceleration*trackingCamera.acceleration < 1.f &&
		    trackingCamera.rotVel*trackingCamera.rotVel     + trackingCamera.rotAccel*trackingCamera.rotAccel         < 1.f)
		{
			setWarCamActive(false);
		}
	}
	return(true);
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
SDWORD	getPresAngle( void )
{
	return(presAvAngle);
}
//-----------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------
UDWORD	getNumDroidsSelected( void )
{
	return(selNumSelected(selectedPlayer));
}

//-----------------------------------------------------------------------------------

/* Returns whether or not the tracking camera is active */
bool	getWarCamStatus( void )
{
	/* Is it switched off? */
	if(trackingCamera.status == CAM_INACTIVE)
	{
		return(false);
	}
	else
	{
		/* Tracking is ON */
		return(true);
	}
}

//-----------------------------------------------------------------------------------

/* Flips the status of tracking to the opposite of what it presently is */
void	camToggleStatus( void )
{
 	/* If it's off */
	if(trackingCamera.status == CAM_INACTIVE)
	{
		/* Switch it on */
		setWarCamActive(true);
	}
	else
	{
		/* Otherwise, switch it off */
		setWarCamActive(false);
		if(getDrivingStatus())
		{
			StopDriverMode();
		}
	}
}


/*	Flips on/off whether we print out full info about the droid being tracked.
	If ON then this info is permanent on screen and realtime updating */
void	camToggleInfo(void)
{
	bFullInfo = !bFullInfo;
}

/* Informs the tracking camera that we want to start tracking to a new radar target */
void	requestRadarTrack(SDWORD x, SDWORD y)
{
	radarX = (SWORD)x;
 	radarY = (SWORD)y;
 	bRadarTrackingRequested = true;
	trackingCamera.status = CAM_REQUEST;
	processWarCam();
// 	setWarCamActive(true);
}

/* Returns whether we're presently tracking to a new _location_ */
bool	getRadarTrackingStatus( void )
{
bool	retVal;

	if(trackingCamera.status == CAM_INACTIVE)
	{
		retVal = false;
	}
	else
	{
		//if(/*trackingCamera.target && */trackingCamera.target->type == OBJ_TARGET)
        //if you know why the above check was commented out please tell me AB 19/11/98
        if(trackingCamera.target && trackingCamera.target->type == OBJ_TARGET)
		{
			retVal = true;
		}
		else
		{
			retVal = false;
		}
	}
	return(retVal);
}

void	toggleRadarAllignment( void )
{
	bRadarAllign = !bRadarAllign;
}

void	camInformOfRotation( Vector3i *rotation )
{
	trackingCamera.rotation.x = rotation->x;
	trackingCamera.rotation.y = rotation->y;
	trackingCamera.rotation.z = rotation->z;
}
