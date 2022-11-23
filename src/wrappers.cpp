/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"	// multiplayer
#include "lib/sound/audio.h"
#include "lib/framework/wzapp.h"

#include "clparse.h"
#include "frontend.h"
#include "keyedit.h"
#include "mission.h"
#include "multiint.h"
#include "multilimit.h"
#include "multistat.h"
#include "warzoneconfig.h"
#include "wrappers.h"
#include "titleui/titleui.h"

struct STAR
{
	int      xPos;
	int      speed;
	PIELIGHT colour;
};

static bool		firstcall = false;
static bool		bPlayerHasLost = false;
static bool		bPlayerHasWon = false;
static UBYTE    scriptWinLoseVideo = PLAY_NONE;

static HostLaunch hostlaunch = HostLaunch::Normal;  // used to detect if we are hosting a game via command line option.
static bool bHeadlessAutoGameModeCLIOption = false;
static bool bActualHeadlessAutoGameMode = false;

static uint32_t lastTick = 0;
static int barLeftX, barLeftY, barRightX, barRightY, boxWidth, boxHeight, starsNum, starHeight;
static STAR *stars = nullptr;

static STAR newStar()
{
	STAR s;
	s.xPos = rand() % barRightX;
	s.speed = static_cast<int>((rand() % 30 + 6) * pie_GetVideoBufferWidth() / 640.0);
	s.colour = pal_SetBrightness(150 + rand() % 100);
	return s;
}

static void setupLoadingScreen()
{
	unsigned int i;
	int w = pie_GetVideoBufferWidth();
	int h = pie_GetVideoBufferHeight();
	int offset;

	boxHeight = static_cast<int>(h / 40.0);
	offset = boxHeight;
	boxWidth = w - 2 * offset;

	barRightX = w - offset;
	barRightY = h - offset;

	barLeftX = barRightX - boxWidth;
	barLeftY = barRightY - boxHeight;

	starsNum = boxWidth / std::max<int>(boxHeight, 1);
	starHeight = static_cast<int>(2.0 * h / 640.0);

	if (!stars)
	{
		stars = (STAR *)malloc(sizeof(STAR) * starsNum);
	}

	for (i = 0; i < starsNum; ++i)
	{
		stars[i] = newStar();
	}
}

bool recalculateEffectiveHeadlessValue()
{
	if (hostlaunch == HostLaunch::Skirmish || hostlaunch == HostLaunch::Autohost || hostlaunch == HostLaunch::LoadReplay || autogame_enabled())
	{
		// only support headless mode if hostlaunch is --skirmish or --autogame
		return bHeadlessAutoGameModeCLIOption;
	}
	return false;
}

void setHostLaunch(HostLaunch value)
{
	hostlaunch = value;
	bActualHeadlessAutoGameMode = recalculateEffectiveHeadlessValue();
}

HostLaunch getHostLaunch()
{
	return hostlaunch;
}

void setHeadlessGameMode(bool enabled)
{
	bHeadlessAutoGameModeCLIOption = enabled;
	bActualHeadlessAutoGameMode = recalculateEffectiveHeadlessValue();
}

bool headlessGameMode()
{
	return bActualHeadlessAutoGameMode;
}


// //////////////////////////////////////////////////////////////////
// Initialise frontend globals and statics.
//
bool frontendInitVars()
{
	firstcall = true;

	return true;
}

// ///////////////// /////////////////////////////////////////////////
// Main Front end game loop.
TITLECODE titleLoop()
{
	TITLECODE RetCode = TITLECODE_CONTINUE;

	pie_SetFogStatus(false);
	if (screen_RestartBackDrop())
	{
		// changed value - draw the backdrop
		// otherwise, pie_ScreenFrameRenderBegin handles drawing it
		screen_Display();
	}
	wzShowMouse(true);

	// When we first init the game, firstcall is true.
	if (firstcall)
	{
		firstcall = false;
		// First check to see if --host was given as a command line option, if not,
		// then check --join and if neither, run the normal game menu.
		if (hostlaunch != HostLaunch::Normal)
		{
			if (hostlaunch == HostLaunch::Skirmish)
			{
				SPinit(LEVEL_TYPE::SKIRMISH);
			}
			else // single player
			{
				NetPlay.bComms = true; // use network = true
				NetPlay.isUPNP_CONFIGURED = false;
				NetPlay.isUPNP_ERROR = false;
				bMultiMessages = true;
				NETinit(true);
				NETdiscoverUPnPDevices();
			}
			bMultiPlayer = true;
			ingame.side = InGameSide::HOST_OR_SINGLEPLAYER;
			game.type = LEVEL_TYPE::SKIRMISH;
			// Ensure the game has a place to return to
			changeTitleMode(TITLE);
			changeTitleUI(std::make_shared<WzMultiplayerOptionsTitleUI>(wzTitleUICurrent));
		}
		else if (strlen(iptoconnect))
		{
			NetPlay.bComms = true; // use network = true
			NETinit(true);
			// Ensure the joinGame has a place to return to
			changeTitleMode(TITLE);
			joinGame(iptoconnect, cliConnectToIpAsSpectator);
		}
		else
		{
			changeTitleMode(TITLE);			// normal game, run main title screen.
		}
		// Using software cursors (when on) for these menus due to a bug in SDL's SDL_ShowCursor()
		wzSetCursor(CURSOR_DEFAULT);
	}

	if (wzTitleUICurrent)
	{
		// Creates a pointer, so if... when, the UI changes during a run, this does not disappear
		std::shared_ptr<WzTitleUI> current = wzTitleUICurrent;
		RetCode = current->run();
	}

	if ((RetCode == TITLECODE_SAVEGAMELOAD) || (RetCode == TITLECODE_STARTGAME))
	{
		return RetCode; // don't flip
	}
	NETflush();  // Send any pending network data.

	audio_Update();

	pie_SetFogStatus(false);

	if ((keyDown(KEY_LALT) || keyDown(KEY_RALT)) && keyPressed(KEY_RETURN))
	{
		war_setWindowMode(wzAltEnterToggleFullscreen());
	}
	return RetCode;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Loading Screen.

//loadbar update
void loadingScreenCallback()
{
	const PIELIGHT loadingbar_background = WZCOL_LOADING_BAR_BACKGROUND;
	const uint32_t currTick = wzGetTicks();
	unsigned int i;

	if (currTick - lastTick < 50)
	{
		return;
	}

	lastTick = currTick;

	/* Draw the black rectangle at the bottom, with a two pixel border */
	pie_UniTransBoxFill(barLeftX - 2, barLeftY - 2, barRightX + 2, barRightY + 2, loadingbar_background);

	for (i = 1; i < starsNum; ++i)
	{
		stars[i].xPos = stars[i].xPos + stars[i].speed;
		if (barLeftX + stars[i].xPos >= barRightX)
		{
			stars[i] = newStar();
			stars[i].xPos = 1;
		}
		{
			const int topX = barLeftX + stars[i].xPos;
			const int topY = barLeftY + i * (boxHeight - starHeight) / starsNum;
			const int botX = MIN(topX + stars[i].speed, barRightX);
			const int botY = topY + starHeight;

			pie_UniTransBoxFill(topX, topY, botX, botY, stars[i].colour);
		}
	}

	pie_ScreenFrameRenderEnd();
	pie_ScreenFrameRenderBegin();

	audio_Update();

	wzPumpEventsWhileLoading();
}

// fill buffers with the static screen
void initLoadingScreen(bool drawbdrop)
{
	pie_ScreenFrameRenderBegin(); // start a frame *if one isn't yet started*
	setupLoadingScreen();
	wzShowMouse(false);
	pie_SetFogStatus(false);

	// setup the callback....
	resSetLoadCallback(loadingScreenCallback);

	if (drawbdrop)
	{
		if (!screen_GetBackDrop())
		{
			pie_LoadBackDrop(SCREEN_RANDOMBDROP);
		}
		screen_RestartBackDrop();
	}
	else
	{
		screen_StopBackDrop();
	}
}

// shut down the loading screen
void closeLoadingScreen()
{
	if (stars)
	{
		free(stars);
		stars = nullptr;
	}
	resSetLoadCallback(nullptr);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Gameover screen.

bool displayGameOver(bool bDidit, bool showBackDrop)
{
	bool isFirstCallForThisGame = !testPlayerHasLost() && !testPlayerHasWon();
	if (bMultiPlayer)
	{
		// This is a bit of a hack and partially relies upon the logic in endconditions.js
		bool isGameFullyOver =
			NetPlay.players[selectedPlayer].isSpectator	// gameOverMessage is only called for spectators when the game fully ends
			|| bDidit; // can only win when the game actually ends :)
		if (isGameFullyOver && !ingame.endTime.has_value())
		{
			ingame.endTime = std::chrono::steady_clock::now();
			debug(LOG_INFO, "Game ended (duration: %lld)", (long long)std::chrono::duration_cast<std::chrono::seconds>(ingame.endTime.value() - ingame.startTime).count());
		}
	}
	if (bDidit)
	{
		setPlayerHasWon(true);
		multiplayerWinSequence(true);
		if (bMultiPlayer)
		{
			updateMultiStatsWins();
		}
	}
	else
	{
		setPlayerHasLost(true);
		if (bMultiPlayer && isFirstCallForThisGame) // make sure we only accumulate one loss (even if this is called more than once, for example when losing initially, and then when the game fully ends)
		{
			updateMultiStatsLoses();
		}
	}
	if (bMultiPlayer && isFirstCallForThisGame)
	{
		updateMultiStatsGames(); // update games played.

		PLAYERSTATS st = getMultiStats(selectedPlayer);
		saveMultiStats(getPlayerName(selectedPlayer), getPlayerName(selectedPlayer), &st);
	}

	//clear out any mission widgets - timers etc that may be on the screen
	clearMissionWidgets();

	if (bMultiPlayer && NetPlay.players[selectedPlayer].isSpectator)
	{
		// Special message for spectators to inform them that the game is fully over
		addConsoleMessage(_("GAME OVER"), CENTRE_JUSTIFY, SYSTEM_MESSAGE, false, MAX_CONSOLE_MESSAGE_DURATION);
		addConsoleMessage(_("The battle is over - you can leave the room."), CENTRE_JUSTIFY, SYSTEM_MESSAGE, false, MAX_CONSOLE_MESSAGE_DURATION);
		// TODO: Display this in a form with a "Quit to Main Menu" button?, or adapt intAddMissionResult to have a separate display for spectators?
	}
	else
	{
		intAddMissionResult(bDidit, true, showBackDrop);
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////
bool testPlayerHasLost()
{
	return (bPlayerHasLost);
}

void setPlayerHasLost(bool val)
{
	bPlayerHasLost = val;
}


////////////////////////////////////////////////////////////////////////////////
bool testPlayerHasWon()
{
	return (bPlayerHasWon);
}

void setPlayerHasWon(bool val)
{
	bPlayerHasWon = val;
}

/*access functions for scriptWinLoseVideo - used to indicate when the script is playing the win/lose video*/
void setScriptWinLoseVideo(UBYTE val)
{
	scriptWinLoseVideo = val;
}

UBYTE getScriptWinLoseVideo()
{
	return scriptWinLoseVideo;
}
