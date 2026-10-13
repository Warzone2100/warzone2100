// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#include "gamestate_harness.h"
#include "gamestate_serialize.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/sync_debug.h"
#include "lib/netplay/nettypes.h"

#include "game.h"
#include "game_world.h"
#include "map.h"
#include "objmem.h"
#include "mission.h"
#include "multiplay.h"  // captureQueuedDroidInfo, restoreQueuedDroidInfo
#include "random.h"
#include "visibility.h"

#include <algorithm>
#include <cstdlib>

namespace gamestate
{
namespace harness
{

static uint32_t g_saveAtTick = 0;
static std::string g_savePath;
static bool g_saveWhenDying = false;
static size_t g_dyingMinCount = 1;
static bool g_dyingNeedsType[3] = {false, false, false}; // OBJ_DROID, OBJ_STRUCTURE, OBJ_FEATURE
static bool g_dyingNeedsPendingVis = false;
static uint32_t g_exitAtTick = 0;
static uint32_t g_rewindAtTick = 0;
static uint32_t g_rewindTicksLater = 0;
static uint32_t g_rewindRestoreAtTick = 0;
static std::string g_rewindSnapshot;
static std::vector<std::vector<std::vector<uint8_t>>> g_rewindPendingGameQueue; // per game-queue slot, the unread messages at the snapshot
static std::vector<std::vector<uint8_t>> g_rewindQueuedOrders; // the local droid orders not sent yet at the snapshot
static bool g_suppressLoadedEvent = false;
static std::string g_resaveOnLoadPath;
static bool g_saveAfterMessages = false;
static bool g_saveWhenCoveragePending = false;
static uint32_t g_checkCoverageTicks = 0;
static uint32_t g_coverageChecks = 0;
static uint32_t g_coverageChecksDiffering = 0;
static bool g_haveTestSeed = false;
static uint32_t g_testSeed = 0;

void setSaveAtTick(uint32_t tick, const std::string &savePath)
{
	g_saveAtTick = tick;
	g_savePath = savePath;
}

void setSaveWhenDying(bool enable)
{
	g_saveWhenDying = enable;
}

bool setDyingFilter(const std::string &filter)
{
	size_t start = 0;
	while (start <= filter.size())
	{
		const size_t end = std::min(filter.find(',', start), filter.size());
		const std::string item = filter.substr(start, end - start);
		if (item == "droid")
		{
			g_dyingNeedsType[OBJ_DROID] = true;
		}
		else if (item == "structure")
		{
			g_dyingNeedsType[OBJ_STRUCTURE] = true;
		}
		else if (item == "feature")
		{
			g_dyingNeedsType[OBJ_FEATURE] = true;
		}
		else if (item == "pendingvis")
		{
			g_dyingNeedsPendingVis = true;
		}
		else if (!item.empty() && item.find_first_not_of("0123456789") == std::string::npos)
		{
			g_dyingMinCount = std::max<size_t>(1, std::stoul(item));
		}
		else
		{
			return false;
		}
		start = end + 1;
	}
	return true;
}

// Whether the objects pending destruction satisfy --gamestate-save-when-dying and its filter.
static bool dyingConditionMet()
{
	if (psDestroyedObj.size() < g_dyingMinCount)
	{
		return false;
	}
	bool haveType[3] = {false, false, false};
	bool havePendingVis = false;
	for (const BASE_OBJECT *psObj : psDestroyedObj)
	{
		if (psObj->type <= OBJ_FEATURE)
		{
			haveType[psObj->type] = true;
		}
		for (const GameWorld *world : {&gameWorld, &mission.gameWorld})
		{
			const auto &pending = world->objects.pendingVisRemoval;
			havePendingVis = havePendingVis || std::find(pending.begin(), pending.end(), psObj) != pending.end();
		}
	}
	for (int t = 0; t < 3; ++t)
	{
		if (g_dyingNeedsType[t] && !haveType[t])
		{
			return false;
		}
	}
	return !g_dyingNeedsPendingVis || havePendingVis;
}

void setExitAtTick(uint32_t tick)
{
	g_exitAtTick = tick;
}

void setRewindTest(uint32_t tick, uint32_t ticksLater)
{
	g_rewindAtTick = tick;
	g_rewindTicksLater = ticksLater;
}

void setSuppressLoadedEvent(bool enable)
{
	g_suppressLoadedEvent = enable;
}

bool suppressLoadedEvent()
{
	return g_suppressLoadedEvent;
}

void setResaveOnLoad(const std::string &savePath)
{
	g_resaveOnLoadPath = savePath;
}

void setTestSeed(uint32_t seed)
{
	g_haveTestSeed = true;
	g_testSeed = seed;
	// The same sequence the start-up code runs with the clock as the seed (realmain)
	srand(seed);
	gameSRand(static_cast<uint32_t>(rand()));
}

uint64_t testScriptRandomState(int player, const std::string &scriptName)
{
	if (!g_haveTestSeed)
	{
		return 0;
	}
	// FNV-1a over the seed, the player and the script name, so each instance gets its own sequence
	uint64_t h = 14695981039346656037ull;
	const auto mix = [&h](uint8_t byte) { h = (h ^ byte) * 1099511628211ull; };
	for (int i = 0; i < 4; ++i)
	{
		mix(static_cast<uint8_t>(g_testSeed >> (8 * i)));
		mix(static_cast<uint8_t>(static_cast<uint32_t>(player) >> (8 * i)));
	}
	for (char c : scriptName)
	{
		mix(static_cast<uint8_t>(c));
	}
	return (h != 0) ? h : 1; // the state must not be 0
}

std::string testScriptPrelude()
{
	if (!g_haveTestSeed)
	{
		return std::string();
	}
	// 2026-01-01T00:00:00Z plus the seed in minutes
	const uint64_t fixedTime = 1767225600000ull + static_cast<uint64_t>(g_testSeed % 1440) * 60000ull;
	return "(function (fixedTime) {\n"
	       "\tconst RealDate = Date;\n"
	       "\tfunction FixedDate(...args) {\n"
	       "\t\tif (!new.target) { return new RealDate(fixedTime).toString(); }\n"
	       "\t\treturn args.length === 0 ? new RealDate(fixedTime) : new RealDate(...args);\n"
	       "\t}\n"
	       "\tFixedDate.prototype = RealDate.prototype;\n"
	       "\tFixedDate.now = function () { return fixedTime; };\n"
	       "\tFixedDate.UTC = RealDate.UTC;\n"
	       "\tFixedDate.parse = RealDate.parse;\n"
	       "\tglobalThis.Date = FixedDate;\n"
	       "})(" + std::to_string(fixedTime) + ");\n";
}

static void checkCoverage(const char *when);

void onColdLoadComplete()
{
	checkCoverage("after load");
	if (g_resaveOnLoadPath.empty())
	{
		return;
	}
	const bool ok = saveGame(g_resaveOnLoadPath.c_str(), GTYPE_SAVE_MIDMISSION);
	debug(LOG_INFO, "GameState harness: %s re-save \"%s\" right after the load, at gameTime %u",
	      ok ? "wrote" : "FAILED to write", g_resaveOnLoadPath.c_str(), gameTime);
	g_resaveOnLoadPath.clear();
	wzQuit(ok ? EXIT_SUCCESS : EXIT_FAILURE);
}

void setSaveAfterMessages(bool enable)
{
	g_saveAfterMessages = enable;
}

void setSaveWhenCoveragePending(bool enable)
{
	g_saveWhenCoveragePending = enable;
}

void setCheckCoverage(uint32_t ticks)
{
	g_checkCoverageTicks = ticks;
}

static void checkCoverage(const char *when)
{
	if (g_checkCoverageTicks == 0)
	{
		return;
	}
	++g_coverageChecks;
	bool differs = false;
	// Both worlds: the inactive one (a campaign home base or off-world map) is restored by recomputing it too
	const std::pair<GameWorld *, const char *> worlds[] = {{&gameWorld, "active"}, {&mission.gameWorld, "inactive"}};
	for (const auto &w : worlds)
	{
		const CoverageCheckResult result = visCheckCoverage(*w.first);
		if (result.differing > 0)
		{
			differs = true;
			debug(LOG_INFO, "GameState harness: coverage check (%s) at gameTime %u, %s world: %zu of %zu object(s) differ from a recompute; first: %s",
			      when, gameTime, w.second, result.differing, result.checked, result.firstDifference.c_str());
		}
		if (result.inconsistentTiles > 0)
		{
			differs = true;
			debug(LOG_INFO, "GameState harness: coverage check (%s) at gameTime %u, %s world: %zu tile count(s) differ from the watched tiles; first: %s",
			      when, gameTime, w.second, result.inconsistentTiles, result.firstInconsistentTile.c_str());
		}
		if (result.unexplored > 0)
		{
			differs = true;
			debug(LOG_INFO, "GameState harness: coverage check (%s) at gameTime %u, %s world: %zu watcher(s) with unexplored watched tiles; first: %s",
			      when, gameTime, w.second, result.unexplored, result.firstUnexplored.c_str());
		}
	}
	if (differs)
	{
		++g_coverageChecksDiffering;
	}
}

static void logCoverageSummary()
{
	if (g_checkCoverageTicks != 0)
	{
		debug(LOG_INFO, "GameState harness: coverage checks: %u, with differences: %u", g_coverageChecks, g_coverageChecksDiffering);
	}
}

static void maybeSave()
{
	if (g_saveAtTick == 0 || gameTime < g_saveAtTick)
	{
		return;
	}
	if (g_saveWhenDying && !dyingConditionMet())
	{
		return;
	}
	if (g_saveWhenCoveragePending && !visCoverageChangesPending())
	{
		return;
	}
	g_saveAtTick = 0; // save exactly once
	if (visCoverageChangesPending())
	{
		// Between two ticks, after the messages: the coverage isn't up to date yet by design (the pending changes are
		// applied at the start of the next tick), so there is nothing to compare a recompute with
		debug(LOG_INFO, "GameState harness: no coverage check at the save: changes are pending");
	}
	else
	{
		checkCoverage("save");
	}
	const size_t dying = psDestroyedObj.size();
	const bool ok = saveGame(g_savePath.c_str(), GTYPE_SAVE_MIDMISSION);
	debug(LOG_INFO, "GameState harness: %s save \"%s\" at gameTime %u (%zu object(s) pending destruction)",
	      ok ? "wrote" : "FAILED to write", g_savePath.c_str(), gameTime, dying);
	if (!ok)
	{
		wzQuit(EXIT_FAILURE);
	}
}

static void maybeRewind()
{
	if (g_rewindAtTick != 0 && gameTime >= g_rewindAtTick && (!g_saveWhenDying || dyingConditionMet()))
	{
		g_rewindAtTick = 0;
		checkCoverage("rewind snapshot");
		try
		{
			g_rewindSnapshot = serializeGameState();
		}
		catch (const std::exception &e)
		{
			debug(LOG_ERROR, "GameState harness: rewind snapshot FAILED: %s", e.what());
			wzQuit(EXIT_FAILURE);
			return;
		}
		// Like a disk save's pending-resume section: the in-flight game-queue messages (scheduled for later ticks by
		// the lockstep latency) belong to the snapshot's timeline, so keep them to re-inject on restore.
		g_rewindPendingGameQueue.assign(MAX_GAMEQUEUE_SLOTS, {});
		for (unsigned p = 0; p < MAX_GAMEQUEUE_SLOTS; ++p)
		{
			g_rewindPendingGameQueue[p] = NETgameQueueCapturePending(p);
		}
		// And the local droid orders not sent yet
		g_rewindQueuedOrders = captureQueuedDroidInfo();
		g_rewindRestoreAtTick = gameTime + std::max<uint32_t>(g_rewindTicksLater, 1);
		debug(LOG_INFO, "GameState harness: rewind snapshot taken at gameTime %u (%zu bytes, %zu object(s) pending destruction); restoring at gameTime %u",
		      gameTime, g_rewindSnapshot.size(), psDestroyedObj.size(), g_rewindRestoreAtTick);
		return;
	}
	if (g_rewindRestoreAtTick != 0 && gameTime >= g_rewindRestoreAtTick)
	{
		g_rewindRestoreAtTick = 0;
		const uint32_t fromTime = gameTime;
		try
		{
			deserializeGameState(g_rewindSnapshot);
		}
		catch (const std::exception &e)
		{
			debug(LOG_ERROR, "GameState harness: rewind restore FAILED: %s", e.what());
			wzQuit(EXIT_FAILURE);
			return;
		}
		g_rewindSnapshot.clear();
		// Replace the live game-queue contents (messages for the live timeline's future ticks) with the
		// snapshot's in-flight messages, as the cold load does with a save's pending-resume section.
		for (unsigned p = 0; p < MAX_GAMEQUEUE_SLOTS; ++p)
		{
			if (NETgameQueueCapturePending(p).empty() && g_rewindPendingGameQueue[p].empty())
			{
				continue;
			}
			NETinitQueue(NETgameQueue(p));
			NETgameQueueRestorePending(p, g_rewindPendingGameQueue[p]);
		}
		g_rewindPendingGameQueue.clear();
		restoreQueuedDroidInfo(g_rewindQueuedOrders);
		g_rewindQueuedOrders.clear();
		// Mirror what the cold load does after its reconstruct: restart the skirmish danger worker
		// (the restore stopped it, and mapInit keeps the restored danger maps),
		// and restart the sync-debug accumulator from the snapshot's save-boundary CRC.
		mapInit(gameWorld);
		resetSyncDebug();
		applyResumeSyncDebugCrc();
		setSyncCheckFloorTime(gameTime);
		syncCrcDetailArmOnSaveOrLoad();
		debug(LOG_INFO, "GameState harness: rewound from gameTime %u to gameTime %u", fromTime, gameTime);
	}
}

void afterMessages()
{
	if (g_saveAfterMessages)
	{
		maybeSave();
	}
}

void tick()
{
	if (g_checkCoverageTicks != 0 && (gameTime / GAME_TICKS_PER_UPDATE) % g_checkCoverageTicks == 0)
	{
		checkCoverage("periodic");
	}
	if (!g_saveAfterMessages)
	{
		maybeSave();
	}
	maybeRewind();
	if (g_exitAtTick != 0 && gameTime >= g_exitAtTick)
	{
		g_exitAtTick = 0;
		logCoverageSummary();
		debug(LOG_INFO, "GameState harness: reached gameTime %u; exiting.", gameTime);
		wzQuit(EXIT_SUCCESS);
	}
}

} // namespace harness
} // namespace gamestate
