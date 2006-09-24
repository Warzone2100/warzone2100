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
	{"end of list",NULL}
};

BOOL	attemptCheatCode( STRING *pName )
{
	UDWORD	index;
	STRING	errorString[255];

	index = 0;

	while(cheatCodes[index].function!=NULL)
	{
		if (strcmp(pName, cheatCodes[index].pName) == 0)
		{
			/* We've got our man... */
			cheatCodes[index].function();	// run it
			/* And get out of here */
			return(TRUE);
		}
		index++;
	}
	/* We didn't find it */
	sprintf(errorString,"%s?",pName);
	addConsoleMessage(errorString,LEFT_JUSTIFY);
	return(FALSE);
}
