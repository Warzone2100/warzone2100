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

GameStateSnapshot takeGameStateSnapshot()
{
	GameStateSnapshot snapshot;
	const nlohmann::ordered_json document = gamestate::gameStateToJson();
	// Check it now, while the game it was taken from is at hand: a snapshot that couldn't be restored is a serializer
	// bug, better caught here than when a restore needs it
	try
	{
		gamestate::checkGameStateDocument(document);
	}
	catch (const std::exception &e)
	{
		ASSERT(false, "A snapshot of this game fails its own check: %s", e.what());
		throw;
	}
	snapshot.document = document.dump();
	// Like a savegame's pending-resume section: the game-queue messages not read yet (scheduled for later ticks by
	// the lockstep latency) belong to the snapshot's timeline
	snapshot.gameQueue.assign(MAX_GAMEQUEUE_SLOTS, {});
	for (unsigned p = 0; p < MAX_GAMEQUEUE_SLOTS; ++p)
	{
		snapshot.gameQueue[p] = NETgameQueueCapturePending(p);
	}
	snapshot.queuedOrders = captureQueuedDroidInfo();
	snapshot.gameTime = gameTime;
	return snapshot;
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
	for (unsigned p = 0; p < MAX_GAMEQUEUE_SLOTS && p < snapshot.gameQueue.size(); ++p)
	{
		if (NETgameQueueCapturePending(p).empty() && snapshot.gameQueue[p].empty())
		{
			continue;
		}
		NETinitQueue(NETgameQueue(p));
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
	return GameStateRestoreResult::Restored;
}
