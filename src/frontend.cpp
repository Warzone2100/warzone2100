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
#include "lib/netplay/connection_provider_registry.h"
#include "lib/netplay/netplay.h"
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
#include "titleui/campaign.h"
#include "titleui/optionstitleui.h"
#include "urlhelpers.h"
#include "game.h"
#include "map.h" //for builtInMap and useTerrainOverrides
#include "notifications.h"
#include "activity.h"
#include "hci/groups.h"

#include <fmt/core.h>

#if defined(__EMSCRIPTEN__)
# include "emscripten_helpers.h"
#endif

// ////////////////////////////////////////////////////////////////////////////
// Global variables

char			aLevelName[MAX_LEVEL_NAME_SIZE + 1];	//256];			// vital! the wrf file to use.

bool			bLimiterLoaded = false;

static std::shared_ptr<IMAGEFILE> pFlagsImages;

#define TUTORIAL_LEVEL "TUTORIAL3"
#define TRANSLATION_URL "https://translate.wz2100.net"

// ////////////////////////////////////////////////////////////////////////////
// Forward definitions

static std::shared_ptr<W_BUTTON> makeTextButton(UDWORD id, const std::string &txt, unsigned int style, optional<unsigned int> minimumWidth = nullopt);
static std::shared_ptr<W_SLIDER> makeFESlider(UDWORD id, UDWORD parent, UDWORD stops, UDWORD pos);

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

const char * VIDEO_TAG = "videoMissing";

void notifyAboutMissingVideos()
{
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

static void closeMissingVideosNotification()
{
	cancelOrDismissNotificationsWithTag(VIDEO_TAG);
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
#if defined(__EMSCRIPTEN__)
		wzDisplayDialog(Dialog_Information, "Multiplayer requires the native version.", "The web version of Warzone 2100 does not support online multiplayer. Please visit https://wz2100.net to download the native version for your platform.");
#endif
		break;
	case FRONTEND_SINGLEPLAYER:
		changeTitleMode(SINGLE);
		break;
	case FRONTEND_OPTIONS:
		changeTitleUI(std::make_shared<WzOptionsTitleUI>(wzTitleUICurrent));
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
		setGroupButtonEnabled(false); // hack to disable the groups UI for the tutorial
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
	}
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
	useTerrainOverrides = true;
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
			ActivityManager::instance().navigateToMenu("Campaign");
			changeTitleUI(std::make_shared<WzCampaignSelectorTitleUI>(wzTitleUICurrent));
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
	closeMissingVideosNotification();

	addBackdrop();
	addTopForm(false);
	addBottomForm();

	addSideText(FRONTEND_SIDETEXT ,	FRONTEND_SIDEX, FRONTEND_SIDEY, _("MULTI PLAYER"));

#if !defined(__EMSCRIPTEN__)
	addTextButton(FRONTEND_HOST,     FRONTEND_POS2X, FRONTEND_POS2Y, _("Host Game"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_JOIN,     FRONTEND_POS3X, FRONTEND_POS3Y, _("Join Game"), WBUT_TXTCENTRE);
#endif
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
			// First of all, make sure we've reset any prior networking state
			NETshutdown();
			// don't pretend we are running a network game. Really do it!
			NetPlay.bComms = true; // use network = true
			ingame.side = InGameSide::HOST_OR_SINGLEPLAYER;
			bMultiPlayer = true;
			bMultiMessages = true;
			NETinit(war_getHostConnectionProvider());
			NETinitPortMapping();
			game.type = LEVEL_TYPE::SKIRMISH;		// needed?
			changeTitleUI(std::make_shared<WzMultiplayerOptionsTitleUI>(wzTitleUICurrent));
			break;
		case FRONTEND_JOIN:
			// Don't call `NETinit()` just yet.
			// It will be called automatically during join attempts.
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

std::vector<unsigned int> availableDisplayScalesSorted()
{
	std::vector<unsigned int> displayScales = wzAvailableDisplayScales();
	std::sort(displayScales.begin(), displayScales.end());
	return displayScales;
}

std::shared_ptr<IMAGEFILE> getFlagsImages()
{
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
	return pFlagsImages;
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
#if defined(__EMSCRIPTEN__)
	cache.gfxBackend.setText(WZ_EmscriptenGetBottomRendererSysInfoString(), font_small);
#else
	cache.gfxBackend.setText(WzString::fromUtf8(gfx_api::context::get().getFormattedRendererInfoString()), font_small);
#endif

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
std::shared_ptr<W_LABEL> addSideText(WIDGET* psParent, UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt)
{
	ASSERT_OR_RETURN(nullptr, psParent != nullptr, "Invalid parent");

	W_LABINIT sLabInit;

	sLabInit.formID = 0;
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

	auto psLabel = std::make_shared<W_LABEL>(&sLabInit);
	psLabel->setTransparentToClicks(true);
	psLabel->setTransparentToMouse(true);
	psParent->attach(psLabel);
	return psLabel;
}

W_LABEL *addSideText(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD formId, UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt)
{
	ASSERT_OR_RETURN(nullptr, psScreen != nullptr, "Invalid screen pointer");

	WIDGET *psParent = widgGetFromID(psScreen, formId);
	return addSideText(psParent, id, PosX, PosY, txt).get();
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

std::shared_ptr<W_BUTTON> addTextButton(UDWORD id,  UDWORD PosX, UDWORD PosY, const std::string &txt, unsigned int style)
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
	return button;
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

std::shared_ptr<W_SLIDER> addFESlider(UDWORD id, UDWORD parent, UDWORD x, UDWORD y, UDWORD stops, UDWORD pos)
{
	auto slider = makeFESlider(id, parent, stops, pos);
	slider->move(x, y);
	widgGetFromID(psWScreen, parent)->attach(slider);
	return slider;
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
