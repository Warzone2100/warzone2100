/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
 * IntDisplay.c
 *
 * Callback and display functions for interface.
 *
 */
#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/math_ext.h"

/* Includes direct access to render library */
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piepalette.h"

#include "lib/ivis_opengl/piemode.h"			// ffs
#include "lib/ivis_opengl/pieclip.h"			// ffs
#include "lib/ivis_opengl/pieblitfunc.h"

// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/rendmode.h"
#include "lib/ivis_opengl/piematrix.h"

#include "lib/framework/input.h"
#include "lib/widget/slider.h"
#include "lib/widget/editbox.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/bar.h"

#include "lib/gamelib/gtime.h"
#include "lib/sound/audio.h"

#include "intdisplay.h"

#include "objects.h"
#include "loop.h"
#include "map.h"
#include "radar.h"

#include "display3d.h"
#include "edit3d.h"
#include "structure.h"
#include "research.h"
#include "function.h"
#include "hci.h"
#include "stats.h"
#include "game.h"
#include "power.h"
#include "order.h"
#include "frontend.h"
#include "intimage.h"
#include "component.h"
#include "console.h"
#include "cmddroid.h"
#include "group.h"
#include "transporter.h"
#include "mission.h"

#include "multiplay.h"

// Is a clickable form widget hilited, either because the cursor is over it or it is flashing.
//
#define formIsHilite(p) 	(((W_CLICKFORM*)p)->state & WCLICK_HILITE)
#define formIsFlashing(p)	(((W_CLICKFORM*)p)->state & WCLICK_FLASHON)

// Is a button widget hilited, either because the cursor is over it or it is flashing.
//
#define buttonIsHilite(p) 	(((W_BUTTON*)p)->state & WBUTS_HILITE)
#define buttonIsFlashing(p)  (((W_BUTTON*)p)->state & WBUTS_FLASHON)

#define FORM_OPEN_ANIM_DURATION		(GAME_TICKS_PER_SEC/6) // Time duration for form open/close anims.

//the loop default value
#define DEFAULT_LOOP		1

static int FormOpenAudioID;	// ID of sfx to play when form opens.
static int FormCloseAudioID; // ID of sfx to play when form closes.
static int FormOpenCount;	// Count used to ensure only one sfx played when two forms opening.
static int FormCloseCount;	// Count used to ensure only one sfx played when two forms closeing.

BASE_STATS *CurrentStatsTemplate = NULL;

#define	DEFAULT_BUTTON_ROTATION (45)
#define BUT_TRANSPORTER_DIST (5000)
#define BUT_TRANSPORTER_SCALE (20)
#define BUT_TRANSPORTER_ALT (-50)

static UDWORD ManuPower = 0;	// Power required to manufacture the current item.


// Display surfaces for rendered buttons.
// FIXME: These variables are never, ever referenced directly;
//        they're always used through the below .*Buffers,
//        so rather than declaring them here we should remove
//        these and make them part of the below
static BUTTON_SURFACE TopicSurfaces[NUM_TOPICSURFACES];
static BUTTON_SURFACE ObjectSurfaces[NUM_OBJECTSURFACES];
static BUTTON_SURFACE StatSurfaces[NUM_STATSURFACES];
static BUTTON_SURFACE System0Surfaces[NUM_SYSTEM0SURFACES];

// Working buffers for rendered buttons.
RENDERED_BUTTON System0Buffers[NUM_SYSTEM0BUFFERS];	// References ObjectSurfaces.
RENDERED_BUTTON ObjectBuffers[NUM_OBJECTBUFFERS];	// References ObjectSurfaces.
RENDERED_BUTTON TopicBuffers[NUM_TOPICBUFFERS];		// References TopicSurfaces.
RENDERED_BUTTON StatBuffers[NUM_STATBUFFERS];		// References StatSurfaces.

// Get the first factory assigned to a command droid
STRUCTURE *droidGetCommandFactory(DROID *psDroid);

static SDWORD ButtonDrawXOffset;
static SDWORD ButtonDrawYOffset;


// Set audio IDs for form opening/closing anims.
// Use -1 to dissable audio.
//
void SetFormAudioIDs(int OpenID,int CloseID)
{
	FormOpenAudioID = OpenID;
	FormCloseAudioID = CloseID;
	FormOpenCount = 0;
	FormCloseCount = 0;
}

static void setBarGraphValue(W_BARGRAPH *barGraph, PIELIGHT colour, int value, int range)
{
	barGraph->majorCol = colour;
	barGraph->majorSize = PERNUM(WBAR_SCALE, clip(value, 0, range), range);
	barGraph->style &= ~WIDG_HIDDEN;
}

static void formatEmpty(W_BARGRAPH *barGraph)
{
	barGraph->text.clear();
	setBarGraphValue(barGraph, WZCOL_BLACK, 0, 1);
}

static void formatTimeText(W_BARGRAPH *barGraph, int time)
{
	char timeText[20];
	ssprintf(timeText, "%d:%02d", time/60, time%60);
	barGraph->text = timeText;
	barGraph->textCol = WZCOL_CONSTRUCTION_BARTEXT;
}

static void formatTime(W_BARGRAPH *barGraph, int buildPointsDone, int buildPointsTotal, int buildRate, char const *toolTip)
{
	widgSetTipText(barGraph, toolTip);

	if (buildRate != 0)
	{
		int timeToBuild = (buildPointsTotal - buildPointsDone) / buildRate;

		formatTimeText(barGraph, timeToBuild);
	}
	else
	{
		barGraph->text.clear();
	}

	setBarGraphValue(barGraph, WZCOL_YELLOW, buildPointsDone, buildPointsTotal);
}

static void formatPowerText(W_BARGRAPH *barGraph, int neededPower)
{
	char powerText[20];
	ssprintf(powerText, "%d", neededPower);
	barGraph->text = powerText;
	barGraph->textCol = WZCOL_POWERQUEUE_BARTEXT;
}

static void formatPower(W_BARGRAPH *barGraph, int neededPower, int powerToBuild)
{
	if (neededPower == -1 || powerToBuild == 0)
	{
		formatEmpty(barGraph);
		return;
	}

	widgSetTipText(barGraph, _("Waiting for Power"));
	formatPowerText(barGraph, neededPower);
	setBarGraphValue(barGraph, WZCOL_GREEN, powerToBuild - neededPower, powerToBuild);
}

// Widget callback to update the progress bar in the object stats screen.
//
void intUpdateProgressBar(WIDGET *psWidget, W_CONTEXT *psContext)
{
	BASE_OBJECT			*psObj;
	DROID				*Droid;
	STRUCTURE			*Structure;
	FACTORY				*Manufacture;
	RESEARCH_FACILITY	*Research;
	W_BARGRAPH			*BarGraph = (W_BARGRAPH*)psWidget;

	psObj = (BASE_OBJECT*)BarGraph->pUserData;	// Get the object associated with this widget.

	if (psObj == NULL)
	{
		BarGraph->style |= WIDG_HIDDEN;
		return;
	}

	if (isDead((BASE_OBJECT *)psObj))
	{
		return;
	}

	BarGraph->majorSize = 0;
	BarGraph->style |= WIDG_HIDDEN;

	switch (psObj->type)
	{
		case OBJ_DROID:						// If it's a droid and...
			Droid = (DROID*)psObj;

			if(DroidIsBuilding(Droid))  // Is it a building.
			{
				ASSERT(Droid->asBits[COMP_CONSTRUCT].nStat, "intUpdateProgressBar: invalid droid type" );

				Structure = DroidGetBuildStructure(Droid);  // Get the structure's building.

				if (Structure)
				{
					//show progress of build
					if (Structure->currentBuildPts != 0)
					{
						formatTime(BarGraph, Structure->currentBuildPts, Structure->pStructureType->buildPoints, Structure->lastBuildRate, _("Build Progress"));
					}
					else
					{
						formatPower(BarGraph, checkPowerRequest(Structure), Structure->pStructureType->powerToBuild);
					}
				}
			}
			break;

		case OBJ_STRUCTURE:					// If it's a structure and...
			Structure = (STRUCTURE*)psObj;

			if (StructureIsManufacturingPending(Structure))  // Is it manufacturing.
			{
				Manufacture = StructureGetFactory(Structure);

				if (Manufacture->psSubject != NULL && Manufacture->buildPointsRemaining < Manufacture->psSubject->buildPoints)
				{
					// Started production. Set the colour of the bar to yellow.
					int buildPointsTotal = FactoryGetTemplate(Manufacture)->buildPoints;
					int buildRate = Manufacture->timeStartHold == 0? Manufacture->productionOutput : 0;
					formatTime(BarGraph, buildPointsTotal - Manufacture->buildPointsRemaining, buildPointsTotal, buildRate, _("Construction Progress"));
				}
				else
				{
					// Not yet started production.
					int neededPower = checkPowerRequest(Structure);
					int powerToBuild = Manufacture->psSubject != NULL? Manufacture->psSubject->powerPoints : 0;
					formatPower(BarGraph, neededPower, powerToBuild);
				}
			}
			else if(structureIsResearchingPending(Structure))  // Is it researching.
			{
				Research = StructureGetResearch(Structure);
				unsigned currentPoints = 0;
				if (Research->psSubject != NULL)
				{
					currentPoints = asPlayerResList[selectedPlayer][Research->psSubject->index].currentPoints;
				}
				if (currentPoints != 0)
				{
					int researchRate = Research->timeStartHold == 0? Research->researchPoints : 0;
					formatTime(BarGraph, currentPoints, Research->psSubject->researchPoints, researchRate, _("Research Progress"));
				}
				else
				{
					// Not yet started production.
					int neededPower = checkPowerRequest(Structure);
					int powerToBuild = Research->psSubject != NULL? Research->psSubject->researchPower : 0;
					formatPower(BarGraph, neededPower, powerToBuild);
				}
			}

			break;

		default:
			ASSERT( false, "intUpdateProgressBar: invalid object type" );
	}
}


void intUpdateQuantity(WIDGET *psWidget, W_CONTEXT *psContext)
{
	BASE_OBJECT		*psObj;
	STRUCTURE		*Structure;
	DROID_TEMPLATE *        psTemplate;
	W_LABEL			*Label = (W_LABEL*)psWidget;

	psObj = (BASE_OBJECT*)Label->pUserData;  // Get the object associated with this widget.
	Structure = (STRUCTURE*)psObj;

	if (psObj != NULL && psObj->type == OBJ_STRUCTURE && StructureIsManufacturingPending(Structure))
	{
		ASSERT(!isDead(psObj),"intUpdateQuantity: object is dead");

		psTemplate = FactoryGetTemplate(StructureGetFactory(Structure));
		int remaining = getProduction(Structure, psTemplate).numRemaining();
		snprintf(Label->aText, sizeof(Label->aText), "%d", remaining);
		Label->style &= ~WIDG_HIDDEN;
	}
	else
	{
		Label->style |= WIDG_HIDDEN;
	}
}

//callback to display the factory number
void intAddFactoryInc(WIDGET *psWidget, W_CONTEXT *psContext)
{
	W_LABEL         *Label = (W_LABEL*)psWidget;
	BASE_OBJECT     *psObj = (BASE_OBJECT*)Label->pUserData;

	// Get the object associated with this widget.
	if (psObj != NULL && !isDead(psObj))
	{
		STRUCTURE	*Structure = (STRUCTURE*)psObj;
		FACTORY		*Factory = &Structure->pFunctionality->factory;

		ASSERT( (Structure->pStructureType->type == REF_FACTORY ||
			Structure->pStructureType->type == REF_CYBORG_FACTORY ||
			Structure->pStructureType->type == REF_VTOL_FACTORY),
			"intAddFactoryInc: structure is not a factory" );

		snprintf(Label->aText, sizeof(Label->aText), "%u", Factory->psAssemblyPoint->factoryInc + 1);
		Label->style &= ~WIDG_HIDDEN;
	}
	else
	{
		Label->aText[0] = '\0';
		Label->style |= WIDG_HIDDEN;
	}
}

//callback to display the production quantity number for a template
void intAddProdQuantity(WIDGET *psWidget, W_CONTEXT *psContext)
{
	W_LABEL				*Label = (W_LABEL*)psWidget;
	DROID_TEMPLATE *                psTemplate = (DROID_TEMPLATE *)Label->pUserData;

	// Get the object associated with this widget.
	if (psTemplate != NULL)
	{
		STRUCTURE	*psStructure = NULL;
		BASE_OBJECT	*psObj = getCurrentSelected();

		if (psObj != NULL && psObj->type == OBJ_STRUCTURE && !isDead(psObj))
		{
			psStructure = (STRUCTURE *)psObj;
		}

		ProductionRunEntry entry;
		if (psStructure != NULL && StructIsFactory(psStructure))
		{
			entry = getProduction(psStructure, psTemplate);
		}

		// now find out how many we have built
		if (entry.isValid())
		{
			if (psStructure->pFunctionality->factory.productionLoops != 0)
			{
				snprintf(Label->aText, sizeof(Label->aText), "%u/%u", entry.numRemaining(), entry.quantity);
			}
			else
			{
				snprintf(Label->aText, sizeof(Label->aText), "%u", entry.numRemaining());
			}
			Label->style &= ~WIDG_HIDDEN;
		}
		else
		{
			Label->aText[0] = '\0';
			Label->style |= WIDG_HIDDEN;
		}
	}
}

//callback to display the production loop quantity number for a factory
void intAddLoopQuantity(WIDGET *psWidget, W_CONTEXT *psContext)
{
	W_LABEL		*Label = (W_LABEL*)psWidget;
	STRUCTURE	*psStruct = (STRUCTURE *)Label->pUserData;

	//loop depends on the factory
	if (psStruct && psStruct->pFunctionality && !psStruct->died)
	{
		FACTORY *psFactory = &psStruct->pFunctionality->factory;

		if (psFactory->productionLoops == INFINITE_PRODUCTION)
		{
			sstrcpy(Label->aText, "∞");
		}
		else if (psFactory->productionLoops != 0)
		{
			snprintf(Label->aText, sizeof(Label->aText), "%u", psFactory->productionLoops + DEFAULT_LOOP);
		}
		else
		{
			Label->aText[0] = '\0';  // Don't show "1" loop.
		}
		Label->style &= ~WIDG_HIDDEN;
	}
	else
	{
		//hide the label if no factory
		Label->aText[0] = '\0';
		Label->style |= WIDG_HIDDEN;
	}
}

// callback to update the command droid size label
void intUpdateCommandSize(WIDGET *psWidget, W_CONTEXT *psContext)
{
	W_LABEL				*Label = (W_LABEL*)psWidget;
	BASE_OBJECT			*psObj = (BASE_OBJECT*)Label->pUserData;

	// Get the object associated with this widget.
	if (psObj != NULL && !isDead(psObj))
	{
		DROID	*psDroid = (DROID *)psObj;

		ASSERT( psDroid->droidType == DROID_COMMAND,
			"intUpdateCommandSize: droid is not a command droid" );

		ssprintf(Label->aText, "%u/%u", psDroid->psGroup ? psDroid->psGroup->getNumMembers() : 0, cmdDroidMaxGroup(psDroid));
		Label->style &= ~WIDG_HIDDEN;
	}
	else
	{
		Label->aText[0] = '\0';
		Label->style |= WIDG_HIDDEN;
	}
}

// callback to update the command droid experience
void intUpdateCommandExp(WIDGET *psWidget, W_CONTEXT *psContext)
{
	W_LABEL				*Label = (W_LABEL*)psWidget;
	BASE_OBJECT			*psObj = (BASE_OBJECT*)Label->pUserData;
	SDWORD				i, numStars;

	// Get the object associated with this widget.
	if (psObj != NULL && !isDead(psObj))
	{
		DROID	*psDroid = (DROID *)psObj;

		ASSERT(psObj->type == OBJ_DROID, "intUpdateCommandExp: invalid droid pointer");
		ASSERT(psDroid->droidType == DROID_COMMAND, "intUpdateCommandExp: droid is not a command droid");

		numStars = getDroidLevel(psDroid);
		numStars = (numStars >= 1) ? (numStars - 1) : 0;
		for(i=0; i<numStars; i++)
		{
			Label->aText[i] = '*';
		}
		Label->aText[i] = '\0';
		Label->style &= ~WIDG_HIDDEN;
	}
	else
	{
		Label->aText[0] = '\0';
		Label->style |= WIDG_HIDDEN;
	}
}

// callback to update the command droid factories
void intUpdateCommandFact(WIDGET *psWidget, W_CONTEXT *psContext)
{
	W_LABEL				*Label = (W_LABEL*)psWidget;
	BASE_OBJECT			*psObj = (BASE_OBJECT*)Label->pUserData;
	SDWORD				i,cIndex, start;

	// Get the object associated with this widget.
	if (psObj != NULL && !isDead(psObj))
	{
		DROID		*psDroid = (DROID *)psObj;

		ASSERT(psObj->type == OBJ_DROID, "intUpdateCommandFact: invalid droid pointer");
		ASSERT(psDroid->droidType == DROID_COMMAND, "intUpdateCommandFact: droid is not a command droid");

		// see which type of factory this is for
		if (Label->id >= IDOBJ_COUNTSTART && Label->id < IDOBJ_COUNTEND)
		{
			start = DSS_ASSPROD_SHIFT;
		}
		else if (Label->id >= IDOBJ_CMDFACSTART && Label->id < IDOBJ_CMDFACEND)
		{
			start = DSS_ASSPROD_CYBORG_SHIFT;
		}
		else
		{
			start = DSS_ASSPROD_VTOL_SHIFT;
		}

		cIndex = 0;
		for (i = 0; i < 5; ++i)  // TODO Support up to MAX_FACTORY (which won't fit in the ugly secondaryOrder bitmask hack).
		{
			if ( psDroid->secondaryOrder & (1 << (i + start)) )
			{
				Label->aText[cIndex] = (char) ('0' + i + 1);
				cIndex += 1;
			}
		}
		Label->aText[cIndex] = '\0';
		Label->style &= ~WIDG_HIDDEN;
	}
	else
	{
		Label->aText[0] = '\0';
		Label->style |= WIDG_HIDDEN;
	}
}


#define BARXOFFSET	46

// Widget callback to update and display the power bar.
// !!!!!!!!!!!!!!!!!!!!!!ONLY WORKS ON A SIDEWAYS POWERBAR!!!!!!!!!!!!!!!!!
void intDisplayPowerBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_BARGRAPH *BarGraph = (W_BARGRAPH*)psWidget;
	SDWORD		x0,y0;
	SDWORD		Avail,ManPow,realPower;
	SDWORD		Empty;
	SDWORD		BarWidth, textWidth = 0;
	SDWORD		iX,iY;
	static char		szVal[8];

	double desiredPower = getPowerMinusQueued(selectedPlayer);
	static double displayPower;
	static unsigned lastRealTime;
	displayPower = desiredPower + (displayPower - desiredPower)*exp((realTime - lastRealTime) / -80.);  // If realTime < lastRealTime, then exp() returns 0 due to unsigned overflow.
	lastRealTime = realTime;

	ManPow = ManuPower / POWERBAR_SCALE;
	Avail = (displayPower + 1e-8) / POWERBAR_SCALE;
	realPower = (displayPower + 1e-8) - ManuPower;

	BarWidth = BarGraph->width;
	iV_SetFont(font_regular);
	sprintf( szVal, "%d", realPower );
	textWidth = iV_GetTextWidth( szVal );
	BarWidth -= textWidth;

	if (ManPow > Avail)
	{
		Empty = BarWidth - ManPow;
	}
	else
	{
		Empty = BarWidth - Avail;
	}

	if (Avail > BarWidth)
	{
		ManPow = PERNUM(BarWidth,ManPow,Avail);
		Avail = BarWidth;
		Empty = 0;
	}

	if (ManPow > BarWidth)
	{
		ManPow = BarWidth;
		Avail = 0;
		Empty = 0;
	}

	x0 = xOffset + BarGraph->x;
	y0 = yOffset + BarGraph->y;

	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	pie_SetFogStatus(false);


	iV_DrawImage(IntImages,IMAGE_PBAR_TOP,x0,y0);

	iX = x0 + 3;
	iY = y0 + 10;

	x0 += iV_GetImageWidth(IntImages,IMAGE_PBAR_TOP);

	//fill in the empty section behind text
	if(textWidth > 0)
	{
		iV_DrawImageRect(IntImages,IMAGE_PBAR_EMPTY,
							x0,y0,
							textWidth, iV_GetImageHeight(IntImages,IMAGE_PBAR_EMPTY));
		x0 += textWidth;
	}

	//draw required section
	if (ManPow > Avail)
	{
		//draw the required in red
		iV_DrawImageRect(IntImages,IMAGE_PBAR_USED,
							x0,y0,
							ManPow, iV_GetImageHeight(IntImages,IMAGE_PBAR_USED));
	}
	else
	{
		iV_DrawImageRect(IntImages,IMAGE_PBAR_REQUIRED,
							x0,y0,
							ManPow, iV_GetImageHeight(IntImages,IMAGE_PBAR_REQUIRED));
	}

	x0 += ManPow;

	//draw the available section if any!
	if(Avail-ManPow > 0)
	{
		iV_DrawImageRect(IntImages,IMAGE_PBAR_AVAIL,
							x0,y0,
							Avail-ManPow, iV_GetImageHeight(IntImages,IMAGE_PBAR_AVAIL));

		x0 += Avail-ManPow;
	}

	//fill in the rest with empty section
	if(Empty > 0)
	{
		iV_DrawImageRect(IntImages,IMAGE_PBAR_EMPTY,
							x0,y0,
							Empty, iV_GetImageHeight(IntImages,IMAGE_PBAR_EMPTY));
		x0 += Empty;
	}

	iV_DrawImage(IntImages,IMAGE_PBAR_BOTTOM,x0,y0);
	if (Avail < 0)
	{
		const char *need = _("Need more resources!");
		if ((realTime / 1250) % 5 == 0 )
		{
			iV_SetTextColour(WZCOL_BLACK);
			iV_SetFont(font_small);
			iV_DrawText(need, iX + 102, iY - 1  );
			iV_SetTextColour(WZCOL_RED);
		}
		else
		{
			iV_SetTextColour(WZCOL_RED);
			iV_SetFont(font_small);
			iV_DrawText(need, iX + 102, iY - 1  );
		}
	}
	else
	{
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	}
	// draw text value
	iV_SetFont(font_regular);
	iV_DrawText( szVal, iX, iY );
}


// Widget callback to display a rendered status button, ie the progress of a manufacturing or
// building task.
//
void intDisplayStatusButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_CLICKFORM         *Form = (W_CLICKFORM*)psWidget;
	BASE_OBJECT         *psObj;
	STRUCTURE           *Structure;
	DROID               *Droid;
	bool                Down;
	SDWORD              Image;
	bool                Hilight = false;
	BASE_STATS          *Stats, *psResGraphic;
	RENDERED_BUTTON     *Buffer = (RENDERED_BUTTON*)Form->pUserData;
	UDWORD              IMDType = 0, compID;
	UDWORD              Player = selectedPlayer;			// changed by AJL for multiplayer.
	void                *Object;
	bool	            bOnHold = false;

	OpenButtonRender((UWORD)(xOffset+Form->x), (UWORD)(yOffset+Form->y),(UWORD)Form->width,(UWORD)Form->height);

	Down = Form->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK);

		Hilight = Form->state & WCLICK_HILITE;

		if(Hilight) {
			Buffer->ImdRotation += realTimeAdjustedAverage(BUTTONOBJ_ROTSPEED);
		}

		Hilight = formIsHilite(Form);	// Hilited or flashing.

		Buffer->State = Form->state;

		Object = NULL;
		Image = -1;
		psObj = (BASE_OBJECT*)Buffer->Data;	// Get the object associated with this widget.

		if (psObj && isDead(psObj))
		{
			// this may catch this horrible crash bug we've been having,
			// who knows?.... Shipping tomorrow, la de da :-)
			psObj = NULL;
			Buffer->Data = NULL;
			intRefreshScreen();
		}

		if(psObj) {
			switch (psObj->type) {
				case OBJ_DROID:						// If it's a droid...
					Droid = (DROID*)psObj;

					if(DroidIsBuilding(Droid)) {
						Structure = DroidGetBuildStructure(Droid);
						if(Structure) {
							Object = Structure;
							IMDType = IMDTYPE_STRUCTURE;
							RENDERBUTTON_INITIALISED(Buffer);
						}
					} else if (DroidGoingToBuild(Droid)) {
						Stats = DroidGetBuildStats(Droid);
						ASSERT( Stats!=NULL,"intDisplayStatusButton : NULL Stats pointer." );
						Object = (void*)Stats;	//StatGetStructureIMD(Stats,selectedPlayer);
						Player = selectedPlayer;
						IMDType = IMDTYPE_STRUCTURESTAT;
						RENDERBUTTON_INITIALISED(Buffer);
					} else if (orderState(Droid, DORDER_DEMOLISH)) {
						Stats = (BASE_STATS *)structGetDemolishStat();
						ASSERT( Stats!=NULL,"intDisplayStatusButton : NULL Stats pointer." );
						Object = (void*)Stats;
						Player = selectedPlayer;
						IMDType = IMDTYPE_STRUCTURESTAT;
						RENDERBUTTON_INITIALISED(Buffer);
					} else if (Droid->droidType == DROID_COMMAND) {
						Structure = droidGetCommandFactory(Droid);
						if (Structure) {
							Object = Structure;
							IMDType = IMDTYPE_STRUCTURE;
							RENDERBUTTON_INITIALISED(Buffer);
						}
					}
					break;

				case OBJ_STRUCTURE:					// If it's a structure...
					Structure = (STRUCTURE*)psObj;
					switch(Structure->pStructureType->type)
					{
						case REF_FACTORY:
						case REF_CYBORG_FACTORY:
						case REF_VTOL_FACTORY:
							if (StructureIsManufacturingPending(Structure))
							{
								IMDType = IMDTYPE_DROIDTEMPLATE;
								Object = (void*)FactoryGetTemplate(StructureGetFactory(Structure));
								RENDERBUTTON_INITIALISED(Buffer);
								bOnHold = StructureIsOnHoldPending(Structure);
							}

							break;

						case REF_RESEARCH:
							if (structureIsResearchingPending(Structure))
							{
								iIMDShape *shape = (iIMDShape *)Object;
								Stats = (BASE_STATS*)Buffer->Data2;
								if (!Stats)
								{
									break;
								}
								bOnHold = StructureIsOnHoldPending(Structure);
								StatGetResearchImage(Stats,&Image,&shape, &psResGraphic, false);
								Object = shape;
								if (psResGraphic)
								{
									// we have a Stat associated with this research topic
									if (StatIsStructure(psResGraphic))
									{
										// overwrite the Object pointer
										Object = (void*)psResGraphic;
										Player = selectedPlayer;
										// this defines how the button is drawn
										IMDType = IMDTYPE_STRUCTURESTAT;
									}
									else
									{
										compID = StatIsComponent(psResGraphic);
										if (compID != COMP_UNKNOWN)
										{
											// this defines how the button is drawn
											IMDType = IMDTYPE_COMPONENT;
											// overwrite the Object pointer
											Object = (void*)psResGraphic;
										}
										else
										{
											ASSERT(false, "intDisplayStatsButton:Invalid Stat for research button");
											Object = NULL;
											IMDType = IMDTYPE_RESEARCH;
										}
									}
								}
								else
								{
									// no Stat for this research topic so just use the graphic provided
									// if Object != NULL the there must be a IMD so set the object to
									// equal the Research stat
									if (Object != NULL)
									{
										Object = (void*)Stats;
										IMDType = IMDTYPE_RESEARCH;
									}
								}
								RENDERBUTTON_INITIALISED(Buffer);
							}
							break;
						default:
							break;
					}
					break;

				default:
					ASSERT( false, "intDisplayObjectButton: invalid structure type" );
			}
		} else
		{
			RENDERBUTTON_INITIALISED(Buffer);
		}

		ButtonDrawXOffset = ButtonDrawYOffset = 0;


		// Render the object into the button.
		if(Object) {
			if(Image >= 0) {
				RenderToButton(IntImages,(UWORD)Image,Object,Player,Buffer,Down,IMDType,TOPBUTTON);
			} else {
				RenderToButton(NULL,0,Object,Player,Buffer,Down,IMDType,TOPBUTTON);
			}
		} else if(Image >= 0) {
			RenderImageToButton(IntImages,(UWORD)Image,Buffer,Down,TOPBUTTON);
		} else {
			RenderBlankToButton(Buffer,Down,TOPBUTTON);
		}

	CloseButtonRender();

	//need to flash the button if a factory is on hold production
	if (bOnHold)
	{
		if (((realTime/250) % 2) == 0)
		{
			iV_DrawImage(IntImages,IMAGE_BUT0_DOWN,xOffset+Form->x,yOffset+Form->y);
		}
		else
		{
			iV_DrawImage(IntImages,IMAGE_BUT_HILITE,xOffset+Form->x,yOffset+Form->y);
		}
	}
	else
	{
		if (Hilight)
		{

			iV_DrawImage(IntImages,IMAGE_BUT_HILITE,xOffset+Form->x,yOffset+Form->y);
		}
	}
}


// Widget callback to display a rendered object button.
//
void intDisplayObjectButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_CLICKFORM *Form = (W_CLICKFORM*)psWidget;
	BASE_OBJECT *psObj;
	bool Down;
	bool Hilight = false;
	RENDERED_BUTTON *Buffer = (RENDERED_BUTTON*)Form->pUserData;
	UDWORD IMDType = 0;
	void *Object;

	OpenButtonRender((UWORD)(xOffset+Form->x), (UWORD)(yOffset+Form->y),(UWORD)Form->width,(UWORD)(Form->height+9));

	Down = Form->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK);

		Hilight = Form->state & WCLICK_HILITE;

		if(Hilight) {
			Buffer->ImdRotation += realTimeAdjustedAverage(BUTTONOBJ_ROTSPEED);
		}

		Hilight = formIsHilite(Form);	// Hilited or flashing.

		Buffer->State = Form->state;

		Object = NULL;
		psObj = (BASE_OBJECT*)Buffer->Data;	// Get the object associated with this widget.

		if (psObj && isDead(psObj))
		{
			// this may catch this horrible crash bug we've been having,
			// who knows?.... Shipping tomorrow, la de da :-)
			psObj = NULL;
			Buffer->Data = NULL;
			intRefreshScreen();
		}

		if(psObj) {
			switch (psObj->type) {
				case OBJ_DROID:						// If it's a droid...
					IMDType = IMDTYPE_DROID;
					Object = (void*)psObj;
					break;

				case OBJ_STRUCTURE:					// If it's a structure...
					IMDType = IMDTYPE_STRUCTURE;
					Object = (void*)psObj;
					break;

				default:
					ASSERT( false, "intDisplayStatusButton: invalid structure type" );
			}
		}

		ButtonDrawXOffset = ButtonDrawYOffset = 0;

		if(Object) {
			RenderToButton(NULL,0,Object,selectedPlayer,Buffer,Down,IMDType,BTMBUTTON);	// ajl, changed from 0 to selectedPlayer
		} else {
			RenderBlankToButton(Buffer,Down,BTMBUTTON);
		}

		RENDERBUTTON_INITIALISED(Buffer);

	CloseButtonRender();

	if (Hilight)
	{

		iV_DrawImage(IntImages,IMAGE_BUTB_HILITE,xOffset+Form->x,yOffset+Form->y);
	}
}


// Widget callback to display a rendered stats button, ie the job selection window buttons.
//
void intDisplayStatsButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_CLICKFORM     *Form = (W_CLICKFORM*)psWidget;
	BASE_STATS      *Stat, *psResGraphic;
	bool            Down;
	SDWORD          Image, compID;
	bool            Hilight = false;
	RENDERED_BUTTON *Buffer = (RENDERED_BUTTON*)Form->pUserData;
	UDWORD          IMDType = 0;
	UDWORD          Player = selectedPlayer;		// ajl, changed for multiplayer (from 0)
	void            *Object;

	OpenButtonRender((UWORD)(xOffset+Form->x), (UWORD)(yOffset+Form->y),(UWORD)Form->width,(UWORD)Form->height);

	Down = Form->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK);

		Hilight = Form->state & WCLICK_HILITE;

		if(Hilight) {
			Buffer->ImdRotation += realTimeAdjustedAverage(BUTTONOBJ_ROTSPEED);
		}

		Hilight = formIsHilite(Form);

		Buffer->State = Form->state;

		Object = NULL;
		Image = -1;

		Stat = (BASE_STATS*)Buffer->Data;

		ButtonDrawXOffset = ButtonDrawYOffset = 0;

		if(Stat)
		{
			if(StatIsStructure(Stat))
			{
				Object = (void*)Stat;
				Player = selectedPlayer;
				IMDType = IMDTYPE_STRUCTURESTAT;
			}
			else if(StatIsTemplate(Stat))
			{
				IMDType = IMDTYPE_DROIDTEMPLATE;
				Object = (void*)Stat;
			}
			else
			{
				//if(StatIsComponent(Stat))
				//{
				//	IMDType = IMDTYPE_COMPONENT;
				//	Shape = StatGetComponentIMD(Stat);
				//}
				compID = StatIsComponent(Stat); // This failes for viper body.
				if (compID != COMP_UNKNOWN)
				{
					IMDType = IMDTYPE_COMPONENT;
					Object = (void*)Stat;	//StatGetComponentIMD(Stat, compID);
				}
				else if(StatIsResearch(Stat))
				{
					iIMDShape *shape = (iIMDShape *)Object;
					/*IMDType = IMDTYPE_RESEARCH;
					StatGetResearchImage(Stat,&Image,(iIMDShape**)&Object,true);
					//if Object != NULL the there must be a IMD so set the object to
					//equal the Research stat
					if (Object != NULL)
					{
						Object = (void*)Stat;
					}*/
                    StatGetResearchImage(Stat,&Image,&shape, &psResGraphic, true);
					Object = shape;
                    if (psResGraphic)
                    {
                        //we have a Stat associated with this research topic
                        if  (StatIsStructure(psResGraphic))
                        {
                            //overwrite the Object pointer
                            Object = (void*)psResGraphic;
				            Player = selectedPlayer;
                            //this defines how the button is drawn
				            IMDType = IMDTYPE_STRUCTURESTAT;
                        }
                        else
                        {
            				compID = StatIsComponent(psResGraphic);
				            if (compID != COMP_UNKNOWN)
				            {
                                //this defines how the button is drawn
					            IMDType = IMDTYPE_COMPONENT;
                                //overwrite the Object pointer
					            Object = (void*)psResGraphic;
				            }
                            else
                            {
                                ASSERT( false,
                                    "intDisplayStatsButton:Invalid Stat for research button" );
                                Object = NULL;
                                IMDType = IMDTYPE_RESEARCH;
                            }
                        }
                    }
                    else
                    {
                        //no Stat for this research topic so just use the graphic provided
                        //if Object != NULL the there must be a IMD so set the object to
                        //equal the Research stat
                        if (Object != NULL)
                        {
                            Object = (void*)Stat;
                            IMDType = IMDTYPE_RESEARCH;
                        }
                    }
				}
			}

			if(Down)
			{
				CurrentStatsTemplate = Stat;
			}

		}
		else
		{
			IMDType = IMDTYPE_COMPONENT;
			//BLANK button for now - AB 9/1/98
			Object = NULL;
			CurrentStatsTemplate = NULL;
		}


		if(Object) {
			if(Image >= 0) {
				RenderToButton(IntImages,(UWORD)Image,Object,Player,Buffer,Down,IMDType,TOPBUTTON);
  			} else {
				RenderToButton(NULL,0,Object,Player,Buffer,Down,IMDType,TOPBUTTON);
			}
		} else if(Image >= 0) {
			RenderImageToButton(IntImages,(UWORD)Image,Buffer,Down,TOPBUTTON);
		} else {
			RenderBlankToButton(Buffer,Down,TOPBUTTON);
		}

		RENDERBUTTON_INITIALISED(Buffer);

	CloseButtonRender();

	if (Hilight)
	{

		iV_DrawImage(IntImages,IMAGE_BUT_HILITE,xOffset+Form->x,yOffset+Form->y);
	}
}


void RenderToButton(IMAGEFILE *ImageFile,UWORD ImageID,void *Object,UDWORD Player,
					RENDERED_BUTTON *Buffer,bool Down, UDWORD IMDType, UDWORD buttonType)
{
	CreateIMDButton(ImageFile,ImageID,Object,Player,Buffer,Down,IMDType,buttonType);
}

void RenderImageToButton(IMAGEFILE *ImageFile,UWORD ImageID,RENDERED_BUTTON *Buffer,bool Down, UDWORD buttonType)
{
	CreateImageButton(ImageFile,ImageID,Buffer,Down,buttonType);
}

void RenderBlankToButton(RENDERED_BUTTON *Buffer,bool Down, UDWORD buttonType)
{
	CreateBlankButton(Buffer,Down,buttonType);
}


void AdjustTabFormSize(W_TABFORM *Form,UDWORD *x0,UDWORD *y0,UDWORD *x1,UDWORD *y1)
{
	/* Adjust for where the tabs are */
	if(Form->majorPos == WFORM_TABLEFT) {
		*x0 += Form->tabMajorThickness - Form->tabHorzOffset;
	} else if(Form->minorPos == WFORM_TABLEFT) {
		*x0 += Form->tabMinorThickness - Form->tabHorzOffset;
	}
	if(Form->majorPos == WFORM_TABRIGHT) {
		*x1 -= Form->tabMajorThickness - Form->tabHorzOffset;
	} else if(Form->minorPos == WFORM_TABRIGHT) {
		*x1 -= Form->tabMinorThickness - Form->tabHorzOffset;
	}
	if(Form->majorPos == WFORM_TABTOP) {
		*y0 += Form->tabMajorThickness - Form->tabVertOffset;
	} else if(Form->minorPos == WFORM_TABTOP) {
		*y0 += Form->tabMinorThickness - Form->tabVertOffset;
	}
	if(Form->majorPos == WFORM_TABBOTTOM) {
		*y1 -= Form->tabMajorThickness - Form->tabVertOffset;
	} else if(Form->minorPos == WFORM_TABBOTTOM) {
		*y1 -= Form->tabMinorThickness - Form->tabVertOffset;
	}
}


// Widget callback function to do the open form animation. Doesn't just open Plain Forms!!
//
void intOpenPlainForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_TABFORM	*Form = (W_TABFORM*)psWidget;
	UDWORD		Tx0,Ty0,Tx1,Ty1;
	UDWORD		Range;
	UDWORD		Duration;
	UDWORD		APos;
	SDWORD		Ay0,Ay1;

	Tx0 = xOffset+Form->x;
	Ty0 = yOffset+Form->y;
	Tx1 = Tx0 + Form->width;
	Ty1 = Ty0 + Form->height;

	if(Form->animCount == 0) {
		if( (FormOpenAudioID >= 0) && (FormOpenCount == 0) ) {
			audio_PlayTrack(FormOpenAudioID);
			FormOpenCount++;
		}
		Form->Ax0 = (UWORD)Tx0;
		Form->Ax1 = (UWORD)Tx1;
		Form->Ay0 = (UWORD)(Ty0 + (Form->height/2) - 4);
		Form->Ay1 = (UWORD)(Ty0 + (Form->height/2) + 4);
		Form->startTime = realTime;
	} else {
		FormOpenCount = 0;
	}

	RenderWindowFrame(FRAME_NORMAL, Form->Ax0, Form->Ay0, Form->Ax1 - Form->Ax0, Form->Ay1 - Form->Ay0);

	Form->animCount++;

	Range = (Form->height/2)-4;
	Duration = (realTime - Form->startTime) << 16 ;
	APos = (Range * (Duration / FORM_OPEN_ANIM_DURATION) ) >> 16;

	Ay0 = Ty0 + (Form->height/2) - 4 - APos;
	Ay1 = Ty0 + (Form->height/2) + 4 + APos;

	if(Ay0 <= (SDWORD)Ty0)
	{
		Ay0 = Ty0;
	}

	if(Ay1 >= (SDWORD)Ty1)
	{
		Ay1 = Ty1;
	}
	Form->Ay0 = (UWORD)Ay0;
	Form->Ay1 = (UWORD)Ay1;

	if((Form->Ay0 == Ty0) && (Form->Ay1 == Ty1))
	{
		if (Form->pUserData != NULL)
		{
			Form->display = (WIDGET_DISPLAY)Form->pUserData;
		}
		else
		{
			//default to display
			Form->display = intDisplayPlainForm;
		}
		Form->disableChildren = false;
		Form->animCount = 0;
	}
}


// Widget callback function to do the close form animation.
//
void intClosePlainForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_TABFORM *Form = (W_TABFORM*)psWidget;
	UDWORD Ty0, Ty1;
	UDWORD Range;
	UDWORD Duration;
	UDWORD APos;

	Ty0 = yOffset+Form->y + (Form->height/2) - 4;
	Ty1 = yOffset+Form->y + (Form->height/2) + 4;

	if(Form->animCount == 0) {
		if( (FormCloseAudioID >= 0) && (FormCloseCount == 0) ){
			audio_PlayTrack(FormCloseAudioID);
			FormCloseCount++;
		}
		Form->Ax0 = (UWORD)(xOffset+Form->x);
		Form->Ay0 = (UWORD)(yOffset+Form->y);
		Form->Ax1 = (UWORD)(Form->Ax0 + Form->width);
		Form->Ay1 = (UWORD)(Form->Ay0 + Form->height);
		Form->startTime = realTime;
	} else {
		FormCloseCount = 0;
	}

	RenderWindowFrame(FRAME_NORMAL, Form->Ax0, Form->Ay0, Form->Ax1 - Form->Ax0, Form->Ay1 - Form->Ay0);

	Form->animCount++;

	Range = (Form->height/2)-4;
	Duration = (realTime - Form->startTime) << 16 ;
	APos = (Range * (Duration / FORM_OPEN_ANIM_DURATION) ) >> 16;

	Form->Ay0 = (UWORD)(yOffset + Form->y + APos);
	Form->Ay1 = (UWORD)(yOffset + Form->y + Form->height - APos);

	if(Form->Ay0 >= Ty0) {
		Form->Ay0 = (UWORD)Ty0;
	}
	if(Form->Ay1 <= Ty1) {
		Form->Ay1 = (UWORD)Ty1;
	}

	if((Form->Ay0 == Ty0) && (Form->Ay1 == Ty1)) {
		Form->pUserData = (void*)1;
		Form->animCount = 0;
	}
}


void intDisplayPlainForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_TABFORM *Form = (W_TABFORM*)psWidget;
	UDWORD x0,y0,x1,y1;

	x0 = xOffset+Form->x;
	y0 = yOffset+Form->y;
	x1 = x0 + Form->width;
	y1 = y0 + Form->height;

	RenderWindowFrame(FRAME_NORMAL, x0, y0, x1 - x0, y1 - y0);
}


void intDisplayStatsForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_TABFORM *Form = (W_TABFORM*)psWidget;
	UDWORD x0,y0,x1,y1;

	x0 = xOffset+Form->x;
	y0 = yOffset+Form->y;
	x1 = x0 + Form->width;
	y1 = y0 + Form->height;

	AdjustTabFormSize(Form,&x0,&y0,&x1,&y1);

	RenderWindowFrame(FRAME_NORMAL, x0, y0, x1 - x0, y1 - y0);
}


// Display an image for a widget.
//
void intDisplayImage(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y;

	iV_DrawImage(IntImages,psWidget->UserData,x,y);
}


//draws the mission clock - flashes when below a predefined time
void intDisplayMissionClock(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	UDWORD  x = xOffset+psWidget->x;
	UDWORD  y = yOffset+psWidget->y;
	UDWORD  flash;

	// Draw the background image
	iV_DrawImage(IntImages, UNPACKDWORD_TRI_B(psWidget->UserData), x, y);
	// Need to flash the timer when < 5 minutes remaining, but > 4 minutes
	flash = UNPACKDWORD_TRI_A(psWidget->UserData);
	if (flash && ((realTime / 250) % 2) == 0)
	{
		iV_DrawImage(IntImages, UNPACKDWORD_TRI_C(psWidget->UserData), x, y);
	}
}



// Display one of two images depending on if the widget is hilighted by the mouse.
//
void intDisplayImageHilight(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y, flash;
	UWORD ImageID;
	bool Hilight = false;

	switch(psWidget->type) {
		case WIDG_FORM:
			Hilight = formIsHilite(psWidget);
			break;

		case WIDG_BUTTON:
			Hilight = buttonIsHilite(psWidget);
			break;

		case WIDG_EDITBOX:
			if( ((W_EDITBOX*)psWidget)->state & WEDBS_HILITE) {
				Hilight = true;
			}
			break;

		case WIDG_SLIDER:
			if( ((W_SLIDER*)psWidget)->state & SLD_HILITE) {
				Hilight = true;
			}
			break;

		default:
			Hilight = false;
	}

	ImageID = UNPACKDWORD_TRI_C(psWidget->UserData);


	//need to flash the button if Full Transporter
	flash = UNPACKDWORD_TRI_A(psWidget->UserData);
	if (flash && psWidget->id == IDTRANS_LAUNCH)
	{
		if (((realTime/250) % 2) == 0)
		{
    		iV_DrawImage(IntImages,UNPACKDWORD_TRI_B(psWidget->UserData),x,y);
		}
		else
		{
        	iV_DrawImage(IntImages,ImageID,x,y);
		}
	}
	else
	{
		iV_DrawImage(IntImages,ImageID,x,y);
		if (Hilight)
		{
			iV_DrawImage(IntImages,UNPACKDWORD_TRI_B(psWidget->UserData),x,y);
		}
	}
}


static void GetButtonState(WIDGET *psWidget,bool *Hilight,UDWORD *Down,bool *Grey)
{
	switch(psWidget->type) {
		case WIDG_FORM:
			*Hilight = formIsHilite(psWidget);
			if( ((W_CLICKFORM*)psWidget)->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK)) {
				*Down = 1;
			}
			if( ((W_CLICKFORM*)psWidget)->state & WCLICK_GREY) {
				*Grey = true;
			}
			break;

		case WIDG_BUTTON:
			*Hilight = buttonIsHilite(psWidget);
			if( ((W_BUTTON*)psWidget)->state & (WBUTS_DOWN | WBUTS_LOCKED | WBUTS_CLICKLOCK)) {
				*Down = 1;
			}
			if( ((W_BUTTON*)psWidget)->state & WBUTS_GREY) {
				*Grey = true;
			}
			break;

		case WIDG_EDITBOX:
			if( ((W_EDITBOX*)psWidget)->state & WEDBS_HILITE) {
				*Hilight = true;
			}
			break;

		case WIDG_SLIDER:
			if( ((W_SLIDER*)psWidget)->state & SLD_HILITE) {
				*Hilight = true;
			}
			if( ((W_SLIDER*)psWidget)->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK)) {
				*Down = 1;
			}
			break;

		default:
			*Hilight = false;
	}
}


// Display one of two images depending on if the widget is hilighted by the mouse.
//
void intDisplayButtonHilight(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y;
	bool Hilight = false;
	bool Grey = false;
	UDWORD Down = 0;
	UWORD ImageID;

	GetButtonState(psWidget,&Hilight,&Down,&Grey);
	if(Grey) {
		ImageID = UNPACKDWORD_TRI_A(psWidget->UserData);
		Hilight = false;
	} else {
		ImageID = UNPACKDWORD_TRI_C(psWidget->UserData) + Down;
	}

	iV_DrawImage(IntImages,ImageID,x,y);
	if(Hilight) {
		iV_DrawImage(IntImages,UNPACKDWORD_TRI_B(psWidget->UserData),x,y);
	}

}

// Flash one of two images, regardless of whether or not it is highlighted
// Commented-out portions are retained because I am planning on making the intensity of the
// flash depend on whether or not the button is highlighted.
void intDisplayButtonFlash(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y;
	UWORD ImageID;

	ASSERT( psWidget->type == WIDG_BUTTON,"intDisplayButtonFlash : Not a button" );

	if ((realTime / 250) % 2 == 0)
	{
		ImageID = UNPACKDWORD_TRI_B(psWidget->UserData);
	}
	else
	{
		ImageID = UNPACKDWORD_TRI_C(psWidget->UserData);
	}

	iV_DrawImage(IntImages,ImageID,x,y);
}

void intDisplayReticuleButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	bool	Hilight = false;
	bool	Down = false;
	UBYTE	DownTime = UNPACKDWORD_QUAD_C(psWidget->UserData);
	UBYTE	Index = UNPACKDWORD_QUAD_D(psWidget->UserData);
	UBYTE	flashing = UNPACKDWORD_QUAD_A(psWidget->UserData);
	UBYTE	flashTime = UNPACKDWORD_QUAD_B(psWidget->UserData);
	UWORD	ImageID;

	ASSERT(psWidget->type == WIDG_BUTTON, "Not a button");

	if (((W_BUTTON*)psWidget)->state & WBUTS_GREY)
	{
		iV_DrawImage(IntImages,IMAGE_RETICULE_GREY,x,y);
		return;
	}

	Down = ((W_BUTTON*)psWidget)->state & (WBUTS_DOWN | WBUTS_CLICKLOCK);
	Hilight = buttonIsHilite(psWidget);

	if(Down)
	{
		if((DownTime < 1) && (Index != IMAGE_CANCEL_UP))
		{
			ImageID = IMAGE_RETICULE_BUTDOWN;	// Do the button flash.
		}
		else
		{
			ImageID = (UWORD)(Index+1);					// It's down.
		}
		DownTime++;
		//stop the reticule from flashing if it was
		flashing = (UBYTE)false;
	}
	else
	{
		//flashing button?
		if (flashing)
		{
			if (((realTime/250) % 2) == 0)
			{
				ImageID = (UWORD)(Index);//IMAGE_RETICULE_BUTDOWN;//a step in the right direction JPS 27-4-98
			}
			else
			{
				ImageID = (UWORD)(Index+1);
				flashTime = 0;
			}
			flashTime++;
		}
		else
		{
			DownTime = 0;
			ImageID = Index;						// It's up.
		}
	}

	iV_DrawImage(IntImages,ImageID,x,y);

	if(Hilight)
	{
		if (Index == IMAGE_CANCEL_UP)
		{
			iV_DrawImage(IntImages,IMAGE_CANCEL_HILIGHT,x,y);
		}
		else
		{
			iV_DrawImage(IntImages,IMAGE_RETICULE_HILIGHT,x,y);
		}
	}

	psWidget->UserData = PACKDWORD_QUAD(flashTime,flashing,DownTime,Index);
}


void intDisplayTab(WIDGET *psWidget,UDWORD TabType, UDWORD Position,
				   UDWORD Number,bool Selected,bool Hilight,UDWORD x,UDWORD y,UDWORD Width,UDWORD Height)
{
	TABDEF *Tab = (TABDEF*)psWidget->pUserData;

	if(TabType == TAB_MAJOR)
	{
		iV_DrawImage(IntImages, (UWORD)Tab->MajorUp, x, y);

		if(Hilight) {
			iV_DrawImage(IntImages,(UWORD)Tab->MajorHilight,x,y);
		} else if(Selected) {
			iV_DrawImage(IntImages,(UWORD)Tab->MajorSelected,x,y);
		}
	} else {
		iV_DrawImage(IntImages, (UWORD)(Tab->MinorUp), x, y);

		if(Hilight) {
			iV_DrawImage(IntImages,Tab->MinorHilight,x,y);
		} else if(Selected) {
			iV_DrawImage(IntImages,Tab->MinorSelected,x,y);
		}
	}
}

// Display one of three images depending on if the widget is currently depressed (ah!).
//
void intDisplayButtonPressed(WIDGET *psWidget, UDWORD xOffset,
							 UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_BUTTON	*psButton = (W_BUTTON*)psWidget;
	UDWORD		x = xOffset+psButton->x;
	UDWORD		y = yOffset+psButton->y;
	UBYTE		Hilight = 0;
	UWORD		ImageID;

	if (psButton->state & (WBUTS_DOWN | WBUTS_LOCKED | WBUTS_CLICKLOCK))
	{
		ImageID = UNPACKDWORD_TRI_A(psWidget->UserData);
	}
	else
	{
		ImageID = UNPACKDWORD_TRI_C(psWidget->UserData);
	}

	Hilight = (UBYTE)buttonIsHilite(psButton);

	iV_DrawImage(IntImages,ImageID,x,y);
	if (Hilight)
	{
		iV_DrawImage(IntImages,UNPACKDWORD_TRI_B(psWidget->UserData),x,y);
	}
}

// Display DP images depending on factory and if the widget is currently depressed
void intDisplayDPButton(WIDGET *psWidget, UDWORD xOffset,
						UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_BUTTON	*psButton = (W_BUTTON*)psWidget;
	STRUCTURE	*psStruct = (STRUCTURE*)psButton->pUserData;
	UDWORD		x = xOffset+psButton->x;
	UDWORD		y = yOffset+psButton->y;
	UBYTE		hilight = 0, down = 0;
	UWORD		imageID;

	if (psStruct)
	{
		ASSERT( StructIsFactory(psStruct),
			"intDisplayDPButton: structure is not a factory" );

		if (psButton->state & (WBUTS_DOWN | WBUTS_LOCKED | WBUTS_CLICKLOCK))
		{
			down = true;
		}

		hilight = (UBYTE)buttonIsHilite(psButton);

		switch(psStruct->pStructureType->type)
		{
		case REF_FACTORY:
			imageID = IMAGE_FDP_UP;
			break;
		case REF_CYBORG_FACTORY:
			imageID = IMAGE_CDP_UP;
			break;
		case REF_VTOL_FACTORY:
			imageID = IMAGE_VDP_UP;
			break;
		default:
			return;
		}


		iV_DrawImage(IntImages,imageID,x,y);
		if (hilight)
		{
			imageID++;
			iV_DrawImage(IntImages,(UWORD)imageID,x,y);
		}
		else if (down)
		{
			imageID--;
			iV_DrawImage(IntImages,(UWORD)imageID,x,y);
		}

	}
}


void intDisplaySlider(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_SLIDER *Slider = (W_SLIDER*)psWidget;
	UDWORD x = xOffset+psWidget->x;
	UDWORD y = yOffset+psWidget->y;
	SWORD sx;

	iV_DrawImage(IntImages,IMAGE_SLIDER_BACK,x+STAT_SLD_OX,y+STAT_SLD_OY);

	sx = (SWORD)((Slider->width - Slider->barSize)
	 			 * Slider->pos / Slider->numStops);

	iV_DrawImage(IntImages,IMAGE_SLIDER_BUT,x+sx,y-2);
}


/* display highlighted edit box from left, middle and end edit box graphics */
void intDisplayEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{

	W_EDITBOX	*psEditBox = (W_EDITBOX *) psWidget;
	UWORD		iImageIDLeft, iImageIDMid, iImageIDRight;
	UDWORD		iX, iY, iDX, iXRight;
	UDWORD		iXLeft = xOffset + psWidget->x,
				iYLeft = yOffset + psWidget->y;

	if ( psEditBox->state & WEDBS_HILITE )
	{
		iImageIDLeft  = IMAGE_DES_EDITBOXLEFTH;
		iImageIDMid   = IMAGE_DES_EDITBOXMIDH;
		iImageIDRight = IMAGE_DES_EDITBOXRIGHTH;
	}
	else
	{
		iImageIDLeft  = IMAGE_DES_EDITBOXLEFT;
		iImageIDMid   = IMAGE_DES_EDITBOXMID;
		iImageIDRight = IMAGE_DES_EDITBOXRIGHT;
	}

	/* draw left side of bar */
	iX = iXLeft;
	iY = iYLeft;
	iV_DrawImage( IntImages, iImageIDLeft, iX, iY );

	/* draw middle of bar */
	iX += iV_GetImageWidth( IntImages, iImageIDLeft );
	iDX = iV_GetImageWidth( IntImages, iImageIDMid );
	iXRight = xOffset + psWidget->width - iV_GetImageWidth( IntImages, iImageIDRight );
	while ( iX < iXRight )
	{
		iV_DrawImage( IntImages, iImageIDMid, iX, iY );
		iX += iDX;
	}

	/* draw right side of bar */
	iV_DrawImage( IntImages, iImageIDRight, iXRight, iY );
}


void intDisplayNumber(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_LABEL		*Label = (W_LABEL*)psWidget;
	UDWORD		x = Label->x + xOffset;
	UDWORD		y = Label->y + yOffset;
	UDWORD		Quantity = 1;

	//Quantity depends on the factory
	if (Label->pUserData != NULL)
	{
		STRUCTURE	*psStruct = (STRUCTURE *)Label->pUserData;
		FACTORY		*psFactory = (FACTORY *)psStruct->pFunctionality;

		if (psFactory && !psStruct->died)
		{
			Quantity = psFactory->productionLoops;
		}
	}

	if(Quantity >= STAT_SLDSTOPS)
	{
		iV_DrawImage(IntImages,IMAGE_SLIDER_INFINITY,x+4,y);
	}
	else
	{
		snprintf(Label->aText, sizeof(Label->aText), "%02u", Quantity);

		for (int i = 0; Label->aText[i]; ++i)
		{
			iV_DrawImage(IntImages, (UWORD)(IMAGE_0 + (Label->aText[i] - '0')), x, y);
			x += iV_GetImageWidth(IntImages, (UWORD)(IMAGE_0 + (Label->aText[i]-'0'))) + 1;
		}
	}
}


// Initialise all the surfaces,graphics etc. used by the interface.
//
void intInitialiseGraphics(void)
{
	// Initialise any bitmaps used by the interface.
	imageInitBitmaps();

	// Initialise button surfaces.
	InitialiseButtonData();
}


// Free up all surfaces,graphics etc. used by the interface.
//
void interfaceDeleteGraphics(void)
{
	unsigned int i;

	// Setting all these pointers may, or may not be necessary, but it surely is safe
	// Look above (near the declaration of .*Surfaces) for a detailed description of why this .*Surfaces stuff is bad
	for(i = 0; i < NUM_OBJECTSURFACES; ++i)
	{
		free(ObjectSurfaces[i].Buffer);
		ObjectSurfaces[i].Buffer = NULL;
		iV_SurfaceDestroy(ObjectSurfaces[i].Surface);
	}

	for(i = 0; i < NUM_TOPICSURFACES; ++i)
	{
		free(TopicSurfaces[i].Buffer);
		TopicSurfaces[i].Buffer = NULL;
		iV_SurfaceDestroy(TopicSurfaces[i].Surface);
	}

	for(i = 0; i < NUM_STATSURFACES; ++i)
	{
		free(StatSurfaces[i].Buffer);
		StatSurfaces[i].Buffer = NULL;
		iV_SurfaceDestroy(StatSurfaces[i].Surface);
	}

	for(i = 0; i < NUM_SYSTEM0SURFACES; ++i)
	{
		free(System0Surfaces[i].Buffer);
		System0Surfaces[i].Buffer = NULL;
		iV_SurfaceDestroy(System0Surfaces[i].Surface);
	}
}


// Initialise data for interface buttons.
//
void InitialiseButtonData(void)
{
	// Allocate surfaces for rendered buttons.
	UDWORD Width = (iV_GetImageWidth(IntImages,IMAGE_BUT0_UP)+3) & 0xfffffffc;	// Ensure width is whole number of dwords.
	UDWORD Height = iV_GetImageHeight(IntImages,IMAGE_BUT0_UP);
	UDWORD WidthTopic = (iV_GetImageWidth(IntImages,IMAGE_BUTB0_UP)+3) & 0xfffffffc;	// Ensure width is whole number of dwords.
	UDWORD HeightTopic = iV_GetImageHeight(IntImages,IMAGE_BUTB0_UP);
	UDWORD i;

	for(i=0; i<NUM_OBJECTSURFACES; i++) {
		ObjectSurfaces[i].Buffer = (UBYTE*)malloc(Width*Height);
		ASSERT( ObjectSurfaces[i].Buffer!=NULL,"intInitialise : Failed to allocate Object surface" );
		ObjectSurfaces[i].Surface = iV_SurfaceCreate(Width,Height);
		ASSERT( ObjectSurfaces[i].Surface!=NULL,"intInitialise : Failed to create Object surface" );
	}

	for(i=0; i<NUM_OBJECTBUFFERS; i++) {
		RENDERBUTTON_NOTINUSE(&ObjectBuffers[i]);
		ObjectBuffers[i].ButSurf = &ObjectSurfaces[i%NUM_OBJECTSURFACES];
	}

	for(i=0; i<NUM_SYSTEM0SURFACES; i++) {
		System0Surfaces[i].Buffer = (UBYTE*)malloc(Width*Height);
		ASSERT( System0Surfaces[i].Buffer!=NULL,"intInitialise : Failed to allocate System0 surface" );
		System0Surfaces[i].Surface = iV_SurfaceCreate(Width,Height);
		ASSERT( System0Surfaces[i].Surface!=NULL,"intInitialise : Failed to create System0 surface" );
	}

	for(i=0; i<NUM_SYSTEM0BUFFERS; i++) {
		RENDERBUTTON_NOTINUSE(&System0Buffers[i]);
		System0Buffers[i].ButSurf = &System0Surfaces[i%NUM_SYSTEM0SURFACES];
	}

	for(i=0; i<NUM_TOPICSURFACES; i++) {
		TopicSurfaces[i].Buffer = (UBYTE*)malloc(WidthTopic*HeightTopic);
		ASSERT( TopicSurfaces[i].Buffer!=NULL,"intInitialise : Failed to allocate Topic surface" );
		TopicSurfaces[i].Surface = iV_SurfaceCreate(WidthTopic,HeightTopic);
		ASSERT( TopicSurfaces[i].Surface!=NULL,"intInitialise : Failed to create Topic surface" );
	}

	for(i=0; i<NUM_TOPICBUFFERS; i++) {
		RENDERBUTTON_NOTINUSE(&TopicBuffers[i]);
		TopicBuffers[i].ButSurf = &TopicSurfaces[i%NUM_TOPICSURFACES];
	}

	for(i=0; i<NUM_STATSURFACES; i++) {
		StatSurfaces[i].Buffer = (UBYTE*)malloc(Width*Height);
		ASSERT( StatSurfaces[i].Buffer!=NULL,"intInitialise : Failed to allocate Stats surface" );
		StatSurfaces[i].Surface = iV_SurfaceCreate(Width,Height);
		ASSERT( StatSurfaces[i].Surface!=NULL,"intInitialise : Failed to create Stat surface" );
	}

	for(i=0; i<NUM_STATBUFFERS; i++) {
		RENDERBUTTON_NOTINUSE(&StatBuffers[i]);
		StatBuffers[i].ButSurf = &StatSurfaces[i%NUM_STATSURFACES];
	}
}


void RefreshObjectButtons(void)
{
	UDWORD i;

	for(i=0; i<NUM_OBJECTBUFFERS; i++) {
		RENDERBUTTON_NOTINITIALISED(&ObjectBuffers[i]);
	}
}

void RefreshSystem0Buttons(void)
{
	UDWORD i;

	for(i=0; i<NUM_SYSTEM0BUFFERS; i++) {
		RENDERBUTTON_NOTINITIALISED(&System0Buffers[i]);
	}
}
void RefreshTopicButtons(void)
{
	UDWORD i;

	for(i=0; i<NUM_TOPICBUFFERS; i++) {
		RENDERBUTTON_NOTINITIALISED(&TopicBuffers[i]);
	}
}


void RefreshStatsButtons(void)
{
	UDWORD i;

	for(i=0; i<NUM_STATBUFFERS; i++) {
		RENDERBUTTON_NOTINITIALISED(&StatBuffers[i]);
	}
}


void ClearObjectBuffers(void)
{
	UDWORD i;

	for(i=0; i<NUM_OBJECTBUFFERS; i++) {
		ClearObjectButtonBuffer(i);
	}
}

void ClearTopicBuffers(void)
{
	UDWORD i;

	for(i=0; i<NUM_TOPICBUFFERS; i++) {
		ClearTopicButtonBuffer(i);
	}
}

void ClearObjectButtonBuffer(SDWORD BufferID)
{
	RENDERBUTTON_NOTINITIALISED(&ObjectBuffers[BufferID]);	//  what have I done
	RENDERBUTTON_NOTINUSE(&ObjectBuffers[BufferID]);
	ObjectBuffers[BufferID].Data = NULL;
	ObjectBuffers[BufferID].Data2 = NULL;
	ObjectBuffers[BufferID].ImdRotation = DEFAULT_BUTTON_ROTATION;
}

void ClearTopicButtonBuffer(SDWORD BufferID)
{
	RENDERBUTTON_NOTINITIALISED(&TopicBuffers[BufferID]);	//  what have I done
	RENDERBUTTON_NOTINUSE(&TopicBuffers[BufferID]);
	TopicBuffers[BufferID].Data = NULL;
	TopicBuffers[BufferID].Data2 = NULL;
	TopicBuffers[BufferID].ImdRotation = DEFAULT_BUTTON_ROTATION;
}

SDWORD GetObjectBuffer(void)
{
	SDWORD i;

	for(i=0; i<NUM_OBJECTBUFFERS; i++) {
		if( IsBufferInUse(&ObjectBuffers[i])==false )
		{
			return i;
		}
	}

	return -1;
}

void ClearStatBuffers(void)
{
	UDWORD i;

	for(i=0; i<NUM_STATBUFFERS; i++) {
		RENDERBUTTON_NOTINITIALISED(&StatBuffers[i]);	//  what have I done
		RENDERBUTTON_NOTINUSE(&StatBuffers[i]);
		StatBuffers[i].Data = NULL;
		StatBuffers[i].ImdRotation = DEFAULT_BUTTON_ROTATION;
	}
}

SDWORD GetStatBuffer(void)
{
	SDWORD i;

	for(i=0; i<NUM_STATBUFFERS; i++) {
		if( IsBufferInUse(&StatBuffers[i])==false )
 		{
	 		return i;
		}
	}

	return -1;
}

/*these have been set up for the Transporter - the design screen DOESN'T use them
NB On the PC there are 80!!!!!*/
void ClearSystem0Buffers(void)
{
	UDWORD i;

	for(i=0; i<NUM_SYSTEM0BUFFERS; i++) {
		ClearSystem0ButtonBuffer(i);
	}
}

void ClearSystem0ButtonBuffer(SDWORD BufferID)
{
	RENDERBUTTON_NOTINITIALISED(&System0Buffers[BufferID]);	//  what have I done
	RENDERBUTTON_NOTINUSE(&System0Buffers[BufferID]);
	System0Buffers[BufferID].Data = NULL;
	System0Buffers[BufferID].Data2 = NULL;
	System0Buffers[BufferID].ImdRotation = DEFAULT_BUTTON_ROTATION;
}

SDWORD GetSystem0Buffer(void)
{
	SDWORD i;

	for(i=0; i<NUM_SYSTEM0BUFFERS; i++)
	{
		if( IsBufferInUse(&System0Buffers[i])==false )
		{
			return i;
		}
	}

	return -1;
}


static UWORD ButXPos = 0;
static UWORD ButYPos = 0;
static UWORD ButWidth, ButHeight;

void OpenButtonRender(UWORD XPos,UWORD YPos,UWORD Width,UWORD Height)
{
	ButXPos = XPos;
	ButYPos = YPos;
	ButWidth = Width;
	ButHeight = Height;
}

void CloseButtonRender(void)
{
}


// Clear a button bitmap. ( copy the button background ).
//
void ClearButton(bool Down,UDWORD Size, UDWORD buttonType)
{
	if(Down)
	{
		iV_DrawImage(IntImages,(UWORD)(IMAGE_BUT0_DOWN+(buttonType*2)),ButXPos,ButYPos);
	}
	else
	{
		iV_DrawImage(IntImages,(UWORD)(IMAGE_BUT0_UP+(buttonType*2)),ButXPos,ButYPos);
	}
}

// Create a button by rendering an IMD object into it.
//
void CreateIMDButton(IMAGEFILE *ImageFile, UWORD ImageID, void *Object, UDWORD Player, RENDERED_BUTTON *Buffer, bool Down, UDWORD IMDType, UDWORD buttonType)
{
	UDWORD Size;
	Vector3i Rotation, Position, NullVector;
	UDWORD ox,oy;
	UDWORD Radius;
	UDWORD basePlateSize;
	SDWORD scale;

	if(Down)
	{
		ox = oy = 2;
	}
	else
	{
		ox = oy = 0;
	}

	if ((IMDType == IMDTYPE_DROID) || (IMDType == IMDTYPE_DROIDTEMPLATE))
	{	// The case where we have to render a composite droid.
		if(Down)
		{
			//the top button is smaller than the bottom button
			if (buttonType == TOPBUTTON)
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUT0_DOWN)/2) + ButtonDrawXOffset + 2,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUT0_DOWN)/2) + 2 + 8 + ButtonDrawYOffset);
			}
			else
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUTB0_DOWN)/2) + ButtonDrawXOffset + 2,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUTB0_DOWN)/2) + 2 + 12 + ButtonDrawYOffset);
			}
		}
		else
		{
			//the top button is smaller than the bottom button
			if (buttonType == TOPBUTTON)
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUT0_UP)/2) + ButtonDrawXOffset,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUT0_UP)/2) + 8  + ButtonDrawYOffset);
			}
			else
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUT0_UP)/2) + ButtonDrawXOffset,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUTB0_UP)/2) + 12  + ButtonDrawYOffset);
			}
		}

		if (IMDType == IMDTYPE_DROID)
		{
			Radius = getComponentDroidRadius((DROID*)Object);
		}
		else
		{
			Radius = getComponentDroidTemplateRadius((DROID_TEMPLATE*)Object);
		}

		Size = 2;
		scale = DROID_BUT_SCALE;
		ASSERT( Radius <= 128,"create PIE button big component found" );

		ClearButton(Down, Size, buttonType);

		Rotation.x = -30;
		Rotation.y = (UDWORD) Buffer->ImdRotation;
		Rotation.z = 0;

		NullVector.x = 0;
		NullVector.y = 0;
		NullVector.z = 0;

		if(IMDType == IMDTYPE_DROID)
		{
			if (((DROID*)Object)->droidType == DROID_TRANSPORTER || ((DROID*)Object)->droidType == DROID_SUPERTRANSPORTER)
			{
				Position.x = 0;
				Position.y = 0;//BUT_TRANSPORTER_ALT;
				Position.z = BUTTON_DEPTH;
				if (((DROID*)Object)->droidType == DROID_TRANSPORTER)
				{
					scale = DROID_BUT_SCALE/2;
				}
				else
				{
					scale = DROID_BUT_SCALE/3;
				}
			}
			else
			{
				Position.x = Position.y = 0;
				Position.z = BUTTON_DEPTH;
			}
		}
		else//(IMDType == IMDTYPE_DROIDTEMPLATE)
		{
			if (((DROID_TEMPLATE*)Object)->droidType == DROID_TRANSPORTER || ((DROID_TEMPLATE*)Object)->droidType == DROID_SUPERTRANSPORTER)
			{
				Position.x = 0;
				Position.y = 0;//BUT_TRANSPORTER_ALT;
				Position.z = BUTTON_DEPTH;
				if (((DROID_TEMPLATE*)Object)->droidType == DROID_TRANSPORTER)
				{
					scale = DROID_BUT_SCALE/2;
				}
				else
				{
					scale = DROID_BUT_SCALE/3;
				}
			}
			else
			{
				Position.x = Position.y = 0;
				Position.z = BUTTON_DEPTH;
			}
		}

		//lefthand display droid buttons
		if (IMDType == IMDTYPE_DROID)
		{
			displayComponentButtonObject((DROID*)Object,&Rotation,&Position,true, scale);
		}
		else
		{
			displayComponentButtonTemplate((DROID_TEMPLATE*)Object,&Rotation,&Position,true, scale);
		}
	}
	else
	{	// Just drawing a single IMD.

		if(Down)
		{
			if (buttonType == TOPBUTTON)
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUT0_DOWN)/2) + ButtonDrawXOffset + 2,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUT0_DOWN)/2) + 2 + 8 + ButtonDrawYOffset);
			}
			else
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUTB0_DOWN)/2) + ButtonDrawXOffset + 2,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUTB0_DOWN)/2) + 2 + 12 + ButtonDrawYOffset);
			}
		}
		else
		{
			if (buttonType == TOPBUTTON)
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUT0_UP)/2) + ButtonDrawXOffset,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUT0_UP)/2) + 8  + ButtonDrawYOffset);
			}
			else
			{
				pie_SetGeometricOffset(
					(ButXPos + iV_GetImageWidth(IntImages,IMAGE_BUTB0_UP)/2) + ButtonDrawXOffset,
					(ButYPos + iV_GetImageHeight(IntImages,IMAGE_BUTB0_UP)/2) + 12  + ButtonDrawYOffset);
			}
		}

	// Decide which button grid size to use.
		if(IMDType == IMDTYPE_COMPONENT)
		{
			Radius = getComponentRadius((BASE_STATS*)Object);
			Size = 2;//small structure
			scale = rescaleButtonObject(Radius, COMP_BUT_SCALE, COMPONENT_RADIUS);
			//scale = COMP_BUT_SCALE;
			//ASSERT( Radius <= OBJECT_RADIUS,"Object too big for button - %s",
			//		((BASE_STATS*)Object)->pName );
			// NOTE: The Super transport is huge, and is considered a component type, so refit it to inside the button.
			const char * const name = ((BASE_STATS*)Object)->pName;
			if (!strcmp(name, "SuperTransportBody"))
			{
				scale *= .4;
			}
			else if (!strcmp(name, "TransporterBody"))
			{
				scale *= .6;
			}
		}
		else if(IMDType == IMDTYPE_RESEARCH)
		{
			Radius = getResearchRadius((BASE_STATS*)Object);
			if (Radius <= 100)
			{
				Size = 2;//small structure
				scale = rescaleButtonObject(Radius, COMP_BUT_SCALE, COMPONENT_RADIUS);
				//scale = COMP_BUT_SCALE;
			}
			else if (Radius <= 128)
			{
				Size = 2;//small structure
				scale = SMALL_STRUCT_SCALE;
			}
			else if (Radius <= 256)
			{
				Size = 1;//med structure
				scale = MED_STRUCT_SCALE;
			}
			else
			{
				Size = 0;
				scale = LARGE_STRUCT_SCALE;
			}
		}
		else if (IMDType == IMDTYPE_STRUCTURE)
		{
			basePlateSize = getStructureSizeMax((STRUCTURE*)Object);
			if (basePlateSize == 1)
			{
				Size = 2;//small structure
				scale = SMALL_STRUCT_SCALE;
			}
			else if (basePlateSize == 2)
			{
				Size = 1;//med structure
				scale = MED_STRUCT_SCALE;
			}
			else
			{
				Size = 0;
				scale = LARGE_STRUCT_SCALE;
			}
		}
		else if (IMDType == IMDTYPE_STRUCTURESTAT)
		{
			basePlateSize = getStructureStatSizeMax((STRUCTURE_STATS*)Object);
			if (basePlateSize == 1)
			{
				Size = 2;//small structure
				scale = SMALL_STRUCT_SCALE;
			}
			else if (basePlateSize == 2)
			{
				Size = 1;//med structure
				scale = MED_STRUCT_SCALE;
			}
			else
			{
				Size = 0;
				scale = LARGE_STRUCT_SCALE;
			}
		}
		else
		{

			Radius = ((iIMDShape*)Object)->sradius;

			if (Radius <= 128)
			{
				Size = 2;//small structure
				scale = SMALL_STRUCT_SCALE;
			}
			else if (Radius <= 256)
			{
				Size = 1;//med structure
				scale = MED_STRUCT_SCALE;
			}
			else
			{
				Size = 0;
				scale = LARGE_STRUCT_SCALE;
			}
		}

		ClearButton(Down,Size, buttonType);

		Rotation.x = -30;
		Rotation.y = (UWORD ) Buffer->ImdRotation;
		Rotation.z = 0;

		NullVector.x = 0;
		NullVector.y = 0;
		NullVector.z = 0;

		Position.x = 0;
		Position.y = 0;
		Position.z = BUTTON_DEPTH; //was 		Position.z = Radius*30;

		if (ImageFile)
		{
			iV_DrawImage(ImageFile,ImageID,ButXPos+ox,ButYPos+oy);
		}

		pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);

		/* all non droid buttons */
		if (IMDType == IMDTYPE_COMPONENT)
		{
			displayComponentButton((BASE_STATS*)Object,&Rotation,&Position,true, scale);
		}
		else if (IMDType == IMDTYPE_RESEARCH)
		{
			displayResearchButton((BASE_STATS*)Object,&Rotation,&Position,true, scale);
		}
		else if (IMDType == IMDTYPE_STRUCTURE)
		{
			displayStructureButton((STRUCTURE*)Object,&Rotation,&Position,true, scale);
		}
		else if (IMDType == IMDTYPE_STRUCTURESTAT)
		{
			displayStructureStatButton((STRUCTURE_STATS*)Object, &Rotation, &Position, true, scale);
		}
		else
		{
			displayIMDButton((iIMDShape*)Object,&Rotation,&Position,true, scale);
		}

		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	}
}


// Create a button by rendering an image into it.
//
void CreateImageButton(IMAGEFILE *ImageFile,UWORD ImageID,RENDERED_BUTTON *Buffer,bool Down, UDWORD buttonType)
{
	ClearButton(Down,0, buttonType);
	iV_DrawImage(ImageFile, ImageID, ButXPos, ButYPos);
}


// Create a blank button.
//
void CreateBlankButton(RENDERED_BUTTON *Buffer,bool Down, UDWORD buttonType)
{
	UDWORD ox,oy;

	if(Down) {
		ox = oy = 1;
	} else {
		ox = oy = 0;
	}

	ClearButton(Down,0, buttonType);

	// Draw a question mark, bit of quick hack this.
	iV_DrawImage(IntImages,IMAGE_QUESTION_MARK,ButXPos+ox+10,ButYPos+oy+3);
}

// Returns true if the droid is currently demolishing something or moving to demolish something.
//
bool DroidIsDemolishing(DROID *Droid)
{
	BASE_STATS	*Stats;
	UDWORD x,y;

	if (!(droidType(Droid) == DROID_CONSTRUCT ||
		droidType(Droid) == DROID_CYBORG_CONSTRUCT))
	{
		return false;
	}

	if(orderStateStatsLoc(Droid, DORDER_DEMOLISH,&Stats,&x,&y)) // Moving to demolish location?
	{
		return true;
	}
	else if (orderStateObj(Droid, DORDER_DEMOLISH)) // Is demolishing?
	{
		return true;
	}

	return false;
}

// Returns true if the droid is currently repairing another droid.
bool DroidIsRepairing(DROID *Droid)
{
	if (!(droidType(Droid) == DROID_REPAIR
	   || droidType(Droid) == DROID_CYBORG_REPAIR))
	{
		return false;
	}

	if (orderStateObj(Droid, DORDER_DROIDREPAIR))
	{
		return true;
	}

	return false;
}

// Returns true if the droid is currently building something.
//
bool DroidIsBuilding(DROID *Droid)
{
	BASE_STATS	*Stats;
	UDWORD x,y;

    if (!(droidType(Droid) == DROID_CONSTRUCT ||
        droidType(Droid) == DROID_CYBORG_CONSTRUCT))
    {
        return false;
    }

	if (orderStateStatsLoc(Droid, DORDER_BUILD, &Stats, &x, &y))
	{
		// Moving to build location?
		return false;
	}
	else if (orderStateObj(Droid, DORDER_BUILD)
	      || orderStateObj(Droid, DORDER_HELPBUILD)) // Is building or helping?
	{
		return true;
	}

	return false;
}


// Returns true if the droid has been ordered build something ( but has'nt started yet )
//
bool DroidGoingToBuild(DROID *Droid)
{
	BASE_STATS	*Stats;
	UDWORD x,y;

    if (!(droidType(Droid) == DROID_CONSTRUCT ||
        droidType(Droid) == DROID_CYBORG_CONSTRUCT))
    {
        return false;
    }

	if(orderStateStatsLoc(Droid, DORDER_BUILD,&Stats,&x,&y)) {	// Moving to build location?
		return true;
	}

	return false;
}


// Get the structure for a structure which a droid is currently building.
//
STRUCTURE *DroidGetBuildStructure(DROID *Droid)
{
	BASE_OBJECT *Structure = NULL;

	if (orderStateObj(Droid, DORDER_BUILD))
	{
		Structure = orderStateObj(Droid, DORDER_HELPBUILD);
	}

	return (STRUCTURE*)Structure;
}

// Get the first factory assigned to a command droid
STRUCTURE *droidGetCommandFactory(DROID *psDroid)
{
	SDWORD		inc;
	STRUCTURE	*psCurr;

	for(inc = 0; inc < MAX_FACTORY; inc++)
	{
		if ( psDroid->secondaryOrder & (1 << (inc + DSS_ASSPROD_SHIFT)) )
		{
			// found an assigned factory - look for it in the lists
			for (psCurr = apsStructLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
			{
				if ( (psCurr->pStructureType->type == REF_FACTORY) &&
					 ( ((FACTORY *)psCurr->pFunctionality)->
								psAssemblyPoint->factoryInc == inc ) )
				{
					return psCurr;
				}
			}
		}
		if ( psDroid->secondaryOrder & (1 << (inc + DSS_ASSPROD_CYBORG_SHIFT)) )
		{
			// found an assigned factory - look for it in the lists
			for (psCurr = apsStructLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
			{
				if ( (psCurr->pStructureType->type == REF_CYBORG_FACTORY) &&
					 ( ((FACTORY *)psCurr->pFunctionality)->
								psAssemblyPoint->factoryInc == inc ) )
				{
					return psCurr;
				}
			}
		}
		if ( psDroid->secondaryOrder & (1 << (inc + DSS_ASSPROD_VTOL_SHIFT)) )
		{
			// found an assigned factory - look for it in the lists
			for (psCurr = apsStructLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
			{
				if ( (psCurr->pStructureType->type == REF_VTOL_FACTORY) &&
					 ( ((FACTORY *)psCurr->pFunctionality)->
								psAssemblyPoint->factoryInc == inc ) )
				{
					return psCurr;
				}
			}
		}
	}

	return NULL;
}

// Get the stats for a structure which a droid is going to ( but not yet ) building.
//
BASE_STATS *DroidGetBuildStats(DROID *Droid)
{
	BASE_STATS *Stats;
	UDWORD x,y;

	if(orderStateStatsLoc(Droid, DORDER_BUILD,&Stats,&x,&y)) {	// Moving to build location?
		return Stats;
	}

	return NULL;
}

iIMDShape *DroidGetIMD(DROID *Droid)
{
	return Droid->sDisplay.imd;
}

template<typename Functionality>
static inline bool _structureIsManufacturingPending(Functionality const &functionality)
{
	if (functionality.statusPending != FACTORY_NOTHING_PENDING)
	{
		return functionality.statusPending == FACTORY_START_PENDING || functionality.statusPending == FACTORY_HOLD_PENDING;
	}
	return functionality.psSubject != NULL;
}

bool StructureIsManufacturingPending(STRUCTURE *structure)
{
	switch (structure->pStructureType->type)
	{
		case REF_FACTORY:
		case REF_CYBORG_FACTORY:
		case REF_VTOL_FACTORY:
			return _structureIsManufacturingPending(structure->pFunctionality->factory);
		default:
			return false;
	}
}

FACTORY *StructureGetFactory(STRUCTURE *Structure)
{
	return &Structure->pFunctionality->factory;
}

bool structureIsResearchingPending(STRUCTURE *structure)
{
	return structure->pStructureType->type == REF_RESEARCH && _structureIsManufacturingPending(structure->pFunctionality->researchFacility);
}

template<typename Functionality>
static inline bool structureIsOnHoldPending(Functionality const &functionality)
{
	if (functionality.statusPending != FACTORY_NOTHING_PENDING)
	{
		return functionality.statusPending == FACTORY_HOLD_PENDING;
	}
	return functionality.timeStartHold != 0;
}

bool StructureIsOnHoldPending(STRUCTURE *structure)
{
	switch (structure->pStructureType->type)
	{
		case REF_FACTORY:
		case REF_CYBORG_FACTORY:
		case REF_VTOL_FACTORY:
			return structureIsOnHoldPending(structure->pFunctionality->factory);
		case REF_RESEARCH:
			return structureIsOnHoldPending(structure->pFunctionality->researchFacility);
		default:
			ASSERT(false, "Huh?");
			return false;
	}
}

RESEARCH_FACILITY *StructureGetResearch(STRUCTURE *Structure)
{
	return &Structure->pFunctionality->researchFacility;
}


DROID_TEMPLATE *FactoryGetTemplate(FACTORY *Factory)
{
	if (Factory->psSubjectPending != NULL)
	{
		return (DROID_TEMPLATE *)Factory->psSubjectPending;
	}

	return (DROID_TEMPLATE *)Factory->psSubject;
}

bool StatIsStructure(BASE_STATS const *Stat)
{
	return (Stat->ref >= REF_STRUCTURE_START && Stat->ref <
				REF_STRUCTURE_START + REF_RANGE);
}

bool StatIsFeature(BASE_STATS const *Stat)
{
	return (Stat->ref >= REF_FEATURE_START && Stat->ref <
				REF_FEATURE_START + REF_RANGE);
}

iIMDShape *StatGetStructureIMD(BASE_STATS *Stat,UDWORD Player)
{
	(void)Player;
	return ((STRUCTURE_STATS*)Stat)->pIMD[0];
}

bool StatIsTemplate(BASE_STATS *Stat)
{
	return (Stat->ref >= REF_TEMPLATE_START &&
				 Stat->ref < REF_TEMPLATE_START + REF_RANGE);
}

SDWORD StatIsComponent(BASE_STATS *Stat)
{
	if(Stat->ref >= REF_BODY_START &&
				 Stat->ref < REF_BODY_START + REF_RANGE) {
		return COMP_BODY;
	}

	if(Stat->ref >= REF_BRAIN_START &&
				 Stat->ref < REF_BRAIN_START + REF_RANGE) {
		return COMP_BRAIN;
	}

	if(Stat->ref >= REF_PROPULSION_START &&
				 Stat->ref < REF_PROPULSION_START + REF_RANGE) {
		return COMP_PROPULSION;
	}

	if(Stat->ref >= REF_WEAPON_START &&
				 Stat->ref < REF_WEAPON_START + REF_RANGE) {
		return COMP_WEAPON;
	}

	if(Stat->ref >= REF_SENSOR_START &&
				 Stat->ref < REF_SENSOR_START + REF_RANGE) {
		return COMP_SENSOR;
	}

	if(Stat->ref >= REF_ECM_START &&
				 Stat->ref < REF_ECM_START + REF_RANGE) {
		return COMP_ECM;
	}

	if(Stat->ref >= REF_CONSTRUCT_START &&
				 Stat->ref < REF_CONSTRUCT_START + REF_RANGE) {
		return COMP_CONSTRUCT;
	}

	if(Stat->ref >= REF_REPAIR_START &&
				 Stat->ref < REF_REPAIR_START + REF_RANGE) {
		return COMP_REPAIRUNIT;
	}

	return COMP_UNKNOWN;
}

bool StatGetComponentIMD(BASE_STATS *Stat, SDWORD compID,iIMDShape **CompIMD,iIMDShape **MountIMD)
{
	WEAPON_STATS		*psWStat;

	*CompIMD = NULL;
	*MountIMD = NULL;

	switch (compID)
	{
	case COMP_BODY:
		*CompIMD = ((COMPONENT_STATS *)Stat)->pIMD;
		return true;

	case COMP_BRAIN:
		psWStat = ((BRAIN_STATS *)Stat)->psWeaponStat;
		*MountIMD = psWStat->pMountGraphic;
		*CompIMD = psWStat->pIMD;
		return true;

	case COMP_WEAPON:
		*MountIMD = ((WEAPON_STATS*)Stat)->pMountGraphic;
		*CompIMD = ((COMPONENT_STATS *)Stat)->pIMD;
		return true;

	case COMP_SENSOR:
		*MountIMD = ((SENSOR_STATS*)Stat)->pMountGraphic;
		*CompIMD = ((COMPONENT_STATS *)Stat)->pIMD;
		return true;

	case COMP_ECM:
		*MountIMD = ((ECM_STATS*)Stat)->pMountGraphic;
		*CompIMD = ((COMPONENT_STATS *)Stat)->pIMD;
		return true;

	case COMP_CONSTRUCT:
		*MountIMD = ((CONSTRUCT_STATS*)Stat)->pMountGraphic;
		*CompIMD = ((COMPONENT_STATS *)Stat)->pIMD;
		return true;

	case COMP_PROPULSION:
		*CompIMD = ((COMPONENT_STATS *)Stat)->pIMD;
		return true;

	case COMP_REPAIRUNIT:
		*MountIMD = ((REPAIR_STATS*)Stat)->pMountGraphic;
		*CompIMD = ((COMPONENT_STATS *)Stat)->pIMD;
		return true;

	default:
		//COMP_UNKNOWN should be an error
		ASSERT( false, "StatGetComponent : Unknown component" );
	}

	return false;
}


bool StatIsResearch(BASE_STATS *Stat)
{
	return (Stat->ref >= REF_RESEARCH_START && Stat->ref <
				REF_RESEARCH_START + REF_RANGE);
}

void StatGetResearchImage(BASE_STATS *psStat, SDWORD *Image, iIMDShape **Shape,
                          BASE_STATS **ppGraphicData, bool drawTechIcon)
{
	*Image = -1;
	if (drawTechIcon)
	{
		if (((RESEARCH *)psStat)->iconID != NO_RESEARCH_ICON)
		{
			*Image = ((RESEARCH *)psStat)->iconID;
		}
	}
    //if the research has a Stat associated with it - use this as display in the button
    if (((RESEARCH *)psStat)->psStat)
    {
        *ppGraphicData = ((RESEARCH *)psStat)->psStat;
        //make sure the IMDShape is initialised
        *Shape = NULL;
    }
    else
    {
        //no stat so just just the IMD associated with the research
	    *Shape = ((RESEARCH *)psStat)->pIMD;
        //make sure the stat is initialised
        *ppGraphicData = NULL;
    }
}

static void intDisplayBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, bool isPowerBar)
{
	W_BARGRAPH *BarGraph = (W_BARGRAPH *)psWidget;
	char szVal[30];
	char const *szCheckWidth = "00000";
	int x0 = xOffset + BarGraph->x;
	int y0 = yOffset + BarGraph->y;
	int arbitaryOffset = 3;
	int iX, iY;
	int barWidth = 100, width;
	int i, precisionFactor = 1, value;

	if (isPowerBar)
	{
		//draw the background image
		iV_DrawImage(IntImages, IMAGE_DES_POWERBAR_LEFT, x0, y0);
		iV_DrawImage(IntImages, IMAGE_DES_POWERBAR_RIGHT, x0 + psWidget->width - iV_GetImageWidth(IntImages, IMAGE_DES_POWERBAR_RIGHT), y0);
	}

	// Arbitrary increment for the position of the bars
	x0 += arbitaryOffset;
	y0 += arbitaryOffset;

	/* indent to allow text value */
	iX = x0 + iV_GetTextWidth(szCheckWidth);
	iY = y0 + (iV_GetImageHeight(IntImages, IMAGE_DES_STATSCURR) - iV_GetTextLineSize())/2 - iV_GetTextAboveBase();

	if (isPowerBar)
	{
		// Adjust the width based on the text drawn
		barWidth = BarGraph->width - (iX - x0 + arbitaryOffset);
	}

	//draw current value section
	width = MIN(BarGraph->majorSize * barWidth / 100, barWidth);
	iV_DrawImageRect(IntImages, IMAGE_DES_STATSCURR, iX, y0, width, iV_GetImageHeight(IntImages, IMAGE_DES_STATSCURR));

	/* draw text value */
	for (i = 0; i < BarGraph->precision; ++i)
	{
		precisionFactor *= 10;
	}
	value = (BarGraph->iOriginal * precisionFactor + BarGraph->denominator/2) / BarGraph->denominator;
	sprintf(szVal, "%d%s%.*d", value/precisionFactor, precisionFactor == 1? "" : ".", BarGraph->precision, value%precisionFactor);
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	iV_DrawText(szVal, x0, iY);

	//draw the comparison value - only if not zero
	if (BarGraph->minorSize != 0)
	{
		width = MIN(BarGraph->minorSize * barWidth / 100, barWidth);
		iV_DrawImage(IntImages, IMAGE_DES_STATSCOMP, iX + width, y0 - 1);
	}
}

/* Draws a stats bar for the design screen */
void intDisplayStatsBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	intDisplayBar(psWidget, xOffset, yOffset, false);
}

/* Draws a Template Power Bar for the Design Screen */
void intDisplayDesignPowerBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	intDisplayBar(psWidget, xOffset, yOffset, true);
}


// Widget callback function to play an audio track.
//
#define WIDGETBEEPGAP (200)	// 200 milliseconds between each beep please
void WidgetAudioCallback(int AudioID)
{
	static	SDWORD LastTimeAudio;
	if(AudioID >= 0) {

		SDWORD TimeSinceLastWidgetBeep;

		// Don't allow a widget beep if one was made in the last WIDGETBEEPGAP milliseconds
		// This stops double beeps happening (which seems to happen all the time)
		TimeSinceLastWidgetBeep=realTime-LastTimeAudio;
		if (TimeSinceLastWidgetBeep<0 || TimeSinceLastWidgetBeep>WIDGETBEEPGAP)
		{
			LastTimeAudio=realTime;
			audio_PlayTrack(AudioID);
		}
	}
}


// Widget callback to display a contents button for the Transporter
void intDisplayTransportButton(WIDGET *psWidget, UDWORD xOffset,
						  UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_CLICKFORM		*Form = (W_CLICKFORM*)psWidget;
	bool			Down;
	bool			Hilight = false;
	RENDERED_BUTTON		*Buffer = (RENDERED_BUTTON*)Form->pUserData;
	DROID			*psDroid = NULL;
	UDWORD			gfxId;

	OpenButtonRender((UWORD)(xOffset+Form->x), (UWORD)(yOffset+Form->y),(UWORD)Form->width,
		(UWORD)Form->height);

	Down = Form->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK);

	// Allocate this outside of the if so the rank icons are always draw
	psDroid = (DROID*)Buffer->Data;
	// There should always be a droid associated with the button
	ASSERT(psDroid != NULL, "Invalid droid pointer");

		Hilight = Form->state & WCLICK_HILITE;

		if(Hilight)
		{
			Buffer->ImdRotation += realTimeAdjustedAverage(BUTTONOBJ_ROTSPEED);
		}

		Hilight = formIsHilite(Form);

		Buffer->State = Form->state;

		if (psDroid)
		{
			RenderToButton(NULL,0,psDroid,psDroid->player,Buffer,Down,IMDTYPE_DROID,TOPBUTTON);
		}
		else
		{
			RenderBlankToButton(Buffer,Down,TOPBUTTON);
		}
		RENDERBUTTON_INITIALISED(Buffer);

	CloseButtonRender();

	if (Hilight)
	{
		iV_DrawImage(IntImages,IMAGE_BUT_HILITE,xOffset+Form->x,yOffset+Form->y);
	}

	if (psDroid && missionForReInforcements())
	{
		// Add the experience level for each droid
		gfxId = getDroidRankGraphic(psDroid);
		if (gfxId != UDWORD_MAX)
		{
			/* Render the rank graphic at the correct location */
			iV_DrawImage(IntImages, (UWORD)gfxId, xOffset+Form->x + 50, yOffset+Form->y + 30);
		}
	}
}

/* Draws blips on radar to represent Proximity Display and damaged structures */
void drawRadarBlips(int radarX, int radarY, float pixSizeH, float pixSizeV)
{
	PROXIMITY_DISPLAY	*psProxDisp;
	UWORD			imageID;
	UDWORD			delay = 150;
	UDWORD			i;
	SDWORD width, height;
	int		x = 0, y = 0;
	static const uint16_t imagesEnemy[] = {IMAGE_RAD_ENMREAD, IMAGE_RAD_ENM1, IMAGE_RAD_ENM2, IMAGE_RAD_ENM3};
	static const uint16_t imagesResource[] = {IMAGE_RAD_RESREAD, IMAGE_RAD_RES1, IMAGE_RAD_RES2, IMAGE_RAD_RES3};
	static const uint16_t imagesArtifact[] = {IMAGE_RAD_ARTREAD, IMAGE_RAD_ART1, IMAGE_RAD_ART2, IMAGE_RAD_ART3};
	static const uint16_t imagesBurningResource[] = {IMAGE_RAD_BURNRESREAD, IMAGE_RAD_BURNRES1, IMAGE_RAD_BURNRES2, IMAGE_RAD_BURNRES3, IMAGE_RAD_BURNRES4, IMAGE_RAD_BURNRES5, IMAGE_RAD_BURNRES6};
	static const uint16_t *const imagesProxTypes[] = {imagesEnemy, imagesResource, imagesArtifact};

	// store the width & height of the radar/mini-map
	width = scrollMaxX - scrollMinX;
	height = scrollMaxY - scrollMinY;

	// Check if it's time to remove beacons
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		/* Go through all the proximity Displays*/
		for (psProxDisp = apsProxDisp[i]; psProxDisp != NULL; psProxDisp = psProxDisp->psNext)
		{
			if(psProxDisp->psMessage->dataType == MSG_DATA_BEACON)
			{
				MESSAGE		*psCurrMsg = psProxDisp->psMessage;
				VIEWDATA	*pViewData = (VIEWDATA *)psCurrMsg->pViewData;

				ASSERT(pViewData != NULL, "Message without data!");

				if (pViewData->type == VIEW_BEACON)
				{
					ASSERT(pViewData->pData != NULL, "Help message without data!");
					if (pViewData->pData != NULL && (((VIEW_PROXIMITY *)pViewData->pData)->timeAdded + 60000) <= gameTime)
					{
						debug(LOG_MSG, "blip timeout for %d, from %d", i, (((VIEW_PROXIMITY *)pViewData->pData)->sender));
						removeMessage(psCurrMsg, i);	//remove beacon
						break;	//there can only be 1 beacon per player
					}
				}
			}
		}
	}

	/* Go through all the proximity Displays */
	for (psProxDisp = apsProxDisp[selectedPlayer]; psProxDisp != NULL; psProxDisp = psProxDisp->psNext)
	{
		unsigned        animationLength = ARRAY_SIZE(imagesEnemy) - 1;  // Same size as imagesResource and imagesArtifact.
		const uint16_t *images;

		if (psProxDisp->psMessage->player != selectedPlayer)
		{
			continue;
		}

		if (psProxDisp->type == POS_PROXDATA)
		{
			PROX_TYPE proxType = ((VIEW_PROXIMITY*)((VIEWDATA *)psProxDisp->psMessage->pViewData)->pData)->proxType;
			images = imagesProxTypes[proxType];
		}
		else
		{
			FEATURE *psFeature = (FEATURE *)psProxDisp->psMessage->pViewData;

			ASSERT(psFeature && psFeature->psStats, "Bad feature message");
			if (psFeature && psFeature->psStats && psFeature->psStats->subType == FEAT_OIL_RESOURCE)
			{
				images = imagesResource;
				if (fireOnLocation(psFeature->pos.x, psFeature->pos.y))
				{
					images = imagesBurningResource;
					animationLength = ARRAY_SIZE(imagesBurningResource) - 1;  // Longer animation for burning oil wells.
				}
			}
			else
			{
				images = imagesArtifact;
			}
		}

		// Draw the 'blips' on the radar - use same timings as radar blips if the message is read - don't animate
		if (psProxDisp->psMessage->read)
		{
			imageID = images[0];
		}
		else
		{
			// Draw animated
			if (realTime - psProxDisp->timeLastDrawn > delay)
			{
				++psProxDisp->strobe;
				psProxDisp->timeLastDrawn = realTime;
			}
			psProxDisp->strobe %= animationLength;
			imageID = images[1 + psProxDisp->strobe];
		}

		if (psProxDisp->type == POS_PROXDATA)
		{
			VIEW_PROXIMITY *psViewProx = (VIEW_PROXIMITY *)((VIEWDATA *)psProxDisp->psMessage->pViewData)->pData;

			x = (psViewProx->x / TILE_UNITS - scrollMinX) * pixSizeH;
			y = (psViewProx->y / TILE_UNITS - scrollMinY) * pixSizeV;
		}
		else if (psProxDisp->type == POS_PROXOBJ)
		{
			x = (((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.x / TILE_UNITS - scrollMinX) * pixSizeH;
			y = (((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.y / TILE_UNITS - scrollMinY) * pixSizeV;
		}
		else
		{
			ASSERT(false, "Bad message type");
			continue;
		}

		// NOTE:  On certain missions (limbo & expand), there is still valid data that is stored outside the
		// normal radar/mini-map view.  We must now calculate the radar/mini-map's bounding box, and clip
		// everything outside the box.
		if ( (x+radarX) < width*pixSizeV/2 && (x+radarX) > -width*pixSizeV/2
			&& (y+radarY) < height*pixSizeH/2 && (y+radarY) > -height*pixSizeH/2)
		{
			// Draw the 'blip'
			iV_DrawImage(IntImages, imageID, x + radarX, y + radarY);
		}
	}
	if (audio_GetPreviousQueueTrackRadarBlipPos(&x, &y))
	{
		unsigned        animationLength = ARRAY_SIZE(imagesEnemy) - 1;
		int             strobe = (realTime/delay) % animationLength;
		x = (x / TILE_UNITS - scrollMinX) * pixSizeH;
		y = (y / TILE_UNITS - scrollMinY) * pixSizeV;
		imageID = imagesEnemy[strobe];

		// NOTE:  On certain missions (limbo & expand), there is still valid data that is stored outside the
		// normal radar/mini-map view.  We must now calculate the radar/mini-map's bounding box, and clip
		// everything outside the box.
		if ( (x+radarX) < width*pixSizeV/2 && (x+radarX) > -width*pixSizeV/2
			&& (y+radarY) < height*pixSizeH/2 && (y+radarY) > -height*pixSizeH/2)
		{
			// Draw the 'blip'
			iV_DrawImage(IntImages, imageID, x + radarX, y + radarY);
		}
	}
}


/*Displays the proximity messages blips over the world*/
void intDisplayProximityBlips(WIDGET *psWidget, WZ_DECL_UNUSED UDWORD xOffset,
					WZ_DECL_UNUSED UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_CLICKFORM			*psButton = (W_CLICKFORM*)psWidget;
	PROXIMITY_DISPLAY	*psProxDisp = (PROXIMITY_DISPLAY *)psButton->pUserData;
	MESSAGE				*psMsg = psProxDisp->psMessage;
	SDWORD				x = 0, y = 0;

	ASSERT( psMsg->type == MSG_PROXIMITY, "Invalid message type" );

	//if no data - ignore message
	if (psMsg->pViewData == NULL || psMsg->player != selectedPlayer)
	{
		return;
	}
	if (psProxDisp->type == POS_PROXDATA)
	{
		x = ((VIEW_PROXIMITY*)((VIEWDATA *)psProxDisp->psMessage->pViewData)->pData)->x;
		y = ((VIEW_PROXIMITY*)((VIEWDATA *)psProxDisp->psMessage->pViewData)->pData)->y;
	}
	else if (psProxDisp->type == POS_PROXOBJ)
	{
		x = ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.x;
		y = ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.y;
	}

	//if not within view ignore message
	if (!clipXY(x, y))
	{
		return;
	}

	//if the message is read - don't draw
	if (!psMsg->read)
	{
		//set the button's x/y so that can be clicked on
		psButton->x = (SWORD)(psProxDisp->screenX - psButton->width/2);
		psButton->y = (SWORD)(psProxDisp->screenY - psButton->height/2);
	}
}


static UDWORD sliderMousePos(	W_SLIDER *Slider )
{
	return (widgGetFromID(psWScreen,Slider->formID)->x + Slider->x)
			+ ((Slider->pos * Slider->width) / Slider->numStops );
}


static UWORD sliderMouseUnit(W_SLIDER *Slider)
{
	UWORD posStops = (UWORD)(Slider->numStops / 20);

	if(posStops==0 || Slider->pos == 0 || Slider->pos == Slider->numStops)
	{
		return 1;
	}

	if(Slider->pos < posStops)
	{
		return (Slider->pos);
	}

	if(Slider->pos > (Slider->numStops-posStops))
	{
		return (UWORD)(Slider->numStops-Slider->pos);
	}
	return posStops;
}

void intUpdateQuantitySlider(WIDGET *psWidget, W_CONTEXT *psContext)
{
	W_SLIDER *Slider = (W_SLIDER*)psWidget;

	if(Slider->state & SLD_HILITE)
	{
		if(keyDown(KEY_LEFTARROW))
		{
			if(Slider->pos > 0)
			{
				Slider->pos = (UWORD)(Slider->pos - sliderMouseUnit(Slider));
				setMousePos(sliderMousePos(Slider), mouseY());	// move mouse
			}
		}
		else if(keyDown(KEY_RIGHTARROW))
		{
			if(Slider->pos < Slider->numStops)
			{
				Slider->pos = (UWORD)(Slider->pos + sliderMouseUnit(Slider));
				setMousePos(sliderMousePos(Slider), mouseY());	// move mouse
			}
		}
	}
}

void intDisplayResSubGroup(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_LABEL		*Label = (W_LABEL*)psWidget;
	UDWORD		x = Label->x + xOffset;
	UDWORD		y = Label->y + yOffset;
	RESEARCH    *psResearch = (RESEARCH *)Label->pUserData;

	if (psResearch->subGroup != NO_RESEARCH_ICON)
	{
		iV_DrawImage(IntImages, psResearch->subGroup, x, y);
	}
}

void intDisplayAllyIcon(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_LABEL		*Label =  (W_LABEL*)psWidget;
	UDWORD		x = Label->x + xOffset;
	UDWORD		y = Label->y + yOffset;

	iV_DrawImageTc(IntImages, IMAGE_ALLY_RESEARCH, IMAGE_ALLY_RESEARCH_TC, x, y, pal_GetTeamColour(getPlayerColour(psWidget->UserData)));
}

void intDisplayAllyBar(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	W_BARGRAPH *psBar = (W_BARGRAPH *)psWidget;
	unsigned bestCompletion = 0;
	const int researchNotStarted = 3600000;
	int bestPowerNeeded = researchNotStarted;
	int bestTimeToResearch = researchNotStarted;
	int researchPowerCost = researchNotStarted;
	for (int player = 0; player < MAX_PLAYERS; ++player)
	{
		if (player != selectedPlayer && aiCheckAlliances(selectedPlayer, player))
		{
			// Check each research facility to see if they are doing this topic. (As opposed to having started the topic, but stopped researching it.)
			for (STRUCTURE *psOtherStruct = apsStructLists[player]; psOtherStruct; psOtherStruct = psOtherStruct->psNext)
			{
				RESEARCH_FACILITY *res = (RESEARCH_FACILITY*)psOtherStruct->pFunctionality;
				if (psOtherStruct->pStructureType->type == REF_RESEARCH && psOtherStruct->status == SS_BUILT &&
				    res->psSubject && res->psSubject->ref == asResearch[psWidget->UserData].ref)
				{
					unsigned completion = asPlayerResList[player][psWidget->UserData].currentPoints;
					if (bestCompletion < completion)
					{
						bestCompletion = completion;
						bestPowerNeeded = 0;
						psBar->majorCol = pal_GetTeamColour(getPlayerColour(player));
					}

					int powerNeeded = checkPowerRequest(psOtherStruct);
					if (powerNeeded == -1)
					{
						int rindex = res->psSubject->index;
						int timeToResearch = (res->psSubject->researchPoints - asPlayerResList[player][rindex].currentPoints) / std::max(res->researchPoints, 1u);
						bestTimeToResearch = std::min(bestTimeToResearch, timeToResearch);
					}
					else
					{
						bestPowerNeeded = std::min(bestPowerNeeded, powerNeeded);
						researchPowerCost = res->psSubject->researchPower;
					}
					break;
				}
			}
		}
	}

	if (bestTimeToResearch != researchNotStarted)
	{
		// Show research progress.
		formatTimeText(psBar, bestTimeToResearch);
		setBarGraphValue(psBar, psBar->majorCol, bestCompletion, asResearch[psWidget->UserData].researchPoints);
	}
	else if (bestPowerNeeded != researchNotStarted)
	{
		// Show how much power is needed, before research can start.
		formatPowerText(psBar, bestPowerNeeded);
		setBarGraphValue(psBar, psBar->majorCol, researchPowerCost - bestPowerNeeded, researchPowerCost);
	}
	barGraphDisplayTrough(psWidget, xOffset, yOffset, pColours);
}

/* Set the shadow power for the selected player */
void intSetShadowPower(int quantity)
{
	ManuPower = quantity;
}
