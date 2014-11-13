/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
#include "lib/framework/math_ext.h"
#include "lib/gamelib/gtime.h"
#include "lib/gamelib/animobj.h"
#include "lib/netplay/netplay.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "console.h"

#include "move.h"

#include "objects.h"
#include "visibility.h"
#include "map.h"
#include "fpath.h"
#include "loop.h"
#include "geometry.h"
#include "anim_id.h"
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
#include "random.h"
#include "mission.h"
#include "drive.h"

/* max and min vtol heights above terrain */
#define	VTOL_HEIGHT_MIN				250
#define	VTOL_HEIGHT_LEVEL			300
#define	VTOL_HEIGHT_MAX				350

/* minimum distance for cyborgs to jump */
#define	CYBORG_MIN_JUMP_DISTANCE	500
#define	CYBORG_MAX_JUMP_HEIGHT		200

// Maximum size of an object for collision
#define OBJ_MAXRADIUS	(TILE_UNITS * 4)

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

// How far out from an obstruction to start avoiding it
#define AVOID_DIST		(TILE_UNITS*2)

// Speed to approach a final way point, if possible.
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
#define TRACKED_SPIN_ANGLE              DEG(45)
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
#define PERSON_SPIN_ANGLE               DEG(45)
// The speed at which people spin
#define PERSON_SPIN_SPEED		DEG(500)
// The speed at which people turn while going forward
#define PERSON_TURN_SPEED		DEG(250)
// How fast a person accelerates
#define PERSON_ACCEL			250
// How fast a person decelerates
#define PERSON_DECEL			450


/************************************************************************************/
/*             VTOL model defines                                                 */

// The magnitude of direction change required for a vtol to spin on the spot
#define VTOL_SPIN_ANGLE                 DEG(180)
// The speed at which vtols spin (zero means can't spin)
#define VTOL_SPIN_SPEED                 DEG(200)
// The minimum speed at which vtols turn while going forward
#define VTOL_TURN_SPEED			DEG(100)
// How fast vtols accelerate
#define VTOL_ACCEL				200
// How fast vtols decelerate
#define VTOL_DECEL				200
// How fast vtols 'skid'
#define VTOL_SKID_DECEL			600

/// Extra precision added to movement calculations.
#define EXTRA_BITS                              8
#define EXTRA_PRECISION                         (1 << EXTRA_BITS)


static uint32_t oilTimer = 0;
static unsigned drumCount = 0;

/* Function prototypes */
static void	moveUpdatePersonModel(DROID *psDroid, SDWORD speed, uint16_t direction);

const char *moveDescription(MOVE_STATUS status)
{
	switch (status)
	{
	case MOVEINACTIVE : return "Inactive";
	case MOVENAVIGATE : return "Navigate";
	case MOVETURN : return "Turn";
	case MOVEPAUSE : return "Pause";
	case MOVEPOINTTOPOINT : return "P2P";
	case MOVETURNTOTARGET : return "Turn2target";
	case MOVEHOVER : return "Hover";
	case MOVEDRIVE : return "Drive";
	case MOVEWAITROUTE : return "Waitroute";
	case MOVESHUFFLE : return "Shuffle";
	}
	return "Error";	// satisfy compiler
}

/** Initialise the movement system
 */
bool moveInitialise(void)
{
	oilTimer = 0;
	drumCount = 0;

	return true;
}

/** Set a target location in world coordinates for a droid to move to
 *  @return true if the routing was successful, if false then the calling code
 *          should not try to route here again for a while
 *  @todo Document what "should not try to route here again for a while" means.
 */
static bool moveDroidToBase(DROID *psDroid, UDWORD x, UDWORD y, bool bFormation, FPATH_MOVETYPE moveType)
{
	FPATH_RETVAL		retVal = FPR_OK;

	CHECK_DROID(psDroid);

	// in multiPlayer make Transporter move like the vtols
	if (isTransporter(psDroid) && game.maxPlayers == 0)
	{
		fpathSetDirectRoute(psDroid, x, y);
		psDroid->sMove.Status = MOVENAVIGATE;
		psDroid->sMove.pathIndex = 0;
		return true;
	}
	// NOTE: While Vtols can fly, then can't go through things, like the transporter.
	else if ((game.maxPlayers > 0 && isTransporter(psDroid)))
	{
		fpathSetDirectRoute(psDroid, x, y);
		retVal = FPR_OK;
	}
	else
	{
		retVal = fpathDroidRoute(psDroid, x, y, moveType);
	}

	if ( retVal == FPR_OK )
	{
		// bit of a hack this - john
		// if astar doesn't have a complete route, it returns a route to the nearest clear tile.
		// the location of the clear tile is in DestinationX,DestinationY.
		// reset x,y to this position so the formation gets set up correctly
		x = psDroid->sMove.destination.x;
		y = psDroid->sMove.destination.y;

		objTrace(psDroid->id, "unit %d: path ok - base Speed %u, speed %d, target(%u|%d, %u|%d)",
		         (int)psDroid->id, psDroid->baseSpeed, psDroid->sMove.speed, x, map_coord(x), y, map_coord(y));

		psDroid->sMove.Status = MOVENAVIGATE;
		psDroid->sMove.pathIndex = 0;
	}
	else if (retVal == FPR_WAIT)
	{
		// the route will be calculated by the path-finding thread
		psDroid->sMove.Status = MOVEWAITROUTE;
		psDroid->sMove.destination.x = x;
		psDroid->sMove.destination.y = y;
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
bool moveDroidTo(DROID* psDroid, UDWORD x, UDWORD y, FPATH_MOVETYPE moveType)
{
	return moveDroidToBase(psDroid, x, y, true, moveType);
}

/** Move a droid to a location, not joining a formation
 *  @see moveDroidToBase() for the parameter and return value specification
 */
bool moveDroidToNoFormation(DROID* psDroid, UDWORD x, UDWORD y, FPATH_MOVETYPE moveType)
{
	ASSERT(x > 0 && y > 0, "Bad movement position");
	return moveDroidToBase(psDroid, x, y, false, moveType);
}


/** Move a droid directly to a location.
 *  @note This is (or should be) used for VTOLs only.
 */
void moveDroidToDirect(DROID* psDroid, UDWORD x, UDWORD y)
{
	ASSERT( psDroid != NULL && isVtolDroid(psDroid),
		"moveUnitToDirect: only valid for a vtol unit" );

	fpathSetDirectRoute(psDroid, x, y);
	psDroid->sMove.Status = MOVENAVIGATE;
	psDroid->sMove.pathIndex = 0;
}


/** Turn a droid towards a given location.
 */
void moveTurnDroid(DROID *psDroid, UDWORD x, UDWORD y)
{
	uint16_t moveDir = calcDirection(psDroid->pos.x, psDroid->pos.y, x, y);

	if (psDroid->rot.direction != moveDir)
	{
		psDroid->sMove.target.x = x;
		psDroid->sMove.target.y = y;
		psDroid->sMove.Status = MOVETURNTOTARGET;
	}
}

// Tell a droid to move out the way for a shuffle
static void moveShuffleDroid(DROID *psDroid, Vector2i s)
{
	SDWORD  mx, my;
	bool	frontClear = true, leftClear = true, rightClear = true;
	SDWORD	lvx,lvy, rvx,rvy, svx,svy;
	SDWORD	shuffleMove;

	CHECK_DROID(psDroid);

	uint16_t shuffleDir = iAtan2(s);
	int32_t shuffleMag = iHypot(s);

	if (shuffleMag == 0)
	{
		return;
	}

	shuffleMove = SHUFFLE_MOVE;

	// calculate the possible movement vectors
	svx = s.x * shuffleMove / shuffleMag;  // Straight in the direction of s.
	svy = s.y * shuffleMove / shuffleMag;

	lvx = -svy;  // 90° to the... right?
	lvy = svx;

	rvx = svy;   // 90° to the... left?
	rvy = -svx;

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
	static GridList gridList;  // static to avoid allocations.
	gridList = gridStartIterate(psDroid->pos.x, psDroid->pos.y, SHUFFLE_DIST);
	for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
	{
		DROID *psCurr = castDroid(*gi);
		if (psCurr == NULL || psCurr->died || psCurr == psDroid)
		{
			continue;
		}

		uint16_t droidDir = iAtan2(removeZ(psCurr->pos - psDroid->pos));
		int diff = angleDelta(shuffleDir - droidDir);
		if (diff > -DEG(135) && diff < -DEG(45))
		{
			leftClear = false;
		}
		else if (diff > DEG(45) && diff < DEG(135))
		{
			rightClear = false;
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
	Vector2i tar = removeZ(psDroid->pos) + Vector2i(mx, my);
	if (isVtolDroid(psDroid))
	{
		actionVTOLLandingPos(psDroid, &tar);
	}


	// set up the move state
	if (psDroid->sMove.Status != MOVESHUFFLE)
	{
		psDroid->sMove.shuffleStart = gameTime;
	}
	psDroid->sMove.Status = MOVESHUFFLE;
	psDroid->sMove.src = removeZ(psDroid->pos);
	psDroid->sMove.target = tar;
	psDroid->sMove.numPoints = 0;
	psDroid->sMove.pathIndex = 0;

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

	if ( psPropStats->propulsionType == PROPULSION_TYPE_LIFT )
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
	int32_t hx0, hx1, hy0, hy1;
	int newPitch, deltaPitch, pitchLimit;
	int32_t dzdx, dzdy, dzdv, dzdw;
	const int d = 20;
	int32_t vX, vY;

	if (psDroid->droidType == DROID_PERSON || cyborgDroid(psDroid) || isTransporter(psDroid)
		|| isFlying(psDroid))
	{
		/* The ground doesn't affect the pitch/roll of these droids*/
		return;
	}

	// Find the height of 4 points around the droid.
	//    hy0
	// hx0 * hx1      (* = droid)
	//    hy1
	hx1 = map_Height(psDroid->pos.x + d, psDroid->pos.y);
	hx0 = map_Height(MAX(0, psDroid->pos.x - d), psDroid->pos.y);
	hy1 = map_Height(psDroid->pos.x, psDroid->pos.y + d);
	hy0 = map_Height(psDroid->pos.x, MAX(0, psDroid->pos.y - d));

	//update height in case were in the bottom of a trough
	psDroid->pos.z = MAX(psDroid->pos.z, (hx0 + hx1)/2);
	psDroid->pos.z = MAX(psDroid->pos.z, (hy0 + hy1)/2);

	// Vector of length 65536 pointing in direction droid is facing.
	vX = iSin(psDroid->rot.direction);
	vY = iCos(psDroid->rot.direction);

	// Calculate pitch of ground.
	dzdx = hx1 - hx0;                                    // 2*d*∂z(x, y)/∂x       of ground
	dzdy = hy1 - hy0;                                    // 2*d*∂z(x, y)/∂y       of ground
	dzdv = dzdx*vX + dzdy*vY;                            // 2*d*∂z(x, y)/∂v << 16 of ground, where v is the direction the droid is facing.
	newPitch = iAtan2(dzdv, (2*d) << 16);                // pitch = atan(∂z(x, y)/∂v)/2π << 16

	deltaPitch = angleDelta(newPitch - psDroid->rot.pitch);

	// Limit the rate the front comes down to simulate momentum
	pitchLimit = gameTimeAdjustedIncrement(DEG(PITCH_LIMIT));
	deltaPitch = MAX(deltaPitch, -pitchLimit);

	// Update pitch.
	psDroid->rot.pitch += deltaPitch;

	// Calculate and update roll of ground (not taking pitch into account, but good enough).
	dzdw = dzdx * vY - dzdy * vX;				// 2*d*∂z(x, y)/∂w << 16 of ground, where w is at right angles to the direction the droid is facing.
	psDroid->rot.roll = iAtan2(dzdw, (2*d) << 16);		// pitch = atan(∂z(x, y)/∂w)/2π << 16
}


struct BLOCKING_CALLBACK_DATA
{
	PROPULSION_TYPE propulsionType;
	bool blocking;
	Vector2i src;
	Vector2i dst;
};

static bool moveBlockingTileCallback(Vector2i pos, int32_t dist, void *data_)
{
	BLOCKING_CALLBACK_DATA *data = (BLOCKING_CALLBACK_DATA *)data_;
	data->blocking |= pos != data->src && pos != data->dst && fpathBlockingTile(map_coord(pos.x), map_coord(pos.y), data->propulsionType);
	return !data->blocking;
}

// Returns -1 - distance if the direct path to the waypoint is blocked, otherwise returns the distance to the waypoint.
static int32_t moveDirectPathToWaypoint(DROID *psDroid, unsigned positionIndex)
{
	Vector2i src = removeZ(psDroid->pos);
	Vector2i dst = psDroid->sMove.asPath[positionIndex];
	Vector2i delta = dst - src;
	int32_t dist = iHypot(delta);
	BLOCKING_CALLBACK_DATA data;
	data.propulsionType = getPropulsionStats(psDroid)->propulsionType;
	data.blocking = false;
	data.src = src;
	data.dst = dst;
	rayCast(src, dst, &moveBlockingTileCallback, &data);
	return data.blocking? -1 - dist : dist;
}

// Returns true if still able to find the path.
static bool moveBestTarget(DROID *psDroid)
{
	int positionIndex = std::max(psDroid->sMove.pathIndex - 1, 0);
	int32_t dist = moveDirectPathToWaypoint(psDroid, positionIndex);
	if (dist >= 0)
	{
		// Look ahead in the path.
		while (dist >= 0 && dist < TILE_UNITS*5)
		{
			++positionIndex;
			if (positionIndex >= psDroid->sMove.numPoints)
			{
				dist = -1;
				break;  // Reached end of path.
			}
			dist = moveDirectPathToWaypoint(psDroid, positionIndex);
		}
		if (dist < 0)
		{
			--positionIndex;
		}
	}
	else
	{
		// Lost sight of path, backtrack.
		while (dist < 0 && dist >= -TILE_UNITS*7 && positionIndex > 0)
		{
			--positionIndex;
			dist = moveDirectPathToWaypoint(psDroid, positionIndex);
		}
		if (dist < 0)
		{
			return false;  // Couldn't find path, and backtracking didn't help.
		}
	}
	psDroid->sMove.pathIndex = positionIndex + 1;
	psDroid->sMove.src = removeZ(psDroid->pos);
	psDroid->sMove.target = psDroid->sMove.asPath[positionIndex];
	return true;
}

/* Get the next target point from the route */
static bool moveNextTarget(DROID *psDroid)
{
	CHECK_DROID(psDroid);

	// See if there is anything left in the move list
	if (psDroid->sMove.pathIndex == psDroid->sMove.numPoints)
	{
		return false;
	}

	if (psDroid->sMove.pathIndex == 0)
	{
		psDroid->sMove.src = removeZ(psDroid->pos);
	}
	else
	{
		psDroid->sMove.src = psDroid->sMove.asPath[psDroid->sMove.pathIndex - 1];
	}
	psDroid->sMove.target = psDroid->sMove.asPath[psDroid->sMove.pathIndex];
	++psDroid->sMove.pathIndex;

	CHECK_DROID(psDroid);
	return true;
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
			return psObj->sDisplay.imd->radius / 2;

		case OBJ_FEATURE:
			return psObj->sDisplay.imd->radius / 2;

		default:
			ASSERT(false, "unknown object type");
			return 0;
	}
}


// see if a Droid has run over a person
static void moveCheckSquished(DROID *psDroid, int32_t emx, int32_t emy)
{
	int32_t		rad, radSq, objR, xdiff, ydiff, distSq;
	const int32_t	droidR = moveObjRadius((BASE_OBJECT *)psDroid);
	const int32_t   mx = gameTimeAdjustedAverage(emx, EXTRA_PRECISION);
	const int32_t   my = gameTimeAdjustedAverage(emy, EXTRA_PRECISION);

	static GridList gridList;  // static to avoid allocations.
	gridList = gridStartIterate(psDroid->pos.x, psDroid->pos.y, OBJ_MAXRADIUS);
	for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
	{
		BASE_OBJECT *psObj = *gi;
		if (psObj->type != OBJ_DROID || ((DROID *)psObj)->droidType != DROID_PERSON)
		{
			// ignore everything but people
			continue;
		}

		ASSERT(psObj->type == OBJ_DROID && ((DROID *)psObj)->droidType == DROID_PERSON, "squished - eerk");

		objR = moveObjRadius(psObj);
		rad = droidR + objR;
		radSq = rad*rad;

		xdiff = psDroid->pos.x + mx - psObj->pos.x;
		ydiff = psDroid->pos.y + my - psObj->pos.y;
		distSq = xdiff*xdiff + ydiff*ydiff;

		if (((2*radSq)/3) > distSq)
		{
			if ((psDroid->player != psObj->player) && !aiCheckAlliances(psDroid->player, psObj->player))
			{
				// run over a bloke - kill him
				destroyDroid((DROID *)psObj, gameTime);
				scoreUpdateVar(WD_BARBARIANS_MOWED_DOWN);
			}
		}
	}
}


// See if the droid has been stopped long enough to give up on the move
static bool moveBlocked(DROID *psDroid)
{
	SDWORD	xdiff,ydiff, diffSq;
	UDWORD	blockTime;

	if (psDroid->sMove.bumpTime == 0 || psDroid->sMove.bumpTime > gameTime)
	{
		// no bump - can't be blocked
		return false;
	}

	// See if the block can be cancelled
	if (abs(angleDelta(psDroid->rot.direction - psDroid->sMove.bumpDir)) > DEG(BLOCK_DIR))
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
		if (!isHumanPlayer(psDroid->player) && bMultiPlayer)
		{
			psDroid->lastFrustratedTime = gameTime;
			objTrace(psDroid->id, "FRUSTRATED");
		}
		else
		{
			objTrace(psDroid->id, "BLOCKED");
		}
		// if the unit cannot see the next way point - reroute it's got stuck
		if ((bMultiPlayer || psDroid->player == selectedPlayer || psDroid->lastFrustratedTime == gameTime)
		    && psDroid->sMove.pathIndex != psDroid->sMove.numPoints)
		{
			objTrace(psDroid->id, "Trying to reroute to (%d,%d)", psDroid->sMove.destination.x, psDroid->sMove.destination.y);
			moveDroidTo(psDroid, psDroid->sMove.destination.x, psDroid->sMove.destination.y);
			return false;
		}

		return true;
	}

	return false;
}


// Calculate the actual movement to slide around
static void moveCalcSlideVector(DROID *psDroid, int32_t objX, int32_t objY, int32_t *pMx, int32_t *pMy)
{
	int32_t dirX, dirY, dirMagSq, dotRes;
	const int32_t mx = *pMx;
	const int32_t my = *pMy;
	// Calculate the vector to the obstruction
	const int32_t obstX = psDroid->pos.x - objX;
	const int32_t obstY = psDroid->pos.y - objY;

	// if the target dir is the same, don't need to slide
	if (obstX*mx + obstY*my >= 0)
	{
		return;
	}

	// Choose the tangent vector to this on the same side as the target
	dotRes = obstY * mx - obstX * my;
	if (dotRes >= 0)
	{
		dirX = obstY;
		dirY = -obstX;
	}
	else
	{
		dirX = -obstY;
		dirY = obstX;
		dotRes = -dotRes;
	}
	dirMagSq = MAX(1, dirX * dirX + dirY * dirY);

	// Calculate the component of the movement in the direction of the tangent vector
	*pMx = (int64_t)dirX * dotRes / dirMagSq;
	*pMy = (int64_t)dirY * dotRes / dirMagSq;
}


static void moveOpenGates(DROID *psDroid, Vector2i tile)
{
	// is the new tile a gate?
	if (!worldOnMap(tile.x, tile.y))
	{
		return;
	}
	MAPTILE *psTile = mapTile(tile);
	if (!isFlying(psDroid) && psTile && psTile->psObject && psTile->psObject->type == OBJ_STRUCTURE && aiCheckAlliances(psTile->psObject->player, psDroid->player))
	{
		requestOpenGate((STRUCTURE *)psTile->psObject);  // If it's a friendly gate, open it. (It would be impolite to open an enemy gate.)
	}
}

static void moveOpenGates(DROID *psDroid)
{
	Vector2i pos = removeZ(psDroid->pos) + iSinCosR(psDroid->sMove.moveDir, psDroid->sMove.speed * SAS_OPEN_SPEED / GAME_TICKS_PER_SEC);
	moveOpenGates(psDroid, map_coord(pos));
}

// see if a droid has run into a blocking tile
// TODO See if this function can be simplified.
static void moveCalcBlockingSlide(DROID *psDroid, int32_t *pmx, int32_t *pmy, uint16_t tarDir, uint16_t *pSlideDir)
{
	PROPULSION_TYPE	propulsion = getPropulsionStats(psDroid)->propulsionType;
	SDWORD	horizX,horizY, vertX,vertY;
	uint16_t slideDir;
	// calculate the new coords and see if they are on a different tile
	const int32_t mx = gameTimeAdjustedAverage(*pmx, EXTRA_PRECISION);
	const int32_t my = gameTimeAdjustedAverage(*pmy, EXTRA_PRECISION);
	const int32_t tx = map_coord(psDroid->pos.x);
	const int32_t ty = map_coord(psDroid->pos.y);
	const int32_t nx = psDroid->pos.x + mx;
	const int32_t ny = psDroid->pos.y + my;
	const int32_t ntx = map_coord(nx);
	const int32_t nty = map_coord(ny);
	const int32_t blkCX = world_coord(ntx) + TILE_UNITS/2;
	const int32_t blkCY = world_coord(nty) + TILE_UNITS/2;

	CHECK_DROID(psDroid);

	// is the new tile a gate?
	moveOpenGates(psDroid, Vector2i(ntx, nty));

	// is the new tile blocking?
	if (!fpathBlockingTile(ntx, nty, propulsion))
	{
		// not blocking, don't change the move vector
		return;
	}

	// if the droid is shuffling - just stop
	if (psDroid->sMove.Status == MOVESHUFFLE)
	{
		objTrace(psDroid->id, "Was shuffling, now stopped");
		psDroid->sMove.Status = MOVEINACTIVE;
	}

	// note the bump time and position if necessary
	if (!isVtolDroid(psDroid) &&
		psDroid->sMove.bumpTime == 0)
	{
		psDroid->sMove.bumpTime = gameTime;
		psDroid->sMove.lastBump = 0;
		psDroid->sMove.pauseTime = 0;
		psDroid->sMove.bumpX = psDroid->pos.x;
		psDroid->sMove.bumpY = psDroid->pos.y;
		psDroid->sMove.bumpDir = psDroid->rot.direction;
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
			if (gameRand(2) == 0)
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
		int	intx = psDroid->pos.x & TILE_MASK;
		int	inty = psDroid->pos.y & TILE_MASK;
		bool	bJumped = false;
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
			*pmx = 0;
			*pmy = 0;
		}
		else
		{
			moveCalcSlideVector(psDroid, blkCX,blkCY, pmx,pmy);
		}
	}

	slideDir = iAtan2(*pmx, *pmy);
	if (ntx != tx)
	{
		// hit a horizontal block
		if ((tarDir < DEG(90) || tarDir > DEG(270)) &&
			(slideDir >= DEG(90) && slideDir <= DEG(270)))
		{
			slideDir = tarDir;
		}
		else if ((tarDir >= DEG(90) && tarDir <= DEG(270)) &&
				 (slideDir < DEG(90) || slideDir > DEG(270)))
		{
			slideDir = tarDir;
		}
	}
	if (nty != ty)
	{
		// hit a vertical block
		if ((tarDir < DEG(180)) &&
			(slideDir >= DEG(180)))
		{
			slideDir = tarDir;
		}
		else if ((tarDir >= DEG(180)) &&
				 (slideDir < DEG(180)))
		{
			slideDir = tarDir;
		}
	}
	*pSlideDir = slideDir;

	CHECK_DROID(psDroid);
}


// see if a droid has run into another droid
// Only consider stationery droids
static void moveCalcDroidSlide(DROID *psDroid, int *pmx, int *pmy)
{
	int32_t		droidR, rad, radSq, objR, xdiff, ydiff, distSq, spmx, spmy;
	bool            bLegs;

	CHECK_DROID(psDroid);

	bLegs = false;
	if (psDroid->droidType == DROID_PERSON || cyborgDroid(psDroid))
	{
		bLegs = true;
	}
	spmx = gameTimeAdjustedAverage(*pmx, EXTRA_PRECISION);
	spmy = gameTimeAdjustedAverage(*pmy, EXTRA_PRECISION);

	droidR = moveObjRadius((BASE_OBJECT *)psDroid);
	BASE_OBJECT *psObst = NULL;
	static GridList gridList;  // static to avoid allocations.
	gridList = gridStartIterate(psDroid->pos.x, psDroid->pos.y, OBJ_MAXRADIUS);
	for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
	{
		BASE_OBJECT *psObj = *gi;
		if (psObj->died)
		{
			continue;
		}
		if (psObj->type == OBJ_DROID)
		{
			if (isTransporter((DROID *)psObj))
			{
				// ignore transporters
				continue;
			}
			if (!bLegs && ((DROID *)psObj)->droidType == DROID_PERSON)
			{
				// everything else doesn't avoid people
				continue;
			}
			if (psObj->player == psDroid->player
			    && psDroid->lastFrustratedTime > 0
			    && gameTime - psDroid->lastFrustratedTime < FRUSTRATED_TIME)
			{
				continue; // clip straight through own units when sufficient frustrated -- using cheat codes!
			}
		}
		else
		{
			// ignore anything that isn't a droid
			continue;
		}

		objR = moveObjRadius(psObj);
		rad = droidR + objR;
		radSq = rad*rad;

		xdiff = psDroid->pos.x + spmx - psObj->pos.x;
		ydiff = psDroid->pos.y + spmy - psObj->pos.y;
		distSq = xdiff * xdiff + ydiff * ydiff;
		if (xdiff * spmx + ydiff * spmy >= 0)
		{
			// object behind
			continue;
		}

		if (radSq > distSq)
		{
			if (psObst != NULL)
			{
				// hit more than one droid - stop
				*pmx = 0;
				*pmy = 0;
				psObst = NULL;
				break;
			}
			else
			{
				psObst = psObj;

				// note the bump time and position if necessary
				if (psDroid->sMove.bumpTime == 0)
				{
					psDroid->sMove.bumpTime = gameTime;
					psDroid->sMove.lastBump = 0;
					psDroid->sMove.pauseTime = 0;
					psDroid->sMove.bumpX = psDroid->pos.x;
					psDroid->sMove.bumpY = psDroid->pos.y;
					psDroid->sMove.bumpDir = psDroid->rot.direction;
				}
				else
				{
					psDroid->sMove.lastBump = (UWORD)(gameTime - psDroid->sMove.bumpTime);
				}

				// tell inactive droids to get out the way
				if (psObst->type == OBJ_DROID)
				{
					DROID *psShuffleDroid = (DROID *)psObst;

					if (aiCheckAlliances(psObst->player, psDroid->player)
					    && psShuffleDroid->action != DACTION_WAITDURINGREARM
					    && psShuffleDroid->sMove.Status == MOVEINACTIVE)
					{
						moveShuffleDroid(psShuffleDroid, psDroid->sMove.target - removeZ(psDroid->pos));
					}
				}
			}
		}
	}

	if (psObst != NULL)
	{
		// Try to slide round it
		moveCalcSlideVector(psDroid, psObst->pos.x, psObst->pos.y, pmx, pmy);
	}
	CHECK_DROID(psDroid);
}

// get an obstacle avoidance vector
static Vector2i moveGetObstacleVector(DROID *psDroid, Vector2i dest)
{
	int32_t                 numObst = 0, distTot = 0;
	Vector2i                dir(0, 0);
	PROPULSION_STATS *      psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;

	ASSERT(psPropStats, "invalid propulsion stats pointer");

	int ourMaxSpeed = psPropStats->maxSpeed;
	int ourRadius = moveObjRadius(psDroid);
	if (ourMaxSpeed == 0)
	{
		return dest;  // No point deciding which way to go, if we can't move...
	}

	// scan the neighbours for obstacles
	static GridList gridList;  // static to avoid allocations.
	gridList = gridStartIterate(psDroid->pos.x, psDroid->pos.y, AVOID_DIST);
	for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
	{
		if (*gi == psDroid)
		{
			continue;  // Don't try to avoid ourselves.
		}

		DROID *psObstacle = castDroid(*gi);
		if (psObstacle == NULL)
		{
			// Object wrong type to worry about.
			continue;
		}

		// vtol droids only avoid each other and don't affect ground droids
		if (isVtolDroid(psDroid) != isVtolDroid(psObstacle))
		{
			continue;
		}

		if (isTransporter(psObstacle) ||
		    (psObstacle->droidType == DROID_PERSON &&
		     psObstacle->player != psDroid->player))
		{
			// don't avoid people on the other side - run over them
			continue;
		}

		PROPULSION_STATS *obstaclePropStats = asPropulsionStats + psObstacle->asBits[COMP_PROPULSION].nStat;
		int obstacleMaxSpeed = obstaclePropStats->maxSpeed;
		int obstacleRadius = moveObjRadius(psObstacle);
		int totalRadius = ourRadius + obstacleRadius;

		// Try to guess where the obstacle will be when we get close.
		// Velocity guess 1: Guess the velocity the droid is actually moving at.
		Vector2i obstVelocityGuess1 = iSinCosR(psObstacle->sMove.moveDir, psObstacle->sMove.speed);
		// Velocity guess 2: Guess the velocity the droid wants to move at.
		Vector2i obstTargetDiff = psObstacle->sMove.target - psObstacle->pos;
		Vector2i obstVelocityGuess2 = iSinCosR(iAtan2(obstTargetDiff), obstacleMaxSpeed * std::min(iHypot(obstTargetDiff), AVOID_DIST)/AVOID_DIST);
		if (moveBlocked(psObstacle))
		{
			obstVelocityGuess2 = Vector2i(0, 0);  // This obstacle isn't going anywhere, even if it wants to.
			//obstVelocityGuess2 = -obstVelocityGuess2;
		}
		// Guess the average of the two guesses.
		Vector2i obstVelocityGuess = (obstVelocityGuess1 + obstVelocityGuess2) / 2;

		// Find the guessed obstacle speed and direction, clamped to half our speed.
		int obstSpeedGuess = std::min(iHypot(obstVelocityGuess), ourMaxSpeed/2);
		uint16_t obstDirectionGuess = iAtan2(obstVelocityGuess);

		// Position of obstacle relative to us.
		Vector2i diff = removeZ(psObstacle->pos - psDroid->pos);

		// Find very approximate position of obstacle relative to us when we get close, based on our guesses.
		Vector2i deltaDiff = iSinCosR(obstDirectionGuess, (int64_t)std::max(iHypot(diff) - totalRadius*2/3, 0)*obstSpeedGuess / ourMaxSpeed);
		if (!fpathBlockingTile(map_coord(psObstacle->pos.x + deltaDiff.x), map_coord(psObstacle->pos.y + deltaDiff.y), obstaclePropStats->propulsionType))  // Don't assume obstacle can go through cliffs.
		{
			diff += deltaDiff;
		}

		if (diff * dest < 0)
		{
			// object behind
			continue;
		}

		int centreDist = std::max(iHypot(diff), 1);
		int dist = std::max(centreDist - totalRadius, 1);

		dir += diff * 65536 / (centreDist * dist);
		distTot += 65536 / dist;
		numObst += 1;
	}

	if (dir == Vector2i(0, 0))
	{
		return dest;
	}

	dir = Vector2i(dir.x / numObst, dir.y / numObst);
	distTot /= numObst;

	// Create the avoid vector
	Vector2i o(dir.y, -dir.x);
	Vector2i avoid = dest*o < 0? -o : o;

	// Normalise dest and avoid.
	dest = dest * 32768/(iHypot(dest) + 1);
	avoid = avoid * 32768/(iHypot(avoid) + 1);

	// combine the avoid vector and the target vector
	int ratio = std::min(distTot * ourRadius/2, 65536);

	return dest * (65536 - ratio) + avoid * ratio;
}

/*!
 * Get a direction for a droid to avoid obstacles etc.
 * \param psDroid Which droid to examine
 * \return The normalised direction vector
 */
static uint16_t moveGetDirection(DROID *psDroid)
{
	Vector2i src = removeZ(psDroid->pos);  // Do not want precice precision here, would overflow.
	Vector2i target = psDroid->sMove.target;
	Vector2i dest = target - src;

	// Transporters don't need to avoid obstacles, but everyone else should
	if (!isTransporter(psDroid))
	{
		dest = moveGetObstacleVector(psDroid, dest);
	}

	return iAtan2(dest);
}

// Check if a droid has got to a way point
static bool moveReachedWayPoint(DROID *psDroid)
{
	// Calculate the vector to the droid
	const Vector2i droid = removeZ(psDroid->pos) - psDroid->sMove.target;
	const bool last = psDroid->sMove.pathIndex == psDroid->sMove.numPoints;
	int sqprecision = last ? ((TILE_UNITS / 4) * (TILE_UNITS / 4)) : ((TILE_UNITS / 2) * (TILE_UNITS / 2));

	if (last && psDroid->sMove.bumpTime != 0)
	{
		// Make waypoint tolerance 1 tile after 0 seconds, 2 tiles after 3 seconds, X tiles after (X + 1)² seconds.
		sqprecision = (gameTime - psDroid->sMove.bumpTime + GAME_TICKS_PER_SEC)*(TILE_UNITS*TILE_UNITS / GAME_TICKS_PER_SEC);
	}

	// Else check current waypoint
	return droid*droid < sqprecision;
}

#define MAX_SPEED_PITCH  60

/** Calculate the new speed for a droid based on factors like pitch.
 *  @todo Remove hack for steep slopes not properly marked as blocking on some maps.
 */
SDWORD moveCalcDroidSpeed(DROID *psDroid)
{
	const uint16_t		maxPitch = DEG(MAX_SPEED_PITCH);
	UDWORD			mapX, mapY;
	int			speed, pitch;
	WEAPON_STATS		*psWStats;

	CHECK_DROID(psDroid);

	// NOTE: This screws up since the transporter is offscreen still (on a mission!), and we are trying to find terrainType of a tile (that is offscreen!)
	if (psDroid->droidType == DROID_TRANSPORTER && missionIsOffworld())
	{
		PROPULSION_STATS	*propulsion = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
		speed = propulsion->maxSpeed;
	}
	else
	{
		mapX = map_coord(psDroid->pos.x);
		mapY = map_coord(psDroid->pos.y);
		speed = calcDroidSpeed(psDroid->baseSpeed, terrainType(mapTile(mapX,mapY)), psDroid->asBits[COMP_PROPULSION].nStat, getDroidEffectiveLevel(psDroid));
	}


	// now offset the speed for the slope of the droid
	pitch = angleDelta(psDroid->rot.pitch);
	speed = (maxPitch - pitch) * speed / maxPitch;
	if (speed <= 10)
	{
		// Very nasty hack to deal with buggy maps, where some cliffs are
		// not properly marked as being cliffs, but too steep to drive over.
		// This confuses the heck out of the path-finding code! - Per
		speed = 10;
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

	// slow down shuffling VTOLs
	if (isVtolDroid(psDroid) &&
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
static bool moveDroidStopped(DROID* psDroid, SDWORD speed)
{
	if (psDroid->sMove.Status == MOVEINACTIVE && speed == 0 && psDroid->sMove.speed == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

// Direction is target direction.
static void moveUpdateDroidDirection(DROID *psDroid, SDWORD *pSpeed, uint16_t direction,
		uint16_t iSpinAngle, int iSpinSpeed, int iTurnSpeed, uint16_t *pDroidDir)
{
	*pDroidDir = psDroid->rot.direction;

	// don't move if in MOVEPAUSE state
	if (psDroid->sMove.Status == MOVEPAUSE)
	{
		return;
	}

	int diff = angleDelta(direction - *pDroidDir);
	// Turn while moving - slow down speed depending on target angle so that we can turn faster
	*pSpeed = std::max<int>(*pSpeed * (iSpinAngle - abs(diff)) / iSpinAngle, 0);

	// iTurnSpeed is turn speed at max velocity, increase turn speed up to iSpinSpeed when slowing down
	int turnSpeed = std::min<int>(iTurnSpeed + int64_t(iSpinSpeed - iTurnSpeed) * abs(diff) / iSpinAngle, iSpinSpeed);

	// Calculate the maximum change in direction
	int maxChange = gameTimeAdjustedAverage(turnSpeed);

	// Move *pDroidDir towards target, by at most maxChange.
	*pDroidDir += clip(diff, -maxChange, maxChange);
}


// Calculate current speed perpendicular to droids direction
static int moveCalcPerpSpeed(DROID *psDroid, uint16_t iDroidDir, SDWORD iSkidDecel)
{
	int adiff = angleDelta(iDroidDir - psDroid->sMove.moveDir);
	int perpSpeed = iSinR(abs(adiff), psDroid->sMove.speed);

	// decelerate the perpendicular speed
	perpSpeed = MAX(0, perpSpeed - gameTimeAdjustedAverage(iSkidDecel));

	return perpSpeed;
}


static void moveCombineNormalAndPerpSpeeds(DROID *psDroid, int fNormalSpeed, int fPerpSpeed, uint16_t iDroidDir)
{
	int16_t         adiff;
	int		relDir;
	int		finalSpeed;

	/* set current direction */
	psDroid->rot.direction = iDroidDir;

	/* set normal speed and direction if perpendicular speed is zero */
	if (fPerpSpeed == 0)
	{
		psDroid->sMove.speed = fNormalSpeed;
		psDroid->sMove.moveDir = iDroidDir;
		return;
	}

	finalSpeed = iHypot(fNormalSpeed, fPerpSpeed);

	// calculate the angle between the droid facing and movement direction
	relDir = iAtan2(fPerpSpeed, fNormalSpeed);

	// choose the finalDir on the same side as the old movement direction
	adiff = angleDelta(iDroidDir - psDroid->sMove.moveDir);

	psDroid->sMove.moveDir = adiff < 0 ? iDroidDir + relDir : iDroidDir - relDir;  // Cast wrapping intended.
	psDroid->sMove.speed = finalSpeed;
}


// Calculate the current speed in the droids normal direction
static int moveCalcNormalSpeed(DROID *psDroid, int fSpeed, uint16_t iDroidDir, SDWORD iAccel, SDWORD iDecel)
{
	uint16_t        adiff;
	int		normalSpeed;

	adiff = (uint16_t)(iDroidDir - psDroid->sMove.moveDir);  // Cast wrapping intended.
	normalSpeed = iCosR(adiff, psDroid->sMove.speed);

	if (normalSpeed < fSpeed)
	{
		// accelerate
		normalSpeed += gameTimeAdjustedAverage(iAccel);
		if (normalSpeed > fSpeed)
		{
			normalSpeed = fSpeed;
		}
	}
	else
	{
		// decelerate
		normalSpeed -= gameTimeAdjustedAverage(iDecel);
		if (normalSpeed < fSpeed)
		{
			normalSpeed = fSpeed;
		}
	}

	return normalSpeed;
}

static void moveGetDroidPosDiffs(DROID *psDroid, int32_t *pDX, int32_t *pDY)
{
	int32_t move = psDroid->sMove.speed * EXTRA_PRECISION;  // high precision

	*pDX = iSinR(psDroid->sMove.moveDir, move);
	*pDY = iCosR(psDroid->sMove.moveDir, move);
}

// see if the droid is close to the final way point
static void moveCheckFinalWaypoint( DROID *psDroid, SDWORD *pSpeed )
{
	int minEndSpeed = (*pSpeed + 2)/3;
	minEndSpeed = std::min(minEndSpeed, MIN_END_SPEED);

	// don't do this for VTOLs doing attack runs
	if (isVtolDroid(psDroid) && (psDroid->action == DACTION_VTOLATTACK))
	{
		return;
	}

	if (psDroid->sMove.Status != MOVESHUFFLE &&
	    psDroid->sMove.pathIndex == psDroid->sMove.numPoints)
	{
		Vector2i diff = removeZ(psDroid->pos) - psDroid->sMove.target;
		int distSq = diff*diff;
		if (distSq < END_SPEED_RANGE*END_SPEED_RANGE)
		{
			*pSpeed = (*pSpeed - minEndSpeed) * distSq / (END_SPEED_RANGE*END_SPEED_RANGE) + minEndSpeed;
		}
	}
}

static void moveUpdateDroidPos(DROID *psDroid, int32_t dx, int32_t dy)
{
	Position newPos;	// high precision coordinates (unusable for squared calculations)

	CHECK_DROID(psDroid);

	if (psDroid->sMove.Status == MOVEPAUSE || isDead((BASE_OBJECT *)psDroid))
	{
		// don't actually move if the move is paused
		return;
	}

	psDroid->pos.x += gameTimeAdjustedAverage(dx, EXTRA_PRECISION);
	psDroid->pos.y += gameTimeAdjustedAverage(dy, EXTRA_PRECISION);

	/* impact if about to go off map else update coordinates */
	if ( worldOnMap( psDroid->pos.x, psDroid->pos.y ) == false )
	{
		/* transporter going off-world will trigger next map, and is ok */
		ASSERT(isTransporter(psDroid), "droid trying to move off the map!");
		if (!isTransporter(psDroid))
		{
			/* dreadful last-ditch crash-avoiding hack - sort this! - GJ */
			destroyDroid(psDroid, gameTime);
			return;
		}
	}

	// lovely hack to keep transporters just on the map
	// two weeks to go and the hacks just get better !!!
	if (isTransporter(psDroid))
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
static void moveUpdateGroundModel(DROID *psDroid, SDWORD speed, uint16_t direction)
{
	int			fPerpSpeed, fNormalSpeed;
	uint16_t                iDroidDir;
	uint16_t                slideDir;
	PROPULSION_STATS	*psPropStats;
	int32_t                 spinSpeed, turnSpeed, spinAngle, skidDecel, dx, dy, bx, by;

	CHECK_DROID(psDroid);

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	switch (psPropStats->propulsionType)
	{
	case PROPULSION_TYPE_HOVER:
		spinSpeed = psDroid->baseSpeed * DEG(3)/4;
		turnSpeed = psDroid->baseSpeed * DEG(1)/4;
		spinAngle = DEG(180);
		skidDecel = HOVER_SKID_DECEL;
		break;
	case PROPULSION_TYPE_WHEELED:
		spinSpeed = psDroid->baseSpeed * DEG(3)/4;
		turnSpeed = psDroid->baseSpeed * DEG(1)/3;
		spinAngle = DEG(180);
		skidDecel = WHEELED_SKID_DECEL;
		break;
	case PROPULSION_TYPE_TRACKED:
	default:
		spinSpeed = psDroid->baseSpeed * DEG(3)/4;
		turnSpeed = psDroid->baseSpeed * DEG(1)/3;
		spinAngle = TRACKED_SPIN_ANGLE;
		skidDecel = TRACKED_SKID_DECEL;
		break;
	}

	// nothing to do if the droid is stopped
	if ( moveDroidStopped( psDroid, speed ) == true )
	{
		return;
	}

	// Used to update some kind of weird neighbour list here.

	moveCheckFinalWaypoint( psDroid, &speed );

	moveUpdateDroidDirection(psDroid, &speed, direction, spinAngle, spinSpeed, turnSpeed, &iDroidDir);

	fNormalSpeed = moveCalcNormalSpeed(psDroid, speed, iDroidDir, TRACKED_ACCEL, TRACKED_DECEL);
	fPerpSpeed   = moveCalcPerpSpeed(psDroid, iDroidDir, skidDecel);

	moveCombineNormalAndPerpSpeeds(psDroid, fNormalSpeed, fPerpSpeed, iDroidDir);
	moveGetDroidPosDiffs(psDroid, &dx, &dy);
	moveOpenGates(psDroid);
	moveCheckSquished(psDroid, dx,dy);
	moveCalcDroidSlide(psDroid, &dx, &dy);
	bx = dx;
	by = dy;
	moveCalcBlockingSlide(psDroid, &bx, &by, direction, &slideDir);
	if (bx != dx || by != dy)
	{
		moveUpdateDroidDirection(psDroid, &speed, slideDir, spinAngle, psDroid->baseSpeed*DEG(1), psDroid->baseSpeed*DEG(1)/3, &iDroidDir);
		psDroid->rot.direction = iDroidDir;
	}

	moveUpdateDroidPos(psDroid, bx, by);

	//set the droid height here so other routines can use it
	psDroid->pos.z = map_Height(psDroid->pos.x, psDroid->pos.y);//jps 21july96
	updateDroidOrientation(psDroid);
}

/* Update a persons position and speed given target values */
static void moveUpdatePersonModel(DROID *psDroid, SDWORD speed, uint16_t direction)
{
	int			fPerpSpeed, fNormalSpeed;
	int32_t			dx, dy;
	uint16_t                iDroidDir;
	uint16_t                slideDir;

	CHECK_DROID(psDroid);

	// nothing to do if the droid is stopped
	if ( moveDroidStopped( psDroid, speed ) == true )
	{
		if ( psDroid->droidType == DROID_PERSON &&
			 psDroid->order.type != DORDER_RUNBURN &&
			 (psDroid->action == DACTION_ATTACK ||
			  psDroid->action == DACTION_ROTATETOATTACK) )
		{
			/* remove previous anim */
			if ( psDroid->psCurAnim != NULL &&
				 psDroid->psCurAnim->psAnim->uwID != ID_ANIM_DROIDFIRE )
			{
				const bool bRet = animObj_Remove(psDroid->psCurAnim, psDroid->psCurAnim->psAnim->uwID);
				ASSERT(bRet, "animObj_Remove failed");
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

	// Used to update some kind of weird neighbour list here.

	moveUpdateDroidDirection(psDroid, &speed, direction, PERSON_SPIN_ANGLE, PERSON_SPIN_SPEED, PERSON_TURN_SPEED, &iDroidDir);

	fNormalSpeed = moveCalcNormalSpeed(psDroid, speed, iDroidDir, PERSON_ACCEL, PERSON_DECEL);

	/* people don't skid at the moment so set zero perpendicular speed */
	fPerpSpeed = 0;

	moveCombineNormalAndPerpSpeeds(psDroid, fNormalSpeed, fPerpSpeed, iDroidDir);
	moveGetDroidPosDiffs( psDroid, &dx, &dy );
	moveOpenGates(psDroid);
	moveCalcDroidSlide(psDroid, &dx,&dy);
	moveCalcBlockingSlide(psDroid, &dx, &dy, direction, &slideDir);
	moveUpdateDroidPos( psDroid, dx, dy );

	//set the droid height here so other routines can use it
	psDroid->pos.z = map_Height(psDroid->pos.x, psDroid->pos.y);//jps 21july96

	/* update anim if moving and not on fire */
	if ( psDroid->droidType == DROID_PERSON && speed != 0 &&
		 psDroid->order.type != DORDER_RUNBURN )
	{
		/* remove previous anim */
		if ( psDroid->psCurAnim != NULL &&
			 (psDroid->psCurAnim->psAnim->uwID != ID_ANIM_DROIDRUN ||
			  psDroid->psCurAnim->psAnim->uwID != ID_ANIM_DROIDRUN)   )
		{
			const bool bRet = animObj_Remove(psDroid->psCurAnim, psDroid->psCurAnim->psAnim->uwID);
			ASSERT(bRet, "animObj_Remove failed" );
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
				const bool bRet = animObj_Remove(psDroid->psCurAnim, psDroid->psCurAnim->psAnim->uwID);
				ASSERT(bRet, "animObj_Remove failed");
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

#define	VTOL_VERTICAL_SPEED		(((psDroid->baseSpeed / 4) > 60) ? ((SDWORD)psDroid->baseSpeed / 4) : 60)

/* primitive 'bang-bang' vtol height controller */
static void moveAdjustVtolHeight(DROID * psDroid, int32_t iMapHeight)
{
	int32_t	iMinHeight, iMaxHeight, iLevelHeight;
	if (isTransporter(psDroid) && !bMultiPlayer)
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
	psDroid->pos.z = map_Height(psDroid->pos.x,psDroid->pos.y) + VTOL_HEIGHT_LEVEL;
}

static void moveUpdateVtolModel(DROID *psDroid, SDWORD speed, uint16_t direction)
{
	int fPerpSpeed, fNormalSpeed;
	uint16_t   iDroidDir;
	uint16_t   slideDir;
	int32_t iMapZ, iSpinSpeed, iTurnSpeed, dx, dy;
	uint16_t targetRoll;

	CHECK_DROID(psDroid);

	// nothing to do if the droid is stopped
	if ( moveDroidStopped(  psDroid, speed ) == true )
	{
		return;
	}

	moveCheckFinalWaypoint( psDroid, &speed );

	if (isTransporter(psDroid))
	{
		moveUpdateDroidDirection(psDroid, &speed, direction, VTOL_SPIN_ANGLE, VTOL_SPIN_SPEED, VTOL_TURN_SPEED, &iDroidDir);
	}
	else
	{
		iSpinSpeed = std::max<int>(psDroid->baseSpeed*DEG(1)/2, VTOL_SPIN_SPEED);
		iTurnSpeed = std::max<int>(psDroid->baseSpeed*DEG(1)/8, VTOL_TURN_SPEED);
		moveUpdateDroidDirection(psDroid, &speed, direction, VTOL_SPIN_ANGLE, iSpinSpeed, iTurnSpeed, &iDroidDir);
	}

	fNormalSpeed = moveCalcNormalSpeed(psDroid, speed, iDroidDir, VTOL_ACCEL, VTOL_DECEL);
	fPerpSpeed   = moveCalcPerpSpeed(psDroid, iDroidDir, VTOL_SKID_DECEL);

	moveCombineNormalAndPerpSpeeds(psDroid, fNormalSpeed, fPerpSpeed, iDroidDir);

	moveGetDroidPosDiffs( psDroid, &dx, &dy );

	/* set slide blocking tile for map edge */
	if (!isTransporter(psDroid))
	{
		moveCalcBlockingSlide(psDroid, &dx, &dy, direction, &slideDir);
	}

	moveUpdateDroidPos( psDroid, dx, dy );

	/* update vtol orientation */
	targetRoll = clip(4 * angleDelta(psDroid->sMove.moveDir - psDroid->rot.direction), -DEG(60), DEG(60));
	psDroid->rot.roll = psDroid->rot.roll + (uint16_t)gameTimeAdjustedIncrement(3 * angleDelta(targetRoll - psDroid->rot.roll));

	/* do vertical movement - only if on the map */
	if (worldOnMap(psDroid->pos.x, psDroid->pos.y))
	{
		iMapZ = map_Height(psDroid->pos.x, psDroid->pos.y);
		psDroid->pos.z = MAX(iMapZ, psDroid->pos.z + gameTimeAdjustedIncrement(psDroid->sMove.iVertSpeed));
		moveAdjustVtolHeight(psDroid, iMapZ);
	}
}

#define CYBORG_VERTICAL_SPEED	((int)psDroid->baseSpeed / 2)

static void
moveCyborgLaunchAnimDone( ANIM_OBJECT *psObj )
{
	DROID	*psDroid = (DROID*)psObj->psParent;

	ASSERT( psDroid != NULL,
			"moveCyborgLaunchAnimDone: invalid cyborg pointer" );

	/* raise cyborg a little bit so flying - terrible hack - GJ */
	// Actually, worse than a terrible hack, since it would break synch...
	//psDroid->pos.z++;
	//psDroid->sMove.iVertSpeed = CYBORG_VERTICAL_SPEED;

	psDroid->psCurAnim = NULL;
}

static void
moveCyborgTouchDownAnimDone( ANIM_OBJECT *psObj )
{
	DROID	*psDroid = (DROID*)psObj->psParent;

	ASSERT( psDroid != NULL,
			"moveCyborgTouchDownAnimDone: invalid cyborg pointer" );

	psDroid->psCurAnim = NULL;
	// See comment in moveCyborgLaunchAnimDone().
	//psDroid->pos.z = map_Height( psDroid->pos.x, psDroid->pos.y );
}


static void moveUpdateJumpCyborgModel(DROID *psDroid, SDWORD speed, uint16_t direction)
{
	int	fPerpSpeed, fNormalSpeed;
	uint16_t iDroidDir;
	int32_t dx, dy;

	// nothing to do if the droid is stopped
	if ( moveDroidStopped(  psDroid, speed ) == true )
	{
		return;
	}

	// Used to update some kind of weird neighbour list here.

	moveUpdateDroidDirection(psDroid, &speed, direction, VTOL_SPIN_ANGLE, psDroid->baseSpeed*DEG(1), psDroid->baseSpeed*DEG(1)/3, &iDroidDir);

	fNormalSpeed = moveCalcNormalSpeed(psDroid, speed, iDroidDir, VTOL_ACCEL, VTOL_DECEL);
	fPerpSpeed   = 0;
	moveCombineNormalAndPerpSpeeds(psDroid, fNormalSpeed, fPerpSpeed, iDroidDir);

	moveGetDroidPosDiffs( psDroid, &dx, &dy );
	moveUpdateDroidPos( psDroid, dx, dy );
}

static void moveUpdateCyborgModel(DROID *psDroid, SDWORD moveSpeed, uint16_t moveDir, UBYTE oldStatus)
{
	int32_t                 iMapZ = map_Height(removeZ(psDroid->pos));

	CHECK_DROID(psDroid);

	// nothing to do if the droid is stopped
	if ( moveDroidStopped( psDroid, moveSpeed ) == true )
	{
		if ( psDroid->psCurAnim != NULL )
		{
			if (!animObj_Remove(psDroid->psCurAnim, psDroid->psCurAnim->uwID))
			{
				debug(LOG_NEVER, "couldn't remove walk anim");
			}
			psDroid->psCurAnim = NULL;
		}

		return;
	}

	PROPULSION_STATS *psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( psPropStats != NULL,
			"moveUpdateCyborgModel: invalid propulsion stats pointer" );

	/* do vertical movement */
	if (psPropStats->propulsionType == PROPULSION_TYPE_JUMP)
	{
		int iDz = gameTimeAdjustedIncrement(psDroid->sMove.iVertSpeed);
		int iDroidZ = psDroid->pos.z;

		if (iDroidZ + iDz < iMapZ)
		{
			psDroid->sMove.iVertSpeed = 0;
			psDroid->pos.z = iMapZ;
		}
		else
		{
			psDroid->pos.z = psDroid->pos.z + iDz;
		}
		if (psDroid->pos.z >= iMapZ + CYBORG_MAX_JUMP_HEIGHT && psDroid->sMove.iVertSpeed > 0)
		{
			psDroid->sMove.iVertSpeed = -CYBORG_VERTICAL_SPEED;
		}
	}

	/* calculate move distance */
	Vector2i iD = psDroid->sMove.destination - removeZ(psDroid->pos);
	int32_t iDist = iHypot(iD);

	/* set jumping cyborg walking short distances */
	if ( (psPropStats->propulsionType != PROPULSION_TYPE_JUMP) ||
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
					psDroid->psCurAnim = animObj_Add(psDroid, ID_ANIM_SUPERCYBORG_RUN, 0, 0);
				}
				else if (cyborgDroid(psDroid))
				{
					psDroid->psCurAnim = animObj_Add(psDroid, ID_ANIM_CYBORG_RUN, 0, 0);
				}
			}
		} else {
			// If the droid went off screen then remove the animation, saves memory and time.
			if(!clipXY(psDroid->pos.x,psDroid->pos.y))
			{
				const bool bRet = animObj_Remove(psDroid->psCurAnim, psDroid->psCurAnim->psAnim->uwID);
				ASSERT(bRet, "animObj_Remove failed");
				psDroid->psCurAnim = NULL;
			}
		}

		/* use baba person movement */
		moveUpdatePersonModel(psDroid, moveSpeed, moveDir);
	}
	else
	{
		/* jumping cyborg: remove walking animation if present */
		if ( psDroid->psCurAnim != NULL )
		{
			if ((psDroid->psCurAnim->uwID == ID_ANIM_CYBORG_RUN
			  || psDroid->psCurAnim->uwID == ID_ANIM_SUPERCYBORG_RUN
			  || psDroid->psCurAnim->uwID == ID_ANIM_CYBORG_PACK_RUN)
			 && !animObj_Remove(psDroid->psCurAnim, psDroid->psCurAnim->uwID))
			{
				debug(LOG_NEVER, "couldn't remove walk anim");
			}
			psDroid->psCurAnim = NULL;
		}

		/* add jumping or landing anim */
		if ( (oldStatus == MOVEPOINTTOPOINT) &&
				  (psDroid->sMove.Status == MOVEINACTIVE) )
		{
			psDroid->psCurAnim = animObj_Add(psDroid, ID_ANIM_CYBORG_PACK_LAND, 0, 1);
			animObj_SetDoneFunc( psDroid->psCurAnim, moveCyborgTouchDownAnimDone );
		}
		else if ( psDroid->sMove.Status == MOVEPOINTTOPOINT )
		{
			if ( psDroid->pos.z == iMapZ )
			{
				if ( psDroid->sMove.iVertSpeed == 0 )
				{
					psDroid->psCurAnim = animObj_Add(psDroid, ID_ANIM_CYBORG_PACK_JUMP, 0, 1);
					animObj_SetDoneFunc( psDroid->psCurAnim, moveCyborgLaunchAnimDone );
				}
				else
				{
					psDroid->psCurAnim = animObj_Add(psDroid, ID_ANIM_CYBORG_PACK_LAND, 0, 1);
					animObj_SetDoneFunc( psDroid->psCurAnim, moveCyborgTouchDownAnimDone );
				}
			}
			else
			{
				moveUpdateJumpCyborgModel(psDroid, moveSpeed, moveDir);
			}
		}
	}

	psDroid->rot.pitch = 0;
	psDroid->rot.roll  = 0;
}

static void moveDescending( DROID *psDroid )
{
	int32_t iMapHeight = map_Height( psDroid->pos.x, psDroid->pos.y);

	psDroid->sMove.speed = 0;

	if ( psDroid->pos.z > iMapHeight)
	{
		/* descending */
		psDroid->sMove.iVertSpeed = (SWORD)-VTOL_VERTICAL_SPEED;
	}
	else
	{
		/* on floor - stop */
		psDroid->pos.z = iMapHeight;
		psDroid->sMove.iVertSpeed = 0;

		/* reset move state */
		psDroid->sMove.Status = MOVEINACTIVE;

		/* conform to terrain */
		updateDroidOrientation(psDroid);
	}
}


bool moveCheckDroidMovingAndVisible( void *psObj )
{
	DROID	*psDroid = (DROID*)psObj;

	if ( psDroid == NULL )
	{
		return false;
	}

	/* check for dead, not moving or invisible to player */
	if ( psDroid->died || moveDroidStopped( psDroid, 0 ) ||
		 (isTransporter(psDroid) && psDroid->order.type == DORDER_NONE) ||
		 !(psDroid->visible[selectedPlayer])                                )
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
		 (psDroid->visible[selectedPlayer]) )
	{
		iPropType = asPropulsionStats[(psDroid)->asBits[COMP_PROPULSION].nStat].propulsionType;
		psPropType = &asPropulsionTypes[iPropType];

		/* play specific wheeled and transporter or stats-specified noises */
		if ( iPropType == PROPULSION_TYPE_WHEELED && psDroid->droidType != DROID_CONSTRUCT )
		{
			iAudioID = ID_SOUND_TREAD;
		}
		else if (isTransporter(psDroid))
		{
			iAudioID = ID_SOUND_BLIMP_FLIGHT;
		}
		else if (iPropType == PROPULSION_TYPE_LEGGED && cyborgDroid(psDroid))
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


static bool moveDroidStartCallback( void *psObj )
{
	DROID *psDroid = (DROID*)psObj;

	if ( psDroid == NULL )
	{
		return false;
	}

	movePlayDroidMoveAudio( psDroid );

	return true;
}


static void movePlayAudio( DROID *psDroid, bool bStarted, bool bStoppedBefore, SDWORD iMoveSpeed )
{
	UBYTE				propType;
	PROPULSION_STATS	*psPropStats;
	PROPULSION_TYPES	*psPropType;
	bool				bStoppedNow;
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
		if ((propType == PROPULSION_TYPE_WHEELED && psDroid->droidType != DROID_CONSTRUCT)
		    || psPropType->startID == NO_SOUND)
		{
			movePlayDroidMoveAudio( psDroid );
			return;
		}
		else if (isTransporter(psDroid))
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
		if (isTransporter(psDroid))
		{
			iAudioID = ID_SOUND_BLIMP_LAND;
		}
		else if ( propType != PROPULSION_TYPE_WHEELED || psDroid->droidType == DROID_CONSTRUCT )
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
		 (psDroid->visible[selectedPlayer]) )
	{
		if ( audio_PlayObjDynamicTrack( psDroid, iAudioID,
				pAudioCallback ) )
		{
			psDroid->iAudioID = iAudioID;
		}
	}
}


static bool pickupOilDrum(int toPlayer, int fromPlayer)
{
	addPower(toPlayer, OILDRUM_POWER);  // give power

	if (toPlayer == selectedPlayer)
	{
		CONPRINTF(ConsoleString, (ConsoleString, _("You found %u power in an oil drum."), OILDRUM_POWER));
	}

	// fromPlayer == ANYPLAYER seems to mean that the drum was not pre-placed on the map.
	if (bMultiPlayer && fromPlayer == ANYPLAYER)
	{
		// when player finds oil, we init the timer, and flag that we need a drum
		if (!oilTimer)
		{
			oilTimer = gameTime;
		}
		// if player finds more than one drum (before timer expires), then we tack on ~50 sec to timer.
		if (drumCount++ == 0)
		{
			oilTimer += GAME_TICKS_PER_SEC * 50;
		}
	}

	return true;
}

// called when a droid moves to a new tile.
// use to pick up oil, etc..
static void checkLocalFeatures(DROID *psDroid)
{
	// NOTE: Why not do this for AI units also?
	if ((!isHumanPlayer(psDroid->player) && psDroid->order.type != DORDER_RECOVER) || isVtolDroid(psDroid) || isTransporter(psDroid))  // VTOLs or transporters can't pick up features!
	{
		return;
	}

	// scan the neighbours
#define DROIDDIST ((TILE_UNITS*5)/2)
	static GridList gridList;  // static to avoid allocations.
	gridList = gridStartIterate(psDroid->pos.x, psDroid->pos.y, DROIDDIST);
	for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
	{
		BASE_OBJECT *psObj = *gi;
		bool pickedUp = false;

		if (psObj->type == OBJ_FEATURE && !psObj->died)
		{
			switch (((FEATURE *)psObj)->psStats->subType)
			{
				case FEAT_OIL_DRUM:
					pickedUp = pickupOilDrum(psDroid->player, psObj->player);
					break;
				case FEAT_GEN_ARTE:
					pickedUp = pickupArtefact(psDroid->player, psObj->player);
					break;
				default:
					break;
			}
		}

		if (!pickedUp)
		{
			// Object is not a living oil drum or artefact.
			continue;
		}

		turnOffMultiMsg(true);
		removeFeature((FEATURE *)psObj);  // remove artifact+.
		turnOffMultiMsg(false);
	}

	// once they found a oil drum, we then wait ~600 secs before we pop up new one(s).
	if (oilTimer + GAME_TICKS_PER_SEC * 600u < gameTime && drumCount > 0)
	{
		addOilDrum(drumCount);
		oilTimer = 0;
		drumCount = 0;
	}
}


/* Frame update for the movement of a tracked droid */
void moveUpdateDroid(DROID *psDroid)
{
	UDWORD				oldx, oldy;
	UBYTE				oldStatus = psDroid->sMove.Status;
	SDWORD				moveSpeed;
	uint16_t			moveDir;
	PROPULSION_STATS	*psPropStats;
	Vector3i 			pos;
	bool				bStarted = false, bStopped;

	CHECK_DROID(psDroid);

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT_OR_RETURN(, psPropStats != NULL, "Invalid propulsion stats pointer");

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
	moveDir = psDroid->rot.direction;

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
	case MOVESHUFFLE:
		if (moveReachedWayPoint(psDroid) || (psDroid->sMove.shuffleStart + MOVE_SHUFFLETIME) < gameTime)
		{
			if (psPropStats->propulsionType == PROPULSION_TYPE_LIFT)
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
			moveDir = moveGetDirection(psDroid);

			moveSpeed = moveCalcDroidSpeed(psDroid);
		}
		break;
	case MOVEWAITROUTE:
		moveDroidTo(psDroid, psDroid->sMove.destination.x, psDroid->sMove.destination.y);
		moveSpeed = MAX(0, psDroid->sMove.speed - 1);
		if (psDroid->sMove.Status != MOVENAVIGATE)
		{
			break;
		}
		// No break.
	case MOVENAVIGATE:
		// Get the next control point
		if (!moveNextTarget(psDroid))
		{
			// No more waypoints - finish
			if ( psPropStats->propulsionType == PROPULSION_TYPE_LIFT )
			{
				psDroid->sMove.Status = MOVEHOVER;
			}
			else
			{
				psDroid->sMove.Status = MOVEINACTIVE;
			}
			break;
		}

		if (isVtolDroid(psDroid))
		{
			psDroid->rot.pitch = 0;
		}

		psDroid->sMove.Status = MOVEPOINTTOPOINT;
		psDroid->sMove.bumpTime = 0;
		moveSpeed = MAX(0, psDroid->sMove.speed - 1);

		/* save started status for movePlayAudio */
		if (psDroid->sMove.speed == 0)
		{
			bStarted = true;
		}

		// No break.
	case MOVEPOINTTOPOINT:
	case MOVEPAUSE:
		// moving between two way points
		if (psDroid->sMove.numPoints == 0)
		{
			debug(LOG_WARNING, "No path to follow, but psDroid->sMove.Status = %d", psDroid->sMove.Status);
		}
		else
		{
			ASSERT_OR_RETURN(, psDroid->sMove.asPath != NULL, "NULL path of non-zero length!");
		}

		// Get the best control point.
		if (psDroid->sMove.numPoints == 0 || !moveBestTarget(psDroid))
		{
			// Got stuck somewhere, can't find the path.
			moveDroidTo(psDroid, psDroid->sMove.destination.x, psDroid->sMove.destination.y);
		}

		// See if the target point has been reached
		if (moveReachedWayPoint(psDroid))
		{
			// Got there - move onto the next waypoint
			if (!moveNextTarget(psDroid))
			{
				// No more waypoints - finish
				if ( psPropStats->propulsionType == PROPULSION_TYPE_LIFT )
				{
					// check the location for vtols
					Vector2i tar = removeZ(psDroid->pos);
					if (psDroid->order.type != DORDER_PATROL && psDroid->order.type != DORDER_CIRCLE  // Not doing an order which means we never land (which means we might want to land).
					    && psDroid->action != DACTION_MOVETOREARM && psDroid->action != DACTION_MOVETOREARMPOINT
					    && actionVTOLLandingPos(psDroid, &tar)  // Can find a sensible place to land.
					    && map_coord(tar) != map_coord(psDroid->sMove.destination))  // We're not at the right place to land.
					{
						psDroid->sMove.destination = tar;
						moveDroidTo(psDroid, psDroid->sMove.destination.x, psDroid->sMove.destination.y);
					}
					else
					{
						psDroid->sMove.Status = MOVEHOVER;
					}
				}
				else
				{
					psDroid->sMove.Status = MOVETURN;
				}
				objTrace(psDroid->id, "Arrived at destination!");
				break;
			}
		}

		moveDir = moveGetDirection(psDroid);
		moveSpeed = moveCalcDroidSpeed(psDroid);

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
		if ( psPropStats->propulsionType == PROPULSION_TYPE_LIFT )
		{
			psDroid->sMove.Status = MOVEPOINTTOPOINT;
		}
		else
		{
			psDroid->sMove.Status = MOVEINACTIVE;
		}
		break;
	case MOVETURNTOTARGET:
		moveSpeed = 0;
		moveDir = iAtan2(psDroid->sMove.target - removeZ(psDroid->pos));
		break;
	case MOVEHOVER:
		moveDescending(psDroid);
		break;
	// Driven around by the player.
	case MOVEDRIVE:
		driveSetDroidMove(psDroid);
		moveSpeed = driveGetMoveSpeed();	//psDroid->sMove.speed;
		moveDir = DEG(driveGetMoveDir());		//psDroid->sMove.dir;
		break;

	default:
		ASSERT( false, "moveUpdateUnit: unknown move state" );
		break;
	}

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
	else if ( psPropStats->propulsionType == PROPULSION_TYPE_LIFT )
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
		visTilesUpdate((BASE_OBJECT *)psDroid);

		// object moved from one tile to next, check to see if droid is near stuff.(oil)
		checkLocalFeatures(psDroid);
	}

	// See if it's got blocked
	if ( (psPropStats->propulsionType != PROPULSION_TYPE_LIFT) && moveBlocked(psDroid) )
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
	if (worldOnMap(psDroid->pos.x, psDroid->pos.y) && terrainType(mapTile(map_coord(psDroid->pos.x), map_coord(psDroid->pos.y))) == TER_WATER)
	{
		updateDroidOrientation(psDroid);
	}

	if (psDroid->sMove.Status == MOVETURNTOTARGET && psDroid->rot.direction == moveDir)
	{
		if (psPropStats->propulsionType == PROPULSION_TYPE_LIFT)
		{
			psDroid->sMove.Status = MOVEPOINTTOPOINT;
		}
		else
		{
			psDroid->sMove.Status = MOVEINACTIVE;
		}
		objTrace(psDroid->id, "MOVETURNTOTARGET complete");
	}

	if (psDroid->burnStart != 0 && psDroid->droidType != DROID_PERSON && psDroid->visible[selectedPlayer])
	{
		pos.x = psDroid->pos.x + (18-rand()%36);
		pos.z = psDroid->pos.y + (18-rand()%36);
		pos.y = psDroid->pos.z + (psDroid->sDisplay.imd->max.y / 3);
		addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, NULL, 0, gameTime - deltaGameTime + 1);
	}

	movePlayAudio( psDroid, bStarted, bStopped, moveSpeed );
	ASSERT(droidOnMap(psDroid), "%s moved off map (%u, %u)->(%u, %u)", droidGetName(psDroid), oldx, oldy, (UDWORD)psDroid->pos.x, (UDWORD)psDroid->pos.y);
	CHECK_DROID(psDroid);
}
