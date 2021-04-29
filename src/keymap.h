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

#include <optional-lite/optional.hpp>

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

	bool is(const KEY_CODE keyCode) const;
	bool is(const MOUSE_KEY_CODE mouseKeyCode) const;

	nonstd::optional<KEY_CODE> asKeyCode() const;
	nonstd::optional<MOUSE_KEY_CODE> asMouseKeyCode() const;

	KeyMappingInput(const KEY_CODE keyCode);
	KeyMappingInput(const MOUSE_KEY_CODE mouseKeyCode);

	KeyMappingInput();

	struct Hash
	{
		std::size_t operator()(KeyMappingInput const& kmi) const noexcept
		{
			const std::size_t hSource = std::hash<std::size_t>{}(static_cast<unsigned int>(kmi.source));
			std::size_t hValue = 0;
			switch (kmi.source)
			{
			case KeyMappingInputSource::KEY_CODE:
				hValue += static_cast<unsigned int>(kmi.value.keyCode);
				break;
			case KeyMappingInputSource::MOUSE_KEY_CODE:
				// Offset by large value to avoid conflicts with KEY_CODEs
				hValue += 10000 + static_cast<unsigned int>(kmi.value.mouseKeyCode);
				break;
			}
			return hSource ^ (hValue << 1);
		}
	};
};

bool operator==(const KeyMappingInput& lhs, const KeyMappingInput& rhs);
bool operator!=(const KeyMappingInput& lhs, const KeyMappingInput& rhs);

enum class KeyMappingSlot {
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
	KeyMappingInput    input;
	KEY_ACTION         action;
	KeyMappingSlot     slot;

	bool isActivated() const;

	bool hasMeta() const;

	bool toString(char* pOutStr) const;
};

KEY_MAPPING *keyAddMapping(KEY_STATUS status, KEY_CODE metaCode, KeyMappingInput input, KEY_ACTION action, void (*pKeyMapFunc)(), const KeyMappingSlot slot = KeyMappingSlot::PRIMARY);
KEY_MAPPING *keyGetMappingFromFunction(void (*function)(), const KeyMappingSlot slot);
KEY_MAPPING *keyFindMapping(const KEY_CODE metaCode, const KeyMappingInput input, const KeyMappingSlot slot = KeyMappingSlot::LAST);
void keyProcessMappings(const bool bExclude, const bool allowMouseWheelEvents);
void keyInitMappings(bool bForceDefaults);
KeyMappingInput getLastInput();
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
struct KeyFunctionInfo
{
	void      (*function)();
	std::string name;
	std::string displayName;

	KeyFunctionInfo(
		void      (*function)(),
		std::string name,
		std::string displayName
	);

	// Prevent copies. The entries are immutable and thus should never be copied around.
	KeyFunctionInfo(const KeyFunctionInfo&) = delete;
	void operator=(const KeyFunctionInfo&) = delete;

    // Allow construction-time move semantics
	KeyFunctionInfo(KeyFunctionInfo&&) = default;
};
void invalidateKeyMappingSortOrder();
const std::vector<std::reference_wrapper<const KeyFunctionInfo>> allKeymapEntries();
KeyFunctionInfo const *keyFunctionInfoByFunction(void (*function)());
KeyFunctionInfo const *keyFunctionInfoByName(std::string const &name);
extern std::list<KEY_MAPPING> keyMappings;
KeyMappingInputSource keyMappingSourceByName(std::string const& name);
KeyMappingSlot keyMappingSlotByName(std::string const& name);


#endif // __INCLUDED_SRC_KEYMAP_H__
