/*
 * Wrappers.c
 * frontend loop & also loading screen & game over screen.
 * AlexL. Pumpkin Studios, EIDOS Interactive, 1997
 */

#include <stdio.h>

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/strres.h"

#include "lib/ivis_common/ivisdef.h" //ivis palette code
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/textdraw.h" //ivis text code
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/screen.h"


#include "lib/ivis_common/piemode.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_common/piefunc.h"


#include "hci.h"		// access to widget screen.
#include "lib/widget/widget.h"
#include "wrappers.h"
#include "winmain.h"
#include "objects.h"
#include "display.h"
#include "display3d.h"
#include "frontend.h"
#include "frend.h"		// display logo.
#include "console.h"
#include "intimage.h"
#include "text.h"
#include "intdisplay.h"	//for shutdown
#include "lib/sound/audio.h"
#include "audio_id.h"
#include "lib/gamelib/gtime.h"
#include "ingameop.h"
#include "keymap.h"
#include "mission.h"



#include "keyedit.h"
#include "seqdisp.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_common/rendmode.h"
#include "resource.h"
#include "lib/netplay/netplay.h"	// multiplayer
#include "multiplay.h"
#include "multiint.h"
#include "multistat.h"
#include "multilimit.h"

typedef struct _star
{
	UWORD	xPos;
	SDWORD	speed;
} STAR;


STAR	stars[30];	// quick hack for loading stuff



#define LOADBARY	460		// position of load bar.
#define LOADBARY2	470
#define LOAD_BOX_SHADES	6


extern int WFont;
extern BOOL bLoadSaveUp;

static BOOL		firstcall;
static UDWORD	loadScreenCallNo=0;
static BOOL		bPlayerHasLost = FALSE;
static BOOL		bPlayerHasWon = FALSE;
static UBYTE    scriptWinLoseVideo = PLAY_NONE;


void	startCreditsScreen	( BOOL bRenderActive);
void	runCreditsScreen	( void );
UDWORD	lastTick=0;



void	initStars( void )
{
UDWORD	i;
	for(i=0; i<20; i++)
	{
		stars[i].xPos = (UWORD)(rand()%598);		// scatter them
		stars[i].speed = rand()%10+2;	// always move
	}
}

// //////////////////////////////////////////////////////////////////
// Initialise frontend globals and statics.
//
BOOL frontendInitVars(void)
{
	firstcall = TRUE;

	initStars();


	return TRUE;
}

// ///////////////// /////////////////////////////////////////////////
// Main Front end game loop.
TITLECODE titleLoop(void)
{
	TITLECODE RetCode = TITLECODE_CONTINUE;

	pie_GlobalRenderBegin();
	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	pie_SetFogStatus(FALSE);
	screen_RestartBackDrop();

	if (firstcall) {
		startTitleMenu();
		titleMode = TITLE;
		firstcall = FALSE;

		frameSetCursorFromRes(IDC_DEFAULT);						// reset cursor	(sw)

		if(NetPlay.bLobbyLaunched)					// lobbies skip title screens & go into the game
		{
			if (NetPlay.bHost)
			{
				ingame.bHostSetup = TRUE;
			}
			else
			{
				ingame.bHostSetup = FALSE;
			}

			changeTitleMode(QUIT);
		}
		else if(gameSpy.bGameSpy)
		{
			// set host
			if (NetPlay.bHost)
			{
				ingame.bHostSetup = TRUE;
			}
			else
			{
				ingame.bHostSetup = FALSE;
			}
			// set protocol
			// set address
			// if host goto options.
			// if client goto game find.
			if(NetPlay.bHost)
			{
				changeTitleMode(MULTIOPTION);
			}
			else
			{
				changeTitleMode(GAMEFIND);
			}
		}

	}

	switch(titleMode)								// run relevant title screen code.
	{

										// MULTIPLAYER screens
		case PROTOCOL:
			runConnectionScreen();					// multiplayer connection screen.
			break;
		case MULTIOPTION:
			runMultiOptions();
			break;
		case GAMEFIND:
			runGameFind();
			break;
		case FORCESELECT:
			runForceSelect();
			break;
		case MULTI:
			runMultiPlayerMenu();
			break;
		case MULTILIMIT:
			runLimitScreen();
			break;
		case KEYMAP:
			runKeyMapEditor();
			break;


		case TITLE:
			runTitleMenu();
			break;

		case SINGLE:
			runSinglePlayerMenu();
			break;

		case TUTORIAL:
			runTutorialMenu();
			break;


//		case GRAPHICS:
//			runGraphicsOptionsMenu();
//			break;

		case CREDITS:
			runCreditsScreen();
			break;

//		case DEMOMODE:
//			runDemoMenu();
//			break;
//	case VIDEO:
//			runVideoOptionsMenu();
//			break;
		case OPTIONS:
			runOptionsMenu();
			break;

		case GAME:
			runGameOptionsMenu();
			break;


		case GAME2:
			runGameOptions2Menu();
			break;

		case GAME3:
			runGameOptions3Menu();
			break;


		case QUIT:
			RetCode = TITLECODE_QUITGAME;
			break;

		case STARTGAME:
		case LOADSAVEGAME:

			initLoadingScreen(TRUE,TRUE);//render active

  			if (titleMode == LOADSAVEGAME)
			{
				RetCode = TITLECODE_SAVEGAMELOAD;
			}
			else
			{
				RetCode = TITLECODE_STARTGAME;
			}

			pie_GlobalRenderEnd(TRUE);//force to black

			return RetCode;			// don't flip!
			break;

		case SHOWINTRO:

			pie_SetFogStatus(FALSE);
			pie_ScreenFlip(CLEAR_BLACK);//flip to clear screen but not here//reshow intro video.
	  		pie_ScreenFlip(CLEAR_BLACK);//flip to clear screen but not here

			changeTitleMode(TITLE);
			RetCode = TITLECODE_SHOWINTRO;
			break;

		default:
			debug( LOG_ERROR, "unknown title screen mode" );
			abort();
	}

	audio_Update();


	pie_GlobalRenderEnd(FALSE);//force to black  //[movie]  --We got no videos yet, so better to display the backdrop... -Q
	pie_SetFogStatus(FALSE);
	pie_ScreenFlip(CLEAR_BLACK);//title loop

	if ((keyDown(KEY_LALT) || keyDown(KEY_RALT))
	    /* Check for toggling display mode */
	    && keyPressed(KEY_RETURN)) {
		screenToggleMode();
	}
	SDL_Delay(30);	//To fix ALL menus to be less CPU hogging. -Q 5-14-05
	return RetCode;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Loading Screen.




//loadbar update
void loadingScreenCallback(void)
{
	UDWORD			i;
	UDWORD			topX,topY,botX,botY;
	UDWORD			currTick;


	if(SDL_GetTicks()-lastTick < 16) {
		return;
	}
	currTick = SDL_GetTicks();
	if ((currTick - lastTick) > 500)
	{
		currTick -= lastTick;
		debug( LOG_NEVER, "loadingScreenCallback: pause %d\n", currTick );
	}
	lastTick = SDL_GetTicks();
	pie_GlobalRenderBegin();
	DrawBegin();
	pie_UniTransBoxFill(1, 1, 2, 2, 0x00010101, 32);
	/* Draw the black rectangle at the bottom */

	topX = 10+D_W;
	topY = 450+D_H-1;
	botX = 630+D_W;
	botY = 470+D_H+1;
//	pie_BoxFillIndex(10+D_W,450+D_H-1,630+D_W,470+D_H+1,COL_BLACK);
	pie_UniTransBoxFill(topX,topY,botX,botY,0x00010101, 24);

	for(i=1; i<19; i++)
	{
	   	if(stars[i].xPos + stars[i].speed >=598)
		{
			stars[i].xPos = 1;
		}
		else
		{
			stars[i].xPos = (UWORD)(stars[i].xPos + stars[i].speed);
		}
		pie_UniTransBoxFill(10+stars[i].xPos+D_W,450+i+D_H,10+stars[i].xPos+(2*stars[i].speed)+D_W,450+i+2+D_H,0x00ffffff, 255);

   	}

	DrawEnd();
	pie_GlobalRenderEnd(TRUE);//force to black
	pie_ScreenFlip(CLEAR_OFF_AND_NO_BUFFER_DOWNLOAD);//loading callback		// dont clear.
	audio_Update();
}


// fill buffers with the static screen
void initLoadingScreen( BOOL drawbdrop, BOOL bRenderActive)
{
	if (!drawbdrop)	// fill buffers
	{
		//just init the load bar with the current screen
		// setup the callback....
		pie_SetFogStatus(FALSE);
		pie_ScreenFlip(CLEAR_BLACK);//init loading
		pie_ScreenFlip(CLEAR_BLACK);//init loading
		resSetLoadCallback(loadingScreenCallback);
		loadScreenCallNo = 0;
		return;
	}
	if (bRenderActive)
	{
		pie_GlobalRenderEnd(TRUE);//force to black
	}

	pie_ResetBackDrop();

	pie_SetFogStatus(FALSE);
	pie_ScreenFlip(CLEAR_BLACK);//init loading
	pie_ScreenFlip(CLEAR_BLACK);//init loading

	// setup the callback....
	resSetLoadCallback(loadingScreenCallback);
	loadScreenCallNo = 0;

	screen_StopBackDrop();

	if (bRenderActive)
	{
		pie_GlobalRenderBegin();
	}

}

UDWORD lastChange = 0;

// fill buffers with the static screen
void startCreditsScreen( BOOL bRenderActive)
{
	SCREENTYPE	screen = SCREEN_CREDITS;

	lastChange = gameTime;
	// fill buffers

	{
		pie_LoadBackDrop(screen,FALSE);
	}

	if (bRenderActive)
	{
		pie_GlobalRenderEnd(TRUE);//force to black
	}

	pie_SetFogStatus(FALSE);
	pie_ScreenFlip(CLEAR_BLACK);//flip to set back buffer
	pie_ScreenFlip(CLEAR_BLACK);//init loading

	if (bRenderActive)
	{
		pie_GlobalRenderBegin();
	}
}

/* This function does nothing - since it's already been drawn */
void	runCreditsScreen( void )
{
	// Check for key presses now.
	if(keyReleased(KEY_ESC)
	   || keyReleased(KEY_SPACE)
	   || mouseReleased(MOUSE_LMB)
	   || (gameTime-lastChange > 4000)
	   )
	{
		lastChange = gameTime;
		changeTitleMode(QUIT);
	}
	return;
}

// shut down the loading screen
void closeLoadingScreen(void)
{
	resSetLoadCallback(NULL);
	loadScreenCallNo = 0;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Gameover screen.

BOOL displayGameOver(BOOL bDidit)
{


// AlexL says take this out......
//	setConsolePermanence(TRUE,TRUE);
//	flushConsoleMessages( );

//	addConsoleMessage(" ", CENTRE_JUSTIFY );
//	addConsoleMessage(strresGetString(psStringRes,STR_GAM_GAMEOVER), CENTRE_JUSTIFY );
//	addConsoleMessage(" ", CENTRE_JUSTIFY );

    //set this for debug mode too - what gets displayed then depends on whether we
    //have won or lost and if we are in debug mode

	// this bit decides whether to auto quit to front end.
	//if(!getDebugMappingStatus())
	{
		if(bDidit)
		{
			setPlayerHasWon(TRUE);	// quit to frontend..
		}
		else
		{
			setPlayerHasLost(TRUE);
		}
	}

	if(bMultiPlayer)
	{
		if(bDidit)
		{
			updateMultiStatsWins();
			multiplayerWinSequence(TRUE);
		}
		else
		{
			updateMultiStatsLoses();
		}
	}

//	if(getDebugMappingStatus())
//	{
//		intAddInGameOptions();
//	}
	else

	{

        //clear out any mission widgets - timers etc that may be on the screen
        clearMissionWidgets();
		intAddMissionResult(bDidit, TRUE);
    }

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////
BOOL testPlayerHasLost( void )
{
	return(bPlayerHasLost);
}

void setPlayerHasLost( BOOL val )
{
	bPlayerHasLost = val;
}


////////////////////////////////////////////////////////////////////////////////
BOOL testPlayerHasWon( void )
{
	return(bPlayerHasWon);
}

void setPlayerHasWon( BOOL val )
{
	bPlayerHasWon = val;
}

/*access functions for scriptWinLoseVideo - used to indicate when the script is playing the win/lose video*/
void setScriptWinLoseVideo(UBYTE val)
{
    scriptWinLoseVideo = val;
}

UBYTE getScriptWinLoseVideo(void)
{
    return scriptWinLoseVideo;
}
