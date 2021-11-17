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
/*
 * titleui/old.cpp
 *
 * This is a catch-all for stuff that hasn't properly been encapsulated into individual classes yet.
 * Code adapted from frontend.cpp & wrappers.cpp & frontend.h by Alex Lee / Pumpkin Studios / Eidos PLC.
 */

#include "titleui.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"
#include "../intdisplay.h"
#include "../frontend.h"
#include "../hci.h"
#include "../keyedit.h"
#include "../mission.h"
#include "../multiint.h"
#include "../multilimit.h"
#include "../multistat.h"
#include "../musicmanager.h"
#include "../warzoneconfig.h"
#include "../frend.h"
#include "../activity.h"

WzOldTitleUI::WzOldTitleUI(tMode mode) : mode(mode)
{

}

void WzOldTitleUI::start()
{
	switch (mode)
	{
	case CAMPAIGNS:
		ActivityManager::instance().navigateToMenu("Campaign");
		startCampaignSelector();
		break;
	case SINGLE:
		ActivityManager::instance().navigateToMenu("Single Player");
		startSinglePlayerMenu();
		break;
	case GAME:
		ActivityManager::instance().navigateToMenu("Game Options");
		startGameOptionsMenu();
		break;
	case GRAPHICS_OPTIONS:
		ActivityManager::instance().navigateToMenu("Graphics Options");
		startGraphicsOptionsMenu();
		break;
	case AUDIO_AND_ZOOM_OPTIONS:
		ActivityManager::instance().navigateToMenu("Audio + Zoom Options");
		startAudioAndZoomOptionsMenu();
		break;
	case VIDEO_OPTIONS:
		ActivityManager::instance().navigateToMenu("Video Options");
		startVideoOptionsMenu();
		break;
	case MOUSE_OPTIONS:
		ActivityManager::instance().navigateToMenu("Mouse Options");
		startMouseOptionsMenu();
		break;
	case TUTORIAL:
		ActivityManager::instance().navigateToMenu("Tutorial");
		startTutorialMenu();
		break;
	case OPTIONS:
		ActivityManager::instance().navigateToMenu("Options");
		startOptionsMenu();
		break;
	case TITLE:
		ActivityManager::instance().navigateToMenu("Main");
		startTitleMenu();
		break;
	case MULTI:
		ActivityManager::instance().navigateToMenu("Multiplayer");
		startMultiPlayerMenu();		// goto multiplayer menu
		break;
	case KEYMAP:
		ActivityManager::instance().navigateToMenu("KeyMap Editor");
		startKeyMapEditor(gInputManager, gKeyFuncConfig, true);
		break;
	case MUSIC_MANAGER:
		ActivityManager::instance().navigateToMenu("Music Manager");
		startMusicManager();
		break;
	case MULTIPLAY_OPTIONS:
		ActivityManager::instance().navigateToMenu("Multiplay Options");
		startMultiplayOptionsMenu();
		break;
	case STARTGAME:
	case QUIT:
	case LOADSAVEGAME:
		bLimiterLoaded = false;
		break;
	case SHOWINTRO:
		ActivityManager::instance().navigateToMenu("Show Intro");
		break;
	default:
		debug(LOG_FATAL, "Unknown title mode requested");
		abort();
		break;
	}

	return;
}

TITLECODE WzOldTitleUI::run()
{
	if (mode != STARTGAME)
	{
		screen_disableMapPreview();
	}

	switch (mode) // run relevant title screen code.
	{
	// MULTIPLAYER screens
	case MULTI:
		runMultiPlayerMenu();
		break;
	case KEYMAP:
		runKeyMapEditor(gInputManager, gKeyFuncConfig);
		break;

	case MUSIC_MANAGER:
		runMusicManager();
		break;

	case TITLE:
		runTitleMenu();
		break;

	case CAMPAIGNS:
		runCampaignSelector();
		break;

	case SINGLE:
		runSinglePlayerMenu();
		break;

	case TUTORIAL:
		runTutorialMenu();
		break;

	case OPTIONS:
		runOptionsMenu();
		break;

	case GAME:
		runGameOptionsMenu();
		break;

	case GRAPHICS_OPTIONS:
		runGraphicsOptionsMenu();
		break;

	case AUDIO_AND_ZOOM_OPTIONS:
		runAudioAndZoomOptionsMenu();
		break;

	case VIDEO_OPTIONS:
		runVideoOptionsMenu();
		break;

	case MOUSE_OPTIONS:
		runMouseOptionsMenu();
		break;

	case MULTIPLAY_OPTIONS:
		runMultiplayOptionsMenu();
		break;

	case QUIT:
		return TITLECODE_QUITGAME;

	// The "don't flip" behavior is preserved in wrappers.cpp by checking for these title codes (which are clearly special)
	case STARTGAME:
		return TITLECODE_STARTGAME;
	case LOADSAVEGAME:
		return TITLECODE_SAVEGAMELOAD;

	case SHOWINTRO:
		pie_SetFogStatus(false);
		pie_ScreenFlip(CLEAR_BLACK);
		changeTitleMode(TITLE);
		return TITLECODE_SHOWINTRO;

	default:
		debug(LOG_FATAL, "unknown title screen mode");
		abort();
	}
	return TITLECODE_CONTINUE;
}

void WzOldTitleUI::screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	// If the Video Options screen is up, the current resolution text (and other values) should be refreshed
	if (mode == VIDEO_OPTIONS)
	{
		refreshCurrentVideoOptionsValues();
	}
}
