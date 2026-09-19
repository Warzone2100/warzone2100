/*
	This file is part of Warzone 2100.
	Copyright (C) 2023  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_MULTIPLAYDEFS_H__
#define __INCLUDED_SRC_MULTIPLAYDEFS_H__

#include "lib/framework/frame.h"
#include "lib/framework/types.h"
#include "lib/netplay/nettypes.h"

#include <algorithm>
#include <cstdint>

// The behavior used once a player is considered to have *left* a game in progress
// (i.e. after any reconnect window, see PLAYER_RECONNECT_WAIT_* below, has elapsed).
enum class PLAYER_LEAVE_MODE : uint8_t
{
	DESTROY_RESOURCES = 0,
	SPLIT_WITH_TEAM = 1
};

constexpr PLAYER_LEAVE_MODE PLAYER_LEAVE_MODE_MAX = PLAYER_LEAVE_MODE::SPLIT_WITH_TEAM;

constexpr PLAYER_LEAVE_MODE PLAYER_LEAVE_MODE_DEFAULT = PLAYER_LEAVE_MODE::SPLIT_WITH_TEAM;

// The maximum number of seconds the host will hold a dropped player's slot (resources kept
// intact) waiting for them to reconnect mid-match (via a state snapshot) before declaring them
// left and applying the configured PLAYER_LEAVE_MODE. 0 = do not wait (declare left immediately).
constexpr uint16_t PLAYER_RECONNECT_WAIT_SECONDS_MAX = 300;     // 5 minutes
constexpr uint16_t PLAYER_RECONNECT_WAIT_SECONDS_DEFAULT = 30;

static inline uint16_t clampPlayerReconnectWaitSeconds(uint32_t seconds)
{
	return static_cast<uint16_t>(std::min<uint32_t>(seconds, PLAYER_RECONNECT_WAIT_SECONDS_MAX));
}

enum class BLIND_MODE : uint8_t
{
	NONE,
	BLIND_LOBBY,	// standard blind mode lobby (players' true identities are hidden from everyone except a spectator host until the game starts)
	BLIND_LOBBY_SIMPLE_LOBBY,	// BLIND_LOBBY, but not showing any other players in lobby (players will just be informed that they are waiting for other players)
	BLIND_GAME,		// standard blind mode game (players' true identities are hidden from everyone except a spectator host until the game is over)
	BLIND_GAME_SIMPLE_LOBBY		// BLIND_GAME, but not showing any other players in lobby (players will just be informed that they are waiting for other players)
};

constexpr BLIND_MODE BLIND_MODE_MAX = BLIND_MODE::BLIND_GAME_SIMPLE_LOBBY;

static inline bool isBlindSimpleLobby(BLIND_MODE mode) { return mode == BLIND_MODE::BLIND_LOBBY_SIMPLE_LOBBY || mode == BLIND_MODE::BLIND_GAME_SIMPLE_LOBBY; }

#endif // __INCLUDED_SRC_MULTIPLAYDEFS_H__
