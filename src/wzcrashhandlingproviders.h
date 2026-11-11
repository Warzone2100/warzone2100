// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2021-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
#ifndef _WZ_CRASHHANDLING_PROVIDERS_H_
#define _WZ_CRASHHANDLING_PROVIDERS_H_

#include <string>
#include <nlohmann/json_fwd.hpp>

bool useCrashHandlingProvider(int argc, const char * const *argv, bool& out_debugCrashHandler);
bool initCrashHandlingProvider(const std::string& platformPrefDir, const std::string& defaultLogFilePath, bool debugCrashHandler);
bool shutdownCrashHandlingProvider();

bool crashHandlingProviderSetTag(const std::string& key, const std::string& value);
bool crashHandlingProviderSetContext(const std::string& key, const nlohmann::json& contextDictionary);

bool crashHandlingProviderCaptureException(const char* type, const char* value, const std::string& description, bool captureStackTrace, bool handled = true, const nlohmann::json *additionalData = nullptr);

bool crashHandlingProviderTestCrash();

#endif //_WZ_CRASHHANDLING_PROVIDERS_H_
