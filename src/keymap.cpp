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
InputContexts InputContext::contexts = InputContexts();

static const unsigned int MAX_ICONTEXT_PRIORITY = std::numeric_limits<unsigned int>::max();
const InputContext InputContext::ALWAYS_ACTIVE = { MAX_ICONTEXT_PRIORITY, InputContext::State::ACTIVE,    N_("Global Hotkeys") };
const InputContext InputContext::BACKGROUND    = { 0,                     InputContext::State::ACTIVE,    N_("Other Hotkeys") };
const InputContext InputContext::GAMEPLAY      = { 1,                     InputContext::State::ACTIVE,    N_("Gameplay") };
const InputContext InputContext::RADAR         = { { 2, 0 },              InputContext::State::ACTIVE,    N_("Radar") };
const InputContext InputContext::__DEBUG       = { MAX_ICONTEXT_PRIORITY, InputContext::State::INACTIVE,  N_("Debug") };

static unsigned int inputCtxIndexCounter = 0;
InputContext::InputContext(const ContextPriority priority, const State initialState, const char* const displayName)
	: priority(priority)
	, index(inputCtxIndexCounter++)
	, displayName(displayName)
	, defaultState(initialState)
{
	contexts.push_back(*this);
}

const InputContexts InputContext::getAllContexts()
{
	return contexts;
}

void InputManager::setContextState(const InputContext& context, const InputContext::State newState)
{
	contextStates[context.index] = newState;
	bMappingsSortOrderDirty = true;
}

bool InputManager::isContextActive(const InputContext& context) const
{
	const auto state = contextStates[context.index];
	return state != InputContext::State::INACTIVE;
}

unsigned int InputManager::getContextPriority(const InputContext& context) const
{
	switch (contextStates[context.index])
	{
	case InputContext::State::PRIORITIZED:
		return context.priority.prioritized;
	case InputContext::State::ACTIVE:
		return context.priority.active;
	case InputContext::State::INACTIVE:
	default:
		return 0;
	}
}

const std::string InputContext::getDisplayName() const
{
	return displayName;
}

bool operator==(const InputContext& lhs, const InputContext& rhs)
{
	return lhs.index == rhs.index;
}

bool operator!=(const InputContext& lhs, const InputContext& rhs)
{
	return !(lhs == rhs);
}

KeyFunctionInfo::KeyFunctionInfo(
	const InputContext& context,
	const KeyMappingType type,
	const MappableFunction function,
	const std::string name,
	const std::string displayName
)
	: KeyFunctionInfo(context, type, function, name, displayName, {})
{
}

KeyFunctionInfo::KeyFunctionInfo(
	const InputContext& context,
	const KeyMappingType type,
	const MappableFunction function,
	const std::string name,
	const std::string displayName,
	const std::vector<std::pair<KeyMappingSlot, KeyCombination>> defaultMappings
)
	: context(context)
	, type(type)
	, function(function)
	, name(name)
	, displayName(displayName)
	, defaultMappings(defaultMappings)
{
}

KeyFunctionInfo::KeyFunctionInfo(
	const InputContext& context,
	const KeyMappingType type,
	const MappableFunction function,
	const std::string saveId
)
	: context(context)
	, type(type)
	, function(function)
	, name(name)
	, displayName("")
	, defaultMappings({})
{
}

bool operator==(const KeyMapping& lhs, const KeyMapping& rhs)
{
	return lhs.input == rhs.input
		&& lhs.metaKeyCode == rhs.metaKeyCode
		&& lhs.action == rhs.action && lhs.slot == rhs.slot
		&& &lhs.info == &rhs.info; // Assume infos are immutable with only one copy existing at a time.
}

bool operator!=(const KeyMapping& lhs, const KeyMapping& rhs)
{
	return !(lhs == rhs);
}

void InputManager::resetContextStates()
{
	const auto contexts = InputContext::getAllContexts();
	contextStates = std::vector<InputContext::State>(contexts.size(), InputContext::State::INACTIVE);
	for (const InputContext& context : contexts)
	{
		contextStates[context.index] = context.defaultState;
	}
	bMappingsSortOrderDirty = true;
}

void InputManager::makeAllContextsInactive()
{
	for (const InputContext& context : InputContext::getAllContexts())
	{
		if (context != InputContext::ALWAYS_ACTIVE)
		{
			setContextState(context, InputContext::State::INACTIVE);
		}
	}
	bMappingsSortOrderDirty = true;
}

KeyMapping& InputManager::addMapping(const KEY_CODE meta, const KeyMappingInput input, const KeyAction action, const KeyFunctionInfo& info, const KeyMappingSlot slot)
{
	/* Make sure the meta key is the left variant */
	KEY_CODE leftMeta = meta;
	if (meta == KEY_RCTRL)
	{
		leftMeta = KEY_LCTRL;
	}
	else if (meta == KEY_RALT)
	{
		leftMeta = KEY_LALT;
	}
	else if (meta == KEY_RSHIFT)
	{
		leftMeta = KEY_LSHIFT;
	}
	else if (meta == KEY_RMETA)
	{
		leftMeta = KEY_LMETA;
	}

	/* Create the mapping as the last element in the list */
	keyMappings.push_back({
		info,
		gameTime,
		leftMeta,
		input,
		action,
		slot
	});

	/* Invalidate the sorting order and return the newly created mapping */
	bMappingsSortOrderDirty = true;
	return keyMappings.back();
}

nonstd::optional<std::reference_wrapper<KeyMapping>> InputManager::getMapping(const KeyFunctionInfo& info, const KeyMappingSlot slot)
{
	auto mapping = std::find_if(keyMappings.begin(), keyMappings.end(), [&info, slot](const KeyMapping& mapping) {
		return mapping.info.name == info.name && mapping.slot == slot;
	});
	if (mapping != keyMappings.end())
	{
		return *mapping;
	}

	return nonstd::nullopt;
}

std::vector<std::reference_wrapper<KeyMapping>> InputManager::findMappingsForInput(const KEY_CODE meta, const KeyMappingInput input)
{
	std::vector<std::reference_wrapper<KeyMapping>> matches;
	for (KeyMapping& mapping : keyMappings)
	{
		if (mapping.metaKeyCode == meta && mapping.input == input)
		{
			matches.push_back(mapping);
		}
	}

	return matches;
}

std::vector<KeyMapping> InputManager::removeConflictingMappings(const KEY_CODE meta, const KeyMappingInput input, const InputContext context)
{
	/* Find any mapping with same keys */
	const auto matches = findMappingsForInput(meta, input);
	std::vector<KeyMapping> conflicts;
	for (KeyMapping& mapping : matches)
	{
		/* Clear only if the mapping is for an assignable binding. Do not clear if there is no conflict (different context) */
		const bool bConflicts = mapping.info.context == context;
		if (mapping.info.type == KeyMappingType::ASSIGNABLE && bConflicts)
		{
			conflicts.push_back(mapping);
			removeMapping(mapping);
		}
	}

	return conflicts;
}

void InputManager::shutdown()
{
	keyMappings.clear();
}

void InputManager::clearAssignableMappings()
{
	keyMappings.remove_if([](const KeyMapping& mapping) {
		return mapping.info.type == KeyMappingType::ASSIGNABLE;
	});
}

const std::list<KeyMapping> InputManager::getAllMappings() const
{
	return keyMappings;
}

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
static bool isCombination(const KeyMapping& mapping)
{
	return mapping.metaKeyCode != KEY_CODE::KEY_IGNORE;
}

static bool isActiveSingleKey(const KeyMapping& mapping)
{
	switch (mapping.action)
	{
	case KeyAction::PRESSED:
		return mapping.input.isPressed();
	case KeyAction::DOWN:
		return mapping.input.isDown();
	case KeyAction::RELEASED:
		return mapping.input.isReleased();
	default:
		debug(LOG_WARNING, "Unknown key action (action code %u) while processing keymap.", (unsigned int)mapping.action);
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

static bool isActiveCombination(const KeyMapping& mapping)
{
	ASSERT(mapping.hasMeta(), "isActiveCombination called for non-meta key mapping!");

	const bool bSubKeyIsPressed = mapping.input.isPressed();
	const bool bMetaIsDown = keyDown(mapping.metaKeyCode);

	const auto altMeta = getAlternativeForMetaKey(mapping.metaKeyCode);
	const bool bHasAlt = altMeta != KEY_IGNORE;
	const bool bAltMetaIsDown = bHasAlt && keyDown(altMeta);

	return bSubKeyIsPressed && (bMetaIsDown || bAltMetaIsDown);
}

bool KeyMapping::isActivated() const
{
	return isCombination(*this)
		? isActiveCombination(*this)
		: isActiveSingleKey(*this);
}

bool KeyMapping::hasMeta() const
{
	return metaKeyCode != KEY_CODE::KEY_IGNORE;
}

bool KeyMapping::toString(char* pOutStr) const
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
	KeyMapping	*psMapping;
	UDWORD	xPos, yPos;
	SDWORD	spin;
};
static	KEYMAP_MARKER	qwertyKeyMappings[NUM_QWERTY_KEYS];


static bool bDoingDebugMappings = false;
static bool bWantDebugMappings[MAX_PLAYERS] = {false};
// ----------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------
/* Last meta and sub key that were recorded */
static KEY_CODE	lastMetaKey;
static KeyMappingInput lastInput;

// ----------------------------------------------------------------------------------
class KeyFunctionInfoTable {
public:
	KeyFunctionInfoTable(std::vector<KeyFunctionInfo>& items)
		: ordered_list(std::move(items))
	{
		for (size_t i = 0; i < ordered_list.size(); ++i)
		{
			name_to_index_map[ordered_list[i].name] = i;
		}
	}
public:
	nonstd::optional<std::reference_wrapper<const KeyFunctionInfo>> keyFunctionInfoByName(std::string const &name) const
	{
		auto it = name_to_index_map.find(name);
		if (it != name_to_index_map.end()) {
			return ordered_list[it->second];
		}
		return nonstd::nullopt;
	}

	const KeyFunctionEntries allKeyFunctionEntries() const
	{
		KeyFunctionEntries entries;
		for (auto const& keyFunctionInfo : ordered_list)
		{
			entries.push_back(keyFunctionInfo);
		}
		return entries;
	}
private:
	const std::vector<KeyFunctionInfo> ordered_list;
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
	entries.emplace_back(KeyFunctionInfo(InputContext::ALWAYS_ACTIVE,   KeyMappingType::FIXED,       kf_ChooseManufacture,                                          "ChooseManufacture",            N_("Manufacture"),                                  {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_F1,           KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::ALWAYS_ACTIVE,   KeyMappingType::FIXED,       kf_ChooseResearch,                                             "ChooseResearch",               N_("Research"),                                     {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_F2,           KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::ALWAYS_ACTIVE,   KeyMappingType::FIXED,       kf_ChooseBuild,                                                "ChooseBuild",                  N_("Build"),                                        {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_F3,           KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::ALWAYS_ACTIVE,   KeyMappingType::FIXED,       kf_ChooseDesign,                                               "ChooseDesign",                 N_("Design"),                                       {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_F4,           KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::ALWAYS_ACTIVE,   KeyMappingType::FIXED,       kf_ChooseIntelligence,                                         "ChooseIntelligence",           N_("Intelligence Display"),                         {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_F5,           KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::ALWAYS_ACTIVE,   KeyMappingType::FIXED,       kf_ChooseCommand,                                              "ChooseCommand",                N_("Commanders"),                                   {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_F6,           KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_QuickSave,                                                  "QuickSave",                    N_("QuickSave"),                                    {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_F7,           KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_ToggleRadar,                                                "ToggleRadar",                  N_("Toggle Radar"),                                 {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_F7,           KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_QuickLoad,                                                  "QuickLoad",                    N_("QuickLoad"),                                    {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_F8,           KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_ToggleConsole,                                              "ToggleConsole",                N_("Toggle Console Display"),                       {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_F8,           KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_ToggleEnergyBars,                                           "ToggleEnergyBars",             N_("Toggle Damage Bars On/Off"),                    {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_F9,           KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::BACKGROUND,      KeyMappingType::FIXED,       kf_ScreenDump,                                                 "ScreenDump",                   N_("Take Screen Shot"),                             {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_F10,          KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_ToggleFormationSpeedLimiting,                               "ToggleFormationSpeedLimiting", N_("Toggle Formation Speed Limiting"),              {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_F11,          KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_MoveToLastMessagePos,                                       "MoveToLastMessagePos",         N_("View Location of Previous Message"),            {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_F12,          KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_ToggleSensorDisplay,                                        "ToggleSensorDisplay",          N_("Toggle Sensor display"),                        {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_F12,          KeyAction::PRESSED}}}));
	// ASSIGN GROUPS
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AssignGrouping_N(0),                                        "AssignGrouping_0",             N_("Assign Group 0"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_0,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AssignGrouping_N(1),                                        "AssignGrouping_1",             N_("Assign Group 1"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_1,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AssignGrouping_N(2),                                        "AssignGrouping_2",             N_("Assign Group 2"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_2,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AssignGrouping_N(3),                                        "AssignGrouping_3",             N_("Assign Group 3"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_3,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AssignGrouping_N(4),                                        "AssignGrouping_4",             N_("Assign Group 4"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_4,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AssignGrouping_N(5),                                        "AssignGrouping_5",             N_("Assign Group 5"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_5,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AssignGrouping_N(6),                                        "AssignGrouping_6",             N_("Assign Group 6"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_6,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AssignGrouping_N(7),                                        "AssignGrouping_7",             N_("Assign Group 7"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_7,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AssignGrouping_N(8),                                        "AssignGrouping_8",             N_("Assign Group 8"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_8,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AssignGrouping_N(9),                                        "AssignGrouping_9",             N_("Assign Group 9"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_9,            KeyAction::PRESSED}}}));
	// ADD TO GROUP
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AddGrouping_N(0),                                           "AddGrouping_0",                N_("Add to Group 0"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_0,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AddGrouping_N(1),                                           "AddGrouping_1",                N_("Add to Group 1"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_1,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AddGrouping_N(2),                                           "AddGrouping_2",                N_("Add to Group 2"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_2,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AddGrouping_N(3),                                           "AddGrouping_3",                N_("Add to Group 3"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_3,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AddGrouping_N(4),                                           "AddGrouping_4",                N_("Add to Group 4"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_4,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AddGrouping_N(5),                                           "AddGrouping_5",                N_("Add to Group 5"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_5,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AddGrouping_N(6),                                           "AddGrouping_6",                N_("Add to Group 6"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_6,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AddGrouping_N(7),                                           "AddGrouping_7",                N_("Add to Group 7"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_7,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AddGrouping_N(8),                                           "AddGrouping_8",                N_("Add to Group 8"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_8,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AddGrouping_N(9),                                           "AddGrouping_9",                N_("Add to Group 9"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_9,            KeyAction::PRESSED}}}));
	// SELECT GROUPS - Will jump to the group as well as select if group is ALREADY selected
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectGrouping_N(0),                                        "SelectGrouping_0",             N_("Select Group 0"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_0,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectGrouping_N(1),                                        "SelectGrouping_1",             N_("Select Group 1"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_1,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectGrouping_N(2),                                        "SelectGrouping_2",             N_("Select Group 2"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_2,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectGrouping_N(3),                                        "SelectGrouping_3",             N_("Select Group 3"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_3,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectGrouping_N(4),                                        "SelectGrouping_4",             N_("Select Group 4"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_4,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectGrouping_N(5),                                        "SelectGrouping_5",             N_("Select Group 5"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_5,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectGrouping_N(6),                                        "SelectGrouping_6",             N_("Select Group 6"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_6,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectGrouping_N(7),                                        "SelectGrouping_7",             N_("Select Group 7"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_7,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectGrouping_N(8),                                        "SelectGrouping_8",             N_("Select Group 8"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_8,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectGrouping_N(9),                                        "SelectGrouping_9",             N_("Select Group 9"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_9,            KeyAction::PRESSED}}}));
	// SELECT COMMANDER - Will jump to the group as well as select if group is ALREADY selected
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectCommander_N(0),                                       "SelectCommander_0",            N_("Select Commander 0"),                           {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_0,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectCommander_N(1),                                       "SelectCommander_1",            N_("Select Commander 1"),                           {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_1,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectCommander_N(2),                                       "SelectCommander_2",            N_("Select Commander 2"),                           {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_2,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectCommander_N(3),                                       "SelectCommander_3",            N_("Select Commander 3"),                           {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_3,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectCommander_N(4),                                       "SelectCommander_4",            N_("Select Commander 4"),                           {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_4,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectCommander_N(5),                                       "SelectCommander_5",            N_("Select Commander 5"),                           {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_5,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectCommander_N(6),                                       "SelectCommander_6",            N_("Select Commander 6"),                           {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_6,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectCommander_N(7),                                       "SelectCommander_7",            N_("Select Commander 7"),                           {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_7,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectCommander_N(8),                                       "SelectCommander_8",            N_("Select Commander 8"),                           {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_8,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectCommander_N(9),                                       "SelectCommander_9",            N_("Select Commander 9"),                           {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_9,            KeyAction::PRESSED}}}));
	// MULTIPLAYER
	entries.emplace_back(KeyFunctionInfo(InputContext::BACKGROUND,      KeyMappingType::ASSIGNABLE,  kf_addMultiMenu,                                               "addMultiMenu",                 N_("Multiplayer Options / Alliance dialog"),        {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_KPENTER,      KeyAction::PRESSED}}}));
	// GAME CONTROLS - Moving around, zooming in, rotating etc
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_CameraUp,                                                   "CameraUp",                     N_("Move Camera Up"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_UPARROW,      KeyAction::DOWN   }}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_CameraDown,                                                 "CameraDown",                   N_("Move Camera Down"),                             {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_DOWNARROW,    KeyAction::DOWN   }}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_CameraRight,                                                "CameraRight",                  N_("Move Camera Right"),                            {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_RIGHTARROW,   KeyAction::DOWN   }}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_CameraLeft,                                                 "CameraLeft",                   N_("Move Camera Left"),                             {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_LEFTARROW,    KeyAction::DOWN   }}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SeekNorth,                                                  "SeekNorth",                    N_("Snap View to North"),                           {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_BACKSPACE,    KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_ToggleCamera,                                               "ToggleCamera",                 N_("Toggle Tracking Camera"),                       {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_SPACE,        KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::BACKGROUND,      KeyMappingType::FIXED,       kf_addInGameOptions,                                           "addInGameOptions",             N_("Display In-Game Options"),                      {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_ESC,          KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::RADAR,           KeyMappingType::ASSIGNABLE,  kf_RadarZoom(-1),                                              "RadarZoomOut",                 N_("Zoom Radar Out"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MINUS,        KeyAction::PRESSED}}, {KeyMappingSlot::SECONDARY, {KEY_CODE::KEY_IGNORE, MOUSE_KEY_CODE::MOUSE_WDN, KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::RADAR,           KeyMappingType::ASSIGNABLE,  kf_RadarZoom(1),                                               "RadarZoomIn",                  N_("Zoom Radar In"),                                {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_EQUALS,       KeyAction::PRESSED}}, {KeyMappingSlot::SECONDARY, {KEY_CODE::KEY_IGNORE, MOUSE_KEY_CODE::MOUSE_WUP, KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_Zoom(-1),                                                   "ZoomIn",                       N_("Zoom In"),                                      {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_KP_PLUS,      KeyAction::DOWN   }}, {KeyMappingSlot::SECONDARY, {KEY_CODE::KEY_IGNORE, MOUSE_KEY_CODE::MOUSE_WUP, KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_Zoom(1),                                                    "ZoomOut",                      N_("Zoom Out"),                                     {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   MOUSE_KEY_CODE::MOUSE_WUP,  KeyAction::PRESSED}}, {KeyMappingSlot::SECONDARY, {KEY_CODE::KEY_IGNORE, MOUSE_KEY_CODE::MOUSE_WDN, KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_PitchForward,                                               "PitchForward",                 N_("Pitch Forward"),                                {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_KP_2,         KeyAction::DOWN   }}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_RotateLeft,                                                 "RotateLeft",                   N_("Rotate Left"),                                  {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_KP_4,         KeyAction::DOWN   }}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_ResetPitch,                                                 "ResetPitch",                   N_("Reset Pitch"),                                  {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_KP_5,         KeyAction::DOWN   }}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_RotateRight,                                                "RotateRight",                  N_("Rotate Right"),                                 {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_KP_6,         KeyAction::DOWN   }}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_PitchBack,                                                  "PitchBack",                    N_("Pitch Back"),                                   {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_KP_8,         KeyAction::DOWN   }}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_RightOrderMenu,                                             "RightOrderMenu",               N_("Orders Menu"),                                  {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_KP_0,         KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SlowDown,                                                   "SlowDown",                     N_("Decrease Game Speed"),                          {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_MINUS,        KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SpeedUp,                                                    "SpeedUp",                      N_("Increase Game Speed"),                          {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_EQUALS,       KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_NormalSpeed,                                                "NormalSpeed",                  N_("Reset Game Speed"),                             {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_BACKSPACE,    KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_FaceNorth,                                                  "FaceNorth",                    N_("View North"),                                   {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_UPARROW,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_FaceSouth,                                                  "FaceSouth",                    N_("View South"),                                   {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_DOWNARROW,    KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_FaceEast,                                                   "FaceEast",                     N_("View East"),                                    {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_LEFTARROW,    KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_FaceWest,                                                   "FaceWest",                     N_("View West"),                                    {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_RIGHTARROW,   KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_JumpToResourceExtractor,                                    "JumpToResourceExtractor",      N_("View next Oil Derrick"),                        {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_KP_STAR,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_JumpToRepairUnits,                                          "JumpToRepairUnits",            N_("View next Repair Unit"),                        {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_JumpToConstructorUnits,                                     "JumpToConstructorUnits",       N_("View next Truck"),                              {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_JumpToSensorUnits,                                          "JumpToSensorUnits",            N_("View next Sensor Unit"),                        {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_JumpToCommandUnits,                                         "JumpToCommandUnits",           N_("View next Commander"),                          {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_ToggleOverlays,                                             "ToggleOverlays",               N_("Toggle Overlays"),                              {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_TAB,          KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_ToggleConsoleDrop,                                          "ToggleConsoleDrop",            N_("Toggle Console History "),                      {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_BACKQUOTE,    KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_ToggleTeamChat,                                             "ToggleTeamChat",               N_("Toggle Team Chat History"),                     {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_BACKQUOTE,    KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_RotateBuildingCW,                                           "RotateBuildingClockwise",      N_("Rotate Building Clockwise"),                    {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_RotateBuildingACW,                                          "RotateBuildingAnticlockwise",  N_("Rotate Building Anticlockwise"),                {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	// IN GAME MAPPINGS - Single key presses - ALL __DEBUG keymappings will be removed for master
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_CentreOnBase,                                               "CentreOnBase",                 N_("Center View on HQ"),                            {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_B,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidAttackCease,                                        "SetDroidAttackCease",          N_("Hold Fire"),                                    {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_C,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_JumpToUnassignedUnits,                                      "JumpToUnassignedUnits",        N_("View Unassigned Units"),                        {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_D,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidAttackReturn,                                       "SetDroidAttackReturn",         N_("Return Fire"),                                  {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_E,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidAttackAtWill,                                       "SetDroidAttackAtWill",         N_("Fire at Will"),                                 {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_F,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidMoveGuard,                                          "SetDroidMoveGuard",            N_("Guard Position"),                               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_G,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidReturnToBase,                                       "SetDroidReturnToBase",         N_("Return to HQ"),                                 {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_H,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidOrderHold,                                          "SetDroidOrderHold",            N_("Hold Position"),                                {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_H,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidRangeOptimum,                                       "SetDroidRangeOptimum",         N_("Optimum Range"),                                {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_I,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidRangeShort,                                         "SetDroidRangeShort",           N_("Short Range"),                                  {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_O,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidMovePursue,                                         "SetDroidMovePursue",           N_("Pursue"),                                       {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_P,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidMovePatrol,                                         "SetDroidMovePatrol",           N_("Patrol"),                                       {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_Q,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidGoForRepair,                                        "SetDroidGoForRepair",          N_("Return For Repair"),                            {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_R,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidOrderStop,                                          "SetDroidOrderStop",            N_("Stop Droid"),                                   {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_S,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidGoToTransport,                                      "SetDroidGoToTransport",        N_("Go to Transport"),                              {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_T,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidRangeLong,                                          "SetDroidRangeLong",            N_("Long Range"),                                   {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_U,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SendGlobalMessage,                                          "SendGlobalMessage",            N_("Send Global Text Message"),                     {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_RETURN,       KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SendTeamMessage,                                            "SendTeamMessage",              N_("Send Team Text Message"),                       {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_RETURN,       KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_AddHelpBlip,                                                "AddHelpBlip",                  N_("Drop a beacon"),                                {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_H,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_ToggleShadows,                                              "ToggleShadows",                N_("Toggles shadows"),                              {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_S,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_toggleTrapCursor,                                           "toggleTrapCursor",             N_("Trap cursor"),                                  {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_T,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::RADAR,           KeyMappingType::ASSIGNABLE,  kf_ToggleRadarTerrain,                                         "ToggleRadarTerrain",           N_("Toggle radar terrain"),                         {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_TAB,          KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::RADAR,           KeyMappingType::ASSIGNABLE,  kf_ToggleRadarAllyEnemy,                                       "ToggleRadarAllyEnemy",         N_("Toggle ally-enemy radar view"),                 {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_TAB,          KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_ShowMappings,                                               "ShowMappings",                 N_("Show all keyboard mappings"),                   {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_M,            KeyAction::PRESSED}}}));
	// Some extra non QWERTY mappings but functioning in same way
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidRetreatMedium,                                      "SetDroidRetreatMedium",        N_("Retreat at Medium Damage"),                     {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_COMMA,        KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidRetreatHeavy,                                       "SetDroidRetreatHeavy",         N_("Retreat at Heavy Damage"),                      {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_FULLSTOP,     KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidRetreatNever,                                       "SetDroidRetreatNever",         N_("Do or Die!"),                                   {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_FORWARDSLASH, KeyAction::PRESSED}}}));
	// In game mappings - COMBO (CTRL + LETTER) presses
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllCombatUnits,                                       "SelectAllCombatUnits",         N_("Select all Combat Units"),                      {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_A,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllCyborgs,                                           "SelectAllCyborgs",             N_("Select all Cyborgs"),                           {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_C,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllDamaged,                                           "SelectAllDamaged",             N_("Select all Heavily Damaged Units"),             {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_D,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllHalfTracked,                                       "SelectAllHalfTracked",         N_("Select all Half-tracks"),                       {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_F,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllHovers,                                            "SelectAllHovers",              N_("Select all Hovers"),                            {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_H,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SetDroidRecycle,                                            "SetDroidRecycle",              N_("Return for Recycling"),                         {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_R,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllOnScreenUnits,                                     "SelectAllOnScreenUnits",       N_("Select all Units on Screen"),                   {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_S,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllTracked,                                           "SelectAllTracked",             N_("Select all Tracks"),                            {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_T,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllUnits,                                             "SelectAllUnits",               N_("Select EVERY unit"),                            {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_U,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllVTOLs,                                             "SelectAllVTOLs",               N_("Select all VTOLs"),                             {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_V,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllArmedVTOLs,                                        "SelectAllArmedVTOLs",          N_("Select all fully-armed VTOLs"),                 {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_V,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllWheeled,                                           "SelectAllWheeled",             N_("Select all Wheels"),                            {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_W,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_FrameRate,                                                  "FrameRate",                    N_("Show frame rate"),                              {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_Y,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllSameType,                                          "SelectAllSameType",            N_("Select all units with the same components"),    {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_Z,            KeyAction::PRESSED}}}));
	// In game mappings - COMBO (SHIFT + LETTER) presses
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllCombatCyborgs,                                     "SelectAllCombatCyborgs",       N_("Select all Combat Cyborgs"),                    {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_C,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllEngineers,                                         "SelectAllEngineers",           N_("Select all Engineers"),                         {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_E,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllLandCombatUnits,                                   "SelectAllLandCombatUnits",     N_("Select all Land Combat Units"),                 {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_G,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllMechanics,                                         "SelectAllMechanics",           N_("Select all Mechanics"),                         {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_M,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllTransporters,                                      "SelectAllTransporters",        N_("Select all Transporters"),                      {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_P,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllRepairTanks,                                       "SelectAllRepairTanks",         N_("Select all Repair Tanks"),                      {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_R,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllSensorUnits,                                       "SelectAllSensorUnits",         N_("Select all Sensor Units"),                      {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_S,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectAllTrucks,                                            "SelectAllTrucks",              N_("Select all Trucks"),                            {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_T,            KeyAction::PRESSED}}}));
	// SELECT PLAYERS - DEBUG ONLY
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectNextFactory,                                          "SelectNextFactory",            N_("Select next Factory"),                          {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectNextResearch,                                         "SelectNextResearch",           N_("Select next Research Facility"),                {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectNextPowerStation,                                     "SelectNextPowerStation",       N_("Select next Power Generator"),                  {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectNextCyborgFactory,                                    "SelectNextCyborgFactory",      N_("Select next Cyborg Factory"),                   {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_SelectNextVTOLFactory,                                      "SelectNextVtolFactory",        N_("Select next VTOL Factory"),                     {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_JumpNextFactory,                                            "JumpNextFactory",              N_("Jump to next Factory"),                         {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_JumpNextResearch,                                           "JumpNextResearch",             N_("Jump to next Research Facility"),               {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_JumpNextPowerStation,                                       "JumpNextPowerStation",         N_("Jump to next Power Generator"),                 {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_JumpNextCyborgFactory,                                      "JumpNextCyborgFactory",        N_("Jump to next Cyborg Factory"),                  {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::ASSIGNABLE,  kf_JumpNextVTOLFactory,                                        "JumpNextVtolFactory",          N_("Jump to next VTOL Factory"),                    {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_MAXSCAN,      KeyAction::PRESSED}}}));
	// Debug options
	entries.emplace_back(KeyFunctionInfo(InputContext::BACKGROUND,      KeyMappingType::HIDDEN,      kf_ToggleDebugMappings,                                        "ToggleDebugMappings",          N_("Toggle Debug Mappings"),                        {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_BACKSPACE,    KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_ToggleShowPath,                                             "ToggleShowPath",               N_("Toggle display of droid path"),                 {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_M,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_ToggleShowGateways,                                         "ToggleShowGateways",           N_("Toggle display of gateways"),                   {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_E,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_ToggleVisibility,                                           "ToggleVisibility",             N_("Toggle visibility"),                            {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_V,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_RaiseTile,                                                  "RaiseTile",                    N_("Raise tile height"),                            {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_W,            KeyAction::DOWN   }}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_LowerTile,                                                  "LowerTile",                    N_("Lower tile height"),                            {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_A,            KeyAction::DOWN   }}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_ToggleFog,                                                  "ToggleFog",                    N_("Toggles All fog"),                              {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_J,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_ToggleWeather,                                              "ToggleWeather",                N_("Trigger some weather"),                         {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_Q,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_TriFlip,                                                    "TriFlip",                      N_("Flip terrain triangle"),                        {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_IGNORE,   KEY_CODE::KEY_K,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_PerformanceSample,                                          "PerformanceSample",            N_("Make a performance measurement sample"),        {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_K,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_AllAvailable,                                               "AllAvailable",                 N_("Make all items available"),                     {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_A,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_KillSelected,                                               "KillSelected",                 N_("Kill Selected Unit(s)"),                        {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LALT,     KEY_CODE::KEY_K,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_ToggleGodMode,                                              "ToggleGodMode",                N_("Toggle god Mode Status"),                       {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_G,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_ChooseOptions,                                              "ChooseOptions",                N_("Display Options Screen"),                       {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_O,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_FinishResearch,                                             "FinishResearch",               N_("Complete current research"),                    {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_X,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_RevealMapAtPos,                                             "RevealMapAtPos",               N_("Reveal map at mouse position"),                 {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LSHIFT,   KEY_CODE::KEY_W,            KeyAction::PRESSED}}}));
	entries.emplace_back(KeyFunctionInfo(InputContext::__DEBUG,         KeyMappingType::HIDDEN,      kf_TraceObject,                                                "TraceObject",                  N_("Trace a game object"),                          {{KeyMappingSlot::PRIMARY, {KEY_CODE::KEY_LCTRL,    KEY_CODE::KEY_L,            KeyAction::PRESSED}}}));

	// Mappings bindable at runtime (hidden, not saved, custom bind logic)
	entries.emplace_back(KeyFunctionInfo(InputContext::GAMEPLAY,        KeyMappingType::HIDDEN,      kf_JumpToMapMarker,                                            "JumpToMapMarker"));

	return KeyFunctionInfoTable(entries);
}
static const KeyFunctionInfoTable keyFunctionInfoTable = initializeKeyFunctionInfoTable();

const KeyFunctionEntries allKeyFunctionEntries()
{
	return keyFunctionInfoTable.allKeyFunctionEntries();
}

nonstd::optional<std::reference_wrapper<const KeyFunctionInfo>> keyFunctionInfoByName(std::string const &name)
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

// ----------------------------------------------------------------------------------
/*
	Here is where we assign functions to keys and to combinations of keys.
	these will be read in from a .cfg file customisable by the player from
	an in-game menu
*/
void InputManager::resetMappings(bool bForceDefaults)
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
	if (!bForceDefaults)
	{
		if (loadKeyMap(*this))
		{
			debug(LOG_WZ, "Loaded key map successfully");
		}
		else
		{
			bForceDefaults = true;
		}
	}

	// Use addDefaultMapping to add the default key mapping if either: (a) bForceDefaults is true, or (b) the loaded key mappings are missing an entry
	bool didAdd = false;
	for (const KeyFunctionInfo& info : allKeyFunctionEntries())
	{
		for (const auto& mapping : info.defaultMappings)
		{
			const auto slot = mapping.first;
			const auto keys = mapping.second;
			didAdd |= addDefaultMapping(keys.meta, keys.input, keys.action, info, bForceDefaults, slot);
		}
	}

	if (didAdd)
	{
		saveKeyMap(*this);
	}
}

// ----------------------------------------------------------------------------------
bool InputManager::removeMapping(const KeyMapping& mappingToRemove)
{
	auto mapping = std::find_if(keyMappings.begin(), keyMappings.end(), [mappingToRemove](const KeyMapping& mapping) {
		return mapping == mappingToRemove;
	});
	if (mapping != keyMappings.end())
	{
		keyMappings.erase(mapping);
		bMappingsSortOrderDirty = true;
		return true;
	}
	return false;
}

bool InputManager::addDefaultMapping(const KEY_CODE metaCode, const KeyMappingInput input, const KeyAction action, const KeyFunctionInfo& info, const bool bForceDefaults, const KeyMappingSlot slot)
{
	const auto psMapping = getMapping(info, slot);
	if (!bForceDefaults && psMapping.has_value())
	{
		// Not forcing defaults, and there is already a mapping entry for this function with this slot
		return false;
	}

	// Otherwise, force / overwrite with the default
	if (!bForceDefaults)
	{
		debug(LOG_INFO, "Adding missing keymap entry: %s", info.displayName.c_str());
	}
	if (psMapping.has_value())
	{
		// Remove any existing mapping for this function
		removeMapping(*psMapping);
	}
	if (!bForceDefaults)
	{	
		// Clear the keys from any other mappings
		removeConflictingMappings(metaCode, input, info.context);
	}

	// Set default key mapping
	addMapping(metaCode, input, action, info, slot);
	return true;
}

// ----------------------------------------------------------------------------------
/* Allows _new_ mappings to be made at runtime */
static bool checkQwertyKeys(InputManager& inputManager)
{
	/* Are we trying to make a new map marker? */
	if (keyDown(KEY_LALT))
	{
		/* Did we press a key */
		const KEY_CODE qKey = getQwertyKey();
		if (qKey)
		{
			const auto info = keyFunctionInfoByName("JumpToMapMarker");
			ASSERT(info.has_value(), "Could not find keymap info table entry for JumpToMapMarker");
			if (!info.has_value())
			{
				return false;
			}

			const auto existing = inputManager.findMappingsForInput(KEY_CODE::KEY_LSHIFT, qKey);
			if (existing.size() > 0 && std::any_of(existing.begin(), existing.end(), [](const KeyMapping& mapping) {
				return mapping.info.name != "JumpToMapMarker";
			}))
			{
				return false;
			}

			for (const KeyMapping& old : existing)
			{
				if (old.info.name == "JumpToMapMarker")
				{
					inputManager.removeMapping(old);
				}
			}

			const unsigned int tableEntry = asciiKeyCodeToTable(qKey);
			/* We're assigning something to the key */
			debug(LOG_NEVER, "Assigning keymapping to tableEntry: %i", tableEntry);
			if (qwertyKeyMappings[tableEntry].psMapping)
			{
				/* Get rid of the old mapping on this key if there was one */
				inputManager.removeMapping(*qwertyKeyMappings[tableEntry].psMapping);
			}

			/* Now add the new one for this location */
			qwertyKeyMappings[tableEntry].psMapping =
				&inputManager.addMapping(KEY_LSHIFT, KeyMappingInput((KEY_CODE)qKey), KeyAction::PRESSED, *info, KeyMappingSlot::PRIMARY);

			/* Store away the position and view angle */
			qwertyKeyMappings[tableEntry].xPos = playerPos.p.x;
			qwertyKeyMappings[tableEntry].yPos = playerPos.p.z;
			qwertyKeyMappings[tableEntry].spin = playerPos.r.y;

			return true;
		}
	}
	return false;
}

// ----------------------------------------------------------------------------------
/* allows checking if mapping should currently be ignored in processMappings */
static bool isIgnoredMapping(const InputManager& inputManager, const bool bAllowMouseWheelEvents, const KeyMapping& mapping)
{
	if (!inputManager.isContextActive(mapping.info.context))
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

	if (mapping.info.function == nullptr)
	{
		return true;
	}

	const bool bIsDebugMapping = mapping.info.context == InputContext::__DEBUG;
	if (bIsDebugMapping && !getDebugMappingStatus()) {
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------------
/* Manages update of all the active function mappings */
void InputManager::processMappings(const bool bAllowMouseWheelEvents)
{
	/* Bomb out if there are none */
	if (keyMappings.empty())
	{
		return;
	}

	/* Check if player has made new camera markers */
	checkQwertyKeys(*this);

	/* If mappings have been updated or context priorities have changed, sort the mappings by priority and whether or not they have meta keys */
	if (bMappingsSortOrderDirty)
	{
		keyMappings.sort([this](const KeyMapping& a, const KeyMapping& b) {
			// Primary sort by priority
			const unsigned int priorityA = getContextPriority(a.info.context);
			const unsigned int priorityB = getContextPriority(b.info.context);
			if (priorityA != priorityB)
			{
				return priorityA > priorityB;
			}

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
	for (const KeyMapping& keyToProcess : keyMappings)
	{
		/* Skip inappropriate ones when necessary */
		if (isIgnoredMapping(*this, bAllowMouseWheelEvents, keyToProcess))
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
			keyToProcess.info.function();
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
