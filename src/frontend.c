/*
 * FrontEnd.c
 *
 * front end title and options screens.
 * Alex Lee. Pumpkin Studios. Eidos PLC 98,
 */

/*	Playstation button symbol -> font mappings.
|	=	X
{	=	Circle
}	=	Square
~	=	Triangle
*/

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/strres.h"
#include "winmain.h"
#include "objects.h"
#include "display.h"
#include "lib/widget/widget.h"
/* Includes direct access to render library */
#include "lib/ivis_common/ivisdef.h"
#include "lib/ivis_common/piestate.h"

#include "keyedit.h"
#include "lib/ivis_common/piefunc.h"
#include "warzoneconfig.h"

// FIXME Direct iVis implementation include!
#include "lib/ivis_common/rendmode.h"

#include "display3d.h"
#include "intdisplay.h"
#include "lib/sound/audio.h"					// for sound.
#include "audio_id.h"				// for sound..

#include "lib/sound/cdaudio.h"
#include "lib/sound/mixer.h"
#include "configuration.h"


#include "design.h"					// for intadddesign
#include "hci.h"					// for intShowPower
#include "text.h"					// to get at string resources.
#include "frontend.h"
#include "console.h"
#include "wrappers.h"
#include "component.h"
#include "loadsave.h"
#include "csnap.h"
//#include "wrappers.h"				// for bUsingKeyboard.
#include "frend.h"
#include "game.h"
#include "init.h"
#include "difficulty.h"
#include "ingameop.h"


#include "advvis.h"
#include "seqdisp.h"
#include "multiplay.h"
#include "multiint.h"
#include "multilimit.h"
#include "multistat.h"
#include "lib/netplay/netplay.h"

extern BOOL bSubtitles;

extern void intUpdateOptionText(struct _widget *psWidget, struct _w_context *psContext);
extern void set_active_data_directory(int);

extern CURSORSNAP InterfaceSnap;
extern void ProcessCursorSnap(void);

int StartWithGame = 1;	// New game starts in Cam 1.

char OnString[]={"On "};
char OffString[]={"Off"};


char	strFog[MAX_STR_LENGTH];
char	strTrans[MAX_STR_LENGTH];

tMode titleMode;					// the global case


// ////////////////////////////////////////////////////////////////////////////
// Local Definitions
// iPalette			titlePalette;
int				FEFont;
//int				FEBigFont;
char			pLevelName[MAX_LEVEL_NAME_SIZE+1];	//256];			// vital! the wrf file to use.

BOOL			bForceEditorLoaded = FALSE;
BOOL			bUsingKeyboard = FALSE;		// to disable mouse pointer when using keys.
BOOL			bUsingSlider   = FALSE;

// ////////////////////////////////////////////////////////////////////////////
// extern Definitions

//extern W_SCREEN		*psWScreen;					//The widget screen

extern char	SaveGamePath[];

extern BOOL firstcall;
extern IMAGEFILE *FrontImages;


// ////////////////////////////////////////////////////////////////////////////
// Function Definitions

void		processFrontendSnap		(BOOL bHideCursor);
void		changeTitleMode			(tMode mode);
BOOL		startTitleMenu			(void);
BOOL		runTitleMenu			(void);
void		startSinglePlayerMenu	(void);
BOOL		runSinglePlayerMenu		(void);
//BOOL		runDemoMenu				(void);
//BOOL		startDemoMenu			(void);
BOOL		startTutorialMenu		(void);
BOOL		runTutorialMenu			(void);
BOOL		startMultiPlayerMenu	(void);
BOOL		runMultiPlayerMenu		(void);
BOOL		startOptionsMenu		(void);
BOOL		runOptionsMenu			(void);
BOOL		startGameOptionsMenu	(void);
BOOL		runGameOptionsMenu		(void);
BOOL		startGameOptions2Menu	(void);
BOOL		runGameOptions2Menu		(void);
BOOL		startGameOptions3Menu	(void);
BOOL		runGameOptions3Menu		(void);
//BOOL		startVideoOptionsMenu	(void);
//BOOL		runVideoOptionsMenu		(void);
//BOOL		startGraphicsOptionsMenu(void);
//BOOL		runGraphicsptionsMenu	(void);

void		addTopForm				(void);
void		removeTopForm			(void);
void		addBottomForm			(void);
void		removeBottomForm		(void);
void		addBackdrop				(void);
void		removeBackdrop			(void);

void		addTextButton			(UDWORD id,  UDWORD PosX, UDWORD PosY, char *txt,BOOL bAlignLeft,BOOL bGrey);
void 		addText					(int FontID,UDWORD FormID,UDWORD id,  UDWORD PosX, UDWORD PosY, char *txt, UDWORD attachID,BOOL *State);
void		addSideText				(UDWORD id,  UDWORD PosX, UDWORD PosY, char *txt);
void		addFESlider				(UDWORD id, UDWORD parent, UDWORD x,UDWORD y,UDWORD stops,UDWORD pos,UDWORD attachID);

void		displayLogo				(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
void		displayTitleBitmap		(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
void		displayTextOption		(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
void		displayTextAt270		(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
static void	displayBigSlider		(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);





// Returns TRUE if escape key pressed on PC or close button pressed on Playstation.
//
BOOL CancelPressed(void)
{

	if(keyPressed(KEY_ESC)) {
		return TRUE;
	}


	return FALSE;
}
// ////////////////////////////////////////////////////////////////////////////
// for cursorsnap stuff on pc

void processFrontendSnap(BOOL bHideCursor)
{
	static POINT point,opoint;

	point.x = mouseX();
	point.y = mouseY();

	if(point.x != opoint.x ||  point.y  != opoint.y)
	{
		bUsingKeyboard = FALSE;
	}

	if(!bUsingSlider)
	{
		if(keyPressed(KEY_RIGHTARROW))
		{
			bUsingKeyboard	= TRUE;
			GotoDirectionalSnap(&InterfaceSnap,SNAP_RIGHT,0,0);
		}
		else if(keyPressed(KEY_LEFTARROW))
		{
			bUsingKeyboard	= TRUE;
			GotoDirectionalSnap(&InterfaceSnap,SNAP_LEFT,0,0);
		}
	}
	if(keyPressed(KEY_UPARROW))
	{
		bUsingKeyboard	= TRUE;
		bUsingSlider	= FALSE;
		GotoDirectionalSnap(&InterfaceSnap,SNAP_UP,0,0);
	}
	else if(keyPressed(KEY_DOWNARROW))
	{
		bUsingKeyboard	= TRUE;
		bUsingSlider	= FALSE;
		GotoDirectionalSnap(&InterfaceSnap,SNAP_DOWN,0,0);
	}

	if (!keyDown(KEY_LALT) && !keyDown(KEY_RALT)/* Check for toggling display mode */
		&& (psWScreen->psFocus == NULL))
	{
		if(keyPressed(KEY_RETURN) )
		{
			bUsingKeyboard = TRUE;
			setMouseDown(MOUSE_LMB);
		}

		if(keyReleased(KEY_RETURN) )
		{
			bUsingKeyboard = TRUE;
			setMouseUp(MOUSE_LMB);
		}
	}

	if(!bHideCursor)
	{
		bUsingKeyboard = FALSE;
	}

	opoint.x = mouseX();
	opoint.y = mouseY();
}


// ////////////////////////////////////////////////////////////////////////////
// Change Mode
void changeTitleMode(tMode mode)
{
	tMode oldMode;

	widgDelete(psWScreen,FRONTEND_BACKDROP);		// delete backdrop.

	oldMode = titleMode;							// store old mode
	titleMode = mode;								// set new mode

	switch(mode)
	{
/*	case DEMOMODE:// demo case. remove for release
		startDemoMenu();
		break;
	case VIDEO:
		startVideoOptionsMenu();
		break;
*/
	case SINGLE:
		startSinglePlayerMenu();
		break;
	case GAME:
		startGameOptionsMenu();
		break;

	case GAME2:
		startGameOptions2Menu();
		break;

	case GAME3:
		startGameOptions3Menu();
		break;

	case TUTORIAL:
		startTutorialMenu();
		break;
	case OPTIONS:
		startOptionsMenu();
		break;
	case TITLE:
		startTitleMenu();
		break;


//	case GRAPHICS:
//		startGraphicsOptionsMenu();
//		break;
	case CREDITS:
		startCreditsScreen(FALSE);
		break;

 	case MULTI:
		startMultiPlayerMenu();		// goto multiplayer menu
		break;
	case PROTOCOL:
		startConnectionScreen();
		break;
	case FORCESELECT:
		bUsingKeyboard = FALSE;
		startForceSelect();
		break;
	case MULTIOPTION:
		bUsingKeyboard = FALSE;
		if(oldMode == MULTILIMIT)
		{
			startMultiOptions(TRUE);
		}
		else
		{
			startMultiOptions(FALSE);
		}
		break;
	case GAMEFIND:
		bUsingKeyboard = FALSE;
		startGameFind();
		break;
	case MULTILIMIT:
		bUsingKeyboard = FALSE;
		startLimitScreen();
		break;
	case KEYMAP:
		bUsingKeyboard = FALSE;
		startKeyMapEditor(TRUE);
		break;

	case STARTGAME:
	case QUIT:
	case LOADSAVEGAME:
		bUsingKeyboard = FALSE;
		bForceEditorLoaded = FALSE;
	case SHOWINTRO:
		break;

	default:
		debug( LOG_ERROR, "Unknown title mode requested" );
		abort();
		break;
	}

	return;
}


// ////////////////////////////////////////////////////////////////////////////
// Title Screen
BOOL startTitleMenu(void)
{


//	widgDelete(psWScreen,1);	// close reticule if it's open. MAGIC NUMBERS?
	intRemoveReticule();

	addBackdrop();
	addTopForm();
	addBottomForm();

		addTextButton(FRONTEND_SINGLEPLAYER,FRONTEND_POS2X,FRONTEND_POS2Y, strresGetString(psStringRes, STR_FE_SINGLE),FALSE,FALSE);
		if(!bDisableLobby)
		{
			addTextButton(FRONTEND_MULTIPLAYER,	FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_MULTI)   ,FALSE,FALSE);
		}else{
			addTextButton(FRONTEND_MULTIPLAYER,	FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_MULTI)   ,FALSE,TRUE);
		}
		addTextButton(FRONTEND_TUTORIAL,	FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_TUT) ,FALSE,FALSE);
		addTextButton(FRONTEND_OPTIONS,		FRONTEND_POS5X,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_OPTIONS) ,FALSE,FALSE);

	addTextButton(FRONTEND_QUIT,		FRONTEND_POS6X,FRONTEND_POS6Y, strresGetString(psStringRes, STR_FE_QUIT),FALSE,FALSE);


	addSideText	 (FRONTEND_SIDETEXT ,	FRONTEND_SIDEX,FRONTEND_SIDEY,strresGetString(psStringRes, STR_FE_SIDEMAIN));


	SetMousePos(0,320,FRONTEND_BOTFORMY+FRONTEND_POS2Y);
	SnapToID(&InterfaceSnap,4);


	return TRUE;
}


BOOL runTitleMenu(void)
{
	UDWORD id;

	processFrontendSnap(TRUE);

	id = widgRunScreen(psWScreen);						// Run the current set of widgets

		switch(id)
		{
			case FRONTEND_QUIT:
			changeTitleMode(CREDITS);
			break;
		case FRONTEND_MULTIPLAYER:
			changeTitleMode(MULTI);
			break;
		case FRONTEND_SINGLEPLAYER:
			changeTitleMode(SINGLE);
			break;
		case FRONTEND_OPTIONS:
			changeTitleMode(OPTIONS);
			break;
		case FRONTEND_TUTORIAL:
			changeTitleMode(TUTORIAL);
			break;
		default:
			break;
		}

	DrawBegin();
	StartCursorSnap(&InterfaceSnap);
	widgDisplayScreen(psWScreen);						// show the widgets currently running
	DrawEnd();

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// Tutorial Menu

BOOL startTutorialMenu(void)
{
	addBackdrop();
	addTopForm();
	addBottomForm();


	addTextButton(FRONTEND_TUTORIAL, FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes,STR_FE_TUT),FALSE,FALSE);
	addTextButton(FRONTEND_FASTPLAY, FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes,STR_FE_FASTPLAY),FALSE,FALSE);
	addSideText	 (FRONTEND_SIDETEXT ,FRONTEND_SIDEX,FRONTEND_SIDEY,strresGetString(psStringRes,STR_FE_SIDETUT));
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);

	SetCurrentSnapID(&InterfaceSnap,FRONTEND_FASTPLAY);



	return TRUE;
}

BOOL runTutorialMenu(void)
{
	UDWORD id;

	processFrontendSnap(TRUE);
	id = widgRunScreen(psWScreen);						// Run the current set of widgets
	switch(id)
	{
		case FRONTEND_TUTORIAL:
			strcpy(pLevelName,TUTORIAL_LEVEL);
			changeTitleMode(STARTGAME);

			break;

		case FRONTEND_FASTPLAY:
			strcpy(pLevelName,"FASTPLAY");
			changeTitleMode(STARTGAME);

			break;

		case FRONTEND_QUIT:
			changeTitleMode(TITLE);
			break;

		default:
			break;
	}

	// If close button pressed then return from this menu.
	if(CancelPressed()) {
		changeTitleMode(TITLE);
	}

	DrawBegin();
	StartCursorSnap(&InterfaceSnap);
	widgDisplayScreen(psWScreen);						// show the widgets currently running
	DrawEnd();
	return TRUE;

}


// ////////////////////////////////////////////////////////////////////////////
// Single Player Menu

void startSinglePlayerMenu(void)
{
	addBackdrop();
	addTopForm();
	addBottomForm();

	addTextButton(FRONTEND_LOADGAME, FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes,STR_FE_LOAD),FALSE,FALSE);
	addTextButton(FRONTEND_NEWGAME,  FRONTEND_POS3X,FRONTEND_POS3Y,strresGetString(psStringRes,STR_FE_NEW) ,FALSE,FALSE);

	addSideText	 (FRONTEND_SIDETEXT ,FRONTEND_SIDEX,FRONTEND_SIDEY,strresGetString(psStringRes,STR_FE_SIDESINGLE1));
	SetCurrentSnapID(&InterfaceSnap,FRONTEND_LOADGAME);
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);
}

static void endSinglePlayerMenu( void )
{
	removeTopForm();
	removeBottomForm();
	removeBackdrop();
}

static void frontEndNewGame( void )
{
	switch(StartWithGame) {
		case 1:
			strcpy(pLevelName,DEFAULT_LEVEL);
			seq_ClearSeqList();

			seq_AddSeqToList("cam1/c001.rpl",NULL,"cam1/c001.txa",FALSE,0);

			seq_StartNextFullScreenVideo();
            break;

		case 2:
			strcpy(pLevelName,"CAM_2A");
			break;

		case 3:
			strcpy(pLevelName,"CAM_3A");
			break;
	}

	changeTitleMode(STARTGAME);
}

void loadOK( void )
{

	if(strlen(sRequestResult))
	{
		strcpy(saveGameName,sRequestResult);
		changeTitleMode(LOADSAVEGAME);
	}
	SetCurrentSnapID(&InterfaceSnap,FRONTEND_LOADGAME);

}

BOOL runSinglePlayerMenu(void)
{
	UDWORD id;

	processFrontendSnap(TRUE);


	if(bLoadSaveUp)
	{
		if(runLoadSave(FALSE))// check for file name.
		{
			loadOK();
			SetCurrentSnapID(&InterfaceSnap,FRONTEND_LOADGAME);
		}
	}
	else

	{

	id = widgRunScreen(psWScreen);						// Run the current set of widgets



		switch(id)
		{
			case FRONTEND_NEWGAME:
					frontEndNewGame();
				break;

			case FRONTEND_LOADCAM2:
 #ifdef PSX_DIFFICULTY_MENU
				StartMenuDepth = 1;
				StartWithGame = 2;
				endSinglePlayerMenu();
				startDifficultyMenu();
 #else
				strcpy(pLevelName,"CAM_2A");
				changeTitleMode(STARTGAME);
 #ifdef LOADINGBACKDROPS
				AddLoadingBackdrop(TRUE);
 #else
				initLoadingScreen(TRUE,TRUE);
 #endif
 #endif
				break;

			case FRONTEND_LOADCAM3:
 #ifdef PSX_DIFFICULTY_MENU
				StartMenuDepth = 1;
				StartWithGame = 3;
				endSinglePlayerMenu();
				startDifficultyMenu();
 #else
				strcpy(pLevelName,"CAM_3A");
				changeTitleMode(STARTGAME);
 #ifdef LOADINGBACKDROPS
				AddLoadingBackdrop(TRUE);
 #else
				initLoadingScreen(TRUE,TRUE);
 #endif
 #endif
				break;
			case FRONTEND_LOADGAME:

				addLoadSave(LOAD_FRONTEND,SaveGamePath,"gam",strresGetString(psStringRes,STR_MR_LOAD_GAME));	// change mode when loadsave returns
				break;

			case FRONTEND_QUIT:
				changeTitleMode(TITLE);
				break;

			default:
				break;
		}

	if(CancelPressed())
	{

		changeTitleMode(TITLE);

	}

	}

	DrawBegin();
	StartCursorSnap(&InterfaceSnap);

	if(!bLoadSaveUp)										// if save/load screen is up
	{
		widgDisplayScreen(psWScreen);						// show the widgets currently running
	}
	if(bLoadSaveUp)										// if save/load screen is up
	{
		displayLoadSave();
	}





	DrawEnd();
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// Multi Player Menu
BOOL startMultiPlayerMenu(void)
{
	addBackdrop();
	addTopForm();
	addBottomForm();

	addSideText	 (FRONTEND_SIDETEXT ,	FRONTEND_SIDEX,FRONTEND_SIDEY,strresGetString(psStringRes, STR_FE_SIDEMULTI));

	addTextButton(FRONTEND_HOST,     FRONTEND_POS2X,FRONTEND_POS2Y, strresGetString(psStringRes, STR_FE_HOST),FALSE,FALSE);
	addTextButton(FRONTEND_JOIN,     FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_JOIN),FALSE,FALSE);

	addTextButton(FRONTEND_FORCEEDIT,FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_FORCEEDIT),FALSE,FALSE);
	addTextButton(FRONTEND_SKIRMISH, FRONTEND_POS5X,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_SKIRMISH ),FALSE,FALSE);

	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);

	SetMousePos(0,320,FRONTEND_BOTFORMY+FRONTEND_POS3Y);
	SnapToID(&InterfaceSnap,3);
	return TRUE;
}

BOOL runMultiPlayerMenu(void)
{
	UDWORD id;

	processFrontendSnap(TRUE);

	id = widgRunScreen(psWScreen);						// Run the current set of widgets
	switch(id)
	{
	case FRONTEND_HOST:
		ingame.bHostSetup = TRUE;
		changeTitleMode(PROTOCOL);
		break;
	case FRONTEND_JOIN:
		ingame.bHostSetup = FALSE;
		changeTitleMode(PROTOCOL);
		break;
	case FRONTEND_FORCEEDIT:

		if(!bForceEditorLoaded)
		{
			initLoadingScreen( TRUE, TRUE);
/*			if (!resLoad("wrf/forcedit.wrf", 500,
						 DisplayBuffer, displayBufferSize,
						 psGameHeap))				//need the object heaps to have been set up before loading
			{
				return FALSE;
			}
*/
			if (!resLoad("wrf/piestats.wrf", 501,
						 DisplayBuffer, displayBufferSize,
						 psGameHeap))				//need the object heaps to have been set up before loading
			{
				return FALSE;
			}

			if (!resLoad("wrf/forcedit2.wrf", 502,
						 DisplayBuffer, displayBufferSize,
						 psGameHeap))				//need the object heaps to have been set up before loading
			{
				return FALSE;
			}

			bForceEditorLoaded = TRUE;
			closeLoadingScreen();
		}

		changeTitleMode(FORCESELECT);
		return TRUE;//skip draw.
		break;

	case FRONTEND_SKIRMISH:
		ingame.bHostSetup = TRUE;

		NETuseNetwork(FALSE);						// pretend its a multiplayer.

//		strcpy(sPlayer,"LastUsed");					// initialize name string.
//		loadMultiStats(sPlayer,&nullStats);
//		NETchangePlayerName(1,sPlayer);

		changeTitleMode(MULTIOPTION);
		break;

	case FRONTEND_QUIT:
		changeTitleMode(TITLE);
		break;
	default:
		break;
	}

	DrawBegin();
	StartCursorSnap(&InterfaceSnap);
	widgDisplayScreen(psWScreen);					// show the widgets currently running
	DrawEnd();

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// Options Menu
BOOL startOptionsMenu(void)
{
	addBackdrop();
	addTopForm();
	addBottomForm();

	addSideText	 (FRONTEND_SIDETEXT ,	FRONTEND_SIDEX,FRONTEND_SIDEY, strresGetString(psStringRes, STR_FE_SIDEOPTIONS));
	addTextButton(FRONTEND_GAMEOPTIONS,	FRONTEND_POS2X,FRONTEND_POS2Y, strresGetString(psStringRes, STR_FE_GAME),FALSE,FALSE);
	addTextButton(FRONTEND_GAMEOPTIONS2,FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_GRAPHICS),FALSE,FALSE);
	addTextButton(FRONTEND_GAMEOPTIONS3,	FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_AUDIO),FALSE,FALSE);
	addTextButton(FRONTEND_KEYMAP,		FRONTEND_POS5X,FRONTEND_POS5Y, strresGetString(psStringRes, STR_KM_KEYMAP),FALSE,FALSE);
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);

	SetMousePos(0,320,FRONTEND_BOTFORMY+FRONTEND_POS3Y);
	SnapToID(&InterfaceSnap,3);



	return TRUE;
}



BOOL runOptionsMenu(void)
{
	UDWORD id;

	processFrontendSnap(TRUE);

	id = widgRunScreen(psWScreen);						// Run the current set of widgets
	switch(id)
	{

	case FRONTEND_GAMEOPTIONS:
		changeTitleMode(GAME);
		break;
	case FRONTEND_GAMEOPTIONS2:
		changeTitleMode(GAME2);
		break;
	case FRONTEND_GAMEOPTIONS3:
		changeTitleMode(GAME3);
		break;
//	case FRONTEND_VIDEO:
//		changeTitleMode(VIDEO);
//		break;
//	case FRONTEND_GRAPHICS:
//		changeTitleMode(GRAPHICS);
//		break;
	case FRONTEND_KEYMAP:
		changeTitleMode(KEYMAP);
		break;

	case FRONTEND_QUIT:
		changeTitleMode(TITLE);
		break;


	default:
		break;
	}



	// If close button pressed then return from this menu.
	if(CancelPressed()) {


		changeTitleMode(TITLE);

	}

	DrawBegin();
	StartCursorSnap(&InterfaceSnap);
	widgDisplayScreen(psWScreen);						// show the widgets currently running
	DrawEnd();

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// Game Options Menu 2!
BOOL startGameOptions2Menu(void)
{
	addBackdrop();
	addTopForm();
	addBottomForm();


	////////////
	// mouseflip
	addTextButton(FRONTEND_MFLIP,	 FRONTEND_POS2X-35,   FRONTEND_POS2Y, strresGetString(psStringRes,STR_FE_MFLIP),TRUE,FALSE);
	if( getInvertMouseStatus() )
	{// flipped
		addTextButton(FRONTEND_MFLIP_R, FRONTEND_POS2M-55,  FRONTEND_POS2Y, strresGetString(psStringRes,STR_FE_ON),TRUE,FALSE);
	}
	else
	{	// not flipped
		addTextButton(FRONTEND_MFLIP_R, FRONTEND_POS2M-55,  FRONTEND_POS2Y, strresGetString(psStringRes,STR_FE_OFF),TRUE,FALSE);
	}

	////////////
	// screenshake
	addTextButton(FRONTEND_SSHAKE,	 FRONTEND_POS3X-35,   FRONTEND_POS3Y, strresGetString(psStringRes,STR_FE_SSHAKE),TRUE,FALSE);
	if(getShakeStatus())
	{// shaking on
		addTextButton(FRONTEND_SSHAKE_R, FRONTEND_POS3M-55,  FRONTEND_POS3Y, strresGetString(psStringRes,STR_FE_ON),TRUE,FALSE);
	}
	else
	{//shaking off.
		addTextButton(FRONTEND_SSHAKE_R, FRONTEND_POS3M-55,  FRONTEND_POS3Y, strresGetString(psStringRes,STR_FE_OFF),TRUE,FALSE);
	}

	////////////
	// fog
	addTextButton(FRONTEND_FOGTYPE,	 FRONTEND_POS4X-35,   FRONTEND_POS4Y, strresGetString(psStringRes,STR_FE_FOG),TRUE,FALSE);
	if(war_GetFog())
	{
		addTextButton(FRONTEND_FOGTYPE_R,FRONTEND_POS4M-55,FRONTEND_POS4Y, strresGetString(psStringRes,STR_FE_CRAPFOG),TRUE,FALSE);
	}
	else
	{
		addTextButton(FRONTEND_FOGTYPE_R,FRONTEND_POS4M-55,FRONTEND_POS4Y, strresGetString(psStringRes,STR_FE_GOODFOG),TRUE,FALSE);
	}

/*	////////////
	// fog
	addTextButton(FRONTEND_FOG,		FRONTEND_POS5X-15,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_FOG),TRUE,FALSE);
	if (war_GetFog())
	{
		addTextButton(FRONTEND_FOG_R,	FRONTEND_POS5M-55,	FRONTEND_POS5Y, strresGetString(psStringRes,STR_FE_ON),TRUE,FALSE);
	}
	else
	{
		addTextButton(FRONTEND_FOG_R,	FRONTEND_POS5M-55,	FRONTEND_POS5Y, strresGetString(psStringRes,STR_FE_OFF),TRUE,FALSE);
	}
*/

//	////////////
//	//sequence mode.
	addTextButton(FRONTEND_SEQUENCE,	FRONTEND_POS6X-35,FRONTEND_POS6Y, strresGetString(psStringRes, STR_SEQ_PLAYBACK),TRUE,FALSE);
	if (war_GetSeqMode() == SEQ_FULL)
	{
		addTextButton(FRONTEND_SEQUENCE_R,	FRONTEND_POS6M-55,FRONTEND_POS6Y, strresGetString(psStringRes,STR_SEQ_FULL),TRUE,FALSE);
	}
	else if (war_GetSeqMode() == SEQ_SMALL)
	{
		addTextButton(FRONTEND_SEQUENCE_R,	FRONTEND_POS6M-55,FRONTEND_POS6Y, strresGetString(psStringRes,STR_SEQ_WINDOW),TRUE,FALSE);	}
	else
	{
		addTextButton(FRONTEND_SEQUENCE_R,	FRONTEND_POS6M-55,FRONTEND_POS6Y, strresGetString(psStringRes,STR_SEQ_MINIMAL),TRUE,FALSE);
	}

//	////////////
//	//translucency mode.
//	addTextButton(FRONTEND_TRANSPARENCY,	FRONTEND_POS5X-15,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_TRANSPARENCY),TRUE,FALSE);
//	if (!war_GetTranslucent())
//	{
//		addTextButton(FRONTEND_TRANSPARENCY_R,	FRONTEND_POS5M-55,FRONTEND_POS5Y, strresGetString(psStringRes,STR_FE_OFF),TRUE,FALSE);
//	}
//	else if (!war_GetAdditive())
//	{
//	addTextButton(FRONTEND_TRANSPARENCY_R,	FRONTEND_POS5M-55,FRONTEND_POS5Y, strresGetString(psStringRes,STR_COMPATIBLE),TRUE,FALSE);
//	}
//	else
//	{
//	addTextButton(FRONTEND_TRANSPARENCY_R,	FRONTEND_POS5M-55,FRONTEND_POS5Y, strresGetString(psStringRes,STR_FE_ON),TRUE,FALSE);
//	}

	////////////
	//subtitle mode.
	if(war_GetAllowSubtitles())
	{
		addTextButton(FRONTEND_SUBTITLES,	FRONTEND_POS5X-35,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_SUBTITLES),TRUE,FALSE);
	}
	else
	{
		addTextButton(FRONTEND_SUBTITLES,	FRONTEND_POS5X-35,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_SUBTITLES),TRUE,TRUE);
	}

	if(war_GetAllowSubtitles())
	{
		if ( !seq_GetSubtitles() )
		{
			addTextButton(FRONTEND_SUBTITLES_R,	FRONTEND_POS5M-55,FRONTEND_POS5Y, strresGetString(psStringRes,STR_FE_OFF),TRUE,FALSE);
		}
		else
		{
			addTextButton(FRONTEND_SUBTITLES_R,	FRONTEND_POS5M-55,FRONTEND_POS5Y, strresGetString(psStringRes,STR_FE_ON),TRUE,FALSE);
		}
	}
	else
	{
		addTextButton(FRONTEND_SUBTITLES_R,	FRONTEND_POS6M-55,FRONTEND_POS5Y, strresGetString(psStringRes,STR_FE_OFF),TRUE,TRUE);
	}

	////////////
	// quit.
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);

	return TRUE;
}


BOOL runGameOptions2Menu(void)
{
	UDWORD id;

	processFrontendSnap(FALSE);

	id = widgRunScreen(psWScreen);						// Run the current set of widgets
	switch(id)
	{
	case FRONTEND_SSHAKE:
	case FRONTEND_SSHAKE_R:
		if( getShakeStatus() )
		{
			setShakeStatus(FALSE);
			widgSetString(psWScreen,FRONTEND_SSHAKE_R, strresGetString(psStringRes,STR_FE_OFF));
		}
		else
		{
			setShakeStatus(TRUE);
			widgSetString(psWScreen,FRONTEND_SSHAKE_R, strresGetString(psStringRes,STR_FE_ON));
		}
		break;
		break;
	case FRONTEND_MFLIP:
	case FRONTEND_MFLIP_R:
		if( getInvertMouseStatus() )
		{//	 flipped
			setInvertMouseStatus(FALSE);
			widgSetString(psWScreen,FRONTEND_MFLIP_R, strresGetString(psStringRes,STR_FE_OFF));
		}
		else
		{	// not flipped
			setInvertMouseStatus(TRUE);
			widgSetString(psWScreen,FRONTEND_MFLIP_R, strresGetString(psStringRes,STR_FE_ON));
		}
		break;

	case FRONTEND_FOGTYPE:
	case FRONTEND_FOGTYPE_R:
	if( war_GetFog()	)
	{	// turn off crap fog, turn on vis fog.
		war_SetFog(FALSE);
		avSetStatus(TRUE);
		widgSetString(psWScreen,FRONTEND_FOGTYPE_R, strresGetString(psStringRes,STR_FE_GOODFOG));
	}
	else
	{	// turn off vis fog, turn on normal crap fog.
		avSetStatus(FALSE);
		war_SetFog(TRUE);
		widgSetString(psWScreen,FRONTEND_FOGTYPE_R, strresGetString(psStringRes,STR_FE_CRAPFOG));
	}
	break;

	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	case FRONTEND_SUBTITLES:
	case FRONTEND_SUBTITLES_R:
		if(	seq_GetSubtitles())
		{// turn off
			seq_SetSubtitles(FALSE);
			widgSetString(psWScreen,FRONTEND_SUBTITLES_R,strresGetString(psStringRes,STR_FE_OFF));
		}
		else
		{// turn on
			seq_SetSubtitles(TRUE);
			widgSetString(psWScreen,FRONTEND_SUBTITLES_R,strresGetString(psStringRes,STR_FE_ON));
		}
		break;


/*	case FRONTEND_FOG:
	case FRONTEND_FOG_R:
		if (war_GetFog())
		{
			war_SetFog(FALSE);
			widgSetString(psWScreen,FRONTEND_FOG_R,strresGetString(psStringRes,STR_FE_OFF));
		}
		else
		{
			war_SetFog(TRUE);
			widgSetString(psWScreen,FRONTEND_FOG_R,strresGetString(psStringRes,STR_FE_ON));
		}
		break;
*/


//	case FRONTEND_TRANSPARENCY:
//	case FRONTEND_TRANSPARENCY_R:
//		if (!war_GetTranslucent())
//		{
//			war_SetTranslucent(TRUE);
//			war_SetAdditive(FALSE);
//			widgSetString(psWScreen,FRONTEND_TRANSPARENCY_R, strresGetString(psStringRes,STR_COMPATIBLE));
//		}
//		else if (!war_GetAdditive())
//		{
//			war_SetTranslucent(TRUE);
//			war_SetAdditive(TRUE);
//			widgSetString(psWScreen,FRONTEND_TRANSPARENCY_R,strresGetString(psStringRes,STR_FE_ON));
//		}
//		else
//		{
//			war_SetTranslucent(FALSE);
//			war_SetAdditive(FALSE);
//			widgSetString(psWScreen,FRONTEND_TRANSPARENCY_R,strresGetString(psStringRes,STR_FE_OFF));
//		}
//		break;
	case FRONTEND_SEQUENCE:
	case FRONTEND_SEQUENCE_R:
		if( war_GetSeqMode() == SEQ_FULL )
		{
			war_SetSeqMode(SEQ_SMALL);
			widgSetString(psWScreen,FRONTEND_SEQUENCE_R, strresGetString(psStringRes,STR_SEQ_WINDOW));
		}
		else if( war_GetSeqMode() == SEQ_SMALL )
		{
			war_SetSeqMode(SEQ_SKIP);
			widgSetString(psWScreen,FRONTEND_SEQUENCE_R, strresGetString(psStringRes,STR_SEQ_MINIMAL));
		}
		else
		{
			war_SetSeqMode(SEQ_FULL);
			widgSetString(psWScreen,FRONTEND_SEQUENCE_R, strresGetString(psStringRes,STR_SEQ_FULL));
		}
		break;

	default:
		break;
	}


	// If close button pressed then return from this menu.
	if(CancelPressed()) {
		changeTitleMode(OPTIONS);
	}

	DrawBegin();
	StartCursorSnap(&InterfaceSnap);
	widgDisplayScreen(psWScreen);						// show the widgets currently running
	DrawEnd();

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// Game Options Menu
BOOL startGameOptions3Menu(void)
{
	addBackdrop();
	addTopForm();
	addBottomForm();

	// 2d audio
	addTextButton(FRONTEND_FX, FRONTEND_POS2X-25,FRONTEND_POS2Y, strresGetString(psStringRes, STR_FE_FX),TRUE,FALSE);
	addFESlider(FRONTEND_FX_SL,FRONTEND_BOTFORM, FRONTEND_POS2M, FRONTEND_POS2Y+5, AUDIO_VOL_MAX,mixer_GetWavVolume(),FRONTEND_FX );

	// 3d audio
	addTextButton(FRONTEND_3D_FX, FRONTEND_POS3X-25,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_3D_FX),TRUE,FALSE);
	addFESlider(FRONTEND_3D_FX_SL,FRONTEND_BOTFORM, FRONTEND_POS3M, FRONTEND_POS3Y+5,AUDIO_VOL_MAX,mixer_Get3dWavVolume(),FRONTEND_3D_FX );

	// cd audio
	addTextButton(FRONTEND_MUSIC, FRONTEND_POS4X-25,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_MUSIC),TRUE,FALSE);
	addFESlider(FRONTEND_MUSIC_SL,FRONTEND_BOTFORM, FRONTEND_POS4M, FRONTEND_POS4Y+5,AUDIO_VOL_MAX,mixer_GetCDVolume(),FRONTEND_MUSIC );

	// quit.
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);

	//add some text down the side of the form
	addSideText	 (FRONTEND_SIDETEXT ,	FRONTEND_SIDEX,FRONTEND_SIDEY, strresGetString(psStringRes, STR_FE_SIDEOPTIONS));


	return TRUE;
}

BOOL runGameOptions3Menu(void)
{
	UDWORD id;

	processFrontendSnap(FALSE);

	id = widgRunScreen(psWScreen);						// Run the current set of widgets
	switch(id)
	{

	case FRONTEND_FX:
	case FRONTEND_3D_FX:
	case FRONTEND_MUSIC:
		SetMousePos(0,FRONTEND_BOTFORMX+FRONTEND_POS1M+5,mouseY()-3);	// move mouse
		break;

	case FRONTEND_FX_SL:
		mixer_SetWavVolume(widgGetSliderPos(psWScreen,FRONTEND_FX_SL));
		break;

	case FRONTEND_3D_FX_SL:
		mixer_Set3dWavVolume(widgGetSliderPos(psWScreen,FRONTEND_3D_FX_SL));
		break;

	case FRONTEND_MUSIC_SL:
		mixer_SetCDVolume(widgGetSliderPos(psWScreen,FRONTEND_MUSIC_SL));
		break;


	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	default:
		break;
	}

	// If close button pressed then return from this menu.
	if(CancelPressed()) {
		changeTitleMode(TITLE);
	}

	DrawBegin();
	StartCursorSnap(&InterfaceSnap);
	widgDisplayScreen(psWScreen);						// show the widgets currently running
	DrawEnd();
	return TRUE;
}



// ////////////////////////////////////////////////////////////////////////////
// Game Options Menu
BOOL startGameOptionsMenu(void)
{

	UDWORD	w,h;



	addBackdrop();
	addTopForm();
	addBottomForm();


	// difficulty
	addTextButton(FRONTEND_DIFFICULTY,  FRONTEND_POS2X-25,FRONTEND_POS2Y, strresGetString(psStringRes, STR_FE_DIFFICULTY),TRUE,FALSE);
	switch(getDifficultyLevel())
	{
	case DL_EASY:
		addTextButton(FRONTEND_DIFFICULTY_R,FRONTEND_POS2M-25,FRONTEND_POS2Y, strresGetString(psStringRes, STR_EASY),TRUE,FALSE);
		break;
	case DL_NORMAL:
		addTextButton(FRONTEND_DIFFICULTY_R,FRONTEND_POS2M-25,FRONTEND_POS2Y, strresGetString(psStringRes, STR_NORMAL),TRUE,FALSE);
		break;
	case DL_HARD:
	default:
		addTextButton(FRONTEND_DIFFICULTY_R,FRONTEND_POS2M-25,FRONTEND_POS2Y, strresGetString(psStringRes, STR_HARD),TRUE,FALSE);
		break;
	}

	// scroll speed.
	addTextButton(FRONTEND_SCROLLSPEED, FRONTEND_POS3X-25,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_SCROLL),TRUE,FALSE);
	addFESlider(FRONTEND_SCROLLSPEED_SL,FRONTEND_BOTFORM, FRONTEND_POS3M, FRONTEND_POS3Y+5, 16,(scroll_speed_accel/100),FRONTEND_SCROLLSPEED);

	// colour stuff
	w = 	iV_GetImageWidth(FrontImages,IMAGE_PLAYER0);
	h = 	iV_GetImageHeight(FrontImages,IMAGE_PLAYER0);
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P0, FRONTEND_POS4M+(0*(w+6)),FRONTEND_POS4Y,w,h,0,IMAGE_PLAYER0	,IMAGE_PLAYERX,TRUE);
//	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P1, FRONTEND_POS6M-(3*(w+4)),FRONTEND_POS6Y,w,h,0,IMAGE_PLAYER1	,IMAGE_HI34,TRUE);
//	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P2, FRONTEND_POS6M-(2*(w+4)),FRONTEND_POS6Y,w,h,0,IMAGE_PLAYER2	,IMAGE_HI34,TRUE);
//	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P3, FRONTEND_POS6M-(1*(w+4)),FRONTEND_POS6Y,w,h,0,IMAGE_PLAYER3	,IMAGE_HI34,TRUE);
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P4, FRONTEND_POS4M+(1*(w+6)),FRONTEND_POS4Y,w,h,0,IMAGE_PLAYER4	,IMAGE_PLAYERX,TRUE);
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P5, FRONTEND_POS4M+(2*(w+6)),FRONTEND_POS4Y,w,h,0,IMAGE_PLAYER5	,IMAGE_PLAYERX,TRUE);
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P6, FRONTEND_POS4M+(3*(w+6)),FRONTEND_POS4Y,w,h,0,IMAGE_PLAYER6	,IMAGE_PLAYERX,TRUE);
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P7, FRONTEND_POS4M+(4*(w+6)),FRONTEND_POS4Y,w,h,0,IMAGE_PLAYER7,IMAGE_PLAYERX,TRUE);

	widgSetButtonState(psWScreen, FE_P0+getPlayerColour(0), WBUT_LOCK);
	addTextButton(FRONTEND_COLOUR,		FRONTEND_POS4X-25,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_CLAN),TRUE,FALSE);

	// quit.
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);

	//add some text down the side of the form
	addSideText	 (FRONTEND_SIDETEXT ,	FRONTEND_SIDEX,FRONTEND_SIDEY, strresGetString(psStringRes, STR_FE_SIDEOPTIONS));

	return TRUE;
}

BOOL runGameOptionsMenu(void)
{
	UDWORD id;

	processFrontendSnap(FALSE);

	id = widgRunScreen(psWScreen);						// Run the current set of widgets
	switch(id)
	{

//	case FRONTEND_GAMMA:
	case FRONTEND_SCROLLSPEED:
		SetMousePos(0,FRONTEND_BOTFORMX+FRONTEND_POS1M+5,mouseY()-3);	// move mouse
		break;

/*	case FRONTEND_FOGTYPE:
	case FRONTEND_FOGTYPE_R:
	if( war_GetFog()	)
	{	// turn off crap fog, turn on vis fog.
		war_SetFog(FALSE);
		avSetStatus(TRUE);
		widgSetString(psWScreen,FRONTEND_FOGTYPE_R, strresGetString(psStringRes,STR_FE_GOODFOG));
	}
	else
	{	// turn off vis fog, turn on normal crap fog.
		avSetStatus(FALSE);
		war_SetFog(TRUE);
		widgSetString(psWScreen,FRONTEND_FOGTYPE_R, strresGetString(psStringRes,STR_FE_CRAPFOG));
	}
	break;
*/

	case FRONTEND_DIFFICULTY:
	case FRONTEND_DIFFICULTY_R:
		switch( getDifficultyLevel() )
		{
		case DL_EASY:
			setDifficultyLevel(DL_NORMAL);
			widgSetString(psWScreen,FRONTEND_DIFFICULTY_R, strresGetString(psStringRes,STR_NORMAL));
			break;
		case DL_NORMAL:
			setDifficultyLevel(DL_HARD);
			widgSetString(psWScreen,FRONTEND_DIFFICULTY_R, strresGetString(psStringRes,STR_HARD) );
			break;
		case DL_HARD:
			setDifficultyLevel(DL_EASY);
			widgSetString(psWScreen,FRONTEND_DIFFICULTY_R, strresGetString(psStringRes,STR_EASY));
			break;
		default: // DL_TOUGH and DL_KILLER
			break;
		}
		break;

	case FRONTEND_SCROLLSPEED_SL:
		scroll_speed_accel = widgGetSliderPos(psWScreen,FRONTEND_SCROLLSPEED_SL) * 100; //0-1600
		if(scroll_speed_accel ==0)		// make sure you CAN scroll.
		{
			scroll_speed_accel = 100;
		}
		break;

	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	case FE_P0:
		widgSetButtonState(psWScreen, FE_P0, WBUT_LOCK);
//		widgSetButtonState(psWScreen, FE_P1, 0);
//		widgSetButtonState(psWScreen, FE_P2, 0);
//		widgSetButtonState(psWScreen, FE_P3, 0);
		widgSetButtonState(psWScreen, FE_P4, 0);
		widgSetButtonState(psWScreen, FE_P5, 0);
		widgSetButtonState(psWScreen, FE_P6, 0);
		widgSetButtonState(psWScreen, FE_P7, 0);
		setPlayerColour(0,0);
		break;
	case FE_P4:
		widgSetButtonState(psWScreen, FE_P0, 0);
	//	widgSetButtonState(psWScreen, FE_P1, 0);
	//	widgSetButtonState(psWScreen, FE_P2, 0);
	//	widgSetButtonState(psWScreen, FE_P3, 0);
		widgSetButtonState(psWScreen, FE_P4, WBUT_LOCK);
		widgSetButtonState(psWScreen, FE_P5, 0);
		widgSetButtonState(psWScreen, FE_P6, 0);
		widgSetButtonState(psWScreen, FE_P7, 0);
		setPlayerColour(0,4);
		break;
	case FE_P5:
		widgSetButtonState(psWScreen, FE_P0, 0);
	//	widgSetButtonState(psWScreen, FE_P1, 0);
	//	widgSetButtonState(psWScreen, FE_P2, 0);
	//	widgSetButtonState(psWScreen, FE_P3, 0);
		widgSetButtonState(psWScreen, FE_P4, 0);
		widgSetButtonState(psWScreen, FE_P5, WBUT_LOCK);
		widgSetButtonState(psWScreen, FE_P6, 0);
		widgSetButtonState(psWScreen, FE_P7, 0);
		setPlayerColour(0,5);
		break;
	case FE_P6:
		widgSetButtonState(psWScreen, FE_P0, 0);
	//	widgSetButtonState(psWScreen, FE_P1, 0);
	//	widgSetButtonState(psWScreen, FE_P2, 0);
	//	widgSetButtonState(psWScreen, FE_P3, 0);
		widgSetButtonState(psWScreen, FE_P4, 0);
		widgSetButtonState(psWScreen, FE_P5, 0);
		widgSetButtonState(psWScreen, FE_P6, WBUT_LOCK);
		widgSetButtonState(psWScreen, FE_P7, 0);
		setPlayerColour(0,6);
		break;
	case FE_P7:
		widgSetButtonState(psWScreen, FE_P0, 0);
	//	widgSetButtonState(psWScreen, FE_P1, 0);
	//	widgSetButtonState(psWScreen, FE_P2, 0);
	//	widgSetButtonState(psWScreen, FE_P3, 0);
		widgSetButtonState(psWScreen, FE_P4, 0);
		widgSetButtonState(psWScreen, FE_P5, 0);
		widgSetButtonState(psWScreen, FE_P6, 0);
		widgSetButtonState(psWScreen, FE_P7, WBUT_LOCK);
		setPlayerColour(0,7);
		break;

	default:
		break;
	}

	// If close button pressed then return from this menu.
	if(CancelPressed()) {
		changeTitleMode(TITLE);
	}

	DrawBegin();
	StartCursorSnap(&InterfaceSnap);
	widgDisplayScreen(psWScreen);						// show the widgets currently running
	DrawEnd();
	return TRUE;
}



// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// common widgets.

void addBackdrop(void)
{
	W_FORMINIT		sFormInit;

	memset(&sFormInit, 0, sizeof(W_FORMINIT));				// Backdrop
	sFormInit.formID = 0;
	sFormInit.id = FRONTEND_BACKDROP;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD)( (DISP_WIDTH - HIDDEN_FRONTEND_WIDTH)/2);
	sFormInit.y = (SWORD)( (DISP_HEIGHT - HIDDEN_FRONTEND_HEIGHT)/2);
	sFormInit.width = HIDDEN_FRONTEND_WIDTH-1;
	sFormInit.height = HIDDEN_FRONTEND_HEIGHT-1;
	sFormInit.pDisplay = displayTitleBitmap;
	widgAddForm(psWScreen, &sFormInit);
}

// ////////////////////////////////////////////////////////////////////////////

void removeBackdrop(void)
{
	widgDelete( psWScreen, FRONTEND_BACKDROP );
}

// ////////////////////////////////////////////////////////////////////////////

void addBottomForm(void)
{
	W_FORMINIT		sFormInit;
	memset(&sFormInit, 0, sizeof(W_FORMINIT));

	sFormInit.formID = FRONTEND_BACKDROP;
	sFormInit.id = FRONTEND_BOTFORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = FRONTEND_BOTFORMX;
	sFormInit.y = FRONTEND_BOTFORMY;
	sFormInit.width = FRONTEND_BOTFORMW;
	sFormInit.height = FRONTEND_BOTFORMH;

	sFormInit.pDisplay = intOpenPlainForm;
	sFormInit.disableChildren = TRUE;

	widgAddForm(psWScreen, &sFormInit);
}

// ////////////////////////////////////////////////////////////////////////////

void removeBottomForm( void )
{
	widgDelete( psWScreen, FRONTEND_BOTFORM );
}

// ////////////////////////////////////////////////////////////////////////////

void addTopForm(void)
{
	W_FORMINIT		sFormInit;

	memset(&sFormInit, 0, sizeof(W_FORMINIT));

	sFormInit.formID = FRONTEND_BACKDROP;
	sFormInit.id = FRONTEND_TOPFORM;
	sFormInit.style = WFORM_PLAIN;

	if(titleMode == MULTIOPTION)
	{
		sFormInit.x		= FRONTEND_TOPFORM_WIDEX;
		sFormInit.y		= FRONTEND_TOPFORM_WIDEY;
		sFormInit.width = FRONTEND_TOPFORM_WIDEW;
		sFormInit.height= FRONTEND_TOPFORM_WIDEH;
	}
	else

	{
		sFormInit.x		= FRONTEND_TOPFORMX;
		sFormInit.y		= FRONTEND_TOPFORMY;
		sFormInit.width = FRONTEND_TOPFORMW;
		sFormInit.height= FRONTEND_TOPFORMH;
	}
	sFormInit.pDisplay = intDisplayPlainForm;
	widgAddForm(psWScreen, &sFormInit);

	sFormInit.formID= FRONTEND_TOPFORM;
	sFormInit.id	= FRONTEND_LOGO;
	sFormInit.x		= (short)((sFormInit.width/2)-(FRONTEND_LOGOW/2)); //115;
	sFormInit.y		= (short)((sFormInit.height/2)-(FRONTEND_LOGOH/2));//18;
	sFormInit.width = FRONTEND_LOGOW;
	sFormInit.height= FRONTEND_LOGOH;
	sFormInit.pDisplay= displayLogo;
	widgAddForm(psWScreen, &sFormInit);
}

// ////////////////////////////////////////////////////////////////////////////

void removeTopForm( void )
{
	widgDelete( psWScreen, FRONTEND_TOPFORM );
}

// ////////////////////////////////////////////////////////////////////////////
void addTextButton(UDWORD id,  UDWORD PosX, UDWORD PosY, char *txt,BOOL bAlign,BOOL bGrey)
{
	W_BUTINIT		sButInit;
	memset(&sButInit, 0, sizeof(W_BUTINIT));

	sButInit.formID = FRONTEND_BOTFORM;
	sButInit.id = id;
	sButInit.x = (short)PosX;
	sButInit.y = (short)PosY;

	if(bAlign)
	{
		sButInit.style = WBUT_PLAIN;
		sButInit.width = (short)(iV_GetTextWidth(txt)+10);//FRONTEND_BUTWIDTH;

		sButInit.x+=35;

	}
	else
	{
		sButInit.style = WBUT_PLAIN | WBUT_TXTCENTRE;
		sButInit.width = FRONTEND_BUTWIDTH;
	}


	sButInit.pUserData = (void*)bGrey;				// store disable state

	sButInit.height = FRONTEND_BUTHEIGHT;
	sButInit.pDisplay = displayTextOption;
	sButInit.FontID = FEFont;
	sButInit.pText = txt;
	widgAddButton(psWScreen, &sButInit);

	if(bGrey)										// dont allow clicks to this button...
	{
		widgSetButtonState(psWScreen,id,WBUT_DISABLE);
	}

}

// ////////////////////////////////////////////////////////////////////////////
void addFESlider(UDWORD id, UDWORD parent, UDWORD x,UDWORD y,UDWORD stops,UDWORD pos,UDWORD attachID )
{
	W_SLDINIT		sSldInit;

	memset(&sSldInit, 0, sizeof(W_SLDINIT));
	sSldInit.formID		= parent;
	sSldInit.id			= id;
	sSldInit.style		= WSLD_PLAIN;
	sSldInit.x			= (short)x;
	sSldInit.y			= (short)y;
	sSldInit.width		= iV_GetImageWidth(IntImages,IMAGE_SLIDER_BIG);
	sSldInit.height		= iV_GetImageHeight(IntImages,IMAGE_SLIDER_BIG);
	sSldInit.orientation= WSLD_LEFT;
	sSldInit.numStops	= (UBYTE) stops;
	sSldInit.barSize	= iV_GetImageHeight(IntImages,IMAGE_SLIDER_BIG);
	sSldInit.pos		= (UBYTE) pos;
	sSldInit.pDisplay	= displayBigSlider;
	sSldInit.pCallback  = intUpdateQuantitySlider;
	widgAddSlider(psWScreen, &sSldInit);

}

// ////////////////////////////////////////////////////////////////////////////
void addSideText(UDWORD id,  UDWORD PosX, UDWORD PosY, char *txt)
{
#if !defined(PSX) || defined(ROTATEDTEXT)
	W_LABINIT	sLabInit;
	memset(&sLabInit, 0, sizeof(W_LABINIT));

	sLabInit.formID = FRONTEND_BACKDROP;
	sLabInit.id = id;
	sLabInit.style = WLAB_PLAIN;
	sLabInit.x = (short) PosX;
	sLabInit.y = (short) PosY;
	sLabInit.width = 30;
	sLabInit.height = FRONTEND_BOTFORMH;

	sLabInit.FontID = FEFont;

	sLabInit.pDisplay = displayTextAt270;
	sLabInit.pText = txt;
	widgAddLabel(psWScreen, &sLabInit);
#endif
}


void addText(int FontID,UDWORD FormID,UDWORD id,  UDWORD PosX, UDWORD PosY, char *txt, UDWORD attachID,BOOL *State)
{
	W_LABINIT	sLabInit;

	debug( LOG_NEVER, "addText : %s\n", txt );
	memset(&sLabInit, 0, sizeof(W_LABINIT));

	sLabInit.formID = FormID;
	sLabInit.id = id;
	sLabInit.style = WLAB_PLAIN;
	sLabInit.x = (short) PosX;
	sLabInit.y = (short) (PosY+4);
	sLabInit.width = 16;
	sLabInit.height = FRONTEND_BUTHEIGHT;
	sLabInit.FontID = FontID;
	sLabInit.pText = txt;
	sLabInit.UserData	= attachID;
	sLabInit.pUserData	= State;
	sLabInit.pCallback  = intUpdateOptionText;
	widgAddLabel(psWScreen, &sLabInit);
}


#ifdef _MSC_VER
#define snprintf _snprintf
#endif
// ////////////////////////////////////////////////////////////////////////////
// drawing functions

// show a background piccy
void displayTitleBitmap(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
    const size_t sTmpSize = 200;

	char	sTmp[200];			//Couldn't have sTmp[sTmpSize], .net did NOT like that, so for now...

	iV_SetFont(WFont);
	iV_SetTextColour(-1);

#ifdef DEBUG
	snprintf( sTmp, sTmpSize, "Version %s - Built %s - DEBUG", VERSION, __DATE__ );
#else
	snprintf( sTmp, sTmpSize, "Version %s - Built %s", VERSION, __DATE__ );
#endif

	pie_DrawText270(sTmp,DISP_WIDTH-10,DISP_HEIGHT-15);
}

// ////////////////////////////////////////////////////////////////////////////
// show warzone logo
void displayLogo(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	iV_DrawTransImage(FrontImages,IMAGE_FE_LOGO,xOffset+psWidget->x,yOffset+psWidget->y);
}




// ////////////////////////////////////////////////////////////////////////////
// show a text option.
void displayTextOption(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	SDWORD			fx,fy, fw;
	W_BUTTON		*psBut;
	BOOL			hilight = FALSE;
	BOOL			greyOut = (BOOL) psWidget->pUserData;			// if option is unavailable.

	psBut = (W_BUTTON *)psWidget;
	iV_SetFont(psBut->FontID);

	if(widgGetMouseOver(psWScreen) == psBut->id)					// if mouse is over text then hilight.
	{
		hilight = TRUE;
	}

  	fw = iV_GetTextWidth(psBut->pText);
	fy = yOffset + psWidget->y + (psWidget->height - iV_GetTextLineSize())/2 - iV_GetTextAboveBase();

	if (psWidget->style & WBUT_TXTCENTRE)							//check for centering, calculate offset.
	{
		fx = xOffset + psWidget->x + ((psWidget->width - fw) / 2);
	}
	else
	{
		fx = xOffset + psWidget->x;
	}

	if(greyOut)														// unavailable
	{
		iV_SetTextColour(PIE_TEXT_DARKBLUE);
	}
	else															// available
	{
		if(hilight)													// hilight
		{
			iV_SetTextColour(PIE_TEXT_WHITE);
//#ifdef PSX
//			displayHilightPulseBox( fx-4,fy+iV_GetTextAboveBase()-iV_GetTextBelowBase(),
//									fx+fw,fy+iV_GetTextBelowBase());
//#endif
		}
		else														// dont highlight
		{
			iV_SetTextColour(PIE_TEXT_LIGHTBLUE);//(unsigned short)iV_PaletteNearestColour(129,142,184)
		}
	}

	iV_DrawText( psBut->pText, fx, fy);



	if(!greyOut)													// dont snap to unavailable buttons.
	{
//		AddCursorSnap(&InterfaceSnap, (SWORD)(fx+10) ,(short) fy,psWidget->formID,psWidget->id,NULL);
		if (psWidget->style & WBUT_TXTCENTRE) {							//check for centering, calculate offset.
//			DBPRINTF(("%d : %s\n",fx+fw/2,psBut->pText);
			AddCursorSnap(&InterfaceSnap, (SWORD)(fx+fw/2) ,(short) fy,psWidget->formID,psWidget->id,&FrontendBias);
		} else {
//			DBPRINTF(("%d : %s\n",fx+10,psBut->pText);
			AddCursorSnap(&InterfaceSnap, (SWORD)(fx+10) ,(short) fy,psWidget->formID,psWidget->id,&FrontendBias);
		}
	}

	return;
}


// ////////////////////////////////////////////////////////////////////////////
#if !defined(PSX) || defined(ROTATEDTEXT)

// show text written on its side.
void displayTextAt270(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	SDWORD		fx,fy;
	W_LABEL		*psLab;

	psLab = (W_LABEL *)psWidget;

	iV_SetFont(FEFont);


	iV_SetTextColour(PIE_TEXT_WHITE);

	fx = xOffset + psWidget->x;

	fy = yOffset + psWidget->y + iV_GetTextWidth(psLab->aText) ;


	iV_DrawText270( psLab->aText, fx, fy);
}
#endif


// ////////////////////////////////////////////////////////////////////////////
// show, well have a guess..
static void displayBigSlider(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_SLIDER *Slider = (W_SLIDER*)psWidget;
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y;
	SWORD sx;


	iV_DrawTransImage(IntImages,IMAGE_SLIDER_BIG,x+STAT_SLD_OX,y+STAT_SLD_OY);			// draw bdrop

	sx = (SWORD)((Slider->width-3 - Slider->barSize) * Slider->pos / Slider->numStops);	// determine pos.
	iV_DrawTransImage(IntImages,IMAGE_SLIDER_BIGBUT,x+3+sx,y+3);								//draw amount


}


//// Given a string id, set a text buttons dimensions.
////
//void SetTextButtonExt(W_BUTINIT *psButInit,UWORD x,UWORD y,UDWORD StringID)
//{
//	psButInit->x = x;
//	psButInit->y = y;
//	psButInit->width = iV_GetTextWidth(strresGetString(psStringRes,StringID));
//	psButInit->height = iV_GetTextLineSize();
//}


// Placed here to avoid automatic inlining in InGameOp.c by the Playstation compiler.
//
BOOL addIGTextButton(UDWORD id,UWORD y,UDWORD StringID,UDWORD Style)
{
	W_BUTINIT sButInit;

	memset(&sButInit, 0, sizeof(W_BUTINIT ));

	//resume
	sButInit.formID		= INTINGAMEOP;
	sButInit.id			= id;
	sButInit.style		= Style;
//	SetTextButtonExt(&sButInit,INTINGAMEOP_1_X,y,StringID);


	sButInit.x			= INTINGAMEOP_1_X;
	sButInit.y			= y;
	sButInit.width		= INTINGAMEOP_OP_W;
	sButInit.height		= INTINGAMEOP_OP_H;

	sButInit.FontID		= WFont;
	sButInit.pDisplay	= displayTextOption;
	sButInit.pText		= strresGetString(psStringRes,StringID);
	widgAddButton(psWScreen, &sButInit);

	return TRUE;
}



