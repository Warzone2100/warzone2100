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
#include "lib/sound/audio.h"

#include "gamestate_capture.h"
#include "gamestate_serialize.h"
#include "gamestate_snapshot.h"
#include "gamestate_snapshot_ring.h"
#include "loop.h"
#include "wrappers.h"

#include <algorithm>
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
unsigned restoreAttempts = 0;    ///< restores tried by this seek
bool resumePaused = false;       ///< playback was paused when the seek was asked for: pause it again when done
unsigned timeStartsOwed = 0;     ///< gameTimeStart() calls made to fast-forward a paused playback, to undo
bool audioMuted = false;         ///< sound turned off while fast-forwarding
bool replayEnded = false;

enum class RingMode
{
	Default,  ///< with the GUI only, default options
	On,       ///< always, with ringOptions
	Off,
};
RingMode ringMode = RingMode::Default;
SnapshotRingOptions ringOptions;
bool ringStartedForPlayback = false;

// A restart from the start: asked for (the game is to be quit), then carried out once the replay plays again
struct Restart
{
	std::string file;
	uint32_t target = 0;
	bool resumePaused = false;
	bool launchedFromCommandLine = false;
};
nonstd::optional<Restart> restartToQuit;   ///< the game loop is to quit the game for it
nonstd::optional<Restart> restartToSeek;   ///< the replay is loading again: seek once it plays

std::function<void(GameStateSnapshot &, unsigned)> restoreHook;
unsigned restoreHookAttempts = 0;

double msSince(std::chrono::steady_clock::time_point start)
{
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

bool ringWanted()
{
	switch (ringMode)
	{
	case RingMode::On: return true;
	case RingMode::Off: return false;
	case RingMode::Default: break;
	}
	return !headlessGameMode();
}

// As the replay controller pauses playback
void pausePlayback()
{
	if (!gameTimeIsStopped())
	{
		gameTimeStop();
	}
	setAudioPause(true);
	setConsolePause(true);
}

// Fast-forwarding needs game time running and is better without sound
void beginFastForward()
{
	while (gameTimeIsStopped())
	{
		gameTimeStart();
		++timeStartsOwed;
	}
	if (!headlessGameMode() && !audioMuted)
	{
		audioMuted = true;
		audio_StopAll();
		audio_setPause(true);
	}
}

void endFastForward()
{
	if (audioMuted)
	{
		audioMuted = false;
		audio_StopAll();
		audio_setPause(false);
	}
	for (; timeStartsOwed > 0; --timeStartsOwed)
	{
		gameTimeStop();
	}
	if (resumePaused)
	{
		pausePlayback();
	}
	resumePaused = false;
}

// Play the replay again from the start, then seek to the target: the last resort
void startRestart(const char *why)
{
	const std::string &file = NETreplayFilename();
	if (file.empty())
	{
		debug(LOG_ERROR, "Replay seek to gameTime %u failed: %s, and the replay can't be played again from the start (its file is unknown)%s",
		      target, why, gameStateRestoreFailed() ? "; the game is stopped" : "; playback goes on");
		state = SeekState::None;
		endFastForward();
		return;
	}
	debug(LOG_INFO, "Replay seek to gameTime %u: %s; playing the replay again from the start", target, why);
	Restart restart;
	restart.file = file;
	restart.target = target;
	restart.resumePaused = resumePaused;
	restart.launchedFromCommandLine = getHostLaunch() == HostLaunch::LoadReplay;  // (reset when the game shuts down)
	restartToQuit = restart;
	state = SeekState::None;
	resumePaused = false;
	endFastForward();
}

// Restore the newest snapshot that can be restored, at or before the target: true if one was. On false, the seek has
// been handed to startRestart(), or fast-forwards from the live game.
bool restoreForSeek()
{
	uint32_t searchAt = target;
	while (true)
	{
		std::string error;
		nonstd::optional<GameStateSnapshot> snapshot = snapshotRingFindAtOrBefore(searchAt, &error);
		if (!snapshot.has_value())
		{
			if (!gameStateRestoreFailed() && target >= gameTime)
			{
				// A seek forward whose snapshots on the way couldn't be restored: playback is still before the target
				debug(LOG_INFO, "Replay seek to gameTime %u: no snapshot left to restore (%s); fast-forwarding from gameTime %u",
				      target, error.c_str(), gameTime);
				return false;
			}
			startRestart(restoreAttempts == 0 ? "no snapshot to restore" : "no snapshot left that can be restored");
			return false;
		}
		if (snapshot->gameTime <= gameTime && target >= gameTime && !gameStateRestoreFailed())
		{
			// Older than playback, which can go on to the target itself
			debug(LOG_INFO, "Replay seek to gameTime %u: fast-forwarding from gameTime %u instead of restoring an older snapshot", target, gameTime);
			return false;
		}
		const uint32_t snapshotTime = snapshot->gameTime;
		if (restoreHook)
		{
			restoreHook(snapshot.value(), restoreHookAttempts);
		}
		++restoreHookAttempts;
		++restoreAttempts;
		const uint32_t from = gameTime;
		const auto restoreStart = std::chrono::steady_clock::now();
		const GameStateRestoreResult result = restoreGameStateSnapshot(snapshot.value(), &error);
		const double ms = msSince(restoreStart);
		restoreMs += ms;
		if (result == GameStateRestoreResult::Restored)
		{
			replayEnded = false;  // (the replay's game queues were put back)
			debug(LOG_INFO, "Replay seek to gameTime %u: restored the snapshot at gameTime %u (from gameTime %u, %.1f ms)",
			      target, gameTime, from, ms);
			return true;
		}
		if (result == GameStateRestoreResult::Rejected)
		{
			debug(LOG_ERROR, "Replay seek to gameTime %u: the snapshot at gameTime %u was rejected (%s)", target, snapshotTime, error.c_str());
		}
		else
		{
			debug(LOG_ERROR, "Replay seek to gameTime %u: restoring the snapshot at gameTime %u failed at stage %s (%s); the game is stopped until a restore succeeds",
			      target, snapshotTime, gameStateLastRestoreStage(), error.c_str());
		}
		snapshotRingDrop(snapshotTime);  // (it would fail again)
		if (snapshotTime == 0)
		{
			startRestart("no snapshot left that can be restored");
			return false;
		}
		searchAt = snapshotTime - 1;
	}
}

void startSeek()
{
	startedAt = std::chrono::steady_clock::now();
	restoreMs = 0;
	restoreAttempts = 0;
	beginFastForward();
	// Not before the first snapshot (the start of playback)
	const std::vector<uint32_t> times = snapshotRingTimes();
	if (!times.empty())
	{
		target = std::max(target, times.front());
	}
	// Restore unless playback can simply go on to the target: it is at or before it, and the ring has nothing newer
	// than playback on the way
	bool restore = target < gameTime;
	for (uint32_t t : times)
	{
		restore = restore || (t > gameTime && t <= target);
	}
	if (restore && !restoreForSeek() && state == SeekState::None)
	{
		return;  // restarting from the start, or given up
	}
	fastForwardFrom = gameTime;
	state = SeekState::FastForwarding;
}

void finishSeek(const char *how)
{
	debug(LOG_INFO, "Replay seek to gameTime %u: %s at gameTime %u, requested at gameTime %u; fast-forwarded %u tick(s) from gameTime %u; "
	      "%.1f ms in all (%u restore(s), %.1f ms)",
	      target, how, gameTime, requestedAt, (gameTime - fastForwardFrom) / GAME_TICKS_PER_UPDATE, fastForwardFrom, msSince(startedAt),
	      restoreAttempts, restoreMs);
	state = SeekState::None;
	endFastForward();
}

// A seek starts at a capture point, or once the replay has ended (no message for another tick will come),
// or after a restore failed half way (the seek restores again, or plays the replay from the start)
bool seekCanStart()
{
	return !levelChangePending() && (gameStateCaptureSafe() || replayEnded || gameStateRestoreFailed());
}

} // anonymous namespace

void setReplaySnapshotRing(const SnapshotRingOptions &options)
{
	ringMode = RingMode::On;
	ringOptions = options;
}

void disableReplaySnapshotRing()
{
	ringMode = RingMode::Off;
}

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
	if (restartToQuit.has_value())
	{
		return fail("the replay is about to be played again from the start");
	}
	if (state == SeekState::None)
	{
		resumePaused = gameTimeIsStopped();
	}
	else
	{
		debug(LOG_INFO, "Replay seek to gameTime %u: replaced by a seek to gameTime %u, at gameTime %u", target, targetGameTime, gameTime);
	}
	target = targetGameTime;
	// Not past the replay's end (nor before the first snapshot: see startSeek())
	if (replayEndGameTime() != 0)
	{
		target = std::min(target, replayEndGameTime());
	}
	requestedAt = gameTime;
	state = SeekState::Requested;
	return true;
}

void replaySeekAtCapturePoint()
{
	if (!NETisReplay())
	{
		return;
	}
	if (!ringStartedForPlayback)
	{
		ringStartedForPlayback = true;
		if (ringWanted())
		{
			snapshotRingStart(ringOptions);
		}
	}
	if (restartToSeek.has_value() && gameStateCaptureSafe())
	{
		const Restart restart = restartToSeek.value();
		restartToSeek.reset();
		std::string error;
		if (requestReplaySeek(restart.target, &error))
		{
			resumePaused = restart.resumePaused;
			debug(LOG_INFO, "Replay seek to gameTime %u: the replay plays again from the start", target);
		}
		else
		{
			debug(LOG_ERROR, "Replay seek to gameTime %u failed after playing the replay again from the start: %s", restart.target, error.c_str());
		}
	}
	if (state == SeekState::Requested && seekCanStart() && !restartToQuit.has_value())
	{
		startSeek();
	}
	if (state == SeekState::FastForwarding)
	{
		if (gameTime >= target)
		{
			finishSeek("reached");
		}
		else if (replayEnded)
		{
			finishSeek("reached the replay's end");
		}
	}
}

bool replaySeekFastForwarding()
{
	return state == SeekState::FastForwarding && gameTime < target && !replayEnded;
}

void replaySeekStop()
{
	if (state == SeekState::Requested)
	{
		debug(LOG_INFO, "Replay seek to gameTime %u: cancelled before it started", target);
		state = SeekState::None;
		endFastForward();  // (if it replaced a seek that was fast-forwarding)
		return;
	}
	if (state == SeekState::FastForwarding)
	{
		finishSeek("stopped");
	}
}

void replaySeekCancel()
{
	state = SeekState::None;
	resumePaused = false;  // (the game is shutting down: nothing to pause again)
	endFastForward();
	replayEnded = false;
	ringStartedForPlayback = false;
}

void noteReplayEnded()
{
	replayEnded = true;
}

ReplaySeekStatus replaySeekStatus()
{
	ReplaySeekStatus status;
	status.restarting = restartToQuit.has_value() || restartToSeek.has_value();
	status.busy = state != SeekState::None || status.restarting;
	status.target = status.restarting ? (restartToQuit.has_value() ? restartToQuit->target : restartToSeek->target) : target;
	status.from = state == SeekState::FastForwarding ? fastForwardFrom : gameTime;
	status.ended = replayEnded;
	return status;
}

uint32_t replayEndGameTime()
{
	return NETisReplay() ? NETreplayEndGameTime() : 0;
}

bool replayRestartPending()
{
	return restartToQuit.has_value();
}

bool takeReplayRestart(std::string &file, bool &launchedFromCommandLine)
{
	if (!restartToQuit.has_value())
	{
		return false;
	}
	restartToSeek = restartToQuit;
	restartToQuit.reset();
	file = restartToSeek->file;
	launchedFromCommandLine = restartToSeek->launchedFromCommandLine;
	return true;
}

void setReplaySeekRestoreHook(std::function<void(GameStateSnapshot &snapshot, unsigned attempt)> hook)
{
	restoreHook = std::move(hook);
}

} // namespace gamestate
