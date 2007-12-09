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
/*
 * loadsave.c
 * load and save Popup screens.
 *
 * these don't actually do any loading or saving, but just
 * return a filename to use for the ops.
 */

#include <ctype.h>
#include <physfs.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/widget/widget.h"
#include "lib/ivis_common/piepalette.h"		// for predefined colours.
#include "lib/ivis_common/rendmode.h"		// for boxfill
#include "hci.h"
#include "loadsave.h"
#include "multiplay.h"
#include "game.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
#include "frontend.h"
#include "main.h"
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
static BOOL _addLoadSave		(BOOL bLoad, const char *sSearchPath, const char *sExtension, const char *title);
static BOOL _runLoadSave		(BOOL bResetMissionWidgets);
static void displayLoadBanner	(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void displayLoadSlot		(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void displayLoadSaveEdit	(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void		removeWildcards		(char *pStr);

static	W_SCREEN	*psRequestScreen;					// Widget screen for requester
static	BOOL		mode;
static	UDWORD		chosenSlotId;

BOOL				bLoadSaveUp = FALSE;        // true when interface is up and should be run.
char				saveGameName[256];          //the name of the save game to load from the front end
char				sRequestResult[PATH_MAX];   // filename returned;
BOOL				bRequestLoad = FALSE;
LOADSAVE_MODE		bLoadSaveMode;

static char			sPath[255];
static char			sExt[4];

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
BOOL addLoadSave(LOADSAVE_MODE mode, const char *sSearchPath, const char *sExtension, const char *title)
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
static BOOL _addLoadSave(BOOL bLoad, const char *sSearchPath, const char *sExtension, const char *title)
{
	W_FORMINIT		sFormInit;
	W_BUTINIT		sButInit;
	W_LABINIT		sLabInit;
	UDWORD			slotCount;
// removed hardcoded values!  change with the defines above! -Q
	static char	sSlots[totalslots][totalslotspace];
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

				pie_UploadDisplayBuffer();			// Upload the current display back buffer into system memory.

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
	widgSetTipFont(psRequestScreen,font_regular);

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
	sFormInit.UserData = bLoad;
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
	sLabInit.FontID = font_regular;
	widgAddLabel(psRequestScreen, &sLabInit);


	// add cancel.
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID = LOADSAVE_BANNER;
	sButInit.x = 8;
	sButInit.y = 8;
	sButInit.width		= iV_GetImageWidth(IntImages,IMAGE_NRUTER);
	sButInit.height		= iV_GetImageHeight(IntImages,IMAGE_NRUTER);
	sButInit.UserData	= PACKDWORD_TRI(0,IMAGE_NRUTER , IMAGE_NRUTER);

	sButInit.id = LOADSAVE_CANCEL;
	sButInit.style = WBUT_PLAIN;
	sButInit.pTip = _("Close");
	sButInit.FontID = font_regular;
	sButInit.pDisplay = intDisplayImageHilight;
	widgAddButton(psRequestScreen, &sButInit);

	// add slots
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID		= LOADSAVE_FORM;
	sButInit.style		= WBUT_PLAIN;
	sButInit.width		= LOADENTRY_W;
	sButInit.height		= LOADENTRY_H;
	sButInit.pDisplay	= displayLoadSlot;
	sButInit.FontID		= font_regular;

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

	strlcpy(sPath, sSearchPath, sizeof(sPath));  // setup locals.
	strlcpy(sExt, sExtension, sizeof(sExt));

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
		strlcpy(sSlots[slotCount], *i, sizeof(sSlots[slotCount]));  //store it!
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
	for (i = files; *i != NULL; ++i)
	{
		char del_file[PATH_MAX];

		// Construct the full path to the file by appending the
		// filename to the directory it is in.
		snprintf(del_file, sizeof(del_file), "%s/%s", saveGameName, *i);

		debug(LOG_WZ, "Deleting [%s].", del_file);

		// Delete the file
		if (!PHYSFS_delete(del_file))
		{
			debug(LOG_ERROR, "Warning [%s] could not be deleted due to PhysicsFS error: %s", del_file, PHYSFS_getLastError());
		}
	}
	PHYSFS_freeList(files);

	if (!PHYSFS_delete(saveGameName))		// now (should be)empty directory
	{
		debug(LOG_ERROR, "Warning directory[%s] could not be deleted because %s", saveGameName,PHYSFS_getLastError());
	}
	
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
	static char     sDelete[PATH_MAX];
	UDWORD		i;
	W_CONTEXT		context;

	id = widgRunScreen(psRequestScreen);

	strlcpy(sRequestResult, "", sizeof(sRequestResult));					// set returned filename to null;

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
				sEdInit.FontID= font_regular;
				sEdInit.pBoxDisplay = displayLoadSaveEdit;
				widgAddEditBox(psRequestScreen, &sEdInit);

				if (((W_BUTTON *)widgGetFromID(psRequestScreen,id))->pText != NULL)
				{
					snprintf(sDelete, sizeof(sDelete), "%s%s.%s",
					         sPath,
					         ((W_BUTTON *)widgGetFromID(psRequestScreen,id))->pText ,
					         sExt);
				}
				else
				{
					strlcpy(sDelete, "", sizeof(sDelete));
				}

				widgHide(psRequestScreen,id);		// hide the old button
				chosenSlotId = id;

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
		char sTemp[MAX_STR_LENGTH];

		if(!keyPressed(KEY_RETURN))						// enter was not pushed, so not a vaild entry.
		{
			widgDelete(psRequestScreen,SAVEENTRY_EDIT);	//unselect this box, and go back ..
			widgReveal(psRequestScreen,chosenSlotId);
			return TRUE;
		}


		// scan to see if that game exists in another slot, if
		// so then fail.
		strlcpy(sTemp, ((W_EDITBOX *)widgGetFromID(psRequestScreen,id))->aText, sizeof(sTemp));

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
			strlcpy(sTemp, ((W_EDITBOX *)widgGetFromID(psRequestScreen,id))->aText, sizeof(sTemp));
			removeWildcards(sTemp);
			snprintf(sRequestResult, sizeof(sRequestResult), "%s%s.%s", sPath, sTemp, sExt);
			if (strlen(sDelete) != 0)
			{
				deleteSaveGame(sDelete);	//only delete game if a new game fills the slot
			}
		}
		else
		{
			goto failure;				// we entered a blank name..
		}

		// we're done. saving.
		closeLoadSave();
		bRequestLoad = FALSE;
        if (bResetMissionWidgets && widgGetFromID(psWScreen,IDMISSIONRES_FORM) == NULL)
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
    if (bResetMissionWidgets && widgGetFromID(psWScreen,IDMISSIONRES_FORM) == NULL)
	{
		resetMissionWidgets();
	}
    return TRUE;

// success on load.
success:
	setCampaignNumber( getCampaign(sRequestResult) );
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
// char HANDLER, replaces dos wildcards in a string with harmless chars.
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

static void displayLoadBanner(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	PIELIGHT col;
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;

	if(psWidget->pUserData)
	{
		col = WZCOL_GREEN;
	}
	else
	{
		col = WZCOL_MENU_LOAD_BORDER;
	}

	pie_BoxFill(x, y, x + psWidget->width, y + psWidget->height, col);
	pie_BoxFill(x + 2,y + 2, x + psWidget->width - 2, y + psWidget->height - 2, WZCOL_MENU_BACKGROUND);
}

// ////////////////////////////////////////////////////////////////////////////
static void displayLoadSlot(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{

	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	char  butString[64];

	drawBlueBox(x,y,psWidget->width,psWidget->height);	//draw box

	if(((W_BUTTON *)psWidget)->pTip )
	{
		strlcpy(butString, ((W_BUTTON *)psWidget)->pTip, sizeof(butString));

		iV_SetFont(font_regular);									// font
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);

		while(iV_GetTextWidth(butString) > psWidget->width)
		{
			butString[strlen(butString)-1]='\0';
		}

		//draw text
		iV_DrawText( butString, x+4, y+17);
	}
}

// ////////////////////////////////////////////////////////////////////////////
static void displayLoadSaveEdit(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	UDWORD	w = psWidget->width;
	UDWORD  h = psWidget->height;

	pie_BoxFill(x, y, x + w, y + h, WZCOL_MENU_LOAD_BORDER);
	pie_BoxFill(x + 1, y + 1, x + w - 1, y + h - 1, WZCOL_MENU_BACKGROUND);
}

// ////////////////////////////////////////////////////////////////////////////
void drawBlueBox(UDWORD x,UDWORD y, UDWORD w, UDWORD h)
{
	pie_BoxFill(x - 1, y - 1, x + w + 1, y + h + 1, WZCOL_MENU_BORDER);
	pie_BoxFill(x, y , x + w, y + h, WZCOL_MENU_BACKGROUND);
}
