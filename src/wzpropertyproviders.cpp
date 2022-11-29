/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2021  Warzone 2100 Project

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

#include "wzpropertyproviders.h"
#include <string>
#include <unordered_map>
#include <vector>

#include "lib/framework/wzglobal.h" // required for config.h
#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/wzstring.h"

#include "version.h"
#include "build_tools/autorevision.h"
#include "urlhelpers.h"
#include "activity.h"

#include <LaunchInfo.h>

#ifndef PACKAGE_DISTRIBUTOR
# define PACKAGE_DISTRIBUTOR "UNKNOWN"
#endif

// Includes for Windows

#if defined(WZ_OS_WIN)
#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#ifndef WIN32_EXTRA_LEAN
# define WIN32_EXTRA_LEAN
#endif
# undef NOMINMAX
# define NOMINMAX 1
#include <windows.h>
#include <psapi.h>
#endif

// MARK: - BuildPropertyProvider

enum class BuildProperty {
	GIT_BRANCH,
	GIT_TAG,
	GIT_EXTRA,
	GIT_FULL_HASH,
	GIT_SHORT_HASH,
	GIT_WC_MODIFIED,
	GIT_MOST_RECENT_COMMIT_DATE,
	WZ_PACKAGE_DISTRIBUTOR,
	COMPILE_DATE,
	VERSION_STRING,
	PLATFORM,
	PARENTPROCESS,
	ANCESTORS,
	// WZ 4.1.0+
	WIN_PACKAGE_FULLNAME
};

#if defined(WZ_OS_WIN)

#include <ntverp.h>				// Windows SDK - include for access to VER_PRODUCTBUILD
#if VER_PRODUCTBUILD >= 9200
	// 9200 is the Windows SDK 8.0 (which introduced family support)
	#include <winapifamily.h>	// Windows SDK
#else
	// Earlier SDKs don't have the concept of families - provide simple implementation
	// that treats everything as "desktop"
	#if !defined(WINAPI_PARTITION_DESKTOP)
		#define WINAPI_PARTITION_DESKTOP			0x00000001
	#endif
	#if !defined(WINAPI_FAMILY_PARTITION)
		#define WINAPI_FAMILY_PARTITION(Partition)	((WINAPI_PARTITION_DESKTOP & Partition) == Partition)
	#endif
#endif

static std::string Get_WinPackageFullName()
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	typedef LONG (WINAPI *GetCurrentPackageFullNameFunc)(
		UINT32 *packageFullNameLength,
		PWSTR  packageFullName
	);
#if !defined(APPMODEL_ERROR_NO_PACKAGE)
#  define APPMODEL_ERROR_NO_PACKAGE 15700
#endif

	HMODULE hKernel32 = GetModuleHandleW(L"kernel32");

	GetCurrentPackageFullNameFunc _GetCurrentPackageFullName = reinterpret_cast<GetCurrentPackageFullNameFunc>(reinterpret_cast<void*>(GetProcAddress(hKernel32, "GetCurrentPackageFullName")));

	if (!_GetCurrentPackageFullName)
	{
		return "";
	}

	UINT32 length = 0;
	LONG rc = _GetCurrentPackageFullName(&length, NULL);
	if (rc != ERROR_INSUFFICIENT_BUFFER)
	{
		if (rc == APPMODEL_ERROR_NO_PACKAGE)
		{
			// Process has no package identity
			return "";
		}
		else
		{
			// Unexpected error
			debug(LOG_WARNING, "Error %ld in GetCurrentPackageFullName", rc);
			return "";
		}
	}

	std::vector<wchar_t> fullNameW(length, 0);
	rc = _GetCurrentPackageFullName(&length, &fullNameW[0]);
	if (rc != ERROR_SUCCESS)
	{
		debug(LOG_WARNING, "Error %ld retrieving PackageFullName", rc);
		return "";
	}

	// convert UTF-16 fullNameW to UTF-8
	std::vector<char> utf8Buffer;
	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, fullNameW.data(), -1, NULL, 0, NULL, NULL);
	if ( utf8Len <= 0 )
	{
		// Encoding conversion error
		debug(LOG_ERROR, "WideCharToMultiByte[1] failed");
		return "";
	}
	utf8Buffer.resize(utf8Len, 0);
	if ( (utf8Len = WideCharToMultiByte(CP_UTF8, 0, fullNameW.data(), -1, &utf8Buffer[0], utf8Len, NULL, NULL)) <= 0 )
	{
		// Encoding conversion error
		debug(LOG_ERROR, "WideCharToMultiByte[2] failed");
		return "";
	}
	std::string fullName = std::string(utf8Buffer.data(), utf8Len - 1);
	return fullName;
#else
	// For now, return empty string
	return "";
#endif
}
#endif

static std::string GetCurrentBuildPropertyValue(const BuildProperty& property)
{
	using BP = BuildProperty;
	switch (property)
	{
		case BP::GIT_BRANCH:
			return VCS_BRANCH;
		case BP::GIT_TAG:
			return VCS_TAG;
		case BP::GIT_EXTRA:
			return VCS_EXTRA;
		case BP::GIT_FULL_HASH:
			return VCS_FULL_HASH;
		case BP::GIT_SHORT_HASH:
			return VCS_SHORT_HASH;
		case BP::GIT_WC_MODIFIED:
			return std::to_string(VCS_WC_MODIFIED);
		case BP::GIT_MOST_RECENT_COMMIT_DATE:
			return VCS_MOST_RECENT_COMMIT_DATE;
		case BP::WZ_PACKAGE_DISTRIBUTOR:
			return PACKAGE_DISTRIBUTOR;
		case BP::COMPILE_DATE:
			return getCompileDate();
		case BP::VERSION_STRING:
			return version_getVersionString();
		case BP::PLATFORM:
			return wzGetPlatform().toUtf8();
		case BP::PARENTPROCESS:
			return LaunchInfo::getParentImageName().fullPath();
		case BP::ANCESTORS:
		{
			auto ancestors = LaunchInfo::getAncestorProcesses();
			std::string ancestorsStr;
			for (const auto& ancestor : ancestors)
			{
				if (!ancestorsStr.empty()) ancestorsStr += ";";
				ancestorsStr += "\"";
				ancestorsStr += WzString::fromUtf8(ancestor.imageFileName.fullPath()).replace("\"", "\\\"").toStdString();
				ancestorsStr += "\"";
			}
			return ancestorsStr;
		}
		case BP::WIN_PACKAGE_FULLNAME:
#if defined(WZ_OS_WIN)
			return Get_WinPackageFullName();
#else
			return "";
#endif
	}
	return ""; // silence warning
}

static const std::unordered_map<std::string, BuildProperty> strToBuildPropertyMap = {
	{"GIT_BRANCH", BuildProperty::GIT_BRANCH},
	{"GIT_TAG", BuildProperty::GIT_TAG},
	{"GIT_EXTRA", BuildProperty::GIT_EXTRA},
	{"GIT_FULL_HASH", BuildProperty::GIT_FULL_HASH},
	{"GIT_SHORT_HASH", BuildProperty::GIT_SHORT_HASH},
	{"GIT_WC_MODIFIED", BuildProperty::GIT_WC_MODIFIED},
	{"GIT_MOST_RECENT_COMMIT_DATE", BuildProperty::GIT_MOST_RECENT_COMMIT_DATE},
	{"WZ_PACKAGE_DISTRIBUTOR", BuildProperty::WZ_PACKAGE_DISTRIBUTOR},
	{"COMPILE_DATE", BuildProperty::COMPILE_DATE},
	{"VERSION_STRING", BuildProperty::VERSION_STRING},
	{"PLATFORM", BuildProperty::PLATFORM},
	{"PARENTPROCESS", BuildProperty::PARENTPROCESS},
	{"ANCESTORS", BuildProperty::ANCESTORS},
	{"WIN_PACKAGE_FULLNAME", BuildProperty::WIN_PACKAGE_FULLNAME}
};

BuildPropertyProvider::~BuildPropertyProvider() { }
bool BuildPropertyProvider::getPropertyValue(const std::string& property, std::string& output_value)
{
	auto it = strToBuildPropertyMap.find(property);
	if (it == strToBuildPropertyMap.end())
	{
		return false;
	}
	output_value = GetCurrentBuildPropertyValue(it->second);
	return true;
}

// MARK: - EnvironmentPropertyProvider

static const std::unordered_map<std::string, EnvironmentPropertyProvider::EnvironmentProperty> strToEnvironmentPropertyMap = {
	{"FIRST_LAUNCH", EnvironmentPropertyProvider::EnvironmentProperty::FIRST_LAUNCH},
	{"INSTALLED_PATH", EnvironmentPropertyProvider::EnvironmentProperty::INSTALLED_PATH},
	{"WIN_INSTALLED_BINARIES", EnvironmentPropertyProvider::EnvironmentProperty::WIN_INSTALLED_BINARIES},
	{"WIN_LOADEDMODULES", EnvironmentPropertyProvider::EnvironmentProperty::WIN_LOADEDMODULES},
	{"WIN_LOADEDMODULENAMES", EnvironmentPropertyProvider::EnvironmentProperty::WIN_LOADEDMODULENAMES},
	{"ENV_VAR_NAMES", EnvironmentPropertyProvider::EnvironmentProperty::ENV_VAR_NAMES},
	{"SYSTEM_RAM", EnvironmentPropertyProvider::EnvironmentProperty::SYSTEM_RAM},
};

#if defined(WZ_OS_WIN)

static bool win_Utf16toUtf8(const wchar_t* buffer, std::vector<char>& u8_buffer)
{
	// Convert the UTF-16 to UTF-8
	int outputLength = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, NULL, 0, NULL, NULL);
	if (outputLength <= 0)
	{
//		debug(LOG_ERROR, "Encoding conversion error.");
		return false;
	}
	if (u8_buffer.size() < static_cast<size_t>(outputLength))
	{
		u8_buffer.resize(outputLength, 0);
	}
	if (WideCharToMultiByte(CP_UTF8, 0, buffer, -1, &u8_buffer[0], outputLength, NULL, NULL) <= 0)
	{
//		debug(LOG_ERROR, "Encoding conversion error.");
		return false;
	}
	return true;
}

static std::vector<std::string> Get_WinInstalledBinaries()
{
	std::vector<std::string> files;

	// Get the full path to the folder containing this executable
	std::string binPath = LaunchInfo::getCurrentProcessDetails().imageFileName.dirname();
	if (binPath.empty())
	{
		return {};
	}

	// List files in the same directory as this executable
	std::string findFileStr = binPath + "\\*";
	std::vector<wchar_t> wFindFileStr;
	if (!win_utf8ToUtf16(findFileStr.c_str(), wFindFileStr))
	{
		return {};
	}
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = FindFirstFileW(wFindFileStr.data(), &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return {};
	}
	std::vector<char> u8_buffer(MAX_PATH, 0);
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			// convert ffd.cFileName to UTF-8
			if (!win_Utf16toUtf8(ffd.cFileName, u8_buffer))
			{
//				debug(LOG_ERROR, "Encoding conversion error.");
				continue;
			}
			files.push_back(std::string(u8_buffer.data()));
		}
	} while (FindNextFileW(hFind, &ffd) != 0);
	FindClose(hFind);

	return files;
}

#define WIN_MAX_EXTENDED_PATH 32767

static std::vector<std::string> Get_WinProcessModules(bool filenamesOnly = false)
{
	std::vector<std::string> processModules;
	std::vector<HMODULE> hMods(2048);
	DWORD cbNeeded = 2048 * sizeof(HMODULE);
	do {
		hMods.resize(cbNeeded / sizeof(HMODULE));
		DWORD currentSize = static_cast<DWORD>(hMods.size() * sizeof(HMODULE));
		if(EnumProcessModulesEx(GetCurrentProcess(), hMods.data(), currentSize, &cbNeeded, LIST_MODULES_ALL) == 0)
		{
			// EnumProcessModulesEx failed
			return {};
		}
	} while (cbNeeded > static_cast<DWORD>(hMods.size() * sizeof(HMODULE)));

	std::vector<wchar_t> buffer(WIN_MAX_EXTENDED_PATH + 1, 0);
	std::vector<char> u8_buffer(WIN_MAX_EXTENDED_PATH, 0);
	for (size_t i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
	{
		// Get the full path to the module's file.
		DWORD moduleFileNameLen = GetModuleFileNameW(hMods[i], &buffer[0], buffer.size() - 1);
		DWORD lastError = GetLastError();
		if ((moduleFileNameLen == 0) && (lastError != ERROR_SUCCESS))
		{
			// GetModuleFileName failed
//			debug(LOG_ERROR, "GetModuleFileName failed: %lu", moduleFileNameLen);
			continue;
		}
		else if (moduleFileNameLen > (buffer.size() - 1))
		{
//			debug(LOG_ERROR, "GetModuleFileName returned a length: %lu >= buffer length: %zu", moduleFileNameLen, buffer.size());
			continue;
		}

		// Because Windows XP's GetModuleFileName does not guarantee null-termination,
		// always append a null-terminator
		buffer[moduleFileNameLen] = 0;

		if (!win_Utf16toUtf8(buffer.data(), u8_buffer))
		{
//			debug(LOG_ERROR, "Encoding conversion error.");
			continue;
		}
		if (!filenamesOnly)
		{
			processModules.push_back(std::string(u8_buffer.data()));
			continue;
		}
		// otherwise, get the filename only
		size_t u8BufferLength = strlen(u8_buffer.data());
		if (u8BufferLength == 0)
		{
			continue;
		}
		std::string filename;
		for (size_t idx = u8BufferLength - 1; idx > 0; idx--)
		{
			if (u8_buffer[idx] == '\\' && (idx < (u8BufferLength - 1)))
			{
				filename = std::string(&u8_buffer[idx + 1]);
				break;
			}
		}
		if (filename.empty())
		{
			filename = std::string(u8_buffer.data());
		}

		processModules.push_back(std::move(filename));
	}

	return processModules;
}
#endif // defined(WZ_OS_WIN)

#if defined(WZ_OS_WIN)

std::unordered_map<std::string, std::string> GetEnvironmentVariables()
{
	std::unordered_map<std::string, std::string> results;
	auto free_envStringsW = [](wchar_t* p) { if (p) { FreeEnvironmentStringsW(p); } };
	auto envStringsW = std::unique_ptr<wchar_t, decltype(free_envStringsW)>{GetEnvironmentStringsW(), free_envStringsW};
	if (!envStringsW)
	{
		return {};
	}
	std::vector<char> u8_buffer(WIN_MAX_EXTENDED_PATH, 0);
	const wchar_t* pEnvStart = envStringsW.get();
	while (*pEnvStart != '\0')
	{
		size_t envEntryLen = wcslen(pEnvStart);
		// convert UTF-16 to UTF-8
		if (win_Utf16toUtf8(pEnvStart, u8_buffer))
		{
			const char* pEnvStr = u8_buffer.data();
			const char* separatorPos = strchr(pEnvStr, '=');
			if (separatorPos)
			{
				std::string envVarName = std::string(pEnvStr, separatorPos - pEnvStr);
				std::string envVarValue = std::string(++separatorPos);
				if (!envVarName.empty())
				{
					results[envVarName] = envVarValue;
				}
			}
		}
		pEnvStart += envEntryLen + 1;
	}
	return results;
}

#elif defined(WZ_OS_UNIX)

# include <unistd.h>
# if !defined(HAVE_ENVIRON_DECL)
  extern char **environ;
# endif

std::unordered_map<std::string, std::string> GetEnvironmentVariables()
{
	std::unordered_map<std::string, std::string> results;
	if (!environ)
	{
		return {};
	}
	for (char ** env = environ; *env; ++env)
	{
		const char* separatorPos = strchr(*env, '=');
		if (!separatorPos) { continue; }
		std::string envVarName = std::string(*env, separatorPos - *env);
		std::string envVarValue = std::string(++separatorPos);
		if (envVarName.empty()) { continue; }
		results[envVarName] = envVarValue;
	}
	return results;
}

#else

std::unordered_map<std::string, std::string> GetEnvironmentVariables()
{
	// not implemented
	return {};
}

#endif


std::string GetEnvironmentVariableNames()
{
	auto envVariables = GetEnvironmentVariables();
	const std::string separator = "="; // "=" should not occur in an env var name on Windows, Linux, etc
	std::string result;
	for (const auto& it : envVariables)
	{
		if (it.first.empty())
		{
			continue;
		}
		if (!result.empty())
		{
			result += separator;
		}
		result += it.first;
	}
	return result;
}

std::string EnvironmentPropertyProvider::GetCurrentEnvironmentPropertyValue(const EnvironmentProperty& property)
{
	using EP = EnvironmentProperty;
	switch (property)
	{
		case EP::FIRST_LAUNCH:
		{
			if (firstLaunchDateStr.empty())
			{
				auto record = ActivityManager::instance().getRecord();
				if (record)
				{
					firstLaunchDateStr = record->getFirstLaunchDate();
				}
			}
			return firstLaunchDateStr;
		}
		case EP::INSTALLED_PATH:
			return LaunchInfo::getCurrentProcessDetails().imageFileName.dirname();
		case EP::WIN_INSTALLED_BINARIES:
		{
#if defined(WZ_OS_WIN)
			if (binDirFilesStr.empty())
			{
				auto binDirFiles = Get_WinInstalledBinaries();
				for (const auto& fileName : binDirFiles)
				{
					if (!binDirFilesStr.empty()) binDirFilesStr += ";";
					binDirFilesStr += "\"";
					binDirFilesStr += WzString::fromUtf8(fileName).replace("\"", "\\\"").toStdString();
					binDirFilesStr += "\"";
				}
			}
#endif
			return binDirFilesStr;
		}
		case EP::WIN_LOADEDMODULES:
		{
#if defined(WZ_OS_WIN)
			if (processModulesStr.empty())
			{
				auto processModules = Get_WinProcessModules(false);
				for (const auto& modulePath : processModules)
				{
					if (!processModulesStr.empty()) processModulesStr += ";";
					processModulesStr += "\"";
					processModulesStr += WzString::fromUtf8(modulePath).replace("\"", "\\\"").toStdString();
					processModulesStr += "\"";
				}
			}
#endif
			return processModulesStr;
		}
		case EP::WIN_LOADEDMODULENAMES:
		{
#if defined(WZ_OS_WIN)
			if (processModuleNamesStr.empty())
			{
				auto processModuleNames = Get_WinProcessModules(true);
				for (const auto& fileName : processModuleNames)
				{
					if (!processModuleNamesStr.empty()) processModuleNamesStr += ";";
					processModuleNamesStr += "\"";
					processModuleNamesStr += WzString::fromUtf8(fileName).replace("\"", "\\\"").toStdString();
					processModuleNamesStr += "\"";
				}
			}
#endif
			return processModuleNamesStr;
		}
		case EP::ENV_VAR_NAMES:
			return GetEnvironmentVariableNames();
		case EP::SYSTEM_RAM:
			return std::to_string(wzGetCurrentSystemRAM());
	}
	return ""; // silence warning
}

EnvironmentPropertyProvider::~EnvironmentPropertyProvider() { }
bool EnvironmentPropertyProvider::getPropertyValue(const std::string& property, std::string& output_value)
{
	auto it = strToEnvironmentPropertyMap.find(property);
	if (it == strToEnvironmentPropertyMap.end())
	{
		return false;
	}
	output_value = GetCurrentEnvironmentPropertyValue(it->second);
	return true;
}

// MARK: - CombinedPropertyProvider

CombinedPropertyProvider::~CombinedPropertyProvider() { }
bool CombinedPropertyProvider::getPropertyValue(const std::string& property, std::string& output_value)
{
	for (auto& provider : propertyProviders)
	{
		if (provider->getPropertyValue(property, output_value))
		{
			return true;
		}
	}
	return false;
}

