/*
 * loadsave.c
 * load and save Popup screens.
 *
 * these don't actually do any loading or saving, but just
 * return a filename to use for the ops.
 */

#include <ctype.h>
#include <string.h>
#include <physfs.h>
#ifndef  _MSC_VER
#include <unistd.h>
#endif		//above line not in .net --Qamly

#include "lib/framework/frame.h"
#include "lib/widget/widget.h"
#include "lib/ivis_common/piepalette.h"		// for predefined colours.
#include "lib/ivis_common/rendmode.h"		// for boxfill
#include "hci.h"
#include "loadsave.h"
#include "multiplay.h"
#include "game.h"
#include "audio_id.h"
#include "frontend.h"
#include "winmain.h"
#include "display3d.h"
#include "display.h"
#ifndef WIN32
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include "lib/netplay/netplay.h"
#include "loop.h"
#include "intdisplay.h"
#include "text.h"
#include "mission.h"
#include "lib/gamelib/gtime.h"
//======================================================================================
//--------------------------------
#define totalslots 20			//saves slots.. was 10 , now 20   *Away with hard coding values!* -Q
#define totalslotspace 64		//guessing 64 max chars for filename.
//--------------------------------
// ////////////////////////////////////////////////////////////////////////////
#define LOADSAVE_X				130	+ D_W
#define LOADSAVE_Y				30	+ D_H		//was 170 -Q
#define LOADSAVE_W				380
#define LOADSAVE_H				240	//was 200 -Q

#define MAX_SAVE_NAME			60


#define LOADSAVE_HGAP			9		//from 5 to 9 -Q
#define LOADSAVE_VGAP			9		//from 5 to 9 -Q
#define LOADSAVE_BANNER_DEPTH	40 		//was 25 top banner which displays either load or save


#define LOADENTRY_W				(LOADSAVE_W -(3 * LOADSAVE_HGAP)) /2
#define LOADENTRY_H				(LOADSAVE_H -(6 * LOADSAVE_VGAP )- (LOADSAVE_BANNER_DEPTH+LOADSAVE_VGAP) ) /5

#define ID_LOADSAVE				21000
#define LOADSAVE_FORM			ID_LOADSAVE+1		// back form.
#define LOADSAVE_CANCEL			ID_LOADSAVE+2		// cancel but.
#define LOADSAVE_LABEL			ID_LOADSAVE+3		// load/save
#define LOADSAVE_BANNER			ID_LOADSAVE+4		// banner.

#define LOADENTRY_START			ID_LOADSAVE+10		// each of the buttons.
#define LOADENTRY_END			ID_LOADSAVE+10 +totalslots  // must have unique ID hmm -Q

#define SAVEENTRY_EDIT			ID_LOADSAVE+50		// save edit box. must be highest value possible I guess. -Q

// ////////////////////////////////////////////////////////////////////////////
void drawBlueBox				(UDWORD x,UDWORD y, UDWORD w, UDWORD h);
BOOL closeLoadSave				(void);
BOOL runLoadSave				(BOOL bResetMissionWidgets);
BOOL displayLoadSave			(void);
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

	return _addLoadSave(bLoad,sSearchPath,sExtension,title);
}
//****************************************************************************************
// Load menu/save menu?
//*****************************************************************************************
static BOOL _addLoadSave(BOOL bLoad,CHAR *sSearchPath,CHAR *sExtension, CHAR *title)
{
	W_FORMINIT		sFormInit;
	W_BUTINIT		sButInit;
	W_LABINIT		sLabInit;
	UDWORD			slotCount;
// removed hardcoded values!  change with the defines above! -Q
	static STRING	sSlots[totalslots][totalslotspace];
	char **i, **files;

	mode = bLoad;
	debug(LOG_WZ, "_addLoadSave(%d, %s, %s, %s)", bLoad, sSearchPath, sExtension, title);

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

	(void) PHYSFS_mkdir(sSearchPath); // just in case

	widgCreateScreen(&psRequestScreen);			// init the screen.
	widgSetTipFont(psRequestScreen,WFont);

	/* add a form to place the tabbed form on */
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = 0;				//this adds the blue background, and the "box" behind the buttons -Q
	sFormInit.id = LOADSAVE_FORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD)(LOADSAVE_X);
	sFormInit.y = (SWORD)(LOADSAVE_Y);
	sFormInit.width = LOADSAVE_W;
	sFormInit.height = (LOADSAVE_H*2)-46;		// hmm..the bottom of the box.... -Q
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
	sFormInit.pUserData = (void *)bLoad;
	widgAddForm(psRequestScreen, &sFormInit);


	// Add Banner Label
	memset(&sLabInit, 0, sizeof(W_LABINIT));
	sLabInit.formID = LOADSAVE_BANNER;
	sLabInit.id		= LOADSAVE_LABEL;
	sLabInit.style	= WLAB_ALIGNCENTRE;
	sLabInit.x		= 0;
	sLabInit.y		= 3;
	sLabInit.width	= LOADSAVE_W-(2*LOADSAVE_HGAP);	//LOADSAVE_W;
	sLabInit.height = LOADSAVE_BANNER_DEPTH;		//This looks right -Q
	sLabInit.pText	= title;
	sLabInit.FontID = WFont;
	widgAddLabel(psRequestScreen, &sLabInit);


	// add cancel.
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID = LOADSAVE_BANNER;
	sButInit.x = 8;
	sButInit.y = 8;
	sButInit.width		= iV_GetImageWidth(IntImages,IMAGE_NRUTER);
	sButInit.height		= iV_GetImageHeight(IntImages,IMAGE_NRUTER);
	sButInit.pUserData	= (void*)PACKDWORD_TRI(0,IMAGE_NRUTER , IMAGE_NRUTER);

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

	for(slotCount = 0; slotCount< totalslots; slotCount++)
	{
		sButInit.id		= slotCount+LOADENTRY_START;

		if(slotCount<(totalslots/2))
		{
			sButInit.x	= LOADSAVE_HGAP;
			sButInit.y	= (SWORD)((LOADSAVE_BANNER_DEPTH +(2*LOADSAVE_VGAP)) + (
                slotCount*(LOADSAVE_VGAP+LOADENTRY_H)));
		}
		else
		{
			sButInit.x	= (2*LOADSAVE_HGAP)+LOADENTRY_W;
			sButInit.y	= (SWORD)((LOADSAVE_BANNER_DEPTH +(2* LOADSAVE_VGAP)) + (
                (slotCount-(totalslots/2)) *(LOADSAVE_VGAP+LOADENTRY_H)));
		}
		widgAddButton(psRequestScreen, &sButInit);
	}

	// fill slots.
	slotCount = 0;

	strcpy(sPath,sSearchPath);							// setup locals.
	strcpy(sExt,sExtension);

	debug(LOG_WZ, "_addLoadSave: Searching \"%s\" for savegames", sSearchPath);

	// add savegame filenames minus extensions to buttons (up to max 10)
	files = PHYSFS_enumerateFiles(sSearchPath);
	for (i = files; *i != NULL; i++) {
		W_BUTTON *button;

		if (!strstr(*i, sExtension)) {
			continue;
		}
		button = (W_BUTTON*)widgGetFromID(psRequestScreen, LOADENTRY_START + slotCount);

		debug(LOG_WZ, "_addLoadSave: We found [%s]", *i);
		/* Set the tip and add the button */
		(*i)[strlen(*i) - 4] = '\0'; // remove .gam extension
		strcpy(sSlots[slotCount], *i);		//store it!
		button->pTip = sSlots[slotCount];
		button->pText = sSlots[slotCount];
		slotCount++;		// goto next but...
		if (slotCount == totalslots) {
			break;	// only show up to 10 entries
		}
	}
	PHYSFS_freeList(files);

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
	return _runLoadSave(bResetMissionWidgets);
}


/***************************************************************************
	Delete a savegame.  saveGameName should be a .gam extension save game
	filename reference.  We delete this file, any .es file with the same
	name, and any files in the directory with the same name.
***************************************************************************/
void deleteSaveGame(char* saveGameName)
{
	char **files, **i;

	ASSERT( strlen(saveGameName) < MAX_STR_LENGTH,"deleteSaveGame; save game name too long" );

	PHYSFS_delete(saveGameName);
	saveGameName[strlen(saveGameName)-4] = '\0';// strip extension

	strcat(saveGameName,".es");					// remove script data if it exists.
	PHYSFS_delete(saveGameName);
	saveGameName[strlen(saveGameName)-3] = '\0';// strip extension

	// check for a directory and remove that too.
	files = PHYSFS_enumerateFiles(saveGameName);
	for (i = files; *i != NULL; i++) {
		debug(LOG_WZ, "Deleting [%s].", *i);
		PHYSFS_delete(*i);
	}
	PHYSFS_freeList(files);
	PHYSFS_delete(saveGameName);	// now empty directory
	return;
}


// ////////////////////////////////////////////////////////////////////////////
// Returns TRUE if cancel pressed or a valid game slot was selected.
// if when returning TRUE strlen(sRequestResult) != 0 then a valid game
// slot was selected otherwise cancel was selected..
static BOOL _runLoadSave(BOOL bResetMissionWidgets)
{
	UDWORD		id=0;
	W_EDBINIT	sEdInit;
	CHAR		sTemp[MAX_STR_LENGTH];
	UDWORD		i;
	W_CONTEXT		context;
	BOOL		bSkipCD = FALSE;

	id = widgRunScreen(psRequestScreen);

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
				goto success;
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

	if(psWidget->pUserData)
	{
		col = COL_GREEN;
	}
	else
	{
		col = COL_RED;
	}

	iV_BoxFill(x,y,x+psWidget->width,y+psWidget->height,col);
	iV_BoxFill(x+2,y+2,x+psWidget->width-2,y+psWidget->height-2,COL_BLUE);


}
// ////////////////////////////////////////////////////////////////////////////
static void displayLoadSlot(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{

	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
//	UWORD	im = (UWORD)UNPACKDWORD_TRI_B((UDWORD)psWidget->pUserData);
//	UWORD	im2= (UWORD)(UNPACKDWORD_TRI_C((UDWORD)psWidget->pUserData));
	STRING  butString[64];

	drawBlueBox(x,y,psWidget->width,psWidget->height);	//draw box

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


	}

}
// ////////////////////////////////////////////////////////////////////////////
static void displayLoadSaveEdit(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours)
{
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	UDWORD	w = psWidget->width;
	UDWORD  h = psWidget->height;

	iV_BoxFill(x,y,x+w,y+h,COL_RED);
	iV_BoxFill(x+1,y+1,x+w-1,y+h-1,COL_BLUE);
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



