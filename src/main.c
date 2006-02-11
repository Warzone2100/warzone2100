/*
 * main.c
 *
 */
#include "frame.h"

#include <physfs.h>

#include "widget.h"
#include "script.h"
#include "init.h"
#include "loop.h"
#include "objects.h"
#include "display.h"
#include "piestate.h"
#include "gtime.h"
#include "winmain.h"
#include "wrappers.h"
#include "scripttabs.h"
#include "deliverance.h"
#include "frontend.h"
#include "seqdisp.h"
#include "audio.h"
#include "console.h"
#include "rendmode.h"
#include "piemode.h"
#include "levels.h"
#include "research.h"
#include "warzoneconfig.h"
#include "clparse.h"
#include "cdspan.h"
#include "configuration.h"
#include "multiplay.h"
#include "netplay.h"
#include "loadsave.h"
#include "game.h"
#include "lighting.h"
#include "mixer.h"
#include "screen.h"

#ifndef DEFAULT_DATA_DIR
	#define DEFAULT_DATA_DIR "/usr/share"
#endif
char datadir[MAX_PATH]; // Global that src/clparse.c:ParseCommandLineEarly can write to, so it can override the default datadir on runtime

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
char	SaveGamePath[255];
char	ScreenDumpPath[255];
char	MultiForcesPath[255];
char	MultiCustomMapsPath[255];
char	MultiPlayersPath[255];
char	KeyMapPath[255];
char*	UserMusicPath;
char	__UserMusicPath[255];
char	RegFilePath[255];

/*
BOOL checkDisableLobby(void)
{
	BOOL	disable;

	disable = FALSE;
	if(!openWarzoneKey())
	{
		return FALSE;
	}

	if (!getWarzoneKeyNumeric("DisableLobby",(DWORD*)&disable))
	{
		return FALSE;
	}

	if (!closeWarzoneKey())
	{
		return FALSE;
	}

	return disable;
}
*/

/***************************************************************************
	When looking for our data, first check if we have been given an explicit
	data path from the user. Then check the current directory. Finally check
	the usual suspects in Unixland and round up some random hang arounds.
	
	This function sets the datadir variable.
***************************************************************************/
static void find_data_dir(void)
{
	printf("Finding data dir\n");
	/* Do we have a user supplied data dir? It must point to a directory with gamedesc.lev or
	 * a warzone.wz with this file inside. */
	if (datadir[0] != '\0') {
		strcpy(datadir, DEFAULT_DATA_DIR);
		if (!PHYSFS_addToSearchPath(datadir, 1) || !PHYSFS_exists("gamedesc.lev")) {
			debug(LOG_ERROR, "Required file gamedesc.lev not found in requested data dir \"%s\".", datadir);
			exit(1);
		} else {
			return;
		}
	}
	
	/* Check current dir for unpacked game data */
	if (!PHYSFS_addToSearchPath(PHYSFS_getBaseDir(), 1) || !PHYSFS_exists("gamedesc.lev")) {
		debug(LOG_WZ, "Could not find data in current dir \"%s\".", PHYSFS_getBaseDir());
		(void) PHYSFS_removeFromSearchPath(PHYSFS_getBaseDir());
	} else {
		char* tmp;

		strcpy(datadir, PHYSFS_getBaseDir());
		tmp = strrchr(datadir, *PHYSFS_getDirSeparator());
		if (tmp != NULL) *tmp = '\0'; // Trim ending '/', which getBaseDir always provides
		return;
	}
	
	/* Check for warzone.wz in current dir */
	strcpy(datadir, PHYSFS_getBaseDir());
	strcat(datadir, "warzone.wz");
	if (!PHYSFS_addToSearchPath(datadir, 1) || !PHYSFS_exists("gamedesc.lev")) {
		debug(LOG_WZ, "Did not find warzone.wz in currect directory \"%s\".", datadir);
		(void) PHYSFS_removeFromSearchPath(datadir);
	} else {
		return;
	}
	
	/* Check for warzone.wz in data/ dir */
	strcpy(datadir, PHYSFS_getBaseDir());
	strcat(datadir, "data/warzone.wz");
	if (!PHYSFS_addToSearchPath(datadir, 1) || !PHYSFS_exists("gamedesc.lev")) {
		debug(LOG_WZ, "Did not find warzone.wz in data/ directory \"%s\".", datadir);
		(void) PHYSFS_removeFromSearchPath(datadir);
	} else {
		return;
	}
	
	/* Check for warzone.wz in Unixland system data directory and check if we are running 
	 * straight out of the build directory (for convenience). */
	strcpy(datadir, PHYSFS_getBaseDir());
	*strrchr(datadir, '/') = '\0'; // Trim ending '/', which getBaseDir always provides
	if (strcmp(strrchr(datadir, '/'), "/bin" ) == 0) {
		strcpy(strrchr(datadir, '/'), "/share/warzone/warzone.wz" );
	}	else if (strcmp(strrchr(datadir, '/'), "/src" ) == 0 ) {
		strcpy( strrchr( datadir, '/' ), "/data" );
	}
	if (!PHYSFS_addToSearchPath(datadir, 1) || !PHYSFS_exists("gamedesc.lev")) {
		debug(LOG_WZ, "Could not find data in \"%s\".", datadir);
		debug(LOG_ERROR, "Could not find game data. Aborting.");
		exit(1);
	}
}

/***************************************************************************
	Initialize the PhysicsFS library.
***************************************************************************/
static void initialize_PhysicsFS(void)
{
	PHYSFS_Version compiled;
	PHYSFS_Version linked;
	char **i;
	char overridepath[MAX_PATH], writepath[MAX_PATH], mappath[MAX_PATH];
#ifdef WIN32
  const char *writedir = "warzone-2.0";
#else
  const char *writedir = ".warzone-2.0";
#endif

	PHYSFS_VERSION(&compiled);
	PHYSFS_getLinkedVersion(&linked);

	debug(LOG_WZ, "Compiled against PhysFS version: %d.%d.%d",
	      compiled.major, compiled.minor, compiled.patch);
	debug(LOG_WZ, "Linked against PhysFS version: %d.%d.%d",
	      linked.major, linked.minor, linked.patch);

	strcpy(writepath, PHYSFS_getUserDir());
  if (PHYSFS_setWriteDir(writepath) == 0) {
		debug(LOG_ERROR, "Error setting write directory to home directory \"%s\": %s",
		      writepath, PHYSFS_getLastError());
		exit(1);
  }
	strcat(writepath, writedir);
  (void) PHYSFS_mkdir(writedir); /* Just in case it does not exist yet */
  if (PHYSFS_setWriteDir(writepath) == 0) {
		debug(LOG_ERROR, "Error setting write directory to \"%s\": %s",
		      writepath, PHYSFS_getLastError());
		exit(1);
  }
	PHYSFS_addToSearchPath(writepath, 0); /* add to search path */

  find_data_dir();
  debug(LOG_WZ, "Data dir set to \"%s\".", datadir);

	snprintf(overridepath, sizeof(overridepath), "%smods", 
	         PHYSFS_getBaseDir());
	strcpy(mappath, PHYSFS_getBaseDir());
	strcat(mappath, "maps");

	/* The 1 below means append to search path, while 0 means prepend. */
	if (!PHYSFS_addToSearchPath(overridepath, 0)) {
		debug(LOG_WZ, "Error adding override path %s: %s", overridepath,
		      PHYSFS_getLastError());
	}
	if (!PHYSFS_addToSearchPath(mappath, 0)) {
		debug(LOG_WZ, "Error adding map path %s: %s", mappath,
		      PHYSFS_getLastError());
	}

	/** Debugging and sanity checks **/

	debug(LOG_WZ, "Search paths:");
	for (i = PHYSFS_getSearchPath(); *i != NULL; i++) {
		debug(LOG_WZ, "    [%s]", *i);
	}
	PHYSFS_freeList(PHYSFS_getSearchPath());
	debug(LOG_WZ, "Write path: %s", PHYSFS_getWriteDir());

	PHYSFS_permitSymbolicLinks(1);
	debug(LOG_WZ, "gamedesc.lev found at %s", PHYSFS_getRealDir("gamedesc.lev"));
}

/***************************************************************************
	Make a directory in write path and set a variable to point to it.
***************************************************************************/
static void make_dir(char *dest, char *dirname, char *subdir)
{
	strcpy(dest, dirname);
	if (subdir != NULL) {
		strcat(dest, "/");
		strcat(dest, subdir);
	}
	{
		unsigned int l = strlen(dest);

		if (dest[l-1] != '/') {
			dest[l] = '/';
			dest[l+1] = '\0';
		}
	}
	PHYSFS_mkdir(dest);
	if (PHYSFS_isDirectory(dest) == 0) {
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
	BOOL			bVidMem = FALSE;
	SDWORD			dispBitDepth = DISP_BITDEPTH;
	SDWORD			introVideoControl = 3;
	GAMECODE		loopStatus = 0;
	iColour*		psPaletteBuffer;
	SDWORD			pSize;

	/*** Initialize the debug subsystem ***/
	/* Debug stuff for .net, don't delete :) */
#ifdef _MSC_VER	
#ifdef _DEBUG
	{
		int tmpFlag; //debug stuff for VC -Q

		tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
		tmpFlag |= (_CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_ALLOC_MEM_DF);
		_CrtSetDbgFlag( tmpFlag );			// just turning on VC debug stuff...
	}
#endif
	#ifndef MAX_PATH
	#define MAX_PATH 512
	#endif
#endif

	debug_init();
	datadir[0] = '\0'; // this needs to be before ParseCommandLineEarly

	// find early boot info
	if (!ParseCommandLineEarly(argc, argv)) {
		return -1;
	}

	/*** Initialize PhysicsFS ***/

#ifdef WIN32
	PHYSFS_init(NULL);
#else
	PHYSFS_init(argv[0]);
#endif
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
	strcpy(__UserMusicPath, "music");
	UserMusicPath = __UserMusicPath;


	/*** etc ***/

	// initialise all the command line states
	clIntroVideo = FALSE;
	war_SetDefaultStates();

init://jump here from the end if re_initialising

	if (!blkInitialise())
	{
		return FALSE;
	}

	loadRenderMode();//get the registry entry for clRendMode

	bDisableLobby = FALSE;

	// parse the command line
	if (!reInit) {
		if (!ParseCommandLine(argc, argv)) {
			return -1;
		}
	}

	debug(LOG_MAIN, "reinitializing");

	// find out if the lobby stuff has been disabled
//	bDisableLobby = checkDisableLobby();
	if (!bDisableLobby &&
		!lobbyInitialise())			// ajl. Init net stuff. Lobby can modify startup conditions like commandline.
	{
		return -1;
	}

	reInit = FALSE;//just so we dont restart again

#ifdef USE_FILE_PATH
	chdir(FILE_PATH);
#endif

	bVidMem = FALSE;
	dispBitDepth = DISP_BITDEPTH;

//	frameDDEnumerate();

	if (!frameInitialise(NULL, "Warzone 2100", DISP_WIDTH,DISP_HEIGHT,dispBitDepth, war_getFullscreen(), bVidMem))
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
		DBERROR(("Out of memory"));
		return -1;
	}
	if (!loadFileToBuffer("palette.bin", psPaletteBuffer, (256 * sizeof(iColour)+1),(UDWORD*)&pSize))
	{
		DBERROR(("Couldn't load palette data"));
		return -1;
	}
	pal_AddNewPalette(psPaletteBuffer);
	FREE(psPaletteBuffer);

	pie_LoadBackDrop(SCREEN_RANDOMBDROP,FALSE);
	pie_SetFogStatus(FALSE);
	pie_ScreenFlip(CLEAR_BLACK);

	quit = FALSE;

	/* check CDROM drive available */
	if ( cdspan_CheckCDAvailable() == FALSE )
	{
		DBERROR( ("Cannot detect CDROM drive\n") );
		quit = TRUE;
	}

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

				set_active_data_directory(MAX_NUM_PATCHES);

				if (!frontendInitialise("wrf\\frontend.wrf"))
				{
					goto exit;
				}

				frontendInitialised = TRUE;
				frontendInitVars();
				//if intro required set up the video
				if (introVideoControl <= 1)
				{
					seq_ClearSeqList();
					seq_AddSeqToList("eidos-logo.rpl",NULL, NULL, FALSE,0);
					seq_AddSeqToList("pumpkin.rpl",NULL, NULL, FALSE,0);
					seq_AddSeqToList("titles.rpl",NULL, NULL, FALSE,0);
					seq_AddSeqToList("devastation.rpl",NULL,"devastation.txa", FALSE,0);

					seq_StartNextFullScreenVideo();
					introVideoControl = 2;
				}
				break;

			case GS_SAVEGAMELOAD:
				screen_RestartBackDrop();
				gameStatus = GS_NORMAL;
				// load up a save game
				if (!loadGameInit(saveGameName,FALSE))
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
					DBERROR(("Invalid Research Stats"));
					goto exit;
				}
				//and check the structure stats are valid
				if (!checkStructureStats())
				{
					DBERROR(("Invalid Structure Stats"));
					goto exit;
				}

				//set a flag for the trigger/event system to indicate initialisation is complete
				gameInitialised = TRUE;
				screen_StopBackDrop();
				break;
			case GS_VIDEO_MODE:
				DBERROR(("Video_mode no longer valid"));
				if (introVideoControl == 0)
				{
					videoInitialised = TRUE;
				}
				break;

			default:
				debug(LOG_ERROR, "Unknown game status on shutdown!");
		}

		
		debug(LOG_MAIN, "Entering main loop");

		Restart = FALSE;
		//firstTime = TRUE;

		while (!Restart)
		{
			frameRet = frameUpdate();
			
			if (pie_GetRenderEngine() == ENGINE_OPENGL)	//Was ENGINE_D3D -Q
			{
				if ( frameRet == FRAME_SETFOCUS )
				{
//					D3DTestCooperativeLevel( TRUE );
				}
				else
				{
//					D3DTestCooperativeLevel( FALSE );
				}
			}

			switch (frameRet)
			{
			case FRAME_KILLFOCUS:
				paused = TRUE;
				gameTimeStop();

				mixer_SaveIngameVols();
				mixer_RestoreWinVols();
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

				else if (pie_GetRenderEngine() == ENGINE_OPENGL)	//Was ENGINE_D3D -Q
				{
//					dtm_RestoreTextures();
				}
				mixer_SaveWinVols();
				mixer_RestoreIngameVols();
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
					pie_SetSwirlyBoxes(TRUE);
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
								seq_AddSeqToList("eidos-logo.rpl",NULL,NULL, FALSE,0);
								seq_AddSeqToList("pumpkin.rpl",NULL,NULL, FALSE,0);
								seq_AddSeqToList("titles.rpl",NULL,NULL, FALSE,0);
								seq_AddSeqToList("devastation.rpl",NULL,"devastation.txa", FALSE,0);
								seq_StartNextFullScreenVideo();
								introVideoControl = 2;//play the video but dont init the sound system
								break;

							case TITLECODE_CONTINUE:
								break;

							default:
								debug(LOG_ERROR, "Unknown code returned by titleLoop");
						}
					}
					pie_SetSwirlyBoxes(FALSE);
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

								seq_AddSeqToList("factory.rpl",NULL,NULL, FALSE,0);
								seq_StartNextFullScreenVideo();//"sequences\\factory.rpl","sequences\\factory.wav");
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
	ASSERT((status == GS_TITLE_SCREEN ||
			status == GS_MISSION_SCREEN ||
			status == GS_NORMAL ||
			status == GS_VIDEO_MODE ||
			status == GS_SAVEGAMELOAD,
		"SetGameMode: invalid game mode"));

	gameStatus = status;
}

