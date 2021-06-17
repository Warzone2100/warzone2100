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

#include <LaunchInfo.h>

#ifndef PACKAGE_DISTRIBUTOR
# define PACKAGE_DISTRIBUTOR "UNKNOWN"
#endif

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
			return LaunchInfo::getParentImageName();
		case BP::ANCESTORS:
		{
			auto ancestors = LaunchInfo::getAncestorProcesses();
			std::string ancestorsStr;
			for (const auto& ancestor : ancestors)
			{
				if (!ancestorsStr.empty()) ancestorsStr += ";";
				ancestorsStr += "\"";
				ancestorsStr += WzString::fromUtf8(ancestor.imageFileName).replace("\"", "\\\"").toStdString();
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
