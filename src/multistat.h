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

	EcKey identity;
};

struct RESEARCH;

bool saveMultiStats(const char *sFName, const char *sPlayerName, const PLAYERSTATS *playerStats);	// to disk
bool loadMultiStats(char *sPlayerName, PLAYERSTATS *playerStats);					// form disk
PLAYERSTATS const &getMultiStats(UDWORD player);									// get from net
const EcKey& getLocalSharedIdentity();
bool setMultiStats(uint32_t player, PLAYERSTATS plStats, bool bLocal);  // set + send to net.
bool clearPlayerMultiStats(uint32_t playerIndex);
bool sendMultiStats(uint32_t playerIndex, optional<uint32_t> recipientPlayerIndex = nullopt); // send to net
bool sendMultiStatsHostVerifiedIdentities(uint32_t playerIndex);
bool sendMultiStatsScoreUpdates(uint32_t player);
void updateMultiStatsDamage(UDWORD attacker, UDWORD defender, UDWORD inflicted);
void updateMultiStatsGames();
void updateMultiStatsWins();
void updateMultiStatsLoses();
void incrementMultiStatsResearchPerformance(UDWORD player);
void incrementMultiStatsResearchPotential(UDWORD player);
struct BASE_OBJECT;
void updateMultiStatsKills(BASE_OBJECT *psKilled, UDWORD player);
void updateMultiStatsBuilt(BASE_OBJECT *psBuilt);
void updateMultiStatsResearchComplete(RESEARCH *psResearch, UDWORD player);
bool recvMultiStats(NETQUEUE queue);

void multiStatsSetVerifiedIdentityFromJoin(uint32_t playerIndex, const EcKey::Key &identity);
void multiStatsSetVerifiedHostIdentityFromJoin(const EcKey::Key &identity);

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
bool updateMultiStatsIdentitiesInJSON(nlohmann::json& json, bool useVerifiedJoinIdentity = false);

bool generateBlindIdentity();

// When host, this will return:
// - The identity verified on initial player join
// When a client, this will return:
// - In "regular" lobbies, the current player identity (if it has been verified with this client)
// - In "blind" lobbies...
//   - Before game ends, an empty identity
//   - After game ends, the host-verified join identity
const EcKey& getVerifiedJoinIdentity(UDWORD player);

// In blind games, it returns the verified join identity (if executed on the host, or on all clients after the game has ended)
// In regular games, it returns the current player identity
struct TrueIdentity
{
	const EcKey& identity;
	bool verified;
};
TrueIdentity getTruePlayerIdentity(UDWORD player);

// Should be used when a player identity is to be output (in logs, or otherwise accessible to the user)
const EcKey& getOutputPlayerIdentity(UDWORD player);

#endif // __INCLUDED_SRC_MULTISTATS_H__
