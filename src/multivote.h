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

#include "multiplay.h"
#include "ai.h"

#include <cstdint>
#include <nlohmann/json_fwd.hpp>
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
bool recvVote(NETQUEUE queue, bool inLobby = true);

// local options

void loadMultiOptionPrefValues(const char *sPlayerName, uint32_t playerIndex);
bool saveMultiOptionPrefValues(const char *sPlayerName, uint32_t playerIndex);
nlohmann::json getMultiOptionPrefValuesJSON(uint32_t playerIndex);
void sendPlayerMultiOptPreferencesBuiltin();
void resetMultiOptionPrefValues(uint32_t player);
void resetAllMultiOptionPrefValues();
void multiOptionPrefValuesSwap(uint32_t playerIndexA, uint32_t playerIndexB);

void setMultiOptionPrefValue(ScavType val, bool pref);
void setMultiOptionPrefValue(AllianceType val, bool pref);
void setMultiOptionPrefValue(PowerSetting val, bool pref);
void setMultiOptionPrefValue(CampType val, bool pref);
void setMultiOptionPrefValue(TechLevel val, bool pref);
void setMultiOptionBuiltinPrefValues(bool pref);

bool getMultiOptionPrefValue(ScavType val);
bool getMultiOptionPrefValue(AllianceType val);
bool getMultiOptionPrefValue(PowerSetting val);
bool getMultiOptionPrefValue(CampType val);
bool getMultiOptionPrefValue(TechLevel val);

// option prefs totals

size_t getMultiOptionPrefValueTotal(ScavType val, bool playersOnly = true);
size_t getMultiOptionPrefValueTotal(AllianceType val, bool playersOnly = true);
size_t getMultiOptionPrefValueTotal(PowerSetting val, bool playersOnly = true);
size_t getMultiOptionPrefValueTotal(CampType val, bool playersOnly = true);
size_t getMultiOptionPrefValueTotal(TechLevel val, bool playersOnly = true);

#endif // __INCLUDED_WZ_MULTI_VOTE_H
