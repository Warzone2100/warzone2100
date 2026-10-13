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
 *  Replay seek: jump to another game time while playing a replay back, backwards or forwards,
 *  using the snapshot ring (gamestate_snapshot_ring.h).
 *
 *  A seek is done at the next capture point.
 *  If playback is past the target, or the ring has a snapshot between playback and the target,
 *  the newest snapshot at or before the target is restored (restoreGameStateSnapshot).
 *  Then the game fast-forwards to the target, ticking without waiting for real time
 *  (the game loop asks replaySeekFastForwarding()).
 *
 *  The viewer's view goes on across a seek: the camera stays where it is,
 *  the selection is kept where its objects exist at the target,
 *  and what showed objects of the timeline left behind is closed (uiobjectrefs.h).
 */

#pragma once

#include <cstdint>
#include <string>

namespace gamestate
{

/// Ask for a seek to the given game time. False (error says why) if it can't be done: not playing a replay back, or no
/// snapshot ring. A seek already pending or fast-forwarding is replaced.
bool requestReplaySeek(uint32_t targetGameTime, std::string *error = nullptr);

/// At the capture point of every game-loop iteration (main thread), before the snapshot ring's: start a requested
/// seek, and notice a fast-forward that has reached its target.
void replaySeekAtCapturePoint();

/// Whether the game loop should tick without waiting for real time: a seek is fast-forwarding to its target.
bool replaySeekFastForwarding();

/// Drop a pending or running seek (when the game shuts down).
void replaySeekCancel();

} // namespace gamestate
