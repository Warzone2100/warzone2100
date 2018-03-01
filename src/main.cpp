/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
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
/** @file
 *  The main file that launches the game and starts up everything.
 */

// Get platform defines before checking for them!
#include "lib/framework/wzapp.h"
#include <QtCore/QTextCodec>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

#if defined(WZ_OS_WIN)
#  include <shlobj.h> /* For SHGetFolderPath */
#  include <shellapi.h> /* CommandLineToArgvW */
#elif defined(WZ_OS_UNIX)
#  include <errno.h>
#endif // WZ_OS_WIN

#include "lib/framework/input.h"
#include "lib/framework/physfs_ext.h"
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
#include "lib/script/script.h"
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
#include "qtscript.h"
#include "research.h"
#include "scripttabs.h"
#include "seqdisp.h"
#include "warzoneconfig.h"
#include "main.h"
#include "wrappers.h"
#include "version.h"
#include "map.h"
#include "keybind.h"
#include <time.h>

#if defined(WZ_OS_MAC)
// NOTE: Moving these defines is likely to (and has in the past) break the mac builds
# include <CoreServices/CoreServices.h>
# include <unistd.h>
# include "cocoa_wrapper.h"
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
bool	gameInitialised = false;
char	SaveGamePath[PATH_MAX];
char	ScreenDumpPath[PATH_MAX];
char	MultiCustomMapsPath[PATH_MAX];
char	MultiPlayersPath[PATH_MAX];
char	KeyMapPath[PATH_MAX];
// Start game in title mode:
static GS_GAMEMODE gameStatus = GS_TITLE_SCREEN;
// Status of the gameloop
static GAMECODE gameLoopStatus = GAMECODE_CONTINUE;
static FOCUS_STATE focusState = FOCUS_IN;


/*!
 * Retrieves the current working directory and copies it into the provided output buffer
 * \param[out] dest the output buffer to put the current working directory in
 * \param size the size (in bytes) of \c dest
 * \return true on success, false if an error occurred (and dest doesn't contain a valid directory)
 */
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
		const int err = GetLastError();
		char *err_string = NULL;

		// Retrieve a string describing the error number (uses LocalAlloc() to allocate memory for err_string)
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, (char *)&err_string, 0, NULL);

		// Print an error message with the above description
		debug(LOG_ERROR, "GetCurrentDirectory failed (error code: %d): %s", err, err_string);

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


static void getPlatformUserDir(char *const tmpstr, size_t const size)
{
#if defined(WZ_OS_WIN)
//  When WZ_PORTABLE is passed, that means we want the config directory at the same location as the program file
	DWORD dwRet;
	wchar_t tmpWStr[MAX_PATH];
#ifndef WZ_PORTABLE
	if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, tmpWStr)))
	{
#else
	if ((dwRet = GetCurrentDirectoryW(MAX_PATH, tmpWStr)))
	{
		if (dwRet > MAX_PATH)
		{
			debug(LOG_FATAL, "Buffer exceeds maximum path to create directory. Exiting.");
			exit(1);
		}
#endif
		if (WideCharToMultiByte(CP_UTF8, 0, tmpWStr, -1, tmpstr, size, NULL, NULL) == 0)
		{
			debug(LOG_FATAL, "Config directory encoding conversion error.");
			exit(1);
		}
		strlcat(tmpstr, PHYSFS_getDirSeparator(), size);
	}
	else
#elif defined(WZ_OS_MAC)
	if (cocoaGetApplicationSupportDir(tmpstr, size))
	{
		strlcat(tmpstr, PHYSFS_getDirSeparator(), size);
	}
	else
#endif
		if (PHYSFS_getUserDir())
		{
			strlcpy(tmpstr, PHYSFS_getUserDir(), size); // Use PhysFS supplied UserDir (As fallback on Windows / Mac, default on Linux)
		}
	// If PhysicsFS fails (might happen if environment variable HOME is unset or wrong) then use the current working directory
		else if (getCurrentDir(tmpstr, size))
		{
			strlcat(tmpstr, PHYSFS_getDirSeparator(), size);
		}
		else
		{
			debug(LOG_FATAL, "Can't get UserDir?");
			abort();
		}
}

static void initialize_ConfigDir()
{
	char tmpstr[PATH_MAX] = { '\0' };

	if (strlen(configdir) == 0)
	{
		getPlatformUserDir(tmpstr, sizeof(tmpstr));

		if (!PHYSFS_setWriteDir(tmpstr)) // Workaround for PhysFS not creating the writedir as expected.
		{
			debug(LOG_FATAL, "Error setting write directory to \"%s\": %s",
			      tmpstr, WZ_PHYSFS_getLastError());
			exit(1);
		}

		if (!PHYSFS_mkdir(WZ_WRITEDIR)) // s.a.
		{
			debug(LOG_FATAL, "Error creating directory \"%s\": %s",
			      WZ_WRITEDIR, WZ_PHYSFS_getLastError());
			exit(1);
		}

		// Append the Warzone subdir
		sstrcat(tmpstr, WZ_WRITEDIR);
		sstrcat(tmpstr, PHYSFS_getDirSeparator());

		if (!PHYSFS_setWriteDir(tmpstr))
		{
			debug(LOG_FATAL, "Error setting write directory to \"%s\": %s",
			      tmpstr, WZ_PHYSFS_getLastError());
			exit(1);
		}
	}
	else
	{
		sstrcpy(tmpstr, configdir);

		// Make sure that we have a directory separator at the end of the string
		if (tmpstr[strlen(tmpstr) - 1] != PHYSFS_getDirSeparator()[0])
		{
			sstrcat(tmpstr, PHYSFS_getDirSeparator());
		}

		debug(LOG_WZ, "Using custom configuration directory: %s", tmpstr);

		if (!PHYSFS_setWriteDir(tmpstr)) // Workaround for PhysFS not creating the writedir as expected.
		{
			debug(LOG_FATAL, "Error setting write directory to \"%s\": %s",
			      tmpstr, WZ_PHYSFS_getLastError());
			exit(1);
		}
	}

	if (!OverrideRPTDirectory(tmpstr))
	{
		// since it failed, we just use our default path, and not the user supplied one.
		debug(LOG_ERROR, "Error setting exception hanlder to use directory %s", tmpstr);
	}


	// User's home dir first so we always see what we write
	PHYSFS_mount(PHYSFS_getWriteDir(), NULL, PHYSFS_PREPEND);

	PHYSFS_permitSymbolicLinks(1);

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
	char tmpstr[PATH_MAX], prefix[PATH_MAX];
	char *separator;

#if defined(WZ_OS_MAC)
	// version-independent location for video files
	registerSearchPath("/Library/Application Support/Warzone 2100/", 1);
#endif

	// Find out which PREFIX we are in...
	sstrcpy(prefix, PHYSFS_getBaseDir());

	separator = strrchr(prefix, *PHYSFS_getDirSeparator());
	if (separator)
	{
		*separator = '\0'; // Trim ending '/', which getBaseDir always provides

		separator = strrchr(prefix, *PHYSFS_getDirSeparator());
		if (separator)
		{
			*separator = '\0'; // Skip the last dir from base dir
		}
	}

	// Commandline supplied datadir
	if (strlen(datadir) != 0)
	{
		registerSearchPath(datadir, 1);
	}

	// User's home dir
	registerSearchPath(PHYSFS_getWriteDir(), 2);
	rebuildSearchPath(mod_multiplay, true);

	if (!PHYSFS_exists("gamedesc.lev"))
	{
		// Data in source tree
		sstrcpy(tmpstr, prefix);
		sstrcat(tmpstr, "/data/");
		registerSearchPath(tmpstr, 3);
		rebuildSearchPath(mod_multiplay, true);

		if (!PHYSFS_exists("gamedesc.lev"))
		{
			// Relocation for AutoPackage
			sstrcpy(tmpstr, prefix);
			sstrcat(tmpstr, "/share/warzone2100/");
			registerSearchPath(tmpstr, 4);
			rebuildSearchPath(mod_multiplay, true);

			if (!PHYSFS_exists("gamedesc.lev"))
			{
				// Program dir
				registerSearchPath(PHYSFS_getBaseDir(), 5);
				rebuildSearchPath(mod_multiplay, true);

				if (!PHYSFS_exists("gamedesc.lev"))
				{
					// Guessed fallback default datadir on Unix
					registerSearchPath(WZ_DATADIR, 6);
					rebuildSearchPath(mod_multiplay, true);
				}
			}
		}
	}

#ifdef WZ_OS_MAC
	if (!PHYSFS_exists("gamedesc.lev"))
	{
		CFURLRef resourceURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
		char resourcePath[PATH_MAX];
		if (CFURLGetFileSystemRepresentation(resourceURL, true,
		                                     (UInt8 *) resourcePath,
		                                     PATH_MAX))
		{
			chdir(resourcePath);
			registerSearchPath("data", 7);
			rebuildSearchPath(mod_multiplay, true);
		}
		else
		{
			debug(LOG_ERROR, "Could not change to resources directory.");
		}

		if (resourceURL != NULL)
		{
			CFRelease(resourceURL);
		}
	}
#endif

	/** Debugging and sanity checks **/

	printSearchPath();

	if (PHYSFS_exists("gamedesc.lev"))
	{
		debug(LOG_WZ, "gamedesc.lev found at %s", PHYSFS_getRealDir("gamedesc.lev"));
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
static void startTitleLoop()
{
	SetGameMode(GS_TITLE_SCREEN);

	initLoadingScreen(true);
	if (!frontendInitialise("wrf/frontend.wrf"))
	{
		debug(LOG_FATAL, "Shutting down after failure");
		exit(EXIT_FAILURE);
	}
	closeLoadingScreen();
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


/*!
 * Preparations before entering the game loop
 * Would start the timer in an event based mainloop
 */
static void startGameLoop()
{
	SetGameMode(GS_NORMAL);

	// Not sure what aLevelName is, in relation to game.map. But need to use aLevelName here, to be able to start the right map for campaign, and need game.hash, to start the right non-campaign map, if there are multiple identically named maps.
	if (!levLoadData(aLevelName, &game.hash, nullptr, GTYPE_SCENARIO_START))
	{
		debug(LOG_FATAL, "Shutting down after failure");
		exit(EXIT_FAILURE);
	}

	screen_StopBackDrop();

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
	if (game.type == SKIRMISH)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_START_NEXT_LEVEL);
	}
	triggerEvent(TRIGGER_START_LEVEL);
	screen_disableMapPreview();
}


/*!
 * Shutdown/cleanup after the game loop
 * Would stop the timer
 */
static void stopGameLoop()
{
	clearInfoMessages(); // clear CONPRINTF messages before each new game/mission
	if (gameLoopStatus != GAMECODE_NEWLEVEL)
	{
		clearBlueprints();
		initLoadingScreen(true); // returning to f.e. do a loader.render not active
		pie_EnableFog(false); // don't let the normal loop code set status on
		if (gameLoopStatus != GAMECODE_LOADGAME)
		{
			if (!levReleaseAll())
			{
				debug(LOG_ERROR, "levReleaseAll failed!");
			}
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
	if (!wzIsFullscreen())
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
	screen_RestartBackDrop();
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
	gameLoopStatus = gameLoop();
	switch (gameLoopStatus)
	{
	case GAMECODE_CONTINUE:
	case GAMECODE_PLAYVIDEO:
		break;
	case GAMECODE_QUITGAME:
		debug(LOG_MAIN, "GAMECODE_QUITGAME");
		stopGameLoop();
		startTitleLoop(); // Restart into titleloop
		break;
	case GAMECODE_LOADGAME:
		debug(LOG_MAIN, "GAMECODE_LOADGAME");
		stopGameLoop();
		initSaveGameLoad(); // Restart and load a savegame
		break;
	case GAMECODE_NEWLEVEL:
		debug(LOG_MAIN, "GAMECODE_NEWLEVEL");
		stopGameLoop();
		startGameLoop(); // Restart gameloop
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


/*!
 * Run the code inside the titleloop
 */
static void runTitleLoop()
{
	switch (titleLoop())
	{
	case TITLECODE_CONTINUE:
		break;
	case TITLECODE_QUITGAME:
		debug(LOG_MAIN, "TITLECODE_QUITGAME");
		stopTitleLoop();
		wzQuit();
		break;
	case TITLECODE_SAVEGAMELOAD:
		{
			debug(LOG_MAIN, "TITLECODE_SAVEGAMELOAD");
			initLoadingScreen(true);
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
			break;
		}
	case TITLECODE_STARTGAME:
		debug(LOG_MAIN, "TITLECODE_STARTGAME");
		initLoadingScreen(true);
		stopTitleLoop();
		startGameLoop(); // Restart into gameloop
		closeLoadingScreen();
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

	// Screenshot key is now available globally
	if (keyPressed(KEY_F10))
	{
		kf_ScreenDump();
		inputLoseFocus();		// remove it from input stream
	}

	if (NetPlay.bComms || focusState == FOCUS_IN || !war_GetPauseOnFocusLoss())
	{
		if (loop_GetVideoStatus())
		{
			videoLoop(); // Display the video if necessary
		}
		else switch (GetGameMode())
			{
			case GS_NORMAL: // Run the gameloop code
				runGameLoop();
				break;
			case GS_TITLE_SCREEN: // Run the titleloop code
				runTitleLoop();
				break;
			default:
				break;
			}
		realTimeUpdate(); // Update realTime.
	}
}

bool getUTF8CmdLine(int *const utfargc WZ_DECL_UNUSED, const char *** const utfargv WZ_DECL_UNUSED) // explicitely pass by reference
{
#ifdef WZ_OS_WIN
	int wargc;
	wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);

	if (wargv == NULL)
	{
		const int err = GetLastError();
		char *err_string;

		// Retrieve a (locally encoded) string describing the error (uses LocalAlloc() to allocate memory)
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, (char *)&err_string, 0, NULL);

		debug(LOG_FATAL, "CommandLineToArgvW failed: %s (code:%d)", err_string, err);

		LocalFree(err_string); // Free the chunk of memory FormatMessageA gave us
		LocalFree(wargv);
		return false;
	}
	// the following malloc and UTF16toUTF8 will be cleaned up in realmain().
	*utfargv = (const char **)malloc(sizeof(const char *) * wargc);
	if (!*utfargv)
	{
		debug(LOG_FATAL, "Out of memory!");
		abort();
		return false;
	}
	for (int i = 0; i < wargc; ++i)
	{
		STATIC_ASSERT(sizeof(wchar_t) == sizeof(utf_16_char)); // Should be true on windows
		(*utfargv)[i] = UTF16toUTF8((const utf_16_char *)wargv[i], NULL); // only returns null when memory runs out
		if ((*utfargv)[i] == NULL)
		{
			*utfargc = i;
			LocalFree(wargv);
			abort();
			return false;
		}
	}
	*utfargc = wargc;
	LocalFree(wargv);
#endif
	return true;
}

// for backend detection
extern const char *BACKEND;

int realmain(int argc, char *argv[])
{
	int utfargc = argc;
	const char **utfargv = (const char **)argv;
	wzMain(argc, argv);		// init Qt integration first

#ifdef WZ_OS_MAC
	cocoaInit();
#endif

	debug_init();
	debug_register_callback(debug_callback_stderr, nullptr, nullptr, nullptr);
#if defined(WZ_OS_WIN) && defined(DEBUG_INSANE)
	debug_register_callback(debug_callback_win32debug, NULL, NULL, NULL);
#endif // WZ_OS_WIN && DEBUG_INSANE

	// *****
	// NOTE: Try *NOT* to use debug() output routines without some other method of informing the user.  All this output is sent to /dev/nul at this point on some platforms!
	// *****
	if (!getUTF8CmdLine(&utfargc, &utfargv))
	{
		return EXIT_FAILURE;
	}

	setupExceptionHandler(utfargc, utfargv, version_getFormattedVersionString());

	/*** Initialize PhysicsFS ***/
	initialize_PhysicsFS(utfargv[0]);

	/*** Initialize translations ***/
	initI18n();

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

	PHYSFS_mkdir("challenges");	// custom challenges

	PHYSFS_mkdir("logs");		// netplay, mingw crash reports & WZ logs

	make_dir(MultiCustomMapsPath, "maps", nullptr); // needed to prevent crashes when getting map

	PHYSFS_mkdir("mods/autoload");	// mods that are automatically loaded
	PHYSFS_mkdir("mods/campaign");	// campaign only mods activated with --mod_ca=example.wz
	PHYSFS_mkdir("mods/downloads");	// mod download directory
	PHYSFS_mkdir("mods/global");	// global mods activated with --mod=example.wz
	PHYSFS_mkdir("mods/multiplay");	// multiplay only mods activated with --mod_mp=example.wz
	PHYSFS_mkdir("mods/music");	// music mods that are automatically loaded

	make_dir(MultiPlayersPath, "multiplay", "players"); // player profiles

	PHYSFS_mkdir("music");	// custom music overriding default music and music mods

	make_dir(SaveGamePath, "savegames", nullptr); 	// save games
	PHYSFS_mkdir("savegames/campaign");		// campaign save games
	PHYSFS_mkdir("savegames/skirmish");		// skirmish save games

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
		time_t aclock;
		struct tm *newtime;
		char buf[PATH_MAX];

		time(&aclock);					// Get time in seconds
		newtime = localtime(&aclock);		// Convert time to struct
		// Note: We are using fopen(), and not physfs routines to open the file
		// log name is logs/(or \)WZlog-MMDD_HHMMSS.txt
		snprintf(buf, sizeof(buf), "%slogs%sWZlog-%02d%02d_%02d%02d%02d.txt", PHYSFS_getWriteDir(), PHYSFS_getDirSeparator(),
		         newtime->tm_mon + 1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
		debug_register_callback(debug_callback_file, debug_callback_file_init, debug_callback_file_exit, buf);

		debug(LOG_WZ, "Using %s debug file", buf);
	}

	// NOTE: it is now safe to use debug() calls to make sure output gets captured.
	check_Physfs();
	debug(LOG_WZ, "Warzone 2100 - %s", version_getFormattedVersionString());
	debug(LOG_WZ, "Using language: %s", getLanguage());
	debug(LOG_WZ, "Backend: %s", BACKEND);
	debug(LOG_MEMORY, "sizeof: SIMPLE_OBJECT=%ld, BASE_OBJECT=%ld, DROID=%ld, STRUCTURE=%ld, FEATURE=%ld, PROJECTILE=%ld",
	      (long)sizeof(SIMPLE_OBJECT), (long)sizeof(BASE_OBJECT), (long)sizeof(DROID), (long)sizeof(STRUCTURE), (long)sizeof(FEATURE), (long)sizeof(PROJECTILE));


	/* Put in the writedir root */
	sstrcpy(KeyMapPath, "keymap.json");

	// initialise all the command line states
	war_SetDefaultStates();

	debug(LOG_MAIN, "initializing");

	loadConfig();

	// parse the command line
	if (!ParseCommandLine(utfargc, utfargv))
	{
		return EXIT_FAILURE;
	}

	// Save new (commandline) settings
	saveConfig();

	// Find out where to find the data
	scanDataDirs();

	// Now we check the mods to see if they exist or not (specified on the command line)
	// FIX ME: I know this is a bit hackish, but better than nothing for now?
	{
		char modtocheck[256];
		int result = 0;

		// check global mods
		for (auto const &modname : global_mods)
		{
			ssprintf(modtocheck, "mods/global/%s", modname.c_str());
			result = PHYSFS_exists(modtocheck);
			result |= WZ_PHYSFS_isDirectory(modtocheck);
			if (!result)
			{
				debug(LOG_ERROR, "The (global) mod (%s) you have specified doesn't exist!", modtocheck);
			}
			else
			{
				info("(global) mod (%s) is enabled", modname.c_str());
			}
		}
		// check campaign mods
		for (auto const &modname : campaign_mods)
		{
			ssprintf(modtocheck, "mods/campaign/%s", modname.c_str());
			result = PHYSFS_exists(modtocheck);
			result |= WZ_PHYSFS_isDirectory(modtocheck);
			if (!result)
			{
				debug(LOG_ERROR, "The mod_ca (%s) you have specified doesn't exist!", modtocheck);
			}
			else
			{
				info("mod_ca (%s) is enabled", modname.c_str());
			}
		}
		// check multiplay mods
		for (auto const &modname : multiplay_mods)
		{
			ssprintf(modtocheck, "mods/multiplay/%s", modname.c_str());
			result = PHYSFS_exists(modtocheck);
			result |= WZ_PHYSFS_isDirectory(modtocheck);
			if (!result)
			{
				debug(LOG_ERROR, "The mod_mp (%s) you have specified doesn't exist!", modtocheck);
			}
			else
			{
				info("mod_mp (%s) is enabled", modname.c_str());
			}
		}
	}

	if (!wzMainScreenSetup(war_getAntialiasing(), war_getFullscreen(), war_GetVsync()))
	{
		return EXIT_FAILURE;
	}

	debug(LOG_WZ, "Warzone 2100 - %s", version_getFormattedVersionString());
	debug(LOG_WZ, "Using language: %s", getLanguage());
	debug(LOG_WZ, "Backend: %s", BACKEND);
	debug(LOG_MEMORY, "sizeof: SIMPLE_OBJECT=%ld, BASE_OBJECT=%ld, DROID=%ld, STRUCTURE=%ld, FEATURE=%ld, PROJECTILE=%ld",
	      (long)sizeof(SIMPLE_OBJECT), (long)sizeof(BASE_OBJECT), (long)sizeof(DROID), (long)sizeof(STRUCTURE), (long)sizeof(FEATURE), (long)sizeof(PROJECTILE));

	int w = pie_GetVideoBufferWidth();
	int h = pie_GetVideoBufferHeight();

	char buf[256];
	ssprintf(buf, "Video Mode %d x %d (%s)", w, h, war_getFullscreen() ? "fullscreen" : "window");
	addDumpInfo(buf);

	float horizScaleFactor, vertScaleFactor;
	wzGetGameToRendererScaleFactor(&horizScaleFactor, &vertScaleFactor);
	debug(LOG_WZ, "Game to Renderer Scale Factor (w x h): %f x %f", horizScaleFactor, vertScaleFactor);

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
	unsigned int windowWidth = 0, windowHeight = 0;
	wzGetWindowResolution(nullptr, &windowWidth, &windowHeight);
	war_SetWidth(windowWidth);
	war_SetHeight(windowHeight);

	pie_SetFogStatus(false);
	pie_ScreenFlip(CLEAR_BLACK);

	pal_Init();

	pie_LoadBackDrop(SCREEN_RANDOMBDROP);
	pie_SetFogStatus(false);
	pie_ScreenFlip(CLEAR_BLACK);

	if (!systemInitialise(horizScaleFactor, vertScaleFactor))
	{
		return EXIT_FAILURE;
	}

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
		startTitleLoop();
		break;
	case GS_SAVEGAMELOAD:
		initSaveGameLoad();
		break;
	case GS_NORMAL:
		startGameLoop();
		break;
	default:
		debug(LOG_ERROR, "Weirdy game status, I'm afraid!!");
		break;
	}

#if defined(WZ_CC_MSVC) && defined(DEBUG)
	debug_MEMSTATS();
#endif
	debug(LOG_MAIN, "Entering main loop");
#ifndef WZ_OS_MAC
	wzMainEventLoop();
#else // WZ_OS_MAC
	// On Mac, use special method of launching NSApplication properly
	cocoaRunApplication(&wzMainEventLoop);
#endif
	saveConfig();
	systemShutdown();
#ifdef WZ_OS_WIN	// clean up the memory allocated for the command line conversion
	for (int i = 0; i < argc; i++)
	{
		const char *** const utfargvF = &utfargv;
		free((void *)(*utfargvF)[i]);
	}
	free(utfargv);
#endif
	wzShutdown();
	debug(LOG_MAIN, "Completed shutting down Warzone 2100");
	return EXIT_SUCCESS;
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
