/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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

#include "urlhelpers.h"
#include "lib/framework/frame.h"

#if defined(WZ_OS_WIN)
# include <shellapi.h> /* For ShellExecuteEx  */
# include <shlwapi.h> /* For UrlCanonicalize */
# include <objbase.h> /* For CoInitializeEx */
# include <ntverp.h> /* Windows SDK - include for access to VER_PRODUCTBUILD */
# if VER_PRODUCTBUILD >= 9600
	// 9600 is the Windows SDK 8.1
	# include <VersionHelpers.h> /* For IsWindows7OrGreater() */
# else
	// Earlier SDKs may not have VersionHelpers.h - use simple fallback
	inline bool IsWindows7OrGreater()
	{
		DWORD dwVersion = GetVersion();
		DWORD dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
		DWORD dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
		return (dwMajorVersion > 6) || (dwMajorVersion == 6 && dwMinorVersion >= 1);
	}
# endif
# include <system_error>
#endif

#if defined(WZ_OS_MAC)
#  include "lib/framework/cocoa_wrapper.h" /* For cocoaOpenURL */
#endif // WZ_OS_MAC

#if defined(HAVE_POSIX_SPAWN) || defined(HAVE_POSIX_SPAWNP)
# include <spawn.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <unistd.h>
# if !defined(HAVE_ENVIRON_DECL)
  extern char **environ;
# endif
#endif

#include <vector>

static const std::vector<std::string> validLinkPrefixes = { "http://", "https://" };

bool urlHasHTTPorHTTPSPrefix(char const *url)
{
	// Verify URL starts with http:// or https://
	bool bValidLinkPrefix = false;
	for (const auto& validLinkPrefix : validLinkPrefixes)
	{
		if (strncasecmp(url, validLinkPrefix.c_str(), validLinkPrefix.size()) == 0)
		{
			bValidLinkPrefix = true;
			break;
		}
	}
	return bValidLinkPrefix;
}

#if defined(WZ_OS_WIN)
bool win_utf8ToUtf16(const char* str, std::vector<wchar_t>& outputWStr)
{
	int wstr_len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	if (wstr_len <= 0)
	{
		DWORD dwError = GetLastError();
		debug(LOG_ERROR, "Could not not convert string from UTF-8; MultiByteToWideChar failed with error %lu: %s\n", dwError, str);
		return false;
	}
	outputWStr = std::vector<wchar_t>(wstr_len, L'\0');
	if (MultiByteToWideChar(CP_UTF8, 0, str, -1, &outputWStr[0], wstr_len) == 0)
	{
		DWORD dwError = GetLastError();
		debug(LOG_ERROR, "Could not not convert string from UTF-8; MultiByteToWideChar[2] failed with error %lu: %s\n", dwError, str);
		return false;
	}
	return true;
}
#endif

#if !defined(WZ_OS_WIN) && !defined(WZ_OS_MAC)
bool xdg_open(const char *url)
{
// for linux
# if defined(HAVE_POSIX_SPAWNP)
	char progName[] = "xdg-open";
	char *urlCopy = strdup(url);
	char *argv[3] = { progName, urlCopy, nullptr };
	pid_t pid = 0;
	int spawnResult = posix_spawnp(&pid, progName, nullptr, nullptr, argv, environ);
	if (spawnResult == 0)
	{
		waitpid(pid, nullptr, 0);
	}
	free(urlCopy);
	return spawnResult == 0;
# else
	char lbuf[250] = {'\0'};
	ssprintf(lbuf, "xdg-open %s &", url);
	int stupidWarning = system(lbuf);
	(void)stupidWarning;  // Why is system() a warn_unused_result function..?
	return true;
# endif
}
#endif

bool openURLInBrowser(char const *url)
{
	if (!url || !*url) return false;

	// Verify URL starts with http:// or https://
	if (!urlHasHTTPorHTTPSPrefix(url))
	{
		debug(LOG_ERROR, "Rejecting attempt to open link with URL: %s", url);
		return false;
	}

	//FIXME: There is no decent way we can re-init the display to switch to window or fullscreen within game. refs: screenToggleMode().

#if defined(WZ_OS_WIN)
	std::vector<wchar_t> wUrl;
	if (!win_utf8ToUtf16(url, wUrl))
	{
		return false;
	}

	#define _INTERNET_MAX_URL_LENGTH 2083
	if (wUrl.size() > _INTERNET_MAX_URL_LENGTH)
	{
		debug(LOG_ERROR, "URL length (%zu) exceeds maximum supported length: %s\n", wUrl.size(), url);
		return false;
	}
	std::vector<wchar_t> wUrlCanonicalized(_INTERNET_MAX_URL_LENGTH + 1, L'\0');
	DWORD canonicalizedLength = wUrlCanonicalized.size();
	DWORD dwFlags = URL_UNESCAPE | URL_ESCAPE_UNSAFE;
	#ifndef URL_ESCAPE_AS_UTF8
	# define URL_ESCAPE_AS_UTF8 0x00040000
	#endif
	if (IsWindows7OrGreater())
	{
		dwFlags |= URL_ESCAPE_AS_UTF8;
	}
	HRESULT hr = ::UrlCanonicalizeW(wUrl.data(), wUrlCanonicalized.data(), &canonicalizedLength, dwFlags);
	if (!SUCCEEDED(hr))
	{
		// Failed to canonicalize URL
		std::string errorMessage = std::system_category().message(hr);
		debug(LOG_ERROR, "Failed to canonicalize URL ('%s') with error: %s", url, errorMessage.c_str());
		return false;
	}
	HRESULT coInitResult = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	SHELLEXECUTEINFOW ShExecInfo = {};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
	ShExecInfo.fMask = SEE_MASK_NOASYNC;
	ShExecInfo.lpVerb = L"open";
	ShExecInfo.lpFile = wUrlCanonicalized.data();
	ShExecInfo.nShow = SW_SHOWNORMAL;
	bool bShellExecuteFailure = false;
	if (!::ShellExecuteExW(&ShExecInfo))
	{
		DWORD dwError = ::GetLastError();
		debug(LOG_ERROR, "ShellExecuteEx failed with error: %lu", dwError);
		bShellExecuteFailure = true;
	}
	if (coInitResult == S_OK || coInitResult == S_FALSE)
	{
		::CoUninitialize();
	}
	return !bShellExecuteFailure;
#elif defined (WZ_OS_MAC)
	return cocoaOpenURL(url);
#else
	// for linux
	return xdg_open(url);
#endif
}

#include <curl/curl.h>

std::string urlEncode(const char* urlFragment)
{
# if LIBCURL_VERSION_NUM >= 0x070F04	// cURL 7.15.4+
	CURL *curl = curl_easy_init();
	if (!curl)
	{
		debug(LOG_ERROR, "curl_easy_init failed");
		return "";
	}
	char *urlEscaped = curl_easy_escape(curl, urlFragment, 0);
	if (!urlEscaped)
	{
		debug(LOG_ERROR, "curl_easy_escape failed");
		return "";
	}
# else
	char *urlEscaped = curl_escape(urlFragment, 0);
	if (!urlEscaped)
	{
		debug(LOG_ERROR, "curl_escape failed");
		return "";
	}
# endif
	std::string result = urlEscaped;
	curl_free(urlEscaped);
# if LIBCURL_VERSION_NUM >= 0x070F04	// cURL 7.15.4+
	curl_easy_cleanup(curl);
# endif
	return result;
}

bool openFolderInDefaultFileManager(const char* path)
{
#if defined(WZ_OS_WIN)
	std::vector<wchar_t> wPath;
	if (!win_utf8ToUtf16(path, wPath))
	{
		return false;
	}

	HRESULT coInitResult = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	SHELLEXECUTEINFOW ShExecInfo = {};
	ShExecInfo.cbSize = sizeof(ShExecInfo);
	ShExecInfo.fMask = SEE_MASK_NOASYNC;
	ShExecInfo.hwnd = nullptr;
	ShExecInfo.lpVerb = L"explore";
	ShExecInfo.lpFile = wPath.data();
	ShExecInfo.nShow = SW_SHOW;
	bool bShellExecuteFailure = false;
	if (!::ShellExecuteExW(&ShExecInfo))
	{
		DWORD dwError = ::GetLastError();
		debug(LOG_ERROR, "ShellExecuteEx failed with error: %lu", dwError);
		bShellExecuteFailure = true;
	}
	if (coInitResult == S_OK || coInitResult == S_FALSE)
	{
		::CoUninitialize();
	}
	return !bShellExecuteFailure;
#elif defined (WZ_OS_MAC)
	return cocoaSelectFolderInFinder(path);
#else
	return xdg_open(path);
#endif
	return false;
}
