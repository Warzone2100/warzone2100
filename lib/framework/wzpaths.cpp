/*
 *	This file is part of Warzone 2100.
 *	Copyright (C) 2018-2020  Warzone 2100 Project
 *
 *	Warzone 2100 is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Warzone 2100 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with Warzone 2100; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "frame.h"
#include "wzpaths.h"

#include <physfs.h>
#include <vector>
#include <algorithm>

#ifdef WZ_BINDIR
static std::vector<std::string> splitAtAnyDelimiter(const std::string& s, const std::string& delimiters)
{
	std::vector<std::string> v;

	auto pos = s.begin();
	while(pos != s.end())
	{
		auto end = std::find_first_of(pos, s.end(), delimiters.begin(), delimiters.end());
		v.emplace_back(pos, end);
		pos = end;
		if (pos != s.end())
		{
			++pos;
		}
	}

	return v;
}
#endif

std::string getWZInstallPrefix()
{
	const std::string dirSeparator(PHYSFS_getDirSeparator());
	ASSERT(dirSeparator.length() > 0, "PHYSFS_getDirSeparator returned an empty string");

	// Construct the install PREFIX path
	std::string prefixDir(PHYSFS_getBaseDir());
	while (!prefixDir.empty() && (prefixDir.rfind(dirSeparator, std::string::npos) == (prefixDir.length() - dirSeparator.length())))
	{
		prefixDir.resize(prefixDir.length() - dirSeparator.length()); // Remove trailing path separators
	}
	size_t binDirComponentsCount = 1;
#ifdef WZ_BINDIR
	// Trim off as many path components as are in WZ_BINDIR
	std::string binDir(WZ_BINDIR);
	std::vector<std::string> binDirComponents = splitAtAnyDelimiter(binDir, "/");
	binDirComponentsCount = std::count_if(binDirComponents.begin(), binDirComponents.end(), [](const std::string &s) { return !s.empty() && (s != "."); });
	ASSERT(binDirComponentsCount > 0, "WZ_BINDIR unexpectedly has zero components?: \"%s\"", WZ_BINDIR);
#endif
	for (size_t i = 0; i < binDirComponentsCount; ++i)
	{
		size_t lastSlash = prefixDir.rfind(dirSeparator, std::string::npos);
		if (lastSlash != std::string::npos)
		{
			prefixDir = prefixDir.substr(0, lastSlash); // Trim off the last path component
		}
	}

	return prefixDir;
}

// MARK: - WzPathInfo

WzPathInfo::WzPathInfo(const std::string &file)
: file(file)
{ }

WzPathInfo WzPathInfo::fromPlatformIndependentPath(const std::string& file)
{
	return WzPathInfo(file);
}

// Returns the name of the file (excluding the path).
std::string WzPathInfo::fileName() const
{
	auto lastSlashPos = file.rfind('/');
	if (std::string::npos == lastSlashPos)
	{
		// no slash - treat the entire string as the filename
		return file;
	}
	return file.substr(lastSlashPos + 1);
}

// Returns the file name, including the path.
std::string WzPathInfo::filePath() const
{
	return file;
}

// Returns the base name of the file (without the path).
// The base name = all characters in the file up to (but not including) the first '.' character.
// ex.
// ```cpp
//   WzPathInfo info("/autohost/example.js");
//   auto result = info.baseName(); // result == "example"
// ```
std::string WzPathInfo::baseName() const
{
	auto result = fileName();
	auto firstPeriodPos = result.find('.');
	if (std::string::npos == firstPeriodPos)
	{
		// no period
		return result;
	}
	return result.substr(0, firstPeriodPos);
}

// Returns the file's path. Does *not* include the file name.
std::string WzPathInfo::path() const
{
	auto lastSlashPos = file.rfind('/');
	if (std::string::npos == lastSlashPos)
	{
		// no slash - return "." as the path
		return ".";
	}
	return file.substr(0, std::max<size_t>(lastSlashPos, (size_t)1));
}
