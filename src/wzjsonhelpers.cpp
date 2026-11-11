// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2024-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/**
 * @file wzjsonhelpers.cpp
 */

#include "wzjsonhelpers.h"
#include "lib/framework/frame.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/file.h"

optional<nlohmann::json> wzLoadJsonObjectFromFile(const std::string& filePath, bool quiet /*= false*/)
{
	if (!PHYSFS_exists(filePath.c_str()))
	{
		return nullopt;
	}
	std::vector<char> data;
	if (!loadFileToBufferVector(filePath.c_str(), data, false, true))
	{
		return nullopt;
	}

	if (data.empty())
	{
		if (!quiet) { debug(LOG_ERROR, "Empty file: %s", filePath.c_str()); }
		return nullopt;
	}
	if (data.back() != 0)
	{
		data.push_back('\0'); // always ensure data is null-terminated
	}

	// parse JSON
	nlohmann::json mRoot;
	try {
		mRoot = nlohmann::json::parse(data.begin(), data.end() - 1);
	}
	catch (const std::exception &e) {
		if (!quiet) { debug(LOG_ERROR, "JSON document from %s is invalid: %s", filePath.c_str(), e.what()); }
		return nullopt;
	}
	catch (...) {
		if (!quiet) { debug(LOG_ERROR, "Unexpected exception parsing JSON %s", filePath.c_str()); }
		return nullopt;
	}
	if (mRoot.is_null())
	{
		if (!quiet) { debug(LOG_ERROR, "JSON document from %s is null", filePath.c_str()); }
		return nullopt;
	}
	if (!mRoot.is_object())
	{
		if (!quiet) { debug(LOG_ERROR, "JSON document from %s is not an object. Read: \n%s", filePath.c_str(), data.data()); }
		return nullopt;
	}
	data.clear();

	return mRoot;
}
