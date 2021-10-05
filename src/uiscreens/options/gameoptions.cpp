#include "gameoptions.h"


#include "lib/widget/gridlayout.h"
#include "lib/widget/alignment.h"

#include "../../warzoneconfig.h"
#include "../../difficulty.h"
#include "../../urlhelpers.h"
#include "../../notifications.h"

static char const* gameOptionsDifficultyString()
{
	switch (getDifficultyLevel())
	{
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
	if (getCameraAccel())
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
void startGameOptionsMenu()
{
	UDWORD	w, h;
	int playercolor;

	addBackdrop();
	addTopForm(false);
	addBottomForm();

	// language
	addTextButton(FRONTEND_LANGUAGE, FRONTEND_POS2X - 35, FRONTEND_POS2Y, _("Language"), WBUT_SECONDARY);
	addTextButton(FRONTEND_LANGUAGE_R, FRONTEND_POS2M - 55, FRONTEND_POS2Y, getLanguageName(), WBUT_SECONDARY);

	// Difficulty
	addTextButton(FRONTEND_DIFFICULTY, FRONTEND_POS3X - 35, FRONTEND_POS3Y, _("Campaign Difficulty"), WBUT_SECONDARY);
	addTextButton(FRONTEND_DIFFICULTY_R, FRONTEND_POS3M - 55, FRONTEND_POS3Y, gameOptionsDifficultyString(), WBUT_SECONDARY);

	// Camera speed
	addTextButton(FRONTEND_CAMERASPEED, FRONTEND_POS4X - 35, FRONTEND_POS4Y, _("Camera Speed"), WBUT_SECONDARY);
	addTextButton(FRONTEND_CAMERASPEED_R, FRONTEND_POS4M - 55, FRONTEND_POS4Y, gameOptionsCameraSpeedString(), WBUT_SECONDARY);

	// Colour stuff
	addTextButton(FRONTEND_COLOUR, FRONTEND_POS5X - 35, FRONTEND_POS5Y, _("Unit Colour:"), 0);

	w = iV_GetImageWidth(FrontImages, IMAGE_PLAYERN);
	h = iV_GetImageHeight(FrontImages, IMAGE_PLAYERN);

	addMultiBut(psWScreen, FRONTEND_BOTFORM, FE_P0, FRONTEND_POS6M + (0 * (w + 6)) - 20, FRONTEND_POS6Y, w, h, nullptr, IMAGE_PLAYERN, IMAGE_PLAYERX, true, 0);
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FE_P4, FRONTEND_POS6M + (1 * (w + 6)) - 20, FRONTEND_POS6Y, w, h, nullptr, IMAGE_PLAYERN, IMAGE_PLAYERX, true, 4);
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FE_P5, FRONTEND_POS6M + (2 * (w + 6)) - 20, FRONTEND_POS6Y, w, h, nullptr, IMAGE_PLAYERN, IMAGE_PLAYERX, true, 5);
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FE_P6, FRONTEND_POS6M + (3 * (w + 6)) - 20, FRONTEND_POS6Y, w, h, nullptr, IMAGE_PLAYERN, IMAGE_PLAYERX, true, 6);
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FE_P7, FRONTEND_POS6M + (4 * (w + 6)) - 20, FRONTEND_POS6Y, w, h, nullptr, IMAGE_PLAYERN, IMAGE_PLAYERX, true, 7);

	// FIXME: if playercolor = 1-3, then we Assert in widgSetButtonState() since we don't define FE_P1 - FE_P3
	// I assume the reason is that in SP games, those are reserved for the AI?  Valid values are 0, 4-7.
	// This is a workaround, until we find what is setting that to 1-3.  See configuration.c:701
	playercolor = war_GetSPcolor();
	if (playercolor >= 1 && playercolor <= 3)
	{
		playercolor = 0;
	}
	widgSetButtonState(psWScreen, FE_P0 + playercolor, WBUT_LOCK);
	addTextButton(FRONTEND_COLOUR_CAM, FRONTEND_POS6X - 20, FRONTEND_POS6Y, _("Campaign"), 0);

	playercolor = war_getMPcolour();
	for (int colour = -1; colour < MAX_PLAYERS_IN_GUI; ++colour)
	{
		int cellX = (colour + 1) % 7;
		int cellY = (colour + 1) / 7;
		addMultiBut(psWScreen, FRONTEND_BOTFORM, FE_MP_PR + colour + 1, FRONTEND_POS7M + cellX * (w + 2) - 20, FRONTEND_POS7Y + cellY * (h + 2) - 5, w, h, nullptr, IMAGE_PLAYERN, IMAGE_PLAYERX, true, colour >= 0 ? colour : MAX_PLAYERS + 1);
	}
	widgSetButtonState(psWScreen, FE_MP_PR + playercolor + 1, WBUT_LOCK);
	addTextButton(FRONTEND_COLOUR_MP, FRONTEND_POS7X - 20, FRONTEND_POS7Y, _("Skirmish/Multiplayer"), 0);

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
	WidgetTriggers const& triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_HYPERLINK:
		openURLInBrowser(TRANSLATION_URL);
		break;
	case FRONTEND_LANGUAGE:
	case FRONTEND_LANGUAGE_R:

		setNextLanguage(widgGetButtonKey_DEPRECATED(psWScreen) == WKEY_PRIMARY);
		widgSetString(psWScreen, FRONTEND_LANGUAGE_R, getLanguageName());
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
		break;

	case FRONTEND_DIFFICULTY:
	case FRONTEND_DIFFICULTY_R:
		setDifficultyLevel(seqCycle(getDifficultyLevel(), DL_EASY, 1, DL_INSANE));
		widgSetString(psWScreen, FRONTEND_DIFFICULTY_R, gameOptionsDifficultyString());
		if (getDifficultyLevel() == DL_INSANE)
		{
			const std::string DIFF_TAG = "difficulty";

			if (!hasNotificationsWithTag(DIFF_TAG))
			{
				WZ_Notification notification;
				notification.duration = 10 * GAME_TICKS_PER_SEC;;
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
