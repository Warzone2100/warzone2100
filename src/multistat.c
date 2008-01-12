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

#include <stdio.h>
#include <string.h>
#include <physfs.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "objmem.h"
#include "power.h"
#include "map.h"
#include "lib/widget/widget.h"
#include "effects.h"	// for discovery flash
#include "lib/netplay/netplay.h"
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

// ////////////////////////////////////////////////////////////////////////////
// Get Player's stats
PLAYERSTATS getMultiStats(UDWORD player,BOOL bLocal)
{
	static PLAYERSTATS stat;
	UDWORD		playerDPID;

	playerDPID = player2dpid[player];

	if(bLocal)
	{
		NETgetLocalPlayerData(playerDPID, &stat);
	}
	else
	{
		NETgetGlobalPlayerData(playerDPID, &stat);
	}

	return stat;
}

// ////////////////////////////////////////////////////////////////////////////
// Set Player's stats
BOOL setMultiStats(SDWORD dp, PLAYERSTATS plStats, BOOL bLocal)
{
	UDWORD	playerDPID = (UDWORD) dp;

	if(bLocal)
	{
		NETsetLocalPlayerData(playerDPID,&plStats,sizeof(PLAYERSTATS));
	}
	else
	{
		NETsetGlobalPlayerData(playerDPID,&plStats,sizeof(PLAYERSTATS));
	}
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// Load Player Stats
BOOL loadMultiStats(char *sPlayerName, PLAYERSTATS *st)
{
	char				fileName[255];
	UDWORD				size;
	char				*pFileData;

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
			return FALSE; // wrong version or not a stats file
		}

		num = sscanf(pFileData, "WZ.STA.v2\n%u %u %u %u %u",
		             &st->wins, &st->losses, &st->totalKills, &st->totalScore, &st->played);
		if (num < 6)
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

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// Save Player Stats
#define MAX_STA_SIZE 500
BOOL saveMultiStats(const char *sFileName, const char *sPlayerName, const PLAYERSTATS *st)
{
	char buffer[MAX_STA_SIZE];
	char fileName[255] = "";

	snprintf(buffer, MAX_STA_SIZE, "WZ.STA.v2\n%s %u %u %u %u %u",
		sPlayerName, st->wins, st->losses, st->totalKills, st->totalScore, st->played);

	snprintf(fileName, sizeof(fileName), "%s%s.sta", MultiPlayersPath, sFileName);

	saveFile(fileName, buffer, strlen(buffer));

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// score update functions

// update players damage stats.
void updateMultiStatsDamage(UDWORD attacker, UDWORD defender, UDWORD inflicted)
{
	PLAYERSTATS st;

	if(isHumanPlayer(attacker))
	{
		st = getMultiStats(attacker,TRUE);	// get stats
		if(NetPlay.bComms)
		{
			st.scoreToAdd += (2*inflicted);
		}
		else
		{
			st.recentScore += (2*inflicted);
		}
		setMultiStats(player2dpid[attacker], st, TRUE);
	}
	else
	{
		ingame.skScores[attacker][0] += (2*inflicted);	// increment skirmish players rough score.
	}


	if(isHumanPlayer(defender))
	{
		st = getMultiStats(defender,TRUE);	// get stats
		if(NetPlay.bComms)
		{
			st.scoreToAdd  -= inflicted;
		}
		else
		{
			st.recentScore  -= inflicted;
		}
		setMultiStats(player2dpid[defender], st, TRUE);
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

	st  = getMultiStats(selectedPlayer,TRUE);
	st.played ++;
	setMultiStats(player2dpid[selectedPlayer], st, TRUE);
}

// games won
void updateMultiStatsWins(void)
{
	PLAYERSTATS	st;
	st  = getMultiStats(selectedPlayer,TRUE);
	st.wins ++;
	setMultiStats(player2dpid[selectedPlayer], st, TRUE);
}

//games lost.
void updateMultiStatsLoses(void)
{
	PLAYERSTATS	st;
	st  = getMultiStats(selectedPlayer,TRUE);
	++st.losses;
	setMultiStats(player2dpid[selectedPlayer], st, TRUE);
}

// update kills
void updateMultiStatsKills(BASE_OBJECT *psKilled,UDWORD player)
{
	PLAYERSTATS	st;

	if(isHumanPlayer(player))
	{
		st  = getMultiStats(player,TRUE);

		if(NetPlay.bComms)
		{
			st.killsToAdd++;		// increase kill count;
		}
		else
		{
			st.recentKills++;
		}
		setMultiStats(player2dpid[player], st, TRUE);
	}
	else
	{
		ingame.skScores[player][1]++;
	}
}
