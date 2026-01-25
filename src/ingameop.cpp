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
/**
 * @file ingameop.c
 * Ingame options screen.
 * Pumpkin Studios. 98
 */

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/widget/widget.h"
#include "lib/widget/label.h"
#include "lib/netplay/netplay.h"
#include "lib/sound/audio.h"					// for sound.
#include "lib/sound/mixer.h"

#include "display3d.h"
#include "intdisplay.h"
#include "hci.h"			// for wFont def.& intmode.
#include "loop.h"
#include "lib/framework/cursors.h"
#include "frontend.h"		// for textdisplay function
#include "loadsave.h"		// for textdisplay function
#include "console.h"		// to add console message
#include "keybind.h"
#include "multiplay.h"
#include "musicmanager.h"
#include "ingameop.h"
#include "mission.h"
#include "warzoneconfig.h"
#include "qtscript.h"		// for bInTutorial
#include "radar.h"
#include "seqdisp.h"
#include "campaigninfo.h"
#include "hci/groups.h"
#include "screens/netpregamescreen.h"
#include "screens/guidescreen.h"
#include "screens/ingameopscreen.h"
#include "titleui/options/optionsforms.h"

bool hostQuitConfirmation = true;

bool	InGameOpUp		= false;
bool 	isInGamePopupUp = false;
static bool		isInGameConfirmQuitUp = false;

// ////////////////////////////////////////////////////////////////////////////
// functions

// ////////////////////////////////////////////////////////////////////////////

static bool addIGTextButton(UDWORD id, UWORD x, UWORD y, UWORD width, const char *string, UDWORD Style)
{
	W_BUTINIT sButInit;

	//resume
	sButInit.formID		= INTINGAMEOP;
	sButInit.id			= id;
	sButInit.style		= Style;


	sButInit.x			= x;
	sButInit.y			= y;
	sButInit.width		= width;
	sButInit.height		= INTINGAMEOP_OP_H;

	sButInit.pDisplay	= displayTextOption;
	sButInit.pText		= string;
	sButInit.pUserData = new DisplayTextOptionCache();
	sButInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};
	widgAddButton(psWScreen, &sButInit);

	return true;
}

static bool addHostQuitOptions()
{
	// get rid of the old stuff.
	widgDelete(psWScreen, INTINGAMEPOPUP);
	widgDelete(psWScreen, INTINGAMEOP);

	auto const &parent = psWScreen->psForm;

	// add form
	auto inGameOp = std::make_shared<IntFormAnimated>();
	parent->attach(inGameOp);
	inGameOp->id = INTINGAMEOP;
	inGameOp->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(INTINGAMEOP3_X, INTINGAMEOP3_Y, INTINGAMEOP3_W, INTINGAMEOP3_H);
	}));

	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOP_1_Y, INTINGAMEOP_OP_W, _("Resume Game"), OPALIGN);

	auto inGamePopup = std::make_shared<IntFormAnimated>();
	parent->attach(inGamePopup);
	inGamePopup->id = INTINGAMEPOPUP;
	inGamePopup->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		assert(psWScreen != nullptr);
		WIDGET * inGameOp = widgGetFromID(psWScreen, INTINGAMEOP);
		assert(inGameOp != nullptr);
		psWidget->setGeometry((pie_GetVideoBufferWidth() - 600)/2, inGameOp->y() - 26 - 20, 600, 26);
	}));

	auto label = std::make_shared<W_LABEL>();
	inGamePopup->attach(label);
	label->setGeometry(0, 0, inGamePopup->width(), inGamePopup->height());
	label->setString(_("WARNING: You're the host. If you quit, the game ends for everyone!"));
	label->setTextAlignment(WLAB_ALIGNCENTRE);
	label->setFont(font_medium, WZCOL_YELLOW);

	addIGTextButton(INTINGAMEOP_QUIT, INTINGAMEOP_1_X, INTINGAMEOP_2_Y, INTINGAMEOP_OP_W, _("Host Quit"), OPALIGN);

	return true;
}

// ////////////////////////////////////////////////////////////////////////////

static bool _intAddInGameOptions()
{
	audio_StopAll();

	//clear out any mission widgets - timers etc that may be on the screen
	clearMissionWidgets();

	setWidgetsStatus(true);

	//if already open, then close!
	if (widgGetFromID(psWScreen, INTINGAMEOP))
	{
		intCloseInGameOptions(false, true);
		return true;
	}

	setReticulesEnabled(false);

	// Pause the game.
	if (!gamePaused())
	{
		kf_TogglePauseMode();
	}

	auto const &parent = psWScreen->psForm;

	// add form
	auto ingameOp = std::make_shared<IntFormAnimated>();
	parent->attach(ingameOp);
	ingameOp->id = INTINGAMEOP;

	int row = 1;
	// add 'resume'
	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Resume Game"), OPALIGN);
	row++;

	if (hasGameGuideTopics())
	{
		// add "View Guide"
		addIGTextButton(INTINGAMEOP_OPENGAMEGUIDE, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("View Guide"), OPALIGN);
		row++;
	}

	// add 'options'
	addIGTextButton(INTINGAMEOP_OPTIONS, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Options"), OPALIGN);
	row++;

	if (!((bMultiPlayer && NetPlay.bComms) || bInTutorial || NETisReplay()))
	{
		if (bMultiPlayer)
		{
			// add 'load'
			addIGTextButton(INTINGAMEOP_LOAD_SKIRMISH, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Load Game"), OPALIGN);
			row++;
			// add 'save'
			addIGTextButton(INTINGAMEOP_SAVE_SKIRMISH, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Save Game"), OPALIGN);
			row++;
		}
		else
		{
			// add 'load'
			addIGTextButton(INTINGAMEOP_LOAD_MISSION, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Load Game"), OPALIGN);
			row++;
			if (!getCamTweakOption_AutosavesOnly())
			{
				// add 'save'
				addIGTextButton(INTINGAMEOP_SAVE_MISSION, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Save Game"), OPALIGN);
				row++;
			}
		}
	}

	// add 'quit' text
	if (NetPlay.isHost && bMultiPlayer && NetPlay.bComms)
	{
		addIGTextButton(hostQuitConfirmation ? INTINGAMEOP_HOSTQUIT : INTINGAMEOP_QUIT, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Host Quit"), OPALIGN);
	}
	else
	{
		addIGTextButton(INTINGAMEOP_CONFIRM_QUIT, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Quit"), OPALIGN);
	}

	// calculate layout
	int lines = row;
	ingameOp->setCalcLayout([lines](WIDGET *psWidget){
		psWidget->setGeometry(INTINGAMEOP_X, INTINGAMEOPAUTO_Y(lines), INTINGAMEOP_W, INTINGAMEOPAUTO_H(lines));
	});

	intMode		= INT_INGAMEOP;			// change interface mode.
	InGameOpUp	= true;					// inform interface.

	wzSetCursor(CURSOR_DEFAULT);

	return true;
}

bool intAddInGameOptions()
{
	return _intAddInGameOptions();
}

static bool startInGameConfirmQuit()
{
	widgDelete(psWScreen, INTINGAMEOP); // get rid of the old stuff.
	auto const& parent = psWScreen->psForm;

	// add form
	auto ingameOp = std::make_shared<IntFormAnimated>();
	parent->attach(ingameOp);
	ingameOp->id = INTINGAMEOP;

	int row = 1;

	auto label = std::make_shared<W_LABEL>();
	ingameOp->attach(label);
	label->setGeometry(INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP4_OP_W, INTINGAMEOP_OP_H);
	if (bMultiPlayer && (NetPlay.bComms || NETisReplay()))
	{
		label->setString(_("Warning: Are you sure?")); //Do not mention saving in real multiplayer matches
	}
	else
	{
		label->setString(_("Warning: Are you sure? Any unsaved progress will be lost."));
	}
	label->setTextAlignment(WLAB_ALIGNCENTRE);
	label->setFont(font_medium, WZCOL_YELLOW);

	row++;

	// add quit confirmation text
	addIGTextButton(INTINGAMEOP_QUIT, INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP4_OP_W, _("Confirm"), OPALIGN);
	row++;
	addIGTextButton(INTINGAMEOP_GO_BACK, INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP4_OP_W, _("Back"), OPALIGN);

	ingameOp->setCalcLayout([row](WIDGET* psWidget) {
		psWidget->setGeometry(INTINGAMEOP4_X, INTINGAMEOPAUTO_Y(row), INTINGAMEOP4_W, INTINGAMEOPAUTO_H(row));
	});

	return true;
}

static bool runInGameConfirmQuit(UDWORD id)
{
	switch (id)
	{
		case INTINGAMEOP_QUIT:
			return true;

		case INTINGAMEOP_GO_BACK:
			intReopenMenuWithoutUnPausing();
			break;

		default:
			break;
	}

	return false;
}

//
// Quick hack to throw up a ingame 'popup' for when the host drops connection.
//
void intAddInGamePopup()
{
	//clear out any mission widgets - timers etc that may be on the screen
	clearMissionWidgets();
	setWidgetsStatus(true);
	intResetScreen(false);

	if (isInGamePopupUp)
	{
		return;
	}

	intCloseInGameOptions(false, false); //clear out option-like menus

	audio_StopAll();

	if (!gamePaused())
	{
		kf_TogglePauseMode();	// Pause the game.
	}

	auto const &parent = psWScreen->psForm;

	auto ingamePopup = std::make_shared<IntFormAnimated>();
	parent->attach(ingamePopup);
	ingamePopup->id = INTINGAMEPOPUP;
	ingamePopup->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(20 + D_W, (240 - 160 / 2) + D_H, 600, 160);
	}));

	// add the text "buttons" now
	W_BUTINIT sButInit;

	sButInit.formID		= INTINGAMEPOPUP;
	sButInit.style		= OPALIGN;
	sButInit.width		= 600;
	sButInit.FontID		= font_large;
	sButInit.x			= 0;
	sButInit.height		= 10;
	sButInit.pDisplay	= displayTextOption;
	sButInit.initPUserDataFunc = []() -> void * { return new DisplayTextOptionCache(); };
	sButInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};

	sButInit.id			= INTINGAMEOP_POPUP_MSG2;
	sButInit.y			= 20;
	sButInit.pText		= _("Host has quit the game!");

	widgAddButton(psWScreen, &sButInit);

	sButInit.id			= INTINGAMEOP_POPUP_MSG1;
	sButInit.y			= 60;
	sButInit.pText		= _("The game can't continue without the host.");

	widgAddButton(psWScreen, &sButInit);

	sButInit.id			= INTINGAMEOP_POPUP_QUIT;
	sButInit.y			= 124;
	sButInit.pText		= _("-->  QUIT  <--");

	widgAddButton(psWScreen, &sButInit);

	intMode		= INT_POPUPMSG;			// change interface mode.
	isInGamePopupUp = true;

	shutdownGameStartScreen();
}

// ////////////////////////////////////////////////////////////////////////////

static void ProcessOptionFinished()
{
	intMode		= INT_NORMAL;

	if (gamePaused())
	{
		kf_TogglePauseMode();
	}

	setReticulesEnabled(true);
}

void intCloseInGameOptionsNoAnim()
{
	if (NetPlay.isHost)
	{
		widgDelete(psWScreen, INTINGAMEPOPUP);
	}
	widgDelete(psWScreen, INTINGAMEOP);
	InGameOpUp = false;

	ProcessOptionFinished();
	resetMissionWidgets();
}

// ////////////////////////////////////////////////////////////////////////////
bool intCloseInGameOptions(bool bPutUpLoadSave, bool bResetMissionWidgets)
{
	isInGameConfirmQuitUp = false;

	if (NetPlay.isHost)
	{
		widgDelete(psWScreen, INTINGAMEPOPUP);
	}

	if (bPutUpLoadSave)
	{
		WIDGET *widg = widgGetFromID(psWScreen, INTINGAMEOP);
		if (widg)
		{
			widgDelete(psWScreen, INTINGAMEOP);
		}

		InGameOpUp = false;
	}
	if (!bPutUpLoadSave)
	{
		// close the form.
		// Start the window close animation.
		IntFormAnimated *form = dynamic_cast<IntFormAnimated *>(widgGetFromID(psWScreen, INTINGAMEPOPUP));
		if (form)	// FIXME: we hijack this routine for the popup close.
		{
			form->closeAnimateDelete();
			isInGamePopupUp = false;
		}
		form = dynamic_cast<IntFormAnimated *>(widgGetFromID(psWScreen, INTINGAMEOP_POPUP_QUIT));
		if (form)
		{
			form->closeAnimateDelete();
			isInGamePopupUp = false;
		}

		form = dynamic_cast<IntFormAnimated *>(widgGetFromID(psWScreen, INTINGAMEOP));
		if (form)
		{
			form->closeAnimateDelete();
			InGameOpUp = false;
		}
		form = dynamic_cast<IntFormAnimated *>(widgGetFromID(psWScreen, INTINGAMEOP_QUIT));
		if (form)
		{
			form->closeAnimateDelete();
			InGameOpUp = false;
		}
	}

	ProcessOptionFinished();

	//don't add the widgets if the load/save screen is put up or exiting to front end
	if (bResetMissionWidgets)
	{
		//put any widgets back on for the missions
		resetMissionWidgets();
	}

	// the setting for group menu display may have been modified
	intShowGroupSelectionMenu();
	return true;
}

void intReopenMenuWithoutUnPausing()
{
	isInGameConfirmQuitUp = false;

	if (NetPlay.isHost)
	{
		widgDelete(psWScreen, INTINGAMEPOPUP);
	}
	widgDelete(psWScreen, INTINGAMEOP);
	intAddInGameOptions();
}

static bool startIGOptionsMenu()
{
	showInGameOptionsScreen(psWScreen, createOptionsBrowser(true), []() {
		// the setting for group menu display may have been modified
		intShowGroupSelectionMenu();
	});
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// process clicks made by user.
void intProcessInGameOptions(UDWORD id)
{
	if (isInGameConfirmQuitUp)
	{
		if (runInGameConfirmQuit(id))
		{
			intCloseInGameOptions(true, true);
		}
		return;
	}

	switch (id)
	{
	// NORMAL KEYS
	case INTINGAMEOP_HOSTQUIT:				//quit was pressed
		addHostQuitOptions();
		break;

	case INTINGAMEOP_CONFIRM_QUIT:
		startInGameConfirmQuit();
		isInGameConfirmQuitUp = true;
		break;

	case INTINGAMEOP_POPUP_QUIT:
	case INTINGAMEOP_QUIT:		//quit was confirmed.
		intCloseInGameOptions(false, false);
		break;

	case INTINGAMEOP_OPENGAMEGUIDE:
		intCloseInGameOptions(false, false);
		showGuideScreen([]() { /* no=op on close func */ }, true);
		break;

	case INTINGAMEOP_OPTIONS:			//game options  was pressed
		intCloseInGameOptions(false, false);
		startIGOptionsMenu();
		break;
	case INTINGAMEOP_RESUME:			//resume was pressed.
		intCloseInGameOptions(false, true);
		break;
	case INTINGAMEOP_GO_BACK:
		intReopenMenuWithoutUnPausing();
		break;


//	case INTINGAMEOP_REPLAY:
//		intCloseInGameOptions(true, false);
//		if(0!=strcmp(getLevelName(),"CAM_1A"))
//		{
//			loopMissionState = LMS_LOADGAME;
//			strcpy(saveGameName, "replay/replay.gam");
//			addConsoleMessage(_("GAME SAVED!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
//		}
//		break;
	case INTINGAMEOP_LOAD_MISSION:
		intCloseInGameOptions(true, false);
		addLoadSave(LOAD_INGAME_MISSION, _("Load Campaign Saved Game"));	// change mode when loadsave returns
		break;
	case INTINGAMEOP_LOAD_SKIRMISH:
		intCloseInGameOptions(true, false);
		addLoadSave(LOAD_INGAME_SKIRMISH, _("Load Skirmish Saved Game"));	// change mode when loadsave returns
		break;
	case INTINGAMEOP_SAVE_MISSION:
		intCloseInGameOptions(true, false);
		addLoadSave(SAVE_INGAME_MISSION, _("Save Campaign Game"));
		break;
	case INTINGAMEOP_SAVE_SKIRMISH:
		intCloseInGameOptions(true, false);
		addLoadSave(SAVE_INGAME_SKIRMISH, _("Save Skirmish Game"));
		break;

	default:
		break;
	}
}
