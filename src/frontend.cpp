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
#include "lib/widget/image.h"
#include "lib/widget/resize.h"

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
#include "activity.h"
#include "clparse.h" // for autorating

// ////////////////////////////////////////////////////////////////////////////
// Global variables

char			aLevelName[MAX_LEVEL_NAME_SIZE + 1];	//256];			// vital! the wrf file to use.

bool			bLimiterLoaded = false;

static std::shared_ptr<IMAGEFILE> pFlagsImages;

#define TUTORIAL_LEVEL "TUTORIAL3"
#define TRANSLATION_URL "https://translate.wz2100.net"

// ////////////////////////////////////////////////////////////////////////////
// Forward definitions

static W_BUTTON * addSmallTextButton(UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt, unsigned int style);
static std::shared_ptr<W_BUTTON> makeTextButton(UDWORD id, const std::string &txt, unsigned int style, optional<unsigned int> minimumWidth = nullopt);
static std::shared_ptr<W_SLIDER> makeFESlider(UDWORD id, UDWORD parent, UDWORD stops, UDWORD pos);
static std::shared_ptr<WIDGET> addMargin(std::shared_ptr<WIDGET> widget);

// ////////////////////////////////////////////////////////////////////////////
// Helper functions

// Returns true if escape key pressed.
//
bool CancelPressed()
{
	const bool cancel = keyPressed(KEY_ESC);
	if (cancel)
	{
		inputLoseFocus();	// clear the input buffer.
	}
	return cancel;
}

// Cycle through options as in program seq(1) from coreutils
// The T cast is to cycle through enums.
template <typename T> static T seqCycle(T value, T min, int inc, T max)
{
	return widgGetButtonKey_DEPRECATED(psWScreen) == WKEY_PRIMARY ?
		value < max ? T(value + inc) : min :  // Cycle forwards.
		min < value ? T(value - inc) : max;  // Cycle backwards.
}

// Cycle through options, which are powers of two, such as [128, 256, 512, 1024, 2048].
template <typename T> static T pow2Cycle(T value, T min, T max)
{
	return widgGetButtonKey_DEPRECATED(psWScreen) == WKEY_PRIMARY ?
		value < max ? std::max<T>(1, value) * 2 : min :  // Cycle forwards.
		min < value ? (value / 2 > 1 ? value / 2 : 0) : max;  // Cycle backwards.
}

static void moveToParentRightEdge(WIDGET *widget, int32_t right)
{
	if (auto parent = widget->parent())
	{
		widget->move(parent->width() - (widget->width() + right), widget->y());
	}
}

// ////////////////////////////////////////////////////////////////////////////
// Title Screen
static void runUpgrdHyperlink()
{
	std::string link = "https://wz2100.net/versioncheck/?ver=";
	std::string version = version_getVersionString();
	std::string versionStr;
	for (char ch : version)
	{
		versionStr += (ch == ' ') ? '_' : ch;
	}
	link += urlEncode(versionStr.c_str());
	openURLInBrowser(link.c_str());
}

static void runHyperlink()
{
	openURLInBrowser("https://wz2100.net/");
}

static void rundonatelink()
{
	openURLInBrowser("http://donations.wz2100.net/");
}

static void runchatlink()
{
	openURLInBrowser("https://wz2100.net/webchat/");
}

static void notifyAboutMissingVideos()
{
	const std::string VIDEO_TAG = "videoMissing";
	if (!hasNotificationsWithTag(VIDEO_TAG))
	{
		WZ_Notification notification;
		notification.duration = 0;
		notification.contentTitle = _("Campaign videos are missing");
		notification.contentText = _("See our FAQ on how to install videos");
		notification.tag = VIDEO_TAG;
		notification.largeIcon = WZ_Notification_Image("images/notifications/exclamation_triangle.png");
		notification.action = WZ_Notification_Action("Open wz2100.net", [](const WZ_Notification&) {
			runHyperlink();
		});
		notification.displayOptions = WZ_Notification_Display_Options::makeIgnorable("campaignVideoNotification", 2);

		addNotification(notification, WZ_Notification_Trigger(GAME_TICKS_PER_SEC));
	}
}

void startTitleMenu()
{
	intRemoveReticule();

	addBackdrop();
	addTopForm(false);
	addBottomForm();

	addTextButton(FRONTEND_SINGLEPLAYER, FRONTEND_POS2X, FRONTEND_POS2Y, _("Single Player"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_MULTIPLAYER, FRONTEND_POS3X, FRONTEND_POS3Y, _("Multi Player"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_TUTORIAL, FRONTEND_POS4X, FRONTEND_POS4Y, _("Tutorial"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_OPTIONS, FRONTEND_POS5X, FRONTEND_POS5Y, _("Options"), WBUT_TXTCENTRE);

	// check whether video sequences are installed
	if (seq_hasVideos())
	{
		addTextButton(FRONTEND_PLAYINTRO, FRONTEND_POS6X, FRONTEND_POS6Y, _("View Intro"), WBUT_TXTCENTRE);
	}
	else
	{
		addTextButton(FRONTEND_PLAYINTRO, FRONTEND_POS6X, FRONTEND_POS6Y, _("View Intro"), WBUT_TXTCENTRE | WBUT_DISABLE);
		widgSetTip(psWScreen, FRONTEND_PLAYINTRO, _("Videos are missing, download them from http://wz2100.net"));

		notifyAboutMissingVideos();
	}

	if (findLastSave())
	{
		addTextButton(FRONTEND_CONTINUE, FRONTEND_POS7X, FRONTEND_POS7Y, _("Continue Last Save"), WBUT_TXTCENTRE);
	}
	else
	{
		addTextButton(FRONTEND_CONTINUE, FRONTEND_POS7X, FRONTEND_POS7Y, _("Continue Last Save"), WBUT_TXTCENTRE | WBUT_DISABLE);
		widgSetTip(psWScreen, FRONTEND_CONTINUE, _("No last save available"));
	}
	addTextButton(FRONTEND_QUIT, FRONTEND_POS8X, FRONTEND_POS8Y, _("Quit Game"), WBUT_TXTCENTRE);
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("MAIN MENU"));

	addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS9X, FRONTEND_POS9Y, _("Official site: http://wz2100.net/"), 0);
	widgSetTip(psWScreen, FRONTEND_HYPERLINK, _("Come visit the forums and all Warzone 2100 news! Click this link."));
	W_BUTTON * pRightAlignedButton = addSmallTextButton(FRONTEND_DONATELINK, FRONTEND_POS9X + 360, FRONTEND_POS9Y, _("Donate: http://donations.wz2100.net/"), 0);
	moveToParentRightEdge(pRightAlignedButton, 1);
	widgSetTip(psWScreen, FRONTEND_DONATELINK, _("Help support the project with our server costs, Click this link."));
	pRightAlignedButton = addSmallTextButton(FRONTEND_CHATLINK, FRONTEND_POS9X + 360, 0, _("Chat with players on Discord or IRC"), 0);
	moveToParentRightEdge(pRightAlignedButton, 6);
	widgSetTip(psWScreen, FRONTEND_CHATLINK, _("Connect to Discord or Freenode through webchat by clicking this link."));
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_UPGRDLINK, 7, 7, MULTIOP_BUTW, MULTIOP_BUTH, _("Check for a newer version"), IMAGE_GAMEVERSION, IMAGE_GAMEVERSION_HI, true);
}

void runContinue()
{
	SPinit(lastSaveMP ? LEVEL_TYPE::SKIRMISH : LEVEL_TYPE::CAMPAIGN);
	sstrcpy(saveGameName, lastSavePath.toPath(SaveGamePath_t::Extension::GAM).c_str());
	bMultiPlayer = lastSaveMP;
	setCampaignNumber(getCampaign(saveGameName));
}

bool runTitleMenu()
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_QUIT:
		changeTitleMode(QUIT);
		break;
	case FRONTEND_MULTIPLAYER:
		changeTitleMode(MULTI);
		break;
	case FRONTEND_SINGLEPLAYER:
		changeTitleMode(SINGLE);
		break;
	case FRONTEND_OPTIONS:
		changeTitleMode(OPTIONS);
		break;
	case FRONTEND_TUTORIAL:
		changeTitleMode(TUTORIAL);
		break;
	case FRONTEND_PLAYINTRO:
		changeTitleMode(SHOWINTRO);
		break;
	case FRONTEND_HYPERLINK:
		runHyperlink();
		break;
	case FRONTEND_UPGRDLINK:
		runUpgrdHyperlink();
		break;
	case FRONTEND_DONATELINK:
		rundonatelink();
		break;
	case FRONTEND_CHATLINK:
		runchatlink();
		break;
	case FRONTEND_CONTINUE:
		runContinue();
		changeTitleMode(LOADSAVEGAME);
		break;

	default:
		break;
	}

	widgDisplayScreen(psWScreen); // show the widgets currently running

	return true;
}


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
	if (!seq_hasVideos())
	{
		addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS9X, FRONTEND_POS9Y, _("Campaign videos are missing! Get them from http://wz2100.net"), 0);
		notifyAboutMissingVideos();
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
	if (!seq_hasVideos())
	{
		addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS9X, FRONTEND_POS9Y, _("Campaign videos are missing! Get them from http://wz2100.net"), 0);
		notifyAboutMissingVideos();
	}
}

static void frontEndNewGame(int which)
{
	std::vector<CAMPAIGN_FILE> list = readCampaignFiles();
	sstrcpy(aLevelName, list[which].level.toUtf8().c_str());
	// show this only when the video sequences are installed
	if (seq_hasVideos())
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
		if (!loadLevFile(list[which].loading.toUtf8(), mod_campaign, false, nullptr))
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
	char buf[512]  =  {'\0'};
	snprintf(buf, sizeof(buf), _("TCP port %d must be opened in your firewall or router to host games!"), NETgetGameserverPort());
	addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS9X, FRONTEND_POS9Y, buf, 0);
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
// Options Menu
void startOptionsMenu()
{
	sliderEnableDrag(true);

	addBackdrop();
	addTopForm(false);
	addBottomForm();

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BOTFORM);
	ASSERT_OR_RETURN(, parent != nullptr, "Unable to find FRONTEND_BOTFORM??");

	auto scrollableList = ScrollableListWidget::make();

	auto addTextButton = [scrollableList](UDWORD id, const std::string &txt, unsigned int style)
	{
		auto button = makeTextButton(id, txt, style);
		if (style & WBUT_TXTCENTRE)
		{
			button->setGeometry(0, 0, FRONTEND_BUTWIDTH, button->height());
		}
		scrollableList->addItem(button);
	};

	addTextButton(FRONTEND_GAMEOPTIONS, _("Game Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_GRAPHICSOPTIONS, _("Graphics Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_VIDEOOPTIONS, _("Video Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_AUDIO_AND_ZOOMOPTIONS, _("Audio / Zoom Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_MOUSEOPTIONS, _("Mouse Options"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_KEYMAP, _("Key Mappings"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_MUSICMANAGER, _("Music Manager"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_MULTIPLAYOPTIONS, _("Multiplay Options"), WBUT_TXTCENTRE);

	scrollableList->setGeometry(0, FRONTEND_POS2Y, FRONTEND_BOTFORMW - 1, FRONTEND_POS9Y - FRONTEND_POS2Y);
	parent->attach(scrollableList);

	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("OPTIONS"));
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
	addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS9X, FRONTEND_POS9Y, _("Open Configuration Directory"), 0);
}

bool runOptionsMenu()
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_GAMEOPTIONS:
		changeTitleMode(GAME);
		break;
	case FRONTEND_GRAPHICSOPTIONS:
		changeTitleMode(GRAPHICS_OPTIONS);
		break;
	case FRONTEND_AUDIO_AND_ZOOMOPTIONS:
		changeTitleMode(AUDIO_AND_ZOOM_OPTIONS);
		break;
	case FRONTEND_VIDEOOPTIONS:
		changeTitleMode(VIDEO_OPTIONS);
		break;
	case FRONTEND_MOUSEOPTIONS:
		changeTitleMode(MOUSE_OPTIONS);
		break;
	case FRONTEND_KEYMAP:
		changeTitleMode(KEYMAP);
		break;
	case FRONTEND_MUSICMANAGER:
		changeTitleMode(MUSIC_MANAGER);
		break;
	case FRONTEND_MULTIPLAYOPTIONS:
		changeTitleMode(MULTIPLAY_OPTIONS);
		break;
	case FRONTEND_QUIT:
		changeTitleMode(TITLE);
		break;
	case FRONTEND_HYPERLINK:
		if (!openFolderInDefaultFileManager(PHYSFS_getWriteDir()))
		{
			// Failed to open write dir in default filesystem browser
			std::string configErrorMessage = _("Failed to open configuration directory in system default file browser.");
			configErrorMessage += "\n\n";
			configErrorMessage += _("Configuration directory is reported as:");
			configErrorMessage += "\n";
			configErrorMessage += PHYSFS_getWriteDir();
			configErrorMessage += "\n\n";
			configErrorMessage += _("If running inside a container / isolated environment, this may differ from the actual path on disk.");
			configErrorMessage += "\n";
			configErrorMessage += _("Please see the documentation for more information on how to locate it manually.");
			wzDisplayDialog(Dialog_Warning, _("Failed to open configuration directory"), configErrorMessage.c_str());
		}
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

char const *graphicsOptionsFmvmodeString()
{
	switch (war_GetFMVmode())
	{
	case FMV_1X: return _("1×");
	case FMV_2X: return _("2×");
	case FMV_FULLSCREEN: return _("Fullscreen");
	default: return _("Unsupported");
	}
}

char const *graphicsOptionsScanlinesString()
{
	switch (war_getScanlineMode())
	{
	case SCANLINES_OFF: return _("Off");
	case SCANLINES_50: return _("50%");
	case SCANLINES_BLACK: return _("Black");
	default: return _("Unsupported");
	}
}

char const *graphicsOptionsScreenShakeString()
{
	return getShakeStatus() ? _("On") : _("Off");
}

char const *graphicsOptionsSubtitlesString()
{
	return seq_GetSubtitles() ? _("On") : _("Off");
}

char const *graphicsOptionsShadowsString()
{
	return getDrawShadows() ? _("On") : _("Off");
}

char const *graphicsOptionsFogString()
{
	return pie_GetFogEnabled() ? _("On") : _("Off");
}

char const *graphicsOptionsRadarString()
{
	return rotateRadar ? _("Rotating") : _("Fixed");
}

char const *graphicsOptionsRadarJumpString()
{
	return war_GetRadarJump() ? _("Instant") : _("Tracked");
}

static std::shared_ptr<WIDGET> makeLODDistanceDropdown()
{
	std::vector<std::tuple<WzString, int>> dropDownChoices = {
		{_("High"), WZ_LODDISTANCEPERCENTAGE_HIGH},
		{_("Default"), 0} // the *system* default (no supplied bias)
	};

	// If current value (from config) is not one of the presets in dropDownChoices, add a "Custom" entry
	size_t currentSettingIdx = 0;
	int currValue = war_getLODDistanceBiasPercentage();
	auto it = std::find_if(dropDownChoices.begin(), dropDownChoices.end(), [currValue](const std::tuple<WzString, int>& item) -> bool {
		return std::get<1>(item) == currValue;
	});
	if (it != dropDownChoices.end())
	{
		currentSettingIdx = it - dropDownChoices.begin();
	}
	else
	{
		dropDownChoices.push_back({WzString::fromUtf8(astringf("(%d)", currValue)), currValue});
		currentSettingIdx = dropDownChoices.size() - 1;
	}

	auto dropdown = std::make_shared<DropdownWidget>();
	dropdown->id = FRONTEND_LOD_DISTANCE_R;
	dropdown->setListHeight(FRONTEND_BUTHEIGHT * std::min<uint32_t>(5, dropDownChoices.size()));
	const auto paddingSize = 10;

	for (const auto& option : dropDownChoices)
	{
		auto item = makeTextButton(0, std::get<0>(option).toUtf8(), 0);
		dropdown->addItem(Margin(0, paddingSize).wrap(item));
	}

	dropdown->setSelectedIndex(currentSettingIdx);

	dropdown->setOnChange([dropDownChoices](DropdownWidget& dropdown) {
		if (auto selectedIndex = dropdown.getSelectedIndex())
		{
			ASSERT_OR_RETURN(, selectedIndex.value() < dropDownChoices.size(), "Invalid index");
			war_setLODDistanceBiasPercentage(std::get<1>(dropDownChoices.at(selectedIndex.value())));
		}
	});

	return Margin(0, 10).wrap(dropdown);
}

// ////////////////////////////////////////////////////////////////////////////
// Graphics Options
void startGraphicsOptionsMenu()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BOTFORM);

	auto label = std::make_shared<W_LABEL>();
	parent->attach(label);
	label->setGeometry(FRONTEND_POS1X + 48, FRONTEND_POS1Y - 14, FRONTEND_BUTWIDTH - FRONTEND_POS1X - 48, FRONTEND_BUTHEIGHT);
	label->setFontColour(WZCOL_TEXT_BRIGHT);
	label->setString(_("* Takes effect on game restart"));
	label->setTextAlignment(WLAB_ALIGNBOTTOMLEFT);

	auto grid = std::make_shared<GridLayout>();
	grid_allocation::slot row(0);

	////////////
	//FMV mode.
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_FMVMODE, _("Video Playback"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_FMVMODE_R, graphicsOptionsFmvmodeString(), WBUT_SECONDARY)));
	row.start++;

	// Scanlines
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_SCANLINES, _("Scanlines"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_SCANLINES_R, graphicsOptionsScanlinesString(), WBUT_SECONDARY)));
	row.start++;

	////////////
	//shadows
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_SHADOWS, _("Shadows"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_SHADOWS_R, graphicsOptionsShadowsString(), WBUT_SECONDARY)));
	row.start++;

	// fog
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_FOG, _("Fog"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_FOG_R, graphicsOptionsFogString(), WBUT_SECONDARY)));
	row.start++;

	// Radar
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_RADAR, _("Radar"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_RADAR_R, graphicsOptionsRadarString(), WBUT_SECONDARY)));
	row.start++;

	// RadarJump
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_RADAR_JUMP, _("Radar Jump"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_RADAR_JUMP_R, graphicsOptionsRadarJumpString(), WBUT_SECONDARY)));
	row.start++;

	// LOD Distance
	// TRANSLATORS: "LOD" = "Level of Detail" - this setting is used to describe how level of detail (in textures) is preserved as distance increases (examples: "Default", "High", etc)
	std::string lodDistanceString = _("LOD Distance");
	lodDistanceString += "*"; // takes effect on game restart
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_LOD_DISTANCE, lodDistanceString.c_str(), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, makeLODDistanceDropdown());
	row.start++;

	// screenshake
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_SSHAKE, _("Screen Shake"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_SSHAKE_R, graphicsOptionsScreenShakeString(), WBUT_SECONDARY)));
	row.start++;

	grid->setGeometry(0, 0, FRONTEND_BUTWIDTH, grid->idealHeight());

	auto scrollableList = ScrollableListWidget::make();
	scrollableList->setGeometry(0, FRONTEND_POS2Y, FRONTEND_BOTFORMW - 1, FRONTEND_BOTFORMH - FRONTEND_POS2Y - 1);
	scrollableList->addItem(grid);
	parent->attach(scrollableList);

	// Add some text down the side of the form
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("GRAPHICS OPTIONS"));

	////////////
	// quit.
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
}

void seqFMVmode()
{
	war_SetFMVmode(seqCycle(war_GetFMVmode(), FMV_FULLSCREEN, 1, FMV_MODE(FMV_MAX - 1)));
}

void seqScanlineMode()
{
	war_setScanlineMode(seqCycle(war_getScanlineMode(), SCANLINES_OFF, 1, SCANLINES_BLACK));
}

bool runGraphicsOptionsMenu()
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	case FRONTEND_SHADOWS:
	case FRONTEND_SHADOWS_R:
		setDrawShadows(!getDrawShadows());
		widgSetString(psWScreen, FRONTEND_SHADOWS_R, graphicsOptionsShadowsString());
		break;

	case FRONTEND_FOG:
	case FRONTEND_FOG_R:
		if (pie_GetFogEnabled())
		{
			pie_SetFogStatus(false);
			pie_EnableFog(false);
		}
		else
		{
			pie_EnableFog(true);
		}
		widgSetString(psWScreen, FRONTEND_FOG_R, graphicsOptionsFogString());
		break;

	case FRONTEND_FMVMODE:
	case FRONTEND_FMVMODE_R:
		seqFMVmode();
		widgSetString(psWScreen, FRONTEND_FMVMODE_R, graphicsOptionsFmvmodeString());
		break;

	case FRONTEND_SCANLINES:
	case FRONTEND_SCANLINES_R:
		seqScanlineMode();
		widgSetString(psWScreen, FRONTEND_SCANLINES_R, graphicsOptionsScanlinesString());
		break;

	case FRONTEND_RADAR:
	case FRONTEND_RADAR_R:
		rotateRadar = !rotateRadar;
		widgSetString(psWScreen, FRONTEND_RADAR_R, graphicsOptionsRadarString());
		break;

	case FRONTEND_RADAR_JUMP:
	case FRONTEND_RADAR_JUMP_R:
		war_SetRadarJump(!war_GetRadarJump());
		widgSetString(psWScreen, FRONTEND_RADAR_JUMP_R, graphicsOptionsRadarJumpString());
		break;

	case FRONTEND_SSHAKE:
	case FRONTEND_SSHAKE_R:
		setShakeStatus(!getShakeStatus());
		widgSetString(psWScreen, FRONTEND_SSHAKE_R, graphicsOptionsScreenShakeString());
		break;

	default:
		break;
	}


	// If close button pressed then return from this menu.
	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);						// show the widgets currently running

	return true;
}

static std::string audioAndZoomOptionsSoundHRTFMode()
{
	// retrieve whether the current "setting" is Auto or not
	// as sound_GetHRTFMode() returns the current actual status (and never auto, even if the request was "auto")
	bool isAutoSetting = war_GetHRTFMode() == HRTFMode::Auto;

	std::string currentMode;
	auto mode = sound_GetHRTFMode();
	switch (mode)
	{
	case HRTFMode::Unsupported:
		currentMode = _("Unsupported");
		return currentMode;
	case HRTFMode::Disabled:
		currentMode = _("Disabled");
		break;
	case HRTFMode::Enabled:
		currentMode = _("Enabled");
		break;
	case HRTFMode::Auto:
		// should not happen, but if it does, just return
		currentMode = _("Auto");
		return currentMode;
	}

	if (isAutoSetting)
	{
		return std::string(_("Auto")) + " (" + currentMode + ")";
	}
	return currentMode;
}

static std::string audioAndZoomOptionsMapZoomString()
{
	char mapZoom[20];
	ssprintf(mapZoom, "%d", war_GetMapZoom());
	return mapZoom;
}

static std::string audioAndZoomOptionsMapZoomRateString()
{
	char mapZoomRate[20];
	ssprintf(mapZoomRate, "%d", war_GetMapZoomRate());
	return mapZoomRate;
}

static std::string audioAndZoomOptionsRadarZoomString()
{
	char radarZoom[20];
	ssprintf(radarZoom, "%d", war_GetRadarZoom());
	return radarZoom;
}

// ////////////////////////////////////////////////////////////////////////////
// Audio and Zoom Options Menu
void startAudioAndZoomOptionsMenu()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BOTFORM);

	auto addSliderWrap = [](std::shared_ptr<WIDGET> widget) {
		Alignment sliderAlignment(Alignment::Vertical::Center, Alignment::Horizontal::Left);
		return sliderAlignment.wrap(addMargin(widget));
	};

	auto grid = std::make_shared<GridLayout>();
	grid_allocation::slot row(0);

	// 2d audio
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_FX, _("Voice Volume"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addSliderWrap(makeFESlider(FRONTEND_FX_SL, FRONTEND_BOTFORM, AUDIO_VOL_MAX, static_cast<UDWORD>(sound_GetUIVolume() * 100.0f))));
	row.start++;

	// 3d audio
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_3D_FX, _("FX Volume"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addSliderWrap(makeFESlider(FRONTEND_3D_FX_SL, FRONTEND_BOTFORM, AUDIO_VOL_MAX, static_cast<UDWORD>(sound_GetEffectsVolume() * 100.0f))));
	row.start++;

	// cd audio
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_MUSIC, _("Music Volume"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addSliderWrap(makeFESlider(FRONTEND_MUSIC_SL, FRONTEND_BOTFORM, AUDIO_VOL_MAX, static_cast<UDWORD>(sound_GetMusicVolume() * 100.0f))));
	row.start++;

	////////////
	//subtitle mode.
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_SUBTITLES, _("Subtitles"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_SUBTITLES_R, graphicsOptionsSubtitlesString(), WBUT_SECONDARY)));
	row.start++;

	// HRTF
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_SOUND_HRTF, _("HRTF"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_SOUND_HRTF_R, audioAndZoomOptionsSoundHRTFMode(), WBUT_SECONDARY)));
	row.start++;
	if (sound_GetHRTFMode() == HRTFMode::Unsupported)
	{
		widgSetButtonState(psWScreen, FRONTEND_SOUND_HRTF, WBUT_DISABLE);
		widgSetTip(psWScreen, FRONTEND_SOUND_HRTF, _("HRTF is not supported on your device / system / OpenAL library"));
		widgSetButtonState(psWScreen, FRONTEND_SOUND_HRTF_R, WBUT_DISABLE);
		widgSetTip(psWScreen, FRONTEND_SOUND_HRTF_R, _("HRTF is not supported on your device / system / OpenAL library"));
	}

	// map zoom
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_MAP_ZOOM, _("Map Zoom"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_MAP_ZOOM_R, audioAndZoomOptionsMapZoomString(), WBUT_SECONDARY)));
	row.start++;

	// map zoom rate
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_MAP_ZOOM_RATE, _("Map Zoom Rate"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_MAP_ZOOM_RATE_R, audioAndZoomOptionsMapZoomRateString(), WBUT_SECONDARY)));
	row.start++;

	// radar zoom
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_RADAR_ZOOM, _("Radar Zoom"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_RADAR_ZOOM_R, audioAndZoomOptionsRadarZoomString(), WBUT_SECONDARY)));
	row.start++;

	grid->setGeometry(0, 0, FRONTEND_BUTWIDTH, grid->idealHeight());

	auto scrollableList = ScrollableListWidget::make();
	scrollableList->setGeometry(0, FRONTEND_POS2Y, FRONTEND_BOTFORMW - 1, FRONTEND_BOTFORMH - FRONTEND_POS2Y - 1);
	scrollableList->addItem(grid);
	parent->attach(scrollableList);

	// quit.
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	//add some text down the side of the form
	// TRANSLATORS: "AUDIO" options determine the volume of game sounds.
	// "OPTIONS" means "SETTINGS".
	// To break this message into two lines, you can use the delimiter "\n",
	// e.g. "AUDIO / ZOOM\nOPTIONS" would show "OPTIONS" in a second line.
	WzString messageString = WzString::fromUtf8(_("AUDIO / ZOOM OPTIONS"));
	std::vector<WzString> messageStringLines = messageString.split("\n");
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, messageStringLines[0].toUtf8().c_str());
	// show a second sidetext line if the translation requires it
	if (messageStringLines.size() > 1)
	{
		messageString.remove(0, messageStringLines[0].length() + 1);
		addSideText(FRONTEND_MULTILINE_SIDETEXT, FRONTEND_SIDEX + 22, \
		FRONTEND_SIDEY, messageString.toUtf8().c_str());
	}
}

bool runAudioAndZoomOptionsMenu()
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_FX:
	case FRONTEND_3D_FX:
	case FRONTEND_MUSIC:
		break;

	case FRONTEND_FX_SL:
		sound_SetUIVolume((float)widgGetSliderPos(psWScreen, FRONTEND_FX_SL) / 100.0f);
		break;

	case FRONTEND_3D_FX_SL:
		sound_SetEffectsVolume((float)widgGetSliderPos(psWScreen, FRONTEND_3D_FX_SL) / 100.0f);
		break;

	case FRONTEND_MUSIC_SL:
		sound_SetMusicVolume((float)widgGetSliderPos(psWScreen, FRONTEND_MUSIC_SL) / 100.0f);
		break;

	case FRONTEND_SUBTITLES:
	case FRONTEND_SUBTITLES_R:
		seq_SetSubtitles(!seq_GetSubtitles());
		widgSetString(psWScreen, FRONTEND_SUBTITLES_R, graphicsOptionsSubtitlesString());
		break;

	case FRONTEND_SOUND_HRTF:
	case FRONTEND_SOUND_HRTF_R:
		{
			std::vector<HRTFMode> modesToCycle = { HRTFMode::Disabled, HRTFMode::Enabled, HRTFMode::Auto };
			auto current = std::find(modesToCycle.begin(), modesToCycle.end(), war_GetHRTFMode());
			if (current == modesToCycle.end())
			{
				current = modesToCycle.begin();
			}
			auto startingPoint = current;
			bool successfulChange = false;
			do {
				current = seqCycle(current, modesToCycle.begin(), 1, modesToCycle.end() - 1);
			} while ( (current != startingPoint) && ((successfulChange = sound_SetHRTFMode(*current)) == false) );
			if (successfulChange)
			{
				war_SetHRTFMode(*current);
				widgSetString(psWScreen, FRONTEND_SOUND_HRTF_R, audioAndZoomOptionsSoundHRTFMode().c_str());
			}
			break;
		}

	case FRONTEND_MAP_ZOOM:
	case FRONTEND_MAP_ZOOM_R:
		{
		    war_SetMapZoom(seqCycle(war_GetMapZoom(), MINDISTANCE_CONFIG, MAP_ZOOM_CONFIG_STEP, MAXDISTANCE));
		    widgSetString(psWScreen, FRONTEND_MAP_ZOOM_R, audioAndZoomOptionsMapZoomString().c_str());
		    break;
		}

	case FRONTEND_MAP_ZOOM_RATE:
	case FRONTEND_MAP_ZOOM_RATE_R:
		{
		    war_SetMapZoomRate(seqCycle(war_GetMapZoomRate(), MAP_ZOOM_RATE_MIN, MAP_ZOOM_RATE_STEP, MAP_ZOOM_RATE_MAX));
		    widgSetString(psWScreen, FRONTEND_MAP_ZOOM_RATE_R, audioAndZoomOptionsMapZoomRateString().c_str());
		    break;
		}

	case FRONTEND_RADAR_ZOOM:
	case FRONTEND_RADAR_ZOOM_R:
		{
		    war_SetRadarZoom(seqCycle(war_GetRadarZoom(), MIN_RADARZOOM, RADARZOOM_STEP, MAX_RADARZOOM));
		    widgSetString(psWScreen, FRONTEND_RADAR_ZOOM_R, audioAndZoomOptionsRadarZoomString().c_str());
		    break;
		}

	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	default:
		break;
	}

	// If close button pressed then return from this menu.
	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);						// show the widgets currently running

	return true;
}

static char const *videoOptionsResolutionGetReadOnlyTooltip()
{
	switch (war_getWindowMode())
	{
		case WINDOW_MODE::desktop_fullscreen:
			return _("In Desktop Fullscreen mode, the resolution matches that of your desktop \n(or what the window manager allows).");
		case WINDOW_MODE::windowed:
			return _("You can change the resolution by resizing the window normally. (Try dragging a corner / edge.)");
		default:
			return "";
	}
}

static void videoOptionsDisableResolutionButtons()
{
	widgReveal(psWScreen, FRONTEND_RESOLUTION_READONLY_LABEL_CONTAINER);
	widgReveal(psWScreen, FRONTEND_RESOLUTION_READONLY_CONTAINER);
	auto readonlyResolutionTooltip = videoOptionsResolutionGetReadOnlyTooltip();
	widgSetTip(psWScreen, FRONTEND_RESOLUTION_READONLY_LABEL, readonlyResolutionTooltip);
	widgSetTip(psWScreen, FRONTEND_RESOLUTION_READONLY, readonlyResolutionTooltip);
	widgHide(psWScreen, FRONTEND_RESOLUTION_DROPDOWN_LABEL_CONTAINER);
	widgHide(psWScreen, FRONTEND_RESOLUTION_DROPDOWN);
}

static void videoOptionsEnableResolutionButtons()
{
	widgHide(psWScreen, FRONTEND_RESOLUTION_READONLY_LABEL_CONTAINER);
	widgHide(psWScreen, FRONTEND_RESOLUTION_READONLY_CONTAINER);
	widgReveal(psWScreen, FRONTEND_RESOLUTION_DROPDOWN_LABEL_CONTAINER);
	widgReveal(psWScreen, FRONTEND_RESOLUTION_DROPDOWN);
}

static std::string videoOptionsAntialiasingString()
{
	if (war_getAntialiasing() == 0)
	{
		return _("Off");
	}
	else
	{
		return std::to_string(war_getAntialiasing()) + "×";
	}
}

char const *videoOptionsWindowModeLabel()
{
	return _("Graphics Mode");
}

static char const *videoOptionsResolutionLabel()
{
	return _("Resolution");
}

char const *videoOptionsDisplayScaleLabel()
{
	return _("Display Scale");
}

static std::string videoOptionsTextureSizeString()
{
	char textureSize[20];
	ssprintf(textureSize, "%d", getTextureSize());
	return textureSize;
}

gfx_api::context::swap_interval_mode getCurrentSwapMode()
{
	return to_swap_mode(war_GetVsync());
}

void saveCurrentSwapMode(gfx_api::context::swap_interval_mode mode)
{
	war_SetVsync(to_int(mode));
}

char const *videoOptionsVsyncString()
{
	switch (getCurrentSwapMode()) {
		case gfx_api::context::swap_interval_mode::immediate:
			return _("Off");
		case gfx_api::context::swap_interval_mode::vsync:
			return _("On");
		case gfx_api::context::swap_interval_mode::adaptive_vsync:
			return _("Adaptive");
	}
	return "n/a";
}

std::string videoOptionsDisplayScaleString()
{
	char resolution[100];
	ssprintf(resolution, "%d%%", war_GetDisplayScale());
	return resolution;
}

std::string videoOptionsGfxBackendString()
{
	return to_display_string(war_getGfxBackend());
}

// ////////////////////////////////////////////////////////////////////////////
// Video Options

class ScreenResolutionsModel
{
public:
	typedef const std::vector<screeninfo>::iterator iterator;
	typedef const std::vector<screeninfo>::const_iterator const_iterator;

	ScreenResolutionsModel(): modes(ScreenResolutionsModel::loadModes())
	{
	}

	const_iterator begin() const
	{
		return modes.begin();
	}

	const_iterator end() const
	{
		return modes.end();
	}

	const_iterator findResolutionClosestToCurrent() const
	{
		auto currentResolution = getCurrentResolution();
		auto closest = std::lower_bound(modes.begin(), modes.end(), currentResolution, compareLess);
		if (closest != modes.end() && compareEq(*closest, currentResolution))
		{
			return closest;
		}

		if (closest != modes.begin())
		{
			--closest;  // If current resolution doesn't exist, round down to next-highest one.
		}

		return closest;
	}

	void selectAt(size_t index) const
	{
		// Disable the ability to use the Video options menu to live-change the window size when in windowed mode.
		// Why?
		//	- Certain window managers don't report their own changes to window size through SDL in all circumstances.
		//	  (For example, attempting to set a window size of 800x600 might result in no actual change when using a
		//	   tiling window manager (ex. i3), but SDL thinks the window size has been set to 800x600. This obviously
		//     breaks things.)
		//  - Manual window resizing is supported (so there is no need for this functionality in the Video menu).
		ASSERT_OR_RETURN(, wzGetCurrentWindowMode() == WINDOW_MODE::fullscreen, "Attempt to change resolution in windowed-mode / desktop-fullscreen mode");

		ASSERT_OR_RETURN(, index < modes.size(), "Invalid mode index passed to ScreenResolutionsModel::selectAt");

		auto selectedResolution = modes.at(index);

		auto currentResolution = getCurrentResolution();
		// Attempt to change the resolution
		if (!wzChangeFullscreenDisplayMode(selectedResolution.screen, selectedResolution.width, selectedResolution.height))
		{
			debug(
				LOG_WARNING,
				"Failed to change active resolution from: %s to: %s",
				ScreenResolutionsModel::resolutionString(currentResolution).c_str(),
				ScreenResolutionsModel::resolutionString(selectedResolution).c_str()
			);
		}

		// Store the new width and height
		war_SetFullscreenModeScreen(selectedResolution.screen);
		war_SetFullscreenModeWidth(selectedResolution.width);
		war_SetFullscreenModeHeight(selectedResolution.height);

		// Update the widget(s)
		refreshCurrentVideoOptionsValues();
	}

	static std::string currentResolutionString()
	{
		return ScreenResolutionsModel::resolutionString(getCurrentResolution());
	}

	static std::string resolutionString(const screeninfo &info)
	{
		return astringf("[%d] %d × %d", info.screen, info.width, info.height);
	}

private:
	const std::vector<screeninfo> modes;

	static screeninfo getCurrentWindowedResolution()
	{
		int screen = 0;
		unsigned int windowWidth = 0, windowHeight = 0;
		wzGetWindowResolution(&screen, &windowWidth, &windowHeight);
		screeninfo info;
		info.screen = screen;
		info.width = windowWidth;
		info.height = windowHeight;
		info.refresh_rate = -1;  // Unused.
		return info;
	}

	static screeninfo getCurrentResolution(optional<WINDOW_MODE> modeOverride = nullopt)
	{
		return (modeOverride.value_or(wzGetCurrentWindowMode()) == WINDOW_MODE::fullscreen) ? wzGetCurrentFullscreenDisplayMode() : getCurrentWindowedResolution();
	}

	static std::vector<screeninfo> loadModes()
	{
		// Get resolutions, sorted with duplicates removed.
		std::vector<screeninfo> modes = wzAvailableResolutions();
		std::sort(modes.begin(), modes.end(), ScreenResolutionsModel::compareLess);
		modes.erase(std::unique(modes.begin(), modes.end(), ScreenResolutionsModel::compareEq), modes.end());

		return modes;
	}

	static std::tuple<int, int, int> compareKey(const screeninfo &x)
	{
		return std::make_tuple(x.screen, x.width, x.height);
	}

	static bool compareLess(const screeninfo &a, const screeninfo &b)
	{
		return ScreenResolutionsModel::compareKey(a) < ScreenResolutionsModel::compareKey(b);
	}

	static bool compareEq(const screeninfo &a, const screeninfo &b)
	{
		return ScreenResolutionsModel::compareKey(a) == ScreenResolutionsModel::compareKey(b);
	}
};

class WindowModeDropdown : public DropdownWidget
{
public:
	static std::shared_ptr<WindowModeDropdown> make(UDWORD widgId = 0, int32_t paddingSize = 10)
	{
		auto dropdown = std::make_shared<WindowModeDropdown>();
		dropdown->id = widgId;

		dropdown->windowModeModel = WindowModeDropdown::getSupportedWindowModesModel();

		dropdown->setListHeight(FRONTEND_BUTHEIGHT * std::min<uint32_t>(5, dropdown->windowModeModel.size()));

		for (const auto& option : dropdown->windowModeModel)
		{
			auto item = makeTextButton(0, std::get<0>(option).toUtf8(), 0);
			dropdown->addItem(Margin(0, paddingSize).wrap(item));
		}

		dropdown->updateSelectedIndex();

		dropdown->setCanChange([](DropdownWidget &widget, size_t newIndex, std::shared_ptr<WIDGET> newSelectedWidget) -> bool {
			auto psDropdown = std::dynamic_pointer_cast<WindowModeDropdown>(widget.shared_from_this());
			ASSERT_OR_RETURN(false, psDropdown != nullptr, "Invalid widget");
			ASSERT_OR_RETURN(false, newIndex < psDropdown->windowModeModel.size(), "Invalid index");
			auto newMode = std::get<1>(psDropdown->windowModeModel.at(newIndex));
			if (newMode == wzGetCurrentWindowMode())
			{
				return true;
			}
			bool success = wzChangeWindowMode(newMode);
			if (success)
			{
				war_setWindowMode(newMode);
				// Update the widget(s)
				refreshCurrentVideoOptionsValues();
			}
			else
			{
				// unable to change to this fullscreen mode, so disable the widget
				debug(LOG_ERROR, "Failed to set fullscreen mode: %s", to_display_string(newMode).c_str());
				auto pTextButtonWrapper = std::dynamic_pointer_cast<MarginWidget>(newSelectedWidget);
				if (pTextButtonWrapper && !pTextButtonWrapper->children().empty())
				{
					auto pTextButton = std::dynamic_pointer_cast<W_BUTTON>(pTextButtonWrapper->children().front());
					if (pTextButton)
					{
						pTextButton->setState(WBUT_DISABLE);
					}
				}
			}
			return success;
		});

		return dropdown;
	};

	void updateSelectedIndex()
	{
		size_t currentSettingIdx = 0;
		auto currValue = war_getWindowMode();
		auto it = std::find_if(windowModeModel.begin(), windowModeModel.end(), [currValue](const std::tuple<WzString, WINDOW_MODE>& item) -> bool {
			return std::get<1>(item) == currValue;
		});
		if (it != windowModeModel.end())
		{
			currentSettingIdx = it - windowModeModel.begin();
			setSelectedIndex(currentSettingIdx);
		}
	}

private:
	static std::vector<std::tuple<WzString, WINDOW_MODE>> getSupportedWindowModesModel()
	{
		std::vector<std::tuple<WzString, WINDOW_MODE>> dropDownChoices = {
			{_("Windowed"), WINDOW_MODE::windowed},
			{_("Desktop Full"), WINDOW_MODE::desktop_fullscreen},
			{_("Fullscreen"), WINDOW_MODE::fullscreen},
		};

		dropDownChoices.erase(std::remove_if(dropDownChoices.begin(), dropDownChoices.end(), [](const std::tuple<WzString, WINDOW_MODE>& item) -> bool {
			return !wzIsSupportedWindowMode(std::get<1>(item));
		}), dropDownChoices.end());

		return dropDownChoices;
	}

private:
	std::vector<std::tuple<WzString, WINDOW_MODE>> windowModeModel;
};

class ResolutionDropdown : public DropdownWidget
{
public:
	static std::shared_ptr<ResolutionDropdown> make(UDWORD widgId = 0, int32_t paddingSize = 10)
	{
		auto dropdown = std::make_shared<ResolutionDropdown>();
		dropdown->id = widgId;
		dropdown->setListHeight(FRONTEND_BUTHEIGHT * 5);

		ScreenResolutionsModel screenResolutionsModel;
		for (auto resolution: screenResolutionsModel)
		{
			auto item = makeTextButton(0, ScreenResolutionsModel::resolutionString(resolution), 0);
			dropdown->addItem(Margin(0, paddingSize).wrap(item));
		}

		auto closestResolution = screenResolutionsModel.findResolutionClosestToCurrent();
		if (closestResolution != screenResolutionsModel.end())
		{
			dropdown->setSelectedIndex(closestResolution - screenResolutionsModel.begin());
		}

		dropdown->setOnChange([screenResolutionsModel](DropdownWidget& dropdown) {
			auto pResolutionDropdown = std::dynamic_pointer_cast<ResolutionDropdown>(dropdown.shared_from_this());
			if (!pResolutionDropdown)
			{
				return;
			}
			if (pResolutionDropdown->skipActualResolutionChange)
			{
				return;
			}
			if (auto selectedIndex = pResolutionDropdown->getSelectedIndex())
			{
				screenResolutionsModel.selectAt(selectedIndex.value());
			}
		});

		return dropdown;
	};

	void updateSelectedIndex()
	{
		auto closestResolution = screenResolutionsModel.findResolutionClosestToCurrent();
		if (closestResolution != screenResolutionsModel.end())
		{
			skipActualResolutionChange = true;
			setSelectedIndex(closestResolution - screenResolutionsModel.begin());
			skipActualResolutionChange = false;
		}
	}
private:
	ScreenResolutionsModel screenResolutionsModel;
	bool skipActualResolutionChange = false;
};

void refreshCurrentVideoOptionsValues()
{
	WIDGET *psWindowModeDropdownWidg = widgGetFromID(psWScreen, FRONTEND_WINDOWMODE_R);
	if (psWindowModeDropdownWidg)
	{
		auto windowModeWidg = std::dynamic_pointer_cast<WindowModeDropdown>(psWindowModeDropdownWidg->shared_from_this());
		if (windowModeWidg)
		{
			windowModeWidg->updateSelectedIndex();
		}
	}
	widgSetString(psWScreen, FRONTEND_FSAA_R, videoOptionsAntialiasingString().c_str());
	if (widgGetFromID(psWScreen, FRONTEND_RESOLUTION_READONLY)) // Resolution option may not be available
	{
		widgSetString(psWScreen, FRONTEND_RESOLUTION_READONLY, ScreenResolutionsModel::currentResolutionString().c_str());
		if (war_getWindowMode() != WINDOW_MODE::fullscreen)
		{
			// If live window resizing is supported & the current mode is "windowed", disable the Resolution option and add a tooltip
			// explaining the user can now resize the window normally.
			videoOptionsDisableResolutionButtons();
		}
		else
		{
			videoOptionsEnableResolutionButtons();
			WIDGET *psDropdownWidg = widgGetFromID(psWScreen, FRONTEND_RESOLUTION_DROPDOWN);
			if (psDropdownWidg)
			{
				auto pResolutionDropdown = std::dynamic_pointer_cast<ResolutionDropdown>(psDropdownWidg->shared_from_this());
				if (pResolutionDropdown)
				{
					pResolutionDropdown->updateSelectedIndex();
				}
			}
		}
	}
	widgSetString(psWScreen, FRONTEND_TEXTURESZ_R, videoOptionsTextureSizeString().c_str());
	widgSetString(psWScreen, FRONTEND_VSYNC_R, videoOptionsVsyncString());
	if (widgGetFromID(psWScreen, FRONTEND_DISPLAYSCALE_R)) // Display Scale option may not be available
	{
		widgSetString(psWScreen, FRONTEND_DISPLAYSCALE_R, videoOptionsDisplayScaleString().c_str());
	}
}

static std::shared_ptr<WIDGET> makeResolutionDropdown()
{
	const auto paddingSize = 10;
	auto dropdown = ResolutionDropdown::make(FRONTEND_RESOLUTION_DROPDOWN);
	return Margin(0, -paddingSize).wrap(dropdown);
}

static std::shared_ptr<WIDGET> makeMinimizeOnFocusLossDropdown()
{
	std::vector<std::tuple<WzString, MinimizeOnFocusLossBehavior>> dropDownChoices = {
		{_("Auto"), MinimizeOnFocusLossBehavior::Auto},
		{_("Off"), MinimizeOnFocusLossBehavior::Off},
		{_("On (Fullscreen)"), MinimizeOnFocusLossBehavior::On_Fullscreen},
	};

	size_t currentSettingIdx = 0;
	auto currValue = wzGetCurrentMinimizeOnFocusLossBehavior();
	auto it = std::find_if(dropDownChoices.begin(), dropDownChoices.end(), [currValue](const std::tuple<WzString, MinimizeOnFocusLossBehavior>& item) -> bool {
		return std::get<1>(item) == currValue;
	});
	if (it != dropDownChoices.end())
	{
		currentSettingIdx = it - dropDownChoices.begin();
	}

	auto dropdown = std::make_shared<DropdownWidget>();
	dropdown->id = FRONTEND_MINIMIZE_ON_FOCUS_LOSS_DROPDOWN;
	dropdown->setListHeight(FRONTEND_BUTHEIGHT * std::min<uint32_t>(5, dropDownChoices.size()));
	const int paddingSize = 10;

	for (const auto& option : dropDownChoices)
	{
		auto item = makeTextButton(0, std::get<0>(option).toUtf8(), 0);
		dropdown->addItem(Margin(0, paddingSize).wrap(item));
	}

	dropdown->setSelectedIndex(currentSettingIdx);

	dropdown->setOnChange([dropDownChoices](DropdownWidget& dropdown) {
		if (auto selectedIndex = dropdown.getSelectedIndex())
		{
			ASSERT_OR_RETURN(, selectedIndex.value() < dropDownChoices.size(), "Invalid index");
			auto behavior = std::get<1>(dropDownChoices.at(selectedIndex.value()));
			wzSetMinimizeOnFocusLoss(behavior);
			war_setMinimizeOnFocusLoss(static_cast<int>(behavior));
		}
	});

	return Margin(0, -paddingSize).wrap(dropdown);
}

static std::shared_ptr<WIDGET> makeFullscreenToggleModeDropdown()
{
	std::vector<std::tuple<WzString, WINDOW_MODE>> dropDownChoices = {
		{_("Desktop Full"), WINDOW_MODE::desktop_fullscreen},
		{_("Fullscreen"), WINDOW_MODE::fullscreen},
	};

	dropDownChoices.erase(std::remove_if(dropDownChoices.begin(), dropDownChoices.end(), [](const std::tuple<WzString, WINDOW_MODE>& item) -> bool {
		return !wzIsSupportedWindowMode(std::get<1>(item));
	}), dropDownChoices.end());

	if (dropDownChoices.size() <= 1)
	{
		// Don't bother making this dropdown if there is only 1 (or zero) fullscreen modes available
		return nullptr;
	}

	size_t currentSettingIdx = 0;
	auto currValue = wzGetToggleFullscreenMode();
	auto it = std::find_if(dropDownChoices.begin(), dropDownChoices.end(), [currValue](const std::tuple<WzString, WINDOW_MODE>& item) -> bool {
		return std::get<1>(item) == currValue;
	});
	if (it != dropDownChoices.end())
	{
		currentSettingIdx = it - dropDownChoices.begin();
	}

	auto dropdown = std::make_shared<DropdownWidget>();
	dropdown->id = FRONTEND_MINIMIZE_ON_FOCUS_LOSS_DROPDOWN;
	dropdown->setListHeight(FRONTEND_BUTHEIGHT * std::min<uint32_t>(5, dropDownChoices.size()));
	const int paddingSize = 10;

	for (const auto& option : dropDownChoices)
	{
		auto item = makeTextButton(0, std::get<0>(option).toUtf8(), 0);
		dropdown->addItem(Margin(0, paddingSize).wrap(item));
	}

	dropdown->setSelectedIndex(currentSettingIdx);

	dropdown->setCanChange([dropDownChoices](DropdownWidget &widget, size_t newIndex, std::shared_ptr<WIDGET> newSelectedWidget) -> bool {
		ASSERT_OR_RETURN(false, newIndex < dropDownChoices.size(), "Invalid index");
		auto toggleFullscreenMode = std::get<1>(dropDownChoices.at(newIndex));
		bool success = wzSetToggleFullscreenMode(toggleFullscreenMode);
		if (success)
		{
			war_setToggleFullscreenMode(static_cast<int>(toggleFullscreenMode));
		}
		else
		{
			debug(LOG_ERROR, "Failed to set toggle fullscreen mode");
		}
		return success;
	});

	return Margin(0, -paddingSize).wrap(dropdown);
}

static std::shared_ptr<WIDGET> makeWindowModeDropdown()
{
	const int paddingSize = 10;
	auto dropdown = WindowModeDropdown::make(FRONTEND_WINDOWMODE_R, paddingSize);
	return Margin(0, -paddingSize).wrap(dropdown);
}

void startVideoOptionsMenu()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	// Add a note about changes taking effect on restart for certain options
	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BOTFORM);

	auto label = std::make_shared<W_LABEL>();
	parent->attach(label);
	label->setGeometry(FRONTEND_POS1X + 48, FRONTEND_POS1Y, FRONTEND_BUTWIDTH - FRONTEND_POS1X - 48, FRONTEND_BUTHEIGHT);
	label->setFontColour(WZCOL_TEXT_BRIGHT);
	label->setString(_("* Takes effect on game restart"));
	label->setTextAlignment(WLAB_ALIGNBOTTOMLEFT);

	auto grid = std::make_shared<GridLayout>();
	grid_allocation::slot row(0);

	// Fullscreen/windowed
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_WINDOWMODE, videoOptionsWindowModeLabel(), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeWindowModeDropdown()));
	row.start++;

	// Resolution
	auto resolutionReadonlyLabel = makeTextButton(FRONTEND_RESOLUTION_READONLY_LABEL, videoOptionsResolutionLabel(), WBUT_SECONDARY | WBUT_DISABLE);
	resolutionReadonlyLabel->setTip(videoOptionsResolutionGetReadOnlyTooltip());
	auto resolutionReadonlyLabelContainer = addMargin(resolutionReadonlyLabel);
	resolutionReadonlyLabelContainer->id = FRONTEND_RESOLUTION_READONLY_LABEL_CONTAINER;

	auto resolutionReadonlyValue = makeTextButton(FRONTEND_RESOLUTION_READONLY, ScreenResolutionsModel::currentResolutionString(), WBUT_SECONDARY | WBUT_DISABLE);
	resolutionReadonlyValue->setTip(videoOptionsResolutionGetReadOnlyTooltip());
	auto resolutionReadonlyValueContainer = addMargin(resolutionReadonlyValue);
	resolutionReadonlyValueContainer->id = FRONTEND_RESOLUTION_READONLY_CONTAINER;

	grid->place({0}, row, resolutionReadonlyLabelContainer);
	grid->place({1, 1, false}, row, resolutionReadonlyValueContainer);

	auto resolutionDropdownLabel = makeTextButton(FRONTEND_RESOLUTION_DROPDOWN_LABEL, videoOptionsResolutionLabel(), WBUT_SECONDARY);
	auto resolutionDropdownLabelContainer = addMargin(resolutionDropdownLabel);
	resolutionDropdownLabelContainer->id = FRONTEND_RESOLUTION_DROPDOWN_LABEL_CONTAINER;
	grid->place({0}, row, resolutionDropdownLabelContainer);
	grid->place({1, 1, false}, row, addMargin(makeResolutionDropdown()));
	row.start++;

	// Texture size
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_TEXTURESZ, _("Texture size"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_TEXTURESZ_R, videoOptionsTextureSizeString(), WBUT_SECONDARY)));
	row.start++;

	// Vsync
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_VSYNC, _("Vertical sync"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_VSYNC_R, videoOptionsVsyncString(), WBUT_SECONDARY)));
	row.start++;

	// Antialiasing
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_FSAA, _("Antialiasing*"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_FSAA_R, videoOptionsAntialiasingString(), WBUT_SECONDARY)));
	row.start++;

	auto antialiasing_label = std::make_shared<W_LABEL>();
	parent->attach(antialiasing_label);
	antialiasing_label->setGeometry(FRONTEND_POS1X + 48, FRONTEND_POS1Y - 18, FRONTEND_BUTWIDTH - FRONTEND_POS1X - 48, FRONTEND_BUTHEIGHT);
	antialiasing_label->setFontColour(WZCOL_YELLOW);
	antialiasing_label->setString(_("Warning: Antialiasing can cause crashes, especially with values > 16"));
	antialiasing_label->setTextAlignment(WLAB_ALIGNBOTTOMLEFT);

	// Display Scale
	const bool showDisplayScale = wzAvailableDisplayScales().size() > 1;
	if (showDisplayScale)
	{
		grid->place({0}, row, addMargin(makeTextButton(FRONTEND_DISPLAYSCALE, videoOptionsDisplayScaleLabel(), WBUT_SECONDARY)));
		grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_DISPLAYSCALE_R, videoOptionsDisplayScaleString(), WBUT_SECONDARY)));
		row.start++;
	}

	// Gfx Backend
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_GFXBACKEND, _("Graphics Backend*"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_GFXBACKEND_R, videoOptionsGfxBackendString(), WBUT_SECONDARY)));
	row.start++;

#if !defined(__EMSCRIPTEN__)
	// Minimize on Focus Loss
	// TRANSLATORS: Shortened form of "Minimize on Focus Loss"
	// An option describing when / whether WZ will auto-minimize the window when it loses focus.
	auto minimizeOnFocusLossLabel = makeTextButton(FRONTEND_MINIMIZE_ON_FOCUS_LOSS, _("Min on Focus Loss"), WBUT_SECONDARY);
	minimizeOnFocusLossLabel->setTip(_("Whether the window should auto-minimize on focus loss"));
	grid->place({0}, row, addMargin(minimizeOnFocusLossLabel));
	grid->place({1, 1, false}, row, addMargin(makeMinimizeOnFocusLossDropdown()));
	row.start++;
#endif

	auto fullscreenToggleModeDropdown = makeFullscreenToggleModeDropdown();
	if (fullscreenToggleModeDropdown)
	{
		// TRANSLATORS: The fullscreen mode used when toggling with keys: Alt + Enter
		auto altEnterToggleLabel = makeTextButton(FRONTEND_ALTENTER_TOGGLE_MODE, _("Alt+Enter Toggle"), WBUT_SECONDARY);
		altEnterToggleLabel->setTip(_("The fullscreen mode used when toggling with keys: Alt + Enter"));
		grid->place({0}, row, addMargin(altEnterToggleLabel));
		grid->place({1, 1, false}, row, addMargin(fullscreenToggleModeDropdown));
		row.start++;
	}

	grid->setGeometry(0, 0, FRONTEND_BUTWIDTH, grid->idealHeight());

	auto scrollableList = ScrollableListWidget::make();
	scrollableList->setGeometry(0, FRONTEND_POS2Y, FRONTEND_BOTFORMW - 1, FRONTEND_BOTFORMH - FRONTEND_POS2Y - 1);
	scrollableList->addItem(grid);
	parent->attach(scrollableList);

	// Add some text down the side of the form
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("VIDEO OPTIONS"));

	// Quit/return
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	refreshCurrentVideoOptionsValues();
}

std::vector<unsigned int> availableDisplayScalesSorted()
{
	std::vector<unsigned int> displayScales = wzAvailableDisplayScales();
	std::sort(displayScales.begin(), displayScales.end());
	return displayScales;
}

void seqVsyncMode()
{
	gfx_api::context::swap_interval_mode currentVsyncMode = getCurrentSwapMode();
	auto startingVsyncMode = currentVsyncMode;
	bool success = false;

	do
	{
		currentVsyncMode = static_cast<gfx_api::context::swap_interval_mode>(seqCycle(static_cast<std::underlying_type<gfx_api::context::swap_interval_mode>::type>(currentVsyncMode), static_cast<std::underlying_type<gfx_api::context::swap_interval_mode>::type>(gfx_api::context::min_swap_interval_mode), 1, static_cast<std::underlying_type<gfx_api::context::swap_interval_mode>::type>(gfx_api::context::max_swap_interval_mode)));

		success = gfx_api::context::get().setSwapInterval(currentVsyncMode);

	} while ((!success) && (currentVsyncMode != startingVsyncMode));

	if (currentVsyncMode == startingVsyncMode)
	{
		// Failed to change vsync mode - display messagebox
		wzDisplayDialog(Dialog_Warning, _("Unable to change Vertical Sync"), _("Warzone failed to change the Vertical Sync mode.\nYour system / drivers may not support other modes."));
	}
	else if (success)
	{
		// succeeded changing vsync mode
		saveCurrentSwapMode(currentVsyncMode);
	}
}

void seqDisplayScale()
{
	std::vector<unsigned int> displayScales = availableDisplayScalesSorted();
	assert(!displayScales.empty());

	// Get currently-configured display scale.
	unsigned int current_displayScale = war_GetDisplayScale();

	// Find current display scale in list.
	auto current = std::lower_bound(displayScales.begin(), displayScales.end(), current_displayScale);
	if (current == displayScales.end() || *current != current_displayScale)
	{
		--current;  // If current display scale doesn't exist, round down to next-highest one.
	}

	// Increment/decrement and loop if required.
	auto startingDisplayScale = current;
	bool successfulDisplayScaleChange = false;
	current = seqCycle(current, displayScales.begin(), 1, displayScales.end() - 1);

	while (current != startingDisplayScale)
	{
		// Attempt to change the display scale
		if (!wzChangeDisplayScale(*current))
		{
			debug(LOG_WARNING, "Failed to change display scale from: %d to: %d", current_displayScale, *current);

			// try the next display scale factor, and loop
			current = seqCycle(current, displayScales.begin(), 1, displayScales.end() - 1);
			continue;
		}
		else
		{
			successfulDisplayScaleChange = true;
			break;
		}
	}

	if (!successfulDisplayScaleChange)
		return;

	// Store the new display scale
	war_SetDisplayScale(*current);
}

bool runVideoOptionsMenu()
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_FSAA:
	case FRONTEND_FSAA_R:
		{
			war_setAntialiasing(pow2Cycle(war_getAntialiasing(), 0, pie_GetMaxAntialiasing()));
			widgSetString(psWScreen, FRONTEND_FSAA_R, videoOptionsAntialiasingString().c_str());
			break;
		}

	case FRONTEND_TEXTURESZ:
	case FRONTEND_TEXTURESZ_R:
		{
			int newTexSize = pow2Cycle(getTextureSize(), 128, 2048);

			// Set the new size
			setTextureSize(newTexSize);

			// Update the widget
			widgSetString(psWScreen, FRONTEND_TEXTURESZ_R, videoOptionsTextureSizeString().c_str());

			break;
		}

	case FRONTEND_VSYNC:
	case FRONTEND_VSYNC_R:
		seqVsyncMode();

		// Update the widget(s)
		refreshCurrentVideoOptionsValues();
		break;

	case FRONTEND_GFXBACKEND:
	case FRONTEND_GFXBACKEND_R:
		{
			const std::vector<video_backend> availableBackends = wzAvailableGfxBackends();
			if (availableBackends.size() >= 1)
			{
				auto current = std::find(availableBackends.begin(), availableBackends.end(), war_getGfxBackend());
				if (current == availableBackends.end())
				{
					current = availableBackends.begin();
				}
				current = seqCycle(current, availableBackends.begin(), 1, availableBackends.end() - 1);
				war_setGfxBackend(*current);
				widgSetString(psWScreen, FRONTEND_GFXBACKEND_R, videoOptionsGfxBackendString().c_str());
			}
			else
			{
				debug(LOG_ERROR, "There must be at least one valid backend");
			}
			break;
		}

	case FRONTEND_DISPLAYSCALE:
	case FRONTEND_DISPLAYSCALE_R:
		seqDisplayScale();

		// Update the widget(s)
		refreshCurrentVideoOptionsValues();
		break;

	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	default:
		break;
	}

	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);

	return true;
}

char const *mouseOptionsMflipString()
{
	return getInvertMouseStatus() ? _("On") : _("Off");
}

char const *mouseOptionsTrapString()
{
	return war_GetTrapCursor() ? _("On") : _("Off");
}

char const *mouseOptionsMbuttonsString()
{
	return getRightClickOrders() ? _("On") : _("Off");
}

char const *mouseOptionsMmrotateString()
{
	return getMiddleClickRotate() ? _("Middle Mouse") : _("Right Mouse");
}

char const *mouseOptionsCursorModeString()
{
	return war_GetColouredCursor() ? _("On") : _("Off");
}

static std::shared_ptr<WIDGET> makeCursorScaleDropdown()
{
	std::vector<std::tuple<WzString, unsigned int>> dropDownChoices = {
		{"100%", 100},
		{"125%", 125},
		{"150%", 150},
		{"200%", 200}
	};

	// If current value (from config) is not one of the presets in dropDownChoices, add a "Custom" entry
	size_t currentSettingIdx = 0;
	unsigned int currValue = war_getCursorScale();
	auto it = std::find_if(dropDownChoices.begin(), dropDownChoices.end(), [currValue](const std::tuple<WzString, int>& item) -> bool {
		return std::get<1>(item) == currValue;
	});
	if (it != dropDownChoices.end())
	{
		currentSettingIdx = it - dropDownChoices.begin();
	}
	else
	{
		dropDownChoices.push_back({WzString::fromUtf8(astringf("(%u%%)", currValue)), currValue});
		currentSettingIdx = dropDownChoices.size() - 1;
	}

	auto dropdown = std::make_shared<DropdownWidget>();
	dropdown->id = FRONTEND_CURSORSCALE_DROPDOWN;
	dropdown->setListHeight(FRONTEND_BUTHEIGHT * std::min<uint32_t>(5, dropDownChoices.size()));
	const auto paddingSize = 10;

	for (const auto& option : dropDownChoices)
	{
		auto item = makeTextButton(0, std::get<0>(option).toUtf8(), 0);
		dropdown->addItem(Margin(0, paddingSize).wrap(item));
	}

	dropdown->setSelectedIndex(currentSettingIdx);

	dropdown->setCanChange([dropDownChoices](DropdownWidget &widget, size_t newIndex, std::shared_ptr<WIDGET> newSelectedWidget) -> bool {
		ASSERT_OR_RETURN(false, newIndex < dropDownChoices.size(), "Invalid index");
		if (wzChangeCursorScale(std::get<1>(dropDownChoices.at(newIndex))))
		{
			return true;
		}
		return false;
	});

	return Margin(0, -paddingSize).wrap(dropdown);
}

// ////////////////////////////////////////////////////////////////////////////
// Mouse Options
void startMouseOptionsMenu()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BOTFORM);

	auto grid = std::make_shared<GridLayout>();
	grid_allocation::slot row(0);

	////////////
	// mouseflip
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_MFLIP, _("Reverse Rotation"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_MFLIP_R, mouseOptionsMflipString(), WBUT_SECONDARY)));
	row.start++;

	// Cursor trapping
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_TRAP, _("Trap Cursor"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_TRAP_R, mouseOptionsTrapString(), WBUT_SECONDARY)));
	row.start++;

	////////////
	// left-click orders
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_MBUTTONS, _("Switch Mouse Buttons"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_MBUTTONS_R, mouseOptionsMbuttonsString(), WBUT_SECONDARY)));
	row.start++;

	////////////
	// middle-click rotate
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_MMROTATE, _("Rotate Screen"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_MMROTATE_R, mouseOptionsMmrotateString(), WBUT_SECONDARY)));
	row.start++;

	// Hardware / software cursor toggle
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_CURSORMODE, _("Colored Cursors"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_CURSORMODE_R, mouseOptionsCursorModeString(), WBUT_SECONDARY)));
	row.start++;

	// Cursor Size / Scaling
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_CURSORSCALE, _("Cursor Size"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeCursorScaleDropdown()));
	row.start++;

	grid->setGeometry(0, 0, FRONTEND_BUTWIDTH, grid->idealHeight());

	auto scrollableList = ScrollableListWidget::make();
	scrollableList->setGeometry(0, FRONTEND_POS2Y, FRONTEND_BOTFORMW - 1, FRONTEND_BOTFORMH - FRONTEND_POS2Y - 1);
	scrollableList->addItem(grid);
	parent->attach(scrollableList);

	// Add some text down the side of the form
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("MOUSE OPTIONS"));

	// Quit/return
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
}

bool runMouseOptionsMenu()
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_MFLIP:
	case FRONTEND_MFLIP_R:
		setInvertMouseStatus(!getInvertMouseStatus());
		widgSetString(psWScreen, FRONTEND_MFLIP_R, mouseOptionsMflipString());
		break;
	case FRONTEND_TRAP:
	case FRONTEND_TRAP_R:
		war_SetTrapCursor(!war_GetTrapCursor());
		widgSetString(psWScreen, FRONTEND_TRAP_R, mouseOptionsTrapString());
		break;

	case FRONTEND_MBUTTONS:
	case FRONTEND_MBUTTONS_R:
		setRightClickOrders(!getRightClickOrders());
		widgSetString(psWScreen, FRONTEND_MBUTTONS_R, mouseOptionsMbuttonsString());
		break;

	case FRONTEND_MMROTATE:
	case FRONTEND_MMROTATE_R:
		setMiddleClickRotate(!getMiddleClickRotate());
		widgSetString(psWScreen, FRONTEND_MMROTATE_R, mouseOptionsMmrotateString());
		break;

	case FRONTEND_CURSORMODE:
	case FRONTEND_CURSORMODE_R:
		war_SetColouredCursor(!war_GetColouredCursor());
		widgSetString(psWScreen, FRONTEND_CURSORMODE_R, mouseOptionsCursorModeString());
		wzSetCursor(CURSOR_DEFAULT);
		break;

	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	default:
		break;
	}

	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);

	return true;
}

static char const *gameOptionsDifficultyString()
{
	switch (getDifficultyLevel())
	{
	case DL_SUPER_EASY: return _("Super Easy");
	case DL_EASY: return _("Easy");
	case DL_NORMAL: return _("Normal");
	case DL_HARD: return _("Hard");
	case DL_INSANE: return _("Insane");
	}
	return _("Unsupported");
}

static std::string gameOptionsCameraSpeedString()
{
	char cameraSpeed[20];
	if(getCameraAccel())
	{
		ssprintf(cameraSpeed, "%d", war_GetCameraSpeed());
	}
	else
	{
		ssprintf(cameraSpeed, "%d / 2", war_GetCameraSpeed());
	}
	return cameraSpeed;
}

// ////////////////////////////////////////////////////////////////////////////
// Game Options Menu

class LanguagesModel
{
public:
	typedef const std::vector<locale_info>::iterator iterator;
	typedef const std::vector<locale_info>::const_iterator const_iterator;

	LanguagesModel()
	{
		locales = getLocales();
	}

	const_iterator begin() const
	{
		return locales.begin();
	}

	const_iterator end() const
	{
		return locales.end();
	}

	bool selectAt(size_t index) const
	{
		ASSERT_OR_RETURN(false, index < locales.size(), "Invalid index: %zu", index);
		return setLanguage(locales[index].code);
	}

	size_t getSelectedIndex() const
	{
		auto currentCode = getLanguage();
		for (auto i = 0; i < locales.size(); i++)
		{
			if (strcmp(currentCode, locales[i].code) == 0)
			{
				return i;
			}
		}

		return 0;
	}

private:
	std::vector<locale_info> locales;
};

class ImageDropdownItem: public MarginWidget
{
protected:
	ImageDropdownItem(): MarginWidget({5, HorizontalPadding})
	{
		setTransparentToMouse(false);
	}

	void initialize(const AtlasImageDef *image, const char *text)
	{
		auto iconAlignment = Alignment::center();
		iconAlignment.resizing = Alignment::Resizing::Fit;

		auto icon = std::make_shared<ImageWidget>(image);
		icon->setTransparentToMouse(true);

		label = std::make_shared<W_LABEL>();
		label->setFont(font_large);
		label->setString(text);
		label->setFontColour(WZCOL_TEXT_MEDIUM);
		label->setTransparentToMouse(true);

		auto grid = std::make_shared<GridLayout>();
		grid->place({0, 1, false}, {0}, Resize::fixed(25, 25).wrap(iconAlignment.wrap(icon)));
		grid->place({1}, {0}, Margin(0, 0, 0, 5).wrap(label));
		grid->setTransparentToMouse(true);
		attach(grid);
	}

public:
	static std::shared_ptr<ImageDropdownItem> make(const AtlasImageDef *image, const char *text)
	{
		class make_shared_enabler: public ImageDropdownItem {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(image, text);
		return widget;
	}

public:
	void setDisabled()
	{
		disabled = true;
	}

	void display(int, int) override
	{
		if (disabled)
		{
			label->setFontColour(WZCOL_TEXT_DARK);
			return;
		}
		label->setFontColour(isMouseOverWidget() ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM);
	}

	static const int HorizontalPadding = 10;

private:
	bool disabled = false;
	std::shared_ptr<W_LABEL> label;
};

static std::shared_ptr<WIDGET> makeLanguageDropdown()
{
	const std::map<WzString, const char *> iconsMap = {
		{"", "icon-system.png"},
		{"ar_SA", "lang-AR.png"},
		{"bg", "flag-BG.png"},
		{"ca_ES", "flag-ES.png"},
		{"cs_CZ", "flag-CZ.png"},
		{"da", "flag-DK.png"},
		{"de", "flag-DE.png"},
		{"el_GR", "flag-GR.png"},
		{"en", "flag-US.png"},
		{"en_GB", "flag-GB.png"},
		{"es", "flag-ES.png"},
		{"et_EE", "flag-EE.png"},
		{"eu_ES", "flag-ES.png"},
		{"fi", "flag-FI.png"},
		{"fr", "flag-FR.png"},
		{"fy_NL", "flag-NL.png"},
		{"ga_IE", "flag-IE.png"},
		{"hr", "flag-HR.png"},
		{"hu", "flag-HU.png"},
		{"id", "flag-ID.png"},
		{"it", "flag-IT.png"},
		{"ko_KR", "flag-KR.png"},
		{"la", "flag-VA.png"},
		{"lt", "flag-LT.png"},
		{"lv", "flag-LV.png"},
		{"nb_NO", "flag-NO.png"},
		{"nn_NO", "flag-NO.png"},
		{"nl", "flag-NL.png"},
		{"pl", "flag-PL.png"},
		{"pt_BR", "flag-BR.png"},
		{"pt", "flag-PT.png"},
		{"ro", "flag-RO.png"},
		{"ru", "flag-RU.png"},
		{"sk", "flag-SK.png"},
		{"sl_SI", "flag-SI.png"},
		{"sv_SE", "flag-SE.png"},
		{"sv", "flag-SE.png"},
		{"tr", "flag-TR.png"},
		{"uz", "flag-UZ.png"},
		{"uk_UA", "flag-UA.png"},
		{"zh_CN", "flag-CN.png"},
		{"zh_TW", "flag-TW.png"},
	};

	if (pFlagsImages == nullptr)
	{
		pFlagsImages.reset(iV_LoadImageFile("images/flags.img"));
		if (pFlagsImages == nullptr)
		{
			std::string errorMessage = astringf(_("Unable to load: %s."), "flags.img");
			if (!getLoadedMods().empty())
			{
				errorMessage += " ";
				errorMessage += _("Please remove all incompatible mods.");
			}
			debug(LOG_FATAL, "%s", errorMessage.c_str());
		}
	}

	auto dropdown = std::make_shared<DropdownWidget>();
	dropdown->id = FRONTEND_LANGUAGE_R;
	dropdown->setListHeight(FRONTEND_BUTHEIGHT * 5);
	// NOTE: By capturing a copy of pFlagsImages in the onDelete handler, we ensure it stays around until the DropdownWidget is deleted
	std::shared_ptr<IMAGEFILE> flagsImagesCopy = pFlagsImages;
	dropdown->setOnDelete([flagsImagesCopy](WIDGET *psWidget) mutable {
		flagsImagesCopy.reset();
	});

	LanguagesModel model;
	for (auto locale: model)
	{
		AtlasImageDef *icon = nullptr;
		auto mapIcon = iconsMap.find(locale.code);
		if (mapIcon != iconsMap.end())
		{
			icon = (pFlagsImages) ? pFlagsImages->find(mapIcon->second) : nullptr;
		}
		auto option = ImageDropdownItem::make(icon, locale.name);
		dropdown->addItem(option);
	}

	dropdown->setSelectedIndex(model.getSelectedIndex());

	dropdown->setCanChange([model](DropdownWidget &widget, size_t newIndex, std::shared_ptr<WIDGET> newSelectedWidget) -> bool {
		bool success = model.selectAt(newIndex);
		if (!success)
		{
			// unable to change the locale to this entry, so disable the widget
			auto pImageDropdownItem = std::dynamic_pointer_cast<ImageDropdownItem>(newSelectedWidget);
			if (pImageDropdownItem)
			{
				pImageDropdownItem->setDisabled();
			}
		}
		return success;
	});

	dropdown->setOnChange([model](DropdownWidget& dropdown) {
		if (auto selectedIndex = dropdown.getSelectedIndex())
		{
			/* Hack to reset current menu text, which looks fancy. */
			widgSetString(psWScreen, FRONTEND_SIDETEXT, _("GAME OPTIONS"));
			widgGetFromID(psWScreen, FRONTEND_QUIT)->setTip(P_("menu", "Return"));
			widgSetString(psWScreen, FRONTEND_LANGUAGE, _("Language"));
			widgSetString(psWScreen, FRONTEND_COLOUR, _("Unit Colour:"));
			widgSetString(psWScreen, FRONTEND_COLOUR_CAM, _("Campaign"));
			widgSetString(psWScreen, FRONTEND_COLOUR_MP, _("Skirmish/Multiplayer"));
			widgSetString(psWScreen, FRONTEND_DIFFICULTY, _("Campaign Difficulty"));
			widgSetString(psWScreen, FRONTEND_CAMERASPEED, _("Camera Speed"));
			widgSetString(psWScreen, FRONTEND_DIFFICULTY_R, gameOptionsDifficultyString());

			// hack to update translations of AI names and tooltips
			readAIs();
		}
	});

	return Margin(0, -ImageDropdownItem::HorizontalPadding).wrap(dropdown);
}

class SettingsGridBuilder
{
public:
	SettingsGridBuilder(std::shared_ptr<GridLayout> grid): grid(grid), row(0, 1, true) {}

	void addRow(std::shared_ptr<WIDGET> leftWidget, std::shared_ptr<WIDGET> rightWidget = nullptr)
	{
		Alignment alignment(Alignment::Vertical::Top, Alignment::Horizontal::Left);
		grid->place({0}, row, addMargin(alignment.wrap(leftWidget)));
		if (rightWidget)
		{
			grid->place({1, 1, false}, row, addMargin(alignment.wrap(rightWidget)));
		}
		row.start++;
	}

private:
	std::shared_ptr<GridLayout> grid;
	grid_allocation::slot row;
};

void startGameOptionsMenu()
{
	UDWORD	w, h;
	int playercolor;

	addBackdrop();
	addTopForm(false);
	addBottomForm(true);

	auto grid = std::make_shared<GridLayout>();
	SettingsGridBuilder gridBuilder(grid);

#define MINIMUM_GAME_OPTIONS_BUTTON_WIDTH (280)
#define MINIMUM_GAME_OPTIONS_RIGHT_BUTTON_WIDTH (240)

	// language
	gridBuilder.addRow(
		makeTextButton(FRONTEND_LANGUAGE, _("Language"), WBUT_SECONDARY, MINIMUM_GAME_OPTIONS_BUTTON_WIDTH),
		makeLanguageDropdown()
	);

	// Difficulty
	gridBuilder.addRow(
		makeTextButton(FRONTEND_DIFFICULTY, _("Campaign Difficulty"), WBUT_SECONDARY, MINIMUM_GAME_OPTIONS_BUTTON_WIDTH),
		makeTextButton(FRONTEND_DIFFICULTY_R, gameOptionsDifficultyString(), WBUT_SECONDARY, MINIMUM_GAME_OPTIONS_RIGHT_BUTTON_WIDTH)
	);

	// Camera speed
	gridBuilder.addRow(
		makeTextButton(FRONTEND_CAMERASPEED, _("Camera Speed"), WBUT_SECONDARY, MINIMUM_GAME_OPTIONS_BUTTON_WIDTH),
		makeTextButton(FRONTEND_CAMERASPEED_R, gameOptionsCameraSpeedString(), WBUT_SECONDARY, MINIMUM_GAME_OPTIONS_RIGHT_BUTTON_WIDTH)
	);

	// Colour stuff
	gridBuilder.addRow(makeTextButton(FRONTEND_COLOUR, _("Unit Colour:"), 0, MINIMUM_GAME_OPTIONS_BUTTON_WIDTH));

	w = iV_GetImageWidth(FrontImages, IMAGE_PLAYERN);
	h = iV_GetImageHeight(FrontImages, IMAGE_PLAYERN);

	// FIXME: if playercolor = 1-3, then we Assert in widgSetButtonState() since we don't define FE_P1 - FE_P3
	// I assume the reason is that in SP games, those are reserved for the AI?  Valid values are 0, 4-7.
	// This is a workaround, until we find what is setting that to 1-3.  See configuration.c:701
	playercolor = war_GetSPcolor();
	if (playercolor >= 1 && playercolor <= 3)
	{
		playercolor = 0;
	}

	auto campaignColor = std::make_shared<GridLayout>();
	std::vector<int> availableCampaignColors = {0, 4, 5, 6, 7};
	Margin unitColorMargin(0, 5, 5, 0);
	for (uint32_t i = 0; i < availableCampaignColors.size(); i++)
	{
		auto colorId = availableCampaignColors[i];
		auto button = makeMultiBut(FE_P0 + colorId, w, h, nullptr, IMAGE_PLAYERN, IMAGE_PLAYERX, true, colorId);
		if (colorId == playercolor)
		{
			button->setState(WBUT_LOCK);
		}
		campaignColor->place({i}, {0}, unitColorMargin.wrap(button));
	}

	Margin colorsMargin(0, 0, 0, 20);
	gridBuilder.addRow(
		colorsMargin.wrap(
			Alignment(Alignment::Vertical::Top, Alignment::Horizontal::Left).wrap(
				makeTextButton(FRONTEND_COLOUR_CAM, _("Campaign"), 0, MINIMUM_GAME_OPTIONS_BUTTON_WIDTH - colorsMargin.left)
			)
		),
		campaignColor
	);

	auto multiplayerColor = std::make_shared<GridLayout>();
	for (int colour = -1; colour < MAX_PLAYERS_IN_GUI; ++colour)
	{
		uint32_t cellX = (colour + 1) % 7;
		uint32_t cellY = (colour + 1) / 7;
		auto button = makeMultiBut(FE_MP_PR + colour + 1, w, h, nullptr, IMAGE_PLAYERN, IMAGE_PLAYERX, true, colour >= 0 ? colour : MAX_PLAYERS + 1);
		if (colour == war_getMPcolour()) {
			button->setState(WBUT_LOCK);
		}
		multiplayerColor->place({cellX}, {cellY}, unitColorMargin.wrap(button));
	}

	gridBuilder.addRow(
		colorsMargin.wrap(
			Alignment(Alignment::Vertical::Top, Alignment::Horizontal::Left).wrap(
				makeTextButton(FRONTEND_COLOUR_MP, _("Skirmish/Multiplayer"), 0, MINIMUM_GAME_OPTIONS_BUTTON_WIDTH - colorsMargin.left)
			)
		),
		multiplayerColor
	);

	grid->setGeometry(0, 0, FRONTEND_BUTWIDTH_WIDE, grid->idealHeight());

	auto scrollableList = ScrollableListWidget::make();
	scrollableList->setGeometry(0, FRONTEND_POS2Y, FRONTEND_BOTFORM_WIDEW - 1, FRONTEND_BOTFORM_WIDEH - FRONTEND_POS2Y - 1);
	scrollableList->addItem(grid);
	widgGetFromID(psWScreen, FRONTEND_BOTFORM)->attach(scrollableList);

	// Quit
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	// "Help Us Translate" link
	const auto helpTranslateMessage = astringf(_("Help us improve translations of Warzone 2100: %s"), TRANSLATION_URL);
	addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS9X, FRONTEND_POS9Y, helpTranslateMessage.c_str(), 0);
	widgSetTip(psWScreen, FRONTEND_HYPERLINK, _("Click to open webpage."));

	// Add some text down the side of the form
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("GAME OPTIONS"));
}

bool runGameOptionsMenu()
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_HYPERLINK:
		openURLInBrowser(TRANSLATION_URL);
		break;

	case FRONTEND_DIFFICULTY:
	case FRONTEND_DIFFICULTY_R:
		setDifficultyLevel(seqCycle(getDifficultyLevel(), DL_SUPER_EASY, 1, DL_INSANE));
		widgSetString(psWScreen, FRONTEND_DIFFICULTY_R, gameOptionsDifficultyString());
		if (getDifficultyLevel() == DL_INSANE)
		{
			const std::string DIFF_TAG = "difficulty";

			if (!hasNotificationsWithTag(DIFF_TAG))
			{
				WZ_Notification notification;
				notification.duration = 10 * GAME_TICKS_PER_SEC;
				notification.contentTitle = _("Insane Difficulty");
				notification.contentText = _("This difficulty is for very experienced players!");
				notification.tag = DIFF_TAG;
				notification.largeIcon = WZ_Notification_Image("images/notifications/skull_crossbones.png");

				addNotification(notification, WZ_Notification_Trigger::Immediate());
			}
		}
		break;

	case FRONTEND_CAMERASPEED:
	case FRONTEND_CAMERASPEED_R:
		war_SetCameraSpeed(seqCycle(war_GetCameraSpeed(), CAMERASPEED_MIN, CAMERASPEED_STEP, CAMERASPEED_MAX));
		widgSetString(psWScreen, FRONTEND_CAMERASPEED_R, gameOptionsCameraSpeedString().c_str());
		break;

	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	case FE_P0:
		widgSetButtonState(psWScreen, FE_P0, WBUT_LOCK);
		widgSetButtonState(psWScreen, FE_P4, 0);
		widgSetButtonState(psWScreen, FE_P5, 0);
		widgSetButtonState(psWScreen, FE_P6, 0);
		widgSetButtonState(psWScreen, FE_P7, 0);
		war_SetSPcolor(0);
		break;
	case FE_P4:
		widgSetButtonState(psWScreen, FE_P0, 0);
		widgSetButtonState(psWScreen, FE_P4, WBUT_LOCK);
		widgSetButtonState(psWScreen, FE_P5, 0);
		widgSetButtonState(psWScreen, FE_P6, 0);
		widgSetButtonState(psWScreen, FE_P7, 0);
		war_SetSPcolor(4);
		break;
	case FE_P5:
		widgSetButtonState(psWScreen, FE_P0, 0);
		widgSetButtonState(psWScreen, FE_P4, 0);
		widgSetButtonState(psWScreen, FE_P5, WBUT_LOCK);
		widgSetButtonState(psWScreen, FE_P6, 0);
		widgSetButtonState(psWScreen, FE_P7, 0);
		war_SetSPcolor(5);
		break;
	case FE_P6:
		widgSetButtonState(psWScreen, FE_P0, 0);
		widgSetButtonState(psWScreen, FE_P4, 0);
		widgSetButtonState(psWScreen, FE_P5, 0);
		widgSetButtonState(psWScreen, FE_P6, WBUT_LOCK);
		widgSetButtonState(psWScreen, FE_P7, 0);
		war_SetSPcolor(6);
		break;
	case FE_P7:
		widgSetButtonState(psWScreen, FE_P0, 0);
		widgSetButtonState(psWScreen, FE_P4, 0);
		widgSetButtonState(psWScreen, FE_P5, 0);
		widgSetButtonState(psWScreen, FE_P6, 0);
		widgSetButtonState(psWScreen, FE_P7, WBUT_LOCK);
		war_SetSPcolor(7);
		break;

	default:
		break;
	}

	if (id >= FE_MP_PR && id <= FE_MP_PMAX)
	{
		int chosenColour = id - FE_MP_PR - 1;
		for (int colour = -1; colour < MAX_PLAYERS_IN_GUI; ++colour)
		{
			unsigned thisID = FE_MP_PR + colour + 1;
			widgSetButtonState(psWScreen, thisID, id == thisID ? WBUT_LOCK : 0);
		}
		war_setMPcolour(chosenColour);
	}

	// If close button pressed then return from this menu.
	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);						// show the widgets currently running

	return true;
}

static std::shared_ptr<WIDGET> makeInactivityMinutesMPDropdown()
{
	auto dropdown = std::make_shared<DropdownWidget>();
	dropdown->id = FRONTEND_INACTIVITY_TIMEOUT_DROPDOWN;
	dropdown->setListHeight(FRONTEND_BUTHEIGHT * 5);
	const auto paddingSize = 10;

	std::vector<uint32_t> inactivityMinutesValues;
	dropdown->addItem(Margin(0, paddingSize).wrap(makeTextButton(0, _("Off"), 0)));
	inactivityMinutesValues.push_back(0);

	auto addInactivityMinutesRow = [&](uint32_t inactivityMinutes) {
		auto item = makeTextButton(0, astringf(_("%u minutes"), inactivityMinutes), 0);
		dropdown->addItem(Margin(0, paddingSize).wrap(item));
		inactivityMinutesValues.push_back(inactivityMinutes);
	};

	for (uint32_t inactivityMinutes = MIN_MPINACTIVITY_MINUTES; inactivityMinutes <= (MIN_MPINACTIVITY_MINUTES + 6); inactivityMinutes++)
	{
		addInactivityMinutesRow(inactivityMinutes);
	}

	if (!std::any_of(inactivityMinutesValues.begin(), inactivityMinutesValues.end(), [](uint32_t inactivityMinutes) {
		return inactivityMinutes == war_getMPInactivityMinutes();
	}))
	{
		// add the current value, which must be a custom manual config edit
		addInactivityMinutesRow(war_getMPInactivityMinutes());
	}

	auto it = std::find(inactivityMinutesValues.begin(), inactivityMinutesValues.end(), war_getMPInactivityMinutes());
	if (it != inactivityMinutesValues.end())
	{
		dropdown->setSelectedIndex(it - inactivityMinutesValues.begin());
	}

	dropdown->setOnChange([inactivityMinutesValues](DropdownWidget& dropdown) {
		if (auto selectedIndex = dropdown.getSelectedIndex())
		{
			ASSERT_OR_RETURN(, selectedIndex.value() < inactivityMinutesValues.size(), "Invalid selected index: %zu", selectedIndex.value());
			uint32_t newInactivityMinutes = inactivityMinutesValues[selectedIndex.value()];
			war_setMPInactivityMinutes(newInactivityMinutes);
			game.inactivityMinutes = war_getMPInactivityMinutes();
		}
	});

	return Margin(0, -paddingSize).wrap(dropdown);
}

static std::shared_ptr<WIDGET> makeLagKickDropdown()
{
	auto dropdown = std::make_shared<DropdownWidget>();
	dropdown->id = FRONTEND_LAG_KICK_DROPDOWN;
	const auto paddingSize = 10;

	std::vector<int> lagKickSecondsValues;
	dropdown->addItem(Margin(0, paddingSize).wrap(makeTextButton(0, _("Off"), 0)));
	lagKickSecondsValues.push_back(0);

	auto addLagKickSecondsRow = [&](int lagKickSeconds, const std::string& label) {
		auto item = makeTextButton(0, label, 0);
		dropdown->addItem(Margin(0, paddingSize).wrap(item));
		lagKickSecondsValues.push_back(lagKickSeconds);
	};

	for (int lagKickSeconds = 60; lagKickSeconds <= 120; lagKickSeconds += 30)
	{
		addLagKickSecondsRow(lagKickSeconds, astringf(_("%u seconds"), (unsigned)lagKickSeconds));
	}

	if (!std::any_of(lagKickSecondsValues.begin(), lagKickSecondsValues.end(), [](int lagKickSeconds) {
		return lagKickSeconds == war_getAutoLagKickSeconds();
	}))
	{
		// add the current value, which must be a custom manual config edit
		addLagKickSecondsRow(war_getAutoLagKickSeconds(), astringf(_("%u seconds"), (unsigned)war_getAutoLagKickSeconds()));
	}

	dropdown->setListHeight(FRONTEND_BUTHEIGHT * std::min<int>(5, lagKickSecondsValues.size()));

	auto it = std::find(lagKickSecondsValues.begin(), lagKickSecondsValues.end(), war_getAutoLagKickSeconds());
	if (it != lagKickSecondsValues.end())
	{
		dropdown->setSelectedIndex(it - lagKickSecondsValues.begin());
	}

	dropdown->setOnChange([lagKickSecondsValues](DropdownWidget& dropdown) {
		if (auto selectedIndex = dropdown.getSelectedIndex())
		{
			ASSERT_OR_RETURN(, selectedIndex.value() < lagKickSecondsValues.size(), "Invalid selected index: %zu", selectedIndex.value());
			int newAutoLagKickSeconds = lagKickSecondsValues[selectedIndex.value()];
			war_setAutoLagKickSeconds(newAutoLagKickSeconds);
		}
	});

	return Margin(0, -paddingSize).wrap(dropdown);
}

static std::shared_ptr<WIDGET> makeOpenSpectatorSlotsMPDropdown()
{
	auto dropdown = std::make_shared<DropdownWidget>();
	dropdown->id = FRONTEND_SPECTATOR_SLOTS_DROPDOWN;
	dropdown->setListHeight(FRONTEND_BUTHEIGHT * 5);
	const auto paddingSize = 10;

	std::vector<uint16_t> openSpectatorSlotsValues;
	dropdown->addItem(Margin(0, paddingSize).wrap(makeTextButton(0, _("None"), 0)));
	openSpectatorSlotsValues.push_back(0);

	auto addOpenSpectatorSlotsRow = [&](uint16_t openSpectatorSlots) {
		auto item = makeTextButton(0, astringf("%u", openSpectatorSlots), 0);
		dropdown->addItem(Margin(0, paddingSize).wrap(item));
		openSpectatorSlotsValues.push_back(openSpectatorSlots);
	};

	for (uint32_t openSpectatorSlots = 1; openSpectatorSlots <= MAX_SPECTATOR_SLOTS; openSpectatorSlots++)
	{
		addOpenSpectatorSlotsRow(openSpectatorSlots);
	}

	auto it = std::find(openSpectatorSlotsValues.begin(), openSpectatorSlotsValues.end(), war_getMPopenSpectatorSlots());
	if (it != openSpectatorSlotsValues.end())
	{
		dropdown->setSelectedIndex(it - openSpectatorSlotsValues.begin());
	}

	dropdown->setOnChange([openSpectatorSlotsValues](DropdownWidget& dropdown) {
		if (auto selectedIndex = dropdown.getSelectedIndex())
		{
			ASSERT_OR_RETURN(, selectedIndex.value() < openSpectatorSlotsValues.size(), "Invalid selected index: %zu", selectedIndex.value());
			uint16_t newOpenSpectatorSlots = openSpectatorSlotsValues[selectedIndex.value()];
			war_setMPopenSpectatorSlots(newOpenSpectatorSlots);
		}
	});

	return Margin(0, -paddingSize).wrap(dropdown);
}

char const *multiplayOptionsUPnPString()
{
	return NetPlay.isUPNP ? _("On") : _("Off");
}

// ////////////////////////////////////////////////////////////////////////////
// Multiplay Options Menu
void startMultiplayOptionsMenu()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BOTFORM);

	auto grid = std::make_shared<GridLayout>();
	grid_allocation::slot row(0);

	// "Hosting Options" title
	grid->place({0, 2}, row, addMargin(makeTextButton(FRONTEND_FX, _("Hosting Options:"), WBUT_DISABLE)));
	row.start++;

	// Game Port
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_GAME_PORT, _("Game Port"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_GAME_PORT_R, std::to_string(NETgetGameserverPort()), WBUT_DISABLE))); // FUTURE TODO: Make this an input field or similar and allow editing (although reject ports <= 1024)
	row.start++;

	// Enable UPnP
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_UPNP, _("Enable UPnP"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_UPNP_R, multiplayOptionsUPnPString(), WBUT_SECONDARY)));
	row.start++;

	// Inactivity Kick
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_INACTIVITY_TIMEOUT, _("Inactivity Timeout"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeInactivityMinutesMPDropdown()));
	row.start++;

	// Lag Kick
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_LAG_KICK, _("Lag Kick"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeLagKickDropdown()));
	row.start++;

	// Spectator Slots
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_SPECTATOR_SLOTS, _("Spectator Slots"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeOpenSpectatorSlotsMPDropdown()));
	row.start++;

	// Enable Autorating lookup
	grid->place({0}, row, addMargin(makeTextButton(FRONTEND_AUTORATING, _("Enable Rating"), WBUT_SECONDARY)));
	grid->place({1, 1, false}, row, addMargin(makeTextButton(FRONTEND_AUTORATING_R, getAutoratingEnable()? _("On") : _("Off"), WBUT_SECONDARY)));
	row.start++;

	grid->setGeometry(0, 0, FRONTEND_BUTWIDTH, grid->idealHeight());

	auto scrollableList = ScrollableListWidget::make();
	scrollableList->setGeometry(0, FRONTEND_POS2Y, FRONTEND_BOTFORMW - 1, FRONTEND_BOTFORMH - FRONTEND_POS2Y - 1);
	scrollableList->addItem(grid);
	parent->attach(scrollableList);

	// quit.
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	//add some text down the side of the form
	WzString messageString = WzString::fromUtf8(_("MULTIPLAY OPTIONS"));
	std::vector<WzString> messageStringLines = messageString.split("\n");
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, messageStringLines[0].toUtf8().c_str());
	// show a second sidetext line if the translation requires it
	if (messageStringLines.size() > 1)
	{
		messageString.remove(0, messageStringLines[0].length() + 1);
		addSideText(FRONTEND_MULTILINE_SIDETEXT, FRONTEND_SIDEX + 22, \
		FRONTEND_SIDEY, messageString.toUtf8().c_str());
	}
}

bool runMultiplayOptionsMenu()
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_UPNP:
	case FRONTEND_UPNP_R:
		NetPlay.isUPNP = !NetPlay.isUPNP;
		widgSetString(psWScreen, FRONTEND_UPNP_R, multiplayOptionsUPnPString());
		break;
	case FRONTEND_AUTORATING:
	case FRONTEND_AUTORATING_R:
		setAutoratingEnable(!getAutoratingEnable());
		widgSetString(psWScreen, FRONTEND_AUTORATING_R, getAutoratingEnable()? _("On") : _("Off"));
		break;

	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	default:
		break;
	}

	// If close button pressed then return from this menu.
	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);						// show the widgets currently running

	return true;
}

struct TitleBitmapCache {
	WzText formattedVersionString;
	WzText modListText;
	WzText gfxBackend;
};

// ////////////////////////////////////////////////////////////////////////////
// drawing functions

// show a background picture (currently used for version and mods labels)
static void displayTitleBitmap(WZ_DECL_UNUSED WIDGET *psWidget, WZ_DECL_UNUSED UDWORD xOffset, WZ_DECL_UNUSED UDWORD yOffset)
{
	char modListText[MAX_STR_LENGTH] = "";

	if (!getModList().empty())
	{
		sstrcat(modListText, _("Mod: "));
		sstrcat(modListText, getModList().c_str());
	}

	if (pie_GetVideoBufferWidth() <= 0 || pie_GetVideoBufferHeight() <= 0)
	{
		// don't bother drawing when the dimensions are <= 0,0
		return;
	}

	assert(psWidget->pUserData != nullptr);
	TitleBitmapCache& cache = *static_cast<TitleBitmapCache*>(psWidget->pUserData);

	cache.formattedVersionString.setText(version_getFormattedVersionString(), font_regular);
	cache.modListText.setText(modListText, font_regular);
	cache.gfxBackend.setText(WzString::fromUtf8(gfx_api::context::get().getFormattedRendererInfoString()), font_small);

	cache.formattedVersionString.render(pie_GetVideoBufferWidth() - 9, pie_GetVideoBufferHeight() - 14, WZCOL_GREY, 270.f);

	if (!getModList().empty())
	{
		cache.modListText.render(9, 14, WZCOL_GREY);
	}

	cache.gfxBackend.render(9, pie_GetVideoBufferHeight() - 10, WZCOL_GREY);

	cache.formattedVersionString.render(pie_GetVideoBufferWidth() - 10, pie_GetVideoBufferHeight() - 15, WZCOL_TEXT_BRIGHT, 270.f);

	if (!getModList().empty())
	{
		cache.modListText.render(10, 15, WZCOL_TEXT_BRIGHT);
	}

	cache.gfxBackend.render(10, pie_GetVideoBufferHeight() - 11, WZCOL_TEXT_BRIGHT);
}

// ////////////////////////////////////////////////////////////////////////////
// show warzone logo
static void displayLogo(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	iV_DrawImage2(WzString::fromUtf8("image_fe_logo.png"), xOffset + psWidget->x(), yOffset + psWidget->y(), psWidget->width(), psWidget->height());
}


// ////////////////////////////////////////////////////////////////////////////
// show, well have a guess..
static void displayBigSlider(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	W_SLIDER *Slider = (W_SLIDER *)psWidget;
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();

	iV_DrawImage(IntImages, IMAGE_SLIDER_BIG, x + STAT_SLD_OX, y + STAT_SLD_OY);			// draw bdrop

	int sx = (Slider->width() - 3 - Slider->barSize) * Slider->pos / std::max<int>(Slider->numStops, 1);  // determine pos.
	iV_DrawImage(IntImages, IMAGE_SLIDER_BIGBUT, x + 3 + sx, y + 3);								//draw amount
}

// ////////////////////////////////////////////////////////////////////////////
// show text written on its side.
static void displayTextAt270(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	SDWORD		fx, fy;
	W_LABEL		*psLab;

	psLab = (W_LABEL *)psWidget;

	// Any widget using displayTextAt270 must have its pUserData initialized to a (DisplayTextOptionCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayTextOptionCache& cache = *static_cast<DisplayTextOptionCache*>(psWidget->pUserData);

	// TODO: Only works for single-line (not "formatted text") labels
	cache.wzText.setText(psLab->getString(), font_large);

	fx = xOffset + psWidget->x();
	fy = yOffset + psWidget->y() + cache.wzText.width();

	cache.wzText.render(fx + 2, fy + 2, WZCOL_GREY, 270.f);
	cache.wzText.render(fx, fy, WZCOL_TEXT_BRIGHT, 270.f);
}

// ////////////////////////////////////////////////////////////////////////////
// show a text option.
void displayTextOption(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	SDWORD			fx, fy, fw;
	W_BUTTON		*psBut = (W_BUTTON *)psWidget;
	bool			hilight = false;
	bool			greyOut = psWidget->UserData || (psBut->getState() & WBUT_DISABLE); // if option is unavailable.

	// Any widget using displayTextOption must have its pUserData initialized to a (DisplayTextOptionCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayTextOptionCache& cache = *static_cast<DisplayTextOptionCache*>(psWidget->pUserData);

	iV_fonts fontID = cache.wzText.getFontID();
	if (fontID == font_count || cache.lastWidgetWidth != psBut->width() || cache.wzText.getText() != psBut->pText)
	{
		fontID = psBut->FontID;
	}
	cache.wzText.setText(psBut->pText, fontID);

	if (cache.wzText.width() > psBut->width())
	{
		// text would exceed the bounds of the button area
		switch (cache.overflowBehavior)
		{
			case DisplayTextOptionCache::OverflowBehavior::ShrinkFont:
				do {
					if (fontID == font_small || fontID == font_bar || fontID == font_regular || fontID == font_regular_bold)
					{
						break;
					}
					auto result = iV_ShrinkFont(fontID);
					if (!result.has_value())
					{
						break;
					}
					fontID = result.value();
					cache.wzText.setText(psBut->pText, fontID);
				} while (cache.wzText.width() > psBut->width());
				break;
			default:
				break;
		}
	}
	cache.lastWidgetWidth = psBut->width();

	if (psBut->isMouseOverWidget()) // if mouse is over text then hilight.
	{
		hilight = true;
	}

	fw = cache.wzText.width();
	fy = yOffset + psWidget->y() + (psWidget->height() - cache.wzText.lineSize()) / 2 - cache.wzText.aboveBase();

	if (psWidget->style & WBUT_TXTCENTRE)							//check for centering, calculate offset.
	{
		fx = xOffset + psWidget->x() + ((psWidget->width() - fw) / 2);
	}
	else
	{
		fx = xOffset + psWidget->x();
	}

	PIELIGHT colour;

	if (greyOut)														// unavailable
	{
		colour = WZCOL_TEXT_DARK;
	}
	else															// available
	{
		if (hilight)													// hilight
		{
			colour = WZCOL_TEXT_BRIGHT;
		}
		else if (psWidget->id == FRONTEND_HYPERLINK || psWidget->id == FRONTEND_DONATELINK || psWidget->id == FRONTEND_CHATLINK) // special case for our hyperlink
		{
			colour = WZCOL_YELLOW;
		}
		else														// don't highlight
		{
			colour = WZCOL_TEXT_MEDIUM;
		}
	}

	cache.wzText.render(fx, fy, colour);

	return;
}


// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// common widgets.

W_FORM *addBackdrop()
{
	return addBackdrop(psWScreen);
}

W_FORM *addBackdrop(const std::shared_ptr<W_SCREEN> &screen)
{
	ASSERT_OR_RETURN(nullptr, screen != nullptr, "Invalid screen pointer");
	W_FORMINIT sFormInit;                              // Backdrop
	sFormInit.formID = 0;
	sFormInit.id = FRONTEND_BACKDROP;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.calcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry((SWORD)((pie_GetVideoBufferWidth() - HIDDEN_FRONTEND_WIDTH) / 2),
							  (SWORD)((pie_GetVideoBufferHeight() - HIDDEN_FRONTEND_HEIGHT) / 2),
							  HIDDEN_FRONTEND_WIDTH - 1,
							  HIDDEN_FRONTEND_HEIGHT - 1);
	});
	sFormInit.pDisplay = displayTitleBitmap;
	sFormInit.pUserData = new TitleBitmapCache();
	sFormInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete ((TitleBitmapCache *)psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};

	return widgAddForm(screen, &sFormInit);
}

// ////////////////////////////////////////////////////////////////////////////
void addTopForm(bool wide)
{
	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);
	ASSERT(parent != nullptr, "Unable to find FRONTEND_BACKDROP?");

	auto topForm = std::make_shared<IntFormTransparent>();
	parent->attach(topForm);
	topForm->id = FRONTEND_TOPFORM;
	if (wide)
	{
		topForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(FRONTEND_TOPFORM_WIDEX, FRONTEND_TOPFORM_WIDEY, FRONTEND_TOPFORM_WIDEW, FRONTEND_TOPFORM_WIDEH);
		}));
	}
	else
	{
		topForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(FRONTEND_TOPFORMX, FRONTEND_TOPFORMY, FRONTEND_TOPFORMW, FRONTEND_TOPFORMH);
		}));
	}

	W_FORMINIT sFormInit;
	sFormInit.formID = FRONTEND_TOPFORM;
	sFormInit.id	= FRONTEND_LOGO;
	int imgW = iV_GetImageWidth(FrontImages, IMAGE_FE_LOGO);
	int imgH = iV_GetImageHeight(FrontImages, IMAGE_FE_LOGO);
	int dstW = topForm->width();
	int dstH = topForm->height();
	if (imgW * dstH < imgH * dstW) // Want to set aspect ratio dstW/dstH = imgW/imgH.
	{
		dstW = imgW * dstH / imgH; // Too wide.
	}
	else if (imgW * dstH > imgH * dstW)
	{
		dstH = imgH * dstW / imgW; // Too high.
	}
	sFormInit.x = (topForm->width()  - dstW) / 2;
	sFormInit.y = (topForm->height() - dstH) / 2;
	sFormInit.width  = dstW;
	sFormInit.height = dstH;
	sFormInit.pDisplay = displayLogo;
	widgAddForm(psWScreen, &sFormInit);
}

// ////////////////////////////////////////////////////////////////////////////
void addBottomForm(bool wide)
{
	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	auto botForm = std::make_shared<IntFormAnimated>();
	parent->attach(botForm);
	botForm->id = FRONTEND_BOTFORM;

	if (wide)
	{
		botForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(FRONTEND_BOTFORM_WIDEX, FRONTEND_BOTFORM_WIDEY, FRONTEND_BOTFORM_WIDEW, FRONTEND_BOTFORM_WIDEH);
		}));
	}
	else
	{
		botForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(FRONTEND_BOTFORMX, FRONTEND_BOTFORMY, FRONTEND_BOTFORMW, FRONTEND_BOTFORMH);
		}));
	}
}

// ////////////////////////////////////////////////////////////////////////////
void addText(UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt, UDWORD formID)
{
	WIDGET *parent = widgGetFromID(psWScreen, formID);
	ASSERT(parent != nullptr, "Unable to find formID: %" PRIu32, formID);

	auto label = std::make_shared<W_LABEL>();
	parent->attach(label);
	label->id = id;
	label->setGeometry(PosX, PosY, MULTIOP_READY_WIDTH, FRONTEND_BUTHEIGHT);
	label->setTextAlignment(WLAB_ALIGNCENTRE);
	label->setFont(font_small, WZCOL_TEXT_BRIGHT);
	label->setString(txt);
}

// ////////////////////////////////////////////////////////////////////////////
W_LABEL *addSideText(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD formId, UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt)
{
	ASSERT_OR_RETURN(nullptr, psScreen != nullptr, "Invalid screen pointer");

	W_LABINIT sLabInit;

	sLabInit.formID = formId;
	sLabInit.id = id;
	sLabInit.x = (short) PosX;
	sLabInit.y = (short) PosY;
	sLabInit.width = 30;
	sLabInit.height = FRONTEND_BOTFORMH;

	sLabInit.FontID = font_large;

	sLabInit.pDisplay = displayTextAt270;
	sLabInit.pText = WzString::fromUtf8(txt);
	sLabInit.pUserData = new DisplayTextOptionCache();
	sLabInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};

	return widgAddLabel(psScreen, &sLabInit);
}

W_LABEL *addSideText(UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt)
{
	return addSideText(psWScreen, FRONTEND_BACKDROP, id, PosX, PosY, txt);
}

// ////////////////////////////////////////////////////////////////////////////
static std::shared_ptr<W_BUTTON> makeTextButton(UDWORD id, const std::string &txt, unsigned int style, optional<unsigned int> minimumWidth)
{
	W_BUTINIT sButInit;

	sButInit.formID = FRONTEND_BOTFORM;
	sButInit.id = id;

	// Align
	sButInit.FontID = font_large;
	if (!(style & WBUT_TXTCENTRE))
	{
		auto textWidth = iV_GetTextWidth(txt.c_str(), sButInit.FontID);
		sButInit.width = (short)std::max<unsigned int>(minimumWidth.value_or(0), textWidth);
	}
	else
	{
		sButInit.style |= WBUT_TXTCENTRE;
	}

	// Enable right clicks
	if (style & WBUT_SECONDARY)
	{
		sButInit.style |= WBUT_SECONDARY;
	}

	sButInit.UserData = (style & WBUT_DISABLE); // store disable state
	auto pUserData = new DisplayTextOptionCache();
	pUserData->overflowBehavior = DisplayTextOptionCache::OverflowBehavior::ShrinkFont;
	sButInit.pUserData = pUserData;
	sButInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};

	sButInit.height = FRONTEND_BUTHEIGHT;
	sButInit.pDisplay = displayTextOption;
	sButInit.pText = txt.c_str();

	auto button = std::make_shared<W_BUTTON>(&sButInit);
	// Disable button
	if (style & WBUT_DISABLE)
	{
		button->setState(WBUT_DISABLE);
	}

	return button;
}

static std::shared_ptr<WIDGET> addMargin(std::shared_ptr<WIDGET> widget)
{
	return Margin(0, 20).wrap(widget);
}

void addTextButton(UDWORD id,  UDWORD PosX, UDWORD PosY, const std::string &txt, unsigned int style)
{
	auto button = makeTextButton(id, txt, style);
	if (style & WBUT_TXTCENTRE)
	{
		button->setGeometry(PosX, PosY, FRONTEND_BUTWIDTH, button->height());
	}
	else
	{
		button->move(PosX + 35, PosY);
	}

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BOTFORM);
	ASSERT(parent != nullptr, "Unable to find FRONTEND_BOTFORM?");
	parent->attach(button);
}

W_BUTTON * addSmallTextButton(UDWORD id,  UDWORD PosX, UDWORD PosY, const char *txt, unsigned int style)
{
	W_BUTINIT sButInit;

	sButInit.formID = FRONTEND_BOTFORM;
	sButInit.id = id;
	sButInit.x = (short)PosX;
	sButInit.y = (short)PosY;

	// Align
	if (!(style & WBUT_TXTCENTRE))
	{
		sButInit.width = (uint16_t)(iV_GetTextWidth(txt, font_small)) + 4;
		sButInit.x += 35;
	}
	else
	{
		sButInit.style |= WBUT_TXTCENTRE;
		sButInit.width = FRONTEND_BUTWIDTH;
	}

	// Enable right clicks
	if (style & WBUT_SECONDARY)
	{
		sButInit.style |= WBUT_SECONDARY;
	}

	sButInit.UserData = (style & WBUT_DISABLE); // store disable state
	sButInit.pUserData = new DisplayTextOptionCache();
	sButInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};

	sButInit.height = FRONTEND_BUTHEIGHT;
	sButInit.pDisplay = displayTextOption;
	sButInit.FontID = font_small;
	sButInit.pText = txt;
	W_BUTTON * pButton = widgAddButton(psWScreen, &sButInit);

	// Disable button
	if (style & WBUT_DISABLE)
	{
		widgSetButtonState(psWScreen, id, WBUT_DISABLE);
	}
	return pButton;
}

// ////////////////////////////////////////////////////////////////////////////
static std::shared_ptr<W_SLIDER> makeFESlider(UDWORD id, UDWORD parent, UDWORD stops, UDWORD pos)
{
	W_SLDINIT sSldInit;
	sSldInit.formID		= parent;
	sSldInit.id			= id;
	sSldInit.width		= iV_GetImageWidth(IntImages, IMAGE_SLIDER_BIG);
	sSldInit.height		= iV_GetImageHeight(IntImages, IMAGE_SLIDER_BIG);
	sSldInit.numStops	= (UBYTE) stops;
	sSldInit.barSize	= iV_GetImageHeight(IntImages, IMAGE_SLIDER_BIG);
	sSldInit.pos		= (UBYTE) pos;
	sSldInit.pDisplay	= displayBigSlider;
	sSldInit.pCallback  = intUpdateQuantitySlider;

	auto slider = std::make_shared<W_SLIDER>(&sSldInit);
	return slider;
}

void addFESlider(UDWORD id, UDWORD parent, UDWORD x, UDWORD y, UDWORD stops, UDWORD pos)
{
	auto slider = makeFESlider(id, parent, stops, pos);
	slider->move(x, y);
	widgGetFromID(psWScreen, parent)->attach(slider);
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

// To be called from frontendShutdown(), which for some reason is in init.cpp...
void frontendIsShuttingDown()
{
	std::weak_ptr<IMAGEFILE> pFlagsImagesWeak(pFlagsImages);
	pFlagsImages.reset();
	ASSERT(pFlagsImagesWeak.expired(), "A reference to the flags.img IMAGEFILE still exists!");
}
