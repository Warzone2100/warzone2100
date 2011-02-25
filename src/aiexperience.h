/*
	This file is part of Warzone 2100.
	Copyright (C) 2006-2011  Warzone 2100 Project

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

#define	SAVE_FORMAT_VERSION			2

#define MAX_OIL_ENTRIES				600		//(Max number of derricks or oil resources) / 2

#define	MAX_OIL_DEFEND_LOCATIONS	100		//max number of attack locations to store
#define	MAX_OIL_LOCATIONS			300		//max number of oil locations to store
#define	MAX_BASE_DEFEND_LOCATIONS	30		//max number of base locations to store
#define SAME_LOC_RANGE				8		//if within this range, consider it the same loc

extern SDWORD baseLocation[MAX_PLAYERS][MAX_PLAYERS][2];
extern SDWORD baseDefendLocation[MAX_PLAYERS][MAX_BASE_DEFEND_LOCATIONS][2];
extern SDWORD oilDefendLocation[MAX_PLAYERS][MAX_OIL_DEFEND_LOCATIONS][2];

extern SDWORD baseDefendLocPrior[MAX_PLAYERS][MAX_BASE_DEFEND_LOCATIONS];
extern SDWORD oilDefendLocPrior[MAX_PLAYERS][MAX_OIL_DEFEND_LOCATIONS];

extern	BOOL SavePlayerAIExperience(SDWORD nPlayer, BOOL bNotify);
extern	SDWORD LoadPlayerAIExperience(SDWORD nPlayer);

extern	void LoadAIExperience(BOOL bNotify);
extern	BOOL SaveAIExperience(BOOL bNotify);


extern	BOOL ExperienceRecallOil(SDWORD nPlayer);
extern	void InitializeAIExperience(void);
extern	BOOL OilResourceAt(UDWORD OilX,UDWORD OilY, SDWORD VisibleToPlayer);

extern	SDWORD ReadAISaveData(SDWORD nPlayer);
extern	BOOL WriteAISaveData(SDWORD nPlayer);

extern	BOOL SetUpInputFile(SDWORD nPlayer);
extern	BOOL SetUpOutputFile(SDWORD nPlayer);

extern	BOOL StoreBaseDefendLoc(SDWORD x, SDWORD y, SDWORD nPlayer);
extern	BOOL StoreOilDefendLoc(SDWORD x, SDWORD y, SDWORD nPlayer);

extern	BOOL SortBaseDefendLoc(SDWORD nPlayer);
extern	BOOL SortOilDefendLoc(SDWORD nPlayer);

extern	SDWORD GetOilDefendLocIndex(SDWORD x, SDWORD y, SDWORD nPlayer);
extern	SDWORD GetBaseDefendLocIndex(SDWORD x, SDWORD y, SDWORD nPlayer);

extern BOOL CanRememberPlayerBaseLoc(SDWORD lookingPlayer, SDWORD enemyPlayer);
extern BOOL CanRememberPlayerBaseDefenseLoc(SDWORD player, SDWORD index);
extern BOOL CanRememberPlayerOilDefenseLoc(SDWORD player, SDWORD index);

extern	void BaseExperienceDebug(SDWORD nPlayer);
extern	void OilExperienceDebug(SDWORD nPlayer);

extern BOOL canRecallOilAt(SDWORD nPlayer, SDWORD x, SDWORD y);

//Return values of experience-loading routine
#define EXPERIENCE_LOAD_OK			0			//no problemens encountered
#define EXPERIENCE_LOAD_ERROR		1			//error while loading experience
#define EXPERIENCE_LOAD_NOSAVE		(-1)		//no experience exists

#endif // __INCLUDED_SRC_AIEXPERIENCE_H__
