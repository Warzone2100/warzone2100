/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
#include "lib/framework/strres.h"
#include "lib/widget/widget.h"
#include "lib/netplay/netplay.h"
#include "lib/ivis_common/piestate.h"		// for getrendertype
#include "lib/ivis_common/rendmode.h"
#include "lib/sound/audio.h"					// for sound.
#include "lib/sound/cdaudio.h"
#include "lib/sound/mixer.h"

#include "display3d.h"
#include "intdisplay.h"
#include "hci.h"			// for wFont def.& intmode.
#include "loop.h"
#include "resource.h"
#include "frontend.h"		// for textdisplay function
#include "loadsave.h"		// for textdisplay function
#include "console.h"		// to add console message
#include "scriptextern.h"	// for tutorial
#include "keybind.h"
#include "multiplay.h"
#include "ingameop.h"
#include "mission.h"
#include "transporter.h"
#include "main.h"

//status bools.(for hci.h)
BOOL	ClosingInGameOp	= FALSE;
BOOL	InGameOpUp		= FALSE;

// ////////////////////////////////////////////////////////////////////////////
// functions

// ////////////////////////////////////////////////////////////////////////////

static BOOL addIGTextButton(UDWORD id, UWORD y, const char *string, UDWORD Style)
{
	W_BUTINIT sButInit;

	memset( &sButInit, 0, sizeof(W_BUTINIT) );

	//resume
	sButInit.formID		= INTINGAMEOP;
	sButInit.id			= id;
	sButInit.style		= Style;


	sButInit.x			= INTINGAMEOP_1_X;
	sButInit.y			= y;
	sButInit.width		= INTINGAMEOP_OP_W;
	sButInit.height		= INTINGAMEOP_OP_H;

	sButInit.FontID		= font_regular;
	sButInit.pDisplay	= displayTextOption;
	sButInit.pText		= string;
	widgAddButton(psWScreen, &sButInit);

	return TRUE;
}

static BOOL addQuitOptions(void)
{
	W_FORMINIT		sFormInit;
//	UWORD WindowWidth;

	if (widgGetFromID(psWScreen,INTINGAMEOP))
	{
		widgDelete(psWScreen, INTINGAMEOP);		// get rid of the old stuff.
	}

	memset(&sFormInit,0, sizeof(W_FORMINIT));


	sFormInit.width		= INTINGAMEOP3_W;

	// add form
	sFormInit.formID	= 0;
	sFormInit.id		= INTINGAMEOP;
	sFormInit.style		= WFORM_PLAIN;
	sFormInit.x		= (SWORD)INTINGAMEOP3_X;
	sFormInit.y		= (SWORD)INTINGAMEOP3_Y;
	sFormInit.height	= INTINGAMEOP3_H;

	sFormInit.pDisplay	= intOpenPlainForm;
	sFormInit.disableChildren= TRUE;

	widgAddForm(psWScreen, &sFormInit);

	//resume
	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_Y, _("Resume Game"), OPALIGN);

	//  quit
	addIGTextButton(INTINGAMEOP_QUIT_CONFIRM, INTINGAMEOP_2_Y, _("Quit"), OPALIGN);

	return TRUE;
}


static BOOL addSlideOptions(void)
{
	W_FORMINIT		sFormInit;

	if (widgGetFromID(psWScreen,INTINGAMEOP))
	{
		widgDelete(psWScreen, INTINGAMEOP);		// get rid of the old stuff.
	}

	memset(&sFormInit,0, sizeof(W_FORMINIT));

	// add form
	sFormInit.formID	= 0;
	sFormInit.id		= INTINGAMEOP;
	sFormInit.style		= WFORM_PLAIN;
	sFormInit.x		= (SWORD)INTINGAMEOP2_X;
	sFormInit.y		= (SWORD)INTINGAMEOP2_Y;
	sFormInit.width		= INTINGAMEOP2_W;
	sFormInit.height	= INTINGAMEOP2_H;

	sFormInit.pDisplay	= intOpenPlainForm;
	sFormInit.disableChildren= TRUE;

	widgAddForm(psWScreen, &sFormInit);

	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_4_Y, _("Resume Game"), WBUT_PLAIN);

	// fx vol
	addIGTextButton(INTINGAMEOP_FXVOL, INTINGAMEOP_1_Y, _("Voice Volume"), WBUT_PLAIN);
	addFESlider(INTINGAMEOP_FXVOL_S, INTINGAMEOP, INTINGAMEOP_MID, INTINGAMEOP_1_Y-5,
				AUDIO_VOL_MAX, (int)(sound_GetUIVolume() * 100.0), INTINGAMEOP_FXVOL);

	// fx vol
	addIGTextButton(INTINGAMEOP_3DFXVOL, INTINGAMEOP_2_Y, _("FX Volume"), WBUT_PLAIN);
	addFESlider(INTINGAMEOP_3DFXVOL_S, INTINGAMEOP, INTINGAMEOP_MID, INTINGAMEOP_2_Y-5,
				AUDIO_VOL_MAX, (int)(sound_GetEffectsVolume() * 100.0), INTINGAMEOP_3DFXVOL);

	// cd vol
	addIGTextButton(INTINGAMEOP_CDVOL, INTINGAMEOP_3_Y, _("Music Volume"), WBUT_PLAIN);
	addFESlider(INTINGAMEOP_CDVOL_S, INTINGAMEOP, INTINGAMEOP_MID, INTINGAMEOP_3_Y-5,
				AUDIO_VOL_MAX, (int)(sound_GetMusicVolume() * 100), INTINGAMEOP_CDVOL);

	/*
	// gamma
	if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		addIGTextButton(INTINGAMEOP_GAMMA, INTINGAMEOP_3_Y, _("Gamma"), WBUT_PLAIN);

		if(gammaValue>3)	   gammaValue = (float)2.9;
		if(gammaValue<0.5)  gammaValue = (float).5;

		addFESlider(INTINGAMEOP_GAMMA_S,INTINGAMEOP , INTINGAMEOP_MID,INTINGAMEOP_3_Y-5,60,(UDWORD)(gammaValue*25),INTINGAMEOP_GAMMA );

	}
*/

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////

static BOOL _intAddInGameOptions(void)
{
//	UWORD WindowWidth;
	W_FORMINIT		sFormInit;


	audio_StopAll();

    //clear out any mission widgets - timers etc that may be on the screen
    clearMissionWidgets();


	setWidgetsStatus(TRUE);

	//if already open, then close!
	if (widgGetFromID(psWScreen,INTINGAMEOP))
	{
		intCloseInGameOptions(FALSE, TRUE);
		return TRUE;
	}

	intResetScreen(FALSE);


	// Pause the game.
	if(!gamePaused())
	{
		kf_TogglePauseMode();
	}


	memset(&sFormInit,0, sizeof(W_FORMINIT));



	sFormInit.width		= INTINGAMEOP_W;

	// add form
	sFormInit.formID	= 0;
	sFormInit.id		= INTINGAMEOP;
	sFormInit.style		= WFORM_PLAIN;
	sFormInit.x			= (SWORD)INTINGAMEOP_X;
	sFormInit.y			= (SWORD)INTINGAMEOP_Y;
	sFormInit.height	= INTINGAMEOP_H;


    if ( (!bMultiPlayer || (NetPlay.bComms==0) )  && !bInTutorial)
	{
	}
	else
	{
		sFormInit.height	= INTINGAMEOP_HS;
	}



	sFormInit.pDisplay	= intOpenPlainForm;
	sFormInit.disableChildren= TRUE;

	widgAddForm(psWScreen, &sFormInit);

	// add 'quit' text

    if ( (!bMultiPlayer || (NetPlay.bComms==0) )  && !bInTutorial)
	{
		addIGTextButton(INTINGAMEOP_QUIT, INTINGAMEOP_5_Y, _("Quit"), OPALIGN);

	}
	else
	{
		addIGTextButton(INTINGAMEOP_QUIT, INTINGAMEOP_3_Y, _("Quit"), OPALIGN);
	}

	// add 'resume'
	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_Y, _("Resume Game"), OPALIGN);

	// add 'options'
	addIGTextButton(INTINGAMEOP_OPTIONS, INTINGAMEOP_2_Y, _("Options"), OPALIGN);


	if ( (!bMultiPlayer || (NetPlay.bComms==0) )  && !bInTutorial)
	{		// add 'load'
		addIGTextButton(INTINGAMEOP_LOAD, INTINGAMEOP_3_Y, _("Load Game"), OPALIGN);
		// add 'save'
		addIGTextButton(INTINGAMEOP_SAVE, INTINGAMEOP_4_Y, _("Save Game"), OPALIGN);
	}


	intMode		= INT_INGAMEOP;			// change interface mode.
	InGameOpUp	= TRUE;					// inform interface.

	// TODO: find out if the line below is needed (may be for old csnap stuff)
	frameSetCursorFromRes(IDC_DEFAULT);						// reset cursor	(sw)

	return TRUE;
}


BOOL intAddInGameOptions(void)
{

	return _intAddInGameOptions();
}

// ////////////////////////////////////////////////////////////////////////////

static void ProcessOptionFinished(void)
{
	intMode		= INT_NORMAL;



	//unpause.
	if(gamePaused())
	{
		kf_TogglePauseMode();
	}


}

void intCloseInGameOptionsNoAnim(BOOL bResetMissionWidgets)
{
	widgDelete(psWScreen, INTINGAMEOP);
	InGameOpUp = FALSE;

	ProcessOptionFinished();

    //don't add the widgets if the load/save screen is put up or exiting to front end
    if (bResetMissionWidgets)
    {
        //put any widgets back on for the missions
        resetMissionWidgets();
    }
}


// ////////////////////////////////////////////////////////////////////////////
BOOL intCloseInGameOptions(BOOL bPutUpLoadSave, BOOL bResetMissionWidgets)
{

	W_TABFORM	*Form;
	WIDGET		*widg;

	if(bPutUpLoadSave)
	{
		widg = widgGetFromID(psWScreen,INTINGAMEOP);
		if(widg)
		{
			widgDelete(psWScreen,INTINGAMEOP);
		}

		InGameOpUp = FALSE;
		ClosingInGameOp = TRUE;
	}
	else
	{
		// close the form.
		// Start the window close animation.
		Form = (W_TABFORM*)widgGetFromID(psWScreen,INTINGAMEOP);
		if(Form) {
			Form->display		 = intClosePlainForm;
			Form->pUserData		 = NULL; // Used to signal when the close anim has finished.
			Form->disableChildren= TRUE;
			ClosingInGameOp		 = TRUE;		// like orderup/closingorder
			InGameOpUp			 = FALSE;
		}
	}

	ProcessOptionFinished();

    //don't add the widgets if the load/save screen is put up or exiting to front end
    if (bResetMissionWidgets)
    {
        //put any widgets back on for the missions
        resetMissionWidgets();
    }

    return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// process clicks made by user.
void intProcessInGameOptions(UDWORD id)
{


	switch(id)
	{
	// NORMAL KEYS
	case INTINGAMEOP_QUIT:				//quit was pressed
		addQuitOptions();
		break;

	case INTINGAMEOP_QUIT_CONFIRM:		//quit was confirmed.
		intCloseInGameOptions(FALSE, FALSE);
		break;

	case INTINGAMEOP_OPTIONS:			//game options  was pressed
		addSlideOptions();
		break;

	case INTINGAMEOP_RESUME:			//resume was pressed.
		intCloseInGameOptions(FALSE, TRUE);
		break;


//	case INTINGAMEOP_REPLAY:
//		intCloseInGameOptions(TRUE, FALSE);
//		if(0!=strcmp(getLevelName(),"CAM_1A"))
//		{
//			loopMissionState = LMS_LOADGAME;
//			strcpy(saveGameName, "replay/replay.gam");
//			addConsoleMessage(_("GAME SAVED!"), LEFT_JUSTIFY, CONSOLE_SYSTEM);
//		}
//		break;
	case INTINGAMEOP_LOAD:
		intCloseInGameOptions(TRUE, FALSE);
		addLoadSave(LOAD_INGAME,SaveGamePath,"gam",_("Load Saved Game"));	// change mode when loadsave returns//		if(runLoadSave())// check for file name.
		break;
	case INTINGAMEOP_SAVE:
		intCloseInGameOptions(TRUE, FALSE);
		addLoadSave(SAVE_INGAME,SaveGamePath,"gam", _("Save Game") );
		break;


	// GAME OPTIONS KEYS
	case INTINGAMEOP_FXVOL:
	case INTINGAMEOP_3DFXVOL:
	case INTINGAMEOP_CDVOL:
//	case INTINGAMEOP_GAMMA:
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

//	case INTINGAMEOP_GAMMA_S:
//		gammaValue = (float)(widgGetSliderPos(psWScreen,INTINGAMEOP_GAMMA_S))/25  ;
//		if(gammaValue<0.5)  gammaValue = (float).5;
//		pie_SetGammaValue(gammaValue);
//		break;


	default:
		break;
	}


}


