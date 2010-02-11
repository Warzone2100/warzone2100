/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
	KeyMap.c
	Alex McLean
	Pumpkin Studios, EIDOS Interactive.
	Internal Use Only
	-----------------

	Handles the assignment of functions to keys.
*/

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/strres.h"
#include "lib/gamelib/gtime.h"
#include "keymap.h"
#include "console.h"
#include "keybind.h"
#include "display3d.h"
#include "keymap.h"
#include "keyedit.h"
#include "scriptcb.h"
#include "lib/script/script.h"
#include "scripttabs.h"


static UDWORD asciiKeyCodeToTable( KEY_CODE code );

// ----------------------------------------------------------------------------------
KEY_MAPPING	*keyGetMappingFromFunction(void	*function)
{
KEY_MAPPING	*psMapping,*psReturn;

	for(psMapping = keyMappings,psReturn = NULL;
		psMapping && !psReturn;
		psMapping = psMapping->psNext)
		{
			if ((void *)psMapping->function == function)
			{
				psReturn = psMapping;
			}
		}

	return(psReturn);
}
// ----------------------------------------------------------------------------------
/* Some stuff allowing the user to add key mappings themselves */

#define	NUM_QWERTY_KEYS	26
typedef	struct	_keymap_Marker
{
KEY_MAPPING	*psMapping;
UDWORD	xPos,yPos;
SDWORD	spin;
} KEYMAP_MARKER;
static	KEYMAP_MARKER	qwertyKeyMappings[NUM_QWERTY_KEYS];


static	BOOL			bDoingDebugMappings = false;
// ----------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------
/* The linked list of present key mappings */
KEY_MAPPING	*keyMappings;

/* Holds number of active mappings */
UDWORD	numActiveMappings;

/* Last meta and sub key that were recorded */
static KEY_CODE	lastMetaKey,lastSubKey;
static BOOL	bKeyProcessing = true;

static void kf_NOOP(void) {}

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
	kf_NOOP,
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
	kf_AssignGrouping_0,
	kf_SelectGrouping_0,
	kf_SelectCommander_0,
	kf_TraceObject,
	kf_NOOP, // unused
	kf_AddMissionOffWorld,
	kf_KillSelected,
	kf_ShowMappings,
	kf_NOOP, // unused
	kf_ToggleVisibility,
	kf_FinishResearch,
	kf_LowerTile,
	kf_ToggleDemoMode,
	kf_ToggleGodMode,
	kf_EndMissionOffWorld,
	kf_SystemClose,
	kf_ToggleShadows,
	kf_RaiseTile,
	kf_NOOP, // unused
	kf_TriFlip,
	kf_NOOP, // unused
	kf_NOOP, // unused
	kf_ToggleWatchWindow,
	kf_ToggleDrivingMode,
	kf_ToggleShowGateways,
	kf_ToggleShowPath,
	kf_MapCheck,
	kf_SetDroidGoToTransport,
	kf_SetDroidMoveGuard,
	kf_toggleTrapCursor,
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
	bKeyProcessing = true;
	processDebugMappings(false);


	for(i=0; i<NUM_QWERTY_KEYS; i++)
	{
		qwertyKeyMappings[i].psMapping = NULL;
	}

	// load the mappings.
	if(!bForceDefaults && loadKeyMap() == true)
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
	keyAddMapping(KEYMAP_ALWAYS_PROCESS,KEY_IGNORE,KEY_F1,KEYMAP_PRESSED,kf_ChooseManufacture,		_("Manufacture"));
	keyAddMapping(KEYMAP_ALWAYS_PROCESS,KEY_IGNORE,KEY_F2,KEYMAP_PRESSED,kf_ChooseResearch,			_("Research"));
	keyAddMapping(KEYMAP_ALWAYS_PROCESS,KEY_IGNORE,KEY_F3,KEYMAP_PRESSED,kf_ChooseBuild,			_("Build"));
	keyAddMapping(KEYMAP_ALWAYS_PROCESS,KEY_IGNORE,KEY_F4,KEYMAP_PRESSED,kf_ChooseDesign,			_("Design"));
   	keyAddMapping(KEYMAP_ALWAYS_PROCESS,KEY_IGNORE,KEY_F5,KEYMAP_PRESSED,kf_ChooseIntelligence,		_("Intelligence Display"));
   	keyAddMapping(KEYMAP_ALWAYS_PROCESS,KEY_IGNORE,KEY_F6,KEYMAP_PRESSED,kf_ChooseCommand,			_("Commanders"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_F7,KEYMAP_PRESSED,kf_ToggleRadar,			_("Toggle Radar"));
  	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_F8,KEYMAP_PRESSED,kf_ToggleConsole,			_("Toggle Console Display"));
  	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_F9,KEYMAP_PRESSED,kf_ToggleEnergyBars,			_("Toggle Damage Bars On/Off"));
	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_F10,KEYMAP_PRESSED,kf_ScreenDump,				_("Take Screen Shot"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_F11,KEYMAP_PRESSED,kf_ToggleFormationSpeedLimiting,			_("Toggle Formation Speed Limiting"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_F12,KEYMAP_PRESSED,kf_MoveToLastMessagePos, _("View Location of Previous Message"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LSHIFT,KEY_F12,KEYMAP_PRESSED,kf_ToggleSensorDisplay,"Toggle Sensor display"); //Which key should we use? --Re enabled see below! -Q 5-10-05
	//                                **********************************
	//                                **********************************
	//										ASSIGN GROUPS
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_0,KEYMAP_PRESSED,kf_AssignGrouping_0,				_("Assign Group 0"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_1,KEYMAP_PRESSED,kf_AssignGrouping_1,				_("Assign Group 1"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_2,KEYMAP_PRESSED,kf_AssignGrouping_2,				_("Assign Group 2"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_3,KEYMAP_PRESSED,kf_AssignGrouping_3,				_("Assign Group 3"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_4,KEYMAP_PRESSED,kf_AssignGrouping_4,				_("Assign Group 4"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_5,KEYMAP_PRESSED,kf_AssignGrouping_5,				_("Assign Group 5"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_6,KEYMAP_PRESSED,kf_AssignGrouping_6,				_("Assign Group 6"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_7,KEYMAP_PRESSED,kf_AssignGrouping_7,				_("Assign Group 7"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_8,KEYMAP_PRESSED,kf_AssignGrouping_8,				_("Assign Group 8"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_9,KEYMAP_PRESSED,kf_AssignGrouping_9,				_("Assign Group 9"));
	//                                **********************************
	//                                **********************************
	//	SELECT GROUPS - Will jump to the group as well as select if group is ALREADY selected
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_0,KEYMAP_PRESSED,kf_SelectGrouping_0,				_("Select Group 0"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_1,KEYMAP_PRESSED,kf_SelectGrouping_1,				_("Select Group 1"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_2,KEYMAP_PRESSED,kf_SelectGrouping_2,				_("Select Group 2"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_3,KEYMAP_PRESSED,kf_SelectGrouping_3,				_("Select Group 3"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_4,KEYMAP_PRESSED,kf_SelectGrouping_4,				_("Select Group 4"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_5,KEYMAP_PRESSED,kf_SelectGrouping_5,				_("Select Group 5"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_6,KEYMAP_PRESSED,kf_SelectGrouping_6,				_("Select Group 6"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_7,KEYMAP_PRESSED,kf_SelectGrouping_7,				_("Select Group 7"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_8,KEYMAP_PRESSED,kf_SelectGrouping_8,				_("Select Group 8"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_9,KEYMAP_PRESSED,kf_SelectGrouping_9,				_("Select Group 9"));
	//                                **********************************
	//                                **********************************
	//	SELECT COMMANDER - Will jump to the group as well as select if group is ALREADY selected
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_0,KEYMAP_PRESSED,kf_SelectCommander_0,				_("Select Commander 0"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_1,KEYMAP_PRESSED,kf_SelectCommander_1,				_("Select Commander 1"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_2,KEYMAP_PRESSED,kf_SelectCommander_2,				_("Select Commander 2"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_3,KEYMAP_PRESSED,kf_SelectCommander_3,				_("Select Commander 3"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_4,KEYMAP_PRESSED,kf_SelectCommander_4,				_("Select Commander 4"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_5,KEYMAP_PRESSED,kf_SelectCommander_5,				_("Select Commander 5"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_6,KEYMAP_PRESSED,kf_SelectCommander_6,				_("Select Commander 6"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_7,KEYMAP_PRESSED,kf_SelectCommander_7,				_("Select Commander 7"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_8,KEYMAP_PRESSED,kf_SelectCommander_8,				_("Select Commander 8"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_9,KEYMAP_PRESSED,kf_SelectCommander_9,				_("Select Commander 9"));
	//                                **********************************
	//                                **********************************
	//	MULTIPLAYER
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KPENTER,KEYMAP_PRESSED,kf_addMultiMenu,		_("Multiplayer Options / Alliance dialog"));
	//
	//	GAME CONTROLS - Moving around, zooming in, rotating etc
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_BACKSPACE,KEYMAP_PRESSED,kf_SeekNorth,		_("Snap View to North"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_SPACE,KEYMAP_PRESSED,kf_ToggleCamera,		_("Toggle Tracking Camera"));
	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_ESC,KEYMAP_PRESSED,kf_addInGameOptions,			_("Display In-Game Options"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_MINUS,KEYMAP_PRESSED,kf_RadarZoomOut,		_("Zoom Radar Out"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_EQUALS,KEYMAP_PRESSED,kf_RadarZoomIn,		_("Zoom Radar In"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_MINUS,KEYMAP_DOWN,kf_ZoomOut,				_("Zoom In"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_PLUS,KEYMAP_DOWN,kf_ZoomIn,				_("Zoom Out"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_2,KEYMAP_DOWN,kf_PitchForward,			_("Pitch Forward"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_4,KEYMAP_DOWN,kf_RotateLeft,				_("Rotate Left"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_5,KEYMAP_DOWN,kf_ResetPitch,				_("Reset Pitch"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_6,KEYMAP_DOWN,kf_RotateRight,				_("Rotate Right"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_8,KEYMAP_DOWN,kf_PitchBack,				_("Pitch Back"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_0,KEYMAP_PRESSED,kf_RightOrderMenu,		_("Orders Menu"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_MINUS,KEYMAP_PRESSED,kf_SlowDown,				_("Decrease Game Speed"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_EQUALS,KEYMAP_PRESSED,kf_SpeedUp,				_("Increase Game Speed"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_BACKSPACE,KEYMAP_PRESSED,kf_NormalSpeed,		_("Reset Game Speed"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_UPARROW,KEYMAP_PRESSED,kf_FaceNorth, _("View North"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_DOWNARROW,KEYMAP_PRESSED,kf_FaceSouth,_("View South") );
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_LEFTARROW,KEYMAP_PRESSED,kf_FaceEast, _("View East"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_RIGHTARROW,KEYMAP_PRESSED,kf_FaceWest, _("View West"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_KP_STAR,KEYMAP_PRESSED,kf_JumpToResourceExtractor,	_("View next Oil Derrick"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_JumpToRepairUnits,	_("View next Repair Unit"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_JumpToConstructorUnits,	_("View next Truck"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_JumpToSensorUnits,	_("View next Sensor Unit"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_JumpToCommandUnits,	_("View next Commander"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_TAB,KEYMAP_PRESSED,kf_ToggleOverlays,			_("Toggle Overlays"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_BACKQUOTE,KEYMAP_PRESSED,kf_ToggleConsoleDrop,_("Console On/Off"));
	//                                **********************************
	// IN GAME MAPPINGS - Single key presses - ALL __DEBUG keymappings will be removed for master
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_B,KEYMAP_PRESSED,kf_CentreOnBase,			_("Center View on HQ"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_C,KEYMAP_PRESSED,kf_SetDroidAttackCease ,	_("Hold Fire"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_D,KEYMAP_PRESSED,kf_JumpToUnassignedUnits,	_("View Unassigned Units"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_E,KEYMAP_PRESSED,kf_SetDroidAttackReturn,	_("Return Fire"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_F,KEYMAP_PRESSED,kf_SetDroidAttackAtWill,	_("Fire at Will"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_G,KEYMAP_PRESSED,kf_SetDroidMoveGuard,		_("Guard Position"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_H,KEYMAP_PRESSED,kf_SetDroidReturnToBase,	_("Return to HQ"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_I,KEYMAP_PRESSED,kf_SetDroidRangeDefault,	_("Optimum Range"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_O,KEYMAP_PRESSED,kf_SetDroidRangeShort,		_("Short Range"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_P,KEYMAP_PRESSED,kf_SetDroidMovePursue ,		_("Pursue"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_Q,KEYMAP_PRESSED,kf_SetDroidMovePatrol ,		_("Patrol"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_R,KEYMAP_PRESSED,kf_SetDroidGoForRepair ,	_("Return For Repair"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_S,KEYMAP_PRESSED,kf_SetDroidMoveHold ,		_("Hold Position"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_T,KEYMAP_PRESSED,kf_SetDroidGoToTransport,	_("Go to Transport"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_RETURN,KEYMAP_PRESSED,kf_SendTextMessage,			_("Send Text Message"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_U,KEYMAP_PRESSED,kf_SetDroidRangeLong,		_("Long Range"));

	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_H,KEYMAP_PRESSED,kf_AddHelpBlip,		"Drop a beacon");

	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_Z,KEYMAP_PRESSED,kf_SensorDisplayOn,		"Sensor display On");
	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_Z,KEYMAP_RELEASED,kf_SensorDisplayOff,	"Sensor display Off");
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LALT,KEY_S,KEYMAP_PRESSED,kf_ToggleShadows, "Toggles shadows");
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_T, KEYMAP_PRESSED, kf_toggleTrapCursor, "Trap cursor");
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_TAB,KEYMAP_PRESSED,kf_ToggleRadarTerrain,         "Toggle radar terrain");
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LSHIFT,KEY_TAB,KEYMAP_PRESSED,kf_ToggleRadarAllyEnemy,      "Toggle ally-enemy radar view");

	// Some extra non QWERTY mappings but functioning in same way
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_COMMA,KEYMAP_PRESSED,kf_SetDroidRetreatMedium,	   _("Retreat at Medium Damage") );
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_FULLSTOP,KEYMAP_PRESSED,kf_SetDroidRetreatHeavy,	   _("Retreat at Heavy Damage") );
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,KEY_FORWARDSLASH,KEYMAP_PRESSED,kf_SetDroidRetreatNever,_("Do or Die!") );
	//                                **********************************
	//                                **********************************
	//								In game mappings - COMBO (CTRL + LETTER) presses.
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_A,KEYMAP_PRESSED,kf_SelectAllCombatUnits,		_("Select all Combat Units"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_D,KEYMAP_PRESSED,kf_SelectAllDamaged,			_("Select all Heavily Damaged Units"));

	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_F,KEYMAP_PRESSED,kf_SelectAllHalfTracked,		_("Select all Half-tracks"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_H,KEYMAP_PRESSED,kf_SelectAllHovers,			_("Select all Hovers"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_R,KEYMAP_PRESSED,kf_SetDroidRecycle,			_("Return for Recycling"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_S,KEYMAP_PRESSED,kf_SelectAllOnScreenUnits,	_("Select all Units on Screen"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_T,KEYMAP_PRESSED,kf_SelectAllTracked,			_("Select all Tracks"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_U,KEYMAP_PRESSED,kf_SelectAllUnits,			_("Select EVERY unit"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_V,KEYMAP_PRESSED,kf_SelectAllVTOLs,			_("Select all VTOLs"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_W,KEYMAP_PRESSED,kf_SelectAllWheeled,			_("Select all Wheels"));
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_Y,KEYMAP_PRESSED,kf_FrameRate,					"Show frame rate");
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_LCTRL,KEY_Z,KEYMAP_PRESSED,kf_SelectAllSameType,		_("Select all Similar Units"));
	//                                **********************************
	//                                **********************************
	//									SELECT PLAYERS - DEBUG ONLY
 	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_SelectNextFactory,_("Select next Factory"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_SelectNextResearch,_("Select next Research Facility"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_SelectNextPowerStation,_("Select next Power Generator"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_SelectNextCyborgFactory,_("Select next Cyborg Factory"));
	keyAddMapping(KEYMAP_ASSIGNABLE,KEY_IGNORE,(KEY_CODE)KEY_MAXSCAN,KEYMAP_PRESSED,kf_ToggleReopenBuildMenu,_("Toggle reopening the build menu"));

	// NOTE THIS!!!!!!!
	// available: ctrl+l
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_M,KEYMAP_PRESSED,kf_ToggleShowPath,				"Toggle display of droid path");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_E,KEYMAP_PRESSED,kf_ToggleShowGateways,			"Toggle display of gateways");
	keyAddMapping(KEYMAP___HIDE,KEY_LSHIFT,KEY_BACKSPACE,KEYMAP_PRESSED,kf_ToggleDebugMappings,			"TOGGLE Debug Mappings");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_M,KEYMAP_PRESSED,kf_ShowMappings,				"Show all keyboard mappings - use pause!");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_V,KEYMAP_PRESSED,kf_ToggleVisibility,			"Toggle visibility");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_W,KEYMAP_DOWN,kf_RaiseTile,					"Raise tile height");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_A,KEYMAP_DOWN,kf_LowerTile,						"Lower tile height");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_Y,KEYMAP_PRESSED,kf_ToggleDemoMode,				"Toggles on/off DEMO Mode");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_B,KEYMAP_PRESSED,kf_EndMissionOffWorld,			"End Mission");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_J,KEYMAP_PRESSED,kf_ToggleFog,					"Toggles All fog");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_Q,KEYMAP_PRESSED,kf_ToggleWeather,				"Trigger some weather");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_K,KEYMAP_PRESSED,kf_TriFlip,					"Flip terrain triangle");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_K,KEYMAP_PRESSED,kf_MapCheck,					"Realign height of all objects on the map");

	//These ones are necessary for debugging
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_A,KEYMAP_PRESSED,kf_AllAvailable,						"Make all items available");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_K,KEYMAP_PRESSED,kf_KillSelected,						"Kill Selected Unit(s)");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_G,KEYMAP_PRESSED,kf_ToggleGodMode,				"Toggle god Mode Status");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_O,KEYMAP_PRESSED,kf_ChooseOptions,				"Display Options Screen");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_X,KEYMAP_PRESSED,kf_FinishResearch,				"Complete current research");
	keyAddMapping(KEYMAP__DEBUG,KEY_LSHIFT,KEY_W,KEYMAP_PRESSED,kf_ToggleWatchWindow,			"Toggle watch window");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_L,KEYMAP_PRESSED,kf_TraceObject,				"Trace a game object");
	keyAddMapping(KEYMAP__DEBUG,KEY_LSHIFT,KEY_D,KEYMAP_PRESSED,kf_ToggleDrivingMode, 			"Toggle Driving Mode");
	saveKeyMap();	// save out the default key mappings.

//  ------------------------ OLD STUFF - Store here!
	/*
	keyAddMapping(KEYMAP__DEBUG,KEY_LSHIFT,KEY_D,KEYMAP_PRESSED,kf_ToggleDrivingMode, 			"Toggle Driving Mode");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_F6,KEYMAP_DOWN,kf_UpGeoOffset,"Raise the geometric offset");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_F7,KEYMAP_DOWN,kf_DownGeoOffset,"Lower the geometric offset");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_E,KEYMAP_PRESSED,kf_ToggleDroidInfo,"Display droid info whilst tracking");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_I,KEYMAP_PRESSED,kf_ToggleWidgets,"Toggle Widgets");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_J,KEYMAP_PRESSED,kf_ToggleRadarAllign,"Toggles Radar allignment");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_R,KEYMAP_PRESSED,kf_ShowNumObjects,"Show number of Objects");
	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_T,KEYMAP_PRESSED,kf_SendTextMessage,"Send Text Message");
	keyAddMapping(KEYMAP_ALWAYS,KEY_IGNORE,KEY_U,KEYMAP_PRESSED,kf_ToggleBackgroundFog,"Toggle Background Fog");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_V,KEYMAP_PRESSED,kf_BuildInfo,"Build date and time");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_Y,KEYMAP_PRESSED,kf_ToggleDemoMode,"Toggles on/off DEMO Mode");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_Z,KEYMAP_PRESSED,kf_ShowGridInfo,"DBPRINTF map grid coverage");

	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_SCROLLLOCK,KEYMAP_PRESSED,kf_TogglePauseMode,	_("Toggle Pause Mode"));		//not needed, done with KEY_ESC
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_J,KEYMAP_PRESSED,kf_MaxScrollLimits,				"Maximum scroll limits");
	keyAddMapping(KEYMAP__DEBUG,KEY_IGNORE,KEY_N,KEYMAP_PRESSED,kf_GiveTemplateSet,				"Give template set(s) to player 0 ");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_KP_MINUS,KEYMAP_PRESSED,kf_SystemClose,			"System Close (EXIT)");			//not working right now
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_I,KEYMAP_PRESSED,kf_RecalcLighting,				"Recalculate lighting");
	keyAddMapping(KEYMAP__DEBUG,KEY_LCTRL,KEY_N,KEYMAP_PRESSED,kf_NewPlayerPower,				"New game player power");

	// This is not needed, use ctrl-o
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F1,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  0");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F2,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  1");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F3,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  2");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F4,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  3");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F5,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  4");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F6,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  5");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F7,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  6");
	keyAddMapping(KEYMAP__DEBUG,KEY_LALT,KEY_F8,KEYMAP_PRESSED,kf_SelectPlayer,					"Select player  7");
	*/
}

// ----------------------------------------------------------------------------------
/* Adds a new mapping to the list */
//BOOL	keyAddMapping(KEY_CODE metaCode, KEY_CODE subCode, KEY_ACTION action,void *function, char *name)
KEY_MAPPING *keyAddMapping(KEY_STATUS status,KEY_CODE metaCode, KEY_CODE subCode, KEY_ACTION action,
					  void (*pKeyMapFunc)(void), const char *name)
{
KEY_MAPPING	*newMapping;

	/* Get some memory for our binding */
	newMapping = (KEY_MAPPING*)malloc(sizeof(KEY_MAPPING));
	if (newMapping == NULL)
	{
		debug(LOG_FATAL, "keyAddMapping: Out of memory!");
		abort();
		return NULL;
	}

	/* Copy over the name */
	newMapping->pName = strdup(name);
	if (newMapping->pName == NULL)
	{
		debug(LOG_FATAL, "keyAddMapping: Out of memory!");
		abort();
		return NULL;
	}

	/* Fill up our entries, first the ones that activate it */
	newMapping->metaKeyCode	= metaCode;
	newMapping->subKeyCode	= subCode;
	newMapping->status	= status;

	/* When it was last called - needed? */
	newMapping->lastCalled	= gameTime;

	/* And what gets called when it's activated */
	newMapping->function	= pKeyMapFunc;

	/* Is it functional on the key being down or just pressed */
	newMapping->action	= action;

	newMapping->altMetaKeyCode = KEY_IGNORE;

	/* We always request only the left hand one */
	if(metaCode == KEY_LCTRL) {newMapping->altMetaKeyCode = KEY_RCTRL;}
	else if(metaCode == KEY_LALT) {newMapping->altMetaKeyCode = KEY_RALT;}
	else if(metaCode == KEY_LSHIFT) {newMapping->altMetaKeyCode = KEY_RSHIFT;}
	else if(metaCode == KEY_LMETA) {newMapping->altMetaKeyCode = KEY_RMETA;}

	/* Set it to be active */
	newMapping->active = true;
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
			if(psCurr->metaKeyCode == metaCode && psCurr->subKeyCode == subCode)
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
		return(false);
	}

	if(psToRemove == keyMappings && keyMappings->psNext == NULL)
	{
		if (keyMappings->pName)	free(keyMappings->pName);		// ffs
		free(keyMappings);
		keyMappings = NULL;
		numActiveMappings = 0;
		return(true);
	}

	/* See if we can find it */
	for(psPrev = NULL, psCurr = keyMappings;
		psCurr != NULL && psCurr!=psToRemove;
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
		if (psCurr->pName)	free(psCurr->pName);		// only free it if it was allocated in the first place (ffs)
		/* and then for the mapping itself */
		free(psCurr);
		numActiveMappings--;
		return(true);
	}
	return(false);
}

// ----------------------------------------------------------------------------------
/* Just returns how many are active */
UDWORD	getNumMappings( void )
{
	return(numActiveMappings);
}


// ----------------------------------------------------------------------------------
/* Allows _new_ mappings to be made at runtime */
static BOOL checkQwertyKeys( void )
{
	KEY_CODE qKey;
	UDWORD tableEntry;
	BOOL aquired = false;

	/* Are we trying to make a new map marker? */
	if (keyDown(KEY_LALT))
	{
		/* Did we press a key */
		qKey = getQwertyKey();
		if (qKey)
		{
			tableEntry = asciiKeyCodeToTable(qKey);
			/* We're assigning something to the key */
			debug(LOG_NEVER, "Assigning keymapping to tableEntry: %i", tableEntry);
			if (qwertyKeyMappings[tableEntry].psMapping)
			{
				/* Get rid of the old mapping on this key if there was one */
				keyRemoveMappingPt(qwertyKeyMappings[tableEntry].psMapping);
			}
			/* Now add the new one for this location */
			qwertyKeyMappings[tableEntry].psMapping =
				keyAddMapping(KEYMAP_ALWAYS, KEY_LSHIFT, qKey, KEYMAP_PRESSED, kf_JumpToMapMarker, "Jump to new map marker");
			aquired = true;

			/* Store away the position and view angle */
			qwertyKeyMappings[tableEntry].xPos = player.p.x;
			qwertyKeyMappings[tableEntry].yPos = player.p.z;
			qwertyKeyMappings[tableEntry].spin = player.r.y;
		}
	}
	return aquired;
}


// ----------------------------------------------------------------------------------
/* Manages update of all the active function mappings */
void	keyProcessMappings( BOOL bExclude )
{
KEY_MAPPING	*keyToProcess;
BOOL		bMetaKeyDown;
BOOL		bKeyProcessed;
SDWORD		i;

	/* Bomb out if there are none */
	if(!keyMappings || !numActiveMappings || !bKeyProcessing)
	{
		return;
	}

	/* Jump out if we've got a new mapping */
  	(void) checkQwertyKeys();

	/* Check for the meta keys */
	if (keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LALT)
	 || keyDown(KEY_RALT) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT)
	 || keyDown(KEY_LMETA) || keyDown(KEY_RMETA))
	{
		bMetaKeyDown = true;
	}
	else
	{
		bMetaKeyDown = false;
	}

 	/* Run through all our mappings */
 	for(keyToProcess = keyMappings; keyToProcess!=NULL; keyToProcess = keyToProcess->psNext)
	{
		/* We haven't acted upon it */
		bKeyProcessed = false;
		if(!keyToProcess->active)
		{
			/* Get out if it's inactive */
			break;
		}
		/* Skip innappropriate ones when necessary */
		if(bExclude && keyToProcess->status!=KEYMAP_ALWAYS_PROCESS)
		{
			break;
		}

		if(keyToProcess->subKeyCode == KEY_MAXSCAN)
		{
			continue;
		}

		if (keyToProcess->function == NULL)
		{
			continue;
		}

		if(keyToProcess->metaKeyCode==KEY_IGNORE && !bMetaKeyDown &&
			!(keyToProcess->status==KEYMAP__DEBUG && !getDebugMappingStatus()) )
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
					 bKeyProcessed = true;
 				}
 				break;
 			case KEYMAP_DOWN:
 				/* Is the key Down? */
 				if(keyDown(keyToProcess->subKeyCode))
 				{
 					lastSubKey = keyToProcess->subKeyCode;
 					/* Jump to the associated function call */
 					 keyToProcess->function();
					 bKeyProcessed = true;
 				}

 				break;
			case KEYMAP_RELEASED:
 				/* Has the key been released? */
 				if(keyReleased(keyToProcess->subKeyCode))
 				{
 					lastSubKey = keyToProcess->subKeyCode;
 					/* Jump to the associated function call */
 					 keyToProcess->function();
					 bKeyProcessed = true;
 				}
				break;

 			default:
				debug(LOG_FATAL, "Unknown key action (action code %u) while processing keymap.", (unsigned int)keyToProcess->action);
				abort();
 				break;
			}
 		}
		/* Process the combi ones */
 		if( (keyToProcess->metaKeyCode!=KEY_IGNORE && bMetaKeyDown) &&
			!(keyToProcess->status==KEYMAP__DEBUG && !getDebugMappingStatus()))
 		{
 			/* It's a combo keypress - one held down and the other pressed */
 			if (keyDown(keyToProcess->metaKeyCode) && keyPressed(keyToProcess->subKeyCode))
 			{
 				lastMetaKey = keyToProcess->metaKeyCode;
 				lastSubKey = keyToProcess->subKeyCode;
 				keyToProcess->function();
				bKeyProcessed = true;
 			}
			else if (keyToProcess->altMetaKeyCode != KEY_IGNORE)
			{
				if(keyDown(keyToProcess->altMetaKeyCode) && keyPressed(keyToProcess->subKeyCode))
				{
 					lastMetaKey = keyToProcess->metaKeyCode;
 					lastSubKey = keyToProcess->subKeyCode;
 					keyToProcess->function();
					bKeyProcessed = true;
				}
			}
 		}
	}

	/* Script callback - find out what meta key was pressed */
	cbPressedMetaKey = KEY_IGNORE;

	/* getLastMetaKey() can't be used here, have to do manually */
	if(keyDown(KEY_LCTRL))
		cbPressedMetaKey = KEY_LCTRL;
	else if(keyDown(KEY_RCTRL))
		cbPressedMetaKey = KEY_RCTRL;
	else if(keyDown(KEY_LALT))
		cbPressedMetaKey = KEY_LALT;
	else if(keyDown(KEY_RALT))
		cbPressedMetaKey = KEY_RALT;
	else if(keyDown(KEY_LSHIFT))
		cbPressedMetaKey = KEY_LSHIFT;
	else if(keyDown(KEY_RSHIFT))
		cbPressedMetaKey = KEY_RSHIFT;
	else if(keyDown(KEY_LMETA))
		cbPressedMetaKey = KEY_LMETA;
	else if(keyDown(KEY_RMETA))
		cbPressedMetaKey = KEY_RMETA;

	/* Find out what keys were pressed */
	for(i=0; i<KEY_MAXSCAN;i++)
	{
		/* Skip meta keys */
		switch(i)
		{
			case KEY_LCTRL:
			case KEY_RCTRL:
			case KEY_LALT:
			case KEY_RALT:
			case KEY_LSHIFT:
			case KEY_RSHIFT:
			case KEY_LMETA:
			case KEY_RMETA:
				continue;
			break;
		}

		/* Let scripts process this key if it's pressed */
		if(keyPressed((KEY_CODE)i))
		{
				cbPressedKey =  i;
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_KEY_PRESSED);
		}
	}

}


// ----------------------------------------------------------------------------------
/* Sends a particular key mapping to the console */
static void keyShowMapping(KEY_MAPPING *psMapping)
{
char	asciiSub[20],asciiMeta[20];
BOOL	onlySub;

	onlySub = true;
	if(psMapping->metaKeyCode!=KEY_IGNORE)
	{
		keyScanToString(psMapping->metaKeyCode,(char *)&asciiMeta,20);
		onlySub = false;
	}

	keyScanToString(psMapping->subKeyCode,(char *)&asciiSub,20);
	if(onlySub)
	{
   		CONPRINTF(ConsoleString,(ConsoleString,"%s - %s",asciiSub,psMapping->pName));
	}
	else
	{
		CONPRINTF(ConsoleString,(ConsoleString,"%s and %s - %s",asciiMeta,asciiSub,psMapping->pName));
	}
	debug(LOG_INPUT, "Received %s from Console", ConsoleString);
}

// ----------------------------------------------------------------------------------
// this function isn't really module static - should be removed - debug only
void keyShowMappings( void )
{
	KEY_MAPPING *psMapping;

	for (psMapping = keyMappings; psMapping; psMapping = psMapping->psNext)
	{
		keyShowMapping(psMapping);
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
		psMapping->active = false;
	}
}

// ----------------------------------------------------------------------------------
void	keyAllMappingsActive( void )
{
KEY_MAPPING	*psMapping;

	for(psMapping = keyMappings; psMapping; psMapping = psMapping->psNext)
	{
		psMapping->active = true;
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
	if( code >= KEY_Q && code<=KEY_P )
	{
		return code - KEY_Q;  // q is the first of the ascii scan codes
	}
	if( code >= KEY_A && code <=KEY_L )
	{
		return (code - KEY_A) + 10;	// ten keys from q to p
	}
	if( code >= KEY_Z && code<=KEY_M )
	{
		return (code - KEY_Z) + 19;	// 19 keys before, the 10 from q..p and the 9 from a..l
	}
	ASSERT(false, "only pass nonzero key codes from getQwertyKey to this function");
	return 0;
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

	for(psMapping = keyMappings,bFound = false; psMapping && !bFound;
		psMapping = psMapping->psNext)
	{
		/* Find the original */
		if(psMapping->metaKeyCode == origMetaCode && psMapping->subKeyCode == origSubCode)
		{
			/* Not all can be remapped */
			if(psMapping->status != KEYMAP_ALWAYS || psMapping->status == KEYMAP_ALWAYS_PROCESS)
			{
				psMapping->metaKeyCode = newMetaCode;
				psMapping->subKeyCode = newSubCode;
				bFound = true;
			}
		}
	}

	return(bFound);
}

// ----------------------------------------------------------------------------------
KEY_MAPPING	*getKeyMapFromName(char *pName)
{
KEY_MAPPING	*psMapping;
		for(psMapping = keyMappings; psMapping;	psMapping = psMapping->psNext)
		{
			if(strcmp(pName,psMapping->pName) == false)
			{
				return(psMapping);
			}
		}
	return(NULL);
}

// ----------------------------------------------------------------------------------
BOOL	keyReAssignMappingName(char *pName,KEY_CODE newMetaCode, KEY_CODE newSubCode)
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
			return(true);
		}
	}
	return(false);
}
// ----------------------------------------------------------------------------------
