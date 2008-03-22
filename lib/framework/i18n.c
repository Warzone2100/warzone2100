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
#include <physfs.h>


/* Always use fallbacks on Windows */
#if defined(WZ_OS_WIN)
#  undef LOCALEDIR
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR "locale"
#endif

struct languageMap
{
	char *typeStr;
	char *fullStr;
	char *realStr;
	char *fallback;
};

const static struct languageMap map[] =
{
	{ "",   N_("System locale"), "", "" },
	{ "en", N_("English"), "en_US.utf8", "en_US" },
	{ "nb", N_("Norwegian"), "nb_NO.utf8", "nb_NO" },
	{ "de", N_("German"), "de_DE.utf8", "de_DE" },
	{ "da", N_("Danish"), "da_DK.utf8", "da_DK" },
	{ "fr", N_("French"), "fr_FR.utf8", "fr_FR" },
	{ "it", N_("Italian"), "it_IT.utf8", "it_IT"  },
	{ "la", N_("Latin"), "la", "la" },
	{ "nl", N_("Dutch"), "nl_NL.utf8", "nl_NL" },
	{ "pt", N_("Portugese"), "pt_PT.utf8", "pt_PT" },
	{ "ru", N_("Russian"), "ru_RU.utf8", "ru_RU" },
};

static int selectedLanguage = 0;

#ifdef WZ_OS_WIN
// This function ripped from Freeciv, see utility/shared.c for
// more definitions.
const char *getWindowsLanguage(void)
{
	char *langname = getenv("LANG");

	/* set LANG by hand if it is not set */
	if (!langname)
	{
		switch (PRIMARYLANGID(GetUserDefaultLangID()))
		{
		case LANG_DANISH:
			return "da";
		case LANG_GERMAN:
			return "de";
		case LANG_ENGLISH:
			return "en";
		case LANG_FRENCH:
			return "fr";
		case LANG_ITALIAN:
			return "it";
		case LANG_DUTCH:
			return "nl";
		case LANG_NORWEGIAN:
			return "nb";
		case LANG_PORTUGUESE:
			return "pt";
		case LANG_RUSSIAN:
			return "ru";
		case LANG_SWEDISH:
			return "sv";
		}
	}
	return langname;
}
#endif

/*!
 * Return the language part of the selected locale
 */
const char* getLanguage(void)
{
	static char language[4] = { '\0' }; // ISO639 language code has to fit in!

#ifdef ENABLE_NLS
	char *localeName = setlocale(LC_MESSAGES, NULL);
	char *delim = NULL;

	if ( !localeName )
	{
#ifdef WZ_OS_WIN
		return getWindowsLanguage();
#else
		return language; // Return empty string on errors
#endif
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
#endif // ENABLE_NLS

	return language;
}


/*!
 * Set the prefered locale
 * \param locale The locale, NOT just the language part
 */
static const char *setLocale(const char* locale)
{
	const char *retval = setlocale(LC_ALL, locale);

	debug(LOG_WZ, "Wanted to set language to %s. Actually set language to %s", 
	      locale, retval ? retval : "");
	setlocale(LC_NUMERIC, "C"); // set radix character to the period (".")
	return retval;
}

void setNextLanguage(void)
{
	selectedLanguage++;
	if (selectedLanguage > sizeof(map) / sizeof(map[0]) - 1)
	{
		selectedLanguage = 0;
	}
	if (!setLocale(map[selectedLanguage].realStr))
	{
		debug(LOG_WARNING, "setNextLanguage: Failed to set %s as %s, trying fallback", 
		      map[selectedLanguage].fullStr, map[selectedLanguage].realStr);
		if (!setLocale(map[selectedLanguage].fallback))
		{
			debug(LOG_ERROR, "setNextLanguage: Failed to set %s as either %s or %s.",
			      map[selectedLanguage].fullStr, map[selectedLanguage].realStr, 
			      map[selectedLanguage].fallback);
			if (selectedLanguage != 0)
			{
				setNextLanguage();	// try next
			}
		}
	}
}

BOOL setLanguageByName(const char *name)
{
	int i;

	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++)
	{
		if (strcmp(name, map[i].fullStr) == 0)
		{
			debug(LOG_WZ, "Setting language to %s (%s)", map[i].fullStr, map[i].realStr);
			return (setLocale(map[i].realStr) != NULL);
		}
	}
	return FALSE;
}

const char* getLanguageName(void)
{
	int i;
	const char *typeStr = getLanguage();

	if (selectedLanguage == 0)
	{
		return gettext(map[0].fullStr); // hardcoded to override current setting, and use system setting
	}
	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++)
	{
		if (strcmp(typeStr, map[i].typeStr) == 0)
		{
			return gettext(map[i].fullStr);
		}
	}

	ASSERT(FALSE, "getLanguageName: Unknown language");
	return NULL;
}

void initI18n(void)
{
	const char *retval = NULL;

	retval = setLocale("");	// set to system default
	if (!retval)
	{
		// no system default?
		debug(LOG_ERROR, "No system language found");
	}
#if defined(WZ_OS_WIN)
	{
		// Retrieve an absolute path to the locale directory
		char localeDir[PATH_MAX];
		strlcpy(localeDir, PHYSFS_getBaseDir(), sizeof(localeDir));
		strlcat(localeDir, "\\" LOCALEDIR, sizeof(localeDir));

		// Set locale directory and translation domain name
		retval = bindtextdomain(PACKAGE, localeDir);
	}
#else
	retval = bindtextdomain(PACKAGE, LOCALEDIR);
#endif
	if (!retval)
	{
		debug(LOG_ERROR, "initI18n: bindtextdomain failed!");
	}

	(void)bind_textdomain_codeset(PACKAGE, "UTF-8");
	(void)textdomain(PACKAGE);
}
