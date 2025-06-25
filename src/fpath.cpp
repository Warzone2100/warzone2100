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
 * @file fpath.c
 *
 * Interface to the routing functions.
 *
 */

#include <future>
#include <unordered_map>

#include "lib/framework/frame.h"
#include "lib/framework/crc.h"
#include "lib/netplay/sync_debug.h"

#include "lib/framework/wzapp.h"

#include "objects.h"
#include "map.h"
#include "multiplay.h"
#include "astar.h"

#include "fpath.h"
#include "profiling.h"

// If the path finding system is shutdown or not
static volatile bool fpathQuit = false;

/* Beware: Enabling this will cause significant slow-down. */
#undef DEBUG_MAP

struct PATHRESULT
{
	UDWORD		droidID;	///< Unique droid ID.
	MOVE_CONTROL	sMove;		///< New movement values for the droid.
	FPATH_RETVAL	retval;		///< Result value from path-finding.
	Vector2i        originalDest;   ///< Used to check if the pathfinding job is to the right destination.
};


// threading stuff
using packagedPathJob = wz::packaged_task<PATHRESULT(const std::shared_ptr<FPathExecuteContext>& ctx)>;

struct FpathThreadInfo
{
public:
	FpathThreadInfo()
	{
		mutex = wzMutexCreate();
		semaphore = wzSemaphoreCreate(0);
	}

	~FpathThreadInfo()
	{
		wzMutexDestroy(mutex);
		mutex = nullptr;
		wzSemaphoreDestroy(semaphore);
		semaphore = nullptr;
	}

	FpathThreadInfo(FpathThreadInfo&&) = delete;
	FpathThreadInfo& operator=(FpathThreadInfo&&) = delete;
	FpathThreadInfo(const FpathThreadInfo&) = delete;
	FpathThreadInfo& operator=(const FpathThreadInfo&) = delete;
public:
	WZ_SEMAPHORE *semaphore;
	WZ_MUTEX *mutex;
	std::list<packagedPathJob> pathJobs;
};

static std::vector<WZ_THREAD *> fpathThreads;
static std::vector<std::unique_ptr<FpathThreadInfo>> fpathThreadsInfo;
static std::unordered_map<uint32_t, wz::future<PATHRESULT>> pathResults;

#ifdef DEBUG
static std::vector<size_t> numJobsPerThreadThisTick;
static uint32_t currentFpathTick = 0;
#endif

constexpr size_t MAX_FPATH_THREADS = 2;

static PATHRESULT fpathExecute(const std::shared_ptr<FPathExecuteContext>& ctx, PATHJOB psJob);


/** This runs in a separate thread */
static int fpathThreadFunc(void *data)
{
	FpathThreadInfo* threadInfo = static_cast<FpathThreadInfo*>(data);
	WZ_SEMAPHORE *fpathSemaphore = threadInfo->semaphore;
	WZ_MUTEX *fpathMutex = threadInfo->mutex;
	std::list<packagedPathJob>& pathJobs = threadInfo->pathJobs;

	// create an fpath astar job context
	auto ctx = makeFPathExecuteContext();

	while (true)
	{
		wzSemaphoreWait(fpathSemaphore);  // Wait until needed.
		wzMutexLock(fpathMutex);

		if (fpathQuit)
		{
			wzMutexUnlock(fpathMutex);
			break;
		}

		if (pathJobs.empty())
		{
			// should not happen - semaphore was signaled!
			ASSERT(!pathJobs.empty(), "Received a signal but no job to consume!");
			wzMutexUnlock(fpathMutex);
			continue;

		}

		WZ_PROFILE_SCOPE(fpathJob);
		// Copy the first job from the queue.
		packagedPathJob job = std::move(pathJobs.front());
		pathJobs.pop_front();

		wzMutexUnlock(fpathMutex);

		job(ctx);
	}
	return 0;
}

static size_t fpathDetermineNumberOfThreads()
{
	auto logicalCPUCount = wzGetLogicalCPUCount();
	if (logicalCPUCount <= 1)
	{
		return 1;
	}
	// subtract one for the main thread
	return std::min<size_t>(logicalCPUCount - 1, MAX_FPATH_THREADS);
}

// initialise the findpath module
bool fpathInitialise()
{
	// The path system is up
	fpathQuit = false;

	if (fpathThreads.empty())
	{
		auto numThreads = fpathDetermineNumberOfThreads();
		debug(LOG_INFO, "Using threads: %zu", numThreads);
		fpathThreads.resize(numThreads, nullptr);
		fpathThreadsInfo.resize(numThreads);
#ifdef DEBUG
		numJobsPerThreadThisTick.resize(numThreads);
#endif
		for (size_t i = 0; i < fpathThreads.size(); ++i)
		{
			fpathThreadsInfo[i] = std::make_unique<FpathThreadInfo>();
			fpathThreads[i] = wzThreadCreate(fpathThreadFunc, fpathThreadsInfo[i].get(), "wzPath");
			wzThreadStart(fpathThreads[i]);
		}
	}

	return true;
}


void fpathShutdown()
{
	if (!fpathThreads.empty())
	{
		// Signal the path finding thread(s) to quit
		fpathQuit = true;
		for (size_t i = 0; i < fpathThreadsInfo.size(); ++i)
		{
			wzSemaphorePost(fpathThreadsInfo[i]->semaphore);  // Wake up a thread
		}
		for (size_t i = 0; i < fpathThreads.size(); ++i)
		{
			wzThreadJoin(fpathThreads[i]);
		}
		fpathThreads.clear();
		fpathThreadsInfo.clear();

#ifdef DEBUG
		numJobsPerThreadThisTick.clear();
		currentFpathTick = 0;
#endif
	}
	fpathHardTableReset();
}


/**
 *	Updates the pathfinding system.
 *	@ingroup pathfinding
 */
void fpathUpdate()
{
	// Nothing now
}

static constexpr size_t fpathPropulsionDomain(PROPULSION_TYPE propulsion)
{
	switch (propulsion)
	{
	default:                        return 0;  // Land
	case PROPULSION_TYPE_LIFT:      return 1;  // Air
	case PROPULSION_TYPE_PROPELLOR: return 2;  // Water
	case PROPULSION_TYPE_HOVER:     return 3;  // Land and water
	}
	return 0; // silence compiler warning
}

inline void hash_combine(std::size_t& seed) { }

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
	std::hash<T> hasher;
#if SIZE_MAX >= UINT64_MAX
	seed ^= hasher(v) + 0x9e3779b97f4a7c15L + (seed<<6) + (seed>>2);
#else
	seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
#endif
	hash_combine(seed, rest...);
}

static inline size_t fpathJobDispatchThreadId(const PATHJOB& job, size_t numThreads)
{
	if (numThreads == 1) { return 0; }

	// Every job that matches a PathfindContext must be processed by the same thread, as the result of fpathAStarRoute is dependent upon jobs
	// within each matching "cohort" having access to the same PathfindContext (and PathfindContexts are not shared between threads).
	//
	// (In other words, the results may slightly differ depending on whether an existing PathfindContext is reused versus starting from scratch.)

	std::size_t h = 0;
	auto domain = fpathPropulsionDomain(job.propulsion);
	const Vector2i tileDest(map_coord(job.destX), map_coord(job.destY));

	// Note: We use part of the behavior of PathfindContext::matches() (which is called by fpathAStarRoute)
	// Specifically, we match using the same logic as fpathIsEquivalentBlocking, plus tileDest
	if (domain == fpathPropulsionDomain(PROPULSION_TYPE_LIFT))
	{
		// Air units ignore move type and player (see: fpathIsEquivalentBlocking)
		// So just use the domain + tileDest
		hash_combine(h, domain, tileDest.x, tileDest.y);
	}
	else
	{
		// All other unit types care about domain + player + moveType (see: fpathIsEquivalentBlocking)
		// So use those + tileDest
		hash_combine(h, domain, job.owner, job.moveType, tileDest.x, tileDest.y);
	}
	return h % numThreads;
}

bool fpathIsEquivalentBlocking(PROPULSION_TYPE propulsion1, int player1, FPATH_MOVETYPE moveType1,
                               PROPULSION_TYPE propulsion2, int player2, FPATH_MOVETYPE moveType2)
{
	auto domain1 = fpathPropulsionDomain(propulsion1);
	auto domain2 = fpathPropulsionDomain(propulsion2);

	if (domain1 != domain2)
	{
		return false;
	}

	if (domain1 == fpathPropulsionDomain(PROPULSION_TYPE_LIFT))
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
bool fpathBaseBlockingTile(SDWORD x, SDWORD y, PROPULSION_TYPE propulsion, int mapIndex, FPATH_MOVETYPE moveType)
{
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
	unsigned aux = auxTile(x, y, mapIndex);

	int auxMask = 0;
	switch (moveType)
	{
	case FMT_MOVE:   auxMask = AUXBITS_NONPASSABLE; break;   // do not wish to shoot our way through enemy buildings, but want to go through friendly gates (without shooting them)
	case FMT_ATTACK: auxMask = AUXBITS_OUR_BUILDING; break;  // move blocked by friendly building, assuming we do not want to shoot it up en route
	case FMT_BLOCK:  auxMask = AUXBITS_BLOCKING; break;      // Do not wish to tunnel through closed gates or buildings.
	}

	unsigned unitbits = prop2bits(propulsion);  // TODO - cache prop2bits to psDroid, and pass in instead of propulsion type
	if ((unitbits & FEATURE_BLOCKED) != 0 && (aux & auxMask) != 0)
	{
		return true;	// move blocked by building, and we cannot or do not want to shoot our way through anything
	}

	// the MAX hack below is because blockTile() range does not include player-specific versions...
	return (blockTile(x, y, MAX(0, mapIndex - MAX_PLAYERS)) & unitbits) != 0;  // finally check if move is blocked by propulsion related factors
}

bool fpathDroidBlockingTile(DROID *psDroid, int x, int y, FPATH_MOVETYPE moveType)
{
	return fpathBaseBlockingTile(x, y, psDroid->getPropulsionStats()->propulsionType, psDroid->player, moveType);
}

// Check if the map tile at a location blocks a droid
bool fpathBlockingTile(SDWORD x, SDWORD y, PROPULSION_TYPE propulsion)
{
	return fpathBaseBlockingTile(x, y, propulsion, 0, FMT_BLOCK);  // with FMT_BLOCK, it is irrelevant which player is passed in
}


// Returns the closest non-blocking tile to pos, or returns pos if no non-blocking tiles are present within a 2 tile distance.
static Position findNonblockingPosition(Position pos, PROPULSION_TYPE propulsion, int player = 0, FPATH_MOVETYPE moveType = FMT_BLOCK)
{
	Vector2i centreTile = map_coord(pos.xy());
	if (!fpathBaseBlockingTile(centreTile.x, centreTile.y, propulsion, player, moveType))
	{
		return pos;  // Fast case, pos is not on a blocking tile.
	}

	Vector2i bestTile = centreTile;
	int bestDistSq = INT32_MAX;
	for (int y = -2; y <= 2; ++y)
		for (int x = -2; x <= 2; ++x)
		{
			Vector2i tile = centreTile + Vector2i(x, y);
			Vector2i diff = world_coord(tile) + Vector2i(TILE_UNITS / 2, TILE_UNITS / 2) - pos.xy();
			int distSq = dot(diff, diff);
			if (distSq < bestDistSq && !fpathBaseBlockingTile(tile.x, tile.y, propulsion, player, moveType))
			{
				bestTile = tile;
				bestDistSq = distSq;
			}
		}

	// Return point on tile closest to the original pos.
	Vector2i minCoord = world_coord(bestTile);
	Vector2i maxCoord = minCoord + Vector2i(TILE_UNITS - 1, TILE_UNITS - 1);

	return Position(std::min(std::max(pos.x, minCoord.x), maxCoord.x), std::min(std::max(pos.y, minCoord.y), maxCoord.y), pos.z);
}



static void fpathSetMove(MOVE_CONTROL *psMoveCntl, SDWORD targetX, SDWORD targetY)
{
	psMoveCntl->asPath.resize(1);
	psMoveCntl->destination = Vector2i(targetX, targetY);
	psMoveCntl->asPath[0] = Vector2i(targetX, targetY);
}


void fpathSetDirectRoute(DROID *psDroid, SDWORD targetX, SDWORD targetY)
{
	fpathSetMove(&psDroid->sMove, targetX, targetY);
}


void fpathRemoveDroidData(int id)
{
	pathResults.erase(id);
}

static FPATH_RETVAL fpathRoute(MOVE_CONTROL *psMove, unsigned id, int startX, int startY, int tX, int tY, PROPULSION_TYPE propulsionType,
                               DROID_TYPE droidType, FPATH_MOVETYPE moveType, int owner, bool acceptNearest, StructureBounds const &dstStructure)
{
	objTrace(id, "called(*,id=%d,sx=%d,sy=%d,ex=%d,ey=%d,prop=%d,type=%d,move=%d,owner=%d)", id, startX, startY, tX, tY, (int)propulsionType, (int)droidType, (int)moveType, owner);

#ifdef DEBUG
	if (gameTime != currentFpathTick)
	{
		if (enabled_debug[currentFpathTick])
		{
			static std::string tmpDgbStr;
			tmpDgbStr = "Last tick fpath jobs per thread:";
			for (const auto& c : numJobsPerThreadThisTick)
			{
				tmpDgbStr += " " + std::to_string(c) + ",";
			}
			debug(LOG_MOVEMENT, "%s", tmpDgbStr.c_str());
		}
		currentFpathTick = gameTime;
		for (auto& c : numJobsPerThreadThisTick)
		{
			c = 0;
		}
	}
#endif

	if (!worldOnMap(startX, startY) || !worldOnMap(tX, tY))
	{
		debug(LOG_ERROR, "Droid trying to find path to/from invalid location (%d %d) -> (%d %d).", startX, startY, tX, tY);
		objTrace(id, "Invalid start/end.");
		syncDebug("fpathRoute(..., %d, %d, %d, %d, %d, %d, %d, %d, %d) = FPR_FAILED", id, startX, startY, tX, tY, propulsionType, droidType, moveType, owner);
		return FPR_FAILED;
	}

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

		auto const I = pathResults.find(id);
		ASSERT_OR_RETURN(FPR_FAILED, I != pathResults.end(), "Missing path result promise");
		PATHRESULT result = I->second.get();
		ASSERT(result.retval != FPR_OK || result.sMove.asPath.size() > 0, "Ok result but no path in list");

		// Copy over select fields - preserve others
		psMove->destination = result.sMove.destination;
		bool correctDestination = tX == result.originalDest.x && tY == result.originalDest.y;
		psMove->pathIndex = 0;
		psMove->Status = MOVENAVIGATE;
		psMove->asPath = result.sMove.asPath;
		FPATH_RETVAL retval = result.retval;
		ASSERT(retval != FPR_OK || psMove->asPath.size() > 0, "Ok result but no path after copy");

		// Remove it from the result list
		pathResults.erase(id);

		objTrace(id, "Got a path to (%d, %d)! Length=%d Retval=%d", psMove->destination.x, psMove->destination.y, (int)psMove->asPath.size(), (int)retval);
		syncDebug("fpathRoute(..., %d, %d, %d, %d, %d, %d, %d, %d, %d) = %d, path[%d] = %08X->(%d, %d)", id, startX, startY, tX, tY, propulsionType, droidType, moveType, owner, retval, (int)psMove->asPath.size(), ~crcSumVector2i(0, psMove->asPath.data(), psMove->asPath.size()), psMove->destination.x, psMove->destination.y);

		if (!correctDestination)
		{
			goto queuePathfinding;  // Seems we got the result of an old pathfinding job for this droid, so need to pathfind again.
		}

		return retval;
	}
queuePathfinding:

	// We were not waiting for a result, and found no trivial path, so create new job and start waiting
	PATHJOB job;
	job.origX = startX;
	job.origY = startY;
	job.droidID = id;
	job.destX = tX;
	job.destY = tY;
	job.dstStructure = dstStructure;
	job.droidType = droidType;
	job.propulsion = propulsionType;
	job.moveType = moveType;
	job.owner = owner;
	job.acceptNearest = acceptNearest;
	job.deleted = false;
	fpathSetBlockingMap(&job);

	debug(LOG_NEVER, "starting new job for droid %d 0x%x", id, id);
	// Clear any results or jobs waiting already. It is a vital assumption that there is only one
	// job or result for each droid in the system at any time.
	fpathRemoveDroidData(id);

	packagedPathJob task([job](const std::shared_ptr<FPathExecuteContext>& ctx) { return fpathExecute(ctx, job); });
	pathResults[id] = task.get_future();

	// Get target thread for job
	auto targetThreadId = fpathJobDispatchThreadId(job, fpathThreads.size());
	auto& threadInfo = *fpathThreadsInfo[targetThreadId];

	// Add to end of appropriate list
	wzMutexLock(threadInfo.mutex);
	bool isFirstJob = threadInfo.pathJobs.empty();
	threadInfo.pathJobs.push_back(std::move(task));
	wzMutexUnlock(threadInfo.mutex);

	wzSemaphorePost(threadInfo.semaphore);  // Increment semaphore

#ifdef DEBUG
	numJobsPerThreadThisTick[targetThreadId]++;
#endif

	objTrace(id, "Queued up a path-finding request to (%d, %d), at least %d items earlier in queue", tX, tY, isFirstJob);
	syncDebug("fpathRoute(..., %d, %d, %d, %d, %d, %d, %d, %d, %d) = FPR_WAIT", id, startX, startY, tX, tY, propulsionType, droidType, moveType, owner);
	return FPR_WAIT;	// wait while polling result queue
}


// Find a route for an DROID to a location in world coordinates
FPATH_RETVAL fpathDroidRoute(DROID *psDroid, SDWORD tX, SDWORD tY, FPATH_MOVETYPE moveType)
{
	bool acceptNearest;
	PROPULSION_STATS *psPropStats = psDroid->getPropulsionStats();

	// override for AI to blast our way through stuff
	if (!isHumanPlayer(psDroid->player) && moveType == FMT_MOVE)
	{
		moveType = (psDroid->asWeaps[0].nStat == 0) ? FMT_MOVE : FMT_ATTACK;
	}

	ASSERT_OR_RETURN(FPR_FAILED, psPropStats != nullptr, "invalid propulsion stats pointer");
	ASSERT_OR_RETURN(FPR_FAILED, psDroid->type == OBJ_DROID, "We got passed an object that isn't a DROID!");

	// Check whether the start and end points of the route are blocking tiles and find an alternative if they are.
	Position startPos = psDroid->pos;
	Position endPos = Position(tX, tY, 0);
	StructureBounds dstStructure = getStructureBounds(worldTile(endPos.xy())->psObject);
	const auto droidPropulsionType = psDroid->getPropulsionStats()->propulsionType;
	startPos = findNonblockingPosition(startPos, droidPropulsionType, psDroid->player, moveType);
	if (!dstStructure.valid())  // If there's a structure over the destination, ignore it, otherwise pathfind from somewhere around the obstruction.
	{
		endPos   = findNonblockingPosition(endPos, droidPropulsionType, psDroid->player, moveType);
	}
	objTrace(psDroid->id, "Want to go to (%d, %d) -> (%d, %d), going (%d, %d) -> (%d, %d)", map_coord(psDroid->pos.x), map_coord(psDroid->pos.y), map_coord(tX), map_coord(tY), map_coord(startPos.x), map_coord(startPos.y), map_coord(endPos.x), map_coord(endPos.y));
	switch (psDroid->order.type)
	{
	case DORDER_BUILD:
	case DORDER_LINEBUILD:                       // build a number of structures in a row (walls + bridges)
		dstStructure = getStructureBounds(psDroid->order.psStats, psDroid->order.pos, psDroid->order.direction);  // Just need to get close enough to build (can be diagonally), do not need to reach the destination tile.
		// fallthrough
	case DORDER_HELPBUILD:                       // help to build a structure
	case DORDER_DEMOLISH:                        // demolish a structure
	case DORDER_REPAIR:
		acceptNearest = false;
		break;
	default:
		acceptNearest = true;
		break;
	}
	return fpathRoute(&psDroid->sMove, psDroid->id, startPos.x, startPos.y, endPos.x, endPos.y, psPropStats->propulsionType,
	                  psDroid->droidType, moveType, psDroid->player, acceptNearest, dstStructure);
}

// Run only from path thread
PATHRESULT fpathExecute(const std::shared_ptr<FPathExecuteContext>& ctx, PATHJOB job)
{
	PATHRESULT result;
	result.droidID = job.droidID;
	result.retval = FPR_FAILED;
	result.originalDest = Vector2i(job.destX, job.destY);

	ASR_RETVAL retval = fpathAStarRoute(ctx, &result.sMove, &job);

	ASSERT(retval != ASR_OK || result.sMove.asPath.size() > 0, "Ok result but no path in result");
	switch (retval)
	{
	case ASR_NEAREST:
		if (job.acceptNearest)
		{
			objTrace(job.droidID, "** Nearest route -- accepted **");
			result.retval = FPR_OK;
		}
		else
		{
			objTrace(job.droidID, "** Nearest route -- rejected **");
			result.retval = FPR_FAILED;
		}
		break;
	case ASR_FAILED:
		objTrace(job.droidID, "** Failed route **");
		// Is this really a good idea? Was in original code.
		if (job.propulsion == PROPULSION_TYPE_LIFT && (job.droidType != DROID_TRANSPORTER && job.droidType != DROID_SUPERTRANSPORTER))
		{
			objTrace(job.droidID, "Doing fallback for non-transport VTOL");
			fpathSetMove(&result.sMove, job.destX, job.destY);
			result.retval = FPR_OK;
		}
		else
		{
			result.retval = FPR_FAILED;
		}
		break;
	case ASR_OK:
		objTrace(job.droidID, "Got route of length %d", (int)result.sMove.asPath.size());
		result.retval = FPR_OK;
		break;
	}
	return result;
}

/** Find the length of the job queue. Function is thread-safe. */
static size_t fpathJobQueueLength()
{
	size_t count = 0;

	for (const auto& threadInfo : fpathThreadsInfo)
	{
		wzMutexLock(threadInfo->mutex);
		count += threadInfo->pathJobs.size(); // O(N) function call for std::list. .empty() is faster, but this function isn't used except in tests.
		wzMutexUnlock(threadInfo->mutex);
	}
	return count;
}


/** Find the length of the result queue, excepting future results. Function must be called from the main thread.. */
static size_t fpathResultQueueLength()
{
	size_t count = 0;

	count = pathResults.size();  // O(N) function call for std::list. .empty() is faster, but this function isn't used except in tests.
	return count;
}


// Only used by fpathTest.
static FPATH_RETVAL fpathSimpleRoute(MOVE_CONTROL *psMove, int id, int startX, int startY, int tX, int tY)
{
	return fpathRoute(psMove, id, startX, startY, tX, tY, PROPULSION_TYPE_WHEELED, DROID_WEAPON, FMT_BLOCK, 0, true, getStructureBounds((BASE_OBJECT *)nullptr));
}

void fpathTest(int x, int y, int x2, int y2)
{
	MOVE_CONTROL sMove;
	FPATH_RETVAL r;
	int i;

	// On non-debug builds prevent warnings about defining but not using fpathJobQueueLength
	(void)fpathJobQueueLength();

	/* Check initial state */
	assert(!fpathThreads.empty());
	for (const auto& threadInfo : fpathThreadsInfo)
	{
		ASSERT(threadInfo->mutex != nullptr, "Failed to initialize mutex?");
		ASSERT(threadInfo->semaphore != nullptr, "Failed to initialize semaphore?");
	}
	assert(fpathJobQueueLength() == 0);
	assert(pathResults.empty());
	fpathRemoveDroidData(0);	// should not crash

	/* This should not leak memory */
	sMove.asPath.clear();
	for (i = 0; i < 100; i++)
	{
		fpathSetMove(&sMove, 1, 1);
	}

	/* Test one path */
	sMove.Status = MOVEINACTIVE;
	r = fpathSimpleRoute(&sMove, 1, x, y, x2, y2);
	assert(r == FPR_WAIT);
	sMove.Status = MOVEWAITROUTE;
	assert(fpathJobQueueLength() == 1 || fpathResultQueueLength() == 1);
	fpathRemoveDroidData(2);	// should not crash, nor remove our path
	assert(fpathJobQueueLength() == 1 || fpathResultQueueLength() == 1);
	while (fpathResultQueueLength() == 0)
	{
		wzYieldCurrentThread();
	}
	assert(fpathJobQueueLength() == 0);
	assert(fpathResultQueueLength() == 1);
	r = fpathSimpleRoute(&sMove, 1, x, y, x2, y2);
	assert(r == FPR_OK);
	assert(sMove.asPath.size() > 0);
	assert(sMove.asPath[sMove.asPath.size() - 1].x == x2);
	assert(sMove.asPath[sMove.asPath.size() - 1].y == y2);
	assert(fpathResultQueueLength() == 0);

	/* Let one hundred paths flower! */
	sMove.Status = MOVEINACTIVE;
	for (i = 1; i <= 100; i++)
	{
		r = fpathSimpleRoute(&sMove, i, x, y, x2, y2);
		assert(r == FPR_WAIT);
	}
	while (fpathResultQueueLength() != 100)
	{
		wzYieldCurrentThread();
	}
	assert(fpathJobQueueLength() == 0);
	for (i = 1; i <= 100; i++)
	{
		sMove.Status = MOVEWAITROUTE;
		r = fpathSimpleRoute(&sMove, i, x, y, x2, y2);
		assert(r == FPR_OK);
		assert(sMove.asPath.size() > 0 && sMove.asPath.size() > 0);
		assert(sMove.asPath[sMove.asPath.size() - 1].x == x2);
		assert(sMove.asPath[sMove.asPath.size() - 1].y == y2);
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
	(void)r;  // Squelch unused-but-set warning.
}

bool fpathCheck(Position orig, Position dest, PROPULSION_TYPE propulsion)
{
	// We have to be careful with this check because it is called on
	// load when playing campaign on droids that are on the other
	// map during missions, and those maps are usually larger.
	if (!worldOnMap(orig.xy()) || !worldOnMap(dest.xy()))
	{
		return false;
	}

	MAPTILE *origTile = worldTile(findNonblockingPosition(orig, propulsion).xy());
	MAPTILE *destTile = worldTile(findNonblockingPosition(dest, propulsion).xy());

	ASSERT_OR_RETURN(false, propulsion != PROPULSION_TYPE_NUM, "Bad propulsion type");
	ASSERT_OR_RETURN(false, origTile != nullptr && destTile != nullptr, "Bad tile parameter");

	switch (propulsion)
	{
	case PROPULSION_TYPE_PROPELLOR:
	case PROPULSION_TYPE_WHEELED:
	case PROPULSION_TYPE_TRACKED:
	case PROPULSION_TYPE_LEGGED:
	case PROPULSION_TYPE_HALF_TRACKED:
		return origTile->limitedContinent == destTile->limitedContinent;
	case PROPULSION_TYPE_HOVER:
		return origTile->hoverContinent == destTile->hoverContinent;
	case PROPULSION_TYPE_LIFT:
		return true;	// assume no map uses skyscrapers to isolate areas
	default:
		break;
	}

	ASSERT(false, "Should never get here, unknown propulsion !");
	return false;	// should never get here
}
