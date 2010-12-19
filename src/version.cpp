/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2010  Warzone 2100 Project

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

#define SVN_AUTOREVISION_STATIC static
#include "src/autorevision.h"

#if (SVN_LOW_REV < SVN_REV)
# define SVN_FULL_REV_STR "r" SVN_LOW_REV_STR ":" SVN_REV_STR
#else
# define SVN_FULL_REV_STR "r" SVN_SHORT_HASH
#endif

unsigned int version_getLowRevision()
{
	return SVN_LOW_REV;
}

unsigned int version_getRevision()
{
	return SVN_REV;
}

const char* version_getVersionString()
{
	static const char* version_string = NULL;

	if (version_string == NULL)
	{
		if (strncmp(svn_uri_cstr, "tags/", strlen("tags/")) == 0)
		{
			version_string = svn_uri_cstr + strlen("tags/");
		}
		else if (strcmp(svn_uri_cstr, "trunk") == 0)
		{
			version_string = "TRUNK " SVN_FULL_REV_STR;
		}
		else if (strncmp(svn_uri_cstr, "branches/", strlen("branches/")) == 0)
		{
			version_string = (SVN_URI " branch " SVN_FULL_REV_STR) + strlen("branches/");
		}
		else if (strncmp(svn_uri_cstr, "refs/heads/", strlen("refs/heads/")) == 0)
		{
			version_string = (SVN_URI " branch " SVN_FULL_REV_STR) + strlen("refs/heads/");
		}
		else if (SVN_REV != 0)
		{
			version_string = SVN_URI " " SVN_FULL_REV_STR;
		}
		else
		{
			version_string = SVN_FULL_REV_STR;
		}
	}

	return version_string;
}

bool version_modified()
{
#if (SVN_WC_MODIFIED)
	return true;
#else
	return false;
#endif
}

bool version_switched()
{
#if (SVN_WC_SWITCHED)
	return true;
#else
	return false;
#endif
}

const char* version_getBuildDate()
{
	return __DATE__;
}

const char* version_getBuildTime()
{
	return __TIME__;
}

const char* version_getSvnDate()
{
#if (SVN_REV == 0)
	return "";
#else
	static char svn_date[sizeof(svn_date_cstr) - 9] = { '\0' };

	if (svn_date[0] == '\0')
	{
		sstrcpy(svn_date, svn_date_cstr);
	}

	return svn_date;
#endif
}

const char* version_getSvnTime()
{
#if (SVN_REV == 0)
	return "";
#else
	return SVN_DATE + sizeof(SVN_DATE) - 8 - 1;
#endif
}

const char* version_getFormattedVersionString()
{
	static char versionString[MAX_STR_LENGTH] = {'\0'};

	if (versionString[0] == '\0')
	{
		// Compose the working copy state string
#if (SVN_WC_MODIFIED && SVN_WC_SWITCHED)
		const char* wc_state = _(" (modified and switched locally)");
#elif (SVN_WC_MODIFIED)
		const char* wc_state = _(" (modified locally)");
#elif (SVN_WC_SWITCHED)
		const char* wc_state = _(" (switched locally)");
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

		if (strncmp(svn_uri_cstr, "tags/", strlen("tags/")) != 0)
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
