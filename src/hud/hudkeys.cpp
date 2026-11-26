// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#include "hudkeys.h"

#include "lib/framework/frame.h"
#include "lib/framework/gamepad_input.h"

#include "../input/manager.h"
#include "../display.h"

std::string hudKeyBindingText(const std::string &keyFunctionName)
{
	auto &mappings = gInputManager.mappings();
	if (isGamepadActiveInput())
	{
		auto gamepad = mappings.get(keyFunctionName, KeyMappingSlot::GAMEPAD);
		if (gamepad.has_value())
		{
			return gamepad.value().get().toString();
		}
	}
	for (auto slot : {KeyMappingSlot::PRIMARY, KeyMappingSlot::SECONDARY})
	{
		auto mapping = mappings.get(keyFunctionName, slot);
		if (mapping.has_value())
		{
			return mapping.value().get().toString();
		}
	}
	return std::string();
}

std::string hudTipWithKeyBinding(const std::string &tip, const std::string &keyFunctionName)
{
	const std::string key = hudKeyBindingText(keyFunctionName);
	if (key.empty() || tip.find(key) != std::string::npos)
	{
		return tip;
	}
	if (tip.empty())
	{
		return key;
	}
	return astringf("%s (%s)", tip.c_str(), key.c_str());
}
