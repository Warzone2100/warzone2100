/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/** @file
 *  Definitions for multi player statistics and scores for league tables.
 *  Also Definitions for saved Arena Forces to enable teams to be saved to disk
 */

#ifndef __INCLUDED_SRC_MULTISTATS_H__
#define __INCLUDED_SRC_MULTISTATS_H__

#include "lib/netplay/netplay.h"
#include <nlohmann/json_fwd.hpp>
#include <map>
#include <chrono>
#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

#define WZ_DEFAULT_PUBLIC_RATING_LOOKUP_SERVICE_URL "https://wz2100-autohost.net/rating/"

enum RATING_SOURCE {
	RATING_SOURCE_LOCAL,
	RATING_SOURCE_HOST
};

struct PLAYERSTATS
{
	uint32_t played = 0;  /// propagated stats.
	uint32_t wins = 0;
	uint32_t losses = 0;
	uint32_t totalKills = 0;
	uint32_t totalScore = 0;

	uint32_t recentKills = 0;  // score/kills in last game.
	uint32_t recentScore = 0;
	uint64_t recentPowerLost = 0;  // power lost in last game (i.e. from droids / structures being killed by other players)

	struct Autorating
	{
		Autorating() = default;
		Autorating(nlohmann::json const &json);

		bool valid = false;
		bool dummy = false;
		bool autohoster = false;
		uint8_t star[3] = {0, 0, 0};
		uint8_t medal = 0;
		uint8_t level = 0;
		std::string elo;
		std::string details;
	};
	Autorating autorating;
	RATING_SOURCE autoratingFrom = RATING_SOURCE_HOST;

	EcKey identity;
};

bool saveMultiStats(const char *sFName, const char *sPlayerName, const PLAYERSTATS *playerStats);	// to disk
bool loadMultiStats(char *sPlayerName, PLAYERSTATS *playerStats);					// form disk
PLAYERSTATS const &getMultiStats(UDWORD player);									// get from net
bool setMultiStats(uint32_t player, PLAYERSTATS plStats, bool bLocal);  // send to net.
void updateMultiStatsDamage(UDWORD attacker, UDWORD defender, UDWORD inflicted);
void updateMultiStatsGames();
void updateMultiStatsWins();
void updateMultiStatsLoses();
void updateMultiStatsKills(BASE_OBJECT *psKilled, UDWORD player);
void recvMultiStats(NETQUEUE queue);
void lookupRatingAsync(uint32_t playerIndex);

bool swapPlayerMultiStatsLocal(uint32_t playerIndexA, uint32_t playerIndexB);

void initKnownPlayers();
void shutdownKnownPlayers();
bool isLocallyKnownPlayer(std::string const &name, EcKey const &key);
void addKnownPlayer(std::string const &name, EcKey const &key, bool override = false);
struct StoredPlayerOptions
{
	optional<std::chrono::system_clock::time_point> mutedTime;
	optional<std::chrono::system_clock::time_point> bannedTime;
};
optional<StoredPlayerOptions> getStoredPlayerOptions(std::string const &name, EcKey const &key);
void storePlayerMuteOption(std::string const &name, EcKey const &key, bool muted);

uint32_t getMultiPlayUnitsKilled(uint32_t player);
void setMultiPlayUnitsKilled(uint32_t player, uint32_t kills);
uint32_t getMultiPlayRecentScore(uint32_t player);
void setMultiPlayRecentScore(uint32_t player, uint32_t score);
uint32_t getSelectedPlayerUnitsKilled();
void resetRecentScoreData();

bool saveMultiStatsToJSON(nlohmann::json& json);
bool loadMultiStatsFromJSON(const nlohmann::json& json);

#endif // __INCLUDED_SRC_MULTISTATS_H__
