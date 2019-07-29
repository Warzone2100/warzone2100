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
 * titleui/gamefind.cpp
 *
 * This is the lobby menu.
 * Code adapted from multiint.cpp
 */

#include "titleui.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"
#include "../intdisplay.h"
#include "../hci.h"
#include "../mission.h"
#include "../console.h"
#include "../multiint.h"
#include "../multilimit.h"
#include "../multistat.h"
#include "../warzoneconfig.h"
#include "../frend.h"
#include "../loadsave.h"			// for blueboxes.

struct DisplayRemoteGameHeaderCache
{
	WzText wzHeaderText_GameName;
	WzText wzHeaderText_MapName;
	WzText wzHeaderText_Players;
	WzText wzHeaderText_Status;
};
struct DisplayRemoteGameCache
{
	WzText wzText_CurrentVsMaxNumPlayers;
	WidthLimitedWzText wzText_GameName;
	WidthLimitedWzText wzText_MapName;
	WidthLimitedWzText wzText_ModNames;
	WidthLimitedWzText wzText_VersionString;
};

static DisplayRemoteGameHeaderCache remoteGameListHeaderCache;

// find games
static void addGames();
static void addConsoleBox();
static void displayRemoteGame(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

WzGameFindTitleUI::WzGameFindTitleUI() {
}

// This is what starts the lobby screen
void WzGameFindTitleUI::start()
{
	addBackdrop();										//background image

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	// draws the background of the games listed
	IntFormAnimated *botForm = new IntFormAnimated(parent);
	botForm->id = FRONTEND_BOTFORM;
	botForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(MULTIOP_OPTIONSX, MULTIOP_OPTIONSY, MULTIOP_CHATBOXW, 415);  // FIXME: Add box at bottom for server messages
	}));

	addSideText(FRONTEND_SIDETEXT,  MULTIOP_OPTIONSX - 3, MULTIOP_OPTIONSY, _("GAMES"));

	// cancel
	addMultiBut(psWScreen, FRONTEND_BOTFORM, CON_CANCEL, 10, 5, MULTIOP_OKW, MULTIOP_OKH, _("Return To Previous Screen"),
	            IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	//refresh
	addMultiBut(psWScreen, FRONTEND_BOTFORM, MULTIOP_REFRESH, MULTIOP_CHATBOXW - MULTIOP_OKW - 5, 5, MULTIOP_OKW, MULTIOP_OKH,
	            _("Refresh Games List"), IMAGE_RELOAD_HI, IMAGE_RELOAD, IMAGE_RELOAD);
	//filter toggle
	addMultiBut(psWScreen, FRONTEND_BOTFORM, MULTIOP_FILTER_TOGGLE, MULTIOP_CHATBOXW - MULTIOP_OKW - 45, 5, MULTIOP_OKW, MULTIOP_OKH,
	            _("Filter Games List"), IMAGE_FILTER, IMAGE_FILTER_ON, IMAGE_FILTER_ON);

	if (safeSearch || multiintDisableLobbyRefresh)
	{
		widgHide(psWScreen, MULTIOP_REFRESH);
		widgHide(psWScreen, MULTIOP_FILTER_TOGGLE);
	}

	addConsoleBox();
	if (!NETfindGame())
	{
		pie_LoadBackDrop(SCREEN_RANDOMBDROP);
	}

	addGames();	// now add games.
	displayConsoleMessages();
}

TITLECODE WzGameFindTitleUI::run()
{
	screen_disableMapPreview();

	static UDWORD lastupdate = 0;

	if (lastupdate > gameTime)
	{
		lastupdate = 0;
	}
	if (gameTime - lastupdate > 6000)
	{
		lastupdate = gameTime;
		addConsoleBox();
		if (safeSearch)
		{
			if (!NETfindGame())						// find games synchronously
			{
				pie_LoadBackDrop(SCREEN_RANDOMBDROP);
			}
		}
		addGames();									//redraw list
	}

	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	if (id == CON_CANCEL)								// ok
	{
		changeTitleUI(std::make_shared<WzProtocolTitleUI>());
	}

	if (id == MULTIOP_REFRESH || id == MULTIOP_FILTER_TOGGLE)
	{
		if (id == MULTIOP_FILTER_TOGGLE)
		{
			toggleFilter = !toggleFilter;
			toggleFilter ? widgSetButtonState(psWScreen, MULTIOP_FILTER_TOGGLE, WBUT_CLICKLOCK) : widgSetButtonState(psWScreen, MULTIOP_FILTER_TOGGLE, 0);
		}
		else
		{
			widgSetButtonState(psWScreen, MULTIOP_FILTER_TOGGLE, 0);
		}
		ingame.localOptionsReceived = true;
		addConsoleBox();
		if (!NETfindGame())							// find games synchronously
		{
			pie_LoadBackDrop(SCREEN_RANDOMBDROP);
		}
		addConsoleMessage(_("Refreshing..."), DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
		addGames();									//redraw list.
	}

	// below is when they hit a game box to connect to--ideally this would be where
	// we would want a modal password entry box.
	if (id >= GAMES_GAMESTART && id <= GAMES_GAMEEND)
	{
		UDWORD gameNumber = id - GAMES_GAMESTART;

		// joinGame is quite capable of asking the user for a password, & is decoupled from lobby, so let it take over
		ingame.localOptionsReceived = false;					// note, we are awaiting options
		sstrcpy(game.name, NetPlay.games[gameNumber].name);		// store name
		joinGame(NetPlay.games[gameNumber].desc.host, 0);
		return TITLECODE_CONTINUE;
	}

	widgDisplayScreen(psWScreen);								// show the widgets currently running
	if (safeSearch)
	{
		iV_DrawText(_("Searching"), D_W + 260, D_H + 460, font_large);
	}

	if (CancelPressed())
	{
		changeTitleUI(std::make_shared<WzProtocolTitleUI>());
	}

	// console box handling
	if (widgGetFromID(psWScreen, MULTIOP_CONSOLEBOX))
	{
		while (getNumberConsoleMessages() > getConsoleLineInfo())
		{
			removeTopConsoleMessage();
		}
		updateConsoleMessages();
	}
	displayConsoleMessages();
	return TITLECODE_CONTINUE;
}

// --- Various statics ---

static void addConsoleBox()
{
	if (widgGetFromID(psWScreen, FRONTEND_TOPFORM))
	{
		widgDelete(psWScreen, FRONTEND_TOPFORM);
	}

	if (widgGetFromID(psWScreen, MULTIOP_CONSOLEBOX))
	{
		return;
	}

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	IntFormAnimated *consoleBox = new IntFormAnimated(parent);
	consoleBox->id = MULTIOP_CONSOLEBOX;
	consoleBox->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(MULTIOP_CONSOLEBOXX, MULTIOP_CONSOLEBOXY, MULTIOP_CONSOLEBOXW, MULTIOP_CONSOLEBOXH);
	}));

	flushConsoleMessages();											// add the chatbox.
	initConsoleMessages();
	enableConsoleDisplay(true);
	setConsoleBackdropStatus(false);
	setConsoleCalcLayout([]() {
		setConsoleSizePos(MULTIOP_CONSOLEBOXX + 4 + D_W, MULTIOP_CONSOLEBOXY + 14 + D_H, MULTIOP_CONSOLEBOXW - 4);
	});
	setConsolePermanence(true, true);
	setConsoleLineInfo(3);											// use x lines on chat window

	addConsoleMessage(_("Connecting to the lobby server..."), DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
	displayConsoleMessages();
	return;
}

// ////////////////////////////////////////////////////////////////////////////
// Game Chooser Screen.

static void addGames()
{
	int i, gcount = 0, added = 0;
	static const char *wrongVersionTip = _("Your version of Warzone is incompatible with this game.");

	//count games to see if need two columns.
	for (i = 0; i < MaxGames; i++)
	{
		if (NetPlay.games[i].desc.dwSize != 0)
		{
			gcount++;
		}
	}

	W_BUTINIT sButInit;
	sButInit.formID = FRONTEND_BOTFORM;
	sButInit.width = GAMES_GAMEWIDTH;
	sButInit.height = GAMES_GAMEHEIGHT;
	sButInit.pDisplay = displayRemoteGame;
	sButInit.initPUserDataFunc = []() -> void * { return new DisplayRemoteGameCache(); };
	sButInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayRemoteGameCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};

	// we want the old games deleted, and only list games when we should
	if (getLobbyError() || !gcount)
	{
		for (i = 0; i < MaxGames; i++)
		{
			widgDelete(psWScreen, GAMES_GAMESTART + i);	// remove old widget
		}
		gcount = 0;
	}
	// in case they refresh, and a game becomes available.
	widgDelete(psWScreen, FRONTEND_NOGAMESAVAILABLE);

	// only have to do this if we have any games available.
	if (!getLobbyError() && gcount)
	{
		for (i = 0; i < MaxGames; i++)							// draw games
		{
			widgDelete(psWScreen, GAMES_GAMESTART + i);	// remove old icon.

			if (NetPlay.games[i].desc.dwSize != 0)
			{
				// either display all games, or games that are the client's specific version
				if (toggleFilter)
				{
					if ((NetPlay.games[i].game_version_major != (unsigned)NETGetMajorVersion()) || (NetPlay.games[i].game_version_minor != (unsigned)NETGetMinorVersion()))
					{
						continue;
					}
				}
				added++;
				sButInit.id = GAMES_GAMESTART + i;
				sButInit.x = 20;
				sButInit.y = (UWORD)(45 + ((5 + GAMES_GAMEHEIGHT) * i));

				// display the correct tooltip message.
				if (!NETisCorrectVersion(NetPlay.games[i].game_version_major, NetPlay.games[i].game_version_minor))
				{
					sButInit.pTip = wrongVersionTip;
				}
				else
				{
					WzString flags;
					if (NetPlay.games[i].privateGame)
					{
						flags += " "; flags += _("[Password required]");
					}
					if (NetPlay.games[i].limits & NO_TANKS)
					{
						flags += " "; flags += _("[No Tanks]");
					}
					if (NetPlay.games[i].limits & NO_BORGS)
					{
						flags += " "; flags += _("[No Cyborgs]");
					}
					if (NetPlay.games[i].limits & NO_VTOLS)
					{
						flags += " "; flags += _("[No VTOLs]");
					}
					char tooltipbuffer[256];
					if (flags.isEmpty())
					{
						ssprintf(tooltipbuffer, _("Hosted by %s"), NetPlay.games[i].hostname);
					}
					else
					{
						ssprintf(tooltipbuffer, _("Hosted by %s —%s"), NetPlay.games[i].hostname, flags.toUtf8().c_str());
					}
					// this is an std::string
					sButInit.pTip = tooltipbuffer;
				}
				sButInit.UserData = i;

				widgAddButton(psWScreen, &sButInit);
			}
		}
		if (!added)
		{
			sButInit = W_BUTINIT();
			sButInit.formID = FRONTEND_BOTFORM;
			sButInit.id = FRONTEND_NOGAMESAVAILABLE;
			sButInit.x = 70;
			sButInit.y = 378;
			sButInit.style = WBUT_TXTCENTRE;
			sButInit.width = FRONTEND_BUTWIDTH;
			sButInit.UserData = 0; // store disable state
			sButInit.height = FRONTEND_BUTHEIGHT;
			sButInit.pDisplay = displayTextOption;
			sButInit.pUserData = new DisplayTextOptionCache();
			sButInit.onDelete = [](WIDGET *psWidget) {
				assert(psWidget->pUserData != nullptr);
				delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
				psWidget->pUserData = nullptr;
			};
			sButInit.FontID = font_large;
			sButInit.pText = _("Can't find any games for your version.");

			widgAddButton(psWScreen, &sButInit);
		}
	}
	else
	{
		// display lobby message based on results.
		// This is a 'button', not text so it can be hilighted/centered.
		const char *txt;
		W_BUTINIT sButInit;

		switch (getLobbyError())
		{
		case ERROR_NOERROR:
			if (NetPlay.HaveUpgrade)
			{
				txt = _("There appears to be a game update available!");
			}
			else
			{
				txt = _("No games are available for your version");
			}
			break;
		case ERROR_FULL:
			txt = _("Game is full");
			break;
		case ERROR_KICKED:
		case ERROR_INVALID:
			txt = _("You were kicked!");
			break;
		case ERROR_WRONGVERSION:
			txt = _("Wrong Game Version!");
			break;
		case ERROR_WRONGDATA:
			txt = _("You have an incompatible mod.");
			break;
		// AFAIK, the only way this can really happy is if the Host's file is named wrong, or a client side error.
		case ERROR_UNKNOWNFILEISSUE:
			txt = _("Host couldn't send file?");
			debug(LOG_POPUP, "Warzone couldn't complete a file request.\n\nPossibly, Host's file is incorrect. Check your logs for more details.");
			break;
		case ERROR_WRONGPASSWORD:
			txt = _("Incorrect Password!");
			break;
		case ERROR_HOSTDROPPED:
			txt = _("Host has dropped connection!");
			break;
		case ERROR_CONNECTION:
		default:
			txt = _("Connection Error");
			break;
		}

		// delete old widget if necessary
		widgDelete(psWScreen, FRONTEND_NOGAMESAVAILABLE);

		sButInit = W_BUTINIT();
		sButInit.formID = FRONTEND_BOTFORM;
		sButInit.id = FRONTEND_NOGAMESAVAILABLE;
		sButInit.x = 70;
		sButInit.y = 50;
		sButInit.style = WBUT_TXTCENTRE;
		sButInit.width = FRONTEND_BUTWIDTH;
		sButInit.UserData = 0; // store disable state
		sButInit.height = FRONTEND_BUTHEIGHT;
		sButInit.pDisplay = displayTextOption;
		sButInit.pUserData = new DisplayTextOptionCache();
		sButInit.onDelete = [](WIDGET *psWidget) {
			assert(psWidget->pUserData != nullptr);
			delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
		};
		sButInit.FontID = font_medium;
		sButInit.pText = txt;

		widgAddButton(psWScreen, &sButInit);
	}
	if (strlen(NetPlay.MOTD))
	{
		permitNewConsoleMessages(true);
		addConsoleMessage(NetPlay.MOTD, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
	setConsolePermanence(false, false);
	updateConsoleMessages();
	displayConsoleMessages();
}

void displayRemoteGame(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	UDWORD	gameID = psWidget->UserData;
	char tmp[80], name[StringSize];

	// Any widget using displayRemoteGame must have its pUserData initialized to a (DisplayRemoteGameCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayRemoteGameCache& cache = *static_cast<DisplayRemoteGameCache*>(psWidget->pUserData);

	if ((getLobbyError() != ERROR_NOERROR) && (bMultiPlayer && !NetPlay.bComms))
	{
		addConsoleMessage(_("Can't connect to lobby server!"), DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
		return;
	}

	// Draw blue boxes for games (buttons) & headers
	drawBlueBox(x, y, psWidget->width(), psWidget->height());
	drawBlueBox(x, y, GAMES_STATUS_START - 4 , psWidget->height());
	drawBlueBox(x, y, GAMES_PLAYERS_START - 4 , psWidget->height());
	drawBlueBox(x, y, GAMES_MAPNAME_START - 4, psWidget->height());

	int lamp = IMAGE_LAMP_RED;
	int statusStart = IMAGE_NOJOIN;
	bool disableButton = true;
	PIELIGHT textColor = WZCOL_TEXT_DARK;

	// As long as they got room, and mods are the same then we process the button(s)
	if (NETisCorrectVersion(NetPlay.games[gameID].game_version_major, NetPlay.games[gameID].game_version_minor))
	{
		if (NetPlay.games[gameID].desc.dwCurrentPlayers >= NetPlay.games[gameID].desc.dwMaxPlayers)
		{
			// If game is full.
			statusStart = IMAGE_NOJOIN_FULL;
		}
		else
		{
			// Game is ok to join!
			textColor = WZCOL_FORM_TEXT;
			statusStart = IMAGE_SKIRMISH_OVER;
			lamp = IMAGE_LAMP_GREEN;
			disableButton = false;

			if (NetPlay.games[gameID].privateGame)  // check to see if it is a private game
			{
				statusStart = IMAGE_LOCKED_NOBG;
				lamp = IMAGE_LAMP_AMBER;
			}
			else if (NetPlay.games[gameID].modlist[0] != '\0')
			{
				statusStart = IMAGE_MOD_OVER;
			}
		}

		ssprintf(tmp, "%d/%d", NetPlay.games[gameID].desc.dwCurrentPlayers, NetPlay.games[gameID].desc.dwMaxPlayers);
		cache.wzText_CurrentVsMaxNumPlayers.setText(tmp, font_regular);
		cache.wzText_CurrentVsMaxNumPlayers.render(x + GAMES_PLAYERS_START + 4 , y + 18, textColor);

		// see what host limits are... then draw them.
		if (NetPlay.games[gameID].limits)
		{
			if (NetPlay.games[gameID].limits & NO_TANKS)
			{
				iV_DrawImage(FrontImages, IMAGE_NO_TANK, x + GAMES_STATUS_START + 37, y + 2);
			}
			if (NetPlay.games[gameID].limits & NO_BORGS)
			{
				iV_DrawImage(FrontImages, IMAGE_NO_CYBORG, x + GAMES_STATUS_START + (37 * 2), y + 2);
			}
			if (NetPlay.games[gameID].limits & NO_VTOLS)
			{
				iV_DrawImage(FrontImages, IMAGE_NO_VTOL, x + GAMES_STATUS_START + (37 * 3) , y + 2);
			}
		}
	}
	// Draw type overlay.
	iV_DrawImage(FrontImages, statusStart, x + GAMES_STATUS_START, y + 3);
	iV_DrawImage(FrontImages, lamp, x - 14, y + 8);
	if (disableButton)
	{
		widgSetButtonState(psWScreen, psWidget->id, WBUT_DISABLE);
	}

	//draw game name, chop when we get a too long name
	sstrcpy(name, NetPlay.games[gameID].name);
	cache.wzText_GameName.setTruncatableText(name, font_regular, (GAMES_MAPNAME_START - GAMES_GAMENAME_START - 4));
	cache.wzText_GameName.render(x + GAMES_GAMENAME_START, y + 12, textColor);

	if (NetPlay.games[gameID].pureMap)
	{
		textColor = WZCOL_RED;
	}
	else
	{
		textColor = WZCOL_FORM_TEXT;
	}
	// draw map name, chop when we get a too long name
	sstrcpy(name, NetPlay.games[gameID].mapname);
	cache.wzText_MapName.setTruncatableText(name, font_regular, (GAMES_PLAYERS_START - GAMES_MAPNAME_START - 4));
	cache.wzText_MapName.render(x + GAMES_MAPNAME_START, y + 12, textColor);

	textColor = WZCOL_FORM_TEXT;
	// draw mod name (if any)
	if (strlen(NetPlay.games[gameID].modlist))
	{
		// FIXME: we really don't have enough space to list all mods
		char tmp[300];
		ssprintf(tmp, _("Mods: %s"), NetPlay.games[gameID].modlist);
		tmp[StringSize] = '\0';
		sstrcpy(name, tmp);
	}
	else
	{
		sstrcpy(name, _("Mods: None!"));
	}
	cache.wzText_ModNames.setTruncatableText(name, font_small, (GAMES_PLAYERS_START - GAMES_MAPNAME_START - 8));
	cache.wzText_ModNames.render(x + GAMES_MODNAME_START, y + 24, textColor);

	// draw version string
	ssprintf(name, _("Version: %s"), NetPlay.games[gameID].versionstring);
	cache.wzText_VersionString.setTruncatableText(name, font_small, (GAMES_MAPNAME_START - 6 - GAMES_GAMENAME_START - 4));
	cache.wzText_VersionString.render(x + GAMES_GAMENAME_START + 6, y + 24, textColor);

	// crappy hack to only draw this once for the header.  TODO fix GUI
	if (gameID == 0)
	{
		// make the 'header' for the table...
		drawBlueBox(x , y - 12 , GAMES_GAMEWIDTH, 12);

		remoteGameListHeaderCache.wzHeaderText_GameName.setText(_("Game Name"), font_small);
		remoteGameListHeaderCache.wzHeaderText_GameName.render(x - 2 + GAMES_GAMENAME_START + 48, y - 3, WZCOL_YELLOW);

		remoteGameListHeaderCache.wzHeaderText_MapName.setText(_("Map Name"), font_small);
		remoteGameListHeaderCache.wzHeaderText_MapName.render(x - 2 + GAMES_MAPNAME_START + 48, y - 3, WZCOL_YELLOW);

		remoteGameListHeaderCache.wzHeaderText_Players.setText(_("Players"), font_small);
		remoteGameListHeaderCache.wzHeaderText_Players.render(x - 2 + GAMES_PLAYERS_START, y - 3, WZCOL_YELLOW);

		remoteGameListHeaderCache.wzHeaderText_Status.setText(_("Status"), font_small);
		remoteGameListHeaderCache.wzHeaderText_Status.render(x - 2 + GAMES_STATUS_START + 48, y - 3, WZCOL_YELLOW);
	}
}

