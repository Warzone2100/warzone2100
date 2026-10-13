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
#include "gamestate_capture.h"
#include "gamestate_snapshot.h"
#include "transporter.h"  // transporterTestLaunchReinforcements
#include "loop.h"  // levelChangePending
#include "map.h"
#include "objmem.h"
#include "mission.h"
#include "random.h"
#include "visibility.h"

#include <algorithm>
#include <chrono>
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
static GameStateSnapshot g_rewindSnapshot;
static bool g_suppressLoadedEvent = false;
static std::string g_resaveOnLoadPath;
static bool g_saveBeforeMessages = false;
static bool g_saveWhenCoveragePending = false;
static uint32_t g_checkCoverageTicks = 0;
static uint32_t g_coverageChecks = 0;
static uint32_t g_coverageChecksDiffering = 0;
static std::string g_rewindFault;          // --gamestate-rewind-fault kind[:arg]
static bool g_rewindFaultContinue = false; // ... ,continue
static uint32_t g_testReinforceAt = 0;      // --gamestate-test-reinforce (0: none)
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

bool setRewindFault(const std::string &spec)
{
	std::string kind = spec;
	g_rewindFaultContinue = false;
	const std::string continueSuffix = ",continue";
	if (kind.size() > continueSuffix.size() && kind.compare(kind.size() - continueSuffix.size(), continueSuffix.size(), continueSuffix) == 0)
	{
		g_rewindFaultContinue = true;
		kind.resize(kind.size() - continueSuffix.size());
	}
	const std::string name = kind.substr(0, kind.find(':'));
	const bool hasArg = kind.find(':') != std::string::npos && kind.find(':') + 1 < kind.size();
	static const char *const plain[] = {"truncate", "bad-stat", "duplicate-id", "dangling-ref", "overlap", "edge", "off-map", "over-limit"};
	const bool known = std::find(std::begin(plain), std::end(plain), name) != std::end(plain)
		|| ((name == "section-version" || name == "throw-at") && hasArg);
	if (!known)
	{
		return false;
	}
	g_rewindFault = kind;
	return true;
}

static bool parseUint(const std::string &text, uint32_t &value)
{
	if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos || text.size() > 9)
	{
		return false;
	}
	value = static_cast<uint32_t>(std::stoul(text));
	return true;
}

bool setTestReinforce(const std::string &spec)
{
	uint32_t tick = 0;
	if (!parseUint(spec, tick) || tick == 0)
	{
		return false;
	}
	g_testReinforceAt = tick;
	return true;
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

static double msSince(std::chrono::steady_clock::time_point start)
{
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

void onColdLoadComplete()
{
	if (g_testReinforceAt != 0 && gameTime >= g_testReinforceAt)
	{
		g_testReinforceAt = 0;  // (the loaded game was saved after the reinforcements were sent, or without them)
	}
	checkCoverage("after load");
	if (g_resaveOnLoadPath.empty())
	{
		return;
	}
	// A load of a save taken at a safe point leaves the game at one, so this runs now. If it is deferred, the load did
	// not restore the network game timing, or the save was taken before the messages (--gamestate-save-before-messages)
	const bool now = requestGameStateCapture([]() {
		const bool ok = saveGame(g_resaveOnLoadPath.c_str(), GTYPE_SAVE_MIDMISSION);
		debug(LOG_INFO, "GameState harness: %s re-save \"%s\" right after the load, at gameTime %u",
		      ok ? "wrote" : "FAILED to write", g_resaveOnLoadPath.c_str(), gameTime);
		g_resaveOnLoadPath.clear();
		wzQuit(ok ? EXIT_SUCCESS : EXIT_FAILURE);
	});
	if (!now)
	{
		debug(LOG_INFO, "GameState harness: the re-save is deferred: the loaded game has messages for the next tick to process first");
	}
}

void setSaveBeforeMessages(bool enable)
{
	g_saveBeforeMessages = enable;
	setGameStateCaptureBeforeMessagesAllowed(enable);
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
	if (levelChangePending())
	{
		return; // the game can't be saved until the next level has started (see saveGame()): save at its first tick
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
		// The pending changes are applied at the start of the next tick: until then there is nothing to compare a
		// recompute with
		debug(LOG_INFO, "GameState harness: no coverage check at the save: changes are pending");
	}
	else
	{
		checkCoverage("save");
	}
	requestGameStateCapture([]() {
		const size_t dying = psDestroyedObj.size();
		const bool ok = saveGame(g_savePath.c_str(), GTYPE_SAVE_MIDMISSION);
		debug(LOG_INFO, "GameState harness: %s save \"%s\" at gameTime %u (%zu object(s) pending destruction%s)",
		      ok ? "wrote" : "FAILED to write", g_savePath.c_str(), gameTime, dying, g_saveBeforeMessages ? ", before the messages" : "");
		if (!ok)
		{
			wzQuit(EXIT_FAILURE);
		}
	});
}

static void takeRewindSnapshot()
{
	checkCoverage("rewind snapshot");
	const auto start = std::chrono::steady_clock::now();
	try
	{
		g_rewindSnapshot = takeGameStateSnapshot();
	}
	catch (const std::exception &e)
	{
		debug(LOG_ERROR, "GameState harness: rewind snapshot FAILED: %s", e.what());
		wzQuit(EXIT_FAILURE);
		return;
	}
	const double ms = msSince(start);
	size_t pendingMessages = 0, pendingBytes = 0;
	for (const auto &slot : g_rewindSnapshot.gameQueue)
	{
		pendingMessages += slot.size();
		for (const auto &message : slot)
		{
			pendingBytes += message.size();
		}
	}
	g_rewindRestoreAtTick = gameTime + std::max<uint32_t>(g_rewindTicksLater, 1);
	debug(LOG_INFO, "GameState harness: rewind snapshot taken at gameTime %u (%zu bytes, %zu object(s) pending destruction); restoring at gameTime %u",
	      gameTime, g_rewindSnapshot.document.size(), psDestroyedObj.size(), g_rewindRestoreAtTick);
	debug(LOG_INFO, "GameState harness: rewind snapshot timing: total %.1f ms; %zu pending game message(s), %zu bytes",
	      ms, pendingMessages, pendingBytes);
}

// At the save point
static void maybeRewindSnapshot()
{
	// Not while a level change is pending, for the same reason as a save (see saveGame()): snapshot in the next level
	if (g_rewindAtTick != 0 && gameTime >= g_rewindAtTick && (!g_saveWhenDying || dyingConditionMet()) && !levelChangePending())
	{
		g_rewindAtTick = 0;
		requestGameStateCapture(takeRewindSnapshot);
	}
}

// The first object reference ({id, player, type}, as writeObjRef writes it) in a document subtree, or nullptr
static nlohmann::ordered_json *findObjRef(nlohmann::ordered_json &j)
{
	if (j.is_object())
	{
		if (j.size() == 3 && j.contains("id") && j.contains("player") && j.contains("type"))
		{
			return &j;
		}
		for (auto &item : j.items())
		{
			if (auto *found = findObjRef(item.value()))
			{
				return found;
			}
		}
	}
	else if (j.is_array())
	{
		for (auto &item : j)
		{
			if (auto *found = findObjRef(item))
			{
				return found;
			}
		}
	}
	return nullptr;
}

// --gamestate-rewind-fault: damage the snapshot document (or arm a throw in the restore) the way the fault kind says.
// Returns what was done, or throws std::runtime_error if the document has nothing to damage that way.
static std::string applyRewindFault(std::string &doc)
{
	const size_t colon = g_rewindFault.find(':');
	const std::string kind = g_rewindFault.substr(0, colon);
	const std::string arg = colon == std::string::npos ? std::string() : g_rewindFault.substr(colon + 1);
	if (kind == "truncate")
	{
		doc.resize(doc.size() / 2);
		return "truncated the document to " + std::to_string(doc.size()) + " bytes";
	}
	if (kind == "throw-at")
	{
		setGameStateRestoreFaultStage(arg);
		return "armed a throw at restore stage " + arg;
	}
	nlohmann::ordered_json j = nlohmann::ordered_json::parse(doc);
	auto &structures = j.at("world").at("structures");
	auto &droids = j.at("world").at("droids");
	auto needStructures = [&](size_t n) {
		if (structures.size() < n)
		{
			throw std::runtime_error("the snapshot has fewer than " + std::to_string(n) + " structure(s)");
		}
	};
	std::string what;
	if (kind == "section-version")
	{
		j.at(arg)["version"] = 99999;
		what = "set section " + arg + "'s version to 99999";
	}
	else if (kind == "bad-stat")
	{
		needStructures(1);
		what = "renamed structure " + std::to_string(structures[0].at("id").get<uint32_t>()) + "'s stat " + structures[0].at("statId").get<std::string>() + " to NoSuchStat";
		structures[0]["statId"] = "NoSuchStat";
	}
	else if (kind == "duplicate-id")
	{
		if (droids.size() < 2)
		{
			throw std::runtime_error("the snapshot has fewer than 2 droids");
		}
		what = "gave droid " + std::to_string(droids[1].at("id").get<uint32_t>()) + " the id of droid " + std::to_string(droids[0].at("id").get<uint32_t>());
		droids[1]["id"] = droids[0].at("id");
	}
	else if (kind == "dangling-ref")
	{
		auto *ref = findObjRef(j.at("world"));
		if (ref == nullptr)
		{
			throw std::runtime_error("the snapshot's world has no object reference");
		}
		what = "repointed a reference to object " + std::to_string(ref->at("id").get<uint32_t>()) + " to the unused id 2147483632";
		(*ref)["id"] = 2147483632u;
	}
	else if (kind == "overlap")
	{
		needStructures(2);
		what = "moved structure " + std::to_string(structures[1].at("id").get<uint32_t>()) + " onto structure " + std::to_string(structures[0].at("id").get<uint32_t>());
		structures[1]["pos"][0] = structures[0].at("pos")[0];
		structures[1]["pos"][1] = structures[0].at("pos")[1];
	}
	else if (kind == "edge" || kind == "off-map")
	{
		needStructures(1);
		const auto &mapDynamic = j.at("world").at("mapDynamic");
		const int x = kind == "edge" ? 64 : mapDynamic.at("width").get<int>() * 128 + 512;
		const int y = kind == "edge" ? 64 : mapDynamic.at("height").get<int>() * 128 + 512;
		what = "moved structure " + std::to_string(structures[0].at("id").get<uint32_t>()) + " to (" + std::to_string(x) + ", " + std::to_string(y) + ")";
		structures[0]["pos"][0] = x;
		structures[0]["pos"][1] = y;
	}
	else if (kind == "over-limit")
	{
		needStructures(1);
		const unsigned player = structures[0].at("player").get<unsigned>();
		for (auto &perStat : j.at("limits").at("structLimits"))
		{
			if (player < perStat.size())
			{
				perStat[player] = 0;
			}
		}
		what = "set every structure limit of player " + std::to_string(player) + " to 0";
	}
	doc = j.dump();
	return what;
}

// At the tick hook. The restore replaces the game queues with the snapshot's,
// so the messages the live game would read next are dropped.
static void maybeRewindRestore()
{
	if (g_rewindRestoreAtTick != 0 && gameTime >= g_rewindRestoreAtTick)
	{
		g_rewindRestoreAtTick = 0;
		const uint32_t fromTime = gameTime;
		if (!g_rewindFault.empty())
		{
			try
			{
				debug(LOG_INFO, "GameState harness: rewind fault %s: %s", g_rewindFault.c_str(), applyRewindFault(g_rewindSnapshot.document).c_str());
			}
			catch (const std::exception &e)
			{
				debug(LOG_ERROR, "GameState harness: rewind fault %s not applied: %s", g_rewindFault.c_str(), e.what());
				wzQuit(EXIT_FAILURE);
				return;
			}
		}
		const auto start = std::chrono::steady_clock::now();
		std::string error;
		const GameStateRestoreResult result = restoreGameStateSnapshot(g_rewindSnapshot, &error);
		setGameStateRestoreFaultStage(std::string());
		if (result == GameStateRestoreResult::Rejected)
		{
			debug(LOG_ERROR, "GameState harness: rewind restore FAILED before changing any live state (%.1f ms): %s; the live game goes on",
			      msSince(start), error.c_str());
			g_rewindSnapshot = GameStateSnapshot();
			return;
		}
		if (result == GameStateRestoreResult::Failed)
		{
			debug(LOG_ERROR, "GameState harness: rewind restore FAILED at stage %s, after the live state was changed (%.1f ms): %s",
			      gameStateLastRestoreStage(), msSince(start), error.c_str());
			g_rewindSnapshot = GameStateSnapshot();
			if (g_rewindFaultContinue)
			{
				debug(LOG_INFO, "GameState harness: going on with the half-restored game (--gamestate-rewind-fault ...,continue)");
				return;
			}
			wzQuit(EXIT_FAILURE);
			return;
		}
		g_rewindSnapshot = GameStateSnapshot();
		debug(LOG_INFO, "GameState harness: rewound from gameTime %u to gameTime %u (%u unresolved reference(s))", fromTime, gameTime,
		      gameStateLastRestoreUnresolvedReferences());
		debug(LOG_INFO, "GameState harness: rewind restore timing: total %.1f ms", msSince(start));
	}
}

// The save point: right after the game loop has processed the received messages, once every message for the next tick
// has been (see gamestate_capture.h)
void afterMessages()
{
	if (!g_saveBeforeMessages && gameStateCaptureSafe())
	{
		maybeSave();
		maybeRewindSnapshot();
	}
	if (g_testReinforceAt != 0 && gameStateCaptureSafe() && gameTime >= g_testReinforceAt)
	{
		g_testReinforceAt = 0;
		const bool launched = transporterTestLaunchReinforcements();
		debug(LOG_INFO, "GameState harness: reinforcements %s at gameTime %u", launched ? "launched" : "NOT launched", gameTime);
	}
}

void tick()
{
	// Not while a level change is pending: the game can't be saved then (see saveGame()), and no tick runs before the
	// level changes, so nothing would ever recompute the coverage from that state
	if (g_checkCoverageTicks != 0 && (gameTime / GAME_TICKS_PER_UPDATE) % g_checkCoverageTicks == 0 && !levelChangePending())
	{
		checkCoverage("periodic");
	}
	if (g_saveBeforeMessages)
	{
		maybeSave();
		maybeRewindSnapshot();
	}
	maybeRewindRestore();
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
