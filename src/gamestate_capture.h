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
 *  The point at which the game state may be captured (a savegame, an in-memory snapshot).
 *
 *  A capture is exact only between two ticks, once every game message that the next tick needs has been processed.
 *  The game loop processes the messages for tick T+1 right after tick T (orders, structure and research requests),
 *  and what they do between the ticks isn't all in the game state.
 *  A route search requested then, for example, uses tick T's congestion overlays and blocking-map caches,
 *  which are not saved, so a capture taken before those messages may resume with searches that find different paths.
 *
 *  In single-player the game loop always ends with those messages processed,
 *  so menu saves, quicksaves and autosaves are always at a safe point.
 *  In a networked game the game loop can end while it waits for another player's messages.
 *  A capture requested then is deferred to the frame in which they arrive.
 *
 *  Every save and snapshot goes through requestGameStateCapture().
 *  saveGame() refuses to save at an unsafe point.
 */

#pragma once

#include <functional>

/// Whether a capture of the game state taken now would be exact: every game message the next tick needs has been
/// processed (checkPlayerGameTime(NET_ALL_PLAYERS)), no level change or savegame load is pending, and the game isn't
/// stopped by a failed in-process restore (gameStateRestoreFailed()).
/// With setGameStateCaptureBeforeMessagesAllowed(true), the first condition is waived.
bool gameStateCaptureSafe();

/// Run capture now if gameStateCaptureSafe(), else queue it to run, in order, at the first safe point (in the game loop,
/// right after it has processed the received messages). Returns true if it ran now. Queued captures are dropped if a
/// level change or savegame load is requested before they run, or when the game shuts down.
bool requestGameStateCapture(std::function<void()> capture);

/// Called by gameLoop() right after it has processed the received messages: runs the queued captures if it is now safe.
void runPendingGameStateCaptures();

/// Called when the game shuts down.
void clearPendingGameStateCaptures();

/// Called by gameLoop() at the start of each tick.
void gameStateCaptureOnTick();

/// Called by restoreGameStateSnapshot() once it has restored a snapshot: the game is back at a capture point, so the
/// check below applies until the next tick (a message read then would be one the restored game reads twice).
void gameStateCaptureNoteRestore();

/// Debug check, called by recvMessage() for each game message it reads: a game message read after a capture (or a
/// restore) and before the next tick is one the capture missed.
void gameStateCaptureCheckGameMessage(unsigned player, const char *messageType);

/// Test-only (--gamestate-save-before-messages): allow captures before the messages for the next tick have been
/// processed, as at the harness's tick hook.
void setGameStateCaptureBeforeMessagesAllowed(bool allowed);
