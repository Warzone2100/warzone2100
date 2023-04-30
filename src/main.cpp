/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
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
/** @file
 *  The main file that launches the game and starts up everything.
 */

// Get platform defines before checking for them!
#include "lib/framework/wzapp.h"

#if defined(WZ_OS_WIN)
#  if defined( _MSC_VER )
	// Silence warning when using MSVC + the Windows 7 SDK (required for XP compatibility)
	//	warning C4091: 'typedef ': ignored on left of 'tagGPFIDL_FLAGS' when no variable is declared
	#pragma warning( push )
	#pragma warning( disable : 4091 )
#  endif
#  include <shlobj.h> /* For SHGetFolderPath */
#  if defined( _MSC_VER )
	#pragma warning( pop )
#  endif
#  include <shellapi.h> /* CommandLineToArgvW */

#  include <ntverp.h>				// Windows SDK - include for access to VER_PRODUCTBUILD
#  if VER_PRODUCTBUILD >= 9200
	// 9200 is the Windows SDK 8.0 (which introduced family support)
	#include <winapifamily.h>	// Windows SDK
#  else
	// Earlier SDKs don't have the concept of families - provide simple implementation
	// that treats everything as "desktop"
	#if !defined(WINAPI_PARTITION_DESKTOP)
		#define WINAPI_PARTITION_DESKTOP			0x00000001
	#endif
	#if !defined(WINAPI_FAMILY_PARTITION)
		#define WINAPI_FAMILY_PARTITION(Partition)	((WINAPI_PARTITION_DESKTOP & Partition) == Partition)
	#endif
#  endif
#elif defined(WZ_OS_UNIX)
#  include <errno.h>
#endif // WZ_OS_WIN

#include "lib/framework/input.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/wzpaths.h"
#include "lib/framework/wztime.h"
#include "lib/exceptionhandler/exceptionhandler.h"
#include "lib/exceptionhandler/dumpinfo.h"

#include "lib/sound/playlist.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"
#include "lib/netplay/netreplay.h"
#include "lib/sound/audio.h"
#include "lib/sound/cdaudio.h"

#include "clparse.h"
#include "challenge.h"
#include "configuration.h"
#include "display.h"
#include "display3d.h"
#include "frontend.h"
#include "game.h"
#include "init.h"
#include "levels.h"
#include "lighting.h"
#include "loadsave.h"
#include "loop.h"
#include "mission.h"
#include "modding.h"
#include "multiplay.h"
#include "notifications.h"
#include "qtscript.h"
#include "research.h"
#include "seqdisp.h"
#include "warzoneconfig.h"
#include "main.h"
#include "wrappers.h"
#include "version.h"
#include "map.h"
#include "keybind.h"
#include "random.h"
#include "urlhelpers.h"
#include "urlrequest.h"
#include <time.h>
#include <LaunchInfo.h>
#include <sodium.h>
#include "updatemanager.h"
#include "activity.h"
#include "stdinreader.h"
#if defined(ENABLE_DISCORD)
#include "integrations/wzdiscordrpc.h"
#endif
#include "wzcrashhandlingproviders.h"
#include "wzpropertyproviders.h"
#include "3rdparty/gsl_finally.h"

#if defined(WZ_OS_UNIX)
# include <signal.h>
# include <time.h>
#endif

#if defined(WZ_OS_MAC)
# include <unistd.h>
# include "lib/framework/mac_wrapper.h"
# include "lib/framework/cocoa_wrapper.h"
#endif // WZ_OS_MAC

/* Always use fallbacks on Windows */
#if defined(WZ_OS_WIN)
#  undef WZ_DATADIR
#endif

#if !defined(WZ_DATADIR)
#  define WZ_DATADIR "data"
#endif


enum FOCUS_STATE
{
	FOCUS_OUT,		// Window does not have the focus
	FOCUS_IN,		// Window has got the focus
};

bool customDebugfile = false;		// Default false: user has NOT specified where to store the stdout/err file.

char datadir[PATH_MAX] = ""; // Global that src/clparse.c:ParseCommandLine can write to, so it can override the default datadir on runtime. Needs to be empty on startup for ParseCommandLine to work!
char configdir[PATH_MAX] = ""; // specifies custom USER directory. Same rules apply as datadir above.
char rulesettag[40] = "";

//flag to indicate when initialisation is complete
const char* SAVEGAME_CAM = "savegames/campaign";
const char* SAVEGAME_CAM_AUTO = "savegames/campaign/auto";
const char* SAVEGAME_SKI = "savegames/skirmish";
const char* SAVEGAME_SKI_AUTO = "savegames/skirmish/auto";

const char *SaveGameLocToPath[] = {
	SAVEGAME_CAM,
	SAVEGAME_CAM_AUTO,
	SAVEGAME_SKI,
	SAVEGAME_SKI_AUTO,
};

std::string SaveGamePath_t::toPath(SaveGamePath_t::Extension ext)
{
	std::string out;
	switch (ext)
	{
	case SaveGamePath_t::Extension::GAM:
		out = std::string(SaveGameLocToPath[loc]) + "/" + gameName + ".gam";
		break;
	case SaveGamePath_t::Extension::JSON:
		out = std::string(SaveGameLocToPath[loc]) + "/" + gameName + ".json";
		break;
	};
	return out;
}

bool	gameInitialised = false;
char	SaveGamePath[PATH_MAX];
char    ReplayPath[PATH_MAX];
char	ScreenDumpPath[PATH_MAX];
char	MultiCustomMapsPath[PATH_MAX];
char	MultiPlayersPath[PATH_MAX];
char	KeyMapPath[PATH_MAX];
char	FavoriteStructuresPath[PATH_MAX];
// Start game in title mode:
static GS_GAMEMODE gameStatus = GS_TITLE_SCREEN;
// Status of the gameloop
static GAMECODE gameLoopStatus = GAMECODE_CONTINUE;
static FOCUS_STATE focusState = FOCUS_IN;

#if defined(WZ_OS_UNIX)
static bool ignoredSIGPIPE = false;
#endif


#if defined(WZ_OS_WIN)

#define WIN_MAX_EXTENDED_PATH 32767

// Gets the full path to the application executable (UTF-16)
static std::wstring getCurrentApplicationPath_WIN()
{
	// On Windows, use GetModuleFileNameW to obtain the full path to the current EXE

	std::vector<wchar_t> buffer(WIN_MAX_EXTENDED_PATH + 1, 0);
	DWORD moduleFileNameLen = GetModuleFileNameW(NULL, &buffer[0], buffer.size() - 1);
	DWORD lastError = GetLastError();
	if ((moduleFileNameLen == 0) && (lastError != ERROR_SUCCESS))
	{
		// GetModuleFileName failed
		debug(LOG_ERROR, "GetModuleFileName failed: %lu", moduleFileNameLen);
		return std::wstring();
	}
	else if (moduleFileNameLen > (buffer.size() - 1))
	{
		debug(LOG_ERROR, "GetModuleFileName returned a length: %lu >= buffer length: %zu", moduleFileNameLen, buffer.size());
		return std::wstring();
	}

	// Because Windows XP's GetModuleFileName does not guarantee null-termination,
	// always append a null-terminator
	buffer[moduleFileNameLen] = 0;

	return std::wstring(buffer.data());
}

// Gets the full path to the folder that contains the application executable (UTF-16)
static std::wstring getCurrentApplicationFolder_WIN()
{
	std::wstring applicationExecutablePath = getCurrentApplicationPath_WIN();
	if (applicationExecutablePath.empty())
	{
		return std::wstring();
	}

	// Find the position of the last slash in the application executable path
	size_t lastSlash = applicationExecutablePath.find_last_of(L"\\/", std::wstring::npos);
	if (lastSlash == std::wstring::npos)
	{
		// Did not find a path separator - does not appear to be a valid application executable path?
		debug(LOG_ERROR, "Unable to find path separator in application executable path");
		return std::wstring();
	}

	// Trim off the executable name
	return applicationExecutablePath.substr(0, lastSlash);
}
#endif

// Gets the full path to the folder that contains the application executable (UTF-8)
static std::string getCurrentApplicationFolder()
{
#if defined(WZ_OS_WIN)
	std::wstring applicationFolderPath_utf16 = getCurrentApplicationFolder_WIN();

	// Convert the UTF-16 to UTF-8
	int outputLength = WideCharToMultiByte(CP_UTF8, 0, applicationFolderPath_utf16.c_str(), -1, NULL, 0, NULL, NULL);
	if (outputLength <= 0)
	{
		debug(LOG_ERROR, "Encoding conversion error.");
		return std::string();
	}
	std::vector<char> u8_buffer(outputLength, 0);
	if (WideCharToMultiByte(CP_UTF8, 0, applicationFolderPath_utf16.c_str(), -1, &u8_buffer[0], outputLength, NULL, NULL) == 0)
	{
		debug(LOG_ERROR, "Encoding conversion error.");
		return std::string();
	}

	return std::string(u8_buffer.data());
#else
	// Not yet implemented for this platform
	return std::string();
#endif
}

// Gets the full path to the prefix of the folder that contains the application executable (UTF-8)
static std::string getCurrentApplicationFolderPrefix()
{
	// Remove the last path component
	std::string appPath = getCurrentApplicationFolder();
	if (appPath.empty())
	{
		return appPath;
	}

	// Remove trailing path separators (slashes)
	while (!appPath.empty() && (appPath.back() == '\\' || appPath.back() == '/'))
	{
		appPath.pop_back();
	}

	// Find the position of the last slash in the application folder
	size_t lastSlash = appPath.find_last_of("\\/", std::string::npos);
	if (lastSlash == std::string::npos)
	{
		// Did not find a path separator - does not appear to be a valid app folder?
		debug(LOG_ERROR, "Unable to find path separator in application executable path");
		return std::string();
	}

	// Trim off the last path component
	return appPath.substr(0, lastSlash);
}

static bool isPortableMode()
{
	static bool _checkedMode = false;
	static bool _isPortableMode = false;
	if (!_checkedMode)
	{
#if defined(WZ_OS_WIN)
		// On Windows, check for the presence of a ".portable" file in the same directory as the application EXE
		std::wstring portableFilePath = getCurrentApplicationFolder_WIN();
		portableFilePath += L"\\.portable";

		if (GetFileAttributesW(portableFilePath.c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			// A .portable file exists in the application directory
			debug(LOG_WARNING, ".portable file detected - enabling portable mode");
			_isPortableMode = true;
		}
#else
		// Not yet implemented for this platform.
#endif
		_checkedMode = true;
	}
	return _isPortableMode;
}

/*!
 * Retrieves the current working directory and copies it into the provided output buffer
 * \param[out] dest the output buffer to put the current working directory in
 * \param size the size (in bytes) of \c dest
 * \return true on success, false if an error occurred (and dest doesn't contain a valid directory)
 */
#if !defined(WZ_PHYSFS_2_1_OR_GREATER)
static bool getCurrentDir(char *const dest, size_t const size)
{
#if defined(WZ_OS_UNIX)
	if (getcwd(dest, size) == nullptr)
	{
		if (errno == ERANGE)
		{
			debug(LOG_ERROR, "The buffer to contain our current directory is too small (%u bytes and more needed)", (unsigned int)size);
		}
		else
		{
			debug(LOG_ERROR, "getcwd failed: %s", strerror(errno));
		}

		return false;
	}
#elif defined(WZ_OS_WIN)
	wchar_t tmpWStr[PATH_MAX];
	const int len = GetCurrentDirectoryW(PATH_MAX, tmpWStr);

	if (len == 0)
	{
		// Retrieve Windows' error number
		const DWORD err = GetLastError();
		char *err_string = NULL;

		// Retrieve a string describing the error number (uses LocalAlloc() to allocate memory for err_string)
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, (char *)&err_string, 0, NULL);

		// Print an error message with the above description
		debug(LOG_ERROR, "GetCurrentDirectory failed (error code: %lu): %s", err, err_string);

		// Free our chunk of memory FormatMessageA gave us
		LocalFree(err_string);

		return false;
	}
	else if (len > size)
	{
		debug(LOG_ERROR, "The buffer to contain our current directory is too small (%u bytes and %d needed)", (unsigned int)size, len);

		return false;
	}
	if (WideCharToMultiByte(CP_UTF8, 0, tmpWStr, -1, dest, size, NULL, NULL) == 0)
	{
		dest[0] = '\0';
		debug(LOG_ERROR, "Encoding conversion error.");
		return false;
	}
#else
# error "Provide an implementation here to copy the current working directory in 'dest', which is 'size' bytes large."
#endif

	// If we got here everything went well
	return true;
}
#endif

#if defined(WZ_OS_WIN)
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) || !defined(WZ_PHYSFS_2_1_OR_GREATER)
static bool win_wcharConvToUtf8(wchar_t *pwStr, std::string &outputUtf8)
{
	std::vector<char> utf8Buffer;
	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, pwStr, -1, NULL, 0, NULL, NULL);
	if ( utf8Len <= 0 )
	{
		// Encoding conversion error
		return false;
	}
	utf8Buffer.resize(utf8Len, 0);
	if ( (utf8Len = WideCharToMultiByte(CP_UTF8, 0, pwStr, -1, &utf8Buffer[0], utf8Len, NULL, NULL)) <= 0 )
	{
		// Encoding conversion error
		return false;
	}
	outputUtf8 = std::string(utf8Buffer.data(), utf8Len - 1);
	return true;
}
#endif
#endif


// Fallback method for earlier PhysFS verions that do not support PHYSFS_getPrefDir
// Importantly, this creates the folders if they do not exist
#if !defined(WZ_PHYSFS_2_1_OR_GREATER)
static std::string getPlatformPrefDir_Fallback(const char *org, const char *app)
{
	WzString basePath;
	WzString appendPath;
	char tmpstr[PATH_MAX] = { '\0' };
	const size_t size = sizeof(tmpstr);
#if defined(WZ_OS_WIN)
	wchar_t tmpWStr[MAX_PATH];

	if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, tmpWStr)))
	{
		std::string utf8Path;
		if (!win_wcharConvToUtf8(tmpWStr, utf8Path))
		{
			debug(LOG_FATAL, "Config directory encoding conversion error.");
			exit(1);
		}
		basePath = WzString::fromUtf8(utf8Path);

		appendPath = WzString();
		// Must append org\app to APPDATA path
		appendPath += org;
		appendPath += PHYSFS_getDirSeparator();
		appendPath += app;
	}
	else
#elif defined(WZ_OS_MAC)
	if (cocoaGetApplicationSupportDir(tmpstr, size))
	{
		basePath = WzString::fromUtf8(tmpstr);
		appendPath = WzString::fromUtf8(app);
	}
	else
#elif defined(WZ_OS_UNIX)
	// Following PhysFS, use XDG's base directory spec, even if not on Linux.
	// Reference: https://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
	const char *envPath = getenv("XDG_DATA_HOME");
	if (envPath == nullptr)
	{
		// XDG_DATA_HOME isn't defined
		// Use HOME, and append ".local/share/" to match XDG's base directory spec
		envPath = getenv("HOME");

		if (envPath == nullptr)
		{
			// On PhysFS < 2.1, fall-back to using PHYSFS_getUserDir() if HOME isn't defined
			debug(LOG_INFO, "HOME environment variable isn't defined - falling back to PHYSFS_getUserDir()");
			envPath = PHYSFS_getUserDir();
		}

		appendPath = WzString(".local") + PHYSFS_getDirSeparator() + "share";
	}

	if (envPath != nullptr)
	{
		basePath = WzString::fromUtf8(envPath);

		if (!appendPath.isEmpty())
		{
			appendPath += PHYSFS_getDirSeparator();
		}
		appendPath += app;
	}
	else
#else
	// On PhysFS < 2.1, fall-back to using PHYSFS_getUserDir() for other OSes
	if (PHYSFS_getUserDir())
	{
		basePath = WzString::fromUtf8(PHYSFS_getUserDir());
		appendPath = WzString::fromUtf8(app);
	}
	else
#endif
	if (getCurrentDir(tmpstr, size))
	{
		basePath = WzString::fromUtf8(tmpstr);
		appendPath = WzString::fromUtf8(app);
	}
	else
	{
		debug(LOG_FATAL, "Can't get home / prefs directory?");
		abort();
	}

	// Create the folders within the basePath if they don't exist
	if (!WZ_PHYSFS_createPlatformPrefDir(basePath, appendPath))
	{
		debug(LOG_FATAL, "Failed to create platform config dir: %s/%s",
			  basePath.toUtf8().c_str(), appendPath.toUtf8().c_str());
		exit(1);
	}

	return (basePath + PHYSFS_getDirSeparator() + appendPath + PHYSFS_getDirSeparator()).toUtf8();
}
#endif

// Retrieves the appropriate storage directory for application-created files / prefs
// (Ensures the directory exists. Creates folders if necessary.)
static std::string getPlatformPrefDir(const char * org, const std::string &app)
{
	if (isPortableMode())
	{
		// When isPortableMode is true, the config dir should be stored in the same prefix as the app's bindir.
		// i.e. If the app executable path is:  <prefix>/bin/warzone2100.exe
		//      the config directory should be: <prefix>/<app>/
		std::string prefixPath = getCurrentApplicationFolderPrefix();
		if (prefixPath.empty())
		{
			// Failed to get the current application folder
			debug(LOG_FATAL, "Error getting the current application folder prefix - unable to proceed with portable mode");
			exit(1);
		}

		std::string appendPath = app;

		// Create the folders within the prefixPath if they don't exist
		if (!PHYSFS_setWriteDir(prefixPath.c_str())) // Workaround for PhysFS not creating the writedir as expected.
		{
			debug(LOG_FATAL, "Error setting write directory to \"%s\": %s",
				  prefixPath.c_str(), WZ_PHYSFS_getLastError());
			exit(1);
		}

		if (!PHYSFS_mkdir(appendPath.c_str()))
		{
			debug(LOG_FATAL, "Error creating directory \"%s\" in \"%s\": %s",
				  appendPath.c_str(), PHYSFS_getWriteDir(), WZ_PHYSFS_getLastError());
			exit(1);
		}

		return prefixPath + PHYSFS_getDirSeparator() + appendPath + PHYSFS_getDirSeparator();
	}

#if defined(WZ_OS_WIN)
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	BuildPropertyProvider buildPropProvider;
	std::string win_package_fullname;
	if (buildPropProvider.getPropertyValue("WIN_PACKAGE_FULLNAME", win_package_fullname) && !win_package_fullname.empty())
	{
		// Running as a packaged Windows desktop app - to behave nicely, we should always use the redirected app data folder location
		// (so it can be cleanly uninstalled)
		# if !defined(KF_FLAG_FORCE_APP_DATA_REDIRECTION)
		#  define KF_FLAG_FORCE_APP_DATA_REDIRECTION 0x00080000
		# endif
		wchar_t* appData = nullptr;
		HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE | KF_FLAG_FORCE_APP_DATA_REDIRECTION, NULL, &appData);
		if (result == S_OK)
		{
			std::string utf8Path;
			win_wcharConvToUtf8(appData, utf8Path);
			CoTaskMemFree(appData);
			appData = nullptr;
			WzString basePath = WzString::fromUtf8(utf8Path);

			WzString appendPath = WzString();
			// Must append org\app to APPDATA path
			appendPath += org;
			appendPath += PHYSFS_getDirSeparator();
			appendPath += WzString::fromUtf8(app);

			// Create the folders within the basePath if they don't exist
			if (!WZ_PHYSFS_createPlatformPrefDir(basePath, appendPath))
			{
				debug(LOG_FATAL, "Failed to create platform config dir: %s/%s",
					  basePath.toUtf8().c_str(), appendPath.toUtf8().c_str());
				abort();
			}

			return (basePath + PHYSFS_getDirSeparator() + appendPath + PHYSFS_getDirSeparator()).toUtf8();
		}
		else
		{
			// log the failure
			debug(LOG_INFO, "Unable to obtain the new AppModel paths - defaulting to the old method");
			CoTaskMemFree(appData);
			appData = nullptr;
			// proceed to the default method
		}
	}
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#endif // defined(WZ_OS_WIN)

#if defined(WZ_PHYSFS_2_1_OR_GREATER)
	const char * prefsDir = PHYSFS_getPrefDir(org, app.c_str());
	if (prefsDir == nullptr)
	{
		debug(LOG_FATAL, "Failed to obtain prefs directory: %s", WZ_PHYSFS_getLastError());
		exit(1);
	}
	return std::string(prefsDir) + PHYSFS_getDirSeparator();
#else
	// PHYSFS_getPrefDir is not available - use fallback method (which requires OS-specific code)
	std::string prefDir = getPlatformPrefDir_Fallback(org, app.c_str());
	if (prefDir.empty())
	{
		debug(LOG_FATAL, "Failed to obtain prefs directory (fallback)");
		exit(1);
	}
	return prefDir;
#endif // defined(WZ_PHYSFS_2_1_OR_GREATER)
}

bool endsWith (std::string const &fullString, std::string const &endString) {
	if (fullString.length() >= endString.length()) {
		return (0 == fullString.compare (fullString.length() - endString.length(), endString.length(), endString));
	} else {
		return false;
	}
}

static std::string getWzPlatformPrefDir()
{
	return getPlatformPrefDir("Warzone 2100 Project", version_getVersionedAppDirFolderName());
}

static void initialize_ConfigDir()
{
	std::string configDir;

	if (strlen(configdir) == 0)
	{
		configDir = getWzPlatformPrefDir();
	}
	else
	{
		configDir = std::string(configdir);

		// Make sure that we have a directory separator at the end of the string
		if (!endsWith(configDir, PHYSFS_getDirSeparator()))
		{
			configDir += PHYSFS_getDirSeparator();
		}

		debug(LOG_WZ, "Using custom configuration directory: %s", configDir.c_str());
	}

	if (!PHYSFS_setWriteDir(configDir.c_str())) // Workaround for PhysFS not creating the writedir as expected.
	{
		debug(LOG_FATAL, "Error setting write directory to \"%s\": %s",
			  configDir.c_str(), WZ_PHYSFS_getLastError());
		exit(1);
	}

	if (!OverrideRPTDirectory(configDir.c_str()))
	{
		// since it failed, we just use our default path, and not the user supplied one.
		debug(LOG_ERROR, "Error setting exception handler to use directory %s", configDir.c_str());
	}


	// Config dir first so we always see what we write
	PHYSFS_mount(PHYSFS_getWriteDir(), NULL, PHYSFS_PREPEND);

	// Do not follow symlinks *inside* search paths / archives
	PHYSFS_permitSymbolicLinks(0);

	debug(LOG_WZ, "Write dir: %s", PHYSFS_getWriteDir());
	debug(LOG_WZ, "Base dir: %s", PHYSFS_getBaseDir());
}


/*!
 * Initialize the PhysicsFS library.
 */
static void initialize_PhysicsFS(const char *argv_0)
{
	int result = PHYSFS_init(argv_0);

	if (!result)
	{
		debug(LOG_FATAL, "There was a problem trying to init Physfs.  Error was %s", WZ_PHYSFS_getLastError());
		exit(-1);
	}
}

static void check_Physfs()
{
	const PHYSFS_ArchiveInfo **i;
	bool zipfound = false;
	PHYSFS_Version compiled;
	PHYSFS_Version linked;

	PHYSFS_VERSION(&compiled);
	PHYSFS_getLinkedVersion(&linked);

	debug(LOG_WZ, "Compiled against PhysFS version: %d.%d.%d",
	      compiled.major, compiled.minor, compiled.patch);
	debug(LOG_WZ, "Linked against PhysFS version: %d.%d.%d",
	      linked.major, linked.minor, linked.patch);
	if (linked.major < 2)
	{
		debug(LOG_FATAL, "At least version 2 of PhysicsFS required!");
		exit(-1);
	}
	if (linked.major == 2 && linked.minor == 0 && linked.patch == 2)
	{
		debug(LOG_ERROR, "You have PhysicsFS 2.0.2, which is buggy. You may experience random errors/crashes due to spuriously missing files.");
		debug(LOG_ERROR, "Please upgrade/downgrade PhysicsFS to a different version, such as 2.0.3 or 2.0.1.");
	}

	for (i = PHYSFS_supportedArchiveTypes(); *i != nullptr; i++)
	{
		debug(LOG_WZ, "[**] Supported archive(s): [%s], which is [%s].", (*i)->extension, (*i)->description);
		if (!strncasecmp("zip", (*i)->extension, 3) && !zipfound)
		{
			zipfound = true;
		}
	}
	if (!zipfound)
	{
		debug(LOG_FATAL, "Your Physfs wasn't compiled with zip support.  Please recompile Physfs with zip support.  Exiting program.");
		exit(-1);
	}
}

/*!
 * \brief Adds default data dirs
 *
 * Priority:
 * Lower loads first. Current:
 * --datadir > User's home dir > source tree data > AutoPackage > BaseDir > DEFAULT_DATADIR
 *
 * Only --datadir and home dir are always examined. Others only if data still not found.
 *
 * We need ParseCommandLine, before we can add any mods...
 *
 * \sa rebuildSearchPath
 */
static void scanDataDirs()
{
#if !defined(WZ_OS_MAC)
	// Check PREFIX-based paths
	std::string tmpstr;

	// Find out which PREFIX we are in...
	std::string prefix = getWZInstallPrefix();
	std::string dirSeparator(PHYSFS_getDirSeparator());

	if (!PHYSFS_exists("gamedesc.lev"))
	{
		// Data in source tree (<prefix>/data/)
		tmpstr = prefix + dirSeparator + "data" + dirSeparator;
		registerSearchPath(tmpstr, 3);
		rebuildSearchPath(mod_multiplay, true);

		if (!PHYSFS_exists("gamedesc.lev"))
		{
			// Guessed fallback default datadir on Unix
			std::string wzDataDir = WZ_DATADIR;
			if(!wzDataDir.empty())
			{
			#ifndef WZ_DATADIR_ISABSOLUTE
				// Treat WZ_DATADIR as a relative path - append to the install PREFIX
				tmpstr = prefix + dirSeparator + wzDataDir;
				registerSearchPath(tmpstr, 4);
				rebuildSearchPath(mod_multiplay, true);
			#else
				// Treat WZ_DATADIR as an absolute path, and use directly
				registerSearchPath(wzDataDir, 4);
				rebuildSearchPath(mod_multiplay, true);
			#endif
			}

			if (!PHYSFS_exists("gamedesc.lev"))
			{
				// Relocation for AutoPackage (<prefix>/share/warzone2100/)
				tmpstr = prefix + dirSeparator + "share" + dirSeparator + "warzone2100" + dirSeparator;
				registerSearchPath(tmpstr, 5);
				rebuildSearchPath(mod_multiplay, true);
			}
		}
	}
#endif

#ifdef WZ_OS_MAC
	if (!PHYSFS_exists("gamedesc.lev"))
	{
		auto resourceDataPathResult = wzMacAppBundleGetResourceDirectoryPath();
		if (resourceDataPathResult.has_value())
		{
			WzString resourceDataPath(resourceDataPathResult.value());
			resourceDataPath += "/data";
			registerSearchPath(resourceDataPath.toUtf8(), 3);
			rebuildSearchPath(mod_multiplay, true);
		}
		else
		{
			debug(LOG_FATAL, "Could not obtain resources directory.");
		}
	}
#endif

	// Commandline supplied datadir
	if (strlen(datadir) != 0)
	{
		registerSearchPath(datadir, 1);
	}

	// User's home dir
	const char *pCurrWriteDir = PHYSFS_getWriteDir();
	if (pCurrWriteDir)
	{
		registerSearchPath(pCurrWriteDir, 2);
	}
	else
	{
		debug(LOG_FATAL, "No write dir set?");
	}
	rebuildSearchPath(mod_multiplay, true);

	/** Debugging and sanity checks **/

	printSearchPath();

	if (PHYSFS_exists("gamedesc.lev"))
	{
		debug(LOG_WZ, "gamedesc.lev found at %s", WZ_PHYSFS_getRealDir_String("gamedesc.lev").c_str());
	}
	else
	{
		debug(LOG_FATAL, "Could not find game data. Aborting.");
		exit(1);
	}
}


/***************************************************************************
	Make a directory in write path and set a variable to point to it.
***************************************************************************/
static void make_dir(char *dest, const char *dirname, const char *subdir)
{
	strcpy(dest, dirname);
	if (subdir != nullptr)
	{
		strcat(dest, "/");
		strcat(dest, subdir);
	}
	{
		size_t l = strlen(dest);

		if (dest[l - 1] != '/')
		{
			dest[l] = '/';
			dest[l + 1] = '\0';
		}
	}
	if (!PHYSFS_mkdir(dest))
	{
		debug(LOG_FATAL, "Unable to create directory \"%s\" in write dir \"%s\"!",
		      dest, PHYSFS_getWriteDir());
		exit(EXIT_FAILURE);
	}
}


/*!
 * Preparations before entering the title (mainmenu) loop
 * Would start the timer in an event based mainloop
 */
static void startTitleLoop(bool onInitialStartup = false)
{
	SetGameMode(GS_TITLE_SCREEN);

	if (!onInitialStartup)
	{
		initLoadingScreen(true);
	}
	if (!frontendInitialise("wrf/frontend.wrf"))
	{
		debug(LOG_FATAL, "Shutting down after failure");
		exit(EXIT_FAILURE);
	}
	if (!onInitialStartup)
	{
		closeLoadingScreen();
	}
}


/*!
 * Shutdown/cleanup after the title (mainmenu) loop
 * Would stop the timer
 */
static void stopTitleLoop()
{
	if (!frontendShutdown())
	{
		debug(LOG_FATAL, "Shutting down after failure");
		exit(EXIT_FAILURE);
	}
}

static void setCDAudioForCurrentGameMode()
{
	auto gameMode = ActivityManager::instance().getCurrentGameMode();
	switch (gameMode)
	{
		case ActivitySink::GameMode::CAMPAIGN:
			cdAudio_SetGameMode(MusicGameMode::CAMPAIGN);
			break;
		case ActivitySink::GameMode::CHALLENGE:
			cdAudio_SetGameMode(MusicGameMode::CHALLENGE);
			break;
		case ActivitySink::GameMode::SKIRMISH:
			cdAudio_SetGameMode(MusicGameMode::SKIRMISH);
			break;
		case ActivitySink::GameMode::MULTIPLAYER:
			cdAudio_SetGameMode(MusicGameMode::MULTIPLAYER);
			break;
		default:
			debug(LOG_ERROR, "Unhandled started game mode for cd audio: %u", (unsigned int)gameMode);
			break;
	}
}

/*!
 * Preparations before entering the game loop
 * Would start the timer in an event based mainloop
 */
static void startGameLoop()
{
	SetGameMode(GS_NORMAL);
	initLoadingScreen(true);

	ActivityManager::instance().startingGame();

	// set up CD audio for game mode
	setCDAudioForCurrentGameMode();

	// Not sure what aLevelName is, in relation to game.map. But need to use aLevelName here, to be able to start the right map for campaign, and need game.hash, to start the right non-campaign map, if there are multiple identically named maps.
	if (!levLoadData(aLevelName, &game.hash, nullptr, GTYPE_SCENARIO_START))
	{
		debug(LOG_FATAL, "Shutting down after failure");
		wzQuit(EXIT_FAILURE);
		return;
	}

	screen_StopBackDrop();
	closeLoadingScreen();

	// Trap the cursor if cursor snapping is enabled
	if (war_GetTrapCursor())
	{
		wzGrabMouse();
	}

	// Disable resizable windows if it's a multiplayer game
	if (runningMultiplayer())
	{
		// This is because the main loop gets frozen while the window resizing / edge dragging occurs
		// which effectively pauses the game, and pausing is otherwise disabled in multiplayer games.
		// FIXME: Figure out a workaround?
		wzSetWindowIsResizable(false);
	}

	// set a flag for the trigger/event system to indicate initialisation is complete
	gameInitialised = true;

	if (challengeActive)
	{
		addMissionTimerInterface();
	}
	triggerEvent(TRIGGER_START_LEVEL);
	screen_disableMapPreview();

	auto currentGameMode = ActivityManager::instance().getCurrentGameMode();
	switch (currentGameMode)
	{
		case ActivitySink::GameMode::MENUS:
			// should not happen
			break;
		case ActivitySink::GameMode::CAMPAIGN:
			// replays not currently supported
			break;
		case ActivitySink::GameMode::CHALLENGE:
		case ActivitySink::GameMode::SKIRMISH:
		case ActivitySink::GameMode::MULTIPLAYER:
		{
			// start saving a replay
			if (!war_getDisableReplayRecording())
			{
				WZGameReplayOptionsHandler replayOptions;
				NETreplaySaveStart((currentGameMode == ActivitySink::GameMode::MULTIPLAYER) ? "multiplay" : "skirmish", replayOptions, war_getMaxReplaysSaved(), (currentGameMode == ActivitySink::GameMode::MULTIPLAYER));
			}
			break;
		}
		default:
			debug(LOG_INFO, "Unhandled case: %u", (unsigned int)currentGameMode);
	}

	setMaxFastForwardTicks(WZ_DEFAULT_MAX_FASTFORWARD_TICKS, true); // default value / spectator "catch-up" behavior
	if (NETisReplay())
	{
		if (!headlessGameMode() && !autogame_enabled())
		{
			// for replays, ensure we don't start off fast-forwarding
			setMaxFastForwardTicks(0, true);
		}
		else
		{
			// when loading replays in headless / autogame mode, set to fast-forward
			setMaxFastForwardTicks(10, false);
		}
	}
}


/*!
 * Shutdown/cleanup after the game loop
 * Would stop the timer
 */
static void stopGameLoop()
{
	clearInfoMessages(); // clear CONPRINTF messages before each new game/mission

	NETreplaySaveStop();
	NETshutdownReplay();

	if (gameLoopStatus != GAMECODE_NEWLEVEL)
	{
		clearBlueprints();
		initLoadingScreen(true); // returning to f.e. do a loader.render not active
		if (gameLoopStatus != GAMECODE_LOADGAME)
		{
			if (!levReleaseAll())
			{
				debug(LOG_ERROR, "levReleaseAll failed!");
			}
			for (auto& player : NetPlay.players)
			{
				player.resetAll();
			}
			NetPlay.players.resize(MAX_CONNECTED_PLAYERS);
		}
		closeLoadingScreen();
		reloadMPConfig();
	}

	// Disable cursor trapping
	if (war_GetTrapCursor())
	{
		wzReleaseMouse();
	}

	// Re-enable resizable windows
	if (wzGetCurrentWindowMode() == WINDOW_MODE::windowed)
	{
		// FIXME: This is required because of the disabling in startGameLoop()
		wzSetWindowIsResizable(true);
	}

	gameInitialised = false;
}


/*!
 * Load a savegame and start into the game loop
 * Game data should be initialised afterwards, so that startGameLoop is not necessary anymore.
 */
static bool initSaveGameLoad()
{
	// NOTE: always setGameMode correctly before *any* loading routines!
	SetGameMode(GS_NORMAL);
	initLoadingScreen(true);

	// load up a save game
	if (!loadGameInit(saveGameName))
	{
		// FIXME: we really should throw up a error window, but we can't (easily) so I won't.
		debug(LOG_ERROR, "Trying to load Game %s failed!", saveGameName);
		debug(LOG_POPUP, "Failed to load a save game! It is either corrupted or a unsupported format.\n\nRestarting main menu.");
		// FIXME: If we bomb out on a in game load, then we would crash if we don't do the next two calls
		// Doesn't seem to be a way to tell where we are in game loop to determine if/when we should do the two calls.
		gameLoopStatus = GAMECODE_FASTEXIT;	// clear out all old data
		stopGameLoop();
		startTitleLoop(); // Restart into titleloop
		SetGameMode(GS_TITLE_SCREEN);
		return false;
	}

	ActivityManager::instance().startingSavedGame();

	// set up CD audio for game mode
	// (must come after savegame is loaded so that proper game mode can be determined)
	setCDAudioForCurrentGameMode();

	screen_StopBackDrop();
	closeLoadingScreen();

	// Trap the cursor if cursor snapping is enabled
	if (war_GetTrapCursor())
	{
		wzGrabMouse();
	}
	if (challengeActive)
	{
		addMissionTimerInterface();
	}

	return true;
}


/*!
 * Run the code inside the gameloop
 */
static void runGameLoop()
{
	// Run the second half of a queued gameloopstatus change
	// (This is needed so that the main loop completes once between initializing the loading screen and actually loading)
	switch (gameLoopStatus)
	{
	case GAMECODE_QUITGAME:
		debug(LOG_MAIN, "GAMECODE_QUITGAME");
		ActivityManager::instance().quitGame(collectEndGameStatsData(), Cheated);
		cdAudio_SetGameMode(MusicGameMode::MENUS);
		stopGameLoop();
		startTitleLoop(); // Restart into titleloop
		gameLoopStatus = GAMECODE_CONTINUE;
		return;
	case GAMECODE_LOADGAME:
		debug(LOG_MAIN, "GAMECODE_LOADGAME");
		stopGameLoop();
		initSaveGameLoad(); // Restart and load a savegame
		gameLoopStatus = GAMECODE_CONTINUE;
		return;
	case GAMECODE_NEWLEVEL:
		debug(LOG_MAIN, "GAMECODE_NEWLEVEL");
		stopGameLoop();
		startGameLoop(); // Restart gameloop
		gameLoopStatus = GAMECODE_CONTINUE;
		return;
	default:
		// ignore other values, and proceed with gameLoop
		break;
	}

	gameLoopStatus = gameLoop();
	switch (gameLoopStatus)
	{
	case GAMECODE_CONTINUE:
	case GAMECODE_PLAYVIDEO:
		break;
	case GAMECODE_QUITGAME:
		debug(LOG_MAIN, "GAMECODE_QUITGAME");
		initLoadingScreen(true);
		break;
	case GAMECODE_LOADGAME:
		debug(LOG_MAIN, "GAMECODE_LOADGAME");
		initLoadingScreen(true);
		break;
	case GAMECODE_NEWLEVEL:
		debug(LOG_MAIN, "GAMECODE_NEWLEVEL");
		initLoadingScreen(true);
		break;
	// Never thrown:
	case GAMECODE_FASTEXIT:
	case GAMECODE_RESTARTGAME:
		break;
	default:
		debug(LOG_ERROR, "Unknown code returned by gameLoop");
		break;
	}
}


static optional<TITLECODE> queuedTilecodeChange = nullopt;

/*!
 * Run the code inside the titleloop
 */
static void runTitleLoop()
{
	if (queuedTilecodeChange.has_value())
	{
		// Run the second half of a queued titlecode change
		// (This is needed so that the main loop completes once between initializing the loading screen and actually loading)
		TITLECODE toProcess = queuedTilecodeChange.value();
		queuedTilecodeChange.reset();
		switch (toProcess)
		{
		case TITLECODE_SAVEGAMELOAD:
			{
				debug(LOG_MAIN, "TITLECODE_SAVEGAMELOAD");
				// Restart into gameloop and load a savegame, ONLY on a good savegame load!
				stopTitleLoop();
				if (!initSaveGameLoad())
				{
					// we had a error loading savegame (corrupt?), so go back to title screen?
					stopGameLoop();
					startTitleLoop();
					changeTitleMode(TITLE);
				}
				closeLoadingScreen();
				return;
			}
		case TITLECODE_STARTGAME:
			debug(LOG_MAIN, "TITLECODE_STARTGAME");
			stopTitleLoop();
			startGameLoop(); // Restart into gameloop
			closeLoadingScreen();
			return;
		default:
			// ignore unexpected value
			break;
		}
	}

	switch (titleLoop())
	{
	case TITLECODE_CONTINUE:
		break;
	case TITLECODE_QUITGAME:
		debug(LOG_MAIN, "TITLECODE_QUITGAME");
		stopTitleLoop();
		wzQuit(0);
		break;
	case TITLECODE_SAVEGAMELOAD:
		{
			debug(LOG_MAIN, "TITLECODE_SAVEGAMELOAD");
			initLoadingScreen(true);
			queuedTilecodeChange = TITLECODE_SAVEGAMELOAD;
			break;
		}
	case TITLECODE_STARTGAME:
		debug(LOG_MAIN, "TITLECODE_STARTGAME");
		initLoadingScreen(true);
		queuedTilecodeChange = TITLECODE_STARTGAME;
		break;
	case TITLECODE_SHOWINTRO:
		debug(LOG_MAIN, "TITLECODE_SHOWINTRO");
		seq_ClearSeqList();
		seq_AddSeqToList("titles.ogg", nullptr, nullptr, false);
		seq_AddSeqToList("devastation.ogg", nullptr, "devastation.txa", false);
		seq_StartNextFullScreenVideo();
		break;
	default:
		debug(LOG_ERROR, "Unknown code returned by titleLoop");
		break;
	}
}

/*!
 * The mainloop.
 * Fetches events, executes appropriate code
 */
void mainLoop()
{
	frameUpdate(); // General housekeeping

	pie_ScreenFrameRenderBegin();

	// Screenshot key is now available globally
	if (keyPressed(KEY_F10))
	{
		kf_ScreenDump();
		inputLoseFocus();		// remove it from input stream
	}

	wzSetCursor(CURSOR_DEFAULT); // if cursor isn't set by anything in the mainLoop, it should revert to default.

	if (NetPlay.bComms || focusState == FOCUS_IN || !war_GetPauseOnFocusLoss())
	{
		if (loop_GetVideoStatus())
		{
			videoLoop(); // Display the video if necessary
			pie_ScreenFrameRenderEnd();
		}
		else switch (GetGameMode())
			{
			case GS_NORMAL: // Run the gameloop code
				runGameLoop();
				// gameLoop handles pie_ScreenFrameRenderEnd()
				break;
			case GS_TITLE_SCREEN: // Run the titleloop code
				runTitleLoop();
				pie_ScreenFrameRenderEnd();
				break;
			default:
				break;
			}
		realTimeUpdate(); // Update realTime.
	}

	wzApplyCursor();
	runNotifications();
#if defined(ENABLE_DISCORD)
	discordRPCPerFrame();
#endif
}

bool getUTF8CmdLine(int *const _utfargc WZ_DECL_UNUSED, char *** const _utfargv WZ_DECL_UNUSED) // explicitely pass by reference
{
#ifdef WZ_OS_WIN
	int wargc;
	wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);

	if (wargv == NULL)
	{
		const DWORD err = GetLastError();
		char *err_string;

		// Retrieve a (locally encoded) string describing the error (uses LocalAlloc() to allocate memory)
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, (char *)&err_string, 0, NULL);

		debug(LOG_FATAL, "CommandLineToArgvW failed: %s (code:%lu)", err_string, err);

		LocalFree(err_string); // Free the chunk of memory FormatMessageA gave us
		LocalFree(wargv);
		return false;
	}
	// the following malloc and UTF16toUTF8 will be cleaned up in realmain().
	*_utfargv = (char **)malloc(sizeof(char *) * wargc);
	if (!*_utfargv)
	{
		debug(LOG_FATAL, "Out of memory!");
		abort();
		return false;
	}
	for (int i = 0; i < wargc; ++i)
	{
		STATIC_ASSERT(sizeof(wchar_t) == sizeof(utf_16_char)); // Should be true on windows
		(*_utfargv)[i] = UTF16toUTF8((const utf_16_char *)wargv[i], NULL); // only returns null when memory runs out
		if ((*_utfargv)[i] == NULL)
		{
			*_utfargc = i;
			LocalFree(wargv);
			abort();
			return false;
		}
	}
	*_utfargc = wargc;
	LocalFree(wargv);
#endif
	return true;
}

#if defined(WZ_OS_WIN)

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

void osSpecificFirstChanceProcessSetup_Win()
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

static bool winCheckIfRunningUnderWine(std::string* output_wineinfostr = nullptr, std::string* output_platform = nullptr)
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	typedef const char* (CDECL *WineGetVersionFunction)(void);
	typedef void (CDECL *WineGetHostVersionFunction)(const char **sysname, const char **release);

	bool bResult = false;
	std::string resultWineVersionInfo = "Wine";

	HMODULE hntdll = GetModuleHandleW(L"ntdll.dll");
	if (hntdll != NULL)
	{
		WineGetVersionFunction pWineGetVersion = reinterpret_cast<WineGetVersionFunction>(reinterpret_cast<void*>(GetProcAddress(hntdll, "wine_get_version")));
		WineGetHostVersionFunction pWineGetHostVersion = reinterpret_cast<WineGetHostVersionFunction>(reinterpret_cast<void*>(GetProcAddress(hntdll, "wine_get_host_version")));

		if (pWineGetVersion == nullptr && pWineGetHostVersion == nullptr && !bResult)
		{
			auto ptrWineSetUnixEnv = GetProcAddress(hntdll, "__wine_set_unix_env");
			if (ptrWineSetUnixEnv != NULL)
			{
				bResult = true;
			}
		}

		if (pWineGetVersion != nullptr)
		{
			bResult = true;
			const char* pWineVer = pWineGetVersion();
			if (pWineVer != nullptr)
			{
				resultWineVersionInfo += std::string(" ") + pWineVer;
			}
		}

		const char* pSysname = nullptr;
		const char* pSysversion = nullptr;
		if (pWineGetHostVersion != nullptr)
		{
			bResult = true;
			pWineGetHostVersion(&pSysname, &pSysversion);
		}
		if (pSysname != nullptr)
		{
			if (output_platform)
			{
				(*output_platform) = pSysname;
			}
			resultWineVersionInfo += std::string(" (under ") + pSysname;
			if (pSysversion != nullptr)
			{
				resultWineVersionInfo += std::string(" ") + pSysversion;
			}
			resultWineVersionInfo += ")";
		}
	}

	if (!bResult)
	{
		HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
		if (hKernel32 != NULL)
		{
			auto wineUnixFileNameFuncPt = GetProcAddress(hKernel32, "wine_get_unix_file_name");
			if (wineUnixFileNameFuncPt != NULL)
			{
				bResult = true;
			}
			else
			{
				auto wineDosFileNameFuncPt = GetProcAddress(hKernel32, "wine_get_dos_file_name");
				if (wineDosFileNameFuncPt != NULL)
				{
					bResult = true;
				}
			}
		}
	}

	if (bResult)
	{
		if (output_wineinfostr)
		{
			(*output_wineinfostr) = resultWineVersionInfo;
		}
	}

	return bResult;
#else
	return false;
#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
}

#define WINEOPENURLWRAPPER(url, cl) \
openURLInBrowser(url); \
if (cl) { wzQuit(0); }

void osSpecificPostInit_Win()
{
	std::string wineInfoStr;
	std::string wineHostPlatform;
	if (winCheckIfRunningUnderWine(&wineInfoStr, &wineHostPlatform))
	{
		const char* pWineNativeAvailableMsg = "You are running the Windows version of Warzone 2100 under Wine.\n\nA native version for your platform is likely available (and will perform better).\n\nPlease visit: https://wz2100.net";
		std::string platformSimple;
		// Display a messagebox that a native version is available for this platform
		if (!wineHostPlatform.empty())
		{
			if (strncasecmp(wineHostPlatform.c_str(), "Darwin", std::min<size_t>(wineHostPlatform.size(), strlen("Darwin"))) == 0)
			{
				// macOS
				platformSimple = "macOS";
				pWineNativeAvailableMsg = "You are running the Windows version of Warzone 2100 under Wine.\n\nA native version for macOS is available.\n\nPlease visit: https://wz2100.net";
			}
			if (strncasecmp(wineHostPlatform.c_str(), "Linux", std::min<size_t>(wineHostPlatform.size(), strlen("Linux"))) == 0)
			{
				// Linux
				platformSimple = "Linux";
				pWineNativeAvailableMsg = "You are running the Windows version of Warzone 2100 under Wine.\n\nNative builds for Linux are available.\n\nPlease visit: https://wz2100.net";
			}
		}

		wzDisplayDialog(Dialog_Information, "Warzone 2100 under Wine", pWineNativeAvailableMsg);

		std::string url = "https://warzone2100.github.io/update-data/redirect/wine.html";
		std::string queryString;
		if (!platformSimple.empty())
		{
			queryString += (queryString.empty()) ? "?" : "&";
			queryString += std::string("platform=") + platformSimple;
		}
		std::string variant;
		if ((GetEnvironmentVariableW(L"STEAM_COMPAT_APP_ID", NULL, 0) > 0) || (GetEnvironmentVariableW(L"SteamAppId", NULL, 0) > 0))
		{
			variant = "Proton";
		}
		if (!variant.empty())
		{
			queryString += (queryString.empty()) ? "?" : "&";
			queryString += std::string("variant=") + variant;
		}
		url += queryString;
		WINEOPENURLWRAPPER(url.c_str(), !variant.empty())
	}
}
#endif /* defined(WZ_OS_WIN) */

void osSpecificFirstChanceProcessSetup()
{
#if defined(WZ_OS_WIN)
	osSpecificFirstChanceProcessSetup_Win();
#elif defined(WZ_OS_UNIX)
	// Before anything else is run, and before creating any threads, ignore SIGPIPE
	// see: https://curl.haxx.se/libcurl/c/CURLOPT_NOSIGNAL.html
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	ignoredSIGPIPE = sigaction(SIGPIPE, &sa, 0) == 0;

	// Initialize time conversion information
	tzset();
#else
	// currently, no-op
#endif
}

void osSpecificPostInit()
{
#if defined(WZ_OS_WIN)
	osSpecificPostInit_Win();
#else
	// currently, no-op
#endif
}

static std::string getDefaultLogFilePath(const char *platformDirSeparator)
{
	static std::string defaultLogFileName;
	if (defaultLogFileName.empty()) // only generate this once per run, so multiple callers get the same value
	{
		time_t aclock;
		struct tm newtime;
		char buf[PATH_MAX];

		time(&aclock);						// Get time in seconds
		newtime = getLocalTime(aclock);		// Convert time to struct
		snprintf(buf, sizeof(buf), "WZlog-%02d%02d_%02d%02d%02d.txt",
				 newtime.tm_mon + 1, newtime.tm_mday, newtime.tm_hour, newtime.tm_min, newtime.tm_sec);
		defaultLogFileName = buf;
	}
	// log name is logs/(or \)WZlog-MMDD_HHMMSS.txt
	return std::string("logs") + platformDirSeparator + defaultLogFileName;
}

static bool initializeCrashHandlingContext(optional<video_backend> gfxbackend)
{
	std::string gfxBackendString = "null backend";
	if (gfxbackend.has_value())
	{
		gfxBackendString = to_string(gfxbackend.value());
	}
	crashHandlingProviderSetTag("wz.gfx_backend", gfxBackendString);
	auto backendInfo = gfx_api::context::get().getBackendGameInfo();
	// Truncate absurdly long backend info values (if needed - common culprit is GL_EXTENSIONS)
	const size_t MAX_BACKENDINFO_VALUE_LENGTH = 2048;
	for (auto& it : backendInfo)
	{
		if (it.second.size() > MAX_BACKENDINFO_VALUE_LENGTH)
		{
			size_t remainingLength = it.second.size() - MAX_BACKENDINFO_VALUE_LENGTH;
			it.second = it.second.substr(0, MAX_BACKENDINFO_VALUE_LENGTH) + "[...] (+ " + std::to_string(remainingLength) + " chars)";
		}
	}
	nlohmann::json jsonBackendInfo = backendInfo;
	crashHandlingProviderSetContext("wz.gfx", jsonBackendInfo);

	return true;
}

static void wzCmdInterfaceInit()
{
	switch (wz_command_interface())
	{
		case WZ_Command_Interface::None:
			return;
		case WZ_Command_Interface::StdIn_Interface:
			stdInThreadInit();
			break;
	}
}

static void wzCmdInterfaceShutdown()
{
	stdInThreadShutdown();
}

static void cleanupOldLogFiles()
{
	const int MAX_OLD_LOGS = war_getOldLogsLimit();
	if (MAX_OLD_LOGS < 0)
	{
		// skip cleanup
		return;
	}

	ASSERT_OR_RETURN(, PHYSFS_isInit() != 0, "PhysFS isn't initialized");
	// safety check to ensure we *never* delete the current log file
	WzString fullCurrentLogFileName = WzString::fromUtf8(getDefaultLogFilePath("/"));
	if (fullCurrentLogFileName.startsWith("logs/"))
	{
		fullCurrentLogFileName.remove(0, 5);
	}

	CleanupFileEnumFilterFunctions filterFuncs;
	constexpr std::chrono::hours MinDeletableAge(24 * 7); // never delete logs that are less than a week old
	auto currentTime = std::chrono::system_clock::now();
	filterFuncs.fileNameFilterFunction = [](const char *fileName) -> bool {
		return filenameEndWithExtension(fileName, ".txt") || filenameEndWithExtension(fileName, ".log");
	};
	filterFuncs.fileLastModifiedFilterFunction = [currentTime, MinDeletableAge](time_t fileLastModified) -> bool {
		return std::chrono::duration_cast<std::chrono::hours>(currentTime - std::chrono::system_clock::from_time_t(fileLastModified)) >= MinDeletableAge;
	};

	// clean up old log .txt / .log files
	WZ_PHYSFS_cleanupOldFilesInFolder("logs", filterFuncs, MAX_OLD_LOGS, [fullCurrentLogFileName](const char *fileName){
		if (fullCurrentLogFileName.compare(fileName) == 0)
		{
			// skip
			return true;
		}
		if (PHYSFS_delete(fileName) == 0)
		{
			debug(LOG_ERROR, "Failed to delete old log file: %s", fileName);
			return false;
		}
		return true;
	});
}

// for backend detection
extern const char *BACKEND;

static int utfargc = 0;
static char **utfargv = nullptr;

void mainShutdown()
{
	ActivityManager::instance().preSystemShutdown();

	switch (GetGameMode())
	{
		case GS_NORMAL:
			// if running a game while quitting, stop the game loop
			// (currently required for some cleanup) (should modelShutdown() be added to systemShutdown?)
			stopGameLoop();
			break;
		case GS_TITLE_SCREEN:
			// if showing the title / menus while quitting, stop the title loop
			// (currently required for some cleanup)
			stopTitleLoop();
			break;
		default:
			break;
	}
	saveConfig();
	writeFavoriteStructsFile(FavoriteStructuresPath);
#if defined(ENABLE_DISCORD)
	discordRPCShutdown();
#endif
	wzCmdInterfaceShutdown();
	urlRequestShutdown();
	cleanupOldLogFiles();
	systemShutdown();
#ifdef WZ_OS_WIN	// clean up the memory allocated for the command line conversion
	for (int i = 0; i < utfargc; i++)
	{
		char *** const utfargvF = &utfargv;
		free((void *)(*utfargvF)[i]);
	}
	free(utfargv);
#endif
	ActivityManager::instance().shutdown();
}

int realmain(int argc, char *argv[])
{
	utfargc = argc;
	utfargv = (char **)argv;

	osSpecificFirstChanceProcessSetup();

	debug_init();
	debug_register_callback(debug_callback_stderr, nullptr, nullptr, nullptr);
#if defined(_WIN32) && defined(DEBUG_INSANE)
	debug_register_callback(debug_callback_win32debug, NULL, NULL, NULL);
#endif // WZ_OS_WIN && DEBUG_INSANE

	// *****
	// NOTE: Try *NOT* to use debug() output routines without some other method of informing the user.  All this output is sent to /dev/nul at this point on some platforms!
	// *****
	if (!getUTF8CmdLine(&utfargc, &utfargv))
	{
		return EXIT_FAILURE;
	}

	// find debugging flags extra early
	if (!ParseCommandLineDebugFlags(utfargc, utfargv))
	{
		return EXIT_FAILURE;
	}

	/*** Initialize PhysicsFS ***/
	initialize_PhysicsFS(utfargv[0]);

	/** Initialize crash-handling provider, if configured */
	/** NOTE: Should come as early as possible in process init, but needs to be after initialize_PhysicsFS because we need the platform pref dir for storing temporary crash files... */
	bool debugCrashHandler = false;
	bool bCrashHandlingProvider = useCrashHandlingProvider(utfargc, utfargv, debugCrashHandler);
	if (bCrashHandlingProvider)
	{
		bCrashHandlingProvider = initCrashHandlingProvider(getWzPlatformPrefDir(), getDefaultLogFilePath(PHYSFS_getDirSeparator()), debugCrashHandler);
	}
	auto shutdown_crash_handling_provider_on_return = gsl::finally([bCrashHandlingProvider] { if (bCrashHandlingProvider) { shutdownCrashHandlingProvider(); } });

	/*** Initialize translations ***/
	/*** NOTE: Should occur before any use of gettext / libintl translation routines. ***/
	initI18n();

	wzMain(argc, argv);		// init Qt integration first

	LaunchInfo::initialize(argc, argv);
	if (!bCrashHandlingProvider)
	{
		setupExceptionHandler(utfargc, utfargv, version_getFormattedVersionString(false), version_getVersionedAppDirFolderName(), isPortableMode());
	}

	/*** Initialize sodium library ***/
	if (sodium_init() < 0) {
        /* libsodium couldn't be initialized - it is not safe to use */
		fprintf(stderr, "Failed to initialize libsodium\n");
		return EXIT_FAILURE;
    }

	/*** Initialize URL Request library ***/
	urlRequestInit();

	// find early boot info
	if (!ParseCommandLineEarly(utfargc, utfargv))
	{
		return EXIT_FAILURE;
	}

	/* Initialize the write/config directory for PhysicsFS.
	 * This needs to be done __after__ the early commandline parsing,
	 * because the user might tell us to use an alternative configuration
	 * directory.
	 */
	initialize_ConfigDir();

	/*** Initialize directory structure ***/

	PHYSFS_mkdir("autohost");	// autohost games launched with --autohost=game

	PHYSFS_mkdir("challenges");	// custom challenges

	PHYSFS_mkdir("logs");		// netplay, mingw crash reports & WZ logs

	make_dir(MultiCustomMapsPath, "maps", nullptr); // needed to prevent crashes when getting map

	PHYSFS_mkdir(version_getVersionedModsFolderPath("autoload").c_str());	// mods that are automatically loaded
	PHYSFS_mkdir(version_getVersionedModsFolderPath("campaign").c_str());	// campaign only mods activated with --mod_ca=example.wz
	PHYSFS_mkdir("mods/downloads");	// mod download directory - NOT currently versioned
	PHYSFS_mkdir(version_getVersionedModsFolderPath("global").c_str());	// global mods activated with --mod=example.wz
	PHYSFS_mkdir(version_getVersionedModsFolderPath("multiplay").c_str());	// multiplay only mods activated with --mod_mp=example.wz
	PHYSFS_mkdir(version_getVersionedModsFolderPath("music").c_str());	// music mods that are automatically loaded

	make_dir(MultiPlayersPath, "multiplay", "players"); // player profiles

	PHYSFS_mkdir("music");	// custom music overriding default music and music mods

	make_dir(SaveGamePath, "savegames", nullptr); 	// save games
	PHYSFS_mkdir(SAVEGAME_CAM);		// campaign save games
	PHYSFS_mkdir(SAVEGAME_CAM_AUTO);	// campaign autosave games
	PHYSFS_mkdir(SAVEGAME_SKI);		// skirmish save games
	PHYSFS_mkdir(SAVEGAME_SKI_AUTO);	// skirmish autosave games

	make_dir(ReplayPath, "replay", nullptr);  // replays
	PHYSFS_mkdir("replay/skirmish");
	PHYSFS_mkdir("replay/multiplay");

	make_dir(ScreenDumpPath, "screenshots", nullptr);	// for screenshots

	PHYSFS_mkdir("tests");			// test games launched with --skirmish=game

	PHYSFS_mkdir("userdata");		// per-mod data user generated data
	PHYSFS_mkdir("userdata/campaign");	// contains campaign templates
	PHYSFS_mkdir("userdata/mp");		// contains multiplayer templates
	memset(rulesettag, 0, sizeof(rulesettag)); // stores tag property of ruleset.json files

	if (!customDebugfile)
	{
		// there was no custom debug file specified  (--debug-file=blah)
		// so we use our write directory to store our logs.
		WzString debug_filename = PHYSFS_getWriteDir();
		debug_filename.append(WzString::fromUtf8(getDefaultLogFilePath(PHYSFS_getDirSeparator())));
		debug_register_callback(debug_callback_file, debug_callback_file_init, debug_callback_file_exit, &debug_filename); // note: by the time this function returns, all use of debug_filename has completed

		debug(LOG_WZ, "Using %s debug file", debug_filename.toUtf8().c_str());
	}

	// Initialize random number generators
	srand((unsigned int)time(NULL));
	gameSRand((uint32_t)rand());

	// NOTE: it is now safe to use debug() calls to make sure output gets captured.
	check_Physfs();
	debug(LOG_WZ, "Warzone 2100 - %s", version_getFormattedVersionString(false));
	debug(LOG_WZ, "Using language: %s", getLanguage());
	debug(LOG_WZ, "Backend: %s", BACKEND);
	debug(LOG_MEMORY, "sizeof: SIMPLE_OBJECT=%ld, BASE_OBJECT=%ld, DROID=%ld, STRUCTURE=%ld, FEATURE=%ld, PROJECTILE=%ld",
	      (long)sizeof(SIMPLE_OBJECT), (long)sizeof(BASE_OBJECT), (long)sizeof(DROID), (long)sizeof(STRUCTURE), (long)sizeof(FEATURE), (long)sizeof(PROJECTILE));

#if defined(WZ_OS_UNIX)
	debug(LOG_WZ, "Ignoring SIGPIPE: %s", (ignoredSIGPIPE) ? "true" : "false");
#endif
	urlRequestOutputDebugInfo();

	// Initialize ActivityManager
	ActivityManager::instance().initialize();

	/* Put in the writedir root */
	sstrcpy(KeyMapPath, "keymap.json");
	sstrcpy(FavoriteStructuresPath, "favoriteStructures.json");

	// initialise all the command line states
	war_SetDefaultStates();

	debug(LOG_MAIN, "initializing");

	loadConfig();
	loadFavoriteStructsFile(FavoriteStructuresPath);

	// parse the command line
	if (!ParseCommandLine(utfargc, utfargv))
	{
		return EXIT_FAILURE;
	}

	// Save new (commandline) settings
	saveConfig();

	// Print out some initial information if in headless mode
	if (headlessGameMode())
	{
		fprintf(stdout, "--------------------------------------------------------------------------------------\n");
		fprintf(stdout, " * Warzone 2100 - Headless Mode\n");
		fprintf(stdout, " * %s\n", version_getFormattedVersionString(false));
		if (to_swap_mode(war_GetVsync()) == gfx_api::context::swap_interval_mode::immediate)
		{
			fprintf(stdout, " * NOTE: VSYNC IS DISABLED - CPU USAGE MAY BE UNBOUNDED\n");
		}
		fprintf(stdout, "--------------------------------------------------------------------------------------\n");
		fflush(stdout);
	}

	// Find out where to find the data
	scanDataDirs();

	// Now we check the mods to see if they exist or not (specified on the command line)
	// FIX ME: I know this is a bit hackish, but better than nothing for now?
	{
		std::string modtocheck;
#if defined WZ_PHYSFS_2_1_OR_GREATER
		PHYSFS_Stat metaData;
#endif

		// check whether given global mods are regular files
		auto globalModsPath = version_getVersionedModsFolderPath("global");
		for (auto iterator = global_mods.begin(); iterator != global_mods.end();)
		{
			modtocheck = globalModsPath + "/" + *iterator;
#if defined WZ_PHYSFS_2_0_OR_GREATER
			if (!PHYSFS_exists(modtocheck.c_str()) || WZ_PHYSFS_isDirectory(modtocheck.c_str()))
#elif defined WZ_PHYSFS_2_1_OR_GREATER
			PHYSFS_stat(modtocheck.c_str(), &metaData);
			if (metaData.filetype != PHYSFS_FILETYPE_REGULAR)
#endif
			{
				debug(LOG_ERROR, "The global mod \"%s\" you have specified doesn't exist!", iterator->c_str());
				global_mods.erase(iterator);
				rebuildSearchPath(mod_multiplay, true);
			}
			else
			{
				wz_info("global mod \"%s\" is enabled", iterator->c_str());
				++iterator;
			}
		}
		// check whether given campaign mods are regular files
		auto campaignModsPath = version_getVersionedModsFolderPath("campaign");
		for (auto iterator = campaign_mods.begin(); iterator != campaign_mods.end();)
		{
			modtocheck = campaignModsPath + "/" + *iterator;
#if defined WZ_PHYSFS_2_0_OR_GREATER
			if (!PHYSFS_exists(modtocheck.c_str()) || WZ_PHYSFS_isDirectory(modtocheck.c_str()))
#elif defined WZ_PHYSFS_2_1_OR_GREATER
			PHYSFS_stat(modtocheck.c_str(), &metaData);
			if (metaData.filetype != PHYSFS_FILETYPE_REGULAR)
#endif
			{
				debug(LOG_ERROR, "The campaign mod \"%s\" you have specified doesn't exist!", iterator->c_str());
				campaign_mods.erase(iterator);
				rebuildSearchPath(mod_campaign, true);
			}
			else
			{
				wz_info("campaign mod \"%s\" is enabled", iterator->c_str());
				++iterator;
			}
		}
		// check whether given multiplay mods are regular files
		auto multiplayModsPath = version_getVersionedModsFolderPath("multiplay");
		for (auto iterator = multiplay_mods.begin(); iterator != multiplay_mods.end();)
		{
			modtocheck = multiplayModsPath + "/" + *iterator;
#if defined WZ_PHYSFS_2_0_OR_GREATER
			if (!PHYSFS_exists(modtocheck.c_str()) || WZ_PHYSFS_isDirectory(modtocheck.c_str()))
#elif defined WZ_PHYSFS_2_1_OR_GREATER
			PHYSFS_stat(modtocheck.c_str(), &metaData);
			if (metaData.filetype != PHYSFS_FILETYPE_REGULAR)
#endif
			{
				debug(LOG_ERROR, "The multiplay mod \"%s\" you have specified doesn't exist!", iterator->c_str());
				multiplay_mods.erase(iterator);
				rebuildSearchPath(mod_multiplay, true);
			}
			else
			{
				wz_info("multiplay mod \"%s\" is enabled", iterator->c_str());
				++iterator;
			}
		}
	}

	optional<video_backend> gfxbackend;
	if (!headlessGameMode())
	{
		gfxbackend = war_getGfxBackend();
	}
	if (!wzMainScreenSetup(gfxbackend, war_getAntialiasing(), war_getWindowMode(), war_GetVsync(), war_getLODDistanceBiasPercentage()))
	{
		saveConfig(); // ensure any setting changes are persisted on failure
		return EXIT_FAILURE;
	}

	initializeCrashHandlingContext(gfxbackend);

	wzCmdInterfaceInit();

	debug(LOG_WZ, "Warzone 2100 - %s", version_getFormattedVersionString(false));
	debug(LOG_WZ, "Using language: %s", getLanguage());
	debug(LOG_WZ, "Backend: %s", BACKEND);
	debug(LOG_MEMORY, "sizeof: SIMPLE_OBJECT=%ld, BASE_OBJECT=%ld, DROID=%ld, STRUCTURE=%ld, FEATURE=%ld, PROJECTILE=%ld",
	      (long)sizeof(SIMPLE_OBJECT), (long)sizeof(BASE_OBJECT), (long)sizeof(DROID), (long)sizeof(STRUCTURE), (long)sizeof(FEATURE), (long)sizeof(PROJECTILE));

	int w = pie_GetVideoBufferWidth();
	int h = pie_GetVideoBufferHeight();

	char buf[256];
	ssprintf(buf, "Video Mode %d x %d (%s)", w, h, to_display_string(war_getWindowMode()).c_str());
	addDumpInfo(buf);

	unsigned int horizScaleFactor, vertScaleFactor;
	wzGetGameToRendererScaleFactorInt(&horizScaleFactor, &vertScaleFactor);
	debug(LOG_WZ, "Game to Renderer Scale Factor (w x h): %u%% x %u%%", horizScaleFactor, vertScaleFactor);

	debug(LOG_MAIN, "Final initialization");
	if (!frameInitialise())
	{
		return EXIT_FAILURE;
	}
	if (!screenInitialise())
	{
		return EXIT_FAILURE;
	}
	if (!pie_LoadShaders())
	{
		return EXIT_FAILURE;
	}

	bool fogConfigOption = pie_GetFogEnabled();
	pie_SetFogStatus(false);

	pal_Init();

	pie_LoadBackDrop(SCREEN_RANDOMBDROP);
	pie_SetFogStatus(false);

	pie_ScreenFrameRenderEnd();

	if (!systemInitialise(horizScaleFactor, vertScaleFactor))
	{
		return EXIT_FAILURE;
	}

	// The systemInitialise() above sets fog to disabled as default so this must be after it
	if (fogConfigOption)
	{
		pie_EnableFog(true);
	}

	pie_InitLighting();

	//set all the pause states to false
	setAllPauseStates(false);

	// Copy this info to be used by the crash handler for the dump file
	ssprintf(buf, "Using Backend: %s", BACKEND);
	addDumpInfo(buf);
	ssprintf(buf, "Using language: %s", getLanguageName());
	addDumpInfo(buf);

	// Do the game mode specific initialisation.
	switch (GetGameMode())
	{
	case GS_TITLE_SCREEN:
		startTitleLoop(true);
		break;
	case GS_SAVEGAMELOAD:
		if (headlessGameMode())
		{
			fprintf(stdout, "Loading savegame ...\n");
		}
		initSaveGameLoad();
		break;
	case GS_NORMAL:
		startGameLoop();
		break;
	default:
		debug(LOG_ERROR, "Weirdy game status, I'm afraid!!");
		break;
	}

	WzInfoManager::initialize();
#if defined(ENABLE_DISCORD)
	discordRPCInitialize();
#endif

#if defined(WZ_CC_MSVC) && defined(DEBUG)
	debug_MEMSTATS();
#endif
	debug(LOG_MAIN, "Entering main loop");

#if defined(WZ_OS_MAC)
	if (headlessGameMode())
	{
		cocoaTransformToBackgroundApplication();
	}
#endif

	osSpecificPostInit();

	wzMainEventLoop(mainShutdown);

	int exitCode = wzGetQuitExitCode();
	wzShutdown();
	debug(LOG_MAIN, "Completed shutting down Warzone 2100");
	return exitCode;
}

/*!
 * Get the mode the game is currently in
 */
GS_GAMEMODE GetGameMode()
{
	return gameStatus;
}

/*!
 * Set the current mode
 */
void SetGameMode(GS_GAMEMODE status)
{
	gameStatus = status;
}
