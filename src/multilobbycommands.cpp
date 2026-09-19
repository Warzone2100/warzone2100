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
/*
 * multilobbycommands.cpp
 */

#include "multilobbycommands.h"
#include "lib/netplay/netplay.h"
#include "multistat.h"
#include "multiint.h"
#include "stdinreader.h"
#include "clparse.h"
#include "hci/quickchat.h"

#include <cstring>
#include <unordered_set>

#define PREFIXED_LOBBY_COMMAND(x) LOBBY_COMMAND_PREFIX x

using WzQuickChatDataContexts::INTERNAL_LOBBY_COMMAND_RESPONSE::Command;
using WzQuickChatDataContexts::INTERNAL_LOBBY_COMMAND_RESPONSE::Context;

static std::unordered_set<std::string> lobbyAdminPublicHashStrings;
static std::unordered_set<std::string> lobbyAdminPublicKeys;

bool addLobbyAdminIdentityHash(const std::string& playerIdentityHash)
{
	lobbyAdminPublicHashStrings.insert(playerIdentityHash);
	return true;
}

bool removeLobbyAdminIdentityHash(const std::string& playerIdentityHash)
{
	return lobbyAdminPublicHashStrings.erase(playerIdentityHash) > 0;
}

bool addLobbyAdminPublicKey(const std::string& publicKeyB64Str)
{
	lobbyAdminPublicKeys.insert(publicKeyB64Str);
	return true;
}

bool removeLobbyAdminPublicKey(const std::string& publicKeyB64Str)
{
	return lobbyAdminPublicKeys.erase(publicKeyB64Str) > 0;
}

// checks for specific identity being an admin
bool identityMatchesAdmin(const EcKey& identity)
{
	std::string senderIdentityHash = identity.publicHashString();
	std::string senderPublicKeyB64 = base64Encode(identity.toBytes(EcKey::Public));
	return lobbyAdminPublicKeys.count(senderPublicKeyB64) != 0 || lobbyAdminPublicHashStrings.count(senderIdentityHash) != 0;
}

// **THIS** is the function that should be used to determine whether a sender currently has permission to execute admin commands
static bool senderHasLobbyCommandAdminPrivs(uint32_t playerIdx, bool quiet = false)
{
	if (playerIdx >= MAX_CONNECTED_PLAYERS)
	{
		return false;
	}
	if (playerIdx == NetPlay.hostPlayer && NetPlay.isHost)
	{
		// the host always has permissions
		return true;
	}

	auto trueIdentity = getTruePlayerIdentity(playerIdx);
	if (trueIdentity.identity.empty())
	{
		return false;
	}
	if (!identityMatchesAdmin(trueIdentity.identity))
	{
		// identity is not in permitted list
		return false;
	}

	// Verify the player's identity has been verified
	if (!trueIdentity.verified)
	{
		// While this player claims to have an identity that matches an admin,
		// they have not yet verified it by responding to a NET_PING with a valid signature
		if (!quiet)
		{
			auto& identity = getOutputPlayerIdentity(playerIdx);
			std::string senderIdentityHash = identity.publicHashString();
			std::string senderPublicKeyB64 = base64Encode(identity.toBytes(EcKey::Public));
			sendLobbyCommandResponse(playerIdx, Context::WaitingForSync);
			if (lobbyAdminPublicKeys.count(senderPublicKeyB64) > 0)
			{
				debug(LOG_INFO, "Received an admin check for player %" PRIu32 " that passed (public key: %s), but they have not yet verified their identity", playerIdx, senderPublicKeyB64.c_str());
			}
			else
			{
				debug(LOG_INFO, "Received an admin check for player %" PRIu32 " that passed (public identity: %s), but they have not yet verified their identity", playerIdx, senderIdentityHash.c_str());
			}
		}
		return false;
	}
	return true;
}

static void lobbyCommand_PrintHelp(uint32_t receiver)
{
	sendLobbyCommandResponse(receiver, Context::Help, Command::Help, senderHasLobbyCommandAdminPrivs(receiver, true) ? 1 : 0);
}

static std::unordered_set<size_t> getConnectedAdminPlayerIndexes()
{
	std::unordered_set<size_t> adminPlayerIndexes;
	for (size_t playerIdx = 0; playerIdx < std::min<size_t>(MAX_CONNECTED_PLAYERS, NetPlay.players.size()); ++playerIdx)
	{
		if (senderHasLobbyCommandAdminPrivs(playerIdx, true))
		{
			adminPlayerIndexes.insert(playerIdx);
		}
	}
	return adminPlayerIndexes;
}

static void lobbyCommand_Admin()
{
	uint32_t adminPlayerBitmask = 0;
	for (const auto adminPlayerIdx : getConnectedAdminPlayerIndexes())
	{
		if (adminPlayerIdx >= MAX_CONNECTED_PLAYERS)
		{
			debug(LOG_ERROR, "Invalid admin index?? %zu", adminPlayerIdx);
			continue;
		}
		adminPlayerBitmask |= (1u << adminPlayerIdx);
	}
	sendLobbyCommandResponse(nullopt, Context::AdminList, Command::Admin, adminPlayerBitmask);
}

#define ADMIN_REQUIRED_FOR_COMMAND(command) \
if (!senderHasLobbyCommandAdminPrivs(message.sender)) \
{ \
	sendLobbyCommandResponse(static_cast<uint32_t>(message.sender), Context::AdminOnly, command); \
	return false; \
}

HostLobbyOperationsInterface::~HostLobbyOperationsInterface() { }

void cmdInterfaceLogChatMsg(const NetworkTextMessage& message, const char* log_prefix, optional<std::string> _senderhash /*= nullopt*/, optional<std::string> _senderPublicKeyB64 /*= nullopt*/)
{
	if (!wz_command_interface_enabled())
	{
		return;
	}

	if (message.sender < 0)
	{
		// for now, skip system messages
		return;
	}

	ASSERT_OR_RETURN(, message.sender < MAX_CONNECTED_PLAYERS, "Invalid message.sender (%d)", message.sender);

	const auto& identity = getOutputPlayerIdentity(message.sender);
	std::string senderhash = _senderhash.value_or(identity.publicHashString(64));
	std::string senderPublicKeyB64 = _senderPublicKeyB64.value_or(base64Encode(identity.toBytes(EcKey::Public)));
	std::string senderVerifiedStatus = (ingame.VerifiedIdentity[message.sender]) ? "V" : "?";
	std::string sendername = getPlayerName(message.sender);
	std::string sendername64 = base64Encode(std::vector<unsigned char>(sendername.begin(), sendername.end()));
	std::string messagetext = message.text;
	std::string messagetext64 = base64Encode(std::vector<unsigned char>(messagetext.begin(), messagetext.end()));
	wz_command_interface_output("%s: %i %s %s %s %s %s %s\n", log_prefix, message.sender, NetPlay.players[message.sender].IPtextAddress, senderhash.c_str(), senderPublicKeyB64.c_str(), sendername64.c_str(), messagetext64.c_str(), senderVerifiedStatus.c_str());
}

bool processChatLobbySlashCommands(const NetworkTextMessage& message, HostLobbyOperationsInterface& cmdInterface)
{
	if (message.sender == SYSTEM_MESSAGE || message.sender == NOTIFY_MESSAGE || message.sender < 0)
	{
		return false;
	}

	if (strncmp(message.text, LOBBY_COMMAND_PREFIX, 1) != 0)
	{
		// Message does not start with command prefix
		return false;
	}

	if (!NetPlay.isHost)
	{
		// only the host should process lobby chat commands
		return false;
	}

	ASSERT_OR_RETURN(false, message.sender < MAX_CONNECTED_PLAYERS, "Out of bounds message sender?: %" PRIi32 "", message.sender);

	// strip /hostmsg off the front of the command, if present (i.e. if starts with "<LOBBY_COMMAND_PREFIX>hostmsg <LOBBY_COMMAND_PREFIX>")
	size_t startingCommandPosition = LOBBY_COMMAND_PREFIX_LENGTH;
	if (strncmp(&message.text[startingCommandPosition], "hostmsg " LOBBY_COMMAND_PREFIX, 7 + 1 + LOBBY_COMMAND_PREFIX_LENGTH) == 0)
	{
		startingCommandPosition += 7 + 1 + LOBBY_COMMAND_PREFIX_LENGTH;
	}

	auto posToNetPlayer = [](int a) {
		for (int i=0; i<MAX_PLAYERS; i++) {
			if (NetPlay.players[i].position == a) {
				return i;
			}
		}
		return a;
	};
	const auto& identity = getOutputPlayerIdentity(message.sender);
	std::string senderhash = identity.publicHashString(64);
	std::string senderPublicKeyB64 = base64Encode(identity.toBytes(EcKey::Public));
	debug(LOG_INFO, "message [%s] [%s]", senderhash.c_str(), message.text);
	cmdInterfaceLogChatMsg(message, "WZCHATCMD", senderhash, senderPublicKeyB64);
	if (strcmp(&message.text[startingCommandPosition], "help") == 0)
	{
		lobbyCommand_PrintHelp(static_cast<uint32_t>(message.sender));
	}
	else if (strcmp(&message.text[startingCommandPosition], "admin") == 0 || strcmp(&message.text[startingCommandPosition], "admins") == 0)
	{
		lobbyCommand_Admin();
	}
	else if (strcmp(&message.text[startingCommandPosition], "me") == 0)
	{
		sendLobbyCommandResponse(static_cast<uint32_t>(message.sender), Context::MyInfo, Command::Me);
	}
	else if (strncmp(&message.text[startingCommandPosition], "team ", 5) == 0)
	{
		ADMIN_REQUIRED_FOR_COMMAND(Command::Team);
		unsigned int s1 = 0, s2 = 0;
		int r = sscanf(&message.text[startingCommandPosition], "team %u %u", &s1, &s2);
		if (r != 2 || s1 >= MAX_PLAYERS || s2 >= MAX_PLAYERS)
		{
			sendLobbyCommandResponse(nullopt, Context::Usage, Command::Team);
			return false;
		}
		if (!cmdInterface.changeTeam(posToNetPlayer(s1), s2, message.sender))
		{
			sendLobbyCommandResponse(nullopt, Context::CommandFailed, Command::Team, (s1 << 16) | s2);
			return false;
		}

		sendLobbyCommandResponse(nullopt, Context::TeamChanged, Command::Team, (s1 << 16) | s2);
	}
	else if (strcmp(&message.text[startingCommandPosition], "hostexit") == 0)
	{
		ADMIN_REQUIRED_FOR_COMMAND(Command::HostExit);
		if (message.sender != NetPlay.hostPlayer && !lobby_slashcommands_hostexit_enabled())
		{
			sendLobbyCommandResponse(static_cast<uint32_t>(message.sender), Context::HostOnly, Command::HostExit);
			return false;
		}
		cmdInterface.quitGame(5);
	}
	else if (strncmp(&message.text[startingCommandPosition], "kick ", 5) == 0 || strncmp(&message.text[startingCommandPosition], "ban ", 4) == 0)
	{
		bool isBan = strncmp(&message.text[startingCommandPosition], "ban", 3) == 0;
		Command cmd = (!isBan) ? Command::Kick : Command::Ban;
		ADMIN_REQUIRED_FOR_COMMAND(cmd);
		std::string command = (!isBan) ? "kick" : "ban";
		unsigned int playerPos = MAX_PLAYERS + 1;
		unsigned int playerIdx = MAX_CONNECTED_PLAYERS + 1;
		std::string commandParse = command + " %u";
		int r = sscanf(&message.text[startingCommandPosition], commandParse.c_str(), &playerPos);
		if (r == 1)
		{
			playerIdx = posToNetPlayer(playerPos);
			if (playerIdx >= MAX_PLAYERS)
			{
				sendLobbyCommandResponse(nullopt, Context::Usage, cmd);
				return false;
			}
		}
		else
		{
			commandParse = command + " s%u";
			r = sscanf(&message.text[startingCommandPosition], commandParse.c_str(), &playerPos);
			if (r != 1 || playerPos >= MAX_SPECTATOR_SLOTS)
			{
				sendLobbyCommandResponse(nullopt, Context::Usage, cmd);
				return false;
			}
			playerIdx = MAX_PLAYER_SLOTS + playerPos;
			ASSERT_OR_RETURN(false, playerIdx < MAX_CONNECTED_PLAYERS, "Invalid index: %u", playerIdx);
		}
		if (playerIdx == NetPlay.hostPlayer)
		{
			// Can't kick the host...
			sendLobbyCommandResponse(nullopt, Context::CannotTargetHost, cmd);
			return false;
		}
		if (!cmdInterface.kickPlayer(playerIdx, _("Administrator has kicked you from the game."), isBan, message.sender))
		{
			sendLobbyCommandResponse(nullopt, Context::CommandFailed, cmd, ((playerIdx < MAX_PLAYER_SLOTS) ? 0 : (1u << 16)) | playerPos);
			return false;
		}
	}
	else if (strncmp(&message.text[startingCommandPosition], "swap ", 5) == 0)
	{
		ADMIN_REQUIRED_FOR_COMMAND(Command::Swap);
		unsigned int s1, s2;
		int r = sscanf(&message.text[startingCommandPosition], "swap %u %u", &s1, &s2);
		int playerIdxA = posToNetPlayer(s1);
		if (r != 2)
		{
			sendLobbyCommandResponse(nullopt, Context::Usage, Command::Swap);
			return false;
		}

		if (playerIdxA >= std::min<int>(game.maxPlayers, MAX_PLAYERS))
		{
			sendLobbyCommandResponse(nullopt, Context::InvalidValue, Command::Swap, (1u << 16) | (static_cast<uint32_t>(playerIdxA) & 0xFFFF));
			return false;
		}
		if (s2 >= game.maxPlayers)
		{
			sendLobbyCommandResponse(nullopt, Context::InvalidValue, Command::Swap, (2u << 16) | (s2 & 0xFFFF));
			return false;
		}

		if (!cmdInterface.changePosition(playerIdxA, s2, message.sender))
		{
			sendLobbyCommandResponse(nullopt, Context::CommandFailed, Command::Swap, (s1 << 16) | s2);
			return false;
		}

		sendLobbyCommandResponse(nullopt, Context::Swapped, Command::Swap, (s1 << 16) | s2);
	}
	else if (strncmp(&message.text[startingCommandPosition], "base ", 5) == 0)
	{
		ADMIN_REQUIRED_FOR_COMMAND(Command::Base);
		int s1;
		int r = sscanf(&message.text[startingCommandPosition], "base %d", &s1);
		if(r != 1)
		{
			sendLobbyCommandResponse(nullopt, Context::Usage, Command::Base);
			return false;
		}

		if (s1 < 0 || s1 > 2)
		{
			sendLobbyCommandResponse(nullopt, Context::InvalidValue, Command::Base);
			return false;
		}
		if (!cmdInterface.changeBase(static_cast<uint8_t>(s1)))
		{
			sendLobbyCommandResponse(nullopt, Context::CommandFailed, Command::Base, s1);
			return false;
		}

		sendLobbyCommandResponse(nullopt, Context::BaseSet, Command::Base, s1);
	}
	else if (strncmp(&message.text[startingCommandPosition], "alliance ", 9) == 0)
	{
		ADMIN_REQUIRED_FOR_COMMAND(Command::Alliance);
		int s1;
		int r = sscanf(&message.text[startingCommandPosition], "alliance %d", &s1);
		if (r != 1)
		{
			sendLobbyCommandResponse(nullopt, Context::Usage, Command::Alliance);
			return false;
		}
		else
		{
			if (s1 < 0 || s1 > 3)
			{
				sendLobbyCommandResponse(nullopt, Context::InvalidValue, Command::Alliance);
				return false;
			}

			uint8_t alliancesType = static_cast<uint8_t>(s1);
			// flip 2nd and 3rd values to match the order displayed in UI
			if (alliancesType == 3)
			{
				alliancesType = 2;
			}
			else if(alliancesType == 2)
			{
				alliancesType = 3;
			}
			if (!cmdInterface.changeAlliances(alliancesType))
			{
				sendLobbyCommandResponse(nullopt, Context::CommandFailed, Command::Alliance, alliancesType);
				return false;
			}

			sendLobbyCommandResponse(nullopt, Context::AllianceSet, Command::Alliance, s1);
		}
	}
	else if (strncmp(&message.text[startingCommandPosition], "scav ", 5) == 0)
	{
		ADMIN_REQUIRED_FOR_COMMAND(Command::Scav);
		int scavsValue;
		int r = sscanf(&message.text[startingCommandPosition], "scav %d", &scavsValue);
		if (r != 1)
		{
			sendLobbyCommandResponse(nullopt, Context::Usage, Command::Scav);
			return false;
		}

		if (!(scavsValue == NO_SCAVENGERS || scavsValue == SCAVENGERS || scavsValue == ULTIMATE_SCAVENGERS))
		{
			sendLobbyCommandResponse(nullopt, Context::InvalidValue, Command::Scav);
			return false;
		}

		if (!cmdInterface.changeScavengers(static_cast<uint8_t>(scavsValue)))
		{
			sendLobbyCommandResponse(nullopt, Context::CommandFailed, Command::Scav, scavsValue);
			return false;
		}

		sendLobbyCommandResponse(nullopt, Context::ScavSet, Command::Scav, scavsValue);
	}
	else if (strncmp(&message.text[startingCommandPosition], "makespec ", 9) == 0)
	{
		ADMIN_REQUIRED_FOR_COMMAND(Command::MakeSpec);
		unsigned int playerPos = MAX_PLAYERS + 1;
		int r = sscanf(&message.text[startingCommandPosition], "makespec %u", &playerPos);
		unsigned int playerIdx = posToNetPlayer(playerPos);
		if (r != 1 || playerPos >= MAX_PLAYERS)
		{
			sendLobbyCommandResponse(nullopt, Context::Usage, Command::MakeSpec);
			return false;
		}
		if (playerIdx == NetPlay.hostPlayer)
		{
			// Can't move the host...
			sendLobbyCommandResponse(nullopt, Context::CannotTargetHost, Command::MakeSpec);
			return false;
		}
		if (playerIdx == message.sender)
		{
			// Can't move self this way (or it'll prevent us from moving back) - use the UI!
			sendLobbyCommandResponse(static_cast<uint32_t>(message.sender), Context::UseUIToMoveSelf, Command::MakeSpec);
			return false;
		}
		if (!cmdInterface.movePlayerToSpectators(playerIdx))
		{
			sendLobbyCommandResponse(nullopt, Context::CommandFailed, Command::MakeSpec, playerPos);
			return false;
		}
	}
	else if (strncmp(&message.text[startingCommandPosition], "makeplayer ", 11) == 0)
	{
		ADMIN_REQUIRED_FOR_COMMAND(Command::MakePlayer);
		unsigned int playerPos = MAX_PLAYERS + 1;
		int r = sscanf(&message.text[startingCommandPosition], "makeplayer s%u", &playerPos);
		if (r != 1 || playerPos >= MAX_SPECTATOR_SLOTS)
		{
			sendLobbyCommandResponse(nullopt, Context::Usage, Command::MakePlayer);
			return false;
		}
		unsigned int playerIdx = MAX_PLAYER_SLOTS + playerPos;
		ASSERT_OR_RETURN(false, playerIdx < MAX_CONNECTED_PLAYERS, "Invalid index: %u", playerIdx);
		if (playerIdx == NetPlay.hostPlayer)
		{
			// Can't move the host...
			sendLobbyCommandResponse(nullopt, Context::CannotTargetHost, Command::MakePlayer);
			return false;
		}
		if (!cmdInterface.requestMoveSpectatorToPlayers(playerIdx))
		{
			sendLobbyCommandResponse(nullopt, Context::CommandFailed, Command::MakePlayer, playerPos);
			return false;
		}
	}
	else if (strncmp(&message.text[startingCommandPosition], "mute ", 5) == 0 || strncmp(&message.text[startingCommandPosition], "unmute ", 7) == 0)
	{
		bool isMute = strncmp(&message.text[startingCommandPosition], "mute", 4) == 0;
		Command cmd = (isMute) ? Command::Mute : Command::Unmute;
		ADMIN_REQUIRED_FOR_COMMAND(cmd);
		std::string command = (isMute) ? "mute" : "unmute";
		unsigned int playerPos = MAX_PLAYERS + 1;
		unsigned int playerIdx = MAX_CONNECTED_PLAYERS + 1;
		std::string commandParse = command + " %u";
		int r = sscanf(&message.text[startingCommandPosition], commandParse.c_str(), &playerPos);
		if (r == 1)
		{
			playerIdx = posToNetPlayer(playerPos);
			if (playerIdx >= MAX_PLAYERS)
			{
				sendLobbyCommandResponse(nullopt, Context::Usage, cmd);
				return false;
			}
		}
		else
		{
			commandParse = command + " s%u";
			r = sscanf(&message.text[startingCommandPosition], commandParse.c_str(), &playerPos);
			if (r != 1 || playerPos >= MAX_SPECTATOR_SLOTS)
			{
				sendLobbyCommandResponse(nullopt, Context::Usage, cmd);
				return false;
			}
			playerIdx = MAX_PLAYER_SLOTS + playerPos;
			ASSERT_OR_RETURN(false, playerIdx < MAX_CONNECTED_PLAYERS, "Invalid index: %u", playerIdx);
		}
		if (playerIdx == NetPlay.hostPlayer)
		{
			// Can't mute the host...
			sendLobbyCommandResponse(nullopt, Context::CannotTargetHost, cmd);
			return false;
		}
		if (!cmdInterface.changeHostChatPermissions(playerIdx, !isMute))
		{
			sendLobbyCommandResponse(nullopt, Context::CommandFailed, cmd, ((playerIdx < MAX_PLAYER_SLOTS) ? 0 : (1u << 16)) | playerPos);
			return false;
		}
	}
	else
	{
		// unrecognized command - not handled
		return false;
	}

	return true;
}
