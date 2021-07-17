/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
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

#include "lib/framework/frame.h"
#include "lib/framework/debug.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/stdio_ext.h"
#include "version.h"
#include "stringdef.h"

#include <algorithm>
#include <vector>
#include <regex>
#include <utility>
#include <optional-lite/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

#include "build_tools/autorevision.h"

static const char vcs_branch_cstr[] = VCS_BRANCH;
static const char vcs_tag[] = VCS_TAG;

struct TagVer
{
	std::string major;
	std::string minor;
	std::string revision;
	std::string qualifier; // -beta1, -rc1, (etc)
};

static optional<TagVer> extractVersionNumberFromTag(const std::string& tag)
{
	std::string tagStr = tag;

	// Remove "v/" or "v" prefix (as in "v3.2.2"), if present
	const std::vector<std::string> prefixesToRemove = {"v/", "v"};
	for (const auto& prefix : prefixesToRemove)
	{
		if (tagStr.rfind(prefix, 0) == 0) {
			tagStr = tagStr.substr(prefix.size());
		}
	}

	std::smatch base_match;
	const std::regex semver_regex("^([0-9]+)(.[0-9]+)?(.[0-9]+)?(.*)?");
	if (!std::regex_match(tagStr, base_match, semver_regex))
	{
		// regex failure
		return nullopt;
	}

	TagVer result;
	// base_match[0] is the whole string
	const size_t matchCount = base_match.size();
	switch (matchCount)
	{
		case 5:
			result.qualifier = base_match[4].str();
			// fallthrough
		case 4:
			result.revision = base_match[3].str().substr(1); // remove the "." prefix
			// fallthrough
		case 3:
			result.minor = base_match[2].str().substr(1); // remove the "." prefix
			result.major = base_match[1].str();
			break;
		default:
			// failure
			return nullopt;
			break;
	}

	return result;
}

/** Obtain the versioned application-data / config writedir folder name
 *  If on a tag, this is "Warzone 2100 <tag>" / "warzone2100-<tag>"
 *  If not on a tag, this is "Warzone 2100 <branch>" / "warzone2100-<branch>"
 *  If no branch is defined, this is "Warzone 2100 <VCS_EXTRA>" / "warzone2100-<VCS_EXTRA>"
 */
std::string version_getVersionedAppDirFolderName()
{
	std::string versionedWriteDirFolderName;

#if defined(WZ_OS_WIN) || defined(WZ_OS_MAC)
	versionedWriteDirFolderName = "Warzone 2100 ";
#else
	versionedWriteDirFolderName = "warzone2100-";
#endif

	if (strlen(vcs_tag))
	{
		optional<TagVer> tagVersion = extractVersionNumberFromTag(vcs_tag);
		if (tagVersion.has_value())
		{
			// If tag is actually a version number (which it should be!)
			// Use only MAJOR.MINOR (ignoring revision) for the appdir folder name
			if (tagVersion.value().major == "3" && tagVersion.value().minor == "4")
			{
				// EXCEPTION: Use "3.4.0" for any 3.4.x release, for compatibility with 3.4.0 (which did not have this code)
				versionedWriteDirFolderName += "3.4.0";
			}
			else
			{
				// Normal case - use only "MAJOR.MINOR"
				versionedWriteDirFolderName += tagVersion.value().major + "." + tagVersion.value().minor;
			}
		}
		else
		{
			// vcs_tag does not resemble a version number - just use it directly
			versionedWriteDirFolderName += vcs_tag;
		}
	}
	else if (strlen(vcs_branch_cstr))
	{
#if defined(WZ_USE_MASTER_BRANCH_APP_DIR)
		// To ease testing new branches with existing files
		// default to using "master" as the branch name
		// if WZ_USE_MASTER_BRANCH_APP_DIR is defined
		versionedWriteDirFolderName += "master";
#else
		versionedWriteDirFolderName += vcs_branch_cstr;
#endif
	}
	else
	{
		// not a branch or a tag, so we are detached most likely.
		std::string vcs_extra_str = VCS_EXTRA;
		// remove any spaces from VCS_EXTRA
		vcs_extra_str.erase(std::remove(vcs_extra_str.begin(), vcs_extra_str.end(), ' '), vcs_extra_str.end());
		versionedWriteDirFolderName += vcs_extra_str;
	}
	return versionedWriteDirFolderName;
}

/** Composes a nicely formatted version string.
* Determine if we are on a tag (which will NOT show the hash)
* or a branch (which WILL show the hash)
* or in a detached state (which WILL show the hash)
*/
const char *version_getVersionString()
{
	static const char *version_string = nullptr;

	if (version_string == nullptr)
	{
		if (strlen(vcs_tag))
		{
			version_string = vcs_tag;
		}
		else if (strlen(vcs_branch_cstr))
		{
			version_string = (VCS_BRANCH " " VCS_SHORT_HASH);
		}
		else
		{
			// not a branch or a tag, so we are detached most likely.
			version_string = VCS_EXTRA;
		}
	}

	return version_string;
}

static std::pair<std::string, std::string> version_getBuildReleaseData()
{
	static std::string buildReleaseStr;
	static std::string buildReleaseEnvironmentStr;
	if (buildReleaseStr.empty() || buildReleaseEnvironmentStr.empty())
	{
		buildReleaseStr = "warzone2100@";
		if (strlen(vcs_tag))
		{
			optional<TagVer> tagVersion = extractVersionNumberFromTag(vcs_tag);
			if (tagVersion.has_value() && !tagVersion.value().qualifier.empty())
			{
				buildReleaseEnvironmentStr = "preview";
			}
			else
			{
				buildReleaseEnvironmentStr = "release";
			}
			// always use the tag directlry
			buildReleaseStr += vcs_tag;
		}
		else
		{
			// if not a tag, use the full commit hash
			buildReleaseStr += VCS_FULL_HASH;
			buildReleaseEnvironmentStr = "development";
		}
	}
	return std::pair<std::string, std::string>(buildReleaseStr, buildReleaseEnvironmentStr);
}

// Should follow the form:
// - warzone2100@TAG_NAME (for tagged builds)
// - warzone2100@FULL_COMMIT_HASH (for other builds)
std::string version_getBuildIdentifierReleaseString()
{
	auto result = version_getBuildReleaseData();
	return result.first;
}

// For tagged builds this will return either:
//	- "release" (for release tags)
//	- "preview" (for tags with a trailing qualifier, like "4.0.0-beta1")
// For other builds, this will return:
//	- "development"
std::string version_getBuildIdentifierReleaseEnvironment()
{
	auto result = version_getBuildReleaseData();
	return result.second;
}

/** Composes a nicely formatted version string.
*
*/
const char *version_getFormattedVersionString(bool translated /* = true */)
{
	static char versionString[MAX_STR_LENGTH] = {'\0'};

	// Compose the working copy state string
#if (VCS_WC_MODIFIED)
	// TRANSLATORS: Printed when compiling with uncommitted changes
	const char *wc_state = (translated) ? _(" (modified locally)") : " (modified locally)";
#else
	const char *wc_state = "";
#endif
	// Compose the build type string
#ifdef DEBUG
	// TRANSLATORS: Printed in Debug builds
	const char *build_type = (translated) ? _(" - DEBUG") : " - DEBUG";
#else
	const char *build_type = "";
#endif

	// Construct the version string
	// TRANSLATORS: This string looks as follows when expanded.
	// "Version: <version name/number>, <working copy state>,
	// Built: <BUILD DATE><BUILD TYPE>"
	snprintf(versionString, MAX_STR_LENGTH, (translated) ? _("Version: %s,%s Built: %s%s") : "Version: %s,%s Built: %s%s", version_getVersionString(), wc_state, getCompileDate(), build_type);

	return versionString;
}
