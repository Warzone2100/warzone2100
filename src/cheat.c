/* Handles cheat codes for Warzone */
/* Alex M 19th - Jan. 1999 */

#include "frame.h"
#include "cheat.h"
#include "console.h"
#include "keybind.h"

static	BOOL	bAllowCheatCodes = TRUE;

typedef struct _cheat_entry
{
STRING	*pName;
void (*function)(void);	// pointer to void* function
} CHEAT_ENTRY;

CHEAT_ENTRY	cheatCodes[] = 
{
//	{"OPHZM",kf_TogglePower},		//power
//	{"VQKZMY^\\Z",kf_ToggleOverlays},//interface
//	{"XVIZ ^SS",kf_AllAvailable},	// give all
//	{"LWPH R^OOVQXL",kf_ShowMappings},//show mappings
//	{"KZROS^KZL",kf_GiveTemplateSet},//templates
//	{"LZSZ\\K ^SS",kf_SelectAllCombatUnits},//select all
//	{"YMPR ^]PIZ",kf_ToggleGodMode},//from above
//	{"SZK KWZMZ ]Z SVXWK",kf_RecalcLighting},//let there be light
//	{"YVQVLW QPH",kf_FinishResearch},//finish now
//	{"PJKSVQZ,",kf_ToggleOutline},
//	{"L\\MZZQ[JRO",kf_ScreenDump},	//screendump
//	{"M^QXZL",kf_ToggleSensorDisplay},//ranges
//	{"JQVK LK^KL",kf_DebugDroidInfo},//unit stats

	{"W^SSP RZVQ L\\W^KE",kf_AddMissionOffWorld},//let me win
	{"KVRZ[ZRP",kf_FrameRate},	 //timedemo
	{"TVSS LZsZ\\KZ[",kf_KillSelected},//kill slected
	{"[ZRP RP[Z",kf_ToggleDemoMode},	//demo mode
	{"UPWQ TZKKSZF",kf_ToggleWeather},//john kettley
	{"WPH Y^LK",kf_FrameRate},//how fast?
	{"LW^TZF",kf_ToggleShakeStatus},//shakey
	{"RPJLZYSVO",kf_ToggleMouseInvert},//mouseflip
	{"]VYYZM ]^TZM",kf_SetKillerLevel},//biffa
	{"LO^MTSZ XMZZQ",kf_SetKillerLevel}, //biffa   
	{"Z^LF",kf_SetEasyLevel},//easy
	{"QPMR^S",kf_SetNormalLevel},//normal
	{"W^M[",kf_SetHardLevel},//hard
	{"[PJ]SZ JO",kf_SetToughUnitsLevel},	// your units take half the damage
	{"HW^SZ YVQ",kf_TogglePower},	// turns on/off infinte power
	{"XZK PYY RF S^Q[",kf_KillEnemy},	// kills all enemy units and structures
	{"IZMLVPQ",kf_BuildInfo},	// tells you when the game was built
	{"KVRZ KPXXSZ",kf_ToggleMissionTimer},
	{"HPMT W^M[ZM",kf_FinishResearch}, 
	{"\\^MPS IPM[ZMR^Q",kf_NoFaults},//carol vorderman
	{"end of list",NULL}
};

char	cheatString[255];

unsigned char	*xorString(unsigned char *string)
{
unsigned char	*pReturn;

	pReturn = string;

	while(*pReturn)
	{
		if(*pReturn>32)
		{
			*pReturn = (UBYTE)(*pReturn ^ 0x3f);
		}
		pReturn++;
	}
	return(string);
}

void	setCheatCodeStatus(BOOL val)
{
	bAllowCheatCodes = val;
}

BOOL	getCheatCodeStatus( void )
{
	return(bAllowCheatCodes);
}

BOOL	attemptCheatCode( STRING *pName )
{
	UDWORD	index;
	STRING	errorString[255];
	char	*xored;

	index = 0;
 
	while(cheatCodes[index].function!=NULL)
	{
		strcpy(cheatString,cheatCodes[index].pName);
		xored = xorString(cheatString);
		if(strcmp(pName,xored) == FALSE)	// strcmp oddity
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

