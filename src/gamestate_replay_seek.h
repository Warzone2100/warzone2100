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
 *  (the game loop asks replaySeekFastForwarding()). A seek past the replay's end stops there.
 *
 *  If a snapshot can't be restored, it is dropped from the ring and the next older one is tried
 *  (also after a restore that failed half way: the next restore replaces everything again).
 *  If none can be, the replay is played again from the start: the game is quit and the replay file loaded again,
 *  as the menu does, and the seek then fast-forwards from the start.
 *
 *  The ring runs during every replay playback with the GUI, from its first capture point
 *  (see setReplaySnapshotRing()).
 *  A playback that was paused is paused again once the seek is done. Sound is off while fast-forwarding.
 *
 *  The viewer's view goes on across a seek: the camera stays where it is,
 *  the selection is kept where its objects exist at the target,
 *  and what showed objects of the timeline left behind is closed (uiobjectrefs.h).
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>

struct GameStateSnapshot;

namespace gamestate
{

struct SnapshotRingOptions;

/// Run the snapshot ring during every replay playback, headless too, with these options (--gamestate-snapshot-ring).
/// Without this, it runs during playback with the GUI only, with the default options.
void setReplaySnapshotRing(const SnapshotRingOptions &options);
/// Don't run the snapshot ring during replay playback at all (--gamestate-snapshot-ring=off): a seek back then plays
/// the replay again from the start.
void disableReplaySnapshotRing();

/// Ask for a seek to the given game time. False (error says why) if it can't be done: not playing a replay back, or
/// a restart from the start already under way. A seek already pending or fast-forwarding is replaced.
bool requestReplaySeek(uint32_t targetGameTime, std::string *error = nullptr);

/// At the capture point of every game-loop iteration (main thread), before the snapshot ring's: start the ring at the
/// start of playback, start a requested seek, and notice a fast-forward that is done.
void replaySeekAtCapturePoint();

/// Whether the game loop should tick without waiting for real time: a seek is fast-forwarding to its target.
bool replaySeekFastForwarding();

/// Stop a pending or running seek where it is, as if it had reached its target (the viewer canceled it).
void replaySeekStop();

/// Drop a pending or running seek (when the game shuts down). A restart from the start under way is kept.
void replaySeekCancel();

/// The REPLAY_ENDED message was processed: playback is at the replay's end (until a seek back).
void noteReplayEnded();

struct ReplaySeekStatus
{
	bool busy = false;           ///< a seek is pending, fast-forwarding, or restarting the replay
	bool restarting = false;     ///< playing the replay again from the start
	uint32_t target = 0;         ///< the game time it goes to
	uint32_t from = 0;           ///< the game time the fast-forward started at
	bool ended = false;          ///< playback is at the replay's end
};
ReplaySeekStatus replaySeekStatus();

/// The game time at which the replay being played back ends (0: unknown).
uint32_t replayEndGameTime();

/// Whether the game should be quit to play the replay again from the start (the game loop then returns
/// GAMECODE_QUITGAME).
bool replayRestartPending();
/// After the game was quit for a restart (from the title loop): take the request. False if there is none.
/// Else the replay file to load again, and whether the replay was started from the command line (--loadreplay),
/// which a load from the menu must be told again
/// (it decides whether a headless game runs unthrottled and quits at the end).
bool takeReplayRestart(std::string &file, bool &launchedFromCommandLine);

/// Test-only (--gamestate-seek-fault): called with every snapshot a seek is about to restore, and the number of
/// restores the seeks have tried before (from 0), to damage it. Empty: none.
void setReplaySeekRestoreHook(std::function<void(GameStateSnapshot &snapshot, unsigned attempt)> hook);

} // namespace gamestate
