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

#ifndef _LIB_FRAMEWORK_WZPATHS_H
#define _LIB_FRAMEWORK_WZPATHS_H

#include <string>
#include <vector>

std::vector<std::string> splitAtAnyDelimiter(const std::string& s, const std::string& delimiters);

std::string getWZInstallPrefix();

class WzPathInfo
{
private:
	WzPathInfo(const std::string &file);
public:
	// Expects a filename / path with "/" as the path separator.
	static WzPathInfo fromPlatformIndependentPath(const std::string& file);
public:
	// Returns the name of the file (excluding the path).
	std::string fileName() const;

	// Returns the file name, including the path.
	std::string filePath() const;

	// Returns the base name of the file (without the path).
	// The base name = all characters in the file up to (but not including) the first '.' character.
	// ex.
	// ```cpp
	//   WzPathInfo info("/autohost/example.js");
	//   auto result = info.baseName(); // result == "example"
	// ```
	std::string baseName() const;

	// Returns the file's path. Does *not* include the file name.
	std::string path() const;

	// Returns the path components as a vector
	std::vector<std::string> pathComponents() const;

private:
	std::string file;
};

#endif // _LIB_FRAMEWORK_WZPATHS_H
