/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "lib/netplay/netplay.h"
#include "loop.h"
#include "intdisplay.h"
#include "mission.h"
#include "lib/gamelib/gtime.h"

#define totalslots 36			// saves slots
#define slotsInColumn 12		// # of slots in a column
#define totalslotspace 64		// guessing 64 max chars for filename.

#define LOADSAVE_X				D_W + 16
#define LOADSAVE_Y				D_H + 5
#define LOADSAVE_W				610
#define LOADSAVE_H				220

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
static void displayLoadBanner(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void displayLoadSlot(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

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
bool saveInMissionRes()
{
	return bLoadSaveMode == SAVE_MISSIONEND;
}

// ////////////////////////////////////////////////////////////////////////////
// return whether the save screen was displayed in the middle of a mission
bool saveMidMission()
{
	return bLoadSaveMode == SAVE_INGAME_MISSION;
}

// ////////////////////////////////////////////////////////////////////////////
bool addLoadSave(LOADSAVE_MODE savemode, const char *title)
{
	bool bLoad = true;
	char NewSaveGamePath[PATH_MAX] = {'\0'};
	bLoadSaveMode = savemode;
	UDWORD			slotCount;
	static char	sSlotCaps[totalslots][totalslotspace];
	static char	sSlotTips[totalslots][totalslotspace];
	char **i, **files;

	switch (savemode)
	{
	case LOAD_FRONTEND_MISSION:
	case LOAD_INGAME_MISSION:
	case LOAD_MISSIONEND:
		ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "campaign");
		break;
	case LOAD_FRONTEND_SKIRMISH:
	case LOAD_INGAME_SKIRMISH:
		ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "skirmish");
		break;
	case SAVE_MISSIONEND:
	case SAVE_INGAME_MISSION:
		ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "campaign");
		bLoad = false;
		break;
	case SAVE_INGAME_SKIRMISH:
		ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "skirmish");
		bLoad = false;
		break;
	default:
		ASSERT("Invalid load/save mode!", "Invalid load/save mode!");
		ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "campaign");
		break;
	}

	mode = bLoad;
	debug(LOG_SAVE, "called (%d, %s)", bLoad, title);

	if ((bLoadSaveMode == LOAD_INGAME_MISSION) || (bLoadSaveMode == SAVE_INGAME_MISSION)
	    || (bLoadSaveMode == LOAD_INGAME_SKIRMISH) || (bLoadSaveMode == SAVE_INGAME_SKIRMISH))
	{
		if (!bMultiPlayer || (NetPlay.bComms == 0))
		{
			gameTimeStop();
			if (GetGameMode() == GS_NORMAL)
			{
				bool radOnScreen = radarOnScreen;				// Only do this in main game.

				bRender3DOnly = true;
				radarOnScreen = false;

				displayWorld();									// Just display the 3d, no interface

				radarOnScreen = radOnScreen;
				bRender3DOnly = false;
			}

			setGamePauseStatus(true);
			setGameUpdatePause(true);
			setScriptPause(true);
			setScrollPause(true);
			setConsolePause(true);

		}

		forceHidePowerBar();
		intRemoveReticule();
	}

	psRequestScreen = new W_SCREEN;

	WIDGET *parent = psRequestScreen->psForm;

	/* add a form to place the tabbed form on */
	// we need the form to be long enough for all resolutions, so we take the total number of items * height
	// and * the gaps, add the banner, and finally, the fudge factor ;)
	IntFormAnimated *loadSaveForm = new IntFormAnimated(parent);
	loadSaveForm->id = LOADSAVE_FORM;
	loadSaveForm->setGeometry(LOADSAVE_X, LOADSAVE_Y, LOADSAVE_W, slotsInColumn * (LOADENTRY_H + LOADSAVE_HGAP) + LOADSAVE_BANNER_DEPTH + 20);

	// Add Banner
	W_FORMINIT sFormInit;
	sFormInit.formID = LOADSAVE_FORM;
	sFormInit.id = LOADSAVE_BANNER;
	sFormInit.x = LOADSAVE_HGAP;
	sFormInit.y = LOADSAVE_VGAP;
	sFormInit.width = LOADSAVE_W - (2 * LOADSAVE_HGAP);
	sFormInit.height = LOADSAVE_BANNER_DEPTH;
	sFormInit.pDisplay = displayLoadBanner;
	sFormInit.UserData = bLoad;
	widgAddForm(psRequestScreen, &sFormInit);

	// Add Banner Label
	W_LABINIT sLabInit;
	sLabInit.formID = LOADSAVE_BANNER;
	sLabInit.FontID = font_large;
	sLabInit.id		= LOADSAVE_LABEL;
	sLabInit.style	= WLAB_ALIGNCENTRE;
	sLabInit.x		= 0;
	sLabInit.y		= 0;
	sLabInit.width	= LOADSAVE_W - (2 * LOADSAVE_HGAP);	//LOADSAVE_W;
	sLabInit.height = LOADSAVE_BANNER_DEPTH;		//This looks right -Q
	sLabInit.pText	= title;
	widgAddLabel(psRequestScreen, &sLabInit);

	// add cancel.
	W_BUTINIT sButInit;
	sButInit.formID = LOADSAVE_BANNER;
	sButInit.x = 8;
	sButInit.y = 10;
	sButInit.width		= iV_GetImageWidth(IntImages, IMAGE_NRUTER);
	sButInit.height		= iV_GetImageHeight(IntImages, IMAGE_NRUTER);
	sButInit.UserData	= PACKDWORD_TRI(0, IMAGE_NRUTER , IMAGE_NRUTER);

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

	for (slotCount = 0; slotCount < totalslots; slotCount++)
	{
		sButInit.id		= slotCount + LOADENTRY_START;

		if (slotCount < slotsInColumn)
		{
			sButInit.x	= 22 + LOADSAVE_HGAP;
			sButInit.y	= (SWORD)((LOADSAVE_BANNER_DEPTH + (2 * LOADSAVE_VGAP)) + (
			                          slotCount * (LOADSAVE_VGAP + LOADENTRY_H)));
		}
		else if (slotCount >= slotsInColumn && (slotCount < (slotsInColumn * 2)))
		{
			sButInit.x	= 22 + (2 * LOADSAVE_HGAP + LOADENTRY_W);
			sButInit.y	= (SWORD)((LOADSAVE_BANNER_DEPTH + (2 * LOADSAVE_VGAP)) + (
			                          (slotCount % slotsInColumn) * (LOADSAVE_VGAP + LOADENTRY_H)));
		}
		else
		{
			sButInit.x	= 22 + (3 * LOADSAVE_HGAP + (2 * LOADENTRY_W));
			sButInit.y	= (SWORD)((LOADSAVE_BANNER_DEPTH + (2 * LOADSAVE_VGAP)) + (
			                          (slotCount % slotsInColumn) * (LOADSAVE_VGAP + LOADENTRY_H)));
		}
		widgAddButton(psRequestScreen, &sButInit);
	}

	// fill slots.
	slotCount = 0;

	debug(LOG_SAVE, "Searching \"%s\" for savegames", NewSaveGamePath);

	// add savegame filenames minus extensions to buttons
	files = PHYSFS_enumerateFiles(NewSaveGamePath);
	for (i = files; *i != nullptr; ++i)
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

		button = (W_BUTTON *)widgGetFromID(psRequestScreen, LOADENTRY_START + slotCount);

		debug(LOG_SAVE, "We found [%s]", *i);

		/* Figure save-time */
		snprintf(savefile, sizeof(savefile), "%s/%s", NewSaveGamePath, *i);
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
bool closeLoadSave()
{
	bLoadSaveUp = false;

	if ((bLoadSaveMode == LOAD_INGAME_MISSION) || (bLoadSaveMode == SAVE_INGAME_MISSION)
	    || (bLoadSaveMode == LOAD_INGAME_SKIRMISH) || (bLoadSaveMode == SAVE_INGAME_SKIRMISH))
	{

		if (!bMultiPlayer || (NetPlay.bComms == 0))
		{
			gameTimeStart();
			setGamePauseStatus(false);
			setGameUpdatePause(false);
			setScriptPause(false);
			setScrollPause(false);
			setConsolePause(false);
		}

		intAddReticule();
		intShowPowerBar();

	}
	delete psRequestScreen;
	psRequestScreen = nullptr;
	// need to "eat" up the return key so it don't pass back to game.
	inputLoseFocus();
	return true;
}

/***************************************************************************
	Delete a savegame.  saveGameName should be a .gam extension save game
	filename reference.  We delete this file, any .es file with the same
	name, and any files in the directory with the same name.
***************************************************************************/
void deleteSaveGame(char *saveGameName)
{
	char **files, **i;

	ASSERT(strlen(saveGameName) < MAX_STR_LENGTH, "deleteSaveGame; save game name too long");

	PHYSFS_delete(saveGameName);
	saveGameName[strlen(saveGameName) - 4] = '\0'; // strip extension

	strcat(saveGameName, ".es");					// remove script data if it exists.
	PHYSFS_delete(saveGameName);
	saveGameName[strlen(saveGameName) - 3] = '\0'; // strip extension

	// check for a directory and remove that too.
	files = PHYSFS_enumerateFiles(saveGameName);
	for (i = files; *i != nullptr; ++i)
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
		debug(LOG_ERROR, "Warning directory[%s] could not be deleted because %s", saveGameName, PHYSFS_getLastError());
	}

	return;
}


// ////////////////////////////////////////////////////////////////////////////
// Returns true if cancel pressed or a valid game slot was selected.
// if when returning true strlen(sRequestResult) != 0 then a valid game slot was selected
// otherwise cancel was selected..
bool runLoadSave(bool bResetMissionWidgets)
{
	static char     sDelete[PATH_MAX];
	UDWORD		i, campaign;
	W_CONTEXT		context;
	char NewSaveGamePath[PATH_MAX] = {'\0'};

	WidgetTriggers const &triggers = widgRunScreen(psRequestScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	sstrcpy(sRequestResult, "");					// set returned filename to null;

	// cancel this operation...
	if (id == LOADSAVE_CANCEL || CancelPressed())
	{
		goto cleanup;
	}
	if (bMultiPlayer)
	{
		ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "skirmish");
	}
	else
	{
		ssprintf(NewSaveGamePath, "%s%s/", SaveGamePath, "campaign");
	}
	// clicked a load entry
	if (id >= LOADENTRY_START  &&  id <= LOADENTRY_END)
	{
		W_BUTTON *slotButton = (W_BUTTON *)widgGetFromID(psRequestScreen, id);

		if (mode)								// Loading, return that entry.
		{
			if (!slotButton->pText.isEmpty())
			{
				ssprintf(sRequestResult, "%s%s%s", NewSaveGamePath, ((W_BUTTON *)widgGetFromID(psRequestScreen, id))->pText.toUtf8().constData(), sExt);
			}
			else
			{
				return false;				// clicked on an empty box
			}

			goto success;
		}
		else //  SAVING!add edit box at that position.
		{

			if (! widgGetFromID(psRequestScreen, SAVEENTRY_EDIT))
			{
				WIDGET *parent = widgGetFromID(psRequestScreen, LOADSAVE_FORM);

				// add blank box.
				W_EDITBOX *saveEntryEdit = new W_EDITBOX(parent);
				saveEntryEdit->id = SAVEENTRY_EDIT;
				saveEntryEdit->setGeometry(slotButton->geometry());
				saveEntryEdit->setString(slotButton->getString());
				saveEntryEdit->setBoxColours(WZCOL_MENU_LOAD_BORDER, WZCOL_MENU_LOAD_BORDER, WZCOL_MENU_BACKGROUND);

				if (!slotButton->pText.isEmpty())
				{
					ssprintf(sDelete, "%s%s%s", NewSaveGamePath, slotButton->pText.toUtf8().constData(), sExt);
				}
				else
				{
					sstrcpy(sDelete, "");
				}

				slotButton->hide();  // hide the old button
				chosenSlotId = id;

				// auto click in the edit box we just made.
				context.xOffset		= 0;
				context.yOffset		= 0;
				context.mx			= mouseX();
				context.my			= mouseY();
				saveEntryEdit->clicked(&context);
			}
			else
			{
				// clicked in a different box. shouldnt be possible!(since we autoclicked in editbox)
			}
		}
	}

	// finished entering a name.
	if (id == SAVEENTRY_EDIT)
	{
		char sTemp[MAX_STR_LENGTH];

		if (!keyPressed(KEY_RETURN) && !keyPressed(KEY_KPENTER))						// enter was not pushed, so not a vaild entry.
		{
			widgDelete(psRequestScreen, SAVEENTRY_EDIT);	//unselect this box, and go back ..
			widgReveal(psRequestScreen, chosenSlotId);
			return true;
		}


		// scan to see if that game exists in another slot, if so then fail.
		sstrcpy(sTemp, widgGetString(psRequestScreen, id));

		for (i = LOADENTRY_START; i < LOADENTRY_END; i++)
		{
			if (i != chosenSlotId)
			{

				if (!((W_BUTTON *)widgGetFromID(psRequestScreen, i))->pText.isEmpty()
				    && strcmp(sTemp, ((W_BUTTON *)widgGetFromID(psRequestScreen, i))->pText.toUtf8().constData()) == 0)
				{
					widgDelete(psRequestScreen, SAVEENTRY_EDIT);	//unselect this box, and go back ..
					widgReveal(psRequestScreen, chosenSlotId);
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
			snprintf(sRequestResult, sizeof(sRequestResult), "%s%s%s", NewSaveGamePath, sTemp, sExt);
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
	if (bResetMissionWidgets && widgGetFromID(psWScreen, IDMISSIONRES_FORM) == nullptr)
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
bool displayLoadSave()
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
	for (i = 0; i < strlen(pStr); i++)
	{
		if (!isalnum(pStr[i])
		    && (pStr[i] != ' ' || i == 0 || pStr[i - 1] == ' ')
		    // We allow spaces as long as they aren't the first char, or two spaces in a row
		    && pStr[i] != '-'
		    && pStr[i] != '+'
		    && pStr[i] != '[' && pStr[i] != ']'
		    && (pStr[i] & 0x80) != 0x80 // á é í ó ú α β γ δ ε
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

static void displayLoadBanner(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	PIELIGHT col;
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();

	if (psWidget->pUserData)
	{
		col = WZCOL_GREEN;
	}
	else
	{
		col = WZCOL_MENU_LOAD_BORDER;
	}

	pie_BoxFill(x, y, x + psWidget->width(), y + psWidget->height(), col);
	pie_BoxFill(x + 2, y + 2, x + psWidget->width() - 2, y + psWidget->height() - 2, WZCOL_MENU_BACKGROUND);
}

// ////////////////////////////////////////////////////////////////////////////
static void displayLoadSlot(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	char  butString[64];

	drawBlueBox(x, y, psWidget->width(), psWidget->height());  //draw box

	if (!((W_BUTTON *)psWidget)->pText.isEmpty())
	{
		sstrcpy(butString, ((W_BUTTON *)psWidget)->pText.toUtf8().constData());

		iV_SetTextColour(WZCOL_FORM_TEXT);
		while (iV_GetTextWidth(butString, font_regular) > psWidget->width())
		{
			butString[strlen(butString) - 1] = '\0';
		}

		iV_DrawText(butString, x + 4, y + 17, font_regular);
	}
}

void drawBlueBox(UDWORD x, UDWORD y, UDWORD w, UDWORD h)
{
	pie_BoxFill(x - 1, y - 1, x + w + 1, y + h + 1, WZCOL_MENU_BORDER);
	pie_BoxFill(x, y , x + w, y + h, WZCOL_MENU_BACKGROUND);
}
