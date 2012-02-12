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
/**
 * @file cheat.c
 * Handles cheat codes for Warzone.
 */
/* Alex M 19th - Jan. 1999 */

#include "lib/framework/frame.h"
#include "lib/exceptionhandler/dumpinfo.h"

#include "cheat.h"
#include "keybind.h"
#include "keymap.h"

struct CHEAT_ENTRY
{
	const char *pName;
	void (*function)(void);	// pointer to void* function
};

bool Cheated = false;
static CHEAT_ENTRY cheatCodes[] =
{
	{"clone wars", kf_CloneSelected}, // clone selected units
	{"noassert", kf_NoAssert}, // turn off asserts
	{"count me", kf_ShowNumObjects}, // give a count of objects in the world
	{"give all", kf_AllAvailable},	// give all
	{"research all", kf_FinishAllResearch}, // research everything at once
	{"superpower", kf_MaxPower}, // get tons of power
	{"more power", kf_UpThePower}, // get tons of power
	{"deity", kf_ToggleGodMode},	//from above
	{"droidinfo", kf_DebugDroidInfo},	//show unit stats
	{"sensors", kf_ToggleSensorDisplay},	//show sensor ranges
	{"let me win", kf_AddMissionOffWorld},	//let me win
	{"timedemo", kf_FrameRate},	 //timedemo
	{"kill", kf_KillSelected},	//kill slected
	{"demo", kf_ToggleDemoMode},	//demo mode
	{"john kettley", kf_ToggleWeather},	//john kettley
	{"shakey", kf_ToggleShakeStatus},	//shakey
	{"mouseflip", kf_ToggleMouseInvert},	//mouseflip
	{"biffer baker", kf_SetKillerLevel},	//indestructive units
	{"easy", kf_SetEasyLevel},	//easy
	{"normal", kf_SetNormalLevel},	//normal
	{"hard", kf_SetHardLevel},	//hard
	{"double up", kf_SetToughUnitsLevel},	// your units take half the damage
	{"whale fin", kf_TogglePower},	// turns on/off infinte power
	{"get off my land", kf_KillEnemy},	// kills all enemy units and structures
	{"build info", kf_BuildInfo},	// tells you when the game was built
	{"time toggle", kf_ToggleMissionTimer},
	{"work harder", kf_FinishResearch},
	{"tileinfo", kf_TileInfo}, // output debug info about a tile
	{"showfps", kf_ToggleFPS},	//displays your average FPS
	{"showsamples", kf_ToggleSamples}, //displays the # of Sound samples in Queue & List
	{"showorders", kf_ToggleOrders}, //displays unit order/action state.
	{"showlevelname", kf_ToggleLevelName}, // shows the current level name on screen
	{"logical", kf_ToggleLogical}, //logical game updates separated from graphics updates.
	{"pause", kf_TogglePauseMode}, // Pause the game.
	{"power info", kf_PowerInfo},
	{"reload me", kf_Reload},	// reload selected weapons immediately
	{"desync me", kf_ForceDesync},
};

bool attemptCheatCode(const char* cheat_name)
{
	const CHEAT_ENTRY * curCheat;
	static const CHEAT_ENTRY * const EndCheat = &cheatCodes[ARRAY_SIZE(cheatCodes)];

	// there is no reason to make people enter "cheat mode" to enter following commands
	if (!strcasecmp("showfps", cheat_name)) 
	{
		kf_ToggleFPS(); 
		return true; 
	}
	else if (!strcasecmp("showlevelname", cheat_name))
	{
		kf_ToggleLevelName();
		return true;
	}

	if (strcmp(cheat_name, "cheat on") == 0 || strcmp(cheat_name, "debug") == 0)
	{
		if (!getDebugMappingStatus())
		{
			kf_ToggleDebugMappings();
		}
		return true;
	}
	if (strcmp(cheat_name, "cheat off") == 0 && getDebugMappingStatus())
	{
		kf_ToggleDebugMappings();
		return true;
	}
	if (!getDebugMappingStatus())
	{
		return false;
	}

	for (curCheat = cheatCodes; curCheat != EndCheat; ++curCheat)
	{
		if (strcmp(cheat_name, curCheat->pName) == 0)
		{
			char buf[256];

			/* We've got our man... */
			curCheat->function();	// run it

			// Copy this info to be used by the crash handler for the dump file
			ssprintf(buf, "User has used cheat code: %s", curCheat->pName);
			addDumpInfo(buf);

			/* And get out of here */
			Cheated = true;
			return true;
		}
	}

	return false;
}
