/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
#include "lib/framework/strres.h"
#include "lib/framework/file.h"
#include "lib/netplay/netplay.h"
#include "lib/widget/widget.h"

#include "objmem.h"
#include "power.h"
#include "map.h"
#include "effects.h"	// for discovery flash
#include "cmddroid.h"
#include "multiplay.h"
#include "multirecv.h"
#include "multistat.h"
#include "multiint.h"
#include "fpath.h"

extern char	MultiPlayersPath[PATH_MAX];

// ////////////////////////////////////////////////////////////////////////////
// STATS STUFF
// ////////////////////////////////////////////////////////////////////////////
static PLAYERSTATS playerStats[MAX_PLAYERS];

// ////////////////////////////////////////////////////////////////////////////
// Get Player's stats
PLAYERSTATS getMultiStats(UDWORD player,BOOL bLocal)
{
	static PLAYERSTATS stat;
	uint32_t playerDPID = player2dpid[player];

	// Copy over the data from our local array
	memcpy(&stat, &playerStats[playerDPID], sizeof(stat));

	return stat;
}

// ////////////////////////////////////////////////////////////////////////////
// Set Player's stats
BOOL setMultiStats(SDWORD dp, PLAYERSTATS plStats, BOOL bLocal)
{
	uint32_t playerDPID = (uint32_t)dp;

	// First copy over the data into our local array
	memcpy(&playerStats[playerDPID], &plStats, sizeof(plStats));

	if (!bLocal)
	{
		// Now send it to all other players
		NETbeginEncode(NET_PLAYER_STATS, NET_ALL_PLAYERS);
			// Send the ID of the player's stats we're updating
			NETuint32_t(&playerDPID);

			// Send over the actual stats
			NETuint32_t(&playerStats[playerDPID].played);
			NETuint32_t(&playerStats[playerDPID].wins);
			NETuint32_t(&playerStats[playerDPID].losses);
			NETuint32_t(&playerStats[playerDPID].totalKills);
			NETuint32_t(&playerStats[playerDPID].totalScore);
			NETuint32_t(&playerStats[playerDPID].recentKills);
			NETuint32_t(&playerStats[playerDPID].recentScore);
			NETuint32_t(&playerStats[playerDPID].killsToAdd);
			NETuint32_t(&playerStats[playerDPID].scoreToAdd);
		NETend();
	}

	return true;
}

void recvMultiStats()
{
	uint32_t playerDPID;

	NETbeginDecode(NET_PLAYER_STATS);
		// Retrieve the ID number of the player for which we need to
		// update the stats
		NETuint32_t(&playerDPID);

		// Retrieve the actual stats
		NETuint32_t(&playerStats[playerDPID].played);
		NETuint32_t(&playerStats[playerDPID].wins);
		NETuint32_t(&playerStats[playerDPID].losses);
		NETuint32_t(&playerStats[playerDPID].totalKills);
		NETuint32_t(&playerStats[playerDPID].totalScore);
		NETuint32_t(&playerStats[playerDPID].recentKills);
		NETuint32_t(&playerStats[playerDPID].recentScore);
		NETuint32_t(&playerStats[playerDPID].killsToAdd);
		NETuint32_t(&playerStats[playerDPID].scoreToAdd);
	NETend();
}

// ////////////////////////////////////////////////////////////////////////////
// Load Player Stats
BOOL loadMultiStats(char *sPlayerName, PLAYERSTATS *st)
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
	st->killsToAdd  = 0;
	st->scoreToAdd  = 0;

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
BOOL saveMultiStats(const char *sFileName, const char *sPlayerName, const PLAYERSTATS *st)
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
	PLAYERSTATS st;

	if(isHumanPlayer(attacker))
	{
		st = getMultiStats(attacker,true);	// get stats
		if(NetPlay.bComms)
		{
			st.scoreToAdd += (2*inflicted);
		}
		else
		{
			st.recentScore += (2*inflicted);
		}
		setMultiStats(player2dpid[attacker], st, true);
	}
	else
	{
		ingame.skScores[attacker][0] += (2*inflicted);	// increment skirmish players rough score.
	}


	if(isHumanPlayer(defender))
	{
		st = getMultiStats(defender,true);	// get stats
		if(NetPlay.bComms)
		{
			st.scoreToAdd  -= inflicted;
		}
		else
		{
			st.recentScore  -= inflicted;
		}
		setMultiStats(player2dpid[defender], st, true);
	}
	else
	{
		ingame.skScores[defender][0] -= inflicted;	// increment skirmish players rough score.
	}
}

// update games played.
void updateMultiStatsGames(void)
{
	PLAYERSTATS	st;

	st  = getMultiStats(selectedPlayer,true);
	st.played ++;
	setMultiStats(player2dpid[selectedPlayer], st, true);
}

// games won
void updateMultiStatsWins(void)
{
	PLAYERSTATS	st;
	st  = getMultiStats(selectedPlayer,true);
	st.wins ++;
	setMultiStats(player2dpid[selectedPlayer], st, true);
}

//games lost.
void updateMultiStatsLoses(void)
{
	PLAYERSTATS	st;
	st  = getMultiStats(selectedPlayer,true);
	++st.losses;
	setMultiStats(player2dpid[selectedPlayer], st, true);
}

// update kills
void updateMultiStatsKills(BASE_OBJECT *psKilled,UDWORD player)
{
	PLAYERSTATS	st;

	if(isHumanPlayer(player))
	{
		st  = getMultiStats(player,true);

		if(NetPlay.bComms)
		{
			st.killsToAdd++;		// increase kill count;
		}
		else
		{
			st.recentKills++;
		}
		setMultiStats(player2dpid[player], st, true);
	}
	else
	{
		ingame.skScores[player][1]++;
	}
}
