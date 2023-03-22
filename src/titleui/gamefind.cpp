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
 * titleui/gamefind.cpp
 *
 * This is the lobby menu.
 * Code adapted from multiint.cpp
 */

#include "titleui.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"
#include "lib/widget/scrollablelist.h"
#include "lib/widget/label.h"
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
#include "../activity.h"

struct DisplayRemoteGameCache
{
	WzText wzText_CurrentVsMaxNumPlayers;
	WidthLimitedWzText wzText_GameName;
	WidthLimitedWzText wzText_MapName;
	WidthLimitedWzText wzText_ModNames;
	WidthLimitedWzText wzText_VersionString;
};

// find games
static std::vector<GAMESTRUCT> gamesList;

WzGameFindTitleUI::WzGameFindTitleUI() {
}
WzGameFindTitleUI::~WzGameFindTitleUI() {
	gamesList.clear();
}

// This is what starts the lobby screen
void WzGameFindTitleUI::start()
{
	addBackdrop();										//background image

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	// draws the background of the games listed
	auto botForm = std::make_shared<IntFormAnimated>();
	parent->attach(botForm);
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
	if (!NETfindGames(gamesList, 0, GAMES_MAX, toggleFilter))
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
	static UDWORD lastFetchRealTime = 0;

	if (lastupdate > gameTime)
	{
		lastupdate = 0;
	}
	bool handleUserRefreshRequest = queuedRefreshOfGamesList && ((realTime - lastFetchRealTime) >= (GAME_TICKS_PER_SEC));
	if (handleUserRefreshRequest || (gameTime - lastupdate > 6000))
	{
		lastupdate = gameTime;
		addConsoleBox();
		if (safeSearch || handleUserRefreshRequest)
		{
			setLobbyError(ERROR_NOERROR); // clear lobby error first
			if (!NETfindGames(gamesList, 0, GAMES_MAX, toggleFilter))	// find games synchronously
			{
				pie_LoadBackDrop(SCREEN_RANDOMBDROP);
			}
			lastFetchRealTime = realTime;
			queuedRefreshOfGamesList = false;
			addGames();	//redraw list
		}
	}

	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	if (id == CON_CANCEL)								// ok
	{
		clearActiveConsole();
		setLobbyError(ERROR_NOERROR); // clear any lobby error
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
		if (!queuedRefreshOfGamesList)
		{
			clearActiveConsole();
			addConsoleMessage(_("Refreshing..."), DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
			queuedRefreshOfGamesList = true;
		}
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

void WzGameFindTitleUI::addConsoleBox()
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

	auto consoleBox = std::make_shared<IntFormAnimated>();
	parent->attach(consoleBox);
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

class GameListHeader : public WIDGET
{
protected:
	GameListHeader()
	{ }

public:
	static std::shared_ptr<GameListHeader> make() {
		class make_shared_enabler : public GameListHeader
		{
		public:
			make_shared_enabler()
				: GameListHeader()
			{
			}
		};
		return std::make_shared<make_shared_enabler>();
	}

public:
	virtual void display(int xOffset, int yOffset) override
	{
		const int xPos = xOffset + x();
		const int yPos = yOffset + y();
		const int w = width();
		const int h = height();

		// draw the 'header' for the table...
		drawBlueBox(xPos, yPos, w, h);

		remoteGameListHeaderCache.wzHeaderText_GameName.setText(_("Game Name"), font_small);
		remoteGameListHeaderCache.wzHeaderText_GameName.render(xPos - 2 + GAMES_GAMENAME_START + 48, yPos + 9, WZCOL_YELLOW);

		remoteGameListHeaderCache.wzHeaderText_MapName.setText(_("Map Name"), font_small);
		remoteGameListHeaderCache.wzHeaderText_MapName.render(xPos - 2 + GAMES_MAPNAME_START + 48, yPos + 9, WZCOL_YELLOW);

		remoteGameListHeaderCache.wzHeaderText_Players.setText(_("Players"), font_small);
		remoteGameListHeaderCache.wzHeaderText_Players.render(xPos - 2 + GAMES_PLAYERS_START, yPos + 9, WZCOL_YELLOW);

		remoteGameListHeaderCache.wzHeaderText_Status.setText(_("Status"), font_small);
		remoteGameListHeaderCache.wzHeaderText_Status.render(xPos - 2 + GAMES_STATUS_START + 48, yPos + 9, WZCOL_YELLOW);
	}

private:
	struct DisplayRemoteGameHeaderCache
	{
		WzText wzHeaderText_GameName;
		WzText wzHeaderText_MapName;
		WzText wzHeaderText_Players;
		WzText wzHeaderText_Status;
	};
	DisplayRemoteGameHeaderCache remoteGameListHeaderCache;
};

static bool joinLobbyGame(uint32_t gameNumber, bool asSpectator = false)
{
	ASSERT_OR_RETURN(false, gameNumber < gamesList.size(), "Invalid gameNumber: %" PRIu32, gameNumber);

	std::vector<JoinConnectionDescription> connectionDesc = {JoinConnectionDescription(gamesList[gameNumber].desc.host, gamesList[gameNumber].hostPort)};
	ActivityManager::instance().willAttemptToJoinLobbyGame(NETgetMasterserverName(), NETgetMasterserverPort(), gamesList[gameNumber].gameId, connectionDesc);

	clearActiveConsole();

	// joinGame is quite capable of asking the user for a password, & is decoupled from lobby, so let it take over
	ingame.localOptionsReceived = false;					// note, we are awaiting options
	sstrcpy(game.name, gamesList[gameNumber].name);			// store name
	return joinGame(connectionDesc, asSpectator) != JoinGameResult::FAILED;
}

class WzLobbyGameDetails: public W_BUTTON
{
protected:
	WzLobbyGameDetails(UDWORD gameIdx)
	: W_BUTTON()
	, gameIdx(gameIdx)
	{ }
public:
	static std::shared_ptr<WzLobbyGameDetails> make(UDWORD gameIdx)
	{
		class make_shared_enabler: public WzLobbyGameDetails {
		public:
			make_shared_enabler(UDWORD gameIdx)
			: WzLobbyGameDetails(gameIdx) { }
		};
		return std::make_shared<make_shared_enabler>(gameIdx);
	}

	virtual void display(int xOffset, int yOffset) override
	{
		int x0 = xOffset + x();
		int y0 = yOffset + y();
		char tmp[80], name[StringSize];

		if (getLobbyError() != ERROR_NOERROR && bMultiPlayer && !NetPlay.bComms)
		{
			addConsoleMessage(_("Can't connect to lobby server!"), DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
			return;
		}

		// Draw blue boxes for games (buttons) & headers
		drawBlueBox(x0, y0, width(), height());
		drawBlueBox(x0, y0, GAMES_STATUS_START - 4 , height());
		drawBlueBox(x0, y0, GAMES_PLAYERS_START - 4 , height());
		drawBlueBox(x0, y0, GAMES_MAPNAME_START - 4, height());

		int statusStart = IMAGE_NOJOIN;
		bool disableButton = true;
		PIELIGHT textColor = WZCOL_TEXT_DARK;

		// As long as they got room, and mods are the same then we process the button(s)
		if (NETisCorrectVersion(gamesList[gameIdx].game_version_major, gamesList[gameIdx].game_version_minor))
		{
			if (gamesList[gameIdx].desc.dwCurrentPlayers >= gamesList[gameIdx].desc.dwMaxPlayers)
			{
				// If game is full.
				statusStart = IMAGE_NOJOIN_FULL;
			}
			else
			{
				// Game is ok to join!
				textColor = WZCOL_FORM_TEXT;
				statusStart = IMAGE_SKIRMISH_OVER;
				disableButton = false;

				if (gamesList[gameIdx].privateGame)  // check to see if it is a private game
				{
					statusStart = IMAGE_LOCKED_NOBG;
				}
				else if (gamesList[gameIdx].modlist[0] != '\0')
				{
					statusStart = IMAGE_MOD_OVER;
				}
			}

			ssprintf(tmp, "%d/%d", gamesList[gameIdx].desc.dwCurrentPlayers, gamesList[gameIdx].desc.dwMaxPlayers);
			wzText_CurrentVsMaxNumPlayers.setText(tmp, font_regular);
			int playersWidth = GAMES_STATUS_START - 8 - GAMES_PLAYERS_START;
			int playersPadding = (playersWidth - wzText_CurrentVsMaxNumPlayers.width()) / 2;
			wzText_CurrentVsMaxNumPlayers.render(x0 + GAMES_PLAYERS_START + playersPadding, y0 + 18, textColor);

			// see what host limits are... then draw them.
			if (gamesList[gameIdx].limits)
			{
				if (gamesList[gameIdx].limits & NO_TANKS)
				{
					iV_DrawImage(FrontImages, IMAGE_NO_TANK, x0 + GAMES_STATUS_START + 37, y0 + 2);
				}
				if (gamesList[gameIdx].limits & NO_BORGS)
				{
					iV_DrawImage(FrontImages, IMAGE_NO_CYBORG, x0 + GAMES_STATUS_START + (37 * 2), y0 + 2);
				}
				if (gamesList[gameIdx].limits & NO_VTOLS)
				{
					iV_DrawImage(FrontImages, IMAGE_NO_VTOL, x0 + GAMES_STATUS_START + (37 * 3) , y0 + 2);
				}
			}
		}
		// Draw type overlay.
		iV_DrawImage(FrontImages, statusStart, x0 + GAMES_STATUS_START, y0 + 3);
		if (disableButton)
		{
			setState(WBUT_DISABLE);
		}

		//draw game name, chop when we get a too long name
		sstrcpy(name, gamesList[gameIdx].name);
		wzText_GameName.setTruncatableText(name, font_regular, (GAMES_MAPNAME_START - GAMES_GAMENAME_START - 4));
		wzText_GameName.render(x0 + GAMES_GAMENAME_START, y0 + 12, textColor);

		if (gamesList[gameIdx].pureMap)
		{
			textColor = WZCOL_RED;
		}
		else
		{
			textColor = WZCOL_FORM_TEXT;
		}
		// draw map name, chop when we get a too long name
		sstrcpy(name, gamesList[gameIdx].mapname);
		wzText_MapName.setTruncatableText(name, font_regular, (GAMES_PLAYERS_START - GAMES_MAPNAME_START - 4));
		wzText_MapName.render(x0 + GAMES_MAPNAME_START, y0 + 12, textColor);

		textColor = WZCOL_FORM_TEXT;
		// draw mod name (if any)
		if (strlen(gamesList[gameIdx].modlist))
		{
			// FIXME: we really don't have enough space to list all mods
			char tmpMods[300];
			ssprintf(tmpMods, _("Mods: %s"), gamesList[gameIdx].modlist);
			tmpMods[StringSize] = '\0';
			sstrcpy(name, tmpMods);
		}
		else
		{
			sstrcpy(name, _("Mods: None!"));
		}
		wzText_ModNames.setTruncatableText(name, font_small, (GAMES_PLAYERS_START - GAMES_MAPNAME_START - 8));
		wzText_ModNames.render(x0 + GAMES_MODNAME_START, y0 + 24, textColor);

		// draw version string
		ssprintf(name, _("Version: %s"), gamesList[gameIdx].versionstring);
		wzText_VersionString.setTruncatableText(name, font_small, (GAMES_MAPNAME_START - 6 - GAMES_GAMENAME_START - 4));
		wzText_VersionString.render(x0 + GAMES_GAMENAME_START + 6, y0 + 24, textColor);
	}
private:
	UDWORD gameIdx = 0;
	WzText wzText_CurrentVsMaxNumPlayers;
	WidthLimitedWzText wzText_GameName;
	WidthLimitedWzText wzText_MapName;
	WidthLimitedWzText wzText_ModNames;
	WidthLimitedWzText wzText_VersionString;
};

class WzLobbyGameSpectateButton: public W_BUTTON
{
protected:
	WzLobbyGameSpectateButton()
	: W_BUTTON()
	{ }
public:
	static std::shared_ptr<WzLobbyGameSpectateButton> make()
	{
		class make_shared_enabler: public WzLobbyGameSpectateButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		ASSERT_OR_RETURN(nullptr, widget != nullptr, "Failed to create WzLobbyGameSpectateButton");
		widget->setTip(_("Join as spectator"));
		return widget;
	}

	virtual void display(int xOffset, int yOffset) override
	{
		int x0 = xOffset + x();
		int y0 = yOffset + y();

		iV_DrawImage(FrontImages, IMAGE_SPECTATE_SM, x0, y0 + 5);
	}

	virtual int32_t idealWidth() override
	{
		return iV_GetImageWidth(FrontImages, IMAGE_SPECTATE_SM);
	}
};

class WzLobbyGameWidget: public WIDGET
{
public:
	static std::shared_ptr<WzLobbyGameWidget> make(UDWORD gameIdx)
	{
		static const char *wrongVersionTip = _("Your version of Warzone is incompatible with this game.");

		class make_shared_enabler: public WzLobbyGameWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->gameIdx = gameIdx;

		// Create WzLobbyGameDetails button
		int gameDetailsX0 = 20;
		auto gameDetailsRowContents = WzLobbyGameDetails::make(gameIdx);
		ASSERT_OR_RETURN(nullptr, gameDetailsRowContents != nullptr, "Failed to create WzLobbyGameDetails");
		widget->attach(gameDetailsRowContents);
		gameDetailsRowContents->setGeometry(gameDetailsX0, 0, GAMES_GAMEWIDTH, GAMES_GAMEHEIGHT);
		gameDetailsRowContents->addOnClickHandler([gameIdx](W_BUTTON& button){
			widgScheduleTask([gameIdx](){
				joinLobbyGame(gameIdx, false);
			});
		});

		// display the correct tooltip message.
		std::string toolTip;
		if (!NETisCorrectVersion(gamesList[gameIdx].game_version_major, gamesList[gameIdx].game_version_minor))
		{
			toolTip = wrongVersionTip;
		}
		else
		{
			std::string flags;
			if (gamesList[gameIdx].privateGame)
			{
				flags += " "; flags += _("[Password required]");
			}
			if (gamesList[gameIdx].limits & NO_TANKS)
			{
				flags += " "; flags += _("[No Tanks]");
			}
			if (gamesList[gameIdx].limits & NO_BORGS)
			{
				flags += " "; flags += _("[No Cyborgs]");
			}
			if (gamesList[gameIdx].limits & NO_VTOLS)
			{
				flags += " "; flags += _("[No VTOLs]");
			}
			if (flags.empty())
			{
				toolTip = astringf(_("Hosted by %s"), gamesList[gameIdx].hostname);
			}
			else
			{
				toolTip = astringf(_("Hosted by %s —%s"), gamesList[gameIdx].hostname, flags.c_str());
			}
		}
		gameDetailsRowContents->setTip(toolTip);

		int gameDetailsX1 = gameDetailsX0 + GAMES_GAMEWIDTH;

		// Create WzLobbyGameSpectateButton button (if needed)
		if (NETisCorrectVersion(gamesList[gameIdx].game_version_major, gamesList[gameIdx].game_version_minor))
		{
			auto spectatorInfo = SpectatorInfo::fromUint32(gamesList[gameIdx].desc.dwUserFlags[1]);
			if (spectatorInfo.availableSpectatorSlots() > 0)
			{
				// has spectator slots - add button
				widget->spectateButton = WzLobbyGameSpectateButton::make();
				ASSERT_OR_RETURN(nullptr, widget->spectateButton != nullptr, "Failed to create WzLobbyGameSpectateButton");
				widget->attach(widget->spectateButton);
				widget->spectateButton->setGeometry(gameDetailsX1 + 2, 0, widget->spectateButton->idealWidth(), GAMES_GAMEHEIGHT);
				widget->spectateButton->addOnClickHandler([gameIdx](W_BUTTON& button){
					widgScheduleTask([gameIdx](){
						joinLobbyGame(gameIdx, true);
					});
				});
			}
		}

		return widget;
	}

	virtual void display(int xOffset, int yOffset) override
	{
		int x0 = xOffset + x();
		int y0 = yOffset + y();

		// Only needs to draw the lamp, otherwise the rest is handled by embedded child widgets
		int lamp = IMAGE_LAMP_RED;
		if (NETisCorrectVersion(gamesList[gameIdx].game_version_major, gamesList[gameIdx].game_version_minor))
		{
			if (gamesList[gameIdx].desc.dwCurrentPlayers < gamesList[gameIdx].desc.dwMaxPlayers)
			{
				// Game is ok to join!
				lamp = IMAGE_LAMP_GREEN;

				if (gamesList[gameIdx].privateGame)  // check to see if it is a private game
				{
					lamp = IMAGE_LAMP_AMBER;
				}
			}
		}
		// Draw lamp
		iV_DrawImage(FrontImages, lamp, x0 + 5, y0 + 8);
	}

	virtual int32_t idealWidth() override
	{
		return 20 + GAMES_GAMEWIDTH + ((spectateButton) ? (2 + spectateButton->idealWidth()) : 0);
	}
	virtual int32_t idealHeight() override
	{
		return GAMES_GAMEHEIGHT;
	}
private:
	UDWORD gameIdx = 0;
	std::shared_ptr<WzLobbyGameSpectateButton> spectateButton;
};

void WzGameFindTitleUI::addGames()
{
	size_t added = 0;

	//count games to see if need two columns.
	size_t gcount = gamesList.size();

	// we want the old games deleted, and only list games when we should
	widgDelete(psWScreen, GAMES_GAMEHEADER);
	widgDelete(psWScreen, GAMES_GAMELIST);
	if (getLobbyError() || !gcount)
	{
		gcount = 0;
	}
	// in case they refresh, and a game becomes available.
	widgDelete(psWScreen, FRONTEND_NOGAMESAVAILABLE);

	// only have to do this if we have any games available.
	if (!getLobbyError() && gcount)
	{
		// add header
		auto headerWidget = GameListHeader::make();
		headerWidget->setGeometry(20, 50 - 12, GAMES_GAMEWIDTH, 12);
		headerWidget->id = GAMES_GAMEHEADER;

		WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BOTFORM);
		ASSERT_OR_RETURN(, parent != nullptr && parent->type == WIDG_FORM, "Could not find parent form from formID");
		if (parent)
		{
			parent->attach(headerWidget);
		}

		auto scrollableGamesList = ScrollableListWidget::make();
		scrollableGamesList->id = GAMES_GAMELIST;
		scrollableGamesList->setGeometry(0, 50, parent->width(), GAMES_GAMEHEIGHT * 12);
		scrollableGamesList->setScrollbarWidth(12);
		parent->attach(scrollableGamesList);
		for (size_t i = 0; i < gamesList.size(); i++)				// draw games
		{
			if (gamesList[i].desc.dwSize != 0)
			{
				auto psRow = WzLobbyGameWidget::make(i);
				psRow->setGeometry(0, 0, parent->width(), GAMES_GAMEHEIGHT);
				scrollableGamesList->addItem(psRow);
				added++;
			}
		}
		if (!added)
		{
			W_BUTINIT sButInit;
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

		W_BUTINIT sButInit;
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
	if (NetPlay.MOTD && strlen(NetPlay.MOTD))
	{
		permitNewConsoleMessages(true);
		addConsoleMessage(NetPlay.MOTD, DEFAULT_JUSTIFY, SYSTEM_MESSAGE, false, MAX_CONSOLE_MESSAGE_DURATION);
	}
	setConsolePermanence(false, false);
	updateConsoleMessages();
	displayConsoleMessages();
}
