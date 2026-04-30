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
#include <cstdlib>
#include <cstring>
#include <vector>
#include <regex>
#include <utility>
#include <sstream>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#include <sys/sysctl.h>
#endif

using nonstd::optional;
using nonstd::nullopt;

#include "build_tools/autorevision.h"

[[maybe_unused]] static const char vcs_branch_cstr[] = VCS_BRANCH;
static const char vcs_tag[] = VCS_TAG;
#if defined(WZ_OS_IOS)
#if defined(WZ_IOS_DEBUG_JIT)
static const char ios_release_channel_cstr[] = "0.1-beta-ios-debug-jit";
#else
static const char ios_release_channel_cstr[] = "0.1-beta-ios";
#endif
#endif

#if defined(WZ_OS_IOS)
static std::string getAppleSysctlString(const char *name)
{
	size_t size = 0;
	if (sysctlbyname(name, nullptr, &size, nullptr, 0) != 0 || size == 0)
	{
		return {};
	}
	std::string buffer(size, '\0');
	if (sysctlbyname(name, buffer.data(), &size, nullptr, 0) != 0 || size == 0)
	{
		return {};
	}
	if (!buffer.empty() && buffer.back() == '\0')
	{
		buffer.pop_back();
	}
	return buffer;
}

static int extractAppleFamilyNumber(const std::string& hardwareId, const char *prefix)
{
	if (hardwareId.rfind(prefix, 0) != 0)
	{
		return -1;
	}
	size_t start = strlen(prefix);
	size_t end = hardwareId.find(',', start);
	std::string numericPart = hardwareId.substr(start, end - start);
	return numericPart.empty() ? -1 : atoi(numericPart.c_str());
}

static std::string appleChipForHardwareId(const std::string& hardwareId)
{
	const int iPhoneFamily = extractAppleFamilyNumber(hardwareId, "iPhone");
	if (iPhoneFamily >= 0)
	{
		if (iPhoneFamily <= 7) return "Apple A8";
		if (iPhoneFamily == 8) return "Apple A9";
		if (iPhoneFamily == 9 || iPhoneFamily == 10) return "Apple A10";
		if (iPhoneFamily == 11) return "Apple A12";
		if (iPhoneFamily == 12) return "Apple A13";
		if (iPhoneFamily == 13) return "Apple A14";
		if (iPhoneFamily == 14) return "Apple A15";
		if (iPhoneFamily == 15) return "Apple A16";
		if (iPhoneFamily >= 16) return "Apple A17 Pro";
	}

	const int iPadFamily = extractAppleFamilyNumber(hardwareId, "iPad");
	if (iPadFamily >= 0)
	{
		if (iPadFamily <= 5) return "Apple A8X";
		if (iPadFamily == 6 || iPadFamily == 7) return "Apple A10";
		if (iPadFamily == 8) return "Apple A12X/A12Z";
		if (iPadFamily == 11) return "Apple A12";
		if (iPadFamily == 13) return "Apple M1";
		if (iPadFamily == 14) return "Apple M2";
		if (iPadFamily >= 16) return "Apple M4";
	}

	if (hardwareId.rfind("MacBookAir10,", 0) == 0
		|| hardwareId.rfind("MacBookPro17,", 0) == 0
		|| hardwareId.rfind("Macmini9,", 0) == 0
		|| hardwareId.rfind("iMac21,", 0) == 0)
	{
		return "Apple M1";
	}
	if (hardwareId.rfind("Mac14,", 0) == 0 || hardwareId.rfind("MacBookAir15,", 0) == 0 || hardwareId.rfind("Mac14", 0) == 0)
	{
		return "Apple M2";
	}
	if (hardwareId.rfind("Mac15,", 0) == 0)
	{
		return "Apple M3";
	}

	return "Apple Silicon";
}
#endif

optional<TagVer> version_extractVersionNumberFromTag(const std::string& tag)
{
    std::istringstream parser(tag);
	// Remove "v/" or "v" prefix (as in "v3.2.2"), if present
    if (parser.peek() == 'v')  parser.get(); // skip
    if (parser.peek() == '/')  parser.get(); // skip
	TagVer result;
	parser >> result.version[0];
    if (parser.fail())
    {
        return nullopt;
    }
	for (int i = 1; i < 3; i++)
    {
        parser.get(); // skip any separator
        parser >> result.version[i];
        if (parser.fail())
        {
            return nullopt;
        }
    }
	if (!parser.eof())
		{
			// it has "-rc/beta1.." suffix 
			if (parser.peek() == '-') parser.get(); // skip
			parser.read(result.qualifier, TAGVER_MAX_QUALIF_LEN - 1);
			result.qualifier[TAGVER_MAX_QUALIF_LEN - 1] = 0;
			if (parser.fail() && !parser.eof())
			{
				return nullopt;
			}
			// must terminate now
			if (!parser.eof())
			{
				return nullopt;
			}
		}
	return result;
}

/** Obtain the versioned application-data / config writedir folder name
 *  If on a tag, this is "Warzone 2100" / "warzone2100"
 *  If not on a tag, this is "Warzone 2100 <branch>" / "warzone2100-<branch>"
 *  If no branch is defined, this is "Warzone 2100 <VCS_EXTRA>" / "warzone2100-<VCS_EXTRA>"
 */
std::string version_getVersionedAppDirFolderName()
{
#if defined(WZ_OS_IOS)
	return "Warzone2100 0.1-beta-ios";
#else
	std::string versionedWriteDirFolderName;
	std::string versionedWriteDirFolderNameSuffix;

#if defined(WZ_OS_WIN) || defined(WZ_OS_MAC)
	versionedWriteDirFolderName = "Warzone 2100";
#else
	versionedWriteDirFolderName = "warzone2100";
#endif

	if (strlen(vcs_tag))
	{
		// release builds now use a stable config directory name, with no suffix
		versionedWriteDirFolderNameSuffix.clear();
	}
	else if (strlen(vcs_branch_cstr))
	{
#if defined(WZ_USE_MASTER_BRANCH_APP_DIR)
		// To ease testing new branches with existing files
		// default to using "master" as the branch name
		// if WZ_USE_MASTER_BRANCH_APP_DIR is defined
		versionedWriteDirFolderNameSuffix = "master";
#else
		versionedWriteDirFolderNameSuffix = vcs_branch_cstr;
#endif
	}
	else
	{
		// not a branch or a tag, so we are detached most likely.
		std::string vcs_extra_str = VCS_EXTRA;
		// remove any spaces from VCS_EXTRA
		vcs_extra_str.erase(std::remove(vcs_extra_str.begin(), vcs_extra_str.end(), ' '), vcs_extra_str.end());
		versionedWriteDirFolderNameSuffix = vcs_extra_str;
	}

	if (!versionedWriteDirFolderNameSuffix.empty())
	{
#if defined(WZ_OS_WIN) || defined(WZ_OS_MAC)
		versionedWriteDirFolderName += " ";
#else
		versionedWriteDirFolderName += "-";
#endif
		versionedWriteDirFolderName += versionedWriteDirFolderNameSuffix;
	}
	return versionedWriteDirFolderName;
#endif
}

/** Obtain the versioned application-data / config "mods" folder path
 *  If on a tag, this is "mods/<tag version>/"
 *  If not on a tag, this is "mods/" (as non-tagged builds have versioned parent config folders)
 */
std::string version_getVersionedModsFolderPath(std::string subFolders /*= ""*/)
{
	std::string versionedModsFolderPath;
	if (strlen(VCS_TAG))
	{
		versionedModsFolderPath = "mods/" VCS_TAG;
	}
	else
	{
		versionedModsFolderPath = "mods";
	}
	if (!subFolders.empty())
	{
		versionedModsFolderPath += "/";
		versionedModsFolderPath += subFolders;
	}
	return versionedModsFolderPath;
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
#if defined(WZ_OS_IOS)
		version_string = ios_release_channel_cstr;
#else
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
#endif
	}

	return version_string;
}

std::string version_getPlatformDisplayString()
{
#if defined(WZ_OS_IOS)
	std::string hardwareId;
#if TARGET_OS_SIMULATOR
	hardwareId = getAppleSysctlString("hw.model");
	if (hardwareId.empty())
	{
		hardwareId = getAppleSysctlString("hw.machine");
	}
	std::string cpuName = appleChipForHardwareId(hardwareId);
	if (!hardwareId.empty())
	{
		return astringf("Simulator / %s (%s)", cpuName.c_str(), hardwareId.c_str());
	}
	return astringf("Simulator / %s", cpuName.c_str());
#else
	hardwareId = getAppleSysctlString("hw.machine");
	std::string cpuName = appleChipForHardwareId(hardwareId);
	if (!hardwareId.empty())
	{
		return astringf("%s (%s)", cpuName.c_str(), hardwareId.c_str());
	}
	return cpuName;
#endif
#else
	return {};
#endif
}

const char *version_getLatestTag()
{
	if(strlen(VCS_TAG))
	{
		return VCS_TAG;
	}
	// this is non empty for sure!
	return VCS_MOST_RECENT_TAGGED_VERSION;
}

// Should follow the form:
// - warzone2100@TAG_NAME (for tagged builds)
// - warzone2100@FULL_COMMIT_HASH (for other builds)
std::string version_getBuildIdentifierReleaseString()
{
	std::string buildReleaseStr = "warzone2100@";
	if (strlen(vcs_tag))
	{
		// always use the tag directly
		buildReleaseStr += vcs_tag;
	}
	else
	{
		// if not a tag, use the full commit hash
		buildReleaseStr += VCS_FULL_HASH;
	}
	return buildReleaseStr;
}

// For tagged builds this will return either:
//	- "release" (for release tags)
//	- "preview" (for tags with a trailing qualifier, like "4.0.0-beta1")
// For other builds, this will return:
//	- "development"
std::string version_getBuildIdentifierReleaseEnvironment()
{
	std::string buildReleaseEnvironmentStr;
	if (strlen(vcs_tag))
	{
		optional<TagVer> tagVersion = version_extractVersionNumberFromTag(vcs_tag);
		if (tagVersion.has_value() && strlen(tagVersion.value().qualifier) != 0)
		{
			buildReleaseEnvironmentStr = "preview";
		}
		else
		{
			buildReleaseEnvironmentStr = "release";
		}
	}
	else
	{
		// if not a tag, "development" environment
		buildReleaseEnvironmentStr = "development";
	}
	return buildReleaseEnvironmentStr;
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
	std::string platformDisplay = version_getPlatformDisplayString();
	std::string platformSuffix;
#if defined(WZ_OS_IOS)
	if (!platformDisplay.empty())
	{
#if defined(WZ_IOS_DEBUG_JIT)
			platformSuffix = astringf(" [%s; Debug JIT]", platformDisplay.c_str());
#elif defined(DEBUG)
			platformSuffix = astringf(" [%s; Debug]", platformDisplay.c_str());
#elif defined(WZ_IOS_JITLESS)
			platformSuffix = astringf(" [%s; JITless]", platformDisplay.c_str());
#else
			platformSuffix = astringf(" [%s]", platformDisplay.c_str());
#endif
	}
	else
	{
#if defined(WZ_IOS_DEBUG_JIT)
			platformSuffix = " [Debug JIT]";
#elif defined(DEBUG)
			platformSuffix = " [Debug]";
#elif defined(WZ_IOS_JITLESS)
			platformSuffix = " [JITless]";
#endif
	}
#endif

	// Construct the version string
	// TRANSLATORS: This string looks as follows when expanded.
	// "Version: <version name/number>, <working copy state>,
	// Built: <BUILD DATE><BUILD TYPE>"
	snprintf(versionString, MAX_STR_LENGTH, (translated) ? _("Version: %s%s,%s Built: %s%s") : "Version: %s%s,%s Built: %s%s", version_getVersionString(), platformSuffix.c_str(), wc_state, getCompileDate(), build_type);

	return versionString;
}

// Returns a user-agent string, following typical formats & best-practices for HTTP(S) requests
std::string version_getHTTPUserAgentString()
{
	std::string userAgentString = "Warzone2100/";
	bool includeCompatible = false;
	if (strlen(vcs_tag))
	{
		userAgentString += vcs_tag;
	}
	else
	{
		// use VCS_MOST_RECENT_TAGGED_VERSION
		userAgentString += VCS_MOST_RECENT_TAGGED_VERSION;
		includeCompatible = true;
	}

	// following best-practices, include a comment with details & descriptive url
	userAgentString += " (";
	if (includeCompatible)
	{
		userAgentString += "compatible; ";
	}
	userAgentString += "+https://github.com/Warzone2100/warzone2100)";

	return userAgentString;
}
