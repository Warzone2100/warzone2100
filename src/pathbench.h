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
 *  Pathfinding benchmark
 *  Times canned route requests and counts the nodes they expand.
 *
 *  Exists so a change to the planner can be shown not to have cost anything
 *  before it is merged, which is the measurement the last attempt at this
 *  refactor did not have and was reverted for.
 *
 *  It reports two things per case, and they answer different questions.
 *  Wall-clock is what a player pays but varies run to run, so it can only ever
 *  say "within noise". Nodes expanded is exact and reproducible, so it can say
 *  "changed by zero", and a refactor that leaves the search alone should move
 *  it by nothing at all. A node count that shifts while the timing looks fine
 *  means the search changed and the timing simply did not resolve it.
 *
 *  Runs the same work two ways. The direct measurement calls the search on the
 *  main thread with jobs built by hand, which isolates search cost from thread
 *  handoff and is the number to compare. The queued measurement pushes the same
 *  requests through the worker pool the game uses and reports how long they
 *  take to drain, which covers dispatch and the context cache but is too
 *  noisy to compare.
 */

#pragma once

#include <cstdint>
#include <string>

/// Selects the benchmark for this run. Returns false if the name is unknown.
bool pathBenchSelect(const std::string &name);

/// True if the benchmark was selected for this run.
bool pathBenchActive();

/// The tests/ config the benchmark launches. Empty if inactive.
const std::string &pathBenchTestConfig();

/// Fixed seed for the synchronised RNG, so a run is reproducible.
uint32_t pathBenchSeed();

/// Overrides how many times each case is timed. More repeats tighten the
/// median at the cost of a longer run.
void pathBenchSetRepeats(uint32_t repeats);

/// Runs the benchmark once the game has settled, then ends the run. Called once
/// per game state update.
void pathBenchUpdate();
