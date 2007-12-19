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
/* Handles cheat codes for Warzone */
/* Alex M 19th - Jan. 1999 */

#include "lib/framework/frame.h"
#include "cheat.h"
#include "console.h"
#include "keybind.h"

typedef struct _cheat_entry
{
	const char *pName;
	void (*function)(void);	// pointer to void* function
} CHEAT_ENTRY;

static CHEAT_ENTRY cheatCodes[] =
{
//	{"VQKZMY^\\Z",kf_ToggleOverlays},//interface
//	{"LWPH R^OOVQXL",kf_ShowMappings},//show mappings
//	{"KZROS^KZL",kf_GiveTemplateSet},//templates
//	{"LZSZ\\K ^SS",kf_SelectAllCombatUnits},//select all
//	{"SZK KWZMZ ]Z SVXWK",kf_RecalcLighting},//let there be light
//	{"PJKSVQZ,",kf_ToggleOutline},
//	{"L\\MZZQ[JRO",kf_ScreenDump},	//screendump

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
	{"no faults", kf_NoFaults},//carol vorderman
	{"tileinfo", kf_TileInfo}, // output debug info about a tile
	{"showfps", kf_ToggleFPS},	//displays your average FPS
	{"showsamples", kf_ToggleSamples}, //displays the # of Sound samples in Queue & List
	{"end of list",NULL}
};

BOOL attemptCheatCode(const char* cheat_name)
{
	UDWORD	index;
	char	errorString[255];

	index = 0;

	while(cheatCodes[index].function!=NULL)
	{
		if (strcmp(cheat_name, cheatCodes[index].pName) == 0)
		{
			/* We've got our man... */
			cheatCodes[index].function();	// run it
			/* And get out of here */
			return(TRUE);
		}
		index++;
	}
	/* We didn't find it */
	strlcpy(errorString, cheat_name, sizeof(errorString));
	strlcat(errorString, "?", sizeof(errorString));

	addConsoleMessage(errorString,LEFT_JUSTIFY);
	return(FALSE);
}
