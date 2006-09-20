/*
 * inGameOp.c
 * ingame options screen.
 * Pumpkin Studios. 98
 */

#include "lib/framework/frame.h"
#include "lib/widget/widget.h"
#include "display3d.h"
#include "intdisplay.h"
#include "hci.h"			// for wFont def.& intmode.
#include "loop.h"
#include "text.h"
#include "lib/ivis_common/piestate.h"		// for getrendertype
#include "resource.h"
//#include "display.h"		// for gammaValue.
#include "frontend.h"		// for textdisplay function
#include "loadsave.h"		// for textdisplay function
#include "console.h"		// to add console message

#include "scriptextern.h"	// for tutorial
#include "lib/ivis_common/rendmode.h"
#include "keybind.h"

#include "lib/sound/audio.h"					// for sound.

#include "lib/sound/cdaudio.h"
#include "lib/sound/mixer.h"
#include "multiplay.h"


#include "csnap.h"
#include "ingameop.h"
#include "mission.h"
#include "transporter.h"
#include "lib/netplay/netplay.h"


extern char	SaveGamePath[];

//extern W_SCREEN *psWScreen;
extern CURSORSNAP InterfaceSnap;
extern void addText(int FontID,UDWORD FormID,UDWORD id,  UDWORD PosX, UDWORD PosY, STRING *txt, UDWORD attachID,BOOL *State);

//status bools.(for hci.h)
BOOL	ClosingInGameOp	= FALSE;
BOOL	InGameOpUp		= FALSE;

// ////////////////////////////////////////////////////////////////////////////
// functions

// ////////////////////////////////////////////////////////////////////////////


void AddMaxStringWidth(STR_RES *psRes, UDWORD StringID);
UDWORD GetMaxStringWidth(void);
void ResetMaxStringWidth(void);



static BOOL addQuitOptions(void)
{
	W_FORMINIT		sFormInit;
//	UWORD WindowWidth;

	DisableCursorSnapsExcept(INTINGAMEOP);

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
	addIGTextButton(INTINGAMEOP_RESUME,INTINGAMEOP_1_Y,STR_GAME_RESUME,OPALIGN);


	SetCurrentSnapID(&InterfaceSnap,INTINGAMEOP_RESUME);

	//  quit
	addIGTextButton(INTINGAMEOP_QUIT_CONFIRM,INTINGAMEOP_2_Y,STR_GAME_QUIT,OPALIGN);

	SetMousePos(0,INTINGAMEOP3_X+INTINGAMEOP_1_X,INTINGAMEOP3_Y+INTINGAMEOP_1_Y); // move mouse to resume.

	return TRUE;
}


static BOOL _addSlideOptions(void)
{
	W_FORMINIT		sFormInit;

	DisableCursorSnapsExcept(INTINGAMEOP);

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




	addIGTextButton(INTINGAMEOP_RESUME,INTINGAMEOP_4_Y,STR_GAME_RESUME,WBUT_PLAIN);

	// fx vol
	addIGTextButton(INTINGAMEOP_FXVOL,INTINGAMEOP_1_Y,STR_FE_FX,WBUT_PLAIN);
	addFESlider(INTINGAMEOP_FXVOL_S, INTINGAMEOP , INTINGAMEOP_MID, INTINGAMEOP_1_Y-5,
				AUDIO_VOL_MAX,mixer_GetWavVolume(),INTINGAMEOP_FXVOL);

	// fx vol
	addIGTextButton(INTINGAMEOP_3DFXVOL,INTINGAMEOP_2_Y,STR_FE_3D_FX,WBUT_PLAIN);
	addFESlider(INTINGAMEOP_3DFXVOL_S, INTINGAMEOP , INTINGAMEOP_MID, INTINGAMEOP_2_Y-5,
				AUDIO_VOL_MAX,mixer_Get3dWavVolume(),INTINGAMEOP_3DFXVOL);

	// cd vol
	addIGTextButton(INTINGAMEOP_CDVOL,INTINGAMEOP_3_Y,STR_FE_MUSIC,WBUT_PLAIN);
	addFESlider(INTINGAMEOP_CDVOL_S,INTINGAMEOP , INTINGAMEOP_MID,INTINGAMEOP_3_Y-5,
				AUDIO_VOL_MAX,mixer_GetCDVolume(),INTINGAMEOP_CDVOL);


	SetCurrentSnapID(&InterfaceSnap,INTINGAMEOP_RESUME);



	/*
#ifndef PSX
	// gamma
	if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		addIGTextButton(INTINGAMEOP_GAMMA,INTINGAMEOP_3_Y,STR_FE_GAMMA,WBUT_PLAIN);

		if(gammaValue>3)	   gammaValue = (float)2.9;
		if(gammaValue<0.5)  gammaValue = (float).5;

		addFESlider(INTINGAMEOP_GAMMA_S,INTINGAMEOP , INTINGAMEOP_MID,INTINGAMEOP_3_Y-5,60,(UDWORD)(gammaValue*25),INTINGAMEOP_GAMMA );

	}
#endif
*/

	return TRUE;
}


static BOOL addSlideOptions(void)
{

	return _addSlideOptions();
}

// ////////////////////////////////////////////////////////////////////////////

static UDWORD MaxStringWidth;

void ResetMaxStringWidth(void)
{
	MaxStringWidth=0;
}

void AddMaxStringWidth(STR_RES *psRes, UDWORD StringID)
{
	UDWORD StringWidth;
	StringWidth=iV_GetTextWidth(strresGetString(psRes,StringID));
	if (StringWidth>MaxStringWidth) MaxStringWidth=StringWidth;

}
UDWORD GetMaxStringWidth(void)
{
	return MaxStringWidth;
}



static BOOL _intAddInGameOptions(void)
{
//	UWORD WindowWidth;
	W_FORMINIT		sFormInit;


	audio_StopAll();

    //clear out any mission widgets - timers etc that may be on the screen
    clearMissionWidgets();


	setWidgetsStatus(TRUE);
	DisableCursorSnapsExcept(INTINGAMEOP);

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
		addIGTextButton(INTINGAMEOP_QUIT,INTINGAMEOP_5_Y,STR_GAME_QUIT,OPALIGN);

	}
	else
	{
		addIGTextButton(INTINGAMEOP_QUIT,INTINGAMEOP_3_Y,STR_GAME_QUIT,OPALIGN);
	}

	// add 'resume'
	addIGTextButton(INTINGAMEOP_RESUME,INTINGAMEOP_1_Y,STR_GAME_RESUME,OPALIGN);

	SetCurrentSnapID(&InterfaceSnap,INTINGAMEOP_RESUME);

	// add 'options'
	addIGTextButton(INTINGAMEOP_OPTIONS,INTINGAMEOP_2_Y,STR_FE_OPTIONS,OPALIGN);


	if ( (!bMultiPlayer || (NetPlay.bComms==0) )  && !bInTutorial)
	{		// add 'load'
		addIGTextButton(INTINGAMEOP_LOAD,INTINGAMEOP_3_Y,STR_MISC_LOADGAME,OPALIGN);
		// add 'save'
		addIGTextButton(INTINGAMEOP_SAVE,INTINGAMEOP_4_Y,STR_MISC_SAVEGAME,OPALIGN);
	}


	intMode		= INT_INGAMEOP;			// change interface mode.
	InGameOpUp	= TRUE;					// inform interface.
	SetMousePos(0,INTINGAMEOP_X+INTINGAMEOP_1_X,INTINGAMEOP_Y+INTINGAMEOP_1_Y); // move mouse to resume.

	pie_SetMouse(IntImages,IMAGE_CURSOR_DEFAULT);			// reset cursor (hw)
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


	EnableAllCursorSnaps();
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
			Form->pUserData		 = (void*)0;	// Used to signal when the close anim has finished.
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
// In Game Options house keeping stuff.
BOOL intRunInGameOptions(void)
{

	processFrontendSnap(FALSE);

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
//			strcpy(saveGameName, "replay\\replay.gam");
//			addConsoleMessage(strresGetString(psStringRes, STR_GAME_SAVED), LEFT_JUSTIFY);
//		}
//		break;
	case INTINGAMEOP_LOAD:
		intCloseInGameOptions(TRUE, FALSE);
		addLoadSave(LOAD_INGAME,SaveGamePath,"gam",strresGetString(psStringRes,STR_MR_LOAD_GAME));	// change mode when loadsave returns//		if(runLoadSave())// check for file name.
		break;
	case INTINGAMEOP_SAVE:
		intCloseInGameOptions(TRUE, FALSE);
		addLoadSave(SAVE_INGAME,SaveGamePath,"gam", strresGetString(psStringRes,STR_MR_SAVE_GAME) );
		break;


	// GAME OPTIONS KEYS
	case INTINGAMEOP_FXVOL:
	case INTINGAMEOP_3DFXVOL:
	case INTINGAMEOP_CDVOL:
//	case INTINGAMEOP_GAMMA:
		SetMousePos(0,INTINGAMEOP2_X+INTINGAMEOP_MID+5 ,mouseY());	// move mouse
		break;


	case INTINGAMEOP_FXVOL_S:
		mixer_SetWavVolume(widgGetSliderPos(psWScreen,INTINGAMEOP_FXVOL_S));
		break;
	case INTINGAMEOP_3DFXVOL_S:
		mixer_Set3dWavVolume(widgGetSliderPos(psWScreen,INTINGAMEOP_3DFXVOL_S));
		break;
	case INTINGAMEOP_CDVOL_S:
		mixer_SetCDVolume(widgGetSliderPos(psWScreen,INTINGAMEOP_CDVOL_S));
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

