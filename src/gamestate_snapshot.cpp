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

#include "gamestate_snapshot.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/nettypes.h"
#include "lib/netplay/sync_debug.h"

#include "gamestate_capture.h"
#include "gamestate_serialize.h"
#include "game_world.h"
#include "map.h"
#include "multiplay.h"  // captureQueuedDroidInfo, restoreQueuedDroidInfo
#include "uiobjectrefs.h"

GameStateSnapshotCapture captureGameStateSnapshot()
{
	GameStateSnapshotCapture capture;
	capture.document = gamestate::gameStateToJson();
	GameStateSnapshot &snapshot = capture.snapshot;
	// Like a savegame's pending-resume section: the game-queue messages not read yet (scheduled for later ticks by
	// the lockstep latency) belong to the snapshot's timeline.
	// During replay playback the queues hold the rest of the replay, which is kept read-only for the whole playback:
	// keep only how far each of them has read it
	snapshot.replayReadPositions = NETreplayReadPositions();
	snapshot.gameQueue.assign(MAX_GAMEQUEUE_SLOTS, {});
	for (unsigned p = 0; p < MAX_GAMEQUEUE_SLOTS; ++p)
	{
		if (snapshot.replayReadPositions.empty() || NETreplayLoadedCount(p) == 0)
		{
			snapshot.gameQueue[p] = NETgameQueueCapturePending(p);
		}
	}
	snapshot.queuedOrders = captureQueuedDroidInfo();
	snapshot.gameTime = gameTime;
	return capture;
}

GameStateSnapshot finishGameStateSnapshot(GameStateSnapshotCapture &&capture)
{
	// Checked now, while the game it was taken from is at hand:
	// a snapshot that couldn't be restored is a serializer bug
	gamestate::checkGameStateDocument(capture.document);
	GameStateSnapshot snapshot = std::move(capture.snapshot);
	snapshot.document = capture.document.dump();
	capture.document = nlohmann::ordered_json();
	return snapshot;
}

GameStateSnapshot takeGameStateSnapshot()
{
	GameStateSnapshotCapture capture = captureGameStateSnapshot();
	try
	{
		return finishGameStateSnapshot(std::move(capture));
	}
	catch (const std::exception &e)
	{
		ASSERT(false, "A snapshot of this game fails its own check, or can't be written out: %s", e.what());
		throw;
	}
}

static bool restoreFailed = false;

bool gameStateRestoreFailed()
{
	return restoreFailed;
}

void clearGameStateRestoreFailed()
{
	restoreFailed = false;
}

GameStateRestoreResult restoreGameStateSnapshot(const GameStateSnapshot &snapshot, std::string *error)
{
	// The replay read positions are checked with the document, before anything is changed
	std::string positionsError;
	if (!snapshot.replayReadPositions.empty() && !NETreplayCheckReadPositions(snapshot.replayReadPositions, &positionsError))
	{
		if (error != nullptr)
		{
			*error = positionsError;
		}
		return GameStateRestoreResult::Rejected;
	}
	if (snapshot.replayReadPositions.empty() && NETisReplay())
	{
		if (error != nullptr)
		{
			*error = "a snapshot taken outside replay playback";
		}
		return GameStateRestoreResult::Rejected;
	}
	// Every object is about to be replaced: what the viewer sees of them is closed, and the selection kept by id
	const UiViewerState viewerState = uiBeforeObjectsReplaced();
	try
	{
		gamestate::deserializeGameState(snapshot.document, gamestate::ScriptScope::AllInstances, /*inProcess=*/true);
	}
	catch (const std::exception &e)
	{
		if (error != nullptr)
		{
			*error = e.what();
		}
		if (gamestate::gameStateLastRestoreStage() == nullptr)
		{
			return GameStateRestoreResult::Rejected;
		}
		// Half restored: objects may point at freed ones (the live projectiles until the sections stage, for one), so
		// the game must neither tick nor be drawn nor be saved
		restoreFailed = true;
		debug(LOG_ERROR, "In-process restore failed at stage %s; the game is stopped", gamestate::gameStateLastRestoreStage());
		return GameStateRestoreResult::Failed;
	}
	// Replace the live game-queue contents (messages for the abandoned timeline's future ticks) with the snapshot's,
	// as the cold load does with a savegame's pending-resume section
	if (!snapshot.replayReadPositions.empty() && !NETreplayRestoreReadPositions(snapshot.replayReadPositions))
	{
		// The positions were checked above, so only a kept message that can't be read back gets here.
		// The game state is the snapshot's but the replay queues aren't.
		if (error != nullptr)
		{
			*error = "the replay's game queues couldn't be put back";
		}
		restoreFailed = true;
		debug(LOG_ERROR, "In-process restore failed putting the replay's game queues back; the game is stopped");
		return GameStateRestoreResult::Failed;
	}
	for (unsigned p = 0; p < MAX_GAMEQUEUE_SLOTS && p < snapshot.gameQueue.size(); ++p)
	{
		if (!snapshot.replayReadPositions.empty() && NETreplayLoadedCount(p) != 0)
		{
			continue;  // put back from the replay read positions
		}
		if (NETgameQueueCapturePending(p).empty() && snapshot.gameQueue[p].empty())
		{
			continue;
		}
		NETgameQueueReset(p);
		NETgameQueueRestorePending(p, snapshot.gameQueue[p]);
	}
	restoreQueuedDroidInfo(snapshot.queuedOrders);
	// As the cold load does after its reconstruct: restart the skirmish danger worker
	// (the restore stopped it, and mapInit keeps the restored danger maps),
	// and restart the sync-debug accumulator from the snapshot's CRC
	mapInit(gameWorld);
	resetSyncDebug();
	applyResumeSyncDebugCrc();
	setSyncCheckFloorTime(gameTime);
	syncCrcDetailArmOnSaveOrLoad();
	// The game is at the snapshot's capture point again: no game message may be read before the next tick
	gameStateCaptureNoteRestore();
	uiAfterObjectsReplaced(viewerState);
	// A restore that succeeds replaces everything, a half-restored world left by one that failed before included
	restoreFailed = false;
	return GameStateRestoreResult::Restored;
}
