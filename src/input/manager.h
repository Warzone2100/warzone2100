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

#ifndef __INCLUDED_SRC_INPUT_MANAGER_H__
#define __INCLUDED_SRC_INPUT_MANAGER_H__

#include <vector>
#include <list>
#include <optional-lite/optional.hpp>

#include "keyconfig.h"
#include "context.h"
#include "mapping.h"

class InputManager
{
	// Input processing
public:
	KeyMapping& addMapping(const KEY_CODE meta, const KeyMappingInput input, const KeyAction action, const KeyFunctionInfo& info, const KeyMappingSlot slot);

	nonstd::optional<std::reference_wrapper<KeyMapping>> getMapping(const KeyFunctionInfo& info, const KeyMappingSlot slot);

	/* Finds all mappings with matching meta and input */
	std::vector<std::reference_wrapper<KeyMapping>> findMappingsForInput(const KEY_CODE meta, const KeyMappingInput input);

	std::vector<KeyMapping> removeConflictingMappings(const KEY_CODE meta, const KeyMappingInput input, const InputContext context);

	/* Removes a mapping specified by a pointer */
	bool removeMapping(const KeyMapping& mappingToRemove);

	void processMappings(const bool bAllowMouseWheelEvents);

	/* (Re-)Initializes mappings to their default values. If `bForceDefaults` is true, any existing mappings will be overwritten with the defaults */
	void resetMappings(const bool bForceDefaults);

	void clearAssignableMappings();

	const std::list<KeyMapping> getAllMappings() const;

private:
	/* Registers a new default key mapping */
	bool addDefaultMapping(const KEY_CODE metaCode, const KeyMappingInput input, const KeyAction action, const KeyFunctionInfo& info, const KeyMappingSlot slot = KeyMappingSlot::PRIMARY);

	// Input contexts
public:
	void resetContextStates();

	void makeAllContextsInactive();

	void setContextState(const InputContext& context, const InputContext::State state);

	/*
	 Gets the status of the context. If this returns `false`, any bindings belonging to the context
	 should not be processed.
	 */
	bool isContextActive(const InputContext& context) const;

	/* Priority of the context when resolving collisions. Context with highest priority wins */
	unsigned int getContextPriority(const InputContext& context) const;

	// General
public:
	void shutdown();

	// Map markers
public:
	void updateMapMarkers();

private:
	/* Explicit hash fn for KEY_CODEs. Do not use std::hash<KEY_CODE> directly to avoid breaking things once/if KEY_CODE is converted to an enum class */
	struct KeyCodeHash
	{
		std::size_t operator()(KEY_CODE keyCode) const
		{
			return static_cast<std::size_t>(keyCode);
		}
	};

	std::vector<InputContext::State> contextStates;
	std::list<KeyMapping> keyMappings;
	std::unordered_map<KEY_CODE, KeyFunctionInfo, KeyCodeHash> markerKeyFunctions;
	bool bMappingsSortOrderDirty = true;
};

#endif // __INCLUDED_SRC_INPUT_MANAGER_H__
