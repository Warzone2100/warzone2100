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

#include <stdio.h>
#include "frame.h"
#include "winmain.h"
#include "objects.h"
#include "display.h"
#include "widget.h"
/* Includes direct access to render library */
#include "ivisdef.h"
#include "piestate.h"

#include "keyedit.h"
#include "piefunc.h"
#include "warzoneconfig.h"

#include "vid.h"

#include "display3d.h"
#include "intdisplay.h"
#include "audio.h"					// for sound.
#include "audio_id.h"				// for sound..

#include "cdaudio.h"
#include "mixer.h"
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
#include "netplay.h"




#define VERSION_STRING	"VER CP0.1"

extern BOOL bSubtitles;

extern VOID intUpdateOptionText(struct _widget *psWidget, struct _w_context *psContext);

extern CURSORSNAP InterfaceSnap;
extern VOID ProcessCursorSnap(VOID);

int StartWithGame = 1;	// New game starts in Cam 1.

char OnString[]={"On "};
char OffString[]={"Off"};

#ifndef PSX
STRING	strFog[MAX_STR_LENGTH];
STRING	strTrans[MAX_STR_LENGTH];
#endif



// ////////////////////////////////////////////////////////////////////////////
// Local Definitions
// iPalette			titlePalette;
int				FEFont;
//int				FEBigFont;
#ifndef NON_INTERACT
char			pLevelName[MAX_LEVEL_NAME_SIZE+1];	//256];			// vital! the wrf file to use.
#else
char			pLevelName[]="ROCKIES";
#endif
//#ifdef PSX
//STRING			saveGameName[256];			//the name of the save game to load from the front end
//#endif
BOOL			bForceEditorLoaded = FALSE;
BOOL			bUsingKeyboard = FALSE;		// to disable mouse pointer when using keys.
BOOL			bUsingSlider   = FALSE;

static tMode	g_tModeNext;


// This is used on the PSX so that things like the mission result screen
// can know if were in the fast play game and behave a bit differently if so.
// Currently just returns FALSE on the PC.
BOOL GetInFastPlay(void)
{

	return FALSE;

}


// ////////////////////////////////////////////////////////////////////////////
// extern Definitions

//extern W_SCREEN		*psWScreen;					//The widget screen

extern char	SaveGamePath[];

extern BOOL firstcall;
extern IMAGEFILE *FrontImages;


// ////////////////////////////////////////////////////////////////////////////
// Function Definitions

VOID		processFrontendSnap		(BOOL bHideCursor);
VOID		changeTitleMode			(tMode mode);  
BOOL		startTitleMenu			(VOID);
BOOL		runTitleMenu			(VOID);
VOID		startSinglePlayerMenu	(VOID);
BOOL		runSinglePlayerMenu		(VOID);
//BOOL		runDemoMenu				(VOID);
//BOOL		startDemoMenu			(VOID);
BOOL		startTutorialMenu		(VOID);
BOOL		runTutorialMenu			(VOID);
BOOL		startMultiPlayerMenu	(VOID);
BOOL		runMultiPlayerMenu		(VOID);
BOOL		startOptionsMenu		(VOID);
BOOL		runOptionsMenu			(VOID);
BOOL		startGameOptionsMenu	(VOID);
BOOL		runGameOptionsMenu		(VOID);
BOOL		startGameOptions2Menu	(VOID);
BOOL		runGameOptions2Menu		(VOID);
//BOOL		startVideoOptionsMenu	(VOID);
//BOOL		runVideoOptionsMenu		(VOID);
//BOOL		startGraphicsOptionsMenu(VOID);
//BOOL		runGraphicsptionsMenu	(VOID);

VOID		addTopForm				(VOID);
VOID		removeTopForm			(VOID);
VOID		addBottomForm			(VOID);
VOID		removeBottomForm		(VOID);
VOID		addBackdrop				(VOID);
VOID		removeBackdrop			(VOID);

VOID		addTextButton			(UDWORD id,  UDWORD PosX, UDWORD PosY, STRING *txt,BOOL bAlignLeft,BOOL bGrey);
VOID 		addText					(int FontID,UDWORD FormID,UDWORD id,  UDWORD PosX, UDWORD PosY, STRING *txt, UDWORD attachID,BOOL *State);
VOID		addSideText				(UDWORD id,  UDWORD PosX, UDWORD PosY, STRING *txt);
VOID		addFESlider				(UDWORD id, UDWORD parent, UDWORD x,UDWORD y,UDWORD stops,UDWORD pos,UDWORD attachID);

VOID		displayLogo				(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
VOID		displayTitleBitmap		(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
VOID		displayTextOption		(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
VOID		displayTextAt270		(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
static VOID	displayBigSlider		(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);





// Returns TRUE if escape key pressed on PC or close button pressed on Playstation.
//
BOOL CancelPressed(VOID)
{

	if(keyPressed(KEY_ESC)) {
		return TRUE;
	}


	return FALSE;
}
// ////////////////////////////////////////////////////////////////////////////
// for cursorsnap stuff on pc
#ifndef PSX
VOID processFrontendSnap(BOOL bHideCursor)
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
#endif

// ////////////////////////////////////////////////////////////////////////////
// Change Mode
VOID changeTitleMode(tMode mode)
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
#ifndef PSX
	case GAME2:
		startGameOptions2Menu();
		break;
#endif
	case TUTORIAL:
		startTutorialMenu();
		break;
	case OPTIONS:
//#ifndef PSX
		startOptionsMenu();
//#else
//		startGameOptionsMenu();
//#endif
		break;
	case TITLE:
		startTitleMenu();
		break;
#ifndef PSX

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
#endif
	case STARTGAME:
	case QUIT:
	case LOADSAVEGAME:
		bUsingKeyboard = FALSE;
		bForceEditorLoaded = FALSE;
	case SHOWINTRO:
		break;

	default:
		DBERROR(("Unknown title mode requested"));
		break;
	}

	return;
}

 
// ////////////////////////////////////////////////////////////////////////////
// Title Screen
BOOL startTitleMenu(VOID)
{	


//	widgDelete(psWScreen,1);	// close reticule if it's open. MAGIC NUMBERS?
	intRemoveReticule();

	addBackdrop();
	addTopForm();
	addBottomForm();


#ifndef PSX
	#ifdef COVERMOUNT	// no multiplayer								
		addTextButton(FRONTEND_TUTORIAL,	FRONTEND_POS2X,FRONTEND_POS2Y, "Demo" ,FALSE,FALSE);
	#ifdef  MULTIDEMO
		addTextButton(FRONTEND_MULTIPLAYER,	FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_MULTI)   ,FALSE,FALSE);	
	#else
		addTextButton(FRONTEND_MULTIPLAYER,	FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_MULTI)   ,FALSE,TRUE);	
	#endif	
		addTextButton(FRONTEND_SINGLEPLAYER,FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_SINGLE),FALSE,TRUE);
		addTextButton(FRONTEND_OPTIONS,		FRONTEND_POS5X,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_OPTIONS) ,FALSE,FALSE);
		addTextButton(FRONTEND_PLAYINTRO,	FRONTEND_POS6X,FRONTEND_POS6Y, strresGetString(psStringRes, STR_FE_INTRO),FALSE,TRUE);		
	#else		// normal
		addTextButton(FRONTEND_SINGLEPLAYER,FRONTEND_POS2X,FRONTEND_POS2Y, strresGetString(psStringRes, STR_FE_SINGLE),FALSE,FALSE);
		if(!bDisableLobby)
		{
			addTextButton(FRONTEND_MULTIPLAYER,	FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_MULTI)   ,FALSE,FALSE);	
		}else{
			addTextButton(FRONTEND_MULTIPLAYER,	FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_MULTI)   ,FALSE,TRUE);	
		}
		addTextButton(FRONTEND_TUTORIAL,	FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_TUT) ,FALSE,FALSE);
		addTextButton(FRONTEND_OPTIONS,		FRONTEND_POS5X,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_OPTIONS) ,FALSE,FALSE);
		addTextButton(FRONTEND_PLAYINTRO,	FRONTEND_POS6X,FRONTEND_POS6Y, strresGetString(psStringRes, STR_FE_INTRO),FALSE,FALSE);
	#endif

	addTextButton(FRONTEND_QUIT,		FRONTEND_POS7X,FRONTEND_POS7Y, strresGetString(psStringRes, STR_FE_QUIT),FALSE,FALSE);

#else
	// PlayStation 
	#ifdef COVERMOUNT
		addTextButton(FRONTEND_SINGLEPLAYER,FRONTEND_POS2X,FRONTEND_POS2Y, strresGetString(psStringRes, STR_FE_SINGLE2),FALSE,FALSE);
		addTextButton(FRONTEND_OPTIONS,		FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_OPTIONS) ,FALSE,FALSE);
		addTextButton(FRONTEND_QUIT,		FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_QUIT),FALSE,FALSE);
	#else
		addTextButton(FRONTEND_SINGLEPLAYER,FRONTEND_POS2X,FRONTEND_POS2Y, strresGetString(psStringRes, STR_FE_SINGLE2),FALSE,FALSE);
		addTextButton(FRONTEND_OPTIONS,		FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_OPTIONS) ,FALSE,FALSE);
#ifdef PSXTUTORIAL
		addTextButton(FRONTEND_TUTORIAL,	FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_TUT) ,FALSE,FALSE);
#else
		addTextButton(FRONTEND_TUTORIAL,	FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_PRACTICE) ,FALSE,FALSE);
#endif
//		addTextButton(FRONTEND_DEMO,		FRONTEND_POS4X,FRONTEND_POS4Y, "Demo Mode",FALSE,FALSE);		// remove on release
		addTextButton(FRONTEND_PLAYINTRO,	FRONTEND_POS5X,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_INTRO),FALSE,FALSE);
	#endif
		addSelectHelp();
		SetCurrentSnapID(&InterfaceSnap,FRONTEND_SINGLEPLAYER);

#endif
	addSideText	 (FRONTEND_SIDETEXT ,	FRONTEND_SIDEX,FRONTEND_SIDEY,strresGetString(psStringRes, STR_FE_SIDEMAIN));

#ifndef PSX
	SetMousePos(0,320,FRONTEND_BOTFORMY+FRONTEND_POS2Y);
	SnapToID(&InterfaceSnap,4);
#endif
	
	return TRUE;
}

#ifndef PSX
static void frontEndCDOK( void )
{
	changeTitleMode( g_tModeNext );
}

static void frontEndCDCancel( void )
{
	changeTitleMode(TITLE);
}

void frontEndCheckCD( tMode tModeNext, CD_INDEX cdIndex )
{
	BOOL	bOK;

	/* save next tmode */
	g_tModeNext = tModeNext;

	if ( !cdspan_DontTest() )
	{
		if ( cdIndex == DISC_EITHER )
		{
			bOK = cdspan_initialCDcheck();
		}
		else
		{
			if ( cdspan_CheckCDPresent( cdIndex ) )
			{
				bOK = TRUE;
			}
			else
			{
				bOK = FALSE;
			}
		}

		if ( bOK == FALSE )
		{
			widgDelete( psWScreen,FRONTEND_BACKDROP );
			showChangeCDBox( psWScreen, cdIndex,
								frontEndCDOK, frontEndCDCancel );
			return;
		}
	}

	changeTitleMode( tModeNext );
}
#endif


BOOL runTitleMenu(VOID)
{
	UDWORD id;

	processFrontendSnap(TRUE);

	id = widgRunScreen(psWScreen);						// Run the current set of widgets 

#ifndef PSX
	if ( !cdspan_ProcessCDChange(id) )
#endif
	{
		switch(id)
		{
			case FRONTEND_QUIT:
#ifndef PSX
			changeTitleMode(CREDITS);
#else
			changeTitleMode(QUIT);
#endif
			break;
#ifndef PSX
		case FRONTEND_MULTIPLAYER:
			frontEndCheckCD(MULTI, DISC_EITHER);
			break;
#endif
		case FRONTEND_SINGLEPLAYER:
			changeTitleMode(SINGLE);
			break;
		case FRONTEND_OPTIONS:
			changeTitleMode(OPTIONS);
			break;
		case FRONTEND_PLAYINTRO:
#ifndef PSX
			frontEndCheckCD(SHOWINTRO, DISC_ONE);
#else
			changeTitleMode(SHOWINTRO);
#endif
			break;
		case FRONTEND_TUTORIAL:
#ifndef PSX
			frontEndCheckCD(TUTORIAL, DISC_ONE);
#else
//			changeTitleMode(TUTORIAL);
			bInFastPlay = TRUE;
			// Practice should be easy.
			setDifficultyLevel(DL_EASY);
			strcpy(pLevelName,"FASTPLAY");
			changeTitleMode(STARTGAME);
 #ifdef LOADINGBACKDROPS
			AddLoadingBackdrop(TRUE);
 #else
			initLoadingScreen(TRUE,TRUE);
 #endif
#endif
			break;
		default:
			break;
		}
	}

	DrawBegin();
	StartCursorSnap(&InterfaceSnap);
	widgDisplayScreen(psWScreen);						// show the widgets currently running
	DrawEnd();

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// Tutorial Menu

BOOL startTutorialMenu(VOID)
{
	addBackdrop();
	addTopForm();
	addBottomForm();
	
#ifndef PSX
	addTextButton(FRONTEND_TUTORIAL, FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes,STR_FE_TUT),FALSE,FALSE);
	addTextButton(FRONTEND_FASTPLAY, FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes,STR_FE_FASTPLAY),FALSE,FALSE);
	addSideText	 (FRONTEND_SIDETEXT ,FRONTEND_SIDEX,FRONTEND_SIDEY,strresGetString(psStringRes,STR_FE_SIDETUT));
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);			 
#endif
	SetCurrentSnapID(&InterfaceSnap,FRONTEND_FASTPLAY);



	return TRUE;
}

BOOL runTutorialMenu(VOID)
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

VOID startSinglePlayerMenu(VOID)
{


	addBackdrop();
	addTopForm();
	addBottomForm();
#ifndef PSX

#ifdef COVERMOUNT						// reduce single player options
	addTextButton(FRONTEND_NEWGAME,  FRONTEND_POS5X,FRONTEND_POS5Y,	strresGetString(psStringRes,STR_FE_NEW) ,FALSE,TRUE);
	addTextButton(FRONTEND_LOADGAME, FRONTEND_POS6X,FRONTEND_POS6Y, strresGetString(psStringRes,STR_FE_LOAD),FALSE,TRUE);
#else
	addTextButton(FRONTEND_LOADGAME, FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes,STR_FE_LOAD),FALSE,FALSE);
	addTextButton(FRONTEND_NEWGAME,  FRONTEND_POS3X,FRONTEND_POS3Y,strresGetString(psStringRes,STR_FE_NEW) ,FALSE,FALSE);
#endif
	addSideText	 (FRONTEND_SIDETEXT ,FRONTEND_SIDEX,FRONTEND_SIDEY,strresGetString(psStringRes,STR_FE_SIDESINGLE1));
	SetCurrentSnapID(&InterfaceSnap,FRONTEND_LOADGAME);
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);			 


#else	//psx
	StartMenuDepth = 0;
	if(QACheatMode == TRUE) {
		YOffset = -32;
		addTextButton(FRONTEND_LOADCAM2, FRONTEND_POS4X,FRONTEND_POS4Y+YOffset, "Start CAM II",FALSE,FALSE);
		addTextButton(FRONTEND_LOADCAM3, FRONTEND_POS5X,FRONTEND_POS5Y+YOffset, "Start CAM III",FALSE,FALSE);
		addTextButton(FRONTEND_QUIT,     FRONTEND_POS6X,FRONTEND_POS6Y+YOffset,strresGetString(psStringRes,STR_FE_RETURN),FALSE,FALSE);
	} else {
		YOffset = 0;
		addTextButton(FRONTEND_QUIT,     FRONTEND_POS5X,FRONTEND_POS5Y+YOffset,strresGetString(psStringRes,STR_FE_RETURN),FALSE,FALSE);
	}

 #ifdef COVERMOUNT
	addTextButton(FRONTEND_TUTORIAL, FRONTEND_POS2X,FRONTEND_POS2Y+YOffset, strresGetString(psStringRes,STR_FE_TUT),FALSE,FALSE);
	addTextButton(FRONTEND_FASTPLAY, FRONTEND_POS3X,FRONTEND_POS3Y+YOffset, strresGetString(psStringRes,STR_FE_FASTPLAY),FALSE,FALSE);
	SetCurrentSnapID(&InterfaceSnap,FRONTEND_TUTORIAL);
 #else
	addTextButton(FRONTEND_NEWGAME,  FRONTEND_POS2X,FRONTEND_POS2Y+YOffset,strresGetString(psStringRes,STR_FE_NEW) ,FALSE,FALSE);
	addTextButton(FRONTEND_LOADGAME, FRONTEND_POS3X,FRONTEND_POS3Y+YOffset, strresGetString(psStringRes,STR_FE_LOAD),FALSE,FALSE);

	SetCurrentSnapID(&InterfaceSnap,FRONTEND_LOADGAME);
 #endif
	addSelectHelp();
	addCancelHelp();

//	addSideText	 (FRONTEND_SIDETEXT ,FRONTEND_SIDEX,FRONTEND_SIDEY,strresGetString(psStringRes,STR_FE_SIDESINGLE2));
#endif
}

void endSinglePlayerMenu( void )
{
	removeTopForm();
	removeBottomForm();
	removeBackdrop();
}

void frontEndNewGame( void )
{
	switch(StartWithGame) {
		case 1:
			strcpy(pLevelName,DEFAULT_LEVEL);
			seq_ClearSeqList();
		#ifndef PSX
			seq_AddSeqToList("CAM1\\c001.rpl",NULL,"CAM1\\c001.txa",FALSE,0);
			/*
			seq_AddTextForVideo("Dawn, July 4th, 2066", 20, 432, 0, 299);
			seq_AddTextForVideo("Project HQ.", 0, 0, 0, 299);
			seq_AddTextForVideo("A New Era", 0, 0, 0, 299);
			seq_AddTextForVideo("Morning, July 4th, 2066", 20, 432, 399, 699);
			seq_AddTextForVideo("In-flight to Western Sector", 0, 0, 399, 699);
			seq_AddTextForVideo("Team Alpha nears its destination", 0, 0, 399, 699);
			*/
		#else
			seq_ClearSeqList();

			seq_AddSeqToList("CAM1\\C001.STR","1656f","CAM1\\c001.txa",FALSE,0);

#if(0)
			seq_AddTextForVideo("Dawn July 4th 2066", 10, 20, 0, 299, FALSE,0);
			seq_AddTextForVideo("Project HQ.", 10,200, 0, 299, FALSE,0);
			seq_AddTextForVideo("A New Era", 0, 0, 0, 299, FALSE,0);
			seq_AddTextForVideo("Morning, July 4th, 2066", 10, 20, 399, 699, FALSE,0);
			seq_AddTextForVideo("In-flight to Western Sector.", 10, 200, 399, 699, FALSE,0);
			seq_AddTextForVideo("Team Alpha nears its destination", 0, 0, 399, 699);
#endif
		#endif
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
#ifndef PSX
	if(strlen(sRequestResult))
	{
		strcpy(saveGameName,sRequestResult);
		changeTitleMode(LOADSAVEGAME);
	}
	SetCurrentSnapID(&InterfaceSnap,FRONTEND_LOADGAME);
#endif
}

BOOL runSinglePlayerMenu(VOID)
{
	UDWORD id;

	processFrontendSnap(TRUE);

#ifndef PSX
	if(bLoadSaveUp)
	{
		if(runLoadSave(FALSE))// check for file name.
		{
			loadOK();
			SetCurrentSnapID(&InterfaceSnap,FRONTEND_LOADGAME);
		}
	}
	else
#endif
	{

	id = widgRunScreen(psWScreen);						// Run the current set of widgets 


#ifndef PSX
	/* GJ to TC - this call processes the CD change widget box */
	if ( !cdspan_ProcessCDChange(id) )
#endif
	{
		switch(id)
		{
			case FRONTEND_NEWGAME:
#ifndef PSX		   // ffs
				if ( cdspan_CheckCDPresent( getCDForCampaign(1) ) )
				{
					frontEndNewGame();
				}
				else
				{
					endSinglePlayerMenu();
					showChangeCDBox( psWScreen, getCDForCampaign(1),
										frontEndNewGame, startSinglePlayerMenu );
				}
#else
				StartWithGame = 1;
 #ifdef PSX_DIFFICULTY_MENU
				StartMenuDepth = 1;
				endSinglePlayerMenu();
				startDifficultyMenu();
 #else
				frontEndNewGame();
				initLoadingScreen(TRUE,TRUE);
 #endif
#endif

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


//#ifndef PSX		// ffs tc
			case FRONTEND_LOADGAME:

				addLoadSave(LOAD_FRONTEND,SaveGamePath,"gam",strresGetString(psStringRes,STR_MR_LOAD_GAME));	// change mode when loadsave returns

				break;
//#endif


			case FRONTEND_QUIT:
				changeTitleMode(TITLE);
				break;


#if defined(PSX) && defined(COVERMOUNT)
			case FRONTEND_TUTORIAL:
				strcpy(pLevelName,TUTORIAL_LEVEL);
				changeTitleMode(STARTGAME);
 #ifdef LOADINGBACKDROPS
				AddLoadingBackdrop(TRUE);
 #else
				initLoadingScreen(TRUE,TRUE);
 #endif
				break;

			case FRONTEND_FASTPLAY:
				strcpy(pLevelName,"FASTPLAY");
				changeTitleMode(STARTGAME);
 #ifdef LOADINGBACKDROPS
				AddLoadingBackdrop(TRUE);
 #else
				initLoadingScreen(TRUE,TRUE);
 #endif
				break;
#endif

			default:
				break;
		}
	}

	if(CancelPressed()) 
	{

		changeTitleMode(TITLE);

	}

	}

	DrawBegin();
	StartCursorSnap(&InterfaceSnap);
#ifndef PSX
	if(!bLoadSaveUp)										// if save/load screen is up
	{
		widgDisplayScreen(psWScreen);						// show the widgets currently running
	}
	if(bLoadSaveUp)										// if save/load screen is up
	{
		displayLoadSave();
	}
#endif




	DrawEnd();
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// Demo menus, remove for release.
/*
BOOL startDemoMenu(VOID)
{
	addBackdrop();
	addTopForm();
	addBottomForm();
	
	addSideText	 (FRONTEND_SIDETEXT ,	FRONTEND_SIDEX,FRONTEND_SIDEY,"DEMO");

#ifdef PSX
#if(0)		// none of these demos work anymore !
	addTextButton(FRONTEND_DEMO1,	FRONTEND_POS2X,FRONTEND_POS2Y, "Rocky Mountains",FALSE,FALSE);
	addTextButton(FRONTEND_DEMO2,   FRONTEND_POS3X,FRONTEND_POS3Y, "New Paradigm HQ",FALSE,FALSE);
	addTextButton(FRONTEND_DEMO3,	FRONTEND_POS4X,FRONTEND_POS4Y, "City Dam",FALSE,FALSE);
	addTextButton(FRONTEND_DEMO4,	FRONTEND_POS5X,FRONTEND_POS5Y, "City Crater",FALSE,FALSE);
#endif
#endif

#ifndef PSX
	addTextButton(FRONTEND_DEMO1,	FRONTEND_POS2X,FRONTEND_POS2Y, "Rocky Mountains",FALSE,FALSE);
	addTextButton(FRONTEND_DEMO2,   FRONTEND_POS3X,FRONTEND_POS3Y, "New Paradigm HQ",FALSE,FALSE);
	addTextButton(FRONTEND_DEMO3,	FRONTEND_POS4X,FRONTEND_POS4Y, "City Dam",FALSE,FALSE);
	addTextButton(FRONTEND_DEMO4,	FRONTEND_POS5X,FRONTEND_POS5Y, "City Crater",FALSE,FALSE);
	addTextButton(FRONTEND_DEMO5,	FRONTEND_POS6X,FRONTEND_POS6Y, "Empty",FALSE,FALSE);
#endif


#ifndef PSX
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);
#else
	addTextButton(FRONTEND_QUIT,FRONTEND_POS5X,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_RETURN),TRUE,FALSE);
#endif
	return TRUE;
}

BOOL runDemoMenu(VOID)
{
	UDWORD id;

	processFrontendSnap(FALSE);

	id = widgRunScreen(psWScreen);						// Run the current set of widgets 
	switch(id)
	{
//#ifdef PSX
//	case FRONTEND_DEMO1:
//		strcpy(pLevelName,"FASTPLAY");
//		changeTitleMode(STARTGAME);
//		break;
//#else
	case FRONTEND_DEMO1:
		strcpy(pLevelName,"DEMO1");
		changeTitleMode(STARTGAME);
		break;
	case FRONTEND_DEMO2:
		strcpy(pLevelName,"DEMO2");
		changeTitleMode(STARTGAME);
		break;
	case FRONTEND_DEMO3:
		strcpy(pLevelName,"DEMO3");
		changeTitleMode(STARTGAME);
		break;
	case FRONTEND_DEMO4:
		strcpy(pLevelName,"DEMO4");
		changeTitleMode(STARTGAME);
		break;
	case FRONTEND_DEMO5:
		strcpy(pLevelName,"DEMO5");
		changeTitleMode(STARTGAME);
		break;
//#endif
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
*/

// ////////////////////////////////////////////////////////////////////////////
// Multi Player Menu

#ifndef PSX
BOOL startMultiPlayerMenu(VOID)
{
	addBackdrop();
	addTopForm();
	addBottomForm();
	
	addSideText	 (FRONTEND_SIDETEXT ,	FRONTEND_SIDEX,FRONTEND_SIDEY,strresGetString(psStringRes, STR_FE_SIDEMULTI));

	addTextButton(FRONTEND_HOST,     FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_HOST),FALSE,FALSE);
	addTextButton(FRONTEND_JOIN,     FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_JOIN),FALSE,FALSE);

#ifdef MULTIDEMO
	addTextButton(FRONTEND_FORCEEDIT,FRONTEND_POS5X,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_FORCEEDIT),FALSE,TRUE);
	addTextButton(FRONTEND_SKIRMISH, FRONTEND_POS6X,FRONTEND_POS6Y, strresGetString(psStringRes, STR_FE_SKIRMISH ),FALSE,TRUE);
#else
	addTextButton(FRONTEND_FORCEEDIT,FRONTEND_POS5X,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_FORCEEDIT),FALSE,FALSE);
	addTextButton(FRONTEND_SKIRMISH, FRONTEND_POS6X,FRONTEND_POS6Y, strresGetString(psStringRes, STR_FE_SKIRMISH ),FALSE,FALSE);
#endif
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);

	SetMousePos(0,320,FRONTEND_BOTFORMY+FRONTEND_POS3Y);
	SnapToID(&InterfaceSnap,3);
	return TRUE;
}

BOOL runMultiPlayerMenu(VOID)
{
	UDWORD id;
//	PLAYERSTATS	nullStats;
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
/*			if (!resLoad("wrf\\forcedit.wrf", 500,
						 DisplayBuffer, displayBufferSize,
						 psGameHeap))				//need the object heaps to have been set up before loading 
			{
				return FALSE;
			}
*/
			if (!resLoad("wrf\\piestats.wrf", 501,
						 DisplayBuffer, displayBufferSize,
						 psGameHeap))				//need the object heaps to have been set up before loading 
			{
				return FALSE;
			}
			
			if (!resLoad("wrf\\forcedit2.wrf", 502,
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
#endif









// ////////////////////////////////////////////////////////////////////////////
// Options Menu
BOOL startOptionsMenu(VOID)
{
	addBackdrop();
	addTopForm();
	addBottomForm();
#ifndef PSX
	addSideText	 (FRONTEND_SIDETEXT ,	FRONTEND_SIDEX,FRONTEND_SIDEY, strresGetString(psStringRes, STR_FE_SIDEOPTIONS));
	addTextButton(FRONTEND_GAMEOPTIONS2,FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_GRAPHICS),FALSE,FALSE);
	addTextButton(FRONTEND_GAMEOPTIONS,	FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_GAME),FALSE,FALSE);
	addTextButton(FRONTEND_KEYMAP,		FRONTEND_POS5X,FRONTEND_POS5Y, strresGetString(psStringRes, STR_KM_KEYMAP),FALSE,FALSE);
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);

	SetMousePos(0,320,FRONTEND_BOTFORMY+FRONTEND_POS3Y);
	SnapToID(&InterfaceSnap,3);
#else
	addTextButton(FRONTEND_SOUNDOPTIONS,FRONTEND_POS3X,FRONTEND_POS2Y, strresGetString(psStringRes, STR_FE_SOUNDOPT),FALSE,FALSE);
	addTextButton(FRONTEND_CONTROLOPTIONS,FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_CONTROLOPT),FALSE,FALSE);
	addTextButton(FRONTEND_DISPLAYOPTIONS,FRONTEND_POS3X,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_DISPLAYOPT),FALSE,FALSE);
	addTextButton(FRONTEND_QUIT,     FRONTEND_POS6X,FRONTEND_POS6Y,strresGetString(psStringRes,STR_FE_RETURN),FALSE,FALSE);
	SetCurrentSnapID(&InterfaceSnap,FRONTEND_SOUNDOPTIONS);
	OptionMenuDepth = 0;
#endif

	return TRUE;
}



BOOL runOptionsMenu(VOID)
{
	UDWORD id;

	processFrontendSnap(TRUE);

	id = widgRunScreen(psWScreen);						// Run the current set of widgets 
	switch(id)
	{
#ifndef PSX
	case FRONTEND_GAMEOPTIONS:
		changeTitleMode(GAME);
		break;
	case FRONTEND_GAMEOPTIONS2:
		changeTitleMode(GAME2);
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
#else
	case FRONTEND_SOUNDOPTIONS:
		OptionMenuDepth = 1;
		endMenu();
		startSoundOptionsMenu();
		break;

	case FRONTEND_CONTROLOPTIONS:
		OptionMenuDepth = 1;
		endMenu();
		startControlOptionsMenu();
		break;

	case FRONTEND_DISPLAYOPTIONS:
		OptionMenuDepth = 1;
		endMenu();
		startDisplayOptionsMenu();
		break;

	case FRONTEND_QUIT:
		if(OptionMenuDepth == 0) {
			changeTitleMode(TITLE);
		} else {
			OptionMenuDepth = 0;
			changeTitleMode(OPTIONS);
		}
		break;
#endif

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
// Graphics Options Menu
/*
#ifndef PSX
BOOL startGraphicsOptionsMenu(VOID)
{
	addBackdrop();
	addTopForm();
	addBottomForm();

//	addSideText	 (FRONTEND_SIDETEXT ,	FRONTEND_SIDEX,FRONTEND_SIDEY,strresGetString(psStringRes, STR_FE_SIDEMULTI));


	addTextButton(FRONTEND_TEXTURES,FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_TEXTURE),TRUE,TRUE);
	addTextButton(FRONTEND_EFFECTS,	FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_EFFECTS),TRUE,TRUE);

	if (war_GetFog())
	{
		strcpy(strFog, "on");
	}
	else
	{
		strcpy(strFog, "off");
	}
	addTextButton(FRONTEND_FOG,		FRONTEND_POS5X,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_FOG),TRUE,FALSE);
	addTextButton(FRONTEND_FOG_R,	FRONTEND_POS5M,FRONTEND_POS5Y, strFog,TRUE,FALSE);

	//translucency
	if (!war_GetTranslucent())
	{
		strcpy(strTrans, "off  compatible");
	}
	else if (!war_GetAdditive())
	{
		strcpy(strTrans, "on  compatible");
	}
	else
	{
		strcpy(strTrans, "on  default");
	}
	addTextButton(FRONTEND_TRANSPARENCY,	FRONTEND_POS6X,FRONTEND_POS6Y, strresGetString(psStringRes, STR_FE_TRANSPARENCY),TRUE,FALSE);
	addTextButton(FRONTEND_TRANSPARENCY_R,	FRONTEND_POS6M,FRONTEND_POS6Y, strTrans,TRUE,FALSE);

	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);

	SetMousePos(0,320,FRONTEND_BOTFORMY+FRONTEND_POS3Y);
	SnapToID(&InterfaceSnap,3);
	return TRUE;
}

BOOL runGraphicsOptionsMenu(VOID)
{
	UDWORD id;

	processFrontendSnap(TRUE);

	id = widgRunScreen(psWScreen);						// Run the current set of widgets 
	switch(id)
	{
	case FRONTEND_TEXTURES:
		break;
	case FRONTEND_EFFECTS:
		break;
	case FRONTEND_FOG:
	case FRONTEND_FOG_R:
		if (war_GetFog())
		{
			war_SetFog(FALSE);
			widgSetString(psWScreen,FRONTEND_FOG_R,"off");
		}
		else
		{
			war_SetFog(TRUE);
			widgSetString(psWScreen,FRONTEND_FOG_R,"on");
		}
//changeTitleMode(GRAPHICS);

		break;
	case FRONTEND_TRANSPARENCY:
	case FRONTEND_TRANSPARENCY_R:
	//translucency
		if (!war_GetTranslucent())
		{
			war_SetTranslucent(TRUE);
			war_SetAdditive(FALSE);
			widgSetString(psWScreen,FRONTEND_TRANSPARENCY_R,"on compatible");
		}
		else if (!war_GetAdditive())
		{
			war_SetTranslucent(TRUE);
			war_SetAdditive(TRUE);
			widgSetString(psWScreen,FRONTEND_TRANSPARENCY_R,"on default");

		}
		else
		{
			war_SetTranslucent(FALSE);
			war_SetAdditive(FALSE);
			widgSetString(psWScreen,FRONTEND_TRANSPARENCY_R,"off compatible");
		}
//		changeTitleMode(GRAPHICS);
		break;

	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
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
#endif

// ////////////////////////////////////////////////////////////////////////////
// Video Options Menu

#ifndef PSX
BOOL startVideoOptionsMenu(VOID)
{
	addBackdrop();
	addTopForm();
	addBottomForm();


	addTextButton(FRONTEND_SOFTWARE,FRONTEND_POS2X,FRONTEND_POS2Y, strresGetString(psStringRes, STR_FE_SOFTWARE),FALSE,FALSE);
	addTextButton(FRONTEND_DIRECTX,	FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes, STR_FE_DIRECTX),FALSE,FALSE);
	addTextButton(FRONTEND_OPENGL,	FRONTEND_POS4X,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_OPENGL),FALSE,TRUE);
	addTextButton(FRONTEND_GLIDE,	FRONTEND_POS5X,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_GLIDE),FALSE,FALSE);

	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);

	SetMousePos(0,320,FRONTEND_BOTFORMY+FRONTEND_POS3Y);
	SnapToID(&InterfaceSnap,3);
	return TRUE;
}

BOOL runVideoOptionsMenu(VOID)
{
	UDWORD id;

	processFrontendSnap(TRUE);

	id = widgRunScreen(psWScreen);						// Run the current set of widgets 
	switch(id)
	{
	case FRONTEND_SOFTWARE:
		if (war_GetRendMode() == REND_MODE_SOFTWARE)
		{
			changeTitleMode(OPTIONS);
		}
		else
		{
			war_SetRendMode(REND_MODE_SOFTWARE);
			reInit = TRUE;//restart
			changeTitleMode(QUIT);
		}
		break;
	case FRONTEND_GLIDE:
		if (war_GetRendMode() == REND_MODE_GLIDE)
		{
			changeTitleMode(OPTIONS);
		}
		else
		{
			war_SetRendMode(REND_MODE_GLIDE);
			reInit = TRUE;//restart
			changeTitleMode(QUIT);
		}
		break;
	case FRONTEND_DIRECTX:
		if (war_GetRendMode() == REND_MODE_HAL)
		{
			changeTitleMode(OPTIONS);
		}
		else
		{
			war_SetRendMode(REND_MODE_HAL);
			reInit = TRUE;//restart
			changeTitleMode(QUIT);
		}
		break;
	case FRONTEND_OPENGL:
		if (war_GetRendMode() == REND_MODE_HAL2)
		{
			changeTitleMode(OPTIONS);
		}
		else
		{
			war_SetRendMode(REND_MODE_HAL2);
			reInit = TRUE;//restart
			changeTitleMode(QUIT);
		}
		break;
	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
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
#endif
*/

#ifndef PSX
// ////////////////////////////////////////////////////////////////////////////
// Game Options Menu 2!
BOOL startGameOptions2Menu(VOID)
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
	if(bAllowSubtitles)
	{
		addTextButton(FRONTEND_SUBTITLES,	FRONTEND_POS5X-35,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_SUBTITLES),TRUE,FALSE);
	}
	else
	{
		addTextButton(FRONTEND_SUBTITLES,	FRONTEND_POS5X-35,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_SUBTITLES),TRUE,TRUE);
	}

	if(bAllowSubtitles)
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


BOOL runGameOptions2Menu(VOID)
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
#endif

// ////////////////////////////////////////////////////////////////////////////
// Game Options Menu
BOOL startGameOptionsMenu(VOID)
{

	UDWORD	w,h;



	addBackdrop();
	addTopForm();
	addBottomForm();

#ifndef PSX
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

	//volume
	addTextButton(FRONTEND_FX, FRONTEND_POS4X-25,FRONTEND_POS4Y, strresGetString(psStringRes, STR_FE_FX),TRUE,FALSE);
	addFESlider(FRONTEND_FX_SL,FRONTEND_BOTFORM, FRONTEND_POS4M, FRONTEND_POS4Y+5, AUDIO_VOL_MAX,sound_GetGlobalVolume(),FRONTEND_FX );
	
	// cd audio
	addTextButton(FRONTEND_MUSIC, FRONTEND_POS5X-25,FRONTEND_POS5Y, strresGetString(psStringRes, STR_FE_MUSIC),TRUE,FALSE);
	addFESlider(FRONTEND_MUSIC_SL,FRONTEND_BOTFORM, FRONTEND_POS5M, FRONTEND_POS5Y+5,AUDIO_VOL_MAX,mixer_GetCDVolume(),FRONTEND_MUSIC );

/*	if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		//gamma
		if(gammaValue>3)	   gammaValue = (float)2.9;
		if(gammaValue<0.5)  gammaValue = (float).5;
	
		addTextButton(FRONTEND_GAMMA, FRONTEND_POS6X-25,FRONTEND_POS6Y, strresGetString(psStringRes, STR_FE_GAMMA),TRUE,FALSE);
		addFESlider(FRONTEND_GAMMA_SL,FRONTEND_BOTFORM, FRONTEND_POS6M, FRONTEND_POS6Y+5, 60, (UDWORD)(gammaValue*25),FRONTEND_GAMMA );
	}
*/

	// colour stuff	
	w = 	iV_GetImageWidth(FrontImages,IMAGE_PLAYER0);
	h = 	iV_GetImageHeight(FrontImages,IMAGE_PLAYER0);
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P0, FRONTEND_POS7M+(0*(w+6)),FRONTEND_POS7Y,w,h,0,IMAGE_PLAYER0	,IMAGE_PLAYERX,TRUE);
//	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P1, FRONTEND_POS6M-(3*(w+4)),FRONTEND_POS6Y,w,h,0,IMAGE_PLAYER1	,IMAGE_HI34,TRUE);
//	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P2, FRONTEND_POS6M-(2*(w+4)),FRONTEND_POS6Y,w,h,0,IMAGE_PLAYER2	,IMAGE_HI34,TRUE);
//	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P3, FRONTEND_POS6M-(1*(w+4)),FRONTEND_POS6Y,w,h,0,IMAGE_PLAYER3	,IMAGE_HI34,TRUE);
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P4, FRONTEND_POS7M+(1*(w+6)),FRONTEND_POS7Y,w,h,0,IMAGE_PLAYER4	,IMAGE_PLAYERX,TRUE);
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P5, FRONTEND_POS7M+(2*(w+6)),FRONTEND_POS7Y,w,h,0,IMAGE_PLAYER5	,IMAGE_PLAYERX,TRUE);
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P6, FRONTEND_POS7M+(3*(w+6)),FRONTEND_POS7Y,w,h,0,IMAGE_PLAYER6	,IMAGE_PLAYERX,TRUE);
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FE_P7, FRONTEND_POS7M+(4*(w+6)),FRONTEND_POS7Y,w,h,0,IMAGE_PLAYER7,IMAGE_PLAYERX,TRUE);

	widgSetButtonState(psWScreen, FE_P0+getPlayerColour(0), WBUT_LOCK);		
	addTextButton(FRONTEND_COLOUR,		FRONTEND_POS7X-25,FRONTEND_POS7Y, strresGetString(psStringRes, STR_FE_CLAN),TRUE,FALSE);


	// quit.
	addMultiBut(psWScreen,FRONTEND_BOTFORM,FRONTEND_QUIT,10,10,30,29, STR_FE_RETURN,IMAGE_RETURN,IMAGE_RETURN_HI,TRUE);

#else	// PSX version.
//22	addTextButton(FRONTEND_FX, FRONTEND_POS1X,FRONTEND_POS1Y, strresGetString(psStringRes, STR_FE_FX),TRUE,FALSE);
//22	addFESlider(FRONTEND_FX_SL,FRONTEND_BOTFORM, FRONTEND_POS1M, FRONTEND_POS1Y+5,
//22				AUDIO_VOL_MAX/2	,sound_GetGlobalVolume()/2,FRONTEND_FX);
//22//	addFESlider(FRONTEND_FX_SL,FRONTEND_BOTFORM, FRONTEND_POS4M, FRONTEND_POS4Y+5, AUDIO_VOL_MAX,sound_GetGlobalVolume(),FRONTEND_FX );
//22//	// cd audio
//22//	addFESlider(FRONTEND_MUSIC_SL,FRONTEND_BOTFORM, FRONTEND_POS5M, FRONTEND_POS5Y+5,AUDIO_VOL_MAX,mixer_GetVolume(),FRONTEND_MUSIC );
//22
//22	addTextButton(FRONTEND_MUSIC, FRONTEND_POS2X,FRONTEND_POS2Y, strresGetString(psStringRes, STR_FE_MUSIC),TRUE,FALSE);
//22	addFESlider(FRONTEND_MUSIC_SL,FRONTEND_BOTFORM, FRONTEND_POS2M, FRONTEND_POS2Y+5,
//22				AUDIO_VOL_MAX/2	,cdAudio_GetVolume()/2,FRONTEND_MUSIC);
//22
//22	addTextButton(FRONTEND_CURSOR, FRONTEND_POS3X,FRONTEND_POS3Y, strresGetString(psStringRes,STR_FE_CURSORSPEED),TRUE,FALSE);
//22	addFESlider(FRONTEND_CURSOR_SL,FRONTEND_BOTFORM, FRONTEND_POS3M, FRONTEND_POS3Y+5,
//22				getCursorSpeedRange(),getCursorSpeedModifier(),FRONTEND_CURSOR);
//22
//22//#ifndef MOUSE_EMULATION_ALLOWED
//22//	addTextButton(FRONTEND_CONTROL, FRONTEND_POS3X,FRONTEND_POS3Y, "Direct Control",TRUE,FALSE);
//22//	if(DirectControl) {
//22//		StateString = OnString;
//22//	} else {
//22//		StateString = OffString;
//22//	}
//22//	addText(FEFont,FRONTEND_BOTFORM,FRONTEND_CONTROL_BT, FRONTEND_POS3M,FRONTEND_POS3Y, StateString,FRONTEND_CONTROL,&DirectControl);
//22//#endif
//22
//22	addTextButton(FRONTEND_SCREENSHAKE, FRONTEND_POS4X,FRONTEND_POS4Y,
//22					strresGetString(psStringRes, STR_FE_SCREENSHAKE),TRUE,FALSE);
//22	if(bShakingPermitted) {
//22		StateString = OnString;
//22	} else {
//22		StateString = OffString;
//22	}
//22	addText(FEFont,FRONTEND_BOTFORM,FRONTEND_SCREENSHAKE_BT, FRONTEND_POS4M,FRONTEND_POS4Y,
//22			StateString,FRONTEND_SCREENSHAKE,&bShakingPermitted);
//22
//22  #ifdef LIBPAD
//22	if( (GetProtocolType(0) == PADPROT_EXPANDED) ) {
//22		addTextButton(FRONTEND_VIBRO, FRONTEND_POS5X,FRONTEND_POS5Y,
//22						strresGetString(psStringRes,STR_FE_VIBRATION),TRUE,FALSE);
//22		if(EnableVibration) {
//22			StateString = OnString;
//22		} else {
//22			StateString = OffString;
//22		}
//22		addText(FEFont,FRONTEND_BOTFORM,FRONTEND_VIBRO_BT, FRONTEND_POS5M,FRONTEND_POS5Y,
//22				StateString,FRONTEND_VIBRO,&EnableVibration);
//22		intSetVibroOnID(FRONTEND_VIBRO);
//22	}
//22  #endif
//22
//22	addTextButton(FRONTEND_CENTRESCREEN,FRONTEND_POS6X,FRONTEND_POS6Y, strresGetString(psStringRes, STR_FE_CENTRESCREEN),TRUE,FALSE);
//22	addTextButton(FRONTEND_QUIT,        FRONTEND_POS7X,FRONTEND_POS7Y, strresGetString(psStringRes, STR_FE_RETURN),TRUE,FALSE);
//22	addSideText	 (FRONTEND_SIDETEXT ,	FRONTEND_SIDEX,FRONTEND_SIDEY, strresGetString(psStringRes, STR_FE_SIDEOPTIONS));
//22	addCentreScreen(FRONTEND_BOTFORM,
//22					FRONTEND_POS6X+iV_GetTextWidth(strresGetString(psStringRes, STR_FE_CENTRESCREEN)) + 16,
//22					FRONTEND_POS6Y);
//22	addSelectHelp();
//22	addCancelHelp();
//22	SetCurrentSnapID(&InterfaceSnap,FRONTEND_QUIT);
#endif

#ifndef PSX
	//add some text down the side of the form
	addSideText	 (FRONTEND_SIDETEXT ,	FRONTEND_SIDEX,FRONTEND_SIDEY, strresGetString(psStringRes, STR_FE_SIDEOPTIONS));
#endif

	return TRUE;
}

BOOL runGameOptionsMenu(VOID)
{
	UDWORD id;

	processFrontendSnap(FALSE);

	id = widgRunScreen(psWScreen);						// Run the current set of widgets 
	switch(id)
	{
#ifndef PSX
//	case FRONTEND_GAMMA:
	case FRONTEND_SCROLLSPEED:
	case FRONTEND_FX:
	case FRONTEND_MUSIC:
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

/*	case FRONTEND_GAMMA_SL:
		// gamma range = 0.4 - 3 
		gammaValue = (float)( widgGetSliderPos(psWScreen,FRONTEND_GAMMA_SL) )/25  ;
		if(gammaValue<0.5)  gammaValue = (float).5;
		pie_SetGammaValue(gammaValue);
		break;
*/
	case FRONTEND_SCROLLSPEED_SL:
		scroll_speed_accel = widgGetSliderPos(psWScreen,FRONTEND_SCROLLSPEED_SL) * 100; //0-1600
		if(scroll_speed_accel ==0)		// make sure you CAN scroll.
		{
			scroll_speed_accel = 100;
		}
		break;
	case FRONTEND_FX_SL:
		mixer_SetWavVolume(widgGetSliderPos(psWScreen,FRONTEND_FX_SL));
		break;

	case FRONTEND_MUSIC_SL:
		mixer_SetCDVolume(widgGetSliderPos(psWScreen,FRONTEND_MUSIC_SL));
		break;
#endif

	case FRONTEND_QUIT:
//#ifdef PSX
//		cdAudio_StopTrack();
//#endif

		changeTitleMode(OPTIONS);
		break;

//	case FRONTEND_VIDEO:
//		changeTitleMode(VIDEO);
//		break;

#ifndef PSX
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
#endif
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

VOID addBackdrop(VOID)
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

VOID removeBackdrop(VOID)
{
	widgDelete( psWScreen, FRONTEND_BACKDROP );
}

// ////////////////////////////////////////////////////////////////////////////

VOID addBottomForm(VOID)
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
#ifndef PSX
	sFormInit.pDisplay = intOpenPlainForm;
	sFormInit.disableChildren = TRUE;
#else
	sFormInit.pDisplay = intDisplayPlainForm;
#endif
	widgAddForm(psWScreen, &sFormInit);	
}

// ////////////////////////////////////////////////////////////////////////////

VOID removeBottomForm( VOID )
{
	widgDelete( psWScreen, FRONTEND_BOTFORM );
}

// ////////////////////////////////////////////////////////////////////////////

VOID addTopForm(VOID)
{
	W_FORMINIT		sFormInit;			

	memset(&sFormInit, 0, sizeof(W_FORMINIT));

	sFormInit.formID = FRONTEND_BACKDROP;
	sFormInit.id = FRONTEND_TOPFORM;
	sFormInit.style = WFORM_PLAIN;
#ifndef PSX
	if(titleMode == MULTIOPTION)
	{
		sFormInit.x		= FRONTEND_TOPFORM_WIDEX;
		sFormInit.y		= FRONTEND_TOPFORM_WIDEY;
		sFormInit.width = FRONTEND_TOPFORM_WIDEW;
		sFormInit.height= FRONTEND_TOPFORM_WIDEH;
	}
	else
#endif
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

VOID removeTopForm( VOID )
{
	widgDelete( psWScreen, FRONTEND_TOPFORM );
}

// ////////////////////////////////////////////////////////////////////////////
VOID addTextButton(UDWORD id,  UDWORD PosX, UDWORD PosY, STRING *txt,BOOL bAlign,BOOL bGrey)
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
#ifndef PSX
		sButInit.x+=35;
#endif
	}
	else
	{
		sButInit.style = WBUT_PLAIN | WBUT_TXTCENTRE;
		sButInit.width = FRONTEND_BUTWIDTH;
	}


	sButInit.pUserData = (VOID*)bGrey;				// store disable state

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
VOID addFESlider(UDWORD id, UDWORD parent, UDWORD x,UDWORD y,UDWORD stops,UDWORD pos,UDWORD attachID )
{
	W_SLDINIT		sSldInit;
#ifndef PSX

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
#else
	memset(&sSldInit, 0, sizeof(W_SLDINIT));
	sSldInit.formID		= parent;
	sSldInit.id			= id;
	sSldInit.style		= WSLD_PLAIN;
	sSldInit.x			= (short)x;
	sSldInit.y			= (short)y;
	sSldInit.width		= iV_GetImageWidth(IntImages,IMAGE_SLIDER_BIG);	//*2;
	sSldInit.height		= iV_GetImageHeight(IntImages,IMAGE_SLIDER_BIG);	//*2;
	sSldInit.orientation= WSLD_LEFT;
	sSldInit.numStops	= (UBYTE) stops;
	sSldInit.barSize	= iV_GetImageHeight(IntImages,IMAGE_SLIDER_BIG);	//*2;
	sSldInit.pos		= (UBYTE) pos;
	sSldInit.pDisplay	= displayBigSlider;
	sSldInit.pCallback  = intUpdateOptionSlider;
// On the PSX we need to tell the slider which text item it's attached to so it can decide
// if it's selected.
	sSldInit.UserData	= attachID;
	if(!widgAddSlider(psWScreen, &sSldInit)) {
		DBPRINTF(("%d %d\n", sSldInit.width,stops));
		DBPRINTF(("Error adding slider\n"));
	}
#endif
}

// ////////////////////////////////////////////////////////////////////////////
VOID addSideText(UDWORD id,  UDWORD PosX, UDWORD PosY, STRING *txt)
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
#ifndef PSX
	sLabInit.FontID = FEFont;
#else
	sLabInit.FontID = FEFont;
//	sLabInit.FontID = FEBigFont;
#endif
	sLabInit.pDisplay = displayTextAt270;
	sLabInit.pText = txt;
	widgAddLabel(psWScreen, &sLabInit);
#endif
}


VOID addText(int FontID,UDWORD FormID,UDWORD id,  UDWORD PosX, UDWORD PosY, STRING *txt, UDWORD attachID,BOOL *State)
{
	W_LABINIT	sLabInit;

DBPRINTF(("addText : %s\n",txt));
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



// ////////////////////////////////////////////////////////////////////////////
// drawing functions

// show a background piccy
VOID displayTitleBitmap(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	STRING	sTmp[128];

	iV_SetFont(WFont);
	iV_SetTextColour(-1);

#ifndef PSX

	switch(war_GetRendMode())
	{
	case REND_MODE_SOFTWARE:
		if(weHave3DNow())
		{
			sprintf(sTmp, VERSION_STRING " - Build: %s SW (With AMD 3DNow!)",__DATE__);
		}
		else
		{
			sprintf(sTmp, VERSION_STRING " - Build: %s SW",__DATE__);
		}
		break;
		
	case REND_MODE_GLIDE:
		if(weHave3DNow())
		{
			sprintf(sTmp, VERSION_STRING " - Build: %s 3DFX (With AMD 3DNow!)",__DATE__);
		}
		else
		{
			sprintf(sTmp, VERSION_STRING " - Build: %s 3DFX",__DATE__);
		}
		break;

	case REND_MODE_HAL:
		if(weHave3DNow())
		{
			sprintf(sTmp, VERSION_STRING " - Build: %s D3D (With AMD 3DNow!)",__DATE__);
		}
		else
		{
			sprintf(sTmp, VERSION_STRING " - Build: %s D3D",__DATE__);
		}
		break;
	default:
		sprintf(sTmp, VERSION_STRING " - Build: %s ???",__DATE__);
		break;
	}
	pie_DrawText270(sTmp,DISP_WIDTH-10,DISP_HEIGHT-15);

#else
#ifdef COVERMOUNT
	sprintf(sTmp,"Pumpkin Studios,Eidos Interactive");
	iV_DrawText(sTmp,(640-iV_GetTextWidth(sTmp))/2,160);
#else
//	sprintf(sTmp,"BETA %s : %s",__DATE__,__TIME__);
//	iV_DrawText(sTmp,(640-iV_GetTextWidth(sTmp))/2,160);
#endif
#endif
}

// ////////////////////////////////////////////////////////////////////////////
// show warzone logo
VOID displayLogo(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{   
#ifndef PSX
	iV_DrawTransImage(FrontImages,IMAGE_FE_LOGO,xOffset+psWidget->x,yOffset+psWidget->y);
#else
	iV_SetOTIndex_PSX(OT2D_FARFORE);
//	iV_DrawImage(FrontImages,IMAGE_FE_LEGAL1,xOffset+psWidget->x-68-32,yOffset+psWidget->y+64+12-6);
//	iV_DrawImage(FrontImages,IMAGE_FE_LEGAL2,xOffset+psWidget->x+208+32,yOffset+psWidget->y+64+12-6);
	iV_DrawImage(FrontImages,IMAGE_FE_LOGO,xOffset+psWidget->x,yOffset+psWidget->y);
#endif
}




// ////////////////////////////////////////////////////////////////////////////
// show a text option.
VOID displayTextOption(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
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
VOID displayTextAt270(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	SDWORD		fx,fy;
	W_LABEL		*psLab;

	psLab = (W_LABEL *)psWidget;
#ifndef PSX
	iV_SetFont(FEFont);
#else
	iV_SetFont(FEFont);
//	iV_SetFont(FEBigFont);
#endif

	iV_SetTextColour(PIE_TEXT_WHITE);

	fx = xOffset + psWidget->x;
#ifndef PSX
	fy = yOffset + psWidget->y + iV_GetTextWidth(psLab->aText) ;		
#else
	fy = psWidget->y + PSXToHeight(WidthToPSX(iV_GetTextWidth(psLab->aText)));		
#endif

	iV_DrawText270( psLab->aText, fx, fy);
}
#endif


// ////////////////////////////////////////////////////////////////////////////
// show, well have a guess..
static VOID displayBigSlider(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	W_SLIDER *Slider = (W_SLIDER*)psWidget;
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y;
	SWORD sx;

#ifndef PSX
	iV_DrawTransImage(IntImages,IMAGE_SLIDER_BIG,x+STAT_SLD_OX,y+STAT_SLD_OY);			// draw bdrop

	sx = (SWORD)((Slider->width-3 - Slider->barSize) * Slider->pos / Slider->numStops);	// determine pos.
	iV_DrawTransImage(IntImages,IMAGE_SLIDER_BIGBUT,x+3+sx,y+3);								//draw amount

#else
	sx = (SWORD)((Slider->width-3 - Slider->barSize) * Slider->pos / Slider->numStops);	// determine pos.
 #ifdef DISPLAYMODE_PAL
	iV_DrawTransImage(IntImages,IMAGE_SLIDER_BIGBUT,x+3+sx,y+4);								//draw amount
 #else
	iV_DrawTransImage(IntImages,IMAGE_SLIDER_BIGBUT,x+3+sx,y+3);								//draw amount
 #endif
	iV_DrawTransImage(IntImages,IMAGE_SLIDER_BIG,x+STAT_SLD_OX,y+STAT_SLD_OY);			// draw bdrop
#endif
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



