/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include "move.h"
#include "order.h"
#include "action.h"
#include "intdisplay.h"
#include "display3d.h"
#include "selection.h"
#include "animation.h"

/* Storage for old viewnagles etc */
struct WARCAM
{
	WARSTATUS status;
	UDWORD trackClass;
	UDWORD lastUpdate;
	iView oldView;

	Vector3f acceleration = Vector3f(0.f, 0.f, 0.f);
	Vector3f velocity = Vector3f(0.f, 0.f, 0.f);
	Vector3f position = Vector3f(0.f, 0.f, 0.f);

	Vector3f rotation = Vector3f(0.f, 0.f, 0.f);
	Vector3f rotVel = Vector3f(0.f, 0.f, 0.f);
	Vector3f rotAccel = Vector3f(0.f, 0.f, 0.f);

	float oldDistance;
	DROID *target;
};

#define MIN_TRACK_HEIGHT 16

/* Holds all the details of our camera */
static	WARCAM	trackingCamera;

/* Present rotation for the 3d camera logo */
static	SDWORD	warCamLogoRotation;

/* Used to animate radar jump */
struct RADAR_JUMP {
	Animation<Vector3f> position;
	Animation<Vector3f> rotation;
};

static RADAR_JUMP radarJumpAnimation
{
	Animation<Vector3f>(graphicsTime, EASE_IN_OUT),
	Animation<Vector3f>(graphicsTime, EASE_IN_OUT)
};

static SDWORD	presAvAngle = 0;

/*	These are the DEFAULT offsets that make us track _behind_ a droid and allow
	it to be pretty far _down_ the screen, so we can see more
*/

/* Offset from droid's world coords */
/* How far we track relative to the droids location - direction matters */
#define	CAM_DEFAULT_OFFSET	-400


/* How much info do you want when tracking a droid - this toggles full stat info */
static	bool bFullInfo = false;

// Prototypes
static bool camTrackCamera();
static void camRadarJump();

//-----------------------------------------------------------------------------------
/* Sets the camera to inactive to begin with */
void	initWarCam()
{
	/* We're not initially following anything */
	trackingCamera.status = CAM_INACTIVE;

	/* Logo setup */
	warCamLogoRotation = 0;
}

#define	LEADER_LEFT			1
#define	LEADER_RIGHT		2
#define	LEADER_UP			3
#define	LEADER_DOWN			4
#define LEADER_STATIC		5

static void processLeaderSelection()
{
	DROID *psDroid;
	DROID *psNew = nullptr;
	UDWORD leaderClass;
	bool bSuccess;
	UDWORD dif;
	UDWORD bestSoFar;

	if (!getWarCamStatus())
	{
		return;
	}

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
			if (DrawnInLastFrame(psDroid->sDisplay.frameNumber) && psDroid->selected && psDroid != trackingCamera.target)
			{
				if (psDroid->sDisplay.screenX < trackingCamera.target->sDisplay.screenX)
				{
					dif = trackingCamera.target->sDisplay.screenX - psDroid->sDisplay.screenX;
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
			if (DrawnInLastFrame(psDroid->sDisplay.frameNumber) && psDroid->selected && psDroid != trackingCamera.target)
			{
				if (psDroid->sDisplay.screenX > trackingCamera.target->sDisplay.screenX)
				{
					dif = psDroid->sDisplay.screenX - trackingCamera.target->sDisplay.screenX;
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
			if (DrawnInLastFrame(psDroid->sDisplay.frameNumber) && psDroid->selected && psDroid != trackingCamera.target)
			{
				if (psDroid->sDisplay.screenY < trackingCamera.target->sDisplay.screenY)
				{
					dif = trackingCamera.target->sDisplay.screenY - psDroid->sDisplay.screenY;
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
			if (DrawnInLastFrame(psDroid->sDisplay.frameNumber) && psDroid->selected && psDroid != trackingCamera.target)
			{
				if (psDroid->sDisplay.screenY > trackingCamera.target->sDisplay.screenY)
				{
					dif = psDroid->sDisplay.screenY - trackingCamera.target->sDisplay.screenY;
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
		camAlignWithTarget(psNew);
	}
}

/* Updates the camera position/angle along with the object movement */
void	processWarCam()
{
	DROID *foundTarget;

	/* Ensure that the camera only ever flips state within this routine! */
	switch (trackingCamera.status)
	{
	case CAM_INACTIVE:
		break;

	case CAM_REQUEST:

		/* See if we can find the target to follow */
		foundTarget = camFindDroidTarget();

		if (foundTarget && !foundTarget->died)
		{
			/* We've got one, so store away info */
			camAlignWithTarget(foundTarget);
			/* We're now into tracking status */
			trackingCamera.status = CAM_TRACK_DROID;
			/* Inform via console */
			if (getWarCamStatus())
			{
				CONPRINTF("WZ/CAM  - %s", droidGetName(foundTarget));
			}
		}
		else
		{
			/* We've requested a track with no droid selected */
			trackingCamera.status = CAM_INACTIVE;
		}
		break;

	case CAM_TRACK_DROID:
		/* Track the droid unless routine comes back false */
		if (!camTrackCamera())
		{
			/*
				Camera track came back false, either because droid died or is
				no longer selected, so reset to old values
			*/
			foundTarget = camFindDroidTarget();
			if (foundTarget && !foundTarget->died)
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

	case CAM_RADAR_JUMP:
		camRadarJump();
		break;

	case CAM_RESET:
		/* Switch to inactive mode */
		trackingCamera.status = CAM_INACTIVE;
		break;
	}
}

//-----------------------------------------------------------------------------------

/* Flips states for camera active */
void	setWarCamActive(bool status)
{
	debug(LOG_NEVER, "setWarCamActive(%d)\n", status);

	/* We're trying to switch it on */
	if (status == true)
	{
		/* If it's not inactive then it's already in use - so return */
		/* We're tracking a droid */
		if (trackingCamera.status != CAM_INACTIVE)
		{
			return;
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
		if (trackingCamera.status == CAM_INACTIVE)
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

DROID *camFindDroidTarget()
{
	DROID	*psDroid;

	for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			/* Return the first one found */
			return psDroid;
		}
	}

	/* We didn't find one */
	return (nullptr);
}


//-----------------------------------------------------------------------------------

/* Stores away old viewangle info and sets up new distance and angles */
void	camAlignWithTarget(DROID *psDroid)
{
	/* Store away the target */
	trackingCamera.target = psDroid;

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
	trackingCamera.lastUpdate = realTime;
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
	Vector3i concern = trackingCamera.target->pos.xzy();
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
	const int angle = 90 - abs((player.r.x / 182) % 90);

	const PROPULSION_STATS *psPropStats = &asPropulsionStats[trackingCamera.target->asBits[COMP_PROPULSION]];

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

	concern.y += angle * 5;

	Vector3i realPos = concern - Vector3i(-behind.x, 0, -behind.y);
	Vector3f separation = Vector3f(realPos) - trackingCamera.position;
	Vector3f acceleration;
	if (!bFlying)
	{
		acceleration = separation * ACCEL_CONSTANT - trackingCamera.velocity * VELOCITY_CONSTANT;
	}
	else
	{
		separation.y /= 2.0f;
		acceleration = separation * (ACCEL_CONSTANT * 4) - trackingCamera.velocity * (VELOCITY_CONSTANT * 2);
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
	if (update & X_UPDATE)
	{
		trackingCamera.velocity.x += realTimeAdjustedIncrement(trackingCamera.acceleration.x);
	}

	if (update & Y_UPDATE)
	{
		trackingCamera.velocity.y += realTimeAdjustedIncrement(trackingCamera.acceleration.y);
	}

	if (update & Z_UPDATE)
	{
		trackingCamera.velocity.z += realTimeAdjustedIncrement(trackingCamera.acceleration.z);
	}
}

//-----------------------------------------------------------------------------------

static void	updateCameraPosition(UBYTE update)
{
	if (update & X_UPDATE)
	{
		/* Need to update position along x axis */
		trackingCamera.position.x += realTimeAdjustedIncrement(trackingCamera.velocity.x);
	}

	if (update & Y_UPDATE)
	{
		/* Need to update position along y axis */
		trackingCamera.position.y += realTimeAdjustedIncrement(trackingCamera.velocity.y);
	}

	if (update & Z_UPDATE)
	{
		/* Need to update position along z axis */
		trackingCamera.position.z += realTimeAdjustedIncrement(trackingCamera.velocity.z);
	}
}

//-----------------------------------------------------------------------------------
/* Calculate the acceleration that the camera spins around at */
static void updateCameraRotationAcceleration(UBYTE update)
{
	SDWORD	worldAngle;
	float	separation;
	SDWORD	xConcern, yConcern, zConcern;
	bool	bTooLow;
	PROPULSION_STATS *psPropStats;
	bool	bGotFlying = false;
	SDWORD	xPos = 0, yPos = 0, zPos = 0;

	bTooLow = false;
	psPropStats = asPropulsionStats + trackingCamera.target->asBits[COMP_PROPULSION];
	if (psPropStats->propulsionType == PROPULSION_TYPE_LIFT)
	{
		int droidHeight, difHeight, droidMapHeight;

		bGotFlying = true;
		droidHeight = trackingCamera.target->pos.z;
		droidMapHeight = map_Height(trackingCamera.target->pos.x, trackingCamera.target->pos.y);
		difHeight = abs(droidHeight - droidMapHeight);
		if (difHeight < MIN_TRACK_HEIGHT)
		{
			bTooLow = true;
		}
	}

	if (update & Y_UPDATE)
	{
		/* Presently only y rotation being calculated - but same idea for other axes */
		/* Check what we're tracking */
		if (getNumDroidsSelected() > 2)
		{
			unsigned group = trackingCamera.target->selected ? GROUP_SELECTED : trackingCamera.target->group;
			yConcern = getAverageTrackAngle(group, false);
		}
		else
		{
			yConcern = trackingCamera.target->rot.direction;
		}
		yConcern += DEG(180);

		while (trackingCamera.rotation.y < 0)
		{
			trackingCamera.rotation.y += DEG(360);
		}

		/* Which way are we facing? */
		worldAngle = trackingCamera.rotation.y;
		separation = angleDelta(yConcern - worldAngle);

		/* Make new acceleration */
		trackingCamera.rotAccel.y = ROT_ACCEL_CONSTANT * separation - ROT_VELOCITY_CONSTANT * trackingCamera.rotVel.y;
	}

	if (update & X_UPDATE)
	{
		if (!bGotFlying)
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
		while (trackingCamera.rotation.x < 0)
		{
			trackingCamera.rotation.x += DEG(360);
		}
		worldAngle =  trackingCamera.rotation.x;
		separation = angleDelta(xConcern - worldAngle);

		/* Make new acceleration */
		trackingCamera.rotAccel.x =
		    /* Make this really slow */
		    ((ROT_ACCEL_CONSTANT) * separation - ROT_VELOCITY_CONSTANT * (float)trackingCamera.rotVel.x);
	}

	/* This looks a bit arse - looks like a flight sim */
	if (update & Z_UPDATE)
	{
		if (bTooLow)
		{
			zConcern = 0;
		}
		else
		{
			zConcern = trackingCamera.target->rot.roll;
		}
		while (trackingCamera.rotation.z < 0)
		{
			trackingCamera.rotation.z += DEG(360);
		}
		worldAngle =  trackingCamera.rotation.z;
		separation = (float)((zConcern - worldAngle));
		if (separation < DEG(-180))
		{
			separation += DEG(360);
		}
		else if (separation > DEG(180))
		{
			separation -= DEG(360);
		}

		/* Make new acceleration */
		trackingCamera.rotAccel.z =
		    /* Make this really slow */
		    ((ROT_ACCEL_CONSTANT / 1) * separation - ROT_VELOCITY_CONSTANT * (float)trackingCamera.rotVel.z);
	}

}

//-----------------------------------------------------------------------------------
/*	Calculate the velocity that the camera spins around at - just add previously
	calculated acceleration */
static void updateCameraRotationVelocity(UBYTE update)
{
	if (update & Y_UPDATE)
	{
		trackingCamera.rotVel.y += realTimeAdjustedIncrement(trackingCamera.rotAccel.y);
	}
	if (update & X_UPDATE)
	{
		trackingCamera.rotVel.x += realTimeAdjustedIncrement(trackingCamera.rotAccel.x);
	}
	if (update & Z_UPDATE)
	{
		trackingCamera.rotVel.z += realTimeAdjustedIncrement(trackingCamera.rotAccel.z);
	}
}

//-----------------------------------------------------------------------------------
/* Move the camera around by adding the velocity */
static void updateCameraRotationPosition(UBYTE update)
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
static bool camTrackCamera()
{
	PROPULSION_STATS *psPropStats;
	bool bFlying = false;

	/* Most importantly - see if the target we're tracking is dead! */
	if (trackingCamera.target->died)
	{
		return (false);
	}

	updateCameraAcceleration(CAM_ALL);
	updateCameraVelocity(CAM_ALL);
	updateCameraPosition(CAM_ALL);

	/* Update the acceleration,velocity and rotation of the camera for rotation */
	/*	You can track roll as well (z axis) but it makes you ill and looks
		like a flight sim, so for now just pitch and orientation */
	psPropStats = asPropulsionStats + trackingCamera.target->asBits[COMP_PROPULSION];
	if (psPropStats->propulsionType == PROPULSION_TYPE_LIFT)
	{
		bFlying = true;
	}

	if (bFlying)
	{
		updateCameraRotationAcceleration(CAM_ALL);
	}
	else
	{
		updateCameraRotationAcceleration(CAM_X_AND_Y);
	}

	if (bFlying)
	{
		updateCameraRotationVelocity(CAM_ALL);
		updateCameraRotationPosition(CAM_ALL);
	}
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
	player.r.y = trackingCamera.rotation.y;
	player.r.z = trackingCamera.rotation.z;

	/* There's a minimum for this - especially when John's VTOL code lets them land vertically on cliffs */
	if (player.r.x > DEG(360 + MAX_PLAYER_X_ANGLE))
	{
		player.r.x = DEG(360 + MAX_PLAYER_X_ANGLE);
	}

	/* Clip the position to the edge of the map */
	CheckScrollLimits();

	/* Store away our last update as acceleration and velocity are all fn()/dt */
	trackingCamera.lastUpdate = realTime;
	if (bFullInfo)
	{
		flushConsoleMessages();
		printDroidInfo(trackingCamera.target);
	}

	return (true);
}

/* Updates the viewpoint animation to jump to the location pointed at the radar */
static void camRadarJump()
{
	radarJumpAnimation.position.update();
	radarJumpAnimation.rotation.update();
	player.p = radarJumpAnimation.position.getCurrent();
	player.r = radarJumpAnimation.rotation.getCurrent();

	if (!radarJumpAnimation.position.isActive() && !radarJumpAnimation.rotation.isActive())
	{
		trackingCamera.status = CAM_INACTIVE;
	}
}

//-----------------------------------------------------------------------------------
DROID *getTrackingDroid()
{
	if (!getWarCamStatus())
	{
		return (nullptr);
	}
	if (trackingCamera.status != CAM_TRACK_DROID)
	{
		return (nullptr);
	}
	return trackingCamera.target;
}

//-----------------------------------------------------------------------------------
SDWORD	getPresAngle()
{
	return (presAvAngle);
}
//-----------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------
UDWORD	getNumDroidsSelected()
{
	return (selNumSelected(selectedPlayer));
}

//-----------------------------------------------------------------------------------

/* Returns whether or not the tracking camera is active */
bool	getWarCamStatus()
{
	/* Is it switched off? */
	if (trackingCamera.status == CAM_INACTIVE)
	{
		return (false);
	}
	else
	{
		/* Tracking is ON */
		return (true);
	}
}

//-----------------------------------------------------------------------------------

/* Flips the status of tracking to the opposite of what it presently is */
void	camToggleStatus()
{
	/* If it's off */
	if (trackingCamera.status == CAM_INACTIVE)
	{
		/* Switch it on */
		setWarCamActive(true);
	}
	else
	{
		/* Otherwise, switch it off */
		setWarCamActive(false);
	}
}


/*	Flips on/off whether we print out full info about the droid being tracked.
	If ON then this info is permanent on screen and realtime updating */
void	camToggleInfo()
{
	bFullInfo = !bFullInfo;
}

/**
 * Find the angle equivalent to `from` in the interval between `to - 180°` and to `to + 180°`.
 *
 * For example:
 * - if `from` is `10°` and `to` is `350°`, it will return `370°`.
 * - if `from` is `350°` and `to` is `0°`, it will return `-10°`.
 *
 * Useful while animating a rotation, to always animate the shortest angle delta.
 */
int32_t calculateRelativeAngle(uint16_t from, uint16_t to)
{
	return to + (int16_t)(from - to);
}

/* Informs the tracking camera that we want to start tracking to a new radar target */
void requestRadarTrack(SDWORD x, SDWORD y)
{
	auto initialPosition = Vector3f(player.p);
	auto targetPosition = Vector3f(x, calculateCameraHeightAt(map_coord(x), map_coord(y)), y);
	auto animationDuration = glm::log(glm::length(targetPosition - initialPosition)) * 100;

	auto targetRotationTemp = trackingCamera.status == CAM_TRACK_DROID ? trackingCamera.oldView.r : player.r;
	auto targetRotation = Vector3i((uint16_t)targetRotationTemp.x, (uint16_t)targetRotationTemp.y, (uint16_t)0);

	auto initialRotation = Vector3f(
		calculateRelativeAngle(player.r.x, targetRotation.x),
		calculateRelativeAngle(player.r.y, targetRotation.y),
		calculateRelativeAngle(player.r.z, targetRotation.z)
	);

	radarJumpAnimation.position
		.setInitialData(initialPosition)
		.setFinalData(targetPosition)
		.setDuration(animationDuration)
		.start();

	radarJumpAnimation.rotation
		.setInitialData(initialRotation)
		.setFinalData(targetRotation)
		.setDuration(animationDuration)
		.start();

	trackingCamera.status = CAM_RADAR_JUMP;
}

/* Returns whether we're presently tracking to a new _location_ */
bool	getRadarTrackingStatus()
{
	return trackingCamera.status == CAM_RADAR_JUMP;
}

void	camInformOfRotation(Vector3i *rotation)
{
	trackingCamera.rotation.x = rotation->x;
	trackingCamera.rotation.y = rotation->y;
	trackingCamera.rotation.z = rotation->z;
}
