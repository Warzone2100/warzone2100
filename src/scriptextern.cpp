/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
 * ScriptExtern.c
 *
 * All game variable access functions for the scripts
 *
 */

#include "lib/framework/frame.h"
#include "map.h"

#include "lib/script/script.h"
#include "lib/netplay/netplay.h"
#include "scripttabs.h"
#include "scriptextern.h"

#include "display.h"

#include "multiplay.h"

#include "main.h"
#include "hci.h"
#include "lib/gamelib/gtime.h"


// current game level
SDWORD		scrGameLevel = 0;

// whether the tutorial is active
bool		bInTutorial = false;

// whether any additional special case victory/failure conditions have been met
bool		bExtraVictoryFlag = false;
bool		bExtraFailFlag = false;



// whether or not to track the player's transporter as it comes
// into an offworld mission.
bool		bTrackTransporter = false;


// reset the script externals for a new level
void scrExternReset(void)
{
	scrGameLevel = 0;
	bInTutorial = false;
	bExtraVictoryFlag = false;
	bExtraFailFlag = false;
}


// General function to get some basic game values
bool scrGenExternGet(UDWORD index)
{
	INTERP_TYPE		type;
	INTERP_VAL	scrFunctionResult;	//function return value to be pushed to stack

	switch (index)
	{

	case EXTID_TRACKTRANSPORTER:
		type = VAL_BOOL;
		scrFunctionResult.v.bval = bTrackTransporter;
		break;
	case EXTID_MAPWIDTH:
		type = VAL_INT;
		scrFunctionResult.v.ival = mapWidth;
		break;
	case EXTID_MAPHEIGHT:
		type = VAL_INT;
		scrFunctionResult.v.ival = mapHeight;
		break;
	case EXTID_GAMEINIT:
		type = VAL_BOOL;
		scrFunctionResult.v.bval = gameInitialised;
		break;
	case EXTID_SELECTEDPLAYER:
		type = VAL_INT;
		scrFunctionResult.v.ival = selectedPlayer;
		break;
	case EXTID_GAMELEVEL:
		type = VAL_INT;
		scrFunctionResult.v.ival = scrGameLevel;
		break;
	case EXTID_GAMETIME:
		type = VAL_INT;
		scrFunctionResult.v.ival = (SDWORD)(gameTime / SCR_TICKRATE);
		break;
	case EXTID_TUTORIAL:
		type = VAL_BOOL;
		scrFunctionResult.v.bval = bInTutorial;
		break;
	case EXTID_CURSOR:
		type = VAL_INT;
		scrFunctionResult.v.ival = 0; // FIXME Set to 0 since function returned undef value
		break;
	case EXTID_INTMODE:
		type = VAL_INT;
		scrFunctionResult.v.ival = intMode;
		break;

	case EXTID_TARGETTYPE:
		type = VAL_INT;
		scrFunctionResult.v.ival = getTargetType();
		break;
	case EXTID_EXTRAVICTORYFLAG:
		type = VAL_BOOL;
		scrFunctionResult.v.bval = bExtraVictoryFlag;
		break;
	case EXTID_EXTRAFAILFLAG:
		type = VAL_BOOL;
		scrFunctionResult.v.bval = bExtraFailFlag;
		break;
	case EXTID_MULTIGAMETYPE:		// multiplayer variable..
		type = VAL_INT;
		scrFunctionResult.v.ival = game.type;
		break;
	case EXTID_MULTIGAMEHUMANMAX:		// multiplayer variable..
		type = VAL_INT;
		scrFunctionResult.v.ival = game.maxPlayers;
		break;
	case EXTID_MULTIGAMEBASETYPE:
		type = VAL_INT;
		scrFunctionResult.v.ival	= game.base;
		break;
	case EXTID_MULTIGAMEALLIANCESTYPE:
		type = VAL_INT;
		scrFunctionResult.v.ival	= game.alliance;
		break;

	default:
		ASSERT(false, "scrGenExternGet: unknown variable index");
		return false;
		break;
	}

	if (!stackPushResult(type, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// General function to set some basic game values
bool scrGenExternSet(UDWORD index)
{
	INTERP_VAL		sVal;
	INTERP_TYPE		type;
	SDWORD			val;

	// Get the value and store it in type,val
	if (!stackPop(&sVal))
	{
		return false;
	}
	type = sVal.type;
	val = sVal.v.ival;

	switch (index)
	{
	case EXTID_GAMELEVEL:
		if (type != VAL_INT)
		{
			ASSERT(false, "invalid type for gameLevel");
			return false;
		}
		scrGameLevel = val;
		break;
	case EXTID_TUTORIAL:
		if (type != VAL_BOOL)
		{
			ASSERT(false, "invalid type for inTutorial");
			return false;
		}
		bInTutorial = val;
		if (val)
		{
			// Since tutorial is skirmish
			NetPlay.players[0].allocated = true;
		}
		break;
	case EXTID_EXTRAVICTORYFLAG:
		if (type != VAL_BOOL)
		{
			ASSERT(false, "invalid type for extraVictoryFlag");
			return false;
		}
		bExtraVictoryFlag = val;
		break;
	case EXTID_EXTRAFAILFLAG:
		if (type != VAL_BOOL)
		{
			ASSERT(false, "invalid type for extraFailFlag");
			return false;
		}
		bExtraFailFlag = val;
		break;
	default:
		ASSERT(false, "scrGenExternSet: unknown variable index");
		return false;
		break;
	}

	return true;
}
