/*
 * main.c
 *
 */

#include <SDL/SDL_main.h>
#include <physfs.h>
#include <string.h>

/* For SHGetFolderPath */
#ifdef WIN32
# include <shlobj.h>
#endif // WIN32

#include "lib/framework/frame.h"
#include "lib/framework/configfile.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"
#include "lib/script/script.h"
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
#include "winmain.h"
#include "wrappers.h"

#ifndef DEFAULT_DATADIR
# define DEFAULT_DATADIR "/usr/share/warzone2100/"
#endif

#if defined(WIN32)
# define WZ_WRITEDIR "Warzone 2100"
#elif defined(__APPLE__)
# include <CoreServices/CoreServices.h>
# define WZ_WRITEDIR "Warzone 2100"
#else
# define WZ_WRITEDIR ".warzone2100"
#endif

char datadir[MAX_PATH] = "\0"; // Global that src/clparse.c:ParseCommandLine can write to, so it can override the default datadir on runtime. Needs to be \0 on startup for ParseCommandLine to work!

char * global_mods[MAX_MODS] = { NULL };
char * campaign_mods[MAX_MODS] = { NULL };
char * multiplay_mods[MAX_MODS] = { NULL };


// Warzone 2100 . Pumpkin Studios

UDWORD	gameStatus = GS_TITLE_SCREEN;	// Start game in title mode.
UDWORD	lastStatus = GS_TITLE_SCREEN;
//flag to indicate when initialisation is complete
BOOL	videoInitialised = FALSE;
BOOL	gameInitialised = FALSE;
BOOL	frontendInitialised = FALSE;
BOOL	reInit = FALSE;
BOOL	bDisableLobby;
BOOL pQUEUE=TRUE;			//This is used to control our pQueue list. Always ON except for SP games! -Q
char	SaveGamePath[MAX_PATH];
char	ScreenDumpPath[MAX_PATH];
char	MultiForcesPath[MAX_PATH];
char	MultiCustomMapsPath[MAX_PATH];
char	MultiPlayersPath[MAX_PATH];
char	KeyMapPath[MAX_PATH];
char	UserMusicPath[MAX_PATH];

void debug_callback_stderr( void**, const char * );
void debug_callback_win32debug( void**, const char * );

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
	char tmpstr[MAX_PATH];
	char ** subdirlist = PHYSFS_enumerateFiles( subdir );
	char ** i = subdirlist;
	while( *i != NULL )
	{
#ifdef DEBUG
		debug( LOG_NEVER, "addSubdirs: Examining subdir: [%s]", *i );
#endif // DEBUG
		if( !checkList || inList( checkList, *i ) )
		{
			strcpy( tmpstr, basedir );
			strcat( tmpstr, subdir );
			strcat( tmpstr, PHYSFS_getDirSeparator() );
			strcat( tmpstr, *i );
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
	char tmpstr[MAX_PATH];
	char ** subdirlist = PHYSFS_enumerateFiles( subdir );
	char ** i = subdirlist;
	while( *i != NULL )
	{
#ifdef DEBUG
		debug( LOG_NEVER, "removeSubdirs: Examining subdir: [%s]", *i );
#endif // DEBUG
		if( !checkList || inList( checkList, *i ) )
		{
			strcpy( tmpstr, basedir );
			strcat( tmpstr, subdir );
			strcat( tmpstr, PHYSFS_getDirSeparator() );
			strcat( tmpstr, *i );
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


/***************************************************************************
	Initialize the PhysicsFS library.
***************************************************************************/
static void initialize_PhysicsFS(void)
{
	PHYSFS_Version compiled;
	PHYSFS_Version linked;
	char tmpstr[MAX_PATH] = { '\0' };
#ifdef __APPLE__
	short vol_ref;
	long dir_id;
	FSSpec fsspec;
	FSRef fsref;
	OSErr error;
#endif

	PHYSFS_VERSION(&compiled);
	PHYSFS_getLinkedVersion(&linked);

	debug(LOG_WZ, "Compiled against PhysFS version: %d.%d.%d",
	      compiled.major, compiled.minor, compiled.patch);
	debug(LOG_WZ, "Linked against PhysFS version: %d.%d.%d",
	      linked.major, linked.minor, linked.patch);

#if defined(WIN32)
	if ( SUCCEEDED( SHGetFolderPathA( NULL, CSIDL_PERSONAL|CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, tmpstr ) ) ) // Use "Documents and Settings\Username\My Documents" ("Personal" data in local lang) if possible
		strcat( tmpstr, PHYSFS_getDirSeparator() );
	else
#elif defined(__APPLE__)
	error = FindFolder(kUserDomain, kApplicationSupportFolderType, FALSE, &vol_ref, &dir_id);
	if (!error)
		error = FSMakeFSSpec(vol_ref, dir_id, (const unsigned char *) "", &fsspec);
	if (!error)
		error = FSpMakeFSRef(&fsspec, &fsref);
	if (!error)
		error = FSRefMakePath(&fsref, tmpstr, MAX_PATH);
	if (!error)
		strcat( tmpstr, PHYSFS_getDirSeparator() );
	else
#endif
	strcpy( tmpstr, PHYSFS_getUserDir() ); // Use PhysFS supplied UserDir (As fallback on Windows / Mac, default on Linux)

	if ( !PHYSFS_setWriteDir( tmpstr ) ) // Workaround for PhysFS not creating the writedir as expected.
	{
		debug( LOG_ERROR, "Error setting write directory to \"%s\": %s",
			PHYSFS_getUserDir(), PHYSFS_getLastError() );
		exit(1);
	}

	if ( !PHYSFS_mkdir( WZ_WRITEDIR ) ) // s.a.
	{
		debug( LOG_ERROR, "Error creating directory \"%s\": %s",
			WZ_WRITEDIR, PHYSFS_getLastError() );
		exit(1);
	}

	// Append the Warzone subdir
	strcat( tmpstr, WZ_WRITEDIR );
	strcat( tmpstr, PHYSFS_getDirSeparator() );

	if ( !PHYSFS_setWriteDir( tmpstr ) ) {
		debug( LOG_ERROR, "Error setting write directory to \"%s\": %s",
			tmpstr, PHYSFS_getLastError() );
		exit(1);
	}

	// User's home dir first so we allways see what we write
	PHYSFS_addToSearchPath( PHYSFS_getWriteDir(), PHYSFS_PREPEND );

	PHYSFS_permitSymbolicLinks(1);

	debug( LOG_WZ, "Write dir: %s", PHYSFS_getWriteDir() );
	debug( LOG_WZ, "Base dir: %s", PHYSFS_getBaseDir() );
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
	char tmpstr[MAX_PATH], prefix[MAX_PATH] = { '\0' };

	// Find out which PREFIX we are in...
	strcpy( tmpstr, PHYSFS_getBaseDir() );
	*strrchr( tmpstr, *PHYSFS_getDirSeparator() ) = '\0'; // Trim ending '/', which getBaseDir always provides

	strncpy( prefix, PHYSFS_getBaseDir(), // Skip the last dir from base dir
		strrchr( tmpstr, *PHYSFS_getDirSeparator() ) - tmpstr );

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
		strcpy( tmpstr, prefix );
		strcat( tmpstr, "/data/" );
		registerSearchPath( tmpstr, 3 );
		rebuildSearchPath( mod_multiplay, TRUE );

		if( !PHYSFS_exists("gamedesc.lev") )
		{
			// Relocation for AutoPackage
			strcpy( tmpstr, prefix );
			strcat( tmpstr, "/share/warzone2100/" );
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
					registerSearchPath( DEFAULT_DATADIR, 6 );
					rebuildSearchPath( mod_multiplay, TRUE );
				}
			}
		}
	}


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
		exit(1);
	}
}


int main(int argc, char *argv[])
{
	FRAME_STATUS		frameRet;
	BOOL			quit = FALSE;
	BOOL			Restart = FALSE;
	BOOL			paused = FALSE;//, firstTime = TRUE;
	SDWORD			introVideoControl = 3;
	int			loopStatus = 0;
	iColour*		psPaletteBuffer;
	UDWORD			pSize;

	/*** Initialize the debug subsystem ***/
#ifdef _MSC_VER
# ifdef DEBUG
	int tmpDbgFlag;
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG ); // Output CRT info to debugger

	tmpDbgFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ); // Grab current flags
#  ifdef DEBUG_MEMORY
	tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF; // Check every (de)allocation
#  endif // DEBUG_MEMORY
	tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF; // Check allocations
	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF; // Check for memleaks
	_CrtSetDbgFlag( tmpDbgFlag );
# endif //DEBUG
#endif // _MSC_VER


	debug_init();
	atexit( debug_exit );

	debug_register_callback( debug_callback_stderr, NULL, NULL, NULL );
#if defined WIN32 && defined DEBUG
//	debug_register_callback( debug_callback_win32debug, NULL, NULL, NULL );
#endif // WIN32

	// find early boot info
	if ( !ParseCommandLineEarly(argc, argv) ) {
		return -1;
	}

#ifdef DEBUG
	debug( LOG_WZ, "Warzone 2100 - Version %s - Built %s - DEBUG", VERSION, __DATE__ );
#else
	debug( LOG_WZ, "Warzone 2100 - Version %s - Built %s", VERSION, __DATE__ );
#endif

	/*** Initialize PhysicsFS ***/

	PHYSFS_init(argv[0]);
	initialize_PhysicsFS();

	make_dir(ScreenDumpPath, "screendumps", NULL);
	make_dir(SaveGamePath, "savegame", NULL);
	make_dir(MultiPlayersPath, "multiplay", NULL);
	make_dir(MultiPlayersPath, "multiplay", "players");
	make_dir(MultiForcesPath, "multiplay", "forces");
	make_dir(MultiCustomMapsPath, "multiplay", "custommaps");

	/* Put these files in the writedir root */
	strcpy(RegFilePath, "config");
	strcpy(KeyMapPath, "keymap.map");
	strcpy(UserMusicPath, "music");

	// initialise all the command line states
	clIntroVideo = FALSE;
	war_SetDefaultStates();

init://jump here from the end if re_initialising

	if (!blkInitialise())
	{
		return FALSE;
	}

	loadRenderMode(); //get the registry entry for clRendMode

	bDisableLobby = FALSE;

	// parse the command line
	if (!reInit) {
		if (!ParseCommandLine(argc, argv)) {
			return -1;
		}
		atexit( closeConfig );
	}

	scanDataDirs();

	debug(LOG_MAIN, "reinitializing");

	// find out if the lobby stuff has been disabled
	if (!bDisableLobby &&
		!lobbyInitialise())			// ajl. Init net stuff. Lobby can modify startup conditions like commandline.
	{
		return -1;
	}

	reInit = FALSE;//just so we dont restart again

	if (!frameInitialise( "Warzone 2100", pie_GetVideoBufferWidth(),pie_GetVideoBufferHeight(), pie_GetVideoBufferDepth(), war_getFullscreen() ))
	{
		return -1;
	}

	pie_SetFogStatus(FALSE);
	pie_ScreenFlip(CLEAR_BLACK);
	pie_ScreenFlip(CLEAR_BLACK);

	if(gameStatus == GS_VIDEO_MODE)
	{
		introVideoControl = 0;//play video
		gameStatus = GS_TITLE_SCREEN;
	}

	//load palette
	psPaletteBuffer = (iColour*)MALLOC(256 * sizeof(iColour)+1);
	if (psPaletteBuffer == NULL)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return -1;
	}
	if (!loadFileToBuffer("palette.bin", (char *)psPaletteBuffer, (256 * sizeof(iColour)+1),&pSize))
	{
		debug( LOG_ERROR, "Couldn't load palette data" );
		abort();
		return -1;
	}
	pal_AddNewPalette(psPaletteBuffer);
	FREE(psPaletteBuffer);

	pie_LoadBackDrop(SCREEN_RANDOMBDROP,FALSE);
	pie_SetFogStatus(FALSE);
	pie_ScreenFlip(CLEAR_BLACK);

	quit = FALSE;

	if (!systemInitialise())
	{
		return -1;
	}

	//set all the pause states to false
	setAllPauseStates(FALSE);

	while (!quit)
	{
// Do the game mode specific initialisation.
		switch(gameStatus)
		{
			case GS_TITLE_SCREEN:
				screen_RestartBackDrop();

				//loadLevels(DIR_MULTIPLAYER);

				if (!frontendInitialise("wrf/frontend.wrf"))
				{
					goto exit;
				}

				frontendInitialised = TRUE;
				frontendInitVars();
				//if intro required set up the video
				if (introVideoControl <= 1)
				{
					seq_ClearSeqList();
					seq_AddSeqToList("eidos-logo.rpl", NULL, NULL, FALSE);
					seq_AddSeqToList("pumpkin.rpl", NULL, NULL, FALSE);
					seq_AddSeqToList("titles.rpl", NULL, NULL, FALSE);
					seq_AddSeqToList("devastation.rpl", NULL, "devastation.txa", FALSE);

					seq_StartNextFullScreenVideo();
					introVideoControl = 2;
				}
				break;

			case GS_SAVEGAMELOAD:
				screen_RestartBackDrop();
				gameStatus = GS_NORMAL;
				// load up a save game
				if (!loadGameInit(saveGameName))
				{
					goto exit;
				}
				/*if (!levLoadData(pLevelName, saveGameName)) {
					return -1;
				}*/
				screen_StopBackDrop();
				break;
			case GS_NORMAL:
				if (!levLoadData(pLevelName, NULL, 0)) {
					goto exit;
				}
				//after data is loaded check the research stats are valid
				if (!checkResearchStats())
				{
					debug( LOG_ERROR, "Invalid Research Stats" );
					goto exit;
				}
				//and check the structure stats are valid
				if (!checkStructureStats())
				{
					debug( LOG_ERROR, "Invalid Structure Stats" );
					goto exit;
				}

				//set a flag for the trigger/event system to indicate initialisation is complete
				gameInitialised = TRUE;
				screen_StopBackDrop();
				break;
			case GS_VIDEO_MODE:
				debug( LOG_ERROR, "Video_mode no longer valid" );
				abort();
				if (introVideoControl == 0)
				{
					videoInitialised = TRUE;
				}
				break;

			default:
				debug( LOG_ERROR, "Unknown game status on shutdown!" );
		}


		debug(LOG_MAIN, "Entering main loop");

		Restart = FALSE;
		//firstTime = TRUE;

		while (!Restart)
		{
			frameRet = frameUpdate();

			switch (frameRet)
			{
			case FRAME_KILLFOCUS:
				paused = TRUE;
				gameTimeStop();

				audio_StopAll();
				break;
			case FRAME_SETFOCUS:
				paused = FALSE;
				gameTimeStart();
				if (!dispModeChange())
				{
					quit = TRUE;
					Restart = TRUE;
				}
				break;
			case FRAME_QUIT:
				debug(LOG_MAIN, "frame quit");
				quit = TRUE;
				Restart = TRUE;
				break;
			default:
				break;
			}

			lastStatus = gameStatus;

			if ((!paused) && (!quit))
			{
				switch(gameStatus)
				{
				case	GS_TITLE_SCREEN:
					if (loop_GetVideoStatus())
					{
						videoLoop();
					}
					else
					{
						switch(titleLoop()) {
							case TITLECODE_QUITGAME:
								debug(LOG_MAIN, "TITLECODE_QUITGAME");
								Restart = TRUE;
								quit = TRUE;
								break;

	//						case TITLECODE_ATTRACT:
	//							DBPRINTF(("TITLECODE_ATTRACT\n"));
	//							break;

							case TITLECODE_SAVEGAMELOAD:
								debug(LOG_MAIN, "TITLECODE_SAVEGAMELOAD");
								gameStatus = GS_SAVEGAMELOAD;
								Restart = TRUE;
								break;
							case TITLECODE_STARTGAME:
								debug(LOG_MAIN, "TITLECODE_STARTGAME");
								gameStatus = GS_NORMAL;
								Restart = TRUE;
								break;

							case TITLECODE_SHOWINTRO:
								debug(LOG_MAIN, "TITLECODE_SHOWINTRO");
								seq_ClearSeqList();
								seq_AddSeqToList("eidos-logo.rpl", NULL, NULL, FALSE);
								seq_AddSeqToList("pumpkin.rpl", NULL, NULL, FALSE);
								seq_AddSeqToList("titles.rpl", NULL, NULL, FALSE);
								seq_AddSeqToList("devastation.rpl", NULL, "devastation.txa", FALSE);
								seq_StartNextFullScreenVideo();
								introVideoControl = 2;//play the video but dont init the sound system
								break;

							case TITLECODE_CONTINUE:
								break;

							default:
								debug(LOG_ERROR, "Unknown code returned by titleLoop");
						}
					}
					break;

/*				case GS_SAVEGAMELOAD:
					if (loopNewLevel)
					{
						//the start of a campaign/expand mission
						DBPRINTF(("GAMECODE_NEWLEVEL\n"));
						loopNewLevel = FALSE;
						// gameStatus is unchanged, just loading additional data
						Restart = TRUE;
					}
					break;
*/
				case	GS_NORMAL:
					if (loop_GetVideoStatus())
					{
						videoLoop();
					}
					else
					{
						loopStatus = gameLoop();
						switch(loopStatus) {
							case GAMECODE_QUITGAME:
								debug(LOG_MAIN, "GAMECODE_QUITGAME");
								gameStatus = GS_TITLE_SCREEN;
								Restart = TRUE;
								if(NetPlay.bLobbyLaunched)
								{
//									changeTitleMode(QUIT);
									quit = TRUE;
								}
								break;
							case GAMECODE_FASTEXIT:
								debug(LOG_MAIN, "GAMECODE_FASTEXIT");
								Restart = TRUE;
								quit = TRUE;
								break;

							case GAMECODE_LOADGAME:
								debug(LOG_MAIN, "GAMECODE_LOADGAME");
								Restart = TRUE;
								gameStatus = GS_SAVEGAMELOAD;
								break;

							case GAMECODE_PLAYVIDEO:
								debug(LOG_MAIN, "GAMECODE_PLAYVIDEO");
//dont schange mode any more								gameStatus = GS_VIDEO_MODE;
								Restart = FALSE;
								break;

							case GAMECODE_NEWLEVEL:
								debug(LOG_MAIN, "GAMECODE_NEWLEVEL");
								// gameStatus is unchanged, just loading additional data
								Restart = TRUE;
								break;

							case GAMECODE_RESTARTGAME:
								debug(LOG_MAIN, "GAMECODE_RESTARTGAME");
								Restart = TRUE;
								break;

							case GAMECODE_CONTINUE:
								break;

							default:
								debug(LOG_ERROR, "Unknown code returned by gameLoop");
						}
					}
					break;

				case	GS_VIDEO_MODE:
					debug(LOG_ERROR, "Video_mode no longer valid");
					if (loop_GetVideoStatus())
					{
						videoLoop();
					}
					else
					{
						if (introVideoControl <= 1)
						{
								seq_ClearSeqList();

								seq_AddSeqToList("factory.rpl",NULL,NULL, FALSE);
								seq_StartNextFullScreenVideo();//"sequences/factory.rpl","sequences/factory.wav");
								introVideoControl = 2;
						}
						else
						{
								debug(LOG_MAIN, "VIDEO_QUIT");
								if (introVideoControl == 2)//finished playing intro video
								{
									gameStatus = GS_TITLE_SCREEN;
									if (videoInitialised)
									{
										Restart = TRUE;
									}
									introVideoControl = 3;
								}
								else
								{
									gameStatus = GS_NORMAL;
								}
						}
					}

					break;

				default:
					debug(LOG_ERROR, "Weirdy game status, I'm afraid!!");
					break;
				}

				gameTimeUpdate();
			}
		}	// End of !Restart loop.

// Do game mode specific shutdown.
		switch(lastStatus) {
			case GS_TITLE_SCREEN:
				if (!frontendShutdown())
				{
					goto exit;
				}
				frontendInitialised = FALSE;
				break;

/*			case GS_SAVEGAMELOAD:
				//get the next level to load up
				gameStatus = GS_NORMAL;
				break;*/
			case GS_NORMAL:
				if (loopStatus != GAMECODE_NEWLEVEL)
				{
					initLoadingScreen(TRUE,FALSE);	// returning to f.e. do a loader.render not active
					pie_EnableFog(FALSE);//dont let the normal loop code set status on
					fogStatus = 0;
					if (loopStatus != GAMECODE_LOADGAME)
					{
						levReleaseAll();
					}
				}
				gameInitialised = FALSE;
				break;

			case	GS_VIDEO_MODE:
				debug(LOG_ERROR, "Video_mode no longer valid");
				if (videoInitialised)
				{
					videoInitialised = FALSE;
				}
				break;

			default:
				debug(LOG_ERROR, "Unknown game status on shutdown!");
				break;
		}

	} // End of !quit loop.

	debug(LOG_MAIN, "Shuting down application");

	systemShutdown();
	pal_ShutDown();
	frameShutDown();

	if (reInit) goto init;

	return 0;

exit:

	debug(LOG_ERROR, "Shutting down after failure");

	systemShutdown();
	pal_ShutDown();
	frameShutDown();

	return 1;
}


UDWORD GetGameMode(void)
{
	return gameStatus;
}

void SetGameMode(UDWORD status)
{
	ASSERT( status == GS_TITLE_SCREEN ||
			status == GS_MISSION_SCREEN ||
			status == GS_NORMAL ||
			status == GS_VIDEO_MODE ||
			status == GS_SAVEGAMELOAD,
		"SetGameMode: invalid game mode" );

	gameStatus = status;
}



