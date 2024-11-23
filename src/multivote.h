/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2023  Warzone 2100 Project

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

#ifndef __INCLUDED_WZ_MULTI_VOTE_H__
#define __INCLUDED_WZ_MULTI_VOTE_H__

#include <cstdint>
#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

void resetLobbyChangeVoteData();
void resetLobbyChangePlayerVote(uint32_t player);
void startLobbyChangeVote();
uint8_t getLobbyChangeVoteTotal();
void sendLobbyChangeVoteData(uint8_t currentVote);

void resetKickVoteData();
bool startKickVote(uint32_t target_player_id, optional<uint32_t> requester_player_id = nullopt);
void processPendingKickVotes();
void cancelOrDismissKickVote(uint32_t targetPlayer);

void cancelOrDismissVoteNotifications();

struct NETQUEUE;
bool recvVoteRequest(NETQUEUE queue);
bool recvVote(NETQUEUE queue);

#endif // __INCLUDED_WZ_MULTI_VOTE_H
