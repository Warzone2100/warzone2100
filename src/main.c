/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
#include "lib/framework/frame.h"

#include <SDL.h>

#if defined(WZ_OS_WIN)
#  include <shlobj.h> /* For SHGetFolderPath */
#elif defined(WZ_OS_UNIX)
#  include <errno.h>
#endif // WZ_OS_WIN

#include "lib/framework/configfile.h"
#include "lib/framework/input.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/tagfile.h"
#include "lib/exceptionhandler/exceptionhandler.h"
#include "lib/exceptionhandler/dumpinfo.h"

#include "lib/sound/playlist.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_common/pieblitfunc.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/piemode.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"
#include "lib/script/script.h"
#include "lib/sound/audio.h"
#include "lib/sound/cdaudio.h"

#include "clparse.h"
#include "challenge.h"
#include "configuration.h"
#include "display.h"
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
#include "research.h"
#include "scripttabs.h"
#include "seqdisp.h"
#include "warzoneconfig.h"
#include "main.h"
#include "wrappers.h"
#include "version.h"
#include "map.h"
#include "parsetest.h"
#include "keybind.h"
#include <time.h>

/* Always use fallbacks on Windows */
#if defined(WZ_OS_WIN)
#  undef WZ_DATADIR
#endif

#if !defined(WZ_DATADIR)
#  define WZ_DATADIR "data"
#endif

typedef enum _focus_state
{
	FOCUS_OUT,		// Window does not have the focus
	FOCUS_IN,		// Window has got the focus
} FOCUS_STATE;

#if defined(WZ_OS_WIN)
# define WZ_WRITEDIR "Warzone 2100 master"
#elif defined(WZ_OS_MAC)
# include <CoreServices/CoreServices.h>
# include <unistd.h>
# include "cocoa_wrapper.h"
# define WZ_WRITEDIR "Warzone 2100 master"
#else
# define WZ_WRITEDIR ".warzone2100-master"
#endif

char datadir[PATH_MAX] = ""; // Global that src/clparse.c:ParseCommandLine can write to, so it can override the default datadir on runtime. Needs to be empty on startup for ParseCommandLine to work!
char configdir[PATH_MAX] = ""; // specifies custom USER directory. Same rules apply as datadir above.

char * global_mods[MAX_MODS] = { NULL };
char * campaign_mods[MAX_MODS] = { NULL };
char * multiplay_mods[MAX_MODS] = { NULL };

char * override_mods[MAX_MODS] = { NULL };
char * override_mod_list = NULL;
bool use_override_mods = false;

char * loaded_mods[MAX_MODS] = { NULL };
char * mod_list = NULL;
bool customDebugfile = false;		// Default false: user has NOT specified where to store the stdout/err file.
int num_loaded_mods = 0;


// Warzone 2100 . Pumpkin Studios

//flag to indicate when initialisation is complete
BOOL	gameInitialised = false;
char	SaveGamePath[PATH_MAX];
char	ScreenDumpPath[PATH_MAX];
char	MultiForcesPath[PATH_MAX];
char	MultiCustomMapsPath[PATH_MAX];
char	MultiPlayersPath[PATH_MAX];
char	KeyMapPath[PATH_MAX];

// Start game in title mode:
static GS_GAMEMODE gameStatus = GS_TITLE_SCREEN;
// Status of the gameloop
static int gameLoopStatus = 0;
static FOCUS_STATE focusState = FOCUS_IN;

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
			return true;
		i++;
	}
	return false;
}


/*!
 * Tries to mount a list of directories, found in /basedir/subdir/<list>.
 * \param basedir Base directory
 * \param subdir A subdirectory of basedir
 * \param appendToPath Whether to append or prepend
 * \param checkList List of directories to check. NULL means any.
 */
void addSubdirs( const char * basedir, const char * subdir, const bool appendToPath, char * checkList[], bool addToModList )
{
	char tmpstr[PATH_MAX];
	char buf[256];
	char ** subdirlist = PHYSFS_enumerateFiles( subdir );
	char ** i = subdirlist;

	while( *i != NULL )
	{
#ifdef DEBUG
		debug( LOG_NEVER, "addSubdirs: Examining subdir: [%s]", *i );
#endif // DEBUG
		if (*i[0] != '.' && (!checkList || inList(checkList, *i)))
		{
			snprintf(tmpstr, sizeof(tmpstr), "%s%s%s%s", basedir, subdir, PHYSFS_getDirSeparator(), *i);
#ifdef DEBUG
			debug( LOG_NEVER, "addSubdirs: Adding [%s] to search path", tmpstr );
#endif // DEBUG
			if (addToModList)
			{
				addLoadedMod(*i);
				snprintf(buf, sizeof(buf), "mod: %s", *i);
				addDumpInfo(buf);
			}
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
			if (!PHYSFS_removeFromSearchPath( tmpstr ))
			{
#ifdef DEBUG	// spams a ton!
				debug(LOG_NEVER, "Couldn't remove %s from search path because %s", tmpstr, PHYSFS_getLastError());
#endif // DEBUG
			}
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

void setOverrideMods(char * modlist)
{
	char * curmod = modlist;
	char * nextmod;
	int i=0;
	while ((nextmod = strstr(curmod, ", ")) && i<MAX_MODS-2)
	{
		override_mods[i] = malloc(nextmod-curmod+1);
		strlcpy(override_mods[i], curmod, nextmod-curmod);
		override_mods[i][nextmod-curmod] = '\0';
		curmod = nextmod + 2;
		i++;
	}
	override_mods[i] = strdup(curmod);
	override_mods[i+1] = NULL;
	override_mod_list = modlist;
	use_override_mods = true;
}
void clearOverrideMods(void)
{
	int i;
	use_override_mods = false;
	for (i=0; i<MAX_MODS && override_mods[i]; i++)
	{
		free(override_mods[i]);
	}
	override_mods[0] = NULL;
	override_mod_list = NULL;
}

void addLoadedMod(const char * modname)
{
	char * mod = strdup(modname);
	int i, modlen;
	if (num_loaded_mods >= MAX_MODS)
	{
		// mod list full
		return;
	}
	modlen = strlen(mod);
	if (modlen >= 3 && strcmp(&mod[modlen-3], ".wz")==0)
	{
		// remove ".wz" from end
		mod[modlen-3] = 0;
		modlen -= 3;
	}
	if (modlen >= 4 && strcmp(&mod[modlen-4], ".cam")==0)
	{
		// remove ".cam.wz" from end
		mod[modlen-4] = 0;
		modlen -= 4;
	}
	else if (modlen >= 4 && strcmp(&mod[modlen-4], ".mod")==0)
	{
		// remove ".mod.wz" from end
		mod[modlen-4] = 0;
		modlen -= 4;
	}
	else if (modlen >= 5 && strcmp(&mod[modlen-5], ".gmod")==0)
	{
		// remove ".gmod.wz" from end
		mod[modlen-5] = 0;
		modlen -= 5;
	}
	// Yes, this is an online insertion sort.
	// I swear, for the numbers of mods this is going to be dealing with
	// (i.e. 0 to 2), it really is faster than, say, Quicksort.
	for (i=0; i<num_loaded_mods && strcmp(loaded_mods[i], mod)>0; i++);
	if (i < num_loaded_mods)
	{
		if (strcmp(loaded_mods[i], mod) == 0)
		{
			// mod already in list
			free(mod);
			return;
		}
		memmove(&loaded_mods[i+1], &loaded_mods[i], (num_loaded_mods-i)*sizeof(char*));
	}
	loaded_mods[i] = mod;
	num_loaded_mods++;
}
void clearLoadedMods(void)
{
	int i;
	for (i=0; i<num_loaded_mods; i++)
	{
		free(loaded_mods[i]);
	}
	num_loaded_mods = 0;
	if (mod_list)
	{
		free(mod_list);
		mod_list = NULL;
	}
}
char * getModList(void)
{
	int i;
	if (mod_list)
	{
		// mod list already constructed
		return mod_list;
	}
	mod_list = malloc(modlist_string_size);
	mod_list[0] = 0; //initialize
	for (i=0; i<num_loaded_mods; i++)
	{
		if (i != 0)
		{
			strlcat(mod_list, ", ", modlist_string_size);
		}
		strlcat(mod_list, loaded_mods[i], modlist_string_size);
	}
	return mod_list;
}


/*!
 * Retrieves the current working directory and copies it into the provided output buffer
 * \param[out] dest the output buffer to put the current working directory in
 * \param size the size (in bytes) of \c dest
 * \return true on success, false if an error occurred (and dest doesn't contain a valid directory)
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
	if ( SUCCEEDED( SHGetFolderPathA( NULL, CSIDL_PERSONAL|CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, tmpstr ) ) )
		strlcat(tmpstr, PHYSFS_getDirSeparator(), size);
	else
#elif defined(WZ_OS_MAC)
	FSRef fsref;
	OSErr error = FSFindFolder(kUserDomain, kApplicationSupportFolderType, false, &fsref);
	if (!error)
		error = FSRefMakePath(&fsref, (UInt8 *) tmpstr, size);
	if (!error)
		strlcat(tmpstr, PHYSFS_getDirSeparator(), size);
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


static void initialize_ConfigDir(void)
{
	char tmpstr[PATH_MAX] = { '\0' };

	if (strlen(configdir) == 0)
	{
		getPlatformUserDir(tmpstr, sizeof(tmpstr));

		if (!PHYSFS_setWriteDir(tmpstr)) // Workaround for PhysFS not creating the writedir as expected.
		{
			debug(LOG_FATAL, "Error setting write directory to \"%s\": %s",
			      tmpstr, PHYSFS_getLastError());
			exit(1);
		}

		if (!PHYSFS_mkdir(WZ_WRITEDIR)) // s.a.
		{
			debug(LOG_FATAL, "Error creating directory \"%s\": %s",
			      WZ_WRITEDIR, PHYSFS_getLastError());
			exit(1);
		}

		// Append the Warzone subdir
		sstrcat(tmpstr, WZ_WRITEDIR);
		sstrcat(tmpstr, PHYSFS_getDirSeparator());

		if (!PHYSFS_setWriteDir(tmpstr))
		{
			debug( LOG_FATAL, "Error setting write directory to \"%s\": %s",
			tmpstr, PHYSFS_getLastError() );
			exit(1);
		}
	}
	else
	{
		sstrcpy(tmpstr, configdir);

		// Make sure that we have a directory separator at the end of the string
		if (tmpstr[strlen(tmpstr) - 1] != PHYSFS_getDirSeparator()[0])
			sstrcat(tmpstr, PHYSFS_getDirSeparator());

		debug(LOG_WZ, "Using custom configuration directory: %s", tmpstr);

		if (!PHYSFS_setWriteDir(tmpstr)) // Workaround for PhysFS not creating the writedir as expected.
		{
			debug(LOG_FATAL, "Error setting write directory to \"%s\": %s",
			      tmpstr, PHYSFS_getLastError());
			exit(1);
		}

		// NOTE: This is currently only used for mingw builds for now.
#if defined (WZ_CC_MINGW)
		if (!OverrideRPTDirectory(tmpstr))
		{
			// since it failed, we just use our default path, and not the user supplied one.
			debug(LOG_ERROR, "Error setting exception hanlder to use directory %s", tmpstr);
		}
#endif
	}

	// User's home dir first so we allways see what we write
	PHYSFS_addToSearchPath( PHYSFS_getWriteDir(), PHYSFS_PREPEND );
#if defined(WZ_OS_MAC)
	PHYSFS_addToSearchPath( "/Library/Application Support/Warzone 2100/", PHYSFS_APPEND );
#endif

	PHYSFS_permitSymbolicLinks(1);

	debug(LOG_WZ, "Write dir: %s", PHYSFS_getWriteDir());
	debug(LOG_WZ, "Base dir: %s", PHYSFS_getBaseDir());
}


/*!
 * Initialize the PhysicsFS library.
 */
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


/*!
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
	if( strlen( datadir ) != 0 )
		registerSearchPath( datadir, 1 );

	// User's home dir
	registerSearchPath( PHYSFS_getWriteDir(), 2 );
	rebuildSearchPath( mod_multiplay, true );

	if( !PHYSFS_exists("gamedesc.lev") )
	{
		// Data in SVN dir
		sstrcpy(tmpstr, prefix);
		sstrcat(tmpstr, "/data/");
		registerSearchPath( tmpstr, 3 );
		rebuildSearchPath( mod_multiplay, true );

		if( !PHYSFS_exists("gamedesc.lev") )
		{
			// Relocation for AutoPackage
			sstrcpy(tmpstr, prefix);
			sstrcat(tmpstr, "/share/warzone2100/");
			registerSearchPath( tmpstr, 4 );
			rebuildSearchPath( mod_multiplay, true );

			if( !PHYSFS_exists("gamedesc.lev") )
			{
				// Program dir
				registerSearchPath( PHYSFS_getBaseDir(), 5 );
				rebuildSearchPath( mod_multiplay, true );

				if( !PHYSFS_exists("gamedesc.lev") )
				{
					// Guessed fallback default datadir on Unix
					registerSearchPath( WZ_DATADIR, 6 );
					rebuildSearchPath( mod_multiplay, true );
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
			rebuildSearchPath( mod_multiplay, true );
		} else {
			debug( LOG_ERROR, "Could not change to resources directory." );
		}

		if( resourceURL != NULL ) {
			CFRelease( resourceURL );
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
		debug( LOG_FATAL, "Could not find game data. Aborting." );
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
		debug(LOG_FATAL, "Unable to create directory \"%s\" in write dir \"%s\"!",
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
		debug( LOG_FATAL, "Shutting down after failure" );
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
		debug( LOG_FATAL, "Shutting down after failure" );
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
		debug( LOG_FATAL, "Shutting down after failure" );
		exit(EXIT_FAILURE);
	}
	//after data is loaded check the research stats are valid
	if (!checkResearchStats())
	{
		debug( LOG_FATAL, "Invalid Research Stats" );
		debug( LOG_FATAL, "Shutting down after failure" );
		exit(EXIT_FAILURE);
	}
	//and check the structure stats are valid
	if (!checkStructureStats())
	{
		debug( LOG_FATAL, "Invalid Structure Stats" );
		debug( LOG_FATAL, "Shutting down after failure" );
		exit(EXIT_FAILURE);
	}

	screen_StopBackDrop();

	// Trap the cursor if cursor snapping is enabled
	if (war_GetTrapCursor())
	{
		SDL_WM_GrabInput(SDL_GRAB_ON);
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
	screen_disableMapPreview();
}


/*!
 * Shutdown/cleanup after the game loop
 * Would stop the timer
 */
static void stopGameLoop(void)
{
	if (gameLoopStatus != GAMECODE_NEWLEVEL)
	{
		initLoadingScreen(true); // returning to f.e. do a loader.render not active
		pie_EnableFog(false); // dont let the normal loop code set status on
		fogStatus = 0;
		if (gameLoopStatus != GAMECODE_LOADGAME)
		{
			if (!levReleaseAll())
			{
				debug(LOG_ERROR, "levReleaseAll failed!");
			}
		}
		reloadMPConfig();
	}

	// Disable cursor trapping
	if (war_GetTrapCursor())
	{
		SDL_WM_GrabInput(SDL_GRAB_OFF);
	}

	gameInitialised = false;
}


/*!
 * Load a savegame and start into the game loop
 * Game data should be initialised afterwards, so that startGameLoop is not necessary anymore.
 */
static bool initSaveGameLoad(void)
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

	// Trap the cursor if cursor snapping is enabled
	if (war_GetTrapCursor())
	{
		SDL_WM_GrabInput(SDL_GRAB_ON);
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

			break;
			}
		case TITLECODE_STARTGAME:
			debug(LOG_MAIN, "TITLECODE_STARTGAME");
			stopTitleLoop();
			startGameLoop(); // Restart into gameloop
			break;
		case TITLECODE_SHOWINTRO:
			debug(LOG_MAIN, "TITLECODE_SHOWINTRO");
			seq_ClearSeqList();
			seq_AddSeqToList("titles.ogg", NULL, NULL, false);
			seq_AddSeqToList("devastation.ogg", NULL, "devastation.txa", false);
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
	if ( activeEvent->state == SDL_APPMOUSEFOCUS )
	{
		setMouseScroll(activeEvent->gain);
		return;
	}

	if ( activeEvent->gain == 1 )
	{
		debug( LOG_NEVER, "WM_SETFOCUS");
		if (focusState != FOCUS_IN)
		{
			focusState = FOCUS_IN;

			// Don't pause in multiplayer!
			if (war_GetPauseOnFocusLoss() && !NetPlay.bComms)
			{
				gameTimeStart();
				audio_ResumeAll();
				cdAudio_Resume();
			}
			// enable scrolling
			setScrollPause(false);
			resetScroll();
		}
	}
	else
	{
		debug( LOG_NEVER, "WM_KILLFOCUS");
		if (focusState != FOCUS_OUT)
		{
			focusState = FOCUS_OUT;

			// Don't pause in multiplayer!
			if (war_GetPauseOnFocusLoss() && !NetPlay.bComms)
			{
				gameTimeStop();
				audio_PauseAll();
				cdAudio_Pause();
			}
			/* Have to tell the input system that we've lost focus */
			inputLooseFocus();
			// stop scrolling
			setScrollPause(true);
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

	while (true)
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
					handleActiveEvent(&event.active);
					break;
				case SDL_QUIT:
					saveConfig();
					return;
				default:
					break;
			}
		}
		// Screenshot key is now available globally
		if(keyPressed(KEY_F10))
		{
			kf_ScreenDump();
			inputLooseFocus();		// remove it from input stream
		}

		// only pause when not in multiplayer, no focus, and we actually want to pause
		if (NetPlay.bComms || focusState == FOCUS_IN || !war_GetPauseOnFocusLoss())
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

			realTimeUpdate(); // Update realTime.
		}
	}
}

static bool fptest1(void)
{
	bool wtf = false;
	float a = 5.19258240356725201562535524577016477814756008082239;
	uint8_t aexpect[] = {5, 5, 5, 5, 5, 3, 3, 2, 1, 10, 1, 4, 6, 42, 6, 3, 2, 2, 5, 2, 1, 1, 1, 1, 2, 26, 1, 3, 1, 11, 2, 2, 1, 1, 3, 2, 2, 1, 2, 9, 1, 1, 9, 25, 1, 4, 4, 2, 8, 2, 14, 139, 1, 1, 1, 19, 1, 1, 10, 2, 1, 10, 1, 1, 1, 28, 11, 4, 4, 1, 2, 12, 59, 1, 4, 4, 8, 2, 2, 37, 1, 85, 1, 1, 15, 8, 1, 100, 1, 1, 2, 9, 1, 5, 1, 17, 6, 15, 3, 1};
	float b = 5.03937732811384756748132236757401955859521845805786;
	uint8_t bexpect[] = {5, 5, 5, 5, 3, 1, 2, 1, 2, 3, 2, 2, 1, 1, 1, 1, 1, 1, 2, 5, 1, 1, 1, 1, 1, 1, 2, 5, 1, 3, 3, 1, 4, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 3, 2, 1, 2, 1, 1, 3, 1, 1, 2, 1, 3, 1, 9, 1, 1, 25, 2, 1, 1, 2, 4, 1, 1, 1, 1, 3, 2, 1, 3, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 7, 2, 1, 1, 1, 1, 2, 3, 2, 1, 11, 1, 1, 1, 1, 2, 1};
	float cr = 0, ci = 0;
	uint8_t cexpect[] = {36, 60, 92, 240, 55, 238, 206, 114, 84, 161, 196, 230, 36, 100, 212, 188, 201, 133, 48, 29, 189, 177, 206, 108, 66, 201, 226, 174, 208, 116, 183, 211, 133, 210, 209, 147, 137, 189, 21, 129, 64, 0, 108, 133, 198, 247, 35, 238, 161, 101, 116, 26, 78, 120, 34, 30, 105, 150, 213, 9, 149, 98, 185, 38, 57, 164, 72, 90, 211, 210, 43, 244, 219, 116, 178, 36, 46, 243, 109, 112, 250, 37, 163, 40, 206, 217, 160, 109, 193, 115, 201, 143, 192, 234, 0, 114, 112, 224, 147, 48};
	float d1 = 0.00000000000000001, d2 = 100000000000000000.0;
	unsigned n;
	for (n = 0; n != 100; ++n)
	{
		if ((unsigned)a != aexpect[n])
		{
			debug(LOG_ERROR, "My 32-bit floating point works differently.");
			wtf = true;
			break;
		}
		a = 1/(a - (unsigned)a);
	}
	for (n = 0; n != 100; ++n)
	{
		if ((unsigned)b != bexpect[n])
		{
			debug(LOG_ERROR, "My 32-bit floating point square roots work differently.");
			wtf = true;
			break;
		}
		b = 1/sqrtf(b - (unsigned)b);
	}
	for (n = 0; n != 100; ++n)
	{
		float t = cr*cr - ci*ci + 0.145750134638f;
		ci = 2*cr*ci + 0.6112626396511f;
		cr = t;
		if ((uint64_t)(cr*2000000000*2000000000)%251 != cexpect[n])
		{
			debug(LOG_ERROR, "My 32-bit floating point mandelbrots work differently.");
			wtf = true;
			break;
		}
	}
	if (d2 + d1 - d2 != 0.0f)
	{
		debug(LOG_ERROR, "My 32-bit addition works differently.");
		wtf = true;
	}

	return !wtf;
}

static bool fptest2(void)
{
	bool wtf = false;
	double a = 5.19258240356725201562535524577016477814756008082239;
	uint16_t aexpect[] = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 3, 1, 55, 1, 3, 1, 2, 1, 1, 2, 2, 4, 2, 17, 1, 1, 5, 1, 28, 26, 1, 6, 4, 2, 1, 1, 2, 2, 4, 1, 11, 2, 1, 32, 5, 4, 2, 1, 1, 4, 1, 1, 1, 2, 8, 1, 4, 2, 2, 2, 2, 31, 1, 1, 90, 3, 1, 4, 1, 1, 273, 1, 4, 167, 5, 1, 1, 4, 1, 1, 1, 1, 6, 3, 1, 10, 1, 1, 4, 1, 1, 1, 1, 3, 3, 1, 2, 2, 1};
	double b = 5.03937732811384756748132236757401955859521845805786;
	uint8_t bexpect[] = {5, 5, 5, 5, 5, 5, 5, 5, 4, 1, 7, 1, 1, 1, 1, 7, 1, 1, 1, 2, 1, 1, 2, 2, 1, 4, 1, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 2, 3, 2, 2, 3, 1, 1, 1, 1, 7, 1, 3, 1, 2, 1, 1, 2, 1, 1, 1, 12, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 12, 18, 1, 2, 3, 2};
	double cr = 0, ci = 0;
	uint8_t cexpect[] = {11, 202, 42, 222, 138, 236, 31, 247, 166, 57, 129, 88, 117, 43, 4, 34, 157, 206, 52, 179, 202, 166, 199, 149, 53, 214, 38, 179, 80, 85, 207, 89, 39, 84, 153, 125, 91, 46, 219, 200, 233, 122, 11, 223, 69, 96, 153, 50, 72, 28, 236, 98, 79, 104, 69, 193, 179, 48, 4, 120, 78, 5, 217, 224, 147, 116, 114, 47, 187, 125, 150, 81, 2, 92, 36, 228, 156, 165, 231, 139, 29, 183, 230, 143, 226, 221, 108, 234, 212, 86, 124, 72, 215, 194, 63, 102, 172, 18, 97, 131};
	float d1 = 0.00000000000000001, d2 = 100000000000000000.0;
	unsigned n;
	for (n = 0; n != 100; ++n)
	{
		if ((unsigned)a != aexpect[n])
		{
			debug(LOG_ERROR, "My 64-bit floating point works differently.");
			wtf = true;
			break;
		}
		a = 1/(a - (unsigned)a);
	}
	for (n = 0; n != 100; ++n)
	{
		if ((unsigned)b != bexpect[n])
		{
			debug(LOG_ERROR, "My 64-bit floating point square roots work differently.");
			wtf = true;
			break;
		}
		b = 1/sqrt(b - (unsigned)b);
	}
	for (n = 0; n != 100; ++n)
	{
		double t = cr*cr - ci*ci + 0.145750134638;
		ci = 2*cr*ci + 0.6112626396511;
		cr = t;
		if ((uint64_t)(cr*2000000000*2000000000)%251 != cexpect[n])
		{
			debug(LOG_ERROR, "My 64-bit floating point mandelbrots work differently.");
			wtf = true;
			break;
		}
	}
	if (d2 + d1 - d2 != 0.0)
	{
		debug(LOG_ERROR, "My 64-bit addition works differently.");
		wtf = true;
	}

	return !wtf;
}

int main(int argc, char *argv[])
{
	int ok;

#ifdef WZ_OS_MAC
	cocoaInit();
#endif

	setupExceptionHandler(argc, argv);

	debug_init();
	debug_register_callback( debug_callback_stderr, NULL, NULL, NULL );
#if defined(WZ_OS_WIN) && defined(DEBUG_INSANE)
	debug_register_callback( debug_callback_win32debug, NULL, NULL, NULL );
#endif // WZ_OS_WIN && DEBUG_INSANE

	// Sanity check that floating point math is the same on all machines. (x86 needs to be compiled at least with the "-msse -mfpmath=sse" compiler flags, in addition to "-mno-fused-madd".)
	ok = fptest1();
	fptest2();  // Multiplayer seems to work, even if doubles are broken. (Should avoid use of doubles, or require "-msse2".)
	if (!ok)
	{
		debug(LOG_WARNING, "Your compiler and compiler flag combination breaks 32-bit floats. Hope there isn't too much floating point code left.");
	}

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

	/*** Initialize directory structure ***/
	make_dir(ScreenDumpPath, "screenshots", NULL);
	make_dir(SaveGamePath, "savegame", NULL);
	PHYSFS_mkdir("maps");		// MUST have this to prevent crashes when getting map
	PHYSFS_mkdir("music");
	PHYSFS_mkdir("logs");		// a place to hold our netplay, mingw crash reports & WZ logs
	make_dir(MultiPlayersPath, "multiplay", NULL);
	make_dir(MultiPlayersPath, "multiplay", "players");
	sstrcpy(MultiCustomMapsPath, "maps");

	if (!customDebugfile)
	{
		// there was no custom debug file specified  (--debug-file=blah)
		// so we use our write directory to store our logs.
		time_t aclock;
		struct tm *newtime;
		char buf[PATH_MAX];

		time( &aclock );					// Get time in seconds
		newtime = localtime( &aclock );		// Convert time to struct
		// Note: We are using fopen(), and not physfs routines to open the file
		// log name is logs/(or \)WZlog-MMDD_HHMMSS.txt
		snprintf(buf, sizeof(buf), "%slogs%sWZlog-%02d%02d_%02d%02d%02d.txt", PHYSFS_getWriteDir(), PHYSFS_getDirSeparator(),
			newtime->tm_mon, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec );
		debug_register_callback( debug_callback_file, debug_callback_file_init, debug_callback_file_exit, buf );
	}
	debug(LOG_WZ, "Warzone 2100 - %s", version_getFormattedVersionString());

	/* Put these files in the writedir root */
	setRegistryFilePath("config");
	sstrcpy(KeyMapPath, "keymap.map");

	// initialise all the command line states
	war_SetDefaultStates();

	debug(LOG_MAIN, "initializing");

	loadConfig();

	loadRenderMode(); //get the registry entry for clRendMode

	NETinit(true);

	// parse the command line
	if (!ParseCommandLine(argc, (const char**)argv)) {
		return -1;
	}

	// Save new (commandline) settings
	saveConfig();

	// Find out where to find the data
	scanDataDirs();

	// Must be run before OpenGL driver is properly initialized due to
	// strange conflicts - Per
	if (selfTest)
	{
		memset(enabled_debug, 0, sizeof(*enabled_debug) * LOG_LAST);
		fprintf(stdout, "Carrying out self-test:\n");
		playListTest();
		audioTest();
		soundTest();
	}

	// Now we check the mods to see if they exsist or not (specified on the command line)
	// They are all capped at 100 mods max(see clparse.c)
	// FIX ME: I know this is a bit hackish, but better than nothing for now?
	{
		char *modname;
		char modtocheck[256];
		int i = 0;
		int result = 0;

		// check global mods
		for(i=0; i < 100; i++)
		{
			modname = global_mods[i];
			if (modname == NULL)
			{
				break;
			}
			ssprintf(modtocheck, "mods/global/%s", modname);
			result = PHYSFS_exists(modtocheck);
			result |= PHYSFS_isDirectory(modtocheck); 
			if (!result)
			{
				debug(LOG_ERROR, "The (global) mod (%s) you have specified doesn't exist!", modname);
			}
			else
			{
				info("(global) mod (%s) is enabled", modname);
			}
		}
		// check campaign mods
		for(i=0; i < 100; i++)
		{
			modname = campaign_mods[i];
			if (modname == NULL)
			{
				break;
			}
			ssprintf(modtocheck, "mods/campaign/%s", modname);
			result = PHYSFS_exists(modtocheck);
			result |= PHYSFS_isDirectory(modtocheck); 
			if (!result)
			{
				debug(LOG_ERROR, "The mod_ca (%s) you have specified doesn't exist!", modname);
			}
			else
			{
				info("mod_ca (%s) is enabled", modname);
			}
		}
		// check multiplay mods
		for(i=0; i < 100; i++)
		{
			modname = multiplay_mods[i];
			if (modname == NULL)
			{
				break;
			}
			ssprintf(modtocheck, "mods/multiplay/%s", modname);
			result = PHYSFS_exists(modtocheck);
			result |= PHYSFS_isDirectory(modtocheck); 
			if (!result)
			{
				debug(LOG_ERROR, "The mod_mp (%s) you have specified doesn't exist!", modname);
			}
			else
			{
				info("mod_mp (%s) is enabled", modname);
			}
		}
	}

	if (!frameInitialise( "Warzone 2100", pie_GetVideoBufferWidth(), pie_GetVideoBufferHeight(), pie_GetVideoBufferDepth(), war_getFSAA(), war_getFullscreen(), war_GetVsync()))
	{
		return -1;
	}
	war_SetWidth(pie_GetVideoBufferWidth());
	war_SetHeight(pie_GetVideoBufferHeight());

	pie_SetFogStatus(false);
	pie_ScreenFlip(CLEAR_BLACK);

	pal_Init();

	pie_LoadBackDrop(SCREEN_RANDOMBDROP);
	pie_SetFogStatus(false);
	pie_ScreenFlip(CLEAR_BLACK);

	if (!systemInitialise())
	{
		return -1;
	}

	//set all the pause states to false
	setAllPauseStates(false);

	/* Runtime unit testing */
	if (selfTest)
	{
		NETtest();
		tagTest();
		parseTest();
		levTest();
		mapTest();
		fprintf(stdout, "All tests PASSED!\n");
		exit(0);
	}

	{
		// Copy this info to be used by the crash handler for the dump file
		char buf[256];

		ssprintf(buf,"Using language: %s", getLanguageName());
		addDumpInfo(buf);
	}
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

#if defined(WZ_CC_MSVC) && defined(DEBUG)
	debug_MEMSTATS();
#endif

	atexit(systemShutdown);
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
