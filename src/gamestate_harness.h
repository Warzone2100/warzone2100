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
 *  All hooks run in gameLoop() right after a game state update (after its "End game state update"
 *  syncDebug line), i.e. between two ticks, so a save or snapshot sees exactly what a menu save sees.
 *  Every option is inert unless set on the command line.
 */

#pragma once

#include <cstdint>
#include <string>

namespace gamestate
{
namespace harness
{

/// --gamestate-save-at-tick=<tick>:<savegame path>: write a normal savegame at the first tick >= tick,
/// and keep running.
void setSaveAtTick(uint32_t tick, const std::string &savePath);

/// --gamestate-save-when-dying: delay the save-at-tick save until a tick that ends with objects waiting to be freed
/// (psDestroyedObj not empty), i.e. a save with references to objects that died in the save tick.
void setSaveWhenDying(bool enable);

/// --gamestate-exit-at-tick=<tick>: quit (exit code 0) at the first tick >= tick.
void setExitAtTick(uint32_t tick);

/// --gamestate-rewind-test=<tick>:<ticks later>: snapshot the GameState in-process at <tick>, keep running for
/// <ticks later> more game time, then restore the snapshot in-process and keep running from it.
void setRewindTest(uint32_t tick, uint32_t ticksLater);

/// --gamestate-test-suppress-loaded-event: test-only. On a GameState cold load, do not fire eventGameLoaded,
/// and drop the queued replay of eventResearched.
void setSuppressLoadedEvent(bool enable);
bool suppressLoadedEvent();

/// --gamestate-resave-on-load=<savegame path>: after a GameState cold load has completed (just before the first
/// tick), write a savegame and quit. Comparing it with the loaded save shows what the load did not restore exactly.
void setResaveOnLoad(const std::string &savePath);

/// --gamestate-test-seed=<seed>: make a run repeatable in another process. Seeds the C library random generator
/// (which picks a new game's random seed) and every script instance's Math.random() from <seed> rather than from
/// the clock. Test-only.
void setTestSeed(uint32_t seed);

/// The initial Math.random() state for a new script instance when a test seed is set, else 0 (keep the clock seed).
uint64_t testScriptRandomState(int player, const std::string &scriptName);

/// Script code to run in a new script instance before anything else when a test seed is set, else empty.
/// It replaces Date with one whose current time is fixed (derived from the seed),
/// since scripts can read the wall clock (ex. NullBot picks its personality by the current minute).
std::string testScriptPrelude();

/// Called at the end of a GameState cold load (stageThreeInitialise), just before the first tick.
void onColdLoadComplete();

/// Per-tick hook, called by gameLoop() right after each game state update.
void tick();

} // namespace harness
} // namespace gamestate
