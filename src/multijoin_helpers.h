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

#pragma once

#include "lib/netplay/netjoin.h"

#include <string>
#include <vector>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

enum class JoinGameFromConnectionStrOutcome
{
	Attempting_IP_Join,
	Attempting_Lobby_Join
};

// Start a join with an expected ip, ip:port, hostname, or hostname:port string
// Note: Does not attempt a lobby join
void joinGameFromIPOrHostnameConnectionStr(const std::string& ipOrHostnameConnectionStr, bool asSpectator = false);

// Start a join with an explicit ip & port
void joinGameFromIPPort(const std::string& host, uint32_t port, bool asSpectator = false);

// Parses a user-provided connection string, which may be either:
// - An IP address or IP:port combination
// - A lobby game ID
optional<JoinGameFromConnectionStrOutcome> joinGameFromConnectionStr(const std::string& userProvidedConnectionStr, bool asSpectator = false);

// Explicitly joins a lobby game, with lobbyAddress and lobbyGameId
void joinLobbyGame(const std::string& lobbyAddress, const std::string& lobbyGameId, bool asSpectator = false);

// Join a game given a list of JoinConnectionDescription
void joinGame(const std::vector<JoinConnectionDescription>& connection_list, bool asSpectator = false);
