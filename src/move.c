/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
/*
 * Move.c
 *
 * Routines for moving units about the map
 *
 */
#include "lib/framework/frame.h"

#include "lib/framework/trig.h"
#include "lib/framework/math-help.h"
#include "lib/gamelib/gtime.h"
#include "lib/gamelib/animobj.h"
#include "lib/netplay/netplay.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"

#include "move.h"

#include "objects.h"
#include "visibility.h"
#include "map.h"
#include "fpath.h"
#include "loop.h"
#include "geometry.h"
#include "anim_id.h"
#include "formationdef.h"
#include "formation.h"
#include "action.h"
#include "display3d.h"
#include "order.h"
#include "astar.h"
#include "combat.h"
#include "mapgrid.h"
#include "display.h"	// needed for widgetsOn flag.
#include "effects.h"
#include "power.h"
#include "scores.h"
#include "multiplay.h"
#include "multigifts.h"

#include "drive.h"

/* max and min vtol heights above terrain */
#define	VTOL_HEIGHT_MIN				250
#define	VTOL_HEIGHT_LEVEL			300
#define	VTOL_HEIGHT_MAX				350

/* minimum distance for cyborgs to jump */
#define	CYBORG_MIN_JUMP_DISTANCE	500
#define	CYBORG_MIN_JUMP_HEIGHT		50
#define	CYBORG_MAX_JUMP_HEIGHT		200

/* Radius of a droid for collision detection */
#define DROID_SIZE		64

// Maximum size of an object for collision
#define OBJ_MAXRADIUS	(TILE_UNITS * 4)

// How close to a way point the next target starts to phase in
#define WAYPOINT_DIST	(TILE_UNITS *2)
#define WAYPOINT_DSQ	(WAYPOINT_DIST*WAYPOINT_DIST)

// Accuracy for the boundary vector
#define BOUND_ACC		1000

/* Width and length of the droid collision box */
#define HITBOX_WIDTH	128
#define HITBOX_LENGTH	(HITBOX_WIDTH * 3)
/* Angle covered by hit box at far end */
#define HITBOX_ANGLE	36

/* How close to a way point a droid has to get before going to the next */
#define DROID_HIT_DIST	16

// how long a shuffle can propagate before they all stop
#define MOVE_SHUFFLETIME	10000

// Length of time a droid has to be stationery to be considered blocked
#define BLOCK_TIME		6000
#define SHUFFLE_BLOCK_TIME	2000
// How long a droid has to be stationary before stopping trying to move
#define BLOCK_PAUSETIME	1500
#define BLOCK_PAUSERELEASE 500
// How far a droid has to move before it is no longer 'stationary'
#define BLOCK_DIST		64
// How far a droid has to rotate before it is no longer 'stationary'
#define BLOCK_DIR		90
// The min and max ratios of target/obstruction distances for an obstruction
// to be on the target
#define BLOCK_MINRATIO	99.f / 100.f
#define BLOCK_MAXRATIO	101.f / 100.f
// The result of the dot product for two vectors to be the same direction
#define BLOCK_DOTVAL	99.f / 100.f

// How far out from an obstruction to start avoiding it
#define AVOID_DIST		(TILE_UNITS*2)

/* Number of game units/sec for base speed */
#define BASE_SPEED		1

/* Number of degrees/sec for base turn rate */
#define BASE_TURN		1

/* What the base speed is intialised to */
#define BASE_SPEED_INIT		((float)BASE_SPEED / (float)BASE_DEF_RATE)

/* What the frame rate is assumed to be at start up */
#define BASE_DEF_RATE	25

/* What the base turn rate is intialised to */
#define BASE_TURN_INIT		((float)BASE_TURN / (float)BASE_DEF_RATE)

// maximum and minimum speed to approach a final way point
#define MAX_END_SPEED		300
#define MIN_END_SPEED		60

// distance from final way point to start slowing
#define END_SPEED_RANGE		(3 * TILE_UNITS)

// how long to pause after firing a FOM_NO weapon
#define FOM_MOVEPAUSE		1500

// distance to consider droids for a shuffle
#define SHUFFLE_DIST		(3*TILE_UNITS/2)
// how far to move for a shuffle
#define SHUFFLE_MOVE		(2*TILE_UNITS/2)

/***********************************************************************************/
/*             Tracked model defines                                               */

// The magnitude of direction change required for a droid to spin on the spot
#define TRACKED_SPIN_ANGLE		(TRIG_DEGREES/8)
// The speed at which tracked droids spin
#define TRACKED_SPIN_SPEED		200
// The speed at which tracked droids turn while going forward
#define TRACKED_TURN_SPEED		60
// How fast a tracked droid accelerates
#define TRACKED_ACCEL			250
// How fast a tracked droid decelerates
#define TRACKED_DECEL			800
// How fast a tracked droid decelerates
#define TRACKED_SKID_DECEL		600
// How fast a wheeled droid decelerates
#define WHEELED_SKID_DECEL		350
// How fast a hover droid decelerates
#define HOVER_SKID_DECEL		120


/************************************************************************************/
/*             Person model defines                                                 */

// The magnitude of direction change required for a person to spin on the spot
#define PERSON_SPIN_ANGLE		(TRIG_DEGREES/8)
// The speed at which people spin
#define PERSON_SPIN_SPEED		500
// The speed at which people turn while going forward
#define PERSON_TURN_SPEED		250
// How fast a person accelerates
#define PERSON_ACCEL			250
// How fast a person decelerates
#define PERSON_DECEL			450


/************************************************************************************/
/*             VTOL model defines                                                 */

// The magnitude of direction change required for a vtol to spin on the spot
#define VTOL_SPIN_ANGLE			(TRIG_DEGREES)
// The speed at which vtols spin (ignored now!)
#define VTOL_SPIN_SPEED			100
// The speed at which vtols turn while going forward (ignored now!)
#define VTOL_TURN_SPEED			100
// How fast vtols accelerate
#define VTOL_ACCEL				200
// How fast vtols decelerate
#define VTOL_DECEL				200
// How fast vtols 'skid'
#define VTOL_SKID_DECEL			600


/* The current base speed for this frame and averages for the last few seconds */
float	baseSpeed;
#define	BASE_FRAMES			10
static UDWORD	baseTimes[BASE_FRAMES];

/* The current base turn rate */
static float	baseTurn;

// The next DROID that should get the router when a lot of units are
// in a MOVEROUTE state
DROID	*psNextRouteDroid;

/* Function prototypes */
static void	moveUpdatePersonModel(DROID *psDroid, SDWORD speed, SDWORD direction);
// Calculate the boundary vector
static void	moveCalcBoundary(DROID *psDroid);
/* Turn a vector into an angle - returns a float (!) */
static float vectorToAngle(float vx, float vy);

static BOOL	g_bFormationSpeedLimitingOn = true;

/* Return the difference in directions */
static UDWORD dirDiff(SDWORD start, SDWORD end)
{
	SDWORD retval, diff;

	diff = end - start;

	if (diff > 0)
	{
		if (diff < 180)
		{
			retval = diff;
		}
		else
		{
			retval = 360 - diff;
		}
	}
	else
	{
		if (diff > -180)
		{
			retval = - diff;
		}
		else
		{
			retval = 360 + diff;
		}
	}

	ASSERT( retval >=0 && retval <=180,
		"dirDiff: result out of range" );

	return retval;
}

/** Initialise the movement system
 */
BOOL moveInitialise(void)
{
	UDWORD i;

	// Initialise the base speed counters
	baseSpeed = BASE_SPEED_INIT;
	baseTurn = BASE_TURN_INIT;
	for (i=0; i< BASE_FRAMES; i++)
	{
		baseTimes[i] = GAME_TICKS_PER_SEC / BASE_DEF_RATE;
	}

	psNextRouteDroid = NULL;

	return true;
}

/** Update the base speed for all movement
 */
void moveUpdateBaseSpeed(void)
{
	UDWORD	totalTime=0, i;

	// Update the list of frame times
	for(i=0; i<BASE_FRAMES-1; i++)
	{
		baseTimes[i]= baseTimes[i+1];
	}
	baseTimes[BASE_FRAMES-1] = frameTime;

	// Add up the time for the last few frames
	for(i=0; i<BASE_FRAMES; i++)
	{
		totalTime += baseTimes[i];
	}

	// Set the base speed
	// here is the original calculation before the fract stuff
	baseSpeed = ((float)totalTime * BASE_SPEED) / (GAME_TICKS_PER_SEC * BASE_FRAMES);

	// Set the base turn rate
	baseTurn = ((float)totalTime * BASE_TURN) / (GAME_TICKS_PER_SEC * BASE_FRAMES);

	// reset the astar counters
	astarResetCounters();

	// check the waiting droid pointer
	if (psNextRouteDroid != NULL)
	{
		if ((psNextRouteDroid->died) ||
			((psNextRouteDroid->sMove.Status != MOVEROUTE) &&
			 (psNextRouteDroid->sMove.Status != MOVEROUTESHUFFLE)))
		{
			objTrace(psNextRouteDroid->id, "Waiting droid %d (player %d) reset",
			         (int)psNextRouteDroid->id, (int)psNextRouteDroid->player);
			psNextRouteDroid = NULL;
		}
	}
}

/** Set a target location for a droid to move to
 *  @return true if the routing was succesful, if false then the calling code
 *          should not try to route here again for a while
 *  @todo Document what "should not try to route here again for a while" means.
 */
static BOOL moveDroidToBase(DROID	*psDroid, UDWORD x, UDWORD y, BOOL bFormation)
{
	FPATH_RETVAL		retVal = FPR_OK;
	SDWORD				fmx1,fmy1, fmx2,fmy2;

	CHECK_DROID(psDroid);

	if(bMultiPlayer && (psDroid->sMove.Status != MOVEWAITROUTE))
	{
		if(SendDroidMove(psDroid,x,y,bFormation) == false)
		{// dont make the move since we'll recv it anyway
			return false;
		}
	}

	// in multiPlayer make Transporter move like the vtols
	if ( psDroid->droidType == DROID_TRANSPORTER && game.maxPlayers == 0)
	{
		fpathSetDirectRoute(psDroid, x, y);
		psDroid->sMove.Status = MOVENAVIGATE;
		psDroid->sMove.Position=0;
		psDroid->sMove.psFormation = NULL;
		return true;
	}
	else if (vtolDroid(psDroid) || (game.maxPlayers > 0 && psDroid->droidType == DROID_TRANSPORTER))
	{
		fpathSetDirectRoute(psDroid, x, y);
		retVal = FPR_OK;
	}
	else
	{
		retVal = fpathRoute(psDroid, x, y);
	}

	/* check formations */
	if ( retVal == FPR_OK )
	{
		// bit of a hack this - john
		// if astar doesn't have a complete route, it returns a route to the nearest clear tile.
		// the location of the clear tile is in DestinationX,DestinationY.
		// reset x,y to this position so the formation gets set up correctly
		x = psDroid->sMove.DestinationX;
		y = psDroid->sMove.DestinationY;

		objTrace(psDroid->id, "unit %d: path ok - base Speed %u, speed %f, target(%d, %d)",
		         (int)psDroid->id, psDroid->baseSpeed, psDroid->sMove.speed, (int)x, (int)y);

		psDroid->sMove.Status = MOVENAVIGATE;
		psDroid->sMove.Position=0;
		psDroid->sMove.fx = psDroid->pos.x;
		psDroid->sMove.fy = psDroid->pos.y;
		psDroid->sMove.fz = psDroid->pos.z;

		// reset the next route droid
		if (psDroid == psNextRouteDroid)
		{
			objTrace(psDroid->id, "Waiting droid %d (player %d) got route",
			         (int)psDroid->id, (int)psDroid->player);
			psNextRouteDroid = NULL;
		}

		// leave any old formation
		if (psDroid->sMove.psFormation)
		{
			formationLeave(psDroid->sMove.psFormation, psDroid);
			psDroid->sMove.psFormation = NULL;
		}

		if (bFormation)
		{
			// join a formation if it exists at the destination
			FORMATION* psFormation = formationFind(x, y);
			if (psFormation)
			{
				psDroid->sMove.psFormation = psFormation;
				formationJoin(psFormation, psDroid);
			}
			else
			{
				// align the formation with the last path of the route
				fmx2 = psDroid->sMove.asPath[psDroid->sMove.numPoints -1].x;
				fmy2 = psDroid->sMove.asPath[psDroid->sMove.numPoints -1].y;
				fmx2 = world_coord(fmx2) + TILE_UNITS / 2;
				fmy2 = world_coord(fmy2) + TILE_UNITS / 2;
				if (psDroid->sMove.numPoints == 1)
				{
					fmx1 = (SDWORD)psDroid->pos.x;
					fmy1 = (SDWORD)psDroid->pos.y;
				}
				else
				{
					fmx1 = psDroid->sMove.asPath[psDroid->sMove.numPoints -2].x;
					fmy1 = psDroid->sMove.asPath[psDroid->sMove.numPoints -2].y;
					fmx1 = world_coord(fmx1) + TILE_UNITS / 2;
					fmy1 = world_coord(fmy1) + TILE_UNITS / 2;
				}

				// no formation so create a new one
				if (formationNew(&psDroid->sMove.psFormation, FT_LINE, (SDWORD)x,(SDWORD)y,
						(SDWORD)calcDirection(fmx1,fmy1, fmx2,fmy2)))
				{
					formationJoin(psDroid->sMove.psFormation, psDroid);
				}
			}
		}
	}
	else if (retVal == FPR_RESCHEDULE)
	{
		objTrace(psDroid->id, "moveDroidToBase(%d): out of time, not our turn; rescheduled", (int)psDroid->id);

		// maxed out routing time this frame - do it next time
		psDroid->sMove.DestinationX = x;
		psDroid->sMove.DestinationY = y;

		if ((psDroid->sMove.Status != MOVEROUTE) &&
			(psDroid->sMove.Status != MOVEROUTESHUFFLE))
		{
			objTrace(psDroid->id, "moveDroidToBase(%d): started waiting at %d",
			         (int)psDroid->id, (int)gameTime);

			psDroid->sMove.Status = MOVEROUTE;

			// note when the unit first tried to route
			psDroid->sMove.bumpTime = gameTime;
		}
	}
	else if (retVal == FPR_WAIT)
	{
		// reset the next route droid
		if (psDroid == psNextRouteDroid)
		{
			objTrace(psDroid->id, "moveDroidToBase(%d): out of time, waiting for next frame (we are next)",
			         (int)psDroid->id);
			psNextRouteDroid = NULL;
		}
		else
		{
			objTrace(psDroid->id, "moveDroidToBase(%d): out of time, waiting for next frame (we are not next)",
			         (int)psDroid->id);
		}

		// the route will be calculated over a number of frames
		psDroid->sMove.Status = MOVEWAITROUTE;
		psDroid->sMove.DestinationX = x;
		psDroid->sMove.DestinationY = y;
	}
	else // if (retVal == FPR_FAILED)
	{
		objTrace(psDroid->id, "Path to (%d, %d) failed for droid %d", (int)x, (int)y, (int)psDroid->id);
		psDroid->sMove.Status = MOVEINACTIVE;
		actionDroid(psDroid, DACTION_SULK);
		return(false);
	}

	CHECK_DROID(psDroid);
	return true;
}

/** Move a droid to a location, joining a formation
 *  @see moveDroidToBase() for the parameter and return value specification
 */
BOOL moveDroidTo(DROID* psDroid, UDWORD x, UDWORD y)
{
	return moveDroidToBase(psDroid,x,y, true);
}

/** Move a droid to a location, not joining a formation
 *  @see moveDroidToBase() for the parameter and return value specification
 */
BOOL moveDroidToNoFormation(DROID* psDroid, UDWORD x, UDWORD y)
{
	ASSERT(x > 0 && y > 0, "Bad movement position");
	return moveDroidToBase(psDroid,x,y, false);
}


/** Move a droid directly to a location.
 *  @note This is (or should be) used for VTOLs only.
 */
void moveDroidToDirect(DROID* psDroid, UDWORD x, UDWORD y)
{
	ASSERT( psDroid != NULL && vtolDroid(psDroid),
		"moveUnitToDirect: only valid for a vtol unit" );

	fpathSetDirectRoute(psDroid, x, y);
	psDroid->sMove.Status = MOVENAVIGATE;
	psDroid->sMove.Position=0;

	// leave any old formation
	if (psDroid->sMove.psFormation)
	{
		formationLeave(psDroid->sMove.psFormation, psDroid);
		psDroid->sMove.psFormation = NULL;
	}
}


/** Turn a droid towards a given location.
 */
void moveTurnDroid(DROID *psDroid, UDWORD x, UDWORD y)
{
	SDWORD moveDir = (SDWORD)calcDirection(psDroid->pos.x, psDroid->pos.y, x, y);

	if ( (SDWORD)(psDroid->direction) != moveDir )
	{
		psDroid->sMove.targetX = (SDWORD)x;
		psDroid->sMove.targetY = (SDWORD)y;
		psDroid->sMove.Status = MOVETURNTOTARGET;
	}
}

// get the difference in direction
static SDWORD moveDirDiff(SDWORD start, SDWORD end)
{
	SDWORD retval, diff;

	diff = end - start;

	if (diff > 0)
	{
		if (diff < 180)
		{
			retval = diff;
		}
		else
		{
			retval = diff - 360;
		}
	}
	else
	{
		if (diff > -180)
		{
			retval = diff;
		}
		else
		{
			retval = 360 + diff;
		}
	}

	return retval;
}

// Tell a droid to move out the way for a shuffle
static void moveShuffleDroid(DROID *psDroid, UDWORD shuffleStart, SDWORD sx, SDWORD sy)
{
	float	shuffleDir, droidDir;
	DROID	*psCurr;
	SDWORD	xdiff,ydiff, mx,my, shuffleMag, diff;
	BOOL	frontClear = true, leftClear = true, rightClear = true;
	SDWORD	lvx,lvy, rvx,rvy, svx,svy;
	SDWORD	shuffleMove;
	SDWORD	tarX,tarY;

	CHECK_DROID(psDroid);

	shuffleDir = vectorToAngle(sx, sy);
	shuffleMag = (SDWORD)sqrtf(sx*sx + sy*sy);

	if (shuffleMag == 0)
	{
		return;
	}

	shuffleMove = SHUFFLE_MOVE;

	// calculate the possible movement vectors
	lvx = -sy * shuffleMove / shuffleMag;
	lvy = sx * shuffleMove / shuffleMag;

	rvx = sy * shuffleMove / shuffleMag;
	rvy = -sx * shuffleMove / shuffleMag;

	svx = lvy; // sx * SHUFFLE_MOVE / shuffleMag;
	svy = rvx; // sy * SHUFFLE_MOVE / shuffleMag;

	// check for blocking tiles
	if (fpathBlockingTile(map_coord((SDWORD)psDroid->pos.x + lvx),
	                      map_coord((SDWORD)psDroid->pos.y + lvy), getPropulsionStats(psDroid)->propulsionType))
	{
		leftClear = false;
	}
	else if (fpathBlockingTile(map_coord((SDWORD)psDroid->pos.x + rvx),
	                           map_coord((SDWORD)psDroid->pos.y + rvy), getPropulsionStats(psDroid)->propulsionType))
	{
		rightClear = false;
	}
	else if (fpathBlockingTile(map_coord((SDWORD)psDroid->pos.x + svx),
	                           map_coord((SDWORD)psDroid->pos.y + svy), getPropulsionStats(psDroid)->propulsionType))
	{
		frontClear = false;
	}

	// find any droids that could block the shuffle
	for(psCurr=apsDroidLists[psDroid->player]; psCurr; psCurr=psCurr->psNext)
	{
		if (psCurr != psDroid)
		{
			xdiff = (SDWORD)psCurr->pos.x - (SDWORD)psDroid->pos.x;
			ydiff = (SDWORD)psCurr->pos.y - (SDWORD)psDroid->pos.y;
			if (xdiff*xdiff + ydiff*ydiff < SHUFFLE_DIST*SHUFFLE_DIST)
			{
				droidDir = vectorToAngle(xdiff, ydiff);
				diff = (SDWORD)moveDirDiff(shuffleDir, droidDir);
				if ((diff > -135) && (diff < -45))
				{
					leftClear = false;
				}
				else if ((diff > 45) && (diff < 135))
				{
					rightClear = false;
				}
			}
		}
	}

	// calculate a target
	if (leftClear)
	{
		mx = lvx;
		my = lvy;
	}
	else if (rightClear)
	{
		mx = rvx;
		my = rvy;
	}
	else if (frontClear)
	{
		mx = svx;
		my = svy;
	}
	else
	{
		// nowhere to shuffle to, quit
		return;
	}

	// check the location for vtols
	tarX = (SDWORD)psDroid->pos.x + mx;
	tarY = (SDWORD)psDroid->pos.y + my;
	if (vtolDroid(psDroid))
	{
		actionVTOLLandingPos(psDroid, (UDWORD *)&tarX,(UDWORD *)&tarY);
	}

	// set up the move state
	if (psDroid->sMove.Status == MOVEROUTE)
	{
		psDroid->sMove.Status = MOVEROUTESHUFFLE;
	}
	else
	{
		psDroid->sMove.Status = MOVESHUFFLE;
	}
	psDroid->sMove.shuffleStart = shuffleStart;
	psDroid->sMove.srcX = (SDWORD)psDroid->pos.x;
	psDroid->sMove.srcY = (SDWORD)psDroid->pos.y;
	psDroid->sMove.targetX = tarX;
	psDroid->sMove.targetY = tarY;
	// setting the Destination could overwrite a MOVEROUTE's destination
	// it is not actually needed for a shuffle anyway
	psDroid->sMove.numPoints = 0;
	psDroid->sMove.Position = 0;
	psDroid->sMove.fx = psDroid->pos.x;
	psDroid->sMove.fy = psDroid->pos.y;

	psDroid->sMove.fz = psDroid->pos.z;

	moveCalcBoundary(psDroid);

	if (psDroid->sMove.psFormation != NULL)
	{
		formationLeave(psDroid->sMove.psFormation, psDroid);
		psDroid->sMove.psFormation = NULL;
	}
	CHECK_DROID(psDroid);
}


/** Stop a droid from moving.
 */
void moveStopDroid(DROID *psDroid)
{
	PROPULSION_STATS	*psPropStats;

	CHECK_DROID(psDroid);

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( psPropStats != NULL,
			"moveUpdateUnit: invalid propulsion stats pointer" );

	if ( psPropStats->propulsionType == LIFT )
	{
		psDroid->sMove.Status = MOVEHOVER;
	}
	else
	{
		psDroid->sMove.Status = MOVEINACTIVE;
	}
}

/** Stops a droid dead in its tracks.
 *  Doesn't allow for any little skidding bits.
 *  @param psDroid the droid to stop from moving
 */
void moveReallyStopDroid(DROID *psDroid)
{
	CHECK_DROID(psDroid);

	psDroid->sMove.Status = MOVEINACTIVE;
	psDroid->sMove.speed = 0;
}


#define PITCH_LIMIT 150

/* Get pitch and roll from direction and tile data */
void updateDroidOrientation(DROID *psDroid)
{
	SDWORD hx0, hx1, hy0, hy1, w;
	SDWORD newPitch, dPitch, pitchLimit;
	double dx, dy;
	double direction, pitch, roll;

	if(psDroid->droidType == DROID_PERSON || cyborgDroid(psDroid) || psDroid->droidType == DROID_TRANSPORTER)
	{
		/* These guys always stand upright */
		return;
	}

	w = 20;
	hx0 = map_Height(psDroid->pos.x + w, psDroid->pos.y);
	hx1 = map_Height(MAX(0, psDroid->pos.x - w), psDroid->pos.y);
	hy0 = map_Height(psDroid->pos.x, psDroid->pos.y + w);
	hy1 = map_Height(psDroid->pos.x, MAX(0, psDroid->pos.y - w));

	//update height in case were in the bottom of a trough
	if (((hx0 +hx1)/2) > (SDWORD)psDroid->pos.z)
	{
		psDroid->pos.z = (UWORD)((hx0 +hx1)/2);
	}
	if (((hy0 +hy1)/2) > (SDWORD)psDroid->pos.z)
	{
		psDroid->pos.z = (UWORD)((hy0 +hy1)/2);
	}

	dx = (double)(hx0 - hx1) / ((double)w * 2.0);
	dy = (double)(hy0 - hy1) / ((double)w * 2.0);
	//dx is atan of angle of elevation along x axis
	//dx is atan of angle of elevation along y axis
	//body
	direction = (M_PI * psDroid->direction) / 180.0;
	pitch = sin(direction) * dx + cos(direction) * dy;
	pitch = atan(pitch);
	newPitch = (SDWORD)((pitch * 180) / M_PI);
	//limit the rate the front comes down to simulate momentum
	pitchLimit = timeAdjustedIncrement(PITCH_LIMIT, true);
	dPitch = newPitch - psDroid->pitch;
	if (dPitch < 0)
	{
		dPitch +=360;
	}
	if ((dPitch > 180) && (dPitch < 360 - pitchLimit))
	{
		psDroid->pitch = (SWORD)(psDroid->pitch - pitchLimit);
	}
	else
	{
		psDroid->pitch = (SWORD)newPitch;
	}
	roll = cos(direction) * dx - sin(direction) * dy;
	roll = atan(roll);
	psDroid->roll = (UWORD)((roll * 180) / M_PI);
}


/** Turn a vector into an angle
 *  @return the vector expressed as an angle
 */
static float vectorToAngle(float vx, float vy)
{
	// Angle in degrees (0->360):
	float angle = 360.0f * atan2f(-vy,vx) / (M_PI * 2.0f);
	angle += 360.0f / 4.0f;

	return wrapf(angle, 360.0f);
}


/* Calculate the change in direction given a target angle and turn rate */
static void moveCalcTurn(float *pCurr, float target, UDWORD rate)
{
	float diff, change, retval = *pCurr;

	ASSERT( target <= 360.0f && target >= 0.0f,
			 "moveCalcTurn: target out of range %f", target );
	ASSERT( retval <= 360.0f && retval >= 0.0f,
			 "moveCalcTurn: cur ang out of range %f", retval );

	// calculate the difference in the angles
	diff = target - retval;

	// calculate the change in direction

	change = (baseTurn * rate); // constant rate so we can use a normal multiplication

	if ((diff >= 0 && diff < change) ||
	    (diff < 0 && diff > -change))
	{
		// got to the target direction
		retval = target;
	}
	else if (diff > 0)
	{
		// Target dir is greater than current
		if (diff < TRIG_DEGREES / 2)
		{
			// Simple case - just increase towards target
			retval += change;
		}
		else
		{
			// decrease to target, but could go over 0 boundary */
			retval -= change;
		}
	}
	else
	{
		if (diff > -(TRIG_DEGREES/2))
		{
			// Simple case - just decrease towards target
			retval -= change;
		}
		else
		{
			// increase to target, but could go over 0 boundary
			retval += change;
		}
	}

	retval = wrapf(retval, 360.0f);

	ASSERT(retval <= 360.0f && retval >= 0.0f, "moveCalcTurn: bad angle %f from (%f, %f, %u)\n",
	       retval, *pCurr, target, rate);

	*pCurr = retval;
}


/* Get the next target point from the route */
static BOOL moveNextTarget(DROID *psDroid)
{
	UDWORD	srcX,srcY, tarX, tarY;

	CHECK_DROID(psDroid);

	// See if there is anything left in the move list
	if (psDroid->sMove.Position == psDroid->sMove.numPoints)
	{
		return false;
	}

	tarX = world_coord(psDroid->sMove.asPath[psDroid->sMove.Position].x) + TILE_UNITS / 2;
	tarY = world_coord(psDroid->sMove.asPath[psDroid->sMove.Position].y) + TILE_UNITS / 2;
	if (psDroid->sMove.Position == 0)
	{
		psDroid->sMove.srcX = (SDWORD)psDroid->pos.x;
		psDroid->sMove.srcY = (SDWORD)psDroid->pos.y;
	}
	else
	{
		srcX = world_coord(psDroid->sMove.asPath[psDroid->sMove.Position -1].x) + TILE_UNITS / 2;
		srcY = world_coord(psDroid->sMove.asPath[psDroid->sMove.Position -1].y) + TILE_UNITS / 2;
		psDroid->sMove.srcX = srcX;
		psDroid->sMove.srcY = srcY ;
	}
	psDroid->sMove.targetX = tarX;
	psDroid->sMove.targetY = tarY;
	psDroid->sMove.Position++;

	CHECK_DROID(psDroid);
	return true;
}

/* Look at the next target point from the route */
static Vector2i movePeekNextTarget(DROID *psDroid)
{
	// See if there is anything left in the move list
	if (psDroid->sMove.Position == psDroid->sMove.numPoints)
	{
		// No points left - fudge one to continue the same direction
		Vector2i
			src = { psDroid->sMove.srcX, psDroid->sMove.srcY },
			target = { psDroid->sMove.targetX, psDroid->sMove.targetY },
			diff = Vector2i_Sub(target, src),
			p = Vector2i_Add(diff, target);
		return p;
	}
	else
	{
		Vector2i p = {
			world_coord(psDroid->sMove.asPath[psDroid->sMove.Position].x) + TILE_UNITS/2,
			world_coord(psDroid->sMove.asPath[psDroid->sMove.Position].y) + TILE_UNITS/2
		};
		return p;
	}
}

// Watermelon:fix these magic number...the collision radius should be based on pie imd radius not some static int's...
static	int mvPersRad = 20, mvCybRad = 30, mvSmRad = 40, mvMedRad = 50, mvLgRad = 60;

// Get the radius of a base object for collision
static SDWORD moveObjRadius(const BASE_OBJECT* psObj)
{
	switch (psObj->type)
	{
		case OBJ_DROID:
		{
			const DROID* psDroid = (const DROID*)psObj;
			if (psDroid->droidType == DROID_PERSON)
			{
				return mvPersRad;
			}
			//else if (psDroid->droidType == DROID_CYBORG)
			else if (cyborgDroid(psDroid))
			{
				return mvCybRad;
			}
			else
			{
				const BODY_STATS* psBdyStats = &asBodyStats[psDroid->asBits[COMP_BODY].nStat];
				switch (psBdyStats->size)
				{
					case SIZE_LIGHT:
						return mvSmRad;

					case SIZE_MEDIUM:
						return mvMedRad;

					case SIZE_HEAVY:
						return mvLgRad;

					case SIZE_SUPER_HEAVY:
						return 130;

					default:
						return psDroid->sDisplay.imd->radius;
				}
			}
			break;
		}
		case OBJ_STRUCTURE:
//			return psObj->sDisplay.imd->visRadius;
			return psObj->sDisplay.imd->radius / 2;

		case OBJ_FEATURE:
//			return psObj->sDisplay.imd->visRadius;
			return psObj->sDisplay.imd->radius / 2;

		default:
			ASSERT(!"Unknown object type", "moveObjRadius: unknown object type");
			return 0;
	}
}


// see if a Droid has run over a person
static void moveCheckSquished(DROID *psDroid, float mx,float my)
{
	SDWORD		i, droidR, rad, radSq;
	SDWORD		objR;
	SDWORD		xdiff,ydiff, distSq;
	NAYBOR_INFO	*psInfo;

	droidR = moveObjRadius((BASE_OBJECT *)psDroid);

	for(i=0; i<(SDWORD)numNaybors; i++)
	{
		psInfo = asDroidNaybors + i;
		if (psInfo->psObj->type != OBJ_DROID ||
			((DROID *)psInfo->psObj)->droidType != DROID_PERSON)
		{
			// ignore everything but people
			continue;
		}

		ASSERT( psInfo->psObj->type == OBJ_DROID &&
				((DROID *)psInfo->psObj)->droidType == DROID_PERSON,
			"squished - eerk" );

		objR = moveObjRadius(psInfo->psObj);
		rad = droidR + objR;
		radSq = rad*rad;

		xdiff = (SDWORD)psDroid->pos.x + mx - psInfo->psObj->pos.x;
		ydiff = (SDWORD)psDroid->pos.y + my - psInfo->psObj->pos.y;
		distSq = xdiff*xdiff + ydiff*ydiff;

		if (((2*radSq)/3) > distSq)
		{
			if ( (psDroid->player != psInfo->psObj->player) &&
				 !aiCheckAlliances(psDroid->player, psInfo->psObj->player) )
			{
				// run over a bloke - kill him
				destroyDroid((DROID*)psInfo->psObj);
				scoreUpdateVar(WD_BARBARIANS_MOWED_DOWN);
				continue;
			}
		}
		else if (psInfo->distSqr > OBJ_MAXRADIUS*OBJ_MAXRADIUS)
		{
			// object is too far away to be hit
			break;
		}
	}
}


// See if the droid has been stopped long enough to give up on the move
static BOOL moveBlocked(DROID *psDroid)
{
	SDWORD	xdiff,ydiff, diffSq;
	UDWORD	blockTime;

	if ((psDroid->sMove.bumpTime == 0) || (psDroid->sMove.bumpTime > gameTime) ||
		(psDroid->sMove.Status == MOVEROUTE) || (psDroid->sMove.Status == MOVEROUTESHUFFLE))
	{
		// no bump - can't be blocked
		return false;
	}

	// See if the block can be cancelled
	if (dirDiff(psDroid->direction, psDroid->sMove.bumpDir) > BLOCK_DIR)
	{
		// Move on, clear the bump
		psDroid->sMove.bumpTime = 0;
		psDroid->sMove.lastBump = 0;
		return false;
	}
	xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->sMove.bumpX;
	ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->sMove.bumpY;
	diffSq = xdiff*xdiff + ydiff*ydiff;
	if (diffSq > BLOCK_DIST*BLOCK_DIST)
	{
		// Move on, clear the bump
		psDroid->sMove.bumpTime = 0;
		psDroid->sMove.lastBump = 0;
		return false;
	}

	if (psDroid->sMove.Status == MOVESHUFFLE)
	{
		blockTime = SHUFFLE_BLOCK_TIME;
	}
	else
	{
		blockTime = BLOCK_TIME;
	}

	if (gameTime - psDroid->sMove.bumpTime > blockTime)
	{
		// Stopped long enough - blocked
		psDroid->sMove.bumpTime = 0;
		psDroid->sMove.lastBump = 0;

		objTrace(psDroid->id, "BLOCKED");
		// if the unit cannot see the next way point - reroute it's got stuck
		if ( ( bMultiPlayer || (psDroid->player == selectedPlayer) ) &&
			(psDroid->sMove.Position != psDroid->sMove.numPoints) &&
			!fpathTileLOS(map_coord((SDWORD)psDroid->pos.x), map_coord((SDWORD)psDroid->pos.y),
						  map_coord(psDroid->sMove.DestinationX), map_coord(psDroid->sMove.DestinationY)))
		{
			objTrace(psDroid->id, "Trying to reroute to (%d,%d)", psDroid->sMove.DestinationX, psDroid->sMove.DestinationY);
			moveDroidTo(psDroid, world_coord(psDroid->sMove.DestinationX), world_coord(psDroid->sMove.DestinationY));
			return false;
		}

		return true;
	}

	return false;
}


// Calculate the actual movement to slide around
static void moveCalcSlideVector(DROID *psDroid,SDWORD objX, SDWORD objY, float *pMx, float *pMy)
{
	SDWORD		obstX, obstY;
	SDWORD		absX, absY;
	SDWORD		dirX, dirY, dirMag;
	float		mx, my, unitX,unitY;
	float		dotRes;

	mx = *pMx;
	my = *pMy;

	// Calculate the vector to the obstruction
	obstX = psDroid->pos.x - objX;
	obstY = psDroid->pos.y - objY;

	// if the target dir is the same, don't need to slide
	if (obstX*mx + obstY*my >= 0)
	{
		return;
	}

	// Choose the tangent vector to this on the same side as the target
	dotRes = (float)obstY * mx - (float)obstX * my;
	if (dotRes >= 0)
	{
		dirX = obstY;
		dirY = -obstX;
	}
	else
	{
		dirX = -obstY;
		dirY = obstX;
		dotRes = (float)dirX * *pMx + (float)dirY * *pMy;
	}
	absX = labs(dirX); absY = labs(dirY);
	dirMag = absX > absY ? absX + absY/2 : absY + absX/2;

	// Calculate the component of the movement in the direction of the tangent vector
	unitX = (float)dirX / (float)dirMag;
	unitY = (float)dirY / (float)dirMag;
	dotRes = dotRes / (float)dirMag;
	*pMx = unitX * dotRes;
	*pMy = unitY * dotRes;
}


// see if a droid has run into a blocking tile
static void moveCalcBlockingSlide(DROID *psDroid, float *pmx, float *pmy, SDWORD tarDir, SDWORD *pSlideDir)
{
	float	mx = *pmx,my = *pmy, nx,ny;
	SDWORD	tx,ty, ntx,nty;		// current tile x,y and new tile x,y
	SDWORD	blkCX,blkCY;
	SDWORD	horizX,horizY, vertX,vertY;
	BOOL	blocked;
	SDWORD	slideDir;
	PROPULSION_TYPE	propulsion = getPropulsionStats(psDroid)->propulsionType;

	CHECK_DROID(psDroid);

	blocked = false;

	// calculate the new coords and see if they are on a different tile
	tx = map_coord(psDroid->sMove.fx);
	ty = map_coord(psDroid->sMove.fy);
	nx = psDroid->sMove.fx + mx;
	ny = psDroid->sMove.fy + my;
	ntx = map_coord(nx);
	nty = map_coord(ny);

	// is the new tile blocking?
	if (fpathBlockingTile(ntx, nty, propulsion))
	{
		blocked = true;
	}

	blkCX = world_coord(ntx) + TILE_UNITS/2;
	blkCY = world_coord(nty) + TILE_UNITS/2;

	// is the new tile blocking?
	if (!blocked)
	{
		// not blocking, don't change the move vector
		return;
	}

	// if the droid is shuffling - just stop
	if (psDroid->sMove.Status == MOVESHUFFLE)
	{
		psDroid->sMove.Status = MOVEINACTIVE;
	}

	// note the bump time and position if necessary
	if (!vtolDroid(psDroid) &&
		psDroid->sMove.bumpTime == 0)
	{
		psDroid->sMove.bumpTime = gameTime;
		psDroid->sMove.lastBump = 0;
		psDroid->sMove.pauseTime = 0;
		psDroid->sMove.bumpX = psDroid->pos.x;
		psDroid->sMove.bumpY = psDroid->pos.y;
		psDroid->sMove.bumpDir = (SWORD)psDroid->direction;
	}

	if (tx != ntx && ty != nty)
	{
		// moved diagonally

		// figure out where the other two possible blocking tiles are
		horizX = mx < 0 ? ntx + 1 : ntx - 1;
		horizY = nty;

		vertX = ntx;
		vertY = my < 0 ? nty + 1 : nty - 1;

		if (fpathBlockingTile(horizX, horizY, propulsion) && fpathBlockingTile(vertX, vertY, propulsion))
		{
			// in a corner - choose an arbitrary slide
			if (ONEINTWO)
			{
				*pmx = 0;
				*pmy = -*pmy;
			}
			else
			{
				*pmx = -*pmx;
				*pmy = 0;
			}
		}
		else if (fpathBlockingTile(horizX, horizY, propulsion))
		{
			*pmy = 0;
		}
		else if (fpathBlockingTile(vertX, vertY, propulsion))
		{
			*pmx = 0;
		}
		else
		{
			moveCalcSlideVector(psDroid, blkCX,blkCY, pmx,pmy);
		}
	}
	else if (tx != ntx)
	{
		// moved horizontally - see which half of the tile were in
		if ((psDroid->pos.y & TILE_MASK) > TILE_UNITS/2)
		{
			// top half
			if (fpathBlockingTile(ntx, nty + 1, propulsion))
			{
				*pmx = 0;
			}
			else
			{
				moveCalcSlideVector(psDroid, blkCX,blkCY, pmx,pmy);
			}
		}
		else
		{
			// bottom half
			if (fpathBlockingTile(ntx, nty - 1, propulsion))
			{
				*pmx = 0;
			}
			else
			{
				moveCalcSlideVector(psDroid, blkCX,blkCY, pmx,pmy);
			}
		}
	}
	else if (ty != nty)
	{
		// moved vertically
		if ((psDroid->pos.x & TILE_MASK) > TILE_UNITS/2)
		{
			// top half
			if (fpathBlockingTile(ntx + 1, nty, propulsion))
			{
				*pmy = 0;
			}
			else
			{
				moveCalcSlideVector(psDroid, blkCX,blkCY, pmx,pmy);
			}
		}
		else
		{
			// bottom half
			if (fpathBlockingTile(ntx - 1, nty, propulsion))
			{
				*pmy = 0;
			}
			else
			{
				moveCalcSlideVector(psDroid, blkCX,blkCY, pmx,pmy);
			}
		}
	}
	else // if (tx == ntx && ty == nty)
	{
		// on a blocking tile - see if we need to jump off
		int	intx = ((int)psDroid->sMove.fx) & TILE_MASK;
		int	inty = ((int)psDroid->sMove.fy) & TILE_MASK;
		BOOL	bJumped = false;
		int	jumpx = psDroid->pos.x;
		int	jumpy = psDroid->pos.y;

		if (intx < TILE_UNITS/2)
		{
			if (inty < TILE_UNITS/2)
			{
				// top left
				if ((mx < 0) && fpathBlockingTile(tx - 1, ty, propulsion))
				{
					bJumped = true;
					jumpy = (jumpy & ~TILE_MASK) -1;
				}
				if ((my < 0) && fpathBlockingTile(tx, ty - 1, propulsion))
				{
					bJumped = true;
					jumpx = (jumpx & ~TILE_MASK) -1;
				}
			}
			else
			{
				// bottom left
				if ((mx < 0) && fpathBlockingTile(tx - 1, ty, propulsion))
				{
					bJumped = true;
					jumpy = (jumpy & ~TILE_MASK) + TILE_UNITS;
				}
				if ((my >= 0) && fpathBlockingTile(tx, ty + 1, propulsion))
				{
					bJumped = true;
					jumpx = (jumpx & ~TILE_MASK) -1;
				}
			}
		}
		else
		{
			if (inty < TILE_UNITS/2)
			{
				// top right
				if ((mx >= 0) && fpathBlockingTile(tx + 1, ty, propulsion))
				{
					bJumped = true;
					jumpy = (jumpy & ~TILE_MASK) -1;
				}
				if ((my < 0) && fpathBlockingTile(tx, ty - 1, propulsion))
				{
					bJumped = true;
					jumpx = (jumpx & ~TILE_MASK) + TILE_UNITS;
				}
			}
			else
			{
				// bottom right
				if ((mx >= 0) && fpathBlockingTile(tx + 1, ty, propulsion))
				{
					bJumped = true;
					jumpy = (jumpy & ~TILE_MASK) + TILE_UNITS;
				}
				if ((my >= 0) && fpathBlockingTile(tx, ty + 1, propulsion))
				{
					bJumped = true;
					jumpx = (jumpx & ~TILE_MASK) + TILE_UNITS;
				}
			}
		}

		if (bJumped)
		{
			psDroid->pos.x = MAX(0, jumpx);
			psDroid->pos.y = MAX(0, jumpy);
			psDroid->sMove.fx = MAX(0, jumpx);
			psDroid->sMove.fy = MAX(0, jumpy);
			*pmx = 0;
			*pmy = 0;
		}
		else
		{
			moveCalcSlideVector(psDroid, blkCX,blkCY, pmx,pmy);
		}
	}

	slideDir = vectorToAngle(*pmx,*pmy);
	if (ntx != tx)
	{
		// hit a horizontal block
		if ((tarDir < 90 || tarDir > 270) &&
			(slideDir >= 90 && slideDir <= 270))
		{
			slideDir = tarDir;
		}
		else if ((tarDir >= 90 && tarDir <= 270) &&
				 (slideDir < 90 || slideDir > 270))
		{
			slideDir = tarDir;
		}
	}
	if (nty != ty)
	{
		// hit a vertical block
		if ((tarDir < 180) &&
			(slideDir >= 180))
		{
			slideDir = tarDir;
		}
		else if ((tarDir >= 180) &&
				 (slideDir < 180))
		{
			slideDir = tarDir;
		}
	}
	*pSlideDir = slideDir;

	CHECK_DROID(psDroid);
}


// see if a droid has run into another droid
// Only consider stationery droids
static void moveCalcDroidSlide(DROID *psDroid, float *pmx, float *pmy)
{
	SDWORD		i, droidR, rad, radSq;
	SDWORD		objR;
	SDWORD		xdiff,ydiff, distSq;
	NAYBOR_INFO	*psInfo;
	BASE_OBJECT	*psObst;
	BOOL		bLegs;

	CHECK_DROID(psDroid);

	bLegs = false;
	if (psDroid->droidType == DROID_PERSON
//	 || psDroid->droidType == DROID_CYBORG)
	 || cyborgDroid(psDroid))
	{
		bLegs = true;
	}

	droidR = moveObjRadius((BASE_OBJECT *)psDroid);
	psObst = NULL;
	for(i=0; i<(SDWORD)numNaybors; i++)
	{
		psInfo = asDroidNaybors + i;
		if (psInfo->psObj->type == OBJ_DROID)
		{
			if ( ((DROID *)psInfo->psObj)->droidType == DROID_TRANSPORTER )
			{
				// ignore transporters
				continue;
			}

			if (bLegs
			 && ((DROID *)psInfo->psObj)->droidType != DROID_PERSON
//			 && ((DROID *)psInfo->psObj)->droidType != DROID_CYBORG)
			 && !cyborgDroid((DROID *)psInfo->psObj))
			{
				// cyborgs/people only avoid other cyborgs/people
				continue;
			}
			if (!bLegs &&
				(((DROID *)psInfo->psObj)->droidType == DROID_PERSON))
			{
				// everything else doesn't avoid people
				continue;
			}
		}
		else
		{
			// ignore anything that isn't a droid
			continue;
		}

		objR = moveObjRadius(psInfo->psObj);
		rad = droidR + objR;
		radSq = rad*rad;

		xdiff = psDroid->sMove.fx + *pmx - psInfo->psObj->pos.x;
		ydiff = psDroid->sMove.fy + *pmy - psInfo->psObj->pos.y;
		distSq = xdiff * xdiff + ydiff * ydiff;
		if ((float)xdiff * *pmx + (float)ydiff * *pmy >= 0)
		{
			// object behind
			continue;
		}

		if (radSq > distSq)
		{
			if (psObst != NULL || !aiCheckAlliances(psInfo->psObj->player, psDroid->player))
			{
				// hit more than one droid - stop
				*pmx = (float)0;
				*pmy = (float)0;
				psObst = NULL;
				break;
			}
			else
			{
//				if (((DROID *)psInfo->psObj)->sMove.Status == MOVEINACTIVE)
				psObst = psInfo->psObj;

				// note the bump time and position if necessary
				if (psDroid->sMove.bumpTime == 0)
				{
					psDroid->sMove.bumpTime = gameTime;
					psDroid->sMove.lastBump = 0;
					psDroid->sMove.pauseTime = 0;
					psDroid->sMove.bumpX = psDroid->pos.x;
					psDroid->sMove.bumpY = psDroid->pos.y;
					psDroid->sMove.bumpDir = (SWORD)psDroid->direction;
				}
				else
				{
					psDroid->sMove.lastBump = (UWORD)(gameTime - psDroid->sMove.bumpTime);
				}

				// tell inactive droids to get out the way
				if ((psObst->type == OBJ_DROID) &&
					aiCheckAlliances(psObst->player, psDroid->player) &&
					((((DROID *)psObst)->sMove.Status == MOVEINACTIVE) ||
					 (((DROID *)psObst)->sMove.Status == MOVEROUTE)) )
				{
					if (psDroid->sMove.Status == MOVESHUFFLE)
					{
						moveShuffleDroid( (DROID *)psObst, psDroid->sMove.shuffleStart,
							psDroid->sMove.targetX - (SDWORD)psDroid->pos.x,
							psDroid->sMove.targetY - (SDWORD)psDroid->pos.y);
					}
					else
					{
						moveShuffleDroid( (DROID *)psObst, gameTime,
							psDroid->sMove.targetX - (SDWORD)psDroid->pos.x,
							psDroid->sMove.targetY - (SDWORD)psDroid->pos.y);
					}
				}
			}
		}
		else if (psInfo->distSqr > OBJ_MAXRADIUS*OBJ_MAXRADIUS)
		{
			// object is too far away to be hit
			break;
		}
	}

	if (psObst != NULL)
	{
		// Try to slide round it
		moveCalcSlideVector(psDroid, (SDWORD)psObst->pos.x,(SDWORD)psObst->pos.y, pmx,pmy);
	}
	CHECK_DROID(psDroid);
}

// get an obstacle avoidance vector
static void moveGetObstacleVector(DROID *psDroid, float *pX, float *pY)
{
	SDWORD				i, xdiff, ydiff, absx, absy, dist;
	BASE_OBJECT			*psObj;
	SDWORD				numObst = 0, distTot = 0;
	float				dirX = 0, dirY = 0;
	float				omag, ox, oy, ratio;
	float				avoidX, avoidY;
	SDWORD				mapX, mapY, tx, ty, td;
	PROPULSION_STATS		*psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;

	ASSERT(psPropStats, "invalid propulsion stats pointer");

	droidGetNaybors(psDroid);

	// scan the neighbours for obstacles
	for(i = 0; i < (SDWORD)numNaybors; i++)
	{
		psObj = asDroidNaybors[i].psObj;
		if (psObj->type != OBJ_DROID ||
			asDroidNaybors[i].distSqr >= AVOID_DIST*AVOID_DIST)
		{
			// object too far away to worry about
			continue;
		}

		// vtol droids only avoid each other and don't affect ground droids
		if ( (vtolDroid(psDroid) && (psObj->type != OBJ_DROID || !vtolDroid((DROID *)psObj))) ||
			 (!vtolDroid(psDroid) && psObj->type == OBJ_DROID && vtolDroid((DROID *)psObj)) )
		{
			continue;
		}

		if (((DROID *)psObj)->droidType == DROID_TRANSPORTER ||
			(((DROID *)psObj)->droidType == DROID_PERSON &&
			 psObj->player != psDroid->player))
		{
			// don't avoid people on the other side - run over them
			continue;
		}
		xdiff = (SDWORD)psObj->pos.x - (SDWORD)psDroid->pos.x;
		ydiff = (SDWORD)psObj->pos.y - (SDWORD)psDroid->pos.y;
		if ((float)xdiff * *pX + (float)ydiff * *pY < 0)
		{
			// object behind
			continue;
		}

		absx = labs(xdiff);
		absy = labs(ydiff);
		dist = absx > absy ? absx + absy/2 : absx/2 + absy;

		if (dist != 0)
		{
			dirX += (float)xdiff / (float)(dist * dist);
			dirY += (float)ydiff / (float)(dist * dist);
			distTot += dist*dist;
			numObst += 1;
		}
		else
		{
			dirX += xdiff;
			dirY += ydiff;
			numObst += 1;
		}
	}

	// now scan for blocking tiles
	mapX = map_coord(psDroid->pos.x);
	mapY = map_coord(psDroid->pos.y);
	for(ydiff=-2; ydiff<=2; ydiff++)
	{
		for(xdiff=-2; xdiff<=2; xdiff++)
		{
			if ((float)xdiff * *pX + (float)ydiff * *pY <= 0)
			{
				// object behind
				continue;
			}
			if (fpathBlockingTile(mapX + xdiff, mapY + ydiff, psPropStats->propulsionType))
			{
				tx = world_coord(xdiff);
				ty = world_coord(ydiff);
				td = tx*tx + ty*ty;
				if (td < AVOID_DIST*AVOID_DIST)
				{
					absx = labs(tx);
					absy = labs(ty);
					dist = absx > absy ? absx + absy/2 : absx/2 + absy;

					if (dist != 0)
					{
						dirX += (float)tx / (float)(dist * dist);
						dirY += (float)ty / (float)(dist * dist);
						distTot += dist*dist;
						numObst += 1;
					}
				}
			}
		}
	}

	if (numObst > 0)
	{
		distTot /= numObst;

		// Create the avoid vector
		if (dirX == 0 && dirY == 0)
		{
			avoidX = 0;
			avoidY = 0;
			distTot = AVOID_DIST*AVOID_DIST;
		}
		else
		{
			omag = sqrtf(dirX*dirX + dirY*dirY);
			ox = dirX / omag;
			oy = dirY / omag;
			if (*pX * oy + *pY * -ox < 0)
			{
// 				debug( LOG_NEVER, "First perp\n");
				avoidX = -oy;
				avoidY = ox;
			}
			else
			{
// 				debug( LOG_NEVER, "Second perp\n");
				avoidX = oy;
				avoidY = -ox;
			}
		}

		// combine the avoid vector and the target vector
		ratio = (float)distTot / (float)(AVOID_DIST * AVOID_DIST);
		if (ratio > 1)
		{
			ratio = 1;
		}

		*pX = *pX * ratio + avoidX * (1.f - ratio);
		*pY = *pY * ratio + avoidY * (1.f - ratio);
	}
	ASSERT(isfinite(*pX) && isfinite(*pY), "moveGetObstacleVector: bad float");
}


/*!
 * Get a direction for a droid to avoid obstacles etc.
 * \param psDroid Which droid to examine
 * \return The normalised direction vector
 */
static Vector2f moveGetDirection(DROID *psDroid)
{
	// Current position and destination direction
	Vector2f
		src = { psDroid->pos.x, psDroid->pos.y },
		dest = { 0.0f, 0.0f };
	// Current waypoint
	Vector2f
		target = { psDroid->sMove.targetX, psDroid->sMove.targetY },
		delta = Vector2f_Sub(target, src);
	float magnitude = Vector2f_ScalarP(delta, delta);

	// Dont fade in the next target point if we are at finished or too far away from the current
	if (psDroid->sMove.Position == psDroid->sMove.numPoints || magnitude > WAYPOINT_DSQ)
	{
		dest = Vector2f_Normalise(delta);
	}
	// We are in reach of the current waypoint and have further points to go
	else
	{
		// Next waypoint
		Vector2f
			nextTarget = Vector2i_To2f( movePeekNextTarget(psDroid) ),
			nextDelta = Vector2f_Sub(nextTarget, src);
		float nextMagnitude = Vector2f_ScalarP(nextDelta, nextDelta);

		// We are already there
		if (magnitude == 0.0f && nextMagnitude == 0.0f)
		{
			Vector2f zero = {0.0f, 0.0f};
			return zero; // We are practically standing on our only waypoint
		}
		// We are passing the current waypoint, so directly head over to the next
		else if (magnitude == 0.0f)
		{
			dest = Vector2f_Normalise(nextDelta);
		}
		// We are passing the next waypoint, so for now don't interpolate it
		else if (nextMagnitude == 0.0f)
		{
			dest = Vector2f_Normalise(delta);
		}
		// Interpolate with the next target
		else
		{
			// Interpolate the vectors based on how close to them we are
			delta = Vector2f_Mult(delta, magnitude);
			nextDelta = Vector2f_Mult(nextDelta, WAYPOINT_DSQ - magnitude);

			dest = Vector2f_Normalise( Vector2f_Add(delta, nextDelta) );
		}
	}

	ASSERT(isfinite(dest.x) && isfinite(dest.y), "moveGetDirection: bad float, early check");

	// Transporters don't need to avoid obstacles, but everyone else should
	if ( psDroid->droidType != DROID_TRANSPORTER )
	{
		moveGetObstacleVector(psDroid, &dest.x, &dest.y);
	}

	ASSERT(isfinite(dest.x) && isfinite(dest.y), "moveGetDirection: bad float");

	return dest;
}


// Calculate the boundary vector
static void moveCalcBoundary(DROID *psDroid)
{
	Vector2i
		src = { psDroid->sMove.srcX, psDroid->sMove.srcY },
		target = { psDroid->sMove.targetX, psDroid->sMove.targetY },
		prev, next, nextTarget;
	SDWORD	absX, absY;
	SDWORD	prevMag, nextMag;
	SDWORD	sumX, sumY;

	// No points left - simple case for the bound vector
	if (psDroid->sMove.Position == psDroid->sMove.numPoints)
	{
		psDroid->sMove.boundX = src.x - target.x;
		psDroid->sMove.boundY = src.y - target.y;
		return;
	}

	// Calculate the vector back along the current path
	prev = Vector2i_Sub(src, target),
	absX = abs(prev.x);
	absY = abs(prev.y);
	prevMag = (absX > absY) ? (absX + absY/2) : (absY + absX/2);
//	prevMag = Vector2i_Length(prev);

	// Calculate the vector to the next target
	nextTarget = movePeekNextTarget(psDroid);

	next = Vector2i_Sub(nextTarget, target);
	absX = abs(next.x);
	absY = abs(next.y);
	nextMag = (absX > absY) ? (absX + absY/2) : (absY + absX/2);
//	nextMag = Vector2i_Length(next);

	if (prevMag != 0 && nextMag == 0)
	{
		// don't bother mixing the boundaries - cos there isn't a next vector anyway
		psDroid->sMove.boundX = src.x - target.x;
		psDroid->sMove.boundY = src.y - target.y;
		return;
	}
	else if (prevMag == 0 || nextMag == 0)
	{
		psDroid->sMove.boundX = 0;
		psDroid->sMove.boundY = 0;
		return;
	}

	// Calculate the vector between the two
	sumX = (prev.x * BOUND_ACC)/prevMag + (next.x * BOUND_ACC)/nextMag;
	sumY = (prev.y * BOUND_ACC)/prevMag + (next.y * BOUND_ACC)/nextMag;

	// Rotate by 90 degrees one way and see if it is the same side as the src vector
	// if not rotate 90 the other.
	if (prev.y*sumY - prev.y*sumX < 0)
	{
		psDroid->sMove.boundX = -sumY;
		psDroid->sMove.boundY = sumX;
	}
	else
	{
		psDroid->sMove.boundX = sumY;
		psDroid->sMove.boundY = -sumX;
	}

	debug( LOG_NEVER, "new boundary: droid %d boundary (%d,%d)\n",
			psDroid->id, psDroid->sMove.boundX, psDroid->sMove.boundY);
}


// Check if a droid has got to a way point
static BOOL moveReachedWayPoint(DROID *psDroid)
{
	SDWORD	droidX,droidY, iRange;

	// Calculate the vector to the droid
	droidX = (SDWORD)psDroid->pos.x - psDroid->sMove.targetX;
	droidY = (SDWORD)psDroid->pos.y - psDroid->sMove.targetY;

	// see if this is a formation end point
	if (psDroid->droidType == DROID_TRANSPORTER ||
		(psDroid->sMove.psFormation &&
		 formationMember(psDroid->sMove.psFormation, psDroid)) ||
		 (vtolDroid(psDroid) && (psDroid->sMove.numPoints == psDroid->sMove.Position)) )
//							 && (psDroid->action != DACTION_VTOLATTACK)) )
	{
		if ( psDroid->droidType == DROID_TRANSPORTER )
		{

			iRange = TILE_UNITS/4;

		}
		else
		{
			iRange = TILE_UNITS/4;
		}

		if (droidX*droidX + droidY*droidY < iRange*iRange)
		{
			return true;
		}
	}
	else
	{
		// if the dot product is -ve the droid has got past the way point
		// but only move onto the next way point if we can see the previous one
		// (this helps units that have got nudged off course).
		if ((psDroid->sMove.boundX * droidX + psDroid->sMove.boundY * droidY <= 0) &&
			fpathTileLOS(map_coord(psDroid->pos.x), map_coord(psDroid->pos.y), map_coord(psDroid->sMove.targetX), map_coord(psDroid->sMove.targetY)))
		{
			objTrace(psDroid->id, "Next waypoint: droid %d bound (%d,%d) target (%d,%d)",
			         (int)psDroid->id, (int)psDroid->sMove.boundX, (int)psDroid->sMove.boundY, (int)droidX, (int)droidY);

			return true;
		}
	}

	return false;
}

void moveToggleFormationSpeedLimiting( void )
{
	g_bFormationSpeedLimitingOn = !g_bFormationSpeedLimitingOn;
}

void moveSetFormationSpeedLimiting( BOOL bVal )
{
	g_bFormationSpeedLimitingOn = bVal;
}

BOOL moveFormationSpeedLimitingOn( void )
{
	return g_bFormationSpeedLimitingOn;
}

#define MAX_SPEED_PITCH  60

/** Calculate the new speed for a droid based on factors like damage and pitch.
 *  @todo Remove hack for steep slopes not properly marked as blocking on some maps.
 */
SDWORD moveCalcDroidSpeed(DROID *psDroid)
{
	UDWORD			mapX,mapY, damLevel;
	SDWORD			speed, pitch;
	WEAPON_STATS	*psWStats;

	CHECK_DROID(psDroid);

	mapX = map_coord(psDroid->pos.x);
	mapY = map_coord(psDroid->pos.y);
	speed = (SDWORD) calcDroidSpeed(psDroid->baseSpeed, terrainType(mapTile(mapX,mapY)),
							  psDroid->asBits[COMP_PROPULSION].nStat,
							  getDroidEffectiveLevel(psDroid));

	pitch = psDroid->pitch;
	if (pitch > MAX_SPEED_PITCH)
	{
		pitch = MAX_SPEED_PITCH;
	}
	else if (pitch < - MAX_SPEED_PITCH)
	{
		pitch = -MAX_SPEED_PITCH;
	}
	// now offset the speed for the slope of the droid
	speed = (MAX_SPEED_PITCH - pitch) * speed / MAX_SPEED_PITCH;
	if (speed <= 0)
	{
		// Very nasty hack to deal with buggy maps, where some cliffs are
		// not properly marked as being cliffs, but too steep to drive over.
		// This confuses the heck out of the path-finding code! - Per
		speed = 10;
	}

	// slow down damaged droids
	damLevel = PERCENT(psDroid->body, psDroid->originalBody);
	if (damLevel < HEAVY_DAMAGE_LEVEL)
	{
		speed = 2 * speed / 3;
	}

	// stop droids that have just fired a no fire while moving weapon
	if (psDroid->numWeaps > 0)
	{
		if (psDroid->asWeaps[0].nStat > 0 && psDroid->asWeaps[0].lastFired + FOM_MOVEPAUSE > gameTime)
		{
			psWStats = asWeaponStats + psDroid->asWeaps[0].nStat;
			if (psWStats->fireOnMove == FOM_NO)
			{
				speed = 0;
			}
		}
	}

	/* adjust speed for formation */
	if(!vtolDroid(psDroid) &&
		moveFormationSpeedLimitingOn() && psDroid->sMove.psFormation)
	{
		SDWORD FrmSpeed = (SDWORD)psDroid->sMove.psFormation->iSpeed;

		if ( speed > FrmSpeed )
		{
			speed = FrmSpeed;
		}
	}

	// slow down shuffling VTOLs
	if (vtolDroid(psDroid) &&
		(psDroid->sMove.Status == MOVESHUFFLE) &&
		(speed > MIN_END_SPEED))
	{
		speed = MIN_END_SPEED;
	}

	CHECK_DROID(psDroid);

	return speed;
}

/** Determine whether a droid has stopped moving.
 *  @return true if the droid doesn't move, false if it's moving.
 */
static BOOL moveDroidStopped(DROID* psDroid, SDWORD speed)
{
	if ((psDroid->sMove.Status == MOVEINACTIVE
	  || psDroid->sMove.Status == MOVEROUTE)
	 && speed == 0
	 && psDroid->sMove.speed == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

static void moveUpdateDroidDirection( DROID *psDroid, SDWORD *pSpeed, SDWORD direction,
		SDWORD iSpinAngle, SDWORD iSpinSpeed, SDWORD iTurnSpeed, float *pDroidDir,
		float *pfSpeed ) // direction is target-direction
{
	float		adiff;
	float		temp;

	*pfSpeed = *pSpeed;
	*pDroidDir = psDroid->direction;

	// don't move if in MOVEPAUSE state
	if (psDroid->sMove.Status == MOVEPAUSE)
	{
		return;
	}

	temp = *pDroidDir;
	adiff = fabsf(direction - *pDroidDir);
	if (adiff > TRIG_DEGREES/2)
	{
		adiff = TRIG_DEGREES - adiff;
	}
	if (adiff > iSpinAngle)
	{
		// large change in direction, spin on the spot
		moveCalcTurn(&temp, direction, iSpinSpeed);
		*pSpeed = 0;
	}
	else
	{
		// small change in direction, turn while moving
		moveCalcTurn(&temp, direction, iTurnSpeed);
	}

	*pDroidDir = temp;
}


// Calculate current speed perpendicular to droids direction
static float moveCalcPerpSpeed( DROID *psDroid, float iDroidDir, SDWORD iSkidDecel )
{
	float		adiff;
	float		perpSpeed;

	adiff = fabsf(iDroidDir - psDroid->sMove.moveDir);
	perpSpeed = psDroid->sMove.speed * trigSin(adiff);

	// decelerate the perpendicular speed
	perpSpeed -= (iSkidDecel * baseSpeed);
	if (perpSpeed < 0)
	{
		perpSpeed = 0;
	}

	return perpSpeed;
}


static void moveCombineNormalAndPerpSpeeds( DROID *psDroid, float fNormalSpeed,
		float fPerpSpeed, float iDroidDir )
{
	float		finalDir, adiff;
	float		finalSpeed;

	/* set current direction */
	psDroid->direction = iDroidDir;

	/* set normal speed and direction if perpendicular speed is zero */
	if (fPerpSpeed == 0)
	{
		psDroid->sMove.speed = fNormalSpeed;
		psDroid->sMove.moveDir = iDroidDir;
		return;
	}

	finalSpeed = sqrtf(fNormalSpeed * fNormalSpeed + fPerpSpeed * fPerpSpeed);

	// calculate the angle between the droid facing and movement direction
	finalDir = trigInvCos(fNormalSpeed / finalSpeed);

	// choose the finalDir on the same side as the old movement direction
	adiff = fabsf(iDroidDir - psDroid->sMove.moveDir);
	if (adiff < TRIG_DEGREES/2)
	{
		if (iDroidDir > psDroid->sMove.moveDir)
		{
			finalDir = iDroidDir - finalDir;
		}
		else
		{
			finalDir = iDroidDir + finalDir;
		}
	}
	else
	{
		if (iDroidDir > psDroid->sMove.moveDir)
		{
			finalDir = iDroidDir + finalDir;
			if (finalDir >= TRIG_DEGREES)
			{
				finalDir -= TRIG_DEGREES;
			}
		}
		else
		{
			finalDir = iDroidDir - finalDir;
			if (finalDir < 0)
			{
				finalDir += TRIG_DEGREES;
			}
		}
	}

	psDroid->sMove.moveDir = finalDir;
	psDroid->sMove.speed = finalSpeed;
}


// Calculate the current speed in the droids normal direction
static float moveCalcNormalSpeed( DROID *psDroid, float fSpeed, float iDroidDir,
		SDWORD iAccel, SDWORD iDecel )
{
	float		adiff;
	float		normalSpeed;

	adiff = fabsf(iDroidDir - psDroid->sMove.moveDir);
	normalSpeed = psDroid->sMove.speed * trigCos(adiff);

	if (normalSpeed < fSpeed)
	{
		// accelerate
		normalSpeed += (iAccel * baseSpeed);
		if (normalSpeed > fSpeed)
		{
			normalSpeed = fSpeed;
		}
	}
	else
	{
		// decelerate
		normalSpeed -= (iDecel * baseSpeed);
		if (normalSpeed < fSpeed)
		{
			normalSpeed = fSpeed;
		}
	}

	return normalSpeed;
}


static void moveGetDroidPosDiffs( DROID *psDroid, float *pDX, float *pDY )
{
	float	move;

	move = (float)psDroid->sMove.speed * (float)baseSpeed;

	*pDX = move * trigSin(psDroid->sMove.moveDir);
	*pDY = move * trigCos(psDroid->sMove.moveDir);
}

// see if the droid is close to the final way point
static void moveCheckFinalWaypoint( DROID *psDroid, SDWORD *pSpeed )
{
	SDWORD		xdiff,ydiff, distSq;
	SDWORD		minEndSpeed = psDroid->baseSpeed/3;

	if (minEndSpeed > MIN_END_SPEED)
	{
		minEndSpeed = MIN_END_SPEED;
	}

	// don't do this for VTOLs doing attack runs
	if (vtolDroid(psDroid) && (psDroid->action == DACTION_VTOLATTACK))
	{
		return;
	}

	if (*pSpeed > minEndSpeed &&
		(psDroid->sMove.Status != MOVESHUFFLE) &&
		psDroid->sMove.Position == psDroid->sMove.numPoints)
	{
		xdiff = (SDWORD)psDroid->pos.x - psDroid->sMove.targetX;
		ydiff = (SDWORD)psDroid->pos.y - psDroid->sMove.targetY;
		distSq = xdiff*xdiff + ydiff*ydiff;
		if (distSq < END_SPEED_RANGE*END_SPEED_RANGE)
		{
			*pSpeed = (MAX_END_SPEED-minEndSpeed) * distSq
						/ (END_SPEED_RANGE*END_SPEED_RANGE) + minEndSpeed;
		}
	}
}

static void moveUpdateDroidPos( DROID *psDroid, float dx, float dy )
{
	SDWORD	iX = 0, iY = 0;

	CHECK_DROID(psDroid);

	if (psDroid->sMove.Status == MOVEPAUSE)
	{
		// don't actually move if the move is paused
		return;
	}

	psDroid->sMove.fx += dx;
	psDroid->sMove.fy += dy;

	iX = psDroid->sMove.fx;
	iY = psDroid->sMove.fy;

	/* impact if about to go off map else update coordinates */
	if ( worldOnMap( iX, iY ) == false )
	{
		/* transporter going off-world will trigger next map, and is ok */
		ASSERT(psDroid->droidType == DROID_TRANSPORTER, "droid trying to move off the map!")
		if (psDroid->droidType != DROID_TRANSPORTER)
		{
			/* dreadful last-ditch crash-avoiding hack - sort this! - GJ */
			destroyDroid( psDroid );
			return;
		}
	}
	else
	{
		psDroid->pos.x = (UWORD)iX;
		psDroid->pos.y = (UWORD)iY;
	}

	// lovely hack to keep transporters just on the map
	// two weeks to go and the hacks just get better !!!
	if (psDroid->droidType == DROID_TRANSPORTER)
	{
		if (psDroid->pos.x == 0)
		{
			psDroid->pos.x = 1;
		}
		if (psDroid->pos.y == 0)
		{
			psDroid->pos.y = 1;
		}
	}
	CHECK_DROID(psDroid);
}

/* Update a tracked droids position and speed given target values */
static void moveUpdateGroundModel(DROID *psDroid, SDWORD speed, SDWORD direction)
{
	float				fPerpSpeed, fNormalSpeed, dx, dy, fSpeed, bx,by;
	float				iDroidDir;
	SDWORD				slideDir;
	PROPULSION_STATS	*psPropStats;
	SDWORD				spinSpeed, turnSpeed, skidDecel;
	// constants for the different propulsion types
	static SDWORD		hvrSkid = HOVER_SKID_DECEL;
	static SDWORD		whlSkid = WHEELED_SKID_DECEL;
	static SDWORD		trkSkid = TRACKED_SKID_DECEL;
	static float		hvrTurn = 3.f / 4.f;		//0.75f;
	static float		whlTurn = 1.f / 1.f;		//1.0f;
	static float		trkTurn = 1.f / 1.f;		//1.0f;

	CHECK_DROID(psDroid);

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	switch (psPropStats->propulsionType)
	{
	case HOVER:
		spinSpeed = psDroid->baseSpeed * hvrTurn;
		turnSpeed = psDroid->baseSpeed / 3 * hvrTurn;
		skidDecel = hvrSkid;//HOVER_SKID_DECEL;
		break;
	case WHEELED:
		spinSpeed = psDroid->baseSpeed * hvrTurn;
		turnSpeed = psDroid->baseSpeed / 3 * whlTurn;
		skidDecel = whlSkid;//WHEELED_SKID_DECEL;
		break;
	case TRACKED:
	default:
		spinSpeed = psDroid->baseSpeed * hvrTurn;
		turnSpeed = psDroid->baseSpeed / 3 * trkTurn;
		skidDecel = trkSkid;//TRACKED_SKID_DECEL;
		break;
	}

	// nothing to do if the droid is stopped
	if ( moveDroidStopped( psDroid, speed ) == true )
	{
		return;
	}

	// update the naybors list
	droidGetNaybors(psDroid);

	moveCheckFinalWaypoint( psDroid, &speed );

	moveUpdateDroidDirection( psDroid, &speed, direction, TRACKED_SPIN_ANGLE,
				spinSpeed, turnSpeed, &iDroidDir, &fSpeed );

	fNormalSpeed = moveCalcNormalSpeed(psDroid, fSpeed, iDroidDir, TRACKED_ACCEL, TRACKED_DECEL);
	fPerpSpeed   = moveCalcPerpSpeed( psDroid, iDroidDir, skidDecel );

	moveCombineNormalAndPerpSpeeds(psDroid, fNormalSpeed, fPerpSpeed, iDroidDir);
	moveGetDroidPosDiffs( psDroid, &dx, &dy );

	moveCheckSquished(psDroid, dx,dy);
	moveCalcDroidSlide(psDroid, &dx,&dy);
	bx = dx;
	by = dy;
	moveCalcBlockingSlide(psDroid, &bx,&by, direction, &slideDir);
	if (bx != dx || by != dy)
	{
		moveUpdateDroidDirection( psDroid, &speed, slideDir, TRACKED_SPIN_ANGLE,
					psDroid->baseSpeed, psDroid->baseSpeed/3, &iDroidDir, &fSpeed );
		psDroid->direction = iDroidDir;
	}

	moveUpdateDroidPos( psDroid, bx, by );

	//set the droid height here so other routines can use it
	psDroid->pos.z = map_Height(psDroid->pos.x, psDroid->pos.y);//jps 21july96
	updateDroidOrientation(psDroid);
}

/* Update a persons position and speed given target values */
static void moveUpdatePersonModel(DROID *psDroid, SDWORD speed, SDWORD direction)
{
	float			fPerpSpeed, fNormalSpeed, dx, dy, fSpeed;
	float			iDroidDir;
	SDWORD			slideDir;
	BOOL			bRet;

	CHECK_DROID(psDroid);

	// nothing to do if the droid is stopped
	if ( moveDroidStopped( psDroid, speed ) == true )
	{
		if ( psDroid->droidType == DROID_PERSON &&
			 psDroid->order != DORDER_RUNBURN &&
			 (psDroid->action == DACTION_ATTACK ||
			  psDroid->action == DACTION_ROTATETOATTACK) )
		{
			/* remove previous anim */
			if ( psDroid->psCurAnim != NULL &&
				 psDroid->psCurAnim->psAnim->uwID != ID_ANIM_DROIDFIRE )
			{
				bRet = animObj_Remove( &psDroid->psCurAnim, psDroid->psCurAnim->psAnim->uwID );
				ASSERT( bRet == true, "moveUpdatePersonModel: animObj_Remove failed" );
				psDroid->psCurAnim = NULL;
			}

			/* add firing anim */
			if ( psDroid->psCurAnim == NULL )
			{
				psDroid->psCurAnim = animObj_Add( psDroid, ID_ANIM_DROIDFIRE, 0, 0 );
			}
			else
			{
				psDroid->psCurAnim->bVisible = true;
			}

			return;
		}

		/* don't show move animations if inactive */
		if ( psDroid->psCurAnim != NULL )
		{
			psDroid->psCurAnim->bVisible = false;
		}
		return;
	}

	// update the naybors list
	droidGetNaybors(psDroid);

	moveUpdateDroidDirection( psDroid, &speed, direction, PERSON_SPIN_ANGLE,
				PERSON_SPIN_SPEED, PERSON_TURN_SPEED, &iDroidDir, &fSpeed );

	fNormalSpeed = moveCalcNormalSpeed( psDroid, fSpeed, iDroidDir,
										PERSON_ACCEL, PERSON_DECEL );
	/* people don't skid at the moment so set zero perpendicular speed */
	fPerpSpeed = 0;

	moveCombineNormalAndPerpSpeeds( psDroid, fNormalSpeed,
										fPerpSpeed, iDroidDir );

	moveGetDroidPosDiffs( psDroid, &dx, &dy );

	moveCalcDroidSlide(psDroid, &dx,&dy);
	moveCalcBlockingSlide(psDroid, &dx,&dy, direction, &slideDir);

	moveUpdateDroidPos( psDroid, dx, dy );

	//set the droid height here so other routines can use it
	psDroid->pos.z = map_Height(psDroid->pos.x, psDroid->pos.y);//jps 21july96

	psDroid->sMove.fz = psDroid->pos.z;


	/* update anim if moving and not on fire */
	if ( psDroid->droidType == DROID_PERSON && speed != 0 &&
		 psDroid->order != DORDER_RUNBURN )
	{
		/* remove previous anim */
		if ( psDroid->psCurAnim != NULL &&
			 (psDroid->psCurAnim->psAnim->uwID != ID_ANIM_DROIDRUN ||
			  psDroid->psCurAnim->psAnim->uwID != ID_ANIM_DROIDRUN)   )
		{
			bRet = animObj_Remove( &psDroid->psCurAnim, psDroid->psCurAnim->psAnim->uwID );
			ASSERT( bRet == true, "moveUpdatePersonModel: animObj_Remove failed" );
			psDroid->psCurAnim = NULL;
		}

		/* if no anim currently attached, get one */
		if ( psDroid->psCurAnim == NULL )
		{
			// Only add the animation if the droid is on screen, saves memory and time.
			if(clipXY(psDroid->pos.x,psDroid->pos.y)) {
				debug( LOG_NEVER, "Added person run anim\n" );
				psDroid->psCurAnim = animObj_Add( (BASE_OBJECT *) psDroid,
													ID_ANIM_DROIDRUN, 0, 0 );
			}
		} else {
			// If the droid went off screen then remove the animation, saves memory and time.
			if(!clipXY(psDroid->pos.x,psDroid->pos.y)) {
				bRet = animObj_Remove( &psDroid->psCurAnim, psDroid->psCurAnim->psAnim->uwID );
				ASSERT( bRet == true, "moveUpdatePersonModel : animObj_Remove failed" );
				psDroid->psCurAnim = NULL;
				debug( LOG_NEVER, "Removed person run anim\n" );
			}
		}
	}

	/* show anim */
	if ( psDroid->psCurAnim != NULL )
	{
		psDroid->psCurAnim->bVisible = true;
	}
	CHECK_DROID(psDroid);
}

#define	VTOL_VERTICAL_SPEED		((((SDWORD)psDroid->baseSpeed / 4) > 60) ? ((SDWORD)psDroid->baseSpeed / 4) : 60)

/* primitive 'bang-bang' vtol height controller */
static void moveAdjustVtolHeight( DROID * psDroid, UDWORD iMapHeight )
{
	UDWORD	iMinHeight, iMaxHeight, iLevelHeight;

	if ( psDroid->droidType == DROID_TRANSPORTER )
	{
		iMinHeight   = 2*VTOL_HEIGHT_MIN;
		iLevelHeight = 2*VTOL_HEIGHT_LEVEL;
		iMaxHeight   = 2*VTOL_HEIGHT_MAX;
	}
	else
	{
		iMinHeight   = VTOL_HEIGHT_MIN;
		iLevelHeight = VTOL_HEIGHT_LEVEL;
		iMaxHeight   = VTOL_HEIGHT_MAX;
	}

	if ( psDroid->pos.z >= (iMapHeight+iMaxHeight) )
	{
		psDroid->sMove.iVertSpeed = (SWORD)-VTOL_VERTICAL_SPEED;
	}
	else if ( psDroid->pos.z < (iMapHeight+iMinHeight) )
	{
		psDroid->sMove.iVertSpeed = (SWORD)VTOL_VERTICAL_SPEED;
	}
	else if ( (psDroid->pos.z < iLevelHeight) &&
			  (psDroid->sMove.iVertSpeed < 0 )    )
	{
		psDroid->sMove.iVertSpeed = 0;
	}
	else if ( (psDroid->pos.z > iLevelHeight) &&
			  (psDroid->sMove.iVertSpeed > 0 )    )
	{
		psDroid->sMove.iVertSpeed = 0;
	}
}

// set a vtol to be hovering in the air
void moveMakeVtolHover( DROID *psDroid )
{
	psDroid->sMove.Status = MOVEHOVER;
	psDroid->pos.z = (UWORD)(map_Height(psDroid->pos.x,psDroid->pos.y) + VTOL_HEIGHT_LEVEL);
}

static void moveUpdateVtolModel(DROID *psDroid, SDWORD speed, SDWORD direction)
{
	float	fPerpSpeed, fNormalSpeed, dx, dy, fSpeed;
	float	iDroidDir;
	SDWORD	iMapZ, iRoll, slideDir, iSpinSpeed, iTurnSpeed;
	float	fDZ, fDroidZ, fMapZ;

	CHECK_DROID(psDroid);

	// nothing to do if the droid is stopped
	if ( moveDroidStopped(  psDroid, speed ) == true )
	{
		return;
	}

	// update the naybors list
	droidGetNaybors(psDroid);

	moveCheckFinalWaypoint( psDroid, &speed );

	if ( psDroid->droidType == DROID_TRANSPORTER )
	{
		moveUpdateDroidDirection( psDroid, &speed, direction, VTOL_SPIN_ANGLE,
					VTOL_SPIN_SPEED, VTOL_TURN_SPEED, &iDroidDir, &fSpeed );
	}
	else
	{
		iSpinSpeed = (psDroid->baseSpeed/2 > VTOL_SPIN_SPEED) ? psDroid->baseSpeed/2 : VTOL_SPIN_SPEED;
		iTurnSpeed = (psDroid->baseSpeed/8 > VTOL_TURN_SPEED) ? psDroid->baseSpeed/8 : VTOL_TURN_SPEED;
		moveUpdateDroidDirection( psDroid, &speed, direction, VTOL_SPIN_ANGLE,
				iSpinSpeed, iTurnSpeed, &iDroidDir, &fSpeed );
	}

	fNormalSpeed = moveCalcNormalSpeed( psDroid, fSpeed, iDroidDir,
										VTOL_ACCEL, VTOL_DECEL );
	fPerpSpeed   = moveCalcPerpSpeed( psDroid, iDroidDir, VTOL_SKID_DECEL );

	moveCombineNormalAndPerpSpeeds( psDroid, fNormalSpeed,
										fPerpSpeed, iDroidDir );

	moveGetDroidPosDiffs( psDroid, &dx, &dy );

	/* set slide blocking tile for map edge */
	if ( psDroid->droidType != DROID_TRANSPORTER )
	{
		moveCalcBlockingSlide( psDroid, &dx, &dy, direction, &slideDir );
	}

	moveUpdateDroidPos( psDroid, dx, dy );

	/* update vtol orientation */
	iRoll = (psDroid->sMove.moveDir - psDroid->direction) / 3;
	if ( iRoll < 0 )
	{
		iRoll += 360;
	}
	psDroid->roll = (UWORD) iRoll;

	iMapZ = map_Height(psDroid->pos.x, psDroid->pos.y);

	/* do vertical movement */

	fDZ = timeAdjustedIncrement(psDroid->sMove.iVertSpeed, true);
	fDroidZ = psDroid->sMove.fz;
	fMapZ = (float) map_Height(psDroid->pos.x, psDroid->pos.y);
	if ( fDroidZ+fDZ < 0 )
	{
		psDroid->sMove.fz = 0;
	}
	else if ( fDroidZ+fDZ < fMapZ )
	{
		psDroid->sMove.fz = fMapZ;
	}
	else
	{
		psDroid->sMove.fz = psDroid->sMove.fz + fDZ;
	}
	psDroid->pos.z = (UWORD)psDroid->sMove.fz;


	moveAdjustVtolHeight( psDroid, iMapZ );
}

#define CYBORG_VERTICAL_SPEED	((SDWORD)psDroid->baseSpeed/2)

static void
moveCyborgLaunchAnimDone( ANIM_OBJECT *psObj )
{
	DROID	*psDroid = (DROID*)psObj->psParent;

	ASSERT( psDroid != NULL,
			"moveCyborgLaunchAnimDone: invalid cyborg pointer" );

	/* raise cyborg a little bit so flying - terrible hack - GJ */
	psDroid->pos.z++;
	psDroid->sMove.iVertSpeed = (SWORD)CYBORG_VERTICAL_SPEED;

	psDroid->psCurAnim = NULL;
}

static void
moveCyborgTouchDownAnimDone( ANIM_OBJECT *psObj )
{
	DROID	*psDroid = (DROID*)psObj->psParent;

	ASSERT( psDroid != NULL,
			"moveCyborgTouchDownAnimDone: invalid cyborg pointer" );

	psDroid->psCurAnim = NULL;
	psDroid->pos.z = map_Height( psDroid->pos.x, psDroid->pos.y );
}


static void moveUpdateJumpCyborgModel(DROID *psDroid, SDWORD speed, SDWORD direction)
{
	float	fPerpSpeed, fNormalSpeed, dx, dy, fSpeed;
	float	iDroidDir;

	// nothing to do if the droid is stopped
	if ( moveDroidStopped(  psDroid, speed ) == true )
	{
		return;
	}

	// update the naybors list
	droidGetNaybors(psDroid);

	moveUpdateDroidDirection( psDroid, &speed, direction, VTOL_SPIN_ANGLE,
				psDroid->baseSpeed, psDroid->baseSpeed/3, &iDroidDir, &fSpeed );

	fNormalSpeed = moveCalcNormalSpeed( psDroid, fSpeed, iDroidDir,
										VTOL_ACCEL, VTOL_DECEL );
	fPerpSpeed   = 0;
	moveCombineNormalAndPerpSpeeds( psDroid, fNormalSpeed,
										fPerpSpeed, iDroidDir );

	moveGetDroidPosDiffs( psDroid, &dx, &dy );
	moveUpdateDroidPos( psDroid, dx, dy );
}

static void
moveUpdateCyborgModel( DROID *psDroid, SDWORD moveSpeed, SDWORD moveDir, UBYTE oldStatus )
{
	PROPULSION_STATS	*psPropStats;
	BASE_OBJECT			*psObj = (BASE_OBJECT *) psDroid;
	UDWORD				iMapZ = map_Height(psDroid->pos.x, psDroid->pos.y);
	SDWORD				iDist, iDx, iDy, iDz, iDroidZ;
	BOOL			bRet;

	CHECK_DROID(psDroid);

	// nothing to do if the droid is stopped
	if ( moveDroidStopped( psDroid, moveSpeed ) == true )
	{
		if ( psDroid->psCurAnim != NULL )
		{
			if ( animObj_Remove( &psDroid->psCurAnim, psDroid->psCurAnim->uwID ) == false )
			{
				debug( LOG_NEVER, "moveUpdateCyborgModel: couldn't remove walk anim\n" );
			}
			psDroid->psCurAnim = NULL;
		}

		return;
	}

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( psPropStats != NULL,
			"moveUpdateCyborgModel: invalid propulsion stats pointer" );

	/* do vertical movement */
	if ( psPropStats->propulsionType == JUMP )
	{
		iDz = timeAdjustedIncrement(psDroid->sMove.iVertSpeed, true);
		iDroidZ = (SDWORD) psDroid->pos.z;

		if ( iDroidZ+iDz < (SDWORD) iMapZ )
		{
			psDroid->sMove.iVertSpeed = 0;
			psDroid->pos.z = (UWORD)iMapZ;
		}
		else
		{
			psDroid->pos.z = (UWORD)(psDroid->pos.z + iDz);
		}

		if ( (psDroid->pos.z >= (iMapZ+CYBORG_MAX_JUMP_HEIGHT)) &&
			 (psDroid->sMove.iVertSpeed > 0)                        )
		{
			psDroid->sMove.iVertSpeed = (SWORD)-CYBORG_VERTICAL_SPEED;
		}


		psDroid->sMove.fz = psDroid->pos.z;

	}

	/* calculate move distance */
	iDx = psDroid->sMove.DestinationX - psDroid->pos.x;
	iDy = psDroid->sMove.DestinationY - psDroid->pos.y;
	iDz = psDroid->pos.z - iMapZ;
	iDist = trigIntSqrt(iDx * iDx + iDy * iDy);

	/* set jumping cyborg walking short distances */
	if ( (psPropStats->propulsionType != JUMP) ||
		 ((psDroid->sMove.iVertSpeed == 0)      &&
		  (iDist < CYBORG_MIN_JUMP_DISTANCE))       )
	{
		if ( psDroid->psCurAnim == NULL )
		{
			// Only add the animation if the droid is on screen, saves memory and time.
			if(clipXY(psDroid->pos.x,psDroid->pos.y))
			{
				if ( psDroid->droidType == DROID_CYBORG_SUPER )
				{
					psDroid->psCurAnim = animObj_Add( psObj, ID_ANIM_SUPERCYBORG_RUN, 0, 0 );
				}
				else if (cyborgDroid(psDroid))
				{
					psDroid->psCurAnim = animObj_Add( psObj, ID_ANIM_CYBORG_RUN, 0, 0 );
				}
			}
		} else {
			// If the droid went off screen then remove the animation, saves memory and time.
			if(!clipXY(psDroid->pos.x,psDroid->pos.y))
			{
				bRet = animObj_Remove( &psDroid->psCurAnim, psDroid->psCurAnim->psAnim->uwID );
				ASSERT( bRet == true, "moveUpdateCyborgModel : animObj_Remove failed" );
				psDroid->psCurAnim = NULL;
			}
		}

		/* use baba person movement */
		moveUpdatePersonModel(psDroid,moveSpeed,moveDir);
	}
	else
	{
		/* jumping cyborg: remove walking animation if present */
		if ( psDroid->psCurAnim != NULL )
		{
			if ( (psDroid->psCurAnim->uwID == ID_ANIM_CYBORG_RUN ||
				  psDroid->psCurAnim->uwID == ID_ANIM_SUPERCYBORG_RUN ||
				  psDroid->psCurAnim->uwID == ID_ANIM_CYBORG_PACK_RUN) &&
				 (animObj_Remove( &psDroid->psCurAnim, psDroid->psCurAnim->uwID ) == false) )
			{
				debug( LOG_NEVER, "moveUpdateCyborgModel: couldn't remove walk anim\n" );
			}
		}

		/* add jumping or landing anim */
		if ( (oldStatus == MOVEPOINTTOPOINT) &&
				  (psDroid->sMove.Status == MOVEINACTIVE) )
		{
			psDroid->psCurAnim = animObj_Add( psObj, ID_ANIM_CYBORG_PACK_LAND, 0, 1 );
			animObj_SetDoneFunc( psDroid->psCurAnim, moveCyborgTouchDownAnimDone );
		}
		else if ( psDroid->sMove.Status == MOVEPOINTTOPOINT )
		{
			if ( psDroid->pos.z == iMapZ )
			{
				if ( psDroid->sMove.iVertSpeed == 0 )
				{
					psDroid->psCurAnim = animObj_Add( psObj, ID_ANIM_CYBORG_PACK_JUMP, 0, 1 );
					animObj_SetDoneFunc( psDroid->psCurAnim, moveCyborgLaunchAnimDone );
				}
				else
				{
					psDroid->psCurAnim = animObj_Add( psObj, ID_ANIM_CYBORG_PACK_LAND, 0, 1 );
					animObj_SetDoneFunc( psDroid->psCurAnim, moveCyborgTouchDownAnimDone );
				}
			}
			else
			{
				moveUpdateJumpCyborgModel( psDroid, moveSpeed, moveDir );
			}
		}
	}

	psDroid->pitch = 0;
	psDroid->roll  = 0;
}

static BOOL moveDescending( DROID *psDroid, UDWORD iMapHeight )
{
	if ( psDroid->pos.z > iMapHeight )
	{
		/* descending */
		psDroid->sMove.iVertSpeed = (SWORD)-VTOL_VERTICAL_SPEED;
		psDroid->sMove.speed = 0;

		/* return true to show still descending */
		return true;
	}
	else
	{
		/* on floor - stop */
		psDroid->sMove.iVertSpeed = 0;

		/* conform to terrain */
		updateDroidOrientation(psDroid);

		/* return false to show stopped descending */
		return false;
	}
}


BOOL moveCheckDroidMovingAndVisible( void *psObj )
{
	DROID	*psDroid = (DROID*)psObj;

	if ( psDroid == NULL )
	{
		return false;
	}

	/* check for dead, not moving or invisible to player */
	if ( psDroid->died || moveDroidStopped( psDroid, 0 ) ||
		 (psDroid->droidType == DROID_TRANSPORTER && psDroid->order == DORDER_NONE) ||
		 !(psDroid->visible[selectedPlayer] || godMode)                                )
	{
		psDroid->iAudioID = NO_SOUND;
		return false;
	}

	return true;
}


static void movePlayDroidMoveAudio( DROID *psDroid )
{
	SDWORD				iAudioID = NO_SOUND;
	PROPULSION_TYPES	*psPropType;
	UBYTE				iPropType = 0;

	ASSERT( psDroid != NULL,
		"movePlayUnitMoveAudio: unit pointer invalid\n" );

	if ( (psDroid != NULL) &&
		 (psDroid->visible[selectedPlayer] || godMode) )
	{
		iPropType = asPropulsionStats[(psDroid)->asBits[COMP_PROPULSION].nStat].propulsionType;
		psPropType = &asPropulsionTypes[iPropType];

		/* play specific wheeled and transporter or stats-specified noises */
		if ( iPropType == WHEELED && psDroid->droidType != DROID_CONSTRUCT )
		{
			iAudioID = ID_SOUND_TREAD;
		}
		else if (psDroid->droidType == DROID_TRANSPORTER)
		{
			iAudioID = ID_SOUND_BLIMP_FLIGHT;
		}
		else if (iPropType == LEGGED && cyborgDroid(psDroid))
		{
			iAudioID = ID_SOUND_CYBORG_MOVE;
		}
		else
		{
			iAudioID = psPropType->moveID;
		}

		if ( iAudioID != NO_SOUND )
		{
			if ( audio_PlayObjDynamicTrack( psDroid, iAudioID,
					moveCheckDroidMovingAndVisible ) )
			{
				psDroid->iAudioID = iAudioID;
			}
		}
	}
}


static BOOL moveDroidStartCallback( void *psObj )
{
	DROID *psDroid = (DROID*)psObj;

	if ( psDroid == NULL )
	{
		return false;
	}

	movePlayDroidMoveAudio( psDroid );

	return true;
}


static void movePlayAudio( DROID *psDroid, BOOL bStarted, BOOL bStoppedBefore, SDWORD iMoveSpeed )
{
	UBYTE				propType;
	PROPULSION_STATS	*psPropStats;
	PROPULSION_TYPES	*psPropType;
	BOOL				bStoppedNow;
	SDWORD				iAudioID = NO_SOUND;
	AUDIO_CALLBACK		pAudioCallback = NULL;

	/* get prop stats */
	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( psPropStats != NULL,
			"moveUpdateUnit: invalid propulsion stats pointer" );
	propType = psPropStats->propulsionType;
	psPropType = &asPropulsionTypes[propType];

	/* get current droid motion status */
	bStoppedNow = moveDroidStopped( psDroid, iMoveSpeed );

	if ( bStarted )
	{
		/* play start audio */
		if ((propType == WHEELED && psDroid->droidType != DROID_CONSTRUCT)
		    || psPropType->startID == NO_SOUND)
		{
			movePlayDroidMoveAudio( psDroid );
			return;
		}
		else if ( psDroid->droidType == DROID_TRANSPORTER )
		{
			iAudioID = ID_SOUND_BLIMP_TAKE_OFF;
		}
		else
		{
			iAudioID = psPropType->startID;
		}

		pAudioCallback = moveDroidStartCallback;
	}
	else if ( !bStoppedBefore && bStoppedNow &&
				(psPropType->shutDownID != NO_SOUND) )
	{
		/* play stop audio */
		if ( psDroid->droidType == DROID_TRANSPORTER )
		{
			iAudioID = ID_SOUND_BLIMP_LAND;
		}
		else if ( propType != WHEELED || psDroid->droidType == DROID_CONSTRUCT )
		{
			iAudioID = psPropType->shutDownID;
		}
	}
	else if (!bStoppedBefore && !bStoppedNow && psDroid->iAudioID == NO_SOUND)
	{
		/* play move audio */
		movePlayDroidMoveAudio( psDroid );
		return;
	}

	if ( (iAudioID != NO_SOUND) &&
		 (psDroid->visible[selectedPlayer] || godMode) )
	{
		if ( audio_PlayObjDynamicTrack( psDroid, iAudioID,
				pAudioCallback ) )
		{
			psDroid->iAudioID = iAudioID;
		}
	}
}


// called when a droid moves to a new tile.
// use to pick up oil, etc..
static void checkLocalFeatures(DROID *psDroid)
{
	SDWORD			i;
	BASE_OBJECT		*psObj;

	// only do for players droids.
	if(psDroid->player != selectedPlayer)
	{
		return;
	}

	droidGetNaybors(psDroid);// update naybor list.

	// scan the neighbours
	for(i=0; i<(SDWORD)numNaybors; i++)
	{
#define DROIDDIST (((TILE_UNITS*3)/2) * ((TILE_UNITS*3)/2))
		psObj = asDroidNaybors[i].psObj;
		if (   psObj->type != OBJ_FEATURE
			|| ((FEATURE *)psObj)->psStats->subType != FEAT_OIL_DRUM
			|| asDroidNaybors[i].distSqr >= DROIDDIST )
		{
			// object too far away to worry about
			continue;
		}


		if(bMultiPlayer && (psObj->player == ANYPLAYER))
		{
			giftPower(ANYPLAYER,selectedPlayer,true);			// give power and tell everyone.
			addOilDrum(1);
		}
		else

		{
			addPower(selectedPlayer,OILDRUM_POWER);
		}
		removeFeature((FEATURE*)psObj);							// remove artifact+ send multiplay info.

	}
}


/* Frame update for the movement of a tracked droid */
void moveUpdateDroid(DROID *psDroid)
{
	float				tangle;		// thats DROID angle and TARGET angle - not some bizzare pun :-)
									// doesn't matter - they're still shit names...! :-)
	SDWORD				fx, fy;
	UDWORD				oldx, oldy, iZ;
	UBYTE				oldStatus = psDroid->sMove.Status;
	SDWORD				moveSpeed;
	float				moveDir;
	PROPULSION_STATS	*psPropStats;
	Vector3i pos;
	BOOL				bStarted = false, bStopped;
	Vector2f			target;

	CHECK_DROID(psDroid);

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( psPropStats != NULL,
			"moveUpdateUnit: invalid propulsion stats pointer" );

//	if(driveModeActive()) {
//		driveUpdateDroid(psDroid);
//	}

	// If the droid has been attacked by an EMP weapon, it is temporarily disabled
	if (psDroid->lastHitWeapon == WSC_EMP)
	{
		if (gameTime - psDroid->timeLastHit < EMP_DISABLE_TIME)
		{
			// Get out without updating
			return;
		}
	}

	/* save current motion status of droid */
	bStopped = moveDroidStopped( psDroid, 0 );

	moveSpeed = 0;
	moveDir = psDroid->direction;

	/* get droid height */
	iZ = map_Height(psDroid->pos.x, psDroid->pos.y);

	switch (psDroid->sMove.Status)
	{
	case MOVEINACTIVE:
		if ( (psDroid->droidType == DROID_PERSON) &&
			 (psDroid->psCurAnim != NULL) &&
			 (psDroid->psCurAnim->psAnim->uwID == ID_ANIM_DROIDRUN) )
		{
			psDroid->psCurAnim->bVisible = false;
		}
		break;
	case MOVEROUTE:
	case MOVEROUTESHUFFLE:
	case MOVESHUFFLE:
		// deal with both waiting for a route (MOVEROUTE) and the droid shuffle (MOVESHUFFLE)
		// here because droids waiting for a route need to shuffle out of the way (MOVEROUTESHUFFLE)
		// of those that have already got a route

		if ((psDroid->sMove.Status == MOVEROUTE) ||
			(psDroid->sMove.Status == MOVEROUTESHUFFLE))
		{
			// see if this droid started waiting for a route before the previous one
			// and note it to be the next droid to route.
			// selectedPlayer always gets precidence in single player
			if (psNextRouteDroid == NULL)
			{
				objTrace(psDroid->id, "Waiting droid set to %d (player %d) started at %d now %d (none waiting)",
				         (int)psDroid->id, (int)psDroid->player, (int)psDroid->sMove.bumpTime, (int)gameTime);
				psNextRouteDroid = psDroid;
			}

			else if (bMultiPlayer &&
					 (psNextRouteDroid->sMove.bumpTime > psDroid->sMove.bumpTime))
			{
				objTrace(psDroid->id, "Waiting droid set to %d (player %d) started at %d now %d (mulitplayer)",
				         (int)psDroid->id, (int)psDroid->player, (int)psDroid->sMove.bumpTime, (int)gameTime);
				psNextRouteDroid = psDroid;
			}

			else if ( (psDroid->player == selectedPlayer) &&
					  ( (psNextRouteDroid->player != selectedPlayer) ||
						(psNextRouteDroid->sMove.bumpTime > psDroid->sMove.bumpTime) ) )
			{
				objTrace(psDroid->id, "Waiting droid set to %d (player %d) started at %d now %d (selectedPlayer)",
				         (int)psDroid->id, (int)psDroid->player, (int)psDroid->sMove.bumpTime, (int)gameTime);
				psNextRouteDroid = psDroid;
			}
			else if ( (psDroid->player != selectedPlayer) &&
					  (psNextRouteDroid->player != selectedPlayer) &&
					  (psNextRouteDroid->sMove.bumpTime > psDroid->sMove.bumpTime) )
			{
				objTrace(psDroid->id, "Waiting droid set to %d (player %d) started at %d now %d (non selectedPlayer)",
				         (int)psDroid->id, (int)psDroid->player, (int)psDroid->sMove.bumpTime, (int)gameTime);
				psNextRouteDroid = psDroid;
			}
		}

		if ((psDroid->sMove.Status == MOVEROUTE) ||
			(psDroid->sMove.Status == MOVEROUTESHUFFLE))
		{
			psDroid->sMove.fx = psDroid->pos.x;
			psDroid->sMove.fy = psDroid->pos.y;
			psDroid->sMove.fz = psDroid->pos.z;

			turnOffMultiMsg(true);
			moveDroidTo(psDroid, psDroid->sMove.DestinationX,psDroid->sMove.DestinationY);
			turnOffMultiMsg(false);
		}
		else if ((psDroid->sMove.Status == MOVESHUFFLE) ||
				 (psDroid->sMove.Status == MOVEROUTESHUFFLE))
		{
			if (moveReachedWayPoint(psDroid) ||
				((psDroid->sMove.shuffleStart + MOVE_SHUFFLETIME) < gameTime))
			{
				if ( psDroid->sMove.Status == MOVEROUTESHUFFLE )
				{
					psDroid->sMove.Status = MOVEROUTE;
				}
				else if ( psPropStats->propulsionType == LIFT )
				{
					psDroid->sMove.Status = MOVEHOVER;
				}
				else
				{
					psDroid->sMove.Status = MOVEINACTIVE;
				}
			}
			else
			{
				// Calculate a target vector
				Vector2f target = moveGetDirection(psDroid);

				// Turn the droid if necessary
				tangle = vectorToAngle(target.x, target.y);

				if ( psDroid->droidType == DROID_TRANSPORTER )
				{
					debug( LOG_NEVER, "a) dir %g,%g (%g)\n", target.x, target.y, tangle );
				}

				moveSpeed = moveCalcDroidSpeed(psDroid);
				moveDir = tangle;
				ASSERT(moveDir >= 0 && moveDir <= 360, "Illegal movement direction");
			}
		}

		break;
	case MOVEWAITROUTE:
		moveDroidTo(psDroid, psDroid->sMove.DestinationX,psDroid->sMove.DestinationY);
		break;
	case MOVENAVIGATE:
		// Get the next control point
		if (!moveNextTarget(psDroid))
		{
			// No more waypoints - finish
			if ( psPropStats->propulsionType == LIFT )
			{
				psDroid->sMove.Status = MOVEHOVER;
			}
			else
			{
				psDroid->sMove.Status = MOVEINACTIVE;
			}
			break;
		}

		// Calculate the direction vector
//		psDroid->direction = calcDirection(psDroid->pos.x,psDroid->pos.y, tarX,tarY);
		psDroid->sMove.fx = psDroid->pos.x;
		psDroid->sMove.fy = psDroid->pos.y;
		psDroid->sMove.fz = psDroid->pos.z;

		moveCalcBoundary(psDroid);

		if (vtolDroid(psDroid))
		{
			psDroid->pitch = 0;
		}

		psDroid->sMove.Status = MOVEPOINTTOPOINT;
		psDroid->sMove.bumpTime = 0;

		/* save started status for movePlayAudio */
		if (psDroid->sMove.speed == 0)
		{
			bStarted = true;
		}

		break;
	case MOVEPOINTTOPOINT:
	case MOVEPAUSE:
		// moving between two way points

		// See if the target point has been reached
		if (moveReachedWayPoint(psDroid))
		{
			// Got there - move onto the next waypoint
			if (!moveNextTarget(psDroid))
			{
				// No more waypoints - finish
//				psDroid->sMove.Status = MOVEINACTIVE;
				if ( psPropStats->propulsionType == LIFT )
				{
					psDroid->sMove.Status = MOVEHOVER;
				}
				else
				{
					psDroid->sMove.Status = MOVETURN;
				}
				break;
			}
			moveCalcBoundary(psDroid);
		}

		if (psDroid->sMove.psFormation && psDroid->sMove.Position == psDroid->sMove.numPoints)
		{
			if (formationGetPos(psDroid->sMove.psFormation, psDroid, &fx,&fy,true))
			{
				psDroid->sMove.targetX = fx;
				psDroid->sMove.targetY = fy;
				moveCalcBoundary(psDroid);
			}
		}

		// Calculate a target vector
		target = moveGetDirection(psDroid);

		// Turn the droid if necessary
		tangle = vectorToAngle(target.x, target.y);

		moveSpeed = moveCalcDroidSpeed(psDroid);

		moveDir = tangle;
		ASSERT(moveDir >= 0 && moveDir <= 360, "Illegal movement direction");

		if ((psDroid->sMove.bumpTime != 0) &&
			(psDroid->sMove.pauseTime + psDroid->sMove.bumpTime + BLOCK_PAUSETIME < gameTime))
		{
			if (psDroid->sMove.Status == MOVEPOINTTOPOINT)
			{
				psDroid->sMove.Status = MOVEPAUSE;
			}
			else
			{
				psDroid->sMove.Status = MOVEPOINTTOPOINT;
			}
			psDroid->sMove.pauseTime = (UWORD)(gameTime - psDroid->sMove.bumpTime);
		}

		if ((psDroid->sMove.Status == MOVEPAUSE) &&
			(psDroid->sMove.bumpTime != 0) &&
			(psDroid->sMove.lastBump > psDroid->sMove.pauseTime) &&
			(psDroid->sMove.lastBump + psDroid->sMove.bumpTime + BLOCK_PAUSERELEASE < gameTime))
		{
			psDroid->sMove.Status = MOVEPOINTTOPOINT;
		}

		break;
	case MOVETURN:
		// Turn the droid to it's final facing
		if (psDroid->sMove.psFormation &&
			psDroid->sMove.psFormation->refCount > 1 &&
			(SDWORD)psDroid->direction != (SDWORD)psDroid->sMove.psFormation->dir)
		{
			moveSpeed = 0;
			moveDir = psDroid->sMove.psFormation->dir;
			ASSERT(moveDir >= 0 && moveDir <= 360, "Illegal movement direction");
		}
		else
		{
			if ( psPropStats->propulsionType == LIFT )
			{
				psDroid->sMove.Status = MOVEPOINTTOPOINT;
			}
			else
			{
				psDroid->sMove.Status = MOVEINACTIVE;
			}
		}
		break;
	case MOVETURNTOTARGET:
		moveSpeed = 0;
		moveDir = calcDirection( psDroid->pos.x, psDroid->pos.y, psDroid->sMove.targetX, psDroid->sMove.targetY );
		if ((int)psDroid->direction == (int)moveDir)
		{
			if ( psPropStats->propulsionType == LIFT )
			{
				psDroid->sMove.Status = MOVEPOINTTOPOINT;
			}
			else
			{
				psDroid->sMove.Status = MOVEINACTIVE;
			}
		}
		ASSERT(moveDir >= 0 && moveDir <= 360, "Illegal movement direction");
		break;
	case MOVEHOVER:
		/* change vtols to attack run mode if target found - but not if no ammo*/
		/*if ( psDroid->droidType != DROID_CYBORG && psDroid->psTarget != NULL &&
			!vtolEmpty(psDroid))
		{
			psDroid->sMove.Status = MOVEPOINTTOPOINT;
			break;
		}*/

/*		moveGetDirection(psDroid, &tx,&ty);
		tangle = vectorToAngle(tx,ty);
		moveSpeed = moveCalcDroidSpeed(psDroid);
		moveDir = tangle;*/

		/* descend if no orders or actions or cyborg at target */
/*		if ( (psDroid->droidType == DROID_CYBORG) ||
			 ((psDroid->action == DACTION_NONE) && (psDroid->order == DORDER_NONE)) ||
			 ((psDroid->action == DACTION_NONE) && (psDroid->order == DORDER_TRANSPORTIN)) ||
			 ((psDroid->action == DACTION_NONE) && (psDroid->order == DORDER_GUARD)) ||
			 (psDroid->action == DACTION_MOVE) || (psDroid->action == DACTION_WAITFORREARM) ||
			 (psDroid->action == DACTION_WAITDURINGREARM)  || (psDroid->action == DACTION_FIRESUPPORT))*/
		{
			if ( moveDescending( psDroid, iZ ) == false )
			{
				/* reset move state */
				psDroid->sMove.Status = MOVEINACTIVE;
			}
/*			else
			{
				UDWORD	landX, landY;

				// see if the landing position is clear
				landX = psDroid->pos.x;
				landY = psDroid->pos.y;
				actionVTOLLandingPos(psDroid, &landX,&landY);
				if (map_coord(landX) != map_coord(psDroid->pos.x)
				 && map_coord(landY) != map_coord(psDroid->pos.y))
				{
					moveDroidToDirect(psDroid, landX,landY);
				}
			}*/
		}
/*		else
		{
			moveAdjustVtolHeight( psDroid, iZ );
		}*/

		break;

		// Driven around by the player.
	case MOVEDRIVE:
		driveSetDroidMove(psDroid);
		moveSpeed = driveGetMoveSpeed();	//psDroid->sMove.speed;
		moveDir = driveGetMoveDir();		//psDroid->sMove.dir;
		ASSERT(moveDir >= 0 && moveDir <= 360, "Illegal movement direction");
		break;

	default:
		ASSERT( false, "moveUpdateUnit: unknown move state" );
		break;
	}

	ASSERT(moveDir >= 0 && moveDir <= 360, "Illegal movement direction");

	// Update the movement model for the droid
	oldx = psDroid->pos.x;
	oldy = psDroid->pos.y;

	if ( psDroid->droidType == DROID_PERSON )
	{
		moveUpdatePersonModel(psDroid, moveSpeed, moveDir);
	}
	else if (cyborgDroid(psDroid))
	{
		moveUpdateCyborgModel(psDroid, moveSpeed, moveDir, oldStatus);
	}
	else if ( psPropStats->propulsionType == LIFT )
	{
		moveUpdateVtolModel(psDroid, moveSpeed, moveDir);
	}
	else
	{
		moveUpdateGroundModel(psDroid, moveSpeed, moveDir);
	}

	if (map_coord(oldx) != map_coord(psDroid->pos.x)
	 || map_coord(oldy) != map_coord(psDroid->pos.y))
	{
		visTilesUpdate((BASE_OBJECT *)psDroid, rayTerrainCallback);
		gridMoveDroid(psDroid, (SDWORD)oldx,(SDWORD)oldy);

		// object moved from one tile to next, check to see if droid is near stuff.(oil)
		checkLocalFeatures(psDroid);
	}

	// See if it's got blocked
	if ( (psPropStats->propulsionType != LIFT) && moveBlocked(psDroid) )
	{
		objTrace(psDroid->id, "status: id %d blocked", (int)psDroid->id);
		psDroid->sMove.Status = MOVETURN;
	}

//	// If were in drive mode and the droid is a follower then stop it when it gets within
//	// range of the driver.
//	if(driveIsFollower(psDroid)) {
//		if(DoFollowRangeCheck) {
//			if(driveInDriverRange(psDroid)) {
//				psDroid->sMove.Status = MOVEINACTIVE;
////				ClearFollowRangeCheck = true;
//			} else {
//				AllInRange = false;
//			}
//		}
//	}

	/* If it's sitting in water then it's got to go with the flow! */
	if (terrainType(mapTile(psDroid->pos.x/TILE_UNITS,psDroid->pos.y/TILE_UNITS)) == TER_WATER)
	{
		updateDroidOrientation(psDroid);
	}

	if( (psDroid->inFire && psDroid->type != DROID_PERSON) && psDroid->visible[selectedPlayer])
	{
		pos.x = psDroid->pos.x + (18-rand()%36);
		pos.z = psDroid->pos.y + (18-rand()%36);
		pos.y = psDroid->pos.z + (psDroid->sDisplay.imd->max.y / 3);
		addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,false,NULL,0);
	}

	movePlayAudio( psDroid, bStarted, bStopped, moveSpeed );
	CHECK_DROID(psDroid);
}
