/*
 * inGameOp.c
 * ingame options screen.
 * Pumpkin Studios. 98
 */

#include "frame.h"
#include "widget.h"
#include "Display3d.h"
#include "intDisplay.h"
#include "HCI.h"			// for wFont def.& intmode.
#include "loop.h"
#include "text.h"
#include "piestate.h"		// for getrendertype
#include "resource.h"
//#include "display.h"		// for gamma.
#include "frontend.h"		// for textdisplay function
#include "loadSave.h"		// for textdisplay function
#include "console.h"		// to add console message

#include "ScriptExtern.h"	// for tutorial 
#include "rendmode.h"
#include "keyBind.h"

#include "audio.h"					// for sound.
#ifdef WIN32
#include "cdaudio.h"
#include "mixer.h"
#include "multiplay.h"
#endif

#include "csnap.h"
#include "inGameOp.h"
#include "mission.h"
#include "transporter.h"
#include "netplay.h"
#ifdef PSX
#include "Primatives.h"
#include "ctrlpsx.h"
#include "vpad.h"
#include "dcache.h"
#include "initpsx.h"
#include "locale.h"

extern BOOL	DirectControl;
extern BOOL	EnableVibration;
extern char OnString[];
extern char OffString[];
extern BOOL bShakingPermitted;
#endif

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



static BOOL addQuitOptions(VOID)
{
	W_FORMINIT		sFormInit;		
	UWORD WindowWidth;	

	DisableCursorSnapsExcept(INTINGAMEOP);

	if (widgGetFromID(psWScreen,INTINGAMEOP))
	{
		widgDelete(psWScreen, INTINGAMEOP);		// get rid of the old stuff.
	}

	memset(&sFormInit,0, sizeof(W_FORMINIT));	

#ifdef PSX
	WidgSetOTIndex(OT2D_FORE);


/*

	Calc the max width of the strings used in the option menu  - so that it will work in any language

*/
	ResetMaxStringWidth();
	AddMaxStringWidth(psStringRes,STR_GAME_RESUME);
	AddMaxStringWidth(psStringRes,STR_GAME_QUIT);
	WindowWidth=GetMaxStringWidth()+40;
	if (WindowWidth<INTINGAMEOP3_W) WindowWidth=INTINGAMEOP3_W;	  // set the min value
	sFormInit.width=WindowWidth;

#else
	sFormInit.width		= INTINGAMEOP3_W;
#endif
	// add form
	sFormInit.formID	= 0;
	sFormInit.id		= INTINGAMEOP;
	sFormInit.style		= WFORM_PLAIN;
	sFormInit.x			= (SWORD)INTINGAMEOP3_X;
	sFormInit.y			= (SWORD)INTINGAMEOP3_Y;
	sFormInit.height	= INTINGAMEOP3_H;
#ifdef WIN32
	sFormInit.pDisplay	= intOpenPlainForm;
	sFormInit.disableChildren= TRUE;
#else
	sFormInit.pDisplay	= intDisplayPlainForm;
#endif
	widgAddForm(psWScreen, &sFormInit);

	//resume
	addIGTextButton(INTINGAMEOP_RESUME,INTINGAMEOP_1_Y,STR_GAME_RESUME,OPALIGN);


	SetCurrentSnapID(&InterfaceSnap,INTINGAMEOP_RESUME);

	//  quit
	addIGTextButton(INTINGAMEOP_QUIT_CONFIRM,INTINGAMEOP_2_Y,STR_GAME_QUIT,OPALIGN);
	
	SetMousePos(0,INTINGAMEOP3_X+INTINGAMEOP_1_X,INTINGAMEOP3_Y+INTINGAMEOP_1_Y); // move mouse to resume.

	return TRUE;
}


static BOOL _addSlideOptions()
{
	W_FORMINIT		sFormInit;			
#ifdef PSX
	char *StateString;
	int bx;
#endif
	DisableCursorSnapsExcept(INTINGAMEOP);

	if (widgGetFromID(psWScreen,INTINGAMEOP))
	{
		widgDelete(psWScreen, INTINGAMEOP);		// get rid of the old stuff.
	}

	memset(&sFormInit,0, sizeof(W_FORMINIT));	

#ifdef PSX
	WidgSetOTIndex(OT2D_FARFORE);
#endif
	// add form
	sFormInit.formID	= 0;
	sFormInit.id		= INTINGAMEOP;
	sFormInit.style		= WFORM_PLAIN;
	sFormInit.x			= (SWORD)INTINGAMEOP2_X;
	sFormInit.y			= (SWORD)INTINGAMEOP2_Y;
	sFormInit.width		= INTINGAMEOP2_W;
	sFormInit.height	= INTINGAMEOP2_H;
#ifdef WIN32
	sFormInit.pDisplay	= intOpenPlainForm;
	sFormInit.disableChildren= TRUE;
#else
	sFormInit.pDisplay	= intDisplayPlainForm;
#endif
	widgAddForm(psWScreen, &sFormInit);

#ifdef PSX
	addIGTextButton(FRONTEND_CENTRESCREEN,INTINGAMEOP_7_Y,STR_FE_CENTRESCREEN,WBUT_PLAIN);
	addCentreScreen(INTINGAMEOP,
					INTINGAMEOP_1_X+iV_GetTextWidth(strresGetString(psStringRes, STR_FE_CENTRESCREEN)) + 16,
					INTINGAMEOP_7_Y-5);	//-10);
	addIGTextButton(INTINGAMEOP_RESUME,INTINGAMEOP_8_Y,STR_GAME_RESUME,WBUT_PLAIN);

	// fx vol
	addIGTextButton(INTINGAMEOP_FXVOL,INTINGAMEOP_1_Y,STR_FE_FX,WBUT_PLAIN);
	// cd vol
	addIGTextButton(INTINGAMEOP_CDVOL,INTINGAMEOP_2_Y,STR_FE_MUSIC,WBUT_PLAIN);
	// cursor speed
	addIGTextButton(INTINGAMEOP_CURSOR_S,INTINGAMEOP_3_Y,STR_FE_CURSORSPEED,WBUT_PLAIN);

	bx = INTINGAMEOP_1_X+128;
	{
		if( (GetCurrentLanguage() == LANGUAGE_SPANISH) ||
			(GetCurrentLanguage() == LANGUAGE_ITALIAN) ) {
			bx += 64;
		}
	}

 #if COUNTRY == COUNTRY_GERMAN
	addFESlider(FRONTEND_FX_SL, INTINGAMEOP , INTINGAMEOP_1_X + 128+64, INTINGAMEOP_1_Y,	//-5,
				AUDIO_VOL_MAX/2,sound_GetGlobalVolume()/2,INTINGAMEOP_FXVOL);
	addFESlider(FRONTEND_MUSIC_SL,INTINGAMEOP , INTINGAMEOP_1_X + 128+64,INTINGAMEOP_2_Y,	//-5,
				AUDIO_VOL_MAX/2,cdAudio_GetVolume()/2,INTINGAMEOP_CDVOL);
	addFESlider(FRONTEND_CURSOR_SL,INTINGAMEOP , INTINGAMEOP_1_X + 128+64,INTINGAMEOP_3_Y,	//-5,
				getCursorSpeedRange(),getCursorSpeedModifier(),INTINGAMEOP_CURSOR_S);
 #else
	addFESlider(FRONTEND_FX_SL, INTINGAMEOP , bx, INTINGAMEOP_1_Y,	//-5,
				AUDIO_VOL_MAX/2,sound_GetGlobalVolume()/2,INTINGAMEOP_FXVOL);
	addFESlider(FRONTEND_MUSIC_SL,INTINGAMEOP , bx,INTINGAMEOP_2_Y,	//-5,
				AUDIO_VOL_MAX/2,cdAudio_GetVolume()/2,INTINGAMEOP_CDVOL);
	addFESlider(FRONTEND_CURSOR_SL,INTINGAMEOP , bx,INTINGAMEOP_3_Y,	//-5,
				getCursorSpeedRange(),getCursorSpeedModifier(),INTINGAMEOP_CURSOR_S);
 #endif

#endif

#ifdef WIN32
	addIGTextButton(INTINGAMEOP_RESUME,INTINGAMEOP_3_Y,STR_GAME_RESUME,WBUT_PLAIN);

	// fx vol
	addIGTextButton(INTINGAMEOP_FXVOL,INTINGAMEOP_1_Y,STR_FE_FX,WBUT_PLAIN);
	addFESlider(INTINGAMEOP_FXVOL_S, INTINGAMEOP , INTINGAMEOP_MID, INTINGAMEOP_1_Y-5,
				AUDIO_VOL_MAX,sound_GetGlobalVolume(),INTINGAMEOP_FXVOL);

	// cd vol
	addIGTextButton(INTINGAMEOP_CDVOL,INTINGAMEOP_2_Y,STR_FE_MUSIC,WBUT_PLAIN);
	addFESlider(INTINGAMEOP_CDVOL_S,INTINGAMEOP , INTINGAMEOP_MID,INTINGAMEOP_2_Y-5,
				AUDIO_VOL_MAX,mixer_GetCDVolume(),INTINGAMEOP_CDVOL);
#endif

	SetCurrentSnapID(&InterfaceSnap,INTINGAMEOP_RESUME);

#ifdef PSX
	//Screen shake?
	addIGTextButton(FRONTEND_SCREENSHAKE,INTINGAMEOP_4_Y,STR_FE_SCREENSHAKE,WBUT_PLAIN);
	if(bShakingPermitted) {
		StateString = OnString;
	} else {
		StateString = OffString;
	}
 #if COUNTRY == COUNTRY_FRENCH
	addText(WFont,INTINGAMEOP,INTINGAMEOP_SCREENSHAKE_BT, INTINGAMEOP_1_X+128+32,INTINGAMEOP_4_Y-9,	//14,
			StateString,FRONTEND_SCREENSHAKE,&bShakingPermitted);
 #else
	bx = INTINGAMEOP_1_X+128;
	{
		if( GetCurrentLanguage() == LANGUAGE_SPANISH) {
			bx += 96;
		}
	}
	addText(WFont,INTINGAMEOP,INTINGAMEOP_SCREENSHAKE_BT, bx,INTINGAMEOP_4_Y-9,	//14,
			StateString,FRONTEND_SCREENSHAKE,&bShakingPermitted);
 #endif

	//Screen shake?
	addIGTextButton(FRONTEND_SUBTITLES,INTINGAMEOP_5_Y,STR_FE_SUBTITLES,WBUT_PLAIN);
	if(bSubtitles) {
		StateString = OnString;
	} else {
		StateString = OffString;
	}
 #if COUNTRY == COUNTRY_FRENCH
	addText(WFont,INTINGAMEOP,INTINGAMEOP_SUBTITLES_BT, INTINGAMEOP_1_X+128+32,INTINGAMEOP_5_Y-9,	//14,
			StateString,FRONTEND_SUBTITLES,&bSubtitles);
 #else
	addText(WFont,INTINGAMEOP,INTINGAMEOP_SUBTITLES_BT, INTINGAMEOP_1_X+128,INTINGAMEOP_5_Y-9,	//14,
			StateString,FRONTEND_SUBTITLES,&bSubtitles);
 #endif

 #ifdef LIBPAD

	if( (GetProtocolType(0) == PADPROT_EXPANDED) ) {
		//Dual Shock Vibration?
		addIGTextButton(FRONTEND_VIBRO,INTINGAMEOP_6_Y,STR_FE_VIBRATION,WBUT_PLAIN);
		if(EnableVibration) {
			StateString = OnString;
		} else {
			StateString = OffString;
		}
 #if COUNTRY == COUNTRY_FRENCH
		addText(WFont,INTINGAMEOP,INTINGAMEOP_VIBRATION_BT, INTINGAMEOP_1_X+128+32,INTINGAMEOP_6_Y-9,	//-14,
				StateString,FRONTEND_VIBRO,&EnableVibration);
 #else
		addText(WFont,INTINGAMEOP,INTINGAMEOP_VIBRATION_BT, INTINGAMEOP_1_X+128,INTINGAMEOP_6_Y-9,	//-14,
				StateString,FRONTEND_VIBRO,&EnableVibration);
 #endif
		intSetVibroOnID(INTINGAMEOP_VIBRATION);
	}

 #endif
#endif

	/*
#ifdef WIN32
	// gamma
	if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		addIGTextButton(INTINGAMEOP_GAMMA,INTINGAMEOP_3_Y,STR_FE_GAMMA,WBUT_PLAIN);
		
		if(gamma>3)	   gamma = (float)2.9;
		if(gamma<0.5)  gamma = (float).5;
	
		addFESlider(INTINGAMEOP_GAMMA_S,INTINGAMEOP , INTINGAMEOP_MID,INTINGAMEOP_3_Y-5,60,(UDWORD)(gamma*25),INTINGAMEOP_GAMMA );

	}
#endif
*/

	return TRUE;
}


static BOOL addSlideOptions(void)
{
#ifdef PSX
	// If the stacks in the dcache then..
	if(SpInDCache()) {
		static BOOL ret;

		// Set the stack pointer to point to the alternative stack which is'nt limited to 1k.
		SetSpAlt();
		ret = _addSlideOptions();
		SetSpAltNormal();

		return ret;
	}
#endif
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
	UWORD WindowWidth;
	W_FORMINIT		sFormInit;			

#ifdef WIN32
	audio_StopAll();

    //clear out any mission widgets - timers etc that may be on the screen
    clearMissionWidgets();
#endif
	
	setWidgetsStatus(TRUE);
	DisableCursorSnapsExcept(INTINGAMEOP);

	//if already open, then close!
	if (widgGetFromID(psWScreen,INTINGAMEOP))
	{
		intCloseInGameOptions(FALSE, TRUE);
		return TRUE;
	}

	intResetScreen(FALSE);

#ifdef PSX
	// Store current option settings in case they cancel.
	StoreGameOptions();
	setGamePauseStatus(TRUE);		
	gameTimeStop();
	if(driveModeActive()) {
		driveDisableControl();
	}
	// Enable interface snap.
	StartInterfaceSnap();
#else
	// Pause the game.
	if(!gamePaused())
	{
		kf_TogglePauseMode();
	}
#endif

	memset(&sFormInit,0, sizeof(W_FORMINIT));	

#ifdef PSX
	WidgSetOTIndex(OT2D_FORE);
/*

	Calc the max width of the strings used in the option menu  - so that it will work in any language

*/
	ResetMaxStringWidth();
	AddMaxStringWidth(psStringRes,STR_FE_OPTIONS);
	AddMaxStringWidth(psStringRes,STR_GAME_RESUME);
	AddMaxStringWidth(psStringRes,STR_GAME_QUIT);
	WindowWidth=GetMaxStringWidth()+40;
	if (WindowWidth<INTINGAMEOP_W) WindowWidth=INTINGAMEOP_W;	  // set the min value
	sFormInit.width=WindowWidth;
#else

	sFormInit.width		= INTINGAMEOP_W;
#endif
	// add form
	sFormInit.formID	= 0;
	sFormInit.id		= INTINGAMEOP;
	sFormInit.style		= WFORM_PLAIN;
	sFormInit.x			= (SWORD)INTINGAMEOP_X;
	sFormInit.y			= (SWORD)INTINGAMEOP_Y;
	sFormInit.height	= INTINGAMEOP_H;

#ifdef WIN32
    if ( (!bMultiPlayer || (NetPlay.bComms==0) )  && !bInTutorial)
	{
	}
	else
	{
		sFormInit.height	= INTINGAMEOP_HS;
	}
#endif

#ifdef WIN32	
	sFormInit.pDisplay	= intOpenPlainForm;
	sFormInit.disableChildren= TRUE;
#else
	sFormInit.pDisplay	= intDisplayPlainForm;
#endif
	widgAddForm(psWScreen, &sFormInit);

	// add 'quit' text
#ifdef WIN32
// #ifdef COVERMOUNT
 #if 0
	addIGTextButton(INTINGAMEOP_QUIT,INTINGAMEOP_3_Y,STR_GAME_QUIT,OPALIGN);
 #else
    if ( (!bMultiPlayer || (NetPlay.bComms==0) )  && !bInTutorial)
	{
		addIGTextButton(INTINGAMEOP_QUIT,INTINGAMEOP_5_Y,STR_GAME_QUIT,OPALIGN);

	}
	else
	{	
		addIGTextButton(INTINGAMEOP_QUIT,INTINGAMEOP_3_Y,STR_GAME_QUIT,OPALIGN);
	}

 #endif
#else
	addIGTextButton(INTINGAMEOP_QUIT,INTINGAMEOP_3_Y,STR_GAME_QUIT,OPALIGN);
#endif

	// add 'resume'
	addIGTextButton(INTINGAMEOP_RESUME,INTINGAMEOP_1_Y,STR_GAME_RESUME,OPALIGN);

	SetCurrentSnapID(&InterfaceSnap,INTINGAMEOP_RESUME);

	// add 'options'
	addIGTextButton(INTINGAMEOP_OPTIONS,INTINGAMEOP_2_Y,STR_FE_OPTIONS,OPALIGN);

#ifdef WIN32		
	if ( (!bMultiPlayer || (NetPlay.bComms==0) )  && !bInTutorial)
	{		// add 'load'
		addIGTextButton(INTINGAMEOP_LOAD,INTINGAMEOP_3_Y,STR_MISC_LOADGAME,OPALIGN);
		// add 'save'
		addIGTextButton(INTINGAMEOP_SAVE,INTINGAMEOP_4_Y,STR_MISC_SAVEGAME,OPALIGN);
	}
#endif

	intMode		= INT_INGAMEOP;			// change interface mode.
	InGameOpUp	= TRUE;					// inform interface.
	SetMousePos(0,INTINGAMEOP_X+INTINGAMEOP_1_X,INTINGAMEOP_Y+INTINGAMEOP_1_Y); // move mouse to resume.
	
	pie_SetMouse(IntImages,IMAGE_CURSOR_DEFAULT);			// reset cursor (hw)
	frameSetCursorFromRes(IDC_DEFAULT);						// reset cursor	(sw)

	return TRUE;
}


BOOL intAddInGameOptions(void)
{
#ifdef PSX
	// If the stacks in the dcache then..
	if(SpInDCache()) {
		static BOOL ret;

		// Set the stack pointer to point to the alternative stack which is'nt limited to 1k.
		SetSpAlt();
		ret = _intAddInGameOptions();
		SetSpAltNormal();

		return ret;
	}
#endif
	return _intAddInGameOptions();
}

// ////////////////////////////////////////////////////////////////////////////

void ProcessOptionFinished(void)
{
	intMode		= INT_NORMAL;

#ifdef PSX
	CancelInterfaceSnap();
	// Unpause the game.
//	setGameUpdatePause(FALSE);
	setGamePauseStatus(FALSE);
	gameTimeStart();
	if(driveModeActive()) {
		driveEnableControl();
	}

	if(DirectControl) {
		StartCameraMode();
	} else {
		StopCameraMode();
	}
#else
	//unpause.
	if(gamePaused())
	{
		kf_TogglePauseMode();
	}
#endif

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
#ifdef PSX
	intCloseInGameOptionsNoAnim(bResetMissionWidgets);
#else
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
#endif
    return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// In Game Options house keeping stuff.
BOOL intRunInGameOptions(void)
{
#ifdef WIN32
	processFrontendSnap(FALSE);
#endif
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// process clicks made by user.
void intProcessInGameOptions(UDWORD id)
{
#ifdef PSX
	if(CancelPressed()) {
		RestoreGameOptions();
		intCloseInGameOptions(FALSE, TRUE);
		return;
	}
	if(VPadTriggered(VPAD_PAUSE)) {
		VPadClearTrig(VPAD_PAUSE);
		intCloseInGameOptions(FALSE, TRUE);
		return;
	}
#endif

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

#ifdef WIN32
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
		addLoadSave(LOAD_INGAME,"savegame\\","gam",strresGetString(psStringRes,STR_MR_LOAD_GAME));	// change mode when loadsave returns//		if(runLoadSave())// check for file name.
		break;
	case INTINGAMEOP_SAVE:
		intCloseInGameOptions(TRUE, FALSE);
		addLoadSave(SAVE_INGAME,"savegame\\","gam", strresGetString(psStringRes,STR_MR_SAVE_GAME) );
		break;
#endif

	// GAME OPTIONS KEYS 
	case INTINGAMEOP_FXVOL:	
	case INTINGAMEOP_CDVOL:	
//	case INTINGAMEOP_GAMMA:	
		SetMousePos(0,INTINGAMEOP2_X+INTINGAMEOP_MID+5 ,mouseY());	// move mouse
		break;

#ifdef WIN32
	case INTINGAMEOP_FXVOL_S:	
		mixer_SetWavVolume(widgGetSliderPos(psWScreen,INTINGAMEOP_FXVOL_S));
		break;
	case INTINGAMEOP_CDVOL_S:	
		mixer_SetCDVolume(widgGetSliderPos(psWScreen,INTINGAMEOP_CDVOL_S));
		break;
	
//	case INTINGAMEOP_GAMMA_S:
//		gamma = (float)(widgGetSliderPos(psWScreen,INTINGAMEOP_GAMMA_S))/25  ;
//		if(gamma<0.5)  gamma = (float).5;
//		pie_SetGammaValue(gamma);
//		break;
#endif
	
	default:
		break;
	}

#ifdef PSX
	processSliderOptions(id);
	processToggleOptions(id);
	processCentreScreen(id);
#endif
}

