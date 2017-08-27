/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

struct KEY_MAPPING
{
	void (*function)();
	KEY_STATUS	status;
	UDWORD		lastCalled;
	KEY_CODE	metaKeyCode;
	KEY_CODE	altMetaKeyCode;
	KEY_CODE	subKeyCode;
	KEY_ACTION	action;
	std::string name;
};

KEY_MAPPING *keyAddMapping(KEY_STATUS status, KEY_CODE metaCode, KEY_CODE subcode, KEY_ACTION action, void (*pKeyMapFunc)(), const char *name);
KEY_MAPPING *keyGetMappingFromFunction(void (*function)());
KEY_MAPPING *keyFindMapping(KEY_CODE metaCode, KEY_CODE subCode);
void keyProcessMappings(bool bExclude);
void keyInitMappings(bool bForceDefaults);
KEY_CODE getLastSubKey();
KEY_CODE getLastMetaKey();
void processDebugMappings(unsigned player, bool val);
bool getDebugMappingStatus();
bool getWantedDebugMappingStatus(unsigned player);
std::string getWantedDebugMappingStatuses(bool val);

bool keyReAssignMapping(KEY_CODE origMetaCode, KEY_CODE origSubCode, KEY_CODE newMetaCode, KEY_CODE newSubCode);

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

void	keyShowMappings();

#endif // __INCLUDED_SRC_KEYMAP_H__
