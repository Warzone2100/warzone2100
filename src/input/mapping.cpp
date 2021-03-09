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

#include "mapping.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h" // For gameTime

#include "keyconfig.h"

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


KeyMapping& KeyMappings::add(const KEY_CODE meta, const KeyMappingInput input, const KeyAction action, const KeyFunctionInfo& info, const KeyMappingSlot slot)
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
	bDirty = true;
	return keyMappings.back();
}

nonstd::optional<std::reference_wrapper<KeyMapping>> KeyMappings::get(const KeyFunctionInfo& info, const KeyMappingSlot slot)
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

std::vector<std::reference_wrapper<KeyMapping>> KeyMappings::find(const KEY_CODE meta, const KeyMappingInput input)
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

bool KeyMappings::remove(const KeyMapping& mappingToRemove)
{
	auto mapping = std::find_if(keyMappings.begin(), keyMappings.end(), [mappingToRemove](const KeyMapping& mapping) {
		return mapping == mappingToRemove;
	});
	if (mapping != keyMappings.end())
	{
		keyMappings.erase(mapping);
		bDirty = true;
		return true;
	}
	return false;
}

std::vector<KeyMapping> KeyMappings::removeConflicting(const KEY_CODE meta, const KeyMappingInput input, const InputContext context)
{
	/* Find any mapping with same keys */
	const auto matches = find(meta, input);
	std::vector<KeyMapping> conflicts;
	for (KeyMapping& mapping : matches)
	{
		/* Clear only if the mapping is for an assignable binding. Do not clear if there is no conflict (different context) */
		const bool bConflicts = mapping.info.context == context;
		if (mapping.info.type == KeyMappingType::ASSIGNABLE && bConflicts)
		{
			conflicts.push_back(mapping);
			remove(mapping);
		}
	}

	return conflicts;
}

bool KeyMappings::isDirty() const
{
	return bDirty;
}

void KeyMappings::clear(nonstd::optional<KeyMappingType> filter)
{
	if (!filter.has_value())
	{
		keyMappings.clear();
	}
	else
	{
		keyMappings.remove_if([filter](const KeyMapping& mapping) {
			return mapping.info.type == filter.value();
		});
	}

	bDirty = true;
}

void KeyMappings::sort(const ContextManager& contexts)
{
	keyMappings.sort([&contexts](const KeyMapping& a, const KeyMapping& b) {
		// Primary sort by priority
		const unsigned int priorityA = contexts.getPriority(a.info.context);
		const unsigned int priorityB = contexts.getPriority(b.info.context);
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
	bDirty = false;
}
