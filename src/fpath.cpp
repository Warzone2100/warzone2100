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
/**
 * @file fpath.c
 *
 * Interface to the routing functions.
 *
 */

#include "lib/framework/frame.h"
#include "lib/framework/crc.h"
#include "lib/netplay/netplay.h"

#include "lib/framework/wzapp_c.h"

#include "objects.h"
#include "map.h"
#include "raycast.h"
#include "geometry.h"
#include "hci.h"
#include "order.h"
#include "multiplay.h"
#include "astar.h"
#include "action.h"

#include "fpath.h"

// If the path finding system is shutdown or not
static volatile bool fpathQuit = false;

/* Beware: Enabling this will cause significant slow-down. */
#undef DEBUG_MAP

/* minimum height difference for VTOL blocking tile */
#define	LIFT_BLOCK_HEIGHT_LIGHTBODY		  30
#define	LIFT_BLOCK_HEIGHT_MEDIUMBODY	 350
#define	LIFT_BLOCK_HEIGHT_HEAVYBODY		 350

struct PATHRESULT
{
	UDWORD		droidID;	///< Unique droid ID.
	MOVE_CONTROL	sMove;		///< New movement values for the droid.
	FPATH_RETVAL	retval;		///< Result value from path-finding.
};


#define NUM_BASIC	 8
#define NUM_DIR		24

// Convert a direction into an offset, spanning two tiles
static const Vector2i aDirOffset[NUM_DIR] =
{
	Vector2i( 0,  1), Vector2i(-1,  1), Vector2i(-1,  0), Vector2i(-1, -1), Vector2i( 0, -1), Vector2i( 1, -1), Vector2i( 1,  0), Vector2i( 1,  1),
	Vector2i(-2, -2), Vector2i(-1, -2), Vector2i( 0, -2), Vector2i( 1, -2), Vector2i( 2, -2), Vector2i(-2, -1), Vector2i( 2, -1), Vector2i(-2,  0),
	Vector2i( 2,  0), Vector2i(-2,  1), Vector2i( 2,  1), Vector2i(-2,  2), Vector2i(-1,  2), Vector2i( 0,  2), Vector2i( 1,  2), Vector2i( 2,  2),
};

// threading stuff
static WZ_THREAD        *fpathThread = NULL;
static WZ_MUTEX         *fpathMutex = NULL;
static WZ_SEMAPHORE     *fpathSemaphore = NULL;
static std::list<PATHJOB>    pathJobs;
static std::list<PATHRESULT> pathResults;

static bool             waitingForResult = false;
static uint32_t         waitingForResultId;
static WZ_SEMAPHORE     *waitingForResultSemaphore = NULL;

static void fpathExecute(PATHJOB *psJob, PATHRESULT *psResult);


/** This runs in a separate thread */
static int fpathThreadFunc(void *)
{
	wzMutexLock(fpathMutex);

	while (!fpathQuit)
	{
		if (pathJobs.empty())
		{
			ASSERT(!waitingForResult, "Waiting for a result (id %u) that doesn't exist.", waitingForResultId);
			wzMutexUnlock(fpathMutex);
			wzSemaphoreWait(fpathSemaphore);  // Go to sleep until needed.
			wzMutexLock(fpathMutex);
			continue;
		}

		// Copy the first job from the queue. Don't pop yet, since the main thread may want to set .deleted = true.
		PATHJOB job = pathJobs.front();

		wzMutexUnlock(fpathMutex);

		// Execute path-finding for this job using our local temporaries
		PATHRESULT result;
		result.droidID = job.droidID;
		memset(&result.sMove, 0, sizeof(result.sMove));
		result.retval = FPR_FAILED;

		fpathExecute(&job, &result);

		wzMutexLock(fpathMutex);

		ASSERT(pathJobs.front().droidID == job.droidID, "Bug");  // The front of pathJobs may have .deleted set to true, but should not otherwise have been modified or deleted.
		if (!pathJobs.front().deleted)
		{
			pathResults.push_back(result);
		}
		pathJobs.pop_front();

		// Unblock the main thread, if it was waiting for this particular result.
		if (waitingForResult && waitingForResultId == job.droidID)
		{
			waitingForResult = false;
			objTrace(waitingForResultId, "These are the droids you are looking for.");
			wzSemaphorePost(waitingForResultSemaphore);
		}
	}
	wzMutexUnlock(fpathMutex);
	return 0;
}


// initialise the findpath module
BOOL fpathInitialise(void)
{
	// The path system is up
	fpathQuit = false;

	if (!fpathThread)
	{
		fpathMutex = wzMutexCreate();
		fpathSemaphore = wzSemaphoreCreate(0);
		waitingForResultSemaphore = wzSemaphoreCreate(0);
		fpathThread = wzThreadCreate(fpathThreadFunc, NULL);
		wzThreadStart(fpathThread);
	}

	return true;
}


void fpathShutdown()
{
	// Signal the path finding thread to quit
	fpathQuit = true;
	wzSemaphorePost(fpathSemaphore);  // Wake up thread.

	fpathHardTableReset();
	if (fpathThread)
	{
		wzThreadJoin(fpathThread);
		fpathThread = NULL;
		wzMutexDestroy(fpathMutex);
		fpathMutex = NULL;
		wzSemaphoreDestroy(fpathSemaphore);
		fpathSemaphore = NULL;
		wzSemaphoreDestroy(waitingForResultSemaphore);
		waitingForResultSemaphore = NULL;
	}
}


/**
 *	Updates the pathfinding system.
 *	@ingroup pathfinding
 */
void fpathUpdate(void)
{
	// Nothing now
}


bool fpathIsEquivalentBlocking(PROPULSION_TYPE propulsion1, int player1, FPATH_MOVETYPE moveType1,
                               PROPULSION_TYPE propulsion2, int player2, FPATH_MOVETYPE moveType2)
{
	int domain1, domain2;
	switch (propulsion1)
	{
		default:                        domain1 = 0; break;  // Land
		case PROPULSION_TYPE_LIFT:      domain1 = 1; break;  // Air
		case PROPULSION_TYPE_PROPELLOR: domain1 = 2; break;  // Water
		case PROPULSION_TYPE_HOVER:     domain1 = 3; break;  // Land and water
	}
	switch (propulsion2)
	{
		default:                        domain2 = 0; break;  // Land
		case PROPULSION_TYPE_LIFT:      domain2 = 1; break;  // Air
		case PROPULSION_TYPE_PROPELLOR: domain2 = 2; break;  // Water
		case PROPULSION_TYPE_HOVER:     domain2 = 3; break;  // Land and water
	}

	if (domain1 != domain2)
	{
		return false;
	}

	if (domain1 == 1)
	{
		return true;  // Air units ignore move type and player.
	}

	if (moveType1 != moveType2 || player1 != player2)
	{
		return false;
	}

	return true;
}

static uint8_t prop2bits(PROPULSION_TYPE propulsion)
{
	uint8_t bits;

	switch (propulsion)
	{
	case PROPULSION_TYPE_LIFT:
		bits = AIR_BLOCKED;
		break;
	case PROPULSION_TYPE_HOVER:
		bits = FEATURE_BLOCKED;
		break;
	case PROPULSION_TYPE_PROPELLOR:
		bits = FEATURE_BLOCKED | LAND_BLOCKED;
		break;
	default:
		bits = FEATURE_BLOCKED | WATER_BLOCKED;
		break;
	}
	return bits;
}

// Check if the map tile at a location blocks a droid
BOOL fpathBaseBlockingTile(SDWORD x, SDWORD y, PROPULSION_TYPE propulsion, int mapIndex, FPATH_MOVETYPE moveType)
{
	uint8_t aux, unitbits = prop2bits(propulsion);	// TODO - cache prop2bits to psDroid, and pass in instead of propulsion type

	/* All tiles outside of the map and on map border are blocking. */
	if (x < 1 || y < 1 || x > mapWidth - 1 || y > mapHeight - 1)
	{
		return true;
	}

	/* Check scroll limits (used in campaign to partition the map. */
	if (propulsion != PROPULSION_TYPE_LIFT && (x < scrollMinX + 1 || y < scrollMinY + 1 || x >= scrollMaxX - 1 || y >= scrollMaxY - 1))
	{
		// coords off map - auto blocking tile
		return true;
	}
	aux = auxTile(x, y, mapIndex);

	if ((unitbits & FEATURE_BLOCKED)
	    && ((moveType == FMT_MOVE && (aux & AUXBITS_ANY_BUILDING)) // do not wish to shoot our way through enemy buildings
	        || (aux & AUXBITS_OUR_BUILDING))) // move blocked by friendly building, assuming we do not want to shoot it up en route
	{
		return true;	// move blocked by building, and we cannot or do not want to shoot our way through anything
	}

	// the MAX hack below is because blockTile() range does not include player-specific versions...
	return (blockTile(x, y, MAX(0, mapIndex - MAX_PLAYERS)) & unitbits);	// finally check if move is blocked by propulsion related factors
}

BOOL fpathDroidBlockingTile(DROID *psDroid, int x, int y, FPATH_MOVETYPE moveType)
{
	return fpathBaseBlockingTile(x, y, getPropulsionStats(psDroid)->propulsionType, psDroid->player, moveType);
}

// Check if the map tile at a location blocks a droid
BOOL fpathBlockingTile(SDWORD x, SDWORD y, PROPULSION_TYPE propulsion)
{
	return fpathBaseBlockingTile(x, y, propulsion, 0, FMT_MOVE);	// with FMT_MOVE, it is irrelevant which player is passed in
}


/** Calculate the distance to a tile from a point
 *
 *  @ingroup pathfinding
 */
static inline int fpathDistToTile(int tileX, int tileY, int pointX, int pointY)
{
	// get the difference in world coords
	int xdiff = world_coord(tileX) - pointX;
	int ydiff = world_coord(tileY) - pointY;

	return iHypot(xdiff, ydiff);
}


static void fpathSetMove(MOVE_CONTROL *psMoveCntl, SDWORD targetX, SDWORD targetY)
{
	psMoveCntl->asPath = (Vector2i *)realloc(psMoveCntl->asPath, sizeof(*psMoveCntl->asPath));
	psMoveCntl->destination = Vector2i(targetX, targetY);
	psMoveCntl->numPoints = 1;
	psMoveCntl->asPath[0] = Vector2i(targetX, targetY);
}


void fpathSetDirectRoute(DROID *psDroid, SDWORD targetX, SDWORD targetY)
{
	fpathSetMove(&psDroid->sMove, targetX, targetY);
}


void fpathRemoveDroidData(int id)
{
	wzMutexLock(fpathMutex);

	for (std::list<PATHJOB>::iterator psJob = pathJobs.begin(); psJob != pathJobs.end(); ++psJob)
	{
		if (psJob->droidID == id)
		{
			psJob->deleted = true;  // Don't delete the job, since job execution order matters, so tell it to throw away the result after executing, instead.
		}
	}
	for (std::list<PATHRESULT>::iterator psResult = pathResults.begin(); psResult != pathResults.end(); )
	{
		if (psResult->droidID == id)
		{
			psResult = pathResults.erase(psResult);
		}
		else
		{
			++psResult;
		}
	}

	wzMutexUnlock(fpathMutex);
}


static FPATH_RETVAL fpathRoute(MOVE_CONTROL *psMove, int id, int startX, int startY, int tX, int tY, PROPULSION_TYPE propulsionType, 
                               DROID_TYPE droidType, FPATH_MOVETYPE moveType, int owner, bool acceptNearest)
{
	objTrace(id, "called(*,id=%d,sx=%d,sy=%d,ex=%d,ey=%d,prop=%d,type=%d,move=%d,owner=%d)", id, startX, startY, tX, tY, (int)propulsionType, (int)droidType, (int)moveType, owner);

	// don't have to do anything if already there
	if (startX == tX && startY == tY)
	{
		// return failed to stop them moving anywhere
		objTrace(id, "Tried to move nowhere");
		syncDebug("fpathRoute(..., %d, %d, %d, %d, %d, %d, %d, %d, %d) = FPR_FAILED", id, startX, startY, tX, tY, propulsionType, droidType, moveType, owner);
		return FPR_FAILED;
	}

	// Check if waiting for a result
	while (psMove->Status == MOVEWAITROUTE)
	{
		objTrace(id, "Checking if we have a path yet");
		wzMutexLock(fpathMutex);

		// psNext should be _declared_ here, after the mutex lock! Used to be a race condition, thanks to -Wdeclaration-after-statement style pseudocompiler compatibility.
		for (std::list<PATHRESULT>::iterator psResult = pathResults.begin(); psResult != pathResults.end(); ++psResult)
		{
			if (psResult->droidID != id)
			{
				continue;  // Wrong result, try next one.
			}

			ASSERT(psResult->retval != FPR_OK || psResult->sMove.asPath, "Ok result but no path in list");

			// Copy over select fields - preserve others
			psMove->destination = psResult->sMove.destination;
			psMove->numPoints = psResult->sMove.numPoints;
			psMove->Position = 0;
			psMove->Status = MOVENAVIGATE;
			free(psMove->asPath);
			psMove->asPath = psResult->sMove.asPath;
			FPATH_RETVAL retval = psResult->retval;
			ASSERT(retval != FPR_OK || psMove->asPath, "Ok result but no path after copy");
			ASSERT(retval != FPR_OK || psMove->numPoints > 0, "Ok result but path empty after copy");

			// Remove it from the result list
			pathResults.erase(psResult);

			wzMutexUnlock(fpathMutex);

			objTrace(id, "Got a path to (%d, %d)! Length=%d Retval=%d", psMove->destination.x, psMove->destination.y, psMove->numPoints, (int)retval);
			syncDebug("fpathRoute(..., %d, %d, %d, %d, %d, %d, %d, %d, %d) = %d, path[%d] = %08X->(%d, %d)", id, startX, startY, tX, tY, propulsionType, droidType, moveType, owner, retval, psMove->numPoints, ~crcSumVector2i(0, psMove->asPath, psMove->numPoints), psMove->destination.x, psMove->destination.y);

			return retval;
		}

		objTrace(id, "No path yet. Waiting.");
		waitingForResult = true;
		waitingForResultId = id;
		wzMutexUnlock(fpathMutex);
		wzSemaphoreWait(waitingForResultSemaphore);  // keep waiting
	}

	// We were not waiting for a result, and found no trivial path, so create new job and start waiting
	PATHJOB job;
	job.origX = startX;
	job.origY = startY;
	job.droidID = id;
	job.destX = tX;
	job.destY = tY;
	job.droidType = droidType;
	job.propulsion = propulsionType;
	job.moveType = moveType;
	job.owner = owner;
	job.acceptNearest = acceptNearest;
	job.deleted = false;
	fpathSetBlockingMap(&job);

	// Clear any results or jobs waiting already. It is a vital assumption that there is only one
	// job or result for each droid in the system at any time.
	fpathRemoveDroidData(id);

	wzMutexLock(fpathMutex);

	// Add to end of list
	bool isFirstJob = pathJobs.empty();
	pathJobs.push_back(job);
	if (isFirstJob)
	{
		wzSemaphorePost(fpathSemaphore);  // Wake up processing thread.
	}

	wzMutexUnlock(fpathMutex);

	objTrace(id, "Queued up a path-finding request to (%d, %d), at least %d items earlier in queue", tX, tY, isFirstJob);
	syncDebug("fpathRoute(..., %d, %d, %d, %d, %d, %d, %d, %d, %d) = FPR_WAIT", id, startX, startY, tX, tY, propulsionType, droidType, moveType, owner);
	return FPR_WAIT;	// wait while polling result queue
}


// Find a route for an DROID to a location in world coordinates
FPATH_RETVAL fpathDroidRoute(DROID* psDroid, SDWORD tX, SDWORD tY, FPATH_MOVETYPE moveType)
{
	bool acceptNearest;
	PROPULSION_STATS *psPropStats = getPropulsionStats(psDroid);

	// override for AI to blast our way through stuff
	if (!isHumanPlayer(psDroid->player) && moveType == FMT_MOVE)
	{
		moveType = (psDroid->asWeaps[0].nStat == 0) ? FMT_MOVE : FMT_ATTACK;
	}

	ASSERT_OR_RETURN(FPR_FAILED, psPropStats != NULL, "invalid propulsion stats pointer");
	ASSERT_OR_RETURN(FPR_FAILED, psDroid->type == OBJ_DROID, "We got passed an object that isn't a DROID!");

	// check whether the end point of the route
	// is a blocking tile and find an alternative if it is
	if (psDroid->sMove.Status != MOVEWAITROUTE && fpathDroidBlockingTile(psDroid, map_coord(tX), map_coord(tY), moveType))
	{
		// find the nearest non blocking tile to the DROID
		int minDist = SDWORD_MAX;
		int nearestDir = NUM_DIR;
		int dir;

		objTrace(psDroid->id, "BLOCKED(%d,%d) - trying workaround", map_coord(tX), map_coord(tY));
		for (dir = 0; dir < NUM_DIR; dir++)
		{
			int x = map_coord(tX) + aDirOffset[dir].x;
			int y = map_coord(tY) + aDirOffset[dir].y;

			if (tileOnMap(x, y) && !fpathDroidBlockingTile(psDroid, x, y, moveType))
			{
				// pick the adjacent tile closest to our starting point
				int tileDist = fpathDistToTile(x, y, psDroid->pos.x, psDroid->pos.y);

				if (tileDist < minDist)
				{
					minDist = tileDist;
					nearestDir = dir;
				}
			}

			if (dir == NUM_BASIC - 1 && nearestDir != NUM_DIR)
			{
				break;	// found a solution without checking at greater distance
			}
		}

		if (nearestDir == NUM_DIR)
		{
			// surrounded by blocking tiles, give up
			objTrace(psDroid->id, "route to (%d, %d) failed - target blocked", map_coord(tX), map_coord(tY));
			return FPR_FAILED;
		}
		else
		{
			tX = world_coord(map_coord(tX) + aDirOffset[nearestDir].x) + TILE_UNITS / 2;
			tY = world_coord(map_coord(tY) + aDirOffset[nearestDir].y) + TILE_UNITS / 2;
			objTrace(psDroid->id, "Workaround found at (%d, %d)", map_coord(tX), map_coord(tY));
		}
	}
	switch (psDroid->order)
	{
	case DORDER_BUILD:
	case DORDER_HELPBUILD:                       // help to build a structure
	case DORDER_LINEBUILD:                       // 6 - build a number of structures in a row (walls + bridges)
	case DORDER_DEMOLISH:                        // demolish a structure
	case DORDER_REPAIR:
		acceptNearest = false;
		break;
	default:
		acceptNearest = true;
		break;
	}
	return fpathRoute(&psDroid->sMove, psDroid->id, psDroid->pos.x, psDroid->pos.y, tX, tY, psPropStats->propulsionType, 
	                  psDroid->droidType, moveType, psDroid->player, acceptNearest);
}

// Run only from path thread
static void fpathExecute(PATHJOB *psJob, PATHRESULT *psResult)
{
	ASR_RETVAL retval = fpathAStarRoute(&psResult->sMove, psJob);

	ASSERT(retval != ASR_OK || psResult->sMove.asPath, "Ok result but no path in result");
	ASSERT(retval == ASR_FAILED || psResult->sMove.numPoints > 0, "Ok result but no length of path in result");
	switch (retval)
	{
	case ASR_NEAREST:
		if (psJob->acceptNearest)
		{
			objTrace(psJob->droidID, "** Nearest route -- accepted **");
			psResult->retval = FPR_OK;
		}
		else
		{
			objTrace(psJob->droidID, "** Nearest route -- rejected **");
			psResult->retval = FPR_FAILED;
		}
		break;
	case ASR_FAILED:
		objTrace(psJob->droidID, "** Failed route **");
		// Is this really a good idea? Was in original code.
		if (psJob->propulsion == PROPULSION_TYPE_LIFT && psJob->droidType != DROID_TRANSPORTER)
		{
			objTrace(psJob->droidID, "Doing fallback for non-transport VTOL");
			fpathSetMove(&psResult->sMove, psJob->destX, psJob->destY);
			psResult->retval = FPR_OK;
		}
		else
		{
			psResult->retval = FPR_FAILED;
		}
		break;
	case ASR_OK:
		objTrace(psJob->droidID, "Got route of length %d", psResult->sMove.numPoints);
		psResult->retval = FPR_OK;
		break;
	}
}

// Variables for the callback
static BOOL		obstruction;

/** The visibility ray callback
 */
static bool fpathVisCallback(Vector3i pos, int32_t dist, void *data)
{
	DROID *psDroid = (DROID *)data;

	if (fpathBlockingTile(map_coord(pos.x), map_coord(pos.y), getPropulsionStats(psDroid)->propulsionType))
	{
		// found an obstruction
		obstruction = true;
		return false;
	}

	return true;
}

BOOL fpathTileLOS(DROID *psDroid, Vector3i dest)
{
	Vector2i dir = removeZ(dest - psDroid->pos);

	// Initialise the callback variables
	obstruction = false;

	rayCast(psDroid->pos, iAtan2(dir), iHypot(dir), fpathVisCallback, psDroid);

	return !obstruction;
}

/** Find the length of the job queue. Function is thread-safe. */
static int fpathJobQueueLength(void)
{
	int count = 0;

	wzMutexLock(fpathMutex);
	count = pathJobs.size();  // O(N) function call for std::list. .empty() is faster, but this function isn't used except in tests.
	wzMutexUnlock(fpathMutex);
	return count;
}


/** Find the length of the result queue, excepting future results. Function is thread-safe. */
static int fpathResultQueueLength(void)
{
	int count = 0;

	wzMutexLock(fpathMutex);
	count = pathResults.size();  // O(N) function call for std::list. .empty() is faster, but this function isn't used except in tests.
	wzMutexUnlock(fpathMutex);
	return count;
}


static FPATH_RETVAL fpathSimpleRoute(MOVE_CONTROL *psMove, int id, int startX, int startY, int tX, int tY)
{
	return fpathRoute(psMove, id, startX, startY, tX, tY, PROPULSION_TYPE_WHEELED, DROID_WEAPON, FMT_MOVE, 0, true);
}

void fpathTest(int x, int y, int x2, int y2)
{
	MOVE_CONTROL sMove;
	FPATH_RETVAL r;
	int i;

	// On non-debug builds prevent warnings about defining but not using fpathJobQueueLength
	(void)fpathJobQueueLength;

	/* Check initial state */
	assert(fpathThread != NULL);
	assert(fpathMutex != NULL);
	assert(fpathSemaphore != NULL);
	assert(pathJobs.empty());
	assert(pathResults.empty());
	fpathRemoveDroidData(0);	// should not crash

	/* This should not leak memory */
	sMove.asPath = NULL;
	for (i = 0; i < 100; i++) fpathSetMove(&sMove, 1, 1);
	free(sMove.asPath);
	sMove.asPath = NULL;

	/* Test one path */
	sMove.Status = MOVEINACTIVE;
	r = fpathSimpleRoute(&sMove, 1, x, y, x2, y2);
	assert(r == FPR_WAIT);
	sMove.Status = MOVEWAITROUTE;
	assert(fpathJobQueueLength() == 1 || fpathResultQueueLength() == 1);
	fpathRemoveDroidData(2);	// should not crash, nor remove our path
	assert(fpathJobQueueLength() == 1 || fpathResultQueueLength() == 1);
	while (fpathResultQueueLength() == 0) wzYieldCurrentThread();
	assert(fpathJobQueueLength() == 0);
	assert(fpathResultQueueLength() == 1);
	r = fpathSimpleRoute(&sMove, 1, x, y, x2, y2);
	assert(r == FPR_OK);
	assert(sMove.numPoints > 0 && sMove.asPath);
	assert(sMove.asPath[sMove.numPoints - 1].x == x2);
	assert(sMove.asPath[sMove.numPoints - 1].y == y2);
	assert(fpathResultQueueLength() == 0);

	/* Let one hundred paths flower! */
	sMove.Status = MOVEINACTIVE;
	for (i = 1; i <= 100; i++)
	{
		r = fpathSimpleRoute(&sMove, i, x, y, x2, y2);
		assert(r == FPR_WAIT);
	}
	while (fpathResultQueueLength() != 100) wzYieldCurrentThread();
	assert(fpathJobQueueLength() == 0);
	for (i = 1; i <= 100; i++)
	{
		sMove.Status = MOVEWAITROUTE;
		r = fpathSimpleRoute(&sMove, i, x, y, x2, y2);
		assert(r == FPR_OK);
		assert(sMove.numPoints > 0 && sMove.asPath);
		assert(sMove.asPath[sMove.numPoints - 1].x == x2);
		assert(sMove.asPath[sMove.numPoints - 1].y == y2);
	}
	assert(fpathResultQueueLength() == 0);

	/* Kill a hundred flowers */
	sMove.Status = MOVEINACTIVE;
	for (i = 1; i <= 100; i++)
	{
		r = fpathSimpleRoute(&sMove, i, x, y, x2, y2);
		assert(r == FPR_WAIT);
	}
	for (i = 1; i <= 100; i++)
	{
		fpathRemoveDroidData(i);
	}
	//assert(pathJobs.empty()); // can now be marked .deleted as well
	assert(pathResults.empty());
}

bool fpathCheck(Position orig, Position dest, PROPULSION_TYPE propulsion)
{
	MAPTILE *origTile;
	MAPTILE *destTile;

	// We have to be careful with this check because it is called on
	// load when playing campaign on droids that are on the other
	// map during missions, and those maps are usually larger.
	if (!worldOnMap(orig.x, orig.y) || !worldOnMap(dest.x, dest.y))
	{
		return false;
	}

	origTile = worldTile(orig.x, orig.y);
	destTile = worldTile(dest.x, dest.y);

	ASSERT(propulsion != PROPULSION_TYPE_NUM, "Bad propulsion type");
	ASSERT(origTile != NULL && destTile != NULL, "Bad tile parameter");

	switch (propulsion)
	{
	case PROPULSION_TYPE_PROPELLOR:
	case PROPULSION_TYPE_WHEELED:
	case PROPULSION_TYPE_TRACKED:
	case PROPULSION_TYPE_LEGGED:
	case PROPULSION_TYPE_SKI: 	// ?!
	case PROPULSION_TYPE_HALF_TRACKED:
		return origTile->limitedContinent == destTile->limitedContinent;
	case PROPULSION_TYPE_HOVER:
		return origTile->hoverContinent == destTile->hoverContinent;
	case PROPULSION_TYPE_JUMP:
	case PROPULSION_TYPE_LIFT:
		return true;	// FIXME: This is not entirely correct for all possible maps. - Per
	case PROPULSION_TYPE_NUM:
		break;
	}
	return true;	// should never get here
}
