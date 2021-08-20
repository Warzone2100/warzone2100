/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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

#include "chat.h"
#include "ai.h"
#include "lib/netplay/netplay.h"
#include "qtscript.h"

InGameChatMessage::InGameChatMessage(uint32_t messageSender, char const *messageText)
{
	sender = messageSender;
	text = messageText;
}

bool InGameChatMessage::isGlobal() const
{
	return !toAllies && toPlayers.empty();
}

bool InGameChatMessage::shouldReceive(uint32_t playerIndex) const
{
	ASSERT_OR_RETURN(false, playerIndex < MAX_PLAYERS, "Invalid player index: %" PRIu32 "", playerIndex);
	if ((playerIndex >= game.maxPlayers) && ((playerIndex >= NetPlay.players.size()) || (!NetPlay.players[playerIndex].allocated)))
	{
		return false;
	}
	return isGlobal() || toPlayers.find(playerIndex) != toPlayers.end() || (toAllies && aiCheckAlliances(sender, playerIndex));
}

std::vector<uint32_t> InGameChatMessage::getReceivers() const
{
	std::vector<uint32_t> receivers;

	for (auto playerIndex = 0; playerIndex < MAX_PLAYERS; playerIndex++)
	{
		if (shouldReceive(playerIndex) && openchannels[playerIndex])
		{
			receivers.push_back(playerIndex);
		}
	}

	return receivers;
}

std::string InGameChatMessage::formatReceivers() const
{
	if (isGlobal()) {
		return _("Global");
	}

	if (toAllies && toPlayers.empty()) {
		return _("Allies");
	}

	auto directs = toPlayers.begin();
	std::stringstream ss;
	if (toAllies) {
		ss << _("Allies");
	} else {
		ss << _("private to ");
		ss << getPlayerName(*directs++);
	}

	while (directs != toPlayers.end())
	{
		auto nextName = getPlayerName(*directs++);
		ss << (directs == toPlayers.end() ? _(" and ") : ", ");
		ss << nextName;
	}

	return ss.str();
}

void InGameChatMessage::sendToHumanPlayers()
{
	char formatted[MAX_CONSOLE_STRING_LENGTH];
	ssprintf(formatted, "%s (%s): %s", getPlayerName(sender), formatReceivers().c_str(), text);

	auto message = NetworkTextMessage(sender, formatted);
	message.teamSpecific = toAllies && toPlayers.empty();

	if (sender == selectedPlayer || shouldReceive(selectedPlayer)) {
		printInGameTextMessage(message);
	}

	if (isGlobal()) {
		message.enqueue(NETbroadcastQueue());
		return;
	}

	for (auto receiver: getReceivers())
	{
		if (isHumanPlayer(receiver))
		{
			message.enqueue(NETnetQueue(receiver));
		}
	}
}

void InGameChatMessage::sendToAiPlayer(uint32_t receiver)
{
	if (!ingame.localOptionsReceived)
	{
		return;
	}

	uint32_t responsiblePlayer = whosResponsible(receiver);

	if (responsiblePlayer >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "sendToAiPlayer() - responsiblePlayer >= MAX_PLAYERS");
		return;
	}

	if (!isHumanPlayer(responsiblePlayer))
	{
		debug(LOG_ERROR, "sendToAiPlayer() - responsiblePlayer is not human.");
		return;
	}

	NETbeginEncode(NETnetQueue(responsiblePlayer), NET_AITEXTMSG);
	NETuint32_t(&sender);
	NETuint32_t(&receiver);
	NETstring(text, MAX_CONSOLE_STRING_LENGTH);
	NETend();
}

void InGameChatMessage::sendToAiPlayers()
{
	for (auto receiver: getReceivers())
	{
		if (!isHumanPlayer(receiver))
		{
			if (myResponsibility(receiver))
			{
				triggerEventChat(sender, receiver, text);
			}
			else
			{
				sendToAiPlayer(receiver);
			}
		}
	}
}

void InGameChatMessage::sendToSpectators()
{
	if (!ingame.localOptionsReceived)
	{
		return;
	}

	char formatted[MAX_CONSOLE_STRING_LENGTH];
	ssprintf(formatted, "%s (%s): %s", getPlayerName(sender), _("Spectators"), text);

	if ((sender == selectedPlayer || shouldReceive(selectedPlayer)) && NetPlay.players[selectedPlayer].isSpectator) {
		auto message = NetworkTextMessage(SPECTATOR_MESSAGE, formatted);
		printInGameTextMessage(message);
	}

	for (auto receiver: getReceivers())
	{
		if (isHumanPlayer(receiver) && NetPlay.players[receiver].isSpectator && receiver != selectedPlayer)
		{
			ASSERT(!myResponsibility(receiver), "Should not be my responsibility...");
			enqueueSpectatorMessage(NETnetQueue(receiver), formatted);
		}
	}
}

void InGameChatMessage::enqueueSpectatorMessage(NETQUEUE queue, char const* formattedMsg)
{
	NETbeginEncode(queue, NET_SPECTEXTMSG);
	NETuint32_t(&sender);
	NETstring(formattedMsg, MAX_CONSOLE_STRING_LENGTH);
	NETend();
}

void InGameChatMessage::addReceiverByPosition(uint32_t playerPosition)
{
	toPlayers.insert(findPlayerIndexByPosition(playerPosition));
}

void InGameChatMessage::addReceiverByIndex(uint32_t playerIndex)
{
	toPlayers.insert(playerIndex);
}

void InGameChatMessage::send()
{
	if (NetPlay.players[selectedPlayer].isSpectator && !NetPlay.isHost)
	{
		sendToSpectators();
	}
	else
	{
		sendToHumanPlayers();
		sendToAiPlayers();
		triggerEventChat(sender, sender, text);
	}
}
