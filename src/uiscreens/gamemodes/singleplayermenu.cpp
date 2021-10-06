#include "singleplayermenu.h"

#include "../../challenge.h" // challengeActive
#include "../../loadsave.h" // loadSave
#include "../../multiplay.h" // InGameSide, game
#include "../../configuration.h" // skirmish constants

#include "multiplayermenu.h" // bMultiplayer


// ////////////////////////////////////////////////////////////////////////////
// Single Player Menu
void startSinglePlayerMenu()
{
	challengeActive = false;
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	addTextButton(FRONTEND_NEWGAME, FRONTEND_POS2X, FRONTEND_POS2Y, _("New Campaign"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_SKIRMISH, FRONTEND_POS3X, FRONTEND_POS3Y, _("Start Skirmish Game"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_CHALLENGES, FRONTEND_POS4X, FRONTEND_POS4Y, _("Challenges"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_LOADGAME_MISSION, FRONTEND_POS5X, FRONTEND_POS5Y, _("Load Campaign Game"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_LOADGAME_SKIRMISH, FRONTEND_POS6X, FRONTEND_POS6Y, _("Load Skirmish Game"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_REPLAY, FRONTEND_POS7X, FRONTEND_POS7Y, _("View Skirmish Replay"), WBUT_TXTCENTRE);

	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("SINGLE PLAYER"));
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
	// show this only when the video sequences are not installed
	if (!PHYSFS_exists("sequences/devastation.ogg"))
	{
		addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS9X, FRONTEND_POS9Y, _("Campaign videos are missing! Get them from http://wz2100.net"), 0);
	}
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
		WidgetTriggers const& triggers = widgRunScreen(psWScreen);
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
