/*
 * Move.c
 *
 * Routines for moving units about the map
 *
 */

/* Movement printfs */
//#define DEBUG_GROUP1
/* calc turn printfs */
//#define DEBUG_GROUP2
/* obstacle avoid printfs */
//#define DEBUG_GROUP3
/* Reroute printf's */
//#define DEBUG_GROUP4
// boundary printf's
//#define DEBUG_GROUP5
// waiting droid printf's
//#define DEBUG_GROUP6

#include <stdio.h>
#include <math.h>
#include "lib/framework/frame.h"
#include "lib/framework/trig.h"

//#define DEBUG_DRIVE_SPEED
#ifdef DEBUG
BOOL	moveDoMessage;
#undef DBP6
#define DBP6( x ) \
	if (moveDoMessage) \
		debug( LOG_NEVER, x )
#endif


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
#include "display3d.h"
#include "order.h"
#include "astar.h"
#include "combat.h"
#include "mapgrid.h"
#include "display.h"	// needed for widgetsOn flag.
#include "effects.h"
#include "lib/framework/fractions.h"
#include "power.h"
#include "scores.h"

#include "optimisepath.h"

//#include "multigifts.h"
#include "drive.h"

#ifdef ARROWS
#include "arrow.h"
#endif


#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "multigifts.h"





//static BOOL DebugP=FALSE;


/* system definitions */

#define	DROID_RUN_SOUND			1


// get rid of the fast rounding for the movement code

#define MAKEINT(x) ((SDWORD)(x))



#define	FORMATIONS_DISABLE		0

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
#define BLOCK_MINRATIO	FRACTCONST(99,100)
#define BLOCK_MAXRATIO	FRACTCONST(101,100)
// The result of the dot product for two vectors to be the same direction
#define BLOCK_DOTVAL	FRACTCONST(99,100)

// How far out from an obstruction to start avoiding it
#define AVOID_DIST		(TILE_UNITS*2)

/* Number of game units/sec for base speed */
#define BASE_SPEED		1

/* Number of degrees/sec for base turn rate */
#define BASE_TURN		1

/* What the base speed is intialised to */
#define BASE_SPEED_INIT		FRACTCONST(BASE_SPEED, BASE_DEF_RATE)

/* What the frame rate is assumed to be at start up */
#define BASE_DEF_RATE	25

/* What the base turn rate is intialised to */
#define BASE_TURN_INIT		FRACTCONST(BASE_TURN, BASE_DEF_RATE)

// maximum and minimum speed to approach a final way point
#define MAX_END_SPEED		300
#define MIN_END_SPEED		60

// distance from final way point to start slowing
#define END_SPEED_RANGE		(3 * TILE_UNITS)


// times for rerouting
#define REROUTE_BASETIME	200
#define REROUTE_RNDTIME		400

// how long to pause after firing a FOM_NO weapon
#define FOM_MOVEPAUSE		1500

// distance to consider droids for a shuffle
#define SHUFFLE_DIST		(3*TILE_UNITS/2)
// how far to move for a shuffle
#define SHUFFLE_MOVE		(2*TILE_UNITS/2)

/***********************************************************************************/
/*                      Slope defines                                              */

// angle after which the droid starts to turn back down a slope
#define SLOPE_TURN_ANGLE	50

// max and min ranges of roll for controlling how much to turn
#define SLOPE_MIN_ROLL		5
#define SLOPE_MAX_ROLL		30

// base amount to turn
#define SLOPE_DIR_CHANGE	20

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
FRACT	baseSpeed;
#define	BASE_FRAMES			10
UDWORD	baseTimes[BASE_FRAMES];

/* The current base turn rate */
FRACT	baseTurn;

// The next object that should get the router when a lot of units are
// in a MOVEROUTE state
DROID	*psNextRouteDroid;

/* Function prototypes */
BOOL	tileInRange(UDWORD tileX, UDWORD tileY, DROID *psDroid);
void	fillNewBlocks(DROID *psDroid);
void	fillInitialView(DROID *psDroid);
void	moveUpdatePersonModel(DROID *psDroid, SDWORD speed, SDWORD direction);
// Calculate the boundary vector
void	moveCalcBoundary(DROID *psDroid);
/* Turn a vector into an angle - returns a FRACT (!) */
static FRACT vectorToAngle(FRACT vx, FRACT vy);

/* Calculate the angle between two normalised vectors */
#define VECTOR_ANGLE(vx1,vy1, vx2,vy2) \
	trigInvCos(FRACTmul(vx1, vx2) + FRACTmul(vy1,vy2))

// Abbreviate some of the FRACT defines
#define MKF(x)		MAKEFRACT(x)
#define MKI(x)		MAKEINT(x)
#define FCONST(x,y)	FRACTCONST(x,y)
#define Fmul(x,y)	FRACTmul(x,y)
#define Fdiv(x,y)	FRACTdiv(x,y)
#define Fabs(x)		FRACTabs(x)

//typedef enum MOVESOUNDTYPE	{ MOVESOUNDSTART, MOVESOUNDIDLE, MOVESOUNDMOVEOFF,
//								MOVESOUNDMOVE, MOVESOUNDSTOPHISS, MOVESOUNDSHUTDOWN };

extern UDWORD	selectedPlayer;


static BOOL	g_bFormationSpeedLimitingOn = TRUE;




/* Initialise the movement system */
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

	return TRUE;
}

/* Update the base speed for all movement */
void moveUpdateBaseSpeed(void)
{
//	UDWORD	totalTime=0, numFrames=0, i;
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
	// baseSpeed = (totalTime * BASE_SPEED) / (GAME_TICKS_PER_SEC * BASE_FRAMES);
	baseSpeed = FRACTdiv( FRACTmul(MAKEFRACT(BASE_SPEED), MAKEFRACT(totalTime)),
						  FRACTmul(MAKEFRACT(GAME_TICKS_PER_SEC), MAKEFRACT(BASE_FRAMES)) );

	// Set the base turn rate
	baseTurn = FRACTdiv( FRACTmul(MAKEFRACT(BASE_TURN), MAKEFRACT(totalTime)),
						 FRACTmul(MAKEFRACT(GAME_TICKS_PER_SEC), MAKEFRACT(BASE_FRAMES)) );


	// reset the astar counters
	astarResetCounters();

	// check the waiting droid pointer
	if (psNextRouteDroid != NULL)
	{
		if ((psNextRouteDroid->died) ||
			((psNextRouteDroid->sMove.Status != MOVEROUTE) &&
			 (psNextRouteDroid->sMove.Status != MOVEROUTESHUFFLE)))
		{
			DBP6(("Waiting droid %d (player %d) reset\n", psNextRouteDroid->id, psNextRouteDroid->player));
			psNextRouteDroid = NULL;
		}
	}
}

/* Set a target location for a droid to move to */
// Now returns a BOOL based on the success of the routing
// returns TRUE if the routing was successful ... if FALSE then the calling code should not try to route here again for a while
BOOL _moveDroidToBase(DROID	*psDroid, UDWORD x, UDWORD y, BOOL bFormation)
{
	FPATH_RETVAL		retVal = FPR_OK;
	SDWORD				fmx1,fmy1, fmx2,fmy2;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"moveUnitTo: Invalid unit pointer" );


	if(bMultiPlayer && (psDroid->sMove.Status != MOVEWAITROUTE))
	{
		if(SendDroidMove(psDroid,x,y,bFormation) == FALSE)
		{// dont make the move since we'll recv it anyway
			return FALSE;
		}
	}


//	DBPRINTF(("movedroidto (%d,%d) -> (%d,%d)\n",psDroid->x,psDroid->y,x,y);

#if FORMATIONS_DISABLE
	retVal = FPR_OK;
	fpathSetDirectRoute(psDroid, (SDWORD)x, (SDWORD)y);
#else
    //in multiPlayer make Transporter move like the vtols
	if ( psDroid->droidType == DROID_TRANSPORTER AND game.maxPlayers == 0)
	{
		fpathSetDirectRoute((BASE_OBJECT *)psDroid, (SDWORD)x, (SDWORD)y);
		psDroid->sMove.Status = MOVENAVIGATE;
		psDroid->sMove.Position=0;
		psDroid->sMove.psFormation = NULL;
		return TRUE;
	}
	else if (vtolDroid(psDroid) OR (game.maxPlayers > 0 AND psDroid->
        droidType == DROID_TRANSPORTER))
	{
		fpathSetDirectRoute((BASE_OBJECT *)psDroid, (SDWORD)x, (SDWORD)y);
		retVal = FPR_OK;
	}
	else
	{
		retVal = fpathRoute((BASE_OBJECT *)psDroid, &(psDroid->sMove), (SDWORD)x,(SDWORD)y);
	}
#endif
	// ----
	// ----

	/* check formations */
	if ( retVal == FPR_OK )
	{
		DBP1(("unit(%d): base Speed %d, speed %d\n",
			 psDroid->id, psDroid->baseSpeed, MAKEINT(psDroid->sMove.speed)));

		// bit of a hack this - john
		// if astar doesn't have a complete route, it returns a route to the nearest clear tile.
		// the location of the clear tile is in DestinationX,DestinationY.
		// reset x,y to this position so the formation gets set up correctly
		x = psDroid->sMove.DestinationX;
		y = psDroid->sMove.DestinationY;

		psDroid->sMove.Status = MOVENAVIGATE;
		psDroid->sMove.Position=0;
		psDroid->sMove.fx = MAKEFRACT(psDroid->x);
		psDroid->sMove.fy = MAKEFRACT(psDroid->y);

		psDroid->sMove.fz = MAKEFRACT(psDroid->z);


		// reset the next route droid
		if (psDroid == psNextRouteDroid)
		{
			DBP6(("Waiting droid %d (player %d) got route\n", psDroid->id, psDroid->player));
			psNextRouteDroid = NULL;
		}

//		DBPRINTF(("moveDroidTo: form %p id %d\n",psDroid->sMove.psFormation, psDroid->id));

		// leave any old formation
		if (psDroid->sMove.psFormation)
		{
			formationLeave(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
			psDroid->sMove.psFormation = NULL;
		}

#if !FORMATIONS_DISABLE
		if (bFormation)
		{
			// join a formation if it exists at the destination
			if (formationFind(&psDroid->sMove.psFormation, (SDWORD)x,(SDWORD)y))
			{
				formationJoin(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
			}
			else
			{
				// align the formation with the last path of the route
				fmx2 = psDroid->sMove.asPath[psDroid->sMove.numPoints -1].x;
				fmy2 = psDroid->sMove.asPath[psDroid->sMove.numPoints -1].y;
				fmx2 = (fmx2 << TILE_SHIFT) + TILE_UNITS/2;
				fmy2 = (fmy2 << TILE_SHIFT) + TILE_UNITS/2;
				if (psDroid->sMove.numPoints == 1)
				{
					fmx1 = (SDWORD)psDroid->x;
					fmy1 = (SDWORD)psDroid->y;
				}
				else
				{
					fmx1 = psDroid->sMove.asPath[psDroid->sMove.numPoints -2].x;
					fmy1 = psDroid->sMove.asPath[psDroid->sMove.numPoints -2].y;
					fmx1 = (fmx1 << TILE_SHIFT) + TILE_UNITS/2;
					fmy1 = (fmy1 << TILE_SHIFT) + TILE_UNITS/2;
				}

				// no formation so create a new one
				if (formationNew(&psDroid->sMove.psFormation, FT_LINE, (SDWORD)x,(SDWORD)y,
						(SDWORD)calcDirection(fmx1,fmy1, fmx2,fmy2)))
				{
					formationJoin(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
				}
			}
		}
//		DBPRINTF(("moveDroidTo: form %p id %d end\n",psDroid->sMove.psFormation, psDroid->id));
#endif

	}
	else if (retVal == FPR_RESCHEDULE)
	{
		DBP4(("reroute: id %d\n", psDroid->id));

		// maxed out routing time this frame - do it next time
		psDroid->sMove.DestinationX = x;
		psDroid->sMove.DestinationY = y;

		if ((psDroid->sMove.Status != MOVEROUTE) &&
			(psDroid->sMove.Status != MOVEROUTESHUFFLE))
		{
			DBP6(("Unit %d (player %d) started waiting at %d\n", psDroid->id, psDroid->player, gameTime));

			psDroid->sMove.Status = MOVEROUTE;

			// note when the unit first tried to route
			psDroid->sMove.bumpTime = gameTime;

//			psDroid->sMove.bumpTime = gameTime + REROUTE_BASETIME + REROUTE_RNDTIME - (rand()%REROUTE_RNDTIME);
		}
	}
	else if (retVal == FPR_WAIT)
	{
		// reset the next route droid
		if (psDroid == psNextRouteDroid)
		{
			DBP6(("Waiting droid %d (player %d) got route\n", psDroid->id, psDroid->player));
			psNextRouteDroid = NULL;
		}

		// the route will be calculated over a number of frames
		psDroid->sMove.Status = MOVEWAITROUTE;
		psDroid->sMove.DestinationX = x;
		psDroid->sMove.DestinationY = y;
	}
	else // if (retVal == FPR_FAILED)
	{
		psDroid->sMove.Status = MOVEINACTIVE;
		actionDroid(psDroid, DACTION_SULK);
//		DBPRINTF(("mdt: FALSE\n");
		return(FALSE);
	}

	return TRUE;
}


// Shame about this but the find path code uses too much stack space
// so we can't safely run it in the dcache.
//
BOOL moveDroidToBase(DROID	*psDroid, UDWORD x, UDWORD y, BOOL bFormation)
{

	return _moveDroidToBase(psDroid,x,y,bFormation);
}


// move a droid to a location, joining a formation
BOOL moveDroidTo(DROID *psDroid, UDWORD x,UDWORD y)
{
	return moveDroidToBase(psDroid,x,y, TRUE);
}

// move a droid to a location, not joining a formation
BOOL moveDroidToNoFormation(DROID *psDroid, UDWORD x,UDWORD y)
{
	return moveDroidToBase(psDroid,x,y, FALSE);
}


// move a droid directly to a location (used by vtols only)
void moveDroidToDirect(DROID *psDroid, UDWORD x, UDWORD y)
{
	ASSERT( PTRVALID(psDroid, sizeof(DROID)) && vtolDroid(psDroid),
		"moveUnitToDirect: only valid for a vtol unit" );

	fpathSetDirectRoute((BASE_OBJECT *)psDroid, (SDWORD)x, (SDWORD)y);
	psDroid->sMove.Status = MOVENAVIGATE;
	psDroid->sMove.Position=0;

	// leave any old formation
	if (psDroid->sMove.psFormation)
	{
		formationLeave(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
		psDroid->sMove.psFormation = NULL;
	}
}


// Get a droid to turn towards a locaton
void moveTurnDroid(DROID *psDroid, UDWORD x, UDWORD y)
{
	SDWORD	moveDir = (SDWORD)calcDirection(psDroid->x,psDroid->y, x, y);

	if ( psDroid->direction != (UDWORD) moveDir )
	{
		psDroid->sMove.targetX = (SDWORD)x;
		psDroid->sMove.targetY = (SDWORD)y;
		psDroid->sMove.Status = MOVETURNTOTARGET;
	}
}

// get the difference in direction
SDWORD moveDirDiff(SDWORD start, SDWORD end)
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

// initialise a droid for a shuffle move
/*void moveShuffleInit(DROID *psDroid, SDWORD mx, SDWORD my)
{
	// set up the move state
	psDroid->sMove.Status = MOVESHUFFLE;
	psDroid->sMove.shuffleX = (SWORD)mx;
	psDroid->sMove.shuffleY = (SWORD)my;
	psDroid->sMove.srcX = (SDWORD)psDroid->x;
	psDroid->sMove.srcY = (SDWORD)psDroid->y;
	psDroid->sMove.targetX = (SDWORD)psDroid->x + mx;
	psDroid->sMove.targetY = (SDWORD)psDroid->y + my;
	psDroid->sMove.DestinationX = psDroid->sMove.targetX;
	psDroid->sMove.DestinationY = psDroid->sMove.targetY;
	psDroid->sMove.numPoints = 0;
	psDroid->sMove.Position = 0;
	psDroid->sMove.fx = MAKEFRACT(psDroid->x);
	psDroid->sMove.fy = MAKEFRACT(psDroid->y);
	psDroid->sMove.bumpTime = 0;
	moveCalcBoundary(psDroid);

	if (psDroid->sMove.psFormation != NULL)
	{
		formationLeave(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
		psDroid->sMove.psFormation = NULL;
	}
}*/

// Tell a droid to move out the way for a shuffle
void moveShuffleDroid(DROID *psDroid, UDWORD shuffleStart, SDWORD sx, SDWORD sy)
{
	FRACT	shuffleDir, droidDir;
	DROID	*psCurr;
	SDWORD	xdiff,ydiff, mx,my, shuffleMag, diff;
	BOOL	frontClear = TRUE, leftClear = TRUE, rightClear = TRUE;
	SDWORD	lvx,lvy, rvx,rvy, svx,svy;
	SDWORD	shuffleMove;
	SDWORD	tarX,tarY;

	shuffleDir = vectorToAngle(MKF(sx),MKF(sy));
	shuffleMag = (SDWORD)iSQRT(sx*sx + sy*sy);

	if (shuffleMag == 0)
	{
		return;
	}

	shuffleMove = SHUFFLE_MOVE;
/*	if (vtolDroid(psDroid))
	{
		shuffleMove /= 4;
	}*/

	// calculate the possible movement vectors
	lvx = -sy * shuffleMove / shuffleMag;
	lvy = sx * shuffleMove / shuffleMag;

	rvx = sy * shuffleMove / shuffleMag;
	rvy = -sx * shuffleMove / shuffleMag;

	svx = lvy; // sx * SHUFFLE_MOVE / shuffleMag;
	svy = rvx; // sy * SHUFFLE_MOVE / shuffleMag;

	// check for blocking tiles
	if ( fpathBlockingTile( ((SDWORD)psDroid->x + lvx) >> TILE_SHIFT,
						    ((SDWORD)psDroid->y + lvy) >> TILE_SHIFT ) )
	{
		leftClear = FALSE;
	}
	else if ( fpathBlockingTile( ((SDWORD)psDroid->x + rvx) >> TILE_SHIFT,
								 ((SDWORD)psDroid->y + rvy) >> TILE_SHIFT ) )
	{
		rightClear = FALSE;
	}
	else if ( fpathBlockingTile( ((SDWORD)psDroid->x + svx) >> TILE_SHIFT,
								 ((SDWORD)psDroid->y + svy) >> TILE_SHIFT ) )
	{
		frontClear = FALSE;
	}

	// find any droids that could block the shuffle
	for(psCurr=apsDroidLists[psDroid->player]; psCurr; psCurr=psCurr->psNext)
	{
		if (psCurr != psDroid)
		{
			xdiff = (SDWORD)psCurr->x - (SDWORD)psDroid->x;
			ydiff = (SDWORD)psCurr->y - (SDWORD)psDroid->y;
			if (xdiff*xdiff + ydiff*ydiff < SHUFFLE_DIST*SHUFFLE_DIST)
			{
				droidDir = vectorToAngle(MKF(xdiff),MKF(ydiff));
				diff = (SDWORD)moveDirDiff(MKI(shuffleDir), MKI(droidDir));
				if ((diff > -135) && (diff < -45))
				{
					leftClear = FALSE;
				}
				else if ((diff > 45) && (diff < 135))
				{
					rightClear = FALSE;
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
	tarX = (SDWORD)psDroid->x + mx;
	tarY = (SDWORD)psDroid->y + my;
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
//	psDroid->sMove.shuffleX = (SWORD)sx;
//	psDroid->sMove.shuffleY = (SWORD)sy;
	psDroid->sMove.shuffleStart = shuffleStart;
	psDroid->sMove.srcX = (SDWORD)psDroid->x;
	psDroid->sMove.srcY = (SDWORD)psDroid->y;
//	psDroid->sMove.targetX = (SDWORD)psDroid->x + mx;
//	psDroid->sMove.targetY = (SDWORD)psDroid->y + my;
	psDroid->sMove.targetX = tarX;
	psDroid->sMove.targetY = tarY;
	// setting the Destination could overwrite a MOVEROUTE's destination
	// it is not actually needed for a shuffle anyway
//	psDroid->sMove.DestinationX = psDroid->sMove.targetX;
//	psDroid->sMove.DestinationY = psDroid->sMove.targetY;
//	psDroid->sMove.bumpTime = 0;
	psDroid->sMove.numPoints = 0;
	psDroid->sMove.Position = 0;
	psDroid->sMove.fx = MAKEFRACT(psDroid->x);
	psDroid->sMove.fy = MAKEFRACT(psDroid->y);

	psDroid->sMove.fz = MAKEFRACT(psDroid->z);

	moveCalcBoundary(psDroid);

	if (psDroid->sMove.psFormation != NULL)
	{
		formationLeave(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
		psDroid->sMove.psFormation = NULL;
	}
}


/* Stop a droid */
void moveStopDroid(DROID *psDroid)
{
	PROPULSION_STATS	*psPropStats;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"moveStopUnit: Invalid unit pointer" );

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( PTRVALID(psPropStats, sizeof(PROPULSION_STATS)),
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

/*Stops a droid dead in its tracks - doesn't allow for any little skidding bits*/
void moveReallyStopDroid(DROID *psDroid)
{
    ASSERT( PTRVALID(psDroid, sizeof(DROID)),
        "moveReallyStopUnit: invalid unit pointer" );

    psDroid->sMove.Status = MOVEINACTIVE;
    psDroid->sMove.speed = MKF(0);
}



#define PITCH_LIMIT 150

/* Get pitch and roll from direction and tile data - NOT VERY PSX FRIENDLY */
void updateDroidOrientation(DROID *psDroid)
{
	SDWORD hx0, hx1, hy0, hy1, w;
	SDWORD newPitch, dPitch, pitchLimit;
	double dx, dy;
	double direction, pitch, roll;
	ASSERT( psDroid->x < (mapWidth << TILE_SHIFT),
		"mapHeight: x coordinate bigger than map width" );
	ASSERT( psDroid->y < (mapHeight<< TILE_SHIFT),
		"mapHeight: y coordinate bigger than map height" );


	//if(psDroid->droidType == DROID_PERSON OR psDroid->droidType == DROID_CYBORG OR
    if(psDroid->droidType == DROID_PERSON OR cyborgDroid(psDroid) OR
        psDroid->droidType == DROID_TRANSPORTER)
	{
		/* These guys always stand upright */
		return;
	}

	w = 20;
	hx0 = map_Height(psDroid->x + w, psDroid->y);
	hx1 = map_Height(psDroid->x - w, psDroid->y);
	hy0 = map_Height(psDroid->x, psDroid->y + w);
	hy1 = map_Height(psDroid->x, psDroid->y - w);

	//update height in case were in the bottom of a trough
	if (((hx0 +hx1)/2) > (SDWORD)psDroid->z)
	{
		psDroid->z = (UWORD)((hx0 +hx1)/2);
	}
	if (((hy0 +hy1)/2) > (SDWORD)psDroid->z)
	{
		psDroid->z = (UWORD)((hy0 +hy1)/2);
	}

	dx = (double)(hx0 - hx1) / ((double)w * 2.0);
	dy = (double)(hy0 - hy1) / ((double)w * 2.0);
	//dx is atan of angle of elevation along x axis
	//dx is atan of angle of elevation along y axis
	//body
	direction = (PI * psDroid->direction) / 180.0;
	pitch = sin(direction) * dx + cos(direction) * dy;
	pitch = atan(pitch);
	newPitch = (SDWORD)((pitch * 180) / PI);
	//limit the rate the front comes down to simulate momentum
	pitchLimit = PITCH_LIMIT * frameTime/GAME_TICKS_PER_SEC;
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
	psDroid->roll = (UWORD)((roll * 180) / PI);
	//turret
/*	direction = (PI * psDroid->turretDirection) / 180.0;
	pitch = sin(direction) * dx + cos(direction) * dy;
	pitch = atan(pitch);
	psDroid->turretPitch = (UDWORD)((pitch * 180) / PI);
	roll = cos(direction) * dx - sin(direction) * dy;
	roll = atan(roll);
	psDroid->turretRoll = (UDWORD)((roll * 180) / PI);
*/
	return;
}




/* Calculate the normalised vector between a droid and a point */
/*void moveCalcVector(DROID *psDroid, UDWORD x, UDWORD y, FRACT *pVX, FRACT *pVY)
{
	SDWORD	dx,dy, mag;
	FRACT	root;

	// Calc the basic vector
	dx = (SDWORD)x - (SDWORD)psDroid->x;
	dy = (SDWORD)y - (SDWORD)psDroid->y;

	// normalise
	mag = dx*dx + dy*dy;
	root = fSQRT(MAKEFRACT(mag));
	*pVX = FRACTdiv(dx, root);
	*pVY = FRACTdiv(dy, root);
}*/


/* Turn a vector into an angle - returns a FRACT (!) */
static FRACT vectorToAngle(FRACT vx, FRACT vy)
{
	FRACT	angle;	// Angle in degrees (0->360)

	angle = (float)(TRIG_DEGREES * atan2(-vy,vx) / PI / 2);
	angle += TRIG_DEGREES/4;
	if (angle < 0)
	{
		angle += TRIG_DEGREES;
	}

	if(angle>=360.0f)
	{
		angle -= 360.0f;
	}



	return angle;
}


/* Turn an angle into a vector */
static void angleToVector(SDWORD angle, FRACT *pX, FRACT *pY)
{
	*pX = trigSin(angle);
	*pY = trigCos(angle);
}


/* Calculate the change in direction given a target angle and turn rate */
static void moveCalcTurn(FRACT *pCurr, FRACT target, UDWORD rate)
{
	FRACT	diff, change;
#ifdef DEBUG							//Ugh.  If your gonna ONLY use this variable in "DEBUG", then
	SDWORD	path=0;				//make sure you wrap the function that uses it also!
#define SET_PATH(x) path=x
#else
#define SET_PATH(x)
#endif

	ASSERT( target < MAKEFRACT(360) && target >= MAKEFRACT(0),
			 "moveCalcTurn: target out of range" );

	ASSERT( (*pCurr) < MAKEFRACT(360) && (*pCurr) >= MAKEFRACT(0),
			 "moveCalcTurn: cur ang out of range" );


	// calculate the difference in the angles
	diff = target - *pCurr;

	// calculate the change in direction



//	change = FRACTmul( baseTurn, MAKEFRACT(rate) );

	change = (baseTurn* rate);		// constant rate so we can use a normal mult
	if(change < FRACTCONST(1,1))
	{
		change = FRACTCONST(1,1);	// HACK to solve issue of when framerate so high
	}								// that integer angle to turn per frame is less than 1



	DBP2(("change : %f\n", change));


	if ((diff >= 0 && diff < change) ||
		(diff < 0 && diff > -change))
	{
		// got to the target direction
		*pCurr = target;
		SET_PATH(0);
	}
	else if (diff > 0)
	{
		// Target dir is greater than current
		if (diff < MKF(TRIG_DEGREES/2))
		{
			// Simple case - just increase towards target
			*pCurr += change;
			SET_PATH(1);
		}
		else
		{
			// decrease to target, but could go over 0 boundary */
			*pCurr -= change;
			SET_PATH(2);
		}
	}
	else
	{
		if (diff > MKF(-(TRIG_DEGREES/2)))
		{
			// Simple case - just decrease towards target
			*pCurr -= change;
			SET_PATH(3);
		}
		else
		{
			// increase to target, but could go over 0 boundary
			*pCurr += change;
			SET_PATH(4);
		}
	}

	if (*pCurr < 0)
	{
		*pCurr += MKF(TRIG_DEGREES);
	}
	if (*pCurr >= MKF(TRIG_DEGREES))
	{
		*pCurr -= MKF(TRIG_DEGREES);
	}


	DBP2(("path %d: diff %f\n", path, diff));

#ifdef DEBUG			//Don't forget that if you don't define the variable, then we error out.
	ASSERT( MAKEINT(*pCurr) < 360 && MAKEINT(*pCurr) >= 0,
			 "moveCalcTurn: angle out of range - path %d\n"
			 "   NOTE - ANYONE WHO SEES THIS PLEASE REMEMBER: path %d", path, path );
#endif
}


/* Get the next target point from the route */
static BOOL moveNextTarget(DROID *psDroid)
{
	UDWORD	srcX,srcY, tarX, tarY;

	// See if there is anything left in the move list
//	if (psDroid->sMove.MovementList[psDroid->sMove.Position].XCoordinate == -1)
	if (psDroid->sMove.Position == psDroid->sMove.numPoints)
	{
		return FALSE;
	}

/*	tarX = (psDroid->sMove.MovementList[psDroid->sMove.Position].XCoordinate
				<< TILE_SHIFT) + TILE_UNITS/2;
	tarY = (psDroid->sMove.MovementList[psDroid->sMove.Position].YCoordinate
				<< TILE_SHIFT) + TILE_UNITS/2;*/
	tarX = (psDroid->sMove.asPath[psDroid->sMove.Position].x << TILE_SHIFT) + TILE_UNITS/2;
	tarY = (psDroid->sMove.asPath[psDroid->sMove.Position].y << TILE_SHIFT) + TILE_UNITS/2;
	if (psDroid->sMove.Position == 0)
	{
		psDroid->sMove.srcX = (SDWORD)psDroid->x;
		psDroid->sMove.srcY = (SDWORD)psDroid->y;
	}
	else
	{
		srcX = (psDroid->sMove.asPath[psDroid->sMove.Position -1].x << TILE_SHIFT) + TILE_UNITS/2;
		srcY = (psDroid->sMove.asPath[psDroid->sMove.Position -1].y << TILE_SHIFT) + TILE_UNITS/2;
		psDroid->sMove.srcX = srcX;
		psDroid->sMove.srcY = srcY ;
	}
	psDroid->sMove.targetX = tarX;
	psDroid->sMove.targetY = tarY;
	psDroid->sMove.Position++;


	return TRUE;
}

/* Look at the next target point from the route */
static void movePeekNextTarget(DROID *psDroid, SDWORD *pX, SDWORD *pY)
{
	SDWORD	xdiff, ydiff;

	// See if there is anything left in the move list
	if (psDroid->sMove.Position == psDroid->sMove.numPoints)
	{
		// No points left - fudge one to continue the same direction
		xdiff = psDroid->sMove.targetX - psDroid->sMove.srcX;
		ydiff = psDroid->sMove.targetY - psDroid->sMove.srcY;
		*pX = psDroid->sMove.targetX + xdiff;
		*pY = psDroid->sMove.targetY + ydiff;
	}
	else
	{
/*		*pX = (psDroid->sMove.MovementList[psDroid->sMove.Position].XCoordinate
					<< TILE_SHIFT) + TILE_UNITS/2;
		*pY = (psDroid->sMove.MovementList[psDroid->sMove.Position].YCoordinate
					<< TILE_SHIFT) + TILE_UNITS/2;*/
		*pX = (psDroid->sMove.asPath[psDroid->sMove.Position].x << TILE_SHIFT) + TILE_UNITS/2;
		*pY = (psDroid->sMove.asPath[psDroid->sMove.Position].y << TILE_SHIFT) + TILE_UNITS/2;
	}
}

// hack to get the box in the 2D display
//	QUAD	sBox;

/* Get a direction vector to avoid anything in front of a droid */
/*void moveObstacleVector(DROID *psDroid, FRACT *pX, FRACT *pY)
{
	FRACT	wsin,wcos, lsin,lcos, x,y;
	SDWORD	dir, diff, obstDir, target;
	UDWORD	distSqr, i;
	POINT	sPos;
	BASE_OBJECT		*psObst;

	// Calculate the obstacle box
	dir = (SDWORD)psDroid->direction;
	wsin = FRACTmul(MAKEFRACT(HITBOX_WIDTH/2), trigSin(dir));
	wcos = FRACTmul(MAKEFRACT(HITBOX_WIDTH/2), trigCos(dir));
	lsin = FRACTmul(MAKEFRACT(HITBOX_LENGTH), trigSin(dir));
	lcos = FRACTmul(MAKEFRACT(HITBOX_LENGTH), trigCos(dir));

	x = MAKEFRACT(psDroid->x);
	y = MAKEFRACT(psDroid->y);
	sBox.coords[0].x = MAKEINT(x - wcos);
	sBox.coords[0].y = MAKEINT(y + wsin);
	sBox.coords[1].x = MAKEINT(x + wcos);
	sBox.coords[1].y = MAKEINT(y - wsin);
	sBox.coords[3].x = MAKEINT(MAKEFRACT(sBox.coords[0].x) + lsin);
	sBox.coords[3].y = MAKEINT(MAKEFRACT(sBox.coords[0].y) + lcos);
	sBox.coords[2].x = MAKEINT(MAKEFRACT(sBox.coords[1].x) + lsin);
	sBox.coords[2].y = MAKEINT(MAKEFRACT(sBox.coords[1].y) + lcos);

	// Find the nearest obstacle in the box
	distSqr = UDWORD_MAX;
	psObst = NULL;
	for(i=0; i<numNaybors; i++)
	{
		sPos.x = (SDWORD)asDroidNaybors[i].psObj->x;
		sPos.y = (SDWORD)asDroidNaybors[i].psObj->y;
		if (inQuad(&sPos, &sBox) && asDroidNaybors[i].distSqr < distSqr)
		{
			psObst = asDroidNaybors[i].psObj;
			distSqr = asDroidNaybors[i].distSqr;
		}
	}

	if (psObst)
	{
//		moveCalcVector(psDroid, psObst->x, psObst->y, pX,pY);
		// got an obstacle - turn the droid away from it
		obstDir = (SDWORD)calcDirection(psDroid->x, psDroid->y, psObst->x,psObst->y);
		diff = directionDiff(obstDir, dir);
		ASSERT( diff > -90 && diff < 90, "big diff" );
		if (diff < 0)
		{
			target = dir + HITBOX_ANGLE + diff;
		}
		else
		{
			target = dir - HITBOX_ANGLE + diff;
		}

		angleToVector(MAKEFRACT(target), pX,pY);
	}
	else
	{
		// no obstacle - no control vector
		*pX = MAKEFRACT(0);
		*pY = MAKEFRACT(0);
	}
}*/

static	int mvPersRad = 20, mvCybRad = 30, mvSmRad = 40, mvMedRad = 50, mvLgRad = 60;

// Get the radius of a base object for collision
static SDWORD moveObjRadius(BASE_OBJECT *psObj)
{
	SDWORD	radius;
	BODY_STATS	*psBdyStats;

	switch (psObj->type)
	{
	case OBJ_DROID:
		if ( ((DROID *)psObj)->droidType == DROID_PERSON )
		{
			radius = mvPersRad;
		}
		//else if ( ((DROID *)psObj)->droidType == DROID_CYBORG )
        else if (cyborgDroid((DROID *)psObj))
		{
			radius = mvCybRad;
		}
		else
		{
			radius = psObj->sDisplay.imd->radius;
			psBdyStats = asBodyStats + ((DROID *)psObj)->asBits[COMP_BODY].nStat;
			switch (psBdyStats->size)
			{
			default:
			case SIZE_LIGHT:
				radius = mvSmRad;
				break;
			case SIZE_MEDIUM:
				radius = mvMedRad;
				break;
			case SIZE_HEAVY:
				radius = mvLgRad;
				break;
			case SIZE_SUPER_HEAVY:
				radius = 130;
				break;
			}
		}
		break;
	case OBJ_STRUCTURE:
//		radius = psObj->sDisplay.imd->visRadius;
		radius = psObj->sDisplay.imd->radius/2;
		break;
	case OBJ_FEATURE:
//		radius = psObj->sDisplay.imd->visRadius;
		radius = psObj->sDisplay.imd->radius/2;
		break;
	default:
		ASSERT( FALSE,"moveObjRadius: unknown object type" );
		radius = 0;
		break;
	}

	return radius;
}

// Find any object in the naybors list that the droid has hit
/*BOOL moveFindObstacle(DROID *psDroid, FRACT mx, FRACT my, BASE_OBJECT **ppsObst)
{
	SDWORD		i, droidR, rad, radSq;
	SDWORD		objR;
	FRACT		xdiff,ydiff, distSq;
	NAYBOR_INFO	*psInfo;
	SDWORD		distSq1;


	droidR = moveObjRadius((BASE_OBJECT *)psDroid);

	droidGetNaybors(psDroid);

	for(i=0; i<(SDWORD)numNaybors; i++)
	{
		psInfo = asDroidNaybors + i;
		if (psInfo->psObj->type == OBJ_DROID &&
			((DROID *)psInfo->psObj)->droidType != DROID_PERSON)
		{
			// ignore droids
			continue;
		}
		objR = moveObjRadius(psInfo->psObj);
		rad = droidR + objR;
		radSq = rad*rad;

		xdiff = MAKEFRACT(psDroid->x) + mx - MAKEFRACT(psInfo->psObj->x);
		ydiff = MAKEFRACT(psDroid->y) + my - MAKEFRACT(psInfo->psObj->y);
#ifndef PSX

		distSq = FRACTmul(xdiff,xdiff) + FRACTmul(ydiff,ydiff);
		distSq1 = MAKEINT(distSq);
#else
		x1 = MAKEINT (xdiff);
		y1 = MAKEINT (ydiff);
		distSq1 = (x1*x1)+(y1*y1);
#endif

		if (radSq > (distSq1))
		{
			if ((psDroid->droidType != DROID_PERSON) &&
				(psInfo->psObj->type == OBJ_DROID) &&
				((DROID *)psInfo->psObj)->droidType == DROID_PERSON &&
				psDroid->player != psInfo->psObj->player)
			{	// run over a bloke - kill him

				destroyDroid((DROID*)psInfo->psObj);
				continue;
			}
			else
			{
				*ppsObst = psInfo->psObj;
				// note the bump time and position if necessary
				if (psDroid->sMove.bumpTime == 0)
				{
					psDroid->sMove.bumpTime = gameTime;
					psDroid->sMove.bumpX = psDroid->x;
					psDroid->sMove.bumpY = psDroid->y;
					psDroid->sMove.bumpDir = (SDWORD)psDroid->direction;
				}
				return TRUE;
			}
		}
		else if (psInfo->distSqr > OBJ_MAXRADIUS*OBJ_MAXRADIUS)
		{
			// object is too far away to be hit
			break;
		}
	}

	return FALSE;
}*/



// see if a Droid has run over a person
void moveCheckSquished(DROID *psDroid, FRACT mx,FRACT my)
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

		xdiff = (SDWORD)psDroid->x + MAKEINT(mx) - (SDWORD)psInfo->psObj->x;
		ydiff = (SDWORD)psDroid->y + MAKEINT(my) - (SDWORD)psInfo->psObj->y;
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
BOOL moveBlocked(DROID *psDroid)
{
	SDWORD	xdiff,ydiff, diffSq;
	UDWORD	blockTime;

	if ((psDroid->sMove.bumpTime == 0) || (psDroid->sMove.bumpTime > gameTime) ||
		(psDroid->sMove.Status == MOVEROUTE) || (psDroid->sMove.Status == MOVEROUTESHUFFLE))
	{
		// no bump - can't be blocked
		return FALSE;
	}

	// See if the block can be cancelled
	if (dirDiff((SDWORD)psDroid->direction, psDroid->sMove.bumpDir) > BLOCK_DIR)
	{
		// Move on, clear the bump
		psDroid->sMove.bumpTime = 0;
		psDroid->sMove.lastBump = 0;
		return FALSE;
	}
	xdiff = (SDWORD)psDroid->x - (SDWORD)psDroid->sMove.bumpX;
	ydiff = (SDWORD)psDroid->y - (SDWORD)psDroid->sMove.bumpY;
	diffSq = xdiff*xdiff + ydiff*ydiff;
	if (diffSq > BLOCK_DIST*BLOCK_DIST)
	{
		// Move on, clear the bump
		psDroid->sMove.bumpTime = 0;
		psDroid->sMove.lastBump = 0;
		return FALSE;
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

		// if the unit cannot see the next way point - reroute it's got stuck
		if ( (

			  bMultiPlayer ||

			  (psDroid->player == selectedPlayer)) &&
			(psDroid->sMove.Position != psDroid->sMove.numPoints) &&
//			!fpathTileLOS((SDWORD)psDroid->x >> TILE_SHIFT, (SDWORD)psDroid->y >> TILE_SHIFT,
//						  psDroid->sMove.targetX >> TILE_SHIFT, psDroid->sMove.targetY >> TILE_SHIFT))
			!fpathTileLOS((SDWORD)psDroid->x >> TILE_SHIFT, (SDWORD)psDroid->y >> TILE_SHIFT,
						  psDroid->sMove.DestinationX >> TILE_SHIFT, psDroid->sMove.DestinationY >> TILE_SHIFT))
		{
			moveDroidTo(psDroid, psDroid->sMove.DestinationX, psDroid->sMove.DestinationY);
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}


// See if an object is on a droids target
BOOL moveObjOnTarget(DROID *psDroid, BASE_OBJECT *psObst)
{
	SDWORD	xdiff, ydiff;
	SDWORD	radius;

/*	xdiff = (SDWORD)psObst->x - (psDroid->sMove.DestinationX << TILE_SHIFT) -
				TILE_UNITS/2;
	ydiff = (SDWORD)psObst->y - (psDroid->sMove.DestinationY << TILE_SHIFT) -
				TILE_UNITS/2;*/
	xdiff = (SDWORD)psObst->x - psDroid->sMove.DestinationX;
	ydiff = (SDWORD)psObst->y - psDroid->sMove.DestinationY;
	radius = moveObjRadius(psObst)*2;

	if (xdiff*xdiff + ydiff*ydiff < radius*radius)
	{
		return TRUE;
	}

	return FALSE;

	/*	SDWORD	absX, absY;
	SDWORD	tarX, tarY, tarMag;
	SDWORD	obstX, obstY, obstMag;
	FRACT	distRatio, dot;

	// Calculate the vectors to the target and obstruction
	tarX = (SDWORD)psDroid->x - psDroid->sMove.targetX;
	tarY = (SDWORD)psDroid->y - psDroid->sMove.targetY;
	absX = labs(tarX); absY = labs(tarY);
	tarMag = absX > absY ? absX + absY/2 : absY + absX/2;
	obstX = (SDWORD)psDroid->x - (SDWORD)psObst->x;
	obstY = (SDWORD)psDroid->y - (SDWORD)psObst->y;
	absX = labs(obstX); absY = labs(obstY);
	obstMag = absX > absY ? absX + absY/2 : absY + absX/2;

	// If the difference in distance is too big, obstruction cannot be
	// on target
	distRatio = FRACTdiv(MAKEFRACT(obstMag), MAKEFRACT(tarMag));
	if (distRatio < BLOCK_MINRATIO || distRatio > BLOCK_MAXRATIO)
	{
		return FALSE;
	}

	// Do the dot product to see if the vectors are the same direction
	dot = FRACTmul(FRACTdiv(MAKEFRACT(tarX), MAKEFRACT(tarMag)),
				   FRACTdiv(MAKEFRACT(obstX), MAKEFRACT(obstMag))) +
		  FRACTmul(FRACTdiv(MAKEFRACT(tarY), MAKEFRACT(tarMag)),
				   FRACTdiv(MAKEFRACT(obstY), MAKEFRACT(obstMag)));
	if (dot > BLOCK_DOTVAL)
	{
		return TRUE;
	}

	return FALSE;*/
}


// Calculate the actual movement to slide around
void moveCalcSlideVector(DROID *psDroid,SDWORD objX, SDWORD objY, FRACT *pMx, FRACT *pMy)
{
	SDWORD		obstX, obstY;
	SDWORD		absX, absY;
	SDWORD		dirX, dirY, dirMag;
	FRACT		mx, my, unitX,unitY;
	FRACT		dotRes;

	mx = *pMx;
	my = *pMy;

	// Calculate the vector to the obstruction
	obstX = (SDWORD)psDroid->x - objX;
	obstY = (SDWORD)psDroid->y - objY;

	// if the target dir is the same, don't need to slide
	if (obstX*mx + obstY*my >= 0)
	{
		return;
	}

	// Choose the tangent vector to this on the same side as the target
//	tarX = psDriod->sMove.targetX - (SDWORD)psDroid->x;
//	tarY = psDriod->sMove.targetY - (SDWORD)psDroid->y;
//	dotRes = FRACTmul(MAKEFRACT(obstY),*pMx);
//	dotRes -= FRACTmul(MAKEFRACT(obstX),*pMy);
//	dotRes = obstY * mx - obstX * my;
	dotRes = FRACTmul(MAKEFRACT(obstY),mx) - FRACTmul(MAKEFRACT(obstX),my);
	if (dotRes >= 0)
	{
		dirX = obstY;
		dirY = -obstX;
	}
	else
	{
		dirX = -obstY;
		dirY = obstX;
		dotRes = FRACTmul(MAKEFRACT(dirX),*pMx) + FRACTmul(MAKEFRACT(dirY),*pMy);
	}
	absX = labs(dirX); absY = labs(dirY);
	dirMag = absX > absY ? absX + absY/2 : absY + absX/2;

	// Calculate the component of the movement in the direction of the tangent vector
	unitX = FRACTdiv(MAKEFRACT(dirX), MAKEFRACT(dirMag));
	unitY = FRACTdiv(MAKEFRACT(dirY), MAKEFRACT(dirMag));
	dotRes = FRACTdiv(dotRes, MAKEFRACT(dirMag));
	*pMx = FRACTmul(unitX, dotRes);
	*pMy = FRACTmul(unitY, dotRes);
}


// see if a droid has run into a blocking tile
void moveCalcBlockingSlide(DROID *psDroid, FRACT *pmx, FRACT *pmy, SDWORD tarDir, SDWORD *pSlideDir)
{
	FRACT	mx = *pmx,my = *pmy, nx,ny;
	SDWORD	tx,ty, ntx,nty;		// current tile x,y and new tile x,y
	SDWORD	blkCX,blkCY;
	SDWORD	horizX,horizY, vertX,vertY;
	SDWORD	intx,inty;
	SDWORD	jumpx,jumpy, bJumped=FALSE;
#ifdef DEBUG
	BOOL	slide =FALSE;
#define NOTE_SLIDE	slide=TRUE
	SDWORD	state = 0;
#define NOTE_STATE(x) state = x
#else
#define NOTE_STATE(x)
#define NOTE_SLIDE
#define NOTE_STATE(x)
#endif
//	FRACT	mag, rad, temp;
	FRACT	radx,rady;
	BOOL	blocked;
	SDWORD	slideDir;

	blocked = FALSE;
	radx = MKF(0);
	rady = MKF(0);

	// calculate the new coords and see if they are on a different tile
	tx = MAKEINT(psDroid->sMove.fx) >> TILE_SHIFT;
	ty = MAKEINT(psDroid->sMove.fy) >> TILE_SHIFT;
	nx = psDroid->sMove.fx + mx;
	ny = psDroid->sMove.fy + my;
	ntx = MAKEINT(nx) >> TILE_SHIFT;
	nty = MAKEINT(ny) >> TILE_SHIFT;

	// is the new tile blocking?
	if (fpathBlockingTile(ntx,nty))
	{
		blocked = TRUE;
	}

	// now test ahead of the droid
/*	if (!blocked)
	{
		rad = MKF(moveObjRadius((BASE_OBJECT *)psDroid));
		mag = fSQRT(mx*mx + my*my);

		if (mag==0)
		{
			*pmx = MKF(0);
			*pmy = MKF(0);
			return;
		}

		radx = (rad * mx) / mag;
		rady = (rad * my) / mag;

		nx = psDroid->sMove.fx + radx;
		ny = psDroid->sMove.fy + rady;
		tx = MAKEINT(nx) >> TILE_SHIFT;
		ty = MAKEINT(ny) >> TILE_SHIFT;
		nx += mx;
		ny += my;
		ntx = MAKEINT(nx) >> TILE_SHIFT;
		nty = MAKEINT(ny) >> TILE_SHIFT;

		// is the new tile blocking?
		if (fpathBlockingTile(ntx,nty))
		{
			blocked = TRUE;
		}
	}

	// now test one side of the droid
	if (!blocked)
	{
		nx = psDroid->sMove.fx - rady;
		ny = psDroid->sMove.fy + radx;
		tx = MAKEINT(nx) >> TILE_SHIFT;
		ty = MAKEINT(ny) >> TILE_SHIFT;
		nx += mx;
		ny += my;
		ntx = MAKEINT(nx) >> TILE_SHIFT;
		nty = MAKEINT(ny) >> TILE_SHIFT;

		// is the new tile blocking?
		if (fpathBlockingTile(ntx,nty))
		{
			blocked = TRUE;
			temp = radx;
			radx = -rady;
			rady = radx;
		}
	}

	// now test the other side of the droid
	if (!blocked)
	{
		nx = psDroid->sMove.fx + rady;
		ny = psDroid->sMove.fy - radx;
		tx = MAKEINT(nx) >> TILE_SHIFT;
		ty = MAKEINT(ny) >> TILE_SHIFT;
		nx += mx;
		ny += my;
		ntx = MAKEINT(nx) >> TILE_SHIFT;
		nty = MAKEINT(ny) >> TILE_SHIFT;

		// is the new tile blocking?
		if (fpathBlockingTile(ntx,nty))
		{
			blocked = TRUE;
			temp = radx;
			radx = rady;
			rady = -radx;
		}
	}*/

	blkCX = (ntx << TILE_SHIFT) + TILE_UNITS/2;
	blkCY = (nty << TILE_SHIFT) + TILE_UNITS/2;

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
		psDroid->sMove.bumpX = psDroid->x;
		psDroid->sMove.bumpY = psDroid->y;
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

		if (fpathBlockingTile(horizX,horizY) && fpathBlockingTile(vertX,vertY))
		{
			// in a corner - choose an arbitrary slide
			if (ONEINTWO)
			{
				*pmx = MAKEFRACT(0);
				*pmy = -*pmy;
				NOTE_STATE(1);
			}
			else
			{
				*pmx = -*pmx;
				*pmy = MAKEFRACT(0);
				NOTE_STATE(2);
			}
		}
		else if (fpathBlockingTile(horizX,horizY))
		{
			*pmy = MAKEFRACT(0);
			NOTE_STATE(3);
		}
		else if (fpathBlockingTile(vertX,vertY))
		{
			*pmx = MAKEFRACT(0);
			NOTE_STATE(4);
		}
		else
		{
			moveCalcSlideVector(psDroid, blkCX,blkCY, pmx,pmy);
			NOTE_SLIDE;
			NOTE_STATE(5);
		}

//		*pmx = MAKEFRACT(0);
//		*pmy = MAKEFRACT(0);
	}
	else if (tx != ntx)
	{
		// moved horizontally - see which half of the tile were in
		if ((psDroid->y & TILE_MASK) > TILE_UNITS/2)
		{
			// top half
			if (fpathBlockingTile(ntx,nty+1))
			{
				*pmx = MAKEFRACT(0);
				NOTE_STATE(6);
			}
			else
			{
				moveCalcSlideVector(psDroid, blkCX,blkCY, pmx,pmy);
				NOTE_SLIDE;
				NOTE_STATE(7);
			}
		}
		else
		{
			// bottom half
			if (fpathBlockingTile(ntx,nty-1))
			{
				*pmx = MAKEFRACT(0);
				NOTE_STATE(8);
			}
			else
			{
				moveCalcSlideVector(psDroid, blkCX,blkCY, pmx,pmy);
				NOTE_SLIDE;
				NOTE_STATE(9);
			}
		}
	}
	else if (ty != nty)
	{
		// moved vertically
		if ((psDroid->x & TILE_MASK) > TILE_UNITS/2)
		{
			// top half
			if (fpathBlockingTile(ntx+1,nty))
			{
				*pmy = MAKEFRACT(0);
				NOTE_STATE(10);
			}
			else
			{
				moveCalcSlideVector(psDroid, blkCX,blkCY, pmx,pmy);
				NOTE_SLIDE;
				NOTE_STATE(11);
			}
		}
		else
		{
			// bottom half
			if (fpathBlockingTile(ntx-1,nty))
			{
				*pmy = MAKEFRACT(0);
				NOTE_STATE(12);
			}
			else
			{
				moveCalcSlideVector(psDroid, blkCX,blkCY, pmx,pmy);
				NOTE_SLIDE;
				NOTE_STATE(13);
			}
		}
	}
	else // if (tx == ntx && ty == nty)
	{
		// on a blocking tile - see if we need to jump off

		intx = MAKEINT(psDroid->sMove.fx) & TILE_MASK;
		inty = MAKEINT(psDroid->sMove.fy) & TILE_MASK;
		jumpx = (SDWORD)psDroid->x;
		jumpy = (SDWORD)psDroid->y;
		bJumped = FALSE;

/*		jumpx = MAKEINT(nx - mx);
		jumpy = MAKEINT(ny - my);
		intx = jumpx & TILE_MASK;
		inty = jumpy & TILE_MASK;
		bJumped = FALSE;*/

		if (intx < TILE_UNITS/2)
		{
			if (inty < TILE_UNITS/2)
			{
				// top left
				if ((mx < 0) && fpathBlockingTile(tx-1, ty))
				{
					bJumped = TRUE;
					jumpy = (jumpy & ~TILE_MASK) -1;
				}
				if ((my < 0) && fpathBlockingTile(tx, ty-1))
				{
					bJumped = TRUE;
					jumpx = (jumpx & ~TILE_MASK) -1;
				}
				NOTE_STATE(14);
			}
			else
			{
				// bottom left
				if ((mx < 0) && fpathBlockingTile(tx-1, ty))
				{
					bJumped = TRUE;
					jumpy = (jumpy & ~TILE_MASK) + TILE_UNITS;
				}
				if ((my >= 0) && fpathBlockingTile(tx, ty+1))
				{
					bJumped = TRUE;
					jumpx = (jumpx & ~TILE_MASK) -1;
				}
				NOTE_STATE(15);
			}
		}
		else
		{
			if (inty < TILE_UNITS/2)
			{
				// top right
				if ((mx >= 0) && fpathBlockingTile(tx+1, ty))
				{
					bJumped = TRUE;
					jumpy = (jumpy & ~TILE_MASK) -1;
				}
				if ((my < 0) && fpathBlockingTile(tx, ty-1))
				{
					bJumped = TRUE;
					jumpx = (jumpx & ~TILE_MASK) + TILE_UNITS;
				}
				NOTE_STATE(16);
			}
			else
			{
				// bottom right
				if ((mx >= 0) && fpathBlockingTile(tx+1, ty))
				{
					bJumped = TRUE;
					jumpy = (jumpy & ~TILE_MASK) + TILE_UNITS;
				}
				if ((my >= 0) && fpathBlockingTile(tx, ty+1))
				{
					bJumped = TRUE;
					jumpx = (jumpx & ~TILE_MASK) + TILE_UNITS;
				}
				NOTE_STATE(17);
			}
		}

		if (bJumped)
		{
			psDroid->x = (SWORD)(jumpx - MKI(radx));
			psDroid->y = (SWORD)(jumpy - MKI(rady));
			psDroid->sMove.fx = MAKEFRACT(jumpx);
			psDroid->sMove.fy = MAKEFRACT(jumpy);
			*pmx = MAKEFRACT(0);
			*pmy = MAKEFRACT(0);
		}
		else
		{
			moveCalcSlideVector(psDroid, blkCX,blkCY, pmx,pmy);
			NOTE_SLIDE;
		}
	}


	slideDir = MKI(vectorToAngle(*pmx,*pmy));
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

#ifdef DEBUG
	nx = psDroid->sMove.fx + *pmx;
	ny = psDroid->sMove.fy + *pmy;

//	ASSERT( slide || (!fpathBlockingTile(MAKEINT(nx)>>TILE_SHIFT, MAKEINT(ny)>>TILE_SHIFT)),
//		"moveCalcBlockingSlide: slid onto a blocking tile" );
#endif

}




// see if a droid has run into another droid
// Only consider stationery droids
void moveCalcDroidSlide(DROID *psDroid, FRACT *pmx, FRACT *pmy)
{
	SDWORD		i, droidR, rad, radSq;
	SDWORD		objR;
	SDWORD		xdiff,ydiff, distSq;
	NAYBOR_INFO	*psInfo;
	BASE_OBJECT	*psObst;
	BOOL		bLegs;

	bLegs = FALSE;
	if (psDroid->droidType == DROID_PERSON ||
		//psDroid->droidType == DROID_CYBORG)
        cyborgDroid(psDroid))
	{
		bLegs = TRUE;
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

			if (bLegs &&
				((DROID *)psInfo->psObj)->droidType != DROID_PERSON &&
				//((DROID *)psInfo->psObj)->droidType != DROID_CYBORG)
                !cyborgDroid((DROID *)psInfo->psObj))
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

		xdiff = MAKEINT(psDroid->sMove.fx + *pmx) - (SDWORD)psInfo->psObj->x;
		ydiff = MAKEINT(psDroid->sMove.fy + *pmy) - (SDWORD)psInfo->psObj->y;
		distSq = xdiff*xdiff + ydiff*ydiff;
		if (Fmul(MKF(xdiff),(*pmx)) + Fmul(MKF(ydiff),(*pmy)) >= 0)
		{
			// object behind
			continue;
		}

		if (radSq > distSq)
		{
			if (psObst != NULL || !aiCheckAlliances(psInfo->psObj->player, psDroid->player))
			{
				// hit more than one droid - stop
				*pmx = (FRACT)0;
				*pmy = (FRACT)0;
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
					psDroid->sMove.bumpX = psDroid->x;
					psDroid->sMove.bumpY = psDroid->y;
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
							psDroid->sMove.targetX - (SDWORD)psDroid->x,
							psDroid->sMove.targetY - (SDWORD)psDroid->y);
					}
					else
					{
						moveShuffleDroid( (DROID *)psObst, gameTime,
							psDroid->sMove.targetX - (SDWORD)psDroid->x,
							psDroid->sMove.targetY - (SDWORD)psDroid->y);
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
		moveCalcSlideVector(psDroid, (SDWORD)psObst->x,(SDWORD)psObst->y, pmx,pmy);
	}
}




// Get the movement vector for an object
/*void moveGetObstMove(BASE_OBJECT *psObj, FRACT *pX, FRACT *pY)
{
	switch (psObj->type)
	{
	case OBJ_DROID:
		if (((DROID *)psObj)->sMove.Status == MOVEPOINTTOPOINT)
		{
			*pX=((DROID *)psObj)->sMove.dx;
			*pY=((DROID *)psObj)->sMove.dy;
		}
		else
		{
			*pX=(FRACT)0;
			*pY=(FRACT)0;
		}
		break;
	case OBJ_STRUCTURE:
		*pX=(FRACT)0;
		*pY=(FRACT)0;
		break;
	case OBJ_FEATURE:
		*pX=(FRACT)0;
		*pY=(FRACT)0;
		break;
	default:
		ASSERT( FALSE,"moveGetObstMove: unknown object type" );
		*pX = (FRACT)0;
		*pY = (FRACT)0;
		break;
	}
}*/


// Get a charged particle vector from an object
/*BOOL moveGetObstVector(DROID *psDroid, BASE_OBJECT *psObj, FRACT *pX, FRACT *pY)
{
	SDWORD		xdiff,ydiff, absX,absY, mag;
	FRACT		normX,normY;
	FRACT		mx,my;//, omx,omy;
	FRACT		avoidX, avoidY, resMag;
//	SDWORD		obstRad, obstBound, obstRange;

	xdiff = (SDWORD)psDroid->x - (SDWORD)psObj->x;
	ydiff = (SDWORD)psDroid->y - (SDWORD)psObj->y;
	mx = psDroid->sMove.dx;
	my = psDroid->sMove.dy;

	if (xdiff* (*pX) + ydiff* (*pY) >= 0 ||
		moveObjOnTarget(psDroid,psObj))
	{
		return FALSE;
	}
//	moveGetObstMove(psObj, &omx,&omy);
//	obstRad = moveObjRadius(psObj);
//	obstBound = 3 * obstRad;
//	obstRange = obstBound - obstRad;

	// Calculate the normalised vector from the obstacle to the droid
	absX = labs(xdiff);
	absY = labs(ydiff);
	mag = absX > absY ? absX + absY/2 : absY + absX/2;
	normX = FRACTdiv(MAKEFRACT(xdiff), MAKEFRACT(mag));
	normY = FRACTdiv(MAKEFRACT(ydiff), MAKEFRACT(mag));
	DBP3(("mag %d\n", mag));

	// Create the avoid vector
	if (FRACTmul(*pX, normY) + FRACTmul(*pY,-normX) < 0)
	{
		DBP3(("First perp\n"));
		avoidX = -normY;
		avoidY = normX;
	}
	else
	{
		DBP3(("Second perp\n"));
		avoidX = normY;
		avoidY = -normX;
	}


//	if (mag > obstBound)
//	{
//		DBP3(("mag > obstBound\n"));
		*pX = *pX * (float)mag / AVOID_DIST +
			  avoidX * (AVOID_DIST - (float)mag)/AVOID_DIST;
		*pY = *pY * (float)mag / AVOID_DIST +
			  avoidY * (AVOID_DIST - (float)mag)/AVOID_DIST;

		resMag = FRACTmul(*pX, *pX) + FRACTmul(*pY,*pY);
		resMag = fSQRT(resMag);
		*pX = FRACTdiv((*pX),resMag);
		*pY = FRACTdiv((*pY),resMag);
//	}
//	else
//	{
//		DBP3(("mag < obstBound\n"));
//		*pX = *pX * (float)mag / AVOID_DIST +
//			  normX * (float)(obstBound - mag)/obstRange +
//			  avoidX * (mag - obstRad)/obstBound;
//		*pY = *pY * (float)mag / AVOID_DIST +
//			  normY * (float)(obstBound - mag)/obstRange +
//			  avoidY * (mag - obstRad)/obstBound;
//	}

	return TRUE;
}*/


// Get the distance to a tile if it is on the map
BOOL moveGetTileObst(SDWORD cx,SDWORD cy, SDWORD ox,SDWORD oy, SDWORD *pDist)
{
	SDWORD	absx, absy;

	if ((cx + ox < 0) || (cx + ox >= (SDWORD)mapWidth) ||
		(cy + oy < 0) || (cy + oy >= (SDWORD)mapHeight))
	{
		return FALSE;
	}

//	if (TERRAIN_TYPE(mapTile(cx+ox, cy+oy)) != TER_CLIFFFACE)
	if (!fpathBlockingTile(cx+ox, cy+oy))
	{
		return FALSE;
	}

	absx = labs(ox) * TILE_UNITS;
	absy = labs(oy) * TILE_UNITS;
	*pDist = absx > absy ? absx + absy/2 : absy + absx/2;

	return TRUE;
}


// Get a charged particle vector from all nearby objects
void moveGetObstVector2(DROID *psDroid, FRACT *pX, FRACT *pY)
{
	SDWORD		xdiff,ydiff, absX,absY, mag, minMag;
	FRACT		ox,oy, normX,normY, ratio;
	FRACT		avoidX, avoidY, resMag;
	SDWORD		i, size;
	BASE_OBJECT	*psObj;
	SDWORD		mapX,mapY;
	FRACT DivTop,DivBot;


	normX=normY = (FRACT)0;
	size = 0;
	minMag = AVOID_DIST;

	droidGetNaybors(psDroid);

	for(i=0; i<(SDWORD)numNaybors; i++)
	{
		if (asDroidNaybors[i].distSqr > AVOID_DIST*AVOID_DIST)
		{
			// object too far away to worry about
			continue;
		}
		psObj = asDroidNaybors[i].psObj;
		if (moveObjOnTarget(psDroid,psObj))
		{
			continue;
		}
		xdiff = (SDWORD)psDroid->x - (SDWORD)psObj->x;
		ydiff = (SDWORD)psDroid->y - (SDWORD)psObj->y;
		if (Fmul(MKF(xdiff),(*pX)) + Fmul(MKF(ydiff),(*pY)) >= 0)
		{
			// object behind
			continue;
		}
		// Calculate the vector from the obstacle to the droid
		absX = labs(xdiff);
		absY = labs(ydiff);
		mag = absX > absY ? absX + absY/2 : absY + absX/2;
		ox = FRACTdiv(MAKEFRACT(xdiff), MAKEFRACT(mag));
		oy = FRACTdiv(MAKEFRACT(ydiff), MAKEFRACT(mag));

		// Add the obstacle vector to the total, biased for distance
//		ratio = FRACTdiv(MAKEFRACT(AVOID_DIST*AVOID_DIST - mag*mag), MAKEFRACT(AVOID_DIST*AVOID_DIST));

		if (mag >= AVOID_DIST )	// dont consider if it is EQUAL !!
		{
			continue;	// not worth considering if it is right on the border ... leads to divide by zero errors
		}

		DivTop=MAKEFRACT((AVOID_DIST*AVOID_DIST)-(mag*mag));
		DivBot=MAKEFRACT(AVOID_DIST*AVOID_DIST);


		ratio=FRACTdiv(DivTop,DivBot);


		normX += FRACTmul(ox, ratio);
		normY += FRACTmul(oy, ratio);
		size += 1;//AVOID_DIST - mag;
		if (minMag > mag)
		{
			minMag = mag;
		}
	}

	// now scan the tiles for TER_CLIFFFACE
	mapX = (SDWORD)(psDroid->x >> TILE_SHIFT);
	mapY = (SDWORD)(psDroid->y >> TILE_SHIFT);
	for(ydiff=-2; ydiff<=2; ydiff++)
	{
		for(xdiff=-2; xdiff<=2; xdiff++)
		{
			if (Fmul(MKF(xdiff),(*pX)) + Fmul(MKF(ydiff),(*pY)) <= 0)
			{
				// object behind
				continue;
			}
			if (moveGetTileObst(mapX,mapY, xdiff,ydiff, &mag))
			{

				if (mag >= AVOID_DIST )	// dont consider if it is EQUAL !!
				{
					continue;	// not worth considering if it is right on the border ... leads to divide by zero errors
				}

  				ox = FRACTdiv(MAKEFRACT(-xdiff*TILE_UNITS), MAKEFRACT(mag));
				oy = FRACTdiv(MAKEFRACT(-ydiff*TILE_UNITS), MAKEFRACT(mag));

				// Add the obstacle vector to the total, biased for distance
//old				ratio = FRACTdiv(MAKEFRACT(AVOID_DIST*AVOID_DIST - mag*mag), MAKEFRACT(AVOID_DIST*AVOID_DIST));
				DivTop=MAKEFRACT((AVOID_DIST*AVOID_DIST)-(mag*mag));
				DivBot=MAKEFRACT(AVOID_DIST*AVOID_DIST);



				ratio=FRACTdiv(DivTop,DivBot);

				normX += FRACTmul(ox, ratio);
				normY += FRACTmul(oy, ratio);
				size += 1;//AVOID_DIST - mag;
				if (minMag > mag)
				{
					minMag = mag;
				}
			}
		}
	}

	if (size != 0)
	{

/*		normX = FRACTdiv(normX, MAKEFRACT(size));
		normY = FRACTdiv(normY, MAKEFRACT(size));*/
		resMag = fSQRT(FRACTmul(normX,normX) + FRACTmul(normY,normY));



		{


			normX = FRACTdiv(normX, resMag);
			normY = FRACTdiv(normY, resMag);
			mag = minMag;

			// Create the avoid vector
			if (FRACTmul(*pX, normY) + FRACTmul(*pY,-normX) < 0)
			{
				DBP3(("First perp\n"));
				avoidX = -normY;
				avoidY = normX;
			}
			else
			{
				DBP3(("Second perp\n"));
				avoidX = normY;
				avoidY = -normX;
			}



			*pX = *pX * (float)mag / AVOID_DIST +
				  avoidX * (AVOID_DIST - (float)mag)/AVOID_DIST;

			*pY = *pY * (float)mag / AVOID_DIST +
				  avoidY * (AVOID_DIST - (float)mag)/AVOID_DIST;



			resMag = FRACTmul(*pX, *pX) + FRACTmul(*pY,*pY);
			resMag = fSQRT(resMag);
			*pX = FRACTdiv((*pX),resMag);
			*pY = FRACTdiv((*pY),resMag);

		}

	}
}


// get an obstacle avoidance vector
void moveGetObstVector3(DROID *psDroid, FRACT *pX, FRACT *pY)
{
	SDWORD		i,xdiff,ydiff;
	BASE_OBJECT	*psObj;
	SDWORD		numObst, dirTot, distTot;
	FRACT		ox,oy, ratio;
	FRACT		avoidX,avoidY;
	SDWORD		mapX,mapY, tx,ty, td;

	numObst = 0;
	dirTot = 0;
	distTot = 0;

	droidGetNaybors(psDroid);

	// scan the neighbours for obstacles
	for(i=0; i<(SDWORD)numNaybors; i++)
	{
		if (asDroidNaybors[i].distSqr >= AVOID_DIST*AVOID_DIST)
		{
			// object too far away to worry about
			continue;
		}
		psObj = asDroidNaybors[i].psObj;
		if (moveObjOnTarget(psDroid,psObj))
		{
			continue;
		}
		xdiff = (SDWORD)psDroid->x - (SDWORD)psObj->x;
		ydiff = (SDWORD)psDroid->y - (SDWORD)psObj->y;
		if (Fmul(MKF(xdiff),(*pX)) + Fmul(MKF(ydiff),(*pY)) >= 0)
		{
			// object behind
			continue;
		}

		dirTot += calcDirection(psObj->x,psObj->y, psDroid->x,psDroid->y);
		distTot += AVOID_DIST*AVOID_DIST - asDroidNaybors[i].distSqr;
		numObst += 1;
	}

	// now scan for blocking tiles
	mapX = (SDWORD)(psDroid->x >> TILE_SHIFT);
	mapY = (SDWORD)(psDroid->y >> TILE_SHIFT);
	for(ydiff=-2; ydiff<=2; ydiff++)
	{
		for(xdiff=-2; xdiff<=2; xdiff++)
		{
			if (Fmul(MKF(xdiff),(*pX)) + Fmul(MKF(ydiff),(*pY)) <= 0)
			{
				// object behind
				continue;
			}
			if (fpathBlockingTile(mapX+xdiff, mapY+ydiff))
			{
				tx = xdiff << TILE_SHIFT;
				ty = ydiff << TILE_SHIFT;
				td = tx*tx + ty*ty;
				if (td < AVOID_DIST*AVOID_DIST)
				{
					dirTot += calcDirection(psDroid->x+tx,psDroid->y+ty,
									psDroid->x,psDroid->y);
					distTot += AVOID_DIST*AVOID_DIST - td;
					numObst += 1;
				}
			}
		}
	}

	if (numObst > 0)
	{
		dirTot /= numObst;
		distTot /= numObst;

		// Create the avoid vector
//		dirTot = (SDWORD)adjustDirection(dirTot, 180);
		angleToVector(dirTot, &ox, &oy);
		if (FRACTmul((*pX), oy) + FRACTmul((*pY),-ox) < 0)
		{
			DBP3(("First perp\n"));
			avoidX = -oy;
			avoidY = ox;
		}
		else
		{
			DBP3(("Second perp\n"));
			avoidX = oy;
			avoidY = -ox;
		}

		// combine the avoid vector and the target vector
		ratio = Fdiv(MKF(distTot), MKF(AVOID_DIST*AVOID_DIST));
		*pX = Fmul((*pX), (1 - ratio)) + Fmul(avoidX, ratio);
		*pY = Fmul((*pY), (1 - ratio)) + Fmul(avoidY, ratio);
	}
}

/* arrow colours */
#define	YELLOWARROW		117
#define	GREENARROW		253
#define	WHITEARROW		255
#define REDARROW		179

// get an obstacle avoidance vector
void moveGetObstVector4(DROID *psDroid, FRACT *pX, FRACT *pY)
{
	SDWORD				i,xdiff,ydiff, absx,absy, dist;
	BASE_OBJECT			*psObj;
	SDWORD				numObst, distTot;
	FRACT				dirX,dirY;
	FRACT				omag, ox,oy, ratio;
	FRACT				avoidX,avoidY;
	SDWORD				mapX,mapY, tx,ty, td;
	PROPULSION_STATS	*psPropStats;

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( PTRVALID(psPropStats, sizeof(PROPULSION_STATS)),
			"moveUpdateUnit: invalid propulsion stats pointer" );

	numObst = 0;
	dirX = MKF(0);
	dirY = MKF(0);
	distTot = 0;

	droidGetNaybors(psDroid);

	// scan the neighbours for obstacles
	for(i=0; i<(SDWORD)numNaybors; i++)
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
		xdiff = (SDWORD)psObj->x - (SDWORD)psDroid->x;
		ydiff = (SDWORD)psObj->y - (SDWORD)psDroid->y;
		if (Fmul(MKF(xdiff),(*pX)) + Fmul(MKF(ydiff),(*pY)) < 0)
		{
			// object behind
			continue;
		}

		absx = labs(xdiff);
		absy = labs(ydiff);
		dist = absx > absy ? absx + absy/2 : absx/2 + absy;

		if (dist != 0)
		{
			dirX += Fdiv(MKF(xdiff),MKF(dist*dist));
			dirY += Fdiv(MKF(ydiff),MKF(dist*dist));
			distTot += dist*dist;
			numObst += 1;
		}
		else
		{
			dirX += MKF(xdiff);
			dirY += MKF(ydiff);
			numObst += 1;
		}
	}

	// now scan for blocking tiles
	mapX = (SDWORD)(psDroid->x >> TILE_SHIFT);
	mapY = (SDWORD)(psDroid->y >> TILE_SHIFT);
	for(ydiff=-2; ydiff<=2; ydiff++)
	{
		for(xdiff=-2; xdiff<=2; xdiff++)
		{
			if (Fmul(MKF(xdiff),(*pX)) + Fmul(MKF(ydiff),(*pY)) <= 0)
			{
				// object behind
				continue;
			}
			if (fpathBlockingTile(mapX+xdiff, mapY+ydiff))
			{
				tx = xdiff << TILE_SHIFT;
				ty = ydiff << TILE_SHIFT;
				td = tx*tx + ty*ty;
				if (td < AVOID_DIST*AVOID_DIST)
				{
					absx = labs(tx);
					absy = labs(ty);
					dist = absx > absy ? absx + absy/2 : absx/2 + absy;

					if (dist != 0)
					{
						dirX += Fdiv(MKF(tx),MKF(dist*dist));
						dirY += Fdiv(MKF(ty),MKF(dist*dist));
						distTot += dist*dist;
						numObst += 1;
					}
				}
			}
		}
	}

	if (numObst > 0)
	{
#ifdef ARROWS
		static BOOL bTest = TRUE;
#endif

		distTot /= numObst;

		// Create the avoid vector
		if (dirX == MKF(0) && dirY == MKF(0))
		{
			avoidX = MKF(0);
			avoidY = MKF(0);
			distTot = AVOID_DIST*AVOID_DIST;
		}
		else
		{
			omag = fSQRT(dirX*dirX + dirY*dirY);
			ox = dirX / omag;
			oy = dirY / omag;
			if (FRACTmul((*pX), oy) + FRACTmul((*pY),-ox) < 0)
			{
				DBP3(("First perp\n"));
				avoidX = -oy;
				avoidY = ox;
			}
			else
			{
				DBP3(("Second perp\n"));
				avoidX = oy;
				avoidY = -ox;
			}
		}

		// combine the avoid vector and the target vector
		ratio = Fdiv(MKF(distTot), MKF(AVOID_DIST*AVOID_DIST));
		if (ratio > MKF(1))
		{
			ratio = MKF(1);
		}

		*pX = Fmul((*pX), ratio) + Fmul(avoidX, (1 - ratio));
		*pY = Fmul((*pY), ratio) + Fmul(avoidY, (1 - ratio));

#ifdef ARROWS
		if ( bTest && psDroid->selected)
		{
			SDWORD	iHeadX, iHeadY, iHeadZ;

			/* target direction - yellow */
			iHeadX = psDroid->sMove.targetX;
			iHeadY = psDroid->sMove.targetY;
			iHeadZ = map_Height( iHeadX, iHeadY );
			arrowAdd( psDroid->x, psDroid->y, psDroid->z,
						iHeadX, iHeadY, iHeadZ, YELLOWARROW );

			/* average obstacle vector - green */
			iHeadX = MAKEINT(FRACTmul(ox, 200)) + psDroid->x;
			iHeadY = MAKEINT(FRACTmul(oy, 200)) + psDroid->y;
			arrowAdd( psDroid->x, psDroid->y, psDroid->z,
						iHeadX, iHeadY, iHeadZ, GREENARROW );

			/* normal - green */
			iHeadX = MAKEINT(FRACTmul(avoidX, 100)) + psDroid->x;
			iHeadY = MAKEINT(FRACTmul(avoidY, 100)) + psDroid->y;
			arrowAdd( psDroid->x, psDroid->y, psDroid->z,
						iHeadX, iHeadY, iHeadZ, GREENARROW );

			/* resultant - white */
			iHeadX = MAKEINT(FRACTmul((*pX), 200)) + psDroid->x;
			iHeadY = MAKEINT(FRACTmul((*pY), 200)) + psDroid->y;
			arrowAdd( psDroid->x, psDroid->y, psDroid->z,
						iHeadX, iHeadY, iHeadZ, WHITEARROW );
		}
#endif

	}
}

/*void moveUpdateRepulsiveVector( FRACT fVObstX, FRACT fVObstY, FRACT fVTarX, FRACT fVTarY,
					FRACT *pfVRepX, FRACT *pfVRepY, FRACT *pfDistTot )
{
	FRACT	fDistObjSq, fDistObj, fDistTar;
	FRACT	fDot, fCos, fCross, fAngle, fScale;

	fDistTar   = fSQRT( Fmul( fVTarX, fVTarX ) + Fmul( fVTarY, fVTarY ) );

	fDistObjSq = Fmul( fVObstX, fVObstX ) + Fmul( fVObstY, fVObstY );
	fDistObj   = fSQRT( fDistObjSq );

	fDot = Fmul(fVObstX,fVTarX) + Fmul(fVObstY,fVTarY);
	fCos = Fdiv( fDot, Fmul( fDistTar, fDistObj ) );
	fAngle = trigInvCos( fCos );

	// scale by angle to obstacle (zero repulsion for objects directly behind)
	fScale = Fdiv((MKF(180) - fAngle), MKF(180));

	// scale by distance to obstacle squared
	fScale = Fdiv( fScale, fDistObjSq );

	// decide which avoidance perpendicular to use
	fCross = FRACTmul(fVTarX, fVObstY) + FRACTmul(fVTarY,-fVObstX);
	if ( fCross < 0 )
	{
		*pfVRepX += Fmul( -fVObstY, fScale );
		*pfVRepY += Fmul(  fVObstX, fScale );
	}
	else
	{
		*pfVRepX += Fmul(  fVObstY, fScale );
		*pfVRepY += Fmul( -fVObstX, fScale );
	}

	*pfDistTot += fDistObjSq;
}*/


void moveUpdateRepulsiveVector( FRACT fVObstX, FRACT fVObstY, FRACT fVTarX, FRACT fVTarY,
					FRACT *pfVRepX, FRACT *pfVRepY, FRACT *pfDistTot )
{
	FRACT	fDistObjSq, fDistObj;
	FRACT	fDot, fCross;

	// ignore obstacles behind
	fDot = Fmul(fVObstX,fVTarX) + Fmul(fVObstY,fVTarY);
	if (fDot <= 0)
	{
		return;
	}

	fDistObjSq = Fmul( fVObstX, fVObstX ) + Fmul( fVObstY, fVObstY );
	fDistObj   = fSQRT( fDistObjSq );

	// decide which avoidance perpendicular to use
	fCross = FRACTmul(fVTarX, fVObstY) + FRACTmul(fVTarY,-fVObstX);
	if ( fCross < 0 )
	{
		*pfVRepX += Fdiv( -fVObstY, fDistObj );
		*pfVRepY += Fdiv(  fVObstX, fDistObj );
	}
	else
	{
		*pfVRepX += Fdiv(  fVObstY, fDistObj );
		*pfVRepY += Fdiv( -fVObstX, fDistObj );
	}

	*pfDistTot += fDistObjSq;
}

// get an obstacle avoidance vector
void moveGetObstVector5(DROID *psDroid, FRACT *pX, FRACT *pY)
{
	SDWORD				i,xdiff,ydiff, numObst;
	BASE_OBJECT			*psObj;
	FRACT				dirX,dirY, fDx, fDy, fDistTot;
	FRACT				omag, ratio;
	FRACT				avoidX,avoidY;
	SDWORD				mapX,mapY, tx,ty, td;
	PROPULSION_STATS	*psPropStats;

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( PTRVALID(psPropStats, sizeof(PROPULSION_STATS)),
			"moveUpdateUnit: invalid propulsion stats pointer" );

	numObst  = 0;
	dirX     = MKF(0);
	dirY     = MKF(0);
	fDistTot = MKF(0);

	/* if not flying check ground objects */
	if ( psPropStats->propulsionType != LIFT )
	{
		droidGetNaybors(psDroid);

		// scan the neighbours for obstacles
		for(i=0; i<(SDWORD)numNaybors; i++)
		{
			psObj = asDroidNaybors[i].psObj;
			if (psObj->type != OBJ_DROID ||
				asDroidNaybors[i].distSqr >= AVOID_DIST*AVOID_DIST)
			{
				// object too far away to worry about
				continue;
			}
			if (((DROID *)psObj)->droidType == DROID_TRANSPORTER ||
				(((DROID *)psObj)->droidType == DROID_PERSON &&
				 psObj->player != psDroid->player))
			{
				// don't avoid people on the other side - run over them
				continue;
			}

			xdiff = (SDWORD)psObj->x - (SDWORD)psDroid->x;
			ydiff = (SDWORD)psObj->y - (SDWORD)psDroid->y;

			fDx = MKF(xdiff);
			fDy = MKF(ydiff);

			/* stop droids shagging an inactive droid which is
			 * sitting on their target */
/*			if ( psObj->type == OBJ_STRUCTURE || psObj->type == OBJ_FEATURE ||
				 (psObj->type == OBJ_DROID &&
				  ((DROID *) psObj)->sMove.Status == MOVEINACTIVE) )
			{
				SDWORD	iDx = (SDWORD)psObj->x - psDroid->sMove.targetX,
						iDy = (SDWORD)psObj->y - psDroid->sMove.targetY,
						iRad = moveObjRadius(psObj)*2;

				if ( iDx*iDx + iDy*iDy < iRad*iRad )
				{
					moveStopDroid( psDroid );
					return;
				}
			}*/

			moveUpdateRepulsiveVector( fDx, fDy, *pX, *pY,
										&dirX, &dirY, &fDistTot );

			numObst ++;
		}
	}

	// now scan for blocking tiles
	mapX = (SDWORD)(psDroid->x >> TILE_SHIFT);
	mapY = (SDWORD)(psDroid->y >> TILE_SHIFT);
	for(ydiff=-2; ydiff<=2; ydiff++)
	{
		for(xdiff=-2; xdiff<=2; xdiff++)
		{
			if ( (xdiff != 0 || ydiff != 0) &&
				 fpathBlockingTile(mapX+xdiff, mapY+ydiff))
			{
				tx = xdiff << TILE_SHIFT;
				ty = ydiff << TILE_SHIFT;
				td = tx*tx + ty*ty;
				fDx = MKF(tx);
				fDy = MKF(ty);
				if (td < AVOID_DIST*AVOID_DIST)
				{
					moveUpdateRepulsiveVector( fDx, fDy, *pX, *pY,
												&dirX, &dirY, &fDistTot );
					numObst ++;
				}
			}
		}
	}

	if (numObst > 0)
	{
#ifdef ARROWS
		static BOOL bTest = TRUE;
#endif

		fDistTot /= numObst;

		// Create the avoid vector
		if (dirX == MKF(0) && dirY == MKF(0))
		{
			avoidX = MKF(0);
			avoidY = MKF(0);
			fDistTot = MKF(AVOID_DIST*AVOID_DIST);
		}
		else
		{
			omag = fSQRT(dirX*dirX + dirY*dirY);
			avoidX = dirX / omag;
			avoidY = dirY / omag;
		}

		// combine the avoid vector and the target vector
		ratio = Fdiv(MKF(fDistTot), MKF(AVOID_DIST*AVOID_DIST));
		if (ratio > MKF(1))
		{
			ratio = MKF(1);
		}

		*pX = Fmul((*pX), ratio) + Fmul(avoidX, (1 - ratio));
		*pY = Fmul((*pY), ratio) + Fmul(avoidY, (1 - ratio));

#ifdef ARROWS
		if ( bTest )
		{
			SDWORD	iHeadX, iHeadY, iHeadZ;

			/* target direction - yellow */
			iHeadX = psDroid->sMove.targetX;
			iHeadY = psDroid->sMove.targetY;
			iHeadZ = map_Height( iHeadX, iHeadY );
			arrowAdd( psDroid->x, psDroid->y, psDroid->z,
						iHeadX, iHeadY, iHeadZ, YELLOWARROW );

			/* avoid vector - green */
			iHeadX = MAKEINT(FRACTmul(avoidX, 100)) + psDroid->x;
			iHeadY = MAKEINT(FRACTmul(avoidY, 100)) + psDroid->y;
			arrowAdd( psDroid->x, psDroid->y, psDroid->z,
						iHeadX, iHeadY, iHeadZ, GREENARROW );

			/* resultant - white */
			iHeadX = MAKEINT(FRACTmul((*pX), 200)) + psDroid->x;
			iHeadY = MAKEINT(FRACTmul((*pY), 200)) + psDroid->y;
			arrowAdd( psDroid->x, psDroid->y, psDroid->z,
						iHeadX, iHeadY, iHeadZ, WHITEARROW );
		}
#endif

	}
}



/* Get a direction for a droid to avoid obstacles etc. */
// This routine smells ...
static void moveGetDirection(DROID *psDroid, FRACT *pX, FRACT *pY)
{
	SDWORD	dx,dy, tx,ty;
	SDWORD	mag;
	FRACT	root;
	BOOL	bNoVector;

	SDWORD	ndx,ndy, ntx,nty, nmag;
	FRACT	nroot;


	tx = psDroid->sMove.targetX;
	ty = psDroid->sMove.targetY;

	// Calc the basic vector
	dx = tx - (SDWORD)psDroid->x;
	dy = ty - (SDWORD)psDroid->y;
	// If the droid is getting close to the way point start to phase in the next target
	mag = dx*dx + dy*dy;

	bNoVector = TRUE;

	// fade in the next target point if we arn't at the end of the waypoints
	if ((psDroid->sMove.Position != psDroid->sMove.numPoints) &&
		(mag < WAYPOINT_DSQ))
	{
		// find the next target
		movePeekNextTarget(psDroid, &ntx, &nty);
		ndx = ntx - (SDWORD)psDroid->x;
		ndy = nty - (SDWORD)psDroid->y;
		nmag = ndx*ndx + ndy*ndy;

		if (mag != 0 && nmag != 0)
		{
			// Get the size of the vectors
			root = fSQRT(MAKEFRACT(mag));
			nroot = fSQRT(MAKEFRACT(nmag));

			// Split the proportion of the vectors based on how close to the point they are
			ndx = (ndx * (WAYPOINT_DSQ - mag)) / WAYPOINT_DSQ;
			ndy = (ndy * (WAYPOINT_DSQ - mag)) / WAYPOINT_DSQ;

			dx = (dx * mag) / WAYPOINT_DSQ;
			dy = (dy * mag) / WAYPOINT_DSQ;

			// Calculate the normalised result
			*pX = FRACTdiv(MKF(dx), root) + FRACTdiv(MKF(ndx), nroot);
			*pY = FRACTdiv(MKF(dy), root) + FRACTdiv(MKF(ndy), nroot);
			bNoVector = FALSE;
		}
	}

	if (bNoVector)

	{

		root = fSQRT(MAKEFRACT(mag));
		*pX = FRACTdiv(MKF(dx), root);
		*pY = FRACTdiv(MKF(dy), root);



	}


	if ( psDroid->droidType != DROID_TRANSPORTER )
	{
		moveGetObstVector4(psDroid, pX,pY);
	}


}


// Calculate the boundary vector
void moveCalcBoundary(DROID *psDroid)
{
	SDWORD	absX, absY;
	SDWORD	prevX,prevY, prevMag;
	SDWORD	nTarX,nTarY, nextX,nextY,nextMag;
	SDWORD	sumX,sumY;

	// No points left - simple case for the bound vector
	if (psDroid->sMove.Position == psDroid->sMove.numPoints)
	{
		psDroid->sMove.boundX = (SWORD)(psDroid->sMove.srcX - psDroid->sMove.targetX);
		psDroid->sMove.boundY = (SWORD)(psDroid->sMove.srcY - psDroid->sMove.targetY);
		return;
	}

	// Calculate the vector back along the current path
	prevX = psDroid->sMove.srcX - psDroid->sMove.targetX;
	prevY = psDroid->sMove.srcY - psDroid->sMove.targetY;
	absX = labs(prevX);
	absY = labs(prevY);
	prevMag = absX > absY ? absX + absY/2 : absY + absX/2;
//	prevMag = sqrt(prevX*prevX + prevY*prevY);

	// Calculate the vector to the next target
	movePeekNextTarget(psDroid, &nTarX, &nTarY);
	nextX = nTarX - psDroid->sMove.targetX;
	nextY = nTarY - psDroid->sMove.targetY;
	absX = labs(nextX);
	absY = labs(nextY);
	nextMag = absX > absY ? absX + absY/2 : absY + absX/2;
//	nextMag = sqrt(nextX*nextX + nextY*nextY);

	if (prevMag != 0 && nextMag == 0)
	{
		// don't bother mixing the boundaries - cos there isn't a next vector anyway
		psDroid->sMove.boundX = (SWORD)(psDroid->sMove.srcX - psDroid->sMove.targetX);
		psDroid->sMove.boundY = (SWORD)(psDroid->sMove.srcY - psDroid->sMove.targetY);
		return;
	}
	else if (prevMag == 0 || nextMag == 0)
	{
		psDroid->sMove.boundX = 0;
		psDroid->sMove.boundY = 0;
		return;
	}

	// Calculate the vector between the two
	sumX = (prevX * BOUND_ACC)/prevMag + (nextX * BOUND_ACC)/nextMag;
	sumY = (prevY * BOUND_ACC)/prevMag + (nextY * BOUND_ACC)/nextMag;

	// Rotate by 90 degrees one way and see if it is the same side as the src vector
	// if not rotate 90 the other.
	if (prevX*sumY - prevY*sumX < 0)
	{
		psDroid->sMove.boundX = (SWORD)-sumY;
		psDroid->sMove.boundY = (SWORD)sumX;
	}
	else
	{
		psDroid->sMove.boundX = (SWORD)sumY;
		psDroid->sMove.boundY = (SWORD)-sumX;
	}

	DBP5(("new boundary: droid %d boundary (%d,%d)\n",
			psDroid->id, psDroid->sMove.boundX, psDroid->sMove.boundY));
}


// Check if a droid has got to a way point
BOOL moveReachedWayPoint(DROID *psDroid)
{
	SDWORD	droidX,droidY, iRange;

	// Calculate the vector to the droid
	droidX = (SDWORD)psDroid->x - psDroid->sMove.targetX;
	droidY = (SDWORD)psDroid->y - psDroid->sMove.targetY;

	// see if this is a formation end point
	if (psDroid->droidType == DROID_TRANSPORTER ||
		(psDroid->sMove.psFormation &&
		 formationMember(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid)) ||
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
			return TRUE;
		}
	}
	else
	{
		// if the dot product is -ve the droid has got past the way point
		// but only move onto the next way point if we can see the previous one
		// (this helps units that have got nudged off course).
		if ((psDroid->sMove.boundX * droidX + psDroid->sMove.boundY * droidY <= 0) &&
			fpathTileLOS((SDWORD)psDroid->x >> TILE_SHIFT, (SDWORD)psDroid->y >> TILE_SHIFT, psDroid->sMove.targetX >> TILE_SHIFT, psDroid->sMove.targetY >> TILE_SHIFT))
		{
//		DBPRINTF(("Waypoint %d\n", psDroid->sMove.Position));
			DBP5(("Next waypoint: droid %d bound (%d,%d) target (%d,%d)\n",
					psDroid->id, psDroid->sMove.boundX,psDroid->sMove.boundY,
					droidX,droidY));

			return TRUE;
		}
	}

	return FALSE;
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
#define PSX_SPEED_ADJUST 40

// Calculate the new speed for a droid
SDWORD moveCalcDroidSpeed(DROID *psDroid)
{
	UDWORD			mapX,mapY, damLevel;//, tarSpeed;
	SDWORD			speed, pitch;
	WEAPON_STATS	*psWStats;

	mapX = psDroid->x >> TILE_SHIFT;
	mapY = psDroid->y >> TILE_SHIFT;
	speed = (SDWORD) calcDroidSpeed(psDroid->baseSpeed, TERRAIN_TYPE(mapTile(mapX,mapY)),
							  psDroid->asBits[COMP_PROPULSION].nStat);



/*	if ( vtolDroid(psDroid) &&
		 ((asBodyStats + psDroid->asBits[COMP_BODY].nStat)->size == SIZE_HEAVY) )
	{
		speed /= 2;
	}*/


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


//#ifdef PSX
//	pitch=0;		// hack for the demo
//#endif



	// slow down damaged droids
	damLevel = PERCENT(psDroid->body, psDroid->originalBody);
	if (damLevel < HEAVY_DAMAGE_LEVEL)
	{
		speed = 2 * speed / 3;
	}


	// stop droids that have just fired a no fire while moving weapon
	//if (psDroid->numWeaps > 0 && psDroid->asWeaps[0].lastFired + FOM_MOVEPAUSE > gameTime)
    if (psDroid->asWeaps[0].nStat > 0 && psDroid->asWeaps[0].lastFired + FOM_MOVEPAUSE > gameTime)
	{
		psWStats = asWeaponStats + psDroid->asWeaps[0].nStat;
		if (psWStats->fireOnMove == FOM_NO)
		{
			speed = 0;
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

//	/* adjust speed for formation */
//	if ( moveFormationSpeedLimitingOn() &&
//		 psDroid->sMove.psFormation &&
//		 speed > (SDWORD)psDroid->sMove.psFormation->iSpeed )
//	{
//		speed = psDroid->sMove.psFormation->iSpeed;
//	}

//#if(1)
//	if(psDroid->selected) {
//		printf("%d : %d : %d\n",driveGetSpeed(),psDroid->baseSpeed,speed);
//	}
//#endif

	return speed;
}

BOOL moveDroidStopped( DROID *psDroid, SDWORD speed )
{
	if ((psDroid->sMove.Status == MOVEINACTIVE || psDroid->sMove.Status == MOVEROUTE) &&
		speed == 0 && psDroid->sMove.speed == MKF(0))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void moveUpdateDroidDirection( DROID *psDroid, SDWORD *pSpeed, SDWORD direction,
		SDWORD iSpinAngle, SDWORD iSpinSpeed, SDWORD iTurnSpeed, SDWORD *pDroidDir,
		FRACT *pfSpeed )
{
	SDWORD		adiff;
	FRACT		temp;

	*pfSpeed = MKF(*pSpeed);
	*pDroidDir = (SDWORD) psDroid->direction;

	// don't move if in MOVEPAUSE state
	if (psDroid->sMove.Status == MOVEPAUSE)
	{
		return;
	}

	temp = MKF(*pDroidDir);
	adiff = labs(direction - *pDroidDir);
	if (adiff > TRIG_DEGREES/2)
	{
		adiff = TRIG_DEGREES - adiff;
	}
	if (adiff > iSpinAngle)
	{
		// large change in direction, spin on the spot
		DBP2(("Spin "));
		moveCalcTurn(&temp, MKF(direction), iSpinSpeed);
		*pSpeed = 0;
	}
	else
	{
		// small change in direction, turn while moving
		DBP2(("Curve "));
		moveCalcTurn(&temp, MKF(direction), iTurnSpeed);
	}

	*pDroidDir = MAKEINT(temp);
}






// Calculate current speed perpendicular to droids direction
FRACT moveCalcPerpSpeed( DROID *psDroid, SDWORD iDroidDir, SDWORD iSkidDecel )
{
	SDWORD		adiff;
	FRACT		perpSpeed;

	adiff = labs(iDroidDir - psDroid->sMove.dir);
	perpSpeed = Fmul(psDroid->sMove.speed,trigSin(adiff));

	// decelerate the perpendicular speed
	perpSpeed -= (iSkidDecel * baseSpeed);
	if (perpSpeed < MKF(0))
	{
		perpSpeed = MKF(0);
	}

	return perpSpeed;
}


void moveCombineNormalAndPerpSpeeds( DROID *psDroid, FRACT fNormalSpeed,
										FRACT fPerpSpeed, SDWORD iDroidDir )
{
	SDWORD		finalDir, adiff;
	FRACT		finalSpeed;

	/* set current direction */
	psDroid->direction = (UWORD)iDroidDir;

	/* set normal speed and direction if perpendicular speed is zero */
	if (fPerpSpeed == MKF(0))
	{
		psDroid->sMove.speed = fNormalSpeed;
		psDroid->sMove.dir   = (SWORD)iDroidDir;
		return;
	}

	finalSpeed = fSQRT(Fmul(fNormalSpeed,fNormalSpeed) + Fmul(fPerpSpeed,fPerpSpeed));

	// calculate the angle between the droid facing and movement direction
	finalDir = MAKEINT(trigInvCos(Fdiv(fNormalSpeed,finalSpeed)));

	// choose the finalDir on the same side as the old movement direction
	adiff = labs(iDroidDir - psDroid->sMove.dir);
	if (adiff < TRIG_DEGREES/2)
	{
		if (iDroidDir > psDroid->sMove.dir)
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
		if (iDroidDir > psDroid->sMove.dir)
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

	psDroid->sMove.dir = (SWORD)finalDir;
	psDroid->sMove.speed = finalSpeed;
}




// Calculate the current speed in the droids normal direction
FRACT moveCalcNormalSpeed( DROID *psDroid, FRACT fSpeed, SDWORD iDroidDir,
							SDWORD iAccel, SDWORD iDecel )
{
	SDWORD		adiff;
	FRACT		normalSpeed;

	adiff = labs(iDroidDir - psDroid->sMove.dir);
	normalSpeed = Fmul(psDroid->sMove.speed,trigCos(adiff));

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


void moveGetDroidPosDiffs( DROID *psDroid, FRACT *pDX, FRACT *pDY )
{
	FRACT	move;


	move = Fmul(psDroid->sMove.speed, baseSpeed);


	*pDX = Fmul(move,trigSin(psDroid->sMove.dir));
	*pDY = Fmul(move,trigCos(psDroid->sMove.dir));
}

// see if the droid is close to the final way point
void moveCheckFinalWaypoint( DROID *psDroid, SDWORD *pSpeed )
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
		xdiff = (SDWORD)psDroid->x - psDroid->sMove.targetX;
		ydiff = (SDWORD)psDroid->y - psDroid->sMove.targetY;
		distSq = xdiff*xdiff + ydiff*ydiff;
		if (distSq < END_SPEED_RANGE*END_SPEED_RANGE)
		{
			*pSpeed = (MAX_END_SPEED-minEndSpeed) * distSq
						/ (END_SPEED_RANGE*END_SPEED_RANGE) + minEndSpeed;
		}
	}
}

void moveUpdateDroidPos( DROID *psDroid, FRACT dx, FRACT dy )
{
	SDWORD	iX = 0, iY = 0;

	if (psDroid->sMove.Status == MOVEPAUSE)
	{
		// don't actually move if the move is paused
		return;
	}

	psDroid->sMove.fx += dx;
	psDroid->sMove.fy += dy;
//	psDroid->sMove.dx = dx;
//	psDroid->sMove.dy = dy;

//#ifdef PSX
//	psDroid->x = (UDWORD)MAKEINT(psDroid->sMove.fx);
//	psDroid->y = (UDWORD)MAKEINT(psDroid->sMove.fy);
//
//	if ( psDroid->x > 0x80000000)
//	{
//		DBPRINTF(("Droid off edge of map ... fixing (a)\n"));
//		psDroid->x=1;
//	}
//	else
//	{
//		if ( psDroid->x > mapWidth*TILE_UNITS )
//		{
//			DBPRINTF(("Droid off edge of map ... fixing (b)\n"));
//			psDroid->x= mapWidth*TILE_UNITS-1;
//
//		}
//	}
//
//
//	if ( psDroid->y > 0x80000000)
//	{
//		DBPRINTF(("Droid off edge of map ... fixing (c)\n"));
//		psDroid->y=1;
//	}
//	else
//	{
//		if ( psDroid->y > mapHeight*TILE_UNITS )
//		{
//			DBPRINTF(("Droid off edge of map ... fixing (d)\n"));
//			psDroid->y= mapHeight*TILE_UNITS-1;
//
//		}
//	}
//#else
	iX = MAKEINT(psDroid->sMove.fx);
	iY = MAKEINT(psDroid->sMove.fy);

	/* impact if about to go off map else update coordinates */
	if ( worldOnMap( iX, iY ) == FALSE )
	{
		if ( psDroid->droidType == DROID_TRANSPORTER )
		{
			/* transporter going off-world - trigger next map */

		}
		else
		{
			/* dreadful last-ditch crash-avoiding hack - sort this! - GJ */
			debug( LOG_NEVER, "**** droid about to go off map - fixed ****\n" );
			destroyDroid( psDroid );
		}
	}
	else
	{
		psDroid->x = (UWORD)iX;
		psDroid->y = (UWORD)iY;
	}

	// lovely hack to keep transporters just on the map
	// two weeks to go and the hacks just get better !!!
	if (psDroid->droidType == DROID_TRANSPORTER)
	{
		if (psDroid->x == 0)
		{
			psDroid->x = 1;
		}
		if (psDroid->y == 0)
		{
			psDroid->y = 1;
		}
	}

//#endif
}

/* Update a tracked droids position and speed given target values */
void moveUpdateGroundModel(DROID *psDroid, SDWORD speed, SDWORD direction)
{
	FRACT				fPerpSpeed, fNormalSpeed, dx, dy, fSpeed, bx,by;
	SDWORD				iDroidDir, slideDir;
	PROPULSION_STATS	*psPropStats;
	SDWORD				spinSpeed, turnSpeed, skidDecel;
	// constants for the different propulsion types
	static SDWORD		hvrSkid = HOVER_SKID_DECEL;
	static SDWORD		whlSkid = WHEELED_SKID_DECEL;
	static SDWORD		trkSkid = TRACKED_SKID_DECEL;
	static FRACT		hvrTurn = FRACTCONST(3,4);		//0.75f;
	static FRACT		whlTurn = FRACTCONST(1,1);		//1.0f;
	static FRACT		trkTurn = FRACTCONST(1,1);		//1.0f;

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	switch (psPropStats->propulsionType)
	{
	case HOVER:
		spinSpeed = MAKEINT(psDroid->baseSpeed*hvrTurn);
		turnSpeed = MAKEINT(psDroid->baseSpeed/3*hvrTurn);
		skidDecel = hvrSkid;//HOVER_SKID_DECEL;
		break;
	case WHEELED:
		spinSpeed = MAKEINT(psDroid->baseSpeed*hvrTurn);
		turnSpeed = MAKEINT(psDroid->baseSpeed/3*whlTurn);
		skidDecel = whlSkid;//WHEELED_SKID_DECEL;
		break;
	case TRACKED:
	default:
		spinSpeed = MAKEINT(psDroid->baseSpeed*hvrTurn);
		turnSpeed = MAKEINT(psDroid->baseSpeed/3*trkTurn);
		skidDecel = trkSkid;//TRACKED_SKID_DECEL;
		break;
	}

	// nothing to do if the droid is stopped
	if ( moveDroidStopped( psDroid, speed ) == TRUE )
	{
		return;
	}

	// update the naybors list
	droidGetNaybors(psDroid);

#ifdef DEBUG_DRIVE_SPEED
if(psDroid == driveGetDriven())	debug( LOG_NEVER, "%d ", speed );
#endif

	moveCheckFinalWaypoint( psDroid, &speed );

#ifdef DEBUG_DRIVE_SPEED
if(psDroid == driveGetDriven())	debug( LOG_NEVER, "%d ", speed );
#endif

//	moveUpdateDroidDirection( psDroid, &speed, direction, TRACKED_SPIN_ANGLE,
//				TRACKED_SPIN_SPEED, TRACKED_TURN_SPEED, &iDroidDir, &fSpeed );
	moveUpdateDroidDirection( psDroid, &speed, direction, TRACKED_SPIN_ANGLE,
				spinSpeed, turnSpeed, &iDroidDir, &fSpeed );

#ifdef DEBUG_DRIVE_SPEED
if(psDroid == driveGetDriven())	debug( LOG_NEVER, "%d ", speed );
#endif

	fNormalSpeed = moveCalcNormalSpeed( psDroid, fSpeed, iDroidDir,
										TRACKED_ACCEL, TRACKED_DECEL );
	fPerpSpeed   = moveCalcPerpSpeed( psDroid, iDroidDir, skidDecel );

	moveCombineNormalAndPerpSpeeds( psDroid, fNormalSpeed,
										fPerpSpeed, iDroidDir );

#ifdef DEBUG_DRIVE_SPEED
if(psDroid == driveGetDriven())	debug( LOG_NEVER, "%d\n", speed );
#endif

//	if (psDroid->direction != psDroid->sMove.dir)
/*	if (fPerpSpeed > 0)
	{
		DBPRINTF(("droid %d direction %d total dir %d perpspeed %f\n",
			psDroid->id, psDroid->direction, psDroid->sMove.dir, fPerpSpeed));
	}*/

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
		psDroid->direction = (UWORD)iDroidDir;
	}

	moveUpdateDroidPos( psDroid, bx, by );

	//set the droid height here so other routines can use it
	psDroid->z = map_Height(psDroid->x, psDroid->y);//jps 21july96
	updateDroidOrientation(psDroid);
}

/* Update a persons position and speed given target values */
void moveUpdatePersonModel(DROID *psDroid, SDWORD speed, SDWORD direction)
{
	FRACT			fPerpSpeed, fNormalSpeed, dx, dy, fSpeed;
	SDWORD			iDroidDir, slideDir;
//	BASE_OBJECT		*psObst;
	BOOL			bRet;

	// nothing to do if the droid is stopped
	if ( moveDroidStopped( psDroid, speed ) == TRUE )
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
				ASSERT( bRet == TRUE, "moveUpdatePersonModel: animObj_Remove failed" );
				psDroid->psCurAnim = NULL;
			}

			/* add firing anim */
			if ( psDroid->psCurAnim == NULL )
			{
				psDroid->psCurAnim = animObj_Add( psDroid, ID_ANIM_DROIDFIRE, 0, 0 );
			}
			else
			{
				psDroid->psCurAnim->bVisible = TRUE;
			}

			return;
		}

		/* don't show move animations if inactive */
		if ( psDroid->psCurAnim != NULL )
		{
//			DBPRINTF(("droid anim stopped %p\n",psDroid);
//DBPRINTF(("vis 1 off %p\n",psDroid);


// On the pc we make the animation non-visible
//
// on the psx we remove the animation totally, and reallocate it when it is next needed
//    ... this is so we can allow the playstation to use far fewer animation entries


			psDroid->psCurAnim->bVisible = FALSE;


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
	fPerpSpeed = MKF(0);

	moveCombineNormalAndPerpSpeeds( psDroid, fNormalSpeed,
										fPerpSpeed, iDroidDir );

	moveGetDroidPosDiffs( psDroid, &dx, &dy );

/*	if (moveFindObstacle(psDroid, dx,dy, &psObst))
	{
		moveCalcSlideVector(psDroid, (SDWORD)psObst->x, (SDWORD)psObst->y, &dx, &dy);
	}*/

	moveCalcDroidSlide(psDroid, &dx,&dy);
	moveCalcBlockingSlide(psDroid, &dx,&dy, direction, &slideDir);

	moveUpdateDroidPos( psDroid, dx, dy );

	//set the droid height here so other routines can use it
	psDroid->z = map_Height(psDroid->x, psDroid->y);//jps 21july96

	psDroid->sMove.fz = MAKEFRACT(psDroid->z);


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
			ASSERT( bRet == TRUE, "moveUpdatePersonModel: animObj_Remove failed" );
			psDroid->psCurAnim = NULL;
		}

		/* if no anim currently attached, get one */
		if ( psDroid->psCurAnim == NULL )
		{
			// Only add the animation if the droid is on screen, saves memory and time.
			if(clipXY(psDroid->x,psDroid->y)) {
				debug( LOG_NEVER, "Added person run anim\n" );
				psDroid->psCurAnim = animObj_Add( (BASE_OBJECT *) psDroid,
													ID_ANIM_DROIDRUN, 0, 0 );
			}
		} else {
			// If the droid went off screen then remove the animation, saves memory and time.
			if(!clipXY(psDroid->x,psDroid->y)) {
				bRet = animObj_Remove( &psDroid->psCurAnim, psDroid->psCurAnim->psAnim->uwID );
				ASSERT( bRet == TRUE, "moveUpdatePersonModel : animObj_Remove failed" );
				psDroid->psCurAnim = NULL;
				debug( LOG_NEVER, "Removed person run anim\n" );
			}
		}
	}

	/* show anim */
	if ( psDroid->psCurAnim != NULL )
	{
		psDroid->psCurAnim->bVisible = TRUE;
	}
}

//#define	VTOL_VERTICAL_SPEED		((SDWORD)psDroid->baseSpeed / 4)
#define	VTOL_VERTICAL_SPEED		((((SDWORD)psDroid->baseSpeed / 4) > 60) ? ((SDWORD)psDroid->baseSpeed / 4) : 60)

/* primitive 'bang-bang' vtol height controller */
void moveAdjustVtolHeight( DROID * psDroid, UDWORD iMapHeight )
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

	if ( psDroid->z >= (iMapHeight+iMaxHeight) )
	{
		psDroid->sMove.iVertSpeed = (SWORD)-VTOL_VERTICAL_SPEED;
	}
	else if ( psDroid->z < (iMapHeight+iMinHeight) )
	{
		psDroid->sMove.iVertSpeed = (SWORD)VTOL_VERTICAL_SPEED;
	}
	else if ( (psDroid->z < iLevelHeight) &&
			  (psDroid->sMove.iVertSpeed < 0 )    )
	{
		psDroid->sMove.iVertSpeed = 0;
	}
	else if ( (psDroid->z > iLevelHeight) &&
			  (psDroid->sMove.iVertSpeed > 0 )    )
	{
		psDroid->sMove.iVertSpeed = 0;
	}
}

// set a vtol to be hovering in the air
void moveMakeVtolHover( DROID *psDroid )
{
	psDroid->sMove.Status = MOVEHOVER;
	psDroid->z = (UWORD)(map_Height(psDroid->x,psDroid->y) + VTOL_HEIGHT_LEVEL);
}

void moveUpdateVtolModel(DROID *psDroid, SDWORD speed, SDWORD direction)
{
	FRACT	fPerpSpeed, fNormalSpeed, dx, dy, fSpeed;
	SDWORD	iDroidDir, iMapZ, iRoll, slideDir, iSpinSpeed, iTurnSpeed;
//	SDWORD iDZ, iDroidZ;

	FRACT	fDZ, fDroidZ, fMapZ;


	// nothing to do if the droid is stopped
	if ( moveDroidStopped(  psDroid, speed ) == TRUE )
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
		fpathBlockingTile = fpathLiftSlideBlockingTile;
		moveCalcBlockingSlide( psDroid, &dx, &dy, direction, &slideDir );
		fpathBlockingTile = fpathGroundBlockingTile;
	}

	moveUpdateDroidPos( psDroid, dx, dy );

	/* update vtol orientation */
	iRoll = (psDroid->sMove.dir - (SDWORD) psDroid->direction) / 3;
	if ( iRoll < 0 )
	{
		iRoll += 360;
	}
	psDroid->roll = (UWORD) iRoll;

	iMapZ = map_Height(psDroid->x, psDroid->y);

	/* do vertical movement */

	fDZ = (FRACT)(psDroid->sMove.iVertSpeed * (SDWORD)frameTime) / GAME_TICKS_PER_SEC;
	fDroidZ = psDroid->sMove.fz;
	fMapZ = (FRACT) map_Height(psDroid->x, psDroid->y);
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
	psDroid->z = (UWORD)psDroid->sMove.fz;


	moveAdjustVtolHeight( psDroid, iMapZ );
}

#ifndef FINALBUILD

void moveGetStatusStr( UBYTE status, char *szStr )
{
	switch ( status )
	{
		case MOVEINACTIVE:
			strcpy( szStr, "MOVEINACTIVE" );
			break;
		case MOVENAVIGATE:
			strcpy( szStr, "MOVENAVIGATE" );
			break;
		case MOVETURN:
			strcpy( szStr, "MOVETURN" );
			break;
		case MOVEPAUSE:
			strcpy( szStr, "MOVEPAUSE" );
			break;
		case MOVEPOINTTOPOINT:
			strcpy( szStr, "MOVEPOINTTOPOINT" );
			break;
		case MOVETURNSTOP:
			strcpy( szStr, "MOVETURNSTOP" );
			break;
		case MOVETURNTOTARGET:
			strcpy( szStr, "MOVETURNTOTARGET" );
			break;
		case MOVEROUTE:
			strcpy( szStr, "MOVEROUTE" );
			break;
		case MOVEHOVER:
			strcpy( szStr, "MOVEHOVER" );
			break;
		case MOVEDRIVE:
			strcpy( szStr, "MOVEDRIVE" );
			break;
		case MOVEDRIVEFOLLOW:
			strcpy( szStr, "MOVEDRIVEFOLLOW" );
			break;
		default:
			strcpy( szStr, "" );
			break;
	}
}
#endif

#define CYBORG_VERTICAL_SPEED	((SDWORD)psDroid->baseSpeed/2)

void
moveCyborgLaunchAnimDone( ANIM_OBJECT *psObj )
{
	DROID	*psDroid = psObj->psParent;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
			"moveCyborgLaunchAnimDone: invalid cyborg pointer" );

	/* raise cyborg a little bit so flying - terrible hack - GJ */
	psDroid->z++;
	psDroid->sMove.iVertSpeed = (SWORD)CYBORG_VERTICAL_SPEED;

	psDroid->psCurAnim = NULL;
}

void
moveCyborgTouchDownAnimDone( ANIM_OBJECT *psObj )
{
	DROID	*psDroid = psObj->psParent;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
			"moveCyborgTouchDownAnimDone: invalid cyborg pointer" );

	psDroid->psCurAnim = NULL;
	psDroid->z = map_Height( psDroid->x, psDroid->y );
}


void moveUpdateJumpCyborgModel(DROID *psDroid, SDWORD speed, SDWORD direction)
{
	FRACT	fPerpSpeed, fNormalSpeed, dx, dy, fSpeed;
	SDWORD	iDroidDir;

	// nothing to do if the droid is stopped
	if ( moveDroidStopped(  psDroid, speed ) == TRUE )
	{
		return;
	}

	// update the naybors list
	droidGetNaybors(psDroid);

	moveUpdateDroidDirection( psDroid, &speed, direction, VTOL_SPIN_ANGLE,
				psDroid->baseSpeed, psDroid->baseSpeed/3, &iDroidDir, &fSpeed );

	fNormalSpeed = moveCalcNormalSpeed( psDroid, fSpeed, iDroidDir,
										VTOL_ACCEL, VTOL_DECEL );
	fPerpSpeed   = MKF(0);
	moveCombineNormalAndPerpSpeeds( psDroid, fNormalSpeed,
										fPerpSpeed, iDroidDir );

	moveGetDroidPosDiffs( psDroid, &dx, &dy );
	moveUpdateDroidPos( psDroid, dx, dy );
}

void
moveUpdateCyborgModel( DROID *psDroid, SDWORD moveSpeed, SDWORD moveDir, UBYTE oldStatus )
{
	PROPULSION_STATS	*psPropStats;
	BASE_OBJECT			*psObj = (BASE_OBJECT *) psDroid;
	UDWORD				iMapZ = map_Height(psDroid->x, psDroid->y);
	SDWORD				iDist, iDx, iDy, iDz, iDroidZ;
	BOOL			bRet;


	// nothing to do if the droid is stopped
	if ( moveDroidStopped( psDroid, moveSpeed ) == TRUE )
	{
		if ( psDroid->psCurAnim != NULL )
		{
			if ( animObj_Remove( &psDroid->psCurAnim, psDroid->psCurAnim->uwID ) == FALSE )
			{
				debug( LOG_NEVER, "moveUpdateCyborgModel: couldn't remove walk anim\n" );
			}
			psDroid->psCurAnim = NULL;
		}

		return;
	}

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( PTRVALID(psPropStats, sizeof(PROPULSION_STATS)),
			"moveUpdateCyborgModel: invalid propulsion stats pointer" );

	/* do vertical movement */
	if ( psPropStats->propulsionType == JUMP )
	{
		iDz = psDroid->sMove.iVertSpeed * (SDWORD)frameTime / GAME_TICKS_PER_SEC;
		iDroidZ = (SDWORD) psDroid->z;

		if ( iDroidZ+iDz < (SDWORD) iMapZ )
		{
			psDroid->sMove.iVertSpeed = 0;
			psDroid->z = (UWORD)iMapZ;
		}
		else
		{
			psDroid->z = (UWORD)(psDroid->z + iDz);
		}

		if ( (psDroid->z >= (iMapZ+CYBORG_MAX_JUMP_HEIGHT)) &&
			 (psDroid->sMove.iVertSpeed > 0)                        )
		{
			psDroid->sMove.iVertSpeed = (SWORD)-CYBORG_VERTICAL_SPEED;
		}


		psDroid->sMove.fz = MAKEFRACT(psDroid->z);

	}

	/* calculate move distance */
	iDx = (SDWORD) psDroid->sMove.DestinationX - (SDWORD) psDroid->x;
	iDy = (SDWORD) psDroid->sMove.DestinationY - (SDWORD) psDroid->y;
	iDz = (SDWORD) psDroid->z - (SDWORD) iMapZ;
	iDist = MAKEINT( trigIntSqrt( iDx*iDx + iDy*iDy ) );

	/* set jumping cyborg walking short distances */
	if ( (psPropStats->propulsionType != JUMP) ||
		 ((psDroid->sMove.iVertSpeed == 0)      &&
		  (iDist < CYBORG_MIN_JUMP_DISTANCE))       )
	{
		if ( psDroid->psCurAnim == NULL )
		{
			// Only add the animation if the droid is on screen, saves memory and time.
			if(clipXY(psDroid->x,psDroid->y))
			{
//DBPRINTF(("Added cyborg run anim\n"));
                //What about my new cyborg droids?????!!!!!!!
				/*if ( psDroid->droidType == DROID_CYBORG )
				{
					psDroid->psCurAnim = animObj_Add( psObj, ID_ANIM_CYBORG_RUN, 0, 0 );
				}
				else if ( psDroid->droidType == DROID_CYBORG_SUPER )
				{
					psDroid->psCurAnim = animObj_Add( psObj, ID_ANIM_SUPERCYBORG_RUN, 0, 0 );
				}*/
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
			if(!clipXY(psDroid->x,psDroid->y)) {
				bRet = animObj_Remove( &psDroid->psCurAnim, psDroid->psCurAnim->psAnim->uwID );
				ASSERT( bRet == TRUE, "moveUpdateCyborgModel : animObj_Remove failed" );
				psDroid->psCurAnim = NULL;
//DBPRINTF(("Removed cyborg run anim\n"));
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
				 (animObj_Remove( &psDroid->psCurAnim, psDroid->psCurAnim->uwID ) == FALSE) )
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
			if ( psDroid->z == iMapZ )
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

BOOL moveDescending( DROID *psDroid, UDWORD iMapHeight )
{

	if ( psDroid->z > iMapHeight )

	{
		/* descending */
		psDroid->sMove.iVertSpeed = (SWORD)-VTOL_VERTICAL_SPEED;
		psDroid->sMove.speed = MAKEFRACT(0);
//		psDroid->sMove.Speed = 0;

		/* return TRUE to show still descending */
		return TRUE;
	}
	else
	{
		/* on floor - stop */
		psDroid->sMove.iVertSpeed = 0;

		/* conform to terrain */
		updateDroidOrientation(psDroid);

		/* return FALSE to show stopped descending */
		return FALSE;
	}
}


BOOL moveCheckDroidMovingAndVisible( AUDIO_SAMPLE *psSample )
{
	DROID	*psDroid;

	ASSERT( PTRVALID(psSample, sizeof(AUDIO_SAMPLE)),
		"moveCheckUnitMovingAndVisible: audio sample pointer invalid\n" );

	if ( psSample->psObj == NULL )
	{
		return FALSE;
	}
	else
	{
		psDroid = psSample->psObj;
		ASSERT( PTRVALID(psDroid, sizeof(DROID)),
			"moveCheckUnitMovingAndVisible: unit pointer invalid\n" );
	}

	/* check for dead, not moving or invisible to player */
	if ( psDroid->died || moveDroidStopped( psDroid, 0 ) ||
		 (psDroid->droidType == DROID_TRANSPORTER && psDroid->order == DORDER_NONE) ||
		 !(psDroid->visible[selectedPlayer] OR godMode)                                )
	{
		psDroid->iAudioID = NO_SOUND;
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}



void movePlayDroidMoveAudio( DROID *psDroid )
{
	SDWORD				iAudioID = NO_SOUND;
	PROPULSION_TYPES	*psPropType;
	UBYTE				iPropType = 0;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"movePlayUnitMoveAudio: unit pointer invalid\n" );

	if ( (psDroid != NULL) &&
		 (psDroid->visible[selectedPlayer] OR godMode) )
	{
		iPropType = asPropulsionStats[(psDroid)->asBits[COMP_PROPULSION].nStat].propulsionType;
		psPropType = &asPropulsionTypes[iPropType];

		/* play specific wheeled and transporter or stats-specified noises */
		if ( iPropType == WHEELED && psDroid->droidType != DROID_CONSTRUCT )
		{
			iAudioID = ID_SOUND_TREAD;
		}
		else if ( psDroid->droidType == DROID_TRANSPORTER )
		{
			iAudioID = ID_SOUND_BLIMP_FLIGHT;
		}
		//else if ( iPropType == LEGGED && psDroid->droidType == DROID_CYBORG )
        else if ( iPropType == LEGGED && cyborgDroid(psDroid))
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



BOOL moveDroidStartCallback( AUDIO_SAMPLE *psSample )
{
	DROID				*psDroid;

	ASSERT( PTRVALID(psSample, sizeof(AUDIO_SAMPLE)),
		"moveUnitStartCallback: audio sample pointer invalid\n" );

	if ( psSample->psObj == NULL )
	{
		return FALSE;
	}
	else
	{
		psDroid = psSample->psObj;
		ASSERT( PTRVALID(psDroid, sizeof(DROID)),
			"moveDroidStartCallback: unit pointer invalid\n" );
	}

	if ( psDroid != NULL )
	{
		movePlayDroidMoveAudio( psDroid );
	}

	return TRUE;
}



void movePlayAudio( DROID *psDroid, BOOL bStarted, BOOL bStoppedBefore, SDWORD iMoveSpeed )
{
	UBYTE				propType;
	PROPULSION_STATS	*psPropStats;
	PROPULSION_TYPES	*psPropType;
	BOOL				bStoppedNow;
	SDWORD				iAudioID = NO_SOUND;
	AUDIO_CALLBACK		pAudioCallback = NULL;

	/* get prop stats */
	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( PTRVALID(psPropStats, sizeof(PROPULSION_STATS)),
			"moveUpdateUnit: invalid propulsion stats pointer" );
	propType = psPropStats->propulsionType;
	psPropType = &asPropulsionTypes[propType];

	/* get current droid motion status */
	bStoppedNow = moveDroidStopped( psDroid, iMoveSpeed );

	if ( bStarted )
	{
		/* play start audio */
		if ( (propType == WHEELED && psDroid->droidType != DROID_CONSTRUCT) ||
			 (psPropType->startID == NO_SOUND)                                 )
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
	else if ( (!bStoppedBefore && !bStoppedNow) &&
			  (psDroid->iAudioID == NO_SOUND)      )
	{
		/* play move audio */
		movePlayDroidMoveAudio( psDroid );
		return;
	}

	if ( (iAudioID != NO_SOUND) &&
		 (psDroid->visible[selectedPlayer] OR godMode) )
	{
		if ( audio_PlayObjDynamicTrack( psDroid, iAudioID,
				pAudioCallback ) )
		{
			psDroid->iAudioID = iAudioID;
		}
	}

#if 0
if ( oldStatus != newStatus )
{
	char	szOldStatus[100], szNewStatus[100];
	moveGetStatusStr( oldStatus, szOldStatus );
	moveGetStatusStr( newStatus, szNewStatus );
	debug( LOG_NEVER, "oldStatus = %s newStatus = %s\n", szOldStatus, szNewStatus );
}
#endif

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
			giftPower(ANYPLAYER,selectedPlayer,TRUE);			// give power and tell everyone.
			addOilDrum(1);
		}
		else

		{
			addPower(selectedPlayer,OILDRUM_POWER);
		}
		removeFeature((FEATURE*)psObj);							// remove artifact+ send multiplay info.

	}
}


//static UDWORD LastMoveFrame;

/* Frame update for the movement of a tracked droid */
void moveUpdateDroid(DROID *psDroid)
{
//	SDWORD		xdiff,ydiff, obstX,obstY;
//	UDWORD		mapX,mapY, tarSpeed;
//	FRACT		newX,newY;
//	FRACT		speed;
//	FRACT		dangle;
//	BASE_OBJECT	*psObst;

	FRACT				tx,ty;		 //adiff, dx,dy, mx,my;
	FRACT				tangle;		// thats DROID angle and TARGET angle - not some bizzare pun :-)
									// doesn't matter - they're still shit names...! :-)
	SDWORD				fx, fy;
	UDWORD				oldx, oldy, iZ;
	UBYTE				oldStatus = psDroid->sMove.Status;
	SDWORD				moveSpeed, moveDir;
	PROPULSION_STATS	*psPropStats;
	iVector				pos;
	BOOL				bStarted = FALSE, bStopped;
//	UDWORD				landX,landY;

//	ASSERT( psDroid->x != 0 && psDroid->y != 0,
//		"moveUpdateUnit: unit at (0,0)" );

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( PTRVALID(psPropStats, sizeof(PROPULSION_STATS)),
			"moveUpdateUnit: invalid propulsion stats pointer" );

//	if(driveModeActive()) {
//		driveUpdateDroid(psDroid);
//	}

    //if the droid has been attacked by an EMP weapon, it is temporarily disabled
    if (psDroid->lastHitWeapon == WSC_EMP)
    {
        if (gameTime - psDroid->timeLastHit < EMP_DISABLE_TIME)
        {
            //get out without updating
            return;
        }
    }

	/* save current motion status of droid */
	bStopped = moveDroidStopped( psDroid, 0 );

#ifdef DEBUG_GROUP4
	if (psDroid->sMove.Status != MOVEINACTIVE &&
		psDroid->sMove.Status != MOVETURN &&
		psDroid->sMove.Status != MOVEPOINTTOPOINT)
	{
		DBP4(("status: id %d state %d\n", psDroid->id, psDroid->sMove.Status));
	}
#endif

	fpathSetBlockingTile( psPropStats->propulsionType );
	fpathSetCurrentObject( (BASE_OBJECT *) psDroid );

	moveSpeed = 0;
	moveDir = (SDWORD)psDroid->direction;

	/* get droid height */
	iZ = map_Height(psDroid->x, psDroid->y);

	switch (psDroid->sMove.Status)
	{
	case MOVEINACTIVE:
		if ( (psDroid->droidType == DROID_PERSON) &&
			 (psDroid->psCurAnim != NULL) &&
			 (psDroid->psCurAnim->psAnim->uwID == ID_ANIM_DROIDRUN) )
		{
			psDroid->psCurAnim->bVisible = FALSE;
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
				DBP6(("Waiting droid set to %d (player %d) started at %d now %d (none waiting)\n",
					psDroid->id, psDroid->player, psDroid->sMove.bumpTime, gameTime));
				psNextRouteDroid = psDroid;
			}

			else if (bMultiPlayer &&
					 (psNextRouteDroid->sMove.bumpTime > psDroid->sMove.bumpTime))
			{
				DBP6(("Waiting droid set to %d (player %d) started at %d now %d (mulitplayer)\n",
					psDroid->id, psDroid->player, psDroid->sMove.bumpTime, gameTime));
				psNextRouteDroid = psDroid;
			}

			else if ( (psDroid->player == selectedPlayer) &&
					  ( (psNextRouteDroid->player != selectedPlayer) ||
						(psNextRouteDroid->sMove.bumpTime > psDroid->sMove.bumpTime) ) )
			{
				DBP6(("Waiting droid set to %d (player %d) started at %d now %d (selectedPlayer)\n",
					psDroid->id, psDroid->player, psDroid->sMove.bumpTime, gameTime));
				psNextRouteDroid = psDroid;
			}
			else if ( (psDroid->player != selectedPlayer) &&
					  (psNextRouteDroid->player != selectedPlayer) &&
					  (psNextRouteDroid->sMove.bumpTime > psDroid->sMove.bumpTime) )
			{
				DBP6(("Waiting droid set to %d (player %d) started at %d now %d (non selectedPlayer)\n",
					psDroid->id, psDroid->player, psDroid->sMove.bumpTime, gameTime));
				psNextRouteDroid = psDroid;
			}
		}

		if ((psDroid->sMove.Status == MOVEROUTE) ||
			(psDroid->sMove.Status == MOVEROUTESHUFFLE))
//			 (gameTime >= psDroid->sMove.bumpTime) )
		{
			psDroid->sMove.fx = MAKEFRACT(psDroid->x);
			psDroid->sMove.fy = MAKEFRACT(psDroid->y);

			psDroid->sMove.fz = MAKEFRACT(psDroid->z);

//			psDroid->sMove.bumpTime = 0;

			turnOffMultiMsg(TRUE);
			moveDroidTo(psDroid, psDroid->sMove.DestinationX,psDroid->sMove.DestinationY);
			fpathSetBlockingTile( psPropStats->propulsionType );
			turnOffMultiMsg(FALSE);
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
				moveGetDirection(psDroid, &tx,&ty);

				// Turn the droid if necessary
				tangle = vectorToAngle(tx,ty);

				if ( psDroid->droidType == DROID_TRANSPORTER )
				{
					debug( LOG_NEVER, "a) dir %g,%g (%g)\n", tx, ty, tangle );
				}

				moveSpeed = moveCalcDroidSpeed(psDroid);
				moveDir = MAKEINT(tangle);
			}
		}

		break;
	case MOVEWAITROUTE:
		moveDroidTo(psDroid, psDroid->sMove.DestinationX,psDroid->sMove.DestinationY);
		fpathSetBlockingTile( psPropStats->propulsionType );
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
//		psDroid->direction = calcDirection(psDroid->x,psDroid->y, tarX,tarY);
//		moveCalcVector(psDroid, tarX,tarY, &psDroid->sMove.dx,&psDroid->sMove.dy)
		psDroid->sMove.fx = MAKEFRACT(psDroid->x);
		psDroid->sMove.fy = MAKEFRACT(psDroid->y);

		psDroid->sMove.fz = MAKEFRACT(psDroid->z);


		moveCalcBoundary(psDroid);

		if (vtolDroid(psDroid))
		{
			psDroid->pitch = 0;
		}

		psDroid->sMove.Status = MOVEPOINTTOPOINT;
		psDroid->sMove.bumpTime = 0;

		/* save started status for movePlayAudio */
		if ( psDroid->sMove.speed == MKF(0) )
		{
			bStarted = TRUE;
		}

		break;
	case MOVEPOINTTOPOINT:
	case MOVEPAUSE:
		// moving between two way points

#ifdef ARROWS
		// display the route
		if (psDroid->selected)
		{
			SDWORD	pos, x,y,z, px,py,pz;

			// display the boundary vector
			x = psDroid->sMove.targetX;
			y = psDroid->sMove.targetY;
			z = map_Height( x,y );
			px = x - psDroid->sMove.boundY;
			py = y + psDroid->sMove.boundX;
			pz = map_Height( px,py );
			arrowAdd( x,y,z, px,py,pz, REDARROW );

			// display the route
			px = (SDWORD)psDroid->x;
			py = (SDWORD)psDroid->y;
			pz = map_Height( px, py );
			pos = psDroid->sMove.Position;
			pos = ((pos - 1) <= 0) ? 0 : (pos - 1);
			for(; pos < psDroid->sMove.numPoints; pos += 1)
			{
				x = (psDroid->sMove.asPath[pos].x << TILE_SHIFT) + TILE_UNITS/2;
				y = (psDroid->sMove.asPath[pos].y << TILE_SHIFT) + TILE_UNITS/2;
				z = map_Height( x,y );
				arrowAdd( px,py,pz, x,y,z, REDARROW );

				px = x; py = y; pz = z;
			}
		}
#endif

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
				DBP2(("\n"));
				break;
			}
			moveCalcBoundary(psDroid);
		}

#if !FORMATIONS_DISABLE
		if (psDroid->sMove.psFormation &&
			psDroid->sMove.Position == psDroid->sMove.numPoints)
		{
			if (vtolDroid(psDroid))
			{
				// vtols have to use the ground blocking tile when they are going to land
				fpathBlockingTile = fpathGroundBlockingTile;
			}

			if (formationGetPos(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid, &fx,&fy,TRUE))
			{
				psDroid->sMove.targetX = fx;
				psDroid->sMove.targetY = fy;
				moveCalcBoundary(psDroid);
			}

			/*if (vtolDroid(psDroid))
			{
				// reset to the normal blocking tile
				fpathBlockingTile = fpathLiftBlockingTile;
			}*/
		}
#endif
//		DebugP=FALSE;
//		if ( psDroid->droidType == DROID_TRANSPORTER ) DebugP=TRUE;

		// Calculate a target vector
		moveGetDirection(psDroid, &tx,&ty);


		// Turn the droid if necessary
		// calculate the difference in the angles
//		dangle = (float) psDroid->direction;
		tangle = vectorToAngle(tx,ty);




		moveSpeed = moveCalcDroidSpeed(psDroid);

		moveDir = MAKEINT(tangle);


//if ( psDroid->droidType == DROID_TRANSPORTER )
//{
//			DBPRINTF(("dir %d,%d ($%x=%d)\n",tx,ty,tangle,moveDir));
//	}


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
			psDroid->direction != (UDWORD)psDroid->sMove.psFormation->dir)
		{
			moveSpeed = 0;
			moveDir = psDroid->sMove.psFormation->dir;
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
		moveDir = (SDWORD)calcDirection(psDroid->x,psDroid->y,
								psDroid->sMove.targetX, psDroid->sMove.targetY);
		if (psDroid->direction == (UDWORD)moveDir)
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
		moveDir = MAKEINT(tangle);*/

		/* descend if no orders or actions or cyborg at target */
/*		if ( (psDroid->droidType == DROID_CYBORG) ||
			 ((psDroid->action == DACTION_NONE) && (psDroid->order == DORDER_NONE)) ||
			 ((psDroid->action == DACTION_NONE) && (psDroid->order == DORDER_TRANSPORTIN)) ||
			 ((psDroid->action == DACTION_NONE) && (psDroid->order == DORDER_GUARD)) ||
			 (psDroid->action == DACTION_MOVE) || (psDroid->action == DACTION_WAITFORREARM) ||
			 (psDroid->action == DACTION_WAITDURINGREARM)  || (psDroid->action == DACTION_FIRESUPPORT))*/
		{
			if ( moveDescending( psDroid, iZ ) == FALSE )
			{
				/* reset move state */
				psDroid->sMove.Status = MOVEINACTIVE;
			}
/*			else
			{
				// see if the landing position is clear
				landX = psDroid->x;
				landY = psDroid->y;
				actionVTOLLandingPos(psDroid, &landX,&landY);
				if ((SDWORD)(landX >> TILE_SHIFT) != (psDroid->x >> TILE_SHIFT) &&
					(SDWORD)(landY >> TILE_SHIFT) != (psDroid->y >> TILE_SHIFT))
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
		moveSpeed = driveGetMoveSpeed();	//MAKEINT(psDroid->sMove.speed);
		moveDir = driveGetMoveDir();		//psDroid->sMove.dir;
//		DBPRINTF(("%d\n",frameGetFrameNumber()-LastMoveFrame);
//		LastMoveFrame = frameGetFrameNumber();
//		psDroid->sMove.speed = MAKEFRACT(driveSpeed);
//		psDroid->sMove.dir = driveDir;
//		psDroid->direction = driveDir;
		break;

	// Follow the droid being driven around by the player.
	case MOVEDRIVEFOLLOW:
//		orderDroidLoc(psDroid,DORDER_MOVE, psDrivenDroid->x,psDrivenDroid->y)
//		if (gameTime >= psDroid->sMove.bumpTime)
//		{
//			if(psDrivenDroid != NULL) {
//				psDroid->sMove.DestinationX = psDrivenDroid->x;
//				psDroid->sMove.DestinationY = psDrivenDroid->y;
//				psDroid->sMove.fx = MAKEFRACT(psDroid->x);
//				psDroid->sMove.fy = MAKEFRACT(psDroid->y);
//				psDroid->sMove.bumpTime = 0;
//				moveDroidTo(psDroid, psDroid->sMove.DestinationX,psDroid->sMove.DestinationY);
//			} else {
//				psDroid->sMove.Status = MOVEINACTIVE;
//			}
//		}
		break;

	default:
		ASSERT( FALSE, "moveUpdateUnit: unknown move state" );
		break;
	}

	// Update the movement model for the droid
	oldx = psDroid->x;
	oldy = psDroid->y;

	if ( psDroid->droidType == DROID_PERSON )
	{
		moveUpdatePersonModel(psDroid,moveSpeed,moveDir);
	}
	//else if ( psDroid->droidType == DROID_CYBORG )
    else if (cyborgDroid(psDroid))
	{
		moveUpdateCyborgModel(psDroid,moveSpeed,moveDir,oldStatus);
	}
	else if ( psPropStats->propulsionType == LIFT )
	{
		moveUpdateVtolModel(psDroid,moveSpeed,moveDir);
	}
	else
	{
		moveUpdateGroundModel(psDroid,moveSpeed,moveDir);
	}


	if ((SDWORD)oldx >> TILE_SHIFT != psDroid->x >> TILE_SHIFT ||
		(SDWORD)oldy >> TILE_SHIFT != psDroid->y >> TILE_SHIFT)

	{

		visTilesUpdate((BASE_OBJECT *)psDroid,FALSE);

		gridMoveObject((BASE_OBJECT *)psDroid, (SDWORD)oldx,(SDWORD)oldy);

		// object moved from one tile to next, check to see if droid is near stuff.(oil)
		checkLocalFeatures(psDroid);
	}

	// See if it's got blocked
	if ( (psPropStats->propulsionType != LIFT) && moveBlocked(psDroid) )
	{
		DBP4(("status: id %d blocked\n", psDroid->id));
		psDroid->sMove.Status = MOVETURN;
	}

//	// If were in drive mode and the droid is a follower then stop it when it gets within
//	// range of the driver.
//	if(driveIsFollower(psDroid)) {
//		if(DoFollowRangeCheck) {
////DBPRINTF(("%d\n",gameTime);
//			if(driveInDriverRange(psDroid)) {
//				psDroid->sMove.Status = MOVEINACTIVE;
////				ClearFollowRangeCheck = TRUE;
//			} else {
//				AllInRange = FALSE;
//			}
//		}
//	}

	// reset the blocking tile function and current object
	fpathBlockingTile = fpathGroundBlockingTile;
	fpathSetCurrentObject( NULL );

//	ASSERT( psDroid->x != 0 && psDroid->y != 0,
//		"moveUpdateUnit (end): unit at (0,0)" );



	/* If it's sitting in water then it's got to go with the flow! */
	if(TERRAIN_TYPE(mapTile(psDroid->x/TILE_UNITS,psDroid->y/TILE_UNITS)) == TER_WATER)
	{
		updateDroidOrientation(psDroid);
	}



	if( (psDroid->inFire AND psDroid->type != DROID_PERSON) AND psDroid->visible[selectedPlayer])
	{
		pos.x = psDroid->x + (18-rand()%36);
		pos.z = psDroid->y + (18-rand()%36);
//		pos.y = map_Height(pos.x,pos.z) + (psDroid->sDisplay.imd->ymax/3);
		pos.y = psDroid->z + (psDroid->sDisplay.imd->ymax/3);
		addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
	}


#if DROID_RUN_SOUND
	movePlayAudio( psDroid, bStarted, bStopped, moveSpeed );
#endif

}
