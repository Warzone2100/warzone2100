/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "wzpaths.h"

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

#ifdef PORTABLE
#  ifdef PACKAGE
#   undef PACKAGE
#   define PACKAGE "warzone2100_portable"
#  endif
#endif
// Language names (http://en.wikipedia.org/wiki/List_of_ISO_639-2_codes)
#define LANG_NAME_BASQUE "euskara"
#define LANG_NAME_CATALAN "català"
#define LANG_NAME_CHINESE_SIMPLIFIED "汉语"
#define LANG_NAME_CHINESE_TRADITIONAL "漢語"
#define LANG_NAME_CROATIAN "Hrvatski"
#define LANG_NAME_CZECH "česky"
#define LANG_NAME_DANISH "Dansk"
#define LANG_NAME_DUTCH "Nederlands"
#define LANG_NAME_ENGLISH "English"
#define LANG_NAME_ENGLISH_UK "British English"
#define LANG_NAME_ESTONIAN "Eesti Keel"
#define LANG_NAME_FINNISH "suomi"
#define LANG_NAME_FRENCH "Français"
#define LANG_NAME_FRISIAN_NETHERLANDS "frysk"
#define LANG_NAME_GERMAN "Deutsch"
#define LANG_NAME_GREEK "Ελληνικά"
#define LANG_NAME_HUNGARIAN "magyar"
#define LANG_NAME_IRISH "Imruagadh"
#define LANG_NAME_ITALIAN "Italiano"
#define LANG_NAME_KOREAN "한국어"
#define LANG_NAME_LATIN "latine"
#define LANG_NAME_LATVIAN "latviešu valoda"
#define LANG_NAME_LITHUANIAN "lietuvių kalba"
#define LANG_NAME_NORWEGIAN "Norsk"
#define LANG_NAME_NORWEGIAN_NYNORSK "nynorsk"
#define LANG_NAME_POLISH "Polski"
#define LANG_NAME_PORTUGUESE_BRAZILIAN "Português Brasileiro"
#define LANG_NAME_PORTUGUESE "Português"
#define LANG_NAME_ROMANIAN "română"
#define LANG_NAME_RUSSIAN "Русский"
#define LANG_NAME_SLOVAK "Slovensky"
#define LANG_NAME_SLOVENIAN "Slovenski"
#define LANG_NAME_SPANISH "Español"
#define LANG_NAME_SWEDISH "svenska"
#define LANG_NAME_SWEDISH_SWEDEN "svenska (Sverige)"
#define LANG_NAME_TURKISH "Türkçe"
#define LANG_NAME_UKRAINIAN "Українська"
#define LANG_NAME_UZBEK_CYRILLIC "Ўзбек"

#if defined(WZ_OS_WIN)
/*
 *  See msdn.microsoft.com for this stuff, esp.
 *  http://msdn.microsoft.com/en-us/library/ms693062%28VS.85,printer%29.aspx
 *  http://msdn.microsoft.com/en-us/library/dd318693%28VS.85,printer%29.aspx
 */
static const struct
{
	const char *language;
	const char *name;
	USHORT usPrimaryLanguage;
	USHORT usSubLanguage;
} map[] =
{
	{ "", N_("System locale"), LANG_NEUTRAL, SUBLANG_DEFAULT },
#  if defined(ENABLE_NLS)
	{ "ca", LANG_NAME_CATALAN, LANG_CATALAN, SUBLANG_DEFAULT },
	{ "cs", LANG_NAME_CZECH, LANG_CZECH, SUBLANG_DEFAULT },
	{ "da", LANG_NAME_DANISH, LANG_DANISH, SUBLANG_DEFAULT },
	{ "de", LANG_NAME_GERMAN, LANG_GERMAN, SUBLANG_GERMAN },
	{ "el", LANG_NAME_GREEK, LANG_GREEK, SUBLANG_DEFAULT },
	{ "en", LANG_NAME_ENGLISH, LANG_ENGLISH, SUBLANG_DEFAULT },
	{ "en_GB", LANG_NAME_ENGLISH_UK, LANG_ENGLISH, SUBLANG_ENGLISH_UK },
	{ "es", LANG_NAME_SPANISH, LANG_SPANISH, SUBLANG_SPANISH },
	{ "et_EE", LANG_NAME_ESTONIAN, LANG_ESTONIAN, SUBLANG_DEFAULT },
//	{ "eu", LANG_NAME_BASQUE, LANG_BASQUE, SUBLANG_DEFAULT },
	{ "fi", LANG_NAME_FINNISH, LANG_FINNISH, SUBLANG_DEFAULT },
	{ "fr", LANG_NAME_FRENCH, LANG_FRENCH, SUBLANG_FRENCH },
	/* Our Frisian translation is the "West Frisian" variation of it. This
	 * variation is mostly spoken in the Dutch province Friesland (Fryslân
	 * in Frisian) and has ISO 639-3 code "fry".
	 *
	 * FIXME: We should really use a sub-language code for this. E.g.
	 *        fy_XX.
	 */
	{ "fy", LANG_NAME_FRISIAN_NETHERLANDS, LANG_FRISIAN, SUBLANG_FRISIAN_NETHERLANDS },
	{ "ga", LANG_NAME_IRISH, LANG_IRISH, SUBLANG_IRISH_IRELAND },
	{ "hr", LANG_NAME_CROATIAN, LANG_CROATIAN, SUBLANG_DEFAULT },
	{ "hu", LANG_NAME_HUNGARIAN, LANG_HUNGARIAN, SUBLANG_DEFAULT },
	{ "it", LANG_NAME_ITALIAN, LANG_ITALIAN, SUBLANG_ITALIAN },
	{ "ko_KR", LANG_NAME_KOREAN, LANG_KOREAN, SUBLANG_DEFAULT },
//	{ "la", LANG_NAME_LATIN, LANG_LATIN, SUBLANG_DEFAULT },
	{ "lt", LANG_NAME_LITHUANIAN, LANG_LITHUANIAN, SUBLANG_DEFAULT },
//	{ "lv", LANG_NAME_LATVIAN, LANG_LATVIAN, SUBLANG_DEFAULT },
	// MSDN uses "no"...
	{ "nb", LANG_NAME_NORWEGIAN, LANG_NORWEGIAN, SUBLANG_DEFAULT },
//	{ "nn", LANG_NAME_NORWEGIAN_NYNORSK, LANG_NORWEGIAN, SUBLANG_NORWEGIAN_NYNORSK },
	{ "nl", LANG_NAME_DUTCH, LANG_DUTCH, SUBLANG_DUTCH },
	{ "pl", LANG_NAME_POLISH, LANG_POLISH, SUBLANG_DEFAULT },
	{ "pt_BR", LANG_NAME_PORTUGUESE_BRAZILIAN, LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN },
	{ "pt", LANG_NAME_PORTUGUESE, LANG_PORTUGUESE, SUBLANG_DEFAULT },
	{ "ro", LANG_NAME_ROMANIAN, LANG_ROMANIAN, SUBLANG_DEFAULT },
	{ "ru", LANG_NAME_RUSSIAN, LANG_RUSSIAN, SUBLANG_DEFAULT },
	{ "sk", LANG_NAME_SLOVAK, LANG_SLOVAK, SUBLANG_DEFAULT },
	{ "sl", LANG_NAME_SLOVENIAN, LANG_SLOVENIAN, SUBLANG_DEFAULT },
#if (WINVER >= 0x0600)
//	{ "sv_SE", LANG_NAME_SWEDISH_SWEDEN, LANG_SWEDISH, SUBLANG_SWEDISH_SWEDEN },
#else
//	{ "sv_SE", LANG_NAME_SWEDISH_SWEDEN, LANG_SWEDISH, SUBLANG_SWEDISH },
#endif
//	{ "sv", LANG_NAME_SWEDISH, LANG_SWEDISH, SUBLANG_DEFAULT },
	{ "tr", LANG_NAME_TURKISH, LANG_TURKISH, SUBLANG_DEFAULT },
//	{ "uz", LANG_NAME_UZBEK_CYRILLIC, LANG_UZBEK, SUBLANG_UZBEK_CYRILLIC },
	{ "uk_UA", LANG_NAME_UKRAINIAN, LANG_UKRAINIAN, SUBLANG_DEFAULT },
	{ "zh_CN", LANG_NAME_CHINESE_SIMPLIFIED, LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED },
	{ "zh_TW", LANG_NAME_CHINESE_TRADITIONAL, LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL },
#  endif
};
#else
static const struct
{
	const char *language;
	const char *name;
	const char *locale;
	const char *localeFallback;
} map[] =
{
	{ "",   N_("System locale"), "", "" },
#  if defined(ENABLE_NLS)
	{ "ca_ES", LANG_NAME_CATALAN, "ca_ES.UTF-8", "ca" },
	{ "cs_CZ", LANG_NAME_CZECH, "cs_CZ.UTF-8", "cs" },
	{ "da", LANG_NAME_DANISH, "da_DK.UTF-8", "da_DK" },
	{ "de", LANG_NAME_GERMAN, "de_DE.UTF-8", "de_DE" },
	{ "el_GR", LANG_NAME_GREEK, "el_GR.UTF-8", "el" },
	{ "en", LANG_NAME_ENGLISH, "en_US.UTF-8", "en_US" },
	{ "en_GB", LANG_NAME_ENGLISH_UK, "en_GB.UTF-8", "en_GB" },
	{ "es", LANG_NAME_SPANISH, "es_ES.UTF-8", "es_ES" },
	{ "et_EE", LANG_NAME_ESTONIAN, "et_EE.UTF-8", "et" },
//	{ "eu_ES", LANG_NAME_BASQUE, "eu_ES.UTF-8", "eu" },
	{ "fi", LANG_NAME_FINNISH, "fi_FI.UTF-8", "fi_FI" },
	{ "fr", LANG_NAME_FRENCH, "fr_FR.UTF-8", "fr_FR" },
	/* Our Frisian translation is the "West Frisian" variation of it. This
	 * variation is mostly spoken in the Dutch province Friesland (Fryslân
	 * in Frisian) and has ISO 639-3 code "fry".
	 */
	{ "fy_NL", LANG_NAME_FRISIAN_NETHERLANDS, "fy_NL.UTF-8", "fy" },
	{ "ga_IE", LANG_NAME_IRISH, "ga_IE.UTF-8", "ga" },
	{ "hr", LANG_NAME_CROATIAN, "hr_HR.UTF-8", "hr_HR" },
	{ "hu", LANG_NAME_HUNGARIAN, "hu_HU.UTF-8", "hu_HU" },
	{ "it", LANG_NAME_ITALIAN, "it_IT.UTF-8", "it_IT" },
	{ "ko_KR", LANG_NAME_KOREAN, "ko_KR.UTF-8", "ko" },
	{ "la", LANG_NAME_LATIN, "la.UTF-8", "la" },
	{ "lt", LANG_NAME_LITHUANIAN, "lt_LT.UTF-8", "lt_LT" },
//	{ "lv", LANG_NAME_LATVIAN, "lv_LV.UTF-8", "lv_LV" },
	{ "nb_NO", LANG_NAME_NORWEGIAN, "nb_NO.UTF-8", "nb" },
//	{ "nn_NO", LANG_NAME_NORWEGIAN_NYNORSK, "nn_NO.UTF-8", "nn" },
	{ "nl", LANG_NAME_DUTCH, "nl_NL.UTF-8", "nl_NL" },
	{ "pl", LANG_NAME_POLISH, "pl_PL.UTF-8", "pl_PL" },
	{ "pt_BR", LANG_NAME_PORTUGUESE_BRAZILIAN, "pt_BR.UTF-8", "pt_BR" },
	{ "pt", LANG_NAME_PORTUGUESE, "pt_PT.UTF-8", "pt_PT" },
	{ "ro", LANG_NAME_ROMANIAN, "ro_RO.UTF-8", "ro_RO" },
	{ "ru", LANG_NAME_RUSSIAN, "ru_RU.UTF-8", "ru_RU" },
	{ "sk", LANG_NAME_SLOVAK, "sk_SK.UTF-8", "sk_SK" },
	{ "sl_SI", LANG_NAME_SLOVENIAN, "sl_SI.UTF-8", "sl" },
//	{ "sv_SE", LANG_NAME_SWEDISH_SWEDEN, "sv_SE.UTF-8", "sv" },
//	{ "sv", LANG_NAME_SWEDISH, "sv.UTF-8", "sv" },
	{ "tr", LANG_NAME_TURKISH, "tr_TR.UTF-8", "tr_TR" },
//	{ "uz", LANG_NAME_UZBEK_CYRILLIC, "uz_UZ.UTF-8", "uz_UZ" },
	{ "uk_UA", LANG_NAME_UKRAINIAN, "uk_UA.UTF-8", "uk" },
	{ "zh_CN", LANG_NAME_CHINESE_SIMPLIFIED, "zh_CN.UTF-8", "zh_CN" },
	{ "zh_TW", LANG_NAME_CHINESE_TRADITIONAL, "zh_TW.UTF-8", "zh_TW" },
#  endif
};
#endif

static unsigned int selectedLanguage = 0;


/*!
 * Return the language part of the selected locale
 */
#if !defined(ENABLE_NLS)
const char *getLanguage()
{
	return "";
}
#elif defined(WZ_OS_WIN)
const char *getLanguage()
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
const char *getLanguage()
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


const char *getLanguageName()
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
	bool success = SUCCEEDED(SetThreadLocale(MAKELCID(MAKELANGID(usPrimaryLanguage, usSubLanguage), SORT_DEFAULT)));

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
static bool setLocaleUnix(const char *locale)
{
#ifdef WZ_OS_MAC
	// Set the appropriate language environment variable *before* the call to libintl's `setlocale`.

	// First, check if LC_ALL or LC_MESSAGES is set, as these may override LANG
	const char *pEnvLang = getenv("LC_ALL");
	if ((pEnvLang != nullptr) && (pEnvLang[0] != '\0'))
	{
		debug(LOG_WARNING, "Environment variable \"LC_ALL\" is set, and may override runtime language changes.");
	}
	pEnvLang = getenv("LC_MESSAGES");
	if ((pEnvLang != nullptr) && (pEnvLang[0] != '\0'))
	{
		debug(LOG_WARNING, "Environment variable \"LC_MESSAGES\" is set, and may override runtime language changes.");
	}

	// Set the LANG env-var (temporarily saving the old value)
	pEnvLang = getenv("LANG");
	std::string prior_LANG((pEnvLang != nullptr) ? pEnvLang : "");
#  if defined HAVE_SETENV
	setenv("LANG", locale, 1);
#  else
	# warning "No supported method to set environment variables"
#  endif
#endif // (ifdef WZ_OS_MAC)

	const char *actualLocale = setlocale(LC_ALL, locale);

	if (actualLocale == NULL)
	{
		info("Failed to set locale to \"%s\"", locale);
#ifdef WZ_OS_MAC
#  if defined HAVE_SETENV
		setenv("LANG", prior_LANG.c_str(), 1);
#  else
		# warning "No supported method to set environment variables"
#  endif
#endif
	}
	else
	{
		if (strcmp(locale, actualLocale))
		{
			debug(LOG_WZ, "Requested locale \"%s\", got \"%s\" instead", locale, actualLocale);
		}
	}

	const char * numericLocale = setlocale(LC_NUMERIC, "C"); // set radix character to the period (".")
	debug(LOG_WZ, "LC_NUMERIC: \"%s\"", (numericLocale != nullptr) ? numericLocale : "<null>");

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


void setNextLanguage(bool prev)
{
	selectedLanguage = (selectedLanguage + ARRAY_SIZE(map) + (prev? -1 : 1)) % ARRAY_SIZE(map);

	if (!setLanguage(map[selectedLanguage].language) && selectedLanguage != 0)
	{
		setNextLanguage(prev); // try next
	}
}


void initI18n()
{
	const char *textdomainDirectory = nullptr;

	if (!setLanguage("")) // set to system default
	{
		// no system default?
		debug(LOG_ERROR, "initI18n: No system language found");
	}
#ifdef WZ_OS_MAC
	{
		char resourcePath[PATH_MAX];
		CFURLRef resourceURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
		if (CFURLGetFileSystemRepresentation(resourceURL, true, (UInt8 *) resourcePath, PATH_MAX))
		{
			sstrcat(resourcePath, "/locale");
			textdomainDirectory = bindtextdomain(PACKAGE, resourcePath);
		}
		else
		{
			debug(LOG_ERROR, "Could not change to resources directory.");
		}

		if (resourceURL != NULL)
		{
			CFRelease(resourceURL);
		}

		debug(LOG_INFO, "resourcePath is %s", resourcePath);
	}
#else
# ifdef WZ_LOCALEDIR
	// New locale-dir setup (CMake)
	#ifndef WZ_LOCALEDIR_ISABSOLUTE
	// Treat WZ_LOCALEDIR as a relative path - append to the install PREFIX
	const std::string prefixDir = getWZInstallPrefix();
	const std::string dirSeparator(PHYSFS_getDirSeparator());
	std::string localeDir = prefixDir + dirSeparator + WZ_LOCALEDIR;
	textdomainDirectory = bindtextdomain(PACKAGE, localeDir.c_str());
	#else
	// Treat WZ_LOCALEDIR as an absolute path, and use directly
	textdomainDirectory = bindtextdomain(PACKAGE, WZ_LOCALEDIR);
	#endif
# else
	// Old locale-dir setup (autotools)
	textdomainDirectory = bindtextdomain(PACKAGE, LOCALEDIR);
# endif
#endif // ifdef WZ_OS_MAC
	if (!textdomainDirectory)
	{
		debug(LOG_ERROR, "initI18n: bindtextdomain failed!");
	}

	(void)bind_textdomain_codeset(PACKAGE, "UTF-8");
	(void)textdomain(PACKAGE);
}
