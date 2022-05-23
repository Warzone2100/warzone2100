/*
	This file is part of Warzone 2100.
	Copyright (C) 2021-2022  Warzone 2100 Project

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

#include "map_jsonhelpers.h"
#include "../include/wzmaplib/map.h"
#include "map_internal.h"

namespace WzMap {

// MARK: - Helper functions for loading / saving JSON files

optional<nlohmann::json> loadJsonObjectFromFile(const std::string& filename, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	const auto &path = filename.c_str();
	std::vector<char> data;
	if (!mapIO.loadFullFile(filename, data))
	{
		return nullopt;
	}
	if (data.empty())
	{
		debug(pCustomLogger, LOG_ERROR, "Empty file: %s", path);
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
		debug(pCustomLogger, LOG_ERROR, "JSON document from %s is invalid: %s", path, e.what());
		return nullopt;
	}
	catch (...) {
		debug(pCustomLogger, LOG_ERROR, "Unexpected exception parsing JSON %s", path);
		return nullopt;
	}
	if (mRoot.is_null())
	{
		debug(pCustomLogger, LOG_ERROR, "JSON document from %s is null", path);
		return nullopt;
	}
	if (!mRoot.is_object())
	{
		debug(pCustomLogger, LOG_ERROR, "JSON document from %s is not an object. Read: \n%s", path, data.data());
		return nullopt;
	}
	data.clear();

	return mRoot;
}

bool saveOrderedJsonObjectToFile(const nlohmann::ordered_json& jsonObj, const std::string& filename, IOProvider& mapIO, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	std::string jsonStr = jsonObj.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::ignore);
#if SIZE_MAX > UINT32_MAX
	if (jsonStr.size() > static_cast<size_t>(std::numeric_limits<uint32_t>::max()))
	{
		debug(pCustomLogger, LOG_ERROR, "jsonString.size (%zu) exceeds uint32_t::max", jsonStr.size());
		return false;
	}
#endif
	return mapIO.writeFullFile(filename, jsonStr.c_str(), static_cast<uint32_t>(jsonStr.size()));
}

} // namespace WzMap
