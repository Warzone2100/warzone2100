/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2017  Warzone 2100 Project

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

#include "src/autorevision.h"  // Apparently must add the "src/" so make doesn't needlessly recompile version.cpp every time.

static const char vcs_branch_cstr[] = VCS_BRANCH;
static const char vcs_tag[] = VCS_TAG;

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
		versionedWriteDirFolderName += vcs_tag;
	}
	else if (strlen(vcs_branch_cstr))
	{
#if defined(DEBUG) || defined(WZ_USE_MASTER_BRANCH_APP_DIR)
		// To ease testing new branches with existing files, DEBUG builds
		// (or when WZ_USE_MASTER_BRANCH_APP_DIR is defined)
		// default to using "master" as the branch
		versionedWriteDirFolderName += "master";
#else
		// For Release builds, use the actual branch name
		versionedWriteDirFolderName += vcs_branch_cstr;
#endif
	}
	else
	{
		// not a branch or a tag, so we are detached most likely.
		std::string vcs_extra_str = VCS_EXTRA;
		// remove any spaces from VCS_EXTRA
		std::remove(vcs_extra_str.begin(), vcs_extra_str.end(), ' ');
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

/** Composes a nicely formatted version string.
*
*/
const char *version_getFormattedVersionString()
{
	static char versionString[MAX_STR_LENGTH] = {'\0'};

	if (versionString[0] == '\0')
	{
		// Compose the working copy state string
#if (VCS_WC_MODIFIED)
		const char *wc_state = _(" (modified locally)");
#else
		const char *wc_state = "";
#endif
		// Compose the build type string
#ifdef DEBUG
		const char *build_type = _(" - DEBUG");
#else
		const char *build_type = "";
#endif

		// Construct the version string
		// TRANSLATORS: This string looks as follows when expanded.
		// "Version <version name/number> <working copy state><BUILD DATE><BUILD TYPE>"
		snprintf(versionString, MAX_STR_LENGTH, _("Version: %s,%s Built:%s%s"), version_getVersionString(), wc_state, __DATE__, build_type);
	}

	return versionString;
}
