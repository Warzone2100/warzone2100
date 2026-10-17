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
/** \file
 *  Command-line test harness for GameState save / load / rewind fidelity.
 *
 *  The saves and the rewind snapshot are taken at the save point (see gamestate_capture.h): between two ticks, right
 *  after the game loop has processed the messages for the next tick (afterMessages()), where a menu save is taken.
 *  The other hooks, and the rewind restore, run right after a game state update (tick()).
 *  Every option is inert unless set on the command line.
 */

#pragma once

#include <cstdint>
#include <string>

#include <nonstd/optional.hpp>

namespace gamestate
{
namespace harness
{

/// --gamestate-save-at-tick=<tick>:<savegame path>: write a normal savegame at the first tick >= tick,
/// and keep running.
void setSaveAtTick(uint32_t tick, const std::string &savePath);

/// --gamestate-save-when-dying: delay the save-at-tick save until a tick that ends with objects waiting to be freed
/// (psDestroyedObj not empty), i.e. a save with references to objects that died in the save tick.
/// Delays the --gamestate-rewind-test snapshot the same way.
void setSaveWhenDying(bool enable);

/// --gamestate-dying-filter=<filter>: with --gamestate-save-when-dying,
/// what the objects pending destruction must include.
/// Comma-separated: a minimum count, "droid", "structure", "feature" (at least one of that type),
/// "pendingvis" (at least one whose tile visibility removal is still pending).
/// Returns false on a bad filter.
bool setDyingFilter(const std::string &filter);

/// --gamestate-exit-at-tick=<tick>: quit (exit code 0) at the first tick >= tick.
void setExitAtTick(uint32_t tick);

/// --gamestate-rewind-test=<tick>:<ticks later>: snapshot the GameState in-process at <tick>, keep running for
/// <ticks later> more game time, then restore the snapshot in-process and keep running from it.
void setRewindTest(uint32_t tick, uint32_t ticksLater);

/// --gamestate-rewind-fault=<kind>[,continue]: test-only, with --gamestate-rewind-test.
/// Damage the snapshot right before it is restored, to see where the restore fails and what is left. Kinds:
/// - truncate (cut the document in half)
/// - section-version:<section> (an unknown section version)
/// - bad-stat (a structure with an unknown stat)
/// - duplicate-id (two droids with one id)
/// - dangling-ref (the first object reference in the world repointed to an unused id)
/// - overlap (a structure moved onto another)
/// - edge (a structure moved to the map corner)
/// - off-map (a structure moved past the map)
/// - over-limit (every structure limit of a player set to 0, which a restore must ignore)
/// - throw-at:<stage> (throw when the restore reaches that stage, see gameStateLastRestoreStage())
/// - during replay playback, replay-position (a game queue's replay read position put past the replay's end)
///   and no-replay-positions (the snapshot's replay read positions dropped, as if taken outside replay playback)
/// A restore that fails before changing any live state lets the live game go on.
/// One that fails later quits, or with ",continue" goes on with the half-restored game.
/// Returns false on an unknown kind.
bool setRewindFault(const std::string &spec);

/// --gamestate-snapshot-ring=<interval ms>[,<budget KB>[,<zlib level>]][,sync] | off: the snapshot ring
/// (gamestate_snapshot_ring.h) during replay playback.
/// With options, it runs with them during every playback, headless too.
/// "sync" (test-only) waits for the worker rather than taking a snapshot late,
/// so snapshots are taken at the same game times in every run.
/// "off" turns it off (by default it runs with the GUI only: gamestate_replay_seek.h).
/// Returns false on a bad spec.
bool setSnapshotRing(const std::string &spec);

/// --gamestate-seek-test=<at>:<target>[,<at>:<target>...]: test-only, during replay playback.
/// At the first capture point at or after each <at> (in order),
/// request a replay seek to <target> (gamestate_replay_seek.h).
/// Returns false on a bad spec.
bool setSeekTest(const std::string &spec);

/// --gamestate-seek-fault=<kind>[,<restores>]: test-only. Damage the snapshots the replay seeks restore,
/// the first <restores> of them (default 1) in the whole run, with a --gamestate-rewind-fault kind,
/// to exercise the seek's fallbacks (an older snapshot, then playing the replay again from the start).
/// Returns false on a bad spec.
bool setSeekFault(const std::string &spec);

/// --gamestate-test-suppress-loaded-event: on a GameState cold load, do not fire eventGameLoaded,
/// and drop the queued replay of eventResearched.
void setSuppressLoadedEvent(bool enable);
bool suppressLoadedEvent();

/// --gamestate-test-continue-levels: go on to the next campaign level
/// without waiting on the mission result screen (which needs a click),
/// and log the texture pages replaceTexture() replaced when each level starts (and when a save is loaded).
void setContinueLevels(bool enable);
bool continueLevels();
/// At the end of stageThreeInitialise (a level started, or a save was loaded): log the replaced textures if asked
void noteLevelStarted();

/// --gamestate-resave-on-load=<savegame path>: after a GameState cold load has completed
/// (just before the first tick), write a savegame and quit.
void setResaveOnLoad(const std::string &savePath);

/// --gamestate-test-seed=<seed>: make a run repeatable in another process, on any platform,
/// with or without the GUI. Set explicitly from <seed> rather than from the clock:
/// - a new game's random seed is <seed> itself (gameSRand, through testGameSeed()),
///   for skirmish and multiplayer games in SendFireUp, and campaign games when they start from the title loop
/// - every script instance's Math.random() (testScriptRandomState) and its clock (testScriptPrelude)
/// - the network latency is negotiated without the wall clock (gameTimeSetDeterministicLatency), as with a CRC trace
/// The C library's generator, which the simulation doesn't use (effects, player colors), is seeded with <seed> too.
void setTestSeed(uint32_t seed);

/// With --gamestate-test-seed, the random seed of a new game, else nullopt (the game picks its own)
nonstd::optional<uint32_t> testGameSeed();

/// The initial Math.random() state for a new script instance when a test seed is set, else 0 (keep the clock seed).
uint64_t testScriptRandomState(int player, const std::string &scriptName);

/// Script code to run in a new script instance before anything else when a test seed is set, else empty.
/// It replaces Date with one whose current time is fixed (derived from the seed),
/// since scripts can read the wall clock (ex. NullBot picks its personality by the current minute).
std::string testScriptPrelude();

/// --gamestate-save-before-messages: test-only.
/// Take the --gamestate-save-at-tick save and the --gamestate-rewind-test snapshot right after a tick (tick()),
/// before the game loop processes the messages for the next tick, instead of at the save point.
/// NOTE: a game saved there may resume differently.
/// (--gamestate-save-after-messages, which selects the default save point, is still accepted.)
void setSaveBeforeMessages(bool enable);

/// --gamestate-save-when-coverage-pending: delay the save until there are tile coverage changes not yet applied
/// (visCoverageChangesPending), as after a range upgrade between two ticks.
void setSaveWhenCoveragePending(bool enable);

/// Hook called by gameLoop() right after it has processed the received messages: the save point, once it is safe.
void afterMessages();

/// --gamestate-check-coverage=<ticks>: test-only.
/// Every <ticks> ticks, right before a harness save or rewind snapshot, and right after a GameState load,
/// check the following (visCheckCoverage), and log the checks that find a difference:
/// - that the tile coverage of every droid and structure in both worlds matches a recompute from its current state
/// - that the per-tile counts add up
/// - that watched tiles are explored
/// A loaded game can only recompute the coverage,
/// so a difference at a save point is coverage the loaded game won't have.
void setCheckCoverage(uint32_t ticks);

/// Called at the end of a GameState cold load (stageThreeInitialise), just before the first tick.
void onColdLoadComplete();

/// Per-tick hook, called by gameLoop() right after each game state update.
void tick();

/// --gamestate-ui-stress=<ticks>: test-only. Every <ticks> ticks (at the capture point),
/// point the UI at the live droid most likely to die next (the one with the least body points left), whoever's it is:
/// the camera tracks it, the sensor view targets it, it and its player's other weakest droids are selected,
/// and one of the forms is opened in turn (when the droid is the local player's).
/// And every frame, once the interface is refreshed (afterScreenRefresh()),
/// check that every UI reference to a game object is to one not yet freed (checkUiObjectRefs).
/// Returns false on a bad value.
bool setUiStress(const std::string &spec);

/// --gamestate-test-reinforce=<tick>: at the first capture point at or after <tick>,
/// during an off-world campaign mission, send reinforcements as the player does from the transporter screen
/// (transporterTestLaunchReinforcements).
/// Not in a game loaded at or after <tick>: it was saved after that.
/// Returns false on a bad value.
bool setTestReinforce(const std::string &spec);

/// Frame hook, called by renderLoop() right after the interface's refresh (intDoScreenRefresh), before any widget
/// runs or is drawn.
void afterScreenRefresh();

} // namespace harness
} // namespace gamestate
