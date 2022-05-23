/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2020  Warzone 2100 Project

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

#include <sstream>
#include <vector>

#include <locale.h>
#include <physfs.h>
#include "wzpaths.h"

#ifdef WZ_OS_MAC
# include <CoreFoundation/CoreFoundation.h>
# include <CoreFoundation/CFURL.h>
#endif

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

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
#define LANG_NAME_ARABIC "العربية"
#define LANG_NAME_BASQUE "euskara"
#define LANG_NAME_BULGARIAN "български език"
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
#define LANG_NAME_INDONESIAN "Bahasa Indonesia"
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
	const char *localeFilename; // the actual filename on disk, minus the extension (ex. "en_GB")
	USHORT usPrimaryLanguage;
	USHORT usSubLanguage;
} map[] =
{
	{ "", N_("System locale"), "", LANG_NEUTRAL, SUBLANG_DEFAULT },
#  if defined(ENABLE_NLS)
	{ "ar_SA", LANG_NAME_ARABIC, "ar_SA", LANG_ARABIC, SUBLANG_DEFAULT },
	{ "bg", LANG_NAME_BULGARIAN, "bg_BG", LANG_BULGARIAN, SUBLANG_DEFAULT },
	{ "ca_ES", LANG_NAME_CATALAN, "ca_ES", LANG_CATALAN, SUBLANG_DEFAULT },
	{ "cs_CZ", LANG_NAME_CZECH, "cs", LANG_CZECH, SUBLANG_DEFAULT },
	{ "da", LANG_NAME_DANISH, "da", LANG_DANISH, SUBLANG_DEFAULT },
	{ "de", LANG_NAME_GERMAN, "de", LANG_GERMAN, SUBLANG_GERMAN },
	{ "el_GR", LANG_NAME_GREEK, "el", LANG_GREEK, SUBLANG_DEFAULT },
	{ "en", LANG_NAME_ENGLISH, "en", LANG_ENGLISH, SUBLANG_DEFAULT },
	{ "en_GB", LANG_NAME_ENGLISH_UK, "en_GB", LANG_ENGLISH, SUBLANG_ENGLISH_UK },
	{ "es", LANG_NAME_SPANISH, "es", LANG_SPANISH, SUBLANG_SPANISH },
	{ "et_EE", LANG_NAME_ESTONIAN, "et_EE", LANG_ESTONIAN, SUBLANG_DEFAULT },
//	{ "eu", LANG_NAME_BASQUE, LANG_BASQUE, SUBLANG_DEFAULT },
	{ "fi", LANG_NAME_FINNISH, "fi", LANG_FINNISH, SUBLANG_DEFAULT },
	{ "fr", LANG_NAME_FRENCH, "fr", LANG_FRENCH, SUBLANG_FRENCH },
	/* Our Frisian translation is the "West Frisian" variation of it. This
	 * variation is mostly spoken in the Dutch province Friesland (Fryslân
	 * in Frisian) and has ISO 639-3 code "fry".
	 *
	 * FIXME: We should really use a sub-language code for this. E.g.
	 *        fy_XX.
	 */
	{ "fy_NL", LANG_NAME_FRISIAN_NETHERLANDS, "fy", LANG_FRISIAN, SUBLANG_FRISIAN_NETHERLANDS },
	{ "ga_IE", LANG_NAME_IRISH, "ga", LANG_IRISH, SUBLANG_IRISH_IRELAND },
	{ "hr", LANG_NAME_CROATIAN, "hr", LANG_CROATIAN, SUBLANG_DEFAULT },
	{ "hu", LANG_NAME_HUNGARIAN, "hu", LANG_HUNGARIAN, SUBLANG_DEFAULT },
	{ "id", LANG_NAME_INDONESIAN, "id_ID", LANG_INDONESIAN, SUBLANG_DEFAULT },
	{ "it", LANG_NAME_ITALIAN, "it", LANG_ITALIAN, SUBLANG_ITALIAN },
	{ "ko_KR", LANG_NAME_KOREAN, "ko", LANG_KOREAN, SUBLANG_DEFAULT },
//	{ "la", LANG_NAME_LATIN, "la", LANG_LATIN, SUBLANG_DEFAULT },
	{ "lt", LANG_NAME_LITHUANIAN, "lt", LANG_LITHUANIAN, SUBLANG_DEFAULT },
//	{ "lv", LANG_NAME_LATVIAN, LANG_LATVIAN, SUBLANG_DEFAULT },
	// MSDN uses "no"...
	{ "nb_NO", LANG_NAME_NORWEGIAN, "nb", LANG_NORWEGIAN, SUBLANG_DEFAULT },
//	{ "nn", LANG_NAME_NORWEGIAN_NYNORSK, LANG_NORWEGIAN, SUBLANG_NORWEGIAN_NYNORSK },
	{ "nl", LANG_NAME_DUTCH, "nl", LANG_DUTCH, SUBLANG_DUTCH },
	{ "pl", LANG_NAME_POLISH, "pl", LANG_POLISH, SUBLANG_DEFAULT },
	{ "pt_BR", LANG_NAME_PORTUGUESE_BRAZILIAN, "pt_BR", LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN },
	{ "pt", LANG_NAME_PORTUGUESE, "pt", LANG_PORTUGUESE, SUBLANG_DEFAULT },
	{ "ro", LANG_NAME_ROMANIAN, "ro", LANG_ROMANIAN, SUBLANG_DEFAULT },
	{ "ru", LANG_NAME_RUSSIAN, "ru", LANG_RUSSIAN, SUBLANG_DEFAULT },
	{ "sk", LANG_NAME_SLOVAK, "sk", LANG_SLOVAK, SUBLANG_DEFAULT },
	{ "sl_SI", LANG_NAME_SLOVENIAN, "sl", LANG_SLOVENIAN, SUBLANG_DEFAULT },
#if (WINVER >= 0x0600)
//	{ "sv_SE", LANG_NAME_SWEDISH_SWEDEN, LANG_SWEDISH, SUBLANG_SWEDISH_SWEDEN },
#else
//	{ "sv_SE", LANG_NAME_SWEDISH_SWEDEN, LANG_SWEDISH, SUBLANG_SWEDISH },
#endif
//	{ "sv", LANG_NAME_SWEDISH, LANG_SWEDISH, SUBLANG_DEFAULT },
	{ "tr", LANG_NAME_TURKISH, "tr", LANG_TURKISH, SUBLANG_DEFAULT },
//	{ "uz", LANG_NAME_UZBEK_CYRILLIC, LANG_UZBEK, SUBLANG_UZBEK_CYRILLIC },
	{ "uk_UA", LANG_NAME_UKRAINIAN, "uk_UA", LANG_UKRAINIAN, SUBLANG_DEFAULT },
	{ "zh_CN", LANG_NAME_CHINESE_SIMPLIFIED, "zh_CN", LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED },
	{ "zh_TW", LANG_NAME_CHINESE_TRADITIONAL, "zh_TW", LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL },
#  endif
};
#else
static const struct
{
	const char *language;
	const char *name;
	const char *localeFilename; // the actual filename on disk, minus the extension (ex. "en_GB")
	const char *locale;
	const char *localeFallback;
} map[] =
{
	{ "",   N_("System locale"), "", "", "" },
#  if defined(ENABLE_NLS)
	{ "ar_SA", LANG_NAME_ARABIC, "ar_SA", "ar_SA.UTF-8", "ar"},
	{ "bg", LANG_NAME_BULGARIAN, "bg_BG", "bg_BG.UTF-8", "bg" },
	{ "ca_ES", LANG_NAME_CATALAN, "ca_ES", "ca_ES.UTF-8", "ca" },
	{ "cs_CZ", LANG_NAME_CZECH, "cs", "cs_CZ.UTF-8", "cs" },
	{ "da", LANG_NAME_DANISH, "da", "da_DK.UTF-8", "da_DK" },
	{ "de", LANG_NAME_GERMAN, "de", "de_DE.UTF-8", "de_DE" },
	{ "el_GR", LANG_NAME_GREEK, "el", "el_GR.UTF-8", "el" },
	{ "en", LANG_NAME_ENGLISH, "en", "en_US.UTF-8", "en_US" },
	{ "en_GB", LANG_NAME_ENGLISH_UK, "en_GB", "en_GB.UTF-8", "en_GB" },
	{ "es", LANG_NAME_SPANISH, "es", "es_ES.UTF-8", "es_ES" },
	{ "et_EE", LANG_NAME_ESTONIAN, "et_EE", "et_EE.UTF-8", "et" },
//	{ "eu_ES", LANG_NAME_BASQUE, "eu_ES.UTF-8", "eu" },
	{ "fi", LANG_NAME_FINNISH, "fi", "fi_FI.UTF-8", "fi_FI" },
	{ "fr", LANG_NAME_FRENCH, "fr", "fr_FR.UTF-8", "fr_FR" },
	/* Our Frisian translation is the "West Frisian" variation of it. This
	 * variation is mostly spoken in the Dutch province Friesland (Fryslân
	 * in Frisian) and has ISO 639-3 code "fry".
	 */
	{ "fy_NL", LANG_NAME_FRISIAN_NETHERLANDS, "fy", "fy_NL.UTF-8", "fy" },
	{ "ga_IE", LANG_NAME_IRISH, "ga", "ga_IE.UTF-8", "ga" },
	{ "hr", LANG_NAME_CROATIAN, "hr", "hr_HR.UTF-8", "hr_HR" },
	{ "hu", LANG_NAME_HUNGARIAN, "hu", "hu_HU.UTF-8", "hu_HU" },
	{ "id", LANG_NAME_INDONESIAN, "id_ID", "id_ID.UTF-8", "id" },
	{ "it", LANG_NAME_ITALIAN, "it", "it_IT.UTF-8", "it_IT" },
	{ "ko_KR", LANG_NAME_KOREAN, "ko", "ko_KR.UTF-8", "ko" },
	{ "la", LANG_NAME_LATIN, "la", "la.UTF-8", "la" },
	{ "lt", LANG_NAME_LITHUANIAN, "lt", "lt_LT.UTF-8", "lt_LT" },
//	{ "lv", LANG_NAME_LATVIAN, "lv_LV.UTF-8", "lv_LV" },
	{ "nb_NO", LANG_NAME_NORWEGIAN, "nb", "nb_NO.UTF-8", "nb" },
//	{ "nn_NO", LANG_NAME_NORWEGIAN_NYNORSK, "nn_NO.UTF-8", "nn" },
	{ "nl", LANG_NAME_DUTCH, "nl", "nl_NL.UTF-8", "nl_NL" },
	{ "pl", LANG_NAME_POLISH, "pl", "pl_PL.UTF-8", "pl_PL" },
	{ "pt_BR", LANG_NAME_PORTUGUESE_BRAZILIAN, "pt_BR", "pt_BR.UTF-8", "pt_BR" },
	{ "pt", LANG_NAME_PORTUGUESE, "pt", "pt_PT.UTF-8", "pt_PT" },
	{ "ro", LANG_NAME_ROMANIAN, "ro", "ro_RO.UTF-8", "ro_RO" },
	{ "ru", LANG_NAME_RUSSIAN, "ru", "ru_RU.UTF-8", "ru_RU" },
	{ "sk", LANG_NAME_SLOVAK, "sk", "sk_SK.UTF-8", "sk_SK" },
	{ "sl_SI", LANG_NAME_SLOVENIAN, "sl", "sl_SI.UTF-8", "sl" },
//	{ "sv_SE", LANG_NAME_SWEDISH_SWEDEN, "sv_SE.UTF-8", "sv" },
//	{ "sv", LANG_NAME_SWEDISH, "sv.UTF-8", "sv" },
	{ "tr", LANG_NAME_TURKISH, "tr", "tr_TR.UTF-8", "tr_TR" },
//	{ "uz", LANG_NAME_UZBEK_CYRILLIC, "uz_UZ.UTF-8", "uz_UZ" },
	{ "uk_UA", LANG_NAME_UKRAINIAN, "uk_UA", "uk_UA.UTF-8", "uk" },
	{ "zh_CN", LANG_NAME_CHINESE_SIMPLIFIED, "zh_CN", "zh_CN.UTF-8", "zh_CN" },
	{ "zh_TW", LANG_NAME_CHINESE_TRADITIONAL, "zh_TW", "zh_TW.UTF-8", "zh_TW" },
#  endif
};
#endif

static unsigned int selectedLanguage = 0;
static bool canUseLANGUAGEEnvVar = false;

static char *compileDate = nullptr;

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
		wz_info("Failed to set locale to \"%d\"", usPrimaryLanguage);
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
	const char *pEnvLang = nullptr;

	// Set the appropriate language environment variables *before* the call to libintl's `setlocale`.
	// (The LANGUAGE and LANG env-vars) (temporarily saving the old values)
	// This is *REQUIRED* on some platforms where other environment variables (LC_ALL, LC_MESSAGES, etc) are sometimes set (ex. macOS), or run-time language switching won't work
	pEnvLang = getenv("LANGUAGE");
	std::string prior_LANGUAGE((pEnvLang != nullptr) ? pEnvLang : "");
	pEnvLang = getenv("LANG");
	std::string prior_LANG((pEnvLang != nullptr) ? pEnvLang : "");
#if defined(HAVE_SETENV)
	setenv("LANGUAGE", locale, 1);
	setenv("LANG", locale, 1);
#else
	# warning "No supported method to set environment variables"
#endif

	const char *actualLocale = setlocale(LC_ALL, locale);

	if (actualLocale == NULL)
	{
		wz_info("Failed to set locale to \"%s\"", locale);
#if defined(HAVE_SETENV)
		setenv("LANGUAGE", prior_LANGUAGE.c_str(), 1);
		setenv("LANG", prior_LANG.c_str(), 1);
#else
		# warning "No supported method to set environment variables"
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
static bool setLocaleUnix_LANGUAGEFallback(const char *localeFilename)
{
	if (!canUseLANGUAGEEnvVar)
	{
		return false;
	}
#if defined(HAVE_SETENV) // or possibly other methods in the future?
	const char *pEnvLang = getenv("LANGUAGE");
	std::string prior_LANGUAGE((pEnvLang != nullptr) ? pEnvLang : "");

	setlocale(LC_ALL, ""); // use default system locale

#if defined(HAVE_SETENV)
	setenv("LANGUAGE", localeFilename, 1); // libintl will pick this up and use this preferentially
#else
	# warning "No supported method to set environment variables"
#endif

	const char * numericLocale = setlocale(LC_NUMERIC, "C"); // set radix character to the period (".")
	debug(LOG_WZ, "LC_NUMERIC: \"%s\"", (numericLocale != nullptr) ? numericLocale : "<null>");

	return true;
#else // defined(HAVE_SETENV)
	return false;
#endif // defined(HAVE_SETENV)
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
			return setLocaleUnix(map[i].locale) || setLocaleUnix(map[i].localeFallback) || setLocaleUnix_LANGUAGEFallback(map[i].localeFilename);
#  endif
		}
	}

	debug(LOG_ERROR, "Requested language \"%s\" not supported.", language);

	return false;
#endif
}

std::vector<locale_info> getLocales()
{
	std::vector<locale_info> locales;
	for (const auto &entry: map)
	{
		locales.push_back({entry.language, entry.name});
	}

	return locales;
}

std::string wzBindTextDomain(const char *domainname, const char *dirname)
{
#if (LIBINTL_VERSION >= 0x001500) && defined(_WIN32) && !defined(__CYGWIN__)
	// gettext 0.21+ provides a wbindtextdomain function on native Windows platforms
	// that properly supports Unicode paths

	// convert the dirname from UTF-8 to UTF-16
	int wstr_len = MultiByteToWideChar(CP_UTF8, 0, dirname, -1, NULL, 0);
	if (wstr_len <= 0)
	{
		DWORD dwError = GetLastError();
		debug(LOG_ERROR, "Could not not convert string from UTF-8; MultiByteToWideChar failed with error %lu: %s\n", dwError, dirname);
		return std::string();
	}
	auto wstr_dirname = std::vector<wchar_t>(wstr_len, L'\0');
	if (MultiByteToWideChar(CP_UTF8, 0, dirname, -1, &wstr_dirname[0], wstr_len) == 0)
	{
		DWORD dwError = GetLastError();
		debug(LOG_ERROR, "Could not not convert string from UTF-8; MultiByteToWideChar[2] failed with error %lu: %s\n", dwError, dirname);
		return std::string();
	}

	// and call wbindtextdomain
	const wchar_t *pResult = wbindtextdomain(domainname, wstr_dirname.data());
	if (!pResult)
	{
		debug(LOG_ERROR, "wbindtextdomain failed");
		return std::string();
	}

	// convert the result back to UTF-8
	std::vector<char> utf8Buffer;
	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, pResult, -1, NULL, 0, NULL, NULL);
	if ( utf8Len <= 0 )
	{
		// Encoding conversion error
		DWORD dwError = GetLastError();
		debug(LOG_ERROR, "Could not not convert string to UTF-8; WideCharToMultiByte failed with error %lu\n", dwError);
		return std::string();
	}
	utf8Buffer.resize(utf8Len, 0);
	if ( (utf8Len = WideCharToMultiByte(CP_UTF8, 0, pResult, -1, &utf8Buffer[0], utf8Len, NULL, NULL)) <= 0 )
	{
		// Encoding conversion error
		DWORD dwError = GetLastError();
		debug(LOG_ERROR, "Could not not convert string to UTF-8; WideCharToMultiByte[2] failed with error %lu\n", dwError);
		return std::string();
	}
	return std::string(utf8Buffer.data(), utf8Len - 1);
#else
	// call the normal bindtextdomain function
	const char * pResult = bindtextdomain(domainname, dirname);
	if (!pResult)
	{
		return std::string();
	}
	return std::string(pResult);
#endif
}

static bool checkSupportsLANGUAGEenvVarOverride()
{
#if defined(_LIBINTL_H) && defined(LIBINTL_VERSION)
	// Only known to be supported by GNU Gettext runtime (libintl) behavior
	// GNU Gettext permits overriding with the "LANGUAGE" environment variable to specify the language for messages
	// As long as the current locale does not equal the "plain" C locale (i.e. just "C")

	optional<std::string> prevLocale;
	const char *actualLocale = setlocale(LC_MESSAGES, NULL);
	if (actualLocale != NULL)
	{
		prevLocale = actualLocale;
	}

	// Try to set "default" locale
	actualLocale = setlocale(LC_MESSAGES, "");
	if (actualLocale == NULL)
	{
		// Failed
		return false;
	}

	// Can only use LANGUAGE override if the locale is *not* plain "C"
	bool result = !(strcmp(actualLocale, "C") == 0);

	if (prevLocale.has_value())
	{
		// reset back to prev locale before test
		setlocale(LC_MESSAGES, prevLocale.value().c_str());
	}

	return result;
#else
	return false;
#endif
}

void initI18n()
{
	std::string textdomainDirectory;

	canUseLANGUAGEEnvVar = checkSupportsLANGUAGEenvVarOverride();

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
			textdomainDirectory = wzBindTextDomain(PACKAGE, resourcePath);
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
	textdomainDirectory = wzBindTextDomain(PACKAGE, localeDir.c_str());
	#else
	// Treat WZ_LOCALEDIR as an absolute path, and use directly
	textdomainDirectory = wzBindTextDomain(PACKAGE, WZ_LOCALEDIR);
	#endif
# else
	// Old locale-dir setup (autotools)
	textdomainDirectory = wzBindTextDomain(PACKAGE, LOCALEDIR);
# endif
#endif // ifdef WZ_OS_MAC
	if (textdomainDirectory.empty())
	{
		debug(LOG_ERROR, "initI18n: bindtextdomain failed!");
	}

	(void)bind_textdomain_codeset(PACKAGE, "UTF-8");
	(void)textdomain(PACKAGE);
}

// convert macro __DATE__ to ISO 8601 format
const char *getCompileDate()
{
	if (compileDate == nullptr)
	{
		std::istringstream date(__DATE__);
		std::string monthName;
		int day = 0, month = 0, year = 0;
		date >> monthName >> day >> year;

		std::string monthNames[] = {
						"Jan", "Feb", "Mar", "Apr",
						"May", "Jun", "Jul", "Aug",
						"Sep", "Oct", "Nov", "Dec"
		                           };
		for (int i = 0; i < 12; i++)
		{
			if (monthNames[i] == monthName)
			{
				month = i + 1;
				break;
			}
		}
		asprintfNull(&compileDate, "%04d-%02d-%02d", year, month, day);
	}
	return compileDate;
}
