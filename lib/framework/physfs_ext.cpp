/*
	This file is part of Warzone 2100.
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

#include "physfs_ext.h"
#include "string_ext.h"
#include "frame.h"

#include <optional-lite/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

bool WZ_PHYSFS_enumerateFiles(const char *dir, const std::function<bool (char* file)>& enumFunc)
{
	char **files = PHYSFS_enumerateFiles(dir);
	if (!files)
	{
		debug(LOG_ERROR, "PHYSFS_enumerateFiles(\"%s\") failed: %s", dir, WZ_PHYSFS_getLastError());
		return false;
	}
	for (char **i = files; *i != nullptr; ++i)
	{
		if (!enumFunc(*i))
		{
			break;
		}
	}
	PHYSFS_freeList(files);
	return true;
}

bool WZ_PHYSFS_enumerateFolders(const std::string &dir, const std::function<bool (char* folder)>& enumFunc)
{
	char **files = PHYSFS_enumerateFiles(dir.c_str());
	if (!files)
	{
		debug(LOG_ERROR, "PHYSFS_enumerateFiles(\"%s\") failed: %s", dir.c_str(), WZ_PHYSFS_getLastError());
		return false;
	}
	std::string baseDir = dir;
	if (!strEndsWith(baseDir, "/"))
	{
		baseDir += "/";
	}
	std::string isDir;
	for (char **i = files; *i != nullptr; ++i)
	{
		isDir = baseDir;
		isDir.append(*i);
		if (!WZ_PHYSFS_isDirectory(isDir.c_str()))
		{
			continue;
		}
		if (!enumFunc(*i))
		{
			break;
		}
	}
	PHYSFS_freeList(files);
	return true;
}

bool WZ_PHYSFS_createPlatformPrefDir(const WzString& basePath, const WzString& appendPath)
{
	// Get the existing writeDir if any (to properly reset it after)
	optional<std::string> originalWriteDir;
	const char* pTmp = PHYSFS_getWriteDir();
	if (pTmp)
	{
		originalWriteDir = std::string(pTmp);
	}
	pTmp = nullptr;

	// Create the folders within the basePath if they don't exist

	if (!PHYSFS_setWriteDir(basePath.toUtf8().c_str())) // Workaround for PhysFS not creating the writedir as expected.
	{
		debug(LOG_FATAL, "Error setting write directory to \"%s\": %s",
			  basePath.toUtf8().c_str(), WZ_PHYSFS_getLastError());
		return false;
	}

	WzString currentBasePath = basePath;
	const std::vector<WzString> appendPaths = appendPath.split(PHYSFS_getDirSeparator());
	for (const auto &folder : appendPaths)
	{
		if (!PHYSFS_mkdir(folder.toUtf8().c_str()))
		{
			debug(LOG_FATAL, "Error creating directory \"%s\" in \"%s\": %s",
				  folder.toUtf8().c_str(), PHYSFS_getWriteDir(), WZ_PHYSFS_getLastError());
			return false;
		}

		currentBasePath += PHYSFS_getDirSeparator();
		currentBasePath += folder;

		if (!PHYSFS_setWriteDir(currentBasePath.toUtf8().c_str())) // Workaround for PhysFS not creating the writedir as expected.
		{
			debug(LOG_FATAL, "Error setting write directory to \"%s\": %s",
				  currentBasePath.toUtf8().c_str(), WZ_PHYSFS_getLastError());
			return false;
		}
	}

	// Reset to original write dir
	PHYSFS_setWriteDir((originalWriteDir.has_value()) ? originalWriteDir.value().c_str() : nullptr);

	return true;
}
