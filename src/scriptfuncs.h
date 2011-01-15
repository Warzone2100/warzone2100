/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
 *  All the C functions callable from the script code
 */

#ifndef __INCLUDED_SRC_SCRIPTFUNCS_H__
#define __INCLUDED_SRC_SCRIPTFUNCS_H__

#include "objectdef.h"
#include "messagedef.h"			//for VIEWDATA

// Lua
#include "lib/lua/warzone.h"

// AI won't build there if there are more than
// MAX_BLOCKING_TILES on some location
#define MAX_BLOCKING_TILES		1

/// Forward declarations to allow pointers to these types
struct BASE_OBJECT;
struct DROID;

extern BOOL scriptInit(void);
extern void scriptSetStartPos(int position, int x, int	y);
extern Vector2i getPlayerStartPosition(int player);
extern BOOL scrSetSunPosition(void);
extern BOOL scrSetSunIntensity(void);

// not used in scripts, but used in code.
extern  BOOL objectInRange(struct BASE_OBJECT *psList, SDWORD x, SDWORD y, SDWORD range);

typedef enum gamestatus
{
	STATUS_ReticuleIsOpen,
	STATUS_BattleMapViewEnabled,
	STATUS_DeliveryReposInProgress
} GAMESTATUS;

extern BOOL beingResearchedByAlly(SDWORD resIndex, SDWORD player);
extern BOOL ThreatInRange(SDWORD player, SDWORD range, SDWORD rangeX, SDWORD rangeY, BOOL bVTOLs);
extern BOOL skTopicAvail(UWORD inc, UDWORD player);
extern UDWORD numPlayerWeapDroidsInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, BOOL bVTOLs);
extern UDWORD numPlayerWeapStructsInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, BOOL bFinished);
extern UDWORD playerWeapDroidsCostInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, BOOL bVTOLs);
extern UDWORD playerWeapStructsCostInRange(SDWORD player, SDWORD lookingPlayer, SDWORD range, SDWORD rangeX, SDWORD rangeY, BOOL bFinished);
extern UDWORD numEnemyObjInRange(SDWORD player, SDWORD range, SDWORD rangeX, SDWORD rangeY, BOOL bVTOLs, BOOL bFinished);
extern BOOL addBeaconBlip(SDWORD x, SDWORD y, SDWORD forPlayer, SDWORD sender, char * textMsg);
extern BOOL sendBeaconToPlayer(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, char * beaconMsg);
extern MESSAGE * findBeaconMsg(UDWORD player, SDWORD sender);
extern SDWORD getNumRepairedBy(struct DROID *psDroidToCheck, SDWORD player);
extern BOOL objectInRangeVis(struct BASE_OBJECT *psList, SDWORD x, SDWORD y, SDWORD range, SDWORD lookingPlayer);
extern SDWORD getPlayerFromString(char *playerName);

extern void registerScriptfuncs(lua_State *L);

extern BOOL addBeaconBlip(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, char * textMsg);
extern VIEWDATA *CreateBeaconViewData(SDWORD sender, UDWORD LocX, UDWORD LocY);

#endif // __INCLUDED_SRC_SCRIPTFUNCS_H__
