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
#include <unordered_map>

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

class KeyMapSaveTable {
public:
	KeyMapSaveTable(std::initializer_list<KeyMapSaveEntry> items)
	: ordered_list(items)
	{
		for (size_t i = 0; i < ordered_list.size(); ++i)
		{
			functionpt_to_index_map[ordered_list[i].function] = i;
			name_to_index_map[ordered_list[i].name] = i;
		}
	}
public:
	KeyMapSaveEntry const *keymapEntryByFunction(void (*function)()) const
	{
		auto it = functionpt_to_index_map.find(function);
		if (it != functionpt_to_index_map.end()) {
			return &ordered_list[it->second];
		}
		return nullptr;
	}

	KeyMapSaveEntry const *keymapEntryByName(std::string const &name) const
	{
		auto it = name_to_index_map.find(name);
		if (it != name_to_index_map.end()) {
			return &ordered_list[it->second];
		}
		return nullptr;
	}

	bool sortKeyMappingFunc(const KEY_MAPPING& a, const KEY_MAPPING& b) const
	{
		auto it_a = functionpt_to_index_map.find(a.function);
		auto it_b = functionpt_to_index_map.find(b.function);
		if (it_a == functionpt_to_index_map.end())
		{
			debug(LOG_ERROR, "Failed to find KEY_MAPPING a (%s)", a.name.c_str());
			return &a < &b;
		}
		if (it_b == functionpt_to_index_map.end())
		{
			debug(LOG_ERROR, "Failed to find KEY_MAPPING b (%s)", b.name.c_str());
			return &a < &b;
		}
		return it_a->second < it_b->second;
	}
private:
	std::vector<KeyMapSaveEntry> ordered_list;
	std::unordered_map<void (*)(), size_t> functionpt_to_index_map;
	std::unordered_map<std::string, size_t> name_to_index_map;
};

static KeyMapSaveTable const keyMapSaveTable(
{
	{kf_ChooseManufacture, "ChooseManufacture"},
	{kf_ChooseResearch, "ChooseResearch"},
	{kf_ChooseBuild, "ChooseBuild"},
	{kf_ChooseDesign, "ChooseDesign"},
	{kf_ChooseIntelligence, "ChooseIntelligence"},
	{kf_ChooseCommand, "ChooseCommand"},
	{kf_QuickSave, "QuickSave"},
	{kf_ToggleRadar, "ToggleRadar"},
	{kf_QuickLoad, "QuickLoad"},
	{kf_ToggleConsole, "ToggleConsole"},
	{kf_ToggleEnergyBars, "ToggleEnergyBars"},
	{kf_ScreenDump, "ScreenDump"},
	{kf_ToggleFormationSpeedLimiting, "ToggleFormationSpeedLimiting"},
	{kf_MoveToLastMessagePos, "MoveToLastMessagePos"},
	{kf_ToggleSensorDisplay, "ToggleSensorDisplay"},
	// **********************************
	// **********************************
	// ASSIGN GROUPS
	{kf_AssignGrouping_0, "AssignGrouping_0"},
	{kf_AssignGrouping_1, "AssignGrouping_1"},
	{kf_AssignGrouping_2, "AssignGrouping_2"},
	{kf_AssignGrouping_3, "AssignGrouping_3"},
	{kf_AssignGrouping_4, "AssignGrouping_4"},
	{kf_AssignGrouping_5, "AssignGrouping_5"},
	{kf_AssignGrouping_6, "AssignGrouping_6"},
	{kf_AssignGrouping_7, "AssignGrouping_7"},
	{kf_AssignGrouping_8, "AssignGrouping_8"},
	{kf_AssignGrouping_9, "AssignGrouping_9"},
	// **********************************
	// **********************************
	// ADD TO GROUP
	{kf_AddGrouping_0, "AddGrouping_0"},
	{kf_AddGrouping_1, "AddGrouping_1"},
	{kf_AddGrouping_2, "AddGrouping_2"},
	{kf_AddGrouping_3, "AddGrouping_3"},
	{kf_AddGrouping_4, "AddGrouping_4"},
	{kf_AddGrouping_5, "AddGrouping_5"},
	{kf_AddGrouping_6, "AddGrouping_6"},
	{kf_AddGrouping_7, "AddGrouping_7"},
	{kf_AddGrouping_8, "AddGrouping_8"},
	{kf_AddGrouping_9, "AddGrouping_9"},
	// **********************************
	// **********************************
	// SELECT GROUPS - Will jump to the group as well as select if group is ALREADY selected
	{kf_SelectGrouping_0, "SelectGrouping_0"},
	{kf_SelectGrouping_1, "SelectGrouping_1"},
	{kf_SelectGrouping_2, "SelectGrouping_2"},
	{kf_SelectGrouping_3, "SelectGrouping_3"},
	{kf_SelectGrouping_4, "SelectGrouping_4"},
	{kf_SelectGrouping_5, "SelectGrouping_5"},
	{kf_SelectGrouping_6, "SelectGrouping_6"},
	{kf_SelectGrouping_7, "SelectGrouping_7"},
	{kf_SelectGrouping_8, "SelectGrouping_8"},
	{kf_SelectGrouping_9, "SelectGrouping_9"},
	// **********************************
	// **********************************
	// SELECT COMMANDER - Will jump to the group as well as select if group is ALREADY selected
	{kf_SelectCommander_0, "SelectCommander_0"},
	{kf_SelectCommander_1, "SelectCommander_1"},
	{kf_SelectCommander_2, "SelectCommander_2"},
	{kf_SelectCommander_3, "SelectCommander_3"},
	{kf_SelectCommander_4, "SelectCommander_4"},
	{kf_SelectCommander_5, "SelectCommander_5"},
	{kf_SelectCommander_6, "SelectCommander_6"},
	{kf_SelectCommander_7, "SelectCommander_7"},
	{kf_SelectCommander_8, "SelectCommander_8"},
	{kf_SelectCommander_9, "SelectCommander_9"},
	// **********************************
	// **********************************
	// MULTIPLAYER
	{kf_addMultiMenu, "addMultiMenu"},
	//
	// GAME CONTROLS - Moving around, zooming in, rotating etc
	{kf_CameraUp, "CameraUp"},
	{kf_CameraDown, "CameraDown"},
	{kf_CameraRight, "CameraRight"},
	{kf_CameraLeft, "CameraLeft"},
	{kf_SeekNorth, "SeekNorth"},
	{kf_ToggleDriveMode, "ToggleDriveMode"},
	{kf_ToggleCamera, "ToggleCamera"},
	{kf_addInGameOptions, "addInGameOptions"},
	{kf_RadarZoomOut, "RadarZoomOut"},
	{kf_RadarZoomIn, "RadarZoomIn"},
	{kf_ZoomIn, "ZoomIn"},
	{kf_ZoomOut, "ZoomOut"},
	{kf_PitchForward, "PitchForward"},
	{kf_RotateLeft, "RotateLeft"},
	{kf_ResetPitch, "ResetPitch"},
	{kf_RotateRight, "RotateRight"},
	{kf_PitchBack, "PitchBack"},
	{kf_RightOrderMenu, "RightOrderMenu"},
	{kf_SlowDown, "SlowDown"},
	{kf_SpeedUp, "SpeedUp"},
	{kf_NormalSpeed, "NormalSpeed"},
	{kf_FaceNorth, "FaceNorth"},
	{kf_FaceSouth, "FaceSouth"},
	{kf_FaceEast, "FaceEast"},
	{kf_FaceWest, "FaceWest"},
	{kf_JumpToResourceExtractor, "JumpToResourceExtractor"},
	{kf_JumpToRepairUnits, "JumpToRepairUnits"},
	{kf_JumpToConstructorUnits, "JumpToConstructorUnits"},
	{kf_JumpToSensorUnits, "JumpToSensorUnits"},
	{kf_JumpToCommandUnits, "JumpToCommandUnits"},
	{kf_ToggleOverlays, "ToggleOverlays"},
	{kf_ToggleConsoleDrop, "ToggleConsoleDrop"},
	{kf_ToggleTeamChat, "ToggleTeamChat"},
	{kf_RotateBuildingCW, "RotateBuildingClockwise"},
	{kf_RotateBuildingACW, "RotateBuildingAnticlockwise"},
	// **********************************
	// IN GAME MAPPINGS - Single key presses - ALL __DEBUG keymappings will be removed for master
	{kf_CentreOnBase, "CentreOnBase"},
	{kf_SetDroidAttackCease, "SetDroidAttackCease"},
	{kf_JumpToUnassignedUnits, "JumpToUnassignedUnits"},
	{kf_SetDroidAttackReturn, "SetDroidAttackReturn"},
	{kf_SetDroidAttackAtWill, "SetDroidAttackAtWill"},
	{kf_SetDroidMoveGuard, "SetDroidMoveGuard"},
	{kf_SetDroidReturnToBase, "SetDroidReturnToBase"},
	{kf_SetDroidOrderHold, "SetDroidOrderHold"},
	{kf_SetDroidRangeOptimum, "SetDroidRangeOptimum"},
	{kf_SetDroidRangeShort, "SetDroidRangeShort"},
	{kf_SetDroidMovePursue, "SetDroidMovePursue"},
	{kf_SetDroidMovePatrol, "SetDroidMovePatrol"},
	{kf_SetDroidGoForRepair, "SetDroidGoForRepair"},
	{kf_SetDroidOrderStop, "SetDroidOrderStop"},
	{kf_SetDroidGoToTransport, "SetDroidGoToTransport"},
	{kf_SetDroidRangeLong, "SetDroidRangeLong"},
	{kf_SendGlobalMessage, "SendGlobalMessage"},
	{kf_SendTeamMessage, "SendTeamMessage"},
	{kf_AddHelpBlip, "AddHelpBlip"},
	//
	{kf_ToggleShadows, "ToggleShadows"},
	{kf_toggleTrapCursor, "toggleTrapCursor"},
	{kf_ToggleRadarTerrain, "ToggleRadarTerrain"},
	{kf_ToggleRadarAllyEnemy, "ToggleRadarAllyEnemy"},
	{kf_ShowMappings, "ShowMappings"},
	//
	// Some extra non QWERTY mappings but functioning in same way
	{kf_SetDroidRetreatMedium, "SetDroidRetreatMedium"},
	{kf_SetDroidRetreatHeavy, "SetDroidRetreatHeavy"},
	{kf_SetDroidRetreatNever, "SetDroidRetreatNever"},
	// **********************************
	// **********************************
	// In game mappings - COMBO (CTRL + LETTER) presses.
	{kf_SelectAllCombatUnits, "SelectAllCombatUnits"},
	{kf_SelectAllCyborgs, "SelectAllCyborgs"},
	{kf_SelectAllDamaged, "SelectAllDamaged"},
	{kf_SelectAllHalfTracked, "SelectAllHalfTracked"},
	{kf_SelectAllHovers, "SelectAllHovers"},
	{kf_SetDroidRecycle, "SetDroidRecycle"},
	{kf_SelectAllOnScreenUnits, "SelectAllOnScreenUnits"},
	{kf_SelectAllTracked, "SelectAllTracked"},
	{kf_SelectAllUnits, "SelectAllUnits"},
	{kf_SelectAllVTOLs, "SelectAllVTOLs"},
	{kf_SelectAllArmedVTOLs, "SelectAllArmedVTOLs"},
	{kf_SelectAllWheeled, "SelectAllWheeled"},
	{kf_FrameRate, "FrameRate"},
	{kf_SelectAllSameType, "SelectAllSameType"},
	// **********************************
	// **********************************
	// In game mappings - COMBO (SHIFT + LETTER) presses.
	{kf_SelectAllCombatCyborgs, "SelectAllCombatCyborgs"},
	{kf_SelectAllEngineers, "SelectAllEngineers"},
	{kf_SelectAllLandCombatUnits, "SelectAllLandCombatUnits"},
	{kf_SelectAllMechanics, "SelectAllMechanics"},
	{kf_SelectAllTransporters, "SelectAllTransporters"},
	{kf_SelectAllRepairTanks, "SelectAllRepairTanks"},
	{kf_SelectAllSensorUnits, "SelectAllSensorUnits"},
	{kf_SelectAllTrucks, "SelectAllTrucks"},
	// **********************************
	// **********************************
	// SELECT PLAYERS - DEBUG ONLY
	{kf_SelectNextFactory, "SelectNextFactory"},
	{kf_SelectNextResearch, "SelectNextResearch"},
	{kf_SelectNextPowerStation, "SelectNextPowerStation"},
	{kf_SelectNextCyborgFactory, "SelectNextCyborgFactory"},
	{kf_SelectNextVTOLFactory, "SelectNextVtolFactory"},
	{kf_JumpNextFactory, "JumpNextFactory"},
	{kf_JumpNextResearch, "JumpNextResearch"},
	{kf_JumpNextPowerStation, "JumpNextPowerStation"},
	{kf_JumpNextCyborgFactory, "JumpNextCyborgFactory"},
	{kf_JumpNextVTOLFactory, "JumpNextVtolFactory"},
	//
	// Debug options
	{kf_ToggleDebugMappings, "ToggleDebugMappings"},
	{kf_ToggleShowPath, "ToggleShowPath"},
	{kf_ToggleShowGateways, "ToggleShowGateways"},
	{kf_ToggleVisibility, "ToggleVisibility"},
	{kf_RaiseTile, "RaiseTile"},
	{kf_LowerTile, "LowerTile"},
	{kf_ToggleFog, "ToggleFog"},
	{kf_ToggleWeather, "ToggleWeather"},
	{kf_TriFlip, "TriFlip"},
	{kf_PerformanceSample, "PerformanceSample"},
	//
	// These ones are necessary for debugging
	{kf_AllAvailable, "AllAvailable"},
	{kf_KillSelected, "KillSelected"},
	{kf_ToggleGodMode, "ToggleGodMode"},
	{kf_ChooseOptions, "ChooseOptions"},
	{kf_FinishResearch, "FinishResearch"},
	{kf_RevealMapAtPos, "RevealMapAtPos"},
	{kf_TraceObject, "TraceObject"},
});

KeyMapSaveEntry const *keymapEntryByFunction(void (*function)())
{
	return keyMapSaveTable.keymapEntryByFunction(function);
}

KeyMapSaveEntry const *keymapEntryByName(std::string const &name)
{
	return keyMapSaveTable.keymapEntryByName(name);
}

static bool keyAddDefaultMapping(KEY_STATUS status, KEY_CODE metaCode, KEY_CODE subCode, KEY_ACTION action, void (*pKeyMapFunc)(), const char *name, bool bForceDefaults); // forward-declare

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
	bool didLoadKeyMap = false;
	if (!bForceDefaults && loadKeyMap() == true)
	{
		debug(LOG_WZ, "Loaded key map successfully");
		didLoadKeyMap = true;
	}
	if (!didLoadKeyMap)
	{
		bForceDefaults = true;
	}
	// Use keyAddDefaultMapping to add the default key mapping if either: (a) bForceDefaults is true, or (b) the loaded key mappings are missing an entry
	bool didAdd = false;

	// ********************************* ALL THE MAPPINGS ARE NOW IN ORDER, PLEASE ****
	// ********************************* DO NOT REORDER THEM!!!!!! ********************
	/* ALL OF THIS NEEDS TO COME IN OFF A USER CUSTOMISABLE TEXT FILE */
	//                                **********************************
	//                                **********************************
	//									FUNCTION KEY MAPPINGS F1 to F12
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F1,  KEYMAP_PRESSED, kf_ChooseManufacture,            N_("Manufacture"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F2,  KEYMAP_PRESSED, kf_ChooseResearch,               N_("Research"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F3,  KEYMAP_PRESSED, kf_ChooseBuild,                  N_("Build"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F4,  KEYMAP_PRESSED, kf_ChooseDesign,                 N_("Design"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F5,  KEYMAP_PRESSED, kf_ChooseIntelligence,           N_("Intelligence Display"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F6,  KEYMAP_PRESSED, kf_ChooseCommand,                N_("Commanders"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F7,  KEYMAP_PRESSED, kf_QuickSave,                    N_("QuickSave"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_LSHIFT, KEY_F7,  KEYMAP_PRESSED, kf_ToggleRadar,                  N_("Toggle Radar"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F8,  KEYMAP_PRESSED, kf_QuickLoad,                    N_("QuickLoad"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_LSHIFT, KEY_F8,  KEYMAP_PRESSED, kf_ToggleConsole,                N_("Toggle Console Display"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F9,  KEYMAP_PRESSED, kf_ToggleEnergyBars,             N_("Toggle Damage Bars On/Off"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS,         KEY_IGNORE, KEY_F10, KEYMAP_PRESSED, kf_ScreenDump,                   N_("Take Screen Shot"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F11, KEYMAP_PRESSED, kf_ToggleFormationSpeedLimiting, N_("Toggle Formation Speed Limiting"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F12, KEYMAP_PRESSED, kf_MoveToLastMessagePos,         N_("View Location of Previous Message"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_LSHIFT, KEY_F12, KEYMAP_PRESSED, kf_ToggleSensorDisplay,          N_("Toggle Sensor display"), bForceDefaults) || didAdd; //Which key should we use? --Re enabled see below! -Q 5-10-05
	//                                **********************************
	//                                **********************************
	//	ASSIGN GROUPS - Will create or replace the existing group
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_0, KEYMAP_PRESSED, kf_AssignGrouping_0, N_("Assign Group 0"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_1, KEYMAP_PRESSED, kf_AssignGrouping_1, N_("Assign Group 1"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_2, KEYMAP_PRESSED, kf_AssignGrouping_2, N_("Assign Group 2"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_3, KEYMAP_PRESSED, kf_AssignGrouping_3, N_("Assign Group 3"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_4, KEYMAP_PRESSED, kf_AssignGrouping_4, N_("Assign Group 4"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_5, KEYMAP_PRESSED, kf_AssignGrouping_5, N_("Assign Group 5"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_6, KEYMAP_PRESSED, kf_AssignGrouping_6, N_("Assign Group 6"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_7, KEYMAP_PRESSED, kf_AssignGrouping_7, N_("Assign Group 7"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_8, KEYMAP_PRESSED, kf_AssignGrouping_8, N_("Assign Group 8"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_9, KEYMAP_PRESSED, kf_AssignGrouping_9, N_("Assign Group 9"), bForceDefaults) || didAdd;
	//	ADD TO GROUPS - Will add the selected units to the group
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_0, KEYMAP_PRESSED, kf_AddGrouping_0, N_("Add to Group 0"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_1, KEYMAP_PRESSED, kf_AddGrouping_1, N_("Add to Group 1"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_2, KEYMAP_PRESSED, kf_AddGrouping_2, N_("Add to Group 2"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_3, KEYMAP_PRESSED, kf_AddGrouping_3, N_("Add to Group 3"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_4, KEYMAP_PRESSED, kf_AddGrouping_4, N_("Add to Group 4"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_5, KEYMAP_PRESSED, kf_AddGrouping_5, N_("Add to Group 5"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_6, KEYMAP_PRESSED, kf_AddGrouping_6, N_("Add to Group 6"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_7, KEYMAP_PRESSED, kf_AddGrouping_7, N_("Add to Group 7"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_8, KEYMAP_PRESSED, kf_AddGrouping_8, N_("Add to Group 8"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_9, KEYMAP_PRESSED, kf_AddGrouping_9, N_("Add to Group 9"), bForceDefaults) || didAdd;
	//                                **********************************
	//                                **********************************
	//	SELECT GROUPS - Will jump to the group as well as select if group is ALREADY selected
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_0, KEYMAP_PRESSED, kf_SelectGrouping_0, N_("Select Group 0"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_1, KEYMAP_PRESSED, kf_SelectGrouping_1, N_("Select Group 1"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_2, KEYMAP_PRESSED, kf_SelectGrouping_2, N_("Select Group 2"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_3, KEYMAP_PRESSED, kf_SelectGrouping_3, N_("Select Group 3"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_4, KEYMAP_PRESSED, kf_SelectGrouping_4, N_("Select Group 4"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_5, KEYMAP_PRESSED, kf_SelectGrouping_5, N_("Select Group 5"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_6, KEYMAP_PRESSED, kf_SelectGrouping_6, N_("Select Group 6"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_7, KEYMAP_PRESSED, kf_SelectGrouping_7, N_("Select Group 7"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_8, KEYMAP_PRESSED, kf_SelectGrouping_8, N_("Select Group 8"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_9, KEYMAP_PRESSED, kf_SelectGrouping_9, N_("Select Group 9"), bForceDefaults) || didAdd;
	//                                **********************************
	//                                **********************************
	//	SELECT COMMANDER - Will jump to the group as well as select if group is ALREADY selected
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_0, KEYMAP_PRESSED, kf_SelectCommander_0, N_("Select Commander 0"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_1, KEYMAP_PRESSED, kf_SelectCommander_1, N_("Select Commander 1"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_2, KEYMAP_PRESSED, kf_SelectCommander_2, N_("Select Commander 2"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_3, KEYMAP_PRESSED, kf_SelectCommander_3, N_("Select Commander 3"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_4, KEYMAP_PRESSED, kf_SelectCommander_4, N_("Select Commander 4"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_5, KEYMAP_PRESSED, kf_SelectCommander_5, N_("Select Commander 5"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_6, KEYMAP_PRESSED, kf_SelectCommander_6, N_("Select Commander 6"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_7, KEYMAP_PRESSED, kf_SelectCommander_7, N_("Select Commander 7"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_8, KEYMAP_PRESSED, kf_SelectCommander_8, N_("Select Commander 8"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_9, KEYMAP_PRESSED, kf_SelectCommander_9, N_("Select Commander 9"), bForceDefaults) || didAdd;
	//                                **********************************
	//                                **********************************
	//	MULTIPLAYER
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KPENTER, KEYMAP_PRESSED, kf_addMultiMenu, N_("Multiplayer Options / Alliance dialog"), bForceDefaults) || didAdd;
	//
	//	GAME CONTROLS - Moving around, zooming in, rotating etc
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_UPARROW,           KEYMAP_DOWN,    kf_CameraUp,                N_("Move Camera Up"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_DOWNARROW,         KEYMAP_DOWN,    kf_CameraDown,              N_("Move Camera Down"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_RIGHTARROW,        KEYMAP_DOWN,    kf_CameraRight,             N_("Move Camera Right"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_LEFTARROW,         KEYMAP_DOWN,    kf_CameraLeft,              N_("Move Camera Left"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_BACKSPACE,         KEYMAP_PRESSED, kf_SeekNorth,               N_("Snap View to North"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_SPACE,             KEYMAP_PRESSED, kf_ToggleCamera,            N_("Toggle Tracking Camera"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_SPACE,             KEYMAP_PRESSED, kf_ToggleDriveMode,            N_("Toggle Drive Mode"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS,     KEY_IGNORE, KEY_ESC,               KEYMAP_PRESSED, kf_addInGameOptions,        N_("Display In-Game Options"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_MINUS,             KEYMAP_PRESSED, kf_RadarZoomOut,            N_("Zoom Radar Out"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_EQUALS,            KEYMAP_PRESSED, kf_RadarZoomIn,             N_("Zoom Radar In"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_PLUS,           KEYMAP_DOWN,    kf_ZoomIn,                  N_("Zoom In"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_MINUS,          KEYMAP_DOWN,    kf_ZoomOut,                 N_("Zoom Out"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_2,              KEYMAP_DOWN,    kf_PitchForward,            N_("Pitch Forward"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_4,              KEYMAP_DOWN,    kf_RotateLeft,              N_("Rotate Left"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_5,              KEYMAP_DOWN,    kf_ResetPitch,              N_("Reset Pitch"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_6,              KEYMAP_DOWN,    kf_RotateRight,             N_("Rotate Right"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_8,              KEYMAP_DOWN,    kf_PitchBack,               N_("Pitch Back"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_0,              KEYMAP_PRESSED, kf_RightOrderMenu,          N_("Orders Menu"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_MINUS,             KEYMAP_PRESSED, kf_SlowDown,                N_("Decrease Game Speed"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_EQUALS,            KEYMAP_PRESSED, kf_SpeedUp,                 N_("Increase Game Speed"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_BACKSPACE,         KEYMAP_PRESSED, kf_NormalSpeed,             N_("Reset Game Speed"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_UPARROW,           KEYMAP_PRESSED, kf_FaceNorth,               N_("View North"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_DOWNARROW,         KEYMAP_PRESSED, kf_FaceSouth,               N_("View South"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_LEFTARROW,         KEYMAP_PRESSED, kf_FaceEast,                N_("View East"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_RIGHTARROW,        KEYMAP_PRESSED, kf_FaceWest,                N_("View West"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_STAR,           KEYMAP_PRESSED, kf_JumpToResourceExtractor, N_("View next Oil Derrick"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpToRepairUnits,       N_("View next Repair Unit"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpToConstructorUnits,  N_("View next Truck"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpToSensorUnits,       N_("View next Sensor Unit"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpToCommandUnits,      N_("View next Commander"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_TAB,               KEYMAP_PRESSED, kf_ToggleOverlays,          N_("Toggle Overlays"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_BACKQUOTE,         KEYMAP_PRESSED, kf_ToggleConsoleDrop,       N_("Toggle Console History "), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_BACKQUOTE,         KEYMAP_PRESSED, kf_ToggleTeamChat,          N_("Toggle Team Chat History"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_RotateBuildingCW,        N_("Rotate Building Clockwise"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_RotateBuildingACW,       N_("Rotate Building Anticlockwise"), bForceDefaults) || didAdd;
	//                                **********************************
	// IN GAME MAPPINGS - Single key presses - ALL __DEBUG keymappings will be removed for master
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_B,      KEYMAP_PRESSED, kf_CentreOnBase,          N_("Center View on HQ"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_C,      KEYMAP_PRESSED, kf_SetDroidAttackCease,   N_("Hold Fire"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_D,      KEYMAP_PRESSED, kf_JumpToUnassignedUnits, N_("View Unassigned Units"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_E,      KEYMAP_PRESSED, kf_SetDroidAttackReturn,  N_("Return Fire"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_F,      KEYMAP_PRESSED, kf_SetDroidAttackAtWill,  N_("Fire at Will"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_G,      KEYMAP_PRESSED, kf_SetDroidMoveGuard,     N_("Guard Position"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_H,      KEYMAP_PRESSED, kf_SetDroidReturnToBase,  N_("Return to HQ"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_H,      KEYMAP_PRESSED, kf_SetDroidOrderHold,     N_("Hold Position"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_I,      KEYMAP_PRESSED, kf_SetDroidRangeOptimum,  N_("Optimum Range"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_O,      KEYMAP_PRESSED, kf_SetDroidRangeShort,    N_("Short Range"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_P,      KEYMAP_PRESSED, kf_SetDroidMovePursue,    N_("Pursue"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_Q,      KEYMAP_PRESSED, kf_SetDroidMovePatrol,    N_("Patrol"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_R,      KEYMAP_PRESSED, kf_SetDroidGoForRepair,   N_("Return For Repair"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_S,      KEYMAP_PRESSED, kf_SetDroidOrderStop,     N_("Stop Droid"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_T,      KEYMAP_PRESSED, kf_SetDroidGoToTransport, N_("Go to Transport"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_U,      KEYMAP_PRESSED, kf_SetDroidRangeLong,     N_("Long Range"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_RETURN, KEYMAP_PRESSED, kf_SendGlobalMessage,     N_("Send Global Text Message"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_RETURN, KEYMAP_PRESSED, kf_SendTeamMessage,       N_("Send Team Text Message"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT,   KEY_H,      KEYMAP_PRESSED, kf_AddHelpBlip,           N_("Drop a beacon"), bForceDefaults) || didAdd;

	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT,   KEY_S,   KEYMAP_PRESSED,  kf_ToggleShadows,        N_("Toggles shadows"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT,   KEY_T,   KEYMAP_PRESSED,  kf_toggleTrapCursor,     N_("Trap cursor"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_TAB, KEYMAP_PRESSED,  kf_ToggleRadarTerrain,   N_("Toggle radar terrain"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_TAB, KEYMAP_PRESSED,  kf_ToggleRadarAllyEnemy, N_("Toggle ally-enemy radar view"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_M,   KEYMAP_PRESSED,  kf_ShowMappings,         N_("Show all keyboard mappings"), bForceDefaults) || didAdd;

	// Some extra non QWERTY mappings but functioning in same way
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_COMMA,        KEYMAP_PRESSED, kf_SetDroidRetreatMedium, N_("Retreat at Medium Damage"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_FULLSTOP,     KEYMAP_PRESSED, kf_SetDroidRetreatHeavy,  N_("Retreat at Heavy Damage"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_FORWARDSLASH, KEYMAP_PRESSED, kf_SetDroidRetreatNever,  N_("Do or Die!"), bForceDefaults) || didAdd;
	//                                **********************************
	//                                **********************************
	//								In game mappings - COMBO (CTRL + LETTER) presses.
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_A, KEYMAP_PRESSED, kf_SelectAllCombatUnits,   N_("Select all Combat Units"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_C, KEYMAP_PRESSED, kf_SelectAllCyborgs,       N_("Select all Cyborgs"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_D, KEYMAP_PRESSED, kf_SelectAllDamaged,       N_("Select all Heavily Damaged Units"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_F, KEYMAP_PRESSED, kf_SelectAllHalfTracked,   N_("Select all Half-tracks"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_H, KEYMAP_PRESSED, kf_SelectAllHovers,        N_("Select all Hovers"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_R, KEYMAP_PRESSED, kf_SetDroidRecycle,        N_("Return for Recycling"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_S, KEYMAP_PRESSED, kf_SelectAllOnScreenUnits, N_("Select all Units on Screen"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_T, KEYMAP_PRESSED, kf_SelectAllTracked,       N_("Select all Tracks"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_U, KEYMAP_PRESSED, kf_SelectAllUnits,         N_("Select EVERY unit"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_V, KEYMAP_PRESSED, kf_SelectAllVTOLs,         N_("Select all VTOLs"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_V, KEYMAP_PRESSED, kf_SelectAllArmedVTOLs,   N_("Select all fully-armed VTOLs"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_W, KEYMAP_PRESSED, kf_SelectAllWheeled,       N_("Select all Wheels"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG,     KEY_LCTRL, KEY_Y, KEYMAP_PRESSED, kf_FrameRate,              N_("Show frame rate"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_Z, KEYMAP_PRESSED, kf_SelectAllSameType,      N_("Select all units with the same components"), bForceDefaults) || didAdd;
	//                                **********************************
	//                                **********************************
	//                                                              In game mappings - COMBO (SHIFT + LETTER) presses.
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_C, KEYMAP_PRESSED, kf_SelectAllCombatCyborgs,   N_("Select all Combat Cyborgs"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_E, KEYMAP_PRESSED, kf_SelectAllEngineers,       N_("Select all Engineers"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_G, KEYMAP_PRESSED, kf_SelectAllLandCombatUnits, N_("Select all Land Combat Units"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_M, KEYMAP_PRESSED, kf_SelectAllMechanics,       N_("Select all Mechanics"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_P, KEYMAP_PRESSED, kf_SelectAllTransporters,    N_("Select all Transporters"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_R, KEYMAP_PRESSED, kf_SelectAllRepairTanks,     N_("Select all Repair Tanks"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_S, KEYMAP_PRESSED, kf_SelectAllSensorUnits,     N_("Select all Sensor Units"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_T, KEYMAP_PRESSED, kf_SelectAllTrucks,          N_("Select all Trucks"), bForceDefaults) || didAdd;
	//                                **********************************
	//                                **********************************
	//									SELECT PLAYERS - DEBUG ONLY
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_SelectNextFactory,       N_("Select next Factory"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_SelectNextResearch,      N_("Select next Research Facility"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_SelectNextPowerStation,  N_("Select next Power Generator"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_SelectNextCyborgFactory, N_("Select next Cyborg Factory"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_SelectNextVTOLFactory,   N_("Select next VTOL Factory"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpNextFactory,         N_("Jump to next Factory"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpNextResearch,        N_("Jump to next Research Facility"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpNextPowerStation,    N_("Jump to next Power Generator"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpNextCyborgFactory,   N_("Jump to next Cyborg Factory"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpNextVTOLFactory,     N_("Jump to next VTOL Factory"), bForceDefaults) || didAdd;

	// Debug options
	didAdd = keyAddDefaultMapping(KEYMAP___HIDE, KEY_LSHIFT, KEY_BACKSPACE, KEYMAP_PRESSED, kf_ToggleDebugMappings, N_("Toggle Debug Mappings"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_M,         KEYMAP_PRESSED, kf_ToggleShowPath,      N_("Toggle display of droid path"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_E,         KEYMAP_PRESSED, kf_ToggleShowGateways,  N_("Toggle display of gateways"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_IGNORE, KEY_V,         KEYMAP_PRESSED, kf_ToggleVisibility,    N_("Toggle visibility"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_IGNORE, KEY_W,         KEYMAP_DOWN,    kf_RaiseTile,           N_("Raise tile height"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_IGNORE, KEY_A,         KEYMAP_DOWN,    kf_LowerTile,           N_("Lower tile height"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_J,         KEYMAP_PRESSED, kf_ToggleFog,           N_("Toggles All fog"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_Q,         KEYMAP_PRESSED, kf_ToggleWeather,       N_("Trigger some weather"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_IGNORE, KEY_K,         KEYMAP_PRESSED, kf_TriFlip,             N_("Flip terrain triangle"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_K,         KEYMAP_PRESSED, kf_PerformanceSample,   N_("Make a performance measurement sample"), bForceDefaults) || didAdd;

	//These ones are necessary for debugging
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LALT,   KEY_A, KEYMAP_PRESSED, kf_AllAvailable,      N_("Make all items available"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LALT,   KEY_K, KEYMAP_PRESSED, kf_KillSelected,      N_("Kill Selected Unit(s)"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_G, KEYMAP_PRESSED, kf_ToggleGodMode,     N_("Toggle god Mode Status"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_O, KEYMAP_PRESSED, kf_ChooseOptions,     N_("Display Options Screen"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_X, KEYMAP_PRESSED, kf_FinishResearch,    N_("Complete current research"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LSHIFT, KEY_W, KEYMAP_PRESSED, kf_RevealMapAtPos,    N_("Reveal map at mouse position"), bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_L, KEYMAP_PRESSED, kf_TraceObject,       N_("Trace a game object"), bForceDefaults) || didAdd;

	// ensure sort ordering
	keyMappings.sort([](const KEY_MAPPING &a, const KEY_MAPPING &b) {
		return keyMapSaveTable.sortKeyMappingFunc(a, b);
	});

	if (didAdd)
	{
		saveKeyMap();	// save out the default key mappings.
	}
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

static bool keyAddDefaultMapping(KEY_STATUS status, KEY_CODE metaCode, KEY_CODE subCode, KEY_ACTION action, void (*pKeyMapFunc)(), const char *name, bool bForceDefaults)
{
	KEY_MAPPING *psMapping = keyGetMappingFromFunction(pKeyMapFunc);
	if (!bForceDefaults && psMapping != nullptr)
	{
		// Not forcing defaults, and there is already a mapping entry for this function
		return false;
	}

	// Otherwise, force / overwrite with the default
	if (!bForceDefaults)
	{
		debug(LOG_INFO, "Adding missing keymap entry: %s", name);
	}
	if (psMapping)
	{
		// Remove any existing mapping for this function
		keyRemoveMappingPt(psMapping);
		psMapping = nullptr;
	}
	if (!bForceDefaults)
	{
		// Clear down conflicting mappings using these keys... But only if it isn't unassigned
		keyReAssignMapping(metaCode, subCode, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN);
	}

	// Set default key mapping
	psMapping = keyAddMapping(status, metaCode, subCode, action, pKeyMapFunc, name);
	return true;
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
/* allows checking if mapping should currently be ignored in keyProcessMappings */
static bool isIgnoredMapping(const bool bExclude, const KEY_MAPPING& mapping)
{
	if (bExclude && mapping.status != KEYMAP_ALWAYS_PROCESS)
	{
		return true;
	}

	if (mapping.subKeyCode == (KEY_CODE)KEY_MAXSCAN)
	{
		return true;
	}

	if (mapping.function == nullptr)
	{
		return true;
	}

	const bool bIsDebugMapping = mapping.status == KEYMAP__DEBUG;
	if (bIsDebugMapping && !getDebugMappingStatus()) {
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------------
/* Manages update of all the active function mappings */
void keyProcessMappings(bool bExclude)
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
		if (isIgnoredMapping(bExclude, *keyToProcess))
		{
			continue;
		}

		// Check if there exists another keybinding, with the same subKeyCode, but with meta- or altMetaKeyCode
		// and which has its respective meta key currently down. If any are found, then we want to completely skip
		// any bindings without meta, as the keybind with meta (being more specific) should take precedence. This
		// allows e.g. having camera movement in arrow keys and alignment in arrow keys + ctrl
		const bool bIsKeyCombination = keyToProcess->metaKeyCode != KEY_IGNORE;
		if (bMetaKeyDown && !bIsKeyCombination) {
			bool bOverridingCombinationExists = false;
			for (auto otherKey = keyMappings.begin(); otherKey != keyMappings.end(); ++otherKey)
			{
				/* Skip inappropriate ones when necessary */
				if (isIgnoredMapping(bExclude, *keyToProcess))
				{
					continue;
				}

				/* Only match agaist keys with the same subKeyCode */
				if (otherKey->subKeyCode != keyToProcess->subKeyCode)
				{
					continue;
				}

				bool bIsKeyCombination = otherKey->metaKeyCode != KEY_IGNORE;
				if (bIsKeyCombination && keyPressed(otherKey->subKeyCode))
				{
					bool bHasAlt = otherKey->altMetaKeyCode != KEY_IGNORE;
					if (keyDown(otherKey->metaKeyCode) || (bHasAlt && keyDown(otherKey->altMetaKeyCode)))
					{
						bOverridingCombinationExists = true;
						break;
					}
				}
			}

			if (bOverridingCombinationExists) {
				continue;
			}
		}

		/* Process simple ones (single keys) */
		if (!bIsKeyCombination)
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
		/* Process the combination ones */
		else
		{
			/* It's a combo keypress - meta held down and the sub pressed */
			bool bSubKeyIsPressed = keyPressed(keyToProcess->subKeyCode);
			if (bSubKeyIsPressed)
			{
				bool bHasAlt = keyToProcess->altMetaKeyCode != KEY_IGNORE;

				/* First, try the meta key */
				if (keyDown(keyToProcess->metaKeyCode))
				{
					lastMetaKey = keyToProcess->metaKeyCode;
					lastSubKey = keyToProcess->subKeyCode;
					keyToProcess->function();
				}
				/* Meta key not held, check if we have alternative and try that */
				else if (bHasAlt && keyDown(keyToProcess->altMetaKeyCode))
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
