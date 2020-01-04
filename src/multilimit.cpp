/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include "challenge.h"
#include "objmem.h"
#include "titleui/titleui.h"

// ////////////////////////////////////////////////////////////////////////////
// defines
#define	IDLIMITS				22000
#define IDLIMITS_RETURN			(IDLIMITS+1)
#define IDLIMITS_OK				(IDLIMITS+2)
#define IDLIMITS_FORCE				(IDLIMITS+3)
#define IDLIMITS_FORCEOFF			(IDLIMITS+4)
#define IDLIMITS_ENTRIES_START	(IDLIMITS+5)
#define IDLIMITS_ENTRIES_END	(IDLIMITS+99)

#define LIMITSX					25
#define LIMITSY					30
#define LIMITSW					580
#define LIMITSH					430

#define LIMITS_OKX				(LIMITSW-90)
#define LIMITS_OKY				(LIMITSH-42)

#define LIMITS_FORCEOFFX			(LIMITSX+25)
#define LIMITS_FORCEX				(LIMITSX+65)

#define BARWIDTH				480
#define BARHEIGHT				40

// ////////////////////////////////////////////////////////////////////////////
// protos.

static void displayStructureBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

struct DisplayStructureBarCache {
	WzText wzNameText;
	WzText wzLimitText;
};

// ////////////////////////////////////////////////////////////////////////////
static inline void freeLimitSet()
{
	// Free the old set if required
	ingame.structureLimits.clear();
}

// ////////////////////////////////////////////////////////////////////////////
WzMultiLimitTitleUI::WzMultiLimitTitleUI(std::shared_ptr<WzMultiplayerOptionsTitleUI> parent) : parent(parent)
{

}

void WzMultiLimitTitleUI::start()
{
	addBackdrop();//background

	// load stats...
	if (!bLimiterLoaded)
	{
		initLoadingScreen(true);

		if (!resLoad("wrf/limiter_data.wrf", 503))
		{
			debug(LOG_INFO, "Unable to load limiter_data during WzMultiLimitTitleUI start; returning...");
			changeTitleUI(parent);
			closeLoadingScreen();
			return;
		}

		bLimiterLoaded = true;

		closeLoadingScreen();
	}

	if (challengeActive)
	{
		resetLimits();
		// turn off the sliders
		sliderEnableDrag(false);
	}
	else
	{
		//enable the sliders
		sliderEnableDrag(true);
	}

	// TRANSLATORS: Sidetext of structure limits screen
	addSideText(FRONTEND_SIDETEXT1, LIMITSX - 2, LIMITSY, _("LIMITS"));

	IntFormAnimated *limitsForm = new IntFormAnimated(widgGetFromID(psWScreen, FRONTEND_BACKDROP), false);
	limitsForm->id = IDLIMITS;
	limitsForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(LIMITSX, LIMITSY, LIMITSW, LIMITSH);
	}));

	// force limits off button
	addMultiBut(psWScreen, IDLIMITS, IDLIMITS_FORCEOFF,
	            LIMITS_FORCEOFFX, LIMITS_OKY,
	            iV_GetImageWidth(FrontImages, IMAGE_DARK_UNLOCKED),
	            iV_GetImageHeight(FrontImages, IMAGE_DARK_UNLOCKED),
	            _("Map Can Exceed Limits"), IMAGE_DARK_UNLOCKED, IMAGE_DARK_UNLOCKED, true);
	// force limits button
	addMultiBut(psWScreen, IDLIMITS, IDLIMITS_FORCE,
	            LIMITS_FORCEX, LIMITS_OKY,
	            iV_GetImageWidth(FrontImages, IMAGE_DARK_LOCKED),
	            iV_GetImageHeight(FrontImages, IMAGE_DARK_LOCKED),
	            _("Force Limits at Start"), IMAGE_DARK_LOCKED, IMAGE_DARK_LOCKED, true);
	if (challengeActive)
	{
		widgSetButtonState(psWScreen, IDLIMITS_FORCE, WBUT_DISABLE);
		widgSetButtonState(psWScreen, IDLIMITS_FORCEOFF, WBUT_DISABLE);
	}
	else if (ingame.flags & MPFLAGS_FORCELIMITS)
	{
		widgSetButtonState(psWScreen, IDLIMITS_FORCE, WBUT_DISABLE);
		widgSetButtonState(psWScreen, IDLIMITS_FORCEOFF, 0);
	}
	else
	{
		widgSetButtonState(psWScreen, IDLIMITS_FORCE, 0);
		widgSetButtonState(psWScreen, IDLIMITS_FORCEOFF, WBUT_DISABLE);
	}

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
	limitsList->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		IntListTabWidget *limitsList = static_cast<IntListTabWidget *>(psWidget);
		assert(limitsList != nullptr);
		limitsList->setChildSize(BARWIDTH, BARHEIGHT);
		limitsList->setChildSpacing(5, 5);
		limitsList->setGeometry(50, 10, BARWIDTH, 370);
	}));

	//Put the buttons on it
	int limitsButtonId = IDLIMITS_ENTRIES_START;

	for (unsigned i = 0; i < numStructureStats; ++i)
	{
		if (asStructureStats[i].base.limit != LOTS_OF)
		{
			W_FORM *button = new W_FORM(limitsList);
			button->id = limitsButtonId;
			button->displayFunction = displayStructureBar;
			button->UserData = i;
			button->pUserData = new DisplayStructureBarCache();
			button->setOnDelete([](WIDGET *psWidget) {
				assert(psWidget->pUserData != nullptr);
				delete static_cast<DisplayStructureBarCache *>(psWidget->pUserData);
				psWidget->pUserData = nullptr;
			});
			limitsList->addWidgetToLayout(button);
			++limitsButtonId;

			addFESlider(limitsButtonId, limitsButtonId - 1, 290, 11,
			            asStructureStats[i].maxLimit,
			            asStructureStats[i].upgrade[0].limit);
			++limitsButtonId;
		}
	}
}

TITLECODE WzMultiLimitTitleUI::run()
{
	parent->frontendMultiMessages(false);							// network stuff.

	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	// sliders
	if ((id > IDLIMITS_ENTRIES_START)  && (id < IDLIMITS_ENTRIES_END))
	{
		unsigned statid = widgGetFromID(psWScreen, id - 1)->UserData;
		if (statid)
		{
			asStructureStats[statid].upgrade[0].limit = (UBYTE)((W_SLIDER *)(widgGetFromID(psWScreen, id)))->pos;
		}
	}
	else
	{
		// icons that are always about.
		switch (id)
		{
		case IDLIMITS_FORCE:
			widgSetButtonState(psWScreen, IDLIMITS_FORCE, WBUT_DISABLE);
			widgSetButtonState(psWScreen, IDLIMITS_FORCEOFF, 0);
			break;
		case IDLIMITS_FORCEOFF:
			widgSetButtonState(psWScreen, IDLIMITS_FORCE, 0);
			widgSetButtonState(psWScreen, IDLIMITS_FORCEOFF, WBUT_DISABLE);
			break;
		case IDLIMITS_RETURN:
			// reset the sliders..
			resetLimits();
			// free limiter structure
			freeLimitSet();
			if (widgGetButtonState(psWScreen, IDLIMITS_FORCE))
			{
				ingame.flags &= ~MPFLAGS_FORCELIMITS;
			}
			//inform others
			if (NetPlay.isHost)
			{
				sendOptions();
			}

			changeTitleUI(parent);

			// make some noize.
			if (!ingame.localOptionsReceived)
			{
				addConsoleMessage(_("Limits reset to default values"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
			}
			else
			{
				sendTextMessage("Limits Reset To Default Values", true);
			}

			resetReadyStatus(false);

			break;
		case IDLIMITS_OK:
			if (!challengeActive && widgGetButtonState(psWScreen, IDLIMITS_FORCE))
			{
				ingame.flags |= MPFLAGS_FORCELIMITS;
			}
			else
			{
				ingame.flags &= ~MPFLAGS_FORCELIMITS;
			}
			resetReadyStatus(false);
			createLimitSet();
			changeTitleUI(parent);
			break;
		default:
			break;
		}
	}

	widgDisplayScreen(psWScreen);						// show the widgets currently running
	return TITLECODE_CONTINUE;
}

// ////////////////////////////////////////////////////////////////////////////
void createLimitSet()
{
	UDWORD			numchanges = 0;

	debug(LOG_NET, "LimitSet created");
	// free old limiter structure
	freeLimitSet();

	// don't bother creating if a challenge mode is active
	if (challengeActive)
	{
		return;
	}

	// Count the number of changes
	for (unsigned i = 0; i < numStructureStats; i++)
	{
		// If the limit differs from the default
		if (asStructureStats[i].upgrade[0].limit != LOTS_OF)
		{
			numchanges++;
		}
	}

	if (numchanges > 0)
	{
		// Allocate some memory for the changes
		ingame.structureLimits.reserve(numchanges);

		// Prepare chunk
		for (unsigned i = 0; i < numStructureStats; i++)
		{
			if (asStructureStats[i].upgrade[0].limit != LOTS_OF)
			{
				ASSERT_OR_RETURN(, ingame.structureLimits.size() < numchanges, "Bad number of changed limits");
				ingame.structureLimits.push_back(MULTISTRUCTLIMITS { i, asStructureStats[i].upgrade[0].limit });
			}
		}
	}

	if (NetPlay.isHost)
	{
		sendOptions();
	}
}

// ////////////////////////////////////////////////////////////////////////////
bool applyLimitSet()
{
	if (ingame.structureLimits.empty())
	{
		return false;
	}

	// Get the limits and decode
	UBYTE flags = ingame.flags & MPFLAGS_FORCELIMITS;
	for (const auto& structLimit : ingame.structureLimits)
	{
		int id = structLimit.id;

		// So long as the ID is valid
		if (id < numStructureStats)
		{
			for (int player = 0; player < MAX_PLAYERS; player++)
			{
				asStructureStats[id].upgrade[player].limit = structLimit.limit;

				if (ingame.flags & MPFLAGS_FORCELIMITS)
				{
					while (asStructureStats[id].curCount[player] > asStructureStats[id].upgrade[player].limit)
					{
						for (STRUCTURE *psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
						{
							if (psStruct->pStructureType->type == asStructureStats[id].type)
							{
								removeStruct(psStruct, true);
								break;
							}
						}

					}
				}
			}
			if (asStructureStats[id].type == REF_VTOL_FACTORY && structLimit.limit == 0)
			{
				flags |= MPFLAGS_NO_VTOLS;
			}
			if (asStructureStats[id].type == REF_CYBORG_FACTORY && structLimit.limit == 0)
			{
				flags |= MPFLAGS_NO_CYBORGS;
			}
			if (asStructureStats[id].type == REF_LASSAT && structLimit.limit == 0)
			{
				flags |= MPFLAGS_NO_LASSAT;
			}
			if (asStructureStats[id].type == REF_SAT_UPLINK && structLimit.limit == 0)
			{
				flags |= MPFLAGS_NO_UPLINK;
			}
		}
	}

	// If structure limits are enforced and VTOLs are disabled, disable also AA structures
	if ((flags & (MPFLAGS_FORCELIMITS | MPFLAGS_NO_VTOLS)) == (MPFLAGS_FORCELIMITS | MPFLAGS_NO_VTOLS))
	{
		for (int i = 0; i < numStructureStats; i++)
		{
			if (asStructureStats[i].numWeaps > 0 && asStructureStats[i].psWeapStat[0]->surfaceToAir == SHOOT_IN_AIR)
			{
				for (int player = 0; player < MAX_PLAYERS; player++)
				{
					asStructureStats[i].upgrade[player].limit = 0;
				}
			}
		}
	}
	// Disable related research according to disabledWhen in research.json
	if (flags & MPFLAGS_FORCELIMITS)
	{
		RecursivelyDisableResearchByFlags(flags & ~(MPFLAGS_FORCELIMITS | MPFLAGS_NO_UPLINK));
	}
	/*
	  Special rules for LasSat and Uplink which do not produce units and have no upgrades:
	  - research can be disabled even if limits are not enforced
	  - because of the research path, only disable Uplink if also LasSat is disabled
	*/
	if (flags & MPFLAGS_NO_LASSAT)
	{
		RecursivelyDisableResearchByFlags(MPFLAGS_NO_LASSAT);
	}
	if ((flags & (MPFLAGS_NO_LASSAT | MPFLAGS_NO_UPLINK)) == (MPFLAGS_NO_LASSAT | MPFLAGS_NO_UPLINK))
	{
		RecursivelyDisableResearchByFlags(MPFLAGS_NO_UPLINK);
	}
	freeLimitSet();
	return true;
}

// ////////////////////////////////////////////////////////////////////////////

static void displayStructureBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using displayStructureBar must have its pUserData initialized to a (DisplayStructureBarCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayStructureBarCache& cache = *static_cast<DisplayStructureBarCache *>(psWidget->pUserData);

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

	displayStructureStatButton(stat, &rotation, &position, scale);

	// draw name
	cache.wzNameText.setText(_(getName(stat)), font_regular);
	cache.wzNameText.render(x + 80, y + psWidget->height() / 2 + 3, WZCOL_TEXT_BRIGHT);

	// draw limit
	ssprintf(str, "%d", ((W_SLIDER *)widgGetFromID(psWScreen, psWidget->id + 1))->pos);
	cache.wzLimitText.setText(str, font_regular);
	cache.wzLimitText.render(x + 270, y + psWidget->height() / 2 + 3, WZCOL_TEXT_BRIGHT);

	return;
}

void resetLimits(void)
{
	for (unsigned i = 0; i < numStructureStats; ++i)
	{
		asStructureStats[i].upgrade[0].limit = asStructureStats[i].base.limit;
	}
}
