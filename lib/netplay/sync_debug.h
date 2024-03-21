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

/// Sync debugging. Only prints anything, if different players would print different things.
#define syncDebug(...) do { _syncDebug(__FUNCTION__, __VA_ARGS__); } while(0)
#ifdef WZ_CC_MINGW
void _syncDebug(const char* function, const char* str, ...) WZ_DECL_FORMAT(__MINGW_PRINTF_FORMAT, 2, 3);
#else
void _syncDebug(const char* function, const char* str, ...) WZ_DECL_FORMAT(printf, 2, 3);
#endif

/// Faster than syncDebug. Make sure that str is a format string that takes ints only.
void _syncDebugIntList(const char* function, const char* str, int* ints, size_t numInts);
#define syncDebugBacktrace() do { _syncDebugBacktrace(__FUNCTION__); } while(0)
void _syncDebugBacktrace(const char* function);                  ///< Adds a backtrace to syncDebug, if the platform supports it. Can be a bit slow, don't call way too often, unless desperate.
uint32_t syncDebugGetCrc();                                      ///< syncDebug() calls between uint32_t crc = syncDebugGetCrc(); and syncDebugSetCrc(crc); appear in synch debug logs, but without triggering a desynch if different.
void syncDebugSetCrc(uint32_t crc);                              ///< syncDebug() calls between uint32_t crc = syncDebugGetCrc(); and syncDebugSetCrc(crc); appear in synch debug logs, but without triggering a desynch if different.

typedef uint16_t GameCrcType;  // Truncate CRC of game state to 16 bits, to save a bit of bandwidth.
void resetSyncDebug();                                              ///< Resets the syncDebug, so syncDebug from a previous game doesn't cause a spurious desynch dump.
GameCrcType nextDebugSync();                                        ///< Returns a CRC corresponding to all syncDebug() calls since the last nextDebugSync() or resetSyncDebug() call.
bool checkDebugSync(uint32_t checkGameTime, GameCrcType checkCrc);  ///< Dumps all syncDebug() calls from that gameTime, if the CRC doesn't match.


// Set whether verbose debug mode - outputting the current player's sync log for every single game tick - is enabled until a specific gameTime value
// WARNING: This may significantly impact performance *and* will fill up your drive with a lot of logs data!
// It is only intended to be used for debugging issues such as: replays desyncing when gameplay does not, etc. (And don't let the game run too long / set untilGameTime too high!)
void NET_setDebuggingModeVerboseOutputAllSyncLogs(uint32_t untilGameTime = 0);
void debugVerboseLogSyncIfNeeded();

struct NETQUEUE;

void recvDebugSync(NETQUEUE queue);
