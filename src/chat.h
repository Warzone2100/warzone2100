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

#ifndef __INCLUDED_SRC_CHAT_H__
#define __INCLUDED_SRC_CHAT_H__

#include <set>
#include <vector>
#include <sstream>
#include "multiplay.h"

struct InGameChatMessage
{
	uint32_t sender;
	char const *text;
	bool toAllies = false;

	InGameChatMessage(uint32_t messageSender, char const *messageText);
	void send();
	void addReceiverByPosition(uint32_t playerPosition);
	void addReceiverByIndex(uint32_t playerIndex);

private:
	std::set<uint32_t> toPlayers;

	bool isGlobal() const;
	bool shouldReceive(uint32_t playerIndex) const;
	std::vector<uint32_t> getReceivers() const;
	std::string formatReceivers() const;
	void sendToHumanPlayers();
	void sendToAiPlayers();
	void sendToAiPlayer(uint32_t receiver);
	void sendToSpectators();
	void enqueueSpectatorMessage(NETQUEUE queue, char const* formattedMsg);
};

#endif // __INCLUDED_SRC_CHAT_H__
