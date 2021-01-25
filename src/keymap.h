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

#ifndef __INCLUDED_SRC_KEYMAP_H__
#define __INCLUDED_SRC_KEYMAP_H__

#include "lib/framework/input.h"


enum KEY_ACTION
{
	KEYMAP_DOWN,
	KEYMAP_PRESSED,
	KEYMAP_RELEASED
};

enum KEY_STATUS
{
	KEYMAP__DEBUG,
	KEYMAP_ALWAYS,
	KEYMAP_ASSIGNABLE,
	KEYMAP_ALWAYS_PROCESS,
	KEYMAP___HIDE
};

enum class KeyMappingInputSource
{
	KEY_CODE = 1,
	MOUSE_KEY_CODE
};

union KeyMappingInputValue
{
	KEY_CODE       keyCode;
	MOUSE_KEY_CODE mouseKeyCode;

	KeyMappingInputValue(const KEY_CODE keyCode);
	KeyMappingInputValue(const MOUSE_KEY_CODE mouseKeyCode);
};

struct KeyMappingInput {
	KeyMappingInputSource source;
	KeyMappingInputValue  value;

	bool isPressed() const;
	bool isDown() const;
	bool isReleased() const;

	bool isCleared() const;

	KeyMappingInput(const KEY_CODE keyCode);
	KeyMappingInput(const MOUSE_KEY_CODE mouseKeyCode);

	KeyMappingInput();
};

enum class KeyMappingPriority {
	PRIMARY,
	SECONDARY,
	LAST
};

struct KEY_MAPPING
{
	void             (*function)();
	KEY_STATUS         status;
	UDWORD             lastCalled;
	KEY_CODE           metaKeyCode;
	KEY_CODE           altMetaKeyCode;
	KeyMappingInput    input;
	KEY_ACTION         action;
	std::string        name;
	KeyMappingPriority priority;
};

KEY_MAPPING *keyAddMapping(KEY_STATUS status, KEY_CODE metaCode, KeyMappingInput input, KEY_ACTION action, void (*pKeyMapFunc)(), const char *name, const KeyMappingPriority priority = KeyMappingPriority::PRIMARY);
KEY_MAPPING *keyGetMappingFromFunction(void (*function)(), const KeyMappingPriority priority);
KEY_MAPPING *keyFindMapping(const KEY_CODE metaCode, const KeyMappingInput input, const KeyMappingPriority priority = KeyMappingPriority::LAST);
void keyProcessMappings(const bool bExclude, const bool allowMouseWheelEvents);
void keyInitMappings(bool bForceDefaults);
KEY_CODE getLastSubKey();
KEY_CODE getLastMetaKey();
void processDebugMappings(unsigned player, bool val);
bool getDebugMappingStatus();
bool getWantedDebugMappingStatus(unsigned player);
std::string getWantedDebugMappingStatuses(bool val);

bool clearKeyMappingIfExists(const KEY_CODE metaCode, const KeyMappingInput input);

UDWORD	getMarkerX(KEY_CODE code);
UDWORD	getMarkerY(KEY_CODE code);
SDWORD	getMarkerSpin(KEY_CODE code);

// for keymap editor.
struct KeyMapSaveEntry
{
	void (*function)();
	char const *name;
};
KeyMapSaveEntry const *keymapEntryByFunction(void (*function)());
KeyMapSaveEntry const *keymapEntryByName(std::string const &name);
extern std::list<KEY_MAPPING> keyMappings;
KeyMappingInputSource keyMappingSourceByName(std::string const& name);
KeyMappingPriority keyMappingPriorityByName(std::string const& name);


#endif // __INCLUDED_SRC_KEYMAP_H__
