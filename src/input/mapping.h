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

#ifndef __INCLUDED_SRC_INPUT_MAPPING_H__
#define __INCLUDED_SRC_INPUT_MAPPING_H__

#include <list>
#include <vector>
#include <functional>
#include <nonstd/optional.hpp>

#include "lib/framework/frame.h"
#include "lib/framework/input.h"

#include "keyconfig.h"
#include "context.h"

struct KeyMapping
{
	const KeyFunctionInfo& info;
	UDWORD                 lastCalled;
	KeyCombination         keys;
	KeyMappingSlot         slot;

	bool isInvalid() const;

	bool isActivated() const;

	bool hasMeta() const;

	bool toString(char* pOutStr) const;
};

bool operator==(const KeyMapping& lhs, const KeyMapping& rhs);
bool operator!=(const KeyMapping& lhs, const KeyMapping& rhs);


class KeyMappings
{
public:
	KeyMapping& add(const KeyCombination keys, const KeyFunctionInfo& info, const KeyMappingSlot slot);

	nonstd::optional<std::reference_wrapper<KeyMapping>> get(const KeyFunctionInfo& info, const KeyMappingSlot slot);

	/* Finds all mappings with matching meta and input */
	std::vector<std::reference_wrapper<KeyMapping>> find(const KEY_CODE meta, const KeyMappingInput input);

	std::vector<std::reference_wrapper<KeyMapping>> findConflicting(const KEY_CODE meta, const KeyMappingInput input, const ContextId contextId, const ContextManager& contexts);

	std::vector<KeyMapping> removeConflicting(const KEY_CODE meta, const KeyMappingInput input, const ContextId& contextId, const ContextManager& contexts);

	/* Removes a mapping specified by a pointer */
	bool remove(const KeyMapping& mappingToRemove);

	void clear(nonstd::optional<KeyMappingType> filter = nonstd::nullopt);

	// I/O
public:
	/* Loads the key mappings from disk */
	bool load(const char* path, const KeyFunctionConfiguration& keyFuncConfig);

	/* Saves the key mappings to disk */
	bool save(const char* path) const;

private:
	void sort(const ContextManager& contexts);

	bool isDirty() const;

	std::list<KeyMapping> keyMappings;
	bool bDirty = true;

	friend class InputManager;

	// For range-based-for
public:
	std::list<KeyMapping>::const_iterator begin() const
	{
		return keyMappings.cbegin();
	}

	std::list<KeyMapping>::const_iterator end() const
	{
		return keyMappings.cend();
	}
};

#endif // __INCLUDED_SRC_INPUT_MAPPING_H__
