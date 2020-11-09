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

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/strres.h"
#include "lib/widget/widget.h"
#include "lib/widget/label.h"
#include "lib/netplay/netplay.h"
#include "lib/ivis_opengl/piestate.h"		// for getrendertype
#include "lib/sound/audio.h"					// for sound.
#include "lib/sound/cdaudio.h"
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
#include "transporter.h"
#include "main.h"
#include "warzoneconfig.h"
#include "qtscript.h"		// for bInTutorial
#include "radar.h"
#include "seqdisp.h"
#include "texture.h"

bool hostQuitConfirmation;

bool	InGameOpUp		= false;
bool 	isInGamePopupUp = false;
static bool 	isGraphicsOptionsUp = false;
static bool 	isVideoOptionsUp = false;
static bool 	isMouseOptionsUp = false;
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
	delete widgGetFromID(psWScreen, INTINGAMEPOPUP);
	delete widgGetFromID(psWScreen, INTINGAMEOP);

	WIDGET *parent = psWScreen->psForm;

	// add form
	auto inGameOp = new IntFormAnimated(parent);
	inGameOp->id = INTINGAMEOP;
	inGameOp->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(INTINGAMEOP3_X, INTINGAMEOP3_Y, INTINGAMEOP3_W, INTINGAMEOP3_H);
	}));

	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOP_1_Y, INTINGAMEOP_OP_W, _("Resume Game"), OPALIGN);

	auto inGamePopup = new IntFormAnimated(parent);
	inGamePopup->id = INTINGAMEPOPUP;
	inGamePopup->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		assert(psWScreen != nullptr);
		WIDGET * inGameOp = widgGetFromID(psWScreen, INTINGAMEOP);
		assert(inGameOp != nullptr);
		psWidget->setGeometry((pie_GetVideoBufferWidth() - 600)/2, inGameOp->y() - 26 - 20, 600, 26);
	}));

	auto label = new W_LABEL(inGamePopup);
	label->setGeometry(0, 0, inGamePopup->width(), inGamePopup->height());
	label->setString(_("WARNING: You're the host. If you quit, the game ends for everyone!"));
	label->setTextAlignment(WLAB_ALIGNCENTRE);
	label->setFont(font_medium, WZCOL_YELLOW);

	addIGTextButton(INTINGAMEOP_QUIT, INTINGAMEOP_1_X, INTINGAMEOP_2_Y, INTINGAMEOP_OP_W, _("Host Quit"), OPALIGN);

	return true;
}

static bool addAudioOptions()
{
	delete widgGetFromID(psWScreen, INTINGAMEOP);  // get rid of the old stuff.

	WIDGET *parent = psWScreen->psForm;

	// add form
	IntFormAnimated *ingameOp = new IntFormAnimated(parent);
	ingameOp->id = INTINGAMEOP;
#ifdef DEBUG
#define ROWS 5
#else
#define ROWS 4
#endif
	ingameOp->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(INTINGAMEOP2_X, INTINGAMEOPAUTO_Y(ROWS), INTINGAMEOP2_W, INTINGAMEOPAUTO_H(ROWS));
	}));
#undef ROWS

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

	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_SW_W, _("Resume Game"), OPALIGN);

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

	intResetScreen(false);


	// Pause the game.
	if (!gamePaused())
	{
		kf_TogglePauseMode();
	}

	WIDGET *parent = psWScreen->psForm;

	bool s = (bMultiPlayer && NetPlay.bComms != 0) || bInTutorial;

	// add form
	IntFormAnimated *ingameOp = new IntFormAnimated(parent);
	ingameOp->id = INTINGAMEOP;
	ingameOp->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		bool s = (bMultiPlayer && NetPlay.bComms != 0) || bInTutorial;
		psWidget->setGeometry(INTINGAMEOP_X, INTINGAMEOPAUTO_Y(s? 3 : 5), INTINGAMEOP_W, INTINGAMEOPAUTO_H(s? 3 : 5));
	}));

	int row = 1;
	// add 'resume'
	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Resume Game"), OPALIGN);
	row++;

	// add 'options'
	addIGTextButton(INTINGAMEOP_OPTIONS, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Options"), OPALIGN);
	row++;

	if (!s)
	{
		if (!bMultiPlayer)
		{
			// add 'load'
			addIGTextButton(INTINGAMEOP_LOAD_MISSION, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Load Game"), OPALIGN);
			row++;
			// add 'save'
			addIGTextButton(INTINGAMEOP_SAVE_MISSION, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Save Game"), OPALIGN);
			row++;
		}
		else
		{
			// add 'load'
			addIGTextButton(INTINGAMEOP_LOAD_SKIRMISH, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Load Game"), OPALIGN);
			row++;
			// add 'save'
			addIGTextButton(INTINGAMEOP_SAVE_SKIRMISH, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Save Game"), OPALIGN);
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
		addIGTextButton(INTINGAMEOP_QUIT, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Quit"), OPALIGN);
	}

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

	audio_StopAll();

	if (!gamePaused())
	{
		kf_TogglePauseMode();	// Pause the game.
	}

	WIDGET *parent = psWScreen->psForm;

	IntFormAnimated *ingamePopup = new IntFormAnimated(parent);
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



	//unpause.
	if (gamePaused())
	{
		kf_TogglePauseMode();
	}


}

void intCloseInGameOptionsNoAnim(bool bResetMissionWidgets)
{
	if (NetPlay.isHost)
	{
		widgDelete(psWScreen, INTINGAMEPOPUP);
	}
	widgDelete(psWScreen, INTINGAMEOP);
	InGameOpUp = false;

	ProcessOptionFinished();

	//don't add the widgets if the load/save screen is put up or exiting to front end
	if (bResetMissionWidgets)
	{
		//put any widgets back on for the missions
		resetMissionWidgets();
	}
}


// ////////////////////////////////////////////////////////////////////////////
bool intCloseInGameOptions(bool bPutUpLoadSave, bool bResetMissionWidgets)
{
	bool keymapWasUp = isKeyMapEditorUp;

	isGraphicsOptionsUp = false;
	isVideoOptionsUp = false;
	isMouseOptionsUp = false;
	if (isKeyMapEditorUp)
	{
		runInGameKeyMapEditor(KM_RETURN);
	}
	isKeyMapEditorUp = false;
	if (isMusicManagerUp)
	{
		runInGameMusicManager(MM_RETURN);
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
		IntFormAnimated *form;
		if (isInGamePopupUp)	// FIXME: we hijack this routine for the popup close.
		{
			form = (IntFormAnimated *)widgGetFromID(psWScreen, INTINGAMEPOPUP);
			isInGamePopupUp = false;
		}
		else
		{
			form = (IntFormAnimated *)widgGetFromID(psWScreen, INTINGAMEOP);
		}

		if (form)
		{
			form->closeAnimateDelete();
			InGameOpUp			 = false;
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

static bool startIGOptionsMenu()
{
	delete widgGetFromID(psWScreen, INTINGAMEOP);  // get rid of the old stuff.
	WIDGET *parent = psWScreen->psForm;
	isGraphicsOptionsUp = false;
	isVideoOptionsUp = false;
	isMouseOptionsUp = false;
	isKeyMapEditorUp = false;
	isMusicManagerUp = false;

	// add form
	IntFormAnimated *ingameOp = new IntFormAnimated(parent);
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

	bool s = bMultiPlayer && NetPlay.bComms != 0;
	// Key map editor does not allow editing for true multiplayer
	addIGTextButton(INTINGAMEOP_KEYMAP, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, s? _("View Key Mappings") : _("Key Mappings"), OPALIGN);
	row++;

	addIGTextButton(INTINGAMEOP_MUSICMANAGER, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Music Manager"), OPALIGN);
	row++;

	ingameOp->setCalcLayout([row](WIDGET *psWidget, unsigned int, unsigned int, unsigned int, unsigned int){
		psWidget->setGeometry(INTINGAMEOP_X, INTINGAMEOPAUTO_Y(row), INTINGAMEOP_W, INTINGAMEOPAUTO_H(row));
	});

	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Resume Game"), OPALIGN);

	return true;
}

// Graphics Options
static bool startIGGraphicsOptionsMenu()
{
	delete widgGetFromID(psWScreen, INTINGAMEOP);  // get rid of the old stuff.
	WIDGET *parent = psWScreen->psForm;

	// add form
	IntFormAnimated *ingameOp = new IntFormAnimated(parent);
	ingameOp->id = INTINGAMEOP;
	ingameOp->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(INTINGAMEOP2_X, INTINGAMEOPAUTO_Y(7), INTINGAMEOP2_W, INTINGAMEOPAUTO_H(7));
	}));

	int row = 1;
	// FMV mode.
	addIGTextButton(INTINGAMEOP_FMVMODE,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Video Playback"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_FMVMODE_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, graphicsOptionsFmvmodeString(), WBUT_PLAIN);
	row++;

	// Scanlines
	addIGTextButton(INTINGAMEOP_SCANLINES,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Scanlines"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_SCANLINES_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, graphicsOptionsScanlinesString(), WBUT_PLAIN);
	row++;

	// Subtitles
	addIGTextButton(INTINGAMEOP_SUBTITLES,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Subtitles"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_SUBTITLES_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, graphicsOptionsSubtitlesString(), WBUT_PLAIN);
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

	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_SW_W, _("Resume Game"), OPALIGN);

	return true;
}

static bool runIGGraphicsOptionsMenu(UDWORD id)
{
	switch (id)
	{
	case INTINGAMEOP_SUBTITLES:
	case INTINGAMEOP_SUBTITLES_R:
		seq_SetSubtitles(!seq_GetSubtitles());
		widgSetString(psWScreen, INTINGAMEOP_SUBTITLES_R, graphicsOptionsSubtitlesString());
		break;

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
	delete widgGetFromID(psWScreen, INTINGAMEOP);  // get rid of the old stuff.
	WIDGET *parent = psWScreen->psForm;

	// add form
	IntFormAnimated *ingameOp = new IntFormAnimated(parent);
	ingameOp->id = INTINGAMEOP;
	ingameOp->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		bool showDisplayScales = wzAvailableDisplayScales().size() > 1;
		psWidget->setGeometry(INTINGAMEOP2_X, INTINGAMEOPAUTO_Y(showDisplayScales? 3 : 2), INTINGAMEOP2_W, INTINGAMEOPAUTO_H(showDisplayScales? 3 : 2));
	}));

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

	// Quit/return
	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_SW_W, _("Resume Game"), OPALIGN);

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
	delete widgGetFromID(psWScreen, INTINGAMEOP);  // get rid of the old stuff.
	WIDGET *parent = psWScreen->psForm;

	// add form
	IntFormAnimated *ingameOp = new IntFormAnimated(parent);
	ingameOp->id = INTINGAMEOP;
	ingameOp->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		bool s = bMultiPlayer && NetPlay.bComms != 0;
		psWidget->setGeometry(INTINGAMEOP2_X, INTINGAMEOPAUTO_Y(s ? 6 : 7), INTINGAMEOP2_W, INTINGAMEOPAUTO_H(s ? 6 : 7));
	}));

	int row = 1;
	// mouseflip
	addIGTextButton(INTINGAMEOP_MFLIP,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Reverse Rotation"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_MFLIP_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, mouseOptionsMflipString(), WBUT_PLAIN);
	row++;

	bool s = bMultiPlayer && NetPlay.bComms != 0;
	if (!s) // Cursor trapping does not work for true multiplayer
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

	// Function of the scroll wheel
	addIGTextButton(INTINGAMEOP_SCROLLEVENT,   INTINGAMEOP_2_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, _("Scroll Event"), WBUT_PLAIN);
	addIGTextButton(INTINGAMEOP_SCROLLEVENT_R, INTINGAMEOP_MID, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_OP_W, mouseOptionsScrollEventString(), WBUT_PLAIN);
	row++;

	// Quit/return
	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_X, INTINGAMEOPAUTO_Y_LINE(row), INTINGAMEOP_SW_W, _("Resume Game"), OPALIGN);

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

	case INTINGAMEOP_SCROLLEVENT:
	case INTINGAMEOP_SCROLLEVENT_R:
		seqScrollEvent();
		widgSetString(psWScreen, INTINGAMEOP_SCROLLEVENT_R, mouseOptionsScrollEventString());
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
		if (runInGameKeyMapEditor(id))
		{
			intCloseInGameOptions(true, true);
		}
		return;
	}
	else if (isMusicManagerUp)
	{
		if (runInGameMusicManager(id))
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
		delete widgGetFromID(psWScreen, INTINGAMEOP);  // get rid of the old stuff.
		startInGameKeyMapEditor(false);
		isKeyMapEditorUp = true;
		break;
	case INTINGAMEOP_MUSICMANAGER:
		delete widgGetFromID(psWScreen, INTINGAMEOP);  // get rid of the old stuff.
		startInGameMusicManager();
		isMusicManagerUp = true;
		break;
	case INTINGAMEOP_RESUME:			//resume was pressed.
		intCloseInGameOptions(false, true);
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
		sound_SetUIVolume((float)widgGetSliderPos(psWScreen, INTINGAMEOP_FXVOL_S) / 100.0);
		break;
	case INTINGAMEOP_3DFXVOL_S:
		sound_SetEffectsVolume((float)widgGetSliderPos(psWScreen, INTINGAMEOP_3DFXVOL_S) / 100.0);
		break;
	case INTINGAMEOP_CDVOL_S:
		sound_SetMusicVolume((float)widgGetSliderPos(psWScreen, INTINGAMEOP_CDVOL_S) / 100.0);
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
