/*
 *	This file is part of Warzone 2100.
 *	Copyright (C) 2018-2019  Warzone 2100 Project
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

#include "wzpaths.h"

#include "frame.h"
#include <physfs.h>
#include <vector>

#ifdef WZ_BINDIR
static std::vector<std::string> splitAtAnyDelimiter(const std::string& s, const std::string& delimiters)
{
	std::vector<std::string> v;

	auto pos = s.begin();
	auto end = pos;

	while(end != s.end())
	{
		end = std::find_first_of(pos, s.end(), delimiters.begin(), delimiters.end());
		v.emplace_back(pos, end);
		pos = end + 1;
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
