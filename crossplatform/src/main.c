/*
 * main.c
 *
 */
#include "frame.h"
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
#include "configfile.h"
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
#include "wdg.h"
#include "multiwdg.h"
#include "screen.h"

// Warzone 2100 . Pumpkin Studios

UDWORD	gameStatus = GS_TITLE_SCREEN;	// Start game in title mode.
UDWORD	lastStatus = GS_TITLE_SCREEN;
//flag to indicate when initialisation is complete
BOOL	videoInitialised = FALSE;
BOOL	gameInitialised = FALSE;
BOOL	frontendInitialised = FALSE;
BOOL	reInit = FALSE;
BOOL	bGlideFound=FALSE;		
BOOL	bDisableLobby;
char	SaveGamePath[255];
char	ScreenDumpPath[255];
char	MultiForcesPath[255];
char	MultiCustomMapsPath[255];
char	MultiPlayersPath[255];
char	KeyMapPath[255];
char*	UserMusicPath;

#ifndef WIN32

char	__UserMusicPath[255];

#endif

// Some prototypes because I can't be arse to create a .h file
BOOL InitGlideDLL(void);
BOOL ShutdownGlideDLL(void);


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

int main(int argc, char *argv[])
{
	FRAME_STATUS		frameRet;
	BOOL			quit = FALSE;
	BOOL			Restart = FALSE;
	BOOL			paused = FALSE;//, firstTime = TRUE;
	BOOL			bGlide = FALSE;
	BOOL			bVidMem = FALSE;
	SDWORD			dispBitDepth = DISP_BITDEPTH;
	SDWORD			introVideoControl = 3;
	GAMECODE		loopStatus = 0;
	iColour*		psPaletteBuffer;
	SDWORD			pSize;
#ifdef _MSC_VER	
	int tmpFlag; //debug stuff for VC -Q
#endif
#ifndef WIN32

	char	UnixUserPath[255];

#endif

#ifdef WIN32				//Note, Should we fix this like linux so we don't use the reg, and use config file? -Q
	strcpy(SaveGamePath,"savegame\\");
	strcpy(MultiForcesPath,"multiplay\\forces\\");
	strcpy(MultiCustomMapsPath,"multiplay\\custommaps\\");
	strcpy(MultiPlayersPath,"multiplay\\players\\");
	strcpy(ScreenDumpPath,"");
	strcpy(KeyMapPath,"keymap.map");
	UserMusicPath = NULL;
#else
	strcpy(UnixUserPath,(char *)getenv("HOME"));
	strcat(UnixUserPath,"/.warzone2100/");
	CreateDirectory(UnixUserPath,NULL);
	strcpy(ScreenDumpPath,UnixUserPath);
	strcat(ScreenDumpPath,"screendumps/");
	CreateDirectory(ScreenDumpPath,NULL);
	strcpy(SaveGamePath,UnixUserPath);
	strcat(SaveGamePath,"savegame/");
	strcpy(MultiPlayersPath,UnixUserPath);
	strcat(MultiPlayersPath,"multiplay/");
	CreateDirectory(MultiPlayersPath,NULL);
	strcat(MultiPlayersPath,"players/");
	strcpy(MultiForcesPath,UnixUserPath);
	strcat(MultiForcesPath,"multiplay/forces/");
	strcpy(MultiCustomMapsPath,UnixUserPath);
	strcat(MultiCustomMapsPath,"multiplay/custommaps/");
	strcpy(UnixRegFilePath,UnixUserPath);
	strcat(UnixRegFilePath,"config");
	strcpy(KeyMapPath,UnixUserPath);
	strcat(KeyMapPath,"keymap.map");
	strcpy(__UserMusicPath,UnixUserPath);
	strcat(__UserMusicPath,"music");
	UserMusicPath = __UserMusicPath;
#endif

	// initialise all the command line states
	clIntroVideo = FALSE;

	debug_init(); // must be called early

//=========================
/*
#ifdef _DEBUG		//Debug stuff for .net, don't delete :)  -Q
#ifdef _MSC_VER
tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
tmpFlag |=(_CRTDBG_CHECK_ALWAYS_DF|_CRTDBG_ALLOC_MEM_DF);
_CrtSetDbgFlag( tmpFlag );			//just turning on VC debug stuff...
#endif
#endif
*/
//=========================


	war_SetDefaultStates();

	war_SetRendMode(REND_MODE_SOFTWARE); // NOID changed from REND_MODE_HAL

	bGlideFound = FALSE;
   
init://jump here from the end if re_initialising

	// initialise memory stuff, moved out of frameinit by ajl. 
	if (!memInitialise())
	{
		return FALSE;
	}

	if (!blkInitialise())
	{
		return FALSE;
	}


	loadRenderMode();//get the registry entry for clRendMode

	bDisableLobby = FALSE;

	// parse the command line
//	if (bDisableLobby || !NetPlay.bLobbyLaunched)
//	{
		if(!reInit)
		{
			if(!ParseCommandLine(argc, argv))
			{
				return -1;
			}
		}
//	}

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

	//always start windowed toggle to fullscreen later
	if (war_GetRendMode() == REND_MODE_HAL)
	{
		bGlide = FALSE;
		bVidMem = TRUE;
		dispBitDepth = DISP_HARDBITDEPTH;
	}
	else if (war_GetRendMode() == REND_MODE_REF)
	{
		bGlide = FALSE;
		bVidMem = TRUE;
		dispBitDepth = DISP_HARDBITDEPTH;
	}
	else if (war_GetRendMode() == REND_MODE_RGB)
	{
		bGlide = FALSE;
		bVidMem = FALSE;
		dispBitDepth = DISP_HARDBITDEPTH;
	}
	else if (war_GetRendMode() == REND_MODE_GLIDE)
	{
		bGlide = TRUE;
		bVidMem = FALSE;
		dispBitDepth = DISP_HARDBITDEPTH;
	}
	else
	{
		bGlide = FALSE;
		bVidMem = FALSE;
		dispBitDepth = DISP_BITDEPTH;
	}

//	frameDDEnumerate();

	if (!frameInitialise(NULL, "Warzone 2100", DISP_WIDTH,DISP_HEIGHT,dispBitDepth, war_getFullscreen(), bVidMem, bGlide))
	{
		return -1;
	}
	if (!wdgLoadAllWDGCatalogs())
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
	if (!loadFileToBuffer("palette.bin", (UBYTE*)psPaletteBuffer, (256 * sizeof(iColour)+1),(UDWORD*)&pSize))
	{
		DBERROR(("Couldn't load palette data"));
		return -1;
	}
	pal_AddNewPalette(psPaletteBuffer);
	FREE(psPaletteBuffer);

	if (war_GetRendMode() == REND_MODE_GLIDE)
	{
		pie_LoadBackDrop(SCREEN_RANDOMBDROP,TRUE);
	}
	else
	{
		pie_LoadBackDrop(SCREEN_RANDOMBDROP,FALSE);
	}
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
	
// If windowed mode not requested then toggle to full screen. Doing
// it here rather than in the call to frameInitialise fixes a problem
// where machines with an NVidia and a 3DFX would kill the 3dfx display. (Definitly a HACK, PD)
/*	if(!clStartWindowed)
	{
		screenToggleMode();
	}
*/
	//set all the pause states to false
	setAllPauseStates(FALSE);

	while (!quit)
	{
// Do the game mode specific initialisation.
		switch(gameStatus)
		{
			case GS_TITLE_SCREEN:
				screen_RestartBackDrop();

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
				DBERROR(("Unknown game status on startup!"));
		}

		
		DBPRINTF(("Entering main loop\n"));

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
								DBPRINTF(("TITLECODE_QUITGAME\n"));
								Restart = TRUE;
								quit = TRUE;
								break;

	//						case TITLECODE_ATTRACT:
	//							DBPRINTF(("TITLECODE_ATTRACT\n"));
	//							break;

							case TITLECODE_SAVEGAMELOAD:
								DBPRINTF(("TITLECODE_SAVEGAMELOAD\n"));
								gameStatus = GS_SAVEGAMELOAD;
								Restart = TRUE;
								break;
							case TITLECODE_STARTGAME:
								DBPRINTF(("TITLECODE_STARTGAME\n"));
								gameStatus = GS_NORMAL;
								Restart = TRUE;
								break;

							case TITLECODE_SHOWINTRO:	
								DBPRINTF(("TITLECODE_SHOWINTRO\n"));
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
								DBERROR(("Unknown code returned by titleLoop"));
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
								DBPRINTF(("GAMECODE_FASTEXIT\n"));
								Restart = TRUE;
								quit = TRUE;
								break;

							case GAMECODE_LOADGAME:
								DBPRINTF(("GAMECODE_LOADGAME\n"));
								Restart = TRUE;
								gameStatus = GS_SAVEGAMELOAD;
								break;

							case GAMECODE_PLAYVIDEO:
								DBPRINTF(("GAMECODE_PLAYVIDEO\n"));
//dont schange mode any more								gameStatus = GS_VIDEO_MODE;
								Restart = FALSE;
								break;

							case GAMECODE_NEWLEVEL:
								DBPRINTF(("GAMECODE_NEWLEVEL\n"));
								// gameStatus is unchanged, just loading additional data
								Restart = TRUE;
								break;

							case GAMECODE_RESTARTGAME:
								DBPRINTF(("GAMECODE_RESTARTGAME\n"));
								Restart = TRUE;
								break;

							case GAMECODE_CONTINUE:
								break;

							default:
								DBERROR(("Unknown code returned by gameLoop"));
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
								DBPRINTF(("VIDEO_QUIT\n"));
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
					debug(LOG_ERROR, "Weirdy game status I'm afraid!!");
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

