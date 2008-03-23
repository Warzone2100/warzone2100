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

#include <physfs.h>


/* Always use fallbacks on Windows */
#if defined(WZ_OS_WIN)
#  undef LOCALEDIR
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR "locale"
#endif


#if defined(WZ_OS_WIN)
const static struct
{
	const char * language;
	const char * name;
	USHORT usPrimaryLanguage;
} map[] = {
	{ "", N_("System locale"), LANG_NEUTRAL },
#  if defined(ENABLE_NLS)
	{ "da", N_("Danish"), LANG_DANISH },
	{ "de", N_("German"), LANG_GERMAN },
	{ "en", N_("English"), LANG_ENGLISH },
	{ "fr", N_("French"), LANG_FRENCH },
	{ "it", N_("Italian"), LANG_ITALIAN },
	{ "nl", N_("Dutch"), LANG_DUTCH },
	{ "nb", N_("Norwegian"), LANG_NORWEGIAN },
	{ "pt", N_("Portuegese"), LANG_PORTUGUESE },
	{ "ru", N_("Russian"), LANG_RUSSIAN },
	{ "sv", N_("Swedish"), LANG_SWEDISH },
#  endif
};
#else
const static struct
{
	const char *language;
	const char *name;
	const char *locale;
	const char *localeFallback;
} map[] = {
	{ "",   N_("System locale"), "", "" },
#  if defined(ENABLE_NLS)
	{ "en", N_("English"), "en_US.UTF-8", "en_US" },
	{ "nb", N_("Norwegian"), "nb_NO.UTF-8", "nb_NO" },
	{ "de", N_("German"), "de_DE.UTF-8", "de_DE" },
	{ "da", N_("Danish"), "da_DK.UTF-8", "da_DK" },
	{ "fr", N_("French"), "fr_FR.UTF-8", "fr_FR" },
	{ "it", N_("Italian"), "it_IT.UTF-8", "it_IT" },
	{ "la", N_("Latin"), "la.UTF-8", "la" },
	{ "nl", N_("Dutch"), "nl_NL.UTF-8", "nl_NL" },
	{ "pt", N_("Portuegese"), "pt_PT.UTF-8", "pt_PT" },
	{ "ru", N_("Russian"), "ru_RU.UTF-8", "ru_RU" },
#  endif
};
#endif

static unsigned int selectedLanguage = 0;


/*!
 * Return the language part of the selected locale
 */
#if !defined(ENABLE_NLS)
const char* getLanguage(void)
{
	return "";
}
#elif defined(WZ_OS_WIN)
const char *getLanguage(void)
{
	USHORT usPrimaryLanguage = PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale()));
	unsigned int i;

	if (selectedLanguage == 0)
	{
		return "";  // Return empty string for system default
	}

	for (i = 0; i < ARRAY_SIZE(map); i++)
	{
		if (usPrimaryLanguage == map[i].usPrimaryLanguage)
		{
			return map[i].language;
		}
	}

	return "";
}
#else
const char *getLanguage(void)
{
	static char language[4] = { '\0' }; // ISO639 language code has to fit in!
	const char *localeName = setlocale(LC_MESSAGES, NULL);
	char *delim = NULL;

	if (selectedLanguage == 0 || localeName == NULL)
	{
		return "";  // Return empty string for system default and errors
	}

	strlcpy(language, localeName, sizeof(language));

	delim = strchr(language, '_');

	if ( !delim )
	{
		delim = strchr(language, '.');
	}

	if ( delim ) // Cut after '_' or '.'
	{
		*delim = '\0';
	}

	return language;
}
#endif


const char* getLanguageName(void)
{
	const char *language = getLanguage();
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(map); i++)
	{
		if (strcmp(language, map[i].language) == 0)
		{
			return gettext(map[i].name);
		}
	}

	ASSERT(FALSE, "getLanguageName: Unknown language");
	return NULL;
}


#if defined(ENABLE_NLS)
#  if defined(WZ_OS_WIN)
static BOOL setLocaleWindows(USHORT usPrimaryLanguage)
{
	BOOL success = SUCCEEDED( SetThreadLocale( MAKELCID( MAKELANGID(usPrimaryLanguage, SUBLANG_DEFAULT), SORT_DEFAULT ) ) );

	if (!success)
		debug(LOG_ERROR, "Failed to set locale to \"%d\"", usPrimaryLanguage);
	else
		debug(LOG_WZ, "Requested locale \"%d\"", usPrimaryLanguage);

	setlocale(LC_NUMERIC, "C"); // set radix character to the period (".")

	return success;
}
#  else
/*!
 * Set the prefered locale
 * \param locale The locale, NOT just the language part
 * \note Use this instead of setlocale(), because we need the default radix character
 */
static BOOL setLocaleUnix(const char* locale)
{
	const char *actualLocale = setlocale(LC_ALL, locale);

	if (actualLocale == NULL)
		debug(LOG_ERROR, "Failed to set locale to \"%s\"", locale);
	else
		debug(LOG_WZ, "Requested locale \"%s\", got \"%s\" instead", locale, actualLocale);

	setlocale(LC_NUMERIC, "C"); // set radix character to the period (".")

	return (actualLocale != NULL);
}
#  endif
#endif


BOOL setLanguage(const char *language)
{
#if !defined(ENABLE_NLS)
	return TRUE;
#else
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(map); i++)
	{
		if (strcmp(language, map[i].language) == 0)
		{
			selectedLanguage = i;
			debug(LOG_WZ, "Setting language to \"%s\" (%s)", map[i].name, map[i].language);

#  if defined(WZ_OS_WIN)
			return setLocaleWindows(map[i].usPrimaryLanguage);
#  else
			return setLocaleUnix(map[i].locale) || setLocaleUnix(map[i].localeFallback);
#  endif
		}
	}

	debug(LOG_ERROR, "Requested language \"%s\" not supported.", language);

	return FALSE;
#endif
}


void setNextLanguage(void)
{
	selectedLanguage++;
	if (selectedLanguage > ARRAY_SIZE(map) - 1)
	{
		selectedLanguage = 0;
	}

	if (!setLanguage(map[selectedLanguage].language) && selectedLanguage != 0)
	{
		setNextLanguage(); // try next
	}
}


void initI18n(void)
{
	const char *textdomainDirectory = NULL;

	if (!setLanguage("")) // set to system default
	{
		// no system default?
		debug(LOG_ERROR, "initI18n: No system language found");
	}
#if defined(WZ_OS_WIN)
	{
		// Retrieve an absolute path to the locale directory
		char localeDir[PATH_MAX];
		strlcpy(localeDir, PHYSFS_getBaseDir(), sizeof(localeDir));
		strlcat(localeDir, "\\" LOCALEDIR, sizeof(localeDir));

		// Set locale directory and translation domain name
		textdomainDirectory = bindtextdomain(PACKAGE, localeDir);
	}
#else
	textdomainDirectory = bindtextdomain(PACKAGE, LOCALEDIR);
#endif
	if (!textdomainDirectory)
	{
		debug(LOG_ERROR, "initI18n: bindtextdomain failed!");
	}

	(void)bind_textdomain_codeset(PACKAGE, "UTF-8");
	(void)textdomain(PACKAGE);
}
