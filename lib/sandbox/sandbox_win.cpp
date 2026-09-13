// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

/**
 * The Windows backend.
 *
 * harden() only.
 */

#include "sandbox_internal.h"

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

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

typedef BOOL (WINAPI *SetDefaultDllDirectoriesFunction)(
  DWORD DirectoryFlags
);
#if !defined(LOAD_LIBRARY_SEARCH_APPLICATION_DIR)
# define LOAD_LIBRARY_SEARCH_APPLICATION_DIR	0x00000200
#endif
#if !defined(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)
# define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS		0x00001000
#endif
#if !defined(LOAD_LIBRARY_SEARCH_SYSTEM32)
# define LOAD_LIBRARY_SEARCH_SYSTEM32			0x00000800
#endif

typedef BOOL (WINAPI *SetDllDirectoryWFunction)(
  LPCWSTR lpPathName
);

typedef BOOL (WINAPI *SetSearchPathModeFunction)(
  DWORD Flags
);
#if !defined(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE)
# define BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE	0x00000001
#endif
#if !defined(BASE_SEARCH_PATH_PERMANENT)
# define BASE_SEARCH_PATH_PERMANENT					0x00008000
#endif

typedef BOOL (WINAPI *SetProcessDEPPolicyFunction)(
  DWORD dwFlags
);
#if !defined(PROCESS_DEP_ENABLE)
# define PROCESS_DEP_ENABLE		0x00000001
#endif

namespace wz::sandbox
{

void backendHarden()
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	HMODULE hKernel32 = GetModuleHandleW(L"kernel32");

	SetDefaultDllDirectoriesFunction _SetDefaultDllDirectories = reinterpret_cast<SetDefaultDllDirectoriesFunction>(reinterpret_cast<void*>(GetProcAddress(hKernel32, "SetDefaultDllDirectories")));

	if (_SetDefaultDllDirectories)
	{
		// Use SetDefaultDllDirectories to limit directories to search
		_SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
	}

	SetDllDirectoryWFunction _SetDllDirectoryW = reinterpret_cast<SetDllDirectoryWFunction>(reinterpret_cast<void*>(GetProcAddress(hKernel32, "SetDllDirectoryW")));

	if (_SetDllDirectoryW)
	{
		// Remove the current working directory from the default DLL search order
		_SetDllDirectoryW(L"");
	}

	SetSearchPathModeFunction _SetSearchPathMode = reinterpret_cast<SetSearchPathModeFunction>(reinterpret_cast<void*>(GetProcAddress(hKernel32, "SetSearchPathMode")));
	if (_SetSearchPathMode)
	{
		// Enable safe search mode for the process
		_SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT);
	}
#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */

    // Enable heap terminate-on-corruption.
    // A correct application can continue to run even if this call fails,
    // so it is safe to ignore the return value and call the function as follows:
    // (void)HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    //
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	typedef BOOL (WINAPI *HSI)
		   (HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);
	HSI pHsi = reinterpret_cast<HSI>(reinterpret_cast<void*>(GetProcAddress(hKernel32, "HeapSetInformation")));
	if (pHsi)
	{
		#ifndef HeapEnableTerminationOnCorruption
		#   define HeapEnableTerminationOnCorruption (HEAP_INFORMATION_CLASS)1
		#endif

		(void)((pHsi)(NULL, HeapEnableTerminationOnCorruption, NULL, 0));
	}
#else
	(void)HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	SetProcessDEPPolicyFunction _SetProcessDEPPolicy = reinterpret_cast<SetProcessDEPPolicyFunction>(reinterpret_cast<void*>(GetProcAddress(hKernel32, "SetProcessDEPPolicy")));
	if (_SetProcessDEPPolicy)
	{
		// Ensure DEP is enabled
		_SetProcessDEPPolicy(PROCESS_DEP_ENABLE);
	}
#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
}

Status backendConfine(const Plan& plan, Tier requested)
{
	(void)plan;
	(void)requested;

	Status result;
	result.detail = "not implemented on Windows";
	return result;
}

} // namespace wz::sandbox
