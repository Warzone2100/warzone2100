/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2021  Warzone 2100 Project

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

#include <cstdint>
#include "multiplay.h"

#define LOBBY_COMMAND_PREFIX "/"
#define LOBBY_COMMAND_PREFIX_LENGTH 1

class HostLobbyOperationsInterface
{
public:
	virtual ~HostLobbyOperationsInterface();

public:
	virtual bool changeTeam(uint32_t player, uint8_t team) = 0;
	virtual bool changePosition(uint32_t player, uint8_t position) = 0;
	virtual bool changeBase(uint8_t baseValue) = 0;
	virtual bool changeAlliances(uint8_t allianceValue) = 0;
	virtual bool changeScavengers(uint8_t scavsValue) = 0;
	virtual bool kickPlayer(uint32_t player_id, const char *reason, bool ban) = 0;
	virtual bool movePlayerToSpectators(uint32_t player_id) = 0;
	virtual bool requestMoveSpectatorToPlayers(uint32_t player_id) = 0;
	virtual void quitGame(int exitCode) = 0;
};

bool processChatLobbySlashCommands(const NetworkTextMessage& message, HostLobbyOperationsInterface& cmdInterface);

bool addLobbyAdminIdentityHash(const std::string& playerIdentityHash);
bool removeLobbyAdminIdentityHash(const std::string& playerIdentityHash);

bool addLobbyAdminPublicKey(const std::string& publicKeyB64Str);
bool removeLobbyAdminPublicKey(const std::string& publicKeyB64Str);
