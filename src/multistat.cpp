/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
 * MultiStat.c
 *
 * Alex Lee , pumpkin studios, EIDOS
 *
 * load / update / store multiplayer statistics for league tables etc...
 */

#include "lib/framework/frame.h"
#include "lib/framework/file.h"
#include "lib/netplay/nettypes.h"

#include "main.h"
#include "mission.h" // for cheats
#include "multistat.h"

// ////////////////////////////////////////////////////////////////////////////
// STATS STUFF
// ////////////////////////////////////////////////////////////////////////////
static PLAYERSTATS playerStats[MAX_PLAYERS];

// ////////////////////////////////////////////////////////////////////////////
// Get Player's stats
PLAYERSTATS getMultiStats(UDWORD player)
{
	return playerStats[player];
}

// ////////////////////////////////////////////////////////////////////////////
// Set Player's stats
// send stats to all players when bLocal is false
bool setMultiStats(uint32_t playerIndex, PLAYERSTATS plStats, bool bLocal)
{
	if (playerIndex >= MAX_PLAYERS)
	{
		return true;
	}

	// First copy over the data into our local array
	playerStats[playerIndex] = plStats;

	if (!bLocal)
	{
		// Now send it to all other players
		NETbeginEncode(NETbroadcastQueue(), NET_PLAYER_STATS);
			// Send the ID of the player's stats we're updating
			NETuint32_t(&playerIndex);

			// Send over the actual stats
			NETuint32_t(&playerStats[playerIndex].played);
			NETuint32_t(&playerStats[playerIndex].wins);
			NETuint32_t(&playerStats[playerIndex].losses);
			NETuint32_t(&playerStats[playerIndex].totalKills);
			NETuint32_t(&playerStats[playerIndex].totalScore);
			NETuint32_t(&playerStats[playerIndex].recentKills);
			NETuint32_t(&playerStats[playerIndex].recentScore);
		NETend();
	}

	return true;
}

void recvMultiStats(NETQUEUE queue)
{
	uint32_t playerIndex;

	NETbeginDecode(queue, NET_PLAYER_STATS);
		// Retrieve the ID number of the player for which we need to
		// update the stats
		NETuint32_t(&playerIndex);

		if (playerIndex >= MAX_PLAYERS)
		{
			return;
		}

		// we don't what to update ourselves, we already know our score (FIXME: rewrite setMultiStats())
		if (!myResponsibility(playerIndex))
		{
			// Retrieve the actual stats
			NETuint32_t(&playerStats[playerIndex].played);
			NETuint32_t(&playerStats[playerIndex].wins);
			NETuint32_t(&playerStats[playerIndex].losses);
			NETuint32_t(&playerStats[playerIndex].totalKills);
			NETuint32_t(&playerStats[playerIndex].totalScore);
			NETuint32_t(&playerStats[playerIndex].recentKills);
			NETuint32_t(&playerStats[playerIndex].recentScore);
		}
	NETend();
}

// ////////////////////////////////////////////////////////////////////////////
// Load Player Stats
bool loadMultiStats(char *sPlayerName, PLAYERSTATS *st)
{
	char				fileName[255];
	UDWORD				size;
	char				*pFileData;

	memset(st, 0, sizeof(PLAYERSTATS));	// clear in case we don't get to load

	// Prevent an empty player name (where the first byte is a 0x0 terminating char already)
	if (!*sPlayerName)
	{
		strcpy(sPlayerName, _("Player"));
	}

	snprintf(fileName, sizeof(fileName), "%s%s.sta", MultiPlayersPath, sPlayerName);

	debug(LOG_WZ, "loadMultiStats: %s", fileName);

	// check player already exists
	if ( !PHYSFS_exists( fileName ) )
	{
		PLAYERSTATS			blankstats;

		memset(&blankstats, 0, sizeof(PLAYERSTATS));
		saveMultiStats(sPlayerName,sPlayerName,&blankstats);		// didnt exist so create.
	}
	else
	{
		int num = 0;

		loadFile(fileName,&pFileData,&size);

		if (strncmp(pFileData, "WZ.STA.v3", 9) != 0)
		{
			return false; // wrong version or not a stats file
		}

		num = sscanf(pFileData, "WZ.STA.v3\n%u %u %u %u %u",
		             &st->wins, &st->losses, &st->totalKills, &st->totalScore, &st->played);
		if (num < 5)
		{
			st->played = 0;	// must be old, buggy format still
		}
		free(pFileData);
	}

	// reset recent scores
	st->recentKills = 0;
	st->recentScore = 0;

	// clear any skirmish stats.
	for(size = 0;size<MAX_PLAYERS;size++)
	{
		ingame.skScores[size][0] =0;
		ingame.skScores[size][1] =0;
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Save Player Stats
#define MAX_STA_SIZE 500
bool saveMultiStats(const char *sFileName, const char *sPlayerName, const PLAYERSTATS *st)
{
	char buffer[MAX_STA_SIZE];
	char fileName[255] = "";

	snprintf(buffer, MAX_STA_SIZE, "WZ.STA.v3\n%u %u %u %u %u",
	         st->wins, st->losses, st->totalKills, st->totalScore, st->played);

	snprintf(fileName, sizeof(fileName), "%s%s.sta", MultiPlayersPath, sFileName);

	saveFile(fileName, buffer, strlen(buffer));

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// score update functions

// update players damage stats.
void updateMultiStatsDamage(UDWORD attacker, UDWORD defender, UDWORD inflicted)
{
	if (Cheated)
	{
		return;
	}
	// FIXME: Why in the world are we using two different structs for stats when we can use only one?
	if (attacker < MAX_PLAYERS)
	{
		if (NetPlay.bComms)
		{
			playerStats[attacker].totalScore  += 2 * inflicted;
			playerStats[attacker].recentScore += 2 * inflicted;
		}
		else
		{
			ingame.skScores[attacker][0] += 2 * inflicted;  // increment skirmish players rough score.
		}
	}

	// FIXME: Why in the world are we using two different structs for stats when we can use only one?
	if (defender < MAX_PLAYERS)
	{
		if (NetPlay.bComms)
		{
			playerStats[defender].totalScore  -= inflicted;
			playerStats[defender].recentScore -= inflicted;
		}
		else
		{
			ingame.skScores[defender][0] -= inflicted;  // increment skirmish players rough score.
		}
	}
}

// update games played.
void updateMultiStatsGames(void)
{
	if (Cheated || selectedPlayer >= MAX_PLAYERS)
	{
		return;
	}
	++playerStats[selectedPlayer].played;
}

// games won
void updateMultiStatsWins(void)
{
	if (Cheated || selectedPlayer >= MAX_PLAYERS)
	{
		return;
	}
	++playerStats[selectedPlayer].wins;
}

//games lost.
void updateMultiStatsLoses(void)
{
	if (Cheated || selectedPlayer >= MAX_PLAYERS)
	{
		return;
	}
	++playerStats[selectedPlayer].losses;
}

// update kills
void updateMultiStatsKills(BASE_OBJECT *psKilled,UDWORD player)
{
	if (Cheated || player > MAX_PLAYERS)
	{
		return;
	}
	// FIXME: Why in the world are we using two different structs for stats when we can use only one?
	if (NetPlay.bComms)
	{
		++playerStats[player].totalKills;
		++playerStats[player].recentKills;
	}
	else
	{
		ingame.skScores[player][1]++;
	}
}
