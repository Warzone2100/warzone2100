/*
	This file is part of Warzone 2100.
	Copyright (C) 2021  Warzone 2100 Project

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
#ifndef _WZ_CRASHHANDLING_PROVIDERS_H_
#define _WZ_CRASHHANDLING_PROVIDERS_H_

#include <string>
#include <nlohmann/json_fwd.hpp>

bool useCrashHandlingProvider(int argc, const char * const *argv, bool& out_debugCrashHandler);
bool initCrashHandlingProvider(const std::string& platformPrefDir, const std::string& defaultLogFilePath, bool debugCrashHandler);
bool shutdownCrashHandlingProvider();

bool crashHandlingProviderSetTag(const std::string& key, const std::string& value);
bool crashHandlingProviderSetContext(const std::string& key, const nlohmann::json& contextDictionary);

bool crashHandlingProviderTestCrash();

#endif //_WZ_CRASHHANDLING_PROVIDERS_H_
