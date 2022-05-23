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

#ifndef __INCLUDED_SRC_INPUT_CONFIG_H__
#define __INCLUDED_SRC_INPUT_CONFIG_H__

#include <unordered_map>
#include <nonstd/optional.hpp>

#include "lib/framework/frame.h"
#include "lib/framework/input.h"

#include "context.h"

enum class KeyMappingInputSource
{
	KEY_CODE = 1,
	MOUSE_KEY_CODE
};

KeyMappingInputSource keyMappingSourceByName(const std::string& name);

union KeyMappingInputValue
{
	KEY_CODE       keyCode;
	MOUSE_KEY_CODE mouseKeyCode;

	KeyMappingInputValue(const KEY_CODE keyCode);
	KeyMappingInputValue(const MOUSE_KEY_CODE mouseKeyCode);
};

struct KeyMappingInput
{
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

enum class KeyAction
{
	DOWN,
	PRESSED,
	RELEASED
};

struct KeyCombination
{
	KEY_CODE        meta;
	KeyMappingInput input;
	KeyAction       action;

	KeyCombination(
		const KEY_CODE        meta,
		const KeyMappingInput input,
		const KeyAction       action
	);

	KeyCombination(
		const KeyMappingInput input,
		const KeyAction       action
	);

	KeyCombination(
		const KEY_CODE        meta,
		const KeyMappingInput input
	);

	KeyCombination(
		const KeyMappingInput input
	);
};

bool operator==(const KeyCombination& lhs, const KeyCombination& rhs);
bool operator!=(const KeyCombination& lhs, const KeyCombination& rhs);

enum class KeyMappingType
{
	/* The mapping value can be freely re-assigned. */
	ASSIGNABLE,

	/* The mapping value is visible, but cannot be re-assigned. */
	FIXED,

	/* The mapping does not show up in the key map options */
	HIDDEN
};

enum class KeyMappingSlot
{
	PRIMARY,
	SECONDARY,
	LAST
};

KeyMappingSlot keyMappingSlotByName(const std::string& name);


typedef std::function<void()> MappableFunction;

struct KeyFunctionInfo
{
	const ContextId&       context;
	const KeyMappingType   type;
	const MappableFunction function;
	const std::string      name;
	const std::string      displayName;

	const std::vector<std::pair<KeyMappingSlot, KeyCombination>> defaultMappings;

	const bool bIsDebugOnly;

	KeyFunctionInfo(
		const ContextId&       context,
		const KeyMappingType   type,
		const MappableFunction function,
		const std::string      name
	);

	KeyFunctionInfo(
		const ContextId&       context,
		const KeyMappingType   type,
		const MappableFunction function,
		const std::string      name,
		const std::string      displayName,
		const std::vector<std::pair<KeyMappingSlot, KeyCombination>> defaultMappings,
		const bool             bIsDebugOnly = false
	);

	// Prevent copies. The entries are immutable and thus should never be copied around.
	KeyFunctionInfo(const KeyFunctionInfo&) = delete;
	void operator=(const KeyFunctionInfo&) = delete;

	// Allow construction-time move semantics
	KeyFunctionInfo(KeyFunctionInfo&&) = default;
};

typedef std::vector<std::reference_wrapper<const KeyFunctionInfo>> KeyFunctionEntries;

class KeyFunctionConfiguration {
public:
	KeyFunctionConfiguration();

	nonstd::optional<std::reference_wrapper<const KeyFunctionInfo>> keyFunctionInfoByName(const std::string& name) const;
	const KeyFunctionEntries allKeyFunctionEntries() const;

private:
	std::vector<KeyFunctionInfo> entries;
	std::unordered_map<std::string, size_t> nameToIndexMap;
};

#endif // __INCLUDED_SRC_INPUT_CONFIG_H__
