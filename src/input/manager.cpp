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
#include <optional-lite/optional.hpp>

#include "manager.h"
#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/gamelib/gtime.h" // For gameTime

#include "context.h"
#include "config.h"
#include "mapping.h"

#include "../keybind.h"
#include "../keyedit.h"
#include "../display3d.h"   // For playerPos
#include "../qtscript.h"    // For triggerEventKeyPressed


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

void InputManager::resetMappings(bool bForceDefaults)
{
	keyMappings.clear();
	markerKeyFunctions.clear();

	bMappingsSortOrderDirty = true;
	for (unsigned n = 0; n < MAX_PLAYERS; ++n)
	{
		processDebugMappings(n, false);
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
			debug(LOG_WARNING, "Loading key map failed, forcing defaults");
			bForceDefaults = true;
		}
	}

	/* Add in the default mappings if we are forcing defaults (e.g. "reset to defaults" button was pressed from the UI) or loading key map failed. */
	for (const KeyFunctionInfo& info : allKeyFunctionEntries())
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

	saveKeyMap(*this);
}

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

bool InputManager::addDefaultMapping(const KEY_CODE metaCode, const KeyMappingInput input, const KeyAction action, const KeyFunctionInfo& info, const KeyMappingSlot slot)
{
	const auto psMapping = getMapping(info, slot);
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
		removeMapping(*psMapping);
#if defined(__GNUC__) && (__GNUC__ == 5 && __GNUC_MINOR__ == 4)
#pragma GCC diagnostic pop
#endif // __GNUC__

	}

	// Clear the keys from any other mappings
	removeConflictingMappings(metaCode, input, info.context);

	// Set default key mapping
	addMapping(metaCode, input, action, info, slot);
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

	const auto existing = findMappingsForInput(KEY_CODE::KEY_LSHIFT, qKey);
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
			removeMapping(old);
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
		addMapping(KEY_LSHIFT, qKey, KeyAction::PRESSED, info, KeyMappingSlot::PRIMARY);
	}
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
	updateMapMarkers();

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

		/* Skip if the input is already consumed. Handles skips for context/priority and meta-conflicts */
		const auto bIsAlreadyConsumed = consumedInputs.find(keyToProcess.input) != consumedInputs.end();
		if (bIsAlreadyConsumed)
		{
			continue;
		}

		/* Execute the action if mapping was hit */
		if (keyToProcess.isActivated())
		{
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
