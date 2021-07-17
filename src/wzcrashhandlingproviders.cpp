/*
	This file is part of Warzone 2100.
	Copyright (C) 2021  Warzone 2100 Project

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

#include <3rdparty/json/json.hpp> // Must come before WZ includes

#include "wzcrashhandlingproviders.h"

#include "lib/framework/wzglobal.h" // required for config.h
#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/wzstring.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/string_ext.h"

#include "version.h"
#include "urlhelpers.h"

/* Crash-handling providers */

#if defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
# define WZ_CRASHHANDLING_PROVIDER
#endif

#if defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
# include <sentry.h>
# include <wz-sentry-config.h>
# if !defined(WZ_CRASHHANDLING_PROVIDER_SENTRY_DSN)
#  define WZ_CRASHHANDLING_PROVIDER_SENTRY_DSN ""
# endif
static bool enabledSentryProvider = false;
#endif

const size_t tagKeyMaxLength = 32;
const size_t tagValueMaxLength = 200;

#if defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
static bool initCrashHandlingProvider_Sentry(const std::string& platformPrefDir_Input, const std::string& defaultLogFilePath)
{
	ASSERT_OR_RETURN(false, !platformPrefDir_Input.empty(), "platformPrefDir must not be empty");
	ASSERT_OR_RETURN(false, !defaultLogFilePath.empty(), "defaultLogFilePath must not be empty");
	if (strlen(WZ_CRASHHANDLING_PROVIDER_SENTRY_DSN) == 0)
	{
		debug(LOG_INFO, "Insufficient configuration to enable - skipping");
		return false;
	}
	sentry_options_t *options = sentry_options_new();
	if (!options)
	{
		return false;
	}
	auto releaseString = version_getBuildIdentifierReleaseString();
	auto environmentString = version_getBuildIdentifierReleaseEnvironment();
	sentry_options_set_dsn(options, WZ_CRASHHANDLING_PROVIDER_SENTRY_DSN);
	sentry_options_set_release(options, releaseString.c_str());
	sentry_options_set_environment(options, environmentString.c_str());
	// for the temp path, always use a subdirectory of the default platform pref dir
	// Make sure that we have a directory separator at the end of the string
	std::string platformPrefDir = platformPrefDir_Input;
	if (!strEndsWith(platformPrefDir, PHYSFS_getDirSeparator()))
	{
		platformPrefDir += PHYSFS_getDirSeparator();
	}
	std::string crashDbPath = platformPrefDir + ".crash-db";
#if defined(WZ_OS_WIN)
	// On Windows: Convert UTF-8 path to UTF-16 and call sentry_options_set_database_pathw
	std::vector<wchar_t> wUtf16Path;
	if (!win_utf8ToUtf16(crashDbPath.c_str(), wUtf16Path))
	{
		debug(LOG_FATAL, "Unable to convert path string to UTF16 - fatal error");
		abort();
	}
	sentry_options_set_database_pathw(options, wUtf16Path.data());
#else
	sentry_options_set_database_path(options, crashDbPath.c_str());
#endif
	std::string logFileFullPath = platformPrefDir + defaultLogFilePath;
#if defined(WZ_OS_WIN)
	// On Windows: Convert UTF-8 path to UTF-16 and call sentry_options_add_attachmentw
	if (!win_utf8ToUtf16(logFileFullPath.c_str(), wUtf16Path))
	{
		debug(LOG_FATAL, "Unable to convert path string (2) to UTF16 - fatal error");
		abort();
	}
	sentry_options_add_attachmentw(options, wUtf16Path.data());
#else
	sentry_options_add_attachment(options, logFileFullPath.c_str());
#endif
	int result = sentry_init(options);
	return result == 0;
}

static bool shutdownCrashHandlingProvider_Sentry()
{
	sentry_close();
	return true;
}

static bool crashHandlingProviderSetTag_Sentry(const std::string& key, const std::string& value)
{
	sentry_set_tag(key.c_str(), value.c_str());
	return true;
}

sentry_value_t mapJsonToSentryValue(const nlohmann::json& instance); // forward-declaration

static sentry_value_t mapJsonObjectToSentryValue(const nlohmann::json &obj)
{
	if (!obj.is_object())
	{
		return sentry_value_new_null();
	}
	sentry_value_t value = sentry_value_new_object();
	for (auto it = obj.begin(); it != obj.end(); ++it)
	{
		sentry_value_t prop_val = mapJsonToSentryValue(it.value());
		sentry_value_set_by_key(value, it.key().c_str(), prop_val);
	}
	return value;
}

static sentry_value_t mapJsonArrayToSentryValue(const nlohmann::json &array)
{
	if (!array.is_array())
	{
		return sentry_value_new_null();
	}
	sentry_value_t value = sentry_value_new_list();
	for (int i = 0; i < array.size(); i++)
	{
		sentry_value_append(value, mapJsonToSentryValue(array.at(i)));
	}
	return value;
}

sentry_value_t mapJsonToSentryValue(const nlohmann::json& instance)
{
	switch (instance.type())
	{
		case nlohmann::json::value_t::null:
			return sentry_value_new_null();
		case nlohmann::json::value_t::boolean:
			return sentry_value_new_bool(instance.get<bool>());
		case nlohmann::json::value_t::number_integer:
		case nlohmann::json::value_t::number_unsigned:
			return sentry_value_new_int32(instance.get<int32_t>());
		case nlohmann::json::value_t::number_float:
			return sentry_value_new_double(instance.get<double>());
		case nlohmann::json::value_t::string:
		{
			auto strValue = instance.get<std::string>();
			return sentry_value_new_string(strValue.c_str());
		}
		case nlohmann::json::value_t::array:
			return mapJsonArrayToSentryValue(instance);
		case nlohmann::json::value_t::object:
			return mapJsonObjectToSentryValue(instance);
		case nlohmann::json::value_t::binary:
			debug(LOG_ERROR, "Unexpected binary value type");
			return sentry_value_new_null();
		case nlohmann::json::value_t::discarded:
			return sentry_value_new_null();
	}
	return sentry_value_new_null(); // should never be reached
}

static bool crashHandlingProviderSetContext_Sentry(const std::string& key, const nlohmann::json& contextDictionary)
{
	ASSERT_OR_RETURN(false, contextDictionary.is_object(), "contextDictionary must be an object");
	sentry_value_t value = mapJsonObjectToSentryValue(contextDictionary);
	if (sentry_value_get_type(value) != SENTRY_VALUE_TYPE_OBJECT)
	{
		return false;
	}
	sentry_set_context(key.c_str(), value);
	return true;
}

#endif // defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)

bool initCrashHandlingProvider(const std::string& platformPrefDir, const std::string& defaultLogFilePath)
{
#if !defined(WZ_CRASHHANDLING_PROVIDER)
	return false;
#elif defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
	// Sentry crash-handling provider
	ASSERT_OR_RETURN(true, !enabledSentryProvider, "Called more than once");
	enabledSentryProvider = initCrashHandlingProvider_Sentry(platformPrefDir, defaultLogFilePath);
	return enabledSentryProvider;
#else
	#error No available init for crash handling provider
	return false;
#endif
}

bool shutdownCrashHandlingProvider()
{
#if !defined(WZ_CRASHHANDLING_PROVIDER)
	return false;
#elif defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
	// Sentry crash-handling provider
	ASSERT_OR_RETURN(true, enabledSentryProvider, "Already shut-down");
	bool result = shutdownCrashHandlingProvider_Sentry();
	enabledSentryProvider = !result;
	return result;
#else
	// No available shutdown for crash handling provider
	return false;
#endif
}

bool crashHandlingProviderSetTag(const std::string& key, const std::string& value)
{
	ASSERT_OR_RETURN(false, key.size() <= tagKeyMaxLength, "Maximum key length exceeded");
	ASSERT_OR_RETURN(false, value.size() <= tagValueMaxLength, "Maximum value length exceeded");
#if !defined(WZ_CRASHHANDLING_PROVIDER)
	return false;
#elif defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
	// Sentry crash-handling provider
	if (!enabledSentryProvider) { return false; }
	return crashHandlingProviderSetTag_Sentry(key, value);
#else
	// No available setTag for crash handling provider
	return false;
#endif
}

bool crashHandlingProviderSetContext(const std::string& key, const nlohmann::json& contextDictionary)
{
#if !defined(WZ_CRASHHANDLING_PROVIDER)
	return false;
#elif defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
	// Sentry crash-handling provider
	if (!enabledSentryProvider) { return false; }
	return crashHandlingProviderSetContext_Sentry(key, contextDictionary);
#else
	// No available setTag for crash handling provider
	return false;
#endif
}

bool useCrashHandlingProvider(int argc, const char * const *argv)
{
#if !defined(WZ_CRASHHANDLING_PROVIDER)
	return false; // use native crash-handling exception handler
#else
	// if compiled with a crash-handling provider, search for "--wz-crash-rpt"
	if (argv)
	{
		for (int i = 0; i < argc; ++i)
		{
			if (argv[i] && !strcasecmp(argv[i], "--wz-crash-rpt"))
			{
				return false;
			}
		}
	}
	return true;
#endif
}

