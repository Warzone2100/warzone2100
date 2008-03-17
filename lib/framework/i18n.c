/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2008  Warzone Resurrection Project

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
#include "frame.h"

#include <locale.h>


/* Always use fallbacks on Windows */
#if defined(WZ_OS_WIN)
#  undef LOCALEDIR
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR "locale"
#endif


/*!
 * Return the language part of the selected locale
 */
const char* getLanguage(void)
{
	static char language[4] = { '\0' }; // ISO639 language code has to fit in!
	static BOOL haveLanguage = FALSE;

#ifdef ENABLE_NLS
	if ( ! haveLanguage )  // only get language name once for speed optimization
	{
		char *localeName = setlocale(LC_MESSAGES, NULL);
		char *delim = NULL;

		haveLanguage = TRUE;

		if ( !localeName )
		{
			return language; // Return empty string on errors
		}

		strlcpy(language, localeName, sizeof(language));

		delim = strchr(language, '_');

		if ( !delim )
		{
			delim = strchr(language, '.');
		}

		if ( delim )
		{
			*delim = '\0';
		}
	}
#endif // ENABLE_NLS

	return language;
}


/*!
 * Set the prefered language
 * \param locale The locale, NOT just the language part
 */
void setLanguage(const char* locale)
{
	setlocale(LC_ALL, locale);
	setlocale(LC_NUMERIC, "C"); // set radix character to the period (".")
}


void initI18n(void)
{
	setLanguage(WZ_SYSTEM_LOCALE); // Default to system locale till config is parsed
#if defined(WZ_OS_WIN)
	{
		// Retrieve an absolute path to the locale directory
		char localeDir[PATH_MAX];
		strlcpy(localeDir, PHYSFS_getBaseDir(), sizeof(localeDir));
		strlcat(localeDir, "\\" LOCALEDIR, sizeof(localeDir));

		// Set locale directory and translation domain name
		(void)bindtextdomain(PACKAGE, localeDir);
	}
#else
	(void)bindtextdomain(PACKAGE, LOCALEDIR);
#endif
	(void)bind_textdomain_codeset(PACKAGE, "UTF-8"); // FIXME should be UCS4 to avoid conversions
	(void)textdomain(PACKAGE);
}
