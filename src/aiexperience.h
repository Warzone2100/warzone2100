/*
	This file is part of Warzone 2100.
	Copyright (C) 2006-2010  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_AIEXPERIENCE_H__
#define __INCLUDED_SRC_AIEXPERIENCE_H__

#define	MAX_OIL_DEFEND_LOCATIONS	100		//max number of attack locations to store
#define	MAX_BASE_DEFEND_LOCATIONS	30		//max number of base locations to store

extern SDWORD baseLocation[MAX_PLAYERS][MAX_PLAYERS][2];
extern SDWORD baseDefendLocation[MAX_PLAYERS][MAX_BASE_DEFEND_LOCATIONS][2];
extern SDWORD oilDefendLocation[MAX_PLAYERS][MAX_OIL_DEFEND_LOCATIONS][2];

extern SDWORD baseDefendLocPrior[MAX_PLAYERS][MAX_BASE_DEFEND_LOCATIONS];
extern SDWORD oilDefendLocPrior[MAX_PLAYERS][MAX_OIL_DEFEND_LOCATIONS];

BOOL SavePlayerAIExperience(SDWORD nPlayer, BOOL bNotify);
SDWORD LoadPlayerAIExperience(SDWORD nPlayer);

void LoadAIExperience(BOOL bNotify);
BOOL SaveAIExperience(BOOL bNotify);


void InitializeAIExperience(void);

BOOL StoreBaseDefendLoc(SDWORD x, SDWORD y, SDWORD nPlayer);
BOOL StoreOilDefendLoc(SDWORD x, SDWORD y, SDWORD nPlayer);

int GetOilDefendLocIndex(SDWORD x, SDWORD y, SDWORD nPlayer);
SDWORD GetBaseDefendLocIndex(SDWORD x, SDWORD y, SDWORD nPlayer);

bool CanRememberPlayerBaseLoc(SDWORD lookingPlayer, SDWORD enemyPlayer);
bool CanRememberPlayerBaseDefenseLoc(SDWORD player, SDWORD index);
bool CanRememberPlayerOilDefenseLoc(SDWORD player, SDWORD index);

void BaseExperienceDebug(SDWORD nPlayer);
void OilExperienceDebug(SDWORD nPlayer);

#endif // __INCLUDED_SRC_AIEXPERIENCE_H__
