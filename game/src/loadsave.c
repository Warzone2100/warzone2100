#ifdef WIN32
/*
 * loadsave.c
 * load and save Popup screens.
 *
 * these don't actually do any loading or saving, but just
 * return a filename to use for the ops.
 */
#include "frame.h"
#include "widget.h"
#include "piePalette.h"		// for predefined colours.
#include "rendMode.h"		// for boxfill
#include "hci.h"
#include "loadsave.h"
#ifdef WIN32
#include "multiplay.h"
#include "game.h"
#include "audio_id.h"
#include "frontend.h"
#include "winmain.h"
#include "display3d.h"
#include "display.h"
#endif
#include "netplay.h"
#include "loop.h"
#include "intdisplay.h"
#include "text.h"
#include "mission.h"
#include "gTime.h"
#ifdef PSX
#include "primatives.h"
#include "csnap.h"
#include "dcache.h"
extern CURSORSNAP InterfaceSnap;
#endif

// ////////////////////////////////////////////////////////////////////////////
#define LOADSAVE_X				130	+ D_W
#define LOADSAVE_Y				170	+ D_H
#define LOADSAVE_W				380
#define LOADSAVE_H				200

#define MAX_SAVE_NAME			60

#ifdef WIN32
#define LOADSAVE_HGAP			5
#define LOADSAVE_VGAP			5
#define LOADSAVE_BANNER_DEPTH	25
#else
#define LOADSAVE_HGAP			(4+22)
#define LOADSAVE_VGAP			4
#define LOADSAVE_BANNER_DEPTH	24
#endif

#define LOADENTRY_W				(LOADSAVE_W -(3 * LOADSAVE_HGAP)) /2
#define LOADENTRY_H				(LOADSAVE_H -(6 * LOADSAVE_VGAP )- (LOADSAVE_BANNER_DEPTH+LOADSAVE_VGAP) ) /5

#define ID_LOADSAVE				21000
#define LOADSAVE_FORM			ID_LOADSAVE+1		// back form.
#define LOADSAVE_CANCEL			ID_LOADSAVE+2		// cancel but.
#define LOADSAVE_LABEL			ID_LOADSAVE+3		// load/save
#define LOADSAVE_BANNER			ID_LOADSAVE+4		// banner.

#define LOADENTRY_START			ID_LOADSAVE+5		// each of the buttons.	
#define LOADENTRY_END			ID_LOADSAVE+15	

#define SAVEENTRY_EDIT			ID_LOADSAVE+16		// save edit box.

// ////////////////////////////////////////////////////////////////////////////
void drawBlueBox				(UDWORD x,UDWORD y, UDWORD w, UDWORD h);
BOOL closeLoadSave				();
BOOL runLoadSave				(BOOL bResetMissionWidgets);
BOOL displayLoadSave			();
static BOOL _addLoadSave		(BOOL bLoad,CHAR *sSearchPath,CHAR *sExtension, CHAR *title);
static BOOL _runLoadSave		(BOOL bResetMissionWidgets);
static void displayLoadBanner	(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
static void displayLoadSlot		(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
static void displayLoadSaveEdit	(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);
void		removeWildcards		(char *pStr);

static	W_SCREEN	*psRequestScreen;					// Widget screen for requester
static	BOOL		mode;
static	UDWORD		chosenSlotId;

BOOL				bLoadSaveUp = FALSE;						// true when interface is up and should be run.
STRING				saveGameName[256];			//the name of the save game to load from the front end
STRING				sRequestResult[255];						// filename returned;
STRING				sDelete[MAX_STR_LENGTH];
BOOL				bRequestLoad = FALSE;
LOADSAVE_MODE		bLoadSaveMode;

static CHAR			sPath[255];
static CHAR			sExt[4];

#ifdef PSX
void displayHilightPulseBox(SWORD x0,SWORD y0,SWORD x1,SWORD y1);	// defined in frontend.c
#endif

// ////////////////////////////////////////////////////////////////////////////
// return whether the save screen was displayed in the mission results screen
BOOL saveInMissionRes(void)
{
	return bLoadSaveMode == SAVE_MISSIONEND;
}

// ////////////////////////////////////////////////////////////////////////////
// return whether the save screen was displayed in the middle of a mission
BOOL saveMidMission(void)
{
	return bLoadSaveMode == SAVE_INGAME;
}

// ////////////////////////////////////////////////////////////////////////////
BOOL addLoadSave(LOADSAVE_MODE mode,CHAR *sSearchPath,CHAR *sExtension, CHAR *title)
{
BOOL bLoad;

	bLoadSaveMode = mode;

	switch(mode)
	{
	case LOAD_FRONTEND:
	case LOAD_MISSIONEND:
	case LOAD_INGAME:
	case LOAD_FORCE:
		bLoad = TRUE;
		break;
	case SAVE_MISSIONEND:
	case SAVE_INGAME:
	case SAVE_FORCE:
	default:
		bLoad = FALSE;
		break;
	}

#ifdef PSX
	// If the stacks in the dcache then..
	if(SpInDCache()) {
		static BOOL _bLoad;
		static CHAR *_sSearchPath;
		static CHAR *_sExtension;
		static CHAR *_title;
		static BOOL ret;

		_bLoad = bLoad;
		_sSearchPath = sSearchPath;
		_sExtension = sExtension;
		_title = title;

		// Set the stack pointer to point to the alternative stack which is'nt limited to 1k.
		SetSpAlt();
		ret = _addLoadSave(bLoad,sSearchPath,sExtension,title);
		SetSpAltNormal();

		return ret;
	}
#endif

	return _addLoadSave(bLoad,sSearchPath,sExtension,title);
}


// ////////////////////////////////////////////////////
static BOOL _addLoadSave(BOOL bLoad,CHAR *sSearchPath,CHAR *sExtension, CHAR *title)
{
	W_FORMINIT		sFormInit;
	W_BUTINIT		sButInit;
	W_LABINIT		sLabInit;
	UDWORD			slotCount;
	static STRING	sSlots[10][64];
	STRING			sTemp[255];

#ifdef WIN32
	WIN32_FIND_DATA	found;	
	HANDLE			dir;
#endif
	
	mode = bLoad;

#ifdef WIN32
	if(GetCurrentDirectory(255,(char*)&sTemp) == 0)
	{
		return FALSE;										// failed, directory probably didn't exist.
	}
	
	if ((bLoadSaveMode == LOAD_INGAME) || (bLoadSaveMode == SAVE_INGAME))
	{
		if (!bMultiPlayer || (NetPlay.bComms ==0))
		{
			gameTimeStop();
			if(GetGameMode() == GS_NORMAL)
			{	
				BOOL radOnScreen = radarOnScreen;				// Only do this in main game.
					
				bRender3DOnly = TRUE;
				radarOnScreen = FALSE;
		
				displayWorld();									// Just display the 3d, no interface
		
				pie_UploadDisplayBuffer(DisplayBuffer);			// Upload the current display back buffer into system memory.
			
				iV_ScaleBitmapRGB(DisplayBuffer,iV_GetDisplayWidth(),
								 iV_GetDisplayHeight(),2,2,2);	// Make it darker.
			
				radarOnScreen = radOnScreen;
				bRender3DOnly = FALSE;
			}

			setGamePauseStatus( TRUE );
			setGameUpdatePause(TRUE);
			setScriptPause(TRUE);
			setScrollPause(TRUE);
			setConsolePause(TRUE);

		}

		forceHidePowerBar();
		intRemoveReticule();
	}

	CreateDirectory(sSearchPath,NULL);			// create the directory required... fails if already there, so no problem.
#endif
	widgCreateScreen(&psRequestScreen);			// init the screen.
	widgSetTipFont(psRequestScreen,WFont);
#ifdef PSX
	DisableCursorSnapsExcept(LOADSAVE_FORM);
	WidgSetOTIndex(OT2D_FORE);
#endif

	/* add a form to place the tabbed form on */
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = 0;
	sFormInit.id = LOADSAVE_FORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD)(LOADSAVE_X);
	sFormInit.y = (SWORD)(LOADSAVE_Y);
	sFormInit.width = LOADSAVE_W;
	sFormInit.height = LOADSAVE_H;
	sFormInit.disableChildren = TRUE;
	sFormInit.pDisplay = intOpenPlainForm;
	widgAddForm(psRequestScreen, &sFormInit);

	// Add Banner
	sFormInit.formID = LOADSAVE_FORM;
	sFormInit.id = LOADSAVE_BANNER;
	sFormInit.x = LOADSAVE_HGAP;
	sFormInit.y = LOADSAVE_VGAP;
	sFormInit.width = LOADSAVE_W-(2*LOADSAVE_HGAP);
	sFormInit.height = LOADSAVE_BANNER_DEPTH;
	sFormInit.disableChildren = FALSE;
	sFormInit.pDisplay = displayLoadBanner;
	sFormInit.pUserData = (VOID *)bLoad;
	widgAddForm(psRequestScreen, &sFormInit);

#ifdef PSX
	WidgSetOTIndex(OT2D_FARFORE);
#endif
	// Add Banner Label
	memset(&sLabInit, 0, sizeof(W_LABINIT));
	sLabInit.formID = LOADSAVE_BANNER;
	sLabInit.id		= LOADSAVE_LABEL;
	sLabInit.style	= WLAB_ALIGNCENTRE;
	sLabInit.x		= 0;
	sLabInit.y		= 4;
	sLabInit.width	= LOADSAVE_W-(2*LOADSAVE_HGAP);	//LOADSAVE_W;
	sLabInit.height = 20;
	sLabInit.pText	= title;
	sLabInit.FontID = WFont;
	widgAddLabel(psRequestScreen, &sLabInit);


	// add cancel.
	memset(&sButInit, 0, sizeof(W_BUTINIT));
#ifdef WIN32
	sButInit.formID = LOADSAVE_BANNER;
	sButInit.x = 4;
	sButInit.y = 3;
	sButInit.width		= iV_GetImageWidth(IntImages,IMAGE_NRUTER);
	sButInit.height		= iV_GetImageHeight(IntImages,IMAGE_NRUTER);
	sButInit.pUserData	= (void*)PACKDWORD_TRI(0,IMAGE_NRUTER , IMAGE_NRUTER);
#else
	sButInit.formID = LOADSAVE_FORM;
	sButInit.x = 6;
	sButInit.y = 6;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pUserData = (void*)PACKDWORD_TRI(0,IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
#endif
	sButInit.id = LOADSAVE_CANCEL;
	sButInit.style = WBUT_PLAIN;
	sButInit.pTip = strresGetString(psStringRes, STR_MISC_CLOSE);
	sButInit.FontID = WFont;
	sButInit.pDisplay = intDisplayImageHilight;
	widgAddButton(psRequestScreen, &sButInit);

	// add slots
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID		= LOADSAVE_FORM;
	sButInit.style		= WBUT_PLAIN;
	sButInit.width		= LOADENTRY_W;
	sButInit.height		= LOADENTRY_H;
	sButInit.pDisplay	= displayLoadSlot;
	sButInit.FontID		= WFont;

	for(slotCount = 0; slotCount< 10 ; slotCount++)
	{
		sButInit.id		= slotCount+LOADENTRY_START;
		
		if(slotCount<5)
		{
			sButInit.x	= LOADSAVE_HGAP;
			sButInit.y	= (SWORD)((LOADSAVE_BANNER_DEPTH +(2*LOADSAVE_VGAP)) + (
                slotCount*(LOADSAVE_VGAP+LOADENTRY_H)));
		}
		else
		{
			sButInit.x	= (2*LOADSAVE_HGAP)+LOADENTRY_W;
			sButInit.y	= (SWORD)((LOADSAVE_BANNER_DEPTH +(2* LOADSAVE_VGAP)) + (
                (slotCount-5) *(LOADSAVE_VGAP+LOADENTRY_H)));
		}
		widgAddButton(psRequestScreen, &sButInit);
	}


	// fill slots.
	slotCount = 0;

	sprintf(sTemp,"%s*.%s",sSearchPath,sExtension);		// form search string.
	strcpy(sPath,sSearchPath);							// setup locals.
	strcpy(sExt,sExtension);
	dir =FindFirstFile(sTemp,&found);
	if(dir != INVALID_HANDLE_VALUE)
	{
		while( TRUE ) 
		{
			/* Set the tip and add the button */		
			found.cFileName[strlen(found.cFileName) -4 ] = '\0';			// chop extension

			strcpy(sSlots[slotCount],found.cFileName);		//store it!
			
			((W_BUTTON *)widgGetFromID(psRequestScreen,LOADENTRY_START+slotCount))->pTip		= sSlots[slotCount];
			((W_BUTTON *)widgGetFromID(psRequestScreen,LOADENTRY_START+slotCount))->pText		= sSlots[slotCount];
				
			slotCount++;		// goto next but.
	
			if(!FindNextFile(dir,&found ) || slotCount == 10 )// only show upto 10 entrys.
			{
				break;
			}
		}
	}
	FindClose(dir);
	bLoadSaveUp = TRUE;
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
BOOL closeLoadSave(void)
{
	widgDelete(psRequestScreen,LOADSAVE_FORM);
	bLoadSaveUp = FALSE;

	if ((bLoadSaveMode == LOAD_INGAME) || (bLoadSaveMode == SAVE_INGAME))
	{

		if (!bMultiPlayer || (NetPlay.bComms == 0))
		{
			gameTimeStart();
			setGamePauseStatus( FALSE );
			setGameUpdatePause(FALSE);
			setScriptPause(FALSE);
			setScrollPause(FALSE);
			setConsolePause(FALSE);
		}

		intAddReticule();
		intShowPowerBar();

	}
	widgReleaseScreen(psRequestScreen);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
void loadSaveCDOK( void )
{
	bRequestLoad = TRUE;
	closeLoadSave();
	loadOK();
}

// ////////////////////////////////////////////////////////////////////////////
void loadSaveCDCancel( void )
{
	bRequestLoad = FALSE;
	widgReveal(psRequestScreen,LOADSAVE_FORM);
}

// ////////////////////////////////////////////////////////////////////////////
BOOL runLoadSave(BOOL bResetMissionWidgets)
{
#ifdef PSX
	// If the stacks in the dcache then..
	if(SpInDCache()) {
		static BOOL ret;

		SetSpAlt();
		ret = _runLoadSave(bResetMissionWidgets);
		SetSpAltNormal();

		return ret;
	}
#endif
	return _runLoadSave(bResetMissionWidgets);
}


// ////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
void deleteSaveGame(char* saveGameName)
{
	CHAR			sTemp2[MAX_STR_LENGTH],	sToDel[MAX_STR_LENGTH];
	WIN32_FIND_DATA	found;	
	HANDLE			dir;

	ASSERT((strlen(saveGameName) < MAX_STR_LENGTH,"deleteSaveGame; save game name too long"));

	DeleteFile(saveGameName);					// remove old file.

	saveGameName[strlen(saveGameName)-4] = '\0';// strip extension
	
	strcat(saveGameName,".es");					// remove script data if it exists.
	DeleteFile(saveGameName);
	
	saveGameName[strlen(saveGameName)-3] = '\0';// strip extension

	// if it's a save game, delete the other files.
	// check for a directory and remove that too.
	sprintf(sTemp2,"%s\\*.*",saveGameName);

	dir =FindFirstFile(sTemp2,&found);			// remove other files
	if(dir != INVALID_HANDLE_VALUE)
	{
		sprintf(sToDel,"%s\\%s",saveGameName,found.cFileName);
		DeleteFile(sToDel);
		while( FindNextFile(dir,&found ) ) 
		{
			sprintf(sToDel,"%s\\%s",saveGameName,found.cFileName);
			DeleteFile(sToDel);
		}
	}
	FindClose(dir);	

	RemoveDirectory(saveGameName);
	return;
}
#endif

// ////////////////////////////////////////////////////////////////////////////
// Returns TRUE if cancel pressed or a valid game slot was selected.
// if when returning TRUE strlen(sRequestResult) != 0 then a valid game
// slot was selected otherwise cancel was selected..
static BOOL _runLoadSave(BOOL bResetMissionWidgets)
{
	UDWORD		id=0;
	W_EDBINIT	sEdInit;
	CHAR		sTemp[MAX_STR_LENGTH];
	CD_INDEX	CDrequired;
	UDWORD		iCampaign,i;
	W_CONTEXT		context;
	BOOL		bSkipCD = FALSE;

	id = widgRunScreen(psRequestScreen);

	if ( cdspan_ProcessCDChange(id) )
	{
		return bRequestLoad;
	}

	strcpy(sRequestResult,"");					// set returned filename to null;

	// cancel this operation...
	if(id == LOADSAVE_CANCEL || CancelPressed() )
	{
		goto failure;
	}

	// clicked a load entry
	if( id >= LOADENTRY_START  &&  id <= LOADENTRY_END )
	{

		if(mode)								// Loading, return that entry.
		{
			if( ((W_BUTTON *)widgGetFromID(psRequestScreen,id))->pText )
			{
				sprintf(sRequestResult,"%s%s.%s",sPath,	((W_BUTTON *)widgGetFromID(psRequestScreen,id))->pText ,sExt);
			}
			else
			{
				goto failure;				// clicked on an empty box
			}
		
			if( bLoadSaveMode == LOAD_FORCE || bLoadSaveMode ==SAVE_FORCE )
			{	
				goto successforce;				// it's a force, dont check the cd.
			}

			/* check correct CD in drive */
			iCampaign = getCampaign(sRequestResult,&bSkipCD);
			if ( iCampaign == 0 OR bSkipCD )
			{
				DBPRINTF( ("getCampaign returned 0 or we're loading a skirmish game: assuming correct CD in drive\n") );
			}
			CDrequired = getCDForCampaign( iCampaign );
			if ( (iCampaign == 0) || cdspan_CheckCDPresent( CDrequired ) OR bSkipCD)
			{
				goto success;
			}
			else
			{
				bRequestLoad = FALSE;
				widgHide(psRequestScreen,LOADSAVE_FORM);
				showChangeCDBox( psRequestScreen, CDrequired,
									loadSaveCDOK, loadSaveCDCancel );
				return FALSE;
			}
		}
		else //  SAVING!add edit box at that position.
		{

			if( ! widgGetFromID(psRequestScreen,SAVEENTRY_EDIT))
			{
				// add blank box.
				memset(&sEdInit, 0, sizeof(W_EDBINIT));
				sEdInit.formID= LOADSAVE_FORM;
				sEdInit.id    = SAVEENTRY_EDIT;
				sEdInit.style = WEDB_PLAIN;
				sEdInit.x	  =	widgGetFromID(psRequestScreen,id)->x;
				sEdInit.y     =	widgGetFromID(psRequestScreen,id)->y;
				sEdInit.width = widgGetFromID(psRequestScreen,id)->width;
				sEdInit.height= widgGetFromID(psRequestScreen,id)->height;
				sEdInit.pText = ((W_BUTTON *)widgGetFromID(psRequestScreen,id))->pText; 
				sEdInit.FontID= WFont;
				sEdInit.pBoxDisplay = displayLoadSaveEdit;
				widgAddEditBox(psRequestScreen, &sEdInit);
				
				sprintf(sTemp,"%s%s.%s",
						sPath,
						((W_BUTTON *)widgGetFromID(psRequestScreen,id))->pText ,
						sExt);

				widgHide(psRequestScreen,id);		// hide the old button
				chosenSlotId = id; 

				strcpy(sDelete,sTemp);				// prepare the savegame name.
				sTemp[strlen(sTemp)-4] = '\0';		// strip extension
	
				// auto click in the edit box we just made.
				context.psScreen	= psRequestScreen;
				context.psForm		= (W_FORM *)psRequestScreen->psForm;
				context.xOffset		= 0;
				context.yOffset		= 0;
				context.mx			= mouseX();
				context.my			= mouseY();
				editBoxClicked((W_EDITBOX*)widgGetFromID(psRequestScreen,SAVEENTRY_EDIT), &context);
			}
			else
			{
				// clicked in a different box. shouldnt be possible!(since we autoclicked in editbox)
			}
		}
	}

	// finished entering a name.
	if( id == SAVEENTRY_EDIT) 
	{
		if(!keyPressed(KEY_RETURN))						// enter was not pushed, so not a vaild entry.	
		{
			widgDelete(psRequestScreen,SAVEENTRY_EDIT);	//unselect this box, and go back ..
			widgReveal(psRequestScreen,chosenSlotId);
			return TRUE;
		}


		// scan to see if that game exists in another slot, if
		// so then fail.
		strcpy(sTemp,((W_EDITBOX *)widgGetFromID(psRequestScreen,id))->aText);

		for(i=LOADENTRY_START;i<LOADENTRY_END;i++)
		{
			if( i != chosenSlotId)
			{
				
				if( ((W_BUTTON *)widgGetFromID(psRequestScreen,i))->pText 
					&& strcmp( sTemp,	((W_BUTTON *)widgGetFromID(psRequestScreen,i))->pText ) ==0)
				{	
					widgDelete(psRequestScreen,SAVEENTRY_EDIT);	//unselect this box, and go back ..
					widgReveal(psRequestScreen,chosenSlotId);
				// move mouse to same box..
				//	SetMousePos(widgGetFromID(psRequestScreen,i)->x ,widgGetFromID(psRequestScreen,i)->y);
					audio_PlayTrack(ID_SOUND_BUILD_FAIL);
					return TRUE;
				}
			}
		}


		// return with this name, as we've edited it.
		if (strlen(((W_EDITBOX *)widgGetFromID(psRequestScreen,id))->aText))
		{
			strcpy(sTemp,((W_EDITBOX *)widgGetFromID(psRequestScreen,id))->aText);
			removeWildcards(sTemp);
			sprintf(sRequestResult,"%s%s.%s",
					sPath,
	  				sTemp,
					sExt);
			deleteSaveGame(sDelete);	//only delete game if a new game fills the slot
		}
		else
		{
			goto failure;				// we entered a blank name..
		}
	
		// we're done. saving.
		closeLoadSave();
		bRequestLoad = FALSE;
        if (bResetMissionWidgets AND widgGetFromID(psWScreen,IDMISSIONRES_FORM) == NULL)
        {
            resetMissionWidgets();			//reset the mission widgets here if necessary
        }
		return TRUE;
	}

	return FALSE;

// failed and/or cancelled..
failure:
	closeLoadSave();
	bRequestLoad = FALSE;
    if (bResetMissionWidgets AND widgGetFromID(psWScreen,IDMISSIONRES_FORM) == NULL)
	{
		resetMissionWidgets();
	}
    return TRUE;

// success on load.
success:
	setCampaignNumber( getCampaign(sRequestResult,&bSkipCD) );
successforce:
	closeLoadSave();		
	bRequestLoad = TRUE;
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// should be done when drawing the other widgets.
BOOL displayLoadSave(void)
{
	widgDisplayScreen(psRequestScreen);	// display widgets.
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// STRING HANDLER, replaces dos wildcards in a string with harmless chars.
void removeWildcards(char *pStr)
{
	UDWORD i;

	for(i=0;i<strlen(pStr);i++)
	{
/*	if(   pStr[i] == '?' 
		   || pStr[i] == '*'
		   || pStr[i] == '"'
		   || pStr[i] == '.' 
		   || pStr[i] == '/' 
		   || pStr[i] == '\\'
		   || pStr[i] == '|' )
		{
			pStr[i] = '_';
		}
*/
		if( !isalnum(pStr[i]) 
 		 && pStr[i] != ' '
		 && pStr[i] != '-'
		 && pStr[i] != '+'
  		 && pStr[i] != '!'
		 )
		{
			pStr[i] = '_';
		}
			
	}

	if (strlen(pStr) >= MAX_SAVE_NAME)
	{
		pStr[MAX_SAVE_NAME - 1] = 0;
	}

	return;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// DISPLAY FUNCTIONS

static void displayLoadBanner(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	//UDWORD col;
    UBYTE   col;
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;

	UNUSEDPARAMETER(pColours);

	if(psWidget->pUserData)
	{
		col = COL_GREEN;
	}
	else
	{
		col = COL_RED;
	}

#ifdef WIN32
	iV_BoxFill(x,y,x+psWidget->width,y+psWidget->height,col);
	iV_BoxFill(x+2,y+2,x+psWidget->width-2,y+psWidget->height-2,COL_BLUE);
#else
	iV_BoxFill(x+2,y+2,x+psWidget->width-2,y+psWidget->height-2,COL_BLUE);
	iV_BoxFill(x,y,x+psWidget->width,y+psWidget->height,col);
#endif

}
// ////////////////////////////////////////////////////////////////////////////
static void displayLoadSlot(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	UWORD	im = (UWORD)UNPACKDWORD_TRI_B((UDWORD)psWidget->pUserData);
	UWORD	im2= (UWORD)(UNPACKDWORD_TRI_C((UDWORD)psWidget->pUserData));
	STRING  butString[64];

	UNUSEDPARAMETER(pColours);
#ifdef PSX
	if(((W_BUTTON*)psWidget)->state & WBUTS_HILITE)	{
		iV_SetOTIndex_PSX(iV_GetOTIndex_PSX()-1);
		displayHilightPulseBox(x-4,y-4,x+psWidget->width+4,y+psWidget->height+4);
		iV_SetOTIndex_PSX(iV_GetOTIndex_PSX()+1);
	}
#endif
#ifdef WIN32
	drawBlueBox(x,y,psWidget->width,psWidget->height);	//draw box
#endif
	if(((W_BUTTON *)psWidget)->pTip )
	{
		strcpy(butString,((W_BUTTON *)psWidget)->pTip);
		
		iV_SetFont(WFont);									// font
		iV_SetTextColour(-1);								//colour

		while(iV_GetTextWidth(butString) > psWidget->width)
		{
			butString[strlen(butString)-1]='\0';
		}

		//draw text								
		iV_DrawText( butString, x+4, y+17);

#ifdef PSX
		AddCursorSnap(&InterfaceSnap,
						x+(psWidget->width/2),
						y+(psWidget->height/2),psWidget->formID,psWidget->id,NULL);
#endif
	}
#ifdef PSX
	drawBlueBox(x,y,psWidget->width,psWidget->height);	//draw box
#endif
}
// ////////////////////////////////////////////////////////////////////////////
static void displayLoadSaveEdit(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	UDWORD	w = psWidget->width;
	UDWORD  h = psWidget->height;
	UNUSEDPARAMETER(pColours);

#ifdef WIN32
	iV_BoxFill(x,y,x+w,y+h,COL_RED);
	iV_BoxFill(x+1,y+1,x+w-1,y+h-1,COL_BLUE);
#else
	iV_BoxFill(x+1,y+1,x+w-1,y+h-1,COL_BLUE);
	iV_BoxFill(x,y,x+w,y+h,COL_RED);
#endif

}

// ////////////////////////////////////////////////////////////////////////////
void drawBlueBox(UDWORD x,UDWORD y, UDWORD w, UDWORD h)
{
    UBYTE       dark = COL_BLUE;
    UBYTE       light = COL_LIGHTBLUE;

	// box
	pie_BoxFillIndex(x-1,y-1,x+w+1,y+h+1,light);	
	pie_BoxFillIndex(x,y,x+w,y+h,dark);
}
#endif