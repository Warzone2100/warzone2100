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
 * multilimit.c
 *
 * interface for setting limits to the game, bots, structlimits etc...
 */
#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/strres.h"
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
#include "lib/ivis_common/bitimage.h"	// GFX incs
#include "lib/ivis_common/textdraw.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_common/piestate.h"
#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "multirecv.h"
#include "multiint.h"
#include "multilimit.h"
#include "lib/ivis_common/piemode.h"
#include "lib/script/script.h"

// ////////////////////////////////////////////////////////////////////////////
// defines
#define	IDLIMITS				22000
#define IDLIMITS_RETURN			(IDLIMITS+1)
#define IDLIMITS_OK				(IDLIMITS+2)
#define IDLIMITS_TABS			(IDLIMITS+3)
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
#define BUTPERFORM				8

// ////////////////////////////////////////////////////////////////////////////
// protos.

static void displayStructureBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

// ////////////////////////////////////////////////////////////////////////////

static BOOL useStruct(UDWORD count,UDWORD i)
{
	ASSERT(i < numStructureStats, "Bad structure for structure limit: %d", (int)i);

	if(count >= (4*BUTPERFORM))
	{
		return FALSE;
	}

	// now see if we loaded that stat..
	if(asStructLimits[0][i].globalLimit ==LOTS_OF)
	{
		return FALSE;
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
BOOL startLimitScreen(void)
{
	W_FORMINIT		sButInit;
	W_FORMINIT		sFormInit;
	UDWORD			numButtons = 0;
	UDWORD			i;

	addBackdrop();//background

	// load stats...
	if(!bForceEditorLoaded)
	{
		initLoadingScreen( TRUE );//changed by jeremy mar8

		if (!resLoad("wrf/piestats.wrf", 501))
		{
			return FALSE;
		}

		if (!resLoad("wrf/forcedit2.wrf", 502))
		{
			return FALSE;
		}

		bForceEditorLoaded = TRUE;
		closeLoadingScreen();
	}

	addSideText(FRONTEND_SIDETEXT1,LIMITSX-2,LIMITSY,"LIMITS");	// draw sidetext...

	memset(&sFormInit, 0, sizeof(W_FORMINIT));				// draw blue form...
	sFormInit.formID	= FRONTEND_BACKDROP;
	sFormInit.id		= IDLIMITS;
	sFormInit.style		= WFORM_PLAIN;
	sFormInit.x			= LIMITSX;
	sFormInit.y			= LIMITSY;
	sFormInit.width		= LIMITSW;
	sFormInit.height	= LIMITSH;
	sFormInit.pDisplay	= intDisplayPlainForm;
	widgAddForm(psWScreen, &sFormInit);

	// return button.
	addMultiBut(psWScreen,IDLIMITS,IDLIMITS_RETURN,
					LIMITS_OKX-40,LIMITS_OKY,
					iV_GetImageWidth(FrontImages,IMAGE_RETURN),
					iV_GetImageHeight(FrontImages,IMAGE_RETURN),
					_("Return To Previous Screen"),IMAGE_NO,IMAGE_NO,TRUE);


	// ok button
	addMultiBut(psWScreen,IDLIMITS,IDLIMITS_OK,
					LIMITS_OKX,LIMITS_OKY,
					iV_GetImageWidth(FrontImages,IMAGE_BIGOK),
					iV_GetImageHeight(FrontImages,IMAGE_BIGOK),
					_("Accept Settings"),IMAGE_OK,IMAGE_OK,TRUE);

	// Count the number of minor tabs needed
	numButtons = 0;

	for(i=0;i<numStructureStats;i++ )
	{
		if(useStruct(numButtons,i))
		{
			numButtons++;
		}
	}

	if(numButtons >(4*BUTPERFORM)) numButtons =(4*BUTPERFORM);

	// add tab form..
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = IDLIMITS;
	sFormInit.id = IDLIMITS_TABS;
	sFormInit.style = WFORM_TABBED;
	sFormInit.x = 50;
	sFormInit.y = 10;
	sFormInit.width = LIMITSW - 100;
	sFormInit.height =LIMITSH - 4;
	sFormInit.numMajor = numForms(numButtons, BUTPERFORM);
	sFormInit.majorPos = WFORM_TABTOP;
	sFormInit.minorPos = WFORM_TABNONE;
	sFormInit.majorSize = OBJ_TABWIDTH+3; //!!
	sFormInit.majorOffset = OBJ_TABOFFSET;
	sFormInit.tabVertOffset = (OBJ_TABHEIGHT/2);			//(DES_TAB_HEIGHT/2)+2;
	sFormInit.tabMajorThickness = OBJ_TABHEIGHT;
	sFormInit.pUserData = &StandardTab;
	sFormInit.pTabDisplay = intDisplayTab;

	// TABFIXME --unsure if needs fixing yet.
	for (i=0; i< sFormInit.numMajor; i++)
	{
		sFormInit.aNumMinors[i] = 1;
	}
	widgAddForm(psWScreen, &sFormInit);

	//Put the buttons on it
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID   = IDLIMITS_TABS;//IDLIMITS;
	sButInit.style	  = WFORM_PLAIN;
	sButInit.width    = BARWIDTH;
	sButInit.height	  = BARHEIGHT;
	sButInit.pDisplay = displayStructureBar;
	sButInit.x		  = 2;
	sButInit.y		  = 5;
	sButInit.id	 	  = IDLIMITS_ENTRIES_START;

	numButtons =0;
	for(i=0; i<numStructureStats ;i++)
	{
		if(useStruct(numButtons,i))
		{
			numButtons++;
			sButInit.UserData= i;

			widgAddForm(psWScreen, &sButInit);
			sButInit.id	++;

			addFESlider(sButInit.id,sButInit.id-1, 290,11,
						asStructLimits[0][i].globalLimit,
						asStructLimits[0][i].limit, 0);
			sButInit.id	++;

			if (sButInit.y + BARHEIGHT + 2 > (BUTPERFORM*(BARHEIGHT+2) - 4) )
			{
				sButInit.y = 5;
				sButInit.majorID += 1;
			}
			else
			{
				sButInit.y +=  BARHEIGHT +5;
			}
		}
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////

void runLimitScreen(void)
{
	UDWORD id,statid;

	frontendMultiMessages();							// network stuff.

	id = widgRunScreen(psWScreen);						// Run the current set of widgets

	// sliders
	if((id > IDLIMITS_ENTRIES_START)  && (id< IDLIMITS_ENTRIES_END))
	{
		statid = widgGetFromID(psWScreen,id-1)->UserData ;
		if(statid)
		{
			asStructLimits[0][statid].limit = (UBYTE) ((W_SLIDER*)(widgGetFromID(psWScreen,id)))->pos;
		}
	}
	else
	{
		// icons that are always about.
		switch(id)
		{
		case IDLIMITS_RETURN:
			// reset the sliders..
			eventReset();
			resReleaseBlockData(501);
			resReleaseBlockData(502);
			bForceEditorLoaded = FALSE;
			changeTitleMode(MULTIOPTION);

			// make some noize.
			if(!ingame.localOptionsReceived)
			{
				addConsoleMessage("Limits Reset To Default Values",DEFAULT_JUSTIFY);
			}
			else
			{
				sendTextMessage("Limits Reset To Default Values",TRUE);
			}

			break;
		case IDLIMITS_OK:
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
void createLimitSet(void)
{
	UDWORD			i, numchanges = 0, bufSize;
	MULTISTRUCTLIMITS	*pEntry;

	// Free the old set if required
	if (ingame.numStructureLimits)
	{
		ingame.numStructureLimits = 0;
		free(ingame.pStructureLimits);
		ingame.pStructureLimits = NULL;
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
	bufSize = numStructureStats * sizeof(MULTISTRUCTLIMITS);
	pEntry = malloc(bufSize);
	memset(pEntry, 255, bufSize);

	// Prepare chunk
	ASSERT(numStructureStats < UBYTE_MAX, "Too many structure stats");
	for (i = 0; i < numStructureStats; i++)
	{
		if (asStructLimits[0][i].limit != LOTS_OF)
		{
			pEntry[i].id	= i;
			pEntry[i].limit	= asStructLimits[0][i].limit;
		}
	}

	ingame.numStructureLimits	= numchanges;
	ingame.pStructureLimits		= pEntry;

	sendOptions(0, 0);
}

// ////////////////////////////////////////////////////////////////////////////
void applyLimitSet(void)
{
	MULTISTRUCTLIMITS *pEntry = ingame.pStructureLimits;
	int i, j;

	if (ingame.numStructureLimits == 0)
	{
		return;
	}

	// Get the limits and decode
	for (i = 0; i < numStructureStats; i++)
 	{
		UBYTE id = pEntry[i].id;
		
		// So long as the ID is valid
		if (id < numStructureStats)
		{
			for (j = 0; j < MAX_PLAYERS; j++)
			{
				asStructLimits[j][id].limit = pEntry[i].limit;
			}
		}
	}

	// Free
	if (ingame.numStructureLimits)
	{
		free(ingame.pStructureLimits);
		ingame.numStructureLimits = 0;
		ingame.pStructureLimits = NULL;
	}
}

// ////////////////////////////////////////////////////////////////////////////

static void displayStructureBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	UDWORD	w = psWidget->width;
	UDWORD	h = psWidget->height;
	STRUCTURE_STATS	*stat = asStructureStats + psWidget->UserData;
	Vector3i Rotation, Position;
	char	str[3];

	UDWORD scale,Radius;

	drawBlueBox(x,y,w,h);

	// draw image
	pie_SetGeometricOffset( x+35 ,y+(psWidget->height/2)+9);
	Rotation.x = -15;
	Rotation.y = ((gameTime2/45)%360) ; //45
	Rotation.z = 0;
	Position.x = 0;
	Position.y = 0;
	Position.z = BUTTON_DEPTH*2;//getStructureStatSize(stat)  * 38 * OBJECT_RADIUS;

	Radius = getStructureStatSize(stat);
	if(Radius <= 128) {
		scale = SMALL_STRUCT_SCALE;
	} else if(Radius <= 256) {
		scale = MED_STRUCT_SCALE;
	} else {
		scale = LARGE_STRUCT_SCALE;
	}

	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
	displayStructureStatButton(stat ,0,	 &Rotation,&Position,TRUE, scale);
	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);

	// draw name
	iV_SetFont(font_regular);											// font
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	iV_DrawText(getName(stat->pName), x+80, y+(psWidget->height/2)+3);

	// draw limit
	sprintf(str,"%d",((W_SLIDER*)(widgGetFromID(psWScreen,psWidget->id+1)))->pos);
	iV_DrawText(str, x+270, y+(psWidget->height/2)+3);

	return;
}
