// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#include "wzsandbox.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzpaths.h"
#include "lib/sandbox/sandbox.h"

#include <physfs.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

/* Always use fallbacks on Windows */
#if defined(WZ_OS_WIN)
#  undef WZ_DATADIR
#endif

#if !defined(WZ_DATADIR)
#  define WZ_DATADIR "data"
#endif

static const wz::sandbox::Tier defaultTier = wz::sandbox::Tier::Off;

/// Returns the value of --<name>=<value>, or an empty string if the option is absent. Mirrors the game's
/// own parser, which takes no other form of an option with a value and strips quotes a shell left behind.
static std::string findOptionValue(int argc, const char * const *argv, const char* name)
{
	const size_t nameLen = strlen(name);
	std::string value;

	for (int i = 1; i < argc; ++i)
	{
		if (argv[i] == nullptr || strncmp(argv[i], "--", 2) != 0)
		{
			continue;
		}
		const char* option = argv[i] + 2;
		if (strncmp(option, name, nameLen) != 0 || option[nameLen] != '=')
		{
			continue;
		}
		value = option + nameLen + 1;   // the last occurrence wins
	}

	if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
	{
		value = value.substr(1, value.size() - 2);
	}

	return value;
}

/// Returns the directory part of a path, or an empty string for a bare filename.
static std::string parentDirectory(const std::string& path)
{
	const size_t split = path.find_last_of("/\\");
	return (split != std::string::npos) ? path.substr(0, split) : std::string();
}

static void addPath(std::vector<std::string>& paths, const std::string& path)
{
	if (!path.empty())
	{
		paths.push_back(path);
	}
}

static void addPath(std::vector<std::string>& paths, const char* path)
{
	if (path != nullptr)
	{
		addPath(paths, std::string(path));
	}
}

static wz::sandbox::Tier requestedTier(int argc, const char * const *argv)
{
	wz::sandbox::Tier tier = defaultTier;

	// parseTier() only writes on success, so an unrecognized value leaves the previous choice standing.
	const char* envValue = getenv("WZ_SANDBOX");
	if (envValue != nullptr && !wz::sandbox::parseTier(envValue, tier))
	{
		debug(LOG_ERROR, "Ignoring unrecognized WZ_SANDBOX value: %s", envValue);
	}

	const std::string argValue = findOptionValue(argc, argv, "sandbox");
	if (!argValue.empty() && !wz::sandbox::parseTier(argValue.c_str(), tier))
	{
		debug(LOG_ERROR, "Ignoring unrecognized --sandbox value: %s", argValue.c_str());
	}

	return tier;
}

static wz::sandbox::Plan buildPlan(int argc, const char * const *argv, const std::string& platformPrefDir)
{
	wz::sandbox::Plan plan;

	const std::string prefix = getWZInstallPrefix();
	const std::string separator(PHYSFS_getDirSeparator());

	// The candidates scanDataDirs() probes
	addPath(plan.readRoots, prefix + separator + "data");
	addPath(plan.readRoots, prefix + separator + "share" + separator + "warzone2100");
	const std::string dataDir = WZ_DATADIR;
	if (!dataDir.empty())
	{
#if !defined(WZ_DATADIR_ISABSOLUTE)
		addPath(plan.readRoots, prefix + separator + dataDir);
#else
		addPath(plan.readRoots, dataDir);
#endif
	}
	addPath(plan.readRoots, findOptionValue(argc, argv, "datadir"));

#if defined(WZ_LOCALEDIR)
# if !defined(WZ_LOCALEDIR_ISABSOLUTE)
	addPath(plan.readRoots, prefix + separator + WZ_LOCALEDIR);
# else
	addPath(plan.readRoots, WZ_LOCALEDIR);
# endif
#endif

	// The platform pref dir is needed even with --configdir, because the crash-handling provider uses it
	addPath(plan.writeRoots, platformPrefDir);
	addPath(plan.writeRoots, findOptionValue(argc, argv, "configdir"));

	const std::string cmdInterface = findOptionValue(argc, argv, "enablecmdinterface");
	const std::string unixSocketPrefix = "unixsocket:";
	if (cmdInterface.compare(0, unixSocketPrefix.size(), unixSocketPrefix) == 0)
	{
		addPath(plan.socketDirs, parentDirectory(cmdInterface.substr(unixSocketPrefix.size())));
	}

	// Covers the <path>.detail.txt companion file as well
	addPath(plan.writeFileParents, parentDirectory(findOptionValue(argc, argv, "gamestate-crc-trace")));

	// crashpad_handler is spawned from the directory holding the game binary
	addPath(plan.execRoots, PHYSFS_getBaseDir());

	return plan;
}

void wzSandboxEarlySetup(int argc, const char * const *argv, const std::string& platformPrefDir)
{
	wz::sandbox::confine(buildPlan(argc, argv, platformPrefDir), requestedTier(argc, argv));
}
