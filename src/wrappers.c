/*
 * Wrappers.c   
 * frontend loop & also loading screen & game over screen. 
 * AlexL. Pumpkin Studios, EIDOS Interactive, 1997 
 */

#include <stdio.h>

#include "Frame.h"
#include "ivisdef.h" //ivis palette code
#include "pieState.h"
#include "textdraw.h" //ivis text code

#ifdef WIN32
#include "piemode.h"
#include "pieMatrix.h"
#include "pieFunc.h"
#endif

#include "hci.h"		// access to widget screen.
#include "widget.h"
#include "Wrappers.h"
#include "WinMain.h"
#include "Objects.h"
#include "Display.h"
#include "Display3d.h"
#include "FrontEnd.h"
#include "Frend.h"		// display logo.
#include "console.h"
#include "intimage.h"
#include "text.h"
#include "intdisplay.h"	//for shutdown
#include "audio.h"		
#include "audio_id.h"		
#include "gtime.h"
#include "ingameop.h"
#include "KeyMap.h"
#include "mission.h"

#ifdef WIN32
#include "keyedit.h"
#include "SeqDisp.h"
#include "3dfxfunc.h"
#include "vid.h"
#include "config.h"
#include "resource.h"
#include "netPlay.h"	// multiplayer 
#include "multiplay.h"
#include "Multiint.h"				
#include "multistat.h"
#include "multilimit.h"
#endif

#ifdef PSX
#include "map.h"
#include "display3d_psx.h"
#include "libsn.h"
#include "vid.h"
#include "Primatives.h"
#include "VPad.h"
#include "CtrlPSX.h"
#include "InitPSX.h"

/* callback type for res pre-load callback*/
typedef void (*SPECIALVBLCALLBACK)(void);

void SetSpecialVblCallback( SPECIALVBLCALLBACK routine);


extern BOOL fastExit;
extern BOOL IsMouseDrawEnabled(void);


#endif

#ifdef WIN32   


extern void frontEndCheckCD( tMode tModeNext, CD_INDEX cdIndex );

typedef struct _star
{
	UWORD	xPos;
	SDWORD	speed;
} STAR;


STAR	stars[30];	// quick hack for loading stuff
#endif

#ifdef WIN32
#define LOADBARY	460		// position of load bar.
#define LOADBARY2	470
#define LOAD_BOX_SHADES	6
#else
#define LOADBARY	(460-16)		// position of load bar.
#define LOADBARY2	(470-16)
#define LOAD_BOX_SHADES	6
#endif

extern IMAGEFILE	*FrontImages;
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


#ifdef WIN32
void	initStars( void )
{
UDWORD	i;
	for(i=0; i<20; i++)
	{
		stars[i].xPos = (UWORD)(rand()%598);		// scatter them
		stars[i].speed = rand()%10+2;	// always move
	}
}
#endif
// //////////////////////////////////////////////////////////////////
// Initialise frontend globals and statics.
//
BOOL frontendInitVars(void)
{
	firstcall = TRUE;
#ifdef WIN32
	initStars();
#endif

	return TRUE;
}

// //////////////////////////////////////////////////////////////////
// play the intro if first EVER boot.
BOOL playIntroOnInstall( VOID )
{
#ifdef WIN32
	DWORD result;

	if(!openWarzoneKey())
		return FALSE;

	if(!getWarzoneKeyNumeric("nointro",&result))
	{
		setWarzoneKeyNumeric("nointro",1);
		closeWarzoneKey();

		frontEndCheckCD(SHOWINTRO, DISC_ONE);
		return TRUE;
	}
	else
	{
		closeWarzoneKey();
		return FALSE;
	}

#else
	return FALSE;
#endif
}

// ///////////////// /////////////////////////////////////////////////
// Main Front end game loop.
TITLECODE titleLoop(void)
{
	TITLECODE RetCode = TITLECODE_CONTINUE;
#ifdef PSX
	StartScene();	// Setup all the primative handling for this frame
#ifdef DEBUG
	pollhost();
#endif
	iV_SetScaleFlags_PSX(IV_SCALE_POSITION | IV_SCALE_SIZE);
#endif


#ifdef WIN32
	pie_GlobalRenderBegin();
	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	pie_SetFogStatus(FALSE);
	screen_RestartBackDrop();
#else
	if(GetControllerType(0) == CON_MOUSE) {
		EnableMouseDraw(TRUE);
	}
#endif

	if(firstcall)
	{	
#ifndef COVERMOUNT
		if ( playIntroOnInstall() == FALSE )
#endif
		{
			startTitleMenu();
			titleMode = TITLE;
		}

		firstcall = FALSE;

		pie_SetMouse(IntImages,IMAGE_CURSOR_DEFAULT);			// reset cursor (hw)

#ifdef WIN32

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

			if(NetPlay.lpDirectPlay4A)		// make sure lobby is valid.
			{
				changeTitleMode(MULTIOPTION);
			}
			else
			{
				changeTitleMode(QUIT);
			}
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
#endif
	}

	switch(titleMode)								// run relevant title screen code.
	{

#ifdef WIN32										// MULTIPLAYER screens
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
#endif

		case TITLE:
			runTitleMenu();
			break;	

		case SINGLE:
			runSinglePlayerMenu();
			break;

		case TUTORIAL:
			runTutorialMenu();
			break;

#ifdef WIN32
//		case GRAPHICS:
//			runGraphicsOptionsMenu();
//			break;

		case CREDITS:
			runCreditsScreen();
			break;
#endif
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

#ifdef WIN32
		case GAME2:
			runGameOptions2Menu();
			break;
#endif

		case QUIT:
			RetCode = TITLECODE_QUITGAME;
			break;
		
		case STARTGAME:	
		case LOADSAVEGAME:
#ifdef WIN32
			initLoadingScreen(TRUE,TRUE);//render active
#endif
  			if (titleMode == LOADSAVEGAME)
			{
				RetCode = TITLECODE_SAVEGAMELOAD;
			}
			else
			{
				RetCode = TITLECODE_STARTGAME;
			}
#ifdef WIN32
			pie_GlobalRenderEnd(TRUE);//force to black
#endif
			return RetCode;			// don't flip!
			break;

		case SHOWINTRO:
#ifdef PSX		// ffs js
			screenFlip(TRUE);//flip to clear screen but not here//reshow intro video.
	  		screenFlip(TRUE);//flip to clear screen but not here
#else
			pie_SetFogStatus(FALSE);
			pie_ScreenFlip(CLEAR_BLACK);//flip to clear screen but not here//reshow intro video.
	  		pie_ScreenFlip(CLEAR_BLACK);//flip to clear screen but not here
#endif
			changeTitleMode(TITLE);
			RetCode = TITLECODE_SHOWINTRO;
			break;

		default:
			DBERROR(("unknown title screen mode "));
	}

	audio_Update();

#ifdef WIN32			
	if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		if(!bUsingKeyboard)
		{
			pie_DrawMouse(mouseX(),mouseY());  //iV_DrawMousePointer(mouseX(),mouseY());
		}
	}
	pie_GlobalRenderEnd(TRUE);//force to black
	pie_SetFogStatus(FALSE);
	pie_ScreenFlip(CLEAR_BLACK);//title loop

//#ifdef WIN32
	if ((keyDown(KEY_LALT) || keyDown(KEY_RALT)) &&	/* Check for toggling display mode */
		keyPressed(KEY_RETURN) AND pie_GetRenderEngine()!=ENGINE_GLIDE)
	{
		screenToggleMode();
	}
//#endif

#else
	if(GetControllerType(0) == CON_MOUSE) {
		if(IsMouseDrawEnabled()) {
	//	if(MouseDrawEnabled) {
			iV_SetOTIndex_PSX(0);
			pie_DrawMouse(mouseX(),mouseY());
		}
	}

 	EndScene();		// finalise the primative for this frame (start drawing)
#endif // End of ifdef PSX
	return RetCode;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Loading Screen.


#ifdef WIN32

//loadbar update
void loadingScreenCallback(void)
{
	UDWORD			i;
	UDWORD			topX,topY,botX,botY;
	UDWORD			currTick;
  
	
	if(GetTickCount()-lastTick < 16)	// cos 1000/60Hz gives roughly 16...:-)
	{
		return;
	}
 	currTick = GetTickCount();
	if ((currTick - lastTick) > 500)
	{
		currTick -= lastTick;
		DBPRINTF(("loadingScreenCallback: pause %d\n", currTick));
	}
	lastTick = GetTickCount();
	pie_GlobalRenderBegin();
	DrawBegin();
  	if(pie_GetRenderEngine() == ENGINE_D3D)
	{
		pie_UniTransBoxFill(1,1,2,2,0x00010101, 32);
	}
	/* Draw the black rectangle at the bottom */

	topX = 10+D_W;
	topY = 450+D_H-1;
	botX = 630+D_W;
	botY = 470+D_H+1;
//	pie_BoxFillIndex(10+D_W,450+D_H-1,630+D_W,470+D_H+1,COL_BLACK);
	if (pie_Hardware())
	{
		pie_UniTransBoxFill(topX,topY,botX,botY,0x00010101, 24);
	}
	else
	{
		pie_BoxFillIndex(topX,topY,botX,botY,COL_BLACK);
	}


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
		if (pie_Hardware())
		{
			pie_UniTransBoxFill(10+stars[i].xPos+D_W,450+i+D_H,10+stars[i].xPos+(2*stars[i].speed)+D_W,450+i+2+D_H,0x00ffffff, 255);
		}
		else
		{
	   	  	pie_BoxFillIndex(10+stars[i].xPos+D_W,450+i+D_H,10+stars[i].xPos+2+D_W,450+i+2+D_H,COL_WHITE);
		}

   	}
	
	DrawEnd();
	pie_GlobalRenderEnd(TRUE);//force to black
	pie_ScreenFlip(CLEAR_OFF_AND_NO_BUFFER_DOWNLOAD);//loading callback		// dont clear.
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
#ifdef COVERMOUNT
	SCREENTYPE	screen = SCREEN_SLIDE1;
#else
	SCREENTYPE	screen = SCREEN_CREDITS;
#endif

	lastChange = gameTime;
	// fill buffers
	if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		pie_LoadBackDrop(screen,TRUE);
	}
	else
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
	static UBYTE quitstage=0;
#ifdef COVERMOUNT
	SCREENTYPE	screen;
#endif
	// Check for key presses now.

	if(keyReleased(KEY_ESC) 
	   || keyReleased(KEY_SPACE)
	   || mouseReleased(MOUSE_LMB)
	   || (gameTime-lastChange > 4000)
	   )
	{
		lastChange = gameTime;
#ifdef COVERMOUNT
		quitstage++;		
		switch(quitstage)
		{
		case 1:
			screen = SCREEN_SLIDE2;
			break;
		case 2:
			screen = SCREEN_SLIDE3;
			break;
		case 3:
			screen = SCREEN_SLIDE4;
			break;
//		case 4:
//			screen = SCREEN_SLIDE5;
//			break;		
		case 4:
			screen = SCREEN_CREDITS;
			break;
		case 5:
			default:
			changeTitleMode(QUIT);
			return;
			break;
		}		
		if (pie_GetRenderEngine() == ENGINE_GLIDE)
		{
			pie_LoadBackDrop(screen,TRUE);
		}
		else
		{
			pie_LoadBackDrop(screen,FALSE);
		}
		pie_SetFogStatus(FALSE);
		pie_ScreenFlip(CLEAR_BLACK);//flip to set back buffer
		pie_ScreenFlip(CLEAR_BLACK);//init loading
#else
		changeTitleMode(QUIT);
#endif	

	}
	return;
}

// shut down the loading screen
void closeLoadingScreen(void)
{
	resSetLoadCallback(NULL);
	loadScreenCallNo = 0;
}

#else	// PSX Version.

extern void DrawBoxNow(int x1, int y1, int x2, int y2, int r,int g,int b);

#define LOADINGBARDELAY (100)
//loadbar update
void loadingScreenCallback(void)
{
	static UDWORD	lastdraw=0;
	UDWORD			x;
	UDWORD			y = 230;
	UDWORD			i;
	if(clock()-lastdraw < LOADINGBARDELAY ) 
	{
		return;
	}

	// Set the draw environment to draw to the front buffer.
	// This get's reset the next time we do a StartScene,EndScene
	SetFrontBufferDraw();

	lastdraw = clock();
	loadScreenCallNo +=8;

//	StartScene();	// Setup all the primative handling for this frame

	iV_SetScaleFlags_PSX(IV_SCALE_POSITION | IV_SCALE_SIZE);
	DrawBoxNow( 9+8 ,LOADBARY-2  ,631-8,  LOADBARY2+2 , 128,128,128);	//COL_GREEN);		// draw blue box	
	DrawBoxNow( 11+8,LOADBARY  , 629-8, LOADBARY2 ,0,0,0);	//1); // COL_BLACK);	// draw black box
	x = (10+8)+(loadScreenCallNo % (620-24-16) );							// draw sweep.
	for(i=0; i<LOAD_BOX_SHADES; i++)
	{
		DrawBoxNow(x+(i*4),LOADBARY,x+(i*4)+4,LOADBARY2,
			i*30,i*30,i*30);
	}

//	// Restore normal backbuffer draw.
//	SetBackBufferDraw();

//BPRINTF(("loadingscreencallback\n"));
//EndScene();		// finalise the primative for this frame (start drawing)
// 	EndSceneSpecial();		// finalise the primative for this frame (start drawing)
	return;
}

extern UBYTE *LoadBackdrop(char *FileName,BOOL UsePrimBuffer);
extern BOOL UnloadBackdrop(void);
extern void StartBackdropDisplay(void);
extern void StopBackdropDisplay(void);

// fill buffers with the static screen
//void initLoadingScreen( BOOL drawbdrop, BOOL bRenderActive)
//{
//}

// fill buffers with the static screen
void initLoadingScreen( BOOL drawbdrop, BOOL bRenderActive)
{
	int i;
	char TitleText[] = {"Warzone 2100"};

	DBPRINTF(("initLoadingScreen\n"));

	SetISBG(0);		// was 0 ,Dissable background clear.

	// Load the backdrop into system memory.
//	LoadBackdrop("loading.tim",TRUE);
//	// Set it's update function.
//	StartBackdropDisplay();

//	SetISBG(0);		// Dissable background clear.

	StopBackdropDisplay();
	DrawSync(0);

	// flip to lowres for this screen.
	SetDisplaySize(DISPLAY_WIDTH,DISPLAY_HEIGHT);
	EnableMouseDraw(FALSE);

	// make sure the backdrop has been downloaded on both display buffers.
	for(i=0; i<2; i++) {
		StartScene();
		ResetBlueWash();
		ClearBlueWash(FALSE);
		iV_SetFont(WFont);
		iV_SetOTIndex_PSX(OT2D_FARFORE);
		iV_SetTextColour(-1);
		iV_DrawText(TitleText, (640-iV_GetTextWidth(TitleText))/2,96);
		iV_DrawText(strresGetString(psStringRes,STR_GAM_LOADING),
					(640-iV_GetTextWidth(strresGetString(psStringRes,STR_GAM_LOADING)))/2,96+32);
	 	EndScene();
	}

	StartScene();
	EndScene();
	DrawSync(0);

//	// Stop downloading backdrop.
//	StopBackdropDisplay();
//	// And remove it from memory.
//	UnloadBackdrop();

	// setup the callback....
	SetSpecialVblCallback(loadingScreenCallback);
	loadScreenCallNo = 0;
}


void StartLoadingBar(void)
{
	SetSpecialVblCallback(loadingScreenCallback);
}


void StopLoadingBar(void)
{
	SetSpecialVblCallback(NULL);
}



// shut down the loading screen
void closeLoadingScreen(void)
{
	DBPRINTF(("closeLoadingScreen\n"));
	SetSpecialVblCallback(NULL);
	loadScreenCallNo = 0;

	SetISBG(ENABLE_ISBG);		// was 0 Enable background clear (NOT).
	RemoveLoadingBackdrop();
}

#endif	// End of PSX version.



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Gameover screen.

BOOL displayGameOver(BOOL bDidit)
{

#ifdef WIN32
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
#endif
	{
#ifdef PSX
		if(bDidit)
		{
			setPlayerHasWon(TRUE);	// quit to frontend..
		}
		else
		{
			setPlayerHasLost(TRUE);
		}
#endif
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