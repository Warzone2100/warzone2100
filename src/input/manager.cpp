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

#include <list>
#include <unordered_set>
#include <nonstd/optional.hpp>

#include "lib/framework/frame.h"
#include "lib/framework/input.h"

#include "manager.h"
#include "context.h"
#include "mapping.h"

#include "../keybind.h"
#include "../display3d.h"   // For playerPos
#include "lib/netplay/nettypes.h"	// For NETisReplay()


KeyMappings& InputManager::mappings()
{
	return keyMappings;
}

const KeyMappings& InputManager::cmappings() const
{
	return keyMappings;
}

ContextManager& InputManager::contexts()
{
	return contextManager;
}

DebugInputManager& InputManager::debugManager()
{
	return dbgInputManager;
}

InputManager::InputManager()
{
	registerDefaultContexts(contextManager, dbgInputManager);
}

void InputManager::shutdown()
{
	keyMappings.clear();
}

void InputManager::setKeyMapJsonPath(const std::string& path)
{
	if (path == currentKeyMapJsonPath)
	{
		return;
	}
	currentKeyMapJsonPath = path;
}

bool InputManager::mappingsSortRequired() const
{
	return bMappingsSortOrderDirty || contextManager.isDirty() || keyMappings.isDirty();
}

void InputManager::resetMappings(bool bForceDefaults, const KeyFunctionConfiguration& keyFuncConfig)
{
	contextManager.resetStates();
	keyMappings.clear();
	markerKeyFunctions.clear();

	bMappingsSortOrderDirty = true;
	for (unsigned n = 0; n < MAX_PLAYERS; ++n)
	{
		dbgInputManager.setPlayerWantsDebugMappings(n, false);
	}

	// load the mappings.
	if (!bForceDefaults)
	{
		if (keyMappings.load(currentKeyMapJsonPath.c_str(), keyFuncConfig))
		{
			debug(LOG_WZ, "Loaded key map successfully");
		}
		else
		{
			debug(LOG_WARNING, "Loading key map failed, forcing defaults");
			bForceDefaults = true;
		}
	}

	/* Add in the default mappings if we are forcing defaults (e.g. "reset to defaults" button was pressed from the UI) or loading key map failed. */
	for (const KeyFunctionInfo& info : keyFuncConfig.allKeyFunctionEntries())
	{
		for (const auto& mapping : info.defaultMappings)
		{
			const auto slot = mapping.first;
			const auto keys = mapping.second;
			/* Always add non-assignable mappings as they are not saved. */
			if (bForceDefaults || info.type != KeyMappingType::ASSIGNABLE)
			{
				addDefaultMapping(keys.meta, keys.input, keys.action, info, slot);
			}
		}
	}

	keyMappings.save(currentKeyMapJsonPath.c_str());
}

void InputManager::saveMappings()
{
	debug(LOG_WZ, "Saving: %s", currentKeyMapJsonPath.c_str());
	keyMappings.save(currentKeyMapJsonPath.c_str());
}

bool InputManager::addDefaultMapping(const KEY_CODE metaCode, const KeyMappingInput input, const KeyAction action, const KeyFunctionInfo& info, const KeyMappingSlot slot)
{
	const auto psMapping = keyMappings.get(info, slot);
	if (psMapping.has_value())
	{
		// Older GCC versions flag the nonstd::optional unwrapping here as potentially uninitialized usage. This is
		// a false-positive, and fixed on newer compiler versions (This broke Ubuntu 16.04 builds). I could not find
		// a way of cleanly tricking the compiler to comply, so we just ignore the warning for the line.
#if defined(__GNUC__) && (__GNUC__ == 5 && __GNUC_MINOR__ == 4)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif //  __GNUC__
		// Remove any existing mapping for this function
		keyMappings.remove(*psMapping);
#if defined(__GNUC__) && (__GNUC__ == 5 && __GNUC_MINOR__ == 4)
#pragma GCC diagnostic pop
#endif // __GNUC__

	}

	// Clear the keys from any other mappings
	keyMappings.removeConflicting(metaCode, input, info.context, contextManager);

	// Set default key mapping
	keyMappings.add({ metaCode, input, action }, info, slot);
	return true;
}

// ----------------------------------------------------------------------------------
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
	for (const KEY_CODE& code : qwertyCodes)
	{
		if (keyPressed(code))
		{
			return code; // Top-, middle- or bottom-row key pressed.
		}
	}

	return (KEY_CODE)0; // no ascii key pressed
}

void InputManager::updateMapMarkers()
{
	/* Are we trying to make a new map marker? */
	if (!keyDown(KEY_LALT))
	{
		return;
	}

	/* Did we press a key */
	KEY_CODE qKey = getQwertyKey();
	if (!qKey)
	{
		return;
	}

	const auto existing = keyMappings.find(KEY_CODE::KEY_LSHIFT, qKey);
	if (existing.size() > 0 && std::any_of(existing.begin(), existing.end(), [](const KeyMapping& mapping) {
		return mapping.info.name != "JumpToMapMarker";
	}))
	{
		return;
	}

	for (const KeyMapping& old : existing)
	{
		if (old.info.name == "JumpToMapMarker")
		{
			keyMappings.remove(old);
		}
	}

	/* Destroy any existing keymap entries for the key */
	const auto& entry = markerKeyFunctions.find(qKey);
	if (entry != markerKeyFunctions.end())
	{
		markerKeyFunctions.erase(entry);
	}

	/* Create a new keymap entry. x/z/yaw are captured within the lambda in kf_JumpToMapMarker */
	markerKeyFunctions.emplace(
		qKey,
		KeyFunctionInfo(
			InputContext::GAMEPLAY,
			KeyMappingType::HIDDEN,
			kf_JumpToMapMarker(playerPos.p.x, playerPos.p.z, playerPos.r.y),
			"JumpToMapMarker"
		)
	);
	const auto& maybeInfo = markerKeyFunctions.find(qKey);
	if (maybeInfo != markerKeyFunctions.end())
	{
		const auto& info = maybeInfo->second;
		keyMappings.add({ KEY_CODE::KEY_LSHIFT, qKey }, info, KeyMappingSlot::PRIMARY);
	}
}

// ----------------------------------------------------------------------------------
/* allows checking if mapping should currently be ignored in processMappings */
static bool isIgnoredMapping(InputManager& inputManager, const bool bAllowMouseWheelEvents, const KeyMapping& mapping, bool bypassActiveContextCheck = false)
{
	if (!bypassActiveContextCheck && !inputManager.contexts().isActive(mapping.info.context))
	{
		return true;
	}

	if (mapping.isInvalid())
	{
		return true;
	}

	if (!bAllowMouseWheelEvents && (mapping.keys.input.is(MOUSE_KEY_CODE::MOUSE_WUP) || mapping.keys.input.is(MOUSE_KEY_CODE::MOUSE_WDN)))
	{
		return true;
	}

	if (mapping.info.function == nullptr)
	{
		return true;
	}

	const DebugInputManager& dbgInputManager = inputManager.debugManager();;
	if (mapping.info.bIsDebugOnly && !dbgInputManager.debugMappingsAllowed() && !NETisReplay())
	{
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------------
/* Manages update of all the active function mappings */
void InputManager::processMappings(const bool bAllowMouseWheelEvents)
{
	/* Check if player has made new camera markers */
	updateMapMarkers();

	/* If mappings have been updated or context priorities have changed, sort the mappings by priority and whether or not they have meta keys */
	if (mappingsSortRequired())
	{
		keyMappings.sort(contextManager);
		contextManager.clearDirty();
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

		/* Skip if the input is already consumed. Handles skips for context/priority and meta-conflicts */
		const auto bIsAlreadyConsumed = consumedInputs.find(keyToProcess.keys.input) != consumedInputs.end();
		if (bIsAlreadyConsumed)
		{
			continue;
		}

		/* Execute the action if mapping was hit */
		if (keyToProcess.isActivated())
		{
			keyToProcess.info.function();
			consumedInputs.insert(keyToProcess.keys.input);
		}
	}
}

nonstd::optional<std::reference_wrapper<const KeyMapping>> InputManager::findCurrentMapping(const bool bAllowMouseWheelEvents, bool filterHidden)
{
	/* If mappings have been updated or context priorities have changed, sort the mappings by priority and whether or not they have meta keys */
	if (mappingsSortRequired())
	{
		keyMappings.sort(contextManager);
		contextManager.clearDirty();
		bMappingsSortOrderDirty = false;
	}

	/* Run through all sorted mappings */
	for (const KeyMapping& keyToProcess : keyMappings)
	{
		/* Skip inappropriate ones when necessary */
		if (isIgnoredMapping(*this, bAllowMouseWheelEvents, keyToProcess, true))
		{
			continue;
		}
		if (filterHidden && keyToProcess.info.type == KeyMappingType::HIDDEN)
		{
			continue;
		}

		/* Execute the action if mapping was hit */
		if (keyToProcess.isActivated())
		{
			return keyToProcess;
		}
	}

	return nullopt;
}
