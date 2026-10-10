// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
#include "lib/framework/gamepad_input.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/connection_provider_registry.h"
#include "lib/netplay/netplay.h"	// multiplayer
#include "lib/sound/audio.h"
#include "lib/framework/wzapp.h"

#include "clparse.h"
#include "frontend.h"
#include "mission.h"
#include "multiint.h"
#include "multilimit.h"
#include "multistat.h"
#include "screens/spectatorgameoverscreen.h"
#include "warzoneconfig.h"
#include "wrappers.h"
#include "titleui/titleui.h"
#include "stdinreader.h"
#include "multijoin_helpers.h"

#include <vector>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

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
static bool bHostLaunchStartNotReady = false;
static bool loadingScreenSessionActive = false;

struct LoadingBarLayout
{
	int barLeftX = 0;
	int barLeftY = 0;
	int barRightX = 0;
	int barRightY = 0;
	int boxWidth = 0;
	int boxHeight = 0;
	int starsNum = 0;
	int starHeight = 0;
	int width = 0;
	int height = 0;
};

static LoadingBarLayout loadingBar;
static std::vector<STAR> loadingStars;

static STAR newStar(const LoadingBarLayout& layout)
{
	STAR s;
	s.xPos = rand() % layout.barRightX;
	s.speed = static_cast<int>((rand() % 30 + 6) * pie_GetVideoBufferWidth() / 640.0);
	s.colour = pal_SetBrightness(150 + rand() % 100);
	return s;
}

static LoadingBarLayout loadingBarMetricsFor(int width, int height)
{
	LoadingBarLayout layout;
	const int offset = static_cast<int>(height / 40.0);

	layout.boxHeight = offset;
	layout.boxWidth = width - 2 * offset;
	layout.barRightX = width - offset;
	layout.barRightY = height - offset;
	layout.barLeftX = layout.barRightX - layout.boxWidth;
	layout.barLeftY = layout.barRightY - layout.boxHeight;
	layout.starsNum = std::max(0, layout.boxWidth / std::max(layout.boxHeight, 1));
	layout.starHeight = static_cast<int>(2.0 * height / 640.0);
	layout.width = width;
	layout.height = height;
	return layout;
}

static void ensureLoadingBarLayout()
{
	const int width = pie_GetVideoBufferWidth();
	const int height = pie_GetVideoBufferHeight();
	if (width == loadingBar.width && height == loadingBar.height
		&& static_cast<int>(loadingStars.size()) == loadingBar.starsNum)
	{
		return;
	}

	loadingBar = loadingBarMetricsFor(width, height);
	if (loadingBar.starsNum <= 0 || loadingBar.barRightX <= 0)
	{
		loadingStars.clear();
		return;
	}

	loadingStars.resize(static_cast<size_t>(loadingBar.starsNum));
	for (STAR& star : loadingStars)
	{
		star = newStar(loadingBar);
	}
}

static void renderLoadingScreenPass()
{
	ensureLoadingBarLayout();

	const PIELIGHT loadingbar_background = WZCOL_LOADING_BAR_BACKGROUND;

	pie_UniTransBoxFill(loadingBar.barLeftX - 2, loadingBar.barLeftY - 2, loadingBar.barRightX + 2, loadingBar.barRightY + 2, loadingbar_background);

	for (size_t i = 1; i < loadingStars.size(); ++i)
	{
		loadingStars[i].xPos = loadingStars[i].xPos + loadingStars[i].speed;
		if (loadingBar.barLeftX + loadingStars[i].xPos >= loadingBar.barRightX)
		{
			loadingStars[i] = newStar(loadingBar);
			loadingStars[i].xPos = 1;
		}
		{
			const int topX = loadingBar.barLeftX + loadingStars[i].xPos;
			const int topY = loadingBar.barLeftY + static_cast<int>(i) * (loadingBar.boxHeight - loadingBar.starHeight) / static_cast<int>(loadingStars.size());
			const int botX = MIN(topX + loadingStars[i].speed, loadingBar.barRightX);
			const int botY = topY + loadingBar.starHeight;

			pie_UniTransBoxFill(topX, topY, botX, botY, loadingStars[i].colour);
		}
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

void resetHostLaunch()
{
	setHostLaunch(HostLaunch::Normal);
	setHostLaunchStartNotReady(false);
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

void setHostLaunchStartNotReady(bool value)
{
	bHostLaunchStartNotReady = value;
}

bool getHostLaunchStartNotReady()
{
	if (bHostLaunchStartNotReady && headlessGameMode() && !wz_command_interface_enabled())
	{
		debug(LOG_ERROR, "--autohost-not-ready specified while in headless mode without --enablecmdinterface specified. No way to start the host. Ignoring.");
		bHostLaunchStartNotReady = false;
	}
	return bHostLaunchStartNotReady;
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
	if (!headlessGameMode())
	{
		screen_RestartBackDrop();
	}
	wzShowMouse(!isGamepadActiveInput());

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
				bMultiMessages = true;
				NETinit(war_getHostConnectionProvider());
				NETinitPortMapping();
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
			// Ensure the joinGame has a place to return to
			changeTitleMode(TITLE);
			// Don't call `NETinit()` just yet.
			// It will be automatically called by `joinGame()` upon connection attempt
			// with the correct connection provider type corresponding to the connection string.
			joinGameFromIPOrHostnameConnectionStr(iptoconnect, cliConnectAsSpectator);
		}
		else if (!cli_lobby_game_to_connect_str().empty())
		{
			NetPlay.bComms = true; // use network = true
			// Ensure the joinGame has a place to return to
			changeTitleMode(TITLE);
			// Don't call `NETinit()` just yet.
			// It will be automatically called upon connection attempt
			// with the correct connection provider type corresponding to discovered connection info.
			joinLobbyGame(NETgetLobbyserverAddress(), cli_lobby_game_to_connect_str(), cliConnectAsSpectator);
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

bool isLoadingScreenActive()
{
	return loadingScreenSessionActive;
}

void wrappers_recordLoadingScreen(const gfx_api::RenderPassContext&)
{
	renderLoadingScreenPass();
}

#if defined(__EMSCRIPTEN__)
void wzemscripten_display_web_loading_indicator(int x)
{
	MAIN_THREAD_EM_ASM({
		if (typeof wz_js_display_loading_indicator === "function") {
			wz_js_display_loading_indicator($0);
		}
		else {
			console.log('Cannot find wz_js_display_loading_indicator function');
		}
	}, x);
}
#endif

// Bar geometry is derived on the first loading-pass record.
void initLoadingScreen(bool drawbdrop)
{
	wzShowMouse(false);
	pie_SetFogStatus(false);
	loadingScreenSessionActive = true;

#if defined(__EMSCRIPTEN__)
	wzemscripten_display_web_loading_indicator(1);
#endif

	if (drawbdrop && !headlessGameMode())
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
	loadingScreenSessionActive = false;

	loadingStars.clear();
	loadingStars.shrink_to_fit();
	loadingBar = {};
#if defined(__EMSCRIPTEN__)
	wzemscripten_display_web_loading_indicator(0);
#endif
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

			// If in blind mode, send data on who the players were
			if (game.blindMode != BLIND_MODE::NONE)
			{
				if (NetPlay.isHost)
				{
					// Send updated player info (which will include real names, now that game has ended) to all players
					NETSendAllPlayerInfoTo(NET_ALL_PLAYERS);

					// Send the verified player identity from initial join for each player (now that game has ended)
					for (uint32_t idx = 0; idx < MAX_CONNECTED_PLAYERS; ++idx)
					{
						sendMultiStatsHostVerifiedIdentities(idx);
					}
				}

				// Note: Replay player info updating occurs as part of NETreplaySaveStop
			}
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
	// Replay viewers have no profile stats to save
	if (bMultiPlayer && isFirstCallForThisGame && !NETisReplay() && selectedPlayer < MAX_CONNECTED_PLAYERS)
	{
		updateMultiStatsGames(); // update games played.

		PLAYERSTATS st = getMultiStats(selectedPlayer);
		saveMultiStats(sPlayer, sPlayer, &st);
	}

	//clear out any mission widgets - timers etc that may be on the screen
	clearMissionWidgets();

	if (bMultiPlayer && NetPlay.players[selectedPlayer].isSpectator)
	{
		// Special screen for spectators to inform them that the game is fully over
		showSpectatorGameOverScreen();
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
