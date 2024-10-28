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

	uint32_t recentKills = 0;  // score/kills in last game. (total kills - both droids and structures)
	uint32_t recentDroidsKilled = 0;
	uint32_t recentDroidsLost = 0;
	uint32_t recentDroidsBuilt = 0;
	uint32_t recentStructuresKilled = 0;
	uint32_t recentStructuresLost = 0;
	uint32_t recentStructuresBuilt = 0;
	uint32_t recentScore = 0;
	uint32_t recentResearchComplete = 0;
	uint64_t recentPowerLost = 0;  // power lost in last game (i.e. from droids / structures being killed by other players)
	uint64_t recentDroidPowerLost = 0;	// power lost in last game (from droids being killed by other players)
	uint64_t recentStructurePowerLost = 0;	// power lost in last game (from structures being killed by other players)
	uint64_t recentPowerWon = 0;  // power that was destroyed in last game (i.e. from droids / structures being killed)
	uint64_t recentResearchPotential = 0;  // how many labs were ticking
	uint64_t recentResearchPerformance = 0;  // how many labs were ticking with objective (researching)

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
		uint8_t altNameTextColorOverride[3] = {255, 255, 255}; // rgb
		uint8_t eloTextColorOverride[3] = {255, 255, 255}; // rgb
		std::string elo;
		std::string details;
		std::string altName;
	};
	Autorating autorating;
	RATING_SOURCE autoratingFrom = RATING_SOURCE_HOST;

	EcKey identity;
};

struct RESEARCH;

bool saveMultiStats(const char *sFName, const char *sPlayerName, const PLAYERSTATS *playerStats);	// to disk
bool loadMultiStats(char *sPlayerName, PLAYERSTATS *playerStats);					// form disk
PLAYERSTATS const &getMultiStats(UDWORD player);									// get from net
bool setMultiStats(uint32_t player, PLAYERSTATS plStats, bool bLocal);  // set + send to net.
bool sendMultiStats(uint32_t playerIndex, optional<uint32_t> recipientPlayerIndex = nullopt); // send to net
bool sendMultiStatsScoreUpdates(uint32_t player);
void updateMultiStatsDamage(UDWORD attacker, UDWORD defender, UDWORD inflicted);
void updateMultiStatsGames();
void updateMultiStatsWins();
void updateMultiStatsLoses();
void incrementMultiStatsResearchPerformance(UDWORD player);
void incrementMultiStatsResearchPotential(UDWORD player);
void updateMultiStatsKills(BASE_OBJECT *psKilled, UDWORD player);
void updateMultiStatsBuilt(BASE_OBJECT *psBuilt);
void updateMultiStatsResearchComplete(RESEARCH *psResearch, UDWORD player);
bool recvMultiStats(NETQUEUE queue);
void lookupRatingAsync(uint32_t playerIndex);

void multiStatsSetVerifiedIdentityFromJoin(uint32_t playerIndex, const EcKey::Key &identity);

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

void setMultiPlayRecentDroidsKilled(uint32_t player, uint32_t value);
void setMultiPlayRecentDroidsLost(uint32_t player, uint32_t value);
void setMultiPlayRecentDroidsBuilt(uint32_t player, uint32_t value);
void setMultiPlayRecentStructuresKilled(uint32_t player, uint32_t value);
void setMultiPlayRecentStructuresLost(uint32_t player, uint32_t value);
void setMultiPlayRecentStructuresBuilt(uint32_t player, uint32_t value);
void setMultiPlayRecentPowerLost(uint32_t player, uint64_t powerLost);
void setMultiPlayRecentDroidPowerLost(uint32_t player, uint64_t powerLost);
void setMultiPlayRecentStructurePowerLost(uint32_t player, uint64_t powerLost);
void setMultiPlayRecentPowerWon(uint32_t player, uint64_t powerWon);
void setMultiPlayRecentResearchComplete(uint32_t player, uint32_t value);
void setMultiPlayRecentResearchPotential(uint32_t player, uint64_t value);
void setMultiPlayRecentResearchPerformance(uint32_t player, uint64_t value);

void resetRecentScoreData();

bool saveMultiStatsToJSON(nlohmann::json& json);
bool loadMultiStatsFromJSON(const nlohmann::json& json);

#endif // __INCLUDED_SRC_MULTISTATS_H__
