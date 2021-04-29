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
#include <unordered_set>

static UDWORD asciiKeyCodeToTable(KEY_CODE code);
static KEY_CODE getQwertyKey();

// ----------------------------------------------------------------------------------
bool KeyMappingInput::isPressed() const
{
	switch (source) {
	case KeyMappingInputSource::KEY_CODE:
		return keyPressed(value.keyCode);
	case KeyMappingInputSource::MOUSE_KEY_CODE:
		return mousePressed(value.mouseKeyCode);
	default:
		return false;
	}
}

bool KeyMappingInput::isDown() const
{
	switch (source) {
	case KeyMappingInputSource::KEY_CODE:
		return keyDown(value.keyCode);
	case KeyMappingInputSource::MOUSE_KEY_CODE:
		return mouseDown(value.mouseKeyCode);
	default:
		return false;
	}
}

bool KeyMappingInput::isReleased() const
{
	switch (source) {
	case KeyMappingInputSource::KEY_CODE:
		return keyReleased(value.keyCode);
	case KeyMappingInputSource::MOUSE_KEY_CODE:
		return mouseReleased(value.mouseKeyCode);
	default:
		return false;
	}
}

bool KeyMappingInput::isCleared() const
{
	return source == KeyMappingInputSource::KEY_CODE && value.keyCode == KEY_CODE::KEY_MAXSCAN;
}

bool KeyMappingInput::is(const KEY_CODE keyCode) const
{
	return source == KeyMappingInputSource::KEY_CODE && value.keyCode == keyCode;
}

bool KeyMappingInput::is(const MOUSE_KEY_CODE mouseKeyCode) const
{
	return source == KeyMappingInputSource::MOUSE_KEY_CODE && value.mouseKeyCode == mouseKeyCode;
}

nonstd::optional<KEY_CODE> KeyMappingInput::asKeyCode() const
{
	if (source == KeyMappingInputSource::KEY_CODE)
	{
		return value.keyCode;
	}
	else
	{
		return nonstd::nullopt;
	}
}

nonstd::optional<MOUSE_KEY_CODE> KeyMappingInput::asMouseKeyCode() const
{
	if (source == KeyMappingInputSource::MOUSE_KEY_CODE)
	{
		return value.mouseKeyCode;
	}
	else
	{
		return nonstd::nullopt;
	}
}

KeyMappingInput::KeyMappingInput()
	: source(KeyMappingInputSource::KEY_CODE)
	, value(KEY_CODE::KEY_IGNORE)
{}

KeyMappingInput::KeyMappingInput(const KEY_CODE KeyCode)
	: source(KeyMappingInputSource::KEY_CODE)
	, value(KeyMappingInputValue(KeyCode))
{
}

KeyMappingInput::KeyMappingInput(const MOUSE_KEY_CODE mouseKeyCode)
	: source(KeyMappingInputSource::MOUSE_KEY_CODE)
	, value(KeyMappingInputValue(mouseKeyCode))
{
}

KeyMappingInputValue::KeyMappingInputValue(const KEY_CODE keyCode)
	: keyCode(keyCode)
{
}

KeyMappingInputValue::KeyMappingInputValue(const MOUSE_KEY_CODE mouseKeyCode)
	: mouseKeyCode(mouseKeyCode)
{
}

// Key mapping inputs are unions with type enum attached, so we overload the equality
// comparison for convenience.
bool operator==(const KeyMappingInput& lhs, const KeyMappingInput& rhs) {
	if (lhs.source != rhs.source) {
		return false;
	}

	switch (lhs.source) {
	case KeyMappingInputSource::KEY_CODE:
		return lhs.value.keyCode == rhs.value.keyCode;
	case KeyMappingInputSource::MOUSE_KEY_CODE:
		return lhs.value.mouseKeyCode == rhs.value.mouseKeyCode;
	default:
		return false;
	}
}

bool operator!=(const KeyMappingInput& lhs, const KeyMappingInput& rhs) {
	return !(lhs == rhs);
}

// ----------------------------------------------------------------------------------

KEY_MAPPING *keyGetMappingFromFunction(void (*function)(), const KeyMappingSlot slot)
{
	auto mapping = std::find_if(keyMappings.begin(), keyMappings.end(), [function, slot](KEY_MAPPING const &mapping) {
		return mapping.function == function && mapping.slot == slot;
	});
	return mapping != keyMappings.end()? &*mapping : nullptr;
}

static bool isCombination(const KEY_MAPPING* mapping)
{
	return mapping->metaKeyCode != KEY_CODE::KEY_IGNORE;
}

static bool isActiveSingleKey(const KEY_MAPPING* mapping)
{
	switch (mapping->action)
	{
	case KEY_ACTION::KEYMAP_PRESSED:
		return mapping->input.isPressed();
	case KEY_ACTION::KEYMAP_DOWN:
		return mapping->input.isDown();
	case KEY_ACTION::KEYMAP_RELEASED:
		return mapping->input.isReleased();
	default:
		debug(LOG_WARNING, "Unknown key action (action code %u) while processing keymap.", (unsigned int)mapping->action);
		return false;
	}
}

static KEY_CODE getAlternativeForMetaKey(const KEY_CODE meta)
{
	auto altMeta = KEY_CODE::KEY_IGNORE;
	if (meta == KEY_CODE::KEY_LCTRL)
	{
		altMeta = KEY_CODE::KEY_RCTRL;
	}
	else if (meta == KEY_CODE::KEY_LALT)
	{
		altMeta = KEY_CODE::KEY_RALT;
	}
	else if (meta == KEY_CODE::KEY_LSHIFT)
	{
		altMeta = KEY_CODE::KEY_RSHIFT;
	}
	else if (meta == KEY_CODE::KEY_LMETA)
	{
		altMeta = KEY_CODE::KEY_RMETA;
	}

	return altMeta;
}

static bool isActiveCombination(const KEY_MAPPING* mapping)
{
	ASSERT(mapping->hasMeta(), "isActiveCombination called for non-meta key mapping!");

	const bool bSubKeyIsPressed = mapping->input.isPressed();
	const bool bMetaIsDown = keyDown(mapping->metaKeyCode);

	const auto altMeta = getAlternativeForMetaKey(mapping->metaKeyCode);
	const bool bHasAlt = altMeta != KEY_IGNORE;
	const bool bAltMetaIsDown = bHasAlt && keyDown(altMeta);

	return bSubKeyIsPressed && (bMetaIsDown || bAltMetaIsDown);
}

bool KEY_MAPPING::isActivated() const
{
	return isCombination(this)
		? isActiveCombination(this)
		: isActiveSingleKey(this);
}

bool KEY_MAPPING::hasMeta() const
{
	return metaKeyCode != KEY_CODE::KEY_IGNORE;
}

bool KEY_MAPPING::toString(char* pOutStr) const
{
	// Figure out if the keycode is for mouse or keyboard and print the name of
	// the respective key/mouse button to `asciiSub`
	char asciiSub[20] = "\0";
	switch (input.source)
	{
	case KeyMappingInputSource::KEY_CODE:
		keyScanToString(input.value.keyCode, (char*)&asciiSub, 20);
		break;
	case KeyMappingInputSource::MOUSE_KEY_CODE:
		mouseKeyCodeToString(input.value.mouseKeyCode, (char*)&asciiSub, 20);
		break;
	default:
		strcpy(asciiSub, "NOT VALID");
		debug(LOG_WZ, "Encountered invalid key mapping source %u while converting mapping to string!", static_cast<unsigned int>(input.source));
		return true;
	}

	if (hasMeta())
	{
		char asciiMeta[20] = "\0";
		keyScanToString(metaKeyCode, (char*)&asciiMeta, 20);

		sprintf(pOutStr, "%s %s", asciiMeta, asciiSub);
	}
	else
	{
		sprintf(pOutStr, "%s", asciiSub);
	}
	return true;
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
static bool bMappingsSortOrderDirty = true;

void invalidateKeyMappingSortOrder()
{
	bMappingsSortOrderDirty = true;
}

/* Last meta and sub key that were recorded */
static KEY_CODE	lastMetaKey;
static KeyMappingInput lastInput;

// ----------------------------------------------------------------------------------
KeyFunctionInfo::KeyFunctionInfo(
	void      (*function)(),
	std::string name,
	std::string displayName
)
	: function(function)
	, name(name)
	, displayName(displayName)
{}

class KeyFunctionInfoTable {
public:
	KeyFunctionInfoTable(std::vector<KeyFunctionInfo>& items)
		: ordered_list(std::move(items))
	{
		for (size_t i = 0; i < ordered_list.size(); ++i)
		{
			functionpt_to_index_map[ordered_list[i].function] = i;
			name_to_index_map[ordered_list[i].name] = i;
		}
	}
public:
	KeyFunctionInfo const *keyFunctionInfoByFunction(void (*function)()) const
	{
		auto it = functionpt_to_index_map.find(function);
		if (it != functionpt_to_index_map.end()) {
			return &ordered_list[it->second];
		}
		return nullptr;
	}

	KeyFunctionInfo const *keyFunctionInfoByName(std::string const &name) const
	{
		auto it = name_to_index_map.find(name);
		if (it != name_to_index_map.end()) {
			return &ordered_list[it->second];
		}
		return nullptr;
	}

	const std::vector<std::reference_wrapper<const KeyFunctionInfo>> allKeymapEntries() const
	{
		std::vector<std::reference_wrapper<const KeyFunctionInfo>> entries;
		for (auto const &keyFunctionInfo : ordered_list)
		{
			entries.push_back(keyFunctionInfo);
		}
		return entries;
	}
private:
	std::vector<KeyFunctionInfo> ordered_list;
	std::unordered_map<void (*)(), size_t> functionpt_to_index_map;
	std::unordered_map<std::string, size_t> name_to_index_map;
};

// Definitions/Configuration for all mappable Key Functions
//
// NOTE: The initialization is done as a function with bunch of emplace_backs instead of an initializer list for two reasons:
//        1.) KeyFunctionInfo is marked as non-copy to avoid unnecessarily copying them around. Using an initializer list requires
//            types to be copyable, so we cannot use initializer lists, at all (we use move-semantics with std::move instead)
//        2.) The initializer list itself would require >20kb of stack memory due to sheer size of this thing. Inserting all
//            entries one-by-one requires only one entry on the stack at a time, mitigating the risk of a stack overflow.
static KeyFunctionInfoTable initializeKeyFunctionInfoTable()
{
	std::vector<KeyFunctionInfo> entries;
	entries.emplace_back(KeyFunctionInfo(kf_ChooseManufacture,              "ChooseManufacture",            N_("Manufacture")));
	entries.emplace_back(KeyFunctionInfo(kf_ChooseResearch,                 "ChooseResearch",               N_("Research")));
	entries.emplace_back(KeyFunctionInfo(kf_ChooseBuild,                    "ChooseBuild",                  N_("Build")));
	entries.emplace_back(KeyFunctionInfo(kf_ChooseDesign,                   "ChooseDesign",                 N_("Design")));
	entries.emplace_back(KeyFunctionInfo(kf_ChooseIntelligence,             "ChooseIntelligence",           N_("Intelligence Display")));
	entries.emplace_back(KeyFunctionInfo(kf_ChooseCommand,                  "ChooseCommand",                N_("Commanders")));
	entries.emplace_back(KeyFunctionInfo(kf_QuickSave,                      "QuickSave",                    N_("QuickSave")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleRadar,                    "ToggleRadar",                  N_("Toggle Radar")));
	entries.emplace_back(KeyFunctionInfo(kf_QuickLoad,                      "QuickLoad",                    N_("QuickLoad")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleConsole,                  "ToggleConsole",                N_("Toggle Console Display")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleEnergyBars,               "ToggleEnergyBars",             N_("Toggle Damage Bars On/Off")));
	entries.emplace_back(KeyFunctionInfo(kf_ScreenDump,                     "ScreenDump",                   N_("Take Screen Shot")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleFormationSpeedLimiting,   "ToggleFormationSpeedLimiting", N_("Toggle Formation Speed Limiting")));
	entries.emplace_back(KeyFunctionInfo(kf_MoveToLastMessagePos,           "MoveToLastMessagePos",         N_("View Location of Previous Message")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleSensorDisplay,            "ToggleSensorDisplay",          N_("Toggle Sensor display")));
	// ASSIGN GROUPS
	entries.emplace_back(KeyFunctionInfo(kf_AssignGrouping_0,               "AssignGrouping_0",             N_("Assign Group 0")));
	entries.emplace_back(KeyFunctionInfo(kf_AssignGrouping_1,               "AssignGrouping_1",             N_("Assign Group 1")));
	entries.emplace_back(KeyFunctionInfo(kf_AssignGrouping_2,               "AssignGrouping_2",             N_("Assign Group 2")));
	entries.emplace_back(KeyFunctionInfo(kf_AssignGrouping_3,               "AssignGrouping_3",             N_("Assign Group 3")));
	entries.emplace_back(KeyFunctionInfo(kf_AssignGrouping_4,               "AssignGrouping_4",             N_("Assign Group 4")));
	entries.emplace_back(KeyFunctionInfo(kf_AssignGrouping_5,               "AssignGrouping_5",             N_("Assign Group 5")));
	entries.emplace_back(KeyFunctionInfo(kf_AssignGrouping_6,               "AssignGrouping_6",             N_("Assign Group 6")));
	entries.emplace_back(KeyFunctionInfo(kf_AssignGrouping_7,               "AssignGrouping_7",             N_("Assign Group 7")));
	entries.emplace_back(KeyFunctionInfo(kf_AssignGrouping_8,               "AssignGrouping_8",             N_("Assign Group 8")));
	entries.emplace_back(KeyFunctionInfo(kf_AssignGrouping_9,               "AssignGrouping_9",             N_("Assign Group 9")));
	// ADD TO GROUP
	entries.emplace_back(KeyFunctionInfo(kf_AddGrouping_0,                  "AddGrouping_0",                N_("Add to Group 0")));
	entries.emplace_back(KeyFunctionInfo(kf_AddGrouping_1,                  "AddGrouping_1",                N_("Add to Group 1")));
	entries.emplace_back(KeyFunctionInfo(kf_AddGrouping_2,                  "AddGrouping_2",                N_("Add to Group 2")));
	entries.emplace_back(KeyFunctionInfo(kf_AddGrouping_3,                  "AddGrouping_3",                N_("Add to Group 3")));
	entries.emplace_back(KeyFunctionInfo(kf_AddGrouping_4,                  "AddGrouping_4",                N_("Add to Group 4")));
	entries.emplace_back(KeyFunctionInfo(kf_AddGrouping_5,                  "AddGrouping_5",                N_("Add to Group 5")));
	entries.emplace_back(KeyFunctionInfo(kf_AddGrouping_6,                  "AddGrouping_6",                N_("Add to Group 6")));
	entries.emplace_back(KeyFunctionInfo(kf_AddGrouping_7,                  "AddGrouping_7",                N_("Add to Group 7")));
	entries.emplace_back(KeyFunctionInfo(kf_AddGrouping_8,                  "AddGrouping_8",                N_("Add to Group 8")));
	entries.emplace_back(KeyFunctionInfo(kf_AddGrouping_9,                  "AddGrouping_9",                N_("Add to Group 9")));
	// SELECT GROUPS - Will jump to the group as well as select if group is ALREADY selected
	entries.emplace_back(KeyFunctionInfo(kf_SelectGrouping_0,               "SelectGrouping_0",             N_("Select Group 0")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectGrouping_1,               "SelectGrouping_1",             N_("Select Group 1")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectGrouping_2,               "SelectGrouping_2",             N_("Select Group 2")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectGrouping_3,               "SelectGrouping_3",             N_("Select Group 3")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectGrouping_4,               "SelectGrouping_4",             N_("Select Group 4")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectGrouping_5,               "SelectGrouping_5",             N_("Select Group 5")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectGrouping_6,               "SelectGrouping_6",             N_("Select Group 6")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectGrouping_7,               "SelectGrouping_7",             N_("Select Group 7")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectGrouping_8,               "SelectGrouping_8",             N_("Select Group 8")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectGrouping_9,               "SelectGrouping_9",             N_("Select Group 9")));
	// SELECT COMMANDER - Will jump to the group as well as select if group is ALREADY selected
	entries.emplace_back(KeyFunctionInfo(kf_SelectCommander_0,              "SelectCommander_0",            N_("Select Commander 0")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectCommander_1,              "SelectCommander_1",            N_("Select Commander 1")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectCommander_2,              "SelectCommander_2",            N_("Select Commander 2")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectCommander_3,              "SelectCommander_3",            N_("Select Commander 3")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectCommander_4,              "SelectCommander_4",            N_("Select Commander 4")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectCommander_5,              "SelectCommander_5",            N_("Select Commander 5")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectCommander_6,              "SelectCommander_6",            N_("Select Commander 6")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectCommander_7,              "SelectCommander_7",            N_("Select Commander 7")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectCommander_8,              "SelectCommander_8",            N_("Select Commander 8")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectCommander_9,              "SelectCommander_9",            N_("Select Commander 9")));
	// MULTIPLAYER
	entries.emplace_back(KeyFunctionInfo(kf_addMultiMenu,                   "addMultiMenu",                 N_("Multiplayer Options / Alliance dialog")));
	// GAME CONTROLS - Moving around, zooming in, rotating etc
	entries.emplace_back(KeyFunctionInfo(kf_CameraUp,                       "CameraUp",                     N_("Move Camera Up")));
	entries.emplace_back(KeyFunctionInfo(kf_CameraDown,                     "CameraDown",                   N_("Move Camera Down")));
	entries.emplace_back(KeyFunctionInfo(kf_CameraRight,                    "CameraRight",                  N_("Move Camera Right")));
	entries.emplace_back(KeyFunctionInfo(kf_CameraLeft,                     "CameraLeft",                   N_("Move Camera Left")));
	entries.emplace_back(KeyFunctionInfo(kf_SeekNorth,                      "SeekNorth",                    N_("Snap View to North")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleCamera,                   "ToggleCamera",                 N_("Toggle Tracking Camera")));
	entries.emplace_back(KeyFunctionInfo(kf_addInGameOptions,               "addInGameOptions",             N_("Display In-Game Options")));
	entries.emplace_back(KeyFunctionInfo(kf_RadarZoomOut,                   "RadarZoomOut",                 N_("Zoom Radar Out")));
	entries.emplace_back(KeyFunctionInfo(kf_RadarZoomIn,                    "RadarZoomIn",                  N_("Zoom Radar In")));
	entries.emplace_back(KeyFunctionInfo(kf_ZoomIn,                         "ZoomIn",                       N_("Zoom In")));
	entries.emplace_back(KeyFunctionInfo(kf_ZoomOut,                        "ZoomOut",                      N_("Zoom Out")));
	entries.emplace_back(KeyFunctionInfo(kf_PitchForward,                   "PitchForward",                 N_("Pitch Forward")));
	entries.emplace_back(KeyFunctionInfo(kf_RotateLeft,                     "RotateLeft",                   N_("Rotate Left")));
	entries.emplace_back(KeyFunctionInfo(kf_ResetPitch,                     "ResetPitch",                   N_("Reset Pitch")));
	entries.emplace_back(KeyFunctionInfo(kf_RotateRight,                    "RotateRight",                  N_("Rotate Right")));
	entries.emplace_back(KeyFunctionInfo(kf_PitchBack,                      "PitchBack",                    N_("Pitch Back")));
	entries.emplace_back(KeyFunctionInfo(kf_RightOrderMenu,                 "RightOrderMenu",               N_("Orders Menu")));
	entries.emplace_back(KeyFunctionInfo(kf_SlowDown,                       "SlowDown",                     N_("Decrease Game Speed")));
	entries.emplace_back(KeyFunctionInfo(kf_SpeedUp,                        "SpeedUp",                      N_("Increase Game Speed")));
	entries.emplace_back(KeyFunctionInfo(kf_NormalSpeed,                    "NormalSpeed",                  N_("Reset Game Speed")));
	entries.emplace_back(KeyFunctionInfo(kf_FaceNorth,                      "FaceNorth",                    N_("View North")));
	entries.emplace_back(KeyFunctionInfo(kf_FaceSouth,                      "FaceSouth",                    N_("View South")));
	entries.emplace_back(KeyFunctionInfo(kf_FaceEast,                       "FaceEast",                     N_("View East")));
	entries.emplace_back(KeyFunctionInfo(kf_FaceWest,                       "FaceWest",                     N_("View West")));
	entries.emplace_back(KeyFunctionInfo(kf_JumpToResourceExtractor,        "JumpToResourceExtractor",      N_("View next Oil Derrick")));
	entries.emplace_back(KeyFunctionInfo(kf_JumpToRepairUnits,              "JumpToRepairUnits",            N_("View next Repair Unit")));
	entries.emplace_back(KeyFunctionInfo(kf_JumpToConstructorUnits,         "JumpToConstructorUnits",       N_("View next Truck")));
	entries.emplace_back(KeyFunctionInfo(kf_JumpToSensorUnits,              "JumpToSensorUnits",            N_("View next Sensor Unit")));
	entries.emplace_back(KeyFunctionInfo(kf_JumpToCommandUnits,             "JumpToCommandUnits",           N_("View next Commander")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleOverlays,                 "ToggleOverlays",               N_("Toggle Overlays")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleConsoleDrop,              "ToggleConsoleDrop",            N_("Toggle Console History ")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleTeamChat,                 "ToggleTeamChat",               N_("Toggle Team Chat History")));
	entries.emplace_back(KeyFunctionInfo(kf_RotateBuildingCW,               "RotateBuildingClockwise",      N_("Rotate Building Clockwise")));
	entries.emplace_back(KeyFunctionInfo(kf_RotateBuildingACW,              "RotateBuildingAnticlockwise",  N_("Rotate Building Anticlockwise")));
	// IN GAME MAPPINGS - Single key presses - ALL __DEBUG keymappings will be removed for master
	entries.emplace_back(KeyFunctionInfo(kf_CentreOnBase,                   "CentreOnBase",                 N_("Center View on HQ")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidAttackCease,            "SetDroidAttackCease",          N_("Hold Fire")));
	entries.emplace_back(KeyFunctionInfo(kf_JumpToUnassignedUnits,          "JumpToUnassignedUnits",        N_("View Unassigned Units")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidAttackReturn,           "SetDroidAttackReturn",         N_("Return Fire")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidAttackAtWill,           "SetDroidAttackAtWill",         N_("Fire at Will")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidMoveGuard,              "SetDroidMoveGuard",            N_("Guard Position")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidReturnToBase,           "SetDroidReturnToBase",         N_("Return to HQ")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidOrderHold,              "SetDroidOrderHold",            N_("Hold Position")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidRangeOptimum,           "SetDroidRangeOptimum",         N_("Optimum Range")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidRangeShort,             "SetDroidRangeShort",           N_("Short Range")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidMovePursue,             "SetDroidMovePursue",           N_("Pursue")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidMovePatrol,             "SetDroidMovePatrol",           N_("Patrol")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidGoForRepair,            "SetDroidGoForRepair",          N_("Return For Repair")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidOrderStop,              "SetDroidOrderStop",            N_("Stop Droid")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidGoToTransport,          "SetDroidGoToTransport",        N_("Go to Transport")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidRangeLong,              "SetDroidRangeLong",            N_("Long Range")));
	entries.emplace_back(KeyFunctionInfo(kf_SendGlobalMessage,              "SendGlobalMessage",            N_("Send Global Text Message")));
	entries.emplace_back(KeyFunctionInfo(kf_SendTeamMessage,                "SendTeamMessage",              N_("Send Team Text Message")));
	entries.emplace_back(KeyFunctionInfo(kf_AddHelpBlip,                    "AddHelpBlip",                  N_("Drop a beacon")));
	//
	entries.emplace_back(KeyFunctionInfo(kf_ToggleShadows,                  "ToggleShadows",                N_("Toggles shadows")));
	entries.emplace_back(KeyFunctionInfo(kf_toggleTrapCursor,               "toggleTrapCursor",             N_("Trap cursor")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleRadarTerrain,             "ToggleRadarTerrain",           N_("Toggle radar terrain")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleRadarAllyEnemy,           "ToggleRadarAllyEnemy",         N_("Toggle ally-enemy radar view")));
	entries.emplace_back(KeyFunctionInfo(kf_ShowMappings,                   "ShowMappings",                 N_("Show all keyboard mappings")));
	// Some extra non QWERTY mappings but functioning in same way
	entries.emplace_back(KeyFunctionInfo( kf_SetDroidRetreatMedium,         "SetDroidRetreatMedium",        N_("Retreat at Medium Damage")));
	entries.emplace_back(KeyFunctionInfo( kf_SetDroidRetreatHeavy,          "SetDroidRetreatHeavy",         N_("Retreat at Heavy Damage")));
	entries.emplace_back(KeyFunctionInfo( kf_SetDroidRetreatNever,          "SetDroidRetreatNever",         N_("Do or Die!")));
	// In game mappings - COMBO (CTRL + LETTER) presses
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllCombatUnits,           "SelectAllCombatUnits",         N_("Select all Combat Units")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllCyborgs,               "SelectAllCyborgs",             N_("Select all Cyborgs")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllDamaged,               "SelectAllDamaged",             N_("Select all Heavily Damaged Units")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllHalfTracked,           "SelectAllHalfTracked",         N_("Select all Half-tracks")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllHovers,                "SelectAllHovers",              N_("Select all Hovers")));
	entries.emplace_back(KeyFunctionInfo(kf_SetDroidRecycle,                "SetDroidRecycle",              N_("Return for Recycling")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllOnScreenUnits,         "SelectAllOnScreenUnits",       N_("Select all Units on Screen")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllTracked,               "SelectAllTracked",             N_("Select all Tracks")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllUnits,                 "SelectAllUnits",               N_("Select EVERY unit")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllVTOLs,                 "SelectAllVTOLs",               N_("Select all VTOLs")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllArmedVTOLs,            "SelectAllArmedVTOLs",          N_("Select all fully-armed VTOLs")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllWheeled,               "SelectAllWheeled",             N_("Select all Wheels")));
	entries.emplace_back(KeyFunctionInfo(kf_FrameRate,                      "FrameRate",                    N_("Show frame rate")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllSameType,              "SelectAllSameType",            N_("Select all units with the same components")));
	// In game mappings - COMBO (SHIFT + LETTER) presses
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllCombatCyborgs,         "SelectAllCombatCyborgs",       N_("Select all Combat Cyborgs")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllEngineers,             "SelectAllEngineers",           N_("Select all Engineers")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllLandCombatUnits,       "SelectAllLandCombatUnits",     N_("Select all Land Combat Units")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllMechanics,             "SelectAllMechanics",           N_("Select all Mechanics")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllTransporters,          "SelectAllTransporters",        N_("Select all Transporters")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllRepairTanks,           "SelectAllRepairTanks",         N_("Select all Repair Tanks")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllSensorUnits,           "SelectAllSensorUnits",         N_("Select all Sensor Units")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectAllTrucks,                "SelectAllTrucks",              N_("Select all Trucks")));
	// SELECT PLAYERS - DEBUG ONLY
	entries.emplace_back(KeyFunctionInfo(kf_SelectNextFactory,              "SelectNextFactory",            N_("Select next Factory")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectNextResearch,             "SelectNextResearch",           N_("Select next Research Facility")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectNextPowerStation,         "SelectNextPowerStation",       N_("Select next Power Generator")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectNextCyborgFactory,        "SelectNextCyborgFactory",      N_("Select next Cyborg Factory")));
	entries.emplace_back(KeyFunctionInfo(kf_SelectNextVTOLFactory,          "SelectNextVtolFactory",        N_("Select next VTOL Factory")));
	entries.emplace_back(KeyFunctionInfo(kf_JumpNextFactory,                "JumpNextFactory",              N_("Jump to next Factory")));
	entries.emplace_back(KeyFunctionInfo(kf_JumpNextResearch,               "JumpNextResearch",             N_("Jump to next Research Facility")));
	entries.emplace_back(KeyFunctionInfo(kf_JumpNextPowerStation,           "JumpNextPowerStation",         N_("Jump to next Power Generator")));
	entries.emplace_back(KeyFunctionInfo(kf_JumpNextCyborgFactory,          "JumpNextCyborgFactory",        N_("Jump to next Cyborg Factory")));
	entries.emplace_back(KeyFunctionInfo(kf_JumpNextVTOLFactory,            "JumpNextVtolFactory",          N_("Jump to next VTOL Factory")));
	// Debug options
	entries.emplace_back(KeyFunctionInfo(kf_ToggleDebugMappings,            "ToggleDebugMappings",          N_("Toggle Debug Mappings")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleShowPath,                 "ToggleShowPath",               N_("Toggle display of droid path")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleShowGateways,             "ToggleShowGateways",           N_("Toggle display of gateways")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleVisibility,               "ToggleVisibility",             N_("Toggle visibility")));
	entries.emplace_back(KeyFunctionInfo(kf_RaiseTile,                      "RaiseTile",                    N_("Raise tile height")));
	entries.emplace_back(KeyFunctionInfo(kf_LowerTile,                      "LowerTile",                    N_("Lower tile height")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleFog,                      "ToggleFog",                    N_("Toggles All fog")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleWeather,                  "ToggleWeather",                N_("Trigger some weather")));
	entries.emplace_back(KeyFunctionInfo(kf_TriFlip,                        "TriFlip",                      N_("Flip terrain triangle")));
	entries.emplace_back(KeyFunctionInfo(kf_PerformanceSample,              "PerformanceSample",            N_("Make a performance measurement sample")));
	entries.emplace_back(KeyFunctionInfo(kf_AllAvailable,                   "AllAvailable",                 N_("Make all items available")));
	entries.emplace_back(KeyFunctionInfo(kf_KillSelected,                   "KillSelected",                 N_("Kill Selected Unit(s)")));
	entries.emplace_back(KeyFunctionInfo(kf_ToggleGodMode,                  "ToggleGodMode",                N_("Toggle god Mode Status")));
	entries.emplace_back(KeyFunctionInfo(kf_ChooseOptions,                  "ChooseOptions",                N_("Display Options Screen")));
	entries.emplace_back(KeyFunctionInfo(kf_FinishResearch,                 "FinishResearch",               N_("Complete current research")));
	entries.emplace_back(KeyFunctionInfo(kf_RevealMapAtPos,                 "RevealMapAtPos",               N_("Reveal map at mouse position")));
	entries.emplace_back(KeyFunctionInfo(kf_TraceObject,                    "TraceObject",                  N_("Trace a game object")));

	return KeyFunctionInfoTable(entries);
}
static const KeyFunctionInfoTable keyFunctionInfoTable = initializeKeyFunctionInfoTable();

const std::vector<std::reference_wrapper<const KeyFunctionInfo>> allKeymapEntries()
{
	return keyFunctionInfoTable.allKeymapEntries();
}

KeyFunctionInfo const *keyFunctionInfoByFunction(void (*function)())
{
	return keyFunctionInfoTable.keyFunctionInfoByFunction(function);
}

KeyFunctionInfo const *keyFunctionInfoByName(std::string const &name)
{
	return keyFunctionInfoTable.keyFunctionInfoByName(name);
}

KeyMappingInputSource keyMappingSourceByName(std::string const& name)
{
	if (name == "default")
	{
		return KeyMappingInputSource::KEY_CODE;
	}
	else if (name == "mouse_key")
	{
		return KeyMappingInputSource::MOUSE_KEY_CODE;
	}
	else
	{
		debug(LOG_WZ, "Encountered invalid key mapping source name '%s', falling back to using 'default'", name.c_str());
		return KeyMappingInputSource::KEY_CODE;
	}
}

KeyMappingSlot keyMappingSlotByName(std::string const& name)
{
	if (name == "primary")
	{
		return KeyMappingSlot::PRIMARY;
	}
	else if (name == "secondary")
	{
		return KeyMappingSlot::SECONDARY;
	}
	else
	{
		debug(LOG_WZ, "Encountered invalid key mapping slot name '%s', falling back to using 'primary'", name.c_str());
		return KeyMappingSlot::PRIMARY;
	}
}

static bool keyAddDefaultMapping(KEY_STATUS status, KEY_CODE metaCode, KeyMappingInput input, KEY_ACTION action, void (*pKeyMapFunc)(), bool bForceDefaults, const KeyMappingSlot slot = KeyMappingSlot::PRIMARY); // forward-declare

// ----------------------------------------------------------------------------------
/*
	Here is where we assign functions to keys and to combinations of keys.
	these will be read in from a .cfg file customisable by the player from
	an in-game menu
*/
void keyInitMappings(bool bForceDefaults)
{
	keyMappings.clear();
	bMappingsSortOrderDirty = true;
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
	// FUNCTION KEY MAPPINGS F1 to F12
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F1,  KEYMAP_PRESSED, kf_ChooseManufacture,            bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F2,  KEYMAP_PRESSED, kf_ChooseResearch,               bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F3,  KEYMAP_PRESSED, kf_ChooseBuild,                  bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F4,  KEYMAP_PRESSED, kf_ChooseDesign,                 bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F5,  KEYMAP_PRESSED, kf_ChooseIntelligence,           bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS_PROCESS, KEY_IGNORE, KEY_F6,  KEYMAP_PRESSED, kf_ChooseCommand,                bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F7,  KEYMAP_PRESSED, kf_QuickSave,                    bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_LSHIFT, KEY_F7,  KEYMAP_PRESSED, kf_ToggleRadar,                  bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F8,  KEYMAP_PRESSED, kf_QuickLoad,                    bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_LSHIFT, KEY_F8,  KEYMAP_PRESSED, kf_ToggleConsole,                bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F9,  KEYMAP_PRESSED, kf_ToggleEnergyBars,             bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS,         KEY_IGNORE, KEY_F10, KEYMAP_PRESSED, kf_ScreenDump,                   bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F11, KEYMAP_PRESSED, kf_ToggleFormationSpeedLimiting, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_IGNORE, KEY_F12, KEYMAP_PRESSED, kf_MoveToLastMessagePos,         bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE,     KEY_LSHIFT, KEY_F12, KEYMAP_PRESSED, kf_ToggleSensorDisplay,          bForceDefaults) || didAdd;
	// ASSIGN GROUPS - Will create or replace the existing group
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_0, KEYMAP_PRESSED, kf_AssignGrouping_0, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_1, KEYMAP_PRESSED, kf_AssignGrouping_1, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_2, KEYMAP_PRESSED, kf_AssignGrouping_2, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_3, KEYMAP_PRESSED, kf_AssignGrouping_3, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_4, KEYMAP_PRESSED, kf_AssignGrouping_4, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_5, KEYMAP_PRESSED, kf_AssignGrouping_5, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_6, KEYMAP_PRESSED, kf_AssignGrouping_6, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_7, KEYMAP_PRESSED, kf_AssignGrouping_7, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_8, KEYMAP_PRESSED, kf_AssignGrouping_8, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_9, KEYMAP_PRESSED, kf_AssignGrouping_9, bForceDefaults) || didAdd;
	// ADD TO GROUPS - Will add the selected units to the group
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_0, KEYMAP_PRESSED, kf_AddGrouping_0, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_1, KEYMAP_PRESSED, kf_AddGrouping_1, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_2, KEYMAP_PRESSED, kf_AddGrouping_2, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_3, KEYMAP_PRESSED, kf_AddGrouping_3, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_4, KEYMAP_PRESSED, kf_AddGrouping_4, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_5, KEYMAP_PRESSED, kf_AddGrouping_5, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_6, KEYMAP_PRESSED, kf_AddGrouping_6, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_7, KEYMAP_PRESSED, kf_AddGrouping_7, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_8, KEYMAP_PRESSED, kf_AddGrouping_8, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_9, KEYMAP_PRESSED, kf_AddGrouping_9, bForceDefaults) || didAdd;
	// SELECT GROUPS - Will jump to the group as well as select if group is ALREADY selected
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_0, KEYMAP_PRESSED, kf_SelectGrouping_0, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_1, KEYMAP_PRESSED, kf_SelectGrouping_1, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_2, KEYMAP_PRESSED, kf_SelectGrouping_2, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_3, KEYMAP_PRESSED, kf_SelectGrouping_3, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_4, KEYMAP_PRESSED, kf_SelectGrouping_4, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_5, KEYMAP_PRESSED, kf_SelectGrouping_5, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_6, KEYMAP_PRESSED, kf_SelectGrouping_6, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_7, KEYMAP_PRESSED, kf_SelectGrouping_7, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_8, KEYMAP_PRESSED, kf_SelectGrouping_8, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_9, KEYMAP_PRESSED, kf_SelectGrouping_9, bForceDefaults) || didAdd;
	// SELECT COMMANDER - Will jump to the group as well as select if group is ALREADY selected
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_0, KEYMAP_PRESSED, kf_SelectCommander_0, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_1, KEYMAP_PRESSED, kf_SelectCommander_1, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_2, KEYMAP_PRESSED, kf_SelectCommander_2, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_3, KEYMAP_PRESSED, kf_SelectCommander_3, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_4, KEYMAP_PRESSED, kf_SelectCommander_4, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_5, KEYMAP_PRESSED, kf_SelectCommander_5, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_6, KEYMAP_PRESSED, kf_SelectCommander_6, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_7, KEYMAP_PRESSED, kf_SelectCommander_7, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_8, KEYMAP_PRESSED, kf_SelectCommander_8, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT, KEY_9, KEYMAP_PRESSED, kf_SelectCommander_9, bForceDefaults) || didAdd;
	// MULTIPLAYER
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KPENTER, KEYMAP_PRESSED, kf_addMultiMenu, bForceDefaults) || didAdd;
	// GAME CONTROLS - Moving around, zooming in, rotating etc
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_UPARROW,           KEYMAP_DOWN,    kf_CameraUp,               bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_DOWNARROW,         KEYMAP_DOWN,    kf_CameraDown,             bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_RIGHTARROW,        KEYMAP_DOWN,    kf_CameraRight,            bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_LEFTARROW,         KEYMAP_DOWN,    kf_CameraLeft,             bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_BACKSPACE,         KEYMAP_PRESSED, kf_SeekNorth,              bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_SPACE,             KEYMAP_PRESSED, kf_ToggleCamera,           bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ALWAYS,     KEY_IGNORE, KEY_ESC,               KEYMAP_PRESSED, kf_addInGameOptions,       bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_MINUS,             KEYMAP_PRESSED, kf_RadarZoomOut,           bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_EQUALS,            KEYMAP_PRESSED, kf_RadarZoomIn,            bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_PLUS,           KEYMAP_DOWN,    kf_ZoomIn,                 bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, MOUSE_KEY_CODE::MOUSE_WUP, KEYMAP_PRESSED, kf_ZoomIn,             bForceDefaults, KeyMappingSlot::SECONDARY) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_MINUS,          KEYMAP_DOWN,    kf_ZoomOut,                bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, MOUSE_KEY_CODE::MOUSE_WDN, KEYMAP_PRESSED, kf_ZoomOut,            bForceDefaults, KeyMappingSlot::SECONDARY) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_2,              KEYMAP_DOWN,    kf_PitchForward,           bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_4,              KEYMAP_DOWN,    kf_RotateLeft,             bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_5,              KEYMAP_DOWN,    kf_ResetPitch,             bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_6,              KEYMAP_DOWN,    kf_RotateRight,            bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_8,              KEYMAP_DOWN,    kf_PitchBack,              bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_0,              KEYMAP_PRESSED, kf_RightOrderMenu,         bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_MINUS,             KEYMAP_PRESSED, kf_SlowDown,               bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_EQUALS,            KEYMAP_PRESSED, kf_SpeedUp,                bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_BACKSPACE,         KEYMAP_PRESSED, kf_NormalSpeed,            bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_UPARROW,           KEYMAP_PRESSED, kf_FaceNorth,              bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_DOWNARROW,         KEYMAP_PRESSED, kf_FaceSouth,              bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_LEFTARROW,         KEYMAP_PRESSED, kf_FaceEast,               bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_RIGHTARROW,        KEYMAP_PRESSED, kf_FaceWest,               bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_KP_STAR,           KEYMAP_PRESSED, kf_JumpToResourceExtractor,bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpToRepairUnits,      bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpToConstructorUnits, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpToSensorUnits,      bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpToCommandUnits,     bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_TAB,               KEYMAP_PRESSED, kf_ToggleOverlays,         bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_BACKQUOTE,         KEYMAP_PRESSED, kf_ToggleConsoleDrop,      bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_BACKQUOTE,         KEYMAP_PRESSED, kf_ToggleTeamChat,         bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_RotateBuildingCW,       bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_RotateBuildingACW,      bForceDefaults) || didAdd;
	// IN GAME MAPPINGS - Single key presses - ALL __DEBUG keymappings will be removed for master
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_B,      KEYMAP_PRESSED, kf_CentreOnBase,          bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_C,      KEYMAP_PRESSED, kf_SetDroidAttackCease,   bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_D,      KEYMAP_PRESSED, kf_JumpToUnassignedUnits, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_E,      KEYMAP_PRESSED, kf_SetDroidAttackReturn,  bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_F,      KEYMAP_PRESSED, kf_SetDroidAttackAtWill,  bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_G,      KEYMAP_PRESSED, kf_SetDroidMoveGuard,     bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_H,      KEYMAP_PRESSED, kf_SetDroidReturnToBase,  bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_H,      KEYMAP_PRESSED, kf_SetDroidOrderHold,     bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_I,      KEYMAP_PRESSED, kf_SetDroidRangeOptimum,  bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_O,      KEYMAP_PRESSED, kf_SetDroidRangeShort,    bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_P,      KEYMAP_PRESSED, kf_SetDroidMovePursue,    bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_Q,      KEYMAP_PRESSED, kf_SetDroidMovePatrol,    bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_R,      KEYMAP_PRESSED, kf_SetDroidGoForRepair,   bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_S,      KEYMAP_PRESSED, kf_SetDroidOrderStop,     bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_T,      KEYMAP_PRESSED, kf_SetDroidGoToTransport, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_U,      KEYMAP_PRESSED, kf_SetDroidRangeLong,     bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_RETURN, KEYMAP_PRESSED, kf_SendGlobalMessage,     bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_RETURN, KEYMAP_PRESSED, kf_SendTeamMessage,       bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT,   KEY_H,      KEYMAP_PRESSED, kf_AddHelpBlip,           bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT,   KEY_S,   KEYMAP_PRESSED,  kf_ToggleShadows,        bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LALT,   KEY_T,   KEYMAP_PRESSED,  kf_toggleTrapCursor,     bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL,  KEY_TAB, KEYMAP_PRESSED,  kf_ToggleRadarTerrain,   bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_TAB, KEYMAP_PRESSED,  kf_ToggleRadarAllyEnemy, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_M,   KEYMAP_PRESSED,  kf_ShowMappings,         bForceDefaults) || didAdd;
	// Some extra non QWERTY mappings but functioning in same way
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_COMMA,        KEYMAP_PRESSED, kf_SetDroidRetreatMedium, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_FULLSTOP,     KEYMAP_PRESSED, kf_SetDroidRetreatHeavy,  bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, KEY_FORWARDSLASH, KEYMAP_PRESSED, kf_SetDroidRetreatNever,  bForceDefaults) || didAdd;
	// In game mappings - COMBO (CTRL + LETTER) presses.
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_A, KEYMAP_PRESSED, kf_SelectAllCombatUnits,   bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_C, KEYMAP_PRESSED, kf_SelectAllCyborgs,       bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_D, KEYMAP_PRESSED, kf_SelectAllDamaged,       bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_F, KEYMAP_PRESSED, kf_SelectAllHalfTracked,   bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_H, KEYMAP_PRESSED, kf_SelectAllHovers,        bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_R, KEYMAP_PRESSED, kf_SetDroidRecycle,        bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_S, KEYMAP_PRESSED, kf_SelectAllOnScreenUnits, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_T, KEYMAP_PRESSED, kf_SelectAllTracked,       bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_U, KEYMAP_PRESSED, kf_SelectAllUnits,         bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_V, KEYMAP_PRESSED, kf_SelectAllVTOLs,         bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_V, KEYMAP_PRESSED, kf_SelectAllArmedVTOLs,   bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_W, KEYMAP_PRESSED, kf_SelectAllWheeled,       bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG,     KEY_LCTRL, KEY_Y, KEYMAP_PRESSED, kf_FrameRate,              bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LCTRL, KEY_Z, KEYMAP_PRESSED, kf_SelectAllSameType,      bForceDefaults) || didAdd;
	// In game mappings - COMBO (SHIFT + LETTER) presses.
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_C, KEYMAP_PRESSED, kf_SelectAllCombatCyborgs,   bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_E, KEYMAP_PRESSED, kf_SelectAllEngineers,       bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_G, KEYMAP_PRESSED, kf_SelectAllLandCombatUnits, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_M, KEYMAP_PRESSED, kf_SelectAllMechanics,       bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_P, KEYMAP_PRESSED, kf_SelectAllTransporters,    bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_R, KEYMAP_PRESSED, kf_SelectAllRepairTanks,     bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_S, KEYMAP_PRESSED, kf_SelectAllSensorUnits,     bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_LSHIFT, KEY_T, KEYMAP_PRESSED, kf_SelectAllTrucks,          bForceDefaults) || didAdd;
	// SELECT PLAYERS - DEBUG ONLY
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_SelectNextFactory,       bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_SelectNextResearch,      bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_SelectNextPowerStation,  bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_SelectNextCyborgFactory, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_SelectNextVTOLFactory,   bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpNextFactory,         bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpNextResearch,        bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpNextPowerStation,    bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpNextCyborgFactory,   bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP_ASSIGNABLE, KEY_IGNORE, (KEY_CODE)KEY_MAXSCAN, KEYMAP_PRESSED, kf_JumpNextVTOLFactory,     bForceDefaults) || didAdd;
	// Debug options
	didAdd = keyAddDefaultMapping(KEYMAP___HIDE, KEY_LSHIFT, KEY_BACKSPACE, KEYMAP_PRESSED, kf_ToggleDebugMappings, bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_M,         KEYMAP_PRESSED, kf_ToggleShowPath,      bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_E,         KEYMAP_PRESSED, kf_ToggleShowGateways,  bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_IGNORE, KEY_V,         KEYMAP_PRESSED, kf_ToggleVisibility,    bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_IGNORE, KEY_W,         KEYMAP_DOWN,    kf_RaiseTile,           bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_IGNORE, KEY_A,         KEYMAP_DOWN,    kf_LowerTile,           bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_J,         KEYMAP_PRESSED, kf_ToggleFog,           bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_Q,         KEYMAP_PRESSED, kf_ToggleWeather,       bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_IGNORE, KEY_K,         KEYMAP_PRESSED, kf_TriFlip,             bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_K,         KEYMAP_PRESSED, kf_PerformanceSample,   bForceDefaults) || didAdd;
	// These ones are necessary for debugging
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LALT,   KEY_A, KEYMAP_PRESSED, kf_AllAvailable,      bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LALT,   KEY_K, KEYMAP_PRESSED, kf_KillSelected,      bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_G, KEYMAP_PRESSED, kf_ToggleGodMode,     bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_O, KEYMAP_PRESSED, kf_ChooseOptions,     bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_X, KEYMAP_PRESSED, kf_FinishResearch,    bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LSHIFT, KEY_W, KEYMAP_PRESSED, kf_RevealMapAtPos,    bForceDefaults) || didAdd;
	didAdd = keyAddDefaultMapping(KEYMAP__DEBUG, KEY_LCTRL,  KEY_L, KEYMAP_PRESSED, kf_TraceObject,       bForceDefaults) || didAdd;

	if (didAdd)
	{
		saveKeyMap();	// save out the default key mappings.
	}
}

// ----------------------------------------------------------------------------------
/* Adds a new mapping to the list */
KEY_MAPPING *keyAddMapping(KEY_STATUS status, KEY_CODE metaCode, KeyMappingInput input, KEY_ACTION action, void (*pKeyMapFunc)(), const KeyMappingSlot slot)
{
	/* Make sure the meta key is the left variant */
	KEY_CODE leftMetaCode = metaCode;
	if (metaCode == KEY_RCTRL)
	{
		leftMetaCode = KEY_LCTRL;
	}
	else if (metaCode == KEY_RALT)
	{
		leftMetaCode = KEY_LALT;
	}
	else if (metaCode == KEY_RSHIFT)
	{
		leftMetaCode = KEY_LSHIFT;
	}
	else if (metaCode == KEY_RMETA)
	{
		leftMetaCode = KEY_LMETA;
	}

	/* Create the mapping as the last element in the list */
	keyMappings.push_back({
		pKeyMapFunc,
		status,
		gameTime,
		leftMetaCode,
		input,
		action,
		slot
	});

	/* Invalidate the sorting order and return the newly created mapping */
	bMappingsSortOrderDirty = true;
	return &keyMappings.back();
}

// ----------------------------------------------------------------------------------
/* Returns a pointer to a mapping if it exists - NULL otherwise */
KEY_MAPPING *keyFindMapping(const KEY_CODE metaCode, const KeyMappingInput input, const KeyMappingSlot slot)
{
	auto mapping = std::find_if(keyMappings.begin(), keyMappings.end(), [metaCode, input, slot](KEY_MAPPING const &mapping) {
		// If slot is set to LAST, find any mapping regardless of slot
		const bool slotMatches = slot == KeyMappingSlot::LAST || mapping.slot == slot;
		return mapping.metaKeyCode == metaCode && mapping.input == input && slotMatches;
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
		bMappingsSortOrderDirty = true;
		return true;
	}
	return false;
}

static bool keyAddDefaultMapping(KEY_STATUS status, KEY_CODE metaCode, KeyMappingInput input, KEY_ACTION action, void (*pKeyMapFunc)(), bool bForceDefaults, const KeyMappingSlot slot)
{
	KEY_MAPPING *psMapping = keyGetMappingFromFunction(pKeyMapFunc, slot);
	if (!bForceDefaults && psMapping != nullptr)
	{
		// Not forcing defaults, and there is already a mapping entry for this function with this slot
		return false;
	}

	// Otherwise, force / overwrite with the default
	if (!bForceDefaults)
	{
		if (const auto entry = keyFunctionInfoTable.keyFunctionInfoByFunction(pKeyMapFunc))
		{
			debug(LOG_INFO, "Adding missing keymap entry: %s", entry->displayName.c_str());
		}
	}
	if (psMapping)
	{
		// Remove any existing mapping for this function
		keyRemoveMappingPt(psMapping);
		psMapping = nullptr;
	}
	if (!bForceDefaults)
	{
		// Clear the keys from any other mappings
		clearKeyMappingIfExists(metaCode, input);
	}

	// Set default key mapping
	psMapping = keyAddMapping(status, metaCode, input, action, pKeyMapFunc, slot);
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
				keyAddMapping(KEYMAP_ALWAYS, KEY_LSHIFT, KeyMappingInput((KEY_CODE)qKey), KEYMAP_PRESSED, kf_JumpToMapMarker);
			aquired = true;

			/* Store away the position and view angle */
			qwertyKeyMappings[tableEntry].xPos = playerPos.p.x;
			qwertyKeyMappings[tableEntry].yPos = playerPos.p.z;
			qwertyKeyMappings[tableEntry].spin = playerPos.r.y;
		}
	}
	return aquired;
}

// ----------------------------------------------------------------------------------
/* allows checking if mapping should currently be ignored in keyProcessMappings */
static bool isIgnoredMapping(const bool bExclude, const bool bAllowMouseWheelEvents, const KEY_MAPPING& mapping)
{
	if (bExclude && mapping.status != KEYMAP_ALWAYS_PROCESS)
	{
		return true;
	}

	if (mapping.input.is(KEY_CODE::KEY_MAXSCAN))
	{
		return true;
	}

	if (!bAllowMouseWheelEvents && (mapping.input.is(MOUSE_KEY_CODE::MOUSE_WUP) || mapping.input.is(MOUSE_KEY_CODE::MOUSE_WDN)))
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
void keyProcessMappings(const bool bExclude, const bool bAllowMouseWheelEvents)
{
	/* Bomb out if there are none */
	if (keyMappings.empty())
	{
		return;
	}

	/* Jump out if we've got a new mapping */
	(void) checkQwertyKeys();

	/* If mappings have been updated, sort the mappings by whether or not they have meta keys */
	if (bMappingsSortOrderDirty)
	{
		keyMappings.sort([](const KEY_MAPPING& a, const KEY_MAPPING& b) {
			// Sort by meta. This causes all mappings with meta to be checked before non-meta mappings,
			// avoiding having to check for meta-conflicts in the processing loop. (e.g. if we should execute
			// a mapping with right arrow key, depending on if another binding on shift+right-arrow is executed
			// or not). In other words, if any mapping with meta is executed, it will consume the respective input,
			// preventing any non-meta mappings with the same input from being executed.
			return a.hasMeta() && !b.hasMeta();
		});
		bMappingsSortOrderDirty = false;
	}

	std::unordered_set<KeyMappingInput, KeyMappingInput::Hash> consumedInputs;

	/* Run through all sorted mappings */
	for (const KEY_MAPPING& keyToProcess : keyMappings)
	{
		/* Skip inappropriate ones when necessary */
		if (isIgnoredMapping(bExclude, bAllowMouseWheelEvents, keyToProcess))
		{
			continue;
		}

		/* Skip if the input is already consumed. Handles skips for meta-conflicts */
		const auto bIsAlreadyConsumed = consumedInputs.find(keyToProcess.input) != consumedInputs.end();
		if (bIsAlreadyConsumed)
		{
			continue;
		}

		/* Execute the action if mapping was hit */
		if (keyToProcess.isActivated())
		{
			if (keyToProcess.hasMeta())
			{
				lastMetaKey = keyToProcess.metaKeyCode;
			}

			lastInput = keyToProcess.input;
			keyToProcess.function();
			consumedInputs.insert(keyToProcess.input);
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
KeyMappingInput getLastInput()
{
	return lastInput;
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
bool clearKeyMappingIfExists(const KEY_CODE metaCode, const KeyMappingInput input)
{
	/* Find any mapping with same keys */
	if (KEY_MAPPING *psMapping = keyFindMapping(metaCode, input))
	{
		/* Not all can be remapped */
		if (psMapping->status != KEY_STATUS::KEYMAP_ALWAYS || psMapping->status == KEY_STATUS::KEYMAP_ALWAYS_PROCESS)
		{
			psMapping->metaKeyCode = KEY_CODE::KEY_IGNORE;
			psMapping->input = KEY_CODE::KEY_MAXSCAN;
			return true;
		}
	}

	return false;
}
