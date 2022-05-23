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
#include <nonstd/optional.hpp>

#include "lib/framework/frame.h"

#include "keyconfig.h"
#include "context.h"
#include "mapping.h"
#include "debugmappings.h"

class InputManager
{
	// Input processing
public:
	/* (Re-)Initializes mappings to their default values. If `bForceDefaults` is true, any existing mappings will be overwritten with the defaults */
	void resetMappings(const bool bForceDefaults, const KeyFunctionConfiguration& keyFuncConfig);

	void processMappings(const bool bAllowMouseWheelEvents);

	// Subsystems/General
public:
	KeyMappings& mappings();
	const KeyMappings& cmappings() const;

	ContextManager& contexts();

	DebugInputManager& debugManager();

	void shutdown();

	// Map markers
public:
	void updateMapMarkers();

public:
	InputManager();

private:
	/* Registers a new default key mapping */
	bool addDefaultMapping(const KEY_CODE metaCode, const KeyMappingInput input, const KeyAction action, const KeyFunctionInfo& info, const KeyMappingSlot slot = KeyMappingSlot::PRIMARY);

	bool mappingsSortRequired() const;

	/* Explicit hash fn for KEY_CODEs. Do not use std::hash<KEY_CODE> directly to avoid breaking things once/if KEY_CODE is converted to an enum class */
	struct KeyCodeHash
	{
		std::size_t operator()(KEY_CODE keyCode) const
		{
			return static_cast<std::size_t>(keyCode);
		}
	};

	ContextManager contextManager;
	KeyMappings keyMappings;
	std::unordered_map<KEY_CODE, KeyFunctionInfo, KeyCodeHash> markerKeyFunctions;
	bool bMappingsSortOrderDirty = true;

	DebugInputManager dbgInputManager = DebugInputManager(MAX_PLAYERS);
};

#endif // __INCLUDED_SRC_INPUT_MANAGER_H__
