/*
 * WinMain.c
 *
 */
#include <direct.h>
#include "Frame.h"
#include "Widget.h"
#include "script.h"
#include "Init.h"
#include "Loop.h"
#include "Objects.h"
#include "Display.h"
#include "pieState.h"
#include "GTime.h"
#include "WinMain.h"
#include "Wrappers.h"
#include "scriptTabs.h"
#include "deliverance.h"
#include "frontend.h"
#include "seqdisp.h"
#include "audio.h"
#include "Console.h"
#include "RendMode.h"
#include "pieMode.h"
#include "dGlide.h"
#include "Levels.h"
#include "research.h"
#include "warzoneConfig.h"
#include "clParse.h"
#include "cdSpan.h"
#include "config.h"
#include "multiplay.h"
#include "netplay.h"
#include "loadsave.h"
#include "d3drender.h"
#include "dx6texman.h"
#include "game.h"
#include "lighting.h"
#include "mixer.h"
#include "wdg.h"
#include "MultiWDG.h"


// Warzone 2100 . Pumpkin Studios

// Quick Note on defines
//
// covermount		- Single Player Demo
//		noninteract	- incomplete. used with covermount to stop player input
//		multidemo	- used with covermount to make a multiplayer demo.
//

UDWORD	gameStatus = GS_TITLE_SCREEN;	// Start game in title mode.
UDWORD	lastStatus = GS_TITLE_SCREEN;
//flag to indicate when initialisation is complete
BOOL	videoInitialised = FALSE;
BOOL	gameInitialised = FALSE;
BOOL	frontendInitialised = FALSE;
BOOL	reInit = FALSE;
BOOL	bGlideFound=FALSE;		
BOOL	bDisableLobby;


// Some prototypes because I can't be arse to create a .h file
BOOL InitGlideDLL(void);
BOOL ShutdownGlideDLL(void);


// callback functions for message boxes and assert boxes
DB_MBRETVAL fxMBCallback(SBYTE *pBuffer)
{
	(void)pBuffer;

	if(pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		grSstControl(GR_CONTROL_DEACTIVATE);
	}

	return DBR_USE_WINDOWS_MB;
}

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

int WINAPI WinMain(
    HINSTANCE  hInstance,		// handle to current instance
    HINSTANCE  hPrevInstance,	// handle to previous instance
    LPSTR  lpCmdLine,			// pointer to command line
    int  nShowCmd)		 		// show state of window
{
	FRAME_STATUS	frameRet;
	BOOL			quit = FALSE;
	BOOL			Restart = FALSE;
	BOOL			paused = FALSE;//, firstTime = TRUE;
	BOOL			bGlide = FALSE;
	BOOL			bVidMem = FALSE;
	SDWORD			dispBitDepth = DISP_BITDEPTH;
	SDWORD			introVideoControl = 3;
	GAMECODE		loopStatus;
	iColour*		psPaletteBuffer;
	SDWORD			pSize;

	(void)nShowCmd;
//	(void)lpCmdLine;
	(void)hPrevInstance;

	// initialise all the command line states
	clStartWindowed = FALSE;
	clIntroVideo = FALSE;

	// save debugging info to disk
	DBOUTPUTFILE("debug.txt");

	if (!pie_CheckForDX6())
	{
		DBERROR(("Unable to create DirectX 6 interface.\nPlease ensure DirectX 6 or later is installed."));
		return -1;
	}

	war_SetDefaultStates();

	war_SetRendMode(REND_MODE_HAL);

	if (InitGlideDLL())	// In ivis02/3dfxdyn.c - returns FALSE if no glide2x.dll is not found
	{
		bGlideFound = TRUE;
		war_SetRendMode(REND_MODE_GLIDE);//default to glide this will be over writen by Registry or Command line if found
	}

   
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
			if(!ParseCommandLine(lpCmdLine,bGlideFound))
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
	_chdir(FILE_PATH);
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

	if (!frameInitialise(hInstance, "Warzone 2100",
									DISP_WIDTH,DISP_HEIGHT,dispBitDepth, !clStartWindowed, bVidMem, bGlide))
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

	if (war_GetRendMode() == REND_MODE_GLIDE)
	{
		dbg_SetMessageBoxCallback(fxMBCallback);
		dbg_SetErrorBoxCallback(fxMBCallback);
		dbg_SetAssertCallback(fxMBCallback);
	}

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
#ifdef COVERMOUNT
		pie_LoadBackDrop(SCREEN_COVERMOUNT,TRUE);
#else
		pie_LoadBackDrop(SCREEN_RANDOMBDROP,TRUE);
#endif
	}
	else
	{
#ifdef COVERMOUNT
		pie_LoadBackDrop(SCREEN_COVERMOUNT,FALSE);
#else
		pie_LoadBackDrop(SCREEN_RANDOMBDROP,FALSE);
#endif
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
			
			if (pie_GetRenderEngine() == ENGINE_D3D)
			{
				if ( frameRet == FRAME_SETFOCUS )
				{
					D3DTestCooperativeLevel( TRUE );
				}
				else
				{
					D3DTestCooperativeLevel( FALSE );
				}
			}

			switch (frameRet)
			{
			case FRAME_KILLFOCUS:
				paused = TRUE;
				gameTimeStop();
				if (pie_GetRenderEngine() == ENGINE_GLIDE)
				{
					if (!gl_Deactivate())
					{
						quit = TRUE;
						Restart = TRUE;
					}
				}
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
				if (pie_GetRenderEngine() == ENGINE_GLIDE)
				{
					if (!gl_Reactivate())
					{
						quit = TRUE;
						Restart = TRUE;
					}
				}
				else if (pie_GetRenderEngine() == ENGINE_D3D)
				{
					dtm_RestoreTextures();
				}
				mixer_SaveWinVols();
				mixer_RestoreIngameVols();
				break;
			case FRAME_QUIT:
				quit = TRUE;
				Restart = TRUE;
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
								DBPRINTF(("GAMECODE_QUITGAME\n"));
								gameStatus = GS_TITLE_SCREEN;
								Restart = TRUE;
#ifdef NON_INTERACT
								quit = TRUE;
#endif

#ifdef WIN32
								if(NetPlay.bLobbyLaunched)
								{
//									changeTitleMode(QUIT);
									quit = TRUE;
								}
#endif
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
				DBERROR(("Video_mode no longer valid"));
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
					DBERROR(("Weirdy game status I'm afraid!!"));
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
				DBERROR(("Video_mode no longer valid"));
				if (videoInitialised)
				{
					videoInitialised = FALSE;
				}
				break;

			default:
				DBERROR(("Unknown game status on shutdown!"));
				break;
		}
	
	} // End of !quit loop.

	DBPRINTF(("Shuting down application\n"));

	systemShutdown();

	pal_ShutDown();

	frameShutDown();

	ShutdownGlideDLL();

	if (reInit) goto init;

	PostQuitMessage(0);

	return 0;

exit:

	DBPRINTF(("Shutting down after fail\n"));

	systemShutdown();

	pal_ShutDown();

	frameShutDown();

	ShutdownGlideDLL();

	PostQuitMessage(1);

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

