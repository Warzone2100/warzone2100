/*
	This file is part of Warzone 2100.
	Copyright (C) 2004  Giel van Schijndel
	Copyright (C) 2007-2020  Warzone 2100 Project

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

#ifndef __INCLUDED_VERSION_H__
#define __INCLUDED_VERSION_H__

#include "lib/framework/types.h"
#include "lib/framework/frame.h"
#include <nonstd/optional.hpp>
#include <string>
#include <vector>
#include <algorithm>

const char *version_getVersionString();
const char *version_getFormattedVersionString(bool translated = true); // not thread-safe
std::string version_getVersionedAppDirFolderName();
std::string version_getVersionedModsFolderPath(std::string subFolders = "");
const char* WZ_DECL_PURE version_getLatestTag();

std::string version_getBuildIdentifierReleaseString();
std::string version_getBuildIdentifierReleaseEnvironment();

#define TAGVER_MAX_QUALIF_LEN 16
struct TagVer
{
    uint16_t version[3] = {0};
	char qualifier[TAGVER_MAX_QUALIF_LEN] = {0}; // -beta1, -rc1, (etc)

    TagVer() = default;

    TagVer(uint16_t major_, uint16_t minor_, uint16_t revision_, const char *qualifier_) : version {major_, minor_, revision_}
    {
		strncpy(qualifier, qualifier_, TAGVER_MAX_QUALIF_LEN - 1);
        qualifier[TAGVER_MAX_QUALIF_LEN - 1] = 0;
    }

	TagVer(const std::vector<uint16_t> &v, const char *qualifier_) : version {v[0], v[1], v[2]}
	{
		strncpy(qualifier, qualifier_, TAGVER_MAX_QUALIF_LEN - 1);
        qualifier[TAGVER_MAX_QUALIF_LEN - 1] = 0;
	}

    uint16_t get_major()
    {
        return version[0];
    };
    uint16_t get_minor()
    {
        return version[1];
    };
    uint16_t revision()
    {
        return version[2];
    };

    bool operator < (const TagVer &other)
    {
        return std::lexicographical_compare(version, version + 3, other.version, other.version + 3);
    }

    bool operator == (const TagVer &other)
    {
        return memcmp(version, other.version, sizeof(uint16_t) * 3) == 0;
    }

	bool operator <= (const TagVer &other)
	{
		return *this < other || *this  == other;
	}

	std::string major_minor()
    {
        char tmp[8] = {0}; // shall be enough 999.999
        ASSERT(get_major() <= 999, "incorrect major version: %i", get_major());
        ASSERT(get_minor() <= 999, "incorrect minor version: %i", get_minor());
        snprintf(tmp, 8, "%u.%u", get_major(), get_minor());
        return std::string(tmp);
    }

	bool hasQualifier()
	{
		return qualifier[0] != 0;
	}
};
nonstd::optional<TagVer> version_extractVersionNumberFromTag(const std::string& tag);
#endif // __INCLUDED_VERSION_H__
