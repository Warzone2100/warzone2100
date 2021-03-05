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
