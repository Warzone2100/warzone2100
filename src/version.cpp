/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2012  Warzone 2100 Project

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

extern const char *BACKEND;

// Two-step process to put quotes around anything, including preprocessor definitions.
#define EXPAND(token) #token
#define QUOTE(token) EXPAND(token)

#define VCS_SHORT_HASH_QUOTED QUOTE(VCS_SHORT_HASH)
#define VCS_URI_QUOTED QUOTE(VCS_URI)
#define VCS_DATE_QUOTED QUOTE(VCS_DATE)

static const char vcs_date_cstr[] = QUOTE(VCS_DATE);
static const char vcs_uri_cstr[] = QUOTE(VCS_URI);

unsigned int version_getRevision()
{
	return VCS_NUM;
}

const char* version_getVersionString()
{
	static const char* version_string = "3.1 beta9";

	if (version_string == NULL)
	{
		if (strncmp(vcs_uri_cstr, "tags/", strlen("tags/")) == 0)
		{
			version_string = vcs_uri_cstr + strlen("tags/");
		}
		else if (strcmp(vcs_uri_cstr, "trunk") == 0)
		{
			version_string = "TRUNK " VCS_SHORT_HASH_QUOTED;
		}
		else if (strncmp(vcs_uri_cstr, "branches/", strlen("branches/")) == 0)
		{
			version_string = (VCS_URI_QUOTED " branch " VCS_SHORT_HASH_QUOTED) + strlen("branches/");
		}
		else if (strncmp(vcs_uri_cstr, "refs/heads/", strlen("refs/heads/")) == 0)
		{
			version_string = (VCS_URI_QUOTED " branch " VCS_SHORT_HASH_QUOTED) + strlen("refs/heads/");
		}
		else if (VCS_NUM != 0)
		{
			version_string = VCS_URI_QUOTED " " VCS_SHORT_HASH_QUOTED;
		}
		else
		{
			version_string = VCS_SHORT_HASH_QUOTED;
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
	return VCS_DATE_QUOTED + sizeof(VCS_DATE_QUOTED) - 8 - 1;
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

		if (strncmp(vcs_uri_cstr, "tags/", strlen("tags/")) != 0)
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
		snprintf(versionString, MAX_STR_LENGTH, _("Version %s-%s%s%s%s"), BACKEND, version_getVersionString(), wc_state, build_date, build_type);
	}

	return versionString;
}
