/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
 * @file main.c
 *
 * The main file that launches the game and starts up everything.
 */

/** @mainpage Warzone2100 Code Documentation
 *
 * @section intro_sec Introduction
 *
 * Welcome to the Warzone2100 Resurrection Project's code library! We are still only starting
 * to document this code base, but it should already be useful for quickly browsing around the
 * code and getting an idea of what exists and how things work.
 *
 * @section install_sec Where to begin
 *
 * @subsection step1 Step 1: Read the wiki documentation
 *
 * We have a great deal of documentation on our wiki page at http://wiki.wz2100.net/ which
 * may help get you started. Please also take notice of our style guide!
 *
 * @subsection step2 Step 2: Skim main.c
 *
 * You may want to begin reading code with main.c, since this is where execution begins,
 * and trace code paths from here.
 *
 * Have fun!
 */

// Get platform defines before checking for them!
#include "lib/framework/frame.h"

#include <SDL.h>

#if defined(WZ_OS_WIN)
// FIXME HACK Workaround DATADIR definition in objbase.h
// This works since DATADIR is never used on Windows.
#  undef DATADIR
#  include <shlobj.h> /* For SHGetFolderPath */
#elif defined(WZ_OS_UNIX)
#  include <errno.h>
#endif // WZ_OS_WIN

#include "lib/framework/configfile.h"
#include "lib/framework/input.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/tagfile.h"

#include "lib/gamelib/gtime.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"
#include "lib/script/script.h"
#include "lib/sound/audio.h"
#include "lib/sound/cdaudio.h"
#include "lib/sqlite3/physfs_vfs.h"
#include "lib/widget/widget.h"

#include "clparse.h"
#include "configuration.h"
#include "display.h"
#include "frontend.h"
#include "game.h"
#include "init.h"
#include "levels.h"
#include "lighting.h"
#include "loadsave.h"
#include "loop.h"
#include "modding.h"
#include "multiplay.h"
#include "research.h"
#include "seqdisp.h"
#include "warzoneconfig.h"
#include "main.h"
#include "wrappers.h"
#include "version.h"


/* Always use fallbacks on Windows */
#if defined(WZ_OS_WIN)
#  undef DATADIR
#endif

#if !defined(DATADIR)
#  define DATADIR "data"
#endif


#if defined(WZ_OS_WIN)
# define WZ_WRITEDIR "Warzone 2100 2.1"
#elif defined(WZ_OS_MAC)
# include <CoreServices/CoreServices.h>
# include <unistd.h>
# define WZ_WRITEDIR "Warzone 2100 2.1"
#else
# define WZ_WRITEDIR ".warzone2100-2.1"
#endif


char datadir[PATH_MAX] = "\0"; // Global that src/clparse.c:ParseCommandLine can write to, so it can override the default datadir on runtime. Needs to be \0 on startup for ParseCommandLine to work!
char configdir[PATH_MAX] = "\0"; // specifies custom USER directory.  Same rules apply as datadir above.

char * global_mods[MAX_MODS] = { NULL };
char * campaign_mods[MAX_MODS] = { NULL };
char * multiplay_mods[MAX_MODS] = { NULL };


// Warzone 2100 . Pumpkin Studios

//flag to indicate when initialisation is complete
BOOL	gameInitialised = FALSE;
char	SaveGamePath[PATH_MAX];
char	ScreenDumpPath[PATH_MAX];
char	MultiForcesPath[PATH_MAX];
char	MultiCustomMapsPath[PATH_MAX];
char	MultiPlayersPath[PATH_MAX];
char	KeyMapPath[PATH_MAX];
char	UserMusicPath[PATH_MAX];

// Start game in title mode:
static GS_GAMEMODE gameStatus = GS_TITLE_SCREEN;
// Status of the gameloop
static int gameLoopStatus = 0;
extern FOCUS_STATE focusState;

extern void debug_callback_stderr( void**, const char * );
extern void debug_callback_win32debug( void**, const char * );

static BOOL inList( char * list[], const char * item )
{
	int i = 0;
#ifdef DEBUG
	debug( LOG_NEVER, "inList: Current item: [%s]", item );
#endif
	while( list[i] != NULL )
	{
#ifdef DEBUG
		debug( LOG_NEVER, "inList: Checking for match with: [%s]", list[i] );
#endif
		if ( strcmp( list[i], item ) == 0 )
			return TRUE;
		i++;
	}
	return FALSE;
}

void addSubdirs( const char * basedir, const char * subdir, const BOOL appendToPath, char * checkList[] )
{
	char tmpstr[PATH_MAX];
	char ** subdirlist = PHYSFS_enumerateFiles( subdir );
	char ** i = subdirlist;
	while( *i != NULL )
	{
#ifdef DEBUG
		debug( LOG_NEVER, "addSubdirs: Examining subdir: [%s]", *i );
#endif // DEBUG
		if( !checkList || inList( checkList, *i ) )
		{
			snprintf(tmpstr, sizeof(tmpstr), "%s%s%s%s", basedir, subdir, PHYSFS_getDirSeparator(), *i);
#ifdef DEBUG
			debug( LOG_NEVER, "addSubdirs: Adding [%s] to search path", tmpstr );
#endif // DEBUG
			PHYSFS_addToSearchPath( tmpstr, appendToPath );
		}
		i++;
	}
	PHYSFS_freeList( subdirlist );
}

void removeSubdirs( const char * basedir, const char * subdir, char * checkList[] )
{
	char tmpstr[PATH_MAX];
	char ** subdirlist = PHYSFS_enumerateFiles( subdir );
	char ** i = subdirlist;
	while( *i != NULL )
	{
#ifdef DEBUG
		debug( LOG_NEVER, "removeSubdirs: Examining subdir: [%s]", *i );
#endif // DEBUG
		if( !checkList || inList( checkList, *i ) )
		{
			snprintf(tmpstr, sizeof(tmpstr), "%s%s%s%s", basedir, subdir, PHYSFS_getDirSeparator(), *i);
#ifdef DEBUG
			debug( LOG_NEVER, "removeSubdirs: Removing [%s] from search path", tmpstr );
#endif // DEBUG
			PHYSFS_removeFromSearchPath( tmpstr );
		}
		i++;
	}
	PHYSFS_freeList( subdirlist );
}

void printSearchPath( void )
{
	char ** i, ** searchPath;

	debug(LOG_WZ, "Search paths:");
	searchPath = PHYSFS_getSearchPath();
	for (i = searchPath; *i != NULL; i++) {
		debug(LOG_WZ, "    [%s]", *i);
	}
	PHYSFS_freeList( searchPath );
}

/** Retrieves the current working directory and copies it into the provided output buffer
 *  \param[out] dest the output buffer to put the current working directory in
 *  \param size the size (in bytes) of \c dest
 *  \return true on success, false if an error occurred (and dest doesn't contain a valid directory)
 */
static bool getCurrentDir(char * const dest, size_t const size)
{
#if defined(WZ_OS_UNIX)
	if (getcwd(dest, size) == NULL)
	{
		if (errno == ERANGE)
		{
			debug(LOG_ERROR, "getPlatformUserDir: The buffer to contain our current directory is too small (%u bytes and more needed)", (unsigned int)size);
		}
		else
		{
			debug(LOG_ERROR, "getPlatformUserDir: getcwd failed: %s", strerror(errno));
		}

		return false;
	}
#elif defined(WZ_OS_WIN)
	const int len = GetCurrentDirectoryA(size, dest);

	if (len == 0)
	{
		// Retrieve Windows' error number
		const int err = GetLastError();
		char* err_string;

		// Retrieve a string describing the error number (uses LocalAlloc() to allocate memory for err_string)
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, (char*)&err_string, 0, NULL);

		// Print an error message with the above description
		debug(LOG_ERROR, "getPlatformUserDir: GetCurrentDirectoryA failed (error code: %d): %s", err, err_string);

		// Free our chunk of memory FormatMessageA gave us
		LocalFree(err_string);

		return false;
	}
	else if (len > size)
	{
		debug(LOG_ERROR, "getPlatformUserDir: The buffer to contain our current directory is too small (%u bytes and %d needed)", (unsigned int)size, len);

		return false;
	}
#else
# error "Provide an implementation here to copy the current working directory in 'dest', which is 'size' bytes large."
#endif

	// If we got here everything went well
	return true;
}

static void getPlatformUserDir(char * const tmpstr, size_t const size)
{
#if defined(WZ_OS_WIN)
	ASSERT(size >= MAX_PATH, "size (%u) is smaller than the required minimum of MAX_PATH (%u)", size, (size_t)MAX_PATH);
	if ( SUCCEEDED( SHGetFolderPathA( NULL, CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, tmpstr ) ) )
		strlcat(tmpstr, PHYSFS_getDirSeparator(), size);
	else
#elif defined(WZ_OS_MAC)
	short vol_ref;
	long dir_id;
	FSSpec fsspec;
	FSRef fsref;
	OSErr error = FindFolder(kUserDomain, kApplicationSupportFolderType, FALSE, &vol_ref, &dir_id);
	if (!error)
		error = FSMakeFSSpec(vol_ref, dir_id, (const unsigned char *) "", &fsspec);
	if (!error)
		error = FSpMakeFSRef(&fsspec, &fsref);
	if (!error)
		error = FSRefMakePath(&fsref, (UInt8 *) tmpstr, size);
	if (!error)
		strlcat(tmpstr, PHYSFS_getDirSeparator(), size);
	else
#endif
	if (PHYSFS_getUserDir())
		strlcpy(tmpstr, PHYSFS_getUserDir(), size); // Use PhysFS supplied UserDir (As fallback on Windows / Mac, default on Linux)
	// If PhysicsFS fails (might happen if environment variable HOME is unset or wrong) then use the current working directory
	else if (getCurrentDir(tmpstr, size))
		strlcat(tmpstr, PHYSFS_getDirSeparator(), size);
	else
		abort();
}

static void initialize_ConfigDir(void)
{
	char tmpstr[PATH_MAX] = { '\0' };

	if (strlen(configdir) == 0)
	{
		getPlatformUserDir(tmpstr, sizeof(tmpstr));

		if (!PHYSFS_setWriteDir(tmpstr)) // Workaround for PhysFS not creating the writedir as expected.
		{
			debug(LOG_ERROR, "Error setting write directory to \"%s\": %s",
			      tmpstr, PHYSFS_getLastError());
			exit(1);
		}

		if (!PHYSFS_mkdir(WZ_WRITEDIR)) // s.a.
		{
			debug(LOG_ERROR, "Error creating directory \"%s\": %s",
			      WZ_WRITEDIR, PHYSFS_getLastError());
			exit(1);
		}

		// Append the Warzone subdir
		strlcat(tmpstr, WZ_WRITEDIR, sizeof(tmpstr));
		strlcat(tmpstr, PHYSFS_getDirSeparator(), sizeof(tmpstr));

		if (!PHYSFS_setWriteDir(tmpstr))
		{
			debug( LOG_ERROR, "Error setting write directory to \"%s\": %s",
			tmpstr, PHYSFS_getLastError() );
			exit(1);
		}
	}
	else
	{
		strlcpy(tmpstr, configdir, sizeof(tmpstr));

		// Make sure that we have a directory separator at the end of the string
		if (tmpstr[strlen(tmpstr) - 1] != PHYSFS_getDirSeparator()[0])
			strlcat(tmpstr, PHYSFS_getDirSeparator(), sizeof(tmpstr));

		debug(LOG_WZ, "Using custom configuration directory: %s", tmpstr);

		if (!PHYSFS_setWriteDir(tmpstr)) // Workaround for PhysFS not creating the writedir as expected.
		{
			debug(LOG_ERROR, "Error setting write directory to \"%s\": %s",
			      tmpstr, PHYSFS_getLastError());
			exit(1);
		}
	}

	// User's home dir first so we allways see what we write
	PHYSFS_addToSearchPath( PHYSFS_getWriteDir(), PHYSFS_PREPEND );

	PHYSFS_permitSymbolicLinks(1);

	debug(LOG_WZ, "Write dir: %s", PHYSFS_getWriteDir());
	debug(LOG_WZ, "Base dir: %s", PHYSFS_getBaseDir());
}

/***************************************************************************
	Initialize the PhysicsFS library.
***************************************************************************/
static void initialize_PhysicsFS(const char* argv_0)
{
	PHYSFS_Version compiled;
	PHYSFS_Version linked;

	PHYSFS_init(argv_0);

	PHYSFS_VERSION(&compiled);
	PHYSFS_getLinkedVersion(&linked);

	debug(LOG_WZ, "Compiled against PhysFS version: %d.%d.%d",
	      compiled.major, compiled.minor, compiled.patch);
	debug(LOG_WZ, "Linked against PhysFS version: %d.%d.%d",
	      linked.major, linked.minor, linked.patch);
}

/*
 * \fn void scanDataDirs( void )
 * \brief Adds default data dirs
 *
 * Priority:
 * Lower loads first. Current:
 * -datadir > User's home dir > SVN data > AutoPackage > BaseDir > DEFAULT_DATADIR
 *
 * Only -datadir and home dir are allways examined. Others only if data still not found.
 *
 * We need ParseCommandLine, before we can add any mods...
 *
 * \sa rebuildSearchPath
 */
static void scanDataDirs( void )
{
	char tmpstr[PATH_MAX], prefix[PATH_MAX];
	char* separator;

	// Find out which PREFIX we are in...
	strlcpy(prefix, PHYSFS_getBaseDir(), sizeof(prefix));

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

	atexit( cleanSearchPath );

	// Commandline supplied datadir
	if( strlen( datadir ) != 0 )
		registerSearchPath( datadir, 1 );

	// User's home dir
	registerSearchPath( PHYSFS_getWriteDir(), 2 );
	rebuildSearchPath( mod_multiplay, TRUE );

	if( !PHYSFS_exists("gamedesc.lev") )
	{
		// Data in SVN dir
		strlcpy(tmpstr, prefix, sizeof(tmpstr));
		strlcat(tmpstr, "/data/", sizeof(tmpstr));
		registerSearchPath( tmpstr, 3 );
		rebuildSearchPath( mod_multiplay, TRUE );

		if( !PHYSFS_exists("gamedesc.lev") )
		{
			// Relocation for AutoPackage
			strlcpy(tmpstr, prefix, sizeof(tmpstr));
			strlcat(tmpstr, "/share/warzone2100/", sizeof(tmpstr));
			registerSearchPath( tmpstr, 4 );
			rebuildSearchPath( mod_multiplay, TRUE );

			if( !PHYSFS_exists("gamedesc.lev") )
			{
				// Program dir
				registerSearchPath( PHYSFS_getBaseDir(), 5 );
				rebuildSearchPath( mod_multiplay, TRUE );

				if( !PHYSFS_exists("gamedesc.lev") )
				{
					// Guessed fallback default datadir on Unix
					registerSearchPath( DATADIR, 6 );
					rebuildSearchPath( mod_multiplay, TRUE );
				}
			}
		}
	}

#ifdef WZ_OS_MAC
	if( !PHYSFS_exists("gamedesc.lev") ) {
		CFURLRef resourceURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
		char resourcePath[PATH_MAX];
		if( CFURLGetFileSystemRepresentation( resourceURL, true,
							(UInt8 *) resourcePath,
							PATH_MAX) ) {
			chdir( resourcePath );
			registerSearchPath( "data", 7 );
			rebuildSearchPath( mod_multiplay, TRUE );
		} else {
			debug( LOG_ERROR, "Could not change to resources directory." );
		}
	}
#endif

	/** Debugging and sanity checks **/

	printSearchPath();

	if( PHYSFS_exists("gamedesc.lev") )
	{
		debug( LOG_WZ, "gamedesc.lev found at %s", PHYSFS_getRealDir( "gamedesc.lev" ) );
	}
	else
	{
		debug( LOG_ERROR, "Could not find game data. Aborting." );
		exit(1);
	}
}

/***************************************************************************
	Make a directory in write path and set a variable to point to it.
***************************************************************************/
static void make_dir(char *dest, const char *dirname, const char *subdir)
{
	strcpy(dest, dirname);
	if (subdir != NULL) {
		strcat(dest, "/");
		strcat(dest, subdir);
	}
	{
		size_t l = strlen(dest);

		if (dest[l-1] != '/') {
			dest[l] = '/';
			dest[l+1] = '\0';
		}
	}
	PHYSFS_mkdir(dest);
	if ( !PHYSFS_mkdir(dest) ) {
		debug(LOG_ERROR, "Unable to create directory \"%s\" in write dir \"%s\"!",
		      dest, PHYSFS_getWriteDir());
		exit(EXIT_FAILURE);
	}
}


/*!
 * Preparations before entering the title (mainmenu) loop
 * Would start the timer in an event based mainloop
 */
static void startTitleLoop(void)
{
	SetGameMode(GS_TITLE_SCREEN);

	screen_RestartBackDrop();
	if (!frontendInitialise("wrf/frontend.wrf"))
	{
		debug( LOG_ERROR, "Shutting down after failure" );
		exit(EXIT_FAILURE);
	}
	frontendInitVars();
}


/*!
 * Shutdown/cleanup after the title (mainmenu) loop
 * Would stop the timer
 */
static void stopTitleLoop(void)
{
	if (!frontendShutdown())
	{
		debug( LOG_ERROR, "Shutting down after failure" );
		exit(EXIT_FAILURE);
	}
}


/*!
 * Preparations before entering the game loop
 * Would start the timer in an event based mainloop
 */
static void startGameLoop(void)
{
	SetGameMode(GS_NORMAL);

	if (!levLoadData(aLevelName, NULL, 0))
	{
		debug( LOG_ERROR, "Shutting down after failure" );
		exit(EXIT_FAILURE);
	}
	//after data is loaded check the research stats are valid
	if (!checkResearchStats())
	{
		debug( LOG_ERROR, "Invalid Research Stats" );
		debug( LOG_ERROR, "Shutting down after failure" );
		exit(EXIT_FAILURE);
	}
	//and check the structure stats are valid
	if (!checkStructureStats())
	{
		debug( LOG_ERROR, "Invalid Structure Stats" );
		debug( LOG_ERROR, "Shutting down after failure" );
		exit(EXIT_FAILURE);
	}

	screen_StopBackDrop();

	// Trap the cursor if cursor snapping is enabled
	if (war_GetTrapCursor())
	{
		SDL_WM_GrabInput(SDL_GRAB_ON);
	}

	// set a flag for the trigger/event system to indicate initialisation is complete
	gameInitialised = TRUE;
}


/*!
 * Shutdown/cleanup after the game loop
 * Would stop the timer
 */
static void stopGameLoop(void)
{
	if (gameLoopStatus != GAMECODE_NEWLEVEL)
	{
		initLoadingScreen(TRUE); // returning to f.e. do a loader.render not active
		pie_EnableFog(FALSE); // dont let the normal loop code set status on
		fogStatus = 0;
		if (gameLoopStatus != GAMECODE_LOADGAME)
		{
			levReleaseAll();
		}
	}

	// Disable cursor trapping
	if (war_GetTrapCursor())
	{
		SDL_WM_GrabInput(SDL_GRAB_OFF);
	}

	gameInitialised = FALSE;
}


/*!
 * Load a savegame and start into the game loop
 * Game data should be initialised afterwards, so that startGameLoop is not necessary anymore.
 */
static void initSaveGameLoad(void)
{
	SetGameMode(GS_NORMAL);

	screen_RestartBackDrop();
	// load up a save game
	if (!loadGameInit(saveGameName))
	{
		debug( LOG_ERROR, "Shutting down after failure" );
		exit(EXIT_FAILURE);
	}
	screen_StopBackDrop();

	// Trap the cursor if cursor snapping is enabled
	if (war_GetTrapCursor())
	{
		SDL_WM_GrabInput(SDL_GRAB_ON);
	}
}


/*!
 * Run the code inside the gameloop
 */
static void runGameLoop(void)
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
		// Never trown:
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
static void runTitleLoop(void)
{
	switch (titleLoop())
	{
		case TITLECODE_CONTINUE:
			break;
		case TITLECODE_QUITGAME:
			debug(LOG_MAIN, "TITLECODE_QUITGAME");
			stopTitleLoop();
			{
				// Create a quit event to halt game loop.
				SDL_Event quitEvent;
				quitEvent.type = SDL_QUIT;
				SDL_PushEvent(&quitEvent);
			}
			break;
		case TITLECODE_SAVEGAMELOAD:
			debug(LOG_MAIN, "TITLECODE_SAVEGAMELOAD");
			stopTitleLoop();
			initSaveGameLoad(); // Restart into gameloop and load a savegame
			break;
		case TITLECODE_STARTGAME:
			debug(LOG_MAIN, "TITLECODE_STARTGAME");
			stopTitleLoop();
			startGameLoop(); // Restart into gameloop
			break;
		case TITLECODE_SHOWINTRO:
			debug(LOG_MAIN, "TITLECODE_SHOWINTRO");
			seq_ClearSeqList();
			seq_AddSeqToList("eidos-logo.rpl", NULL, NULL, FALSE);
			seq_AddSeqToList("pumpkin.rpl", NULL, NULL, FALSE);
			seq_AddSeqToList("titles.rpl", NULL, NULL, FALSE);
			seq_AddSeqToList("devastation.rpl", NULL, "devastation.txa", FALSE);
			seq_StartNextFullScreenVideo();
			break;
		default:
			debug(LOG_ERROR, "Unknown code returned by titleLoop");
			break;
	}
}


/*!
 * Activation (focus change) eventhandler
 */
static void handleActiveEvent(SDL_ActiveEvent * activeEvent)
{
	// Ignore focus loss through SDL_APPMOUSEFOCUS, since it mostly happens accidentialy
	// active.state is a bitflag! Mixed events (eg. APPACTIVE|APPMOUSEFOCUS) will thus not be ignored.
	if ( activeEvent->state != SDL_APPMOUSEFOCUS )
	{
		if ( activeEvent->gain == 1 )
		{
			debug( LOG_NEVER, "WM_SETFOCUS\n");
			if (focusState != FOCUS_IN)
			{
				focusState = FOCUS_IN;

				gameTimeStart();
				// Should be: audio_ResumeAll();
			}

			// Resume playing audio.
			cdAudio_Resume();
		}
		else
		{
			debug( LOG_NEVER, "WM_KILLFOCUS\n");
			if (focusState != FOCUS_OUT)
			{
				focusState = FOCUS_OUT;

				gameTimeStop();
				// Should be: audio_PauseAll();
				audio_StopAll();
			}
			/* Have to tell the input system that we've lost focus */
			inputLooseFocus();

			// Need to pause playing to prevent the audio code from
			// thinking playing has finished.
			cdAudio_Pause();
		}
	}
}


/*!
 * The mainloop.
 * Fetches events, executes appropriate code
 */
static void mainLoop(void)
{
	SDL_Event event;

	while (TRUE)
	{
		frameUpdate(); // General housekeeping

		/* Deal with any windows messages */
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYUP:
				case SDL_KEYDOWN:
					inputHandleKeyEvent(&event.key);
					break;
				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEBUTTONDOWN:
					inputHandleMouseButtonEvent(&event.button);
					break;
				case SDL_MOUSEMOTION:
					inputHandleMouseMotionEvent(&event.motion);
					break;
				case SDL_ACTIVEEVENT:
					 // Ignore this event during multiplayer games since it breaks the game when one player suddenly pauses!
					if (!bMultiPlayer)
					{
						handleActiveEvent(&event.active);
					}
					break;
				case SDL_QUIT:
					return;
				default:
					break;
			}
		}

		if (focusState == FOCUS_IN)
		{
			if (loop_GetVideoStatus())
			{
				videoLoop(); // Display the video if neccessary
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

			gameTimeUpdate(); // Update gametime. FIXME There is probably code duplicated with MaintainFrameStuff
		}
	}
}


int main(int argc, char *argv[])
{
	setupExceptionHandler(argv[0]);

	debug_init();
	atexit( debug_exit );

	debug_register_callback( debug_callback_stderr, NULL, NULL, NULL );
#if defined(WZ_OS_WIN) && defined(DEBUG_INSANE)
	debug_register_callback( debug_callback_win32debug, NULL, NULL, NULL );
#endif // WZ_OS_WIN && DEBUG_INSANE

	/*** Initialize PhysicsFS ***/
	initialize_PhysicsFS(argv[0]);

	/*** Initialize translations ***/
	initI18n();

	// find early boot info
	if ( !ParseCommandLineEarly(argc, (const char**)argv) ) {
		return -1;
	}

	debug(LOG_WZ, "Using language: %s", getLanguage());

	/* Initialize the write/config directory for PhysicsFS.
	 * This needs to be done __after__ the early commandline parsing,
	 * because the user might tell us to use an alternative configuration
	 * directory.
	 */
	initialize_ConfigDir();

	debug(LOG_WZ, "Warzone 2100 - %s", version_getFormattedVersionString());

	/*** Initialize directory structure ***/
	make_dir(ScreenDumpPath, "screendumps", NULL);
	make_dir(SaveGamePath, "savegame", NULL);
	PHYSFS_mkdir("maps");		// MUST have this to prevent crashes when getting map
	PHYSFS_mkdir("music");
	make_dir(MultiPlayersPath, "multiplay", NULL);
	make_dir(MultiPlayersPath, "multiplay", "players");
	make_dir(MultiForcesPath, "multiplay", "forces");
	make_dir(MultiCustomMapsPath, "multiplay", "custommaps");

	/* Put these files in the writedir root */
	setRegistryFilePath("config");
	strlcpy(KeyMapPath, "keymap.map", sizeof(KeyMapPath));
	strlcpy(UserMusicPath, "music", sizeof(UserMusicPath));

	// initialise all the command line states
	war_SetDefaultStates();

	debug(LOG_MAIN, "initializing");

	loadConfig();
	atexit( closeConfig );
	loadRenderMode(); //get the registry entry for clRendMode

	// parse the command line
	if (!ParseCommandLine(argc, (const char**)argv)) {
		return -1;
	}

	// Save new (commandline) settings
	saveConfig();

	// Find out where to find the data
	scanDataDirs();

	// Register the PhysicsFS implementation of the SQLite VFS class with
	// SQLite's VFS system as the default (non-zero=default, zero=default).
	sqlite3_register_physfs_vfs(1);

#ifdef DEBUG
	/* Runtime unit testing */
	NETtest();
	tagTest();
#endif

	NETinit(TRUE);

	if (!frameInitialise( "Warzone 2100", pie_GetVideoBufferWidth(), pie_GetVideoBufferHeight(), pie_GetVideoBufferDepth(), war_getFullscreen() ))
	{
		return -1;
	}
	atexit(frameShutDown);

	pie_SetFogStatus(FALSE);
	pie_ScreenFlip(CLEAR_BLACK);

	pal_Init();
	atexit(pal_ShutDown);

	pie_LoadBackDrop(SCREEN_RANDOMBDROP);
	pie_SetFogStatus(FALSE);
	pie_ScreenFlip(CLEAR_BLACK);

	if (!systemInitialise())
	{
		return -1;
	}
	atexit(systemShutdown);

	//set all the pause states to false
	setAllPauseStates(FALSE);

	// Do the game mode specific initialisation.
	switch(GetGameMode())
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

	debug(LOG_MAIN, "Entering main loop");

	// Enter the mainloop
	mainLoop();

	debug(LOG_MAIN, "Shutting down Warzone 2100");

	return EXIT_SUCCESS;
}


/*!
 * Get the mode the game is currently in
 */
GS_GAMEMODE GetGameMode(void)
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
