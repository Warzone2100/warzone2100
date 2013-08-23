/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
 * @file challenge.c
 * Run challenges dialog.
 *
 */

#include <ctype.h>
#include <physfs.h>
#include <time.h>
#ifndef WIN32
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <QtCore/QTime>
#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/wzconfig.h"
#include "lib/netplay/netplay.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/widget/button.h"

#include "challenge.h"
#include "frontend.h"
#include "hci.h"
#include "intdisplay.h"
#include "loadsave.h"
#include "multiplay.h"
#include "scores.h"
#include "mission.h"

#define totalslots 36			// challenge slots
#define slotsInColumn 12		// # of slots in a column
#define totalslotspace 256		// max chars for slot strings.

#define CHALLENGE_X				D_W + 16
#define CHALLENGE_Y				D_H + 5
#define CHALLENGE_W				610
#define CHALLENGE_H				220

#define CHALLENGE_HGAP			9
#define CHALLENGE_VGAP			9
#define CHALLENGE_BANNER_DEPTH	40 		//top banner which displays either load or save

#define CHALLENGE_ENTRY_W				((CHALLENGE_W / 3 )-(3 * CHALLENGE_HGAP))
#define CHALLENGE_ENTRY_H				(CHALLENGE_H -(5 * CHALLENGE_VGAP )- (CHALLENGE_BANNER_DEPTH+CHALLENGE_VGAP) ) /5

#define ID_LOADSAVE				21000
#define CHALLENGE_FORM			ID_LOADSAVE+1		// back form.
#define CHALLENGE_CANCEL			ID_LOADSAVE+2		// cancel but.
#define CHALLENGE_LABEL			ID_LOADSAVE+3		// load/save
#define CHALLENGE_BANNER			ID_LOADSAVE+4		// banner.

#define CHALLENGE_ENTRY_START			ID_LOADSAVE+10		// each of the buttons.
#define CHALLENGE_ENTRY_END			ID_LOADSAVE+10 +totalslots  // must have unique ID hmm -Q

static	W_SCREEN	*psRequestScreen;					// Widget screen for requester

bool		challengesUp = false;		///< True when interface is up and should be run.
bool		challengeActive = false;	///< Whether we are running a challenge

static void displayLoadBanner(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	PIELIGHT col = WZCOL_GREEN;
	UDWORD	x = xOffset + psWidget->x();
	UDWORD	y = yOffset + psWidget->y();

	pie_BoxFill(x, y, x + psWidget->width(), y + psWidget->height(), col);
	pie_BoxFill(x + 2, y + 2, x + psWidget->width() - 2, y + psWidget->height() - 2, WZCOL_MENU_BACKGROUND);
}

// quite the hack, game name is stored in global sRequestResult
void updateChallenge(bool gameWon)
{
	char sPath[64], *fStr;
	int seconds = 0, newtime = (gameTime - mission.startTime) / GAME_TICKS_PER_SEC;
	bool victory = false;
	WzConfig scores(CHALLENGE_SCORES);

	fStr = strrchr(sRequestResult, '/');
	fStr++;	// skip slash
	if (fStr == '\0')
	{
		debug(LOG_ERROR, "Bad path to challenge file (%s)", sRequestResult);
		return;
	}
	sstrcpy(sPath, fStr);
	sPath[strlen(sPath) - 4] = '\0';	// remove .ini
	scores.beginGroup(sPath);
	victory = scores.value("Victory", false).toBool();
	seconds = scores.value("Seconds", 0).toInt();

	// Update score if we have a victory and best recorded was a loss,
	// or both were losses but time is higher, or both were victories
	// but time is lower.
	if ((!victory && gameWon)
	    || (!gameWon && !victory && newtime > seconds)
	    || (gameWon && victory && newtime < seconds))
	{
		scores.setValue("Seconds", newtime);
		scores.setValue("Victory", gameWon);
		scores.setValue("Player", NetPlay.players[selectedPlayer].name);
	}
	scores.endGroup();
}

// ////////////////////////////////////////////////////////////////////////////
static void displayLoadSlot(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{

	UDWORD	x = xOffset + psWidget->x();
	UDWORD	y = yOffset + psWidget->y();
	char  butString[64];

	drawBlueBox(x, y, psWidget->width(), psWidget->height());	//draw box

	if (!((W_BUTTON *)psWidget)->pText.isEmpty())
	{
		sstrcpy(butString, ((W_BUTTON *)psWidget)->pText.toUtf8().constData());

		iV_SetFont(font_regular);									// font
		iV_SetTextColour(WZCOL_FORM_TEXT);

		while (iV_GetTextWidth(butString) > psWidget->width())
		{
			butString[strlen(butString)-1] = '\0';
		}

		//draw text
		iV_DrawText(butString, x + 4, y + 17);
	}
}

//****************************************************************************************
// Challenge menu
//*****************************************************************************************
bool addChallenges()
{
	char			sPath[PATH_MAX];
	const char *sSearchPath	= "challenges";
	UDWORD			slotCount;
	static char		sSlotCaps[totalslots][totalslotspace];
	static char		sSlotTips[totalslots][totalslotspace];
	static char		sSlotFile[totalslots][totalslotspace];
	char **i, **files;

	(void) PHYSFS_mkdir(sSearchPath); // just in case

	psRequestScreen = new W_SCREEN; // init the screen

	WIDGET *parent = psRequestScreen->psForm;

	/* add a form to place the tabbed form on */
	IntFormAnimated *challengeForm = new IntFormAnimated(parent);
	challengeForm->id = CHALLENGE_FORM;
	challengeForm->setGeometry(CHALLENGE_X, CHALLENGE_Y, CHALLENGE_W, (slotsInColumn * CHALLENGE_ENTRY_H + CHALLENGE_HGAP * slotsInColumn) + CHALLENGE_BANNER_DEPTH + 20);

	// Add Banner
	W_FORMINIT sFormInit;
	sFormInit.formID = CHALLENGE_FORM;
	sFormInit.id = CHALLENGE_BANNER;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = CHALLENGE_HGAP;
	sFormInit.y = CHALLENGE_VGAP;
	sFormInit.width = CHALLENGE_W - (2 * CHALLENGE_HGAP);
	sFormInit.height = CHALLENGE_BANNER_DEPTH;
	sFormInit.pDisplay = displayLoadBanner;
	sFormInit.UserData = 0;
	widgAddForm(psRequestScreen, &sFormInit);

	// Add Banner Label
	W_LABINIT sLabInit;
	sLabInit.formID		= CHALLENGE_BANNER;
	sLabInit.id		= CHALLENGE_LABEL;
	sLabInit.style		= WLAB_ALIGNCENTRE;
	sLabInit.x		= 0;
	sLabInit.y		= 3;
	sLabInit.width		= CHALLENGE_W - (2 * CHALLENGE_HGAP);	//CHALLENGE_W;
	sLabInit.height		= CHALLENGE_BANNER_DEPTH;		//This looks right -Q
	sLabInit.pText		= "Challenge";
	widgAddLabel(psRequestScreen, &sLabInit);

	// add cancel.
	W_BUTINIT sButInit;
	sButInit.formID = CHALLENGE_BANNER;
	sButInit.x = 8;
	sButInit.y = 8;
	sButInit.width		= iV_GetImageWidth(IntImages, IMAGE_NRUTER);
	sButInit.height		= iV_GetImageHeight(IntImages, IMAGE_NRUTER);
	sButInit.UserData	= PACKDWORD_TRI(0, IMAGE_NRUTER , IMAGE_NRUTER);

	sButInit.id = CHALLENGE_CANCEL;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	widgAddButton(psRequestScreen, &sButInit);

	// add slots
	sButInit = W_BUTINIT();
	sButInit.formID		= CHALLENGE_FORM;
	sButInit.width		= CHALLENGE_ENTRY_W;
	sButInit.height		= CHALLENGE_ENTRY_H;
	sButInit.pDisplay	= displayLoadSlot;

	for (slotCount = 0; slotCount < totalslots; slotCount++)
	{
		sButInit.id		= slotCount + CHALLENGE_ENTRY_START;

		if (slotCount < slotsInColumn)
		{
			sButInit.x	= 22 + CHALLENGE_HGAP;
			sButInit.y	= (SWORD)((CHALLENGE_BANNER_DEPTH + (2 * CHALLENGE_VGAP)) + (
			                         slotCount * (CHALLENGE_VGAP + CHALLENGE_ENTRY_H)));
		}
		else if (slotCount >= slotsInColumn && (slotCount < (slotsInColumn *2)))
		{
			sButInit.x	= 22 + (2 * CHALLENGE_HGAP + CHALLENGE_ENTRY_W);
			sButInit.y	= (SWORD)((CHALLENGE_BANNER_DEPTH + (2 * CHALLENGE_VGAP)) + (
			                         (slotCount % slotsInColumn) * (CHALLENGE_VGAP + CHALLENGE_ENTRY_H)));
		}
		else
		{
			sButInit.x	= 22 + (3 * CHALLENGE_HGAP + (2 * CHALLENGE_ENTRY_W));
			sButInit.y	= (SWORD)((CHALLENGE_BANNER_DEPTH + (2 * CHALLENGE_VGAP)) + (
			                         (slotCount % slotsInColumn) * (CHALLENGE_VGAP + CHALLENGE_ENTRY_H)));
		}
		widgAddButton(psRequestScreen, &sButInit);
	}

	// fill slots.
	slotCount = 0;

	sstrcpy(sPath, sSearchPath);
	sstrcat(sPath, "/*.ini");

	debug(LOG_SAVE, "Searching \"%s\" for challenges", sPath);

	// add challenges to buttons
	files = PHYSFS_enumerateFiles(sSearchPath);
	for (i = files; *i != NULL; ++i)
	{
		W_BUTTON *button;
		QString name, map, difficulty, highscore, description;
		bool victory;
		int seconds;

		// See if this filename contains the extension we're looking for
		if (!strstr(*i, ".ini"))
		{
			// If it doesn't, move on to the next filename
			continue;
		}

		/* First grab any high score associated with this challenge */
		sstrcpy(sPath, *i);
		sPath[strlen(sPath) - 4] = '\0';	// remove .ini
		highscore = "no score";
		WzConfig scores(CHALLENGE_SCORES, WzConfig::ReadOnly);
		scores.beginGroup(sPath);
		name = scores.value("Player", "NO NAME").toString();
		victory = scores.value("Victory", false).toBool();
		seconds = scores.value("Seconds", -1).toInt();
		if (seconds > 0)
		{
			QTime format = QTime(0, 0, 0).addSecs(seconds);
			highscore = format.toString(Qt::TextDate) + " by " + name + " (" + QString(victory ? "Victory" : "Survived") + ")";
		}
		ssprintf(sPath, "%s/%s", sSearchPath, *i);
		WzConfig challenge(sPath);
		if (challenge.status() != QSettings::NoError)
		{
			debug(LOG_ERROR, "failure to open %s", sPath);
		}
		challenge.beginGroup("challenge");
		name = challenge.value("Name", "BAD NAME").toString();
		map = challenge.value("Map", "BAD MAP").toString();
		difficulty = challenge.value("Difficulty", "BAD DIFFICULTY").toString();
		description = map + ", " + difficulty + ", " + highscore + ". " + challenge.value("Description", "").toString();

		button = (W_BUTTON*)widgGetFromID(psRequestScreen, CHALLENGE_ENTRY_START + slotCount);

		debug(LOG_SAVE, "We found [%s]", *i);

		/* Set the button-text */
		sstrcpy(sSlotCaps[slotCount], name.toUtf8().constData());		// store it!
		sstrcpy(sSlotTips[slotCount], description.toUtf8().constData());	// store it, too!
		sstrcpy(sSlotFile[slotCount], sPath);					// store filename

		/* Add button */
		button->pTip = sSlotTips[slotCount];
		button->pText = sSlotCaps[slotCount];
		button->pUserData = (void *)sSlotFile[slotCount];
		slotCount++;		// go to next button...
		if (slotCount == totalslots)
		{
			break;
		}
	}
	PHYSFS_freeList(files);

	challengesUp = true;

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
bool closeChallenges()
{
	delete psRequestScreen;
	psRequestScreen = NULL;
	// need to "eat" up the return key so it don't pass back to game.
	inputLoseFocus();
	challengesUp = false;
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Returns true if cancel pressed or a valid game slot was selected.
// if when returning true strlen(sRequestResult) != 0 then a valid game
// slot was selected otherwise cancel was selected..
bool runChallenges(void)
{
	WidgetTriggers const &triggers = widgRunScreen(psRequestScreen);
	for (WidgetTriggers::const_iterator trigger = triggers.begin(); trigger != triggers.end(); ++trigger)
	{
		unsigned id = trigger->widget->id;

		sstrcpy(sRequestResult, "");  // set returned filename to null;

		// cancel this operation...
		if (id == CHALLENGE_CANCEL || CancelPressed())
		{
			goto failure;
		}

		// clicked a load entry
		if (id >= CHALLENGE_ENTRY_START  &&  id <= CHALLENGE_ENTRY_END)
		{
			if (!((W_BUTTON *)widgGetFromID(psRequestScreen, id))->pText.isEmpty())
			{
				sstrcpy(sRequestResult, (const char *)((W_BUTTON *)widgGetFromID(psRequestScreen, id))->pUserData);
			}
			else
			{
				goto failure;  // clicked on an empty box
			}
			goto success;
		}
	}

	return false;

// failed and/or cancelled..
failure:
	closeChallenges();
	challengeActive = false;
	return false;

// success on load.
success:
	closeChallenges();
	challengeActive = true;
	ingame.bHostSetup = true;
	changeTitleMode(MULTIOPTION);
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// should be done when drawing the other widgets.
bool displayChallenges()
{
	widgDisplayScreen(psRequestScreen);	// display widgets.
	return true;
}

