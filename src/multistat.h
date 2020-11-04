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
#include <3rdparty/json/json.hpp>
#include <map>

struct PLAYERSTATS
{
	uint32_t played = 0;  /// propagated stats.
	uint32_t wins = 0;
	uint32_t losses = 0;
	uint32_t totalKills = 0;
	uint32_t totalScore = 0;

	uint32_t recentKills = 0;  // score/kills in last game.
	uint32_t recentScore = 0;

	struct Autorating
	{
		Autorating() = default;
		Autorating(nlohmann::json const &json);

		bool valid = false;
		bool dummy = false;
		uint8_t star[3] = {0, 0, 0};
		uint8_t medal = 0;
		uint8_t level = 0;
		std::string elo;
	};
	Autorating autorating;

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

std::map<std::string, EcKey::Key> const &getKnownPlayers();
void addKnownPlayer(std::string const &name, EcKey const &key, bool override = false);

uint32_t getSelectedPlayerUnitsKilled();

#endif // __INCLUDED_SRC_MULTISTATS_H__
