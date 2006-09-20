#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "text.h"
#include "keymap.h"
#include "console.h"
#include "keybind.h"
#include "display3d.h"

#include "keyedit.h"


/*
	KeyMap.c
	Alex McLean
	Pumpkin Studios, EIDOS Interactive.
	Internal Use Only
	-----------------

	Handles the assignment of functions to keys.
*/

// ----------------------------------------------------------------------------------
/* Function Prototypes */
KEY_MAPPING	*keyAddMapping		( KEY_STATUS status, KEY_CODE metaCode, KEY_CODE subcode,
								 KEY_ACTION action, void (*pKeyMapFunc)(void), STRING *name );
BOOL	keyRemoveMapping		( KEY_CODE metaCode, KEY_CODE subCode );
BOOL	keyRemoveMappingPt		( KEY_MAPPING *psToRemove );
KEY_MAPPING *keyFindMapping		( KEY_CODE metaCode, KEY_CODE subCode );
void	keyClearMappings		( void );
void	keyProcessMappings		( BOOL bExclude );
UDWORD	getNumMappings			( void );
void	keyInitMappings			( BOOL bForceDefaults );
void	keyShowMapping			( KEY_MAPPING *psMapping );
void	keyAllMappingsInactive	( void );
void	keyAllMappingsActive	( void );
void	keySetMappingStatus		( KEY_MAPPING *psMapping, BOOL state );
void	processDebugMappings	( BOOL val );
BOOL	getDebugMappingStatus	( void );
BOOL	keyReAssignMappingName(STRING *pName, KEY_CODE newMetaCode, KEY_CODE newSubCode);

BOOL	keyReAssignMapping( KEY_CODE origMetaCode, KEY_CODE origSubCode,
							KEY_CODE newMetaCode, KEY_CODE newSubCode );
KEY_MAPPING	*getKeyMapFromName(STRING *pName);

extern BOOL	bAllowDebugMode;

// ----------------------------------------------------------------------------------
/* WIN 32 specific */

BOOL	checkQwertyKeys			( void );
UDWORD	asciiKeyCodeToTable		( KEY_CODE code );
KEY_CODE	getQwertyKey		( void );
UDWORD	getMarkerX				( KEY_CODE code );
UDWORD	getMarkerY				( KEY_CODE code );
SDWORD	getMarkerSpin			( KEY_CODE code );

// ----------------------------------------------------------------------------------
KEY_MAPPING	*keyGetMappingFromFunction(void	*function)
{
KEY_MAPPING	*psMapping,*psReturn;

	for(psMapping = keyMappings,psReturn = NULL;
		psMapping AND !psReturn;
		psMapping = psMapping->psNext)
		{
			if(psMapping->function == function)
			{
				psReturn = psMapping;
			}
		}

	return(psReturn);
}
// ----------------------------------------------------------------------------------
/* Some win32 specific stuff allowing the user to add key mappings themselves */

#define	NUM_QWERTY_KEYS	26
typedef	struct	_keymap_Marker
{
KEY_MAPPING	*psMapping;
UDWORD	xPos,yPos;
SDWORD	spin;
} KEYMAP_MARKER;
static	KEYMAP_MARKER	qwertyKeyMappings[NUM_QWERTY_KEYS];


static	BOOL			bDoingDebugMappings = FALSE; // PSX needs this too...
// ----------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------
/* The linked list of present key mappings */
KEY_MAPPING	*keyMappings;

/* Holds number of active mappings */
UDWORD	numActiveMappings;

/* Last meta and sub key that were recorded */
static KEY_CODE	lastMetaKey,lastSubKey;
static BOOL	bKeyProcessing = TRUE;

// ----------------------------------------------------------------------------------
// Adding a mapped function ? add a save pointer! Thank AlexL.
// don't bugger around with the order either. new ones go at the end! DEBUG in debug section..

//typedef void (*_keymapsave)(void);
_keymapsave keyMapSaveTable[] =
{
	kf_ChooseManufacture,
	kf_ChooseResearch,
	kf_ChooseBuild,
	kf_ChooseDesign,
	kf_ChooseIntelligence,
	kf_ChooseCommand,
	kf_ToggleRadar,
	kf_ToggleConsole,
	kf_ToggleEnergyBars,
	kf_ToggleReloadBars,
	kf_ScreenDump ,
	kf_MoveToLastMessagePos,
	kf_AssignGrouping_1,
	kf_AssignGrouping_2,
	kf_AssignGrouping_3,
	kf_AssignGrouping_4,
	kf_AssignGrouping_5,
	kf_AssignGrouping_6,
	kf_AssignGrouping_7,
	kf_AssignGrouping_8,
	kf_AssignGrouping_9,
	kf_SelectGrouping_1,
	kf_SelectGrouping_2,
	kf_SelectGrouping_3,
	kf_SelectGrouping_4,
	kf_SelectGrouping_5,
	kf_SelectGrouping_6,
	kf_SelectGrouping_7,
	kf_SelectGrouping_8,
	kf_SelectGrouping_9,
	kf_addMultiMenu,
	kf_multiAudioStart,
	kf_multiAudioStop,
	kf_SeekNorth,
	kf_ToggleCamera,
	kf_addInGameOptions,
	kf_RadarZoomOut,
	kf_RadarZoomIn,
	kf_ZoomOut,
	kf_ZoomIn,
	kf_PitchForward,
	kf_RotateLeft,
	kf_ResetPitch,
	kf_RotateRight,
	kf_PitchBack,
	kf_RightOrderMenu,
	kf_JumpToResourceExtractor,
	kf_JumpToRepairUnits,
	kf_JumpToConstructorUnits,
	kf_JumpToSensorUnits,
	kf_JumpToCommandUnits,
	kf_ToggleOverlays,
	kf_CentreOnBase,
	kf_SetDroidAttackCease ,
	kf_JumpToUnassignedUnits ,
	kf_SetDroidAttackReturn ,
	kf_SetDroidAttackAtWill ,
	kf_SetDroidReturnToBase ,
	kf_SetDroidRangeDefault,
	kf_ToggleFormationSpeedLimiting,
	kf_SetDroidRangeShort,
	kf_SetDroidMovePursue ,
	kf_SetDroidMovePatrol ,
	kf_SetDroidGoForRepair ,
	kf_SetDroidMoveHold ,
	kf_SendTextMessage,
	kf_SetDroidRangeLong,
	kf_ScatterDroids,
	kf_SetDroidRetreatMedium,
	kf_SetDroidRetreatHeavy,
	kf_SetDroidRetreatNever,
	kf_SelectAllCombatUnits,
	kf_SelectAllDamaged,
	kf_SelectAllHalfTracked,
	kf_SelectAllHovers,
	kf_SetDroidRecycle,
	kf_SelectAllOnScreenUnits,
	kf_SelectAllTracked,
	kf_SelectAllUnits,
	kf_SelectAllVTOLs,
	kf_SelectAllWheeled,
	kf_FinishResearch,
	kf_FrameRate,
	kf_SelectAllSameType,
	kf_SelectNextFactory,
	kf_SelectNextResearch,
	kf_SelectNextPowerStation,
	kf_SelectNextCyborgFactory,
	kf_ToggleConsoleDrop,
	kf_SelectCommander_1,
	kf_SelectCommander_2,
	kf_SelectCommander_3,
	kf_SelectCommander_4,
	kf_SelectCommander_5,
	kf_SelectCommander_6,
	kf_SelectCommander_7,
	kf_SelectCommander_8,
	kf_SelectCommander_9,
	kf_FaceNorth,
	kf_FaceSouth,
	kf_FaceWest,
	kf_FaceEast,
	kf_SpeedUp,
	kf_SlowDown,
	kf_NormalSpeed,
	kf_ToggleRadarJump,
	kf_MovePause,
	kf_ToggleReopenBuildMenu,
	kf_SensorDisplayOn,
	kf_SensorDisplayOff,
	kf_ToggleRadarTerrain,          //radar terrain on/off
	kf_ToggleRadarAllyEnemy,        //enemy/ally radar color toggle
	kf_ToggleSensorDisplay,		//  Was commented out below. moved also!.  Re-enabled --Q 5/10/05
	kf_AddHelpBlip,				//Add a beacon
	kf_AllAvailable,
	kf_ToggleDebugMappings,
	kf_NewPlayerPower,
	kf_TogglePauseMode,
	kf_MaxScrollLimits,
	kf_DebugDroidInfo,
	kf_RecalcLighting,
	kf_ToggleFog,
	kf_ChooseOptions,
	kf_TogglePower,
	kf_ToggleWeather,
	kf_SelectPlayer,
	kf_ToggleMistFog,
	kf_ToggleFogColour,
	kf_AddMissionOffWorld,
	kf_KillSelected,
	kf_ShowMappings,
	kf_GiveTemplateSet,
	kf_ToggleVisibility,
	kf_FinishResearch,
	kf_LowerTile,
	kf_ToggleDemoMode,
	kf_ToggleGodMode,
	kf_EndMissionOffWorld,
	kf_SystemClose,
	kf_ToggleShadows,
	kf_RaiseTile,
	kf_ToggleOutline,
	kf_TriFlip,
	kf_UpDroidScale,
	kf_DownDroidScale,

	NULL		// last function!
};


// ----------------------------------------------------------------------------------
/*
	Here is where we assign functions to keys and to combinations of keys.
	these will be read in from a .cfg file customisable by the player from
	an in-game menu
*/
void	keyInitMappings( BOOL bForceDefaults )
{
	UDWORD	i;
	keyMappings = NULL;
	numActiveMappings = 0;
	bKeyProcessing = TRUE;
	processDebugMappings(FALSE);


	for(i=0; i<NUM_QWERTY_KEYS; i++)
	{
		qwertyKeyMappings[i].psMapping = NULL;
	}

	// load the mappings.
	if(!bForceDefaults && loadKeyMap() == TRUE)
	{
		return;
	}
	// mappings failed to load, so set the defaults.


	// ********************************* ALL THE MAPPINGS ARE NOW IN ORDER, PLEASE ****
	// ********************************* DO NOT REORDER THEM!!!!!! ********************
	/* ALL OF THIS NEEDS TO COME IN OFF A USER CUSTOMISABLE TEXT FILE */
	//                                **********************************
	//                                **********************************
	//									FUNCTION KEY MAPPINGS F1 to F12
	keyAddMapping(KEYMAP_ALWAYS_PROCESS,KEY_IGNORE,KEY_F1,KEYMAP_PRESSED,kf_ChooseManufacture,		strresGetString(psStringRes,STR_RET_MANUFACTURE));
	keyAddMapping(KEYMAP_ALWAYS_PROCESS,KEY_IGNORE,KEY_F2,KEYMAP_PRESSED,kf_ChooseResearch,			strresGetString(psStringRes,STR_RET_RESEARCH));
	keyAddMapping(KEYMAP_ALWAYS_PROCESS,KEY_IGNORE,KEY_F3,KEYMAP_PRESSED,kf_ChooseBuild,			strresGetString(psStringRes,STR_RET_BUILD));
	keyAddMapping(KEYMAP_ALWAYS_PROCESS,KEY_IGNORE,KEY_F4,KEYMAP_PRESSED,kf_ChooseDesign,			strresGetString(psStringRes,STR_RET_DESIGN));
   	keyAddMapping(KEYMAP_ALWAYS_PROCESS,KEY_IGNORE,KEY_F5,KEYMAP_PRESSED,kf_ChooseIntelligence,		strresGetString(psStringRes,STR_RET_INTELLIGENCE));
   	keyAddMapping(KEYMAP_ALWAYS_PROCESS,KEY_IGNORE,KEY_F6,KEYMAP_PRESSED,kf_ChooseCommand,			strresGetString(psStringRes,STR_RET_COMMAND));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_F7,KEYMAP_PRESSED,kf_ToggleRadar,			strresGetString(psStringRes,STR_BIND_TOGRAD));
  	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_F8,KEYMAP_PRESSED,kf_ToggleConsole,			strresGetString(psStringRes,STR_BIND_TOGCON));
  	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_F9,KEYMAP_PRESSED,kf_ToggleEnergyBars,			strresGetString(psStringRes,STR_BIND_BARS));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_F10,KEYMAP_PRESSED,kf_ScreenDump,				strresGetString(psStringRes,STR_BIND_SHOT));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_F11,KEYMAP_PRESSED,kf_ToggleFormationSpeedLimiting,			strresGetString(psStringRes,STR_BIND_SPLIM));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_F12,KEYMAP_PRESSED,kf_MoveToLastMessagePos, strresGetString(psStringRes,STR_BIND_PREV));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LSHIFT,KEY_F12,KEYMAP_PRESSED,kf_ToggleSensorDisplay,"Toggle Sensor display"); //Which key should we use? --Re enabled see below! -Q 5-10-05
	//                                **********************************
	//                                **********************************
	//										ASSIGN GROUPS
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_1,KEYMAP_PRESSED,kf_AssignGrouping_1,				strresGetString(psStringRes,STR_BIND_AS1));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_2,KEYMAP_PRESSED,kf_AssignGrouping_2,				strresGetString(psStringRes,STR_BIND_AS2));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_3,KEYMAP_PRESSED,kf_AssignGrouping_3,				strresGetString(psStringRes,STR_BIND_AS3));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_4,KEYMAP_PRESSED,kf_AssignGrouping_4,				strresGetString(psStringRes,STR_BIND_AS4));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_5,KEYMAP_PRESSED,kf_AssignGrouping_5,				strresGetString(psStringRes,STR_BIND_AS5));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_6,KEYMAP_PRESSED,kf_AssignGrouping_6,				strresGetString(psStringRes,STR_BIND_AS6));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_7,KEYMAP_PRESSED,kf_AssignGrouping_7,				strresGetString(psStringRes,STR_BIND_AS7));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_8,KEYMAP_PRESSED,kf_AssignGrouping_8,				strresGetString(psStringRes,STR_BIND_AS8));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_9,KEYMAP_PRESSED,kf_AssignGrouping_9,				strresGetString(psStringRes,STR_BIND_AS9));
	//                                **********************************
	//                                **********************************
	//	SELECT GROUPS - Will jump to the group as well as select if group is ALREADY selected
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_1,KEYMAP_PRESSED,kf_SelectGrouping_1,				strresGetString(psStringRes,STR_BIND_GR1));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_2,KEYMAP_PRESSED,kf_SelectGrouping_2,				strresGetString(psStringRes,STR_BIND_GR2));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_3,KEYMAP_PRESSED,kf_SelectGrouping_3,				strresGetString(psStringRes,STR_BIND_GR3));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_4,KEYMAP_PRESSED,kf_SelectGrouping_4,				strresGetString(psStringRes,STR_BIND_GR4));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_5,KEYMAP_PRESSED,kf_SelectGrouping_5,				strresGetString(psStringRes,STR_BIND_GR5));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_6,KEYMAP_PRESSED,kf_SelectGrouping_6,				strresGetString(psStringRes,STR_BIND_GR6));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_7,KEYMAP_PRESSED,kf_SelectGrouping_7,				strresGetString(psStringRes,STR_BIND_GR7));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_8,KEYMAP_PRESSED,kf_SelectGrouping_8,				strresGetString(psStringRes,STR_BIND_GR8));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_9,KEYMAP_PRESSED,kf_SelectGrouping_9,				strresGetString(psStringRes,STR_BIND_GR9));
	//                                **********************************
	//                                **********************************
	//	SELECT COMMANDER - Will jump to the group as well as select if group is ALREADY selected
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_1,KEYMAP_PRESSED,kf_SelectCommander_1,				strresGetString(psStringRes,STR_BIND_CMD1));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_2,KEYMAP_PRESSED,kf_SelectCommander_2,				strresGetString(psStringRes,STR_BIND_CMD2));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_3,KEYMAP_PRESSED,kf_SelectCommander_3,				strresGetString(psStringRes,STR_BIND_CMD3));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_4,KEYMAP_PRESSED,kf_SelectCommander_4,				strresGetString(psStringRes,STR_BIND_CMD4));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_5,KEYMAP_PRESSED,kf_SelectCommander_5,				strresGetString(psStringRes,STR_BIND_CMD5));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_6,KEYMAP_PRESSED,kf_SelectCommander_6,				strresGetString(psStringRes,STR_BIND_CMD6));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_7,KEYMAP_PRESSED,kf_SelectCommander_7,				strresGetString(psStringRes,STR_BIND_CMD7));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_8,KEYMAP_PRESSED,kf_SelectCommander_8,				strresGetString(psStringRes,STR_BIND_CMD8));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_9,KEYMAP_PRESSED,kf_SelectCommander_9,				strresGetString(psStringRes,STR_BIND_CMD9));
	//                                **********************************
	//                                **********************************
	//	MULTIPLAYER
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KPENTER,KEYMAP_PRESSED,kf_addMultiMenu,		strresGetString(psStringRes,STR_BIND_MULOP));
	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_KP_FULLSTOP,KEYMAP_PRESSED,kf_multiAudioStart,	strresGetString(psStringRes,STR_BIND_AUDON));
	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_KP_FULLSTOP,KEYMAP_RELEASED,kf_multiAudioStop,	strresGetString(psStringRes,STR_BIND_AUDOFF));
	//
	//	GAME CONTROLS - Moving around, zooming in, rotating etc
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_BACKSPACE,KEYMAP_PRESSED,kf_SeekNorth,		strresGetString(psStringRes,STR_BIND_NORTH));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_SPACE,KEYMAP_PRESSED,kf_ToggleCamera,		strresGetString(psStringRes,STR_BIND_TRACK));
	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_ESC,KEYMAP_PRESSED,kf_addInGameOptions,			strresGetString(psStringRes,STR_BIND_OPT));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_MINUS,KEYMAP_PRESSED,kf_RadarZoomOut,		strresGetString(psStringRes,STR_BIND_RIN));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_EQUALS,KEYMAP_PRESSED,kf_RadarZoomIn,		strresGetString(psStringRes,STR_BIND_ROUT));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_MINUS,KEYMAP_DOWN,kf_ZoomOut,				strresGetString(psStringRes,STR_BIND_ZOUT));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_PLUS,KEYMAP_DOWN,kf_ZoomIn,				strresGetString(psStringRes,STR_BIND_ZIN));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_2,KEYMAP_DOWN,kf_PitchForward,			strresGetString(psStringRes,STR_BIND_PF));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_4,KEYMAP_DOWN,kf_RotateLeft,				strresGetString(psStringRes,STR_BIND_RL));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_5,KEYMAP_DOWN,kf_ResetPitch,				strresGetString(psStringRes,STR_BIND_RP));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_6,KEYMAP_DOWN,kf_RotateRight,				strresGetString(psStringRes,STR_BIND_RR));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_8,KEYMAP_DOWN,kf_PitchBack,				strresGetString(psStringRes,STR_BIND_PB));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_0,KEYMAP_PRESSED,kf_RightOrderMenu,		strresGetString(psStringRes,STR_BIND_ORD));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_MINUS,KEYMAP_PRESSED,kf_SlowDown,				strresGetString(psStringRes,STR_BIND_SLOW_DOWN));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_EQUALS,KEYMAP_PRESSED,kf_SpeedUp,				strresGetString(psStringRes,STR_BIND_SPEED_UP));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_BACKSPACE,KEYMAP_PRESSED,kf_NormalSpeed,		strresGetString(psStringRes,STR_BIND_NORMAL_SPEED));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_UPARROW,KEYMAP_PRESSED,kf_FaceNorth, strresGetString(psStringRes,STR_BIND_UP));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_DOWNARROW,KEYMAP_PRESSED,kf_FaceSouth,strresGetString(psStringRes,STR_BIND_DOWN) );
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_LEFTARROW,KEYMAP_PRESSED,kf_FaceEast, strresGetString(psStringRes,STR_BIND_RIGHT));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_RIGHTARROW,KEYMAP_PRESSED,kf_FaceWest, strresGetString(psStringRes,STR_BIND_LEFT));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_STAR,KEYMAP_PRESSED,kf_JumpToResourceExtractor,	strresGetString(psStringRes,STR_BIND_RESJ));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_JumpToRepairUnits,	strresGetString(psStringRes,STR_BIND_REPJ));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_JumpToConstructorUnits,	strresGetString(psStringRes,STR_BIND_CONJ));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_JumpToSensorUnits,	strresGetString(psStringRes,STR_BIND_SENJ));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_JumpToCommandUnits,	strresGetString(psStringRes,STR_BIND_COMJ));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_TAB,KEYMAP_PRESSED,kf_ToggleOverlays,			strresGetString(psStringRes,STR_BIND_OVERL));
	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_BACKQUOTE,KEYMAP_PRESSED,kf_ToggleConsoleDrop,strresGetString(psStringRes,STR_BIND_CONSOLE));
	//                                **********************************
	// IN GAME MAPPINGS - Single key presses - ALL __DEBUG keymappings will be removed for master
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_B,KEYMAP_PRESSED,kf_CentreOnBase,			strresGetString(psStringRes,STR_BIND_CENTV));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_C,KEYMAP_PRESSED,kf_SetDroidAttackCease ,	strresGetString(psStringRes,STR_BIND_CEASE));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_D,KEYMAP_PRESSED,kf_JumpToUnassignedUnits,	strresGetString(psStringRes,STR_BIND_UNITJ));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_E,KEYMAP_PRESSED,kf_SetDroidAttackReturn,	strresGetString(psStringRes,STR_BIND_ENGAG));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_F,KEYMAP_PRESSED,kf_SetDroidAttackAtWill,	strresGetString(psStringRes,STR_BIND_FAW));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_H,KEYMAP_PRESSED,kf_SetDroidReturnToBase,	strresGetString(psStringRes,STR_BIND_RTB));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_I,KEYMAP_PRESSED,kf_SetDroidRangeDefault,	strresGetString(psStringRes,STR_BIND_DEFR));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_O,KEYMAP_PRESSED,kf_SetDroidRangeShort,		strresGetString(psStringRes,STR_BIND_SHOR));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_P,KEYMAP_PRESSED,kf_SetDroidMovePursue ,		strresGetString(psStringRes,STR_BIND_PURS));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_Q,KEYMAP_PRESSED,kf_SetDroidMovePatrol ,		strresGetString(psStringRes,STR_BIND_PATR));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_R,KEYMAP_PRESSED,kf_SetDroidGoForRepair ,	strresGetString(psStringRes,STR_BIND_REPA));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_S,KEYMAP_PRESSED,kf_SetDroidMoveHold ,		strresGetString(psStringRes,STR_BIND_DSTOP));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_T,KEYMAP_PRESSED,kf_SendTextMessage,			strresGetString(psStringRes,STR_BIND_SENDT));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_U,KEYMAP_PRESSED,kf_SetDroidRangeLong,		strresGetString(psStringRes,STR_BIND_LONGR));

	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_H,KEYMAP_PRESSED,kf_AddHelpBlip,		"Drop a beacon");

	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_Z,KEYMAP_PRESSED,kf_SensorDisplayOn,		"Sensor display On");
	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_Z,KEYMAP_RELEASED,kf_SensorDisplayOff,	"Sensor display Off");
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_S,KEYMAP_PRESSED,kf_ToggleShadows, "Toggles shadows");
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_TAB,KEYMAP_PRESSED,kf_ToggleRadarTerrain,         "Toggle radar terrain");
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LSHIFT,KEY_TAB,KEYMAP_PRESSED,kf_ToggleRadarAllyEnemy,      "Toggle ally-enemy radar view");

	// Some extra non QWERTY mappings but functioning in same way
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_COMMA,KEYMAP_PRESSED,kf_SetDroidRetreatMedium,	   strresGetString(psStringRes,STR_BIND_LDAM) );
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_FULLSTOP,KEYMAP_PRESSED,kf_SetDroidRetreatHeavy,	   strresGetString(psStringRes,STR_BIND_HDAM) );
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_FORWARDSLASH,KEYMAP_PRESSED,kf_SetDroidRetreatNever,strresGetString(psStringRes,STR_BIND_NDAM) );
	//                                **********************************
	//                                **********************************
	//								In game mappings - COMBO (CTRL + LETTER) presses.
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_A,KEYMAP_PRESSED,kf_SelectAllCombatUnits,		strresGetString(psStringRes,STR_BIND_ACU));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_D,KEYMAP_PRESSED,kf_SelectAllDamaged,			strresGetString(psStringRes,STR_BIND_ABDU));

	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_F,KEYMAP_PRESSED,kf_SelectAllHalfTracked,		strresGetString(psStringRes,STR_BIND_AHTR));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_H,KEYMAP_PRESSED,kf_SelectAllHovers,			strresGetString(psStringRes,STR_BIND_AHOV));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_R,KEYMAP_PRESSED,kf_SetDroidRecycle,			strresGetString(psStringRes,STR_BIND_RECY));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_S,KEYMAP_PRESSED,kf_SelectAllOnScreenUnits,	strresGetString(psStringRes,STR_BIND_ASCR));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_T,KEYMAP_PRESSED,kf_SelectAllTracked,			strresGetString(psStringRes,STR_BIND_ATR));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_U,KEYMAP_PRESSED,kf_SelectAllUnits,			strresGetString(psStringRes,STR_BIND_ALL));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_V,KEYMAP_PRESSED,kf_SelectAllVTOLs,			strresGetString(psStringRes,STR_BIND_AVTOL));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_W,KEYMAP_PRESSED,kf_SelectAllWheeled,			strresGetString(psStringRes,STR_BIND_AWHE));
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_Y,KEYMAP_PRESSED,kf_FrameRate,					"Show frame rate");
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_Z,KEYMAP_PRESSED,kf_SelectAllSameType,		strresGetString(psStringRes,STR_BIND_ASIMIL));
	//                                **********************************
	//                                **********************************
	//									SELECT PLAYERS - DEBUG ONLY
 	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_SelectNextFactory,strresGetString(psStringRes,STR_BIND_SELFACTORY));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_SelectNextResearch,strresGetString(psStringRes,STR_BIND_SELRESEARCH));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_SelectNextPowerStation,strresGetString(psStringRes,STR_BIND_SELPOWER));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_SelectNextCyborgFactory,strresGetString(psStringRes,STR_BIND_SELCYBORG));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_ToggleReopenBuildMenu,strresGetString(psStringRes,STR_BIND_REOPEN_BUILD));

	// NOTE THIS!!!!!!!
	// available: ctrl+g, ctrl+e, ctrl+m
	keyAddMapping(KEYMAP___HIDE,KEY_LSHIFT,KEY_BACKSPACE,KEYMAP_PRESSED,kf_ToggleDebugMappings,			"TOGGLE Debug Mappings");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_SCROLLLOCK,KEYMAP_PRESSED,kf_TogglePauseMode,	strresGetString(psStringRes,STR_BIND_PAUSE));
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_J,KEYMAP_PRESSED,kf_MaxScrollLimits,				"Maximum scroll limits");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_M,KEYMAP_PRESSED,kf_ShowMappings,				"Show all keyboard mappings - use pause!");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_N,KEYMAP_PRESSED,kf_GiveTemplateSet,				"Give template set(s) to player 0 ");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_V,KEYMAP_PRESSED,kf_ToggleVisibility,			"Toggle visibility");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_W,KEYMAP_DOWN,kf_RaiseTile,					"Raise tile height");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_A,KEYMAP_DOWN,kf_LowerTile,						"Lower tile height");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_Y,KEYMAP_PRESSED,kf_ToggleDemoMode,				"Toggles on/off DEMO Mode");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_B,KEYMAP_PRESSED,kf_EndMissionOffWorld,			"End Mission");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_KP_MINUS,KEYMAP_PRESSED,kf_SystemClose,			"System Close (EXIT)");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_I,KEYMAP_PRESSED,kf_RecalcLighting,				"Recalculate lighting");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_J,KEYMAP_PRESSED,kf_ToggleFog,					"Toggles All fog");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_K,KEYMAP_PRESSED,kf_ToggleMistFog,				"Toggle Mist Fog");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_L,KEYMAP_PRESSED,kf_ToggleFogColour,				"Toggle Fog Colour Fog");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_N,KEYMAP_PRESSED,kf_NewPlayerPower,				"New game player power");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_O,KEYMAP_PRESSED,kf_ChooseOptions,				"Display Options Screen");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_Q,KEYMAP_PRESSED,kf_ToggleWeather,				"Trigger some weather");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F1,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  0");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F2,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  1");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F3,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  2");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F4,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  3");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F5,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  4");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F6,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  5");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F7,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  6");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F8,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  7");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_K,KEYMAP_PRESSED,kf_TriFlip,					"Flip terrain triangle");
// would be nice, but does not currently work - Per
//	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_P,KEYMAP_PRESSED,kf_ToggleOutline,				"Tile Outline");

	saveKeyMap();	// save out the default key mappings.

//  ------------------------ OLD STUFF - Store here!
	/*
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_F6,KEYMAP_DOWN,kf_UpGeoOffset,"Raise the geometric offset");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_F7,KEYMAP_DOWN,kf_DownGeoOffset,"Lower the geometric offset");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_F8,KEYMAP_DOWN,kf_UpDroidScale,"Increase droid Scaling");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_F9,KEYMAP_DOWN,kf_DownDroidScale,"Decrease droid Scaling");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_C,KEYMAP_PRESSED,kf_SimCloseDown,"Simulate Screen Close Down");
	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_D,KEYMAP_PRESSED,kf_ToggleDrivingMode,"Toggle Driving Mode");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_E,KEYMAP_PRESSED,kf_ToggleDroidInfo,"Display droid info whilst tracking");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_I,KEYMAP_PRESSED,kf_ToggleWidgets,"Toggle Widgets");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_J,KEYMAP_PRESSED,kf_ToggleRadarAllign,"Toggles Radar allignment");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_R,KEYMAP_PRESSED,kf_ShowNumObjects,"Show number of Objects");
	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_T,KEYMAP_PRESSED,kf_SendTextMessage,"Send Text Message");
	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_U,KEYMAP_PRESSED,kf_ToggleBackgroundFog,"Toggle Background Fog");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_V,KEYMAP_PRESSED,kf_BuildInfo,"Build date and time");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_Y,KEYMAP_PRESSED,kf_ToggleDemoMode,"Toggles on/off DEMO Mode");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_Z,KEYMAP_PRESSED,kf_ShowGridInfo,"DBPRINTF map grid coverage");
	*/

//  ------------------------ OLD STUFF - Store here!




}

// ----------------------------------------------------------------------------------
/* Adds a new mapping to the list */
//BOOL	keyAddMapping(KEY_CODE metaCode, KEY_CODE subCode, KEY_ACTION action,void *function, STRING *name)
KEY_MAPPING *keyAddMapping(KEY_STATUS status,KEY_CODE metaCode, KEY_CODE subCode, KEY_ACTION action,
					  void (*pKeyMapFunc)(void), STRING *name)
{
KEY_MAPPING	*newMapping;
BLOCK_HEAP  *psHeap;

	psHeap = memGetBlockHeap();
	memSetBlockHeap(NULL);
	/* Get some memory for our binding */
	newMapping = (KEY_MAPPING*)MALLOC(sizeof(KEY_MAPPING));

	ASSERT( newMapping != NULL, "Couldn't allocate memory for a key mapping" );

	/* Plus one for the terminator */



	newMapping->pName = (STRING*)MALLOC(strlen(name)+1);
	ASSERT( newMapping->pName != NULL, "Couldn't allocate the memory for the string in a mapping" );

	memSetBlockHeap(psHeap);


	/* Copy over the name */
	strcpy(newMapping->pName,name);

	/* Fill up our entries, first the ones that activate it */
	newMapping->metaKeyCode	= metaCode;
	newMapping->subKeyCode	= subCode;
	newMapping->status	= status;

	/* When it was last called - needed? */
	newMapping->lastCalled	= gameTime;

	/* And what gets called when it's activated */
	//newMapping->function	= function;
	newMapping->function	= pKeyMapFunc;

	/* Is it functional on the key being down or just pressed */
	newMapping->action	= action;

	newMapping->altMetaKeyCode = KEY_IGNORE;

	/* We always request only the left hand one */
	if(metaCode == KEY_LCTRL) {newMapping->altMetaKeyCode = KEY_RCTRL;}
	else if(metaCode == KEY_LALT) {newMapping->altMetaKeyCode = KEY_RALT;}
	else if(metaCode == KEY_LSHIFT) {newMapping->altMetaKeyCode = KEY_RSHIFT;}

	/* Set it to be active */
	newMapping->active = TRUE;
	/* Add it to the start of the list */
	newMapping->psNext = keyMappings;
	keyMappings = newMapping;
	numActiveMappings++;

	return(newMapping);
}

// ----------------------------------------------------------------------------------
/* Removes a mapping from the list specified by the key codes */
BOOL	keyRemoveMapping( KEY_CODE metaCode, KEY_CODE subCode )
{
KEY_MAPPING	*mapping;

	mapping = keyFindMapping(metaCode, subCode);
	return(keyRemoveMappingPt(mapping));
}

// ----------------------------------------------------------------------------------
/* Returns a pointer to a mapping if it exists - NULL otherwise */
KEY_MAPPING *keyFindMapping( KEY_CODE metaCode, KEY_CODE subCode )
{
KEY_MAPPING	*psCurr;

	/* See if we can find it */
	for(psCurr = keyMappings; psCurr != NULL; psCurr = psCurr->psNext)
		{
			if(psCurr->metaKeyCode == metaCode AND psCurr->subKeyCode == subCode)
			{
				return(psCurr);
			}
		}
	return(NULL);
}

// ----------------------------------------------------------------------------------
/* clears the mappings list and frees the memory */
void	keyClearMappings( void )
{
	while(keyMappings)
	{
		keyRemoveMappingPt(keyMappings);
	}
}


// ----------------------------------------------------------------------------------
/* Removes a mapping specified by a pointer */
BOOL	keyRemoveMappingPt(KEY_MAPPING *psToRemove)
{
KEY_MAPPING	*psPrev,*psCurr;

	if(psToRemove == NULL)
	{
		return(FALSE);
	}

	if(psToRemove == keyMappings AND keyMappings->psNext == NULL)
	{
		if (keyMappings->pName)	FREE(keyMappings->pName);		// ffs
		FREE(keyMappings);
		keyMappings = NULL;
		numActiveMappings = 0;
		return(TRUE);
	}

	/* See if we can find it */
	for(psPrev = NULL, psCurr = keyMappings;
		psCurr != NULL AND psCurr!=psToRemove;
		psPrev = psCurr, psCurr = psCurr->psNext)
		{
		  /*NOP*/
		}

		/* If it was found... */
	if(psCurr==psToRemove)
	{
		/* See if it was the first element */
		if(psPrev)
		{
			/* It wasn't */
			psPrev->psNext = psCurr->psNext;
		}
		else
		{
			/* It was */
			keyMappings = psCurr->psNext;
		}
		/* Free up the memory, first for the string  */
		if (psCurr->pName)	FREE(psCurr->pName);		// only free it if it was allocated in the first place (ffs)
		/* and then for the mapping itself */
		FREE(psCurr);
		numActiveMappings--;
		return(TRUE);
	}
	return(FALSE);
}

// ----------------------------------------------------------------------------------
/* Just returns how many are active */
UDWORD	getNumMappings( void )
{
	return(numActiveMappings);
}


// ----------------------------------------------------------------------------------
/* Manages update of all the active function mappings */
void	keyProcessMappings( BOOL bExclude )
{
KEY_MAPPING	*keyToProcess;
BOOL		bMetaKeyDown;
BOOL		bKeyProcessed;

	/* Bomb out if there are none */
	if(!keyMappings OR !numActiveMappings OR !bKeyProcessing)
	{
		return;
	}



	/* Jump out if we've got a new mapping */
  	(void) checkQwertyKeys();


	/* Check for the meta keys */
	if(keyDown(KEY_LCTRL) OR keyDown(KEY_RCTRL) OR keyDown(KEY_LALT)
		OR keyDown(KEY_RALT) OR keyDown(KEY_LSHIFT) OR keyDown(KEY_RSHIFT))
	{
		bMetaKeyDown = TRUE;
	}
	else
	{
		bMetaKeyDown = FALSE;
	}

 	/* Run through all our mappings */
 	for(keyToProcess = keyMappings; keyToProcess!=NULL; keyToProcess = keyToProcess->psNext)
	{
		/* We haven't acted upon it */
		bKeyProcessed = FALSE;
		if(!keyToProcess->active)
		{
			/* Get out if it's inactive */
			break;
		}
		/* Skip innappropriate ones when necessary */
		if(bExclude AND keyToProcess->status!=KEYMAP_ALWAYS_PROCESS)
		{
			break;
		}

		if(keyToProcess->subKeyCode == KEY_MAXSCAN)
		{
			continue;
		}


		if(keyToProcess->metaKeyCode==KEY_IGNORE AND !bMetaKeyDown AND
			!(keyToProcess->status==KEYMAP__DEBUG AND bDoingDebugMappings == FALSE) )
 		{
			switch(keyToProcess->action)
 			{
 			case KEYMAP_PRESSED:
 				/* Were the right keys pressed? */
 				if(keyPressed(keyToProcess->subKeyCode))
 				{
 					lastSubKey = keyToProcess->subKeyCode;
 					/* Jump to the associated function call */
 					 keyToProcess->function();
					 bKeyProcessed = TRUE;
 				}
 				break;
 			case KEYMAP_DOWN:
 				/* Is the key Down? */
 				if(keyDown(keyToProcess->subKeyCode))
 				{
 					lastSubKey = keyToProcess->subKeyCode;
 					/* Jump to the associated function call */
 					 keyToProcess->function();
					 bKeyProcessed = TRUE;
 				}

 				break;
			case KEYMAP_RELEASED:
 				/* Has the key been released? */
 				if(keyReleased(keyToProcess->subKeyCode))
 				{
 					lastSubKey = keyToProcess->subKeyCode;
 					/* Jump to the associated function call */
 					 keyToProcess->function();
					 bKeyProcessed = TRUE;
 				}

			break;

 			default:
				debug( LOG_ERROR, "Weirdy action on keymap processing" );
				abort();
 				break;
			}
 		}
		/* Process the combi ones */
 		if( (keyToProcess->metaKeyCode!=KEY_IGNORE AND bMetaKeyDown) AND
			!(keyToProcess->status==KEYMAP__DEBUG AND bDoingDebugMappings == FALSE))
 		{
 			/* It's a combo keypress - one held down and the other pressed */
 			if(keyDown(keyToProcess->metaKeyCode) AND keyPressed(keyToProcess->subKeyCode) )
 			{
 				lastMetaKey = keyToProcess->metaKeyCode;
 				lastSubKey = keyToProcess->subKeyCode;
 				keyToProcess->function();
				bKeyProcessed = TRUE;
 			}
			else if(keyToProcess->altMetaKeyCode!=KEY_IGNORE)
			{
				if(keyDown(keyToProcess->altMetaKeyCode) AND keyPressed(keyToProcess->subKeyCode))
				{
 					lastMetaKey = keyToProcess->metaKeyCode;
 					lastSubKey = keyToProcess->subKeyCode;
 					keyToProcess->function();
					bKeyProcessed = TRUE;
				}
			}
 		}
		if(bKeyProcessed)
		{
			if(keyToProcess->status==KEYMAP__DEBUG AND bDoingDebugMappings)
			{
				// this got really annoying. what purpose? - Per
				// CONPRINTF(ConsoleString,(ConsoleString,"DEBUG MAPPING : %s",keyToProcess->pName));
			}
		}
	}
}

// ----------------------------------------------------------------------------------

/* Allows _new_ mappings to be made at runtime */
BOOL	checkQwertyKeys( void )
{
KEY_CODE	qKey;
UDWORD		tableEntry;
BOOL		aquired;

	aquired = FALSE;
	/* Are we trying to make a new map marker? */
	if( keyDown(KEY_LALT))
	{
		/* Did we press a key */
		qKey = getQwertyKey();
		if(qKey)
		{
			tableEntry = asciiKeyCodeToTable(qKey);
			/* We're assigning something to the key */
			if(qwertyKeyMappings[tableEntry].psMapping)
			{
				/* Get rid of the old mapping on this key if there was one */
				keyRemoveMappingPt(qwertyKeyMappings[tableEntry].psMapping);
			}
			/* Now add the new one for this location */
			qwertyKeyMappings[tableEntry].psMapping =
				keyAddMapping(KEYMAP_ALWAYS,KEY_LSHIFT,qKey,KEYMAP_PRESSED,kf_JumpToMapMarker,"Jump to new map marker");
			aquired = TRUE;

			/* Store away the position and view angle */
			qwertyKeyMappings[tableEntry].xPos = player.p.x;
			qwertyKeyMappings[tableEntry].yPos = player.p.z;
			qwertyKeyMappings[tableEntry].spin = player.r.y;
		}
	}
	return(aquired);
}




// ----------------------------------------------------------------------------------
// this function isn't really module static - should be removed - debug only
void	keyShowMappings( void )
{
KEY_MAPPING	*psMapping;
	for(psMapping = keyMappings; psMapping; psMapping = psMapping->psNext)
	{
		keyShowMapping(psMapping);
	}
}

// ----------------------------------------------------------------------------------
/* Sends a particular key mapping to the console */
void	keyShowMapping(KEY_MAPPING *psMapping)
{
STRING	asciiSub[20],asciiMeta[20];
BOOL	onlySub;

	onlySub = TRUE;
	if(psMapping->metaKeyCode!=KEY_IGNORE)
	{
		keyScanToString(psMapping->metaKeyCode,(STRING *)&asciiMeta,20);
		onlySub = FALSE;
	}

	keyScanToString(psMapping->subKeyCode,(STRING *)&asciiSub,20);
	if(onlySub)
	{
   		CONPRINTF(ConsoleString,(ConsoleString,"%s - %s",asciiSub,psMapping->pName));
	}
	else
	{
		CONPRINTF(ConsoleString,(ConsoleString,"%s and %s - %s",asciiMeta,asciiSub,psMapping->pName));
	}
}

// ----------------------------------------------------------------------------------
/* Returns the key code of the last sub key pressed - allows called functions to have a simple stack */
KEY_CODE	getLastSubKey( void )
{
	return(lastSubKey);
}

// ----------------------------------------------------------------------------------
/* Returns the key code of the last meta key pressed - allows called functions to have a simple stack */
KEY_CODE	getLastMetaKey( void )
{
	return(lastMetaKey);
}

// ----------------------------------------------------------------------------------
/* Allows us to enable/disable the whole mapping system */
void	keyEnableProcessing( BOOL val )
{
	bKeyProcessing = val;
}

// ----------------------------------------------------------------------------------
/* Sets all mappings to be inactive */
void	keyAllMappingsInactive( void )
{
KEY_MAPPING	*psMapping;

	for(psMapping = keyMappings; psMapping; psMapping = psMapping->psNext)
	{
		psMapping->active = FALSE;
	}
}

// ----------------------------------------------------------------------------------
void	keyAllMappingsActive( void )
{
KEY_MAPPING	*psMapping;

	for(psMapping = keyMappings; psMapping; psMapping = psMapping->psNext)
	{
		psMapping->active = TRUE;
	}
}

// ----------------------------------------------------------------------------------
/* Allows us to make active/inactive specific mappings */
void	keySetMappingStatus(KEY_MAPPING *psMapping, BOOL state)
{
	psMapping->active = state;
}


/* Returns the key code of the first ascii key that its finds has been PRESSED */
KEY_CODE	getQwertyKey( void )
{
UDWORD	i;

	for(i = KEY_Q; i <= KEY_P; i++)
	{
		if(keyPressed(i))
		{
			return(i);	// top row key pressed
		}
	}

	for(i = KEY_A; i <= KEY_L; i++)
	{
		if(keyPressed(i))
		{
			return(i);	// middle row key pressed
		}
	}

	for(i = KEY_Z; i <= KEY_M; i++)
	{
		if(keyPressed(i))
		{
			return(i);	// bottomw row key pressed
		}
	}
	return(0);			// no ascii key pressed
}

// ----------------------------------------------------------------------------------
/*	Returns the number (0 to 26) of a key on the keyboard
	from it's keycode. Q is zero, through to M being 25
*/
UDWORD	asciiKeyCodeToTable(KEY_CODE code)
{
	if(code<=KEY_P)
	{
		code = code - KEY_Q;  // q is the first of the ascii scan codes
	}
	else if(code <=KEY_L)
	{
		code = (code - KEY_A) + 10;	// ten keys from q to p
	}
	else if(code<=KEY_M)
	{
		code = (code - KEY_Z) + 19;	// 19 keys before, the 10 from q..p and the 9 from a..l
	}

	return((UDWORD) code);
}

// ----------------------------------------------------------------------------------
/* Returns the map X position associated with the passed in keycode */
UDWORD	getMarkerX( KEY_CODE code )
{
UDWORD	entry;
	entry = asciiKeyCodeToTable(code);
	return(qwertyKeyMappings[entry].xPos);
}

// ----------------------------------------------------------------------------------
/* Returns the map Y position associated with the passed in keycode */
UDWORD	getMarkerY( KEY_CODE code )
{
UDWORD	entry;
	entry = asciiKeyCodeToTable(code);
	return(qwertyKeyMappings[entry].yPos);
}

// ----------------------------------------------------------------------------------
/* Returns the map Y rotation associated with the passed in keycode */
SDWORD	getMarkerSpin( KEY_CODE code )
{
UDWORD	entry;
	entry = asciiKeyCodeToTable(code);
	return(qwertyKeyMappings[entry].spin);
}


// ----------------------------------------------------------------------------------
/* Defines whether we process debug key mapping stuff */
void	processDebugMappings( BOOL val )
{
	bDoingDebugMappings = val;
}

// ----------------------------------------------------------------------------------
/* Returns present status of debug mapping processing */
BOOL	getDebugMappingStatus( void )
{
	return(bDoingDebugMappings);
}
// ----------------------------------------------------------------------------------
BOOL	keyReAssignMapping( KEY_CODE origMetaCode, KEY_CODE origSubCode,
							KEY_CODE newMetaCode, KEY_CODE newSubCode )
{
KEY_MAPPING	*psMapping;
BOOL		bFound;

	for(psMapping = keyMappings,bFound = FALSE; psMapping AND !bFound;
		psMapping = psMapping->psNext)
	{
		/* Find the original */
		if(psMapping->metaKeyCode == origMetaCode AND psMapping->subKeyCode == origSubCode)
		{
			/* Not all can be remapped */
			if(psMapping->status != KEYMAP_ALWAYS OR psMapping->status == KEYMAP_ALWAYS_PROCESS)
			{
				psMapping->metaKeyCode = newMetaCode;
				psMapping->subKeyCode = newSubCode;
				bFound = TRUE;
			}
		}
	}

	return(bFound);
}

/*
BOOL	keyReAssignMappingName(STRING *pName, KEY_CODE newMetaCode, KEY_CODE newSubCode)
							   )
{
KEY_MAPPING	*psMapping;
KEY_CODE	origMetaCode,origSubCode;
BOOL	bReplaced;

  	for(psMapping = keyMappings,bReplaced = FALSE; psMapping AND !bReplaced;
		psMapping = psMapping->psNext)
	{
		if(strcmp(psMapping->pName,pName) == FALSE)	//negative
		{
			if(psMapping->status==KEYMAP_ASSIGNABLE)
			{
				(void)keyAddMapping(psMapping->status,newMetaCode,
					newSubCode, psMapping->action,psMapping->function,psMapping->pName);
				bReplaced = TRUE;
				origMetaCode = psMapping->metaKeyCode;
				origSubCode = psMapping->subKeyCode;
			}
		}
	}

	if(bReplaced)
	{
		keyRemoveMapping(origMetaCode, origSubCode);
	}
	return(bReplaced);
}
*/
// ----------------------------------------------------------------------------------
KEY_MAPPING	*getKeyMapFromName(STRING *pName)
{
KEY_MAPPING	*psMapping;
		for(psMapping = keyMappings; psMapping;	psMapping = psMapping->psNext)
		{
			if(strcmp(pName,psMapping->pName) == FALSE)
			{
				return(psMapping);
			}
		}
	return(NULL);
}
// ----------------------------------------------------------------------------------
BOOL	keyReAssignMappingName(STRING *pName,KEY_CODE newMetaCode, KEY_CODE newSubCode)
{
KEY_MAPPING	*psMapping;


	psMapping = getKeyMapFromName(pName);
	if(psMapping)
	{
		if(psMapping->status == KEYMAP_ASSIGNABLE)
		{
			(void)keyAddMapping(psMapping->status,newMetaCode,
			newSubCode, psMapping->action,psMapping->function,psMapping->pName);
			keyRemoveMappingPt(psMapping);
			return(TRUE);
		}
	}
	return(FALSE);
}
// ----------------------------------------------------------------------------------
