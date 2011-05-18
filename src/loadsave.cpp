/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
#include <time.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/input.h"
#include "lib/framework/stdio_ext.h"
#include "lib/widget/button.h"
#include "lib/widget/editbox.h"
#include "lib/widget/widget.h"
#include "lib/ivis_opengl/piepalette.h"		// for predefined colours.
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"		// for boxfill
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

#define totalslots 36			// saves slots
#define slotsInColumn 12		// # of slots in a column
#define totalslotspace 64		// guessing 64 max chars for filename.

#define LOADSAVE_W				610
#define LOADSAVE_H				220
#define LOADSAVE_X				(getDualScreenCompat() ? (D_W / 2 + 16 - BASE_COORDS_X / 4) : (D_W + 16)) // When D_W is halved, its offset is also halved.. so we reapply it (BASE_COORDS_X/2) divided by 2 again.
#define LOADSAVE_Y				D_H + 5

#define MAX_SAVE_NAME			60

#define LOADSAVE_HGAP			9
#define LOADSAVE_VGAP			9
#define LOADSAVE_BANNER_DEPTH	40 		//top banner which displays either load or save

#define LOADENTRY_W				((LOADSAVE_W / 3 )-(3 * LOADSAVE_HGAP)) 
#define LOADENTRY_H				(LOADSAVE_H -(5 * LOADSAVE_VGAP )- (LOADSAVE_BANNER_DEPTH+LOADSAVE_VGAP) ) /5

#define ID_LOADSAVE				21000
#define LOADSAVE_FORM			ID_LOADSAVE+1		// back form.
#define LOADSAVE_CANCEL			ID_LOADSAVE+2		// cancel but.
#define LOADSAVE_LABEL			ID_LOADSAVE+3		// load/save
#define LOADSAVE_BANNER			ID_LOADSAVE+4		// banner.

#define LOADENTRY_START			ID_LOADSAVE+10		// each of the buttons.
#define LOADENTRY_END			ID_LOADSAVE+10 +totalslots  // must have unique ID hmm -Q

#define SAVEENTRY_EDIT			ID_LOADSAVE + totalslots + totalslots		// save edit box. must be highest value possible I guess. -Q

// ////////////////////////////////////////////////////////////////////////////
static void displayLoadBanner	(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void displayLoadSlot		(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void displayLoadSaveEdit	(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

static	W_SCREEN	*psRequestScreen;					// Widget screen for requester
static	bool		mode;
static	UDWORD		chosenSlotId;

bool				bLoadSaveUp = false;        // true when interface is up and should be run.
char				saveGameName[256];          //the name of the save game to load from the front end
char				sRequestResult[PATH_MAX];   // filename returned;
bool				bRequestLoad = false;
LOADSAVE_MODE		bLoadSaveMode;

static const char *sExt = ".gam";

// ////////////////////////////////////////////////////////////////////////////
// return whether the save screen was displayed in the mission results screen
bool saveInMissionRes(void)
{
	return bLoadSaveMode == SAVE_MISSIONEND;
}

// ////////////////////////////////////////////////////////////////////////////
// return whether the save screen was displayed in the middle of a mission
bool saveMidMission(void)
{
	return bLoadSaveMode == SAVE_INGAME;
}

// ////////////////////////////////////////////////////////////////////////////
bool addLoadSave(LOADSAVE_MODE savemode, const char *title)
{
	bool bLoad;

	bLoadSaveMode = savemode;
	UDWORD			slotCount;
// removed hardcoded values!  change with the defines above! -Q
	static char	sSlotCaps[totalslots][totalslotspace];
	static char	sSlotTips[totalslots][totalslotspace];
	char **i, **files;

	switch(savemode)
	{
	case LOAD_FRONTEND:
	case LOAD_MISSIONEND:
	case LOAD_INGAME:
		bLoad = true;
		break;
	case SAVE_MISSIONEND:
	case SAVE_INGAME:
	default:
		bLoad = false;
		break;
	}

	mode = bLoad;
	debug(LOG_SAVE, "called (%d, %s)", bLoad, title);

	if ((bLoadSaveMode == LOAD_INGAME) || (bLoadSaveMode == SAVE_INGAME))
	{
		if (!bMultiPlayer || (NetPlay.bComms ==0))
		{
			gameTimeStop();
			if(GetGameMode() == GS_NORMAL)
			{
				bool radOnScreen = radarOnScreen;				// Only do this in main game.

				bRender3DOnly = true;
				radarOnScreen = false;

				displayWorld();									// Just display the 3d, no interface

				pie_UploadDisplayBuffer();			// Upload the current display back buffer into system memory.

				radarOnScreen = radOnScreen;
				bRender3DOnly = false;
			}

			setGamePauseStatus( true );
			setGameUpdatePause(true);
			setScriptPause(true);
			setScrollPause(true);
			setConsolePause(true);

		}

		forceHidePowerBar();
		intRemoveReticule();
	}

	(void) PHYSFS_mkdir(SaveGamePath); // just in case

	psRequestScreen = widgCreateScreen(); // init the screen
	widgSetTipFont(psRequestScreen,font_regular);

	/* add a form to place the tabbed form on */
	W_FORMINIT sFormInit;
	sFormInit.formID = 0;				//this adds the blue background, and the "box" behind the buttons -Q
	sFormInit.id = LOADSAVE_FORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD) LOADSAVE_X;
	sFormInit.y = (SWORD) LOADSAVE_Y;
	sFormInit.width = LOADSAVE_W;
	// we need the form to be long enough for all resolutions, so we take the total number of items * height
	// and * the gaps, add the banner, and finally, the fudge factor ;)
	sFormInit.height = (slotsInColumn * LOADENTRY_H + LOADSAVE_HGAP* slotsInColumn)+ LOADSAVE_BANNER_DEPTH+20;
	sFormInit.disableChildren = true;
	sFormInit.pDisplay = intOpenPlainForm;
	widgAddForm(psRequestScreen, &sFormInit);

	// Add Banner
	sFormInit.formID = LOADSAVE_FORM;
	sFormInit.id = LOADSAVE_BANNER;
	sFormInit.x = LOADSAVE_HGAP;
	sFormInit.y = LOADSAVE_VGAP;
	sFormInit.width = LOADSAVE_W-(2*LOADSAVE_HGAP);
	sFormInit.height = LOADSAVE_BANNER_DEPTH;
	sFormInit.disableChildren = false;
	sFormInit.pDisplay = displayLoadBanner;
	sFormInit.UserData = bLoad;
	widgAddForm(psRequestScreen, &sFormInit);

	// Add Banner Label
	W_LABINIT sLabInit;
	sLabInit.formID = LOADSAVE_BANNER;
	sLabInit.id		= LOADSAVE_LABEL;
	sLabInit.style	= WLAB_ALIGNCENTRE;
	sLabInit.x		= 0;
	sLabInit.y		= 3;
	sLabInit.width	= LOADSAVE_W-(2*LOADSAVE_HGAP);	//LOADSAVE_W;
	sLabInit.height = LOADSAVE_BANNER_DEPTH;		//This looks right -Q
	sLabInit.pText	= title;
	widgAddLabel(psRequestScreen, &sLabInit);

	// add cancel.
	W_BUTINIT sButInit;
	sButInit.formID = LOADSAVE_BANNER;
	sButInit.x = 8;
	sButInit.y = 8;
	sButInit.width		= iV_GetImageWidth(IntImages,IMAGE_NRUTER);
	sButInit.height		= iV_GetImageHeight(IntImages,IMAGE_NRUTER);
	sButInit.UserData	= PACKDWORD_TRI(0,IMAGE_NRUTER , IMAGE_NRUTER);

	sButInit.id = LOADSAVE_CANCEL;
	sButInit.style = WBUT_PLAIN;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	widgAddButton(psRequestScreen, &sButInit);

	// add slots
	sButInit = W_BUTINIT();
	sButInit.formID		= LOADSAVE_FORM;
	sButInit.style		= WBUT_PLAIN;
	sButInit.width		= LOADENTRY_W;
	sButInit.height		= LOADENTRY_H;
	sButInit.pDisplay	= displayLoadSlot;

	for(slotCount = 0; slotCount< totalslots; slotCount++)
	{
		sButInit.id		= slotCount+LOADENTRY_START;

		if(slotCount < slotsInColumn)
		{
			sButInit.x	= 22 + LOADSAVE_HGAP;
			sButInit.y	= (SWORD)((LOADSAVE_BANNER_DEPTH +(2*LOADSAVE_VGAP)) + (
				slotCount*(LOADSAVE_VGAP+LOADENTRY_H)));
		}
		else if (slotCount >= slotsInColumn && (slotCount < (slotsInColumn *2)))
		{
			sButInit.x	= 22 + (2*LOADSAVE_HGAP + LOADENTRY_W);
			sButInit.y	= (SWORD)((LOADSAVE_BANNER_DEPTH +(2* LOADSAVE_VGAP)) + (
				(slotCount % slotsInColumn)*(LOADSAVE_VGAP+LOADENTRY_H)));
		}
		else
		{
			sButInit.x	= 22 + (3*LOADSAVE_HGAP + (2*LOADENTRY_W));
			sButInit.y	= (SWORD)((LOADSAVE_BANNER_DEPTH +(2* LOADSAVE_VGAP)) + (
				(slotCount % slotsInColumn)*(LOADSAVE_VGAP+LOADENTRY_H)));
		}
		widgAddButton(psRequestScreen, &sButInit);
	}

	// fill slots.
	slotCount = 0;

	debug(LOG_SAVE, "Searching \"%s\" for savegames", SaveGamePath);

	// add savegame filenames minus extensions to buttons
	files = PHYSFS_enumerateFiles(SaveGamePath);
	for (i = files; *i != NULL; ++i)
	{
		W_BUTTON *button;
		char savefile[256];
		time_t savetime;
		struct tm *timeinfo;

		// See if this filename contains the extension we're looking for
		if (!strstr(*i, sExt))
		{
			// If it doesn't, move on to the next filename
			continue;
		}

		button = (W_BUTTON*)widgGetFromID(psRequestScreen, LOADENTRY_START + slotCount);

		debug(LOG_SAVE, "We found [%s]", *i);

		/* Figure save-time */
		snprintf(savefile, sizeof(savefile), "%s/%s", SaveGamePath, *i);
		savetime = PHYSFS_getLastModTime(savefile);
		timeinfo = localtime(&savetime);
		strftime(sSlotTips[slotCount], sizeof(sSlotTips[slotCount]), "%x %X", timeinfo);

		/* Set the button-text */
		(*i)[strlen(*i) - 4] = '\0'; // remove .gam extension
		sstrcpy(sSlotCaps[slotCount], *i);  //store it!
		
		/* Add button */
		button->pTip = sSlotTips[slotCount];
		button->pText = sSlotCaps[slotCount];
		slotCount++;		// goto next but...
		if (slotCount == totalslots)
		{
			break;
		}
	}
	PHYSFS_freeList(files);

	bLoadSaveUp = true;
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
bool closeLoadSave(void)
{
	widgDelete(psRequestScreen,LOADSAVE_FORM);
	bLoadSaveUp = false;

	if ((bLoadSaveMode == LOAD_INGAME) || (bLoadSaveMode == SAVE_INGAME))
	{

		if (!bMultiPlayer || (NetPlay.bComms == 0))
		{
			gameTimeStart();
			setGamePauseStatus( false );
			setGameUpdatePause(false);
			setScriptPause(false);
			setScrollPause(false);
			setConsolePause(false);
		}

		intAddReticule();
		intShowPowerBar();

	}
	widgReleaseScreen(psRequestScreen);
	// need to "eat" up the return key so it don't pass back to game.
	inputLooseFocus();
	return true;
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

		debug(LOG_SAVE, "Deleting [%s].", del_file);

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
// Returns true if cancel pressed or a valid game slot was selected.
// if when returning true strlen(sRequestResult) != 0 then a valid game slot was selected
// otherwise cancel was selected..
bool runLoadSave(bool bResetMissionWidgets)
{
	UDWORD		id=0;
	static char     sDelete[PATH_MAX];
	UDWORD		i, campaign;
	W_CONTEXT		context;

	id = widgRunScreen(psRequestScreen);

	sstrcpy(sRequestResult, "");					// set returned filename to null;

	// cancel this operation...
	if(id == LOADSAVE_CANCEL || CancelPressed() )
	{
		goto cleanup;
	}

	// clicked a load entry
	if( id >= LOADENTRY_START  &&  id <= LOADENTRY_END )
	{

		if (mode)								// Loading, return that entry.
		{
			if( ((W_BUTTON *)widgGetFromID(psRequestScreen,id))->pText )
			{
				sprintf(sRequestResult, "%s%s%s", SaveGamePath, ((W_BUTTON *)widgGetFromID(psRequestScreen,id))->pText, sExt);
			}
			else
			{
				return false;				// clicked on an empty box
			}

			goto success;
		}
		else //  SAVING!add edit box at that position.
		{

			if( ! widgGetFromID(psRequestScreen,SAVEENTRY_EDIT))
			{
				// add blank box.
				W_EDBINIT sEdInit;
				sEdInit.formID= LOADSAVE_FORM;
				sEdInit.id    = SAVEENTRY_EDIT;
				sEdInit.x	  =	widgGetFromID(psRequestScreen,id)->x;
				sEdInit.y     =	widgGetFromID(psRequestScreen,id)->y;
				sEdInit.width = widgGetFromID(psRequestScreen,id)->width;
				sEdInit.height= widgGetFromID(psRequestScreen,id)->height;
				sEdInit.pText = ((W_BUTTON *)widgGetFromID(psRequestScreen,id))->pText;
				sEdInit.pBoxDisplay = displayLoadSaveEdit;
				widgAddEditBox(psRequestScreen, &sEdInit);

				if (((W_BUTTON *)widgGetFromID(psRequestScreen,id))->pText != NULL)
				{
					snprintf(sDelete, sizeof(sDelete), "%s%s%s", SaveGamePath,
					         ((W_BUTTON *)widgGetFromID(psRequestScreen,id))->pText, sExt);
				}
				else
				{
					sstrcpy(sDelete, "");
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
				widgGetFromID(psRequestScreen, SAVEENTRY_EDIT)->clicked(&context);
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

		if(!keyPressed(KEY_RETURN) && !keyPressed(KEY_KPENTER))						// enter was not pushed, so not a vaild entry.
		{
			widgDelete(psRequestScreen,SAVEENTRY_EDIT);	//unselect this box, and go back ..
			widgReveal(psRequestScreen,chosenSlotId);
			return true;
		}


		// scan to see if that game exists in another slot, if so then fail.
		sstrcpy(sTemp, widgGetString(psRequestScreen, id));

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
				//	setMousePos(widgGetFromID(psRequestScreen,i)->pos.x ,widgGetFromID(psRequestScreen,i)->pos.y);
					audio_PlayTrack(ID_SOUND_BUILD_FAIL);
					return true;
				}
			}
		}


		// return with this name, as we've edited it.
		if (strlen(widgGetString(psRequestScreen, id)))
		{
			sstrcpy(sTemp, widgGetString(psRequestScreen, id));
			removeWildcards(sTemp);
			snprintf(sRequestResult, sizeof(sRequestResult), "%s%s%s", SaveGamePath, sTemp, sExt);
			if (strlen(sDelete) != 0)
			{
				deleteSaveGame(sDelete);	//only delete game if a new game fills the slot
			}
		}
		
		goto cleanup;
	}

	return false;

// failed and/or cancelled..
cleanup:
	closeLoadSave();
	bRequestLoad = false;
    if (bResetMissionWidgets && widgGetFromID(psWScreen,IDMISSIONRES_FORM) == NULL)
	{
		resetMissionWidgets();			//reset the mission widgets here if necessary
	}
    return true;

// success on load.
success:
	campaign = getCampaign(sRequestResult);
	setCampaignNumber(campaign);
	debug(LOG_WZ, "Set campaign for %s to %u", sRequestResult, campaign);
	closeLoadSave();
	bRequestLoad = true;
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// should be done when drawing the other widgets.
bool displayLoadSave(void)
{
	widgDisplayScreen(psRequestScreen);	// display widgets.
	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// char HANDLER, replaces dos wildcards in a string with harmless chars.
void removeWildcards(char *pStr)
{
	UDWORD i;
	
	// Remember never to allow: < > : " / \ | ? *
	
	// Whitelist: Get rid of any characters except:
	// a-z A-Z 0-9 - + ! , = ^ @ # $ % & ' ( ) [ ] (and space and unicode characters ≥ 0x80)
	for (i=0; i<strlen(pStr); i++)
	{
		if (!isalnum(pStr[i])
		    && (pStr[i] != ' ' || i==0 || pStr[i-1]==' ')
			// We allow spaces as long as they aren't the first char, or two spaces in a row
		    && pStr[i] != '-'
		    && pStr[i] != '+'
		    && pStr[i] != '!'
		    && pStr[i] != ','
		    && pStr[i] != '='
		    && pStr[i] != '^'
		    && pStr[i] != '@'
		    && (pStr[i]<35 || pStr[i]>41) // # $ % & ' ( )
		    && pStr[i] != '[' && pStr[i] != ']'
		    && (pStr[i]&0x80) != 0x80  // á é í ó ú α β γ δ ε
			)
		{
			pStr[i] = '_';
		}
	}
	
	if (strlen(pStr) >= MAX_SAVE_NAME)
	{
		pStr[MAX_SAVE_NAME - 1] = 0;
	}
	else if (strlen(pStr) == 0)
	{
		pStr[0] = '!';
		pStr[1] = 0;
		return;
	}
	// Trim trailing spaces
	for (i = strlen(pStr); i > 0 && pStr[i - 1] == ' '; --i)
	{}
	pStr[i] = 0;

	// Trims leading spaces (currently unused)
	/* for (i=0; pStr[i]==' '; i++);
	 if (i != 0)
	 {
	 memmove(pStr, pStr+i, strlen(pStr)+1-i);
	 } */
	
	// If that leaves us with a blank string, replace with '!'
	if (pStr[0] == 0)
	{
		pStr[0] = '!';
		pStr[1] = 0;
	}
	
	return;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// DISPLAY FUNCTIONS

static void displayLoadBanner(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
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
static void displayLoadSlot(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{

	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	char  butString[64];

	drawBlueBox(x,y,psWidget->width,psWidget->height);	//draw box

	if(((W_BUTTON *)psWidget)->pText )
	{
		sstrcpy(butString, ((W_BUTTON *)psWidget)->pText);

		iV_SetFont(font_regular);									// font
		iV_SetTextColour(WZCOL_FORM_TEXT);

		while(iV_GetTextWidth(butString) > psWidget->width)
		{
			butString[strlen(butString)-1]='\0';
		}

		//draw text
		iV_DrawText( butString, x+4, y+17);
	}
}

// ////////////////////////////////////////////////////////////////////////////
static void displayLoadSaveEdit(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
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
