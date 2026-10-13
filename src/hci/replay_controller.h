/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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
 *  The replay playback controls: a transport bar at the bottom of the screen with a timeline (click or drag to seek),
 *  start / back 15 s / pause / play / forward 15 s / fast-forward buttons and the time,
 *  and an overlay while a seek is under way (Esc stops it). Seeks go through gamestate_replay_seek.h.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

bool createReplayControllerOverlay();
void destroyReplayControllerOverlay();
void replayOverlayScreenSizeDidChange();

/// What the controls show and do, on gamestate_replay_seek.h and the game clock
namespace replayplayback
{

uint32_t playheadMs();          ///< the game time playback is at
uint32_t durationMs();          ///< the replay's length (at least the playhead)
bool isBusy();                  ///< a seek is under way
uint32_t seekTargetMs();        ///< the seek's target, while busy
std::vector<uint32_t> snapshotTimes();  ///< the game times a seek back is quick from
void seekTo(uint32_t timeMs);   ///< seek to that time (rounded down to a second)

/// "m:ss" (or "h:mm:ss")
std::string formatTime(uint32_t timeMs, bool useHours);
/// "m:ss / m:ss": the playhead and the length
std::string formatClockLabel(uint32_t playheadMs, uint32_t durationMs);

} // namespace replayplayback

/// Seek targets are whole seconds of game time
constexpr uint32_t REPLAY_SEEK_QUANTUM_MS = 1000;
constexpr uint32_t REPLAY_SKIP_STEP_MS = 15 * 1000;
