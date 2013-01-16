/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2013  Warzone 2100 Project

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

#include "src/autorevision.h"

static const char vcs_date_cstr[] = VCS_DATE;
static const char vcs_branch_cstr[] = VCS_BRANCH;

unsigned int version_getRevision()
{
	return VCS_NUM;
}

const char* version_getVersionString()
{
	static const char* version_string = "3.1.0";

	if (version_string == NULL)
	{
		if (strncmp(vcs_branch_cstr, "tags/", strlen("tags/")) == 0)
		{
			version_string = vcs_branch_cstr + strlen("tags/");
		}
		else if (strcmp(vcs_branch_cstr, "trunk") == 0)
		{
			version_string = "TRUNK " VCS_SHORT_HASH;
		}
		else if (strncmp(vcs_branch_cstr, "branches/", strlen("branches/")) == 0)
		{
			version_string = (VCS_BRANCH " branch " VCS_SHORT_HASH) + strlen("branches/");
		}
		else if (strncmp(vcs_branch_cstr, "refs/heads/", strlen("refs/heads/")) == 0)
		{
			version_string = (VCS_BRANCH " branch " VCS_SHORT_HASH) + strlen("refs/heads/");
		}
		else if (VCS_NUM != 0)
		{
			version_string = VCS_BRANCH " " VCS_SHORT_HASH;
		}
		else
		{
			version_string = VCS_SHORT_HASH;
		}
	}

	return version_string;
}

bool version_modified()
{
	return VCS_WC_MODIFIED;
}

const char* version_getBuildDate()
{
	return __DATE__;
}

const char* version_getBuildTime()
{
	return __TIME__;
}

const char* version_getVcsDate()
{
#if (VCS_NUM == 0)
	return "";
#else
	static char vcs_date[sizeof(vcs_date_cstr) - 9] = { '\0' };

	if (vcs_date[0] == '\0')
	{
		sstrcpy(vcs_date, vcs_date_cstr);
	}

	return vcs_date;
#endif
}

const char* version_getVcsTime()
{
#if (VCS_NUM == 0)
	return "";
#else
	return VCS_DATE + sizeof(VCS_DATE) - 8 - 1;
#endif
}

const char* version_getFormattedVersionString()
{
	static char versionString[MAX_STR_LENGTH] = {'\0'};

	if (versionString[0] == '\0')
	{
		// Compose the working copy state string
#if (VCS_WC_MODIFIED)
		const char* wc_state = _(" (modified locally)");
#else
		const char* wc_state = "";
#endif

		// Compose the build type string
#ifdef DEBUG
		const char* build_type = _(" - DEBUG");
#else
		const char* build_type = "";
#endif

		const char* build_date = NULL;

		if (strncmp(vcs_branch_cstr, "tags/", strlen("tags/")) != 0)
		{
			sasprintf((char**)&build_date, _(" - Built %s"), version_getBuildDate());
		}
		else
		{
			build_date = "";
		}

		// Construct the version string
		// TRANSLATORS: This string looks as follows when expanded.
		// "Version <version name/number> <working copy state><BUILD DATE><BUILD TYPE>"
		snprintf(versionString, MAX_STR_LENGTH, _("Version %s%s%s%s"), version_getVersionString(), wc_state, build_date, build_type);
	}

	return versionString;
}
