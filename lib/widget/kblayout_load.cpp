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
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/
/** @file
 *  Loading on-screen keyboard layouts and accents from the game data
 *  (apart from kblayout.cpp, which stays free of file-system dependencies).
 */

#include "kblayout.h"

#include "lib/framework/frame.h"
#include "lib/framework/file.h"

namespace wzkb
{

static const char* const LAYOUT_DIRECTORY = "keyboards/";

bool loadLayout(const std::string& id, LayoutData& output)
{
	// layout ids are plain names - never let one escape the keyboards directory
	if (id.empty() || id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-") != std::string::npos)
	{
		debug(LOG_ERROR, "Invalid keyboard layout id: %s", id.c_str());
		return false;
	}
	const std::string path = std::string(LAYOUT_DIRECTORY) + id + ".json";
	std::vector<char> buffer;
	if (!loadFileToBufferVector(path.c_str(), buffer, false, false))
	{
		debug(LOG_WARNING, "Could not load keyboard layout: %s", path.c_str());
		return false;
	}
	std::string error;
	if (!parseLayoutJSON(buffer.data(), buffer.size(), output, &error))
	{
		debug(LOG_ERROR, "Invalid keyboard layout %s: %s", path.c_str(), error.c_str());
		return false;
	}
	return true;
}

bool loadAccents(AccentData& output)
{
	const std::string path = std::string(LAYOUT_DIRECTORY) + "accents.json";
	std::vector<char> buffer;
	if (!loadFileToBufferVector(path.c_str(), buffer, false, false))
	{
		debug(LOG_WARNING, "Could not load keyboard accents: %s", path.c_str());
		return false;
	}
	std::string error;
	if (!parseAccentJSON(buffer.data(), buffer.size(), output, &error))
	{
		debug(LOG_ERROR, "Invalid keyboard accents %s: %s", path.c_str(), error.c_str());
		return false;
	}
	return true;
}

} // namespace wzkb
