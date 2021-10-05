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
 * FrontEnd.c
 *
 * front end title and options screens.
 * Alex Lee. Pumpkin Studios. Eidos PLC 98,
 */

#include "lib/framework/wzapp.h"

#include "lib/framework/input.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/physfs_ext.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/sound/mixer.h"
#include "lib/sound/tracklib.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/slider.h"
#include "lib/widget/dropdown.h"
#include "lib/widget/gridlayout.h"
#include "lib/widget/margin.h"
#include "lib/widget/alignment.h"

#include <limits>

#include "advvis.h"
#include "challenge.h"
#include "component.h"
#include "configuration.h"
#include "difficulty.h"
#include "display.h"
#include "droid.h"
#include "frend.h"
#include "frontend.h"
#include "group.h"
#include "hci.h"
#include "init.h"
#include "levels.h"
#include "intdisplay.h"
#include "keybind.h"
#include "keyedit.h"
#include "loadsave.h"
#include "main.h"
#include "mission.h"
#include "modding.h"
#include "multiint.h"
#include "multilimit.h"
#include "multiplay.h"
#include "radar.h"
#include "seqdisp.h"
#include "texture.h"
#include "version.h"
#include "warzoneconfig.h"
#include "wrappers.h"
#include "titleui/titleui.h"
#include "urlhelpers.h"
#include "game.h"
#include "map.h" //for builtInMap
#include "notifications.h"

#include "uiscreens/titlemenu.h"
#include "uiscreens/common.h"
#include "uiscreens/options/graphicsoptions.h"

// ////////////////////////////////////////////////////////////////////////////
// Global variables

char			aLevelName[MAX_LEVEL_NAME_SIZE + 1];	//256];			// vital! the wrf file to use.

bool			bLimiterLoaded = false;

#define TUTORIAL_LEVEL "TUTORIAL3"

// ////////////////////////////////////////////////////////////////////////////
// Tutorial Menu
void startTutorialMenu()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();


	addTextButton(FRONTEND_TUTORIAL, FRONTEND_POS3X, FRONTEND_POS3Y, _("Tutorial"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_FASTPLAY, FRONTEND_POS4X, FRONTEND_POS4Y, _("Fast Play"), WBUT_TXTCENTRE);
	addSideText(FRONTEND_SIDETEXT , FRONTEND_SIDEX, FRONTEND_SIDEY, _("TUTORIALS"));
	// TRANSLATORS: "Return", in this context, means "return to previous screen/menu"
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
}

bool runTutorialMenu()
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_TUTORIAL:
		SPinit(LEVEL_TYPE::CAMPAIGN);
		sstrcpy(aLevelName, TUTORIAL_LEVEL);
		changeTitleMode(STARTGAME);
		break;

	case FRONTEND_FASTPLAY:
		SPinit(LEVEL_TYPE::CAMPAIGN);
		sstrcpy(aLevelName, "FASTPLAY");
		changeTitleMode(STARTGAME);
		break;

	case FRONTEND_QUIT:
		changeTitleMode(TITLE);
		break;

	default:
		break;
	}

	// If close button pressed then return from this menu.
	if (CancelPressed())
	{
		changeTitleMode(TITLE);
	}

	widgDisplayScreen(psWScreen);						// show the widgets currently running

	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// Single Player Menu
void startSinglePlayerMenu()
{
	challengeActive = false;
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	addTextButton(FRONTEND_NEWGAME,  FRONTEND_POS2X, FRONTEND_POS2Y, _("New Campaign") , WBUT_TXTCENTRE);
	addTextButton(FRONTEND_SKIRMISH, FRONTEND_POS3X, FRONTEND_POS3Y, _("Start Skirmish Game"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_CHALLENGES, FRONTEND_POS4X, FRONTEND_POS4Y, _("Challenges"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_LOADGAME_MISSION, FRONTEND_POS5X, FRONTEND_POS5Y, _("Load Campaign Game"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_LOADGAME_SKIRMISH, FRONTEND_POS6X, FRONTEND_POS6Y, _("Load Skirmish Game"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_REPLAY, FRONTEND_POS7X,FRONTEND_POS7Y, _("View Skirmish Replay"), WBUT_TXTCENTRE);

	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("SINGLE PLAYER"));
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
	// show this only when the video sequences are not installed
	if (!PHYSFS_exists("sequences/devastation.ogg"))
	{
		addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS9X, FRONTEND_POS9Y, _("Campaign videos are missing! Get them from http://wz2100.net"), 0);
	}
}

void startCampaignSelector()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	std::vector<CAMPAIGN_FILE> list = readCampaignFiles();
	ASSERT(list.size() <= static_cast<size_t>(std::numeric_limits<UDWORD>::max()), "list.size() (%zu) exceeds UDWORD max", list.size());
	for (UDWORD i = 0; i < static_cast<UDWORD>(list.size()); i++)
	{
		addTextButton(FRONTEND_CAMPAIGN_1 + i, FRONTEND_POS1X, FRONTEND_POS2Y + FRONTEND_BUTHEIGHT * i, gettext(list[i].name.toUtf8().c_str()), WBUT_TXTCENTRE);
	}
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("CAMPAIGNS"));
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
	// show this only when the video sequences are not installed
	if (!PHYSFS_exists("sequences/devastation.ogg"))
	{
		addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS9X, FRONTEND_POS9Y, _("Campaign videos are missing! Get them from http://wz2100.net"), 0);
	}
}

static void frontEndNewGame(int which)
{
	std::vector<CAMPAIGN_FILE> list = readCampaignFiles();
	sstrcpy(aLevelName, list[which].level.toUtf8().c_str());
	// show this only when the video sequences are installed
	if (PHYSFS_exists("sequences/devastation.ogg"))
	{
		if (!list[which].video.isEmpty())
		{
			seq_ClearSeqList();
			seq_AddSeqToList(list[which].video.toUtf8().c_str(), nullptr, list[which].captions.toUtf8().c_str(), false);
			seq_StartNextFullScreenVideo();
		}
	}
	if (!list[which].package.isEmpty())
	{
		WzString path;
		path += PHYSFS_getWriteDir();
		path += PHYSFS_getDirSeparator();
		path += "campaigns";
		path += PHYSFS_getDirSeparator();
		path += list[which].package;
		if (!PHYSFS_mount(path.toUtf8().c_str(), NULL, PHYSFS_APPEND))
		{
			debug(LOG_ERROR, "Failed to load campaign mod \"%s\": %s",
			      path.toUtf8().c_str(), WZ_PHYSFS_getLastError());
		}
	}
	if (!list[which].loading.isEmpty())
	{
		debug(LOG_WZ, "Adding campaign mod level \"%s\"", list[which].loading.toUtf8().c_str());
		if (!loadLevFile(list[which].loading.toUtf8().c_str(), mod_campaign, false, nullptr))
		{
			debug(LOG_ERROR, "Failed to load %s", list[which].loading.toUtf8().c_str());
			return;
		}
	}
	debug(LOG_WZ, "Loading campaign mod -- %s", aLevelName);
	changeTitleMode(STARTGAME);
}

static void loadOK()
{
	if (strlen(sRequestResult))
	{
		sstrcpy(saveGameName, sRequestResult);
		changeTitleMode(LOADSAVEGAME);
	}
}

void SPinit(LEVEL_TYPE gameType)
{
	uint8_t playercolor;

	NetPlay.bComms = false;
	bMultiPlayer = false;
	bMultiMessages = false;
	game.type = gameType;
	NET_InitPlayers();
	NetPlay.players[0].allocated = true;
	NetPlay.players[0].autoGame = false;
	NetPlay.players[0].difficulty = AIDifficulty::HUMAN;
	game.maxPlayers = MAX_PLAYERS - 1;	// Currently, 0 - 10 for a total of MAX_PLAYERS
	// make sure we have a valid color choice for our SP game. Valid values are 0, 4-7
	playercolor = war_GetSPcolor();

	if (playercolor >= 1 && playercolor <= 3)
	{
		playercolor = 0;	// default is green
	}
	setPlayerColour(0, playercolor);
	game.hash.setZero();	// must reset this to zero
	builtInMap = true;
}

bool runCampaignSelector()
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.
	if (id == FRONTEND_QUIT)
	{
		changeTitleMode(SINGLE); // go back
	}
	else if (id >= FRONTEND_CAMPAIGN_1 && id <= FRONTEND_CAMPAIGN_6) // chose a campaign
	{
		SPinit(LEVEL_TYPE::CAMPAIGN);
		frontEndNewGame(id - FRONTEND_CAMPAIGN_1);
	}
	else if (id == FRONTEND_HYPERLINK)
	{
		runHyperlink();
	}

	widgDisplayScreen(psWScreen); // show the widgets currently running

	if (CancelPressed())
	{
		changeTitleMode(SINGLE);
	}

	return true;
}

bool runSinglePlayerMenu()
{
	if (bLoadSaveUp)
	{
		if (runLoadSave(false)) // check for file name.
		{
			loadOK();
		}
	}
	else if (challengesUp)
	{
		runChallenges();
	}
	else
	{
		WidgetTriggers const &triggers = widgRunScreen(psWScreen);
		unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

		switch (id)
		{
		case FRONTEND_NEWGAME:
			changeTitleMode(CAMPAIGNS);
			break;

		case FRONTEND_LOADGAME_MISSION:
			SPinit(LEVEL_TYPE::CAMPAIGN);
			addLoadSave(LOAD_FRONTEND_MISSION, _("Load Campaign Saved Game"));	// change mode when loadsave returns
			break;

		case FRONTEND_LOADGAME_SKIRMISH:
			SPinit(LEVEL_TYPE::SKIRMISH);
			bMultiPlayer = true;
			addLoadSave(LOAD_FRONTEND_SKIRMISH, _("Load Skirmish Saved Game"));	// change mode when loadsave returns
			break;

		case FRONTEND_REPLAY:
			SPinit(LEVEL_TYPE::SKIRMISH);
			bMultiPlayer = true;
			addLoadSave(LOADREPLAY_FRONTEND_SKIRMISH, _("Load Skirmish Replay"));  // change mode when loadsave returns
			break;

		case FRONTEND_SKIRMISH:
			SPinit(LEVEL_TYPE::SKIRMISH);
			sstrcpy(game.map, DEFAULTSKIRMISHMAP);
			game.hash = levGetMapNameHash(game.map);
			game.maxPlayers = DEFAULTSKIRMISHMAPMAXPLAYERS;

			ingame.side = InGameSide::HOST_OR_SINGLEPLAYER;
			changeTitleUI(std::make_shared<WzMultiplayerOptionsTitleUI>(wzTitleUICurrent));
			break;

		case FRONTEND_QUIT:
			changeTitleMode(TITLE);
			break;

		case FRONTEND_CHALLENGES:
			SPinit(LEVEL_TYPE::SKIRMISH);
			addChallenges();
			break;

		case FRONTEND_HYPERLINK:
			runHyperlink();
			break;

		default:
			break;
		}

		if (CancelPressed())
		{
			changeTitleMode(TITLE);
		}
	}

	if (!bLoadSaveUp && !challengesUp)						// if save/load screen is up
	{
		widgDisplayScreen(psWScreen);						// show the widgets currently running
	}
	if (bLoadSaveUp)								// if save/load screen is up
	{
		displayLoadSave();
	}
	else if (challengesUp)
	{
		displayChallenges();
	}

	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// Multi Player Menu
void startMultiPlayerMenu()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	addSideText(FRONTEND_SIDETEXT ,	FRONTEND_SIDEX, FRONTEND_SIDEY, _("MULTI PLAYER"));

	addTextButton(FRONTEND_HOST,     FRONTEND_POS2X, FRONTEND_POS2Y, _("Host Game"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_JOIN,     FRONTEND_POS3X, FRONTEND_POS3Y, _("Join Game"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_REPLAY,   FRONTEND_POS7X, FRONTEND_POS7Y, _("View Replay"), WBUT_TXTCENTRE);

	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	// This isn't really a hyperlink for now... perhaps link to the wiki ?
	addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS9X, FRONTEND_POS9Y, _("TCP port 2100 must be opened in your firewall or router to host games!"), 0);
}

bool runMultiPlayerMenu()
{
	if (bLoadSaveUp)
	{
		if (runLoadSave(false)) // check for file name.
		{
			loadOK();
		}
	}
	else
	{
		WidgetTriggers const &triggers = widgRunScreen(psWScreen);
		unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

		switch (id)
		{
		case FRONTEND_HOST:
			// don't pretend we are running a network game. Really do it!
			NetPlay.bComms = true; // use network = true
			NetPlay.isUPNP_CONFIGURED = false;
			NetPlay.isUPNP_ERROR = false;
			ingame.side = InGameSide::HOST_OR_SINGLEPLAYER;
			bMultiPlayer = true;
			bMultiMessages = true;
			NETinit(true);
			NETdiscoverUPnPDevices();
			game.type = LEVEL_TYPE::SKIRMISH;		// needed?
			changeTitleUI(std::make_shared<WzMultiplayerOptionsTitleUI>(wzTitleUICurrent));
			break;
		case FRONTEND_JOIN:
			NETinit(true);
			ingame.side = InGameSide::MULTIPLAYER_CLIENT;
			if (getLobbyError() != ERROR_INVALID)
			{
				setLobbyError(ERROR_NOERROR);
			}
			changeTitleUI(std::make_shared<WzProtocolTitleUI>());
			break;
		case FRONTEND_REPLAY:
			SPinit(LEVEL_TYPE::SKIRMISH);
			bMultiPlayer = true;
			game.maxPlayers = DEFAULTSKIRMISHMAPMAXPLAYERS;
			addLoadSave(LOADREPLAY_FRONTEND_MULTI, _("Load Multiplayer Replay"));  // change mode when loadsave returns
			break;

		case FRONTEND_QUIT:
			changeTitleMode(TITLE);
			break;
		default:
			break;
		}

		if (CancelPressed())
		{
			changeTitleMode(TITLE);
		}
	}

	if (!bLoadSaveUp)
	{
		widgDisplayScreen(psWScreen);		// show the widgets currently running
	}
	else if (bLoadSaveUp)					// if save/load screen is up
	{
		displayLoadSave();
	}

	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// Change Mode
void changeTitleMode(tMode mode)
{
	changeTitleUI(std::make_shared<WzOldTitleUI>(mode));
}

// ////////////////////////////////////////////////////////////////////////////
// Handling Screen Size Changes

#define DISPLAY_SCALE_PROMPT_TAG_PREFIX		"displayscale::prompt::"
#define DISPLAY_SCALE_PROMPT_INCREASE_TAG	DISPLAY_SCALE_PROMPT_TAG_PREFIX "increase"
#define WZ_SUGGESTED_MAX_LOGICAL_SCREEN_SIZE_DIMENSION 1440
static unsigned int lastProcessedDisplayScale = 0;
static int lastProcessedScreenWidth = 0;
static int lastProcessedScreenHeight = 0;

/* Tell the frontend when the screen has been resized */
void frontendScreenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight)
{
	// NOTE:
	// By setting the appropriate calcLayout functions on all interface elements,
	// they should automatically recalculate their layout on screen resize.
	if (wzTitleUICurrent)
	{
		std::shared_ptr<WzTitleUI> current = wzTitleUICurrent;
		current->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
	}

	// Determine if there should be a prompt to increase display scaling
	const unsigned int currentDisplayScale = wzGetCurrentDisplayScale();
	if ((newWidth >= oldWidth && newHeight >= oldHeight)
		&& (lastProcessedScreenWidth != newWidth || lastProcessedScreenHeight != newHeight) // filter out duplicate events
		&& (lastProcessedDisplayScale == 0 || lastProcessedDisplayScale == currentDisplayScale)	// filter out duplicate events & display-scale-only changes
		&& (newWidth >= WZ_SUGGESTED_MAX_LOGICAL_SCREEN_SIZE_DIMENSION || newHeight >= WZ_SUGGESTED_MAX_LOGICAL_SCREEN_SIZE_DIMENSION))
	{
		unsigned int suggestedDisplayScale = wzGetSuggestedDisplayScaleForCurrentWindowSize(WZ_SUGGESTED_MAX_LOGICAL_SCREEN_SIZE_DIMENSION);
		if ((suggestedDisplayScale > currentDisplayScale)
			&& !hasNotificationsWithTag(DISPLAY_SCALE_PROMPT_INCREASE_TAG, NotificationScope::DISPLAYED_ONLY))
		{
			cancelOrDismissNotificationIfTag([](const std::string& tag) {
				return (tag.rfind(DISPLAY_SCALE_PROMPT_TAG_PREFIX, 0) == 0);
			});

			WZ_Notification notification;
			notification.duration = 0;
			notification.contentTitle = _("Increase Game Display Scale?");
			std::string contextText = "\n";
			contextText += _("With your current resolution & display scale settings, the game's user interface may appear small, and the game perspective may appear distorted.");
			contextText += "\n\n";
			contextText += _("You can fix this by increasing the game's Display Scale setting.");
			contextText += "\n\n";
			contextText += astringf(_("Would you like to increase the game's Display Scale to: %u%%?"), suggestedDisplayScale);
			notification.contentText = contextText;
			notification.action = WZ_Notification_Action(_("Increase Display Scale"), [](const WZ_Notification&) {
				// Determine maximum display scale for current window size
				unsigned int desiredDisplayScale = wzGetSuggestedDisplayScaleForCurrentWindowSize(WZ_SUGGESTED_MAX_LOGICAL_SCREEN_SIZE_DIMENSION);
				if (desiredDisplayScale == wzGetCurrentDisplayScale())
				{
					// nothing to do now
					return;
				}

				// Store the new display scale
				auto priorDisplayScale = war_GetDisplayScale();
				war_SetDisplayScale(desiredDisplayScale);

				if (wzChangeDisplayScale(desiredDisplayScale))
				{
					WZ_Notification completed_notification;
					completed_notification.duration = 6 * GAME_TICKS_PER_SEC;
					completed_notification.contentTitle = astringf(_("Display Scale Increased to: %u%%"), desiredDisplayScale);
					std::string contextText = _("You can adjust the Display Scale at any time in the Video Options menu.");
					completed_notification.contentText = contextText;
					completed_notification.tag = DISPLAY_SCALE_PROMPT_TAG_PREFIX "completed";
					addNotification(completed_notification, WZ_Notification_Trigger::Immediate());
				}
				else
				{
					// failed to change display scale - restore the prior setting
					war_SetDisplayScale(priorDisplayScale);
				}
			});
			notification.onDismissed = [](const WZ_Notification&, WZ_Notification_Dismissal_Reason reason) {
				if (reason != WZ_Notification_Dismissal_Reason::USER_DISMISSED) { return; }
				WZ_Notification dismissed_notification;
				dismissed_notification.duration = 0;
				dismissed_notification.contentTitle = _("Tip: Adjusting Display Scale");
				std::string contextText = _("You can adjust the Display Scale at any time in the Video Options menu.");
				dismissed_notification.contentText = contextText;
				dismissed_notification.tag = DISPLAY_SCALE_PROMPT_TAG_PREFIX "dismissed_info";
				dismissed_notification.displayOptions = WZ_Notification_Display_Options::makeOneTime("displayscale::tip");
				addNotification(dismissed_notification, WZ_Notification_Trigger::Immediate());
			};
			notification.tag = DISPLAY_SCALE_PROMPT_INCREASE_TAG;
			notification.displayOptions = WZ_Notification_Display_Options::makeIgnorable("displayscale::prompt_increase", 4);

			addNotification(notification, WZ_Notification_Trigger(GAME_TICKS_PER_SEC * 2));
		}
	}
	lastProcessedDisplayScale = currentDisplayScale;
	lastProcessedScreenWidth = newWidth;
	lastProcessedScreenHeight = newHeight;
}
