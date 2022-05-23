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

#include <set>
#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

bool WZ_PHYSFS_enumerateFiles(const char *dir, const std::function<bool (const char* file)>& enumFunc)
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

bool WZ_PHYSFS_enumerateFolders(const std::string &dir, const std::function<bool (const char* folder)>& enumFunc)
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

bool filenameEndWithExtension(const char *filename, const char *extension)
{
	if (nullptr == filename)
	{
		return false;
	}

	size_t filenameLength = strlen(filename);
	size_t extensionLength = strlen(extension);
	return extensionLength < filenameLength && 0 == strcmp(filename + filenameLength - extensionLength, extension);
}

typedef std::pair<const char*, time_t> SaveTimePair;
struct compareTimes {
	bool operator()(const SaveTimePair &lhs,
					const SaveTimePair &rhs) const {
		return lhs.second < rhs.second;
	}
};

int WZ_PHYSFS_cleanupOldFilesInFolder(const char *path, const char *extension, int fileLimit, const std::function<bool (const char *fileName)>& deleteFileFunction)
{
	ASSERT_OR_RETURN(-1, extension != nullptr, "Null extension");
	CleanupFileEnumFilterFunctions filterFuncs;
	filterFuncs.fileNameFilterFunction = [extension](const char *fileName) -> bool {
		return filenameEndWithExtension(fileName, extension);
	};
	return WZ_PHYSFS_cleanupOldFilesInFolder(path, filterFuncs, fileLimit, deleteFileFunction);
}

int WZ_PHYSFS_cleanupOldFilesInFolder(const char *path, const CleanupFileEnumFilterFunctions& fileFilterFunctions, int fileLimit, const std::function<bool (const char *fileName)>& deleteFileFunction)
{
	ASSERT_OR_RETURN(-1, path != nullptr, "Null path");
	ASSERT_OR_RETURN(-1, deleteFileFunction != nullptr, "No deleteFileFunction");
	char **i, **files;
	files = PHYSFS_enumerateFiles(path);
	ASSERT_OR_RETURN(-1, files, "PHYSFS_enumerateFiles(\"%s\") failed: %s", path, WZ_PHYSFS_getLastError());
	int nfiles = 0;
	for (i = files; *i != nullptr; ++i)
	{
		if (fileFilterFunctions.fileNameFilterFunction != nullptr && !fileFilterFunctions.fileNameFilterFunction(*i))
		{
			continue;
		}
		nfiles++;
	}
	if (nfiles <= fileLimit || nfiles <= 0)
	{
		PHYSFS_freeList(files);
		return 0;
	}

	// too many files
	debug(LOG_SAVE, "found %i matching files in %s, limit is %i", nfiles, path, fileLimit);

	// build a sorted list of file + save time
	std::multiset<SaveTimePair, compareTimes> fileTimes;
	char savefile[PATH_MAX];
	for (i = files; *i != nullptr; ++i)
	{
		if (fileFilterFunctions.fileNameFilterFunction != nullptr && !fileFilterFunctions.fileNameFilterFunction(*i))
		{
			continue;
		}
		/* Gather save-time */
		snprintf(savefile, sizeof(savefile), "%s/%s", path, *i);
		time_t savetime = WZ_PHYSFS_getLastModTime(savefile);
		if (fileFilterFunctions.fileLastModifiedFilterFunction != nullptr && !fileFilterFunctions.fileLastModifiedFilterFunction(savetime))
		{
			continue;
		}
		fileTimes.insert(SaveTimePair{*i, savetime});
	}

	// now delete the oldest
	int numFilesDeleted = 0;
	while (nfiles > fileLimit && !fileTimes.empty())
	{
		const char* pOldestFilename = fileTimes.begin()->first;
		char oldestSavePath[PATH_MAX];
		snprintf(oldestSavePath, sizeof(oldestSavePath), "%s/%s", path, pOldestFilename);
		if (deleteFileFunction(oldestSavePath))
		{
			++numFilesDeleted;
			--nfiles;
		}
		fileTimes.erase(fileTimes.begin());
		if (fileLimit < 0)
		{
			break;
		}
	}
	fileTimes.clear();
	PHYSFS_freeList(files);
	return numFilesDeleted;
}
