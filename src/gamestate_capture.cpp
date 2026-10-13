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

#include "gamestate_capture.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"  // NET_ALL_PLAYERS

#include "gamestate_snapshot.h"  // gameStateRestoreFailed
#include "loop.h"  // levelChangePending

#include <vector>

static std::vector<std::function<void()>> pendingCaptures;
static bool capturedSinceTick = false;
static uint32_t captureGameTime = 0;
static const char *captureKind = "capture";
static bool beforeMessagesAllowed = false;

bool gameStateCaptureSafe()
{
	return !levelChangePending() && !gameStateRestoreFailed() && (beforeMessagesAllowed || checkPlayerGameTime(NET_ALL_PLAYERS));
}

static void runCapture(const std::function<void()> &capture)
{
	capturedSinceTick = true;
	captureGameTime = gameTime;
	captureKind = "capture";
	capture();
}

void gameStateCaptureNoteRestore()
{
	capturedSinceTick = true;
	captureGameTime = gameTime;
	captureKind = "restore";
}

bool requestGameStateCapture(std::function<void()> capture)
{
	if (pendingCaptures.empty() && gameStateCaptureSafe())
	{
		runCapture(capture);
		return true;
	}
	debug(LOG_INFO, "Deferring a game state capture at gameTime %u until the messages for the next tick have arrived", gameTime);
	pendingCaptures.push_back(std::move(capture));
	return false;
}

void runPendingGameStateCaptures()
{
	if (pendingCaptures.empty())
	{
		return;
	}
	if (levelChangePending())
	{
		// Requested for the world that is about to be replaced
		debug(LOG_WARNING, "Dropping %zu deferred game state capture(s): a level change or a savegame load is pending", pendingCaptures.size());
		pendingCaptures.clear();
		return;
	}
	if (!gameStateCaptureSafe())
	{
		return;
	}
	// A capture may request another (or save, which can't): run the ones queued so far
	std::vector<std::function<void()>> captures;
	captures.swap(pendingCaptures);
	for (const auto &capture : captures)
	{
		runCapture(capture);
	}
}

void clearPendingGameStateCaptures()
{
	if (!pendingCaptures.empty())
	{
		debug(LOG_WARNING, "Dropping %zu deferred game state capture(s): the game is shutting down", pendingCaptures.size());
	}
	pendingCaptures.clear();
	capturedSinceTick = false;
}

void gameStateCaptureOnTick()
{
	capturedSinceTick = false;
}

void gameStateCaptureCheckGameMessage(unsigned player, const char *messageType)
{
	if (capturedSinceTick && !beforeMessagesAllowed)
	{
		ASSERT(false, "Game message %s of player %u read after a game state %s at gameTime %u (now %u), before the next tick: the capture missed it",
		       messageType, player, captureKind, captureGameTime, gameTime);
		capturedSinceTick = false;  // once per capture
	}
}

void setGameStateCaptureBeforeMessagesAllowed(bool allowed)
{
	beforeMessagesAllowed = allowed;
}
