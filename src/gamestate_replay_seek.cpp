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

#include "gamestate_replay_seek.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/nettypes.h"

#include "gamestate_capture.h"
#include "gamestate_serialize.h"
#include "gamestate_snapshot.h"
#include "gamestate_snapshot_ring.h"
#include "wrappers.h"  // headlessGameMode

#include <chrono>

namespace gamestate
{

namespace
{

enum class SeekState
{
	None,
	Requested,       ///< to be started at the next capture point
	FastForwarding,  ///< ticking towards the target
};

SeekState state = SeekState::None;
uint32_t target = 0;
uint32_t requestedAt = 0;        ///< game time when it was requested
uint32_t fastForwardFrom = 0;    ///< game time the fast-forward started at
std::chrono::steady_clock::time_point startedAt;
double restoreMs = 0;

double msSince(std::chrono::steady_clock::time_point start)
{
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

void startSeek()
{
	startedAt = std::chrono::steady_clock::now();
	restoreMs = 0;
	// Restore unless playback can simply go on to the target: it is at or before it, and the ring has nothing newer
	// than playback on the way
	std::string error;
	nonstd::optional<GameStateSnapshot> snapshot;
	const std::vector<uint32_t> times = snapshotRingTimes();
	bool restore = target < gameTime;
	if (!restore)
	{
		for (uint32_t t : times)
		{
			restore = restore || (t > gameTime && t <= target);
		}
	}
	if (restore)
	{
		snapshot = snapshotRingFindAtOrBefore(target, &error);
		if (!snapshot.has_value())
		{
			debug(LOG_ERROR, "Replay seek to gameTime %u failed: %s; playback goes on", target, error.c_str());
			state = SeekState::None;
			return;
		}
		const uint32_t from = gameTime;
		const auto restoreStart = std::chrono::steady_clock::now();
		const GameStateRestoreResult result = restoreGameStateSnapshot(snapshot.value(), &error);
		restoreMs = msSince(restoreStart);
		if (result == GameStateRestoreResult::Rejected)
		{
			debug(LOG_ERROR, "Replay seek to gameTime %u failed: the snapshot at gameTime %u was rejected (%s); playback goes on",
			      target, snapshot->gameTime, error.c_str());
			state = SeekState::None;
			return;
		}
		if (result == GameStateRestoreResult::Failed)
		{
			// The game is stopped (gameStateRestoreFailed())
			debug(LOG_ERROR, "Replay seek to gameTime %u failed: restoring the snapshot at gameTime %u failed at stage %s (%s); the game is stopped",
			      target, snapshot->gameTime, gameStateLastRestoreStage(), error.c_str());
			state = SeekState::None;
			return;
		}
		debug(LOG_INFO, "Replay seek to gameTime %u: restored the snapshot at gameTime %u (from gameTime %u, %.1f ms)",
		      target, gameTime, from, restoreMs);
	}
	fastForwardFrom = gameTime;
	state = SeekState::FastForwarding;
}

void finishSeek()
{
	debug(LOG_INFO, "Replay seek to gameTime %u: reached at gameTime %u, requested at gameTime %u; fast-forwarded %u tick(s) from gameTime %u; "
	      "%.1f ms in all (restore %.1f ms)",
	      target, gameTime, requestedAt, (gameTime - fastForwardFrom) / GAME_TICKS_PER_UPDATE, fastForwardFrom, msSince(startedAt), restoreMs);
	state = SeekState::None;
}

} // anonymous namespace

bool requestReplaySeek(uint32_t targetGameTime, std::string *error)
{
	auto fail = [error](const char *why) {
		if (error != nullptr)
		{
			*error = why;
		}
		return false;
	};
	if (!NETisReplay())
	{
		return fail("not playing a replay back");
	}
	if (!headlessGameMode())
	{
		return fail("only headless (the UI's pointers to game objects aren't released on a restore)");
	}
	if (!snapshotRingRunning())
	{
		return fail("the snapshot ring isn't running");
	}
	target = targetGameTime;
	requestedAt = gameTime;
	state = SeekState::Requested;
	return true;
}

void replaySeekAtCapturePoint()
{
	if (state == SeekState::Requested && gameStateCaptureSafe())
	{
		startSeek();
	}
	if (state == SeekState::FastForwarding && gameTime >= target)
	{
		finishSeek();
	}
}

bool replaySeekFastForwarding()
{
	return state == SeekState::FastForwarding && gameTime < target;
}

void replaySeekCancel()
{
	state = SeekState::None;
}

} // namespace gamestate
