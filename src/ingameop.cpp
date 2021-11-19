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
#include "keyedit.h"
#include "multiplay.h"
#include "musicmanager.h"
#include "ingameop.h"
#include "mission.h"
#include "warzoneconfig.h"
#include "qtscript.h"		// for bInTutorial
#include "radar.h"
#include "seqdisp.h"

bool hostQuitConfirmation = true;

bool	InGameOpUp		= false;
bool 	isInGamePopupUp = false;
static bool 	isGraphicsOptionsUp = false;
static bool 	isVideoOptionsUp = false;
static bool 	isMouseOptionsUp = false;
static bool		isInGameConfirmQuitUp = false;
bool	isKeyMapEditorUp = false;
bool	isMusicManagerUp = false;
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

static bool addAudioOptions()
{
	widgDelete(psWScreen, INTINGAMEOP);  // get rid of the old stuff.

	auto const &parent = psWScreen->psForm;

	// add form
	auto ingameOp = std::make_shared<IntFormAnimated>();
	parent->attach(ingameOp);
	ingameOp->id = INTINGAMEOP;

	int row = 1;
	// voice vol
	addIGTextButton(INTINGAMEOP_FXVOL, INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Voice Volume"), WBUT_PLAIN);
	addFESlider(INTINGAMEOP_FXVOL_S, INTINGAMEOP, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row),
	            AUDIO_VOL_MAX, (int)(sound_GetUIVolume() * 100.0));
	row++;

	// fx vol
	addIGTextButton(INTINGAMEOP_3DFXVOL, INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("FX Volume"), WBUT_PLAIN);
	addFESlider(INTINGAMEOP_3DFXVOL_S, INTINGAMEOP, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row),
	            AUDIO_VOL_MAX, (int)(sound_GetEffectsVolume() * 100.0));
	row++;

	// music vol
	addIGTextButton(INTINGAMEOP_CDVOL, INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Music Volume"), WBUT_PLAIN);
	addFESlider(INTINGAMEOP_CDVOL_S, INTINGAMEOP, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row),
	            AUDIO_VOL_MAX, (int)(sound_GetMusicVolume() * 100));
	row++;

	// Subtitles
	addIGTextButton(INTINGAMEOP_SUBTITLES,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Subtitles"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_SUBTITLES_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, graphicsOptionsSubtitlesString(), WBUT_PLAIN);
	row++;

#ifdef DEBUG
	// Tactical UI: Target Origin
	if (tuiTargetOrigin)
	{
		addIGTextButton(INTINGAMEOP_TUI_TARGET_ORIGIN_SW, INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_SW_W,
		                _("Tactical UI (Target Origin Icon): Show"), WBUT_PLAIN);
	}
	else
	{
		addIGTextButton(INTINGAMEOP_TUI_TARGET_ORIGIN_SW, INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_SW_W,
		                _("Tactical UI (Target Origin Icon): Hide"), WBUT_PLAIN);
	}
	row++;
#endif

	addIGTextButton(INTINGAMEOP_GO_BACK, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_SW_W, _("Go Back"), OPALIGN);
	row++;

	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_SW_W, _("Resume Game"), OPALIGN);

	ingameOp->setCalcLayout([row](WIDGET *psWidget) {
		psWidget->setGeometry(INTINGAMEOP2_X, INTINGAMEOPAUTO_Y(row), INTINGAMEOP2_W, INTINGAMEOPAUTO_H(row));
	});

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
	if (widgGetFromID(psWScreen, INTINGAMEOP) || isMusicManagerUp)
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
			// add 'save'
			addIGTextButton(INTINGAMEOP_SAVE_MISSION, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Save Game"), OPALIGN);
			row++;
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
	sliderEnableDrag(true);
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
	label->setGeometry(INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP4_OP_W, 0);
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
	if (isKeyMapEditorUp)
	{
		runInGameKeyMapEditor(gInputManager, gKeyFuncConfig, KM_RETURN);
	}
	isKeyMapEditorUp = false;
	if (isMusicManagerUp)
	{
		runInGameMusicManager(MM_RETURN, gInputManager);
	}
	isMusicManagerUp = false;

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
	bool keymapWasUp = isKeyMapEditorUp;

	isGraphicsOptionsUp = false;
	isVideoOptionsUp = false;
	isMouseOptionsUp = false;
	isInGameConfirmQuitUp = false;

	if (isKeyMapEditorUp)
	{
		runInGameKeyMapEditor(gInputManager, gKeyFuncConfig, KM_RETURN);
	}
	isKeyMapEditorUp = false;
	if (isMusicManagerUp)
	{
		runInGameMusicManager(MM_RETURN, gInputManager);
	}
	isMusicManagerUp = false;

	if (NetPlay.isHost)
	{
		widgDelete(psWScreen, INTINGAMEPOPUP);
	}

	if (bPutUpLoadSave || keymapWasUp)
	{
		WIDGET *widg = widgGetFromID(psWScreen, INTINGAMEOP);
		if (widg)
		{
			widgDelete(psWScreen, INTINGAMEOP);
		}

		InGameOpUp = false;
	}
	if (!bPutUpLoadSave || keymapWasUp)
	{
		// close the form.
		// Start the window close animation.
		IntFormAnimated *form = (IntFormAnimated *)widgGetFromID(psWScreen, INTINGAMEPOPUP);
		if (form)	// FIXME: we hijack this routine for the popup close.
		{
			form->closeAnimateDelete();
			isInGamePopupUp = false;
		}
		form = (IntFormAnimated *)widgGetFromID(psWScreen, INTINGAMEOP_POPUP_QUIT);
		if (form)
		{
			form->closeAnimateDelete();
			isInGamePopupUp = false;
		}

		form = (IntFormAnimated *)widgGetFromID(psWScreen, INTINGAMEOP);
		if (form)
		{
			form->closeAnimateDelete();
			InGameOpUp = false;
		}
		form = (IntFormAnimated *)widgGetFromID(psWScreen, INTINGAMEOP_QUIT);
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

	return true;
}

void intReopenMenuWithoutUnPausing()
{
	isGraphicsOptionsUp = false;
	isVideoOptionsUp = false;
	isMouseOptionsUp = false;
	isKeyMapEditorUp = false;
	isMusicManagerUp = false;
	isInGameConfirmQuitUp = false;

	if (NetPlay.isHost)
	{
		widgDelete(psWScreen, INTINGAMEPOPUP);
	}
	widgDelete(psWScreen, INTINGAMEOP);
	widgDelete(psWScreen, MM_FORM); // hack: There's a soft-lock somewhere with the music manager UI. Ensure it gets closed here since we're setting isMusicManagerUp = false above
	intAddInGameOptions();
}

static bool startIGOptionsMenu()
{
	widgDelete(psWScreen, INTINGAMEOP);  // get rid of the old stuff.
	auto const &parent = psWScreen->psForm;
	isGraphicsOptionsUp = false;
	isVideoOptionsUp = false;
	isMouseOptionsUp = false;
	isKeyMapEditorUp = false;
	isMusicManagerUp = false;
	widgDelete(psWScreen, MM_FORM); // hack: There's a soft-lock somewhere with the music manager UI. Ensure it gets closed here since we're setting isMusicManagerUp = false above
	isInGameConfirmQuitUp = false;

	// add form
	auto ingameOp = std::make_shared<IntFormAnimated>();
	parent->attach(ingameOp);
	ingameOp->id = INTINGAMEOP;
	int row = 1;
	// Game Options can't be changed during game
	addIGTextButton(INTINGAMEOP_GRAPHICSOPTIONS, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Graphics Options"), OPALIGN);
	row++;

	addIGTextButton(INTINGAMEOP_VIDEOOPTIONS, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Video Options"), OPALIGN);
	row++;

	// Zoom options are not available during game
	addIGTextButton(INTINGAMEOP_AUDIOOPTIONS, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Audio Options"), OPALIGN);
	row++;

	addIGTextButton(INTINGAMEOP_MOUSEOPTIONS, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Mouse Options"), OPALIGN);
	row++;

	// Key map editor does not allow editing for true multiplayer
	addIGTextButton(INTINGAMEOP_KEYMAP, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, (bMultiPlayer && NetPlay.bComms) ? _("View Key Mappings") : _("Key Mappings"), OPALIGN);
	row++;

	addIGTextButton(INTINGAMEOP_MUSICMANAGER, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Music Manager"), OPALIGN);
	row++;

	addIGTextButton(INTINGAMEOP_GO_BACK, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Go Back"), OPALIGN);
	row++;

	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Resume Game"), OPALIGN);

	ingameOp->setCalcLayout([row](WIDGET *psWidget) {
		psWidget->setGeometry(INTINGAMEOP_X, INTINGAMEOPAUTO_Y(row), INTINGAMEOP_W, INTINGAMEOPAUTO_H(row));
	});

	return true;
}

// Graphics Options
static bool startIGGraphicsOptionsMenu()
{
	widgDelete(psWScreen, INTINGAMEOP);  // get rid of the old stuff.
	auto const &parent = psWScreen->psForm;

	// add form
	auto ingameOp = std::make_shared<IntFormAnimated>();
	parent->attach(ingameOp);
	ingameOp->id = INTINGAMEOP;

	int row = 1;
	// FMV mode.
	addIGTextButton(INTINGAMEOP_FMVMODE,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Video Playback"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_FMVMODE_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, graphicsOptionsFmvmodeString(), WBUT_PLAIN);
	row++;

	// Scanlines
	addIGTextButton(INTINGAMEOP_SCANLINES,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Scanlines"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_SCANLINES_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, graphicsOptionsScanlinesString(), WBUT_PLAIN);
	row++;

	// Shadows
	addIGTextButton(INTINGAMEOP_SHADOWS,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Shadows"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_SHADOWS_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, graphicsOptionsShadowsString(), WBUT_PLAIN);
	row++;

	// Radar
	addIGTextButton(INTINGAMEOP_RADAR,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Radar"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_RADAR_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, graphicsOptionsRadarString(), WBUT_PLAIN);
	row++;

	// RadarJump
	addIGTextButton(INTINGAMEOP_RADAR_JUMP,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Radar Jump"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_RADAR_JUMP_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, graphicsOptionsRadarJumpString(), WBUT_PLAIN);
	row++;

	// Screen shake
	addIGTextButton(INTINGAMEOP_SCREENSHAKE,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Shake"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_SCREENSHAKE_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, graphicsOptionsScreenShakeString(), WBUT_PLAIN);
	row++;

	addIGTextButton(INTINGAMEOP_GO_BACK, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_SW_W, _("Go Back"), OPALIGN);
	row++;

	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_SW_W, _("Resume Game"), OPALIGN);

	ingameOp->setCalcLayout([row](WIDGET *psWidget) {
		psWidget->setGeometry(INTINGAMEOP2_X, INTINGAMEOPAUTO_Y(row), INTINGAMEOP2_W, INTINGAMEOPAUTO_H(row));
	});

	return true;
}

static bool runIGGraphicsOptionsMenu(UDWORD id)
{
	switch (id)
	{
	case INTINGAMEOP_SHADOWS:
	case INTINGAMEOP_SHADOWS_R:
		setDrawShadows(!getDrawShadows());
		widgSetString(psWScreen, INTINGAMEOP_SHADOWS_R, graphicsOptionsShadowsString());
		break;

	case INTINGAMEOP_FMVMODE:
	case INTINGAMEOP_FMVMODE_R:
		seqFMVmode();
		widgSetString(psWScreen, INTINGAMEOP_FMVMODE_R, graphicsOptionsFmvmodeString());
		break;

	case INTINGAMEOP_SCANLINES:
	case INTINGAMEOP_SCANLINES_R:
		seqScanlineMode();
		widgSetString(psWScreen, INTINGAMEOP_SCANLINES_R, graphicsOptionsScanlinesString());
		break;

	case INTINGAMEOP_RADAR:
	case INTINGAMEOP_RADAR_R:
		rotateRadar = !rotateRadar;
		widgSetString(psWScreen, INTINGAMEOP_RADAR_R, graphicsOptionsRadarString());
		break;

	case INTINGAMEOP_RADAR_JUMP:
	case INTINGAMEOP_RADAR_JUMP_R:
		war_SetRadarJump(!war_GetRadarJump());
		widgSetString(psWScreen, INTINGAMEOP_RADAR_JUMP_R, graphicsOptionsRadarJumpString());
		break;

	case INTINGAMEOP_SCREENSHAKE:
	case INTINGAMEOP_SCREENSHAKE_R:
		setShakeStatus(!getShakeStatus());
		widgSetString(psWScreen, INTINGAMEOP_SCREENSHAKE_R, graphicsOptionsScreenShakeString());
		break;

	case INTINGAMEOP_GO_BACK:
		intReopenMenuWithoutUnPausing();
		break;

	case INTINGAMEOP_RESUME:			//resume was pressed.
		return true;

	default:
		break;
	}

	return false;
}

// Video Options
static void refreshCurrentIGVideoOptionsValues()
{
	widgSetString(psWScreen, INTINGAMEOP_VSYNC_R, videoOptionsVsyncString());
	if (widgGetFromID(psWScreen, INTINGAMEOP_DISPLAYSCALE_R)) // Display Scale option may not be available
	{
		widgSetString(psWScreen, INTINGAMEOP_DISPLAYSCALE_R, videoOptionsDisplayScaleString().c_str());
	}
}

static bool startIGVideoOptionsMenu()
{
	widgDelete(psWScreen, INTINGAMEOP);  // get rid of the old stuff.
	auto const &parent = psWScreen->psForm;

	// add form
	auto ingameOp = std::make_shared<IntFormAnimated>();
	parent->attach(ingameOp);
	ingameOp->id = INTINGAMEOP;

	int row = 1;
	// Fullscreen/windowed can't be changed during game
	// Resolution can't be changed during game
	// Texture size can't be changed during game

	// Vsync
	addIGTextButton(INTINGAMEOP_VSYNC, INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Vertical sync"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_VSYNC_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, videoOptionsVsyncString(), WBUT_PLAIN);
	row++;

	// Antialiasing can't be changed during game

	// Display Scale
	if (wzAvailableDisplayScales().size() > 1)
	{
		addIGTextButton(INTINGAMEOP_DISPLAYSCALE, INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, videoOptionsDisplayScaleLabel(), WBUT_PLAIN);
		addIGTextButton(INTINGAMEOP_DISPLAYSCALE_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, videoOptionsDisplayScaleString().c_str(), WBUT_PLAIN);
		row++;
	}

	addIGTextButton(INTINGAMEOP_GO_BACK, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_SW_W, _("Go Back"), OPALIGN);
	row++;

	// Quit/return
	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_SW_W, _("Resume Game"), OPALIGN);

	ingameOp->setCalcLayout([row](WIDGET *psWidget) {
		psWidget->setGeometry(INTINGAMEOP2_X, INTINGAMEOPAUTO_Y(row), INTINGAMEOP2_W, INTINGAMEOPAUTO_H(row));
	});

	refreshCurrentIGVideoOptionsValues();

	return true;
}

static bool runIGVideoOptionsMenu(UDWORD id)
{
	switch (id)
	{
	case INTINGAMEOP_VSYNC:
	case INTINGAMEOP_VSYNC_R:
		seqVsyncMode();

		// Update the widget(s)
		refreshCurrentIGVideoOptionsValues();
		break;

	case INTINGAMEOP_DISPLAYSCALE:
	case INTINGAMEOP_DISPLAYSCALE_R:
		seqDisplayScale();

		// Update the widget(s)
		refreshCurrentIGVideoOptionsValues();
		break;

	case INTINGAMEOP_GO_BACK:
		intReopenMenuWithoutUnPausing();
		break;

	case INTINGAMEOP_RESUME:			//resume was pressed.
		return true;

	default:
		break;
	}

	return false;
}

// Mouse Options
static bool startIGMouseOptionsMenu()
{
	widgDelete(psWScreen, INTINGAMEOP);  // get rid of the old stuff.
	auto const &parent = psWScreen->psForm;

	// add form
	auto ingameOp = std::make_shared<IntFormAnimated>();
	parent->attach(ingameOp);
	ingameOp->id = INTINGAMEOP;

	int row = 1;
	// mouseflip
	addIGTextButton(INTINGAMEOP_MFLIP,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Reverse Rotation"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_MFLIP_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, mouseOptionsMflipString(), WBUT_PLAIN);
	row++;

	if (!bMultiPlayer || !NetPlay.bComms) // Cursor trapping does not work for true multiplayer
	{
		addIGTextButton(INTINGAMEOP_TRAP,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Trap Cursor"), WBUT_PLAIN);
		addIGTextButton(INTINGAMEOP_TRAP_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, mouseOptionsTrapString(), WBUT_PLAIN);
		row++;
	}

	// left-click orders
	addIGTextButton(INTINGAMEOP_MBUTTONS,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Switch Mouse Buttons"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_MBUTTONS_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, mouseOptionsMbuttonsString(), WBUT_PLAIN);
	row++;

	// middle-click rotate
	addIGTextButton(INTINGAMEOP_MMROTATE,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Rotate Screen"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_MMROTATE_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, mouseOptionsMmrotateString(), WBUT_PLAIN);
	row++;

	// Hardware / software cursor toggle
	addIGTextButton(INTINGAMEOP_CURSORMODE,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Colored Cursors"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_CURSORMODE_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, mouseOptionsCursorModeString(), WBUT_PLAIN);
	row++;

	addIGTextButton(INTINGAMEOP_GO_BACK, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_SW_W, _("Go Back"), OPALIGN);
	row++;

	// Quit/return
	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_SW_W, _("Resume Game"), OPALIGN);

	ingameOp->setCalcLayout([row](WIDGET *psWidget) {
		psWidget->setGeometry(INTINGAMEOP2_X, INTINGAMEOPAUTO_Y(row), INTINGAMEOP2_W, INTINGAMEOPAUTO_H(row));
	});

	return true;
}

static bool runIGMouseOptionsMenu(UDWORD id)
{
	switch (id)
	{
	case INTINGAMEOP_MFLIP:
	case INTINGAMEOP_MFLIP_R:
		setInvertMouseStatus(!getInvertMouseStatus());
		widgSetString(psWScreen, INTINGAMEOP_MFLIP_R, mouseOptionsMflipString());
		break;

	case INTINGAMEOP_TRAP:
	case INTINGAMEOP_TRAP_R:
		war_SetTrapCursor(!war_GetTrapCursor());
		widgSetString(psWScreen, INTINGAMEOP_TRAP_R, mouseOptionsTrapString());
		break;

	case INTINGAMEOP_MBUTTONS:
	case INTINGAMEOP_MBUTTONS_R:
		setRightClickOrders(!getRightClickOrders());
		widgSetString(psWScreen, INTINGAMEOP_MBUTTONS_R, mouseOptionsMbuttonsString());
		break;

	case INTINGAMEOP_MMROTATE:
	case INTINGAMEOP_MMROTATE_R:
		setMiddleClickRotate(!getMiddleClickRotate());
		widgSetString(psWScreen, INTINGAMEOP_MMROTATE_R, mouseOptionsMmrotateString());
		break;

	case INTINGAMEOP_CURSORMODE:
	case INTINGAMEOP_CURSORMODE_R:
		war_SetColouredCursor(!war_GetColouredCursor());
		widgSetString(psWScreen, INTINGAMEOP_CURSORMODE_R, mouseOptionsCursorModeString());
		wzSetCursor(CURSOR_DEFAULT);
		break;

	case INTINGAMEOP_GO_BACK:
		intReopenMenuWithoutUnPausing();
		break;

	case INTINGAMEOP_RESUME:			//resume was pressed.
		return true;

	default:
		break;
	}

	return false;
}

// ////////////////////////////////////////////////////////////////////////////
// process clicks made by user.
void intProcessInGameOptions(UDWORD id)
{
	if (isGraphicsOptionsUp)
	{
		if (runIGGraphicsOptionsMenu(id))
		{
			intCloseInGameOptions(true, true);
		}
		return;
	}
	else if (isVideoOptionsUp)
	{
		if (runIGVideoOptionsMenu(id))
		{
			intCloseInGameOptions(true, true);
		}
		return;
	}
	else if (isMouseOptionsUp)
	{
		if (runIGMouseOptionsMenu(id))
		{
			intCloseInGameOptions(true, true);
		}
		return;
	}
	else if (isKeyMapEditorUp)
	{
		if (runInGameKeyMapEditor(gInputManager, gKeyFuncConfig, id))
		{
			intCloseInGameOptions(true, true);
		}
		return;
	}
	else if (isMusicManagerUp)
	{
		if (runInGameMusicManager(id, gInputManager))
		{
			intCloseInGameOptions(true, true);
		}
		return;
	}
	else if (isInGameConfirmQuitUp)
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

	case INTINGAMEOP_OPTIONS:			//game options  was pressed
		startIGOptionsMenu();
		break;
	case INTINGAMEOP_GRAPHICSOPTIONS:
		startIGGraphicsOptionsMenu();
		isGraphicsOptionsUp = true;
		break;
	case INTINGAMEOP_VIDEOOPTIONS:
		startIGVideoOptionsMenu();
		isVideoOptionsUp = true;
		break;
	case INTINGAMEOP_AUDIOOPTIONS:
		addAudioOptions();
		break;
	case INTINGAMEOP_MOUSEOPTIONS:
		startIGMouseOptionsMenu();
		isMouseOptionsUp = true;
		break;
	case INTINGAMEOP_KEYMAP:			//keymap was pressed
		widgDelete(psWScreen, INTINGAMEOP);  // get rid of the old stuff.
		startInGameKeyMapEditor(gInputManager, gKeyFuncConfig, false);
		isKeyMapEditorUp = true;
		break;
	case INTINGAMEOP_MUSICMANAGER:
		widgDelete(psWScreen, INTINGAMEOP);  // get rid of the old stuff.
		startInGameMusicManager(gInputManager);
		isMusicManagerUp = true;
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
	// GAME OPTIONS KEYS
	case INTINGAMEOP_FXVOL:
	case INTINGAMEOP_3DFXVOL:
	case INTINGAMEOP_CDVOL:
		break;


	case INTINGAMEOP_FXVOL_S:
		sound_SetUIVolume((float)widgGetSliderPos(psWScreen, INTINGAMEOP_FXVOL_S) / 100.0f);
		break;
	case INTINGAMEOP_3DFXVOL_S:
		sound_SetEffectsVolume((float)widgGetSliderPos(psWScreen, INTINGAMEOP_3DFXVOL_S) / 100.0f);
		break;
	case INTINGAMEOP_CDVOL_S:
		sound_SetMusicVolume((float)widgGetSliderPos(psWScreen, INTINGAMEOP_CDVOL_S) / 100.0f);
		break;
	case INTINGAMEOP_SUBTITLES:
	case INTINGAMEOP_SUBTITLES_R:
		seq_SetSubtitles(!seq_GetSubtitles());
		widgSetString(psWScreen, INTINGAMEOP_SUBTITLES_R, graphicsOptionsSubtitlesString());
		break;

	case INTINGAMEOP_TUI_TARGET_ORIGIN_SW:
		tuiTargetOrigin = !tuiTargetOrigin;
		if (tuiTargetOrigin)
		{
			widgSetString(psWScreen, INTINGAMEOP_TUI_TARGET_ORIGIN_SW, _("Tactical UI (Target Origin Icon): Show"));
		}
		else
		{
			widgSetString(psWScreen, INTINGAMEOP_TUI_TARGET_ORIGIN_SW, _("Tactical UI (Target Origin Icon): Hide"));
		}

		break;

	default:
		break;
	}
}
