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
 *  multistat.h
 *
 *   Alex Lee
 *
 * Pumpkin Studios, Eidos PLC, Jan98.
 * Definitions for multi player statistics and scores for league tables.
 * Also Definitions for saved Arena Forces to enable teams to be saved to disk
 */

#ifndef __INCLUDED_SRC_MULTISTATS_H__
#define __INCLUDED_SRC_MULTISTATS_H__

typedef struct
{
	uint32_t played;						/// propogated stats.
	uint32_t wins;
	uint32_t losses;
	uint32_t totalKills;
	uint32_t totalScore;

	uint32_t recentKills;				// score/kills in last game.
	uint32_t recentScore;

	uint32_t killsToAdd;					// things to add next time score is updated.
	uint32_t scoreToAdd;
} PLAYERSTATS;

// stat defs
extern BOOL			saveMultiStats			(const char *sFName, const char *sPlayerName, const PLAYERSTATS *playerStats);	// to disk
extern BOOL			loadMultiStats			(char *sPlayerName, PLAYERSTATS *playerStats);					// form disk
extern PLAYERSTATS	getMultiStats			(UDWORD player,BOOL bLocal);									// get from net
extern BOOL			setMultiStats			(SDWORD playerDPID, PLAYERSTATS plStats,BOOL bLocal);			// send to net.
extern void			updateMultiStatsDamage	(UDWORD attacker, UDWORD defender, UDWORD inflicted);
extern void			updateMultiStatsGames	(void);
extern void			updateMultiStatsWins	(void);
extern void			updateMultiStatsLoses	(void);
extern void			updateMultiStatsKills	(BASE_OBJECT *psKilled,UDWORD player);

extern void recvMultiStats(void);

#endif // __INCLUDED_SRC_MULTISTATS_H__
