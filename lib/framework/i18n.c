/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2009  Warzone Resurrection Project

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

#include "string_ext.h"

#ifdef WZ_OS_MAC
# include <CoreFoundation/CoreFoundation.h>
# include <CoreFoundation/CFURL.h>
#endif

/* Always use fallbacks on Windows */
#if defined(WZ_OS_WIN)
#  undef LOCALEDIR
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR "locale"
#endif


#if defined(WZ_OS_WIN)
/*
 *  See msdn.microsoft.com for this stuff, esp.
 *  http://msdn.microsoft.com/en-us/library/ms693062%28VS.85,printer%29.aspx
 *  http://msdn.microsoft.com/en-us/library/dd318693%28VS.85,printer%29.aspx
 */
static const struct
{
	const char * language;
	const char * name;
	USHORT usPrimaryLanguage;
	USHORT usSubLanguage;
} map[] = {
	{ "", N_("System locale"), LANG_NEUTRAL, SUBLANG_DEFAULT },
#  if defined(ENABLE_NLS)
	{ "cs", N_("Czech"), LANG_CZECH, SUBLANG_DEFAULT },
	{ "da", N_("Danish"), LANG_DANISH, SUBLANG_DEFAULT },
	{ "de", N_("German"), LANG_GERMAN, SUBLANG_GERMAN },
//	{ "en", N_("English"), LANG_ENGLISH, SUBLANG_DEFAULT },
	{ "en_GB", N_("English (United Kingdom)"), LANG_ENGLISH, SUBLANG_ENGLISH_UK },
	{ "es", N_("Spanish"), LANG_SPANISH, SUBLANG_SPANISH },
	{ "et_EE", N_("Estonian"), LANG_ESTONIAN, SUBLANG_DEFAULT },
//	{ "eu", N_("Basque"), LANG_BASQUE, SUBLANG_DEFAULT },
	{ "fi", N_("Finnish"), LANG_FINNISH, SUBLANG_DEFAULT },
	{ "fr", N_("French"), LANG_FRENCH, SUBLANG_FRENCH },
	/* Our Frisian translation is the "West Frisian" variation of it. This
	 * variation is mostly spoken in the Dutch province Friesland (Fryslân
	 * in Frisian) and has ISO 639-3 code "fry".
	 *
	 * FIXME: We should really use a sub-language code for this. E.g.
	 *        fy_XX.
	 */
	{ "fy", N_("Frisian"), LANG_FRISIAN, SUBLANG_FRISIAN_NETHERLANDS },
	{ "ga", N_("Irish"), LANG_IRISH, SUBLANG_IRISH_IRELAND },
	{ "hr", N_("Croatian"), LANG_CROATIAN, SUBLANG_DEFAULT },
	{ "it", N_("Italian"), LANG_ITALIAN, SUBLANG_ITALIAN },
//	{ "la", N_("Latin"), LANG_LATIN, SUBLANG_DEFAULT },
	{ "lt", N_("Lithuanian"), LANG_LITHUANIAN, SUBLANG_DEFAULT },
//	{ "lv", N_("Latvian"), LANG_LATVIAN, SUBLANG_DEFAULT },
	// MSDN uses "no"...
	{ "nb", N_("Norwegian"), LANG_NORWEGIAN, SUBLANG_DEFAULT },
//	{ "nn", N_("Norwegian (Nynorsk)"), LANG_NORWEGIAN, SUBLANG_NORWEGIAN_NYNORSK },
	{ "nl", N_("Dutch"), LANG_DUTCH, SUBLANG_DUTCH },
	{ "pl", N_("Polish"), LANG_POLISH, SUBLANG_DEFAULT },
	{ "pt_BR", N_("Brazilian Portuguese"), LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN },
	{ "pt", N_("Portuguese"), LANG_PORTUGUESE, SUBLANG_DEFAULT },
	{ "ro", N_("Romanian"), LANG_ROMANIAN, SUBLANG_DEFAULT },
	{ "ru", N_("Russian"), LANG_RUSSIAN, SUBLANG_DEFAULT },
	{ "sl", N_("Slovenian"), LANG_SLOVENIAN, SUBLANG_DEFAULT },
#if (WINVER >= 0x0600)
//	{ "sv_SE", N_("Swedish (Sweden)"), LANG_SWEDISH, SUBLANG_SWEDISH_SWEDEN },
#else
//	{ "sv_SE", N_("Swedish (Sweden)"), LANG_SWEDISH, SUBLANG_SWEDISH },
#endif
//	{ "sv", N_("Swedish"), LANG_SWEDISH, SUBLANG_DEFAULT },
//	{ "tr", N_("Turkish"), LANG_TURKISH, SUBLANG_DEFAULT },
//	{ "uz", N_("Uzbek (Cyrillic)"), LANG_UZBEK, SUBLANG_UZBEK_CYRILLIC },
	{ "uk_UA", N_("Ukrainian"), LANG_UKRAINIAN, SUBLANG_DEFAULT },
	{ "zh_CN", N_("Simplified Chinese"), LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED },
	{ "zh_TW", N_("Traditional Chinese"), LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL },
#  endif
};
#else
static const struct
{
	const char *language;
	const char *name;
	const char *locale;
	const char *localeFallback;
} map[] = {
	{ "",   N_("System locale"), "", "" },
#  if defined(ENABLE_NLS)
	{ "cs", N_("Czech"), "cs.UTF-8", "cs" },
	{ "da", N_("Danish"), "da_DK.UTF-8", "da_DK" },
	{ "de", N_("German"), "de_DE.UTF-8", "de_DE" },
//	{ "en", N_("English"), "en_US.UTF-8", "en_US" },
	{ "en_GB", N_("English (United Kingdom)"), "en_GB.UTF-8", "en_GB" },
	{ "es", N_("Spanish"), "es_ES.UTF-8", "es_ES" },
	{ "et_EE", N_("Estonian"), "et_EE.UTF-8", "et_EE" },
//	{ "eu", N_("Basque"), "eu.UTF-8", "eu" },
	{ "fi", N_("Finnish"), "fi.UTF-8", "fi" },
	{ "fr", N_("French"), "fr_FR.UTF-8", "fr_FR" },
	/* Our Frisian translation is the "West Frisian" variation of it. This
	 * variation is mostly spoken in the Dutch province Friesland (Fryslân
	 * in Frisian) and has ISO 639-3 code "fry".
	 *
	 * FIXME: We should really use a sub-language code for this. E.g.
	 *        fy_XX.
	 */
	{ "fy", N_("Frisian"), "fy.UTF-8", "fy" },
	{ "ga", N_("Irish"), "ga.UTF-8", "ga" },
	{ "hr", N_("Croatian"), "hr_HR.UTF-8", "hr_HR" },
	{ "it", N_("Italian"), "it_IT.UTF-8", "it_IT" },
	{ "la", N_("Latin"), "la.UTF-8", "la" },
	{ "lt", N_("Lithuanian"), "lt.UTF-8", "lt" },
//	{ "lv", N_("Latvian"), "lv.UTF-8", "lv" },
	{ "nb", N_("Norwegian"), "nb_NO.UTF-8", "nb_NO" },
//	{ "nn", N_("Norwegian (Nynorsk)"), "nn.UTF-8", "nn" },
	{ "nl", N_("Dutch"), "nl_NL.UTF-8", "nl_NL" },
	{ "pl", N_("Polish"), "pl.UTF-8", "pl" },
	{ "pt_BR", N_("Brazilian Portuguese"), "pt_BR.UTF-8", "pt_BR" },
	{ "pt", N_("Portuguese"), "pt_PT.UTF-8", "pt_PT" },
	{ "ro", N_("Romanian"), "ro.UTF-8", "ro" },
	{ "ru", N_("Russian"), "ru_RU.UTF-8", "ru_RU" },
	{ "sl", N_("Slovenian"), "sl.UTF-8", "sl" },
//	{ "sv_SE", N_("Swedish (Sweden)"), "sv_SE.UTF-8", "sv_SE" },
//	{ "sv", N_("Swedish"), "sv.UTF-8", "sv" },
//	{ "tr", N_("Turkish"), "tr.UTF-8", "tr" },
//	{ "uz", N_("Uzbek (Cyrillic)"), "uz.UTF-8", "uz" },
	{ "uk_UA", N_("Ukrainian"), "uk_UA.UTF-8", "uk_UA" },
	{ "zh_CN", N_("Simplified Chinese"), "zh_CN.UTF-8", "zh_CN" },
	{ "zh_TW", N_("Traditional Chinese"), "zh_TW.UTF-8", "zh_TW" },
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
	static char language[6] = { '\0' }; // large enough for xx_YY
	const char *localeName = setlocale(LC_MESSAGES, NULL);
	char *delim = NULL;

	if (selectedLanguage == 0 || localeName == NULL)
	{
		return "";  // Return empty string for system default and errors
	}

	sstrcpy(language, localeName);

	// cut anything after a '.' to get rid of the encoding part
	delim = strchr(language, '.');
	if (delim)
	{
		*delim = '\0';
	}

	// if language is xx_XX, cut the _XX part
	delim = strchr(language, '_');
	if (delim)
	{
		if (!strncasecmp(language, delim + 1, 2))
		{
			*delim = '\0';
		}
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

	return language;
}


#if defined(ENABLE_NLS)
#  if defined(WZ_OS_WIN)
static bool setLocaleWindows(USHORT usPrimaryLanguage, USHORT usSubLanguage)
{
	bool success = SUCCEEDED( SetThreadLocale( MAKELCID( MAKELANGID(usPrimaryLanguage, usSubLanguage), SORT_DEFAULT ) ) );

	if (!success)
	{
		info("Failed to set locale to \"%d\"", usPrimaryLanguage);
	}
	else
	{
		debug(LOG_WZ, "Requested locale \"%d\"", usPrimaryLanguage);
	}

	setlocale(LC_NUMERIC, "C"); // set radix character to the period (".")

	return success;
}
#  else
/*!
 * Set the prefered locale
 * \param locale The locale, NOT just the language part
 * \note Use this instead of setlocale(), because we need the default radix character
 */
static bool setLocaleUnix(const char* locale)
{
	const char *actualLocale = setlocale(LC_ALL, locale);

	if (actualLocale == NULL)
	{
		info("Failed to set locale to \"%s\"", locale);
	}
	else
	{
		if (strcmp(locale, actualLocale))
		{
			debug(LOG_WZ, "Requested locale \"%s\", got \"%s\" instead", locale, actualLocale);
		}
	}

	setlocale(LC_NUMERIC, "C"); // set radix character to the period (".")

	return (actualLocale != NULL);
}
#  endif
#endif


bool setLanguage(const char *language)
{
#if !defined(ENABLE_NLS)
	return true;
#else
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(map); i++)
	{
		if (strcmp(language, map[i].language) == 0)
		{
			selectedLanguage = i;
			debug(LOG_WZ, "Setting language to \"%s\" (%s)", map[i].name, map[i].language);

#  if defined(WZ_OS_WIN)
			return setLocaleWindows(map[i].usPrimaryLanguage, map[i].usSubLanguage);
#  else
			return setLocaleUnix(map[i].locale) || setLocaleUnix(map[i].localeFallback);
#  endif
		}
	}

	debug(LOG_ERROR, "Requested language \"%s\" not supported.", language);

	return false;
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
		sstrcpy(localeDir, PHYSFS_getBaseDir());
		sstrcat(localeDir, "\\" LOCALEDIR);

		// Set locale directory and translation domain name
		textdomainDirectory = bindtextdomain(PACKAGE, localeDir);
	}
#else
	#ifdef WZ_OS_MAC
	{
		char resourcePath[PATH_MAX];
		CFURLRef resourceURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
		if( CFURLGetFileSystemRepresentation( resourceURL, true, (UInt8 *) resourcePath, PATH_MAX) )
		{
			sstrcat(resourcePath, "/locale");
			textdomainDirectory = bindtextdomain(PACKAGE, resourcePath);
		}
		else
		{
			debug( LOG_ERROR, "Could not change to resources directory." );
		}

		debug(LOG_INFO, "resourcePath is %s", resourcePath);
	}
	#else
	textdomainDirectory = bindtextdomain(PACKAGE, LOCALEDIR);
	#endif
#endif
	if (!textdomainDirectory)
	{
		debug(LOG_ERROR, "initI18n: bindtextdomain failed!");
	}

	(void)bind_textdomain_codeset(PACKAGE, "UTF-8");
	(void)textdomain(PACKAGE);
}
