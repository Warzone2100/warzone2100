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
 * titleui/protocol.cpp
 *
 * This is the protocol menu.
 * It can open the lobby menu, or allows you to enter an IP address to join a game directly.
 * Code adapted from multiint.cpp
 */

#include "titleui.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"
#include "../multiplay.h"
#include "../intdisplay.h"
#include "../hci.h"
#include "../multiint.h"
#include "../warzoneconfig.h"
#include "../frend.h"

WzProtocolTitleUI::WzProtocolTitleUI()
{

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

void WzProtocolTitleUI::start()
{
	addBackdrop();										//background
	addTopForm(false);										// logo
	addBottomForm();

	// Obliterate any hanging settings screen
	closeIPDialog();

	// is it really a problem to leave this unchanged? in any case, when gamefinder is made a TitleUI it can properly forget again
	// safeSearch		= false;

	// don't pretend we are running a network game. Really do it!
	NetPlay.bComms = true; // use network = true

	addSideText(FRONTEND_SIDETEXT,  FRONTEND_SIDEX, FRONTEND_SIDEY, _("CONNECTION"));

	addMultiBut(psWScreen, FRONTEND_BOTFORM, CON_CANCEL, 10, 10, MULTIOP_OKW, MULTIOP_OKH,
	            _("Return To Previous Screen"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);	// goback buttpn levels

	addTextButton(CON_TYPESID_START + 0, FRONTEND_POS2X, FRONTEND_POS2Y, _("Lobby"), WBUT_TXTCENTRE);
	addTextButton(CON_TYPESID_START + 1, FRONTEND_POS3X, FRONTEND_POS3Y, _("IP"), WBUT_TXTCENTRE);

	if (hasWaitingIP) {
		hasWaitingIP = false;
		openIPDialog();
	}
}

TITLECODE WzProtocolTitleUI::run()
{
	screen_disableMapPreview();

	auto const &curScreen = psSettingsScreen ? psSettingsScreen : psWScreen;
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
		if (getLobbyError() != ERROR_INVALID)
		{
			setLobbyError(ERROR_NOERROR);
		}
		changeTitleUI(std::make_shared<WzGameFindTitleUI>());
		break;
	case CON_TYPESID_START+1: // IP button
		openIPDialog();
		break;
	case CON_OK:
		sstrcpy(serverName, widgGetString(curScreen, CON_IP));
		if (serverName[0] == '\0')
		{
			sstrcpy(serverName, "127.0.0.1");  // Default to localhost.
		}
		hasWaitingIP = true;
		closeIPDialog();
		joinGame(serverName);
		break;
	case CON_IP_CANCEL:
		closeIPDialog();
		break;
	}

	widgDisplayScreen(psWScreen);							// show the widgets currently running
	if (psSettingsScreen)
	{
		widgDisplayScreen(psSettingsScreen);						// show the widgets currently running
	}

	if (CancelPressed())
	{
		changeTitleMode(MULTI);
	}
	return TITLECODE_CONTINUE;
}

// ////////////////////////////////////////////////////////////////////////////
// Connection Options Screen.

void WzProtocolTitleUI::screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	if (!psSettingsScreen) return;
	psSettingsScreen->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

void WzProtocolTitleUI::openIPDialog()			//internet options
{
	psSettingsScreen = W_SCREEN::make();

	W_FORMINIT sFormInit;           //Connection Settings
	sFormInit.formID = 0;
	sFormInit.id = CON_SETTINGS;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.calcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(CON_SETTINGSX, CON_SETTINGSY, CON_SETTINGSWIDTH, CON_SETTINGSHEIGHT);
	});
	sFormInit.pDisplay = intDisplayFeBox;
	widgAddForm(psSettingsScreen, &sFormInit);

	addMultiBut(psSettingsScreen, CON_SETTINGS, CON_OK, CON_OKX, CON_OKY, MULTIOP_OKW, MULTIOP_OKH,
	            _("Accept Settings"), IMAGE_OK, IMAGE_OK, true);
	addMultiBut(psSettingsScreen, CON_SETTINGS, CON_IP_CANCEL, CON_OKX + MULTIOP_OKW + 10, CON_OKY, MULTIOP_OKW, MULTIOP_OKH,
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
	widgAddLabel(psSettingsScreen, &sLabInit);


	W_EDBINIT sEdInit;             // address
	sEdInit.formID = CON_SETTINGS;
	sEdInit.id = CON_IP;
	sEdInit.x = CON_IPX;
	sEdInit.y = CON_IPY;
	sEdInit.width = CON_NAMEBOXWIDTH;
	sEdInit.height = CON_NAMEBOXHEIGHT;
	sEdInit.pText = mpGetServerName();
	sEdInit.pBoxDisplay = intDisplayEditBox;
	if (!widgAddEditBox(psSettingsScreen, &sEdInit))
	{
		closeIPDialog();
		return;
	}
	widgSetString(psSettingsScreen, CON_IP, sEdInit.pText);
	// auto click in the text box
	W_CONTEXT sContext = W_CONTEXT::ZeroContext();
	sContext.mx			= CON_NAMEBOXWIDTH;
	sContext.my			= 0;
	widgGetFromID(psSettingsScreen, CON_IP)->clicked(&sContext);
}

void WzProtocolTitleUI::closeIPDialog()
{
	psSettingsScreen = nullptr;
}
