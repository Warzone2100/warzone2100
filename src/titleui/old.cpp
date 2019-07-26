/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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
#include "../hci.h"
#include "../keyedit.h"
#include "../keymap.h"
#include "../mission.h"
#include "../multiint.h"
#include "../multilimit.h"
#include "../multistat.h"
#include "../warzoneconfig.h"
#include "../frend.h"

// frontend.cpp
void startTitleMenu();
void startTutorialMenu();
void startSinglePlayerMenu();
void startCampaignSelector();
void startMultiPlayerMenu();
void startOptionsMenu();
void startGraphicsOptionsMenu();
void startAudioAndZoomOptionsMenu();
void startVideoOptionsMenu();
void startMouseOptionsMenu();
void startGameOptionsMenu();
void refreshCurrentVideoOptionsValues();

// Adopted (see below)
static void runConnectionScreen();
static bool startConnectionScreen();

WzOldTitleUI::WzOldTitleUI(tMode mode) : mode(mode)
{

}

void WzOldTitleUI::start()
{
	switch (mode)
	{
	case CAMPAIGNS:
		startCampaignSelector();
		break;
	case SINGLE:
		startSinglePlayerMenu();
		break;
	case GAME:
		startGameOptionsMenu();
		break;
	case GRAPHICS_OPTIONS:
		startGraphicsOptionsMenu();
		break;
	case AUDIO_AND_ZOOM_OPTIONS:
		startAudioAndZoomOptionsMenu();
		break;
	case VIDEO_OPTIONS:
		startVideoOptionsMenu();
		break;
	case MOUSE_OPTIONS:
		startMouseOptionsMenu();
		break;
	case TUTORIAL:
		startTutorialMenu();
		break;
	case OPTIONS:
		startOptionsMenu();
		break;
	case TITLE:
		startTitleMenu();
		break;
	case MULTI:
		startMultiPlayerMenu();		// goto multiplayer menu
		break;
	case PROTOCOL:
		startConnectionScreen();
		break;
	case GAMEFIND:
		startGameFind();
		break;
	case KEYMAP:
		startKeyMapEditor(true);
		break;
	case STARTGAME:
	case QUIT:
	case LOADSAVEGAME:
		bLimiterLoaded = false;
	case SHOWINTRO:
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
	case PROTOCOL:
		runConnectionScreen(); // multiplayer connection screen.
		break;
	case GAMEFIND:
		runGameFind();
		break;
	case MULTI:
		runMultiPlayerMenu();
		break;
	case KEYMAP:
		runKeyMapEditor();
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

void WzOldTitleUI::screenSizeDidChange()
{
	// If the Video Options screen is up, the current resolution text (and other values) should be refreshed
	if (mode == VIDEO_OPTIONS)
	{
		refreshCurrentVideoOptionsValues();
	}
}

// --- Adopted to clean up multiint.cpp ---

static bool				SettingsUp		= false;
static UBYTE				InitialProto	= 0;
static W_SCREEN				*psConScreen;

// ////////////////////////////////////////////////////////////////////////////
// Connection Options Screen.

void multiOptionsScreenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	if (psConScreen == nullptr) return;
	psConScreen->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

/*!
 * Set the server name
 * \param hostname The hostname or IP address of the server to connect to
 */
void mpSetServerName(const char *hostname)
{
	sstrcpy(serverName, hostname);
}

/**
 * @return The hostname or IP address of the server we will connect to.
 */
const char *mpGetServerName()
{
	return serverName;
}

static bool OptionsInet()			//internet options
{
	psConScreen = new W_SCREEN;

	W_FORMINIT sFormInit;           //Connection Settings
	sFormInit.formID = 0;
	sFormInit.id = CON_SETTINGS;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.calcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(CON_SETTINGSX, CON_SETTINGSY, CON_SETTINGSWIDTH, CON_SETTINGSHEIGHT);
	});
	sFormInit.pDisplay = intDisplayFeBox;
	widgAddForm(psConScreen, &sFormInit);

	addMultiBut(psConScreen, CON_SETTINGS, CON_OK, CON_OKX, CON_OKY, MULTIOP_OKW, MULTIOP_OKH,
	            _("Accept Settings"), IMAGE_OK, IMAGE_OK, true);
	addMultiBut(psConScreen, CON_SETTINGS, CON_IP_CANCEL, CON_OKX + MULTIOP_OKW + 10, CON_OKY, MULTIOP_OKW, MULTIOP_OKH,
	            _("Cancel"), IMAGE_NO, IMAGE_NO, true);

	//label.
	W_LABINIT sLabInit;
	sLabInit.formID = CON_SETTINGS;
	sLabInit.id		= CON_SETTINGS_LABEL;
	sLabInit.style	= WLAB_ALIGNCENTRE;
	sLabInit.x		= 0;
	sLabInit.y		= 10;
	sLabInit.width	= CON_SETTINGSWIDTH;
	sLabInit.height = 20;
	sLabInit.pText	= WzString::fromUtf8(_("IP Address or Machine Name"));
	widgAddLabel(psConScreen, &sLabInit);


	W_EDBINIT sEdInit;             // address
	sEdInit.formID = CON_SETTINGS;
	sEdInit.id = CON_IP;
	sEdInit.x = CON_IPX;
	sEdInit.y = CON_IPY;
	sEdInit.width = CON_NAMEBOXWIDTH;
	sEdInit.height = CON_NAMEBOXHEIGHT;
	sEdInit.pText = serverName;
	sEdInit.pBoxDisplay = intDisplayEditBox;
	if (!widgAddEditBox(psConScreen, &sEdInit))
	{
		return false;
	}
	// auto click in the text box
	W_CONTEXT sContext;
	sContext.xOffset	= 0;
	sContext.yOffset	= 0;
	sContext.mx			= CON_NAMEBOXWIDTH;
	sContext.my			= 0;
	widgGetFromID(psConScreen, CON_IP)->clicked(&sContext);

	SettingsUp = true;
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Draw the connections screen.
bool startConnectionScreen()
{
	addBackdrop();										//background
	addTopForm();										// logo
	addBottomForm();

	SettingsUp		= false;
	InitialProto	= 0;
	safeSearch		= false;

	// don't pretend we are running a network game. Really do it!
	NetPlay.bComms = true; // use network = true

	addSideText(FRONTEND_SIDETEXT,  FRONTEND_SIDEX, FRONTEND_SIDEY, _("CONNECTION"));

	addMultiBut(psWScreen, FRONTEND_BOTFORM, CON_CANCEL, 10, 10, MULTIOP_OKW, MULTIOP_OKH,
	            _("Return To Previous Screen"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);	// goback buttpn levels

	addTextButton(CON_TYPESID_START + 0, FRONTEND_POS2X, FRONTEND_POS2Y, _("Lobby"), WBUT_TXTCENTRE);
	addTextButton(CON_TYPESID_START + 1, FRONTEND_POS3X, FRONTEND_POS3Y, _("IP"), WBUT_TXTCENTRE);

	return true;
}

void runConnectionScreen()
{
	W_SCREEN *curScreen = SettingsUp ? psConScreen : psWScreen;
	WidgetTriggers const &triggers = widgRunScreen(curScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case CON_CANCEL: //cancel
		changeTitleMode(MULTI);
		bMultiPlayer = false;
		bMultiMessages = false;
		break;
	case CON_TYPESID_START+0: // Lobby button
		if (LobbyError != ERROR_INVALID)
		{
			setLobbyError(ERROR_NOERROR);
		}
		changeTitleMode(GAMEFIND);
		break;
	case CON_TYPESID_START+1: // IP button
		OptionsInet();
		break;
	case CON_OK:
		sstrcpy(serverName, widgGetString(psConScreen, CON_IP));
		if (serverName[0] == '\0')
		{
			sstrcpy(serverName, "127.0.0.1");  // Default to localhost.
		}

		if (SettingsUp == true)
		{
			delete psConScreen;
			psConScreen = nullptr;
			SettingsUp = false;
		}

		joinGame(serverName, 0);
		break;
	case CON_IP_CANCEL:
		if (SettingsUp == true)
		{
			delete psConScreen;
			psConScreen = nullptr;
			SettingsUp = false;
		}
		break;
	}

	widgDisplayScreen(psWScreen);							// show the widgets currently running
	if (SettingsUp == true)
	{
		widgDisplayScreen(psConScreen);						// show the widgets currently running
	}

	if (CancelPressed())
	{
		changeTitleMode(MULTI);
	}
}
