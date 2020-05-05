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
#include "lib/netplay/netplay.h"
#include "lib/gamelib/gtime.h"
#include "keymap.h"
#include "qtscript.h"
#include "console.h"
#include "keybind.h"
#include "display3d.h"
#include "keymap.h"
#include "keyedit.h"

#include <algorithm>

static UDWORD asciiKeyCodeToTable(KEY_CODE code);
static KEY_CODE getQwertyKey();

// ----------------------------------------------------------------------------------
KEY_MAPPING *keyGetMappingFromFunction(void (*function)())
{
	auto mapping = std::find_if(keyMappings.begin(), keyMappings.end(), [function](KEY_MAPPING const &mapping) {
		return mapping.function == function;
	});
	return mapping != keyMappings.end()? &*mapping : nullptr;
}
// ----------------------------------------------------------------------------------
/* Some stuff allowing the user to add key mappings themselves */

#define	NUM_QWERTY_KEYS	26
struct KEYMAP_MARKER
{
	KEY_MAPPING	*psMapping;
	UDWORD	xPos, yPos;
	SDWORD	spin;
};
static	KEYMAP_MARKER	qwertyKeyMappings[NUM_QWERTY_KEYS];


static bool bDoingDebugMappings = false;
static bool bWantDebugMappings[MAX_PLAYERS] = {false};
// ----------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------
/* The linked list of present key mappings */
std::list<KEY_MAPPING> keyMappings;

/* Last meta and sub key that were recorded */
static KEY_CODE	lastMetaKey, lastSubKey;

// ----------------------------------------------------------------------------------
// Adding a mapped function ? add a save pointer! Thank AlexL.

static KeyMapSaveEntry const keyMapSaveTable[] =
{
	{kf_ChooseManufacture, "ChooseManufacture"},
	{kf_ChooseResearch, "ChooseResearch"},
	{kf_ChooseBuild, "ChooseBuild"},
	{kf_ChooseDesign, "ChooseDesign"},
	{kf_ChooseIntelligence, "ChooseIntelligence"},
	{kf_ChooseCommand, "ChooseCommand"},
	{kf_ToggleRadar, "ToggleRadar"},
	{kf_ToggleConsole, "ToggleConsole"},
	{kf_ToggleEnergyBars, "ToggleEnergyBars"},
	{kf_ToggleTeamChat, "ToggleTeamChat"},
	{kf_ScreenDump, "ScreenDump"} ,
	{kf_MoveToLastMessagePos, "MoveToLastMessagePos"},
	{kf_AssignGrouping_1, "AssignGrouping_1"},
	{kf_AssignGrouping_2, "AssignGrouping_2"},
	{kf_AssignGrouping_3, "AssignGrouping_3"},
	{kf_AssignGrouping_4, "AssignGrouping_4"},
	{kf_AssignGrouping_5, "AssignGrouping_5"},
	{kf_AssignGrouping_6, "AssignGrouping_6"},
	{kf_AssignGrouping_7, "AssignGrouping_7"},
	{kf_AssignGrouping_8, "AssignGrouping_8"},
	{kf_AssignGrouping_9, "AssignGrouping_9"},
	{kf_SelectGrouping_1, "SelectGrouping_1"},
	{kf_SelectGrouping_2, "SelectGrouping_2"},
	{kf_SelectGrouping_3, "SelectGrouping_3"},
	{kf_SelectGrouping_4, "SelectGrouping_4"},
	{kf_SelectGrouping_5, "SelectGrouping_5"},
	{kf_SelectGrouping_6, "SelectGrouping_6"},
	{kf_SelectGrouping_7, "SelectGrouping_7"},
	{kf_SelectGrouping_8, "SelectGrouping_8"},
	{kf_SelectGrouping_9, "SelectGrouping_9"},
	{kf_addMultiMenu, "addMultiMenu"},
	{kf_multiAudioStart, "multiAudioStart"},
	{kf_multiAudioStop, "multiAudioStop"},
	{kf_CameraUp, "CameraUp"},
	{kf_CameraDown, "CameraDown"},
	{kf_CameraLeft, "CameraLeft"},
	{kf_CameraRight, "CameraRight"},
	{kf_SeekNorth, "SeekNorth"},
	{kf_ToggleCamera, "ToggleCamera"},
	{kf_addInGameOptions, "addInGameOptions"},
	{kf_RadarZoomOut, "RadarZoomOut"},
	{kf_RadarZoomIn, "RadarZoomIn"},
	{kf_ZoomOut, "ZoomOut"},
	{kf_ZoomIn, "ZoomIn"},
	{kf_PitchForward, "PitchForward"},
	{kf_RotateLeft, "RotateLeft"},
	{kf_ResetPitch, "ResetPitch"},
	{kf_RotateRight, "RotateRight"},
	{kf_PitchBack, "PitchBack"},
	{kf_RightOrderMenu, "RightOrderMenu"},
	{kf_JumpToResourceExtractor, "JumpToResourceExtractor"},
	{kf_JumpToRepairUnits, "JumpToRepairUnits"},
	{kf_JumpToConstructorUnits, "JumpToConstructorUnits"},
	{kf_JumpToSensorUnits, "JumpToSensorUnits"},
	{kf_JumpToCommandUnits, "JumpToCommandUnits"},
	{kf_ToggleOverlays, "ToggleOverlays"},
	{kf_CentreOnBase, "CentreOnBase"},
	{kf_SetDroidAttackCease, "SetDroidAttackCease"} ,
	{kf_JumpToUnassignedUnits, "JumpToUnassignedUnits"} ,
	{kf_SetDroidAttackReturn, "SetDroidAttackReturn"} ,
	{kf_SetDroidAttackAtWill, "SetDroidAttackAtWill"} ,
	{kf_SetDroidReturnToBase, "SetDroidReturnToBase"} ,
	{kf_ToggleFormationSpeedLimiting, "ToggleFormationSpeedLimiting"},
	{kf_SetDroidMovePatrol, "SetDroidMovePatrol"} ,
	{kf_SetDroidGoForRepair, "SetDroidGoForRepair"} ,
	{kf_SetDroidOrderHold, "SetDroidOrderHold"},
	{kf_SendGlobalMessage, "SendGlobalMessage"},
	{kf_SendTeamMessage, "SendTeamMessage"},
	{kf_ScatterDroids, "ScatterDroids"},
	{kf_SetDroidRetreatMedium, "SetDroidRetreatMedium"},
	{kf_SetDroidRetreatHeavy, "SetDroidRetreatHeavy"},
	{kf_SetDroidRetreatNever, "SetDroidRetreatNever"},
	{kf_SelectAllCombatUnits, "SelectAllCombatUnits"},
	{kf_SelectAllDamaged, "SelectAllDamaged"},
	{kf_SelectAllHalfTracked, "SelectAllHalfTracked"},
	{kf_SelectAllHovers, "SelectAllHovers"},
	{kf_SetDroidRecycle, "SetDroidRecycle"},
	{kf_SelectAllOnScreenUnits, "SelectAllOnScreenUnits"},
	{kf_SelectAllTracked, "SelectAllTracked"},
	{kf_SelectAllUnits, "SelectAllUnits"},
	{kf_SelectAllVTOLs, "SelectAllVTOLs"},
	{kf_SelectAllWheeled, "SelectAllWheeled"},
	{kf_FrameRate, "FrameRate"},
	{kf_SelectAllSameType, "SelectAllSameType"},
	{kf_SelectNextFactory, "SelectNextFactory"},
	{kf_SelectNextResearch, "SelectNextResearch"},
	{kf_SelectNextPowerStation, "SelectNextPowerStation"},
	{kf_SelectNextCyborgFactory, "SelectNextCyborgFactory"},
	{kf_ToggleConsoleDrop, "ToggleConsoleDrop"},
	{kf_SelectCommander_1, "SelectCommander_1"},
	{kf_SelectCommander_2, "SelectCommander_2"},
	{kf_SelectCommander_3, "SelectCommander_3"},
	{kf_SelectCommander_4, "SelectCommander_4"},
	{kf_SelectCommander_5, "SelectCommander_5"},
	{kf_SelectCommander_6, "SelectCommander_6"},
	{kf_SelectCommander_7, "SelectCommander_7"},
	{kf_SelectCommander_8, "SelectCommander_8"},
	{kf_SelectCommander_9, "SelectCommander_9"},
	{kf_FaceNorth, "FaceNorth"},
	{kf_FaceSouth, "FaceSouth"},
	{kf_FaceWest, "FaceWest"},
	{kf_FaceEast, "FaceEast"},
	{kf_SpeedUp, "SpeedUp"},
	{kf_SlowDown, "SlowDown"},
	{kf_NormalSpeed, "NormalSpeed"},
	{kf_ToggleRadarJump, "ToggleRadarJump"},
	{kf_MovePause, "MovePause"},
	{kf_ToggleRadarTerrain, "ToggleRadarTerrain"},          //radar terrain on/off
	{kf_ToggleRadarAllyEnemy, "ToggleRadarAllyEnemy"},      //enemy/ally radar color toggle
	{kf_ToggleSensorDisplay, "ToggleSensorDisplay"},        //  Was commented out below. moved also!.  Re-enabled --Q 5/10/05
	{kf_AddHelpBlip, "AddHelpBlip"},                        //Add a beacon
	{kf_AllAvailable, "AllAvailable"},
	{kf_ToggleDebugMappings, "ToggleDebugMappings"},
	{kf_TogglePauseMode, "TogglePauseMode"},
	{kf_MaxScrollLimits, "MaxScrollLimits"},
	{kf_DebugDroidInfo, "DebugDroidInfo"},
	{kf_RecalcLighting, "RecalcLighting"},
	{kf_ToggleFog, "ToggleFog"},
	{kf_ChooseOptions, "ChooseOptions"},
	{kf_TogglePower, "TogglePower"},
	{kf_ToggleWeather, "ToggleWeather"},
	{kf_SelectPlayer, "SelectPlayer"},
	{kf_AssignGrouping_0, "AssignGrouping_0"},
	{kf_SelectGrouping_0, "SelectGrouping_0"},
	{kf_SelectCommander_0, "SelectCommander_0"},
	{kf_TraceObject, "TraceObject"},
	{kf_KillSelected, "KillSelected"},
	{kf_ShowMappings, "ShowMappings"},
	{kf_ToggleVisibility, "ToggleVisibility"},
	{kf_FinishResearch, "FinishResearch"},
	{kf_LowerTile, "LowerTile"},
	{kf_ToggleGodMode, "ToggleGodMode"},
	{kf_SystemClose, "SystemClose"},
	{kf_ToggleShadows, "ToggleShadows"},
	{kf_RaiseTile, "RaiseTile"},
	{kf_TriFlip, "TriFlip"},
	{kf_RevealMapAtPos, "RevealMapAtPos"},
	{kf_ToggleShowGateways, "ToggleShowGateways"},
	{kf_ToggleShowPath, "ToggleShowPath"},
	{kf_PerformanceSample, "PerformanceSample"},
	{kf_SetDroidGoToTransport, "SetDroidGoToTransport"},
	{kf_toggleTrapCursor, "toggleTrapCursor"},
	{kf_SelectAllCyborgs, "SelectAllCyborgs"},
	{kf_SelectAllCombatCyborgs, "SelectAllCombatCyborgs"},
	{kf_SelectAllEngineers, "SelectAllEngineers"},
	{kf_SelectAllLandCombatUnits, "SelectAllLandCombatUnits"},
	{kf_SelectAllMechanics, "SelectAllMechanics"},
	{kf_SelectAllTransporters, "SelectAllTransporters"},
	{kf_SelectAllRepairTanks, "SelectAllRepairTanks"},
	{kf_SelectAllSensorUnits, "SelectAllSensorUnits"},
	{kf_SelectAllTrucks, "SelectAllTrucks"},
	{kf_SetDroidOrderStop, "SetDroidOrderStop"},
	{kf_SelectAllArmedVTOLs, "SelectAllArmedVTOLs"},
	{kf_SetDroidMovePursue, "SetDroidMovePursue"},
	{kf_SetDroidMoveGuard, "SetDroidMoveGuard"},
	{kf_SetDroidRangeOptimum, "SetDroidRangeOptimum"},
	{kf_SetDroidRangeShort, "SetDroidRangeShort"},
	{kf_SetDroidRangeLong, "SetDroidRangeLong"},
	{kf_QuickSave, "QuickSave"},
	{kf_QuickLoad, "QuickLoad"},
};

KeyMapSaveEntry const *keymapEntryByFunction(void (*function)())
{
	for (auto &entry : keyMapSaveTable) {
		if (entry.function == function) {
			return &entry;
		}
	}
	return nullptr;
}

KeyMapSaveEntry const *keymapEntryByName(std::string const &name)
{
	for (auto &entry : keyMapSaveTable) {
		if (entry.name == name) {
			return &entry;
		}
	}
	return nullptr;
}

// ----------------------------------------------------------------------------------
/*
	Here is where we assign functions to keys and to combinations of keys.
	these will be read in from a .cfg file customisable by the player from
	an in-game menu
*/
void keyInitMappings(bool bForceDefaults)
{
	keyMappings.clear();
	for (unsigned n = 0; n < MAX_PLAYERS; ++n)
	{
		processDebugMappings(n, false);
	}

	for (unsigned i = 0; i < NUM_QWERTY_KEYS; i++)
	{
		qwertyKeyMappings[i].psMapping = nullptr;
	}

	// load the mappings.
	if (!bForceDefaults && loadKeyMap() == true)
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
	keyAddMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F1,  KEYMAP_PRESSED, kf_ChooseManufacture,            N_("Manufacture"));
	keyAddMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F2,  KEYMAP_PRESSED, kf_ChooseResearch,               N_("Research"));
	keyAddMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F3,  KEYMAP_PRESSED, kf_ChooseBuild,                  N_("Build"));
	keyAddMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F4,  KEYMAP_PRESSED, kf_ChooseDesign,                 N_("Design"));
	keyAddMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F5,  KEYMAP_PRESSED, kf_ChooseIntelligence,           N_("Intelligence Display"));
	keyAddMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F6,  KEYMAP_PRESSED, kf_ChooseCommand,                N_("Commanders"));
	keyAddMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F7,  KEYMAP_PRESSED, kf_QuickSave,                    N_("QuickSave"));
	keyAddMapping(KEYMAP_ASSIGNABLE,     KEY_LSHIFT, KEY_F7,  KEYMAP_PRESSED, kf_ToggleRadar,                  N_("Toggle Radar"));
	keyAddMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F8,  KEYMAP_PRESSED, kf_QuickLoad,                    N_("QuickLoad"));
	keyAddMapping(KEYMAP_ASSIGNABLE,     KEY_LSHIFT, KEY_F8,  KEYMAP_PRESSED, kf_ToggleConsole,                N_("Toggle Console Display"));
	keyAddMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F9,  KEYMAP_PRESSED, kf_ToggleEnergyBars,             N_("Toggle Damage Bars On/Off"));
	keyAddMapping(KEYMAP_ALWAYS,         KEY_IGNORE, KEY_F10, KEYMAP_PRESSED, kf_ScreenDump,                   N_("Take Screen Shot"));
	keyAddMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F11, KEYMAP_PRESSED, kf_ToggleFormationSpeedLimiting, N_("Toggle Formation Speed Limiting"));
	keyAddMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F12, KEYMAP_PRESSED, kf_MoveToLastMessagePos,         N_("View Location of Previous Message"));
	keyAddMapping(KEYMAP_ASSIGNABLE,     KEY_LSHIFT, KEY_F12, KEYMAP_PRESSED, kf_ToggleSensorDisplay,          N_("Toggle Sensor display")); //Which key should we use? --Re enabled see below! -Q 5-10-05
	//                                **********************************
	//                                **********************************
	//										ASSIGN GROUPS
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_0, KEYMAP_PRESSED, kf_AssignGrouping_0, N_("Assign Group 0"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_1, KEYMAP_PRESSED, kf_AssignGrouping_1, N_("Assign Group 1"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_2, KEYMAP_PRESSED, kf_AssignGrouping_2, N_("Assign Group 2"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_3, KEYMAP_PRESSED, kf_AssignGrouping_3, N_("Assign Group 3"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_4, KEYMAP_PRESSED, kf_AssignGrouping_4, N_("Assign Group 4"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_5, KEYMAP_PRESSED, kf_AssignGrouping_5, N_("Assign Group 5"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_6, KEYMAP_PRESSED, kf_AssignGrouping_6, N_("Assign Group 6"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_7, KEYMAP_PRESSED, kf_AssignGrouping_7, N_("Assign Group 7"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_8, KEYMAP_PRESSED, kf_AssignGrouping_8, N_("Assign Group 8"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_9, KEYMAP_PRESSED, kf_AssignGrouping_9, N_("Assign Group 9"));
	//                                **********************************
	//                                **********************************
	//	SELECT GROUPS - Will jump to the group as well as select if group is ALREADY selected
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_0, KEYMAP_PRESSED, kf_SelectGrouping_0, N_("Select Group 0"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_1, KEYMAP_PRESSED, kf_SelectGrouping_1, N_("Select Group 1"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_2, KEYMAP_PRESSED, kf_SelectGrouping_2, N_("Select Group 2"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_3, KEYMAP_PRESSED, kf_SelectGrouping_3, N_("Select Group 3"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_4, KEYMAP_PRESSED, kf_SelectGrouping_4, N_("Select Group 4"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_5, KEYMAP_PRESSED, kf_SelectGrouping_5, N_("Select Group 5"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_6, KEYMAP_PRESSED, kf_SelectGrouping_6, N_("Select Group 6"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_7, KEYMAP_PRESSED, kf_SelectGrouping_7, N_("Select Group 7"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_8, KEYMAP_PRESSED, kf_SelectGrouping_8, N_("Select Group 8"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_9, KEYMAP_PRESSED, kf_SelectGrouping_9, N_("Select Group 9"));
	//                                **********************************
	//                                **********************************
	//	SELECT COMMANDER - Will jump to the group as well as select if group is ALREADY selected
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_0, KEYMAP_PRESSED, kf_SelectCommander_0, N_("Select Commander 0"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_1, KEYMAP_PRESSED, kf_SelectCommander_1, N_("Select Commander 1"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_2, KEYMAP_PRESSED, kf_SelectCommander_2, N_("Select Commander 2"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_3, KEYMAP_PRESSED, kf_SelectCommander_3, N_("Select Commander 3"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_4, KEYMAP_PRESSED, kf_SelectCommander_4, N_("Select Commander 4"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_5, KEYMAP_PRESSED, kf_SelectCommander_5, N_("Select Commander 5"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_6, KEYMAP_PRESSED, kf_SelectCommander_6, N_("Select Commander 6"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_7, KEYMAP_PRESSED, kf_SelectCommander_7, N_("Select Commander 7"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_8, KEYMAP_PRESSED, kf_SelectCommander_8, N_("Select Commander 8"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_9, KEYMAP_PRESSED, kf_SelectCommander_9, N_("Select Commander 9"));
	//                                **********************************
	//                                **********************************
	//	MULTIPLAYER
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KPENTER, KEYMAP_PRESSED, kf_addMultiMenu, N_("Multiplayer Options / Alliance dialog"));
	//
	//	GAME CONTROLS - Moving around, zooming in, rotating etc
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_UPARROW,           KEYMAP_DOWN,    kf_CameraUp,                N_("Move Camera Up"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_DOWNARROW,         KEYMAP_DOWN,    kf_CameraDown,              N_("Move Camera Down"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_RIGHTARROW,        KEYMAP_DOWN,    kf_CameraRight,             N_("Move Camera Right"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_LEFTARROW,         KEYMAP_DOWN,    kf_CameraLeft,              N_("Move Camera Left"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_BACKSPACE,         KEYMAP_PRESSED, kf_SeekNorth,               N_("Snap View to North"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_SPACE,             KEYMAP_PRESSED, kf_ToggleCamera,            N_("Toggle Tracking Camera"));
	keyAddMapping(KEYMAP_ALWAYS,     KEY_IGNORE, KEY_ESC,               KEYMAP_PRESSED, kf_addInGameOptions,        N_("Display In-Game Options"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_MINUS,             KEYMAP_PRESSED, kf_RadarZoomOut,            N_("Zoom Radar Out"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_EQUALS,            KEYMAP_PRESSED, kf_RadarZoomIn,             N_("Zoom Radar In"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_MINUS,          KEYMAP_DOWN,    kf_ZoomOut,                 N_("Zoom In"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_PLUS,           KEYMAP_DOWN,    kf_ZoomIn,                  N_("Zoom Out"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_2,              KEYMAP_DOWN,    kf_PitchForward,            N_("Pitch Forward"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_4,              KEYMAP_DOWN,    kf_RotateLeft,              N_("Rotate Left"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_5,              KEYMAP_DOWN,    kf_ResetPitch,              N_("Reset Pitch"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_6,              KEYMAP_DOWN,    kf_RotateRight,             N_("Rotate Right"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_8,              KEYMAP_DOWN,    kf_PitchBack,               N_("Pitch Back"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_0,              KEYMAP_PRESSED, kf_RightOrderMenu,          N_("Orders Menu"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_MINUS,             KEYMAP_PRESSED, kf_SlowDown,                N_("Decrease Game Speed"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_EQUALS,            KEYMAP_PRESSED, kf_SpeedUp,                 N_("Increase Game Speed"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_BACKSPACE,         KEYMAP_PRESSED, kf_NormalSpeed,             N_("Reset Game Speed"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_UPARROW,           KEYMAP_PRESSED, kf_FaceNorth,               N_("View North"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_DOWNARROW,         KEYMAP_PRESSED, kf_FaceSouth,               N_("View South"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_LEFTARROW,         KEYMAP_PRESSED, kf_FaceEast,                N_("View East"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_RIGHTARROW,        KEYMAP_PRESSED, kf_FaceWest,                N_("View West"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_STAR,           KEYMAP_PRESSED, kf_JumpToResourceExtractor, N_("View next Oil Derrick"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpToRepairUnits,       N_("View next Repair Unit"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpToConstructorUnits,  N_("View next Truck"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpToSensorUnits,       N_("View next Sensor Unit"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpToCommandUnits,      N_("View next Commander"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_TAB,               KEYMAP_PRESSED, kf_ToggleOverlays,          N_("Toggle Overlays"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_BACKQUOTE,         KEYMAP_PRESSED, kf_ToggleConsoleDrop,       N_("Toggle Console History "));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_BACKQUOTE,         KEYMAP_PRESSED, kf_ToggleTeamChat,          N_("Toggle Team Chat History"));
	//                                **********************************
	// IN GAME MAPPINGS - Single key presses - ALL __DEBUG keymappings will be removed for master
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_B,      KEYMAP_PRESSED, kf_CentreOnBase,          N_("Center View on HQ"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_C,      KEYMAP_PRESSED, kf_SetDroidAttackCease,   N_("Hold Fire"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_D,      KEYMAP_PRESSED, kf_JumpToUnassignedUnits, N_("View Unassigned Units"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_E,      KEYMAP_PRESSED, kf_SetDroidAttackReturn,  N_("Return Fire"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_F,      KEYMAP_PRESSED, kf_SetDroidAttackAtWill,  N_("Fire at Will"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_G,      KEYMAP_PRESSED, kf_SetDroidMoveGuard,     N_("Guard Position"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_H,      KEYMAP_PRESSED, kf_SetDroidReturnToBase,  N_("Return to HQ"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_H,      KEYMAP_PRESSED, kf_SetDroidOrderHold,     N_("Hold Position"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_I,      KEYMAP_PRESSED, kf_SetDroidRangeOptimum,  N_("Optimum Range"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_O,      KEYMAP_PRESSED, kf_SetDroidRangeShort,    N_("Short Range"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_P,      KEYMAP_PRESSED, kf_SetDroidMovePursue,    N_("Pursue"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_Q,      KEYMAP_PRESSED, kf_SetDroidMovePatrol,    N_("Patrol"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_R,      KEYMAP_PRESSED, kf_SetDroidGoForRepair,   N_("Return For Repair"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_S,      KEYMAP_PRESSED, kf_SetDroidOrderStop,     N_("Stop Droid"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_T,      KEYMAP_PRESSED, kf_SetDroidGoToTransport, N_("Go to Transport"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_U,      KEYMAP_PRESSED, kf_SetDroidRangeLong,     N_("Long Range"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_RETURN, KEYMAP_PRESSED, kf_SendGlobalMessage,     N_("Send Global Text Message"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_RETURN, KEYMAP_PRESSED, kf_SendTeamMessage,       N_("Send Team Text Message"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LALT,   KEY_H,      KEYMAP_PRESSED, kf_AddHelpBlip,           N_("Drop a beacon"));

	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LALT,   KEY_S,   KEYMAP_PRESSED,  kf_ToggleShadows,        N_("Toggles shadows"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LALT,   KEY_T,   KEYMAP_PRESSED,  kf_toggleTrapCursor,     N_("Trap cursor"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_TAB, KEYMAP_PRESSED,  kf_ToggleRadarTerrain,   N_("Toggle radar terrain"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_TAB, KEYMAP_PRESSED,  kf_ToggleRadarAllyEnemy, N_("Toggle ally-enemy radar view"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_M,   KEYMAP_PRESSED,  kf_ShowMappings,         N_("Show all keyboard mappings"));

	// Some extra non QWERTY mappings but functioning in same way
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_COMMA,        KEYMAP_PRESSED, kf_SetDroidRetreatMedium, N_("Retreat at Medium Damage"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_FULLSTOP,     KEYMAP_PRESSED, kf_SetDroidRetreatHeavy,  N_("Retreat at Heavy Damage"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_FORWARDSLASH, KEYMAP_PRESSED, kf_SetDroidRetreatNever,  N_("Do or Die!"));
	//                                **********************************
	//                                **********************************
	//								In game mappings - COMBO (CTRL + LETTER) presses.
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_A, KEYMAP_PRESSED, kf_SelectAllCombatUnits,   N_("Select all Combat Units"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_C, KEYMAP_PRESSED, kf_SelectAllCyborgs,       N_("Select all Cyborgs"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_D, KEYMAP_PRESSED, kf_SelectAllDamaged,       N_("Select all Heavily Damaged Units"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_F, KEYMAP_PRESSED, kf_SelectAllHalfTracked,   N_("Select all Half-tracks"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_H, KEYMAP_PRESSED, kf_SelectAllHovers,        N_("Select all Hovers"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_R, KEYMAP_PRESSED, kf_SetDroidRecycle,        N_("Return for Recycling"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_S, KEYMAP_PRESSED, kf_SelectAllOnScreenUnits, N_("Select all Units on Screen"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_T, KEYMAP_PRESSED, kf_SelectAllTracked,       N_("Select all Tracks"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_U, KEYMAP_PRESSED, kf_SelectAllUnits,         N_("Select EVERY unit"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_V, KEYMAP_PRESSED, kf_SelectAllVTOLs,         N_("Select all VTOLs"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_V, KEYMAP_PRESSED, kf_SelectAllArmedVTOLs,   N_("Select all fully-armed VTOLs"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_W, KEYMAP_PRESSED, kf_SelectAllWheeled,       N_("Select all Wheels"));
	keyAddMapping(KEYMAP__DEBUG,     KEY_LCTRL, KEY_Y, KEYMAP_PRESSED, kf_FrameRate,              N_("Show frame rate"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_Z, KEYMAP_PRESSED, kf_SelectAllSameType,      N_("Select all units with the same components"));
	//                                **********************************
	//                                **********************************
	//                                                              In game mappings - COMBO (SHIFT + LETTER) presses.
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_C, KEYMAP_PRESSED, kf_SelectAllCombatCyborgs,   N_("Select all Combat Cyborgs"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_E, KEYMAP_PRESSED, kf_SelectAllEngineers,       N_("Select all Engineers"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_G, KEYMAP_PRESSED, kf_SelectAllLandCombatUnits, N_("Select all Land Combat Units"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_M, KEYMAP_PRESSED, kf_SelectAllMechanics,       N_("Select all Mechanics"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_P, KEYMAP_PRESSED, kf_SelectAllTransporters,    N_("Select all Transporters"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_R, KEYMAP_PRESSED, kf_SelectAllRepairTanks,     N_("Select all Repair Tanks"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_S, KEYMAP_PRESSED, kf_SelectAllSensorUnits,     N_("Select all Sensor Units"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_T, KEYMAP_PRESSED, kf_SelectAllTrucks,          N_("Select all Trucks"));
	//                                **********************************
	//                                **********************************
	//									SELECT PLAYERS - DEBUG ONLY
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_SelectNextFactory,       N_("Select next Factory"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_SelectNextResearch,      N_("Select next Research Facility"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_SelectNextPowerStation,  N_("Select next Power Generator"));
	keyAddMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_SelectNextCyborgFactory, N_("Select next Cyborg Factory"));

	// Debug options
	keyAddMapping(KEYMAP___HIDE, KEY_LSHIFT, KEY_BACKSPACE, KEYMAP_PRESSED, kf_ToggleDebugMappings, N_("Toggle Debug Mappings"));
	keyAddMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_M,         KEYMAP_PRESSED, kf_ToggleShowPath,      N_("Toggle display of droid path"));
	keyAddMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_E,         KEYMAP_PRESSED, kf_ToggleShowGateways,  N_("Toggle display of gateways"));
	keyAddMapping(KEYMAP__DEBUG, KEY_IGNORE, KEY_V,         KEYMAP_PRESSED, kf_ToggleVisibility,    N_("Toggle visibility"));
	keyAddMapping(KEYMAP__DEBUG, KEY_IGNORE, KEY_W,         KEYMAP_DOWN,    kf_RaiseTile,           N_("Raise tile height"));
	keyAddMapping(KEYMAP__DEBUG, KEY_IGNORE, KEY_A,         KEYMAP_DOWN,    kf_LowerTile,           N_("Lower tile height"));
	keyAddMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_J,         KEYMAP_PRESSED, kf_ToggleFog,           N_("Toggles All fog"));
	keyAddMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_Q,         KEYMAP_PRESSED, kf_ToggleWeather,       N_("Trigger some weather"));
	keyAddMapping(KEYMAP__DEBUG, KEY_IGNORE, KEY_K,         KEYMAP_PRESSED, kf_TriFlip,             N_("Flip terrain triangle"));
	keyAddMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_K,         KEYMAP_PRESSED, kf_PerformanceSample,   N_("Make a performance measurement sample"));

	//These ones are necessary for debugging
	keyAddMapping(KEYMAP__DEBUG, KEY_LALT,   KEY_A, KEYMAP_PRESSED, kf_AllAvailable,      N_("Make all items available"));
	keyAddMapping(KEYMAP__DEBUG, KEY_LALT,   KEY_K, KEYMAP_PRESSED, kf_KillSelected,      N_("Kill Selected Unit(s)"));
	keyAddMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_G, KEYMAP_PRESSED, kf_ToggleGodMode,     N_("Toggle god Mode Status"));
	keyAddMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_O, KEYMAP_PRESSED, kf_ChooseOptions,     N_("Display Options Screen"));
	keyAddMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_X, KEYMAP_PRESSED, kf_FinishResearch,    N_("Complete current research"));
	keyAddMapping(KEYMAP__DEBUG, KEY_LSHIFT, KEY_W, KEYMAP_PRESSED, kf_RevealMapAtPos,    N_("Reveal map at mouse position"));
	keyAddMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_L, KEYMAP_PRESSED, kf_TraceObject,       N_("Trace a game object"));
	saveKeyMap();	// save out the default key mappings.
}

// ----------------------------------------------------------------------------------
/* Adds a new mapping to the list */
//bool	keyAddMapping(KEY_CODE metaCode, KEY_CODE subCode, KEY_ACTION action,void *function, char *name)
KEY_MAPPING *keyAddMapping(KEY_STATUS status, KEY_CODE metaCode, KEY_CODE subCode, KEY_ACTION action, void (*pKeyMapFunc)(), const char *name)
{
	/* Get some memory for our binding */
	keyMappings.emplace_back();
	KEY_MAPPING *newMapping = &keyMappings.back();

	/* Copy over the name */
	newMapping->name = name;

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
	if (metaCode == KEY_LCTRL)
	{
		newMapping->altMetaKeyCode = KEY_RCTRL;
	}
	else if (metaCode == KEY_LALT)
	{
		newMapping->altMetaKeyCode = KEY_RALT;
	}
	else if (metaCode == KEY_LSHIFT)
	{
		newMapping->altMetaKeyCode = KEY_RSHIFT;
	}
	else if (metaCode == KEY_LMETA)
	{
		newMapping->altMetaKeyCode = KEY_RMETA;
	}

	return newMapping;
}

// ----------------------------------------------------------------------------------
/* Returns a pointer to a mapping if it exists - NULL otherwise */
KEY_MAPPING *keyFindMapping(KEY_CODE metaCode, KEY_CODE subCode)
{
	auto mapping = std::find_if(keyMappings.begin(), keyMappings.end(), [metaCode, subCode](KEY_MAPPING const &mapping) {
		return mapping.metaKeyCode == metaCode && mapping.subKeyCode == subCode;
	});
	return mapping != keyMappings.end()? &*mapping : nullptr;
}

// ----------------------------------------------------------------------------------
/* Removes a mapping specified by a pointer */
static bool keyRemoveMappingPt(KEY_MAPPING *psToRemove)
{
	auto mapping = std::find_if(keyMappings.begin(), keyMappings.end(), [psToRemove](KEY_MAPPING const &mapping) {
		return &mapping == psToRemove;
	});
	if (mapping != keyMappings.end())
	{
		keyMappings.erase(mapping);
		return true;
	}
	return false;
}


// ----------------------------------------------------------------------------------
/* Allows _new_ mappings to be made at runtime */
static bool checkQwertyKeys()
{
	KEY_CODE qKey;
	UDWORD tableEntry;
	bool aquired = false;

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
void	keyProcessMappings(bool bExclude)
{
	/* Bomb out if there are none */
	if (keyMappings.empty())
	{
		return;
	}

	/* Jump out if we've got a new mapping */
	(void) checkQwertyKeys();

	/* Check for the meta keys */
	bool bMetaKeyDown = keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LALT)
	    || keyDown(KEY_RALT) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT)
	    || keyDown(KEY_LMETA) || keyDown(KEY_RMETA);

	/* Run through all our mappings */
	for (auto keyToProcess = keyMappings.begin(); keyToProcess != keyMappings.end(); ++keyToProcess)
	{
		/* Skip inappropriate ones when necessary */
		if (bExclude && keyToProcess->status != KEYMAP_ALWAYS_PROCESS)
		{
			break;
		}

		if (keyToProcess->subKeyCode == (KEY_CODE)KEY_MAXSCAN)
		{
			continue;
		}

		if (keyToProcess->function == nullptr)
		{
			continue;
		}

		if (keyToProcess->metaKeyCode == KEY_IGNORE && !bMetaKeyDown &&
		    !(keyToProcess->status == KEYMAP__DEBUG && !getDebugMappingStatus()))
		{
			switch (keyToProcess->action)
			{
			case KEYMAP_PRESSED:
				/* Were the right keys pressed? */
				if (keyPressed(keyToProcess->subKeyCode))
				{
					lastSubKey = keyToProcess->subKeyCode;
					/* Jump to the associated function call */
					keyToProcess->function();
				}
				break;
			case KEYMAP_DOWN:
				/* Is the key Down? */
				if (keyDown(keyToProcess->subKeyCode))
				{
					lastSubKey = keyToProcess->subKeyCode;
					/* Jump to the associated function call */
					keyToProcess->function();
				}

				break;
			case KEYMAP_RELEASED:
				/* Has the key been released? */
				if (keyReleased(keyToProcess->subKeyCode))
				{
					lastSubKey = keyToProcess->subKeyCode;
					/* Jump to the associated function call */
					keyToProcess->function();
				}
				break;

			default:
				debug(LOG_FATAL, "Unknown key action (action code %u) while processing keymap.", (unsigned int)keyToProcess->action);
				abort();
				break;
			}
		}
		/* Process the combi ones */
		if ((keyToProcess->metaKeyCode != KEY_IGNORE && bMetaKeyDown) &&
		    !(keyToProcess->status == KEYMAP__DEBUG && !getDebugMappingStatus()))
		{
			/* It's a combo keypress - one held down and the other pressed */
			if (keyDown(keyToProcess->metaKeyCode) && keyPressed(keyToProcess->subKeyCode))
			{
				lastMetaKey = keyToProcess->metaKeyCode;
				lastSubKey = keyToProcess->subKeyCode;
				keyToProcess->function();
			}
			else if (keyToProcess->altMetaKeyCode != KEY_IGNORE)
			{
				if (keyDown(keyToProcess->altMetaKeyCode) && keyPressed(keyToProcess->subKeyCode))
				{
					lastMetaKey = keyToProcess->metaKeyCode;
					lastSubKey = keyToProcess->subKeyCode;
					keyToProcess->function();
				}
			}
		}
	}

	/* Script callback - find out what meta key was pressed */
	int pressedMetaKey = KEY_IGNORE;

	/* getLastMetaKey() can't be used here, have to do manually */
	if (keyDown(KEY_LCTRL))
	{
		pressedMetaKey = KEY_LCTRL;
	}
	else if (keyDown(KEY_RCTRL))
	{
		pressedMetaKey = KEY_RCTRL;
	}
	else if (keyDown(KEY_LALT))
	{
		pressedMetaKey = KEY_LALT;
	}
	else if (keyDown(KEY_RALT))
	{
		pressedMetaKey = KEY_RALT;
	}
	else if (keyDown(KEY_LSHIFT))
	{
		pressedMetaKey = KEY_LSHIFT;
	}
	else if (keyDown(KEY_RSHIFT))
	{
		pressedMetaKey = KEY_RSHIFT;
	}
	else if (keyDown(KEY_LMETA))
	{
		pressedMetaKey = KEY_LMETA;
	}
	else if (keyDown(KEY_RMETA))
	{
		pressedMetaKey = KEY_RMETA;
	}

	/* Find out what keys were pressed */
	for (int i = 0; i < KEY_MAXSCAN; i++)
	{
		/* Skip meta keys */
		switch (i)
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
		if (keyPressed((KEY_CODE)i))
		{
			triggerEventKeyPressed(pressedMetaKey, i);
		}
	}
}

// ----------------------------------------------------------------------------------
/* Returns the key code of the last sub key pressed - allows called functions to have a simple stack */
KEY_CODE getLastSubKey()
{
	return lastSubKey;
}

// ----------------------------------------------------------------------------------
/* Returns the key code of the last meta key pressed - allows called functions to have a simple stack */
KEY_CODE getLastMetaKey()
{
	return lastMetaKey;
}


static const KEY_CODE qwertyCodes[26] =
{
	//  +---+   +---+   +---+   +---+   +---+   +---+   +---+   +---+   +---+   +---+
	    KEY_Q,  KEY_W,  KEY_E,  KEY_R,  KEY_T,  KEY_Y,  KEY_U,  KEY_I,  KEY_O,  KEY_P,
	//  +---+   +---+   +---+   +---+   +---+   +---+   +---+   +---+   +---+   +---+
	//    +---+   +---+   +---+   +---+   +---+   +---+   +---+   +---+   +---+
	      KEY_A,  KEY_S,  KEY_D,  KEY_F,  KEY_G,  KEY_H,  KEY_J,  KEY_K,  KEY_L,
	//    +---+   +---+   +---+   +---+   +---+   +---+   +---+   +---+   +---+
	//        +---+   +---+   +---+   +---+   +---+   +---+   +---+
	          KEY_Z,  KEY_X,  KEY_C,  KEY_V,  KEY_B,  KEY_N,  KEY_M
	//        +---+   +---+   +---+   +---+   +---+   +---+   +---+
};

/* Returns the key code of the first ascii key that its finds has been PRESSED */
static KEY_CODE getQwertyKey()
{
	for (KEY_CODE code : qwertyCodes)
	{
		if (keyPressed(code))
		{
			return code;  // Top-, middle- or bottom-row key pressed.
		}
	}

	return (KEY_CODE)0;                     // no ascii key pressed
}

// ----------------------------------------------------------------------------------
/*	Returns the number (0 to 26) of a key on the keyboard
	from it's keycode. Q is zero, through to M being 25
*/
UDWORD asciiKeyCodeToTable(KEY_CODE code)
{
	unsigned i;
	for (i = 0; i < ARRAY_SIZE(qwertyCodes); ++i)
	{
		if (code == qwertyCodes[i])
		{
			return i;
		}
	}

	ASSERT(false, "only pass nonzero key codes from getQwertyKey to this function");
	return 0;
}

// ----------------------------------------------------------------------------------
/* Returns the map X position associated with the passed in keycode */
UDWORD	getMarkerX(KEY_CODE code)
{
	UDWORD	entry;
	entry = asciiKeyCodeToTable(code);
	return (qwertyKeyMappings[entry].xPos);
}

// ----------------------------------------------------------------------------------
/* Returns the map Y position associated with the passed in keycode */
UDWORD	getMarkerY(KEY_CODE code)
{
	UDWORD	entry;
	entry = asciiKeyCodeToTable(code);
	return (qwertyKeyMappings[entry].yPos);
}

// ----------------------------------------------------------------------------------
/* Returns the map Y rotation associated with the passed in keycode */
SDWORD	getMarkerSpin(KEY_CODE code)
{
	UDWORD	entry;
	entry = asciiKeyCodeToTable(code);
	return (qwertyKeyMappings[entry].spin);
}


// ----------------------------------------------------------------------------------
/* Defines whether we process debug key mapping stuff */
void processDebugMappings(unsigned player, bool val)
{
	bWantDebugMappings[player] = val;
	bDoingDebugMappings = true;
	for (unsigned n = 0; n < MAX_PLAYERS; ++n)
	{
		bDoingDebugMappings = bDoingDebugMappings && (bWantDebugMappings[n] || !NetPlay.players[n].allocated);
	}
}

// ----------------------------------------------------------------------------------
/* Returns present status of debug mapping processing */
bool getDebugMappingStatus()
{
	return bDoingDebugMappings;
}
bool getWantedDebugMappingStatus(unsigned player)
{
	return bWantDebugMappings[player];
}
std::string getWantedDebugMappingStatuses(bool val)
{
	char ret[MAX_PLAYERS + 1];
	char *p = ret;
	for (unsigned n = 0; n < MAX_PLAYERS; ++n)
	{
		if (NetPlay.players[n].allocated && bWantDebugMappings[n] == val)
		{
			*p++ = '0' + NetPlay.players[n].position;
		}
	}
	std::sort(ret, p);
	*p++ = '\0';
	return ret;
}
// ----------------------------------------------------------------------------------
bool keyReAssignMapping(KEY_CODE origMetaCode, KEY_CODE origSubCode, KEY_CODE newMetaCode, KEY_CODE newSubCode)
{
	/* Find the original */
	if (KEY_MAPPING *psMapping = keyFindMapping(origMetaCode, origSubCode))
	{
		/* Not all can be remapped */
		if (psMapping->status != KEYMAP_ALWAYS || psMapping->status == KEYMAP_ALWAYS_PROCESS)
		{
			psMapping->metaKeyCode = newMetaCode;
			psMapping->subKeyCode = newSubCode;
			return true;
		}
	}

	return false;
}
