/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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
#ifndef _WZ_UPDATE_MANAGER_H_
#define _WZ_UPDATE_MANAGER_H_

#include "terrain_defs.h"
#include <unordered_set>
#include <string>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

class WzInfoManager {
public:
	static void initialize();
	static void shutdown();
};


struct CompatCheckIssue
{
	enum class Severity
	{
		Warning,
		Critical
	};
	struct ConfigFlags
	{
		std::unordered_set<TerrainShaderQuality> supportedTerrain = {TerrainShaderQuality::CLASSIC, TerrainShaderQuality::MEDIUM, TerrainShaderQuality::NORMAL_MAPPING};
		bool multilobby = true;
	};

	std::string identifier;
	Severity severity = Severity::Warning;
	bool unsupported = false;
	std::string infoLink;
	ConfigFlags configFlags;
};

struct CompatCheckResults
{
	optional<CompatCheckIssue> issue;
	bool successfulCheck = false;

public:
	CompatCheckResults(bool successfulCheck, optional<CompatCheckIssue> issue = nullopt)
	: issue(issue)
	, successfulCheck(successfulCheck)
	{ }

public:
	bool hasIssue() const { return issue.has_value(); }
};

typedef std::function<void (CompatCheckResults results)> CompatCheckResultsHandlerFunc;

// Get the compat check results
// NOTE: resultClosure may be called on any thread at any time - use wzAsyncExecOnMainThread inside your closure if you need to perform tasks on the main thread
void asyncGetCompatCheckResults(CompatCheckResultsHandlerFunc resultClosure);

optional<bool> getVersionCheckNewVersionAvailable();

#endif //_WZ_UPDATE_MANAGER_H_
