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
 *  Deterministic movement benchmark: metrics sink and headless scenario runner.
 *
 *  Measures movement quality as numbers so changes to the movement micro-AI can
 *  be scored against a baseline instead of judged by eye. Runs a scripted
 *  scenario headless for a fixed number of game ticks, then writes a JSON
 *  scorecard and quits.
 *
 *  Everything sampled here is deterministic sim state, and the scenario ends at
 *  a fixed tick, so two runs of the same scenario produce an identical
 *  finalPositionsCrc. A mismatch means something broke sync.
 */

#pragma once

#include "lib/framework/frame.h"

#include <cstdint>
#include <string>

/**
 * Counters sampled from the movement micro-AI while a bench scenario runs.
 *
 * The instance is only allocated in bench mode, so normal play pays one
 * predictable-not-taken branch per hook site.
 */
struct MovementMetrics
{
	uint64_t bumps = 0;         ///< moveCalcDroidSlide registered a fresh bump
	uint64_t bumpsRepeat = 0;   ///< contact while already bumped (lastBump refresh)
	uint64_t hardStops = 0;     ///< second contact in one scan zeroed the move vector
	uint64_t giveUps = 0;       ///< moveBlocked gave up on the move entirely
	uint64_t repaths = 0;       ///< moveBlocked rerouted to the same destination
};

/// Non-null only while a bench scenario is running.
extern MovementMetrics *g_moveMetrics;

/// Selects the scenario to run. Returns false if the name is not a known scenario.
bool movementBenchSelectScenario(const std::string &name);

/// True if a scenario was selected for this run.
bool movementBenchActive();

/// The tests/ config file the active scenario launches. Empty if inactive.
const std::string &movementBenchTestConfig();

/// Fixed seed for the synchronised RNG, so a run is reproducible.
uint32_t movementBenchSeed();

/// Samples sim state and ends the run once complete. Called once per game state update.
void movementBenchUpdate();
