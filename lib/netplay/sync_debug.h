/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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

#pragma once

#include "lib/framework/wzglobal.h"
#include "lib/framework/frame.h"

#include <stddef.h>
#include <stdint.h>
#include <string>

/// Sync debugging. Only prints anything, if different players would print different things.
#define syncDebug(...) do { _syncDebug(__FUNCTION__, __VA_ARGS__); } while(0)
void _syncDebug(const char* function, const char* str, ...) WZ_DECL_FORMAT(WZ_PRINTF_FORMAT, 2, 3);

/// Faster than syncDebug. Make sure that str is a format string that takes ints only.
void _syncDebugIntList(const char* function, const char* str, int* ints, size_t numInts);
#define syncDebugBacktrace() do { _syncDebugBacktrace(__FUNCTION__); } while(0)
void _syncDebugBacktrace(const char* function);                  ///< Adds a backtrace to syncDebug, if the platform supports it. Can be a bit slow, don't call way too often, unless desperate.
uint32_t syncDebugGetCrc();                                      ///< syncDebug() calls between uint32_t crc = syncDebugGetCrc(); and syncDebugSetCrc(crc); appear in synch debug logs, but without triggering a desynch if different.
void syncDebugSetCrc(uint32_t crc);                              ///< syncDebug() calls between uint32_t crc = syncDebugGetCrc(); and syncDebugSetCrc(crc); appear in synch debug logs, but without triggering a desynch if different.

/// Snapshot resume support: a tick's object syncDebug is attributed to the NEXT sync boundary's CRC
/// (sendPlayerGameTime/nextDebugSync run mid-gameStateUpdate, after the headers but before the object
/// updates). A client restored from a GameState snapshot reset its sync log, so its first post-resume
/// boundary CRC is missing the saved tick's object contributions that a continuously-playing host folded
/// in - making that one boundary's CRC (and its echoed GAME_GAME_TIME checkCrc) diverge. Capture
/// syncDebugGetCrc() at save time into the snapshot; on restore, after resetSyncDebug(), seed it back so
/// the first post-resume boundary reproduces the host's CRC exactly.
void setResumeSyncDebugCrc(uint32_t crc);                          ///< Stash the accumulator CRC captured at the snapshot's save tick (no-op until applyResumeSyncDebugCrc()).
void applyResumeSyncDebugCrc();                                    ///< If a resume CRC was stashed, seed the (freshly reset) accumulator with it. Consumes the stash.

typedef uint16_t GameCrcType;  // Truncate CRC of game state to 16 bits, to save a bit of bandwidth.
void resetSyncDebug();                                              ///< Resets the syncDebug, so syncDebug from a previous game doesn't cause a spurious desynch dump.
GameCrcType nextDebugSync();                                        ///< Returns a CRC corresponding to all syncDebug() calls since the last nextDebugSync() or resetSyncDebug() call.
bool checkDebugSync(uint32_t checkGameTime, GameCrcType checkCrc);  ///< Dumps all syncDebug() calls from that gameTime, if the CRC doesn't match.

/// Per-tick sync-CRC trace, for the cross-process load sync test. When a trace
/// file is set, the local per-tick game-state CRC is appended each tick as "<gameTime> <crc>\n".
/// Two clients (host + mid-match joiner) each writing a trace should produce identical lines for
/// every gameTime they both simulate; the first differing line is the desync tick. Empty path
/// disables tracing.
void setSyncCrcTraceFile(const std::string &filename);
bool syncCrcTraceActive();                                        ///< True iff a sync-CRC trace file is open. Used to switch on deterministic, wall-clock-free latency negotiation so two independent runs' traces stay comparable.
void syncCrcTraceRecord(uint32_t atGameTime, GameCrcType crc);     ///< Append one (gameTime, crc) line if tracing is enabled; no-op otherwise.
void setSyncCrcDetailTick(uint32_t tick);                         ///< At this gameTime, dump the full per-tick sync-debug log to "<tracefile>.detail.txt" (0 = disabled). Diff the original-run vs loaded-run detail to pinpoint exactly which object/subsystem/field diverges.
void setSyncCrcDetailOnSave(int numTicks);                        ///< Enable auto-dump: arm a window of `numTicks` detailed dumps whenever a GameState savegame is written or restored (0 = disabled). Avoids having to know the save tick up front.
void syncCrcDetailArmOnSaveOrLoad();                              ///< Call from the GameState save/cold-load path to arm the auto-dump window (no-op unless setSyncCrcDetailOnSave was enabled).


// Set whether verbose debug mode - outputting the current player's sync log for every single game tick - is enabled until a specific gameTime value
// WARNING: This may significantly impact performance *and* will fill up your drive with a lot of logs data!
// It is only intended to be used for debugging issues such as: replays desyncing when gameplay does not, etc. (And don't let the game run too long / set untilGameTime too high!)
void NET_setDebuggingModeVerboseOutputAllSyncLogs(uint32_t untilGameTime = 0);
void debugVerboseLogSyncIfNeeded();

struct NETQUEUE;

void recvDebugSync(NETQUEUE queue);
