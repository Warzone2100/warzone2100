#include "multiplayermenu.h"

#include "../../titleui/titleui.h"
#include "../../titleui/multiplayer.h"

#include "../../challenge.h" // challengeActive
#include "../../loadsave.h" // loadSave
#include "../../multiplay.h" // InGameSide, game
#include "../../configuration.h" // skirmish constants

// ////////////////////////////////////////////////////////////////////////////
// Multi Player Menu
void startMultiPlayerMenu()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("MULTI PLAYER"));

	addTextButton(FRONTEND_HOST, FRONTEND_POS2X, FRONTEND_POS2Y, _("Host Game"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_JOIN, FRONTEND_POS3X, FRONTEND_POS3Y, _("Join Game"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_REPLAY, FRONTEND_POS7X, FRONTEND_POS7Y, _("View Replay"), WBUT_TXTCENTRE);

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
		WidgetTriggers const& triggers = widgRunScreen(psWScreen);
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
