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
/**
 * @file cheat.c
 * Handles cheat codes for Warzone.
 */
/* Alex M 19th - Jan. 1999 */

#include "lib/framework/frame.h"
#include "lib/exceptionhandler/dumpinfo.h"
#include "lib/netplay/netplay.h"
#include "lib/framework/string_ext.h"

#include "cheat.h"
#include "keybind.h"
#include "keymap.h"
#include "multiplay.h"
#include "qtscript.h"
#include "template.h"
#include "activity.h"

struct CHEAT_ENTRY
{
	const char *pName;
	void (*function)();  // pointer to void* function
};

bool Cheated = false;
static CHEAT_ENTRY cheatCodes[] =
{
	{"templates", listTemplates}, // print templates
	{"jsload", jsAutogame}, // load an AI script for selectedPlayer
	{"jsdebug", jsShowDebug}, // show scripting states
	{"teach us", kf_TeachSelected}, // give experience to selected units
	{"untouchable", kf_Unselectable}, // make selected droids unselectable
	{"clone wars", []{ kf_CloneSelected(10); }}, // clone selected units
	{"clone wars!", []{ kf_CloneSelected(40); }}, // clone selected units
	{"clone wars!!", []{ kf_CloneSelected(135); }}, // clone selected units
	{"noassert", kf_NoAssert}, // turn off asserts
	{"count me", kf_ShowNumObjects}, // give a count of objects in the world
	{"give all", kf_AllAvailable},	// give all
	{"research all", kf_FinishAllResearch}, // research everything at once
	{"superpower", kf_MaxPower}, // get tons of power
	{"more power", kf_UpThePower}, // get tons of power
	{"deity", kf_ToggleGodMode},	//from above
	{"droidinfo", kf_DebugDroidInfo},	//show unit stats
	{"sensors", kf_ToggleSensorDisplay},	//show sensor ranges
	{"timedemo", kf_FrameRate},	 //timedemo
	{"kill", kf_KillSelected},	//kill selected
	{"john kettley", kf_ToggleWeather},	//john kettley
	{"mouseflip", kf_ToggleMouseInvert},	//mouseflip
	{"biffer baker", kf_BifferBaker},	// almost invincible units
	{"easy", kf_SetEasyLevel},	//easy
	{"normal", kf_SetNormalLevel},	//normal
	{"hard", kf_SetHardLevel},	//hard
	{"double up", kf_DoubleUp},	// your units take half the damage
	{"whale fin", kf_TogglePower},	// turns on/off infinte power
	{"get off my land", kf_KillEnemy},	// kills all enemy units and structures
	{"build info", kf_BuildInfo},	// tells you when the game was built
	{"time toggle", kf_ToggleMissionTimer},
	{"work harder", kf_FinishResearch},
	{"tileinfo", kf_TileInfo}, // output debug info about a tile
	{"showfps", kf_ToggleFPS},	//displays your average FPS
	{"showsamples", kf_ToggleSamples}, //displays the # of Sound samples in Queue & List
	{"showorders", kf_ToggleOrders}, //displays unit order/action state.
	{"pause", kf_TogglePauseMode}, // Pause the game.
	{"power info", kf_PowerInfo},
	{"reload me", kf_Reload},	// reload selected weapons immediately
	{"desync me", kf_ForceDesync},
	{"damage me", kf_DamageMe},
	{"autogame on", kf_AutoGame},
	{"autogame off", kf_AutoGame},

};

bool _attemptCheatCode(const char *cheat_name)
{
	const CHEAT_ENTRY *curCheat;
	static const CHEAT_ENTRY *const EndCheat = &cheatCodes[ARRAY_SIZE(cheatCodes)];

	// there is no reason to make people enter "cheat mode" to enter following commands
	if (!strcasecmp("showfps", cheat_name))
	{
		kf_ToggleFPS();
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
		if (strcasecmp(cheat_name, curCheat->pName) == 0)
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

bool attemptCheatCode(const char *cheat_name)
{
	bool result = _attemptCheatCode(cheat_name);
	if (result)
	{
		ActivityManager::instance().cheatUsed(cheat_name);
	}
	return result;
}

void sendProcessDebugMappings(bool val)
{
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_DEBUG_MODE);
	NETbool(&val);
	NETend();
}

void recvProcessDebugMappings(NETQUEUE queue)
{
	bool val = false;
	NETbeginDecode(queue, GAME_DEBUG_MODE);
	NETbool(&val);
	NETend();

	bool oldDebugMode = getDebugMappingStatus();
	processDebugMappings(queue.index, val);
	bool newDebugMode = getDebugMappingStatus();

	std::string cmsg;
	if (val)
	{
		cmsg = astringf(_("%s wants to enable debug mode. Enabled: %s, Disabled: %s."), getPlayerName(queue.index), getWantedDebugMappingStatuses(true).c_str(), getWantedDebugMappingStatuses(false).c_str());
	}
	else
	{
		cmsg = astringf(_("%s wants to disable debug mode. Enabled: %s, Disabled: %s."), getPlayerName(queue.index), getWantedDebugMappingStatuses(true).c_str(), getWantedDebugMappingStatuses(false).c_str());
	}
	addConsoleMessage(cmsg.c_str(), DEFAULT_JUSTIFY,  SYSTEM_MESSAGE);

	if (!oldDebugMode && newDebugMode)
	{
		addConsoleMessage(_("Debug mode now enabled!"), DEFAULT_JUSTIFY,  SYSTEM_MESSAGE);
		Cheated = true;
		triggerEventCheatMode(true);
	}
	else if (oldDebugMode && !newDebugMode)
	{
		addConsoleMessage(_("Debug mode now disabled!"), DEFAULT_JUSTIFY,  SYSTEM_MESSAGE);
		triggerEventCheatMode(false);
	}
}
