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
 * @file wrappers.c
 * Frontend loop & also loading screen & game over screen.
 * AlexL. Pumpkin Studios, EIDOS Interactive, 1997
 */
#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/strres.h"

#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/textdraw.h" //ivis text code
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_common/piemode.h"
#include "lib/ivis_common/piefunc.h"

#include "hci.h"		// access to widget screen.
#include "wrappers.h"
#include "main.h"
#include "objects.h"
#include "display.h"
#include "display3d.h"
#include "frontend.h"
#include "frend.h"		// display logo.
#include "console.h"
#include "intimage.h"
#include "intdisplay.h"	//for shutdown
#include "lib/sound/audio.h"
#include "lib/gamelib/gtime.h"
#include "ingameop.h"
#include "keymap.h"
#include "mission.h"
#include "keyedit.h"
#include "seqdisp.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_common/rendmode.h"
#include "lib/framework/cursors.h"
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

#define MAX_STARS 20
static STAR	stars[MAX_STARS];	// quick hack for loading stuff

static BOOL		firstcall = false;
static UDWORD	loadScreenCallNo=0;
static BOOL		bPlayerHasLost = false;
static BOOL		bPlayerHasWon = false;
static UBYTE    scriptWinLoseVideo = PLAY_NONE;

void	startCreditsScreen	( void );
void	runCreditsScreen	( void );

static	UDWORD	lastTick = 0;
static	UDWORD	lastChange = 0;


static void initStars(void)
{
	unsigned int i;

	for(i = 0; i < MAX_STARS; ++i)
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
	firstcall = true;
	initStars();

	return true;
}

// ///////////////// /////////////////////////////////////////////////
// Main Front end game loop.
TITLECODE titleLoop(void)
{
	TITLECODE RetCode = TITLECODE_CONTINUE;

	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	pie_SetFogStatus(false);
	screen_RestartBackDrop();

	if (firstcall)
	{
		firstcall = false;

		changeTitleMode(TITLE);

		// Set the default uncoloured cursor here, since it looks slightly
		// better for menus and such.
		pie_SetMouse(CURSOR_DEFAULT, false);
	}

	switch(titleMode) // run relevant title screen code.
	{
		// MULTIPLAYER screens
		case PROTOCOL:
			runConnectionScreen(); // multiplayer connection screen.
			break;
		case MULTIOPTION:
			runMultiOptions();
			break;
		case GAMEFIND:
			runGameFind();
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

		case GAME4:
			runGameOptions4Menu();
			break;

		case QUIT:
			RetCode = TITLECODE_QUITGAME;
			break;

		case STARTGAME:
		case LOADSAVEGAME:
			initLoadingScreen(true);//render active
  			if (titleMode == LOADSAVEGAME)
			{
				RetCode = TITLECODE_SAVEGAMELOAD;
			}
			else
			{
				RetCode = TITLECODE_STARTGAME;
			}
			return RetCode;			// don't flip!

		case SHOWINTRO:
			pie_SetFogStatus(false);
	  		pie_ScreenFlip(CLEAR_BLACK);
			changeTitleMode(TITLE);
			RetCode = TITLECODE_SHOWINTRO;
			break;

		default:
			debug( LOG_ERROR, "unknown title screen mode" );
			abort();
	}

	audio_Update();

	pie_SetFogStatus(false);
	pie_ScreenFlip(CLEAR_BLACK);//title loop

	if ((keyDown(KEY_LALT) || keyDown(KEY_RALT))
	    /* Check for toggling display mode */
	    && keyPressed(KEY_RETURN)) {
		screenToggleMode();
	}
	return RetCode;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Loading Screen.




//loadbar update
void loadingScreenCallback(void)
{
	unsigned int i;
	UDWORD			topX,topY,botX,botY;
	UDWORD			currTick;
	PIELIGHT		colour;

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
	colour.byte.r = 1;
	colour.byte.g = 1;
	colour.byte.b = 1;
	colour.byte.a = 32;
	pie_UniTransBoxFill(1, 1, 2, 2, colour);
	/* Draw the black rectangle at the bottom */

	topX = 10+D_W;
	topY = 450+D_H-1;
	botX = 630+D_W;
	botY = 470+D_H+1;
	colour.byte.a = 24;
	pie_UniTransBoxFill(topX, topY, botX, botY, colour);

	for(i = 1; i < MAX_STARS; ++i)
	{
	   	if(stars[i].xPos + stars[i].speed >=598)
		{
			stars[i].xPos = 1;
		}
		else
		{
			stars[i].xPos = (UWORD)(stars[i].xPos + stars[i].speed);
		}

		colour.byte.r = 200;
		colour.byte.g = 200;
		colour.byte.b = 200;
		colour.byte.a = 255;
		pie_UniTransBoxFill(10 + stars[i].xPos+D_W, 450 + i + D_H, 10 + stars[i].xPos + (2 * stars[i].speed) + D_W, 450 + i + 2 + D_H, colour);
   	}

	pie_ScreenFlip(CLEAR_OFF_AND_NO_BUFFER_DOWNLOAD);//loading callback		// dont clear.
	audio_Update();
}


// fill buffers with the static screen
void initLoadingScreen( BOOL drawbdrop )
{
	if (!drawbdrop)	// fill buffers
	{
		//just init the load bar with the current screen
		// setup the callback....
		pie_SetFogStatus(false);
		pie_ScreenFlip(CLEAR_BLACK);
		resSetLoadCallback(loadingScreenCallback);
		loadScreenCallNo = 0;
		return;
	}

	pie_SetFogStatus(false);
	pie_ScreenFlip(CLEAR_BLACK);//init loading

	// setup the callback....
	resSetLoadCallback(loadingScreenCallback);
	loadScreenCallNo = 0;

	screen_StopBackDrop();
}


// fill buffers with the static screen
void startCreditsScreen(void)
{
	SCREENTYPE	screen = SCREEN_CREDITS;

	lastChange = gameTime;
	// fill buffers

	pie_LoadBackDrop(screen);

	pie_SetFogStatus(false);
	pie_ScreenFlip(CLEAR_BLACK);//init loading
}

/* This function does nothing - since it's already been drawn */
void runCreditsScreen( void )
{
	// Check for key presses now.
	if( keyReleased(KEY_ESC)
	   || keyReleased(KEY_SPACE)
	   || mouseReleased(MOUSE_LMB)
	   || gameTime - lastChange > 4000 )
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
//	setConsolePermanence(true,true);
//	flushConsoleMessages( );

//	addConsoleMessage(" ", CENTRE_JUSTIFY, SYSTEM_MESSAGE);
//	addConsoleMessage(_("Game Over"), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
//	addConsoleMessage(" ", CENTRE_JUSTIFY, SYSTEM_MESSAGE);

	if(bDidit)
	{
		setPlayerHasWon(true);
		multiplayerWinSequence(true);
		if(bMultiPlayer)
		{
			updateMultiStatsWins();
		}
	}
	else
	{
		setPlayerHasLost(true);
		if(bMultiPlayer)
		{
			updateMultiStatsLoses();
		}
	}

	//clear out any mission widgets - timers etc that may be on the screen
	clearMissionWidgets();
	intAddMissionResult(bDidit, true);

	return true;
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
