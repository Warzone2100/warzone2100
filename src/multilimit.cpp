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
 * multilimit.c
 *
 * interface for setting limits to the game, bots, structlimits etc...
 */
#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/strres.h"
#include "lib/widget/slider.h"
#include "lib/widget/widget.h"
#include "hci.h"
#include "intimage.h"
#include "intdisplay.h"
#include "init.h"		// for gameheap
#include "frend.h"
#include "stats.h"
#include "frontend.h"
#include "component.h"
#include "loadsave.h"
#include "wrappers.h"	// for loading screen
#include "lib/gamelib/gtime.h"
#include "console.h"
#include "lib/ivis_opengl/bitimage.h"	// GFX incs
#include "lib/ivis_opengl/textdraw.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "multirecv.h"
#include "multiint.h"
#include "multilimit.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/script/script.h"
#include "challenge.h"

// ////////////////////////////////////////////////////////////////////////////
// defines
#define	IDLIMITS				22000
#define IDLIMITS_RETURN			(IDLIMITS+1)
#define IDLIMITS_OK				(IDLIMITS+2)
#define IDLIMITS_ENTRIES_START	(IDLIMITS+4)
#define IDLIMITS_ENTRIES_END	(IDLIMITS+99)

#define LIMITSX					25
#define LIMITSY					30
#define LIMITSW					580
#define LIMITSH					430

#define LIMITS_OKX				(LIMITSW-90)
#define LIMITS_OKY				(LIMITSH-42)

#define BARWIDTH				480
#define BARHEIGHT				40

// ////////////////////////////////////////////////////////////////////////////
// protos.

static void displayStructureBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

// ////////////////////////////////////////////////////////////////////////////
static inline void freeLimitSet()
{
	// Free the old set if required
	if (ingame.numStructureLimits)
	{
		free(ingame.pStructureLimits);
		ingame.numStructureLimits = 0;
		ingame.pStructureLimits = nullptr;
	}
}

// ////////////////////////////////////////////////////////////////////////////
bool startLimitScreen()
{
	addBackdrop();//background

	// load stats...
	if (!bLimiterLoaded)
	{
		initLoadingScreen(true);

		if (!resLoad("wrf/limiter_data.wrf", 503))
		{
			return false;
		}

		bLimiterLoaded = true;

		closeLoadingScreen();
	}

	if (challengeActive)
	{
		// reset the sliders..
		// it's a HACK since the actual limiter structure was cleared in the startMultiOptions function
		for (unsigned i = 0; i < numStructureStats; ++i)
		{
			asStructLimits[0][i].limit = asStructLimits[0][i].globalLimit;
		}

		// turn off the sliders
		sliderEnableDrag(false);
	}
	else
	{
		//enable the sliders
		sliderEnableDrag(true);
	}

	addSideText(FRONTEND_SIDETEXT1, LIMITSX - 2, LIMITSY, "LIMITS");	// draw sidetext...

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	IntFormAnimated *limitsForm = new IntFormAnimated(parent, false);
	limitsForm->id = IDLIMITS;
	limitsForm->setGeometry(LIMITSX, LIMITSY, LIMITSW, LIMITSH);

	// return button.
	addMultiBut(psWScreen, IDLIMITS, IDLIMITS_RETURN,
	            LIMITS_OKX - 40, LIMITS_OKY,
	            iV_GetImageWidth(FrontImages, IMAGE_NO),
	            iV_GetImageHeight(FrontImages, IMAGE_NO),
	            _("Apply Defaults and Return To Previous Screen"), IMAGE_NO, IMAGE_NO, true);

	// ok button
	addMultiBut(psWScreen, IDLIMITS, IDLIMITS_OK,
	            LIMITS_OKX, LIMITS_OKY,
	            iV_GetImageWidth(FrontImages, IMAGE_OK),
	            iV_GetImageHeight(FrontImages, IMAGE_OK),
	            _("Accept Settings"), IMAGE_OK, IMAGE_OK, true);

	// add tab form..
	IntListTabWidget *limitsList = new IntListTabWidget(limitsForm);
	limitsList->setChildSize(BARWIDTH, BARHEIGHT);
	limitsList->setChildSpacing(5, 5);
	limitsList->setGeometry(50, 10, BARWIDTH, 370);

	//Put the buttons on it
	int limitsButtonId = IDLIMITS_ENTRIES_START;

	for (unsigned i = 0; i < numStructureStats; ++i)
	{
		if (asStructLimits[0][i].globalLimit != LOTS_OF)
		{
			W_FORM *button = new W_FORM(limitsList);
			button->id = limitsButtonId;
			button->displayFunction = displayStructureBar;
			button->UserData = i;
			limitsList->addWidgetToLayout(button);
			++limitsButtonId;

			addFESlider(limitsButtonId, limitsButtonId - 1, 290, 11,
			            asStructLimits[0][i].globalLimit,
			            asStructLimits[0][i].limit);
			++limitsButtonId;
		}
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////

void runLimitScreen()
{
	frontendMultiMessages();							// network stuff.

	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	// sliders
	if ((id > IDLIMITS_ENTRIES_START)  && (id < IDLIMITS_ENTRIES_END))
	{
		unsigned statid = widgGetFromID(psWScreen, id - 1)->UserData;
		if (statid)
		{
			asStructLimits[0][statid].limit = (UBYTE)((W_SLIDER *)(widgGetFromID(psWScreen, id)))->pos;
		}
	}
	else
	{
		// icons that are always about.
		switch (id)
		{
		case IDLIMITS_RETURN:
			// reset the sliders..
			for (unsigned i = 0; i < numStructureStats; ++i)
			{
				asStructLimits[0][i].limit = asStructLimits[0][i].globalLimit;
			}
			// free limiter structure
			freeLimitSet();
			//inform others
			if (bHosted)
			{
				sendOptions();
			}

			eventReset();
			changeTitleMode(MULTIOPTION);

			// make some noize.
			if (!ingame.localOptionsReceived)
			{
				addConsoleMessage(_("Limits reset to default values"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
			}
			else
			{
				sendTextMessage("Limits Reset To Default Values", true);
			}

			break;
		case IDLIMITS_OK:
			resetReadyStatus(false);
			createLimitSet();
			changeTitleMode(MULTIOPTION);
			break;
		default:
			break;
		}
	}

	widgDisplayScreen(psWScreen);						// show the widgets currently running
}

// ////////////////////////////////////////////////////////////////////////////
void createLimitSet()
{
	UDWORD			i, numchanges = 0, bufSize, idx = 0;
	MULTISTRUCTLIMITS	*pEntry;

	debug(LOG_NET, "LimitSet created");
	// free old limiter structure
	freeLimitSet();

	// don't bother creating if a challenge mode is active
	if (challengeActive)
	{
		return;
	}

	// Count the number of changes
	for (i = 0; i < numStructureStats; i++)
	{
		// If the limit differs from the default
		if (asStructLimits[0][i].limit != LOTS_OF)
		{
			numchanges++;
		}
	}

	// Allocate some memory for the changes
	bufSize = numchanges * sizeof(MULTISTRUCTLIMITS);
	pEntry = (MULTISTRUCTLIMITS *)malloc(bufSize);
	memset(pEntry, 255, bufSize);

	// Prepare chunk
	for (i = 0; i < numStructureStats; i++)
	{
		if (asStructLimits[0][i].limit != LOTS_OF)
		{
			ASSERT_OR_RETURN(, idx < numchanges, "Bad number of changed limits");
			pEntry[idx].id		= i;
			pEntry[idx].limit	= asStructLimits[0][i].limit;
			idx++;
		}
	}

	ingame.numStructureLimits	= numchanges;
	ingame.pStructureLimits		= pEntry;

	if (bHosted)
	{
		sendOptions();
	}
}

// ////////////////////////////////////////////////////////////////////////////
void applyLimitSet()
{
	MULTISTRUCTLIMITS *pEntry = ingame.pStructureLimits;

	if (ingame.numStructureLimits == 0)
	{
		return;
	}

	// Get the limits and decode
	for (int i = 0; i < ingame.numStructureLimits; ++i)
	{
		int id = pEntry[i].id;

		// So long as the ID is valid
		if (id < numStructureStats)
		{
			for (auto &asStructLimit : asStructLimits)
			{
				asStructLimit[id].limit = pEntry[i].limit;
			}
		}
	}

	freeLimitSet();
}

// ////////////////////////////////////////////////////////////////////////////

static void displayStructureBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	int w = psWidget->width();
	int h = psWidget->height();
	STRUCTURE_STATS	*stat = asStructureStats + psWidget->UserData;
	Position position;
	Vector3i rotation;
	char str[20];

	UDWORD scale, Radius;

	drawBlueBox(x, y, w, h);

	// draw image
	pie_SetGeometricOffset(x + 35, y + psWidget->height() / 2 + 9);
	rotation.x = -15;
	rotation.y = ((realTime / 45) % 360) ; //45
	rotation.z = 0;
	position.x = 0;
	position.y = 0;
	position.z = BUTTON_DEPTH * 2; //getStructureStatSize(stat)  * 38 * OBJECT_RADIUS;

	Radius = getStructureStatSizeMax(stat);
	if (Radius <= 128)
	{
		scale = SMALL_STRUCT_SCALE;
	}
	else if (Radius <= 256)
	{
		scale = MED_STRUCT_SCALE;
	}
	else
	{
		scale = LARGE_STRUCT_SCALE;
	}

	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
	displayStructureStatButton(stat, &rotation, &position, scale);
	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);

	// draw name
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	iV_DrawText(_(getName(stat)), x + 80, y + psWidget->height() / 2 + 3, font_regular);

	// draw limit
	ssprintf(str, "%d", ((W_SLIDER *)widgGetFromID(psWScreen, psWidget->id + 1))->pos);
	iV_DrawText(str, x + 270, y + psWidget->height() / 2 + 3, font_regular);

	return;
}
