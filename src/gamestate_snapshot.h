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
 *  In-process GameState snapshots: taken and restored by the same running game,
 *  for the rewind harness and replay seek (gamestate_replay_seek.h).
 *  Not the savegame path: a savegame is written by saveGame() and loaded by the cold load (gamestate_savegame.cpp),
 *  which keeps its own pending-resume handling and failure path.
 *
 *  A snapshot is taken at a capture point (see gamestate_capture.h), through requestGameStateCapture().
 *  Restoring one puts the game back at that point:
 *  between two ticks, with every game message the next tick needs processed.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct GameStateSnapshot
{
	std::string document;                                     ///< the GameState document (serializeGameState)
	std::vector<std::vector<std::vector<uint8_t>>> gameQueue; ///< per game-queue slot, the messages not read yet
	std::vector<std::vector<uint8_t>> queuedOrders;           ///< the local droid orders not sent yet
	uint32_t gameTime = 0;                                    ///< the game time it was taken at

	bool empty() const { return document.empty(); }
};

/// Take a snapshot of the running game. Call it at a capture point (from a requestGameStateCapture() capture).
/// Throws gamestate::StateError (or another std::exception) if the game can't be serialized.
GameStateSnapshot takeGameStateSnapshot();

enum class GameStateRestoreResult
{
	Restored,  ///< the game is now at the snapshot
	Rejected,  ///< the restore failed before changing anything: the live game can go on
	Failed,    ///< the restore failed after changing live state: the game is half restored and must not go on
};

/// Restore a snapshot into the running game: the GameState document, then the game queues, the unsent local orders,
/// the danger worker and the sync-debug state, as they were when it was taken. Call it between two ticks. On a
/// failure, error (if given) says why, and gamestate::gameStateLastRestoreStage() where.
GameStateRestoreResult restoreGameStateSnapshot(const GameStateSnapshot &snapshot, std::string *error = nullptr);
